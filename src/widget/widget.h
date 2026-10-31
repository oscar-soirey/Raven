/**
 * Raven - widget/widget.h
 * Internal widget and window model shared by the widget core and by every
 * widget implementation in src/widgets/.
 *
 * A widget type is a rvn_widget_class_t (a table of functions) plus a
 * structure whose first member is rvn_widget_t. To add a widget type:
 *   1. add its value to RWidgetType,
 *   2. write src/widgets/<name>.c defining its structure and class,
 *   3. list the class in widget/registry.c.
 */

#ifndef RVN_WIDGET_INTERNAL_H
#define RVN_WIDGET_INTERNAL_H

#include "../core/context.h"
#include "../render/draw.h"
#include "../render/font.h"

typedef struct rvn_widget_t rvn_widget_t;
typedef struct rvn_window_t rvn_window_t;
struct rvn_xml_node_t;

/* ---------------------------------------------------------------------------
 *  Flags
 * ------------------------------------------------------------------------- */

/* widget state (rvn_widget_t::flags) */
#define RVN_WF_MEASURE_DIRTY (1u << 0)  /* desired size must be recomputed */
#define RVN_WF_ARRANGE_DIRTY (1u << 1)  /* children must be placed again */
#define RVN_WF_DISABLED      (1u << 2)
#define RVN_WF_NO_HIT        (1u << 3)  /* the widget itself ignores the mouse (children do not) */
#define RVN_WF_CLIP          (1u << 4)  /* children are clipped to the bounds */
#define RVN_WF_FOCUSABLE     (1u << 5)
#define RVN_WF_HOVERED       (1u << 6)  /* the cursor is over the widget or a descendant */
#define RVN_WF_PRESSED       (1u << 7)  /* set by widgets while a click is in progress */
#define RVN_WF_FOCUSED       (1u << 8)
#define RVN_WF_DEAD          (1u << 9)  /* deleted, memory released at the end of the dispatch */
#define RVN_WF_POPUP         (1u << 10) /* shown as a popup: rect is in window coordinates */
#define RVN_WF_INTERNAL      (1u << 11) /* owned by another widget without being its child */
#define RVN_WF_WINDOW_DRAG   (1u << 12) /* part of the title bar of a window with a custom header */
#define RVN_WF_LAYOUT_DIRTY  (RVN_WF_MEASURE_DIRTY | RVN_WF_ARRANGE_DIRTY)

/* class traits (rvn_widget_class_t::flags) */
#define RVN_CF_SINGLE_CHILD (1u << 0)   /* adding a child replaces the previous one */
#define RVN_CF_NO_CHILDREN  (1u << 1)
#define RVN_CF_OVERFLOW     (1u << 2)   /* children may lie outside the bounds (no culling by bounds) */
#define RVN_CF_UI_PAINT     (1u << 3)   /* paint calls application code: UI thread only */
#define RVN_CF_NO_HIT       (1u << 4)   /* instances start with RVN_WF_NO_HIT */
#define RVN_CF_FOCUSABLE    (1u << 5)   /* instances start with RVN_WF_FOCUSABLE */
#define RVN_CF_CLIP         (1u << 6)   /* instances start with RVN_WF_CLIP */
#define RVN_CF_SURFACE     (1u << 7)   /* draws its own surface, in the material of the widget */
#define RVN_CF_OWN_CHILDREN (1u << 8)  /* paints its children itself, from its paint function */
#define RVN_CF_NO_MIRROR    (1u << 9)   /* a surface of its own: its children are not mirrored in a right-to-left layout */

/* ---------------------------------------------------------------------------
 *  Widget
 * ------------------------------------------------------------------------- */

/** How a child is placed by its parent (see Rslot_set_... in raven_widget.h). */
typedef struct rvn_slot_t {
	rvn_margin_t padding;
	RHAlign halign;
	RVAlign valign;
	/** R_TRUE = fill with coefficient `fill`, R_FALSE = auto. */
	int is_fill;
	float fill;
	float max_size;
	int column, row;
	int column_span, row_span;
	/** Canvas: min x, min y, max x, max y. */
	float anchors[4];
	float position[2];
	float size[2];
	float alignment[2];
	char* title;
	/** Dock area: where the panel goes when it is added (0 = behind the first tabs), next to which panel, taking how much. */
	RDockSide dock;
	char* dock_to;
	float dock_size;
	/** Dock area: R_TRUE removes the close button of the panel. */
	int dock_fixed;
	/** Dock area: icon drawn before the title of the tab (Rdock_area_set_panel_icon). */
	int has_icon;
	Rid icon;
	float icon_uv[4];
	/** R_TRUE: the icon is a mask drawn in the text color of the tab. */
	int icon_tint;
} rvn_slot_t;

/** Transition of a widget: the one it plays when it appears, and the one playing now. */
typedef struct rvn_fx_t {
	RTransition appear;
	float appear_delay;
	RTransition playing_kind;
	int playing;
	/** Seconds left before the transition starts (staggered groups). */
	float delay;
	/** 0 = where the transition starts, 1 = in place. */
	RSpring progress;
} rvn_fx_t;

/**
 * Geometry applied to everything a widget and its subtree drew, after the
 * fact (rvn_drawlist_transform). A class sets it from its paint function to
 * squash, stretch or nudge itself; zeroed means "as painted".
 */
typedef struct rvn_xform_t {
	/** Scale minus one, per axis, around the center of the widget. */
	float grow_x, grow_y;
	/** Offset, in logical units. */
	float dx, dy;
} rvn_xform_t;

/** What a transition does to its widget during one frame (anim.c). */
typedef struct rvn_transition_state_t {
	int active;
	float opacity;
	float sx, sy;
	float dx, dy;
	/** Pivot of the scale, from the center of the widget, in units of its size (-0.5 .. 0.5). */
	float pivot_x, pivot_y;
} rvn_transition_state_t;

typedef struct rvn_widget_class_t {
	RWidgetType type;
	/** Tag of the widget in XML. */
	const char* name;
	/** Size of the instance structure (rvn_widget_t is its first member). */
	size_t size;
	uint32_t flags;

	void (*init)(rvn_widget_t* widget);
	void (*destroy)(rvn_widget_t* widget);

	/**
	 * Returns the desired size of the content, measuring children with
	 * rvn_widget_measure. NULL: the largest child (slot padding included).
	 */
	rvn_vec2_t (*measure)(rvn_widget_t* widget);
	/**
	 * Places the children inside widget->rect.w x widget->rect.h with
	 * rvn_widget_arrange. NULL: every child fills the widget (slot applied).
	 */
	void (*arrange)(rvn_widget_t* widget);
	/** Draws the widget itself, below its children. */
	void (*paint)(rvn_widget_t* widget, RPainter* painter);
	/** Draws above the children (scrollbars, focus outline). */
	void (*paint_overlay)(rvn_widget_t* widget, RPainter* painter);
	/** Handles an input event. Returns R_TRUE to stop the bubbling. */
	int  (*event)(rvn_widget_t* widget, const REvent* event);
	/** Sets a property from text (XML attribute). Returns R_FALSE when the name is unknown. */
	int  (*set_attribute)(rvn_widget_t* widget, const char* name, const char* value);
	/** Tells whether a child takes part in painting and hit testing. NULL: all do. */
	int  (*child_active)(rvn_widget_t* widget, rvn_widget_t* child);
	/**
	 * Tells whether a point of the widget (from its corner, as shown) can
	 * reach a child: R_FALSE where the class draws something of its own over
	 * that child. NULL: every point can.
	 */
	int  (*child_at)(rvn_widget_t* widget, rvn_widget_t* child, float x, float y);
	/** Called after a child was added or removed. */
	void (*children_changed)(rvn_widget_t* widget);
	/** Handles an XML child element that is not a widget (<Item>, ...). Returns R_FALSE when unknown. */
	int  (*xml_child)(rvn_widget_t* widget, const struct rvn_xml_node_t* node);
	/** Called when the widget, shown as a popup, has been closed. */
	void (*popup_closed)(rvn_widget_t* widget);
	/**
	 * For a widget that moves its window when dragged (RVN_WF_WINDOW_DRAG):
	 * R_FALSE where it keeps the mouse for itself (the buttons of a header bar).
	 * NULL: the whole widget drags.
	 */
	int  (*window_drag_at)(rvn_widget_t* widget, float x, float y);
} rvn_widget_class_t;

struct rvn_widget_t {
	Rid id;
	const rvn_widget_class_t* cls;
	rvn_widget_t* parent;
	/** Window showing the widget, NULL while detached. */
	rvn_window_t* window;
	rvn_widget_t** children;
	uint32_t child_count;
	uint32_t child_cap;

	uint32_t flags;
	RVisibility visibility;
	/** RDirection of the widget itself (0: inherit). */
	uint16_t direction;
	/** Accessibility: what a screen reader says of it (core/accessibility.c), or NULL. */
	struct rvn_a11y_t* a11y;
	/** Bounds relative to the parent (to the window for popups and roots), logical units. */
	rvn_rect_t rect;
	/** Desired size computed by the last measure pass. */
	rvn_vec2_t desired;
	/** Constraints on the desired size; a negative value means "none". */
	rvn_vec2_t fixed_size;
	rvn_vec2_t min_size;
	rvn_vec2_t max_size;
	rvn_slot_t slot;
	float opacity;
	/** Overrides of the theme for this widget, allocated on first use. */
	rvn_style_t* style;
	/** Material of its own, or 0 for the one of the theme. */
	RMaterial material;
	/** Classes, separated by spaces, or NULL. */
	char* classes;
	/** Theme of the widget: its own or the one of an ancestor; NULL = the theme in use. */
	const rvn_theme_t* theme;
	/** R_TRUE when `theme` was given to this widget itself. */
	uint8_t theme_own;
	/** What the styles of the theme give to this widget, or NULL; valid while style_generation is the one of the library. */
	const rvn_style_match_t* match;
	uint32_t style_generation;

	char* name;
	char* tooltip;
	/** Node of the public XML document the widget was built from (Rwidget_get_xml_node), or 0. */
	Rid xml_node;
	char* action;
	void* user_handle;
	/** Told when the widget is gone (Rwidget_set_destroy_callback). */
	RWidgetDestroyCallback destroy_callback;
	void* destroy_user;
	/** The tag made by code the widget was built for (Rxml_register_tag): its index + 1, or 0. */
	uint8_t xml_tag;
	RWidgetEventCallback event_callback;
	void* event_user;
	/** 0 = inherit from the parent. */
	RCursor cursor;
	Rid context_menu;
	/** Takes what is dropped on the widget from other applications, or NULL. */
	RDropCallback drop_callback;
	void* drop_user;

	/** Eased state of the widget, advanced while painting: under the pointer, and pressed. */
	RSpring hover;
	RSpring press;
	/** Set by the paint function of the class: see rvn_xform_t. */
	rvn_xform_t xform;
	/** Set by the paint function of the class: offset of the children for this frame (smooth scrolling). */
	rvn_vec2_t child_shift;
	/**
	 * Size of the children on screen, for a class that draws them larger or
	 * smaller than they are laid out (a canvas that zooms): a child whose
	 * rectangle is at (x, y) shows at (x, y) * child_scale from the corner of
	 * the widget. 0 means 1. The class draws them so itself (it has
	 * RVN_CF_OWN_CHILDREN); the hit tests and the mouse events of the
	 * subtree follow (rvn_widget_to_virtual).
	 */
	float child_scale;
	/** Transitions, allocated on first use. */
	rvn_fx_t* fx;

	/** Number of widgets in the subtree, this one included. */
	uint32_t subtree_count;
	/** Number of widgets in the subtree whose paint must run on the UI thread. */
	uint32_t ui_paint_count;
};

/* ---------------------------------------------------------------------------
 *  Registry (widget/registry.c)
 * ------------------------------------------------------------------------- */

const rvn_widget_class_t* rvn_widget_class_by_type(RWidgetType type);
const rvn_widget_class_t* rvn_widget_class_by_name(const char* name);

/* ---------------------------------------------------------------------------
 *  Tree (widget/widget.c)
 * ------------------------------------------------------------------------- */

/** Resolves a handle without reporting errors. */
rvn_widget_t* rvn_widget_get(Rid id);
/**
 * Resolves a handle for a public function: checks the library state, the
 * calling thread, the handle and, when `type` is not 0, the widget type.
 * Reports an error naming `function` and returns NULL on failure.
 */
rvn_widget_t* rvn_widget_check(Rid id, RWidgetType type, const char* function);

#define RVN_WIDGET(variable, id, type, result) \
	rvn_widget_t* variable = rvn_widget_check((id), (type), __func__); \
	if (!variable) return result

#define RVN_WIDGET_VOID(variable, id, type) \
	rvn_widget_t* variable = rvn_widget_check((id), (type), __func__); \
	if (!variable) return

rvn_widget_t* rvn_widget_create(RWidgetType type);
/** Creates a widget of a class that is not in the registry (internal popups). */
rvn_widget_t* rvn_widget_create_class(const rvn_widget_class_t* cls);
/** Detaches and destroys a widget and its subtree. */
void rvn_widget_destroy(rvn_widget_t* widget);
/** @param index Position among the children, or -1 to append. */
void rvn_widget_add_child(rvn_widget_t* parent, rvn_widget_t* child, int index);
/** Removes the widget from its parent without destroying it. */
void rvn_widget_detach(rvn_widget_t* widget);
int  rvn_widget_child_index(const rvn_widget_t* parent, const rvn_widget_t* child);
/** Sets the window of a whole subtree (used for roots and popups). */
void rvn_widget_set_window(rvn_widget_t* widget, rvn_window_t* window);
/** R_TRUE when the widget and all its ancestors are visible and active in their parent. */
int  rvn_widget_is_shown(const rvn_widget_t* widget);
/** R_TRUE when the widget and all its ancestors are enabled. */
int  rvn_widget_is_enabled(const rvn_widget_t* widget);
/** R_TRUE when `ancestor` is `widget` or one of its ancestors. */
int  rvn_widget_is_inside(const rvn_widget_t* widget, const rvn_widget_t* ancestor);
/** Top-left corner of the widget in window coordinates. */
rvn_vec2_t rvn_widget_origin(const rvn_widget_t* widget);
/**
 * Widgets under an ancestor that scales its children (child_scale) live in a
 * space of their own, where rvn_widget_origin and their rectangles are as if
 * nothing was scaled. These convert a point of the window to that space of a
 * widget (what its events carry), and back (to place a popup next to it).
 */
void rvn_widget_to_virtual(const rvn_widget_t* widget, float* x, float* y);
void rvn_widget_from_virtual(const rvn_widget_t* widget, float* x, float* y);
/** Releases the memory of the widgets deleted during the event dispatch. */
void rvn_widgets_collect(void);

/* ---------------------------------------------------------------------------
 *  Layout (widget/layout.c)
 * ------------------------------------------------------------------------- */

/** The widget needs a new measure + arrange + paint. */
void rvn_widget_invalidate_layout(rvn_widget_t* widget);
/** The widget only needs to be painted again. */
void rvn_widget_invalidate_paint(rvn_widget_t* widget);

/** Desired size of a widget (cached). (0, 0) when collapsed. */
rvn_vec2_t rvn_widget_measure(rvn_widget_t* widget);
/** Desired size of a child plus its slot padding. */
rvn_vec2_t rvn_widget_measure_slot(rvn_widget_t* child);
/** Gives a widget its final bounds, relative to its parent, and arranges its children. */
void rvn_widget_arrange(rvn_widget_t* widget, float x, float y, float width, float height);
/** Places a child inside `area` according to its slot padding and alignment. */
void rvn_widget_arrange_slot(rvn_widget_t* child, rvn_rect_t area);
RVN_INLINE int rvn_widget_collapsed(const rvn_widget_t* widget) { return widget->visibility == R_COLLAPSED; }

/* ---------------------------------------------------------------------------
 *  Style (widget/style.c)
 * ------------------------------------------------------------------------- */

/** Theme a widget reads its values from. */
RVN_INLINE const rvn_theme_t* rvn_widget_theme(const rvn_widget_t* widget) {
	return widget->theme ? widget->theme : &g_rvn.theme;
}
/** Looks again for the styles that apply to the widget (slow path of the lookups below). */
void rvn_widget_restyle(rvn_widget_t* widget);
RVN_INLINE const rvn_style_match_t* rvn_widget_match(const rvn_widget_t* widget) {
	if (widget->style_generation != g_rvn.style_generation) rvn_widget_restyle((rvn_widget_t*)(uintptr_t)widget);
	return widget->match;
}
/** Material a widget is drawn with, and whether it is a glass. */
RVN_INLINE RMaterial rvn_widget_material(const rvn_widget_t* widget) {
	const rvn_style_match_t* match;
	if (widget->material) return widget->material;
	/* the looks of the platforms are opaque: only a widget told so itself is made of glass */
	if (rvn_widget_theme(widget)->family != RVN_FAMILY_RAVEN) return R_MATERIAL_SOLID;
	match = rvn_widget_match(widget);
	if (match && match->material) return match->material;
	return rvn_widget_theme(widget)->material;
}
RVN_INLINE int rvn_widget_glass(const rvn_widget_t* widget) {
	return rvn_widget_material(widget) != R_MATERIAL_SOLID;
}
/**
 * macOS Glass (macOS 26): the look of the widget is one of a platform whose
 * bars, menus and floating surfaces are drawn in glass by the widgets
 * themselves (rvn_widget_glass stays false: the material is solid).
 */
RVN_INLINE int rvn_widget_liquid(const rvn_widget_t* widget) {
	return rvn_widget_theme(widget)->liquid;
}
int rvn_parse_material(const char* text, RMaterial* material);

rvn_color_t rvn_widget_color(const rvn_widget_t* widget, RColorRole role);
float rvn_widget_metric(const rvn_widget_t* widget, RMetric metric);
int   rvn_widget_has_color(const rvn_widget_t* widget, RColorRole role);
/** R_TRUE when the widget itself, or a style of the theme for it, gives the metric (not only the theme). */
int   rvn_widget_has_metric(const rvn_widget_t* widget, RMetric metric);
void  rvn_widget_set_color(rvn_widget_t* widget, RColorRole role, rvn_color_t color);
void  rvn_widget_set_metric(rvn_widget_t* widget, RMetric metric, float value);
/** Color of the text of a widget: its override, else the inherited color, dimmed when disabled. */
rvn_color_t rvn_widget_text_color(const rvn_widget_t* widget, const RPainter* painter);
/** Makes a built-in theme the theme in use (Rlook_set). */
void  rvn_theme_apply(RTheme theme);
/** Fills a theme with a built-in one (its styles are removed; name and path are kept). */
void  rvn_theme_build(rvn_theme_t* theme, RTheme builtin);
/** Fills a theme with a look (its styles are removed; name and path are kept). */
void  rvn_theme_build_look(rvn_theme_t* theme, RLook look);

/* ---- looks (widget/style.c, widget/theme.c) ---- */

int   rvn_look_valid(RLook look);
/** The look a NATIVE one stands for on this platform; the look itself otherwise. */
RLook rvn_look_resolve(RLook look);
/** "WindowsDark", "RavenLight", "NativeDark"...; the names of Raven 0.2 ("Dark", "GlassLight"...) for the looks of Raven. */
const char* rvn_look_name(RLook look);
const char* rvn_look_old_name(RLook look);
/** Look by name, new ones and old ones; R_FALSE when unknown. */
int   rvn_look_by_name(const char* name, RLook* look);
/** The dark look of the same family (itself when it is dark): the island and what must stay dark. */
RLook rvn_look_dark_counterpart(RLook look);
int   rvn_look_dark(RLook look);
/**
 * Makes a look the one of the application. `keep_theme`: the theme file in
 * use, when it follows the look, is laid over it again (Rlook_set); else the
 * look replaces it (Rtheme_apply, Rtheme_use of a look, high contrast).
 */
void  rvn_look_apply(RLook look, int keep_theme);

/* ---- parts and states (widget/look.c) ---- */

/**
 * The parts of the widgets that styles can reach (<Style Part="Thumb">).
 * RVN_PART_BODY is the widget itself: a style without Part.
 */
typedef enum rvn_part_t {
	RVN_PART_BODY = 0,
	/** Check box: the box and its check mark; radio button: the circle and its dot. */
	RVN_PART_BOX,
	RVN_PART_MARK,
	RVN_PART_RADIO,
	RVN_PART_DOT,
	/** Slider, progress bar, switch: the groove, what fills it, the knob of a slider, the knob of a switch. */
	RVN_PART_TRACK,
	RVN_PART_FILL,
	RVN_PART_THUMB,
	RVN_PART_KNOB,
	/** The arrow of a combo box, of a spin box, of an expander, of a sub-menu. */
	RVN_PART_ARROW,
	/** The list of a combo box, a menu. */
	RVN_PART_POPUP,
	/** A row of a list, of a tree, of a menu, of a combo box; a title of a menu bar. */
	RVN_PART_ITEM,
	/** The mark of the selected row, tab or item (the pill of Windows). */
	RVN_PART_INDICATOR,
	RVN_PART_TAB,
	/** The header of an expander, the bar of the tabs. */
	RVN_PART_HEADER,
	RVN_PART_SCROLLBAR,
	RVN_PART_SCROLL_THUMB,
	RVN_PART_TOOLTIP,
	/** The buttons of the title bar (minimize, maximize), and the one that closes. */
	RVN_PART_CAPTION_BUTTON,
	RVN_PART_CLOSE_BUTTON,
	/** The ring of the keyboard focus. */
	RVN_PART_FOCUS,
	RVN_PART_COUNT
} rvn_part_t;

/** States a style can ask for (<Style State="Hovered Checked">). */
#define RVN_STATE_HOVERED  (1u << 0)
#define RVN_STATE_PRESSED  (1u << 1)
#define RVN_STATE_FOCUSED  (1u << 2)
#define RVN_STATE_DISABLED (1u << 3)
#define RVN_STATE_CHECKED  (1u << 4)
#define RVN_STATE_SELECTED (1u << 5)
#define RVN_STATE_OPEN     (1u << 6)
/** The styles of a button (Style="Primary", "Flat"). */
#define RVN_STATE_PRIMARY  (1u << 7)
#define RVN_STATE_FLAT     (1u << 8)

int rvn_part_by_name(const char* name, int* part);
const char* rvn_part_name(int part);
/** Parses "Hovered Checked" into RVN_STATE_... bits; R_FALSE (with the word in `bad`) on an unknown word. */
int rvn_states_parse(const char* text, uint32_t* states, char* bad, size_t bad_size);
/** Writes the names of the states, separated by spaces. */
void rvn_states_write(uint32_t states, rvn_text_t* out);
/** The property of a part named in a style: R_TRUE and its RVN_LOOK_ bit, which `value` is parsed into. */
int rvn_part_property(const char* name, const char* value, rvn_part_look_t* look, uint32_t* bit, int* bad_value);
/** Name of the property of an RVN_LOOK_ bit. */
const char* rvn_part_property_name(uint32_t bit);

/** States of a widget that every widget has: hovered (`hovered`), pressed (`pressed`), focused, disabled. */
uint32_t rvn_widget_states(const rvn_widget_t* widget, const RPainter* painter, int hovered, int pressed);

/** What the look of a family gives to a part in some states, before the styles: filled by the widget. */
typedef void (*rvn_part_base_t)(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context);

/** A part in some states: what `base` gives (nothing when NULL), then what the styles of the theme change. */
void rvn_part_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_base_t base, const void* context, rvn_part_look_t* look);
/**
 * Same, eased between the states: `hover` and `press` (0..1, the springs of
 * the widget) blend the look without HOVERED and PRESSED, with HOVERED, and
 * with both.
 */
void rvn_part_look_eased(const rvn_widget_t* widget, int part, uint32_t states, float hover, float press,
	rvn_part_base_t base, const void* context, rvn_part_look_t* look);
/** R_TRUE when the styles of the theme give something to this part of the widget. */
int  rvn_part_styled(const rvn_widget_t* widget, int part);
/** Applies what the styles give to a part in some states over `look`. */
void rvn_part_apply_styles(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look);
void rvn_part_look_clear(rvn_part_look_t* look);
void rvn_part_look_mix(rvn_part_look_t* a, const rvn_part_look_t* b, float t);
/** Draws a part: its shadow, its background, its border (with its own top and bottom edges). */
void rvn_paint_part(RPainter* painter, rvn_rect_t rect, const rvn_part_look_t* look);

/** The family of the look of a widget: RVN_FAMILY_... */
RVN_INLINE int rvn_widget_family(const rvn_widget_t* widget) {
	return rvn_widget_theme(widget)->family;
}
/** How a widget moves: the motion asked for, calmed down by the looks that do not bounce. */
RMotion rvn_widget_motion(const rvn_widget_t* widget, RMotion motion);
/** R_TRUE when the look of the widget squashes and bounces (Raven), R_FALSE for the platforms. */
int rvn_widget_springy(const rvn_widget_t* widget);

/** The font of the text of a family of the platforms (Segoe UI), opened on first use; R_INVALID_ID when the system has none. */
Rid  rvn_look_platform_font(int family);
/** The font of its icons (Segoe Fluent Icons, Segoe MDL2 Assets); R_INVALID_ID when the system has none. */
Rid  rvn_look_icon_font(int family);
/** Draws an icon of the font of icons of the family, centered on (cx, cy); R_FALSE when there is no such font (draw it by hand). */
int  rvn_paint_look_icon(RPainter* painter, const rvn_widget_t* widget, uint32_t codepoint, float cx, float cy, float size, rvn_color_t color);
/** Gives a theme of a platform the accent color the user chose for the system, when the system tells it. */
void rvn_look_system_accent(rvn_theme_t* theme);
/** Forgets the fonts of the platforms (shutdown). */
void rvn_look_shutdown(void);
/** Wallpaper of the built-in glass themes. */
Rid   rvn_theme_builtin_wallpaper(int dark);
/** A soft gradient made of a base color and up to five blobs of color (0xRRGGBB). */
Rid   rvn_wallpaper_create_colors(uint32_t base, const uint32_t* blobs, int count);

/* ---- named themes, styles, theme files (widget/theme.c) ---- */

/** Named theme, the built-in ones included. NULL when unknown. */
rvn_theme_t* rvn_theme_find(const char* name);
/** Releases what a theme owns and zeroes it. */
void  rvn_theme_clear(rvn_theme_t* theme);
/** Deep copy; `name` and `path` of the destination are kept. */
void  rvn_theme_copy(rvn_theme_t* destination, const rvn_theme_t* source);
/** Something a widget may read from a theme changed: every widget is styled, measured and painted again. */
void  rvn_theme_changed(void);
/** Empties the caches of the styles after a rule or the classes of a widget changed. */
void  rvn_styles_changed(void);
/** The accent color and what follows from it. */
void  rvn_theme_set_accent(rvn_theme_t* theme, rvn_color_t accent);
/** Gives a theme to a widget (`own`) or lets it follow its parent again (theme NULL, own 0), and tells its subtree. */
void  rvn_widget_set_theme(rvn_widget_t* widget, const rvn_theme_t* theme, int own);
/** The widget has a new parent or a new popup owner: it follows the theme of that one unless it has its own. */
void  rvn_widget_inherit_theme(rvn_widget_t* widget, const rvn_theme_t* theme);
/** The dark counterpart of the theme in use (itself when it is dark): the island and what must stay dark. */
const rvn_theme_t* rvn_theme_dark_twin(void);
int   rvn_widget_has_class(const rvn_widget_t* widget, const char* cls);
void  rvn_themes_init(void);
void  rvn_themes_shutdown(void);
/** A watched theme file changed (xml/xml_watch.c). */
void  rvn_theme_file_changed(const char* path);
int   rvn_color_role_by_name(const char* name, RColorRole* role);
int   rvn_metric_by_name(const char* name, RMetric* metric);
const char* rvn_color_role_name(RColorRole role);
const char* rvn_metric_name(RMetric metric);

/* ---------------------------------------------------------------------------
 *  Painting and hit testing (widget/paint.c)
 * ------------------------------------------------------------------------- */

/** Paints a widget and its subtree. `parent` describes the space of the parent. */
void rvn_widget_paint(rvn_widget_t* widget, const RPainter* parent);
/** Deepest widget under a point given in the coordinate space of the parent of `widget`. */
rvn_widget_t* rvn_widget_hit_test(rvn_widget_t* widget, float x, float y);
void rvn_paint_init(void);
void rvn_paint_shutdown(void);
/** Number of paint jobs started since the last call (statistics). */
int  rvn_paint_take_job_count(void);

/* ---------------------------------------------------------------------------
 *  Animation (widget/anim.c)
 * ------------------------------------------------------------------------- */

/** Time by which the animations of a window advance, `elapsed` seconds after its previous frame. */
float rvn_animation_dt(double elapsed);
/** Starts a transition now. */
void rvn_widget_play_transition(rvn_widget_t* widget, RTransition transition, float delay);
/** Changes the transition a widget plays when it appears, and / or its delay (NULL = keep). */
void rvn_widget_set_appear(rvn_widget_t* widget, const RTransition* transition, const float* delay);
/** The widget has just been shown: plays the transition it was given, if any. */
void rvn_widget_appear(rvn_widget_t* widget);
/**
 * Advances the transition of a widget about to be painted. Returns R_FALSE
 * when the widget is not to be drawn at all this frame (it waits its turn).
 */
int rvn_transition_begin(rvn_widget_t* widget, RPainter* painter, rvn_transition_state_t* state);
int rvn_parse_transition(const char* text, RTransition* transition);

/* ---------------------------------------------------------------------------
 *  Events (widget/events.c)
 * ------------------------------------------------------------------------- */

/** Sends an event to a widget and bubbles it to the ancestors. Returns R_TRUE when handled. */
int  rvn_widget_dispatch(rvn_widget_t* target, REvent* event);
/** Sends an event to one widget only. */
int  rvn_widget_send(rvn_widget_t* target, REvent* event);
/** Fires the action of the widget (Rwidget_set_action). */
void rvn_widget_fire_action(rvn_widget_t* widget);
/** Sends all further mouse events to the widget until the capture is released. */
void rvn_widget_capture_mouse(rvn_widget_t* widget);
void rvn_widget_release_mouse(rvn_widget_t* widget);
void rvn_widget_focus(rvn_widget_t* widget);

/* ---------------------------------------------------------------------------
 *  Window (widget/window.c)
 * ------------------------------------------------------------------------- */

typedef enum rvn_popup_placement_t {
	/** Below the anchor, left edges aligned; above when there is no room. */
	RVN_POPUP_BELOW,
	/** Right of the anchor, top edges aligned; left when there is no room. */
	RVN_POPUP_RIGHT,
	/** Top-left corner on the anchor position. */
	RVN_POPUP_AT,
	/** A dialog: in the middle of the window, above a veil that keeps the rest out of reach. */
	RVN_POPUP_DIALOG
} rvn_popup_placement_t;

typedef struct rvn_popup_t {
	rvn_widget_t* root;
	/** Widget that opened the popup; receives the focus back. May be NULL. */
	rvn_widget_t* owner;
	rvn_rect_t anchor;
	rvn_popup_placement_t placement;
	/** Minimum width (combo box lists match their button). */
	float min_width;
	/** 0 = just opened .. 1 = fully shown: the popup grows out of its anchor. */
	RSpring shown;
	/** Dialogs: the value given when closing, and who to tell. The window deletes the root of a dialog. */
	int result;
	RDialogClosedCallback closed;
	void* closed_user;
	/** Dialogs: the widget that had the keyboard before, which gets it back. */
	Rid focus_back;
} rvn_popup_t;

/**
 * A spring that changes the layout (the height of an expander, the place of
 * a docked panel). Paint functions may run on any thread and cannot ask for
 * a new layout, so these springs are advanced by the window itself, on the UI
 * thread, before each layout.
 */
typedef struct rvn_layout_spring_t {
	rvn_widget_t* widget;
	RSpring* spring;
	float target;
	RMotion motion;
} rvn_layout_spring_t;

/** A popup that was closed and is still shrinking away. It takes no input. */
typedef struct rvn_ghost_t {
	rvn_widget_t* root;
	rvn_rect_t anchor;
	RSpring shown;
	/** A dialog: drawn with its veil, and deleted once it is gone. */
	int dialog;
} rvn_ghost_t;

typedef struct rvn_shortcut_t {
	RKey key;
	uint32_t modifiers;
	char* action;
} rvn_shortcut_t;

struct rvn_window_t {
	Rid id;
	/** Node of the public XML document the window was built from, or 0. */
	Rid xml_node;
	rvn_pwindow_t* platform;
	rvn_widget_t* root;
	/** A widget floating at the top of the window above the content (Rwindow_set_island), or NULL. */
	rvn_widget_t* island;
	char* title;

	/** Size of the client area in pixels and pixels per logical unit. */
	int width_px, height_px;
	float scale;
	int visible;
	int surface_ready;
	/** RWindowFlags given at creation. */
	uint32_t window_flags;
	rvn_pwindow_state_t state;
	/** R_TRUE while the window is the one the user works in. */
	int active;
	/** What the screen reader was last told (core/accessibility.c). */
	uint64_t a11y_focus;
	uint32_t a11y_hash;
	uint32_t a11y_generation;

	int needs_layout;
	int needs_paint;
	int continuous;
	/** Set from any thread by Rwindow_request_redraw. */
	rvn_atomic_t redraw_requested;
	/** Frames queued or being rendered. */
	rvn_atomic_t frames_in_flight;
	rvn_atomic_t render_us;
	/** Realtime viewports seen during the last paint. */
	rvn_atomic_t realtime_viewports;
	/** Set while painting by widgets whose animation is not finished: another frame is needed. */
	rvn_atomic_t animating;
	/** When the window was last painted, for the pace of the animations. */
	double paint_time;
	/** Image drawn behind the content (Rwindow_set_background), or R_INVALID_ID. */
	Rid background;

	int has_clear_color;
	float clear_color[4];
	/** R_TRUE when the surface of the window has an alpha the window system uses (R_WINDOW_TRANSPARENT that worked). */
	int transparent;
	/** What was asked for behind the window, and what the window system gives (0 = R_BACKDROP_NONE). */
	RBackdrop backdrop_wanted;
	RBackdrop backdrop;
	/** The theme was dark when the window system was last told about the backdrop. */
	int backdrop_dark;
	float opacity;

	/* input state */
	rvn_widget_t* hover;
	rvn_widget_t* focus;
	/** Last painted editable caret, in surface pixels; its owner guards against stale focus. */
	rvn_box_t text_input_rect;
	Rid text_input_owner;
	rvn_widget_t* capture;
	/** R_TRUE when the focus was moved with the keyboard: widgets then draw a focus outline. */
	int focus_visible;
	float mouse_x, mouse_y;
	int mouse_inside;
	uint32_t modifiers;
	uint32_t buttons;
	double click_time;
	float click_x, click_y;
	int click_count;
	int click_button;
	RCursor cursor;
	/** Custom header: the resize edge under the pointer, when the window system does not handle them itself. */
	rvn_pregion_t edge;

	/* drag and drop from other applications */
	RDropCallback drop_callback;
	void* drop_user;
	/** What would take a drop at the pointer: a widget, or the window itself (drop_on_window). */
	rvn_widget_t* drop_target;
	int drop_on_window;
	int drop_accepted;
	/** Highlight of the target: where it is and how much of it shows. */
	rvn_rect_t drop_rect;
	float drop_radius;
	RSpring drop_shown;

	/* popups, bottom to top */
	rvn_popup_t* popups;
	uint32_t popup_count;
	uint32_t popup_cap;
	rvn_ghost_t* ghosts;
	uint32_t ghost_count;
	uint32_t ghost_cap;
	/** The subtree that is leaving the window right now (rvn_window_forget_widget), or NULL. */
	rvn_widget_t* forgetting;
	rvn_layout_spring_t* layout_springs;
	uint32_t layout_spring_count;
	uint32_t layout_spring_cap;

	/* tooltip */
	rvn_widget_t* tooltip_widget;
	Rid tooltip_timer;
	int tooltip_visible;
	float tooltip_x, tooltip_y;
	RSpring tooltip_shown;

	rvn_shortcut_t* shortcuts;
	uint32_t shortcut_count;
	uint32_t shortcut_cap;

	/* viewports of this window that own a native child window */
	rvn_widget_t** native_viewports;
	uint32_t native_viewport_count;
	uint32_t native_viewport_cap;

	RWindowCloseCallback close_callback;
	void* close_user;
	RWindowResizeCallback resize_callback;
	void* resize_user;
	void* user_handle;

	/** Keys held down (Rwindow_is_key_down), indexed by RKey. */
	uint8_t keys[R_KEY_COUNT];
	/** R_FALSE: Tab does not move the keyboard focus (Rwindow_set_tab_navigation). */
	int tab_navigation;
	/** Drag and drop inside the application (Rdrag_begin). */
	int drag_active;
	char* drag_type;
	char* drag_data;
	char* drag_label;
	Rid drag_source;
	/** Sees every input event of the window before the widgets (Rwindow_set_event_hook). */
	RWindowEventHook event_hook;
	void* event_hook_user;

	RFrameStats stats;
	uint64_t frame_index;
	int dead;
};

rvn_window_t* rvn_window_get(Rid id);
rvn_window_t* rvn_window_check(Rid id, const char* function);
/** Entry point of the platform events. */
void rvn_window_on_platform_event(const rvn_pevent_t* event);
/** What a pixel of a window with a custom header is, for the window system (rvn_pregion_handler_t). */
rvn_pregion_t rvn_window_region_at(void* user, int x, int y);
/** Asks to close the window as the user would: the close callback may refuse. */
void rvn_window_request_close(rvn_window_t* window);
/** Maximizes the window, or restores it when it is maximized. */
void rvn_window_toggle_maximized(rvn_window_t* window);
/** Lays out and paints the window when something changed. */
void rvn_window_update(rvn_window_t* window);
/** Runs the layout passes now if the window needs them. */
void rvn_window_layout(rvn_window_t* window);
/** R_TRUE when the window will produce a frame at the next update. */
int  rvn_window_wants_frame(rvn_window_t* window);
/*
 * Waits until the windows that will paint can take a frame (the previous one
 * left for the renderer). Called before the input is read: the frame that
 * follows shows the pointer where it is now, not where it was a frame ago.
 */
void rvn_windows_throttle(void);
/** Clears every reference the window holds to a widget about to be destroyed. */
void rvn_window_forget_widget(rvn_window_t* window, rvn_widget_t* widget);
void rvn_window_set_focus(rvn_window_t* window, rvn_widget_t* widget);
void rvn_window_destroy(rvn_window_t* window);
void rvn_windows_destroy_all(void);
void rvn_windows_collect(void);
/** Logical size of the client area. */
rvn_vec2_t rvn_window_size(const rvn_window_t* window);
/** R_TRUE when what is behind the window on the screen shows where the window draws nothing. */
int  rvn_window_see_through(const rvn_window_t* window);
void rvn_window_add_shortcut(rvn_window_t* window, RKey key, uint32_t modifiers, const char* action);
void rvn_window_remove_shortcut(rvn_window_t* window, RKey key, uint32_t modifiers);

/**
 * Moves a spring that belongs to `widget` toward `target`, giving the widget
 * a new layout at every step. The spring must live as long as the widget.
 * Without a window, or with the animations off, the value is set at once.
 */
void rvn_widget_animate_layout(rvn_widget_t* widget, RSpring* spring, float target, RMotion motion);
/** To call before releasing the memory of a spring given to rvn_widget_animate_layout while its widget lives on. */
void rvn_widget_forget_layout_spring(rvn_widget_t* widget, const RSpring* spring);

/** Shows a widget tree above the window content. The popup does not own `root`. */
void rvn_window_open_popup(rvn_window_t* window, rvn_widget_t* root, rvn_widget_t* owner,
	rvn_rect_t anchor, rvn_popup_placement_t placement, float min_width);
/** Closes a popup and all the popups opened above it. */
void rvn_window_close_popup(rvn_window_t* window, rvn_widget_t* root);
/** The tooltip text of the widget under the pointer changed: the tooltip starts over. */
void rvn_window_rearm_tooltip(rvn_window_t* window);
/* ---- core/locale.c ---- */
/** "@key" values of attributes: R_TRUE when handled (the result of the widget in *result). */
int rvn_locale_attribute(rvn_widget_t* widget, const char* name, const char* value, int* result);
/** R_TRUE when the current language is written from right to left. */
int rvn_locale_rtl(void);
int rvn_locale_code_is_rtl(const char* code);
void rvn_locale_shutdown(void);

/* ---- core/accessibility.c ---- */
float rvn_a11y_text_scale(void);
int   rvn_a11y_focus_always(void);
float rvn_a11y_focus_width(void);
void  rvn_a11y_structure_changed(void);
void  rvn_a11y_widget_destroyed(rvn_widget_t* widget);
void  rvn_a11y_shutdown(void);
void  rvn_a11y_filter_instances(RDrawInstance* instances, uint32_t count);
/** After a frame of the window: tells the screen reader what changed. */
void  rvn_a11y_frame(rvn_window_t* window);
/** XML attributes Accessible*: R_TRUE / R_FALSE when the name is one of them, -1 otherwise. */
int   rvn_a11y_attribute(rvn_widget_t* widget, const char* name, const char* value);

/* ---- widget/direction.c ---- */
/** The direction of the windows follows the language (Rset_layout_direction(R_DIRECTION_AUTO)): lays them out again. */
void rvn_direction_refresh(void);
/** R_TRUE when the widget is laid out from right to left. */
int rvn_widget_is_rtl(const rvn_widget_t* widget);
/** Direction of the windows when nothing more precise is given. */
int rvn_direction_global_rtl(void);

/** Closes the menus and the lists that are open. Dialogs stay, with what is under them. */
void rvn_window_close_popups(rvn_window_t* window);
/**
 * Shows a widget as a dialog of the window. The window owns `root` from now
 * on and deletes it when the dialog closes; `closed` is called at that moment.
 */
void rvn_window_open_dialog(rvn_window_t* window, rvn_widget_t* root, RDialogClosedCallback closed, void* user);
/** Closes the dialog that contains `inside`, giving `result` to its callback. R_FALSE when there is none. */
int  rvn_window_close_dialog(rvn_widget_t* inside, int result);
/** Root of the dialog on top of the others, or NULL. */
rvn_widget_t* rvn_window_top_dialog(const rvn_window_t* window);
int  rvn_window_popup_index(const rvn_window_t* window, const rvn_widget_t* root);

/** Triggers a menu item (shortcuts). Implemented in widgets/menu.c. */
void rvn_menu_item_trigger(rvn_widget_t* item);
/** Destroys the native child windows of the viewports inside a subtree leaving the window. */
void rvn_window_release_natives(rvn_window_t* window, rvn_widget_t* subtree);
/** Native viewports (widgets/viewport.c): release the child window / apply its state after a paint. */
void rvn_viewport_native_release(rvn_widget_t* widget, rvn_window_t* window);
void rvn_viewport_native_sync(rvn_widget_t* widget, uint64_t frame);

/** Enabled menu item bound to a shortcut anywhere below `root`, or NULL. Implemented in widgets/menu.c. */
rvn_widget_t* rvn_menu_find_shortcut(rvn_widget_t* root, RKey key, uint32_t modifiers);
/** Parses "Ctrl+Shift+S". Implemented in widgets/menu.c. */
int rvn_parse_shortcut(const char* text, RKey* key, uint32_t* modifiers);
/** Opens a menu as a popup at a window position. Implemented in widgets/menu.c. */
void rvn_menu_popup(rvn_widget_t* menu, rvn_window_t* window, float x, float y);

/* ---------------------------------------------------------------------------
 *  Attribute parsing (xml/xml_attributes.c), shared by all set_attribute functions
 * ------------------------------------------------------------------------- */

int rvn_parse_float(const char* text, float* value);
int rvn_parse_int(const char* text, int* value);
int rvn_parse_bool(const char* text, int* value);
/** "#RGB", "#RRGGBB", "#RRGGBBAA" or a few color names. */
int rvn_parse_color(const char* text, rvn_color_t* color);
/** "all", "horizontal,vertical" or "left,top,right,bottom". */
int rvn_parse_margin(const char* text, rvn_margin_t* margin);
/** Up to `max` comma-separated numbers; returns how many were read. */
int rvn_parse_floats(const char* text, float* values, int max);
int rvn_parse_orientation(const char* text, ROrientation* orientation);
/** Sets a base property (name, visibility, slot, size, colors...). */
int rvn_widget_set_base_attribute(rvn_widget_t* widget, const char* name, const char* value);
/** Sets any attribute: the ones of the widget class first, then the base ones. */
int rvn_widget_set_attribute(rvn_widget_t* widget, const char* name, const char* value);

#endif /* RVN_WIDGET_INTERNAL_H */
