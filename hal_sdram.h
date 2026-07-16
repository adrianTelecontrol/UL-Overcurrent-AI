#ifndef _SDRAM_HAL_H_
#define _SDRAM_HAL_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
  HAL_SDRAM_STATUS_NOT_TESTED = 0,
  HAL_SDRAM_STATUS_OK,
  HAL_SDRAM_STATUS_PATTERN_FAILED,
} HAL_SDRAM_Status_e;

extern volatile uint32_t g_ui32SDRAMFailAddress;
extern volatile uint16_t g_ui16SDRAMExpected;
extern volatile uint16_t g_ui16SDRAMObserved;

int HAL_SDRAM_ConfigureEPI(void);
bool HAL_SDRAM_RunSelfTest(void);
HAL_SDRAM_Status_e HAL_SDRAM_GetStatus(void);

#endif // _SDRAM_HAL_H_


