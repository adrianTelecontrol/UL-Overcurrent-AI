/**
 * @file    eve_video.h
 * @brief   AVI video playback via FT812 Media FIFO.
 *
 * Strategy overview
 * -----------------
 * The FT812 CMD_PLAYVIDEO command accepts compressed video data fed through
 * the Media FIFO (REG_MEDIAFIFO_READ / REG_MEDIAFIFO_WRITE).  We carve out a
 * ring buffer in the upper part of RAM_G, then pump SD-card data into it
 * while the co-processor decodes frames autonomously.
 *
 * RAM_G layout assumed by this driver
 * ------------------------------------
 *   0x00000000 … EVE_VIDEO_MEDIAFIFO_BASE-1  →  free for other bitmaps
 *   EVE_VIDEO_MEDIAFIFO_BASE                 →  Media FIFO ring buffer
 *                                               (EVE_VIDEO_MEDIAFIFO_SIZE bytes)
 *
 * The FT812 RAM_G is 1 MB (0x000000 – 0x0FFFFF).
 * We place the 256 kB FIFO at the very top so it does not collide with
 * bitmaps loaded elsewhere.
 *
 * Video file requirements
 * -----------------------
 *   • Format  : MJPEG-in-AVI  (the only format the FT812 co-processor can
 *               decode autonomously via CMD_PLAYVIDEO / OPT_MEDIAFIFO)
 *   • Pixel   : up to 800×480 (display resolution)
 *   • Audio   : optional; if present the FT812 will play it on its DAC
 *               (set OPT_SOUND to enable, omit to mute)
 *   • File    : stored on the FAT SD card, path passed at call time
 *
 * Typical call
 * ------------
 * @code
 *   EVE_Video_Play("0:/video.avi",
 *                  OPT_FULLSCREEN | OPT_NOTEAR | OPT_MEDIAFIFO);
 * @endcode
 *
 * @note  Requires FT_81X_ENABLE to be defined (FT812 / FT813 co-processor).
 */

#ifndef EVE_VIDEO_H
#define EVE_VIDEO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * Tuneable constants – adjust to taste
 * --------------------------------------------------------------------- */

/**
 * @brief Base address of the Media FIFO ring buffer inside RAM_G.
 *
 * Must be 4-byte aligned.  Leave the bottom 768 kB free for bitmaps;
 * place the 256 kB FIFO in the top quarter of the 1 MB RAM_G.
 */
#define EVE_VIDEO_MEDIAFIFO_BASE    (768UL * 1024UL)   /* 0x000C0000 */

/**
 * @brief Size of the Media FIFO ring buffer (bytes).
 *
 * Must be a power of two and at least 64 kB.  256 kB gives comfortable
 * margin for SD-card burst latency at typical MJPEG bitrates.
 */
#define EVE_VIDEO_MEDIAFIFO_SIZE    (256UL * 1024UL)   /* 0x00040000 */

/**
 * @brief Bytes read from the SD card per loop iteration.
 *
 * Keeping this at 4 kB matches the FAT cluster read unit and keeps the
 * stack pressure low (the buffer lives in static storage, not the stack).
 */
#define EVE_VIDEO_SD_CHUNK_BYTES    (4096U)

/* -----------------------------------------------------------------------
 * Return codes
 * --------------------------------------------------------------------- */

typedef enum {
    EVE_VIDEO_OK            =  0,   /**< Playback finished normally.        */
    EVE_VIDEO_ERR_OPEN      = -1,   /**< Could not open file on SD card.    */
    EVE_VIDEO_ERR_COPRO     = -2,   /**< Co-processor entered fault state.  */
    EVE_VIDEO_ERR_PARAM     = -3,   /**< Invalid parameter (NULL, etc.).    */
    EVE_VIDEO_ERR_SPI       = -4,   /**< SPI link to FT812 is broken.       */
} EVE_VideoResult_t;

/* -----------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------- */

/**
 * @brief  Play an MJPEG-AVI file from the SD card on the FT812 display.
 *
 * This function blocks until the video finishes or an error occurs.
 * It continuously reads the SD card and feeds data into the Media FIFO
 * while the FT812 co-processor decodes and displays frames.
 *
 * @param  pcFileName   FAT path to the AVI file, e.g. "0:/video.avi".
 * @param  ui32Options  Bit-OR of OPT_* flags:
 *                        OPT_MEDIAFIFO  – required; use Media FIFO input
 *                        OPT_FULLSCREEN – scale to display size
 *                        OPT_NOTEAR     – wait for VSYNC before each swap
 *                        OPT_SOUND      – enable audio track
 *
 * @return EVE_VideoResult_t status code.
 */
EVE_VideoResult_t EVE_Video_Play(const uint8_t drive, const char *pcFileName, uint32_t ui32Options);

/**
 * @brief  Convert an EVE_VideoResult_t code to a human-readable string.
 *
 * @param  eResult   Return value from EVE_Video_Play().
 * @return Pointer to a static string; do not free.
 */
const char *EVE_Video_ResultStr(EVE_VideoResult_t eResult);

#ifdef __cplusplus
}
#endif

#endif /* EVE_VIDEO_H */




