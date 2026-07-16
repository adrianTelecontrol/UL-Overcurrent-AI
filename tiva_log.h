#ifndef TIVA_LOG_H_ 
#define TIVA_LOG_H_

#include <stdint.h>
#include "utils/uartstdio.h" // Ensure TivaWare utils are included

// --------------------------------------------------------------------------
//  ANSI COLOR CODES (For colored terminal output like ESP-IDF)
// --------------------------------------------------------------------------
#define LOG_COLOR_RED     "\033[0;31m"
#define LOG_COLOR_GREEN   "\033[0;32m"
#define LOG_COLOR_YELLOW  "\033[0;33m"
#define LOG_COLOR_RESET   "\033[0m"

// --------------------------------------------------------------------------
//  LOGGING MACROS
// --------------------------------------------------------------------------

// ERROR (Red) - Use for critical failures
#define TIVA_LOGE(tag, format, ...)  \
    //UARTprintf(LOG_COLOR_RED "E [%s] " format LOG_COLOR_RESET "\n", tag, ##__VA_ARGS__)

// WARNING (Yellow) - Use for recoverable issues
#define TIVA_LOGW(tag, format, ...)  \
    //UARTprintf(LOG_COLOR_YELLOW "W [%s] " format LOG_COLOR_RESET "\n", tag, ##__VA_ARGS__)

// INFO (Green) - Use for general state updates
#define TIVA_LOGI(tag, format, ...)  \
    //UARTprintf(LOG_COLOR_GREEN "I [%s] " format LOG_COLOR_RESET "\n", tag, ##__VA_ARGS__)

// DEBUG (Default/White) - Use for detailed variable tracing
#define TIVA_LOGD(tag, format, ...)  \
    // UARTprintf("D [%s] " format "\n", tag, ##__VA_ARGS__)

// --------------------------------------------------------------------------
//  HELPER: HEX DUMP (Like ESP_LOG_BUFFER_HEX)
// --------------------------------------------------------------------------
// Useful for debugging SPI buffers or Image data
static inline void LOG_HEX(const char *tag, const uint8_t *data, uint16_t len)
{
    UARTprintf("D [%s] Dump (%d bytes):\n", tag, len);
    uint16_t i = 0;
    for (i = 0; i < len; i++) {
        UARTprintf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) UARTprintf("\n"); // Newline every 16 bytes
    }
    UARTprintf("\n");
}

#endif /* TIVA_LOG_H_ */


