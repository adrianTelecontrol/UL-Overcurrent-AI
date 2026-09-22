# Theme Change Workflow

## Purpose

The UI theme system changes the active palette, reloads the active font IDs, and
forces a full framebuffer repaint. This prevents the previous theme from
remaining in regions that did not have a dirty widget.

## User flow

1. User presses `TEMA` in the **Opciones** form.
2. `onThemeBtnReleasedEvent()` posts `EVT_CMD_CHANGE_THEME`.
3. `hge_platform.c` forwards that event to the callback registered by
   `Theme_Init()`.
4. `onChangeThemeEvent()` in `gui_theme.c` selects the next entry in its
   `themes[]` array.
5. The function preserves the active font family with `Theme_SetFontFamily()`.
6. `HgePlatform_RequestFullRepaint()` posts `EVT_CMD_FULL_REPAINT`.
7. The renderer composites the full form and sends a complete framebuffer to
   the FT812 display.

## Theme definitions

Themes are `gfx_Theme_t` objects in
`third_party/hybrid_graphics_engine/engine/tm4c1294_ft812/gui_theme.c`.

Each palette uses semantic fields:

| Field | Use |
| --- | --- |
| `background` | Canvas background |
| `surface` | Cards, panels, and input boxes |
| `primary` | Primary action and highlighted values |
| `secondary` | Secondary action and headings |
| `textMain` | Main text |
| `textMuted` | Supporting text |
| `border` | Borders and separators |
| `danger` | Errors and destructive actions |
| `warning` | Warnings and pending state |
| `success` | Success state |
| `emphasis` | Strong status emphasis |

## Add a theme

1. Define a new `gfx_Theme_t` in `gui_theme.c`.
2. Set every palette field. Do not leave fields at zero.
3. Add the new theme pointer to `onChangeThemeEvent()`'s `themes[]` array in
   the intended cycle order.
4. Build the firmware and test each form with `TEMA`.

Example:

```c
gfx_Theme_t g_ThemeExample = {
    .palette.background = COLOR_GRAY_900,
    .palette.surface    = COLOR_GRAY_800,
    .palette.primary    = COLOR_CYAN_300,
    .palette.secondary  = COLOR_SLATE_700,
    .palette.textMain   = COLOR_WHITE,
    .palette.textMuted  = COLOR_SLATE_300,
    .palette.border     = COLOR_SLATE_500,
    .palette.danger     = COLOR_RED_300,
    .palette.warning    = COLOR_AMBER_300,
    .palette.success    = COLOR_GREEN_300,
    .palette.emphasis   = COLOR_CYAN_300,
};
```

Then add `&g_ThemeExample` to `themes[]`.

## Widget rules

Use semantic widget styles whenever possible:

```c
.style = STYLE_PRIMARY
.style = STYLE_TEXT_MAIN
.style = STYLE_DANGER
```

The graphics engine resolves these styles from `g_pCurrentTheme` during
rendering. Avoid hard-coded RGB565 colors for text and buttons.

Rectangles store a literal `.color`. If a rectangle represents a themed panel,
set it from the palette during initialization and update it on
`EVT_CMD_CHANGE_THEME`:

```c
static void onThemeChanged(EventParam_t arg) {
    (void)arg;
    panelData.color = g_pCurrentTheme->palette.surface;
    panelData.bIsDirty = true;
}

Event_Subscribe(EVT_CMD_CHANGE_THEME, (EventHandler_fn)onThemeChanged);
```

The canvas background is resolved from `g_pCurrentTheme->palette.background`
by the graphics engine during full and partial painting. A form does not need
to recreate its canvas when the theme changes.

## Font behavior

`Theme_SetFontFamily()` loads fixed native-size BDF assets for headings and UI
text. It keeps `FONT_FAM_MONO` for telemetry values and icons. Theme changes
preserve the current font family before loading IDs into the new theme.

Do not use bitmap scaling to make text larger. Add or load a matching native
font size instead.

## Verification checklist

- Cycle through every theme from **Opciones > TEMA**.
- Confirm canvas background changes on every form.
- Check panels, static rectangles, labels, buttons, and icons.
- Check text contrast for normal, warning, danger, and disabled states.
- Confirm a full repaint occurs with no pixels from the prior theme.
- Rebuild `Debug` before flashing target hardware.
