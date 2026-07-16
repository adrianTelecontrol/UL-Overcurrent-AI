/*
 * pantallaFT8xx.h
 *
 *  Created on: 22 jul. 2020
 *      Author: Zerch
 */

#ifndef CONFIG_SCREEN_FT8XX_H_
#define CONFIG_SCREEN_FT8XX_H_

#include "stdint.h"
//---------------------------------
//----Configuraci�n para la API----
//---------------------------------
// WQVGA display parameters

//#define LCD_800_480
//#define LCD_480_272
//#define LCD_320_240
#define LCD_800_480_4

#ifdef LCD_800_480
#define LCD_WIDTH           800     // Active width of LCD display
#define LCD_HEIGHT          480     // Active height of LCD display
#define LCD_X               1     // Random value added to porch?
#define LCD_Y               1     // Random value added to porch?
#define LCD_SYSTEM_CLK      48     //Value in MHz
#define LCD_SCREEN_CLK      12     //Value in MHz
#define LCD_HFRONT_PORCH    LCD_X+4     //Value en clk or pixels
#define LCD_HBACK_PORCH     2     //Value en clk or pixels
//#define LCD_HPULSE_WIDTH    4     //Value en clk or pixels
#define LCD_VFRONT_PORCH    LCD_Y+4     //Value en clk or pixels
#define LCD_VBACK_PORCH     2     //Value en clk or pixels
//#define LCD_VPULSE_WIDTH    4     //Value en clk or pixels

#define LCD_H_CYCLE     LCD_WIDTH+LCD_HFRONT_PORCH+LCD_HBACK_PORCH+LCD_X     // Total number of clocks per line
#define LCD_H_SYNC_0    LCD_HFRONT_PORCH-LCD_X   // Start of horizontal sync pulse
#define LCD_H_SYNC_1    LCD_H_SYNC_0*2      // End of horizontal sync pulse
#define LCD_H_OFFSET    LCD_H_SYNC_1+LCD_HBACK_PORCH      // Start of active line
#define LCD_V_CYCLE     LCD_HEIGHT+LCD_VFRONT_PORCH+LCD_VBACK_PORCH+LCD_Y      // Total number of lines per screen
#define LCD_V_SYNC_0    LCD_VFRONT_PORCH-LCD_Y       // Start of vertical sync pulse
#define LCD_V_SYNC_1    LCD_V_SYNC_0*2        // End of vertical sync pulse
#define LCD_V_OFFSET    LCD_V_SYNC_1+LCD_VBACK_PORCH      // Start of active screen
#define LCD_P_CLK       LCD_SYSTEM_CLK/LCD_SCREEN_CLK       // Pixel Clock
#define LCD_SWIZZLE     0       // Define RGB output pins
#define LCD_P_CLK_POL   1       // Define active edge of PCLK
#endif

#ifdef LCD_480_272
#define LCD_WIDTH       480     // Active width of LCD display
#define LCD_HEIGHT      272     // Active height of LCD display
#define LCD_X               4     // Random value added to porch?
#define LCD_Y               4     // Random value added to porch?
#define LCD_SYSTEM_CLK      48     //Value in MHz
#define LCD_SCREEN_CLK      8     //Value in MHz
#define LCD_HFRONT_PORCH    LCD_X+4     //Value en clk or pixels
#define LCD_HBACK_PORCH     43     //Value en clk or pixels
//#define LCD_HPULSE_WIDTH    4     //Value en clk or pixels
#define LCD_VFRONT_PORCH    LCD_Y+4     //Value en clk or pixels
#define LCD_VBACK_PORCH     12     //Value en clk or pixels
//#define LCD_VPULSE_WIDTH    4     //Value en clk or pixels

#define LCD_H_CYCLE     LCD_WIDTH+LCD_HFRONT_PORCH+LCD_HBACK_PORCH+LCD_X     // Total number of clocks per line
#define LCD_H_SYNC_0    LCD_HFRONT_PORCH-LCD_X   // Start of horizontal sync pulse
#define LCD_H_SYNC_1    LCD_H_SYNC_0*2      // End of horizontal sync pulse
#define LCD_H_OFFSET    LCD_H_SYNC_1+LCD_HBACK_PORCH      // Start of active line
#define LCD_V_CYCLE     LCD_HEIGHT+LCD_VFRONT_PORCH+LCD_VBACK_PORCH+LCD_Y      // Total number of lines per screen
#define LCD_V_SYNC_0    LCD_VFRONT_PORCH-LCD_Y       // Start of vertical sync pulse
#define LCD_V_SYNC_1    LCD_V_SYNC_0*2        // End of vertical sync pulse
#define LCD_V_OFFSET    LCD_V_SYNC_1+LCD_VBACK_PORCH      // Start of active screen
#define LCD_P_CLK       LCD_SYSTEM_CLK/LCD_SCREEN_CLK       // Pixel Clock
#define LCD_SWIZZLE     0       // Define RGB output pins
#define LCD_P_CLK_POL   1       // Define active edge of PCLK
#endif

#ifdef LCD_800_480_4
#define LCD_WIDTH       800     // Active width of LCD display
#define LCD_HEIGHT      480     // Active height of LCD display


#define LCD_SYSTEM_CLK      48     //Value in MHz
#define LCD_SCREEN_CLK      12     //Value in MHz

#define LCD_H_OFFSET    75      // Start of active line
#define LCD_H_SYNC_0    16       // Start of horizontal sync pulse
#define LCD_H_SYNC_1    LCD_H_SYNC_0*2      // End of horizontal sync pulse
#define LCD_H_CYCLE     LCD_WIDTH+LCD_H_OFFSET+LCD_H_SYNC_0+LCD_H_SYNC_1     // Total number of clocks per line

#define LCD_V_OFFSET    15      // Start of active screen
#define LCD_V_SYNC_0    3       // Start of vertical sync pulse
#define LCD_V_SYNC_1    LCD_V_SYNC_0*2       // End of vertical sync pulse
#define LCD_V_CYCLE     LCD_HEIGHT+LCD_V_OFFSET+LCD_V_SYNC_0+LCD_V_SYNC_1     // Total number of lines per screen

#define LCD_P_CLK       LCD_SYSTEM_CLK/LCD_SCREEN_CLK       // Pixel Clock
#define LCD_SWIZZLE     0       // Define RGB output pins
#define LCD_P_CLK_POL   1       // Define active edge of PCLK
#endif

//Pin asociado a las interrupciones
// de la pantalla
#define INT_TOUCH_SCREEN        GPIO_PIN_6
//bits de interrupci�n
#define INT_BIT_SWAP            (0x0001)
#define INT_BIT_TOUCH           (0x0002)
#define INT_BIT_TAG             (0x0004)
#define INT_BIT_SOUND           (0x0008)
#define INT_BIT_PLAYBACK        (0x0010)
#define INT_BIT_CMDEMPTY        (0x0020)
#define INT_BIT_CMDFLAG         (0x0040)
#define INT_BIT_CONVCOMPLETE    (0x0080)

//*****************************************************************************
//
//                          Host command transaction 
// 
//*****************************************************************************

//
// Host commands: Power mode
//

// Switch to active mode. Dummy memory read from address (read twice).
#define FT81x_HOST_CMD_ACTIVE   0x00
// Put FT81x core to standby mode. ACTIVE cmd to wake up
#define FT81x_HOST_CMD_STANDBY  0x41
// Put FT81x core to sleep mode. ACTIVE cmd to wake up
#define FT81x_HOST_CMD_SLEEP    0x42
// Switch off 1.2V core voltage to the difital core circuits.
#define FT81x_HOST_CMD_PWRDOWN  0x43
// Select power down individual ROMs
#define FT81x_HOST_CMD_PD_ROMS  0x49

//
// Host commands: Clock and reset
//

// Select PLL input from external crystal osc or external clk 
#define FT81x_HOST_CMD_CLKEXT    0x44
// Select PLL input from internal relaxation osc (default)
#define FT81x_HOST_CMD_CLKINT    0x48
// This cmd will only be effective when PLL is stopped
#define FT81x_HOST_CMD_CLKSEL    0x61
// Send reset pulse to FT81x core. Setting through SPI won't be affected

//
// Host commands: Configuration 
//

// This will set the drive strenght for various pins.
#define FT81x_HOST_CMD_PIN_DRIVE 0x70
//During power down, all output and in/out pins will not be driven
#define FT81x_HOST_CMD_PIN_PD_STATE    0x71

#define FT81x_HOST_CMD_CHIP_ID_CODE     0x00000C


//*****************************************************************************
//
//                          Host command parameters 
// 
//*****************************************************************************

#define FT81x_HOST_PARAM_EMPTY              0x00 

// CLock select params
#define FT81x_HOST_PARAM_CLK_DEFAULT        0x00    // 60MHz

// Power Down cmd params
#define FT81x_HOST_PARAM_PD_ROM_MAIN        0b10000000
#define FT81x_HOST_PARAM_PD_ROM_RCOSATAN    0b01000000
#define FT81x_HOST_PARAM_PD_ROM_SAMPLE      0b00100000
#define FT81x_HOST_PARAM_PD_ROM_JABOOT      0b00010000
#define FT81x_HOST_PARAM_PD_ROM_J1BOOT      0b00001000


#define FT81x_CHIP_ID_VALUE                  0x7C
#endif /* PANTALLAFT8XX_H_ */
