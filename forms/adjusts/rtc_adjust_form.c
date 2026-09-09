#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "gui_core.h"
#include "gui_theme.h"
#include "gui_canvas.h"
#include "forms_manager.h"
#include "FT8xx_params.h"
#include "event_engine.h"
#include "rtc_module.h"
#include "forms/common_widgets.h"

#include "rtc_adjust_form.h"

int16_t g_i16RtcAdjustFormIndex = 0;
static gfx_Canvas g_sRtcAdjustCanvas;

// Enumerador para saber qué estamos ajustando
typedef enum {
    SEL_DAY, SEL_MONTH, SEL_YEAR,
    SEL_HOUR, SEL_MIN, SEL_SEC
} RtcField_e;

static RtcField_e g_eSelectedField = SEL_DAY;

// Variables temporales para el ajuste
static uint8_t adj_day = 1, adj_month = 1;
static uint16_t adj_year = 2026;
static uint8_t adj_hour = 12, adj_min = 0, adj_sec = 0;

// Buffers de texto para los botones
static char str_day[4], str_month[4], str_year[6];
static char str_hour[4], str_min[4], str_sec[4];

// Widgets
static gfx_GenericWidget titleWidget, subtitleWidget;
static gfx_GenericWidget btnDayW, btnMonthW, btnYearW;
static gfx_GenericWidget btnHourW, btnMinW, btnSecW;
static gfx_GenericWidget btnPlusW, btnMinusW;
static gfx_GenericWidget btnSaveW, btnBackW;
static gfx_GenericWidget sepDate1W, sepDate2W, sepTime1W, sepTime2W; // Separadores "/" y ":"

// Data
static gfx_Label titleData, subtitleData;
static gfx_Button btnDayData, btnMonthData, btnYearData;
static gfx_Button btnHourData, btnMinData, btnSecData;
static gfx_Button btnPlusData, btnMinusData;
static gfx_Button btnSaveData, btnBackData;
static gfx_Label sepDate1Data, sepDate2Data, sepTime1Data, sepTime2Data;

// ========================================================
// RUTINAS DE ACTUALIZACIÓN VISUAL
// ========================================================
static void updateRtcDisplay(void) {
    // Actualizar textos con ceros a la izquierda
    sprintf(str_day, "%02d", adj_day);
    sprintf(str_month, "%02d", adj_month);
    sprintf(str_year, "%04d", adj_year);
    sprintf(str_hour, "%02d", adj_hour);
    sprintf(str_min, "%02d", adj_min);
    sprintf(str_sec, "%02d", adj_sec);

    // Actualizar estilos (Resaltar el seleccionado)
    btnDayData.style   = (g_eSelectedField == SEL_DAY)   ? STYLE_DANGER : STYLE_SECONDARY;
    btnMonthData.style = (g_eSelectedField == SEL_MONTH) ? STYLE_DANGER : STYLE_SECONDARY;
    btnYearData.style  = (g_eSelectedField == SEL_YEAR)  ? STYLE_DANGER : STYLE_SECONDARY;
    btnHourData.style  = (g_eSelectedField == SEL_HOUR)  ? STYLE_DANGER : STYLE_SECONDARY;
    btnMinData.style   = (g_eSelectedField == SEL_MIN)   ? STYLE_DANGER : STYLE_SECONDARY;
    btnSecData.style   = (g_eSelectedField == SEL_SEC)   ? STYLE_DANGER : STYLE_SECONDARY;

    // Marcar como sucios para forzar repintado
    btnDayData.bIsDirty = true; btnMonthData.bIsDirty = true; btnYearData.bIsDirty = true;
    btnHourData.bIsDirty = true; btnMinData.bIsDirty = true; btnSecData.bIsDirty = true;
    
    //Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

// ========================================================
// CALLBACKS DE SELECCIÓN
// ========================================================
static void onFieldSelected(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    if(btn == &btnDayData) g_eSelectedField = SEL_DAY;
    else if(btn == &btnMonthData) g_eSelectedField = SEL_MONTH;
    else if(btn == &btnYearData) g_eSelectedField = SEL_YEAR;
    else if(btn == &btnHourData) g_eSelectedField = SEL_HOUR;
    else if(btn == &btnMinData) g_eSelectedField = SEL_MIN;
    else if(btn == &btnSecData) g_eSelectedField = SEL_SEC;
    updateRtcDisplay();
}

// ========================================================
// CALLBACKS DE INCREMENTO / DECREMENTO
// ========================================================
static void getDaysInMonth(uint8_t m, uint16_t y, uint8_t *max_days) {
    if(m == 2) {
        *max_days = ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) ? 29 : 28;
    } else if(m == 4 || m == 6 || m == 9 || m == 11) {
        *max_days = 30;
    } else {
        *max_days = 31;
    }
}

static void onPlusBtnRelease(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    uint8_t max_days = 31;
    
    switch(g_eSelectedField) {
        case SEL_DAY:
            getDaysInMonth(adj_month, adj_year, &max_days);
            if(++adj_day > max_days) adj_day = 1;
            break;
        case SEL_MONTH:
            if(++adj_month > 12) adj_month = 1;
            // Ajustar día si el nuevo mes tiene menos días (ej. de 31 Ene a Feb)
            getDaysInMonth(adj_month, adj_year, &max_days);
            if(adj_day > max_days) adj_day = max_days;
            break;
        case SEL_YEAR:
            if(++adj_year > 2099) adj_year = 2000;
            break;
        case SEL_HOUR:
            if(++adj_hour > 23) adj_hour = 0;
            break;
        case SEL_MIN:
            if(++adj_min > 59) adj_min = 0;
            break;
        case SEL_SEC:
            if(++adj_sec > 59) adj_sec = 0;
            break;
    }
    updateRtcDisplay();
}

static void onMinusBtnRelease(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    uint8_t max_days = 31;

    switch(g_eSelectedField) {
        case SEL_DAY:
            getDaysInMonth(adj_month, adj_year, &max_days);
            if(--adj_day < 1) adj_day = max_days;
            break;
        case SEL_MONTH:
            if(--adj_month < 1) adj_month = 12;
            getDaysInMonth(adj_month, adj_year, &max_days);
            if(adj_day > max_days) adj_day = max_days;
            break;
        case SEL_YEAR:
            if(--adj_year < 2000) adj_year = 2099;
            break;
        case SEL_HOUR:
            if(adj_hour == 0) adj_hour = 23; else adj_hour--;
            break;
        case SEL_MIN:
            if(adj_min == 0) adj_min = 59; else adj_min--;
            break;
        case SEL_SEC:
            if(adj_sec == 0) adj_sec = 59; else adj_sec--;
            break;
    }
    updateRtcDisplay();
}

// ========================================================
// CALLBACKS DE GUARDADO / RETORNO
// ========================================================
static void onSaveBtnRelease(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    
    // Aquí invocas las funciones de tu RTC para actualizar el hardware
    // Ejemplo:
	RTC_setDate(adj_day, adj_month, adj_year);
    RTC_setTime(adj_hour, adj_min, adj_sec);
    
    // Volver al menú de opciones
    //Event_Post(EVT_SYS_SHOW_OPTIONS_FORM, (EventParam_t){.ptr = NULL});
}

static void onBackBtnRelease(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    Event_Post(EVT_SYS_SHOW_OPTIONS_FORM, (EventParam_t){.ptr = NULL});
}

static void onShowThisFormEvent(EventParam_t arg) {
    // Precargar variables temporales con los valores actuales del RTC real
    // Ejemplo: RTC_getCurrentDateTime(&adj_day, &adj_month, &adj_year, &adj_hour, &adj_min, &adj_sec);
	RTC_getCurrentDateTime(&adj_day, &adj_month, &adj_year, &adj_hour, &adj_min, &adj_sec);
    updateRtcDisplay();
}

// ========================================================
// INICIALIZACIÓN
// ========================================================
void initRtcAdjustForm(void) {
    g_sRtcAdjustCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;

    // Títulos
    titleData = (gfx_Label) { .text = "ADJUST RTC", .pos.x = 125, .pos.y = 50, .alignment = ALIGN_LEFT, .typo = TYPO_H3, .style = STYLE_TEXT_MAIN, .isVisible = true };
    titleWidget.eWidgetType = WD_TYPE_LABEL; titleWidget.pvWidget = &titleData;

    subtitleData = (gfx_Label){ .text = "SELECCIONE EL CAMPO Y USE + O - PARA AJUSTAR", .pos.x = LCD_WIDTH / 2, .pos.y = 100, .alignment = ALIGN_CENTER, .style = STYLE_TEXT_MUTED, .typo = TYPO_CAPTION, .isVisible = true };
    subtitleWidget.eWidgetType = WD_TYPE_LABEL; subtitleWidget.pvWidget = &subtitleData;

    // --- MACRO PARA CREAR LOS BOTONES DE CAMPO ---
    #define CREATE_FIELD_BTN(BtnData, BtnWdgt, Buf, X, Y, W) \
        BtnData = (gfx_Button){ .label = Buf, .pos.x = X, .pos.y = Y, .size.width = W, .size.height = 70, .radius = 5, .borderWidth = 2, .style = STYLE_SECONDARY, .typo = TYPO_H1, .bIsVisible = true, .onPressed = onGenericBtnPressed, .onRelease = onFieldSelected }; \
        gfx_initRegTouch(&BtnData, WD_TYPE_BUTTON); \
        BtnWdgt.eWidgetType = WD_TYPE_BUTTON; BtnWdgt.pvWidget = &BtnData;

    float startX = 160;
    float dateY = 140;
    float timeY = 250;
    float smallBtnW = 100;
    float largeBtnW = 140; // Para el año
    float gap = 30;

    // Botones de Fecha
    CREATE_FIELD_BTN(btnDayData, btnDayW, str_day, startX, dateY, smallBtnW)
    CREATE_FIELD_BTN(btnMonthData, btnMonthW, str_month, startX + smallBtnW + gap, dateY, smallBtnW)
    CREATE_FIELD_BTN(btnYearData, btnYearW, str_year, startX + (smallBtnW + gap)*2, dateY, largeBtnW)

    // Botones de Hora
    CREATE_FIELD_BTN(btnHourData, btnHourW, str_hour, startX, timeY, smallBtnW)
    CREATE_FIELD_BTN(btnMinData, btnMinW, str_min, startX + smallBtnW + gap, timeY, smallBtnW)
    CREATE_FIELD_BTN(btnSecData, btnSecW, str_sec, startX + (smallBtnW + gap)*2, timeY, smallBtnW)

    // --- SEPARADORES ---
    #define CREATE_SEP(SepData, SepWdgt, Txt, X, Y) \
        SepData = (gfx_Label) { .text = Txt, .pos.x = X, .pos.y = Y + 35, .alignment = ALIGN_CENTER, .typo = TYPO_H1, .style = STYLE_TEXT_MAIN, .isVisible = true }; \
        SepWdgt.eWidgetType = WD_TYPE_LABEL; SepWdgt.pvWidget = &SepData;

    CREATE_SEP(sepDate1Data, sepDate1W, "/", startX + smallBtnW + (gap/2), dateY)
    CREATE_SEP(sepDate2Data, sepDate2W, "/", startX + smallBtnW*2 + gap + (gap/2), dateY)
    
    CREATE_SEP(sepTime1Data, sepTime1W, ":", startX + smallBtnW + (gap/2), timeY)
    CREATE_SEP(sepTime2Data, sepTime2W, ":", startX + smallBtnW*2 + gap + (gap/2), timeY)

    // --- CONTROLES + / - (Derecha) ---
    float ctrlX = startX + (smallBtnW + gap)*2 + largeBtnW + 50;
    float ctrlSize = 80;

    btnPlusData = (gfx_Button){ .label = "+", .pos.x = ctrlX, .pos.y = dateY, .size.width = ctrlSize, .size.height = ctrlSize, .radius = 45, .borderWidth = 2, .style = STYLE_DEFAULT, .typo = TYPO_H1, .bIsVisible = true, .onPressed = onGenericBtnPressed, .onRelease = onPlusBtnRelease };
    gfx_initRegTouch(&btnPlusData, WD_TYPE_BUTTON);
    btnPlusW.eWidgetType = WD_TYPE_BUTTON; btnPlusW.pvWidget = &btnPlusData;

    btnMinusData = (gfx_Button){ .label = "-", .pos.x = ctrlX, .pos.y = timeY + 70 - ctrlSize, .size.width = ctrlSize, .size.height = ctrlSize, .radius = 45, .borderWidth = 2, .style = STYLE_DEFAULT, .typo = TYPO_H1, .bIsVisible = true, .onPressed = onGenericBtnPressed, .onRelease = onMinusBtnRelease };
    gfx_initRegTouch(&btnMinusData, WD_TYPE_BUTTON);
    btnMinusW.eWidgetType = WD_TYPE_BUTTON; btnMinusW.pvWidget = &btnMinusData;

    // --- BOTONES DE ACCIÓN (Abajo) ---
    btnBackData = (gfx_Button){ .label = "REGRESAR", .pos.x = 20, .pos.y = LCD_HEIGHT - 140, .size.width = 180, .size.height = 60, .radius = 5, .borderWidth = 2, .style = STYLE_DANGER, .typo = TYPO_H3, .bIsVisible = true, .onPressed = onGenericBtnPressed, .onRelease = onBackBtnRelease };
    gfx_initRegTouch(&btnBackData, WD_TYPE_BUTTON);
    btnBackW.eWidgetType = WD_TYPE_BUTTON; btnBackW.pvWidget = &btnBackData;

    btnSaveData = (gfx_Button){ .label = "GUARDAR", .pos.x = LCD_WIDTH - 200, .pos.y = LCD_HEIGHT - 140, .size.width = 180, .size.height = 60, .radius = 5, .borderWidth = 2, .style = STYLE_SUCCESS, .typo = TYPO_H3, .bIsVisible = true, .onPressed = onGenericBtnPressed, .onRelease = onSaveBtnRelease };
    gfx_initRegTouch(&btnSaveData, WD_TYPE_BUTTON);
    btnSaveW.eWidgetType = WD_TYPE_BUTTON; btnSaveW.pvWidget = &btnSaveData;

    // Ensamble
    useFullHeader(&g_sRtcAdjustCanvas);
    useNavigationButtons(&g_sRtcAdjustCanvas);
    
    canvasInsertAtTop(&g_sRtcAdjustCanvas.psWidgets, &titleWidget);
    canvasInsertAtTop(&g_sRtcAdjustCanvas.psWidgets, &subtitleWidget);
    
    canvasInsertAtTop(&g_sRtcAdjustCanvas.psWidgets, &btnDayW); canvasInsertAtTop(&g_sRtcAdjustCanvas.psWidgets, &btnMonthW); canvasInsertAtTop(&g_sRtcAdjustCanvas.psWidgets, &btnYearW);
    canvasInsertAtTop(&g_sRtcAdjustCanvas.psWidgets, &btnHourW); canvasInsertAtTop(&g_sRtcAdjustCanvas.psWidgets, &btnMinW); canvasInsertAtTop(&g_sRtcAdjustCanvas.psWidgets, &btnSecW);
    
    canvasInsertAtTop(&g_sRtcAdjustCanvas.psWidgets, &sepDate1W); canvasInsertAtTop(&g_sRtcAdjustCanvas.psWidgets, &sepDate2W);
    canvasInsertAtTop(&g_sRtcAdjustCanvas.psWidgets, &sepTime1W); canvasInsertAtTop(&g_sRtcAdjustCanvas.psWidgets, &sepTime2W);

    canvasInsertAtTop(&g_sRtcAdjustCanvas.psWidgets, &btnPlusW); canvasInsertAtTop(&g_sRtcAdjustCanvas.psWidgets, &btnMinusW);
    
    canvasInsertAtTop(&g_sRtcAdjustCanvas.psWidgets, &btnBackW); canvasInsertAtTop(&g_sRtcAdjustCanvas.psWidgets, &btnSaveW);

	
	Event_Subscribe(EVT_SYS_SHOW_ADJ_RTC_FORM, (EventHandler_fn)onShowThisFormEvent);

    g_i16RtcAdjustFormIndex = FormManager_AddForm(&g_sRtcAdjustCanvas);
}