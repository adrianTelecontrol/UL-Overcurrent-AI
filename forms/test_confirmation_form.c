
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

#include "common_widgets.h"

#include "test_confirmation_form.h"

int16_t g_i16TestConfirmationIndex = 0; 

gfx_Canvas g_sTestConfirmationCanvas;

// Container
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget formSubtitleWidget;
static gfx_GenericWidget formLegendWidget;
static gfx_GenericWidget variableParamFrameWidget;
static gfx_GenericWidget durationFrameWidget;
static gfx_GenericWidget variableParamLabelWidget;
static gfx_GenericWidget durationLabelWidget;
static gfx_GenericWidget durationValueWidget;
static gfx_GenericWidget variableParamValueWidget;
static gfx_GenericWidget returnButtonWidget;
static gfx_GenericWidget confirmButtonWidget;

// Widgets
static gfx_Label formTitleData;
static gfx_Label formSubtitleData;
static gfx_Label formLegendData;
static gfx_Rectangle variableParamFrameData;
static gfx_Rectangle durationFrameData;
static gfx_Label durationLabelData;
static gfx_Label variableParamLabelData;
static gfx_Label durationValueData;
static gfx_Label variableParamValueData;
static gfx_Button returnButtonData;
static gfx_Button confirmButtonData;

// Buffers
static char durationBuf[7] = "4.0s";
static char variableParamBuf[9] = "450.0A";
static char variableParamLabelBuf[20] = "Temperatura [C]";

static EventID_e g_eFormCallback = EVT_SYS_NULL;

// Callbacks
static void onShowThisFormEvent(EventParam_t arg) {
	ul_crush_test_s crush;
	if(arg.ui32 == UL_TEST_CRUSH) {
		strcpy(variableParamLabelBuf, "TEMPERATURA [C]");
		crush = ExperimentCfg_getCurrCrushCfg();
		snprintf(durationBuf, sizeof(durationBuf), "%u", crush.ui16Duration);
		snprintf(variableParamBuf, sizeof(variableParamBuf), "%.1f", crush.f32TargetTemp);
		durationValueData.bIsDirty = true;
		variableParamValueData.bIsDirty = true;

		g_eFormCallback = EVT_SYS_SHOW_CRUSH_CONFIG_FORM;
	}
	else if(arg.ui32 == UL_TEST_FAULT) {
		strcpy(variableParamLabelBuf, "CORRIENTE [A]");
		ul_fault_current_test_s fault= ExperimentCfg_getCurrFaultCfg();
		snprintf(durationBuf, sizeof(durationBuf), "%u", fault.ui16Duration);
		snprintf(variableParamBuf, sizeof(variableParamBuf), "%.1f", fault.f32TargetCurrent);
		durationValueData.bIsDirty = true;
		variableParamValueData.bIsDirty = true;

		g_eFormCallback = EVT_SYS_SHOW_FAULT_CONFIG_FORM;
	} else if(arg.ui32 == UL_TEST_SEQUENCE) {
		strcpy(variableParamLabelBuf, "CORRIENTE MAX [A]");
		const ul_sequence_profile_t *profile = ExperimentCfg_getCurrSequenceCfg();
		snprintf(durationBuf, sizeof(durationBuf), "%u", profile->totalDurationSec);
		snprintf(variableParamBuf, sizeof(variableParamBuf), "%.1f", profile->maxCurrentRequested);
		durationValueData.bIsDirty = true;
		variableParamValueData.bIsDirty = true;

		g_eFormCallback = EVT_SYS_SHOW_SEQUENCE_CONFIG_FORM;
	}

}

static void onReturnButtonReleased(gfx_Button *btn) {
	onGenericBtnRelease(btn);

	Event_Post(g_eFormCallback, (EventParam_t){.ptr = NULL});
}

static void onConfirmButtonRelease(gfx_Button *btn) {
	onGenericBtnRelease(btn);
	uint32_t testType = 0;
	if(g_eFormCallback == EVT_SYS_SHOW_FAULT_CONFIG_FORM) {
		Event_Post(EVT_SYS_SHOW_TEST_RUNNING, (EventParam_t){.ui32 = UL_TEST_FAULT});
		testType = UL_TEST_FAULT;
	}
	else if(g_eFormCallback == EVT_SYS_SHOW_CRUSH_CONFIG_FORM) {
		Event_Post(EVT_SYS_SHOW_TEST_RUNNING, (EventParam_t){.ui32 = UL_TEST_CRUSH});
		testType = UL_TEST_CRUSH;
	}
	else if(g_eFormCallback == EVT_SYS_SHOW_SEQUENCE_CONFIG_FORM) {
		Event_Post(EVT_SYS_SHOW_TEST_RUNNING, (EventParam_t){.ui32 = UL_TEST_SEQUENCE});
		testType = UL_TEST_SEQUENCE;
	}

	Event_Post(EVT_SYS_START_TEST, (EventParam_t){.ui32 = testType});
}

void initTestConfirmationForm(void) {
	g_sTestConfirmationCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
	
	formTitleData = (gfx_Label) {
		.name = "formTitleData",
		.text = "PRUEBAS",
        .pos.x = 125,
        .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
	};
	formTitleWidget.eWidgetType = WD_TYPE_LABEL;
	formTitleWidget.pvWidget = (void *)&formTitleData;

	formSubtitleData = (gfx_Label){
		.text = "CONFIRMAR PARAMETROS",
		.pos.x = LCD_WIDTH / 2,
		.pos.y = 110,
		.alignment = ALIGN_CENTER,
		.style = STYLE_DANGER,
		.typo = TYPO_H1,
		.isVisible = true,
	};
	formSubtitleWidget.eWidgetType = WD_TYPE_LABEL;
	formSubtitleWidget.pvWidget = (void *)&formSubtitleData;

	formLegendData = (gfx_Label){
		.text = "Atencion: Se inyectara alta corriente en el circuito.\nVerifique que los valores sean correctos y el area\neste despejada.",
		.pos.x = LCD_WIDTH / 2,
		.pos.y = 170,
		.alignment = ALIGN_CENTER,
		.style = STYLE_TEXT_MUTED,
		.typo = TYPO_CAPTION,
		.isVisible = true,
	};
	formLegendWidget.eWidgetType = WD_TYPE_LABEL;
	formLegendWidget.pvWidget = (void *)&formLegendData;

	uint16_t frameWidth = (LCD_WIDTH / 3.5f);
	variableParamFrameData = (gfx_Rectangle){
		.pos.x = (LCD_WIDTH / 2.0) - frameWidth - 20,
		.pos.y = 230,
		.dim.width = frameWidth,
		.dim.height = 130,
		.round = 5,
		.color = g_pCurrentTheme->palette.background,
		.borderWidth = 3,
	};
	variableParamFrameWidget.eWidgetType = WD_TYPE_RECT;
	variableParamFrameWidget.pvWidget = (void *)&variableParamFrameData;
	
	durationFrameData = (gfx_Rectangle){
		.pos.x = (LCD_WIDTH / 2.0) + 20,
		.pos.y = 230,
		.dim.width = frameWidth,
		.dim.height = 130,
		.round = 5,
		.color = g_pCurrentTheme->palette.background,
		.borderWidth = 3,
	};
	durationFrameWidget.eWidgetType = WD_TYPE_RECT;
	durationFrameWidget.pvWidget = (void *)&durationFrameData;

	variableParamLabelData = (gfx_Label){
		.text = variableParamLabelBuf,
		.pos.x = variableParamFrameData.pos.x + frameWidth / 2, 
		.pos.y = variableParamFrameData.pos.y + 130 / 4.0,
		.alignment = ALIGN_CENTER,
		.style = STYLE_TEXT_MUTED,
		.typo = TYPO_CAPTION,
		.isVisible = true,
	};
	variableParamLabelWidget.eWidgetType = WD_TYPE_LABEL;
	variableParamLabelWidget.pvWidget = (void *)&variableParamLabelData;

	variableParamValueData = (gfx_Label){
		.text = variableParamBuf,
		.pos.x = variableParamFrameData.pos.x + frameWidth / 2, 
		.pos.y = variableParamFrameData.pos.y + 130 / 2.0,
		.alignment = ALIGN_CENTER,
		.style = STYLE_PRIMARY,
		.typo = TYPO_H1,
		.isVisible = true,
	};
	variableParamValueWidget.eWidgetType = WD_TYPE_LABEL;
	variableParamValueWidget.pvWidget = (void *)&variableParamValueData;

	durationLabelData = (gfx_Label){
		.text = "DURACION [s]",
		.pos.x = durationFrameData.pos.x + frameWidth / 2, 
		.pos.y = durationFrameData.pos.y + 130 / 4.0,
		.alignment = ALIGN_CENTER,
		.style = STYLE_TEXT_MUTED,
		.typo = TYPO_CAPTION,
		.isVisible = true,
	};
	durationLabelWidget.eWidgetType = WD_TYPE_LABEL;
	durationLabelWidget.pvWidget = (void *)&durationLabelData;

	durationValueData = (gfx_Label){
		.text = durationBuf,
		.pos.x = durationFrameData.pos.x + frameWidth / 2, 
		.pos.y = durationFrameData.pos.y + 130 / 2.0,
		.alignment = ALIGN_CENTER,
		.style = STYLE_SECONDARY,
		.typo = TYPO_H1,
		.isVisible = true,
	};
	durationValueWidget.eWidgetType = WD_TYPE_LABEL;
	durationValueWidget.pvWidget = (void *)&durationValueData;

	confirmButtonData= (gfx_Button){
		.label = "EJECUTAR",
		.size.width = LCD_WIDTH / 4.0,
		.size.height = 75,
		.pos.x = LCD_WIDTH * ( 3.0 / 4.0 ) - 30,
		.pos.y = 390,
		.borderWidth = 2,
		.bIsVisible = true,
		.radius = 3,
		.typo = TYPO_H3,
		.style = STYLE_DANGER,
		.state = BTN_STATE_NORMAL,
		.onPressed = onGenericBtnPressed,
		.onRelease = onConfirmButtonRelease,
	};
	gfx_initRegTouch((void *)&confirmButtonData, WD_TYPE_BUTTON);
	confirmButtonWidget.eWidgetType = WD_TYPE_BUTTON;
	confirmButtonWidget.pvWidget = (void *)&confirmButtonData;

	returnButtonData = (gfx_Button){
		.label = "VOLVER",
		.size.width = LCD_WIDTH / 4.0,
		.size.height = 75,
		.pos.x = LCD_WIDTH * ( 3.0 / 8.0 ),
		.pos.y = 390,
		.borderWidth = 2,
		.bIsVisible = true,
		.radius = 3,
		.typo = TYPO_H3,
		.style = STYLE_DEFAULT,
		.state = BTN_STATE_NORMAL,
		.onPressed = onGenericBtnPressed,
		.onRelease = onReturnButtonReleased,
	};
	gfx_initRegTouch((void *)&returnButtonData, WD_TYPE_BUTTON);
	returnButtonWidget.eWidgetType = WD_TYPE_BUTTON;
	returnButtonWidget.pvWidget = (void *)&returnButtonData;

	useFullHeader(&g_sTestConfirmationCanvas);
	canvasInsertAtTop(&g_sTestConfirmationCanvas.psWidgets, &formTitleWidget);
	canvasInsertAtTop(&g_sTestConfirmationCanvas.psWidgets, &formSubtitleWidget);
	canvasInsertAtTop(&g_sTestConfirmationCanvas.psWidgets, &formLegendWidget);
	canvasInsertAtTop(&g_sTestConfirmationCanvas.psWidgets, &variableParamFrameWidget);
	canvasInsertAtTop(&g_sTestConfirmationCanvas.psWidgets, &durationFrameWidget);
	canvasInsertAtTop(&g_sTestConfirmationCanvas.psWidgets, &variableParamLabelWidget);
	canvasInsertAtTop(&g_sTestConfirmationCanvas.psWidgets, &durationLabelWidget);
	canvasInsertAtTop(&g_sTestConfirmationCanvas.psWidgets, &durationValueWidget);
	canvasInsertAtTop(&g_sTestConfirmationCanvas.psWidgets, &variableParamValueWidget);
	canvasInsertAtTop(&g_sTestConfirmationCanvas.psWidgets, &returnButtonWidget);
	canvasInsertAtTop(&g_sTestConfirmationCanvas.psWidgets, &confirmButtonWidget);

	
	Event_Subscribe(EVT_SYS_SHOW_TEST_CONFIRMATION, (EventHandler_fn)onShowThisFormEvent);

	g_i16TestConfirmationIndex = FormManager_AddForm(&g_sTestConfirmationCanvas);
}

