/**
 * Raven - media/media.h
 * Playback of video and sound with the media framework of the system
 * (raven_controls.h, VIDEO AND SOUND): the players of the core (media.c) and
 * the backends that decode for them.
 *
 * A backend opens a file or an address as a stream, plays it with its own
 * clock (the clock of the sound card when there is sound), and answers
 * questions: where it is, whether it ended, whether a new picture came. Every
 * function is called from the UI thread; a backend whose framework calls it
 * back on other threads keeps what it learns under its own lock. Nothing is
 * pushed to the core: it asks (media.c, rvn_media_update, once per frame while
 * a player is busy), so the state of a player changes only between frames.
 *
 *   AVFoundation      macOS, iOS       media_avfoundation.m (AVPlayer, AVPlayerItemVideoOutput)
 *   Media Foundation  Windows          media_mf.c (IMFMediaEngine in frame-server mode)
 *   GStreamer         Linux, BSD       media_gstreamer.c (playbin and an appsink, opened at run time)
 *   fake              tests            media.c (Rtest_set_fake_media: frames computed from the time)
 */

#ifndef RVN_MEDIA_H
#define RVN_MEDIA_H

#include "../core/base.h"
#include "raven/raven_controls.h"

/** What a stream is doing (rvn_media_info_t::status). */
typedef enum rvn_media_status_t {
	/** Still opening: the duration and the tracks are not known yet. */
	RVN_MEDIA_OPENING = 0,
	/** Opened: it plays or waits as it is told. */
	RVN_MEDIA_OPENED,
	/** It cannot be played: rvn_media_info_t::error says why. */
	RVN_MEDIA_FAILED
} rvn_media_status_t;

/** What a backend tells of a stream when asked (rvn_media_backend_t::poll). */
typedef struct rvn_media_info_t {
	rvn_media_status_t status;
	/** Seconds; 0 while unknown or for a live stream. */
	double duration;
	/** Seconds from the start. */
	double position;
	/** Size of the picture in pixels, as it is to be shown (0 x 0: no picture). */
	int width, height;
	int has_video, has_audio;
	/** R_TRUE once each time it played to its end (the backend clears it when it reports it). */
	int ended;
	/** RVN_MEDIA_FAILED: why, in English. */
	char error[256];
} rvn_media_info_t;

typedef struct rvn_media_backend_t {
	/** "AVFoundation", "Media Foundation", "GStreamer", "fake". */
	const char* name;
	/** R_TRUE when the framework is there (it may be loaded the first time this is asked). */
	int (*available)(void);
	/**
	 * Starts opening a file path or an address (`is_url`). Returns NULL when
	 * it cannot even start, `error` (256 bytes) then saying why; else the
	 * stream, paused at its start, which opens in the background.
	 */
	void* (*open)(const char* location, int is_url, char* error);
	void (*close)(void* stream);
	void (*play)(void* stream);
	void (*pause)(void* stream);
	/** Goes to a time in seconds, as exactly as the format allows (playing or paused, it stays so). */
	void (*seek)(void* stream, double seconds);
	/** 0..1, and the sound off. */
	void (*set_volume)(void* stream, float volume, int muted);
	/** 1 = normal speed. */
	void (*set_rate)(void* stream, float rate);
	/**
	 * Fills `info`. `dt`: seconds of the clock of the animations since the
	 * previous poll (only the fake backend uses it; the others have a clock
	 * of their own).
	 */
	void (*poll)(void* stream, double dt, rvn_media_info_t* info);
	/**
	 * The picture of now, when it is not the one given last: returns it as
	 * RGBA8 pixels (width * height * 4 bytes, rvn_alloc'ed, given to the
	 * caller), else NULL.
	 */
	uint8_t* (*take_frame)(void* stream, int* width, int* height);
} rvn_media_backend_t;

/** The backend of this system, or NULL where there is none (media_*.c). */
const rvn_media_backend_t* rvn_media_system_backend(void);

/** The core, once per frame (core/context.c, rvn_update): the players that are busy ask their backends what changed. */
void rvn_media_update(void);
/** Rshutdown: every player goes. */
void rvn_media_shutdown(void);

/** R_TRUE when the platform runs the event loop of the system (the main run loop of macOS...); R_FALSE for the null platform (headless). */
int rvn_media_platform_pumps(void);

/** Converts BGRA rows (as most frameworks decode) to tight RGBA8. `stride` in bytes. */
void rvn_media_bgra_to_rgba(uint8_t* to, const uint8_t* from, int width, int height, size_t stride);

/* ---- for widgets/video_view.c ---- */

struct rvn_widget_t;
/** The player of a VideoView (widgets/video_view.c). */
Rid rvn_video_view_player_of(struct rvn_widget_t* view);
/** Its player changed: a new state (an event), the time, the sound, the size of the picture (event 0). */
void rvn_video_view_player_changed(struct rvn_widget_t* view, RMediaEvent event);

/** Creates a player for a VideoView (it is deleted with it, rvn_media_player_release). */
Rid rvn_media_player_create_internal(void);
void rvn_media_player_release(Rid player);
/** The VideoView whose picture follows the player: its paint is asked for when the player changes. */
void rvn_media_player_set_view(Rid player, Rid view);
/**
 * Brings the texture to the picture of now (UI thread) and returns it;
 * R_INVALID_ID before the first picture. `playing` receives R_TRUE while
 * pictures keep coming (the view then asks for the next frame).
 */
Rid rvn_media_player_frame(Rid player, int* playing);

#endif /* RVN_MEDIA_H */
