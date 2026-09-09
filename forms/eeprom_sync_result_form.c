#include <string.h>
#include <stdio.h>

#include "gui_core.h"
#include "gui_canvas.h"
#include "forms_manager.h"
#include "gui_theme.h"
#include "FT8xx_params.h"
#include "event_engine.h"
#include "common_widgets.h"

#include "eeprom_sync_result_form.h"

// ========================================================
// VARIABLES DEL FORMULARIO
// ========================================================
int16_t g_i16EepromSyncIndex = 0; 
static gfx_Canvas g_sEepromSyncCanvas;

// Widgets
static gfx_GenericWidget operationWidget, statusWidget, okBtnWidget, formTitleWidget;
static gfx_Label operationNameData, statusData, formTitleData;
static gfx_Button okBtnData;

static char titleBuf[40] = "FACTORY RESET";
static char statusBuf[40] = "WAITING FOR RESPONSE...";

// ========================================================
// CALLBACKS DE UI
// ========================================================
static void onOkBtnRelease(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_OPTIONS_FORM, (EventParam_t){.ptr = NULL});
}

// ========================================================
// EVENT HANDLERS (Suscripciones)
// ========================================================

// Evento para preparar y mostrar la pantalla (arg.bool_ = true para Factory Reset)
static void onShowSyncForm(EventParam_t arg) {
    bool isFactoryReset = arg.bool_;
    
    if (isFactoryReset) {
        strcpy(operationNameData.text, "FACTORY RESET");
    } else {
        strcpy(operationNameData.text, "SAVE TO EEPROM");
    }
    
    strncpy(statusData.text, "WAITING FOR RESPONSE...", sizeof(statusData.text) - 1);
    statusData.style = STYLE_DANGER; 
    
    operationNameData.bIsDirty = true;
    statusData.bIsDirty = true;

    //Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

// Evento disparado por inst_can_buffer.c al recibir el ACK
static void onEepromAckReceived(EventParam_t arg) {
    bool bSuccess = arg.bool_;

    if (bSuccess) {
        strcpy(statusBuf, "OPERATION SUCCESSFUL!");
        statusData.style = STYLE_SUCCESS;
    } else {
        strcpy(statusBuf, "OPERATION FAILED!");
        statusData.style = STYLE_DANGER; 
    }
    
    statusData.bIsDirty = true;

    Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

// ========================================================
// INICIALIZACIÓN
// ========================================================
void initEepromSyncForm(void) {
    g_sEepromSyncCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
    g_sEepromSyncCanvas.psWidgets = NULL; 
    
    formTitleData = (gfx_Label) {
        .name = "formTitleData",
        .text = "EEPROM",
        .pos.x = 125, .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
    };
    formTitleWidget.eWidgetType = WD_TYPE_LABEL; formTitleWidget.pvWidget = &formTitleData;

    // Título
    operationNameData = (gfx_Label) {
        .name = "syncTitle", .text = titleBuf,
        .pos.x = LCD_WIDTH / 2, .pos.y = LCD_HEIGHT / 2 - 60, 
        .alignment = ALIGN_CENTER, .typo = TYPO_H2,           
        .style = STYLE_TEXT_MAIN, .isVisible = true,
    };
    operationWidget.eWidgetType = WD_TYPE_LABEL; operationWidget.pvWidget = &operationNameData;

    // Estado
    statusData = (gfx_Label){
        .name = "syncStatus", .text = statusBuf,
        .pos.x = LCD_WIDTH / 2, .pos.y = LCD_HEIGHT / 2 + 10,
        .alignment = ALIGN_CENTER, .style = STYLE_DANGER,
        .typo = TYPO_H3, .isVisible = true,
    };
    statusWidget.eWidgetType = WD_TYPE_LABEL; statusWidget.pvWidget = &statusData;

    // Botón OK
    okBtnData = (gfx_Button) {
        .label = "OK",
        .pos.x = (LCD_WIDTH / 2) - 80, .pos.y = LCD_HEIGHT / 2 + 80, 
        .size.width = 160, .size.height = 50,
        .borderWidth = 3, .radius = 4, .state = BTN_STATE_NORMAL, 
        .style = STYLE_DEFAULT, .typo = TYPO_H3, .bIsVisible = true, 
        .onPressed = onGenericBtnPressed, .onRelease = onOkBtnRelease,
    };
    gfx_initRegTouch((void *)&okBtnData, WD_TYPE_BUTTON);
    okBtnWidget.eWidgetType = WD_TYPE_BUTTON; okBtnWidget.pvWidget = &okBtnData;

    // Ensamblaje
    useFullHeader(&g_sEepromSyncCanvas); 
    
	canvasInsertAtTop(&g_sEepromSyncCanvas.psWidgets, &formTitleWidget);
    canvasInsertAtTop(&g_sEepromSyncCanvas.psWidgets, &operationWidget);
    canvasInsertAtTop(&g_sEepromSyncCanvas.psWidgets, &statusWidget);
    canvasInsertAtTop(&g_sEepromSyncCanvas.psWidgets, &okBtnWidget);
    
    // Suscripciones a Eventos
    Event_Subscribe(EVT_SYS_SHOW_EEPROM_SYNC_RESULT_FORM, (EventHandler_fn)onShowSyncForm);
    Event_Subscribe(EVT_CAN_INST_ACK_FACTORY_RESET, (EventHandler_fn)onEepromAckReceived);
    Event_Subscribe(EVT_CAN_INST_ACK_SAVE_EEPROM, (EventHandler_fn)onEepromAckReceived);

    g_i16EepromSyncIndex = FormManager_AddForm(&g_sEepromSyncCanvas);
}
