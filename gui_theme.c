
#include <stddef.h>

#include "event_engine.h"
#include "gui_theme.h"
#include "gui_colors.h"
#include "font_engine.h"

#include "helpers.h"

gfx_Theme_t *g_pCurrentTheme = NULL;

static const char *TASK_NAME = "gfx_theme";

// ---------------------------------------------------------
// THEME 1: INDUSTRIAL LIGHT MODE
// ---------------------------------------------------------
gfx_Theme_t g_ThemeLight = {
    .palette.background = COLOR_WHITE,
    .palette.surface    = COLOR_GRAY_50,
    .palette.primary    = COLOR_BLUE_PRIMARY,
    .palette.secondary  = COLOR_GRAY_200,
    .palette.textMain   = COLOR_BLACK,
    .palette.textMuted  = COLOR_GRAY_500,
    .palette.border     = COLOR_GRAY_400,
    .palette.danger     = COLOR_RED_ALERT,
    .palette.success    = COLOR_GREEN_OK
};

// ---------------------------------------------------------
// THEME 2: INDUSTRIAL DARK MODE
// ---------------------------------------------------------
gfx_Theme_t g_ThemeDark = {
    .palette.background = COLOR_GRAY_900,
    .palette.surface    = COLOR_GRAY_400,
    .palette.primary    = COLOR_CYAN_NEON,
    .palette.secondary  = COLOR_GRAY_700,
    .palette.textMain   = COLOR_WHITE,
    .palette.textMuted  = COLOR_GRAY_550,
    .palette.border     = COLOR_GRAY_600,
    .palette.danger     = COLOR_RED_MUTED,
    .palette.success    = COLOR_GREEN_MUTED
};

gfx_Theme_t g_ThemeSoftLight = {
    .palette.background = COLOR_OFFWHITE_BLUE, // Fondo que descansa la vista
    .palette.surface    = COLOR_WHITE,         // Los paneles (como tu bgPanel) flotan en blanco puro
    .palette.primary    = COLOR_TEAL_PRIMARY,  // Botones de acción principal
    .palette.secondary  = COLOR_BLUE_GRAY_200, // Botones secundarios
    .palette.textMain   = COLOR_BLUE_GRAY_900, // Texto oscuro (mejor contraste que negro puro)
    .palette.textMuted  = COLOR_BLUE_GRAY_600, // Texto secundario
    .palette.border     = COLOR_BLUE_GRAY_400, // Bordes sutiles
    .palette.danger     = COLOR_SOFT_RED,      // Alertas
    .palette.success    = COLOR_SOFT_GREEN     // Estados OK
};

gfx_Theme_t g_TelecontrolDarkTheme = {
    .palette = {
        .background = 0x0841, // Original: #0a0b0d (Negro profundo industrial)
        .surface    = 0x10C4, // Original: #161b22 (Fondo de paneles/tarjetas)
        .primary    = 0xF800, // Original: #FF0000 (Rojo corporativo Telecontrol)
        .secondary  = 0x5D3F, // Original: #58a6ff (Azul claro para información)
        .textMain   = 0xFFFF, // Original: #FFFFFF (Blanco puro, máxima legibilidad HMI)
        .textMuted  = 0x8CB3, // Original: #8b949e (Gris azulado para unidades y etiquetas secundarias)
        .border     = 0x31A7, // Original: #30363d (Gris medio para bordes de botones y separadores)
        .danger     = 0xF800, // Original: #FF0000 (Mismo que el primary por reglas de hardware de alarmas)
        .success    = 0x268C  // Original: #23d160 (Verde estilo LED de alta intensidad)
    },
    .fonts = {
        .currentFamily = FONT_FAM_MONO, // O el nombre de tu fuente compilada principal
        .h1 = FONT_SIZE_48,      // Títulos de pantalla (ej. "PARO DE EMERGENCIA")
        .h2 = FONT_SIZE_42,      // Subtítulos de grupo y botones principales
		.h3 = FONT_SIZE_32,
        .body = FONT_SIZE_24,    // Texto normal de navegación y botones estándar
        .caption = FONT_SIZE_16, // Pequeñas etiquetas (ej. "Corriente Nominal [A]")
        .mono = FONT_SIZE_18,     // Tipografía JetBrains para los indicadores en tiempo real (ej. 100.30 C)
		.mono_bold = FONT_SIZE_18,
		.icon = FONT_SIZE_48,
    }
};

static void onChangeThemeEvent(EventParam_t param) {
	gfx_FontFamily_e activeFamily = g_pCurrentTheme->fonts.currentFamily;

	if(g_pCurrentTheme == &g_ThemeDark) {
		g_pCurrentTheme = &g_ThemeLight;
	} else {
		g_pCurrentTheme = &g_ThemeDark;
	}

    Theme_SetFontFamily(activeFamily);

	Event_Post(EVT_CMD_FULL_REPAINT, (EventParam_t){.ptr = NULL});
}

// Función de ayuda para resolver el fontId basado en el tema actual
int8_t Theme_ResolveFontId(gfx_TypoStyle_e typo) {
    if (g_pCurrentTheme == NULL) return -1;
    
    switch (typo) {
        case TYPO_H1: return g_pCurrentTheme->fonts.h1;
        case TYPO_H2: return g_pCurrentTheme->fonts.h2;
		case TYPO_H3: return g_pCurrentTheme->fonts.h3;
        case TYPO_BODY: return g_pCurrentTheme->fonts.body;
        case TYPO_CAPTION: return g_pCurrentTheme->fonts.caption;
        case TYPO_MONO: return g_pCurrentTheme->fonts.mono;
		case TYPO_ICON: return g_pCurrentTheme->fonts.icon;
        default: return -1;
    }
}

// ---------------------------------------------------------
// INITIALIZATION
// ---------------------------------------------------------
void Theme_Init(bool isDark) {
    // gfx_FontFamily_e activeFamily = g_pCurrentTheme->fonts.currentFamily;
	

    if (isDark) {
        // g_pCurrentTheme = &g_ThemeDark;
        g_pCurrentTheme = &g_TelecontrolDarkTheme;
    } else {
        g_pCurrentTheme = &g_ThemeLight;
    }
    // Cargar la familia de fuentes por defecto al arrancar
    // Theme_SetFontFamily(FONT_FAM_INTER);
    //Theme_SetFontFamilyLow(FONT_FAM_INTER);

	Event_Subscribe(EVT_CMD_CHANGE_THEME, (EventHandler_fn) onChangeThemeEvent);
}

void Theme_SetMode(bool isDark) {
    gfx_FontFamily_e activeFamily = g_pCurrentTheme->fonts.currentFamily;

    if (isDark) {
        //g_pCurrentTheme = &g_ThemeDark;
        g_pCurrentTheme = &g_TelecontrolDarkTheme;
    } else {
        g_pCurrentTheme = &g_ThemeLight;
    }

    // Transferir la familia activa al nuevo tema y recargar los punteros
    Theme_SetFontFamilyLow(activeFamily);
}

void Theme_SetModeLow(bool isDark) {
    gfx_FontFamily_e activeFamily = g_pCurrentTheme->fonts.currentFamily;

    if (isDark) {
        //g_pCurrentTheme = &g_ThemeDark;
        g_pCurrentTheme = &g_TelecontrolDarkTheme;
    } else {
        g_pCurrentTheme = &g_ThemeLight;
    }

    // Transferir la familia activa al nuevo tema y recargar los punteros
    Theme_SetFontFamilyLow(activeFamily);
}

void Theme_SetModeHigh(bool isDark) {
    gfx_FontFamily_e activeFamily = g_pCurrentTheme->fonts.currentFamily;

    if (isDark) {
        // g_pCurrentTheme = &g_ThemeDark;
        g_pCurrentTheme = &g_TelecontrolDarkTheme;
    } else {
        g_pCurrentTheme = &g_ThemeLight;
    }

    // Transferir la familia activa al nuevo tema y recargar los punteros
    Theme_SetFontFamilyHigh(activeFamily);
}

void Theme_PreloadFonts(void)
{
    FontEngine_FontLoadDynamic(FONT_FAM_INTER, FONT_WEIGHT_BOLD,    FONT_SIZE_48);
    FontEngine_FontLoadDynamic(FONT_FAM_INTER, FONT_WEIGHT_BOLD,    FONT_SIZE_32);
    FontEngine_FontLoadDynamic(FONT_FAM_INTER, FONT_WEIGHT_REGULAR, FONT_SIZE_24);
    FontEngine_FontLoadDynamic(FONT_FAM_INTER, FONT_WEIGHT_REGULAR, FONT_SIZE_18);

    TIVA_LOGI(TASK_NAME, "Font pre-load complete.");
}

void Theme_SetFontFamily(gfx_FontFamily_e newFamily) {
    if (!g_pCurrentTheme) return;

    g_pCurrentTheme->fonts.currentFamily = newFamily;

    // Cargar dinámicamente desde la SD y guardar los IDs en el tema
    g_pCurrentTheme->fonts.h1      = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_BOLD, g_pCurrentTheme->fonts.h1);
    g_pCurrentTheme->fonts.h2      = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_BOLD, g_pCurrentTheme->fonts.h2);
	g_pCurrentTheme->fonts.h3	   = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_BOLD, g_pCurrentTheme->fonts.h3); 
    g_pCurrentTheme->fonts.body    = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_REGULAR, g_pCurrentTheme->fonts.body);
    g_pCurrentTheme->fonts.caption = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_REGULAR, g_pCurrentTheme->fonts.caption);
	g_pCurrentTheme->fonts.icon    = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_ICON, g_pCurrentTheme->fonts.icon);
    
    g_pCurrentTheme->fonts.mono    = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_REGULAR, g_pCurrentTheme->fonts.mono); 
    g_pCurrentTheme->fonts.mono_bold    = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_BOLD, g_pCurrentTheme->fonts.mono_bold); 
}

void Theme_SetFontFamilyHigh(gfx_FontFamily_e newFamily) {
    if (!g_pCurrentTheme) return;

    g_pCurrentTheme->fonts.currentFamily = newFamily;

    // Cargar dinámicamente desde la SD y guardar los IDs en el tema
    g_pCurrentTheme->fonts.h1      = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_BOLD, g_pCurrentTheme->fonts.h1);
	g_pCurrentTheme->fonts.h3	   = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_BOLD, g_pCurrentTheme->fonts.h3); 
    // g_pCurrentTheme->fonts.h2      = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_BOLD, FONT_SIZE_32);
    // g_pCurrentTheme->fonts.body    = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_REGULAR, FONT_SIZE_24);
    // g_pCurrentTheme->fonts.caption = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_REGULAR, FONT_SIZE_18);
    
    // // Nota: La fuente mono suele mantenerse constante sin importar el tema general
    // // para asegurar que los números no salten.
    g_pCurrentTheme->fonts.mono    = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_REGULAR, g_pCurrentTheme->fonts.mono); 
    g_pCurrentTheme->fonts.mono_bold    = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_BOLD, g_pCurrentTheme->fonts.mono_bold); 
}

void Theme_SetFontFamilyLow(gfx_FontFamily_e newFamily) {
    if (!g_pCurrentTheme) return;

    g_pCurrentTheme->fonts.currentFamily = newFamily;

    // Cargar dinámicamente desde la SD y guardar los IDs en el tema
    //g_pCurrentTheme->fonts.h1      = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_BOLD, FONT_SIZE_48);
    g_pCurrentTheme->fonts.h2      = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_BOLD, g_pCurrentTheme->fonts.h2);
    g_pCurrentTheme->fonts.body    = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_REGULAR, g_pCurrentTheme->fonts.body);
    g_pCurrentTheme->fonts.caption = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_REGULAR, g_pCurrentTheme->fonts.caption);
	g_pCurrentTheme->fonts.icon    = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_ICON, g_pCurrentTheme->fonts.icon);
    
    // Nota: La fuente mono suele mantenerse constante sin importar el tema general
    // para asegurar que los números no salten.
    // g_pCurrentTheme->fonts.mono    = FontEngine_FontLoadDynamic(newFamily, FONT_WEIGHT_REGULAR, FONT_SIZE_24); 
}

