/*
 * The mobile showcase (../shared) in a desktop window: to work on its UI
 * without a phone, and to see what the desktops answer in the Device tab
 * (file dialogs, open URL, clipboard, images from the network; no camera,
 * share sheet nor haptics). Built with the other examples: raven_mobile.
 *
 *   raven_mobile           the window
 *   raven_mobile --check   no window: opens the Device tab, copies and
 *                          pastes, downloads the image of the network tab,
 *                          prints what it shows, and quits
 */
#include "../../common/example.h"
#include "showcase.h"

#if defined(_WIN32)
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

/* The font of the titles: the phone build scripts copy it into the package, here it is read from the repository. */
static void register_title_font(void) {
	size_t size = 0;
	char* data = Rfile_read(RAVEN_EXAMPLE_DIR "/../../tools/designer/fonts/BricolageGrotesque-Bold-subset.ttf", &size);
	if (data) Rfile_register_memory("fonts/BricolageGrotesque-Bold-subset.ttf", data, size, R_TRUE);
	Rmemory_free(data);
}

static void click(Rid window, const char* name) {
	float x, y, w, h;
	Rid widget = Rwidget_find(Rwindow_get_content(window), name);
	Rwidget_scroll_into_view(widget);
	Rdraw_frame();
	Rwidget_get_rect(widget, &x, &y, &w, &h);
	Rwindow_inject_mouse_move(window, x + w * 0.5f, y + h * 0.5f);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	Rdraw_frame();
}

static void print_label(Rid window, const char* name) {
	printf("%s: %s\n", name, Rlabel_get_text(Rwidget_find(Rwindow_get_content(window), name)));
}

/* The Device tab without a window or a GPU (the null platform): what a desktop with no services answers. */
static int check(void) {
	RInitDesc desc;
	Rid window;
	double start;
	int i;
	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_NULL;
	desc.headless = R_TRUE;
	Rerror_set_callback(example_on_error);
	if (!Rinit_ex(&desc)) return 1;
	register_title_font();
	window = showcase_create();
	if (window == R_INVALID_ID) return 1;
	Rdraw_frame();
	click(window, "tab4");
	click(window, "haptic_light");
	print_label(window, "dev_status");
	click(window, "take_photo");
	for (i = 0; i < 3; i++) Rdraw_frame();
	print_label(window, "dev_status");
	click(window, "copy");
	click(window, "paste");
	print_label(window, "clip_info");
	print_label(window, "perm_camera");
	print_label(window, "perm_notify");
	click(window, "load_url");
	start = Rget_time();
	while (Rget_time() - start < 25.0 && strncmp(Rlabel_get_text(Rwidget_find(Rwindow_get_content(window), "net_info")), "Downloading", 11) == 0)
		Rwait_frame(0.05);
	print_label(window, "net_info");
	Rshutdown();
	return 0;
}

int main(int argc, char** argv) {
	Rid window;
	/* showcase.xml and the picture: relative paths, as in the package of the phones */
	if (chdir(RAVEN_EXAMPLE_DIR "/shared/assets") != 0) {
		fprintf(stderr, "cannot find %s\n", RAVEN_EXAMPLE_DIR "/shared/assets");
		return 1;
	}
	if (argc > 1 && strcmp(argv[1], "--check") == 0) return check();
	if (!example_init("Raven Showcase")) return 1;
	Rtheme_follow_system(R_TRUE);
	register_title_font();
	window = showcase_create();
	if (window == R_INVALID_ID) return 1;
	example_run(window, NULL, NULL);
	return 0;
}
