#ifndef _HELPERS_H
#define _HELPERS_H

/**
 * Helper functions
 */

#include <stdbool.h>
#include <stdint.h>

#include "tiva_log.h"

extern const uint32_t g_ui32SysClock;
extern volatile uint32_t g_ui32MsTicks;	
/*
 * MS_2_CLK
 */
#define MS_2_CLK(ms)                                                           \
  ((uint32_t)(((g_ui32SysClock) / (3.0f)) / ((1.0f) / ((ms) / (1000.0f)))))

/* DWT (Data Watchpoint and Trace) registers, only exists on ARM Cortex with a
 * DWT unit */
#define DWT_CONTROL (*((volatile uint32_t *)0xE0001000))
/*!< DWT Control register */
#define DWT_CYCCNTENA_BIT (1UL << 0)
/*!< CYCCNTENA bit in DWT_CONTROL register */
#define DWT_CYCCNT (*((volatile uint32_t *)0xE0001004))
/*!< DWT Cycle Counter register */
#define DEMCR (*((volatile uint32_t *)0xE000EDFC))
/*!< DEMCR: Debug Exception and Monitor Control Register */
#define TRCENA_BIT (1UL << 24)
/*!< Trace enable bit in DEMCR register */

#define DWTInitCycleCounter() DEMCR |= TRCENA_BIT
/*!< TRCENA: Enable trace and debug block DEMCR (Debug Exception and Monitor
 * Control Register */

#define DWTResetCycleCounter() DWT_CYCCNT = 0
/*!< Reset cycle counter */

#define DWTEnableCycleCounter() DWT_CONTROL |= DWT_CYCCNTENA_BIT
/*!< Enable cycle counter */

#define DWTDisableCycleCounter() DWT_CONTROL &= ~DWT_CYCCNTENA_BIT
/*!< Disable cycle counter */

#define DWTGetCycleCounter() DWT_CYCCNT
/*!< Read cycle counter register */


void Helper_FloatToString(char *buffer, uint32_t whole, uint32_t frac,
                          bool bAddEnd);

// inline uint32_t GetExecTimeMs(void)
// {
//     const uint32_t CLOCK_PERIOD = g_ui32SysClock / 1E6;                        
//     /* 5. Calculate Microseconds (Integer Math for UARTprintf) */              
//     /* Assuming 120 MHz Clock: 120 cycles = 1 us */                            
//     return ( DWTGetCycleCounter() / CLOCK_PERIOD ) / 1000;                               
//     // uint32_t us_frac =                                                         
//     //     (duration % CLOCK_PERIOD) * 100 / CLOCK_PERIOD; /* 2 decimal places */ 
//     // g_ui32ExecMs = us_whole / 1000;                                       
// 	
// }

inline uint32_t GetExecTimeMs(void)
{
	return g_ui32MsTicks;
}

inline uint32_t GetExecTimeUs(void)
{
    const uint32_t CLOCK_PERIOD = g_ui32SysClock / 1E6;                        
    /* 5. Calculate Microseconds (Integer Math for UARTprintf) */              
    /* Assuming 120 MHz Clock: 120 cycles = 1 us */                            
    return ( DWTGetCycleCounter() / CLOCK_PERIOD );                               
    // uint32_t us_frac =                                                         
    //     (duration % CLOCK_PERIOD) * 100 / CLOCK_PERIOD; /* 2 decimal places */ 
    // g_ui32ExecMs = us_whole / 1000;                                       
	
}

inline void StartCycleCounter(void) {
    DWTInitCycleCounter();
    DWTResetCycleCounter();
    DWTEnableCycleCounter();
}


uint32_t g_ui32ExecMs;
#define MEASURE_EXECUTION(code_to_measure)                                     \
  do {                                                                         \
    DWTInitCycleCounter();                                                     \
    DWTResetCycleCounter();                                                    \
    DWTEnableCycleCounter();                                                   \
    /* 2. Execute Code */                                                      \
    code_to_measure;                                                           \
                                                                               \
    /* 4. Calculate Duration */                                                \
    uint32_t duration = DWTGetCycleCounter();                                  \
    DWTDisableCycleCounter();                                                  \
    const uint32_t CLOCK_PERIOD = g_ui32SysClock / 1E6;                        \
    /* 5. Calculate Microseconds (Integer Math for UARTprintf) */              \
    /* Assuming 120 MHz Clock: 120 cycles = 1 us */                            \
    uint32_t us_whole = duration / CLOCK_PERIOD;                               \
    uint32_t us_frac =                                                         \
        (duration % CLOCK_PERIOD) * 100 / CLOCK_PERIOD; /* 2 decimal places */ \
    g_ui32ExecMs = us_whole / 1000;                                       	   \
    /* 6. Log result using Integer formatting (%d.%02d) */                     \
    UARTprintf("I [PERF] '%s' took: %u cycles (%u.%02u us), (%u ms)\n",        \
               #code_to_measure, duration, us_whole, us_frac, g_ui32ExecDurMs);       \
                                                                               \
  } while (0)																	\



#endif // _HELPERS_H
