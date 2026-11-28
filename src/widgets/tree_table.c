/**
 * Raven - widgets/tree_table.c
 * TreeTable: the nodes of a Table whose rows nest (a file explorer, an
 * outline, the variables of a debugger). The widget is the Table of
 * widgets/table.c; this file keeps the tree and tells the table which nodes
 * are the rows shown, and draws and drives the first column (indentation,
 * chevron, icon, guide lines).
 *
 * Stored nodes are objects named by numbers that never change (`nodes` maps
 * them). The rows shown are a flat array of nodes, made again when the tree
 * changed (expanded, collapsed, sorted, added, moved) and asked for: a burst
 * of changes costs one walk. While the tree is changed, the selection of the
 * table, which is in rows, is held by the nodes (marks, numbers) and put
 * back in rows afterwards: it follows its nodes, a node removed leaves it,
 * and a node hidden in a closed branch gives its place to the branch.
 *
 * A virtual tree stores only the nodes that were opened (rvn_vnode_t: the
 * number the application gave, the index among its siblings, the count of
 * its children and of the rows under it), in a tree of their own. A row is
 * found by going down that tree, skipping the rows under each open node,
 * and the node at an index is asked of the application: opening a folder
 * of 100 000 files makes one record. Opening and closing move the
 * selection of the table as rows inserted and removed.
 */

#include "table.h"
#include "../render/renderer.h"

#include <limits.h>
#include <stdlib.h>

/* Seconds after which typed letters start a new search. */
#define RVN_TREE_SEARCH_DELAY 1.0

typedef struct rvn_tree_node_t {
	int id;
	struct rvn_tree_node_t* parent;
	struct rvn_tree_node_t** children;
	uint32_t child_count;
	uint32_t child_cap;
	/** `columns` texts of the tree (NULL: empty). */
	char** cells;
	/** NULL, or 1 + columns colors of text: the row, then each cell (0: none). */
	rvn_color_t* colors;
	Rid icon;
	int expanded;
	/** Shows a chevron before it has children (Rtree_table_set_node_has_children). */
	int lazy;
	/* filled when the rows are made: level, index among the siblings, row (valid while `stamp` is the tree's) */
	int depth;
	int index;
	int row;
	uint32_t stamp;
	/** Selected, while the rows are made again. */
	int mark;
} rvn_tree_node_t;

/* A node of a virtual tree that was opened (and may be closed again: it keeps the state of what is under it). */
typedef struct rvn_vnode_t {
	int key;
	struct rvn_vnode_t* parent;
	/** Among the children of the parent. */
	int index;
	/** Children, asked when it was opened. */
	int count;
	/** Rows under it while it is open: its children and the rows under those that are open. */
	int below;
	int expanded;
	/** The children that were opened, by index. */
	struct rvn_vnode_t** open;
	uint32_t open_count;
	uint32_t open_cap;
} rvn_vnode_t;

struct rvn_tree_t {
	/* ---- stored ---- */
	rvn_tree_node_t root;
	/** By number (NULL once removed); numbers are never given again. */
	rvn_tree_node_t** nodes;
	uint32_t node_cap;
	int next_id;
	int node_count;
	/** Cells of each node (at least the columns of the table). */
	uint32_t columns;
	/** The rows shown. */
	rvn_tree_node_t** rows;
	uint32_t row_count;
	uint32_t row_cap;
	uint32_t stamp;
	/** The rows must be made again; the selection is already in rows. */
	int rows_dirty;
	/** The rows must be made again; the selection is held by the nodes. */
	int detached;
	int* marked;
	uint32_t marked_count;
	uint32_t marked_cap;
	int selected_id, anchor_id, focus_id, edit_id;

	/* ---- virtual ---- */
	int is_virtual;
	rvn_vnode_t vroot;
	RTreeTableChildCountCallback count_callback;
	RTreeTableChildCallback child_callback;
	RTreeTableHasChildrenCallback has_children_callback;
	RTreeTableNodeTextCallback text_callback;
	void* user;
	RTreeTableNodeIconCallback icon_callback;
	void* icon_user;

	/* ---- options ---- */
	int guides;
	int type_search;
	char search[64];
	double search_time;
	RTreeTableExpandCallback expand_callback;
	void* expand_user;
};

rvn_tree_t* rvn_tree_create(void) {
	rvn_tree_t* tree = (rvn_tree_t*)rvn_calloc(sizeof(rvn_tree_t));
	tree->root.expanded = R_TRUE;
	tree->root.icon = R_INVALID_ID;
	tree->next_id = 1;
	tree->type_search = R_TRUE;
	tree->vroot.expanded = R_TRUE;
	return tree;
}

/* ---------------------------------------------------------------------------
 *  Stored nodes
 * ------------------------------------------------------------------------- */

static rvn_tree_node_t* rvn_tree_node(rvn_tree_t* tree, int id) {
	if (id == 0) return &tree->root;
	if (id < 0 || (uint32_t)id >= tree->node_cap) return NULL;
	return tree->nodes[id];
}

static void rvn_tree_node_release(rvn_tree_t* tree, rvn_tree_node_t* node) {
	uint32_t i;
	if (node->cells) for (i = 0; i < tree->columns; i++) rvn_free(node->cells[i]);
	rvn_free(node->cells);
	rvn_free(node->colors);
	rvn_free(node->children);
}

/* Frees the nodes under `top` and `top` itself (not the root), without recursion: a chain of any length. */
static void rvn_tree_free_subtree(rvn_tree_t* tree, rvn_tree_node_t* top) {
	rvn_tree_node_t* node = top;
	for (;;) {
		rvn_tree_node_t* parent;
		/* down to a node without children left */
		while (node->child_count > 0) node = node->children[--node->child_count];
		parent = node->parent;
		if (node == &tree->root) {
			return;
		}
		tree->nodes[node->id] = NULL;
		tree->node_count--;
		rvn_tree_node_release(tree, node);
		if (node == top) {
			rvn_free(node);
			return;
		}
		rvn_free(node);
		node = parent;
	}
}

static void rvn_vnode_free(rvn_vnode_t* node, int self) {
	uint32_t i;
	for (i = 0; i < node->open_count; i++) rvn_vnode_free(node->open[i], R_TRUE);
	rvn_free(node->open);
	node->open = NULL;
	node->open_count = node->open_cap = 0;
	if (self) rvn_free(node);
}

/* Every stored node gone, every virtual node forgotten. */
static void rvn_tree_reset(rvn_tree_t* tree) {
	rvn_tree_free_subtree(tree, &tree->root);
	rvn_free(tree->root.children);
	tree->root.children = NULL;
	tree->root.child_cap = 0;
	tree->row_count = 0;
	tree->marked_count = 0;
	tree->rows_dirty = tree->detached = R_FALSE;
	rvn_vnode_free(&tree->vroot, R_FALSE);
	tree->vroot.below = tree->vroot.count = 0;
}

void rvn_tree_free(rvn_table_t* table) {
	rvn_tree_t* tree = table->tree;
	if (!tree) return;
	rvn_tree_reset(tree);
	rvn_free(tree->nodes);
	rvn_free(tree->rows);
	rvn_free(tree->marked);
	rvn_free(tree);
	table->tree = NULL;
}

int rvn_tree_is_virtual(const rvn_table_t* table) {
	return table->tree && table->tree->is_virtual;
}

/* Makes the rows shown: the nodes whose parents are all open, from the top (no recursion). */
static void rvn_tree_flatten(rvn_tree_t* tree) {
	rvn_tree_node_t* parent = &tree->root;
	uint32_t i = 0;
	int depth = 0;
	tree->stamp++;
	tree->row_count = 0;
	for (;;) {
		if (i < parent->child_count) {
			rvn_tree_node_t* child = parent->children[i];
			child->index = (int)i;
			child->depth = depth;
			child->row = (int)tree->row_count;
			child->stamp = tree->stamp;
			RVN_ARRAY_RESERVE(tree->rows, tree->row_cap, tree->row_count + 1);
			tree->rows[tree->row_count++] = child;
			if (child->expanded && child->child_count > 0) {
				parent = child;
				i = 0;
				depth++;
				continue;
			}
			i++;
		} else {
			if (parent == &tree->root) break;
			i = (uint32_t)parent->index + 1;
			parent = parent->parent;
			depth--;
		}
	}
	tree->rows_dirty = R_FALSE;
}

static int rvn_tree_node_row(const rvn_tree_t* tree, const rvn_tree_node_t* node) {
	return node && node != &tree->root && node->stamp == tree->stamp ? node->row : -1;
}

static int rvn_tree_row_id(const rvn_tree_t* tree, int row) {
	return row >= 0 && (uint32_t)row < tree->row_count ? tree->rows[row]->id : 0;
}

static void rvn_tree_mark(rvn_tree_t* tree, rvn_tree_node_t* node) {
	if (node->mark) return;
	node->mark = R_TRUE;
	RVN_ARRAY_RESERVE(tree->marked, tree->marked_cap, tree->marked_count + 1);
	tree->marked[tree->marked_count++] = node->id;
}

/*
 * The tree is about to change: the selection of the table goes into the
 * nodes (they are marked, and the selected row, the anchor, the current
 * row and the edited row are kept as numbers).
 */
static void rvn_tree_detach(rvn_table_t* table) {
	rvn_tree_t* tree = table->tree;
	uint32_t i;
	int row;
	if (tree->detached || tree->is_virtual) return;
	if (tree->rows_dirty) rvn_tree_flatten(tree);
	tree->marked_count = 0;
	if (table->multi) {
		for (i = 0; i < table->range_count; i++) {
			for (row = table->ranges[i].first; row <= table->ranges[i].last && (uint32_t)row < tree->row_count; row++)
				rvn_tree_mark(tree, tree->rows[row]);
		}
	} else if (table->selected >= 0 && (uint32_t)table->selected < tree->row_count) {
		rvn_tree_mark(tree, tree->rows[table->selected]);
	}
	tree->selected_id = rvn_tree_row_id(tree, table->selected);
	tree->anchor_id = rvn_tree_row_id(tree, table->anchor);
	tree->focus_id = rvn_tree_row_id(tree, table->focus);
	tree->edit_id = table->editing ? rvn_tree_row_id(tree, table->edit_row) : 0;
	tree->detached = R_TRUE;
	table->hot = -1;
}

/* The row of a node kept by number: itself, or the nearest open parent when it is hidden in a closed branch; -1 when gone. */
static int rvn_tree_kept_row(rvn_tree_t* tree, int id) {
	rvn_tree_node_t* node = id > 0 ? rvn_tree_node(tree, id) : NULL;
	for (; node && node != &tree->root; node = node->parent) {
		int row = rvn_tree_node_row(tree, node);
		if (row >= 0) return row;
	}
	return -1;
}

/* The rows are made again and the selection held by the nodes goes back into rows. */
static void rvn_tree_attach(rvn_table_t* table) {
	rvn_tree_t* tree = table->tree;
	uint32_t i;
	int first = -1;
	rvn_tree_flatten(tree);
	tree->detached = R_FALSE;
	table->range_count = 0;
	for (i = 0; i < tree->marked_count; i++) {
		rvn_tree_node_t* node = rvn_tree_node(tree, tree->marked[i]);
		int row;
		if (!node || !node->mark) continue;
		node->mark = R_FALSE;
		/* a node hidden in a closed branch: the branch is selected */
		row = rvn_tree_kept_row(tree, node->id);
		if (row < 0) continue;
		if (table->multi) rvn_table_range_add(table, row, row);
		if (first < 0 || row < first) first = row;
	}
	tree->marked_count = 0;
	(void)first;
	table->selected = table->mode == R_TABLE_SELECTION_NONE ? -1 : rvn_tree_kept_row(tree, tree->selected_id);
	/* (multiple selection: the selected row is one of the selection, the first one when its own node went away) */
	if (table->multi && table->selected < 0) table->selected = table->range_count > 0 ? table->ranges[0].first : -1;
	table->anchor = rvn_tree_kept_row(tree, tree->anchor_id);
	table->focus = rvn_tree_kept_row(tree, tree->focus_id);
	if (table->anchor < 0) table->anchor = table->selected;
	if (table->focus < 0 || !table->multi) table->focus = table->selected;
	if (table->editing) {
		rvn_tree_node_t* edited = tree->edit_id > 0 ? rvn_tree_node(tree, tree->edit_id) : NULL;
		int row = rvn_tree_node_row(tree, edited);
		if (row >= 0) {
			table->edit_row = row;
		} else {
			/* (the operations that hide or remove the edited row close the editor first; this is the last resort) */
			table->editing = R_FALSE;
			if (table->editor) table->editor->visibility = R_COLLAPSED;
		}
	}
	table->select_y.started = R_FALSE;
}

/* The rows shown, up to date. */
static void rvn_tree_sync(rvn_table_t* table) {
	rvn_tree_t* tree = table->tree;
	if (tree->is_virtual) return;
	if (tree->detached) rvn_tree_attach(table);
	else if (tree->rows_dirty) rvn_tree_flatten(tree);
}

/* The tree changed: the table lays out and paints again, the screen readers are told. */
static void rvn_tree_changed(rvn_table_t* table) {
	rvn_a11y_structure_changed();
	rvn_widget_invalidate_layout(&table->base);
}

/* Splits "a|b|c" into the cells of a node. */
static void rvn_tree_set_cells(rvn_tree_t* tree, rvn_tree_node_t* node, const char* cells) {
	uint32_t c;
	for (c = 0; c < tree->columns && cells; c++) {
		size_t length = strcspn(cells, "|");
		rvn_free(node->cells[c]);
		node->cells[c] = length > 0 ? rvn_strndup(cells, length) : NULL;
		cells += length;
		if (*cells == '|') cells++;
		else cells = NULL;
	}
}

static rvn_tree_node_t* rvn_tree_insert(rvn_table_t* table, rvn_tree_node_t* parent, int index, const char* cells) {
	rvn_tree_t* tree = table->tree;
	rvn_tree_node_t* node;
	if (tree->columns < table->column_count) rvn_tree_columns_changed(table);
	rvn_tree_detach(table);
	node = (rvn_tree_node_t*)rvn_calloc(sizeof(rvn_tree_node_t));
	node->id = tree->next_id++;
	node->parent = parent;
	node->icon = R_INVALID_ID;
	node->cells = tree->columns > 0 ? (char**)rvn_calloc(sizeof(char*) * tree->columns) : NULL;
	rvn_tree_set_cells(tree, node, cells);
	if ((uint32_t)node->id >= tree->node_cap) {
		/* (the numbers not given yet are NULL) */
		uint32_t old = tree->node_cap;
		RVN_ARRAY_RESERVE(tree->nodes, tree->node_cap, (uint32_t)node->id + 1);
		memset(tree->nodes + old, 0, sizeof(rvn_tree_node_t*) * (tree->node_cap - old));
	}
	tree->nodes[node->id] = node;
	tree->node_count++;
	if (index < 0 || (uint32_t)index > parent->child_count) index = (int)parent->child_count;
	RVN_ARRAY_RESERVE(parent->children, parent->child_cap, parent->child_count + 1);
	memmove(parent->children + index + 1, parent->children + index, sizeof(rvn_tree_node_t*) * (parent->child_count - (uint32_t)index));
	parent->children[index] = node;
	parent->child_count++;
	rvn_tree_changed(table);
	return node;
}

static int rvn_tree_is_inside(const rvn_tree_node_t* node, const rvn_tree_node_t* ancestor) {
	for (; node; node = node->parent) if (node == ancestor) return R_TRUE;
	return R_FALSE;
}

static void rvn_tree_unlink(rvn_tree_node_t* node) {
	rvn_tree_node_t* parent = node->parent;
	uint32_t i;
	for (i = 0; i < parent->child_count; i++) {
		if (parent->children[i] != node) continue;
		memmove(parent->children + i, parent->children + i + 1, sizeof(rvn_tree_node_t*) * (parent->child_count - i - 1));
		parent->child_count--;
		return;
	}
}

/*
 * A change that hides or removes what is under `node` (and the node itself
 * with `self`): an editor open there is closed first, the keyboard coming
 * back to the table.
 */
static void rvn_tree_close_editor_in(rvn_table_t* table, rvn_tree_node_t* node, int self) {
	rvn_tree_t* tree = table->tree;
	rvn_tree_node_t* edited;
	if (!table->editing) return;
	rvn_tree_sync(table);
	edited = table->edit_row >= 0 && (uint32_t)table->edit_row < tree->row_count ? tree->rows[table->edit_row] : NULL;
	if (edited && rvn_tree_is_inside(edited, node) && (self || edited != node)) rvn_table_close_editor(&table->base, R_TRUE);
}

static void rvn_tree_remove(rvn_table_t* table, rvn_tree_node_t* node) {
	rvn_tree_t* tree = table->tree;
	Rid id = table->base.id;
	rvn_tree_close_editor_in(table, node, R_TRUE);
	if (!rvn_widget_get(id)) return;
	rvn_tree_detach(table);
	rvn_tree_unlink(node);
	rvn_tree_free_subtree(tree, node);
	rvn_tree_changed(table);
}

void rvn_tree_columns_changed(rvn_table_t* table) {
	rvn_tree_t* tree = table->tree;
	uint32_t columns = table->column_count, i;
	if (columns <= tree->columns) return;
	for (i = 1; i < tree->node_cap; i++) {
		rvn_tree_node_t* node = tree->nodes[i];
		char** cells;
		if (!node) continue;
		cells = (char**)rvn_calloc(sizeof(char*) * columns);
		if (node->cells) memcpy(cells, node->cells, sizeof(char*) * tree->columns);
		rvn_free(node->cells);
		node->cells = cells;
		if (node->colors) {
			rvn_color_t* colors = (rvn_color_t*)rvn_calloc(sizeof(rvn_color_t) * (columns + 1));
			memcpy(colors, node->colors, sizeof(rvn_color_t) * (tree->columns + 1));
			rvn_free(node->colors);
			node->colors = colors;
		}
	}
	tree->columns = columns;
}

void rvn_tree_clear(rvn_table_t* table) {
	rvn_tree_t* tree = table->tree;
	rvn_tree_reset(tree);
	tree->is_virtual = R_FALSE;
	tree->count_callback = NULL;
	tree->child_callback = NULL;
	tree->has_children_callback = NULL;
	tree->text_callback = NULL;
	tree->icon_callback = NULL;
}

int rvn_tree_add_top(rvn_table_t* table, const char* cells) {
	rvn_tree_node_t* node = rvn_tree_insert(table, &table->tree->root, -1, cells);
	rvn_tree_sync(table);
	return rvn_tree_node_row(table->tree, node);
}

void rvn_tree_remove_row(rvn_table_t* table, int row) {
	rvn_tree_sync(table);
	if (row < 0 || (uint32_t)row >= table->tree->row_count) return;
	rvn_tree_remove(table, table->tree->rows[row]);
}

/* ---------------------------------------------------------------------------
 *  Virtual nodes
 * ------------------------------------------------------------------------- */

static int rvn_vtree_count(rvn_table_t* table, int key) {
	rvn_tree_t* tree = table->tree;
	int count = tree->count_callback ? tree->count_callback(table->base.id, key, tree->user) : 0;
	return count > 0 ? count : 0;
}

static int rvn_vtree_child(rvn_table_t* table, int key, int index) {
	rvn_tree_t* tree = table->tree;
	return tree->child_callback ? tree->child_callback(table->base.id, key, index, tree->user) : 0;
}

static int rvn_vtree_has_children(rvn_table_t* table, int key) {
	rvn_tree_t* tree = table->tree;
	if (tree->has_children_callback) return tree->has_children_callback(table->base.id, key, tree->user) != 0;
	return rvn_vtree_count(table, key) > 0;
}

/* The open child of `node` at `index`, or NULL. */
static rvn_vnode_t* rvn_vnode_at(const rvn_vnode_t* node, int index) {
	uint32_t low = 0, high = node->open_count;
	while (low < high) {
		uint32_t middle = low + (high - low) / 2;
		if (node->open[middle]->index < index) low = middle + 1;
		else high = middle;
	}
	return low < node->open_count && node->open[low]->index == index ? node->open[low] : NULL;
}

/* A row of a virtual tree: the open node it is under, its index there, its level, and its record when it was opened. */
typedef struct rvn_vrow_t {
	rvn_vnode_t* parent;
	int index;
	int depth;
	rvn_vnode_t* node;
} rvn_vrow_t;

static int rvn_vtree_resolve(rvn_tree_t* tree, int row, rvn_vrow_t* out) {
	rvn_vnode_t* node = &tree->vroot;
	int depth = 0;
	if (row < 0 || row >= tree->vroot.below) return R_FALSE;
	for (;;) {
		rvn_vnode_t* into = NULL;
		int skipped = 0;
		uint32_t i;
		/* the rows under the open children before the row are skipped */
		for (i = 0; i < node->open_count; i++) {
			rvn_vnode_t* child = node->open[i];
			int at;
			if (child->index >= node->count) break;
			at = child->index + skipped;
			if (row < at) break;
			if (row == at) {
				out->parent = node;
				out->index = child->index;
				out->depth = depth;
				out->node = child;
				return R_TRUE;
			}
			if (child->expanded) {
				if (row <= at + child->below) {
					into = child;
					row -= at + 1;
					break;
				}
				skipped += child->below;
			}
		}
		if (!into) {
			out->parent = node;
			out->index = row - skipped;
			out->depth = depth;
			out->node = rvn_vnode_at(node, out->index);
			return out->index < node->count;
		}
		node = into;
		depth++;
	}
}

static int rvn_vtree_key(rvn_table_t* table, const rvn_vrow_t* where) {
	return where->node ? where->node->key : rvn_vtree_child(table, where->parent->key, where->index);
}

/* The row of an open node, -1 when one of its parents is closed. */
static int rvn_vnode_row(const rvn_tree_t* tree, const rvn_vnode_t* node) {
	int row;
	uint32_t i;
	const rvn_vnode_t* parent = node->parent;
	if (!parent) return -1;
	if (parent == &tree->vroot) {
		row = 0;
	} else {
		if (!parent->expanded) return -1;
		row = rvn_vnode_row(tree, parent);
		if (row < 0) return -1;
		row++;
	}
	row += node->index;
	for (i = 0; i < parent->open_count && parent->open[i]->index < node->index; i++)
		if (parent->open[i]->expanded) row += parent->open[i]->below;
	return row;
}

/* The record of an opened node by its number, or NULL. */
static rvn_vnode_t* rvn_vnode_find(rvn_vnode_t* node, int key) {
	uint32_t i;
	if (node->key == key && node->parent) return node;
	for (i = 0; i < node->open_count; i++) {
		rvn_vnode_t* found = rvn_vnode_find(node->open[i], key);
		if (found) return found;
	}
	return NULL;
}

/* The rows under an open node, from its count and its open children. */
static int rvn_vnode_below(const rvn_vnode_t* node) {
	int below = node->count;
	uint32_t i;
	for (i = 0; i < node->open_count; i++)
		if (node->open[i]->expanded && node->open[i]->index < node->count) below += node->open[i]->below;
	return below;
}

static int rvn_vnode_compare(const void* a, const void* b) {
	int x = (*(const rvn_vnode_t* const*)a)->index, y = (*(const rvn_vnode_t* const*)b)->index;
	return x < y ? -1 : (x > y ? 1 : 0);
}

/*
 * Asks the count of children of an open node again, finds its open children
 * again by their numbers (a sort moved them), forgets those that are gone,
 * and does the same under those that are open. Its `below` is made again.
 */
static void rvn_vtree_refresh_node(rvn_table_t* table, rvn_vnode_t* node) {
	uint32_t i, kept = 0;
	node->count = rvn_vtree_count(table, node->key);
	for (i = 0; i < node->open_count; i++) {
		rvn_vnode_t* child = node->open[i];
		if (child->index >= node->count || rvn_vtree_child(table, node->key, child->index) != child->key) {
			int index;
			child->index = -1;
			for (index = 0; index < node->count; index++) {
				if (rvn_vtree_child(table, node->key, index) == child->key) {
					child->index = index;
					break;
				}
			}
		}
		if (child->index < 0) {
			rvn_vnode_free(child, R_TRUE);
			continue;
		}
		node->open[kept++] = child;
	}
	node->open_count = kept;
	if (kept > 1) qsort(node->open, kept, sizeof(rvn_vnode_t*), rvn_vnode_compare);
	for (i = 0; i < node->open_count; i++) if (node->open[i]->expanded) rvn_vtree_refresh_node(table, node->open[i]);
	node->below = rvn_vnode_below(node);
}

/* `below` of the open parents of `node` changed by `delta`. */
static void rvn_vnode_grow(rvn_tree_t* tree, rvn_vnode_t* node, int delta) {
	rvn_vnode_t* parent;
	(void)tree;
	for (parent = node->parent; parent; parent = parent->parent) {
		parent->below += delta;
		if (!parent->expanded) break;
	}
}

/* Opens or closes the virtual node at `row`; R_TRUE when the selection changed. */
static int rvn_vtree_set_expanded(rvn_table_t* table, int row, int expanded) {
	rvn_tree_t* tree = table->tree;
	rvn_vrow_t where;
	Rid id = table->base.id;
	rvn_vnode_t* node;
	int key, changed = R_FALSE;
	if (!rvn_vtree_resolve(tree, row, &where)) return R_FALSE;
	if ((where.node && where.node->expanded) == (expanded != 0)) return R_FALSE;
	key = rvn_vtree_key(table, &where);
	if (expanded) {
		/* the application loads the children now if it wants */
		if (tree->expand_callback) {
			tree->expand_callback(id, key, R_TRUE, tree->expand_user);
			if (!rvn_widget_get(id) || table->tree != tree || !tree->is_virtual) return R_FALSE;
			/* (it may have changed the tree: the row is found again) */
			if (!rvn_vtree_resolve(tree, row, &where) || rvn_vtree_key(table, &where) != key || (where.node && where.node->expanded)) return R_FALSE;
		}
		node = where.node;
		if (!node) {
			uint32_t at = 0;
			node = (rvn_vnode_t*)rvn_calloc(sizeof(rvn_vnode_t));
			node->key = key;
			node->parent = where.parent;
			node->index = where.index;
			while (at < where.parent->open_count && where.parent->open[at]->index < where.index) at++;
			RVN_ARRAY_RESERVE(where.parent->open, where.parent->open_cap, where.parent->open_count + 1);
			memmove(where.parent->open + at + 1, where.parent->open + at, sizeof(rvn_vnode_t*) * (where.parent->open_count - at));
			where.parent->open[at] = node;
			where.parent->open_count++;
		}
		node->expanded = R_TRUE;
		rvn_vtree_refresh_node(table, node);
		rvn_vnode_grow(tree, node, node->below);
		rvn_table_rows_inserted(table, row + 1, node->below);
	} else {
		node = where.node;
		node->expanded = R_FALSE;
		rvn_vnode_grow(tree, node, -node->below);
		/* (an editor in the rows that close is closed first) */
		if (table->editing && table->edit_row > row && table->edit_row <= row + node->below) rvn_table_close_editor(&table->base, R_TRUE);
		if (!rvn_widget_get(id)) return R_FALSE;
		changed = rvn_table_rows_collapsed(table, row, node->below);
		if (tree->expand_callback) tree->expand_callback(id, key, R_FALSE, tree->expand_user);
	}
	if (rvn_widget_get(id)) rvn_tree_changed(table);
	return changed;
}

/* The row of a virtual node: an open one directly, another one by looking through the rows shown. */
static int rvn_vtree_find_row(rvn_table_t* table, int key) {
	rvn_tree_t* tree = table->tree;
	rvn_vnode_t* node = key > 0 ? rvn_vnode_find(&tree->vroot, key) : NULL;
	int row, count = tree->vroot.below;
	if (key <= 0) return -1;
	if (node) return rvn_vnode_row(tree, node);
	for (row = 0; row < count; row++) {
		rvn_vrow_t where;
		if (rvn_vtree_resolve(tree, row, &where) && rvn_vtree_key(table, &where) == key) return row;
	}
	return -1;
}

/* ---------------------------------------------------------------------------
 *  Rows, for the table
 * ------------------------------------------------------------------------- */

int rvn_tree_count(rvn_table_t* table) {
	rvn_tree_t* tree = table->tree;
	if (tree->is_virtual) return tree->vroot.below;
	rvn_tree_sync(table);
	return (int)tree->row_count;
}

int rvn_tree_row(rvn_table_t* table, int row, rvn_tree_row_t* out) {
	rvn_tree_t* tree = table->tree;
	memset(out, 0, sizeof(*out));
	out->icon = R_INVALID_ID;
	if (tree->is_virtual) {
		rvn_vrow_t where;
		if (!rvn_vtree_resolve(tree, row, &where)) return R_FALSE;
		out->node = rvn_vtree_key(table, &where);
		out->depth = where.depth;
		out->expanded = where.node && where.node->expanded;
		out->expandable = out->expanded || rvn_vtree_has_children(table, out->node);
		out->index = where.index;
		out->siblings = where.parent->count;
		if (tree->icon_callback) out->icon = tree->icon_callback(table->base.id, out->node, tree->icon_user);
		return R_TRUE;
	}
	rvn_tree_sync(table);
	if (row < 0 || (uint32_t)row >= tree->row_count) return R_FALSE;
	{
		rvn_tree_node_t* node = tree->rows[row];
		out->node = node->id;
		out->depth = node->depth;
		out->expandable = node->child_count > 0 || node->lazy;
		out->expanded = node->expanded && out->expandable;
		out->index = node->index;
		out->siblings = (int)node->parent->child_count;
		out->icon = node->icon;
	}
	return R_TRUE;
}

static rvn_tree_node_t* rvn_tree_row_node(rvn_table_t* table, int row) {
	rvn_tree_t* tree = table->tree;
	if (tree->is_virtual) return NULL;
	rvn_tree_sync(table);
	return row >= 0 && (uint32_t)row < tree->row_count ? tree->rows[row] : NULL;
}

const char* rvn_tree_cell(rvn_table_t* table, int row, int column) {
	rvn_tree_t* tree = table->tree;
	if (tree->is_virtual) {
		rvn_vrow_t where;
		const char* text;
		if (!tree->text_callback || !rvn_vtree_resolve(tree, row, &where)) return "";
		text = tree->text_callback(table->base.id, rvn_vtree_key(table, &where), column, tree->user);
		return text ? text : "";
	} else {
		rvn_tree_node_t* node = rvn_tree_row_node(table, row);
		if (!node || column < 0 || (uint32_t)column >= tree->columns || !node->cells[column]) return "";
		return node->cells[column];
	}
}

char** rvn_tree_cell_slot(rvn_table_t* table, int row, int column) {
	rvn_tree_node_t* node = rvn_tree_row_node(table, row);
	if (!node || column < 0 || column >= (int)table->column_count) return NULL;
	if (table->tree->columns < table->column_count) rvn_tree_columns_changed(table);
	return &node->cells[column];
}

rvn_color_t rvn_tree_cell_color(rvn_table_t* table, int row, int column) {
	rvn_tree_node_t* node = rvn_tree_row_node(table, row);
	if (!node || !node->colors || column < -1 || column >= (int)table->tree->columns) return 0;
	return node->colors[column + 1];
}

void rvn_tree_set_cell_color(rvn_table_t* table, int row, int column, rvn_color_t color) {
	rvn_tree_node_t* node = rvn_tree_row_node(table, row);
	if (!node || column < -1 || column >= (int)table->tree->columns) return;
	if (!node->colors && !rvn_color_a(color)) return;
	if (!node->colors) node->colors = (rvn_color_t*)rvn_calloc(sizeof(rvn_color_t) * (table->tree->columns + 1));
	node->colors[column + 1] = color;
}

/* ---------------------------------------------------------------------------
 *  Sorting: the children of each node among themselves
 * ------------------------------------------------------------------------- */

static void rvn_tree_sort_children(rvn_table_t* table, rvn_tree_node_t* node, int column, int descending,
	rvn_table_sort_key_t** keys, uint32_t* key_cap, rvn_tree_node_t*** scratch, uint32_t* scratch_cap) {
	uint32_t i, n = node->child_count;
	if (n < 2) return;
	RVN_ARRAY_RESERVE(*keys, *key_cap, n);
	RVN_ARRAY_RESERVE(*scratch, *scratch_cap, n);
	for (i = 0; i < n; i++) {
		rvn_table_sort_key(table, column, node->children[i]->cells ? node->children[i]->cells[column] : NULL, &(*keys)[i]);
		(*keys)[i].row = i;
	}
	rvn_table_sort_keys(*keys, n, descending);
	for (i = 0; i < n; i++) (*scratch)[i] = node->children[(*keys)[i].row];
	memcpy(node->children, *scratch, sizeof(rvn_tree_node_t*) * n);
}

void rvn_tree_sort(rvn_table_t* table, int column, int descending) {
	rvn_tree_t* tree = table->tree;
	rvn_table_sort_key_t* keys = NULL;
	rvn_tree_node_t** scratch = NULL;
	uint32_t key_cap = 0, scratch_cap = 0, i;
	if (tree->is_virtual || column < 0 || (uint32_t)column >= tree->columns) return;
	rvn_tree_detach(table);
	/* (every node with children, by number: no recursion) */
	rvn_tree_sort_children(table, &tree->root, column, descending, &keys, &key_cap, &scratch, &scratch_cap);
	for (i = 1; i < tree->node_cap; i++)
		if (tree->nodes[i]) rvn_tree_sort_children(table, tree->nodes[i], column, descending, &keys, &key_cap, &scratch, &scratch_cap);
	rvn_free(keys);
	rvn_free(scratch);
	rvn_tree_sync(table);
}

/* ---------------------------------------------------------------------------
 *  Opening and closing
 * ------------------------------------------------------------------------- */

/* The selection as it is now, to tell whether a change moved it. */
static uint64_t rvn_tree_selection_print(rvn_table_t* table) {
	uint64_t print = (uint64_t)(uint32_t)Rtree_table_get_selected_node(table->base.id);
	uint32_t i;
	for (i = 0; table->multi && i < table->range_count; i++)
		print = print * 1000003u + (uint64_t)(uint32_t)table->ranges[i].first * 31u + (uint64_t)(uint32_t)table->ranges[i].last;
	return print;
}

static void rvn_tree_notify_selection(rvn_table_t* table) {
	if (table->selection_callback) table->selection_callback(table->base.id, table->selected, table->selection_user);
}

/* Opens or closes a stored node; R_FALSE when the table went away. */
static int rvn_tree_node_set_expanded(rvn_table_t* table, rvn_tree_node_t* node, int expanded) {
	rvn_tree_t* tree = table->tree;
	Rid id = table->base.id;
	int number = node->id;
	expanded = expanded != 0;
	if (node == &tree->root || node->expanded == expanded) return R_TRUE;
	if (!expanded) {
		/* (the editor of the node itself stays) */
		rvn_tree_close_editor_in(table, node, R_FALSE);
		if (!rvn_widget_get(id)) return R_FALSE;
	}
	rvn_tree_detach(table);
	node->expanded = expanded;
	rvn_tree_changed(table);
	if (tree->expand_callback) {
		tree->expand_callback(id, number, expanded, tree->expand_user);
		if (!rvn_widget_get(id)) return R_FALSE;
	}
	return R_TRUE;
}

/* Opens or closes the node of a row, stored or virtual. `notify`: the user did it (a selection moved out of a closed branch is told). */
static void rvn_tree_set_row_expanded(rvn_widget_t* widget, int row, int expanded, int notify) {
	rvn_table_t* table = (rvn_table_t*)widget;
	Rid id = widget->id;
	uint64_t before = rvn_tree_selection_print(table);
	if (table->tree->is_virtual) {
		int changed = rvn_vtree_set_expanded(table, row, expanded);
		if (notify && changed && rvn_widget_get(id)) rvn_tree_notify_selection(table);
		return;
	} else {
		rvn_tree_node_t* node = rvn_tree_row_node(table, row);
		if (!node || !rvn_tree_node_set_expanded(table, node, expanded)) return;
	}
	if (notify && rvn_widget_get(id) && rvn_tree_selection_print(table) != before) rvn_tree_notify_selection(table);
}

void rvn_tree_toggle_row(rvn_widget_t* widget, int row, int notify) {
	rvn_tree_row_t info;
	if (!rvn_tree_row((rvn_table_t*)widget, row, &info) || !info.expandable) return;
	rvn_tree_set_row_expanded(widget, row, !info.expanded, notify);
}

/* Opens everything under the node of a row (the row itself too). */
static void rvn_tree_expand_under_row(rvn_widget_t* widget, int row) {
	rvn_table_t* table = (rvn_table_t*)widget;
	Rid id = widget->id;
	rvn_tree_row_t info;
	int depth, r;
	if (!rvn_tree_row(table, row, &info)) return;
	depth = info.depth;
	/* the rows under it grow as their nodes open: they are walked as they come */
	for (r = row; rvn_widget_get(id) && r < rvn_table_count(table); r++) {
		if (!rvn_tree_row(table, r, &info) || (r > row && info.depth <= depth)) break;
		if (info.expandable && !info.expanded) rvn_tree_set_row_expanded(widget, r, R_TRUE, R_FALSE);
	}
}

/* Stored: opens or closes a node and everything under it (no recursion). */
static void rvn_tree_set_subtree_expanded(rvn_table_t* table, rvn_tree_node_t* top, int expanded) {
	rvn_tree_t* tree = table->tree;
	Rid id = table->base.id;
	int* stack = NULL;
	uint32_t count = 0, cap = 0, i;
	RVN_ARRAY_RESERVE(stack, cap, 1);
	stack[count++] = top->id;
	while (count > 0) {
		int number = stack[--count];
		rvn_tree_node_t* node = rvn_tree_node(tree, number);
		if (!node) continue;
		if (node != &tree->root && (node->child_count > 0 || node->lazy || !expanded)) {
			if (!rvn_tree_node_set_expanded(table, node, expanded) || !rvn_widget_get(id) || table->tree != tree) break;
			/* (the callback may have removed it, or added its children) */
			node = rvn_tree_node(tree, number);
			if (!node) continue;
		}
		RVN_ARRAY_RESERVE(stack, cap, count + node->child_count);
		for (i = node->child_count; i > 0; i--) stack[count++] = node->children[i - 1]->id;
	}
	rvn_free(stack);
}

/* ---------------------------------------------------------------------------
 *  The first column: indentation, chevron, icon, guides
 * ------------------------------------------------------------------------- */

static float rvn_tree_indent(const rvn_widget_t* widget) {
	return rvn_maxf(12.0f, rvn_widget_metric(widget, R_METRIC_INDENT));
}

static float rvn_tree_icon_size(const rvn_widget_t* widget) {
	return rvn_clampf(rvn_widget_metric(widget, R_METRIC_ROW_HEIGHT) - 10.0f, 4.0f, 18.0f);
}

/* Room before the chevron of the top level. */
#define RVN_TREE_MARGIN 4.0f

float rvn_tree_inset(rvn_widget_t* widget, int row) {
	rvn_tree_row_t info;
	float indent = rvn_tree_indent(widget), inset;
	if (!rvn_tree_row((rvn_table_t*)widget, row, &info)) return 0.0f;
	inset = RVN_TREE_MARGIN + indent * (float)(info.depth + 1);
	if (info.icon != R_INVALID_ID) inset += rvn_tree_icon_size(widget) + 6.0f;
	return inset;
}

void rvn_tree_paint_row(rvn_widget_t* widget, RPainter* painter, int row, float x, float y, float height, rvn_color_t text) {
	rvn_table_t* table = (rvn_table_t*)widget;
	rvn_tree_row_t info;
	float indent = rvn_tree_indent(widget), left = x + RVN_TREE_MARGIN;
	int level;
	if (!rvn_tree_row(table, row, &info)) return;
	if (table->tree->guides) {
		/* a line down each level above the row, through the middle of the chevrons */
		rvn_color_t guide = rvn_color_fade(rvn_widget_color(widget, R_COLOR_SEPARATOR), 0.8f);
		for (level = 0; level < info.depth; level++)
			rvn_paint_rect(painter, rvn_rect(floorf(left + indent * ((float)level + 0.5f)), y, 1.0f, height), guide);
	}
	if (info.expandable) {
		float cx = left + indent * ((float)info.depth + 0.5f), cy = y + height * 0.5f;
		if (rvn_widget_family(widget) != RVN_FAMILY_RAVEN) {
			rvn_platform_paint_chevron(painter, widget, cx, cy, 10.0f, info.expanded ? 1.5707964f : 0.0f, rvn_platform_secondary(widget));
		} else {
			rvn_paint_icon(painter, info.expanded ? RVN_ICON_CHEVRON_DOWN : RVN_ICON_CHEVRON_RIGHT, cx, cy, 7.0f, rvn_color_fade(text, 0.75f));
		}
	}
	if (info.icon != R_INVALID_ID) {
		static const float uv[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
		float size = rvn_tree_icon_size(widget);
		/* (an icon in currentColor takes the color of the text) */
		rvn_paint_image(painter, rvn_rect(left + indent * (float)(info.depth + 1) + 1.0f, floorf(y + (height - size) * 0.5f), size, size),
			info.icon, uv, rvn_icon_tint(info.icon, R_FALSE, text, rvn_rgba(1.0f, 1.0f, 1.0f, 1.0f)), 0.0f);
	}
}

int rvn_tree_press(rvn_widget_t* widget, int row, float x) {
	rvn_tree_row_t info;
	float indent = rvn_tree_indent(widget), from;
	if (!rvn_tree_row((rvn_table_t*)widget, row, &info) || !info.expandable) return R_FALSE;
	from = RVN_TREE_MARGIN + indent * (float)info.depth;
	if (x < from - 2.0f || x >= from + indent) return R_FALSE;
	rvn_tree_set_row_expanded(widget, row, !info.expanded, R_TRUE);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Keyboard
 * ------------------------------------------------------------------------- */

static int rvn_tree_cursor(const rvn_table_t* table) {
	return table->multi ? table->focus : table->selected;
}

/* The row of the parent of the node of a row, -1 at the top level. */
static int rvn_tree_parent_row(rvn_table_t* table, int row) {
	rvn_tree_row_t info;
	int depth;
	if (!rvn_tree_row(table, row, &info) || info.depth == 0) return -1;
	if (!table->tree->is_virtual) return rvn_tree_node_row(table->tree, rvn_tree_row_node(table, row)->parent);
	depth = info.depth;
	/* the rows of its earlier siblings (and their open subtrees) are above it: the first row of a smaller level is the parent */
	for (row--; row >= 0; row--) if (rvn_tree_row(table, row, &info) && info.depth < depth) return row;
	return -1;
}

int rvn_tree_key(rvn_widget_t* widget, const REvent* event) {
	rvn_table_t* table = (rvn_table_t*)widget;
	rvn_tree_row_t info;
	int row = rvn_tree_cursor(table), count = rvn_table_count(table);
	if (event->key != R_KEY_LEFT && event->key != R_KEY_RIGHT) return R_FALSE;
	if (event->modifiers & (R_MOD_CTRL | R_MOD_ALT | R_MOD_SUPER | R_MOD_SHIFT)) return R_FALSE;
	/* with cells to edit, the keys act on the tree in its own column only */
	if (rvn_table_has_editable(table) && rvn_table_current_column(table) > 0) return R_FALSE;
	if (count == 0 || table->mode == R_TABLE_SELECTION_NONE) return R_FALSE;
	if (row < 0) {
		rvn_table_select_input(widget, 0, 0);
		return R_TRUE;
	}
	if (!rvn_tree_row(table, row, &info)) return R_FALSE;
	if (event->key == R_KEY_LEFT) {
		/* close it, or go up to the parent */
		if (info.expandable && info.expanded) {
			rvn_tree_set_row_expanded(widget, row, R_FALSE, R_TRUE);
		} else {
			int parent = rvn_tree_parent_row(table, row);
			if (parent >= 0) rvn_table_select_input(widget, parent, 0);
		}
		return R_TRUE;
	}
	/* open it, or go down to its first child */
	if (info.expandable && !info.expanded) {
		rvn_tree_set_row_expanded(widget, row, R_TRUE, R_TRUE);
		return R_TRUE;
	}
	if (info.expanded && row + 1 < count) {
		rvn_tree_row_t next;
		if (rvn_tree_row(table, row + 1, &next) && next.depth > info.depth) {
			rvn_table_select_input(widget, row + 1, 0);
			return R_TRUE;
		}
	}
	/* a leaf: with cells to edit, Right goes on to the next cell */
	return !rvn_table_has_editable(table);
}

/* The next row from `start` (wrapping) whose first cell starts with the letters typed, or -1. */
static int rvn_tree_search(rvn_table_t* table, int start) {
	size_t length = strlen(table->tree->search);
	int count = rvn_table_count(table), i;
	for (i = 0; i < count; i++) {
		int row = (start + i) % count;
		const char* text = rvn_tree_cell(table, row, 0);
		while (*text == ' ') text++;
		if (rvn_strnicmp(text, table->tree->search, length) == 0) return row;
	}
	return -1;
}

int rvn_tree_text(rvn_widget_t* widget, const REvent* event) {
	rvn_table_t* table = (rvn_table_t*)widget;
	rvn_tree_t* tree = table->tree;
	int row = rvn_tree_cursor(table), column = rvn_table_current_column(table);
	rvn_tree_row_t info;
	uint32_t codepoint = event->codepoint;
	/* a character on a cell edited by typing goes to its editor */
	if (rvn_table_has_editable(table) && column >= 0 && table->columns[column].editable && table->columns[column].kind != R_TABLE_COLUMN_CHECK)
		return R_FALSE;
	if (codepoint == ' ' || rvn_table_count(table) == 0) return R_FALSE;
	if ((codepoint == '+' || codepoint == '-' || codepoint == '*') && !tree->search[0]) {
		if (row < 0 || !rvn_tree_row(table, row, &info)) return R_TRUE;
		if (codepoint == '*') rvn_tree_expand_under_row(widget, row);
		else if (info.expandable && info.expanded != (codepoint == '+')) rvn_tree_set_row_expanded(widget, row, codepoint == '+', R_TRUE);
		return R_TRUE;
	}
	if (!tree->type_search || table->mode == R_TABLE_SELECTION_NONE) return R_FALSE;
	{
		double now = Rget_time();
		size_t length;
		int found;
		if (now - tree->search_time > RVN_TREE_SEARCH_DELAY || now < tree->search_time) tree->search[0] = '\0';
		tree->search_time = now;
		length = strlen(tree->search);
		if (length + 5 >= sizeof(tree->search)) return R_TRUE;
		tree->search[length + (size_t)rvn_utf8_encode(codepoint, tree->search + length)] = '\0';
		/* a first letter goes on to the next match; more letters refine the one there */
		found = rvn_tree_search(table, row < 0 ? 0 : (length == 0 ? row + 1 : row));
		if (found >= 0) rvn_table_select_input(widget, found, 0);
	}
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  XML and the screen readers
 * ------------------------------------------------------------------------- */

int rvn_tree_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_tree_t* tree = ((rvn_table_t*)widget)->tree;
	if (RVN_ATTR("GuideLines")) {
		if (!rvn_parse_bool(value, &tree->guides)) return R_FALSE;
	} else if (RVN_ATTR("TypeSearch")) {
		if (!rvn_parse_bool(value, &tree->type_search)) return R_FALSE;
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_paint(widget);
	return R_TRUE;
}

/* <Row Cells="a|b" Icon="raven:folder" Expanded="true" HasChildren="true"> <Cell>..</Cell> <Row>..</Row> </Row> */
static int rvn_tree_xml_node(rvn_table_t* table, rvn_tree_node_t* parent, const rvn_xml_node_t* xml) {
	const rvn_xml_node_t* child;
	const char* value = rvn_xml_attribute(xml, "Cells");
	rvn_tree_node_t* node;
	uint32_t c = 0;
	int flag = R_FALSE, has_cells = R_FALSE;
	for (child = xml->first_child; child; child = child->next) if (rvn_stricmp(child->name, "Cell") == 0) has_cells = R_TRUE;
	if (!value) value = rvn_xml_attribute(xml, "Text");
	if (!value && !has_cells && !xml->first_child) value = xml->text;
	node = rvn_tree_insert(table, parent, -1, value);
	value = rvn_xml_attribute(xml, "Icon");
	if (value && value[0]) node->icon = rvn_xml_texture(value);
	value = rvn_xml_attribute(xml, "Expanded");
	if (value) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		node->expanded = flag;
	}
	value = rvn_xml_attribute(xml, "HasChildren");
	if (value) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		node->lazy = flag;
	}
	for (child = xml->first_child; child; child = child->next) {
		if (rvn_stricmp(child->name, "Cell") == 0) {
			const char* text = rvn_xml_attribute(child, "Text");
			if (c < table->tree->columns) rvn_str_set(&node->cells[c], text ? text : child->text);
			c++;
		} else if (rvn_stricmp(child->name, "Row") == 0 || rvn_stricmp(child->name, "Item") == 0) {
			if (!rvn_tree_xml_node(table, node, child)) return R_FALSE;
		} else {
			return R_FALSE;
		}
	}
	return R_TRUE;
}

int rvn_tree_xml_row(rvn_widget_t* widget, const rvn_xml_node_t* node) {
	rvn_table_t* table = (rvn_table_t*)widget;
	if (table->tree->is_virtual || table->column_count == 0) return R_FALSE;
	return rvn_tree_xml_node(table, &table->tree->root, node);
}

int rvn_tree_table_accessible_row(rvn_widget_t* widget, int row, int* level, int* position, int* size, int* expandable, int* expanded) {
	rvn_tree_row_t info;
	*level = *position = *size = *expandable = *expanded = 0;
	if (!((rvn_table_t*)widget)->tree || !rvn_tree_row((rvn_table_t*)widget, row, &info)) return R_FALSE;
	*level = info.depth + 1;
	*position = info.index + 1;
	*size = info.siblings;
	*expandable = info.expandable;
	*expanded = info.expanded;
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

#define RVN_TREE_TABLE(variable, id, result) \
	RVN_WIDGET(variable##_widget, id, R_WIDGET_TREE_TABLE, result); \
	rvn_table_t* variable = (rvn_table_t*)variable##_widget

/* A stored node by number, or NULL with an error. `root`: 0 is accepted. */
static rvn_tree_node_t* rvn_tree_node_check(rvn_table_t* table, int node, int root, const char* function) {
	rvn_tree_node_t* found;
	if (table->tree->is_virtual) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "%s: the tree table is virtual", function);
		return NULL;
	}
	found = (node == 0 && !root) ? NULL : rvn_tree_node(table->tree, node);
	if (!found) rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: invalid node %d", function, node);
	return found;
}

int Rtree_table_add_node(Rid id, int parent, const char* cells) {
	return Rtree_table_insert_node(id, parent, -1, cells);
}

int Rtree_table_insert_node(Rid id, int parent, int index, const char* cells) {
	RVN_TREE_TABLE(table, id, 0);
	rvn_tree_node_t* at = rvn_tree_node_check(table, parent, R_TRUE, __func__);
	if (!at) return 0;
	return rvn_tree_insert(table, at, index, cells)->id;
}

void Rtree_table_remove_node(Rid id, int node) {
	RVN_TREE_TABLE(table, id, );
	rvn_tree_node_t* found = rvn_tree_node_check(table, node, R_FALSE, __func__);
	if (found) rvn_tree_remove(table, found);
}

int Rtree_table_move_node(Rid id, int node, int parent, int index) {
	RVN_TREE_TABLE(table, id, R_FALSE);
	rvn_tree_node_t* moved = rvn_tree_node_check(table, node, R_FALSE, __func__);
	rvn_tree_node_t* into = moved ? rvn_tree_node_check(table, parent, R_TRUE, __func__) : NULL;
	if (!into) return R_FALSE;
	if (rvn_tree_is_inside(into, moved)) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rtree_table_move_node: a node cannot go under itself");
		return R_FALSE;
	}
	/* (it may land in a closed branch) */
	rvn_tree_close_editor_in(table, moved, R_TRUE);
	if (!rvn_widget_get(id)) return R_FALSE;
	rvn_tree_detach(table);
	rvn_tree_unlink(moved);
	if (index < 0 || (uint32_t)index > into->child_count) index = (int)into->child_count;
	RVN_ARRAY_RESERVE(into->children, into->child_cap, into->child_count + 1);
	memmove(into->children + index + 1, into->children + index, sizeof(rvn_tree_node_t*) * (into->child_count - (uint32_t)index));
	into->children[index] = moved;
	into->child_count++;
	moved->parent = into;
	rvn_tree_changed(table);
	return R_TRUE;
}

int Rtree_table_is_node_valid(Rid id, int node) {
	RVN_TREE_TABLE(table, id, R_FALSE);
	if (node == 0) return R_TRUE;
	if (table->tree->is_virtual) return rvn_vtree_find_row(table, node) >= 0 || rvn_vnode_find(&table->tree->vroot, node) != NULL;
	return rvn_tree_node(table->tree, node) != NULL;
}

int Rtree_table_get_node_count(Rid id) {
	RVN_TREE_TABLE(table, id, 0);
	return table->tree->is_virtual ? 0 : table->tree->node_count;
}

int Rtree_table_get_parent(Rid id, int node) {
	RVN_TREE_TABLE(table, id, -1);
	if (node == 0) return -1;
	if (table->tree->is_virtual) {
		rvn_vnode_t* open = rvn_vnode_find(&table->tree->vroot, node);
		int row;
		if (open) return open->parent ? open->parent->key : -1;
		row = rvn_vtree_find_row(table, node);
		if (row < 0) return -1;
		row = rvn_tree_parent_row(table, row);
		return row < 0 ? 0 : Rtree_table_get_row_node(id, row);
	} else {
		rvn_tree_node_t* found = rvn_tree_node(table->tree, node);
		return found ? found->parent->id : -1;
	}
}

int Rtree_table_get_child_count(Rid id, int node) {
	RVN_TREE_TABLE(table, id, 0);
	if (table->tree->is_virtual) return rvn_vtree_count(table, node);
	{
		rvn_tree_node_t* found = rvn_tree_node(table->tree, node);
		return found ? (int)found->child_count : 0;
	}
}

int Rtree_table_get_child(Rid id, int node, int index) {
	RVN_TREE_TABLE(table, id, 0);
	if (table->tree->is_virtual) return index >= 0 && index < rvn_vtree_count(table, node) ? rvn_vtree_child(table, node, index) : 0;
	{
		rvn_tree_node_t* found = rvn_tree_node(table->tree, node);
		return found && index >= 0 && (uint32_t)index < found->child_count ? found->children[index]->id : 0;
	}
}

int Rtree_table_get_depth(Rid id, int node) {
	RVN_TREE_TABLE(table, id, -1);
	if (node == 0) return -1;
	if (table->tree->is_virtual) {
		rvn_tree_row_t info;
		int row = rvn_vtree_find_row(table, node);
		return row >= 0 && rvn_tree_row(table, row, &info) ? info.depth : -1;
	} else {
		rvn_tree_node_t* found = rvn_tree_node(table->tree, node);
		int depth = -1;
		for (; found && found != &table->tree->root; found = found->parent) depth++;
		return found ? depth : -1;
	}
}

void Rtree_table_set_node_cell(Rid id, int node, int column, const char* text) {
	RVN_TREE_TABLE(table, id, );
	rvn_tree_node_t* found = rvn_tree_node_check(table, node, R_FALSE, __func__);
	if (!found) return;
	if (column < 0 || column >= (int)table->column_count) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtree_table_set_node_cell: invalid column %d", column);
		return;
	}
	if (table->tree->columns < table->column_count) rvn_tree_columns_changed(table);
	if (rvn_str_set(&found->cells[column], text && text[0] ? text : NULL)) rvn_widget_invalidate_paint(&table->base);
}

const char* Rtree_table_get_node_cell(Rid id, int node, int column) {
	RVN_TREE_TABLE(table, id, "");
	if (table->tree->is_virtual) {
		const char* text = table->tree->text_callback && node > 0 ? table->tree->text_callback(id, node, column, table->tree->user) : NULL;
		return text ? text : "";
	} else {
		rvn_tree_node_t* found = node > 0 ? rvn_tree_node(table->tree, node) : NULL;
		if (!found || column < 0 || (uint32_t)column >= table->tree->columns || !found->cells[column]) return "";
		return found->cells[column];
	}
}

void Rtree_table_set_node_icon(Rid id, int node, Rid texture) {
	RVN_TREE_TABLE(table, id, );
	rvn_tree_node_t* found = rvn_tree_node_check(table, node, R_FALSE, __func__);
	if (!found || found->icon == texture) return;
	found->icon = texture;
	rvn_widget_invalidate_paint(&table->base);
}

Rid Rtree_table_get_node_icon(Rid id, int node) {
	RVN_TREE_TABLE(table, id, R_INVALID_ID);
	if (table->tree->is_virtual) return table->tree->icon_callback && node > 0 ? table->tree->icon_callback(id, node, table->tree->icon_user) : R_INVALID_ID;
	{
		rvn_tree_node_t* found = node > 0 ? rvn_tree_node(table->tree, node) : NULL;
		return found ? found->icon : R_INVALID_ID;
	}
}

void Rtree_table_set_node_has_children(Rid id, int node, int has_children) {
	RVN_TREE_TABLE(table, id, );
	rvn_tree_node_t* found = rvn_tree_node_check(table, node, R_FALSE, __func__);
	if (!found) return;
	found->lazy = has_children ? R_TRUE : R_FALSE;
	rvn_a11y_structure_changed();
	rvn_widget_invalidate_paint(&table->base);
}

void Rtree_table_set_expanded(Rid id, int node, int expanded) {
	RVN_TREE_TABLE(table, id, );
	if (table->tree->is_virtual) {
		int row = rvn_vtree_find_row(table, node);
		if (row >= 0) rvn_vtree_set_expanded(table, row, expanded);
		return;
	} else {
		rvn_tree_node_t* found = rvn_tree_node_check(table, node, R_FALSE, __func__);
		if (found) rvn_tree_node_set_expanded(table, found, expanded);
	}
}

int Rtree_table_is_expanded(Rid id, int node) {
	RVN_TREE_TABLE(table, id, R_FALSE);
	if (node == 0) return R_TRUE;
	if (table->tree->is_virtual) {
		rvn_vnode_t* open = rvn_vnode_find(&table->tree->vroot, node);
		return open && open->expanded;
	} else {
		rvn_tree_node_t* found = rvn_tree_node(table->tree, node);
		return found && found->expanded;
	}
}

static void rvn_tree_set_all(Rid id, int node, int expanded, const char* function) {
	RVN_TREE_TABLE(table, id, );
	if (table->tree->is_virtual) {
		if (node == 0) {
			int row;
			if (!expanded) {
				/* every top-level row closed, from the bottom (the rows above keep their place) */
				for (row = rvn_table_count(table) - 1; row >= 0 && rvn_widget_get(id); row--) {
					rvn_tree_row_t info;
					if (rvn_tree_row(table, row, &info) && info.expanded) rvn_vtree_set_expanded(table, row, R_FALSE);
				}
				return;
			}
			for (row = 0; row < rvn_table_count(table) && rvn_widget_get(id); row++) {
				rvn_tree_row_t info;
				if (rvn_tree_row(table, row, &info) && info.expandable && !info.expanded) rvn_vtree_set_expanded(table, row, R_TRUE);
			}
		} else {
			int row = rvn_vtree_find_row(table, node);
			if (row < 0) return;
			if (expanded) rvn_tree_expand_under_row(&table->base, row);
			else rvn_vtree_set_expanded(table, row, R_FALSE);
		}
		return;
	} else {
		rvn_tree_node_t* found = rvn_tree_node_check(table, node, R_TRUE, function);
		if (found) rvn_tree_set_subtree_expanded(table, found, expanded);
	}
}

void Rtree_table_expand_all(Rid id, int node) {
	rvn_tree_set_all(id, node, R_TRUE, __func__);
}

void Rtree_table_collapse_all(Rid id, int node) {
	rvn_tree_set_all(id, node, R_FALSE, __func__);
}

int Rtree_table_get_row_node(Rid id, int row) {
	rvn_tree_row_t info;
	RVN_TREE_TABLE(table, id, 0);
	return rvn_tree_row(table, row, &info) ? info.node : 0;
}

int Rtree_table_get_node_row(Rid id, int node) {
	RVN_TREE_TABLE(table, id, -1);
	if (table->tree->is_virtual) return rvn_vtree_find_row(table, node);
	rvn_tree_sync(table);
	return node > 0 ? rvn_tree_node_row(table->tree, rvn_tree_node(table->tree, node)) : -1;
}

/* Opens the parents of a stored node; its row, or -1. */
static int rvn_tree_reveal_node(rvn_table_t* table, int node) {
	rvn_tree_node_t* found;
	rvn_tree_node_t* walk;
	Rid id = table->base.id;
	if (table->tree->is_virtual) return rvn_vtree_find_row(table, node);
	found = rvn_tree_node_check(table, node, R_FALSE, "Rtree_table_reveal_node");
	if (!found) return -1;
	for (walk = found->parent; walk && walk != &table->tree->root; walk = walk->parent) {
		if (!walk->expanded && !rvn_tree_node_set_expanded(table, walk, R_TRUE)) return -1;
		if (!rvn_widget_get(id) || !rvn_tree_node(table->tree, node)) return -1;
	}
	rvn_tree_sync(table);
	return rvn_tree_node_row(table->tree, found);
}

void Rtree_table_reveal_node(Rid id, int node) {
	RVN_TREE_TABLE(table, id, );
	int row = rvn_tree_reveal_node(table, node);
	if (row < 0 || !rvn_widget_get(id)) return;
	if (table->base.window && table->base.window->needs_layout) rvn_window_layout(table->base.window);
	rvn_table_reveal(&table->base, row);
}

void Rtree_table_select_node(Rid id, int node) {
	RVN_TREE_TABLE(table, id, );
	int row = node == 0 ? -1 : rvn_tree_reveal_node(table, node);
	if (!rvn_widget_get(id)) return;
	if (table->base.window && table->base.window->needs_layout) rvn_window_layout(table->base.window);
	rvn_table_select(&table->base, row, R_FALSE);
}

int Rtree_table_get_selected_node(Rid id) {
	RVN_TREE_TABLE(table, id, 0);
	return Rtree_table_get_row_node(id, rvn_table_count(table) > 0 ? table->selected : -1);
}

void Rtree_table_set_expanded_callback(Rid id, RTreeTableExpandCallback callback, void* user_data) {
	RVN_TREE_TABLE(table, id, );
	table->tree->expand_callback = callback;
	table->tree->expand_user = user_data;
}

void Rtree_table_set_virtual(Rid id, RTreeTableChildCountCallback child_count, RTreeTableChildCallback child,
	RTreeTableHasChildrenCallback has_children, RTreeTableNodeTextCallback cell_text, void* user_data) {
	RVN_TREE_TABLE(table, id, );
	rvn_tree_t* tree = table->tree;
	if (child_count && !child) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtree_table_set_virtual: a virtual tree needs the child callback");
		return;
	}
	rvn_table_close_editor(&table->base, R_TRUE);
	if (!rvn_widget_get(id)) return;
	/* (Rtable_clear_rows clears the selection and the scroll, and the tree) */
	Rtable_clear_rows(id);
	tree->is_virtual = child_count != NULL;
	tree->count_callback = child_count;
	tree->child_callback = child;
	tree->has_children_callback = has_children;
	tree->text_callback = cell_text;
	tree->user = user_data;
	tree->vroot.key = 0;
	tree->vroot.expanded = R_TRUE;
	if (tree->is_virtual) {
		rvn_vtree_refresh_node(table, &tree->vroot);
	}
	rvn_tree_changed(table);
}

void Rtree_table_set_icon_callback(Rid id, RTreeTableNodeIconCallback callback, void* user_data) {
	RVN_TREE_TABLE(table, id, );
	table->tree->icon_callback = callback;
	table->tree->icon_user = user_data;
	rvn_widget_invalidate_paint(&table->base);
}

void Rtree_table_refresh(Rid id, int node) {
	RVN_TREE_TABLE(table, id, );
	rvn_tree_t* tree = table->tree;
	rvn_vnode_t* open;
	if (!tree->is_virtual) {
		rvn_widget_invalidate_layout(&table->base);
		return;
	}
	open = node == 0 ? &tree->vroot : rvn_vnode_find(&tree->vroot, node);
	if (open) {
		int before = open->below;
		rvn_vtree_refresh_node(table, open);
		if (open != &tree->vroot && open->expanded) rvn_vnode_grow(tree, open, open->below - before);
	}
	rvn_table_rows_refreshed(&table->base);
}

void Rtree_table_set_guide_lines(Rid id, int visible) {
	RVN_TREE_TABLE(table, id, );
	table->tree->guides = visible ? R_TRUE : R_FALSE;
	rvn_widget_invalidate_paint(&table->base);
}

int Rtree_table_get_guide_lines(Rid id) {
	RVN_TREE_TABLE(table, id, R_FALSE);
	return table->tree->guides;
}

void Rtree_table_set_type_search(Rid id, int enabled) {
	RVN_TREE_TABLE(table, id, );
	table->tree->type_search = enabled ? R_TRUE : R_FALSE;
	table->tree->search[0] = '\0';
}
