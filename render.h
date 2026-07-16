#ifndef RENDER_MODULE_H_
#define RENDER_MODULE_H_

#include <stdint.h>
#include <stdbool.h>

// #include "gfx.h"

typedef union
{
	uint16_t u16;
	uint8_t u8[2];
} pixel16_t;

typedef struct
{
	uint16_t ui16Width;
	uint16_t ui16Height;
	uint32_t ui32BuffSize;
	pixel16_t *psPixelBuffer; 
} GfxLayer_t;

extern pixel16_t *g_pDrawingBuffer;
extern pixel16_t *g_pSendingBuffer;

extern uint32_t g_ui32StartTxCycles;
extern uint32_t g_ui32EndTxCycles;
extern volatile uint32_t g_ui32FullFrameVerifyFailures;
extern volatile uint32_t g_ui32FullFrameRetrySuccesses;
extern volatile uint32_t g_ui32LastFullFrameVerifyOffset;

extern bool g_bIsBackgroundReady;

// --- DIRTY RECTANGLE QUEUE STRUCTURES ---
typedef struct {
    uint8_t *pSrcSDRAM;  
    uint32_t destRAMG; 
    uint16_t length;   
} DMARenderJob_t;

#define MAX_DMA_JOBS 1024

typedef struct {
    DMARenderJob_t jobs[MAX_DMA_JOBS];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} DMAJobQueue_t;

typedef enum {
    RENDER_IDLE = 0,
    RENDER_WAIT_DMA,
	RENDER_EVE_COMPONENTS,
	RENDER_FINISHED,
} RenderState_e;

typedef struct {
    RenderState_e state;
} RenderEngine_t;

extern volatile DMAJobQueue_t g_DMAQueue;
extern volatile RenderEngine_t g_RenderEngine;

extern bool g_bIsJobTransfer; 

void Render_StartSGTransfer(bool bIsBlocking);

void Render_BuildDynamicSG(uint8_t *pSrc, uint32_t totalBytes);

bool Render_BuildScatterGatherSegments(pixel16_t *pActiveBuffer, int16_t x, int16_t y,
                              int16_t w, int16_t h);

void Render_SendContinuosBlockSG(pixel16_t *pActiveBuffer, int16_t x, int16_t y, int16_t w, int16_t h);

void Render_DisplayBitmap(void);

void Render_SendFullFrame(bool bIsBlocking);

bool Render_PushDMAjob(DMARenderJob_t job);

void Render_DisplayFrame(void );

void Render_Task(void);

bool Render_Init(const uint16_t ui16ResWidth, const uint16_t ui16ResHeight);

#endif // RENDER_MODULE_H_


