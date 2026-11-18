/**
 * Raven - render/metal/metal_backend.h
 * State shared by the files of the Metal backend (Objective-C, ARC).
 *
 *   metal_backend.m    the RBackendVTable, Init / Shutdown, WaitIdle, device loss, iOS suspension
 *   metal_pipeline.m   shader libraries, pipeline states, samplers, effects (CreateShader)
 *   metal_texture.m    textures and their uploads
 *   metal_frame.m      surfaces, RenderFrame, viewport callbacks, capture, glass (backdrop + blur)
 *
 * The shaders are Metal Shading Language, compiled when the backend starts
 * from the sources embedded in generated/shaders.h (docs/METAL_BACKEND.md).
 *
 * Everything here runs on the render thread, except rvn_metal_set_suspended
 * (the main thread of iOS) and the completed handlers of the command buffers
 * (a thread of Metal), which only touch what is atomic or behind the
 * suspension lock.
 */

#ifndef RVN_METAL_BACKEND_H
#define RVN_METAL_BACKEND_H

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#include <stdatomic.h>

#include "../renderer.h"

/** Frames recorded ahead of the GPU, per window (and drawables of the layer). */
#define RVN_MTL_FRAMES 3
/** Times the frame is halved to blur the backdrop of glass (as RVN_VK_BLUR_LEVELS). */
#define RVN_MTL_BLUR_LEVELS 3
/** Smallest instance buffer, in instances. */
#define RVN_MTL_MIN_INSTANCES 1024
/** Every render target of the backend: the drawables, the backdrop images, the headless targets. */
#define RVN_MTL_COLOR_FORMAT MTLPixelFormatBGRA8Unorm

/* bindings of the shaders (common.metal) */
#define RVN_MTL_BUFFER_INSTANCES        0
#define RVN_MTL_BUFFER_UNIFORMS         1
#define RVN_MTL_TEXTURE_UI              0
#define RVN_MTL_SAMPLER_UI              0
#define RVN_MTL_TEXTURE_BACKDROP        1
#define RVN_MTL_TEXTURE_BACKDROP_BLUR   2
#define RVN_MTL_BLUR_TEXTURE_SOURCE     0
#define RVN_MTL_BLUR_BUFFER_UNIFORMS    0

/** Size of one RDrawInstance, which the shaders read from a device buffer (RvnInstance). */
#define RVN_MTL_INSTANCE_SIZE 80
/** Built-in effects (REffect), as RVN_MTL_EFFECT_COUNT of generated/shaders.h. */
#define RVN_MTL_EFFECTS 8

/** RvnUniforms of common.metal (setVertexBytes). */
typedef struct rvn_mtl_uniforms_t {
	float scale[2];
	float translate[2];
	float pointer[2];
	float inv_size[2];
} rvn_mtl_uniforms_t;

/** RvnBlurUniforms of common.metal (setFragmentBytes, 16 bytes). */
typedef struct rvn_mtl_blur_uniforms_t {
	float step[2];
	int32_t mode;
	int32_t pad;
} rvn_mtl_blur_uniforms_t;

/* ---------------------------------------------------------------------------
 *  Objects
 * ------------------------------------------------------------------------- */

/** A texture of Raven. */
@interface RvnMtlTexture : NSObject {
@public
	Rid id;
	id<MTLTexture> texture;
	Ruint width, height;
	RTextureFormat format;
	RFilterType filter;
	NSUInteger levels;
	/** ALPHA8 kept as RGBA8 (the device has no swizzle): uploads are expanded to (255, 255, 255, value). */
	int expanded;
}
@end

/** The target of a window: its layer, or an offscreen texture for a headless run. */
@interface RvnMtlSurface : NSObject {
@public
	Rid window;
	/** nil for a headless surface. */
	CAMetalLayer* layer;
	/** The offscreen target of a headless surface. */
	id<MTLTexture> headless;
	/** What the layer was last given. */
	Ruint drawable_width, drawable_height;
	int vsync;
	int transparent;
	/** Frames of this surface the GPU may still read: starts at RVN_MTL_FRAMES. */
	dispatch_semaphore_t slots;
	uint64_t cursor;
	id<MTLBuffer> instances[RVN_MTL_FRAMES];
	/* backdrop of glass: down[0] is the sharp backdrop, up[0] the blurred one */
	id<MTLTexture> down[RVN_MTL_BLUR_LEVELS];
	id<MTLTexture> up[RVN_MTL_BLUR_LEVELS - 1];
	Ruint glass_width, glass_height;
	int glass_ready;
	int glass_filled;
}
@end

/** An effect given to CreateShader. A nil pipeline is drawn with the fallback. */
@interface RvnMtlShader : NSObject {
@public
	Rid id;
	int effect;
	id<MTLRenderPipelineState> pipeline;
}
@end

/* ---------------------------------------------------------------------------
 *  Global state
 * ------------------------------------------------------------------------- */

typedef struct rvn_mtl_t {
	RBackendInitInfo info;
	int initialized;
	id<MTLDevice> device;
	id<MTLCommandQueue> queue;

	/* metal_pipeline.m */
	id<MTLLibrary> ui_library;
	id<MTLLibrary> blur_library;
	id<MTLLibrary> effect_libraries[RVN_MTL_EFFECTS];
	id<MTLRenderPipelineState> effect_pipelines[RVN_MTL_EFFECTS];
	int effect_failed[RVN_MTL_EFFECTS];
	id<MTLRenderPipelineState> ui_pipeline;
	id<MTLRenderPipelineState> erase_pipeline;
	id<MTLRenderPipelineState> fallback_pipeline;
	id<MTLRenderPipelineState> blur_pipeline;
	id<MTLSamplerState> sampler_nearest;
	id<MTLSamplerState> sampler_linear;
	id<MTLSamplerState> sampler_trilinear;
	NSMutableDictionary<NSNumber*, RvnMtlShader*>* shaders;
	int custom_shader_reported;

	/* metal_texture.m */
	NSMutableDictionary<NSNumber*, RvnMtlTexture*>* textures;
	RvnMtlTexture* white;
	int can_swizzle;
	/** Uploads since the last frame: committed before the next one (rvn_mtl_uploads_flush). */
	id<MTLCommandBuffer> upload_commands;
	id<MTLBlitCommandEncoder> upload_blit;

	/* metal_frame.m */
	NSMutableDictionary<NSNumber*, RvnMtlSurface*>* surfaces;

	/* metal_backend.m */
	/** The last command buffer committed (behind the suspension lock). */
	id<MTLCommandBuffer> last_committed;
	/** Set by the completed handlers (another thread). */
	atomic_int device_lost;
	atomic_int failure_reported;
} rvn_mtl_t;

extern rvn_mtl_t g_mtl;

/* ---------------------------------------------------------------------------
 *  metal_backend.m
 * ------------------------------------------------------------------------- */

/** R_TRUE while the backend may draw: initialized, device not lost. */
int rvn_mtl_usable(void);
/** R_TRUE while iOS suspended rendering (rvn_metal_set_suspended). */
int rvn_mtl_suspended(void);
/**
 * Commits `commands` unless rendering is suspended (R_FALSE then: nothing is
 * committed and its completed handlers never run). Commits the pending
 * uploads first. `drawable` (may be nil) is presented by it.
 */
int rvn_mtl_commit(id<MTLCommandBuffer> commands, id<MTLDrawable> drawable);
/** Looks at the status of a completed command buffer: device loss, errors (any thread). */
void rvn_mtl_check_completed(id<MTLCommandBuffer> commands);

/* ---------------------------------------------------------------------------
 *  metal_pipeline.m
 * ------------------------------------------------------------------------- */

int  rvn_mtl_pipeline_init(void);
void rvn_mtl_pipeline_shutdown(void);
int  rvn_mtl_shader_create(Rid id, const uint32_t* spirv, size_t size);
void rvn_mtl_shader_destroy(Rid id);
/** The pipeline that draws an R_DRAW_SHADER command: its effect, else the fallback. */
id<MTLRenderPipelineState> rvn_mtl_shader_pipeline(Rid id);

/* ---------------------------------------------------------------------------
 *  metal_texture.m
 * ------------------------------------------------------------------------- */

int  rvn_mtl_textures_init(void);
void rvn_mtl_textures_shutdown(void);
RvnMtlTexture* rvn_mtl_texture_find(Rid id);
/** The sampler a texture is read with (its RFilterType, and whether it has mip levels). */
id<MTLSamplerState> rvn_mtl_texture_sampler(const RvnMtlTexture* texture);
/** Encodes nothing more into the upload command buffer and hands it back (nil when there is none). */
id<MTLCommandBuffer> rvn_mtl_uploads_take(void);
int  rvn_mtl_texture_create(Rid id, const RTextureDesc* desc, const void* pixels);
void rvn_mtl_texture_update(Rid id, Ruint x, Ruint y, Ruint width, Ruint height, const void* pixels);
void rvn_mtl_texture_set_filter(Rid id, RFilterType filter);
void rvn_mtl_texture_destroy(Rid id);

/* ---------------------------------------------------------------------------
 *  metal_frame.m
 * ------------------------------------------------------------------------- */

int  rvn_mtl_surface_create(Rid window, const RNativeWindow* native, Ruint width, Ruint height);
void rvn_mtl_surface_destroy(Rid window);
void rvn_mtl_surfaces_shutdown(void);
void rvn_mtl_render_frame(const RDrawData* data, RCapture* capture);

#endif /* RVN_METAL_BACKEND_H */
