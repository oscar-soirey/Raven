/**
 * Raven example - media player
 * A VideoView with its controls (media_player.xml), a playlist that goes on
 * to the next item at the end of each one, and the sounds of an interface
 * played once with Rsound_play. The media are in media/ (made by
 * media/generate.sh): two short videos and two sounds. Full screen, from
 * the button of the controls, F or a double click, gives the video the whole
 * window; Escape gives the rest back.
 *
 * For the automated runs (RAVEN_EXAMPLE_FRAMES, RAVEN_EXAMPLE_CAPTURE in
 * common/example.h, or RAVEN_HEADLESS): RAVEN_MEDIA_THEME=light the light
 * look, RAVEN_MEDIA_ITEM=n starts with another item of the playlist.
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static const struct {
	const char* file;
	const char* title;
} g_items[] = {
	{ "clip.mp4", "Test pattern" },
	{ "zoom.mp4", "Mandelbrot zoom" },
	{ "chime.wav", "Chime (sound only)" },
};
#define ITEM_COUNT ((int)(sizeof(g_items) / sizeof(g_items[0])))

static struct {
	Rid window;
	Rid video, playlist, title, status, side, heading, keys, repeat, backend;
	int item;
} app;

static void format_time(char* out, size_t size, double seconds) {
	int total = seconds > 0.0 ? (int)seconds : 0;
	snprintf(out, size, "%d:%02d", total / 60, total % 60);
}

static void show_status(void) {
	static const char* const states[] = { "Nothing loaded", "Loading", "Paused", "Playing", "Ended", "Cannot be played" };
	RMediaState state = Rmedia_player_get_state(app.video);
	char text[256], at[16], total[16];
	int width = 0, height = 0;
	format_time(at, sizeof(at), Rmedia_player_get_position(app.video));
	format_time(total, sizeof(total), Rmedia_player_get_duration(app.video));
	snprintf(text, sizeof(text), "%s  -  %s / %s", states[(int)state - (int)R_MEDIA_EMPTY], at, total);
	if (Rmedia_player_get_video_size(app.video, &width, &height)) {
		size_t length = strlen(text);
		snprintf(text + length, sizeof(text) - length, "  -  %d x %d, %d pictures", width, height, Rmedia_player_get_frame_count(app.video));
	} else if (state == R_MEDIA_ERROR) {
		size_t length = strlen(text);
		snprintf(text + length, sizeof(text) - length, ": %s", Rmedia_player_get_error(app.video));
	}
	Rlabel_set_text(app.status, text);
}

static void play_item(int item) {
	char path[1024];
	app.item = (item % ITEM_COUNT + ITEM_COUNT) % ITEM_COUNT;
	snprintf(path, sizeof(path), "%s/media/%s", RAVEN_EXAMPLE_DIR, g_items[app.item].file);
	Rlabel_set_text(app.title, g_items[app.item].title);
	if (Rlist_view_get_selected(app.playlist) != app.item) Rlist_view_set_selected(app.playlist, app.item);
	/* (AutoPlay is on: it plays once loaded) */
	Rvideo_view_load_file(app.video, path);
	show_status();
}

static void on_media(Rid player, RMediaEvent event, void* user_data) {
	(void)player;
	(void)user_data;
	if (event == R_MEDIA_EVENT_ENDED) {
		/* the next one, or the first again when the playlist repeats */
		if (app.item + 1 < ITEM_COUNT || Rcheckbox_is_checked(app.repeat)) play_item(app.item + 1);
	}
	show_status();
}

static void on_selected(Rid list, int index, void* user_data) {
	(void)list;
	(void)user_data;
	if (index >= 0 && index != app.item) play_item(index);
}

static void on_fullscreen(Rid view, int fullscreen, void* user_data) {
	RVisibility rest = fullscreen ? R_COLLAPSED : R_VISIBLE;
	(void)user_data;
	Rwidget_set_visibility(app.side, rest);
	Rwidget_set_visibility(app.heading, rest);
	Rwidget_set_visibility(app.keys, rest);
	Rwidget_focus(view);
}

static void on_action(Rid sender, const char* action, void* user_data) {
	char path[1024];
	(void)sender;
	(void)user_data;
	if (strcmp(action, "sound.click") == 0 || strcmp(action, "sound.chime") == 0) {
		snprintf(path, sizeof(path), "%s/media/%s", RAVEN_EXAMPLE_DIR, strcmp(action, "sound.click") == 0 ? "click.wav" : "chime.wav");
		Rsound_play(path, 0.8f);
	}
}

int main(void) {
	const char* theme = getenv("RAVEN_MEDIA_THEME");
	const char* item = getenv("RAVEN_MEDIA_ITEM");
	char text[160];
	int i;
	if (!example_init("Raven media player")) return 1;
	Rlook_set(theme && strcmp(theme, "light") == 0 ? R_LOOK_RAVEN_LIGHT : R_LOOK_RAVEN_DARK);
	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/media_player.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.video = Rwidget_find(app.window, "video");
	app.playlist = Rwidget_find(app.window, "playlist");
	app.title = Rwidget_find(app.window, "title");
	app.status = Rwidget_find(app.window, "status");
	app.side = Rwidget_find(app.window, "side");
	app.heading = Rwidget_find(app.window, "heading");
	app.keys = Rwidget_find(app.window, "keys");
	app.repeat = Rwidget_find(app.window, "repeat");
	app.backend = Rwidget_find(app.window, "backend");

	for (i = 0; i < ITEM_COUNT; i++) Rlist_view_add_item(app.playlist, g_items[i].title);
	Rlist_view_set_selected(app.playlist, 0);
	Rlist_view_set_selection_changed_callback(app.playlist, on_selected, NULL);
	Rmedia_player_set_event_callback(app.video, on_media, NULL);
	Rvideo_view_set_fullscreen_callback(app.video, on_fullscreen, NULL);
	Raction_bind("sound.click", on_action, NULL);
	Raction_bind("sound.chime", on_action, NULL);

	if (Rmedia_is_supported()) snprintf(text, sizeof(text), "Played by %s.", Rmedia_get_backend_name());
	else snprintf(text, sizeof(text), "This system has no media framework Raven can use.");
	Rlabel_set_text(app.backend, text);
	/* (the first item is loaded by the XML) */
	if (item && atoi(item) > 0) play_item(atoi(item));
	Rwidget_focus(app.video);
	show_status();

	example_run(app.window, NULL, NULL);
	return 0;
}
