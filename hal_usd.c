/**
 *
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <driverlib/sysctl.h>
#include <driverlib/systick.h>

#include "inc/hw_memmap.h"

#include <fatfs/src/diskio.h>
#include <fatfs/src/ff.h>

#include "bitmap_parser.h"
#include "helpers.h"
#include "font_engine.h"
#include "gui_core.h"
#include "file_manager.h"

#include "hal_usd.h"

static const char TASK_NAME[] = "SDSPI_TASK";

static FATFS g_sFatFs;

extern void SD_disk_timerproc (void);
volatile uint32_t g_ui32MsTicks = 0;

void SysTickHandler(void) {
  // Call the FatFs tick timer
  SD_disk_timerproc();
  g_ui32MsTicks++;
}

bool HAL_uSD_init(void) {
  FRESULT iFResult;

  SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);

  iFResult = f_mount(DRIVE_SD_ID, &g_sFatFs);
  if (iFResult != FR_OK) {
    TIVA_LOGI(TASK_NAME, "Error mounting the SD drive: %s",
              FM_StringFromFResult(iFResult));
    return false;
  }

  return true;
}
