#include <stdint.h>
#include <stddef.h>

#include "FT8xx_params.h"
#include "can_id_map.h"
#include "event_engine.h"
#include "forms_manager.h"
#include "gui_canvas.h"
#include "gui_core.h"
#include "gui_theme.h"
#include "hal_inst_can.h"

#include "common_widgets.h"
#include "factory_reset_confirmation_form.h"

int16_t g_i16FactoryResetConfirmationIndex = 0;

static gfx_Canvas g_sFactoryResetConfirmationCanvas;
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget warningWidget;
static gfx_GenericWidget descriptionWidget;
static gfx_GenericWidget cancelButtonWidget;
static gfx_GenericWidget confirmButtonWidget;
static gfx_Label formTitleData;
static gfx_Label warningData;
static gfx_Label descriptionData;
static gfx_Button cancelButtonData;
static gfx_Button confirmButtonData;

static void onCancelButtonReleased(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_OPTIONS_FORM, (EventParam_t){.ptr = NULL});
}

static void onConfirmButtonReleased(gfx_Button *btn) {
    HAL_CAN_Msg_t msg = {
        .id = CAN_ID_REQ_FACTORY_RESET,
        .isExtended = false,
        .length = 0,
    };

    onGenericBtnRelease(btn);
    HAL_CAN_Transmit(&msg);
    Event_Post(EVT_SYS_SHOW_EEPROM_SYNC_RESULT_FORM, (EventParam_t){.bool_ = true});
}

void initFactoryResetConfirmationForm(void) {
    g_sFactoryResetConfirmationCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
    g_sFactoryResetConfirmationCanvas.psWidgets = NULL;

    formTitleData = (gfx_Label) {
        .name = "formTitleData",
        .text = "AJUSTES",
        .pos.x = 125,
        .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3,
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
    };
    formTitleWidget.eWidgetType = WD_TYPE_LABEL;
    formTitleWidget.pvWidget = &formTitleData;

    warningData = (gfx_Label) {
        .name = "factoryResetWarning",
        .text = "CONFIRMAR FACTORY RESET",
        .pos.x = LCD_WIDTH / 2,
        .pos.y = 145,
        .alignment = ALIGN_CENTER,
        .typo = TYPO_H1,
        .style = STYLE_DANGER,
        .isVisible = true,
    };
    warningWidget.eWidgetType = WD_TYPE_LABEL;
    warningWidget.pvWidget = &warningData;

    descriptionData = (gfx_Label) {
        .name = "factoryResetDescription",
        .text = "Esta accion restaura configuracion de \nfabrica y no se puede deshacer.",
        .pos.x = LCD_WIDTH / 2,
        .pos.y = 220,
        .alignment = ALIGN_CENTER,
        .typo = TYPO_H3,
        .style = STYLE_TEXT_MUTED,
        .isVisible = true,
    };
    descriptionWidget.eWidgetType = WD_TYPE_LABEL;
    descriptionWidget.pvWidget = &descriptionData;

    cancelButtonData = (gfx_Button) {
        .label = "CANCELAR",
        .pos.x = LCD_WIDTH / 8,
        .pos.y = 360,
        .size.width = LCD_WIDTH / 3,
        .size.height = 70,
        .borderWidth = 3,
        .radius = 3,
        .state = BTN_STATE_NORMAL,
        .style = STYLE_DEFAULT,
        .typo = TYPO_H3,
        .bIsVisible = true,
        .onPressed = onGenericBtnPressed,
        .onRelease = onCancelButtonReleased,
    };
    gfx_initRegTouch((void *)&cancelButtonData, WD_TYPE_BUTTON);
    cancelButtonWidget.eWidgetType = WD_TYPE_BUTTON;
    cancelButtonWidget.pvWidget = &cancelButtonData;

    confirmButtonData = (gfx_Button) {
        .label = "RESET FACTORY",
        .pos.x = LCD_WIDTH - (LCD_WIDTH / 8) - (LCD_WIDTH / 3),
        .pos.y = 360,
        .size.width = LCD_WIDTH / 3,
        .size.height = 70,
        .borderWidth = 3,
        .radius = 3,
        .state = BTN_STATE_NORMAL,
        .style = STYLE_DANGER,
        .typo = TYPO_H3,
        .bIsVisible = true,
        .onPressed = onGenericBtnPressed,
        .onRelease = onConfirmButtonReleased,
    };
    gfx_initRegTouch((void *)&confirmButtonData, WD_TYPE_BUTTON);
    confirmButtonWidget.eWidgetType = WD_TYPE_BUTTON;
    confirmButtonWidget.pvWidget = &confirmButtonData;

    useFullHeader(&g_sFactoryResetConfirmationCanvas);
    canvasInsertAtTop(&g_sFactoryResetConfirmationCanvas.psWidgets, &formTitleWidget);
    canvasInsertAtTop(&g_sFactoryResetConfirmationCanvas.psWidgets, &warningWidget);
    canvasInsertAtTop(&g_sFactoryResetConfirmationCanvas.psWidgets, &descriptionWidget);
    canvasInsertAtTop(&g_sFactoryResetConfirmationCanvas.psWidgets, &cancelButtonWidget);
    canvasInsertAtTop(&g_sFactoryResetConfirmationCanvas.psWidgets, &confirmButtonWidget);

    g_i16FactoryResetConfirmationIndex = FormManager_AddForm(&g_sFactoryResetConfirmationCanvas);
}
