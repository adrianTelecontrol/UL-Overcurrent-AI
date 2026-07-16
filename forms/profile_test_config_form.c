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

// Asumiendo que creas su respectivo .h
#include "profile_test_config_form.h"

int16_t g_i16ProfileConfigFormIndex;

static gfx_Canvas g_sProfileTestConfigCanvas;

// ========================================================
// CONTENEDORES DE WIDGETS
// ========================================================
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget formSubtitleWidget;
static gfx_GenericWidget startTestWidget;
static gfx_GenericWidget cancelConfigWidget;

// Caja de Calibre
static gfx_GenericWidget setCaliberLabelWidget;
static gfx_GenericWidget setCaliberBoxWidget;
static gfx_GenericWidget setCaliberValueWidget;
static gfx_GenericWidget caliberTouchWidget;

// Botón Toggle de Resistencia
static gfx_GenericWidget resistanceToggleBtnWidget;

// ========================================================
// DATOS DE WIDGETS
// ========================================================
static gfx_Label formTitleData;
static gfx_Label formSubtitleData;
static gfx_Button startTestData;
static gfx_Button cancelConfigData;

static gfx_Label setCaliberLabelData;
static gfx_Rectangle setCaliberBoxData;
static gfx_Label setCaliberValueData;
static gfx_TouchArea caliberTouchAreaData;

static gfx_Button resistanceToggleBtnData;

// Buffers de texto y estado
static char caliberValBuffer[8] = "00"; // Valor por defecto
static char curCaliber[12] = "00";
static bool g_bIsHighResistance = false; // Estado del Toggle

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
static void onResistanceToggleReleasedEvent(gfx_Button *btn) {
    g_bIsHighResistance = !g_bIsHighResistance; // Invertir estado
    
    if (g_bIsHighResistance) {
        btn->label = "RESISTENCIA: ALTA";
        btn->style = STYLE_DANGER; // Cambia de color
    } else {
        btn->label = "RESISTENCIA: BAJA";
        btn->style = STYLE_SECONDARY; // Color neutral
    }
    
    btn->bIsDirty = true;
    onGenericBtnRelease(btn);

    // Opcional: Event_Post(EVT_SYS_MOD_PROFILE_CFG_RESISTANCE, (EventParam_t){.ui32 = g_bIsHighResistance});
}

// ---- Disparador del Teclado Numérico ----
static void onCaliberTouchRelease(gfx_TouchArea *area) {
    // Aquí puedes extraer el valor de tu estructura si la tienes separada
    // ul_profile_test_s cfg = ExperimentCfg_getCurrProfileCfg();
    // Event_Post(EVT_SYS_NUMPAD_MOD_PROFILE_CFG_CALIBER, (EventParam_t){.f32 = cfg.f32Caliber});
    const ul_sequence_profile_t *cfg = ExperimentCfg_getCurrSequenceCfg();
	snprintf(curCaliber, sizeof(curCaliber), "%s", cfg->pcCaliber);
    Event_Post(EVT_SYS_NUMPAD_MOD_PROFILE_CFG_CALIBER, (EventParam_t){.str = curCaliber});
}

// ---- Actualizaciones Visuales desde Eventos ----
static void onProfileCaliberChanged(EventParam_t arg) {
	snprintf(caliberValBuffer, sizeof(caliberValBuffer), "%s", arg.str);
    setCaliberValueData.bIsDirty = true;
}

// ========================================================
// INICIALIZACIÓN DE LA UI
// ========================================================

void initProfileTestConfigForm(void) {
    g_sProfileTestConfigCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;

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
        .text = "CONFIGURACION: PERFIL DE SEGMENTOS",
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
    float box_y = 170; // Más abajo para usar el centro de la pantalla

    // ==========================================
    // CAJA: CALIBRE DEL CABLE
    // ==========================================
    setCaliberBoxData = (gfx_Rectangle){
        .pos.x = center_x, .pos.y = box_y, .dim.width = boxWidth, .dim.height = boxHeight,
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
    // BOTÓN TOGGLE DE RESISTENCIA
    // ==========================================
    uint16_t toggleWidth = LCD_WIDTH / 3.5f;
    resistanceToggleBtnData = (gfx_Button){
        .label = g_bIsHighResistance ? "RESISTENCIA: ALTA" : "RESISTENCIA: BAJA",
        .pos.x = LCD_WIDTH / 2.0f - toggleWidth / 2.0f, 
        .pos.y = box_y + boxHeight + 25, // Posicionado justo debajo de las cajas
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
        .label = "ATRAS",
        .pos.x = LCD_WIDTH / 2 - LCD_WIDTH / 4.0 - 10, .pos.y = LCD_HEIGHT - 90,
        .size.width = LCD_WIDTH / 4.0, .size.height = 70,
        .borderWidth = 3, .radius = 5, .style = STYLE_DEFAULT, .typo = TYPO_MONO_BOLD,
        .bIsVisible = true, .state = BTN_STATE_NORMAL,
        .onPressed = onGenericBtnPressed, .onRelease = onCancelTestReleasedEvent,
    };
    gfx_initRegTouch((void *)&cancelConfigData, WD_TYPE_BUTTON);
    cancelConfigWidget.eWidgetType = WD_TYPE_BUTTON; cancelConfigWidget.pvWidget = &cancelConfigData;

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
    useFullHeader(&g_sProfileTestConfigCanvas);

    canvasInsertAtTop(&g_sProfileTestConfigCanvas.psWidgets, &formTitleWidget);
    canvasInsertAtTop(&g_sProfileTestConfigCanvas.psWidgets, &formSubtitleWidget);
    canvasInsertAtTop(&g_sProfileTestConfigCanvas.psWidgets, &startTestWidget);
    canvasInsertAtTop(&g_sProfileTestConfigCanvas.psWidgets, &cancelConfigWidget);
    canvasInsertAtTop(&g_sProfileTestConfigCanvas.psWidgets, &resistanceToggleBtnWidget);

    // Insertar Caja (Calibre)
    canvasInsertAtTop(&g_sProfileTestConfigCanvas.psWidgets, &setCaliberLabelWidget);
    canvasInsertAtTop(&g_sProfileTestConfigCanvas.psWidgets, &setCaliberBoxWidget);
    canvasInsertAtTop(&g_sProfileTestConfigCanvas.psWidgets, &setCaliberValueWidget);
    canvasInsertAtTop(&g_sProfileTestConfigCanvas.psWidgets, &caliberTouchWidget);

    // Suscripciones de eventos
    Event_Subscribe(EVT_SYS_PROFILE_CFG_CALIBER, (EventHandler_fn)onProfileCaliberChanged);

    g_i16ProfileConfigFormIndex = FormManager_AddForm(&g_sProfileTestConfigCanvas);
}