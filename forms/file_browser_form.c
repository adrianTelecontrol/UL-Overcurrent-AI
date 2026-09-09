#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <fatfs/src/ff.h>

#include "FT8xx_params.h"
#include "gui_core.h"
#include "gui_canvas.h"
#include "event_engine.h"
#include "file_manager.h"
#include "forms_manager.h"
#include "icon_map.h"
#include "experiments_cfg.h"

#include "common_widgets.h"
// ========================================================
// VARIABLES DE ESTADO Y NAVEGACIÓN
// ========================================================
#define MAX_LV_ITEMS 100 // Capacidad máxima del explorador en memoria
int16_t g_i16FileBrowserIndex = 0;

static char currentPath[64] = "1:/"; // Ruta base (Raíz del USB)
static gfx_ListViewItem listItemsMemory[MAX_LV_ITEMS];

static gfx_Canvas g_sFileBrowserCanvas;
static gfx_GenericWidget listViewWidget, btnUpWidget, btnDownWidget, formTitleWidget, msgLabelWidget, subtitleLabelWidget;
static gfx_GenericWidget cancelBtnWidget, btnErrorReturnWidget;

static gfx_Label formTitleData, msgLabelData, subtitleLabelData;
static gfx_ListView fileListData;
static gfx_Button btnUpData, btnDownData, cancelBtnData, btnErrorReturnData;

// ========================================================
// LÓGICA DE ESCANEO DEL DIRECTORIO (FatFs)
// ========================================================
static void populateListView(void) {
    DIR dj;
    FILINFO fno;
    FRESULT res;
    
    char lfnBuffer[100]; 
    
    fileListData.itemCount = 0;
    fileListData.scrollOffset = 0; 

    // 1. Agregar opción de "Volver Atrás"
    if (strcmp(currentPath, "1:/") != 0 && strcmp(currentPath, "1:") != 0) {
        strcpy(listItemsMemory[fileListData.itemCount].text, "..");
        listItemsMemory[fileListData.itemCount].type = LV_ITEM_BACK;
        fileListData.itemCount++;
    }

    // 2. Abrir el directorio actual
    res = f_opendir(&dj, currentPath);
    if (res == FR_OK) {
        for (;;) {
            memset(lfnBuffer, 0, sizeof(lfnBuffer));
            fno.lfname = lfnBuffer;
            fno.lfsize = sizeof(lfnBuffer);

            res = f_readdir(&dj, &fno);
            if (res != FR_OK || fno.fname[0] == 0) break;
            
            if (fno.fattrib & (AM_HID | AM_SYS)) continue;
            if (fileListData.itemCount >= MAX_LV_ITEMS) break;

            char *fileNameToUse = (fno.lfname[0] != '\0') ? fno.lfname : fno.fname;

            // 3A. Si es CARPETA
            if (fno.fattrib & AM_DIR) {
                if (strcmp(fileNameToUse, ".") == 0 || strcmp(fileNameToUse, "..") == 0) continue;

                strncpy(listItemsMemory[fileListData.itemCount].text, fileNameToUse, 31);
                listItemsMemory[fileListData.itemCount].text[31] = '\0'; 
                listItemsMemory[fileListData.itemCount].type = LV_ITEM_FOLDER;
                fileListData.itemCount++;
            } 
            // 3B. Si es ARCHIVO
            else {
                char *ext = strrchr(fileNameToUse, '.');
                if (ext != NULL && (strcmp(ext, ".CSV") == 0 || strcmp(ext, ".csv") == 0)) {
                    strncpy(listItemsMemory[fileListData.itemCount].text, fileNameToUse, 31);
                    listItemsMemory[fileListData.itemCount].text[31] = '\0'; 
                    listItemsMemory[fileListData.itemCount].type = LV_ITEM_FILE;
                    fileListData.itemCount++;
                }
            }
        }
    }

    // ==========================================================
    // 4. MOTOR DE ORDENAMIENTO (ALFABÉTICO A-Z)
    // ==========================================================
    if (fileListData.itemCount > 1) {
        // Si el índice 0 es "..", empezamos a ordenar desde el índice 1 para no mover el "Atrás"
        int startIndex = (listItemsMemory[0].type == LV_ITEM_BACK) ? 1 : 0;

		int i = startIndex;
        for (; i < fileListData.itemCount - 1; i++) {
			int j = i + 1;
            for (; j < fileListData.itemCount; j++) {
                
                bool shouldSwap = false;

                // Regla A: Las carpetas siempre van arriba de los archivos
                if (listItemsMemory[i].type == LV_ITEM_FILE && listItemsMemory[j].type == LV_ITEM_FOLDER) {
                    shouldSwap = true;
                }
                // Regla B: Si son del mismo tipo, se ordenan alfabéticamente de la A a la Z
                else if (listItemsMemory[i].type == listItemsMemory[j].type) {
                    // strcmp evalúa la cadena. Si retorna mayor a 0, significa que 
                    // text[i] es alfabéticamente posterior a text[j], por lo que debemos intercambiarlos.
                    if (strcmp(listItemsMemory[i].text, listItemsMemory[j].text) > 0) {
                        shouldSwap = true;
                    }
                }

                // Intercambiar en la memoria
                if (shouldSwap) {
                    gfx_ListViewItem tempItem = listItemsMemory[i];
                    listItemsMemory[i] = listItemsMemory[j];
                    listItemsMemory[j] = tempItem;
                }
            }
        }
    }

    fileListData.bIsDirty = true;
}

// ========================================================
// CALLBACKS
// ========================================================
static void onProfileParserErrorEvent(void) {
	msgLabelData.text = "Profile parser error";
	msgLabelData.isVisible = true;
	msgLabelData.bIsDirty = true;
	fileListData.bIsVisible = false;
	fileListData.bIsDirty = true;
	btnDownData.bIsVisible = false;
	btnDownData.bIsDirty = true;
	btnUpData.bIsVisible = false;
	btnUpData.bIsDirty = true;
	cancelBtnData.bIsVisible = false;
	cancelBtnData.bIsDirty = true;
	btnErrorReturnData.bIsVisible = true;
	btnErrorReturnData.bIsDirty= true;
}

// Función ejecutada cuando se toca un elemento en la ListView
// NOTA: Ahora recibe el dataIndex directamente calculado por gui_core
static void onListItemSelected(gfx_ListView *lv, uint16_t dataIndex) {
    
    // Validación de seguridad (gui_core ya lo verifica, pero es buena práctica)
    if (dataIndex >= lv->itemCount) return; 

    gfx_ListViewItem *selectedItem = &lv->items[dataIndex];

    if (selectedItem->type == LV_ITEM_FOLDER) {
        // NAVEGAR DENTRO DE LA CARPETA: Agregar "/NombreCarpeta" a currentPath
        // Si no estamos en la raíz pura, agregamos el slash
        if (currentPath[strlen(currentPath)-1] != '/') strcat(currentPath, "/");
        strcat(currentPath, selectedItem->text);
        
        populateListView(); // Recargar datos
    } 
    else if (selectedItem->type == LV_ITEM_BACK) {
        // NAVEGAR ARRIBA: Buscar el último '/' y cortar la cadena ahí
        char *lastSlash = strrchr(currentPath, '/');
        if (lastSlash != NULL) {
            *lastSlash = '\0'; // Corta el string
            // Si nos quedamos con "1:", lo restauramos a "1:/"
            if (strcmp(currentPath, "1:") == 0) strcpy(currentPath, "1:/");
        }
        
        populateListView(); // Recargar datos
    }
    else if (selectedItem->type == LV_ITEM_FILE) {
        // ES UN ARCHIVO: Formatear la ruta absoluta para pasársela al Preview
        static char absoluteFilePath[100];
        if (currentPath[strlen(currentPath)-1] == '/') {
            sprintf(absoluteFilePath, "%s%s", currentPath, selectedItem->text);
        } else {
            sprintf(absoluteFilePath, "%s/%s", currentPath, selectedItem->text);
        }
        
        // Lanzar el evento pasando la ruta absoluta
        // (Descomentado para que funcione con tu motor de eventos)

		bool ret = ExperimentCfg_newSequenceTest(absoluteFilePath);
		if(ret) 
        	Event_Post(EVT_SYS_SHOW_SEQUENCE_PREVIEW_FORM, (EventParam_t){.ptr = NULL});
		else
		 	onProfileParserErrorEvent();
    }
}

// Controles de Paginación (Arriba / Abajo)
static void onScrollUp(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    if (fileListData.scrollOffset > 0) {
        fileListData.scrollOffset--;
        fileListData.bIsDirty = true;
    }
}

static void onScrollDown(gfx_Button *btn) {
    onGenericBtnRelease(btn);
    if (fileListData.scrollOffset + fileListData.visibleItems < fileListData.itemCount) {
        fileListData.scrollOffset++;
        fileListData.bIsDirty = true;
    }
}

static void onShowFileBrowserForm(EventParam_t arg) {
    // Iniciar siempre en la raíz del USB al abrir el menú
    strcpy(currentPath, "1:/");
    populateListView();
}

static void onUSBConnectedEvent(EventParam_t arg) {
	populateListView();
	fileListData.bIsVisible = true;
	fileListData.bIsDirty = true;
	msgLabelData.isVisible = false;
	msgLabelData.bIsDirty = false;
	btnDownData.bIsVisible = true;
	btnDownData.bIsDirty = true;
	btnUpData.bIsVisible = true;
	btnUpData.bIsDirty = true;
	cancelBtnData.bIsVisible = true;
	cancelBtnData.bIsDirty = true;
	btnErrorReturnData.bIsVisible = false;
	btnErrorReturnData.bIsDirty= false;
}

static void onUSBDisconnectedEvent(EventParam_t arg) {
	fileListData.bIsVisible = false;
	fileListData.bIsDirty = true;
	msgLabelData.isVisible = true;
	msgLabelData.bIsDirty = true;
	btnDownData.bIsVisible = false;
	btnDownData.bIsDirty = true;
	btnUpData.bIsVisible = false;
	btnUpData.bIsDirty = true;
	cancelBtnData.bIsVisible = false;
	cancelBtnData.bIsDirty = true;
	btnErrorReturnData.bIsVisible = false;
	btnErrorReturnData.bIsDirty= false;
}

static void onCancelReleasedEvent(gfx_Button *btn) {
	onGenericBtnRelease(btn);
	
	Event_Post(EVT_SYS_SHOW_TEST_SELECTION_FORM, (EventParam_t){.ptr = NULL});	
}

static void onErrorReturnReleased(gfx_Button *btn) {
	onGenericBtnRelease(btn);
	
	onUSBConnectedEvent((EventParam_t){.ptr = NULL});
	// Event_Post(EVT_SYS_SHOW_TEST_SELECTION_FORM, (EventParam_t){.ptr = NULL});	
}

// ========================================================
// INICIALIZACIÓN DE LA UI
// ========================================================

void initFileBrowserForm(void) {
    g_sFileBrowserCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
    g_sFileBrowserCanvas.psWidgets = NULL; 

    // --- TÍTULO ---
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

	msgLabelData = (gfx_Label) {
		.name = "noUSBLabel",
		.text = "NO USB CONNECTED ",
		.alignment = ALIGN_CENTER,
		.pos.x = LCD_WIDTH / 2,
		.pos.y = LCD_HEIGHT / 2,
		.style = STYLE_DANGER,
		.typo = TYPO_H1,
		.isVisible = true,
	};
	msgLabelWidget.eWidgetType = WD_TYPE_LABEL;
	msgLabelWidget.pvWidget = (void *)&msgLabelData;

	subtitleLabelData = (gfx_Label) {
		.name = "subtitleLabel",
		.text = "Explorador USB: Seleccionar Perfil",
		.pos.x = 20,
		.pos.y = 100,
		.typo = TYPO_H3,
		.style = STYLE_SECONDARY,
		.isVisible = true,
		.alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER),
	};
	subtitleLabelWidget.eWidgetType = WD_TYPE_LABEL;
	subtitleLabelWidget.pvWidget = (void *)&subtitleLabelData;

    // --- LIST VIEW WIDGET ---
    fileListData = (gfx_ListView) {
        .name = "usbList",
        .pos.x = 20,
        .pos.y = 135,
        .size.width = 600,
        .size.height = 270,
        .bIsVisible = false,
        .bIsDirty = true,
        
        .items = listItemsMemory,
        .maxItems = MAX_LV_ITEMS,
        .itemCount = 0,
        .scrollOffset = 0,
        
        .itemHeight = 55, // 6 filas visibles (300 / 50 = 6)
        .visibleItems = 5,
        
        .bgColor = g_pCurrentTheme->palette.surface,
        .lineColor = g_pCurrentTheme->palette.border,
        .textColor = g_pCurrentTheme->palette.textMain,
        .typo = TYPO_BODY,
        
		._lastTouchedIndex = -1,
        .onItemSelect = onListItemSelected // <--- Callback crucial
    };
    listViewWidget.eWidgetType = WD_TYPE_LISTVIEW; 
    listViewWidget.pvWidget = (void *)&fileListData;
    gfx_initRegTouch((void *)&fileListData, WD_TYPE_LISTVIEW);

    // --- BOTONES DE SCROLL (A la derecha de la lista) ---
	uint16_t btnHeight = 50;
    btnUpData = (gfx_Button) {
        .label = ICON_UP, 
		.pos.x = 640, 
		.pos.y = fileListData.pos.y + fileListData.size.height / 2.0 - btnHeight - 5, 
		.size.width = 70, 
		.size.height = btnHeight,
        .style = STYLE_SECONDARY, 
		.bIsVisible = false, 
		.onPressed = onGenericBtnPressed,
		.onRelease = onScrollUp, 
		.typo = TYPO_ICON,
		.radius = 5,
		.borderWidth = 2
    };
    gfx_initRegTouch((void *)&btnUpData, WD_TYPE_BUTTON);
    btnUpWidget.eWidgetType = WD_TYPE_BUTTON; btnUpWidget.pvWidget = (void *)&btnUpData;

    btnDownData = (gfx_Button) {
        .label = ICON_DOWN, 
		.pos.x = 640, 
		.pos.y = fileListData.pos.y + fileListData.size.height / 2.0 + 5, 
		.size.width = 70, 
		.size.height = btnHeight,
        .style = STYLE_SECONDARY, 
		.bIsVisible = false, 
		.onPressed = onGenericBtnPressed,
		.onRelease = onScrollDown, 
		.typo = TYPO_ICON,
		.radius = 5,
		.borderWidth = 2
    };
    gfx_initRegTouch((void *)&btnDownData, WD_TYPE_BUTTON);
    btnDownWidget.eWidgetType = WD_TYPE_BUTTON; btnDownWidget.pvWidget = (void *)&btnDownData;

    cancelBtnData = (gfx_Button) {
        .label = "CANCELAR", 
		.pos.x = 640, 
		.pos.y = fileListData.pos.y + fileListData.size.height - btnHeight * 1.2, 
		.size.width = 120, 
		.size.height = btnHeight * 1.2,
        .style = STYLE_DEFAULT, 
		.bIsVisible = false, 
		.onPressed = onGenericBtnPressed,
		.onRelease = onCancelReleasedEvent, 
		.typo = TYPO_BODY,
		.radius = 5,
		.borderWidth = 2
    };
    gfx_initRegTouch((void *)&cancelBtnData, WD_TYPE_BUTTON);
    cancelBtnWidget.eWidgetType = WD_TYPE_BUTTON; cancelBtnWidget.pvWidget = (void *)&cancelBtnData;

    btnErrorReturnData = (gfx_Button) {
        .label = "REGRESAR", 
		.pos.x = LCD_WIDTH / 2.0f - 150 / 2.0f, 
		.pos.y = fileListData.pos.y + fileListData.size.height - btnHeight * 1.5 - 20, 
		.size.width = 150, 
		.size.height = 75,
        .style = STYLE_DEFAULT, 
		.bIsVisible = false, 
		.onPressed = onGenericBtnPressed,
		.onRelease = onErrorReturnReleased, 
		.typo = TYPO_BODY,
		.radius = 5,
		.borderWidth = 2
    };
    gfx_initRegTouch((void *)&btnErrorReturnData, WD_TYPE_BUTTON);
    btnErrorReturnWidget.eWidgetType = WD_TYPE_BUTTON; btnErrorReturnWidget.pvWidget = (void *)&btnErrorReturnData;

    // Insertar en Canvas
    useFullHeader(&g_sFileBrowserCanvas);
	useNavigationButtons(&g_sFileBrowserCanvas);
    canvasInsertAtTop(&g_sFileBrowserCanvas.psWidgets, &btnErrorReturnWidget);
    canvasInsertAtTop(&g_sFileBrowserCanvas.psWidgets, &formTitleWidget);
    canvasInsertAtTop(&g_sFileBrowserCanvas.psWidgets, &subtitleLabelWidget);
	canvasInsertAtTop(&g_sFileBrowserCanvas.psWidgets, &msgLabelWidget);
    canvasInsertAtTop(&g_sFileBrowserCanvas.psWidgets, &listViewWidget);
    canvasInsertAtTop(&g_sFileBrowserCanvas.psWidgets, &btnUpWidget);
    canvasInsertAtTop(&g_sFileBrowserCanvas.psWidgets, &btnDownWidget);
    canvasInsertAtTop(&g_sFileBrowserCanvas.psWidgets, &cancelBtnWidget);

    Event_Subscribe(EVT_SYS_SHOW_FILE_BROWSER, (EventHandler_fn)onShowFileBrowserForm);
	Event_Subscribe(EVT_SYS_USB_DISCONNECTED, (EventHandler_fn)onUSBDisconnectedEvent);
	Event_Subscribe(EVT_SYS_USB_CONNECTED, (EventHandler_fn)onUSBConnectedEvent);
    g_i16FileBrowserIndex = FormManager_AddForm(&g_sFileBrowserCanvas);
}