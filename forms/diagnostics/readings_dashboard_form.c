#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gui_core.h"
#include "FT8xx_params.h"
#include "forms/common_widgets.h"
#include "EVE_colors.h"
#include "gui_canvas.h"
#include "forms_manager.h"

#include "helpers.h"
#include "gui_theme.h"
#include "event_engine.h"
#include "icon_map.h"

#include "readings_dashboard_form.h"

static gfx_Canvas g_sReadingsCanvas;

int16_t g_i16ReadingsDashboardFormID = 0;
static bool g_bIsSecondaryViewVisible = false;

// ==========================================
// 1. Contenedores de Widgets
// ==========================================
static gfx_GenericWidget titleWidget;

// Paneles Base (Columna Izquierda y Derecha)
static gfx_GenericWidget leftColBgWidget;
static gfx_GenericWidget rightColBgWidget;

// Cajas individuales de indicadores (3 por columna)
static gfx_GenericWidget boxL1Widget, boxL2Widget, boxL3Widget;
static gfx_GenericWidget boxR1Widget, boxR2Widget, boxR3Widget;

// Etiquetas de título de cada caja
static gfx_GenericWidget lblL1Widget, lblL2Widget, lblL3Widget;
static gfx_GenericWidget lblR1Widget, lblR2Widget, lblR3Widget;

// Valores numéricos de cada caja
static gfx_GenericWidget valL1Widget, valL2Widget, valL3Widget;
static gfx_GenericWidget valR1Widget, valR2Widget, valR3Widget;

// Iconos de cada caja
static gfx_GenericWidget iconL1Widget, iconL2Widget, iconL3Widget;
static gfx_GenericWidget iconR1Widget, iconR2Widget, iconR3Widget;

// Botones de navegación
static gfx_GenericWidget leftArrowButtonWidget;
static gfx_GenericWidget rightArrowButtonWidget;
static gfx_GenericWidget returnBtnWidget; // Botón para salir del dashboard

// ==========================================
// 2. Datos en Memoria
// ==========================================
static gfx_Label titleData;

static gfx_Rectangle leftColBgData;
static gfx_Rectangle rightColBgData;

static gfx_Rectangle boxL1Data, boxL2Data, boxL3Data;
static gfx_Rectangle boxR1Data, boxR2Data, boxR3Data;

static gfx_Label lblL1Data, lblL2Data, lblL3Data;
static gfx_Label lblR1Data, lblR2Data, lblR3Data;

static gfx_Label valL1Data, valL2Data, valL3Data;
static gfx_Label valR1Data, valR2Data, valR3Data;

static gfx_Label iconL1Data, iconL2Data, iconL3Data;
static gfx_Label iconR1Data, iconR2Data, iconR3Data;

static gfx_Button leftArrowButtonData;
static gfx_Button rightArrowButtonData;
static gfx_Button returnBtnData;

// ==========================================
// 3. Buffers de Texto y Definiciones
// ==========================================

// Los 11 Parámetros a mostrar
static char paramNames[11][30] = {
    "T. Sonda Primaria",  // 0
    "T. Sonda Sec.",      // 1
    "T. Tx Primario",     // 2
    "T. Tx Secundario",   // 3
    "T. Cable A",         // 4
    "T. Cable B",         // 5
    "T. CJC",             // 6
    "V. Tx Primario",     // 7
    "V. Tx Secundario",   // 8
    "I. Tx Primario",     // 9
    "I. Tx Secundario"    // 10
};

// Buffers para los valores (actualizados por los eventos CAN)
static char valBuffers[11][16] = {
    "--.- [C]", "--.- [C]", "--.- [C]", "--.- [C]", "--.- [C]", "--.- [C]", "--.- [C]", 
    "--.- [V]", "--.- [V]", 
    "--.- [A]", "--.- [A]"
};

// Íconos correspondientes a cada parámetro
static char paramIcons[11][2] = {
    ICON_TEMPERATURE, ICON_TEMPERATURE, ICON_TEMPERATURE, ICON_TEMPERATURE, ICON_TEMPERATURE, ICON_TEMPERATURE, ICON_TEMPERATURE,
    ICON_WAVE, ICON_WAVE,
    ICON_LIGHTNING, ICON_LIGHTNING
};

// ==========================================
// RUTINAS DE ACTUALIZACIÓN DE VISTA
// ==========================================

#define ANIMATION_OFFSET 30
#define PANEL_WIDTH 365

// Función auxiliar para reubicar todos los widgets en X
static void updateLayoutPositions(float base_x) {
    float leftCol_x = base_x;
    float rightCol_x = leftCol_x + PANEL_WIDTH + 10;

    float boxL_x = leftCol_x + 10;
    float boxR_x = rightCol_x + 10;

    float lblL_x = boxL_x + 35;
    float lblR_x = boxR_x + 35;

    float valL_x = boxL_x + 85;
    float valR_x = boxR_x + 85;

    float iconL_x = boxL_x + 10;
    float iconR_x = boxR_x + 10;

    // Fondos
    leftColBgData.pos.x = leftCol_x;
    rightColBgData.pos.x = rightCol_x;

    // Cajas
    boxL1Data.pos.x = boxL_x; boxL2Data.pos.x = boxL_x; boxL3Data.pos.x = boxL_x;
    boxR1Data.pos.x = boxR_x; boxR2Data.pos.x = boxR_x; boxR3Data.pos.x = boxR_x;

    // Títulos
    lblL1Data.pos.x = lblL_x; lblL2Data.pos.x = lblL_x; lblL3Data.pos.x = lblL_x;
    lblR1Data.pos.x = lblR_x; lblR2Data.pos.x = lblR_x; lblR3Data.pos.x = lblR_x;

    // Valores Numéricos
    valL1Data.pos.x = valL_x; valL2Data.pos.x = valL_x; valL3Data.pos.x = valL_x;
    valR1Data.pos.x = valR_x; valR2Data.pos.x = valR_x; valR3Data.pos.x = valR_x;

    // Iconos
    iconL1Data.pos.x = iconL_x; iconL2Data.pos.x = iconL_x; iconL3Data.pos.x = iconL_x;
    iconR1Data.pos.x = iconR_x; iconR2Data.pos.x = iconR_x; iconR3Data.pos.x = iconR_x;
}

static void loadPrimaryView(void) {
    // 1. Mover todo a la posición inicial (Alineado a la izquierda)
    updateLayoutPositions(10);

    // 2. Cargar datos de la Vista 1
    lblL1Data.text = paramNames[0]; valL1Data.text = valBuffers[0]; iconL1Data.text = paramIcons[0];
    lblL2Data.text = paramNames[1]; valL2Data.text = valBuffers[1]; iconL2Data.text = paramIcons[1];
    lblL3Data.text = paramNames[2]; valL3Data.text = valBuffers[2]; iconL3Data.text = paramIcons[2];

    lblR1Data.text = paramNames[3]; valR1Data.text = valBuffers[3]; iconR1Data.text = paramIcons[3];
    lblR2Data.text = paramNames[4]; valR2Data.text = valBuffers[4]; iconR2Data.text = paramIcons[4];
    lblR3Data.text = paramNames[5]; valR3Data.text = valBuffers[5]; iconR3Data.text = paramIcons[5];
    
    // Restaurar visibilidad de la caja inferior derecha
    lblR3Data.isVisible = true; valR3Data.isVisible = true; iconR3Data.isVisible = true;
    
    // 3. Ajustar botones
    rightArrowButtonData.bIsVisible = true;
    leftArrowButtonData.bIsVisible = false;
    
    Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void loadSecondaryView(void) {
    // 1. Mover todo ligeramente a la derecha para dar espacio al botón izquierdo
    updateLayoutPositions(15.0f + ANIMATION_OFFSET);

    // 2. Cargar datos de la Vista 2
    lblL1Data.text = paramNames[6]; valL1Data.text = valBuffers[6]; iconL1Data.text = paramIcons[6];
    lblL2Data.text = paramNames[7]; valL2Data.text = valBuffers[7]; iconL2Data.text = paramIcons[7];
    lblL3Data.text = paramNames[8]; valL3Data.text = valBuffers[8]; iconL3Data.text = paramIcons[8];

    lblR1Data.text = paramNames[9]; valR1Data.text = valBuffers[9]; iconR1Data.text = paramIcons[9];
    lblR2Data.text = paramNames[10]; valR2Data.text = valBuffers[10]; iconR2Data.text = paramIcons[10];
    
    // Ocultar caja vacía
    lblR3Data.isVisible = false; valR3Data.isVisible = false; iconR3Data.isVisible = false;
    
    // 3. Ajustar botones
    rightArrowButtonData.bIsVisible = false;
    leftArrowButtonData.bIsVisible = true;
    
    Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

// ==========================================
// CALLBACKS DE NAVEGACIÓN
// ==========================================

static void onRightArrowReleaseEvent(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    g_bIsSecondaryViewVisible = true;
    loadSecondaryView();
}

static void onLeftArrowReleaseEvent(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    g_bIsSecondaryViewVisible = false;
    loadPrimaryView();
}

static void onReturnBtnReleaseEvent(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_DEBUG_MENU, (EventParam_t){.ptr = NULL});
}

// ==========================================
// CALLBACKS DE ACTUALIZACIÓN DE DATOS CAN
// ==========================================

// Macro para simplificar la creación de callbacks de actualización
#define CREATE_UPDATE_CALLBACK(FuncName, BufferIdx, CondView) \
static void FuncName(EventParam_t arg) { \
    if(GetExecTimeMs() % 400 < 80) { \
        if (BufferIdx >= 7 && BufferIdx <= 8) snprintf(valBuffers[BufferIdx], 16, "%.1f [V]", arg.f32); \
        else if (BufferIdx >= 9) snprintf(valBuffers[BufferIdx], 16, "%.1f [A]", arg.f32); \
        else snprintf(valBuffers[BufferIdx], 16, "%.1f [C]", arg.f32); \
        if(CondView) { \
            valL1Data.bIsDirty = true; valL2Data.bIsDirty = true; valL3Data.bIsDirty = true; \
            valR1Data.bIsDirty = true; valR2Data.bIsDirty = true; valR3Data.bIsDirty = true; \
        } \
    } \
}

// Generar callbacks para los 11 parámetros
CREATE_UPDATE_CALLBACK(onTempSondaPri,  0, !g_bIsSecondaryViewVisible)
CREATE_UPDATE_CALLBACK(onTempSondaSec,  1, !g_bIsSecondaryViewVisible)
CREATE_UPDATE_CALLBACK(onTempTxPri,     2, !g_bIsSecondaryViewVisible)
CREATE_UPDATE_CALLBACK(onTempTxSec,     3, !g_bIsSecondaryViewVisible)
CREATE_UPDATE_CALLBACK(onTempCableA,    4, !g_bIsSecondaryViewVisible)
CREATE_UPDATE_CALLBACK(onTempCableB,    5, !g_bIsSecondaryViewVisible)

CREATE_UPDATE_CALLBACK(onTempCJC,       6, g_bIsSecondaryViewVisible)
CREATE_UPDATE_CALLBACK(onVoltTxPri,     7, g_bIsSecondaryViewVisible)
CREATE_UPDATE_CALLBACK(onVoltTxSec,     8, g_bIsSecondaryViewVisible)
CREATE_UPDATE_CALLBACK(onCurrTxPri,     9, g_bIsSecondaryViewVisible)
CREATE_UPDATE_CALLBACK(onCurrTxSec,    10, g_bIsSecondaryViewVisible)


// ==========================================
// 4. FUNCIÓN DE INICIALIZACIÓN
// ==========================================

void initReadingsDashboardForm(void) {
    g_sReadingsCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background; 
    g_sReadingsCanvas.psWidgets = NULL;

    titleData = (gfx_Label){
        .text = "LECTURAS", .name = "sysTitle",
        .pos.x = 125, .pos.y = 50, .alignment = ALIGN_LEFT,
        .typo = TYPO_H3, .style = STYLE_TEXT_MAIN, .isVisible = true,
    };
    titleWidget.eWidgetType = WD_TYPE_LABEL; titleWidget.pvWidget = &titleData;

    // Geometría base
    uint16_t panelWidth = 365;
    uint16_t panelHeight = 325;
    uint16_t indicatorWidth = panelWidth - 20;
    uint16_t indicatorHeight = (panelHeight - 40) / 3.0;
    

    // --- Columna Izquierda ---
    leftColBgData = (gfx_Rectangle){ .pos.x = 10, .pos.y = 80, .dim.width = panelWidth, .dim.height = panelHeight, .round = 5, .color = g_pCurrentTheme->palette.surface, .borderWidth = 1 };
    leftColBgWidget.eWidgetType = WD_TYPE_RECT; leftColBgWidget.pvWidget = &leftColBgData;

    boxL1Data = (gfx_Rectangle){ .pos.x = leftColBgData.pos.x + 10, .pos.y = leftColBgData.pos.y + 10, .dim.width = indicatorWidth, .dim.height = indicatorHeight, .round = 5, .color = g_pCurrentTheme->palette.background, .borderWidth = 1 };
    boxL2Data = (gfx_Rectangle){ .pos.x = boxL1Data.pos.x, .pos.y = boxL1Data.pos.y + indicatorHeight + 10, .dim.width = indicatorWidth, .dim.height = indicatorHeight, .round = 5, .color = g_pCurrentTheme->palette.background, .borderWidth = 1 };
    boxL3Data = (gfx_Rectangle){ .pos.x = boxL1Data.pos.x, .pos.y = boxL2Data.pos.y + indicatorHeight + 10, .dim.width = indicatorWidth, .dim.height = indicatorHeight, .round = 5, .color = g_pCurrentTheme->palette.background, .borderWidth = 1 };
    
    boxL1Widget.eWidgetType = WD_TYPE_RECT; boxL1Widget.pvWidget = &boxL1Data;
    boxL2Widget.eWidgetType = WD_TYPE_RECT; boxL2Widget.pvWidget = &boxL2Data;
    boxL3Widget.eWidgetType = WD_TYPE_RECT; boxL3Widget.pvWidget = &boxL3Data;

    // --- Columna Derecha ---
    rightColBgData = (gfx_Rectangle){ .pos.x = leftColBgData.pos.x + panelWidth + 10, .pos.y = 80, .dim.width = panelWidth, .dim.height = panelHeight, .round = 5, .color = g_pCurrentTheme->palette.surface, .borderWidth = 1 };
    rightColBgWidget.eWidgetType = WD_TYPE_RECT; rightColBgWidget.pvWidget = &rightColBgData;

    boxR1Data = (gfx_Rectangle){ .pos.x = rightColBgData.pos.x + 10, .pos.y = rightColBgData.pos.y + 10, .dim.width = indicatorWidth, .dim.height = indicatorHeight, .round = 5, .color = g_pCurrentTheme->palette.background, .borderWidth = 1 };
    boxR2Data = (gfx_Rectangle){ .pos.x = boxR1Data.pos.x, .pos.y = boxR1Data.pos.y + indicatorHeight + 10, .dim.width = indicatorWidth, .dim.height = indicatorHeight, .round = 5, .color = g_pCurrentTheme->palette.background, .borderWidth = 1 };
    boxR3Data = (gfx_Rectangle){ .pos.x = boxR1Data.pos.x, .pos.y = boxR2Data.pos.y + indicatorHeight + 10, .dim.width = indicatorWidth, .dim.height = indicatorHeight, .round = 5, .color = g_pCurrentTheme->palette.background, .borderWidth = 1 };

    boxR1Widget.eWidgetType = WD_TYPE_RECT; boxR1Widget.pvWidget = &boxR1Data;
    boxR2Widget.eWidgetType = WD_TYPE_RECT; boxR2Widget.pvWidget = &boxR2Data;
    boxR3Widget.eWidgetType = WD_TYPE_RECT; boxR3Widget.pvWidget = &boxR3Data;

    // --- Instanciación de Etiquetas (Labels, Valores e Iconos) ---
    // Macro para crear componentes internos de las cajas
    #define INIT_BOX_CONTENT(LblData, ValData, IconData, BoxRef, LblWdgt, ValWdgt, IconWdgt) \
        LblData = (gfx_Label){ .pos.x = BoxRef.pos.x + 35, .pos.y = BoxRef.pos.y + BoxRef.dim.height / 5, .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER), .typo = TYPO_CAPTION, .style = STYLE_TEXT_MUTED, .isVisible = true }; \
        ValData = (gfx_Label){ .pos.x = BoxRef.pos.x + 85, .pos.y = LblData.pos.y + 10, .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_TOP), .typo = TYPO_H1, .style = STYLE_SECONDARY, .isVisible = true }; \
        IconData = (gfx_Label){ .pos.x = BoxRef.pos.x + 10, .pos.y = ValData.pos.y + 5, .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_TOP), .typo = TYPO_ICON, .style = STYLE_SECONDARY, .isVisible = true }; \
        LblWdgt.eWidgetType = WD_TYPE_LABEL; LblWdgt.pvWidget = &LblData; \
        ValWdgt.eWidgetType = WD_TYPE_LABEL; ValWdgt.pvWidget = &ValData; \
        IconWdgt.eWidgetType = WD_TYPE_LABEL; IconWdgt.pvWidget = &IconData;

    INIT_BOX_CONTENT(lblL1Data, valL1Data, iconL1Data, boxL1Data, lblL1Widget, valL1Widget, iconL1Widget)
    INIT_BOX_CONTENT(lblL2Data, valL2Data, iconL2Data, boxL2Data, lblL2Widget, valL2Widget, iconL2Widget)
    INIT_BOX_CONTENT(lblL3Data, valL3Data, iconL3Data, boxL3Data, lblL3Widget, valL3Widget, iconL3Widget)

    INIT_BOX_CONTENT(lblR1Data, valR1Data, iconR1Data, boxR1Data, lblR1Widget, valR1Widget, iconR1Widget)
    INIT_BOX_CONTENT(lblR2Data, valR2Data, iconR2Data, boxR2Data, lblR2Widget, valR2Widget, iconR2Widget)
    INIT_BOX_CONTENT(lblR3Data, valR3Data, iconR3Data, boxR3Data, lblR3Widget, valR3Widget, iconR3Widget)

    // Botones de Navegación laterales (Flechas)
    leftArrowButtonData = (gfx_Button) {
        .label = "<", .pos.x = 5, .pos.y = leftColBgData.pos.y, .size.width = 30, .size.height = panelHeight,
        .radius = 2, .borderWidth = 2, .style = STYLE_DEFAULT, .typo = TYPO_H3, .bIsVisible = false,
        .onPressed = onGenericBtnPressed, .onRelease = onLeftArrowReleaseEvent,
    };
    gfx_initRegTouch((void *)&leftArrowButtonData, WD_TYPE_BUTTON);
    leftArrowButtonWidget.eWidgetType = WD_TYPE_BUTTON; leftArrowButtonWidget.pvWidget = &leftArrowButtonData;
    
    rightArrowButtonData = (gfx_Button) {
        .label = ">", .pos.x = rightColBgData.pos.x + rightColBgData.dim.width + 10, .pos.y = leftColBgData.pos.y, 
        .size.width = 30, .size.height = panelHeight,
        .radius = 2, .borderWidth = 2, .style = STYLE_DEFAULT, .typo = TYPO_H3, .bIsVisible = true,
        .onPressed = onGenericBtnPressed, .onRelease = onRightArrowReleaseEvent,
    };
    gfx_initRegTouch((void *)&rightArrowButtonData, WD_TYPE_BUTTON);
    rightArrowButtonWidget.eWidgetType = WD_TYPE_BUTTON; rightArrowButtonWidget.pvWidget = &rightArrowButtonData;

    returnBtnData = (gfx_Button) {
        .label = "REGRESAR",
        .size.width = 180, 
		.size.height = 55,
        .pos.x = LCD_WIDTH / 2 - 180 / 2, 
		.pos.y = LCD_HEIGHT - 60,
        .borderWidth = 2, 
		.radius = 5, 
		.style = STYLE_DANGER, 
		.typo = TYPO_H3,
        .bIsVisible = true, 
		.state = BTN_STATE_NORMAL,
        .onPressed = onGenericBtnPressed, 
		.onRelease = onReturnBtnReleaseEvent,
    };
    gfx_initRegTouch((void *)&returnBtnData, WD_TYPE_BUTTON);
    returnBtnWidget.eWidgetType = WD_TYPE_BUTTON; returnBtnWidget.pvWidget = &returnBtnData;

    // --- Inserción en Canvas ---
    useFullHeader(&g_sReadingsCanvas);
    canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &titleWidget);
    canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &returnBtnWidget);
    
    canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &leftColBgWidget);
    canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &rightColBgWidget);
    
    canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &boxL1Widget); canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &boxL2Widget); canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &boxL3Widget);
    canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &boxR1Widget); canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &boxR2Widget); canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &boxR3Widget);

    canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &lblL1Widget); canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &lblL2Widget); canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &lblL3Widget);
    canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &lblR1Widget); canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &lblR2Widget); canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &lblR3Widget);

    canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &valL1Widget); canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &valL2Widget); canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &valL3Widget);
    canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &valR1Widget); canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &valR2Widget); canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &valR3Widget);

    canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &iconL1Widget); canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &iconL2Widget); canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &iconL3Widget);
    canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &iconR1Widget); canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &iconR2Widget); canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &iconR3Widget);

    canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &leftArrowButtonWidget);
    canvasInsertAtTop(&g_sReadingsCanvas.psWidgets, &rightArrowButtonWidget);

    // Cargar la vista primaria por defecto
    loadPrimaryView();

    // Suscripciones
    Event_Subscribe(EVT_CAN_INST_TEMP_PROBE_MAIN,      (EventHandler_fn)onTempSondaPri);
    Event_Subscribe(EVT_CAN_INST_TEMP_PROBE_SECONDARY, (EventHandler_fn)onTempSondaSec);
    Event_Subscribe(EVT_CAN_INST_TEMP_TX_PRIMARY,      (EventHandler_fn)onTempTxPri);
    Event_Subscribe(EVT_CAN_INST_TEMP_TX_SECONDARY,    (EventHandler_fn)onTempTxSec);
    Event_Subscribe(EVT_CAN_INST_TEMP_CABLE_A,         (EventHandler_fn)onTempCableA);
    Event_Subscribe(EVT_CAN_INST_TEMP_CABLE_B,         (EventHandler_fn)onTempCableB);
    Event_Subscribe(EVT_CAN_INST_TEMP_CJC,             (EventHandler_fn)onTempCJC);
    
    Event_Subscribe(EVT_CAN_INST_VOLTAGE_PRIMARY,      (EventHandler_fn)onVoltTxPri);
    Event_Subscribe(EVT_CAN_INST_VOLTAGE_SECONDARY,    (EventHandler_fn)onVoltTxSec);
    
    Event_Subscribe(EVT_CAN_INST_CURRENT_PRIMARY,      (EventHandler_fn)onCurrTxPri);
    Event_Subscribe(EVT_CAN_INST_CURRENT_SECUNDARY,    (EventHandler_fn)onCurrTxSec);

    g_i16ReadingsDashboardFormID = FormManager_AddForm(&g_sReadingsCanvas);
}