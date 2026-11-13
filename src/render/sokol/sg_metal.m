/**
 * Raven - render/sokol/sg_metal.m
 * The Metal part of the sokol_gfx backend (the counterpart of sg_egl.c): the device, the read-back, and
 * the autorelease pool of the render thread. Compiled with ARC (CMakeLists.txt).
 *
 * Like the GLES3 build, every window renders its frame into an offscreen image first (which Rwindow_capture
 * reads back); a window of the window system is presented by drawing that image into the next drawable of
 * its CAMetalLayer (the layer the Cocoa platform installs, RNativeWindow::window). sokol_gfx presents the
 * drawable itself in sg_commit().
 *
 * Status: written without a Mac at hand. The offscreen part passed test_render on a Mac at its first run;
 * presentation is verified by looking at the examples.
 */

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <TargetConditionals.h>

#include "sg_backend.h"

/* libobjc: a pool that can be pushed and popped from C, with or without ARC */
extern void* objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void* pool);

static id<MTLDevice> g_device;

int rvn_sg_context_init(const RBackendInitInfo* info, sg_environment* environment) {
	RVN_UNUSED(info);
	g_device = MTLCreateSystemDefaultDevice();
	if (!g_device) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Sokol: no Metal device on this machine");
		return R_FALSE;
	}
	environment->metal.device = (__bridge const void*)g_device;
	/* the pixel format of a CAMetalLayer (the Cocoa platform sets it); the frames are RGBA8 images drawn into it */
	environment->defaults.color_format = SG_PIXELFORMAT_BGRA8;
	return R_TRUE;
}

void rvn_sg_context_shutdown(void) {
	g_device = nil;
}

void rvn_sg_context_finish(void) {
	/* the queue is serial: when an empty buffer committed now is done, so is everything before it */
	id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)sg_mtl_command_queue();
	id<MTLCommandBuffer> commands = [queue commandBuffer];
	[commands commit];
	[commands waitUntilCompleted];
}

uint8_t* rvn_sg_read_target(const rvn_sg_target_t* target) {
	sg_mtl_image_info info = sg_mtl_query_image_info(target->image);
	id<MTLTexture> texture = (__bridge id<MTLTexture>)info.tex[info.active_slot];
	id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)sg_mtl_command_queue();
	NSUInteger width = (NSUInteger)target->width, height = (NSUInteger)target->height;
	NSUInteger row = width * 4, size = row * height;
	uint8_t* pixels = Rpixels_alloc(size);

	/* the images of sokol_gfx live in private memory: copy to a buffer the CPU can read */
	id<MTLBuffer> buffer = [g_device newBufferWithLength:size options:MTLResourceStorageModeShared];
	id<MTLCommandBuffer> commands = [queue commandBuffer];
	id<MTLBlitCommandEncoder> blit = [commands blitCommandEncoder];
	[blit copyFromTexture:texture
		sourceSlice:0
		sourceLevel:0
		sourceOrigin:MTLOriginMake(0, 0, 0)
		sourceSize:MTLSizeMake(width, height, 1)
		toBuffer:buffer
		destinationOffset:0
		destinationBytesPerRow:row
		destinationBytesPerImage:size];
	[blit endEncoding];
	[commands commit];
	/* the frame was committed before this buffer on the same queue: waiting for this one waits for both */
	[commands waitUntilCompleted];
	/* the image is stored with the top row first: no flip */
	memcpy(pixels, [buffer contents], size);
	return pixels;
}

void* rvn_sg_pool_push(void) {
	return objc_autoreleasePoolPush();
}

void rvn_sg_pool_pop(void* pool) {
	objc_autoreleasePoolPop(pool);
}

/* ---------------------------------------------------------------------------
 *  Windows
 * ------------------------------------------------------------------------- */

typedef struct rvn_sg_mtl_window_t {
	/** The layer of the window; the window system owns it and outlives the surface. */
	const void* layer;
	/** The drawable of the frame being presented (retained), or NULL. */
	void* drawable;
} rvn_sg_mtl_window_t;

int rvn_sg_present_create(rvn_sg_surface_t* surface, const RNativeWindow* native) {
	rvn_sg_mtl_window_t* window;
	CAMetalLayer* layer;
	if ((native->system != R_WINDOW_SYSTEM_COCOA && native->system != R_WINDOW_SYSTEM_UIKIT) || !native->window) return R_FALSE;
	layer = (__bridge CAMetalLayer*)(void*)native->window;
	if (![layer isKindOfClass:[CAMetalLayer class]]) return R_FALSE;
	/* the layer draws with the device of sokol_gfx, in the format of the swapchain pass */
	layer.device = g_device;
	layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
	layer.framebufferOnly = YES;
	window = (rvn_sg_mtl_window_t*)rvn_calloc(sizeof(*window));
	window->layer = (const void*)native->window;
	surface->present = window;
	return R_TRUE;
}

void rvn_sg_present_destroy(rvn_sg_surface_t* surface) {
	rvn_sg_mtl_window_t* window = (rvn_sg_mtl_window_t*)surface->present;
	if (!window) return;
	if (window->drawable) CFBridgingRelease(window->drawable);
	rvn_free(window);
	surface->present = NULL;
}

int rvn_sg_present_begin(rvn_sg_surface_t* surface, int vsync, sg_swapchain* swapchain) {
	rvn_sg_mtl_window_t* window = (rvn_sg_mtl_window_t*)surface->present;
	CAMetalLayer* layer = (__bridge CAMetalLayer*)window->layer;
	CGSize size = CGSizeMake((CGFloat)surface->width, (CGFloat)surface->height);
	id<CAMetalDrawable> drawable;
	/* the layer was installed by the window system, which does not follow the size of the frame */
	if (layer.drawableSize.width != size.width || layer.drawableSize.height != size.height) layer.drawableSize = size;
#if TARGET_OS_OSX
	if (layer.displaySyncEnabled != (vsync ? YES : NO)) layer.displaySyncEnabled = vsync ? YES : NO;
#else
	(void)vsync;
#endif
	drawable = [layer nextDrawable];
	if (!drawable) return R_FALSE;
	window->drawable = (void*)CFBridgingRetain(drawable);
	swapchain->width = surface->width;
	swapchain->height = surface->height;
	swapchain->metal.current_drawable = (__bridge const void*)drawable;
	return R_TRUE;
}

void rvn_sg_present_end(rvn_sg_surface_t* surface) {
	rvn_sg_mtl_window_t* window = (rvn_sg_mtl_window_t*)surface->present;
	/* sg_commit() has queued the presentation, which keeps the drawable alive */
	if (window->drawable) {
		CFBridgingRelease(window->drawable);
		window->drawable = NULL;
	}
}
