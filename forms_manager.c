
#include <stdlib.h>
#include <string.h>

#include "build_config.h"
#include "gpu_ft81x.h"
#include "FT8xx_params.h"
#include "event_engine.h"
#include "gesture_engine.h"
#include "gui_core.h"
#include "render.h"

#include "forms/boot_form.h"
#include "forms/common_widgets.h"
#include "forms/config_form.h"
#include "forms/dashboard_form.h"
#include "forms/test_selection_form.h"
#include "forms/fault_test_config_form.h"
#include "forms/numpad_modify_value_form.h"
#include "forms/test_confirmation_form.h"
#include "forms/test_running_form.h"
#include "forms/test_finished_form.h"
#include "forms/crush_test_config_form.h"
#include "forms/file_browser_form.h"
#include "forms/sequence_test_preview_form.h"
#include "forms/options_selection_form.h"
#include "forms/adjusts/voltage_selection_form.h"
#include "forms/adjusts/current_selection_form.h"
#include "forms/adjusts/adjust_numpad_form.h"
#include "forms/adjusts/temp_selection_form.h"
#include "forms/adjusts/export_cfg_form.h"
#include "forms/adjusts/cfg_browser_form.h"
#include "forms/adjusts/cfg_import_result_form.h"
#include "forms/adjusts/export_cfg_opt_select_form.h"
#include "forms/profile_test_config_form.h"
#include "forms/diagnostics/debug_menu_form.h"
#include "forms/diagnostics/debug_io_form.h"
#include "forms/diagnostics/variac_debug_form.h"
#include "forms/diagnostics/readings_dashboard_form.h"
#include "forms/adjusts/rtc_adjust_form.h"
#include "forms/eeprom_sync_result_form.h"
#include "forms/factory_reset_confirmation_form.h"
#include "forms/voltage_preset_form.h"

#include "forms_manager.h"

static gfx_Canvas *g_psForms[MAX_CANVAS_WIDGETS];

static uint8_t g_ui8FormCounter = 0;
gfx_Canvas *g_psCurrentForm;
uint16_t g_ui16CurrentIndex = 0;

static widget_type_e g_eLockedWidgetType = WD_TYPE_NULL;
static void *g_pLockedWidget = NULL;

static void onShowHomeFormEvent(EventParam_t arg) {
  g_psCurrentForm = g_psForms[g_i16DashboardFormID];
  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowTestSelectionFormEvent(EventParam_t arg) {
	g_psCurrentForm = g_psForms[g_i16FormSelectionID];
	g_bIsBackgroundReady = false;

  	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowFaultConfigFormEvent(EventParam_t arg) {
	g_psCurrentForm = g_psForms[g_i16FaultConfigFormIndex];
	g_bIsBackgroundReady = false;

  	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowConfigFormEvent(EventParam_t arg) {
  g_psCurrentForm = g_psForms[g_i16ConfigFormID];
  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onNumpadModValueFormEvent(EventParam_t arg) {
  g_psCurrentForm = g_psForms[g_i16NumpadModifyValueIndex];
  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowTestConfirmationFormEvent(EventParam_t arg) {
  g_psCurrentForm = g_psForms[g_i16TestConfirmationIndex];
  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowFactoryResetConfirmationFormEvent(EventParam_t arg) {
  g_psCurrentForm = g_psForms[g_i16FactoryResetConfirmationIndex];
  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowTestRunningFormEvent(EventParam_t arg) {
  g_psCurrentForm = g_psForms[g_i16TestRunningIndex];
  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowFinishedTestEvent(EventParam_t arg) {
  g_psCurrentForm = g_psForms[g_i16TestFinishedFormIndex];
  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowCrushTestConfigEvent(EventParam_t arg) {
  g_psCurrentForm = g_psForms[g_i16CrushTestConfigIndex];
  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowFileBrowserFormEvent(EventParam_t arg) {
  g_psCurrentForm = g_psForms[g_i16FileBrowserIndex];
  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowSequencePreviewFormEvent(EventParam_t arg) {
  g_psCurrentForm = g_psForms[g_i16PreviewFormIndex];
  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowOptionsFormEvent(EventParam_t arg) {
  g_psCurrentForm = g_psForms[g_i16OptionsSelectionIndex];
  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowAdjVoltageFormEvent(EventParam_t arg) {
  g_psCurrentForm = g_psForms[g_i16VoltageSelectionIndex];
  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowAdjNumpadForm(EventParam_t arg) {
  g_psCurrentForm = g_psForms[g_i16AdjustNumpadIndex];
  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowAdjCurrentFormEvent(EventParam_t arg) {
  g_psCurrentForm = g_psForms[g_i16CurrentSelectionIndex];
  g_bIsBackgroundReady = false;

  Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowAdjTempFormEvent(EventParam_t arg) {
  	g_psCurrentForm = g_psForms[g_i16TempSelectionIndex];
  	g_bIsBackgroundReady = false;

  	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowAdjExportCfgFormEvent(EventParam_t arg) {
  	g_psCurrentForm = g_psForms[g_i16ExportCfgIndex];
  	g_bIsBackgroundReady = false;

  	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowAdjCfgSelectFormEvent(EventParam_t arg) {
  	g_psCurrentForm = g_psForms[g_i16ExportCfgSelectIndex];
  	g_bIsBackgroundReady = false;

  	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowAdjCfgBrowserFormEvent(EventParam_t arg) {
  	g_psCurrentForm = g_psForms[g_i16CfgBrowserIndex];
  	g_bIsBackgroundReady = false;

  	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowAdjImportResultFormEvent(EventParam_t arg) {
  	g_psCurrentForm = g_psForms[g_i16CfgImportResultFormIndex];
  	g_bIsBackgroundReady = false;

  	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowProfileConfigFormEvent(EventParam_t arg) {
  	g_psCurrentForm = g_psForms[g_i16ProfileConfigFormIndex];
  	g_bIsBackgroundReady = false;

  	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowDebugMenuFormEvent(EventParam_t arg) {
  	g_psCurrentForm = g_psForms[g_i16DebugMenuFormIndex];
  	g_bIsBackgroundReady = false;

  	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowDebugIOFormEvent(EventParam_t arg) {
  	g_psCurrentForm = g_psForms[g_i16DebugIoFormIndex];
  	g_bIsBackgroundReady = false;

  	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowVariacDebugFormEvent(EventParam_t arg) {
  	g_psCurrentForm = g_psForms[g_i16VariacDebugFormIndex];
  	g_bIsBackgroundReady = false;

  	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onReadingDashboardFormEvent(EventParam_t arg) {
  	g_psCurrentForm = g_psForms[g_i16ReadingsDashboardFormID];
  	g_bIsBackgroundReady = false;

  	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowAdjRTCFormEvent(EventParam_t arg) {
  	g_psCurrentForm = g_psForms[g_i16RtcAdjustFormIndex];
  	g_bIsBackgroundReady = false;

  	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowEEPROMSyncResultEvent(EventParam_t arg) {
  	g_psCurrentForm = g_psForms[g_i16EepromSyncIndex];
  	g_bIsBackgroundReady = false;

  	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

static void onShowVoltagePresetFormEvent(EventParam_t arg) {
  	g_psCurrentForm = g_psForms[g_i16VoltagePresetIndex];
  	g_bIsBackgroundReady = false;

  	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

bool FormManager_HandleGesture(TouchStatus touchStatus, gesture_type_e gesture) {
  	static Position newPos;
  	static gfx_Button *btn;
  	static gfx_GenericWidgetNode *temp = NULL;

  	switch (gesture) {
  		case GESTURE_LOCK_OBJ:
  		  	g_pLockedWidget = NULL;
  		  	g_eLockedWidgetType = WD_TYPE_NULL;

  		  	temp = g_psCurrentForm->psWidgets;
  		  	while (temp != NULL) {
  		  	  	if (gfx_isWidgetTouched(&temp->sWidget, touchStatus)) {
  		  	  	  	g_pLockedWidget = temp->sWidget.pvWidget;
  		  	  	  	g_eLockedWidgetType = temp->sWidget.eWidgetType;

  		  	  	  	break;
  		  	  	}

  		  	  	temp = temp->psNext;
  		  	}
  		  	break;

  		case GESTURE_DRAG:
  		  	if (g_pLockedWidget == NULL)
  		  	  	break;

  		  	if (g_eLockedWidgetType == WD_TYPE_BUTTON) {
  		  	  	btn = (gfx_Button *)g_pLockedWidget;
  		  	  	newPos.x = touchStatus.x - btn->size.width / 2;
  		  	  	newPos.y = touchStatus.y - btn->size.height / 2;

  		  	  	if (btn->onPosChanged != NULL) {
  		  	  	  	btn->onPosChanged(btn, newPos);
  		  	  	}
  		  	} else if (g_eLockedWidgetType == WD_TYPE_SLIDER) {
  		  	  	gfx_Slider *sld = (gfx_Slider *)g_pLockedWidget;
  		  	  	gfx_processSliderTouch(sld, touchStatus);
  		  	} else if (g_eLockedWidgetType == WD_TYPE_GRAPH_CURSOR) {
  		  	  	gfx_GraphCursor *cursor = (gfx_GraphCursor *)g_pLockedWidget;
  		  	  	gfx_processCursorTouch(cursor, touchStatus);
  		  	}
  		  	break;
  		case GESTURE_RELEASE:
  		  // Only clicking
  		  	temp = g_psCurrentForm->psWidgets;
  		  	while (temp != NULL) {
  		  	  	if (gfx_isWidgetTouched(&temp->sWidget, touchStatus)) {
  		  	  	  	g_pLockedWidget = temp->sWidget.pvWidget;
  		  	  	  	g_eLockedWidgetType = temp->sWidget.eWidgetType;

  		  	  	  	break;
  		  	  	}

  		  	  	temp = temp->psNext;
  		  	}

  		  	// Call release callback if locked widget has one
  		  	if (g_pLockedWidget != NULL) {
  		  	  	if (g_eLockedWidgetType == WD_TYPE_BUTTON) {
  		  	  	  	btn = (gfx_Button *)g_pLockedWidget;
  		  	  	  	if (btn->onRelease != NULL) {
  		  	  	  	  	btn->onRelease(btn);
  		  	  	  	}
  		  	  	} else if (g_eLockedWidgetType == WD_TYPE_SLIDER) {
  		  	  	  	gfx_Slider *sld = (gfx_Slider *)g_pLockedWidget;
  		  	  	  	gfx_processSliderTouch(sld, touchStatus);
  		  	  	} else if (g_eLockedWidgetType == WD_TYPE_GRAPH_OVERLAY) {
  		  	  	  	gfx_GraphOverlay *over = (gfx_GraphOverlay *)g_pLockedWidget;
  		  	  	  	if (over != NULL) {
						int trace = gfx_processOverlayTouch(over, touchStatus);
						if(trace != -1)
  		  	  	  	  		over->onTraceToggle(over, trace);
  		  	  	  	}
  		  	  	} else if(g_eLockedWidgetType == WD_TYPE_TOUCH_AREA) {
					gfx_TouchArea *area = (gfx_TouchArea *)g_pLockedWidget;
					if(area->onAreaTouchRelease != NULL) {
						area->onAreaTouchRelease(area);
					}
				} else if(g_eLockedWidgetType == WD_TYPE_LISTVIEW) {
					gfx_ListView *list = (gfx_ListView *)g_pLockedWidget;
					gfx_processListViewTouch(list, touchStatus);
				}
  		  	}

  		  	// Clear locked widget
  		  	g_pLockedWidget = NULL;
  		  	g_eLockedWidgetType = WD_TYPE_NULL;
  		  	break;

  		case GESTURE_PRESSED:
  		  	temp = g_psCurrentForm->psWidgets;
  		  	while (temp != NULL) {
  		  	  	if (gfx_isWidgetTouched(&temp->sWidget, touchStatus)) {
  		  	  	  	if (temp->sWidget.eWidgetType == WD_TYPE_BUTTON) {
  		  	  	  	  	gfx_Button *btn = ((gfx_Button *)temp->sWidget.pvWidget);
  		  	  	  	  	if (btn->onPressed != NULL)
  		  	  	  	  	  btn->onPressed(btn);
  		  	  	  	} else if (temp->sWidget.eWidgetType == WD_TYPE_SLIDER) {
  		  	  	  	  	gfx_Slider *sld = (gfx_Slider *)temp->sWidget.pvWidget;
  		  	  	  	  	gfx_processSliderTouch(sld, touchStatus);
  		  	  	  	}

  		  	  	  	break;
  		  	  	}
  		  	  	temp = temp->psNext;
  		  	}
  		  	break;
  		default:
  		  break;
  		}

  	return true;
}




// En form_manager.c
bool FormManager_CheckSoftwareDirty(void) {
  bool bNeedsUpdate = false;
  gfx_GenericWidgetNode *iter = g_psCurrentForm->psWidgets;

  while (iter != NULL) {
    gfx_DirtyRect bbox = {0};

    // Delegamos la lógica al tipo de widget correspondiente
    switch (iter->sWidget.eWidgetType) {
    case WD_TYPE_BUTTON:
      if (((gfx_Button *)iter->sWidget.pvWidget)->bIsDirty) {
        bbox = gfx_ButtonProcessState((gfx_Button *)iter->sWidget.pvWidget);
        gfx_compositePartialFrame(g_psCurrentForm, g_pDrawingBuffer, bbox.x,
                                  bbox.y, bbox.w, bbox.h);
      }
      break;
    case WD_TYPE_LABEL: {
      if (((gfx_Label *)iter->sWidget.pvWidget)->bIsDirty) {
        bbox = gfx_LabelProcessState((gfx_Label *)iter->sWidget.pvWidget);
        gfx_compositePartialFrame(g_psCurrentForm, g_pDrawingBuffer, bbox.x,
                                  bbox.y, bbox.w, bbox.h);
      }
      break;
    }
    case WD_TYPE_SLIDER: {
      if (((gfx_Slider *)iter->sWidget.pvWidget)->bIsDirty) {
        bbox = gfx_SliderProcessState((gfx_Slider *)iter->sWidget.pvWidget);
        gfx_compositePartialFrame(g_psCurrentForm, g_pDrawingBuffer, bbox.x,
                                  bbox.y, bbox.w, bbox.h);
      }
      break;
    }
    case WD_TYPE_GRAPH_OVERLAY: {
      if (((gfx_GraphOverlay *)iter->sWidget.pvWidget)->bIsDirty) {
        bbox = gfx_GraphOverlayProcessState(
            (gfx_GraphOverlay *)iter->sWidget.pvWidget);
        gfx_compositePartialFrame(g_psCurrentForm, g_pDrawingBuffer, bbox.x,
                                  bbox.y, bbox.w, bbox.h);
      }
      break;
    }
    case WD_TYPE_LISTVIEW: {
      if (((gfx_ListView *)iter->sWidget.pvWidget)->bIsDirty) {
        bbox = gfx_ListViewProcessState(
            (gfx_ListView *)iter->sWidget.pvWidget);
        gfx_compositePartialFrame(g_psCurrentForm, g_pDrawingBuffer, bbox.x,
                                  bbox.y, bbox.w, bbox.h);
      }
      break;
    }
      // ... otros widgets
    }

    // Si el widget cambió, creamos los trabajos DMA solo para su área
    if (bbox.isDirty) {
      bNeedsUpdate = true;

      // Generar trabajos Scatter-Gather solo para el Bounding Box
      int row = 0;
      for (; row < bbox.h; row++) {
        DMARenderJob_t job;

        // Calcular el offset en SDRAM y RAM_G
        uint32_t pixelOffset = ((bbox.y + row) * LCD_WIDTH) + bbox.x;

        // Puntero de origen en SDRAM (g_pDrawingBuffer + offset)
        job.pSrcSDRAM = (uint8_t *)(g_pDrawingBuffer + pixelOffset);

        // Dirección destino en EVE G_RAM (0 + offset * 2 bytes por pixel
        // RGB565)
        job.destRAMG = (pixelOffset * 2);

        // Longitud a transferir (ancho del widget * 2)
        job.length = bbox.w * 2;

        // Meter el trabajo a la cola de tu ISR
		Render_PushDMAjob(job);
      }

      return bNeedsUpdate;
    }
    iter = iter->psNext;
  }

  return bNeedsUpdate;
}

bool FormManager_CheckHardwareDirty(void) {
  bool bNeedsUpdate = false;
  gfx_GenericWidgetNode *iter = g_psCurrentForm->psWidgets;

  // Iteramos por todos los widgets de la pantalla actual
  while (iter != NULL) {

    // Evaluamos SOLO los widgets que se renderizan mediante EVE (Hardware)
    switch (iter->sWidget.eWidgetType) {

    case WD_TYPE_MULTIGRAPH: {
      gfx_MultiGraph *mgraph = (gfx_MultiGraph *)iter->sWidget.pvWidget;
      // Verificamos si alguna de sus banderas de actualización está activa
      if (mgraph->bIsDirty || mgraph->bEVEDirty) {
        bNeedsUpdate = true;
      }
      break;
    }

    case WD_TYPE_GRAPH: {
      gfx_Graph *graph = (gfx_Graph *)iter->sWidget.pvWidget;
      if (graph->bIsDirty || graph->bEVEDirty) {
        bNeedsUpdate = true;
      }
      break;
    }

    case WD_TYPE_IMAGE: {
      gfx_Image *img = (gfx_Image *)iter->sWidget.pvWidget;
      // Las imágenes decodificadas por hardware usan este flag si
      // cambian de posición, escala, o si se ocultan/muestran.
      if (img->bIsDirty) {
        bNeedsUpdate = true;
      }
      break;
    }
	case WD_TYPE_DUAL_GRAPH: {
		//gfx_DualGraph *graph = (gfx_DualGraph *)iter->sWidget.pvWidget;
		bNeedsUpdate = true;
		break;
	}
      // case WD_TYPE_GRAPH_CURSOR: {
      // 	gfx_GraphCursor *cursor = (gfx_GraphCursor
      // *)iter->sWidget.pvWidget;

      // 	bNeedsUpdate = true;
      // 	break;
      // }

      // NOTA: Si en el futuro decides que los Sliders se rendericen
      // 100% por EVE (usando CMD_SLIDER) en lugar de software,
      // agregarías su case aquí.
    }

    // Si ya encontramos al menos un componente sucio, podríamos hacer un
    // 'break' para ahorrar ciclos de CPU (Fast Return), pero si necesitas que
    // todos ejecuten alguna pre-lógica, lo dejamos iterar. Para máximo
    // rendimiento:
    if (bNeedsUpdate) {
      break;
    }

    iter = iter->psNext;
  }

  return bNeedsUpdate;
}

void FormManager_RenderEVEComponents(void) {
  // if(!g_bIsBackgroundReady) return;

  gfx_GenericWidgetNode *iter = g_psCurrentForm->psWidgets;
  // Lets check if the canvas contains a widget with EVE elements

  iter = g_psCurrentForm->psWidgets;
  // 1. Iniciamos una nueva Display List
  while (iter != NULL) {
    if (iter->sWidget.eWidgetType == WD_TYPE_GRAPH) {
      gfx_Graph *gf = (gfx_Graph *)iter->sWidget.pvWidget;
      gfx_GraphRenderEVEComponents((gfx_Graph *)iter->sWidget.pvWidget);
      gf->bEVEDirty = false;

      API_LIB_EndCoProList();
      API_LIB_AwaitCoProEmpty();
      API_LIB_BeginCoProList();
    } else if (iter->sWidget.eWidgetType == WD_TYPE_MULTIGRAPH) {
      gfx_MultiGraph *gf = (gfx_MultiGraph *)iter->sWidget.pvWidget;
      // if(gf->bEVEDirty)
      {
        gfx_MultigraphRenderEVEComponents(gf);
        gf->bEVEDirty = false;

        API_LIB_EndCoProList();
        API_LIB_AwaitCoProEmpty();
        API_LIB_BeginCoProList();
      }
    } else if(iter->sWidget.eWidgetType == WD_TYPE_DUAL_GRAPH) {
		gfx_DualGraph *dual = (gfx_DualGraph *)iter->sWidget.pvWidget;
			gfx_DualGraphRenderEVEComponents(dual);

        	API_LIB_EndCoProList();
        	API_LIB_AwaitCoProEmpty();
        	API_LIB_BeginCoProList();
	} else if (iter->sWidget.eWidgetType == WD_TYPE_IMAGE) {
      gfx_Image *img = (gfx_Image *)iter->sWidget.pvWidget;
      gfx_ImageRenderEVEComponents(img);

      // API_LIB_EndCoProList();
      // API_LIB_AwaitCoProEmpty();
      // API_LIB_BeginCoProList();
    } else if (iter->sWidget.eWidgetType == WD_TYPE_GRAPH_CURSOR) {
      gfx_GraphCursor *cursor = (gfx_GraphCursor *)iter->sWidget.pvWidget;
      if (cursor->isVisible)
        gfx_GraphCursorRenderEVE(cursor);
      // API_LIB_EndCoProList();
      // API_LIB_AwaitCoProEmpty();
      // API_LIB_BeginCoProList();
    } 
    iter = iter->psNext;
  }
}

void FormManager_CompositeFrame(pixel16_t *psPixelBuffer) {
  gfx_compositeFrame(g_psCurrentForm, psPixelBuffer);
}

int16_t FormManager_AddForm(gfx_Canvas *form) {
  if (g_ui8FormCounter < MAX_CANVAS_WIDGETS) {
    g_psForms[g_ui8FormCounter] = form;

    g_ui8FormCounter++;
    return g_ui8FormCounter - 1;
  }

  return -1;
}

void FormManager_Init(void) {
  initCommonWidgets();

  initBootForm();
  initDashboardForm();
  initConfigForm();
  initTestSelectionForm();
  initFaultTestConfigForm();
  initNumpadModifyValueForm();
  initTestConfirmationForm();
  initTestRunningForm();
  initTestFinishedForm();
  initCrushTestConfigForm();
  initFileBrowserForm();
  initPreviewProfileForm();
  initOptionsSelectionForm();
  initVoltageSelectionIndex();
  initAdjustNumpadForm();
  initCurrentSelectionForm();
  initTempSelectionForm();
  initExportCgfForm();
  initExportCfgOptSelectForm();
  initCfgBrowserForm();
  initCfgImportResultForm();
  initProfileTestConfigForm();
  initDebugMenuForm();
  initDebugIoForm();
  initReadingsDashboardForm();
  initVariacDebugForm();
  initRtcAdjustForm();
  initEepromSyncForm();
  initFactoryResetConfirmationForm();
  initVoltagePresetForm();

  // Form changing events
  Event_Subscribe(EVT_SYS_SHOW_HOME_FORM, (EventHandler_fn)onShowHomeFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_CONFIG_FORM,
                  (EventHandler_fn)onShowConfigFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_TEST_SELECTION_FORM, (EventHandler_fn)onShowTestSelectionFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_FAULT_CONFIG_FORM, (EventHandler_fn)onShowFaultConfigFormEvent);
  Event_Subscribe(EVT_SYS_NUMPAD_MOD_FAULT_CFG_CURRENT, (EventHandler_fn)onNumpadModValueFormEvent);
  Event_Subscribe(EVT_SYS_NUMPAD_MOD_FAULT_CFG_DURATION, (EventHandler_fn)onNumpadModValueFormEvent);
  Event_Subscribe(EVT_SYS_NUMPAD_MOD_FAULT_CFG_PRESET_VOLTAGE, (EventHandler_fn)onNumpadModValueFormEvent);
  Event_Subscribe(EVT_SYS_NUMPAD_MOD_FAULT_CFG_CALIBER, (EventHandler_fn)onNumpadModValueFormEvent);
  Event_Subscribe(EVT_SYS_NUMPAD_MOD_CRUSH_CFG_DURATION, (EventHandler_fn)onNumpadModValueFormEvent);
  Event_Subscribe(EVT_SYS_NUMPAD_MOD_CRUSH_CFG_TEMP, (EventHandler_fn)onNumpadModValueFormEvent);
  Event_Subscribe(EVT_SYS_NUMPAD_MOD_CRUSH_CFG_CALIBER, (EventHandler_fn)onNumpadModValueFormEvent);
  Event_Subscribe(EVT_SYS_NUMPAD_MOD_PROFILE_CFG_CALIBER, (EventHandler_fn)onNumpadModValueFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_TEST_CONFIRMATION, (EventHandler_fn)onShowTestConfirmationFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_FACTORY_RESET_CONFIRMATION, (EventHandler_fn)onShowFactoryResetConfirmationFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_TEST_RUNNING, (EventHandler_fn)onShowTestRunningFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_FINISHED_TEST, (EventHandler_fn)onShowFinishedTestEvent);
  Event_Subscribe(EVT_SYS_SHOW_CRUSH_CONFIG_FORM, (EventHandler_fn)onShowCrushTestConfigEvent);
  Event_Subscribe(EVT_SYS_SHOW_FILE_BROWSER, (EventHandler_fn)onShowFileBrowserFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_SEQUENCE_PREVIEW_FORM, (EventHandler_fn)onShowSequencePreviewFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_OPTIONS_FORM, ( EventHandler_fn )onShowOptionsFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_ADJ_SELECT_VOLTAGE, (EventHandler_fn)onShowAdjVoltageFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_ADJ_SELECT_CURRENT, (EventHandler_fn)onShowAdjCurrentFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_ADJ_SELECT_TEMP, (EventHandler_fn)onShowAdjTempFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_ADJ_NUMPAD_FORM, (EventHandler_fn)onShowAdjNumpadForm);
  Event_Subscribe(EVT_SYS_SHOW_ADJ_EXPORT_CFG, (EventHandler_fn)onShowAdjExportCfgFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_ADJ_CFG_SELECT_FORM, (EventHandler_fn)onShowAdjCfgSelectFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_ADJ_CFG_BROWSER, ( EventHandler_fn )onShowAdjCfgBrowserFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_ADJ_CFG_IMPORT_RESULT, (EventHandler_fn)onShowAdjImportResultFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_SEQUENCE_CONFIG_FORM, (EventHandler_fn)onShowProfileConfigFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_DEBUG_MENU, ( EventHandler_fn )onShowDebugMenuFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_DEBUG_IO_FORM, ( EventHandler_fn )onShowDebugIOFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_VARIAC_DEBUG_FORM, (EventHandler_fn)onShowVariacDebugFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_READINGS_DASHBOARD_FORM, (EventHandler_fn)onReadingDashboardFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_ADJ_RTC_FORM, (EventHandler_fn)onShowAdjRTCFormEvent);
  Event_Subscribe(EVT_SYS_SHOW_EEPROM_SYNC_RESULT_FORM, (EventHandler_fn)onShowEEPROMSyncResultEvent);
  #ifdef ENABLE_VOLTAGE_PRESET_FORM
  Event_Subscribe(EVT_SYS_SHOW_VOLTAGE_PRESET_FORM, (EventHandler_fn)onShowVoltagePresetFormEvent);
  #endif

  
  g_psCurrentForm = g_psForms[g_i16BootFormID];
  // g_psCurrentForm = g_psForms[g_i16GraphFormID];
}


