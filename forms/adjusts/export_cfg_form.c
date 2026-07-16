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

#include "export_cfg_form.h"

int16_t g_i16ExportCfgIndex = 0; 

gfx_Canvas g_sExportCfgCanvas;

// Container
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget formSubtitleWidget, cfgSavePathWidget;
static gfx_GenericWidget returnBtnWidget;

// Widgets
static gfx_Label formTitleData;
static gfx_Label formSubtitleData, cfgSavePathData;
static gfx_Button returnBtnData;

// Callbacks
static void onReturnBtnReleased(gfx_Button *btn) {
	onGenericBtnRelease(btn);

	Event_Post(EVT_SYS_SHOW_OPTIONS_FORM, (EventParam_t){.ptr = NULL});
}

// ========================================================
// CALLBACKS DE ESTADO (Vienen del Backend CAN)
// ========================================================

// Evento: Al entrar a la pantalla (Inicia el proceso)
static void onShowExportCfgForm(EventParam_t arg) {
    // Resetear la UI a estado de "Cargando"
    formSubtitleData.text = "EXPORTANDO CFG.\nESPERE UN MOMENTO...";
    formSubtitleData.style = STYLE_SECONDARY;
    formSubtitleData.bIsDirty = true;
    
    cfgSavePathData.isVisible = false;
    cfgSavePathData.bIsDirty = true;
    
    returnBtnData.bIsVisible = false; // Ocultar el botón hasta que termine
    returnBtnData.bIsDirty = true;
}

// Evento: Recepción exitosa
static void onExportSuccess(EventParam_t arg) {
    char *filePath = (char *)arg.ptr;

    formSubtitleData.text = "CONFIG EXPORTADA\nEXITOSAMENTE";
    formSubtitleData.style = STYLE_SUCCESS;
    formSubtitleData.bIsDirty = true;
    
    // Mostrar la ruta real donde se guardó
    static char pathBuf[64];
    snprintf(pathBuf, sizeof(pathBuf), "Guardado en: %s", filePath);
    cfgSavePathData.text = pathBuf;
    cfgSavePathData.style = STYLE_SUCCESS;
    cfgSavePathData.isVisible = true;
    cfgSavePathData.bIsDirty = true;
    
    returnBtnData.bIsVisible = true;
    returnBtnData.bIsDirty = true;
}

// Evento: Fallo en la recepción o Timeout
static void onExportError(EventParam_t arg) {
    formSubtitleData.text = "ERROR DE EXPORTACION";
    formSubtitleData.style = STYLE_DANGER;
    formSubtitleData.bIsDirty = true;
    
    cfgSavePathData.text = "Fallo de comunicacion o archivo corrupto.";
    cfgSavePathData.style = STYLE_DANGER;
    cfgSavePathData.isVisible = true;
    cfgSavePathData.bIsDirty = true;
    
    returnBtnData.bIsVisible = true;
    returnBtnData.bIsDirty = true;
}


void initExportCgfForm(void) {
	g_sExportCfgCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
	
	formTitleData = (gfx_Label) {
		.name = "formTitleData",
		.text = "EXPORT CFG",
        .pos.x = 110,
        .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
	};
	formTitleWidget.eWidgetType = WD_TYPE_LABEL;
	formTitleWidget.pvWidget = (void *)&formTitleData;

	uint16_t strWidth, strHeight;
	FontEngine_GetStringDimensions("EXPORTANDO CFG.\nESPERE UN MOMENTO...", Theme_ResolveFontId(TYPO_H1), &strWidth, &strHeight, 1);
	formSubtitleData = (gfx_Label){
		.text = "EXPORTANDO CFG.\nESPERE UN MOMENTO...",
		.pos.x = LCD_WIDTH / 2,
		.pos.y = LCD_HEIGHT / 2 - 40,
		.alignment = ALIGN_CENTER,
		.style = STYLE_SECONDARY,
		.typo = TYPO_H1,
		.oldSize.height = strHeight,
		.oldSize.width = strWidth,
		.oldPos.x = LCD_WIDTH / 2,
		.oldPos.y = LCD_HEIGHT / 2 - 40,
		.isVisible = true,
	};
	formSubtitleWidget.eWidgetType = WD_TYPE_LABEL;
	formSubtitleWidget.pvWidget = (void *)&formSubtitleData;

	cfgSavePathData = (gfx_Label){
		.text = "Config guardado en: USB:/CONFIGS/10_06_2026_04_34_56.tel",
		.pos.x = LCD_WIDTH / 2,
		.pos.y = formSubtitleData.pos.y + 100,
		.alignment = ALIGN_CENTER,
		.style = STYLE_SUCCESS,
		.typo = TYPO_MONO,
		.isVisible = false,
	};
	cfgSavePathWidget.eWidgetType = WD_TYPE_LABEL;
	cfgSavePathWidget.pvWidget = (void *)&cfgSavePathData;

	uint16_t workingHeight = 310;

	returnBtnData = (gfx_Button) {
		.name = "cancelBtn",
		.label = "OK",
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
		.onRelease = onReturnBtnReleased,
	};
	gfx_initRegTouch((void *)&returnBtnData, WD_TYPE_BUTTON);
	returnBtnWidget.eWidgetType = WD_TYPE_BUTTON;
	returnBtnWidget.pvWidget = (void *)&returnBtnData;

	useFullHeader(&g_sExportCfgCanvas);
	useNavigationButtons(&g_sExportCfgCanvas);
	canvasInsertAtTop(&g_sExportCfgCanvas.psWidgets, &formTitleWidget);
	canvasInsertAtTop(&g_sExportCfgCanvas.psWidgets, &formSubtitleWidget);
	canvasInsertAtTop(&g_sExportCfgCanvas.psWidgets, &cfgSavePathWidget);
	canvasInsertAtTop(&g_sExportCfgCanvas.psWidgets, &returnBtnWidget);

	Event_Subscribe(EVT_SYS_SHOW_ADJ_EXPORT_CFG, (EventHandler_fn)onShowExportCfgForm);
    Event_Subscribe(EVT_SYS_CFG_EXPORT_SUCCESS, (EventHandler_fn)onExportSuccess);
    Event_Subscribe(EVT_SYS_CFG_EXPORT_ERROR, (EventHandler_fn)onExportError);

	g_i16ExportCfgIndex = FormManager_AddForm(&g_sExportCfgCanvas);
}

