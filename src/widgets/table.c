/**
 * Raven - widgets/table.c
 * Table (Slate SListView with a header row): rows of cells under columns
 * the user resizes and sorts, with optional multiple row selection. Rows are
 * virtualized: only the visible ones are drawn, so a table of a million
 * rows costs the same as a table of ten.
 *
 * The cells are either stored by the table (Rtable_add_row, Rtable_set_cell)
 * or pulled from the application when they are drawn (Rtable_set_virtual_rows).
 *
 * Sorting a stored table moves its rows: the indices are always the order
 * shown. A virtual table only shows which column is sorted and tells the
 * application (Rtable_set_sort_callback), which orders its data itself.
 *
 * The cells of editable columns are edited in place by a TextInput the table
 * holds as its only child, shown over the cell; the application checks and
 * keeps the text (Rtable_set_cell_edited_callback). The first columns may be
 * frozen: they stay where they are while the others scroll under them.
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
	int editable;
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
	rvn_color_t* row_colors;
	rvn_color_t* cell_colors;
	/* virtual mode */
	int is_virtual;
	int virtual_count;
	RTableCellTextCallback cell_callback;
	void* cell_user;
	/* the color of the text of a cell, asked of the application (NULL: the theme's) */
	RTableCellColorCallback color_callback;
	void* color_user;
	int multi;
	int anchor;
	uint8_t* selected_rows;
	uint32_t selection_cap;

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

	/** The first columns that do not scroll sideways. */
	uint32_t frozen;
	/** Column of the current cell (on the selected row), -1: the first editable one. */
	int column;
	/* ---- the editor of a cell: a TextInput over it, made the first time ---- */
	rvn_widget_t* editor;
	int editing;
	int edit_row, edit_column;
	/** The edited callback is running: the focus it moves does not close the editor again. */
	int committing;
	RTableCellEditedCallback edited_callback;
	void* edited_user;
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
	table->anchor = -1;
	table->hot = -1;
	table->hot_header = -1;
	table->pressed_header = -1;
	table->resizing = -1;
	table->sort_column = -1;
	table->show_header = R_TRUE;
	table->column = -1;
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
	rvn_free(table->row_colors);
	rvn_free(table->cell_colors);
	rvn_free(table->selected_rows);
}

static int rvn_table_count(const rvn_table_t* table) {
	return table->is_virtual ? table->virtual_count : (int)table->row_count;
}

int rvn_table_multi_select(const rvn_widget_t* widget) {
	return ((const rvn_table_t*)widget)->multi;
}

static void rvn_table_selection_reserve(rvn_table_t* table) {
	uint32_t before = table->selection_cap;
	RVN_ARRAY_RESERVE(table->selected_rows, table->selection_cap, (uint32_t)rvn_table_count(table));
	if (table->selection_cap > before) memset(table->selected_rows + before, 0, table->selection_cap - before);
}

static int rvn_table_is_selected(const rvn_table_t* table, int row) {
	if (row < 0 || row >= rvn_table_count(table)) return R_FALSE;
	return table->multi ? ((uint32_t)row < table->selection_cap && table->selected_rows[row]) : row == table->selected;
}

static void rvn_table_selection_reset(rvn_table_t* table, int row) {
	if (table->selected_rows) memset(table->selected_rows, 0, table->selection_cap);
	if (table->multi && row >= 0) {
		rvn_table_selection_reserve(table);
		table->selected_rows[row] = 1;
	}
	table->selected = table->anchor = row;
}

static rvn_color_t* rvn_table_repack_colors(rvn_color_t* colors, uint32_t old_columns, uint32_t columns, uint32_t rows, uint32_t capacity) {
	rvn_color_t* grown;
	uint32_t r;
	if (!colors) return NULL;
	grown = (rvn_color_t*)rvn_calloc(sizeof(rvn_color_t) * (size_t)capacity * columns);
	for (r = 0; r < rows; r++) memcpy(grown + (size_t)r * columns, colors + (size_t)r * old_columns,
		sizeof(rvn_color_t) * rvn_mini((int)old_columns, (int)columns));
	rvn_free(colors);
	return grown;
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

/* The color of the text of a cell: the callback's when it gives one, else the
 * stored cell color, else the stored row color (stored rows only), else `fallback`. */
static rvn_color_t rvn_table_cell_color(rvn_table_t* table, int row, int column, rvn_color_t fallback) {
	float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
	rvn_color_t color = 0;
	if (table->color_callback && table->color_callback(table->base.id, row, column, &r, &g, &b, &a, table->color_user) && a > 0.0f)
		return rvn_rgba(r, g, b, a);
	if (!table->is_virtual) {
		if (table->cell_colors) color = table->cell_colors[(size_t)row * table->column_count + (size_t)column];
		if (!rvn_color_a(color) && table->row_colors) color = table->row_colors[row];
	}
	return rvn_color_a(color) ? color : fallback;
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

/* ---------------------------------------------------------------------------
 *  Columns on screen: the frozen ones stay at the left, the others scroll
 * ------------------------------------------------------------------------- */

static uint32_t rvn_table_frozen_count(const rvn_table_t* table) {
	return rvn_minu(table->frozen, table->column_count);
}

/* Width of the frozen columns, as laid out. */
static float rvn_table_frozen_width(const rvn_table_t* table) {
	float width = 0.0f;
	uint32_t i;
	for (i = 0; i < rvn_table_frozen_count(table); i++) width += table->columns[i].shown;
	return width;
}

/* Left edge of a column in the widget, the columns that scroll moved by `offset`. */
static float rvn_table_column_left(const rvn_table_t* table, const rvn_table_geometry_t* geometry, int column, float offset) {
	float x = geometry->body.x;
	int i;
	for (i = 0; i < column; i++) x += table->columns[i].shown;
	return (uint32_t)column < rvn_table_frozen_count(table) ? x : x - offset;
}

/* Where a column may show, from left to right: the frozen ones over their own part of
 * the body, the others to the right of it. */
static void rvn_table_column_span(const rvn_table_t* table, const rvn_table_geometry_t* geometry, int column, float* from, float* to) {
	float frozen = rvn_minf(rvn_table_frozen_width(table), geometry->body.w);
	if ((uint32_t)column < rvn_table_frozen_count(table)) {
		*from = geometry->body.x;
		*to = geometry->body.x + frozen;
	} else {
		*from = geometry->body.x + frozen;
		*to = geometry->body.x + geometry->body.w;
	}
}

/* The column under x in the body or the header (any y), or -1. */
static int rvn_table_column_at(const rvn_table_t* table, const rvn_table_geometry_t* geometry, float x, float offset) {
	uint32_t i;
	for (i = 0; i < table->column_count; i++) {
		float left = rvn_table_column_left(table, geometry, (int)i, offset), from, to;
		rvn_table_column_span(table, geometry, (int)i, &from, &to);
		if (x >= rvn_maxf(left, from) && x < rvn_minf(left + table->columns[i].shown, to)) return (int)i;
	}
	return -1;
}

static int rvn_table_has_editable(const rvn_table_t* table) {
	uint32_t i;
	for (i = 0; i < table->column_count; i++) if (table->columns[i].editable) return R_TRUE;
	return R_FALSE;
}

/* The column of the current cell: the one chosen, else the first editable one (-1: none). */
static int rvn_table_current_column(const rvn_table_t* table) {
	uint32_t i;
	if (table->column >= 0 && table->column < (int)table->column_count) return table->column;
	for (i = 0; i < table->column_count; i++) if (table->columns[i].editable) return (int)i;
	return -1;
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

static void rvn_table_place_editor(rvn_widget_t* widget);

static void rvn_table_arrange(rvn_widget_t* widget) {
	rvn_table_t* table = (rvn_table_t*)widget;
	rvn_table_geometry_t geometry;
	rvn_table_geometry(widget, &geometry);
	rvn_scrollbar_clamp(&table->bar);
	rvn_scrollbar_clamp(&table->hbar);
	rvn_table_place_editor(widget);
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

int rvn_table_accessible_row_at(rvn_widget_t* widget, float x, float y) {
	return rvn_table_row_at(widget, x, y);
}

rvn_rect_t rvn_table_accessible_rect(rvn_widget_t* widget, int row) {
	rvn_table_geometry_t geometry;
	rvn_table_geometry(widget, &geometry);
	return rvn_rect_intersect(geometry.body, rvn_rect(geometry.body.x,
		geometry.body.y + rvn_table_row_height(widget) * row - ((rvn_table_t*)widget)->bar.offset,
		geometry.body.w, rvn_table_row_height(widget)));
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
	for (i = 0; i < table->column_count; i++) {
		float from, to;
		left = rvn_table_column_left(table, &geometry, (int)i, table->hbar.offset);
		rvn_table_column_span(table, &geometry, (int)i, &from, &to);
		/* (the edge of a column gone under the frozen ones is not grabbed) */
		if (fabsf(x - (left + table->columns[i].shown)) <= RVN_TABLE_GRIP && left + table->columns[i].shown > from) *edge = (int)i;
	}
	found = rvn_table_column_at(table, &geometry, x, table->hbar.offset);
	return found;
}

/* Scrolls sideways so that a column that scrolls is shown whole (as much as there is room for). */
static void rvn_table_reveal_column(rvn_widget_t* widget, int column) {
	rvn_table_t* table = (rvn_table_t*)widget;
	rvn_table_geometry_t geometry;
	float left, right, frozen;
	if (column < 0 || column >= (int)table->column_count || (uint32_t)column < rvn_table_frozen_count(table)) return;
	rvn_table_geometry(widget, &geometry);
	frozen = rvn_table_frozen_width(table);
	/* (in the coordinates of the content) */
	left = rvn_table_column_left(table, &geometry, column, 0.0f) - geometry.body.x;
	right = left + table->columns[column].shown;
	if (right > table->hbar.offset + geometry.body.w) rvn_scrollbar_set(&table->hbar, right - geometry.body.w);
	if (left < table->hbar.offset + frozen) rvn_scrollbar_set(&table->hbar, left - frozen);
	rvn_widget_invalidate_paint(widget);
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
	if (!table->multi && table->selected == index) return;
	rvn_table_selection_reset(table, index);
	rvn_a11y_selection_changed(widget);
	rvn_table_reveal(widget, index);
	rvn_widget_invalidate_paint(widget);
	if (notify && table->selection_callback) table->selection_callback(widget->id, index, table->selection_user);
}

static void rvn_table_select_input(rvn_widget_t* widget, int row, uint32_t modifiers) {
	rvn_table_t* table = (rvn_table_t*)widget;
	int i;
	if (!table->multi || !(modifiers & (R_MOD_SHIFT | R_MOD_CTRL | R_MOD_SUPER))) {
		rvn_table_select(widget, row, R_TRUE);
		return;
	}
	rvn_table_selection_reserve(table);
	if ((modifiers & R_MOD_SHIFT) && table->anchor >= 0) {
		if (!(modifiers & (R_MOD_CTRL | R_MOD_SUPER))) memset(table->selected_rows, 0, table->selection_cap);
		for (i = rvn_mini(row, table->anchor); i <= rvn_maxi(row, table->anchor); i++) table->selected_rows[i] = 1;
		table->selected = row;
	} else {
		table->selected_rows[row] = !table->selected_rows[row];
		table->anchor = row;
		table->selected = table->selected_rows[row] ? row : Rtable_get_selection(widget->id, 0);
	}
	rvn_table_reveal(widget, row);
	rvn_a11y_selection_changed(widget);
	rvn_widget_invalidate_paint(widget);
	if (table->selection_callback) table->selection_callback(widget->id, table->selected, table->selection_user);
}

static void rvn_table_activate(rvn_widget_t* widget) {
	rvn_table_t* table = (rvn_table_t*)widget;
	if (table->selected < 0) return;
	if (table->activated_callback) table->activated_callback(widget->id, table->selected, table->activated_user);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

/* ---------------------------------------------------------------------------
 *  Editing a cell: a TextInput over it
 * ------------------------------------------------------------------------- */

/* The part of a cell that shows (the editor is put there), with the target scroll offsets.
 * R_FALSE when none of it shows. */
static int rvn_table_cell_rect(rvn_widget_t* widget, int row, int column, rvn_rect_t* out) {
	rvn_table_t* table = (rvn_table_t*)widget;
	rvn_table_geometry_t geometry;
	float height = rvn_table_row_height(widget), left, from, to;
	rvn_rect_t cell;
	if (row < 0 || row >= rvn_table_count(table) || column < 0 || column >= (int)table->column_count) return R_FALSE;
	rvn_table_geometry(widget, &geometry);
	left = rvn_table_column_left(table, &geometry, column, table->hbar.offset);
	rvn_table_column_span(table, &geometry, column, &from, &to);
	cell = rvn_rect(left, geometry.body.y + height * (float)row - table->bar.offset, table->columns[column].shown, height);
	*out = rvn_rect_intersect(cell, rvn_rect(from, geometry.body.y, rvn_maxf(0.0f, to - from), geometry.body.h));
	return out->w > 1.0f && out->h > 1.0f;
}

static void rvn_table_place_editor(rvn_widget_t* widget) {
	rvn_table_t* table = (rvn_table_t*)widget;
	rvn_rect_t cell;
	if (!table->editor) return;
	if (!table->editing || !rvn_table_cell_rect(widget, table->edit_row, table->edit_column, &cell)) {
		rvn_widget_arrange(table->editor, 0.0f, 0.0f, 0.0f, 0.0f);
		return;
	}
	rvn_widget_measure(table->editor);
	rvn_widget_arrange(table->editor, cell.x, cell.y, cell.w, cell.h);
}

/* Hides the editor; `refocus`: the keyboard comes back to the table. */
static void rvn_table_close_editor(rvn_widget_t* widget, int refocus) {
	rvn_table_t* table = (rvn_table_t*)widget;
	if (!table->editing) return;
	table->editing = R_FALSE;
	/* (the editor loses the focus first: it is not editing any more, nothing is kept twice) */
	if (refocus && table->editor && (table->editor->flags & RVN_WF_FOCUSED)) rvn_widget_focus(widget);
	if (!rvn_widget_alive(widget)) return;
	if (table->editor) table->editor->visibility = R_COLLAPSED;
	rvn_a11y_structure_changed();
	rvn_widget_invalidate_layout(widget);
}

/*
 * Keeps the text of the editor: the application is asked first. `refocus`: the
 * user pressed Enter or Tab (a refused text keeps the editor open); otherwise
 * the focus went elsewhere (a refused text is dropped). Returns R_TRUE when the
 * text was kept and the editor closed; the table may be gone (rvn_widget_alive).
 */
static int rvn_table_commit_edit(rvn_widget_t* widget, int refocus) {
	rvn_table_t* table = (rvn_table_t*)widget;
	int row = table->edit_row, column = table->edit_column, accepted = R_TRUE;
	char* text;
	char* before;
	if (!table->editing || table->committing) return !table->editing;
	text = rvn_strdup(table->editor ? Rtext_input_get_text(table->editor->id) : "");
	before = rvn_strdup(rvn_table_cell(table, row, column));
	if (table->edited_callback) {
		table->committing = R_TRUE;
		accepted = table->edited_callback(widget->id, row, column, text, table->edited_user) != 0;
		if (!rvn_widget_alive(widget)) {
			rvn_free(text);
			rvn_free(before);
			return R_FALSE;
		}
		table->committing = R_FALSE;
	}
	/* (the callback may have closed the editor, or cleared the rows) */
	if (!table->editing) accepted = R_FALSE;
	if (accepted && !table->is_virtual && row < (int)table->row_count && column < (int)table->column_count) {
		/* a stored cell takes the text, unless the callback already wrote what it keeps */
		if (strcmp(rvn_table_cell(table, row, column), before) == 0)
			rvn_str_set(&table->cells[(size_t)row * table->column_count + (size_t)column], text);
	}
	rvn_free(text);
	rvn_free(before);
	if (!accepted && refocus && table->editing) {
		/* refused: the user corrects it */
		if (table->editor) Rtext_input_select_all(table->editor->id);
		return R_FALSE;
	}
	rvn_table_close_editor(widget, refocus);
	rvn_widget_invalidate_paint(widget);
	return accepted;
}

/* The table an editor belongs to (NULL once it is gone). */
static rvn_widget_t* rvn_table_of(void* user) {
	rvn_widget_t* widget = rvn_widget_get((Rid)(uintptr_t)user);
	return widget && !(widget->flags & RVN_WF_DEAD) && widget->cls->type == R_WIDGET_TABLE ? widget : NULL;
}

/* Enter in the editor: keep the text, and go down a row. */
static void rvn_table_editor_committed(Rid field, const char* text, void* user) {
	rvn_widget_t* widget = rvn_table_of(user);
	rvn_table_t* table = (rvn_table_t*)widget;
	int row;
	(void)field;
	(void)text;
	if (!widget || !table->editing) return;
	row = table->edit_row;
	if (!rvn_table_commit_edit(widget, R_TRUE) || !rvn_widget_alive(widget)) return;
	if (row + 1 < rvn_table_count(table)) rvn_table_select(widget, row + 1, R_TRUE);
}

/* The focus left the editor (a click elsewhere, the window): its text is kept. */
static int rvn_table_editor_event(Rid field, const REvent* event, void* user) {
	rvn_widget_t* widget = rvn_table_of(user);
	(void)field;
	if (widget && event->type == R_EVENT_FOCUS_LOST && ((rvn_table_t*)widget)->editing) rvn_table_commit_edit(widget, R_FALSE);
	return R_FALSE;
}

/* Opens the editor on a cell: `typed` (a character the user typed) replaces its text, else the text is selected. */
static int rvn_table_begin_edit(rvn_widget_t* widget, int row, int column, const char* typed) {
	rvn_table_t* table = (rvn_table_t*)widget;
	Rid id = widget->id;
	int length;
	if (row < 0 || row >= rvn_table_count(table) || column < 0 || column >= (int)table->column_count) return R_FALSE;
	if (table->editing) {
		if (table->edit_row == row && table->edit_column == column) return R_TRUE;
		if (!rvn_table_commit_edit(widget, R_TRUE) || !rvn_widget_alive(widget)) return R_FALSE;
	}
	if (!table->editor) {
		rvn_widget_t* editor = rvn_widget_create(R_WIDGET_TEXT_INPUT);
		if (!editor) return R_FALSE;
		if (!rvn_widget_add_child(widget, editor, -1)) return R_FALSE;
		table->editor = editor;
		Rtext_input_set_committed_callback(editor->id, rvn_table_editor_committed, (void*)(uintptr_t)id);
		editor->event_callback = rvn_table_editor_event;
		editor->event_user = (void*)(uintptr_t)id;
	}
	if (table->selected != row || (table->multi && Rtable_get_selection_count(id) != 1)) {
		rvn_table_select(widget, row, R_TRUE);
		if (!rvn_widget_alive(widget) || table->editing) return R_FALSE;
	}
	table->column = column;
	rvn_table_reveal(widget, row);
	rvn_table_reveal_column(widget, column);
	table->edit_row = row;
	table->edit_column = column;
	Rtext_input_set_text(table->editor->id, typed ? typed : rvn_table_cell(table, row, column));
	/* a screen reader names the field after its column */
	Rwidget_set_accessible_name(table->editor->id, table->columns[column].header ? table->columns[column].header : "");
	table->editor->visibility = R_VISIBLE;
	table->editing = R_TRUE;
	rvn_widget_invalidate_layout(widget);
	rvn_a11y_structure_changed();
	rvn_widget_focus(table->editor);
	if (!rvn_widget_alive(widget) || !table->editing) return R_FALSE;
	/* (after the focus, which selects everything when it came with the keyboard) */
	length = (int)strlen(Rtext_input_get_text(table->editor->id));
	if (typed) rvn_text_input_set_selection(table->editor, length, length);
	else rvn_text_input_set_selection(table->editor, 0, length);
	return R_TRUE;
}

/* The next editable cell after the one edited (`step` 1), or before it (-1), row after row. */
static int rvn_table_next_editable(const rvn_table_t* table, int step, int* row, int* column) {
	int r = table->edit_row, c = table->edit_column, count = rvn_table_count(table), columns = (int)table->column_count;
	for (;;) {
		c += step;
		if (c >= columns) { c = 0; r++; }
		if (c < 0) { c = columns - 1; r--; }
		if (r < 0 || r >= count) return R_FALSE;
		if (table->columns[c].editable) break;
	}
	*row = r;
	*column = c;
	return R_TRUE;
}

/* Keys that reach the table while a cell is edited (the editor did not use them). */
static int rvn_table_editing_key(rvn_widget_t* widget, const REvent* event) {
	rvn_table_t* table = (rvn_table_t*)widget;
	int row, column, step;
	switch (event->key) {
		case R_KEY_ESCAPE:
			rvn_table_close_editor(widget, R_TRUE);
			return R_TRUE;
		case R_KEY_TAB:
			if (event->modifiers & (R_MOD_CTRL | R_MOD_ALT | R_MOD_SUPER)) return R_FALSE;
			step = (event->modifiers & R_MOD_SHIFT) ? -1 : 1;
			if (!rvn_table_next_editable(table, step, &row, &column)) {
				rvn_table_commit_edit(widget, R_TRUE);
				return R_TRUE;
			}
			if (rvn_table_commit_edit(widget, R_TRUE) && rvn_widget_alive(widget)) rvn_table_begin_edit(widget, row, column, NULL);
			return R_TRUE;
		case R_KEY_UP:
		case R_KEY_DOWN:
			/* the text is kept, and the row above or below selected */
			row = table->edit_row + (event->key == R_KEY_UP ? -1 : 1);
			if (rvn_table_commit_edit(widget, R_TRUE) && rvn_widget_alive(widget) && row >= 0 && row < rvn_table_count(table))
				rvn_table_select(widget, row, R_TRUE);
			return R_TRUE;
		default:
			/* the other keys do not move the table under the editor; the shortcuts of the window still work */
			return R_FALSE;
	}
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

/* A plain decimal number ("12", "-3.5", "1e3", "40 %"): words strtod also reads, such as "Nan" or
 * "Inf", and values that do not fit stay texts. */
static int rvn_table_parse_number(const char* text, double* number) {
	char* end = NULL;
	const char* digits;
	while (*text == ' ') text++;
	digits = (*text == '+' || *text == '-') ? text + 1 : text;
	if (!((*digits >= '0' && *digits <= '9') || *digits == '.')) return R_FALSE;
	*number = strtod(text, &end);
	if (!end || end == text || !isfinite(*number)) return R_FALSE;
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
	int selected = -1, anchor = -1;
	if (n < 2 || column < 0 || column >= (int)columns) return;
	rvn_color_t* row_colors = table->row_colors ? (rvn_color_t*)rvn_calloc(sizeof(rvn_color_t) * table->row_cap) : NULL;
	rvn_color_t* cell_colors = table->cell_colors ? (rvn_color_t*)rvn_calloc(sizeof(rvn_color_t) * (size_t)table->row_cap * columns) : NULL;
	uint8_t* selection = table->multi ? (uint8_t*)rvn_calloc(rvn_maxi((int)n, (int)table->selection_cap)) : NULL;
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
		if ((int)keys[i].row == table->anchor) anchor = (int)i;
		if (row_colors) row_colors[i] = table->row_colors[keys[i].row];
		if (cell_colors) memcpy(cell_colors + (size_t)i * columns, table->cell_colors + (size_t)keys[i].row * columns, sizeof(rvn_color_t) * columns);
		if (selection) selection[i] = (uint8_t)rvn_table_is_selected(table, (int)keys[i].row);
	}
	rvn_free(keys);
	rvn_free(table->cells);
	table->cells = sorted;
	rvn_free(table->row_colors);
	rvn_free(table->cell_colors);
	table->row_colors = row_colors;
	table->cell_colors = cell_colors;
	if (selection) {
		rvn_free(table->selected_rows);
		table->selected_rows = selection;
		table->selection_cap = (uint32_t)rvn_maxi((int)n, (int)table->selection_cap);
	}
	table->anchor = anchor;
	table->selected = selected;
	table->select_y.started = R_FALSE;
}

static void rvn_table_sort(rvn_widget_t* widget, int column, int descending, int notify) {
	rvn_table_t* table = (rvn_table_t*)widget;
	if (column < -1 || column >= (int)table->column_count) column = -1;
	/* (the rows move under the editor) */
	if (!table->is_virtual && column >= 0) rvn_table_close_editor(widget, R_TRUE);
	table->sort_column = column;
	table->sort_descending = descending ? R_TRUE : R_FALSE;
	rvn_a11y_structure_changed();
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
	int count = rvn_table_count(table), first, last, i, pass;
	float offset, hoffset;
	uint32_t c, frozen = rvn_table_frozen_count(table);
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	rvn_color_t selection = rvn_widget_color(widget, R_COLOR_SELECTION);
	rvn_color_t separator = rvn_widget_color(widget, R_COLOR_SEPARATOR);
	rvn_vec2_t origin = rvn_widget_origin(widget);
	float mouse_x = widget->window ? widget->window->mouse_x - origin.x : -1.0f;
	float mouse_y = widget->window ? widget->window->mouse_y - origin.y : -1.0f;

	rvn_table_geometry(widget, &geometry);
	offset = rvn_scrollbar_shown(&table->bar, painter);
	hoffset = rvn_scrollbar_shown(&table->hbar, painter);

	if (platform) {
		rvn_platform_paint_list_frame(painter, widget, rect, rvn_widget_states(widget, painter, R_FALSE, R_FALSE));
	} else {
		rvn_paint_box(painter, rect, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS), rvn_widget_color(widget, R_COLOR_INPUT),
			rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH), rvn_widget_color(widget, focused ? R_COLOR_FOCUS : R_COLOR_BORDER));
	}

	/* ---- the header ---- */
	if (geometry.header > 0.0f) {
		rvn_paint_push_clip(painter, rvn_rect(1.0f, 1.0f, rect.w - 2.0f, geometry.header));
		rvn_paint_rect(painter, rvn_rect(1.0f, 1.0f, rect.w - 2.0f, geometry.header), rvn_widget_color(widget, R_COLOR_HEADER));
		/* the columns that scroll, then the frozen ones over them */
		for (pass = 0; pass < 2; pass++)
		for (c = pass ? 0 : frozen; c < (pass ? frozen : table->column_count); c++) {
			rvn_table_column_t* column = &table->columns[c];
			float x = rvn_table_column_left(table, &geometry, (int)c, hoffset), from, to;
			rvn_rect_t cell = rvn_rect(x, 1.0f, column->shown, geometry.header);
			float arrow = (int)c == table->sort_column ? 14.0f : 0.0f;
			rvn_table_column_span(table, &geometry, (int)c, &from, &to);
			if (x + column->shown <= from || x >= to) continue;
			rvn_paint_push_clip(painter, rvn_rect(from, 1.0f, rvn_maxf(0.0f, to - from), geometry.header));
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
			rvn_paint_pop_clip(painter);
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
	if (table->multi || table->selected < 0) table->select_y.started = R_FALSE;
	for (i = table->multi ? first : table->selected; i >= 0 && i < (table->multi ? last : table->selected + 1); i++) {
		if (!rvn_table_is_selected(table, i)) continue;
		/* the selection slides from the previous row to the new one; a table without the focus shows it dimmer */
		float y = table->multi ? row * (float)i : rvn_paint_spring(painter, &table->select_y, row * (float)i, R_MOTION_SNAPPY);
		rvn_rect_t line = rvn_rect(geometry.body.x, geometry.body.y + y - offset, geometry.body.w, row);
		if (platform) {
			rvn_platform_paint_item(painter, widget, rvn_platform_row(widget, line),
				rvn_widget_states(widget, painter, table->hot == i && (widget->flags & RVN_WF_HOVERED), R_FALSE) | RVN_STATE_SELECTED,
				RVN_ITEM_LIST, 1.0f);
		} else {
			rvn_paint_row(painter, widget, line, focused ? selection : rvn_color_mix(selection, rvn_widget_color(widget, R_COLOR_INPUT), 0.45f));
		}
	}
	for (i = first; i < last; i++) {
		float y = geometry.body.y + row * (float)i - offset;
		rvn_color_t color = text;
		if (!rvn_table_is_selected(table, i) && i == table->hot && (widget->flags & RVN_WF_HOVERED) && painter->enabled) {
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
			rvn_color_t item = rvn_platform_item_foreground(widget, states | (rvn_table_is_selected(table, i) ? RVN_STATE_SELECTED : 0), RVN_ITEM_LIST);
			if (rvn_color_a(item)) color = item;
		}
		for (pass = 0; pass < 2; pass++)
		for (c = pass ? 0 : frozen; c < (pass ? frozen : table->column_count); c++) {
			rvn_table_column_t* column = &table->columns[c];
			float x = rvn_table_column_left(table, &geometry, (int)c, hoffset), from, to;
			rvn_rect_t cell = rvn_rect(x + padding, y, rvn_maxf(0.0f, column->shown - padding * 2.0f), row);
			rvn_table_column_span(table, &geometry, (int)c, &from, &to);
			/* (the cell being edited is under its editor) */
			if (x + column->shown > from && x < to && !(table->editing && i == table->edit_row && (int)c == table->edit_column)) {
				rvn_paint_push_clip(painter, rvn_rect_intersect(cell, rvn_rect(from, y, rvn_maxf(0.0f, to - from), row)));
				rvn_paint_text_in(painter, R_INVALID_ID, font, cell, column->align, rvn_table_cell(table, i, (int)c), -1,
					rvn_table_cell_color(table, i, (int)c, color));
				rvn_paint_pop_clip(painter);
			}
		}
		if (table->grid_lines) rvn_paint_rect(painter, rvn_rect(geometry.body.x, y + row - 1.0f, geometry.body.w, 1.0f), rvn_color_fade(separator, 0.6f));
	}
	if (table->grid_lines) {
		for (c = 0; c + 1 < table->column_count; c++) {
			float x = rvn_table_column_left(table, &geometry, (int)c, hoffset) + table->columns[c].shown, from, to;
			rvn_table_column_span(table, &geometry, (int)c, &from, &to);
			if (x <= from || x > to) continue;
			rvn_paint_rect(painter, rvn_rect(x - 1.0f, geometry.body.y, 1.0f, rvn_minf(geometry.body.h, row * (float)count - offset)),
				rvn_color_fade(separator, 0.6f));
		}
	}
	/* the current cell of a table that edits its cells: a ring of the focus color */
	if (focused && !table->editing && rvn_table_has_editable(table) && table->selected >= first && table->selected < last) {
		int column = rvn_table_current_column(table);
		if (column >= 0) {
			float x = rvn_table_column_left(table, &geometry, column, hoffset), from, to;
			rvn_table_column_span(table, &geometry, column, &from, &to);
			rvn_paint_push_clip(painter, rvn_rect(from, geometry.body.y, rvn_maxf(0.0f, to - from), geometry.body.h));
			rvn_paint_box(painter, rvn_rect(x + 1.0f, geometry.body.y + row * (float)table->selected - offset + 1.0f,
				rvn_maxf(0.0f, table->columns[column].shown - 2.0f), row - 2.0f), 2.0f, 0, 1.5f, rvn_widget_color(widget, R_COLOR_FOCUS));
			rvn_paint_pop_clip(painter);
		}
	}
	rvn_paint_pop_clip(painter);

	/* the edge of the frozen columns, and a shadow over the columns that went under them */
	if (frozen > 0 && frozen < table->column_count) {
		float edge = geometry.body.x + rvn_minf(rvn_table_frozen_width(table), geometry.body.w);
		float top = 1.0f, height = geometry.header + geometry.body.h;
		rvn_paint_rect(painter, rvn_rect(edge - 1.0f, top, 1.0f, height), separator);
		if (hoffset > 0.5f) {
			float shade = rvn_minf(1.0f, hoffset / 24.0f);
			rvn_paint_push_clip(painter, rvn_rect(edge, top, rvn_maxf(0.0f, geometry.body.x + geometry.body.w - edge), height));
			rvn_paint_gradient(painter, rvn_rect(edge, top, 8.0f, height), NULL, rvn_rgba(0.0f, 0.0f, 0.0f, 0.22f * shade),
				rvn_rgba(0.0f, 0.0f, 0.0f, 0.0f), R_TRUE);
			rvn_paint_pop_clip(painter);
		}
	}

	/* the editor follows its cell when the table scrolls */
	if (table->editing && table->editor) {
		rvn_rect_t cell;
		if (!rvn_table_cell_rect(widget, table->edit_row, table->edit_column, &cell)) cell = rvn_rect(0.0f, 0.0f, 0.0f, 0.0f);
		if (fabsf(cell.x - table->editor->rect.x) > 0.5f || fabsf(cell.y - table->editor->rect.y) > 0.5f ||
			fabsf(cell.w - table->editor->rect.w) > 0.5f || fabsf(cell.h - table->editor->rect.h) > 0.5f) rvn_widget_invalidate_layout(widget);
	}

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
				int column;
				rvn_table_geometry(widget, &geometry);
				column = rvn_table_column_at(table, &geometry, event->local_x, table->hbar.offset);
				if (column >= 0) table->column = column;
				/* a double click edits an editable cell, and activates the row elsewhere */
				if (event->clicks == 2 && column >= 0 && table->columns[column].editable && !(event->modifiers & (R_MOD_SHIFT | R_MOD_CTRL | R_MOD_SUPER))) {
					rvn_table_begin_edit(widget, row, column, NULL);
					return R_TRUE;
				}
				rvn_table_select_input(widget, row, event->modifiers);
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
			if (changed && table->editing) rvn_widget_invalidate_layout(widget);
			rvn_widget_invalidate_paint(widget);
			return changed;
		}
		case R_EVENT_FOCUS_GAINED:
		case R_EVENT_FOCUS_LOST:
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_TEXT:
			/* a character typed on an editable current cell starts editing it with that character */
			if (table->editing || event->codepoint < 32 || event->codepoint == 127 || table->selected < 0) return R_FALSE;
			{
				int column = rvn_table_current_column(table);
				char typed[5] = { 0 };
				if (column < 0 || !table->columns[column].editable) return R_FALSE;
				typed[rvn_utf8_encode(event->codepoint, typed)] = '\0';
				rvn_table_begin_edit(widget, table->selected, column, typed);
				return R_TRUE;
			}
		case R_EVENT_KEY_DOWN:
			if (table->editing) return rvn_table_editing_key(widget, event);
			rvn_table_geometry(widget, &geometry);
			if (rvn_table_has_editable(table)) {
				/* a table with editable cells has a current cell: Left and Right move it, F2 and Enter edit it */
				int column = rvn_table_current_column(table);
				switch (event->key) {
					case R_KEY_LEFT:
					case R_KEY_RIGHT:
						if (event->modifiers & (R_MOD_CTRL | R_MOD_ALT | R_MOD_SUPER)) return R_FALSE;
						column = rvn_clampi(column + (event->key == R_KEY_LEFT ? -1 : 1), 0, (int)table->column_count - 1);
						table->column = column;
						rvn_table_reveal_column(widget, column);
						if (table->selected < 0 && count > 0) rvn_table_select(widget, 0, R_TRUE);
						rvn_widget_invalidate_paint(widget);
						return R_TRUE;
					case R_KEY_F2:
					case R_KEY_ENTER:
					case R_KEY_KP_ENTER:
						if (table->selected >= 0 && column >= 0 && table->columns[column].editable) {
							rvn_table_begin_edit(widget, table->selected, column, NULL);
							return R_TRUE;
						}
						if (event->key == R_KEY_F2) return R_FALSE;
						break;
					default:
						break;
				}
			}
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
			rvn_table_select_input(widget, rvn_clampi(row, 0, count - 1), event->modifiers & R_MOD_SHIFT);
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
	table->cell_colors = rvn_table_repack_colors(table->cell_colors, old, table->column_count, table->row_count, table->row_cap);
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
	rvn_free(table->row_colors);
	rvn_free(table->cell_colors);
	table->row_colors = table->cell_colors = NULL;
	rvn_table_selection_reset(table, -1);
	rvn_a11y_structure_changed();
	table->hot = -1;
	table->bar.offset = 0.0f;
	rvn_widget_invalidate_layout(widget);
}

static void rvn_table_clear_columns(rvn_widget_t* widget) {
	rvn_table_t* table = (rvn_table_t*)widget;
	uint32_t i;
	rvn_table_close_editor(widget, R_TRUE);
	rvn_table_clear_rows(widget);
	rvn_free(table->cells);
	table->cells = NULL;
	table->row_cap = 0;
	for (i = 0; i < table->column_count; i++) rvn_free(table->columns[i].header);
	table->column_count = 0;
	table->column = -1;
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
		table->row_colors = rvn_table_repack_colors(table->row_colors, 1, 1, table->row_count, cap);
		table->cell_colors = rvn_table_repack_colors(table->cell_colors, columns, columns, table->row_count, cap);
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
	rvn_a11y_structure_changed();
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
	} else if (RVN_ATTR("MultiSelect")) {
		if (!rvn_parse_bool(value, &number)) return R_FALSE;
		Rtable_set_multi_select(widget->id, number);
	} else if (RVN_ATTR("GridLines")) {
		if (!rvn_parse_bool(value, &table->grid_lines)) return R_FALSE;
	} else if (RVN_ATTR("Striped")) {
		if (!rvn_parse_bool(value, &table->striped)) return R_FALSE;
	} else if (RVN_ATTR("FrozenColumns")) {
		if (!rvn_parse_int(value, &number) || number < 0) return R_FALSE;
		table->frozen = (uint32_t)number;
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
		value = rvn_xml_attribute(node, "Editable");
		if (value && !rvn_parse_bool(value, &column->editable)) return R_FALSE;
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
	if (table->row_colors) {
		memmove(table->row_colors + row, table->row_colors + row + 1, sizeof(rvn_color_t) * (table->row_count - (uint32_t)row - 1));
		table->row_colors[table->row_count - 1] = 0;
	}
	if (table->cell_colors) {
		memmove(table->cell_colors + (size_t)row * columns, table->cell_colors + (size_t)(row + 1) * columns,
			sizeof(rvn_color_t) * (table->row_count - (uint32_t)row - 1) * columns);
		memset(table->cell_colors + (size_t)(table->row_count - 1) * columns, 0, sizeof(rvn_color_t) * columns);
	}
	if (table->multi) {
		rvn_table_selection_reserve(table);
		memmove(table->selected_rows + row, table->selected_rows + row + 1, table->row_count - (uint32_t)row - 1);
		table->selected_rows[table->row_count - 1] = 0;
	}
	table->row_count--;
	rvn_a11y_structure_changed();
	/* the editor too */
	if (table->editing && table->edit_row == row) rvn_table_close_editor(widget, R_TRUE);
	else if (table->editing && table->edit_row > row) table->edit_row--;
	/* the selection follows the row it was on */
	if (table->selected == row) table->selected = -1;
	else if (table->selected > row) table->selected--;
	if (table->multi && table->selected < 0) table->selected = Rtable_get_selection(id, 0);
	if (table->anchor == row) table->anchor = table->selected;
	else if (table->anchor > row) table->anchor--;
	table->hot = -1;
	rvn_widget_invalidate_layout(widget);
}

void Rtable_clear_rows(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_close_editor(widget, R_TRUE);
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
	int selected = table->selected, anchor = table->anchor;
	int keep = rvn_mini(rvn_table_count(table), callback && count > 0 ? count : 0);
	uint8_t* selection = table->multi && keep ? (uint8_t*)rvn_calloc((size_t)keep) : NULL;
	int i;
	float offset = table->bar.offset;
	for (i = 0; selection && i < keep; i++) selection[i] = (uint8_t)rvn_table_is_selected(table, i);
	/* refreshing the count keeps the editor on its row while the row is still there */
	if (table->editing && (!callback || table->edit_row >= (callback && count > 0 ? count : 0))) rvn_table_close_editor(widget, R_TRUE);
	rvn_table_clear_rows(widget);
	table->is_virtual = callback != NULL;
	table->virtual_count = callback && count > 0 ? count : 0;
	table->cell_callback = callback;
	table->cell_user = user_data;
	/* refreshing the count of a live table keeps its selection and scroll position */
	if (selected >= 0 && selected < rvn_table_count(table)) table->selected = selected;
	if (anchor >= 0 && anchor < rvn_table_count(table)) table->anchor = anchor;
	if (table->multi) {
		rvn_table_selection_reserve(table);
		if (selection) memcpy(table->selected_rows, selection, (size_t)keep);
		if (table->selected < 0) table->selected = Rtable_get_selection(id, 0);
	}
	rvn_free(selection);
	rvn_a11y_structure_changed();
	table->bar.offset = offset;
	rvn_widget_invalidate_layout(widget);
}

void Rtable_set_cell_color_callback(Rid id, RTableCellColorCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	((rvn_table_t*)widget)->color_callback = callback;
	((rvn_table_t*)widget)->color_user = user_data;
	rvn_widget_invalidate_paint(widget);
}

void Rtable_set_selected(Rid id, int row) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_select(widget, row, R_FALSE);
}

void Rtable_set_cell_text_color(Rid id, int row, int column, float r, float g, float b, float a) {
	rvn_color_t** colors;
	size_t index, count;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_t* table = (rvn_table_t*)widget;
	if (table->is_virtual || row < 0 || row >= (int)table->row_count || column < -1 || column >= (int)table->column_count) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtable_set_cell_text_color: invalid stored cell");
		return;
	}
	colors = column < 0 ? &table->row_colors : &table->cell_colors;
	count = column < 0 ? table->row_cap : (size_t)table->row_cap * table->column_count;
	index = column < 0 ? (size_t)row : (size_t)row * table->column_count + (size_t)column;
	if (!*colors && a > 0) *colors = (rvn_color_t*)rvn_calloc(sizeof(rvn_color_t) * count);
	if (*colors) (*colors)[index] = a > 0 ? rvn_rgba(r, g, b, a) : 0;
	rvn_widget_invalidate_paint(widget);
}

void Rtable_set_multi_select(Rid id, int enabled) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_t* table = (rvn_table_t*)widget;
	if (table->multi == (enabled != 0)) return;
	table->multi = enabled != 0;
	if (table->multi) rvn_table_selection_reset(table, table->selected);
	else {
		rvn_free(table->selected_rows);
		table->selected_rows = NULL;
		table->selection_cap = 0;
	}
	rvn_a11y_structure_changed();
	rvn_widget_invalidate_paint(widget);
}

int Rtable_is_row_selected(Rid id, int row) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, R_FALSE);
	return rvn_table_is_selected((const rvn_table_t*)widget, row);
}

int Rtable_get_selection_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, 0);
	rvn_table_t* table = (rvn_table_t*)widget;
	int row, count = 0;
	if (!table->multi) return table->selected >= 0 ? 1 : 0;
	for (row = 0; row < rvn_mini(rvn_table_count(table), (int)table->selection_cap); row++) if (table->selected_rows[row]) count++;
	return count;
}

int Rtable_get_selection(Rid id, int index) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, -1);
	rvn_table_t* table = (rvn_table_t*)widget;
	int row;
	if (index < 0) return -1;
	if (!table->multi) return index == 0 ? table->selected : -1;
	for (row = 0; row < rvn_mini(rvn_table_count(table), (int)table->selection_cap); row++)
		if (table->selected_rows[row] && index-- == 0) return row;
	return -1;
}

void Rtable_set_row_selected(Rid id, int row, int selected) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_t* table = (rvn_table_t*)widget;
	if (row < 0 || row >= rvn_table_count(table)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtable_set_row_selected: invalid row");
		return;
	}
	if (!table->multi) {
		if (selected || row == table->selected) rvn_table_select(widget, selected ? row : -1, R_FALSE);
		return;
	}
	rvn_table_selection_reserve(table);
	table->selected_rows[row] = selected != 0;
	if (selected) table->selected = table->anchor = row;
	else if (table->selected == row) table->selected = Rtable_get_selection(id, 0);
	rvn_a11y_selection_changed(widget);
	rvn_widget_invalidate_paint(widget);
}

void Rtable_clear_selection(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_select(widget, -1, R_FALSE);
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

void Rtable_set_column_editable(Rid id, int column, int editable) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	rvn_table_t* table = (rvn_table_t*)widget;
	RVN_TABLE_COLUMN(table, column, "Rtable_set_column_editable", RVN_NOTHING)
	table->columns[column].editable = editable ? R_TRUE : R_FALSE;
	rvn_widget_invalidate_paint(widget);
}

int Rtable_is_column_editable(Rid id, int column) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, R_FALSE);
	rvn_table_t* table = (rvn_table_t*)widget;
	RVN_TABLE_COLUMN(table, column, "Rtable_is_column_editable", R_FALSE)
	return table->columns[column].editable;
}

void Rtable_set_cell_edited_callback(Rid id, RTableCellEditedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	((rvn_table_t*)widget)->edited_callback = callback;
	((rvn_table_t*)widget)->edited_user = user_data;
}

int Rtable_edit_cell(Rid id, int row, int column) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, R_FALSE);
	rvn_table_t* table = (rvn_table_t*)widget;
	RVN_TABLE_COLUMN(table, column, "Rtable_edit_cell", R_FALSE)
	if (row < 0 || row >= rvn_table_count(table)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtable_edit_cell: invalid row %d", row);
		return R_FALSE;
	}
	if (widget->window && widget->window->needs_layout) rvn_window_layout(widget->window);
	return rvn_table_begin_edit(widget, row, column, NULL);
}

int Rtable_is_editing(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, R_FALSE);
	return ((rvn_table_t*)widget)->editing;
}

int Rtable_end_edit(Rid id, int commit) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, R_TRUE);
	if (!((rvn_table_t*)widget)->editing) return R_TRUE;
	if (commit) return rvn_table_commit_edit(widget, R_TRUE);
	rvn_table_close_editor(widget, R_TRUE);
	return R_TRUE;
}

void Rtable_set_frozen_columns(Rid id, int count) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_TABLE);
	if (count < 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtable_set_frozen_columns: invalid count %d", count);
		return;
	}
	((rvn_table_t*)widget)->frozen = (uint32_t)count;
	rvn_widget_invalidate_layout(widget);
}

int Rtable_get_frozen_columns(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_TABLE, 0);
	return (int)((rvn_table_t*)widget)->frozen;
}
