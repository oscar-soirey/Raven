#include "test.h"
#include "../src/widget/widget.h"
#include "../src/render/renderer.h"

static int g_mode, g_dropped;
static Rid g_nested_window;
static Ruint g_instances;
static RDrawInstance g_outline;

static void observe_frame(const RDrawData* data, RCapture* capture) {
	Ruint i;
	(void)capture;
	g_instances = data->instance_count;
	for (i = 0; i < data->instance_count; i++)
		if ((data->instances[i].flags & R_PRIM_MASK) == R_PRIM_RECT && data->instances[i].border_width > 0)
			g_outline = data->instances[i];
}

static int on_drop(Rid target, const RDropEvent* event, void* user) {
	(void)user;
	if (event->phase == R_DROP_DROPPED) {
		EXPECT_ERRORS_BEGIN();
		Rdrop_set_highlight_rect(0, 0, 1, 1);
		EXPECT_ERRORS_END(1);
		g_dropped++;
		return R_TRUE;
	}
	if (event->phase != R_DROP_ENTER && event->phase != R_DROP_MOVE) return R_TRUE;
	switch (g_mode) {
		case 1: Rdrop_set_highlight_rect(12, 8, 60, 2); break;
		case 2: Rdrop_set_highlight_rect(0, 0, 0, 0); break;
		case 3: Rdrop_set_highlight_rect(-10, -20, 25, 30); break;
		case 4:
			Rdrop_set_highlight_rect(1, 2, 30, 40);
			EXPECT_ERRORS_BEGIN();
			Rdrop_set_highlight_rect(NAN, 0, 1, 1);
			Rdrop_set_highlight_rect(0, 0, -1, 1);
			Rdrop_set_highlight_rect(0, 0, 1, INFINITY);
			EXPECT_ERRORS_END(3);
			break;
		case 5:
			Rdrop_set_highlight_rect(1, 2, 3, 4);
			g_mode = 2;
			CHECK(Rwindow_inject_drag(g_nested_window, 50, 50, R_TRUE));
			g_mode = 5;
			Rdrop_set_highlight_rect(5, 6, 7, 8);
			break;
		case 6: Rwidget_destroy(target); break;
		case 7: Rdrop_set_highlight_rect(0, 0, 10, 10); return R_FALSE;
	}
	return R_TRUE;
}

static void test_rectangles(void) {
	Rid window = Rwindow_create("Drop", 400, 200, 0);
	Rid box = Rwidget_create(R_WIDGET_HBOX);
	Rid spacer = Rwidget_add_child(box, Rwidget_create(R_WIDGET_CUSTOM));
	Rid target = Rwidget_add_child(box, Rwidget_create(R_WIDGET_CUSTOM));
	rvn_window_t* state = rvn_window_get(window);
	float x, y, w, h;
	Rwidget_set_size(spacer, 100, 0);
	Rslot_set_fill(target, 1);
	Rwindow_set_content(window, box);
	Rwidget_set_drop_callback(target, on_drop, NULL);
	Rdraw_frame();
	Rwidget_get_rect(target, &x, &y, &w, &h);
	EXPECT_ERRORS_BEGIN();
	Rdrop_set_highlight_rect(0, 0, 10, 10);
	EXPECT_ERRORS_END(1);
	g_mode = 1;
	CHECK(Rwindow_inject_drag(window, x + 20, y + 20, R_TRUE));
	CHECK_NEAR(state->drop_rect.x, x + 12);
	CHECK_NEAR(state->drop_rect.y, y + 8);
	CHECK_NEAR(state->drop_rect.w, 60);
	CHECK_NEAR(state->drop_rect.h, 2);
	Rdraw_frame();
	CHECK(g_instances > 0); /* the highlight is actually painted */
	CHECK_NEAR(g_outline.rect[0], x + 12);
	CHECK_NEAR(g_outline.rect[2] - g_outline.rect[0], 60);
	Rset_ui_scale(2);
	Rdraw_frame();
	Rwidget_get_rect(target, &x, &y, &w, &h);
	CHECK(Rwindow_inject_drag(window, x + 20, y + 20, R_TRUE));
	Rdraw_frame();
	CHECK_NEAR(g_outline.rect[0], (x + 12) * Rwindow_get_scale(window));
	CHECK_NEAR(g_outline.rect[2] - g_outline.rect[0], 60 * Rwindow_get_scale(window));
	Rset_ui_scale(1);
	Rdraw_frame();
	Rwidget_get_rect(target, &x, &y, &w, &h);
	g_mode = 2;
	CHECK(Rwindow_inject_drag(window, x + 20, y + 20, R_TRUE));
	Rdraw_frame();
	CHECK(g_instances == 0); /* quiet acceptance leaves no old highlight */
	CHECK(Rwindow_inject_drop(window, x + 20, y + 20, NULL, 0, "data"));
	CHECK(g_dropped == 1);
	g_mode = 0;
	CHECK(Rwindow_inject_drag(window, x + 20, y + 20, R_TRUE));
	CHECK_NEAR(state->drop_rect.x, x);
	CHECK_NEAR(state->drop_rect.w, w);
	g_mode = 3;
	CHECK(Rwindow_inject_drag(window, x + 20, y + 20, R_TRUE));
	CHECK_NEAR(state->drop_rect.x, x);
	CHECK_NEAR(state->drop_rect.y, y);
	CHECK_NEAR(state->drop_rect.w, 15);
	CHECK_NEAR(state->drop_rect.h, 10);
	g_mode = 4;
	CHECK(Rwindow_inject_drag(window, x + 20, y + 20, R_TRUE));
	CHECK_NEAR(state->drop_rect.w, 30);
	g_nested_window = Rwindow_create("Nested", 100, 100, 0);
	Rwindow_set_drop_callback(g_nested_window, on_drop, NULL);
	g_mode = 5;
	CHECK(Rwindow_inject_drag(window, x + 20, y + 20, R_TRUE));
	CHECK_NEAR(state->drop_rect.x, x + 5);
	CHECK_NEAR(state->drop_rect.w, 7);
	CHECK_NEAR(rvn_window_get(g_nested_window)->drop_rect.w, 0);
	Rwindow_destroy(g_nested_window);
	g_mode = 7;
	CHECK(!Rwindow_inject_drag(window, x + 20, y + 20, R_TRUE));
	CHECK(!Rwindow_inject_drop(window, x + 20, y + 20, NULL, 0, "refused"));
	CHECK(g_dropped == 1);
	g_mode = 6;
	CHECK(!Rwindow_inject_drag(window, x + 20, y + 20, R_TRUE));
	CHECK(!Rwidget_is_valid(target));
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
	RUN(test_rectangles);
	Rshutdown();
	return test_report("test_drop_highlight");
}
