/**
 * Raven tests - layout: the measure / arrange passes of every panel.
 * Sizes are checked through Rwidget_get_rect, in a 400 x 300 window.
 */

#include "test.h"

static Rid g_window;

/* A fixed-size block standing for any content. */
static Rid block(float width, float height) {
	Rid spacer = Rwidget_create(R_WIDGET_SPACER);
	Rwidget_set_size(spacer, width, height);
	return spacer;
}

static void test_vbox(void) {
	Rid box = Rwidget_create(R_WIDGET_VBOX);
	Rid a = Rwidget_add_child(box, block(50, 20));
	Rid b = Rwidget_add_child(box, block(80, 30));
	Rid c = Rwidget_add_child(box, block(10, 40));
	float w, h;
	Rwindow_set_content(g_window, box);

	/* auto slots stack with their desired height and fill the width */
	CHECK_RECT(a, 0, 0, 400, 20);
	CHECK_RECT(b, 0, 20, 400, 30);
	CHECK_RECT(c, 0, 50, 400, 40);
	Rwidget_get_desired_size(box, &w, &h);
	CHECK_NEAR(w, 80);
	CHECK_NEAR(h, 90);

	/* a fill slot takes what is left */
	Rslot_set_fill(b, 1.0f);
	CHECK_RECT(b, 0, 20, 400, 240);
	CHECK_RECT(c, 0, 260, 400, 40);

	/* two fill slots share in proportion to their coefficients */
	Rslot_set_fill(c, 3.0f);
	CHECK_RECT(b, 0, 20, 400, 70);
	CHECK_RECT(c, 0, 90, 400, 210);

	/* alignment across the axis, padding and spacing */
	Rslot_set_auto(b);
	Rslot_set_auto(c);
	Rslot_set_halign(a, R_HALIGN_CENTER);
	Rslot_set_halign(b, R_HALIGN_RIGHT);
	Rslot_set_padding(c, 5, 6, 7, 8);
	Rpanel_set_spacing(box, 10);
	CHECK_RECT(a, 175, 0, 50, 20);
	CHECK_RECT(b, 320, 30, 80, 30);
	CHECK_RECT(c, 5, 76, 388, 40);

	/* a collapsed child takes no space, a hidden one keeps it */
	Rwidget_set_visibility(a, R_COLLAPSED);
	CHECK_RECT(b, 320, 0, 80, 30);
	Rwidget_set_visibility(a, R_HIDDEN);
	CHECK_RECT(b, 320, 30, 80, 30);

	/* the slot maximum caps a fill slot */
	Rwidget_set_visibility(a, R_VISIBLE);
	Rslot_set_fill(b, 1.0f);
	Rslot_set_max_size(b, 100.0f);
	Rslot_set_halign(b, R_HALIGN_FILL);
	CHECK_RECT(b, 0, 30, 400, 100);
}

static void test_hbox(void) {
	Rid box = Rwidget_create(R_WIDGET_HBOX);
	Rid a = Rwidget_add_child(box, block(50, 20));
	Rid b = Rwidget_add_child(box, block(80, 30));
	Rwindow_set_content(g_window, box);
	Rslot_set_fill(b, 1.0f);
	Rslot_set_valign(a, R_VALIGN_BOTTOM);
	CHECK_RECT(a, 0, 280, 50, 20);
	CHECK_RECT(b, 50, 0, 350, 300);
	Rslot_set_valign(a, R_VALIGN_CENTER);
	CHECK_RECT(a, 0, 140, 50, 20);
}

static void test_overlay_and_border(void) {
	Rid overlay = Rwidget_create(R_WIDGET_OVERLAY);
	Rid back = Rwidget_add_child(overlay, block(10, 10));
	Rid border = Rwidget_add_child(overlay, Rwidget_create(R_WIDGET_BORDER));
	Rid inside = Rwidget_add_child(border, block(60, 40));
	Rid replaced;
	Rwindow_set_content(g_window, overlay);

	Rslot_set_halign(border, R_HALIGN_RIGHT);
	Rslot_set_valign(border, R_VALIGN_BOTTOM);
	Rslot_set_padding(border, 0, 0, 10, 10);
	Rborder_set_padding(border, 4, 4, 4, 4);
	CHECK_RECT(back, 0, 0, 400, 300);
	CHECK_RECT(border, 322, 242, 68, 48);
	CHECK_RECT(inside, 326, 246, 60, 40);

	/* a single-child panel replaces its child */
	replaced = Rwidget_add_child(border, block(20, 20));
	CHECK(!Rwidget_is_valid(inside));
	CHECK(Rwidget_get_child_count(border) == 1);
	CHECK_RECT(replaced, 366, 266, 20, 20);
}

static void test_grid(void) {
	Rid grid = Rwidget_create(R_WIDGET_GRID);
	Rid a = Rwidget_add_child(grid, block(40, 20));
	Rid b = Rwidget_add_child(grid, block(60, 30));
	Rid c = Rwidget_add_child(grid, block(100, 10));
	Rid d = Rwidget_add_child(grid, block(20, 50));
	float w, h;
	Rwindow_set_content(g_window, grid);
	Rslot_set_grid(a, 0, 0);
	Rslot_set_grid(b, 1, 0);
	Rslot_set_grid(c, 0, 1);
	Rslot_set_grid(d, 1, 1);

	/* columns: 100, 60 - rows: 30, 50 */
	Rwidget_get_desired_size(grid, &w, &h);
	CHECK_NEAR(w, 160);
	CHECK_NEAR(h, 80);
	CHECK_RECT(a, 0, 0, 100, 30);
	CHECK_RECT(b, 100, 0, 60, 30);
	CHECK_RECT(d, 100, 30, 60, 50);

	/* a fill column takes the free width, a fill row the free height */
	Rgrid_set_column_fill(grid, 1, 1.0f);
	Rgrid_set_row_fill(grid, 0, 1.0f);
	CHECK_RECT(b, 100, 0, 300, 250);
	CHECK_RECT(c, 0, 250, 100, 50);

	/* a spanning cell covers both columns */
	Rslot_set_grid_span(c, 2, 1);
	Rwidget_set_visibility(d, R_COLLAPSED);
	CHECK_RECT(c, 0, 290, 400, 10);

	/* the first column is now only as wide as `a`; gaps of 8 between tracks */
	Rpanel_set_spacing(grid, 8);
	CHECK_RECT(b, 48, 0, 352, 282);

	/* a rejected Slot.Column / Row / span leaves the slot as it is (issue #127) */
	CHECK(!Rwidget_set_attribute(b, "Slot.Column", "-3"));
	CHECK(!Rwidget_set_attribute(b, "Slot.Row", "-1"));
	CHECK(!Rwidget_set_attribute(c, "Slot.ColumnSpan", "0"));
	CHECK(!Rwidget_set_attribute(b, "Slot.Column", "1e30")); /* not an int */
	CHECK_RECT(b, 48, 0, 352, 282);
	CHECK_RECT(c, 0, 290, 400, 10);
	/* an accepted one is clamped as by Rslot_set_grid_span */
	CHECK(Rwidget_set_attribute(c, "Slot.ColumnSpan", "100000"));
	Rwidget_get_rect(c, NULL, NULL, NULL, NULL);
	CHECK(Rwidget_set_attribute(c, "Slot.ColumnSpan", "2"));
	CHECK_RECT(b, 48, 0, 352, 282);
}

static void test_uniform_grid(void) {
	Rid grid = Rwidget_create(R_WIDGET_UNIFORM_GRID);
	Rid cells[5];
	int i;
	for (i = 0; i < 5; i++) cells[i] = Rwidget_add_child(grid, block(10, 10));
	Rwindow_set_content(g_window, grid);
	Runiform_grid_set_columns(grid, 2);
	/* 2 columns x 3 rows sharing 400 x 300 */
	CHECK_RECT(cells[0], 0, 0, 200, 100);
	CHECK_RECT(cells[1], 200, 0, 200, 100);
	CHECK_RECT(cells[4], 0, 200, 200, 100);
}

static void test_size_box_and_canvas(void) {
	Rid box = Rwidget_create(R_WIDGET_SIZE_BOX);
	Rid content = Rwidget_add_child(box, block(10, 10));
	Rid canvas, pinned, stretched;
	Rwindow_set_content(g_window, box);

	/* 2:1 inside 400 x 300 is 400 x 200, centered */
	Rsize_box_set_aspect_ratio(box, 2.0f);
	CHECK_RECT(content, 0, 50, 400, 200);
	Rsize_box_set_aspect_ratio(box, 0.5f);
	CHECK_RECT(content, 125, 0, 150, 300);

	canvas = Rwidget_create(R_WIDGET_CANVAS);
	pinned = Rwidget_add_child(canvas, block(40, 20));
	stretched = Rwidget_add_child(canvas, block(1, 1));
	Rwindow_set_content(g_window, canvas);

	/* pinned to the bottom-right corner, pivot on its own bottom-right corner */
	Rslot_set_anchors(pinned, 1, 1, 1, 1);
	Rslot_set_alignment(pinned, 1, 1);
	Rslot_set_position(pinned, -10, -10);
	CHECK_RECT(pinned, 350, 270, 40, 20);

	/* stretched over the whole canvas with a 20 unit margin */
	Rslot_set_anchors(stretched, 0, 0, 1, 1);
	Rslot_set_position(stretched, 20, 20);
	Rslot_set_size(stretched, 20, 20);
	CHECK_RECT(stretched, 20, 20, 360, 260);
}

static void test_switcher_and_constraints(void) {
	Rid switcher = Rwidget_create(R_WIDGET_SWITCHER);
	Rid first = Rwidget_add_child(switcher, block(10, 10));
	Rid second = Rwidget_add_child(switcher, block(10, 10));
	Rid box, limited;
	float w, h;
	Rwindow_set_content(g_window, switcher);
	CHECK_RECT(first, 0, 0, 400, 300);
	Rswitcher_set_index(switcher, 1);
	CHECK(Rswitcher_get_index(switcher) == 1);
	CHECK_RECT(second, 0, 0, 400, 300);

	box = Rwidget_create(R_WIDGET_VBOX);
	limited = Rwidget_add_child(box, block(500, 5));
	Rwindow_set_content(g_window, box);
	Rslot_set_halign(limited, R_HALIGN_LEFT);
	Rwidget_set_max_size(limited, 120, -1);
	Rwidget_set_min_size(limited, -1, 33);
	Rwidget_get_desired_size(limited, &w, &h);
	CHECK_NEAR(w, 120);
	CHECK_NEAR(h, 33);
	CHECK_RECT(limited, 0, 0, 120, 33);
}

static void test_scroll_box(void) {
	Rid scroll = Rwidget_create(R_WIDGET_SCROLL_BOX);
	Rid rows[10];
	float x, y, bar = Rtheme_get_metric(R_METRIC_SCROLLBAR_SIZE);
	int i;
	for (i = 0; i < 10; i++) rows[i] = Rwidget_add_child(scroll, block(50, 100));
	Rwindow_set_content(g_window, scroll);

	/* 1000 units of content in a 300 unit view: the scrollbar takes its width */
	CHECK_RECT(rows[0], 0, 0, 400 - bar, 100);
	Rscroll_box_set_offset(scroll, 0, 250);
	CHECK_RECT(rows[0], 0, -250, 400 - bar, 100);
	CHECK_RECT(rows[3], 0, 50, 400 - bar, 100);

	/* offsets are clamped to the content */
	Rscroll_box_set_offset(scroll, 0, 5000);
	Rscroll_box_get_offset(scroll, &x, &y);
	CHECK_NEAR(y, 700);
	Rscroll_box_scroll_to_end(scroll);
	CHECK_RECT(rows[9], 0, 200, 400 - bar, 100);

	Rscroll_box_set_offset(scroll, 0, 0);
	Rwidget_scroll_into_view(rows[5]);
	Rscroll_box_get_offset(scroll, &x, &y);
	CHECK_NEAR(y, 300);

	/* the mouse wheel scrolls, and widgets scrolled out of view cannot be hit */
	Rscroll_box_set_offset(scroll, 0, 0);
	Rwindow_inject_mouse_move(g_window, 100, 100);
	Rwindow_inject_mouse_wheel(g_window, 0, -1);
	Rscroll_box_get_offset(scroll, &x, &y);
	CHECK(y > 0);

	/* a box that also scrolls across brings a widget into view along both axes */
	{
		Rid grid = Rwidget_create(R_WIDGET_SCROLL_BOX);
		Rid target = R_INVALID_ID;
		float w, h;
		int row, column;
		Rscroll_box_set_scroll_both(grid, R_TRUE);
		for (row = 0; row < 5; row++) {
			Rid line = Rwidget_add_child(grid, Rwidget_create(R_WIDGET_HBOX));
			for (column = 0; column < 10; column++) {
				Rid cell = Rwidget_add_child(line, block(100, 100));
				if (row == 3 && column == 7) target = cell;
			}
		}
		Rwindow_set_content(g_window, grid);
		Rwidget_scroll_into_view(target);
		Rscroll_box_get_offset(grid, &x, &y);
		CHECK(x > 0 && y > 0);
		Rwidget_get_rect(target, &x, &y, &w, &h);
		CHECK(x >= 0 && x + w <= 400 - bar);
		CHECK(y >= 0 && y + h <= 300 - bar);
	}
}

static void test_splitter_and_wrap(void) {
	Rid splitter = Rwidget_create(R_WIDGET_SPLITTER);
	Rid left = Rwidget_add_child(splitter, block(10, 10));
	Rid right = Rwidget_add_child(splitter, block(10, 10));
	float handle = Rtheme_get_metric(R_METRIC_SPLITTER_SIZE);
	float x, y, w, h;
	Rid wrap, items[5];
	int i;
	Rwindow_set_content(g_window, splitter);

	Rslot_set_fill(left, 1.0f);
	Rslot_set_fill(right, 3.0f);
	Rwidget_get_rect(left, &x, &y, &w, &h);
	CHECK_NEAR(w, (400 - handle) * 0.25f);
	CHECK_NEAR(h, 300);

	/* dragging the handle 50 units to the right grows the left pane by 50 */
	Rwindow_inject_mouse_move(g_window, w + handle * 0.5f, 150);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(g_window, w + handle * 0.5f + 50, 150);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK_RECT(left, 0, 0, w + 50, 300);

	/* right to left: the first pane is on the right, and its handle is on its left (issue #123) */
	Rslot_set_fill(left, 1.0f);
	Rslot_set_fill(right, 3.0f);
	Rwidget_set_direction(splitter, R_DIRECTION_RTL);
	CHECK_RECT(left, 400 - w, 0, w, 300);
	CHECK_RECT(right, 0, 0, 400 - handle - w, 300);
	/* dragging that handle 50 units to the left grows the first pane by 50 */
	Rwindow_inject_mouse_move(g_window, 400 - w - handle * 0.5f, 150);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(g_window, 400 - w - handle * 0.5f - 50, 150);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK_RECT(left, 400 - w - 50, 0, w + 50, 300);
	CHECK_RECT(right, 0, 0, 400 - handle - w - 50, 300);
	Rwidget_set_direction(splitter, R_DIRECTION_INHERIT);

	wrap = Rwidget_create(R_WIDGET_WRAP_BOX);
	for (i = 0; i < 5; i++) items[i] = Rwidget_add_child(wrap, block(150, 40));
	Rwindow_set_content(g_window, wrap);
	/* two blocks of 150 fit in 400, the third wraps */
	CHECK_RECT(items[1], 150, 0, 150, 40);
	CHECK_RECT(items[2], 0, 40, 150, 40);
	CHECK_RECT(items[4], 0, 80, 150, 40);
	Rwidget_get_desired_size(wrap, &w, &h);
	CHECK_NEAR(h, 120);
}

static void test_window_resize_and_text(void) {
	Rid box = Rwidget_create(R_WIDGET_VBOX);
	Rid label = Rwidget_add_child(box, Rlabel_create("The quick brown fox jumps over the lazy dog, again and again and again."));
	Rid filler = Rwidget_add_child(box, block(10, 10));
	float single_w, single_h, w, h, text_w, text_h;
	Rwindow_set_content(g_window, box);
	Rslot_set_fill(filler, 1.0f);

	Rwidget_get_desired_size(label, &single_w, &single_h);
	CHECK(single_w > 300 && single_h > 8);
	Rfont_measure_text(R_INVALID_ID, 0, "The quick brown fox", &text_w, &text_h);
	CHECK(text_w > 80 && text_w < single_w);
	/* the label rounds its desired size up to whole units */
	CHECK(text_h <= single_h && text_h > single_h - 1.0f);

	/* a wrapped label gets taller when the window gets narrower */
	Rlabel_set_wrap(label, R_TRUE);
	Rwindow_set_size(g_window, 150, 300);
	Rwidget_get_rect(label, NULL, NULL, &w, &h);
	CHECK_NEAR(w, 150);
	CHECK(h >= single_h * 3 - 1);
	CHECK_RECT(filler, 0, h, 150, 300 - h);

	Rwindow_set_size(g_window, 400, 300);
	Rwidget_get_rect(label, NULL, NULL, &w, &h);
	CHECK_NEAR(w, 400);
	CHECK(h < single_h * 3);
}

static void test_tree_api(void) {
	Rid box = Rwidget_create(R_WIDGET_VBOX);
	Rid a = Rwidget_add_child(box, block(10, 10));
	Rid b = Rwidget_add_child(box, block(10, 10));
	Rid other = Rwidget_create(R_WIDGET_HBOX);
	Rwindow_set_content(g_window, box);

	Rwidget_set_name(b, "second");
	CHECK(Rwidget_find(g_window, "second") == b);
	CHECK(Rwidget_find(box, "missing") == R_INVALID_ID);
	CHECK(Rwidget_get_parent(a) == box);
	CHECK(Rwidget_get_child(box, 1) == b);
	CHECK(Rwidget_get_window(a) == g_window);
	CHECK(Rwidget_get_type(box) == R_WIDGET_VBOX);

	/* moving a widget to another parent */
	Rwidget_insert_child(other, a, 0);
	CHECK(Rwidget_get_child_count(box) == 1);
	CHECK(Rwidget_get_parent(a) == other);
	CHECK(Rwidget_get_window(a) == R_INVALID_ID);

	/* deleting a parent deletes its children; stale handles are detected */
	Rwidget_destroy(other);
	CHECK(!Rwidget_is_valid(a));
	EXPECT_ERRORS_BEGIN();
	Rwidget_set_name(a, "ghost");
	CHECK(Rwidget_add_child(a, b) == R_INVALID_ID);
	CHECK(Rwidget_add_child(box, box) == R_INVALID_ID);
	CHECK(Rwidget_add_child(b, box) == R_INVALID_ID); /* a spacer has no children */
	Rgrid_set_column_fill(box, 0, 1.0f);             /* wrong widget type */
	EXPECT_ERRORS_END(5);
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	g_window = Rwindow_create("layout", 400, 300, R_WINDOW_DEFAULT);
	CHECK(Rwindow_is_valid(g_window));

	RUN(test_vbox);
	RUN(test_hbox);
	RUN(test_overlay_and_border);
	RUN(test_grid);
	RUN(test_uniform_grid);
	RUN(test_size_box_and_canvas);
	RUN(test_switcher_and_constraints);
	RUN(test_scroll_box);
	RUN(test_splitter_and_wrap);
	RUN(test_window_resize_and_text);
	RUN(test_tree_api);

	Rshutdown();
	return test_report("test_layout");
}
