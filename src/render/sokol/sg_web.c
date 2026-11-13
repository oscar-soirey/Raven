/**
 * Raven - render/sokol/sg_web.c
 * WebGL 2 in a browser: the native-API part of the Sokol backend on the web (Emscripten).
 *
 * The context belongs to the canvas of the page and lives on the main thread, so the backend runs on the UI
 * thread (the web platform asks for it: rvn_platform_t::single_thread_render). There is one canvas and one
 * window: its frame is drawn into the default framebuffer, and the browser shows the canvas when the
 * callback of the animation frame returns, so nothing is swapped here.
 */

#include "sg_backend.h"

#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>

static struct {
	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
	const char* canvas;
} g_web;

int rvn_sg_context_init(const RBackendInitInfo* info, sg_environment* environment) {
	EmscriptenWebGLContextAttributes attributes;
	RVN_UNUSED(environment);
	g_web.canvas = info->window_system == R_WINDOW_SYSTEM_WEB && info->native_display ? (const char*)info->native_display : "#canvas";
	emscripten_webgl_init_context_attributes(&attributes);
	attributes.majorVersion = 2;
	attributes.minorVersion = 0;
	/* the page shows the canvas as it is: opaque, no depth or stencil, no antialiasing (the UI draws its own edges) */
	attributes.alpha = 0;
	attributes.depth = 0;
	attributes.stencil = 0;
	attributes.antialias = 0;
	attributes.premultipliedAlpha = 1;
	attributes.preserveDrawingBuffer = 0;
	attributes.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
	attributes.explicitSwapControl = 0;
	g_web.context = emscripten_webgl_create_context(g_web.canvas, &attributes);
	if (g_web.context <= 0) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Sokol: the browser cannot create a WebGL 2 context for the canvas");
		g_web.context = 0;
		return R_FALSE;
	}
	if (emscripten_webgl_make_context_current(g_web.context) != EMSCRIPTEN_RESULT_SUCCESS) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Sokol: cannot make the WebGL 2 context current");
		emscripten_webgl_destroy_context(g_web.context);
		g_web.context = 0;
		return R_FALSE;
	}
	return R_TRUE;
}

void rvn_sg_context_shutdown(void) {
	if (g_web.context) {
		emscripten_webgl_make_context_current(0);
		emscripten_webgl_destroy_context(g_web.context);
	}
	memset(&g_web, 0, sizeof(g_web));
}

/* ---------------------------------------------------------------------------
 *  The window: the canvas
 * ------------------------------------------------------------------------- */

int rvn_sg_present_create(rvn_sg_surface_t* surface, const RNativeWindow* native) {
	if (!g_web.context || native->system != R_WINDOW_SYSTEM_WEB) return R_FALSE;
	/* nothing to make: the canvas has its context. Any non-NULL value says "presented". */
	surface->present = (void*)&g_web;
	return R_TRUE;
}

void rvn_sg_present_destroy(rvn_sg_surface_t* surface) {
	surface->present = NULL;
}

int rvn_sg_present_begin(rvn_sg_surface_t* surface, int vsync, sg_swapchain* swapchain) {
	RVN_UNUSED(vsync);
	/* (the pace is the one of the animation frames of the browser) */
	swapchain->width = surface->width;
	swapchain->height = surface->height;
	swapchain->gl.framebuffer = 0;
	return R_TRUE;
}

void rvn_sg_present_end(rvn_sg_surface_t* surface) {
	RVN_UNUSED(surface);
}
