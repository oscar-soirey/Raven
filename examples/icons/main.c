/**
 * Raven example - icons
 * The icons shipped with Raven (Ricon_get), every one of them in a gallery
 * built from C, and some in buttons, tabs and a tree (icons.xml, written
 * Source="raven:save"). They are drawn in the color of the text around them:
 * the button at the top right switches between the dark and the light theme,
 * and the icons follow, as they follow hover, pressed and disabled.
 *
 * RAVEN_ICONS_LIGHT=1 starts in the light theme, and RAVEN_ICONS_HOVER=1 puts
 * the pointer on the second button, for the automated runs that capture it
 * (RAVEN_EXAMPLE_FRAMES, RAVEN_EXAMPLE_CAPTURE in common/example.h).
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static struct {
	Rid window;
	int light;
} app;

/* The button of the theme names the other one. */
static void show_theme(void) {
	Rimage_set_texture(Rwidget_find(app.window, "theme_icon"), Ricon_get(app.light ? "moon" : "sun"));
	Rlabel_set_text(Rwidget_find(app.window, "theme_text"), app.light ? "Dark" : "Light");
}

static void on_theme(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	app.light = !app.light;
	Rtheme_apply(app.light ? R_THEME_LIGHT : R_THEME_DARK);
	show_theme();
}

/* One tile per icon: the icon, its name under it. */
static void fill_gallery(Rid gallery) {
	int i, count = Ricon_get_count();
	for (i = 0; i < count; i++) {
		const char* name = Ricon_get_name(i);
		Rid tile = Rwidget_add_child(gallery, Rwidget_create(R_WIDGET_VBOX));
		Rid image = Rwidget_add_child(tile, Rwidget_create(R_WIDGET_IMAGE));
		Rid label = Rwidget_add_child(tile, Rlabel_create(name));
		Rwidget_set_size(tile, 96, 64);
		Rimage_set_texture(image, Ricon_get(name));
		Rwidget_set_size(image, 24, 24);
		Rslot_set_padding(image, 0, 8, 0, 6);
		Rslot_set_halign(image, R_HALIGN_CENTER);
		Rslot_set_halign(label, R_HALIGN_CENTER);
		Rlabel_set_text_size(label, 11);
		Rwidget_set_opacity(label, 0.7f);
		Rwidget_set_tooltip(tile, name);
	}
}

/* RAVEN_ICONS_HOVER: the pointer on the second button, once the window is laid out. */
static void hover(double delta, void* user_data) {
	static int frame;
	float x, y, w, h;
	(void)delta;
	(void)user_data;
	if (++frame != 3) return;
	Rwidget_get_rect(Rwidget_find(app.window, "hovered"), &x, &y, &w, &h);
	Rwindow_inject_mouse_move(app.window, x + w * 0.5f, y + h * 0.5f);
}

int main(void) {
	if (!example_init("Raven icons")) return 1;
	app.light = getenv("RAVEN_ICONS_LIGHT") != NULL;
	Rtheme_apply(app.light ? R_THEME_LIGHT : R_THEME_DARK);
	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/icons.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	fill_gallery(Rwidget_find(app.window, "gallery"));
	Raction_bind("icons.theme", on_theme, NULL);
	show_theme();
	if (getenv("RAVEN_ICONS_HOVER")) Rset_frame_callback(hover, NULL);

	example_run(app.window, NULL, NULL);
	return 0;
}
