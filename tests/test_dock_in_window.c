#include "test.h"
#include "../src/core/context.h"
#include "../src/widget/widget.h"
#include "../src/render/renderer.h"

static int g_clicks, g_red, g_blue, g_red_index, g_blue_index;
static RDrawInstance g_red_rect;

static void paint_panel(Rid widget, RPainter* painter, float w, float h, void* user) {
	(void)widget;
	Rdraw_rect(painter, 0, 0, w, h, 0, user ? 1 : 0, 0, user ? 0 : 1, 1);
}

static int on_panel(Rid widget, const REvent* event, void* user) {
	(void)widget; (void)user;
	if (event->type == R_EVENT_MOUSE_DOWN) { g_clicks++; return R_TRUE; }
	return R_FALSE;
}

static void observe_frame(const RDrawData* data, RCapture* capture) {
	Ruint i;
	(void)capture;
	g_red = g_blue = 0;
	for (i = 0; i < data->instance_count; i++) {
		const RDrawInstance* instance = &data->instances[i];
		if ((instance->flags & R_PRIM_MASK) != R_PRIM_RECT) continue;
		if (instance->color == 0xFF0000FFu) { g_red++; g_red_rect = *instance; g_red_index = (int)i; }
		if (instance->color == 0xFFFF0000u) { g_blue++; g_blue_index = (int)i; }
	}
}

static Rid add_panel(Rid dock, const char* name, int red) {
	Rid panel = Rwidget_create(R_WIDGET_CUSTOM);
	Rwidget_set_name(panel, name);
	Rcustom_set_paint_callback(panel, paint_panel, (void*)(uintptr_t)red);
	Rwidget_set_event_callback(panel, on_panel, NULL);
	Rdock_area_add_panel(dock, panel, name, R_DOCK_TAB, R_INVALID_ID, 0);
	return panel;
}

static void drag(Rid window, float x0, float y0, float x1, float y1) {
	Rwindow_inject_mouse_move(window, x0, y0);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(window, x1, y1);
	Rdraw_frame();
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	Rdraw_frame();
}

static void test_floating(void) {
	Rid window = Rwindow_create("Dock", 800, 600, 0);
	Rid canvas = Rwidget_create(R_WIDGET_CANVAS);
	Rid dock = Rwidget_add_child(canvas, Rdock_area_create());
	Rid red, blue, floating;
	float x, y, w, h;
	char saved[4096], before[4096];
	Rwindow_set_content(window, canvas);
	Rslot_set_position(dock, 100, 100);
	Rslot_set_size(dock, 400, 300);
	blue = add_panel(dock, "blue", 0);
	red = add_panel(dock, "red name,%", 1);
	Rdraw_frame();
	CHECK_STR(Rdock_area_save_layout(dock), "T(1;blue,red%20name%2C%25)");
	CHECK(Rwidget_set_attribute(dock, "FloatMode", "InWindow"));
	CHECK(Rdock_area_float_panel(dock, red, 550, 450) == window);
	Rdraw_frame();
	floating = Rwidget_get_parent(red);
	CHECK(floating != dock);
	CHECK(Rdock_area_get_panel_window(dock, red) == window);
	CHECK(g_rvn.windows.live == 1);
	CHECK(g_red == 1 && g_blue == 1 && g_red_index > g_blue_index);
	CHECK(g_red_rect.rect[2] > 500 && g_red_rect.clip[2] > 500); /* outside DockArea, inside the window */
	Rwidget_get_rect(red, &x, &y, &w, &h);
	CHECK(x >= 0 && y >= 0 && x + w <= 800 && y + h <= 600);
	test_click_at(window, x + w * 0.5f, y + h * 0.5f);
	CHECK(g_clicks == 1); /* hit testing outside the dock area reaches the floating panel */
	snprintf(saved, sizeof(saved), "%s", Rdock_area_save_layout(dock));
	CHECK(strncmp(saved, "D2(", 3) == 0);
	CHECK(Rdock_area_load_layout(dock, saved));
	Rdraw_frame();
	CHECK_STR(Rdock_area_save_layout(dock), saved);
	CHECK(Rdock_area_get_panel_window(dock, red) == window);
	/* Rejected descriptions must preserve membership, geometry and serialized state. */
	snprintf(before, sizeof(before), "%s", Rdock_area_save_layout(dock));
	CHECK(!Rdock_area_load_layout(dock, "D2(T(0;blue);F(1,2,100,100,T(0;blue)))"));
	CHECK(!Rdock_area_load_layout(dock, "D2(T(0;blue);F(nan,2,100,100,T(0;red%20name%2C%25)))"));
	CHECK(!Rdock_area_load_layout(dock, "D2(T(0;unknown))"));
	CHECK_STR(Rdock_area_save_layout(dock), before);
	Rdock_area_hide_panel(dock, red);
	Rdraw_frame();
	CHECK(!Rdock_area_is_panel_open(dock, red));
	CHECK(g_red == 0);
	CHECK(Rdock_area_float_panel(dock, red, 550, 450) == window);
	Rdraw_frame();
	CHECK(Rdock_area_is_panel_shown(dock, red));
	CHECK(g_red == 1);
	Rdock_area_dock_panel(dock, red);
	Rdraw_frame();
	CHECK(Rwidget_get_parent(red) == dock && Rwindow_is_valid(window));
	CHECK(rvn_window_get(window)->float_layer_count == 0);
	CHECK(Rdock_area_load_layout(dock, "T(0;blue,red%20name%2C%25)")); /* legacy layouts still work */
	CHECK(Rdock_area_float_panel(dock, red, 250, 200) == window);
	Rdraw_frame();
	floating = Rwidget_get_parent(red);
	Rwidget_get_rect(floating, &x, &y, &w, &h);
	/* Drag a card over the middle of the canvas; it moves without docking. */
	drag(window, x + 25, y + 10, 300, 250);
	CHECK(Rwidget_get_parent(red) == floating);
	Rwidget_get_rect(floating, &x, &y, &w, &h);
	CHECK_NEAR(x, 275);
	CHECK_NEAR(y, 240);
	/* The same tab can dock on the home bar. */
	drag(window, x + 25, y + 10, 125, 110);
	CHECK(Rwidget_get_parent(red) == dock);
	CHECK(g_rvn.windows.live == 1);
	Rdock_area_float_panel(dock, red, 700, 500);
	Rdraw_frame();
	Rwindow_set_size(window, 300, 200);
	Rdraw_frame();
	Rwidget_get_rect(Rwidget_get_parent(red), &x, &y, &w, &h);
	CHECK(x >= 0 && y >= 0 && x + w <= 300 && y + h <= 200);
	Rdock_area_set_float_mode(dock, R_DOCK_FLOAT_WINDOW); /* existing card keeps its mode */
	CHECK(Rdock_area_get_panel_window(dock, red) == window);
	Rwidget_destroy(red);
	Rdraw_frame();
	CHECK(Rwindow_is_valid(window));
	CHECK(rvn_window_get(window)->float_layer_count == 0);
	CHECK(Rwidget_is_valid(blue));
	Rwindow_destroy(window);
}


static void test_layout_lifecycle(void) {
	Rid window = Rwindow_create("Layouts", 800, 600, 0);
	Rid dock = Rdock_area_create();
	Rid a, b, hidden;
	char name[600], saved[4096];
	int i;
	memset(name, 'x', sizeof(name) - 1);
	name[sizeof(name) - 1] = '\0';
	Rwindow_set_content(window, dock);
	a = add_panel(dock, name, 1);
	b = add_panel(dock, "b", 0);
	hidden = add_panel(dock, "hidden", 0);
	Rdock_area_hide_panel(dock, hidden);
	Rdock_area_set_float_mode(dock, R_DOCK_FLOAT_IN_WINDOW);
	Rdraw_frame();
	CHECK(Rdock_area_float_panel(dock, a, 10, 20) == window);
	CHECK(Rdock_area_float_panel(dock, b, 400, 200) == window);
	Rdraw_frame();
	CHECK(rvn_window_get(window)->float_layer_count == 2);
	/* Empty home tree and multiple cards, including a name longer than the old parser buffer. */
	snprintf(saved, sizeof(saved), "%s", Rdock_area_save_layout(dock));
	for (i = 0; i < 3; i++) {
		CHECK(Rdock_area_load_layout(dock, saved));
		Rdraw_frame();
		CHECK_STR(Rdock_area_save_layout(dock), saved);
		CHECK(rvn_window_get(window)->float_layer_count == 2);
		CHECK(!Rdock_area_is_panel_open(dock, hidden));
	}
	Rdock_area_hide_panel(dock, a);
	Rdraw_frame();
	snprintf(saved, sizeof(saved), "%s", Rdock_area_save_layout(dock));
	CHECK(Rdock_area_load_layout(dock, saved));
	Rdraw_frame();
	CHECK(!Rdock_area_is_panel_open(dock, a));
	Rdock_area_open_panel(dock, a);
	CHECK(Rdock_area_is_panel_open(dock, a));
	Rdock_area_close_panel(dock, b);
	Rdraw_frame();
	CHECK(!Rwidget_is_valid(b));
	CHECK(rvn_window_get(window)->float_layer_count == 0);
	CHECK(Rwindow_is_valid(window));
	/* Destroying the home area also destroys its floating children without closing the window. */
	Rdock_area_float_panel(dock, a, 20, 20);
	Rdraw_frame();
	Rwidget_destroy(dock);
	Rdraw_frame();
	CHECK(!Rwidget_is_valid(a) && !Rwidget_is_valid(hidden));
	CHECK(rvn_window_get(window)->float_layer_count == 0);
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
	RUN(test_floating);
	RUN(test_layout_lifecycle);
	Rshutdown();
	return test_report("test_dock_in_window");
}
