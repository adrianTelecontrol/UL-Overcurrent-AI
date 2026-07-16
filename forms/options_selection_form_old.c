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

#include "options_selection_form.h"

int16_t g_i16OptionsSelectionIndex = 0; 

gfx_Canvas g_sOptionsSelectionCanvas;

// Containers
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget formSubtitleWidget;
static gfx_GenericWidget tempOptWidget, voltOptWidget, currOptWidget;
static gfx_GenericWidget pidOptWidget, calendarOptWidget, exportCfgWidget;

// Nuevos Contenedores
static gfx_GenericWidget saveEepromWidget;
static gfx_GenericWidget debugWidget;

// Widgets Data
static gfx_Label formTitleData;
static gfx_Label formSubtitleData;
static gfx_Button tempOptData, voltOptData, currOptData;
static gfx_Button pidOptData, calendarOptData, exportCfgData;

// Nuevos Widgets Data
static gfx_Button saveEepromData;
static gfx_Button debugData;

static EventID_e g_eFormCallback = EVT_SYS_NULL;

// Callbacks
static void onShowThisFormEvent(EventParam_t arg) {

}

static void onTempBtnReleasedEvent(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_ADJ_SELECT_TEMP, (EventParam_t){.ptr = NULL});
}

static void onVoltBtnReleasedEvent(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_ADJ_SELECT_VOLTAGE, (EventParam_t){.ptr = NULL});
}

static void onCurrBtnReleasedEvent(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_ADJ_SELECT_CURRENT, (EventParam_t){.ptr = NULL});
}

static void onPIDBtnReleasedEvent(gfx_Button *btn) {
    onGenericBtnRelease(btn);
}

static void onRTCBtnReleasedEvent(gfx_Button *btn) {
    onGenericBtnRelease(btn);
	Event_Post(EVT_SYS_SHOW_ADJ_RTC_FORM, (EventParam_t){.ptr = NULL});
}

static void onExportCfgBtnReleasedEvent(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_ADJ_CFG_SELECT_FORM, (EventParam_t){.ptr = NULL});
}

// Nuevos Callbacks
static void onSaveEepromBtnReleasedEvent(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    // Asumiendo que crearás este evento para disparar el guardado de ajustes
    Event_Post(EVT_SYS_SAVE_EEPROM, (EventParam_t){.ptr = NULL});
}

static void onDebugBtnReleasedEvent(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    // Asumiendo que crearás una pantalla de menú intermedio para seleccionar qué 
    // formulario de debug abrir (E/S, Variac, Lecturas, etc.)
    Event_Post(EVT_SYS_SHOW_DEBUG_MENU, (EventParam_t){.ptr = NULL});
}

void initOptionsSelectionForm(void) {
    g_sOptionsSelectionCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
    
    formTitleData = (gfx_Label) {
        .name = "formTitleData",
        .text = "AJUSTES",
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
        .text = "SELECCIONE UN MENU DE AJUSTE",
        .pos.x = LCD_WIDTH / 2,
        .pos.y = 90,
        .alignment = ALIGN_CENTER,
        .style = STYLE_DANGER,
        .typo = TYPO_H3,
        .isVisible = true,
    };
    formSubtitleWidget.eWidgetType = WD_TYPE_LABEL;
    formSubtitleWidget.pvWidget = (void *)&formSubtitleData;

    // --- REAJUSTE DE GEOMETRÍA (4 Filas x 2 Columnas) ---
    uint16_t workingHeight = 280; // Aumentado ligeramente para acomodar la 4ta fila
    uint16_t btnWidth = LCD_WIDTH / 3.5;
    uint16_t verticalPadding = 15;
    uint16_t btnHeight = (workingHeight - (verticalPadding * 3)) / 4.0f; // Calculado para 4 botones

    float col1_x = (LCD_WIDTH / 2 - btnWidth) / 2.0;
    float col2_x = LCD_WIDTH / 2.0f + (LCD_WIDTH / 2 - btnWidth) / 2.0;
    
    float row1_y = formSubtitleData.pos.y + 25;
    float row2_y = row1_y + btnHeight + verticalPadding;
    float row3_y = row2_y + btnHeight + verticalPadding;
    float row4_y = row3_y + btnHeight + verticalPadding;

    // Fila 1
    tempOptData = (gfx_Button) {
        .label = "TEMPERATURA",
        .pos.x = col1_x, .pos.y = row1_y, .size.width = btnWidth, .size.height = btnHeight,
        .borderWidth = 3, .radius = 2, .state = BTN_STATE_NORMAL, .style = STYLE_SECONDARY, .typo = TYPO_BODY, .bIsVisible = true, 
        .onPressed = onGenericBtnPressed, .onRelease = onTempBtnReleasedEvent,
    };
    gfx_initRegTouch((void *)&tempOptData, WD_TYPE_BUTTON);
    tempOptWidget.eWidgetType = WD_TYPE_BUTTON; tempOptWidget.pvWidget = &tempOptData;

    pidOptData = (gfx_Button) {
        .label = "PID",
        .pos.x = col2_x, .pos.y = row1_y, .size.width = btnWidth, .size.height = btnHeight,
        .borderWidth = 3, .radius = 2, .state = BTN_STATE_NORMAL, .style = STYLE_SECONDARY, .typo = TYPO_BODY, .bIsVisible = true, 
        .onPressed = onGenericBtnPressed, .onRelease = onPIDBtnReleasedEvent,
    };
    gfx_initRegTouch((void *)&pidOptData, WD_TYPE_BUTTON);
    pidOptWidget.eWidgetType = WD_TYPE_BUTTON; pidOptWidget.pvWidget = &pidOptData;

    // Fila 2
    voltOptData = (gfx_Button) {
        .label = "VOLTAJE",
        .pos.x = col1_x, .pos.y = row2_y, .size.width = btnWidth, .size.height = btnHeight,
        .borderWidth = 3, .radius = 2, .state = BTN_STATE_NORMAL, .style = STYLE_DEFAULT, .typo = TYPO_BODY, .bIsVisible = true, 
        .onPressed = onGenericBtnPressed, .onRelease = onVoltBtnReleasedEvent,
    };
    gfx_initRegTouch((void *)&voltOptData, WD_TYPE_BUTTON);
    voltOptWidget.eWidgetType = WD_TYPE_BUTTON; voltOptWidget.pvWidget = &voltOptData;

    calendarOptData = (gfx_Button) {
        .label = "RTC",
        .pos.x = col2_x, .pos.y = row1_y, .size.width = btnWidth, .size.height = btnHeight,
        //.pos.x = col2_x, .pos.y = row2_y, .size.width = btnWidth, .size.height = btnHeight,
        .borderWidth = 3, .radius = 2, .state = BTN_STATE_NORMAL, .style = STYLE_DEFAULT, .typo = TYPO_BODY, .bIsVisible = true, 
        .onPressed = onGenericBtnPressed, .onRelease = onRTCBtnReleasedEvent,
    };
    gfx_initRegTouch((void *)&calendarOptData, WD_TYPE_BUTTON);
    calendarOptWidget.eWidgetType = WD_TYPE_BUTTON; calendarOptWidget.pvWidget = &calendarOptData;

    // Fila 3
    currOptData = (gfx_Button) {
        .label = "CORRIENTE",
        .pos.x = col1_x, .pos.y = row3_y, .size.width = btnWidth, .size.height = btnHeight,
        .borderWidth = 3, .radius = 2, .state = BTN_STATE_NORMAL, .style = STYLE_SECONDARY, .typo = TYPO_BODY, .bIsVisible = true, 
        .onPressed = onGenericBtnPressed, .onRelease = onCurrBtnReleasedEvent,
    };
    gfx_initRegTouch((void *)&currOptData, WD_TYPE_BUTTON);
    currOptWidget.eWidgetType = WD_TYPE_BUTTON; currOptWidget.pvWidget = &currOptData;

    exportCfgData = (gfx_Button) {
        .label = "LOAD/EXPORT CFG",
        .pos.x = col2_x, .pos.y = row3_y, .size.width = btnWidth, .size.height = btnHeight,
        .borderWidth = 3, .radius = 2, .state = BTN_STATE_NORMAL, .style = STYLE_SECONDARY, .typo = TYPO_BODY, .bIsVisible = true, 
        .onPressed = onGenericBtnPressed, .onRelease = onExportCfgBtnReleasedEvent,
    };
    gfx_initRegTouch((void *)&exportCfgData, WD_TYPE_BUTTON);
    exportCfgWidget.eWidgetType = WD_TYPE_BUTTON; exportCfgWidget.pvWidget = &exportCfgData;

    // Fila 4 (Nuevos Botones)
    saveEepromData = (gfx_Button) {
        .label = "SAVE TO EEPROM",
        .pos.x = col1_x, .pos.y = row4_y, .size.width = btnWidth, .size.height = btnHeight,
        .borderWidth = 3, .radius = 2, .state = BTN_STATE_NORMAL, .style = STYLE_DEFAULT, .typo = TYPO_BODY, .bIsVisible = true, 
        .onPressed = onGenericBtnPressed, .onRelease = onSaveEepromBtnReleasedEvent,
    };
    gfx_initRegTouch((void *)&saveEepromData, WD_TYPE_BUTTON);
    saveEepromWidget.eWidgetType = WD_TYPE_BUTTON; saveEepromWidget.pvWidget = &saveEepromData;

    debugData = (gfx_Button) {
        .label = "DEBUG",
        .pos.x = col2_x, .pos.y = row4_y, .size.width = btnWidth, .size.height = btnHeight,
        .borderWidth = 3, .radius = 2, .state = BTN_STATE_NORMAL, .style = STYLE_DEFAULT, .typo = TYPO_BODY, .bIsVisible = true, 
        .onPressed = onGenericBtnPressed, .onRelease = onDebugBtnReleasedEvent,
    };
    gfx_initRegTouch((void *)&debugData, WD_TYPE_BUTTON);
    debugWidget.eWidgetType = WD_TYPE_BUTTON; debugWidget.pvWidget = &debugData;


    // Ensamble
    useFullHeader(&g_sOptionsSelectionCanvas);
    useNavigationButtons(&g_sOptionsSelectionCanvas);
    
    canvasInsertAtTop(&g_sOptionsSelectionCanvas.psWidgets, &formTitleWidget);
    canvasInsertAtTop(&g_sOptionsSelectionCanvas.psWidgets, &formSubtitleWidget);
    
    canvasInsertAtTop(&g_sOptionsSelectionCanvas.psWidgets, &tempOptWidget);
    canvasInsertAtTop(&g_sOptionsSelectionCanvas.psWidgets, &pidOptWidget);
    
    canvasInsertAtTop(&g_sOptionsSelectionCanvas.psWidgets, &voltOptWidget);
    canvasInsertAtTop(&g_sOptionsSelectionCanvas.psWidgets, &calendarOptWidget);
    
    canvasInsertAtTop(&g_sOptionsSelectionCanvas.psWidgets, &currOptWidget);
    canvasInsertAtTop(&g_sOptionsSelectionCanvas.psWidgets, &exportCfgWidget);

    canvasInsertAtTop(&g_sOptionsSelectionCanvas.psWidgets, &saveEepromWidget);
    canvasInsertAtTop(&g_sOptionsSelectionCanvas.psWidgets, &debugWidget);
    
    g_i16OptionsSelectionIndex = FormManager_AddForm(&g_sOptionsSelectionCanvas);
}