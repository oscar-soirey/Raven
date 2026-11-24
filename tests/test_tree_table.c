/**
 * Raven tests - TreeTable: a Table whose rows nest.
 *
 * The stored model (nodes added, inserted, removed with their subtree,
 * moved, numbers that never change), the rows shown as nodes open and
 * close, sorting within each parent, the selection (single and ranges)
 * following its nodes and leaving a closed branch for the branch, the
 * keyboard (Right / Left / + / - / * and the search by letters), the
 * chevron and the double click, cells edited and check boxes toggled in a
 * tree, the expanded callback that loads children, a virtual tree of
 * 100 000 nodes (opened, scrolled, selected, refreshed after a sort, in
 * bounded time), the screen reader tree (level, position, expanded) and XML.
 */

#include "test.h"
#include "../src/core/accessibility.h"
#include "../src/render/renderer.h"

#include <time.h>

#define W 500.0f
#define H 400.0f

static Rid g_window;
static int g_glyphs;

static void observe_frame(const RDrawData* data, RCapture* capture) {
	Ruint i;
	(void)capture;
	g_glyphs = 0;
	for (i = 0; i < data->instance_count; i++)
		if ((data->instances[i].flags & R_PRIM_MASK) == R_PRIM_GLYPH) g_glyphs++;
}

static float row_height(void) {
	return Rtheme_get_metric(R_METRIC_ROW_HEIGHT);
}

/* The middle of a row, the header above the rows. */
static float row_y(int row) {
	return 1.0f + row_height() + 4.0f + row_height() * ((float)row + 0.5f);
}

static double seconds(void) {
	return (double)clock() / (double)CLOCKS_PER_SEC;
}

/* The nodes of the rows shown, from the top, as "a b c" (their first cells). */
static const char* shown(Rid table) {
	static char text[1024];
	int row, count = Rtable_get_row_count(table);
	size_t used = 0;
	text[0] = '\0';
	for (row = 0; row < count && used + 32 < sizeof(text); row++)
		used += (size_t)snprintf(text + used, sizeof(text) - used, row ? " %s" : "%s", Rtable_get_cell(table, row, 0));
	return text;
}

/*
 *  docs (1)         size
 *    a.txt (2)      5
 *    b.txt (3)      30
 *  src (4)
 *    main.c (5)     100
 *    lib (6)
 *      x.c (7)      7
 *  zeta (8)         2
 */
static int g_ids[9];

static Rid make_tree(void) {
	Rid table = Rwidget_create(R_WIDGET_TREE_TABLE);
	Rwindow_set_content(g_window, table);
	Rtable_add_column(table, "Name", 160);
	Rtable_add_column(table, "Size", 80);
	g_ids[1] = Rtree_table_add_node(table, 0, "docs|");
	g_ids[2] = Rtree_table_add_node(table, g_ids[1], "a.txt|5");
	g_ids[3] = Rtree_table_add_node(table, g_ids[1], "b.txt|30");
	g_ids[4] = Rtree_table_add_node(table, 0, "src|");
	g_ids[5] = Rtree_table_add_node(table, g_ids[4], "main.c|100");
	g_ids[6] = Rtree_table_add_node(table, g_ids[4], "lib|");
	g_ids[7] = Rtree_table_add_node(table, g_ids[6], "x.c|7");
	g_ids[8] = Rtree_table_add_node(table, 0, "zeta|2");
	Rdraw_frame();
	return table;
}

static int v_count_none(Rid table, int node, void* user) {
	(void)table; (void)node; (void)user;
	return 0;
}

static void test_model(void) {
	Rid table = make_tree();
	int i, extra;
	CHECK(Rwidget_get_type(table) == R_WIDGET_TREE_TABLE);
	for (i = 1; i <= 8; i++) CHECK(g_ids[i] > 0);
	CHECK(g_ids[2] != g_ids[1] && g_ids[8] != g_ids[7]);
	CHECK(Rtree_table_get_node_count(table) == 8);
	/* closed nodes: only the top level shows */
	CHECK(Rtable_get_row_count(table) == 3);
	CHECK_STR(shown(table), "docs src zeta");
	CHECK(Rtree_table_get_child_count(table, 0) == 3);
	CHECK(Rtree_table_get_child_count(table, g_ids[4]) == 2);
	CHECK(Rtree_table_get_child(table, g_ids[4], 1) == g_ids[6]);
	CHECK(Rtree_table_get_child(table, g_ids[4], 2) == 0);
	CHECK(Rtree_table_get_parent(table, g_ids[7]) == g_ids[6]);
	CHECK(Rtree_table_get_parent(table, g_ids[1]) == 0);
	CHECK(Rtree_table_get_depth(table, g_ids[7]) == 2);
	CHECK(Rtree_table_get_depth(table, g_ids[8]) == 0);
	CHECK_STR(Rtree_table_get_node_cell(table, g_ids[5], 1), "100");
	CHECK(Rtree_table_get_node_row(table, g_ids[5]) == -1);
	CHECK(Rtree_table_get_node_row(table, g_ids[4]) == 1);
	CHECK(Rtree_table_get_row_node(table, 2) == g_ids[8]);
	CHECK(Rtree_table_is_node_valid(table, 0) && Rtree_table_is_node_valid(table, g_ids[7]));

	/* insert at an index, set a cell of a hidden node */
	extra = Rtree_table_insert_node(table, 0, 1, "between|1");
	CHECK_STR(shown(table), "docs between src zeta");
	Rtree_table_set_node_cell(table, g_ids[7], 0, "y.c");
	CHECK_STR(Rtree_table_get_node_cell(table, g_ids[7], 0), "y.c");
	/* Rtable_* on rows: the cells of the row shown */
	Rtable_set_cell(table, 1, 1, "2");
	CHECK_STR(Rtree_table_get_node_cell(table, extra, 1), "2");
	/* Rtable_add_row: a node at the top level, its row */
	CHECK(Rtable_add_row(table, "last|9") == 4);
	CHECK(Rtree_table_get_node_count(table) == 10);

	/* move: lib to the top level, before docs */
	CHECK(Rtree_table_move_node(table, g_ids[6], 0, 0));
	CHECK_STR(shown(table), "lib docs between src zeta last");
	CHECK(Rtree_table_get_parent(table, g_ids[6]) == 0);
	CHECK(Rtree_table_get_child_count(table, g_ids[4]) == 1);
	/* a node never goes under itself */
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rtree_table_move_node(table, g_ids[6], g_ids[7], 0));
	EXPECT_ERRORS_END(1);
	/* remove: the node and its subtree; the numbers of the others stay */
	Rtree_table_remove_node(table, g_ids[6]);
	CHECK(!Rtree_table_is_node_valid(table, g_ids[6]) && !Rtree_table_is_node_valid(table, g_ids[7]));
	CHECK(Rtree_table_get_node_count(table) == 8);
	CHECK_STR(shown(table), "docs between src zeta last");
	CHECK(Rtree_table_get_node_row(table, g_ids[8]) == 3);
	Rtable_remove_row(table, 1);
	CHECK_STR(shown(table), "docs src zeta last");
	/* numbers are never given again */
	CHECK(Rtree_table_add_node(table, 0, "new") > extra);

	EXPECT_ERRORS_BEGIN();
	CHECK(Rtree_table_add_node(table, 9999, "x") == 0);
	Rtree_table_remove_node(table, 0);
	Rtable_set_virtual_rows(table, 10, NULL, NULL);
	Rtree_table_set_virtual(table, v_count_none, NULL, NULL, NULL, NULL);
	EXPECT_ERRORS_END(4);
	CHECK(Rtree_table_get_node_count(table) == 8);
	/* a plain table is not a tree */
	{
		Rid plain = Rwidget_create(R_WIDGET_TABLE);
		EXPECT_ERRORS_BEGIN();
		CHECK(Rtree_table_add_node(plain, 0, "x") == 0);
		EXPECT_ERRORS_END(1);
		Rwidget_destroy(plain);
	}
	Rtable_clear_rows(table);
	CHECK(Rtable_get_row_count(table) == 0 && Rtree_table_get_node_count(table) == 0);
	Rwidget_destroy(table);
}

static void test_expand_collapse(void) {
	Rid table = make_tree();
	Rtree_table_set_expanded(table, g_ids[4], R_TRUE);
	CHECK_STR(shown(table), "docs src main.c lib zeta");
	CHECK(Rtree_table_is_expanded(table, g_ids[4]) && !Rtree_table_is_expanded(table, g_ids[6]));
	/* a node inside a closed one keeps its state */
	Rtree_table_set_expanded(table, g_ids[6], R_TRUE);
	Rtree_table_set_expanded(table, g_ids[4], R_FALSE);
	CHECK_STR(shown(table), "docs src zeta");
	Rtree_table_set_expanded(table, g_ids[4], R_TRUE);
	CHECK_STR(shown(table), "docs src main.c lib x.c zeta");
	CHECK(Rtree_table_get_node_row(table, g_ids[7]) == 4);
	Rtree_table_collapse_all(table, 0);
	CHECK_STR(shown(table), "docs src zeta");
	Rtree_table_expand_all(table, 0);
	CHECK_STR(shown(table), "docs a.txt b.txt src main.c lib x.c zeta");
	Rtree_table_collapse_all(table, g_ids[4]);
	CHECK_STR(shown(table), "docs a.txt b.txt src zeta");
	CHECK(!Rtree_table_is_expanded(table, g_ids[6]));
	Rtree_table_expand_all(table, g_ids[4]);
	CHECK_STR(shown(table), "docs a.txt b.txt src main.c lib x.c zeta");
	/* reveal opens the parents */
	Rtree_table_collapse_all(table, 0);
	Rtree_table_reveal_node(table, g_ids[7]);
	CHECK_STR(shown(table), "docs src main.c lib x.c zeta");
	Rtree_table_select_node(table, g_ids[2]);
	CHECK(Rtree_table_get_selected_node(table) == g_ids[2]);
	CHECK(Rtable_get_selected(table) == 1);
	Rwidget_destroy(table);
}

static void test_sort_within_parents(void) {
	Rid table = make_tree();
	Rtree_table_expand_all(table, 0);
	Rtree_table_select_node(table, g_ids[7]);
	Rtable_sort(table, 0, R_TRUE);
	/* each level in its own order: the children stay under their parents */
	CHECK_STR(shown(table), "zeta src main.c lib x.c docs b.txt a.txt");
	CHECK(Rtree_table_get_selected_node(table) == g_ids[7]);
	CHECK(Rtable_get_selected(table) == 4);
	/* numbers by value, then the texts (the folders have no size, they keep their order) */
	Rtable_sort(table, 1, R_FALSE);
	CHECK_STR(shown(table), "zeta src main.c lib x.c docs a.txt b.txt");
	CHECK(Rtree_table_get_node_row(table, g_ids[7]) == 4);
	CHECK(Rtree_table_get_selected_node(table) == g_ids[7]);
	/* the numbers did not change */
	CHECK(Rtree_table_get_parent(table, g_ids[3]) == g_ids[1]);
	CHECK_STR(Rtree_table_get_node_cell(table, g_ids[3], 0), "b.txt");
	CHECK(Rtable_get_sort_column(table) == 1 && !Rtable_is_sort_descending(table));
	Rwidget_destroy(table);
}

static int g_selection_calls, g_last_selected;

static void on_selection(Rid table, int row, void* user) {
	(void)table; (void)user;
	g_selection_calls++;
	g_last_selected = row;
}

static void test_selection_follows_nodes(void) {
	Rid table = make_tree();
	int first, last;
	Rtable_set_selection_changed_callback(table, on_selection, NULL);
	Rtree_table_expand_all(table, 0);
	/* single: the selection stays on its node when the rows above open and close */
	Rtree_table_select_node(table, g_ids[5]);
	CHECK(Rtable_get_selected(table) == 4);
	Rtree_table_set_expanded(table, g_ids[1], R_FALSE);
	CHECK(Rtable_get_selected(table) == 2 && Rtree_table_get_selected_node(table) == g_ids[5]);
	/* closing its branch moves it to the branch */
	Rtree_table_set_expanded(table, g_ids[4], R_FALSE);
	CHECK(Rtree_table_get_selected_node(table) == g_ids[4]);
	/* removing it clears it */
	Rtree_table_remove_node(table, g_ids[4]);
	CHECK(Rtable_get_selected(table) == -1);
	CHECK(g_selection_calls == 0);
	Rwidget_destroy(table);

	/* multiple: ranges over the rows shown */
	table = make_tree();
	Rtable_set_selection_mode(table, R_TABLE_SELECTION_MULTIPLE);
	Rtree_table_expand_all(table, 0);
	/* docs a.txt b.txt src main.c lib x.c zeta: select a.txt..main.c and zeta */
	Rtable_select_range(table, 1, 4, R_TRUE);
	Rtable_set_row_selected(table, 7, R_TRUE);
	CHECK(Rtable_get_selection_count(table) == 5);
	CHECK(Rtable_get_selection_range_count(table) == 2);
	/* closing docs: a.txt and b.txt are hidden, docs takes their place in the selection */
	Rtree_table_set_expanded(table, g_ids[1], R_FALSE);
	CHECK_STR(shown(table), "docs src main.c lib x.c zeta");
	CHECK(Rtable_is_row_selected(table, 0) && Rtable_is_row_selected(table, 1) && Rtable_is_row_selected(table, 2));
	CHECK(!Rtable_is_row_selected(table, 3) && Rtable_is_row_selected(table, 5));
	CHECK(Rtable_get_selection_range(table, 0, &first, &last) && first == 0 && last == 2);
	/* opening it again: its children come back unselected, the rows below move down */
	Rtree_table_set_expanded(table, g_ids[1], R_TRUE);
	CHECK(Rtable_get_selection_count(table) == 4);
	CHECK(Rtable_is_row_selected(table, 0) && !Rtable_is_row_selected(table, 1) && !Rtable_is_row_selected(table, 2));
	CHECK(Rtable_is_row_selected(table, 3) && Rtable_is_row_selected(table, 4) && Rtable_is_row_selected(table, 7));
	/* everything selected, then a branch closed: the ranges merge */
	Rtable_select_all(table);
	Rtree_table_set_expanded(table, g_ids[4], R_FALSE);
	CHECK(Rtable_get_row_count(table) == 5 && Rtable_get_selection_count(table) == 5);
	CHECK(Rtable_get_selection_range_count(table) == 1);
	/* sorting keeps the selected nodes */
	Rtable_clear_selection(table);
	Rtable_set_row_selected(table, 1, R_TRUE); /* a.txt */
	Rtable_sort(table, 0, R_TRUE);
	CHECK_STR(shown(table), "zeta src docs b.txt a.txt");
	CHECK(Rtable_get_selection_count(table) == 1 && Rtable_is_row_selected(table, 4));
	Rwidget_destroy(table);
}

static void test_keyboard(void) {
	Rid table = make_tree();
	Rtable_set_selection_changed_callback(table, on_selection, NULL);
	Rwidget_focus(table);
	g_selection_calls = 0;
	/* Right with nothing selected: the first row */
	test_key(g_window, R_KEY_RIGHT, 0);
	CHECK(Rtree_table_get_selected_node(table) == g_ids[1]);
	/* Right opens, Right again goes to the first child */
	test_key(g_window, R_KEY_RIGHT, 0);
	CHECK(Rtree_table_is_expanded(table, g_ids[1]));
	CHECK(Rtree_table_get_selected_node(table) == g_ids[1]);
	test_key(g_window, R_KEY_RIGHT, 0);
	CHECK(Rtree_table_get_selected_node(table) == g_ids[2]);
	/* on a leaf, Right does nothing; Left goes to the parent, Left again closes it */
	test_key(g_window, R_KEY_RIGHT, 0);
	CHECK(Rtree_table_get_selected_node(table) == g_ids[2]);
	test_key(g_window, R_KEY_LEFT, 0);
	CHECK(Rtree_table_get_selected_node(table) == g_ids[1]);
	test_key(g_window, R_KEY_LEFT, 0);
	CHECK(!Rtree_table_is_expanded(table, g_ids[1]));
	/* Down to src, * opens everything under it, - closes it, + opens it */
	test_key(g_window, R_KEY_DOWN, 0);
	CHECK(Rtree_table_get_selected_node(table) == g_ids[4]);
	Rwindow_inject_text(g_window, "*");
	CHECK_STR(shown(table), "docs src main.c lib x.c zeta");
	Rwindow_inject_text(g_window, "-");
	CHECK_STR(shown(table), "docs src zeta");
	Rwindow_inject_text(g_window, "+");
	CHECK_STR(shown(table), "docs src main.c lib x.c zeta");
	/* Left on a node inside: to the parent; up and down cross the levels */
	test_key(g_window, R_KEY_END, 0);
	CHECK(Rtree_table_get_selected_node(table) == g_ids[8]);
	test_key(g_window, R_KEY_UP, 0);
	CHECK(Rtree_table_get_selected_node(table) == g_ids[7]);
	test_key(g_window, R_KEY_LEFT, 0);
	CHECK(Rtree_table_get_selected_node(table) == g_ids[6]);
	CHECK(g_selection_calls >= 5);
	/* the search by letters: "m" goes to main.c, "z" to zeta */
	Rtree_table_set_type_search(table, R_TRUE);
	Rwindow_inject_text(g_window, "m");
	CHECK(Rtree_table_get_selected_node(table) == g_ids[5]);
	Rtree_table_set_type_search(table, R_TRUE); /* (starts a new search) */
	Rwindow_inject_text(g_window, "z");
	CHECK(Rtree_table_get_selected_node(table) == g_ids[8]);
	Rtree_table_set_type_search(table, R_TRUE);
	Rwindow_inject_text(g_window, "l");
	Rwindow_inject_text(g_window, "i");
	CHECK(Rtree_table_get_selected_node(table) == g_ids[6]);
	Rtree_table_set_type_search(table, R_FALSE);
	Rwindow_inject_text(g_window, "d");
	CHECK(Rtree_table_get_selected_node(table) == g_ids[6]);

	/* the user closes the branch of the selection: it moves to the branch, and the application is told */
	Rtree_table_set_type_search(table, R_TRUE);
	Rtree_table_select_node(table, g_ids[7]);
	g_selection_calls = 0;
	Rtree_table_select_node(table, g_ids[4]);
	Rtable_set_row_selected(table, Rtree_table_get_node_row(table, g_ids[7]), R_TRUE);
	CHECK(Rtree_table_get_selected_node(table) == g_ids[7]);
	test_key(g_window, R_KEY_LEFT, 0); /* x.c -> lib */
	test_key(g_window, R_KEY_LEFT, 0); /* close lib */
	test_key(g_window, R_KEY_LEFT, 0); /* lib -> src */
	CHECK(Rtree_table_get_selected_node(table) == g_ids[4]);
	g_selection_calls = 0;
	Rtree_table_select_node(table, g_ids[5]);
	Rtree_table_select_node(table, g_ids[5]);
	{
		/* a click on the chevron of src closes it: main.c was selected */
		float x, y, w, h;
		Rdraw_frame();
		Rwidget_get_rect(table, &x, &y, &w, &h);
		test_click_at(g_window, x + 1.0f + 4.0f + Rtheme_get_metric(R_METRIC_INDENT) * 0.5f, y + row_y(1));
	}
	CHECK(!Rtree_table_is_expanded(table, g_ids[4]));
	CHECK(Rtree_table_get_selected_node(table) == g_ids[4]);
	CHECK(g_selection_calls == 1 && g_last_selected == 1);
	Rwidget_destroy(table);
}

static void test_mouse(void) {
	Rid table = make_tree();
	float x, y, w, h, indent = Rtheme_get_metric(R_METRIC_INDENT);
	if (indent < 12.0f) indent = 12.0f;
	Rwidget_get_rect(table, &x, &y, &w, &h);
	/* a click on the chevron opens, without selecting */
	test_click_at(g_window, x + 1.0f + 4.0f + indent * 0.5f, y + row_y(0));
	CHECK(Rtree_table_is_expanded(table, g_ids[1]));
	CHECK(Rtable_get_selected(table) == -1);
	Rdraw_frame();
	/* a click on the text selects; a double click opens or closes */
	test_click_at(g_window, x + 100.0f, y + row_y(3));
	CHECK(Rtree_table_get_selected_node(table) == g_ids[4]);
	test_click_at(g_window, x + 100.0f, y + row_y(3));
	CHECK(Rtree_table_is_expanded(table, g_ids[4]));
	Rdraw_frame();
	CHECK_STR(shown(table), "docs a.txt b.txt src main.c lib zeta");
	/* the chevron of a child is one indentation further */
	test_click_at(g_window, x + 1.0f + 4.0f + indent * 1.5f, y + row_y(5));
	CHECK(Rtree_table_is_expanded(table, g_ids[6]));
	/* a leaf has none: the click selects */
	Rdraw_frame();
	test_click_at(g_window, x + 1.0f + 4.0f + indent * 1.5f, y + row_y(4));
	CHECK(Rtree_table_get_selected_node(table) == g_ids[5]);
	/* the text of a deeper row starts further right */
	Rdraw_frame();
	CHECK(g_glyphs > 0);
	Rwidget_destroy(table);
}

static int g_edits;

static int on_edited(Rid table, int row, int column, const char* text, void* user) {
	(void)table; (void)row; (void)column; (void)text; (void)user;
	g_edits++;
	return R_TRUE;
}

static void test_cells_in_a_tree(void) {
	Rid table = make_tree();
	Rid editor;
	float x, y, w, h, ex, ey, ew, eh;
	int done;
	Rtree_table_expand_all(table, 0);
	done = Rtable_add_column(table, "Done", 60);
	Rtable_set_column_kind(table, done, R_TABLE_COLUMN_CHECK);
	Rtable_set_column_editable(table, 0, R_TRUE);
	Rtable_set_cell_edited_callback(table, on_edited, NULL);
	Rdraw_frame();
	/* a check box toggled on a row writes the node */
	CHECK(!Rtable_is_cell_checked(table, 6, done));
	Rwidget_get_rect(table, &x, &y, &w, &h);
	test_click_at(g_window, x + 1.0f + 160.0f + 80.0f + 30.0f, y + row_y(6));
	CHECK(Rtable_is_cell_checked(table, 6, done));
	CHECK_STR(Rtree_table_get_node_cell(table, g_ids[7], done), "1");
	/* the cell follows its node when the rows move */
	Rtree_table_set_expanded(table, g_ids[1], R_FALSE);
	CHECK(Rtable_is_cell_checked(table, 4, done));
	/* the editor of the first column starts after the chevron and the indentation */
	CHECK(Rtable_edit_cell(table, 4, 0));
	editor = Rwidget_get_child(table, 0);
	Rdraw_frame();
	Rwidget_get_rect(editor, &ex, &ey, &ew, &eh);
	CHECK(ex > x + 1.0f + 2.0f * Rtheme_get_metric(R_METRIC_INDENT));
	Rwindow_inject_text(g_window, "z");
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK_STR(Rtree_table_get_node_cell(table, g_ids[7], 0), "z");
	CHECK(g_edits == 2);
	/* closing the branch of an edited row closes the editor */
	CHECK(Rtable_edit_cell(table, 4, 0));
	Rtree_table_set_expanded(table, g_ids[6], R_FALSE);
	CHECK(!Rtable_is_editing(table));
	/* Left and Right: the tree in its column, the cells in the others */
	Rwidget_focus(table);
	Rtree_table_select_node(table, g_ids[6]);
	test_key(g_window, R_KEY_RIGHT, 0);
	CHECK(Rtree_table_is_expanded(table, g_ids[6]));
	Rdraw_frame();
	Rwidget_destroy(table);
}

/* ---- lazy loading ---- */

static int g_expanded_calls, g_collapsed_calls;

static void on_expanded(Rid table, int node, int expanded, void* user) {
	(void)user;
	if (!expanded) {
		g_collapsed_calls++;
		return;
	}
	g_expanded_calls++;
	/* the children are made when the folder is opened the first time */
	if (Rtree_table_get_child_count(table, node) == 0) {
		int child = Rtree_table_add_node(table, node, "loaded 1|");
		Rtree_table_add_node(table, node, "loaded 2|");
		Rtree_table_set_node_has_children(table, child, R_TRUE);
	}
}

static void test_lazy_children(void) {
	Rid table = Rwidget_create(R_WIDGET_TREE_TABLE);
	int folder;
	Rwindow_set_content(g_window, table);
	Rtable_add_column(table, "Name", 160);
	folder = Rtree_table_add_node(table, 0, "folder");
	Rtree_table_add_node(table, 0, "after");
	Rtree_table_set_node_has_children(table, folder, R_TRUE);
	Rtree_table_set_expanded_callback(table, on_expanded, NULL);
	g_expanded_calls = g_collapsed_calls = 0;
	Rtable_set_selected(table, 1);
	Rtree_table_set_expanded(table, folder, R_TRUE);
	CHECK(g_expanded_calls == 1);
	CHECK_STR(shown(table), "folder loaded 1 loaded 2 after");
	CHECK(Rtree_table_get_selected_node(table) == Rtree_table_get_row_node(table, 3));
	/* the loaded child opens the same way */
	Rtree_table_set_expanded(table, Rtree_table_get_row_node(table, 1), R_TRUE);
	CHECK_STR(shown(table), "folder loaded 1 loaded 1 loaded 2 loaded 2 after");
	Rtree_table_set_expanded(table, folder, R_FALSE);
	CHECK(g_collapsed_calls == 1);
	Rtree_table_set_expanded(table, folder, R_TRUE);
	CHECK(g_expanded_calls == 3);
	CHECK(Rtable_get_row_count(table) == 6);
	Rwidget_destroy(table);
}

/* ---- a virtual tree: 100 folders of 1000 files ---- */

#define FOLDERS 100
#define FILES 1000

static int g_count_calls, g_text_calls, g_order_descending, g_virtual_loads;
static char g_text[64];

/* folders are 1..FOLDERS; the files of folder f are f * 10000 + i (i from 1) */
static int v_count(Rid table, int node, void* user) {
	(void)table; (void)user;
	g_count_calls++;
	if (node == 0) return FOLDERS;
	if (node <= FOLDERS) return FILES;
	return 0;
}

static int v_child(Rid table, int parent, int index, void* user) {
	(void)table; (void)user;
	if (g_order_descending) index = (parent == 0 ? FOLDERS : FILES) - 1 - index;
	return parent == 0 ? index + 1 : parent * 10000 + index + 1;
}

static int v_has_children(Rid table, int node, void* user) {
	(void)table; (void)user;
	return node <= FOLDERS;
}

static const char* v_text(Rid table, int node, int column, void* user) {
	(void)table; (void)user;
	g_text_calls++;
	if (column == 0) {
		if (node <= FOLDERS) snprintf(g_text, sizeof(g_text), "folder %03d", node);
		else snprintf(g_text, sizeof(g_text), "file %03d-%04d", node / 10000, node % 10000);
	} else if (column == 1) {
		snprintf(g_text, sizeof(g_text), "%d", node <= FOLDERS ? 0 : node % 10000);
	} else {
		snprintf(g_text, sizeof(g_text), "%s", (node % 3) == 0 ? "1" : "0");
	}
	return g_text;
}

static Rid v_icon(Rid table, int node, void* user) {
	(void)table; (void)user;
	return node <= FOLDERS ? Ricon_get("folder") : Ricon_get("file");
}

static void v_expanded(Rid table, int node, int expanded, void* user) {
	(void)table; (void)node; (void)user;
	if (expanded) g_virtual_loads++;
}

static void v_sorted(Rid table, int column, int descending, void* user) {
	(void)column; (void)user;
	g_order_descending = descending;
	Rtree_table_refresh(table, 0);
}

static void test_virtual_tree(void) {
	Rid table = Rwidget_create(R_WIDGET_TREE_TABLE);
	double start, took;
	int row, node, i;
	rvn_a11y_node_t a11y;
	Rwindow_set_content(g_window, table);
	Rtable_add_column(table, "Name", 160);
	Rtable_add_column(table, "Size", 80);
	Rtable_add_column(table, "Done", 50);
	Rtable_set_column_kind(table, 2, R_TABLE_COLUMN_CHECK);
	Rtable_set_selection_mode(table, R_TABLE_SELECTION_MULTIPLE);
	g_order_descending = 0;
	Rtree_table_set_virtual(table, v_count, v_child, v_has_children, v_text, NULL);
	Rtree_table_set_icon_callback(table, v_icon, NULL);
	Rtree_table_set_expanded_callback(table, v_expanded, NULL);
	Rtable_set_sort_callback(table, v_sorted, NULL);
	CHECK(Rtable_get_row_count(table) == FOLDERS);
	CHECK(Rtree_table_get_node_count(table) == 0);
	CHECK_STR(Rtable_get_cell(table, 4, 0), "folder 005");

	/* every folder opened: 100 100 rows, in bounded time */
	g_count_calls = 0;
	start = seconds();
	Rtree_table_expand_all(table, 0);
	took = seconds() - start;
	CHECK(Rtable_get_row_count(table) == FOLDERS + FOLDERS * FILES);
	CHECK(g_virtual_loads == FOLDERS);
	printf("  100 folders of 1000 files opened in %.1f ms\n", took * 1000.0);
	CHECK(took < 2.0);
	/* rows anywhere: found by going down the open nodes */
	CHECK_STR(Rtable_get_cell(table, 0, 0), "folder 001");
	CHECK_STR(Rtable_get_cell(table, 1, 0), "file 001-0001");
	CHECK_STR(Rtable_get_cell(table, 1001, 0), "folder 002");
	CHECK_STR(Rtable_get_cell(table, 1001 * 57 + 500, 0), "file 058-0500");
	CHECK(Rtree_table_get_row_node(table, 1001 * 99) == 100);
	CHECK(Rtree_table_get_node_row(table, 58 * 10000 + 500) == 1001 * 57 + 500);
	CHECK(Rtree_table_get_parent(table, 58 * 10000 + 500) == 58);
	CHECK(Rtree_table_get_depth(table, 58 * 10000 + 500) == 1);

	/* scrolling to the end and drawing costs the rows on screen only */
	g_text_calls = 0;
	start = seconds();
	for (i = 0; i < 20; i++) {
		Rtable_scroll_to_row(table, (Rtable_get_row_count(table) - 1) * i / 19);
		Rdraw_frame();
	}
	took = seconds() - start;
	printf("  20 frames scrolled across 100 100 rows in %.1f ms\n", took * 1000.0);
	CHECK(g_text_calls < 20 * 200);
	CHECK(took < 2.0);

	/* a selection across folders, then one closed: the ranges follow */
	Rtable_select_range(table, 990, 1010, R_TRUE);
	CHECK(Rtable_get_selection_count(table) == 21);
	/* closing folder 001 (row 0): rows 1..1000 go, 990..1000 were selected, so folder 001 is */
	Rtree_table_set_expanded(table, 1, R_FALSE);
	CHECK(Rtable_get_row_count(table) == FOLDERS + (FOLDERS - 1) * FILES);
	CHECK(Rtable_get_selection_count(table) == 11);
	CHECK(Rtable_is_row_selected(table, 0) && Rtable_is_row_selected(table, 1) && Rtable_is_row_selected(table, 10));
	CHECK(!Rtable_is_row_selected(table, 11));
	/* opening it again: the rows below move down */
	Rtree_table_set_expanded(table, 1, R_TRUE);
	CHECK(Rtable_get_selection_count(table) == 11);
	CHECK(Rtable_is_row_selected(table, 1001) && Rtable_is_row_selected(table, 1010) && !Rtable_is_row_selected(table, 1));

	/* a sort (two clicks on the header): the application orders its data and refreshes; open folders stay open */
	Rtree_table_set_expanded(table, 3, R_FALSE);
	{
		float x, y, w, h;
		Rwidget_get_rect(table, &x, &y, &w, &h);
		test_click_at(g_window, x + 40.0f, y + 1.0f + row_height() * 0.5f);
		CHECK(!g_order_descending && Rtable_get_sort_column(table) == 0);
		test_click_at(g_window, x + 40.0f, y + 1.0f + row_height() * 0.5f);
	}
	CHECK(g_order_descending);
	CHECK_STR(Rtable_get_cell(table, 0, 0), "folder 100");
	CHECK(Rtable_get_row_count(table) == FOLDERS + (FOLDERS - 1) * FILES);
	CHECK(!Rtree_table_is_expanded(table, 3) && Rtree_table_is_expanded(table, 100));
	CHECK_STR(Rtable_get_cell(table, 1, 0), "file 100-1000");
	row = Rtree_table_get_node_row(table, 3);
	CHECK(row > 0 && Rtree_table_get_row_node(table, row) == 3);
	CHECK_STR(Rtable_get_cell(table, row + 1, 0), "folder 002");

	/* the screen reader: a tree item with its level and place */
	CHECK(rvn_a11y_describe(rvn_a11y_child(table, 1), &a11y));
	CHECK(a11y.role == R_ROLE_TREE_ITEM && a11y.level == 2 && a11y.position == 1 && a11y.set_size == FILES);
	CHECK(rvn_a11y_describe(rvn_a11y_child(table, row), &a11y));
	CHECK(a11y.level == 1 && a11y.position == 98 && a11y.set_size == FOLDERS);
	CHECK((a11y.states & RVN_A11Y_EXPANDABLE) && !(a11y.states & RVN_A11Y_EXPANDED));
	CHECK(rvn_a11y_act(rvn_a11y_child(table, row), RVN_A11Y_EXPAND));
	CHECK(Rtree_table_is_expanded(table, 3));

	/* keys on a virtual tree */
	Rwidget_focus(table);
	Rtable_clear_selection(table);
	Rtable_set_row_selected(table, row + 5, R_TRUE);
	test_key(g_window, R_KEY_LEFT, 0);
	CHECK(Rtable_get_current_row(table) == row);
	test_key(g_window, R_KEY_LEFT, 0);
	CHECK(!Rtree_table_is_expanded(table, 3));
	node = Rtree_table_get_row_node(table, Rtable_get_current_row(table));
	CHECK(node == 3);
	/* Rtable_clear_rows leaves the virtual mode */
	Rtable_clear_rows(table);
	CHECK(Rtable_get_row_count(table) == 0);
	CHECK(Rtree_table_add_node(table, 0, "stored") > 0);
	Rwidget_destroy(table);
}

/* A virtual tree whose folders are only read when opened. */
static int g_loaded[4];

static int lazy_count(Rid table, int node, void* user) {
	(void)table; (void)user;
	if (node == 0) return 3;
	return node <= 3 && g_loaded[node] ? 2 : 0;
}

static int lazy_child(Rid table, int parent, int index, void* user) {
	(void)table; (void)user;
	return parent == 0 ? index + 1 : parent * 10 + index;
}

static int lazy_has(Rid table, int node, void* user) {
	(void)table; (void)user;
	return node <= 3;
}

static const char* lazy_text(Rid table, int node, int column, void* user) {
	(void)table; (void)column; (void)user;
	snprintf(g_text, sizeof(g_text), "n%d", node);
	return g_text;
}

static void lazy_expanded(Rid table, int node, int expanded, void* user) {
	(void)table; (void)user;
	if (expanded && node <= 3) g_loaded[node] = 1;
}

static void test_virtual_lazy(void) {
	Rid table = Rwidget_create(R_WIDGET_TREE_TABLE);
	Rwindow_set_content(g_window, table);
	Rtable_add_column(table, "Name", 160);
	memset(g_loaded, 0, sizeof(g_loaded));
	Rtree_table_set_virtual(table, lazy_count, lazy_child, lazy_has, lazy_text, NULL);
	Rtree_table_set_expanded_callback(table, lazy_expanded, NULL);
	CHECK_STR(shown(table), "n1 n2 n3");
	/* the count is asked after the callback loaded the folder */
	Rtree_table_set_expanded(table, 2, R_TRUE);
	CHECK(g_loaded[2] && !g_loaded[1]);
	CHECK_STR(shown(table), "n1 n2 n20 n21 n3");
	Rwidget_destroy(table);
}

static void test_accessibility(void) {
	Rid table = make_tree();
	rvn_a11y_node_t node;
	Rtable_set_selection_mode(table, R_TABLE_SELECTION_MULTIPLE);
	Rtree_table_set_expanded(table, g_ids[4], R_TRUE);
	/* docs src main.c lib zeta */
	CHECK(rvn_a11y_describe(table, &node));
	CHECK(node.role == R_ROLE_TABLE && (node.states & RVN_A11Y_TREE_GRID) && (node.states & RVN_A11Y_MULTI_SELECT));
	CHECK(rvn_a11y_child_count(table) == 5);
	CHECK(rvn_a11y_describe(rvn_a11y_child(table, 1), &node));
	CHECK(node.role == R_ROLE_TREE_ITEM && (node.states & RVN_A11Y_TABLE_ROW));
	CHECK(node.level == 1 && node.position == 2 && node.set_size == 3);
	CHECK((node.states & RVN_A11Y_EXPANDABLE) && (node.states & RVN_A11Y_EXPANDED));
	CHECK(strstr(node.name, "src") != NULL);
	CHECK(rvn_a11y_describe(rvn_a11y_child(table, 3), &node));
	CHECK(node.level == 2 && node.position == 2 && node.set_size == 2);
	CHECK((node.states & RVN_A11Y_EXPANDABLE) && !(node.states & RVN_A11Y_EXPANDED));
	CHECK(rvn_a11y_describe(rvn_a11y_child(table, 2), &node));
	CHECK(!(node.states & RVN_A11Y_EXPANDABLE));
	/* the screen reader opens and closes rows, and selects them */
	CHECK(rvn_a11y_act(rvn_a11y_child(table, 3), RVN_A11Y_EXPAND));
	CHECK(Rtree_table_is_expanded(table, g_ids[6]) && Rtable_get_row_count(table) == 6);
	CHECK(rvn_a11y_act(rvn_a11y_child(table, 1), RVN_A11Y_COLLAPSE));
	CHECK(Rtable_get_row_count(table) == 3);
	CHECK(rvn_a11y_act(rvn_a11y_child(table, 2), RVN_A11Y_ADD_SELECTION));
	CHECK(rvn_a11y_selection_count(table) == 1);
	CHECK(rvn_a11y_describe(rvn_a11y_child(table, 2), &node) && (node.states & RVN_A11Y_SELECTED));
	/* a plain table has no levels */
	{
		Rid plain = Rwidget_create(R_WIDGET_TABLE);
		Rwidget_destroy(table);
		Rwindow_set_content(g_window, plain);
		Rtable_add_column(plain, "A", 50);
		Rtable_add_row(plain, "x");
		CHECK(rvn_a11y_describe(rvn_a11y_child(plain, 0), &node) && node.level == 0 && node.role == R_ROLE_LIST_ITEM);
		CHECK(rvn_a11y_describe(plain, &node) && !(node.states & RVN_A11Y_TREE_GRID));
		Rwidget_destroy(plain);
	}
}

static void test_xml(void) {
	const char* xml =
		"<TreeTable Name=\"files\" GuideLines=\"true\" SelectionMode=\"Multiple\" TypeSearch=\"false\">"
		"  <Column Header=\"Name\" Width=\"200\" Editable=\"true\"/>"
		"  <Column Header=\"Size\" Align=\"Right\"/>"
		"  <Column Header=\"Used\" Kind=\"Progress\"/>"
		"  <Row Cells=\"src||0.5\" Icon=\"raven:folder\" Expanded=\"true\">"
		"    <Row Cells=\"main.c|12 KB\" Icon=\"raven:file\"/>"
		"    <Item><Cell>a|b.c</Cell><Cell>1 KB</Cell></Item>"
		"    <Row Cells=\"lazy\" HasChildren=\"true\"/>"
		"  </Row>"
		"  <Row>README.md|3 KB</Row>"
		"</TreeTable>";
	Rid table = Rwidget_create_from_xml(xml, strlen(xml));
	int src;
	CHECK(table != R_INVALID_ID);
	if (table == R_INVALID_ID) return;
	CHECK(Rwidget_get_type(table) == R_WIDGET_TREE_TABLE);
	CHECK(Rtree_table_get_guide_lines(table));
	CHECK(Rtable_get_selection_mode(table) == R_TABLE_SELECTION_MULTIPLE);
	CHECK(Rtable_get_column_kind(table, 2) == R_TABLE_COLUMN_PROGRESS);
	CHECK(Rtable_is_column_editable(table, 0));
	CHECK_STR(shown(table), "src main.c a|b.c lazy README.md");
	src = Rtree_table_get_row_node(table, 0);
	CHECK(Rtree_table_is_expanded(table, src));
	CHECK(Rtree_table_get_node_icon(table, src) == Ricon_get("folder"));
	CHECK(Rtree_table_get_node_icon(table, Rtree_table_get_row_node(table, 4)) == R_INVALID_ID);
	CHECK_STR(Rtable_get_cell(table, 2, 1), "1 KB");
	CHECK_STR(Rtable_get_cell(table, 4, 1), "3 KB");
	{
		rvn_a11y_node_t node;
		CHECK(rvn_a11y_describe(rvn_a11y_child(table, 3), &node) && (node.states & RVN_A11Y_EXPANDABLE));
	}
	Rwidget_destroy(table);
	/* other names */
	xml = "<TreeGrid><Column Header=\"A\"/><Row>x</Row></TreeGrid>";
	table = Rwidget_create_from_xml(xml, strlen(xml));
	CHECK(table != R_INVALID_ID && Rwidget_get_type(table) == R_WIDGET_TREE_TABLE && Rtable_get_row_count(table) == 1);
	Rwidget_destroy(table);
	/* a node before the columns, or an unknown element, is an error */
	EXPECT_ERRORS_BEGIN();
	xml = "<TreeTable><Row>x</Row></TreeTable>";
	table = Rwidget_create_from_xml(xml, strlen(xml));
	if (table != R_INVALID_ID) Rwidget_destroy(table);
	EXPECT_ERRORS_END(1);
}

/* A stored tree of 100 000 nodes: made, opened, sorted and closed in bounded time. */
static void test_large_stored_tree(void) {
	Rid table = Rwidget_create(R_WIDGET_TREE_TABLE);
	double start, took;
	int f, i;
	char cells[64];
	Rwindow_set_content(g_window, table);
	Rtable_add_column(table, "Name", 160);
	Rtable_add_column(table, "Size", 80);
	start = seconds();
	for (f = 0; f < 100; f++) {
		int folder;
		snprintf(cells, sizeof(cells), "folder %d|", f);
		folder = Rtree_table_add_node(table, 0, cells);
		for (i = 0; i < 999; i++) {
			snprintf(cells, sizeof(cells), "file %d|%d", i, (i * 7919) % 1000);
			Rtree_table_add_node(table, folder, cells);
		}
	}
	CHECK(Rtree_table_get_node_count(table) == 100000);
	Rtree_table_expand_all(table, 0);
	CHECK(Rtable_get_row_count(table) == 100000);
	Rtable_set_selected(table, 50000);
	Rtable_sort(table, 1, R_TRUE);
	Rdraw_frame();
	Rtree_table_collapse_all(table, 0);
	CHECK(Rtable_get_row_count(table) == 100);
	CHECK(Rtable_get_selected(table) == 50);
	took = seconds() - start;
	printf("  100 000 stored nodes made, opened, sorted, closed in %.1f ms\n", took * 1000.0);
	CHECK(took < 3.0);
	Rwidget_destroy(table);
}

int main(void) {
	RBackendVTable backend = *rvn_backend_null();
	RInitDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_CUSTOM;
	desc.headless = R_TRUE;
	desc.worker_threads = 1;
	desc.disable_render_thread = R_TRUE;
	backend.RenderFrame = observe_frame;
	Rbackend_register(&backend);
	Rerror_set_callback(test_error_callback);
	if (!Rinit_ex(&desc)) return 1;
	Rset_animations_enabled(R_FALSE);
	Rlook_set(R_LOOK_RAVEN_DARK);
	g_window = Rwindow_create("TreeTable", (int)W, (int)H, 0);
	RUN(test_model);
	RUN(test_expand_collapse);
	RUN(test_sort_within_parents);
	RUN(test_selection_follows_nodes);
	RUN(test_keyboard);
	RUN(test_mouse);
	RUN(test_cells_in_a_tree);
	RUN(test_lazy_children);
	RUN(test_virtual_tree);
	RUN(test_virtual_lazy);
	RUN(test_accessibility);
	RUN(test_xml);
	RUN(test_large_stored_tree);
	Rshutdown();
	return test_report("test_tree_table");
}
