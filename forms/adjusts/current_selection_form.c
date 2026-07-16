#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "gui_core.h"
#include "gui_theme.h"
#include "gui_canvas.h"
#include "forms_manager.h"
#include "FT8xx_params.h"
#include "event_engine.h"
#include "experiments_cfg.h"
#include "helpers.h"

#include "forms/common_widgets.h"
#include "forms/adjusts/adjust_numpad_form.h"

#include "current_selection_form.h"

int16_t g_i16CurrentSelectionIndex = 0; 

gfx_Canvas g_sCurrentSelectionCanvas;

// Container
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget formSubtitleWidget;
static gfx_GenericWidget currentPrimaryBtnWidget, currentSecondaryBtnWidget;
static gfx_GenericWidget currentPrimaryFrameWidget, currentSecondaryFrameWidget;
static gfx_GenericWidget currentPrimaryLbWidget, currentSecondaryLbWidget;
static gfx_GenericWidget cancelBtnWidget;

// Widgets
static gfx_Label formTitleData;
static gfx_Label formSubtitleData;
static gfx_Button currentPrimaryBtnData, currentSecondaryBtnData;
static gfx_Label currentPrimaryLbData, currentSecondaryLbData;
static gfx_Rectangle currentPrimaryFrameData, currentSecondaryFrameData;
static gfx_Button cancelBtnData;

// Buffers
static char currentPrimaryBuff[] = "100.0 [A]";
static char currentSecondaryBuff[] = "100.0 [A]";

static EventID_e g_eFormCallback = EVT_SYS_NULL;

// Callbacks
static void onCancelBtnRelased(gfx_Button *btn) {
	onGenericBtnRelease(btn);

	Event_Post(EVT_SYS_SHOW_OPTIONS_FORM, (EventParam_t){.ptr = NULL});
}

static void onPrimaryCurrentValue(EventParam_t arg) {
	if(GetExecTimeMs() % 300 < 20) {
		sprintf(currentPrimaryBuff, "%.1f [A]", arg.f32);
		currentPrimaryLbData.bIsDirty = true;
	}
}

static void onSecondaryCurrentValue(EventParam_t arg) {
	if(GetExecTimeMs() % 300 < 20) {
		sprintf(currentSecondaryBuff, "%.1f [A]", arg.f32);
		currentSecondaryLbData.bIsDirty = true;
	}
}

static void onPrimaryBtnReleased(gfx_Button *btn) {
	onGenericBtnRelease(btn);

	Event_Post(EVT_SYS_SHOW_ADJ_NUMPAD_FORM, (EventParam_t){.ui32 = ADJ_CURRENT_PRIMARY});
}

static void onSecondaryBtnReleased(gfx_Button *btn) {
	onGenericBtnRelease(btn);

	Event_Post(EVT_SYS_SHOW_ADJ_NUMPAD_FORM, (EventParam_t){.ui32 = ADJ_CURRENT_SECONDARY});
}

void initCurrentSelectionForm(void) {
	g_sCurrentSelectionCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
	
	formTitleData = (gfx_Label) {
		.name = "formTitleData",
		.text = "CORRIENTES",
        .pos.x = 110,
        .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
	};
	formTitleWidget.eWidgetType = WD_TYPE_LABEL;
	formTitleWidget.pvWidget = (void *)&formTitleData;

	formSubtitleData = (gfx_Label){
		.text = "SELECCIONE UNA CORRIENTE PARA AJUSTAR",
		.pos.x = LCD_WIDTH / 2,
		.pos.y = 100,
		.alignment = ALIGN_CENTER,
		.style = STYLE_DANGER,
		.typo = TYPO_H3,
		.isVisible = true,
	};
	formSubtitleWidget.eWidgetType = WD_TYPE_LABEL;
	formSubtitleWidget.pvWidget = (void *)&formSubtitleData;

	uint16_t workingHeight = 310;
	uint16_t btnWidth = LCD_WIDTH / 3;
	uint16_t btnHeight = ( workingHeight - 80) / 3.0f;

	currentPrimaryBtnData = (gfx_Button) {
		.name = "tempBtn",
		.label = "PRIMARIO",
		.pos.x = (LCD_WIDTH / 2 - btnWidth),
		.pos.y = formSubtitleData.pos.y + 40,
		.size.width = btnWidth,
		.size.height = btnHeight,
		.borderWidth = 1,
		.radius = 5,
		.state = BTN_STATE_NORMAL,
		.style = STYLE_SECONDARY,
		.typo = TYPO_H3,
		.bIsVisible = true, 
		.onPressed = onGenericBtnPressed,
		.onRelease = onPrimaryBtnReleased,
	};
	gfx_initRegTouch((void *)&currentPrimaryBtnData, WD_TYPE_BUTTON);
	currentPrimaryBtnWidget.eWidgetType = WD_TYPE_BUTTON;
	currentPrimaryBtnWidget.pvWidget = (void *)&currentPrimaryBtnData;

	currentSecondaryBtnData = (gfx_Button) {
		.name = "currentBtn",
		.label = "SECUNDARIO",
		.pos.x = (LCD_WIDTH / 2 - btnWidth),
		.pos.y = currentPrimaryBtnData.pos.y + btnHeight + 20,
		.size.width = btnWidth,
		.size.height = btnHeight,
		.borderWidth = 1,
		.radius = 5,
		.state = BTN_STATE_NORMAL,
		.style = STYLE_DEFAULT,
		.typo = TYPO_H3,
		.bIsVisible = true, 
		.onPressed = onGenericBtnPressed,
		.onRelease = onSecondaryBtnReleased,
	};
	gfx_initRegTouch((void *)&currentSecondaryBtnData, WD_TYPE_BUTTON);
	currentSecondaryBtnWidget.eWidgetType = WD_TYPE_BUTTON;
	currentSecondaryBtnWidget.pvWidget = (void *)&currentSecondaryBtnData;

	currentPrimaryFrameData = (gfx_Rectangle) {
		.name = "currentPrimaryFrame",
		.pos.x = currentPrimaryBtnData.pos.x + currentPrimaryBtnData.size.width + 30,
		.pos.y = currentPrimaryBtnData.pos.y,
		.dim.height = btnHeight,
		.dim.width = btnWidth,
		.round = 6,
		.borderWidth = 3,
		.color = g_pCurrentTheme->palette.surface,
	};
	currentPrimaryFrameWidget.eWidgetType = WD_TYPE_RECT;
	currentPrimaryFrameWidget.pvWidget = (void *)&currentPrimaryFrameData;

	currentPrimaryLbData = (gfx_Label) {
		.name = "currentPrimaryLabel",
		.text = currentPrimaryBuff,
		.pos.x = currentPrimaryFrameData.pos.x + currentPrimaryFrameData.dim.width / 2,
		.pos.y = currentPrimaryFrameData.pos.y + currentPrimaryFrameData.dim.height / 2,
		.alignment = ALIGN_CENTER,
		.isVisible = true,
		.style = STYLE_TEXT_MAIN,
		.typo = TYPO_H3,
	};
	currentPrimaryLbWidget.eWidgetType = WD_TYPE_LABEL;
	currentPrimaryLbWidget.pvWidget = (void *)&currentPrimaryLbData;

	currentSecondaryFrameData = (gfx_Rectangle) {
		.name = "currentPrimaryFrame",
		.pos.x = currentPrimaryBtnData.pos.x + currentSecondaryBtnData.size.width + 30,
		.pos.y = currentSecondaryBtnData.pos.y,
		.dim.height = btnHeight,
		.dim.width = btnWidth,
		.round = 6,
		.borderWidth = 3,
		.color = g_pCurrentTheme->palette.surface,
	};
	currentSecondaryFrameWidget.eWidgetType = WD_TYPE_RECT;
	currentSecondaryFrameWidget.pvWidget = (void *)&currentSecondaryFrameData;

	currentSecondaryLbData = (gfx_Label) {
		.name = "currentPrimaryLabel",
		.text = currentSecondaryBuff,
		.pos.x = currentSecondaryFrameData.pos.x + currentSecondaryFrameData.dim.width / 2,
		.pos.y = currentSecondaryFrameData.pos.y + currentSecondaryFrameData.dim.height / 2,
		.alignment = ALIGN_CENTER,
		.isVisible = true,
		.style = STYLE_TEXT_MAIN,
		.typo = TYPO_H3,
	};
	currentSecondaryLbWidget.eWidgetType = WD_TYPE_LABEL;
	currentSecondaryLbWidget.pvWidget = (void *)&currentSecondaryLbData;

	cancelBtnData = (gfx_Button) {
		.name = "cancelBtn",
		.label = "CANCELAR",
		.size.width = (LCD_WIDTH - 60) / 3.0,
		.size.height = 60,
		.pos.x = LCD_WIDTH / 2 - (LCD_WIDTH - 60) / 6.0,
		.pos.y = LCD_HEIGHT - 130,
		.radius = 4,
		.borderWidth = 2,
		.bIsVisible = true,
		.state = BTN_STATE_NORMAL,
		.style = STYLE_DANGER,
		.typo = TYPO_H3,
		.onPressed = onGenericBtnPressed,
		.onRelease = onCancelBtnRelased,
	};
	gfx_initRegTouch((void *)&cancelBtnData, WD_TYPE_BUTTON);
	cancelBtnWidget.eWidgetType = WD_TYPE_BUTTON;
	cancelBtnWidget.pvWidget = (void *)&cancelBtnData;

	useFullHeader(&g_sCurrentSelectionCanvas);
	useNavigationButtons(&g_sCurrentSelectionCanvas);
	canvasInsertAtTop(&g_sCurrentSelectionCanvas.psWidgets, &formTitleWidget);
	canvasInsertAtTop(&g_sCurrentSelectionCanvas.psWidgets, &formSubtitleWidget);
	canvasInsertAtTop(&g_sCurrentSelectionCanvas.psWidgets, &currentPrimaryBtnWidget);
	canvasInsertAtTop(&g_sCurrentSelectionCanvas.psWidgets, &currentSecondaryBtnWidget);
	canvasInsertAtTop(&g_sCurrentSelectionCanvas.psWidgets, &currentPrimaryFrameWidget);
	canvasInsertAtTop(&g_sCurrentSelectionCanvas.psWidgets, &currentSecondaryFrameWidget);
	canvasInsertAtTop(&g_sCurrentSelectionCanvas.psWidgets, &currentSecondaryLbWidget);
	canvasInsertAtTop(&g_sCurrentSelectionCanvas.psWidgets, &currentPrimaryLbWidget);
	canvasInsertAtTop(&g_sCurrentSelectionCanvas.psWidgets, &cancelBtnWidget);
	
	Event_Subscribe(EVT_CAN_INST_CURRENT_PRIMARY, (EventHandler_fn)onPrimaryCurrentValue);
	Event_Subscribe(EVT_CAN_INST_CURRENT_SECUNDARY, (EventHandler_fn)onSecondaryCurrentValue);

	g_i16CurrentSelectionIndex = FormManager_AddForm(&g_sCurrentSelectionCanvas);
}

