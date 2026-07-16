#ifndef FORM_MANAGER_H
#define FORM_MANAGER_H

#include "gui_core.h"
#include "gesture_engine.h"

#define MAX_FORM_NUMBER		5

typedef struct {
	gfx_Canvas *canvas;
	char name[10];
} Form_Entry_t;

extern gfx_Canvas *g_psCurrentForm;

bool FormManager_HandleGesture(TouchStatus touchStatus, gesture_type_e gesture);

bool FormManager_CheckHardwareDirty(void);

bool FormManager_CheckSoftwareDirty(void);

void FormManager_CompositeFrame(pixel16_t *psPixelBuffer);

void FormManager_RenderEVEComponents(void);

int16_t FormManager_AddForm(gfx_Canvas *form);

void FormManager_Init(void);

#endif


