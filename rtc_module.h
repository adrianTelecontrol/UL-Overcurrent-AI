#ifndef RTC_MODULE_H_
#define RTC_MODULE_H_


typedef struct tm RTC_timeDate;

bool RTC_getFileFormattedDate(char *out_buffer, size_t max_len);
bool RTC_getFormattedDate(char *out_buffer, size_t max_len);

bool RTC_getFormattedTime(char *out_buffer, size_t max_len);
bool RTC_getFileFormattedTime(char *out_buffer, size_t max_len);

// Funciones para ajuste numérico directo desde la Interfaz Gráfica
bool RTC_getCurrentDateTime(uint8_t *day, uint8_t *month, uint16_t *year, uint8_t *hour, uint8_t *min, uint8_t *sec);
bool RTC_setDate(uint8_t day, uint8_t month, uint16_t year);
bool RTC_setTime(uint8_t hour, uint8_t min, uint8_t sec);

bool RTC_isBatteryOk(void);

bool RTC_initModule(void);

#endif // RTC_MODULE_H_


