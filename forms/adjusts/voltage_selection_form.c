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

#include "voltage_selection_form.h"

int16_t g_i16VoltageSelectionIndex = 0; 

gfx_Canvas g_sVoltageSelectionCanvas;

// Container
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget formSubtitleWidget;
static gfx_GenericWidget voltPrimaryBtnWidget, voltSecondaryBtnWidget;
static gfx_GenericWidget voltPrimaryFrameWidget, voltSecondaryFrameWidget;
static gfx_GenericWidget voltPrimaryLbWidget, voltSecondaryLbWidget;
static gfx_GenericWidget cancelBtnWidget;

// Widgets
static gfx_Label formTitleData;
static gfx_Label formSubtitleData;
static gfx_Button voltPrimaryBtnData, voltSecondaryBtnData;
static gfx_Label voltPrimaryLbData, voltSecondaryLbData;
static gfx_Rectangle voltPrimaryFrameData, voltSecondaryFrameData;
static gfx_Button cancelBtnData;

// Buffers
static char voltPrimaryBuff[] = "100.0 [V]";
static char voltSecondaryBuff[] = "100.0 [V]";

static EventID_e g_eFormCallback = EVT_SYS_NULL;

// Callbacks
static void onShowThisFormEvent(EventParam_t arg) {

}

static void onCancelBtnRelased(gfx_Button *btn) {
	onGenericBtnRelease(btn);

	Event_Post(EVT_SYS_SHOW_OPTIONS_FORM, (EventParam_t){.ptr = NULL});
}

static void onPrimaryTxVoltageValue(EventParam_t arg) {
	if(GetExecTimeMs() % 300 < 20) {
		sprintf(voltPrimaryBuff, "%.1f [V]", arg.f32);
		voltPrimaryLbData.bIsDirty = true;
	}
}

static void onSecondaryTxVoltageValue(EventParam_t arg) {
	if(GetExecTimeMs() % 300 < 20) {
		sprintf(voltSecondaryBuff, "%.1f [V]", arg.f32);
		voltSecondaryLbData.bIsDirty = true;
	}
}

static void onPrimaryBtnReleased(gfx_Button *btn) {
	onGenericBtnRelease(btn);

	Event_Post(EVT_SYS_SHOW_ADJ_NUMPAD_FORM, (EventParam_t){.ui32 = ADJ_VOLTAGE_PRIMARY});
}

static void onSecondaryBtnReleased(gfx_Button *btn) {
	onGenericBtnRelease(btn);

	Event_Post(EVT_SYS_SHOW_ADJ_NUMPAD_FORM, (EventParam_t){.ui32 = ADJ_VOLTAGE_SECONDARY});
}

void initVoltageSelectionIndex(void) {
	g_sVoltageSelectionCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
	
	formTitleData = (gfx_Label) {
		.name = "formTitleData",
		.text = "VOLTAJES",
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
		.text = "SELECCIONE UN VOLTAJE PARA AJUSTAR",
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

	voltPrimaryBtnData = (gfx_Button) {
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
	gfx_initRegTouch((void *)&voltPrimaryBtnData, WD_TYPE_BUTTON);
	voltPrimaryBtnWidget.eWidgetType = WD_TYPE_BUTTON;
	voltPrimaryBtnWidget.pvWidget = (void *)&voltPrimaryBtnData;

	voltSecondaryBtnData = (gfx_Button) {
		.name = "voltBtn",
		.label = "SECUNDARIO",
		.pos.x = (LCD_WIDTH / 2 - btnWidth),
		.pos.y = voltPrimaryBtnData.pos.y + btnHeight + 20,
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
	gfx_initRegTouch((void *)&voltSecondaryBtnData, WD_TYPE_BUTTON);
	voltSecondaryBtnWidget.eWidgetType = WD_TYPE_BUTTON;
	voltSecondaryBtnWidget.pvWidget = (void *)&voltSecondaryBtnData;

	voltPrimaryFrameData = (gfx_Rectangle) {
		.name = "voltPrimaryFrame",
		.pos.x = voltPrimaryBtnData.pos.x + voltPrimaryBtnData.size.width + 30,
		.pos.y = voltPrimaryBtnData.pos.y,
		.dim.height = btnHeight,
		.dim.width = btnWidth,
		.round = 6,
		.borderWidth = 3,
		.color = g_pCurrentTheme->palette.surface,
	};
	voltPrimaryFrameWidget.eWidgetType = WD_TYPE_RECT;
	voltPrimaryFrameWidget.pvWidget = (void *)&voltPrimaryFrameData;

	voltPrimaryLbData = (gfx_Label) {
		.name = "voltPrimaryLabel",
		.text = voltPrimaryBuff,
		.pos.x = voltPrimaryFrameData.pos.x + voltPrimaryFrameData.dim.width / 2,
		.pos.y = voltPrimaryFrameData.pos.y + voltPrimaryFrameData.dim.height / 2,
		.alignment = ALIGN_CENTER,
		.isVisible = true,
		.style = STYLE_TEXT_MAIN,
		.typo = TYPO_H3,
	};
	voltPrimaryLbWidget.eWidgetType = WD_TYPE_LABEL;
	voltPrimaryLbWidget.pvWidget = (void *)&voltPrimaryLbData;

	voltSecondaryFrameData = (gfx_Rectangle) {
		.name = "voltPrimaryFrame",
		.pos.x = voltPrimaryBtnData.pos.x + voltSecondaryBtnData.size.width + 30,
		.pos.y = voltSecondaryBtnData.pos.y,
		.dim.height = btnHeight,
		.dim.width = btnWidth,
		.round = 6,
		.borderWidth = 3,
		.color = g_pCurrentTheme->palette.surface,
	};
	voltSecondaryFrameWidget.eWidgetType = WD_TYPE_RECT;
	voltSecondaryFrameWidget.pvWidget = (void *)&voltSecondaryFrameData;

	voltSecondaryLbData = (gfx_Label) {
		.name = "voltPrimaryLabel",
		.text = voltSecondaryBuff,
		.pos.x = voltSecondaryFrameData.pos.x + voltSecondaryFrameData.dim.width / 2,
		.pos.y = voltSecondaryFrameData.pos.y + voltSecondaryFrameData.dim.height / 2,
		.alignment = ALIGN_CENTER,
		.isVisible = true,
		.style = STYLE_TEXT_MAIN,
		.typo = TYPO_H3,
	};
	voltSecondaryLbWidget.eWidgetType = WD_TYPE_LABEL;
	voltSecondaryLbWidget.pvWidget = (void *)&voltSecondaryLbData;

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

	useFullHeader(&g_sVoltageSelectionCanvas);
	useNavigationButtons(&g_sVoltageSelectionCanvas);
	canvasInsertAtTop(&g_sVoltageSelectionCanvas.psWidgets, &formTitleWidget);
	canvasInsertAtTop(&g_sVoltageSelectionCanvas.psWidgets, &formSubtitleWidget);
	canvasInsertAtTop(&g_sVoltageSelectionCanvas.psWidgets, &voltPrimaryBtnWidget);
	canvasInsertAtTop(&g_sVoltageSelectionCanvas.psWidgets, &voltSecondaryBtnWidget);
	canvasInsertAtTop(&g_sVoltageSelectionCanvas.psWidgets, &voltPrimaryFrameWidget);
	canvasInsertAtTop(&g_sVoltageSelectionCanvas.psWidgets, &voltSecondaryFrameWidget);
	canvasInsertAtTop(&g_sVoltageSelectionCanvas.psWidgets, &voltSecondaryLbWidget);
	canvasInsertAtTop(&g_sVoltageSelectionCanvas.psWidgets, &voltPrimaryLbWidget);
	canvasInsertAtTop(&g_sVoltageSelectionCanvas.psWidgets, &cancelBtnWidget);
	
	Event_Subscribe(EVT_SYS_SHOW_TEST_CONFIRMATION, (EventHandler_fn)onShowThisFormEvent);
	Event_Subscribe(EVT_CAN_INST_VOLTAGE_PRIMARY, (EventHandler_fn)onPrimaryTxVoltageValue);
	Event_Subscribe(EVT_CAN_INST_VOLTAGE_SECONDARY, (EventHandler_fn)onSecondaryTxVoltageValue);

	g_i16VoltageSelectionIndex = FormManager_AddForm(&g_sVoltageSelectionCanvas);
}

