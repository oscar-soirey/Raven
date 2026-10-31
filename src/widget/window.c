/**
 * Raven - widget/window.c
 * Windows: layout and paint scheduling, input routing (hover, focus, mouse
 * capture, popups, tooltips, shortcuts) and the public window API.
 */

#include "widget.h"
#include "../widgets/common.h"
#include "../render/renderer.h"

/* one frame waits to be rendered at most: the next one is painted with what the pointer did last (see rvn_windows_throttle) */
#define RVN_MAX_FRAMES_IN_FLIGHT 1
/** Backdrop captures per frame (glass). The first one blurs the whole window, the others an area of it. */
#define RVN_MAX_BACKDROPS 24
#define RVN_DOUBLE_CLICK_TIME 0.4
#define RVN_DOUBLE_CLICK_DISTANCE 4.0f
#define RVN_TOOLTIP_DELAY 0.6
#define RVN_MAX_PATH_DEPTH 128

/* ---------------------------------------------------------------------------
 *  Lookup
 * ------------------------------------------------------------------------- */

rvn_window_t* rvn_window_get(Rid id) {
	rvn_window_t* window = (rvn_window_t*)rvn_pool_get(&g_rvn.windows, id);
	return (window && !window->dead) ? window : NULL;
}

rvn_window_t* rvn_window_check(Rid id, const char* function) {
	rvn_window_t* window;
	if (!rvn_api_check(function)) return NULL;
	window = rvn_window_get(id);
	if (!window) rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "%s: invalid window handle 0x%08X", function, (unsigned)id);
	return window;
}

#define RVN_WINDOW(variable, id, result) \
	rvn_window_t* variable = rvn_window_check((id), __func__); \
	if (!variable) return result

#define RVN_WINDOW_VOID(variable, id) \
	rvn_window_t* variable = rvn_window_check((id), __func__); \
	if (!variable) return

rvn_vec2_t rvn_window_size(const rvn_window_t* window) {
	float scale = window->scale > 0.0f ? window->scale : 1.0f;
	return rvn_vec2((float)window->width_px / scale, (float)window->height_px / scale);
}

static REvent rvn_window_event(rvn_window_t* window, REventType type) {
	REvent event;
	memset(&event, 0, sizeof(event));
	event.type = type;
	event.window = window->id;
	event.target = R_INVALID_ID;
	event.x = window->mouse_x;
	event.y = window->mouse_y;
	event.modifiers = window->modifiers;
	return event;
}

/* ---------------------------------------------------------------------------
 *  Tooltip
 * ------------------------------------------------------------------------- */

static void rvn_window_forget_ghost(rvn_window_t* window, const rvn_widget_t* root);
static void rvn_window_paint_drop(rvn_window_t* window, RPainter* painter);
static void rvn_window_paint_drag(rvn_window_t* window, RPainter* painter);

static void rvn_window_tooltip_hide(rvn_window_t* window) {
	if (window->tooltip_timer != R_INVALID_ID) {
		Rtimer_destroy(window->tooltip_timer);
		window->tooltip_timer = R_INVALID_ID;
	}
	if (window->tooltip_visible) {
		window->tooltip_visible = R_FALSE;
		window->needs_paint = R_TRUE;
	}
	window->tooltip_widget = NULL;
}

static void rvn_window_tooltip_fire(Rid timer, void* user_data) {
	rvn_window_t* window = rvn_window_get((Rid)(uintptr_t)user_data);
	RVN_UNUSED(timer);
	if (!window) return;
	window->tooltip_timer = R_INVALID_ID;
	if (window->tooltip_widget && window->tooltip_widget->tooltip) {
		window->tooltip_visible = R_TRUE;
		memset(&window->tooltip_shown, 0, sizeof(window->tooltip_shown));
		window->tooltip_x = window->mouse_x;
		window->tooltip_y = window->mouse_y;
		window->needs_paint = R_TRUE;
	}
}

/* Arms the tooltip of the nearest widget with a tooltip text under the cursor. */
static void rvn_window_tooltip_arm(rvn_window_t* window) {
	rvn_widget_t* walk;
	rvn_window_tooltip_hide(window);
	for (walk = window->hover; walk; walk = walk->parent) {
		if (walk->tooltip) break;
		if (walk->flags & RVN_WF_POPUP) {
			walk = NULL;
			break;
		}
	}
	if (!walk) return;
	window->tooltip_widget = walk;
	window->tooltip_timer = Rtimer_create(RVN_TOOLTIP_DELAY, R_FALSE, rvn_window_tooltip_fire,
		(void*)(uintptr_t)window->id);
}

void rvn_window_rearm_tooltip(rvn_window_t* window) {
	if (!window->dead) rvn_window_tooltip_arm(window);
}

/* The tooltip of a platform: a small flyout that fades in where it is. Windows: text at 12; macOS: 11; GNOME: dark, 13. */
static void rvn_window_paint_tooltip_fluent(rvn_window_t* window, RPainter* painter) {
	const rvn_widget_t* owner = window->tooltip_widget;
	const char* text = owner->tooltip;
	const char* line;
	rvn_font_t* font = rvn_font_resolve(R_INVALID_ID);
	rvn_font_metrics_t metrics;
	rvn_vec2_t size = rvn_window_size(window);
	const int family = rvn_widget_family(owner);
	float font_size = family == RVN_FAMILY_MACOS ? 11.0f : family == RVN_FAMILY_GNOME ? 13.0f : 12.0f;
	float pad_x = family == RVN_FAMILY_MACOS ? 6.0f : family == RVN_FAMILY_GNOME ? 10.0f : 9.0f;
	float pad_y = family == RVN_FAMILY_MACOS ? 3.0f : family == RVN_FAMILY_GNOME ? 6.0f : 6.0f;
	float width = 0.0f, height, x, y, shown, opacity = painter->opacity;
	rvn_part_look_t look;
	int lines = 0;

	if (!font) return;
	shown = rvn_paint_spring_from(painter, &window->tooltip_shown, 1.0f, R_MOTION_SMOOTH);
	painter->opacity *= rvn_clampf(shown, 0.0f, 1.0f);
	rvn_font_metrics(font, font_size, &metrics);
	for (line = text;;) {
		const char* newline = strchr(line, '\n');
		int length = newline ? (int)(newline - line) : (int)strlen(line);
		width = rvn_maxf(width, rvn_text_width(font, font_size, line, length));
		lines++;
		if (!newline) break;
		line = newline + 1;
	}
	width = ceilf(width) + pad_x * 2.0f;
	height = ceilf(metrics.line_height * (float)lines) + pad_y * 2.0f + 2.0f;
	x = rvn_clampf(window->tooltip_x + 12.0f, 0.0f, rvn_maxf(0.0f, size.x - width));
	y = window->tooltip_y + 22.0f;
	if (y + height > size.y) y = window->tooltip_y - height - 6.0f;
	if (y < 0.0f) y = 0.0f;
	rvn_part_look(owner, RVN_PART_TOOLTIP, 0, rvn_platform_popup_look, NULL, &look);
	if (rvn_widget_liquid(owner) && !rvn_part_styled(owner, RVN_PART_TOOLTIP)) {
		/* macOS 26: a drop of glass of the color of tooltips */
		rvn_glass_t glass;
		float radii[4];
		rvn_glass_style(owner, RVN_SURFACE_POPUP, 0.0f, 0.0f, &glass);
		glass.tint = look.background;
		if (rvn_color_a(glass.tint) > 210) glass.tint = (glass.tint & 0x00FFFFFFu) | (210u << 24);
		glass.shadow = 8.0f;
		radii[0] = radii[1] = radii[2] = radii[3] = look.radius;
		rvn_paint_glass(painter, rvn_rect(x, y, width, height), radii, &glass);
	} else {
		rvn_paint_part(painter, rvn_rect(x, y, width, height), &look);
	}
	y += pad_y + 1.0f;
	for (line = text;;) {
		const char* newline = strchr(line, '\n');
		int length = newline ? (int)(newline - line) : (int)strlen(line);
		rvn_paint_text(painter, R_INVALID_ID, font_size, x + pad_x, y, line, length, look.foreground);
		y += metrics.line_height;
		if (!newline) break;
		line = newline + 1;
	}
	painter->opacity = opacity;
}

static void rvn_window_paint_tooltip(rvn_window_t* window, RPainter* painter) {
	const char* text;
	const char* line;
	rvn_font_t* font;
	rvn_font_metrics_t metrics;
	rvn_vec2_t size = rvn_window_size(window);
	float font_size = RVN_THEME_METRIC(R_METRIC_FONT_SIZE);
	float width = 0.0f, height, x, y, shown, opacity = painter->opacity;
	uint32_t first_instance = painter->list->instance_count, first_command = painter->list->command_count;
	int lines = 0;

	if (!window->tooltip_visible || !window->tooltip_widget || !window->tooltip_widget->tooltip) return;
	if (rvn_widget_family(window->tooltip_widget) != RVN_FAMILY_RAVEN) {
		rvn_window_paint_tooltip_fluent(window, painter);
		return;
	}
	shown = rvn_paint_spring_from(painter, &window->tooltip_shown, 1.0f, R_MOTION_BOUNCY);
	painter->opacity *= rvn_clampf(shown * 2.0f, 0.0f, 1.0f);
	text = window->tooltip_widget->tooltip;
	font = rvn_font_resolve(R_INVALID_ID);
	if (!font) return;
	rvn_font_metrics(font, font_size, &metrics);

	for (line = text;;) {
		const char* newline = strchr(line, '\n');
		int length = newline ? (int)(newline - line) : (int)strlen(line);
		width = rvn_maxf(width, rvn_text_width(font, font_size, line, length));
		lines++;
		if (!newline) break;
		line = newline + 1;
	}
	width += 16.0f;
	height = metrics.line_height * (float)lines + 10.0f;
	x = rvn_clampf(window->tooltip_x + 12.0f, 0.0f, rvn_maxf(0.0f, size.x - width));
	y = window->tooltip_y + 20.0f;
	if (y + height > size.y) y = window->tooltip_y - height - 6.0f;
	if (y < 0.0f) y = 0.0f;

	if (g_rvn.theme.glass) {
		rvn_glass_t glass;
		float radii[4];
		memset(&glass, 0, sizeof(glass));
		glass.tint = RVN_THEME_COLOR(R_COLOR_TOOLTIP);
		glass.rim = rvn_rgba(1.0f, 1.0f, 1.0f, g_rvn.theme.dark ? 0.5f : 0.8f);
		glass.frost = 1.0f;
		glass.refraction = 8.0f;
		glass.saturation = 1.6f;
		glass.shadow = 12.0f;
		glass.shadow_color = RVN_THEME_COLOR(R_COLOR_SHADOW);
		radii[0] = radii[1] = radii[2] = radii[3] = rvn_minf(height * 0.5f, 12.0f);
		rvn_paint_glass(painter, rvn_rect(x, y, width, height), radii, &glass);
	} else {
		rvn_paint_shadow(painter, rvn_rect(x, y + 2.0f, width, height), 4.0f, 8.0f, RVN_THEME_COLOR(R_COLOR_SHADOW));
		rvn_paint_box(painter, rvn_rect(x, y, width, height), 4.0f, RVN_THEME_COLOR(R_COLOR_TOOLTIP),
			1.0f, RVN_THEME_COLOR(R_COLOR_POPUP_BORDER));
	}
	y += 5.0f;
	for (line = text;;) {
		const char* newline = strchr(line, '\n');
		int length = newline ? (int)(newline - line) : (int)strlen(line);
		rvn_paint_text(painter, R_INVALID_ID, font_size, x + 8.0f, y, line, length, RVN_THEME_COLOR(R_COLOR_TOOLTIP_TEXT));
		y += metrics.line_height;
		if (!newline) break;
		line = newline + 1;
	}
	/* pops out of the pointer */
	painter->opacity = opacity;
	rvn_drawlist_transform(painter->list, first_instance, first_command, 0.7f + 0.3f * shown, 0.7f + 0.3f * shown,
		(window->tooltip_x + 8.0f) * painter->scale, (window->tooltip_y + 12.0f) * painter->scale, 0.0f, 0.0f, painter->clip);
}

/* ---------------------------------------------------------------------------
 *  Hover and cursor
 * ------------------------------------------------------------------------- */

/* Collects a widget and its ancestors, up to and including the popup root. */
static int rvn_hover_path(rvn_widget_t* from, rvn_widget_t** path) {
	int count = 0;
	rvn_widget_t* walk;
	for (walk = from; walk && count < RVN_MAX_PATH_DEPTH; walk = walk->parent) {
		path[count++] = walk;
		if (walk->flags & RVN_WF_POPUP) break;
	}
	return count;
}

static void rvn_hover_clear_flags(rvn_widget_t* from) {
	rvn_widget_t* walk;
	for (walk = from; walk; walk = walk->parent) walk->flags &= ~RVN_WF_HOVERED;
}

static void rvn_window_set_hover(rvn_window_t* window, rvn_widget_t* target) {
	rvn_widget_t* old_path[RVN_MAX_PATH_DEPTH];
	rvn_widget_t* new_path[RVN_MAX_PATH_DEPTH];
	Rid left[RVN_MAX_PATH_DEPTH];
	Rid entered[RVN_MAX_PATH_DEPTH];
	int old_count, new_count, left_count = 0, entered_count = 0, i, j;
	REvent event;

	if (window->hover == target) return;

	old_count = window->hover ? rvn_hover_path(window->hover, old_path) : 0;
	new_count = target ? rvn_hover_path(target, new_path) : 0;
	for (i = 0; i < old_count; i++) {
		for (j = 0; j < new_count && new_path[j] != old_path[i]; j++) {}
		if (j == new_count) left[left_count++] = old_path[i]->id;
	}
	for (i = 0; i < new_count; i++) {
		for (j = 0; j < old_count && old_path[j] != new_path[i]; j++) {}
		if (j == old_count) entered[entered_count++] = new_path[i]->id;
	}

	rvn_hover_clear_flags(window->hover);
	for (i = 0; i < new_count; i++) new_path[i]->flags |= RVN_WF_HOVERED;
	window->hover = target;
	window->needs_paint = R_TRUE;

	/* notify through handles: a callback may delete widgets of either path */
	for (i = 0; i < left_count; i++) {
		rvn_widget_t* widget = rvn_widget_get(left[i]);
		if (!widget) continue;
		event = rvn_window_event(window, R_EVENT_MOUSE_LEAVE);
		rvn_widget_send(widget, &event);
	}
	for (i = entered_count; i > 0; i--) {
		rvn_widget_t* widget = rvn_widget_get(entered[i - 1]);
		if (!widget) continue;
		event = rvn_window_event(window, R_EVENT_MOUSE_ENTER);
		rvn_widget_send(widget, &event);
	}
	if (!window->dead) rvn_window_tooltip_arm(window);
}

static rvn_widget_t* rvn_window_hit(rvn_window_t* window, float x, float y, int* popup_index) {
	uint32_t i;
	for (i = window->popup_count; i > 0; i--) {
		rvn_widget_t* hit = rvn_widget_hit_test(window->popups[i - 1].root, x, y);
		/* a dialog keeps for itself everything that is under it, the veil around it included */
		if (hit || window->popups[i - 1].placement == RVN_POPUP_DIALOG) {
			if (popup_index) *popup_index = (int)i - 1;
			return hit;
		}
	}
	if (popup_index) *popup_index = -1;
	/* the island floats above the content */
	if (window->island) {
		rvn_widget_t* hit = rvn_widget_hit_test(window->island, x, y);
		if (hit) return hit;
	}
	return window->root ? rvn_widget_hit_test(window->root, x, y) : NULL;
}

static void rvn_window_update_cursor(rvn_window_t* window) {
	RCursor cursor = R_CURSOR_ARROW;
	rvn_widget_t* walk = window->capture ? window->capture : window->hover;
	switch (window->capture ? RVN_PREGION_CLIENT : window->edge) {
		case RVN_PREGION_LEFT: case RVN_PREGION_RIGHT: cursor = R_CURSOR_RESIZE_H; walk = NULL; break;
		case RVN_PREGION_TOP: case RVN_PREGION_BOTTOM: cursor = R_CURSOR_RESIZE_V; walk = NULL; break;
		case RVN_PREGION_TOP_LEFT: case RVN_PREGION_BOTTOM_RIGHT: cursor = R_CURSOR_RESIZE_NWSE; walk = NULL; break;
		case RVN_PREGION_TOP_RIGHT: case RVN_PREGION_BOTTOM_LEFT: cursor = R_CURSOR_RESIZE_NESW; walk = NULL; break;
		default: break;
	}
	for (; walk; walk = walk->parent) {
		if (walk->cursor != 0) {
			cursor = walk->cursor;
			break;
		}
		if (walk->flags & RVN_WF_POPUP) break;
	}
	if (cursor != window->cursor) {
		window->cursor = cursor;
		if (window->platform) g_rvn.platform->window_set_cursor(window->platform, cursor);
	}
}

/* Recomputes the hovered widget for a cursor that did not move (after a layout). */
static void rvn_window_refresh_hover(rvn_window_t* window) {
	if (window->capture || !window->mouse_inside) return;
	rvn_window_set_hover(window, rvn_window_hit(window, window->mouse_x, window->mouse_y, NULL));
	if (!window->dead) rvn_window_update_cursor(window);
}

/* ---------------------------------------------------------------------------
 *  Focus
 * ------------------------------------------------------------------------- */

void rvn_window_set_focus(rvn_window_t* window, rvn_widget_t* widget) {
	rvn_widget_t* old = window->focus;
	REvent event;
	if (old == widget) return;
	window->focus = widget;
	window->text_input_owner = R_INVALID_ID;
	window->needs_paint = R_TRUE;
	if (old) {
		old->flags &= ~RVN_WF_FOCUSED;
		event = rvn_window_event(window, R_EVENT_FOCUS_LOST);
		rvn_widget_send(old, &event);
	}
	/* the callback above may have changed the focus again */
	if (widget && window->focus == widget && !(widget->flags & RVN_WF_DEAD)) {
		widget->flags |= RVN_WF_FOCUSED;
		event = rvn_window_event(window, R_EVENT_FOCUS_GAINED);
		rvn_widget_send(widget, &event);
	}
}

typedef struct rvn_focus_list_t {
	rvn_widget_t** items;
	uint32_t count;
	uint32_t cap;
} rvn_focus_list_t;

static void rvn_focus_collect(rvn_widget_t* widget, rvn_focus_list_t* list) {
	uint32_t i;
	if (widget->visibility != R_VISIBLE || (widget->flags & RVN_WF_DISABLED)) return;
	if (widget->flags & RVN_WF_FOCUSABLE) {
		RVN_ARRAY_RESERVE(list->items, list->cap, list->count + 1);
		list->items[list->count++] = widget;
	}
	for (i = 0; i < widget->child_count; i++) {
		rvn_widget_t* child = widget->children[i];
		if (widget->cls->child_active && !widget->cls->child_active(widget, child)) continue;
		rvn_focus_collect(child, list);
	}
}

static void rvn_window_focus_next(rvn_window_t* window, int direction) {
	rvn_focus_list_t list;
	uint32_t i, index = 0;
	int found = R_FALSE;

	memset(&list, 0, sizeof(list));
	/* the Tab key stays inside a dialog */
	if (rvn_window_top_dialog(window)) rvn_focus_collect(rvn_window_top_dialog(window), &list);
	else if (window->root) rvn_focus_collect(window->root, &list);
	if (list.count == 0) {
		rvn_free(list.items);
		return;
	}
	for (i = 0; i < list.count; i++) {
		if (list.items[i] == window->focus) {
			index = i;
			found = R_TRUE;
			break;
		}
	}
	if (!found) index = direction > 0 ? 0 : list.count - 1;
	else index = (index + (direction > 0 ? 1u : list.count - 1u)) % list.count;
	window->focus_visible = R_TRUE;
	rvn_window_set_focus(window, list.items[index]);
	rvn_free(list.items);
}

/* ---------------------------------------------------------------------------
 *  Popups
 * ------------------------------------------------------------------------- */

int rvn_window_popup_index(const rvn_window_t* window, const rvn_widget_t* root) {
	uint32_t i;
	for (i = 0; i < window->popup_count; i++) {
		if (window->popups[i].root == root) return (int)i;
	}
	return -1;
}

void rvn_window_open_popup(rvn_window_t* window, rvn_widget_t* root, rvn_widget_t* owner,
	rvn_rect_t anchor, rvn_popup_placement_t placement, float min_width) {

	rvn_popup_t* popup;
	if (rvn_window_popup_index(window, root) >= 0) return;
	RVN_ARRAY_RESERVE(window->popups, window->popup_cap, window->popup_count + 1);
	popup = &window->popups[window->popup_count++];
	memset(popup, 0, sizeof(*popup));
	popup->root = root;
	popup->owner = owner;
	if (owner) {
		/* the owner gave the place where it believes it is: inside a canvas that zooms, it shows somewhere else */
		float x0 = anchor.x, y0 = anchor.y, x1 = anchor.x + anchor.w, y1 = anchor.y + anchor.h;
		rvn_widget_from_virtual(owner, &x0, &y0);
		rvn_widget_from_virtual(owner, &x1, &y1);
		anchor = rvn_rect(x0, y0, x1 - x0, y1 - y0);
	}
	popup->anchor = anchor;
	popup->placement = placement;
	popup->min_width = min_width;
	/* opened again while it was still closing, here or in another window */
	if (root->window) rvn_window_forget_ghost(root->window, root);
	root->flags |= RVN_WF_POPUP | RVN_WF_LAYOUT_DIRTY;
	if (root->window != window) rvn_widget_set_window(root, window);
	/* a menu or a list opened from a part of the window that has its own theme wears it too */
	if (owner) rvn_widget_inherit_theme(root, owner->theme);
	rvn_window_tooltip_hide(window);
	window->needs_layout = R_TRUE;
	window->needs_paint = R_TRUE;
}

rvn_widget_t* rvn_window_top_dialog(const rvn_window_t* window) {
	uint32_t i;
	for (i = window->popup_count; i > 0; i--) {
		if (window->popups[i - 1].placement == RVN_POPUP_DIALOG) return window->popups[i - 1].root;
	}
	return NULL;
}

void rvn_window_open_dialog(rvn_window_t* window, rvn_widget_t* root, RDialogClosedCallback closed, void* user) {
	rvn_popup_t* popup;
	rvn_widget_t* focus = window->focus;
	if (rvn_window_popup_index(window, root) >= 0) return;
	/* menus and lists do not stay open under a dialog */
	rvn_window_close_popups(window);
	if (root->parent || root->window) rvn_widget_detach(root);
	window->capture = NULL;
	rvn_window_set_focus(window, NULL);
	if (window->dead || (root->flags & RVN_WF_DEAD)) return;
	/* the widget that had the keyboard gets it back when the dialog closes */
	rvn_window_open_popup(window, root, NULL, rvn_rect(0.0f, 0.0f, 0.0f, 0.0f), RVN_POPUP_DIALOG, 0.0f);
	popup = &window->popups[window->popup_count - 1];
	popup->focus_back = (focus && !(focus->flags & RVN_WF_DEAD)) ? focus->id : R_INVALID_ID;
	popup->closed = closed;
	popup->closed_user = user;
	rvn_window_refresh_hover(window);
}

int rvn_window_close_dialog(rvn_widget_t* inside, int result) {
	rvn_window_t* window = inside->window;
	uint32_t i;
	if (!window) return R_FALSE;
	for (i = window->popup_count; i > 0; i--) {
		rvn_popup_t* popup = &window->popups[i - 1];
		if (popup->placement != RVN_POPUP_DIALOG || !rvn_widget_is_inside(inside, popup->root)) continue;
		popup->result = result;
		rvn_window_close_popup(window, popup->root);
		return R_TRUE;
	}
	return R_FALSE;
}

void rvn_window_close_popup(rvn_window_t* window, rvn_widget_t* root) {
	int index = rvn_window_popup_index(window, root);
	if (index < 0) return;
	rvn_dispatch_begin();
	while ((int)window->popup_count > index) {
		rvn_popup_t popup = window->popups[--window->popup_count];
		rvn_widget_t* closed = popup.root;
		int dialog = popup.placement == RVN_POPUP_DIALOG;
		int ghost_shown = R_FALSE;
		/* the popup closes because its own widgets are being removed: nothing of it may stay behind */
		int leaving = window->forgetting && rvn_widget_is_inside(closed, window->forgetting);
		if (window->hover && rvn_widget_is_inside(window->hover, closed)) {
			rvn_hover_clear_flags(window->hover);
			window->hover = NULL;
		}
		if (window->capture && rvn_widget_is_inside(window->capture, closed)) window->capture = NULL;
		if (window->focus && rvn_widget_is_inside(window->focus, closed)) {
			window->focus->flags &= ~RVN_WF_FOCUSED;
			window->focus = NULL;
		}
		closed->flags &= ~RVN_WF_POPUP;
		if (!leaving && !g_rvn.animations_disabled && !window->dead && window->visible && !(closed->flags & RVN_WF_DEAD)) {
			/* it stays in the window, without input, for the time it takes to shrink away */
			rvn_ghost_t* ghost;
			RVN_ARRAY_RESERVE(window->ghosts, window->ghost_cap, window->ghost_count + 1);
			ghost = &window->ghosts[window->ghost_count++];
			ghost->root = closed;
			ghost->anchor = popup.anchor;
			ghost->shown = popup.shown;
			ghost->shown.started = R_TRUE;
			ghost->dialog = dialog;
			ghost_shown = R_TRUE;
		} else if (!closed->parent && (!dialog || leaving)) {
			/* a popup that is not part of the tree leaves the window */
			rvn_widget_set_window(closed, NULL);
		}
		if (closed->cls->popup_closed) closed->cls->popup_closed(closed);
		if (dialog) {
			Rid owner = popup.focus_back;
			Rid closed_id = closed->id;
			if (popup.closed) popup.closed(closed_id, popup.result, popup.closed_user);
			/* (the callback may have deleted anything) */
			if (!ghost_shown && !leaving && rvn_widget_get(closed_id)) rvn_widget_destroy(closed);
			if (!window->dead && rvn_widget_get(owner) && !window->focus && rvn_window_top_dialog(window) == NULL) {
				rvn_widget_focus(rvn_widget_get(owner));
			}
			if (window->dead) break;
		}
	}
	if (!window->dead) {
		window->needs_layout = R_TRUE;
		window->needs_paint = R_TRUE;
	}
	rvn_dispatch_end();
}

void rvn_window_close_popups(rvn_window_t* window) {
	uint32_t i;
	/* what is above the dialog on top; everything when there is no dialog */
	for (i = window->popup_count; i > 0; i--) {
		if (window->popups[i - 1].placement == RVN_POPUP_DIALOG) break;
	}
	if (i < window->popup_count) rvn_window_close_popup(window, window->popups[i].root);
}

static void rvn_window_close_all_popups(rvn_window_t* window) {
	if (window->popup_count > 0) rvn_window_close_popup(window, window->popups[0].root);
}

/** Space kept between a dialog and the edges of its window, and between its frame and its content. */
#define RVN_DIALOG_MARGIN 24.0f
#define RVN_DIALOG_PADDING 18.0f

static void rvn_window_layout_popup(rvn_window_t* window, rvn_popup_t* popup) {
	rvn_vec2_t area = rvn_window_size(window);
	rvn_vec2_t desired = rvn_widget_measure(popup->root);
	float width = rvn_maxf(desired.x, popup->min_width);
	float height = desired.y;
	float x = popup->anchor.x, y = popup->anchor.y;

	if (width > area.x) width = area.x;
	if (height > area.y) height = area.y;

	switch (popup->placement) {
		case RVN_POPUP_DIALOG: {
			float room = RVN_DIALOG_MARGIN + RVN_DIALOG_PADDING;
			width = rvn_minf(desired.x, rvn_maxf(area.x - room * 2.0f, 40.0f));
			height = rvn_minf(desired.y, rvn_maxf(area.y - room * 2.0f, 40.0f));
			rvn_widget_arrange(popup->root, floorf((area.x - width) * 0.5f), floorf((area.y - height) * 0.5f), width, height);
			return;
		}
		case RVN_POPUP_BELOW:
			y = popup->anchor.y + popup->anchor.h;
			if (y + height > area.y && popup->anchor.y - height >= 0.0f) y = popup->anchor.y - height;
			break;
		case RVN_POPUP_RIGHT:
			x = popup->anchor.x + popup->anchor.w;
			if (x + width > area.x && popup->anchor.x - width >= 0.0f) x = popup->anchor.x - width;
			break;
		case RVN_POPUP_AT:
			break;
	}
	x = rvn_clampf(x, 0.0f, rvn_maxf(0.0f, area.x - width));
	y = rvn_clampf(y, 0.0f, rvn_maxf(0.0f, area.y - height));
	rvn_widget_arrange(popup->root, floorf(x), floorf(y), width, height);
}

/* ---------------------------------------------------------------------------
 *  References to widgets
 * ------------------------------------------------------------------------- */

void rvn_window_forget_widget(rvn_window_t* window, rvn_widget_t* widget) {
	uint32_t i;
	if (window->capture && rvn_widget_is_inside(window->capture, widget)) window->capture = NULL;
	if (window->focus && rvn_widget_is_inside(window->focus, widget)) {
		window->focus->flags &= ~RVN_WF_FOCUSED;
		window->focus = NULL;
	}
	if (window->hover && rvn_widget_is_inside(window->hover, widget)) {
		rvn_hover_clear_flags(window->hover);
		window->hover = NULL;
	}
	if (window->tooltip_widget && rvn_widget_is_inside(window->tooltip_widget, widget)) {
		rvn_window_tooltip_hide(window);
	}
	if (window->drop_target && rvn_widget_is_inside(window->drop_target, widget)) {
		window->drop_target = NULL;
		window->drop_accepted = R_FALSE;
	}
	/* a spring of the layout that stops being stepped must not stay half-way */
	for (i = 0; i < window->layout_spring_count;) {
		rvn_layout_spring_t* entry = &window->layout_springs[i];
		if (!rvn_widget_is_inside(entry->widget, widget)) {
			i++;
			continue;
		}
		rvn_spring_set(entry->spring, entry->target);
		window->layout_springs[i] = window->layout_springs[--window->layout_spring_count];
	}
	/* close the popups shown from this subtree, lowest first (it closes the ones above) */
	for (i = 0; i < window->popup_count; i++) {
		rvn_popup_t* popup = &window->popups[i];
		if (rvn_widget_is_inside(popup->root, widget) || (popup->owner && rvn_widget_is_inside(popup->owner, widget))) {
			rvn_widget_t* previous = window->forgetting;
			window->forgetting = widget;
			rvn_window_close_popup(window, popup->root);
			window->forgetting = previous;
			break;
		}
	}
	for (i = 0; i < window->ghost_count;) {
		rvn_widget_t* root = window->ghosts[i].root;
		if (!rvn_widget_is_inside(root, widget)) {
			i++;
			continue;
		}
		window->ghosts[i] = window->ghosts[--window->ghost_count];
		/* (a closed popup that is not in the tree only stayed in the window to be drawn) */
		if (!root->parent && !(root->flags & RVN_WF_POPUP)) rvn_widget_set_window(root, NULL);
	}
	window->needs_paint = R_TRUE;
}

void rvn_window_release_natives(rvn_window_t* window, rvn_widget_t* subtree) {
	uint32_t i;
	for (i = window->native_viewport_count; i > 0; i--) {
		rvn_widget_t* viewport = window->native_viewports[i - 1];
		if (rvn_widget_is_inside(viewport, subtree)) rvn_viewport_native_release(viewport, window);
	}
}

/* ---------------------------------------------------------------------------
 *  Layout
 * ------------------------------------------------------------------------- */

void rvn_window_layout(rvn_window_t* window) {
	double start = rvn_clock_seconds();
	rvn_vec2_t size = rvn_window_size(window);
	int pass;
	uint32_t i;

	/* a second pass is needed when the arrangement changes a desired size (wrapped text) */
	for (pass = 0; pass < 4 && window->needs_layout; pass++) {
		window->needs_layout = R_FALSE;
		if (window->root) {
			rvn_widget_measure(window->root);
			/* the content obeys its slot like any child: padding and alignment in the window */
			rvn_widget_arrange_slot(window->root, rvn_rect(0.0f, 0.0f, size.x, size.y));
		}
		if (window->island) {
			/* at the top, in the middle, at its own size */
			rvn_vec2_t desired = rvn_widget_measure(window->island);
			float width = rvn_minf(desired.x, size.x), height = rvn_minf(desired.y, size.y);
			rvn_widget_arrange(window->island, floorf((size.x - width) * 0.5f), window->island->slot.padding.t, width, height);
		}
		for (i = 0; i < window->popup_count; i++) rvn_window_layout_popup(window, &window->popups[i]);
	}
	window->needs_layout = R_FALSE;
	window->needs_paint = R_TRUE;
	window->stats.layout_ms = (rvn_clock_seconds() - start) * 1000.0;
	rvn_window_refresh_hover(window);
}

/* ---------------------------------------------------------------------------
 *  Paint
 * ------------------------------------------------------------------------- */

int rvn_window_see_through(const rvn_window_t* window) {
	return window->transparent && window->backdrop != (RBackdrop)0 && window->backdrop != R_BACKDROP_NONE;
}

/* The image behind the content: the one of the window, else the wallpaper of a glass theme. */
static void rvn_window_paint_background(rvn_window_t* window, RPainter* painter) {
	Rid texture = rvn_texture_get(window->background) ? window->background : R_INVALID_ID;
	rvn_texture_t* object;
	rvn_vec2_t size = rvn_window_size(window);
	float scale, visible_u, visible_v, uv[4];

	/* a window with its own clear color keeps it; one that is seen through has the screen for a wallpaper */
	if (texture == R_INVALID_ID && !window->has_clear_color && !rvn_window_see_through(window)) texture = g_rvn.theme.wallpaper;
	object = rvn_texture_get(texture);
	if (!object || !object->created || object->width <= 0 || object->height <= 0 || size.x <= 0.0f || size.y <= 0.0f) return;

	/* covers the window whatever its proportions: the centered part of the image with the same ratio */
	scale = rvn_maxf(size.x / (float)object->width, size.y / (float)object->height);
	visible_u = size.x / ((float)object->width * scale);
	visible_v = size.y / ((float)object->height * scale);
	uv[0] = 0.5f - visible_u * 0.5f;
	uv[1] = 0.5f - visible_v * 0.5f;
	uv[2] = 0.5f + visible_u * 0.5f;
	uv[3] = 0.5f + visible_v * 0.5f;
	rvn_paint_image(painter, rvn_rect(0.0f, 0.0f, size.x, size.y), texture, uv, rvn_rgba(1.0f, 1.0f, 1.0f, 1.0f), 0.0f);
}

/* Detaches the root of a ghost that is not shown any more. */
static void rvn_window_remove_ghost(rvn_window_t* window, uint32_t index) {
	rvn_widget_t* root = window->ghosts[index].root;
	int dialog = window->ghosts[index].dialog;
	window->ghosts[index] = window->ghosts[--window->ghost_count];
	if (dialog) {
		/* the window owns its dialogs */
		if (!(root->flags & RVN_WF_POPUP)) rvn_widget_destroy(root);
		return;
	}
	/* a popup that is not part of the tree leaves the window, unless it was opened again meanwhile */
	if (!root->parent && !(root->flags & RVN_WF_POPUP) && root->window == window) rvn_widget_set_window(root, NULL);
}

static void rvn_window_forget_ghost(rvn_window_t* window, const rvn_widget_t* root) {
	uint32_t i;
	for (i = 0; i < window->ghost_count; i++) {
		if (window->ghosts[i].root == root) {
			window->ghosts[i] = window->ghosts[--window->ghost_count];
			return;
		}
	}
}

/*
 * A popup grows out of what opened it: it is painted in place, then scaled
 * around the point of its frame that is nearest to its anchor.
 */
static void rvn_window_paint_popup(RPainter* painter, rvn_widget_t* root, rvn_rect_t anchor, float shown) {
	uint32_t first_instance = painter->list->instance_count, first_command = painter->list->command_count;
	float opacity = painter->opacity;
	float scale, pivot_x, pivot_y;

	if (shown > 0.999f && shown < 1.001f) {
		rvn_widget_paint(root, painter);
		return;
	}
	painter->opacity *= rvn_clampf(shown * 1.8f, 0.0f, 1.0f);
	rvn_widget_paint(root, painter);
	painter->opacity = opacity;

	scale = 0.72f + 0.28f * shown;
	pivot_x = rvn_clampf(anchor.x + anchor.w * 0.5f, root->rect.x, root->rect.x + root->rect.w);
	pivot_y = rvn_clampf(anchor.y + anchor.h * 0.5f, root->rect.y, root->rect.y + root->rect.h);
	rvn_drawlist_transform(painter->list, first_instance, first_command, scale, scale,
		pivot_x * painter->scale, pivot_y * painter->scale, 0.0f, 0.0f, painter->clip);
}

/*
 * A dialog: a veil falls over the window, and the sheet rises from below
 * and settles with a bounce.
 */
static void rvn_window_paint_dialog(rvn_window_t* window, RPainter* painter, rvn_widget_t* root, float shown) {
	rvn_vec2_t size = rvn_window_size(window);
	rvn_rect_t frame = rvn_rect(root->rect.x - RVN_DIALOG_PADDING, root->rect.y - RVN_DIALOG_PADDING,
		root->rect.w + RVN_DIALOG_PADDING * 2.0f, root->rect.h + RVN_DIALOG_PADDING * 2.0f);
	float veil = rvn_clampf(shown, 0.0f, 1.0f), opacity = painter->opacity;
	uint32_t first_instance, first_command;

	if (g_rvn.theme.glass) {
		/* the window goes out of focus behind the sheet */
		rvn_glass_t glass;
		float radii[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		memset(&glass, 0, sizeof(glass));
		glass.tint = rvn_rgba(0.0f, 0.0f, 0.0f, g_rvn.theme.dark ? 0.34f : 0.16f);
		glass.frost = 1.0f;
		glass.saturation = 1.0f;
		painter->opacity *= veil;
		rvn_paint_glass(painter, rvn_rect(0.0f, 0.0f, size.x, size.y), radii, &glass);
		painter->opacity = opacity;
	} else {
		rvn_paint_rect(painter, rvn_rect(0.0f, 0.0f, size.x, size.y), rvn_rgba(0.0f, 0.0f, 0.0f, 0.42f * veil));
	}

	first_instance = painter->list->instance_count;
	first_command = painter->list->command_count;
	painter->opacity *= rvn_clampf(shown * 2.2f, 0.0f, 1.0f);
	if (g_rvn.theme.glass) {
		rvn_paint_surface(painter, root, frame, 26.0f, RVN_SURFACE_POPUP, 0.0f, 0.0f);
	} else if (g_rvn.theme.liquid) {
		/* macOS 26: sheets are glass too, with the corners of its windows */
		rvn_paint_surface(painter, root, frame, 16.0f, RVN_SURFACE_POPUP, 0.0f, 0.0f);
	} else {
		rvn_paint_shadow(painter, rvn_rect(frame.x, frame.y + 6.0f, frame.w, frame.h), 10.0f, 28.0f, RVN_THEME_COLOR(R_COLOR_SHADOW));
		rvn_paint_box(painter, frame, 10.0f, RVN_THEME_COLOR(R_COLOR_POPUP), 1.0f, RVN_THEME_COLOR(R_COLOR_POPUP_BORDER));
	}
	rvn_widget_paint(root, painter);
	painter->opacity = opacity;
	if (shown < 0.999f || shown > 1.001f) {
		float scale = 0.9f + 0.1f * shown;
		rvn_drawlist_transform(painter->list, first_instance, first_command, scale, scale,
			(frame.x + frame.w * 0.5f) * painter->scale, (frame.y + frame.h) * painter->scale,
			0.0f, (1.0f - shown) * 60.0f * painter->scale, painter->clip);
	}
}

/* What would take the drop of a drag coming from another application lights up. */
static void rvn_window_paint_drop(rvn_window_t* window, RPainter* painter) {
	rvn_color_t accent = RVN_THEME_COLOR(R_COLOR_ACCENT);
	rvn_rect_t rect = window->drop_rect;
	float shown, opacity, grow;

	if (!window->drop_accepted && !window->drop_shown.started) return;
	shown = rvn_paint_spring_from(painter, &window->drop_shown, window->drop_accepted ? 1.0f : 0.0f,
		window->drop_accepted ? R_MOTION_BOUNCY : R_MOTION_SNAPPY);
	if (shown <= 0.01f) return;
	/* the frame lands on its target from a little further out */
	opacity = rvn_clampf(shown, 0.0f, 1.0f);
	grow = (1.0f - shown) * 14.0f;
	rect = rvn_rect(rect.x - grow, rect.y - grow, rect.w + grow * 2.0f, rect.h + grow * 2.0f);
	rvn_paint_shadow(painter, rect, window->drop_radius, 18.0f, rvn_color_fade(accent, 0.35f * opacity));
	rvn_paint_box(painter, rect, window->drop_radius, rvn_color_fade(accent, 0.16f * opacity),
		2.0f, rvn_color_fade(accent, 0.95f * opacity));
}

static int rvn_window_can_paint(const rvn_window_t* window) {
	return window->surface_ready && window->width_px > 0 && window->height_px > 0;
}

static void rvn_window_paint(rvn_window_t* window, RCapture* capture) {
	rvn_frame_t* frame;
	RPainter painter;
	double start;
	uint32_t i, widgets = 0, backdrops;
	rvn_color_t clear;

	window->needs_paint = R_FALSE;
	window->text_input_owner = R_INVALID_ID;
	window->text_input_rect = rvn_box(0.0f, 0.0f, 0.0f, 0.0f);
	if (!rvn_window_can_paint(window) || (!window->visible && !capture)) return;

	/* never queue more than a couple of frames: this is what paces a busy UI thread */
	rvn_renderer_throttle(&window->frames_in_flight, RVN_MAX_FRAMES_IN_FLIGHT);

	frame = rvn_renderer_acquire_frame();
	start = rvn_clock_seconds();

	memset(&painter, 0, sizeof(painter));
	painter.list = &frame->list;
	painter.window = window;
	painter.scale = window->scale;
	painter.clip = rvn_box(0.0f, 0.0f, (float)window->width_px, (float)window->height_px);
	painter.opacity = 1.0f;
	painter.text_color = RVN_THEME_COLOR(R_COLOR_TEXT);
	painter.enabled = R_TRUE;
	/* animations advance by the time since the last frame */
	painter.dt = rvn_animation_dt(start - window->paint_time);
	window->paint_time = start;

	rvn_atomic_store(&window->realtime_viewports, 0);
	rvn_atomic_store(&window->animating, 0);
	rvn_window_paint_background(window, &painter);
	if (window->root) {
		rvn_widget_paint(window->root, &painter);
		widgets += window->root->subtree_count;
	}
	if (window->island) {
		rvn_widget_paint(window->island, &painter);
		widgets += window->island->subtree_count;
	}
	rvn_window_paint_drop(window, &painter);
	rvn_window_paint_drag(window, &painter);
	/* popups that were closed shrink away under the ones that are open */
	for (i = 0; i < window->ghost_count;) {
		rvn_ghost_t* ghost = &window->ghosts[i];
		float shown = rvn_paint_spring_from(&painter, &ghost->shown, 0.0f, R_MOTION_SNAPPY);
		if (shown <= 0.02f) {
			rvn_window_remove_ghost(window, i);
			continue;
		}
		if (ghost->dialog) rvn_window_paint_dialog(window, &painter, ghost->root, shown);
		else rvn_window_paint_popup(&painter, ghost->root, ghost->anchor, shown);
		i++;
	}
	for (i = 0; i < window->popup_count; i++) {
		rvn_popup_t* popup = &window->popups[i];
		float shown = rvn_paint_spring_from(&painter, &popup->shown, 1.0f, R_MOTION_BOUNCY);
		if (popup->placement == RVN_POPUP_DIALOG) rvn_window_paint_dialog(window, &painter, popup->root, shown);
		else rvn_window_paint_popup(&painter, popup->root, popup->anchor, shown);
		widgets += popup->root->subtree_count;
	}
	rvn_window_paint_tooltip(window, &painter);
	/* color filters for the color blind (core/accessibility.c) */
	rvn_a11y_filter_instances(frame->list.instances, frame->list.instance_count);
	/* what the screen reader must be told after this frame */
	rvn_a11y_frame(window);

	/* new glyphs reach the GPU before the frame that uses them */
	rvn_fonts_flush();
	rvn_fonts_resolve_pages(&frame->list);
	/* glass: only keep the backdrop captures that show something new */
	backdrops = rvn_drawlist_resolve_backdrops(&frame->list,
		rvn_box(0.0f, 0.0f, (float)window->width_px, (float)window->height_px), RVN_MAX_BACKDROPS);

	window->stats.paint_ms = (rvn_clock_seconds() - start) * 1000.0;
	window->stats.render_ms = (double)rvn_atomic_load(&window->render_us) / 1000.0;
	window->stats.widget_count = widgets;
	window->stats.instance_count = frame->list.instance_count;
	window->stats.draw_call_count = frame->list.command_count - backdrops;
	window->stats.backdrop_count = backdrops;
	window->stats.paint_job_count = (Ruint)rvn_paint_take_job_count();
	window->stats.frame_index = window->frame_index;

	clear = RVN_THEME_COLOR(R_COLOR_WINDOW);
	frame->data.window = window->id;
	frame->data.framebuffer_width = (Ruint)window->width_px;
	frame->data.framebuffer_height = (Ruint)window->height_px;
	frame->data.vsync = g_rvn.vsync;
	if (rvn_window_see_through(window)) {
		/* only what the widgets draw hides the screen; a clear color of the window, if any, veils it (premultiplied) */
		float alpha = window->has_clear_color ? window->clear_color[3] : 0.0f;
		frame->data.clear_color[0] = window->has_clear_color ? window->clear_color[0] * alpha : 0.0f;
		frame->data.clear_color[1] = window->has_clear_color ? window->clear_color[1] * alpha : 0.0f;
		frame->data.clear_color[2] = window->has_clear_color ? window->clear_color[2] * alpha : 0.0f;
		frame->data.clear_color[3] = alpha;
	} else if (window->has_clear_color) {
		memcpy(frame->data.clear_color, window->clear_color, sizeof(window->clear_color));
	} else {
		rvn_color_to_floats(clear, &frame->data.clear_color[0], &frame->data.clear_color[1],
			&frame->data.clear_color[2], &frame->data.clear_color[3]);
	}
	frame->data.frame_index = window->frame_index++;
	frame->data.scale = window->scale;
	frame->data.pointer[0] = window->mouse_inside ? window->mouse_x * window->scale : -1e6f;
	frame->data.pointer[1] = window->mouse_inside ? window->mouse_y * window->scale : -1e6f;
	frame->capture = capture;
	frame->frames_in_flight = &window->frames_in_flight;
	frame->render_us = &window->render_us;
	rvn_renderer_submit_frame(frame);

	/* native child windows follow the viewports that were just painted, the others hide */
	for (i = 0; i < window->native_viewport_count; i++) {
		rvn_viewport_native_sync(window->native_viewports[i], window->frame_index - 1);
	}
}

int rvn_window_wants_frame(rvn_window_t* window) {
	if (window->dead) return R_FALSE;
	if (window->needs_layout || window->layout_spring_count > 0) return R_TRUE;
	if (!rvn_window_can_paint(window) || !window->visible) return R_FALSE;
	return window->needs_paint || window->continuous || rvn_atomic_load(&window->animating) != 0 ||
		rvn_atomic_load(&window->realtime_viewports) > 0 || rvn_atomic_load(&window->redraw_requested) != 0;
}

void rvn_widget_animate_layout(rvn_widget_t* widget, RSpring* spring, float target, RMotion motion) {
	rvn_window_t* window = widget->window;
	rvn_layout_spring_t* entry = NULL;
	uint32_t i;

	if (!window || window->dead || g_rvn.animations_disabled || !spring->started) {
		rvn_spring_set(spring, target);
		rvn_widget_invalidate_layout(widget);
		return;
	}
	for (i = 0; i < window->layout_spring_count && !entry; i++) {
		if (window->layout_springs[i].spring == spring) entry = &window->layout_springs[i];
	}
	if (!entry) {
		RVN_ARRAY_RESERVE(window->layout_springs, window->layout_spring_cap, window->layout_spring_count + 1);
		entry = &window->layout_springs[window->layout_spring_count++];
	}
	entry->widget = widget;
	entry->spring = spring;
	entry->target = target;
	entry->motion = motion;
	window->needs_layout = R_TRUE;
}

void rvn_widget_forget_layout_spring(rvn_widget_t* widget, const RSpring* spring) {
	rvn_window_t* window = widget->window;
	uint32_t i;
	if (!window) return;
	for (i = 0; i < window->layout_spring_count; i++) {
		if (window->layout_springs[i].spring != spring) continue;
		window->layout_springs[i] = window->layout_springs[--window->layout_spring_count];
		return;
	}
}

/* Advances the springs that drive the layout; R_TRUE while some are moving. */
static int rvn_window_step_layout_springs(rvn_window_t* window) {
	RPainter clock;
	uint32_t i;
	if (window->layout_spring_count == 0) return R_FALSE;
	/* (only the time of a painter is used: the springs are stepped as they would be while painting) */
	memset(&clock, 0, sizeof(clock));
	clock.dt = rvn_animation_dt(rvn_clock_seconds() - window->paint_time);
	for (i = 0; i < window->layout_spring_count;) {
		rvn_layout_spring_t* entry = &window->layout_springs[i];
		rvn_paint_spring_from(&clock, entry->spring, entry->target, entry->motion);
		rvn_widget_invalidate_layout(entry->widget);
		if (!rvn_spring_moving(entry->spring, entry->target)) {
			window->layout_springs[i] = window->layout_springs[--window->layout_spring_count];
		} else {
			i++;
		}
	}
	return window->layout_spring_count > 0;
}

void rvn_window_update(rvn_window_t* window) {
	if (window->dead) return;
	if (rvn_atomic_exchange(&window->redraw_requested, 0)) window->needs_paint = R_TRUE;
	rvn_window_step_layout_springs(window);
	if (window->continuous || rvn_atomic_load(&window->realtime_viewports) > 0 || rvn_atomic_load(&window->animating) != 0) {
		window->needs_paint = R_TRUE;
	}
	if (window->needs_layout) rvn_window_layout(window);
	if (window->needs_paint) rvn_window_paint(window, NULL);
}

void rvn_invalidate_all_windows(void) {
	uint32_t i;
	for (i = 0; i < g_rvn.widgets.used; i++) {
		rvn_widget_t* widget = (rvn_widget_t*)rvn_pool_at(&g_rvn.widgets, i);
		if (widget) widget->flags |= RVN_WF_LAYOUT_DIRTY;
	}
	for (i = 0; i < g_rvn.windows.used; i++) {
		rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (window && !window->dead) {
			window->needs_layout = R_TRUE;
			window->needs_paint = R_TRUE;
		}
	}
}

void rvn_windows_theme_changed(void) {
	uint32_t i;
	/* the materials the OS puts behind a window are light or dark with the theme */
	if (!g_rvn.platform || !g_rvn.platform->window_set_backdrop) return;
	for (i = 0; i < g_rvn.windows.used; i++) {
		rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (!window || window->dead || !window->transparent || !window->backdrop_wanted) continue;
		if (window->backdrop_dark == g_rvn.theme.dark) continue;
		window->backdrop_dark = g_rvn.theme.dark;
		window->backdrop = (RBackdrop)g_rvn.platform->window_set_backdrop(window->platform, (int)window->backdrop_wanted, g_rvn.theme.dark);
	}
}

void rvn_repaint_all_windows(void) {
	uint32_t i;
	for (i = 0; i < g_rvn.windows.used; i++) {
		rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (window && !window->dead) window->needs_paint = R_TRUE;
	}
}

/* ---------------------------------------------------------------------------
 *  Custom header (R_WINDOW_CUSTOM_HEADER)
 * ------------------------------------------------------------------------- */

/** Width of the band along the edges that resizes the window, and length of its corners. */
#define RVN_RESIZE_GRIP 6.0f
#define RVN_RESIZE_CORNER 18.0f

/* What a point of the window is for the window system: an edge, the title bar, or content. */
static rvn_pregion_t rvn_window_region(rvn_window_t* window, float x, float y) {
	rvn_vec2_t size = rvn_window_size(window);
	rvn_widget_t* walk;
	int popup_index = -1;

	if (!(window->window_flags & R_WINDOW_CUSTOM_HEADER)) return RVN_PREGION_CLIENT;
	if (!(window->window_flags & R_WINDOW_NO_RESIZE) && window->state != RVN_PWINDOW_MAXIMIZED) {
		int left = x < RVN_RESIZE_GRIP, right = x >= size.x - RVN_RESIZE_GRIP;
		int top = y < RVN_RESIZE_GRIP, bottom = y >= size.y - RVN_RESIZE_GRIP;
		if (left || right || top || bottom) {
			int near_left = x < RVN_RESIZE_CORNER, near_right = x >= size.x - RVN_RESIZE_CORNER;
			int near_top = y < RVN_RESIZE_CORNER, near_bottom = y >= size.y - RVN_RESIZE_CORNER;
			if (near_top && near_left) return RVN_PREGION_TOP_LEFT;
			if (near_top && near_right) return RVN_PREGION_TOP_RIGHT;
			if (near_bottom && near_left) return RVN_PREGION_BOTTOM_LEFT;
			if (near_bottom && near_right) return RVN_PREGION_BOTTOM_RIGHT;
			if (left) return RVN_PREGION_LEFT;
			if (right) return RVN_PREGION_RIGHT;
			return top ? RVN_PREGION_TOP : RVN_PREGION_BOTTOM;
		}
	}
	if (window->capture) return RVN_PREGION_CLIENT;
	walk = rvn_window_hit(window, x, y, &popup_index);
	if (popup_index >= 0) {
		/* the title bar still moves the window through the veil of a dialog */
		if (walk || !window->root || window->popups[popup_index].placement != RVN_POPUP_DIALOG) return RVN_PREGION_CLIENT;
		walk = rvn_widget_hit_test(window->root, x, y);
	}
	for (; walk; walk = walk->parent) {
		if (walk->flags & RVN_WF_WINDOW_DRAG) {
			rvn_vec2_t origin = rvn_widget_origin(walk);
			if (walk->cls->window_drag_at && !walk->cls->window_drag_at(walk, x - origin.x, y - origin.y)) {
				return RVN_PREGION_CLIENT;
			}
			return RVN_PREGION_CAPTION;
		}
		/* a control inside the title bar keeps the mouse; what only shows something lets it through */
		if (walk->cls->event || walk->event_callback || (walk->flags & RVN_WF_FOCUSABLE)) break;
	}
	return RVN_PREGION_CLIENT;
}

rvn_pregion_t rvn_window_region_at(void* user, int x, int y) {
	rvn_window_t* window = (rvn_window_t*)user;
	float scale;
	if (!window || window->dead) return RVN_PREGION_CLIENT;
	scale = window->scale > 0.0f ? window->scale : 1.0f;
	return rvn_window_region(window, (float)x / scale, (float)y / scale);
}

void rvn_window_toggle_maximized(rvn_window_t* window) {
	if (window->window_flags & R_WINDOW_NO_RESIZE) return;
	g_rvn.platform->window_set_state(window->platform,
		window->state == RVN_PWINDOW_MAXIMIZED ? RVN_PWINDOW_NORMAL : RVN_PWINDOW_MAXIMIZED);
}

void rvn_window_request_close(rvn_window_t* window) {
	int close;
	rvn_dispatch_begin();
	close = !window->close_callback || window->close_callback(window->id, window->close_user);
	if (close && !window->dead) rvn_window_destroy(window);
	rvn_dispatch_end();
}

/* The window system took the pointer (it moves or resizes the window): nothing is pressed any more. */
static void rvn_window_lose_pointer(rvn_window_t* window) {
	window->capture = NULL;
	window->buttons = 0;
	window->click_count = 0;
	window->edge = RVN_PREGION_CLIENT;
	rvn_window_set_hover(window, NULL);
}

/*
 * Window systems that do not ask for the regions (X11): a press of the left
 * button on the title bar or on an edge starts moving or resizing the window.
 * Returns R_TRUE when the press was used.
 */
static int rvn_window_press_region(rvn_window_t* window, float x, float y) {
	rvn_pregion_t region;
	int clicks = window->click_count;
	if (g_rvn.platform->native_regions || window->capture) return R_FALSE;
	region = rvn_window_region(window, x, y);
	if (region == RVN_PREGION_CLIENT) return R_FALSE;
	rvn_window_lose_pointer(window);
	if (window->dead) return R_TRUE;
	if (region != RVN_PREGION_CAPTION) {
		g_rvn.platform->window_begin_resize(window->platform, region);
	} else if (clicks == 2) {
		rvn_window_toggle_maximized(window);
	} else {
		/* (the count is kept so that the next press can be the second click) */
		window->click_count = clicks;
		g_rvn.platform->window_begin_move(window->platform);
	}
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Drag and drop from other applications
 * ------------------------------------------------------------------------- */

static int rvn_window_drop_send(rvn_window_t* window, RDropPhase phase, float x, float y,
	const char* const* paths, int path_count, const char* text) {

	RDropEvent event;
	RDropCallback callback;
	void* user;
	Rid target;
	int result;

	memset(&event, 0, sizeof(event));
	event.phase = phase;
	event.window = window->id;
	event.x = event.local_x = x;
	event.y = event.local_y = y;
	event.paths = paths;
	event.path_count = path_count;
	event.text = text;
	if (window->drag_active) {
		event.text = window->drag_data;
		event.drag_type = window->drag_type;
		event.drag_source = window->drag_source;
	}
	if (window->drop_target) {
		rvn_vec2_t origin = rvn_widget_origin(window->drop_target);
		event.local_x = x - origin.x;
		event.local_y = y - origin.y;
		callback = window->drop_target->drop_callback;
		user = window->drop_target->drop_user;
		target = window->drop_target->id;
	} else if (window->drop_on_window) {
		callback = window->drop_callback;
		user = window->drop_user;
		target = window->id;
	} else {
		return R_FALSE;
	}
	if (!callback) return R_FALSE;
	rvn_dispatch_begin();
	result = callback(target, &event, user) != 0;
	rvn_dispatch_end();
	return result;
}

static void rvn_window_drop_clear(rvn_window_t* window) {
	window->drop_target = NULL;
	window->drop_on_window = R_FALSE;
	window->drop_accepted = R_FALSE;
	window->needs_paint = R_TRUE;
}

/* The drag moved to (x, y), or left the window. Returns R_TRUE when a drop there would be taken. */
static int rvn_window_drag(rvn_window_t* window, float x, float y, int inside) {
	rvn_widget_t* target = NULL;
	int on_window, accepted;

	if (inside) {
		for (target = rvn_window_hit(window, x, y, NULL); target; target = target->parent) {
			if (target->drop_callback && rvn_widget_is_enabled(target)) break;
		}
	}
	/* a drag inside the application only goes to widgets */
	on_window = inside && !target && window->drop_callback != NULL && !window->drag_active;

	if (target != window->drop_target || on_window != window->drop_on_window) {
		Rid target_id = target ? target->id : R_INVALID_ID;
		if (window->drop_target || window->drop_on_window) rvn_window_drop_send(window, R_DROP_LEAVE, x, y, NULL, 0, NULL);
		if (window->dead) return R_FALSE;
		rvn_window_drop_clear(window);
		/* (the callback may have deleted the new target) */
		target = target ? rvn_widget_get(target_id) : NULL;
		if (!target && !on_window) return R_FALSE;
		window->drop_target = target;
		window->drop_on_window = !target;
		accepted = rvn_window_drop_send(window, R_DROP_ENTER, x, y, NULL, 0, NULL);
	} else if (target || on_window) {
		accepted = rvn_window_drop_send(window, R_DROP_MOVE, x, y, NULL, 0, NULL);
	} else {
		return R_FALSE;
	}
	if (window->dead) return R_FALSE;
	window->drop_accepted = accepted && (window->drop_target || window->drop_on_window);
	window->needs_paint = R_TRUE;
	if (window->drop_accepted) {
		rvn_rect_t old = window->drop_rect;
		if (window->drop_target) {
			rvn_vec2_t origin = rvn_widget_origin(window->drop_target);
			window->drop_rect = rvn_rect(origin.x, origin.y, window->drop_target->rect.w, window->drop_target->rect.h);
			window->drop_radius = rvn_widget_metric(window->drop_target, R_METRIC_CORNER_RADIUS) + 2.0f;
		} else {
			rvn_vec2_t size = rvn_window_size(window);
			window->drop_rect = rvn_rect(8.0f, 8.0f, rvn_maxf(size.x - 16.0f, 0.0f), rvn_maxf(size.y - 16.0f, 0.0f));
			window->drop_radius = 14.0f;
		}
		/* another target: the highlight lands again */
		if (old.x != window->drop_rect.x || old.y != window->drop_rect.y || old.w != window->drop_rect.w || old.h != window->drop_rect.h) {
			memset(&window->drop_shown, 0, sizeof(window->drop_shown));
		}
	}
	return window->drop_accepted;
}

static int rvn_window_drop(rvn_window_t* window, float x, float y, const char* const* paths, int path_count, const char* text) {
	int taken = R_FALSE;
	/* (some window systems only tell about the drop: this finds the target) */
	if (rvn_window_drag(window, x, y, R_TRUE)) taken = rvn_window_drop_send(window, R_DROP_DROPPED, x, y, paths, path_count, text);
	if (!window->dead) rvn_window_drop_clear(window);
	return taken;
}

/* ---------------------------------------------------------------------------
 *  Input (positions in logical units)
 * ------------------------------------------------------------------------- */

static void rvn_window_drag_end(rvn_window_t* window) {
	window->drag_active = R_FALSE;
	rvn_free(window->drag_type);
	rvn_free(window->drag_data);
	rvn_free(window->drag_label);
	window->drag_type = window->drag_data = window->drag_label = NULL;
	window->drag_source = R_INVALID_ID;
	window->needs_paint = R_TRUE;
}

static void rvn_window_drag_cancel(rvn_window_t* window) {
	if (!window->drag_active) return;
	if (window->drop_target || window->drop_on_window) rvn_window_drop_send(window, R_DROP_LEAVE, window->mouse_x, window->mouse_y, NULL, 0, NULL);
	if (window->dead) return;
	rvn_window_drop_clear(window);
	rvn_window_drag_end(window);
}

static void rvn_window_paint_drag(rvn_window_t* window, RPainter* painter) {
	const char* label;
	float size, width, x, y;
	rvn_font_t* font;
	if (!window->drag_active || !window->mouse_inside) return;
	label = window->drag_label && window->drag_label[0] ? window->drag_label : NULL;
	if (!label) return;
	size = RVN_THEME_METRIC(R_METRIC_FONT_SIZE);
	font = rvn_font_resolve(R_INVALID_ID);
	width = (font ? rvn_text_width(font, size, label, -1) : 80.0f) + 20.0f;
	x = window->mouse_x + 14.0f;
	y = window->mouse_y + 10.0f;
	rvn_paint_shadow(painter, rvn_rect(x, y, width, size + 12.0f), 6.0f, 12.0f, rvn_color_fade(RVN_THEME_COLOR(R_COLOR_SHADOW), 0.6f));
	rvn_paint_box(painter, rvn_rect(x, y, width, size + 12.0f), 6.0f, RVN_THEME_COLOR(R_COLOR_POPUP),
		1.0f, RVN_THEME_COLOR(window->drop_accepted ? R_COLOR_ACCENT : R_COLOR_POPUP_BORDER));
	rvn_paint_text_in(painter, R_INVALID_ID, size, rvn_rect(x + 10.0f, y, width - 20.0f, size + 12.0f), 0.0f, label, -1,
		RVN_THEME_COLOR(R_COLOR_TEXT));
}

/* The hook of the window sees the event before the widgets; R_TRUE consumes it. */
static int rvn_window_hook(rvn_window_t* window, REvent* event) {
	if (!window->event_hook) return R_FALSE;
	event->target = window->hover ? window->hover->id : R_INVALID_ID;
	return window->event_hook(window->id, event, window->event_hook_user) ? R_TRUE : R_FALSE;
}

static void rvn_window_mouse_move(rvn_window_t* window, float x, float y) {
	REvent event;
	float dx = window->mouse_inside ? x - window->mouse_x : 0.0f;
	float dy = window->mouse_inside ? y - window->mouse_y : 0.0f;

	window->mouse_x = x;
	window->mouse_y = y;
	window->mouse_inside = R_TRUE;
	if (window->tooltip_visible && (fabsf(x - window->tooltip_x) > 6.0f || fabsf(y - window->tooltip_y) > 6.0f)) {
		rvn_window_tooltip_arm(window);
	}

	/* custom header: the edges resize the window and are not part of the content */
	if ((window->window_flags & R_WINDOW_CUSTOM_HEADER) && !g_rvn.platform->native_regions && !window->capture) {
		rvn_pregion_t region = window->buttons ? RVN_PREGION_CLIENT : rvn_window_region(window, x, y);
		window->edge = region == RVN_PREGION_CAPTION ? RVN_PREGION_CLIENT : region;
		if (window->edge != RVN_PREGION_CLIENT) {
			rvn_window_set_hover(window, NULL);
			if (!window->dead) rvn_window_update_cursor(window);
			return;
		}
	}

	event = rvn_window_event(window, R_EVENT_MOUSE_MOVE);
	event.delta_x = dx;
	event.delta_y = dy;
	if (!window->capture) {
		rvn_widget_t* under = rvn_window_hit(window, x, y, NULL);
		rvn_window_set_hover(window, under);
		if (window->dead) return;
	}
	if (rvn_window_hook(window, &event) || window->dead) {
		if (!window->dead) rvn_window_update_cursor(window);
		return;
	}
	if (window->drag_active) {
		rvn_window_drag(window, x, y, R_TRUE);
		window->needs_paint = R_TRUE;
		return;
	}
	if (window->capture) {
		rvn_widget_send(window->capture, &event);
	} else {
		rvn_widget_t* hit = rvn_window_hit(window, x, y, NULL);
		rvn_window_set_hover(window, hit);
		if (window->dead) return;
		hit = window->hover;
		if (hit && rvn_widget_is_enabled(hit)) rvn_widget_dispatch(hit, &event);
	}
	if (!window->dead) rvn_window_update_cursor(window);
}

static void rvn_window_mouse_leave(rvn_window_t* window) {
	window->mouse_inside = R_FALSE;
	window->edge = RVN_PREGION_CLIENT;
	if (!window->capture) rvn_window_set_hover(window, NULL);
	if (!window->dead) rvn_window_tooltip_hide(window);
}

static rvn_widget_t* rvn_focusable_ancestor(rvn_widget_t* widget) {
	for (; widget; widget = widget->parent) {
		if (widget->flags & RVN_WF_FOCUSABLE) return widget;
		if (widget->flags & RVN_WF_POPUP) break;
	}
	return NULL;
}

static void rvn_window_open_context_menu(rvn_window_t* window, rvn_widget_t* target) {
	rvn_widget_t* walk;
	for (walk = target; walk; walk = walk->parent) {
		rvn_widget_t* menu;
		if (walk->context_menu == R_INVALID_ID) continue;
		menu = rvn_widget_get(walk->context_menu);
		if (menu && menu->cls->type == R_WIDGET_MENU) rvn_menu_popup(menu, window, window->mouse_x, window->mouse_y);
		return;
	}
}

static void rvn_window_mouse_button(rvn_window_t* window, RMouseButton button, int pressed) {
	REvent event;
	rvn_widget_t* target;
	rvn_widget_t* hit;
	int popup_index = -1;
	float x = window->mouse_x, y = window->mouse_y;

	if ((int)button < 0 || (int)button > R_MOUSE_X2) return;
	if (pressed) window->buttons |= 1u << button;
	else window->buttons &= ~(1u << button);
	rvn_window_tooltip_hide(window);

	{
		REvent hooked = rvn_window_event(window, pressed ? R_EVENT_MOUSE_DOWN : R_EVENT_MOUSE_UP);
		hooked.button = button;
		hooked.clicks = window->click_count;
		if (rvn_window_hook(window, &hooked) || window->dead) {
			if (!pressed && window->buttons == 0) window->capture = NULL;
			if (!pressed && window->drag_active && window->buttons == 0) rvn_window_drag_cancel(window);
			return;
		}
	}

	/* the end of a drag inside the application: dropped where the pointer is */
	if (window->drag_active) {
		if (!pressed && window->buttons == 0) {
			if (rvn_window_drag(window, x, y, R_TRUE) && !window->dead) {
				rvn_window_drop_send(window, R_DROP_DROPPED, x, y, NULL, 0, NULL);
			}
			if (window->dead) return;
			rvn_window_drop_clear(window);
			rvn_window_drag_end(window);
			window->capture = NULL;
			rvn_window_refresh_hover(window);
		} else if (pressed && button != R_MOUSE_LEFT) {
			rvn_window_drag_cancel(window);
		}
		return;
	}

	hit = rvn_window_hit(window, x, y, &popup_index);

	if (pressed) {
		double now = rvn_clock_seconds();
		window->focus_visible = R_FALSE;
		if (button == (RMouseButton)window->click_button && now - window->click_time < RVN_DOUBLE_CLICK_TIME &&
			fabsf(x - window->click_x) < RVN_DOUBLE_CLICK_DISTANCE && fabsf(y - window->click_y) < RVN_DOUBLE_CLICK_DISTANCE) {
			window->click_count++;
		} else {
			window->click_count = 1;
		}
		window->click_time = now;
		window->click_x = x;
		window->click_y = y;
		window->click_button = (int)button;

		/* a click outside of the open menus and lists closes them and is not passed on */
		if (window->popup_count > 0 && !window->capture &&
			window->popups[window->popup_count - 1].placement != RVN_POPUP_DIALOG &&
			(popup_index < 0 || window->popups[popup_index].placement == RVN_POPUP_DIALOG)) {
			rvn_window_close_popups(window);
			rvn_window_refresh_hover(window);
			return;
		}

		if (button == R_MOUSE_LEFT && (window->window_flags & R_WINDOW_CUSTOM_HEADER) && rvn_window_press_region(window, x, y)) {
			return;
		}

		/* an island unfolded by a click folds when the user goes on with something else */
		if (!window->capture) {
			rvn_islands_pressed(window, hit, popup_index >= 0);
			if (window->dead) return;
		}

		target = window->capture ? window->capture : hit;
		if (!target) {
			rvn_window_set_focus(window, NULL);
			return;
		}
		if (!rvn_widget_is_enabled(target)) return;
		/* menus and lists do not take the keyboard focus from the widget that opened them; a dialog has its own */
		if ((popup_index < 0 || window->popups[popup_index].placement == RVN_POPUP_DIALOG) && !window->capture) {
			rvn_window_set_focus(window, rvn_focusable_ancestor(target));
			if (window->dead || (target->flags & RVN_WF_DEAD)) return;
		}
		event = rvn_window_event(window, R_EVENT_MOUSE_DOWN);
		event.button = button;
		event.clicks = window->click_count;
		if (window->capture) rvn_widget_send(target, &event);
		else rvn_widget_dispatch(target, &event);
	} else {
		int was_captured = window->capture != NULL;
		target = window->capture ? window->capture : hit;
		if (target && rvn_widget_is_enabled(target)) {
			event = rvn_window_event(window, R_EVENT_MOUSE_UP);
			event.button = button;
			event.clicks = window->click_count;
			if (window->capture) rvn_widget_send(target, &event);
			else rvn_widget_dispatch(target, &event);
		}
		if (window->dead) return;
		if (window->buttons == 0) {
			/* the hover was frozen during the capture: bring it up to date */
			window->capture = NULL;
			if (was_captured) rvn_window_refresh_hover(window);
		}
		if (button == R_MOUSE_RIGHT && hit && !(hit->flags & RVN_WF_DEAD) && !window->dead) {
			rvn_window_open_context_menu(window, hit);
		}
	}
	if (!window->dead) rvn_window_update_cursor(window);
}

static void rvn_window_mouse_wheel(rvn_window_t* window, float wheel_x, float wheel_y) {
	REvent event;
	rvn_widget_t* target = window->capture ? window->capture : rvn_window_hit(window, window->mouse_x, window->mouse_y, NULL);
	rvn_window_tooltip_hide(window);
	event = rvn_window_event(window, R_EVENT_MOUSE_WHEEL);
	event.wheel_x = wheel_x;
	event.wheel_y = wheel_y;
	if (rvn_window_hook(window, &event) || window->dead) return;
	if (!target || !rvn_widget_is_enabled(target)) return;
	rvn_widget_dispatch(target, &event);
}

static void rvn_window_key(rvn_window_t* window, RKey key, int pressed, uint32_t modifiers, int repeat) {
	REvent event;
	uint32_t i;

	window->modifiers = modifiers;
	if ((int)key > 0 && (int)key < R_KEY_COUNT) window->keys[key] = pressed ? 1 : 0;
	if (pressed) rvn_window_tooltip_hide(window);
	event = rvn_window_event(window, pressed ? R_EVENT_KEY_DOWN : R_EVENT_KEY_UP);
	event.key = key;
	event.repeat = repeat;
	if (rvn_window_hook(window, &event) || window->dead) return;
	if (window->drag_active) {
		if (pressed && key == R_KEY_ESCAPE) rvn_window_drag_cancel(window);
		return;
	}

	/* the top popup sees the keyboard first (menu navigation) */
	if (window->popup_count > 0 && window->popups[window->popup_count - 1].placement != RVN_POPUP_DIALOG) {
		rvn_widget_t* top = window->popups[window->popup_count - 1].root;
		if (rvn_widget_dispatch(top, &event) || window->dead) return;
		if (pressed && key == R_KEY_ESCAPE && window->popup_count > 0) {
			rvn_window_close_popup(window, window->popups[window->popup_count - 1].root);
			return;
		}
	}
	if (window->focus && rvn_widget_is_enabled(window->focus)) {
		if (rvn_widget_dispatch(window->focus, &event) || window->dead) return;
	} else if (rvn_window_top_dialog(window)) {
		/* nothing has the keyboard: the dialog itself may want the key (Enter, a letter) */
		if (rvn_widget_dispatch(rvn_window_top_dialog(window), &event) || window->dead) return;
	}
	if (!pressed) return;

	if (rvn_window_top_dialog(window)) {
		/* Escape dismisses the dialog; the shortcuts of the window wait */
		if (key == R_KEY_TAB && !(modifiers & (R_MOD_CTRL | R_MOD_ALT))) rvn_window_focus_next(window, (modifiers & R_MOD_SHIFT) ? -1 : 1);
		else if (key == R_KEY_ESCAPE) rvn_window_close_dialog(rvn_window_top_dialog(window), 0);
		return;
	}

	if (key == R_KEY_TAB && window->tab_navigation && !(modifiers & (R_MOD_CTRL | R_MOD_ALT))) {
		rvn_window_focus_next(window, (modifiers & R_MOD_SHIFT) ? -1 : 1);
		return;
	}
	if (key == R_KEY_ESCAPE && window->focus) {
		rvn_window_set_focus(window, NULL);
		return;
	}
	for (i = 0; i < window->shortcut_count; i++) {
		rvn_shortcut_t* shortcut = &window->shortcuts[i];
		char* action;
		if (shortcut->key != key || shortcut->modifiers != (modifiers & 0xFu)) continue;
		/* copy: the callback may unregister the shortcut */
		action = rvn_strdup(shortcut->action);
		rvn_fire_action(window->id, action);
		rvn_free(action);
		return;
	}
	/* shortcuts shown in the menus of the window */
	{
		rvn_widget_t* item = rvn_menu_find_shortcut(window->root, key, modifiers & 0xFu);
		if (item) rvn_menu_item_trigger(item);
	}
}

static void rvn_window_text(rvn_window_t* window, uint32_t codepoint) {
	REvent event;
	event = rvn_window_event(window, R_EVENT_TEXT);
	event.codepoint = codepoint;
	if (rvn_window_hook(window, &event) || window->dead) return;
	if (!window->focus || !rvn_widget_is_enabled(window->focus)) return;
	rvn_widget_dispatch(window->focus, &event);
}

static void rvn_window_resized(rvn_window_t* window, int width_px, int height_px, float platform_scale) {
	float scale = (platform_scale > 0.0f ? platform_scale : 1.0f) * g_rvn.ui_scale;
	rvn_vec2_t size;
	if (window->width_px == width_px && window->height_px == height_px && window->scale == scale) return;
	window->width_px = width_px;
	window->height_px = height_px;
	window->scale = scale;
	rvn_window_close_popups(window);
	window->needs_layout = R_TRUE;
	window->needs_paint = R_TRUE;
	if (window->resize_callback) {
		size = rvn_window_size(window);
		window->resize_callback(window->id, (int)(size.x + 0.5f), (int)(size.y + 0.5f), window->resize_user);
	}
}

/* ---------------------------------------------------------------------------
 *  Platform events
 * ------------------------------------------------------------------------- */

void rvn_window_on_platform_event(const rvn_pevent_t* pevent) {
	rvn_window_t* window = (rvn_window_t*)pevent->user;
	float scale;

	if (pevent->type == RVN_PEVENT_MODAL_TICK) {
		rvn_update();
		return;
	}
	if (!window || window->dead) return;
	scale = window->scale > 0.0f ? window->scale : 1.0f;

	/* a dialog of the OS is open: the windows are shown, resized and repainted, but take no input */
	if (g_rvn.input_blocked > 0) {
		switch (pevent->type) {
			case RVN_PEVENT_RESIZE:
			case RVN_PEVENT_EXPOSE:
			case RVN_PEVENT_FOCUS:
			case RVN_PEVENT_STATE:
				break;
			case RVN_PEVENT_DRAG:
			case RVN_PEVENT_DROP:
				if (pevent->accepted) *pevent->accepted = R_FALSE;
				return;
			default:
				return;
		}
	}

	rvn_dispatch_begin();
	switch (pevent->type) {
		case RVN_PEVENT_CLOSE:
			rvn_window_request_close(window);
			break;
		case RVN_PEVENT_RESIZE:
			rvn_window_resized(window, pevent->width, pevent->height, pevent->scale);
			break;
		case RVN_PEVENT_EXPOSE:
			window->needs_paint = R_TRUE;
			break;
		case RVN_PEVENT_MOUSE_MOVE:
			window->modifiers = pevent->modifiers;
			rvn_window_mouse_move(window, (float)pevent->x / scale, (float)pevent->y / scale);
			break;
		case RVN_PEVENT_MOUSE_BUTTON:
			window->modifiers = pevent->modifiers;
			rvn_window_mouse_move(window, (float)pevent->x / scale, (float)pevent->y / scale);
			if (!window->dead) rvn_window_mouse_button(window, (RMouseButton)pevent->button, pevent->pressed);
			break;
		case RVN_PEVENT_MOUSE_WHEEL:
			window->modifiers = pevent->modifiers;
			rvn_window_mouse_wheel(window, pevent->wheel_x, pevent->wheel_y);
			break;
		case RVN_PEVENT_MOUSE_LEAVE:
			rvn_window_mouse_leave(window);
			break;
		case RVN_PEVENT_KEY:
			rvn_window_key(window, pevent->key, pevent->pressed, pevent->modifiers, pevent->repeat);
			break;
		case RVN_PEVENT_TEXT:
			rvn_window_text(window, pevent->codepoint);
			break;
		case RVN_PEVENT_FOCUS:
			if (!pevent->focused) {
				/* the window lost the keyboard: drop transient state */
				window->capture = NULL;
				window->buttons = 0;
				window->modifiers = 0;
				memset(window->keys, 0, sizeof(window->keys));
				rvn_window_drag_cancel(window);
				rvn_window_close_popups(window);
				rvn_window_tooltip_hide(window);
			}
			window->active = pevent->focused;
			window->needs_paint = R_TRUE;
			break;
		case RVN_PEVENT_STATE:
			if (window->state != pevent->state) {
				window->state = pevent->state;
				window->edge = RVN_PREGION_CLIENT;
				window->needs_layout = R_TRUE;
				window->needs_paint = R_TRUE;
			}
			break;
		case RVN_PEVENT_DRAG: {
			int accepted = rvn_window_drag(window, (float)pevent->x / scale, (float)pevent->y / scale, !pevent->leaving);
			if (pevent->accepted) *pevent->accepted = accepted;
			break;
		}
		case RVN_PEVENT_DROP: {
			int taken = rvn_window_drop(window, (float)pevent->x / scale, (float)pevent->y / scale,
				pevent->drop_paths, pevent->drop_count, pevent->drop_text);
			if (pevent->accepted) *pevent->accepted = taken;
			break;
		}
		case RVN_PEVENT_MODAL_TICK:
			break;
	}
	rvn_dispatch_end();
}

/* ---------------------------------------------------------------------------
 *  Shortcuts
 * ------------------------------------------------------------------------- */

void rvn_window_remove_shortcut(rvn_window_t* window, RKey key, uint32_t modifiers) {
	uint32_t i;
	for (i = 0; i < window->shortcut_count; i++) {
		if (window->shortcuts[i].key == key && window->shortcuts[i].modifiers == modifiers) {
			rvn_free(window->shortcuts[i].action);
			window->shortcuts[i] = window->shortcuts[--window->shortcut_count];
			return;
		}
	}
}

void rvn_window_add_shortcut(rvn_window_t* window, RKey key, uint32_t modifiers, const char* action) {
	rvn_shortcut_t* shortcut;
	rvn_window_remove_shortcut(window, key, modifiers);
	RVN_ARRAY_RESERVE(window->shortcuts, window->shortcut_cap, window->shortcut_count + 1);
	shortcut = &window->shortcuts[window->shortcut_count++];
	shortcut->key = key;
	shortcut->modifiers = modifiers;
	shortcut->action = rvn_strdup(action);
}

/* ---------------------------------------------------------------------------
 *  Loss of the device
 * ------------------------------------------------------------------------- */

void rvn_windows_device_reset(void) {
	uint32_t i;
	for (i = 0; i < g_rvn.windows.used; i++) {
		rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		RNativeWindow native;
		if (!window || !window->platform || !window->surface_ready) continue;
		/* the backend was started again: the surface of the window is to be made again, as at its creation */
		memset(&native, 0, sizeof(native));
		g_rvn.platform->window_get_native(window->platform, &native);
		if (!rvn_renderer_create_surface(window->id, &native, (uint32_t)rvn_maxi(window->width_px, 1),
			(uint32_t)rvn_maxi(window->height_px, 1))) {
			rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "after the loss of the device: cannot make the surface of a window again");
			window->surface_ready = R_FALSE;
			continue;
		}
		window->needs_layout = R_TRUE;
		window->needs_paint = R_TRUE;
	}
}

/* ---------------------------------------------------------------------------
 *  Destruction
 * ------------------------------------------------------------------------- */

static void rvn_window_free(rvn_window_t* window) {
	uint32_t i;
	/* waits for the frames still queued: they point into this structure */
	if (window->surface_ready) rvn_renderer_destroy_surface(window->id);
	if (window->platform) g_rvn.platform->window_destroy(window->platform);
	rvn_pool_remove(&g_rvn.windows, window->id);
	for (i = 0; i < window->shortcut_count; i++) rvn_free(window->shortcuts[i].action);
	rvn_free(window->shortcuts);
	rvn_free(window->native_viewports);
	rvn_free(window->popups);
	rvn_free(window->ghosts);
	rvn_free(window->layout_springs);
	rvn_free(window->title);
	rvn_free(window->drag_type);
	rvn_free(window->drag_data);
	rvn_free(window->drag_label);
	rvn_free(window);
}

void rvn_window_destroy(rvn_window_t* window) {
	rvn_widget_t* root;
	if (window->dead) return;
	rvn_window_close_all_popups(window);
	if (window->dead) return;
	while (window->ghost_count > 0) rvn_window_remove_ghost(window, 0);
	rvn_window_tooltip_hide(window);
	root = window->root;
	if (root) rvn_widget_destroy(root);
	window->root = NULL;
	if (window->island) rvn_widget_destroy(window->island);
	window->island = NULL;
	window->hover = window->focus = window->capture = NULL;
	/* the handle stops resolving now; the slot stays reserved until the surface is gone */
	window->dead = R_TRUE;
	if (g_rvn.dispatch_depth > 0) {
		RVN_ARRAY_RESERVE(g_rvn.dead_windows, g_rvn.dead_window_cap, g_rvn.dead_window_count + 1);
		g_rvn.dead_windows[g_rvn.dead_window_count++] = window;
	} else {
		rvn_window_free(window);
	}
}

void rvn_windows_collect(void) {
	uint32_t i;
	for (i = 0; i < g_rvn.dead_window_count; i++) rvn_window_free(g_rvn.dead_windows[i]);
	g_rvn.dead_window_count = 0;
}

void rvn_windows_destroy_all(void) {
	uint32_t i;
	for (i = 0; i < g_rvn.windows.used; i++) {
		rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (window && !window->dead) rvn_window_destroy(window);
	}
	rvn_windows_collect();
}

/* ---------------------------------------------------------------------------
 *  Public API: windows
 * ------------------------------------------------------------------------- */

Rid Rwindow_create(const char* title, int width, int height, Ruint flags) {
	rvn_window_t* window;
	rvn_pwindow_desc_t desc;
	RNativeWindow native;

	RVN_API_GUARD(R_INVALID_ID);
	if (width <= 0 || height <= 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rwindow_create: invalid size %d x %d", width, height);
		return R_INVALID_ID;
	}

	window = (rvn_window_t*)rvn_calloc(sizeof(rvn_window_t));
	window->title = rvn_strdup(title ? title : "");
	window->tooltip_timer = R_INVALID_ID;
	window->cursor = R_CURSOR_ARROW;
	window->click_button = -1;
	window->opacity = 1.0f;
	window->tab_navigation = R_TRUE;
	window->id = rvn_pool_add(&g_rvn.windows, window);
	if (window->id == R_INVALID_ID) {
		rvn_free(window->title);
		rvn_free(window);
		return R_INVALID_ID;
	}

	desc.title = window->title;
	desc.width = (int)((float)width * g_rvn.ui_scale + 0.5f);
	desc.height = (int)((float)height * g_rvn.ui_scale + 0.5f);
	desc.flags = flags;
	desc.user = window;
	window->platform = g_rvn.platform->window_create(&desc);
	if (!window->platform) {
		rvn_error(R_ERROR_PLATFORM, R_SEVERITY_ERROR, "Rwindow_create: the window system refused to create the window");
		rvn_pool_remove(&g_rvn.windows, window->id);
		rvn_free(window->title);
		rvn_free(window);
		return R_INVALID_ID;
	}
	g_rvn.platform->window_get_size(window->platform, &window->width_px, &window->height_px);
	window->scale = g_rvn.platform->window_get_scale(window->platform) * g_rvn.ui_scale;
	if (window->scale <= 0.0f) window->scale = 1.0f;

	memset(&native, 0, sizeof(native));
	g_rvn.platform->window_get_native(window->platform, &native);
	if (!rvn_renderer_create_surface(window->id, &native, (uint32_t)rvn_maxi(window->width_px, 1),
		(uint32_t)rvn_maxi(window->height_px, 1))) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Rwindow_create: cannot create the rendering surface");
		g_rvn.platform->window_destroy(window->platform);
		rvn_pool_remove(&g_rvn.windows, window->id);
		rvn_free(window->title);
		rvn_free(window);
		return R_INVALID_ID;
	}
	window->surface_ready = R_TRUE;
	window->needs_layout = R_TRUE;
	window->needs_paint = R_TRUE;
	window->visible = (flags & R_WINDOW_HIDDEN) == 0;
	window->window_flags = flags;
	window->transparent = native.transparent != 0;
	if (flags & R_WINDOW_TRANSPARENT) {
		/* seen through from the start; Rwindow_set_backdrop says what the OS puts behind */
		window->backdrop_wanted = R_BACKDROP_TRANSPARENT;
		if (window->transparent && g_rvn.platform->window_set_backdrop) {
			window->backdrop = (RBackdrop)g_rvn.platform->window_set_backdrop(window->platform, (int)R_BACKDROP_TRANSPARENT, g_rvn.theme.dark);
			window->backdrop_dark = g_rvn.theme.dark;
		}
	}
	window->state = g_rvn.platform->window_get_state(window->platform);
	window->active = R_TRUE;
	g_rvn.had_window = R_TRUE;
	if (window->visible) g_rvn.platform->window_show(window->platform, R_TRUE);
	return window->id;
}

void Rwindow_destroy(Rid id) {
	RVN_WINDOW_VOID(window, id);
	rvn_window_destroy(window);
}

int Rwindow_is_valid(Rid window) {
	return g_rvn.initialized && rvn_window_get(window) != NULL;
}

void Rwindow_set_content(Rid id, Rid widget_id) {
	rvn_widget_t* widget = NULL;
	RVN_WINDOW_VOID(window, id);
	if (widget_id != R_INVALID_ID) {
		widget = rvn_widget_check(widget_id, 0, __func__);
		if (!widget) return;
	}
	if (window->root == widget) return;
	if (window->root) rvn_widget_destroy(window->root);
	window->root = NULL;
	if (widget) {
		if (widget->parent || widget->window) rvn_widget_detach(widget);
		window->root = widget;
		rvn_widget_set_window(widget, window);
		widget->flags |= RVN_WF_LAYOUT_DIRTY;
	}
	window->needs_layout = R_TRUE;
	window->needs_paint = R_TRUE;
}

int Rwindow_set_backdrop(Rid id, RBackdrop backdrop) {
	int given;
	RVN_WINDOW(window, id, R_FALSE);
	if (backdrop < R_BACKDROP_NONE || backdrop > R_BACKDROP_MICA) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwindow_set_backdrop: invalid backdrop");
		return R_FALSE;
	}
	if (backdrop != R_BACKDROP_NONE && !(window->window_flags & R_WINDOW_TRANSPARENT)) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rwindow_set_backdrop: the window was not created with R_WINDOW_TRANSPARENT");
		return R_FALSE;
	}
	given = (int)R_BACKDROP_NONE;
	if (backdrop != R_BACKDROP_NONE && window->transparent && g_rvn.platform->window_set_backdrop) {
		given = g_rvn.platform->window_set_backdrop(window->platform, (int)backdrop, g_rvn.theme.dark);
	} else if (g_rvn.platform->window_set_backdrop && window->transparent) {
		g_rvn.platform->window_set_backdrop(window->platform, (int)R_BACKDROP_NONE, g_rvn.theme.dark);
	}
	window->backdrop_wanted = backdrop;
	window->backdrop = (RBackdrop)given;
	window->backdrop_dark = g_rvn.theme.dark;
	window->needs_paint = R_TRUE;
	return backdrop == R_BACKDROP_NONE || given != (int)R_BACKDROP_NONE;
}

RBackdrop Rwindow_get_backdrop(Rid id) {
	RVN_WINDOW(window, id, R_BACKDROP_NONE);
	return window->backdrop ? window->backdrop : R_BACKDROP_NONE;
}

int Rwindow_set_opacity(Rid id, float opacity) {
	RVN_WINDOW(window, id, R_FALSE);
	opacity = rvn_clampf(opacity, 0.0f, 1.0f);
	if (!g_rvn.platform->window_set_opacity || !g_rvn.platform->window_set_opacity(window->platform, opacity)) return R_FALSE;
	window->opacity = opacity;
	return R_TRUE;
}

int Rwindow_set_topmost(Rid id, int topmost) {
	RVN_WINDOW(window, id, R_FALSE);
	if (!g_rvn.platform->window_set_topmost) return R_FALSE;
	return g_rvn.platform->window_set_topmost(window->platform, topmost ? R_TRUE : R_FALSE);
}

void Rwindow_center(Rid id) {
	RVN_WINDOW_VOID(window, id);
	if (g_rvn.platform->window_center) g_rvn.platform->window_center(window->platform);
}

float Rwindow_get_opacity(Rid id) {
	RVN_WINDOW(window, id, 1.0f);
	return window->opacity;
}

void Rwindow_set_island(Rid id, Rid widget_id) {
	rvn_widget_t* widget = NULL;
	RVN_WINDOW_VOID(window, id);
	if (widget_id != R_INVALID_ID) {
		widget = rvn_widget_check(widget_id, 0, __func__);
		if (!widget) return;
	}
	if (window->island == widget) return;
	if (window->island) rvn_widget_destroy(window->island);
	window->island = NULL;
	if (widget) {
		if (widget->parent || widget->window) rvn_widget_detach(widget);
		window->island = widget;
		rvn_widget_set_window(widget, window);
		widget->flags |= RVN_WF_LAYOUT_DIRTY;
		rvn_widget_appear(widget);
	}
	window->needs_layout = R_TRUE;
	window->needs_paint = R_TRUE;
}

Rid Rwindow_get_island(Rid id) {
	RVN_WINDOW(window, id, R_INVALID_ID);
	return window->island ? window->island->id : R_INVALID_ID;
}

Rid Rwindow_get_content(Rid id) {
	RVN_WINDOW(window, id, R_INVALID_ID);
	return window->root ? window->root->id : R_INVALID_ID;
}

void Rwindow_set_title(Rid id, const char* title) {
	RVN_WINDOW_VOID(window, id);
	if (!rvn_str_set(&window->title, title ? title : "")) return;
	g_rvn.platform->window_set_title(window->platform, window->title);
	/* a header bar shows it */
	window->needs_paint = R_TRUE;
}

const char* Rwindow_get_title(Rid id) {
	RVN_WINDOW(window, id, "");
	return window->title ? window->title : "";
}

void Rwindow_set_size(Rid id, int width, int height) {
	RVN_WINDOW_VOID(window, id);
	if (width <= 0 || height <= 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rwindow_set_size: invalid size");
		return;
	}
	g_rvn.platform->window_set_size(window->platform, (int)((float)width * window->scale + 0.5f),
		(int)((float)height * window->scale + 0.5f));
}

void Rwindow_get_size(Rid id, int* width, int* height) {
	rvn_window_t* window;
	rvn_vec2_t size;
	if (width) *width = 0;
	if (height) *height = 0;
	window = rvn_window_check(id, __func__);
	if (!window) return;
	size = rvn_window_size(window);
	if (width) *width = (int)(size.x + 0.5f);
	if (height) *height = (int)(size.y + 0.5f);
}

void Rwindow_set_min_size(Rid id, int width, int height) {
	RVN_WINDOW_VOID(window, id);
	g_rvn.platform->window_set_min_size(window->platform, (int)((float)rvn_maxi(width, 0) * window->scale + 0.5f),
		(int)((float)rvn_maxi(height, 0) * window->scale + 0.5f));
}

void Rwindow_set_position(Rid id, int x, int y) {
	RVN_WINDOW_VOID(window, id);
	g_rvn.platform->window_set_position(window->platform, x, y);
}

void Rwindow_get_position(Rid id, int* x, int* y) {
	rvn_window_t* window;
	int px = 0, py = 0;
	if (x) *x = 0;
	if (y) *y = 0;
	window = rvn_window_check(id, __func__);
	if (!window) return;
	g_rvn.platform->window_get_position(window->platform, &px, &py);
	if (x) *x = px;
	if (y) *y = py;
}

void Rwindow_get_framebuffer_size(Rid id, int* width, int* height) {
	rvn_window_t* window;
	if (width) *width = 0;
	if (height) *height = 0;
	window = rvn_window_check(id, __func__);
	if (!window) return;
	if (width) *width = window->width_px;
	if (height) *height = window->height_px;
}

float Rwindow_get_scale(Rid id) {
	RVN_WINDOW(window, id, 1.0f);
	return window->scale;
}

void Rwindow_show(Rid id) {
	RVN_WINDOW_VOID(window, id);
	window->visible = R_TRUE;
	window->needs_paint = R_TRUE;
	g_rvn.platform->window_show(window->platform, R_TRUE);
}

void Rwindow_hide(Rid id) {
	RVN_WINDOW_VOID(window, id);
	window->visible = R_FALSE;
	/* a frame still queued must not be presented once the window is hidden (on Wayland that would show it again) */
	rvn_renderer_sync();
	g_rvn.platform->window_show(window->platform, R_FALSE);
}

void Rwindow_maximize(Rid id) {
	RVN_WINDOW_VOID(window, id);
	g_rvn.platform->window_set_state(window->platform, RVN_PWINDOW_MAXIMIZED);
}

void Rwindow_minimize(Rid id) {
	RVN_WINDOW_VOID(window, id);
	g_rvn.platform->window_set_state(window->platform, RVN_PWINDOW_MINIMIZED);
}

void Rwindow_restore(Rid id) {
	RVN_WINDOW_VOID(window, id);
	g_rvn.platform->window_set_state(window->platform, RVN_PWINDOW_NORMAL);
}

int Rwindow_is_maximized(Rid id) {
	RVN_WINDOW(window, id, R_FALSE);
	return window->state == RVN_PWINDOW_MAXIMIZED;
}

int Rwindow_is_minimized(Rid id) {
	RVN_WINDOW(window, id, R_FALSE);
	return window->state == RVN_PWINDOW_MINIMIZED;
}

int Rwindow_is_active(Rid id) {
	RVN_WINDOW(window, id, R_FALSE);
	return window->active;
}

void Rwindow_close(Rid id) {
	RVN_WINDOW_VOID(window, id);
	rvn_window_request_close(window);
}

void Rwindow_begin_move(Rid id) {
	RVN_WINDOW_VOID(window, id);
	rvn_dispatch_begin();
	rvn_window_lose_pointer(window);
	if (!window->dead) g_rvn.platform->window_begin_move(window->platform);
	rvn_dispatch_end();
}

void Rwindow_begin_resize(Rid id, RWindowEdge edge) {
	static const rvn_pregion_t regions[] = {
		RVN_PREGION_LEFT, RVN_PREGION_RIGHT, RVN_PREGION_TOP, RVN_PREGION_BOTTOM,
		RVN_PREGION_TOP_LEFT, RVN_PREGION_TOP_RIGHT, RVN_PREGION_BOTTOM_LEFT, RVN_PREGION_BOTTOM_RIGHT
	};
	RVN_WINDOW_VOID(window, id);
	if (edge < R_EDGE_LEFT || edge > R_EDGE_BOTTOM_RIGHT) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwindow_begin_resize: invalid edge");
		return;
	}
	if ((window->window_flags & R_WINDOW_NO_RESIZE) || window->state == RVN_PWINDOW_MAXIMIZED) return;
	rvn_dispatch_begin();
	rvn_window_lose_pointer(window);
	if (!window->dead) g_rvn.platform->window_begin_resize(window->platform, regions[edge - R_EDGE_LEFT]);
	rvn_dispatch_end();
}

void Rwindow_set_drop_callback(Rid id, RDropCallback callback, void* user_data) {
	RVN_WINDOW_VOID(window, id);
	window->drop_callback = callback;
	window->drop_user = user_data;
}

void Rwindow_set_clear_color(Rid id, float r, float g, float b, float a) {
	RVN_WINDOW_VOID(window, id);
	window->has_clear_color = R_TRUE;
	window->clear_color[0] = r;
	window->clear_color[1] = g;
	window->clear_color[2] = b;
	window->clear_color[3] = a;
	window->needs_paint = R_TRUE;
}

void Rwindow_set_background(Rid id, Rid texture) {
	RVN_WINDOW_VOID(window, id);
	if (texture != R_INVALID_ID && !rvn_texture_get(texture)) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rwindow_set_background: invalid texture");
		return;
	}
	window->background = texture;
	window->needs_paint = R_TRUE;
}

void Rwindow_request_redraw(Rid id) {
	if (!g_rvn.initialized) return;
	if (!Ris_ui_thread()) {
		/* the window table belongs to the UI thread: ask for every window instead */
		rvn_atomic_store(&g_rvn.redraw_all, 1);
		rvn_wake();
		return;
	}
	{
		rvn_window_t* window = rvn_window_get(id);
		if (window) rvn_atomic_store(&window->redraw_requested, 1);
	}
}

void Rwindow_set_continuous_redraw(Rid id, int enabled) {
	RVN_WINDOW_VOID(window, id);
	window->continuous = enabled != 0;
	if (enabled) window->needs_paint = R_TRUE;
}

void Rwindow_set_close_callback(Rid id, RWindowCloseCallback callback, void* user_data) {
	RVN_WINDOW_VOID(window, id);
	window->close_callback = callback;
	window->close_user = user_data;
}

void Rwindow_set_resize_callback(Rid id, RWindowResizeCallback callback, void* user_data) {
	RVN_WINDOW_VOID(window, id);
	window->resize_callback = callback;
	window->resize_user = user_data;
}

void Rwindow_set_user_handle(Rid id, void* handle) {
	RVN_WINDOW_VOID(window, id);
	window->user_handle = handle;
}

void* Rwindow_get_user_handle(Rid id) {
	RVN_WINDOW(window, id, NULL);
	return window->user_handle;
}

int Rwindow_get_native_handle(Rid id, RNativeWindow* native) {
	RVN_WINDOW(window, id, R_FALSE);
	if (!native) return R_FALSE;
	memset(native, 0, sizeof(*native));
	g_rvn.platform->window_get_native(window->platform, native);
	return native->system != R_WINDOW_SYSTEM_NONE;
}

uint8_t* Rwindow_capture(Rid id, int* width, int* height) {
	RCapture capture;
	rvn_window_t* window;
	if (width) *width = 0;
	if (height) *height = 0;
	window = rvn_window_check(id, __func__);
	if (!window) return NULL;
	memset(&capture, 0, sizeof(capture));
	if (window->needs_layout) rvn_window_layout(window);
	rvn_window_paint(window, &capture);
	/* the capture structure lives on this stack: wait for the render thread */
	rvn_renderer_sync();
	if (width) *width = capture.width;
	if (height) *height = capture.height;
	return capture.pixels;
}

void Rwindow_get_frame_stats(Rid id, RFrameStats* stats) {
	rvn_window_t* window;
	if (!stats) return;
	memset(stats, 0, sizeof(*stats));
	window = rvn_window_check(id, __func__);
	if (window) *stats = window->stats;
}

/* ---------------------------------------------------------------------------
 *  Public API: input state and synthetic input
 * ------------------------------------------------------------------------- */

Rid Rwindow_get_focused_widget(Rid id) {
	RVN_WINDOW(window, id, R_INVALID_ID);
	return window->focus ? window->focus->id : R_INVALID_ID;
}

Rid Rwindow_get_hovered_widget(Rid id) {
	RVN_WINDOW(window, id, R_INVALID_ID);
	return window->hover ? window->hover->id : R_INVALID_ID;
}

void Rwindow_get_mouse_position(Rid id, float* x, float* y) {
	rvn_window_t* window;
	if (x) *x = 0.0f;
	if (y) *y = 0.0f;
	window = rvn_window_check(id, __func__);
	if (!window) return;
	if (x) *x = window->mouse_x;
	if (y) *y = window->mouse_y;
}

/* Synthetic input acts on the current layout, like real input does. */
static rvn_window_t* rvn_window_inject_begin(Rid id, const char* function) {
	rvn_window_t* window = rvn_window_check(id, function);
	if (!window) return NULL;
	if (window->needs_layout) rvn_window_layout(window);
	rvn_dispatch_begin();
	return window;
}

void Rwindow_inject_mouse_move(Rid id, float x, float y) {
	rvn_window_t* window = rvn_window_inject_begin(id, __func__);
	if (!window) return;
	rvn_window_mouse_move(window, x, y);
	rvn_dispatch_end();
}

void Rwindow_inject_mouse_button(Rid id, RMouseButton button, int pressed) {
	rvn_window_t* window = rvn_window_inject_begin(id, __func__);
	if (!window) return;
	rvn_window_mouse_button(window, button, pressed);
	rvn_dispatch_end();
}

void Rwindow_inject_mouse_wheel(Rid id, float wheel_x, float wheel_y) {
	rvn_window_t* window = rvn_window_inject_begin(id, __func__);
	if (!window) return;
	rvn_window_mouse_wheel(window, wheel_x, wheel_y);
	rvn_dispatch_end();
}

void Rwindow_inject_key(Rid id, RKey key, int pressed, Ruint modifiers) {
	rvn_window_t* window = rvn_window_inject_begin(id, __func__);
	if (!window) return;
	rvn_window_key(window, key, pressed, modifiers, R_FALSE);
	rvn_dispatch_end();
}

void Rwindow_inject_text(Rid id, const char* text) {
	rvn_window_t* window = rvn_window_inject_begin(id, __func__);
	const char* cursor = text;
	const char* end;
	if (!window) return;
	end = text ? text + strlen(text) : NULL;
	while (text && cursor < end && !window->dead) {
		rvn_window_text(window, rvn_utf8_decode(&cursor, end));
	}
	rvn_dispatch_end();
}

int Rwindow_inject_drag(Rid id, float x, float y, int inside) {
	rvn_window_t* window = rvn_window_inject_begin(id, __func__);
	int accepted;
	if (!window) return R_FALSE;
	accepted = rvn_window_drag(window, x, y, inside);
	rvn_dispatch_end();
	return accepted;
}

int Rwindow_inject_drop(Rid id, float x, float y, const char* const* paths, int path_count, const char* text) {
	rvn_window_t* window = rvn_window_inject_begin(id, __func__);
	int taken;
	if (!window) return R_FALSE;
	taken = rvn_window_drop(window, x, y, path_count > 0 ? paths : NULL, path_count > 0 ? path_count : 0, text);
	rvn_dispatch_end();
	return taken;
}

void Rwindow_register_shortcut(Rid id, RKey key, Ruint modifiers, const char* action) {
	RVN_WINDOW_VOID(window, id);
	if (!action || !action[0]) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rwindow_register_shortcut: empty action name");
		return;
	}
	rvn_window_add_shortcut(window, key, modifiers & 0xFu, action);
}

void Rwindow_unregister_shortcut(Rid id, RKey key, Ruint modifiers) {
	RVN_WINDOW_VOID(window, id);
	rvn_window_remove_shortcut(window, key, modifiers & 0xFu);
}

void Rwindow_close_popups(Rid id) {
	RVN_WINDOW_VOID(window, id);
	rvn_window_close_popups(window);
}

/* ---------------------------------------------------------------------------
 *  Raw input state
 * ------------------------------------------------------------------------- */

int Rwindow_is_key_down(Rid id, RKey key) {
	rvn_window_t* window = rvn_window_check(id, "Rwindow_is_key_down");
	if (!window || (int)key <= 0 || (int)key >= R_KEY_COUNT) return R_FALSE;
	return window->keys[key] ? R_TRUE : R_FALSE;
}

int Rwindow_is_mouse_button_down(Rid id, RMouseButton button) {
	rvn_window_t* window = rvn_window_check(id, "Rwindow_is_mouse_button_down");
	if (!window || (int)button < 0 || (int)button > R_MOUSE_X2) return R_FALSE;
	return (window->buttons & (1u << button)) ? R_TRUE : R_FALSE;
}

Ruint Rwindow_get_modifiers(Rid id) {
	rvn_window_t* window = rvn_window_check(id, "Rwindow_get_modifiers");
	return window ? window->modifiers : 0;
}

int Rwindow_is_mouse_inside(Rid id) {
	rvn_window_t* window = rvn_window_check(id, "Rwindow_is_mouse_inside");
	return window ? window->mouse_inside : R_FALSE;
}

void Rwindow_set_tab_navigation(Rid id, int enabled) {
	rvn_window_t* window = rvn_window_check(id, "Rwindow_set_tab_navigation");
	if (window) window->tab_navigation = enabled ? R_TRUE : R_FALSE;
}

void Rwindow_set_event_hook(Rid id, RWindowEventHook hook, void* user_data) {
	rvn_window_t* window = rvn_window_check(id, "Rwindow_set_event_hook");
	if (!window) return;
	window->event_hook = hook;
	window->event_hook_user = user_data;
}

int Rwindow_has_popup(Rid id) {
	rvn_window_t* window = rvn_window_check(id, "Rwindow_has_popup");
	return window && window->popup_count > 0 ? R_TRUE : R_FALSE;
}

int Rwindow_get_popup_count(Rid id) {
	rvn_window_t* window = rvn_window_check(id, "Rwindow_get_popup_count");
	return window ? (int)window->popup_count : 0;
}

Rid Rwindow_get_popup(Rid id, int index) {
	rvn_window_t* window = rvn_window_check(id, "Rwindow_get_popup");
	if (!window || index < 0 || (uint32_t)index >= window->popup_count || !window->popups[index].root) return R_INVALID_ID;
	return window->popups[index].root->id;
}

int Rwindow_wants_text_input(Rid id) {
	rvn_window_t* window = rvn_window_check(id, "Rwindow_wants_text_input");
	RWidgetType type;
	if (!window || !window->focus || (window->focus->flags & RVN_WF_DEAD)) return R_FALSE;
	type = window->focus->cls->type;
	return type == R_WIDGET_TEXT_INPUT || type == R_WIDGET_TEXT_EDITOR || type == R_WIDGET_SPIN_BOX ? R_TRUE : R_FALSE;
}

int Rwindow_get_text_input_rect(Rid id, int* x, int* y, int* width, int* height) {
	rvn_window_t* window;
	rvn_box_t box;
	if (x) *x = 0;
	if (y) *y = 0;
	if (width) *width = 0;
	if (height) *height = 0;
	window = rvn_window_check(id, "Rwindow_get_text_input_rect");
	if (!window || !window->active || !window->visible || !window->focus ||
		!rvn_widget_is_enabled(window->focus) || window->focus->id != window->text_input_owner) return R_FALSE;
	box = window->text_input_rect;
	if (rvn_box_empty(box)) return R_FALSE;
	if (x) *x = (int)floorf(box.x0);
	if (y) *y = (int)floorf(box.y0);
	if (width) *width = (int)(ceilf(box.x1) - floorf(box.x0));
	if (height) *height = (int)(ceilf(box.y1) - floorf(box.y0));
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Drag and drop inside the application
 * ------------------------------------------------------------------------- */

int Rdrag_begin(Rid source, const char* type, const char* data, const char* label) {
	rvn_widget_t* widget;
	rvn_window_t* window;
	if (!rvn_api_check("Rdrag_begin")) return R_FALSE;
	widget = rvn_widget_get(source);
	if (!widget) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rdrag_begin: invalid widget handle 0x%08X", (unsigned)source);
		return R_FALSE;
	}
	window = widget->window;
	if (!window || window->dead) return R_FALSE;
	if (window->drag_active) rvn_window_drag_cancel(window);
	window->drag_active = R_TRUE;
	window->drag_type = rvn_strdup(type ? type : "");
	window->drag_data = rvn_strdup(data ? data : "");
	window->drag_label = rvn_strdup(label ? label : "");
	window->drag_source = source;
	/* the widget that started it no longer follows the pointer */
	window->capture = NULL;
	rvn_window_tooltip_hide(window);
	rvn_window_drag(window, window->mouse_x, window->mouse_y, R_TRUE);
	window->needs_paint = R_TRUE;
	return R_TRUE;
}

int Rdrag_is_active(Rid id) {
	rvn_window_t* window = rvn_window_check(id, "Rdrag_is_active");
	return window ? window->drag_active : R_FALSE;
}

const char* Rdrag_get_type(Rid id) {
	rvn_window_t* window = rvn_window_check(id, "Rdrag_get_type");
	return window && window->drag_active ? window->drag_type : NULL;
}

const char* Rdrag_get_data(Rid id) {
	rvn_window_t* window = rvn_window_check(id, "Rdrag_get_data");
	return window && window->drag_active ? window->drag_data : NULL;
}

void Rdrag_cancel(Rid id) {
	rvn_window_t* window = rvn_window_check(id, "Rdrag_cancel");
	if (!window) return;
	rvn_dispatch_begin();
	rvn_window_drag_cancel(window);
	rvn_dispatch_end();
}

void Rwindow_set_icon(Rid id, const uint8_t* rgba, int width, int height) {
	rvn_window_t* window = rvn_window_check(id, "Rwindow_set_icon");
	if (!window || !window->platform || !g_rvn.platform->window_set_icon) return;
	g_rvn.platform->window_set_icon(window->platform, rgba, width, height);
}

/* ---------------------------------------------------------------------------
 *  Scale of the interface
 * ------------------------------------------------------------------------- */

void Rset_vsync(int enabled) {
	uint32_t i;
	RVN_API_GUARD_VOID();
	enabled = enabled ? R_TRUE : R_FALSE;
	if (enabled == g_rvn.vsync) return;
	g_rvn.vsync = enabled;
	for (i = 0; i < g_rvn.windows.used; i++) {
		rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (window) Rwindow_request_redraw(window->id);
	}
}

int Rget_vsync(void) {
	return g_rvn.vsync;
}

void Rset_ui_scale(float scale) {
	uint32_t i;
	RVN_API_GUARD_VOID();
	if (!(scale > 0.0f)) scale = 1.0f;
	scale = rvn_clampf(scale, 0.25f, 4.0f);
	if (scale == g_rvn.ui_scale) return;
	g_rvn.ui_scale = scale;
	for (i = 0; i < g_rvn.windows.used; i++) {
		rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		float platform_scale;
		if (!window || window->dead || !window->platform) continue;
		platform_scale = g_rvn.platform->window_get_scale(window->platform);
		window->scale = (platform_scale > 0.0f ? platform_scale : 1.0f) * g_rvn.ui_scale;
		rvn_window_close_popups(window);
	}
	rvn_invalidate_all_windows();
	/* the glyphs are rasterized for the new sizes */
	rvn_repaint_all_windows();
}

float Rget_ui_scale(void) {
	return g_rvn.ui_scale > 0.0f ? g_rvn.ui_scale : 1.0f;
}

int rvn_window_pointer(const struct rvn_window_t* window, float* x, float* y, uint32_t* buttons) {
	*x = window->mouse_x;
	*y = window->mouse_y;
	*buttons = window->buttons;
	return window->mouse_inside && !window->dead;
}

void rvn_windows_throttle(void) {
	uint32_t i;
	for (i = 0; i < g_rvn.windows.used; i++) {
		rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (window && rvn_window_wants_frame(window)) rvn_renderer_throttle(&window->frames_in_flight, RVN_MAX_FRAMES_IN_FLIGHT);
	}
}
