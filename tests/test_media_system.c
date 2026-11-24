/**
 * Raven tests - media played by the framework of the system: AVFoundation on
 * macOS, Media Foundation on Windows, GStreamer on Linux. Plays the clips of
 * examples/media_player/media (a 2 s H.264 / AAC clip of 160 x 120, WAV
 * sounds): the duration, the size, pictures that arrive and change, a seek,
 * the end, a loop, a sound played once, errors. The sound cannot be heard by
 * a test: its state is checked. Reported as skipped (77) where the system
 * has no media framework Raven can use. Real time, headless (null platform,
 * null backend).
 */

#include "test.h"

#ifndef RAVEN_TEST_MEDIA
#define RAVEN_TEST_MEDIA "examples/media_player/media"
#endif

static char g_path[1024];
/* The system has a media framework but cannot decode the clip (no H.264 decoder installed): skipped. */
static int g_no_decoder;

static const char* media(const char* name) {
	snprintf(g_path, sizeof(g_path), "%s/%s", RAVEN_TEST_MEDIA, name);
	return g_path;
}

typedef struct events_t {
	int state, loaded, time, ended, error;
} events_t;

static void on_event(Rid player, RMediaEvent event, void* user_data) {
	events_t* e = (events_t*)user_data;
	(void)player;
	if (event == R_MEDIA_EVENT_STATE) e->state++;
	else if (event == R_MEDIA_EVENT_LOADED) e->loaded++;
	else if (event == R_MEDIA_EVENT_TIME) e->time++;
	else if (event == R_MEDIA_EVENT_ENDED) e->ended++;
	else if (event == R_MEDIA_EVENT_ERROR) e->error++;
}

/* Runs the loop for up to `seconds` until the player is in `state` (or not in it); the pictures are taken meanwhile. */
static int wait_state(Rid player, RMediaState state, int in_state, double seconds) {
	double end = Rget_time() + seconds;
	while (Rget_time() < end) {
		Rwait_frame(0.01);
		if (!Rmedia_player_is_valid(player)) return R_FALSE;
		Rmedia_player_get_texture(player);
		if ((Rmedia_player_get_state(player) == state) == in_state) return R_TRUE;
	}
	return R_FALSE;
}

static void run_for(Rid player, double seconds) {
	double end = Rget_time() + seconds;
	while (Rget_time() < end) {
		Rwait_frame(0.01);
		if (player != R_INVALID_ID && Rmedia_player_is_valid(player)) Rmedia_player_get_texture(player);
	}
}

static void test_clip(void) {
	events_t e;
	Rid player = Rmedia_player_create(), texture;
	int width = 0, height = 0, frames, tw = 0, th = 0;
	memset(&e, 0, sizeof(e));
	Rmedia_player_set_event_callback(player, on_event, &e);
	CHECK(Rmedia_player_load_file(player, media("clip.mp4")));
	CHECK(wait_state(player, R_MEDIA_LOADING, R_FALSE, 10.0));
	printf("  %s: %.3f s, error \"%s\"\n", Rmedia_get_backend_name(), Rmedia_player_get_duration(player), Rmedia_player_get_error(player));
	if (Rmedia_player_get_state(player) == R_MEDIA_ERROR) {
		g_no_decoder = R_TRUE;
		Rmedia_player_destroy(player);
		return;
	}
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_PAUSED);
	CHECK(e.loaded == 1);
	CHECK(fabs(Rmedia_player_get_duration(player) - 2.0) < 0.1);
	CHECK(Rmedia_player_get_video_size(player, &width, &height));
	CHECK(width == 160 && height == 120);
	CHECK(Rmedia_player_has_audio(player));

	/* playing: pictures arrive in one texture of the size of the clip */
	Rmedia_player_set_volume(player, 0.2f);
	Rmedia_player_play(player);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_PLAYING);
	run_for(player, 0.8);
	frames = Rmedia_player_get_frame_count(player);
	printf("  %d pictures in 0.8 s, at %.3f s\n", frames, Rmedia_player_get_position(player));
	CHECK(frames >= 5);
	texture = Rmedia_player_get_texture(player);
	CHECK(texture != R_INVALID_ID);
	Rtexture_get_size(texture, &tw, &th);
	CHECK(tw == 160 && th == 120);
	CHECK(Rmedia_player_get_position(player) > 0.3);
	CHECK(e.time >= 1);

	/* paused: the time and the pictures stand */
	Rmedia_player_pause(player);
	run_for(player, 0.1);
	{
		double at = Rmedia_player_get_position(player);
		frames = Rmedia_player_get_frame_count(player);
		run_for(player, 0.4);
		CHECK(fabs(Rmedia_player_get_position(player) - at) < 0.05);
		CHECK(Rmedia_player_get_frame_count(player) <= frames + 1);
	}

	/* a seek near the end, then the end */
	Rmedia_player_seek(player, 1.5);
	run_for(player, 0.3);
	printf("  after the seek: %.3f s\n", Rmedia_player_get_position(player));
	CHECK(fabs(Rmedia_player_get_position(player) - 1.5) < 0.1);
	Rmedia_player_play(player);
	CHECK(wait_state(player, R_MEDIA_ENDED, R_TRUE, 5.0));
	CHECK(e.ended == 1);
	CHECK(fabs(Rmedia_player_get_position(player) - Rmedia_player_get_duration(player)) < 0.15);

	/* a loop goes through the end without ending */
	Rmedia_player_set_loop(player, R_TRUE);
	Rmedia_player_seek(player, 1.4);
	Rmedia_player_play(player);
	run_for(player, 1.5);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_PLAYING);
	CHECK(e.ended == 1);
	printf("  looped: at %.3f s\n", Rmedia_player_get_position(player));
	CHECK(Rmedia_player_get_position(player) < 1.4);
	Rmedia_player_destroy(player);
}

static void test_sound(void) {
	events_t e;
	Rid player = Rmedia_player_create(), once;
	memset(&e, 0, sizeof(e));
	Rmedia_player_set_event_callback(player, on_event, &e);
	CHECK(Rmedia_player_load_file(player, media("chime.wav")));
	CHECK(wait_state(player, R_MEDIA_LOADING, R_FALSE, 10.0));
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_PAUSED);
	CHECK(fabs(Rmedia_player_get_duration(player) - 1.2) < 0.1);
	CHECK(!Rmedia_player_get_video_size(player, NULL, NULL));
	CHECK(Rmedia_player_has_audio(player));
	Rmedia_player_set_volume(player, 0.1f);
	Rmedia_player_play(player);
	CHECK(wait_state(player, R_MEDIA_ENDED, R_TRUE, 5.0));
	CHECK(e.ended == 1);
	CHECK(Rmedia_player_get_frame_count(player) == 0);
	Rmedia_player_destroy(player);

	/* once, and forgotten at its end */
	once = Rsound_play(media("click.wav"), 0.1f);
	CHECK(once != R_INVALID_ID);
	CHECK(wait_state(once, R_MEDIA_PLAYING, R_TRUE, 5.0));
	run_for(R_INVALID_ID, 1.0);
	CHECK(!Rmedia_player_is_valid(once));
}

static void test_errors(void) {
	events_t e;
	Rid player = Rmedia_player_create();
	memset(&e, 0, sizeof(e));
	Rmedia_player_set_event_callback(player, on_event, &e);
	/* a file that is not there */
	CHECK(!Rmedia_player_load_file(player, media("missing.mp4")));
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_ERROR);
	CHECK(strstr(Rmedia_player_get_error(player), "missing.mp4") != NULL);
	run_for(player, 0.05);
	CHECK(e.error == 1);
	/* a file that is not media */
	CHECK(Rmedia_player_load_file(player, media("generate.sh")));
	CHECK(wait_state(player, R_MEDIA_LOADING, R_FALSE, 10.0));
	printf("  not media: \"%s\"\n", Rmedia_player_get_error(player));
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_ERROR);
	CHECK(strlen(Rmedia_player_get_error(player)) > 0);
	CHECK(e.error == 2);
	Rmedia_player_destroy(player);
	CHECK(Rsound_play(media("missing.wav"), 1.0f) == R_INVALID_ID);
}

static void test_view(void) {
	Rid window = Rwindow_create("media", 320, 240, R_WINDOW_DEFAULT);
	Rid view = Rvideo_view_create();
	Rvideo_view_set_controls(view, R_TRUE);
	Rvideo_view_set_autoplay(view, R_TRUE);
	Rmedia_player_set_muted(view, R_TRUE);
	Rwindow_set_content(window, view);
	CHECK(Rvideo_view_load_file(view, media("clip.mp4")));
	CHECK(wait_state(view, R_MEDIA_PLAYING, R_TRUE, 10.0));
	run_for(R_INVALID_ID, 0.6);
	/* the pictures were taken by the paint of the view */
	printf("  the view took %d pictures\n", Rmedia_player_get_frame_count(view));
	CHECK(Rmedia_player_get_frame_count(view) >= 5);
	Rwindow_destroy(window);
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	if (!Rmedia_is_supported()) {
		printf("test_media_system: no media framework here, skipped\n");
		Rshutdown();
		return 77;
	}
	printf("media framework: %s\n", Rmedia_get_backend_name());
	RUN(test_clip);
	if (g_no_decoder) {
		printf("test_media_system: %s cannot decode the H.264 / AAC clip here, skipped\n", Rmedia_get_backend_name());
		Rshutdown();
		return 77;
	}
	RUN(test_sound);
	RUN(test_errors);
	RUN(test_view);
	Rdraw_frame();
	Rshutdown();
	return test_report("test_media_system");
}
