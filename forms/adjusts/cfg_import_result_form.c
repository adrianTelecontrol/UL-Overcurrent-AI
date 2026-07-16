#include "cfg_import_result_form.h"
#include <string.h>
#include <stdio.h>

#include "gui_core.h"
#include "gui_canvas.h"
#include "forms_manager.h"
#include "gui_theme.h"
#include "FT8xx_params.h"
#include "icon_map.h" // Asumiendo que aquí tienes macros como ICON_CHECK, ICON_WARNING

#include "forms/common_widgets.h"

// ========================================================
// VARIABLES DEL FORMULARIO
// ========================================================
static gfx_Canvas g_sResultCanvas;
int16_t g_i16CfgImportResultFormIndex;

// Widgets
static gfx_GenericWidget formTitleWidget, iconWidget, mainMsgWidget, subMsgWidget, btnOkWidget, errorCodeWidget;
static gfx_Label formTitleData, iconData, mainMsgData, subMsgData, errorCodeData;
static gfx_Button btnOkData;

static char errCodeBuff[] = "ERROR: 0x0000";
// ========================================================
// CALLBACKS
// ========================================================
static void onOkBtnRelease(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    // Al presionar Aceptar, regresamos al menú principal o menú de selección
    Event_Post(EVT_SYS_SHOW_OPTIONS_FORM, (EventParam_t){.ptr = NULL});
}

// ========================================================
// ACTUALIZACIÓN VISUAL SEGÚN EL ESTADO
// ========================================================
static void updateUIForState(CfgImportResult_e state) {
	errorCodeData.isVisible = false;
	subMsgData.style = STYLE_SECONDARY;
	btnOkData.bIsVisible = false;
    switch (state) {
        case CFG_RESULT_SUCCESS:
            iconData.text = ICON_CHECK;
            iconData.style = STYLE_SUCCESS;
            mainMsgData.text = "CONFIGURACION ENVIADA";
            mainMsgData.style = STYLE_SUCCESS;
            subMsgData.text = "EL HMI ha enviado la configuracion.\nEsperando resultado...";
            break;

        case CFG_RESULT_ERR_FILE:
            iconData.text = ICON_WARNING;
            iconData.style = STYLE_DANGER;
            mainMsgData.text = "ARCHIVO INVALIDO";
            mainMsgData.style = STYLE_DANGER;
            subMsgData.text = "El archivo USB no es un .TEL valido o esta corrupto.";
            break;

        case CFG_RESULT_ERR_REJECTED:
            iconData.text = ICON_WARNING; // O un ICON_CROSS si tienes
            iconData.style = STYLE_DANGER;
            mainMsgData.text = "CARGA RECHAZADA";
            mainMsgData.style = STYLE_DANGER;
            subMsgData.text = "La instrumentacion rechazo el archivo (Error Checksum).";
            break;

        case CFG_RESULT_ERR_TIMEOUT:
            iconData.text = ICON_WARNING; 
            iconData.style = STYLE_DANGER;
            mainMsgData.text = "ERROR DE COMUNICACION";
            mainMsgData.style = STYLE_DANGER;
            subMsgData.text = "No hubo respuesta de la instrumentacion remota.";
            break;
            
        default:
            break;
    }

    // Forzar redibujado de los textos cambiados
    iconData.bIsDirty = true;
    mainMsgData.bIsDirty = true;
    subMsgData.bIsDirty = true;
}

static void onCfgLoadOkEvent(EventParam_t arg) {
    iconData.text = ICON_CHECK;
    iconData.style = STYLE_SUCCESS;
    mainMsgData.text = "CONFIGURACION CARGADA";
    mainMsgData.style = STYLE_SUCCESS;
    subMsgData.text = "La instrumentacion ha aceptado los parametros.";
	iconData.bIsDirty = true;
	mainMsgData.bIsDirty = true;
	subMsgData.bIsDirty = true;
	btnOkData.bIsVisible = true;
	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onCfgLoadErrorEvent(EventParam_t arg) {
    iconData.text = ICON_WARNING; // O un ICON_CROSS si tienes
    iconData.style = STYLE_DANGER;
    mainMsgData.text = "Error";
    mainMsgData.style = STYLE_DANGER;
    subMsgData.text = "La instrumentacion rechazo el archivo.";
	subMsgData.style = STYLE_DANGER;
	snprintf(errCodeBuff, sizeof(errCodeBuff), "Error: %#04x", arg.ui32);
	errorCodeData.text = errCodeBuff;
	errorCodeData.style = STYLE_DANGER;
	errorCodeData.isVisible = true;

	iconData.bIsDirty = true;
	mainMsgData.bIsDirty = true;
	subMsgData.bIsDirty = true;
	btnOkData.bIsVisible = true;
	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onImportTimeout(EventParam_t arg) {
    iconData.text = ICON_WARNING; // O un ICON_CROSS si tienes
    iconData.style = STYLE_DANGER;

    mainMsgData.text = "IMPORT TIMEOUT";
    mainMsgData.style = STYLE_DANGER;
	mainMsgData.bIsDirty = true;
    
    subMsgData.text = "La instrumentacion tardo demasiado en responder!";
    subMsgData.style = STYLE_DANGER;
    subMsgData.isVisible = true;
    subMsgData.bIsDirty = true;
    
    btnOkData.bIsVisible = true;
    btnOkData.bIsDirty = true;

	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

// Este evento es invocado para mostrar esta pantalla y pasarle el resultado
static void onShowResultForm(EventParam_t arg) {
    CfgImportResult_e result = (CfgImportResult_e)arg.ui32; // Extraemos el estado del parámetro
    updateUIForState(result);
}

// ========================================================
// INICIALIZACIÓN
// ========================================================
void initCfgImportResultForm(void) {
    g_sResultCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
    g_sResultCanvas.psWidgets = NULL; 

    // --- TÍTULO ---
    formTitleData = (gfx_Label) {
        .name = "formTitle", .text = "IMPORT CFG",
        .pos.x = 110, .pos.y = 50, .alignment = ALIGN_LEFT,
        .typo = TYPO_H3, .style = STYLE_TEXT_MAIN, .isVisible = true,
    };
    formTitleWidget.eWidgetType = WD_TYPE_LABEL; formTitleWidget.pvWidget = &formTitleData;

    // --- ÍCONO CENTRAL ---
    iconData = (gfx_Label) {
        .name = "iconLabel", .text = "", // Se llena dinámicamente
        .pos.x = LCD_WIDTH / 2, .pos.y = 150, .alignment = ALIGN_CENTER,
        .typo = TYPO_ICON, .style = STYLE_TEXT_MAIN, .isVisible = true,
    };
    iconWidget.eWidgetType = WD_TYPE_LABEL; iconWidget.pvWidget = &iconData;

    // --- MENSAJE PRINCIPAL ---
    mainMsgData = (gfx_Label) {
        .name = "mainMsg", .text = "", 
        .pos.x = LCD_WIDTH / 2, .pos.y = 220, .alignment = ALIGN_CENTER,
        .typo = TYPO_H2, .style = STYLE_TEXT_MAIN, .isVisible = true,
    };
    mainMsgWidget.eWidgetType = WD_TYPE_LABEL; mainMsgWidget.pvWidget = &mainMsgData;

    // --- SUB MENSAJE (Explicación) ---
    subMsgData = (gfx_Label) {
        .name = "subMsg", .text = "", 
        .pos.x = LCD_WIDTH / 2, .pos.y = 290, .alignment = ALIGN_CENTER,
        .typo = TYPO_BODY, .style = STYLE_SECONDARY, .isVisible = true,
		.oldPos.x = LCD_WIDTH / 2, .oldPos.y = 290,
    };
    subMsgWidget.eWidgetType = WD_TYPE_LABEL; subMsgWidget.pvWidget = &subMsgData;

    errorCodeData = (gfx_Label) {
        .name = "errorCode", .text = "", 
        .pos.x = LCD_WIDTH / 2, .pos.y = subMsgData.pos.y + 30, .alignment = ALIGN_CENTER,
        .typo = TYPO_CAPTION, .style = STYLE_SECONDARY, .isVisible = false,
		.oldPos.x = LCD_WIDTH / 2, .oldPos.y = subMsgData.pos.y + 50,
    };
    errorCodeWidget.eWidgetType = WD_TYPE_LABEL; errorCodeWidget.pvWidget = &errorCodeData;

    // --- BOTÓN ACEPTAR ---
    btnOkData = (gfx_Button) {
        .label = "ACEPTAR", 
        .pos.x = LCD_WIDTH / 2 - 100, .pos.y = 350, 
        .size.width = 200, .size.height = 50,
        .style = STYLE_DEFAULT, .bIsVisible = false, 
        .onPressed = onGenericBtnPressed, .onRelease = onOkBtnRelease, 
        .typo = TYPO_BODY, .radius = 5, .borderWidth = 2
    };
    gfx_initRegTouch((void *)&btnOkData, WD_TYPE_BUTTON);
    btnOkWidget.eWidgetType = WD_TYPE_BUTTON; btnOkWidget.pvWidget = &btnOkData;

    // Insertar en Canvas
    useFullHeader(&g_sResultCanvas);
	useNavigationButtons(&g_sResultCanvas);
    canvasInsertAtTop(&g_sResultCanvas.psWidgets, &formTitleWidget);
    canvasInsertAtTop(&g_sResultCanvas.psWidgets, &iconWidget);
    canvasInsertAtTop(&g_sResultCanvas.psWidgets, &mainMsgWidget);
    canvasInsertAtTop(&g_sResultCanvas.psWidgets, &subMsgWidget);
    canvasInsertAtTop(&g_sResultCanvas.psWidgets, &errorCodeWidget);
    canvasInsertAtTop(&g_sResultCanvas.psWidgets, &btnOkWidget);

    Event_Subscribe(EVT_SYS_SHOW_ADJ_CFG_IMPORT_RESULT, (EventHandler_fn)onShowResultForm);
	Event_Subscribe(EVT_CAN_INST_CFG_LOAD_ERROR, (EventHandler_fn)onCfgLoadErrorEvent);
	Event_Subscribe(EVT_CAN_INST_CFG_LOAD_OK, (EventHandler_fn)onCfgLoadOkEvent);
	Event_Subscribe(EVT_SYS_CFG_IMPORT_TIMEOUT, (EventHandler_fn)onImportTimeout);

    g_i16CfgImportResultFormIndex = FormManager_AddForm(&g_sResultCanvas);
}