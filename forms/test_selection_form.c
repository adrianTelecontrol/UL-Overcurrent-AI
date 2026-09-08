
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "gui_core.h"
#include "gui_theme.h"
#include "gui_canvas.h"
#include "forms_manager.h"
#include "FT8xx_params.h"
#include "event_engine.h"
#include "experiments_cfg.h"

#include "common_widgets.h"

#include "test_selection_form.h"

// Form canvas
static gfx_Canvas g_sFormSelectionCanvas;
int16_t g_i16FormSelectionID = 0;

// Widgets containers
static gfx_GenericWidget titleWidget;
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget faultCurrentBtnWidget;
static gfx_GenericWidget crushBtnWidget;
static gfx_GenericWidget patternBtnWidget;
static gfx_GenericWidget faultTestNameWidget;
static gfx_GenericWidget crushTestNameWidget;
static gfx_GenericWidget patternTestNameWidget;

static gfx_GenericWidget faultSubtitleWidget;
static gfx_GenericWidget crushSubtitleWidget;
static gfx_GenericWidget patternSubtitleWidget;


// Widgets data
static gfx_Label titleData;
static gfx_Label formTitleData;
static gfx_Button faultCurrentBtnData;
static gfx_Button crushBtnData;
static gfx_Button patternBtnData;
static gfx_Label faultTestNameData;
static gfx_Label crushTestNameData;
static gfx_Label patternTestNameData;
static gfx_Label faultSubtitleData;
static gfx_Label crushSubtitleData;
static gfx_Label patternSubtitleData;

// Callbacks
static void onFaultCurrentBtnReleased(gfx_Button *btn) {
	onGenericBtnRelease(btn);
	ExperimentCfg_newFaultTest(UL_FAULT_DEFAULT_CURRENT, UL_FAULT_DEFAULT_DURATION_SEC, UL_FAULT_DEFAULT_PRESET_VOLTAGE);

	Event_Post(EVT_SYS_FAULT_CFG_CURRENT, (EventParam_t){.f32 = UL_FAULT_DEFAULT_CURRENT});
	Event_Post(EVT_SYS_FAULT_CFG_DURATION, (EventParam_t){.f32 = UL_FAULT_DEFAULT_DURATION_SEC});
	Event_Post(EVT_SYS_FAULT_CFG_PRESET_VOLTAGE, (EventParam_t){.f32 = UL_FAULT_DEFAULT_PRESET_VOLTAGE});
	Event_Post(EVT_SYS_SHOW_FAULT_CONFIG_FORM, (EventParam_t){.ptr = NULL});
}

static void onCrushBtnReleased(gfx_Button *btn) {
	onGenericBtnRelease(btn);
	
	bool ret = ExperimentCfg_newCrushTest(UL_CRUSH_DEFAULT_TEMP, UL_CRUSH_DEFAULT_DURATION_SEC);
	if(!ret) return;
	
	Event_Post(EVT_SYS_CRUSH_CFG_DURATION, (EventParam_t){.f32 = UL_CRUSH_DEFAULT_DURATION_SEC});
	Event_Post(EVT_SYS_CRUSH_CFG_TEMP, (EventParam_t){.f32 = UL_CRUSH_DEFAULT_TEMP});
	Event_Post(EVT_SYS_SHOW_CRUSH_CONFIG_FORM, (EventParam_t){.ptr = NULL});
}

static void onPatternBtnReleased(gfx_Button *btn) {
	onGenericBtnRelease(btn);

	Event_Post(EVT_SYS_SHOW_FILE_BROWSER, (EventParam_t){.ptr = NULL});
}


void initTestSelectionForm(void) {
	g_sFormSelectionCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;

    titleData = (gfx_Label){
        .text = "PRUEBAS",
        .name = "sysTitle",
        .pos.x = 110,
        .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
    };
    titleWidget.eWidgetType = WD_TYPE_LABEL; titleWidget.pvWidget = (void *)&titleData;

	formTitleData = (gfx_Label){
		.name = "formTitle",
		.text = "Seleccione el modo de ensayo",
		.pos.x = 20,
		.pos.y = 100,
		.typo = TYPO_BODY,
		.style = STYLE_SECONDARY,
		.isVisible = true,
		.alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER),
	};
	formTitleWidget.eWidgetType = WD_TYPE_LABEL;
	formTitleWidget.pvWidget = (void *)&formTitleData;

	uint16_t btnWidth = (LCD_WIDTH / 3.0);
	uint16_t btnHeight = (LCD_HEIGHT - formTitleData.pos.y - 140) / 3.0;
	faultCurrentBtnData = (gfx_Button) {
		.name = "faultCurrentBtn",
		.label = "MODO A: FAULT",
		.pos.x = 10,
		.pos.y = formTitleData.pos.y + 30, 
		.size.height = btnHeight,
		.size.width = btnWidth,
		.bIsVisible = true,
		.borderWidth = 1,
		.radius = 4,
		.typo = TYPO_BODY,
		.style = STYLE_DANGER,
		.state = BTN_STATE_NORMAL,
		.onPressed = onGenericBtnPressed,
		.onRelease = onFaultCurrentBtnReleased,
	};
	gfx_initRegTouch((void *)&faultCurrentBtnData, WD_TYPE_BUTTON);
	faultCurrentBtnWidget.eWidgetType = WD_TYPE_BUTTON;
	faultCurrentBtnWidget.pvWidget = (void *)&faultCurrentBtnData;

	faultTestNameData = (gfx_Label){
		.name = "faultTestData",
		.text = "PRUEBA DE CORTOCIRCUITO",
		.pos.x = faultCurrentBtnData.pos.x + faultCurrentBtnData.size.width + 15,
		.pos.y = faultCurrentBtnData.pos.y,
		.style = STYLE_TEXT_MAIN,
		.typo = TYPO_MONO_BOLD,
		.alignment = ( gfx_Align_e )( ALIGN_LEFT | ALIGN_TOP ),
		.isVisible = true,
	};
	faultTestNameWidget.eWidgetType = WD_TYPE_LABEL;
	faultTestNameWidget.pvWidget = (void *)&faultTestNameData;

	faultSubtitleData = (gfx_Label){
		.name = "faultSubtitle",
		.text = "Pulso exacto de alta corriente (4s@450A).",
		.pos.x = faultCurrentBtnData.pos.x + faultCurrentBtnData.size.width + 15,
		.pos.y = faultCurrentBtnData.pos.y + 30,
		.style = STYLE_TEXT_MUTED,
		.typo = TYPO_CAPTION,
		.alignment = ( gfx_Align_e )( ALIGN_LEFT | ALIGN_TOP ),
		.isVisible = true,
	};
	faultSubtitleWidget.eWidgetType = WD_TYPE_LABEL;
	faultSubtitleWidget.pvWidget = (void *)&faultSubtitleData;

	crushBtnData = (gfx_Button) {
		.name = "crushBtn",
		.label = "MODO B: CRUSH",
		.pos.x = 10, 
		.pos.y = faultCurrentBtnData.pos.y + faultCurrentBtnData.size.height + 10, 
		.size.height = btnHeight,
		.size.width = btnWidth,
		.bIsVisible = true,
		.borderWidth = 1,
		.radius = 4,
		.typo = TYPO_BODY,
		.style = STYLE_SUCCESS,
		.state = BTN_STATE_NORMAL,
		.onPressed = onGenericBtnPressed,
		.onRelease = onCrushBtnReleased,
	};
	gfx_initRegTouch((void *)&crushBtnData, WD_TYPE_BUTTON);
	crushBtnWidget.eWidgetType = WD_TYPE_BUTTON;
	crushBtnWidget.pvWidget = (void *)&crushBtnData;

	crushTestNameData = (gfx_Label){
		.name = "crushTestData",
		.text = "OVERCURRENT WITH CRUSH",
		.pos.x = crushBtnData.pos.x + crushBtnData.size.width + 15,
		.pos.y = crushBtnData.pos.y,
		.style = STYLE_TEXT_MAIN,
		.typo = TYPO_MONO_BOLD,
		.alignment = (gfx_Align_e)( ALIGN_LEFT | ALIGN_TOP ),
		.isVisible = true,
	};
	crushTestNameWidget.eWidgetType = WD_TYPE_LABEL;
	crushTestNameWidget.pvWidget = (void *)&crushTestNameData;

	crushSubtitleData = (gfx_Label){
		.name = "crushSubtitle",
		.text = "Control termico en lazo cerrado para mantener una\ntemperatura objetivo.",
		.pos.x = crushBtnData.pos.x + crushBtnData.size.width + 15,
		.pos.y = crushBtnData.pos.y + 30,
		.style = STYLE_TEXT_MUTED,
		.typo = TYPO_CAPTION,
		.alignment = ( gfx_Align_e )( ALIGN_LEFT | ALIGN_TOP ),
		.isVisible = true,
	};
	crushSubtitleWidget.eWidgetType = WD_TYPE_LABEL;
	crushSubtitleWidget.pvWidget = (void *)&crushSubtitleData;

	patternBtnData = (gfx_Button) {
		.name = "patternBtn",
		.label = "MODO C: PATTERN",
		.pos.x = 10,
		.pos.y = crushBtnData.pos.y + crushBtnData.size.height + 10, 
		.size.height = btnHeight,
		.size.width = btnWidth,
		.bIsVisible = true,
		.borderWidth = 1,
		.radius = 4,
		.typo = TYPO_BODY,
		.style = STYLE_SECONDARY,
		.state = BTN_STATE_NORMAL,
		.onPressed = onGenericBtnPressed,
		.onRelease = onPatternBtnReleased,
	};
	gfx_initRegTouch((void *)&patternBtnData, WD_TYPE_BUTTON);
	patternBtnWidget.eWidgetType = WD_TYPE_BUTTON;
	patternBtnWidget.pvWidget = (void *)&patternBtnData;

	patternTestNameData = (gfx_Label){
		.name = "patternNameData",
		.text = "OVERCURRENT (STEP)",
		.pos.x = patternBtnData.pos.x + patternBtnData.size.width + 10,
		.pos.y = patternBtnData.pos.y,
		.style = STYLE_TEXT_MAIN,
		.typo = TYPO_MONO_BOLD,
		.alignment = ( gfx_Align_e )( ALIGN_LEFT | ALIGN_TOP ),
		.isVisible = true,
	};
	patternTestNameWidget.eWidgetType = WD_TYPE_LABEL;
	patternTestNameWidget.pvWidget = (void *)&patternTestNameData;

	patternSubtitleData = (gfx_Label){
		.name = "patternSubtitle",
		.text = "Ejecucion de secuencias: rampas y escalones de\ncorriente definidos por perfiles.",
		.pos.x = patternBtnData.pos.x + patternBtnData.size.width + 10,
		.pos.y = patternBtnData.pos.y + 30,
		.style = STYLE_TEXT_MUTED,
		.typo = TYPO_CAPTION,
		.alignment = ( gfx_Align_e )( ALIGN_LEFT | ALIGN_TOP ),
		.isVisible = true,
	};
	patternSubtitleWidget.eWidgetType = WD_TYPE_LABEL;
	patternSubtitleWidget.pvWidget = (void *)&patternSubtitleData;

	useNavigationButtons(&g_sFormSelectionCanvas);
	useFullHeader(&g_sFormSelectionCanvas);

	canvasInsertAtTop(&g_sFormSelectionCanvas.psWidgets, &titleWidget);
	canvasInsertAtTop(&g_sFormSelectionCanvas.psWidgets, &formTitleWidget);
	canvasInsertAtTop(&g_sFormSelectionCanvas.psWidgets, &faultCurrentBtnWidget);
	canvasInsertAtTop(&g_sFormSelectionCanvas.psWidgets, &crushBtnWidget);
	canvasInsertAtTop(&g_sFormSelectionCanvas.psWidgets, &patternBtnWidget);
	canvasInsertAtTop(&g_sFormSelectionCanvas.psWidgets, &faultTestNameWidget);
	canvasInsertAtTop(&g_sFormSelectionCanvas.psWidgets, &crushTestNameWidget);
	canvasInsertAtTop(&g_sFormSelectionCanvas.psWidgets, &patternTestNameWidget);
	canvasInsertAtTop(&g_sFormSelectionCanvas.psWidgets, &faultSubtitleWidget);
	canvasInsertAtTop(&g_sFormSelectionCanvas.psWidgets, &crushSubtitleWidget);
	canvasInsertAtTop(&g_sFormSelectionCanvas.psWidgets, &patternSubtitleWidget);

	g_i16FormSelectionID = FormManager_AddForm(&g_sFormSelectionCanvas);	
}


