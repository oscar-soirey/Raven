/**
 * Raven tests - the widgets of creative tools (color picker, timeline, node
 * graph), the background blur and the windows that are seen through, and
 * the hooks other libraries build on (tags made by code, destroy callback,
 * image decoding).
 */

#include "test.h"

#include <stdlib.h>

static Rid make_window(const char* xml) {
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	CHECK(Rwindow_is_valid(window));
	Rdraw_frame();
	return window;
}

static void click_at(Rid window, float x, float y) {
	Rwindow_inject_mouse_move(window, x, y);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
}

/* Press, move in two steps (a drag starts after a few units), release. */
static void drag(Rid window, float x0, float y0, float x1, float y1) {
	Rwindow_inject_mouse_move(window, x0, y0);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(window, (x0 + x1) * 0.5f, (y0 + y1) * 0.5f);
	Rwindow_inject_mouse_move(window, x1, y1);
	Rdraw_frame();
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	Rdraw_frame();
}

static void press_key(Rid window, RKey key, Ruint modifiers) {
	Rwindow_inject_key(window, key, R_TRUE, modifiers);
	Rwindow_inject_key(window, key, R_FALSE, modifiers);
}

/* ---------------------------------------------------------------------------
 *  Color picker
 * ------------------------------------------------------------------------- */

static int g_color_calls = 0;
static float g_color[4];
static void on_color(Rid picker, float r, float g, float b, float a, void* user_data) {
	(void)picker;
	(void)user_data;
	g_color_calls++;
	g_color[0] = r;
	g_color[1] = g;
	g_color[2] = b;
	g_color[3] = a;
}

static void test_color_picker(void) {
	Rid window, picker;
	float r, g, b, a, h, s, v, x, y, w, hh;

	CHECK(test_init(0));
	window = make_window("<Window Width=\"320\" Height=\"460\"><VBox Slot.Padding=\"10\">"
		"<ColorPicker Name=\"p\" Color=\"#FF0000\" Swatches=\"#00FF00, #0000FF80\"/></VBox></Window>");
	picker = Rwidget_find(window, "p");
	CHECK(picker != R_INVALID_ID && Rwidget_get_type(picker) == R_WIDGET_COLOR_PICKER);

	Rcolor_picker_get_color(picker, &r, &g, &b, &a);
	CHECK(r == 1.0f && g == 0.0f && b == 0.0f && a == 1.0f);
	Rcolor_picker_get_hsv(picker, &h, &s, &v);
	CHECK_NEAR(h, 0.0f);
	CHECK(s == 1.0f && v == 1.0f);
	CHECK_STR(Rcolor_picker_get_hex(picker), "#FF0000");
	CHECK(Rcolor_picker_get_swatch_count(picker) == 2);

	/* text */
	CHECK(Rcolor_picker_set_hex(picker, "#00ff0080"));
	Rcolor_picker_get_color(picker, &r, &g, &b, &a);
	CHECK(r == 0.0f && g == 1.0f && b == 0.0f);
	CHECK_NEAR(a * 255.0f, 128.0f);
	CHECK_STR(Rcolor_picker_get_hex(picker), "#00FF0080");
	/* a code without opacity leaves the opacity as it is */
	CHECK(Rcolor_picker_set_hex(picker, "36C"));
	CHECK_STR(Rcolor_picker_get_hex(picker), "#3366CC80");
	CHECK(!Rcolor_picker_set_hex(picker, "not a color"));
	CHECK_STR(Rcolor_picker_get_hex(picker), "#3366CC80");
	CHECK(Rcolor_picker_set_hex(picker, "#3366CCFF"));
	CHECK_STR(Rcolor_picker_get_hex(picker), "#3366CC");

	/* hue, saturation, value; a gray remembers its hue */
	Rcolor_picker_set_hsv(picker, 240.0f, 1.0f, 1.0f);
	Rcolor_picker_get_color(picker, &r, &g, &b, &a);
	CHECK(r == 0.0f && g == 0.0f && b == 1.0f);
	Rcolor_picker_set_hsv(picker, 120.0f, 0.0f, 0.5f);
	Rcolor_picker_get_color(picker, &r, &g, &b, &a);
	CHECK_NEAR(r * 255.0f, 127.5f);
	CHECK(r == g && g == b);
	Rcolor_picker_get_hsv(picker, &h, &s, &v);
	CHECK_NEAR(h, 120.0f);

	/* the conversions, there and back */
	Rcolor_rgb_to_hsv(1.0f, 0.5f, 0.0f, &h, &s, &v);
	CHECK_NEAR(h, 30.0f);
	CHECK(s == 1.0f && v == 1.0f);
	Rcolor_hsv_to_rgb(h, s, v, &r, &g, &b);
	CHECK_NEAR(r * 255.0f, 255.0f);
	CHECK_NEAR(g * 255.0f, 127.5f);
	CHECK_NEAR(b * 255.0f, 0.0f);

	/* without opacity, colors are opaque */
	Rcolor_picker_set_color(picker, 0.2f, 0.4f, 0.6f, 0.5f);
	Rcolor_picker_set_alpha_enabled(picker, R_FALSE);
	Rcolor_picker_get_color(picker, &r, &g, &b, &a);
	CHECK(a == 1.0f);
	CHECK_STR(Rcolor_picker_get_hex(picker), "#336699");
	Rcolor_picker_set_alpha_enabled(picker, R_TRUE);

	/* swatches */
	Rcolor_picker_add_swatch(picker, 1.0f, 1.0f, 1.0f, 1.0f);
	CHECK(Rcolor_picker_get_swatch_count(picker) == 3);
	Rcolor_picker_clear_swatches(picker);
	CHECK(Rcolor_picker_get_swatch_count(picker) == 0);

	/* the mouse in the square: saturation across, value down; the callback tells */
	Rcolor_picker_set_changed_callback(picker, on_color, NULL);
	Rcolor_picker_set_hsv(picker, 200.0f, 1.0f, 1.0f);
	CHECK(g_color_calls == 0);
	Rdraw_frame();
	Rwidget_get_rect(picker, &x, &y, &w, &hh);
	CHECK(w > 100.0f && hh > 200.0f);
	click_at(window, x + w * 0.5f, y + 2.0f);
	CHECK(g_color_calls >= 1);
	Rcolor_picker_get_hsv(picker, &h, &s, &v);
	CHECK_NEAR(h, 200.0f);
	CHECK_NEAR(s * 100.0f, 50.0f);
	CHECK(v > 0.95f);
	Rcolor_picker_get_color(picker, &r, &g, &b, &a);
	CHECK(g_color[0] == r && g_color[1] == g && g_color[2] == b && g_color[3] == a);
	/* dragging out of the square keeps the gesture and clamps */
	drag(window, x + w * 0.5f, y + 20.0f, x - 50.0f, y + 20.0f);
	Rcolor_picker_get_hsv(picker, &h, &s, &v);
	CHECK(s == 0.0f);
	CHECK_NEAR(h, 200.0f);

	/* as a button that opens the picker in a popup */
	Rcolor_picker_set_popup(picker, R_TRUE);
	Rdraw_frame();
	Rwidget_get_rect(picker, &x, &y, &w, &hh);
	CHECK(hh < 60.0f);
	click_at(window, x + w * 0.5f, y + hh * 0.5f);
	Rdraw_frame();
	press_key(window, R_KEY_ESCAPE, 0);
	Rdraw_frame();
	Rcolor_picker_set_popup(picker, R_FALSE);
	Rcolor_picker_set_compact(picker, R_TRUE);
	Rdraw_frame();

	Rshutdown();
}

/* ---------------------------------------------------------------------------
 *  Timeline
 * ------------------------------------------------------------------------- */

static int g_time_calls = 0;
static double g_time_value = 0.0;
static void on_time(Rid timeline, double seconds, void* user_data) {
	(void)timeline;
	(void)user_data;
	g_time_calls++;
	g_time_value = seconds;
}

static int g_item_calls[4];
static int g_item_last[4];
static void on_item(Rid timeline, int item, void* user_data) {
	int which = (int)(intptr_t)user_data;
	(void)timeline;
	g_item_calls[which]++;
	g_item_last[which] = item;
}

static void test_timeline(void) {
	Rid window, tl;
	int track_a, track_b, one, two, key, marker, i;
	double start, end;
	float x, y, w, h, second;

	CHECK(test_init(0));
	window = make_window("<Window Width=\"900\" Height=\"300\">"
		"<Timeline Name=\"t\" Duration=\"10\" FrameRate=\"10\" HeaderWidth=\"100\">"
		"<Track Name=\"A\" Color=\"#FF0000\"><Clip Start=\"1\" Length=\"2\" Label=\"one\"/><Key Time=\"5\" Label=\"k\"/></Track>"
		"<Track Name=\"B\"><Clip Start=\"4\" End=\"6\" Label=\"two\" Selected=\"true\"/></Track>"
		"<Marker Time=\"8\" Label=\"m\"/></Timeline></Window>");
	tl = Rwidget_find(window, "t");
	CHECK(tl != R_INVALID_ID && Rwidget_get_type(tl) == R_WIDGET_TIMELINE);
	Rwidget_get_rect(tl, &x, &y, &w, &h);
	CHECK_RECT(tl, 0.0f, 0.0f, 900.0f, 300.0f);
	/* 100 units of headers, then 10 seconds over 800 units */
	second = 80.0f;

	CHECK(Rtimeline_get_duration(tl) == 10.0);
	CHECK(Rtimeline_get_frame_rate(tl) == 10.0);
	CHECK(Rtimeline_get_track_count(tl) == 2);
	track_a = Rtimeline_get_track(tl, 0);
	track_b = Rtimeline_get_track(tl, 1);
	CHECK(track_a > 0 && track_b > 0 && track_a != track_b);
	CHECK(Rtimeline_get_track(tl, 2) == 0);
	CHECK_STR(Rtimeline_get_track_name(tl, track_a), "A");
	CHECK(Rtimeline_get_item_count(tl) == 4);
	one = two = key = marker = 0;
	for (i = 0; i < Rtimeline_get_item_count(tl); i++) {
		int item = Rtimeline_get_item(tl, i);
		switch (Rtimeline_get_item_type(tl, item)) {
			case R_TIMELINE_CLIP:
				if (strcmp(Rtimeline_get_item_label(tl, item), "one") == 0) one = item;
				else two = item;
				break;
			case R_TIMELINE_KEY: key = item; break;
			case R_TIMELINE_MARKER: marker = item; break;
			default: break;
		}
	}
	CHECK(one && two && key && marker);
	CHECK(Rtimeline_get_item_track(tl, one) == track_a && Rtimeline_get_item_track(tl, two) == track_b);
	CHECK(Rtimeline_get_item_start(tl, one) == 1.0 && Rtimeline_get_item_length(tl, one) == 2.0);
	CHECK(Rtimeline_get_item_start(tl, two) == 4.0 && Rtimeline_get_item_length(tl, two) == 2.0);
	CHECK(Rtimeline_get_item_start(tl, key) == 5.0 && Rtimeline_get_item_length(tl, key) == 0.0);
	CHECK(Rtimeline_get_item_track(tl, marker) == 0 && Rtimeline_get_item_start(tl, marker) == 8.0);
	CHECK(Rtimeline_get_item_type(tl, 9999) == R_TIMELINE_NONE);
	CHECK(Rtimeline_is_selected(tl, two) && !Rtimeline_is_selected(tl, one));
	CHECK(Rtimeline_get_selection_count(tl) == 1 && Rtimeline_get_selected(tl, 0) == two);

	Rtimeline_set_time_callback(tl, on_time, NULL);
	Rtimeline_set_changed_callback(tl, on_item, (void*)(intptr_t)0);
	Rtimeline_set_selection_callback(tl, on_item, (void*)(intptr_t)1);
	Rtimeline_set_removed_callback(tl, on_item, (void*)(intptr_t)2);
	Rtimeline_set_activated_callback(tl, on_item, (void*)(intptr_t)3);
	memset(g_item_calls, 0, sizeof(g_item_calls));

	/* the ruler: the playhead comes under the pointer, on a frame */
	click_at(window, x + 100.0f + second * 3.02f, y + 15.0f);
	CHECK(g_time_calls >= 1);
	CHECK_NEAR(g_time_value * 100.0, 300.0);
	CHECK_NEAR(Rtimeline_get_time(tl) * 100.0, 300.0);
	/* setting it from the application does not call back */
	g_time_calls = 0;
	Rtimeline_set_time(tl, 2.5);
	CHECK(g_time_calls == 0 && Rtimeline_get_time(tl) == 2.5);
	Rtimeline_set_time(tl, 99.0);
	CHECK(Rtimeline_get_time(tl) == 10.0);
	Rtimeline_set_time(tl, 9.5);

	/* a clip is dragged two seconds later, and down to the other track */
	drag(window, x + 100.0f + second * 2.0f, y + 30.0f + 19.0f, x + 100.0f + second * 4.0f, y + 30.0f + 38.0f + 19.0f);
	CHECK_NEAR(Rtimeline_get_item_start(tl, one) * 100.0, 300.0);
	CHECK(Rtimeline_get_item_length(tl, one) == 2.0);
	CHECK(Rtimeline_get_item_track(tl, one) == track_b);
	/* (told as it happens: once per step of the gesture) */
	CHECK(g_item_calls[0] >= 1 && g_item_last[0] == one);
	CHECK(Rtimeline_is_selected(tl, one) && !Rtimeline_is_selected(tl, two));
	CHECK(g_item_calls[1] >= 1);

	/* the right end of the other clip is pulled one second further */
	drag(window, x + 100.0f + second * 6.0f - 3.0f, y + 30.0f + 38.0f + 19.0f, x + 100.0f + second * 7.0f - 3.0f, y + 30.0f + 38.0f + 19.0f);
	CHECK(Rtimeline_get_item_start(tl, two) == 4.0);
	CHECK_NEAR(Rtimeline_get_item_length(tl, two) * 100.0, 300.0);
	CHECK(g_item_last[0] == two);

	/* Escape while dragging puts things back */
	Rwindow_inject_mouse_move(window, x + 100.0f + second * 5.0f, y + 30.0f + 19.0f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(window, x + 100.0f + second * 6.0f, y + 30.0f + 19.0f);
	Rwindow_inject_mouse_move(window, x + 100.0f + second * 7.0f, y + 30.0f + 19.0f);
	CHECK_NEAR(Rtimeline_get_item_start(tl, key) * 100.0, 700.0);
	press_key(window, R_KEY_ESCAPE, 0);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	CHECK(Rtimeline_get_item_start(tl, key) == 5.0);

	/* a locked track keeps its items */
	Rtimeline_set_track_locked(tl, track_a, R_TRUE);
	drag(window, x + 100.0f + second * 5.0f, y + 30.0f + 19.0f, x + 100.0f + second * 6.0f, y + 30.0f + 19.0f);
	CHECK(Rtimeline_get_item_start(tl, key) == 5.0);
	Rtimeline_set_track_locked(tl, track_a, R_FALSE);
	Rtimeline_set_track_muted(tl, track_a, R_TRUE);
	CHECK(Rtimeline_is_track_muted(tl, track_a));

	/* the keyboard: Delete removes what is selected, after telling */
	Rtimeline_select(tl, key, R_FALSE);
	CHECK(Rtimeline_get_selection_count(tl) == 1 && Rtimeline_is_selected(tl, key));
	Rwidget_focus(tl);
	press_key(window, R_KEY_DELETE, 0);
	CHECK(g_item_calls[2] == 1 && g_item_last[2] == key);
	CHECK(Rtimeline_get_item_count(tl) == 3);
	CHECK(Rtimeline_get_item_type(tl, key) == R_TIMELINE_NONE);
	/* arrows move the playhead by a frame, Home goes to the start */
	Rtimeline_set_time(tl, 2.0);
	press_key(window, R_KEY_RIGHT, 0);
	CHECK_NEAR(Rtimeline_get_time(tl) * 100.0, 210.0);
	press_key(window, R_KEY_HOME, 0);
	CHECK(Rtimeline_get_time(tl) == 0.0);
	press_key(window, R_KEY_A, R_MOD_CTRL);
	CHECK(Rtimeline_get_selection_count(tl) == 3);
	Rtimeline_clear_selection(tl);
	CHECK(Rtimeline_get_selection_count(tl) == 0);

	/* the view: Ctrl + wheel zooms around the pointer */
	Rtimeline_get_view(tl, &start, &end);
	CHECK(start == 0.0 && end == 10.0);
	Rwindow_inject_mouse_move(window, x + 100.0f + second * 5.0f, y + 100.0f);
	Rwindow_inject_key(window, R_KEY_LEFT_CTRL, R_TRUE, R_MOD_CTRL);
	Rwindow_inject_mouse_wheel(window, 0.0f, 2.0f);
	Rwindow_inject_key(window, R_KEY_LEFT_CTRL, R_FALSE, 0);
	Rdraw_frame();
	Rtimeline_get_view(tl, &start, &end);
	CHECK(start > 0.5 && end < 9.5 && end - start < 8.0);
	CHECK_NEAR((start + end) * 0.5 * 10.0, 50.0);
	Rtimeline_set_view(tl, 2.0, 6.0);
	Rtimeline_get_view(tl, &start, &end);
	CHECK(start == 2.0 && end == 6.0);
	EXPECT_ERRORS_BEGIN();
	Rtimeline_set_view(tl, 5.0, 5.0);
	Rtimeline_set_duration(tl, -1.0);
	EXPECT_ERRORS_END(2);

	/* from the application */
	Rtimeline_set_item(tl, one, track_a, 0.5, 1.5);
	CHECK(Rtimeline_get_item_track(tl, one) == track_a && Rtimeline_get_item_start(tl, one) == 0.5 && Rtimeline_get_item_length(tl, one) == 1.5);
	Rtimeline_set_item_label(tl, one, "renamed");
	CHECK_STR(Rtimeline_get_item_label(tl, one), "renamed");
	{
		int track_c = Rtimeline_add_track(tl, "C");
		int clip = Rtimeline_add_clip(tl, track_c, 1.0, 1.0, "c");
		int added_key = Rtimeline_add_key(tl, track_c, 3.0);
		CHECK(track_c > 0 && clip > 0 && added_key > 0 && clip != added_key);
		CHECK(Rtimeline_get_track_count(tl) == 3 && Rtimeline_get_item_count(tl) == 5);
		Rtimeline_set_track_name(tl, track_c, "Sound");
		CHECK_STR(Rtimeline_get_track_name(tl, track_c), "Sound");
		/* removing a track removes what was on it */
		Rtimeline_remove_track(tl, track_c);
		CHECK(Rtimeline_get_track_count(tl) == 2 && Rtimeline_get_item_count(tl) == 3);
		CHECK(Rtimeline_get_item_type(tl, clip) == R_TIMELINE_NONE);
	}
	Rtimeline_remove_item(tl, marker);
	CHECK(Rtimeline_get_item_count(tl) == 2);

	/* playing */
	CHECK(!Rtimeline_is_playing(tl));
	Rtimeline_set_playing(tl, R_TRUE);
	CHECK(Rtimeline_is_playing(tl));
	Rdraw_frame();
	Rtimeline_set_playing(tl, R_FALSE);
	CHECK(!Rtimeline_is_playing(tl));
	Rtimeline_set_loop(tl, R_TRUE);

	Rtimeline_clear(tl);
	CHECK(Rtimeline_get_item_count(tl) == 0 && Rtimeline_get_track_count(tl) == 0);
	Rdraw_frame();
	Rshutdown();
}

/* ---------------------------------------------------------------------------
 *  Node graph
 * ------------------------------------------------------------------------- */

static int g_graph_calls[5];
static int g_graph_last[5];
static void on_graph(Rid graph, int item, void* user_data) {
	int which = (int)(intptr_t)user_data;
	(void)graph;
	g_graph_calls[which]++;
	g_graph_last[which] = item;
}

static int g_connect_allowed = 1;
static int g_connect_calls = 0;
static int on_connect(Rid graph, int output_pin, int input_pin, void* user_data) {
	(void)user_data;
	g_connect_calls++;
	CHECK(Rnode_graph_is_pin_output(graph, output_pin) && !Rnode_graph_is_pin_output(graph, input_pin));
	return g_connect_allowed;
}

static void test_node_graph(void) {
	Rid window, graph, check;
	int a, b, c, a_out, a_color, b_in, b_any, c_in, link, other;
	float x, y, w, h, px, py, qx, qy, zoom;

	CHECK(test_init(0));
	window = make_window("<Window Width=\"900\" Height=\"600\"><NodeGraph Name=\"g\">"
		"<Type Name=\"number\" Color=\"#00FF00\"/>"
		"<Node Name=\"a\" Title=\"Source\" X=\"50\" Y=\"50\"><Output Name=\"o\" Label=\"Out\" Type=\"number\"/><Output Name=\"c\" Label=\"Col\" Type=\"color\"/></Node>"
		"<Node Name=\"b\" Title=\"Mix\" X=\"400\" Y=\"80\"><Input Name=\"i\" Label=\"In\" Type=\"number\"/><Input Label=\"Any\"/>"
		"<Checkbox Name=\"chk\" Text=\"Option\"/></Node>"
		"<Node Name=\"c\" Title=\"Result\" X=\"400\" Y=\"320\" Color=\"#FF0000\"><Input Name=\"i\" Label=\"In\" Type=\"number\"/></Node>"
		"<Link From=\"a.o\" To=\"b.i\"/>"
		"</NodeGraph></Window>");
	graph = Rwidget_find(window, "g");
	CHECK(graph != R_INVALID_ID && Rwidget_get_type(graph) == R_WIDGET_NODE_GRAPH);
	CHECK_RECT(graph, 0.0f, 0.0f, 900.0f, 600.0f);

	/* what the document said */
	CHECK(Rnode_graph_get_node_count(graph) == 3);
	a = Rnode_graph_find_node(graph, "a");
	b = Rnode_graph_find_node(graph, "b");
	c = Rnode_graph_find_node(graph, "c");
	CHECK(a && b && c && a != b && b != c);
	CHECK(Rnode_graph_get_node(graph, 0) == a && Rnode_graph_get_node(graph, 2) == c && Rnode_graph_get_node(graph, 3) == 0);
	CHECK(Rnode_graph_find_node(graph, "nobody") == 0);
	CHECK_STR(Rnode_graph_get_node_title(graph, b), "Mix");
	CHECK_STR(Rnode_graph_get_node_name(graph, b), "b");
	Rnode_graph_get_node_position(graph, c, &px, &py);
	CHECK(px == 400.0f && py == 320.0f);
	a_out = Rnode_graph_find_pin(graph, a, "o");
	a_color = Rnode_graph_find_pin(graph, a, "c");
	b_in = Rnode_graph_find_pin(graph, b, "i");
	b_any = Rnode_graph_find_pin(graph, b, "Any");
	c_in = Rnode_graph_find_pin(graph, c, "i");
	CHECK(a_out && a_color && b_in && b_any && c_in);
	CHECK(Rnode_graph_get_output(graph, a, 0) == a_out && Rnode_graph_get_output(graph, a, 1) == a_color && Rnode_graph_get_output(graph, a, 2) == 0);
	CHECK(Rnode_graph_get_input(graph, b, 1) == b_any && Rnode_graph_get_input(graph, a, 0) == 0);
	CHECK(Rnode_graph_get_pin_node(graph, b_any) == b);
	CHECK(Rnode_graph_is_pin_output(graph, a_out) && !Rnode_graph_is_pin_output(graph, b_in));
	CHECK_STR(Rnode_graph_get_pin_label(graph, a_color), "Col");
	CHECK_STR(Rnode_graph_get_pin_type(graph, a_color), "color");
	CHECK_STR(Rnode_graph_get_pin_type(graph, b_any), "");
	CHECK(Rnode_graph_get_link_count(graph) == 1);
	link = Rnode_graph_get_link(graph, 0);
	CHECK(link && Rnode_graph_get_link_output(graph, link) == a_out && Rnode_graph_get_link_input(graph, link) == b_in);
	CHECK(Rnode_graph_get_input_link(graph, b_in) == link && Rnode_graph_get_input_link(graph, c_in) == 0);
	check = Rnode_graph_get_node_content(graph, b);
	CHECK(check != R_INVALID_ID && check == Rwidget_find(window, "chk"));
	CHECK(Rnode_graph_get_node_content(graph, a) == R_INVALID_ID);

	/* the rules of linking: an output to an input of another node, of the same type or of any */
	CHECK(Rnode_graph_connect(graph, a_color, b_in) == 0);
	CHECK(Rnode_graph_connect(graph, b_in, a_out) == 0);
	CHECK(Rnode_graph_connect(graph, a_out, b_in) == link);
	other = Rnode_graph_connect(graph, a_out, c_in);
	CHECK(other && other != link && Rnode_graph_get_link_count(graph) == 2);
	CHECK(Rnode_graph_connect(graph, a_color, b_any) != 0);
	CHECK(Rnode_graph_get_link_count(graph) == 3);
	/* an input takes one link: a new one replaces it */
	CHECK(Rnode_graph_connect(graph, a_out, b_any) != 0);
	CHECK(Rnode_graph_get_link_count(graph) == 3);
	CHECK(Rnode_graph_get_link_output(graph, Rnode_graph_get_input_link(graph, b_any)) == a_out);
	Rnode_graph_disconnect(graph, other);
	Rnode_graph_disconnect(graph, Rnode_graph_get_input_link(graph, b_any));
	CHECK(Rnode_graph_get_link_count(graph) == 1 && Rnode_graph_get_input_link(graph, c_in) == 0);
	EXPECT_ERRORS_BEGIN();
	CHECK(Rnode_graph_connect(graph, a_out, 99999) == 0);
	CHECK(Rnode_graph_add_input(graph, 99999, "x", NULL) == 0);
	EXPECT_ERRORS_END(2);

	/* sizes and places: the pins are on the edges of their node */
	Rnode_graph_get_node_size(graph, a, &w, &h);
	CHECK(w >= 140.0f && h > 60.0f);
	Rnode_graph_get_pin_position(graph, a_out, &px, &py);
	CHECK(px == 50.0f + w && py > 80.0f && py < 50.0f + h);
	Rnode_graph_get_pin_position(graph, b_in, &qx, &qy);
	CHECK(qx == 400.0f && qy > 110.0f);
	/* until the view is moved, the corner of the canvas is the corner of the widget */
	Rnode_graph_get_view(graph, &x, &y, &zoom);
	CHECK(x == 450.0f && y == 300.0f && zoom == 1.0f);
	Rnode_graph_to_canvas(graph, 120.0f, 80.0f, &x, &y);
	CHECK(x == 120.0f && y == 80.0f);

	Rnode_graph_set_moved_callback(graph, on_graph, (void*)(intptr_t)0);
	Rnode_graph_set_selection_callback(graph, on_graph, (void*)(intptr_t)1);
	Rnode_graph_set_linked_callback(graph, on_graph, (void*)(intptr_t)2);
	Rnode_graph_set_unlinked_callback(graph, on_graph, (void*)(intptr_t)3);
	memset(g_graph_calls, 0, sizeof(g_graph_calls));

	/* a node is dragged by its header */
	drag(window, 420.0f, 330.0f, 520.0f, 380.0f);
	Rnode_graph_get_node_position(graph, c, &px, &py);
	CHECK(px == 500.0f && py == 370.0f);
	CHECK(g_graph_calls[0] == 1 && g_graph_last[0] == c);
	CHECK(Rnode_graph_is_node_selected(graph, c) && Rnode_graph_get_selection_count(graph) == 1 && Rnode_graph_get_selected_node(graph, 0) == c);
	CHECK(g_graph_calls[1] == 1 && g_graph_last[1] == c);
	/* Escape while dragging puts it back */
	Rwindow_inject_mouse_move(window, 520.0f, 380.0f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(window, 560.0f, 400.0f);
	Rwindow_inject_mouse_move(window, 600.0f, 420.0f);
	Rnode_graph_get_node_position(graph, c, &px, &py);
	CHECK(px == 580.0f && py == 410.0f);
	press_key(window, R_KEY_ESCAPE, 0);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	Rnode_graph_get_node_position(graph, c, &px, &py);
	CHECK(px == 500.0f && py == 370.0f);
	CHECK(g_graph_calls[0] == 1);

	/* a wire is pulled from an output to an input: the application is asked, then told */
	Rnode_graph_set_connect_callback(graph, on_connect, NULL);
	Rnode_graph_get_pin_position(graph, a_out, &px, &py);
	Rnode_graph_get_pin_position(graph, c_in, &qx, &qy);
	g_connect_allowed = 0;
	drag(window, px, py, qx, qy);
	CHECK(g_connect_calls == 1 && Rnode_graph_get_input_link(graph, c_in) == 0 && g_graph_calls[2] == 0);
	g_connect_allowed = 1;
	drag(window, px, py, qx, qy);
	other = Rnode_graph_get_input_link(graph, c_in);
	CHECK(g_connect_calls == 2 && other != 0 && Rnode_graph_get_link_output(graph, other) == a_out);
	CHECK(g_graph_calls[2] == 1 && g_graph_last[2] == other);
	CHECK(Rnode_graph_get_link_count(graph) == 2);
	/* pulled the other way round, from the input, it is the same link */
	Rnode_graph_disconnect(graph, other);
	drag(window, qx, qy, px, py);
	other = Rnode_graph_get_input_link(graph, c_in);
	CHECK(other != 0 && Rnode_graph_get_link_output(graph, other) == a_out && g_graph_calls[2] == 2);
	/* a wire that does not fit goes nowhere: a color into a number */
	{
		float cx, cy;
		Rnode_graph_get_pin_position(graph, a_color, &cx, &cy);
		Rnode_graph_disconnect(graph, other);
		drag(window, cx, cy, qx, qy);
		CHECK(Rnode_graph_get_input_link(graph, c_in) == 0 && g_graph_calls[2] == 2);
		drag(window, px, py, qx, qy);
		other = Rnode_graph_get_input_link(graph, c_in);
		CHECK(other != 0 && g_graph_calls[2] == 3);
	}
	/* dropped on the node itself, it finds a pin that fits */
	Rnode_graph_disconnect(graph, other);
	{
		float nx, ny, nw, nh;
		Rnode_graph_get_node_position(graph, c, &nx, &ny);
		Rnode_graph_get_node_size(graph, c, &nw, &nh);
		drag(window, px, py, nx + nw * 0.5f, ny + 12.0f);
		other = Rnode_graph_get_input_link(graph, c_in);
		CHECK(other != 0 && g_graph_calls[2] == 4);
	}
	/* taken by its end at the input and let go in the void, a link is removed */
	drag(window, qx, qy, qx - 60.0f, qy + 150.0f);
	CHECK(Rnode_graph_get_input_link(graph, c_in) == 0);
	CHECK(g_graph_calls[3] == 1 && g_graph_last[3] == other);
	CHECK(Rnode_graph_get_link_count(graph) == 1);
	Rnode_graph_set_connect_callback(graph, NULL, NULL);

	/* selection: a click, Shift + click, a rectangle, the empty canvas */
	click_at(window, 420.0f, 92.0f);
	CHECK(Rnode_graph_is_node_selected(graph, b) && Rnode_graph_get_selection_count(graph) == 1);
	Rwindow_inject_mouse_move(window, 520.0f, 380.0f);
	Rwindow_inject_key(window, R_KEY_LEFT_SHIFT, R_TRUE, R_MOD_SHIFT);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	Rwindow_inject_key(window, R_KEY_LEFT_SHIFT, R_FALSE, 0);
	CHECK(Rnode_graph_get_selection_count(graph) == 2 && Rnode_graph_is_node_selected(graph, c));
	click_at(window, 800.0f, 560.0f);
	CHECK(Rnode_graph_get_selection_count(graph) == 0);
	drag(window, 20.0f, 20.0f, 880.0f, 590.0f);
	CHECK(Rnode_graph_get_selection_count(graph) == 3);
	Rnode_graph_clear_selection(graph);
	Rnode_graph_select_node(graph, a, R_FALSE);
	Rnode_graph_select_node(graph, b, R_TRUE);
	CHECK(Rnode_graph_get_selection_count(graph) == 2 && !Rnode_graph_is_node_selected(graph, c));
	Rnode_graph_clear_selection(graph);

	/* the widget inside a node works like any other */
	Rwidget_get_rect(check, &x, &y, &w, &h);
	CHECK(x > 400.0f && y > 110.0f && w > 20.0f);
	CHECK(!Rcheckbox_is_checked(check));
	click_at(window, x + 10.0f, y + h * 0.5f);
	CHECK(Rcheckbox_is_checked(check));
	/* ... also when the graph is seen smaller: the wheel zooms around the pointer */
	px = x + 10.0f;
	py = y + h * 0.5f;
	Rwindow_inject_mouse_move(window, px, py);
	Rwindow_inject_mouse_wheel(window, 0.0f, -1.0f);
	Rdraw_frame();
	Rnode_graph_get_view(graph, NULL, NULL, &zoom);
	CHECK_NEAR(zoom * 100.0f, 80.0f);
	Rnode_graph_to_canvas(graph, px, py, &qx, &qy);
	CHECK_NEAR(qx, px);
	CHECK_NEAR(qy, py);
	click_at(window, px, py);
	CHECK(!Rcheckbox_is_checked(check));
	/* a place of the canvas is elsewhere in the widget now, and the widget of the node with it */
	Rnode_graph_from_canvas(graph, 400.0f, 80.0f, &qx, &qy);
	CHECK(fabsf(qx - 400.0f) > 1.0f || fabsf(qy - 80.0f) > 1.0f);
	Rnode_graph_from_canvas(graph, x + w - 4.0f, y + h * 0.5f, &qx, &qy);
	click_at(window, qx, qy);
	CHECK(Rcheckbox_is_checked(check));
	/* seen from far, the widgets inside the nodes let the node take the mouse */
	Rnode_graph_set_view(graph, 450.0f, 300.0f, 0.25f);
	Rdraw_frame();
	Rnode_graph_from_canvas(graph, x + 10.0f, y + h * 0.5f, &qx, &qy);
	click_at(window, qx, qy);
	CHECK(Rcheckbox_is_checked(check));
	CHECK(Rnode_graph_is_node_selected(graph, b));

	/* the view */
	Rnode_graph_set_view(graph, 100.0f, 50.0f, 2.0f);
	Rnode_graph_get_view(graph, &x, &y, &zoom);
	CHECK(x == 100.0f && y == 50.0f && zoom == 2.0f);
	Rnode_graph_to_canvas(graph, 450.0f, 300.0f, &x, &y);
	CHECK_NEAR(x, 100.0f);
	CHECK_NEAR(y, 50.0f);
	Rnode_graph_from_canvas(graph, 110.0f, 50.0f, &x, &y);
	CHECK_NEAR(x, 470.0f);
	CHECK_NEAR(y, 300.0f);
	EXPECT_ERRORS_BEGIN();
	Rnode_graph_set_view(graph, 0.0f, 0.0f, 0.0f);
	EXPECT_ERRORS_END(1);
	Rnode_graph_frame_all(graph);
	Rdraw_frame();
	Rnode_graph_get_view(graph, &x, &y, &zoom);
	CHECK(zoom <= 1.0f && zoom >= 0.2f);
	{
		/* every node is in sight */
		int i;
		for (i = 0; i < Rnode_graph_get_node_count(graph); i++) {
			float nx, ny, nw, nh, sx, sy, ex, ey;
			int node = Rnode_graph_get_node(graph, i);
			Rnode_graph_get_node_position(graph, node, &nx, &ny);
			Rnode_graph_get_node_size(graph, node, &nw, &nh);
			Rnode_graph_from_canvas(graph, nx, ny, &sx, &sy);
			Rnode_graph_from_canvas(graph, nx + nw, ny + nh, &ex, &ey);
			CHECK(sx >= 0.0f && sy >= 0.0f && ex <= 900.0f && ey <= 600.0f);
		}
	}
	Rnode_graph_set_grid_visible(graph, R_FALSE);
	Rnode_graph_set_flow_visible(graph, R_TRUE);
	Rdraw_frame();
	Rnode_graph_set_flow_visible(graph, R_FALSE);

	/* read only: looked at, not changed */
	Rnode_graph_set_view(graph, 450.0f, 300.0f, 1.0f);
	Rdraw_frame();
	Rnode_graph_set_read_only(graph, R_TRUE);
	Rnode_graph_get_node_position(graph, a, &px, &py);
	drag(window, px + 20.0f, py + 12.0f, px + 120.0f, py + 60.0f);
	Rnode_graph_get_node_position(graph, a, &qx, &qy);
	CHECK(qx == px && qy == py);
	Rnode_graph_set_read_only(graph, R_FALSE);

	/* the Delete key: links go; nodes only when the application listens */
	Rnode_graph_clear_selection(graph);
	Rnode_graph_select_node(graph, c, R_FALSE);
	Rwidget_focus(graph);
	press_key(window, R_KEY_DELETE, 0);
	CHECK(Rnode_graph_get_node_count(graph) == 3);
	Rnode_graph_set_removed_callback(graph, on_graph, (void*)(intptr_t)4);
	press_key(window, R_KEY_DELETE, 0);
	CHECK(Rnode_graph_get_node_count(graph) == 2 && g_graph_calls[4] == 1 && g_graph_last[4] == c);
	CHECK(Rnode_graph_find_node(graph, "c") == 0);
	/* removing a node removes its pins, their links, and its widget */
	CHECK(Rnode_graph_get_link_count(graph) == 1);
	Rnode_graph_remove_node(graph, b);
	Rdraw_frame();
	CHECK(Rnode_graph_get_node_count(graph) == 1 && Rnode_graph_get_link_count(graph) == 0);
	CHECK(!Rwidget_is_valid(check));

	/* built by the application */
	{
		int n1 = Rnode_graph_add_node(graph, "One", 10.0f, 10.0f);
		int n2 = Rnode_graph_add_node(graph, "Two", 300.0f, 10.0f);
		int out = Rnode_graph_add_output(graph, n1, "Value", "number");
		int in = Rnode_graph_add_input(graph, n2, "Value", "number");
		Rid slider = Rwidget_create(R_WIDGET_SLIDER), label = Rlabel_create("inside");
		int user = 5;
		CHECK(n1 && n2 && out && in);
		CHECK(Rnode_graph_connect(graph, out, in) != 0);
		Rnode_graph_set_node_title(graph, n1, "First");
		CHECK_STR(Rnode_graph_get_node_title(graph, n1), "First");
		Rnode_graph_set_node_name(graph, n1, "first");
		CHECK(Rnode_graph_find_node(graph, "first") == n1);
		Rnode_graph_set_pin_name(graph, out, "v");
		CHECK(Rnode_graph_find_pin(graph, n1, "v") == out && Rnode_graph_find_pin(graph, n1, "Value") == 0);
		Rnode_graph_set_node_position(graph, n1, 20.0f, 30.0f);
		Rnode_graph_get_node_position(graph, n1, &px, &py);
		CHECK(px == 20.0f && py == 30.0f);
		Rnode_graph_set_node_color(graph, n1, 1.0f, 0.5f, 0.0f);
		Rnode_graph_set_type_color(graph, "number", 0.0f, 0.5f, 1.0f);
		Rnode_graph_set_node_user_data(graph, n1, &user);
		CHECK(Rnode_graph_get_node_user_data(graph, n1) == &user && Rnode_graph_get_node_user_data(graph, n2) == NULL);
		/* a widget inside: it makes the node larger, and belongs to the graph */
		Rnode_graph_get_node_size(graph, n2, &w, &h);
		Rnode_graph_set_node_content(graph, n2, slider);
		CHECK(Rnode_graph_get_node_content(graph, n2) == slider);
		Rnode_graph_get_node_size(graph, n2, &qx, &qy);
		CHECK(qy > h + 10.0f);
		Rdraw_frame();
		Rnode_graph_set_node_content(graph, n2, label);
		Rdraw_frame();
		CHECK(!Rwidget_is_valid(slider) && Rnode_graph_get_node_content(graph, n2) == label);
		Rnode_graph_set_node_content(graph, n2, R_INVALID_ID);
		Rdraw_frame();
		CHECK(!Rwidget_is_valid(label) && Rnode_graph_get_node_content(graph, n2) == R_INVALID_ID);
		/* widgets only get in through the nodes */
		label = Rlabel_create("stray");
		EXPECT_ERRORS_BEGIN();
		Rwidget_add_child(graph, label);
		EXPECT_ERRORS_END(1);
		Rwidget_destroy(label);
		Rnode_graph_remove_pin(graph, out);
		CHECK(Rnode_graph_get_link_count(graph) == 0 && Rnode_graph_get_output(graph, n1, 0) == 0);
		Rnode_graph_clear(graph);
		CHECK(Rnode_graph_get_node_count(graph) == 0);
	}
	Rdraw_frame();
	Rshutdown();
}

/* A slider inside a node, dragged while the graph is seen at half its size, then larger than life. */
static void test_zoomed_content(void) {
	Rid window, graph, slider, combo;
	float x, y, w, h, x0, y0, x1, y1;
	int node;

	CHECK(test_init(0));
	window = make_window("<Window Width=\"900\" Height=\"600\"><NodeGraph Name=\"g\">"
		"<Node Name=\"n\" Title=\"Node\" X=\"200\" Y=\"150\"><Slider Name=\"s\" Width=\"200\" Value=\"0\"/>"
		"<ComboBox Name=\"c\" Items=\"One|Two|Three\" Selected=\"0\"/></Node></NodeGraph></Window>");
	graph = Rwidget_find(window, "g");
	slider = Rwidget_find(window, "s");
	combo = Rwidget_find(window, "c");
	node = Rnode_graph_find_node(graph, "n");
	CHECK(slider != R_INVALID_ID && combo != R_INVALID_ID && node != 0);
	/* (until the view moves, the canvas and the widget are the same space) */
	Rwidget_get_rect(slider, &x, &y, &w, &h);
	CHECK(x > 200.0f && y > 180.0f);
	CHECK_NEAR(w, 200.0f);
	CHECK(Rslider_get_value(slider) == 0.0f);

	Rnode_graph_set_view(graph, 300.0f, 200.0f, 0.5f);
	Rdraw_frame();
	Rnode_graph_from_canvas(graph, x, y + h * 0.5f, &x0, &y0);
	Rnode_graph_from_canvas(graph, x + w, y + h * 0.5f, &x1, &y1);
	/* half the size on screen */
	CHECK_NEAR(x1 - x0, 100.0f);
	drag(window, x0 + 2.0f, y0, (x0 + x1) * 0.5f, y0);
	CHECK(Rslider_get_value(slider) > 0.42f && Rslider_get_value(slider) < 0.58f);
	/* the node itself did not move: the slider took the mouse */
	{
		float nx, ny;
		Rnode_graph_get_node_position(graph, node, &nx, &ny);
		CHECK(nx == 200.0f && ny == 150.0f);
	}
	drag(window, (x0 + x1) * 0.5f, y0, x1 + 40.0f, y0);
	CHECK(Rslider_get_value(slider) == 1.0f);

	Rnode_graph_set_view(graph, 300.0f, 200.0f, 2.0f);
	Rdraw_frame();
	Rnode_graph_from_canvas(graph, x, y + h * 0.5f, &x0, &y0);
	Rnode_graph_from_canvas(graph, x + w, y + h * 0.5f, &x1, &y1);
	CHECK_NEAR(x1 - x0, 400.0f);
	drag(window, x1 - 4.0f, y0, x0 + (x1 - x0) * 0.25f, y0);
	CHECK(Rslider_get_value(slider) > 0.2f && Rslider_get_value(slider) < 0.3f);

	/* a list that drops down from a widget of a zoomed node opens where the widget shows, and its rows work */
	{
		float cx, cy;
		/* where the combo box is on the canvas: read at life size */
		Rnode_graph_set_view(graph, 300.0f, 200.0f, 1.0f);
		Rdraw_frame();
		Rwidget_get_rect(combo, &x, &y, &w, &h);
		Rnode_graph_to_canvas(graph, x + w * 0.5f, y + h * 0.5f, &cx, &cy);
		Rnode_graph_set_view(graph, 300.0f, 200.0f, 0.5f);
		Rdraw_frame();
		Rnode_graph_from_canvas(graph, cx, cy, &x0, &y0);
		click_at(window, x0, y0);
		Rdraw_frame();
		/* the second row: under the button as it shows (half its height), one row and a half further; popups are not zoomed */
		click_at(window, x0, y0 + h * 0.25f + 2.0f + Rtheme_get_metric(R_METRIC_ROW_HEIGHT) * 1.5f);
		Rdraw_frame();
		CHECK(Rcombo_box_get_selected(combo) == 1);
	}
	Rshutdown();
}

/* ---------------------------------------------------------------------------
 *  Background blur, windows that are seen through
 * ------------------------------------------------------------------------- */

static void test_blur_and_transparency(void) {
	Rid window, plain, blur, label;
	CHECK(test_init(0));

	window = make_window("<Window Width=\"400\" Height=\"300\" Transparent=\"true\" Backdrop=\"Blur\">"
		"<BackgroundBlur Name=\"b\" Style=\"Native\" Radius=\"12\" Tint=\"#00000040\" Padding=\"8\" CornerRadius=\"10\" Grain=\"0.3\" Saturation=\"1.4\" Shadow=\"16\">"
		"<Label Name=\"l\" Text=\"over the desktop\"/></BackgroundBlur></Window>");
	blur = Rwidget_find(window, "b");
	label = Rwidget_find(window, "l");
	CHECK(blur != R_INVALID_ID && Rwidget_get_type(blur) == R_WIDGET_BACKGROUND_BLUR);
	CHECK(Rbackground_blur_get_style(blur) == R_BLUR_NATIVE);
	CHECK(Rbackground_blur_get_radius(blur) == 12.0f);
	CHECK_RECT(blur, 0.0f, 0.0f, 400.0f, 300.0f);
	CHECK_RECT(label, 8.0f, 8.0f, 384.0f, 284.0f);
	/* the window is seen through, and the system blurs what is behind */
	CHECK(Rwindow_get_backdrop(window) == R_BACKDROP_BLUR);
	CHECK(Rwindow_set_backdrop(window, R_BACKDROP_ACRYLIC));
	CHECK(Rwindow_get_backdrop(window) == R_BACKDROP_ACRYLIC);
	CHECK(Rwindow_set_backdrop(window, R_BACKDROP_TRANSPARENT));
	CHECK(Rwindow_get_backdrop(window) == R_BACKDROP_TRANSPARENT);
	CHECK(Rwindow_get_opacity(window) == 1.0f);
	CHECK(Rwindow_set_opacity(window, 0.5f));
	CHECK(Rwindow_get_opacity(window) == 0.5f);
	Rwindow_set_opacity(window, 7.0f);
	CHECK(Rwindow_get_opacity(window) == 1.0f);

	/* each style paints */
	Rbackground_blur_set_style(blur, R_BLUR_PLAIN);
	Rdraw_frame();
	Rbackground_blur_set_style(blur, R_BLUR_FROSTED);
	Rdraw_frame();
	Rbackground_blur_set_style(blur, R_BLUR_GLASS);
	CHECK(Rbackground_blur_get_style(blur) == R_BLUR_GLASS);
	Rdraw_frame();
	Rbackground_blur_set_radius(blur, 30.0f);
	CHECK(Rbackground_blur_get_radius(blur) == 30.0f);
	Rbackground_blur_set_tint(blur, 1.0f, 1.0f, 1.0f, 0.2f);
	Rbackground_blur_clear_tint(blur);
	Rbackground_blur_set_saturation(blur, 1.0f);
	Rbackground_blur_set_grain(blur, 0.0f);
	Rbackground_blur_set_corner_radius(blur, 4.0f, 8.0f, 12.0f, 16.0f);
	Rbackground_blur_set_padding(blur, 1.0f, 2.0f, 3.0f, 4.0f);
	Rbackground_blur_set_shadow(blur, 0.0f);
	Rdraw_frame();
	CHECK_RECT(label, 1.0f, 2.0f, 396.0f, 294.0f);
	CHECK(Rwidget_set_attribute(blur, "Style", "Frosted") && Rbackground_blur_get_style(blur) == R_BLUR_FROSTED);
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rwidget_set_attribute(blur, "Style", "Smoked"));
	EXPECT_ERRORS_END(0);

	/* an ordinary window is not seen through, and says so */
	plain = Rwindow_create("plain", 300, 200, 0);
	CHECK(Rwindow_get_backdrop(plain) == R_BACKDROP_NONE);
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rwindow_set_backdrop(plain, R_BACKDROP_BLUR));
	EXPECT_ERRORS_END(1);
	CHECK(Rwindow_get_backdrop(plain) == R_BACKDROP_NONE);
	/* asked for from code, with the flag */
	Rwindow_destroy(plain);
	plain = Rwindow_create("clear", 300, 200, R_WINDOW_TRANSPARENT);
	CHECK(Rwindow_get_backdrop(plain) == R_BACKDROP_TRANSPARENT);
	blur = Rbackground_blur_create();
	Rwindow_set_content(plain, blur);
	CHECK(Rbackground_blur_get_style(blur) == R_BLUR_FROSTED);
	Rdraw_frame();
	Rshutdown();
}

/* ---------------------------------------------------------------------------
 *  What other libraries build on
 * ------------------------------------------------------------------------- */

static int g_gauge_created = 0;
static char g_gauge_level[32];
static Rid tag_create(const char* tag, void* user_data) {
	CHECK_STR(tag, "Gauge");
	CHECK(user_data == (void*)g_gauge_level);
	g_gauge_created++;
	return Rwidget_create(R_WIDGET_CUSTOM);
}

static int tag_attribute(Rid widget, const char* name, const char* value, void* user_data) {
	(void)widget;
	(void)user_data;
	if (strcmp(name, "Level") != 0) return R_FALSE;
	snprintf(g_gauge_level, sizeof(g_gauge_level), "%s", value);
	return R_TRUE;
}

static int g_destroyed = 0;
static void on_destroyed(Rid widget, void* user_data) {
	(void)widget;
	g_destroyed += *(int*)user_data;
}

static int g_curve_painted = 0;
static void paint_curve(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	(void)widget;
	(void)user_data;
	Rdraw_curve(painter, 0.0f, 0.0f, width * 0.5f, 0.0f, width * 0.5f, height, width, height, 3.0f, 1.0f, 0.5f, 0.0f, 1.0f);
	g_curve_painted++;
}

static void test_hooks(void) {
	static const char xml[] = "<VBox><Gauge Name=\"g\" Level=\"0.7\" Width=\"50\" Height=\"20\"/><Label Text=\"after\"/></VBox>";
	Rid tree, gauge, custom, window;
	int amount = 3;

	CHECK(test_init(0));

	/* a tag made by code */
	CHECK(!Rxml_is_tag_registered("Gauge"));
	EXPECT_ERRORS_BEGIN();
	CHECK(Rwidget_create_from_xml(xml, strlen(xml)) == R_INVALID_ID);
	CHECK(!Rxml_register_tag("Button", tag_create, NULL, NULL));
	CHECK(!Rxml_register_tag("Gauge", NULL, NULL, NULL));
	EXPECT_ERRORS_END(3);
	CHECK(Rxml_register_tag("Gauge", tag_create, tag_attribute, g_gauge_level));
	CHECK(Rxml_is_tag_registered("gauge"));
	tree = Rwidget_create_from_xml(xml, strlen(xml));
	CHECK(tree != R_INVALID_ID && g_gauge_created == 1);
	gauge = Rwidget_find(tree, "g");
	CHECK(gauge != R_INVALID_ID && Rwidget_get_type(gauge) == R_WIDGET_CUSTOM);
	CHECK(Rwidget_get_child(tree, 0) == gauge && Rwidget_get_child_count(tree) == 2);
	/* its own attribute went to its function, the others to the widget */
	CHECK_STR(g_gauge_level, "0.7");
	CHECK(Rwidget_set_attribute(gauge, "Level", "0.2"));
	CHECK_STR(g_gauge_level, "0.2");
	CHECK(Rwidget_set_attribute(gauge, "Opacity", "0.5"));
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rwidget_set_attribute(gauge, "Unknown", "1"));
	EXPECT_ERRORS_END(0);
	/* the widgets of Raven do not hear about it */
	CHECK(!Rwidget_set_attribute(Rwidget_get_child(tree, 1), "Level", "1"));
	CHECK_STR(g_gauge_level, "0.2");
	/* a widget outlives its tag */
	Rxml_unregister_tag("Gauge");
	CHECK(!Rxml_is_tag_registered("Gauge"));
	CHECK(!Rwidget_set_attribute(gauge, "Level", "0.9"));
	CHECK_STR(g_gauge_level, "0.2");
	EXPECT_ERRORS_BEGIN();
	CHECK(Rwidget_create_from_xml(xml, strlen(xml)) == R_INVALID_ID);
	EXPECT_ERRORS_END(1);

	/* told when a widget is gone: with its parent, once */
	Rwidget_set_destroy_callback(gauge, on_destroyed, &amount);
	Rwidget_destroy(tree);
	Rdraw_frame();
	CHECK(g_destroyed == 3);
	custom = Rwidget_create(R_WIDGET_CUSTOM);
	Rwidget_set_destroy_callback(custom, on_destroyed, &amount);
	Rwidget_set_destroy_callback(custom, NULL, NULL);
	Rwidget_destroy(custom);
	Rdraw_frame();
	CHECK(g_destroyed == 3);
	/* ... also when Rshutdown is what deletes it (checked below) */
	custom = Rwidget_create(R_WIDGET_CUSTOM);
	Rwidget_set_destroy_callback(custom, on_destroyed, &amount);

	/* files named in documents */
	Rxml_set_base_path("/somewhere/ui");
	CHECK_STR(Rxml_resolve_path("pages/a.html"), "/somewhere/ui/pages/a.html");
	Rxml_set_base_path(NULL);

	/* a picture decoded without making a texture: a 2 x 2 bitmap, bottom row first, blue green red */
	{
		static const unsigned char bmp[] = {
			'B', 'M', 70, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0,
			40, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 1, 0, 24, 0, 0, 0, 0, 0, 16, 0, 0, 0, 19, 11, 0, 0, 19, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			255, 0, 0,   0, 255, 0,   0, 0,
			0, 0, 255,   255, 255, 255,   0, 0
		};
		int width = 0, height = 0;
		uint8_t* pixels = Rimage_decode(bmp, sizeof(bmp), &width, &height);
#if defined(RAVEN_TEST_NO_IMAGE_DECODER)
		CHECK(pixels == NULL && width == 0 && height == 0);
#else
		CHECK(pixels != NULL && width == 2 && height == 2);
		if (pixels) {
			/* top left is red, top right white, bottom left blue, bottom right green; all opaque */
			CHECK(pixels[0] == 255 && pixels[1] == 0 && pixels[2] == 0 && pixels[3] == 255);
			CHECK(pixels[4] == 255 && pixels[5] == 255 && pixels[6] == 255);
			CHECK(pixels[8] == 0 && pixels[9] == 0 && pixels[10] == 255);
			CHECK(pixels[12] == 0 && pixels[13] == 255 && pixels[14] == 0);
		}
		Rpixels_free(pixels);
#endif
		CHECK(Rimage_decode("not an image", 12, &width, &height) == NULL && width == 0);
		CHECK(Rimage_decode(NULL, 0, NULL, NULL) == NULL);
	}

	/* a curve drawn by a custom widget */
	window = Rwindow_create("curve", 200, 100, 0);
	custom = Rwidget_create(R_WIDGET_CUSTOM);
	Rcustom_set_paint_callback(custom, paint_curve, NULL);
	Rwindow_set_content(window, custom);
	Rdraw_frame();
	CHECK(g_curve_painted >= 1);

	Rshutdown();
	CHECK(g_destroyed == 6);
}

/* The sizes of the timeline follow the theme and the text scale, or what the application sets. */
static void test_timeline_sizes(void) {
	Rid window, tl;
	int marker = 0, i;
	float x, y, w, h;

	CHECK(test_init(0));
	window = make_window("<Window Width=\"900\" Height=\"400\">"
		"<Timeline Name=\"t\" Duration=\"10\" HeaderWidth=\"100\">"
		"<Track Name=\"A\"><Clip Start=\"1\" Length=\"2\" Label=\"one\"/></Track>"
		"<Marker Time=\"8\" Label=\"m\"/></Timeline></Window>");
	tl = Rwidget_find(window, "t");
	/* the flat theme: the sizes of version 0.1.2 */
	CHECK(Rtimeline_get_ruler_height(tl) == 30.0f);
	CHECK(Rtimeline_get_default_track_height(tl) == 38.0f);
	CHECK(Rtimeline_get_header_width(tl) == 100.0f);

	/* larger texts: the ruler and the tracks grow with them */
	Raccessibility_set_text_scale(1.5f);
	Rdraw_frame();
	CHECK(Rtimeline_get_ruler_height(tl) == 45.0f);
	CHECK(Rtimeline_get_default_track_height(tl) == 57.0f);
	/* the flag of the marker is hit where it is painted, below the old 19 units */
	for (i = 0; i < Rtimeline_get_item_count(tl); i++) {
		int item = Rtimeline_get_item(tl, i);
		if (Rtimeline_get_item_type(tl, item) == R_TIMELINE_MARKER) marker = item;
	}
	CHECK(marker != 0);
	Rwidget_get_rect(tl, &x, &y, &w, &h);
	click_at(window, x + 100.0f + 80.0f * 8.0f + 4.0f, y + 22.0f);
	Rdraw_frame();
	CHECK(Rtimeline_is_selected(tl, marker));
	Raccessibility_set_text_scale(1.0f);

	/* set by the application, and back to automatic with 0 */
	Rtimeline_set_ruler_height(tl, 40.0f);
	Rtimeline_set_default_track_height(tl, 50.0f);
	CHECK(Rtimeline_get_ruler_height(tl) == 40.0f);
	CHECK(Rtimeline_get_default_track_height(tl) == 50.0f);
	Rtimeline_set_default_track_height(tl, 4.0f);
	CHECK(Rtimeline_get_default_track_height(tl) == 16.0f);
	Rtimeline_set_ruler_height(tl, 0.0f);
	Rtimeline_set_default_track_height(tl, 0.0f);
	CHECK(Rtimeline_get_ruler_height(tl) == 30.0f);
	CHECK(Rtimeline_get_default_track_height(tl) == 38.0f);
	Rtimeline_set_header_width(tl, 0.0f);
	Rdraw_frame();
	CHECK(Rtimeline_get_header_width(tl) >= 96.0f);
	Rwindow_destroy(window);

	/* in XML */
	window = make_window("<Window Width=\"600\" Height=\"300\">"
		"<Timeline Name=\"t\" RulerHeight=\"36\" TrackHeight=\"44\"/></Window>");
	tl = Rwidget_find(window, "t");
	CHECK(Rtimeline_get_ruler_height(tl) == 36.0f);
	CHECK(Rtimeline_get_default_track_height(tl) == 44.0f);
	Rwindow_destroy(window);
	Rshutdown();
}

int main(void) {
	RUN(test_color_picker);
	RUN(test_timeline);
	RUN(test_timeline_sizes);
	RUN(test_node_graph);
	RUN(test_zoomed_content);
	RUN(test_blur_and_transparency);
	RUN(test_hooks);
	return test_report("test_tools");
}
