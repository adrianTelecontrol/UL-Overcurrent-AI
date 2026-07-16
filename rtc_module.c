
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <inc/hw_memmap.h>
#include <time.h>
#include <driverlib/sysctl.h>
#include <driverlib/hibernate.h>

#include "event_engine.h"
#include "helpers.h"
#include "rtc_module.h"

static char TAG[] = "RTC_module";
static char DATE_STR[] = "22/06/2026";
static char TIME_STR[] = "09:35:45";

bool RTC_parseDate(const char *data_str, RTC_timeDate *timeDate) {
    if(timeDate == NULL || data_str == NULL) return false;

    // Make a local copy in RAM because strtok modifies the string
    char buffer[16];
    strncpy(buffer, data_str, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    // Parse Day
    char *token = strtok(buffer, "/");
    if (token == NULL) return false;
    int mday = atoi(token);

    // Parse Month (Remember to pass NULL to continue parsing the same string)
    token = strtok(NULL, "/");
    if (token == NULL) return false;
    int month = atoi(token) - 1; // struct tm months are 0-11

    // Parse Year
    token = strtok(NULL, "/");
    if (token == NULL) return false;
    int year = atoi(token) - 1900; // struct tm years are years since 1900

    // Validation
    if(mday > 31 || mday < 1) return false;
    if(month > 11 || month < 0) return false;
    if(year < 0) return false;
    
    timeDate->tm_mday = mday;
    timeDate->tm_mon = month;
    timeDate->tm_year = year;

    return true;
}

bool RTC_parseTime(const char *data_str, RTC_timeDate *timeDate) {
    if(timeDate == NULL || data_str == NULL) return false;

    // Make a local copy in RAM
    char buffer[16];
    strncpy(buffer, data_str, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    // Parse Hours
    char *token = strtok(buffer, ":");
    if (token == NULL) return false;
    int hours = atoi(token);

    // Parse Minutes (Pass NULL)
    token = strtok(NULL, ":");
    if (token == NULL) return false;
    int min = atoi(token);

    // Parse Seconds (Pass NULL)
    token = strtok(NULL, ":");
    if (token == NULL) return false;
    int sec = atoi(token);

    // Validation
    if(hours > 23 || hours < 0) return false;
    if(min > 59 || min < 0) return false;
    if(sec > 59 || sec < 0) return false;

    timeDate->tm_hour = hours;
    timeDate->tm_min = min; 
    timeDate->tm_sec = sec;

    return true;
}

bool RTC_setTimeDate(const char *date_str, const char *time_str) {
    
    // Initialize to zero so tm_wday, tm_yday, etc., don't have garbage data
    RTC_timeDate timeDate = {0};

    if(!RTC_parseDate(date_str, &timeDate) || !RTC_parseTime(time_str, &timeDate)) {
        // TIVA_LOGE(TAG, "Wrong format of strings!");
        return false;
    }

    HibernateCalendarSet(&timeDate);
    SysCtlDelay(MS_2_CLK(1));

    // Check whether the changes were applied
    RTC_timeDate check;
    HibernateCalendarGet(&check); 
    
    // Fixed typo in the validation comparison
    if( check.tm_hour != timeDate.tm_hour || check.tm_min != timeDate.tm_min ||
        check.tm_year != timeDate.tm_year || check.tm_mon != timeDate.tm_mon ||
        check.tm_mday != timeDate.tm_mday)
    {
        // TIVA_LOGE(TAG, "DateTime changes were not applied!");   
        return false;
    }

    return true;
}

bool RTC_getFormattedDate(char *out_buffer, size_t max_len) {
    if (out_buffer == NULL || max_len == 0) return false;

    RTC_timeDate currentTime;
    
    HibernateCalendarGet(&currentTime);

    // Formatear la cadena.
    // IMPORTANTE: 
    // tm_mon va de 0 a 11, por lo que sumamos 1.
    // tm_year son los años desde 1900, por lo que sumamos 1900.
    // %02d asegura que días y meses menores a 10 tengan un '0' a la izquierda (ej. "04").
    snprintf(out_buffer, max_len, "%02d/%02d/%04d", 
             currentTime.tm_mday, 
             currentTime.tm_mon + 1, 
             currentTime.tm_year + 1900);

    return true;
}

bool RTC_getFileFormattedDate(char *out_buffer, size_t max_len) {
    if (out_buffer == NULL || max_len == 0) return false;

    RTC_timeDate currentTime;
    
    HibernateCalendarGet(&currentTime);

    // Formatear la cadena.
    // IMPORTANTE: 
    // tm_mon va de 0 a 11, por lo que sumamos 1.
    // tm_year son los años desde 1900, por lo que sumamos 1900.
    // %02d asegura que días y meses menores a 10 tengan un '0' a la izquierda (ej. "04").
    snprintf(out_buffer, max_len, "%02d_%02d_%04d", 
             currentTime.tm_mday, 
             currentTime.tm_mon + 1, 
             currentTime.tm_year + 1900);

    return true;
}

bool RTC_getFormattedTime(char *out_buffer, size_t max_len) {
    if (out_buffer == NULL || max_len == 0) return false;

    RTC_timeDate currentTime;
    
    HibernateCalendarGet(&currentTime);

    snprintf(out_buffer, max_len, "%02d:%02d:%02d", 
             currentTime.tm_hour, 
             currentTime.tm_min, 
             currentTime.tm_sec);

    return true;
}

bool RTC_getFileFormattedTime(char *out_buffer, size_t max_len) {
    if (out_buffer == NULL || max_len == 0) return false;

    RTC_timeDate currentTime;
    
    HibernateCalendarGet(&currentTime);

    snprintf(out_buffer, max_len, "%02d_%02d_%02d", 
             currentTime.tm_hour, 
             currentTime.tm_min, 
             currentTime.tm_sec);

    return true;
}

bool RTC_isBatteryOk(void) {
	// Clear any lingering low-battery flags before starting a fresh check
    HibernateIntClear(HIBERNATE_INT_LOW_BAT);

    // 1. Force the hardware to sample the VBAT pin
    HibernateBatCheckStart();

    // 2. Wait for the hardware battery check to complete.
    // HibernateBatCheckDone returns non-zero while busy, and 0 when finished.
	uint32_t counter = 0;
    while (HibernateBatCheckDone() != 0) {
        // In an industrial HMI environment, consider adding a timeout 
        // counter here to prevent infinite blocking if the bus faults.
		SysCtlDelay(MS_2_CLK(1));
		counter++;
		if(counter == 100) return false;
    }

    // 3. Read the raw (unmasked) interrupt status
    uint32_t status = HibernateIntStatus(0);

    // 4. Evaluate the flag
    // If HIBERNATE_INT_LOW_BAT is set, voltage is near 0V (missing) or dead.
    if (status & HIBERNATE_INT_LOW_BAT) {
        return false; // Disconnected or completely dead
    }

    return true; // Connected and healthy
}

// =========================================================================
// AJUSTES DIRECTOS DE FECHA Y HORA (USADOS POR LA UI)
// =========================================================================

bool RTC_getCurrentDateTime(uint8_t *day, uint8_t *month, uint16_t *year, uint8_t *hour, uint8_t *min, uint8_t *sec) {
    if (!day || !month || !year || !hour || !min || !sec) return false;

    RTC_timeDate currentTime;
    HibernateCalendarGet(&currentTime);

    // Deshacemos el formato de 'struct tm' para regresar valores naturales a la UI
    *day   = currentTime.tm_mday;
    *month = currentTime.tm_mon + 1;       // tm_mon va de 0 a 11
    *year  = currentTime.tm_year + 1900;   // tm_year son años desde 1900
    
    *hour  = currentTime.tm_hour;
    *min   = currentTime.tm_min;
    *sec   = currentTime.tm_sec;

    return true;
}

bool RTC_setDate(uint8_t day, uint8_t month, uint16_t year) {
    // Validaciones de seguridad para el hardware
    if(day < 1 || day > 31) return false;
    if(month < 1 || month > 12) return false;
    if(year < 2000) return false;

    RTC_timeDate currentTime;
    
    // 1. Leemos el calendario actual para preservar la hora intacta
    HibernateCalendarGet(&currentTime);

    // 2. Modificamos solo la estructura de la fecha (adaptando a struct tm)
    currentTime.tm_mday = day;
    currentTime.tm_mon  = month - 1;
    currentTime.tm_year = year - 1900;

    // 3. Mandamos los datos al registro de hardware
    HibernateCalendarSet(&currentTime);
    SysCtlDelay(MS_2_CLK(1)); // Damos tiempo a que se propague en el dominio de reloj bajo

	RTC_getFormattedDate(DATE_STR, sizeof(DATE_STR));
	Event_Post(EVT_SYS_DATE_CHANGED, (EventParam_t){.str = DATE_STR});

    return true;
}

bool RTC_setTime(uint8_t hour, uint8_t min, uint8_t sec) {
    // Validaciones de seguridad
    if(hour > 23) return false;
    if(min > 59) return false;
    if(sec > 59) return false;

    RTC_timeDate currentTime;
    
    // 1. Leemos el calendario actual para preservar la fecha intacta
    HibernateCalendarGet(&currentTime);

    // 2. Modificamos solo el tiempo
    currentTime.tm_hour = hour;
    currentTime.tm_min  = min;
    currentTime.tm_sec  = sec;

    // 3. Mandamos los datos al registro de hardware
    HibernateCalendarSet(&currentTime);
    SysCtlDelay(MS_2_CLK(1)); 

    return true;
}

bool RTC_initModule(void) {
    // 1. MUST enable peripheral clock BEFORE touching any registers!
    SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);
    
    // Wait for the peripheral to be ready
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_HIBERNATE));

    // 2. Now it is safe to clear interrupts
    uint32_t hibStatus = HibernateIntStatus(false);
    HibernateIntClear(hibStatus);
    
    HibernateEnableExpClk(SysCtlClockGet());
    HibernateLowBatSet(HIBERNATE_LOW_BAT_DETECT | HIBERNATE_LOW_BAT_2_1V);
    
    // Configure the clock (Note: Tiva typically needs the 32kHz oscillator driven)
    HibernateClockConfig(HIBERNATE_OSC_LOWDRIVE);
    HibernateCounterMode(HIBERNATE_COUNTER_24HR);

    HibernateBatCheckStart();
    // Wait UNTIL it is done (! instead of nothing)
    while(HibernateBatCheckDone());
    
    // HibernateRTCSet(0); // Optional: Resets the counter. Usually, you only do this on first boot.
    HibernateWakeSet(HIBERNATE_WAKE_PIN);
    HibernateIntEnable(HIBERNATE_INT_RTC_MATCH_0 | HIBERNATE_INT_PIN_WAKE | HIBERNATE_INT_LOW_BAT | HIBERNATE_INT_VDDFAIL);
    
    HibernateRTCEnable();

    // DEBUG
    if(!RTC_setTimeDate(DATE_STR, TIME_STR)) {
        // Handle failure
		return false;
    }

	Event_Post(EVT_SYS_DATE_CHANGED, (EventParam_t){.str = DATE_STR});

	return true;
}
