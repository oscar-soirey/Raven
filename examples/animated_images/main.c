/**
 * Raven example - animated images
 * Animated GIF and APNG files shown by Image widgets (animated_images.xml):
 * a spinner (GIF with a transparent color), a bouncing ball (APNG with an
 * alpha channel) and a countdown that plays once and fires its action. The
 * buttons play, pause and stop them all; the slider shows a frame of the
 * ball; the switch turns the animations off, as reduced motion does.
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static struct {
	Rid window;
	Rid images[3];
	Rid ball_info, countdown_info, seek;
	int seeking;
} app;

static void show_state(void) {
	char text[96];
	snprintf(text, sizeof(text), "ball.png  -  %d frames, %.2f s", Rimage_get_frame_count(app.images[1]), Rimage_get_duration(app.images[1]));
	Rlabel_set_text(app.ball_info, text);
}

static void on_action(Rid sender, const char* action, void* user_data) {
	int i;
	(void)user_data;
	if (strcmp(action, "anim.play") == 0) {
		for (i = 0; i < 3; i++) Rimage_play(app.images[i]);
	} else if (strcmp(action, "anim.pause") == 0) {
		for (i = 0; i < 3; i++) Rimage_pause(app.images[i]);
	} else if (strcmp(action, "anim.stop") == 0) {
		for (i = 0; i < 3; i++) Rimage_stop(app.images[i]);
	} else if (strcmp(action, "anim.replay") == 0) {
		Rimage_stop(app.images[2]);
		Rimage_play(app.images[2]);
		Rlabel_set_text(app.countdown_info, "countdown.gif");
	} else if (strcmp(action, "anim.finished") == 0) {
		/* Action="anim.finished": the countdown played its last time */
		Rlabel_set_text(app.countdown_info, "countdown.gif  -  done");
	} else if (strcmp(action, "anim.reduce") == 0) {
		Rset_animations_enabled(!Rcheckbox_is_checked(sender));
	}
}

/* The slider shows a frame of the ball, which stops there. */
static void on_seek(Rid slider, float value, void* user_data) {
	(void)slider;
	(void)user_data;
	Rimage_pause(app.images[1]);
	Rimage_set_frame(app.images[1], (int)(value + 0.5f));
}

int main(void) {
	static const char* const names[3] = { "spinner", "ball", "countdown" };
	int i;
	if (!example_init("Raven animated images")) return 1;
	Rtheme_apply(R_THEME_DARK);
	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/animated_images.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	for (i = 0; i < 3; i++) app.images[i] = Rwidget_find(app.window, names[i]);
	app.ball_info = Rwidget_find(app.window, "ball.info");
	app.countdown_info = Rwidget_find(app.window, "countdown.info");
	app.seek = Rwidget_find(app.window, "seek");
	Rslider_set_range(app.seek, 0.0f, (float)(Rimage_get_frame_count(app.images[1]) - 1));
	Rslider_set_changed_callback(app.seek, on_seek, NULL);
	Raction_bind("anim.play", on_action, NULL);
	Raction_bind("anim.pause", on_action, NULL);
	Raction_bind("anim.stop", on_action, NULL);
	Raction_bind("anim.replay", on_action, NULL);
	Raction_bind("anim.finished", on_action, NULL);
	Raction_bind("anim.reduce", on_action, NULL);
	show_state();

	example_run(app.window, NULL, NULL);
	return 0;
}
