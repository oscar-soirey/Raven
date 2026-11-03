/**
 * Raven - render/device.c
 * Recovery from the loss of the GPU device.
 *
 * A backend reports the loss with Rbackend_device_lost (the Vulkan backend
 * does it when a call answers VK_ERROR_DEVICE_LOST). At the next update the
 * UI thread starts the backend again (Shutdown, then Init) and makes again
 * everything the backend had:
 *
 *   - the surface of every window,
 *   - every texture, with the same handle: from the copy of its pixels
 *     (small textures), from the file or the encoded image it was made from,
 *     or transparent with the same size (see rvn_texture_t),
 *   - the pages of the glyph atlas, from the copy the fonts keep,
 *   - every shader, from its SPIR-V.
 *
 * Then the application is told (Rset_device_reset_callback), for the pixels
 * that only it has. When the backend cannot start again (no GPU any more),
 * it is tried again every second.
 */

#include "renderer.h"
#include "font.h"
#include "../core/context.h"

#define RVN_DEVICE_RETRY_SECONDS 1.0

void rvn_device_check(void) {
	RBackendInitInfo info;
	uint32_t i;

	if (!rvn_renderer_device_lost()) return;
	if (g_rvn.device_retry_time > 0.0 && rvn_clock_seconds() < g_rvn.device_retry_time) return;

	memset(&info, 0, sizeof(info));
	info.app_name = g_rvn.app_name ? g_rvn.app_name : "Raven application";
	info.enable_validation = g_rvn.enable_validation;
	info.vsync = g_rvn.vsync;
	info.window_system = g_rvn.platform->system;
	info.native_display = g_rvn.platform->native_display();

	/* (nothing of the old device may be used once it is shut down) */
	rvn_renderer_sync();
	if (!rvn_renderer_restart(&info)) {
		if (!g_rvn.device_retry_reported) {
			rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "the GPU device was lost and cannot be started again yet: trying again every second");
			g_rvn.device_retry_reported = R_TRUE;
		}
		g_rvn.device_retry_time = rvn_clock_seconds() + RVN_DEVICE_RETRY_SECONDS;
		/* the backend is down: the next attempt goes through the same path */
		Rbackend_device_lost();
		return;
	}
	g_rvn.device_retry_time = 0.0;
	g_rvn.device_retry_reported = R_FALSE;

	rvn_windows_device_reset();
	for (i = 0; i < g_rvn.textures.used; i++) {
		rvn_texture_t* texture = (rvn_texture_t*)rvn_pool_at(&g_rvn.textures, i);
		if (texture) rvn_texture_restore(texture);
	}
	rvn_fonts_device_reset();
	for (i = 0; i < g_rvn.shaders.used; i++) {
		rvn_shader_t* shader = (rvn_shader_t*)rvn_pool_at(&g_rvn.shaders, i);
		if (shader) rvn_shader_restore(shader);
	}
	rvn_error(R_ERROR_BACKEND, R_SEVERITY_WARNING, "the GPU device was lost: the %s backend was started again",
		g_rvn.backend == R_BACKEND_VULKAN ? "Vulkan" : "rendering");

	if (g_rvn.device_reset_callback) {
		rvn_dispatch_begin();
		g_rvn.device_reset_callback(g_rvn.device_reset_user);
		rvn_dispatch_end();
	}
	rvn_invalidate_all_windows();
}
