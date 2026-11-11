/**
 * Raven - render/renderer.h
 * Bridge between the UI thread and the rendering backend.
 *
 * Every backend call is turned into a command and executed, in submission
 * order, by a dedicated render thread. The UI thread never waits for the GPU
 * except for back-pressure: it will not queue more than a couple of frames
 * per window. When the render thread is disabled, commands run inline.
 *
 *      UI thread                               render thread
 *   paint -> frame packet --+
 *   texture create/update --+--> queue --> backend (Vulkan) --> present
 *   surface create/destroy -+
 */

#ifndef RVN_RENDERER_H
#define RVN_RENDERER_H

#include "draw.h"
#include "../core/thread.h"

/* ---------------------------------------------------------------------------
 *  Textures (render/texture.c): UI-thread description of a GPU texture
 * ------------------------------------------------------------------------- */

typedef struct rvn_texture_t {
	Rid id;
	int width, height;
	RTextureFormat format;
	RFilterType filter;
	/** 1 once the pixels were handed to the renderer, or decoding failed. */
	rvn_atomic_t ready;
	/** R_TRUE while the backend texture exists. */
	int created;
	/** Created by a backend-specific function (wrapped image view, shared memory). */
	int external;
	/*
	 * What remakes the texture after the loss of the device (render/device.c):
	 * the file it was read from, or a copy of the encoded file, or a copy of
	 * the pixels of a small texture. None: it comes back transparent.
	 */
	char* source_path;
	uint8_t* source_data;
	size_t source_size;
	uint8_t* kept_pixels;
	/** Texels per logical unit: 1 for a picture, the scale an SVG was drawn at (0 counts as 1). */
	float density;
	/*
	 * An SVG drawn at the size it is shown (no scale asked for): the most
	 * pixels per logical unit it was painted with during the last frame, and
	 * R_TRUE while it is being drawn again at a new scale on a worker.
	 */
	int svg_auto;
	float svg_wanted;
	int svg_pending;
} rvn_texture_t;

rvn_texture_t* rvn_texture_get(Rid id);
/** Allocates a handle with no backend texture yet. */
rvn_texture_t* rvn_texture_register(void);
/** Creates a texture from raw pixels. Takes ownership of `pixels` (rvn_alloc'ed, may be NULL). */
Rid rvn_texture_create_raw(int width, int height, RTextureFormat format, RFilterType filter, void* pixels);
void rvn_textures_shutdown(void);
/** After the loss of the device: makes the backend texture again (see rvn_texture_t). */
void rvn_texture_restore(rvn_texture_t* texture);

/* ---------------------------------------------------------------------------
 *  Shaders (render/shader.c): fragment shaders of effects
 * ------------------------------------------------------------------------- */

typedef struct rvn_shader_t {
	Rid id;
	/** R_SHADER_* flags given at creation (R_SHADER_BACKDROP: reads what is behind). */
	Ruint flags;
	/** The backend accepted it (or the backend has no shaders: drawn as rectangles). */
	int created;
	/** Built-in effect it stands for, or -1. */
	int effect;
	/** Its SPIR-V, to make it again after the loss of the device. */
	char* source_glsl; /* Browser effect source, when supplied instead of SPIR-V. */
	uint32_t* spirv;
	size_t spirv_size;
} rvn_shader_t;

rvn_shader_t* rvn_shader_get(Rid id);
void rvn_shaders_shutdown(void);
/** After the loss of the device: gives the shader to the backend again. */
void rvn_shader_restore(rvn_shader_t* shader);
/** The shader of a .spv file, made once per path and flags and kept until the end (the Effect attribute). */
Rid rvn_shader_from_file_cached(const char* path, Ruint flags);
/** The SPIR-V words are rvn_alloc'ed; the renderer takes ownership. */
void rvn_renderer_create_shader(Rid shader, uint32_t* spirv, size_t size);
void rvn_renderer_destroy_shader(Rid shader);

/** Decodes an image file into RGBA8 (render/image_decode.c). Free with rvn_free. */
uint8_t* rvn_image_decode(const uint8_t* data, size_t size, int* width, int* height);
/**
 * Same; an SVG is rasterized at `svg_scale` pixels per unit of the SVG (0 or
 * less: RVN_SVG_DENSITY). `density`, when given, receives the pixels per
 * logical unit of the result: the scale of an SVG, 1 for the other images.
 */
uint8_t* rvn_image_decode_at(const uint8_t* data, size_t size, float svg_scale, int* width, int* height, float* density);
/** Pixels per unit an SVG is first drawn at, before it has been shown (then: at the size it is shown, rvn_textures_update_svg). */
#define RVN_SVG_DENSITY 2.0f
/**
 * Once a frame: draws again, on a worker, the SVGs that were painted bigger
 * than their pixels (a screen of a higher density, a stretched Image, a zoom)
 * or much smaller.
 */
void rvn_textures_update_svg(void);
/* ---- render/svg_decode.c (nanosvg) ---- */
int rvn_svg_detect(const uint8_t* data, size_t size);
uint8_t* rvn_svg_decode(const uint8_t* data, size_t size, float scale, int* width, int* height);
/** Downloads and decodes an HTTP(S) image asynchronously. Internal resource helper. */
Rid rvn_texture_create_from_url_async(const char* url);

/* ---------------------------------------------------------------------------
 *  Renderer (render/renderer.c)
 * ------------------------------------------------------------------------- */

int  rvn_renderer_init(const RBackendVTable* backend, const RBackendInitInfo* info, int threaded);
void rvn_renderer_shutdown(void);
int  rvn_renderer_is_threaded(void);
/**
 * Sets a function called every few milliseconds while the UI thread is
 * blocked on the render thread (rvn_platform_t::wait_service). NULL = none.
 * Call after rvn_renderer_init.
 */
void rvn_renderer_set_wait_service(void (*service)(void));

/** Blocks until the surface exists. */
int  rvn_renderer_create_surface(Rid window, const RNativeWindow* native, uint32_t width, uint32_t height);
/** Blocks until every queued frame of the window is rendered and the surface destroyed. */
void rvn_renderer_destroy_surface(Rid window);

/** The pixel buffers are rvn_alloc'ed; the renderer takes ownership and frees them. */
void rvn_renderer_create_texture(Rid texture, const RTextureDesc* desc, void* pixels);
void rvn_renderer_update_texture(Rid texture, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void* pixels);
void rvn_renderer_set_texture_filter(Rid texture, RFilterType filter);
void rvn_renderer_destroy_texture(Rid texture);

typedef void (*rvn_render_fn)(void* arg);
/** Runs a function on the render thread, in order with the other commands. */
void rvn_renderer_call(rvn_render_fn function, void* arg, int wait);
/** Blocks until every command queued so far has been executed. */
void rvn_renderer_sync(void);

/** R_TRUE once the backend reported the loss of its device (Rbackend_device_lost). */
int rvn_renderer_device_lost(void);
/**
 * Shuts the backend down and initializes it again (after the loss of its
 * device). Every surface, texture and shader of the backend is gone
 * afterwards: the caller makes them again (render/device.c).
 */
int rvn_renderer_restart(const RBackendInitInfo* info);
/** Remakes everything after the loss of the device, if it was lost (render/device.c). UI thread. */
void rvn_device_check(void);

/** One frame of one window travelling from the UI thread to the render thread. */
typedef struct rvn_frame_t {
	rvn_drawlist_t list;
	RDrawData data;
	/** Optional: receives the rendered pixels. */
	RCapture* capture;
	/** Optional counters owned by the window. */
	rvn_atomic_t* frames_in_flight;
	rvn_atomic_t* render_us;
	struct rvn_frame_t* next;
} rvn_frame_t;

/** Returns an empty frame packet (recycled, so its arrays keep their capacity). */
rvn_frame_t* rvn_renderer_acquire_frame(void);
/** Blocks while `frames_in_flight` is at least `max_frames`. */
void rvn_renderer_throttle(rvn_atomic_t* frames_in_flight, int max_frames);
/** Queues the frame. `frame->data` must be filled except for the instance / command arrays. */
void rvn_renderer_submit_frame(rvn_frame_t* frame);

/* Built-in backends */
#if defined(RAVEN_BACKEND_WEBGL_ENABLED)
const RBackendVTable* rvn_backend_webgl(void);
#endif
const RBackendVTable* rvn_backend_null(void);
#if defined(RAVEN_BACKEND_VULKAN_ENABLED)
const RBackendVTable* rvn_backend_vulkan(void);
#endif

#endif /* RVN_RENDERER_H */
