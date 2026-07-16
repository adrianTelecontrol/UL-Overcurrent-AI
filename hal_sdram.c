/**
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_ssi.h>
#include <inc/hw_epi.h>
#include "driverlib/sysctl.h"
#include "driverlib/epi.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"

#include "tiva_log.h"

#include "hal_sdram.h"

//*****************************************************************************
//
// Use the following to specify the GPIO pins used by the SDRAM EPI bus.
//
//*****************************************************************************
#define EPI_PORTA_PINS (GPIO_PIN_7 | GPIO_PIN_6)
#define EPI_PORTB_PINS (GPIO_PIN_3)
#define EPI_PORTC_PINS (GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4)
#define EPI_PORTG_PINS (GPIO_PIN_1 | GPIO_PIN_0)
#define EPI_PORTH_PINS (GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0)
#define EPI_PORTK_PINS (GPIO_PIN_5)
#define EPI_PORTL_PINS                                                         \
  (GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0)
#define EPI_PORTM_PINS (GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0)
#define EPI_PORTP_PINS (GPIO_PIN_3 | GPIO_PIN_2)

#define SDRAM_APP_START_ADDRESS 0x60000000
#define SDRAM_APP_END_ADDRESS 0x61FFFFFF

// static const char TASK_NAME[] = "SDRAM_HAL";


int HAL_SDRAM_ConfigureEPI(void) {
  //
  // The EPI0 peripheral must be enabled for use.
  //
  SysCtlPeripheralEnable(SYSCTL_PERIPH_EPI0);

  //
  // For this example EPI0 is used with multiple pins on PortA, B, C, G, H,
  // K, L, M and N.  The actual port and pins used may be different on your
  // part, consult the data sheet for more information.
  // TODO: Update based upon the EPI pin assignment on your target part.
  //
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);

  //
  // This step configures the internal pin muxes to set the EPI pins for use
  // with EPI.  Please refer to the datasheet for more information about pin
  // muxing.  Note that EPI0S27:20 are not used for the EPI SDRAM
  // implementation.
  //
  GPIOPinConfigure(GPIO_PH0_EPI0S0);
  GPIOPinConfigure(GPIO_PH1_EPI0S1);
  GPIOPinConfigure(GPIO_PH2_EPI0S2);
  GPIOPinConfigure(GPIO_PH3_EPI0S3);
  GPIOPinConfigure(GPIO_PC7_EPI0S4);
  GPIOPinConfigure(GPIO_PC6_EPI0S5);
  GPIOPinConfigure(GPIO_PC5_EPI0S6);
  GPIOPinConfigure(GPIO_PC4_EPI0S7);
  GPIOPinConfigure(GPIO_PA6_EPI0S8);
  GPIOPinConfigure(GPIO_PA7_EPI0S9);
  GPIOPinConfigure(GPIO_PG1_EPI0S10);
  GPIOPinConfigure(GPIO_PG0_EPI0S11);
  GPIOPinConfigure(GPIO_PM3_EPI0S12);
  GPIOPinConfigure(GPIO_PM2_EPI0S13);
  GPIOPinConfigure(GPIO_PM1_EPI0S14);
  GPIOPinConfigure(GPIO_PM0_EPI0S15);
  GPIOPinConfigure(GPIO_PL0_EPI0S16);
  GPIOPinConfigure(GPIO_PL1_EPI0S17);
  GPIOPinConfigure(GPIO_PL2_EPI0S18);
  GPIOPinConfigure(GPIO_PL3_EPI0S19);
  GPIOPinConfigure(GPIO_PB3_EPI0S28);
  GPIOPinConfigure(GPIO_PP2_EPI0S29);
  GPIOPinConfigure(GPIO_PP3_EPI0S30);
  GPIOPinConfigure(GPIO_PK5_EPI0S31);

  //
  // Configure the GPIO pins for EPI mode.  All the EPI pins require 8mA
  // drive strength in push-pull operation.  This step also gives control of
  // pins to the EPI module.
  //
  GPIOPinTypeEPI(GPIO_PORTA_BASE, EPI_PORTA_PINS);
  GPIOPinTypeEPI(GPIO_PORTB_BASE, EPI_PORTB_PINS);
  GPIOPinTypeEPI(GPIO_PORTC_BASE, EPI_PORTC_PINS);
  GPIOPinTypeEPI(GPIO_PORTG_BASE, EPI_PORTG_PINS);
  GPIOPinTypeEPI(GPIO_PORTH_BASE, EPI_PORTH_PINS);
  GPIOPinTypeEPI(GPIO_PORTK_BASE, EPI_PORTK_PINS);
  GPIOPinTypeEPI(GPIO_PORTL_BASE, EPI_PORTL_PINS);
  GPIOPinTypeEPI(GPIO_PORTM_BASE, EPI_PORTM_PINS);
  GPIOPinTypeEPI(GPIO_PORTP_BASE, EPI_PORTP_PINS);

  uint32_t ui32Strength = GPIO_STRENGTH_8MA;
  uint32_t ui32PinType = GPIO_PIN_TYPE_STD;

  GPIOPadConfigSet(GPIO_PORTA_BASE, EPI_PORTA_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTB_BASE, EPI_PORTB_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTC_BASE, EPI_PORTC_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTG_BASE, EPI_PORTG_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTH_BASE, EPI_PORTH_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTK_BASE, EPI_PORTK_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTL_BASE, EPI_PORTL_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTM_BASE, EPI_PORTM_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTP_BASE, EPI_PORTP_PINS, ui32Strength, ui32PinType);

  //
  // Set the EPI clock to half the system clock.
  //
  EPIDividerSet(EPI0_BASE, 4);

  //
  // Sets the usage mode of the EPI module.  For this example we will use
  // the SDRAM mode to talk to the external 64MB SDRAM daughter card.
  //
  EPIModeSet(EPI0_BASE, EPI_MODE_SDRAM);

  //
  // Configure the SDRAM mode.  We configure the SDRAM according to our core
  // clock frequency.  We will use the normal (or full power) operating
  // state which means we will not use the low power self-refresh state.
  // Set the SDRAM size to 64MB with a refresh interval of 468 clock ticks.
  //
  // DO NOT CHANGE UI32REFRESH VALUE!!!!!!!!!!!!!!!!!!!
  EPIConfigSDRAMSet(EPI0_BASE,
                    (EPI_SDRAM_CORE_FREQ_15_30 | EPI_SDRAM_FULL_POWER |
                     EPI_SDRAM_SIZE_512MBIT),
                    234);

  //
  // Set the address map.  The EPI0 is mapped from 0x60000000 to 0x01FFFFFF.
  // For this example, we will start from a base address of 0x60000000 with
  // a size of 256MB.  Although our SDRAM is only 64MB, there is no 64MB
  // aperture option so we pick the next larger size.
  //
  EPIAddressMapSet(EPI0_BASE, EPI_ADDR_RAM_SIZE_256MB | EPI_ADDR_RAM_BASE_6);

  //
  // Wait for the SDRAM wake-up to complete by polling the SDRAM
  // initialization sequence bit.  This bit is true when the SDRAM interface
  // is going through the initialization and false when the SDRAM interface
  // it is not in a wake-up period.
  //
  while (HWREG(EPI0_BASE + EPI_O_STAT) & EPI_STAT_INITSEQ) {
  }

  //
  // Write to the first 2 and last 2 address of the SDRAM card.  Since the
  // SDRAM card is word addressable, we will write words.
  //
  HWREGH(SDRAM_APP_START_ADDRESS) = 0xabcd;
  HWREGH(SDRAM_APP_START_ADDRESS + 0x2) = 0x1234;
  HWREGH(SDRAM_APP_END_ADDRESS - 0x3) = 0xdcba;
  HWREGH(SDRAM_APP_END_ADDRESS - 0x1) = 0x4321;

  if ((HWREGH(SDRAM_APP_START_ADDRESS) == 0xabcd) &&
      (HWREGH(SDRAM_APP_START_ADDRESS + 0x2) == 0x1234) &&
      (HWREGH(SDRAM_APP_END_ADDRESS - 0x3) == 0xdcba) &&
      (HWREGH(SDRAM_APP_END_ADDRESS - 0x1) == 0x4321)) {
    //
    // Read and write operations were successful.  Return with no errors.
    //
    return (1);
  } else {
    //
    // Read and write operations were failure.  Return with error.
    //
    return (0);
  }
}


