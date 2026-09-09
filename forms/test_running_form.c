
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "gui_core.h"
#include "gui_theme.h"
#include "gui_canvas.h"
#include "forms_manager.h"
#include "font_engine.h"
#include "FT8xx_params.h"
#include "gui_colors.h"
#include "icon_map.h"
#include "event_engine.h"
#include "experiments_cfg.h"
#include "helpers.h"

#include "common_widgets.h"

#include "test_running_form.h"

gfx_Canvas g_sTestRunningCanvas;
int16_t g_i16TestRunningIndex;

// Container
static gfx_GenericWidget formTitleWidget;
static gfx_GenericWidget testStatusWidget;
static gfx_GenericWidget statusIconWidget;
static gfx_GenericWidget dualGraphWidget;
static gfx_GenericWidget progressBarWidget;
static gfx_GenericWidget graphOverlayWidget;
static gfx_GenericWidget timeRemainingWidget;
static gfx_GenericWidget progressLabelWidget;
static gfx_GenericWidget percentageLabelWidget;
static gfx_GenericWidget infoFrameWidget;
static gfx_GenericWidget infoLabelWidget;
static gfx_GenericWidget sensorsLabelWidget;
static gfx_GenericWidget variableSPWidget;
static gfx_GenericWidget temp2ValueWidget;
static gfx_GenericWidget temp3ValueWidget;
static gfx_GenericWidget temp4ValueWidget;
static gfx_GenericWidget voltageValueWidget;
static gfx_GenericWidget durationValueWidget;
static gfx_GenericWidget stabilityValueWidget;
static gfx_GenericWidget continueButtonWidget;
static gfx_GenericWidget emergencyStopWidget;

// Widgets
static gfx_Label formTitleData;
static gfx_Label testStatusData;
static gfx_Label statusIconData;
static gfx_DualGraph dualGraphData;
static gfx_Slider progressBarData;
static gfx_GraphOverlay graphOverlayData;
static gfx_Label timeRemainingData;
static gfx_Label progressLabelData;
static gfx_Label percentageLabelData;
static gfx_Rectangle infoFrameData;
static gfx_Label infoLabelData;
static gfx_Label variableSPData;
static gfx_Label temp2ValueData;
static gfx_Label temp3ValueData;
static gfx_Label temp4ValueData;
static gfx_Label voltageValueData;
static gfx_Label durationValueData;
static gfx_Label stabilityValueData;
static gfx_Label sensorsLabelData;
static gfx_Button continueButtonData, emergencyStopData;

// Buffers
static char testStatusBuf[50] = "ESTADO: INICIANDO PRUEBA";
static char timeRemainigBuf[25] = "T. RESTANTE: 00:00";
static char percentageBuf[5] = "0%";
static char variableParamBuf[20] = "I SP: 0.0 [A]";
static char temp2Buff[20] = "I: 0.0 [A]";
static char temp3Buff[20] = "T1: 0.0 [C]";
static char temp4Buff[20] = "T2: 0.0 [C]";
static char voltageBuff[20] = "Vo: 0.0 V";
static char durationValBuf[20] = "T SP: 00:00";
static char stabilityTimeValBuf[30] = "T ST: 00:00";
static uint32_t g_ui32LastUIUpdate = 0;
static uint32_t g_ui32CurrentTestType;
#define GRAPH_MAX_POINTS 200
static float traceCurrentData[GRAPH_MAX_POINTS];
static float traceTempData[GRAPH_MAX_POINTS];
static float traceProfileData[GRAPH_MAX_POINTS];

// Callbacks
static void onNewCurrentValue(EventParam_t arg) {
    // Índice 0 - Corriente 
	if(arg.ptr == NULL) return;

	float *val = arg.ptr;
	if(g_ui32CurrentTestType == UL_TEST_CRUSH) {
		gfx_DualGraphAddData(&dualGraphData, 0, val[0]);
	} else {
		gfx_DualGraphAddStaticData(&dualGraphData, 0, val[0], val[1], val[2]);
	}

	if(GetExecTimeMs() % 500 < 80) {
		sprintf(graphOverlayData.traces[0].valueText, "I: %.2f [A]", val[0]);
		graphOverlayData.bIsDirty = true;

		sprintf(temp2Buff, "I: %.2f [A]", val[0]);
		temp2ValueData.bIsDirty = true;
	}
}

// Cuando llega un nuevo valor de Temperatura por CAN
static void onNewTempValue(EventParam_t arg) {
    // Índice 1 = Temperatura
	if(arg.ptr == NULL) return;

	float *val = arg.ptr;
	if(g_ui32CurrentTestType == UL_TEST_CRUSH) {
		gfx_DualGraphAddData(&dualGraphData, 1, val[0]);
	} else {
		gfx_DualGraphAddStaticData(&dualGraphData, 1, val[0], val[1], val[2]);
	}

	if(GetExecTimeMs() % 500 < 80) {
		sprintf(graphOverlayData.traces[1].valueText, "T1: %.2f [C]", val[0]);
		graphOverlayData.bIsDirty = true;

		sprintf(temp3Buff, "T1: %.2f [C]", val[0]);
		temp3ValueData.bIsDirty = true;
	}
}

static void onContinueButtonRelease(gfx_Button *btn) {
	onGenericBtnRelease(btn);
	Event_Post(EVT_SYS_SHOW_FINISHED_TEST, (EventParam_t){.ptr = NULL});
}

static void onTestStartedOk(EventParam_t arg) {
	// g_bTestStartedOk = true;
	strcpy(testStatusBuf, "ESTADO: PRUEBA INICIADA CORRECTAMENTE");
	testStatusData.bIsDirty = true;
	continueButtonData.bIsVisible = false;
	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onTestFinishedOk(EventParam_t arg) {
	//g_bTestStartedOk = false;
	strcpy(testStatusBuf, "ESTADO: PRUEBA FINALIZADA CORRECTAMENTE");
	progressBarData.currentValue = 100;
	strcpy(percentageBuf, "100%");
	strcpy(timeRemainigBuf, "T. RESTANTE: 00:00");
	testStatusData.bIsDirty = true;
	continueButtonData.bIsVisible = true;
	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onTestFailed(EventParam_t arg) {
	strcpy(testStatusBuf, "ESTADO: ERROR DURANTE EJECUCION DE PRUEBA");
	testStatusData.bIsDirty = true;
	continueButtonData.bIsVisible = true;
	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}


static void onProgressBarValueChanged(EventParam_t arg) {
	progressBarData.currentValue = arg.ui32;
	progressBarData.bIsDirty = true;
	snprintf(percentageBuf, sizeof(percentageBuf), "%u%%", arg.ui32);
}

static void onRemainingTimeChanged(EventParam_t arg) {
	snprintf(timeRemainigBuf, sizeof(timeRemainigBuf), "T. RESTANTE: %s", arg.str);
	
	timeRemainingData.bIsDirty = true;
	if(GetExecTimeMs() - g_ui32LastUIUpdate > 2000) {
		Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
		g_ui32LastUIUpdate = GetExecTimeMs();
	}

}

static void onStabilityTimeValueChanged(EventParam_t arg) {
	snprintf(stabilityTimeValBuf, sizeof(stabilityTimeValBuf), "T ST: %s", arg.str);
	stabilityValueData.bIsDirty = true;
}

// static void onEmergencyStopBtnRelease(EventParam_t arg) {
// 	strcpy(testStatusBuf, "ERROR: INSTRUMENTACION DEJO DE RESPONDER");
// 	testStatusData.bIsDirty = true;
// 	continueButtonData.bIsVisible = true;
// 	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
// }

static void onTempProbeSecondaryChanged(EventParam_t arg) {
	snprintf(temp4Buff, sizeof(temp4Buff), "T2: %.1f [C]", arg.f32);
	temp4ValueData.bIsDirty = true;
}

static void onVoltageSecondaryChanged(EventParam_t arg) {
	snprintf(voltageBuff, sizeof(voltageBuff), "Vo: %.2f [V]", arg.f32);
	voltageValueData.bIsDirty = true;
}

static void onEmergencyStopBtnRelease(gfx_Button *btn) {
	onGenericBtnRelease(btn);
	Event_Post(EVT_SYS_TEST_EMERGENCY_STOPPED, ( EventParam_t ){.ptr = NULL});
	
	Event_Post(EVT_SYS_SHOW_FINISHED_TEST, (EventParam_t){.ptr = NULL});
	
}

static void onShowThisForm(EventParam_t arg) {
	g_ui32CurrentTestType = arg.ui32;
	if(arg.ui32 == UL_TEST_CRUSH) {
		ul_crush_test_s crush = ExperimentCfg_getCurrCrushCfg();
		snprintf(variableParamBuf, sizeof(variableParamBuf), "Temp SP: %.2f [C]", crush.f32TargetTemp);
		snprintf(durationValBuf, sizeof(durationValBuf), "Time SP: %u [s]", crush.ui16Duration);
		snprintf(dualGraphData.xAxisName, 16, "Muestras");
		snprintf(dualGraphData.yAxisNameRight, 16, "T [C]");
		dualGraphData.maxXValue = 200;
		dualGraphData.maxY[1] = crush.f32TargetTemp * 1.15f;
		dualGraphData.maxY[0] = 650.0f;
		dualGraphData.totalPointsAdded[0] = 0;
		dualGraphData.heads[0] = 0;
		dualGraphData.totalPointsAdded[1] = 0;
		dualGraphData.heads[1] = 0;
		dualGraphData.bgProfileData[0] = NULL;
		dualGraphData.bIsDirty = true;
		variableSPData.bIsDirty = true;
		dualGraphData.bShowBgProfile[0] = false;
		durationValueData.bIsDirty = true;
	} else if(arg.ui32 == UL_TEST_FAULT) {
		ul_fault_current_test_s fault = ExperimentCfg_getCurrFaultCfg();
		snprintf(variableParamBuf, sizeof(variableParamBuf), "I SP: %.2f [A]", fault.f32TargetCurrent);
		snprintf(durationValBuf, sizeof(durationValBuf), "Time SP: %u [s]", fault.ui16Duration);
		snprintf(dualGraphData.xAxisName, 16, "t [s]");
		snprintf(dualGraphData.yAxisNameRight, 16, "T [C]");
		dualGraphData.maxXValue = fault.ui16Duration;
		dualGraphData.maxY[0] = fault.f32TargetCurrent * 1.15f;
		dualGraphData.totalPointsAdded[0] = 0;
		dualGraphData.heads[0] = 0;
		dualGraphData.totalPointsAdded[1] = 0;
		dualGraphData.heads[1] = 0;
		dualGraphData.bgProfileData[0] = NULL;
		dualGraphData.bShowBgProfile[0] = false;
		dualGraphData.bIsDirty = true;
		variableSPData.bIsDirty = true;
	} else if(arg.ui32 == UL_TEST_SEQUENCE) {
		ul_fault_current_test_s fault = ExperimentCfg_getCurrFaultCfg();
		const ul_sequence_profile_t *profile = ExperimentCfg_getCurrSequenceCfg();
		snprintf(variableParamBuf, sizeof(variableParamBuf), "I MAX: %.2f [A]", profile->maxCurrentRequested);
		snprintf(durationValBuf, sizeof(durationValBuf), "Time SP: %u [s]", profile->totalDurationSec);
		snprintf(dualGraphData.xAxisName, 16, "t [s]");
		snprintf(dualGraphData.yAxisNameRight, 16, "T [C]");
		ExperimentCfg_generateSequencePoints(traceProfileData, GRAPH_MAX_POINTS);
		dualGraphData.bgProfileData[0] = traceProfileData;
		dualGraphData.maxY[0] = profile->maxCurrentRequested;
		dualGraphData.maxXValue = profile->totalDurationSec;
		dualGraphData.totalPointsAdded[0] = 0;
		dualGraphData.heads[0] = 0;
		dualGraphData.totalPointsAdded[1] = 0;
		dualGraphData.heads[1] = 0;
		dualGraphData.bIsDirty = true;
		variableSPData.bIsDirty = true;
		dualGraphData.bShowBgProfile[0] = true;
		durationValueData.bIsDirty = true;
	} else {
		strcpy(variableParamBuf, "Param ERROR!");
		strcpy(durationValBuf, "Param ERROR!");
		variableSPData.bIsDirty = true;
		durationValueData.bIsDirty = true;
	}
}

void initTestRunningForm(void) {
	g_sTestRunningCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
	
	formTitleData = (gfx_Label) {
		.name = "formTitleData",
		.text = "PRUEBAS",
        .pos.x = 125,
        .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H3,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
	};
	formTitleWidget.eWidgetType = WD_TYPE_LABEL;
	formTitleWidget.pvWidget = (void *)&formTitleData;

	testStatusData = (gfx_Label) {
		.name = "testStatus",
		.text = testStatusBuf,
        .pos.x = 25,
        .pos.y = 80,
        .alignment = (gfx_Align_e)( ALIGN_LEFT | ALIGN_VCENTER ),
        .typo = TYPO_MONO_BOLD,           
        .style = STYLE_DANGER,
        .isVisible = true,
	};
	testStatusWidget.eWidgetType = WD_TYPE_LABEL;
	testStatusWidget.pvWidget = (void *)&testStatusData;

	statusIconData = (gfx_Label) {
		.name = "statusIcon",
		.text = ICON_PROCESS,
        .pos.x = 5,
        .pos.y = 90,
        .alignment = (gfx_Align_e)( ALIGN_LEFT | ALIGN_VCENTER ),
        .typo = TYPO_ICON,           
        .style = STYLE_PRIMARY,
        .isVisible = true,
	};
	statusIconWidget.eWidgetType = WD_TYPE_LABEL;
	statusIconWidget.pvWidget = (void *)&statusIconData;

	timeRemainingData = (gfx_Label) {
		.name = "timeRemainig",
		.text = timeRemainigBuf,
        .pos.x = LCD_WIDTH - 20,
        .pos.y = 80,
        .alignment = (gfx_Align_e)( ALIGN_RIGHT| ALIGN_VCENTER ),
        .typo = TYPO_MONO_BOLD,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
	};
	timeRemainingWidget.eWidgetType = WD_TYPE_LABEL;
	timeRemainingWidget.pvWidget = (void *)&timeRemainingData;
	
	dualGraphData = (gfx_DualGraph){
        .pos.x = 230,
        .pos.y = 120,
        .size.width = LCD_WIDTH - 250,  // Margen de 20px a cada lado
        .size.height = 280,
    
        // Diseño y Colores
        .bgColor = g_pCurrentTheme->palette.surface,
        .gridColor = g_pCurrentTheme->palette.border,
        .lineWidth = 2, // EVE usa subpixeles (x16 internamente)
        .typo = TYPO_CAPTION,
        .bShowLabels = true,
        .bIsVisible = true,
    
        // --- ETIQUETAS DE LOS EJES (NUEVO) ---
        .textColor = g_pCurrentTheme->palette.textMuted,
        .bShowXLabels = true,
        .xAxisName = "t [s]",
        .yAxisNameLeft = "I [A]",
        .yAxisNameRight = "MUESTRAS",
        // .maxXValue = 100.0f, -> ¡Ojo! Este lo debes sobreescribir al cargar el perfil.
    
        .gridLinesX = 5,
        .gridLinesY = 4,
    
        .maxPoints = GRAPH_MAX_POINTS,
        .bIsDirty = true,
        .bEVEDirty = true,

        // ========================================================
        // TRAZA ESTÁTICA DE FONDO: PERFIL ESPERADO
        // ========================================================
        .bgProfileData = {traceProfileData, NULL},
        .bgProfileColor = {g_pCurrentTheme->palette.secondary, 0},
        .bShowBgProfile = {true, false},
        .bBgIsDashed = {true, false},
        .bgDashLen = {6, 0},
        .bgSpaceLen = {6, 0},

        // ========================================================
        // EJE 0 (IZQUIERDA) : CORRIENTE REAL (Dinámica)
        // ========================================================
        .dataSets[0] = traceCurrentData,
        .lineColors[0] = g_pCurrentTheme->palette.success, 
        .minY[0] = 0,
        .maxY[0] = 500, // Escala: 0 a 500 Amperios (Se auto-ajustará después)

        // ========================================================
        // EJE 1 (DERECHA) : TEMPERATURA REAL (Dinámica)
        // ========================================================
        .dataSets[1] = traceTempData,
        .lineColors[1] = g_pCurrentTheme->palette.danger,  
        .minY[1] = 0,
        .maxY[1] = 150, // Escala: 0 a 150 Grados Celsius
    };
    
    // Envolver en el Generic Widget
    dualGraphWidget.eWidgetType = WD_TYPE_DUAL_GRAPH;
    dualGraphWidget.pvWidget = (void *)&dualGraphData;

	uint16_t strWidth, strHeight;
	FontEngine_GetStringDimensions("TC1: 62.49 [C]", Theme_ResolveFontId(TYPO_MONO), &strWidth, &strHeight, 1);
	
	uint16_t overlayWidth = strWidth * 1.3f;
	uint16_t overlayHeight = strHeight * 1.6f;
	graphOverlayData = (gfx_GraphOverlay){
	    .name = "graphOvl",
	    .pos.x = dualGraphData.pos.x + dualGraphData.size.width - overlayWidth - 45, 
	    .pos.y = dualGraphData.pos.y + 10,        
	    .size.width = overlayWidth - 10,
	    .size.height = overlayHeight * 1.8, 
	    .bgColor = g_pCurrentTheme->palette.background, 
	    .textColor = g_pCurrentTheme->palette.textMain, // Make it pop more than textMuted
	    .typo = TYPO_CAPTION,
	    // --- DATA ---
	    .numTraces = 2,
	    .traces = {
	        [0] = {
	            // Must strictly match the graph's lineColor to make visual sense
	            .color = g_pCurrentTheme->palette.success, 
	            .valueText = "--", // Default state before first event
	            .isVisible = true
	        }, 
	        [1] = {
	            // Must strictly match the graph's lineColor to make visual sense
	            .color = g_pCurrentTheme->palette.danger, 
	            .valueText = "--", // Default state before first event
	            .isVisible = true
	        }
	    },
	    .bIsDirty = true,
	};

	graphOverlayWidget.eWidgetType = WD_TYPE_GRAPH_OVERLAY;
	graphOverlayWidget.pvWidget = (void *)&graphOverlayData;

	infoFrameData = (gfx_Rectangle){
		.pos.x = 10,
		.pos.y = dualGraphData.pos.y,
		.dim.height = dualGraphData.size.height,
		.dim.width = LCD_WIDTH - dualGraphData.size.width - 10 * 4,
		.color = g_pCurrentTheme->palette.surface,
		.round = 5,
		.borderWidth = 2,
	};
	infoFrameWidget.eWidgetType = WD_TYPE_RECT;
	infoFrameWidget.pvWidget = (void *)&infoFrameData;
	
	infoLabelData = (gfx_Label) {
		.name = "infoLabel",
		.text = "PARAMS",
        .pos.x = infoFrameData.pos.x + infoFrameData.dim.width / 2,
        .pos.y = infoFrameData.pos.y + 20,
        .alignment = ALIGN_CENTER,
        .typo = TYPO_BODY,           
        .style = STYLE_SECONDARY,
        .isVisible = true,
	};
	infoLabelWidget.eWidgetType = WD_TYPE_LABEL;
	infoLabelWidget.pvWidget = (void *)&infoLabelData;

	variableSPData = (gfx_Label) {
		.name = "currentSP",
		.text = variableParamBuf,
        .pos.x = infoFrameData.pos.x + 10,
        .pos.y = infoLabelData.pos.y + 30,
        .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER),
        .typo = TYPO_MONO_BOLD,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
	};
	variableSPWidget.eWidgetType = WD_TYPE_LABEL;
	variableSPWidget.pvWidget = (void *)&variableSPData;

	durationValueData = (gfx_Label) {
		.name = "tempValue",
		.text = durationValBuf,
        .pos.x = infoFrameData.pos.x + 10,
        .pos.y = variableSPData.pos.y + 30,
        .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER),
        .typo = TYPO_MONO_BOLD,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
	};
	durationValueWidget.eWidgetType = WD_TYPE_LABEL;
	durationValueWidget.pvWidget = (void *)&durationValueData;

	sensorsLabelData = (gfx_Label) {
		.name = "sensorsData",
		.text = "SENSORES",
        .pos.x = infoFrameData.pos.x + infoFrameData.dim.width / 2,
        .pos.y = durationValueData.pos.y + 30,
        .alignment = (gfx_Align_e)(ALIGN_CENTER),
        .typo = TYPO_BODY,           
        .style = STYLE_SECONDARY,
        .isVisible = true,
	};
	sensorsLabelWidget.eWidgetType = WD_TYPE_LABEL;
	sensorsLabelWidget.pvWidget = (void *)&sensorsLabelData;

	temp2ValueData = (gfx_Label) {
		.name = "WHYYY7",
		.text = temp2Buff,
        .pos.x = infoFrameData.pos.x + 10,
        .pos.y = sensorsLabelData.pos.y + 30,
        .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER),
        .typo = TYPO_MONO,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
	};
	temp2ValueWidget.eWidgetType = WD_TYPE_LABEL;
	temp2ValueWidget.pvWidget = (void *)&temp2ValueData;

	temp3ValueData = (gfx_Label) {
		.name = "tempValue",
		.text = temp3Buff,
        .pos.x = infoFrameData.pos.x + 10,
        .pos.y = temp2ValueData.pos.y + 30,
        .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER),
        .typo = TYPO_MONO,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
	};
	temp3ValueWidget.eWidgetType = WD_TYPE_LABEL;
	temp3ValueWidget.pvWidget = (void *)&temp3ValueData;

	temp4ValueData = (gfx_Label) {
		.name = "temp4Value",
		.text = temp4Buff,
        .pos.x = infoFrameData.pos.x + 10,
        .pos.y = temp3ValueData.pos.y + 30,
        .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER),
        .typo = TYPO_MONO,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
	};
	temp4ValueWidget.eWidgetType = WD_TYPE_LABEL;
	temp4ValueWidget.pvWidget = (void *)&temp4ValueData;
	
	voltageValueData = (gfx_Label) {
		.name = "voltValue",
		.text = voltageBuff,
        .pos.x = infoFrameData.pos.x + 10,
        .pos.y = temp4ValueData.pos.y + 30,
        .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER),
        .typo = TYPO_MONO,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
	};
	voltageValueWidget.eWidgetType = WD_TYPE_LABEL;
	voltageValueWidget.pvWidget = (void *)&voltageValueData;

	stabilityValueData = (gfx_Label) {
		.name = "tempValue",
		.text = stabilityTimeValBuf,
        .pos.x = infoFrameData.pos.x + 10,
        .pos.y = voltageValueData.pos.y + 30,
        .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER),
        .typo = TYPO_MONO,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
	};
	stabilityValueWidget.eWidgetType = WD_TYPE_LABEL;
	stabilityValueWidget.pvWidget = (void *)&stabilityValueData;

    progressBarData = (gfx_Slider){
        .name = "progBar",
        .knobRadius = 0,
        .maxValue = 100,
        .minValue = 0,
        // .size.width = LCD_WIDTH - 60,
		.size.width = dualGraphData.size.width,
        .size.height = 30,
        .pos.x = dualGraphData.pos.x,
        .pos.y = LCD_HEIGHT - 35,
        .trackHeight = 30,
        .style = STYLE_SECONDARY,  // Red progress bar
        .onValueChanged = NULL, 
        .bIsVertical = false,
        .bShowKnob = false,     // Flat progress bar look
        .currentValue = 0,
    };
    progressBarWidget.eWidgetType = WD_TYPE_SLIDER;
    progressBarWidget.pvWidget = (void *)&progressBarData;

	progressLabelData = (gfx_Label) {
		.name = "progressLabel",
		.text = "PROGRESO:",
        .pos.x = progressBarData.pos.x,
        .pos.y = progressBarData.pos.y - 15,
        .alignment = (gfx_Align_e)( ALIGN_LEFT | ALIGN_VCENTER ),
        .typo = TYPO_MONO_BOLD,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
	};
	progressLabelWidget.eWidgetType = WD_TYPE_LABEL;
	progressLabelWidget.pvWidget = (void *)&progressLabelData;

	percentageLabelData = (gfx_Label) {
		.name = "percentageLabel",
		.text = percentageBuf,
        .pos.x = progressBarData.pos.x + progressBarData.size.width / 2.0f,
        .pos.y = progressBarData.pos.y + progressBarData.size.height / 2.0f,
        .alignment = (gfx_Align_e)(ALIGN_CENTER),
        .typo = TYPO_MONO_BOLD,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
	};
	percentageLabelWidget.eWidgetType = WD_TYPE_LABEL;
	percentageLabelWidget.pvWidget = (void *)&percentageLabelData;

	continueButtonData = (gfx_Button) {
		.name = "continueButton",
		.label = "CONTINUAR",
		.size.width = infoFrameData.dim.width,
		.size.height = 55,
		.pos.x = infoFrameData.pos.x, 
		.pos.y = progressBarData.pos.y - 25,
		.borderWidth = 2,
		.radius = 4,
		.state = BTN_STATE_NORMAL,
		.typo = TYPO_H3,
		.style = STYLE_SUCCESS,
		.bIsVisible = false,
		.onPressed = onGenericBtnPressed,
		.onRelease = onContinueButtonRelease,
	};
	gfx_initRegTouch((void *)&continueButtonData, WD_TYPE_BUTTON);
	continueButtonWidget.eWidgetType = WD_TYPE_BUTTON;
	continueButtonWidget.pvWidget = (void *)&continueButtonData;

	emergencyStopData = (gfx_Button){
		.label = "EMERGENCY STOP",
		.size.height = 55,
		.size.width = 245,
		.pos.x = LCD_WIDTH / 2.0 + 130 + 20,
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

	
	//useHeaderNoClock(&g_sTestRunningCanvas);
	useFullHeaderNoEmergencyBtn(&g_sTestRunningCanvas);

	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &formTitleWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &timeRemainingWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &testStatusWidget);
	//canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &statusIconWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &dualGraphWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &progressBarWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &progressLabelWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &percentageLabelWidget);
	//canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &graphOverlayWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &infoFrameWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &infoLabelWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &variableSPWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &temp2ValueWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &temp3ValueWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &temp4ValueWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &voltageValueWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &durationValueWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &sensorsLabelWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &stabilityValueWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &continueButtonWidget);
	canvasInsertAtTop(&g_sTestRunningCanvas.psWidgets, &emergencyStopWidget);


    // 4. Suscripción a Eventos (Enlazando con hal_inst_can.c y tu Motor de Eventos)
    //Event_Subscribe(EVT_CAN_INST_CURRENT_PRIMARY, (EventHandler_fn)onNewCurrentValue);
    // Event_Subscribe(EVT_CAN_INST_TEMP_PROBE_MAIN, (EventHandler_fn)onNewTempValue);
    Event_Subscribe(EVT_UI_GRAPH_CURRENT_VALUE, (EventHandler_fn)onNewCurrentValue);
    Event_Subscribe(EVT_UI_GRAPH_TEMP_VALUE, (EventHandler_fn)onNewTempValue);
	Event_Subscribe(EVT_SYS_SHOW_TEST_RUNNING, (EventHandler_fn)onShowThisForm);
	Event_Subscribe(EVT_CAN_INST_TEST_START_OK, ( EventHandler_fn )onTestStartedOk);
	Event_Subscribe(EVT_SYS_TEST_FINISHED_OK, (EventHandler_fn)onTestFinishedOk);
	Event_Subscribe(EVT_SYS_TEST_FINISHED_FAIL, ( EventHandler_fn )onTestFailed);
	Event_Subscribe(EVT_UI_TEST_UPDATE_PROGRESS, (EventHandler_fn)onProgressBarValueChanged);
	Event_Subscribe(EVT_UI_TEST_UPDATE_TIME, (EventHandler_fn)onRemainingTimeChanged);
	Event_Subscribe(EVT_UI_TEST_UPDATE_STABILITY_TIME, ( EventHandler_fn )onStabilityTimeValueChanged);
    //Event_Subscribe(EVT_SYS_TEST_EMERGENCY_STOPPED, (EventHandler_fn)onEmergencyStopBtnRelease);
	Event_Subscribe(EVT_CAN_INST_TEMP_PROBE_SECONDARY, (EventHandler_fn)onTempProbeSecondaryChanged);
	Event_Subscribe(EVT_CAN_INST_VOLTAGE_SECONDARY, ( EventHandler_fn )onVoltageSecondaryChanged);
	
	g_i16TestRunningIndex = FormManager_AddForm(&g_sTestRunningCanvas);
}

