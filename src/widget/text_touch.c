/**
 * Raven - widget/text_touch.c
 * Text selected with a finger, as phones do it: a long press selects the
 * word under the finger (the finger then drags the end of the selection), a
 * double tap selects a word, handles at both ends move the selection, the
 * caret is dragged (iOS) or its handle is (Android), and the edit menu of the
 * system (Cut, Copy, Paste, Select All) floats next to the selection.
 *
 * Only where the platform has an edit menu (rvn_platform_t::edit_menu_show:
 * iOS, Android); elsewhere a finger stays a mouse and a long press a right
 * click (widget/touch.c), and nothing here runs.
 *
 * Who does what:
 *  - widget/touch.c tells what the finger does (rvn_text_touch_press, _drag,
 *    _release, _long_press, _before_tap / _after_tap, _scrolling). A tap is
 *    still the click of the mouse (the caret, a double click selects the
 *    word): only what it left decides the handles and the menu.
 *  - The text widgets (TextInput, TextEditor) give their selection through
 *    rvn_widget_class_t::text_touch, and tell where its ends were painted
 *    (rvn_text_touch_ends): the handles follow the text when it scrolls.
 *  - The handles are drawn here, over the widgets (rvn_text_touch_paint), in
 *    the look of the system: a bar with a dot at each end on iOS, drops on
 *    Android (and the drop under the caret, which goes after a few seconds).
 *  - The menu is the system's (edit_menu_show, after the next paint, where
 *    the selection now is); its entries come back as RVN_PEVENT_EDIT and are
 *    the keys of the widget (Ctrl+X, C, V, A): Raven's clipboard and the
 *    widget's own commands (undo, the changed callbacks).
 * The handles and the menu go when the selection changes otherwise (typing),
 * the focus leaves the field, or a tap lands elsewhere.
 *
 * SHORTCUT: only the main cursor of a TextEditor; a word is what a double
 * click selects (no dictionary of the system); handles do not cross (the end
 * dragged stops at the other); the selection does not scroll the text by
 * itself when a handle is held at the edge of the field (the caret is kept in
 * view as the end moves); the menu does not follow content that coasts after
 * a flick (it comes back where the selection was when the finger left).
 */

#include "widget.h"

enum { RVN_TT_NONE, RVN_TT_CARET, RVN_TT_SELECTION };
enum { RVN_TT_DRAG_NONE, RVN_TT_DRAG_START, RVN_TT_DRAG_END, RVN_TT_DRAG_CARET, RVN_TT_DRAG_WORDS };

/* Android: the drop under a caret goes when it was not touched for this long */
#define RVN_TT_CARET_HANDLE_TIME 4.0
/* Android (Material): the drop of a handle, 22 dp wide */
#define RVN_TT_DROP 11.0f
/* iOS: the dot at the end of a handle */
#define RVN_TT_DOT 5.5f

struct rvn_text_touch_t {
	/** The text widget the handles are for (focused), or NULL: no handles, no menu. */
	rvn_widget_t* widget;
	int mode;
	/** What the finger holds (RVN_TT_DRAG_*), and whether it moved since. */
	int drag;
	int moved;
	/** From the finger to the point of the text the handle holds (it stays at that distance). */
	float grab_dx, grab_dy;
	/** Long press: where the finger was (the content may move under it as the field is revealed: only the finger counts). */
	float press_x, press_y;
	/** The selection this left: when it is otherwise (typing), the handles go. */
	rvn_text_pos_t anchor, caret;
	/** Long press: the word it selected first (the selection grows from it). */
	rvn_text_pos_t word_from, word_to;
	/** Where the start and the end were painted: the line of the caret there, in units of the window. */
	rvn_rect_t ends[2];
	int end_shown[2];
	int painted;
	/** The menu: to be shown after the next paint, hidden while content scrolls, the entries shown now (0: none). */
	int menu_pending;
	int menu_paused;
	/** Where the ends were at the last frame the menu waited for (it shows once they stay put). */
	float menu_place[4];
	unsigned menu_items;
	/** Android: when the drop under the caret was last of use. */
	double caret_time;
	/* what a tap found, before its click */
	rvn_widget_t* tap_widget;
	rvn_text_pos_t tap_anchor, tap_caret;
	int tap_mode;
	int tap_menu;
	/** A finger on the caret of iOS that has not moved yet: it drags the caret if it does (rvn_text_touch_grab). */
	int grab_armed;
};

static int g_tt_forced;

int rvn_text_touch_enabled(void) {
	if (g_tt_forced) return R_TRUE;
	return g_rvn.platform && g_rvn.platform->edit_menu_show != NULL;
}

void rvn_text_touch_force(int mode) {
	g_tt_forced = mode;
}

/* iOS: bars with dots, the caret dragged itself; elsewhere (Android) drops */
static int rvn_tt_apple(void) {
	if (g_tt_forced) return g_tt_forced == 2;
	return g_rvn.platform && g_rvn.platform->system == R_WINDOW_SYSTEM_UIKIT;
}

static int rvn_tt_cmp(rvn_text_pos_t a, rvn_text_pos_t b) {
	if (a.line != b.line) return a.line < b.line ? -1 : 1;
	return a.column < b.column ? -1 : a.column > b.column ? 1 : 0;
}

static rvn_text_pos_t rvn_tt_min(rvn_text_pos_t a, rvn_text_pos_t b) { return rvn_tt_cmp(a, b) <= 0 ? a : b; }
static rvn_text_pos_t rvn_tt_max(rvn_text_pos_t a, rvn_text_pos_t b) { return rvn_tt_cmp(a, b) >= 0 ? a : b; }

static struct rvn_text_touch_t* rvn_tt(rvn_window_t* window) {
	if (!window->text_touch) window->text_touch = (struct rvn_text_touch_t*)rvn_calloc(sizeof(struct rvn_text_touch_t));
	return window->text_touch;
}

/* The text widget a finger touches: the target itself (TextInput and TextEditor have no children). */
static rvn_widget_t* rvn_tt_text_widget(rvn_widget_t* target) {
	if (!target || (target->flags & RVN_WF_DEAD) || !target->cls->text_touch) return NULL;
	return rvn_widget_is_enabled(target) && rvn_widget_is_shown(target) ? target : NULL;
}

/* A point of the window, in the widget (as widget/touch.c finds it). */
static void rvn_tt_local(const rvn_widget_t* widget, float x, float y, float* local_x, float* local_y) {
	rvn_vec2_t origin = rvn_widget_origin(widget);
	rvn_widget_to_virtual(widget, &x, &y);
	*local_x = x - origin.x;
	*local_y = y - origin.y;
}

/* (the widget measures its text with the font of its part of the tree, as it does for a click) */
static rvn_text_pos_t rvn_tt_position_at(rvn_widget_t* widget, float x, float y) {
	rvn_text_pos_t pos;
	float lx, ly;
	Rid previous = rvn_font_scope_enter(widget->font_scope);
	rvn_tt_local(widget, x, y, &lx, &ly);
	pos = widget->cls->text_touch->position_at(widget, lx, ly);
	rvn_font_scope_leave(previous);
	return pos;
}

static void rvn_tt_word_at(rvn_widget_t* widget, rvn_text_pos_t pos, rvn_text_pos_t* from, rvn_text_pos_t* to) {
	Rid previous = rvn_font_scope_enter(widget->font_scope);
	widget->cls->text_touch->word_at(widget, pos, from, to);
	rvn_font_scope_leave(previous);
}

static void rvn_tt_select(struct rvn_text_touch_t* tt, rvn_text_pos_t anchor, rvn_text_pos_t caret) {
	Rid previous = rvn_font_scope_enter(tt->widget->font_scope);
	tt->widget->cls->text_touch->set_selection(tt->widget, anchor, caret);
	rvn_font_scope_leave(previous);
	tt->widget->cls->text_touch->get_selection(tt->widget, &tt->anchor, &tt->caret);
	tt->mode = rvn_tt_cmp(tt->anchor, tt->caret) != 0 ? RVN_TT_SELECTION : RVN_TT_CARET;
	rvn_widget_invalidate_paint(tt->widget);
}

/* ---------------------------------------------------------------------------
 *  The menu of the system
 * ------------------------------------------------------------------------- */

static unsigned rvn_tt_items(const struct rvn_text_touch_t* tt) {
	unsigned state = tt->widget->cls->text_touch->state(tt->widget), items = 0;
	if (tt->mode == RVN_TT_SELECTION && !(state & RVN_TEXT_TOUCH_SECRET)) {
		if (state & RVN_TEXT_TOUCH_EDITABLE) items |= RVN_EDIT_CUT;
		items |= RVN_EDIT_COPY;
	}
	if ((state & RVN_TEXT_TOUCH_EDITABLE) && Rclipboard_has_text()) items |= RVN_EDIT_PASTE;
	if (!(state & RVN_TEXT_TOUCH_EMPTY) && !(state & RVN_TEXT_TOUCH_ALL)) items |= RVN_EDIT_SELECT_ALL;
	return items;
}

static void rvn_tt_menu_hide(rvn_window_t* window, struct rvn_text_touch_t* tt) {
	tt->menu_pending = R_FALSE;
	if (tt->menu_items && g_rvn.platform->edit_menu_hide && window->platform) g_rvn.platform->edit_menu_hide(window->platform);
	tt->menu_items = 0;
}

/*
 * After a paint: next to where the selection (or the caret) is now. Not
 * while it is out of sight or still moving (the field is being revealed
 * above the keyboard that just came): it waits for the frames after.
 */
static void rvn_tt_menu_show(rvn_window_t* window, struct rvn_text_touch_t* tt, RPainter* painter) {
	rvn_rect_t a = tt->ends[0], b = tt->ends[1];
	float x0, y0, x1, y1, scale = window->scale > 0.0f ? window->scale : 1.0f;
	int rect[4];
	unsigned items;
	if (tt->mode == RVN_TT_CARET || !tt->end_shown[1]) b = a;
	if (!tt->end_shown[0]) a = b;
	if (!tt->end_shown[0] && !tt->end_shown[1]) {
		tt->menu_place[0] = -1e30f;
		return;
	}
	if (a.x != tt->menu_place[0] || a.y != tt->menu_place[1] || b.x != tt->menu_place[2] || b.y != tt->menu_place[3]) {
		tt->menu_place[0] = a.x;
		tt->menu_place[1] = a.y;
		tt->menu_place[2] = b.x;
		tt->menu_place[3] = b.y;
		rvn_paint_keep_animating(painter);
		return;
	}
	tt->menu_pending = R_FALSE;
	items = rvn_tt_items(tt);
	if (!items) {
		rvn_tt_menu_hide(window, tt);
		return;
	}
	x0 = rvn_minf(a.x, b.x);
	x1 = rvn_maxf(a.x, b.x);
	y0 = rvn_minf(a.y, b.y);
	y1 = rvn_maxf(a.y + a.h, b.y + b.h);
	/* (a selection over several lines: the menu is over its middle) */
	if (b.y > a.y + a.h * 0.5f && tt->widget) {
		rvn_vec2_t origin = rvn_widget_origin(tt->widget);
		x0 = origin.x;
		x1 = origin.x + tt->widget->rect.w;
	}
	rect[0] = (int)floorf(x0 * scale);
	rect[1] = (int)floorf(y0 * scale);
	rect[2] = (int)ceilf((x1 - x0) * scale);
	rect[3] = (int)ceilf((y1 - y0) * scale);
	if (g_rvn.platform->edit_menu_show && window->platform) g_rvn.platform->edit_menu_show(window->platform, rect, items);
	tt->menu_items = items;
}

static void rvn_tt_menu_toggle(struct rvn_text_touch_t* tt, rvn_window_t* window) {
	if (tt->menu_items || tt->menu_pending) rvn_tt_menu_hide(window, tt);
	else tt->menu_pending = R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  The handles
 * ------------------------------------------------------------------------- */

void rvn_text_touch_end(rvn_window_t* window) {
	struct rvn_text_touch_t* tt = window->text_touch;
	if (!tt) return;
	rvn_tt_menu_hide(window, tt);
	if (tt->widget && !(tt->widget->flags & RVN_WF_DEAD)) rvn_widget_invalidate_paint(tt->widget);
	window->needs_paint = R_TRUE;
	tt->widget = NULL;
	tt->mode = RVN_TT_NONE;
	tt->drag = RVN_TT_DRAG_NONE;
	tt->menu_paused = R_FALSE;
	tt->grab_armed = R_FALSE;
	tt->painted = R_FALSE;
}

/* The handles are for `widget` now, as it is. */
static struct rvn_text_touch_t* rvn_tt_begin(rvn_window_t* window, rvn_widget_t* widget) {
	struct rvn_text_touch_t* tt = rvn_tt(window);
	if (tt->widget && tt->widget != widget) rvn_text_touch_end(window);
	if (tt->widget != widget) {
		tt->widget = widget;
		tt->painted = R_FALSE;
		tt->end_shown[0] = tt->end_shown[1] = R_FALSE;
	}
	widget->cls->text_touch->get_selection(widget, &tt->anchor, &tt->caret);
	tt->mode = rvn_tt_cmp(tt->anchor, tt->caret) != 0 ? RVN_TT_SELECTION : RVN_TT_CARET;
	tt->drag = RVN_TT_DRAG_NONE;
	tt->caret_time = rvn_clock_seconds();
	rvn_widget_invalidate_paint(widget);
	return tt;
}

/* R_TRUE while the handles still belong to the focused field as the finger left it; otherwise they go. */
static int rvn_tt_check(rvn_window_t* window) {
	struct rvn_text_touch_t* tt = window->text_touch;
	rvn_widget_t* widget = tt ? tt->widget : NULL;
	rvn_text_pos_t anchor, caret;
	if (!widget) return R_FALSE;
	if ((widget->flags & RVN_WF_DEAD) || window->focus != widget || !rvn_widget_is_shown(widget) || !rvn_widget_is_enabled(widget)) {
		rvn_text_touch_end(window);
		return R_FALSE;
	}
	widget->cls->text_touch->get_selection(widget, &anchor, &caret);
	if (rvn_tt_cmp(anchor, tt->anchor) != 0 || rvn_tt_cmp(caret, tt->caret) != 0) {
		/* typed, moved with the keys, changed by the application */
		rvn_text_touch_end(window);
		return R_FALSE;
	}
	return R_TRUE;
}

static int rvn_tt_caret_handle_shown(const struct rvn_text_touch_t* tt) {
	if (tt->mode != RVN_TT_CARET || !tt->end_shown[0] || rvn_tt_apple()) return R_FALSE;
	return tt->drag == RVN_TT_DRAG_CARET || rvn_clock_seconds() - tt->caret_time < RVN_TT_CARET_HANDLE_TIME;
}

/* The middle of a handle (what a finger takes), in units of the window. */
static rvn_vec2_t rvn_tt_handle_center(const struct rvn_text_touch_t* tt, int end) {
	rvn_rect_t line = tt->ends[end];
	if (rvn_tt_apple()) {
		/* (the dot is over the start, under the end) */
		return rvn_vec2(line.x, end == 0 ? line.y + line.h * 0.5f - RVN_TT_DOT : line.y + line.h * 0.5f + RVN_TT_DOT);
	}
	if (tt->mode == RVN_TT_CARET) return rvn_vec2(line.x, line.y + line.h + RVN_TT_DROP * 1.4142f);
	return rvn_vec2(end == 0 ? line.x - RVN_TT_DROP : line.x + RVN_TT_DROP, line.y + line.h + RVN_TT_DROP);
}

/* The handle under a finger: 0 the start (or the caret), 1 the end, -1 none. */
static int rvn_tt_hit(rvn_window_t* window, const struct rvn_text_touch_t* tt, float x, float y) {
	float size = rvn_maxf(rvn_touch_min_size(window), 40.0f), best_distance = 1e30f;
	int best = -1, end;
	for (end = 0; end < 2; end++) {
		rvn_vec2_t center;
		float half_h = size * 0.5f, dx, dy;
		if (!tt->end_shown[end]) continue;
		if (tt->mode == RVN_TT_CARET && (end == 1 || !rvn_tt_caret_handle_shown(tt))) continue;
		if (tt->mode != RVN_TT_CARET && tt->mode != RVN_TT_SELECTION) continue;
		center = rvn_tt_handle_center(tt, end);
		/* (the bar of iOS is as tall as the line) */
		if (rvn_tt_apple()) half_h = rvn_maxf(half_h, tt->ends[end].h * 0.5f + RVN_TT_DOT * 2.0f);
		dx = x - center.x;
		dy = y - center.y;
		if (fabsf(dx) > size * 0.5f || fabsf(dy) > half_h) continue;
		if (dx * dx + dy * dy < best_distance) {
			best_distance = dx * dx + dy * dy;
			best = end;
		}
	}
	return best;
}

/* iOS: a finger on the caret (it drags it when it moves; a tap there is the menu). */
static int rvn_tt_on_caret(rvn_window_t* window, const struct rvn_text_touch_t* tt, float x, float y) {
	float half = rvn_maxf(rvn_touch_min_size(window), 40.0f) * 0.5f;
	rvn_rect_t line = tt->ends[0];
	if (tt->mode != RVN_TT_CARET || !tt->end_shown[0] || !rvn_tt_apple()) return R_FALSE;
	return fabsf(x - line.x) <= half && y >= line.y - 8.0f && y <= line.y + line.h + 8.0f;
}

int rvn_text_touch_press(rvn_window_t* window, float x, float y) {
	struct rvn_text_touch_t* tt = window->text_touch;
	int end;
	rvn_vec2_t hold;
	if (!rvn_text_touch_enabled() || !tt || !tt->widget) return 0;
	tt->grab_armed = R_FALSE;
	tt->drag = RVN_TT_DRAG_NONE;
	if (!rvn_tt_check(window)) return 0;
	end = rvn_tt_hit(window, tt, x, y);
	if (end < 0) {
		if (!rvn_tt_on_caret(window, tt, x, y)) return 0;
		/* (until it moves it may still be a tap, or the first of a double tap) */
		tt->grab_armed = R_TRUE;
		tt->grab_dx = tt->ends[0].x - x;
		tt->grab_dy = tt->ends[0].y + tt->ends[0].h * 0.5f - y;
		return 2;
	}
	tt->drag = tt->mode == RVN_TT_CARET ? RVN_TT_DRAG_CARET : end == 0 ? RVN_TT_DRAG_START : RVN_TT_DRAG_END;
	tt->moved = R_FALSE;
	/* the finger keeps the point of the text the handle hangs from where it was: at the same distance */
	hold = rvn_vec2(tt->ends[end].x, tt->ends[end].y + tt->ends[end].h * 0.5f);
	tt->grab_dx = hold.x - x;
	tt->grab_dy = hold.y - y;
	tt->caret_time = rvn_clock_seconds();
	return 1;
}

int rvn_text_touch_grab(rvn_window_t* window) {
	struct rvn_text_touch_t* tt = window->text_touch;
	if (!tt || !tt->grab_armed) return R_FALSE;
	tt->grab_armed = R_FALSE;
	if (!rvn_tt_check(window)) return R_FALSE;
	tt->drag = RVN_TT_DRAG_CARET;
	tt->moved = R_FALSE;
	return R_TRUE;
}

void rvn_text_touch_drag(rvn_window_t* window, float x, float y) {
	struct rvn_text_touch_t* tt = window->text_touch;
	rvn_text_pos_t pos, from, to;
	if (!tt || !tt->widget || tt->drag == RVN_TT_DRAG_NONE) return;
	if ((tt->widget->flags & RVN_WF_DEAD) || window->focus != tt->widget) {
		rvn_text_touch_end(window);
		return;
	}
	if (tt->drag == RVN_TT_DRAG_WORDS && !tt->moved) {
		/* (until the finger itself moves: the field may scroll under it, revealed above the keyboard) */
		float dx = x - tt->press_x, dy = y - tt->press_y;
		if (dx * dx + dy * dy < 64.0f) return;
	}
	tt->moved = R_TRUE;
	tt->caret_time = rvn_clock_seconds();
	/* (the menu would hide the text under the finger) */
	rvn_tt_menu_hide(window, tt);
	if (tt->drag == RVN_TT_DRAG_WORDS) {
		pos = rvn_tt_position_at(tt->widget, x, y);
		rvn_tt_word_at(tt->widget, pos, &from, &to);
		if (rvn_tt_cmp(from, to) == 0) from = to = pos;
		if (rvn_tt_cmp(pos, tt->word_from) < 0) rvn_tt_select(tt, tt->word_to, from);
		else if (rvn_tt_cmp(pos, tt->word_to) > 0) rvn_tt_select(tt, tt->word_from, to);
		else rvn_tt_select(tt, tt->word_from, tt->word_to);
		return;
	}
	pos = rvn_tt_position_at(tt->widget, x + tt->grab_dx, y + tt->grab_dy);
	if (tt->drag == RVN_TT_DRAG_CARET) {
		rvn_tt_select(tt, pos, pos);
	} else if (tt->drag == RVN_TT_DRAG_START) {
		rvn_text_pos_t other = rvn_tt_max(tt->anchor, tt->caret);
		if (rvn_tt_cmp(pos, other) < 0) rvn_tt_select(tt, other, pos);
	} else {
		rvn_text_pos_t other = rvn_tt_min(tt->anchor, tt->caret);
		if (rvn_tt_cmp(pos, other) > 0) rvn_tt_select(tt, other, pos);
	}
}

void rvn_text_touch_release(rvn_window_t* window, float x, float y, int cancelled) {
	struct rvn_text_touch_t* tt = window->text_touch;
	int drag, moved;
	RVN_UNUSED(x);
	RVN_UNUSED(y);
	if (!tt || tt->drag == RVN_TT_DRAG_NONE) return;
	drag = tt->drag;
	moved = tt->moved;
	tt->drag = RVN_TT_DRAG_NONE;
	tt->caret_time = rvn_clock_seconds();
	if (!rvn_tt_check(window) || cancelled) return;
	if (drag == RVN_TT_DRAG_WORDS || (moved && tt->mode == RVN_TT_SELECTION)) {
		/* the menu, where the selection now is */
		tt->menu_pending = R_TRUE;
	} else if (!moved) {
		/* a tap on a handle (on Android the drop under the caret): the menu, or no more of it */
		rvn_tt_menu_toggle(tt, window);
	}
	rvn_widget_invalidate_paint(tt->widget);
}

int rvn_text_touch_long_press(rvn_window_t* window, rvn_widget_t* target, float x, float y) {
	rvn_widget_t* widget;
	struct rvn_text_touch_t* tt;
	rvn_text_pos_t pos, from, to;
	if (!rvn_text_touch_enabled()) return R_FALSE;
	widget = rvn_tt_text_widget(target);
	if (!widget) return R_FALSE;
	if (window->focus != widget) rvn_widget_focus(widget);
	if (window->dead || (widget->flags & RVN_WF_DEAD) || window->focus != widget) return R_FALSE;
	pos = rvn_tt_position_at(widget, x, y);
	rvn_tt_word_at(widget, pos, &from, &to);
	if (rvn_tt_cmp(from, to) == 0) from = to = pos;
	tt = rvn_tt_begin(window, widget);
	rvn_tt_menu_hide(window, tt);
	rvn_tt_select(tt, from, to);
	tt->word_from = from;
	tt->word_to = to;
	tt->drag = RVN_TT_DRAG_WORDS;
	tt->moved = R_FALSE;
	tt->press_x = x;
	tt->press_y = y;
	/* (the tick of the system that tells the press was long enough) */
	if (g_rvn.platform->haptic) g_rvn.platform->haptic((int)R_HAPTIC_SELECTION);
	return R_TRUE;
}

void rvn_text_touch_before_tap(rvn_window_t* window) {
	struct rvn_text_touch_t* tt = window->text_touch;
	if (!tt) return;
	tt->tap_widget = NULL;
	if (!rvn_text_touch_enabled() || !tt->widget || !rvn_tt_check(window)) return;
	tt->tap_widget = tt->widget;
	tt->tap_anchor = tt->anchor;
	tt->tap_caret = tt->caret;
	tt->tap_mode = tt->mode;
	tt->tap_menu = tt->menu_items != 0 || tt->menu_pending;
}

void rvn_text_touch_after_tap(rvn_window_t* window, rvn_widget_t* target) {
	struct rvn_text_touch_t* tt;
	rvn_widget_t* widget, *before;
	rvn_text_pos_t anchor, caret;
	if (!rvn_text_touch_enabled() || window->dead) return;
	tt = rvn_tt(window);
	before = tt->tap_widget;
	tt->tap_widget = NULL;
	widget = rvn_tt_text_widget(target);
	if (!widget || window->focus != widget) {
		/* a tap elsewhere */
		if (tt->widget) rvn_text_touch_end(window);
		return;
	}
	widget->cls->text_touch->get_selection(widget, &anchor, &caret);
	if (rvn_tt_cmp(anchor, caret) != 0) {
		/* a double tap selected a word (a triple tap the line) */
		tt = rvn_tt_begin(window, widget);
		tt->menu_pending = R_TRUE;
		return;
	}
	if (before == widget && tt->tap_mode == RVN_TT_CARET && rvn_tt_cmp(caret, tt->tap_caret) == 0 && rvn_tt_cmp(anchor, tt->tap_anchor) == 0) {
		/* a tap on the caret: the menu (Paste, Select All), or no more of it */
		int menu = tt->tap_menu;
		tt = rvn_tt_begin(window, widget);
		if (menu) rvn_tt_menu_hide(window, tt);
		else tt->menu_pending = R_TRUE;
		return;
	}
	tt = rvn_tt_begin(window, widget);
	rvn_tt_menu_hide(window, tt);
}

void rvn_text_touch_scrolling(rvn_window_t* window, int scrolling) {
	struct rvn_text_touch_t* tt = window->text_touch;
	if (!tt || !tt->widget) return;
	if (scrolling) {
		if (tt->menu_items || tt->menu_pending) {
			rvn_tt_menu_hide(window, tt);
			tt->menu_paused = R_TRUE;
		}
	} else if (tt->menu_paused) {
		tt->menu_paused = R_FALSE;
		tt->menu_pending = R_TRUE;
		window->needs_paint = R_TRUE;
	}
}

void rvn_text_touch_ends(const rvn_widget_t* widget, const RPainter* painter, rvn_rect_t start, rvn_rect_t end) {
	rvn_window_t* window = widget->window;
	struct rvn_text_touch_t* tt = window ? window->text_touch : NULL;
	rvn_rect_t ends[2];
	int i;
	if (!tt || tt->widget != widget) return;
	ends[0] = start;
	ends[1] = end;
	for (i = 0; i < 2; i++) {
		/* shown when its line is in what the widget shows (not scrolled away, not under what clips it) */
		rvn_box_t box = rvn_paint_snap(painter, rvn_rect(ends[i].x, ends[i].y + ends[i].h * 0.5f, 0.0f, 0.0f));
		tt->end_shown[i] = box.x0 >= painter->clip.x0 - 1.0f && box.x0 <= painter->clip.x1 + 1.0f &&
			box.y0 >= painter->clip.y0 && box.y0 <= painter->clip.y1;
		tt->ends[i] = rvn_rect(painter->ox + ends[i].x, painter->oy + ends[i].y, 0.0f, ends[i].h);
	}
	tt->painted = R_TRUE;
}

/* Android: a drop hanging from (x, y), its point at the top-right (the start), the top-left (the end) or the top (the caret). */
static void rvn_tt_paint_drop(RPainter* painter, float x, float y, int kind, rvn_color_t color) {
	float r = RVN_TT_DROP;
	if (kind == 2) {
		float cy = y + r * 1.4142f, side = r * 0.7071f;
		rvn_paint_triangle(painter, x, y, x - side, cy - side, x + side, cy - side, color);
		rvn_paint_box(painter, rvn_rect(x - r, cy - r, r * 2.0f, r * 2.0f), r, color, 0.0f, color);
	} else {
		float radii[4];
		radii[0] = kind == 0 ? r : 0.0f;
		radii[1] = kind == 0 ? 0.0f : r;
		radii[2] = r;
		radii[3] = r;
		rvn_paint_box4(painter, rvn_rect(kind == 0 ? x - r * 2.0f : x, y, r * 2.0f, r * 2.0f), radii, color, 0.0f, color);
	}
}

/* iOS: a bar as tall as the line, with a dot over the start or under the end. */
static void rvn_tt_paint_bar(RPainter* painter, rvn_rect_t line, int end, rvn_color_t color) {
	float dot_y = end == 0 ? line.y - RVN_TT_DOT + 1.0f : line.y + line.h + RVN_TT_DOT - 1.0f;
	rvn_paint_rect(painter, rvn_rect(line.x - 1.0f, line.y, 2.0f, line.h), color);
	rvn_paint_box(painter, rvn_rect(line.x - RVN_TT_DOT, dot_y - RVN_TT_DOT, RVN_TT_DOT * 2.0f, RVN_TT_DOT * 2.0f),
		RVN_TT_DOT, color, 0.0f, color);
}

void rvn_text_touch_paint(rvn_window_t* window, RPainter* painter) {
	struct rvn_text_touch_t* tt = window->text_touch;
	rvn_color_t color;
	int i;
	if (!tt || !tt->widget || !rvn_tt_check(window)) return;
	if (!tt->painted) tt->end_shown[0] = tt->end_shown[1] = R_FALSE;
	color = rvn_widget_color(tt->widget, R_COLOR_ACCENT);
	if (tt->mode == RVN_TT_SELECTION) {
		for (i = 0; i < 2; i++) {
			if (!tt->end_shown[i]) continue;
			if (rvn_tt_apple()) rvn_tt_paint_bar(painter, tt->ends[i], i, color);
			else rvn_tt_paint_drop(painter, tt->ends[i].x, tt->ends[i].y + tt->ends[i].h, i, color);
		}
	} else if (rvn_tt_caret_handle_shown(tt)) {
		rvn_tt_paint_drop(painter, tt->ends[0].x, tt->ends[0].y + tt->ends[0].h, 2, color);
		/* (a frame when it is to go) */
		if (tt->drag != RVN_TT_DRAG_CARET) rvn_paint_wake_after(painter, RVN_TT_CARET_HANDLE_TIME - (rvn_clock_seconds() - tt->caret_time) + 0.01);
	}
	if (tt->menu_pending && !tt->menu_paused && tt->drag == RVN_TT_DRAG_NONE) rvn_tt_menu_show(window, tt, painter);
	tt->painted = R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  The entries of the menu: the keys of the widget
 * ------------------------------------------------------------------------- */

void rvn_text_touch_command(rvn_window_t* window, unsigned command) {
	struct rvn_text_touch_t* tt = window->text_touch;
	rvn_widget_t* widget;
	REvent event;
	Rid id;
	if (command == 0) {
		/* the system closed the menu (a touch elsewhere, the keyboard) */
		if (tt) tt->menu_items = 0;
		return;
	}
	widget = tt && tt->widget && rvn_tt_check(window) ? tt->widget : rvn_tt_text_widget(window->focus);
	if (!widget || window->focus != widget) return;
	if (tt) {
		/* (the system already closed it) */
		tt->menu_items = 0;
		tt->menu_pending = R_FALSE;
	}
	memset(&event, 0, sizeof(event));
	event.type = R_EVENT_KEY_DOWN;
	event.window = window->id;
	event.modifiers = R_MOD_CTRL;
	switch (command) {
		case RVN_EDIT_CUT: event.key = R_KEY_X; break;
		case RVN_EDIT_COPY: event.key = R_KEY_C; break;
		case RVN_EDIT_PASTE: event.key = R_KEY_V; break;
		case RVN_EDIT_SELECT_ALL: event.key = R_KEY_A; break;
		default: return;
	}
	id = widget->id;
	rvn_widget_send(widget, &event);
	widget = rvn_widget_get(id);
	if (window->dead || !widget || window->focus != widget || !widget->cls->text_touch) {
		if (!window->dead) rvn_text_touch_end(window);
		return;
	}
	tt = rvn_tt_begin(window, widget);
	/* after Select All, what can be done with all of it */
	if (command == RVN_EDIT_SELECT_ALL) tt->menu_pending = R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Bookkeeping
 * ------------------------------------------------------------------------- */

void rvn_text_touch_forget_widget(rvn_window_t* window, rvn_widget_t* widget) {
	struct rvn_text_touch_t* tt = window->text_touch;
	if (!tt) return;
	if (tt->tap_widget && rvn_widget_is_inside(tt->tap_widget, widget)) tt->tap_widget = NULL;
	if (tt->widget && rvn_widget_is_inside(tt->widget, widget)) {
		tt->widget = NULL;
		rvn_text_touch_end(window);
	}
}

void rvn_text_touch_window_free(rvn_window_t* window) {
	rvn_free(window->text_touch);
	window->text_touch = NULL;
}

unsigned rvn_text_touch_shown(const rvn_window_t* window, unsigned* menu) {
	const struct rvn_text_touch_t* tt = window->text_touch;
	if (menu) *menu = tt && tt->widget ? tt->menu_items : 0;
	if (!tt || !tt->widget) return 0;
	if (tt->mode == RVN_TT_SELECTION) return tt->end_shown[0] || tt->end_shown[1] ? RVN_TEXT_TOUCH_SHOWN_SELECTION : 0;
	return rvn_tt_caret_handle_shown(tt) ? RVN_TEXT_TOUCH_SHOWN_CARET : 0;
}

int rvn_text_touch_handle(const rvn_window_t* window, int end, float* x, float* y) {
	const struct rvn_text_touch_t* tt = window->text_touch;
	rvn_vec2_t center;
	if (!tt || !tt->widget || end < 0 || end > 1 || !tt->end_shown[end]) return R_FALSE;
	center = rvn_tt_handle_center(tt, end);
	*x = center.x;
	*y = center.y;
	return R_TRUE;
}
