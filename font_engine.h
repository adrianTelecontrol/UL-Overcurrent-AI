#ifndef FONT_ENGINE_H
#define FONT_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#include "render.h"

typedef struct {
  uint32_t bitmapOffset;
  uint8_t width;
  uint8_t height;
  int8_t xOffset;
  int8_t yOffset;
  uint8_t advanceX;
} BDF_Glyph_t;

typedef struct {
    uint16_t firstChar;
    uint16_t lastChar;
    uint8_t  yAdvance;      // Total line height (e.g., 31)
    int8_t   globalYOffset; // Distance from baseline to bottom (e.g., -6)
    BDF_Glyph_t *glyphs;
    uint8_t *pixelPool;
    uint32_t poolSize;
} BDF_Font_t;

typedef enum {
    // Horizontal Alignments (Lower Nibble)
    ALIGN_LEFT      = 0x01,
    ALIGN_RIGHT     = 0x02,
    ALIGN_HCENTER   = 0x04,
    
    // Vertical Alignments (Upper Nibble)
    ALIGN_TOP       = 0x10,
    ALIGN_BOTTOM    = 0x20,
    ALIGN_VCENTER   = 0x40,
    
    // Convenience Combinations
    ALIGN_CENTER    = (ALIGN_HCENTER | ALIGN_VCENTER)
} gfx_Align_e;

// Global font instance
typedef enum {
	FONT_FAM_ROBOTO = 0,
	FONT_FAM_INTER,
	FONT_FAM_BEBAS,
	FONT_FAM_MOMO,
	FONT_FAM_MONO,	
} gfx_FontFamily_e;

typedef enum {
	FONT_SIZE_16 = 0,
	FONT_SIZE_18,
	FONT_SIZE_24,
	FONT_SIZE_28,
	FONT_SIZE_32,
	FONT_SIZE_38,
	FONT_SIZE_42,
	FONT_SIZE_48,
} gfx_FontSize_e;

typedef enum {
	FONT_WEIGHT_REGULAR = 0,
	FONT_WEIGHT_BOLD,
	FONT_WEIGHT_ITALIC,
	FONT_WEIGHT_BLACK,
	FONT_WEIGHT_ICON,
} gfx_FontWeight_e;

typedef struct {
	gfx_FontFamily_e family;
	gfx_FontWeight_e weight;
	uint8_t ui8Scale;
	uint16_t ui16Color;
	gfx_FontSize_e size;
} gfx_Font_t;

typedef struct {
	bool isLoaded;
	gfx_FontFamily_e family;
	gfx_FontWeight_e weight;
	gfx_FontSize_e size;
	BDF_Font_t bdfData;
} gfx_FontSlot_t;

#define MAX_LOADED_FONTS 10

extern gfx_FontSlot_t g_FontCache[MAX_LOADED_FONTS];

// BDF_Font_t g_SystemFont[FONT_NUMBER];

static inline uint8_t BDF_HexToByte(const char *hex) {
  uint8_t val = 0;
  int i = 0;
  for (; i < 2; i++) {
    uint8_t c = hex[i];
    if (c >= '0' && c <= '9')
      val = (val << 4) | (c - '0');
    else if (c >= 'A' && c <= 'F')
      val = (val << 4) | (c - 'A' + 10);
    else if (c >= 'a' && c <= 'f')
      val = (val << 4) | (c - 'a' + 10);
  }
  return val;
}

void FontEngine_GetStringDimensions(const char *str, int8_t fontId, uint16_t *pWidth, uint16_t *pHeight, uint8_t scale);

int8_t FontEngine_FontLoadDynamic(gfx_FontFamily_e family, gfx_FontWeight_e weight, uint8_t size);

void FontEngine_DrawChar(pixel16_t *pBuffer, int8_t fontId, int16_t *cursorX, int16_t cursorY, char c, uint16_t color);

void FontEngine_DrawCharScaled(pixel16_t *pBuffer, uint8_t fontId, int16_t *cursorX, int16_t cursorY, char c, uint16_t color, uint16_t scale);

void FontEngine_DrawString(pixel16_t *pBuffer, int8_t fontId, int16_t x, int16_t y, const char *str, uint16_t color, uint8_t alignment, uint8_t scale);

#endif // FONT_ENGINE_H


