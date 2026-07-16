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
#include "icon_map.h" // Asumiendo que tienes un ICON_CIRCLE o similar

#include "forms/common_widgets.h"
#include "debug_io_form.h"

#define NUM_IO_CHANNELS 10

int16_t g_i16DebugIoFormIndex = 0; 
static gfx_Canvas g_sDebugIoCanvas;

// ========================================================
// CONTENEDORES Y DATOS DE WIDGETS
// ========================================================
static gfx_GenericWidget formTitleWidget;
static gfx_Label formTitleData;

static gfx_GenericWidget dinSectionLblWidget;
static gfx_Label dinSectionLblData;

static gfx_GenericWidget doutSectionLblWidget;
static gfx_Label doutSectionLblData;

static gfx_GenericWidget returnBtnWidget;
static gfx_Button returnBtnData;

// Entradas (LEDs simulados con Etiquetas e Íconos)
static gfx_GenericWidget dinLblWidgets[NUM_IO_CHANNELS];
static gfx_Label dinLblData[NUM_IO_CHANNELS];
static char dinTextBuffers[NUM_IO_CHANNELS][16];

// Salidas (Botones)
static gfx_GenericWidget doutBtnWidgets[NUM_IO_CHANNELS];
static gfx_Button doutBtnData[NUM_IO_CHANNELS];
static char doutTextBuffers[NUM_IO_CHANNELS][16];

// Estado lógico de las salidas para saber qué pedir en el Toggle
static bool g_bOutState[NUM_IO_CHANNELS] = {false};

// ========================================================
// CALLBACKS DE INTERACCIÓN Y EVENTOS
// ========================================================

static void onReturnBtnReleased(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    // Regresar al menú de opciones de desarrollador/diagnóstico
    Event_Post(EVT_SYS_SHOW_DEBUG_MENU, (EventParam_t){.ptr = NULL});
}

// ---- Botones de Salida (HMI -> Instrumentación) ----
static void onDoutBtnReleased(gfx_Button *btn) {
    onGenericBtnRelease(btn);

    // Identificar qué botón fue presionado comparando punteros
    int channel = -1;
    int i = 0;
    for (; i < NUM_IO_CHANNELS; i++) {
        if (btn == &doutBtnData[i]) {
            channel = i;
            break;
        }
    }

    if (channel >= 0) {
        // Solicitamos el estado INVERSO al actual
        bool requestedState = !g_bOutState[channel];
        uint32_t payload = (channel << 16) | (requestedState ? 1 : 0);
        
        // La UI NO cambia todavía. Solo envía la petición por CAN.
        Event_Post(EVT_SYS_REQ_DOUT_TOGGLE, (EventParam_t){.ui32 = payload});
    }
}

// ---- Recepción de Entradas (Instrumentación -> HMI) ----
static void onDinStateChanged(EventParam_t arg) {
    uint8_t channel = (arg.ui32 >> 16) & 0xFF;
    bool isHigh = (arg.ui32 & 0xFF) != 0;

    if (channel < NUM_IO_CHANNELS) {
        if (isHigh) {
            dinLblData[channel].style = STYLE_SUCCESS; // LED "Encendido" (Verde)
        } else {
            dinLblData[channel].style = STYLE_SECONDARY; // LED "Apagado" (Gris)
        }
        dinLblData[channel].bIsDirty = true;
    }
}

// ---- Confirmación de Salidas (Instrumentación -> HMI) ----
static void onDoutStateChanged(EventParam_t arg) {
    uint8_t channel = (arg.ui32 >> 16) & 0xFF;
    bool isHigh = (arg.ui32 & 0xFF) != 0;

    if (channel < NUM_IO_CHANNELS) {
        // 1. Actualizar memoria de estado
        g_bOutState[channel] = isHigh;

        // 2. Actualizar Botón Visualmente
        sprintf(doutTextBuffers[channel], "OUT %d: %s", channel, isHigh ? "ON" : "OFF");
        doutBtnData[channel].style = isHigh ? STYLE_SUCCESS : STYLE_PRIMARY;
        doutBtnData[channel].bIsDirty = true;
    }
}

// ========================================================
// INICIALIZACIÓN DE LA UI
// ========================================================

void initDebugIoForm(void) {
    g_sDebugIoCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
    
    // --- TÍTULOS ---
    formTitleData = (gfx_Label) {
        .text = "E/S DIGITALES",
        .pos.x = 110, .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3, .style = STYLE_TEXT_MAIN, .isVisible = true,
    };
    formTitleWidget.eWidgetType = WD_TYPE_LABEL; formTitleWidget.pvWidget = &formTitleData;

    dinSectionLblData = (gfx_Label) {
        .text = "ENTRADAS",
        .pos.x = LCD_WIDTH * 0.25f, .pos.y = 100,
        .alignment = ALIGN_CENTER,
        .typo = TYPO_H3, .style = STYLE_TEXT_MUTED, .isVisible = true,
    };
    dinSectionLblWidget.eWidgetType = WD_TYPE_LABEL; dinSectionLblWidget.pvWidget = &dinSectionLblData;

    doutSectionLblData = (gfx_Label) {
        .text = "SALIDAS",
        .pos.x = LCD_WIDTH * 0.75f, .pos.y = 100,
        .alignment = ALIGN_CENTER,
        .typo = TYPO_H3, .style = STYLE_TEXT_MUTED, .isVisible = true,
    };
    doutSectionLblWidget.eWidgetType = WD_TYPE_LABEL; doutSectionLblWidget.pvWidget = &doutSectionLblData;

    // --- GEOMETRÍA DEL GRID ---
    uint16_t rowStart = 130;
    uint16_t rowHeight = 45;
    uint16_t itemWidth = 140;

    // Centros para la zona izquierda (DIN) y derecha (DOUT)
    float dinCol1_x = (LCD_WIDTH * 0.25f) - (itemWidth / 2.0f) - 10;
    float dinCol2_x = (LCD_WIDTH * 0.25f) + (itemWidth / 2.0f) + 10;

    float doutCol1_x = (LCD_WIDTH * 0.75f) - (itemWidth / 2.0f) - 10;
    float doutCol2_x = (LCD_WIDTH * 0.75f) + (itemWidth / 2.0f) + 10;

    // Generar Entradas y Salidas
    int i = 0;
    for (; i < NUM_IO_CHANNELS; i++) {
        uint8_t row = i % 5;
        uint8_t isCol2 = (i >= 5) ? 1 : 0;
        
        float currentY = rowStart + (row * rowHeight);

        // 1. Crear Input "LED" (Etiqueta con texto estático indicando estado vía color)
        sprintf(dinTextBuffers[i], " IN %d ", i);
        dinLblData[i] = (gfx_Label) {
            .text = dinTextBuffers[i],
            .pos.x = isCol2 ? dinCol2_x : dinCol1_x,
            .pos.y = currentY + 15,
            .alignment = ALIGN_CENTER,
            .typo = TYPO_H3,
            .style = STYLE_SECONDARY, // Inicia en OFF
            .isVisible = true,
        };
        dinLblWidgets[i].eWidgetType = WD_TYPE_LABEL;
        dinLblWidgets[i].pvWidget = &dinLblData[i];

        // 2. Crear Output Button
        sprintf(doutTextBuffers[i], "OUT %d: OFF", i);
        doutBtnData[i] = (gfx_Button) {
            .label = doutTextBuffers[i],
            .pos.x = (isCol2 ? doutCol2_x : doutCol1_x) - (itemWidth / 2.0f),
            .pos.y = currentY,
            .size.width = itemWidth,
            .size.height = rowHeight - 8,
            .borderWidth = 1, .radius = 4,
            .state = BTN_STATE_NORMAL,
            .style = STYLE_PRIMARY, // Inicia en OFF (Azul/Secundario)
            .typo = TYPO_BODY,
            .bIsVisible = true,
            .onPressed = onGenericBtnPressed,
            .onRelease = onDoutBtnReleased,
        };
        gfx_initRegTouch((void *)&doutBtnData[i], WD_TYPE_BUTTON);
        doutBtnWidgets[i].eWidgetType = WD_TYPE_BUTTON;
        doutBtnWidgets[i].pvWidget = &doutBtnData[i];
    }

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
    useFullHeader(&g_sDebugIoCanvas);
    //useNavigationButtons(&g_sDebugIoCanvas);

    canvasInsertAtTop(&g_sDebugIoCanvas.psWidgets, &formTitleWidget);
    canvasInsertAtTop(&g_sDebugIoCanvas.psWidgets, &dinSectionLblWidget);
    canvasInsertAtTop(&g_sDebugIoCanvas.psWidgets, &doutSectionLblWidget);
    canvasInsertAtTop(&g_sDebugIoCanvas.psWidgets, &returnBtnWidget);

    for (i = 0; i < NUM_IO_CHANNELS; i++) {
        canvasInsertAtTop(&g_sDebugIoCanvas.psWidgets, &dinLblWidgets[i]);
        canvasInsertAtTop(&g_sDebugIoCanvas.psWidgets, &doutBtnWidgets[i]);
    }

    // Suscripciones de eventos
    Event_Subscribe(EVT_CAN_INST_DIN_CHANGED, (EventHandler_fn)onDinStateChanged);
    Event_Subscribe(EVT_CAN_INST_DOUT_CHANGED, (EventHandler_fn)onDoutStateChanged);

    g_i16DebugIoFormIndex = FormManager_AddForm(&g_sDebugIoCanvas);
}