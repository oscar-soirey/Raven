/**
 * Raven - widgets/tab_view.c
 * TabView: a bar of tabs above one visible page. Every child is a page; the
 * title of its tab is the title of its slot (Rslot_set_title).
 *
 * Documents mode (Rtab_view_set_documents, XML Documents="true") : the major
 * tabs of an application (Unreal : the level, then one tab per open asset).
 * Tabs on the left with an icon and a close button (a middle click closes
 * too) ; the slot keeps the icon (has_icon...) and dock_fixed (no close
 * button). Too many tabs : they shrink.
 */

#include "common.h"

typedef struct rvn_tab_view_t {
	rvn_widget_t base;
	int active;
	int hot;
	RTabChangedCallback callback;
	void* user_data;
	/** Left edge and width of the mark of the active tab: it slides from one tab to the next. */
	RSpring mark_x;
	RSpring mark_w;
	/* ---- documents mode ---- */
	int documents;
	int hot_close;          /* the pointer is on the close button of the hot tab */
	int press_close;        /* the close button pressed (-1 : none) */
	RTabCloseCallback close_callback;
	void* close_user;
	/** The page shown : kept when tabs before it come and go. */
	rvn_widget_t* active_page;
} rvn_tab_view_t;

#define RVN_TAB_ICON 16.0f
#define RVN_TAB_ICON_GAP 6.0f
#define RVN_TAB_CLOSE 18.0f

static void rvn_tab_view_init(rvn_widget_t* widget) {
	((rvn_tab_view_t*)widget)->hot = -1;
	((rvn_tab_view_t*)widget)->press_close = -1;
}

static int rvn_tab_documents(const rvn_widget_t* widget) {
	return ((const rvn_tab_view_t*)widget)->documents;
}

static float rvn_tab_bar_height(const rvn_widget_t* widget) {
	if (rvn_tab_documents(widget)) return rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT) + 10.0f;
	/* the segmented control of the glass themes keeps some air under it */
	return rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT) + (rvn_widget_glass(widget) ? 8.0f : 2.0f);
}

static int rvn_tab_closable(const rvn_widget_t* widget, const rvn_widget_t* child) {
	return rvn_tab_documents(widget) && !child->slot.dock_fixed;
}

static const char* rvn_tab_title(const rvn_widget_t* child) {
	return (child->slot.title && child->slot.title[0]) ? child->slot.title : "Tab";
}

static float rvn_tab_natural_width(const rvn_widget_t* widget, const rvn_widget_t* child) {
	float w = rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, rvn_tab_title(child)) +
		rvn_widget_metric(widget, R_METRIC_PADDING_X) * 2.0f + 4.0f;
	if (!rvn_tab_documents(widget)) return w;
	if (child->slot.has_icon) w += RVN_TAB_ICON + RVN_TAB_ICON_GAP;
	if (rvn_tab_closable(widget, child)) w += RVN_TAB_CLOSE + 4.0f;
	return rvn_clampf(w + 8.0f, 96.0f, 280.0f);
}

/* Documents : the tabs shrink when they do not fit. */
static float rvn_tab_width(const rvn_widget_t* widget, const rvn_widget_t* child) {
	float w = rvn_tab_natural_width(widget, child);
	if (rvn_tab_documents(widget) && widget->rect.w > 0.0f) {
		float total = 0.0f;
		uint32_t i;
		for (i = 0; i < widget->child_count; i++) total += rvn_tab_natural_width(widget, widget->children[i]);
		if (total > widget->rect.w - 8.0f) w = rvn_maxf(48.0f, w * (widget->rect.w - 8.0f) / total);
	}
	return w;
}

/* The close button of a tab (documents). */
static rvn_rect_t rvn_tab_close_rect(const rvn_widget_t* widget, float x, float w) {
	float bar = rvn_tab_bar_height(widget);
	return rvn_rect(x + w - RVN_TAB_CLOSE - 6.0f, (bar - RVN_TAB_CLOSE) * 0.5f + 1.0f, RVN_TAB_CLOSE, RVN_TAB_CLOSE);
}

static rvn_widget_t* rvn_tab_view_page(rvn_widget_t* widget) {
	rvn_tab_view_t* tabs = (rvn_tab_view_t*)widget;
	if (tabs->active < 0 || tabs->active >= (int)widget->child_count) return NULL;
	return widget->children[tabs->active];
}

static rvn_vec2_t rvn_tab_view_measure(rvn_widget_t* widget) {
	rvn_widget_t* page = rvn_tab_view_page(widget);
	rvn_vec2_t desired = page ? rvn_widget_measure_slot(page) : rvn_vec2(0.0f, 0.0f);
	float bar = 0.0f;
	uint32_t i;
	/* documents: the major tabs fill the window, they do not grow with their pages
	 * (a page bigger than the room is cut, it never pushes out of the window) */
	if (rvn_tab_documents(widget)) return rvn_vec2(0.0f, rvn_tab_bar_height(widget) + 40.0f);
	for (i = 0; i < widget->child_count; i++) bar += rvn_tab_width(widget, widget->children[i]);
	desired.x = rvn_maxf(desired.x, bar);
	desired.y += rvn_tab_bar_height(widget);
	return desired;
}

static void rvn_tab_view_arrange(rvn_widget_t* widget) {
	rvn_widget_t* page = rvn_tab_view_page(widget);
	float bar = rvn_tab_bar_height(widget);
	if (page && !rvn_widget_collapsed(page)) {
		rvn_rect_t room = rvn_rect(0.0f, bar, widget->rect.w, rvn_maxf(0.0f, widget->rect.h - bar));
		rvn_widget_arrange_slot(page, room);
		/* the page stays inside the tab view, and is cut at its edges */
		if (page->rect.x < room.x || page->rect.y < room.y || page->rect.x + page->rect.w > room.x + room.w ||
			page->rect.y + page->rect.h > room.y + room.h)
			rvn_widget_arrange(page, room.x, room.y, room.w, room.h);
		page->flags |= RVN_WF_CLIP;
	}
}

static int rvn_tab_view_child_active(rvn_widget_t* widget, rvn_widget_t* child) {
	return rvn_tab_view_page(widget) == child;
}

static void rvn_tab_view_children_changed(rvn_widget_t* widget) {
	rvn_tab_view_t* tabs = (rvn_tab_view_t*)widget;
	uint32_t i;
	/* the page shown stays shown when tabs before it come and go */
	if (tabs->active_page)
		for (i = 0; i < widget->child_count; i++)
			if (widget->children[i] == tabs->active_page) {
				tabs->active = (int)i;
				break;
			}
	if (tabs->active >= (int)widget->child_count) tabs->active = (int)widget->child_count - 1;
	if (tabs->active < 0 && widget->child_count > 0) tabs->active = 0;
	tabs->active_page = tabs->active >= 0 && tabs->active < (int)widget->child_count ? widget->children[tabs->active] : NULL;
	tabs->hot = -1;
	tabs->press_close = -1;
	rvn_widget_invalidate_paint(widget);
}

static int rvn_tab_at(const rvn_widget_t* widget, float x, float y, int* on_close) {
	float position = rvn_tab_documents(widget) ? 4.0f : 0.0f;
	uint32_t i;
	if (on_close) *on_close = R_FALSE;
	if (y < 0.0f || y >= rvn_tab_bar_height(widget)) return -1;
	for (i = 0; i < widget->child_count; i++) {
		float width = rvn_tab_width(widget, widget->children[i]);
		if (x >= position && x < position + width) {
			if (on_close && rvn_tab_closable(widget, widget->children[i]))
				*on_close = rvn_rect_contains(rvn_tab_close_rect(widget, position, width), x, y);
			return (int)i;
		}
		position += width;
	}
	return -1;
}

/* The user closes a tab : the application may keep it (R_FALSE), else the page is deleted. */
static void rvn_tab_request_close(rvn_widget_t* widget, int index) {
	rvn_tab_view_t* tabs = (rvn_tab_view_t*)widget;
	rvn_widget_t* page;
	Rid content;
	if (index < 0 || index >= (int)widget->child_count) return;
	page = widget->children[index];
	if (!rvn_tab_closable(widget, page)) return;
	content = page->id;
	if (tabs->close_callback && !tabs->close_callback(widget->id, index, content, tabs->close_user)) return;
	if (!rvn_widget_alive(widget)) return;
	if (Rwidget_is_valid(content) && Rwidget_get_parent(content) == widget->id) Rwidget_destroy(content);
}

static void rvn_tab_view_activate(rvn_widget_t* widget, int index, int notify) {
	rvn_tab_view_t* tabs = (rvn_tab_view_t*)widget;
	rvn_widget_t* previous = rvn_tab_view_page(widget);
	if (index < 0 || index >= (int)widget->child_count || index == tabs->active) return;
	if (previous && widget->window) rvn_window_forget_widget(widget->window, previous);
	/* the new page comes from the side its tab is on, or the way it was told to appear (documents : at once) */
	if (!tabs->documents) {
		rvn_widget_t* page = widget->children[index];
		if (page->fx && page->fx->appear != R_TRANSITION_NONE && page->fx->appear != 0) rvn_widget_appear(page);
		else rvn_widget_play_transition(page, index > tabs->active ? R_TRANSITION_SLIDE_LEFT : R_TRANSITION_SLIDE_RIGHT, 0.0f);
	}
	tabs->active = index;
	tabs->active_page = widget->children[index];
	rvn_widget_invalidate_layout(widget);
	if (notify) {
		if (tabs->callback) tabs->callback(widget->id, index, tabs->user_data);
		if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
	}
}

static int rvn_tab_view_event(rvn_widget_t* widget, const REvent* event) {
	rvn_tab_view_t* tabs = (rvn_tab_view_t*)widget;
	int index;
	int on_close = R_FALSE;
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			index = rvn_tab_at(widget, event->local_x, event->local_y, &on_close);
			if (index != tabs->hot || on_close != tabs->hot_close) {
				tabs->hot = index;
				tabs->hot_close = on_close;
				rvn_widget_invalidate_paint(widget);
			}
			return R_FALSE;
		case R_EVENT_MOUSE_LEAVE:
			if (tabs->hot >= 0) {
				tabs->hot = -1;
				tabs->hot_close = R_FALSE;
				rvn_widget_invalidate_paint(widget);
			}
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			index = rvn_tab_at(widget, event->local_x, event->local_y, &on_close);
			if (index < 0) return R_FALSE;
			if (tabs->documents && event->button == R_MOUSE_MIDDLE) {
				rvn_tab_request_close(widget, index);
				return R_TRUE;
			}
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			if (on_close) {
				tabs->press_close = index;
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			rvn_tab_view_activate(widget, index, R_TRUE);
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (tabs->press_close < 0 || event->button != R_MOUSE_LEFT) return R_FALSE;
			index = rvn_tab_at(widget, event->local_x, event->local_y, &on_close);
			{
				int pressed = tabs->press_close;
				tabs->press_close = -1;
				rvn_widget_invalidate_paint(widget);
				if (index == pressed && on_close) rvn_tab_request_close(widget, index);
			}
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

/* The tabs of Windows: their background, their text, the pill of the selected one. */
static void rvn_tab_fluent_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	const int documents = rvn_tab_documents(widget);
	RVN_UNUSED(context);
	switch (part) {
		case RVN_PART_TAB:
			look->radius = documents ? rvn_widget_metric(widget, R_METRIC_POPUP_RADIUS) : rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
			if (states & RVN_STATE_SELECTED) {
				look->foreground = rvn_widget_color(widget, R_COLOR_TEXT);
				if (documents) {
					/* the selected tab is a piece of the page, raised into the bar */
					look->background = rvn_widget_color(widget, R_COLOR_TAB_ACTIVE);
					look->border = rvn_fluent(widget, RVN_FLUENT_CARD_BORDER);
					look->border_width = 1.0f;
				}
			} else {
				look->foreground = (states & RVN_STATE_DISABLED) ? rvn_widget_color(widget, R_COLOR_TEXT_DISABLED) : rvn_fluent(widget, RVN_FLUENT_TEXT_SECONDARY);
				if (states & RVN_STATE_PRESSED) look->background = rvn_fluent(widget, RVN_FLUENT_SUBTLE_PRESSED);
				else if (states & RVN_STATE_HOVERED) look->background = rvn_widget_color(widget, R_COLOR_ROW_HOVERED);
				if (states & RVN_STATE_HOVERED) look->foreground = rvn_widget_color(widget, R_COLOR_TEXT);
			}
			break;
		case RVN_PART_INDICATOR:
			look->background = rvn_widget_color(widget, R_COLOR_ACCENT);
			look->size = 16.0f;
			look->radius = 1.5f;
			break;
		case RVN_PART_HEADER:
			look->border = rvn_fluent(widget, RVN_FLUENT_CARD_BORDER);
			break;
		default:
			break;
	}
}

static void rvn_tab_view_paint_fluent(rvn_widget_t* widget, RPainter* painter, float mark_x, float mark_w) {
	rvn_tab_view_t* tabs = (rvn_tab_view_t*)widget;
	float bar = rvn_tab_bar_height(widget), x = tabs->documents ? 4.0f : 0.0f;
	float size = rvn_widget_font_size(widget, 0.0f);
	rvn_part_look_t look, header;
	uint32_t i;

	rvn_part_look(widget, RVN_PART_HEADER, 0, rvn_tab_fluent_look, NULL, &header);
	if (rvn_color_a(header.background)) rvn_paint_rect(painter, rvn_rect(0.0f, 0.0f, widget->rect.w, bar), header.background);
	for (i = 0; i < widget->child_count; i++) {
		rvn_widget_t* page = widget->children[i];
		float width = rvn_tab_width(widget, page);
		int active = (int)i == tabs->active;
		int hot = (int)i == tabs->hot && (widget->flags & RVN_WF_HOVERED) && painter->enabled;
		uint32_t states = rvn_widget_states(widget, painter, hot, R_FALSE) | (active ? RVN_STATE_SELECTED : 0);
		float tx;
		rvn_part_look(widget, RVN_PART_TAB, states & ~RVN_STATE_FOCUSED, rvn_tab_fluent_look, NULL, &look);
		if (tabs->documents) {
			/* TabView: the selected tab opens onto the page; a short line between the others */
			rvn_rect_t tab = rvn_rect(x, 6.0f, width - 2.0f, bar - 6.0f);
			int closable = rvn_tab_closable(widget, page);
			float radii[4];
			radii[0] = radii[1] = look.radius;
			radii[2] = radii[3] = 0.0f;
			if (active) {
				rvn_paint_box4(painter, rvn_rect(tab.x, tab.y, tab.w, tab.h + look.radius), radii, look.border, 0.0f, 0);
				rvn_paint_box4(painter, rvn_rect(tab.x + 1.0f, tab.y + 1.0f, tab.w - 2.0f, tab.h + look.radius), radii, look.background, 0.0f, 0);
			} else {
				if (rvn_color_a(look.background)) {
					float small[4];
					small[0] = small[1] = small[2] = small[3] = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
					rvn_paint_box4(painter, rvn_rect(tab.x + 2.0f, tab.y + 3.0f, tab.w - 4.0f, tab.h - 6.0f), small, look.background, 0.0f, 0);
				}
				if (i + 1 < widget->child_count && (int)i + 1 != tabs->active && !hot) {
					rvn_paint_rect(painter, rvn_rect(tab.x + tab.w + 0.5f, tab.y + (tab.h - 16.0f) * 0.5f, 1.0f, 16.0f),
						rvn_widget_color(widget, R_COLOR_SEPARATOR));
				}
			}
			tx = tab.x + 12.0f;
			rvn_paint_push_clip(painter, rvn_rect(tab.x, tab.y, rvn_maxf(0.0f, tab.w - (closable ? RVN_TAB_CLOSE + 10.0f : 6.0f)), tab.h));
			if (page->slot.has_icon) {
				rvn_color_t tint = page->slot.icon_tint ? look.foreground : rvn_rgba(1.0f, 1.0f, 1.0f, 1.0f);
				rvn_paint_image(painter, rvn_rect(tx, floorf(tab.y + (tab.h - RVN_TAB_ICON) * 0.5f), RVN_TAB_ICON, RVN_TAB_ICON),
					page->slot.icon, page->slot.icon_uv, tint, 0.0f);
				tx += RVN_TAB_ICON + RVN_TAB_ICON_GAP;
			}
			rvn_paint_text_in(painter, R_INVALID_ID, size, rvn_rect(tx, tab.y, tab.x + tab.w - tx, tab.h), 0.0f, rvn_tab_title(page), -1, look.foreground);
			rvn_paint_pop_clip(painter);
			if (closable && (active || hot)) {
				rvn_rect_t close = rvn_tab_close_rect(widget, x, width);
				int over = hot && tabs->hot_close;
				if (over || tabs->press_close == (int)i) {
					rvn_paint_box(painter, close, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS),
						tabs->press_close == (int)i ? rvn_fluent(widget, RVN_FLUENT_SUBTLE_PRESSED) : rvn_widget_color(widget, R_COLOR_ROW_HOVERED), 0.0f, 0);
				}
				/* Cancel, from Segoe Fluent Icons */
				if (!rvn_paint_look_icon(painter, widget, 0xE711, close.x + close.w * 0.5f, close.y + close.h * 0.5f, 10.0f, look.foreground)) {
					rvn_paint_icon(painter, RVN_ICON_CROSS, close.x + close.w * 0.5f, close.y + close.h * 0.5f, 7.0f, look.foreground);
				}
			}
		} else {
			/* SelectorBar: titles, the one under the pointer on a soft background, the selected one with its pill */
			rvn_rect_t item = rvn_rect(x + 2.0f, 4.0f, width - 4.0f, bar - 8.0f);
			if (!active && rvn_color_a(look.background)) rvn_paint_box(painter, item, look.radius, look.background, 0.0f, 0);
			rvn_paint_text_in(painter, R_INVALID_ID, size, rvn_rect(x, 0.0f, width, bar - 2.0f), 0.5f, rvn_tab_title(page), -1, look.foreground);
		}
		x += width;
	}
	if (tabs->documents) {
		/* the line under the bar, open under the selected tab */
		float start = 4.0f, position = 4.0f;
		for (i = 0; i < widget->child_count; i++) {
			float width = rvn_tab_width(widget, widget->children[i]);
			if ((int)i == tabs->active) {
				rvn_paint_rect(painter, rvn_rect(0.0f, bar - 1.0f, position, 1.0f), header.border);
				start = position + width - 2.0f;
			}
			position += width;
		}
		rvn_paint_rect(painter, rvn_rect(tabs->active >= 0 ? start : 0.0f, bar - 1.0f, widget->rect.w - (tabs->active >= 0 ? start : 0.0f), 1.0f), header.border);
		return;
	}
	if (tabs->active >= 0 && mark_w > 4.0f) {
		float pill;
		rvn_part_look(widget, RVN_PART_INDICATOR, RVN_STATE_SELECTED, rvn_tab_fluent_look, NULL, &look);
		pill = rvn_minf(look.size, mark_w - 8.0f);
		rvn_paint_part(painter, rvn_rect(floorf(mark_x + (mark_w - pill) * 0.5f), bar - 6.0f, pill, 3.0f), &look);
	}
}

/*
 * macOS and GNOME: the tabs are a segmented control (NSSegmentedControl,
 * AdwToggleGroup): a rounded groove holding the titles, the active one on a
 * raised pill that slides. Document tabs (AdwTabBar, the tabs of Safari):
 * rounded tabs, the selected one raised, a close button.
 */
static void rvn_tab_other_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look, const void* context) {
	const int mac = rvn_widget_family(widget) == RVN_FAMILY_MACOS;
	const int dark = rvn_widget_theme(widget)->dark;
	RVN_UNUSED(context);
	switch (part) {
		case RVN_PART_HEADER:
			look->background = rvn_platform_ink(widget, mac ? (dark ? 0.1f : 0.06f) : 0.1f);
			/* (macOS 26: the segmented control is a capsule, and so is its pill) */
			look->radius = mac ? (rvn_widget_liquid(widget) ? 12.0f : 6.0f) : 9.0f;
			if (mac) {
				look->border = rvn_platform_ink(widget, 0.06f);
				look->border_width = 1.0f;
			}
			break;
		case RVN_PART_INDICATOR:
			/* the raised pill: white, or the color of a control in the dark */
			look->background = dark ? rvn_platform_ink(widget, mac ? 0.28f : 0.16f) : rvn_hex(0xFFFFFF, 255);
			look->radius = mac ? (rvn_widget_liquid(widget) ? 11.0f : 5.0f) : 6.0f;
			look->shadow = mac ? 1.5f : 3.0f;
			look->shadow_color = rvn_hex(0x000000, dark ? 90 : 46);
			if (mac && !dark) {
				look->border = rvn_hex(0x000000, 20);
				look->border_width = 0.5f;
			}
			break;
		case RVN_PART_TAB:
			look->radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS) + (mac ? 1.0f : 2.0f);
			look->foreground = (states & RVN_STATE_DISABLED) ? rvn_widget_color(widget, R_COLOR_TEXT_DISABLED) : rvn_widget_color(widget, R_COLOR_TEXT);
			if (states & RVN_STATE_SELECTED) look->background = rvn_widget_color(widget, R_COLOR_CONTROL_PRESSED);
			else if (states & RVN_STATE_HOVERED) look->background = rvn_platform_ink(widget, 0.06f);
			/* (GNOME: the titles that are not active, a little dimmer) */
			if (!(states & RVN_STATE_SELECTED) && !mac && !(states & RVN_STATE_HOVERED)) look->foreground = rvn_platform_ink(widget, 0.75f);
			break;
		default:
			break;
	}
}

static void rvn_tab_view_paint_other(rvn_widget_t* widget, RPainter* painter, float mark_x, float mark_w) {
	rvn_tab_view_t* tabs = (rvn_tab_view_t*)widget;
	const int mac = rvn_widget_family(widget) == RVN_FAMILY_MACOS;
	float bar = rvn_tab_bar_height(widget), size = rvn_widget_font_size(widget, 0.0f), x = 0.0f, total = 0.0f;
	rvn_part_look_t look;
	uint32_t i;

	if (tabs->documents) {
		x = 4.0f;
		for (i = 0; i < widget->child_count; i++) {
			rvn_widget_t* page = widget->children[i];
			float width = rvn_tab_width(widget, page);
			int active = (int)i == tabs->active;
			int hot = (int)i == tabs->hot && (widget->flags & RVN_WF_HOVERED) && painter->enabled;
			int closable = rvn_tab_closable(widget, page);
			rvn_rect_t tab = rvn_rect(x + 1.0f, 5.0f, width - 2.0f, bar - 10.0f);
			uint32_t states = rvn_widget_states(widget, painter, hot, R_FALSE) | (active ? RVN_STATE_SELECTED : 0);
			float tx = tab.x + 10.0f;
			rvn_part_look(widget, RVN_PART_TAB, states & ~RVN_STATE_FOCUSED, rvn_tab_other_look, NULL, &look);
			rvn_paint_part(painter, tab, &look);
			rvn_paint_push_clip(painter, rvn_rect(tab.x, tab.y, rvn_maxf(0.0f, tab.w - (closable ? RVN_TAB_CLOSE + 8.0f : 4.0f)), tab.h));
			if (page->slot.has_icon) {
				rvn_color_t tint = page->slot.icon_tint ? look.foreground : rvn_rgba(1.0f, 1.0f, 1.0f, 1.0f);
				rvn_paint_image(painter, rvn_rect(tx, floorf(tab.y + (tab.h - RVN_TAB_ICON) * 0.5f), RVN_TAB_ICON, RVN_TAB_ICON),
					page->slot.icon, page->slot.icon_uv, tint, 0.0f);
				tx += RVN_TAB_ICON + RVN_TAB_ICON_GAP;
			}
			rvn_paint_text_in(painter, R_INVALID_ID, size, rvn_rect(tx, tab.y, tab.x + tab.w - tx, tab.h), 0.0f, rvn_tab_title(page), -1, look.foreground);
			rvn_paint_pop_clip(painter);
			if (closable && (active || hot)) {
				rvn_rect_t close = rvn_tab_close_rect(widget, x, width);
				int over = hot && tabs->hot_close;
				if (over || tabs->press_close == (int)i) {
					rvn_paint_box(painter, close, mac ? 4.0f : close.w * 0.5f, rvn_platform_ink(widget, tabs->press_close == (int)i ? 0.16f : 0.08f), 0.0f, 0);
				}
				rvn_paint_icon(painter, RVN_ICON_CROSS, close.x + close.w * 0.5f, close.y + close.h * 0.5f, 7.0f, look.foreground);
			}
			x += width;
		}
		return;
	}
	for (i = 0; i < widget->child_count; i++) total += rvn_tab_width(widget, widget->children[i]);
	/* the groove, then the pill of the active tab (it slides), then the titles */
	{
		float height = mac ? rvn_minf(rvn_widget_liquid(widget) ? 24.0f : 22.0f, bar - 2.0f) : bar - 2.0f;
		float top = floorf((bar - height) * 0.5f), inset = mac ? 1.0f : 3.0f;
		rvn_part_look(widget, RVN_PART_HEADER, rvn_widget_states(widget, painter, R_FALSE, R_FALSE), rvn_tab_other_look, NULL, &look);
		rvn_paint_part(painter, rvn_rect(0.0f, top, total, height), &look);
		if (tabs->active >= 0 && mark_w > 4.0f) {
			rvn_part_look(widget, RVN_PART_INDICATOR, RVN_STATE_SELECTED, rvn_tab_other_look, NULL, &look);
			rvn_paint_part(painter, rvn_rect(mark_x + inset, top + inset, mark_w - inset * 2.0f, height - inset * 2.0f), &look);
		}
		for (i = 0; i < widget->child_count; i++) {
			rvn_widget_t* page = widget->children[i];
			float width = rvn_tab_width(widget, page);
			int active = (int)i == tabs->active;
			int hot = (int)i == tabs->hot && (widget->flags & RVN_WF_HOVERED) && painter->enabled;
			uint32_t states = rvn_widget_states(widget, painter, hot, R_FALSE) | (active ? RVN_STATE_SELECTED : 0);
			rvn_part_look(widget, RVN_PART_TAB, states & ~RVN_STATE_FOCUSED, rvn_tab_other_look, NULL, &look);
			/* (macOS: a hairline between two titles that are not next to the active one) */
			if (mac && i + 1 < widget->child_count && !active && (int)i + 1 != tabs->active) {
				rvn_paint_rect(painter, rvn_rect(x + width - 0.5f, top + 5.0f, 1.0f, height - 10.0f), rvn_platform_ink(widget, 0.15f));
			}
			if (!active && hot && !mac) {
				rvn_paint_box(painter, rvn_rect(x + inset, top + inset, width - inset * 2.0f, height - inset * 2.0f), 6.0f,
					rvn_platform_ink(widget, 0.05f), 0.0f, 0);
			}
			rvn_paint_text_in(painter, R_INVALID_ID, size, rvn_rect(x, top, width, height), 0.5f, rvn_tab_title(page), -1, look.foreground);
			x += width;
		}
	}
}

static void rvn_tab_view_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_tab_view_t* tabs = (rvn_tab_view_t*)widget;
	float bar = rvn_tab_bar_height(widget), position = 0.0f;
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	float mark_x = 0.0f, mark_w = 0.0f, stretch;
	uint32_t i;

	/* where the mark of the active tab is now: it slides to its tab, stretched by its own speed */
	for (i = 0; i < widget->child_count; i++) {
		float width = rvn_tab_width(widget, widget->children[i]);
		if ((int)i == tabs->active) {
			mark_x = position;
			mark_w = width;
		}
		position += width;
	}
	position = 0.0f;
	mark_x = rvn_paint_spring(painter, &tabs->mark_x, mark_x, R_MOTION_BOUNCY);
	mark_w = rvn_paint_spring(painter, &tabs->mark_w, mark_w, R_MOTION_BOUNCY);
	stretch = rvn_minf(fabsf(tabs->mark_x.velocity) * 0.018f, mark_w * 0.35f);
	mark_x -= stretch * 0.5f;
	mark_w += stretch;

	if (rvn_widget_family(widget) == RVN_FAMILY_WINDOWS) {
		rvn_tab_view_paint_fluent(widget, painter, mark_x, mark_w);
		return;
	}
	if (rvn_widget_family(widget) != RVN_FAMILY_RAVEN) {
		rvn_tab_view_paint_other(widget, painter, mark_x, mark_w);
		return;
	}
	if (tabs->documents) {
		/* the major tabs of the application : left aligned, icon, title, close button */
		float x = 4.0f, radius = rvn_minf(6.0f, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS) + 2.0f);
		float padding = rvn_widget_metric(widget, R_METRIC_PADDING_X);
		rvn_paint_rect(painter, rvn_rect(0.0f, 0.0f, widget->rect.w, bar), rvn_widget_color(widget, R_COLOR_HEADER));
		rvn_paint_rect(painter, rvn_rect(0.0f, bar - 1.0f, widget->rect.w, 1.0f), rvn_widget_color(widget, R_COLOR_SEPARATOR));
		for (i = 0; i < widget->child_count; i++) {
			rvn_widget_t* page = widget->children[i];
			float width = rvn_tab_width(widget, page);
			int is_active = (int)i == tabs->active;
			int is_hot = (int)i == tabs->hot && (widget->flags & RVN_WF_HOVERED);
			int closable = rvn_tab_closable(widget, page);
			rvn_rect_t tab = rvn_rect(x, 4.0f, width - 2.0f, bar - 4.0f);
			rvn_color_t color = is_active ? text : rvn_color_fade(text, is_hot ? 0.85f : 0.6f);
			float radii[4];
			float tx = tab.x + padding;
			radii[0] = radii[1] = radius;
			radii[2] = radii[3] = 0.0f;
			if (is_active) {
				rvn_paint_box4(painter, rvn_rect(tab.x, tab.y, tab.w, tab.h + 1.0f), radii, rvn_widget_color(widget, R_COLOR_TAB_ACTIVE), 0.0f, 0);
				rvn_paint_box4(painter, rvn_rect(tab.x, tab.y, tab.w, 2.0f), radii, rvn_widget_color(widget, R_COLOR_ACCENT), 0.0f, 0);
			} else if (is_hot) {
				rvn_paint_box4(painter, tab, radii, rvn_widget_color(widget, R_COLOR_HEADER_HOVERED), 0.0f, 0);
			} else if (i + 1 < widget->child_count && (int)i + 1 != tabs->active) {
				rvn_paint_rect(painter, rvn_rect(tab.x + tab.w + 0.5f, tab.y + 7.0f, 1.0f, tab.h - 14.0f), rvn_widget_color(widget, R_COLOR_SEPARATOR));
			}
			rvn_paint_push_clip(painter, rvn_rect(tab.x, tab.y, rvn_maxf(0.0f, tab.w - (closable ? RVN_TAB_CLOSE + 8.0f : 4.0f)), tab.h));
			if (page->slot.has_icon) {
				rvn_color_t tint = page->slot.icon_tint ? color : rvn_rgba(1.0f, 1.0f, 1.0f, is_active ? 1.0f : 0.75f);
				rvn_paint_image(painter, rvn_rect(tx, floorf(tab.y + (tab.h - RVN_TAB_ICON) * 0.5f), RVN_TAB_ICON, RVN_TAB_ICON),
					page->slot.icon, page->slot.icon_uv, tint, 0.0f);
				tx += RVN_TAB_ICON + RVN_TAB_ICON_GAP;
			}
			rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), rvn_rect(tx, tab.y, tab.x + tab.w - tx, tab.h), 0.0f,
				rvn_tab_title(page), -1, color);
			rvn_paint_pop_clip(painter);
			if (closable && (is_active || is_hot)) {
				rvn_rect_t close = rvn_tab_close_rect(widget, x, width);
				int over = is_hot && tabs->hot_close;
				if (over || tabs->press_close == (int)i)
					rvn_paint_box(painter, close, RVN_TAB_CLOSE * 0.5f, rvn_widget_color(widget, R_COLOR_CONTROL_PRESSED), 0.0f, 0);
				rvn_paint_icon(painter, RVN_ICON_CROSS, close.x + RVN_TAB_CLOSE * 0.5f, close.y + RVN_TAB_CLOSE * 0.5f, 7.0f,
					over ? text : rvn_color_fade(text, 0.6f));
			}
			x += width;
		}
		return;
	}
	if (rvn_widget_glass(widget)) {
		/* a segmented control: a capsule holding the titles, the active one on a lighter pill of glass */
		float total = 0.0f;
		for (i = 0; i < widget->child_count; i++) total += rvn_tab_width(widget, widget->children[i]);
		if (total > 0.0f) {
			rvn_paint_box(painter, rvn_rect(0.0f, 2.0f, total, bar - 6.0f), (bar - 6.0f) * 0.5f,
				rvn_widget_color(widget, R_COLOR_TRACK), 0.0f, 0);
		}
		if (tabs->active >= 0 && mark_w > 4.0f) {
			rvn_rect_t pill = rvn_rect(mark_x + 2.0f, 4.0f, mark_w - 4.0f, bar - 10.0f);
			rvn_paint_surface(painter, widget, pill, pill.h * 0.5f, RVN_SURFACE_CONTROL, 1.0f, 0.6f);
		}
		for (i = 0; i < widget->child_count; i++) {
			float width = rvn_tab_width(widget, widget->children[i]);
			rvn_rect_t rect = rvn_rect(position, 2.0f, width, bar - 6.0f);
			rvn_rect_t pill = rvn_rect(rect.x + 2.0f, rect.y + 2.0f, rect.w - 4.0f, rect.h - 4.0f);
			if ((int)i == tabs->active) {
				/* (its pill is the mark drawn above) */
			} else if ((int)i == tabs->hot && (widget->flags & RVN_WF_HOVERED)) {
				rvn_paint_box(painter, pill, pill.h * 0.5f, rvn_widget_color(widget, R_COLOR_HEADER_HOVERED), 0.0f, 0);
			}
			rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), rect, 0.5f,
				rvn_tab_title(widget->children[i]), -1, (int)i == tabs->active ? text : rvn_color_fade(text, 0.62f));
			position += width;
		}
		return;
	}
	rvn_paint_rect(painter, rvn_rect(0.0f, 0.0f, widget->rect.w, bar), rvn_widget_color(widget, R_COLOR_HEADER));
	rvn_paint_rect(painter, rvn_rect(0.0f, bar - 1.0f, widget->rect.w, 1.0f), rvn_widget_color(widget, R_COLOR_SEPARATOR));
	if (tabs->active >= 0 && mark_w > 0.0f) {
		rvn_paint_rect(painter, rvn_rect(mark_x, 0.0f, mark_w, bar), rvn_widget_color(widget, R_COLOR_TAB_ACTIVE));
		rvn_paint_rect(painter, rvn_rect(mark_x, 0.0f, mark_w, 2.0f), rvn_widget_color(widget, R_COLOR_ACCENT));
	}
	for (i = 0; i < widget->child_count; i++) {
		float width = rvn_tab_width(widget, widget->children[i]);
		rvn_rect_t rect = rvn_rect(position, 0.0f, width, bar);
		if ((int)i == tabs->active) {
			/* (its background is the mark drawn above) */
		} else if ((int)i == tabs->hot && (widget->flags & RVN_WF_HOVERED)) {
			rvn_paint_rect(painter, rect, rvn_widget_color(widget, R_COLOR_HEADER_HOVERED));
		}
		rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), rect, 0.5f,
			rvn_tab_title(widget->children[i]), -1,
			(int)i == tabs->active ? text : rvn_color_mix(text, rvn_widget_color(widget, R_COLOR_HEADER), 0.3f));
		position += width;
	}
}

static int rvn_tab_view_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	int index;
	if (RVN_ATTR("ActiveTab")) {
		if (!rvn_parse_int(value, &index)) return R_FALSE;
		rvn_tab_view_activate(widget, index, R_FALSE);
		return R_TRUE;
	}
	if (RVN_ATTR("Documents")) {
		if (!rvn_parse_bool(value, &index)) return R_FALSE;
		((rvn_tab_view_t*)widget)->documents = index;
		rvn_widget_invalidate_layout(widget);
		return R_TRUE;
	}
	return R_FALSE;
}

const rvn_widget_class_t rvn_class_tab_view = {
	.type = R_WIDGET_TAB_VIEW,
	.name = "TabView",
	.size = sizeof(rvn_tab_view_t),
	.flags = RVN_CF_SURFACE,
	.init = rvn_tab_view_init,
	.measure = rvn_tab_view_measure,
	.arrange = rvn_tab_view_arrange,
	.paint = rvn_tab_view_paint,
	.event = rvn_tab_view_event,
	.set_attribute = rvn_tab_view_set_attribute,
	.child_active = rvn_tab_view_child_active,
	.children_changed = rvn_tab_view_children_changed
};

int Rtab_view_add_tab(Rid id, const char* title, Rid content) {
	rvn_widget_t* page;
	RVN_WIDGET(widget, id, R_WIDGET_TAB_VIEW, -1);
	page = rvn_widget_check(content, 0, __func__);
	if (!page) return -1;
	if (rvn_widget_is_inside(widget, page)) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rtab_view_add_tab: the content contains the tab view");
		return -1;
	}
	rvn_str_set(&page->slot.title, title);
	rvn_widget_add_child(widget, page, -1);
	return (int)widget->child_count - 1;
}

void Rtab_view_set_active(Rid id, int index) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TAB_VIEW);
	rvn_tab_view_activate(widget, index, R_FALSE);
}

int Rtab_view_get_active(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TAB_VIEW, -1);
	return ((rvn_tab_view_t*)widget)->active;
}

void Rtab_view_set_changed_callback(Rid id, RTabChangedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TAB_VIEW);
	((rvn_tab_view_t*)widget)->callback = callback;
	((rvn_tab_view_t*)widget)->user_data = user_data;
}

/* ---- documents mode, tabs by their page ---- */

static rvn_widget_t* rvn_tab_page_of(rvn_widget_t* widget, Rid content, const char* function) {
	uint32_t i;
	for (i = 0; i < widget->child_count; i++)
		if (widget->children[i]->id == content) return widget->children[i];
	rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: the widget is not a page of the tab view", function);
	return NULL;
}

void Rtab_view_set_documents(Rid id, int documents) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TAB_VIEW);
	((rvn_tab_view_t*)widget)->documents = documents != 0;
	rvn_widget_invalidate_layout(widget);
}

int Rtab_view_get_tab_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TAB_VIEW, 0);
	return (int)widget->child_count;
}

Rid Rtab_view_get_tab(Rid id, int index) {
	RVN_WIDGET(widget, id, R_WIDGET_TAB_VIEW, R_INVALID_ID);
	if (index < 0 || index >= (int)widget->child_count) return R_INVALID_ID;
	return widget->children[index]->id;
}

int Rtab_view_find_tab(Rid id, Rid content) {
	uint32_t i;
	RVN_WIDGET(widget, id, R_WIDGET_TAB_VIEW, -1);
	for (i = 0; i < widget->child_count; i++)
		if (widget->children[i]->id == content) return (int)i;
	return -1;
}

void Rtab_view_set_tab_title(Rid id, Rid content, const char* title) {
	rvn_widget_t* page;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TAB_VIEW);
	page = rvn_tab_page_of(widget, content, __func__);
	if (!page) return;
	rvn_str_set(&page->slot.title, title ? title : "");
	rvn_widget_invalidate_paint(widget);
}

void Rtab_view_set_tab_icon(Rid id, Rid content, Rid texture, float u0, float v0, float u1, float v1, int tint) {
	rvn_widget_t* page;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TAB_VIEW);
	page = rvn_tab_page_of(widget, content, __func__);
	if (!page) return;
	page->slot.has_icon = texture != R_INVALID_ID;
	page->slot.icon = texture;
	page->slot.icon_uv[0] = u0;
	page->slot.icon_uv[1] = v0;
	page->slot.icon_uv[2] = u1;
	page->slot.icon_uv[3] = v1;
	page->slot.icon_tint = tint != 0;
	rvn_widget_invalidate_paint(widget);
}

void Rtab_view_set_tab_closable(Rid id, Rid content, int closable) {
	rvn_widget_t* page;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TAB_VIEW);
	page = rvn_tab_page_of(widget, content, __func__);
	if (!page) return;
	page->slot.dock_fixed = !closable;
	rvn_widget_invalidate_paint(widget);
}

void Rtab_view_show_tab(Rid id, Rid content) {
	int index;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TAB_VIEW);
	index = Rtab_view_find_tab(id, content);
	if (index >= 0) rvn_tab_view_activate(widget, index, R_TRUE);
}

void Rtab_view_set_close_callback(Rid id, RTabCloseCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TAB_VIEW);
	((rvn_tab_view_t*)widget)->close_callback = callback;
	((rvn_tab_view_t*)widget)->close_user = user_data;
}
