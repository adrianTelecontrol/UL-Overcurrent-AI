#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "gui_core.h"
#include "gui_theme.h"
#include "gui_canvas.h"
#include "FT8xx_params.h"
#include "forms_manager.h"
#include "event_engine.h"
#include "forms/common_widgets.h" 
#include "debug_menu_form.h"

int16_t g_i16DebugMenuFormIndex = 0;
static gfx_Canvas g_sDebugMenuCanvas;

// Widgets
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget ioBtnWidget, variacBtnWidget, dashboardBtnWidget;
static gfx_Button ioBtnData, variacBtnData, dashboardBtnData;

static gfx_Label titleData;

static void onDebugSelectionRelease(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    
    // Identificar qué botón se presionó
    if (btn == &ioBtnData) {
        Event_Post(EVT_SYS_SHOW_DEBUG_IO_FORM, (EventParam_t){.ptr = NULL});
    } else if (btn == &variacBtnData) {
        Event_Post(EVT_SYS_SHOW_VARIAC_DEBUG_FORM, (EventParam_t){.ptr = NULL});
    } else if (btn == &dashboardBtnData) {
        Event_Post(EVT_SYS_SHOW_READINGS_DASHBOARD_FORM, (EventParam_t){.ptr = NULL});
    }
}

void initDebugMenuForm(void) {
    g_sDebugMenuCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;

    // Título
    titleData = (gfx_Label){ 
		.text = "DIAGNOSTICOS", 
        .pos.x = 125, .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
	};
    formTitleWidget.eWidgetType = WD_TYPE_LABEL; 
	formTitleWidget.pvWidget = &titleData;

    // Dimensiones para los 3 botones verticales
    uint16_t btnWidth = 350;
    uint16_t btnHeight = 80;
    uint16_t startY = 90;
    uint16_t gap = 30;

    // Botón 1: E/S Digitales
    ioBtnData = (gfx_Button){
        .label = "E/S DIGITALES", 
		.pos.x = (LCD_WIDTH - btnWidth)/2, 
		.pos.y = startY,
        .size.width = btnWidth, 
		.size.height = btnHeight, 
		.style = STYLE_DEFAULT, 
		.typo = TYPO_H3,
        .onPressed = onGenericBtnPressed, 
		.onRelease = onDebugSelectionRelease,
		.bIsVisible = true,
		.radius = 4,
		.borderWidth = 3,
    };
    gfx_initRegTouch(&ioBtnData, WD_TYPE_BUTTON);
    ioBtnWidget.eWidgetType = WD_TYPE_BUTTON; 
	ioBtnWidget.pvWidget = &ioBtnData;

    // Botón 2: Variac
    variacBtnData = (gfx_Button){
        .label = "CONTROL VARIAC", 
		.pos.x = (LCD_WIDTH - btnWidth)/2, 
		.pos.y = startY + btnHeight + gap,
        .size.width = btnWidth, 
		.size.height = btnHeight, 
		.style = STYLE_SECONDARY, 
		.typo = TYPO_H3,
        .onPressed = onGenericBtnPressed, 
		.onRelease = onDebugSelectionRelease,
		.bIsVisible = true,
		.radius = 4,
		.borderWidth = 3,
    };
    gfx_initRegTouch(&variacBtnData, WD_TYPE_BUTTON);
    variacBtnWidget.eWidgetType = WD_TYPE_BUTTON; variacBtnWidget.pvWidget = &variacBtnData;

    // Botón 3: Dashboard de Lecturas
    dashboardBtnData = (gfx_Button){
        .label = "LECTURAS SISTEMA", 
		.pos.x = (LCD_WIDTH - btnWidth)/2, 
		.pos.y = startY + (btnHeight + gap) * 2,
        .size.width = btnWidth, 
		.size.height = btnHeight, 
		.style = STYLE_DEFAULT, 
		.typo = TYPO_H3,
        .onPressed = onGenericBtnPressed, 
		.onRelease = onDebugSelectionRelease,
		.bIsVisible = true,
		.radius = 4,
		.borderWidth = 3,
    };
    gfx_initRegTouch(&dashboardBtnData, WD_TYPE_BUTTON);
    dashboardBtnWidget.eWidgetType = WD_TYPE_BUTTON; dashboardBtnWidget.pvWidget = &dashboardBtnData;

    // Ensamble
    useFullHeader(&g_sDebugMenuCanvas);
    useNavigationButtons(&g_sDebugMenuCanvas);
    canvasInsertAtTop(&g_sDebugMenuCanvas.psWidgets, &formTitleWidget);
    canvasInsertAtTop(&g_sDebugMenuCanvas.psWidgets, &ioBtnWidget);
    canvasInsertAtTop(&g_sDebugMenuCanvas.psWidgets, &variacBtnWidget);
    canvasInsertAtTop(&g_sDebugMenuCanvas.psWidgets, &dashboardBtnWidget);

    g_i16DebugMenuFormIndex = FormManager_AddForm(&g_sDebugMenuCanvas);
}