/*
 * hal_spi.h
 *
 *  Created on: 19/02/26 
 *      Author: Adrian
 */

#ifndef HAL_SPI_H_
#define HAL_SPI_H_

#include <stdbool.h>
#include <stdint.h>

#include <driverlib/gpio.h>
#include <driverlib/ssi.h>
#include <inc/hw_memmap.h>

// SSI3 selected 
#define HAL_SPI_PORT 	GPIO_PORTQ_BASE
#define HAL_SPI_CLK 	GPIO_PIN_0
#define HAL_SPI_SDO 	GPIO_PIN_2
#define HAL_SPI_SDI 	GPIO_PIN_3

// With 20MHz at high bitrate it works fine
#define HAL_TFT_SPI_LOW_BITRATE		9E6		// 9Mhz
#define HAL_TFT_SPI_HIGH_BITRATE	15E6		// 15Mhz

#define HAL_GPIO_HIGH 	0XFF // Estado alto para los pines
#define HAL_GPIO_LOW 	0X00  // Estado bajo para los pines

#define HAL_TFT_SPI_CS 		GPIO_PIN_1 // PQ1   Chip selector de la pantalla
#define HAL_TFT_SPI_PD 		GPIO_PIN_6 // PM6   pin para despertar a la pantalla
#define HAL_TFT_SPI_LED 	GPIO_PIN_0

#define HAL_TFT_QSPI_PORT	GPIO_PORTK_BASE
#define HAL_TFT_SPI_DIR1 	GPIO_PIN_6
#define HAL_TFT_SPI_DIR2 	GPIO_PIN_7

extern uint8_t g_HAL_uDMA_ControlTable[1024];
extern volatile bool g_bSPI_TransferActive;
extern volatile uint32_t g_ui32ExecDurMs;
extern bool g_bIsQuadActive;

inline void HAL_TFT_SPI_CS_Enable() { GPIOPinWrite(GPIO_PORTQ_BASE, HAL_TFT_SPI_CS, HAL_GPIO_LOW); }
inline void HAL_TFT_SPI_CS_Disable() { GPIOPinWrite(GPIO_PORTQ_BASE, HAL_TFT_SPI_CS, HAL_GPIO_HIGH); }
inline void HAL_TFT_SPI_PD_Low() { GPIOPinWrite(GPIO_PORTM_BASE, HAL_TFT_SPI_PD, HAL_GPIO_LOW); }
inline void HAL_TFT_SPI_PD_High() { GPIOPinWrite(GPIO_PORTM_BASE, HAL_TFT_SPI_PD, HAL_GPIO_HIGH); }
inline void HAL_TFT_SPI_LED_On() {
  GPIOPinWrite(GPIO_PORTN_BASE, HAL_TFT_SPI_LED, HAL_GPIO_HIGH);
}
inline void HAL_TFT_SPI_LED_Off() {
  GPIOPinWrite(GPIO_PORTN_BASE, HAL_TFT_SPI_LED, HAL_GPIO_LOW);
}

inline void HAL_TFT_SPI_RX() {
	GPIOPinWrite(GPIO_PORTK_BASE, HAL_TFT_SPI_DIR1, HAL_GPIO_LOW);
	GPIOPinWrite(GPIO_PORTK_BASE, HAL_TFT_SPI_DIR2, HAL_GPIO_LOW);
}

inline void HAL_TFT_SPI_TX() {
	GPIOPinWrite(GPIO_PORTK_BASE, HAL_TFT_SPI_DIR1, HAL_GPIO_HIGH);
	GPIOPinWrite(GPIO_PORTK_BASE, HAL_TFT_SPI_DIR2, HAL_GPIO_HIGH);
}

inline void HAL_TFT_SPI_SingleMode() {
  	GPIOPinWrite(GPIO_PORTK_BASE, HAL_TFT_SPI_DIR1, HAL_GPIO_HIGH);
  	GPIOPinWrite(GPIO_PORTK_BASE, HAL_TFT_SPI_DIR2, HAL_GPIO_LOW);
}

//
// Function declarations
//
uint8_t HAL_TFT_SPI_ReadWrite8(uint16_t txData);

bool HAL_TFT_SPI_IsBusy(void);

bool HAL_TFT_SPI_uDMATransfer(const uint8_t *pTxBuffer,
                                    uint8_t *pRxBuffer,
                                    uint32_t count , bool bIsBlocking);

void HAL_TFT_SPI_SwitchTo_Quad(void);

void HAL_TFT_SPI_SetHighSpeed(void);

void HAL_TFT_SPI_Init(void);

#endif /* TIVA_SPI_H_ */
