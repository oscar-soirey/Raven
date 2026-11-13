/*
 * Tearing of dock areas: a tab dragged out of the area takes its panel to a
 * window of its own; dragged back over the area, it docks there; a double
 * click and the closing of the window put panels back where they were.
 * Runs headless (null backend): the events are injected.
 */
#include <Raven.h>
#include <stdio.h>
#include <string.h>

static int failures = 0;
#define CHECK(condition, what) do { if (!(condition)) { fprintf(stderr, "FAIL %s (line %d)\n", what, __LINE__); failures++; } else printf("ok   %s\n", what); } while (0)

static void frames(int n) {
	int i;
	for (i = 0; i < n; i++) Rdraw_frame();
}

/* Presses at (x0, y0) in a window, drags in steps to (x1, y1) of that window, releases. */
static void drag(Rid window, float x0, float y0, float x1, float y1) {
	int i;
	Rwindow_inject_mouse_move(window, x0, y0);
	frames(1);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	frames(1);
	for (i = 1; i <= 12; i++) {
		Rwindow_inject_mouse_move(window, x0 + (x1 - x0) * (float)i / 12.0f, y0 + (y1 - y0) * (float)i / 12.0f);
		frames(1);
	}
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	frames(3);
}

/* The same, the pointer going in steps to a point of the screen: the window may move under it (a floating window follows its tab). */
static void drag_to_screen(Rid window, float x0, float y0, float screen_x, float screen_y) {
	int i, wx, wy;
	float start_x, start_y;
	Rwindow_get_position(window, &wx, &wy);
	start_x = (float)wx + x0;
	start_y = (float)wy + y0;
	Rwindow_inject_mouse_move(window, x0, y0);
	frames(1);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	frames(1);
	for (i = 1; i <= 12; i++) {
		float sx = start_x + (screen_x - start_x) * (float)i / 12.0f, sy = start_y + (screen_y - start_y) * (float)i / 12.0f;
		Rwindow_get_position(window, &wx, &wy);
		Rwindow_inject_mouse_move(window, sx - (float)wx, sy - (float)wy);
		frames(1);
	}
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	frames(3);
}

static void double_click(Rid window, float x, float y) {
	Rwindow_inject_mouse_move(window, x, y);
	frames(1);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	frames(3);
}

int main(int argc, char** argv) {
	RInitDesc desc;
	Rid window, dock, files, output, problems, floating_window, floating;
	int wx, wy;
	const char* capture = argc > 1 ? argv[1] : NULL;
	memset(&desc, 0, sizeof(desc));
	desc.backend = capture ? R_BACKEND_VULKAN : R_BACKEND_NULL;
	desc.headless = R_TRUE;
	if (!Rinit_ex(&desc)) return 1;
	Rset_animation_time_step(1.0 / 60.0);

	static const char xml[] =
		"<Window Title=\"Tearing\" Width=\"900\" Height=\"560\">"
		"  <DockArea Name=\"dock\">"
		"    <Label Name=\"editor\" Text=\"Editor\" Slot.Title=\"Editor\"/>"
		"    <Label Name=\"files\" Text=\"Files\" Slot.Title=\"Files\" Slot.Dock=\"Left\" Slot.DockTo=\"editor\" Slot.DockSize=\"0.3\"/>"
		"    <Label Name=\"output\" Text=\"Output\" Slot.Title=\"Output\" Slot.Dock=\"Bottom\" Slot.DockTo=\"editor\" Slot.DockSize=\"0.35\"/>"
		"    <Label Name=\"problems\" Text=\"Problems\" Slot.Title=\"Problems\" Slot.Dock=\"Tab\" Slot.DockTo=\"output\"/>"
		"  </DockArea>"
		"</Window>";
	window = Rwindow_create_from_xml(xml, sizeof(xml) - 1);
	CHECK(window != R_INVALID_ID, "the window is made");
	Rwindow_set_position(window, 100, 100);
	dock = Rwidget_find(window, "dock");
	files = Rwidget_find(window, "files");
	output = Rwidget_find(window, "output");
	problems = Rwidget_find(window, "problems");
	frames(10);
	CHECK(Rdock_area_is_tearable(dock), "a dock area is tearable by default");

	/* 1. the tab Files dragged far below the window: a window of its own */
	drag(window, 40.0f, 14.0f, 450.0f, 760.0f);
	floating_window = Rdock_area_get_panel_window(dock, files);
	CHECK(floating_window != R_INVALID_ID, "the dragged tab took Files to a window of its own");
	CHECK(Rwidget_get_parent(files) != dock, "Files is no longer a child of the area");
	Rwindow_get_position(floating_window, &wx, &wy);
	CHECK(wx > 300 && wy > 700, "the window opened where the tab was dropped");
	{
		int fw = 0, fh = 0;
		Rwindow_get_size(floating_window, &fw, &fh);
		printf("     floating window %d x %d\n", fw, fh);
		CHECK(fw >= 250 && fw <= 300 && fh >= 500, "the window has the size the panel had in the area");
	}
	CHECK(Rdock_area_is_panel_open(dock, output) != 0, "the other panels stayed");
	if (capture) {
		frames(20);
		{
			char path[512];
			int w = 0, h = 0, i;
			uint8_t* pixels;
			FILE* file;
			snprintf(path, sizeof(path), "%s_main.ppm", capture);
			pixels = Rwindow_capture(window, &w, &h);
			file = pixels ? fopen(path, "wb") : NULL;
			if (file) { fprintf(file, "P6\n%d %d\n255\n", w, h); for (i = 0; i < w * h; i++) fwrite(pixels + i * 4, 1, 3, file); fclose(file); }
			Rpixels_free(pixels);
			snprintf(path, sizeof(path), "%s_float.ppm", capture);
			pixels = Rwindow_capture(floating_window, &w, &h);
			file = pixels ? fopen(path, "wb") : NULL;
			if (file) { fprintf(file, "P6\n%d %d\n255\n", w, h); for (i = 0; i < w * h; i++) fwrite(pixels + i * 4, 1, 3, file); fclose(file); }
			Rpixels_free(pixels);
		}
	}
	/* the functions of the area work on its floating panel */
	Rdock_area_set_panel_title(dock, files, "Explorer");
	CHECK(Rdock_area_is_panel_open(dock, files), "the area still knows its floating panel");

	/* 2. the floating tab dragged over the middle of the main window: docked there */
	floating = Rwidget_get_parent(files);
	/* from its tab to the middle of the editor of the main window (at 100, 100 on the screen) */
	drag_to_screen(floating_window, 30.0f, 14.0f, 100.0f + 600.0f, 100.0f + 160.0f);
	frames(4);
	CHECK(Rwidget_get_parent(files) == dock, "dropped over the main window, Files is docked there again");
	CHECK(!Rwindow_is_valid(floating_window), "the empty floating window closed");
	(void)floating;

	/* 3. Rdock_area_float_panel, then a double click on its tab: back where it was */
	floating_window = Rdock_area_float_panel(dock, problems, 1200, 300);
	frames(4);
	CHECK(floating_window != R_INVALID_ID && Rwidget_get_parent(problems) != dock, "Rdock_area_float_panel");
	double_click(floating_window, 30.0f, 14.0f);
	CHECK(Rwidget_get_parent(problems) == dock, "a double click on the floating tab docks it back");
	CHECK(!Rwindow_is_valid(floating_window), "and its window closes");

	/* 4. closing a floating window puts its panel back */
	floating_window = Rdock_area_float_panel(dock, output, 1200, 300);
	frames(4);
	Rwindow_close(floating_window);
	frames(4);
	CHECK(Rwidget_get_parent(output) == dock, "closing the floating window docks its panel back");

	/* 5. Tearable="false": a tab dragged out stays */
	Rdock_area_set_tearable(dock, R_FALSE);
	drag(window, 40.0f, 14.0f, 450.0f, 760.0f);
	CHECK(Rwidget_get_parent(files) == dock, "an area that is not tearable keeps its panels");

	Rshutdown();
	printf(failures ? "%d failure(s)\n" : "all good\n", failures);
	return failures ? 1 : 0;
}
