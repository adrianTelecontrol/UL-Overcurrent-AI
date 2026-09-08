
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <inc/hw_epi.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_ssi.h>
#include <inc/hw_types.h>

#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "drivers/pinout.h"

#include "utils/uartstdio.h"

#include "build_config.h"
#include "gpu_ft81x.h"
#include "FT8xx_params.h"
#include "gui_core.h"
#include "helpers.h"
#include "font_engine.h"
#include "forms_manager.h"
#include "gesture_engine.h"
#include "render.h"
#include "hal_tft_spi.h"
#include "hal_sdram.h"
#include "hal_usd.h"
#include "tiva_log.h"
#include "event_engine.h"
#include "video_engine.h"
#include "hal_eeprom.h"
#include "rtc_module.h"
#include "test/control_sim.h"
#include "hal_usb.h"
#include "file_manager.h"
#include "sys_manager.h"
#include "hal_inst_can.h"
#include "inst_can_buffer.h"
#include "experiments_cfg.h"
#include "data_model.h"

#include "gui_theme.h"


// Application specifics
#define APP_VERSION_MAJOR 1
#define APP_VERSION_MINOR 0 
#define APP_VERSION_PATCH 0 
const char *APP_NAME = "UL-1 Overcurrent Tester";
const char *BOARD_VER = "TivaBoard Ver 2.2";
const char *FIRMWARE_VER = "Firmware 1.0.0";

const uint32_t g_ui32SysClock = 120E6;

static const char TASK_NAME[] = "main_task";
volatile uint32_t g_ui32AppFatalErrorCode = 0;

#define APP_FATAL_SDRAM 1U
#define APP_FATAL_RENDER_INIT 2U

#ifdef DEBUG
void __error__() {}
#endif

// The EPI is configurated before entering the main so that the SDRAM
// is available before the main program starts to execute
int _system_pre_init(void) {
  MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                          SYSCTL_CFG_VCO_240),
                         g_ui32SysClock);

  HAL_SDRAM_ConfigureEPI();
  return 1;
}

// Configure the UART port used for logging
void ConfigureUART(void) {
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

  MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
  MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
  MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

  UARTStdioConfig(0, 115200, g_ui32SysClock);
}

static void App_FatalHalt(uint32_t ui32ErrorCode) {
  g_ui32AppFatalErrorCode = ui32ErrorCode;
  while (1) {
  }
}

int main(void) {
  	// Enable all the ports
  	PinoutSet(false, false);

	if (!HAL_SDRAM_RunSelfTest()) {
		App_FatalHalt(APP_FATAL_SDRAM);
	}

	// The SysTick is a requirement for the uSD card
  	MAP_SysTickPeriodSet(g_ui32SysClock / 1000);
  	MAP_SysTickEnable();
  	MAP_SysTickIntEnable();

  	// Configure the UART for system logging
  	//ConfigureUART();

	HAL_CAN_Init(g_ui32SysClock, 250E3);

  	TIVA_LOGI(TASK_NAME, "Starting application...");

  	// Init RTC
  	if( !RTC_initModule() ) {
		TIVA_LOGE(TASK_NAME, "Error: Failed to initialize RTC!");
	} else {
		TIVA_LOGI(TASK_NAME, "RTC module initialized correctly!");
	}

  	// Init USB
  	if( !HAL_USB_Init() ) {
		TIVA_LOGE(TASK_NAME, "Error: Failed to initialize USB");
	} else {
		TIVA_LOGI(TASK_NAME, "USB initialized correctly");
	}

	// Init SD card
  	if (!HAL_uSD_init()) {
		TIVA_LOGE(TASK_NAME, "Error: Failed to open SD card");
  	} else {
		TIVA_LOGI(TASK_NAME, "uSD initialized correctly!");
	}

	// Init graphics engine
	if (!Render_Init(LCD_WIDTH, LCD_HEIGHT)) {
		App_FatalHalt(APP_FATAL_RENDER_INIT);
	}

	// Init FT81x SPI communication
  	HAL_TFT_SPI_Init();

	// Wake up the screen
  	API_WakeUpScreen();
	
	// EERPOM init
  	if(!HAL_EEPROM_init()) {
		TIVA_LOGE(TASK_NAME, "Error: Failed to initialize EEPROM");
	}
  	else {
		TIVA_LOGI(TASK_NAME, "EEPROM initialized correctly!");
	} 

	// Init gesture engine
	GestureEngine_Init();

	// Calibrate screen if needed
  	GestureEngine_CalibrateScreen();

	// The DWT will be our clock source
  	StartCycleCounter(); 

	// Initialize the UI theme
	// This is done in two steps to divide the loading times
  	Theme_Init(true);
  	Theme_SetModeHigh(true);

  	// Play splash screen 
	#ifdef ENABLE_SPLASH_SCR
  	EVE_VideoResult_t eResult = EVE_Video_Play(
	 	 DRIVE_SD_ID,
  	    //"s_hd.avi",
  	    "splash_nw.avi",	// no watermark
		//"s_hd_1280.avi",
  	    OPT_FULLSCREEN | OPT_NOTEAR | OPT_MEDIAFIFO
  	);
  
  	if (eResult != EVE_VIDEO_OK) {
  	    TIVA_LOGE("MAIN", "Video failed: %s", EVE_Video_ResultStr(eResult));
  	}
	#endif

	// Loead the other half of the theme
  	Theme_SetModeLow(true);

	// Init Form Manager
  	FormManager_Init();

  	// Test Unit
  	//controlSimulatorInit();

  	// Composite initial full frame
  	FormManager_CompositeFrame(g_pDrawingBuffer);

	// Send the initial full frame
	Render_SendFullFrame(true);
	Render_DisplayFrame();

	// System manager init: contains the global state machine
	SysManager_Init();

	// The data model contains the current values of the sensors
	DataModel_Init();

	// Experiments configuration: handles the current configuration values for the tests
	ExperimentCfg_init();

  	// Event_Post(EVT_CMD_START_BOOT_SEQ, (EventParam_t){.ptr = NULL});

  	while (1) {
		// Instrumentation manager task
		InstManager_Task();
		// The backend of the system
		SysManager_Task();
		// Dispatch the pending events	
		Event_Dispatch();
		// Handles the composition and transmision of the frame
		Render_Task();
		// Handles the touch input and gesture detection
		GestureEngine_Task();
		// Handle the USB events
		HAL_USB_Task();
		// Just for showcase and debug purpuses
		//controlSimulatiorTask();
  	} 

}


