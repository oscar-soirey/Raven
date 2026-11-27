/**
 * Raven tests - media players and VideoView with the fake media of the tests
 * (Rtest_set_fake_media): the state of a player, its events, the time on the
 * clock of the animations (1/60 s per frame), seeks, the end, loops, the
 * speed, errors, sounds played once; the VideoView, its pictures (only while
 * it plays and is shown), its controls with the mouse and the keyboard, XML,
 * accessibility. Headless: the null platform and the null backend.
 * test_media_system plays real files with the framework of the system.
 */

#include "test.h"

#define W 480.0f
#define H 320.0f

static Rid g_window;

typedef struct events_t {
	int state, loaded, time, ended, error;
	RMediaEvent last;
	Rid player;
	/* R_TRUE: the player is deleted when it ends */
	int destroy_on_end;
} events_t;

static void on_event(Rid player, RMediaEvent event, void* user_data) {
	events_t* e = (events_t*)user_data;
	e->player = player;
	e->last = event;
	switch (event) {
		case R_MEDIA_EVENT_STATE: e->state++; break;
		case R_MEDIA_EVENT_LOADED: e->loaded++; break;
		case R_MEDIA_EVENT_TIME: e->time++; break;
		case R_MEDIA_EVENT_ENDED:
			e->ended++;
			if (e->destroy_on_end) Rmedia_player_destroy(player);
			break;
		case R_MEDIA_EVENT_ERROR: e->error++; break;
	}
}

static void test_player(void) {
	events_t e;
	Rid player = Rmedia_player_create();
	int width = 0, height = 0;
	memset(&e, 0, sizeof(e));
	CHECK(Rmedia_is_supported());
	CHECK_STR(Rmedia_get_backend_name(), "fake");
	CHECK(Rmedia_player_is_valid(player));
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_EMPTY);
	CHECK_STR(Rmedia_player_get_source(player), "");
	CHECK(Rmedia_player_get_texture(player) == R_INVALID_ID);
	Rmedia_player_set_event_callback(player, on_event, &e);

	/* it opens at the next frame, paused at its start */
	CHECK(Rmedia_player_load_file(player, "movie.mp4"));
	CHECK_STR(Rmedia_player_get_source(player), "movie.mp4");
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_LOADING);
	CHECK(e.state == 0);
	Rtest_pump(1);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_PAUSED);
	CHECK(e.loaded == 1);
	CHECK(e.state == 1);
	CHECK(e.player == player);
	CHECK_NEAR(Rmedia_player_get_duration(player), 2.0);
	CHECK(Rmedia_player_get_video_size(player, &width, &height));
	CHECK(width == 160 && height == 120);
	CHECK(Rmedia_player_has_audio(player));
	CHECK_STR(Rmedia_player_get_error(player), "");

	/* the first picture, then nothing new while it does not move */
	CHECK(Rmedia_player_get_texture(player) != R_INVALID_ID);
	CHECK(Rmedia_player_get_frame_count(player) == 1);
	Rmedia_player_get_texture(player);
	CHECK(Rmedia_player_get_frame_count(player) == 1);
	Rtest_pump(5);
	Rmedia_player_get_texture(player);
	CHECK(Rmedia_player_get_frame_count(player) == 1);
	CHECK_NEAR(Rmedia_player_get_position(player), 0.0);

	/* 30 frames of 1/60 s: half a second, and a time event each quarter */
	e.time = 0;
	Rmedia_player_play(player);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_PLAYING);
	Rtest_pump(30);
	CHECK_NEAR(Rmedia_player_get_position(player), 0.5);
	CHECK(e.state == 2);
	CHECK(e.time >= 1 && e.time <= 2);
	{
		/* a picture per 1/30 s of the clip */
		Rid texture = Rmedia_player_get_texture(player);
		int frames = Rmedia_player_get_frame_count(player);
		int tw = 0, th = 0;
		CHECK(frames == 2);
		Rtexture_get_size(texture, &tw, &th);
		CHECK(tw == 160 && th == 120);
		Rtest_pump(2);
		/* the same texture, a new picture */
		CHECK(Rmedia_player_get_texture(player) == texture);
		CHECK(Rmedia_player_get_frame_count(player) == frames + 1);
	}

	/* paused, the time stands */
	Rmedia_player_pause(player);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_PAUSED);
	{
		double at = Rmedia_player_get_position(player);
		Rtest_pump(20);
		CHECK_NEAR(Rmedia_player_get_position(player), at);
	}

	/* a seek is told at the next frame */
	e.time = 0;
	Rmedia_player_seek(player, 1.9);
	CHECK_NEAR(Rmedia_player_get_position(player), 1.9);
	CHECK(e.time == 0);
	Rtest_pump(1);
	CHECK(e.time == 1);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_PAUSED);

	/* to the end */
	Rmedia_player_play(player);
	Rtest_pump(10);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_ENDED);
	CHECK(e.ended == 1);
	CHECK_NEAR(Rmedia_player_get_position(player), 2.0);
	Rtest_pump(10);
	CHECK(e.ended == 1);

	/* played again: from the start */
	Rmedia_player_play(player);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_PLAYING);
	CHECK_NEAR(Rmedia_player_get_position(player), 0.0);
	Rtest_pump(6);
	CHECK_NEAR(Rmedia_player_get_position(player), 0.1);

	/* a seek back from the end leaves it paused there */
	Rmedia_player_seek(player, 2.0);
	Rtest_pump(2);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_ENDED);
	Rmedia_player_seek(player, 0.5);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_PAUSED);

	/* a loop starts again without ending */
	Rmedia_player_set_loop(player, R_TRUE);
	CHECK(Rmedia_player_get_loop(player));
	e.ended = 0;
	Rmedia_player_seek(player, 1.95);
	Rmedia_player_play(player);
	Rtest_pump(12);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_PLAYING);
	CHECK(e.ended == 0);
	CHECK(Rmedia_player_get_position(player) < 0.2);
	Rmedia_player_set_loop(player, R_FALSE);

	/* twice the speed */
	Rmedia_player_set_rate(player, 2.0f);
	CHECK(Rmedia_player_get_rate(player) == 2.0f);
	Rmedia_player_seek(player, 0.0);
	Rtest_pump(30);
	CHECK_NEAR(Rmedia_player_get_position(player), 1.0);
	Rmedia_player_set_rate(player, 100.0f);
	CHECK(Rmedia_player_get_rate(player) == 4.0f);
	Rmedia_player_set_rate(player, 1.0f);

	/* stop: paused at the start */
	Rmedia_player_stop(player);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_PAUSED);
	CHECK_NEAR(Rmedia_player_get_position(player), 0.0);

	/* the sound */
	Rmedia_player_set_volume(player, 0.25f);
	CHECK(Rmedia_player_get_volume(player) == 0.25f);
	Rmedia_player_set_volume(player, 3.0f);
	CHECK(Rmedia_player_get_volume(player) == 1.0f);
	Rmedia_player_set_muted(player, R_TRUE);
	CHECK(Rmedia_player_is_muted(player));

	/* play asked while it loads: it plays as soon as it is opened */
	CHECK(Rmedia_player_load_file(player, "other.mp4"));
	Rmedia_player_play(player);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_LOADING);
	Rtest_pump(1);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_PLAYING);
	/* (the settings stay through a load) */
	CHECK(Rmedia_player_is_muted(player));

	/* a sound has no picture */
	CHECK(Rmedia_player_load_file(player, "voice.WAV"));
	Rtest_pump(1);
	CHECK(!Rmedia_player_get_video_size(player, &width, &height));
	CHECK(width == 0 && height == 0);
	CHECK(Rmedia_player_has_audio(player));
	CHECK(Rmedia_player_get_texture(player) == R_INVALID_ID);

	/* an error comes later, with its event */
	e.error = 0;
	CHECK(Rmedia_player_load_file(player, "invalid.mp4"));
	Rtest_pump(1);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_ERROR);
	CHECK(e.error == 1);
	CHECK(strlen(Rmedia_player_get_error(player)) > 0);
	Rmedia_player_play(player);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_ERROR);

	Rmedia_player_close(player);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_EMPTY);
	CHECK_STR(Rmedia_player_get_source(player), "");
	Rmedia_player_destroy(player);
	CHECK(!Rmedia_player_is_valid(player));

	EXPECT_ERRORS_BEGIN();
	Rmedia_player_play(player);
	CHECK(Rmedia_player_get_state(player) == R_MEDIA_EMPTY);
	Rmedia_player_play(g_window);
	EXPECT_ERRORS_END(3);
}

/* A callback may delete the player it is told about. */
static void test_destroy_in_callback(void) {
	events_t e;
	Rid player = Rmedia_player_create();
	memset(&e, 0, sizeof(e));
	e.destroy_on_end = R_TRUE;
	Rmedia_player_set_event_callback(player, on_event, &e);
	Rmedia_player_load_file(player, "short.mp4");
	Rmedia_player_play(player);
	Rtest_pump(130);
	CHECK(e.ended == 1);
	CHECK(!Rmedia_player_is_valid(player));
	Rtest_pump(2);
}

static void test_sounds(void) {
	Rid a = Rsound_play("click.wav", 0.5f), b = Rsound_play("chime.wav", 1.0f), c;
	CHECK(Rmedia_player_is_valid(a));
	CHECK(Rmedia_player_is_valid(b));
	CHECK(Rmedia_player_get_volume(a) == 0.5f);
	Rtest_pump(2);
	CHECK(Rmedia_player_get_state(a) == R_MEDIA_PLAYING);
	/* gone at their end (2 s of the fake clip) */
	Rtest_pump(125);
	CHECK(!Rmedia_player_is_valid(a));
	CHECK(!Rmedia_player_is_valid(b));
	/* a sound that cannot be played is gone at the next frame */
	c = Rsound_play("invalid.wav", 1.0f);
	CHECK(Rmedia_player_is_valid(c));
	Rtest_pump(1);
	CHECK(!Rmedia_player_is_valid(c));
	/* stopped all at once */
	a = Rsound_play("click.wav", 1.0f);
	b = Rsound_play("click.wav", 1.0f);
	Rtest_pump(1);
	Rsound_stop_all();
	CHECK(!Rmedia_player_is_valid(a));
	CHECK(!Rmedia_player_is_valid(b));
	/* a player of the application is not a sound */
	c = Rmedia_player_create();
	Rsound_stop_all();
	CHECK(Rmedia_player_is_valid(c));
	Rmedia_player_destroy(c);
	EXPECT_ERRORS_BEGIN();
	CHECK(Rsound_play(NULL, 1.0f) == R_INVALID_ID);
	EXPECT_ERRORS_END(1);
}

static int g_full_calls, g_full_last, g_actions;
static void on_fullscreen(Rid view, int fullscreen, void* user_data) {
	(void)view; (void)user_data;
	g_full_calls++;
	g_full_last = fullscreen;
}
static void on_action(Rid sender, const char* action, void* user_data) {
	(void)sender; (void)action; (void)user_data;
	g_actions++;
}

static void test_video_view(void) {
	static const char* xml =
		"<VideoView Name=\"video\" Source=\"clip.mp4\" AutoPlay=\"true\" Controls=\"true\" Muted=\"true\" Volume=\"0.5\" "
		"Stretch=\"Cover\" OnChanged=\"video.changed\"/>";
	Rid view, player, play, seek, time, mute, volume, full, bar;
	int frames;
	float x, y, w, h;

	Raction_bind("video.changed", on_action, NULL);
	view = Rwidget_create_from_xml(xml, strlen(xml));
	CHECK(Rwidget_get_type(view) == R_WIDGET_VIDEO_VIEW);
	player = Rvideo_view_get_player(view);
	CHECK(Rmedia_player_is_valid(player));
	CHECK(player != view);
	/* the functions of the player take the view */
	CHECK(Rmedia_player_is_muted(view));
	CHECK(Rmedia_player_get_volume(view) == 0.5f);
	CHECK(Rvideo_view_get_autoplay(view));
	CHECK(Rvideo_view_get_controls(view));
	CHECK(Rvideo_view_get_stretch(view) == R_STRETCH_COVER);
	CHECK(strstr(Rmedia_player_get_source(view), "clip.mp4") != NULL);
	CHECK(Rmedia_player_get_state(view) == R_MEDIA_LOADING);
	{
		/* the player cannot be deleted apart from its view */
		EXPECT_ERRORS_BEGIN();
		Rmedia_player_destroy(player);
		EXPECT_ERRORS_END(1);
		CHECK(Rmedia_player_is_valid(player));
	}
	Rwindow_set_content(g_window, view);
	g_actions = 0;
	Rtest_pump(2);
	CHECK(Rmedia_player_get_state(view) == R_MEDIA_PLAYING);
	CHECK(g_actions == 1);
	CHECK(Rwidget_get_accessible_role(view) == R_ROLE_GROUP);

	/* the pictures come while it plays and is painted */
	Rtest_pump(30);
	frames = Rmedia_player_get_frame_count(view);
	CHECK(frames >= 10);
	CHECK(!Rtest_is_idle());

	/* the controls */
	bar = Rtest_find(view, "controls");
	play = Rtest_find(view, "controls/play");
	seek = Rtest_find(view, "seek");
	time = Rtest_find(view, "time");
	mute = Rtest_find(view, "mute");
	volume = Rtest_find(view, "volume");
	full = Rtest_find(view, "fullscreen");
	CHECK(bar != R_INVALID_ID && play != R_INVALID_ID && seek != R_INVALID_ID && time != R_INVALID_ID);
	CHECK(mute != R_INVALID_ID && volume != R_INVALID_ID && full != R_INVALID_ID);
	CHECK(Rvideo_view_are_controls_visible(view));
	CHECK(Rtest_is_shown(play));
	/* full screen only when the application handles it */
	CHECK(!Rtest_is_shown(full));
	CHECK_STR(Rwidget_get_accessible_name(play), "Pause");
	Rwidget_get_rect(bar, &x, &y, &w, &h);
	CHECK(w == W);
	CHECK_NEAR(y + h, H);

	/* pause with the button: no more frames are asked for */
	CHECK(Rtest_click(play));
	CHECK(Rmedia_player_get_state(view) == R_MEDIA_PAUSED);
	CHECK(Rtest_settle(2.0));
	frames = Rmedia_player_get_frame_count(view);
	{
		double at = Rmedia_player_get_position(view);
		Rtest_pump(20);
		CHECK(Rmedia_player_get_frame_count(view) == frames);
		CHECK_NEAR(Rmedia_player_get_position(view), at);
	}
	CHECK_STR(Rwidget_get_accessible_name(play), "Play");

	/* the time, and the bar to go anywhere (the clip is 20 s) */
	Rmedia_player_seek(view, 5.0);
	Rtest_pump(1);
	CHECK_STR(Rtest_get_text(time), "0:05 / 0:20");
	Rwidget_get_rect(seek, &x, &y, &w, &h);
	CHECK(Rtest_click_at(seek, w * 0.75f, h * 0.5f, R_MOUSE_LEFT, 1, 0));
	CHECK(Rmedia_player_get_position(view) > 13.0 && Rmedia_player_get_position(view) < 17.0);
	CHECK(Rmedia_player_get_state(view) == R_MEDIA_PAUSED);
	Rtest_pump(1);
	CHECK(strncmp(Rtest_get_text(time), "0:1", 3) == 0);

	/* the sound */
	CHECK(Rtest_click(mute));
	CHECK(!Rmedia_player_is_muted(view));
	CHECK_STR(Rwidget_get_accessible_name(mute), "Mute");
	CHECK(Rtest_click(mute));
	CHECK(Rmedia_player_is_muted(view));
	Rmedia_player_set_muted(view, R_FALSE);
	Rwidget_get_rect(volume, &x, &y, &w, &h);
	CHECK(Rtest_click_at(volume, 1.0f, h * 0.5f, R_MOUSE_LEFT, 1, 0));
	CHECK(Rmedia_player_get_volume(view) < 0.1f);

	/* the keyboard, on the picture */
	Rmedia_player_set_volume(view, 0.5f);
	Rmedia_player_seek(view, 10.0);
	CHECK(Rtest_key(view, R_KEY_SPACE, 0));
	CHECK(Rmedia_player_get_state(view) == R_MEDIA_PLAYING);
	CHECK(Rtest_key(view, R_KEY_K, 0));
	CHECK(Rmedia_player_get_state(view) == R_MEDIA_PAUSED);
	CHECK(Rtest_key(view, R_KEY_RIGHT, 0));
	CHECK_NEAR(Rmedia_player_get_position(view), 15.0);
	CHECK(Rtest_key(view, R_KEY_LEFT, 0));
	CHECK(Rtest_key(view, R_KEY_LEFT, 0));
	CHECK_NEAR(Rmedia_player_get_position(view), 5.0);
	CHECK(Rtest_key(view, R_KEY_END, 0));
	CHECK_NEAR(Rmedia_player_get_position(view), 20.0);
	CHECK(Rtest_key(view, R_KEY_HOME, 0));
	CHECK_NEAR(Rmedia_player_get_position(view), 0.0);
	CHECK(Rtest_key(view, R_KEY_M, 0));
	CHECK(Rmedia_player_is_muted(view));
	CHECK(Rtest_key(view, R_KEY_M, 0));
	CHECK(!Rmedia_player_is_muted(view));
	CHECK(Rtest_key(view, R_KEY_DOWN, 0));
	CHECK(Rmedia_player_get_volume(view) < 0.45f);
	CHECK(Rtest_key(view, R_KEY_UP, 0));
	CHECK(Rtest_key(view, R_KEY_UP, 0));
	CHECK(Rmedia_player_get_volume(view) > 0.55f);

	/* a click on the picture plays and pauses */
	CHECK(Rtest_click_at(view, W * 0.5f, 40.0f, R_MOUSE_LEFT, 1, 0));
	CHECK(Rmedia_player_get_state(view) == R_MEDIA_PLAYING);
	CHECK(Rtest_click_at(view, W * 0.5f, 40.0f, R_MOUSE_LEFT, 1, 0));
	CHECK(Rmedia_player_get_state(view) == R_MEDIA_PAUSED);

	/* full screen, done by the application */
	Rvideo_view_set_fullscreen_callback(view, on_fullscreen, NULL);
	Rtest_pump(1);
	CHECK(Rtest_is_shown(full));
	CHECK(Rtest_click(full));
	CHECK(g_full_calls == 1 && g_full_last == 1);
	CHECK(Rvideo_view_is_fullscreen(view));
	CHECK_STR(Rwidget_get_accessible_name(full), "Exit full screen");
	CHECK(Rtest_key(view, R_KEY_ESCAPE, 0));
	CHECK(g_full_calls == 2 && g_full_last == 0);
	CHECK(!Rvideo_view_is_fullscreen(view));
	Rvideo_view_set_fullscreen(view, R_TRUE);
	CHECK(Rvideo_view_is_fullscreen(view));
	CHECK(g_full_calls == 2);

	/* hidden, a playing view asks for nothing */
	Rmedia_player_play(view);
	Rtest_pump(2);
	Rwidget_set_visibility(view, R_HIDDEN);
	Rtest_pump(2);
	frames = Rmedia_player_get_frame_count(view);
	Rtest_pump(20);
	CHECK(Rmedia_player_get_frame_count(view) == frames);
	CHECK(Rmedia_player_get_state(view) == R_MEDIA_PLAYING);
	Rwidget_set_visibility(view, R_VISIBLE);
	Rtest_pump(2);
	CHECK(Rmedia_player_get_frame_count(view) > frames);

	/* no controls */
	Rvideo_view_set_controls(view, R_FALSE);
	CHECK(Rtest_find(view, "controls") == R_INVALID_ID);
	CHECK(!Rvideo_view_are_controls_visible(view));
	Rtest_pump(2);
	Rvideo_view_set_controls(view, R_TRUE);
	CHECK(Rtest_find(view, "controls/play") != R_INVALID_ID);

	/* the actions: one per change of state */
	g_actions = 0;
	Rmedia_player_pause(view);
	Rtest_pump(1);
	CHECK(g_actions == 1);

	/* nothing else goes in it */
	{
		Rid label = Rlabel_create("x");
		EXPECT_ERRORS_BEGIN();
		CHECK(Rwidget_add_child(view, label) == R_INVALID_ID);
		EXPECT_ERRORS_END(1);
		Rwidget_destroy(label);
	}
	/* (the content replaced is deleted, its player with it) */
	Rwindow_set_content(g_window, R_INVALID_ID);
	CHECK(!Rmedia_player_is_valid(player));
	Raction_unbind("video.changed");
}

static void test_video_view_states(void) {
	Rid view = Rvideo_view_create(), box = Rwidget_create(R_WIDGET_VBOX);
	Rwidget_add_child(box, view);
	Rwindow_set_content(g_window, box);
	Rtest_pump(1);
	CHECK(Rmedia_player_get_state(view) == R_MEDIA_EMPTY);
	/* nothing to play: the button is off */
	Rvideo_view_set_controls(view, R_TRUE);
	Rtest_pump(1);
	CHECK(!Rwidget_is_enabled(Rtest_find(view, "play")));
	/* an error is shown, not played */
	CHECK(Rvideo_view_load_file(view, "invalid.mp4"));
	Rtest_pump(2);
	CHECK(Rmedia_player_get_state(view) == R_MEDIA_ERROR);
	/* a sound in a view: no picture */
	Rvideo_view_set_autoplay(view, R_TRUE);
	CHECK(Rvideo_view_load_url(view, "https://example.org/song.mp3"));
	Rtest_pump(2);
	CHECK(Rmedia_player_get_state(view) == R_MEDIA_PLAYING);
	CHECK(!Rmedia_player_get_video_size(view, NULL, NULL));
	CHECK(Rmedia_player_get_frame_count(view) == 0);
	/* its size is the one of its picture */
	Rvideo_view_load_file(view, "clip.mp4");
	Rtest_pump(2);
	{
		float x, y, w, h;
		Rslot_set_halign(view, R_HALIGN_LEFT);
		Rvideo_view_set_controls(view, R_FALSE);
		Rtest_pump(2);
		Rwidget_get_rect(view, &x, &y, &w, &h);
		CHECK(w == 160.0f && h == 120.0f);
	}
	EXPECT_ERRORS_BEGIN();
	Rvideo_view_set_stretch(view, (RStretch)12345);
	EXPECT_ERRORS_END(1);
	Rwindow_set_content(g_window, R_INVALID_ID);
}

static void test_xml_schema(void) {
	static const char* alias = "<Video Source=\"a.mp4\" Loop=\"true\" Rate=\"1.5\"/>";
	Rid view = Rwidget_create_from_xml(alias, strlen(alias));
	CHECK(Rwidget_get_type(view) == R_WIDGET_VIDEO_VIEW);
	CHECK(Rmedia_player_get_loop(view));
	CHECK(Rmedia_player_get_rate(view) == 1.5f);
	Rwidget_destroy(view);
	CHECK(Rxml_get_tag_count() > 0);
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	g_window = Rwindow_create("video", (int)W, (int)H, R_WINDOW_DEFAULT);
	Rtest_set_deterministic(R_TRUE);

	Rtest_set_fake_media(2.0, 160, 120, 30.0);
	RUN(test_player);
	RUN(test_destroy_in_callback);
	RUN(test_sounds);
	Rtest_set_fake_media(20.0, 160, 120, 30.0);
	RUN(test_video_view);
	RUN(test_video_view_states);
	RUN(test_xml_schema);

	/* a player left playing goes with the library */
	{
		Rid player = Rmedia_player_create();
		Rmedia_player_load_file(player, "left.mp4");
		Rmedia_player_play(player);
		Rsound_play("left.wav", 1.0f);
		Rtest_pump(3);
	}
	Rdraw_frame();
	Rshutdown();
	return test_report("test_video_view");
}
