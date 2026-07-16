/**
 * graphics_engine.c
 *
 * Hybrid Rendering Engine:
 * 1. Scatter-Gather Linked Lists for full-screen (768KB) transitions.
 * 2. Asynchronous Dirty Rectangle Queue for high-speed UI widget updates.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <driverlib/interrupt.h>
#include <driverlib/rom_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/udma.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_ssi.h>
#include <inc/hw_types.h>

#include "gpu_ft81x.h"
#include "FT8xx_params.h"
#include "event_engine.h"
#include "font_engine.h"
#include "forms_manager.h"
#include "gui_core.h"
#include "hal_tft_spi.h"
#include "helpers.h"


#include "render.h"

// --- CONFIGURATION ---
#define GFX_WIDTH 800
#define GFX_HEIGHT 480
#define GFX_PIXELS_PER_FRAME (GFX_WIDTH * GFX_HEIGHT)
#define GFX_BUFFER_SIZE_BYTES (GFX_PIXELS_PER_FRAME * 2) // 768,000 Bytes

#define TRANSFER_SIZE 1024
#define FULL_FRAME_VERIFY_CHUNK_BYTES 256
#define FULL_FRAME_MAX_RETRIES 3

#define GFX_ENABLE_INT
// #define MEASURE_PERF_ENABLE
//  #define MEASURE_PERF_ENABLE_FPS

// 768,000 / 1024 = 750 Total Tasks.
#define TASKS_LIST0 256 // 255 Data + 1 Link
#define TASKS_LIST1 256 // 255 Data + 1 Link
#define TASKS_LIST2 240 // 240 Data (Stops)

// --- GLOBALS ---
pixel16_t *g_pBufferA;
pixel16_t *g_pBufferB;

pixel16_t *g_pDrawingBuffer;
pixel16_t *g_pSendingBuffer;

extern uint8_t g_HAL_uDMA_ControlTable[1024];
#define PRIMARY_CTRL_TABLE ((tDMAControlTable *)g_HAL_uDMA_ControlTable)

// --- FULL SCREEN SCATTER GATHER STRUCTURES ---
#if defined(__ICCARM__)
#pragma data_alignment = 1024
#elif defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN(g_txList0, 1024)
#pragma DATA_ALIGN(g_rxList0, 1024)
#pragma DATA_ALIGN(g_txList1, 1024)
#pragma DATA_ALIGN(g_rxList1, 1024)
#pragma DATA_ALIGN(g_txList2, 1024)
#pragma DATA_ALIGN(g_rxList2, 1024)
#endif
static tDMAControlTable g_txList0[TASKS_LIST0] __attribute__((aligned(1024)));
static tDMAControlTable g_rxList0[TASKS_LIST0] __attribute__((aligned(1024)));
static tDMAControlTable g_txList1[TASKS_LIST1] __attribute__((aligned(1024)));
static tDMAControlTable g_rxList1[TASKS_LIST1] __attribute__((aligned(1024)));
static tDMAControlTable g_txList2[TASKS_LIST2] __attribute__((aligned(1024)));
static tDMAControlTable g_rxList2[TASKS_LIST2] __attribute__((aligned(1024)));

static tDMAControlTable txLink1, txLink2;
static tDMAControlTable rxLink1, rxLink2;
static uint32_t g_ulDummyRx;

uint32_t g_ui32StartTxCycles;
uint32_t g_ui32EndTxCycles;

static bool g_bRequestFullRepaint = false;
static bool g_bPendingFullFrameVerify = false;
static uint8_t g_ui8FullFrameRetryCount = 0;
static uint8_t g_ui8VerifyScratch[FULL_FRAME_VERIFY_CHUNK_BYTES];

bool g_bIsBackgroundReady = false;
volatile uint32_t g_ui32FullFrameVerifyFailures = 0;
volatile uint32_t g_ui32FullFrameRetrySuccesses = 0;
volatile uint32_t g_ui32LastFullFrameVerifyOffset = 0;

volatile DMAJobQueue_t g_DMAQueue = {0};

volatile RenderEngine_t g_RenderEngine = {RENDER_IDLE};

// static const char *TASK_NAME = "gfx";

bool g_bIsJobTransfer;
// Sumidero para los bytes RX que genera el reloj SPI al transmitir
static uint8_t g_dummyRxByte;

static void Render_ClearDMAQueue(void) {
  g_DMAQueue.head = 0;
  g_DMAQueue.tail = 0;
  g_DMAQueue.count = 0;
}

static bool Render_VerifyFullFrameRAMG(pixel16_t *pSourceBuffer) {
  uint32_t offset = 0;

  while (offset < GFX_BUFFER_SIZE_BYTES) {
    uint32_t chunk = GFX_BUFFER_SIZE_BYTES - offset;
    if (chunk > FULL_FRAME_VERIFY_CHUNK_BYTES) {
      chunk = FULL_FRAME_VERIFY_CHUNK_BYTES;
    }

    EVE_ReadMemoryBlock(RAM_G + offset, g_ui8VerifyScratch, chunk);

    if (memcmp(g_ui8VerifyScratch, ((uint8_t *)pSourceBuffer) + offset,
               chunk) != 0) {
      g_ui32LastFullFrameVerifyOffset = offset;
      g_ui32FullFrameVerifyFailures++;
      return false;
    }

    offset += chunk;
  }

  return true;
}

// SSI Interrupt handler
void SSI3IntHandler(void) {
  uint32_t ui32Status = MAP_SSIIntStatus(SSI3_BASE, 1);
  MAP_SSIIntClear(SSI3_BASE, ui32Status);

  if (g_bSPI_TransferActive && !MAP_uDMAChannelIsEnabled(UDMA_CH15_SSI3TX)) {

    while (MAP_SSIBusy(SSI3_BASE))
      ;
    HAL_TFT_SPI_CS_Disable();

    if (g_bIsJobTransfer) {

      g_DMAQueue.count--;
      g_DMAQueue.tail = (g_DMAQueue.tail + 1) % MAX_DMA_JOBS;

      if (g_DMAQueue.count > 0) {

        uint32_t trash;
        while (MAP_SSIDataGetNonBlocking(SSI3_BASE, &trash))
          ;
        MAP_SSIIntClear(SSI3_BASE, SSI_RXOR);

        DMARenderJob_t nextJob = g_DMAQueue.jobs[g_DMAQueue.tail];

        HAL_TFT_SPI_CS_Enable();

        if (g_bIsQuadActive) {
          MAP_SSIAdvModeSet(SSI3_BASE, SSI_ADV_MODE_QUAD_WRITE);
          MAP_SSIDataPut(SSI3_BASE, (uint8_t)((nextJob.destRAMG >> 16) | 0x80));
          MAP_SSIDataPut(SSI3_BASE, (uint8_t)(nextJob.destRAMG >> 8));
          MAP_SSIDataPut(SSI3_BASE, (uint8_t)(nextJob.destRAMG));
        } else {
          EVE_AddrForWr(nextJob.destRAMG);
        }

        // A. Reconfigurar "Sumidero" RX
        MAP_uDMAChannelControlSet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT,
                                  UDMA_SIZE_8 | UDMA_SRC_INC_NONE |
                                      UDMA_DST_INC_NONE | UDMA_ARB_1);
        MAP_uDMAChannelTransferSet(
            UDMA_CH14_SSI3RX | UDMA_PRI_SELECT, UDMA_MODE_BASIC,
            (void *)(SSI3_BASE + SSI_O_DR), &g_dummyRxByte, nextJob.length);

        // B. Reconfigurar TX
        MAP_uDMAChannelControlSet(UDMA_CH15_SSI3TX | UDMA_PRI_SELECT,
                                  UDMA_SIZE_8 | UDMA_SRC_INC_8 |
                                      UDMA_DST_INC_NONE | UDMA_ARB_1);
        MAP_uDMAChannelTransferSet(
            UDMA_CH15_SSI3TX | UDMA_PRI_SELECT, UDMA_MODE_BASIC,
            nextJob.pSrcSDRAM, (void *)(SSI3_BASE + SSI_O_DR), nextJob.length);

        // C. Habilitar AMBOS canales
        MAP_uDMAChannelEnable(UDMA_CH14_SSI3RX);
        MAP_uDMAChannelEnable(UDMA_CH15_SSI3TX);

        return;
      }
    }

    // LIMPIEZA FINAL
    MAP_SSIDMADisable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);
    MAP_SSIIntDisable(SSI3_BASE, SSI_DMATX);

    g_bSPI_TransferActive = false;
    g_bIsJobTransfer = false;
  }

  if (ui32Status & SSI_RXOR) {
    MAP_SSIIntClear(SSI3_BASE, SSI_RXOR);
  }
}

// --- BUILDER FUNCTIONS ---
/**
 * Pushes a new DMA Scatter-Gather job into the circular queue.
 * Returns true if successful, false if the queue is full.
 */
bool Render_PushDMAjob(DMARenderJob_t job) {
  // 1. Check for Queue Overflow
  if (g_DMAQueue.count >= MAX_DMA_JOBS) {
    // TIVA_LOGE("GFX", "DMA Queue Overflow! Increase MAX_DMA_JOBS.");
    return false;
  }

  // 2. Insert the job at the current 'head' index
  g_DMAQueue.jobs[g_DMAQueue.head] = job;

  // 3. Advance the 'head' with circular wrap-around
  g_DMAQueue.head = (g_DMAQueue.head + 1) % MAX_DMA_JOBS;

  // 4. Safely increment the job count
  g_DMAQueue.count++;

  return true;
}

void Render_BuildDynamicSG(uint8_t *pSrc, uint32_t totalBytes) {
  if (totalBytes == 0)
    return;

  uint32_t bytesRem = totalBytes;
  uint32_t chunk;
  uint32_t mode;
  int tCount = 0;

  // Pre-calculate links just in case we need to chain lists
  MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST2, g_txList2, 1);
  txLink2 = PRIMARY_CTRL_TABLE[15 & 0x1F];
  MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST2, g_rxList2, 1);
  rxLink2 = PRIMARY_CTRL_TABLE[14 & 0x1F];

  MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST1, g_txList1, 1);
  txLink1 = PRIMARY_CTRL_TABLE[15 & 0x1F];
  MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST1, g_rxList1, 1);
  rxLink1 = PRIMARY_CTRL_TABLE[14 & 0x1F];

  // --- POPULATE LIST 0 ---
  int i = 0;
  for (; i < TASKS_LIST0 - 1; i++) {
    if (bytesRem == 0)
      break;

    chunk = (bytesRem > TRANSFER_SIZE) ? TRANSFER_SIZE : bytesRem;
    mode = (bytesRem <= TRANSFER_SIZE) ? UDMA_MODE_BASIC
                                       : UDMA_MODE_PER_SCATTER_GATHER;

    g_txList0[i] = (tDMAControlTable)uDMATaskStructEntry(
        chunk, UDMA_SIZE_8, UDMA_SRC_INC_8, pSrc, UDMA_DST_INC_NONE,
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4, mode);
    g_rxList0[i] = (tDMAControlTable)uDMATaskStructEntry(
        chunk, UDMA_SIZE_8, UDMA_SRC_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR),
        UDMA_DST_INC_NONE, &g_ulDummyRx, UDMA_ARB_4, mode);

    pSrc += chunk;
    bytesRem -= chunk;
    tCount++;

    if (mode == UDMA_MODE_BASIC) {
      MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, tCount, g_txList0, 1);
      MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, tCount, g_rxList0, 1);
      return; // We finished within List 0!
    }
  }

  // Add Chain Link to List 1
  g_txList0[tCount] = (tDMAControlTable)uDMATaskStructEntry(
      4, UDMA_SIZE_32, UDMA_SRC_INC_32, &txLink1, UDMA_DST_INC_32,
      &PRIMARY_CTRL_TABLE[15], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
  g_rxList0[tCount] = (tDMAControlTable)uDMATaskStructEntry(
      4, UDMA_SIZE_32, UDMA_SRC_INC_32, &rxLink1, UDMA_DST_INC_32,
      &PRIMARY_CTRL_TABLE[14], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
  tCount++;

  // --- POPULATE LIST 1 ---
  tCount = 0;
  for (i = 0; i < TASKS_LIST1 - 1; i++) {
    if (bytesRem == 0)
      break;

    chunk = (bytesRem > TRANSFER_SIZE) ? TRANSFER_SIZE : bytesRem;
    mode = (bytesRem <= TRANSFER_SIZE) ? UDMA_MODE_BASIC
                                       : UDMA_MODE_PER_SCATTER_GATHER;

    g_txList1[i] = (tDMAControlTable)uDMATaskStructEntry(
        chunk, UDMA_SIZE_8, UDMA_SRC_INC_8, pSrc, UDMA_DST_INC_NONE,
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4, mode);
    g_rxList1[i] = (tDMAControlTable)uDMATaskStructEntry(
        chunk, UDMA_SIZE_8, UDMA_SRC_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR),
        UDMA_DST_INC_NONE, &g_ulDummyRx, UDMA_ARB_4, mode);

    pSrc += chunk;
    bytesRem -= chunk;
    tCount++;

    if (mode == UDMA_MODE_BASIC) {
      MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, tCount, g_txList1, 1);
      MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, tCount, g_rxList1, 1);
      // Arm List 0 so it starts the chain!
      MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST0, g_txList0,
                                      1);
      MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST0, g_rxList0,
                                      1);
      return;
    }
  }

  // Add Chain Link to List 2
  g_txList1[tCount] = (tDMAControlTable)uDMATaskStructEntry(
      4, UDMA_SIZE_32, UDMA_SRC_INC_32, &txLink2, UDMA_DST_INC_32,
      &PRIMARY_CTRL_TABLE[15], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
  g_rxList1[tCount] = (tDMAControlTable)uDMATaskStructEntry(
      4, UDMA_SIZE_32, UDMA_SRC_INC_32, &rxLink2, UDMA_DST_INC_32,
      &PRIMARY_CTRL_TABLE[14], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);

  // --- POPULATE LIST 2 ---
  tCount = 0;
  for (i = 0; i < TASKS_LIST2; i++) {
    if (bytesRem == 0)
      break;

    chunk = (bytesRem > TRANSFER_SIZE) ? TRANSFER_SIZE : bytesRem;
    mode = (bytesRem <= TRANSFER_SIZE) ? UDMA_MODE_BASIC
                                       : UDMA_MODE_PER_SCATTER_GATHER;

    g_txList2[i] = (tDMAControlTable)uDMATaskStructEntry(
        chunk, UDMA_SIZE_8, UDMA_SRC_INC_8, pSrc, UDMA_DST_INC_NONE,
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4, mode);
    g_rxList2[i] = (tDMAControlTable)uDMATaskStructEntry(
        chunk, UDMA_SIZE_8, UDMA_SRC_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR),
        UDMA_DST_INC_NONE, &g_ulDummyRx, UDMA_ARB_4, mode);

    pSrc += chunk;
    bytesRem -= chunk;
    tCount++;

    if (mode == UDMA_MODE_BASIC) {
      MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, tCount, g_txList2, 1);
      MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, tCount, g_rxList2, 1);
      // Arm List 0 to start the massive chain!
      MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST0, g_txList0,
                                      1);
      MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST0, g_rxList0,
                                      1);
      return;
    }
  }
}

void Render_BuildSGFromBuffer(uint8_t *pBuffer) {
  uint8_t *pSrc = pBuffer;
  int i;

  // STEP 1: CALCULATE LINK STRUCTURES
  MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST2, g_txList2, 1);
  txLink2 = PRIMARY_CTRL_TABLE[15 & 0x1F];
  MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST2, g_rxList2, 1);
  rxLink2 = PRIMARY_CTRL_TABLE[14 & 0x1F];

  MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST1, g_txList1, 1);
  txLink1 = PRIMARY_CTRL_TABLE[15 & 0x1F];
  MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST1, g_rxList1, 1);
  rxLink1 = PRIMARY_CTRL_TABLE[14 & 0x1F];

  // STEP 2: POPULATE LIST 0
  for (i = 0; i < TASKS_LIST0 - 1; i++) {
    g_txList0[i] = (tDMAControlTable)uDMATaskStructEntry(
        TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_8, pSrc, UDMA_DST_INC_NONE,
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4,
        UDMA_MODE_PER_SCATTER_GATHER);
    g_rxList0[i] = (tDMAControlTable)uDMATaskStructEntry(
        TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_NONE,
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_DST_INC_NONE, &g_ulDummyRx,
        UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
    pSrc += TRANSFER_SIZE;
  }
  g_txList0[TASKS_LIST0 - 1] = (tDMAControlTable)uDMATaskStructEntry(
      4, UDMA_SIZE_32, UDMA_SRC_INC_32, &txLink1, UDMA_DST_INC_32,
      &PRIMARY_CTRL_TABLE[15], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
  g_rxList0[TASKS_LIST0 - 1] = (tDMAControlTable)uDMATaskStructEntry(
      4, UDMA_SIZE_32, UDMA_SRC_INC_32, &rxLink1, UDMA_DST_INC_32,
      &PRIMARY_CTRL_TABLE[14], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);

  // STEP 3: POPULATE LIST 1
  for (i = 0; i < TASKS_LIST1 - 1; i++) {
    g_txList1[i] = (tDMAControlTable)uDMATaskStructEntry(
        TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_8, pSrc, UDMA_DST_INC_NONE,
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4,
        UDMA_MODE_PER_SCATTER_GATHER);
    g_rxList1[i] = (tDMAControlTable)uDMATaskStructEntry(
        TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_NONE,
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_DST_INC_NONE, &g_ulDummyRx,
        UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
    pSrc += TRANSFER_SIZE;
  }
  g_txList1[TASKS_LIST1 - 1] = (tDMAControlTable)uDMATaskStructEntry(
      4, UDMA_SIZE_32, UDMA_SRC_INC_32, &txLink2, UDMA_DST_INC_32,
      &PRIMARY_CTRL_TABLE[15], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
  g_rxList1[TASKS_LIST1 - 1] = (tDMAControlTable)uDMATaskStructEntry(
      4, UDMA_SIZE_32, UDMA_SRC_INC_32, &rxLink2, UDMA_DST_INC_32,
      &PRIMARY_CTRL_TABLE[14], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);

  // STEP 4: POPULATE LIST 2
  for (i = 0; i < TASKS_LIST2; i++) {
    uint32_t mode =
        (i == TASKS_LIST2 - 1) ? UDMA_MODE_BASIC : UDMA_MODE_PER_SCATTER_GATHER;
    g_txList2[i] = (tDMAControlTable)uDMATaskStructEntry(
        TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_8, pSrc, UDMA_DST_INC_NONE,
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4, mode);
    g_rxList2[i] = (tDMAControlTable)uDMATaskStructEntry(
        TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_NONE,
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_DST_INC_NONE, &g_ulDummyRx,
        UDMA_ARB_4, mode);
    pSrc += TRANSFER_SIZE;
  }

  // STEP 5: ARM LIST 0
  MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST0, g_txList0, 1);
  MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST0, g_rxList0, 1);
}

bool Render_BuildScatterGatherSegments(pixel16_t *pActiveBuffer, int16_t x, int16_t y,
                              int16_t w, int16_t h) {
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > 800)
    w = 800 - x;
  if (y + h > 480)
    h = 480 - y;
  if (w <= 0 || h <= 0)
    return true;

  uint16_t totalBytesPerRow = w * 2;
  // TM4C Hardware Limit: Maximum 1024 items per basic uDMA transfer
  const uint16_t MAX_DMA_TRANSFER = 1024;
  // const uint16_t MAX_DMA_TRANSFER = 600;

  // TIVA_LOGI(TASK_NAME, "Segments to build: (%u, %u) -> [%u, %u]", x, y, w,
  // h);
  int row = 0;
  for (; row < h; row++) {
    uint32_t screenOffset = ((y + row) * 800) + x;

    uint32_t currentDestRAMG = screenOffset * 2;
    uint8_t *pCurrentSDRAM = (uint8_t *)&pActiveBuffer[screenOffset];
    int32_t bytesRemaining = totalBytesPerRow;

    // Slice wide rows into uDMA-safe chunks (<= 1024 bytes)
    while (bytesRemaining > 0) {
      if (g_DMAQueue.count >= MAX_DMA_JOBS)
        return false; // Queue full!

      uint16_t chunkLength = (bytesRemaining > MAX_DMA_TRANSFER)
                                 ? MAX_DMA_TRANSFER
                                 : bytesRemaining;

      DMARenderJob_t newJob;
      newJob.pSrcSDRAM = pCurrentSDRAM;
      newJob.destRAMG = currentDestRAMG;
      newJob.length = chunkLength;

      // Push to ring buffer safely
      g_DMAQueue.jobs[g_DMAQueue.head] = newJob;
      g_DMAQueue.head = (g_DMAQueue.head + 1) % MAX_DMA_JOBS;
      g_DMAQueue.count++;

      // Advance pointers for the next chunk of this row
      pCurrentSDRAM += chunkLength;
      currentDestRAMG += chunkLength;
      bytesRemaining -= chunkLength;
    }
  }
  return true;
}

void Render_StartSGTransfer(bool bIsBlocking) {
  g_bSPI_TransferActive = true;

  MAP_uDMAErrorStatusClear();
  MAP_SSIIntDisable(SSI3_BASE,
                    SSI_DMATX | SSI_DMARX | SSI_RXFF | SSI_RXOR | SSI_RXTO);
  MAP_SSIIntClear(SSI3_BASE,
                  SSI_DMATX | SSI_DMARX | SSI_RXFF | SSI_RXOR | SSI_RXTO);

  uint32_t trash;
  while (MAP_SSIDataGetNonBlocking(SSI3_BASE, &trash))
    ;
  MAP_SSIIntClear(SSI3_BASE, SSI_RXOR | SSI_RXTO);

  MAP_uDMAChannelEnable(UDMA_CH14_SSI3RX);
  MAP_uDMAChannelEnable(UDMA_CH15_SSI3TX);
  MAP_SSIDMAEnable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);
  if (!bIsBlocking) {
    MAP_SSIIntEnable(SSI3_BASE, SSI_DMATX);
  }

  MAP_uDMAChannelRequest(UDMA_CH15_SSI3TX);

  if (bIsBlocking) {
    while (MAP_uDMAChannelIsEnabled(UDMA_CH15_SSI3TX)) {
      if (HWREG(SSI3_BASE + SSI_O_RIS) & SSI_RIS_RORRIS) {
        HWREG(SSI3_BASE + SSI_O_ICR) = SSI_ICR_RORIC;
      }
    }
    while (MAP_SSIBusy(SSI3_BASE))
      ;
    MAP_SSIDMADisable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);
    g_bSPI_TransferActive = false;
  }
}

static void onFullRepaintEvent(EventParam_t evt) {
  g_bRequestFullRepaint = true;
}


void Render_SendFullFrame(bool bIsBlocking) {
  uint8_t attempts = 0;

  while (g_bSPI_TransferActive)
    ;

  Render_ClearDMAQueue();

  do {
    Render_BuildSGFromBuffer((uint8_t *)g_pDrawingBuffer);

    HAL_TFT_SPI_CS_Enable();
    EVE_AddrForWr(RAM_G);

    while (MAP_SSIBusy(SSI3_BASE))
      ;

    Render_StartSGTransfer(bIsBlocking);

    if (!bIsBlocking) {
      g_bPendingFullFrameVerify = true;
      return;
    }

    HAL_TFT_SPI_CS_Disable();
    g_bIsBackgroundReady = true;

    if (Render_VerifyFullFrameRAMG(g_pDrawingBuffer)) {
      return;
    }

    attempts++;
  } while (attempts < FULL_FRAME_MAX_RETRIES);
}

void Render_SendContinuosBlockSG(pixel16_t *pActiveBuffer, int16_t x, int16_t y, 
								int16_t w, int16_t h) {
  // 1. Bounds checking
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > 800)
    w = 800 - x;
  if (y + h > 480)
    h = 480 - y;
  if (w <= 0 || h <= 0)
    return;

  // 2. Memory Math
  uint32_t startOffset = (y * 800) + x;
  uint32_t endOffset = ((y + h - 1) * 800) + (x + w);
  uint32_t totalBytesToSend = (endOffset - startOffset) * 2;

  uint8_t *pSrcSDRAM = (uint8_t *)&pActiveBuffer[startOffset];
  uint32_t startingRAMG = startOffset * 2;

  // 3. Build the SG Links dynamically
  Render_BuildDynamicSG(pSrcSDRAM, totalBytesToSend);

  // 4. Assert CS and manually send the single address
  HAL_TFT_SPI_CS_Enable();
  HWREG(SSI3_BASE + SSI_O_DR) = (startingRAMG >> 16) | 0x80;
  HWREG(SSI3_BASE + SSI_O_DR) = (startingRAMG >> 8) & 0xFF;
  HWREG(SSI3_BASE + SSI_O_DR) = startingRAMG & 0xFF;
  while (MAP_SSIBusy(SSI3_BASE))
    ;

  // 5. Fire the Scatter Gather DMA and poll until complete
  Render_StartSGTransfer(true);

  // g_RenderEngine.state = RENDER_WAIT_SG_ISR;

  // 6. Release CS to complete the FT81x transaction
  // HAL_TFT_SPI_CS_Disable();
}

void Render_DisplayFrameBitmap(void) {
  uint16_t ui16Width = 800;
  uint16_t ui16Height = 480;

  // API_LIB_BeginCoProList();
  // API_CMD_DLSTART();
  // API_CLEAR_COLOR_RGB(255, 19, 30);
  // API_CLEAR(1, 1, 1);
  // API_COLOR_RGB(255, 255, 255);

  API_BITMAP_HANDLE(0);
  API_BITMAP_SOURCE(RAM_G);
  uint16_t BytesPerPixel = 2;
  uint16_t ui16Stride = ui16Width * BytesPerPixel;

  uint8_t u8Scale = 1;
  uint16_t u16DrawnWidth = ui16Width * u8Scale;
  uint16_t u16DrawnHeight = ui16Height * u8Scale;

  API_BITMAP_LAYOUT(RGB565, ui16Stride, ui16Height);
  API_BITMAP_LAYOUT_H(ui16Stride >> 10, ui16Height >> 9);
  API_BITMAP_SIZE(NEAREST, BORDER, BORDER, u16DrawnWidth, u16DrawnHeight);
  API_BITMAP_SIZE_H(u16DrawnWidth >> 9, u16DrawnHeight >> 9);

  int32_t s32ScaleFactor = 65536 * u8Scale;
  API_CMD_LOADIDENTITY();
  API_CMD_SCALE(s32ScaleFactor, s32ScaleFactor);
  API_CMD_SETMATRIX();

  API_BEGIN(BITMAPS);
  API_VERTEX2II(0, 0, 0, 0);
  API_END();

  // API_DISPLAY();
  // API_CMD_SWAP();
  // API_LIB_EndCoProList();
  // API_LIB_AwaitCoProEmpty();

  // Signal that the background is displayed
  g_bIsBackgroundReady = true;
}

void Render_DisplayFrame(void) {
  API_LIB_BeginCoProList();
  API_CMD_DLSTART();
  API_CLEAR_COLOR_RGB(0, 0, 0);
  API_CLEAR(1, 1, 1);

  // Dibuja la imagen base (G_RAM -> Pantalla)
  Render_DisplayFrameBitmap();

  // Dibuja gráficas / Líneas / Imágenes SD encima
  FormManager_RenderEVEComponents();

  API_DISPLAY();
  API_CMD_SWAP();
  API_LIB_EndCoProList();
}

void Render_Task(void) {
  // bHardwareNeedsUpdate es estática para recordar si necesitamos
  // redibujar el hardware después de que termine una transferencia DMA
  // asíncrona.
  static bool bHardwareNeedsUpdate = false;

  switch (g_RenderEngine.state) {

  case RENDER_IDLE:
    // ---------------------------------------------------------
    // REGLA DE ORO: Si el bus SPI está siendo utilizado por el DMA
    // y la ISR en background, abortamos y cedemos el ciclo de CPU.
    // ---------------------------------------------------------
    if (g_bSPI_TransferActive) {
      return;
    }

    // 2. Override: Repintado Completo (Ej. Cambio de pantalla/tema)
    if (g_bRequestFullRepaint) {
      // Compositor monolítico
	  FormManager_CompositeFrame(g_pDrawingBuffer);
      // Sobrescribe la cola con un trabajo gigante o una ráfaga de trabajos
      // para enviar el buffer completo (g_pDrawingBuffer) a la RAM_G.
      g_ui8FullFrameRetryCount = 0;
	  Render_SendFullFrame(false);
      g_RenderEngine.state = RENDER_WAIT_DMA;
      g_bRequestFullRepaint = false;
      break;
    }

    // 1. Recolector de Bounding Boxes (Dirty Rectangles)
    // Si hay widgets de software modificados, esta función itera sobre ellos,
    // dibuja en SDRAM su nueva apariencia, y mete automáticamente
    // los trabajos a la cola g_DMAQueue.
	FormManager_CheckSoftwareDirty();

    // Evalúa si componentes puramente de EVE (gráficas) cambiaron.
    bHardwareNeedsUpdate = FormManager_CheckHardwareDirty();
	//bHardwareNeedsUpdate = true;

    // 3. ¿Tenemos bloques listos para transferir vía uDMA?
    if (g_DMAQueue.count > 0) {

      // VACIAR TODA LA COLA SIN SALIR DEL ESTADO
      while (g_DMAQueue.count > 0) {

        DMARenderJob_t job = g_DMAQueue.jobs[g_DMAQueue.tail];
        g_DMAQueue.tail = (g_DMAQueue.tail + 1) % MAX_DMA_JOBS;
        g_DMAQueue.count--;

        HAL_TFT_SPI_CS_Enable();

        uint8_t *pData = (uint8_t *)job.pSrcSDRAM;
        uint32_t i = 0;

        if (g_bIsQuadActive) {
          MAP_SSIAdvModeSet(SSI3_BASE, SSI_ADV_MODE_QUAD_WRITE);
          MAP_SSIDataPut(SSI3_BASE,
                         (uint8_t)((job.destRAMG >> 16) | 0x80)); // MEM_WRITE
          MAP_SSIDataPut(SSI3_BASE, (uint8_t)(job.destRAMG >> 8));
          MAP_SSIDataPut(SSI3_BASE, (uint8_t)(job.destRAMG));

          for (; i < job.length; i++) {
            MAP_SSIDataPut(SSI3_BASE, pData[i]);
          }
        } else {
          EVE_AddrForWr(job.destRAMG);
          for (i = 0; i < job.length; i++) {
            HAL_TFT_SPI_ReadWrite8(pData[i]);
          }
        }

        // Esperar a que los bytes de este bloque salgan físicamente
        while (MAP_SSIBusy(SSI3_BASE)) {
        }

        HAL_TFT_SPI_CS_Disable();
      }

      // Toda la RAM_G ha sido actualizada, pasamos a renderizar EVE
      g_RenderEngine.state = RENDER_EVE_COMPONENTS;
    } 
	if (bHardwareNeedsUpdate) {
      g_RenderEngine.state = RENDER_EVE_COMPONENTS;
    }

    // 4. FAST PATH: Si no hay trabajos DMA, pero el hardware EVE sí cambió.
    break;

  case RENDER_WAIT_DMA:
    // ---------------------------------------------------------
    // El CPU está libre. La ISR (SSI3IntHandler) está vaciando
    // la cola de trabajos DMA en segundo plano.
    // ---------------------------------------------------------

    // La ISR bajará esta bandera cuando g_DMAQueue.count llegue a 0
    if (!g_bSPI_TransferActive) {
      if (g_bPendingFullFrameVerify) {
        if (!Render_VerifyFullFrameRAMG(g_pDrawingBuffer)) {
          if (g_ui8FullFrameRetryCount < FULL_FRAME_MAX_RETRIES) {
            g_ui8FullFrameRetryCount++;
            Render_SendFullFrame(false);
            g_RenderEngine.state = RENDER_WAIT_DMA;
            break;
          }
        } else if (g_ui8FullFrameRetryCount > 0) {
          g_ui32FullFrameRetrySuccesses++;
        }

        g_bPendingFullFrameVerify = false;
        g_ui8FullFrameRetryCount = 0;
      }

      g_RenderEngine.state = RENDER_EVE_COMPONENTS;
    }
    break;

  case RENDER_EVE_COMPONENTS:

    if (g_bRequestFullRepaint) {
      g_RenderEngine.state = RENDER_IDLE;
      break;
    }
    // Construimos la Display List con el nuevo fondo en RAM_G y los Overlays
    Render_DisplayFrame();
    // ---------------------------------------------------------
    // Limpieza de estado de frame
    // ---------------------------------------------------------
    bHardwareNeedsUpdate = false;

    g_RenderEngine.state = RENDER_IDLE;
    break;
  }
}

bool Render_Init(const uint16_t ui16ResWidth, const uint16_t ui16ResHeight) {
  uint32_t bufferSizeInBytes = ui16ResHeight * ui16ResWidth * sizeof(uint16_t);
  g_pBufferA = (pixel16_t *)malloc(bufferSizeInBytes);

  if (!g_pBufferA)
    return false;

  memset(g_pBufferA, 0x00, bufferSizeInBytes);

  g_pDrawingBuffer = g_pBufferA;

  Event_Subscribe(EVT_CMD_FULL_REPAINT, (EventHandler_fn)onFullRepaintEvent);

  return true;
}



