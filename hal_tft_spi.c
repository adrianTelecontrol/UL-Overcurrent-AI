/*
 * hal_spi.c
 *
 *  Created on: 19/02/26
 *      Author: Adrian
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <driverlib/gpio.h>
#include <driverlib/interrupt.h>
#include <driverlib/pin_map.h>
#include <driverlib/rom_map.h>
#include <driverlib/ssi.h>
#include <driverlib/sysctl.h>
#include <driverlib/udma.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_ssi.h>
#include <inc/hw_types.h>

#include "render.h"
#include "hal_tft_spi.h"
#include <helpers.h>


// --- MEMORY ALIGNMENT ---
// Matches the official example lines 138-146
#if defined(ewarm)
#pragma data_alignment = 1024
uint8_t g_HAL_uDMA_ControlTable[1024];
#elif defined(ccs)
#pragma DATA_ALIGN(g_HAL_uDMA_ControlTable, 1024)
uint8_t g_HAL_uDMA_ControlTable[1024];
#else
uint8_t g_HAL_uDMA_ControlTable[1024] __attribute__((aligned(1024)));
#endif

extern const uint32_t g_ui32SysClock; // Get system clock from PLL

volatile bool g_bSPI_TransferActive = false;
static const volatile uint8_t g_dummyTxZero = 0;
volatile uint32_t g_ui32ExecDurMs;

bool g_bIsQuadActive = false;

// SSI Interrupt handler WORKING
/*
void SSI3IntHandler(void) {
  // 1. Read and clear the interrupt status
  uint32_t ui32Status = MAP_SSIIntStatus(SSI3_BASE, 1);
  MAP_SSIIntClear(SSI3_BASE, ui32Status);

  // 2. Check if this interrupt is because the DMA transfer finished
  if (g_bSPI_TransferActive && !MAP_uDMAChannelIsEnabled(UDMA_CH15_SSI3TX)) {

    // Wait for the final few bits to shift out of the SPI hardware
    while (MAP_SSIBusy(SSI3_BASE))
      ;

    // CRITICAL: Close the EVE SPI transaction!
    HAL_TFT_SPI_CS_Disable();

    // Clean up DMA and disable this interrupt until the next frame
    MAP_SSIDMADisable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);
    MAP_SSIIntDisable(SSI3_BASE, SSI_TXFF);

    // Signal to the CPU that the DMA is free!
    g_bSPI_TransferActive = false;

  }

  // Safety catch for RX Overruns
  if (ui32Status & SSI_RXOR) {
    MAP_SSIIntClear(SSI3_BASE, SSI_RXOR);
  }
}*/

bool HAL_TFT_SPI_IsBusy(void) { return g_bSPI_TransferActive; }

uint8_t HAL_TFT_SPI_ReadWrite8(uint16_t txData) {
  uint32_t ui32RxData;
  SSIDataPut(SSI3_BASE, txData);

  while (SSIBusy(SSI3_BASE)) {
  }

  SSIDataGet(SSI3_BASE, &ui32RxData);

  return (uint8_t)(ui32RxData & 0xFF);
}

// --- INIT ---
// 1. MUST be static to survive function return in non-blocking mode
static uint32_t g_dummyRxByte; 
// static uint32_t g_dummyTxZero = 0;

bool HAL_TFT_SPI_uDMATransfer(const uint8_t *pTxBuffer, uint8_t *pRxBuffer,
                          uint32_t count, bool bIsBlocking) {
  // Límite de hardware del uDMA en modo básico
  if (count == 0 || count > 1024) {
      return false; 
  }

  // Identificar si estamos haciendo una escritura pura en QuadSPI
  bool bIsQuadWrite = g_bIsQuadActive && (pRxBuffer == NULL);

  MAP_SSIIntDisable(SSI3_BASE, SSI_DMATX | SSI_DMARX | SSI_RXFF | SSI_RXOR | SSI_RXTO);
  MAP_SSIIntClear(SSI3_BASE, SSI_DMATX | SSI_DMARX | SSI_RXTO | SSI_RXOR);

  MAP_uDMAChannelDisable(UDMA_CH14_SSI3RX);
  MAP_uDMAChannelDisable(UDMA_CH15_SSI3TX);

  // Limpiar basura del FIFO RX
  uint32_t garbage;
  while (MAP_SSIDataGetNonBlocking(SSI3_BASE, &garbage));

  // ---------------------------------------------------------
  // CONFIGURACIÓN TX (Siempre requerida)
  // ---------------------------------------------------------
  void *pSrc = (pTxBuffer != NULL) ? (void *)pTxBuffer : (void *)&g_dummyTxZero;
  uint32_t srcInc = (pTxBuffer != NULL) ? UDMA_SRC_INC_8 : UDMA_SRC_INC_NONE;

  MAP_uDMAChannelAttributeDisable(UDMA_CH15_SSI3TX, UDMA_ATTR_ALTSELECT);
  MAP_uDMAChannelControlSet(UDMA_CH15_SSI3TX | UDMA_PRI_SELECT,
                            UDMA_SIZE_8 | srcInc | UDMA_DST_INC_NONE | UDMA_ARB_4);
  MAP_uDMAChannelTransferSet(UDMA_CH15_SSI3TX | UDMA_PRI_SELECT,
                             UDMA_MODE_BASIC, pSrc, (void *)(SSI3_BASE + SSI_O_DR), count);
  MAP_uDMAChannelEnable(UDMA_CH15_SSI3TX);

  // ---------------------------------------------------------
  // CONFIGURACIÓN RX (Excluida en QuadSPI Write)
  // ---------------------------------------------------------
  if (!bIsQuadWrite) {
      void *pDest = (pRxBuffer != NULL) ? (void *)pRxBuffer : (void *)&g_dummyRxByte;
      uint32_t dstInc = (pRxBuffer != NULL) ? UDMA_DST_INC_8 : UDMA_DST_INC_NONE;

      MAP_uDMAChannelAttributeDisable(UDMA_CH14_SSI3RX, UDMA_ATTR_ALTSELECT);
      MAP_uDMAChannelControlSet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT,
                                UDMA_SIZE_8 | UDMA_SRC_INC_NONE | dstInc | UDMA_ARB_4);
      MAP_uDMAChannelTransferSet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT,
                                 UDMA_MODE_BASIC, (void *)(SSI3_BASE + SSI_O_DR), pDest, count);
      MAP_uDMAChannelEnable(UDMA_CH14_SSI3RX);

      // Activar hardware SPI para pedir datos al DMA en ambas direcciones
      MAP_SSIDMAEnable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);
  } else {
      // Activar hardware SPI SOLO para pedir datos de transmisión
      MAP_SSIDMAEnable(SSI3_BASE, SSI_DMA_TX);
  }

  // ---------------------------------------------------------
  // EJECUCIÓN
  // ---------------------------------------------------------
  if (!bIsBlocking) {
    g_bSPI_TransferActive = true;
  } else {
    // Esperar a que el canal TX termine
    while (MAP_uDMAChannelIsEnabled(UDMA_CH15_SSI3TX)) {
        // En Single SPI, proteger contra Overruns
        if (!bIsQuadWrite && (HWREG(SSI3_BASE + SSI_O_RIS) & SSI_RIS_RORRIS)) {
            HWREG(SSI3_BASE + SSI_O_ICR) = SSI_ICR_RORIC;
        }
    }
    
    // Esperar a que los cables físicos terminen de enviar
    while (MAP_SSIBusy(SSI3_BASE));

    // Limpieza
    MAP_uDMAChannelDisable(UDMA_CH15_SSI3TX);
    
    if (!bIsQuadWrite) {
        MAP_uDMAChannelDisable(UDMA_CH14_SSI3RX);
        MAP_SSIDMADisable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);
    } else {
        MAP_SSIDMADisable(SSI3_BASE, SSI_DMA_TX);
    }
    
    g_bSPI_TransferActive = false;
  }

  return true;
}

void uDMA_Init(void) {
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
  while (!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_UDMA))
    ;

  MAP_uDMAEnable();
  MAP_uDMAControlBaseSet(g_HAL_uDMA_ControlTable);

  MAP_uDMAChannelAssign(UDMA_CH14_SSI3RX);
  MAP_uDMAChannelAssign(UDMA_CH15_SSI3TX);
}

void SPI3_Init(void) {
  // Habilitar el periférico SSI3
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);
  while (!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_SSI3))
    ;

  // Habilitar el Puerto Q (CLK, D0, D1)
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
  while (!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOQ))
    ;
    
  // Habilitar el Puerto P (D2, D3) - ¡Corregido a PP0 y PP1!
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
  while (!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOP))
    ;

  // Configurar el multiplexor de pines para SSI3
  MAP_GPIOPinConfigure(GPIO_PQ0_SSI3CLK);
  MAP_GPIOPinConfigure(GPIO_PQ2_SSI3XDAT0);   // D0
  MAP_GPIOPinConfigure(GPIO_PQ3_SSI3XDAT1);   // D1
  MAP_GPIOPinConfigure(GPIO_PP0_SSI3XDAT2);   // D2 (Nuevo IO2 en PP0)
  MAP_GPIOPinConfigure(GPIO_PP1_SSI3XDAT3);   // D3 (Nuevo IO3 en PP1)

  // Configurar los pads y el tipo de pin para el Puerto Q (CLK, D0, D1)
  MAP_GPIOPadConfigSet(GPIO_PORTQ_BASE, GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3,
                       GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);
  MAP_GPIOPinTypeSSI(GPIO_PORTQ_BASE, GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3);

  // Configurar los pads y el tipo de pin para el Puerto P (D2, D3)
  MAP_GPIOPadConfigSet(GPIO_PORTP_BASE, GPIO_PIN_0 | GPIO_PIN_1,
                       GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);
  MAP_GPIOPinTypeSSI(GPIO_PORTP_BASE, GPIO_PIN_0 | GPIO_PIN_1);

  // Configurar SSI en modo Single (se cambiará a Quad más adelante mediante software)
  MAP_SSIConfigSetExpClk(SSI3_BASE, g_ui32SysClock, SSI_FRF_MOTO_MODE_0,
                         SSI_MODE_MASTER, HAL_TFT_SPI_LOW_BITRATE, 8);

  MAP_SSIDMAEnable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);

  // Habilitar Interrupciones (Requeridas para uDMA incluso en modo bloqueante)
  MAP_IntEnable(INT_SSI3);
  MAP_SSIEnable(SSI3_BASE);
}

void HAL_TFT_SPI_SwitchTo_Quad(void) {
    // 1. Ensure the last single-mode transaction is fully complete
    while (MAP_SSIBusy(SSI3_BASE));
    SysCtlDelay(3); // Guard: shift register fully done

    // 2. Drain any garbage left in the RX FIFO from previous transactions
    uint32_t trash;
    while (MAP_SSIDataGetNonBlocking(SSI3_BASE, &trash));

    // 3. Safely disable, switch mode, re-enable
    MAP_SSIDisable(SSI3_BASE);
    SSIAdvModeSet(SSI3_BASE, SSI_ADV_MODE_QUAD_WRITE);
    MAP_SSIEnable(SSI3_BASE);

    // 4. Bus starts in TX direction (QUAD_WRITE)
    HAL_TFT_SPI_TX();

    g_bIsQuadActive = true;
}

void HAL_TFT_SPI_SetHighSpeed(void) {
    // Ensure bus is idle before touching the peripheral
    while (MAP_SSIBusy(SSI3_BASE));
    SysCtlDelay(3);

    uint32_t trash;
    while (MAP_SSIDataGetNonBlocking(SSI3_BASE, &trash));

    MAP_SSIDisable(SSI3_BASE);

    // Reconfigure clock speed — this resets control registers
    SSIConfigSetExpClk(SSI3_BASE, g_ui32SysClock, SSI_FRF_MOTO_MODE_0,
                       SSI_MODE_MASTER, HAL_TFT_SPI_HIGH_BITRATE, 8);

    // Re-apply Quad mode if it was active, since SSIConfigSetExpClk wiped it
    if (g_bIsQuadActive) {
        SSIAdvModeSet(SSI3_BASE, SSI_ADV_MODE_QUAD_WRITE);
    }

    MAP_SSIEnable(SSI3_BASE);

    // Ensure buffer direction is correct after the reconfiguration
    if (g_bIsQuadActive) {
        HAL_TFT_SPI_TX();
    } else {
        HAL_TFT_SPI_SingleMode();
    }
}

void HAL_TFT_SPI_Init(void) {
  GPIOPinTypeGPIOOutput(GPIO_PORTM_BASE, HAL_TFT_SPI_PD);
  GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, HAL_TFT_SPI_CS);
  GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, HAL_TFT_SPI_LED);
  GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, HAL_TFT_SPI_DIR1);
  GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, HAL_TFT_SPI_DIR2);
  GPIOPadConfigSet(GPIO_PORTQ_BASE, HAL_TFT_SPI_CS, GPIO_STRENGTH_4MA,
                   GPIO_PIN_TYPE_STD);
  
  GPIOPinWrite(GPIO_PORTK_BASE, HAL_TFT_SPI_DIR1, HAL_GPIO_HIGH);
  GPIOPinWrite(GPIO_PORTK_BASE, HAL_TFT_SPI_DIR2, HAL_GPIO_LOW);

  uDMA_Init();
  SPI3_Init();
}
