
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "gui_core.h"
#include "gui_theme.h"
#include "gui_canvas.h"
#include "forms_manager.h"
#include "FT8xx_params.h"
#include "event_engine.h"
#include "experiments_cfg.h"

#include "common_widgets.h"
#include "fault_test_config_form.h"

int16_t g_i16FaultConfigFormIndex;

static gfx_Canvas g_sFaultTestConfigCanvas;

// ========================================================
// CONTENEDORES DE WIDGETS
// ========================================================
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget formSubtitleWidget;
static gfx_GenericWidget startTestWidget;
static gfx_GenericWidget cancelConfigWidget;

// Etiquetas de Título de Caja
static gfx_GenericWidget setCaliberLabelWidget;
static gfx_GenericWidget setCurrentLabelWidget;
static gfx_GenericWidget testDurationLabelWidget;

// Fondos de Caja
static gfx_GenericWidget setCaliberBoxWidget;
static gfx_GenericWidget setCurrentBoxWidget;
static gfx_GenericWidget testDurationBoxWidget;

// Valores Dinámicos
static gfx_GenericWidget setCaliberValueWidget;
static gfx_GenericWidget setCurrentValueWidget;
static gfx_GenericWidget testDurationValueWidget;

// Áreas Táctiles
static gfx_GenericWidget caliberTouchWidget;
static gfx_GenericWidget currentTouchWidget;
static gfx_GenericWidget durationTouchWidget;

// Nuevo: Botón Toggle de Resistencia
static gfx_GenericWidget resistanceToggleBtnWidget;

// ========================================================
// DATOS DE WIDGETS
// ========================================================
static gfx_Label formTitleData;
static gfx_Label formSubtitleData;
static gfx_Button startTestData;
static gfx_Button cancelConfigData;

static gfx_Label setCaliberLabelData;
static gfx_Label setCurrentLabelData;
static gfx_Label testDurationLabelData;

static gfx_Rectangle setCaliberBoxData;
static gfx_Rectangle setCurrentBoxData;
static gfx_Rectangle testDurationBoxData;

static gfx_Label setCaliberValueData;
static gfx_Label setCurrentValueData;
static gfx_Label testDurationValueData;

static gfx_TouchArea caliberTouchAreaData;
static gfx_TouchArea currentTouchAreaData;
static gfx_TouchArea durationTouchAreaData;

static gfx_Button resistanceToggleBtnData;

// Buffers de texto y estado
static char caliberValBuffer[8] = "0"; 
static char currentValBuffer[8] = "450.0";
static char testDurationBuffer[6] = "4.0";
static char curCaliber[8] = "0";
static bool g_bIsHighResistance = false; // Estado del Toggle

// ========================================================
// CALLBACKS DE INTERACCIÓN
// ========================================================

static void onStartTestReleasedEvent(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_TEST_CONFIRMATION, (EventParam_t){.ui32 = UL_TEST_FAULT});
}

static void onCancelTestReleasedEvent(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_TEST_SELECTION_FORM, (EventParam_t){.ptr = NULL});
}

// ---- Disparador del Toggle de Resistencia ----
static void onResistanceToggleReleasedEvent(gfx_Button *btn) {
    g_bIsHighResistance = !g_bIsHighResistance; // Invertir estado
    
    if (g_bIsHighResistance) {
        btn->label = "RESISTENCIA: ALTA";
        btn->style = STYLE_DANGER; // Cambia de color para llamar la atención
    } else {
        btn->label = "RESISTENCIA: BAJA";
        btn->style = STYLE_SECONDARY; // Color neutral
    }
    
    btn->bIsDirty = true;
    onGenericBtnRelease(btn);

    // Enviar evento para actualizar la configuración en el backend
    // Asumiendo que agregaste bIsHighResistance a ul_fault_current_test_s
    Event_Post(EVT_SYS_FAULT_CFG_SUBMIT_IS_HIGH_RESISTENCE, (EventParam_t){.bool_ = g_bIsHighResistance});
}

// ---- Disparadores del Teclado Numérico ----

static void onCaliberTouchRelease(gfx_TouchArea *area) {
    ul_fault_current_test_s cfg = ExperimentCfg_getCurrFaultCfg();
	snprintf(curCaliber, sizeof(curCaliber), "%s", cfg.pcCaliber);
    Event_Post(EVT_SYS_NUMPAD_MOD_FAULT_CFG_CALIBER, (EventParam_t){.str = curCaliber});
}

static void onCurrentTouchRelease(gfx_TouchArea *area) {
    ul_fault_current_test_s cfg = ExperimentCfg_getCurrFaultCfg();
    Event_Post(EVT_SYS_NUMPAD_MOD_FAULT_CFG_CURRENT, (EventParam_t){.f32 = cfg.f32TargetCurrent});
}

static void onDurationTouchRelease(gfx_TouchArea *area) {
    ul_fault_current_test_s cfg = ExperimentCfg_getCurrFaultCfg();
    Event_Post(EVT_SYS_NUMPAD_MOD_FAULT_CFG_DURATION, (EventParam_t){.f32 = cfg.ui16Duration});
}

// ---- Actualizaciones Visuales desde Eventos CAN/Sistema ----

static void onFaultCaliberChanged(EventParam_t arg) {
	snprintf(caliberValBuffer, sizeof(caliberValBuffer), "%s", arg.str);
    setCaliberValueData.bIsDirty = true;
}

static void onFaultCurrentChanged(EventParam_t arg) {
    sprintf(currentValBuffer, "%.1f", arg.f32);
    setCurrentValueData.bIsDirty = true;
}

static void onFaultDurationChanged(EventParam_t arg) {
    sprintf(testDurationBuffer, "%.0f", arg.f32);
    testDurationValueData.bIsDirty = true;
}

// ========================================================
// INICIALIZACIÓN DE LA UI
// ========================================================

void initFaultTestConfigForm(void) {
    g_sFaultTestConfigCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;

    formTitleData = (gfx_Label) {
        .name = "formTitleData",
        .text = "PRUEBAS",
        .pos.x = 110, .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
    };
    formTitleWidget.eWidgetType = WD_TYPE_LABEL; formTitleWidget.pvWidget = &formTitleData;

    formSubtitleData = (gfx_Label){
        .name = "formSubtitle",
        .text = "CONFIGURACION: FAULT CURRENT",
        .pos.x = 20, .pos.y = 100,
        .typo = TYPO_H3, .style = STYLE_SECONDARY,
        .isVisible = true,
        .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER),
    };
    formSubtitleWidget.eWidgetType = WD_TYPE_LABEL; formSubtitleWidget.pvWidget = &formSubtitleData;

    // --- CÁLCULO DE GEOMETRÍA A 3 COLUMNAS ---
    uint16_t colWidth = LCD_WIDTH / 3.0f;
    uint16_t boxWidth = colWidth * 0.85f; 
    uint16_t boxHeight = 100;
    
    float box1_x = (colWidth - boxWidth) / 2.0f;
    float box2_x = colWidth + (colWidth - boxWidth) / 2.0f;
    float box3_x = (colWidth * 2.0f) + (colWidth - boxWidth) / 2.0f;
    float box_y = formSubtitleData.pos.y + 65;

    // ==========================================
    // CAJA 1: CALIBRE DEL CABLE
    // ==========================================
    setCaliberBoxData = (gfx_Rectangle){
        .pos.x = box1_x, .pos.y = box_y, .dim.width = boxWidth, .dim.height = boxHeight,
        .round = 6, .color = g_pCurrentTheme->palette.surface,
    };
    setCaliberBoxWidget.eWidgetType = WD_TYPE_RECT; setCaliberBoxWidget.pvWidget = &setCaliberBoxData;

    caliberTouchAreaData = (gfx_TouchArea) {
        .pos = setCaliberBoxData.pos, .size = setCaliberBoxData.dim, .onAreaTouchRelease = onCaliberTouchRelease,
    };
    gfx_initRegTouch((void *)&caliberTouchAreaData, WD_TYPE_TOUCH_AREA);
    caliberTouchWidget.eWidgetType = WD_TYPE_TOUCH_AREA; caliberTouchWidget.pvWidget = &caliberTouchAreaData;

    setCaliberLabelData = (gfx_Label) {
        .text = "CALIBRE [AWG/mm2]",
        .pos.x = setCaliberBoxData.pos.x + boxWidth / 2.0, .pos.y = box_y - 15,
        .typo = TYPO_CAPTION, .style = STYLE_TEXT_MUTED, .isVisible = true, .alignment = ALIGN_HCENTER,
    };
    setCaliberLabelWidget.eWidgetType = WD_TYPE_LABEL; setCaliberLabelWidget.pvWidget = &setCaliberLabelData;

    setCaliberValueData = (gfx_Label) {
        .text = caliberValBuffer,
        .pos.x = setCaliberBoxData.pos.x + boxWidth / 2, .pos.y = setCaliberBoxData.pos.y + boxHeight / 2,
        .typo = TYPO_H1, .style = STYLE_PRIMARY, .isVisible = true, .alignment = ALIGN_CENTER,
    };
    setCaliberValueWidget.eWidgetType = WD_TYPE_LABEL; setCaliberValueWidget.pvWidget = &setCaliberValueData;

    // ==========================================
    // CAJA 2: CORRIENTE OBJETIVO
    // ==========================================
    setCurrentBoxData = (gfx_Rectangle){
        .pos.x = box2_x, .pos.y = box_y, .dim.width = boxWidth, .dim.height = boxHeight,
        .round = 6, .color = g_pCurrentTheme->palette.surface,
    };
    setCurrentBoxWidget.eWidgetType = WD_TYPE_RECT; setCurrentBoxWidget.pvWidget = &setCurrentBoxData;

    currentTouchAreaData = (gfx_TouchArea) {
        .pos = setCurrentBoxData.pos, .size = setCurrentBoxData.dim, .onAreaTouchRelease = onCurrentTouchRelease,
    };
    gfx_initRegTouch((void *)&currentTouchAreaData, WD_TYPE_TOUCH_AREA);
    currentTouchWidget.eWidgetType = WD_TYPE_TOUCH_AREA; currentTouchWidget.pvWidget = &currentTouchAreaData;

    setCurrentLabelData = (gfx_Label) {
        .text = "CORRIENTE OBJ. [A]",
        .pos.x = setCurrentBoxData.pos.x + boxWidth / 2.0, .pos.y = box_y - 15,
        .typo = TYPO_CAPTION, .style = STYLE_TEXT_MUTED, .isVisible = true, .alignment = ALIGN_HCENTER,
    };
    setCurrentLabelWidget.eWidgetType = WD_TYPE_LABEL; setCurrentLabelWidget.pvWidget = &setCurrentLabelData;

    setCurrentValueData = (gfx_Label) {
        .text = currentValBuffer,
        .pos.x = setCurrentBoxData.pos.x + boxWidth / 2, .pos.y = setCurrentBoxData.pos.y + boxHeight / 2,
        .typo = TYPO_H1, .style = STYLE_PRIMARY, .isVisible = true, .alignment = ALIGN_CENTER,
    };
    setCurrentValueWidget.eWidgetType = WD_TYPE_LABEL; setCurrentValueWidget.pvWidget = &setCurrentValueData;

    // ==========================================
    // CAJA 3: DURACIÓN DEL ENSAYO
    // ==========================================
    testDurationBoxData = (gfx_Rectangle){
        .pos.x = box3_x, .pos.y = box_y, .dim.width = boxWidth, .dim.height = boxHeight,
        .round = 6, .color = g_pCurrentTheme->palette.surface,
    };
    testDurationBoxWidget.eWidgetType = WD_TYPE_RECT; testDurationBoxWidget.pvWidget = &testDurationBoxData;

    durationTouchAreaData = (gfx_TouchArea) {
        .pos = testDurationBoxData.pos, .size = testDurationBoxData.dim, .onAreaTouchRelease = onDurationTouchRelease,
    };
    gfx_initRegTouch((void *)&durationTouchAreaData, WD_TYPE_TOUCH_AREA);
    durationTouchWidget.eWidgetType = WD_TYPE_TOUCH_AREA; durationTouchWidget.pvWidget = &durationTouchAreaData;

    testDurationLabelData = (gfx_Label) {
        .text = "DURACION [s]",
        .pos.x = testDurationBoxData.pos.x + boxWidth / 2, .pos.y = box_y - 15,
        .typo = TYPO_CAPTION, .style = STYLE_TEXT_MUTED, .isVisible = true, .alignment = ALIGN_HCENTER,
    };
    testDurationLabelWidget.eWidgetType = WD_TYPE_LABEL; testDurationLabelWidget.pvWidget = &testDurationLabelData;

    testDurationValueData = (gfx_Label) {
        .text = testDurationBuffer,
        .pos.x = testDurationBoxData.pos.x + boxWidth / 2, .pos.y = testDurationBoxData.pos.y + boxHeight / 2,
        .typo = TYPO_H1, .style = STYLE_TEXT_MAIN, .isVisible = true, .alignment = ALIGN_CENTER,
    };
    testDurationValueWidget.eWidgetType = WD_TYPE_LABEL; testDurationValueWidget.pvWidget = &testDurationValueData;

    // ==========================================
    // NUEVO: BOTÓN TOGGLE DE RESISTENCIA
    // ==========================================
    uint16_t toggleWidth = LCD_WIDTH / 3.5f;
    resistanceToggleBtnData = (gfx_Button){
        .label = g_bIsHighResistance ? "RESISTENCIA: ALTA" : "RESISTENCIA: BAJA",
        .pos.x = LCD_WIDTH / 2.0f - toggleWidth / 2.0f, 
        .pos.y = box_y + boxHeight + 10, // Posicionado justo debajo de las cajas
        .size.width = toggleWidth, 
        .size.height = 45,
        .borderWidth = 2, 
        .radius = 5, 
        .style = g_bIsHighResistance ? STYLE_DANGER : STYLE_SECONDARY, 
        .typo = TYPO_MONO,
        .bIsVisible = true, 
        .state = BTN_STATE_NORMAL,
        .onPressed = onGenericBtnPressed, 
        .onRelease = onResistanceToggleReleasedEvent,
    };
    gfx_initRegTouch((void *)&resistanceToggleBtnData, WD_TYPE_BUTTON);
    resistanceToggleBtnWidget.eWidgetType = WD_TYPE_BUTTON; 
    resistanceToggleBtnWidget.pvWidget = &resistanceToggleBtnData;

    // ==========================================
    // BOTONES INFERIORES
    // ==========================================
    cancelConfigData = (gfx_Button){
        .label = "CANCELAR",
        .pos.x = LCD_WIDTH / 2 - LCD_WIDTH / 4.0 - 10, .pos.y = LCD_HEIGHT - 145,
        .size.width = LCD_WIDTH / 4.0, .size.height = 70,
        .borderWidth = 3, .radius = 5, .style = STYLE_DEFAULT, .typo = TYPO_MONO_BOLD,
        .bIsVisible = true, .state = BTN_STATE_NORMAL,
        .onPressed = onGenericBtnPressed, .onRelease = onCancelTestReleasedEvent,
    };
    gfx_initRegTouch((void *)&cancelConfigData, WD_TYPE_BUTTON);
    cancelConfigWidget.eWidgetType = WD_TYPE_BUTTON; cancelConfigWidget.pvWidget = &cancelConfigData;

    startTestData = (gfx_Button){
        .label = "INICIAR ENSAYO",
        .pos.x = LCD_WIDTH / 2 + 10, .pos.y = LCD_HEIGHT - 145,
        .size.width = LCD_WIDTH / 4.0, .size.height = 70,
        .borderWidth = 3, .radius = 5, .style = STYLE_SUCCESS, .typo = TYPO_MONO_BOLD,
        .bIsVisible = true, .state = BTN_STATE_NORMAL,
        .onPressed = onGenericBtnPressed, .onRelease = onStartTestReleasedEvent,
    };
    gfx_initRegTouch((void *)&startTestData, WD_TYPE_BUTTON);
    startTestWidget.eWidgetType = WD_TYPE_BUTTON; startTestWidget.pvWidget = &startTestData;
    
    // --- ENSAMBLE DEL CANVAS ---
    useFullHeader(&g_sFaultTestConfigCanvas);
    useNavigationButtons(&g_sFaultTestConfigCanvas);

    canvasInsertAtTop(&g_sFaultTestConfigCanvas.psWidgets, &formTitleWidget);
    canvasInsertAtTop(&g_sFaultTestConfigCanvas.psWidgets, &formSubtitleWidget);
    canvasInsertAtTop(&g_sFaultTestConfigCanvas.psWidgets, &startTestWidget);
    canvasInsertAtTop(&g_sFaultTestConfigCanvas.psWidgets, &cancelConfigWidget);
    canvasInsertAtTop(&g_sFaultTestConfigCanvas.psWidgets, &resistanceToggleBtnWidget); // <-- Insertado en el canvas

    // Insertar Caja 1 (Calibre)
    canvasInsertAtTop(&g_sFaultTestConfigCanvas.psWidgets, &setCaliberLabelWidget);
    canvasInsertAtTop(&g_sFaultTestConfigCanvas.psWidgets, &setCaliberBoxWidget);
    canvasInsertAtTop(&g_sFaultTestConfigCanvas.psWidgets, &setCaliberValueWidget);
    canvasInsertAtTop(&g_sFaultTestConfigCanvas.psWidgets, &caliberTouchWidget);

    // Insertar Caja 2 (Corriente)
    canvasInsertAtTop(&g_sFaultTestConfigCanvas.psWidgets, &setCurrentLabelWidget);
    canvasInsertAtTop(&g_sFaultTestConfigCanvas.psWidgets, &setCurrentBoxWidget);
    canvasInsertAtTop(&g_sFaultTestConfigCanvas.psWidgets, &setCurrentValueWidget);
    canvasInsertAtTop(&g_sFaultTestConfigCanvas.psWidgets, &currentTouchWidget);

    // Insertar Caja 3 (Duración)
    canvasInsertAtTop(&g_sFaultTestConfigCanvas.psWidgets, &testDurationLabelWidget);
    canvasInsertAtTop(&g_sFaultTestConfigCanvas.psWidgets, &testDurationBoxWidget);
    canvasInsertAtTop(&g_sFaultTestConfigCanvas.psWidgets, &testDurationValueWidget);
    canvasInsertAtTop(&g_sFaultTestConfigCanvas.psWidgets, &durationTouchWidget);

    // Suscripciones de eventos
    Event_Subscribe(EVT_SYS_FAULT_CFG_CALIBER, (EventHandler_fn)onFaultCaliberChanged);
    Event_Subscribe(EVT_SYS_FAULT_CFG_DURATION, (EventHandler_fn)onFaultDurationChanged);
    Event_Subscribe(EVT_SYS_FAULT_CFG_CURRENT, (EventHandler_fn)onFaultCurrentChanged);

    g_i16FaultConfigFormIndex = FormManager_AddForm(&g_sFaultTestConfigCanvas);
}

