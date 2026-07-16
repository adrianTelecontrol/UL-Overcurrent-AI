#ifndef GFX_THEME_H
#define GFX_THEME_H

#include <stdbool.h>
#include <stdint.h>
#include "font_engine.h"

// Semantic color palette
typedef struct {
    uint16_t background;
    uint16_t surface;
    uint16_t primary;
    uint16_t secondary;
    uint16_t textMain;
    uint16_t textMuted;
    uint16_t border;
    uint16_t danger;
    uint16_t success;
    uint16_t warning;
} gfx_Palette_t;

// Semantic Typography
typedef struct {
    gfx_FontFamily_e currentFamily; // Rastrea la familia global activa
    int8_t h1;                      // Títulos grandes (ej. 42px)
    int8_t h2;                      // Subtítulos (ej. 32px)
	int8_t h3;
    int8_t body;                    // Texto estándar para botones (ej. 24px)
    int8_t caption;                 // Texto pequeño (ej. 18px)
    int8_t mono;                    // Datos dinámicos (siempre monoespaciada)
	int8_t mono_bold;
	int8_t icon;
} gfx_Typography_t;

typedef struct {
    gfx_Palette_t palette;
    gfx_Typography_t fonts;
} gfx_Theme_t;

// Enumerador semántico para los widgets
typedef enum {
    TYPO_H1 = 0,
    TYPO_H2,
	TYPO_H3,
    TYPO_BODY,
    TYPO_CAPTION,
    TYPO_MONO,
	TYPO_MONO_BOLD,
	TYPO_ICON,
} gfx_TypoStyle_e;

extern gfx_Theme_t *g_pCurrentTheme;

int8_t Theme_ResolveFontId(gfx_TypoStyle_e typo);
void Theme_Init(bool isDark);
void Theme_SetMode(bool isDark);
void Theme_SetFontFamily(gfx_FontFamily_e newFamily);
void Theme_SetFontFamilyHigh(gfx_FontFamily_e newFamily);
void Theme_SetFontFamilyLow(gfx_FontFamily_e newFamily);
void Theme_SetModeLow(bool isDark);
void Theme_SetModeHigh(bool isDark);

#endif // GFX_THEME_H




