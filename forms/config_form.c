
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gui_core.h"
#include "gui_canvas.h"
#include "gui_theme.h"
#include "forms_manager.h"
#include "common_widgets.h"
#include "FT8xx_params.h"
#include "file_manager.h"
#include "rtc_module.h"
#include "event_engine.h"

#include "config_form.h"

gfx_GenericWidget writeLogButtonContainer;
gfx_GenericWidget startLogButtonContainer;
gfx_GenericWidget stopLogButtonContainer;
gfx_GenericWidget logCountLabelContainer;

gfx_Button writeLogWidget;
gfx_Button startLogWidget;
gfx_Button stopLogWidget;
gfx_Label logCountWidget;

int16_t g_i16ConfigFormID;

static uint32_t g_logCount = 0;
static char countBuffer[] = "100000 logs";
static gfx_Canvas g_sConfigCanvas;

static void onWriteLogButtonReleased(gfx_Button *btn) {
	onGenericBtnRelease(btn);
	
	char pcData[120];
    char dateStr[12];
	char timeStr[12];
	// char psFilePath[50];
	RTC_getFormattedDate(dateStr, sizeof(dateStr));
	RTC_getFormattedTime(timeStr, sizeof(timeStr));
	snprintf(pcData, sizeof(pcData), "%s %s - %s", dateStr, timeStr, "This is just to test if the usb and log engine is working! Please ignore me :)");
	
	FM_WriteFile(DRIVE_USB_ID, "test2.txt", (uint8_t *)pcData, sizeof(pcData));
}

static void onStartLogButtonRelease(gfx_Button *btn) {
	onGenericBtnRelease(btn);
	
	Event_Post(EVT_SYS_USB_START_LOG, (EventParam_t){.ptr = NULL});
}

static void onStopLogButtonRelease(gfx_Button *btn) {
	onGenericBtnRelease(btn);

	Event_Post(EVT_SYS_USB_STOP_LOG, (EventParam_t){.ptr = NULL});
}

static void onLogAddedEvent(EventParam_t param) {
	g_logCount++;
	snprintf(countBuffer, sizeof(countBuffer), "%u logs", g_logCount);
	logCountWidget.bIsDirty = true;	
}

void initConfigForm(void) {
	g_sConfigCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
	g_sConfigCanvas.psWidgets = NULL;

	writeLogWidget = (gfx_Button) {
		.label = "Test Log",
		.pos.x = LCD_WIDTH / 2 - 150,
		.pos.y = LCD_HEIGHT * 2.0 / 3.0 - 60,
		.size.width = 300,
		.size.height = 120,
		.borderWidth = 1,
		.radius = 6,
		.style = STYLE_SECONDARY,
		.typo = TYPO_H1,
		.onPosChanged = NULL,
		.onPressed = onGenericBtnPressed,
		.onRelease = onWriteLogButtonReleased,
	};
	gfx_initRegTouch((void *)&writeLogWidget, WD_TYPE_BUTTON);
	writeLogButtonContainer.eWidgetType = WD_TYPE_BUTTON;
	writeLogButtonContainer.pvWidget = (void *)&writeLogWidget;

	startLogWidget = (gfx_Button) {
		.label = "Start Log",
		.pos.x = LCD_WIDTH * 1.0 / 3.0 - 125,
		.pos.y = LCD_HEIGHT * 1.0 / 3.0 - 60,
		.size.width = 250,
		.size.height = 120,
		.borderWidth = 1,
		.radius = 6,
		.style = STYLE_SUCCESS,
		.typo = TYPO_H1,
		.onPosChanged = NULL,
		.onPressed = onGenericBtnPressed,
		.onRelease = onStartLogButtonRelease,
	};
	gfx_initRegTouch((void *)&startLogWidget, WD_TYPE_BUTTON);
	startLogButtonContainer.eWidgetType = WD_TYPE_BUTTON;
	startLogButtonContainer.pvWidget = (void *)&startLogWidget;

	stopLogWidget = (gfx_Button) {
		.label = "Stop Log",
		.pos.x = LCD_WIDTH * 2.0 / 3.0 - 125,
		.pos.y = LCD_HEIGHT * 1.0 / 3.0 - 60,
		.size.width = 250,
		.size.height = 120,
		.borderWidth = 1,
		.radius = 6,
		.style = STYLE_DANGER,
		.typo = TYPO_H1,
		.onPosChanged = NULL,
		.onPressed = onGenericBtnPressed,
		.onRelease = onStopLogButtonRelease,
	};
	gfx_initRegTouch((void *)&stopLogWidget, WD_TYPE_BUTTON);
	stopLogButtonContainer.eWidgetType = WD_TYPE_BUTTON;
	stopLogButtonContainer.pvWidget = (void *)&stopLogWidget;

	logCountWidget = (gfx_Label) {
		.text = countBuffer,
		.pos.x = 10,
		.pos.y = writeLogWidget.pos.y + writeLogWidget.size.width / 2,
		.alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER),
		.isVisible = true,
		.style = STYLE_DANGER,
		.typo = TYPO_MONO,
	};
	logCountLabelContainer.eWidgetType = WD_TYPE_LABEL;
	logCountLabelContainer.pvWidget = (void *)&logCountWidget;
	strcpy(countBuffer, "0 logs");
	
	useFullHeader(&g_sConfigCanvas);
	useNavigationButtons(&g_sConfigCanvas);
	canvasInsertAtTop(&g_sConfigCanvas.psWidgets, &writeLogButtonContainer);
	canvasInsertAtTop(&g_sConfigCanvas.psWidgets, &startLogButtonContainer);
	canvasInsertAtTop(&g_sConfigCanvas.psWidgets, &stopLogButtonContainer);
	canvasInsertAtTop(&g_sConfigCanvas.psWidgets, &logCountLabelContainer);

	Event_Subscribe(EVT_SYS_USB_LOG_ADDED, (EventHandler_fn)onLogAddedEvent);
	
	g_i16ConfigFormID = FormManager_AddForm(&g_sConfigCanvas);
}


