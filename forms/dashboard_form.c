#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gui_core.h"
#include "FT8xx_params.h"
#include "form_common.h"
#include "EVE_colors.h"
#include "gui_canvas.h"
#include "forms_manager.h"

#include "helpers.h"
#include "gui_theme.h"
#include "event_engine.h"
#include "common_widgets.h"
#include "icon_map.h"

#include "dashboard_form.h"

static gfx_Canvas g_sSystemCanvas;

int16_t g_i16DashboardFormID = 0;

bool g_bIsSecondaryViewVisible = false;
bool g_bIsUSBConnected = false;
bool g_bIsInstSyncd = false;

// ==========================================
// 1. The Generic Nodes (Widgets)
// ==========================================
// Header
gfx_GenericWidget headerPanelWidget;
gfx_GenericWidget tcLogoImgWidget;
gfx_GenericWidget titleWidget;

// Panel: Temperaturas
gfx_GenericWidget tempPanelBgWidget;
gfx_GenericWidget currentPanelWidget;
gfx_GenericWidget t1PanelWidget;
gfx_GenericWidget voPanelWidget;
gfx_GenericWidget tempPanelTitleWidget;
gfx_GenericWidget currentLabelWidget;
gfx_GenericWidget t1LabelWidget;
gfx_GenericWidget voLabelWidget;
gfx_GenericWidget currentValueWidget;
gfx_GenericWidget t1ValueWidget;
gfx_GenericWidget voValueWidget;

// Panel: Status
gfx_GenericWidget statusPanelBgWidget;
gfx_GenericWidget sysStatusPanelWidget;
gfx_GenericWidget instStatusPanelWidget;
gfx_GenericWidget usbStatusPanelWidget;
gfx_GenericWidget statusPanelTitleWidget;

gfx_GenericWidget sysStatusLabelWidget;
gfx_GenericWidget voutLabelWidget;
gfx_GenericWidget usbStatusLabelWidget;
gfx_GenericWidget sysStatusValueWidget;
gfx_GenericWidget instStatusValueWidget;
gfx_GenericWidget usbStatusValueWidget;
gfx_GenericWidget leftArrowButtonWidget;
gfx_GenericWidget rightArrowButtonWidget;

// Icons
gfx_GenericWidget currentIconWidget;
gfx_GenericWidget temperatureIconWidget;
gfx_GenericWidget tensionIconWidget;
gfx_GenericWidget sysStatusIconWidget;
gfx_GenericWidget instStatusIconWidget;
gfx_GenericWidget usbIconWidget;

// ==========================================
// 2. The Persistent Memory (Data)
// ==========================================
// Header
gfx_Label titleData;

// Panel: Temperaturas
gfx_Rectangle sensorPanelBgData;
gfx_Rectangle currentPanelData;
gfx_Rectangle t1PanelData;
gfx_Rectangle voPanelData;
gfx_Label     sensorPanelTitleData;

gfx_Label     indicator1LbData;
gfx_Label     indicator2LbData;
gfx_Label     indicator3LbData;
gfx_Label     indicator1ValueData;
gfx_Label     indicator2ValueData;
gfx_Label     indicator3ValueData;

gfx_Button leftArrowButtonData;
gfx_Button rightArrowButtonData;

// Panel: Status
gfx_Rectangle statusPanelBgData;
gfx_Rectangle sysStatusPanelData;
gfx_Rectangle instStatusPanelData;
gfx_Rectangle usbPanelData;
gfx_Label     statusPanelTitleData;

gfx_Label     indicator4LbData;
gfx_Label     indicator5LbData;
gfx_Label     indicator6LbData;
gfx_Label     indicator4ValueData;
gfx_Label     indicator5ValueData;
gfx_Label     indicator6ValueData;

// Icon data
gfx_Label indicator1IconData;
gfx_Label indicator2IconData;
gfx_Label indicator3IconData;
gfx_Label indicator4IconData;
gfx_Label indicator5IconData;
gfx_Label indicator6IconData;

// ==========================================
// 3. Static Text Buffers
// ==========================================
static char icons[9][2] = {ICON_TEMPERATURE, ICON_TEMPERATURE, ICON_TEMPERATURE, ICON_CHECK_CIRCLE, ICON_UNSYNC, ICON_USB_DISCONN, ICON_LIGHTNING, ICON_TEMPERATURE, ICON_WAVE};
static char indicatorsName[9][35] = {"Temp. Sonda Principal", "Temp. Tx Primario", "Temp. Tx Secundario", "Estado del sistema", "Instrumentacion", "USB", "Corriente del Secundario", "Temp. Sonda Secundaria", "Voltaje del Secundario"};

static char t1Buffer[16] = "--.- [C]";
static char t2Buffer[16] = "--.- [C]";
static char t3Buffer[16] = "--.- [C]";
static char currentBuffer[16] = "--.- [A]";
static char t4Buffer[16] = "--.-- [C]";
static char voltageBuffer[16] = "--.- [V]";
static char sysStatusBuffer[20] = "Listo";
static char instStatusBuffer[20] = "UNSYNC";
static char usbBuffer[20] = "DESCONECTADO";

// ==========================================
// Callbacks 
// ==========================================

// ------------------- Column 1 --------------------------------- //
static void onTempProbeMainValueChanged(EventParam_t arg) {
	if(GetExecTimeMs() % 300 < 80) {
		snprintf(t1Buffer, 16, "%.1f [C]", arg.f32);
		if(!g_bIsSecondaryViewVisible) {
			indicator1ValueData.text = t1Buffer;
			indicator1ValueData.bIsDirty = true;
			indicator1IconData.text = ICON_TEMPERATURE;
			indicator1IconData.bIsDirty = true;
		}
	}
}

static void onTempTxPrimaryValueChanged(EventParam_t arg) {
	if(GetExecTimeMs() % 300 < 80) {
		snprintf(t2Buffer, 16, "%.1f [C]", arg.f32);
		if(!g_bIsSecondaryViewVisible) {
			indicator2ValueData.text = t2Buffer;
			indicator2ValueData.bIsDirty = true;
			indicator2IconData.text = ICON_TEMPERATURE;
			indicator2IconData.bIsDirty = true;
		}
	}
}

static void onTempTxSecondaryValueChanged(EventParam_t arg) {
	if(GetExecTimeMs() % 300 < 80) {
		snprintf(t3Buffer, 16, "%.1f [C]", arg.f32);
		if(!g_bIsSecondaryViewVisible) {
			indicator3ValueData.text = t3Buffer;
			indicator3ValueData.bIsDirty = true;
			indicator3IconData.text = ICON_TEMPERATURE;
			indicator3IconData.bIsDirty = true;
		}
	}
}

// ------------------- Column 2 --------------------------------- //
static void onUSBConnected(EventParam_t arg) {
	strcpy(usbBuffer, "CONECTADO");
	g_bIsUSBConnected = true;
	if(!g_bIsSecondaryViewVisible) {
		indicator6ValueData.style = STYLE_SUCCESS;
		indicator6ValueData.text = usbBuffer;
		indicator6ValueData.bIsDirty = true;
		indicator6IconData.text = ICON_USB_CONN;
		indicator6IconData.style = STYLE_SUCCESS;
		indicator6IconData.bIsDirty = true;
		usbPanelData.bIsDirty = true;
	} else {
		indicator3ValueData.style = STYLE_SUCCESS;
		indicator3ValueData.bIsDirty = true;
		indicator3ValueData.text = usbBuffer;
		indicator3IconData.text = ICON_USB_CONN;
		indicator3IconData.style = STYLE_SUCCESS;
		indicator3IconData.bIsDirty = true;
		voPanelData.bIsDirty = true;
	}
}

static void onUSBDisconnected(EventParam_t arg) {
	strcpy(usbBuffer, "DESCONECTADO");
	g_bIsUSBConnected = false;
	if(!g_bIsSecondaryViewVisible) {
		indicator6ValueData.style = STYLE_DANGER;
		indicator6ValueData.text = usbBuffer;
		indicator6ValueData.bIsDirty = true;
		indicator6IconData.text = ICON_USB_DISCONN;
		indicator6IconData.style = STYLE_DANGER;
		indicator6IconData.bIsDirty = true;
		usbPanelData.bIsDirty = true;
	} else {
		indicator3ValueData.style = STYLE_DANGER;
		indicator3ValueData.text = usbBuffer;
		indicator3ValueData.bIsDirty = true;
		indicator3IconData.text = ICON_USB_DISCONN;
		indicator3IconData.style = STYLE_DANGER;
		indicator3IconData.bIsDirty = true;
		voPanelData.bIsDirty = true;
	}
}

static void onUSBUnknownDevice(EventParam_t arg) {
	strcpy(usbBuffer, "UNKNOWN");
	g_bIsUSBConnected = false;
	if(!g_bIsSecondaryViewVisible) {
		indicator6ValueData.style = STYLE_DANGER;
		indicator6ValueData.text = usbBuffer;
		indicator6ValueData.bIsDirty = true;
		indicator6IconData.text = ICON_USB_DISCONN;
		indicator6IconData.style = STYLE_DANGER;
		indicator6IconData.bIsDirty = true;
	} else {
		indicator3ValueData.style = STYLE_DANGER;
		indicator3ValueData.text = usbBuffer;
		indicator3ValueData.bIsDirty = true;
		indicator3IconData.text = ICON_USB_DISCONN;
		indicator3IconData.bIsDirty = true;
	}
}

static void onUSBPowerFault(EventParam_t arg) {
	strcpy(usbBuffer, "UNKNOWN");
	g_bIsUSBConnected = false;
	if(!g_bIsSecondaryViewVisible) {
		indicator6ValueData.style = STYLE_DANGER;
		indicator6ValueData.text = usbBuffer;
		indicator6ValueData.bIsDirty = true;
		indicator6IconData.text = ICON_USB_DISCONN;
		indicator6IconData.style = STYLE_DANGER;
		indicator6IconData.bIsDirty = true;
	} else {
		indicator3ValueData.style = STYLE_DANGER;
		indicator3ValueData.text = usbBuffer;
		indicator3ValueData.bIsDirty = true;
		indicator3IconData.text = ICON_USB_DISCONN;
		indicator3IconData.style = STYLE_DANGER;
		indicator3IconData.bIsDirty = true;
	}
}

static void onInstSyncEvent(EventParam_t arg) {
	strcpy(instStatusBuffer, "SYNC");
	g_bIsInstSyncd = true;
	if(!g_bIsSecondaryViewVisible) {
		indicator5ValueData.style = STYLE_SUCCESS;
		indicator5ValueData.text = instStatusBuffer;
		indicator5ValueData.bIsDirty = true;

		indicator5IconData.text = ICON_SYNC;
		indicator5IconData.style = STYLE_SUCCESS;
		indicator5IconData.bIsDirty = true;
		instStatusPanelData.bIsDirty = true;
	} else {
		indicator2ValueData.style = STYLE_SUCCESS;
		indicator2ValueData.text = instStatusBuffer;
		indicator2ValueData.bIsDirty = true;

		indicator2IconData.text = ICON_SYNC;
		indicator2IconData.style = STYLE_SUCCESS;
		indicator2IconData.bIsDirty = true;
		t1PanelData.bIsDirty = true;
	}
	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onInstUnsyncEvent(EventParam_t arg) {
	strcpy(instStatusBuffer, "UNSYNC");
	g_bIsInstSyncd = false;
	if(!g_bIsSecondaryViewVisible) {
		indicator5ValueData.style = STYLE_DANGER;
		indicator5ValueData.text = instStatusBuffer;
		indicator5ValueData.bIsDirty = true;

		indicator5IconData.text = ICON_UNSYNC;
		indicator5IconData.style = STYLE_DANGER;
		indicator5IconData.bIsDirty = true;
	} else {
		indicator2ValueData.style = STYLE_DANGER;
		indicator2ValueData.text = instStatusBuffer;
		indicator2ValueData.bIsDirty = true;

		indicator2IconData.text = ICON_UNSYNC;
		indicator2IconData.style = STYLE_DANGER;
		indicator2IconData.bIsDirty = true;
	}

	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

// ------------------- Column 3 --------------------------------- //
static void onCurrentSecondaryValueChanged(EventParam_t arg) {
	if(GetExecTimeMs() % 500 < 80) {
		sprintf(currentBuffer, "%.1f [A]", arg.f32);
		if(g_bIsSecondaryViewVisible) 
			indicator4ValueData.bIsDirty = true;
	}
}

static void onTempProbeSecondaryValueChanged(EventParam_t arg) {
	if(GetExecTimeMs() % 500 < 80) {
		snprintf(t4Buffer, 16, "%.1f [C]", arg.f32);
		if(g_bIsSecondaryViewVisible) 
			indicator5ValueData.bIsDirty = true;
	}
}

static void onVoltageSecondaryValueChanged(EventParam_t arg) {
	if(GetExecTimeMs() % 500 < 80) {
		snprintf(voltageBuffer, 16, "%.1f [V]", arg.f32);
		if(g_bIsSecondaryViewVisible) 
			indicator6ValueData.bIsDirty = true;
	}
}


static void onLeftArrowReleaseEvent(gfx_Button *btn) {
	onGenericBtnRelease(btn);

	g_bIsSecondaryViewVisible = true;

	sensorPanelBgData.pos.x = rightArrowButtonData.size.width + 15;
	statusPanelBgData.pos.x = sensorPanelBgData.pos.x + sensorPanelBgData.dim.width + 15;

    currentPanelData.pos.x = sensorPanelBgData.pos.x + 10;
    t1PanelData.pos.x = sensorPanelBgData.pos.x + 10;
    voPanelData.pos.x = sensorPanelBgData.pos.x + 10;
    sysStatusPanelData.pos.x = statusPanelBgData.pos.x + 10;
    usbPanelData.pos.x = statusPanelBgData.pos.x + 10;
    instStatusPanelData.pos.x = statusPanelBgData.pos.x + 10;

	indicator1LbData.pos.x = sensorPanelBgData.pos.x + 20;
	indicator2LbData.pos.x = sensorPanelBgData.pos.x + 20;
	indicator3LbData.pos.x = sensorPanelBgData.pos.x + 20;

	indicator1ValueData.pos.x = sensorPanelBgData.pos.x + 70;
	indicator2ValueData.pos.x = sensorPanelBgData.pos.x + 70;
	indicator3ValueData.pos.x = sensorPanelBgData.pos.x + 70;

	indicator1IconData.pos.x = sensorPanelBgData.pos.x + 15;
	indicator2IconData.pos.x = sensorPanelBgData.pos.x + 15;
	indicator3IconData.pos.x = sensorPanelBgData.pos.x + 15;

	indicator4LbData.pos.x = sysStatusPanelData.pos.x + 20;
	indicator5LbData.pos.x = sysStatusPanelData.pos.x + 20;
	indicator6LbData.pos.x = sysStatusPanelData.pos.x + 20;

	indicator4IconData.pos.x = sysStatusPanelData.pos.x + 10;
	indicator5IconData.pos.x = sysStatusPanelData.pos.x + 10;
	indicator6IconData.pos.x = sysStatusPanelData.pos.x + 10;

	indicator4ValueData.pos.x = sysStatusPanelData.pos.x + 70;
	indicator5ValueData.pos.x = instStatusPanelData.pos.x + 70;
	indicator6ValueData.pos.x = usbPanelData.pos.x + 70;
	
	indicator1LbData.text = indicatorsName[3];
	indicator2LbData.text = indicatorsName[4];
	indicator3LbData.text = indicatorsName[5];
	indicator4LbData.text = indicatorsName[6];
	indicator5LbData.text = indicatorsName[7];
	indicator6LbData.text = indicatorsName[8];

	indicator1ValueData.text = sysStatusBuffer;
	indicator2ValueData.text = instStatusBuffer;
	indicator3ValueData.text = usbBuffer;
	indicator4ValueData.text = currentBuffer;
	indicator5ValueData.text = t4Buffer;
	indicator6ValueData.text = voltageBuffer;

	indicator1IconData.text = ICON_CHECK_CIRCLE; // status
	indicator2IconData.text = g_bIsInstSyncd ? ICON_SYNC : ICON_UNSYNC; // inst
	indicator3IconData.text = g_bIsUSBConnected ? ICON_USB_CONN : ICON_USB_DISCONN; // usb
	indicator4IconData.text = ICON_LIGHTNING; // current
	indicator5IconData.text = ICON_TEMPERATURE; // t4
	indicator6IconData.text = ICON_WAVE; // voltage

	indicator3ValueData.typo = TYPO_H3; // usb
	indicator6ValueData.typo = TYPO_H1; // voltage
	indicator3ValueData.pos.y = indicator3LbData.pos.y + 20;
	indicator6ValueData.pos.y = indicator6LbData.pos.y + 10;
	indicator3ValueData.style = g_bIsUSBConnected ? STYLE_SUCCESS : STYLE_DANGER;
	indicator2ValueData.style = g_bIsInstSyncd ? STYLE_SUCCESS : STYLE_DANGER;
	indicator2IconData.style = indicator2ValueData.style;
	indicator3IconData.style =  g_bIsUSBConnected ? STYLE_SUCCESS : STYLE_DANGER;

	indicator4ValueData.style = STYLE_SUCCESS;
	indicator5ValueData.style = STYLE_PRIMARY;
	indicator6ValueData.style = STYLE_SECONDARY;
	indicator4IconData.style = indicator4ValueData.style;
	indicator5IconData.style = indicator5ValueData.style;
	indicator6IconData.style = indicator6ValueData.style;

	indicator1ValueData.style = STYLE_SUCCESS;
	indicator1IconData.style = indicator1ValueData.style;

	rightArrowButtonData.bIsVisible = true;
	leftArrowButtonData.bIsVisible = false;

	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onRightArrowReleaseEvent(gfx_Button *btn) {
	onGenericBtnRelease(btn);

	g_bIsSecondaryViewVisible = false;

	sensorPanelBgData.pos.x = 10;
	statusPanelBgData.pos.x = sensorPanelBgData.pos.x + sensorPanelBgData.dim.width + 10;

    currentPanelData.pos.x = sensorPanelBgData.pos.x + 10,
    t1PanelData.pos.x = sensorPanelBgData.pos.x + 10,
    voPanelData.pos.x = sensorPanelBgData.pos.x + 10,
    sysStatusPanelData.pos.x = statusPanelBgData.pos.x + 10,
    usbPanelData.pos.x = statusPanelBgData.pos.x + 10,
    instStatusPanelData.pos.x = statusPanelBgData.pos.x + 10,

	indicator1LbData.pos.x = 35;
	indicator2LbData.pos.x = 35;
	indicator3LbData.pos.x = 35;
	indicator1ValueData.pos.x = 95;
	indicator2ValueData.pos.x = 95;
	indicator3ValueData.pos.x = 95;
	indicator1IconData.pos.x = 25;
	indicator2IconData.pos.x = 25;
	indicator3IconData.pos.x = 25;

	indicator4LbData.pos.x = sysStatusPanelData.pos.x + 20;
	indicator5LbData.pos.x = sysStatusPanelData.pos.x + 20;
	indicator6LbData.pos.x = sysStatusPanelData.pos.x + 20;

	indicator4IconData.pos.x = sysStatusPanelData.pos.x + 10;
	indicator5IconData.pos.x = sysStatusPanelData.pos.x + 10;
	indicator6IconData.pos.x = sysStatusPanelData.pos.x + 10;

	indicator4ValueData.pos.x = sysStatusPanelData.pos.x + 70;
	indicator5ValueData.pos.x = instStatusPanelData.pos.x + 70;
	indicator6ValueData.pos.x = usbPanelData.pos.x + 70;

	indicator1LbData.text = indicatorsName[0];
	indicator2LbData.text = indicatorsName[1];
	indicator3LbData.text = indicatorsName[2];
	indicator4LbData.text = indicatorsName[3];
	indicator5LbData.text = indicatorsName[4];
	indicator6LbData.text = indicatorsName[5];

	indicator1ValueData.text = t1Buffer;	
	indicator2ValueData.text = t2Buffer;
	indicator3ValueData.text = t3Buffer;
	indicator4ValueData.text = sysStatusBuffer;
	indicator5ValueData.text = instStatusBuffer;
	indicator6ValueData.text = usbBuffer;
	
	indicator1IconData.text = ICON_TEMPERATURE; // current
	indicator2IconData.text = ICON_TEMPERATURE; // 
	indicator3IconData.text = ICON_TEMPERATURE;
	indicator4IconData.text = ICON_CHECK_CIRCLE;
	indicator5IconData.text = g_bIsInstSyncd ? ICON_SYNC : ICON_UNSYNC;
	indicator6IconData.text = g_bIsUSBConnected ? ICON_USB_CONN : ICON_USB_DISCONN;

	indicator3ValueData.typo = TYPO_H1;
	indicator6ValueData.typo = TYPO_H3;
	indicator6ValueData.style = g_bIsUSBConnected ? STYLE_SUCCESS : STYLE_DANGER;
	indicator6IconData.style = indicator6ValueData.style;
	indicator5ValueData.style = g_bIsInstSyncd ? STYLE_SUCCESS : STYLE_DANGER;
	indicator5IconData.style = indicator5ValueData.style;
	indicator1ValueData.style = STYLE_PRIMARY;
	indicator1IconData.style = indicator1ValueData.style;
	indicator2ValueData.style = STYLE_SUCCESS;
	indicator2IconData.style = indicator2ValueData.style;

	indicator3ValueData.style = STYLE_SECONDARY;
	indicator3IconData.style = STYLE_SECONDARY;

	indicator6ValueData.pos.y = indicator6LbData.pos.y + 20;
	indicator3ValueData.pos.y = indicator3LbData.pos.y + 10;

	rightArrowButtonData.bIsVisible = false;
	leftArrowButtonData.bIsVisible = true;

	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onInstStateEvent(EventParam_t arg) {
	if(arg.str == NULL) return;
	
	strncpy(sysStatusBuffer, arg.str, sizeof(sysStatusBuffer));
	
	if(g_bIsSecondaryViewVisible) {
		if(strcmp(arg.str, "ABORTADO") == 0) {
			indicator1ValueData.style = STYLE_DANGER;
			indicator1IconData.style = STYLE_DANGER;
			indicator1IconData.text = ICON_WARNING;
		} else { 
			indicator1ValueData.style = STYLE_SUCCESS;
			indicator1IconData.style = STYLE_SUCCESS;
			indicator1IconData.text = ICON_CHECK;
		}
		indicator1ValueData.bIsDirty = true;
		indicator1IconData.bIsDirty = true;
	}
	else {
		if(strcmp(arg.str, "ABORTADO") == 0) {
			indicator4ValueData.style = STYLE_DANGER;
			indicator4IconData.text = ICON_WARNING;
			indicator4IconData.style = STYLE_DANGER;
		} else { 
			indicator4ValueData.style = STYLE_SUCCESS;
			indicator4IconData.text = ICON_CHECK;
			indicator4IconData.style = STYLE_SUCCESS;
		}
		indicator4ValueData.bIsDirty = true;
		indicator4IconData.bIsDirty = true;
	}
	
}

// ==========================================
// 4. Initialization Function
// ==========================================
void initDashboardForm(void)
{
    g_sSystemCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background; 
    g_sSystemCanvas.psWidgets = NULL;

    titleData = (gfx_Label){
        .text = "System Status",
        .name = "sysTitle",
        .pos.x = 110,
        .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
    };
    titleWidget.eWidgetType = WD_TYPE_LABEL; titleWidget.pvWidget = (void *)&titleData;

    // --- PANEL 1: TEMPERATURAS (Izquierda) ---
	uint16_t panelWidth = 365;
	uint16_t panelHeight = 335;
	uint16_t indicatorWidth = panelWidth - 20;
	uint16_t indicatorHeight = (panelHeight - 40) / 3.0;
    sensorPanelBgData = (gfx_Rectangle){
        .name = "tempBg",
        .pos.x = 10,
        .pos.y = 75,
        .dim.width = panelWidth,
        .dim.height = panelHeight,
        .round = 5,
        .color = g_pCurrentTheme->palette.surface,
		.borderWidth = 1,
    };
    tempPanelBgWidget.eWidgetType = WD_TYPE_RECT; tempPanelBgWidget.pvWidget = (void *)&sensorPanelBgData;


    sensorPanelTitleData = (gfx_Label){ .text = "SENSORES", .name = "tTemp", .pos.x = sensorPanelBgData.pos.x + sensorPanelBgData.dim.width / 2, .pos.y = sensorPanelBgData.pos.y + 30, .alignment = (ALIGN_HCENTER), .typo = TYPO_H3, .style = STYLE_SECONDARY, .isVisible = true };
    tempPanelTitleWidget.eWidgetType = WD_TYPE_LABEL; tempPanelTitleWidget.pvWidget = (void *)&sensorPanelTitleData;

	currentPanelData = (gfx_Rectangle){
        .name = "t1Bg",
        .pos.x = sensorPanelBgData.pos.x + 10,
        .pos.y = sensorPanelBgData.pos.y + 10,
        .dim.width = indicatorWidth,
        .dim.height = indicatorHeight,
        .round = 5,
        .color = g_pCurrentTheme->palette.background,
		.borderWidth = 1,
	};
	currentPanelWidget.eWidgetType = WD_TYPE_RECT; currentPanelWidget.pvWidget = (void *)&currentPanelData;

	t1PanelData = (gfx_Rectangle){
        .name = "t2Bg",
        .pos.x = sensorPanelBgData.pos.x + 10,
        .pos.y = currentPanelData.pos.y + indicatorHeight + 10,
        .dim.width = indicatorWidth,
        .dim.height = indicatorHeight,
        .round = 5,
        .color = g_pCurrentTheme->palette.background,
		.borderWidth = 1,
	};
	t1PanelWidget.eWidgetType = WD_TYPE_RECT; t1PanelWidget.pvWidget = (void *)&t1PanelData;

	voPanelData = (gfx_Rectangle){
        .name = "t3Bg",
        .pos.x = sensorPanelBgData.pos.x + 10,
        .pos.y = t1PanelData.pos.y + indicatorHeight + 10,
        .dim.width = indicatorWidth,
        .dim.height = indicatorHeight,
        .round = 5,
        .color = g_pCurrentTheme->palette.background,
		.borderWidth = 1,
		.bIsDirty = true,
	};
	voPanelWidget.eWidgetType = WD_TYPE_RECT; voPanelWidget.pvWidget = (void *)&voPanelData;
	
	gfx_WidgetStyle_e indicatorLabelStyle = STYLE_TEXT_MUTED;

    indicator1LbData = (gfx_Label){ .text = indicatorsName[0], .name = "t1", .pos.x = 35, .pos.y = currentPanelData.pos.y + indicatorHeight / 5, .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER), .typo = TYPO_CAPTION, .style = indicatorLabelStyle, .isVisible = true };
    currentLabelWidget.eWidgetType = WD_TYPE_LABEL; currentLabelWidget.pvWidget = (void *)&indicator1LbData;

    indicator2LbData = (gfx_Label){ .text = indicatorsName[1], .name = "t2", .pos.x = 35, .pos.y = t1PanelData.pos.y + indicatorHeight / 5, .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER), .typo = TYPO_CAPTION, .style = indicatorLabelStyle, .isVisible = true };
    t1LabelWidget.eWidgetType = WD_TYPE_LABEL; t1LabelWidget.pvWidget = (void *)&indicator2LbData;

    indicator3LbData = (gfx_Label){ .text = indicatorsName[2], .name = "t3", .pos.x = 35, .pos.y = voPanelData.pos.y + indicatorHeight / 5, .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER), .typo = TYPO_CAPTION, .style = indicatorLabelStyle, .isVisible = true };
    voLabelWidget.eWidgetType = WD_TYPE_LABEL; voLabelWidget.pvWidget = (void *)&indicator3LbData;

    indicator1ValueData = (gfx_Label){ .text = t1Buffer, .name = "tval1", .pos.x = 95, .pos.y = indicator1LbData.pos.y + 10, .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_TOP), .typo = TYPO_H1, .style = STYLE_SUCCESS, .isVisible = true };
    currentValueWidget.eWidgetType = WD_TYPE_LABEL; currentValueWidget.pvWidget = (void *)&indicator1ValueData;

    indicator2ValueData = (gfx_Label){ .text = t2Buffer, .name = "tval2", .pos.x = 95, .pos.y = indicator2LbData.pos.y + 10, .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_TOP), .typo = TYPO_H1, .style = STYLE_DANGER, .isVisible = true };
    t1ValueWidget.eWidgetType = WD_TYPE_LABEL; t1ValueWidget.pvWidget = (void *)&indicator2ValueData;

	uint16_t fWidth, fHeight;
	FontEngine_GetStringDimensions(usbBuffer, Theme_ResolveFontId(TYPO_H3), &fWidth, &fHeight, 1);
    indicator3ValueData = (gfx_Label){ 
		.text = t3Buffer, 
		.name = "tval3", 
		.pos.x = 95, 
		.pos.y = indicator3LbData.pos.y + 10, 
		.alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_TOP), 
		.typo = TYPO_H1, 
		.style = STYLE_SECONDARY, 
		.isVisible = true,
		.oldSize.height = fHeight,
		.oldSize.width = fWidth,
	};
    voValueWidget.eWidgetType = WD_TYPE_LABEL; voValueWidget.pvWidget = (void *)&indicator3ValueData;

    // --- PANEL 2: STATUS (Derecha) ---
    statusPanelBgData = (gfx_Rectangle){
        .name = "statBg",
        .pos.x = panelWidth + 20,
        .pos.y = 75,
        .dim.width = panelWidth,
        .dim.height = panelHeight,
        .round = 5,
        .color = g_pCurrentTheme->palette.surface, 
		.borderWidth = 1,
    };
    statusPanelBgWidget.eWidgetType = WD_TYPE_RECT; statusPanelBgWidget.pvWidget = (void *)&statusPanelBgData;

	sysStatusPanelData= (gfx_Rectangle){
        .name = "vinBgPanel",
        .pos.x = statusPanelBgData.pos.x + 10,
        .pos.y = sensorPanelBgData.pos.y + 10,
        .dim.width = indicatorWidth,
        .dim.height = indicatorHeight,
        .round = 5,
        .color = g_pCurrentTheme->palette.background,
		.borderWidth = 1,
	};
	sysStatusPanelWidget.eWidgetType = WD_TYPE_RECT; sysStatusPanelWidget.pvWidget = (void *)&sysStatusPanelData;

	instStatusPanelData = (gfx_Rectangle){
        .name = "t2Bg",
        .pos.x = statusPanelBgData.pos.x + 10,
        .pos.y = sysStatusPanelData.pos.y + indicatorHeight + 10,
        .dim.width = indicatorWidth,
        .dim.height = indicatorHeight,
        .round = 5,
        .color = g_pCurrentTheme->palette.background,
		.borderWidth = 1,
	};
	instStatusPanelWidget.eWidgetType = WD_TYPE_RECT; instStatusPanelWidget.pvWidget = (void *)&instStatusPanelData;

	usbPanelData = (gfx_Rectangle){
        .name = "t3Bg",
        .pos.x = statusPanelBgData.pos.x + 10,
        .pos.y = instStatusPanelData.pos.y + indicatorHeight + 10,
        .dim.width = indicatorWidth,
        .dim.height = indicatorHeight,
        .round = 5,
        .color = g_pCurrentTheme->palette.background,
		.borderWidth = 1,
		.bIsDirty = true,
	};
	usbStatusPanelWidget.eWidgetType = WD_TYPE_RECT; usbStatusPanelWidget.pvWidget = (void *)&usbPanelData;

    statusPanelTitleData = (gfx_Label){ .text = "SISTEMA", .name = "tStat", .pos.x = usbPanelData.pos.x + usbPanelData.dim.width / 2, .pos.y = statusPanelBgData.pos.y + 30, .alignment = ALIGN_HCENTER, .typo = TYPO_H3, .style = STYLE_SECONDARY, .isVisible = true };
    statusPanelTitleWidget.eWidgetType = WD_TYPE_LABEL; statusPanelTitleWidget.pvWidget = (void *)&statusPanelTitleData;

    indicator4LbData = (gfx_Label){ .text = indicatorsName[3], .name = "vin", .pos.x = sysStatusPanelData.pos.x + 20, .pos.y = sysStatusPanelData.pos.y + sysStatusPanelData.dim.height / 5, .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER), .typo = TYPO_CAPTION, .style = indicatorLabelStyle, .isVisible = true };
    sysStatusLabelWidget.eWidgetType = WD_TYPE_LABEL; sysStatusLabelWidget.pvWidget = (void *)&indicator4LbData;

    indicator5LbData = (gfx_Label){ .text = indicatorsName[4], .name = "vout", .pos.x = instStatusPanelData.pos.x + 20, .pos.y = instStatusPanelData.pos.y + instStatusPanelData.dim.height / 5, .alignment = ( gfx_Align_e )(ALIGN_LEFT | ALIGN_VCENTER), .typo = TYPO_CAPTION, .style = indicatorLabelStyle, .isVisible = true };
    voutLabelWidget.eWidgetType = WD_TYPE_LABEL; voutLabelWidget.pvWidget = (void *)&indicator5LbData;

    indicator6LbData = (gfx_Label){ .text = indicatorsName[5], .name = "sysSt", .pos.x = usbPanelData.pos.x + 20, .pos.y = usbPanelData.pos.y + usbPanelData.dim.height / 5, .alignment = ( gfx_Align_e )(ALIGN_LEFT | ALIGN_VCENTER), .typo = TYPO_CAPTION, .style = indicatorLabelStyle, .isVisible = true };
    usbStatusLabelWidget.eWidgetType = WD_TYPE_LABEL; usbStatusLabelWidget.pvWidget = (void *)&indicator6LbData;

    indicator4ValueData = (gfx_Label){ .text = sysStatusBuffer, .name = "vinVal", .pos.x = sysStatusPanelData.pos.x + 70, .pos.y = indicator4LbData.pos.y + 10, .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_TOP), .typo = TYPO_H1, .style = STYLE_SUCCESS, .isVisible = true };
    sysStatusValueWidget.eWidgetType = WD_TYPE_LABEL; sysStatusValueWidget.pvWidget = (void *)&indicator4ValueData;

	FontEngine_GetStringDimensions(instStatusBuffer, Theme_ResolveFontId(TYPO_H3), &fWidth, &fHeight, 1);
   	indicator5ValueData = (gfx_Label){ 
		.text = instStatusBuffer, 
		.name = "voutVal", 
		.pos.x = instStatusPanelData.pos.x + 70, 
		.pos.y = indicator5LbData.pos.y + 10, 
		.alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_TOP), 
		.typo = TYPO_H1, 
		.style = STYLE_DANGER, 
		.isVisible = true,
		.oldPos.x = instStatusPanelData.pos.x + 70, 
		.oldPos.y = indicator5LbData.pos.y + 10, 
		.oldSize.width = fWidth,
		.oldSize.height = fHeight,
	};
    instStatusValueWidget.eWidgetType = WD_TYPE_LABEL; instStatusValueWidget.pvWidget = (void *)&indicator5ValueData;
    
	FontEngine_GetStringDimensions(usbBuffer, Theme_ResolveFontId(TYPO_H3), &fWidth, &fHeight, 1);
    indicator6ValueData = (gfx_Label){ 
		.text = usbBuffer, 
		.name = "tval3", 
		.pos.x = usbPanelData.pos.x + 70, 
		.pos.y = indicator6LbData.pos.y + 20, 
		.alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_TOP), 
		.typo = TYPO_H3, 
		.style = STYLE_DANGER, 
		.isVisible = true, 
		.oldPos.x = usbPanelData.pos.x + 70, 
		.oldPos.y = indicator6LbData.pos.y + 20,
		.oldSize.width = fWidth,
		.oldSize.height = fHeight,
	 };
    usbStatusValueWidget.eWidgetType = WD_TYPE_LABEL; usbStatusValueWidget.pvWidget = (void *)&indicator6ValueData;

	// Icons
	indicator1IconData = (gfx_Label){
		.name = "indicator1IconData",
		.text = icons[0],
		.pos.x = 25,
		.pos.y = indicator1ValueData.pos.y + 10,
		.alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_TOP),
		.typo = TYPO_ICON,
		.style = STYLE_SUCCESS,
		.isVisible = true,
	};
	currentIconWidget.eWidgetType = WD_TYPE_LABEL;
	currentIconWidget.pvWidget = (void *)&indicator1IconData;

	indicator2IconData = (gfx_Label){
		.name = "tempIconData",
		.text = icons[1],
		.pos.x = 25,
		.pos.y = indicator2ValueData.pos.y + 10,
		.alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_TOP),
		.typo = TYPO_ICON,
		.style = STYLE_DANGER,
		.isVisible = true,
	};
	temperatureIconWidget.eWidgetType = WD_TYPE_LABEL;
	temperatureIconWidget.pvWidget = (void *)&indicator2IconData;
	
	indicator3IconData = (gfx_Label){
		.name = "indicator3IconData",
		.text = icons[2],
		.pos.x = 25,
		.pos.y = indicator3ValueData.pos.y + 10,
		.alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_TOP),
		.typo = TYPO_ICON,
		.style = STYLE_SECONDARY,
		.isVisible = true,
	};
	tensionIconWidget.eWidgetType = WD_TYPE_LABEL;
	tensionIconWidget.pvWidget = (void *)&indicator3IconData;

	indicator4IconData = (gfx_Label){
		.name = "sysStatusIcon",
		.text = icons[3],
		.pos.x = sysStatusPanelData.pos.x + 10,
		.pos.y = indicator4ValueData.pos.y + 10,
		.alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_TOP),
		.typo = TYPO_ICON,
		.style = STYLE_SUCCESS,
		.isVisible = true,
	};
	sysStatusIconWidget.eWidgetType = WD_TYPE_LABEL;
	sysStatusIconWidget.pvWidget = (void *)&indicator4IconData;

	indicator5IconData = (gfx_Label){
		.name = "instStatusIcon",
		.text = icons[4],
		.pos.x = instStatusPanelData.pos.x + 10,
		.pos.y = indicator5ValueData.pos.y + 10,
		.alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_TOP),
		.typo = TYPO_ICON,
		.style = STYLE_PRIMARY,
		.isVisible = true,
	};
	instStatusIconWidget.eWidgetType = WD_TYPE_LABEL;
	instStatusIconWidget.pvWidget = (void *)&indicator5IconData;

	indicator6IconData = (gfx_Label){
		.name = "usbIcon",
		.text = icons[5],
		.pos.x = usbPanelData.pos.x + 10,
		.pos.y = indicator6ValueData.pos.y ,
		.alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_TOP),
		.typo = TYPO_ICON,
		.style = STYLE_DANGER,
		.isVisible = true,
	};
	usbIconWidget.eWidgetType = WD_TYPE_LABEL;
	usbIconWidget.pvWidget = (void *)&indicator6IconData;

	leftArrowButtonData = (gfx_Button) {
		.label = ">",
		.pos.x = statusPanelBgData.pos.x + statusPanelBgData.dim.width + 10,
		.pos.y = statusPanelBgData.pos.y,
		.size.width = 35,
		.size.height = panelHeight,
		.radius = 2,
		.borderWidth = 2,
		.state = BTN_STATE_NORMAL,
		.style = STYLE_DEFAULT,
		.typo = TYPO_H3,
		.bIsVisible = true,
		.onPressed = onGenericBtnPressed,
		.onRelease = onLeftArrowReleaseEvent,
	};
	gfx_initRegTouch((void *)&leftArrowButtonData, WD_TYPE_BUTTON);
	leftArrowButtonWidget.eWidgetType = WD_TYPE_BUTTON;
	leftArrowButtonWidget.pvWidget = (void *)&leftArrowButtonData;
	
	rightArrowButtonData = (gfx_Button) {
		.label = "<",
		.pos.x = 5, 
		.pos.y = statusPanelBgData.pos.y,
		.size.width = 30,
		.size.height = panelHeight,
		.radius = 2,
		.borderWidth = 2,
		.state = BTN_STATE_NORMAL,
		.style = STYLE_DEFAULT,
		.typo = TYPO_H3,
		.bIsVisible = false,
		.onPressed = onGenericBtnPressed,
		.onRelease = onRightArrowReleaseEvent,
	};
	gfx_initRegTouch((void *)&rightArrowButtonData, WD_TYPE_BUTTON);
	rightArrowButtonWidget.eWidgetType = WD_TYPE_BUTTON;
	rightArrowButtonWidget.pvWidget = (void *)&rightArrowButtonData;
    // ==========================================
    // 5. Insertion into Canvas (Back-to-front)
    // ==========================================
    // Fondos de paneles
	useFullHeader(&g_sSystemCanvas);
	canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &titleWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &tempPanelBgWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &statusPanelBgWidget);
	canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &currentPanelWidget);
	canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &t1PanelWidget);
	canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &voPanelWidget);
	canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &sysStatusPanelWidget);
	canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &instStatusPanelWidget);
	canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &usbStatusPanelWidget);


    // Contenido Temperaturas
    //canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &tempPanelTitleWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &currentLabelWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &t1LabelWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &voLabelWidget);

    // Contenido Status
    // canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &statusPanelTitleWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &sysStatusLabelWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &voutLabelWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &usbStatusLabelWidget);
	   
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &currentValueWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &t1ValueWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &voValueWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &sysStatusValueWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &instStatusValueWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &usbStatusValueWidget);

	// Iconos
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &currentIconWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &temperatureIconWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &tensionIconWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &sysStatusIconWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &instStatusIconWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &usbIconWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &leftArrowButtonWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &rightArrowButtonWidget);

	useNavigationButtons(&g_sSystemCanvas);
	useLogoWidget(&g_sSystemCanvas);

	// Subscribe to events
	Event_Subscribe(EVT_CAN_INST_CURRENT_SECUNDARY, ( EventHandler_fn )onCurrentSecondaryValueChanged);
	Event_Subscribe(EVT_CAN_INST_VOLTAGE_SECONDARY, ( EventHandler_fn )onVoltageSecondaryValueChanged);
	Event_Subscribe(EVT_CAN_INST_TEMP_PROBE_MAIN, ( EventHandler_fn )onTempProbeMainValueChanged);
	Event_Subscribe(EVT_CAN_INST_TEMP_TX_PRIMARY, ( EventHandler_fn )onTempTxPrimaryValueChanged);
	Event_Subscribe(EVT_CAN_INST_TEMP_TX_SECONDARY, ( EventHandler_fn )onTempTxSecondaryValueChanged);
	Event_Subscribe(EVT_CAN_INST_TEMP_PROBE_SECONDARY, ( EventHandler_fn )onTempProbeSecondaryValueChanged);

	Event_Subscribe(EVT_SYS_USB_CONNECTED, (EventHandler_fn) onUSBConnected);
	Event_Subscribe(EVT_SYS_USB_DISCONNECTED, (EventHandler_fn) onUSBDisconnected);
	Event_Subscribe(EVT_SYS_USB_POWER_FAULT, (EventHandler_fn)onUSBPowerFault);
	Event_Subscribe(EVT_SYS_USB_UNKNOWN_DEVICE, (EventHandler_fn)onUSBUnknownDevice);
	Event_Subscribe(EVT_SYS_BOOT_HANDSHAKE_OK, (EventHandler_fn)onInstSyncEvent);
	Event_Subscribe(EVT_SYS_INST_SYNC_RESTORED, (EventHandler_fn)onInstSyncEvent);
	Event_Subscribe(EVT_SYS_INST_UNSYNC, (EventHandler_fn)onInstUnsyncEvent);
	Event_Subscribe(EVT_CAN_INST_STATE, ( EventHandler_fn )onInstStateEvent);

    // Register Form
    g_i16DashboardFormID = FormManager_AddForm(&g_sSystemCanvas);
}



