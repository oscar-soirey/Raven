/**
 * Raven - render/null/null_backend.c
 * Backend that draws nothing. It is used for headless runs and unit tests,
 * and is the smallest example of a RBackendVTable: copy it to start a
 * new backend.
 */

#include "../renderer.h"

static int rvn_null_init(const RBackendInitInfo* info) {
	RVN_UNUSED(info);
	return R_TRUE;
}

static void rvn_null_shutdown(void) {
}

static int rvn_null_create_surface(Rid window, const RNativeWindow* native, Ruint width, Ruint height) {
	RVN_UNUSED(window);
	RVN_UNUSED(native);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
	return R_TRUE;
}

static void rvn_null_destroy_surface(Rid window) {
	RVN_UNUSED(window);
}

static int rvn_null_create_texture(Rid texture, const RTextureDesc* desc, const void* pixels) {
	RVN_UNUSED(texture);
	RVN_UNUSED(desc);
	RVN_UNUSED(pixels);
	return R_TRUE;
}

static void rvn_null_update_texture(Rid texture, Ruint x, Ruint y, Ruint width, Ruint height, const void* pixels) {
	RVN_UNUSED(texture);
	RVN_UNUSED(x);
	RVN_UNUSED(y);
	RVN_UNUSED(width);
	RVN_UNUSED(height);
	RVN_UNUSED(pixels);
}

static void rvn_null_set_texture_filter(Rid texture, RFilterType filter) {
	RVN_UNUSED(texture);
	RVN_UNUSED(filter);
}

static void rvn_null_destroy_texture(Rid texture) {
	RVN_UNUSED(texture);
}

static void rvn_null_render_frame(const RDrawData* data, RCapture* capture) {
	if (capture) {
		/* a capture of the null backend is the clear color */
		size_t count = (size_t)data->framebuffer_width * data->framebuffer_height, i;
		rvn_color_t clear = rvn_rgba(data->clear_color[0], data->clear_color[1], data->clear_color[2], data->clear_color[3]);
		capture->width = (int)data->framebuffer_width;
		capture->height = (int)data->framebuffer_height;
		capture->pixels = count ? Rpixels_alloc(count * 4) : NULL;
		for (i = 0; i < count; i++) memcpy(capture->pixels + i * 4, &clear, 4);
	}
}

static void rvn_null_wait_idle(void) {
}

const RBackendVTable* rvn_backend_null(void) {
	static const RBackendVTable vtable = {
		"null",
		rvn_null_init,
		rvn_null_shutdown,
		rvn_null_create_surface,
		rvn_null_destroy_surface,
		rvn_null_create_texture,
		rvn_null_update_texture,
		rvn_null_set_texture_filter,
		rvn_null_destroy_texture,
		rvn_null_render_frame,
		rvn_null_wait_idle,
		/* (no effects: their rectangles are not drawn either) */
		NULL,
		NULL,
		0
	};
	return &vtable;
}
