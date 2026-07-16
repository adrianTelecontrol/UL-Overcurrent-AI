/**
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils/uartstdio.h"

#include "gpu_ft81x.h"
#include "EVE_colors.h"
#include "FT8xx_params.h"
#include "font_engine.h"
#include "render.h"
#include "gui_theme.h"
#include "helpers.h"
#include "gui_colors.h"
#include "icon_map.h"

#include "gui_core.h"

const char *TASK_NAME = "gfx";

#ifndef _swap_int16_t
#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }
#endif

#define gfx_write_pixel(pBuf, x, y, color)                                     \
  do {                                                                         \
    if ((x) >= 0 && (x) < LCD_WIDTH && (y) >= 0 && (y) < LCD_HEIGHT) {         \
      (pBuf)[((y) * LCD_WIDTH) + (x)].u16 = (color);                           \
    }                                                                          \
  } while (0)

uint16_t blendRGB565(uint16_t colorTop, uint16_t colorBottom, int16_t currentY, int16_t totalH) {
    if (totalH <= 1) return colorTop; // Protección contra división por cero

    // 1. Extraer canales (desempaquetar 5-6-5)
    int32_t rT = (colorTop >> 11) & 0x1F;
    int32_t gT = (colorTop >> 5)  & 0x3F;
    int32_t bT = colorTop         & 0x1F;

    int32_t rB = (colorBottom >> 11) & 0x1F;
    int32_t gB = (colorBottom >> 5)  & 0x3F;
    int32_t bB = colorBottom         & 0x1F;

    // 2. Calcular la proporción (Escalada por 256 para evitar floats)
    int32_t ratio = (currentY * 256) / totalH; 

    // 3. Interpolar canales
    uint16_t r = rT + (((rB - rT) * ratio) >> 8);
    uint16_t g = gT + (((gB - gT) * ratio) >> 8);
    uint16_t b = bT + (((bB - bT) * ratio) >> 8);

    // 4. Empaquetar de vuelta a RGB565
    return (r << 11) | (g << 5) | b;
}

bool gfx_initRegTouch(void *widget, widget_type_e type) {
  if (type == WD_TYPE_BUTTON) {
    gfx_Button *wd = (gfx_Button *)widget;
    wd->regTouch.x1 = wd->pos.x;
    wd->regTouch.x2 = wd->pos.x + wd->size.width;
    wd->regTouch.y1 = wd->pos.y;
    wd->regTouch.y2 = wd->pos.y + wd->size.height;
  } else if (type == WD_TYPE_SLIDER) {
    gfx_Slider *wd = (gfx_Slider *)widget;
    
    wd->regTouch.x1 = wd->pos.x;
    wd->regTouch.x2 = wd->pos.x + wd->size.width;
    wd->regTouch.y1 = wd->pos.y - wd->knobRadius;
    wd->regTouch.y2 = wd->pos.y + wd->size.height + wd->knobRadius;
  } else if(type == WD_TYPE_GRAPH_CURSOR) {
    gfx_GraphCursor *cursor = (gfx_GraphCursor *)widget;

    cursor->regTouch.x1 = cursor->parent->pos.x + cursor->relX - 35;
    cursor->regTouch.x2 = cursor->parent->pos.x + cursor->relX + 35;
    cursor->regTouch.y1 = cursor->parent->pos.y + cursor->relY;
    cursor->regTouch.y2 = cursor->parent->pos.y + cursor->relX + cursor->parent->size.height;
    
  } else if (type == WD_TYPE_GRAPH_OVERLAY) {
    gfx_GraphOverlay *ovl = (gfx_GraphOverlay *)widget;
	
	ovl->regTouch.x1 = ovl->pos.x;
	ovl->regTouch.x2 = ovl->pos.x + ovl->size.width;
	ovl->regTouch.y1 = ovl->pos.y;
	ovl->regTouch.y2 = ovl->pos.y + ovl->size.height;
  } else if(type == WD_TYPE_TOUCH_AREA) {
	 gfx_TouchArea *touchArea = (gfx_TouchArea *)widget;

	 touchArea->regTouch.x1 = touchArea->pos.x;
	 touchArea->regTouch.x2 = touchArea->pos.x + touchArea->size.width;
	 touchArea->regTouch.y1 = touchArea->pos.y;
	 touchArea->regTouch.y2 = touchArea->pos.y + touchArea->size.height;
  } else if(type == WD_TYPE_NUMPAD) {
	gfx_Numpad *np = (gfx_Numpad *)widget;

	np->regTouch.x1 = np->_buttons[11].pos.x;
	np->regTouch.x2 = np->_buttons[14].pos.x + np->_buttons[14].size.width;
	np->regTouch.y1 = np->_buttons[11].pos.y;
	np->regTouch.y1 = np->_buttons[0].pos.y + np->_buttons[0].size.width;
  } else if(type == WD_TYPE_LISTVIEW) {
	gfx_ListView *list = (gfx_ListView *)widget;

	list->regTouch.x1 = list->pos.x;
	list->regTouch.x2 = list->pos.x + list->size.width;
	list->regTouch.y1 = list->pos.y;
	list->regTouch.y2 = list->pos.y + list->size.height;
  }
  return true;
}

void gfx_calibrate(void) {
  API_LIB_BeginCoProList(); // Begin new screen
  API_CMD_DLSTART();
  API_CLEAR_COLOR_RGB(0, 0, 0); // Clear screen
  API_CLEAR(1, 1, 1);
  API_CMD_TEXT(LCD_WIDTH / 2, LCD_HEIGHT / 2, 30, OPT_CENTER, "Calibracion de pantalla. Presione los puntos.");
  API_CMD_CALIBRATE(0xAAAAAAAA);
  API_DISPLAY();             // Tell EVE that this is end of list
  API_CMD_SWAP();            // Swap buffers in EVE to make this list active
  API_LIB_EndCoProList();    // Finish the co-processor list burst write
  API_LIB_AwaitCoProEmpty(); // Wait until co-processor has consumed all
                             // commands
}

TouchStatus gfx_touchReadRegion(void) {
  uint32_t regTouch;
  uint16_t touch_x;
  uint16_t touch_y;
  float_t xScreen;
  float_t yScreen;
  TouchStatus touch;

  regTouch = EVE_MemRead32(REG_TOUCH_SCREEN_XY); // Lee el registro del touch
  if (regTouch != 0X80008000)                    // y verifica si fue tocado
  {
    // Obtiene las coordenadas segï¿½n el sensor
    touch_x = (uint16_t)(regTouch >> 16);
    touch_y = (uint16_t)regTouch & 0xFFFF;
    // Si es mayor a la regiï¿½n del sensor no fue tocada la pantalla
    if ((touch_x > 800) || (touch_y > 480)) {
      xScreen = 0;
      yScreen = 0;
      touch.state = false;
    } else // Si las coordenas estï¿½n de la pantalla
    {
      xScreen = (float_t)touch_x * (float_t)950 / 965;
      yScreen = touch_y;

      touch.state = true;
    }
    regTouch = 0X80008000;
  } else {
    touch.state = false;
  }

  touch.x = (uint16_t)xScreen;
  touch.y = (uint16_t)yScreen;

  return touch;
}

bool gfx_touchObject(RegionTouchObject regObj, TouchStatus touch) {
  // Si queda dentro de la regiï¿½n deseada, entonces si fue tocado
  if ((touch.x >= regObj.x1) && (touch.x <= regObj.x2) &&
      (touch.y >= regObj.y1) && (touch.y <= regObj.y2))
    return true;

  return false;
}

bool gfx_compositeFrame(gfx_Canvas *srf, pixel16_t *psPixelBuffer) {
  if (srf == NULL) {
    UARTprintf("Cannot render canvas! Is empty.");
    return false;
  }
  
  uint32_t i = 0;
  for(; i < LCD_WIDTH * LCD_HEIGHT; i++)
  {
	psPixelBuffer[i].u16 = g_pCurrentTheme->palette.background;	
  }
  
  uint32_t j = i;
  j++;
  gfx_GenericWidgetNode *iter = srf->psWidgets;
  
  while (iter != NULL) {
    switch (iter->sWidget.eWidgetType) {
    case WD_TYPE_BUTTON:
      gfx_drawButton(psPixelBuffer, (gfx_Button *)iter->sWidget.pvWidget);
      break;
    case WD_TYPE_RECT:
      gfx_drawRectangle(psPixelBuffer, (gfx_Rectangle *)iter->sWidget.pvWidget);
      break;
    case WD_TYPE_LABEL:
      gfx_drawLabel(psPixelBuffer, (gfx_Label *)iter->sWidget.pvWidget);
	  break;
	case WD_TYPE_SLIDER:
      gfx_drawSlider(psPixelBuffer, (gfx_Slider *)iter->sWidget.pvWidget);	
      break;
	case WD_TYPE_GRAPH:
      gfx_drawGraph(psPixelBuffer, (gfx_Graph *)iter->sWidget.pvWidget);	
      break;
	case WD_TYPE_MULTIGRAPH:
      gfx_drawMultiGraph(psPixelBuffer, (gfx_MultiGraph *)iter->sWidget.pvWidget);	
      break;
	case WD_TYPE_GRAPH_OVERLAY:
	  gfx_drawGraphOverlay(psPixelBuffer, (gfx_GraphOverlay *)iter->sWidget.pvWidget);
	  break;
	case WD_TYPE_NUMPAD:
	  gfx_drawNumpad(psPixelBuffer, (gfx_Numpad *)iter->sWidget.pvWidget);
	  break;
	case WD_TYPE_DUAL_GRAPH:
	  gfx_drawDualGraph(psPixelBuffer, (gfx_DualGraph *)iter->sWidget.pvWidget);
	  break;
	case WD_TYPE_LISTVIEW:
	  gfx_drawListView(psPixelBuffer, (gfx_ListView *)iter->sWidget.pvWidget);
	  break;
    default:
      break;
    }

    iter = iter->psNext;
  }

  return true;
}

bool gfx_isWidgetTouched(gfx_GenericWidget *wd, TouchStatus touch) {
  if (wd->pvWidget == NULL)
    return false;

  bool ret = false;
  switch (wd->eWidgetType) {
  case WD_TYPE_BUTTON:
    ret = gfx_touchObject(((gfx_Button *)wd->pvWidget)->regTouch, touch);
    break;
  case WD_TYPE_SLIDER:
    ret = gfx_touchObject(((gfx_Slider *)wd->pvWidget)->regTouch, touch);
    break;
  case WD_TYPE_GRAPH_CURSOR:
    ret = gfx_touchObject(((gfx_GraphCursor *)wd->pvWidget)->regTouch, touch);
	break;
  case WD_TYPE_GRAPH_OVERLAY:
	ret = gfx_touchObject(((gfx_GraphOverlay *)wd->pvWidget)->regTouch, touch);
	break;
  case WD_TYPE_TOUCH_AREA:
	ret = gfx_touchObject(((gfx_TouchArea *)wd->pvWidget)->regTouch, touch);
	break;
  case WD_TYPE_LISTVIEW:
    ret = gfx_touchObject(((gfx_ListView *)wd->pvWidget)->regTouch, touch);
    break;
  default:
    break;
  }

  return ret;
}

void gfx_start(uint32_t colorBackground) {
  API_LIB_BeginCoProList(); // Begin new screen
  API_CMD_DLSTART();

  API_CLEAR_COLOR_RGB((uint8_t)(colorBackground >> 16),
                      (uint8_t)(colorBackground >> 8),
                      (uint8_t)colorBackground);
  API_CLEAR(1, 1, 1); // Tell EVE that this is end of list
}

void gfx_end(void) {
  API_DISPLAY();  // Ends the diplay cmd list
  API_CMD_SWAP(); // Swap buffers in EVE to make this list active

  // EVE_Flush_Buffer();
  API_LIB_EndCoProList(); // Finish the co-processor list burst write
  API_LIB_AwaitCoProEmpty();
}

void gfx_clear(void) {
  gfx_start(EVE_BLACK);
  gfx_end();
}

/**************************************************************************/
/*!
   @brief    Write a line.  Bresenham's algorithm - thx wikpedia
    @param    x0  Start point x coordinate
    @param    y0  Start point y coordinate
    @param    x1  End point x coordinate
    @param    y1  End point y coordinate
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void gfx_writeLine(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t x1,
                   int16_t y1, uint16_t color) {
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    _swap_int16_t(x0, y0);
    _swap_int16_t(x1, y1);
  }

  if (x0 > x1) {
    _swap_int16_t(x0, x1);
    _swap_int16_t(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0 <= x1; x0++) {
    if (steep) {
      gfx_write_pixel(pBuf, y0, x0, color);
    } else {
      gfx_write_pixel(pBuf, x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void gfx_drawFastVLine(pixel16_t *pBuf, int16_t x, int16_t y, int16_t h,
                       uint16_t color) {
  gfx_writeLine(pBuf, x, y, x, y + h - 1, color);
}

void gfx_drawFastHLine(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w,
                       uint16_t color) {
  gfx_writeLine(pBuf, x, y, x + w - 1, y, color);
}

void gfx_fillRect(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w, int16_t h,
                  uint16_t color) {
  int16_t i = x;
  for (; i < x + w; i++) {
    gfx_drawFastVLine(pBuf, i, y, h, color);
  }
}

void gfx_drawCircle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                    uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  gfx_write_pixel(pBuf, x0, y0 + r, color);
  gfx_write_pixel(pBuf, x0, y0 - r, color);
  gfx_write_pixel(pBuf, x0 + r, y0, color);
  gfx_write_pixel(pBuf, x0 - r, y0, color);

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    gfx_write_pixel(pBuf, x0 + x, y0 + y, color);
    gfx_write_pixel(pBuf, x0 - x, y0 + y, color);
    gfx_write_pixel(pBuf, x0 + x, y0 - y, color);
    gfx_write_pixel(pBuf, x0 - x, y0 - y, color);
    gfx_write_pixel(pBuf, x0 + y, y0 + x, color);
    gfx_write_pixel(pBuf, x0 - y, y0 + x, color);
    gfx_write_pixel(pBuf, x0 + y, y0 - x, color);
    gfx_write_pixel(pBuf, x0 - y, y0 - x, color);
  }
}

void gfx_drawCircleHelper(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                          uint8_t cornername, uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (cornername & 0x4) {
      gfx_write_pixel(pBuf, x0 + x, y0 + y, color);
      gfx_write_pixel(pBuf, x0 + y, y0 + x, color);
    }
    if (cornername & 0x2) {
      gfx_write_pixel(pBuf, x0 + x, y0 - y, color);
      gfx_write_pixel(pBuf, x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      gfx_write_pixel(pBuf, x0 - y, y0 + x, color);
      gfx_write_pixel(pBuf, x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      gfx_write_pixel(pBuf, x0 - y, y0 - x, color);
      gfx_write_pixel(pBuf, x0 - x, y0 - y, color);
    }
  }
}

void gfx_fillCircle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                    uint16_t color) {
  gfx_drawFastVLine(pBuf, x0, y0 - r, 2 * r + 1, color);
  gfx_fillCircleHelper(pBuf, x0, y0, r, 3, 0, color);
}

void gfx_fillCircleHelper(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t r,
                          uint8_t corners, int16_t delta, uint16_t color) {

  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;
  int16_t px = x;
  int16_t py = y;

  delta++; // Avoid some +1's in the loop

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    // These checks avoid double-drawing certain lines, important
    // for the SSD1306 library which has an INVERT drawing mode.
    if (x < (y + 1)) {
      if (corners & 1)
        gfx_drawFastVLine(pBuf, x0 + x, y0 - y, 2 * y + delta, color);
      if (corners & 2)
        gfx_drawFastVLine(pBuf, x0 - x, y0 - y, 2 * y + delta, color);
    }
    if (y != py) {
      if (corners & 1)
        gfx_drawFastVLine(pBuf, x0 + py, y0 - px, 2 * px + delta, color);
      if (corners & 2)
        gfx_drawFastVLine(pBuf, x0 - py, y0 - px, 2 * px + delta, color);
      py = y;
    }
    px = x;
  }
}

void gfx_drawEllipse(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t rw,
                     int16_t rh, uint16_t color) {
  // Bresenham's ellipse algorithm
  int16_t x = 0, y = rh;
  int32_t rw2 = rw * rw, rh2 = rh * rh;
  int32_t twoRw2 = 2 * rw2, twoRh2 = 2 * rh2;

  int32_t decision = rh2 - (rw2 * rh) + (rw2 / 4);

  // region 1
  while ((twoRh2 * x) < (twoRw2 * y)) {
    gfx_write_pixel(pBuf, x0 + x, y0 + y, color);
    gfx_write_pixel(pBuf, x0 - x, y0 + y, color);
    gfx_write_pixel(pBuf, x0 + x, y0 - y, color);
    gfx_write_pixel(pBuf, x0 - x, y0 - y, color);
    x++;
    if (decision < 0) {
      decision += rh2 + (twoRh2 * x);
    } else {
      decision += rh2 + (twoRh2 * x) - (twoRw2 * y);
      y--;
    }
  }

  // region 2
  decision = ((rh2 * (2 * x + 1) * (2 * x + 1)) >> 2) +
             (rw2 * (y - 1) * (y - 1)) - (rw2 * rh2);
  while (y >= 0) {
    gfx_write_pixel(pBuf, x0 + x, y0 + y, color);
    gfx_write_pixel(pBuf, x0 - x, y0 + y, color);
    gfx_write_pixel(pBuf, x0 + x, y0 - y, color);
    gfx_write_pixel(pBuf, x0 - x, y0 - y, color);
    y--;
    if (decision > 0) {
      decision += rw2 - (twoRw2 * y);
    } else {
      decision += rw2 + (twoRh2 * x) - (twoRw2 * y);
      x++;
    }
  }
}

void gfx_fillEllipse(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t rw,
                     int16_t rh, uint16_t color) {
  // Bresenham's ellipse algorithm
  int16_t x = 0, y = rh;
  int32_t rw2 = rw * rw, rh2 = rh * rh;
  int32_t twoRw2 = 2 * rw2, twoRh2 = 2 * rh2;

  int32_t decision = rh2 - (rw2 * rh) + (rw2 / 4);

  // region 1
  while ((twoRh2 * x) < (twoRw2 * y)) {
    x++;
    if (decision < 0) {
      decision += rh2 + (twoRh2 * x);
    } else {
      decision += rh2 + (twoRh2 * x) - (twoRw2 * y);
      gfx_drawFastHLine(pBuf, x0 - (x - 1), y0 + y, 2 * (x - 1) + 1, color);
      gfx_drawFastHLine(pBuf, x0 - (x - 1), y0 - y, 2 * (x - 1) + 1, color);
      y--;
    }
  }

  // region 2
  decision = ((rh2 * (2 * x + 1) * (2 * x + 1)) >> 2) +
             (rw2 * (y - 1) * (y - 1)) - (rw2 * rh2);
  while (y >= 0) {
    gfx_drawFastHLine(pBuf, x0 - x, y0 + y, 2 * x + 1, color);
    gfx_drawFastHLine(pBuf, x0 - x, y0 - y, 2 * x + 1, color);

    y--;
    if (decision > 0) {
      decision += rw2 - (twoRw2 * y);
    } else {
      decision += rw2 + (twoRh2 * x) - (twoRw2 * y);
      x++;
    }
  }
}

void gfx_drawRoundRect(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w,
                       int16_t h, int16_t r, uint16_t color) {
  int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
  if (r > max_radius)
    r = max_radius;
  // smarter version
  gfx_drawFastHLine(pBuf, x + r, y, w - 2 * r, color);         // Top
  gfx_drawFastHLine(pBuf, x + r, y + h - 1, w - 2 * r, color); // Bottom
  gfx_drawFastVLine(pBuf, x, y + r, h - 2 * r, color);         // Left
  gfx_drawFastVLine(pBuf, x + w - 1, y + r, h - 2 * r, color); // Right
  // draw four corners
  gfx_drawCircleHelper(pBuf, x + r, y + r, r, 1, color);
  gfx_drawCircleHelper(pBuf, x + w - r - 1, y + r, r, 2, color);
  gfx_drawCircleHelper(pBuf, x + w - r - 1, y + h - r - 1, r, 4, color);
  gfx_drawCircleHelper(pBuf, x + r, y + h - r - 1, r, 8, color);
}

void gfx_fillRoundRect(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w,
                       int16_t h, int16_t r, uint16_t color) {
  int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
  if (r > max_radius)
    r = max_radius;
  // smarter version
  gfx_fillRect(pBuf, x + r, y, w - 2 * r, h, color);
  // draw four corners
  gfx_fillCircleHelper(pBuf, x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
  gfx_fillCircleHelper(pBuf, x + r, y + r, r, 2, h - 2 * r - 1, color);
}

void gfx_fillGradientRoundRect(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t colorTop, uint16_t colorBottom) {
    int16_t max_radius = ((w < h) ? w : h) / 2;
    if (r > max_radius) r = max_radius;
    if (r > 32) r = 32; // Límite de seguridad para el arreglo

    // 1. Precalcular los recortes de las esquinas para evitar usar sqrt()
    int16_t cornerX[32] = {0};
	int16_t i = 0;
    for (; i < r; i++) {
        int32_t dy = r - i;
        int32_t r2 = r * r;
        int32_t dx = 0;
        
        // Algoritmo rápido para encontrar la X del círculo en esta Y
        while ((dx * dx) + (dy * dy) <= r2) { dx++; }
        dx--; 
        
        cornerX[i] = r - dx; // Píxeles vacíos en los bordes
    }

    // 2. Renderizar fila por fila de arriba a abajo
	int16_t row = 0;
    for (; row < h; row++) {
        uint16_t rowColor = blendRGB565(colorTop, colorBottom, row, h);

        int16_t startX = 0;
        if (row < r) {
            startX = cornerX[row];         // Curvatura superior
        } else if (row >= h - r) {
            startX = cornerX[h - 1 - row]; // Curvatura inferior
        }

        int16_t drawW = w - (startX * 2);
        int16_t drawX = x + startX;

        // Dibujar la línea horizontal para esta fila
        gfx_fillRect(pBuf, drawX, y + row, drawW, 1, rowColor);
    }
}

void gfx_drawTriangle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t x1,
                      int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
  gfx_writeLine(pBuf, x0, y0, x1, y1, color);
  gfx_writeLine(pBuf, x1, y1, x2, y2, color);
  gfx_writeLine(pBuf, x2, y2, x0, y0, color);
}

void gfx_fillTriangle(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t x1,
                      int16_t y1, int16_t x2, int16_t y2, uint16_t color) {

  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    _swap_int16_t(y0, y1);
    _swap_int16_t(x0, x1);
  }
  if (y1 > y2) {
    _swap_int16_t(y2, y1);
    _swap_int16_t(x2, x1);
  }
  if (y0 > y1) {
    _swap_int16_t(y0, y1);
    _swap_int16_t(x0, x1);
  }

  if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if (x1 < a)
      a = x1;
    else if (x1 > b)
      b = x1;
    if (x2 < a)
      a = x2;
    else if (x2 > b)
      b = x2;
    gfx_drawFastHLine(pBuf, a, y0, b - a + 1, color);
    return;
  }

  int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
          dx12 = x2 - x1, dy12 = y2 - y1;
  int32_t sa = 0, sb = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if (y1 == y2)
    last = y1; // Include y1 scanline
  else
    last = y1 - 1; // Skip it

  for (y = y0; y <= last; y++) {
    a = x0 + sa / dy01;
    b = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b)
      _swap_int16_t(a, b);
    gfx_drawFastHLine(pBuf, a, y, b - a + 1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = (int32_t)dx12 * (y - y1);
  sb = (int32_t)dx02 * (y - y0);
  for (; y <= y2; y++) {
    a = x1 + sa / dy12;
    b = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b)
      _swap_int16_t(a, b);
    gfx_drawFastHLine(pBuf, a, y, b - a + 1, color);
  }
}

//
// **************** Widget Functions **************************
//
// Pre-calcula los píxeles vacíos en el borde de la esquina para evitar sqrt() en el bucle principal.
// cornerX guarda la 'X delta' (píxeles vacíos) para cada 'dy' (fila de la esquina).
static void gfx_getCornerEmptyPixels(int16_t *cornerX, int16_t r) {
    if (r <= 0) return;
    if (r > 32) r = 32; // Límite de seguridad del array

	int16_t i = 0;
    for (; i < r; i++) {
        int32_t dy = r - i;
        int32_t r2 = r * r;
        int32_t dx = 0;
        
        // Algoritmo rápido de círculo entero: encontrar la X máxima dentro del círculo a esta Y.
        while ((dx * dx) + (dy * dy) <= r2) { dx++; }
        dx--; 
        
        cornerX[i] = r - dx; // Número de píxeles vacíos en el borde
    }
}

// Dibuja un marco de contorno redondeado geométricamente perfecto usando la misma lógica que el relleno de degradado.
// x, y, w, h representan la huella GENERAL (footprint) del objeto incluyendo el borde.
void gfx_drawRoundOutline(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w, int16_t h, int16_t r_outer, int16_t borderWidth, uint16_t color) {
    if (!pBuf || borderWidth <= 0) return;

    // Geometría Exterior
    int16_t max_r_outer = ((w < h) ? w : h) / 2;
    if (r_outer > max_r_outer) r_outer = max_r_outer;
    if (r_outer > 32) r_outer = 32;

    // Geometría Interior (donde encaja el fondo)
    int16_t r_inner = r_outer - borderWidth;
    if (r_inner < 0) r_inner = 0; // Si el borde es más grueso que el radio, la esquina interior es cuadrada.

    // Límites para el área de relleno interior
    int16_t innerXStart = x + borderWidth;
    int16_t innerH = h - (borderWidth * 2);

    // Pre-calcular curvaturas para ambos radios
    int16_t emptyPixelsOuter[32] = {0};
    int16_t emptyPixelsInner[32] = {0};
    gfx_getCornerEmptyPixels(emptyPixelsOuter, r_outer);
    gfx_getCornerEmptyPixels(emptyPixelsInner, r_inner);

    // Renderizar fila por fila
	int16_t row = 0;
    for (; row < h; row++) {
        
        // Encontrar contorno exterior para esta fila
        int16_t emptyOuterX = 0;
        if (row < r_outer) {
            emptyOuterX = emptyPixelsOuter[row];
        } else if (row >= h - r_outer) {
            emptyOuterX = emptyPixelsOuter[h - 1 - row];
        }
        int16_t drawStartX_outer = x + emptyOuterX;
        int16_t drawEndX_outer = x + w - emptyOuterX;
        int16_t drawW_outer = drawEndX_outer - drawStartX_outer;

        // ¿Esta fila es puramente borde sólido (Top/Bottom)?
        if (row < borderWidth || row >= (h - borderWidth)) {
            // Dibujar la barra exterior completa
            gfx_fillRect(pBuf, drawStartX_outer, y + row, drawW_outer, 1, color);
        } else {
            // Esta fila pasa por la sección central (con "agujero" para el fondo).
            // Dibujar dos segmentos (borde izquierdo y derecho).
            
            int16_t innerRow = row - borderWidth; // Normalizar a coordenada y del rectángulo interior

            // Encontrar lógica de contorno interior (donde empieza el fondo)
            int16_t emptyInnerX = 0;
            if (innerRow < r_inner) {
                emptyInnerX = emptyPixelsInner[innerRow];
            } else if (innerRow >= (innerH - r_inner)) {
                // Ajustar para la curva inferior del *rectángulo interior*
                emptyInnerX = emptyPixelsInner[innerH - 1 - innerRow];
            }

            int16_t startX_inner = innerXStart + emptyInnerX;
            int16_t endX_inner = x + w - borderWidth - emptyInnerX;

            // Dibujar Segmento Izquierdo
            gfx_fillRect(pBuf, drawStartX_outer, y + row, startX_inner - drawStartX_outer, 1, color);

            // Dibujar Segmento Derecho
            gfx_fillRect(pBuf, endX_inner, y + row, drawEndX_outer - endX_inner, 1, color);
        }
    }
}

void gfx_drawButton(pixel16_t *pBuf, gfx_Button *btn) {
    if (!pBuf || !btn || !g_pCurrentTheme || !btn->bIsVisible) return;

    // 1. Resolve colors semantically from the active theme
    uint16_t baseBgColor;
    uint16_t baseTextColor = g_pCurrentTheme->palette.textMain;
    uint16_t baseBorderColor = g_pCurrentTheme->palette.border;

    switch (btn->style) {
        case STYLE_PRIMARY:
            baseBgColor = g_pCurrentTheme->palette.primary;
            break;
        case STYLE_SECONDARY:
            baseBgColor = g_pCurrentTheme->palette.secondary;
            break;
        case STYLE_SUCCESS:
            baseBgColor = g_pCurrentTheme->palette.success;
            break;
        case STYLE_DANGER:
            baseBgColor = g_pCurrentTheme->palette.danger;
            break;
        case STYLE_DEFAULT:
        default:
            baseBgColor = g_pCurrentTheme->palette.surface;
            break;
    }

    // 2. Apply Physical State Modifiers (Cinematic shift & darken)
    uint16_t topColor = baseBgColor;
    uint16_t bottomColor = DARKEN_COLOR(baseBgColor);
    int16_t offset = 0;

    if (btn->state == BTN_STATE_PRESSED) {
        offset = 2;
        topColor = DARKEN_COLOR(baseBgColor);
        bottomColor = baseBgColor;
        baseBorderColor = DARKEN_COLOR(baseBorderColor);
    } else if (btn->state == BTN_STATE_DISABLED) {
        topColor = 0x4208;
        bottomColor = 0x2104;
        baseBorderColor = 0x8410;
        baseTextColor = g_pCurrentTheme->palette.textMuted; // Use theme's muted color
    }

    // 3. Calculate footprints and render the chassis
    int16_t footprintX = btn->pos.x - btn->borderWidth + offset;
    int16_t footprintY = btn->pos.y - btn->borderWidth + offset;
    int16_t footprintW = btn->size.width + (btn->borderWidth * 2);
    int16_t footprintH = btn->size.height + (btn->borderWidth * 2);
    int16_t r_outer = btn->radius + btn->borderWidth;

    if (btn->borderWidth > 0) {
        gfx_drawRoundOutline(pBuf, footprintX, footprintY, footprintW, footprintH, 
                             r_outer, btn->borderWidth, baseBorderColor);
    }

    gfx_fillGradientRoundRect(pBuf, 
                              btn->pos.x + offset, 
                              btn->pos.y + offset, 
                              btn->size.width, 
                              btn->size.height, 
                              btn->radius, topColor, bottomColor);

    // 4. Render the localized Font using the global Theme color
    if (btn->label != NULL) {
        int8_t fontId = -1;
        
        // Resolver el ID de la fuente según la jerarquía solicitada
        switch (btn->typo) {
            case TYPO_H1:      		fontId = g_pCurrentTheme->fonts.h1; break;
            case TYPO_H2:      		fontId = g_pCurrentTheme->fonts.h2; break;
            case TYPO_H3:      		fontId = g_pCurrentTheme->fonts.h3; break;
            case TYPO_BODY:    		fontId = g_pCurrentTheme->fonts.body; break;
            case TYPO_CAPTION: 		fontId = g_pCurrentTheme->fonts.caption; break;
            case TYPO_MONO:    		fontId = g_pCurrentTheme->fonts.mono; break;
			case TYPO_MONO_BOLD:	fontId = g_pCurrentTheme->fonts.mono_bold; break; 
			case TYPO_ICON:			fontId = g_pCurrentTheme->fonts.icon; break;
        }
        
        // Si el motor SD logró cargar la fuente, la dibujamos
        if (fontId >= 0) {
            FontEngine_DrawString(pBuf, fontId, 
                           btn->pos.x + offset + (btn->size.width / 2),
                           btn->pos.y + offset + (btn->size.height / 2), 
                           btn->label, baseTextColor, ALIGN_CENTER, 1);
        }
    }
}

void onGenericBtnPressed(gfx_Button *btn) {
    btn->state = BTN_STATE_PRESSED;
    btn->bIsDirty = true;
}

void onGenericBtnRelease(gfx_Button *btn) {
    btn->state = BTN_STATE_NORMAL;
    btn->bIsDirty = true;
}

void gfx_drawLabel(pixel16_t *pBuf, gfx_Label *lb) {
    if (!pBuf || !lb || !lb->text || !g_pCurrentTheme || !lb->isVisible) return;

    // 1. Resolve Semantic Color from the Theme Palette
    // Labels default to textMain, but can be overridden (e.g., a Red DANGER label)
    uint16_t activeColor = g_pCurrentTheme->palette.textMain;
    
    switch (lb->style) {
        case STYLE_PRIMARY:
            activeColor = g_pCurrentTheme->palette.primary;
            break;
        case STYLE_DANGER:
            activeColor = g_pCurrentTheme->palette.danger;
            break;
        case STYLE_SUCCESS:
            activeColor = g_pCurrentTheme->palette.success;
            break;
		case STYLE_TEXT_MAIN:
			activeColor = g_pCurrentTheme->palette.textMain;
			break;
		case STYLE_TEXT_MUTED:
			activeColor = g_pCurrentTheme->palette.textMuted;
			break;
        case STYLE_SECONDARY:
            // Great for subtext/captions that shouldn't distract the user
            activeColor = g_pCurrentTheme->palette.secondary; 
            break;
        case STYLE_DEFAULT:
        default:
            activeColor = g_pCurrentTheme->palette.textMain;
            break;
    }

    // 2. Resolve Semantic Font ID from the Theme Typography
    int8_t fontId = -1;
    switch (lb->typo) {
        case TYPO_H1:      fontId = g_pCurrentTheme->fonts.h1; break;
        case TYPO_H2:      fontId = g_pCurrentTheme->fonts.h2; break;
        case TYPO_H3:      fontId = g_pCurrentTheme->fonts.h3; break;
        case TYPO_BODY:    fontId = g_pCurrentTheme->fonts.body; break;
        case TYPO_CAPTION: fontId = g_pCurrentTheme->fonts.caption; break;
        case TYPO_MONO:    fontId = g_pCurrentTheme->fonts.mono; break;
        case TYPO_MONO_BOLD:    fontId = g_pCurrentTheme->fonts.mono_bold; break;
		case TYPO_ICON: 	fontId = g_pCurrentTheme->fonts.icon; break;
    }

    // 3. Render the string if the font was successfully loaded from the SD card
    if (fontId >= 0) {
        FontEngine_DrawString(pBuf, fontId, lb->pos.x, lb->pos.y, lb->text, activeColor, lb->alignment, 1);
    }
}

void gfx_drawRectangle(pixel16_t *pBuf, gfx_Rectangle *rect)
{
	//gfx_fillRect(pBuf, rect->pos.x, rect->pos.y, rect->dim.width, rect->dim.height, rect->color);
	gfx_fillRoundRect(pBuf, rect->pos.x, rect->pos.y, rect->dim.width, rect->dim.height, rect->round, rect->color);

    if (rect->borderWidth > 0) {
    	int16_t footprintX = rect->pos.x - rect->borderWidth;
    	int16_t footprintY = rect->pos.y - rect->borderWidth;
    	int16_t footprintW = rect->dim.width + (rect->borderWidth * 2);
    	int16_t footprintH = rect->dim.height + (rect->borderWidth * 2);
    	int16_t r_outer = rect->round + rect->borderWidth;
        gfx_drawRoundOutline(pBuf, footprintX, footprintY, footprintW, footprintH, 
                             r_outer, rect->borderWidth, LIGHTEN_COLOR(rect->color));
    }
}

void gfx_drawSlider(pixel16_t *pBuf, gfx_Slider *sl) {
    if (!pBuf || !sl || !g_pCurrentTheme) return;

    // 1. Resolver colores base del tema
    uint16_t trackBgColorTop = g_pCurrentTheme->palette.secondary;      
    uint16_t trackBgColorBot = DARKEN_COLOR(trackBgColorTop); 

    uint16_t trackActiveColorTop = g_pCurrentTheme->palette.success;  
    uint16_t trackActiveColorBot = DARKEN_COLOR(trackActiveColorTop); 

    uint16_t knobColorTop = g_pCurrentTheme->palette.primary;         

    // 2. Proteger límites matemáticos
    if (sl->currentValue < sl->minValue) sl->currentValue = sl->minValue;
    if (sl->currentValue > sl->maxValue) sl->currentValue = sl->maxValue;

    // 3. Abstracción de Orientación (Horizontal vs Vertical)
    uint16_t thickness = sl->bIsVertical ? sl->size.width : sl->size.height;
    uint16_t length    = sl->bIsVertical ? sl->size.height : sl->size.width;

    // Geometría Dinámica
    uint16_t dynamicKnobRadius = thickness * 1.25;
    sl->knobRadius = dynamicKnobRadius;

    int16_t pixelRange = sl->bShowKnob ? length - (1.5 * dynamicKnobRadius) : length - dynamicKnobRadius;
    int16_t valueRange = sl->maxValue - sl->minValue;
    if (valueRange == 0) valueRange = 1;

    int16_t activeOffset = ((sl->currentValue - sl->minValue) * pixelRange) / valueRange;

    // Variables finales de renderizado
    int16_t knobCenterX, knobCenterY;
    int16_t activeTrackX, activeTrackY, activeTrackW, activeTrackH;

    if (sl->bIsVertical) {
        // --- LOGICA VERTICAL ---
        knobCenterX = sl->pos.x + (thickness / 2);
        // Empieza en la parte inferior (pos.y + length) y sube (- activeOffset)
        knobCenterY = (sl->pos.y + length) - (1.0 * dynamicKnobRadius) - activeOffset;

        if (sl->currentValue == 0 && sl->bShowKnob) {
            knobCenterY = (sl->pos.y + length) - (0.5 * dynamicKnobRadius);
        }

        activeTrackX = sl->pos.x;
        activeTrackY = knobCenterY; // Dibuja desde el knob hacia abajo
        activeTrackW = thickness;
        activeTrackH = (sl->pos.y + length) - knobCenterY;
        
    } else {
        // --- LOGICA HORIZONTAL ---
        knobCenterX = sl->pos.x + (1.0 * dynamicKnobRadius) + activeOffset;
        knobCenterY = sl->pos.y + (thickness / 2);

        if (sl->currentValue == 0 && sl->bShowKnob) {
            knobCenterX = sl->pos.x + (0.5 * dynamicKnobRadius);
        }

        activeTrackX = sl->pos.x;
        activeTrackY = sl->pos.y;
        activeTrackW = knobCenterX - sl->pos.x;
        activeTrackH = thickness;
    }

    // =========================================================
    // DIBUJADO CON GRADIENTES
    // =========================================================
    
    // 1. Track de Fondo
    gfx_fillGradientRoundRect(pBuf, 
                              sl->pos.x, sl->pos.y, 
                              sl->size.width, sl->size.height, 
                              thickness, 
                              trackBgColorBot, trackBgColorTop); 

    // 2. Track Activo
    if (sl->currentValue > sl->minValue || (sl->currentValue == 0 && !sl->bShowKnob)) { 
        gfx_fillGradientRoundRect(pBuf, 
                                  activeTrackX, activeTrackY, 
                                  activeTrackW, activeTrackH, 
                                  thickness, 
                                  trackActiveColorTop, trackActiveColorBot);
    }

    // 3. Dibujar la Perilla (Knob)
    if(sl->bShowKnob) {
        gfx_fillCircle(pBuf, knobCenterX, knobCenterY, dynamicKnobRadius / 2, trackBgColorBot);
        gfx_fillCircle(pBuf, knobCenterX, knobCenterY, (dynamicKnobRadius / 2) - 2, knobColorTop);
    }

    // Limpiar bandera
    sl->bIsDirty = false;
}

bool gfx_processSliderTouch(gfx_Slider *sl, TouchStatus touch) {
    if (gfx_touchObject(sl->regTouch, touch)) {
        
        uint16_t thickness = sl->bIsVertical ? sl->size.width : sl->size.height;
        uint16_t dynamicKnobRadius = thickness * 1.25;
        if (dynamicKnobRadius < 2) dynamicKnobRadius = 2;

        int16_t minLimit, maxLimit, touchAxis;
        int16_t newValue = 0;

        if (sl->bIsVertical) {
            // Evaluamos el eje Y (invertido, porque Y=0 es la parte superior)
            minLimit = sl->pos.y + dynamicKnobRadius;
            maxLimit = sl->pos.y + sl->size.height - dynamicKnobRadius;
            touchAxis = touch.y;
            
            if (touchAxis < minLimit) touchAxis = minLimit;
            if (touchAxis > maxLimit) touchAxis = maxLimit;
            
            int16_t pixelRange = maxLimit - minLimit;
            int16_t valueRange = sl->maxValue - sl->minValue;
            
            // Invertimos la matemática: Tocar arriba = MaxValue
            newValue = sl->maxValue - (((touchAxis - minLimit) * valueRange) / pixelRange);
            
        } else {
            // Evaluamos el eje X (como lo tenías antes)
            minLimit = sl->pos.x + dynamicKnobRadius;
            maxLimit = sl->pos.x + sl->size.width - dynamicKnobRadius;
            touchAxis = touch.x;
            
            if (touchAxis < minLimit) touchAxis = minLimit;
            if (touchAxis > maxLimit) touchAxis = maxLimit;
            
            int16_t pixelRange = maxLimit - minLimit;
            int16_t valueRange = sl->maxValue - sl->minValue;
            
            newValue = sl->minValue + (((touchAxis - minLimit) * valueRange) / pixelRange);
        }

        if (newValue != sl->currentValue) {
            sl->currentValue = newValue;
            sl->bIsDirty = true;
            if (sl->onValueChanged != NULL) {
                sl->onValueChanged(sl, sl->currentValue);
            }
            return true; 
        }
    }
    return false;
}

bool gfx_processCursorTouch(gfx_GraphCursor *cursor, TouchStatus touch) {
	if(gfx_touchObject(cursor->regTouch, touch)) {
		// Clamp the values
		// if(touch.x > cursor->parent->pos.x + cursor->parent->size.width) 
		// 	cursor->relX = cursor->parent->pos.x + cursor->parent->size.width;
		// else if(touch.x < cursor->parent->pos.x)
		// 	cursor->relX = cursor->parent->pos.x;
		// else 
		//  	cursor->relX = touch.x - cursor->parent->pos.x;

		cursor->relX = touch.x;

		gfx_initRegTouch((void *)cursor, WD_TYPE_GRAPH_CURSOR);
		return true;
	}

	return false;
}

void gfx_drawGraph(pixel16_t *pBuf, gfx_Graph *graph) {
    if (!pBuf || !graph) return;

    // 1. Dibujar fondo de la gráfica
    gfx_fillRoundRect(pBuf, graph->pos.x, graph->pos.y, 
                      graph->size.width, graph->size.height, 4, g_pCurrentTheme->palette.surface);

    int8_t fontId = -1;
    uint16_t textW = 0, textH = 0;
    
    // Solo cargar la fuente si vamos a dibujar algún texto
    if (graph->bShowLabels || graph->bShowXLabels || graph->xAxisName[0] != '\0' || graph->yAxisName[0] != '\0') {
        fontId = Theme_ResolveFontId(graph->typo); 
        if (fontId >= 0) {
            // Obtener la altura estándar de un número para centrarlo verticalmente
            FontEngine_GetStringDimensions("0", fontId, &textW, &textH, 1); 
        }
    }

    // =========================================================
    // 2. Nombres de los Ejes (Títulos)
    // =========================================================
    if (fontId >= 0) {
        // Eje Y (Arriba a la izquierda, alineado al fondo para no pisar la gráfica)
        if (graph->yAxisName != NULL && graph->yAxisName[0] != '\0') {
            FontEngine_DrawString(pBuf, fontId, 
                                  graph->pos.x, graph->pos.y - 5, 
                                  graph->yAxisName, graph->textColor, 
                                  (gfx_Align_e)(ALIGN_LEFT | ALIGN_BOTTOM), 1);
        }
        
        // Eje X (Abajo a la derecha, debajo de los números)
        if (graph->xAxisName != NULL && graph->xAxisName[0] != '\0') {
            FontEngine_DrawString(pBuf, fontId, 
                                  graph->pos.x + graph->size.width, graph->pos.y + graph->size.height + 22, 
                                  graph->xAxisName, graph->textColor, 
                                  (gfx_Align_e)(ALIGN_RIGHT | ALIGN_TOP), 1);
        }
    }

    // =========================================================
    // 3. Cuadrícula y Etiquetas (Eje Y)
    // =========================================================
    int16_t stepY = graph->size.height / (graph->gridLinesY + 1);
    int16_t valueStep = (graph->maxY - graph->minY) / (graph->gridLinesY + 1);

    // Iteramos desde 0 hasta gridLinesY + 1 para incluir el techo (Max) y el piso (Min)
    int i = 0;
    for (; i <= (graph->gridLinesY + 1); i++) {
        
        int16_t gy = graph->pos.y + (i * stepY);
        int16_t gridValue = graph->maxY - (i * valueStep); // Y está invertido, arriba es Max

        // Dibujar la línea horizontal (Saltamos i=0 y el último i para no sobreescribir el borde)
        if (i > 0 && i < (graph->gridLinesY + 1)) {
            gfx_drawFastHLine(pBuf, graph->pos.x, gy, graph->size.width, graph->gridColor);
        }

        // Dibujar la etiqueta numérica Y
        if (graph->bShowLabels && fontId >= 0) {
            char valStr[12];
            snprintf(valStr, sizeof(valStr), "%d", gridValue);
            
            int16_t textX = graph->pos.x + 5;
            int16_t textY = gy + (textH / 3);
            
            // Prevenir que el texto superior e inferior se salga de los límites de la gráfica
            if (i == 0) textY = graph->pos.y + 15; 
            if (i == (graph->gridLinesY + 1)) textY = graph->pos.y + graph->size.height - textH / 3;

            FontEngine_DrawString(pBuf, fontId, textX, textY, valStr, graph->textColor, ALIGN_LEFT, 1);
        }
    }

    // =========================================================
    // 4. Cuadrícula y Etiquetas (Eje X)
    // =========================================================
    int16_t stepX = graph->size.width / (graph->gridLinesX + 1);
    
    i = 0;
    for (; i <= (graph->gridLinesX + 1); i++) {
        
        int16_t gx = graph->pos.x + (i * stepX);

        // Dibujar la línea vertical
        if (i > 0 && i < (graph->gridLinesX + 1)) {
            gfx_drawFastVLine(pBuf, gx, graph->pos.y, graph->size.height, graph->gridColor);
        }

        // Dibujar la etiqueta numérica X
        if (graph->bShowXLabels && fontId >= 0 && graph->maxXValue > 0) {
            
            // Interpolar el valor en segundos
            float xVal = (graph->maxXValue * i) / (graph->gridLinesX + 1);
            
            char xLabelBuf[16];
            snprintf(xLabelBuf, sizeof(xLabelBuf), "%.0f", xVal); // Formato sin decimales
            
            // Alineación inteligente para los bordes
            gfx_Align_e align = ALIGN_CENTER;
            if (i == 0) align = ALIGN_LEFT;
            else if (i == (graph->gridLinesX + 1)) align = ALIGN_RIGHT;

            // Dibujar 8 píxeles por debajo del piso de la gráfica
            FontEngine_DrawString(pBuf, fontId, 
                                  gx, graph->pos.y + graph->size.height + 2, 
                                  xLabelBuf, graph->textColor, 
                                  (gfx_Align_e)(align | ALIGN_TOP), 1);
        }
    }

    graph->bIsDirty = false;
}

void UpdateDisplayWithGraphOverlay(gfx_Graph *graph, gfx_Graph *graph2) {
  // 1. Iniciamos una nueva Display List
  API_LIB_BeginCoProList();
  API_CMD_DLSTART();
  API_CLEAR_COLOR_RGB(0, 0, 0);
  API_CLEAR(1, 1, 1);
  API_COLOR_RGB(255, 255, 255);

  // =======================================================
  // CAPA 1: TU BITMAP DE SOFTWARE (Exactamente como lo tenías)
  // =======================================================
  uint16_t ui16Width = 800;
  uint16_t ui16Height = 480;
  API_BITMAP_HANDLE(0);
  API_BITMAP_SOURCE(RAM_G); // La base de tu framebuffer
  
  uint16_t BytesPerPixel = 2;
  uint16_t ui16Stride = ui16Width * BytesPerPixel;
  API_BITMAP_LAYOUT(RGB565, ui16Stride, ui16Height);
  API_BITMAP_LAYOUT_H(ui16Stride >> 10, ui16Height >> 9);
  API_BITMAP_SIZE(NEAREST, BORDER, BORDER, ui16Width, ui16Height);
  API_BITMAP_SIZE_H(ui16Width >> 9, ui16Height >> 9);

  API_BEGIN(BITMAPS);
  API_VERTEX2II(0, 0, 0, 0); // Dibuja todo el fondo
  API_END(); // Cerramos el dibujo de bitmaps

  // =======================================================
  // CAPA 2: GRÁFICA 1
  // =======================================================
  if (graph != NULL && graph->data != NULL && graph->maxPoints > 1) {
      int32_t rangeY = (int32_t)graph->maxY - (int32_t)graph->minY;
      if (rangeY <= 0) rangeY = 1;

      API_BEGIN(LINE_STRIP); 
      API_LINE_WIDTH(graph->lineWidth * 16 / 2); 
      
      uint32_t color = 0x22AAAA; 
      API_COLOR_RGB((uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)color);

	  uint16_t i = 0;
      for (; i < graph->maxPoints; i++) {
          uint16_t dataIdx = (graph->head + i) % graph->maxPoints;
          int16_t val = graph->data[dataIdx];

          if (val < graph->minY) val = graph->minY;
          if (val > graph->maxY) val = graph->maxY;

          uint16_t reverse_i = (graph->maxPoints - 1) - i;
          int16_t px = (int16_t)((int32_t)graph->pos.x + ((int32_t)reverse_i * (graph->size.width - 1)) / (graph->maxPoints - 1));
          int16_t py = (int16_t)((int32_t)graph->pos.y + (graph->size.height - 1) - (((int32_t)(val - graph->minY) * (graph->size.height - 1)) / rangeY));

          API_VERTEX2F(px * 16, py * 16);
      }
      
      API_END(); 
      
      // Vaciamos el FIFO de comandos (RAM_CMD) hacia la RAM_DL 
  }
  
  API_LIB_EndCoProList(); 
  
  API_LIB_AwaitCoProEmpty();

  // 3. Volvemos a abrir la ráfaga SPI para continuar inyectando comandos
  API_LIB_BeginCoProList();
  // =======================================================
  // CAPA 3: GRÁFICA 2
  // =======================================================
  if (graph2 != NULL && graph2->data != NULL && graph2->maxPoints > 1) {
      int32_t rangeY = (int32_t)graph2->maxY - (int32_t)graph2->minY;
      if (rangeY <= 0) rangeY = 1;

      API_BEGIN(LINE_STRIP); 
      API_LINE_WIDTH(graph2->lineWidth * 16 / 2); 
      
      uint32_t color = 0xAA2222; 
      API_COLOR_RGB((uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)color);

	  uint16_t i = 0;
      for (; i < graph2->maxPoints; i++) {
          uint16_t dataIdx = (graph2->head + i) % graph2->maxPoints;
          int16_t val = graph2->data[dataIdx];

          if (val < graph2->minY) val = graph2->minY;
          if (val > graph2->maxY) val = graph2->maxY;

          uint16_t reverse_i = (graph2->maxPoints - 1) - i;
          int16_t px = (int16_t)((int32_t)graph2->pos.x + ((int32_t)reverse_i * (graph2->size.width - 1)) / (graph2->maxPoints - 1));
          int16_t py = (int16_t)((int32_t)graph2->pos.y + (graph2->size.height - 1) - (((int32_t)(val - graph2->minY) * (graph2->size.height - 1)) / rangeY));

          API_VERTEX2F(px * 16, py * 16);
      }
      
      API_END(); 
  }

  // 3. Cerramos y hacemos el SWAP en el hardware
  API_DISPLAY();
  API_CMD_SWAP();
  API_LIB_EndCoProList();

  // Esperamos a que todo el frame (SWAP incluido) sea procesado
  API_LIB_AwaitCoProEmpty();
}

void gfx_GraphAddPoint(gfx_Graph *graph, float newValue) {
    if(!graph || !graph->data) return;

    // Sobrescribimos el dato más viejo en la posición 'head'
    graph->data[graph->head] = newValue;
    // Avanzamos el 'head' circularmente
    graph->head = (graph->head + 1) % graph->maxPoints;

	graph->totalPointsAdded++;

	//UpdateDisplayWithGraphOverlay(graph);
    // Le avisamos al motor que debe redibujar la gráfica en el próximo frame
    //graph->bIsDirty = true;
	graph->bEVEDirty = true;
}
/*
void gfx_GraphRenderEVEComponents(gfx_Graph *graph) {
  if (graph != NULL && graph->data != NULL && graph->maxPoints > 1) {
      int32_t rangeY = (int32_t)graph->maxY - (int32_t)graph->minY;
      if (rangeY <= 0) rangeY = 1;

      API_BEGIN(LINE_STRIP); 
      API_LINE_WIDTH(graph->lineWidth * 16 / 2); 
      
      // uint32_t color = 0x22AAAA; 
      // API_COLOR_RGB((uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)color);
      uint16_t c = graph->lineColor;
      API_COLOR_RGB((uint8_t)((c >> 8) & 0xF8), (uint8_t)((c >> 3) & 0xFC), (uint8_t)((c << 3) & 0xF8));

	  uint16_t i = 0;
      for (; i < graph->maxPoints; i++) {
          uint16_t dataIdx = (graph->head + i) % graph->maxPoints;
          int16_t val = graph->data[dataIdx];

          if (val < graph->minY) val = graph->minY;
          if (val > graph->maxY) val = graph->maxY;

          // uint16_t reverse_i = (graph->maxPoints - 1) - i;
          uint16_t reverse_i = i;
          int16_t px = (int16_t)((int32_t)graph->pos.x + ((int32_t)reverse_i * (graph->size.width - 1)) / (graph->maxPoints - 1));
          int16_t py = (int16_t)((int32_t)graph->pos.y + (graph->size.height - 1) - (((int32_t)(val - graph->minY) * (graph->size.height - 1)) / rangeY));

          API_VERTEX2F(px * 16, py * 16);
      }
      
      API_END(); 
      // Vaciamos el FIFO de comandos (RAM_CMD) hacia la RAM_DL 
  }
} */

/*
void gfx_GraphRenderEVEComponents(gfx_Graph *graph) {
    if (graph != NULL && graph->data != NULL && graph->maxPoints > 1 && graph->isTraceVisible) {
        // 1. Calculate how many points we actually have to draw right now
        uint16_t pointsToDraw = (graph->totalPointsAdded < graph->maxPoints) ? 
                                 graph->totalPointsAdded : graph->maxPoints;

        // If we don't have at least 2 points, we can't draw a line strip
        if (pointsToDraw < 2) return;

        int32_t rangeY = (int32_t)graph->maxY - (int32_t)graph->minY;
        if (rangeY <= 0) rangeY = 1;

        API_BEGIN(LINE_STRIP); 
        API_LINE_WIDTH(graph->lineWidth * 16 / 2); 
        
        uint16_t c = graph->lineColor;
        API_COLOR_RGB((uint8_t)((c >> 8) & 0xF8), (uint8_t)((c >> 3) & 0xFC), (uint8_t)((c << 3) & 0xF8));

        // 2. Iterate only over the points we actually have
        uint16_t i = 0;
        for (; i < pointsToDraw; i++) {
            
            uint16_t dataIdx;
            int16_t px;

            // --- PHASE 1: FILLING ---
            if (graph->totalPointsAdded < graph->maxPoints) {
                // Read straight from index 0 to totalPointsAdded
                dataIdx = i; 
                
                // Map 'i' proportionally across the screen. 
                // Since pointsToDraw is less than maxPoints, it will draw from left, 
                // moving rightward, leaving empty space on the right.
                // Notice we divide by (maxPoints - 1) to keep the physical spacing consistent.
                px = (int16_t)((int32_t)graph->pos.x + ((int32_t)i * (graph->size.width - 1)) / (graph->maxPoints - 1));
            } 
            
            // --- PHASE 2: SCROLLING ---
            else {
                // Calculate the oldest point in the ring buffer
                dataIdx = (graph->head + i) % graph->maxPoints;
                
                // i = 0 -> Far Left (Oldest)
                // i = maxPoints - 1 -> Far Right (Newest)
                px = (int16_t)((int32_t)graph->pos.x + ((int32_t)i * (graph->size.width - 1)) / (graph->maxPoints - 1));
            }

            int16_t val = graph->data[dataIdx];

            // Saturate limits
            if (val < graph->minY) val = graph->minY;
            if (val > graph->maxY) val = graph->maxY;

            // Calculate Y (Top-Down)
            int16_t py = (int16_t)((int32_t)graph->pos.y + (graph->size.height - 1) - (((int32_t)(val - graph->minY) * (graph->size.height - 1)) / rangeY));

            API_VERTEX2F(px * 16, py * 16);
        }
        
        API_END(); 
    }
} */

void gfx_GraphRenderEVEComponents(gfx_Graph *graph) {
    if (graph != NULL && graph->data != NULL && graph->maxPoints > 1 && graph->isTraceVisible) {
        // 1. Calcular cuántos puntos tenemos actualmente
        uint16_t pointsToDraw = (graph->totalPointsAdded < graph->maxPoints) ? 
                                 graph->totalPointsAdded : graph->maxPoints;

        // Si no hay al menos 2 puntos, no se puede trazar una línea
        if (pointsToDraw < 2) return;

        int32_t rangeY = (int32_t)graph->maxY - (int32_t)graph->minY;
        if (rangeY <= 0) rangeY = 1;

        // Configuración de Hardware EVE
        uint16_t c = graph->lineColor;
        API_COLOR_RGB((uint8_t)((c >> 8) & 0xF8), (uint8_t)((c >> 3) & 0xFC), (uint8_t)((c << 3) & 0xF8));
        API_LINE_WIDTH(graph->lineWidth * 16 / 2); 

        // --- MÁQUINA DE ESTADOS PARA LÍNEA SEGMENTADA ---
        bool isDrawing = true;
        uint16_t dashCount = 0;
        
        // Valores por defecto seguros por si activas bIsDashed pero olvidas setear los largos
        uint8_t dLen = (graph->dashLen > 0) ? graph->dashLen : 5;
        uint8_t sLen = (graph->spaceLen > 0) ? graph->spaceLen : 5;

        // Si es una línea continua estándar, abrimos la directiva una sola vez por eficiencia
        if (!graph->bIsDashed) {
            API_BEGIN(LINE_STRIP); 
        }

        // 2. Iterar sobre los puntos
        uint16_t i = 0;
        for (; i < pointsToDraw; i++) {
            
            uint16_t dataIdx;
            int16_t px;

            // --- FASE 1 Y 2: CÁLCULO DE POSICIÓN X ---
            if (graph->totalPointsAdded < graph->maxPoints) {
                dataIdx = i; 
                px = (int16_t)((int32_t)graph->pos.x + ((int32_t)i * (graph->size.width - 1)) / (graph->maxPoints - 1));
            } 
            else {
                dataIdx = (graph->head + i) % graph->maxPoints;
                px = (int16_t)((int32_t)graph->pos.x + ((int32_t)i * (graph->size.width - 1)) / (graph->maxPoints - 1));
            }

            // --- CÁLCULO DE POSICIÓN Y ---
            int16_t val = graph->data[dataIdx];
            if (val < graph->minY) val = graph->minY;
            if (val > graph->maxY) val = graph->maxY;

            int16_t py = (int16_t)((int32_t)graph->pos.y + (graph->size.height - 1) - (((int32_t)(val - graph->minY) * (graph->size.height - 1)) / rangeY));

            // --- INYECCIÓN DE VÉRTICES (HARDWARE) ---
            if (graph->bIsDashed) {
                if (isDrawing) {
                    if (dashCount == 0) {
                        // Iniciar un nuevo guión
                        API_BEGIN(LINE_STRIP);
                        API_VERTEX2F(px * 16, py * 16); // Vértice ancla
                    } else {
                        // Continuar el guión actual
                        API_VERTEX2F(px * 16, py * 16);
                    }

                    dashCount++;
                    if (dashCount >= dLen) {
                        API_END(); // Cerrar el guión en la GPU
                        isDrawing = false;
                        dashCount = 0;
                    }
                } else {
                    dashCount++;
                    if (dashCount >= sLen) {
                        isDrawing = true; // El espacio vacío terminó
                        dashCount = 0;
                    }
                }
            } 
            else {
                // Modo continuo normal
                API_VERTEX2F(px * 16, py * 16);
            }
        }
        
        // Cerrar la primitiva al salir del bucle (vital para no corromper la CoPro list de EVE)
        if (!graph->bIsDashed || isDrawing) {
            API_END(); 
        }
    }
}

void gfx_MultiGraphAddData(gfx_MultiGraph *graph, uint8_t traceIndex, int16_t newValue) {
    // Validaciones de seguridad
    if (!graph || graph->maxPoints == 0) return;
    if (traceIndex >= graph->activeTraces || graph->dataSets[traceIndex] == NULL) return;

    // 1. Obtenemos la cabecera actual específica de este trazo
    uint16_t currentHead = graph->heads[traceIndex];

    // 2. Inyectamos el nuevo valor
    graph->dataSets[traceIndex][currentHead] = newValue;

    // 3. Avanzamos ÚNICAMENTE la cabecera de este trazo
    graph->heads[traceIndex] = (currentHead + 1) % graph->maxPoints;

	graph->totalPointsAdded[traceIndex]++;

	graph->bEVEDirty = true;
}

void gfx_drawMultiGraph(pixel16_t *pBuf, gfx_MultiGraph *graph) {
    if (!pBuf || !graph) return;

    // 1. Draw Background
    gfx_fillRoundRect(pBuf, graph->pos.x, graph->pos.y, 
                      graph->size.width, graph->size.height, 4, graph->bgColor);

    // =========================================================
    // 2. Draw Grid and Labels
    // =========================================================
    int8_t fontId = -1;
    uint16_t textW = 0, textH = 0;
    
    if (graph->bShowLabels) {
        fontId = Theme_ResolveFontId(graph->typo); 
        if (fontId >= 0) {
            FontEngine_GetStringDimensions("0", fontId, &textW, &textH, 1); 
        }
    }

    // Y-Axis Horizontal Lines and Texts
    int16_t stepY = graph->size.height / (graph->gridLinesY + 1);
    int16_t valueStep = (graph->maxY - graph->minY) / (graph->gridLinesY + 1);

    int i = 0;
    for (; i <= (graph->gridLinesY + 1); i++) {
        int16_t gy = graph->pos.y + (i * stepY);
        int16_t gridValue = graph->maxY - (i * valueStep); 

        if (i > 0 && i < (graph->gridLinesY + 1)) {
            gfx_drawFastHLine(pBuf, graph->pos.x, gy, graph->size.width, graph->gridColor);
        }

        if (graph->bShowLabels && fontId >= 0) {
            char valStr[12];
            snprintf(valStr, sizeof(valStr), "%d", gridValue); 
            
            int16_t textX = graph->pos.x + 5;
            int16_t textY = gy + (textH / 3);
            
            if (i == 0) textY = graph->pos.y + 15; 
            if (i == (graph->gridLinesY + 1)) textY = graph->pos.y + graph->size.height - textH / 3;

            // Use your engine's text drawing function
            FontEngine_DrawString(pBuf, fontId, textX, textY, valStr, graph->textColor, ALIGN_LEFT, 1);
        }
    }

    // X-Axis Vertical Lines
    if (graph->gridLinesX > 0) {
        int16_t stepX = graph->size.width / (graph->gridLinesX + 1);
		int i = 1;
        for (; i <= graph->gridLinesX; i++) {
            int16_t gx = graph->pos.x + (i * stepX);
            gfx_drawFastVLine(pBuf, gx, graph->pos.y, graph->size.height, graph->gridColor);
        }
    }

    graph->bIsDirty = false;
}

void gfx_MultigraphRenderEVEComponents(gfx_MultiGraph *graph) {

    if (graph != NULL && graph->activeTraces > 0 && graph->maxPoints > 1) {
        int32_t rangeY = (int32_t)graph->maxY - (int32_t)graph->minY;
        if (rangeY <= 0) rangeY = 1;

        uint8_t trace = 0;
        for (; trace < graph->activeTraces; trace++) {
            
            // Ignorar si no hay datos
            if (graph->dataSets[trace] == NULL) continue;

            // 1. Calcular cuántos puntos tiene *esta* traza actualmente
            uint32_t totalPts = graph->totalPointsAdded[trace];
            uint16_t availablePts = (totalPts < graph->maxPoints) ? totalPts : graph->maxPoints;

            // Si no hay al menos 2 puntos, no se puede trazar una línea
            if (availablePts < 2) continue;

            // 2. Downsampling dinámico: Nunca dibujar más puntos que el ancho en píxeles
            uint16_t renderPts = availablePts; 
            if (renderPts > graph->size.width) {
                renderPts = graph->size.width;
            }

            API_BEGIN(LINE_STRIP); 
            API_LINE_WIDTH(graph->lineWidth * 16 / 2); 
            
            uint16_t c = graph->lineColors[trace];
            API_COLOR_RGB((uint8_t)((c >> 8) & 0xF8), (uint8_t)((c >> 3) & 0xFC), (uint8_t)((c << 3) & 0xF8));
            
            uint16_t currentTraceHead = graph->heads[trace];
            uint16_t i = 0;
            
            for (; i < renderPts; i++) {
                
                // dataOffset interpola 'i' para abarcar todos los puntos disponibles
                uint16_t dataOffset = (i * (availablePts - 1)) / (renderPts - 1);
                
                uint16_t dataIdx;
                int16_t px;

                // ==========================================
                // FASE 1: LLENADO (Sweep)
                // ==========================================
                if (totalPts < graph->maxPoints) {
                    // Leemos linealmente desde el índice 0
                    dataIdx = dataOffset; 
                    
                    // La posición X escala con maxPoints para que la señal crezca de izq a der
                    px = (int16_t)((int32_t)graph->pos.x + ((int32_t)dataOffset * (graph->size.width - 1)) / (graph->maxPoints - 1));
                } 
                // ==========================================
                // FASE 2: DESPLAZAMIENTO (Scroll)
                // ==========================================
                else {
                    // Leemos usando el buffer circular, partiendo del dato más viejo (Head)
                    dataIdx = (currentTraceHead + dataOffset) % graph->maxPoints;
                    
                    // La posición X ocupa todo el ancho disponible
                    px = (int16_t)((int32_t)graph->pos.x + ((int32_t)i * (graph->size.width - 1)) / (renderPts - 1));
                }

                int16_t val = graph->dataSets[trace][dataIdx];

                if (val < graph->minY) val = graph->minY;
                if (val > graph->maxY) val = graph->maxY;

                int16_t py = (int16_t)((int32_t)graph->pos.y + (graph->size.height - 1) - (((int32_t)(val - graph->minY) * (graph->size.height - 1)) / rangeY));

                API_VERTEX2F(px * 16, py * 16);
            }
            
            API_END(); // Fin de la traza actual

            // =======================================================
            // THE SPI BURST FIX (Pausa entre trazas)
            // =======================================================
            if (trace < (graph->activeTraces - 1)) {
                API_LIB_EndCoProList(); 
                API_LIB_AwaitCoProEmpty();
                API_LIB_BeginCoProList();
            }
        }
    }
}

void gfx_GraphCursorRenderEVE(gfx_GraphCursor *cursor) {
    if (cursor == NULL || !cursor->isVisible || cursor->parent == NULL) {
        return;
    }

    // 1. Configuramos el estilo de la línea
    API_BEGIN(LINES);
    API_LINE_WIDTH(cursor->lineWidth * 16 / 2); // EVE requiere subpíxeles
    
    
    // Aplicar transparencia (Ej: 150 = semi-transparente)
    API_COLOR_A(cursor->alpha); 

    const gfx_Graph *g = cursor->parent;

    // ==========================================
    // CURSOR VERTICAL (Mide tiempo / Eje X)
    // ==========================================
    if (cursor->type == CURSOR_VERTICAL || cursor->type == CURSOR_CROSSHAIR) {
        // Saturación: Evitar que el cursor se salga de la gráfica
        int16_t drawX = cursor->relX;
        if (drawX < 0) drawX = 0;
        if (drawX > g->size.width) drawX = g->size.width;

        int16_t absX = g->pos.x + drawX;

        API_VERTEX2F(absX * 16, g->pos.y * 16);                               // Punto superior
        API_VERTEX2F(absX * 16, (g->pos.y + g->size.height) * 16);            // Punto inferior
    }

    // ==========================================
    // CURSOR HORIZONTAL (Mide voltaje / Eje Y)
    // ==========================================
    if (cursor->type == CURSOR_HORIZONTAL || cursor->type == CURSOR_CROSSHAIR) {
        // Saturación
        int16_t drawY = cursor->relY;
        if (drawY < 0) drawY = 0;
        if (drawY > g->size.height) drawY = g->size.height;

        int16_t absY = g->pos.y + drawY;

        API_VERTEX2F(g->pos.x * 16, absY * 16);                               // Punto izquierdo
        API_VERTEX2F((g->pos.x + g->size.width) * 16, absY * 16);             // Punto derecho
    }

    API_END();
    
    // Restaurar el canal alfa a 255 (sólido) para que no afecte a los widgets que se dibujen después
    API_COLOR_A(255); 
}

float gfx_GraphCursorGetValue(gfx_GraphCursor *cursor) {
    if (cursor == NULL || cursor->parent == NULL || cursor->parent->maxPoints < 2) return 0;
    
    const gfx_Graph *g = cursor->parent;
    
    // 1. Mapear la posición del pixel (relX) al índice del arreglo [0 a maxPoints-1]
    uint16_t pointIndex = (cursor->relX * (g->maxPoints - 1)) / g->size.width;

    uint16_t dataIdx;

    // 2. Aplicar la lógica inversa de Sweep/Scroll que diseñaste
    if (g->totalPointsAdded < g->maxPoints) {
        // Fase de Llenado: El índice es directo
        dataIdx = pointIndex;
        // Si el cursor está en un área vacía (a la derecha de los datos), devolver 0 o el último dato
        if (dataIdx >= g->totalPointsAdded) return 0; 
    } else {
        // Fase de Desplazamiento: El índice considera el buffer circular
        dataIdx = (g->head + pointIndex) % g->maxPoints;
    }

    return g->data[dataIdx];
}

float gfx_GraphGetAverageBetweenCursors(const gfx_GraphCursor *c1, const gfx_GraphCursor *c2) {
    // 1. Validaciones de seguridad
    if (!c1 || !c2 || !c1->parent || c1->parent != c2->parent) {
        return 0.0f; // Deben pertenecer a la misma gráfica
    }

    const gfx_Graph *g = c1->parent;
    if (g->maxPoints < 2 || g->totalPointsAdded == 0) {
        return 0.0f;
    }

    // 2. Determinar cuál cursor está a la izquierda (Start) y cuál a la derecha (End)
    // Esto permite al usuario cruzar los cursores sin romper el cálculo
    int16_t leftX = (c1->relX < c2->relX) ? c1->relX : c2->relX;
    int16_t rightX = (c1->relX > c2->relX) ? c1->relX : c2->relX;

    // 3. Mapear X (píxeles) a índices lógicos (0 a maxPoints - 1)
    uint16_t startIdx = (leftX * (g->maxPoints - 1)) / g->size.width;
    uint16_t endIdx = (rightX * (g->maxPoints - 1)) / g->size.width;

    // 4. Proteger contra áreas vacías si la gráfica aún se está llenando (Fase de Sweep)
    if (g->totalPointsAdded < g->maxPoints) {
        if (startIdx >= g->totalPointsAdded) {
            return 0.0f; // Ambos cursores están en el espacio vacío a la derecha
        }
        if (endIdx >= g->totalPointsAdded) {
            endIdx = g->totalPointsAdded - 1; // Limitar al dato más nuevo
        }
    }

    // 5. Acumular los valores
    // NOTA: Usamos int32_t para 'sum' para evitar un overflow aritmético. 
    // Si tienes 800 puntos a su valor máximo de 16-bits (32,767), la suma es 
    // ~26 millones, que cabe sobradamente en los 2 billones límite de un int32_t.
    float sum = 0; 
    float count = (endIdx - startIdx) + 1;

	uint16_t i = startIdx;
    for (; i <= endIdx; i++) {
        uint16_t dataIdx;
        
        // Aplicar la lógica del buffer
        if (g->totalPointsAdded < g->maxPoints) {
            dataIdx = i; // Lectura lineal
        } else {
            dataIdx = (g->head + i) % g->maxPoints; // Lectura circular (Ring Buffer)
        }
        
        sum += g->data[dataIdx];
    }

    // 6. Calcular y devolver el promedio
    return sum / count;
}

float gfx_GraphGetMaxBetweenCursors(const gfx_GraphCursor *c1, const gfx_GraphCursor *c2) {
    // 1. Validaciones de seguridad
    if (!c1 || !c2 || !c1->parent || c1->parent != c2->parent) {
        return 0; 
    }

    const gfx_Graph *g = c1->parent;
    if (g->maxPoints < 2 || g->totalPointsAdded == 0) {
        return 0;
    }

    // 2. Determinar Izquierda (Start) y Derecha (End)
    int16_t leftX = (c1->relX < c2->relX) ? c1->relX : c2->relX;
    int16_t rightX = (c1->relX > c2->relX) ? c1->relX : c2->relX;

    // 3. Mapear a índices
    uint16_t startIdx = (leftX * (g->maxPoints - 1)) / g->size.width;
    uint16_t endIdx = (rightX * (g->maxPoints - 1)) / g->size.width;

    // 4. Proteger contra áreas vacías (Fase Sweep)
    if (g->totalPointsAdded < g->maxPoints) {
        if (startIdx >= g->totalPointsAdded) {
            return 0; 
        }
        if (endIdx >= g->totalPointsAdded) {
            endIdx = g->totalPointsAdded - 1; 
        }
    }

    // 5. Inicializar maxVal con el primer dato real dentro del rango
    uint16_t firstDataIdx;
    if (g->totalPointsAdded < g->maxPoints) {
        firstDataIdx = startIdx;
    } else {
        firstDataIdx = (g->head + startIdx) % g->maxPoints;
    }
    
    float maxVal = g->data[firstDataIdx];

    // 6. Iterar buscando un valor mayor
    // Comenzamos desde startIdx + 1 porque ya leímos el primer valor
	uint16_t i = startIdx + 1;
    for (; i <= endIdx; i++) {
        uint16_t dataIdx;
        
        if (g->totalPointsAdded < g->maxPoints) {
            dataIdx = i; 
        } else {
            dataIdx = (g->head + i) % g->maxPoints; 
        }
        
        if (g->data[dataIdx] > maxVal) {
            maxVal = g->data[dataIdx];
        }
    }

    return maxVal;
}

float gfx_GraphGetMinBetweenCursors(const gfx_GraphCursor *c1, const gfx_GraphCursor *c2) {
    // 1. Validaciones de seguridad
    if (!c1 || !c2 || !c1->parent || c1->parent != c2->parent) {
        return 0; 
    }

    const gfx_Graph *g = c1->parent;
    if (g->maxPoints < 2 || g->totalPointsAdded == 0) {
        return 0;
    }

    // 2. Determinar Izquierda (Start) y Derecha (End)
    int16_t leftX = (c1->relX < c2->relX) ? c1->relX : c2->relX;
    int16_t rightX = (c1->relX > c2->relX) ? c1->relX : c2->relX;

    // 3. Mapear a índices
    uint16_t startIdx = (leftX * (g->maxPoints - 1)) / g->size.width;
    uint16_t endIdx = (rightX * (g->maxPoints - 1)) / g->size.width;

    // 4. Proteger contra áreas vacías (Fase Sweep)
    if (g->totalPointsAdded < g->maxPoints) {
        if (startIdx >= g->totalPointsAdded) {
            return 0; 
        }
        if (endIdx >= g->totalPointsAdded) {
            endIdx = g->totalPointsAdded - 1; 
        }
    }

    // 5. Inicializar maxVal con el primer dato real dentro del rango
    uint16_t firstDataIdx;
    if (g->totalPointsAdded < g->maxPoints) {
        firstDataIdx = startIdx;
    } else {
        firstDataIdx = (g->head + startIdx) % g->maxPoints;
    }
    
    float maxVal = g->data[firstDataIdx];

    // 6. Iterar buscando un valor mayor
    // Comenzamos desde startIdx + 1 porque ya leímos el primer valor
	uint16_t i = startIdx + 1;
    for (; i <= endIdx; i++) {
        uint16_t dataIdx;
        
        if (g->totalPointsAdded < g->maxPoints) {
            dataIdx = i; 
        } else {
            dataIdx = (g->head + i) % g->maxPoints; 
        }
        
        if (g->data[dataIdx] < maxVal) {
            maxVal = g->data[dataIdx];
        }
    }

    return maxVal;
}

void gfx_ImageRenderEVEComponents(gfx_Image *img) {
    if (!img || img->size.width == 0 || img->size.height == 0) return;

    // 1. Configurar el contexto de la imagen en el Handle 1
    API_BITMAP_HANDLE(1);
    API_BITMAP_SOURCE(img->ramgAddress);

    uint16_t stride = img->size.width * 2; // RGB565 usa 2 bytes por pixel

    API_BITMAP_LAYOUT(RGB565, stride, img->size.height);
    API_BITMAP_LAYOUT_H(stride >> 10, img->size.height >> 9);

    uint16_t drawnW = img->size.width * img->scale;
    uint16_t drawnH = img->size.height * img->scale;

    API_BITMAP_SIZE(NEAREST, BORDER, BORDER, drawnW, drawnH);
    API_BITMAP_SIZE_H(drawnW >> 9, drawnH >> 9);

    // 2. Aplicar matriz de escalado si es necesario
    if (img->scale > 1) {
        int32_t s32ScaleFactor = 65536 * img->scale; 
        API_CMD_LOADIDENTITY();
        API_CMD_SCALE(s32ScaleFactor, s32ScaleFactor);
        API_CMD_SETMATRIX();
    }

    // 3. Dibujar el bitmap usando el Handle 1
    API_BEGIN(BITMAPS);
    
    // Nota: VERTEX2II usa parámetros (x, y, handle, cell)
    API_VERTEX2II(img->pos.x, img->pos.y, 1, 0); 
    
    API_END();

    // 4. Restaurar la matriz para no afectar a otros widgets de hardware
    if (img->scale > 1) {
        API_CMD_LOADIDENTITY();
        API_CMD_SETMATRIX();
    }
}

bool gfx_ImageLoadPNG(gfx_Image *img, const uint8_t *pngData, uint32_t dataSize, uint32_t targetRamGAddr) {
    if (!img || !pngData || dataSize == 0) return false;

    // 1. Decodificar el PNG apuntando a la zona segura de la RAM_G
    API_LIB_BeginCoProList();
    API_CMD_LOADIMAGE(targetRamGAddr, 0); 
    API_LIB_EndCoProList();
    
    // Inyectar los bytes del archivo PNG
    API_LIB_WriteDataToCMD(pngData, dataSize);
    API_LIB_AwaitCoProEmpty();

    // 2. Extraer las propiedades calculadas por EVE
    API_LIB_BeginCoProList();
    API_CMD_GETPROPS(0, 0, 0);
    API_LIB_EndCoProList();
    API_LIB_AwaitCoProEmpty();

    uint16_t REG_CMD_WRITE_OFFSET = EVE_MemRead16(REG_CMD_WRITE);
    
    // El ancho (width) se almacena 8 bytes atrás en la RAM_CMD
    uint16_t ParameterAddr = ((REG_CMD_WRITE_OFFSET - 8) & 4095);
    img->size.width = EVE_MemRead16((RAM_CMD + ParameterAddr));

    // El alto (height) se almacena 4 bytes atrás
    ParameterAddr = ((REG_CMD_WRITE_OFFSET - 4) & 4095);
    img->size.height = EVE_MemRead16((RAM_CMD + ParameterAddr));

    // Guardamos la dirección asignada
    img->ramgAddress = targetRamGAddr;
    
    // Valor por defecto
    if(img->scale == 0) img->scale = 1; 

    return true;
}

// En gfx_button.c
gfx_DirtyRect gfx_ButtonProcessState(gfx_Button *btn) {
    gfx_DirtyRect rect = {0, 0, 0, 0, false};

    if (btn->bIsDirty) {
        // 1. Aquí actualizas la SDRAM (dibujar el botón presionado/suelto)
        
        // 2. Extraemos el Bounding Box (bbox)
        rect.x = btn->pos.x - btn->borderWidth;
        rect.y = btn->pos.y - btn->borderWidth;
        rect.w = btn->size.width + 2 * btn->borderWidth;
        rect.h = btn->size.height + 2 *btn->borderWidth;
        rect.isDirty = true;
        
        // 3. Limpiamos la bandera del widget
        btn->bIsDirty = false; 
    }
    return rect;
}

gfx_DirtyRect gfx_LabelProcessState(gfx_Label *lbl) {
    gfx_DirtyRect rect = {0, 0, 0, 0, false};
    
    if (lbl->bIsDirty && lbl->text != NULL) {
        // 1. Dibuja primero para actualizar la SDRAM
        
        // 2. Obtener métricas reales de la fuente
        uint8_t fontId = Theme_ResolveFontId(lbl->typo);
        uint16_t totalWidth = 0, totalHeight = 0;
        FontEngine_GetStringDimensions(lbl->text, fontId, &totalWidth, &totalHeight, 1);

        // Extraemos el "Ascent" (La altura desde la línea base hasta la cima de la letra)
        BDF_Font_t *sFont = &g_FontCache[fontId].bdfData;
        int16_t ascent = sFont->yAdvance + sFont->globalYOffset; 

        // 3. Calcular la esquina Top-Left real (startX, startY)
        int16_t startX = lbl->pos.x;
        if (lbl->alignment & ALIGN_RIGHT) {
            startX = lbl->pos.x - totalWidth;
        } else if (lbl->alignment & ALIGN_HCENTER) {
            startX = lbl->pos.x - (totalWidth / 2);
        }

        int16_t startY = lbl->pos.y;
        if (lbl->alignment & ALIGN_TOP) {
            startY = lbl->pos.y;
        } else if (lbl->alignment & ALIGN_VCENTER) {
            // Si es VCENTER, la mitad del texto queda arriba de Y
            startY = lbl->pos.y - (totalHeight / 2);
        } else if (lbl->alignment & ALIGN_BOTTOM) {
            // Si es BOTTOM, todo el texto queda arriba de Y
            startY = lbl->pos.y - totalHeight;
        } else {
            // DEFAULT (Línea Base): ¡Aquí estaba tu misterioso -20!
            // Subimos exactamente la medida del 'ascent' de esta fuente
            startY = lbl->pos.y - ascent;
        }

        // Añadimos un pequeño margen por antialiasing / sangrado
        startX -= 2;
        startY -= 2;
        totalWidth += 4;
        totalHeight += 4;

        // 4. Calcular el "Union Rect" (Crucial para no dejar basura gráfica)
        if (lbl->oldSize.width > 0) {
            // Encontrar la coordenada más a la izquierda y más arriba
            rect.x = (startX < lbl->oldPos.x) ? startX : lbl->oldPos.x;
            rect.y = (startY < lbl->oldPos.y) ? startY : lbl->oldPos.y;
            
            // Encontrar la coordenada más a la derecha y más abajo
            int16_t rightEdge = ((startX + totalWidth) > (lbl->oldPos.x + lbl->oldSize.width)) 
                                ? (startX + totalWidth) : (lbl->oldPos.x + lbl->oldSize.width);
            int16_t bottomEdge = ((startY + totalHeight) > (lbl->oldPos.y + lbl->oldSize.height)) 
                                 ? (startY + totalHeight) : (lbl->oldPos.y + lbl->oldSize.height);
            
            rect.w = rightEdge - rect.x;
            rect.h = bottomEdge - rect.y;
        } else {
            // Primera vez dibujado
            rect.x = startX;
            rect.y = startY;
            rect.w = totalWidth;
            rect.h = totalHeight;
        }

        // 5. Guardar métricas actuales como "viejas" para el próximo refresco
        lbl->oldPos.x = startX;
        lbl->oldPos.y = startY;
        lbl->oldSize.width = totalWidth;
        lbl->oldSize.height = totalHeight;

        rect.isDirty = true;
        lbl->bIsDirty = false;
    }
    
    return rect;
}

gfx_DirtyRect gfx_SliderProcessState(gfx_Slider *sld) {
	gfx_DirtyRect rect = {0};

	if(sld->bIsDirty) {
        rect.x = sld->pos.x;
        rect.y = sld->pos.y;
        rect.w = sld->size.width;
        rect.h = sld->size.height;
        rect.isDirty = true;
        
        // 3. Limpiamos la bandera del widget
        sld->bIsDirty = false; 
		rect.isDirty = true;
	}

	return rect;
}

gfx_DirtyRect gfx_GraphOverlayProcessState(gfx_GraphOverlay *ovl) {
    gfx_DirtyRect rect = {0, 0, 0, 0, false};
    
    if (ovl->bIsDirty) {
        // Actualizamos los pixeles en memoria SDRAM
        //gfx_drawGraphOverlay(g_pDrawingBuffer, ovl); 
        
        rect.x = ovl->pos.x;
        rect.y = ovl->pos.y;
        rect.w = ovl->size.width;
        rect.h = ovl->size.height;
        rect.isDirty = true;
        
        ovl->bIsDirty = false;
    }
    
    return rect;
}

gfx_DirtyRect gfx_ListViewProcessState(gfx_ListView *view) {
    gfx_DirtyRect rect = {0, 0, 0, 0, false};
    
    if (view->bIsDirty) {
        // Actualizamos los pixeles en memoria SDRAM
        
        rect.x = view->pos.x;
        rect.y = view->pos.y;
        rect.w = view->size.width;
        rect.h = view->size.height;
        rect.isDirty = true;
        
        view->bIsDirty = false;
    }
    
    return rect;
}

bool gfx_compositePartialFrame(gfx_Canvas *srf, pixel16_t *psPixelBuffer,
                               int16_t dirtyX, int16_t dirtyY, 
                               int16_t dirtyW, int16_t dirtyH) 
{
    if (srf == NULL || psPixelBuffer == NULL) return false;

    // 1. Borrar el fondo SOLO en el área sucia
    // OPTIMIZACIÓN: Sacamos multiplicaciones pesadas del bucle interno
    pixel16_t bgColor = (pixel16_t)g_pCurrentTheme->palette.background;
    int16_t y = dirtyY;
    for (; y < dirtyY + dirtyH; y++) {
        if (y >= 0 && y < LCD_HEIGHT) {
            uint32_t rowOffset = y * LCD_WIDTH; 

            int16_t x = dirtyX;
            for (; x < dirtyX + dirtyW; x++) {
                if (x >= 0 && x < LCD_WIDTH) {
                    psPixelBuffer[rowOffset + x] = bgColor;
                }
            }
        }
    }

    // 2. Iterar sobre todos los widgets de atrás hacia adelante (Z-Order)
    gfx_GenericWidgetNode *iter = srf->psWidgets;
    while (iter != NULL) {
        int16_t objX = 0, objY = 0, objW = 0, objH = 0;

        // Obtener el Bounding Box de este widget
        if (gfx_getWidgetBounds(&iter->sWidget, &objX, &objY, &objW, &objH)) {

            // 3. Detección de Colisiones (AABB - Axis-Aligned Bounding Box)
            bool isIntersecting = !(objX > dirtyX + dirtyW || 
                                    objX + objW < dirtyX ||
                                    objY > dirtyY + dirtyH || 
                                    objY + objH < dirtyY);

            // 4. Dibujar SOLO si el widget toca la zona que acabamos de borrar
            if (isIntersecting) {
                switch (iter->sWidget.eWidgetType) {
                    case WD_TYPE_BUTTON:
                        gfx_drawButton(psPixelBuffer, (gfx_Button *)iter->sWidget.pvWidget);
                        break;
                    case WD_TYPE_RECT:
                        gfx_drawRectangle(psPixelBuffer, (gfx_Rectangle *)iter->sWidget.pvWidget);
                        break;
                    case WD_TYPE_LABEL:
                        gfx_drawLabel(psPixelBuffer, (gfx_Label *)iter->sWidget.pvWidget);
						//iter = NULL;
						//continue;
                        break;
                    case WD_TYPE_SLIDER:
                        gfx_drawSlider(psPixelBuffer, (gfx_Slider *)iter->sWidget.pvWidget);
                        break;
                    // Gráficas suelen renderizarse por hardware en EVE, 
                    // pero si las haces en software, se quedan aquí:
                    case WD_TYPE_GRAPH:
                        gfx_drawGraph(psPixelBuffer, (gfx_Graph *)iter->sWidget.pvWidget);
                        break;
                    case WD_TYPE_MULTIGRAPH:
                        gfx_drawMultiGraph(psPixelBuffer, (gfx_MultiGraph *)iter->sWidget.pvWidget);
                        break;
					// En tu función de composición:
					case WD_TYPE_GRAPH_OVERLAY:
					    gfx_drawGraphOverlay(psPixelBuffer, (gfx_GraphOverlay *)iter->sWidget.pvWidget);
					    break;
					case WD_TYPE_LISTVIEW:
						gfx_drawListView(psPixelBuffer, (gfx_ListView *)iter->sWidget.pvWidget);
						break;
                    default:
                        break;
                }
            }
        }
        iter = iter->psNext;
    }

    return true;
}

/*
void gfx_drawGraphOverlay(pixel16_t *pBuf, gfx_GraphOverlay *ovl) {
    if (!pBuf || !ovl) return;

    // 1. Dibujar el fondo del panel flotante
    gfx_fillRoundRect(pBuf, ovl->pos.x, ovl->pos.y, 
                      ovl->size.width, ovl->size.height, 5, ovl->bgColor);

    // 2. Resolver la fuente y obtener la altura para espaciar los elementos
    int8_t fontId = Theme_ResolveFontId(ovl->typo);
    uint16_t textW = 0, textH = 0;
    
    if (fontId >= 0) {
        // Obtenemos la altura estándar de esta tipografía
        FontEngine_GetStringDimensions("0", fontId, &textW, &textH, 1); 
    }

    // 3. Iterar sobre las trazas activas y dibujarlas (Apiladas verticalmente)
    int16_t startY = ovl->pos.y + 10; // Margen superior
    int16_t startX = ovl->pos.x + 10; // Margen izquierdo
    uint16_t colorRectSize = textH * 0.8f; // El cuadradito de color escala con la letra

    int i = 0;
    for (; i < ovl->numTraces; i++) {
        if (!ovl->traces[i].isVisible) continue;

        int16_t currentY = startY + (i * (textH + 8)); // 8px de separación entre trazas

        // A. Dibujar el cuadrito de color indicador
        gfx_fillRoundRect(pBuf, startX, currentY, 
                          colorRectSize, colorRectSize, 2, ovl->traces[i].color);

        // B. Dibujar el texto del valor
        // Alineamos el texto usando ALIGN_VCENTER basándonos en el centro del cuadrito
        int16_t textY = currentY + (colorRectSize / 2);
        int16_t textX = startX + colorRectSize + 8; // 8px de separación color->texto

        FontEngine_DrawString(pBuf, fontId, textX, textY, 
                       ovl->traces[i].valueText, ovl->textColor, ALIGN_VCENTER, 1);
    }
} */

void gfx_drawGraphOverlay(pixel16_t *pBuf, gfx_GraphOverlay *ovl) {
    if (!pBuf || !ovl) return;

    // 1. Dibujar el fondo del panel flotante
    gfx_fillRoundRect(pBuf, ovl->pos.x, ovl->pos.y, 
                      ovl->size.width, ovl->size.height, 5, ovl->bgColor);

    // 2. Resolver la fuente y obtener la altura para espaciar los elementos
    int8_t fontId = Theme_ResolveFontId(ovl->typo);
    uint16_t textW = 0, textH = 0;
    
    if (fontId >= 0) {
        FontEngine_GetStringDimensions("0", fontId, &textW, &textH, 1); 
    }

    // 3. Iterar sobre las trazas activas y dibujarlas
    int16_t startY = ovl->pos.y + 10; // Margen superior
    int16_t startX = ovl->pos.x + 10; // Margen izquierdo
    uint16_t colorRectSize = textH * 0.8f; 

    int i = 0;
    for (; i < ovl->numTraces; i++) {
        // ¡ELIMINADO!: if (!ovl->traces[i].isVisible) continue;
        // Ahora SIEMPRE evaluamos la fila para poder ver el contorno y el texto

        int16_t currentY = startY + (i * (textH + 8)); 

        // A. Dibujar el cuadrito de color indicador
        if (ovl->traces[i].isVisible) {
            // Relleno cuando la traza ESTÁ visible
            gfx_fillRoundRect(pBuf, startX, currentY, 
                              colorRectSize, colorRectSize, 2, ovl->traces[i].color);
        } else {
            // Solo el contorno cuando la traza está OCULTA
            // (Si no tienes gfx_drawRoundRect, usa gfx_drawRect estándar)
            gfx_drawRoundRect(pBuf, startX, currentY, 
                              colorRectSize, colorRectSize, 2, ovl->traces[i].color);
        }

        // B. Dibujar el texto del valor (se dibuja siempre)
        int16_t textY = currentY + (colorRectSize / 2);
        int16_t textX = startX + colorRectSize + 8; 

        // Opcional: Podrías cambiar ovl->textColor por un color gris si está oculta
        // uint16_t renderTextColor = ovl->traces[i].isVisible ? ovl->textColor : COLOR_GRAY;

        FontEngine_DrawString(pBuf, fontId, textX, textY, 
                       ovl->traces[i].valueText, ovl->textColor, ALIGN_VCENTER, 1);
    }
}

#define INVALID_TRACE_INDEX -1

int gfx_processOverlayTouch(gfx_GraphOverlay *ovl, TouchStatus touch) {
    if (ovl == NULL) return INVALID_TRACE_INDEX;

    // 1. Obtener las métricas de la fuente
    int8_t fontId = Theme_ResolveFontId(ovl->typo);
    uint16_t textW = 0, textH = 20; 
    if (fontId >= 0) {
        FontEngine_GetStringDimensions("0", fontId, &textW, &textH, 1);
    }

    // 2. Parámetros base del layout
    int16_t startY = ovl->pos.y + 10;
    int16_t startX = ovl->pos.x + 10;
    uint16_t rowHeight = textH + 8; 
    uint16_t colorRectSize = textH * 0.8f;
    int16_t hitPadding = 12; 

    // 3. Comprobar límite superior
    if (touch.y < (startY - hitPadding)) {
        return INVALID_TRACE_INDEX;
    }

    // 4. Calcular matemáticamente qué fila fue tocada
    int16_t relativeY = touch.y - (startY - hitPadding);
    int touchedTrace = relativeY / rowHeight; // Cambiado a int

    // 5. Validar que la traza calculada realmente exista
    if (touchedTrace >= ovl->numTraces) {
        return INVALID_TRACE_INDEX; 
    }

    // 6. Validar el eje X (Asegurarnos de que tocó cerca del cuadrito de color)
    if (touch.x < (startX - hitPadding) || 
        touch.x > (startX + colorRectSize + hitPadding * 2)) {
        return INVALID_TRACE_INDEX;
    }

    // Retorna el índice válido (0, 1, 2, 3...)
	TIVA_LOGI(TASK_NAME, "Touched trace: %d", touchedTrace);
    return touchedTrace;
}

bool gfx_getWidgetBounds(gfx_GenericWidget *widget, int16_t *x, int16_t *y,
                         int16_t *w, int16_t *h) {
  if (widget == NULL || widget->pvWidget == NULL)
    return false;

  switch (widget->eWidgetType) {

  case WD_TYPE_BUTTON: {
    gfx_Button *btn = (gfx_Button *)widget->pvWidget;

    // 1. Obtener dimensiones dinámicas del texto
    uint16_t textWidth = 0;
    uint16_t textHeight = 0;
    int8_t fontId = Theme_ResolveFontId(btn->typo);

    if (btn->label != NULL && fontId >= 0) {
      FontEngine_GetStringDimensions(btn->label, fontId, &textWidth, &textHeight, 1);
    }

    // 2. Calcular el desbordamiento (Alpha-Blending Trap Fix)
    int16_t overflowX = 0;
    if (textWidth > btn->size.width) {
      overflowX = (textWidth - btn->size.width) / 2;
      overflowX += 4; // Padding extra para el anti-aliasing
    }

    // 3. Calcular la huella total (Footprint + Overflow + Cinematic Offset)
    *x = btn->pos.x - overflowX - btn->borderWidth;
    *y = btn->pos.y - btn->borderWidth;

    // Sumamos 2 píxeles extra a W y H para acomodar la animación de
    // "Presionado"
    *w = btn->size.width + (overflowX * 2) + (btn->borderWidth * 2) + 2;
    *h = btn->size.height + (btn->borderWidth * 2) + 2;

    return true;
  }

  case WD_TYPE_RECT: {
    gfx_Rectangle *rect = (gfx_Rectangle *)widget->pvWidget;
    *x = rect->pos.x;
    *y = rect->pos.y;
    *w = rect->dim.width;
    *h = rect->dim.height;
    return true;
  }

  case WD_TYPE_LABEL: {
    gfx_Label *lb = (gfx_Label *)widget->pvWidget;

    // 1. Obtener dimensiones dinámicas
    uint16_t textW = 0, textH = 0;
    uint8_t fontId = Theme_ResolveFontId(lb->typo);

    if (lb->text != NULL) {
      FontEngine_GetStringDimensions(lb->text, fontId, &textW, &textH, 1);
    }

    // 2. Extraer el Ascent para la línea base
    int16_t ascent = 0;
    if (g_FontCache[fontId].isLoaded) {
      BDF_Font_t *sFont = &g_FontCache[fontId].bdfData;
      ascent = sFont->yAdvance + sFont->globalYOffset;
    }

    // 3. Ajustar X
    int16_t startX = lb->pos.x;
    if (lb->alignment & ALIGN_HCENTER) {
      startX = lb->pos.x - (textW / 2);
    } else if (lb->alignment & ALIGN_RIGHT) {
      startX = lb->pos.x - textW;
    }

    // 4. Ajustar Y con la matemática tipográfica correcta
    int16_t startY = lb->pos.y;
    if (lb->alignment & ALIGN_TOP) {
      startY = lb->pos.y;
    } else if (lb->alignment & ALIGN_VCENTER) {
      startY = lb->pos.y - (textH / 2);
    } else if (lb->alignment & ALIGN_BOTTOM) {
      startY = lb->pos.y - textH;
    } else {
      // DEFAULT (Baseline alignment)
      startY = lb->pos.y - ascent;
    }

    // 5. Retornar límites con padding de seguridad (Anti-aliasing)
    *x = startX - 2;
    *y = startY - 2;
    *w = textW + 4;
    *h = textH + 4;

    return true;
  }

  case WD_TYPE_SLIDER: {
    gfx_Slider *sl = (gfx_Slider *)widget->pvWidget;

    // 1. Calcular el mismo radio gigante que usamos en el renderizador
    uint16_t dynamicKnobRadius = sl->size.height * 1.15;

    // 2. Calcular cuánto "sobresale" la perilla por arriba y abajo
    int16_t knobBleedY = dynamicKnobRadius - (sl->size.height / 2);
    if (knobBleedY < 0)
      knobBleedY = 0;

    // 3. Definir la huella total que envuelve la perilla gigante en los
    // extremos
    *x = sl->pos.x - dynamicKnobRadius - 2;
    *y = sl->pos.y - knobBleedY - 2;
    *w = sl->size.width + (dynamicKnobRadius * 2) + 4;
    *h = sl->size.height + (knobBleedY * 2) + 4;

    return true;
  }
  case WD_TYPE_GRAPH: {
    gfx_Graph *graph = (gfx_Graph *)widget->pvWidget;
    *x = graph->pos.x;
    *y = graph->pos.y;
    *w = graph->size.width;
    *h = graph->size.height;
    return true;
  }
  // En form_manager.c (dentro de gfx_getWidgetBounds)
  case WD_TYPE_GRAPH_OVERLAY: {
    gfx_GraphOverlay *ovl = (gfx_GraphOverlay *)widget->pvWidget;
    *x = ovl->pos.x;
    *y = ovl->pos.y;
    *w = ovl->size.width;
    *h = ovl->size.height;
    return true;
  }
  case WD_TYPE_LISTVIEW: {
    gfx_ListView *ovl = (gfx_ListView *)widget->pvWidget;
    *x = ovl->pos.x;
    *y = ovl->pos.y;
    *w = ovl->size.width;
    *h = ovl->size.height;
    return true;
  }
  default:
    return false;
  }
}

static void initNumpadButton(gfx_Button *btn, int16_t x, int16_t y, int16_t w, int16_t h, char *label, uint8_t style) {
    btn->pos.x = x;
    btn->pos.y = y;
    btn->size.width = w;
    btn->size.height = h;
    btn->label = label;
    btn->typo = TYPO_H3;         // Usando tu enumerador tipográfico
    btn->style = (gfx_WidgetStyle_e)style;          // STYLE_PRIMARY, STYLE_DANGER, etc.
    btn->state = BTN_STATE_NORMAL;
    btn->radius = 6;
    btn->borderWidth = 1;
    btn->bIsDirty = true;
    
	btn->onPressed = onGenericBtnPressed;
	btn->onRelease = onGenericBtnRelease;

    // Usamos tu función para armar el Bounding Box táctil
    gfx_initRegTouch((void*)btn, WD_TYPE_BUTTON);
}

void gfx_NumpadInit(gfx_Numpad *np, char *lb, Position pos, Size size, bool isVisible) {
	if(np == NULL) return;

	np->size = size;
	np->pos = pos;

	memset(np->_digitBuffer, 0, NUMPAD_MAX_DIGITS);
	
	np->bIsVisible = isVisible;
	// Init the display
	uint16_t rowHeight = ((size.height - 5 * 10) / 5.0);
	uint16_t verticalSpacer = 10;
	uint16_t horizontalSpacer = 10;
	np->_displayBg = (gfx_Rectangle){
		.pos = pos,
		.dim.height = rowHeight,
		.dim.width = size.width - 20,
		.borderWidth = 3,
		.color = g_pCurrentTheme->palette.surface,
		.round = 4,
	};
	
	np->_displayLabel = (gfx_Label){
		.name = "displayLabel",
		.text = lb,
		.pos.x = pos.x + 20,
		.pos.y = pos.y + rowHeight / 2,
		.alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER),
		.isVisible = true,
		.style = STYLE_TEXT_MUTED,
		.typo = TYPO_BODY,
		.isVisible = true,
	};

	np->_displayValue = (gfx_Label) {
		.name = "displayValue",
		.text = np->_digitBuffer,
		.pos.x = pos.x + size.width - 40,
		.pos.y = pos.y + rowHeight / 2,
		.alignment = (gfx_Align_e)(ALIGN_RIGHT | ALIGN_VCENTER),
		.isVisible = true,
		.style = STYLE_TEXT_MAIN,
		.typo = TYPO_H2,
		.isVisible = true,
	};

	// Initialize buttons
	int16_t incrementalX = pos.x;
	int16_t incrementalY = pos.y + rowHeight + verticalSpacer;
	uint16_t buttonWidth = ((size.width - 10) - 10 * 4) / 4.0f;
	initNumpadButton(&np->_buttons[7], incrementalX, incrementalY, buttonWidth, rowHeight, "7", STYLE_TEXT_MAIN);
	initNumpadButton(&np->_buttons[8], incrementalX + buttonWidth + horizontalSpacer, incrementalY, buttonWidth, rowHeight, "8", STYLE_TEXT_MAIN);
	initNumpadButton(&np->_buttons[9], incrementalX + 2 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight, "9", STYLE_TEXT_MAIN);
	initNumpadButton(&np->_buttons[14], incrementalX + 3 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight, "BORRAR", STYLE_DANGER);
	
	incrementalY += rowHeight + verticalSpacer;
	initNumpadButton(&np->_buttons[4], incrementalX, incrementalY, buttonWidth, rowHeight, "4", STYLE_TEXT_MAIN);
	initNumpadButton(&np->_buttons[5], incrementalX + buttonWidth + horizontalSpacer, incrementalY, buttonWidth, rowHeight, "5", STYLE_TEXT_MAIN);
	initNumpadButton(&np->_buttons[6], incrementalX + 2 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight, "6", STYLE_TEXT_MAIN);
	initNumpadButton(&np->_buttons[13], incrementalX + 3 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight, "CANCELAR", STYLE_TEXT_MUTED);

	incrementalY += rowHeight + verticalSpacer;
	initNumpadButton(&np->_buttons[1], incrementalX, incrementalY, buttonWidth, rowHeight, "1", STYLE_TEXT_MAIN);
	initNumpadButton(&np->_buttons[2], incrementalX + buttonWidth + horizontalSpacer, incrementalY, buttonWidth, rowHeight, "2", STYLE_TEXT_MAIN);
	initNumpadButton(&np->_buttons[3], incrementalX + 2 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight, "3", STYLE_TEXT_MAIN);
	initNumpadButton(&np->_buttons[12], incrementalX + 3 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight * 2 + 10, "OK", STYLE_SUCCESS);

	incrementalY += rowHeight + verticalSpacer;
	initNumpadButton(&np->_buttons[0], incrementalX, incrementalY, buttonWidth, rowHeight, "0", STYLE_TEXT_MAIN);
	initNumpadButton(&np->_buttons[10], incrementalX + buttonWidth + horizontalSpacer, incrementalY, buttonWidth, rowHeight, " ", STYLE_TEXT_MAIN);
	initNumpadButton(&np->_buttons[11], incrementalX + 2 * ( buttonWidth + horizontalSpacer ), incrementalY, buttonWidth, rowHeight, ".", STYLE_TEXT_MAIN);

	gfx_initRegTouch((void *)np, WD_TYPE_NUMPAD);
}

void gfx_drawNumpad(pixel16_t *pBuf, gfx_Numpad *np) {
	if(!np->bIsVisible) return;

	gfx_drawRectangle(pBuf, &np->_displayBg);
	gfx_drawLabel(pBuf, &np->_displayLabel);
	gfx_drawLabel(pBuf, &np->_displayValue);

	uint8_t i = 0;
	for(; i < 15; i++) {
		gfx_drawButton(pBuf, &np->_buttons[i]);
	}

	np->bIsDirty = false;
}

bool gfx_processNumpadTouch(gfx_Numpad *np, TouchStatus touch) {
    if (!np->bIsVisible) return false;
    bool eventHandled = false;

    // Si la pantalla está siendo tocada (Dedo abajo)
    if (touch.state) {
		int i = 0;
        for (; i < 13; i++) {
            // Usamos tu detector de colisiones AABB
            if (gfx_touchObject(np->_buttons[i].regTouch, touch)) {
                // Animación de presionado
                if (np->_buttons[i].state != BTN_STATE_PRESSED) {
                    np->_buttons[i].state = BTN_STATE_PRESSED;
                    np->_buttons[i].bIsDirty = true;
                    np->bIsDirty = true;
                }
                np->_lastTouchedBtn = i;
                eventHandled = true;
            } else {
                // Si el dedo resbala fuera del botón, lo regresamos a estado normal
                if (np->_buttons[i].state == BTN_STATE_PRESSED) {
                    np->_buttons[i].state = BTN_STATE_NORMAL;
                    np->_buttons[i].bIsDirty = true;
                    np->bIsDirty = true;
                }
            }
        }
    } 
    // Dedo levantado (Release)
    else {
        if (np->_lastTouchedBtn != -1) {
            int i = np->_lastTouchedBtn;
            
            // Regresar el botón a la normalidad visual
            np->_buttons[i].state = BTN_STATE_NORMAL;
            np->_buttons[i].bIsDirty = true;
            np->bIsDirty = true;
            np->_lastTouchedBtn = -1;

            // ================== LÓGICA DE NEGOCIO ==================
            if (i >= 0 && i <= 9) { // Números
                if (np->_currentLen < NUMPAD_MAX_DIGITS) {
                    np->_digitBuffer[np->_currentLen++] = '0' + i;
                    np->_digitBuffer[np->_currentLen] = '\0';
                    np->_displayValue.bIsDirty = true;
                }
            } 
            else if (i == NUMPAD_DELETE_BUTTON) { // Borrar
                if (np->_currentLen > 0) {
                    np->_currentLen--;
                    np->_digitBuffer[np->_currentLen] = '\0';
                    np->_displayValue.bIsDirty = true;
                }
            } 
            else if (i == NUMPAD_OK_BUTTON) { // Aceptar (OK)
                int32_t finalValue = atoi(np->_digitBuffer);
				if(np->onOkButtonReleased != NULL)
					np->onOkButtonReleased(np);
				
                np->bIsVisible = false;
            } 
            else if (i == NUMPAD_CANCEL_BUTTON) { // Cancelar (X)
                np->bIsVisible = false;
            }
            eventHandled = true;
        }
    }

    // Retorna true si el Numpad atrapó el toque (útil para bloquear toques en widgets debajo)
    return eventHandled;
}

void gfx_DualGraphAddData(gfx_DualGraph *graph, uint8_t traceIndex, float newValue) {
    // Validaciones de seguridad
    if (!graph || graph->maxPoints == 0 || traceIndex >= DUAL_GRAPH_TRACES) return;
    if (graph->dataSets[traceIndex] == NULL) return;

    uint16_t currentHead = graph->heads[traceIndex];

    graph->dataSets[traceIndex][currentHead] = newValue;
    graph->heads[traceIndex] = (currentHead + 1) % graph->maxPoints;
    graph->totalPointsAdded[traceIndex]++;

    graph->bEVEDirty = true;
}

void gfx_DualGraphAddStaticData(gfx_DualGraph *graph, uint8_t traceIndex, float newValue, uint32_t currentElapsedMs, uint32_t totalDurationMs) {
    // Validaciones de seguridad
    if (!graph || graph->maxPoints == 0 || traceIndex >= DUAL_GRAPH_TRACES) return;
    if (graph->dataSets[traceIndex] == NULL || totalDurationMs == 0) return;

    // 1. Calcular a qué índice del arreglo (bucket) corresponde este milisegundo
    uint16_t targetIdx = (currentElapsedMs * (graph->maxPoints - 1)) / totalDurationMs;
    
    // Protección contra desbordamiento por latencia
    if (targetIdx >= graph->maxPoints) {
        targetIdx = graph->maxPoints - 1;
    }

    // 2. Si hubo retrasos en el ciclo y nos "saltamos" índices, rellenamos los huecos 
    //    con el valor anterior para que la línea no se corte (Zero-Order Hold).
    uint16_t lastDrawnIdx = graph->totalPointsAdded[traceIndex];
    if (lastDrawnIdx > 0 && targetIdx > lastDrawnIdx) {
		uint16_t fill = lastDrawnIdx;
        for (; fill < targetIdx; fill++) {
            graph->dataSets[traceIndex][fill] = graph->dataSets[traceIndex][lastDrawnIdx - 1];
        }
    }

    // 3. Escribir el nuevo valor en la posición temporal correcta
    graph->dataSets[traceIndex][targetIdx] = newValue;

    // 4. Actualizamos el límite de dibujo. Al mantenerlo <= maxPoints, 
    //    forzamos al render de EVE a usar el modo estático progresivo.
    graph->totalPointsAdded[traceIndex] = targetIdx + 1;

    graph->bEVEDirty = true;
}

/*
void gfx_drawDualGraph(pixel16_t *pBuf, gfx_DualGraph *graph) {
    if (!pBuf || !graph) return;

    // 1. Dibujar Fondo
    gfx_fillRoundRect(pBuf, graph->pos.x, graph->pos.y, 
                      graph->size.width, graph->size.height, 4, graph->bgColor);

    int8_t fontId = -1;
    uint16_t textW = 0, textH = 0;
    
    if (graph->bShowLabels) {
        fontId = Theme_ResolveFontId(graph->typo); 
        if (fontId >= 0) {
            FontEngine_GetStringDimensions("0", fontId, &textW, &textH, 1); 
        }
    }

    // 2. Líneas Horizontales y Etiquetas (Ejes Y)
    int16_t stepY = graph->size.height / (graph->gridLinesY + 1);
    
    // Calcular el paso de valor para ambas escalas
    float valStep0 = (float)(graph->maxY[0] - graph->minY[0]) / (graph->gridLinesY + 1);
    float valStep1 = (float)(graph->maxY[1] - graph->minY[1]) / (graph->gridLinesY + 1);

	int i = 0;
    for (; i <= (graph->gridLinesY + 1); i++) {
        int16_t gy = graph->pos.y + (i * stepY);
        
        // Dibujar línea de cuadrícula
        if (i > 0 && i < (graph->gridLinesY + 1)) {
            gfx_drawFastHLine(pBuf, graph->pos.x, gy, graph->size.width, graph->gridColor);
        }

        // Dibujar Etiquetas Duales
        if (graph->bShowLabels && fontId >= 0) {
            char valStr0[12], valStr1[12];
            
            // Invertir Y (el índice 0 es el 'techo' o Max)
            int16_t gridValue0 = graph->maxY[0] - (int16_t)(i * valStep0); 
            int16_t gridValue1 = graph->maxY[1] - (int16_t)(i * valStep1); 

            snprintf(valStr0, sizeof(valStr0), "%d", gridValue0); 
            snprintf(valStr1, sizeof(valStr1), "%d", gridValue1); 
            
            // Prevención de desbordamiento de texto en los bordes top/bottom
            int16_t textY = gy + (textH / 3);
            if (i == 0) textY = graph->pos.y + 15; 
            if (i == (graph->gridLinesY + 1)) textY = graph->pos.y + graph->size.height - textH / 3;

            // Etiqueta Izquierda (Traza 0) - Usamos el color de la línea 0
            int16_t textX0 = graph->pos.x + 5;
            FontEngine_DrawString(pBuf, fontId, textX0, textY, valStr0, graph->lineColors[0], ALIGN_LEFT, 1);
            
            // Etiqueta Derecha (Traza 1) - Usamos el color de la línea 1
            int16_t textX1 = graph->pos.x + graph->size.width - 5;
            FontEngine_DrawString(pBuf, fontId, textX1, textY, valStr1, graph->lineColors[1], ALIGN_RIGHT, 1);
        }
    }

    // 3. Líneas Verticales (Eje X)
    if (graph->gridLinesX > 0) {
        int16_t stepX = graph->size.width / (graph->gridLinesX + 1);
		int i = 1;
        for (; i <= graph->gridLinesX; i++) {
            int16_t gx = graph->pos.x + (i * stepX);
            gfx_drawFastVLine(pBuf, gx, graph->pos.y, graph->size.height, graph->gridColor);
        }
    }

    graph->bIsDirty = false;
}*/
void gfx_drawDualGraph(pixel16_t *pBuf, gfx_DualGraph *graph) {
    if (!pBuf || !graph) return;

    // 1. Dibujar fondo de la gráfica
    gfx_fillRoundRect(pBuf, graph->pos.x, graph->pos.y, 
                      graph->size.width, graph->size.height, 4, graph->bgColor);

    int8_t fontId = -1;
    uint16_t textW = 0, textH = 0;
    
    // Cargar fuente si hay textos por renderizar
    if (graph->bShowLabels || graph->bShowXLabels || 
        graph->xAxisName[0] != '\0' || graph->yAxisNameLeft[0] != '\0' || graph->yAxisNameRight[0] != '\0') {
        
        fontId = Theme_ResolveFontId(graph->typo); 
        if (fontId >= 0) {
            FontEngine_GetStringDimensions("0", fontId, &textW, &textH, 1); 
        }
    }

    // =========================================================
    // 2. Nombres de los Ejes (Títulos)
    // =========================================================
    if (fontId >= 0) {
        if (graph->yAxisNameLeft[0] != '\0') {
            FontEngine_DrawString(pBuf, fontId, graph->pos.x, graph->pos.y - 4, 
                                  graph->yAxisNameLeft, graph->textColor, (gfx_Align_e)(ALIGN_LEFT | ALIGN_BOTTOM), 1);
        }
        
        if (graph->yAxisNameRight[0] != '\0') {
            FontEngine_DrawString(pBuf, fontId, graph->pos.x + graph->size.width, graph->pos.y - 4, 
                                  graph->yAxisNameRight, graph->textColor, (gfx_Align_e)(ALIGN_RIGHT | ALIGN_BOTTOM), 1);
        }

        if (graph->xAxisName[0] != '\0') {
            FontEngine_DrawString(pBuf, fontId, graph->pos.x + graph->size.width, graph->pos.y + graph->size.height + 22, 
                                  graph->xAxisName, graph->textColor, (gfx_Align_e)(ALIGN_RIGHT | ALIGN_TOP), 1);
        }
    }

    // =========================================================
    // 3. Cuadrícula y Etiquetas de ambos Ejes Y
    // =========================================================
    int16_t stepY = graph->size.height / (graph->gridLinesY + 1);
    int16_t valStepLeft = (graph->maxY[0] - graph->minY[0]) / (graph->gridLinesY + 1);
    int16_t valStepRight = (graph->maxY[1] - graph->minY[1]) / (graph->gridLinesY + 1);

	int i = 0;
    for (; i <= (graph->gridLinesY + 1); i++) {
        int16_t gy = graph->pos.y + (i * stepY);

        if (i > 0 && i < (graph->gridLinesY + 1)) {
            gfx_drawFastHLine(pBuf, graph->pos.x, gy, graph->size.width, graph->gridColor);
        }

        if (graph->bShowLabels && fontId >= 0) {
            int16_t textY = gy + (textH / 3);
            if (i == 0) textY = graph->pos.y + 15; 
            if (i == (graph->gridLinesY + 1)) textY = graph->pos.y + graph->size.height - textH / 3;

            char valStr[12];

            // 3A. Eje Izquierdo
            int16_t valLeft = graph->maxY[0] - (i * valStepLeft);
            snprintf(valStr, sizeof(valStr), "%d", valLeft);
            FontEngine_DrawString(pBuf, fontId, graph->pos.x + 5, textY, valStr, graph->lineColors[0], ALIGN_LEFT, 1);

            // 3B. Eje Derecho
            int16_t valRight = graph->maxY[1] - (i * valStepRight);
            snprintf(valStr, sizeof(valStr), "%d", valRight);
            FontEngine_DrawString(pBuf, fontId, graph->pos.x + graph->size.width - 5, textY, valStr, graph->lineColors[1], ALIGN_RIGHT, 1);
        }
    }

    // =========================================================
    // 4. Cuadrícula y Etiquetas del Eje X
    // =========================================================
    int16_t stepX = graph->size.width / (graph->gridLinesX + 1);
    
    for (i = 0; i <= (graph->gridLinesX + 1); i++) {
        int16_t gx = graph->pos.x + (i * stepX);

        if (i > 0 && i < (graph->gridLinesX + 1)) {
            gfx_drawFastVLine(pBuf, gx, graph->pos.y, graph->size.height, graph->gridColor);
        }

        if (graph->bShowXLabels && fontId >= 0 && graph->maxXValue > 0) {
            float xVal = (graph->maxXValue * i) / (graph->gridLinesX + 1);
            char xLabelBuf[16];
            snprintf(xLabelBuf, sizeof(xLabelBuf), "%.0f", xVal);
            
            gfx_Align_e align = ALIGN_CENTER;
            if (i == 0) align = ALIGN_LEFT;
            else if (i == (graph->gridLinesX + 1)) align = ALIGN_RIGHT;

            FontEngine_DrawString(pBuf, fontId, gx, graph->pos.y + graph->size.height + 2, xLabelBuf, graph->textColor, (gfx_Align_e)(align | ALIGN_TOP), 1);
        }
    }

    // =========================================================
    // 5. Dibujar Perfil Estático de Fondo (Software Render)
    // =========================================================
    if (graph->bShowBgProfile && graph->bgProfileData != NULL && graph->maxPoints > 1) {
        
        int32_t rangeY = (int32_t)graph->maxY[0] - (int32_t)graph->minY[0];
        if (rangeY <= 0) rangeY = 1;

        int16_t anchorX = -1, anchorY = -1;
        int16_t prevX = -1, prevY = -1;
        float prevDiff = 0.0f; // Para rastrear la pendiente

		uint16_t i = 0;
        for (; i < graph->maxPoints; i++) {
            
            int16_t px = (int16_t)((int32_t)graph->pos.x + ((int32_t)i * (graph->size.width - 1)) / (graph->maxPoints - 1));
            
            float rawVal = graph->bgProfileData[i];
            float clampedVal = rawVal;
            if (clampedVal < graph->minY[0]) clampedVal = graph->minY[0];
            if (clampedVal > graph->maxY[0]) clampedVal = graph->maxY[0];

            int16_t py = (int16_t)((int32_t)graph->pos.y + (graph->size.height - 1) - (((int32_t)(clampedVal - graph->minY[0]) * (graph->size.height - 1)) / rangeY));

            if (i == 0) {
                // Primer punto: establecer ancla
                anchorX = px;
                anchorY = py;
            } else {
                // Calcular derivada (cambio en Y)
                float diff = rawVal - graph->bgProfileData[i-1];
                
                if (i == 1) {
                    prevDiff = diff; // Establecer la pendiente inicial
                } else {
                    float deltaDiff = diff - prevDiff;
                    if (deltaDiff < 0) deltaDiff = -deltaDiff; // Valor Absoluto rápido

                    // Si la pendiente cambia, detectamos un "vértice" o "esquina"
                    if (deltaDiff > 0.05f) {
                        
                        // Enviar el macro-segmento completo a tu función punteada
                        if (graph->bBgIsDashed) {
                            gfx_drawDashedLine(pBuf, anchorX, anchorY, prevX, prevY, 
                                               graph->bgProfileColor, graph->bgDashLen, graph->bgSpaceLen);
                        } else {
                            gfx_drawDashedLine(pBuf, anchorX, anchorY, prevX, prevY, 
                                               graph->bgProfileColor, 255, 0); 
                        }
                        
                        // El vértice anterior se vuelve nuestra nueva ancla
                        anchorX = prevX;
                        anchorY = prevY;
                        prevDiff = diff;
                    }
                }
            }
            prevX = px;
            prevY = py;
        }

        // 6. Al salir del bucle, trazar el último segmento que quedó pendiente
        if (anchorX != -1 && (anchorX != prevX || anchorY != prevY)) {
            if (graph->bBgIsDashed) {
                gfx_drawDashedLine(pBuf, anchorX, anchorY, prevX, prevY, 
                                   graph->bgProfileColor, graph->bgDashLen, graph->bgSpaceLen);
            } else {
                gfx_drawDashedLine(pBuf, anchorX, anchorY, prevX, prevY, 
                                   graph->bgProfileColor, 255, 0); 
            }
        }
    }

    graph->bIsDirty = false;
}

void gfx_DualGraphRenderEVEComponents(gfx_DualGraph *graph) {
    if (graph != NULL && graph->maxPoints > 1) {
        uint8_t trace = 0;
        for (; trace < DUAL_GRAPH_TRACES; trace++) {
            
            if (graph->dataSets[trace] == NULL) continue;

            uint32_t totalPts = graph->totalPointsAdded[trace];
            uint16_t availablePts = (totalPts < graph->maxPoints) ? totalPts : graph->maxPoints;

            if (availablePts < 2) continue;

            uint16_t renderPts = availablePts; 
            if (renderPts > graph->size.width) {
                renderPts = graph->size.width;
            }

            // === CÁLCULO INDEPENDIENTE DE ESCALA ===
            int32_t rangeY = (int32_t)graph->maxY[trace] - (int32_t)graph->minY[trace];
            if (rangeY <= 0) rangeY = 1;

            API_BEGIN(LINE_STRIP); 
            API_LINE_WIDTH(graph->lineWidth * 16 / 2); 
            
            uint16_t c = graph->lineColors[trace];
            API_COLOR_RGB((uint8_t)((c >> 8) & 0xF8), (uint8_t)((c >> 3) & 0xFC), (uint8_t)((c << 3) & 0xF8));
            
            uint16_t currentTraceHead = graph->heads[trace];
            uint16_t i = 0;
            for (; i < renderPts; i++) {
                
                uint16_t dataOffset = (i * (availablePts - 1)) / (renderPts - 1);
                uint16_t dataIdx;
                int16_t px;

                if (totalPts < graph->maxPoints) {
                    dataIdx = dataOffset; 
                    px = (int16_t)((int32_t)graph->pos.x + ((int32_t)dataOffset * (graph->size.width - 1)) / (graph->maxPoints - 1));
                } 
                else {
                    dataIdx = (currentTraceHead + dataOffset) % graph->maxPoints;
                    px = (int16_t)((int32_t)graph->pos.x + ((int32_t)i * (graph->size.width - 1)) / (renderPts - 1));
                }

                float val = graph->dataSets[trace][dataIdx];

                // === SATURACIÓN CON LÍMITES INDEPENDIENTES ===
                if (val < graph->minY[trace]) val = graph->minY[trace];
                if (val > graph->maxY[trace]) val = graph->maxY[trace];

                int16_t py = (int16_t)((int32_t)graph->pos.y + (graph->size.height - 1) - (((int32_t)(val - graph->minY[trace]) * (graph->size.height - 1)) / rangeY));

                API_VERTEX2F(px * 16, py * 16);
            }
            
            API_END();

            // Protección de saturación SPI Burst entre trazas
            if (trace < (DUAL_GRAPH_TRACES - 1)) {
                API_LIB_EndCoProList(); 
                API_LIB_AwaitCoProEmpty();
                API_LIB_BeginCoProList();
            }
        }
    }
}

/*
void gfx_DualGraphRenderEVEComponents(gfx_DualGraph *graph) {
    if (graph == NULL || !graph->bIsVisible) return;

    // ========================================================
    // 1. DIBUJAR PERFIL DE FONDO (CORRIENTE ESPERADA)
    // ========================================================
    if (graph->bShowBgProfile && graph->bgProfileData != NULL && graph->maxPoints > 1) {
        
        int32_t rangeY = (int32_t)graph->maxY[0] - (int32_t)graph->minY[0]; // Usa la escala del Eje 0 (Corriente)
        if (rangeY <= 0) rangeY = 1;

        // Configurar Color y Grosor
        uint16_t c = graph->bgProfileColor;
        API_COLOR_RGB((uint8_t)((c >> 8) & 0xF8), (uint8_t)((c >> 3) & 0xFC), (uint8_t)((c << 3) & 0xF8));
        API_LINE_WIDTH(graph->lineWidth * 16 / 2); 

        bool isDrawing = true;
        uint16_t dashCount = 0;
        uint8_t dLen = (graph->bgDashLen > 0) ? graph->bgDashLen : 5;
        uint8_t sLen = (graph->bgSpaceLen > 0) ? graph->bgSpaceLen : 5;

        if (!graph->bBgIsDashed) API_BEGIN(LINE_STRIP);

        // Iterar sobre todos los puntos pre-calculados (estático)
		uint16_t i = 0;
        for (; i < graph->maxPoints; i++) {
            
            int16_t px = (int16_t)((int32_t)graph->pos.x + ((int32_t)i * (graph->size.width - 1)) / (graph->maxPoints - 1));
            
            int16_t val = graph->bgProfileData[i];
            if (val < graph->minY[0]) val = graph->minY[0];
            if (val > graph->maxY[0]) val = graph->maxY[0];

            int16_t py = (int16_t)((int32_t)graph->pos.y + (graph->size.height - 1) - (((int32_t)(val - graph->minY[0]) * (graph->size.height - 1)) / rangeY));

            // Inyección segmentada de hardware EVE
            if (graph->bBgIsDashed) {
                if (isDrawing) {
                    if (dashCount == 0) {
                        API_BEGIN(LINE_STRIP);
                        API_VERTEX2F(px * 16, py * 16);
                    } else {
                        API_VERTEX2F(px * 16, py * 16);
                    }
                    dashCount++;
                    if (dashCount >= dLen) {
                        API_END();
                        isDrawing = false;
                        dashCount = 0;
                    }
                } else {
                    dashCount++;
                    if (dashCount >= sLen) {
                        isDrawing = true;
                        dashCount = 0;
                    }
                }
            } else {
                API_VERTEX2F(px * 16, py * 16);
            }
        }
        
        if (!graph->bBgIsDashed || isDrawing) API_END();
    }

    // ========================================================
    // 2. DIBUJAR TRAZAS DINÁMICAS REALES (Corriente y Temp)
    // ========================================================
	uint8_t tr = 0;
    for (; tr < DUAL_GRAPH_TRACES; tr++) {
        
        if (graph->dataSets[tr] == NULL) continue;

        uint16_t pointsToDraw = (graph->totalPointsAdded[tr] < graph->maxPoints) ? 
                                 graph->totalPointsAdded[tr] : graph->maxPoints;

        if (pointsToDraw < 2) continue;

        int32_t rangeY = (int32_t)graph->maxY[tr] - (int32_t)graph->minY[tr];
        if (rangeY <= 0) rangeY = 1;

        API_BEGIN(LINE_STRIP); 
        API_LINE_WIDTH(graph->lineWidth * 16 / 2); 
        
        uint16_t c = graph->lineColors[tr];
        API_COLOR_RGB((uint8_t)((c >> 8) & 0xF8), (uint8_t)((c >> 3) & 0xFC), (uint8_t)((c << 3) & 0xF8));

		uint16_t i = 0;
        for (; i < pointsToDraw; i++) {
            
            uint16_t dataIdx;
            int16_t px;

            if (graph->totalPointsAdded[tr] < graph->maxPoints) {
                dataIdx = i; 
            } else {
                dataIdx = (graph->heads[tr] + i) % graph->maxPoints;
            }
            
            px = (int16_t)((int32_t)graph->pos.x + ((int32_t)i * (graph->size.width - 1)) / (graph->maxPoints - 1));

            int16_t val = graph->dataSets[tr][dataIdx];
            if (val < graph->minY[tr]) val = graph->minY[tr];
            if (val > graph->maxY[tr]) val = graph->maxY[tr];

            int16_t py = (int16_t)((int32_t)graph->pos.y + (graph->size.height - 1) - (((int32_t)(val - graph->minY[tr]) * (graph->size.height - 1)) / rangeY));

            API_VERTEX2F(px * 16, py * 16);
        }
        
        API_END(); 
    }
} */

void gfx_drawListView(pixel16_t *pBuf, gfx_ListView *lv) {
    if (!lv->bIsVisible) return;

    // 1. Dibujar el fondo y borde de la lista
    gfx_fillRoundRect(pBuf, lv->pos.x, lv->pos.y, lv->size.width, lv->size.height, 4, lv->bgColor);
    gfx_drawRoundRect(pBuf, lv->pos.x, lv->pos.y, lv->size.width, lv->size.height, 4, lv->lineColor);

    // 2. Obtener el ID de la fuente
    int8_t fontId = Theme_ResolveFontId(lv->typo);
    int8_t iconFontId = Theme_ResolveFontId(TYPO_ICON); // Fuente para los iconos

    if (fontId < 0) return;

    // 3. Iterar solo sobre los elementos que caben en pantalla a partir del scrollOffset
    uint16_t i = 0;
    for (; i < lv->visibleItems; i++) {
        uint16_t dataIndex = lv->scrollOffset + i;
        
        // Si ya no hay más elementos, dejar de dibujar
        if (dataIndex >= lv->itemCount) break;

        int16_t itemY = lv->pos.y + (i * lv->itemHeight);

        // Dibujar línea separadora horizontal
        if (i > 0) {
            gfx_drawFastHLine(pBuf, lv->pos.x, itemY, lv->size.width, lv->lineColor);
        }

        gfx_ListViewItem *item = &lv->items[dataIndex];
        
        // Mapeo visual de Iconos
        char *iconStr = " ";
        uint16_t itemColor = lv->textColor;

        if (item->type == LV_ITEM_FOLDER || item->type == LV_ITEM_BACK) {
            iconStr = ICON_FOLDER; // Asume que existe en icon_map.h
            itemColor = g_pCurrentTheme->palette.primary; // Carpetas en color primario
        } else {
            iconStr = ICON_FILE;   // Asume que existe en icon_map.h
        }

        // Dibujar el icono (Margen izquierdo)
        FontEngine_DrawString(pBuf, iconFontId, lv->pos.x + 10, itemY + (lv->itemHeight / 2), 
                              iconStr, itemColor, (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER), 1);

        // Dibujar el texto del archivo/carpeta
        FontEngine_DrawString(pBuf, fontId, lv->pos.x + 70, itemY + (lv->itemHeight / 2), 
                              item->text, lv->textColor, (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER), 1);
    }

    lv->bIsDirty = false;
}

bool gfx_processListViewTouch(gfx_ListView *lv, TouchStatus touch) {
    if (!lv->bIsVisible) return false;
    bool eventHandled = false;

    // Verificar si el toque (x, y) cae dentro del rectángulo general de la ListView
    bool isTouchInside = (touch.x >= lv->pos.x && touch.x <= (lv->pos.x + lv->size.width) &&
                          touch.y >= lv->pos.y && touch.y <= (lv->pos.y + lv->size.height));

    // Si la pantalla está siendo tocada (Dedo abajo)
    // if (touch.state) {
    //     if (isTouchInside) {
    //         // Calcular qué fila se está tocando
    //         int16_t relativeY = touch.y - lv->pos.y;
    //         uint16_t clickedScreenIndex = relativeY / lv->itemHeight;
    //         
    //         // Validar que no estemos tocando el fondo vacío de la lista
    //         if (clickedScreenIndex < lv->visibleItems) {
    //             uint16_t dataIndex = lv->scrollOffset + clickedScreenIndex;
    //             
    //             if (dataIndex < lv->itemCount) {
    //                 // Animación de presionado (Feedback visual)
    //                 if (lv->_lastTouchedIndex != dataIndex) {
    //                     lv->_lastTouchedIndex = dataIndex;
    //                     lv->bIsDirty = true;
    //                 }
    //                 eventHandled = true;
    //             }
    //         }
    //     } else {
    //         // Si el dedo resbala hacia afuera de la lista, limpiamos la selección
    //         if (lv->_lastTouchedIndex != -1) {
    //             lv->_lastTouchedIndex = -1;
    //             lv->bIsDirty = true;
    //         }
    //     }
    // } 
    // Dedo levantado (Release)
    // else {
        //if (lv->_lastTouchedIndex != -1) {
            
            // Verificar si, al soltar, el dedo seguía en el MISMO elemento que presionó originalmente
            if (isTouchInside) {
                int16_t relativeY = touch.y - lv->pos.y;
                uint16_t clickedScreenIndex = relativeY / lv->itemHeight;
                uint16_t releaseDataIndex = lv->scrollOffset + clickedScreenIndex;

                //if (releaseDataIndex == lv->_lastTouchedIndex) {
                    // ================== LÓGICA DE NEGOCIO ==================
                    if (lv->onItemSelect != NULL) {
                        // lv->onItemSelect(lv, lv->_lastTouchedIndex);
                        lv->onItemSelect(lv, releaseDataIndex);
                    }
                //}
            }
            
            // Regresar a la normalidad visual
            lv->_lastTouchedIndex = -1;
            lv->bIsDirty = true;
            eventHandled = true;
        //}
    // }
    return eventHandled;
}

void gfx_drawDashedLine(pixel16_t *pBuf, int16_t x0, int16_t y0, int16_t x1,
                        int16_t y1, uint16_t color, uint8_t dashLen, uint8_t spaceLen) {
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    // --- LÓGICA DE SEGMENTACIÓN ---
    uint16_t patternLen = dashLen + spaceLen; 
    uint16_t pixelCount = 0; 
    // ------------------------------

    for (; x0 <= x1; x0++) {
        
        // Solo dibujamos si estamos dentro de la porción "dash" del patrón
        if ((pixelCount % patternLen) < dashLen) {
            if (steep) {
                gfx_write_pixel(pBuf, y0, x0, color);
            } else {
                gfx_write_pixel(pBuf, x0, y0, color);
            }
        }
        pixelCount++; // Avanzamos el contador del patrón en cada iteración

        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void gfx_drawFastHDashedLine(pixel16_t *pBuf, int16_t x, int16_t y, int16_t w,
                             uint16_t color, uint8_t dashLen, uint8_t spaceLen) {
    uint16_t patternLen = dashLen + spaceLen;
    int16_t i = 0;
    
    for (; i < w; i++) {
        if ((i % patternLen) < dashLen) {
            gfx_write_pixel(pBuf, x + i, y, color);
        }
    }
}

void gfx_drawFastVDashedLine(pixel16_t *pBuf, int16_t x, int16_t y, int16_t h,
                             uint16_t color, uint8_t dashLen, uint8_t spaceLen) {
    uint16_t patternLen = dashLen + spaceLen;
    int16_t i = 0;
    
    for (; i < h; i++) {
        if ((i % patternLen) < dashLen) {
            gfx_write_pixel(pBuf, x, y + i, color);
        }
    }
}
