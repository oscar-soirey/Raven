#include "test.h"
#include "../src/core/accessibility.h"
#include "../src/render/renderer.h"

static int g_red, g_green, g_color_calls, g_changes;

static void observe_frame(const RDrawData* data, RCapture* capture) {
	Ruint i;
	(void)capture;
	g_red = g_green = 0;
	for (i = 0; i < data->instance_count; i++) {
		const RDrawInstance* instance = &data->instances[i];
		if ((instance->flags & R_PRIM_MASK) != R_PRIM_GLYPH) continue;
		if (instance->color == 0xFF0000FFu) g_red++;
		if (instance->color == 0xFF00FF00u) g_green++;
	}
}

static void selection_changed(Rid table, int row, void* user) {
	(void)table; (void)row; (void)user;
	g_changes++;
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
	Rshutdown();
	return test_report("test_table_features");
}
