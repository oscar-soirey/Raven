/**
 * Raven - core/accessibility.h
 * The tree that screen readers see, for the platforms (UI Automation,
 * NSAccessibility, AT-SPI). Implemented in core/accessibility.c. UI thread.
 *
 * A node is a window, a widget, or an item of a widget (a row of a list):
 *     window    the Rid of the window
 *     widget    the Rid of the widget
 *     item      the Rid of the widget | (index + 1) << 32
 * The widgets that only lay others out are not nodes: their children are
 * the children of the nearest node above them.
 */

#ifndef RVN_ACCESSIBILITY_INTERNAL_H
#define RVN_ACCESSIBILITY_INTERNAL_H

#include "base.h"
#include "raven/raven_accessibility.h"

typedef uint64_t rvn_a11y_id_t;

#define RVN_A11Y_FOCUSABLE   (1u << 0)
#define RVN_A11Y_FOCUSED     (1u << 1)
#define RVN_A11Y_DISABLED    (1u << 2)
#define RVN_A11Y_CHECKED     (1u << 3)
#define RVN_A11Y_CHECKABLE   (1u << 4)
#define RVN_A11Y_EXPANDED    (1u << 5)
#define RVN_A11Y_EXPANDABLE  (1u << 6)
#define RVN_A11Y_SELECTED    (1u << 7)
#define RVN_A11Y_SELECTABLE  (1u << 8)
#define RVN_A11Y_MULTILINE   (1u << 9)
#define RVN_A11Y_READONLY    (1u << 10)
#define RVN_A11Y_PASSWORD    (1u << 11)
#define RVN_A11Y_HAS_RANGE   (1u << 12)  /* value, min, max are numbers */
#define RVN_A11Y_EDITABLE    (1u << 13)  /* the value is a text that can be set */
#define RVN_A11Y_INVOKABLE   (1u << 14)  /* it can be pressed */
#define RVN_A11Y_OFFSCREEN   (1u << 15)
#define RVN_A11Y_MODAL       (1u << 16)
#define RVN_A11Y_MULTI_SELECT (1u << 17)
#define RVN_A11Y_TABLE_ROW    (1u << 18)

/** What a node is now. The strings stay valid until the next call to rvn_a11y_describe. */
typedef struct rvn_a11y_node_t {
	rvn_a11y_id_t id;
	RAccessibleRole role;
	const char* name;
	const char* description;
	/** Text value (a text field, the selected item of a list), or "". */
	const char* value;
	double number, minimum, maximum, step;
	uint32_t states;
	/** In the window, logical units. */
	float x, y, width, height;
} rvn_a11y_node_t;

typedef enum rvn_a11y_event_t {
	RVN_A11Y_EVENT_FOCUS = 1,
	RVN_A11Y_EVENT_NAME,
	RVN_A11Y_EVENT_VALUE,
	RVN_A11Y_EVENT_STATE,
	RVN_A11Y_EVENT_STRUCTURE
} rvn_a11y_event_t;

typedef enum rvn_a11y_action_t {
	RVN_A11Y_PRESS = 1,   /* button, menu item, link, tab, list item: what a click does */
	RVN_A11Y_TOGGLE,      /* check box */
	RVN_A11Y_EXPAND,
	RVN_A11Y_COLLAPSE,
	RVN_A11Y_FOCUS,
	RVN_A11Y_SELECT,
	RVN_A11Y_ADD_SELECTION,
	RVN_A11Y_REMOVE_SELECTION,
	RVN_A11Y_CLEAR_SELECTION
} rvn_a11y_action_t;

/** The window a platform window shows (its `user`). */
Rid rvn_a11y_window_of(void* platform_user);

/** R_FALSE when the node is gone. */
int  rvn_a11y_describe(rvn_a11y_id_t id, rvn_a11y_node_t* node);
int  rvn_a11y_exists(rvn_a11y_id_t id);
/** The window of a node. */
Rid  rvn_a11y_window(rvn_a11y_id_t id);
/** 0 for a window. */
rvn_a11y_id_t rvn_a11y_parent(rvn_a11y_id_t id);
int  rvn_a11y_child_count(rvn_a11y_id_t id);
rvn_a11y_id_t rvn_a11y_child(rvn_a11y_id_t id, int index);
/** Index among the children of its parent, -1 for a window. */
int  rvn_a11y_index_in_parent(rvn_a11y_id_t id);
/** The deepest node at a point of the window (logical units), or the window. */
rvn_a11y_id_t rvn_a11y_hit(Rid window, float x, float y);
/** The node that has the focus in the window, or the window. */
rvn_a11y_id_t rvn_a11y_focus(Rid window);

int  rvn_a11y_act(rvn_a11y_id_t id, rvn_a11y_action_t action);
int rvn_a11y_selection_count(rvn_a11y_id_t id);
rvn_a11y_id_t rvn_a11y_selection_at(rvn_a11y_id_t id, int index);
int  rvn_a11y_set_value(rvn_a11y_id_t id, const char* text);
int  rvn_a11y_set_number(rvn_a11y_id_t id, double value);

/** Where a node is on the screen, in pixels (for the platforms that ask in screen coordinates). */
void rvn_a11y_screen_rect(rvn_a11y_id_t id, int* x, int* y, int* width, int* height);
/** Pixels per logical unit of the window of a node, and where its client area is on the screen. */
void rvn_a11y_window_origin(rvn_a11y_id_t id, int* x, int* y, float* scale);
/** The language of the application, for the screen reader ("en", "fr-FR"). */
const char* rvn_a11y_locale(void);

/** Names of the roles, for logs and for the platforms that want one ("push button"). */
const char* rvn_a11y_role_name(RAccessibleRole role);

#endif
