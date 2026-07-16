#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "gui_core.h"
#include "gui_theme.h"
#include "gui_canvas.h"
#include "forms_manager.h"
#include "font_engine.h"
#include "FT8xx_params.h"
#include "gui_colors.h"
#include "icon_map.h"
#include "event_engine.h"
#include "experiments_cfg.h"
#include "helpers.h"

#include "common_widgets.h"

#include "test_finished_form.h"

int16_t g_i16TestFinishedFormIndex;

gfx_Canvas g_sTestFinishedCanvas;

// Containers
static gfx_GenericWidget resultIconWidget;
static gfx_GenericWidget resultLabelWidget;
static gfx_GenericWidget savePathLabelWidget;
static gfx_GenericWidget continueButtonWidget;
static gfx_GenericWidget formTitleWidget;

// Widgets
static gfx_Label formTitleData;
static gfx_Label resultIconData;
static gfx_Label resultLabelData;
static gfx_Label savePathLabelData;
static gfx_Button continueButtonData;

// Buffers
static char resultBuf[30] = "ENSAYO COMPLETADO";
static char pathBuf[] = "Log guardado en\nUSB:/UL_OVERCURRENT/LOG/28_05_26/05_49_26.csv";

// Callbacks
static void onContinueButtonRelease(gfx_Button *btn) {
	onGenericBtnRelease(btn);

	Event_Post(EVT_SYS_SHOW_HOME_FORM, (EventParam_t){.ptr = NULL});
}

static void onLogNameChanged(EventParam_t arg) {
	snprintf(pathBuf, sizeof(pathBuf), "LOG GUARDADO EN \n%s", arg.str);
	savePathLabelData.bIsDirty = true;
}

static void onTestEmergencyStopEvent(EventParam_t arg) {
	strcpy(resultBuf, "PARO DE EMERGENCIA");
	resultIconData.text = ICON_WARNING;
	resultLabelData.bIsDirty = true;
}

void initTestFinishedForm(void) {
	g_sTestFinishedCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;

	formTitleData = (gfx_Label) {
		.name = "formTitleData",
		.text = "PRUEBAS",
        .pos.x = 110,
        .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
	};
	formTitleWidget.eWidgetType = WD_TYPE_LABEL;
	formTitleWidget.pvWidget = (void *)&formTitleData;

	resultIconData = (gfx_Label){
		.name = "iconLabel",
		.text = ICON_CHECK_CIRCLE,
		.pos.x = LCD_WIDTH / 2,
		.pos.y = 120,
		.alignment = ALIGN_CENTER,
		.isVisible = true,
		.typo = TYPO_ICON,
		.style = STYLE_SUCCESS,
	};
	resultIconWidget.eWidgetType = WD_TYPE_LABEL;
	resultIconWidget.pvWidget = (void *)&resultIconData;

	resultLabelData = (gfx_Label) {
		.name = "resultLabel",
		.text = resultBuf,
		.pos.x = LCD_WIDTH / 2,
		.pos.y = resultIconData.pos.y + 80,
		.alignment = ALIGN_CENTER,
		.isVisible = true,
		.typo = TYPO_H1,
		.style = STYLE_TEXT_MAIN,
	};
	resultLabelWidget.eWidgetType = WD_TYPE_LABEL;
	resultLabelWidget.pvWidget = (void *)&resultLabelData;

	savePathLabelData = (gfx_Label) {
		.name = "savePathLabel",
		.text = pathBuf,
		.pos.x = LCD_WIDTH / 2,
		.pos.y = resultLabelData.pos.y + 80,
		.alignment = ALIGN_CENTER,
		.isVisible = true,
		.typo = TYPO_BODY,
		.style = STYLE_TEXT_MUTED,
	};
	savePathLabelWidget.eWidgetType = WD_TYPE_LABEL;
	savePathLabelWidget.pvWidget = (void *)&savePathLabelData;
	
	continueButtonData = (gfx_Button) {
		.name = "continueButton",
		.label = "CERRAR",
		.size.width = LCD_WIDTH / 3.5,
		.size.height = 90,
		.pos.x = LCD_WIDTH / 2 - LCD_WIDTH / 8.0,
		.pos.y = 360,
		.borderWidth = 2,
		.radius = 4,
		.state = BTN_STATE_NORMAL,
		.typo = TYPO_H2,
		.style = STYLE_DANGER,
		.bIsVisible = true,
		.onPressed = onGenericBtnPressed,
		.onRelease = onContinueButtonRelease,
	};
	gfx_initRegTouch((void *)&continueButtonData, WD_TYPE_BUTTON);
	continueButtonWidget.eWidgetType = WD_TYPE_BUTTON;
	continueButtonWidget.pvWidget = (void *)&continueButtonData;
	
	useFullHeader(&g_sTestFinishedCanvas);

	canvasInsertAtTop(&g_sTestFinishedCanvas.psWidgets, &formTitleWidget);
	canvasInsertAtTop(&g_sTestFinishedCanvas.psWidgets, &resultIconWidget);
	canvasInsertAtTop(&g_sTestFinishedCanvas.psWidgets, &resultLabelWidget);
	canvasInsertAtTop(&g_sTestFinishedCanvas.psWidgets, &savePathLabelWidget);
	canvasInsertAtTop(&g_sTestFinishedCanvas.psWidgets, &continueButtonWidget);

	Event_Subscribe(EVT_SYS_TEST_SAVED_LOG_NAME, (EventHandler_fn)onLogNameChanged);
	Event_Subscribe(EVT_SYS_TEST_EMERGENCY_STOPPED, (EventHandler_fn)onTestEmergencyStopEvent);

	g_i16TestFinishedFormIndex = FormManager_AddForm(&g_sTestFinishedCanvas);
}