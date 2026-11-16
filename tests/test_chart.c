/**
 * Raven tests - Chart: pie, donut, gauge, heatmap, sparkline.
 *
 * The API and its errors, what is under the pointer (Rchart_get_hovered after
 * Rwindow_inject_mouse_move), the clicks and their callbacks, the copies of
 * the arrays given, and the XML.
 */

#include "test.h"

static Rid g_window;

static struct {
	int calls;
	int slice, selected;
	int row, column;
	float value;
} g_seen;

static Rid content(const char* xml) {
	Rid root = Rwidget_create_from_xml(xml, strlen(xml));
	Rwindow_set_content(g_window, root);
	Rdraw_frame();
	return root;
}

static void center_of(Rid widget, float* cx, float* cy) {
	float x, y, w, h;
	Rwidget_get_rect(widget, &x, &y, &w, &h);
	*cx = x + w * 0.5f;
	*cy = y + h * 0.5f;
}

static void on_slice(Rid chart, int slice, int selected, void* user_data) {
	(void)chart;
	CHECK(user_data == &g_seen);
	g_seen.calls++;
	g_seen.slice = slice;
	g_seen.selected = selected;
}

static void on_cell(Rid chart, int row, int column, float value, void* user_data) {
	(void)chart;
	CHECK(user_data == &g_seen);
	g_seen.calls++;
	g_seen.row = row;
	g_seen.column = column;
	g_seen.value = value;
}

static void test_create(void) {
	Rid chart = Rchart_create(R_CHART_DONUT), other, label = Rwidget_create(R_WIDGET_LABEL);
	CHECK(chart != R_INVALID_ID);
	CHECK(Rwidget_get_type(chart) == R_WIDGET_CHART);
	CHECK(Rchart_get_kind(chart) == R_CHART_DONUT);
	Rchart_set_kind(chart, R_CHART_GAUGE);
	CHECK(Rchart_get_kind(chart) == R_CHART_GAUGE);
	CHECK_STR(Rchart_get_title(chart), "");
	Rchart_set_title(chart, "Load");
	CHECK_STR(Rchart_get_title(chart), "Load");

	/* Rwidget_create gives a pie */
	other = Rwidget_create(R_WIDGET_CHART);
	CHECK(Rchart_get_kind(other) == R_CHART_PIE);
	CHECK(Rchart_get_hovered(other) == -1);
	Rwidget_destroy(other);

	EXPECT_ERRORS_BEGIN();
	CHECK(Rchart_create((RChartKind)12345) == R_INVALID_ID);
	Rchart_set_kind(chart, (RChartKind)0);
	Rchart_set_format(chart, "%d items");
	Rchart_set_format(chart, "%f and %f");
	Rchart_set_format(chart, "no number");
	Rchart_set_color(chart, NAN, 0.0f, 0.0f, 1.0f);
	/* not a chart */
	Rchart_set_value(label, 1.0);
	EXPECT_ERRORS_END(7);
	CHECK(Rchart_get_kind(chart) == R_CHART_GAUGE);
	Rchart_set_format(chart, "%.1f%%");
	Rchart_set_format(chart, NULL);
	Rwidget_destroy(chart);
	Rwidget_destroy(label);
}

static void test_slices(void) {
	Rid chart = Rchart_create(R_CHART_PIE);
	CHECK(Rchart_get_slice_count(chart) == 0);
	CHECK(Rchart_add_slice(chart, "a", 30.0) == 0);
	CHECK(Rchart_add_slice(chart, "b", 50.0) == 1);
	CHECK(Rchart_add_slice(chart, NULL, 20.0) == 2);
	CHECK(Rchart_get_slice_count(chart) == 3);
	CHECK(Rchart_get_total(chart) == 100.0);
	CHECK_STR(Rchart_get_slice_label(chart, 1), "b");
	CHECK_STR(Rchart_get_slice_label(chart, 2), "");
	Rchart_set_slice_label(chart, 2, "c");
	CHECK_STR(Rchart_get_slice_label(chart, 2), "c");
	Rchart_set_slice_value(chart, 0, 10.0);
	CHECK(Rchart_get_slice_value(chart, 0) == 10.0);
	CHECK(Rchart_get_total(chart) == 80.0);
	Rchart_set_slice_color(chart, 1, 1.0f, 0.0f, 0.0f, 1.0f);
	Rchart_set_slice_color(chart, 1, 0.0f, 0.0f, 0.0f, 0.0f);

	/* the selection follows the slices that go */
	Rchart_set_selected(chart, 2);
	CHECK(Rchart_get_selected(chart) == 2);
	Rchart_remove_slice(chart, 0);
	CHECK(Rchart_get_slice_count(chart) == 2);
	CHECK_STR(Rchart_get_slice_label(chart, 0), "b");
	CHECK(Rchart_get_selected(chart) == 1);
	Rchart_remove_slice(chart, 1);
	CHECK(Rchart_get_selected(chart) == -1);
	Rchart_set_hole(chart, 5.0f);
	Rchart_set_center_text(chart, "1.2k", "visits");
	Rchart_set_center_text(chart, NULL, NULL);

	EXPECT_ERRORS_BEGIN();
	CHECK(Rchart_add_slice(chart, "negative", -1.0) == -1);
	CHECK(Rchart_add_slice(chart, "nan", NAN) == -1);
	CHECK(Rchart_add_slice(chart, "infinite", INFINITY) == -1);
	Rchart_set_slice_value(chart, 0, -5.0);
	Rchart_set_slice_value(chart, 7, 1.0);
	CHECK(Rchart_get_slice_value(chart, -1) == 0.0);
	CHECK_STR(Rchart_get_slice_label(chart, 9), "");
	Rchart_remove_slice(chart, 3);
	Rchart_set_selected(chart, 4);
	Rchart_set_hole(chart, NAN);
	EXPECT_ERRORS_END(10);
	CHECK(Rchart_get_slice_count(chart) == 1);
	CHECK(Rchart_get_slice_value(chart, 0) == 50.0);

	Rchart_clear(chart);
	CHECK(Rchart_get_slice_count(chart) == 0);
	CHECK(Rchart_get_total(chart) == 0.0);
	Rwidget_destroy(chart);
}

/* The pointer over a pie and a donut: the slice under it, the legend, the clicks. */
static void test_pie_pointer(void) {
	Rid root = content("<HBox><Chart Name='pie' Kind='Pie' Legend='false' Width='220' Height='300' Slot.VAlign='Top'/>"
		"<Chart Name='donut' Kind='Donut' Width='400' Height='300' Slot.VAlign='Top'/></HBox>");
	Rid pie = Rwidget_find(root, "pie"), donut = Rwidget_find(root, "donut");
	float cx, cy, x, y, w, h;

	/* two halves: the first from the top, clockwise (the right half), the second the left one */
	Rchart_add_slice(pie, "right", 1.0);
	Rchart_add_slice(pie, "left", 1.0);
	Rchart_set_slice_clicked_callback(pie, on_slice, &g_seen);
	Rdraw_frame();
	center_of(pie, &cx, &cy);
	Rwindow_inject_mouse_move(g_window, cx + 60.0f, cy + 10.0f);
	CHECK(Rchart_get_hovered(pie) == 0);
	Rwindow_inject_mouse_move(g_window, cx - 60.0f, cy - 10.0f);
	CHECK(Rchart_get_hovered(pie) == 1);
	/* out of the disc */
	Rwindow_inject_mouse_move(g_window, cx + 140.0f, cy + 140.0f);
	CHECK(Rchart_get_hovered(pie) == -1);
	Rdraw_frame();

	/* a click selects the slice, a second one unselects it */
	memset(&g_seen, 0, sizeof(g_seen));
	test_click_at(g_window, cx - 60.0f, cy);
	CHECK(g_seen.calls == 1 && g_seen.slice == 1 && g_seen.selected == 1);
	CHECK(Rchart_get_selected(pie) == 1);
	test_click_at(g_window, cx - 60.0f, cy);
	CHECK(g_seen.calls == 2 && g_seen.slice == 1 && g_seen.selected == 0);
	CHECK(Rchart_get_selected(pie) == -1);
	/* a slice of nothing takes no room */
	Rchart_set_slice_value(pie, 0, 0.0);
	Rwindow_inject_mouse_move(g_window, cx + 60.0f, cy + 10.0f);
	CHECK(Rchart_get_hovered(pie) == 1);
	Rdraw_frame();

	/* the hole of a donut is not a slice; the legend is */
	Rchart_add_slice(donut, "one", 3.0);
	Rchart_add_slice(donut, "two", 1.0);
	Rdraw_frame();
	Rwidget_get_rect(donut, &x, &y, &w, &h);
	Rwindow_inject_mouse_move(g_window, x + 4.0f + 2.0f, y + h * 0.5f);
	Rdraw_frame();
	Rwindow_inject_mouse_move(g_window, x + w - 60.0f, y + h * 0.5f + 12.0f);
	CHECK(Rchart_get_hovered(donut) == 1);
	Rwindow_inject_mouse_move(g_window, x + w - 60.0f, y + h * 0.5f - 12.0f);
	CHECK(Rchart_get_hovered(donut) == 0);
	Rdraw_frame();
	Rchart_set_legend(donut, R_FALSE);
	Rdraw_frame();
	center_of(donut, &cx, &cy);
	Rwindow_inject_mouse_move(g_window, cx, cy);
	CHECK(Rchart_get_hovered(donut) == -1);
	Rwindow_inject_mouse_move(g_window, cx + 2.0f, cy - 120.0f);
	CHECK(Rchart_get_hovered(donut) == 0);
	Rdraw_frame();
	Rwindow_inject_mouse_move(g_window, 1.0f, 470.0f);
	CHECK(Rchart_get_hovered(donut) == -1);
	Rdraw_frame();
}

static void test_gauge(void) {
	Rid root = content("<VBox><Chart Name='g' Kind='Gauge' Width='240' Height='200'/></VBox>");
	Rid gauge = Rwidget_find(root, "g");
	double min = -1.0, max = -1.0;
	Rchart_get_range(gauge, &min, &max);
	CHECK(min == 0.0 && max == 100.0);
	Rchart_set_range(gauge, -20.0, 40.0);
	Rchart_get_range(gauge, &min, &max);
	CHECK(min == -20.0 && max == 40.0);
	Rchart_set_value(gauge, 25.0);
	CHECK(Rchart_get_value(gauge) == 25.0);
	/* (a value out of the range is kept, and drawn at the end of the arc) */
	Rchart_set_value(gauge, 99.0);
	CHECK(Rchart_get_value(gauge) == 99.0);
	CHECK(Rchart_add_band(gauge, -20.0, 10.0, 0.0f, 0.0f, 0.0f, 0.0f) == 0);
	CHECK(Rchart_add_band(gauge, 30.0, 10.0, 1.0f, 0.5f, 0.0f, 1.0f) == 1);
	CHECK(Rchart_get_band_count(gauge) == 2);
	Rdraw_frame();
	/* min == max: back to 0 .. 100 */
	Rchart_set_range(gauge, 5.0, 5.0);
	Rchart_get_range(gauge, &min, &max);
	CHECK(min == 0.0 && max == 100.0);
	CHECK(Rwidget_get_accessible_role(gauge) == R_ROLE_PROGRESS_BAR);

	EXPECT_ERRORS_BEGIN();
	Rchart_set_range(gauge, 10.0, 0.0);
	Rchart_set_range(gauge, NAN, 1.0);
	Rchart_set_value(gauge, NAN);
	CHECK(Rchart_add_band(gauge, INFINITY, 1.0, 0.0f, 0.0f, 0.0f, 1.0f) == -1);
	EXPECT_ERRORS_END(4);
	CHECK(Rchart_get_value(gauge) == 99.0);
	Rchart_clear_bands(gauge);
	CHECK(Rchart_get_band_count(gauge) == 0);
	Rdraw_frame();
}

static void test_heatmap(void) {
	Rid root = content("<VBox><Chart Name='h' Kind='Heatmap' Legend='false' Width='400' Height='200'/></VBox>");
	Rid map = Rwidget_find(root, "h");
	float values[6] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
	float x, y, w, h;
	double min, max;

	Rchart_set_grid(map, 2, 3, values);
	/* the values were copied */
	values[0] = 100.0f;
	CHECK(Rchart_get_row_count(map) == 2 && Rchart_get_column_count(map) == 3);
	CHECK(Rchart_get_cell(map, 0, 0) == 1.0f);
	CHECK(Rchart_get_cell(map, 1, 2) == 6.0f);
	Rchart_get_range(map, &min, &max);
	CHECK(min == 1.0 && max == 6.0);
	Rchart_set_cell(map, 0, 1, 9.0f);
	CHECK(Rchart_get_cell(map, 0, 1) == 9.0f);
	Rchart_set_cell(map, 1, 1, NAN);
	Rchart_set_row_labels(map, "first|second");
	Rchart_set_column_labels(map, "a||c");
	CHECK_STR(Rchart_get_row_label(map, 1), "second");
	CHECK_STR(Rchart_get_column_label(map, 1), "");
	CHECK_STR(Rchart_get_column_label(map, 2), "c");
	CHECK_STR(Rchart_get_column_label(map, 3), "");
	Rchart_set_color_scale(map, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	Rchart_set_cell_clicked_callback(map, on_cell, &g_seen);
	Rdraw_frame();

	/* the labels on the left and on the top, the cells in the rest: point at the lower right cell */
	Rwidget_get_rect(map, &x, &y, &w, &h);
	Rwindow_inject_mouse_move(g_window, x + w - 10.0f, y + h - 10.0f);
	CHECK(Rchart_get_hovered(map) == 1 * 3 + 2);
	Rdraw_frame();
	memset(&g_seen, 0, sizeof(g_seen));
	test_click_at(g_window, x + w - 10.0f, y + h - 10.0f);
	CHECK(g_seen.calls == 1 && g_seen.row == 1 && g_seen.column == 2 && g_seen.value == 6.0f);
	/* the row labels are not cells */
	Rwindow_inject_mouse_move(g_window, x + 2.0f, y + h - 10.0f);
	CHECK(Rchart_get_hovered(map) == -1);

	/* a fixed range */
	Rchart_set_range(map, 0.0, 10.0);
	Rchart_get_range(map, &min, &max);
	CHECK(min == 0.0 && max == 10.0);
	Rchart_set_range(map, 0.0, 0.0);

	/* a new shape; NULL: zeros */
	Rchart_set_grid(map, 4, 1, NULL);
	CHECK(Rchart_get_row_count(map) == 4 && Rchart_get_column_count(map) == 1);
	CHECK(Rchart_get_cell(map, 3, 0) == 0.0f);
	Rdraw_frame();

	EXPECT_ERRORS_BEGIN();
	Rchart_set_grid(map, -1, 3, values);
	Rchart_set_grid(map, 100000, 100000, NULL);
	Rchart_set_cell(map, 4, 0, 1.0f);
	CHECK(Rchart_get_cell(map, 0, 1) == 0.0f);
	EXPECT_ERRORS_END(4);
	CHECK(Rchart_get_row_count(map) == 4);

	Rchart_clear(map);
	CHECK(Rchart_get_row_count(map) == 0 && Rchart_get_column_count(map) == 0);
	CHECK_STR(Rchart_get_row_label(map, 0), "");
	Rdraw_frame();
}

static void test_sparkline(void) {
	Rid root = content("<VBox><Chart Name='s' Kind='Sparkline' Width='200' Height='40'/></VBox>");
	Rid spark = Rwidget_find(root, "s");
	float values[5] = { 1.0f, 3.0f, 2.0f, NAN, 5.0f };
	float x, y, w, h;
	int i;
	double min, max;

	Rchart_set_values(spark, values, 5);
	values[0] = 50.0f;
	CHECK(Rchart_get_value_count(spark) == 5);
	CHECK(Rchart_get_value_at(spark, 0) == 1.0f);
	Rchart_get_range(spark, &min, &max);
	CHECK(min == 1.0 && max == 5.0);
	Rdraw_frame();

	/* the pointer takes the nearest value */
	Rwidget_get_rect(spark, &x, &y, &w, &h);
	Rwindow_inject_mouse_move(g_window, x + 1.0f, y + h * 0.5f);
	CHECK(Rchart_get_hovered(spark) == 0);
	Rwindow_inject_mouse_move(g_window, x + w - 1.0f, y + h * 0.5f);
	CHECK(Rchart_get_hovered(spark) == 4);
	Rwindow_inject_mouse_move(g_window, x + w * 0.5f, y + h * 0.5f);
	CHECK(Rchart_get_hovered(spark) == 2);
	Rdraw_frame();
	Rwindow_inject_mouse_move(g_window, 630.0f, 470.0f);
	CHECK(Rchart_get_hovered(spark) == -1);

	/* live data: the last ones kept */
	for (i = 0; i < 10; i++) Rchart_push_value(spark, (float)i, 4);
	CHECK(Rchart_get_value_count(spark) == 4);
	CHECK(Rchart_get_value_at(spark, 0) == 6.0f && Rchart_get_value_at(spark, 3) == 9.0f);
	Rchart_push_value(spark, 10.0f, 0);
	CHECK(Rchart_get_value_count(spark) == 5);
	/* as many values: they glide */
	Rchart_set_values(spark, values, 5);
	CHECK(Rchart_get_value_at(spark, 0) == 50.0f);
	Rchart_set_fill(spark, R_FALSE);
	Rchart_set_marker(spark, R_FALSE);
	Rchart_set_color(spark, 0.2f, 0.6f, 0.4f, 1.0f);
	Rdraw_frame();

	EXPECT_ERRORS_BEGIN();
	Rchart_set_values(spark, NULL, 3);
	Rchart_set_values(spark, values, -1);
	Rchart_push_value(spark, 1.0f, -2);
	CHECK(Rchart_get_value_at(spark, 5) == 0.0f);
	EXPECT_ERRORS_END(4);
	CHECK(Rchart_get_value_count(spark) == 5);
	Rchart_set_values(spark, NULL, 0);
	CHECK(Rchart_get_value_count(spark) == 0);
	Rdraw_frame();
}

static void test_xml(void) {
	Rid root = content(
		"<VBox>"
		"<Chart Name='d' Kind='Donut' Title='Browsers' Hole='0.5' CenterText='10k' CenterCaption='visits' Format='%.0f' Selected='1'>"
		"<Slice Label='Chrome' Value='60'/><Slice Value='30' Color='#ff9f0a'>Safari</Slice><Slice Label='Other' Value='10'/>"
		"</Chart>"
		"<Chart Name='g' Kind='Gauge' Min='10' Max='50' Value='42' Format='%.0f%%'>"
		"<Band From='10' To='30'/><Band From='45' To='30' Color='#e5484d'/>"
		"</Chart>"
		"<Chart Name='h' Kind='Heatmap' ColumnLabels='a|b|c' LowColor='#ffffff' HighColor='#0000ff'>"
		"<Row Label='Mon' Values='1 2 3'/><Row Label='Tue' Values='4 5'/><Row>7 8 9 10</Row>"
		"</Chart>"
		"<Chart Name='s' Kind='Sparkline' Values='1, 2, 3; 4 nan 6' Fill='false' Marker='false' Color='#22c55e'/>"
		"</VBox>");
	Rid donut = Rwidget_find(root, "d"), gauge = Rwidget_find(root, "g"), map = Rwidget_find(root, "h"), spark = Rwidget_find(root, "s");
	double min, max;

	CHECK(Rchart_get_kind(donut) == R_CHART_DONUT);
	CHECK_STR(Rchart_get_title(donut), "Browsers");
	CHECK(Rchart_get_slice_count(donut) == 3);
	CHECK_STR(Rchart_get_slice_label(donut, 1), "Safari");
	CHECK(Rchart_get_slice_value(donut, 1) == 30.0);
	CHECK(Rchart_get_selected(donut) == 1);
	CHECK(Rwidget_get_accessible_role(donut) == R_ROLE_IMAGE);
	CHECK_STR(Rwidget_get_accessible_name(donut), "Browsers");

	CHECK(Rchart_get_kind(gauge) == R_CHART_GAUGE);
	Rchart_get_range(gauge, &min, &max);
	CHECK(min == 10.0 && max == 50.0);
	CHECK(Rchart_get_value(gauge) == 42.0);
	CHECK(Rchart_get_band_count(gauge) == 2);

	/* the rows: the grid as wide as the longest, NaN where a row is shorter */
	CHECK(Rchart_get_row_count(map) == 3 && Rchart_get_column_count(map) == 4);
	CHECK(Rchart_get_cell(map, 1, 1) == 5.0f);
	CHECK(Rchart_get_cell(map, 0, 3) != Rchart_get_cell(map, 0, 3));
	CHECK(Rchart_get_cell(map, 2, 3) == 10.0f);
	CHECK_STR(Rchart_get_row_label(map, 1), "Tue");
	CHECK_STR(Rchart_get_row_label(map, 2), "");
	CHECK_STR(Rchart_get_column_label(map, 2), "c");

	CHECK(Rchart_get_value_count(spark) == 6);
	CHECK(Rchart_get_value_at(spark, 3) == 4.0f);
	CHECK(Rchart_get_value_at(spark, 4) != Rchart_get_value_at(spark, 4));
	Rdraw_frame();

	/* the values that are not */
	EXPECT_ERRORS_BEGIN();
	content("<Chart Kind='Bar'/>");
	content("<Chart Format='%d'/>");
	content("<Chart Values='1 two 3'/>");
	content("<Chart><Slice Value='-3'/></Chart>");
	content("<Chart><Wedge/></Chart>");
	EXPECT_ERRORS_END(5);
	/* the other names of the kinds */
	CHECK(Rchart_get_kind(content("<Chart Kind='doughnut'/>")) == R_CHART_DONUT);
	CHECK(Rchart_get_kind(content("<Chart Kind='HeatMap'/>")) == R_CHART_HEATMAP);
}

/* A chart that glides and sweeps in, painted while it moves, then dropped in the middle of it. */
static void test_animation(void) {
	Rid root, chart;
	int i;
	Rset_animations_enabled(R_TRUE);
	root = content("<VBox><Chart Name='c' Kind='Donut' Width='300' Height='200'><Slice Value='1'/><Slice Value='2'/></Chart>"
		"<Chart Kind='Sparkline' Values='1 2 3' Height='30'/><Chart Kind='Gauge' Value='40' Height='120'/></VBox>");
	chart = Rwidget_find(root, "c");
	for (i = 0; i < 3; i++) {
		Rchart_set_slice_value(chart, 0, 5.0 + i);
		Rchart_add_slice(chart, "more", 1.0);
		Rdraw_frame();
	}
	Rchart_remove_slice(chart, 0);
	Rdraw_frame();
	CHECK(Rchart_get_slice_count(chart) == 4);
	Rset_animations_enabled(R_FALSE);
	content("<VBox/>");
}

int main(void) {
	if (!test_init(0)) return 1;
	g_window = Rwindow_create("Chart", 640, 480, R_WINDOW_DEFAULT);
	RUN(test_create);
	RUN(test_slices);
	RUN(test_pie_pointer);
	RUN(test_gauge);
	RUN(test_heatmap);
	RUN(test_sparkline);
	RUN(test_xml);
	RUN(test_animation);
	Rwindow_destroy(g_window);
	Rshutdown();
	return test_report("test_chart");
}
