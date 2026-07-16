/**
 * font_engine.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "helpers.h"
#include "hal_usd.h"
#include "file_manager.h"

#include "FT8xx_params.h"

#include "font_engine.h"

// const char *FONT_BEBAS_PATH = "BEBAS32.BDF";
//  const char *FONT_BEBAS_PATH = "FONTS/INTER/INTREG32.bdf";
const char *FONT_BEBAS_PATH = "FONTS/INTER/INTBLD48.bdf";
const char *FONT_ROBOTO_PATH = "ROBOTO.BDF";

static const char TASK_NAME[] = "fontEngine";

gfx_FontSlot_t g_FontCache[MAX_LOADED_FONTS] = {0};

void FontEngine_GetStringDimensions(const char *str, int8_t fontId, uint16_t *pWidth, uint16_t *pHeight, uint8_t scale) {
    uint16_t maxWidth = 0;
    uint16_t currentLineWidth = 0;
    uint16_t lineCount = 1; // Al menos hay una línea, incluso si está vacía

    // Safety checks + Cache validation
    if (!str || !pWidth || !pHeight || fontId < 0 || fontId >= MAX_LOADED_FONTS || !g_FontCache[fontId].isLoaded) {
        if (pWidth) *pWidth = 0;
        if (pHeight) *pHeight = 0;
        return;
    }

    // Point directly to the loaded BDF data in the cache
    BDF_Font_t *sFont = &g_FontCache[fontId].bdfData;

    // Calculate total advanceX and track newlines
    while (*str) {
        char c = *str;
        
        if (c == '\n') {
            // Al encontrar un salto de línea, evaluamos si la línea actual es la más ancha
            if (currentLineWidth > maxWidth) {
                maxWidth = currentLineWidth;
            }
            currentLineWidth = 0; // Reiniciamos el contador para la siguiente línea
            lineCount++;          // Incrementamos el total de líneas
        } 
        else if (c >= sFont->firstChar && c <= sFont->lastChar) {
            uint16_t charIndex = c - sFont->firstChar;
            currentLineWidth += sFont->glyphs[charIndex].advanceX;
        }
        str++;
    }

    // Verificamos la última línea en caso de que el string no termine con un '\n'
    if (currentLineWidth > maxWidth) {
        maxWidth = currentLineWidth;
    }

    // Protect against scale being 0
    if (scale == 0) scale = 1;

    // Pass the scaled dimensions back
    *pWidth = maxWidth * scale;
    *pHeight = (sFont->yAdvance * lineCount) * scale;
}


void FontEngine_DrawChar(pixel16_t *pBuffer, int8_t fontId, int16_t *cursorX,
                  int16_t cursorY, char c, uint16_t color) {
  if (fontId < 0 || fontId >= MAX_LOADED_FONTS || !g_FontCache[fontId].isLoaded)
    return;

  BDF_Font_t sFont = g_FontCache[fontId].bdfData;

  if (c < sFont.firstChar || c > sFont.lastChar)
    c = '?';

  uint16_t charIndex = c - sFont.firstChar;
  BDF_Glyph_t glyph = sFont.glyphs[charIndex];

  int16_t drawX = *cursorX + glyph.xOffset;
  int16_t drawY = cursorY + glyph.yOffset;

  uint8_t *charPixels = &sFont.pixelPool[glyph.bitmapOffset];
  uint16_t byteIndex = 0;

  // HIGH-SPEED 1:1 RENDER LOOP
  int row = 0;
  for (; row < glyph.height; row++) {
    int16_t screenY = drawY + row;

    // Skip entirely if the row is off-screen
    if (screenY < 0 || screenY >= LCD_HEIGHT) {
      byteIndex += (glyph.width + 7) / 8;
      continue;
    }

    uint32_t rowOffset = screenY * LCD_WIDTH;

    int col = 0;
    for (; col < glyph.width; col++) {
      uint8_t bitMask = 0x80 >> (col % 8);
      uint8_t currentByte = charPixels[byteIndex + (col / 8)];

      // If the bit is set, draw exactly one pixel
      if (currentByte & bitMask) {
        int16_t screenX = drawX + col;

        if (screenX >= 0 && screenX < LCD_WIDTH) {
          pBuffer[rowOffset + screenX].u16 =
              color; // Assign directly, no scaling math!
        }
      }
    }
    byteIndex += (glyph.width + 7) / 8;
  }

  *cursorX += glyph.advanceX;
}

void FontEngine_DrawCharScaled(pixel16_t *pBuffer, uint8_t fontId, int16_t *cursorX,
                        int16_t cursorY, char c, uint16_t color,
                        uint16_t scale) {

  if (fontId >= MAX_LOADED_FONTS)
    return;

  BDF_Font_t sFont = g_FontCache[fontId].bdfData;

  // 1. Bounds check and default
  if (c < sFont.firstChar || c > sFont.lastChar)
    c = '?';

  // 2. Look up the glyph
  uint16_t charIndex = c - sFont.firstChar;
  BDF_Glyph_t glyph = sFont.glyphs[charIndex];

  // 3. Scale the offsets!
  int16_t drawX = *cursorX + (glyph.xOffset * scale);
  int16_t drawY = cursorY + (glyph.yOffset * scale);

  // 4. Point to the pixel pool
  uint8_t *charPixels = &sFont.pixelPool[glyph.bitmapOffset];

  uint16_t byteIndex = 0;

  int row = 0;
  for (; row < glyph.height; row++) {
    int col = 0;
    for (; col < glyph.width; col++) {

      // Check if this specific bit is a 1
      uint8_t bitMask = 0x80 >> (col % 8);
      uint8_t currentByte = charPixels[byteIndex + (col / 8)];

      if (currentByte & bitMask) {
        // --- SCALING LOGIC ---
        // Instead of drawing 1 pixel, draw a block of (scale x scale) pixels
        int16_t baseX = drawX + (col * scale);
        int16_t baseY = drawY + (row * scale);

        int sy = 0;
        for (; sy < scale; sy++) {
          int16_t screenY = baseY + sy;

          // Vertical bounds check
          if (screenY < 0 || screenY >= LCD_HEIGHT)
            continue;

          uint32_t rowOffset = screenY * LCD_WIDTH;

          int sx = 0;
          for (; sx < scale; sx++) {
            int16_t screenX = baseX + sx;

            // Horizontal bounds check
            if (screenX >= 0 && screenX < LCD_WIDTH) {
              pBuffer[rowOffset + screenX].u16 = color;
            }
          }
        }
      }
    }
    // Advance to the next row of data in the BDF array
    byteIndex += (glyph.width + 7) / 8;
  }

  // 6. Advance the cursor scaled!
  *cursorX += (glyph.advanceX * scale);
}

void FontEngine_DrawString(pixel16_t *pBuffer, int8_t fontId, int16_t x, int16_t y, 
                           const char *str, uint16_t color, uint8_t alignment, uint8_t scale) 
{
    // Safety checks + Cache validation
    if (!str || !pBuffer || fontId < 0 || fontId >= MAX_LOADED_FONTS || !g_FontCache[fontId].isLoaded) return;

    if (scale == 0) scale = 1;

    BDF_Font_t *sFont = &g_FontCache[fontId].bdfData;

    // 1. Calculate Line Height and Block Dimensions
    int16_t lineHeight = sFont->yAdvance * scale;
    int16_t numLines = 1;
    
    // Contar cuántas líneas hay en total
    const char *p = str;
    while (*p) {
        if (*p == '\n') numLines++;
        p++;
    }
    int16_t blockHeight = numLines * lineHeight;

    // 2. Adjust Initial Y (Vertical Alignment applies to the WHOLE block)
    int16_t ascent = (sFont->yAdvance + sFont->globalYOffset) * scale;
    int16_t descent = (sFont->globalYOffset) * scale; 
    
    int16_t startY = y;
    if (alignment & ALIGN_TOP) {
        startY = y + ascent; 
    } 
    else if (alignment & ALIGN_VCENTER) {
        startY = y + ascent - (blockHeight / 2);
    }
    else if (alignment & ALIGN_BOTTOM) {
        // El fondo del bloque toca 'y'. Subimos la primera línea basándonos en la cantidad total de líneas.
        startY = y + descent - ((numLines - 1) * lineHeight); 
    }

    // 3. Render Line by Line
    const char *lineStart = str;
    int16_t currentY = startY;

    while (*lineStart) {
        // Encontrar el final de la línea actual
        const char *lineEnd = lineStart;
        while (*lineEnd && *lineEnd != '\n') lineEnd++;

        // A. Calcular el ancho exacto de ESTA línea específica
        uint16_t lineWidth = 0;
        const char *c = lineStart;
        while (c < lineEnd) {
            // Ignoramos retornos de carro (Windows \r) y sumamos el avance de cada letra
            if (*c != '\r' && *c >= sFont->firstChar && *c <= sFont->lastChar) {
                lineWidth += sFont->glyphs[*c - sFont->firstChar].advanceX * scale;
            }
            c++;
        }

        // B. Ajustar el X inicial para esta línea (Alineación Horizontal)
        int16_t cursorX = x;
        if (alignment & ALIGN_RIGHT) {
            cursorX = x - lineWidth;
        } else if (alignment & ALIGN_HCENTER) {
            cursorX = x - (lineWidth / 2);
        }

        // C. Renderizar los caracteres de esta línea
        c = lineStart;
        while (c < lineEnd) {
            if (*c != '\r') { // Ignorar '\r' para evitar basura en pantalla
                if (scale > 1) {
                    FontEngine_DrawCharScaled(pBuffer, fontId, &cursorX, currentY, *c, color, scale);
                } else {
                    FontEngine_DrawChar(pBuffer, fontId, &cursorX, currentY, *c, color);
                }
            }
            c++;
        }

        // D. Avanzar a la siguiente línea
        currentY += lineHeight;
        lineStart = *lineEnd ? lineEnd + 1 : lineEnd; // Saltar el '\n' si no estamos al final
    }
}

/*
void FontEngine_DrawString(pixel16_t *pBuffer, int8_t fontId, int16_t x, int16_t y, 
                    const char *str, uint16_t color, uint8_t alignment, uint8_t scale) 
{
    // Safety checks + Cache validation
    if (!str || !pBuffer || fontId < 0 || fontId >= MAX_LOADED_FONTS || !g_FontCache[fontId].isLoaded) return;

    if (scale == 0) scale = 1;

    BDF_Font_t *sFont = &g_FontCache[fontId].bdfData;

    // 1. Calculate the scaled dimensions
    uint16_t totalWidth = 0, totalHeight = 0;
    FontEngine_GetStringDimensions(str, fontId, &totalWidth, &totalHeight, scale);
    
    // 2. Adjust starting X (Horizontal Alignment)
    int16_t cursorX = x;
    if (alignment & ALIGN_RIGHT) {
        cursorX = x - totalWidth;
    } else if (alignment & ALIGN_HCENTER) {
        cursorX = x - (totalWidth / 2);
    }

    // 3. Adjust starting Y (Vertical Alignment using Typography Math!)
    int16_t cursorY = y;
    
    // Ascent is how far the text goes ABOVE the baseline. 
    int16_t ascent = (sFont->yAdvance + sFont->globalYOffset) * scale;
    
    // Descent is how far the text goes BELOW the baseline (Usually negative).
    int16_t descent = (sFont->globalYOffset) * scale; 

    if (alignment & ALIGN_TOP) {
        // Push the baseline down so the top of the letters touch 'y'
        cursorY = y + ascent; 
    } 
    else if (alignment & ALIGN_VCENTER) {
        // Push baseline down by ascent, then pull it back up by half the total box
        cursorY = y + ascent - (totalHeight / 2);
    }
    else if (alignment & ALIGN_BOTTOM) {
        // Since descent is negative, adding it pulls the baseline UP from 'y'
        cursorY = y + descent; 
    }

    // 4. Fast, single-pass render loop
    while (*str) {
        if (scale > 1) {
            // Use the fallback scaler if requested
            FontEngine_DrawCharScaled(pBuffer, fontId, &cursorX, cursorY, *str, color, scale);
        } else {
            // HIGH-SPEED PATH: Use the 1:1 renderer
            FontEngine_DrawChar(pBuffer, fontId, &cursorX, cursorY, *str, color);
        }
        str++;
    }
}*/

int8_t FontEngine_FontLoadDynamic(gfx_FontFamily_e family, gfx_FontWeight_e weight,
                           gfx_FontSize_e size) {
  // 1. Check if this exact font is already loaded in cache
	int i = 0;
  for (; i < MAX_LOADED_FONTS; i++) {
    if (g_FontCache[i].isLoaded && g_FontCache[i].family == family &&
        g_FontCache[i].weight == weight && g_FontCache[i].size == size) {
      return i; // Return the existing Font ID
    }
  }

  // 2. Find an empty slot
  int8_t freeSlot = -1;
  for (i = 0; i < MAX_LOADED_FONTS; i++) {
    if (!g_FontCache[i].isLoaded) {
      freeSlot = i;
      break;
    }
  }

  if (freeSlot == -1) {
    TIVA_LOGE(TASK_NAME, "Font Cache is full! Cannot load new font.");
    return -1;
  }

  // 3. Build the file path dynamically
  // Example: "FONTS/INTER/BOLD_24.BDF"
  char filePath[64];
  const char *familyStr = "";
  const char *weightStr = "";

  switch (family) {
  case FONT_FAM_INTER:
    familyStr = "INTER";
    break;
  case FONT_FAM_ROBOTO:
    familyStr = "ROBOTO";
    break;
  case FONT_FAM_BEBAS:
    familyStr = "BEBAS";
    break;
  case FONT_FAM_MOMO:
    familyStr = "MOMO";
    break;
  case FONT_FAM_MONO:
    familyStr = "MONO";
    break;
  }

  switch (weight) {
  case FONT_WEIGHT_REGULAR:
    weightStr = "REG";
    break;
  case FONT_WEIGHT_BOLD:
    weightStr = "BOLD";
    break;
  case FONT_WEIGHT_ITALIC:
    weightStr = "ITA";
    break;
  case FONT_WEIGHT_BLACK:
    weightStr = "BLK";
    break;
  case FONT_WEIGHT_ICON:
  	weightStr = "ICONS";
  	break;
  default:
    weightStr = "REG";
    break;
  }
  
  uint8_t numSize = 0;
  switch(size)
  {
	case FONT_SIZE_16: numSize = 16; break;
	case FONT_SIZE_18: numSize = 18; break;
	case FONT_SIZE_24: numSize = 24; break;
	case FONT_SIZE_28: numSize = 28; break;
	case FONT_SIZE_32: numSize = 32; break;
	case FONT_SIZE_38: numSize = 38; break;
	case FONT_SIZE_42: numSize = 42; break;
	case FONT_SIZE_48: numSize = 48; break;
	default: numSize = 18; break;
	
  }

  snprintf(filePath, sizeof(filePath), "FONTS/%s/%s_%d.BDF", familyStr,
           weightStr, numSize);

  //TIVA_LOGI(TASK_NAME, "Attempting to load: %s", filePath);

  // 4. Load from SD SPI
  if (FM_FetchBDF(DRIVE_SD_ID, filePath, &g_FontCache[freeSlot].bdfData, 32, 126)) {
    g_FontCache[freeSlot].family = family;
    g_FontCache[freeSlot].weight = weight;
    g_FontCache[freeSlot].size = size;
    g_FontCache[freeSlot].isLoaded = true;

    return freeSlot; // Return the new Font ID
  }

  TIVA_LOGE(TASK_NAME, "Failed to load font from SD.");
  return -1;
}
