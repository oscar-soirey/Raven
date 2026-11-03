/**
 * Raven - render/vulkan/vk_backend.c
 * The Vulkan implementation of RBackendVTable.
 */

#include "vk_backend.h"

static void rvn_vk_shutdown(void) {
	if (g_vk.device && rvn_vk.DeviceWaitIdle) {
		rvn_vk_queue_lock();
		rvn_vk.DeviceWaitIdle(g_vk.device);
		rvn_vk_queue_unlock();
		rvn_vk_surfaces_shutdown();
		rvn_vk_textures_shutdown();
		rvn_vk_pipeline_shutdown();
	}
	rvn_vk_device_shutdown();
	memset(&g_vk, 0, sizeof(g_vk));
}

static int rvn_vk_init(const RBackendInitInfo* info) {
	memset(&g_vk, 0, sizeof(g_vk));
	g_vk.info = *info;
	if (!rvn_vk_device_init() || !rvn_vk_pipeline_init() || !rvn_vk_textures_init()) {
		rvn_vk_shutdown();
		return R_FALSE;
	}
	g_vk.initialized = R_TRUE;
	return R_TRUE;
}

static void rvn_vk_wait_idle(void) {
	if (!rvn_vk_usable()) return;
	rvn_vk_uploads_flush();
	rvn_vk_wait_frames();
}

const RBackendVTable* rvn_backend_vulkan(void) {
	static const RBackendVTable vtable = {
		"Vulkan",
		rvn_vk_init,
		rvn_vk_shutdown,
		rvn_vk_surface_create,
		rvn_vk_surface_destroy,
		rvn_vk_texture_create,
		rvn_vk_texture_update,
		rvn_vk_texture_set_filter,
		rvn_vk_texture_destroy,
		rvn_vk_render_frame,
		rvn_vk_wait_idle,
		rvn_vk_shader_create,
		rvn_vk_shader_destroy,
		R_BACKEND_CAP_GLASS | R_BACKEND_CAP_ERASE | R_BACKEND_CAP_SHADERS | R_BACKEND_CAP_READBACK |
			R_BACKEND_CAP_CALLBACK | R_BACKEND_CAP_PRESENT
	};
	return &vtable;
}
