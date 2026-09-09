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

#include "temp_selection_form.h"

int16_t g_i16TempSelectionIndex = 0; 

gfx_Canvas g_sTempSelectionCanvas;

// ========================================================
// CONTENEDORES
// ========================================================
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget formSubtitleWidget;

static gfx_GenericWidget tempProbeMainBtnWidget, tempProbeSecondaryBtnWidget;
static gfx_GenericWidget tempTxPrimaryBtnWidget, tempTxSecondaryBtnWidget;
static gfx_GenericWidget cableABtnWidget, cableBBtnWidget, cjcBtnWidget;
static gfx_GenericWidget cancelBtnWidget;

// ========================================================
// WIDGETS
// ========================================================
static gfx_Label formTitleData;
static gfx_Label formSubtitleData;

static gfx_Button tempProbeMainBtnData, tempProbeSecondaryBtnData;
static gfx_Button tempTxPrimaryBtnData, tempTxSecondaryBtnData;
static gfx_Button cableABtnData, cableBBtnData, cjcBtnData;
static gfx_Button cancelBtnData;

// ========================================================
// CALLBACKS
// ========================================================
static void onCancelBtnRelased(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_OPTIONS_FORM, (EventParam_t){.ptr = NULL});
}

static void onProbeMainBtnReleased(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_ADJ_NUMPAD_FORM, (EventParam_t){.ui32 = ADJ_TEMP_PROBE_MAIN});
}

static void onProbeSecondaryBtnReleased(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_ADJ_NUMPAD_FORM, (EventParam_t){.ui32 = ADJ_TEMP_PROBE_SECONDARY});
}

static void onTxPrimaryBtnReleased(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_ADJ_NUMPAD_FORM, (EventParam_t){.ui32 = ADJ_TEMP_TX_PRIMARY});
}

static void onTxSecondaryBtnReleased(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_ADJ_NUMPAD_FORM, (EventParam_t){.ui32 = ADJ_TEMP_TX_SECONDARY});
}

static void onCableABtnReleased(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_ADJ_NUMPAD_FORM, (EventParam_t){.ui32 = ADJ_TEMP_CABLE_A});
}

static void onCableCBtnReleased(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    // Asumiendo que agregaste ADJ_TEMP_CABLE_C a tu enumerador adj_value_e
    Event_Post(EVT_SYS_SHOW_ADJ_NUMPAD_FORM, (EventParam_t){.ui32 = ADJ_TEMP_CABLE_B});
}

static void onCjcBtnReleased(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    // Asumiendo que agregaste ADJ_TEMP_CJC a tu enumerador adj_value_e
    Event_Post(EVT_SYS_SHOW_ADJ_NUMPAD_FORM, (EventParam_t){.ui32 = ADJ_TEMP_CJC});
}

static void onShowThisFormEvent(EventParam_t arg) {
    // Por ahora vacío, pero útil si necesitas resetear el estado de los botones
}

// ========================================================
// INICIALIZACIÓN
// ========================================================
void initTempSelectionForm(void) {
    g_sTempSelectionCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
    
    // --- TÍTULOS ---
    formTitleData = (gfx_Label) {
        .name = "formTitleData",
        .text = "TEMPERATURA",
        .pos.x = 125, .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
    };
    formTitleWidget.eWidgetType = WD_TYPE_LABEL;
    formTitleWidget.pvWidget = (void *)&formTitleData;

    formSubtitleData = (gfx_Label){
        .text = "SELECCIONE UN CANAL PARA AJUSTAR",
        .pos.x = LCD_WIDTH / 2, .pos.y = 100,
        .alignment = ALIGN_CENTER,
        .style = STYLE_DANGER,
        .typo = TYPO_H3,
        .isVisible = true,
    };
    formSubtitleWidget.eWidgetType = WD_TYPE_LABEL;
    formSubtitleWidget.pvWidget = (void *)&formSubtitleData;

    // --- GEOMETRÍA DE LA CUADRÍCULA (2 COLUMNAS) ---
    uint16_t btnWidth = (LCD_WIDTH - 80) / 2.0f; // 20px margen izq, 20px centro, 20px margen der
    uint16_t btnHeight = 60;
    uint16_t col1 = 30;
    uint16_t col2 = 30 + btnWidth + 20;
    
    uint16_t row1 = formSubtitleData.pos.y + 25;
    uint16_t row2 = row1 + btnHeight + 15;
    uint16_t row3 = row2 + btnHeight + 15;
    uint16_t row4 = row3 + btnHeight + 15;

    // --- BOTONES (FILA 1) ---
    tempProbeMainBtnData = (gfx_Button) {
        .label = "SONDA PRINCIPAL", .pos.x = col1, .pos.y = row1, .size.width = btnWidth, .size.height = btnHeight,
        .borderWidth = 1, .radius = 5, .state = BTN_STATE_NORMAL, .style = STYLE_DEFAULT, .typo = TYPO_H3, .bIsVisible = true, 
        .onPressed = onGenericBtnPressed, .onRelease = onProbeMainBtnReleased,
    };
    gfx_initRegTouch((void *)&tempProbeMainBtnData, WD_TYPE_BUTTON);
    tempProbeMainBtnWidget.eWidgetType = WD_TYPE_BUTTON; tempProbeMainBtnWidget.pvWidget = &tempProbeMainBtnData;

    tempProbeSecondaryBtnData = (gfx_Button) {
        .label = "SONDA SECUNDARIA", .pos.x = col2, .pos.y = row1, .size.width = btnWidth, .size.height = btnHeight,
        .borderWidth = 1, .radius = 5, .state = BTN_STATE_NORMAL, .style = STYLE_DEFAULT, .typo = TYPO_H3, .bIsVisible = true, 
        .onPressed = onGenericBtnPressed, .onRelease = onProbeSecondaryBtnReleased,
    };
    gfx_initRegTouch((void *)&tempProbeSecondaryBtnData, WD_TYPE_BUTTON);
    tempProbeSecondaryBtnWidget.eWidgetType = WD_TYPE_BUTTON; tempProbeSecondaryBtnWidget.pvWidget = &tempProbeSecondaryBtnData;

    // --- BOTONES (FILA 2) ---
    tempTxPrimaryBtnData = (gfx_Button) {
        .label = "TX PRINCIPAL", .pos.x = col1, .pos.y = row2, .size.width = btnWidth, .size.height = btnHeight,
        .borderWidth = 1, .radius = 5, .state = BTN_STATE_NORMAL, .style = STYLE_DEFAULT, .typo = TYPO_H3, .bIsVisible = true, 
        .onPressed = onGenericBtnPressed, .onRelease = onTxPrimaryBtnReleased,
    };
    gfx_initRegTouch((void *)&tempTxPrimaryBtnData, WD_TYPE_BUTTON);
    tempTxPrimaryBtnWidget.eWidgetType = WD_TYPE_BUTTON; tempTxPrimaryBtnWidget.pvWidget = &tempTxPrimaryBtnData;

    tempTxSecondaryBtnData = (gfx_Button) {
        .label = "TX SECUNDARIA", .pos.x = col2, .pos.y = row2, .size.width = btnWidth, .size.height = btnHeight,
        .borderWidth = 1, .radius = 5, .state = BTN_STATE_NORMAL, .style = STYLE_DEFAULT, .typo = TYPO_H3, .bIsVisible = true, 
        .onPressed = onGenericBtnPressed, .onRelease = onTxSecondaryBtnReleased,
    };
    gfx_initRegTouch((void *)&tempTxSecondaryBtnData, WD_TYPE_BUTTON);
    tempTxSecondaryBtnWidget.eWidgetType = WD_TYPE_BUTTON; tempTxSecondaryBtnWidget.pvWidget = &tempTxSecondaryBtnData;

    // --- BOTONES (FILA 3) ---
    cableABtnData = (gfx_Button) {
        .label = "CABLE A", .pos.x = col1, .pos.y = row3, .size.width = btnWidth, .size.height = btnHeight,
        .borderWidth = 1, .radius = 5, .state = BTN_STATE_NORMAL, .style = STYLE_DEFAULT, .typo = TYPO_H3, .bIsVisible = true, 
        .onPressed = onGenericBtnPressed, .onRelease = onCableABtnReleased,
    };
    gfx_initRegTouch((void *)&cableABtnData, WD_TYPE_BUTTON);
    cableABtnWidget.eWidgetType = WD_TYPE_BUTTON; cableABtnWidget.pvWidget = &cableABtnData;

    cableBBtnData = (gfx_Button) {
        .label = "CABLE B", .pos.x = col2, .pos.y = row3, .size.width = btnWidth, .size.height = btnHeight,
        .borderWidth = 1, .radius = 5, .state = BTN_STATE_NORMAL, .style = STYLE_DEFAULT, .typo = TYPO_H3, .bIsVisible = true, 
        .onPressed = onGenericBtnPressed, .onRelease = onCableCBtnReleased,
    };
    gfx_initRegTouch((void *)&cableBBtnData, WD_TYPE_BUTTON);
    cableBBtnWidget.eWidgetType = WD_TYPE_BUTTON; cableBBtnWidget.pvWidget = &cableBBtnData;

    // --- BOTONES (FILA 4) ---
    cjcBtnData = (gfx_Button) {
        .label = "CJC (Junta Fria)", .pos.x = col1, .pos.y = row4, .size.width = btnWidth, .size.height = btnHeight,
        .borderWidth = 1, .radius = 5, .state = BTN_STATE_NORMAL, .style = STYLE_DEFAULT, .typo = TYPO_H3, .bIsVisible = true, 
        .onPressed = onGenericBtnPressed, .onRelease = onCjcBtnReleased,
    };
    gfx_initRegTouch((void *)&cjcBtnData, WD_TYPE_BUTTON);
    cjcBtnWidget.eWidgetType = WD_TYPE_BUTTON; cjcBtnWidget.pvWidget = &cjcBtnData;

    cancelBtnData = (gfx_Button) {
        .label = "CANCELAR", .pos.x = col2, .pos.y = row4, .size.width = btnWidth, .size.height = btnHeight,
        .borderWidth = 2, .radius = 5, .state = BTN_STATE_NORMAL, .style = STYLE_DANGER, .typo = TYPO_H3, .bIsVisible = true, 
        .onPressed = onGenericBtnPressed, .onRelease = onCancelBtnRelased,
    };
    gfx_initRegTouch((void *)&cancelBtnData, WD_TYPE_BUTTON);
    cancelBtnWidget.eWidgetType = WD_TYPE_BUTTON; cancelBtnWidget.pvWidget = &cancelBtnData;

    // --- ENSAMBLE DEL CANVAS ---
    useFullHeader(&g_sTempSelectionCanvas);
    useNavigationButtons(&g_sTempSelectionCanvas);
    
    canvasInsertAtTop(&g_sTempSelectionCanvas.psWidgets, &formTitleWidget);
    canvasInsertAtTop(&g_sTempSelectionCanvas.psWidgets, &formSubtitleWidget);
    
    canvasInsertAtTop(&g_sTempSelectionCanvas.psWidgets, &tempProbeMainBtnWidget);
    canvasInsertAtTop(&g_sTempSelectionCanvas.psWidgets, &tempProbeSecondaryBtnWidget);
    canvasInsertAtTop(&g_sTempSelectionCanvas.psWidgets, &tempTxPrimaryBtnWidget);
    canvasInsertAtTop(&g_sTempSelectionCanvas.psWidgets, &tempTxSecondaryBtnWidget);
    canvasInsertAtTop(&g_sTempSelectionCanvas.psWidgets, &cableABtnWidget);
    canvasInsertAtTop(&g_sTempSelectionCanvas.psWidgets, &cableBBtnWidget);
    canvasInsertAtTop(&g_sTempSelectionCanvas.psWidgets, &cjcBtnWidget);
    canvasInsertAtTop(&g_sTempSelectionCanvas.psWidgets, &cancelBtnWidget);
    
    Event_Subscribe(EVT_SYS_SHOW_TEST_CONFIRMATION, (EventHandler_fn)onShowThisFormEvent);

    g_i16TempSelectionIndex = FormManager_AddForm(&g_sTempSelectionCanvas);
}