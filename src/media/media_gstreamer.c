/**
 * Raven - media/media_gstreamer.c
 * Media players on Linux and the BSDs (media.h): GStreamer, where it is
 * installed. A playbin opens the file or the address, picks the decoders
 * that are installed, plays the sound on the output of the system and keeps
 * it in time; its pictures go to an appsink that asks for RGBA (playbin puts
 * the conversion in front of it) and keeps only the last one (max-buffers 1,
 * drop): the picture of now is taken from it when asked, never queued.
 *
 * GStreamer is not needed to build or to run Raven: libgstreamer-1.0.so.0,
 * libgstapp-1.0.so.0, libgobject-2.0.so.0 and libglib-2.0.so.0 are opened
 * with dlopen the first time something is played, and the few types read
 * here are declared here (their layouts have not changed in GStreamer 1.x).
 * Without them, or without the playbin element (gst-plugins-base),
 * Rmedia_is_supported is R_FALSE. Which formats play depends on the plugins
 * installed (H.264 needs gst-plugins-bad, gst-libav or gst-plugins-ugly).
 *
 * Polled from the UI thread (media.c): the messages of the bus (errors, the
 * end, the end of a preroll) are taken there, without waiting.
 */

#include <dlfcn.h>
#include <math.h>
#include <stddef.h>

#include "media.h"

/* ---- what is read of GStreamer and GLib ---- */

typedef struct rvn_gst_mini_object_t {
	size_t type;
	int refcount;
	int lockstate;
	unsigned int flags;
	void* copy;
	void* dispose;
	void* free;
	unsigned int priv_uint;
	void* priv_pointer;
} rvn_gst_mini_object_t;

/* GstMessage: the type follows the mini object */
typedef struct rvn_gst_message_t {
	rvn_gst_mini_object_t mini_object;
	unsigned int type;
} rvn_gst_message_t;

/* GstMapInfo */
typedef struct rvn_gst_map_t {
	void* memory;
	int flags;
	uint8_t* data;
	size_t size;
	size_t maxsize;
	void* user_data[4];
	void* reserved[4];
} rvn_gst_map_t;

/* GError */
typedef struct rvn_g_error_t {
	uint32_t domain;
	int code;
	char* message;
} rvn_g_error_t;

#define RVN_GST_STATE_NULL 1
#define RVN_GST_STATE_PAUSED 3
#define RVN_GST_STATE_PLAYING 4
#define RVN_GST_STATE_CHANGE_FAILURE 0
#define RVN_GST_FORMAT_TIME 3
#define RVN_GST_SEEK_FLAG_FLUSH (1 << 0)
#define RVN_GST_SEEK_FLAG_ACCURATE (1 << 1)
#define RVN_GST_SEEK_TYPE_NONE 0
#define RVN_GST_SEEK_TYPE_SET 1
#define RVN_GST_MAP_READ 1
#define RVN_GST_MESSAGE_EOS (1u << 0)
#define RVN_GST_MESSAGE_ERROR (1u << 1)
#define RVN_GST_MESSAGE_DURATION_CHANGED (1u << 18)
#define RVN_GST_MESSAGE_ASYNC_DONE (1u << 21)
#define RVN_GST_SECOND 1000000000.0

static struct {
	int tried;
	int available;
	int (*init_check)(int*, char***, rvn_g_error_t**);
	void* (*element_factory_make)(const char*, const char*);
	int (*element_set_state)(void*, int);
	void* (*element_get_bus)(void*);
	int (*element_query_position)(void*, int, int64_t*);
	int (*element_query_duration)(void*, int, int64_t*);
	int (*element_seek)(void*, double, int, int, int, int64_t, int, int64_t);
	void* (*bus_pop)(void*);
	void (*message_parse_error)(void*, rvn_g_error_t**, char**);
	void (*mini_object_unref)(void*);
	void* (*caps_from_string)(const char*);
	void* (*caps_get_structure)(void*, unsigned int);
	int (*structure_get_int)(void*, const char*, int*);
	void* (*sample_get_buffer)(void*);
	void* (*sample_get_caps)(void*);
	int (*buffer_map)(void*, rvn_gst_map_t*, int);
	void (*buffer_unmap)(void*, rvn_gst_map_t*);
	char* (*filename_to_uri)(const char*, rvn_g_error_t**);
	void* (*object_ref_sink)(void*);
	void* (*app_sink_try_pull_sample)(void*, uint64_t);
	void* (*app_sink_try_pull_preroll)(void*, uint64_t);
	void (*object_set)(void*, const char*, ...);
	void (*object_get)(void*, const char*, ...);
	void (*object_unref)(void*);
	void (*g_free)(void*);
	void (*error_free)(rvn_g_error_t*);
} g_rvn_gst;

static void* rvn_gst_symbol(void* library, const char* name, int* missing) {
	void* symbol = library ? dlsym(library, name) : NULL;
	if (!symbol) *missing = 1;
	return symbol;
}

#define RVN_GST_LOAD(library, field, name) \
	*(void**)(&g_rvn_gst.field) = rvn_gst_symbol((library), (name), &missing)

static int rvn_gst_available(void) {
	void *gst, *app, *gobject, *glib, *probe;
	int missing = 0;
	if (g_rvn_gst.tried) return g_rvn_gst.available;
	g_rvn_gst.tried = 1;
	gst = dlopen("libgstreamer-1.0.so.0", RTLD_NOW | RTLD_LOCAL);
	app = dlopen("libgstapp-1.0.so.0", RTLD_NOW | RTLD_LOCAL);
	gobject = dlopen("libgobject-2.0.so.0", RTLD_NOW | RTLD_LOCAL);
	glib = dlopen("libglib-2.0.so.0", RTLD_NOW | RTLD_LOCAL);
	if (!gst || !app || !gobject || !glib) return 0;
	/* (the libraries stay open: GStreamer cannot be unloaded) */
	RVN_GST_LOAD(gst, init_check, "gst_init_check");
	RVN_GST_LOAD(gst, element_factory_make, "gst_element_factory_make");
	RVN_GST_LOAD(gst, element_set_state, "gst_element_set_state");
	RVN_GST_LOAD(gst, element_get_bus, "gst_element_get_bus");
	RVN_GST_LOAD(gst, element_query_position, "gst_element_query_position");
	RVN_GST_LOAD(gst, element_query_duration, "gst_element_query_duration");
	RVN_GST_LOAD(gst, element_seek, "gst_element_seek");
	RVN_GST_LOAD(gst, bus_pop, "gst_bus_pop");
	RVN_GST_LOAD(gst, message_parse_error, "gst_message_parse_error");
	RVN_GST_LOAD(gst, mini_object_unref, "gst_mini_object_unref");
	RVN_GST_LOAD(gst, caps_from_string, "gst_caps_from_string");
	RVN_GST_LOAD(gst, caps_get_structure, "gst_caps_get_structure");
	RVN_GST_LOAD(gst, structure_get_int, "gst_structure_get_int");
	RVN_GST_LOAD(gst, sample_get_buffer, "gst_sample_get_buffer");
	RVN_GST_LOAD(gst, sample_get_caps, "gst_sample_get_caps");
	RVN_GST_LOAD(gst, buffer_map, "gst_buffer_map");
	RVN_GST_LOAD(gst, buffer_unmap, "gst_buffer_unmap");
	RVN_GST_LOAD(gst, filename_to_uri, "gst_filename_to_uri");
	RVN_GST_LOAD(gst, object_ref_sink, "gst_object_ref_sink");
	RVN_GST_LOAD(app, app_sink_try_pull_sample, "gst_app_sink_try_pull_sample");
	RVN_GST_LOAD(app, app_sink_try_pull_preroll, "gst_app_sink_try_pull_preroll");
	RVN_GST_LOAD(gobject, object_set, "g_object_set");
	RVN_GST_LOAD(gobject, object_get, "g_object_get");
	RVN_GST_LOAD(gobject, object_unref, "g_object_unref");
	RVN_GST_LOAD(glib, g_free, "g_free");
	RVN_GST_LOAD(glib, error_free, "g_error_free");
	if (missing) return 0;
	{
		rvn_g_error_t* error = NULL;
		if (!g_rvn_gst.init_check(NULL, NULL, &error)) {
			if (error) g_rvn_gst.error_free(error);
			return 0;
		}
	}
	/* playbin and appsink come with gst-plugins-base */
	probe = g_rvn_gst.element_factory_make("playbin", NULL);
	if (!probe) return 0;
	g_rvn_gst.object_unref(g_rvn_gst.object_ref_sink(probe));
	probe = g_rvn_gst.element_factory_make("appsink", NULL);
	if (!probe) return 0;
	g_rvn_gst.object_unref(g_rvn_gst.object_ref_sink(probe));
	g_rvn_gst.available = 1;
	return 1;
}

typedef struct rvn_gst_stream_t {
	void* playbin;
	void* sink;
	void* bus;
	/** A picture taken to learn the size, given at the next take_frame. */
	void* pending;
	int opened, ended, playing;
	int width, height;
	char error[256];
	float rate;
} rvn_gst_stream_t;

static void* rvn_gst_open(const char* location, int is_url, char* error) {
	rvn_gst_stream_t* stream;
	void* caps;
	char* uri = NULL;
	rvn_g_error_t* failure = NULL;
	if (is_url) {
		uri = rvn_strdup(location);
	} else {
		char* made = g_rvn_gst.filename_to_uri(location, &failure);
		if (!made) {
			snprintf(error, 256, "GStreamer cannot open %s: %s", location, failure && failure->message ? failure->message : "not a path");
			if (failure) g_rvn_gst.error_free(failure);
			return NULL;
		}
		uri = rvn_strdup(made);
		g_rvn_gst.g_free(made);
	}
	stream = (rvn_gst_stream_t*)rvn_calloc(sizeof(rvn_gst_stream_t));
	stream->rate = 1.0f;
	stream->playbin = g_rvn_gst.element_factory_make("playbin", NULL);
	stream->sink = g_rvn_gst.element_factory_make("appsink", NULL);
	if (!stream->playbin || !stream->sink) {
		snprintf(error, 256, "GStreamer cannot make a player");
		if (stream->playbin) g_rvn_gst.object_unref(g_rvn_gst.object_ref_sink(stream->playbin));
		if (stream->sink) g_rvn_gst.object_unref(g_rvn_gst.object_ref_sink(stream->sink));
		rvn_free(stream);
		rvn_free(uri);
		return NULL;
	}
	g_rvn_gst.object_ref_sink(stream->playbin);
	/* RGBA, square pixels: what the texture takes as it is */
	caps = g_rvn_gst.caps_from_string("video/x-raw,format=RGBA,pixel-aspect-ratio=1/1");
	g_rvn_gst.object_set(stream->sink, "caps", caps, "max-buffers", 1u, "drop", 1, "sync", 1, "emit-signals", 0, (void*)NULL);
	g_rvn_gst.mini_object_unref(caps);
	/* (playbin owns the sink from now on) */
	g_rvn_gst.object_set(stream->playbin, "uri", uri, "video-sink", stream->sink, (void*)NULL);
	rvn_free(uri);
	stream->bus = g_rvn_gst.element_get_bus(stream->playbin);
	/* paused: it opens and prerolls the first picture */
	if (g_rvn_gst.element_set_state(stream->playbin, RVN_GST_STATE_PAUSED) == RVN_GST_STATE_CHANGE_FAILURE)
		snprintf(stream->error, sizeof(stream->error), "GStreamer cannot open %s", location);
	return stream;
}

static void rvn_gst_close(void* data) {
	rvn_gst_stream_t* stream = (rvn_gst_stream_t*)data;
	if (stream->pending) g_rvn_gst.mini_object_unref(stream->pending);
	g_rvn_gst.element_set_state(stream->playbin, RVN_GST_STATE_NULL);
	if (stream->bus) g_rvn_gst.object_unref(stream->bus);
	g_rvn_gst.object_unref(stream->playbin);
	rvn_free(stream);
}

static void rvn_gst_play(void* data) {
	rvn_gst_stream_t* stream = (rvn_gst_stream_t*)data;
	stream->playing = 1;
	stream->ended = 0;
	g_rvn_gst.element_set_state(stream->playbin, RVN_GST_STATE_PLAYING);
}

static void rvn_gst_pause(void* data) {
	rvn_gst_stream_t* stream = (rvn_gst_stream_t*)data;
	stream->playing = 0;
	g_rvn_gst.element_set_state(stream->playbin, RVN_GST_STATE_PAUSED);
}

static void rvn_gst_seek_rate(rvn_gst_stream_t* stream, double seconds, double rate) {
	g_rvn_gst.element_seek(stream->playbin, rate, RVN_GST_FORMAT_TIME, RVN_GST_SEEK_FLAG_FLUSH | RVN_GST_SEEK_FLAG_ACCURATE,
		RVN_GST_SEEK_TYPE_SET, (int64_t)llround(seconds * RVN_GST_SECOND), RVN_GST_SEEK_TYPE_NONE, -1);
}

static void rvn_gst_seek(void* data, double seconds) {
	rvn_gst_stream_t* stream = (rvn_gst_stream_t*)data;
	stream->ended = 0;
	if (stream->pending) g_rvn_gst.mini_object_unref(stream->pending);
	stream->pending = NULL;
	rvn_gst_seek_rate(stream, seconds, (double)stream->rate);
}

static void rvn_gst_set_volume(void* data, float volume, int muted) {
	rvn_gst_stream_t* stream = (rvn_gst_stream_t*)data;
	g_rvn_gst.object_set(stream->playbin, "volume", (double)volume, "mute", muted ? 1 : 0, (void*)NULL);
}

static void rvn_gst_set_rate(void* data, float rate) {
	rvn_gst_stream_t* stream = (rvn_gst_stream_t*)data;
	int64_t at = 0;
	stream->rate = rate;
	/* (a speed is given with a seek, to where it is) */
	if (stream->opened && g_rvn_gst.element_query_position(stream->playbin, RVN_GST_FORMAT_TIME, &at))
		rvn_gst_seek_rate(stream, (double)at / RVN_GST_SECOND, (double)rate);
}

/* The size of the pictures, from a sample. */
static int rvn_gst_sample_size(void* sample, int* width, int* height) {
	void* caps = g_rvn_gst.sample_get_caps(sample);
	void* structure = caps ? g_rvn_gst.caps_get_structure(caps, 0) : NULL;
	return structure && g_rvn_gst.structure_get_int(structure, "width", width) && g_rvn_gst.structure_get_int(structure, "height", height) &&
		*width > 0 && *height > 0;
}

static void rvn_gst_poll(void* data, double dt, rvn_media_info_t* info) {
	rvn_gst_stream_t* stream = (rvn_gst_stream_t*)data;
	void* message;
	int64_t value = 0;
	int count = 0;
	(void)dt;
	while (stream->bus && !stream->error[0] && (message = g_rvn_gst.bus_pop(stream->bus)) != NULL) {
		unsigned int type = ((rvn_gst_message_t*)message)->type;
		if (type == RVN_GST_MESSAGE_ERROR) {
			rvn_g_error_t* failure = NULL;
			char* debug = NULL;
			g_rvn_gst.message_parse_error(message, &failure, &debug);
			snprintf(stream->error, sizeof(stream->error), "%s", failure && failure->message ? failure->message : "GStreamer cannot play it");
			if (failure) g_rvn_gst.error_free(failure);
			if (debug) g_rvn_gst.g_free(debug);
		} else if (type == RVN_GST_MESSAGE_EOS) {
			stream->ended = 1;
		} else if (type == RVN_GST_MESSAGE_ASYNC_DONE) {
			stream->opened = 1;
		}
		g_rvn_gst.mini_object_unref(message);
	}
	if (stream->error[0]) {
		info->status = RVN_MEDIA_FAILED;
		snprintf(info->error, sizeof(info->error), "%s", stream->error);
		return;
	}
	if (!stream->opened) {
		info->status = RVN_MEDIA_OPENING;
		return;
	}
	info->status = RVN_MEDIA_OPENED;
	if (g_rvn_gst.element_query_duration(stream->playbin, RVN_GST_FORMAT_TIME, &value) && value > 0) info->duration = (double)value / RVN_GST_SECOND;
	if (g_rvn_gst.element_query_position(stream->playbin, RVN_GST_FORMAT_TIME, &value) && value > 0) info->position = (double)value / RVN_GST_SECOND;
	g_rvn_gst.object_get(stream->playbin, "n-video", &count, (void*)NULL);
	info->has_video = count > 0;
	count = 0;
	g_rvn_gst.object_get(stream->playbin, "n-audio", &count, (void*)NULL);
	info->has_audio = count > 0;
	if (info->has_video && stream->width == 0) {
		/* the first picture tells the size; it is kept to be shown */
		void* sample = g_rvn_gst.app_sink_try_pull_preroll(stream->sink, 0);
		if (sample) {
			rvn_gst_sample_size(sample, &stream->width, &stream->height);
			if (stream->pending) g_rvn_gst.mini_object_unref(stream->pending);
			stream->pending = sample;
		}
	}
	info->width = info->has_video ? stream->width : 0;
	info->height = info->has_video ? stream->height : 0;
	if (stream->ended) {
		info->ended = 1;
		stream->ended = 0;
		stream->playing = 0;
		if (info->duration > 0.0) info->position = info->duration;
	}
}

static uint8_t* rvn_gst_take_frame(void* data, int* width, int* height) {
	rvn_gst_stream_t* stream = (rvn_gst_stream_t*)data;
	void* sample = stream->pending;
	void* buffer;
	rvn_gst_map_t map;
	uint8_t* pixels = NULL;
	int w = 0, h = 0;
	stream->pending = NULL;
	if (!sample) sample = g_rvn_gst.app_sink_try_pull_sample(stream->sink, 0);
	/* (paused: a seek prerolls a new picture) */
	if (!sample && !stream->playing) sample = g_rvn_gst.app_sink_try_pull_preroll(stream->sink, 0);
	if (!sample) return NULL;
	buffer = g_rvn_gst.sample_get_buffer(sample);
	memset(&map, 0, sizeof(map));
	if (buffer && rvn_gst_sample_size(sample, &w, &h) && g_rvn_gst.buffer_map(buffer, &map, RVN_GST_MAP_READ)) {
		/* (RGBA rows are 4-byte aligned: tight) */
		if (map.data && map.size >= (size_t)w * (size_t)h * 4) {
			pixels = (uint8_t*)rvn_alloc((size_t)w * (size_t)h * 4);
			memcpy(pixels, map.data, (size_t)w * (size_t)h * 4);
			*width = w;
			*height = h;
			stream->width = w;
			stream->height = h;
		}
		g_rvn_gst.buffer_unmap(buffer, &map);
	}
	g_rvn_gst.mini_object_unref(sample);
	return pixels;
}

static const rvn_media_backend_t g_rvn_gst_backend = {
	"GStreamer",
	rvn_gst_available,
	rvn_gst_open,
	rvn_gst_close,
	rvn_gst_play,
	rvn_gst_pause,
	rvn_gst_seek,
	rvn_gst_set_volume,
	rvn_gst_set_rate,
	rvn_gst_poll,
	rvn_gst_take_frame
};

const rvn_media_backend_t* rvn_media_system_backend(void) {
	return &g_rvn_gst_backend;
}
