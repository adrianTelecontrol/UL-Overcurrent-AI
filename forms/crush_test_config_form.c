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
#include "inst_can_buffer.h"

#include "test_confirmation_form.h"
#include "common_widgets.h"
#include "crush_test_config_form.h"

int16_t g_i16CrushTestConfigIndex;
int16_t g_i16CrushPidConfigIndex;

static gfx_Canvas g_sCrushTestConfigCanvas;
static gfx_Canvas g_sCrushPidConfigCanvas;

// ========================================================
// CONTENEDORES DE WIDGETS
// ========================================================
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget formSubtitleWidget;
static gfx_GenericWidget startTestWidget;
static gfx_GenericWidget cancelConfigWidget;
static gfx_GenericWidget advancedConfigWidget;

// Etiquetas de Título de Caja
static gfx_GenericWidget setCaliberLabelWidget;
static gfx_GenericWidget setTempLabelWidget;
static gfx_GenericWidget testDurationLabelWidget;

// Fondos de Caja
static gfx_GenericWidget setCaliberBoxWidget;
static gfx_GenericWidget setTempBoxWidget;
static gfx_GenericWidget testDurationBoxWidget;

// Valores Dinámicos
static gfx_GenericWidget setCaliberValueWidget;
static gfx_GenericWidget setTempValueWidget;
static gfx_GenericWidget testDurationValueWidget;

// Áreas Táctiles
static gfx_GenericWidget caliberTouchWidget;
static gfx_GenericWidget tempTouchWidget;
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
static gfx_Button advancedConfigData;

static gfx_Label setCaliberLabelData;
static gfx_Label setTempLabelData;
static gfx_Label testDurationLabelData;

static gfx_Rectangle setCaliberBoxData;
static gfx_Rectangle setTempBoxData;
static gfx_Rectangle testDurationBoxData;

static gfx_Label setCaliberValueData;
static gfx_Label setTempValueData;
static gfx_Label testDurationValueData;

static gfx_TouchArea caliberTouchAreaData;
static gfx_TouchArea tempTouchAreaData;
static gfx_TouchArea durationTouchAreaData;

static gfx_Button resistanceToggleBtnData;

// Buffers de texto y estado
static char caliberValBuffer[8] = "0"; 
static char tempValBuffer[8] = "450.0";
static char testDurationBuffer[6] = "4.0";
static char curCaliber[12] = "00";
static bool g_bIsHighResistance = false; // Estado del Toggle

// Advanced PID form
static gfx_GenericWidget pidTitleWidget;
static gfx_GenericWidget pidSubtitleWidget;
static gfx_GenericWidget pidBoxWidgets[3];
static gfx_GenericWidget pidNameWidgets[3];
static gfx_GenericWidget pidValueWidgets[3];
static gfx_GenericWidget pidTouchWidgets[3];
static gfx_GenericWidget pidStatusWidget;
static gfx_GenericWidget pidReturnWidget;
static gfx_GenericWidget pidApplyWidget;
static gfx_GenericWidget pidDefaultsWidget;

static gfx_Label pidTitleData;
static gfx_Label pidSubtitleData;
static gfx_Rectangle pidBoxData[3];
static gfx_Label pidNameData[3];
static gfx_Label pidValueData[3];
static gfx_TouchArea pidTouchData[3];
static gfx_Label pidStatusData;
static gfx_Button pidReturnData;
static gfx_Button pidApplyData;
static gfx_Button pidDefaultsData;

static char pidValueBuffers[3][16] = {"--", "--", "--"};
static char pidStatusBuffer[40] = "SOLICITANDO VALORES...";
static InstTempPidValues_t g_sTempPidValues = {0};
static bool g_bPidValueReceived[3] = {false, false, false};
static bool g_bPidValueEdited[3] = {false, false, false};

static const uint8_t g_ui8VisiblePidIndexes[] = {
    0U,
#if INST_TEMP_PID_ENABLE_INTEGRAL
    1U,
#endif
    2U,
};

#define PID_VALUE_COUNT 3U
#define PID_VISIBLE_COUNT ((uint8_t)(sizeof(g_ui8VisiblePidIndexes) / \
                                    sizeof(g_ui8VisiblePidIndexes[0])))

static void initCrushPidConfigForm(void);
static void onAdvancedConfigReleasedEvent(gfx_Button *btn);

// ========================================================
// CALLBACKS DE INTERACCIÓN
// ========================================================

static void onStartTestReleasedEvent(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_TEST_CONFIRMATION, (EventParam_t){.ui32 = UL_TEST_CRUSH});
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
    Event_Post(EVT_SYS_CRUSH_CFG_SUBMIT_IS_HIGH_RESISTENCE, (EventParam_t){.bool_ = g_bIsHighResistance});
}

// ---- Disparadores del Teclado Numérico ----

static void onCaliberTouchRelease(gfx_TouchArea *area) {
    ul_crush_test_s cfg = ExperimentCfg_getCurrCrushCfg();
	snprintf(curCaliber, sizeof(curCaliber), "%s", cfg.pcCaliber);
    Event_Post(EVT_SYS_NUMPAD_MOD_CRUSH_CFG_CALIBER, (EventParam_t){.str = curCaliber});
}

static void onTempTouchRelease(gfx_TouchArea *area) {
    ul_crush_test_s cfg = ExperimentCfg_getCurrCrushCfg();
    Event_Post(EVT_SYS_NUMPAD_MOD_CRUSH_CFG_TEMP, (EventParam_t){.f32 = cfg.f32TargetTemp});
}

static void onDurationTouchRelease(gfx_TouchArea *area) {
    ul_crush_test_s cfg = ExperimentCfg_getCurrCrushCfg();
    Event_Post(EVT_SYS_NUMPAD_MOD_CRUSH_CFG_DURATION, (EventParam_t){.f32 = cfg.ui16Duration});
}

// ---- Actualizaciones Visuales desde Eventos CAN/Sistema ----

static void onCrushCaliberChanged(EventParam_t arg) {
	snprintf(caliberValBuffer, sizeof(caliberValBuffer), "%s", arg.str);
    setCaliberValueData.bIsDirty = true;
}

static void onCrushTempChanged(EventParam_t arg) {
    sprintf(tempValBuffer, "%.1f", arg.f32);
    setTempValueData.bIsDirty = true;
}

static void onCrushDurationChanged(EventParam_t arg) {
    sprintf(testDurationBuffer, "%.0f", arg.f32);
    testDurationValueData.bIsDirty = true;
}

static void updatePidStatus(const char *text, gfx_WidgetStyle_e style) {
    snprintf(pidStatusBuffer, sizeof(pidStatusBuffer), "%s", text);
    pidStatusData.style = style;
    pidStatusData.bIsDirty = true;
}

static bool areRequiredPidValuesReceived(void) {
    if (!g_bPidValueReceived[0] || !g_bPidValueReceived[2]) {
        return false;
    }

#if INST_TEMP_PID_ENABLE_INTEGRAL
    if (!g_bPidValueReceived[1]) {
        return false;
    }
#endif

    return true;
}

static void updatePidValue(uint8_t index, float value, bool fromInstrument) {
    float *values[3] = {
        &g_sTempPidValues.kp,
        &g_sTempPidValues.ki,
        &g_sTempPidValues.kd,
    };

    if (index >= PID_VALUE_COUNT ||
        (fromInstrument && g_bPidValueEdited[index])) {
        return;
    }

    *values[index] = value;
    snprintf(pidValueBuffers[index], sizeof(pidValueBuffers[index]), "%.3f", value);
    g_bPidValueReceived[index] = true;
    if (!fromInstrument) {
        g_bPidValueEdited[index] = true;
        updatePidStatus("CAMBIOS SIN APLICAR", STYLE_WARNING);
    }
    pidValueData[index].bIsDirty = true;

    if (fromInstrument && areRequiredPidValuesReceived()) {
        updatePidStatus("VALORES ACTUALES", STYLE_SUCCESS);
    }
}

static void onPidKpReceived(EventParam_t arg) {
    updatePidValue(0U, arg.f32, true);
}

static void onPidKiReceived(EventParam_t arg) {
    updatePidValue(1U, arg.f32, true);
}

static void onPidKdReceived(EventParam_t arg) {
    updatePidValue(2U, arg.f32, true);
}

static void onPidKpSubmitted(EventParam_t arg) {
    updatePidValue(0U, arg.f32, false);
}

#if INST_TEMP_PID_ENABLE_INTEGRAL
static void onPidKiSubmitted(EventParam_t arg) {
    updatePidValue(1U, arg.f32, false);
}
#endif

static void onPidKdSubmitted(EventParam_t arg) {
    updatePidValue(2U, arg.f32, false);
}

static void onPidTouchRelease(gfx_TouchArea *area) {
    uint8_t index;
    EventID_e events[3] = {
        EVT_SYS_NUMPAD_MOD_CRUSH_PID_KP,
        EVT_SYS_NUMPAD_MOD_CRUSH_PID_KI,
        EVT_SYS_NUMPAD_MOD_CRUSH_PID_KD,
    };
    float values[3] = {
        g_sTempPidValues.kp,
        g_sTempPidValues.ki,
        g_sTempPidValues.kd,
    };

    for (index = 0U; index < PID_VALUE_COUNT; index++) {
        if (area == &pidTouchData[index]) {
            if (!g_bPidValueReceived[index]) {
                updatePidStatus("ESPERE LOS VALORES ACTUALES", STYLE_WARNING);
                return;
            }
            Event_Post(events[index], (EventParam_t){.f32 = values[index]});
            return;
        }
    }
}

static void onPidReturnReleased(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_CRUSH_CONFIG_FORM, (EventParam_t){.ptr = NULL});
}

static void onPidApplyReleased(gfx_Button *btn) {
    onGenericBtnRelease(btn);

    if (!areRequiredPidValuesReceived()) {
        updatePidStatus("FALTAN VALORES PID", STYLE_DANGER);
        return;
    }

    if (InstManager_ApplyTempPidValues(&g_sTempPidValues)) {
        updatePidStatus("APLICANDO...", STYLE_WARNING);
    } else {
        updatePidStatus("COLA CAN OCUPADA", STYLE_DANGER);
    }
}

static void onPidDefaultsReleased(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    if (InstManager_RestoreTempPidDefaults()) {
        updatePidStatus("RESTAURANDO...", STYLE_WARNING);
    } else {
        updatePidStatus("COLA CAN OCUPADA", STYLE_DANGER);
    }
}

static void onPidApplyAck(EventParam_t arg) {
    uint8_t index;

    if (!arg.bool_) {
        updatePidStatus("ERROR AL APLICAR", STYLE_DANGER);
        return;
    }

    for (index = 0U; index < 3U; index++) {
        g_bPidValueEdited[index] = false;
    }
    updatePidStatus("PID APLICADO", STYLE_SUCCESS);
    InstManager_RequestTempPidValues();
}

static void onPidDefaultsAck(EventParam_t arg) {
    uint8_t index;

    if (!arg.bool_) {
        updatePidStatus("ERROR AL RESTAURAR", STYLE_DANGER);
        return;
    }

    for (index = 0U; index < 3U; index++) {
        g_bPidValueEdited[index] = false;
        g_bPidValueReceived[index] = false;
        strcpy(pidValueBuffers[index], "--");
        pidValueData[index].bIsDirty = true;
    }
    updatePidStatus("VALORES RESTAURADOS", STYLE_SUCCESS);
    if (!InstManager_RequestTempPidValues()) {
        updatePidStatus("RESTAURADO; RECARGA PENDIENTE", STYLE_WARNING);
    }
}

static void onPidThemeChanged(EventParam_t arg) {
    uint8_t index;
    (void)arg;

    for (index = 0U; index < 3U; index++) {
        pidBoxData[index].color = g_pCurrentTheme->palette.surface;
        pidBoxData[index].bIsDirty = true;
    }
}

// ========================================================
// INICIALIZACIÓN DE LA UI
// ========================================================

void initCrushTestConfigForm(void) {
    g_sCrushTestConfigCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;

    formTitleData = (gfx_Label) {
        .name = "formTitleData",
        .text = "PRUEBAS",
        .pos.x = 125, .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
    };
    formTitleWidget.eWidgetType = WD_TYPE_LABEL; formTitleWidget.pvWidget = &formTitleData;

    formSubtitleData = (gfx_Label){
        .name = "formSubtitle",
        .text = "CONFIGURACION: CRUSH TEST",
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
        .pos = setCaliberBoxData.pos, 
		.size.width = setCaliberBoxData.dim.width, 
		.size.height = setCaliberBoxData.dim.height, 
		.onAreaTouchRelease = onCaliberTouchRelease,
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
    // CAJA 2: TEMPERATURA OBJETIVO
    // ==========================================
    setTempBoxData = (gfx_Rectangle){
        .pos.x = box2_x, .pos.y = box_y, .dim.width = boxWidth, .dim.height = boxHeight,
        .round = 6, .color = g_pCurrentTheme->palette.surface,
    };
    setTempBoxWidget.eWidgetType = WD_TYPE_RECT; setTempBoxWidget.pvWidget = &setTempBoxData;

    tempTouchAreaData = (gfx_TouchArea) {
        .pos = setTempBoxData.pos, .size = setTempBoxData.dim, .onAreaTouchRelease = onTempTouchRelease,
    };
    gfx_initRegTouch((void *)&tempTouchAreaData, WD_TYPE_TOUCH_AREA);
    tempTouchWidget.eWidgetType = WD_TYPE_TOUCH_AREA; tempTouchWidget.pvWidget = &tempTouchAreaData;

    setTempLabelData = (gfx_Label) {
        .text = "TEMPERATURA OBJ. [C]",
        .pos.x = setTempBoxData.pos.x + boxWidth / 2.0, .pos.y = box_y - 15,
        .typo = TYPO_CAPTION, .style = STYLE_TEXT_MUTED, .isVisible = true, .alignment = ALIGN_HCENTER,
    };
    setTempLabelWidget.eWidgetType = WD_TYPE_LABEL; setTempLabelWidget.pvWidget = &setTempLabelData;

    setTempValueData = (gfx_Label) {
        .text = tempValBuffer,
        .pos.x = setTempBoxData.pos.x + boxWidth / 2, .pos.y = setTempBoxData.pos.y + boxHeight / 2,
        .typo = TYPO_H1, .style = STYLE_PRIMARY, .isVisible = true, .alignment = ALIGN_CENTER,
    };
    setTempValueWidget.eWidgetType = WD_TYPE_LABEL; setTempValueWidget.pvWidget = &setTempValueData;

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

    advancedConfigData = (gfx_Button){
        .label = "AVANZADO",
        .pos.x = LCD_WIDTH - 190, .pos.y = box_y + boxHeight + 10,
        .size.width = 170, .size.height = 45,
        .borderWidth = 2, .radius = 5,
        .style = STYLE_WARNING, .typo = TYPO_MONO_BOLD,
        .bIsVisible = true, .state = BTN_STATE_NORMAL,
        .onPressed = onGenericBtnPressed,
        .onRelease = onAdvancedConfigReleasedEvent,
    };
    gfx_initRegTouch((void *)&advancedConfigData, WD_TYPE_BUTTON);
    advancedConfigWidget.eWidgetType = WD_TYPE_BUTTON;
    advancedConfigWidget.pvWidget = &advancedConfigData;

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
    useFullHeader(&g_sCrushTestConfigCanvas);
    useNavigationButtons(&g_sCrushTestConfigCanvas);

    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &formTitleWidget);
    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &formSubtitleWidget);
    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &startTestWidget);
    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &cancelConfigWidget);
    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &resistanceToggleBtnWidget);
    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &advancedConfigWidget);

    // Insertar Caja 1 (Calibre)
    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &setCaliberLabelWidget);
    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &setCaliberBoxWidget);
    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &setCaliberValueWidget);
    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &caliberTouchWidget);

    // Insertar Caja 2 (Temp)
    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &setTempLabelWidget);
    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &setTempBoxWidget);
    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &setTempValueWidget);
    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &tempTouchWidget);

    // Insertar Caja 3 (Duración)
    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &testDurationLabelWidget);
    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &testDurationBoxWidget);
    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &testDurationValueWidget);
    canvasInsertAtTop(&g_sCrushTestConfigCanvas.psWidgets, &durationTouchWidget);

    // Suscripciones de eventos
    Event_Subscribe(EVT_SYS_CRUSH_CFG_CALIBER, (EventHandler_fn)onCrushCaliberChanged);
    Event_Subscribe(EVT_SYS_CRUSH_CFG_DURATION, (EventHandler_fn)onCrushDurationChanged);
    Event_Subscribe(EVT_SYS_CRUSH_CFG_TEMP, (EventHandler_fn)onCrushTempChanged);

    g_i16CrushTestConfigIndex = FormManager_AddForm(&g_sCrushTestConfigCanvas);
    initCrushPidConfigForm();
}

static void initCrushPidConfigForm(void) {
    static char pidNames[3][3] = {"Kp", "Ki", "Kd"};
    uint16_t boxWidth = 200U;
    uint16_t boxHeight = 95U;
    uint16_t boxY = 165U;
    uint16_t columnSpacing = 240U;
    uint16_t totalWidth = (uint16_t)(PID_VISIBLE_COUNT * boxWidth) +
                          (uint16_t)((PID_VISIBLE_COUNT - 1U) *
                                     (columnSpacing - boxWidth));
    uint16_t firstX = (uint16_t)((LCD_WIDTH - totalWidth) / 2U);
    uint8_t index;

    g_sCrushPidConfigCanvas.ui16BackgroundColor =
        g_pCurrentTheme->palette.background;

    pidTitleData = (gfx_Label){
        .name = "pidTitle",
        .text = "PRUEBAS",
        .pos.x = 125, .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3,
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
    };
    pidTitleWidget.eWidgetType = WD_TYPE_LABEL;
    pidTitleWidget.pvWidget = &pidTitleData;

    pidSubtitleData = (gfx_Label){
        .name = "pidSubtitle",
        .text = "CRUSH: CONTROL PID AVANZADO",
        .pos.x = LCD_WIDTH / 2, .pos.y = 105,
        .alignment = ALIGN_CENTER,
        .typo = TYPO_H3,
        .style = STYLE_SECONDARY,
        .isVisible = true,
    };
    pidSubtitleWidget.eWidgetType = WD_TYPE_LABEL;
    pidSubtitleWidget.pvWidget = &pidSubtitleData;

    for (index = 0U; index < PID_VISIBLE_COUNT; index++) {
        uint8_t pidIndex = g_ui8VisiblePidIndexes[index];
        uint16_t boxX = firstX + (index * columnSpacing);

        pidBoxData[pidIndex] = (gfx_Rectangle){
            .pos.x = boxX, .pos.y = boxY,
            .dim.width = boxWidth, .dim.height = boxHeight,
            .round = 6, .borderWidth = 2,
            .color = g_pCurrentTheme->palette.surface,
        };
        pidBoxWidgets[pidIndex].eWidgetType = WD_TYPE_RECT;
        pidBoxWidgets[pidIndex].pvWidget = &pidBoxData[pidIndex];

        pidNameData[pidIndex] = (gfx_Label){
            .text = pidNames[pidIndex],
            .pos.x = boxX + (boxWidth / 2), .pos.y = boxY - 16,
            .alignment = ALIGN_HCENTER,
            .typo = TYPO_BODY,
            .style = STYLE_TEXT_MUTED,
            .isVisible = true,
        };
        pidNameWidgets[pidIndex].eWidgetType = WD_TYPE_LABEL;
        pidNameWidgets[pidIndex].pvWidget = &pidNameData[pidIndex];

        pidValueData[pidIndex] = (gfx_Label){
            .text = pidValueBuffers[pidIndex],
            .pos.x = boxX + (boxWidth / 2),
            .pos.y = boxY + (boxHeight / 2),
            .alignment = ALIGN_CENTER,
            .typo = TYPO_H1,
            .style = STYLE_PRIMARY,
            .isVisible = true,
        };
        pidValueWidgets[pidIndex].eWidgetType = WD_TYPE_LABEL;
        pidValueWidgets[pidIndex].pvWidget = &pidValueData[pidIndex];

        pidTouchData[pidIndex] = (gfx_TouchArea){
            .pos.x = boxX, .pos.y = boxY,
            .size.width = boxWidth, .size.height = boxHeight,
            .onAreaTouchRelease = onPidTouchRelease,
        };
        gfx_initRegTouch((void *)&pidTouchData[pidIndex], WD_TYPE_TOUCH_AREA);
        pidTouchWidgets[pidIndex].eWidgetType = WD_TYPE_TOUCH_AREA;
        pidTouchWidgets[pidIndex].pvWidget = &pidTouchData[pidIndex];

        canvasInsertAtTop(&g_sCrushPidConfigCanvas.psWidgets,
                          &pidBoxWidgets[pidIndex]);
        canvasInsertAtTop(&g_sCrushPidConfigCanvas.psWidgets,
                          &pidNameWidgets[pidIndex]);
        canvasInsertAtTop(&g_sCrushPidConfigCanvas.psWidgets,
                          &pidValueWidgets[pidIndex]);
        canvasInsertAtTop(&g_sCrushPidConfigCanvas.psWidgets,
                          &pidTouchWidgets[pidIndex]);
    }

    pidStatusData = (gfx_Label){
        .text = pidStatusBuffer,
        .pos.x = LCD_WIDTH / 2, .pos.y = 290,
        .alignment = ALIGN_CENTER,
        .typo = TYPO_BODY,
        .style = STYLE_TEXT_MUTED,
        .isVisible = true,
    };
    pidStatusWidget.eWidgetType = WD_TYPE_LABEL;
    pidStatusWidget.pvWidget = &pidStatusData;

    pidReturnData = (gfx_Button){
        .label = "VOLVER",
        .pos.x = 30, .pos.y = 335,
        .size.width = 236, .size.height = 70,
        .borderWidth = 3, .radius = 5,
        .style = STYLE_DEFAULT, .typo = TYPO_MONO_BOLD,
        .bIsVisible = true, .state = BTN_STATE_NORMAL,
        .onPressed = onGenericBtnPressed, .onRelease = onPidReturnReleased,
    };
    gfx_initRegTouch((void *)&pidReturnData, WD_TYPE_BUTTON);
    pidReturnWidget.eWidgetType = WD_TYPE_BUTTON;
    pidReturnWidget.pvWidget = &pidReturnData;

    pidDefaultsData = (gfx_Button){
        .label = "RESTAURAR FABRICA",
        .pos.x = 282, .pos.y = 335,
        .size.width = 236, .size.height = 70,
        .borderWidth = 3, .radius = 5,
        .style = STYLE_DANGER, .typo = TYPO_MONO_BOLD,
        .bIsVisible = true, .state = BTN_STATE_NORMAL,
        .onPressed = onGenericBtnPressed, .onRelease = onPidDefaultsReleased,
    };
    gfx_initRegTouch((void *)&pidDefaultsData, WD_TYPE_BUTTON);
    pidDefaultsWidget.eWidgetType = WD_TYPE_BUTTON;
    pidDefaultsWidget.pvWidget = &pidDefaultsData;

    pidApplyData = (gfx_Button){
        .label = "APLICAR PID",
        .pos.x = 534, .pos.y = 335,
        .size.width = 236, .size.height = 70,
        .borderWidth = 3, .radius = 5,
        .style = STYLE_SUCCESS, .typo = TYPO_MONO_BOLD,
        .bIsVisible = true, .state = BTN_STATE_NORMAL,
        .onPressed = onGenericBtnPressed, .onRelease = onPidApplyReleased,
    };
    gfx_initRegTouch((void *)&pidApplyData, WD_TYPE_BUTTON);
    pidApplyWidget.eWidgetType = WD_TYPE_BUTTON;
    pidApplyWidget.pvWidget = &pidApplyData;

    useFullHeader(&g_sCrushPidConfigCanvas);
    useNavigationButtons(&g_sCrushPidConfigCanvas);
    canvasInsertAtTop(&g_sCrushPidConfigCanvas.psWidgets, &pidTitleWidget);
    canvasInsertAtTop(&g_sCrushPidConfigCanvas.psWidgets, &pidSubtitleWidget);
    canvasInsertAtTop(&g_sCrushPidConfigCanvas.psWidgets, &pidStatusWidget);
    canvasInsertAtTop(&g_sCrushPidConfigCanvas.psWidgets, &pidReturnWidget);
    canvasInsertAtTop(&g_sCrushPidConfigCanvas.psWidgets, &pidDefaultsWidget);
    canvasInsertAtTop(&g_sCrushPidConfigCanvas.psWidgets, &pidApplyWidget);

    Event_Subscribe(EVT_SYS_CRUSH_PID_SUBMIT_KP,
                    (EventHandler_fn)onPidKpSubmitted);
#if INST_TEMP_PID_ENABLE_INTEGRAL
    Event_Subscribe(EVT_SYS_CRUSH_PID_SUBMIT_KI,
                    (EventHandler_fn)onPidKiSubmitted);
#endif
    Event_Subscribe(EVT_SYS_CRUSH_PID_SUBMIT_KD,
                    (EventHandler_fn)onPidKdSubmitted);
    Event_Subscribe(EVT_CAN_INST_TEMP_PID_KP,
                    (EventHandler_fn)onPidKpReceived);
    Event_Subscribe(EVT_CAN_INST_TEMP_PID_KI,
                    (EventHandler_fn)onPidKiReceived);
    Event_Subscribe(EVT_CAN_INST_TEMP_PID_KD,
                    (EventHandler_fn)onPidKdReceived);
    Event_Subscribe(EVT_CAN_INST_TEMP_PID_APPLY_ACK,
                    (EventHandler_fn)onPidApplyAck);
    Event_Subscribe(EVT_CAN_INST_TEMP_PID_DEFAULTS_ACK,
                    (EventHandler_fn)onPidDefaultsAck);
    Event_Subscribe(EVT_CMD_CHANGE_THEME,
                    (EventHandler_fn)onPidThemeChanged);

    g_i16CrushPidConfigIndex = FormManager_AddForm(&g_sCrushPidConfigCanvas);
}

static void onAdvancedConfigReleasedEvent(gfx_Button *btn) {
    uint8_t i;

    onGenericBtnRelease(btn);
    for (i = 0U; i < 3U; i++) {
        g_bPidValueReceived[i] = false;
        g_bPidValueEdited[i] = false;
        strcpy(pidValueBuffers[i], "--");
        pidValueData[i].bIsDirty = true;
    }
    strcpy(pidStatusBuffer, "SOLICITANDO VALORES...");
    pidStatusData.style = STYLE_TEXT_MUTED;
    pidStatusData.bIsDirty = true;
    if (!InstManager_RequestTempPidValues()) {
        updatePidStatus("COLA CAN OCUPADA", STYLE_DANGER);
    }
    Event_Post(EVT_SYS_SHOW_CRUSH_PID_CONFIG_FORM,
               (EventParam_t){.ptr = NULL});
}
