/**
 * Raven - widgets/table.h
 * What Table (widgets/table.c) and TreeTable (widgets/tree_table.c) share.
 *
 * A TreeTable is a Table whose rows nest: the same structure, the same
 * columns, cells, editor, sorting and selection code. The table works on
 * the rows that are shown, top to bottom; for a TreeTable those are the
 * nodes whose parents are all expanded, kept by `tree` (rvn_tree_t, private
 * to tree_table.c). A plain Table has no `tree` and pays nothing for it.
 */

#ifndef RVN_WIDGETS_TABLE_H
#define RVN_WIDGETS_TABLE_H

#include "common.h"
#include "../xml/xml.h"

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
	/** RTableColumnKind (0: text). */
	int kind;
} rvn_table_column_t;

/* Rows first..last, both included. */
typedef struct rvn_table_range_t {
	int first, last;
} rvn_table_range_t;

typedef struct rvn_tree_t rvn_tree_t;

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
	/** RTableSelectionMode; `multi` caches mode == MULTIPLE. */
	int mode;
	int multi;
	int anchor;
	/** Multiple selection: the row the keyboard is on (Space toggles it), selected or not. */
	int focus;
	/* multiple selection: sorted ranges that neither overlap nor touch */
	rvn_table_range_t* ranges;
	uint32_t range_count;
	uint32_t range_cap;

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

	/** A TreeTable: its nodes, and which of them are the rows shown (NULL: a plain Table). */
	rvn_tree_t* tree;
} rvn_table_t;

/* What a sort compares, for each row (or each child of a node). */
typedef struct rvn_table_sort_key_t {
	const char* text;
	double number;
	int is_number;
	uint32_t row;
} rvn_table_sort_key_t;

/* ---- widgets/table.c, for the tree ---- */

/** Rows shown (a TreeTable brings its rows up to date first). */
int  rvn_table_count(rvn_table_t* table);
int  rvn_table_is_tree(const rvn_widget_t* widget);
void rvn_table_select(rvn_widget_t* widget, int index, int notify);
/** A row clicked or reached with the keyboard, with the modifiers of a multiple selection. */
void rvn_table_select_input(rvn_widget_t* widget, int row, uint32_t modifiers);
void rvn_table_reveal(rvn_widget_t* widget, int row);
void rvn_table_activate(rvn_widget_t* widget);
void rvn_table_close_editor(rvn_widget_t* widget, int refocus);
int  rvn_table_is_selected(rvn_table_t* table, int row);
int  rvn_table_current_column(const rvn_table_t* table);
int  rvn_table_has_editable(const rvn_table_t* table);
/** The rows of a virtual tree changed (Rtree_table_refresh): the selection is kept within them. */
void rvn_table_rows_refreshed(rvn_widget_t* widget);
void rvn_table_range_add(rvn_table_t* table, int first, int last);
/** `count` rows were shown at `at` (a node expanded): the selection, the anchor, the editor move down. */
void rvn_table_rows_inserted(rvn_table_t* table, int at, int count);
/**
 * The `count` rows under `row` were hidden (its node collapsed): a selection
 * inside goes to `row`, the rows below move up. R_TRUE when the selection changed.
 */
int  rvn_table_rows_collapsed(rvn_table_t* table, int row, int count);
/** The key of a sort for a text of `column` (its kind decides: a number, checked, a progress). */
void rvn_table_sort_key(const rvn_table_t* table, int column, const char* text, rvn_table_sort_key_t* key);
/** Sorts keys (stable: equal keys keep the order of their `row`). */
void rvn_table_sort_keys(rvn_table_sort_key_t* keys, uint32_t count, int descending);

/* ---- widgets/tree_table.c ---- */

/** What the tree column shows of a row. */
typedef struct rvn_tree_row_t {
	/** The id of the node (stored), or the key the application gave it (virtual). */
	int node;
	/** 0 for the top-level nodes. */
	int depth;
	/** It has children, or may have (a chevron is drawn). */
	int expandable;
	int expanded;
	/** Among the children of its parent, from 0, and how many they are. */
	int index, siblings;
	Rid icon;
} rvn_tree_row_t;

rvn_tree_t* rvn_tree_create(void);
void rvn_tree_free(rvn_table_t* table);
/** The rows shown, brought up to date. */
int  rvn_tree_count(rvn_table_t* table);
/** R_FALSE for a row that does not exist. */
int  rvn_tree_row(rvn_table_t* table, int row, rvn_tree_row_t* out);
const char* rvn_tree_cell(rvn_table_t* table, int row, int column);
/** The stored text of a cell, to be replaced (NULL: virtual, or no such cell). */
char** rvn_tree_cell_slot(rvn_table_t* table, int row, int column);
/** The color of the text of a cell (column -1: of the row), 0 for none. */
rvn_color_t rvn_tree_cell_color(rvn_table_t* table, int row, int column);
void rvn_tree_set_cell_color(rvn_table_t* table, int row, int column, rvn_color_t color);
int  rvn_tree_is_virtual(const rvn_table_t* table);
/** Sorts the children of every node (stored), keeping the hierarchy. */
void rvn_tree_sort(rvn_table_t* table, int column, int descending);
/** Columns were added: the nodes get empty cells. */
void rvn_tree_columns_changed(rvn_table_t* table);
/** Every node removed (or, virtual, every node forgotten). */
void rvn_tree_clear(rvn_table_t* table);
/** A top-level node (Rtable_add_row on a TreeTable); returns its row. */
int  rvn_tree_add_top(rvn_table_t* table, const char* cells);
/** Removes the node of a row and its subtree (Rtable_remove_row). */
void rvn_tree_remove_row(rvn_table_t* table, int row);
/** Width, from the left of the first column, taken before its text: the indentation, the chevron, the icon. */
float rvn_tree_inset(rvn_widget_t* widget, int row);
/** Draws the guides, the chevron and the icon of a row in the first column, whose left edge is `x`. */
void rvn_tree_paint_row(rvn_widget_t* widget, RPainter* painter, int row, float x, float y, float height, rvn_color_t text);
/** A press at `x` (from the left of the first column) on a row: on its chevron, it toggles the node. R_TRUE when it did. */
int  rvn_tree_press(rvn_widget_t* widget, int row, float x);
/** Opens or closes the node of a row; `notify`: the user did it. */
void rvn_tree_toggle_row(rvn_widget_t* widget, int row, int notify);
/** Keys of the hierarchy (Left, Right) before the table's. R_TRUE when used. */
int  rvn_tree_key(rvn_widget_t* widget, const REvent* event);
/** Characters typed on a cell that is not edited: + - * and the search by the first letters. R_TRUE when used. */
int  rvn_tree_text(rvn_widget_t* widget, const REvent* event);
int  rvn_tree_set_attribute(rvn_widget_t* widget, const char* name, const char* value);
/** <Row> / <Item>, with the rows nested in it. */
int  rvn_tree_xml_row(rvn_widget_t* widget, const rvn_xml_node_t* node);

#endif /* RVN_WIDGETS_TABLE_H */
