#include <stdlib.h>#include <stdlib.h>
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
#include "can_id_map.h"
#include "hal_inst_can.h"

#include "forms/common_widgets.h"
#include "forms/adjusts/adjust_numpad_form.h"

#include "variac_debug_form.h"

int16_t g_i16VariacDebugFormIndex = 0;
static gfx_Canvas g_sVariacDebugCanvas;

// ========================================================
// CONTENEDORES DE WIDGETS
// ========================================================
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget formSubtitleWidget;
static gfx_GenericWidget returnBtnWidget;

static gfx_GenericWidget voltageDisplayBoxWidget;
static gfx_GenericWidget voltageDisplayLblWidget;
static gfx_GenericWidget voltageTouchWidget;

static gfx_GenericWidget minusBtnWidget;
static gfx_GenericWidget plusBtnWidget;

// Nuevos: Contenedores de LEDs de Estado
static gfx_GenericWidget ledLowWidget;
static gfx_GenericWidget ledHighWidget;
static gfx_GenericWidget ledAlarmWidget;

// ========================================================
// DATOS DE WIDGETS
// ========================================================
static gfx_Label formTitleData;
static gfx_Label formSubtitleData;
static gfx_Button returnBtnData;

static gfx_Rectangle voltageDisplayBoxData;
static gfx_Label voltageDisplayLblData;
static gfx_TouchArea voltageTouchAreaData;

static gfx_Button minusBtnData;
static gfx_Button plusBtnData;

// Nuevos: Datos de LEDs de Estado
static gfx_Label ledLowData;
static gfx_Label ledHighData;
static gfx_Label ledAlarmData;

// Buffers y variables de estado
static char currentVariacValBuffer[16] = "0.0 [V]";
static float g_f32TargetVariacVoltage = 0.0f;

// ========================================================
// RUTINAS AUXILIARES
// ========================================================

static void sendVariacTargetVoltage(float targetVoltage) {
    union { uint8_t c[4]; float f; } can_data_t;
    can_data_t.f = targetVoltage;

    HAL_CAN_Msg_t msg;
    msg.id = CAN_ID_REQ_VARIAC_SET_VOLTAGE;
    msg.isExtended = false;
    msg.length = sizeof(float);
    msg.data[0] = can_data_t.c[0];
    msg.data[1] = can_data_t.c[1];
    msg.data[2] = can_data_t.c[2];
    msg.data[3] = can_data_t.c[3];
    
    HAL_CAN_Transmit(&msg);
}

// ========================================================
// CALLBACKS DE INTERACCIÓN
// ========================================================

static void onReturnBtnReleased(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_DEBUG_MENU, (EventParam_t){.ptr = NULL});
}

static void onMinusBtnReleased(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    g_f32TargetVariacVoltage -= 0.1f;
    if (g_f32TargetVariacVoltage < 0.0f) {
        g_f32TargetVariacVoltage = 0.0f;
    }
    sendVariacTargetVoltage(g_f32TargetVariacVoltage);
}

static void onPlusBtnReleased(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    g_f32TargetVariacVoltage += 0.1f;
    if (g_f32TargetVariacVoltage > 300.0f) { 
        g_f32TargetVariacVoltage = 300.0f;
    }
    sendVariacTargetVoltage(g_f32TargetVariacVoltage);
}

static void onDisplayTouchRelease(gfx_TouchArea *area) {
    Event_Post(EVT_SYS_SHOW_ADJ_NUMPAD_FORM, (EventParam_t){.ui32 = ADJ_VARIAC_VOLTAGE});
}

// ========================================================
// CALLBACKS DE EVENTOS CAN
// ========================================================

static void onVariacVoltageChanged(EventParam_t arg) {
    if(GetExecTimeMs() % 200 < 30) {
        sprintf(currentVariacValBuffer, "%.1f [V]", arg.f32);
        voltageDisplayLblData.bIsDirty = true;
    }
}

// Nuevo: Atrapa el byte de estado para actualizar los LEDs
static void onVariacStatusChanged(EventParam_t arg) {
    uint8_t flags = arg.ui32;
    
    // Asumimos un mapa de bits simple: 
    // Bit 0: Limite Inferior, Bit 1: Limite Superior, Bit 2: Alarma
    bool isLow   = (flags & 0x01) != 0;
    bool isHigh  = (flags & 0x02) != 0;
    bool isAlarm = (flags & 0x04) != 0;

    ledLowData.style   = isLow   ? STYLE_SECONDARY : STYLE_SECONDARY;
    ledHighData.style  = isHigh  ? STYLE_SECONDARY : STYLE_SECONDARY;
    ledAlarmData.style = isAlarm ? STYLE_DANGER  : STYLE_SECONDARY;

    ledLowData.bIsDirty = true;
    ledHighData.bIsDirty = true;
    ledAlarmData.bIsDirty = true;
}

// ========================================================
// INICIALIZACIÓN DE LA UI
// ========================================================

void initVariacDebugForm(void) {
    g_sVariacDebugCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;

    formTitleData = (gfx_Label) {
        .text = "VARIAC DEBUG",
        .pos.x = 110, .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3, .style = STYLE_TEXT_MAIN, .isVisible = true,
    };
    formTitleWidget.eWidgetType = WD_TYPE_LABEL; formTitleWidget.pvWidget = &formTitleData;

    formSubtitleData = (gfx_Label){
        .text = "TOCA EL VALOR PARA INGRESAR VOLTAJE O USA LOS BOTONES",
        .pos.x = LCD_WIDTH / 2, .pos.y = 110,
        .typo = TYPO_CAPTION, .style = STYLE_TEXT_MUTED, .isVisible = true,
        .alignment = ALIGN_CENTER,
    };
    formSubtitleWidget.eWidgetType = WD_TYPE_LABEL; formSubtitleWidget.pvWidget = &formSubtitleData;

    // --- GEOMETRÍA CENTRAL ---
    uint16_t boxWidth = 280;
    uint16_t boxHeight = 120;
    float center_x = LCD_WIDTH / 2.0f;
    float box_y = 170;

    // --- CAJA INDICADORA ---
    voltageDisplayBoxData = (gfx_Rectangle){
        .pos.x = center_x - (boxWidth / 2.0f), .pos.y = box_y, 
        .dim.width = boxWidth, .dim.height = boxHeight,
        .round = 8, .borderWidth = 3, .color = g_pCurrentTheme->palette.surface,
    };
    voltageDisplayBoxWidget.eWidgetType = WD_TYPE_RECT; voltageDisplayBoxWidget.pvWidget = &voltageDisplayBoxData;

    voltageDisplayLblData = (gfx_Label) {
        .text = currentVariacValBuffer,
        .pos.x = center_x, .pos.y = box_y + (boxHeight / 2.0f),
        .typo = TYPO_H1, .style = STYLE_PRIMARY, .isVisible = true, .alignment = ALIGN_CENTER,
    };
    voltageDisplayLblWidget.eWidgetType = WD_TYPE_LABEL; voltageDisplayLblWidget.pvWidget = &voltageDisplayLblData;

    voltageTouchAreaData = (gfx_TouchArea) {
        .pos = voltageDisplayBoxData.pos, .size = voltageDisplayBoxData.dim, 
        .onAreaTouchRelease = onDisplayTouchRelease,
    };
    gfx_initRegTouch((void *)&voltageTouchAreaData, WD_TYPE_TOUCH_AREA);
    voltageTouchWidget.eWidgetType = WD_TYPE_TOUCH_AREA; voltageTouchWidget.pvWidget = &voltageTouchAreaData;

    // --- BOTONES MENOS Y MÁS ---
    uint16_t btnSize = 80;
    uint16_t padding = 30;

    minusBtnData = (gfx_Button){
        .label = "-",
        .pos.x = voltageDisplayBoxData.pos.x - btnSize - padding, 
        .pos.y = box_y + (boxHeight / 2.0f) - (btnSize / 2.0f),
        .size.width = btnSize, .size.height = btnSize,
        .borderWidth = 2, .radius = 40, 
        .style = STYLE_SECONDARY, .typo = TYPO_H1, .bIsVisible = true, .state = BTN_STATE_NORMAL,
        .onPressed = onGenericBtnPressed, .onRelease = onMinusBtnReleased,
    };
    gfx_initRegTouch((void *)&minusBtnData, WD_TYPE_BUTTON);
    minusBtnWidget.eWidgetType = WD_TYPE_BUTTON; minusBtnWidget.pvWidget = &minusBtnData;

    plusBtnData = (gfx_Button){
        .label = "+",
        .pos.x = voltageDisplayBoxData.pos.x + boxWidth + padding, 
        .pos.y = box_y + (boxHeight / 2.0f) - (btnSize / 2.0f),
        .size.width = btnSize, .size.height = btnSize,
        .borderWidth = 2, .radius = 40,
        .style = STYLE_SECONDARY, .typo = TYPO_H1, .bIsVisible = true, .state = BTN_STATE_NORMAL,
        .onPressed = onGenericBtnPressed, .onRelease = onPlusBtnReleased,
    };
    gfx_initRegTouch((void *)&plusBtnData, WD_TYPE_BUTTON);
    plusBtnWidget.eWidgetType = WD_TYPE_BUTTON; plusBtnWidget.pvWidget = &plusBtnData;

    // --- NUEVO: LEDS DE ESTADO ---
    float led_y = box_y + boxHeight + 40; // 40px por debajo del recuadro
    float ledSpacing = 160;

    ledLowData = (gfx_Label) {
        .text = " VARIAC LOW ",
        .pos.x = center_x - ledSpacing, .pos.y = led_y,
        .typo = TYPO_BODY, .style = STYLE_SECONDARY, .isVisible = true, .alignment = ALIGN_CENTER,
    };
    ledLowWidget.eWidgetType = WD_TYPE_LABEL; ledLowWidget.pvWidget = &ledLowData;

    ledHighData = (gfx_Label) {
        .text = " VARIAC HIGH ",
        .pos.x = center_x, .pos.y = led_y,
        .typo = TYPO_BODY, .style = STYLE_SECONDARY, .isVisible = true, .alignment = ALIGN_CENTER,
    };
    ledHighWidget.eWidgetType = WD_TYPE_LABEL; ledHighWidget.pvWidget = &ledHighData;

    ledAlarmData = (gfx_Label) {
        .text = " ALARMA ",
        .pos.x = center_x + ledSpacing, .pos.y = led_y,
        .typo = TYPO_BODY, .style = STYLE_SECONDARY, .isVisible = true, .alignment = ALIGN_CENTER,
    };
    ledAlarmWidget.eWidgetType = WD_TYPE_LABEL; ledAlarmWidget.pvWidget = &ledAlarmData;

    // --- BOTÓN DE RETORNO ---
    returnBtnData = (gfx_Button){
        .label = "REGRESAR",
        .size.width = 180, 
		.size.height = 55,
        .pos.x = LCD_WIDTH / 2 - 180 / 2, 
		.pos.y = LCD_HEIGHT - 100,
        .borderWidth = 2, 
		.radius = 5, 
		.style = STYLE_DANGER, 
		.typo = TYPO_H3,
        .bIsVisible = true, 
		.state = BTN_STATE_NORMAL,
        .onPressed = onGenericBtnPressed, 
		.onRelease = onReturnBtnReleased,
    };
    gfx_initRegTouch((void *)&returnBtnData, WD_TYPE_BUTTON);
    returnBtnWidget.eWidgetType = WD_TYPE_BUTTON; returnBtnWidget.pvWidget = &returnBtnData;

    // --- ENSAMBLE DEL CANVAS ---
    useFullHeader(&g_sVariacDebugCanvas);
    //useNavigationButtons(&g_sVariacDebugCanvas);

    canvasInsertAtTop(&g_sVariacDebugCanvas.psWidgets, &formTitleWidget);
    canvasInsertAtTop(&g_sVariacDebugCanvas.psWidgets, &formSubtitleWidget);
    
    // Central Display
    canvasInsertAtTop(&g_sVariacDebugCanvas.psWidgets, &voltageDisplayBoxWidget);
    canvasInsertAtTop(&g_sVariacDebugCanvas.psWidgets, &voltageDisplayLblWidget);
    canvasInsertAtTop(&g_sVariacDebugCanvas.psWidgets, &voltageTouchWidget);

    // Controls
    canvasInsertAtTop(&g_sVariacDebugCanvas.psWidgets, &minusBtnWidget);
    canvasInsertAtTop(&g_sVariacDebugCanvas.psWidgets, &plusBtnWidget);

    // LEDs de Estado
    canvasInsertAtTop(&g_sVariacDebugCanvas.psWidgets, &ledLowWidget);
    canvasInsertAtTop(&g_sVariacDebugCanvas.psWidgets, &ledHighWidget);
    canvasInsertAtTop(&g_sVariacDebugCanvas.psWidgets, &ledAlarmWidget);
    
    canvasInsertAtTop(&g_sVariacDebugCanvas.psWidgets, &returnBtnWidget);

    // Suscripción a eventos CAN
    Event_Subscribe(EVT_CAN_INST_VARIAC_VOLTAGE, (EventHandler_fn)onVariacVoltageChanged);
    Event_Subscribe(EVT_CAN_INST_VARIAC_STATUS, (EventHandler_fn)onVariacStatusChanged); // <-- NUEVO

    g_i16VariacDebugFormIndex = FormManager_AddForm(&g_sVariacDebugCanvas);
}