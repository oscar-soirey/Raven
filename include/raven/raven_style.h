/**
 * Raven
 * raven_style.h : looks and themes (colors, metrics, materials, styles,
 * theme files).
 *
 * A look is how every widget is drawn: the shapes of the controls, how they
 * show their states, how they move, and the colors, sizes and font that go
 * with them. Raven has its own looks (flat, glass, high contrast) and looks
 * that imitate the platforms (Windows 11, macOS, GNOME); Rlook_set picks the
 * one of the application.
 *
 * A theme is laid over the look: a file that changes colors, sizes and the
 * styles of some widgets (see docs/THEMES.md). A theme file that names no
 * look follows the look in use.
 *
 * Every widget reads its colors and sizes from its theme. A value is looked
 * for in this order:
 *
 *   1. the widget itself   Rwidget_set_color, Rwidget_set_metric, Color.Accent="#f00"
 *   2. the styles          rules of the theme for a type of widget ("Button"),
 *                          for a class (Class="danger"), or for both; for a
 *                          part of a widget ("Thumb") and its states ("Hovered")
 *   3. the theme           the theme of the application, or the one given to
 *                          an ancestor of the widget (Rwidget_set_theme,
 *                          Theme="Night"): a part of a window can have its own
 *
 *   <Theme Name="Night" Accent="#FF9F0A">
 *       <Colors Panel="#101018" Text="#FFF4E0"/>
 *       <Metrics CornerRadius="6" ControlHeight="34"/>
 *       <Style Widget="Button" Class="danger" Accent="#FF453A"/>
 *       <Style Widget="Slider" Part="Thumb" State="Hovered" Background="#FFB340"/>
 *   </Theme>
 */

#ifndef RAVEN_STYLE_H
#define RAVEN_STYLE_H

#include "raven_core.h"


/**
 * The looks. Each one comes dark and light.
 *
 * Raven's looks are the default; the ones of the platforms imitate them
 * closely (their shapes, colors, sizes, states and motion), without using
 * their widgets. The looks of the platforms are opaque, except macOS Glass
 * (macOS 26), whose bars, menus and floating surfaces are glass as on the
 * system; the glass materials themselves (Rtheme_set_material) belong to
 * the glass looks of Raven.
 */
typedef enum RLook{
	/** Raven, flat and dense, for tools. The look at start. */
	R_LOOK_RAVEN_DARK = 0x00E0,
	R_LOOK_RAVEN_LIGHT,
	/**
	 * Raven in glass: controls are capsules of translucent glass that refract
	 * and blur what is behind them, catch the light on their rim and react
	 * to the pointer with short animations. Windows get a soft colored
	 * background unless they have their own (Rwindow_set_background,
	 * Rwindow_set_clear_color).
	 */
	R_LOOK_RAVEN_GLASS_DARK,
	R_LOOK_RAVEN_GLASS_LIGHT,
	/**
	 * High contrast, for low vision: pure black and white, yellow (or blue)
	 * focus and selection, thick borders, larger text (raven_accessibility.h,
	 * Raccessibility_set_high_contrast).
	 */
	R_LOOK_RAVEN_HIGH_CONTRAST_DARK,
	R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT,
	/**
	 * Windows 11 (Fluent): its controls, its colors, the accent color the
	 * user chose (on Windows), the Segoe UI font when the system has it, and
	 * motion without bounce.
	 */
	R_LOOK_WINDOWS_DARK,
	R_LOOK_WINDOWS_LIGHT,
	/**
	 * macOS (Sequoia, Aqua): its controls, its colors, the accent color the
	 * user chose (on macOS), the San Francisco font when the system has it,
	 * no change under the pointer, and selections in the accent color.
	 */
	R_LOOK_MACOS_DARK,
	R_LOOK_MACOS_LIGHT,
	/**
	 * GNOME (libadwaita), the desktop of most Linux distributions: flat
	 * rounded controls, the accent and the font of the desktop (on Linux).
	 */
	R_LOOK_GNOME_DARK,
	R_LOOK_GNOME_LIGHT,
	/**
	 * The look of the platform the application runs on: Windows on Windows,
	 * macOS on macOS (macOS Glass from macOS 26 on), GNOME on Linux and the
	 * other systems.
	 */
	R_LOOK_NATIVE_DARK,
	R_LOOK_NATIVE_LIGHT,
	/**
	 * macOS 26 (Tahoe, Liquid Glass): the controls of macOS, larger and
	 * rounder, and glass where the system puts it, on what floats above the
	 * content: header bars and their buttons, menus, drop-down lists,
	 * tooltips, sheets; the knobs of sliders and switches turn into a lens
	 * of clear glass while they are held. The content itself (fields, lists,
	 * text) stays opaque. The accent color and the font of the system, on
	 * macOS.
	 */
	R_LOOK_MACOS_GLASS_DARK,
	R_LOOK_MACOS_GLASS_LIGHT
}RLook;

/**
 * @deprecated The built-in themes of Raven 0.2, now looks: R_THEME_DARK is
 * R_LOOK_RAVEN_DARK and so on, and Rtheme_apply is Rlook_set.
 */
typedef enum RTheme{
	/** Flat, dense themes for tools. */
	R_THEME_DARK = 0x00E0,
	R_THEME_LIGHT,
	/**
	 * Glass themes: controls are capsules of translucent glass that refract
	 * and blur what is behind them, catch the light on their rim and react
	 * to the pointer with short animations. Windows get a soft colored
	 * background unless they have their own (Rwindow_set_background,
	 * Rwindow_set_clear_color).
	 */
	R_THEME_GLASS_DARK,
	R_THEME_GLASS_LIGHT,
	/**
	 * High contrast, for low vision: pure black and white, yellow (or blue)
	 * focus and selection, thick borders, larger text (raven_accessibility.h,
	 * Raccessibility_set_high_contrast).
	 */
	R_THEME_HIGH_CONTRAST_DARK,
	R_THEME_HIGH_CONTRAST_LIGHT
}RTheme;

/**
 * What surfaces are made of, from the clearest to the most opaque. A theme
 * has one material for everything (Rtheme_set_material); any widget can be
 * given its own (Rwidget_set_material, Material="..." in XML).
 *
 * The three glasses share the same anatomy (a tint over what is behind, a
 * rim that bends it and catches the light, a shadow) and differ by how much
 * they let through. Within one material, surfaces are also told apart by
 * their height above the window: bars lie flat, panels float a little,
 * controls a little more, menus and dialogs highest; each level has a
 * brighter rim and a deeper shadow than the one below.
 */
typedef enum RMaterial{
	/** Opaque: filled with the colors of the theme. The material of R_THEME_DARK and R_THEME_LIGHT. */
	R_MATERIAL_SOLID = 0x02E0,
	/** Clear glass: what is behind stays sharp enough to be recognized, strongly bent at the rim. */
	R_MATERIAL_GLASS,
	/** Frosted glass: what is behind is blurred into soft colors. The material of the glass themes. */
	R_MATERIAL_FROSTED_GLASS,
	/** Frosted glass colored by the accent color of the theme (or of the widget): for what must stand out. */
	R_MATERIAL_TINTED_GLASS,
	/** Not a material: "the one of the theme". Removes the material given to a widget. */
	R_MATERIAL_DEFAULT
}RMaterial;

/** Color roles. The comment lists the widgets reading each role. */
typedef enum RColorRole{
	/** Window clear color. */
	R_COLOR_WINDOW = 0x0100,
	/** Border background. */
	R_COLOR_PANEL,
	/** Border outline, list and input outlines. */
	R_COLOR_BORDER,
	/** Text. Inherited by the children of the widget it is set on. */
	R_COLOR_TEXT,
	R_COLOR_TEXT_DISABLED,
	R_COLOR_TEXT_PLACEHOLDER,
	/** Primary buttons, checked boxes, slider fill, progress fill. */
	R_COLOR_ACCENT,
	R_COLOR_ACCENT_HOVERED,
	R_COLOR_ACCENT_PRESSED,
	/** Text and marks drawn on top of the accent color. */
	R_COLOR_ACCENT_TEXT,
	/** Button, combo box, check box background. */
	R_COLOR_CONTROL,
	R_COLOR_CONTROL_HOVERED,
	R_COLOR_CONTROL_PRESSED,
	R_COLOR_CONTROL_BORDER,
	/** Text input, spin box, list and tree background. */
	R_COLOR_INPUT,
	/** Keyboard focus outline. */
	R_COLOR_FOCUS,
	/** Selected rows and selected text. */
	R_COLOR_SELECTION,
	R_COLOR_ROW_HOVERED,
	/** Slider track, progress background, scrollbar track. */
	R_COLOR_TRACK,
	/** Scrollbar thumb, slider handle. */
	R_COLOR_HANDLE,
	R_COLOR_HANDLE_HOVERED,
	R_COLOR_SEPARATOR,
	/** Menu bar, tab bar, expander header. */
	R_COLOR_HEADER,
	R_COLOR_HEADER_HOVERED,
	/** Active tab. */
	R_COLOR_TAB_ACTIVE,
	/** Menus, combo box lists. */
	R_COLOR_POPUP,
	R_COLOR_POPUP_BORDER,
	R_COLOR_SHADOW,
	R_COLOR_TOOLTIP,
	R_COLOR_TOOLTIP_TEXT,
	/** Track of a switch that is on. */
	R_COLOR_SWITCH,
	/** What went well, what needs attention, what is wrong or destroys something (markers, badges, classes). */
	R_COLOR_SUCCESS,
	R_COLOR_WARNING,
	R_COLOR_DANGER,
	/** Links of formatted text. */
	R_COLOR_LINK,
	/** Background of code in formatted text. */
	R_COLOR_CODE,
	R_COLOR_LAST_
}RColorRole;

#define R_COLOR_COUNT ((int)R_COLOR_LAST_ - (int)R_COLOR_WINDOW)

/** Sizes, in logical units. */
typedef enum RMetric{
	R_METRIC_FONT_SIZE = 0x0140,
	R_METRIC_CORNER_RADIUS,
	R_METRIC_BORDER_WIDTH,
	/** Height of buttons, inputs, combo boxes, tabs, menu bars. */
	R_METRIC_CONTROL_HEIGHT,
	/** Horizontal / vertical padding inside controls. */
	R_METRIC_PADDING_X,
	R_METRIC_PADDING_Y,
	/** Gap between an icon or a check mark and its text. */
	R_METRIC_SPACING,
	R_METRIC_SCROLLBAR_SIZE,
	/** Height of list, tree and menu rows. */
	R_METRIC_ROW_HEIGHT,
	/** Indentation per tree level. */
	R_METRIC_INDENT,
	R_METRIC_SPLITTER_SIZE,
	R_METRIC_CHECK_SIZE,
	R_METRIC_SLIDER_HANDLE_SIZE,
	/** Corner radius of menus, drop-down lists and tooltips. */
	R_METRIC_POPUP_RADIUS,
	/**
	 * The glass of the theme: each one multiplies what the material gives,
	 * 1 = unchanged. Frost (how much is blurred), refraction (how far the rim
	 * bends what is behind), saturation (of what is seen through), rim (the
	 * light it catches) and shadow (the one it drops).
	 */
	R_METRIC_GLASS_FROST,
	R_METRIC_GLASS_REFRACTION,
	R_METRIC_GLASS_SATURATION,
	R_METRIC_GLASS_RIM,
	R_METRIC_GLASS_SHADOW,
	/** Radius of the blur of a BackgroundBlur that was not given one. */
	R_METRIC_BLUR_RADIUS,
	R_METRIC_LAST_
}RMetric;

#define R_METRIC_COUNT ((int)R_METRIC_LAST_ - (int)R_METRIC_FONT_SIZE)


#ifdef __cplusplus
extern "C" {
#endif

	/* -----------------------------------------------------------------------
	 *  Looks
	 * --------------------------------------------------------------------- */

	/**
	 * @brief Changes the look of the application. The theme file in use, if
	 * it names no look of its own, is laid again over the new one; colors
	 * and metrics changed with Rtheme_set_... are lost; what was given to
	 * widgets (Rwidget_set_color...) is kept.
	 *
	 *   Rlook_set(R_LOOK_NATIVE_LIGHT);   the look of the platform, light
	 *
	 * Theme files that name no look (no Look attribute) follow it too, so a
	 * part of a window given one (Theme="paper.xml") changes with it.
	 */
	RAVEN_API void Rlook_set(RLook look);
	/** @brief The look given to Rlook_set (R_LOOK_RAVEN_DARK at start). */
	RAVEN_API RLook Rlook_get(void);

	/** @deprecated Rlook_set: a built-in theme is a look of Raven. */
	RAVEN_API void Rtheme_apply(RTheme theme);

	RAVEN_API void Rtheme_set_color(RColorRole role, float r, float g, float b, float a);
	RAVEN_API void Rtheme_get_color(RColorRole role, float* r, float* g, float* b, float* a);

	RAVEN_API void Rtheme_set_metric(RMetric metric, float value);
	RAVEN_API float Rtheme_get_metric(RMetric metric);

	/**
	 * @brief Material of every widget that was not given one with
	 * Rwidget_set_material. Rtheme_apply sets it: solid for the flat
	 * themes, frosted glass for the glass themes.
	 */
	RAVEN_API void Rtheme_set_material(RMaterial material);
	RAVEN_API RMaterial Rtheme_get_material(void);

	/** @brief Font used by widgets that have no font of their own. */
	RAVEN_API void Rtheme_set_font(Rid font);
	RAVEN_API Rid Rtheme_get_font(void);

	/**
	 * @brief Sets the accent color and everything that follows from it: its
	 * hovered and pressed shades, the focus outline, the selection, the links.
	 */
	RAVEN_API void Rtheme_set_accent(float r, float g, float b);

	/**
	 * @brief Tells whether the theme is a dark one. It decides the tint of
	 * the glass, the colors of the code and which wallpaper the glass themes
	 * use. Rtheme_apply and the theme files set it.
	 */
	RAVEN_API void Rtheme_set_dark(int dark);
	RAVEN_API int Rtheme_is_dark(void);

	/**
	 * @brief Image drawn behind the content of the windows that have no
	 * background of their own, or R_INVALID_ID for none (the window color).
	 */
	RAVEN_API void Rtheme_set_wallpaper(Rid texture);
	RAVEN_API Rid Rtheme_get_wallpaper(void);

	/* -----------------------------------------------------------------------
	 *  Styles: values for a type of widget, for a class, or for both
	 * --------------------------------------------------------------------- */

	/**
	 * @brief Gives a color to the widgets of a type and / or of a class,
	 * instead of the color of the theme.
	 * @param widget      Tag of a type of widget ("Button"), or NULL for every type.
	 * @param style_class A class (Rwidget_set_class, Class="..."), or NULL
	 *                    for the widgets of the type whatever their classes.
	 *
	 * When several styles give the same value to a widget, the one with a
	 * type and a class wins over the one with a class, which wins over the
	 * one with a type; between equals, the last one set wins.
	 */
	RAVEN_API void Rtheme_set_style_color(const char* widget, const char* style_class, RColorRole role, float r, float g, float b, float a);
	RAVEN_API void Rtheme_set_style_metric(const char* widget, const char* style_class, RMetric metric, float value);
	/** @brief R_MATERIAL_DEFAULT removes the material of the style. */
	RAVEN_API void Rtheme_set_style_material(const char* widget, const char* style_class, RMaterial material);
	/** @brief Removes every style of the theme. Rtheme_apply and Rtheme_use do it too. */
	RAVEN_API void Rtheme_clear_styles(void);

	/* -----------------------------------------------------------------------
	 *  Named themes and theme files
	 * --------------------------------------------------------------------- */

	/**
	 * @brief Reads a theme file and keeps the theme under its name (the Name
	 * attribute, else the name of the file). It does not change the theme in
	 * use unless a theme of that name is the one in use.
	 *
	 * The file is watched like the interface files (Rxml_set_live_reload):
	 * saving it updates every widget that uses the theme.
	 * @return The name of the theme, valid until Rshutdown, or NULL after
	 * reporting an error.
	 */
	RAVEN_API const char* Rtheme_load(const char* path);
	/** @brief Same, from a text. `size` may be 0 for a null-terminated text. */
	RAVEN_API const char* Rtheme_load_from_memory(const char* xml, size_t size);

	/**
	 * @brief Makes a named theme the theme of the application: a theme
	 * given by Rtheme_load (laid over the look in use when it names no
	 * look), or the name of a look ("WindowsLight", "RavenDark"; the old
	 * "Dark", "Light", "GlassDark"... too), which is then Rlook_set.
	 * Values changed with Rtheme_set_... are lost.
	 * @return R_FALSE, after reporting an error, when no theme has that name.
	 */
	RAVEN_API int Rtheme_use(const char* name);
	/** @brief Name of the theme in use ("Dark" at start). */
	RAVEN_API const char* Rtheme_get_name(void);

	/** @brief Number of named themes, the four built-in ones included. */
	RAVEN_API int Rtheme_get_count(void);
	/** @brief Name of the index-th one, or NULL. */
	RAVEN_API const char* Rtheme_get_name_at(int index);

	/**
	 * @brief Writes the theme in use, with every change made since it was
	 * applied, as a theme file.
	 * @param name Name given to the theme in the file; NULL keeps the current one.
	 */
	RAVEN_API int Rtheme_save(const char* path, const char* name);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_STYLE_H */
