/**
 * Raven - widgets/table.c
 * Table (Slate SListView with a header row): rows of cells under columns
 * the user resizes and sorts, one row selected. Like the ListView, rows are
 * virtualized: only the visible ones are drawn, so a table of a million
 * rows costs the same as a table of ten.
 *
 * The cells are either stored by the table (Rtable_add_row, Rtable_set_cell)
 * or pulled from the application when they are drawn (Rtable_set_virtual_rows).
 *
 * Sorting a stored table moves its rows: the indices are always the order
 * shown. A virtual table only shows which column is sorted and tells the
 * application (Rtable_set_sort_callback), which orders its data itself.
 */

#include "common.h"
#include "../xml/xml.h"

#include <stdlib.h>

/* Distance from the line between two headers that grabs it, in logical units. */
#define RVN_TABLE_GRIP 4.0f
#define RVN_TABLE_MIN_COLUMN 24.0f
#define RVN_TABLE_DEFAULT_COLUMN 120.0f
/* Rows measured when a column is fitted to its content (double click on its edge). */
#define RVN_TABLE_FIT_ROWS 2000

typedef struct rvn_table_column_t {
	char* header;
	/** Width given (> 0), or the width it had when the user last dragged it. */
	float width;
	/** > 0: share of the room left by the other columns (Fill in XML); the width is then a minimum. */
	float fill;
	/** Width as laid out. */
	float shown;
	/** 0 = left, 0.5 = center, 1 = right. */
	float align;
	int sortable;
} rvn_table_column_t;

typedef struct rvn_table_t {
	rvn_widget_t base;
	rvn_table_column_t* columns;
	uint32_t column_count;
	uint32_t column_cap;
	/* stored mode: row_count rows of column_count cells, row after row (NULL: empty) */
	char** cells;
	uint32_t row_count;
	uint32_t row_cap;
	/* virtual mode */
	int is_virtual;
	int virtual_count;
	RTableCellTextCallback cell_callback;
	void* cell_user;

	int selected;
	int hot;
	int hot_header;
	int pressed_header;
	/** Column whose right edge is being dragged, or -1. */
	int resizing;
	float resize_from_x, resize_from_width;
	int sort_column;
	int sort_descending;
	int show_header;
	int grid_lines;
	int striped;
	rvn_scrollbar_t bar;
	rvn_scrollbar_t hbar;
	RSpring select_y;
	RTableEventCallback selection_callback;
	void* selection_user;
	RTableEventCallback activated_callback;
	void* activated_user;
	RTableSortCallback sort_callback;
	void* sort_user;
} rvn_table_t;

/* Where the parts of the table are, in the coordinates of the widget. */
typedef struct rvn_table_geometry_t {
	float header;
	rvn_rect_t body;
	rvn_rect_t vtrack;
	rvn_rect_t htrack;
} rvn_table_geometry_t;

static void rvn_table_init(rvn_widget_t* widget) {
	rvn_table_t* table = (rvn_table_t*)widget;
	table->selected = -1;
	table->hot = -1;
	table->hot_header = -1;
	table->pressed_header = -1;
	table->resizing = -1;
	table->sort_column = -1;
	table->show_header = R_TRUE;
}

static void rvn_table_free_rows(rvn_table_t* table) {
	size_t i, n = (size_t)table->row_count * table->column_count;
	for (i = 0; i < n; i++) rvn_free(table->cells[i]);
	table->row_count = 0;
}

static void rvn_table_destroy(rvn_widget_t* widget) {
	rvn_table_t* table = (rvn_table_t*)widget;
	uint32_t i;
	rvn_table_free_rows(table);
	rvn_free(table->cells);
	for (i = 0; i < table->column_count; i++) rvn_free(table->columns[i].header);
	rvn_free(table->columns);
}

static int rvn_table_count(const rvn_table_t* table) {
	return table->is_virtual ? table->virtual_count : (int)table->row_count;
}

static const char* rvn_table_cell(rvn_table_t* table, int row, int column) {
	const char* text;
	if (row < 0 || row >= rvn_table_count(table) || column < 0 || column >= (int)table->column_count) return "";
	if (!table->is_virtual) {
		text = table->cells[(size_t)row * table->column_count + (size_t)column];
		return text ? text : "";
	}
	text = table->cell_callback ? table->cell_callback(table->base.id, row, column, table->cell_user) : NULL;
	return text ? text : "";
}

static float rvn_table_row_height(const rvn_widget_t* widget) {
	return rvn_widget_metric(widget, R_METRIC_ROW_HEIGHT);
}

static float rvn_table_header_height(const rvn_widget_t* widget) {
	const rvn_table_t* table = (const rvn_table_t*)widget;
	return table->show_header && table->column_count > 0 ? rvn_table_row_height(widget) + 4.0f : 0.0f;
}

/* Lays the columns out in `room` and returns their total width. */
static float rvn_table_lay_columns(rvn_table_t* table, float room) {
	float fixed = 0.0f, fills = 0.0f, total = 0.0f, left;
	uint32_t i;
	for (i = 0; i < table->column_count; i++) {
		rvn_table_column_t* column = &table->columns[i];
		column->shown = rvn_maxf(column->width, RVN_TABLE_MIN_COLUMN);
		if (column->fill > 0.0f) fills += column->fill;
		fixed += column->shown;
	}
	/* the filling columns share what the others leave, never below their own width */
	left = room - fixed;
	for (i = 0; i < table->column_count; i++) {
		rvn_table_column_t* column = &table->columns[i];
		if (column->fill > 0.0f && left > 0.0f && fills > 0.0f) column->shown += floorf(left * column->fill / fills);
		total += column->shown;
	}
	return total;
}

static void rvn_table_geometry(rvn_widget_t* widget, rvn_table_geometry_t* geometry) {
	rvn_table_t* table = (rvn_table_t*)widget;
	float size = rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE);
	float header = rvn_table_header_height(widget);
	float inner_w = rvn_maxf(0.0f, widget->rect.w - 2.0f), inner_h = rvn_maxf(0.0f, widget->rect.h - 2.0f - header);
	float content = rvn_table_row_height(widget) * (float)rvn_table_count(table);
	float total;
	int vertical, horizontal;

	/* each bar takes room from the other direction: decide both together */
	vertical = content > inner_h + 0.5f;
	total = rvn_table_lay_columns(table, inner_w - (vertical ? size : 0.0f));
	horizontal = total > inner_w - (vertical ? size : 0.0f) + 0.5f;
	if (horizontal && !vertical && content > inner_h - size + 0.5f) {
		vertical = R_TRUE;
		total = rvn_table_lay_columns(table, inner_w - size);
	}
	geometry->header = header;
	geometry->body = rvn_rect(1.0f, 1.0f + header, rvn_maxf(0.0f, inner_w - (vertical ? size : 0.0f)),
		rvn_maxf(0.0f, inner_h - (horizontal ? size : 0.0f)));
	geometry->vtrack = rvn_rect(widget->rect.w - 1.0f - size, geometry->body.y, vertical ? size : 0.0f, geometry->body.h);
	geometry->htrack = rvn_rect(1.0f, geometry->body.y + geometry->body.h, geometry->body.w, horizontal ? size : 0.0f);
	table->bar.content = content;
	table->bar.viewport = geometry->body.h;
	table->hbar.content = total;
	table->hbar.viewport = geometry->body.w;
}

static rvn_vec2_t rvn_table_measure(rvn_widget_t* widget) {
	rvn_table_t* table = (rvn_table_t*)widget;
	float width = 0.0f;
	uint32_t i;
	for (i = 0; i < table->column_count; i++) width += rvn_maxf(table->columns[i].width, RVN_TABLE_MIN_COLUMN);
	/* a table is usually given its size by a fill slot; this is only a sensible default */
	return rvn_vec2(rvn_clampf(width + 2.0f, 160.0f, 640.0f),
		rvn_table_header_height(widget) + rvn_table_row_height(widget) * (float)rvn_clampi(rvn_table_count(table), 3, 8) + 2.0f);
}

static void rvn_table_arrange(rvn_widget_t* widget) {
	rvn_table_t* table = (rvn_table_t*)widget;
	rvn_table_geometry_t geometry;
	rvn_table_geometry(widget, &geometry);
	rvn_scrollbar_clamp(&table->bar);
	rvn_scrollbar_clamp(&table->hbar);
}

static int rvn_table_row_at(rvn_widget_t* widget, float x, float y) {
	rvn_table_t* table = (rvn_table_t*)widget;
	rvn_table_geometry_t geometry;
	int row;
	rvn_table_geometry(widget, &geometry);
	if (!rvn_rect_contains(geometry.body, x, y)) return -1;
	row = (int)floorf((y - geometry.body.y + table->bar.offset) / rvn_table_row_height(widget));
	return (row >= 0 && row < rvn_table_count(table)) ? row : -1;
}

/* The column under x in the header, and through `edge` the column whose right edge is there (-1: none). */
static int rvn_table_header_at(rvn_widget_t* widget, float x, float y, int* edge) {
	rvn_table_t* table = (rvn_table_t*)widget;
	rvn_table_geometry_t geometry;
	float left;
	uint32_t i;
	int found = -1;
	*edge = -1;
	rvn_table_geometry(widget, &geometry);
	if (geometry.header <= 0.0f || y < 1.0f || y >= 1.0f + geometry.header || x < 1.0f || x >= 1.0f + geometry.body.w) return -1;
	left = 1.0f - table->hbar.offset;
	for (i = 0; i < table->column_count; i++) {
		float right = left + table->columns[i].shown;
		if (fabsf(x - right) <= RVN_TABLE_GRIP) *edge = (int)i;
		if (x >= left && x < right) found = (int)i;
		left = right;
	}
	return found;
}

static void rvn_table_reveal(rvn_widget_t* widget, int row) {
	rvn_table_t* table = (rvn_table_t*)widget;
	rvn_table_geometry_t geometry;
	float height = rvn_table_row_height(widget), top = height * (float)row;
	if (row < 0) return;
	/* the content may have changed since the last arrange */
	rvn_table_geometry(widget, &geometry);
	if (top < table->bar.offset) rvn_scrollbar_set(&table->bar, top);
	else if (top + height > table->bar.offset + table->bar.viewport) rvn_scrollbar_set(&table->bar, top + height - table->bar.viewport);
	rvn_widget_invalidate_paint(widget);
}

static void rvn_table_select(rvn_widget_t* widget, int index, int notify) {
	rvn_table_t* table = (rvn_table_t*)widget;
	if (index < -1 || index >= rvn_table_count(table)) index = -1;
	if (table->selected == index) return;
	table->selected = index;
	rvn_table_reveal(widget, index);
	rvn_widget_invalidate_paint(widget);
	if (notify && table->selection_callback) table->selection_callback(widget->id, index, table->selection_user);
}

static void rvn_table_activate(rvn_widget_t* widget) {
	rvn_table_t* table = (rvn_table_t*)widget;
	if (table->selected < 0) return;
	if (table->activated_callback) table->activated_callback(widget->id, table->selected, table->activated_user);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

/* ---------------------------------------------------------------------------
 *  Sorting
 * ------------------------------------------------------------------------- */

typedef struct rvn_table_sort_key_t {
	const char* text;
	double number;
	int is_number;
	uint32_t row;
} rvn_table_sort_key_t;

static int rvn_table_parse_number(const char* text, double* number) {
	char* end = NULL;
	while (*text == ' ') text++;
	if (!*text) return R_FALSE;
	*number = strtod(text, &end);
	if (!end || end == text) return R_FALSE;
	while (*end == ' ' || *end == '%') end++;
	return *end == '\0';
}

/* (qsort has no context: sorting happens on the UI thread only) */
static int g_rvn_table_descending;

static int rvn_table_compare_keys(const void* a, const void* b) {
	const rvn_table_sort_key_t* x = (const rvn_table_sort_key_t*)a;
	const rvn_table_sort_key_t* y = (const rvn_table_sort_key_t*)b;
	int order;
	/* numbers by value, before the texts; texts without regard to case */
	if (x->is_number && y->is_number) order = x->number < y->number ? -1 : (x->number > y->number ? 1 : 0);
	else if (x->is_number != y->is_number) order = x->is_number ? -1 : 1;
	else {
		order = rvn_stricmp(x->text, y->text);
		if (order == 0) order = strcmp(x->text, y->text);
	}
	if (g_rvn_table_descending) order = -order;
	/* the rows that compare equal keep their order, either way */
	if (order == 0) order = x->row < y->row ? -1 : (x->row > y->row ? 1 : 0);
	return order;
}

/* Stored rows: puts them in the order of `column`; the selection stays on its row. */
static void rvn_table_sort_rows(rvn_table_t* table, int column, int descending) {
	uint32_t i, n = table->row_count, columns = table->column_count;
	rvn_table_sort_key_t* keys;
	char** sorted;
	int selected = -1;
	if (n < 2 || column < 0 || column >= (int)columns) return;
	keys = (rvn_table_sort_key_t*)rvn_alloc(sizeof(rvn_table_sort_key_t) * n);
	for (i = 0; i < n; i++) {
		const char* text = table->cells[(size_t)i * columns + (size_t)column];
		keys[i].text = text ? text : "";
		keys[i].is_number = rvn_table_parse_number(keys[i].text, &keys[i].number);
		keys[i].row = i;
	}
	g_rvn_table_descending = descending;
	qsort(keys, n, sizeof(rvn_table_sort_key_t), rvn_table_compare_keys);
	sorted = (char**)rvn_calloc(sizeof(char*) * (size_t)table->row_cap * columns);
	for (i = 0; i < n; i++) {
		memcpy(&sorted[(size_t)i * columns], &table->cells[(size_t)keys[i].row * columns], sizeof(char*) * columns);
		if ((int)keys[i].row == table->selected) selected = (int)i;
	}
	rvn_free(keys);
	rvn_free(table->cells);
	table->cells = sorted;
	table->selected = selected;
	table->select_y.started = R_FALSE;
}

static void rvn_table_sort(rvn_widget_t* widget, int column, int descending, int notify) {
	rvn_table_t* table = (rvn_table_t*)widget;
	if (column < -1 || column >= (int)table->column_count) column = -1;
	table->sort_column = column;
	table->sort_descending = descending ? R_TRUE : R_FALSE;
	if (!table->is_virtual && column >= 0) rvn_table_sort_rows(table, column, table->sort_descending);
	rvn_table_reveal(widget, table->selected);
	rvn_widget_invalidate_paint(widget);
	if (notify && table->sort_callback) table->sort_callback(widget->id, column, table->sort_descending, table->sort_user);
}

/* Width that shows the header and the texts of a column (its edge double clicked). */
static void rvn_table_fit_column(rvn_widget_t* widget, int column) {
	rvn_table_t* table = (rvn_table_t*)widget;
	float padding = rvn_widget_metric(widget, R_METRIC_PADDING_X) * 0.8f, width;
	int i, count = rvn_mini(rvn_table_count(table), RVN_TABLE_FIT_ROWS);
	rvn_table_column_t* col = &table->columns[column];
	width = rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, col->header ? col->header : "") + (col->sortable ? 14.0f : 0.0f);
	for (i = 0; i < count; i++) width = rvn_maxf(width, rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, rvn_table_cell(table, i, column)));
	col->width = ceilf(width + padding * 2.0f + 2.0f);
	col->fill = 0.0f;
	rvn_widget_invalidate_layout(widget);
}

/* ---------------------------------------------------------------------------
 *  Painting
 * ------------------------------------------------------------------------- */

static void rvn_table_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_table_t* table = (rvn_table_t*)widget;
	rvn_table_geometry_t geometry;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	float row = rvn_table_row_height(widget), padding = rvn_widget_metric(widget, R_METRIC_PADDING_X) * 0.8f;
	float font = rvn_widget_font_size(widget, 0.0f);
	int focused = (widget->flags & RVN_WF_FOCUSED) != 0;
	int platform = rvn_widget_family(widget) != RVN_FAMILY_RAVEN;
	int count = rvn_table_count(table), first, last, i;
	float offset, left;
	uint32_t c;
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	rvn_color_t selection = rvn_widget_color(widget, R_COLOR_SELECTION);
	rvn_color_t separator = rvn_widget_color(widget, R_COLOR_SEPARATOR);
	rvn_vec2_t origin = rvn_widget_origin(widget);
	float mouse_x = widget->window ? widget->window->mouse_x - origin.x : -1.0f;
	float mouse_y = widget->window ? widget->window->mouse_y - origin.y : -1.0f;

	rvn_table_geometry(widget, &geometry);
	offset = rvn_scrollbar_shown(&table->bar, painter);
	left = geometry.body.x - rvn_scrollbar_shown(&table->hbar, painter);

	if (platform) {
		rvn_platform_paint_list_frame(painter, widget, rect, rvn_widget_states(widget, painter, R_FALSE, R_FALSE));
	} else {
		rvn_paint_box(painter, rect, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS), rvn_widget_color(widget, R_COLOR_INPUT),
			rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH), rvn_widget_color(widget, focused ? R_COLOR_FOCUS : R_COLOR_BORDER));
	}

	/* ---- the header ---- */
	if (geometry.header > 0.0f) {
		float x = left;
		rvn_paint_push_clip(painter, rvn_rect(1.0f, 1.0f, rect.w - 2.0f, geometry.header));
		rvn_paint_rect(painter, rvn_rect(1.0f, 1.0f, rect.w - 2.0f, geometry.header), rvn_widget_color(widget, R_COLOR_HEADER));
		for (c = 0; c < table->column_count; c++) {
			rvn_table_column_t* column = &table->columns[c];
			rvn_rect_t cell = rvn_rect(x, 1.0f, column->shown, geometry.header);
			float arrow = (int)c == table->sort_column ? 14.0f : 0.0f;
			if (((int)c == table->hot_header && (widget->flags & RVN_WF_HOVERED) && column->sortable && painter->enabled) ||
				(int)c == table->pressed_header) {
				rvn_paint_rect(painter, cell, rvn_widget_color(widget, R_COLOR_HEADER_HOVERED));
			}
			rvn_paint_push_clip(painter, rvn_rect(cell.x + padding, cell.y, rvn_maxf(0.0f, cell.w - padding * 2.0f - arrow), cell.h));
			rvn_paint_text_in(painter, R_INVALID_ID, font, rvn_rect(cell.x + padding, cell.y, rvn_maxf(0.0f, cell.w - padding * 2.0f - arrow), cell.h),
				column->align, column->header ? column->header : "", -1, text);
			rvn_paint_pop_clip(painter);
			if ((int)c == table->sort_column) {
				rvn_paint_icon(painter, table->sort_descending ? RVN_ICON_CHEVRON_DOWN : RVN_ICON_CHEVRON_UP,
					cell.x + cell.w - padding - 5.0f, cell.y + cell.h * 0.5f, 6.0f, rvn_color_fade(text, 0.8f));
			}
			/* the edge the user drags */
			rvn_paint_rect(painter, rvn_rect(cell.x + cell.w - 1.0f, cell.y + 5.0f, 1.0f, rvn_maxf(0.0f, cell.h - 10.0f)),
				(int)c == table->resizing ? rvn_widget_color(widget, R_COLOR_ACCENT) : separator);
			x += column->shown;
		}
		rvn_paint_rect(painter, rvn_rect(1.0f, geometry.header, rect.w - 2.0f, 1.0f), separator);
		rvn_paint_pop_clip(painter);
	}

	/* ---- the rows ---- */
	rvn_paint_push_clip(painter, geometry.body);
	first = rvn_maxi((int)floorf(offset / row), 0);
	last = rvn_mini(count, first + (int)(geometry.body.h / row) + 3);
	if (table->striped) {
		rvn_color_t stripe = rvn_color_fade(rvn_widget_color(widget, R_COLOR_ROW_HOVERED), 0.45f);
		for (i = first; i < last; i++) {
			if (i % 2 == 1) rvn_paint_rect(painter, rvn_rect(geometry.body.x, geometry.body.y + row * (float)i - offset, geometry.body.w, row), stripe);
		}
	}
	if (table->selected >= 0) {
		/* the selection slides from the previous row to the new one; a table without the focus shows it dimmer */
		float y = rvn_paint_spring(painter, &table->select_y, row * (float)table->selected, R_MOTION_SNAPPY);
		rvn_rect_t line = rvn_rect(geometry.body.x, geometry.body.y + y - offset, geometry.body.w, row);
		if (platform) {
			rvn_platform_paint_item(painter, widget, rvn_platform_row(widget, line),
				rvn_widget_states(widget, painter, table->hot == table->selected && (widget->flags & RVN_WF_HOVERED), R_FALSE) | RVN_STATE_SELECTED,
				RVN_ITEM_LIST, 1.0f);
		} else {
			rvn_paint_row(painter, widget, line, focused ? selection : rvn_color_mix(selection, rvn_widget_color(widget, R_COLOR_INPUT), 0.45f));
		}
	} else {
		table->select_y.started = R_FALSE;
	}
	for (i = first; i < last; i++) {
		float y = geometry.body.y + row * (float)i - offset, x = left;
		rvn_color_t color = text;
		if (i != table->selected && i == table->hot && (widget->flags & RVN_WF_HOVERED) && painter->enabled) {
			rvn_rect_t line = rvn_rect(geometry.body.x, y, geometry.body.w, row);
			if (platform) {
				uint32_t states = rvn_widget_states(widget, painter, R_TRUE, R_FALSE);
				rvn_platform_paint_item(painter, widget, rvn_platform_row(widget, line), states, RVN_ITEM_LIST, 0.0f);
			} else {
				rvn_paint_row(painter, widget, line, rvn_widget_color(widget, R_COLOR_ROW_HOVERED));
			}
		}
		if (platform) {
			uint32_t states = rvn_widget_states(widget, painter, i == table->hot && (widget->flags & RVN_WF_HOVERED), R_FALSE);
			rvn_color_t item = rvn_platform_item_foreground(widget, states | (i == table->selected ? RVN_STATE_SELECTED : 0), RVN_ITEM_LIST);
			if (rvn_color_a(item)) color = item;
		}
		for (c = 0; c < table->column_count; c++) {
			rvn_table_column_t* column = &table->columns[c];
			rvn_rect_t cell = rvn_rect(x + padding, y, rvn_maxf(0.0f, column->shown - padding * 2.0f), row);
			if (x + column->shown >= geometry.body.x && x <= geometry.body.x + geometry.body.w) {
				rvn_paint_push_clip(painter, cell);
				rvn_paint_text_in(painter, R_INVALID_ID, font, cell, column->align, rvn_table_cell(table, i, (int)c), -1, color);
				rvn_paint_pop_clip(painter);
			}
			x += column->shown;
		}
		if (table->grid_lines) rvn_paint_rect(painter, rvn_rect(geometry.body.x, y + row - 1.0f, geometry.body.w, 1.0f), rvn_color_fade(separator, 0.6f));
	}
	if (table->grid_lines) {
		float x = left;
		for (c = 0; c + 1 < table->column_count; c++) {
			x += table->columns[c].shown;
			rvn_paint_rect(painter, rvn_rect(x - 1.0f, geometry.body.y, 1.0f, rvn_minf(geometry.body.h, row * (float)count - offset)),
				rvn_color_fade(separator, 0.6f));
		}
	}
	rvn_paint_pop_clip(painter);

	if (geometry.vtrack.w > 0.0f) rvn_scrollbar_paint(&table->bar, widget, painter, geometry.vtrack, R_TRUE, mouse_x, mouse_y);
	if (geometry.htrack.h > 0.0f) rvn_scrollbar_paint(&table->hbar, widget, painter, geometry.htrack, R_FALSE, mouse_x, mouse_y);
}

/* ---------------------------------------------------------------------------
 *  Input
 * ------------------------------------------------------------------------- */

static int rvn_table_event(rvn_widget_t* widget, const REvent* event) {
	rvn_table_t* table = (rvn_table_t*)widget;
	rvn_table_geometry_t geometry;
	int count = rvn_table_count(table), changed = R_FALSE, row, page, edge, header;

	if (table->resizing < 0 && (event->type == R_EVENT_MOUSE_DOWN || event->type == R_EVENT_MOUSE_MOVE || event->type == R_EVENT_MOUSE_UP)) {
		rvn_table_geometry(widget, &geometry);
		if ((geometry.vtrack.w > 0.0f || table->bar.dragging) &&
			rvn_scrollbar_event(&table->bar, widget, event, geometry.vtrack, R_TRUE, &changed)) {
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		}
		if ((geometry.htrack.h > 0.0f || table->hbar.dragging) &&
			rvn_scrollbar_event(&table->hbar, widget, event, geometry.htrack, R_FALSE, &changed)) {
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		}
	}
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			if (table->resizing >= 0) {
				rvn_table_column_t* column = &table->columns[table->resizing];
				column->width = rvn_maxf(RVN_TABLE_MIN_COLUMN, roundf(table->resize_from_width + event->local_x - table->resize_from_x));
				column->fill = 0.0f;
				rvn_widget_invalidate_layout(widget);
				return R_TRUE;
			}
			header = rvn_table_header_at(widget, event->local_x, event->local_y, &edge);
			widget->cursor = edge >= 0 ? R_CURSOR_RESIZE_H : (RCursor)0;
			row = rvn_table_row_at(widget, event->local_x, event->local_y);
			if (row != table->hot || header != table->hot_header) {
				table->hot = row;
				table->hot_header = header;
				rvn_widget_invalidate_paint(widget);
			}
			return R_FALSE;
		case R_EVENT_MOUSE_LEAVE:
			table->hot = -1;
			table->hot_header = -1;
			if (table->resizing < 0) widget->cursor = (RCursor)0;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			header = rvn_table_header_at(widget, event->local_x, event->local_y, &edge);
			if (edge >= 0) {
				if (event->clicks == 2) {
					rvn_table_fit_column(widget, edge);
					return R_TRUE;
				}
				table->resizing = edge;
				table->resize_from_x = event->local_x;
				table->resize_from_width = table->columns[edge].shown;
				rvn_widget_capture_mouse(widget);
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			if (header >= 0) {
				table->pressed_header = table->columns[header].sortable ? header : -1;
				if (table->pressed_header >= 0) rvn_widget_capture_mouse(widget);
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			row = rvn_table_row_at(widget, event->local_x, event->local_y);
			if (row >= 0) {
				rvn_table_select(widget, row, R_TRUE);
				if (event->clicks == 2 && rvn_widget_alive(widget)) rvn_table_activate(widget);
			}
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			if (table->resizing >= 0) {
				table->resizing = -1;
				rvn_widget_release_mouse(widget);
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			if (table->pressed_header >= 0) {
				int pressed = table->pressed_header;
				table->pressed_header = -1;
				rvn_widget_release_mouse(widget);
				rvn_widget_invalidate_paint(widget);
				header = rvn_table_header_at(widget, event->local_x, event->local_y, &edge);
				if (header == pressed) {
					/* a first click sorts up, the next ones turn the order around */
					rvn_table_sort(widget, pressed, pressed == table->sort_column ? !table->sort_descending : R_FALSE, R_TRUE);
				}
				return R_TRUE;
			}
			return R_FALSE;
		case R_EVENT_MOUSE_WHEEL: {
			/* the wheel scrolls the rows; with Shift, or sideways, the columns */
			float along = event->wheel_y, across = event->wheel_x;
			if (event->modifiers & R_MOD_SHIFT) {
				across = along;
				along = 0.0f;
			}
			rvn_table_geometry(widget, &geometry);
			if (along != 0.0f) changed |= rvn_scrollbar_wheel(&table->bar, -along * rvn_table_row_height(widget) * 3.0f);
			if (across != 0.0f) changed |= rvn_scrollbar_wheel(&table->hbar, -across * 40.0f);
			if (changed) table->hot = rvn_table_row_at(widget, event->local_x, event->local_y);
			rvn_widget_invalidate_paint(widget);
			return changed;
		}
		case R_EVENT_FOCUS_GAINED:
		case R_EVENT_FOCUS_LOST:
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_KEY_DOWN:
			rvn_table_geometry(widget, &geometry);
			switch (event->key) {
				case R_KEY_LEFT:
				case R_KEY_RIGHT:
					if (!rvn_scrollbar_needed(&table->hbar)) return R_FALSE;
					rvn_scrollbar_set(&table->hbar, table->hbar.offset + (event->key == R_KEY_LEFT ? -40.0f : 40.0f));
					rvn_widget_invalidate_paint(widget);
					return R_TRUE;
				default:
					break;
			}
			if (count == 0) return R_FALSE;
			page = rvn_maxi(1, (int)(table->bar.viewport / rvn_table_row_height(widget)) - 1);
			switch (event->key) {
				case R_KEY_UP:
					row = table->selected < 0 ? count - 1 : table->selected - 1;
					break;
				case R_KEY_DOWN:
					row = table->selected + 1;
					break;
				case R_KEY_PAGE_UP:
					row = table->selected - page;
					break;
				case R_KEY_PAGE_DOWN:
					row = table->selected + page;
					break;
				case R_KEY_HOME:
					row = 0;
					break;
				case R_KEY_END:
					row = count - 1;
					break;
				case R_KEY_ENTER:
				case R_KEY_KP_ENTER:
					rvn_table_activate(widget);
					return R_TRUE;
				default:
					return R_FALSE;
			}
			rvn_table_select(widget, rvn_clampi(row, 0, count - 1), R_TRUE);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static int rvn_table_pan(rvn_widget_t* widget, float dx, float dy, int coasting) {
	rvn_table_t* table = (rvn_table_t*)widget;
	int moved = rvn_scrollbar_pan(&table->bar, -dy, coasting);
	moved |= rvn_scrollbar_pan(&table->hbar, -dx, coasting);
	return moved;
}

/* ---------------------------------------------------------------------------
 *  Contents
 * ------------------------------------------------------------------------- */

static int rvn_table_add_column(rvn_widget_t* widget, const char* header, float width) {
	rvn_table_t* table = (rvn_table_t*)widget;
	rvn_table_column_t* column;
	uint32_t old = table->column_count, r;
	RVN_ARRAY_RESERVE(table->columns, table->column_cap, table->column_count + 1);
	column = &table->columns[table->column_count++];
	memset(column, 0, sizeof(*column));
	column->header = rvn_strdup(header ? header : "");
	column->width = width > 0.0f ? width : RVN_TABLE_DEFAULT_COLUMN;
	column->sortable = R_TRUE;
	/* the stored rows get an empty cell in the new column */
	if (table->row_count > 0) {
		char** cells = (char**)rvn_calloc(sizeof(char*) * (size_t)table->row_cap * table->column_count);
		for (r = 0; r < table->row_count; r++) memcpy(&cells[(size_t)r * table->column_count], &table->cells[(size_t)r * old], sizeof(char*) * old);
		rvn_free(table->cells);
		table->cells = cells;
	} else {
		rvn_free(table->cells);
		table->cells = NULL;
		table->row_cap = 0;
	}
	rvn_widget_invalidate_layout(widget);
	return (int)table->column_count - 1;
}

static void rvn_table_clear_rows(rvn_widget_t* widget) {
	rvn_table_t* table = (rvn_table_t*)widget;
	rvn_table_free_rows(table);
	table->selected = -1;
	table->hot = -1;
	table->bar.offset = 0.0f;
	rvn_widget_invalidate_layout(widget);
}

static void rvn_table_clear_columns(rvn_widget_t* widget) {
	rvn_table_t* table = (rvn_table_t*)widget;
	uint32_t i;
	rvn_table_clear_rows(widget);
	rvn_free(table->cells);
	table->cells = NULL;
	table->row_cap = 0;
	for (i = 0; i < table->column_count; i++) rvn_free(table->columns[i].header);
	table->column_count = 0;
	table->sort_column = -1;
	table->hbar.offset = 0.0f;
	rvn_widget_invalidate_layout(widget);
}

/* Adds a row of cells separated by '|' (NULL: empty), as many as there are columns. */
static int rvn_table_add_row(rvn_widget_t* widget, const char* cells) {
	rvn_table_t* table = (rvn_table_t*)widget;
	uint32_t columns = table->column_count, c;
	char** row;
	if (columns == 0) return -1;
	if (table->row_count + 1 > table->row_cap) {
		uint32_t cap = table->row_cap ? table->row_cap * 2 : 16;
		char** grown = (char**)rvn_calloc(sizeof(char*) * (size_t)cap * columns);
		if (table->cells) memcpy(grown, table->cells, sizeof(char*) * (size_t)table->row_count * columns);
		rvn_free(table->cells);
		table->cells = grown;
		table->row_cap = cap;
	}
	row = &table->cells[(size_t)table->row_count * columns];
	for (c = 0; c < columns; c++) {
		size_t length = 0;
		row[c] = NULL;
		if (!cells) continue;
		length = strcspn(cells, "|");
		if (length > 0) row[c] = rvn_strndup(cells, length);
		cells += length;
		if (*cells == '|') cells++;
		else cells = NULL;
	}
	table->row_count++;
	rvn_widget_invalidate_layout(widget);
	return (int)table->row_count - 1;
}

static int rvn_parse_table_align(const char* value, float* align) {
	if (rvn_stricmp(value, "Left") == 0 || rvn_stricmp(value, "Start") == 0) *align = 0.0f;
	else if (rvn_stricmp(value, "Center") == 0) *align = 0.5f;
	else if (rvn_stricmp(value, "Right") == 0 || rvn_stricmp(value, "End") == 0) *align = 1.0f;
	else return rvn_parse_float(value, align);
	return R_TRUE;
}

static int rvn_table_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_table_t* table = (rvn_table_t*)widget;
	int number;
	if (RVN_ATTR("Columns")) {
		rvn_table_clear_columns(widget);
		while (*value) {
			size_t length = strcspn(value, "|");
			char* header = rvn_strndup(value, length);
			rvn_table_add_column(widget, header, 0.0f);
			rvn_free(header);
			value += length;
			if (*value == '|') value++;
		}
		return R_TRUE;
	}
	if (RVN_ATTR("Selected")) {
		if (!rvn_parse_int(value, &number)) return R_FALSE;
		rvn_table_select(widget, number, R_FALSE);
		return R_TRUE;
	}
	if (RVN_ATTR("SortColumn")) {
		if (!rvn_parse_int(value, &number)) return R_FALSE;
		rvn_table_sort(widget, number, table->sort_descending, R_FALSE);
		return R_TRUE;
	}
	if (RVN_ATTR("SortDescending")) {
		if (!rvn_parse_bool(value, &number)) return R_FALSE;
		rvn_table_sort(widget, table->sort_column, number, R_FALSE);
		return R_TRUE;
	}
	if (RVN_ATTR("ShowHeader")) {
		if (!rvn_parse_bool(value, &table->show_header)) return R_FALSE;
	} else if (RVN_ATTR("GridLines")) {
		if (!rvn_parse_bool(value, &table->grid_lines)) return R_FALSE;
	} else if (RVN_ATTR("Striped")) {
		if (!rvn_parse_bool(value, &table->striped)) return R_FALSE;
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_layout(widget);
	return R_TRUE;
}

static int rvn_table_xml_child(rvn_widget_t* widget, const rvn_xml_node_t* node) {
	rvn_table_t* table = (rvn_table_t*)widget;
	const char* value;
	if (rvn_stricmp(node->name, "Column") == 0) {
		float width = 0.0f;
		int index;
		rvn_table_column_t* column;
		value = rvn_xml_attribute(node, "Width");
		if (value && !rvn_parse_float(value, &width)) return R_FALSE;
		value = rvn_xml_attribute(node, "Header");
		index = rvn_table_add_column(widget, value ? value : node->text, width);
		column = &table->columns[index];
		value = rvn_xml_attribute(node, "Fill");
		if (value && !rvn_parse_float(value, &column->fill)) return R_FALSE;
		value = rvn_xml_attribute(node, "Sortable");
		if (value && !rvn_parse_bool(value, &column->sortable)) return R_FALSE;
		value = rvn_xml_attribute(node, "Align");
		if (value && !rvn_parse_table_align(value, &column->align)) return R_FALSE;
		return R_TRUE;
	}
	if (rvn_stricmp(node->name, "Row") == 0) {
		const rvn_xml_node_t* cell;
		int row;
		if (table->is_virtual || table->column_count == 0) return R_FALSE;
		value = rvn_xml_attribute(node, "Cells");
		row = rvn_table_add_row(widget, value ? value : (node->first_child ? NULL : node->text));
		/* <Row><Cell>a</Cell><Cell>b</Cell></Row>: texts that may hold a '|' */
		if (!value && node->first_child) {
			uint32_t c = 0;
			for (cell = node->first_child; cell && c < table->column_count; cell = cell->next) {
				const char* text;
				if (rvn_stricmp(cell->name, "Cell") != 0) continue;
				text = rvn_xml_attribute(cell, "Text");
				rvn_str_set(&table->cells[(size_t)row * table->column_count + c], text ? text : cell->text);
				c++;
			}
		}
		return R_TRUE;
	}
	return R_FALSE;
}

const rvn_widget_class_t rvn_class_table = {
	.type = R_WIDGET_TABLE,
	.name = "Table",
	.size = sizeof(rvn_table_t),
	/* the texts of a virtual table come from an application callback */
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_UI_PAINT | RVN_CF_SURFACE,
	.init = rvn_table_init,
	.destroy = rvn_table_destroy,
	.measure = rvn_table_measure,
	.arrange = rvn_table_arrange,
	.paint = rvn_table_paint,
	.event = rvn_table_event,
	.pan = rvn_table_pan,
	.set_attribute = rvn_table_set_attribute,
	.xml_child = rvn_table_xml_child
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

/* Reports a column that does not exist, and returns `fallback` (RVN_NOTHING in a void function). */
#define RVN_NOTHING
#define RVN_TABLE_COLUMN(table, column, function, fallback) \
	if ((column) < 0 || (column) >= (int)(table)->column_count) { \
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, function ": invalid column %d", (column)); \
		return fallback; \
	}

int Rtable_add_column(Rid id, const char* header, float width) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, -1);
	return rvn_table_add_column(widget, header, width);
}

int Rtable_get_column_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, 0);
	return (int)((rvn_table_t*)widget)->column_count;
}

void Rtable_clear_columns(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_clear_columns(widget);
}

void Rtable_set_column_header(Rid id, int column, const char* header) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_t* table = (rvn_table_t*)widget;
	RVN_TABLE_COLUMN(table, column, "Rtable_set_column_header", RVN_NOTHING)
	if (rvn_str_set(&table->columns[column].header, header ? header : "")) rvn_widget_invalidate_paint(widget);
}

const char* Rtable_get_column_header(Rid id, int column) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, "");
	rvn_table_t* table = (rvn_table_t*)widget;
	RVN_TABLE_COLUMN(table, column, "Rtable_get_column_header", "")
	return table->columns[column].header ? table->columns[column].header : "";
}

void Rtable_set_column_width(Rid id, int column, float width) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_t* table = (rvn_table_t*)widget;
	RVN_TABLE_COLUMN(table, column, "Rtable_set_column_width", RVN_NOTHING)
	table->columns[column].width = rvn_maxf(width, RVN_TABLE_MIN_COLUMN);
	rvn_widget_invalidate_layout(widget);
}

float Rtable_get_column_width(Rid id, int column) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, 0.0f);
	rvn_table_t* table = (rvn_table_t*)widget;
	RVN_TABLE_COLUMN(table, column, "Rtable_get_column_width", 0.0f)
	if (widget->window && widget->window->needs_layout) rvn_window_layout(widget->window);
	return table->columns[column].shown > 0.0f ? table->columns[column].shown : table->columns[column].width;
}

void Rtable_set_column_fill(Rid id, int column, float fill) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_t* table = (rvn_table_t*)widget;
	RVN_TABLE_COLUMN(table, column, "Rtable_set_column_fill", RVN_NOTHING)
	table->columns[column].fill = fill > 0.0f ? fill : 0.0f;
	rvn_widget_invalidate_layout(widget);
}

void Rtable_set_column_sortable(Rid id, int column, int sortable) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_t* table = (rvn_table_t*)widget;
	RVN_TABLE_COLUMN(table, column, "Rtable_set_column_sortable", RVN_NOTHING)
	table->columns[column].sortable = sortable ? R_TRUE : R_FALSE;
	rvn_widget_invalidate_paint(widget);
}

void Rtable_set_column_align(Rid id, int column, float align) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_t* table = (rvn_table_t*)widget;
	RVN_TABLE_COLUMN(table, column, "Rtable_set_column_align", RVN_NOTHING)
	table->columns[column].align = rvn_clampf(align, 0.0f, 1.0f);
	rvn_widget_invalidate_paint(widget);
}

int Rtable_add_row(Rid id, const char* cells) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, -1);
	rvn_table_t* table = (rvn_table_t*)widget;
	if (table->is_virtual) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rtable_add_row: the table is in virtual mode");
		return -1;
	}
	if (table->column_count == 0) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rtable_add_row: the table has no column");
		return -1;
	}
	return rvn_table_add_row(widget, cells);
}

void Rtable_set_cell(Rid id, int row, int column, const char* text) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_t* table = (rvn_table_t*)widget;
	RVN_TABLE_COLUMN(table, column, "Rtable_set_cell", RVN_NOTHING)
	if (table->is_virtual || row < 0 || row >= (int)table->row_count) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtable_set_cell: invalid row %d", row);
		return;
	}
	if (rvn_str_set(&table->cells[(size_t)row * table->column_count + (size_t)column], text ? text : "")) rvn_widget_invalidate_paint(widget);
}

const char* Rtable_get_cell(Rid id, int row, int column) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, "");
	return rvn_table_cell((rvn_table_t*)widget, row, column);
}

void Rtable_remove_row(Rid id, int row) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_t* table = (rvn_table_t*)widget;
	uint32_t columns = table->column_count, c;
	if (table->is_virtual || row < 0 || row >= (int)table->row_count) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtable_remove_row: invalid row %d", row);
		return;
	}
	for (c = 0; c < columns; c++) rvn_free(table->cells[(size_t)row * columns + c]);
	memmove(&table->cells[(size_t)row * columns], &table->cells[(size_t)(row + 1) * columns],
		sizeof(char*) * (size_t)(table->row_count - (uint32_t)row - 1) * columns);
	table->row_count--;
	/* the selection follows the row it was on */
	if (table->selected == row) table->selected = -1;
	else if (table->selected > row) table->selected--;
	table->hot = -1;
	rvn_widget_invalidate_layout(widget);
}

void Rtable_clear_rows(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_clear_rows(widget);
	((rvn_table_t*)widget)->virtual_count = 0;
}

int Rtable_get_row_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, 0);
	return rvn_table_count((rvn_table_t*)widget);
}

void Rtable_set_virtual_rows(Rid id, int count, RTableCellTextCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_t* table = (rvn_table_t*)widget;
	int selected = table->selected;
	float offset = table->bar.offset;
	rvn_table_clear_rows(widget);
	table->is_virtual = callback != NULL;
	table->virtual_count = callback && count > 0 ? count : 0;
	table->cell_callback = callback;
	table->cell_user = user_data;
	/* refreshing the count of a live table keeps its selection and scroll position */
	if (selected < rvn_table_count(table)) table->selected = selected;
	table->bar.offset = offset;
	rvn_widget_invalidate_layout(widget);
}

void Rtable_set_selected(Rid id, int row) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_select(widget, row, R_FALSE);
}

int Rtable_get_selected(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, -1);
	return ((rvn_table_t*)widget)->selected;
}

void Rtable_scroll_to_row(Rid id, int row) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	if (row < 0 || row >= rvn_table_count((rvn_table_t*)widget)) return;
	if (widget->window && widget->window->needs_layout) rvn_window_layout(widget->window);
	rvn_table_reveal(widget, row);
}

void Rtable_sort(Rid id, int column, int descending) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	if (column < -1 || column >= (int)((rvn_table_t*)widget)->column_count) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtable_sort: invalid column %d", column);
		return;
	}
	rvn_table_sort(widget, column, descending, R_FALSE);
}

int Rtable_get_sort_column(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, -1);
	return ((rvn_table_t*)widget)->sort_column;
}

int Rtable_is_sort_descending(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, R_FALSE);
	return ((rvn_table_t*)widget)->sort_descending;
}

void Rtable_set_header_visible(Rid id, int visible) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	((rvn_table_t*)widget)->show_header = visible ? R_TRUE : R_FALSE;
	rvn_widget_invalidate_layout(widget);
}

void Rtable_set_grid_lines(Rid id, int visible) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	((rvn_table_t*)widget)->grid_lines = visible ? R_TRUE : R_FALSE;
	rvn_widget_invalidate_paint(widget);
}

void Rtable_set_striped(Rid id, int striped) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	((rvn_table_t*)widget)->striped = striped ? R_TRUE : R_FALSE;
	rvn_widget_invalidate_paint(widget);
}

void Rtable_set_selection_changed_callback(Rid id, RTableEventCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	((rvn_table_t*)widget)->selection_callback = callback;
	((rvn_table_t*)widget)->selection_user = user_data;
}

void Rtable_set_activated_callback(Rid id, RTableEventCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	((rvn_table_t*)widget)->activated_callback = callback;
	((rvn_table_t*)widget)->activated_user = user_data;
}

void Rtable_set_sort_callback(Rid id, RTableSortCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	((rvn_table_t*)widget)->sort_callback = callback;
	((rvn_table_t*)widget)->sort_user = user_data;
}
