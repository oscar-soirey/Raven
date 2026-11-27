/**
 * Raven - media/media.c
 * Media players (raven_controls.h, VIDEO AND SOUND): what the application
 * sees of a stream that a backend plays (media.h), sounds played once
 * (Rsound_play), and the fake backend of the tests (Rtest_set_fake_media).
 *
 * The state of a player changes when it is told (play, pause, a load) and
 * when its backend says so; both are seen by rvn_media_update, once per frame,
 * which compares what the player is with what it last told and sends the
 * events: the callbacks never run inside a call of the application, and a
 * callback may delete the player it is told about. While a player loads or
 * plays, a timer keeps the loop of the application awake (a sound plays
 * without anything to paint); a paused player costs nothing.
 *
 * The pictures are taken only when asked for (rvn_media_player_frame, by the
 * VideoView that paints, or Rmedia_player_get_texture): a video no one sees
 * decodes no picture into a texture.
 */

#include "media.h"
#include "../core/context.h"
#include "../core/handle.h"
#include "../core/fs.h"
#include "../render/renderer.h"
#include "../widget/widget.h"
#include "../platform/platform.h"

#include <math.h>
#include <string.h>

/* Seconds of playback between two R_MEDIA_EVENT_TIME. */
#define RVN_MEDIA_TIME_STEP 0.25
/* How often the loop wakes while a player loads or plays, without anything to paint. */
#define RVN_MEDIA_WAKE 0.03

typedef struct rvn_media_player_t {
	Rid id;
	const rvn_media_backend_t* backend;
	void* stream;
	char* source;
	char* error;
	RMediaState state;
	/** Play as soon as it is opened. */
	int want_play;
	int loaded;
	double duration, position;
	int width, height, has_video, has_audio;
	float volume;
	int muted, loop;
	float rate;
	Rid texture;
	int texture_width, texture_height;
	int frame_count;
	/* what was last told (rvn_media_update) */
	RMediaState told_state;
	int told_loaded;
	double told_position;
	/** A seek: its position is told even if it is close. */
	int seeked;
	/** The VideoView that shows it, or R_INVALID_ID. */
	Rid view;
	RMediaEventCallback callback;
	void* user_data;
	/** Rsound_play: it deletes itself at its end. */
	int sound;
	/** The player of a VideoView: Rmedia_player_destroy does not take it. */
	int internal;
} rvn_media_player_t;

static struct {
	int ready;
	rvn_pool_t pool;
	double last_clock;
	/** The real time the last frame was given, while a real player keeps the deterministic clock in step. */
	double paced_clock;
	Rid timer;
	int system_tried;
	const rvn_media_backend_t* system;
	/* Rtest_set_fake_media */
	double fake_duration, fake_fps;
	int fake_width, fake_height;
} g_media;

static const rvn_media_backend_t g_rvn_fake_media;

#if !defined(RAVEN_MEDIA_AVFOUNDATION) && !defined(RAVEN_MEDIA_MEDIA_FOUNDATION) && !defined(RAVEN_MEDIA_GSTREAMER)
/* (the browser, Android, the null platform elsewhere: nothing plays) */
const rvn_media_backend_t* rvn_media_system_backend(void) {
	return NULL;
}
#endif

/* ---------------------------------------------------------------------------
 *  Helpers
 * ------------------------------------------------------------------------- */

void rvn_media_bgra_to_rgba(uint8_t* to, const uint8_t* from, int width, int height, size_t stride) {
	int x, y;
	for (y = 0; y < height; y++) {
		const uint8_t* source = from + (size_t)y * stride;
		uint8_t* target = to + (size_t)y * (size_t)width * 4;
		for (x = 0; x < width; x++, source += 4, target += 4) {
			target[0] = source[2];
			target[1] = source[1];
			target[2] = source[0];
			/* (the pictures of a video are opaque: some decoders leave this byte at 0) */
			target[3] = 255;
		}
	}
}

int rvn_media_platform_pumps(void) {
	return g_rvn.platform && g_rvn.platform != rvn_platform_null();
}

static const rvn_media_backend_t* rvn_media_backend(void) {
	if (g_media.fake_duration > 0.0) return &g_rvn_fake_media;
	if (!g_media.system_tried) {
		const rvn_media_backend_t* backend = rvn_media_system_backend();
		g_media.system_tried = R_TRUE;
		g_media.system = backend && backend->available() ? backend : NULL;
	}
	return g_media.system;
}

static void rvn_media_init(void) {
	if (g_media.ready) return;
	rvn_pool_init(&g_media.pool, RVN_HANDLE_MEDIA_PLAYER);
	g_media.timer = R_INVALID_ID;
	g_media.last_clock = rvn_clock_seconds();
	g_media.ready = R_TRUE;
}

static rvn_media_player_t* rvn_media_get(Rid id) {
	rvn_widget_t* widget;
	if (!g_media.ready) return NULL;
	if (rvn_handle_type(id) == RVN_HANDLE_MEDIA_PLAYER) return (rvn_media_player_t*)rvn_pool_get(&g_media.pool, id);
	/* a VideoView stands for its player */
	widget = rvn_handle_type(id) == RVN_HANDLE_WIDGET ? rvn_widget_get(id) : NULL;
	if (widget && widget->cls->type == R_WIDGET_VIDEO_VIEW) return (rvn_media_player_t*)rvn_pool_get(&g_media.pool, rvn_video_view_player_of(widget));
	return NULL;
}

static rvn_media_player_t* rvn_media_check(Rid id, const char* function) {
	rvn_media_player_t* player;
	if (!rvn_api_check(function)) return NULL;
	player = rvn_media_get(id);
	if (!player) rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "%s: not a media player or a VideoView", function);
	return player;
}

#define RVN_MEDIA(variable, id, result) \
	rvn_media_player_t* variable = rvn_media_check((id), __func__); \
	if (!variable) return result

#define RVN_MEDIA_VOID(variable, id) \
	rvn_media_player_t* variable = rvn_media_check((id), __func__); \
	if (!variable) return

/* Something is to be told or to be asked for: the loop wakes until it is done. */
static void rvn_media_timer_tick(Rid timer, void* user_data) {
	RVN_UNUSED(timer);
	RVN_UNUSED(user_data);
	/* (nothing: the frame this wakes runs rvn_media_update) */
}

static int rvn_media_busy(const rvn_media_player_t* player) {
	return player->state == R_MEDIA_LOADING || player->state == R_MEDIA_PLAYING || player->state != player->told_state ||
		player->loaded != player->told_loaded || player->seeked;
}

static void rvn_media_keep_awake(void) {
	if (g_media.timer != R_INVALID_ID && Rtimer_is_valid(g_media.timer)) return;
	g_media.timer = Rtimer_create(RVN_MEDIA_WAKE, R_TRUE, rvn_media_timer_tick, NULL);
}

/* The VideoView follows its player (the time, the buttons, a new picture). */
static void rvn_media_tell_view(rvn_media_player_t* player, RMediaEvent event) {
	rvn_widget_t* view = player->view != R_INVALID_ID ? rvn_widget_get(player->view) : NULL;
	if (view && view->cls->type == R_WIDGET_VIDEO_VIEW) rvn_video_view_player_changed(view, event);
}

static void rvn_media_set_state(rvn_media_player_t* player, RMediaState state) {
	player->state = state;
	rvn_media_keep_awake();
	rvn_media_tell_view(player, (RMediaEvent)0);
}

static void rvn_media_fail(rvn_media_player_t* player, const char* message) {
	rvn_str_set(&player->error, message && message[0] ? message : "the media cannot be played");
	player->want_play = R_FALSE;
	rvn_media_set_state(player, R_MEDIA_ERROR);
}

/* Lets go of the stream and of what was known of it (not of the settings). */
static void rvn_media_unload(rvn_media_player_t* player) {
	if (player->stream) player->backend->close(player->stream);
	player->stream = NULL;
	player->backend = NULL;
	if (player->texture != R_INVALID_ID) rvn_texture_release(player->texture);
	player->texture = R_INVALID_ID;
	player->texture_width = player->texture_height = 0;
	player->frame_count = 0;
	player->want_play = R_FALSE;
	player->loaded = R_FALSE;
	player->told_loaded = R_FALSE;
	player->duration = player->position = 0.0;
	player->told_position = 0.0;
	player->width = player->height = 0;
	player->has_video = player->has_audio = R_FALSE;
	rvn_str_set(&player->error, "");
}

static rvn_media_player_t* rvn_media_new(int internal) {
	rvn_media_player_t* player;
	rvn_media_init();
	player = (rvn_media_player_t*)rvn_calloc(sizeof(rvn_media_player_t));
	player->id = rvn_pool_add(&g_media.pool, player);
	if (player->id == R_INVALID_ID) {
		rvn_free(player);
		rvn_error(R_ERROR_OUT_OF_MEMORY, R_SEVERITY_ERROR, "Rmedia_player_create: too many players");
		return NULL;
	}
	player->source = rvn_strdup("");
	player->error = rvn_strdup("");
	player->state = player->told_state = R_MEDIA_EMPTY;
	player->volume = 1.0f;
	player->rate = 1.0f;
	player->texture = R_INVALID_ID;
	player->view = R_INVALID_ID;
	player->internal = internal;
	return player;
}

static void rvn_media_free(rvn_media_player_t* player) {
	rvn_media_unload(player);
	rvn_pool_remove(&g_media.pool, player->id);
	rvn_free(player->source);
	rvn_free(player->error);
	rvn_free(player);
}

/* ---------------------------------------------------------------------------
 *  Every frame
 * ------------------------------------------------------------------------- */

/* Asks the backend what changed. */
static void rvn_media_poll(rvn_media_player_t* player, double dt) {
	rvn_media_info_t info;
	if (!player->stream) return;
	memset(&info, 0, sizeof(info));
	player->backend->poll(player->stream, dt, &info);
	if (info.status == RVN_MEDIA_FAILED) {
		if (player->state != R_MEDIA_ERROR) rvn_media_fail(player, info.error);
		return;
	}
	if (info.status != RVN_MEDIA_OPENED) return;
	player->duration = info.duration > 0.0 ? info.duration : 0.0;
	player->position = info.position > 0.0 ? info.position : 0.0;
	if (player->duration > 0.0 && player->position > player->duration) player->position = player->duration;
	if (player->width != info.width || player->height != info.height || player->has_video != info.has_video) rvn_media_tell_view(player, (RMediaEvent)0);
	player->width = info.width;
	player->height = info.height;
	player->has_video = info.has_video && info.width > 0 && info.height > 0;
	player->has_audio = info.has_audio;
	if (player->state == R_MEDIA_LOADING) {
		player->loaded = R_TRUE;
		if (player->want_play) {
			player->backend->play(player->stream);
			rvn_media_set_state(player, R_MEDIA_PLAYING);
		} else {
			rvn_media_set_state(player, R_MEDIA_PAUSED);
		}
		player->want_play = R_FALSE;
	}
	if (info.ended && player->state == R_MEDIA_PLAYING) {
		if (player->loop) {
			player->backend->seek(player->stream, 0.0);
			player->backend->play(player->stream);
			player->position = 0.0;
			player->seeked = R_TRUE;
		} else {
			player->backend->pause(player->stream);
			if (player->duration > 0.0) player->position = player->duration;
			rvn_media_set_state(player, R_MEDIA_ENDED);
		}
	}
}

/* Calls the application; R_FALSE when the player is gone meanwhile. */
static int rvn_media_send(Rid id, RMediaEvent event) {
	rvn_media_player_t* player = (rvn_media_player_t*)rvn_pool_get(&g_media.pool, id);
	if (!player) return R_FALSE;
	rvn_media_tell_view(player, event);
	player = (rvn_media_player_t*)rvn_pool_get(&g_media.pool, id);
	if (player && player->callback) {
		player->callback(id, event, player->user_data);
		player = (rvn_media_player_t*)rvn_pool_get(&g_media.pool, id);
	}
	return player != NULL;
}

/* Tells what changed since it was last told. */
static void rvn_media_report(Rid id) {
	rvn_media_player_t* player = (rvn_media_player_t*)rvn_pool_get(&g_media.pool, id);
	RMediaState state;
	if (!player) return;
	if (player->loaded && !player->told_loaded) {
		player->told_loaded = R_TRUE;
		if (!rvn_media_send(id, R_MEDIA_EVENT_LOADED)) return;
		player = (rvn_media_player_t*)rvn_pool_get(&g_media.pool, id);
	}
	state = player->state;
	if (state != player->told_state) {
		player->told_state = state;
		if (!rvn_media_send(id, R_MEDIA_EVENT_STATE)) return;
		if (state == R_MEDIA_ENDED && !rvn_media_send(id, R_MEDIA_EVENT_ENDED)) return;
		if (state == R_MEDIA_ERROR && !rvn_media_send(id, R_MEDIA_EVENT_ERROR)) return;
		player = (rvn_media_player_t*)rvn_pool_get(&g_media.pool, id);
	}
	if (player->seeked || (player->state == R_MEDIA_PLAYING && fabs(player->position - player->told_position) >= RVN_MEDIA_TIME_STEP) ||
		(player->state == R_MEDIA_ENDED && player->position != player->told_position)) {
		player->seeked = R_FALSE;
		player->told_position = player->position;
		if (!rvn_media_send(id, R_MEDIA_EVENT_TIME)) return;
		player = (rvn_media_player_t*)rvn_pool_get(&g_media.pool, id);
	}
	/* a sound of Rsound_play is done */
	if (player->sound && (player->state == R_MEDIA_ENDED || player->state == R_MEDIA_ERROR)) rvn_media_free(player);
}

/* A player of the system loads or plays. */
static int rvn_media_real_busy(void) {
	uint32_t i;
	for (i = 0; i < g_media.pool.used; i++) {
		rvn_media_player_t* player = (rvn_media_player_t*)rvn_pool_at(&g_media.pool, i);
		if (player && player->backend && player->backend != &g_rvn_fake_media &&
			(player->state == R_MEDIA_LOADING || player->state == R_MEDIA_PLAYING)) return R_TRUE;
	}
	return R_FALSE;
}

void rvn_media_update(void) {
	double now, dt;
	uint32_t i;
	int busy = R_FALSE;
	if (!g_media.ready || g_media.pool.live == 0) {
		if (g_media.ready) g_media.last_clock = rvn_clock_seconds();
		return;
	}
	now = rvn_clock_seconds();
	if (g_rvn.animation_time_step > 0.0) {
		dt = g_rvn.animation_time_step * (double)Rget_animation_speed();
		/*
		 * The deterministic clock (tests, headless runs) makes frames as fast as they can be made,
		 * each 1/60 s of the animations; a player of the system plays in real time. While one
		 * loads or plays, a frame is not given before its 1/60 s has passed: what is drawn and
		 * filmed then follows the video. (The fake media follow the step and need no wait.)
		 */
		if (rvn_media_real_busy()) {
			double due = g_media.paced_clock + g_rvn.animation_time_step;
			if (now < due && due - now < 1.0) {
				rvn_sleep_ms((int)ceil((due - now) * 1000.0));
				now = rvn_clock_seconds();
			}
		}
		g_media.paced_clock = now;
	} else {
		dt = now - g_media.last_clock;
		/* (a long stall, a debugger: the fake clip goes on from where it was) */
		if (dt > 0.25) dt = 0.25;
		if (dt < 0.0) dt = 0.0;
	}
	g_media.last_clock = now;
	rvn_dispatch_begin();
	for (i = 0; i < g_media.pool.used; i++) {
		rvn_media_player_t* player = (rvn_media_player_t*)rvn_pool_at(&g_media.pool, i);
		Rid id;
		if (!player) continue;
		id = player->id;
		rvn_media_poll(player, dt);
		rvn_media_report(id);
		player = (rvn_media_player_t*)rvn_pool_get(&g_media.pool, id);
		if (player && rvn_media_busy(player)) busy = R_TRUE;
	}
	rvn_dispatch_end();
	if (!busy && g_media.timer != R_INVALID_ID) {
		if (Rtimer_is_valid(g_media.timer)) Rtimer_destroy(g_media.timer);
		g_media.timer = R_INVALID_ID;
	} else if (busy) {
		rvn_media_keep_awake();
	}
}

void rvn_media_shutdown(void) {
	uint32_t i;
	if (!g_media.ready) return;
	for (i = 0; i < g_media.pool.used; i++) {
		rvn_media_player_t* player = (rvn_media_player_t*)rvn_pool_at(&g_media.pool, i);
		if (player) rvn_media_free(player);
	}
	rvn_pool_free(&g_media.pool);
	if (g_media.timer != R_INVALID_ID && Rtimer_is_valid(g_media.timer)) Rtimer_destroy(g_media.timer);
	memset(&g_media, 0, sizeof(g_media));
}

/* ---------------------------------------------------------------------------
 *  Pictures
 * ------------------------------------------------------------------------- */

static Rid rvn_media_take_frame(rvn_media_player_t* player) {
	uint8_t* pixels;
	int width = 0, height = 0;
	if (!player->stream || !player->loaded || !Ris_ui_thread()) return player->texture;
	pixels = player->backend->take_frame(player->stream, &width, &height);
	if (!pixels) return player->texture;
	if (width <= 0 || height <= 0) {
		rvn_free(pixels);
		return player->texture;
	}
	if (player->texture != R_INVALID_ID && rvn_texture_get(player->texture) && width == player->texture_width && height == player->texture_height) {
		rvn_texture_replace_pixels(player->texture, pixels);
	} else {
		if (player->texture != R_INVALID_ID) rvn_texture_release(player->texture);
		player->texture = rvn_texture_create_raw(width, height, R_TEXTURE_RGBA8, R_FILTER_LINEAR, pixels);
		player->texture_width = width;
		player->texture_height = height;
	}
	player->frame_count++;
	return player->texture;
}

Rid rvn_media_player_frame(Rid id, int* playing) {
	rvn_media_player_t* player = rvn_media_get(id);
	if (playing) *playing = R_FALSE;
	if (!player) return R_INVALID_ID;
	if (playing) *playing = player->state == R_MEDIA_PLAYING && player->has_video;
	return rvn_media_take_frame(player);
}

/* ---------------------------------------------------------------------------
 *  For the VideoView
 * ------------------------------------------------------------------------- */

Rid rvn_media_player_create_internal(void) {
	rvn_media_player_t* player = rvn_media_new(R_TRUE);
	return player ? player->id : R_INVALID_ID;
}

void rvn_media_player_release(Rid id) {
	rvn_media_player_t* player = g_media.ready ? (rvn_media_player_t*)rvn_pool_get(&g_media.pool, id) : NULL;
	if (player) rvn_media_free(player);
}

void rvn_media_player_set_view(Rid id, Rid view) {
	rvn_media_player_t* player = g_media.ready ? (rvn_media_player_t*)rvn_pool_get(&g_media.pool, id) : NULL;
	if (player) player->view = view;
}

/* ---------------------------------------------------------------------------
 *  API
 * ------------------------------------------------------------------------- */

int Rmedia_is_supported(void) {
	if (!g_rvn.initialized) return R_FALSE;
	return rvn_media_backend() != NULL;
}

const char* Rmedia_get_backend_name(void) {
	const rvn_media_backend_t* backend;
	if (!g_rvn.initialized) return "";
	backend = rvn_media_backend();
	return backend ? backend->name : "";
}

Rid Rmedia_player_create(void) {
	rvn_media_player_t* player;
	RVN_API_GUARD(R_INVALID_ID);
	player = rvn_media_new(R_FALSE);
	return player ? player->id : R_INVALID_ID;
}

void Rmedia_player_destroy(Rid id) {
	rvn_media_player_t* player;
	if (!rvn_api_check(__func__)) return;
	player = g_media.ready && rvn_handle_type(id) == RVN_HANDLE_MEDIA_PLAYER ? (rvn_media_player_t*)rvn_pool_get(&g_media.pool, id) : NULL;
	if (!player) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rmedia_player_destroy: not a media player");
		return;
	}
	if (player->internal) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rmedia_player_destroy: the player of a VideoView goes with it");
		return;
	}
	rvn_media_free(player);
}

int Rmedia_player_is_valid(Rid id) {
	if (!g_rvn.initialized) return R_FALSE;
	return rvn_media_get(id) != NULL;
}

static int rvn_media_load(rvn_media_player_t* player, const char* location, int is_url) {
	const rvn_media_backend_t* backend = rvn_media_backend();
	char error[256];
	char* absolute = NULL;
	rvn_media_unload(player);
	rvn_str_set(&player->source, location ? location : "");
	if (!location || !location[0]) {
		rvn_media_set_state(player, R_MEDIA_EMPTY);
		return R_FALSE;
	}
	if (!backend) {
		rvn_media_fail(player, "media cannot be played on this system");
		return R_FALSE;
	}
	if (!is_url && backend != &g_rvn_fake_media) {
		if (!rvn_path_exists(location) || rvn_path_is_dir(location)) {
			char message[600];
			snprintf(message, sizeof(message), "no such file: %s", location);
			rvn_media_fail(player, message);
			return R_FALSE;
		}
		/* (the frameworks take absolute paths or URLs) */
		absolute = rvn_path_absolute(location);
	}
	error[0] = '\0';
	player->stream = backend->open(absolute ? absolute : location, is_url, error);
	rvn_free(absolute);
	if (!player->stream) {
		rvn_media_fail(player, error);
		return R_FALSE;
	}
	player->backend = backend;
	backend->set_volume(player->stream, player->volume, player->muted);
	if (player->rate != 1.0f) backend->set_rate(player->stream, player->rate);
	rvn_media_set_state(player, R_MEDIA_LOADING);
	return R_TRUE;
}

int Rmedia_player_load_file(Rid id, const char* path) {
	RVN_MEDIA(player, id, R_FALSE);
	return rvn_media_load(player, path, R_FALSE);
}

int Rmedia_player_load_url(Rid id, const char* url) {
	RVN_MEDIA(player, id, R_FALSE);
	return rvn_media_load(player, url, R_TRUE);
}

void Rmedia_player_close(Rid id) {
	RVN_MEDIA_VOID(player, id);
	rvn_media_unload(player);
	rvn_str_set(&player->source, "");
	rvn_media_set_state(player, R_MEDIA_EMPTY);
}

const char* Rmedia_player_get_source(Rid id) {
	RVN_MEDIA(player, id, "");
	return player->source;
}

void Rmedia_player_play(Rid id) {
	RVN_MEDIA_VOID(player, id);
	switch (player->state) {
		case R_MEDIA_LOADING:
			player->want_play = R_TRUE;
			break;
		case R_MEDIA_ENDED:
			/* from the start again */
			player->backend->seek(player->stream, 0.0);
			player->position = 0.0;
			player->seeked = R_TRUE;
			/* fall through */
		case R_MEDIA_PAUSED:
			player->backend->play(player->stream);
			rvn_media_set_state(player, R_MEDIA_PLAYING);
			break;
		default:
			break;
	}
}

void Rmedia_player_pause(Rid id) {
	RVN_MEDIA_VOID(player, id);
	if (player->state == R_MEDIA_LOADING) player->want_play = R_FALSE;
	if (player->state != R_MEDIA_PLAYING) return;
	player->backend->pause(player->stream);
	rvn_media_set_state(player, R_MEDIA_PAUSED);
}

void Rmedia_player_stop(Rid id) {
	RVN_MEDIA_VOID(player, id);
	if (player->state == R_MEDIA_LOADING) player->want_play = R_FALSE;
	if (player->state != R_MEDIA_PLAYING && player->state != R_MEDIA_PAUSED && player->state != R_MEDIA_ENDED) return;
	player->backend->pause(player->stream);
	player->backend->seek(player->stream, 0.0);
	player->position = 0.0;
	player->seeked = R_TRUE;
	rvn_media_set_state(player, R_MEDIA_PAUSED);
}

void Rmedia_player_seek(Rid id, double seconds) {
	RVN_MEDIA_VOID(player, id);
	if (player->state != R_MEDIA_PLAYING && player->state != R_MEDIA_PAUSED && player->state != R_MEDIA_ENDED) return;
	if (!(seconds >= 0.0)) seconds = 0.0;
	if (player->duration > 0.0 && seconds > player->duration) seconds = player->duration;
	player->backend->seek(player->stream, seconds);
	player->position = seconds;
	player->seeked = R_TRUE;
	/* ended, a seek back makes it paused there */
	if (player->state == R_MEDIA_ENDED && (player->duration <= 0.0 || seconds < player->duration)) rvn_media_set_state(player, R_MEDIA_PAUSED);
	else rvn_media_keep_awake();
	rvn_media_tell_view(player, R_MEDIA_EVENT_TIME);
}

RMediaState Rmedia_player_get_state(Rid id) {
	RVN_MEDIA(player, id, R_MEDIA_EMPTY);
	return player->state;
}

double Rmedia_player_get_position(Rid id) {
	RVN_MEDIA(player, id, 0.0);
	return player->position;
}

double Rmedia_player_get_duration(Rid id) {
	RVN_MEDIA(player, id, 0.0);
	return player->duration;
}

int Rmedia_player_get_video_size(Rid id, int* width, int* height) {
	if (width) *width = 0;
	if (height) *height = 0;
	{
		RVN_MEDIA(player, id, R_FALSE);
		if (!player->has_video) return R_FALSE;
		if (width) *width = player->width;
		if (height) *height = player->height;
		return R_TRUE;
	}
}

int Rmedia_player_has_audio(Rid id) {
	RVN_MEDIA(player, id, R_FALSE);
	return player->has_audio;
}

const char* Rmedia_player_get_error(Rid id) {
	RVN_MEDIA(player, id, "");
	return player->error;
}

void Rmedia_player_set_volume(Rid id, float volume) {
	RVN_MEDIA_VOID(player, id);
	if (!(volume >= 0.0f)) volume = 0.0f;
	if (volume > 1.0f) volume = 1.0f;
	player->volume = volume;
	if (player->stream) player->backend->set_volume(player->stream, player->volume, player->muted);
	rvn_media_tell_view(player, (RMediaEvent)0);
}

float Rmedia_player_get_volume(Rid id) {
	RVN_MEDIA(player, id, 0.0f);
	return player->volume;
}

void Rmedia_player_set_muted(Rid id, int muted) {
	RVN_MEDIA_VOID(player, id);
	player->muted = muted ? R_TRUE : R_FALSE;
	if (player->stream) player->backend->set_volume(player->stream, player->volume, player->muted);
	rvn_media_tell_view(player, (RMediaEvent)0);
}

int Rmedia_player_is_muted(Rid id) {
	RVN_MEDIA(player, id, R_FALSE);
	return player->muted;
}

void Rmedia_player_set_loop(Rid id, int loop) {
	RVN_MEDIA_VOID(player, id);
	player->loop = loop ? R_TRUE : R_FALSE;
}

int Rmedia_player_get_loop(Rid id) {
	RVN_MEDIA(player, id, R_FALSE);
	return player->loop;
}

void Rmedia_player_set_rate(Rid id, float rate) {
	RVN_MEDIA_VOID(player, id);
	if (!(rate >= 0.25f)) rate = 0.25f;
	if (rate > 4.0f) rate = 4.0f;
	player->rate = rate;
	if (player->stream) player->backend->set_rate(player->stream, rate);
}

float Rmedia_player_get_rate(Rid id) {
	RVN_MEDIA(player, id, 1.0f);
	return player->rate;
}

Rid Rmedia_player_get_texture(Rid id) {
	RVN_MEDIA(player, id, R_INVALID_ID);
	return rvn_media_take_frame(player);
}

int Rmedia_player_get_frame_count(Rid id) {
	RVN_MEDIA(player, id, 0);
	return player->frame_count;
}

void Rmedia_player_set_event_callback(Rid id, RMediaEventCallback callback, void* user_data) {
	RVN_MEDIA_VOID(player, id);
	player->callback = callback;
	player->user_data = user_data;
}

Rid Rsound_play(const char* path, float volume) {
	rvn_media_player_t* player;
	Rid id;
	RVN_API_GUARD(R_INVALID_ID);
	if (!path || !path[0]) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rsound_play: no file");
		return R_INVALID_ID;
	}
	player = rvn_media_new(R_FALSE);
	if (!player) return R_INVALID_ID;
	id = player->id;
	player->sound = R_TRUE;
	player->volume = !(volume >= 0.0f) ? 0.0f : volume > 1.0f ? 1.0f : volume;
	if (!rvn_media_load(player, path, R_FALSE)) {
		rvn_media_free(player);
		return R_INVALID_ID;
	}
	player->want_play = R_TRUE;
	return id;
}

void Rsound_stop_all(void) {
	uint32_t i;
	if (!rvn_api_check(__func__) || !g_media.ready) return;
	for (i = 0; i < g_media.pool.used; i++) {
		rvn_media_player_t* player = (rvn_media_player_t*)rvn_pool_at(&g_media.pool, i);
		if (player && player->sound) rvn_media_free(player);
	}
}

/* ---------------------------------------------------------------------------
 *  The fake backend of the tests (Rtest_set_fake_media)
 * ------------------------------------------------------------------------- */

typedef struct rvn_fake_stream_t {
	double duration, position, fps;
	int width, height;
	int video, failed, opened, playing, ended;
	float rate;
	int last_frame;
} rvn_fake_stream_t;

static int rvn_fake_available(void) {
	return R_TRUE;
}

static int rvn_fake_is_audio(const char* location) {
	static const char* const extensions[] = { ".wav", ".mp3", ".m4a", ".aac", ".ogg", ".flac" };
	size_t length = strlen(location), i;
	for (i = 0; i < sizeof(extensions) / sizeof(extensions[0]); i++) {
		size_t n = strlen(extensions[i]);
		if (length >= n && rvn_stricmp(location + length - n, extensions[i]) == 0) return R_TRUE;
	}
	return R_FALSE;
}

static void* rvn_fake_open(const char* location, int is_url, char* error) {
	rvn_fake_stream_t* stream = (rvn_fake_stream_t*)rvn_calloc(sizeof(rvn_fake_stream_t));
	RVN_UNUSED(is_url);
	RVN_UNUSED(error);
	stream->duration = g_media.fake_duration;
	stream->fps = g_media.fake_fps > 0.0 ? g_media.fake_fps : 30.0;
	stream->width = g_media.fake_width > 0 ? g_media.fake_width : 160;
	stream->height = g_media.fake_height > 0 ? g_media.fake_height : 120;
	stream->video = !rvn_fake_is_audio(location);
	stream->failed = strstr(location, "invalid") != NULL;
	stream->rate = 1.0f;
	stream->last_frame = -1;
	return stream;
}

static void rvn_fake_close(void* data) {
	rvn_free(data);
}

static void rvn_fake_play(void* data) {
	rvn_fake_stream_t* stream = (rvn_fake_stream_t*)data;
	stream->playing = R_TRUE;
	stream->ended = R_FALSE;
}

static void rvn_fake_pause(void* data) {
	((rvn_fake_stream_t*)data)->playing = R_FALSE;
}

static void rvn_fake_seek(void* data, double seconds) {
	rvn_fake_stream_t* stream = (rvn_fake_stream_t*)data;
	stream->position = seconds < 0.0 ? 0.0 : seconds > stream->duration ? stream->duration : seconds;
	stream->ended = R_FALSE;
}

static void rvn_fake_set_volume(void* data, float volume, int muted) {
	RVN_UNUSED(data);
	RVN_UNUSED(volume);
	RVN_UNUSED(muted);
}

static void rvn_fake_set_rate(void* data, float rate) {
	((rvn_fake_stream_t*)data)->rate = rate;
}

static void rvn_fake_poll(void* data, double dt, rvn_media_info_t* info) {
	rvn_fake_stream_t* stream = (rvn_fake_stream_t*)data;
	if (stream->failed) {
		info->status = RVN_MEDIA_FAILED;
		snprintf(info->error, sizeof(info->error), "the file cannot be read (fake media)");
		return;
	}
	if (stream->opened && stream->playing) {
		stream->position += dt * (double)stream->rate;
		/* (a small margin: steps of 1/60 s add up to the duration they are meant to) */
		if (stream->position + 1e-9 >= stream->duration) {
			stream->position = stream->duration;
			stream->playing = R_FALSE;
			stream->ended = R_TRUE;
		}
	}
	stream->opened = R_TRUE;
	info->status = RVN_MEDIA_OPENED;
	info->duration = stream->duration;
	info->position = stream->position;
	info->has_audio = R_TRUE;
	info->has_video = stream->video;
	info->width = stream->video ? stream->width : 0;
	info->height = stream->video ? stream->height : 0;
	info->ended = stream->ended;
	stream->ended = R_FALSE;
}

static uint8_t* rvn_fake_take_frame(void* data, int* width, int* height) {
	rvn_fake_stream_t* stream = (rvn_fake_stream_t*)data;
	int frames = (int)ceil(stream->duration * stream->fps - 1e-9), frame, x, y, bar;
	uint8_t* pixels;
	if (!stream->video || !stream->opened) return NULL;
	frame = (int)floor(stream->position * stream->fps + 1e-6);
	if (frame >= frames) frame = frames - 1;
	if (frame < 0) frame = 0;
	if (frame == stream->last_frame) return NULL;
	stream->last_frame = frame;
	pixels = (uint8_t*)rvn_alloc((size_t)stream->width * (size_t)stream->height * 4);
	/* a bar that crosses the picture, on a background whose red is the number of the picture */
	bar = frames > 1 ? (int)((double)frame / (double)(frames - 1) * (double)(stream->width - 8)) : 0;
	for (y = 0; y < stream->height; y++) {
		for (x = 0; x < stream->width; x++) {
			uint8_t* p = pixels + ((size_t)y * (size_t)stream->width + (size_t)x) * 4;
			int lit = x >= bar && x < bar + 8 && (x > 0 || y > 0);
			p[0] = lit ? 255 : (uint8_t)(frame & 255);
			p[1] = lit ? 255 : (uint8_t)(40 + (y * 120) / stream->height);
			p[2] = lit ? 255 : 150;
			p[3] = 255;
		}
	}
	*width = stream->width;
	*height = stream->height;
	return pixels;
}

static const rvn_media_backend_t g_rvn_fake_media = {
	"fake",
	rvn_fake_available,
	rvn_fake_open,
	rvn_fake_close,
	rvn_fake_play,
	rvn_fake_pause,
	rvn_fake_seek,
	rvn_fake_set_volume,
	rvn_fake_set_rate,
	rvn_fake_poll,
	rvn_fake_take_frame
};

void Rtest_set_fake_media(double duration, int width, int height, double fps) {
	if (!rvn_api_check(__func__)) return;
	g_media.fake_duration = duration > 0.0 ? duration : 0.0;
	g_media.fake_width = width;
	g_media.fake_height = height;
	g_media.fake_fps = fps;
}
