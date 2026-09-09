#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gui_core.h"
#include "gui_canvas.h"
#include "forms_manager.h"
#include "event_engine.h"
#include "gui_theme.h"
#include "FT8xx_params.h"

#include "common_widgets.h"

#include "numpad_modify_value_form.h"

#define NUMPAD_MAX_DIGITS 7

gfx_Canvas g_sNumpadModifyCanvas;
int16_t g_i16NumpadModifyValueIndex = 0;

// ========================================================
// CONTENEDORES (Wrappers para la lista enlazada del Canvas)
// ========================================================
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget displayBgWidget;
static gfx_GenericWidget displayLabelWidget;
static gfx_GenericWidget displayValueWidget;
static gfx_GenericWidget buttonWidgets[15];

// ========================================================
// DATOS DE WIDGETS
// ========================================================
static gfx_Label     formTitleData;
static gfx_Rectangle displayBgData;
static gfx_Label     displayLabelData;
static gfx_Label     displayValueData;
static gfx_Button    buttonData[15];

// ========================================================
// BUFFERS DE ESTADO
// ========================================================
static char valueLabelBuffer[50] = "CORRIENTE OBJETIVO [A]";
static char digitBuffer[NUMPAD_MAX_DIGITS + 1] = ""; 
static uint8_t digitLen = 0;

// Form to call on cancel/ok
EventID_e g_eFormCallback = EVT_SYS_NULL;
EventID_e g_eValueSubmitEvent = EVT_SYS_NULL;

// ========================================================
// CALLBACKS DE LÓGICA DEL TECLADO
// ========================================================

static void onNumberBtnReleased(gfx_Button *btn) {
    if (digitLen < NUMPAD_MAX_DIGITS) {
        // REGLA: Si solo hay un "0" inicial y se presiona otro número, reemplazamos el "0"
        if (digitLen == 1 && digitBuffer[0] == '0') {
            digitBuffer[0] = btn->label[0];
        } else {
            digitBuffer[digitLen++] = btn->label[0];
            digitBuffer[digitLen] = '\0';
        }
        displayValueData.bIsDirty = true;
    }
    onGenericBtnRelease(btn);
}

static void onDotBtnReleased(gfx_Button *btn) {
    // REGLA: Si el buffer está vacío por alguna razón, agregar "0." de golpe
    if (digitLen == 0) {
        strcpy(digitBuffer, "0.");
        digitLen = 2;
        displayValueData.bIsDirty = true;
    }
    // Permitir el punto solo si no existe ya uno en la cadena
    else if (digitLen < NUMPAD_MAX_DIGITS && strchr(digitBuffer, '.') == NULL) {
        digitBuffer[digitLen++] = '.';
        digitBuffer[digitLen] = '\0';
        displayValueData.bIsDirty = true;
    }
    onGenericBtnRelease(btn);
}

static void onSpaceBtnReleased(gfx_Button *btn) {
    if (digitLen < NUMPAD_MAX_DIGITS) {
        digitBuffer[digitLen++] = ' ';
        digitBuffer[digitLen] = '\0';
        displayValueData.bIsDirty = true;
    }
    onGenericBtnRelease(btn);
}

static void onDeleteBtnReleased(gfx_Button *btn) {
    if (digitLen > 0) {
        digitLen--;
        digitBuffer[digitLen] = '\0';

        // ========================================================
        // REGLAS DE RESCATE AUTOMÁTICO (Evitan estados inválidos)
        // ========================================================
        
        // Regrada 1: Si al borrar vacías por completo el buffer -> Debe quedar "0"
        if (digitLen == 0) {
            digitBuffer[0] = '0';
            digitBuffer[1] = '\0';
            digitLen = 1;
        }
        // Regla 2: Si tenías algo como "0.5" y borras el 5 -> Queda "0." 
        // Si vuelves a borrar el punto -> Debe quedar "0" en lugar de vacío.
        else if (digitLen == 1 && digitBuffer[0] == '.') {
            digitBuffer[0] = '0';
            digitBuffer[1] = '\0';
            digitLen = 1;
        }
        // Regla 3: Si tenías un número negativo o un espacio huérfano (según tu layout)
        else if (digitLen == 1 && (digitBuffer[0] == '-' || digitBuffer[0] == ' ')) {
            digitBuffer[0] = '0';
            digitBuffer[1] = '\0';
            digitLen = 1;
        }

        displayValueData.bIsDirty = true;
    }
    onGenericBtnRelease(btn);
}

static void onCancelBtnReleased(gfx_Button *btn) {
    // Ejemplo: Regresar al menú anterior o limpiar el buffer
    digitLen = 0;
    digitBuffer[0] = '\0';
    displayValueData.bIsDirty = true;
    
    onGenericBtnRelease(btn);
	Event_Post(g_eFormCallback, (EventParam_t){.ptr = NULL});
}

static void onOkBtnReleased(gfx_Button *btn) {
    float finalValue = atof(digitBuffer);
    
    // Aquí puedes disparar el evento hacia tu lógica de control
    // Event_Post(EVT_UI_NUMPAD_VALUE_ACCEPTED, (EventParam_t){.f32 = finalValue});

    onGenericBtnRelease(btn);
	if(g_eValueSubmitEvent == EVT_SYS_FAULT_CFG_SUBMIT_CALIBER || g_eValueSubmitEvent == EVT_SYS_CRUSH_CFG_SUBMIT_CALIBER || g_eValueSubmitEvent == EVT_SYS_PROFILE_CFG_SUBMIT_CALIBER)
		Event_Post(g_eValueSubmitEvent, (EventParam_t){.str = digitBuffer});
	else
		Event_Post(g_eValueSubmitEvent, (EventParam_t){.f32 = finalValue});

	Event_Post(g_eFormCallback, (EventParam_t){.ptr = NULL});
}

// ========================================================
// CALLBACKS EXTERNOS (Eventos del Sistema)
// ========================================================

static void onFaultCfgCurrent(EventParam_t arg) {
    strcpy(valueLabelBuffer, "CORRIENTE OBJETIVO [A]");

    sprintf(digitBuffer, "%.1f", arg.f32);
    digitLen = strlen(digitBuffer); // IMPORTANTE: Sincronizar la longitud
    
    displayLabelData.bIsDirty = true;
    displayValueData.bIsDirty = true;

	g_eFormCallback = EVT_SYS_SHOW_FAULT_CONFIG_FORM;
	g_eValueSubmitEvent = EVT_SYS_FAULT_CFG_SUBMIT_CURRENT;
}

static void onFaultCfgDuration(EventParam_t arg) {
    strcpy(valueLabelBuffer, "DURACION DEL ENSAYO [s]");

    sprintf(digitBuffer, "%u", (uint32_t)arg.f32);
    digitLen = strlen(digitBuffer); // IMPORTANTE: Sincronizar la longitud
    
    displayLabelData.bIsDirty = true;
    displayValueData.bIsDirty = true;

	g_eFormCallback = EVT_SYS_SHOW_FAULT_CONFIG_FORM;
	g_eValueSubmitEvent = EVT_SYS_FAULT_CFG_SUBMIT_DURATION;
}

static void onFaultCfgPresetVoltage(EventParam_t arg) {
    strcpy(valueLabelBuffer, "PRESET VOLTAJE SEC. [V]");

    sprintf(digitBuffer, "%.1f", arg.f32);
    digitLen = strlen(digitBuffer);
    
    displayLabelData.bIsDirty = true;
    displayValueData.bIsDirty = true;

	g_eFormCallback = EVT_SYS_SHOW_FAULT_CONFIG_FORM;
	g_eValueSubmitEvent = EVT_SYS_FAULT_CFG_SUBMIT_PRESET_VOLTAGE;
}

static void onFaultCfgCaliber(EventParam_t arg) {
    strcpy(valueLabelBuffer, "CALIBRE [AWG]");

    sprintf(digitBuffer, "%s", arg.str);
    digitLen = strlen(digitBuffer); // IMPORTANTE: Sincronizar la longitud
    
    displayLabelData.bIsDirty = true;
    displayValueData.bIsDirty = true;

	g_eFormCallback = EVT_SYS_SHOW_FAULT_CONFIG_FORM;
	g_eValueSubmitEvent = EVT_SYS_FAULT_CFG_SUBMIT_CALIBER;
}

static void onCrushCfgTemp(EventParam_t arg) {
    strcpy(valueLabelBuffer, "TEMPERATURA [C]");

    sprintf(digitBuffer, "%.1f", arg.f32);
    digitLen = strlen(digitBuffer); // IMPORTANTE: Sincronizar la longitud
    
    displayLabelData.bIsDirty = true;
    displayValueData.bIsDirty = true;

	g_eFormCallback = EVT_SYS_SHOW_CRUSH_CONFIG_FORM;
	g_eValueSubmitEvent = EVT_SYS_CRUSH_CFG_SUBMIT_TEMP;
}

static void onCrushCfgDuration(EventParam_t arg) {
    strcpy(valueLabelBuffer, "DURACION DEL ENSAYO [s]");

    sprintf(digitBuffer, "%u", (uint32_t)arg.f32);
    digitLen = strlen(digitBuffer); // IMPORTANTE: Sincronizar la longitud
    
    displayLabelData.bIsDirty = true;
    displayValueData.bIsDirty = true;

	g_eFormCallback = EVT_SYS_SHOW_CRUSH_CONFIG_FORM;
	g_eValueSubmitEvent = EVT_SYS_CRUSH_CFG_SUBMIT_DURATION;

}

static void onCrushCfgCaliber(EventParam_t arg) {
    strcpy(valueLabelBuffer, "CALIBRE [AWG]");

    sprintf(digitBuffer, "%s", arg.str);
    digitLen = strlen(digitBuffer); // IMPORTANTE: Sincronizar la longitud
    
    displayLabelData.bIsDirty = true;
    displayValueData.bIsDirty = true;

	g_eFormCallback = EVT_SYS_SHOW_CRUSH_CONFIG_FORM;
	g_eValueSubmitEvent = EVT_SYS_CRUSH_CFG_SUBMIT_CALIBER;
}

static void onProfileCfgCaliber(EventParam_t arg) {
    strcpy(valueLabelBuffer, "CALIBRE [AWG]");

    sprintf(digitBuffer, "%s", arg.str);
    digitLen = strlen(digitBuffer); // IMPORTANTE: Sincronizar la longitud
    
    displayLabelData.bIsDirty = true;
    displayValueData.bIsDirty = true;

	g_eFormCallback = EVT_SYS_SHOW_SEQUENCE_CONFIG_FORM;
	g_eValueSubmitEvent = EVT_SYS_PROFILE_CFG_SUBMIT_CALIBER;
}

// ========================================================
// INICIALIZACIÓN DE LA UI
// ========================================================

static void createNumpadButton(uint8_t index, int16_t x, int16_t y, int16_t w, int16_t h, char *label, uint8_t style, void (*releaseCallback)(gfx_Button*)) {
    gfx_Button *btn = &buttonData[index];
    
    btn->pos.x = x;
    btn->pos.y = y;
    btn->size.width = w;
    btn->size.height = h;
    btn->label = label;
    btn->typo = TYPO_H3; 
    btn->style = (gfx_WidgetStyle_e)style; 
    btn->state = BTN_STATE_NORMAL;
    btn->radius = 6;
    btn->borderWidth = 1;
    btn->bIsDirty = true;
	btn->bIsVisible = true;
    
    btn->onPressed = onGenericBtnPressed;
    btn->onRelease = releaseCallback; // Usamos el callback específico en lugar del genérico

    gfx_initRegTouch((void*)btn, WD_TYPE_BUTTON);

    // Enlazar al wrapper genérico
    buttonWidgets[index].eWidgetType = WD_TYPE_BUTTON;
    buttonWidgets[index].pvWidget = (void *)btn;
    
    // Añadir al canvas
    canvasInsertAtTop(&g_sNumpadModifyCanvas.psWidgets, &buttonWidgets[index]);
}

void initNumpadModifyValueForm(void) {
    g_sNumpadModifyCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
    g_sNumpadModifyCanvas.psWidgets = NULL; // Asegurar que el canvas inicia limpio

    // 1. TÍTULO DEL FORMULARIO
    formTitleData = (gfx_Label) {
        .name = "formTitleData",
        .text = "PRUEBAS",
        .pos.x = 125,
        .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
        .bIsDirty = true
    };
    formTitleWidget.eWidgetType = WD_TYPE_LABEL;
    formTitleWidget.pvWidget = (void *)&formTitleData;
    
    // Geometría base extraída de tu inicializador original
    int16_t baseX = 10;
    int16_t baseY = 80;
    int16_t totalWidth = LCD_WIDTH; 
    int16_t totalHeight = 400;

    uint16_t rowHeight = ((totalHeight - 5 * 10) / 5.0);
    uint16_t verticalSpacer = 10;
    uint16_t horizontalSpacer = 10;

    // 2. DISPLAY BACKGROUND
    displayBgData = (gfx_Rectangle){
        .pos.x = baseX,
        .pos.y = baseY,
        .dim.height = rowHeight,
        .dim.width = totalWidth - 20,
        .borderWidth = 3,
        .color = g_pCurrentTheme->palette.surface,
        .round = 4,
    };
    displayBgWidget.eWidgetType = WD_TYPE_RECT;
    displayBgWidget.pvWidget = (void *)&displayBgData;

    // 3. DISPLAY LABEL (Izquierda)
    displayLabelData = (gfx_Label){
        .name = "displayLabel",
        .text = valueLabelBuffer,
        .pos.x = baseX + 20,
        .pos.y = baseY + rowHeight / 2,
        .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER),
        .isVisible = true,
        .style = STYLE_TEXT_MUTED,
        .typo = TYPO_BODY,
        .bIsDirty = true
    };
    displayLabelWidget.eWidgetType = WD_TYPE_LABEL;
    displayLabelWidget.pvWidget = (void *)&displayLabelData;

    // 4. DISPLAY VALUE (Derecha)
    displayValueData = (gfx_Label) {
        .name = "displayValue",
        .text = digitBuffer,
        .pos.x = baseX + totalWidth - 40,
        .pos.y = baseY + rowHeight / 2,
        .alignment = (gfx_Align_e)(ALIGN_RIGHT | ALIGN_VCENTER),
        .isVisible = true,
        .style = STYLE_TEXT_MAIN,
        .typo = TYPO_H2,
        .bIsDirty = true
    };
    displayValueWidget.eWidgetType = WD_TYPE_LABEL;
    displayValueWidget.pvWidget = (void *)&displayValueData;

    // Añadir componentes pasivos al canvas
    useFullHeader(&g_sNumpadModifyCanvas);
    canvasInsertAtTop(&g_sNumpadModifyCanvas.psWidgets, &formTitleWidget);
    canvasInsertAtTop(&g_sNumpadModifyCanvas.psWidgets, &displayBgWidget);
    canvasInsertAtTop(&g_sNumpadModifyCanvas.psWidgets, &displayLabelWidget);
    canvasInsertAtTop(&g_sNumpadModifyCanvas.psWidgets, &displayValueWidget);

    // 5. BOTONES
    int16_t incrementalX = baseX;
    int16_t incrementalY = baseY + rowHeight + verticalSpacer;
    uint16_t buttonWidth = ((totalWidth - 10) - 10 * 4) / 4.0f;

    // Fila 1
    createNumpadButton(7,  incrementalX, incrementalY, buttonWidth, rowHeight, "7", STYLE_TEXT_MAIN, onNumberBtnReleased);
    createNumpadButton(8,  incrementalX + buttonWidth + horizontalSpacer, incrementalY, buttonWidth, rowHeight, "8", STYLE_TEXT_MAIN, onNumberBtnReleased);
    createNumpadButton(9,  incrementalX + 2 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight, "9", STYLE_TEXT_MAIN, onNumberBtnReleased);
    createNumpadButton(14, incrementalX + 3 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight, "BORRAR", STYLE_DANGER, onDeleteBtnReleased);
    
    // Fila 2
    incrementalY += rowHeight + verticalSpacer;
    createNumpadButton(4,  incrementalX, incrementalY, buttonWidth, rowHeight, "4", STYLE_TEXT_MAIN, onNumberBtnReleased);
    createNumpadButton(5,  incrementalX + buttonWidth + horizontalSpacer, incrementalY, buttonWidth, rowHeight, "5", STYLE_TEXT_MAIN, onNumberBtnReleased);
    createNumpadButton(6,  incrementalX + 2 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight, "6", STYLE_TEXT_MAIN, onNumberBtnReleased);
    createNumpadButton(13, incrementalX + 3 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight, "CANCELAR", STYLE_TEXT_MUTED, onCancelBtnReleased);

    // Fila 3
    incrementalY += rowHeight + verticalSpacer;
    createNumpadButton(1,  incrementalX, incrementalY, buttonWidth, rowHeight, "1", STYLE_TEXT_MAIN, onNumberBtnReleased);
    createNumpadButton(2,  incrementalX + buttonWidth + horizontalSpacer, incrementalY, buttonWidth, rowHeight, "2", STYLE_TEXT_MAIN, onNumberBtnReleased);
    createNumpadButton(3,  incrementalX + 2 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight, "3", STYLE_TEXT_MAIN, onNumberBtnReleased);
    
    // El botón OK abarca 2 filas de altura (rowHeight * 2 + verticalSpacer)
    createNumpadButton(12, incrementalX + 3 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight * 2 + verticalSpacer, "OK", STYLE_SUCCESS, onOkBtnReleased);

    // Fila 4
    incrementalY += rowHeight + verticalSpacer;
    createNumpadButton(0,  incrementalX, incrementalY, buttonWidth, rowHeight, "0", STYLE_TEXT_MAIN, onNumberBtnReleased);
    createNumpadButton(10, incrementalX + buttonWidth + horizontalSpacer, incrementalY, buttonWidth, rowHeight, " ", STYLE_TEXT_MAIN, onSpaceBtnReleased);
    createNumpadButton(11, incrementalX + 2 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight, ".", STYLE_TEXT_MAIN, onDotBtnReleased);


    // 6. EVENTOS Y REGISTRO DEL FORMULARIO
    Event_Subscribe(EVT_SYS_NUMPAD_MOD_FAULT_CFG_CURRENT, (EventHandler_fn)onFaultCfgCurrent);
    Event_Subscribe(EVT_SYS_NUMPAD_MOD_FAULT_CFG_DURATION, (EventHandler_fn)onFaultCfgDuration);
    Event_Subscribe(EVT_SYS_NUMPAD_MOD_FAULT_CFG_PRESET_VOLTAGE, (EventHandler_fn)onFaultCfgPresetVoltage);
	Event_Subscribe(EVT_SYS_NUMPAD_MOD_FAULT_CFG_CALIBER, (EventHandler_fn)onFaultCfgCaliber);

    Event_Subscribe(EVT_SYS_NUMPAD_MOD_CRUSH_CFG_DURATION, (EventHandler_fn)onCrushCfgDuration);
    Event_Subscribe(EVT_SYS_NUMPAD_MOD_CRUSH_CFG_TEMP, (EventHandler_fn)onCrushCfgTemp);
    Event_Subscribe(EVT_SYS_NUMPAD_MOD_CRUSH_CFG_CALIBER, (EventHandler_fn)onCrushCfgCaliber);

	Event_Subscribe(EVT_SYS_NUMPAD_MOD_PROFILE_CFG_CALIBER, (EventHandler_fn)onProfileCfgCaliber);
    
    g_i16NumpadModifyValueIndex = FormManager_AddForm(&g_sNumpadModifyCanvas);
}


