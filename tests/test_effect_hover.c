/* Issue #35: an effect under an inset overlay must not lose pointer influence
 * when the overlay becomes the topmost hit target. Inspect real draw commands. */
#include "test.h"
#include "../src/render/renderer.h"

static Rid g_mesh, g_glass;
static uint32_t g_mesh_flags, g_glass_flags;
static int g_mesh_seen, g_glass_seen;

static void observe_frame(const RDrawData* data, RCapture* capture) {
	Ruint i;
	(void)capture;
	g_mesh_seen = g_glass_seen = 0;
	for (i = 0; i < data->command_count; i++) {
		const RDrawCommand* command = &data->commands[i];
		if (command->type != R_DRAW_SHADER || !command->instance_count) continue;
		if (command->shader == g_mesh) {
			g_mesh_flags = data->instances[command->first_instance].flags;
			g_mesh_seen++;
		} else if (command->shader == g_glass) {
			g_glass_flags = data->instances[command->first_instance].flags;
			g_glass_seen++;
		}
	}
}

static void point(Rid window, float x, float y, int mesh, int glass) {
	Rwindow_inject_mouse_move(window, x, y);
	Rdraw_frame();
	CHECK(g_mesh_seen == 1 && g_glass_seen == 1);
	CHECK((int)((g_mesh_flags >> 16) & 255) == mesh);
	CHECK((int)((g_glass_flags >> 16) & 255) == glass);
}

static void test_overlay(float scale) {
	const char* xml =
		"<Window Width=\"400\" Height=\"240\"><Canvas>"
		"<Overlay Slot.Position=\"40,40\" Slot.Size=\"240,140\">"
		"<Border Name=\"mesh\" Effect=\"mesh\" CornerRadius=\"18\"/>"
		"<Border Name=\"glass\" Slot.Padding=\"22\" CornerRadius=\"18\">"
		"<Label Name=\"caption\" Text=\"Overlay\" Slot.VAlign=\"Bottom\"/>"
		"</Border></Overlay></Canvas></Window>";
	Rid window, mesh, caption;
	float x, y, w, h;
	RShaderLook look;
	Rset_ui_scale(scale);
	window = Rwindow_create_from_xml(xml, strlen(xml));
	mesh = Rwidget_find(window, "mesh");
	caption = Rwidget_find(window, "caption");
	g_mesh = Rborder_get_effect(mesh);
	g_glass = Rshader_create_from_file(RAVEN_TEST_LIQUID, R_SHADER_ANIMATED | R_SHADER_BACKDROP);
	CHECK(Rshader_is_valid(g_glass));
	Rborder_set_effect(Rwidget_find(window, "glass"), g_glass, NULL);
	point(window, 10, 100, 0, 0);
	point(window, 50, 100, 255, 0);
	point(window, 80, 100, 255, 255); /* cross from the background into the inset border */
	Rwidget_get_rect(caption, &x, &y, &w, &h);
	point(window, x + w * 0.5f, y + h * 0.5f, 255, 255); /* and then its child */
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rdraw_frame();
	CHECK((g_mesh_flags >> 24) == 255 && (g_glass_flags >> 24) == 255);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	point(window, 50, 100, 255, 0);
	point(window, 10, 100, 0, 0);
	Rwindow_set_size(window, 180, 240);
	point(window, 200, 100, 0, 0); /* inside the rectangle, outside the frame clip */
	Rshader_look_defaults(g_mesh, &look);
	look.hover = 0.4f;
	look.press = 0.2f;
	Rborder_set_effect(mesh, g_mesh, &look);
	point(window, 10, 100, 102, 0); /* explicit strengths still override the pointer */
	CHECK((g_mesh_flags >> 24) == 51);
	CHECK(Rwidget_set_attribute(mesh, "EffectPointer", "false"));
	point(window, 80, 100, 0, 255);
	CHECK((g_mesh_flags & R_INSTANCE_NO_POINTER) != 0);
	Rwindow_destroy(window);
	Rshader_destroy(g_glass);
}

int main(void) {
	RBackendVTable backend = *rvn_backend_null();
	RInitDesc desc = {0};
	desc.backend = R_BACKEND_CUSTOM;
	desc.headless = R_TRUE;
	desc.worker_threads = 1;
	desc.disable_render_thread = R_TRUE;
	backend.RenderFrame = observe_frame;
	Rbackend_register(&backend);
	Rerror_set_callback(test_error_callback);
	if (!Rinit_ex(&desc)) return 1;
	Rset_animations_enabled(R_FALSE);
	test_overlay(1.0f);
	test_overlay(2.0f);
	Rshutdown();
	return test_report("test_effect_hover");
}
