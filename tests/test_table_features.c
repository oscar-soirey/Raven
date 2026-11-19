/**
 * Raven tests - Table: cell colors, multiple selection (by the mouse, the
 * keyboard, a finger and the API; kept as ranges, with a million virtual
 * rows; through sorting and removed rows), no selection, and the check box
 * and progress bar columns.
 */

#include "test.h"
#include "../src/core/accessibility.h"
#include "../src/render/renderer.h"

static int g_red, g_green, g_color_calls, g_changes, g_last_row;
/* non-text instances of the color 0xFFFF00FF (the accent, in the tests that set it) and their clip widths */
static int g_magenta;
static float g_magenta_widths[16];

static void observe_frame(const RDrawData* data, RCapture* capture) {
	Ruint i;
	(void)capture;
	g_red = g_green = g_magenta = 0;
	for (i = 0; i < data->instance_count; i++) {
		const RDrawInstance* instance = &data->instances[i];
		if ((instance->flags & R_PRIM_MASK) != R_PRIM_GLYPH) {
			if (instance->color == 0xFFFF00FFu) {
				if (g_magenta < 16) g_magenta_widths[g_magenta] = instance->clip[2] - instance->clip[0];
				g_magenta++;
			}
			continue;
		}
		if (instance->color == 0xFF0000FFu) g_red++;
		if (instance->color == 0xFF00FF00u) g_green++;
	}
}

static void selection_changed(Rid table, int row, void* user) {
	(void)table; (void)user;
	g_changes++;
	g_last_row = row;
}

static void click_row(Rid window, int row, Ruint modifiers) {
	Rwindow_inject_key(window, R_KEY_LEFT_CTRL, R_TRUE, modifiers);
	test_click_at(window, 20, 1 + Rtheme_get_metric(R_METRIC_ROW_HEIGHT) * (row + 0.5f));
	Rwindow_inject_key(window, R_KEY_LEFT_CTRL, R_FALSE, 0);
}

static void test_selection(void) {
	Rid window = Rwindow_create("Table", 400, 300, 0);
	Rid table = Rwidget_create(R_WIDGET_TABLE);
	int i;
	rvn_a11y_node_t node;
	Rwindow_set_content(window, table);
	Rtable_add_column(table, "Value", 200);
	Rtable_set_header_visible(table, R_FALSE);
	for (i = 0; i < 6; i++) Rtable_add_row(table, "row");
	Rtable_set_selection_changed_callback(table, selection_changed, NULL);
	Rdraw_frame();
	click_row(window, 1, 0);
	CHECK(Rtable_get_selection_count(table) == 1);
	Rtable_set_multi_select(table, R_TRUE);
	click_row(window, 3, R_MOD_CTRL);
	CHECK(Rtable_get_selection_count(table) == 2);
	CHECK(Rtable_get_selection(table, 0) == 1 && Rtable_get_selection(table, 1) == 3);
	CHECK(Rtable_get_selected(table) == 3);
	click_row(window, 5, R_MOD_SHIFT);
	CHECK(Rtable_get_selection_count(table) == 3);
	CHECK(Rtable_get_selection(table, 0) == 3 && Rtable_get_selection(table, 2) == 5);
	test_key(window, R_KEY_UP, R_MOD_SHIFT);
	CHECK(Rtable_get_selection_count(table) == 2);
	CHECK(Rtable_get_selected(table) == 4);
	click_row(window, 0, R_MOD_SUPER);
	CHECK(Rtable_get_selection_count(table) == 3);
	click_row(window, 0, R_MOD_SUPER);
	CHECK(!Rtable_is_row_selected(table, 0));
	CHECK(g_changes == 6);
	CHECK(rvn_a11y_describe(table, &node) && (node.states & RVN_A11Y_MULTI_SELECT));
	CHECK(rvn_a11y_child_count(table) == 6);
	CHECK(rvn_a11y_selection_count(table) == 2);
	CHECK(rvn_a11y_describe(rvn_a11y_child(table, 3), &node));
	CHECK((node.states & (RVN_A11Y_TABLE_ROW | RVN_A11Y_SELECTED)) == (RVN_A11Y_TABLE_ROW | RVN_A11Y_SELECTED));
	CHECK(node.height > 0 && node.height < 40);
	CHECK(rvn_a11y_act(rvn_a11y_child(table, 1), RVN_A11Y_ADD_SELECTION));
	CHECK(Rtable_get_selection_count(table) == 3);
	CHECK(rvn_a11y_act(rvn_a11y_child(table, 3), RVN_A11Y_REMOVE_SELECTION));
	CHECK(!Rtable_is_row_selected(table, 3));
	Rtable_set_selected(table, 2);
	CHECK(Rtable_get_selection_count(table) == 1 && Rtable_is_row_selected(table, 2));
	Rtable_set_row_selected(table, 4, R_TRUE);
	Rtable_set_multi_select(table, R_FALSE);
	CHECK(Rtable_get_selection_count(table) == 1 && Rtable_get_selected(table) == 4);
	Rtable_clear_selection(table);
	CHECK(Rtable_get_selection_count(table) == 0 && Rtable_get_selected(table) == -1);
	CHECK(Rtable_get_selection(table, -1) == -1 && Rtable_get_selection(table, 0) == -1);
	EXPECT_ERRORS_BEGIN();
	Rtable_set_row_selected(table, 99, R_TRUE);
	EXPECT_ERRORS_END(1);
	Rwindow_destroy(window);
}

static int stored_color(Rid table, int row, int column, float* r, float* g, float* b, float* a, void* user) {
	(void)table; (void)user;
	*r = 0; *g = 1; *b = 0; *a = 1;
	if (row == 0 && column == 1) *a = 0; /* alpha 0: keep the fixed color */
	return row == 0;
}

static void test_colors_and_sorting(void) {
	Rid window = Rwindow_create("Colors", 400, 300, 0);
	Rid table = Rwidget_create(R_WIDGET_TABLE);
	int i;
	Rwindow_set_content(window, table);
	Rtable_add_column(table, "Key", 100);
	Rtable_add_column(table, "Name", 100);
	Rtable_add_row(table, "b|B");
	Rtable_add_row(table, "a|A");
	Rtable_add_row(table, "c|C");
	Rtable_set_multi_select(table, R_TRUE);
	Rtable_set_row_selected(table, 0, R_TRUE);
	Rtable_set_row_selected(table, 2, R_TRUE);
	Rtable_set_cell_text_color(table, 0, -1, 1, 0, 0, 1);
	Rtable_set_cell_text_color(table, 0, 1, 0, 1, 0, 1);
	Rdraw_frame();
	CHECK(g_red == 1 && g_green == 1); /* explicit colors apply to selected rows too */
	Rtable_set_cell_color_callback(table, stored_color, NULL);
	Rdraw_frame();
	CHECK(g_red == 0 && g_green == 2); /* the callback wins over the row color; alpha 0 keeps the cell color */
	Rtable_set_cell_color_callback(table, NULL, NULL);
	Rdraw_frame();
	CHECK(g_red == 1 && g_green == 1);
	Rtable_sort(table, 0, R_FALSE);
	CHECK(Rtable_get_selection(table, 0) == 1 && Rtable_get_selection(table, 1) == 2);
	CHECK_STR(Rtable_get_cell(table, Rtable_get_selected(table), 0), "c");
	Rdraw_frame();
	CHECK(g_red == 1 && g_green == 1);
	Rtable_remove_row(table, 0);
	CHECK(Rtable_is_row_selected(table, 0) && Rtable_is_row_selected(table, 1));
	Rtable_add_column(table, "Extra", 100);
	for (i = 0; i < 20; i++) Rtable_add_row(table, NULL); /* grow all parallel storage */
	Rdraw_frame();
	CHECK(g_red == 1 && g_green == 1);
	Rtable_set_cell_text_color(table, 0, 1, 0, 0, 0, 0);
	Rdraw_frame();
	CHECK(g_red == 2 && g_green == 0);
	Rtable_set_cell_text_color(table, 0, -1, 0, 0, 0, 0);
	Rdraw_frame();
	CHECK(g_red == 0 && g_green == 0);
	Rtable_remove_row(table, 1);
	CHECK(Rtable_get_selection_count(table) == 1);
	Rtable_clear_rows(table);
	Rtable_add_row(table, "b|B");
	Rdraw_frame();
	CHECK(g_red == 0 && g_green == 0 && Rtable_get_selection_count(table) == 0);
	Rwindow_destroy(window);
}

static const char* virtual_text(Rid table, int row, int column, void* user) {
	(void)table; (void)row; (void)column; (void)user;
	return "V";
}

static int virtual_color(Rid table, int row, int column, float* r, float* g, float* b, float* a, void* user) {
	(void)table; (void)column; (void)user;
	g_color_calls++;
	*r = 1; *g = 0; *b = 0; *a = 1;
	return row == 0;
}

static void test_virtual_rows(void) {
	Rid window = Rwindow_create("Virtual", 300, 200, 0);
	Rid table = Rwidget_create(R_WIDGET_TABLE);
	Rwindow_set_content(window, table);
	Rtable_add_column(table, "Key", 200);
	Rtable_set_virtual_rows(table, 100000, virtual_text, NULL);
	Rtable_set_cell_color_callback(table, virtual_color, NULL);
	Rtable_set_multi_select(table, R_TRUE);
	Rtable_set_row_selected(table, 3, R_TRUE);
	Rtable_set_row_selected(table, 999, R_TRUE);
	Rdraw_frame();
	CHECK(g_red == 1 && g_color_calls > 0 && g_color_calls < 30);
	Rtable_set_virtual_rows(table, 4, virtual_text, NULL);
	CHECK(Rtable_get_selection_count(table) == 1 && Rtable_is_row_selected(table, 3));
	Rtable_set_virtual_rows(table, 100000, virtual_text, NULL);
	CHECK(!Rtable_is_row_selected(table, 999));
	g_color_calls = 0;
	Rtable_scroll_to_row(table, 99999);
	Rdraw_frame();
	CHECK(g_red == 0 && g_color_calls > 0 && g_color_calls < 30);
	Rtable_set_cell_color_callback(table, NULL, NULL);
	Rtable_set_virtual_rows(table, 0, NULL, NULL);
	CHECK(Rtable_get_selection_count(table) == 0);
	Rwindow_destroy(window);
}

/* ---------------------------------------------------------------------------
 *  Multiple selection: mouse, keyboard, finger, API
 * ------------------------------------------------------------------------- */

static int set_attr(Rid widget, const char* name, const char* value) {
	return Rwidget_set_attribute(widget, name, value);
}

static float row_y(int row) {
	return 1 + Rtheme_get_metric(R_METRIC_ROW_HEIGHT) * (row + 0.5f);
}

/* A stored table of `rows` rows "0".."n-1", no header, in a 400 x 300 window. */
static Rid make_rows(Rid window, int rows) {
	Rid table = Rwidget_create(R_WIDGET_TABLE);
	char text[16];
	int i;
	Rwindow_set_content(window, table);
	Rtable_add_column(table, "Value", 200);
	Rtable_set_header_visible(table, R_FALSE);
	for (i = 0; i < rows; i++) {
		snprintf(text, sizeof(text), "%d", i);
		Rtable_add_row(table, text);
	}
	Rtable_set_selection_changed_callback(table, selection_changed, NULL);
	return table;
}

/* "1-3,5": the selection as ranges, to compare in one CHECK. */
static const char* ranges_of(Rid table) {
	static char text[256];
	int i, first, last;
	size_t used = 0;
	text[0] = '\0';
	for (i = 0; i < Rtable_get_selection_range_count(table) && used + 32 < sizeof(text); i++) {
		Rtable_get_selection_range(table, i, &first, &last);
		used += (size_t)snprintf(text + used, sizeof(text) - used, first == last ? "%s%d" : "%s%d-%d", i ? "," : "", first, last);
	}
	return text;
}

static void test_multiple_selection(void) {
	Rid window = Rwindow_create("Multiple", 400, 300, 0);
	Rid table = make_rows(window, 40);
	Rdraw_frame();
	CHECK(Rtable_get_selection_mode(table) == R_TABLE_SELECTION_SINGLE);
	Rtable_set_selection_mode(table, R_TABLE_SELECTION_MULTIPLE);
	CHECK(Rtable_get_selection_mode(table) == R_TABLE_SELECTION_MULTIPLE);
	g_changes = 0;
	/* a click selects one row, Ctrl toggles, Shift extends from the anchor */
	click_row(window, 2, 0);
	CHECK_STR(ranges_of(table), "2");
	click_row(window, 4, R_MOD_CTRL);
	click_row(window, 5, R_MOD_SUPER);
	CHECK_STR(ranges_of(table), "2,4-5");
	CHECK(Rtable_get_selected(table) == 5 && Rtable_get_current_row(table) == 5);
	click_row(window, 8, R_MOD_SHIFT);
	CHECK_STR(ranges_of(table), "5-8");
	click_row(window, 2, R_MOD_SHIFT | R_MOD_CTRL);
	CHECK_STR(ranges_of(table), "2-8");
	click_row(window, 4, R_MOD_CTRL);
	CHECK_STR(ranges_of(table), "2-3,5-8");
	CHECK(Rtable_get_current_row(table) == 4 && !Rtable_is_row_selected(table, 4));
	CHECK(Rtable_get_selected(table) == 2);
	CHECK(g_changes == 6 && g_last_row == 2);

	/* the keyboard: Shift with the arrows, Page Down, End and Home extends from the anchor */
	click_row(window, 3, 0);
	test_key(window, R_KEY_DOWN, R_MOD_SHIFT);
	test_key(window, R_KEY_DOWN, R_MOD_SHIFT);
	CHECK_STR(ranges_of(table), "3-5");
	CHECK(Rtable_get_selected(table) == 5);
	test_key(window, R_KEY_PAGE_DOWN, R_MOD_SHIFT);
	CHECK(Rtable_get_selection_count(table) > 5 && Rtable_get_selection(table, 0) == 3);
	test_key(window, R_KEY_END, R_MOD_SHIFT);
	CHECK_STR(ranges_of(table), "3-39");
	test_key(window, R_KEY_HOME, R_MOD_SHIFT);
	CHECK_STR(ranges_of(table), "0-3");
	/* Ctrl with the arrows moves the current row only; Space toggles it, Shift+Space extends to it */
	test_key(window, R_KEY_DOWN, R_MOD_CTRL);
	test_key(window, R_KEY_DOWN, R_MOD_CTRL);
	test_key(window, R_KEY_DOWN, R_MOD_CTRL);
	test_key(window, R_KEY_DOWN, R_MOD_CTRL);
	test_key(window, R_KEY_DOWN, R_MOD_CTRL);
	test_key(window, R_KEY_DOWN, R_MOD_CTRL);
	CHECK(Rtable_get_current_row(table) == 6);
	CHECK_STR(ranges_of(table), "0-3");
	g_changes = 0;
	test_key(window, R_KEY_SPACE, 0);
	CHECK_STR(ranges_of(table), "0-3,6");
	test_key(window, R_KEY_SPACE, 0);
	CHECK_STR(ranges_of(table), "0-3");
	test_key(window, R_KEY_SPACE, 0);
	Rwindow_inject_text(window, " "); /* the character of Space is not typed anywhere */
	test_key(window, R_KEY_DOWN, R_MOD_CTRL);
	test_key(window, R_KEY_DOWN, R_MOD_CTRL);
	test_key(window, R_KEY_SPACE, R_MOD_SHIFT);
	CHECK_STR(ranges_of(table), "6-8");
	CHECK(g_changes == 4);
	/* a plain arrow goes back to one row */
	test_key(window, R_KEY_UP, 0);
	CHECK_STR(ranges_of(table), "7");
	/* Ctrl/Cmd+A selects every row */
	g_changes = 0;
	test_key(window, R_KEY_A, R_MOD_CTRL);
	CHECK_STR(ranges_of(table), "0-39");
	CHECK(Rtable_get_selection_count(table) == 40 && Rtable_get_selected(table) == 7);
	Rtable_clear_selection(table);
	test_key(window, R_KEY_A, R_MOD_SUPER);
	CHECK(Rtable_get_selection_count(table) == 40);
	CHECK(g_changes == 2);
	/* the screen reader sees the rows selected and the current one focused */
	{
		rvn_a11y_node_t node;
		CHECK(rvn_a11y_selection_count(table) == 40);
		Rtable_set_selected(table, 9);
		CHECK(rvn_a11y_describe(rvn_a11y_child(table, 9), &node) && (node.states & RVN_A11Y_FOCUSED) && (node.states & RVN_A11Y_SELECTED));
	}

	/* the API: ranges merge and split, and fire no callback */
	g_changes = 0;
	Rtable_clear_selection(table);
	Rtable_select_range(table, 10, 12, R_TRUE);
	Rtable_select_range(table, 14, 15, R_TRUE);
	CHECK_STR(ranges_of(table), "10-12,14-15");
	Rtable_select_range(table, 13, 13, R_TRUE);
	CHECK_STR(ranges_of(table), "10-15");
	Rtable_select_range(table, 12, 13, R_FALSE);
	CHECK_STR(ranges_of(table), "10-11,14-15");
	Rtable_set_row_selected(table, 20, R_TRUE);
	CHECK(Rtable_get_selection_count(table) == 5 && Rtable_get_selection(table, 4) == 20 && Rtable_get_selection(table, 5) == -1);
	CHECK(Rtable_get_selected(table) == 20 && Rtable_get_current_row(table) == 20);
	Rtable_select_all(table);
	CHECK_STR(ranges_of(table), "0-39");
	CHECK(g_changes == 0);
	{
		int first = 7, last = 7;
		CHECK(!Rtable_get_selection_range(table, 1, &first, &last) && first == -1 && last == -1);
	}
	EXPECT_ERRORS_BEGIN();
	Rtable_select_range(table, 5, 99, R_TRUE);
	Rtable_select_range(table, 5, 4, R_TRUE);
	Rtable_set_selection_mode(table, (RTableSelectionMode)12345);
	EXPECT_ERRORS_END(3);

	/* back to single selection: the selected row stays alone */
	Rtable_set_selected(table, 3);
	Rtable_select_range(table, 5, 6, R_TRUE);
	Rtable_set_selection_mode(table, R_TABLE_SELECTION_SINGLE);
	CHECK(Rtable_get_selection_count(table) == 1 && Rtable_get_selected(table) == 6);
	CHECK_STR(ranges_of(table), "6");
	/* single selection keeps its keys: Space does nothing, Ctrl+A neither, Shift+Down moves */
	test_key(window, R_KEY_SPACE, 0);
	test_key(window, R_KEY_A, R_MOD_CTRL);
	test_key(window, R_KEY_DOWN, R_MOD_SHIFT);
	CHECK_STR(ranges_of(table), "7");
	Rtable_select_range(table, 1, 2, R_TRUE);
	CHECK_STR(ranges_of(table), "2");
	Rtable_select_all(table);
	CHECK_STR(ranges_of(table), "2");
	Rwindow_destroy(window);
}

static void test_no_selection(void) {
	Rid window = Rwindow_create("None", 400, 300, 0);
	Rid table = make_rows(window, 100);
	rvn_a11y_node_t node;
	set_attr(table, "SelectionMode", "None");
	CHECK(Rtable_get_selection_mode(table) == R_TABLE_SELECTION_NONE);
	Rdraw_frame();
	g_changes = 0;
	click_row(window, 2, 0);
	click_row(window, 3, R_MOD_CTRL);
	test_key(window, R_KEY_A, R_MOD_CTRL);
	test_key(window, R_KEY_SPACE, 0);
	Rtable_set_selected(table, 4);
	Rtable_set_row_selected(table, 4, R_TRUE);
	CHECK(Rtable_get_selection_count(table) == 0 && Rtable_get_selected(table) == -1 && g_changes == 0);
	/* the keys scroll */
	test_key(window, R_KEY_END, 0);
	Rdraw_frame();
	CHECK(rvn_a11y_describe(rvn_a11y_child(table, 0), &node) && (node.states & RVN_A11Y_OFFSCREEN));
	CHECK(!(node.states & RVN_A11Y_SELECTABLE));
	test_key(window, R_KEY_HOME, 0);
	Rdraw_frame();
	CHECK(rvn_a11y_describe(rvn_a11y_child(table, 0), &node) && !(node.states & RVN_A11Y_OFFSCREEN));
	/* XML: Multiple, then Single; MultiSelect still works */
	CHECK(set_attr(table, "SelectionMode", "Multiple"));
	CHECK(Rtable_get_selection_mode(table) == R_TABLE_SELECTION_MULTIPLE);
	CHECK(set_attr(table, "MultiSelect", "false"));
	CHECK(Rtable_get_selection_mode(table) == R_TABLE_SELECTION_SINGLE);
	CHECK(!set_attr(table, "SelectionMode", "Several"));
	Rwindow_destroy(window);
}

static void test_finger_toggles(void) {
	Rid window = Rwindow_create("Finger", 400, 300, 0);
	Rid table = make_rows(window, 10);
	Rtable_set_multi_select(table, R_TRUE);
	Rdraw_frame();
	Rwindow_inject_touch(window, 1, R_TOUCH_DOWN, 20, row_y(1));
	Rwindow_inject_touch(window, 1, R_TOUCH_UP, 20, row_y(1));
	Rwindow_inject_touch(window, 1, R_TOUCH_DOWN, 20, row_y(4));
	Rwindow_inject_touch(window, 1, R_TOUCH_UP, 20, row_y(4));
	CHECK_STR(ranges_of(table), "1,4");
	Rwindow_inject_touch(window, 1, R_TOUCH_DOWN, 20, row_y(1));
	Rwindow_inject_touch(window, 1, R_TOUCH_UP, 20, row_y(1));
	CHECK_STR(ranges_of(table), "4");
	/* the mouse still selects one row */
	click_row(window, 6, 0);
	CHECK_STR(ranges_of(table), "6");
	/* in single selection a tap selects its row */
	Rtable_set_multi_select(table, R_FALSE);
	Rwindow_inject_touch(window, 1, R_TOUCH_DOWN, 20, row_y(2));
	Rwindow_inject_touch(window, 1, R_TOUCH_UP, 20, row_y(2));
	CHECK_STR(ranges_of(table), "2");
	Rwindow_destroy(window);
}

static void test_sort_and_remove_keep_rows(void) {
	Rid window = Rwindow_create("Sort", 400, 300, 0);
	Rid table = make_rows(window, 0);
	const char* names[] = { "e", "b", "d", "a", "c", "f" };
	int i;
	for (i = 0; i < 6; i++) Rtable_add_row(table, names[i]);
	Rtable_set_multi_select(table, R_TRUE);
	/* e, d, a and f selected; the current row is d */
	Rtable_select_range(table, 2, 3, R_TRUE);
	Rtable_set_row_selected(table, 5, R_TRUE);
	Rtable_set_row_selected(table, 0, R_TRUE);
	click_row(window, 2, R_MOD_CTRL);
	click_row(window, 2, R_MOD_CTRL);
	CHECK(Rtable_get_current_row(table) == 2);
	/* a sorted stored table moves its rows: the selection goes with them (a, d, e, f) */
	Rtable_sort(table, 0, R_FALSE);
	CHECK_STR(ranges_of(table), "0,3-5");
	CHECK_STR(Rtable_get_cell(table, Rtable_get_current_row(table), 0), "d");
	CHECK_STR(Rtable_get_cell(table, Rtable_get_selected(table), 0), "d");
	Rtable_sort(table, 0, R_TRUE);
	CHECK_STR(ranges_of(table), "0-2,5");
	/* removing a row moves the rows under it up, and joins the ranges around it */
	Rtable_remove_row(table, 3); /* c */
	CHECK_STR(ranges_of(table), "0-2,4");
	Rtable_set_row_selected(table, 3, R_TRUE);
	CHECK_STR(ranges_of(table), "0-4");
	Rtable_remove_row(table, 0);
	CHECK_STR(ranges_of(table), "0-3");
	Rtable_set_row_selected(table, 2, R_FALSE);
	Rtable_remove_row(table, 2);
	CHECK_STR(ranges_of(table), "0-2");
	Rtable_remove_row(table, 2);
	CHECK_STR(ranges_of(table), "0-1");
	CHECK(Rtable_get_row_count(table) == 2);
	Rwindow_destroy(window);
}

/* The ranges against a flag per row, through random changes. */
static void test_ranges_against_flags(void) {
	Rid window = Rwindow_create("Ranges", 400, 300, 0);
	Rid table = make_rows(window, 64);
	unsigned char flags[64];
	unsigned seed = 12345;
	int step, i, rows = 64;
	memset(flags, 0, sizeof(flags));
	Rtable_set_multi_select(table, R_TRUE);
	for (step = 0; step < 3000; step++) {
		int a, b, on, ok = R_TRUE, count = 0, n;
		seed = seed * 1103515245u + 12345u;
		a = (int)((seed >> 8) % (unsigned)rows);
		b = rvn_mini(rows - 1, a + (int)((seed >> 20) % 9u));
		on = (seed >> 4) & 1;
		switch ((seed >> 16) % 7u) {
			case 0: case 1: case 2:
				Rtable_select_range(table, a, b, on);
				for (i = a; i <= b; i++) flags[i] = (unsigned char)on;
				break;
			case 3:
				Rtable_set_row_selected(table, a, on);
				flags[a] = (unsigned char)on;
				break;
			case 4:
				if (rows > 8) {
					Rtable_remove_row(table, a);
					memmove(flags + a, flags + a + 1, (size_t)(rows - a - 1));
					rows--;
				}
				break;
			case 5:
				if (rows < 64) {
					Rtable_add_row(table, "x");
					flags[rows++] = 0;
				}
				break;
			default:
				if ((seed >> 24) % 16u == 0) {
					Rtable_select_all(table);
					memset(flags, 1, (size_t)rows);
				}
				break;
		}
		for (i = 0; i < rows; i++) {
			if (Rtable_is_row_selected(table, i) != flags[i]) ok = R_FALSE;
			if (flags[i] && Rtable_get_selection(table, count++) != i) ok = R_FALSE;
		}
		if (Rtable_get_selection_count(table) != count) ok = R_FALSE;
		/* ranges sorted, apart: never two that touch */
		for (n = 0; n + 1 < Rtable_get_selection_range_count(table); n++) {
			int f0, l0, f1, l1;
			Rtable_get_selection_range(table, n, &f0, &l0);
			Rtable_get_selection_range(table, n + 1, &f1, &l1);
			if (!(f0 <= l0 && l0 + 1 < f1 && f1 <= l1)) ok = R_FALSE;
		}
		if (!ok) {
			CHECK(ok);
			break;
		}
	}
	Rwindow_destroy(window);
}

static const char* million_text(Rid table, int row, int column, void* user) {
	static char text[16];
	(void)table; (void)column; (void)user;
	snprintf(text, sizeof(text), "%d", row);
	return text;
}

static void test_million_virtual_rows(void) {
	Rid window = Rwindow_create("Million", 400, 300, 0);
	Rid table = Rwidget_create(R_WIDGET_TABLE);
	int i, sum = 0;
	Rwindow_set_content(window, table);
	Rtable_add_column(table, "Row", 200);
	Rtable_set_header_visible(table, R_FALSE);
	Rtable_set_virtual_rows(table, 1000000, million_text, NULL);
	Rtable_set_multi_select(table, R_TRUE);
	Rtable_set_selection_changed_callback(table, selection_changed, NULL);
	Rdraw_frame();
	/* Ctrl+A over a million rows is one range */
	click_row(window, 1, 0);
	g_changes = 0;
	test_key(window, R_KEY_A, R_MOD_CTRL);
	CHECK(Rtable_get_selection_count(table) == 1000000 && Rtable_get_selection_range_count(table) == 1);
	CHECK(Rtable_is_row_selected(table, 999999) && Rtable_get_selection(table, 765432) == 765432);
	CHECK(g_changes == 1);
	/* holes cut it in pieces; the count and the n-th row follow */
	for (i = 0; i < 1000; i++) Rtable_set_row_selected(table, i * 1000 + 500, R_FALSE);
	CHECK(Rtable_get_selection_range_count(table) == 1001);
	CHECK(Rtable_get_selection_count(table) == 999000);
	CHECK(!Rtable_is_row_selected(table, 123500) && Rtable_is_row_selected(table, 123501));
	CHECK(Rtable_get_selection(table, 500) == 501);
	/* Shift+End from the anchor to the last row, after scrolling there */
	click_row(window, 2, 0);
	test_key(window, R_KEY_END, R_MOD_SHIFT);
	CHECK(Rtable_get_selection_count(table) == 999998 && Rtable_get_selected(table) == 999999);
	Rdraw_frame();
	/* the rows drawn ask whether they are selected without a cost per row of the table */
	for (i = 0; i < Rtable_get_selection_range_count(table); i++) {
		int first, last;
		Rtable_get_selection_range(table, i, &first, &last);
		sum += last - first + 1;
	}
	CHECK(sum == 999998);
	/* a smaller count drops the rows that went away from the selection */
	Rtable_set_virtual_rows(table, 500000, million_text, NULL);
	CHECK(Rtable_get_selection_count(table) == 499998 && Rtable_get_selection_range_count(table) == 1);
	CHECK(Rtable_get_selected(table) == 2 && Rtable_get_current_row(table) == 2);
	Rtable_select_range(table, 0, 499999, R_FALSE);
	CHECK(Rtable_get_selection_count(table) == 0 && Rtable_get_selected(table) == -1);
	Rwindow_destroy(window);
}

/* ---------------------------------------------------------------------------
 *  Check box and progress bar columns
 * ------------------------------------------------------------------------- */

/* A click with modifiers at x in a row (no header). */
static void click_cell(Rid window, float x, int row, Ruint modifiers) {
	Rwindow_inject_key(window, R_KEY_LEFT_CTRL, R_TRUE, modifiers);
	test_click_at(window, x, row_y(row));
	Rwindow_inject_key(window, R_KEY_LEFT_CTRL, R_FALSE, 0);
}

static int g_toggles, g_toggle_row, g_toggle_column, g_toggle_refuse;
static char g_toggle_text[8];

static int on_toggled(Rid table, int row, int column, const char* text, void* user) {
	(void)table; (void)user;
	g_toggles++;
	g_toggle_row = row;
	g_toggle_column = column;
	snprintf(g_toggle_text, sizeof(g_toggle_text), "%s", text);
	return !g_toggle_refuse;
}

static void test_check_column(void) {
	Rid window = Rwindow_create("Check", 400, 300, 0);
	Rid table = Rwidget_create(R_WIDGET_TABLE);
	rvn_a11y_node_t node;
	Rwindow_set_content(window, table);
	Rtable_add_column(table, "Done", 60);
	Rtable_add_column(table, "Task", 200);
	Rtable_set_header_visible(table, R_FALSE);
	Rtable_add_row(table, "1|Write");
	Rtable_add_row(table, "0|Test");
	Rtable_add_row(table, "|Ship");
	Rtable_add_row(table, "yes|Rest");
	Rtable_set_column_kind(table, 0, R_TABLE_COLUMN_CHECK);
	CHECK(Rtable_get_column_kind(table, 0) == R_TABLE_COLUMN_CHECK && Rtable_get_column_kind(table, 1) == R_TABLE_COLUMN_TEXT);
	CHECK(Rtable_is_column_editable(table, 0));
	CHECK(Rtable_is_cell_checked(table, 0, 0) && !Rtable_is_cell_checked(table, 1, 0) && !Rtable_is_cell_checked(table, 2, 0));
	CHECK(Rtable_is_cell_checked(table, 3, 0));
	Rtable_set_cell_edited_callback(table, on_toggled, NULL);
	Rtable_set_multi_select(table, R_TRUE);
	Rtable_set_selection_changed_callback(table, selection_changed, NULL);
	Rdraw_frame();

	/* a click on the box toggles it, and leaves the selection alone */
	click_cell(window, 100, 3, 0);
	CHECK_STR(ranges_of(table), "3");
	g_changes = 0;
	test_click_at(window, 30, row_y(1));
	CHECK(g_toggles == 1 && g_toggle_row == 1 && g_toggle_column == 0);
	CHECK_STR(g_toggle_text, "1");
	CHECK(Rtable_is_cell_checked(table, 1, 0));
	CHECK_STR(Rtable_get_cell(table, 1, 0), "1");
	CHECK_STR(ranges_of(table), "3");
	CHECK(g_changes == 0 && !Rtable_is_editing(table));
	/* refused: the cell stays as it was */
	g_toggle_refuse = R_TRUE;
	test_click_at(window, 30, row_y(1));
	CHECK(g_toggles == 2 && Rtable_is_cell_checked(table, 1, 0));
	g_toggle_refuse = R_FALSE;
	/* each click of a double click toggles, and nothing is edited or activated */
	test_click_at(window, 30, row_y(2));
	test_click_at(window, 30, row_y(2));
	CHECK(g_toggles == 4 && !Rtable_is_cell_checked(table, 2, 0) && !Rtable_is_editing(table));
	/* Ctrl+click on the box still toggles the row's selection */
	click_cell(window, 30, 0, R_MOD_CTRL);
	CHECK_STR(ranges_of(table), "0,3");
	CHECK(g_toggles == 4);

	/* the keyboard: Space, Enter and F2 toggle the current cell, a typed character does nothing */
	click_cell(window, 100, 0, 0); /* (the current cell is in the Task column) */
	test_key(window, R_KEY_LEFT, 0);
	test_key(window, R_KEY_SPACE, 0);
	Rwindow_inject_text(window, " ");
	CHECK(g_toggles == 5 && !Rtable_is_cell_checked(table, 0, 0));
	CHECK_STR(ranges_of(table), "0");
	test_key(window, R_KEY_ENTER, 0);
	CHECK(g_toggles == 6 && Rtable_is_cell_checked(table, 0, 0));
	test_key(window, R_KEY_F2, 0);
	CHECK(g_toggles == 7 && !Rtable_is_cell_checked(table, 0, 0));
	Rwindow_inject_text(window, "x");
	CHECK(!Rtable_is_editing(table) && g_toggles == 7);
	/* the Task column is not editable: Space there toggles the row, in multiple selection */
	test_key(window, R_KEY_RIGHT, 0);
	test_key(window, R_KEY_SPACE, 0);
	CHECK(Rtable_get_selection_count(table) == 0 && g_toggles == 7);

	/* the screen reader hears the box in words */
	Rtable_set_cell_checked(table, 0, 0, R_TRUE);
	CHECK(rvn_a11y_describe(rvn_a11y_child(table, 0), &node));
	CHECK(strstr(node.name, "checked, Write") != NULL && strstr(node.name, "not") == NULL);
	CHECK(rvn_a11y_describe(rvn_a11y_child(table, 2), &node) && strstr(node.name, "not checked, Ship") != NULL);

	/* a column only shown: no toggle */
	Rtable_set_column_editable(table, 0, R_FALSE);
	test_click_at(window, 30, row_y(1));
	CHECK(g_toggles == 7 && Rtable_get_selection_count(table) == 1 && Rtable_is_row_selected(table, 1));

	/* sorted by its state: unchecked first, whatever the text says it with */
	Rtable_sort(table, 0, R_FALSE);
	CHECK(!Rtable_is_cell_checked(table, 0, 0) && Rtable_is_cell_checked(table, 3, 0));

	/* Tab in the editor goes past the check boxes */
	Rtable_set_column_editable(table, 0, R_TRUE);
	Rtable_set_column_editable(table, 1, R_TRUE);
	Rtable_set_cell_edited_callback(table, NULL, NULL);
	CHECK(Rtable_edit_cell(table, 0, 1));
	test_key(window, R_KEY_TAB, 0);
	CHECK(Rtable_is_editing(table) && Rtable_get_current_row(table) == 1);
	Rtable_end_edit(table, R_FALSE);
	Rwindow_destroy(window);
}

static const char* virtual_check(Rid table, int row, int column, void* user) {
	(void)table; (void)user;
	if (column == 0) return ((unsigned char*)user)[row] ? "1" : "0";
	return "item";
}

static int virtual_toggled(Rid table, int row, int column, const char* text, void* user) {
	(void)table; (void)column;
	((unsigned char*)user)[row] = text[0] == '1';
	g_toggles++;
	return R_TRUE;
}

static void test_virtual_check_column(void) {
	Rid window = Rwindow_create("Virtual check", 400, 300, 0);
	Rid table = Rwidget_create(R_WIDGET_TABLE);
	static unsigned char states[200000];
	Rwindow_set_content(window, table);
	Rtable_add_column(table, "On", 60);
	Rtable_add_column(table, "Item", 200);
	Rtable_set_header_visible(table, R_FALSE);
	Rtable_set_virtual_rows(table, 200000, virtual_check, states);
	set_attr(table, "SelectionMode", "Multiple");
	Rtable_set_column_kind(table, 0, R_TABLE_COLUMN_CHECK);
	Rtable_set_cell_edited_callback(table, virtual_toggled, states);
	g_toggles = 0;
	Rdraw_frame();
	test_click_at(window, 30, row_y(2));
	CHECK(g_toggles == 1 && states[2] == 1 && Rtable_is_cell_checked(table, 2, 0));
	test_click_at(window, 30, row_y(2));
	CHECK(g_toggles == 2 && states[2] == 0);
	Rwindow_destroy(window);
}

static void test_cells_painted(void) {
	Rid window = Rwindow_create("Painted", 400, 300, 0);
	Rid table = Rwidget_create(R_WIDGET_TABLE);
	rvn_a11y_node_t node;
	float r, g, b, a;
	Rtheme_get_color(R_COLOR_ACCENT, &r, &g, &b, &a);
	Rtheme_set_color(R_COLOR_ACCENT, 1, 0, 1, 1);
	Rwindow_set_content(window, table);
	Rtable_add_column(table, "Progress", 200);
	Rtable_set_header_visible(table, R_FALSE);
	Rtable_set_column_kind(table, 0, R_TABLE_COLUMN_PROGRESS);
	CHECK(!Rtable_is_column_editable(table, 0));
	Rtable_add_row(table, "0.25");
	Rtable_add_row(table, "75%");
	Rtable_add_row(table, "");      /* empty: nothing */
	Rtable_add_row(table, "0");     /* the track only */
	Rtable_add_row(table, "n/a");   /* not a number: nothing */
	Rdraw_frame();
	/* two filled bars, the second three times as long as the first */
	CHECK(g_magenta == 2);
	CHECK(g_magenta_widths[0] > 10.0f && fabsf(g_magenta_widths[1] - g_magenta_widths[0] * 3.0f) < 2.0f);
	CHECK(g_red == 0);
	CHECK(rvn_a11y_describe(rvn_a11y_child(table, 1), &node) && strcmp(node.name, "75%") == 0);
	/* sorted by value, whatever the notation; the empty and wrong cells after them */
	Rtable_sort(table, 0, R_FALSE);
	CHECK_STR(Rtable_get_cell(table, 1, 0), "0.25");
	CHECK_STR(Rtable_get_cell(table, 2, 0), "75%");

	/* check boxes: the checked ones are filled with the accent */
	Rtable_clear_columns(table);
	Rtable_add_column(table, "Done", 100);
	set_attr(table, "SelectionMode", "None");
	Rtable_add_row(table, "1");
	Rtable_add_row(table, "0");
	Rtable_add_row(table, "true");
	Rtable_set_column_kind(table, 0, R_TABLE_COLUMN_CHECK);
	Rdraw_frame();
	CHECK(g_magenta == 2);
	/* (in a table that selects nothing, the box is still toggled) */
	test_click_at(window, 50, row_y(1));
	Rdraw_frame();
	CHECK(g_magenta == 3 && Rtable_get_selection_count(table) == 0);
	Rtheme_set_color(R_COLOR_ACCENT, r, g, b, a);
	Rwindow_destroy(window);
}

static void test_xml_columns(void) {
	const char* xml =
		"<Table Name=\"tasks\" SelectionMode=\"Multiple\">"
		"    <Column Header=\"Done\" Kind=\"Check\"/>"
		"    <Column Header=\"Shown\" Kind=\"Check\" Editable=\"false\" Align=\"Left\"/>"
		"    <Column Header=\"Progress\" Kind=\"Progress\"/>"
		"    <Column Header=\"Task\"/>"
		"    <Row>1|0|0.5|Write</Row>"
		"</Table>";
	Rid table = Rwidget_create_from_xml(xml, strlen(xml));
	CHECK(table != R_INVALID_ID);
	CHECK(Rtable_get_selection_mode(table) == R_TABLE_SELECTION_MULTIPLE);
	CHECK(Rtable_get_column_kind(table, 0) == R_TABLE_COLUMN_CHECK && Rtable_is_column_editable(table, 0));
	CHECK(Rtable_get_column_kind(table, 1) == R_TABLE_COLUMN_CHECK && !Rtable_is_column_editable(table, 1));
	CHECK(Rtable_get_column_kind(table, 2) == R_TABLE_COLUMN_PROGRESS);
	CHECK(Rtable_get_column_kind(table, 3) == R_TABLE_COLUMN_TEXT);
	CHECK(Rtable_is_cell_checked(table, 0, 0));
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
	RUN(test_selection);
	RUN(test_colors_and_sorting);
	RUN(test_virtual_rows);
	RUN(test_multiple_selection);
	RUN(test_no_selection);
	RUN(test_finger_toggles);
	RUN(test_sort_and_remove_keep_rows);
	RUN(test_ranges_against_flags);
	RUN(test_million_virtual_rows);
	RUN(test_check_column);
	RUN(test_virtual_check_column);
	RUN(test_cells_painted);
	RUN(test_xml_columns);
	Rshutdown();
	return test_report("test_table_features");
}
