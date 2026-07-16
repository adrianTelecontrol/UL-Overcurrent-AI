#ifndef GFX_H
#define GFX_H

#include <stdbool.h>
#include <stdint.h>

#include "FT8xx.h"
#include "font_engine.h"
#include "render.h"
#include "gui_theme.h"

#define MAX_CANVAS_WIDGETS 	50
#define MAX_GRAPH_DATA_SETS 5

typedef enum {
	STYLE_DEFAULT = 0,
	STYLE_PRIMARY,
	STYLE_SECONDARY,
	STYLE_TEXT_MAIN,
	STYLE_TEXT_MUTED,
	STYLE_TEXT_MAIN_BOLD,
	STYLE_DANGER,
	STYLE_SUCCESS
} gfx_WidgetStyle_e;

typedef enum {
  	WD_TYPE_NULL = 0,
  	WD_TYPE_RECT,
  	WD_TYPE_BUTTON,
  	WD_TYPE_LABEL,
  	WD_TYPE_SLIDER,
  	WD_TYPE_GRAPH,
  	WD_TYPE_MULTIGRAPH,
  	WD_TYPE_IMAGE,
  	WD_TYPE_GRAPH_OVERLAY,
  	WD_TYPE_GRAPH_CURSOR,
  	WD_TYPE_OVERLAY_TRACE,
	WD_TYPE_TOUCH_AREA,
	WD_TYPE_NUMPAD,
	WD_TYPE_DUAL_GRAPH,
	WD_TYPE_LISTVIEW,
} widget_type_e;

typedef struct RegionTouchObject {
  uint16_t x1;
  uint16_t y1;
  uint16_t x2;
  uint16_t y2;
} RegionTouchObject;

typedef struct {
  uint16_t x;
  uint16_t y;
  uint8_t state;
} TouchStatus;

typedef struct {
  uint16_t width;
  uint16_t height;
} Size;

typedef struct Position {
  int16_t x;
  int16_t y;
} Position;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    bool isDirty;
} gfx_DirtyRect;

typedef struct Point {
  Position pos;
  uint16_t ratio;
  uint32_t color;
} Point;

typedef struct Line {
  Position posInitial;
  Position posFinal;
  uint16_t size;
  uint32_t color;
} Line;

typedef struct Rectangle {
  Position pos;
  Size dim;
  uint16_t round;
  uint32_t color;
  const char *name;
  uint16_t borderWidth;

  bool bIsDirty;
} gfx_Rectangle;


typedef enum {
  BTN_STATE_NORMAL = 0,
  BTN_STATE_PRESSED,
  BTN_STATE_DISABLED
} ButtonState_e;

typedef struct Button {
    Size size;
    Position pos;
    Position oldPos;
    RegionTouchObject regTouch;
    
    uint16_t radius;
    uint8_t borderWidth;
    
    gfx_WidgetStyle_e style;  // Define el COLOR global
    gfx_TypoStyle_e typo;     // Define la FUENTE global
    ButtonState_e state;
    
    char *label;
    char *name;
    bool bIsDirty;
	bool bIsVisible;

    void (*onPressed)(struct Button *);
    void (*onRelease)(struct Button *);
    void (*onPosChanged)(struct Button *, Position newPos);
} gfx_Button;

typedef struct {
    Position pos;
	Position oldPos;
	Size oldSize;
    char *text;
	char *name;
    
    // The Magic Hooks
    gfx_WidgetStyle_e style;  // Resolves to g_pCurrentTheme->palette
    gfx_TypoStyle_e typo;     // Resolves to g_pCurrentTheme->fonts
    gfx_Align_e alignment;
    
	bool isVisible;
    bool bIsDirty;
} gfx_Label;

typedef struct Slider{
	Size size;
	Position pos;
	RegionTouchObject regTouch;
	
	int16_t minValue;
	int16_t maxValue;
	int16_t currentValue;

	uint16_t knobRadius;
	uint8_t trackHeight;
	
	gfx_WidgetStyle_e style;

	char *name;
	bool bIsVertical;
	bool bIsDirty;
	bool bShowKnob;

	void (*onValueChanged)(struct Slider *, int16_t newValue);
} gfx_Slider;

typedef struct Graph {
    Size size;
    Position pos;

    float *data;         
    uint16_t maxPoints;    
    uint16_t head;         

    int16_t minY;          
    int16_t maxY; 

	char xAxisName[16];    // Ej: "Tiempo [s]"
    char yAxisName[16];    // Ej: "Corriente [A]"
    
    bool bShowXLabels;     // Habilitar numeración en X
    float maxXValue;       // Valor máximo del eje X (ej. totalDurationSec)
    uint16_t textColor;    // Color para el texto de los ejes

    uint8_t gridLinesX;    
    uint8_t gridLinesY;    

    // --- NUEVAS PROPIEDADES DE PERSONALIZACIÓN ---
    uint16_t bgColor;      // Color de fondo de la gráfica
    uint16_t gridColor;    // Color de la cuadrícula
    uint16_t lineColor;    // Color de la señal
    uint8_t lineWidth;     // Grosor de la línea en píxeles

	gfx_TypoStyle_e typo;
	bool bShowLabels;
    
    char *name;
    bool bIsDirty;
	bool bEVEDirty;
	bool isTraceVisible;
	bool bIsDashed;
	uint8_t dashLen;
	uint8_t spaceLen;
	uint32_t totalPointsAdded;
} gfx_Graph;

typedef struct {
    Size size;
    Position pos;

    // Array of pointers for data, and array of colors for each trace
    float *dataSets[MAX_GRAPH_DATA_SETS];
    uint16_t lineColors[MAX_GRAPH_DATA_SETS]; 
    
    uint8_t activeTraces;  // Replaced 'setSize' for clarity
    uint16_t maxPoints;    
	uint16_t heads[MAX_GRAPH_DATA_SETS];

    int16_t minY;          
    int16_t maxY;          

    uint8_t gridLinesX;    
    uint8_t gridLinesY;    

    uint16_t bgColor;      
    uint16_t gridColor;    
    uint8_t lineWidth;     // Global line width for all traces

    gfx_TypoStyle_e typo;
    uint16_t textColor;
    bool bShowLabels;
    
    char *name;
    bool bIsDirty;       
	bool bEVEDirty;
	uint32_t totalPointsAdded[MAX_GRAPH_DATA_SETS];
} gfx_MultiGraph;

#define DUAL_GRAPH_TRACES 2

typedef struct {
    Size size;
    Position pos;

    // Arrays para ambas trazas (0 = Izquierda/Corriente, 1 = Derecha/Temperatura)
    float *dataSets[DUAL_GRAPH_TRACES];
    uint16_t lineColors[DUAL_GRAPH_TRACES]; 
    
    uint16_t maxPoints;    
    uint16_t heads[DUAL_GRAPH_TRACES];

    // Escalas independientes para cada eje
    int16_t minY[DUAL_GRAPH_TRACES];          
    int16_t maxY[DUAL_GRAPH_TRACES];          

    uint8_t gridLinesX;    
    uint8_t gridLinesY;    

    uint16_t bgColor;      
    uint16_t gridColor;    
    uint8_t lineWidth;     

    gfx_TypoStyle_e typo;
    bool bShowLabels;
    
    // --- NUEVO: ETIQUETAS DE EJES ---
    char xAxisName[16];          // Ej: "t [s]"
    char yAxisNameLeft[16];      // Ej: "I [A]"
    char yAxisNameRight[16];     // Ej: "T [C]"
    bool bShowXLabels;
    float maxXValue;
    uint16_t textColor;

    // --- NUEVO: TRAZA ESTÁTICA DE FONDO (PERFIL) ---
    float *bgProfileData;        // Puntero a los 400 puntos pre-calculados
    uint16_t bgProfileColor;     // Color (ej. gris claro o azul punteado)
    bool bShowBgProfile;         // Activar o desactivar
    bool bBgIsDashed;            // Convertirla en punteada
    uint8_t bgDashLen;
    uint8_t bgSpaceLen;
    // -----------------------------------------------

    char *name;
    bool bIsDirty;       
    bool bEVEDirty;
    bool bIsVisible;
    uint32_t totalPointsAdded[DUAL_GRAPH_TRACES];
} gfx_DualGraph;

#define MAX_OVERLAY_TRACES 4

typedef struct {
    uint16_t color;          // El color de la línea de la gráfica que representa
    char valueText[34];      // El valor formateado (ej. "12.5 V")
	bool enableTrace;
    bool isVisible;          // Por si quieres apagar temporalmente una lectura
} gfx_TraceOverlayData;

typedef struct GraphOverlay{
    Position pos;
    Size size;               // Tamaño fijo o calculado del recuadro
	RegionTouchObject regTouch;
    
    gfx_TraceOverlayData traces[MAX_OVERLAY_TRACES];
    uint8_t numTraces;       // Cuántas trazas están activas (1 para Graph, >1 para MultiGraph)

    uint16_t bgColor;        // Color del recuadro flotante
    uint16_t textColor;
    gfx_TypoStyle_e typo;
	
	bool isTraceVisible;
	void (*onTraceToggle)(struct GraphOverlay *, int);
	
    char *name;
    bool bIsDirty;
} gfx_GraphOverlay;

typedef enum {
    CURSOR_VERTICAL,
    CURSOR_HORIZONTAL,
    CURSOR_CROSSHAIR
} gfx_CursorType;

typedef struct {
    const char *name;      // Identificador del widget
    Position pos;          // Aunque no la uses para renderizar, el motor podría exigirla
    Size size;             // Igual que la posición
    bool bIsDirty;         // Flag para refresco de pantalla
	RegionTouchObject regTouch;
    // -----------------------------------------------------

    const gfx_Graph *parent;
    gfx_CursorType type;
    int16_t relX;
    int16_t relY;
    uint8_t lineWidth;
    uint16_t color;
    uint8_t alpha;
    bool isVisible;
} gfx_GraphCursor;

typedef struct {
    char *name;
    Position pos;
    Size size;            // Se llenará automáticamente al decodificar el PNG
    uint8_t scale;        // 1 = Tamaño original, 2 = Doble, etc.
    uint32_t ramgAddress; // Dirección en RAM_G (Debe ser > 768000)
    bool bIsDirty;
} gfx_Image;

#define NUMPAD_MAX_DIGITS 		7
#define NUMPAD_SPACE_BUTTON		10
#define NUMPAD_DOT_BUTTON		11
#define NUMPAD_OK_BUTTON		12
#define NUMPAD_CANCEL_BUTTON	13
#define NUMPAD_DELETE_BUTTON	14	
typedef struct Numpad{
	char *name;
	Position pos;
	Size size;
	RegionTouchObject regTouch;
	bool bIsVisible;
	bool bIsDirty;
	void (*onOkButtonReleased)(struct Numpad *);
	void (*onCancelButtonReleased)(struct Numpad *);

	// Elements
	gfx_Rectangle _displayBg;
	gfx_Label _displayLabel;
	gfx_Label _displayValue;
	gfx_Button _buttons[15];
	int8_t _lastTouchedBtn;
	uint8_t _currentLen;
	char _digitBuffer[NUMPAD_MAX_DIGITS];
} gfx_Numpad;

// Añade esto a tu widget_type_e
// WD_TYPE_LISTVIEW

typedef enum {
    LV_ITEM_FILE,
    LV_ITEM_FOLDER,
    LV_ITEM_BACK     // Representa el ".." para subir de nivel
} gfx_ListViewItemType;

typedef struct {
    char text[32];
    gfx_ListViewItemType type;
} gfx_ListViewItem;

typedef struct ListView {
    char *name;
    Position pos;
    Size size;
    bool bIsVisible;
    bool bIsDirty;
	RegionTouchObject regTouch;
	int16_t _lastTouchedIndex;

    gfx_ListViewItem *items; // Puntero al arreglo de elementos
    uint16_t itemCount;      // Cuántos elementos hay cargados
    uint16_t maxItems;       // Capacidad máxima del arreglo

    uint16_t scrollOffset;   // Índice del primer elemento visible (para scroll)
    uint16_t visibleItems;   // Cuántos elementos caben en la pantalla
    uint16_t itemHeight;     // Altura en píxeles de cada fila

    // Estilos
    uint16_t bgColor;
    uint16_t lineColor;
    uint16_t textColor;
    gfx_TypoStyle_e typo;

    // Callback: Devuelve el puntero a la lista y el índice tocado
    void (*onItemSelect)(struct ListView *lv, uint16_t index);
} gfx_ListView;

typedef struct TouchAreaWidget{
	RegionTouchObject regTouch;
	Position pos;
	Size size;

	void (*onAreaTouchRelease)(struct TouchAreaWidget *);
	void (*onAreaTouchPressed)(struct TouchAreaWidget *);
} gfx_TouchArea;

typedef struct {
  widget_type_e eWidgetType;

  void *pvWidget;
} gfx_GenericWidget;

typedef struct GenericWidgetNode {
  gfx_GenericWidget sWidget;
  struct GenericWidgetNode *psNext;
  struct GenericWidgetNode *psPrev;
} gfx_GenericWidgetNode;

typedef struct {
  uint16_t ui16BackgroundColor;
  gfx_GenericWidgetNode *psWidgets;
} gfx_Canvas;

bool gfx_initRegTouch(void *, widget_type_e);

TouchStatus gfx_touchReadRegion(void);

//
// ************ Control Funcitons ************************
//
bool gfx_touchObject(RegionTouchObject, TouchStatus);

bool gfx_isWidgetTouched(gfx_GenericWidget *, TouchStatus);

bool gfx_clearSurface(gfx_Canvas *);

bool gfx_compositeFrame(gfx_Canvas *srf, pixel16_t *psPixelBuffer);

void gfx_start(uint32_t colorBackground);

void gfx_end(void);

void gfx_clear(void);

void gfx_calibrate(void);

bool gfx_getWidgetBounds(gfx_GenericWidget *widget, int16_t *x, int16_t *y,
                         int16_t *w, int16_t *h);
//
// ************ PrimitiveFuncitons ************************
//

void gfx_writeLine(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t x1,
                   int16_t y1, uint16_t color);

void gfx_drawFastVLine(pixel16_t *pBuf, int16_t x, int16_t y, int16_t h,
                       uint16_t color);

void gfx_drawFastHLine(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w,
                       uint16_t color);
void gfx_fillRect(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w, int16_t h,
                  uint16_t color);
void gfx_drawCircle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                    uint16_t color);

void gfx_drawCircleHelper(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                          uint8_t cornername, uint16_t color);

void gfx_fillCircle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                    uint16_t color);

void gfx_fillCircleHelper(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                          uint8_t corners, int16_t delta, uint16_t color);

void gfx_drawEllipse(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t rw,
                     int16_t rh, uint16_t color);
void gfx_fillEllipse(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t rw,
                     int16_t rh, uint16_t color);

void gfx_drawTriangle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t x1,
                      int16_t y1, int16_t x2, int16_t y2, uint16_t color);

void gfx_fillTriangle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t x1,
                      int16_t y1, int16_t x2, int16_t y2, uint16_t color);

void gfx_fillRoundRect(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w,
                       int16_t h, int16_t r, uint16_t color);

void gfx_fillGradientRoundRect(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w,
                               int16_t h, int16_t r, uint16_t colorTop,
                               uint16_t colorBottom);
//
// ************ Widget Funtions ************************
//

void gfx_drawButton(pixel16_t *pBuf, gfx_Button *btn);

void onGenericBtnPressed(gfx_Button *btn);

void onGenericBtnRelease(gfx_Button *btn);

void gfx_drawLabel(pixel16_t *pBuf, gfx_Label *lb);

void gfx_drawRectangle(pixel16_t *pBuf, gfx_Rectangle *rect);

void gfx_drawSlider(pixel16_t *pBuf, gfx_Slider *slider);

bool gfx_processSliderTouch(gfx_Slider *sl, TouchStatus touch);

void gfx_drawGraph(pixel16_t *pBuf, gfx_Graph *graph);

void gfx_GraphAddPoint(gfx_Graph *graph, float newValue);

void gfx_GraphRenderEVEComponents(gfx_Graph *graph);

void UpdateDisplayWithGraphOverlay(gfx_Graph *graph, gfx_Graph *graph2);

void gfx_MultiGraphAddData(gfx_MultiGraph *graph, uint8_t traceIndex, int16_t newValue);

void gfx_drawMultiGraph(pixel16_t *pBuf, gfx_MultiGraph *graph);

void gfx_MultigraphRenderEVEComponents(gfx_MultiGraph *graph);

void gfx_ImageRenderEVEComponents(gfx_Image *img);

bool gfx_ImageLoadPNG(gfx_Image *img, const uint8_t *pngData, uint32_t dataSize, uint32_t targetRamGAddrr);

gfx_DirtyRect gfx_ButtonProcessState(gfx_Button *btn);

gfx_DirtyRect gfx_LabelProcessState(gfx_Label *lbl);

gfx_DirtyRect gfx_SliderProcessState(gfx_Slider *sld);

gfx_DirtyRect gfx_GraphOverlayProcessState(gfx_GraphOverlay *ovl);

bool gfx_compositePartialFrame(gfx_Canvas *srf, pixel16_t *psPixelBuffer,
                               int16_t dirtyX, int16_t dirtyY, 
                               int16_t dirtyW, int16_t dirtyH);

void gfx_drawGraphOverlay(pixel16_t *pBuf, gfx_GraphOverlay *ovl);

void gfx_GraphCursorRenderEVE(gfx_GraphCursor *cursor);

float gfx_GraphCursorGetValue(gfx_GraphCursor *cursor);

float gfx_GraphGetAverageBetweenCursors(const gfx_GraphCursor *c1, const gfx_GraphCursor *c2);

float gfx_GraphGetMaxBetweenCursors(const gfx_GraphCursor *c1, const gfx_GraphCursor *c2);

float gfx_GraphGetMinBetweenCursors(const gfx_GraphCursor *c1, const gfx_GraphCursor *c2);

bool gfx_processCursorTouch(gfx_GraphCursor *cursor, TouchStatus touch);

int gfx_processOverlayTouch(gfx_GraphOverlay *ovl, TouchStatus touch);

bool gfx_getWidgetBounds(gfx_GenericWidget *widget, int16_t *x, int16_t *y,
                         int16_t *w, int16_t *h);

void gfx_NumpadInit(gfx_Numpad *np, char *lb, Position pos, Size size, bool isVisible);

void gfx_drawNumpad(pixel16_t *pBuf, gfx_Numpad *numpad);

bool gfx_NumpadProcessTouch(gfx_Numpad *np, TouchStatus touch);

void gfx_DualGraphAddData(gfx_DualGraph *graph, uint8_t traceIndex, float newValue);

void gfx_DualGraphAddStaticData(gfx_DualGraph *graph, uint8_t traceIndex, float newValue, uint32_t currentElapsedMs, uint32_t totalDurationMs);

void gfx_drawDualGraph(pixel16_t *pBuf, gfx_DualGraph *graph);

void gfx_DualGraphRenderEVEComponents(gfx_DualGraph *graph);

void gfx_drawListView(pixel16_t *pBuf, gfx_ListView *lv);

gfx_DirtyRect gfx_ListViewProcessState(gfx_ListView *view);

bool gfx_processListViewTouch(gfx_ListView *lv, TouchStatus touch);


void gfx_drawDashedLine(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t x1,
                        int16_t y1, uint16_t color, uint8_t dashLen, uint8_t spaceLen);

void gfx_drawFastHDashedLine(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w,
                             uint16_t color, uint8_t dashLen, uint8_t spaceLen);

void gfx_drawFastVDashedLine(pixel16_t *pBuf, int16_t x, int16_t y, int16_t h,
                             uint16_t color, uint8_t dashLen, uint8_t spaceLen);
#endif // GFX_H
