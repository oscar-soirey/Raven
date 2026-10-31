/**
 * Raven - widgets/common.h
 * Shared by the widget implementations: class declarations and helpers for
 * text, scrollbars and single-line text editing.
 */

#ifndef RVN_WIDGETS_COMMON_H
#define RVN_WIDGETS_COMMON_H

#include "../widget/widget.h"
#include "../render/font.h"

/* ---------------------------------------------------------------------------
 *  Classes (one per file of this folder)
 * ------------------------------------------------------------------------- */

extern const rvn_widget_class_t rvn_class_vbox;
extern const rvn_widget_class_t rvn_class_hbox;
extern const rvn_widget_class_t rvn_class_grid;
extern const rvn_widget_class_t rvn_class_uniform_grid;
extern const rvn_widget_class_t rvn_class_overlay;
extern const rvn_widget_class_t rvn_class_border;
extern const rvn_widget_class_t rvn_class_size_box;
extern const rvn_widget_class_t rvn_class_spacer;
extern const rvn_widget_class_t rvn_class_scroll_box;
extern const rvn_widget_class_t rvn_class_splitter;
extern const rvn_widget_class_t rvn_class_wrap_box;
extern const rvn_widget_class_t rvn_class_canvas;
extern const rvn_widget_class_t rvn_class_switcher;
extern const rvn_widget_class_t rvn_class_tab_view;
extern const rvn_widget_class_t rvn_class_expander;
extern const rvn_widget_class_t rvn_class_label;
extern const rvn_widget_class_t rvn_class_button;
extern const rvn_widget_class_t rvn_class_checkbox;
extern const rvn_widget_class_t rvn_class_slider;
extern const rvn_widget_class_t rvn_class_progress_bar;
extern const rvn_widget_class_t rvn_class_image;
extern const rvn_widget_class_t rvn_class_text_input;
extern const rvn_widget_class_t rvn_class_spin_box;
extern const rvn_widget_class_t rvn_class_combo_box;
extern const rvn_widget_class_t rvn_class_separator;
extern const rvn_widget_class_t rvn_class_list_view;
extern const rvn_widget_class_t rvn_class_tree_view;
extern const rvn_widget_class_t rvn_class_tile_view;
extern const rvn_widget_class_t rvn_class_menu_bar;
extern const rvn_widget_class_t rvn_class_menu;
extern const rvn_widget_class_t rvn_class_menu_item;
extern const rvn_widget_class_t rvn_class_viewport;
extern const rvn_widget_class_t rvn_class_custom;
extern const rvn_widget_class_t rvn_class_text_editor;
extern const rvn_widget_class_t rvn_class_header_bar;
extern const rvn_widget_class_t rvn_class_file_browser;
extern const rvn_widget_class_t rvn_class_dock_area;
extern const rvn_widget_class_t rvn_class_island;
extern const rvn_widget_class_t rvn_class_background_blur;
extern const rvn_widget_class_t rvn_class_color_picker;
extern const rvn_widget_class_t rvn_class_timeline;
extern const rvn_widget_class_t rvn_class_node_graph;
extern const rvn_widget_class_t rvn_class_block_editor;

/** Internal class types, outside of the public enum. */
#define RVN_WIDGET_INTERNAL_COMBO_LIST ((RWidgetType)((int)R_WIDGET_LAST_ + 1))
#define RVN_WIDGET_INTERNAL_COMPLETION ((RWidgetType)((int)R_WIDGET_LAST_ + 2))

/** Panels sharing Rpanel_set_spacing start with this layout. */
typedef struct rvn_spaced_panel_t {
	rvn_widget_t base;
	float spacing;
} rvn_spaced_panel_t;

/** Case-insensitive comparison of an attribute name, inside a set_attribute function. */
#define RVN_ATTR(literal) (rvn_stricmp(name, (literal)) == 0)

/* ---------------------------------------------------------------------------
 *  Text helpers (widgets/common.c)
 * ------------------------------------------------------------------------- */

/** Font size of a widget: `size` if positive, else its R_METRIC_FONT_SIZE. */
float rvn_widget_font_size(const rvn_widget_t* widget, float size);
/** Width of a single line of text for a widget. */
float rvn_widget_text_width(const rvn_widget_t* widget, Rid font, float size, const char* text);
/** Height of one line of text for a widget. */
float rvn_widget_line_height(const rvn_widget_t* widget, Rid font, float size);
/** Calls the typed callback result and then the action of the widget; R_TRUE while the widget is alive. */
int rvn_widget_alive(const rvn_widget_t* widget);
/** Draws the keyboard focus outline of a widget when it should be visible. */
void rvn_paint_focus(const rvn_widget_t* widget, RPainter* painter, float radius);

/* ---------------------------------------------------------------------------
 *  Glass themes (widgets/common.c)
 *
 *  With a glass theme (g_rvn.theme.glass) the widgets draw their surfaces
 *  with the helpers below instead of flat boxes. The tint of each kind of
 *  surface comes from a color role, so the palette still drives the look.
 * ------------------------------------------------------------------------- */

typedef enum rvn_surface_t {
	/** Buttons, combo boxes: R_COLOR_CONTROL, _HOVERED, _PRESSED. */
	RVN_SURFACE_CONTROL,
	/** Primary buttons: R_COLOR_ACCENT, _HOVERED, _PRESSED. */
	RVN_SURFACE_ACCENT,
	/** Text fields: R_COLOR_INPUT. */
	RVN_SURFACE_INPUT,
	/** Bars across the window: R_COLOR_PANEL. */
	RVN_SURFACE_BAR,
	/** Panels (Border with the glass material): R_COLOR_PANEL. */
	RVN_SURFACE_PANEL,
	/** Menus, drop-down lists, tooltips: R_COLOR_POPUP. */
	RVN_SURFACE_POPUP,
	/** A knob being dragged: almost clear, strongly refracting. */
	RVN_SURFACE_LENS
} rvn_surface_t;

/** Corner radius of a control of the given height: its metric, or half the height (a capsule) with a glass theme. */
float rvn_control_radius(const rvn_widget_t* widget, float height);
/** Look of a kind of surface in the current theme; `hover` and `press` are 0..1. */
void rvn_glass_style(const rvn_widget_t* widget, rvn_surface_t kind, float hover, float press, rvn_glass_t* glass);
/** Applies the glass metrics of the theme (R_METRIC_GLASS_...) to a glass made by hand. */
void rvn_glass_tune(const rvn_widget_t* widget, rvn_glass_t* glass);
/** Glass surface with one radius, dimmed when the painter is disabled. */
void rvn_paint_surface(RPainter* painter, const rvn_widget_t* widget, rvn_rect_t rect, float radius,
	rvn_surface_t kind, float hover, float press);
/**
 * Background of a hovered or selected row: a plain rectangle, or an inset
 * rounded one with a glass theme.
 */
void rvn_paint_row(RPainter* painter, const rvn_widget_t* widget, rvn_rect_t row, rvn_color_t color);
/** Eased hover / pressed state of a widget for the glass themes (widget->anim). */
float rvn_widget_hover(rvn_widget_t* widget, RPainter* painter, int hovered);
float rvn_widget_press(rvn_widget_t* widget, RPainter* painter, int pressed);
/**
 * Squash and stretch: sets the transform of the widget from its press state
 * (rvn_widget_press), `amount` being the part of its height it loses when
 * fully pressed. To call from the paint function, every time.
 */
void rvn_widget_squash(rvn_widget_t* widget, float press, float amount);
/** A solid knob with a soft shadow (slider, switch), or a lens of clear glass while `press` is 1. */
void rvn_paint_knob(RPainter* painter, const rvn_widget_t* widget, rvn_rect_t rect, float press);
/** R_TRUE when the widget lies on a header bar (macOS 26 makes the controls of bars glass). */
int rvn_widget_on_bar(const rvn_widget_t* widget);

/* ---------------------------------------------------------------------------
 *  The look of Windows 11 (widgets/common.c)
 *
 *  The colors of Fluent that are not color roles of Raven, worked out from
 *  the theme (dark or light), and the base looks shared by several widgets.
 * ------------------------------------------------------------------------- */

typedef enum rvn_fluent_t {
	/** The edge of controls that is not ControlBorder: the bottom one in the light, the top one in the dark. */
	RVN_FLUENT_STROKE_EDGE,
	/** Outlines of check boxes and radio buttons, the bottom line of fields, the arrows. */
	RVN_FLUENT_STRONG_STROKE,
	/** The edges of an accent button: top, bottom. */
	RVN_FLUENT_ACCENT_EDGE_TOP,
	RVN_FLUENT_ACCENT_EDGE_BOTTOM,
	/** Flat controls pressed (SubtleFillColorTertiary). */
	RVN_FLUENT_SUBTLE_PRESSED,
	/** Check boxes and radio buttons that are off: at rest, hovered, pressed. */
	RVN_FLUENT_ALT_FILL,
	RVN_FLUENT_ALT_FILL_HOVERED,
	RVN_FLUENT_ALT_FILL_PRESSED,
	RVN_FLUENT_TEXT_SECONDARY,
	RVN_FLUENT_TEXT_ON_ACCENT_SECONDARY,
	RVN_FLUENT_TEXT_ON_ACCENT_DISABLED,
	RVN_FLUENT_CONTROL_DISABLED,
	RVN_FLUENT_ACCENT_DISABLED,
	/** A field being typed into. */
	RVN_FLUENT_INPUT_ACTIVE,
	/** The solid round of the thumb of a slider, of the knob of a switch that is on. */
	RVN_FLUENT_SOLID_CONTROL,
	/** The inner line of the focus ring. */
	RVN_FLUENT_FOCUS_INNER,
	/** The button that closes a window, under the pointer. */
	RVN_FLUENT_CLOSE,
	RVN_FLUENT_CLOSE_PRESSED,
	/** A card (an expander) and its outline. */
	RVN_FLUENT_CARD,
	RVN_FLUENT_CARD_BORDER
} rvn_fluent_t;

rvn_color_t rvn_fluent(const rvn_widget_t* widget, rvn_fluent_t token);
/**
 * Base look of a button of Windows (Button, ComboBox...): `states` tells its
 * style (RVN_STATE_PRIMARY, RVN_STATE_FLAT) and its state. An rvn_part_base_t.
 */
void rvn_fluent_button_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context);
/** Base look of a field of Windows (TextInput, SpinBox): the bottom line darkens, and turns to the accent when focused. */
void rvn_fluent_field_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context);
/** Base look of a flyout of Windows (menus, the list of a combo box, tooltips). */
void rvn_fluent_popup_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context);
/** A row of a list, a menu, a tree (hovered, pressed, selected) of Windows: inset, rounded, with the pill of the selection. */
void rvn_fluent_paint_item(RPainter* painter, const rvn_widget_t* widget, rvn_rect_t row, uint32_t states, int indicator, float indicator_shown);
/** The look of a surface of a popup (menus, list of a combo box), with its shadow, for any look. */
void rvn_paint_popup_surface(RPainter* painter, const rvn_widget_t* widget, rvn_rect_t rect, float radius, float shadow);
/** A chevron of Windows (the glyph of Segoe Fluent Icons, or drawn when the system has no such font). `angle` as rvn_paint_chevron. */
void rvn_fluent_paint_chevron(RPainter* painter, const rvn_widget_t* widget, float cx, float cy, float size, float angle, rvn_color_t color);

/** Base look of a row of Windows (RVN_PART_ITEM) and of the pill of the selection (RVN_PART_INDICATOR). */
void rvn_fluent_item_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context);

/* ---------------------------------------------------------------------------
 *  The looks of the platforms (widgets/platform_looks.c)
 *
 *  What a widget drawn in the look of a platform (rvn_widget_family not
 *  RVN_FAMILY_RAVEN) asks, whatever the platform: Windows (widgets/common.c),
 *  macOS (Aqua) and GNOME (libadwaita).
 * ------------------------------------------------------------------------- */

/** The kinds of rows: of a list (a list, a tree, the list of a combo box), of a menu, of a bar (the titles of a menu bar). */
#define RVN_ITEM_LIST 0
#define RVN_ITEM_MENU 1
#define RVN_ITEM_BAR  2

/** The color of the text of a widget seen through: the greys of libadwaita and of AppKit. */
rvn_color_t rvn_platform_ink(const rvn_widget_t* widget, float alpha);
/** Secondary text (shortcuts, placeholders, glyphs) of the platform. */
rvn_color_t rvn_platform_secondary(const rvn_widget_t* widget);
/** rvn_part_base_t of a button (states: RVN_STATE_PRIMARY, RVN_STATE_FLAT), of a field, of a flyout or a tooltip. */
void rvn_platform_button_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context);
void rvn_platform_field_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context);
void rvn_platform_popup_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context);
/** A row (RVN_PART_ITEM) or the pill of its selection (RVN_PART_INDICATOR), with the styles. */
void rvn_platform_item_look(const rvn_widget_t* widget, int part, uint32_t states, int kind, rvn_part_look_t* look);
/** Draws a row: its background, and the pill when it is selected (Windows). */
void rvn_platform_paint_item(RPainter* painter, const rvn_widget_t* widget, rvn_rect_t row, uint32_t states, int kind, float indicator_shown);
/** The color of the text of a row in its states (white on the selection of macOS...). */
rvn_color_t rvn_platform_item_foreground(const rvn_widget_t* widget, uint32_t states, int kind);
/** A chevron of the platform; `angle` as rvn_paint_chevron. */
void rvn_platform_paint_chevron(RPainter* painter, const rvn_widget_t* widget, float cx, float cy, float size, float angle, rvn_color_t color);
/** The frame of a list or a tree (a card on Windows and GNOME, a bordered table on macOS), with the styles of the widget. */
void rvn_platform_paint_list_frame(RPainter* painter, const rvn_widget_t* widget, rvn_rect_t rect, uint32_t states);
/** A row of a list inset in its line, as the platform draws it. */
rvn_rect_t rvn_platform_row(const rvn_widget_t* widget, rvn_rect_t line);
/** macOS: the ring of the focus around a field (`focus`: 0..1); nothing on the other platforms. */
void rvn_platform_paint_field_focus(RPainter* painter, const rvn_widget_t* widget, rvn_rect_t rect, float focus, float radius);

/** Where a source of `size` texels goes in an area, and which part of it shows (widgets/image.c). */
void rvn_image_fit(RStretch stretch, rvn_vec2_t size, float width, float height, const float uv_in[4],
	rvn_rect_t* rect, float uv_out[4]);
int  rvn_parse_stretch(const char* value, RStretch* stretch);
/** Texture / font of a file named in an attribute, loaded once (xml/xml_loader.c). */
Rid rvn_xml_texture(const char* path);
Rid rvn_xml_font(const char* path);
/** Theme named in an attribute: the name of a theme, or the path of a theme file (widget/theme.c). NULL after reporting the error. */
const rvn_theme_t* rvn_xml_theme(const char* value);
void rvn_xml_shutdown(void);
/** A mouse button was pressed on `hit` (NULL: on nothing): the unfolded islands of the window it is not in fold (widgets/island.c). */
void rvn_islands_pressed(rvn_window_t* window, rvn_widget_t* hit, int on_popup);
void rvn_islands_shutdown(void);
/** Releases what the file dialogs remember (widgets/dialogs.c). */
void rvn_dialogs_shutdown(void);

/* ---------------------------------------------------------------------------
 *  Formatted text (widgets/markdown.c), used by the labels
 * ------------------------------------------------------------------------- */

/** A Markdown text, parsed, and laid out for a width. */
typedef struct rvn_md_t rvn_md_t;

rvn_md_t* rvn_md_parse(const char* source);
void rvn_md_free(rvn_md_t* md);
/**
 * Places the text for a font, a size and a width (0: nothing wraps) and
 * returns the room it takes. Cheap when nothing changed. UI thread only.
 */
rvn_vec2_t rvn_md_layout(rvn_md_t* md, const rvn_widget_t* widget, Rid font, float size, float width);
/** Draws what the last layout placed, from (x, y) in the widget. `hot_link`: the link under the pointer, or -1. */
void rvn_md_paint(const rvn_md_t* md, const rvn_widget_t* widget, RPainter* painter, float x, float y, rvn_color_t color, int hot_link);
/** Link at a point of the laid out text, or -1. */
int rvn_md_link_at(const rvn_md_t* md, float x, float y);
/** Address of a link, or NULL. */
const char* rvn_md_link(const rvn_md_t* md, int link);
int rvn_md_link_count(const rvn_md_t* md);
/** The text without its marks, appended to `out`. */
void rvn_md_plain_text(const rvn_md_t* md, rvn_text_t* out);

/* ---------------------------------------------------------------------------
 *  Syntax colors (widgets/syntax.c), used by the text editor
 * ------------------------------------------------------------------------- */

typedef enum rvn_token_t {
	RVN_TOKEN_TEXT = 0,
	RVN_TOKEN_COMMENT,
	RVN_TOKEN_STRING,
	RVN_TOKEN_NUMBER,
	RVN_TOKEN_KEYWORD,
	RVN_TOKEN_TAG,
	RVN_TOKEN_ATTRIBUTE,
	RVN_TOKEN_PUNCTUATION,
	RVN_TOKEN_PREPROCESSOR,
	RVN_TOKEN_TYPE,
	RVN_TOKEN_FUNCTION,
	/* formatted text (Markdown) */
	RVN_TOKEN_HEADING,
	RVN_TOKEN_STRONG,
	RVN_TOKEN_EMPHASIS,
	RVN_TOKEN_LINK,
	RVN_TOKEN_COUNT
} rvn_token_t;

/** A run of bytes of a line with one kind of token. What no span covers is plain text. */
typedef struct rvn_span_t {
	int start;
	int end;
	uint8_t token;
} rvn_span_t;

/** A language of the text editor (Rlanguage_register), ready to be used by the lexers. */
typedef struct rvn_language_t rvn_language_t;

/** Language by name, by one of its other names, or by the extension of its files. NULL when unknown. */
const rvn_language_t* rvn_language_find(const char* name);
const RLanguageDesc* rvn_language_desc(const rvn_language_t* language);
/** Changes each time a language is declared: what was found before must be looked for again. */
uint32_t rvn_language_generation(void);
/** The words of a language: list 0 = keywords, 1 = types, 2 = constants. */
int rvn_language_word_count(const rvn_language_t* language, int list);
const char* rvn_language_word(const rvn_language_t* language, int list, int index);
void rvn_languages_shutdown(void);

/**
 * Cuts one line into spans, in order. `state` is what the line starts in (0
 * at the top of the text); the result is the state of the next line.
 * `spans` may be NULL to only compute the state.
 */
uint8_t rvn_syntax_line(const rvn_language_t* language, const char* text, int length, uint8_t state,
	rvn_span_t* spans, int max, int* count);
/** The public name of a kind of token. */
RToken rvn_token_public(rvn_token_t token);
/** Color of a token in a theme; `text` is the color of plain text. */
rvn_color_t rvn_syntax_color(const rvn_theme_t* theme, rvn_token_t token, rvn_color_t text);

/* ---------------------------------------------------------------------------
 *  Scrollbar (widgets/common.c): state + painting + mouse handling
 * ------------------------------------------------------------------------- */

typedef struct rvn_scrollbar_t {
	/** Current scroll position, 0 .. content - viewport. */
	float offset;
	/** Length of the content and of the visible part. */
	float content;
	float viewport;
	int dragging;
	/** Distance between the cursor and the start of the thumb while dragging. */
	float grab;
	/**
	 * The position that is drawn. It glides to `offset`, and a wheel that
	 * pushes against an end sends it past the end for a moment: the content
	 * stretches away and comes back, as on a phone.
	 */
	RSpring shown;
	/** Glass themes: the thumb only shows while the content moves or the pointer is near. */
	RSpring fade;
	/** Windows: the hairline widens to a thumb over its track while the pointer is on it. */
	RSpring expand;
} rvn_scrollbar_t;

float rvn_scrollbar_max(const rvn_scrollbar_t* bar);
int   rvn_scrollbar_needed(const rvn_scrollbar_t* bar);
/** Clamps the offset; returns R_TRUE when it changed. */
int   rvn_scrollbar_clamp(rvn_scrollbar_t* bar);
int   rvn_scrollbar_set(rvn_scrollbar_t* bar, float offset);
/**
 * Scrolls by `delta` logical units (the wheel). Returns R_TRUE when the
 * offset changed; at an end of the range it returns R_FALSE and makes the
 * content bounce, which only needs a repaint.
 */
int   rvn_scrollbar_wheel(rvn_scrollbar_t* bar, float delta);
/** While painting: advances the drawn position and returns it. It may lie outside 0 .. max. */
float rvn_scrollbar_shown(rvn_scrollbar_t* bar, RPainter* painter);
void  rvn_scrollbar_paint(rvn_scrollbar_t* bar, const rvn_widget_t* widget, RPainter* painter,
	rvn_rect_t track, int vertical, float mouse_x, float mouse_y);
/**
 * Handles mouse down / move / up on the track (given in widget coordinates).
 * Returns R_TRUE when the event was used; *changed is set when the offset moved.
 */
int   rvn_scrollbar_event(rvn_scrollbar_t* bar, rvn_widget_t* widget, const REvent* event,
	rvn_rect_t track, int vertical, int* changed);

/* ---------------------------------------------------------------------------
 *  Single-line text editing (widgets/textedit.c), used by TextInput and SpinBox
 * ------------------------------------------------------------------------- */

#define RVN_TEXTEDIT_HANDLED   (1 << 0)
#define RVN_TEXTEDIT_CHANGED   (1 << 1)
#define RVN_TEXTEDIT_COMMITTED (1 << 2)

typedef struct rvn_textedit_t {
	char* text;
	int length;
	int cap;
	/** Caret and selection anchor, as byte offsets. The selection is the range between them. */
	int caret;
	int anchor;
	float scroll;
	int max_chars;
	int password;
	int read_only;
	int mouse_selecting;
	Rid blink_timer;
	double blink_start;
} rvn_textedit_t;

void rvn_textedit_init(rvn_textedit_t* edit);
void rvn_textedit_free(rvn_textedit_t* edit);
/** Returns R_TRUE when the text changed. */
int  rvn_textedit_set_text(rvn_textedit_t* edit, const char* text);
void rvn_textedit_select_all(rvn_textedit_t* edit);
/** Starts or stops the caret blinking of the widget owning the editor. */
void rvn_textedit_set_focused(rvn_textedit_t* edit, rvn_widget_t* widget, int focused);
/** Key and text events. Returns a combination of RVN_TEXTEDIT_... flags. */
int  rvn_textedit_key(rvn_textedit_t* edit, const REvent* event);
int  rvn_textedit_input(rvn_textedit_t* edit, uint32_t codepoint);
/** Mouse press / drag at a horizontal position relative to the left of the text area. */
void rvn_textedit_mouse_down(rvn_textedit_t* edit, const rvn_widget_t* widget, Rid font, float size,
	float x, int clicks, int extend);
void rvn_textedit_mouse_drag(rvn_textedit_t* edit, const rvn_widget_t* widget, Rid font, float size, float x);
/** Draws selection, text and caret inside `area` (widget coordinates). */
void rvn_text_input_rect(const rvn_widget_t* widget, const RPainter* painter, rvn_rect_t caret);
void rvn_textedit_paint(rvn_textedit_t* edit, const rvn_widget_t* widget, RPainter* painter,
	rvn_rect_t area, Rid font, float size, rvn_color_t color, int focused);

#endif /* RVN_WIDGETS_COMMON_H */
