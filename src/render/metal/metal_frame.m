/**
 * Raven - render/metal/metal_frame.m
 * Surfaces, and one frame of one window.
 *
 *   1. wait for the frame slot (three frames can be on the GPU per window)
 *   2. copy the instances into the shared buffer of the slot
 *   3. take the next drawable of the layer
 *   4. one render pass on it: one instanced draw per command, viewport
 *      callbacks, and at each R_DRAW_BACKDROP a pause in which the backdrop
 *      of glass is captured and blurred
 *   5. present, commit
 *
 * Unlike the Vulkan backend, a frame with glass is drawn straight into the
 * drawable too: the layer is made with framebufferOnly = NO, so a drawable can
 * be sampled once its render pass has ended.
 */

#include "metal_backend.h"

@implementation RvnMtlSurface
@end

/* ---------------------------------------------------------------------------
 *  Surfaces
 * ------------------------------------------------------------------------- */

static RvnMtlSurface* rvn_mtl_surface_find(Rid window) {
	RvnMtlSurface* surface = g_mtl.surfaces[@(R_ID_INDEX(window))];
	return surface && surface->window == window ? surface : nil;
}

/* The offscreen target of a headless surface, or a backdrop image. */
static id<MTLTexture> rvn_mtl_target(Ruint width, Ruint height) {
	MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:RVN_MTL_COLOR_FORMAT
		width:width height:height mipmapped:NO];
	descriptor.storageMode = MTLStorageModePrivate;
	descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
	return [g_mtl.device newTextureWithDescriptor:descriptor];
}

/* Gives the layer a size and a synchronization. Inside a transaction: the render thread has no run loop to commit one. */
static void rvn_mtl_layer_configure(RvnMtlSurface* surface, Ruint width, Ruint height, int vsync, int first) {
	CAMetalLayer* layer = surface->layer;
	[CATransaction begin];
	[CATransaction setDisableActions:YES];
	if (first) {
		layer.device = g_mtl.device;
		layer.pixelFormat = RVN_MTL_COLOR_FORMAT;
		/* the drawables are sampled (glass) and copied (capture) */
		layer.framebufferOnly = NO;
		layer.maximumDrawableCount = RVN_MTL_FRAMES;
		/* the colors of the UI are sRGB values, as with Vulkan's SRGB_NONLINEAR: the window server shows them as such */
#if TARGET_OS_OSX
		{
			CGColorSpaceRef srgb = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
			layer.colorspace = srgb;
			CGColorSpaceRelease(srgb);
		}
#else
		if (@available(iOS 16.0, *)) {
			CGColorSpaceRef srgb = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
			layer.colorspace = srgb;
			CGColorSpaceRelease(srgb);
		}
#endif
	}
#if TARGET_OS_OSX
	layer.displaySyncEnabled = vsync ? YES : NO;
#endif
	layer.drawableSize = CGSizeMake(width, height);
	[CATransaction commit];
	surface->drawable_width = width;
	surface->drawable_height = height;
	surface->vsync = vsync;
}

int rvn_mtl_surface_create(Rid window, const RNativeWindow* native, Ruint width, Ruint height) {
	RvnMtlSurface* surface;
	int i;

	if (!g_mtl.initialized) return R_FALSE;
	surface = [RvnMtlSurface new];
	surface->window = window;
	surface->transparent = native->transparent;
	surface->slots = dispatch_semaphore_create(RVN_MTL_FRAMES);

	switch (native->system) {
		case R_WINDOW_SYSTEM_COCOA:
		case R_WINDOW_SYSTEM_UIKIT: {
			id object = (__bridge id)(void*)native->window;
			if (!object || ![object isKindOfClass:[CAMetalLayer class]]) {
				rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Metal: the window has no CAMetalLayer");
				return R_FALSE;
			}
			surface->layer = (CAMetalLayer*)object;
			rvn_mtl_layer_configure(surface, rvn_maxu(width, 1), rvn_maxu(height, 1), g_mtl.info.vsync, R_TRUE);
			break;
		}
		case R_WINDOW_SYSTEM_NONE:
			/* a headless run: frames are rendered (and can be captured), nothing is presented */
			surface->drawable_width = rvn_maxu(width, 1);
			surface->drawable_height = rvn_maxu(height, 1);
			surface->headless = rvn_mtl_target(surface->drawable_width, surface->drawable_height);
			if (!surface->headless) {
				rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Metal: cannot create an offscreen target of %ux%u", width, height);
				return R_FALSE;
			}
			break;
		default:
			rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Metal: the window system 0x%x is not one Metal can draw into", (unsigned)native->system);
			return R_FALSE;
	}
	for (i = 0; i < RVN_MTL_FRAMES; i++) surface->instances[i] = nil;
	g_mtl.surfaces[@(R_ID_INDEX(window))] = surface;
	return R_TRUE;
}

/* Waits until the GPU has finished every frame of the surface: takes every slot, then gives them back. */
static void rvn_mtl_surface_wait(RvnMtlSurface* surface) {
	int i;
	for (i = 0; i < RVN_MTL_FRAMES; i++) dispatch_semaphore_wait(surface->slots, DISPATCH_TIME_FOREVER);
	for (i = 0; i < RVN_MTL_FRAMES; i++) dispatch_semaphore_signal(surface->slots);
}

void rvn_mtl_surface_destroy(Rid window) {
	RvnMtlSurface* surface = rvn_mtl_surface_find(window);
	if (!surface) return;
	rvn_mtl_surface_wait(surface);
	[g_mtl.surfaces removeObjectForKey:@(R_ID_INDEX(window))];
}

void rvn_mtl_surfaces_shutdown(void) {
	for (RvnMtlSurface* surface in g_mtl.surfaces.allValues) rvn_mtl_surface_wait(surface);
	g_mtl.surfaces = nil;
}

/* ---------------------------------------------------------------------------
 *  Glass: the backdrop and its blur (the passes of vk_glass.c)
 * ------------------------------------------------------------------------- */

static void rvn_mtl_glass_release(RvnMtlSurface* surface) {
	int i;
	for (i = 0; i < RVN_MTL_BLUR_LEVELS; i++) surface->down[i] = nil;
	for (i = 0; i < RVN_MTL_BLUR_LEVELS - 1; i++) surface->up[i] = nil;
	surface->glass_ready = R_FALSE;
	surface->glass_filled = R_FALSE;
	surface->glass_width = surface->glass_height = 0;
}

/* Makes the backdrop images for a target of this size. R_FALSE: glass shows white. */
static int rvn_mtl_glass_prepare(RvnMtlSurface* surface, Ruint width, Ruint height) {
	int i;
	if (surface->glass_ready && surface->glass_width == width && surface->glass_height == height) return R_TRUE;
	/* (the frames that used the old images keep them alive in their command buffers) */
	rvn_mtl_glass_release(surface);
	for (i = 0; i < RVN_MTL_BLUR_LEVELS; i++) {
		Ruint w = rvn_maxu(width >> (i + 1), 1), h = rvn_maxu(height >> (i + 1), 1);
		surface->down[i] = rvn_mtl_target(w, h);
		if (!surface->down[i]) goto failed;
		/* enlarged copies exist for every size but the smallest */
		if (i + 1 < RVN_MTL_BLUR_LEVELS) {
			surface->up[i] = rvn_mtl_target(w, h);
			if (!surface->up[i]) goto failed;
		}
	}
	surface->glass_ready = R_TRUE;
	surface->glass_filled = R_FALSE;
	surface->glass_width = width;
	surface->glass_height = height;
	return R_TRUE;

failed:
	rvn_mtl_glass_release(surface);
	return R_FALSE;
}

/*
 * Filters `source` into `target`: all of it, or only `area` (pixels of the
 * target), the rest keeping its content. mode: 0 = reduce, 1 = enlarge (blur.metal).
 */
static void rvn_mtl_glass_filter(id<MTLCommandBuffer> commands, id<MTLTexture> source, id<MTLTexture> target,
	int mode, float spread, const MTLScissorRect* area) {
	MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
	id<MTLRenderCommandEncoder> encoder;
	rvn_mtl_blur_uniforms_t uniforms;
	MTLViewport viewport;

	pass.colorAttachments[0].texture = target;
	pass.colorAttachments[0].loadAction = area ? MTLLoadActionLoad : MTLLoadActionDontCare;
	pass.colorAttachments[0].storeAction = MTLStoreActionStore;
	encoder = [commands renderCommandEncoderWithDescriptor:pass];
	if (!encoder) return;

	/* offsets are in texels of the source: a whole one when reducing, halves when enlarging */
	uniforms.step[0] = spread * (mode == 0 ? 1.0f : 0.5f) / (float)source.width;
	uniforms.step[1] = spread * (mode == 0 ? 1.0f : 0.5f) / (float)source.height;
	uniforms.mode = mode;
	uniforms.pad = 0;

	viewport.originX = 0.0;
	viewport.originY = 0.0;
	viewport.width = (double)target.width;
	viewport.height = (double)target.height;
	viewport.znear = 0.0;
	viewport.zfar = 1.0;
	[encoder setRenderPipelineState:g_mtl.blur_pipeline];
	[encoder setViewport:viewport];
	if (area) [encoder setScissorRect:*area];
	[encoder setFragmentTexture:source atIndex:RVN_MTL_BLUR_TEXTURE_SOURCE];
	[encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:RVN_MTL_BLUR_BUFFER_UNIFORMS];
	[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
	[encoder endEncoding];
}

/* The part of `target` that shows the pixels x0..x1, y0..y1 of the frame (rvn_vk_glass_area). R_FALSE: nothing. */
static int rvn_mtl_glass_area(Ruint frame_width, Ruint frame_height, id<MTLTexture> target,
	float x0, float y0, float x1, float y1, MTLScissorRect* area) {
	float fx = (float)target.width / (float)frame_width;
	float fy = (float)target.height / (float)frame_height;
	int ax0 = rvn_maxi((int)floorf(x0 * fx), 0), ay0 = rvn_maxi((int)floorf(y0 * fy), 0);
	int ax1 = rvn_mini((int)ceilf(x1 * fx), (int)target.width), ay1 = rvn_mini((int)ceilf(y1 * fy), (int)target.height);
	if (ax1 <= ax0 || ay1 <= ay0) return R_FALSE;
	area->x = (NSUInteger)ax0;
	area->y = (NSUInteger)ay0;
	area->width = (NSUInteger)(ax1 - ax0);
	area->height = (NSUInteger)(ay1 - ay0);
	return R_TRUE;
}

/* Captures the frame drawn so far into the backdrop images (rvn_vk_glass_capture, same numbers). */
static void rvn_mtl_glass_capture(id<MTLCommandBuffer> commands, RvnMtlSurface* surface, id<MTLTexture> frame,
	float scale, const int rect[4]) {
	/* the blur is sized in pixels: follow the scale of the window (within reason) */
	float spread = rvn_clampf(scale, 1.0f, 3.0f);
	/* a pixel of the blurred image depends on the frame up to about 18 x spread pixels away, plus the bilinear samples */
	float margin = 18.0f * spread + 20.0f;
	float x0 = (float)rect[0] - margin, y0 = (float)rect[1] - margin;
	float x1 = (float)(rect[0] + rect[2]) + margin, y1 = (float)(rect[1] + rect[3]) + margin;
	float width = (float)frame.width, height = (float)frame.height;
	int partial, i;

	if (rect[2] <= 0 || rect[3] <= 0) {
		/* no area given: everything */
		x0 = y0 = 0.0f;
		x1 = width;
		y1 = height;
	}
	x0 = rvn_maxf(x0, 0.0f);
	y0 = rvn_maxf(y0, 0.0f);
	x1 = rvn_minf(x1, width);
	y1 = rvn_minf(y1, height);
	if (x1 <= x0 || y1 <= y0) return;
	/* an area is only worth its bookkeeping when it is clearly smaller than the frame,
	 * and only possible once the images hold a whole capture */
	partial = surface->glass_filled && (x1 - x0) * (y1 - y0) < width * height * 0.8f;

	/* frame -> 1/2 -> 1/4 -> 1/8: down[0] doubles as the sharp backdrop */
	for (i = 0; i < RVN_MTL_BLUR_LEVELS; i++) {
		id<MTLTexture> source = i == 0 ? frame : surface->down[i - 1];
		MTLScissorRect area;
		if (partial && !rvn_mtl_glass_area((Ruint)frame.width, (Ruint)frame.height, surface->down[i], x0, y0, x1, y1, &area)) continue;
		rvn_mtl_glass_filter(commands, source, surface->down[i], 0, i == 0 ? 1.0f : spread, partial ? &area : NULL);
	}
	/* ... and back up to half size: up[0] is the blurred backdrop */
	for (i = RVN_MTL_BLUR_LEVELS - 2; i >= 0; i--) {
		id<MTLTexture> source = i == RVN_MTL_BLUR_LEVELS - 2 ? surface->down[RVN_MTL_BLUR_LEVELS - 1] : surface->up[i + 1];
		MTLScissorRect area;
		if (partial && !rvn_mtl_glass_area((Ruint)frame.width, (Ruint)frame.height, surface->up[i], x0, y0, x1, y1, &area)) continue;
		rvn_mtl_glass_filter(commands, source, surface->up[i], 1, spread, partial ? &area : NULL);
	}
	surface->glass_filled = R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Commands
 * ------------------------------------------------------------------------- */

/* What the commands of a frame share. */
typedef struct rvn_mtl_pass_t {
	RvnMtlSurface* __unsafe_unretained surface;
	const RDrawData* data;
	id<MTLTexture> __unsafe_unretained target;
	id<MTLBuffer> __unsafe_unretained instances;
	int glass;
} rvn_mtl_pass_t;

/* Begins a render pass on the target of the frame: cleared with the clear color, or loaded. */
static id<MTLRenderCommandEncoder> rvn_mtl_begin_pass(id<MTLCommandBuffer> commands, const rvn_mtl_pass_t* pass, int clear) {
	MTLRenderPassDescriptor* descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
	descriptor.colorAttachments[0].texture = pass->target;
	descriptor.colorAttachments[0].loadAction = clear ? MTLLoadActionClear : MTLLoadActionLoad;
	descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
	/* as given: the target is UNORM, nothing is converted */
	descriptor.colorAttachments[0].clearColor = MTLClearColorMake(pass->data->clear_color[0], pass->data->clear_color[1],
		pass->data->clear_color[2], pass->data->clear_color[3]);
	return [commands renderCommandEncoderWithDescriptor:descriptor];
}

/* Binds everything the UI draws need. Also used again after a backdrop capture and a viewport callback. */
static void rvn_mtl_bind_ui_state(id<MTLRenderCommandEncoder> encoder, const rvn_mtl_pass_t* pass, RvnMtlSurface* surface) {
	float width = (float)pass->target.width, height = (float)pass->target.height;
	id<MTLTexture> backdrop = pass->glass ? surface->down[0] : g_mtl.white->texture;
	id<MTLTexture> backdrop_blur = pass->glass ? surface->up[0] : g_mtl.white->texture;
	rvn_mtl_uniforms_t uniforms;
	MTLViewport viewport;
	MTLScissorRect scissor;

	/* pixels, origin top-left -> clip space, whose y axis points up in Metal */
	uniforms.scale[0] = 2.0f / width;
	uniforms.scale[1] = -2.0f / height;
	uniforms.translate[0] = -1.0f;
	uniforms.translate[1] = 1.0f;
	/* where the pointer is, for the effects */
	uniforms.pointer[0] = pass->data->pointer[0];
	uniforms.pointer[1] = pass->data->pointer[1];
	uniforms.inv_size[0] = 1.0f / width;
	uniforms.inv_size[1] = 1.0f / height;

	viewport.originX = 0.0;
	viewport.originY = 0.0;
	viewport.width = (double)width;
	viewport.height = (double)height;
	viewport.znear = 0.0;
	viewport.zfar = 1.0;
	/* the clip of each primitive is applied by the fragment shaders, not by the scissor */
	scissor.x = 0;
	scissor.y = 0;
	scissor.width = pass->target.width;
	scissor.height = pass->target.height;

	[encoder setRenderPipelineState:g_mtl.ui_pipeline];
	/*
	 * State that Metal keeps in the encoder rather than in the pipeline (Vulkan
	 * keeps it in the pipeline): a viewport callback may have changed it, and
	 * culling or a wireframe fill would hide the primitives drawn after it.
	 */
	[encoder setCullMode:MTLCullModeNone];
	[encoder setTriangleFillMode:MTLTriangleFillModeFill];
	[encoder setFrontFacingWinding:MTLWindingClockwise];
	[encoder setDepthBias:0.0f slopeScale:0.0f clamp:0.0f];
	[encoder setDepthClipMode:MTLDepthClipModeClip];
	[encoder setBlendColorRed:0.0f green:0.0f blue:0.0f alpha:0.0f];
	[encoder setStencilReferenceValue:0];
	[encoder setViewport:viewport];
	[encoder setScissorRect:scissor];
	if (pass->instances) [encoder setVertexBuffer:pass->instances offset:0 atIndex:RVN_MTL_BUFFER_INSTANCES];
	[encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:RVN_MTL_BUFFER_UNIFORMS];
	/* what the glass primitives see through */
	[encoder setFragmentTexture:backdrop atIndex:RVN_MTL_TEXTURE_BACKDROP];
	[encoder setFragmentTexture:backdrop_blur atIndex:RVN_MTL_TEXTURE_BACKDROP_BLUR];
}

/*
 * Draws instances [first, first + count) with the texture of the command.
 * The buffer is bound at the first instance and drawn from instance 0 rather
 * than with a base instance: some GPUs (the iOS Simulator, "Apple 2") cannot
 * draw with a non-zero base instance.
 */
static void rvn_mtl_draw_instances(id<MTLRenderCommandEncoder> encoder, const RDrawCommand* command) {
	RvnMtlTexture* texture = rvn_mtl_texture_find(command->texture);
	if (texture) {
		[encoder setFragmentTexture:texture->texture atIndex:RVN_MTL_TEXTURE_UI];
		[encoder setFragmentSamplerState:rvn_mtl_texture_sampler(texture) atIndex:RVN_MTL_SAMPLER_UI];
	} else {
		[encoder setFragmentTexture:g_mtl.white->texture atIndex:RVN_MTL_TEXTURE_UI];
		[encoder setFragmentSamplerState:g_mtl.sampler_linear atIndex:RVN_MTL_SAMPLER_UI];
	}
	[encoder setVertexBufferOffset:(NSUInteger)command->first_instance * RVN_MTL_INSTANCE_SIZE atIndex:RVN_MTL_BUFFER_INSTANCES];
	/* four vertices generated by the vertex function, one instance per primitive */
	[encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4 instanceCount:command->instance_count];
}

/* Calls the application with the viewport and scissor set to the widget (rvn_vk_run_callback). */
static void rvn_mtl_run_callback(id<MTLRenderCommandEncoder> encoder, id<MTLCommandBuffer> commands,
	const rvn_mtl_pass_t* pass, const RDrawCommand* command) {
	RViewportDrawInfo info;
	MTLViewport viewport;
	MTLScissorRect scissor;
	int target_width = (int)pass->target.width, target_height = (int)pass->target.height;
	int x0 = rvn_maxi(command->clip[0], 0);
	int y0 = rvn_maxi(command->clip[1], 0);
	int x1 = rvn_mini(command->clip[0] + command->clip[2], target_width);
	int y1 = rvn_mini(command->clip[1] + command->clip[3], target_height);

	if (!command->callback || x1 <= x0 || y1 <= y0 || command->rect[2] <= 0 || command->rect[3] <= 0) return;

	viewport.originX = (double)command->rect[0];
	viewport.originY = (double)command->rect[1];
	viewport.width = (double)command->rect[2];
	viewport.height = (double)command->rect[3];
	viewport.znear = 0.0;
	viewport.zfar = 1.0;
	scissor.x = (NSUInteger)x0;
	scissor.y = (NSUInteger)y0;
	scissor.width = (NSUInteger)(x1 - x0);
	scissor.height = (NSUInteger)(y1 - y0);
	[encoder setViewport:viewport];
	[encoder setScissorRect:scissor];

	memset(&info, 0, sizeof(info));
	info.backend = R_BACKEND_METAL;
	info.viewport = command->viewport;
	info.window = pass->data->window;
	info.x = command->rect[0];
	info.y = command->rect[1];
	info.width = command->rect[2];
	info.height = command->rect[3];
	info.clip_x = x0;
	info.clip_y = y0;
	info.clip_width = x1 - x0;
	info.clip_height = y1 - y0;
	info.framebuffer_width = target_width;
	info.framebuffer_height = target_height;
	info.frame_index = pass->data->frame_index;
	info.color_format = (Ruint)RVN_MTL_COLOR_FORMAT;
	/* (valid during the call only: not retained) */
	info.metal_encoder = (__bridge void*)encoder;
	info.metal_command_buffer = (__bridge void*)commands;
	info.metal_device = (__bridge void*)g_mtl.device;
	command->callback(&info, command->user_data);
}

/* Encodes the commands of the frame. Returns the encoder that is still open (a backdrop capture ends one and begins another), or nil. */
static id<MTLRenderCommandEncoder> rvn_mtl_encode_commands(id<MTLCommandBuffer> commands, id<MTLRenderCommandEncoder> encoder,
	RvnMtlSurface* surface, const rvn_mtl_pass_t* pass) {
	const RDrawData* data = pass->data;
	uint32_t i;

	rvn_mtl_bind_ui_state(encoder, pass, surface);
	for (i = 0; i < data->command_count; i++) {
		const RDrawCommand* command = &data->commands[i];
		if (command->type == R_DRAW_CALLBACK) {
			rvn_mtl_run_callback(encoder, commands, pass, command);
			/* the application bound its own state: start again from a known one */
			rvn_mtl_bind_ui_state(encoder, pass, surface);
		} else if (command->type == R_DRAW_BACKDROP) {
			if (!pass->glass) continue;
			[encoder endEncoding];
			rvn_mtl_glass_capture(commands, surface, pass->target, data->scale, command->rect);
			encoder = rvn_mtl_begin_pass(commands, pass, R_FALSE);
			if (!encoder) return nil;
			rvn_mtl_bind_ui_state(encoder, pass, surface);
		} else if (command->type == R_DRAW_INSTANCES || command->type == R_DRAW_ERASE || command->type == R_DRAW_SHADER) {
			id<MTLRenderPipelineState> pipeline = nil;
			if (command->instance_count == 0 || command->first_instance + command->instance_count > data->instance_count) continue;
			if (command->type == R_DRAW_ERASE) pipeline = g_mtl.erase_pipeline;
			else if (command->type == R_DRAW_SHADER) pipeline = rvn_mtl_shader_pipeline(command->shader);
			/* (the pipelines share their bindings: what is bound stays bound) */
			if (pipeline) [encoder setRenderPipelineState:pipeline];
			rvn_mtl_draw_instances(encoder, command);
			if (pipeline) [encoder setRenderPipelineState:g_mtl.ui_pipeline];
		}
	}
	return encoder;
}

/* ---------------------------------------------------------------------------
 *  Frame
 * ------------------------------------------------------------------------- */

/* Copies the instances of the frame into the buffer of the slot, which the GPU no longer reads. */
static id<MTLBuffer> rvn_mtl_upload_instances(RvnMtlSurface* surface, int slot, const RDrawData* data) {
	NSUInteger needed = (NSUInteger)data->instance_count * sizeof(RDrawInstance);
	id<MTLBuffer> buffer = surface->instances[slot];
	if (needed == 0) return buffer;
	if (!buffer || buffer.length < needed) {
		NSUInteger size = buffer ? buffer.length : (NSUInteger)RVN_MTL_MIN_INSTANCES * sizeof(RDrawInstance);
		while (size < needed) size *= 2;
		buffer = [g_mtl.device newBufferWithLength:size
			options:MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined];
		surface->instances[slot] = buffer;
		if (!buffer) {
			rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Metal: cannot allocate %lu bytes of instances", (unsigned long)size);
			return nil;
		}
	}
	memcpy(buffer.contents, data->instances, needed);
	return buffer;
}

/* Converts the read-back BGRA pixels to tightly packed RGBA8 (rvn_vk_finish_capture). */
static void rvn_mtl_finish_capture(const RvnMtlSurface* surface, const uint8_t* source, Ruint width, Ruint height, RCapture* capture) {
	size_t count = (size_t)width * height, i;
	uint8_t* pixels = Rpixels_alloc(count * 4);
	if (!pixels) return;
	for (i = 0; i < count; i++) {
		pixels[i * 4 + 0] = source[i * 4 + 2];
		pixels[i * 4 + 1] = source[i * 4 + 1];
		pixels[i * 4 + 2] = source[i * 4 + 0];
		/* a window is opaque whatever the alpha left in the drawable, unless it is one that is seen through */
		pixels[i * 4 + 3] = surface->transparent ? source[i * 4 + 3] : 255;
	}
	capture->pixels = pixels;
	capture->width = (int)width;
	capture->height = (int)height;
}

/* R_TRUE when the frame contains glass and the surface can render it. */
static int rvn_mtl_frame_uses_glass(RvnMtlSurface* surface, const RDrawData* data, id<MTLTexture> target) {
	uint32_t i;
	for (i = 0; i < data->command_count; i++) {
		if (data->commands[i].type == R_DRAW_BACKDROP) return rvn_mtl_glass_prepare(surface, (Ruint)target.width, (Ruint)target.height);
	}
	return R_FALSE;
}

void rvn_mtl_render_frame(const RDrawData* data, RCapture* capture) {
	RvnMtlSurface* surface;
	dispatch_semaphore_t slots;
	id<CAMetalDrawable> drawable = nil;
	id<MTLTexture> target;
	id<MTLCommandBuffer> commands;
	id<MTLRenderCommandEncoder> encoder;
	id<MTLBuffer> capture_buffer = nil;
	rvn_mtl_pass_t pass;
	Ruint capture_width = 0, capture_height = 0;
	int slot;

	if (capture) memset(capture, 0, sizeof(*capture));
	/* the device is lost (nothing is drawn until Raven started the backend again), or iOS suspended rendering */
	if (!rvn_mtl_usable() || rvn_mtl_suspended()) return;
	surface = rvn_mtl_surface_find(data->window);
	if (!surface || data->framebuffer_width == 0 || data->framebuffer_height == 0) return;

	/* ---- the size and the synchronization of the target ---- */
	if (surface->layer) {
		if (surface->drawable_width != data->framebuffer_width || surface->drawable_height != data->framebuffer_height ||
			surface->vsync != data->vsync) {
			rvn_mtl_layer_configure(surface, data->framebuffer_width, data->framebuffer_height, data->vsync, R_FALSE);
		}
	} else if (surface->drawable_width != data->framebuffer_width || surface->drawable_height != data->framebuffer_height) {
		/* (frames in flight keep the old target alive) */
		id<MTLTexture> resized = rvn_mtl_target(data->framebuffer_width, data->framebuffer_height);
		if (!resized) return;
		surface->headless = resized;
		surface->drawable_width = data->framebuffer_width;
		surface->drawable_height = data->framebuffer_height;
	}

	/*
	 * ---- the slot: the GPU has finished the frame that used it last ----
	 * The cursor moves on only once the frame is committed (below): a frame
	 * given up on the way (no drawable, no command buffer, suspended) gives its
	 * semaphore count back, and must give its slot back too, or the next frame
	 * would write the instances of a frame the GPU may still be reading.
	 */
	slot = (int)(surface->cursor % RVN_MTL_FRAMES);
	slots = surface->slots;
	dispatch_semaphore_wait(slots, DISPATCH_TIME_FOREVER);
	pass.instances = rvn_mtl_upload_instances(surface, slot, data);
	if (data->instance_count > 0 && !pass.instances) {
		dispatch_semaphore_signal(slots);
		return;
	}

	/* ---- the drawable (blocks at most a second) ---- */
	if (surface->layer) {
		drawable = [surface->layer nextDrawable];
		if (!drawable) {
			/* the next frame tries again */
			dispatch_semaphore_signal(slots);
			return;
		}
		target = drawable.texture;
	} else {
		target = surface->headless;
	}

	commands = [g_mtl.queue commandBuffer];
	if (!commands) {
		dispatch_semaphore_signal(slots);
		return;
	}
	commands.label = @"Raven frame";
	[commands addCompletedHandler:^(id<MTLCommandBuffer> done) {
		rvn_mtl_check_completed(done);
		dispatch_semaphore_signal(slots);
	}];

	pass.surface = surface;
	pass.data = data;
	pass.target = target;
	pass.glass = rvn_mtl_frame_uses_glass(surface, data, target);

	/* ---- encode ---- */
	encoder = rvn_mtl_begin_pass(commands, &pass, R_TRUE);
	if (encoder) {
		encoder = rvn_mtl_encode_commands(commands, encoder, surface, &pass);
		if (encoder) [encoder endEncoding];
	}

	if (capture) {
		/* the drawable can be larger than the frame when the window system imposes its size */
		capture_width = rvn_minu((Ruint)target.width, data->framebuffer_width);
		capture_height = rvn_minu((Ruint)target.height, data->framebuffer_height);
		capture_buffer = [g_mtl.device newBufferWithLength:(NSUInteger)capture_width * capture_height * 4
			options:MTLResourceStorageModeShared];
		if (capture_buffer) {
			id<MTLBlitCommandEncoder> blit = [commands blitCommandEncoder];
			[blit copyFromTexture:target sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0)
				sourceSize:MTLSizeMake(capture_width, capture_height, 1)
				toBuffer:capture_buffer destinationOffset:0 destinationBytesPerRow:(NSUInteger)capture_width * 4
				destinationBytesPerImage:(NSUInteger)capture_width * capture_height * 4];
			[blit endEncoding];
		}
	}

	/* ---- present, commit ---- */
	if (!rvn_mtl_commit(commands, drawable)) {
		/* suspended in the meantime: never committed, so the completed handler will not give the slot back */
		dispatch_semaphore_signal(slots);
		return;
	}
	surface->cursor++;

	if (capture_buffer) {
		[commands waitUntilCompleted];
		if (commands.status == MTLCommandBufferStatusCompleted) {
			rvn_mtl_finish_capture(surface, (const uint8_t*)capture_buffer.contents, capture_width, capture_height, capture);
		}
	}
}
