/**
 * Raven example - carousel
 * A Carousel: an onboarding in five pages (carousel.xml). The pages are
 * swiped with the mouse, a finger or two fingers on a trackpad, turned with
 * the arrow keys, the dots, the arrows shown under the pointer, and the Back
 * and Next buttons below; the switches turn the loop, the autoplay and the
 * vertical orientation on.
 *
 * RAVEN_CAROUSEL_SWIPE=1 drives the pages by itself with injected input (a
 * drag, a click on a dot, a drag let go early), for the automated runs that
 * record it (RAVEN_EXAMPLE_FRAMES, RAVEN_EXAMPLE_FILM in common/example.h).
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static struct {
	Rid window;
	Rid pages, status, back, next;
	int frame;
} app;

/* The page shown, below the pages: its title, its number; the buttons for what comes before and after. */
static void show_page(void) {
	static const char* const titles[] = { "Welcome", "Swipe", "Pages", "Dots", "Ready" };
	char text[128];
	int page = Rcarousel_get_page(app.pages), count = Rcarousel_get_page_count(app.pages);
	int loop = Rcarousel_get_loop(app.pages);
	snprintf(text, sizeof(text), "%s  -  %d / %d", page >= 0 && page < 5 ? titles[page] : "", page + 1, count);
	Rlabel_set_text(app.status, text);
	Rwidget_set_enabled(app.back, loop || page > 0);
	Rbutton_set_text(app.next, page == count - 1 && !loop ? "Get started" : "Next");
}

static void on_action(Rid sender, const char* action, void* user_data) {
	(void)user_data;
	if (strcmp(action, "carousel.loop") == 0) {
		Rcarousel_set_loop(app.pages, Rcheckbox_is_checked(sender));
	} else if (strcmp(action, "carousel.autoplay") == 0) {
		Rcarousel_set_autoplay(app.pages, Rcheckbox_is_checked(sender) ? 3.0 : 0.0);
	} else if (strcmp(action, "carousel.vertical") == 0) {
		Rcarousel_set_orientation(app.pages, Rcheckbox_is_checked(sender) ? R_VERTICAL : R_HORIZONTAL);
	} else if (strcmp(action, "carousel.back") == 0) {
		Rcarousel_prev(app.pages);
	} else if (strcmp(action, "carousel.next") == 0) {
		/* the last page: the application would go on to its own screens */
		if (Rcarousel_get_page(app.pages) == Rcarousel_get_page_count(app.pages) - 1 && !Rcarousel_get_loop(app.pages))
			Rcarousel_set_page(app.pages, 0, R_TRUE);
		else
			Rcarousel_next(app.pages);
	}
	show_page();
}

/* ---------------------------------------------------------------------------
 *  RAVEN_CAROUSEL_SWIPE: input injected frame after frame, as a user would give it
 * ------------------------------------------------------------------------- */

static void pages_point(float fx, float fy, float* x, float* y) {
	float px, py, pw, ph;
	Rwidget_get_rect(app.pages, &px, &py, &pw, &ph);
	*x = px + pw * fx;
	*y = py + ph * fy;
}

/*
 * Drags from fx0 to fx1 (fractions of the width of the pages) between two
 * frames, at 70% of their height: eased as a hand that stops before it lets
 * go, or at full speed to the end (a flick).
 */
static void scripted_drag(int frame, int first, int last, float fx0, float fx1, int flick) {
	float x, y, t;
	if (frame < first || frame > last + 1) return;
	t = (float)(frame - first) / (float)(last + 1 - first);
	if (!flick) t = t * t * (3.0f - 2.0f * t);
	pages_point(fx0 + (fx1 - fx0) * t, 0.7f, &x, &y);
	Rwindow_inject_mouse_move(app.window, x, y);
	if (frame == first) Rwindow_inject_mouse_button(app.window, R_MOUSE_LEFT, R_TRUE);
	if (frame == last + 1) Rwindow_inject_mouse_button(app.window, R_MOUSE_LEFT, R_FALSE);
}

static void scripted_frame(double delta, void* user_data) {
	float x, y, dx, dy, dw, dh;
	int frame = app.frame++;
	(void)delta;
	(void)user_data;
	/* 180 frames a second of animation: a film (RAVEN_EXAMPLE_FILM keeps one frame in six) plays at 30 frames a second */
	if (frame == 0) Rset_animation_time_step(1.0 / 180.0);
	/* the pointer comes on the pages: the arrows show */
	if (frame == 12) {
		pages_point(0.6f, 0.7f, &x, &y);
		Rwindow_inject_mouse_move(app.window, x, y);
	}
	/* a drag to the next page */
	scripted_drag(frame, 60, 135, 0.78f, 0.22f, R_FALSE);
	/* a click on the fourth dot */
	if (frame == 300) {
		Rwidget_get_rect(app.pages, &dx, &dy, &dw, &dh);
		/* (five dots 18 apart, the capsule centered 12 + 11 above the bottom edge) */
		x = dx + dw * 0.5f + 18.0f;
		y = dy + dh - 23.0f;
		Rwindow_inject_mouse_move(app.window, x, y);
		Rwindow_inject_mouse_button(app.window, R_MOUSE_LEFT, R_TRUE);
		Rwindow_inject_mouse_button(app.window, R_MOUSE_LEFT, R_FALSE);
	}
	/* a drag let go before half a page: it comes back */
	scripted_drag(frame, 430, 490, 0.7f, 0.45f, R_FALSE);
	/* and the last page, flicked */
	scripted_drag(frame, 620, 638, 0.72f, 0.45f, R_TRUE);
}

int main(void) {
	if (!example_init("Raven carousel")) return 1;
	Rtheme_apply(R_THEME_DARK);
	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/carousel.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.pages = Rwidget_find(app.window, "pages");
	app.status = Rwidget_find(app.window, "status");
	app.back = Rwidget_find(app.window, "back");
	app.next = Rwidget_find(app.window, "next");
	/* OnChanged="carousel.changed": the user or the autoplay went to another page */
	Raction_bind("carousel.changed", on_action, NULL);
	Raction_bind("carousel.loop", on_action, NULL);
	Raction_bind("carousel.autoplay", on_action, NULL);
	Raction_bind("carousel.vertical", on_action, NULL);
	Raction_bind("carousel.back", on_action, NULL);
	Raction_bind("carousel.next", on_action, NULL);
	show_page();
	if (getenv("RAVEN_CAROUSEL_SWIPE")) Rset_frame_callback(scripted_frame, NULL);

	example_run(app.window, NULL, NULL);
	return 0;
}
