/**
 * Raven - widgets/tile_view.c
 * TileView (Slate STileView): a grid of tiles, an icon over a name, that
 * scrolls vertically. The tiles come from the application
 * (Rtile_view_set_items) and only the visible ones are asked for and drawn:
 * a folder of ten thousand files costs what a folder of ten does.
 *
 * Selection of several tiles (Ctrl, Shift, a rubber band from the empty
 * space, Ctrl + A), activation (double click, Enter), a menu per tile and
 * tiles that can be dragged (Rdrag_begin).
 *
 * The callbacks of the application run while painting: the class paints on
 * the UI thread (RVN_CF_UI_PAINT).
 */

#include "common.h"
#include "../render/font.h"

typedef struct rvn_tile_view_t {
	rvn_widget_t base;
	int count;
	RTileItemCallback item_callback;
	void* item_user;

	float tile_w, tile_h, icon_size, spacing;
	int rows;   /* Layout="Rows": one item per line, as wide as the view, icon on the left */

	/** One flag per tile (count of them). */
	uint8_t* selected;
	int selected_cap;
	int current;
	int anchor;
	int hot;
	rvn_scrollbar_t bar;

	/* press that may become a click, a drag or a rubber band */
	int press_pending;
	int press_index;
	float press_x, press_y;
	int press_reduce;
	int band_active;
	float band_x0, band_y0, band_x1, band_y1;
	uint8_t* band_base;

	char* drag_type;
	RTileDragDataCallback drag_data_callback;
	void* drag_data_user;
	int drop_index;

	RTileEventCallback selection_callback;
	void* selection_user;
	RTileEventCallback activated_callback;
	void* activated_user;
	RTileContextCallback context_callback;
	void* context_user;
	char* empty_text;
} rvn_tile_view_t;

static void rvn_tile_view_init(rvn_widget_t* widget) {
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	tiles->tile_w = 112.0f;
	tiles->tile_h = 112.0f;
	tiles->icon_size = 64.0f;
	tiles->spacing = 8.0f;
	tiles->current = -1;
	tiles->anchor = -1;
	tiles->hot = -1;
	tiles->press_index = -1;
	tiles->drop_index = -1;
}

static void rvn_tile_view_destroy(rvn_widget_t* widget) {
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	rvn_free(tiles->selected);
	rvn_free(tiles->band_base);
	rvn_free(tiles->drag_type);
	rvn_free(tiles->empty_text);
}

static void rvn_tile_reserve(rvn_tile_view_t* tiles) {
	if (tiles->count <= tiles->selected_cap) return;
	tiles->selected = (uint8_t*)rvn_realloc(tiles->selected, (size_t)tiles->count);
	memset(tiles->selected + tiles->selected_cap, 0, (size_t)(tiles->count - tiles->selected_cap));
	tiles->selected_cap = tiles->count;
}

static float rvn_tile_width(const rvn_widget_t* widget) {
	const rvn_tile_view_t* tiles = (const rvn_tile_view_t*)widget;
	if (tiles->rows) {
		float bar = rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE);
		return rvn_maxf(16.0f, widget->rect.w - tiles->spacing * 2.0f - bar);
	}
	return tiles->tile_w;
}

static int rvn_tile_columns(const rvn_widget_t* widget) {
	const rvn_tile_view_t* tiles = (const rvn_tile_view_t*)widget;
	if (tiles->rows) return 1;
	float bar = rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE);
	float width = widget->rect.w - tiles->spacing - bar;
	int columns = (int)floorf(width / (tiles->tile_w + tiles->spacing));
	return columns < 1 ? 1 : columns;
}

static float rvn_tile_pitch_y(const rvn_tile_view_t* tiles) {
	return tiles->tile_h + tiles->spacing;
}

static rvn_vec2_t rvn_tile_view_measure(rvn_widget_t* widget) {
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	return rvn_vec2(tiles->tile_w * 2.0f + tiles->spacing * 3.0f, tiles->tile_h * 2.0f + tiles->spacing * 3.0f);
}

static void rvn_tile_view_arrange(rvn_widget_t* widget) {
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	int columns = rvn_tile_columns(widget);
	int rows = (tiles->count + columns - 1) / columns;
	tiles->bar.content = rvn_tile_pitch_y(tiles) * (float)rows + tiles->spacing;
	tiles->bar.viewport = rvn_maxf(0.0f, widget->rect.h);
	rvn_scrollbar_clamp(&tiles->bar);
}

static rvn_rect_t rvn_tile_track(const rvn_widget_t* widget) {
	float bar = rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE);
	return rvn_rect(widget->rect.w - bar - 1.0f, 1.0f, bar, rvn_maxf(0.0f, widget->rect.h - 2.0f));
}

/* Rectangle of a tile in widget coordinates, for a scroll offset. */
static rvn_rect_t rvn_tile_rect(const rvn_widget_t* widget, int index, float offset) {
	const rvn_tile_view_t* tiles = (const rvn_tile_view_t*)widget;
	int columns = rvn_tile_columns(widget);
	int column = index % columns, row = index / columns;
	float tile_w = rvn_tile_width(widget);
	return rvn_rect(tiles->spacing + (tile_w + tiles->spacing) * (float)column,
		tiles->spacing + rvn_tile_pitch_y(tiles) * (float)row - offset, tile_w, tiles->tile_h);
}

static int rvn_tile_at(const rvn_widget_t* widget, float x, float y) {
	const rvn_tile_view_t* tiles = (const rvn_tile_view_t*)widget;
	int columns = rvn_tile_columns(widget);
	float cx = x - tiles->spacing, cy = y + tiles->bar.offset - tiles->spacing;
	int column, row, index;
	if (cx < 0.0f || cy < 0.0f) return -1;
	column = (int)floorf(cx / (rvn_tile_width(widget) + tiles->spacing));
	row = (int)floorf(cy / rvn_tile_pitch_y(tiles));
	if (column >= columns) return -1;
	/* the gaps between the tiles are empty space */
	if (cx - (float)column * (rvn_tile_width(widget) + tiles->spacing) > rvn_tile_width(widget)) return -1;
	if (cy - (float)row * rvn_tile_pitch_y(tiles) > tiles->tile_h) return -1;
	index = row * columns + column;
	return index < tiles->count ? index : -1;
}

static void rvn_tile_reveal(rvn_widget_t* widget, int index) {
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	rvn_rect_t rect;
	if (index < 0) return;
	rvn_tile_view_arrange(widget);
	rect = rvn_tile_rect(widget, index, 0.0f);
	if (rect.y - tiles->spacing < tiles->bar.offset) rvn_scrollbar_set(&tiles->bar, rect.y - tiles->spacing);
	else if (rect.y + rect.h + tiles->spacing > tiles->bar.offset + tiles->bar.viewport)
		rvn_scrollbar_set(&tiles->bar, rect.y + rect.h + tiles->spacing - tiles->bar.viewport);
	rvn_widget_invalidate_paint(widget);
}

static void rvn_tile_notify(rvn_widget_t* widget) {
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	rvn_widget_invalidate_paint(widget);
	if (tiles->selection_callback) tiles->selection_callback(widget->id, tiles->current, tiles->selection_user);
}

static void rvn_tile_select_only(rvn_widget_t* widget, int index, int notify) {
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	rvn_tile_reserve(tiles);
	if (tiles->count > 0) memset(tiles->selected, 0, (size_t)tiles->count);
	if (index >= 0 && index < tiles->count) tiles->selected[index] = 1;
	else index = -1;
	tiles->current = index;
	tiles->anchor = index;
	rvn_tile_reveal(widget, index);
	if (notify) rvn_tile_notify(widget);
	else rvn_widget_invalidate_paint(widget);
}

static void rvn_tile_activate(rvn_widget_t* widget) {
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	if (tiles->current < 0) return;
	if (tiles->activated_callback) tiles->activated_callback(widget->id, tiles->current, tiles->activated_user);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

static void rvn_tile_view_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	float offset = rvn_scrollbar_shown(&tiles->bar, painter);
	int columns = rvn_tile_columns(widget);
	float pitch = rvn_tile_pitch_y(tiles);
	int first_row = (int)floorf((offset - tiles->spacing) / pitch);
	int last_row = (int)floorf((offset + widget->rect.h) / pitch) + 1;
	int first = rvn_maxi(0, first_row * columns), last = rvn_mini(tiles->count, (last_row + 1) * columns), i;
	int focused = (widget->flags & RVN_WF_FOCUSED) != 0;
	float radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
	float font_size = rvn_widget_font_size(widget, 0.0f);
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	rvn_color_t selection = rvn_widget_color(widget, R_COLOR_SELECTION);
	rvn_color_t accent = rvn_widget_color(widget, R_COLOR_ACCENT);
	rvn_vec2_t origin = rvn_widget_origin(widget);

	rvn_tile_reserve(tiles);
	rvn_paint_push_clip(painter, rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h));

	if (tiles->count == 0 && tiles->empty_text && tiles->empty_text[0]) {
		rvn_paint_text_in(painter, R_INVALID_ID, font_size, rvn_rect(0.0f, 0.0f, widget->rect.w, font_size * 3.0f), 0.5f,
			tiles->empty_text, -1, rvn_widget_color(widget, R_COLOR_TEXT_DISABLED));
	}

	for (i = first; i < last; i++) {
		RTileItem item;
		rvn_rect_t rect = rvn_tile_rect(widget, i, offset);
		int is_selected = tiles->selected[i] != 0;
		float icon, label_y;
		memset(&item, 0, sizeof(item));
		item.icon = R_INVALID_ID;
		item.tint[0] = item.tint[1] = item.tint[2] = item.tint[3] = 1.0f;
		item.uv[2] = item.uv[3] = 1.0f;
		if (tiles->item_callback) tiles->item_callback(widget->id, i, &item, tiles->item_user);
		if (!rvn_widget_alive(widget)) break;

		if (is_selected) {
			rvn_paint_box(painter, rect, radius, focused ? rvn_color_fade(selection, 0.85f) : rvn_color_fade(selection, 0.5f),
				i == tiles->current ? 2.0f : 0.0f, accent);
		} else if (i == tiles->drop_index) {
			rvn_paint_box(painter, rect, radius, rvn_color_fade(accent, 0.15f), 1.5f, accent);
		} else if (i == tiles->hot && (widget->flags & RVN_WF_HOVERED) && painter->enabled) {
			rvn_paint_box(painter, rect, radius, rvn_widget_color(widget, R_COLOR_ROW_HOVERED), 0.0f, 0);
		}

		if (tiles->rows) {
			float text_x, text_h;
			const char* label = item.text ? item.text : "";
			int has_detail = item.detail && item.detail[0];
			icon = rvn_minf(tiles->icon_size, rect.h - 6.0f);
			if (item.icon != R_INVALID_ID && icon >= 4.0f)
				rvn_paint_image(painter, rvn_rect(floorf(rect.x + 6.0f), floorf(rect.y + (rect.h - icon) * 0.5f), icon, icon),
					item.icon, item.uv, rvn_rgba(item.tint[0], item.tint[1], item.tint[2], item.tint[3]), item.icon_radius);
			text_x = rect.x + 6.0f + (item.icon != R_INVALID_ID ? icon + 10.0f : 0.0f);
			text_h = font_size + 4.0f + (has_detail ? font_size * 0.85f + 6.0f : 0.0f);
			label_y = floorf(rect.y + (rect.h - text_h) * 0.5f);
			rvn_paint_push_clip(painter, rvn_rect(text_x, rect.y, rect.x + rect.w - text_x - 4.0f, rect.h));
			rvn_paint_text_in(painter, R_INVALID_ID, font_size, rvn_rect(text_x, label_y, rect.x + rect.w - text_x - 8.0f, font_size + 4.0f),
				0.0f, label, -1, item.text_color[3] > 0.0f ?
					rvn_rgba(item.text_color[0], item.text_color[1], item.text_color[2], item.text_color[3]) : text);
			if (has_detail)
				rvn_paint_text_in(painter, R_INVALID_ID, font_size * 0.85f,
					rvn_rect(text_x, label_y + font_size + 6.0f, rect.x + rect.w - text_x - 8.0f, font_size + 2.0f), 0.0f,
					item.detail, -1, is_selected ? text : rvn_widget_color(widget, R_COLOR_TEXT_DISABLED));
			if (item.badge && item.badge[0])
				rvn_paint_text_in(painter, R_INVALID_ID, font_size * 0.85f,
					rvn_rect(rect.x, label_y, rect.w - 8.0f, font_size + 4.0f), 1.0f, item.badge, -1, rvn_color_fade(text, 0.7f));
			rvn_paint_pop_clip(painter);
			continue;
		}

		icon = rvn_minf(tiles->icon_size, rvn_minf(rect.w - 8.0f, rect.h - font_size - 14.0f));
		if (icon < 4.0f) icon = 4.0f;
		if (item.icon != R_INVALID_ID) {
			/* a small hop when hovered */
			float hop = (i == tiles->hot && !tiles->band_active) ? -2.0f : 0.0f;
			rvn_paint_image(painter, rvn_rect(floorf(rect.x + (rect.w - icon) * 0.5f), floorf(rect.y + 5.0f + hop), icon, icon),
				item.icon, item.uv, rvn_rgba(item.tint[0], item.tint[1], item.tint[2], item.tint[3]), item.icon_radius);
		}
		if (item.badge && item.badge[0]) {
			rvn_paint_text_in(painter, R_INVALID_ID, font_size * 0.8f,
				rvn_rect(rect.x + 4.0f, rect.y + 2.0f, rect.w - 8.0f, font_size), 1.0f, item.badge, -1,
				rvn_color_fade(text, 0.7f));
		}
		label_y = rect.y + 5.0f + icon + 2.0f;
		rvn_paint_push_clip(painter, rvn_rect(rect.x + 3.0f, label_y, rect.w - 6.0f, rect.y + rect.h - label_y));
		{
			rvn_font_t* font = rvn_font_resolve(R_INVALID_ID);
			const char* label = item.text ? item.text : "";
			float width = font ? rvn_text_width(font, font_size, label, -1) : 0.0f;
			/* a name too long starts at the left (its beginning is what tells the files apart) */
			float align = width > rect.w - 8.0f ? 0.0f : 0.5f;
			rvn_paint_text_in(painter, R_INVALID_ID, font_size, rvn_rect(rect.x + 4.0f, label_y, rect.w - 8.0f, font_size + 6.0f),
				align, label, -1, item.text_color[3] > 0.0f ?
					rvn_rgba(item.text_color[0], item.text_color[1], item.text_color[2], item.text_color[3]) : text);
			if (item.detail && item.detail[0]) {
				rvn_paint_text_in(painter, R_INVALID_ID, font_size * 0.85f,
					rvn_rect(rect.x + 4.0f, label_y + font_size + 4.0f, rect.w - 8.0f, font_size + 4.0f), 0.5f,
					item.detail, -1, rvn_widget_color(widget, R_COLOR_TEXT_DISABLED));
			}
		}
		rvn_paint_pop_clip(painter);
	}

	if (tiles->band_active) {
		float x0 = rvn_minf(tiles->band_x0, tiles->band_x1), x1 = rvn_maxf(tiles->band_x0, tiles->band_x1);
		float y0 = rvn_minf(tiles->band_y0, tiles->band_y1) - offset, y1 = rvn_maxf(tiles->band_y0, tiles->band_y1) - offset;
		rvn_paint_box(painter, rvn_rect(x0, y0, x1 - x0, y1 - y0), 2.0f, rvn_color_fade(accent, 0.18f), 1.0f, rvn_color_fade(accent, 0.9f));
	}
	rvn_paint_pop_clip(painter);
	rvn_scrollbar_paint(&tiles->bar, widget, painter, rvn_tile_track(widget), R_TRUE,
		widget->window ? widget->window->mouse_x - origin.x : -1.0f, widget->window ? widget->window->mouse_y - origin.y : -1.0f);
}

/* The tiles inside the rubber band (content coordinates) join the base selection. */
static void rvn_tile_band_update(rvn_widget_t* widget) {
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	float x0 = rvn_minf(tiles->band_x0, tiles->band_x1), x1 = rvn_maxf(tiles->band_x0, tiles->band_x1);
	float y0 = rvn_minf(tiles->band_y0, tiles->band_y1), y1 = rvn_maxf(tiles->band_y0, tiles->band_y1);
	int i, last_inside = -1;
	rvn_tile_reserve(tiles);
	for (i = 0; i < tiles->count; i++) {
		rvn_rect_t rect = rvn_tile_rect(widget, i, 0.0f);
		int inside = rect.x <= x1 && rect.x + rect.w >= x0 && rect.y <= y1 && rect.y + rect.h >= y0;
		tiles->selected[i] = (uint8_t)(inside || (tiles->band_base && tiles->band_base[i]));
		if (inside) last_inside = i;
	}
	if (last_inside >= 0) tiles->current = last_inside;
	rvn_tile_notify(widget);
}

static int rvn_tile_view_event(rvn_widget_t* widget, const REvent* event) {
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	int changed = R_FALSE, index;

	if (event->type == R_EVENT_MOUSE_DOWN || event->type == R_EVENT_MOUSE_MOVE || event->type == R_EVENT_MOUSE_UP) {
		if (!tiles->band_active && rvn_scrollbar_event(&tiles->bar, widget, event, rvn_tile_track(widget), R_TRUE, &changed)) {
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		}
	}
	rvn_tile_reserve(tiles);
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			index = rvn_tile_at(widget, event->local_x, event->local_y);
			if (index != tiles->hot) {
				tiles->hot = index;
				rvn_widget_invalidate_paint(widget);
			}
			if (tiles->band_active) {
				float bottom = widget->rect.h;
				tiles->band_x1 = rvn_clampf(event->local_x, 0.0f, widget->rect.w);
				/* past the top or the bottom: the grid scrolls */
				if (event->local_y < 0.0f) rvn_scrollbar_set(&tiles->bar, tiles->bar.offset + event->local_y * 0.3f);
				else if (event->local_y > bottom) rvn_scrollbar_set(&tiles->bar, tiles->bar.offset + (event->local_y - bottom) * 0.3f);
				tiles->band_y1 = rvn_clampf(event->local_y, 0.0f, bottom) + tiles->bar.offset;
				rvn_tile_band_update(widget);
				return R_TRUE;
			}
			if (tiles->press_pending && tiles->press_index >= 0 && tiles->drag_type && widget->window &&
				(widget->window->buttons & (1u << R_MOUSE_LEFT)) &&
				(fabsf(event->x - tiles->press_x) > 5.0f || fabsf(event->y - tiles->press_y) > 5.0f)) {
				char* data;
				size_t cap = 16, used = 0;
				int i, count = 0;
				RTileItem item;
				char label[192];
				tiles->press_pending = R_FALSE;
				tiles->press_reduce = R_FALSE;
				data = (char*)rvn_alloc(cap);
				data[0] = 0;
				for (i = 0; i < tiles->count; i++) {
					char number[16];
					size_t length;
					if (!tiles->selected[i]) continue;
					length = (size_t)snprintf(number, sizeof(number), count ? " %d" : "%d", i);
					if (used + length + 1 > cap) {
						while (used + length + 1 > cap) cap *= 2;
						data = (char*)rvn_realloc(data, cap);
					}
					memcpy(data + used, number, length + 1);
					used += length;
					count++;
				}
				/* the application may give its own data (paths...) instead of the indices */
				if (tiles->drag_data_callback) {
					const char* given = tiles->drag_data_callback(widget->id, tiles->drag_data_user);
					if (!rvn_widget_alive(widget)) {
						rvn_free(data);
						return R_TRUE;
					}
					if (given) {
						rvn_free(data);
						data = rvn_strdup(given);
					}
				}
				memset(&item, 0, sizeof(item));
				item.icon = R_INVALID_ID;
				if (tiles->item_callback) tiles->item_callback(widget->id, tiles->press_index, &item, tiles->item_user);
				if (count > 1) snprintf(label, sizeof(label), "%s (+%d)", item.text ? item.text : "", count - 1);
				else snprintf(label, sizeof(label), "%s", item.text ? item.text : "");
				if (rvn_widget_alive(widget)) Rdrag_begin(widget->id, tiles->drag_type, data, label);
				rvn_free(data);
				return R_TRUE;
			}
			return R_FALSE;
		case R_EVENT_MOUSE_LEAVE:
			tiles->hot = -1;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			if (event->button == R_MOUSE_RIGHT && tiles->context_callback) return R_TRUE;
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			rvn_widget_capture_mouse(widget);
			index = rvn_tile_at(widget, event->local_x, event->local_y);
			tiles->press_pending = R_TRUE;
			tiles->press_index = index;
			tiles->press_reduce = R_FALSE;
			tiles->press_x = event->x;
			tiles->press_y = event->y;
			if (index < 0) {
				/* the empty space: a rubber band (Ctrl / Shift keep what is selected) */
				int keep = (event->modifiers & (R_MOD_CTRL | R_MOD_SHIFT)) != 0;
				rvn_free(tiles->band_base);
				tiles->band_base = NULL;
				if (keep && tiles->count > 0) {
					tiles->band_base = (uint8_t*)rvn_alloc((size_t)tiles->count);
					memcpy(tiles->band_base, tiles->selected, (size_t)tiles->count);
				} else {
					rvn_tile_select_only(widget, -1, R_TRUE);
				}
				tiles->band_active = R_TRUE;
				tiles->band_x0 = tiles->band_x1 = event->local_x;
				tiles->band_y0 = tiles->band_y1 = event->local_y + tiles->bar.offset;
				return R_TRUE;
			}
			if (event->clicks == 2 && index == tiles->current) {
				tiles->press_pending = R_FALSE;
				rvn_tile_activate(widget);
				return R_TRUE;
			}
			if (event->modifiers & R_MOD_SHIFT) {
				int from = tiles->anchor >= 0 ? tiles->anchor : index, to = index, i;
				if (!(event->modifiers & R_MOD_CTRL)) memset(tiles->selected, 0, (size_t)tiles->count);
				if (from > to) { i = from; from = to; to = i; }
				for (i = from; i <= to; i++) tiles->selected[i] = 1;
				tiles->current = index;
				rvn_tile_notify(widget);
			} else if (event->modifiers & R_MOD_CTRL) {
				tiles->selected[index] = (uint8_t)!tiles->selected[index];
				tiles->anchor = index;
				tiles->current = tiles->selected[index] ? index : -1;
				if (tiles->current < 0) {
					int i;
					for (i = 0; i < tiles->count; i++) if (tiles->selected[i]) tiles->current = i;
				}
				rvn_tile_notify(widget);
			} else if (tiles->selected[index]) {
				/* a press on a selected tile keeps the others (a drag takes them all) */
				tiles->press_reduce = R_TRUE;
				if (tiles->current != index) {
					tiles->current = index;
					rvn_tile_notify(widget);
				}
			} else {
				rvn_tile_select_only(widget, index, R_TRUE);
			}
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (event->button == R_MOUSE_RIGHT && tiles->context_callback) {
				index = rvn_tile_at(widget, event->local_x, event->local_y);
				if (index >= 0 && !tiles->selected[index]) rvn_tile_select_only(widget, index, R_TRUE);
				if (!rvn_widget_alive(widget)) return R_TRUE;
				tiles->context_callback(widget->id, index, event->x, event->y, tiles->context_user);
				return R_TRUE;
			}
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			rvn_widget_release_mouse(widget);
			if (tiles->band_active) {
				tiles->band_active = R_FALSE;
				rvn_free(tiles->band_base);
				tiles->band_base = NULL;
				rvn_widget_invalidate_paint(widget);
			} else if (tiles->press_pending && tiles->press_reduce && tiles->press_index >= 0) {
				rvn_tile_select_only(widget, tiles->press_index, R_TRUE);
			}
			tiles->press_pending = R_FALSE;
			tiles->press_reduce = R_FALSE;
			return R_TRUE;
		case R_EVENT_MOUSE_WHEEL:
			if (event->modifiers & R_MOD_CTRL) return R_FALSE;   /* left to the application (zoom) */
			if (!rvn_scrollbar_wheel(&tiles->bar, -event->wheel_y * rvn_tile_pitch_y(tiles) * 0.75f)) {
				rvn_widget_invalidate_paint(widget);
				return R_FALSE;
			}
			tiles->hot = rvn_tile_at(widget, event->local_x, event->local_y);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_FOCUS_GAINED:
		case R_EVENT_FOCUS_LOST:
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_KEY_DOWN: {
			int columns = rvn_tile_columns(widget), next = tiles->current;
			if (tiles->count == 0) return R_FALSE;
			switch (event->key) {
				case R_KEY_LEFT: next = next < 0 ? 0 : next - 1; break;
				case R_KEY_RIGHT: next = next + 1; break;
				case R_KEY_UP: next = next < 0 ? 0 : next - columns; break;
				case R_KEY_DOWN: next = next < 0 ? 0 : next + columns; break;
				case R_KEY_HOME: next = 0; break;
				case R_KEY_END: next = tiles->count - 1; break;
				case R_KEY_ENTER:
				case R_KEY_KP_ENTER:
					rvn_tile_activate(widget);
					return R_TRUE;
				case R_KEY_A:
					if (!(event->modifiers & R_MOD_CTRL)) return R_FALSE;
					memset(tiles->selected, 1, (size_t)tiles->count);
					if (tiles->current < 0) tiles->current = 0;
					rvn_tile_notify(widget);
					return R_TRUE;
				default:
					return R_FALSE;
			}
			next = rvn_clampi(next, 0, tiles->count - 1);
			rvn_tile_select_only(widget, next, R_TRUE);
			return R_TRUE;
		}
		default:
			return R_FALSE;
	}
}

static int rvn_tile_view_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	float number;
	if (RVN_ATTR("TileWidth")) {
		if (!rvn_parse_float(value, &number)) return R_FALSE;
		tiles->tile_w = rvn_maxf(16.0f, number);
	} else if (RVN_ATTR("Layout")) {
		if (rvn_stricmp(value, "Rows") == 0) tiles->rows = R_TRUE;
		else if (rvn_stricmp(value, "Tiles") == 0) tiles->rows = R_FALSE;
		else return R_FALSE;
		rvn_widget_invalidate_layout(widget);
	} else if (RVN_ATTR("TileHeight")) {
		if (!rvn_parse_float(value, &number)) return R_FALSE;
		tiles->tile_h = rvn_maxf(16.0f, number);
	} else if (RVN_ATTR("IconSize")) {
		if (!rvn_parse_float(value, &number)) return R_FALSE;
		tiles->icon_size = rvn_maxf(4.0f, number);
	} else if (RVN_ATTR("Spacing")) {
		if (!rvn_parse_float(value, &number)) return R_FALSE;
		tiles->spacing = rvn_maxf(0.0f, number);
	} else if (RVN_ATTR("EmptyText")) {
		rvn_str_set(&tiles->empty_text, value);
	} else if (RVN_ATTR("DragType")) {
		rvn_free(tiles->drag_type);
		tiles->drag_type = value && value[0] ? rvn_strdup(value) : NULL;
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_layout(widget);
	return R_TRUE;
}

static int rvn_touch_tile_view_pan(rvn_widget_t* widget, float dx, float dy, int coasting) {
	RVN_UNUSED(dx);
	return rvn_scrollbar_pan(&((rvn_tile_view_t*)widget)->bar, -dy, coasting);
}

const rvn_widget_class_t rvn_class_tile_view = {
	.type = R_WIDGET_TILE_VIEW,
	.name = "TileView",
	.size = sizeof(rvn_tile_view_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_UI_PAINT,
	.init = rvn_tile_view_init,
	.destroy = rvn_tile_view_destroy,
	.measure = rvn_tile_view_measure,
	.arrange = rvn_tile_view_arrange,
	.paint = rvn_tile_view_paint,
	.event = rvn_tile_view_event,
	.pan = rvn_touch_tile_view_pan,
	.set_attribute = rvn_tile_view_set_attribute
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

void Rtile_view_set_items(Rid id, int count, RTileItemCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TILE_VIEW);
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	int old = tiles->count;
	tiles->count = count < 0 ? 0 : count;
	tiles->item_callback = callback;
	tiles->item_user = user_data;
	rvn_tile_reserve(tiles);
	/* other items: the selection does not carry over */
	if (tiles->selected_cap > 0) memset(tiles->selected, 0, (size_t)tiles->selected_cap);
	(void)old;
	tiles->current = tiles->anchor = tiles->hot = tiles->press_index = tiles->drop_index = -1;
	tiles->press_pending = R_FALSE;
	tiles->band_active = R_FALSE;
	rvn_free(tiles->band_base);
	tiles->band_base = NULL;
	rvn_widget_invalidate_layout(widget);
}

void Rtile_view_refresh(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TILE_VIEW);
	rvn_widget_invalidate_paint(widget);
}

int Rtile_view_get_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TILE_VIEW, 0);
	return ((rvn_tile_view_t*)widget)->count;
}

void Rtile_view_set_tile_size(Rid id, float width, float height, float icon_size) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TILE_VIEW);
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	tiles->tile_w = rvn_maxf(16.0f, width);
	tiles->tile_h = rvn_maxf(16.0f, height);
	tiles->icon_size = rvn_maxf(4.0f, icon_size);
	rvn_widget_invalidate_layout(widget);
}

void Rtile_view_set_empty_text(Rid id, const char* text) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TILE_VIEW);
	rvn_str_set(&((rvn_tile_view_t*)widget)->empty_text, text);
	rvn_widget_invalidate_paint(widget);
}

void Rtile_view_set_selected(Rid id, int index, int selected) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TILE_VIEW);
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	if (index < 0 || index >= tiles->count) return;
	rvn_tile_reserve(tiles);
	tiles->selected[index] = (uint8_t)(selected != 0);
	if (selected) tiles->current = index;
	else if (tiles->current == index) tiles->current = -1;
	rvn_widget_invalidate_paint(widget);
}

int Rtile_view_is_selected(Rid id, int index) {
	RVN_WIDGET(widget, id, R_WIDGET_TILE_VIEW, R_FALSE);
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	if (index < 0 || index >= tiles->count || index >= tiles->selected_cap) return R_FALSE;
	return tiles->selected[index] != 0;
}

void Rtile_view_clear_selection(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TILE_VIEW);
	rvn_tile_select_only(widget, -1, R_FALSE);
}

void Rtile_view_select_only(Rid id, int index) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TILE_VIEW);
	rvn_tile_select_only(widget, index, R_FALSE);
}

int Rtile_view_get_current(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TILE_VIEW, -1);
	return ((rvn_tile_view_t*)widget)->current;
}

int Rtile_view_get_selection_count(Rid id) {
	int i, count = 0;
	RVN_WIDGET(widget, id, R_WIDGET_TILE_VIEW, 0);
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	for (i = 0; i < tiles->count && i < tiles->selected_cap; i++) count += tiles->selected[i] != 0;
	return count;
}

int Rtile_view_get_selection(Rid id, int nth) {
	int i, count = 0;
	RVN_WIDGET(widget, id, R_WIDGET_TILE_VIEW, -1);
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	for (i = 0; i < tiles->count && i < tiles->selected_cap; i++) {
		if (!tiles->selected[i]) continue;
		if (count++ == nth) return i;
	}
	return -1;
}

void Rtile_view_scroll_to(Rid id, int index) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TILE_VIEW);
	rvn_tile_reveal(widget, index);
}

int Rtile_view_get_item_at(Rid id, float local_x, float local_y) {
	RVN_WIDGET(widget, id, R_WIDGET_TILE_VIEW, -1);
	return rvn_tile_at(widget, local_x, local_y);
}

void Rtile_view_set_drop_highlight(Rid id, int index) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TILE_VIEW);
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	if (tiles->drop_index == index) return;
	tiles->drop_index = index;
	rvn_widget_invalidate_paint(widget);
}

void Rtile_view_set_drag_type(Rid id, const char* type) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TILE_VIEW);
	rvn_tile_view_t* tiles = (rvn_tile_view_t*)widget;
	rvn_free(tiles->drag_type);
	tiles->drag_type = type && type[0] ? rvn_strdup(type) : NULL;
}

void Rtile_view_set_drag_data_callback(Rid id, RTileDragDataCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TILE_VIEW);
	((rvn_tile_view_t*)widget)->drag_data_callback = callback;
	((rvn_tile_view_t*)widget)->drag_data_user = user_data;
}

void Rtile_view_set_selection_changed_callback(Rid id, RTileEventCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TILE_VIEW);
	((rvn_tile_view_t*)widget)->selection_callback = callback;
	((rvn_tile_view_t*)widget)->selection_user = user_data;
}

void Rtile_view_set_activated_callback(Rid id, RTileEventCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TILE_VIEW);
	((rvn_tile_view_t*)widget)->activated_callback = callback;
	((rvn_tile_view_t*)widget)->activated_user = user_data;
}

void Rtile_view_set_context_callback(Rid id, RTileContextCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TILE_VIEW);
	((rvn_tile_view_t*)widget)->context_callback = callback;
	((rvn_tile_view_t*)widget)->context_user = user_data;
}
