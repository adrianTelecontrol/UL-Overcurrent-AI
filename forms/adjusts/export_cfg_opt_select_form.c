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
#include "cfg_manager.h"

#include "forms/common_widgets.h"
#include "forms/adjusts/adjust_numpad_form.h"

#include "export_cfg_opt_select_form.h"

int16_t g_i16ExportCfgSelectIndex = 0; 

gfx_Canvas g_sExportCfgSelectCanvas;

// Container
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget formSubtitleWidget;
static gfx_GenericWidget loadCfgBtnWidget, exportCfgBtnWidget;
static gfx_GenericWidget cancelBtnWidget;

// Widgets
static gfx_Label formTitleData;
static gfx_Label formSubtitleData;
static gfx_Button loadCfgBtnData, exportCfgBtnData;
static gfx_Button cancelBtnData;


// Callbacks
static void onCancelBtnRelased(gfx_Button *btn) {
	onGenericBtnRelease(btn);

	Event_Post(EVT_SYS_SHOW_OPTIONS_FORM, (EventParam_t){.ptr = NULL});
}

static void onLoadBtnReleased(gfx_Button *btn) {
	onGenericBtnRelease(btn);

	Event_Post(EVT_SYS_SHOW_ADJ_CFG_BROWSER, (EventParam_t){.ui32 = NULL});
}

static void onExportBtnReleased(gfx_Button *btn) {
	onGenericBtnRelease(btn);

	// Here we should start the exporting commands
	CfgManager_RequestConfigFromInst();
	Event_Post(EVT_SYS_SHOW_ADJ_EXPORT_CFG, (EventParam_t){.ui32 = NULL});
}

void initExportCfgOptSelectForm(void) {
	g_sExportCfgSelectCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
	
	formTitleData = (gfx_Label) {
		.name = "formTitleData",
		.text = "LOAD/EXPORT CFG",
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
		.text = "SELECCIONE UNA OPCION DE CONFIGURACION",
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

	loadCfgBtnData = (gfx_Button) {
		.name = "tempBtn",
		.label = "CARGAR",
		.pos.x = (LCD_WIDTH / 2 - btnWidth) / 2.0,
		.pos.y = LCD_HEIGHT / 2 - 30,
		.size.width = btnWidth,
		.size.height = btnHeight,
		.borderWidth = 1,
		.radius = 5,
		.state = BTN_STATE_NORMAL,
		.style = STYLE_SECONDARY,
		.typo = TYPO_H3,
		.bIsVisible = true, 
		.onPressed = onGenericBtnPressed,
		.onRelease = onLoadBtnReleased,
	};
	gfx_initRegTouch((void *)&loadCfgBtnData, WD_TYPE_BUTTON);
	loadCfgBtnWidget.eWidgetType = WD_TYPE_BUTTON;
	loadCfgBtnWidget.pvWidget = (void *)&loadCfgBtnData;

	exportCfgBtnData = (gfx_Button) {
		.name = "voltBtn",
		.label = "EXPORTAR",
		.pos.x = LCD_WIDTH / 2.0f + (LCD_WIDTH / 2 - btnWidth) / 2.0,
		.pos.y = LCD_HEIGHT / 2 - 30,
		.size.width = btnWidth,
		.size.height = btnHeight,
		.borderWidth = 1,
		.radius = 5,
		.state = BTN_STATE_NORMAL,
		.style = STYLE_DEFAULT,
		.typo = TYPO_H3,
		.bIsVisible = true, 
		.onPressed = onGenericBtnPressed,
		.onRelease = onExportBtnReleased,
	};
	gfx_initRegTouch((void *)&exportCfgBtnData, WD_TYPE_BUTTON);
	exportCfgBtnWidget.eWidgetType = WD_TYPE_BUTTON;
	exportCfgBtnWidget.pvWidget = (void *)&exportCfgBtnData;

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

	useFullHeader(&g_sExportCfgSelectCanvas);
	useNavigationButtons(&g_sExportCfgSelectCanvas);
	canvasInsertAtTop(&g_sExportCfgSelectCanvas.psWidgets, &formTitleWidget);
	canvasInsertAtTop(&g_sExportCfgSelectCanvas.psWidgets, &formSubtitleWidget);
	canvasInsertAtTop(&g_sExportCfgSelectCanvas.psWidgets, &loadCfgBtnWidget);
	canvasInsertAtTop(&g_sExportCfgSelectCanvas.psWidgets, &exportCfgBtnWidget);
	canvasInsertAtTop(&g_sExportCfgSelectCanvas.psWidgets, &cancelBtnWidget);

	g_i16ExportCfgSelectIndex = FormManager_AddForm(&g_sExportCfgSelectCanvas);
}

