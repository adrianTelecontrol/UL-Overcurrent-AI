/**
 * @file    eve_video.c
 * @brief   MJPEG-AVI video playback via FT812 Media FIFO.
 *
 * How the Media FIFO works (FT812 programmer's guide §4.6)
 * ---------------------------------------------------------
 * 1. The host writes CMD_MEDIAFIFO(addr, size) to define the ring buffer
 *    inside RAM_G.
 * 2. The host writes CMD_PLAYVIDEO(options) immediately after.
 * 3. The co-processor starts decoding and HALTS the command FIFO (it will
 *    not read further commands) until the video ends.
 * 4. The host feeds compressed data by:
 *      a. Reading REG_MEDIAFIFO_READ  → the co-processor's read pointer.
 *      b. Reading REG_MEDIAFIFO_WRITE → the host's own write pointer.
 *      c. Computing free space in the ring buffer.
 *      d. Writing up to (free_space) bytes to RAM_G at the current write
 *         pointer (wrapping at the ring-buffer boundary).
 *      e. Advancing REG_MEDIAFIFO_WRITE by the number of bytes just written.
 * 5. When the file is exhausted the host must write the *exact* remaining
 *    bytes and NOT advance REG_MEDIAFIFO_WRITE further; the co-processor
 *    detects end-of-stream when it catches up to the write pointer after the
 *    final bytes have been consumed.
 * 6. After the video ends, the co-processor resumes processing the command
 *    FIFO normally (CMD_SWAP, etc.).
 *
 * Implementation notes
 * --------------------
 * • We use a static 4 kB staging buffer (EVE_VIDEO_SD_CHUNK_BYTES) so that
 *   each FatFS f_read() call fetches a full cluster at a time.
 * • Writing to RAM_G uses EVE_AddrForWr() / EVE_Write8() with explicit CS
 *   toggling, matching the pattern used throughout EVE.c.
 * • We poll REG_MEDIAFIFO_READ to throttle writes; no RTOS required.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* FatFS */
#include <fatfs/src/ff.h>

/* EVE driver */
#include "FT8xx.h"
#include "gpu_ft81x.h"
#include "video_engine.h"
#include "helpers.h"
#include "file_manager.h"

/* TivaWare */
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"

/* Project logging */
#include "tiva_log.h"

/* HAL SPI helpers referenced in EVE.c */
extern void HAL_TFT_SPI_CS_Enable(void);
extern void HAL_TFT_SPI_CS_Disable(void);

/* -----------------------------------------------------------------------
 * Private helpers
 * --------------------------------------------------------------------- */

static const char TAG[] = "EVE_VIDEO";

/** Static staging buffer – avoids stack pressure inside the pump loop. */
static uint8_t s_sdBuf[EVE_VIDEO_SD_CHUNK_BYTES];

/* -----------------------------------------------------------------------
 * Internal: write a contiguous block of bytes into the Media FIFO ring
 * buffer that lives inside RAM_G.
 *
 * The ring buffer wraps; this function handles the wrap transparently by
 * splitting the write into at most two SPI bursts.
 *
 * @param ui32FifoBase   Base address of the ring buffer inside RAM_G.
 * @param ui32FifoSize   Size of the ring buffer in bytes (power of two).
 * @param ui32WritePtr   Current write offset (0 … ui32FifoSize-1).
 * @param pui8Data       Source data.
 * @param ui32Bytes      Number of bytes to write (caller guarantees fit).
 * @return New write offset after the write.
 * --------------------------------------------------------------------- */
static uint32_t prv_WriteToMediaFifo(uint32_t ui32FifoBase,
                                     uint32_t ui32FifoSize,
                                     uint32_t ui32WritePtr,
                                     const uint8_t *pui8Data,
                                     uint32_t ui32Bytes)
{
    uint32_t ui32SpaceToEnd = ui32FifoSize - ui32WritePtr;

    if (ui32Bytes <= ui32SpaceToEnd) {
        /* Single burst – no wrap */
        HAL_TFT_SPI_CS_Enable();
        EVE_AddrForWr(ui32FifoBase + ui32WritePtr);
		uint32_t i = 0;
        for (; i < ui32Bytes; i++) {
            EVE_Write8(pui8Data[i]);
        }
        HAL_TFT_SPI_CS_Disable();

        ui32WritePtr += ui32Bytes;
        if (ui32WritePtr >= ui32FifoSize) {
            ui32WritePtr = 0;
        }
    } else {
        /* Two bursts: tail of ring then head of ring */
        uint32_t ui32First  = ui32SpaceToEnd;
        uint32_t ui32Second = ui32Bytes - ui32First;

        /* First burst: to the end of the ring buffer */
        HAL_TFT_SPI_CS_Enable();
        EVE_AddrForWr(ui32FifoBase + ui32WritePtr);
		uint32_t i = 0;
        for (; i < ui32First; i++) {
            EVE_Write8(pui8Data[i]);
        }
        HAL_TFT_SPI_CS_Disable();

        /* Second burst: from the beginning of the ring buffer */
        HAL_TFT_SPI_CS_Enable();
        EVE_AddrForWr(ui32FifoBase);
        for (i = 0; i < ui32Second; i++) {
            EVE_Write8(pui8Data[ui32First + i]);
        }
        HAL_TFT_SPI_CS_Disable();

        ui32WritePtr = ui32Second;
    }

    return ui32WritePtr;
}

/* -----------------------------------------------------------------------
 * Internal: calculate free bytes in the ring buffer.
 *
 * Both pointers are ring-buffer offsets (0 … size-1), NOT absolute RAM_G
 * addresses.
 * --------------------------------------------------------------------- */
static inline uint32_t prv_FifoFree(uint32_t ui32FifoSize,
                                    uint32_t ui32Wr,
                                    uint32_t ui32Rd)
{
    /*
     * Standard ring-buffer free-space formula.
     * We keep one slot empty so that full and empty are distinguishable,
     * hence "- 4" (minimum 4-byte alignment unit).
     */
    if (ui32Wr >= ui32Rd) {
        return (ui32FifoSize - (ui32Wr - ui32Rd)) - 4U;
    } else {
        return (ui32Rd - ui32Wr) - 4U;
    }
}

/* -----------------------------------------------------------------------
 * Public: EVE_Video_Play
 * --------------------------------------------------------------------- */
EVE_VideoResult_t EVE_Video_Play(const uint8_t drive, const char *pcFileName, uint32_t ui32Options)
{
    if (pcFileName == NULL ) {
        return EVE_VIDEO_ERR_PARAM;
    }

    /* ------------------------------------------------------------------
     * 0. Sanity-check the SPI link before touching the co-processor.
     * ---------------------------------------------------------------- */
    if (EVE_MemRead8(REG_ID) != 0x7C) {
        TIVA_LOGE(TAG, "SPI link to FT812 is broken (REG_ID != 0x7C).");
        return EVE_VIDEO_ERR_SPI;
    }

    /* ------------------------------------------------------------------
     * 1. Open the AVI file on the SD card.
     * ---------------------------------------------------------------- */

   const char *driveStr = FM_getDriveString(drive);
	if(driveStr == NULL) {
		TIVA_LOGE(TAG, "Drive specified was not found!");
		return EVE_VIDEO_ERR_PARAM;
	}
	char tempFilePath[20];
	
	snprintf(tempFilePath, sizeof(tempFilePath), "%s/%s", driveStr, pcFileName);
    TIVA_LOGI(TAG, "Video to fetch: %s", tempFilePath);
	 
    FIL   hFile;
    FRESULT iFRes = f_open(&hFile, tempFilePath, FA_READ);
    if (iFRes != FR_OK) {
        TIVA_LOGE(TAG, "f_open('%s') failed: %d", pcFileName, (int)iFRes);
        return EVE_VIDEO_ERR_OPEN;
    }
    TIVA_LOGI(TAG, "Opened '%s' (%lu bytes)", pcFileName,
              (unsigned long)f_size(&hFile));

    /* ------------------------------------------------------------------
     * 2. Wait for any pending co-processor commands to finish.
     * ---------------------------------------------------------------- */
    API_LIB_AwaitCoProEmpty();

    /* ------------------------------------------------------------------
     * 3. Issue CMD_MEDIAFIFO to declare the ring buffer, then
     *    CMD_PLAYVIDEO to start decoding.
     *
     *    Both commands must be in the SAME co-processor burst so the
     *    engine sees them atomically before it begins consuming media data.
     * ---------------------------------------------------------------- */

    /* Force OPT_MEDIAFIFO – it is mandatory for the streaming path. */
    ui32Options |= OPT_MEDIAFIFO;

    API_LIB_BeginCoProList();

    API_CMD_MEDIAFIFO(EVE_VIDEO_MEDIAFIFO_BASE, EVE_VIDEO_MEDIAFIFO_SIZE);
    API_CMD_PLAYVIDEO(ui32Options);

    API_LIB_EndCoProList();

    /*
     * Do NOT call API_LIB_AwaitCoProEmpty() here!
     * CMD_PLAYVIDEO causes the co-processor to stall its FIFO until the
     * video finishes.  Polling REG_CMD_READ == REG_CMD_WRITE would loop
     * forever because the co-processor is deliberately not advancing
     * REG_CMD_READ while it is decoding.
     *
     * Instead we detect end-of-video by file exhaustion + the co-processor
     * draining the last bytes from the Media FIFO.
     */

    /* ------------------------------------------------------------------
     * 4. Initialise the Media FIFO write pointer.
     *
     *    REG_MEDIAFIFO_WRITE holds the current host write offset.
     *    Reset it to 0 to start fresh.
     * ---------------------------------------------------------------- */
    EVE_MemWrite32(REG_MEDIAFIFO_WRITE, 0);
    uint32_t ui32WritePtr = 0;   /* local mirror of REG_MEDIAFIFO_WRITE */

    /* ------------------------------------------------------------------
     * 5. Pump loop: read SD → write to Media FIFO ring buffer.
     * ---------------------------------------------------------------- */
    bool     bFileDone     = false;
    bool     bError        = false;
    uint32_t ui32BytesRead = 0;

    while (!bFileDone) {

        /* ---- 5a. Read a chunk from the SD card ---- */
        iFRes = f_read(&hFile, s_sdBuf, sizeof(s_sdBuf), &ui32BytesRead);
        if (iFRes != FR_OK) {
            TIVA_LOGE(TAG, "f_read error: %d", (int)iFRes);
            bError    = true;
            bFileDone = true;
            break;
        }
        if (ui32BytesRead == 0) {
            /* EOF reached */
            bFileDone = true;
            break;
        }

        /* ---- 5b. Wait until the ring buffer has room ---- */
        uint32_t ui32Free = 0;

		// For benchmarking purposes

        do {
            uint32_t ui32ReadPtr = EVE_MemRead32(REG_MEDIAFIFO_READ);
            ui32Free = prv_FifoFree(EVE_VIDEO_MEDIAFIFO_SIZE,
                                    ui32WritePtr, ui32ReadPtr);

            /*
             * While we wait, also check for a co-processor fault.
             * REG_CMD_READ == 0xFFF signals a fatal error.
             */
            if (EVE_MemRead16(REG_CMD_READ) == 0xFFF) {
                TIVA_LOGE(TAG, "Co-processor fault during video playback!");
                bError    = true;
                bFileDone = true;
                break;
            }
        } while (ui32Free < ui32BytesRead);


        if (bError) break;

        /* ---- 5c. Write the chunk into the ring buffer ---- */
        ui32WritePtr = prv_WriteToMediaFifo(EVE_VIDEO_MEDIAFIFO_BASE,
                                            EVE_VIDEO_MEDIAFIFO_SIZE,
                                            ui32WritePtr,
                                            s_sdBuf,
                                            ui32BytesRead);

        /* ---- 5d. Advance REG_MEDIAFIFO_WRITE to inform co-processor ---- */
        EVE_MemWrite32(REG_MEDIAFIFO_WRITE, ui32WritePtr);
    }

    /* ------------------------------------------------------------------
     * 6. Wait for the co-processor to drain the last bytes.
     *
     *    After the file is exhausted we keep polling until:
     *      • REG_MEDIAFIFO_READ == REG_MEDIAFIFO_WRITE  (co-pro consumed all)
     *    …which signals the co-processor that there is no more data and it
     *    can finish the current frame and exit CMD_PLAYVIDEO.
     *
     *    Then we wait for the command FIFO to empty normally.
     * ---------------------------------------------------------------- */
    if (!bError) {
        TIVA_LOGI(TAG, "File fully fed, waiting for CMD_PLAYVIDEO to exit…");

        if (EVE_WaitCmdFifoEmpty() == 0xFF) {
            TIVA_LOGE(TAG, "Command FIFO fault after video end.");
            bError = true;
        }
    }

    /* ------------------------------------------------------------------
     * 7. Issue CMD_SWAP so the last decoded frame stays visible, then
     *    restore a clean display list.
     * ---------------------------------------------------------------- */
    if (!bError) {
        API_LIB_BeginCoProList();
        API_CMD_DLSTART();
        API_CLEAR_COLOR_RGB(0, 0, 0);
        API_CLEAR(1, 1, 1);
        /* The last video frame is now in RAM_G decoded as a bitmap.
         * A simple swap here just clears the screen; callers can build
         * their own post-video display list if needed. */
        API_DISPLAY();
        API_CMD_SWAP();
        API_LIB_EndCoProList();
        API_LIB_AwaitCoProEmpty();
    }

    /* ------------------------------------------------------------------
     * 8. Cleanup.
     * ---------------------------------------------------------------- */
    f_close(&hFile);
    TIVA_LOGI(TAG, "Video playback %s.", bError ? "FAILED" : "completed OK");

    return bError ? EVE_VIDEO_ERR_COPRO : EVE_VIDEO_OK;
}

/* -----------------------------------------------------------------------
 * Public: EVE_Video_ResultStr
 * --------------------------------------------------------------------- */
const char *EVE_Video_ResultStr(EVE_VideoResult_t eResult)
{
    switch (eResult) {
        case EVE_VIDEO_OK:          return "OK";
        case EVE_VIDEO_ERR_OPEN:    return "ERR_OPEN (SD file not found)";
        case EVE_VIDEO_ERR_COPRO:   return "ERR_COPRO (FT812 co-proc fault)";
        case EVE_VIDEO_ERR_PARAM:   return "ERR_PARAM (NULL argument)";
        case EVE_VIDEO_ERR_SPI:     return "ERR_SPI (FT812 not responding)";
        default:                    return "ERR_UNKNOWN";
    }
}


