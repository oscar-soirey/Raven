/**
 * Raven - render/metal/metal_backend.m
 * The Metal implementation of RBackendVTable: Init / Shutdown, WaitIdle, the
 * loss of the device and the suspension of rendering on iOS.
 *
 * Metal is what macOS and iOS draw with: this backend needs nothing that the
 * system does not have (no MoltenVK, no Vulkan loader, no shader compiler at
 * build time). See docs/METAL_BACKEND.md.
 */

#include "metal_backend.h"

#include <pthread.h>

rvn_mtl_t g_mtl;

/*
 * iOS terminates an application that sends work to the GPU while it is in the
 * background. rvn_metal_set_suspended takes this lock to set the flag; the
 * render thread holds it across "is rendering suspended? / commit", so no
 * command buffer is committed once the flag is set.
 */
static pthread_mutex_t g_mtl_suspend_lock = PTHREAD_MUTEX_INITIALIZER;
static atomic_int g_mtl_suspended;

int rvn_mtl_usable(void) {
	return g_mtl.initialized && !atomic_load(&g_mtl.device_lost);
}

int rvn_mtl_suspended(void) {
	return atomic_load(&g_mtl_suspended) != 0;
}

/* ---------------------------------------------------------------------------
 *  Submission
 * ------------------------------------------------------------------------- */

void rvn_mtl_check_completed(id<MTLCommandBuffer> commands) {
	NSError* error;
	if (commands.status != MTLCommandBufferStatusError) return;
	error = commands.error;
	if (error && [error.domain isEqualToString:MTLCommandBufferErrorDomain]) {
#if TARGET_OS_OSX
		/*
		 * MTLCommandBufferErrorDeviceRemoved (11), by its value: the constant does
		 * not exist on iOS (no removable GPU) and is deprecated from macOS 27.
		 */
		if (error.code == 11) {
			/* an external GPU was unplugged: Raven starts the backend again (render/device.c) */
			if (!atomic_exchange(&g_mtl.device_lost, 1)) Rbackend_device_lost();
			return;
		}
#endif
		/* GPU work refused in the background (iOS): expected, the frame is simply not shown */
		if (error.code == MTLCommandBufferErrorNotPermitted) return;
	}
	if (!atomic_exchange(&g_mtl.failure_reported, 1)) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Metal: a frame failed: %s",
			error ? error.localizedDescription.UTF8String : "unknown error");
	}
}

/* Commits the upload command buffer, if any. The suspension lock is held. */
static void rvn_mtl_commit_uploads_locked(void) {
	id<MTLCommandBuffer> uploads = rvn_mtl_uploads_take();
	if (!uploads) return;
	[uploads addCompletedHandler:^(id<MTLCommandBuffer> done) {
		rvn_mtl_check_completed(done);
	}];
	[uploads commit];
	g_mtl.last_committed = uploads;
}

int rvn_mtl_commit(id<MTLCommandBuffer> commands, id<MTLDrawable> drawable) {
	pthread_mutex_lock(&g_mtl_suspend_lock);
	if (atomic_load(&g_mtl_suspended)) {
		/* (the uploads stay encoded: they are committed once rendering resumes) */
		pthread_mutex_unlock(&g_mtl_suspend_lock);
		return R_FALSE;
	}
	/* the textures of the frame are complete before it: the queue runs command buffers in order */
	rvn_mtl_commit_uploads_locked();
	if (commands) {
		if (drawable) [commands presentDrawable:drawable];
		[commands commit];
		g_mtl.last_committed = commands;
	}
	pthread_mutex_unlock(&g_mtl_suspend_lock);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  iOS suspension
 * ------------------------------------------------------------------------- */

void rvn_metal_set_suspended(int suspended) {
	id<MTLCommandBuffer> last = nil;
	@autoreleasepool {
		pthread_mutex_lock(&g_mtl_suspend_lock);
		atomic_store(&g_mtl_suspended, suspended ? 1 : 0);
		if (suspended) last = g_mtl.last_committed;
		pthread_mutex_unlock(&g_mtl_suspend_lock);
		/* the GPU is idle when the notification returns: nothing committed can run in the background */
		if (last) [last waitUntilCompleted];
	}
}

/* ---------------------------------------------------------------------------
 *  Table
 * ------------------------------------------------------------------------- */

static void rvn_mtl_wait_idle(void) {
	id<MTLCommandBuffer> marker, last = nil;
	if (!g_mtl.initialized) return;
	@autoreleasepool {
		/* the queue runs in order: an empty command buffer completes after everything before it */
		marker = [g_mtl.queue commandBuffer];
		if (marker && rvn_mtl_commit(marker, nil)) {
			[marker waitUntilCompleted];
			return;
		}
		/* suspended: nothing new is committed, the last command buffer is what there is to wait for */
		pthread_mutex_lock(&g_mtl_suspend_lock);
		last = g_mtl.last_committed;
		pthread_mutex_unlock(&g_mtl_suspend_lock);
		if (last) [last waitUntilCompleted];
	}
}

static void rvn_mtl_shutdown(void) {
	int i;
	@autoreleasepool {
		if (g_mtl.queue) rvn_mtl_wait_idle();
		rvn_mtl_surfaces_shutdown();
		rvn_mtl_textures_shutdown();
		rvn_mtl_pipeline_shutdown();
		pthread_mutex_lock(&g_mtl_suspend_lock);
		g_mtl.last_committed = nil;
		pthread_mutex_unlock(&g_mtl_suspend_lock);
		g_mtl.upload_blit = nil;
		g_mtl.upload_commands = nil;
		for (i = 0; i < RVN_MTL_EFFECTS; i++) {
			g_mtl.effect_libraries[i] = nil;
			g_mtl.effect_pipelines[i] = nil;
			g_mtl.effect_failed[i] = R_FALSE;
		}
		g_mtl.queue = nil;
		g_mtl.device = nil;
		g_mtl.initialized = R_FALSE;
		g_mtl.custom_shader_reported = R_FALSE;
		atomic_store(&g_mtl.device_lost, 0);
		atomic_store(&g_mtl.failure_reported, 0);
		memset(&g_mtl.info, 0, sizeof(g_mtl.info));
	}
}

static int rvn_mtl_init(const RBackendInitInfo* info) {
	@autoreleasepool {
		g_mtl.info = *info;
		atomic_store(&g_mtl.device_lost, 0);
		atomic_store(&g_mtl.failure_reported, 0);
		/* (RBackendInitInfo::enable_validation: Metal's validation is switched on outside the process, MTL_DEBUG_LAYER=1) */
		g_mtl.device = MTLCreateSystemDefaultDevice();
		if (!g_mtl.device) {
			rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Metal: this system has no Metal device");
			rvn_mtl_shutdown();
			return R_FALSE;
		}
		g_mtl.queue = [g_mtl.device newCommandQueue];
		if (!g_mtl.queue) {
			rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Metal: cannot create a command queue");
			rvn_mtl_shutdown();
			return R_FALSE;
		}
		g_mtl.queue.label = @"Raven";
		g_mtl.surfaces = [NSMutableDictionary dictionary];
		if (!rvn_mtl_pipeline_init() || !rvn_mtl_textures_init()) {
			rvn_mtl_shutdown();
			return R_FALSE;
		}
		g_mtl.initialized = R_TRUE;
	}
	return R_TRUE;
}

static int rvn_mtl_create_surface(Rid window, const RNativeWindow* native, Ruint width, Ruint height) {
	@autoreleasepool {
		return rvn_mtl_surface_create(window, native, width, height);
	}
}

static void rvn_mtl_destroy_surface(Rid window) {
	@autoreleasepool {
		rvn_mtl_surface_destroy(window);
	}
}

static int rvn_mtl_create_texture(Rid texture, const RTextureDesc* desc, const void* pixels) {
	@autoreleasepool {
		return rvn_mtl_texture_create(texture, desc, pixels);
	}
}

static void rvn_mtl_update_texture(Rid texture, Ruint x, Ruint y, Ruint width, Ruint height, const void* pixels) {
	@autoreleasepool {
		rvn_mtl_texture_update(texture, x, y, width, height, pixels);
	}
}

static void rvn_mtl_set_texture_filter(Rid texture, RFilterType filter) {
	@autoreleasepool {
		rvn_mtl_texture_set_filter(texture, filter);
	}
}

static void rvn_mtl_destroy_texture(Rid texture) {
	@autoreleasepool {
		rvn_mtl_texture_destroy(texture);
	}
}

static void rvn_mtl_render(const RDrawData* data, RCapture* capture) {
	/* (drawables are autoreleased: without a pool of its own the render thread piles them up) */
	@autoreleasepool {
		rvn_mtl_render_frame(data, capture);
	}
}

static int rvn_mtl_create_shader(Rid shader, const uint32_t* spirv, size_t size) {
	@autoreleasepool {
		return rvn_mtl_shader_create(shader, spirv, size);
	}
}

static void rvn_mtl_destroy_shader(Rid shader) {
	@autoreleasepool {
		rvn_mtl_shader_destroy(shader);
	}
}

const RBackendVTable* rvn_backend_metal(void) {
	static const RBackendVTable vtable = {
		"Metal",
		rvn_mtl_init,
		rvn_mtl_shutdown,
		rvn_mtl_create_surface,
		rvn_mtl_destroy_surface,
		rvn_mtl_create_texture,
		rvn_mtl_update_texture,
		rvn_mtl_set_texture_filter,
		rvn_mtl_destroy_texture,
		rvn_mtl_render,
		rvn_mtl_wait_idle,
		rvn_mtl_create_shader,
		rvn_mtl_destroy_shader
	};
	return &vtable;
}
