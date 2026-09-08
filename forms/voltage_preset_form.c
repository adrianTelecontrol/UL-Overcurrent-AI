#include <stdlib.h>
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

#include "test_confirmation_form.h"
#include "common_widgets.h"

#include "voltage_preset_form.h"

int16_t g_i16VoltagePresetIndex;

static gfx_Canvas g_sVoltagePresetCanvas;

// ========================================================
// CONTENEDORES DE WIDGETS
// ========================================================
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget formSubtitleWidget;
static gfx_GenericWidget startTestWidget;
static gfx_GenericWidget cancelConfigWidget;
static gfx_GenericWidget skipBtnWidget;
// Caja de Calibre
static gfx_GenericWidget setVoltageLabelWidget;
static gfx_GenericWidget setVoltageBoxWidget;
static gfx_GenericWidget setVoltageValueWidget;
static gfx_GenericWidget voltageTouchWidget;

// ========================================================
// DATOS DE WIDGETS
// ========================================================
static gfx_Label formTitleData;
static gfx_Label formSubtitleData;
static gfx_Button startTestData;
static gfx_Button returnBtnData;

static gfx_Label setVoltageLabelData;
static gfx_Rectangle setVoltageBoxData;
static gfx_Label setVoltageValueData;
static gfx_TouchArea voltageTouchAreaData;

static gfx_Button skipBtnData;

// Buffers de texto y estado
static char voltageValBuffer[8] = "0.0";
static char curVoltage[12] = "0.0";
static bool g_bIsHighResistance = false;

// ========================================================
// CALLBACKS DE INTERACCIÓN
// ========================================================

static void onStartTestReleasedEvent(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    // Asumiendo que tienes un enum UL_TEST_PROFILE o similar para el ensayo C
    Event_Post(EVT_SYS_SHOW_TEST_CONFIRMATION, (EventParam_t){.ui32 = UL_TEST_SEQUENCE});
}

static void onCancelTestReleasedEvent(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    // Regresamos a la pantalla de previsualización del perfil
    Event_Post(EVT_SYS_SHOW_SEQUENCE_PREVIEW_FORM, (EventParam_t){.ptr = NULL});
}

// ---- Disparador del Toggle de Resistencia ----
static void onSkipBtnReleasedEvent(gfx_Button *btn) {
    onGenericBtnRelease(btn);


    // Opcional: Event_Post(EVT_SYS_MOD_PROFILE_CFG_RESISTANCE, (EventParam_t){.ui32 = g_bIsHighResistance});
}

// ---- Disparador del Teclado Numérico ----
static void onCaliberTouchRelease(gfx_TouchArea *area) {
    // Aquí puedes extraer el valor de tu estructura si la tienes separada
    // ul_profile_test_s cfg = ExperimentCfg_getCurrProfileCfg();
    // Event_Post(EVT_SYS_NUMPAD_MOD_PROFILE_CFG_CALIBER, (EventParam_t){.f32 = cfg.f32Caliber});
    const ul_sequence_profile_t *cfg = ExperimentCfg_getCurrSequenceCfg();
	snprintf(curVoltage, sizeof(curVoltage), "%s", cfg->pcCaliber);
    Event_Post(EVT_SYS_NUMPAD_MOD_PROFILE_CFG_CALIBER, (EventParam_t){.str = curVoltage});
}

// ---- Actualizaciones Visuales desde Eventos ----
static void onProfileCaliberChanged(EventParam_t arg) {
	snprintf(voltageValBuffer, sizeof(voltageValBuffer), "%s", arg.str);
    setVoltageValueData.bIsDirty = true;
}

// ========================================================
// INICIALIZACIÓN DE LA UI
// ========================================================

void initVoltagePresetForm(void) {
    g_sVoltagePresetCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;

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
        .text = "CONFIGURACION: PRESET DE VOLTAJE",
        .pos.x = 20, .pos.y = 100,
        .typo = TYPO_H3, .style = STYLE_SECONDARY,
        .isVisible = true,
        .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER),
    };
    formSubtitleWidget.eWidgetType = WD_TYPE_LABEL; formSubtitleWidget.pvWidget = &formSubtitleData;

    // --- GEOMETRÍA CENTRADA ---
    uint16_t boxWidth = LCD_WIDTH / 3.0f * 0.85f; 
    uint16_t boxHeight = 100;
    
    float center_x = (LCD_WIDTH - boxWidth) / 2.0f;
    float box_y = 200; // Más abajo para usar el centro de la pantalla

    // ==========================================
    // CAJA: CALIBRE DEL CABLE
    // ==========================================
    setVoltageBoxData = (gfx_Rectangle){
        .pos.x = center_x, .pos.y = box_y, .dim.width = boxWidth, .dim.height = boxHeight,
        .round = 6, .color = g_pCurrentTheme->palette.surface,
    };
    setVoltageBoxWidget.eWidgetType = WD_TYPE_RECT; setVoltageBoxWidget.pvWidget = &setVoltageBoxData;

    voltageTouchAreaData = (gfx_TouchArea) {
        .pos = setVoltageBoxData.pos, .size = setVoltageBoxData.dim, .onAreaTouchRelease = onCaliberTouchRelease,
    };
    gfx_initRegTouch((void *)&voltageTouchAreaData, WD_TYPE_TOUCH_AREA);
    voltageTouchWidget.eWidgetType = WD_TYPE_TOUCH_AREA; voltageTouchWidget.pvWidget = &voltageTouchAreaData;

    setVoltageLabelData = (gfx_Label) {
        .text = "VOLTAGE TX SECUNDARIO",
        .pos.x = setVoltageBoxData.pos.x + boxWidth / 2.0, .pos.y = box_y - 15,
        .typo = TYPO_H3, .style = STYLE_TEXT_MUTED, .isVisible = true, .alignment = ALIGN_HCENTER,
    };
    setVoltageLabelWidget.eWidgetType = WD_TYPE_LABEL; setVoltageLabelWidget.pvWidget = &setVoltageLabelData;

    setVoltageValueData = (gfx_Label) {
        .text = voltageValBuffer,
        .pos.x = setVoltageBoxData.pos.x + boxWidth / 2, .pos.y = setVoltageBoxData.pos.y + boxHeight / 2,
        .typo = TYPO_H1, .style = STYLE_PRIMARY, .isVisible = true, .alignment = ALIGN_CENTER,
    };
    setVoltageValueWidget.eWidgetType = WD_TYPE_LABEL; setVoltageValueWidget.pvWidget = &setVoltageValueData;

    // ==========================================
    // BOTÓN TOGGLE DE RESISTENCIA
    // ==========================================
    uint16_t toggleWidth = LCD_WIDTH / 3.5f;
    skipBtnData = (gfx_Button){
        .label = "IGNORAR",
        .pos.x = LCD_WIDTH / 2.0f - toggleWidth / 2.0f, 
        .pos.y = box_y + boxHeight + 25, // Posicionado justo debajo de las cajas
        .size.width = toggleWidth, 
        .size.height = 45,
        .borderWidth = 2, 
        .radius = 5, 
        .style = STYLE_DANGER, 
        .typo = TYPO_MONO,
        .bIsVisible = true, 
        .state = BTN_STATE_NORMAL,
        .onPressed = onGenericBtnPressed, 
        .onRelease = onSkipBtnReleasedEvent,
    };
    gfx_initRegTouch((void *)&skipBtnData, WD_TYPE_BUTTON);
    skipBtnWidget.eWidgetType = WD_TYPE_BUTTON; 
    skipBtnWidget.pvWidget = &skipBtnData;

    // ==========================================
    // BOTONES INFERIORES
    // ==========================================
    returnBtnData = (gfx_Button){
        .label = "ATRAS",
        .pos.x = LCD_WIDTH / 2 - LCD_WIDTH / 4.0 - 10, .pos.y = LCD_HEIGHT - 90,
        .size.width = LCD_WIDTH / 4.0, .size.height = 70,
        .borderWidth = 3, .radius = 5, .style = STYLE_DEFAULT, .typo = TYPO_MONO_BOLD,
        .bIsVisible = true, .state = BTN_STATE_NORMAL,
        .onPressed = onGenericBtnPressed, .onRelease = onCancelTestReleasedEvent,
    };
    gfx_initRegTouch((void *)&returnBtnData, WD_TYPE_BUTTON);
    cancelConfigWidget.eWidgetType = WD_TYPE_BUTTON; cancelConfigWidget.pvWidget = &returnBtnData;

    startTestData = (gfx_Button){
        .label = "CONTINUAR",
        .pos.x = LCD_WIDTH / 2 + 10, 
		.pos.y = LCD_HEIGHT - 90,
        .size.width = LCD_WIDTH / 4.0, 
		.size.height = 70,
        .borderWidth = 3, 
		.radius = 5, 
		.style = STYLE_SUCCESS, 
		.typo = TYPO_MONO_BOLD,
        .bIsVisible = true, 
		.state = BTN_STATE_NORMAL,
        .onPressed = onGenericBtnPressed, 
		.onRelease = onStartTestReleasedEvent,
    };
    gfx_initRegTouch((void *)&startTestData, WD_TYPE_BUTTON);
    startTestWidget.eWidgetType = WD_TYPE_BUTTON; startTestWidget.pvWidget = &startTestData;
    
    // --- ENSAMBLE DEL CANVAS ---
    useFullHeader(&g_sVoltagePresetCanvas);

    canvasInsertAtTop(&g_sVoltagePresetCanvas.psWidgets, &formTitleWidget);
    canvasInsertAtTop(&g_sVoltagePresetCanvas.psWidgets, &formSubtitleWidget);
    canvasInsertAtTop(&g_sVoltagePresetCanvas.psWidgets, &startTestWidget);
    canvasInsertAtTop(&g_sVoltagePresetCanvas.psWidgets, &cancelConfigWidget);
    // canvasInsertAtTop(&g_sVoltagePresetCanvas.psWidgets, &skipBtnWidget);

    // Insertar Caja (Calibre)
    canvasInsertAtTop(&g_sVoltagePresetCanvas.psWidgets, &setVoltageLabelWidget);
    canvasInsertAtTop(&g_sVoltagePresetCanvas.psWidgets, &setVoltageBoxWidget);
    canvasInsertAtTop(&g_sVoltagePresetCanvas.psWidgets, &setVoltageValueWidget);
    canvasInsertAtTop(&g_sVoltagePresetCanvas.psWidgets, &voltageTouchWidget);

    // Suscripciones de eventos
    Event_Subscribe(EVT_SYS_PROFILE_CFG_CALIBER, (EventHandler_fn)onProfileCaliberChanged);

    g_i16VoltagePresetIndex = FormManager_AddForm(&g_sVoltagePresetCanvas);
}