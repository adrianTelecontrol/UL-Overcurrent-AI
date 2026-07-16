#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gui_core.h"
#include "gui_canvas.h"
#include "forms_manager.h"
#include "event_engine.h"
#include "gui_theme.h"
#include "FT8xx_params.h"
#include "helpers.h"
#include "can_id_map.h"
#include "hal_inst_can.h"

#include "forms/common_widgets.h"
#include "adjust_numpad_form.h"

#define NUMPAD_MAX_DIGITS 7

gfx_Canvas g_sAdjustNumpadCanvas;
int16_t g_i16AdjustNumpadIndex = 0;

// Tipos de termopares soportados
typedef enum {
    THERMOCOUPLE_TYPE_K,
    THERMOCOUPLE_TYPE_J,
    THERMOCOUPLE_TYPE_T,
    THERMOCOUPLE_TYPE_E,
    THERMOCOUPLE_TYPE_N,
    THERMOCOUPLE_TYPE_R,
    THERMOCOUPLE_TYPE_S,
    THERMOCOUPLE_TYPE_B,
	THERMOCOUPLE_TYPE_CJC,
} thermocouple_type_t;

// Nombres de los termopares para mostrar en el botón
static const char* const tcTypeNames[] = {
    "TC: K", "TC: J", "TC: T", "TC: E", 
    "TC: N", "TC: R", "TC: S", "TC: B", 
	"TC: CJC",
};

// ========================================================
// CONTENEDORES (Wrappers para la lista enlazada del Canvas)
// ========================================================
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget displayBgWidget;
static gfx_GenericWidget currValueBgWidget;
static gfx_GenericWidget displayLabelWidget;
static gfx_GenericWidget displayValueWidget;
static gfx_GenericWidget currValLabelWidget, inputLabelWidget;
static gfx_GenericWidget buttonWidgets[17]; // Aumentado a 17 (0-16)
static gfx_GenericWidget currValueWidget;

// ========================================================
// DATOS DE WIDGETS
// ========================================================
static gfx_Label     formTitleData;
static gfx_Rectangle displayBgData;
static gfx_Rectangle currValueBgData;
static gfx_Label     displayLabelData;
static gfx_Label     displayValueData;
static gfx_Button    buttonData[17]; // Aumentado a 17 (0-16)
static gfx_Label     currValLabelData, inputLabelData;
static gfx_Label     currValueData;

// ========================================================
// BUFFERS DE ESTADO
// ========================================================
static char valueLabelBuffer[50] = "CORRIENTE OBJETIVO [A]";
static char digitBuffer[NUMPAD_MAX_DIGITS + 2] = "0"; // +2 para acomodar el signo y el nulo
static char primaryCurrentBuff[11] = "100.0 [A]";
static char secondaryCurrentBuff[11] = "100.0 [A]";
static char primaryVoltageBuff[11] = "100.0 [V]";
static char secondaryVoltageBuff[11] = "100.0 [V]";
static char mainProbeTempBuff[11] = "100.0 [C]";
static char secondaryProbeTempBuff[11] = "100.0 [C]";
static char primaryTxTempBuff[11] = "100.0 [C]";
static char secondaryTxTempBuff[11] = "100.0 [C]";
static char cableATempBuff[11] = "100.0 [C]";
static char cableBTempBuff[11] = "100.0 [C]";
static char cjcTempBuff[11] = "100.0 [C]";
static char variacVoltageBuff[11] = "100.0 [V]";
static uint8_t digitLen = 0;

static thermocouple_type_t g_eCurrentTCType = THERMOCOUPLE_TYPE_K;

// Form to call on cancel/ok
static EventID_e g_eFormCallback = EVT_SYS_NULL;
static can_id_map_e g_eValueSubmitID;
static adj_value_e g_eAdjValueType;
static bool g_bIsHighPoint = false;
static bool g_IsTcTypeModified = false;

// ========================================================
// CALLBACKS DE LÓGICA DEL TECLADO
// ========================================================
static void onNumberBtnReleased(gfx_Button *btn) {
    // 1. Restricción de 1 solo decimal
    char *dotPos = strchr(digitBuffer, '.');
    
    // Si hay un punto, strlen(dotPos) incluye el propio punto. 
    // Si es por ejemplo ".5", la longitud es 2.
    if (dotPos != NULL && strlen(dotPos) >= 2) {
        onGenericBtnRelease(btn);
        return; // Ignorar el botón, ya tenemos un decimal
    }

    // 2. Lógica normal de inserción
    if (digitLen < NUMPAD_MAX_DIGITS) {
        // Evitar ceros a la izquierda, excepto si hay un "-" antes
        if (digitLen == 1 && digitBuffer[0] == '0') {
            digitBuffer[0] = btn->label[0];
        } 
        else if (digitLen == 2 && digitBuffer[0] == '-' && digitBuffer[1] == '0') {
            digitBuffer[1] = btn->label[0];
        } 
        else {
            digitBuffer[digitLen++] = btn->label[0];
            digitBuffer[digitLen] = '\0';
        }
        displayValueData.bIsDirty = true;
    }
    onGenericBtnRelease(btn);
}

static void onHighLowPointBtnReleased(gfx_Button *btn) {
    btn->state = BTN_STATE_NORMAL;
    if(g_bIsHighPoint) {
        btn->style = STYLE_SECONDARY;
        btn->label = "BAJO";
        btn->bIsDirty = true;
        g_bIsHighPoint = false;

        switch(g_eAdjValueType) {
            case ADJ_VOLTAGE_PRIMARY: g_eValueSubmitID = CAN_ID_VOLTAGE_TX_PRIMARY_ADJ_LOW; break;
            case ADJ_VOLTAGE_SECONDARY: g_eValueSubmitID = CAN_ID_VOLTAGE_TX_SECONDARY_ADJ_LOW; break;
            case ADJ_CURRENT_PRIMARY: g_eValueSubmitID = CAN_ID_CURRENT_TX_PRIMARY_ADJ_LOW; break;
            case ADJ_CURRENT_SECONDARY: g_eValueSubmitID = CAN_ID_CURRENT_TX_SECONDARY_ADJ_LOW; break;
            case ADJ_TEMP_PROBE_MAIN: g_eValueSubmitID = CAN_ID_TEMP_PROBE_MAIN_ADJ_LOW; break;
            case ADJ_TEMP_PROBE_SECONDARY: g_eValueSubmitID = CAN_ID_TEMP_PROBE_SECONDARY_ADJ_LOW; break;
            case ADJ_TEMP_TX_PRIMARY: g_eValueSubmitID = CAN_ID_TEMP_TX_PRIMARY_ADJ_LOW; break;
            case ADJ_TEMP_TX_SECONDARY: g_eValueSubmitID = CAN_ID_TEMP_TX_SECONDARY_ADJ_LOW; break;
            case ADJ_TEMP_CABLE_A: g_eValueSubmitID = CAN_ID_TEMP_CABLE_A_ADJ_LOW; break;
            case ADJ_TEMP_CABLE_B: g_eValueSubmitID = CAN_ID_TEMP_CABLE_B_ADJ_LOW; break;
            case ADJ_TEMP_CJC: g_eValueSubmitID = CAN_ID_TEMP_CJC_ADJ_LOW; break;
            default: break;
        } 
    } 
    else {
        btn->style = STYLE_PRIMARY;
        btn->label = "ALTO";
        btn->bIsDirty = true;
        g_bIsHighPoint = true;

        switch(g_eAdjValueType) {
            case ADJ_VOLTAGE_PRIMARY: g_eValueSubmitID = CAN_ID_VOLTAGE_TX_PRIMARY_ADJ_HIGH; break;
            case ADJ_VOLTAGE_SECONDARY: g_eValueSubmitID = CAN_ID_VOLTAGE_TX_SECONDARY_ADJ_HIGH; break;
            case ADJ_CURRENT_PRIMARY: g_eValueSubmitID = CAN_ID_CURRENT_TX_PRIMARY_ADJ_HIGH; break;
            case ADJ_CURRENT_SECONDARY: g_eValueSubmitID = CAN_ID_CURRENT_TX_SECONDARY_ADJ_HIGH; break;
            case ADJ_TEMP_PROBE_MAIN: g_eValueSubmitID = CAN_ID_TEMP_PROBE_MAIN_ADJ_HIGH; break;
            case ADJ_TEMP_PROBE_SECONDARY: g_eValueSubmitID = CAN_ID_TEMP_PROBE_SECONDARY_ADJ_HIGH; break;
            case ADJ_TEMP_TX_PRIMARY: g_eValueSubmitID = CAN_ID_TEMP_TX_PRIMARY_ADJ_HIGH; break;
            case ADJ_TEMP_TX_SECONDARY: g_eValueSubmitID = CAN_ID_TEMP_TX_SECONDARY_ADJ_HIGH; break;
            case ADJ_TEMP_CABLE_A: g_eValueSubmitID = CAN_ID_TEMP_CABLE_A_ADJ_HIGH; break;
            case ADJ_TEMP_CABLE_B: g_eValueSubmitID = CAN_ID_TEMP_CABLE_B_ADJ_HIGH; break;
            case ADJ_TEMP_CJC: g_eValueSubmitID = CAN_ID_TEMP_CJC_ADJ_HIGH; break;
            default: break;
        } 
    }
}

static void onDotBtnReleased(gfx_Button *btn) {
    if (digitLen == 0) {
        strcpy(digitBuffer, "0.");
        digitLen = 2;
        displayValueData.bIsDirty = true;
    }
    else if (digitLen < NUMPAD_MAX_DIGITS && strchr(digitBuffer, '.') == NULL) {
        digitBuffer[digitLen++] = '.';
        digitBuffer[digitLen] = '\0';
        displayValueData.bIsDirty = true;
    }
    onGenericBtnRelease(btn);
}

// Nuevo Callback para el signo Negativo (Toggle)
static void onMinusBtnReleased(gfx_Button *btn) {
    if (digitLen > 0 && digitBuffer[0] == '-') {
        // Remover el signo negativo desplazando el arreglo a la izquierda
        memmove(digitBuffer, digitBuffer + 1, digitLen); // Esto mueve el '\0' también
        digitLen--;
        
        // Si nos quedamos sin caracteres después de remover el menos, volvemos a "0"
        if (digitLen == 0) {
            digitBuffer[0] = '0';
            digitBuffer[1] = '\0';
            digitLen = 1;
        }
    } else {
        // Añadir el signo negativo si tenemos espacio
        if (digitLen < NUMPAD_MAX_DIGITS) {
            // Desplazar a la derecha
            memmove(digitBuffer + 1, digitBuffer, digitLen + 1); // Movimiento incluye el '\0'
            digitBuffer[0] = '-';
            digitLen++;
        }
    }
    
    displayValueData.bIsDirty = true;
    onGenericBtnRelease(btn);
}

static void onDeleteBtnReleased(gfx_Button *btn) {
    if (digitLen > 0) {
        digitLen--;
        digitBuffer[digitLen] = '\0';

        if (digitLen == 0) {
            digitBuffer[0] = '0';
            digitBuffer[1] = '\0';
            digitLen = 1;
        }
        else if (digitLen == 1 && digitBuffer[0] == '.') {
            digitBuffer[0] = '0';
            digitBuffer[1] = '\0';
            digitLen = 1;
        }
        else if (digitLen == 1 && (digitBuffer[0] == '-' || digitBuffer[0] == ' ')) {
            digitBuffer[0] = '0';
            digitBuffer[1] = '\0';
            digitLen = 1;
        }

        displayValueData.bIsDirty = true;
    }
    onGenericBtnRelease(btn);
}

static void onTcTypeBtnReleased(gfx_Button *btn) {
	g_IsTcTypeModified = true;
    g_eCurrentTCType++;
    if (g_eCurrentTCType > THERMOCOUPLE_TYPE_B) {
        g_eCurrentTCType = THERMOCOUPLE_TYPE_K;
    }
    
    btn->label = (char*)tcTypeNames[g_eCurrentTCType];
    btn->bIsDirty = true;

    onGenericBtnRelease(btn);
}

static void onCancelBtnReleased(gfx_Button *btn) {
    digitLen = 0;
    digitBuffer[0] = '\0';
    displayValueData.bIsDirty = true;
    
    onGenericBtnRelease(btn);
    Event_Post(g_eFormCallback, (EventParam_t){.ptr = NULL});
}

static void onOkBtnReleased(gfx_Button *btn) {
    float finalValue = atof(digitBuffer);
    union can_data{uint8_t c[4]; float f;} can_data_t;
    can_data_t.f = finalValue;

	g_IsTcTypeModified = false;
    onGenericBtnRelease(btn);
    
    HAL_CAN_Msg_t msg;
    msg.id = g_eValueSubmitID;
    msg.length = sizeof(float);
    msg.data[0] = can_data_t.c[0];
    msg.data[1] = can_data_t.c[1];
    msg.data[2] = can_data_t.c[2];
    msg.data[3] = can_data_t.c[3];
    msg.isExtended = false;

	if(g_eAdjValueType == ADJ_TEMP_PROBE_MAIN || g_eAdjValueType == ADJ_TEMP_PROBE_SECONDARY || g_eAdjValueType == ADJ_TEMP_TX_PRIMARY || g_eAdjValueType == ADJ_TEMP_TX_SECONDARY || g_eAdjValueType == ADJ_TEMP_CABLE_A || g_eAdjValueType == ADJ_TEMP_CABLE_B || g_eAdjValueType == ADJ_TEMP_CJC) {
		msg.data[4] = (uint8_t)g_eCurrentTCType;
    	msg.length = sizeof(float) + sizeof(uint8_t);
	}
	memset(digitBuffer, 0, sizeof(digitBuffer));
	strcpy(digitBuffer, "0");
	digitLen = 1;
    HAL_CAN_Transmit(&msg);
}

// ========================================================
// CALLBACKS EXTERNOS (Eventos del Sistema)
// ========================================================
static void onShowThisFormEvent(EventParam_t arg) {
    g_eAdjValueType = (adj_value_e)arg.ui32;

	g_bIsHighPoint = false;
	onHighLowPointBtnReleased(&buttonData[10]);
    
    buttonData[15].bIsVisible = false;
    buttonData[15].bIsDirty = true;

    switch((adj_value_e)arg.ui32) {
        case ADJ_VOLTAGE_PRIMARY:
            g_eFormCallback = EVT_SYS_SHOW_ADJ_SELECT_VOLTAGE;
            g_eValueSubmitID = CAN_ID_VOLTAGE_TX_PRIMARY_ADJ_HIGH;
            strcpy(valueLabelBuffer, "VOLTAJE DEL\nPRIMARIO[V]");
            break;
        case ADJ_VOLTAGE_SECONDARY:
            g_eFormCallback = EVT_SYS_SHOW_ADJ_SELECT_VOLTAGE;
            g_eValueSubmitID = CAN_ID_VOLTAGE_TX_SECONDARY_ADJ_HIGH;
            strcpy(valueLabelBuffer, "VOLTAJE DEL\nSECUNDARIO [V]");
            break;
        case ADJ_CURRENT_PRIMARY:
            g_eFormCallback = EVT_SYS_SHOW_ADJ_SELECT_CURRENT;
            g_eValueSubmitID = CAN_ID_CURRENT_TX_PRIMARY_ADJ_HIGH;
            strcpy(valueLabelBuffer, "CORRIENTE DEL\nPRIMARIO [A]");
            break;
        case ADJ_CURRENT_SECONDARY:
            g_eFormCallback = EVT_SYS_SHOW_ADJ_SELECT_CURRENT;
            g_eValueSubmitID = CAN_ID_CURRENT_TX_SECONDARY_ADJ_HIGH;
            strcpy(valueLabelBuffer, "CORRIENTE DEL\nSECUNDARIO [A]");
            break;
        case ADJ_TEMP_PROBE_MAIN:
            g_eFormCallback = EVT_SYS_SHOW_ADJ_SELECT_TEMP;
            g_eValueSubmitID = CAN_ID_TEMP_PROBE_MAIN_ADJ_HIGH;
            strcpy(valueLabelBuffer, "TEMP. DE SONDA\n PRIMARIA [C]");
            buttonData[15].bIsVisible = true; 
            break;
        case ADJ_TEMP_PROBE_SECONDARY:
            g_eFormCallback = EVT_SYS_SHOW_ADJ_SELECT_TEMP;
            g_eValueSubmitID = CAN_ID_TEMP_PROBE_SECONDARY_ADJ_HIGH;
            strcpy(valueLabelBuffer, "TEMP. DE SONDA\nSECUNDARIA [C]");
            buttonData[15].bIsVisible = true; 
            break;
        case ADJ_TEMP_TX_PRIMARY:
            g_eFormCallback = EVT_SYS_SHOW_ADJ_SELECT_TEMP;
            g_eValueSubmitID = CAN_ID_TEMP_TX_PRIMARY_ADJ_HIGH;
            strcpy(valueLabelBuffer, "TEMP. PRIMARIA\nDEL TX [C]");
            buttonData[15].bIsVisible = true; 
            break;
        case ADJ_TEMP_TX_SECONDARY:
            g_eFormCallback = EVT_SYS_SHOW_ADJ_SELECT_TEMP;
            g_eValueSubmitID = CAN_ID_TEMP_TX_SECONDARY_ADJ_HIGH;
            strcpy(valueLabelBuffer, "TEMP. SECUNDARIA\nDEL TX [C]");
            buttonData[15].bIsVisible = true;
            break;
        case ADJ_TEMP_CABLE_A:
            g_eFormCallback = EVT_SYS_SHOW_ADJ_SELECT_TEMP;
            g_eValueSubmitID = CAN_ID_TEMP_CABLE_A_ADJ_HIGH;
            strcpy(valueLabelBuffer, "TEMP. CABLE A\nSUMINISTRO [C]");
            buttonData[15].bIsVisible = true; 
            break;
        case ADJ_TEMP_CABLE_B:
            g_eFormCallback = EVT_SYS_SHOW_ADJ_SELECT_TEMP;
            g_eValueSubmitID = CAN_ID_TEMP_CABLE_B_ADJ_HIGH;
            strcpy(valueLabelBuffer, "TEMP. CABLE B\nSUMINISTRO [C]");
            buttonData[15].bIsVisible = true; 
            break;
        case ADJ_TEMP_CJC:
            g_eFormCallback = EVT_SYS_SHOW_ADJ_SELECT_TEMP;
            g_eValueSubmitID = CAN_ID_TEMP_CJC_ADJ_HIGH;
            strcpy(valueLabelBuffer, "TEMP CJC [C]");
            buttonData[15].bIsVisible = true; 
            break;
        case ADJ_VARIAC_VOLTAGE:
            g_eFormCallback = EVT_SYS_SHOW_VARIAC_DEBUG_FORM; 
            g_eValueSubmitID = CAN_ID_REQ_VARIAC_SET_VOLTAGE;
            strcpy(valueLabelBuffer, "VARIAC\nVOLTAGE [V]");
            buttonData[15].bIsVisible = false; 
            break;
        default:
            break;
    }
}

static void onVoltagePrimaryValueEvent(EventParam_t arg) {
    snprintf(primaryVoltageBuff, sizeof(primaryVoltageBuff), "%.1f", arg.f32);
    if(g_eAdjValueType == ADJ_VOLTAGE_PRIMARY) {
        currValueData.text = primaryVoltageBuff;
        currValueData.bIsDirty = true;
    }
}

static void onVoltageSecondaryValueEvent(EventParam_t arg) {
    snprintf(secondaryVoltageBuff, sizeof(secondaryVoltageBuff), "%.1f", arg.f32);
    if(g_eAdjValueType == ADJ_VOLTAGE_SECONDARY) {
        currValueData.text = secondaryVoltageBuff;
        currValueData.bIsDirty = true;
    }
}

static void onCurrentPrimaryValueEvent(EventParam_t arg) {
    snprintf(primaryCurrentBuff, sizeof(primaryCurrentBuff), "%.1f", arg.f32);
    if(g_eAdjValueType == ADJ_CURRENT_PRIMARY) {
        currValueData.text = primaryCurrentBuff;
        currValueData.bIsDirty = true;
    }
}

static void onCurrentSecondaryValueEvent(EventParam_t arg) {
    snprintf(secondaryCurrentBuff, sizeof(secondaryCurrentBuff), "%.1f", arg.f32);
    if(g_eAdjValueType == ADJ_CURRENT_SECONDARY) {
        currValueData.text = secondaryCurrentBuff;
        currValueData.bIsDirty = true;
    }
}

static void onMainProbeTempValueEvent(EventParam_t arg) {
    snprintf(mainProbeTempBuff, sizeof(mainProbeTempBuff), "%.1f", arg.f32);
    if(g_eAdjValueType == ADJ_TEMP_PROBE_MAIN) {
        currValueData.text = mainProbeTempBuff;
        currValueData.bIsDirty = true;
    }
}

static void onSecondaryProbeTempValueEvent(EventParam_t arg) {
    snprintf(secondaryProbeTempBuff, sizeof(secondaryProbeTempBuff), "%.1f", arg.f32);
    if(g_eAdjValueType == ADJ_TEMP_PROBE_SECONDARY) {
        currValueData.text = secondaryProbeTempBuff;
        currValueData.bIsDirty = true;
    }
}

static void onTxPrimaryTempValueEvent(EventParam_t arg) {
    snprintf(primaryTxTempBuff, sizeof(primaryTxTempBuff), "%.1f", arg.f32);
    if(g_eAdjValueType == ADJ_TEMP_TX_PRIMARY) {
        currValueData.text = primaryTxTempBuff;
        currValueData.bIsDirty = true;
    }
}

static void onTxSecondaryTempValueEvent(EventParam_t arg) {
    snprintf(secondaryTxTempBuff, sizeof(secondaryTxTempBuff), "%.1f", arg.f32);
    if(g_eAdjValueType == ADJ_TEMP_TX_SECONDARY) {
        currValueData.text = secondaryTxTempBuff;
        currValueData.bIsDirty = true;
    }
}

static void onCableATempValueEvent(EventParam_t arg) {
    if(g_eAdjValueType == ADJ_TEMP_CABLE_A) {
    	snprintf(cableATempBuff, sizeof(cableATempBuff), "%.1f", arg.f32);
        currValueData.text = cableATempBuff;
        currValueData.bIsDirty = true;
    }
}

static void onCableBTempValueEvent(EventParam_t arg) {
    if(g_eAdjValueType == ADJ_TEMP_CABLE_B) {
    	snprintf(cableBTempBuff, sizeof(cableBTempBuff), "%.1f", arg.f32);
        currValueData.text = cableBTempBuff;
        currValueData.bIsDirty = true;
    }
}

static void onCJCTempValueEvent(EventParam_t arg) {
    if(g_eAdjValueType == ADJ_TEMP_CJC) {
    	snprintf(cjcTempBuff, sizeof(cjcTempBuff), "%.1f", arg.f32);
        currValueData.text = cjcTempBuff;
        currValueData.bIsDirty = true;
    }
}

static void onTCTXPrimary(EventParam_t arg) {
	if(g_eAdjValueType == ADJ_TEMP_TX_PRIMARY && g_IsTcTypeModified == false) {
		g_eCurrentTCType = (thermocouple_type_t)arg.ui32;
    	buttonData[15].label = (char*)tcTypeNames[g_eCurrentTCType];
    	buttonData[15].bIsDirty = true;
	}
}

static void onTCTXSecondary(EventParam_t arg) {
	if(g_eAdjValueType == ADJ_TEMP_TX_SECONDARY && g_IsTcTypeModified == false) {
		g_eCurrentTCType = (thermocouple_type_t)arg.ui32;
    	buttonData[15].label = (char*)tcTypeNames[g_eCurrentTCType];
    	buttonData[15].bIsDirty = true;
	}
}
static void onTCProbeMain(EventParam_t arg) {
	if(g_eAdjValueType == ADJ_TEMP_PROBE_MAIN && g_IsTcTypeModified == false) {
		g_eCurrentTCType = (thermocouple_type_t)arg.ui32;
    	buttonData[15].label = (char*)tcTypeNames[g_eCurrentTCType];
    	buttonData[15].bIsDirty = true;
	}
}
static void onTCProbeSecondary(EventParam_t arg) {
	if(g_eAdjValueType == ADJ_TEMP_PROBE_SECONDARY && g_IsTcTypeModified == false) {
		g_eCurrentTCType = (thermocouple_type_t)arg.ui32;
    	buttonData[15].label = (char*)tcTypeNames[g_eCurrentTCType];
    	buttonData[15].bIsDirty = true;
	}
}
static void onTCCableA(EventParam_t arg) {
	if(g_eAdjValueType == ADJ_TEMP_CABLE_A && g_IsTcTypeModified == false) {
		g_eCurrentTCType = (thermocouple_type_t)arg.ui32;
    	buttonData[15].label = (char*)tcTypeNames[g_eCurrentTCType];
    	buttonData[15].bIsDirty = true;
	}
}
static void onTCCableB(EventParam_t arg) {
	if(g_eAdjValueType == ADJ_TEMP_CABLE_B && g_IsTcTypeModified == false) {
		g_eCurrentTCType = (thermocouple_type_t)arg.ui32;
    	buttonData[15].label = (char*)tcTypeNames[g_eCurrentTCType];
    	buttonData[15].bIsDirty = true;
	}
}
static void onTCCJC(EventParam_t arg) {
	if(g_eAdjValueType == ADJ_TEMP_CJC && g_IsTcTypeModified == false) {
		g_eCurrentTCType = (thermocouple_type_t)arg.ui32;
    	buttonData[15].label = (char*)tcTypeNames[g_eCurrentTCType];
    	buttonData[15].bIsDirty = true;
	}
}
static void onVariacVoltageChanged(EventParam_t arg) {
    if(g_eAdjValueType == ADJ_VARIAC_VOLTAGE) {
    	snprintf(variacVoltageBuff, sizeof(variacVoltageBuff), "%.1f", arg.f32);
        currValueData.text = variacVoltageBuff;
        currValueData.bIsDirty = true;
    }
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
    btn->onRelease = releaseCallback;

    gfx_initRegTouch((void*)btn, WD_TYPE_BUTTON);

    buttonWidgets[index].eWidgetType = WD_TYPE_BUTTON;
    buttonWidgets[index].pvWidget = (void *)btn;
    
    canvasInsertAtTop(&g_sAdjustNumpadCanvas.psWidgets, &buttonWidgets[index]);
}

void initAdjustNumpadForm(void) {
    g_sAdjustNumpadCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
    g_sAdjustNumpadCanvas.psWidgets = NULL;

    // 1. TÍTULO DEL FORMULARIO
    formTitleData = (gfx_Label) {
        .name = "formTitleData",
        .text = "AJUSTES",
        .pos.x = 110,
        .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
        .bIsDirty = true
    };
    formTitleWidget.eWidgetType = WD_TYPE_LABEL;
    formTitleWidget.pvWidget = (void *)&formTitleData;
    
    int16_t baseX = 10;
    int16_t baseY = 105;
    int16_t totalWidth = LCD_WIDTH; 
    int16_t totalHeight = 400;

    uint16_t rowHeight = ((totalHeight - 5 * 15) / 5.0);
    uint16_t verticalSpacer = 10;
    uint16_t horizontalSpacer = 10;

    // 2. DISPLAY BACKGROUND
    currValueBgData = (gfx_Rectangle){
        .pos.x = LCD_WIDTH - totalWidth / 3.0f - 10,
        .pos.y = baseY,
        .dim.height = rowHeight,
        .dim.width = totalWidth / 3.0f,
        .borderWidth = 3,
        .color = g_pCurrentTheme->palette.surface,
        .round = 4,
    };
    currValueBgWidget.eWidgetType = WD_TYPE_RECT;
    currValueBgWidget.pvWidget = (void *)&currValueBgData;

    displayBgData = (gfx_Rectangle){
        .pos.x = LCD_WIDTH - totalWidth * ( 2.0f / 3.0f ) - 20,
        .pos.y = baseY,
        .dim.height = rowHeight,
        .dim.width = totalWidth / 3.0f,
        .borderWidth = 3,
        .color = g_pCurrentTheme->palette.surface,
        .round = 4,
    };
    displayBgWidget.eWidgetType = WD_TYPE_RECT;
    displayBgWidget.pvWidget = (void *)&displayBgData;

    inputLabelData = (gfx_Label) {
        .name = "inputLabel",
        .text = "VALOR DE AJUSTE",
        .pos.x = displayBgData.pos.x + displayBgData.dim.width / 2.0f,
        .pos.y = displayBgData.pos.y - 15,
        .typo = TYPO_BODY,
        .style = STYLE_PRIMARY,
        .isVisible = true,
        .alignment = ALIGN_CENTER,
    };
    inputLabelWidget.eWidgetType = WD_TYPE_LABEL;
    inputLabelWidget.pvWidget = (void *)&inputLabelData;

    currValLabelData = (gfx_Label) {
        .name = "currValLabel",
        .text = "VALOR ACTUAL",
        .pos.x = currValueBgData.pos.x + currValueBgData.dim.width / 2.0f,
        .pos.y = currValueBgData.pos.y - 15,
        .typo = TYPO_BODY,
        .style = STYLE_PRIMARY,
        .isVisible = true,
        .alignment = ALIGN_CENTER,
    };
    currValLabelWidget.eWidgetType = WD_TYPE_LABEL;
    currValLabelWidget.pvWidget = (void *)&currValLabelData;

    // 3. DISPLAY LABEL (Izquierda)
    displayLabelData = (gfx_Label){
        .name = "displayLabel",
        .text = valueLabelBuffer,
        .pos.x = baseX,
        .pos.y = baseY + rowHeight / 2,
        .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER),
        .isVisible = true,
        .style = STYLE_SECONDARY,
        .typo = TYPO_BODY,
        .bIsDirty = true
    };
    displayLabelWidget.eWidgetType = WD_TYPE_LABEL;
    displayLabelWidget.pvWidget = (void *)&displayLabelData;

    // 4. DISPLAY VALUE (Derecha)
    displayValueData = (gfx_Label) {
        .name = "displayValue",
        .text = digitBuffer,
        .pos.x = displayBgData.pos.x + displayBgData.dim.width - 10,
        .pos.y = baseY + rowHeight / 2,
        .alignment = (gfx_Align_e)(ALIGN_RIGHT | ALIGN_VCENTER),
        .isVisible = true,
        .style = STYLE_TEXT_MAIN,
        .typo = TYPO_H2,
        .bIsDirty = true
    };
    displayValueWidget.eWidgetType = WD_TYPE_LABEL;
    displayValueWidget.pvWidget = (void *)&displayValueData;

    currValueData = (gfx_Label) {
        .name = "currValue",
        .text = "0",
        .pos.x = currValueBgData.pos.x + currValueBgData.dim.width - 10,
        .pos.y = baseY + rowHeight / 2,
        .alignment = (gfx_Align_e)(ALIGN_RIGHT | ALIGN_VCENTER),
        .isVisible = true,
        .style = STYLE_TEXT_MAIN,
        .typo = TYPO_H2,
        .bIsDirty = true
    };
    currValueWidget.eWidgetType = WD_TYPE_LABEL;
    currValueWidget.pvWidget = (void *)&currValueData;

    useFullHeader(&g_sAdjustNumpadCanvas);
    canvasInsertAtTop(&g_sAdjustNumpadCanvas.psWidgets, &formTitleWidget);
    canvasInsertAtTop(&g_sAdjustNumpadCanvas.psWidgets, &displayBgWidget);
    canvasInsertAtTop(&g_sAdjustNumpadCanvas.psWidgets, &displayLabelWidget);
    canvasInsertAtTop(&g_sAdjustNumpadCanvas.psWidgets, &currValueBgWidget);
    canvasInsertAtTop(&g_sAdjustNumpadCanvas.psWidgets, &inputLabelWidget);
    canvasInsertAtTop(&g_sAdjustNumpadCanvas.psWidgets, &currValLabelWidget);
    canvasInsertAtTop(&g_sAdjustNumpadCanvas.psWidgets, &displayValueWidget);
    canvasInsertAtTop(&g_sAdjustNumpadCanvas.psWidgets, &currValueWidget);

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
    createNumpadButton(13, incrementalX + 3 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight, "REGRESAR", STYLE_TEXT_MUTED, onCancelBtnReleased);

    // Fila 3
    incrementalY += rowHeight + verticalSpacer;
    createNumpadButton(1,  incrementalX, incrementalY, buttonWidth, rowHeight, "1", STYLE_TEXT_MAIN, onNumberBtnReleased);
    createNumpadButton(2,  incrementalX + buttonWidth + horizontalSpacer, incrementalY, buttonWidth, rowHeight, "2", STYLE_TEXT_MAIN, onNumberBtnReleased);
    createNumpadButton(3,  incrementalX + 2 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight, "3", STYLE_TEXT_MAIN, onNumberBtnReleased);
    
    // Botón de Termopar (index 15) posicionado encima del botón "AJUSTAR"
    createNumpadButton(15, incrementalX + 3 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight, (char*)tcTypeNames[g_eCurrentTCType], STYLE_DEFAULT, onTcTypeBtnReleased);

    // Fila 4
    incrementalY += rowHeight + verticalSpacer;
    
    // Cálculo para dividir el espacio de la tercera columna en dos botones (Punto y Menos)
    uint16_t halfButtonWidth = (buttonWidth - horizontalSpacer) / 2.0f;
    uint16_t dotPosX = incrementalX + 2 * ( buttonWidth + horizontalSpacer );
    uint16_t minusPosX = dotPosX + halfButtonWidth + horizontalSpacer;
    
    createNumpadButton(0,  incrementalX, incrementalY, buttonWidth, rowHeight, "0", STYLE_TEXT_MAIN, onNumberBtnReleased);
    createNumpadButton(10, incrementalX + buttonWidth + horizontalSpacer, incrementalY, buttonWidth, rowHeight, "ALTO", STYLE_PRIMARY, onHighLowPointBtnReleased);
    
    // Botones divididos: "." y "-"
    createNumpadButton(11, dotPosX, incrementalY, halfButtonWidth, rowHeight, ".", STYLE_TEXT_MAIN, onDotBtnReleased);
    createNumpadButton(16, minusPosX, incrementalY, halfButtonWidth, rowHeight, "-", STYLE_TEXT_MAIN, onMinusBtnReleased); // Nuevo Botón

    // Botón AJUSTAR
    createNumpadButton(12, incrementalX + 3 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight, "AJUSTAR", STYLE_SUCCESS, onOkBtnReleased);

    // 6. EVENTOS Y REGISTRO DEL FORMULARIO
    Event_Subscribe(EVT_SYS_SHOW_ADJ_NUMPAD_FORM, (EventHandler_fn)onShowThisFormEvent);
    Event_Subscribe(EVT_CAN_INST_VOLTAGE_PRIMARY, (EventHandler_fn)onVoltagePrimaryValueEvent);
    Event_Subscribe(EVT_CAN_INST_VOLTAGE_SECONDARY, (EventHandler_fn)onVoltageSecondaryValueEvent);
    Event_Subscribe(EVT_CAN_INST_CURRENT_PRIMARY, (EventHandler_fn)onCurrentPrimaryValueEvent);
    Event_Subscribe(EVT_CAN_INST_CURRENT_SECUNDARY, (EventHandler_fn)onCurrentSecondaryValueEvent);
    Event_Subscribe(EVT_CAN_INST_TEMP_PROBE_MAIN, (EventHandler_fn)onMainProbeTempValueEvent);
    Event_Subscribe(EVT_CAN_INST_TEMP_PROBE_SECONDARY, (EventHandler_fn)onSecondaryProbeTempValueEvent);
    Event_Subscribe(EVT_CAN_INST_TEMP_TX_PRIMARY, (EventHandler_fn)onTxPrimaryTempValueEvent);
    Event_Subscribe(EVT_CAN_INST_TEMP_TX_SECONDARY, (EventHandler_fn)onTxSecondaryTempValueEvent);
	Event_Subscribe(EVT_CAN_INST_TEMP_CABLE_A, ( EventHandler_fn )onCableATempValueEvent);
	Event_Subscribe(EVT_CAN_INST_TEMP_CABLE_B, ( EventHandler_fn )onCableBTempValueEvent);
	Event_Subscribe(EVT_CAN_INST_TEMP_CJC, ( EventHandler_fn )onCJCTempValueEvent );

	Event_Subscribe(EVT_CAN_INST_TC_TX_PRIMARY, (EventHandler_fn)onTCTXPrimary);
	Event_Subscribe(EVT_CAN_INST_TC_TX_SECONDARY, (EventHandler_fn)onTCTXSecondary);
	Event_Subscribe(EVT_CAN_INST_TC_PROBE_MAIN, (EventHandler_fn)onTCProbeMain);
	Event_Subscribe(EVT_CAN_INST_TC_PROBE_SECONDARY, (EventHandler_fn)onTCProbeSecondary);
	Event_Subscribe(EVT_CAN_INST_TC_CABLE_A, (EventHandler_fn)onTCCableA);
	Event_Subscribe(EVT_CAN_INST_TC_CABLE_B, (EventHandler_fn)onTCCableB);
	Event_Subscribe(EVT_CAN_INST_TC_CJC, (EventHandler_fn)onTCCJC);

    Event_Subscribe(EVT_CAN_INST_VARIAC_VOLTAGE, (EventHandler_fn)onVariacVoltageChanged);
    
    g_i16AdjustNumpadIndex = FormManager_AddForm(&g_sAdjustNumpadCanvas);
}