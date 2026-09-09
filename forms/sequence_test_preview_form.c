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
#include "event_engine.h"

#include "common_widgets.h"
#include "experiments_cfg.h"

// ========================================================
// VARIABLES DEL FORMULARIO
// ========================================================
gfx_Canvas g_sPreviewCanvas;
int16_t g_i16PreviewFormIndex;

// Contenedores
static gfx_GenericWidget formTitleWidget, fileNameWidget, maxCurrentWidget;
static gfx_GenericWidget totalTimeWidget, previewGraphWidget;
static gfx_GenericWidget startBtnWidget, cancelBtnWidget;

// Widgets
static gfx_Label formTitleData, fileNameData, maxCurrentData, totalTimeData;
static gfx_Button startBtnData, cancelBtnData;
static gfx_Graph previewGraphData;

// Buffers de Textos y Gráfica
static char fileNameBuf[MAX_PROFILE_NAME_LEN + 15];
static char maxCurrentBuf[30];
static char totalTimeBuf[30];

#define PREVIEW_GRAPH_POINTS 400
static float tracePreviewData[PREVIEW_GRAPH_POINTS];

// El perfil actualmente cargado en RAM
static const ul_sequence_profile_t *profile;

// ========================================================
// LÓGICA DE SIMULACIÓN Y GENERACIÓN DE LA GRÁFICA
// ========================================================

// Esta función "simula" el paso del tiempo y genera los 400 puntos para la gráfica
static void generateGraphPoints(const ul_sequence_profile_t *profile) {
    
    // Si la duración es 0 (error de parseo), limpiar gráfica
    if (profile->totalDurationSec == 0) {
        memset(tracePreviewData, 0, sizeof(tracePreviewData));
        previewGraphData.totalPointsAdded = 0;
        return;
    }

    // Iterar para rellenar los 400 pixeles/puntos de la pantalla
	ExperimentCfg_generateSequencePoints(tracePreviewData, PREVIEW_GRAPH_POINTS);

    // Configurar los metadatos de la gráfica
    previewGraphData.data = tracePreviewData;
    previewGraphData.maxPoints = PREVIEW_GRAPH_POINTS;
    previewGraphData.totalPointsAdded = PREVIEW_GRAPH_POINTS; 
    previewGraphData.head = 0; 
    
    // Auto-ajustar el eje Y
    previewGraphData.minY = 0;
    // Agregar un 15% de margen superior para que la gráfica respire
    previewGraphData.maxY = profile->maxCurrentRequested * 1.15f; 
    if (previewGraphData.maxY < 10.0f) previewGraphData.maxY = 10.0f; // Mínimo absoluto
    
    previewGraphData.bIsDirty = true;
}

// ========================================================
// CALLBACKS
// ========================================================

static void onCancelBtnRelease(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    // Volver al explorador de archivos
    Event_Post(EVT_SYS_SHOW_FILE_BROWSER, (EventParam_t){.ptr = NULL});
}

static void onStartBtnRelease(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    // TODO: Enviar comandos al módulo CAN con los datos de profile->
    // Y luego ir a la pantalla de "Test Running"
    //Event_Post(EVT_SYS_START_MODE_C, (EventParam_t){.ptr = NULL});
	//Event_Post(EVT_SYS_SHOW_TEST_CONFIRMATION, (EventParam_t){.ui32 = UL_TEST_SEQUENCE});
	Event_Post(EVT_SYS_SHOW_SEQUENCE_CONFIG_FORM, (EventParam_t){.ptr = NULL});
}

// Evento disparado desde el Explorador de Archivos
static void onShowPreviewForm(EventParam_t arg) {
    // 1. Ejecutar el Parser
	profile = ExperimentCfg_getCurrSequenceCfg();
    if (profile != NULL) {
        
        // 2. Extraer el nombre corto para la UI (Cortar la ruta "1:/Carpeta/TEST.CSV" -> "TEST.CSV")
        char *shortName = strrchr(profile->absoluteFilePath, '/');
        shortName = (shortName != NULL) ? shortName + 1 : (char *)profile->absoluteFilePath;
        
        // 3. Formatear Textos
        snprintf(fileNameBuf, sizeof(fileNameBuf), "ARCHIVO: %s", shortName);
        snprintf(maxCurrentBuf, sizeof(maxCurrentBuf), "I Max: %.1f [A]", profile->maxCurrentRequested);
        
        // Formatear Segundos a MM:SS
        uint32_t m = profile->totalDurationSec / 60;
        uint32_t s = profile->totalDurationSec % 60;
        snprintf(totalTimeBuf, sizeof(totalTimeBuf), "Duracion: %02u:%02u", m, s);

        fileNameData.bIsDirty = true;
        maxCurrentData.bIsDirty = true;
        totalTimeData.bIsDirty = true;

		previewGraphData.maxXValue = (float)profile->totalDurationSec;

        // 4. Generar la gráfica simulada
        generateGraphPoints(profile);

    } else {
        // FALLO AL PARSEAR: Llenar todo con error y ocultar el botón de Start
        strcpy(fileNameBuf, "ERROR AL LEER ARCHIVO");
        strcpy(maxCurrentBuf, "--");
        strcpy(totalTimeBuf, "--");
        
        memset(tracePreviewData, 0, sizeof(tracePreviewData));
        previewGraphData.totalPointsAdded = 0;
        previewGraphData.bIsDirty = true;
        
        // Puedes agregar aquí un popup de error o desactivar el botón START
        // startBtnData.state = BTN_STATE_DISABLED; 
    }
}

// ========================================================
// INICIALIZACIÓN DE LA UI
// ========================================================

void initPreviewProfileForm(void) {
    g_sPreviewCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
    g_sPreviewCanvas.psWidgets = NULL;

    // --- TEXTOS ---
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

    fileNameData = (gfx_Label){
		.name = "f", 
		.text = fileNameBuf, 
		.pos.x = 40,
		.pos.y = 85,
		.typo = TYPO_BODY,
		.style = STYLE_SECONDARY,
		.isVisible = true,
		.alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER),
	};
    fileNameWidget.eWidgetType = WD_TYPE_LABEL; fileNameWidget.pvWidget = &fileNameData;


    // --- GRÁFICA (Single Trace con Dashed Line) ---
    previewGraphData = (gfx_Graph) {
        .pos.x = 40, 
		.pos.y = 130,
        .size.width = LCD_WIDTH - 80, 
		.size.height = 235,
        .bgColor = g_pCurrentTheme->palette.surface,
        .gridColor = g_pCurrentTheme->palette.border,
        .lineColor = g_pCurrentTheme->palette.primary, 
        .textColor = g_pCurrentTheme->palette.textMuted,
        .lineWidth = 2,
        .gridLinesX = 5, .gridLinesY = 4,
        
        // ---> NUEVO: Banderas y Nombres de Ejes <---
        .bShowLabels = true, 
        .bShowXLabels = true, 
        .xAxisName = "t [s]",
        .yAxisName = "I [A]",
        
        .typo = TYPO_CAPTION,
        .isTraceVisible = true,
        .bIsDirty = true,
        
        // CONFIGURACIÓN DE LÍNEA SEGMENTADA
        .bIsDashed = true,
        .dashLen = 6,     // 6 pixeles de línea
        .spaceLen = 4     // 4 pixeles de espacio
    };

    previewGraphWidget.eWidgetType = WD_TYPE_GRAPH; 
    previewGraphWidget.pvWidget = &previewGraphData;

    maxCurrentData = (gfx_Label){
		.name = "c", 
		.text = maxCurrentBuf, 
		.pos.x = previewGraphData.pos.x + previewGraphData.size.width, 
		.pos.y = 90, 
		.typo = TYPO_MONO_BOLD, 
		.style = STYLE_DANGER, 
		.isVisible = true,
		.alignment = (gfx_Align_e)(ALIGN_RIGHT | ALIGN_VCENTER),
	};
    maxCurrentWidget.eWidgetType = WD_TYPE_LABEL; maxCurrentWidget.pvWidget = &maxCurrentData;

    totalTimeData = (gfx_Label){
		.name = "d", 
		.text = totalTimeBuf, 
		.pos.x = previewGraphData.pos.x + previewGraphData.size.width, 
		.pos.y = maxCurrentData.pos.y + 25, 
		.typo = TYPO_MONO_BOLD, 
		.style = STYLE_SUCCESS, 
		.isVisible = true,
		.alignment = (gfx_Align_e)(ALIGN_RIGHT | ALIGN_VCENTER),
	};
    totalTimeWidget.eWidgetType = WD_TYPE_LABEL; totalTimeWidget.pvWidget = &totalTimeData;

    // --- BOTONES ---
    cancelBtnData = (gfx_Button) {
        .label = "REGRESAR", 
		.pos.x = LCD_WIDTH / 2 - LCD_WIDTH / 4.0 - 10,
		.pos.y = LCD_HEIGHT -70,
		.size.width = LCD_WIDTH / 4.0,
		.size.height = 60,
        .style = STYLE_DEFAULT, 
		.bIsVisible = true, 
		.onPressed = onGenericBtnPressed,
		.onRelease = onCancelBtnRelease, 
		.typo = TYPO_MONO_BOLD,
		.radius = 5,
		.borderWidth = 2,
    };
    gfx_initRegTouch((void *)&cancelBtnData, WD_TYPE_BUTTON);
    cancelBtnWidget.eWidgetType = WD_TYPE_BUTTON; cancelBtnWidget.pvWidget = &cancelBtnData;

    startBtnData = (gfx_Button) {
        .label = "SIGUIENTE", 
		.pos.x = LCD_WIDTH / 2 + 10,
		.pos.y = LCD_HEIGHT - 70,
		.size.width = LCD_WIDTH / 4.0,
		.size.height = 60,
        .style = STYLE_SUCCESS, 
		.bIsVisible = true, 
		.onPressed = onGenericBtnPressed,
		.onRelease = onStartBtnRelease, 
		.typo = TYPO_MONO_BOLD,
		.radius = 5,
		.borderWidth = 2,
    };
    gfx_initRegTouch((void *)&startBtnData, WD_TYPE_BUTTON);
    startBtnWidget.eWidgetType = WD_TYPE_BUTTON; startBtnWidget.pvWidget = &startBtnData;

    // ENSAMBLE DEL CANVAS
    useFullHeader(&g_sPreviewCanvas);
    canvasInsertAtTop(&g_sPreviewCanvas.psWidgets, &formTitleWidget);
    canvasInsertAtTop(&g_sPreviewCanvas.psWidgets, &fileNameWidget);
    canvasInsertAtTop(&g_sPreviewCanvas.psWidgets, &maxCurrentWidget);
    canvasInsertAtTop(&g_sPreviewCanvas.psWidgets, &totalTimeWidget);
    canvasInsertAtTop(&g_sPreviewCanvas.psWidgets, &previewGraphWidget);
    canvasInsertAtTop(&g_sPreviewCanvas.psWidgets, &cancelBtnWidget);
    canvasInsertAtTop(&g_sPreviewCanvas.psWidgets, &startBtnWidget);

	Event_Subscribe(EVT_SYS_SHOW_SEQUENCE_PREVIEW_FORM, ( EventHandler_fn )onShowPreviewForm);

    g_i16PreviewFormIndex = FormManager_AddForm(&g_sPreviewCanvas);
}
