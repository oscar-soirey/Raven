/**
 * Raven tests - data for the other applications: the formats of the
 * clipboard (Rclipboard_set_data, images) and the drags out
 * of the windows (Rdrag_begin_external), against the clipboard the null
 * platform keeps in the process and the drags it only holds.
 */

#include "test.h"
#include <stdlib.h>

static Rid window_from(const char* xml) {
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	CHECK(Rwindow_is_valid(window));
	return window;
}

static void center_of(Rid widget, float* x, float* y) {
	float rx, ry, rw, rh;
	Rwidget_get_rect(widget, &rx, &ry, &rw, &rh);
	*x = rx + rw * 0.5f;
	*y = ry + rh * 0.5f;
}

/* ---------------------------------------------------------------------------
 *  Clipboard
 * ------------------------------------------------------------------------- */

static void test_clipboard_text(void) {
	char* data;
	size_t size = 99;
	Rclipboard_set_text("plain words");
	CHECK_STR(Rclipboard_get_text(), "plain words");
	CHECK(Rclipboard_has_data("text/plain"));
	CHECK(Rclipboard_has_data("text/plain;charset=utf-8"));
	CHECK(!Rclipboard_has_data("text/html"));
	data = Rclipboard_get_data("TEXT/PLAIN", &size);
	CHECK(data && size == 11 && memcmp(data, "plain words", 11) == 0 && data[11] == '\0');
	Rmemory_free(data);
	CHECK(Rclipboard_get_data("text/html", &size) == NULL && size == 0);

	Rclipboard_set_html("bold", "<b>bold</b>");
	CHECK_STR(Rclipboard_get_text(), "bold");
	data = Rclipboard_get_data("text/html", &size);
	CHECK(data && size == 11);
	if (data) CHECK_STR(data, "<b>bold</b>");
	Rmemory_free(data);
}

static void test_clipboard_custom(void) {
	static const uint8_t node[6] = { 1, 0, 2, 0, 255, 7 };
	RDataItem items[3];
	char* data;
	size_t size = 0;

	items[0].mime = "application/x-raven-test-node";
	items[0].data = node;
	items[0].size = sizeof(node);
	items[1].mime = "text/plain";
	items[1].data = "Node 7 and more";
	items[1].size = 6;
	items[2].mime = "application/x-empty";
	items[2].data = NULL;
	items[2].size = 0;
	CHECK(Rclipboard_set_data(items, 3));
	CHECK(Rclipboard_has_data("application/x-raven-test-node"));
	CHECK(Rclipboard_has_data("application/x-empty"));
	CHECK(!Rclipboard_has_data("application/x-other"));
	/* the text is the given bytes only */
	CHECK_STR(Rclipboard_get_text(), "Node 7");
	data = Rclipboard_get_data("application/x-raven-test-node", &size);
	CHECK(data && size == sizeof(node) && memcmp(data, node, sizeof(node)) == 0);
	Rmemory_free(data);
	data = Rclipboard_get_data("application/x-empty", &size);
	CHECK(data && size == 0);
	Rmemory_free(data);
	CHECK(!Rclipboard_has_image());

	/* (the items were copied: changing them later changes nothing) */
	items[1].data = "Other";
	CHECK_STR(Rclipboard_get_text(), "Node 7");

	/* items without a format, or without bytes, are refused */
	EXPECT_ERRORS_BEGIN();
	items[0].mime = NULL;
	CHECK(!Rclipboard_set_data(items, 1));
	items[0].mime = "text/plain";
	items[0].data = NULL;
	items[0].size = 3;
	CHECK(!Rclipboard_set_data(items, 1));
	CHECK(!Rclipboard_set_data(NULL, 2));
	CHECK(!Rclipboard_set_data(items, -1));
	EXPECT_ERRORS_END(4);
	CHECK_STR(Rclipboard_get_text(), "Node 7");
}

/* A picture with every kind of pixel: gradients, transparency, noise. */
static uint8_t* make_picture(int width, int height, int stride) {
	uint8_t* pixels = (uint8_t*)calloc((size_t)stride * (size_t)height, 1);
	unsigned seed = 12345u;
	int x, y;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			uint8_t* p = pixels + (size_t)y * (size_t)stride + (size_t)x * 4;
			seed = seed * 1103515245u + 12345u;
			p[0] = (uint8_t)(x * 255 / (width - 1));
			p[1] = (uint8_t)(y * 255 / (height - 1));
			p[2] = (uint8_t)(x < width / 2 ? (seed >> 16) : 128);
			p[3] = (uint8_t)(y < height / 3 ? 255 : (x * 7 + y * 3) & 255);
		}
	}
	return pixels;
}

static int same_pixels(const uint8_t* a, int stride, const uint8_t* packed, int width, int height) {
	int y;
	for (y = 0; y < height; y++) {
		if (memcmp(a + (size_t)y * (size_t)stride, packed + (size_t)y * (size_t)width * 4u, (size_t)width * 4u) != 0) return 0;
	}
	return 1;
}

static void test_clipboard_image(void) {
	const int width = 40, height = 24, stride = 40 * 4 + 4;
	uint8_t* pixels = make_picture(width, height, stride);
	char* png;
	size_t size = 0;
	CHECK(Rclipboard_set_image(pixels, width, height, stride));
	CHECK(Rclipboard_has_image());
	CHECK(Rclipboard_has_data("image/png"));
	CHECK(!Rclipboard_has_data("text/plain"));
	png = Rclipboard_get_data("image/png", &size);
	CHECK(png && size > 8 && memcmp(png, "\x89PNG", 4) == 0);
	Rmemory_free(png);
#ifndef RAVEN_TEST_NO_IMAGE_DECODER
	{
		int w = 0, h = 0;
		uint8_t* back = Rclipboard_get_image(&w, &h);
		CHECK(back && w == width && h == height);
		if (back && w == width && h == height) CHECK(same_pixels(pixels, stride, back, width, height));
		Rpixels_free(back);
	}
#endif
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rclipboard_set_image(pixels, width, height, width * 2));
	CHECK(!Rclipboard_set_image(NULL, width, height, 0));
	EXPECT_ERRORS_END(2);
	CHECK(Rclipboard_has_image());
	Rclipboard_set_text("no picture");
	CHECK(!Rclipboard_has_image());
	{
		int w = 5, h = 5;
		CHECK(Rclipboard_get_image(&w, &h) == NULL && w == 0 && h == 0);
	}
	free(pixels);
}

/* ---------------------------------------------------------------------------
 *  Dragging out of the window
 * ------------------------------------------------------------------------- */

static int g_finished, g_operation, g_out_calls, g_out_index, g_begin_result;
static Rid g_finished_source;
static int g_enter, g_leave, g_dropped;
static char g_drop_type[64], g_drop_text[64];

static void on_finished(Rid source, RDragOperation operation, void* user) {
	(void)user;
	g_finished++;
	g_operation = (int)operation;
	g_finished_source = source;
	/* (no drag any more while it is told) */
	CHECK(!Rdrag_external_is_active());
}

static void drag_out(Rid tiles, int index, void* user) {
	static const uint8_t picture[4 * 4] = { 255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 128 };
	RDataItem items[2];
	(void)user;
	g_out_calls++;
	g_out_index = index;
	items[0].mime = "text/uri-list";
	items[0].data = "file:///tmp/tile.png\r\n";
	items[0].size = strlen("file:///tmp/tile.png\r\n");
	items[1].mime = "text/plain";
	items[1].data = "tile";
	items[1].size = 4;
	g_begin_result = Rdrag_begin_external(tiles, items, 2, R_DRAG_OPERATION_COPY, on_finished, NULL);
	Rdrag_set_image(picture, 2, 2);
}

static int on_drop(Rid target, const RDropEvent* event, void* user) {
	(void)target;
	(void)user;
	if (event->phase == R_DROP_ENTER) g_enter++;
	if (event->phase == R_DROP_LEAVE) g_leave++;
	if (event->phase == R_DROP_DROPPED) g_dropped++;
	snprintf(g_drop_type, sizeof(g_drop_type), "%s", event->drag_type ? event->drag_type : "");
	snprintf(g_drop_text, sizeof(g_drop_text), "%s", event->text ? event->text : "");
	return R_TRUE;
}

static void tile_item(Rid tiles, int index, RTileItem* item, void* user) {
	static const char* names[] = { "Red", "Green", "Blue", "Gold" };
	(void)tiles;
	(void)user;
	item->text = names[index & 3];
}

static void reset_counts(void) {
	g_finished = g_out_calls = g_enter = g_leave = g_dropped = 0;
	g_operation = -1;
	g_out_index = -1;
	g_begin_result = -1;
	g_finished_source = R_INVALID_ID;
	g_drop_type[0] = g_drop_text[0] = '\0';
}

static void test_drag_out_api(void) {
	Rid window = window_from("<Window Width=\"300\" Height=\"200\"><Button Name=\"source\" Text=\"Source\"/></Window>");
	Rid source = Rwidget_find(window, "source");
	RDataItem item;
	float sx, sy;
	item.mime = "text/plain";
	item.data = "hello";
	item.size = 5;
	reset_counts();
	Rdraw_frame();
	center_of(source, &sx, &sy);
	CHECK(Rdrag_external_supported());
	CHECK(!Rdrag_external_is_active());

	/* the button must be held */
	CHECK(!Rdrag_begin_external(source, &item, 1, 0, on_finished, NULL));
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rdrag_begin_external(source, NULL, 1, 0, on_finished, NULL));
	CHECK(!Rdrag_begin_external(R_INVALID_ID, &item, 1, 0, on_finished, NULL));
	EXPECT_ERRORS_END(2);

	/* held, moved: the window system takes it; it tells the end */
	Rwindow_inject_mouse_move(window, sx, sy);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	CHECK(Rdrag_begin_external(source, &item, 1, R_DRAG_OPERATION_COPY | R_DRAG_OPERATION_MOVE, on_finished, NULL));
	CHECK(Rdrag_external_is_active());
	CHECK(!Rwindow_inject_drag_end(window, R_DRAG_OPERATION_COPY));
	Rwindow_inject_mouse_move(window, sx + 3.0f, sy);
	CHECK(Rdrag_external_is_active());
	CHECK(g_finished == 0);
	/* (another one cannot start meanwhile) */
	CHECK(!Rdrag_begin_external(source, &item, 1, 0, on_finished, NULL));
	CHECK(Rwindow_inject_drag_end(window, R_DRAG_OPERATION_MOVE));
	CHECK(g_finished == 1 && g_operation == R_DRAG_OPERATION_MOVE && g_finished_source == source);
	CHECK(!Rdrag_external_is_active());
	CHECK(!Rwindow_inject_drag_end(window, R_DRAG_OPERATION_COPY));
	CHECK(g_finished == 1);
	/* the release went to the window system: the window holds no button any more */
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);

	/* released before moving: it never started */
	reset_counts();
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	CHECK(Rdrag_begin_external(source, &item, 1, 0, on_finished, NULL));
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	CHECK(g_finished == 1 && g_operation == R_DRAG_OPERATION_NONE);
	CHECK(!Rdrag_external_is_active());

	/* a second one replaces the one still waiting */
	reset_counts();
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	CHECK(Rdrag_begin_external(source, &item, 1, 0, on_finished, NULL));
	CHECK(Rdrag_begin_external(source, &item, 1, 0, NULL, NULL));
	CHECK(g_finished == 1 && g_operation == R_DRAG_OPERATION_NONE);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	CHECK(!Rdrag_external_is_active());

	/* the window closed while the window system holds the drag: told at once */
	reset_counts();
	Rwindow_inject_mouse_move(window, sx, sy);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	CHECK(Rdrag_begin_external(source, &item, 1, 0, on_finished, NULL));
	Rwindow_inject_mouse_move(window, sx + 8.0f, sy);
	CHECK(Rdrag_external_is_active());
	Rwindow_destroy(window);
	CHECK(g_finished == 1 && g_operation == R_DRAG_OPERATION_NONE);
	CHECK(!Rdrag_external_is_active());
}

static void test_drag_out_tiles(void) {
	Rid window = window_from(
		"<Window Width=\"400\" Height=\"300\"><HBox>"
		"<TileView Name=\"tiles\" TileWidth=\"100\" TileHeight=\"80\" Spacing=\"10\" Width=\"240\"/>"
		"<Border Name=\"target\" Slot.Fill=\"1\"><Label>Drop here</Label></Border>"
		"</HBox></Window>");
	Rid tiles = Rwidget_find(window, "tiles");
	Rid target = Rwidget_find(window, "target");
	float x, y, w, h, tx, ty;
	Rtile_view_set_items(tiles, 4, tile_item, NULL);
	Rtile_view_set_drag_out_callback(tiles, drag_out, NULL);
	Rwidget_set_drop_callback(target, on_drop, NULL);
	Rdraw_frame();
	Rwidget_get_rect(tiles, &x, &y, &w, &h);
	center_of(target, &tx, &ty);

	/* without a drag type: out of the window at once */
	reset_counts();
	Rwindow_inject_mouse_move(window, x + 130.0f, y + 20.0f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(window, x + 140.0f, y + 30.0f);
	CHECK(g_out_calls == 1 && g_out_index == 1 && g_begin_result == R_TRUE);
	CHECK(Rdrag_external_is_active());
	CHECK(!Rdrag_is_active(window));
	Rwindow_inject_mouse_move(window, x + 150.0f, y + 40.0f);
	CHECK(Rwindow_inject_drag_end(window, R_DRAG_OPERATION_COPY));
	CHECK(g_finished == 1 && g_operation == R_DRAG_OPERATION_COPY && g_finished_source == tiles);
	CHECK(g_enter == 0);

	/* with a drag type: inside the window first, to its drop targets */
	reset_counts();
	Rtile_view_set_drag_type(tiles, "swatch");
	Rwindow_inject_mouse_move(window, x + 20.0f, y + 20.0f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(window, x + 30.0f, y + 30.0f);
	CHECK(g_out_calls == 1 && g_out_index == 0 && g_begin_result == R_TRUE);
	CHECK(Rdrag_is_active(window));
	CHECK(Rdrag_external_is_active());
	Rwindow_inject_mouse_move(window, tx, ty);
	CHECK(g_enter == 1);
	CHECK_STR(g_drop_type, "swatch");
	CHECK_STR(g_drop_text, "0");
	/* ... out of it with the pointer */
	Rwindow_inject_mouse_move(window, 410.0f, ty);
	CHECK(g_leave == 1);
	CHECK(!Rdrag_is_active(window));
	CHECK(Rdrag_external_is_active());
	CHECK(g_finished == 0);
	/* ... back over the window: still the drag of the application */
	CHECK(Rwindow_inject_drag(window, tx, ty, R_TRUE));
	CHECK(g_enter == 2);
	CHECK_STR(g_drop_type, "swatch");
	CHECK(Rwindow_inject_drop(window, tx, ty, NULL, 0, "tile"));
	CHECK(g_dropped == 1);
	CHECK_STR(g_drop_type, "swatch");
	CHECK_STR(g_drop_text, "0");
	CHECK(Rwindow_inject_drag_end(window, R_DRAG_OPERATION_COPY));
	CHECK(g_finished == 1 && g_operation == R_DRAG_OPERATION_COPY);
	/* a drag from elsewhere is not mistaken for it any more */
	CHECK(Rwindow_inject_drop(window, tx, ty, NULL, 0, "elsewhere"));
	CHECK_STR(g_drop_type, "");
	CHECK_STR(g_drop_text, "elsewhere");

	/* dropped inside: it never left (pressed elsewhere: not a double click) */
	reset_counts();
	Rwindow_inject_mouse_move(window, x + 70.0f, y + 30.0f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(window, x + 80.0f, y + 40.0f);
	Rwindow_inject_mouse_move(window, tx, ty);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	CHECK(g_dropped == 1);
	CHECK(g_finished == 1 && g_operation == R_DRAG_OPERATION_NONE);
	CHECK(!Rdrag_external_is_active());

	/* Escape cancels both, even when the pointer then leaves */
	reset_counts();
	Rwindow_inject_mouse_move(window, x + 40.0f, y + 70.0f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(window, x + 50.0f, y + 80.0f);
	test_key(window, R_KEY_ESCAPE, 0);
	CHECK(!Rdrag_is_active(window));
	CHECK(g_finished == 1 && g_operation == R_DRAG_OPERATION_NONE);
	Rwindow_inject_mouse_move(window, -20.0f, ty);
	CHECK(!Rdrag_external_is_active());
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	CHECK(g_finished == 1);
	Rwindow_destroy(window);
}

int main(void) {
	if (!test_init(0)) {
		printf("test_transfer: cannot initialize Raven\n");
		return 1;
	}
	RUN(test_clipboard_text);
	RUN(test_clipboard_custom);
	RUN(test_clipboard_image);
	RUN(test_drag_out_api);
	RUN(test_drag_out_tiles);
	Rshutdown();
	return test_report("test_transfer");
}
