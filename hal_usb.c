#include "hal_usb.h"

// TivaWare e Includes de Hardware
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include <fatfs/src/ff.h>
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"

// TivaWare USB Library
#include "usblib/usblib.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhmsc.h"

// Integración con tu arquitectura
#include "event_engine.h" // Para disparar eventos a la GUI
#include "tiva_log.h"     // Asumiendo que usas esto para debugear
#include "file_manager.h"

#ifndef DRIVE_SD
#define DRIVE_SD  0
#define DRIVE_USB 1
#endif

// --- MACROS Y VARIABLES GLOBALES DEL USB ---
#define HCD_MEMORY_SIZE 128
static uint8_t g_pHCDPool[HCD_MEMORY_SIZE]; // Memoria de trabajo del controlador

// Instancia global del Mass Storage Class (usada por fat_usbmsc.c)
tUSBHMSCInstance *g_psMSCInstance = NULL;

static volatile bool g_bUSBIsReady = false;
// static const char *TAG = "HAL_USB";

extern const uint32_t g_ui32SysClock;

static FATFS g_sUSBFatFs;

static const char TAG[] = "halUSB";
// =====================================================================
// CALLBACKS DE EVENTOS USB
// =====================================================================

// Callback para eventos genéricos del Host (Errores de energía, etc.)
void USBHCDEvents(void *pvData) {
    tEventInfo *pEventInfo = (tEventInfo *)pvData;

    switch (pEventInfo->ui32Event) {
        case USB_EVENT_POWER_FAULT:
            g_bUSBIsReady = false;
		    Event_Post(EVT_SYS_USB_POWER_FAULT, ( EventParam_t ){.ptr = NULL});
            break;
        case USB_EVENT_UNKNOWN_CONNECTED:
            g_bUSBIsReady = false;
		    Event_Post(EVT_SYS_USB_UNKNOWN_DEVICE, ( EventParam_t ){.ptr = NULL});
            break;
        default:
            break;
    }
}

void MSCCallback(tUSBHMSCInstance *ps32Instance, uint32_t ui32Event,
                 void *pvData)
{
    switch (ui32Event)
    {
    	case MSC_EVENT_OPEN:
    	{
            g_bUSBIsReady = true;
            
            // Disparamos un evento para que la UI muestre un ícono de USB, por ejemplo
			Event_Post(EVT_SYS_USB_CONNECTED, (EventParam_t ){.ptr = NULL});
    	    break;
    	}

    	case MSC_EVENT_CLOSE:
    	{
            g_bUSBIsReady = false;
            
            // Avisar a la GUI para que cambie el ícono o aborte transferencias
            // Event_Post(EVT_SYS_USB_DISCONNECTED, (EventParam_t){ .i32 = 0 });
			Event_Post(EVT_SYS_USB_DISCONNECTED, (EventParam_t ){.ptr = NULL});

    	    break;
    	}

    	default:
    	{
    	    break;
    	}
    }
}

// Registro de Drivers de Clase soportados por nuestro Host
DECLARE_EVENT_DRIVER(g_sUSBEventDriver, 0, 0, USBHCDEvents);
static tUSBHostClassDriver const * const g_ppHostClassDrivers[] = {
    &g_sUSBHostMSCClassDriver,
    &g_sUSBEventDriver
};

const uint32_t g_ui32NumHostClassDrivers = 
      sizeof(g_ppHostClassDrivers) / sizeof(tUSBHostClassDriver *);

// =====================================================================
// FUNCIONES PÚBLICAS (API)
// =====================================================================

bool HAL_USB_Init(void) {
    // 1. Habilitar periféricos necesarios
    SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);

    // Esperar a que los periféricos estén listos
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_USB0) || 
          !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOQ));

    // 3. Configurar Pines Digitales de Control de Energía (EPEN, PFLT)
    // Para encender y monitorear el switch de 5V del USB Host
	HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTD_BASE + GPIO_O_CR) = 0xff;
    MAP_GPIOPinConfigure(GPIO_PD6_USB0EPEN);
    MAP_GPIOPinTypeUSBAnalog(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    MAP_GPIOPinTypeUSBDigital(GPIO_PORTD_BASE, GPIO_PIN_6);
    MAP_GPIOPinTypeUSBAnalog(GPIO_PORTL_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    MAP_GPIOPinTypeGPIOInput(GPIO_PORTQ_BASE, GPIO_PIN_4);

    // 4. Configurar el Reloj y Modo Host del USB
    // TivaC USB requiere reloj de PLL a 480 MHz o la frecuencia que estés usando
    USBStackModeSet(0, eUSBModeHost, 0);

	USBHCDRegisterDrivers(0, g_ppHostClassDrivers, g_ui32NumHostClassDrivers);


    // 6. Abrir la instancia del driver MSC (Mass Storage Class)
    g_psMSCInstance = USBHMSCDriveOpen(0, MSCCallback);
	
	USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH | USBHCD_VBUS_FILTER);
	
	//
    // Tell the USB library the CPU clock and the PLL frequency.  This is a
    // new requirement for TM4C129 devices.
    //
	uint32_t ui32PLLRate = 0;
    SysCtlVCOGet(SYSCTL_XTAL_25MHZ, &ui32PLLRate);
    USBHCDFeatureSet(0, USBLIB_FEATURE_CPUCLK, (void *)&g_ui32SysClock);
    USBHCDFeatureSet(0, USBLIB_FEATURE_USBPLL, &ui32PLLRate);

    // 5. Inicializar el Host Controller Driver (HCD)
    // OJO: Esta función habilitará la interrupción INT_USB0 internamente
    USBHCDInit(0, g_pHCDPool, HCD_MEMORY_SIZE);

	FRESULT result = f_mount(1, &g_sUSBFatFs);

	if(result != FR_OK) {
		TIVA_LOGI(TAG, "Cannot mount USB drive! Error: %s", FM_StringFromFResult(result));
		return false;
	} else {
		TIVA_LOGI(TAG, "USB mounted correctly!");
	}

    return true;
}



void HAL_USB_Task(void) {
    // Procesa la máquina de estados del hardware USB.
    // Esto detecta si se conectó/desconectó algo y lee descriptores.
    USBHCDMain();
}

bool HAL_USB_IsReady(void) {
    return g_bUSBIsReady;
}


