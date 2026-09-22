#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gui_core.h"
#include "FT8xx_params.h"
#include "form_common.h"
#include "EVE_colors.h"
#include "gui_canvas.h"
#include "forms_manager.h"

#include "common_widgets.h"
#include "helpers.h"
#include "gui_theme.h"
#include "event_engine.h"

#include "boot_form.h"

extern const char *APP_NAME;
extern const char *BOARD_VER;
extern const char *FIRMWARE_VER;

static gfx_Canvas g_sBootCanvas;

int16_t g_i16BootFormID = 0;

// ==========================================
// 1. The Generic Nodes
// ==========================================
// Header
gfx_GenericWidget tcLogoBgWidget;
gfx_GenericWidget mainTitleWidget;
gfx_GenericWidget subTitleWidget;

// Terminal Logs
gfx_GenericWidget log1Widget;
gfx_GenericWidget log2Widget;
gfx_GenericWidget log3Widget;
gfx_GenericWidget log4Widget;
gfx_GenericWidget log5Widget;
gfx_GenericWidget log6Widget;
gfx_GenericWidget log7Widget;
gfx_GenericWidget log8Widget;

gfx_GenericWidget logTimestamp1Widget;
gfx_GenericWidget logTimestamp2Widget;
gfx_GenericWidget logTimestamp3Widget;
gfx_GenericWidget logTimestamp4Widget;
gfx_GenericWidget logTimestamp5Widget;
gfx_GenericWidget logTimestamp6Widget;
gfx_GenericWidget logTimestamp7Widget;
gfx_GenericWidget logTimestamp8Widget;

gfx_GenericWidget countdownWidget;


// After the boot sequence verifications are done, after 5 seconds automatically
// proceed to the dashboard. It is required to stop this automatic behavior through a tap on the screen. However, 
// due to contrictions in the gesture_engine we cannot simply pass that event to the UI. To overcome that problem
// I will put a button of the size of the screen, that button will not be visible
gfx_GenericWidget screenTouchWidget;

// Scrollbar
gfx_GenericWidget scrollbarWidget;

// Footer Progress
gfx_GenericWidget progressTextWidget;
gfx_GenericWidget progressPctWidget;
gfx_GenericWidget progressBarWidget;

// ==========================================
// 2. The Persistent Memory (Widget Data)
// ==========================================
// Header
gfx_Rectangle tcLogoBgData;
gfx_Label     mainTitleData;
gfx_Label     subTitleData;

gfx_Label 	  countdownData;

// Terminal Logs (Using TYPO_MONO)
gfx_Label     log1Data;
gfx_Label     log2Data;	// EEPROM
gfx_Label     log3Data;	// touch 
gfx_Label     log4Data; // batt
gfx_Label     log5Data;	// inst
gfx_Label     log7Data;
gfx_Label     log8Data;

gfx_Label     logTimestamp1Data;
gfx_Label     logTimestamp2Data; // EPROM
gfx_Label     logTimestamp3Data; // TOUCH
gfx_Label     logTimestamp4Data; // BATT
gfx_Label     logTimestamp5Data; // INST
gfx_Label     logTimestamp7Data; // Date
gfx_Label     logTimestamp8Data; // TIme

// Scrollbar & Footer
gfx_Slider    scrollbarData;
gfx_Label     progressTextData;
gfx_Label     progressPctData;
gfx_Slider    progressBarData;

gfx_TouchArea screenTouchAreaData;

static char pctData[10];

#define EVE_FREE_RAMG_START		768000 + 200

static char boardInfoSubtitle[60] = {0};
static char eepromStr[60] = "Checking EEPROM Memory Status..... [ OK ] (4KB/32KB Used)";
static char batteryStr[60] = "Checking Battery Power Status..... [ OK ] (Bat OK)";
static char touchStr[60] = "Loading Touch Calibration......... [ OK ] (Cal OK)";
static char instStr[60] = "Initializing Instrumentation...... [ OK ] (SPI OK)";
static char dateStr[60] = "Reading RTC Date.................. [ INFO ] (20/04/2026)";
static char timeStr[60] = "Reading RTC Time.................. [ INFO ] (16:16:58)";

static char eepromTimestamp[8] = "[0.000]";
static char batteryTimestamp[8] = "[0.000]";
static char touchTimestamp[8] = "[0.000]";
static char instTimestamp[8] = "[0.000]";
static char dateTimestamp[8] = "[0.000]";
static char timeTimestamp[8] = "[0.000]";

static char countdownStr[] = "Touch screen to continue"; 
bool g_bBootAnimationFinished = false;

// ==========================================
// Callbacks
// ==========================================
static void onEEPROMStateEvent(EventParam_t arg) {
	if(arg.bool_) {
		strcpy(eepromStr, "Checking EEPROM Memory Status..... [ OK ] (EEPROM OK)");
		log2Data.style = STYLE_SUCCESS;
	} else {
		strcpy(eepromStr, "Checking EEPROM Memory Status..... [ FAIL ] (ERROr)");
		log2Data.style = STYLE_DANGER;
	}
	log2Data.isVisible = true;
	log2Data.bIsDirty = true;
	logTimestamp2Data.isVisible = true;
	logTimestamp2Data.bIsDirty = true;
}

static void onTouchStateEvent(EventParam_t arg) {
	if(arg.bool_) {
		strcpy(touchStr, "Loading Touch Calibration......... [ OK ] (CALIB OK)");
		log3Data.style = STYLE_SUCCESS;
	} else {
		strcpy(touchStr, "Loading Touch Calibration......... [ FAIL ] (NO DATA)");
		log3Data.style = STYLE_DANGER;
	}
	log3Data.isVisible = true;
	log3Data.bIsDirty = true;
	logTimestamp3Data.isVisible = true;
	logTimestamp3Data.bIsDirty = true;
}

static void onBatteryStateEvent(EventParam_t arg) {
	if(arg.bool_) {
		strcpy(batteryStr, "Checking Battery Power Status..... [ OK ] (BATT ON)");
		log4Data.style = STYLE_SUCCESS;
	} else {
		strcpy(batteryStr, "Checking Battery Power Status..... [ FAIL ] (ERROR)");
		log4Data.style = STYLE_DANGER;
	}
	log4Data.isVisible = true;
	log4Data.bIsDirty = true;
	logTimestamp4Data.isVisible = true;
	logTimestamp4Data.bIsDirty = true;
}

static void onInstStateEvent(EventParam_t arg) {
	if(arg.bool_) {
		strcpy(instStr, "Initializing Instrumentation...... [ OK ] (FOUND)");
		log5Data.style = STYLE_SUCCESS;
	} else {
		strcpy(instStr, "Initializing Instrumentation...... [ FAIL ] (NOT FOUND)");
		log5Data.style = STYLE_DANGER;
	}
	log5Data.isVisible = true;
	log5Data.bIsDirty = true;
	logTimestamp5Data.isVisible = true;
	logTimestamp5Data.bIsDirty = true;
}

static void onRTCDateStateEvent(EventParam_t arg) {
	if(arg.str == NULL) return;

	snprintf(dateStr, sizeof(dateStr), "Reading RTC Date.................. [ INFO ] (%s)", arg.str);

	log7Data.isVisible = true;
	log7Data.bIsDirty = true;
	logTimestamp7Data.isVisible = true;
	logTimestamp7Data.bIsDirty = true;
}

static void onRTCTimeStateEvent(EventParam_t arg) {
	if(arg.str == NULL) return;

	snprintf(timeStr, sizeof(timeStr), "Reading RTC Time.................. [ INFO ] (%s)", arg.str);

	log8Data.isVisible = true;
	log8Data.bIsDirty = true;
	logTimestamp8Data.isVisible = true;
	logTimestamp8Data.bIsDirty = true;
}

static void onRTCTimeChanged(EventParam_t arg) {
	if(arg.str == NULL || !g_bBootAnimationFinished) return;

	snprintf(timeStr, sizeof(timeStr), "Reading RTC Time.................. [ INFO ] (%s)", arg.str);

	log8Data.isVisible = true;
	log8Data.bIsDirty = true;
}

static void onProgressBarChange(EventParam_t arg) {
	if(arg.ui32 <= 100) {
		progressBarData.currentValue = arg.ui32;
		progressBarData.bIsDirty = true;
		snprintf(pctData, sizeof(pctData), "%d%%", arg.ui32);
		progressPctData.bIsDirty = true;

		if(arg.ui32 == 100) g_bBootAnimationFinished = true;
	
	}
}

static void onEEPROMTimestampEvent(EventParam_t arg) {
	if(arg.str == NULL) return;
	strcpy(eepromTimestamp, arg.str);
	logTimestamp2Data.bIsDirty = true;
}

static void onTouchTimestampEvent(EventParam_t arg) {
	if(arg.str == NULL) return;
	strcpy(touchTimestamp, arg.str);
	logTimestamp3Data.bIsDirty = true;

}

static void onBattTimestampEvent(EventParam_t arg) {
	if(arg.str == NULL) return;
	strcpy(batteryTimestamp, arg.str);
	logTimestamp4Data.bIsDirty = true;

}

static void onInstTimestampEvent(EventParam_t arg) {
	if(arg.str == NULL) return;
	strcpy(instTimestamp, arg.str);
	logTimestamp5Data.bIsDirty = true;
}

static void onDateTimestampEvent(EventParam_t arg) {
	if(arg.str == NULL) return;
	strcpy(dateTimestamp, arg.str);
	logTimestamp7Data.bIsDirty = true;
}

static void onTimeTimestampEvent(EventParam_t arg) {
	if(arg.str == NULL) return;
	strcpy(timeTimestamp, arg.str);
	logTimestamp8Data.bIsDirty = true;
}

static void onUpdateCountdown(EventParam_t arg) {
	sprintf(countdownStr, "%u", arg.ui32);
	countdownData.isVisible = true;
	countdownData.bIsDirty = true;
}

static void onToggleContinueEvent(EventParam_t arg) {
	if(countdownData.isVisible) 
		countdownData.isVisible = false;
	else
		countdownData.isVisible = true;

	countdownData.bIsDirty = true;
}

static void onTouchAreaReleased(gfx_TouchArea *area) {
	if(strcmp(countdownStr, "Touch to continue") == 0) {
		Event_Post(EVT_SYS_BOOT_CONTINUE_TO_DASHBOARD, (EventParam_t){.ptr = NULL});
	} else {
		Event_Post(EVT_SYS_BOOT_STOP_AUTO_SEQ, (EventParam_t){.ptr = NULL});
		strcpy(countdownStr, "Touch to continue");
		//countdownData.typo = TYPO_MONO_BOLD;
	}
	countdownData.bIsDirty = true;
}


// ==========================================
// 3. Initialization Function
// ==========================================
void initBootForm(void)
{
    // Set absolute black background for the boot screen
    g_sBootCanvas.ui16BackgroundColor = 0x0000; 
    g_sBootCanvas.psWidgets = NULL;

    // --- HEADER: "TC" Red Square Logo ---
    // --- HEADER: Titles ---
    mainTitleData = (gfx_Label){
        .text = (char *)APP_NAME,
        .name = "mainTitle",
        .pos.x = 110,
        .pos.y = 45,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H1,           
        .style = STYLE_TEXT_MAIN,
		.isVisible = true,
    };
    mainTitleWidget.eWidgetType = WD_TYPE_LABEL;
    mainTitleWidget.pvWidget = (void *)&mainTitleData;

	sprintf(boardInfoSubtitle, "%s | %s", BOARD_VER, FIRMWARE_VER);
    subTitleData = (gfx_Label){
        .text = boardInfoSubtitle,
        .name = "subTitle",
        .pos.x = 110,
        .pos.y = 70,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_CAPTION,           
        .style = STYLE_TEXT_MUTED, 
		.isVisible = true,
    };
    subTitleWidget.eWidgetType = WD_TYPE_LABEL;
    subTitleWidget.pvWidget = (void *)&subTitleData;


    // --- TERMINAL LOGS (y-spacing: ~35px) ---
    uint16_t logStartX = 30;
    // Offset para el inicio del mensaje (Ajusta este valor si tu fuente es más ancha o estrecha)
    // "[0.000] " son 8 caracteres.
    uint16_t logTextX = logStartX + 95; 
    uint16_t logStartY = 130;
    uint16_t spacing = 35;

    // --- Línea 1 ---
    logTimestamp1Data = (gfx_Label){ .text = "[0.000]", .name = "t1", .pos.x = logStartX, .pos.y = logStartY + (spacing * 0), .alignment = ALIGN_LEFT, .typo = TYPO_MONO, .style = STYLE_TEXT_MUTED, .isVisible = true };
    logTimestamp1Widget.eWidgetType = WD_TYPE_LABEL; logTimestamp1Widget.pvWidget = (void *)&logTimestamp1Data;
    
    log1Data = (gfx_Label){ .text = "TELECONTROL HMI OS V2.5.0 - INITIALIZING...", .name = "l1", .pos.x = logTextX, .pos.y = logStartY + (spacing * 0), .alignment = ALIGN_LEFT, .typo = TYPO_MONO_BOLD, .style = STYLE_TEXT_MAIN, .isVisible = true};
    log1Widget.eWidgetType = WD_TYPE_LABEL; log1Widget.pvWidget = (void *)&log1Data;

    // --- Línea 2 ---
    logTimestamp2Data = (gfx_Label){ .text = eepromTimestamp, .name = "t2", .pos.x = logStartX, .pos.y = logStartY + (spacing * 1), .alignment = ALIGN_LEFT, .typo = TYPO_MONO, .style = STYLE_TEXT_MUTED, .isVisible = false  };
    logTimestamp2Widget.eWidgetType = WD_TYPE_LABEL; logTimestamp2Widget.pvWidget = (void *)&logTimestamp2Data;
    
    log2Data = (gfx_Label){ .text = eepromStr, .name = "l2", .pos.x = logTextX, .pos.y = logStartY + (spacing * 1), .alignment = ALIGN_LEFT, .typo = TYPO_MONO, .style = STYLE_SUCCESS, .isVisible = false  };
    log2Widget.eWidgetType = WD_TYPE_LABEL; log2Widget.pvWidget = (void *)&log2Data;

    // --- Línea 3 ---
    logTimestamp3Data = (gfx_Label){ .text = touchTimestamp, .name = "t3", .pos.x = logStartX, .pos.y = logStartY + (spacing * 2), .alignment = ALIGN_LEFT, .typo = TYPO_MONO, .style = STYLE_TEXT_MUTED, .isVisible = false  };
    logTimestamp3Widget.eWidgetType = WD_TYPE_LABEL; logTimestamp3Widget.pvWidget = (void *)&logTimestamp3Data;
    
    log3Data = (gfx_Label){ .text = touchStr, .name = "l3", .pos.x = logTextX, .pos.y = logStartY + (spacing * 2), .alignment = ALIGN_LEFT, .typo = TYPO_MONO, .style = STYLE_SUCCESS, .isVisible = false  };
    log3Widget.eWidgetType = WD_TYPE_LABEL; log3Widget.pvWidget = (void *)&log3Data;

    // --- Línea 4 ---
    logTimestamp4Data = (gfx_Label){ .text = batteryTimestamp, .name = "t4", .pos.x = logStartX, .pos.y = logStartY + (spacing * 3), .alignment = ALIGN_LEFT, .typo = TYPO_MONO, .style = STYLE_TEXT_MUTED, .isVisible = false  };
    logTimestamp4Widget.eWidgetType = WD_TYPE_LABEL; logTimestamp4Widget.pvWidget = (void *)&logTimestamp4Data;
    
    log4Data = (gfx_Label){ .text = batteryStr, .name = "l4", .pos.x = logTextX, .pos.y = logStartY + (spacing * 3), .alignment = ALIGN_LEFT, .typo = TYPO_MONO, .style = STYLE_SUCCESS, .isVisible = false  };
    // (Nota de alineación en tu código original: cambiaste a spacing * 4 aquí, ajusté según tu código original, pero podrías querer que sea spacing * 3)
    log4Widget.eWidgetType = WD_TYPE_LABEL; log4Widget.pvWidget = (void *)&log4Data;

    // --- Línea 5 ---
    logTimestamp5Data = (gfx_Label){ .text = instTimestamp, .name = "t5", .pos.x = logStartX, .pos.y = logStartY + (spacing * 4), .alignment = ALIGN_LEFT, .typo = TYPO_MONO, .style = STYLE_TEXT_MUTED, .isVisible = false  };
    logTimestamp5Widget.eWidgetType = WD_TYPE_LABEL; logTimestamp5Widget.pvWidget = (void *)&logTimestamp5Data;
    
    log5Data = (gfx_Label){ .text = instStr, .name = "l5", .pos.x = logTextX, .pos.y = logStartY + (spacing * 4), .alignment = ALIGN_LEFT, .typo = TYPO_MONO, .style = STYLE_SUCCESS, .isVisible = false  };
    log5Widget.eWidgetType = WD_TYPE_LABEL; log5Widget.pvWidget = (void *)&log5Data;

    // --- Línea 7 ---
    logTimestamp7Data = (gfx_Label){ .text = dateTimestamp, .name = "t7", .pos.x = logStartX, .pos.y = logStartY + (spacing * 5), .alignment = ALIGN_LEFT, .typo = TYPO_MONO, .style = STYLE_TEXT_MUTED, .isVisible = false  };
    logTimestamp7Widget.eWidgetType = WD_TYPE_LABEL; logTimestamp7Widget.pvWidget = (void *)&logTimestamp7Data;
    
    log7Data = (gfx_Label){ .text = dateStr, .name = "l7", .pos.x = logTextX, .pos.y = logStartY + (spacing * 5), .alignment = ALIGN_LEFT, .typo = TYPO_MONO, .style = STYLE_SUCCESS, .isVisible = false  };
    log7Widget.eWidgetType = WD_TYPE_LABEL; log7Widget.pvWidget = (void *)&log7Data;

    // --- Línea 8 ---
    logTimestamp8Data = (gfx_Label){ .text = timeTimestamp, .name = "t8", .pos.x = logStartX, .pos.y = logStartY + (spacing * 6), .alignment = ALIGN_LEFT, .typo = TYPO_MONO, .style = STYLE_TEXT_MUTED, .isVisible = false  };
    logTimestamp8Widget.eWidgetType = WD_TYPE_LABEL; logTimestamp8Widget.pvWidget = (void *)&logTimestamp8Data;
    
    log8Data = (gfx_Label){ .text = timeStr, .name = "l8", .pos.x = logTextX, .pos.y = logStartY + (spacing * 6), .alignment = ALIGN_LEFT, .typo = TYPO_MONO, .style = STYLE_SUCCESS, .isVisible = false  };
    log8Widget.eWidgetType = WD_TYPE_LABEL; log8Widget.pvWidget = (void *)&log8Data;


	// Countdown data
	countdownData = (gfx_Label) {
		.name = "countdownLabel",
		.alignment = ALIGN_CENTER,
		.pos.y = logStartY + (spacing * 7),
		.pos.x = LCD_WIDTH / 2,
		.style = STYLE_DANGER,
		.typo = TYPO_H1,
		.text =  countdownStr,
		.isVisible = false,
	};
	countdownWidget.eWidgetType = WD_TYPE_LABEL;
	countdownWidget.pvWidget = (void *)&countdownData;


    // --- FOOTER PROGRESS SECTION ---
    progressTextData = (gfx_Label){
        .text = ">_ SYSTEM BOOT PROGRESS",
        .name = "progText",
        .pos.x = 30,
        .pos.y = LCD_HEIGHT - 60,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_CAPTION,           
        .style = STYLE_TEXT_MUTED,
		.isVisible = true,
    };
    progressTextWidget.eWidgetType = WD_TYPE_LABEL;
    progressTextWidget.pvWidget = (void *)&progressTextData;

    progressPctData = (gfx_Label){
        .text = pctData,
        .name = "progPct",
        .pos.x = LCD_WIDTH - 40,
        .pos.y = LCD_HEIGHT - 60,
        .alignment = ALIGN_RIGHT,
        .typo = TYPO_CAPTION,           
        .style = STYLE_SECONDARY,
		.isVisible = true,
    };
	strcpy(pctData, "%10");
    progressPctWidget.eWidgetType = WD_TYPE_LABEL;
    progressPctWidget.pvWidget = (void *)&progressPctData;

    progressBarData = (gfx_Slider){
        .name = "progBar",
        .knobRadius = 0,
        .maxValue = 100,
        .minValue = 0,
        .size.width = LCD_WIDTH - 60,
        .size.height = 8,
        .pos.x = 30,
        .pos.y = LCD_HEIGHT - 35,
        .trackHeight = 8,
        .style = STYLE_PRIMARY,  // Red progress bar
        .onValueChanged = NULL, 
        .bIsVertical = false,
        .bShowKnob = false,     // Flat progress bar look
        .currentValue = 10,
    };
    progressBarWidget.eWidgetType = WD_TYPE_SLIDER;
    progressBarWidget.pvWidget = (void *)&progressBarData;

	screenTouchAreaData = (gfx_TouchArea){
		.pos.x = 0,
		.pos.y = 0,
		.size.width = LCD_WIDTH,
		.size.height = LCD_HEIGHT,
		.onAreaTouchRelease = onTouchAreaReleased,
	};
	gfx_initRegTouch((void *)&screenTouchAreaData, WD_TYPE_TOUCH_AREA);

	screenTouchWidget.eWidgetType = WD_TYPE_TOUCH_AREA;
	screenTouchWidget.pvWidget = (void *)&screenTouchAreaData;

    // ==========================================
    // 4. Insertion into Canvas
    // ==========================================
    // Background / Structure
	//useLogoWidget(&g_sBootCanvas);
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &progressBarWidget);
    
    // Texts and Labels
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &mainTitleWidget);
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &subTitleWidget);
	useBlkLogoWidget(&g_sBootCanvas);
	canvasInsertAtTop(&g_sBootCanvas.psWidgets, &countdownWidget);
    
    // Terminal Texts
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &log1Widget);
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &log2Widget);
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &log3Widget);
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &log4Widget);
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &log5Widget);
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &log7Widget);
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &log8Widget);

    // Terminal Timestamps (Nuevas inserciones)
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &logTimestamp1Widget);
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &logTimestamp2Widget);
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &logTimestamp3Widget);
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &logTimestamp4Widget);
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &logTimestamp5Widget);
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &logTimestamp7Widget);
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &logTimestamp8Widget);
    
    // Footer Texts
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &progressTextWidget);
    canvasInsertAtTop(&g_sBootCanvas.psWidgets, &progressPctWidget);

	canvasInsertAtTop(&g_sBootCanvas.psWidgets, &screenTouchWidget);

	Event_Subscribe(EVT_SYS_BOOT_EEPROM_STATE, (EventHandler_fn)onEEPROMStateEvent);
	Event_Subscribe(EVT_SYS_BOOT_BATT_STATE, (EventHandler_fn)onBatteryStateEvent);
	Event_Subscribe(EVT_SYS_BOOT_INST_STATE, (EventHandler_fn)onInstStateEvent);
	Event_Subscribe(EVT_SYS_BOOT_TOUCH_STATE, (EventHandler_fn)onTouchStateEvent);
	Event_Subscribe(EVT_SYS_BOOT_RTC_DATE, (EventHandler_fn)onRTCDateStateEvent);
	Event_Subscribe(EVT_SYS_BOOT_RTC_TIME, (EventHandler_fn)onRTCTimeStateEvent);
	Event_Subscribe(EVT_SYS_BOOT_PROGRESS_VALUE_CHANGE, (EventHandler_fn)onProgressBarChange);
	Event_Subscribe(EVT_SYS_TIME_CHANGED, (EventHandler_fn)onRTCTimeChanged);

	Event_Subscribe(EVT_SYS_BOOT_EEPROM_TIMESTAMP, (EventHandler_fn)onEEPROMTimestampEvent);
	Event_Subscribe(EVT_SYS_BOOT_TOUCH_TIMESTAMP, (EventHandler_fn)onTouchTimestampEvent);
	Event_Subscribe(EVT_SYS_BOOT_BATTERY_TIMESTAMP, (EventHandler_fn)onBattTimestampEvent);
	Event_Subscribe(EVT_SYS_BOOT_INST_TIMESTAMP, (EventHandler_fn)onInstTimestampEvent);
	Event_Subscribe(EVT_SYS_BOOT_DATE_TIMESTAMP, (EventHandler_fn)onDateTimestampEvent);
	Event_Subscribe(EVT_SYS_BOOT_TIME_TIMESTAMP, (EventHandler_fn)onTimeTimestampEvent);

	Event_Subscribe(EVT_SYS_BOOT_COUNTDOWN, (EventHandler_fn)onUpdateCountdown);
	Event_Subscribe(EVT_UI_BOOT_TOGGLE_CONTINUE_LABEL, (EventHandler_fn)onToggleContinueEvent);
    // Register Form
    g_i16BootFormID = FormManager_AddForm(&g_sBootCanvas);
}


