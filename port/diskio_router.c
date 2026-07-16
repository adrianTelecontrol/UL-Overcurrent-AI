// diskio_router.c
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <driverlib/hibernate.h>

#include "fatfs/src/diskio.h"
#include "fatfs/src/ff.h"




// Definimos nuestras unidades (Drive Numbers)
#define DRIVE_SD  0
#define DRIVE_USB 1

// Declaramos las funciones que acabamos de renombrar en los otros archivos
extern DSTATUS SD_disk_initialize (BYTE pdrv);
extern DSTATUS SD_disk_status (BYTE pdrv);
extern DRESULT SD_disk_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
extern DRESULT SD_disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
extern DRESULT SD_disk_ioctl (BYTE pdrv, BYTE cmd, void* buff);

extern DSTATUS USB_disk_initialize (BYTE pdrv);
extern DSTATUS USB_disk_status (BYTE pdrv);
extern DRESULT USB_disk_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
extern DRESULT USB_disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
extern DRESULT USB_disk_ioctl (BYTE pdrv, BYTE cmd, void* buff);


// =========================================================================
// IMPLEMENTACIÓN ESTÁNDAR DE FATFS (El Router)
// =========================================================================

DSTATUS disk_status (BYTE pdrv) {
    switch (pdrv) {
        case DRIVE_SD:  return SD_disk_status(pdrv);
        case DRIVE_USB: return USB_disk_status(pdrv);
    }
    return STA_NOINIT;
}

DSTATUS disk_initialize (BYTE pdrv) {
    switch (pdrv) {
        case DRIVE_SD:  return SD_disk_initialize(pdrv);
        case DRIVE_USB: return USB_disk_initialize(pdrv);
    }
    return STA_NOINIT;
}

DRESULT disk_read (BYTE pdrv, BYTE* buff, DWORD sector, BYTE count) {
    switch (pdrv) {
        case DRIVE_SD:  return SD_disk_read(pdrv, buff, sector, count);
        case DRIVE_USB: return USB_disk_read(pdrv, buff, sector, count);
    }
    return RES_PARERR;
}

DRESULT disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, BYTE count) {
    switch (pdrv) {
        case DRIVE_SD:  return SD_disk_write(pdrv, buff, sector, count);
        case DRIVE_USB: return USB_disk_write(pdrv, buff, sector, count);
    }
    return RES_PARERR;
}

DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff) {
    switch (pdrv) {
        case DRIVE_SD:  return SD_disk_ioctl(pdrv, cmd, buff);
        case DRIVE_USB: return USB_disk_ioctl(pdrv, cmd, buff);
    }
    return RES_PARERR;
}


/*---------------------------------------------------------*/
/* User Provided Timer Function for FatFs module           */
/*---------------------------------------------------------*/
DWORD get_fattime (void)
{
    struct tm currentTime;
    
    // Obtenemos la hora real del hardware Hibernate de la Tiva
    HibernateCalendarGet(&currentTime);

    // FatFs espera un formato de 32-bits empaquetado:
    // bit31:25 Year origin from the 1980 (0..127)
    // bit24:21 Month (1..12)
    // bit20:16 Day of the month (1..31)
    // bit15:11 Hour (0..23)
    // bit10:5  Minute (0..59)
    // bit4:0   Second / 2 (0..29)

    // Nota: tm_year en time.h es "años desde 1900".
    // FatFs quiere "años desde 1980".
    // Así que: (tm_year + 1900) - 1980 = tm_year - 80.

    DWORD fattime = 0;
    
    fattime |= ((DWORD)(currentTime.tm_year - 80) << 25);
    fattime |= ((DWORD)(currentTime.tm_mon + 1)   << 21); // tm_mon es 0-11
    fattime |= ((DWORD)currentTime.tm_mday        << 16);
    fattime |= ((DWORD)currentTime.tm_hour        << 11);
    fattime |= ((DWORD)currentTime.tm_min         << 5);
    fattime |= ((DWORD)(currentTime.tm_sec / 2));

    return fattime;
}
