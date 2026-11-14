#include "test.h"
#include "../src/widget/widget.h"
#include "../src/core/accessibility.h"
#include "../src/render/renderer.h"

static Rid g_clicked;
static int g_red_glyphs;

static void on_item(Rid item, void* user) { (void)user; g_clicked = item; }

static void observe_frame(const RDrawData* data, RCapture* capture) {
	Ruint i;
	(void)capture;
	g_red_glyphs = 0;
	for (i = 0; i < data->instance_count; i++) {
		const RDrawInstance* instance = &data->instances[i];
		if ((instance->flags & R_PRIM_MASK) == R_PRIM_GLYPH && instance->color == 0xFF0000FFu) g_red_glyphs++;
	}
}

static void test_decorations(void) {
	static const RLook looks[] = { R_LOOK_RAVEN_DARK, R_LOOK_MACOS_DARK, R_LOOK_WINDOWS_DARK, R_LOOK_GNOME_DARK };
	Rid window = Rwindow_create("Menus", 500, 300, 0);
	Rid bar = Rwidget_create(R_WIDGET_MENU_BAR);
	Rid divider = Rmenu_bar_add_divider(bar);
	Rid menu = Rmenu_add_menu(bar, "Tools");
	Rid header = Rmenu_add_header(menu, "Devices: pdk1");
	Rid first = Rmenu_add_item(menu, "First");
	Rid middle_header = Rmenu_add_header(menu, "Other");
	Rid last = Rmenu_add_item(menu, "Last");
	rvn_a11y_node_t node;
	float row;
	unsigned i;
	Rmenu_bar_add_divider(bar);
	Rwindow_set_content(window, bar);
	Rmenu_bar_set_native(bar, R_FALSE);
	Rmenu_item_set_clicked_callback(header, on_item, NULL);
	Rmenu_item_set_clicked_callback(first, on_item, NULL);
	Rmenu_item_set_clicked_callback(last, on_item, NULL);
	Rmenu_item_set_shortcut(header, R_KEY_F1, 0);
	CHECK(rvn_a11y_describe(header, &node));
	CHECK(node.role == R_ROLE_HEADING && !(node.states & (RVN_A11Y_INVOKABLE | RVN_A11Y_FOCUSABLE)));
	CHECK(!rvn_a11y_act(header, RVN_A11Y_PRESS));
	CHECK(Rwidget_get_accessible_role(divider) == R_ROLE_SEPARATOR);
	for (i = 0; i < sizeof(looks) / sizeof(looks[0]); i++) {
		Rlook_set(looks[i]);
		Rmenu_set_title_color(menu, 1, 0, 0, 1);
		Rdraw_frame();
		CHECK(g_red_glyphs == 5);
		Rmenu_set_title_color(menu, 1, 0, 0, 0);
		Rdraw_frame();
		CHECK(g_red_glyphs == 0);
	}
	Rlook_set(R_LOOK_RAVEN_DARK);
	Rdraw_frame();
	/* A click in the divider never opens it as a popup. */
	test_click_at(window, 8, 10);
	CHECK(!Rwindow_has_popup(window));
	Rmenu_open(menu, window, 0, 40);
	Rdraw_frame();
	row = Rtheme_get_metric(R_METRIC_ROW_HEIGHT) + 2;
	g_clicked = 0;
	test_click_at(window, 30, 40 + 4 + row * 0.5f);
	CHECK(g_clicked == 0 && Rwindow_has_popup(window));
	test_key(window, R_KEY_DOWN, 0);
	test_key(window, R_KEY_ENTER, 0);
	CHECK(g_clicked == first);
	Rmenu_open(menu, window, 0, 40);
	test_key(window, R_KEY_END, 0);
	test_key(window, R_KEY_ENTER, 0);
	CHECK(g_clicked == last);
	Rmenu_open(menu, window, 0, 40);
	test_key(window, R_KEY_HOME, 0);
	test_key(window, R_KEY_DOWN, 0);
	test_key(window, R_KEY_ENTER, 0);
	CHECK(g_clicked == last); /* skips the middle caption */
	g_clicked = 0;
	test_key(window, R_KEY_F1, 0);
	CHECK(g_clicked == 0);
	Rwidget_destroy(first);
	Rwidget_destroy(last);
	Rmenu_open(menu, window, 0, 40);
	test_key(window, R_KEY_DOWN, 0);
	test_key(window, R_KEY_HOME, 0);
	test_key(window, R_KEY_ENTER, 0);
	CHECK(g_clicked == 0); /* captions-only menu */
	CHECK(Rwidget_get_accessible_role(middle_header) == R_ROLE_HEADING);
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
	RUN(test_decorations);
	Rshutdown();
	return test_report("test_menu_decorations");
}
