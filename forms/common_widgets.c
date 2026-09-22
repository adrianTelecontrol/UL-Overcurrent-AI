
#include <stdlib.h>
#include <string.h>

#include "event_engine.h"
#include "helpers.h"
#include "FT8xx_params.h"
#include "gui_core.h"
#include "gui_canvas.h"
#include "gui_colors.h"
#include "font_engine.h"
#include "gui_theme.h"
#include "forms_manager.h"
#include "file_manager.h"
#include "icon_map.h"
#include "experiments_cfg.h"
#include "hal_inst_can.h"
#include "can_id_map.h"

#include "common_widgets.h"

#ifndef EVE_FREE_RAMG_START
#define EVE_FREE_RAMG_START		768000 + 200
#endif

static gfx_GenericWidget titleWidget;
static gfx_GenericWidget btnInicioWidget;
static gfx_GenericWidget btnTestWidget;
static gfx_GenericWidget btnOptionsWidget;
static gfx_GenericWidget headerPanelWidget;
static gfx_GenericWidget tcLogoImgWidget, tcBlkLogoImgWidget;
static gfx_GenericWidget dateWidget;
static gfx_GenericWidget timeWidget;
static gfx_GenericWidget emergencyStopWidget;

static gfx_GenericWidget homeIconWidget;

static gfx_Button btnInicioData;
static gfx_Button btnTestData;
static gfx_Button btnOptionsData;

static gfx_Label homeIconData;

static gfx_Label titleData;
static gfx_Image tcLogoImgData, tcBlkLogoImgData;
static gfx_Rectangle headerPanelData;
static gfx_Label dateData;
static gfx_Label timeData;
static gfx_Button emergencyStopData;

static char dateBuffer[12] = "21/04/2026";
static char timeBuffer[12] = "15:35:12";

static const char TAG[] = "commonWidgets";
// ==========================================
// Callbacks (Botones)
// ==========================================
static void onInicioBtnReleased(gfx_Button *btn) {
	btnTestData.style = STYLE_DEFAULT;
	btnTestData.state = BTN_STATE_NORMAL;
	btnTestData.bIsDirty = true;
	btnOptionsData.style = STYLE_DEFAULT;
	btnOptionsData.state = BTN_STATE_NORMAL;
	btnOptionsData.bIsDirty = true;

    btn->state = BTN_STATE_NORMAL;
	btn->style = STYLE_DANGER;
    btn->bIsDirty = true;
    Event_Post(EVT_SYS_SHOW_HOME_FORM, (EventParam_t){.ptr = NULL}); // Ejemplo de evento de navegación
}

static void onTestsBtnReleased(gfx_Button *btn) {
	btnInicioData.style = STYLE_DEFAULT;
	btnInicioData.state = BTN_STATE_NORMAL;
	btnInicioData.bIsDirty = true;
	btnOptionsData.style = STYLE_DEFAULT;
	btnOptionsData.state = BTN_STATE_NORMAL;
	btnOptionsData.bIsDirty = true;
	
    btn->state = BTN_STATE_NORMAL;
	btn->style = STYLE_DANGER;
    btn->bIsDirty = true;
	Event_Post(EVT_SYS_SHOW_TEST_SELECTION_FORM, (EventParam_t){.ptr = NULL});
}

static void onOptionsBtnReleased(gfx_Button *btn) {
	btnTestData.style = STYLE_DEFAULT;
	btnTestData.state = BTN_STATE_NORMAL;
	btnTestData.bIsDirty = true;
	btnInicioData.style = STYLE_DEFAULT;
	btnInicioData.state = BTN_STATE_NORMAL;
	btnInicioData.bIsDirty = true;

    btn->state = BTN_STATE_NORMAL;
	btn->style = STYLE_DANGER;
    btn->bIsDirty = true;

	Event_Post(EVT_SYS_SHOW_OPTIONS_FORM, (EventParam_t){.ptr = NULL});
    //Event_Post(EVT_CMD_SHOW_GRAPH_FORM, (EventParam_t){.ptr = NULL});
    //Event_Post(EVT_CMD_NAV_GRAPH, 0);
}


static void onEmergencyStopBtnRelease(gfx_Button *btn) {
	onGenericBtnRelease(btn);	
	
	// TEST ONLY
	HAL_CAN_Msg_t msg;
	msg.id = CAN_ID_REQ_EMERGENCY_STOP;
	msg.isExtended = false;
	msg.length = 0;

	HAL_CAN_Transmit(&msg);

	Event_Post(EVT_SYS_SHOW_HOME_FORM, (EventParam_t){.ptr = NULL});
}

static void onDateChanged(EventParam_t arg) {
	if(arg.str == NULL) return;

	strcpy(dateBuffer, arg.str);
	dateData.bIsDirty = true;
}
static void onRTCTimeChanged(EventParam_t arg) {
	if(arg.str == NULL) return;

	strcpy(timeBuffer, arg.str);
	timeData.bIsDirty = true;
}

static void onShowHomeForm(void) {
	btnTestData.style = STYLE_DEFAULT;
	btnTestData.state = BTN_STATE_NORMAL;
	btnTestData.bIsDirty = true;
	btnOptionsData.style = STYLE_DEFAULT;
	btnOptionsData.state = BTN_STATE_NORMAL;
	btnOptionsData.bIsDirty = true;

	btnInicioData.state = BTN_STATE_NORMAL;
	btnInicioData.style = STYLE_DANGER;
	btnInicioData.bIsDirty = true;
}

static void onShowSelectionForm(void) {
	btnInicioData.style = STYLE_DEFAULT;
	btnInicioData.state = BTN_STATE_NORMAL;
	btnInicioData.bIsDirty = true;
	btnOptionsData.style = STYLE_DEFAULT;
	btnOptionsData.state = BTN_STATE_NORMAL;
	btnOptionsData.bIsDirty = true;
	
    btnTestData.state = BTN_STATE_NORMAL;
    btnTestData.style = STYLE_DANGER;
    btnTestData.bIsDirty = true;
}

static void onShowOptionsForm(void) {
	btnTestData.style = STYLE_DEFAULT;
	btnTestData.state = BTN_STATE_NORMAL;
	btnTestData.bIsDirty = true;
	btnInicioData.style = STYLE_DEFAULT;
	btnInicioData.state = BTN_STATE_NORMAL;
	btnInicioData.bIsDirty = true;

    btnOptionsData.state = BTN_STATE_NORMAL;
    btnOptionsData.style = STYLE_DANGER;
    btnOptionsData.bIsDirty = true;
	
}

static void onThemeChanged(EventParam_t arg) {
	(void)arg;
	headerPanelData.color = g_pCurrentTheme->palette.surface;
	headerPanelData.bIsDirty = true;
}

void initCommonWidgets(void) {

    dateData = (gfx_Label){
        .text = dateBuffer,
        .name = "dateWidget",
        .pos.x = LCD_WIDTH / 2.0 + 130, // Esquina superior derecha
        .pos.y = 25,
        .alignment = ALIGN_RIGHT,
        .typo = TYPO_CAPTION,           
        .style = STYLE_TEXT_MUTED,
        .isVisible = true,
    };
    dateWidget.eWidgetType = WD_TYPE_LABEL; dateWidget.pvWidget = (void *)&dateData;

    timeData = (gfx_Label){
        .text = timeBuffer,
        .name = "timeWidget",
        .pos.x = LCD_WIDTH / 2.0 + 130, // Esquina superior derecha
        .pos.y = 55,
        .alignment = ALIGN_RIGHT,
        .typo = TYPO_BODY,           
        .style = STYLE_TEXT_MAIN_BOLD,
        .isVisible = true,
    };
    timeWidget.eWidgetType = WD_TYPE_LABEL; timeWidget.pvWidget = (void *)&timeData;

	emergencyStopData = (gfx_Button){
		.label = "EMERGENCY STOP",
		.size.height = 55,
		.size.width = 245,
		.pos.x = timeData.pos.x + 20,
		.pos.y = 5,
		.name = "emergencyStop",
		.onPosChanged = NULL,
		.onPressed = onGenericBtnPressed,
		.onRelease = onEmergencyStopBtnRelease,
		.radius = 5,
		.state = BTN_STATE_NORMAL,
		.style = STYLE_DANGER,
		.typo = TYPO_MONO_BOLD,
		.borderWidth = 1,
		.bIsVisible = true,
	};
	gfx_initRegTouch((void *)&emergencyStopData, WD_TYPE_BUTTON);
	emergencyStopWidget.eWidgetType = WD_TYPE_BUTTON; emergencyStopWidget.pvWidget = (void *)&emergencyStopData;

    uint16_t btnWidth = 260;
    uint16_t btnHeight = 60;
    uint16_t btnY = 420;

    btnInicioData = (gfx_Button){
        .name = "btnIni", .label = "Inicio", .size.width = btnWidth, .size.height = btnHeight,
        .pos.x = 5, .pos.y = btnY, .oldPos.x = 65, .oldPos.y = btnY,
        .typo = TYPO_H3, .style = STYLE_PRIMARY, .borderWidth = 0, .radius = 5,
        .state = BTN_STATE_NORMAL, .onPressed = onGenericBtnPressed, .onRelease = onInicioBtnReleased,
		.bIsVisible = true,
    };
    btnInicioWidget.eWidgetType = WD_TYPE_BUTTON; btnInicioWidget.pvWidget = (void *)&btnInicioData;
    gfx_initRegTouch(btnInicioWidget.pvWidget, WD_TYPE_BUTTON);

	homeIconData = (gfx_Label) {
		.name = "homeIcon",
		.alignment = ALIGN_CENTER,
		.isVisible = true,
		.pos.x = btnInicioData.pos.x + 40,
		.pos.y = btnInicioData.pos.y + btnHeight / 2,
		.typo = TYPO_ICON,
		.text = ICON_HOME,
		.style = STYLE_TEXT_MAIN_BOLD,
		.bIsDirty = true
	};
	homeIconWidget.eWidgetType = WD_TYPE_LABEL; homeIconWidget.pvWidget = (void *)&homeIconData;

    btnTestData = (gfx_Button){
        .name = "btnCfg", .label = "Pruebas", .size.width = btnWidth, .size.height = btnHeight,
        .pos.x = btnWidth + 10, .pos.y = btnY, .oldPos.x = 310, .oldPos.y = btnY,
        .typo = TYPO_H3, .style = STYLE_DEFAULT, .borderWidth = 0, .radius = 5,
        .state = BTN_STATE_NORMAL, .onPressed = onGenericBtnPressed, .onRelease = onTestsBtnReleased,
		.bIsVisible = true,
    };
    btnTestWidget.eWidgetType = WD_TYPE_BUTTON; btnTestWidget.pvWidget = (void *)&btnTestData;
    gfx_initRegTouch(btnTestWidget.pvWidget, WD_TYPE_BUTTON);

    btnOptionsData = (gfx_Button){
        .name = "btnGraph", .label = "Opciones", .size.width = btnWidth, .size.height = btnHeight,
        .pos.x = btnWidth * 2 + 15, .pos.y = btnY, .oldPos.x = 555, .oldPos.y = btnY,
        .typo = TYPO_H3, .style = STYLE_DEFAULT, .borderWidth = 0, .radius = 5,
        .state = BTN_STATE_NORMAL, .onPressed = onGenericBtnPressed, .onRelease = onOptionsBtnReleased,
		.bIsVisible = true,
    };
    btnOptionsWidget.eWidgetType = WD_TYPE_BUTTON; btnOptionsWidget.pvWidget = (void *)&btnOptionsData;
    gfx_initRegTouch(btnOptionsWidget.pvWidget, WD_TYPE_BUTTON);

	tcBlkLogoImgData = (gfx_Image) {
		.name = "mainLogo",
		.pos.x = 5,
		.pos.y = 15,
		.scale = 1,
	};

	FM_EVEImageRegistryReset(EVE_FREE_RAMG_START);
	if(!FM_LoadEVEImage(DRIVE_SD_ID, "logo_blk_corrected.png", &tcBlkLogoImgData, 0, NULL)) {
		TIVA_LOGE(TAG, "Fallo al cargar logo_blk_corrected.png en EVE");
	}
	tcBlkLogoImgWidget.eWidgetType = WD_TYPE_IMAGE;
	tcBlkLogoImgWidget.pvWidget = (void *)&tcBlkLogoImgData;

	tcLogoImgData = (gfx_Image) {
		.name = "mainLogo",
		.pos.x = 5,
		.pos.y = 5,
		.scale = 1,
		.hasTransparency = true,
	};

	if(!FM_LoadEVEImage(DRIVE_SD_ID, "logo_tc.png", &tcLogoImgData, 0, NULL)) {
		TIVA_LOGE(TAG, "Fallo al cargar logo_tc.png en EVE");
	}
	tcLogoImgWidget.eWidgetType = WD_TYPE_IMAGE;
	tcLogoImgWidget.pvWidget = (void *)&tcLogoImgData;
	
	headerPanelData = (gfx_Rectangle){
		.dim.width = LCD_WIDTH,
		.dim.height = 65,
		.pos.x = 0,
		.pos.y = 0,
		.color = g_pCurrentTheme->palette.surface,
		.round = 0,
		.borderWidth = 1,
	};
	headerPanelWidget.eWidgetType = WD_TYPE_RECT; headerPanelWidget.pvWidget = (void *)&headerPanelData;

	Event_Subscribe(EVT_SYS_DATE_CHANGED, (EventHandler_fn)onDateChanged);
	Event_Subscribe(EVT_SYS_TIME_CHANGED, (EventHandler_fn)onRTCTimeChanged);
	Event_Subscribe(EVT_SYS_SHOW_HOME_FORM, (EventHandler_fn)onShowHomeForm);
	Event_Subscribe(EVT_SYS_SHOW_TEST_SELECTION_FORM, (EventHandler_fn)onShowSelectionForm);
	Event_Subscribe(EVT_SYS_SHOW_OPTIONS_FORM, (EventHandler_fn)onShowOptionsForm);
	Event_Subscribe(EVT_CMD_CHANGE_THEME, (EventHandler_fn)onThemeChanged);
}

void useNavigationButtons(gfx_Canvas* canvas) {
	if(canvas == NULL) return;

    canvasInsertAtTop(&canvas->psWidgets, &btnInicioWidget);
    canvasInsertAtTop(&canvas->psWidgets, &btnTestWidget);
    canvasInsertAtTop(&canvas->psWidgets, &btnOptionsWidget);
    //canvasInsertAtTop(&canvas->psWidgets, &homeIconWidget);
}

void useHeaderPanelWidget(gfx_Canvas* canvas) {
	if(canvas == NULL) return;

    canvasInsertAtTop(&canvas->psWidgets, &headerPanelWidget);
}

void useLogoWidget(gfx_Canvas* canvas) {
	if(canvas == NULL) return;

    canvasInsertAtTop(&canvas->psWidgets, &tcLogoImgWidget);

}

void useBlkLogoWidget(gfx_Canvas* canvas) {
	if(canvas == NULL) return;

    canvasInsertAtTop(&canvas->psWidgets, &tcBlkLogoImgWidget);

}

void useFullHeader(gfx_Canvas* canvas) {
	if(canvas == NULL) return;

	canvasInsertAtTop(&canvas->psWidgets, &headerPanelWidget);
	canvasInsertAtTop(&canvas->psWidgets, &tcLogoImgWidget);
	canvasInsertAtTop(&canvas->psWidgets, &dateWidget);
	canvasInsertAtTop(&canvas->psWidgets, &timeWidget);
	canvasInsertAtTop(&canvas->psWidgets, &emergencyStopWidget);
}

void useFullHeaderNoEmergencyBtn(gfx_Canvas* canvas) {
	if(canvas == NULL) return;

	canvasInsertAtTop(&canvas->psWidgets, &headerPanelWidget);
	canvasInsertAtTop(&canvas->psWidgets, &tcLogoImgWidget);
	canvasInsertAtTop(&canvas->psWidgets, &dateWidget);
	canvasInsertAtTop(&canvas->psWidgets, &timeWidget);
	//canvasInsertAtTop(&canvas->psWidgets, &emergencyStopWidget);
}

void useHeaderNoClock(gfx_Canvas* canvas) {
	if(canvas == NULL) return;

	canvasInsertAtTop(&canvas->psWidgets, &headerPanelWidget);
	canvasInsertAtTop(&canvas->psWidgets, &tcLogoImgWidget);
	canvasInsertAtTop(&canvas->psWidgets, &dateWidget);
	canvasInsertAtTop(&canvas->psWidgets, &timeWidget);
	canvasInsertAtTop(&canvas->psWidgets, &emergencyStopWidget);
}
