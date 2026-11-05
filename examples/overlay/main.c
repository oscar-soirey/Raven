/**
 * Raven example - overlay
 * A window that is seen through, and the ways to blur what is behind.
 *
 *   <Window Transparent="true" Backdrop="Blur">   or R_WINDOW_TRANSPARENT + Rwindow_set_backdrop
 *   Rwindow_set_opacity                           the whole window fades
 *   <BackgroundBlur Style="Native|Plain|Frosted|Glass">
 *
 * Whether the desktop shows, and blurred or not, is up to the window
 * system: Windows 10 and 11 do it (Mica and Acrylic need Windows 11), X11
 * needs a compositor (and KWin for the blur). Rwindow_get_backdrop tells
 * what was obtained; the example shows it.
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static struct {
	Rid window, result, what;
} app;

static const char* backdrop_name(RBackdrop backdrop) {
	switch (backdrop) {
		case R_BACKDROP_TRANSPARENT: return "the desktop, as it is";
		case R_BACKDROP_BLUR: return "the desktop, blurred";
		case R_BACKDROP_ACRYLIC: return "acrylic";
		case R_BACKDROP_MICA: return "mica";
		default: return "nothing (not available here)";
	}
}

static void show_result(void) {
	char text[200];
	RNativeWindow native;
	snprintf(text, sizeof(text), "obtained: %s", backdrop_name(Rwindow_get_backdrop(app.window)));
	Rlabel_set_text(app.result, text);
	memset(&native, 0, sizeof(native));
	Rwindow_get_native_handle(app.window, &native);
	Rlabel_set_text(app.what, native.transparent
		? "Where this window draws nothing, what is behind it shows. Drag the panel to move the window."
		: "This window system did not give a window that can be seen through: the same interface, on a plain background.");
}

static void action_backdrop(Rid sender, const char* action, void* user_data) {
	static const RBackdrop backdrops[] = { R_BACKDROP_TRANSPARENT, R_BACKDROP_BLUR, R_BACKDROP_ACRYLIC, R_BACKDROP_MICA };
	int index = Rcombo_box_get_selected(sender);
	(void)action;
	(void)user_data;
	/* (when the system cannot do what is asked, the window stays as it was) */
	if (index >= 0 && index < 4) Rwindow_set_backdrop(app.window, backdrops[index]);
	show_result();
}

static void action_opacity(Rid sender, const char* action, void* user_data) {
	(void)action;
	(void)user_data;
	/* when the system refuses, the slider goes back to what the window has */
	if (!Rwindow_set_opacity(app.window, Rslider_get_value(sender))) Rslider_set_value(sender, Rwindow_get_opacity(app.window));
}

static void action_close(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	Rquit(0);
}

int main(void) {
	Rid stripes;
	int i;
	if (!example_init("Raven overlay")) return 1;
	Rtheme_apply(R_THEME_DARK);

	Raction_bind("overlay.backdrop", action_backdrop, NULL);
	Raction_bind("overlay.opacity", action_opacity, NULL);
	Raction_bind("overlay.close", action_close, NULL);

	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/overlay.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.result = Rwidget_find(app.window, "backdrop.result");
	app.what = Rwidget_find(app.window, "what");
	/* something sharp to blur: stripes of color */
	stripes = Rwidget_find(app.window, "stripes");
	for (i = 0; i < 40; i++) {
		static const float colors[4][3] = { { 1.0f, 0.27f, 0.23f }, { 1.0f, 0.84f, 0.04f }, { 0.19f, 0.82f, 0.35f }, { 0.04f, 0.52f, 1.0f } };
		Rid stripe = Rwidget_add_child(stripes, Rwidget_create(R_WIDGET_BORDER));
		Rwidget_set_size(stripe, 16.0f, -1.0f);
		Rborder_set_background(stripe, colors[i % 4][0], colors[i % 4][1], colors[i % 4][2], 1.0f);
	}
	show_result();
	Rwindow_show(app.window);

	if (!example_automated(app.window)) {
		while (Rwait_frame(-1.0)) {
		}
	}
	Rshutdown();
	return Rget_exit_code();
}
