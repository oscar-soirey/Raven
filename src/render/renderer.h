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
	 * (svg_wanted holds the bits of a float that is never negative: paint
	 * jobs on several threads raise it at once, and those bits order like it.)
	 */
	int svg_auto;
	rvn_atomic_t svg_wanted;
	int svg_pending;
	/** An SVG painted only in currentColor, drawn in white: the widgets that show it tint it with their text color. */
	int current_color;
	/**
	 * An animated GIF or APNG: its frames, which the Image widgets that show
	 * it play (widgets/image.c). The texture itself holds the first frame.
	 */
	struct rvn_anim_t* anim;
	/** Times the texture was made again after the loss of the device (its pixels may be gone). */
	uint32_t restores;
} rvn_texture_t;

rvn_texture_t* rvn_texture_get(Rid id);
/** R_TRUE when the texture is an SVG painted only in currentColor (rvn_texture_t.current_color). */
int rvn_texture_is_current_color(Rid id);
/**
 * The tint a widget gives an icon: its text color `foreground` when it asks
 * for it (`tint`) or when the icon is drawn in currentColor, else `plain`.
 */
rvn_color_t rvn_icon_tint(Rid texture, int tint, rvn_color_t foreground, rvn_color_t plain);
/** Allocates a handle with no backend texture yet. */
rvn_texture_t* rvn_texture_register(void);
/** Creates a texture from raw pixels. Takes ownership of `pixels` (rvn_alloc'ed, may be NULL). */
Rid rvn_texture_create_raw(int width, int height, RTextureFormat format, RFilterType filter, void* pixels);
void rvn_textures_shutdown(void);
/** Gives new pixels of the same size to a texture made by rvn_texture_create_raw. Takes ownership of `pixels`. No repaint is asked. */
void rvn_texture_replace_pixels(Rid id, uint8_t* pixels);
/** Destroys a texture made by rvn_texture_create_raw, without asking the windows to be painted again. */
void rvn_texture_release(Rid id);
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
/** Index of the built-in effect whose SPIR-V this is (REffect - R_EFFECT_AURORA), or -1. Thread-safe. */
int rvn_effect_index_of(const uint32_t* spirv, size_t size);
/** The SPIR-V words are rvn_alloc'ed; the renderer takes ownership. */
void rvn_renderer_create_shader(Rid shader, uint32_t* spirv, size_t size);
void rvn_renderer_destroy_shader(Rid shader);

/** Encodes RGBA8 pixels as a PNG file (render/image_encode.c). NULL without pixels. Free with rvn_free. */
uint8_t* rvn_png_encode(const uint8_t* rgba, int width, int height, size_t* size);
/** Decodes an image file into RGBA8 (render/image_decode.c). Free with rvn_free. */
uint8_t* rvn_image_decode(const uint8_t* data, size_t size, int* width, int* height);
/**
 * Same; an SVG is rasterized at `svg_scale` pixels per unit of the SVG (0 or
 * less: RVN_SVG_DENSITY). `density`, when given, receives the pixels per
 * logical unit of the result: the scale of an SVG, 1 for the other images.
 * `current_color`, when given, receives R_TRUE for an SVG painted only in
 * currentColor, drawn in white: a mask that widgets tint with their text color.
 */
uint8_t* rvn_image_decode_at(const uint8_t* data, size_t size, float svg_scale, int* width, int* height, float* density, int* current_color);
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
uint8_t* rvn_svg_decode(const uint8_t* data, size_t size, float scale, int* width, int* height, int* current_color);

/* ---- render/anim_decode.c: animated GIF and APNG ---- */
#define RVN_ANIM_GIF 1
#define RVN_ANIM_APNG 2
/** What happens to the rectangle of a frame before the next one is drawn. */
#define RVN_ANIM_DISPOSE_NONE 0
#define RVN_ANIM_DISPOSE_BACKGROUND 1 /* made transparent */
#define RVN_ANIM_DISPOSE_PREVIOUS 2   /* given back what was there before the frame */
/** How a frame is drawn: replacing its rectangle, or over it. */
#define RVN_ANIM_BLEND_SOURCE 0
#define RVN_ANIM_BLEND_OVER 1

typedef struct rvn_anim_frame_t {
	int x, y, width, height;
	/** How long it is shown (ms), after the browser rule (10 ms or less: 100 ms). */
	uint32_t delay_ms;
	uint8_t dispose, blend, interlaced;
	/** GIF: the index of the transparent color, or -1; its color table (offset in the file, colors). */
	int transparent;
	uint32_t palette, palette_count;
	/** GIF: offset of the LZW code size. APNG: first of its `segments` in rvn_anim_t.segments. */
	uint32_t offset, segments;
} rvn_anim_frame_t;

/** A run of compressed pixels of an APNG frame (IDAT or fdAT data) in the file. */
typedef struct rvn_anim_segment_t {
	uint32_t offset, length;
} rvn_anim_segment_t;

/** An animated image: the structure of its file, which it keeps (or refers to); no decoded frame. Immutable once open. */
typedef struct rvn_anim_t {
	int format;
	int width, height;
	int frame_count;
	/** APNG: R_FALSE when the image of the file is not one of the frames (then even one frame differs from it). */
	int image_is_frame;
	/** Times it plays: 0 forever (GIF: 1 + the repeats of its NETSCAPE2.0 block; APNG: num_plays). */
	int loop_count;
	/** Sum of the delays (ms). */
	uint32_t duration_ms;
	/** Different for every animation opened (never 0). */
	uint32_t serial;
	const uint8_t* data;
	size_t size;
	uint8_t* owned;
	rvn_anim_frame_t* frames;
	/* APNG: the data of the frames; offsets of its IHDR data and its PLTE and tRNS chunks (whole) */
	rvn_anim_segment_t* segments;
	uint32_t segment_count;
	uint32_t header, palette, palette_length, transparency, transparency_length;
} rvn_anim_t;

/** Composes frames into a canvas of the size of the animation (RGBA8, straight alpha). Zero it, set index to -1. */
typedef struct rvn_anim_cursor_t {
	uint8_t* canvas;
	/** The rectangle under the last frame that restores it. */
	uint8_t* saved;
	/** Frame in the canvas (-1: none), of the animation of this serial. */
	int index;
	uint32_t serial;
} rvn_anim_cursor_t;

/** R_TRUE for a GIF, and for a PNG with an animation control chunk. Does not mean it has several frames. */
int rvn_anim_detect(const uint8_t* data, size_t size);
/**
 * Reads the structure of a GIF or APNG of at least two frames (or an APNG
 * whose one frame is not the image of the file: a still image that only
 * this decoder shows right). NULL for anything else (a still image, a
 * damaged header, a canvas too big). With
 * `copy`, the animation keeps a copy of the file; otherwise the data must
 * outlive it. Thread-safe.
 */
rvn_anim_t* rvn_anim_open(const uint8_t* data, size_t size, int copy);
void rvn_anim_free(rvn_anim_t* anim);
/** Composes frame `index` (clamped) into the canvas of the cursor, going on from the frame it holds, or from the first. */
const uint8_t* rvn_anim_compose(const rvn_anim_t* anim, rvn_anim_cursor_t* cursor, int index);
void rvn_anim_cursor_free(rvn_anim_cursor_t* cursor);
/** Frame `index` as a whole image (width x height RGBA8). Free with rvn_free. */
uint8_t* rvn_anim_decode_frame(const rvn_anim_t* anim, int index);
/**
 * rvn_image_decode_at that also gives the animation of an animated GIF or
 * APNG (`anim`, may be NULL; then the pixels are its first frame). The
 * animation keeps a copy of the file.
 */
uint8_t* rvn_image_decode_anim(const uint8_t* data, size_t size, float svg_scale, int* width, int* height, float* density,
	int* current_color, rvn_anim_t** anim);


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
/** Blocks until every command queued so far has been executed and the GPU has finished all its work (the application goes to the background). */
void rvn_renderer_wait_idle(void);
/** Frees the draw lists kept for the next frames (low memory): they are made again when needed. */
void rvn_renderer_trim(void);

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
#if defined(RAVEN_BACKEND_METAL_ENABLED)
const RBackendVTable* rvn_backend_metal(void);
/**
 * iOS: R_TRUE stops every commit of the Metal backend and waits until the GPU
 * is idle (the application goes to the background); R_FALSE draws again.
 * Called by the UIKit platform from the main thread.
 */
void rvn_metal_set_suspended(int suspended);
#endif

#endif /* RVN_RENDERER_H */
