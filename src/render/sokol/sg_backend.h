/**
 * Raven - render/sokol/sg_backend.h
 * State shared by the files of the sokol_gfx backend.
 *
 *   sg_impl.c       the one translation unit that compiles sokol_gfx.h (every function prefixed rvn_)
 *   sg_egl.c        GLES3 context (EGL, surfaceless) and read-back; the native-API part of the backend (Linux)
 *   sg_metal.m      the same for Metal (macOS, iOS)
 *   sg_texture.c    textures, queued uploads
 *   sg_pipeline.c   shaders, pipelines, effects
 *   sg_glass.c      render targets, backdrop of the glass primitives
 *   sg_backend.c    the RBackendVTable, surfaces, frames
 *
 * Raven draws in "Vulkan convention": pixel row 0 is the top of the UI and NDC y points down.
 *  - GL (origin bottom left, NDC y up): that stores the UI top in framebuffer row 0, so offscreen images,
 *    backdrop sampling, gl_FragCoord and read-back need no flip (only presenting to a GL default
 *    framebuffer does).
 *  - Metal, D3D11, WebGPU (origin top left, NDC y up): the transform flips y (rvn_sg_flip_y()), and the
 *    shaders are generated with the two adjustments that follow from it (gen_shaders.py).
 * Viewport and scissor rectangles are given in memory-row space: see rvn_sg_origin().
 *
 * Everything here runs on the render thread.
 */

#ifndef RVN_SG_BACKEND_H
#define RVN_SG_BACKEND_H

#include "../renderer.h"
#include "sg_prefix.h"
#include "../../../third_party/sokol/sokol_gfx.h"

/** Textures and effect shaders, indexed by the index part of their handle. */
#define RVN_SG_MAX_TEXTURES 4096
#define RVN_SG_MAX_EFFECTS 256
#define RVN_SG_MAX_SURFACES 256
/** Times the frame is halved to blur the backdrop of glass: 3 gives a blur of about 16 pixels. */
#define RVN_SG_BLUR_LEVELS 3

typedef struct rvn_sg_texture_t {
	int used;
	int width, height;
	RTextureFormat format;
	RFilterType filter;
	/**
	 * Mip levels of the image (1: only the first). Every RGBA8 texture of at least 16 x 16 pixels has room for the
	 * whole chain, so that the filter can become R_FILTER_MIPMAP later; the levels are made (on the GPU, by
	 * rvn_sg_mips_generate) only while it is.
	 */
	int mips;
	/** The levels must be made again before the next frame. */
	int mips_pending;
	sg_image image;
	sg_view view;
} rvn_sg_texture_t;

/** An image the backend renders to and then samples. */
typedef struct rvn_sg_target_t {
	sg_image image;
	/** Render into it. */
	sg_view color;
	/** Sample it. */
	sg_view sample;
	int width, height;
} rvn_sg_target_t;

typedef struct rvn_sg_surface_t {
	int used;
	Rid window;
	int width, height;
	/** The window can be seen through: the frame carries alpha. */
	int transparent;
	/** The frame itself; presented to the window from here, and read back by Rwindow_capture. */
	rvn_sg_target_t frame;
	/** The native presentation target of the window (sg_egl.c: EGL surface, sg_metal.m: layer), or NULL: headless. */
	void* present;

	/* glass (sg_glass.c): created the first time a frame of the window contains a backdrop command */
	int glass_ready;
	/** The backdrop images hold a whole capture: the next ones may only refresh an area. */
	int glass_filled;
	/** The frame while it is drawn: glass cannot read the image it is drawn into. */
	rvn_sg_target_t scene;
	/** Halved copies of the frame; the first one is the "sharp" backdrop. */
	rvn_sg_target_t down[RVN_SG_BLUR_LEVELS];
	/** Enlarged back; the first one is the blurred backdrop. */
	rvn_sg_target_t up[RVN_SG_BLUR_LEVELS - 1];
} rvn_sg_surface_t;

/** A fragment shader of an effect (raven_shader.h) and its pipeline. */
typedef struct rvn_sg_effect_t {
	int used;
	sg_shader shader;
	sg_pipeline pipeline;
} rvn_sg_effect_t;

typedef struct rvn_sg_state_t {
	int initialized;
	RBackendInitInfo info;

	/* sg_pipeline.c */
	sg_shader ui_shader;
	/** The UI: blend (ONE, 1 - SRC_ALPHA). */
	sg_pipeline ui_pipeline;
	/** The same, removing instead of drawing (R_DRAW_ERASE): blend (ZERO, 1 - SRC_ALPHA). */
	sg_pipeline erase_pipeline;
	/** Full-screen triangle that reduces, enlarges or copies (blur.frag). */
	sg_shader blur_shader;
	sg_pipeline blur_pipeline;
	/** The same, drawing into the swapchain of a window (its pixel format is the one of the window system). */
	sg_pipeline present_pipeline;
	sg_sampler linear, nearest;
	/** Trilinear: the levels of a texture with mipmaps. */
	sg_sampler mipmap;
	rvn_sg_effect_t effects[RVN_SG_MAX_EFFECTS];

	/* sg_texture.c */
	rvn_sg_texture_t textures[RVN_SG_MAX_TEXTURES];
	/** 1x1 white texture bound when a command has no texture. */
	rvn_sg_texture_t white;

	/* sg_backend.c */
	rvn_sg_surface_t surfaces[RVN_SG_MAX_SURFACES];
	/** One transient vertex buffer holding the instances of the frame being recorded. */
	sg_buffer instances;
	size_t instances_size;
} rvn_sg_state_t;

extern rvn_sg_state_t g_sg;

/* ---------------------------------------------------------------------------
 *  sg_egl.c : the native API
 * ------------------------------------------------------------------------- */

/**
 * Creates the graphics context (GL: made current on this thread; Metal: the device) and fills what sg_setup
 * needs of the native API in `environment`. R_FALSE: no usable GPU/driver.
 */
int  rvn_sg_context_init(const RBackendInitInfo* info, sg_environment* environment);
void rvn_sg_context_shutdown(void);
/** Blocks until the GPU finished all submitted work. */
void rvn_sg_context_finish(void);
/** RGBA8 pixels of a target, top row first, allocated with Rpixels_alloc. Calls sg_reset_state_cache(). */
uint8_t* rvn_sg_read_target(const rvn_sg_target_t* target);

/**
 * Autorelease pool of the render thread (Metal: Objective-C objects made by a call live until the pool is
 * popped; GL: nothing). Every entry of the backend table runs between a push and a pop.
 */
void* rvn_sg_pool_push(void);
void  rvn_sg_pool_pop(void* pool);

/**
 * Presentation of a window (the part of the native API; NULL `present` of a surface = headless):
 *  create   makes the native target of the window from its handles. R_FALSE: the window is not presented
 *           (its frames are still rendered and can be captured).
 *  begin    makes it current and fills the swapchain of the pass that draws the frame into it. R_FALSE: skip
 *           this frame (a window that is hidden has no drawable).
 *  end      after sg_commit(): swaps buffers (GL); Metal presents in sg_commit() itself.
 */
int  rvn_sg_present_create(rvn_sg_surface_t* surface, const RNativeWindow* native);
void rvn_sg_present_destroy(rvn_sg_surface_t* surface);
int  rvn_sg_present_begin(rvn_sg_surface_t* surface, int vsync, sg_swapchain* swapchain);
void rvn_sg_present_end(rvn_sg_surface_t* surface);

/** sg_impl.c: R_TRUE for a log item of sokol_gfx that is not worth reporting (see there). */
int rvn_sg_log_is_harmless(uint32_t item);

/** Argument `origin_top_left` of sg_apply_viewport / sg_apply_scissor_rect: rectangles in memory-row space. */
bool rvn_sg_origin(void);
/** R_TRUE when y of the transform must be flipped (the API has its origin at the top left, NDC y up). */
bool rvn_sg_flip_y(void);

/* ---------------------------------------------------------------------------
 *  sg_texture.c
 * ------------------------------------------------------------------------- */

int  rvn_sg_textures_init(void);
void rvn_sg_textures_shutdown(void);
int  rvn_sg_texture_create(Rid texture, const RTextureDesc* desc, const void* pixels);
void rvn_sg_texture_update(Rid texture, Ruint x, Ruint y, Ruint width, Ruint height, const void* pixels);
void rvn_sg_texture_set_filter(Rid texture, RFilterType filter);
void rvn_sg_texture_destroy(Rid texture);
/** The texture of a handle, or the white one. */
const rvn_sg_texture_t* rvn_sg_texture_find(Rid texture);
/**
 * Writes the queued uploads into their images: all the writes of a frame must come before the first copy
 * (sokol_gfx), so they are done together, outside a pass, at the start of a frame.
 */
void rvn_sg_uploads_flush(void);
/** The levels (after the first) of the textures whose pixels changed: halves of the previous level, made by passes of the GPU. */
void rvn_sg_mips_generate(void);

/* ---------------------------------------------------------------------------
 *  sg_pipeline.c
 * ------------------------------------------------------------------------- */

int  rvn_sg_pipelines_init(void);
void rvn_sg_pipelines_shutdown(void);
int  rvn_sg_shader_create(Rid shader, const uint32_t* spirv, size_t size);
void rvn_sg_shader_destroy(Rid shader);
/** The pipeline of an effect, or an invalid one. */
const rvn_sg_effect_t* rvn_sg_effect_find(Rid shader);

/* ---------------------------------------------------------------------------
 *  sg_glass.c
 * ------------------------------------------------------------------------- */

int  rvn_sg_target_create(rvn_sg_target_t* target, int width, int height, const char* label);
/**
 * Filters `source` into `target`: all of it, or only `area` (pixels of the target), the rest keeping its content.
 * mode: 0 = reduce, 1 = enlarge, 2 = copy with the linear sampler (blur.frag). Outside a pass.
 */
void rvn_sg_glass_filter(const rvn_sg_target_t* source, const rvn_sg_target_t* target, int mode, float spread, const int* area);
void rvn_sg_target_destroy(rvn_sg_target_t* target);
/** Creates the offscreen images of a window for its current size. R_FALSE: draw without glass. */
int  rvn_sg_glass_prepare(rvn_sg_surface_t* surface);
void rvn_sg_glass_release(rvn_sg_surface_t* surface);
/** Between two scene passes: copies and blurs the frame drawn so far into the backdrop images. */
void rvn_sg_glass_capture(rvn_sg_surface_t* surface, float scale, const int rect[4]);
/** Draws the finished scene into the frame. */
void rvn_sg_glass_present(rvn_sg_surface_t* surface);

#endif /* RVN_SG_BACKEND_H */
