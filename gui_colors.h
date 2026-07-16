#ifndef GUI_COLORS_H_
#define GUI_COLORS_H_

// ---------------------------------------------------------
// NEUTRALS & GRAYS (RGB565)
// ---------------------------------------------------------
#define COLOR_WHITE             0xFFFF
#define COLOR_BLACK             0x0000

// Light Theme Neutrals
#define COLOR_GRAY_50           0xEF7D  // Very Light Gray (Surface)
#define COLOR_GRAY_200          0xCE79  // Silver/Gray (Secondary Buttons)
#define COLOR_GRAY_400          0xBDD7  // Darker Silver (Borders)
#define COLOR_GRAY_500          0x7BEF  // Medium Gray (Muted Text)

// Dark Theme Neutrals
#define COLOR_GRAY_900          0x10A2  // Very Dark Charcoal (Dark Background)
#define COLOR_GRAY_800          0x2124  // Dark Gray (Dark Surface)
#define COLOR_GRAY_700          0x4208  // Medium-Dark Gray (Dark Secondary)
#define COLOR_GRAY_600          0x52AA  // Lighter Gray (Dark Borders)
#define COLOR_GRAY_550          0x8410  // Dim Gray (Dark Muted Text)

// ---------------------------------------------------------
// BRAND / ACCENT COLORS
// ---------------------------------------------------------
#define COLOR_BLUE_PRIMARY      0x035A  // Deep Corporate Blue
#define COLOR_CYAN_NEON         0x0AEF  // Bright Cyan for Dark Mode contrast

// ---------------------------------------------------------
// STATUS & ALARM COLORS
// ---------------------------------------------------------
#define COLOR_RED_ALERT         0xF800  // Standard bright red
#define COLOR_RED_MUTED         0xD000  // Less aggressive red for dark mode
#define COLOR_GREEN_OK          0x07E0  // Standard bright green
#define COLOR_GREEN_MUTED       0x05E0  // Softer green for dark mode

// ---------------------------------------------------------
// SOFT TEAL THEME (Medical / Modern Light)
// ---------------------------------------------------------
#define COLOR_OFFWHITE_BLUE     0x07DF  // Fondo principal (#F2F4F8) - Reduce el brillo
#define COLOR_TEAL_PRIMARY      0x04A1  // Teal / Verde Azulado (#009688) - Elegante y calmante
#define COLOR_BLUE_GRAY_200     0xCEDB  // Gris-azulado claro para botones secundarios
#define COLOR_BLUE_GRAY_400     0xB5F8  // Gris-azulado para bordes suaves
#define COLOR_BLUE_GRAY_600     0x7C93  // Gris-azulado medio para texto mutado/deshabilitado
#define COLOR_BLUE_GRAY_900     0x2187  // Casi negro con toque azul para el texto principal
#define COLOR_SOFT_RED          0xE1C6  // Rojo menos agresivo pero visible (#E53935)
#define COLOR_SOFT_GREEN        0x4508  // Verde hoja para éxito (#43A047)

// ---------------------------------------------------------
// HELPER MACROS
// ---------------------------------------------------------
// Assuming you already have this somewhere, but good to keep in the colors file!
// #define DARKEN_COLOR(c)         ((((c) & 0xF7DE) >> 1) | (((c) & 0x0821) ? 0x0000 : 0))

// Darkens any RGB565 color by 50% safely and instantly
#define DARKEN_COLOR(c) (((c) & 0xF7DE) >> 1)
#define LIGHTEN_COLOR(c) ((c) - (((c) & 0xE79C) >> 2) + 0x39E7)
#endif // GUI_COLORS_H_


