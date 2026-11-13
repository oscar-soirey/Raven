/**
 * Raven - render/sokol/sg_egl.c
 * GLES 3.0 through EGL: the native-API part of the Sokol backend on Linux.
 *
 * One context for the whole backend. It has no surface of its own ("surfaceless"): every window renders its
 * frame into an offscreen image first, and a window of the window system is presented by drawing that
 * image into the default framebuffer of an EGL window surface made current for the occasion. Without a
 * window system (headless runs, tests) the display is the surfaceless platform of Mesa, and nothing is
 * presented.
 *
 * The EGL display is made from the X11 display of the platform (RBackendInitInfo::native_display). The
 * window surface of a window uses the EGL configuration whose native visual is the visual of the window
 * (RNativeWindow::visual); the context is made without a configuration (EGL_KHR_no_config_context) so
 * that it works with all of them.
 *
 * The EGL library is linked; loading it at run time (as the Vulkan loader is) is a later step.
 */

#include "sg_backend.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

static struct {
	EGLDisplay display;
	EGLContext context;
	/** The configuration of the context, or of the pbuffer that is current when there is no surfaceless context. */
	EGLConfig config;
	/** The display belongs to an X11 connection: windows can be presented. */
	int windows;
	EGLSurface pbuffer;
	/** The surface that is current, or EGL_NO_SURFACE. */
	EGLSurface current;
	PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC create_platform_window;
} g_egl;

typedef struct rvn_sg_egl_window_t {
	EGLSurface surface;
	int vsync;
} rvn_sg_egl_window_t;

static int rvn_sg_egl_has(const char* list, const char* name) {
	size_t length = strlen(name);
	const char* at = list;
	while (at && (at = strstr(at, name)) != NULL) {
		if ((at == list || at[-1] == ' ') && (at[length] == ' ' || at[length] == 0)) return R_TRUE;
		at += length;
	}
	return R_FALSE;
}

static int rvn_sg_egl_choose_config(EGLint surface_type, EGLConfig* config) {
	EGLint attributes[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_SURFACE_TYPE, surface_type,
		EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_NONE
	};
	EGLint count = 0;
	return eglChooseConfig(g_egl.display, attributes, config, 1, &count) && count > 0;
}

int rvn_sg_context_init(const RBackendInitInfo* info, sg_environment* environment) {
	static const EGLint context_attributes[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE };
	PFNEGLGETPLATFORMDISPLAYEXTPROC get_display = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
	EGLint major, minor;
	EGLContext context;
	int no_config;
	RVN_UNUSED(environment);

	g_egl.display = EGL_NO_DISPLAY;
	if (get_display && info->window_system == R_WINDOW_SYSTEM_X11 && info->native_display) {
		g_egl.display = get_display(EGL_PLATFORM_X11_EXT, info->native_display, NULL);
		g_egl.windows = g_egl.display != EGL_NO_DISPLAY;
	}
	if (g_egl.display == EGL_NO_DISPLAY && get_display) g_egl.display = get_display(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL);
	if (g_egl.display == EGL_NO_DISPLAY) g_egl.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (g_egl.display == EGL_NO_DISPLAY || !eglInitialize(g_egl.display, &major, &minor)) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Sokol: no EGL display (install a GLES3 driver, for example Mesa)");
		return R_FALSE;
	}
	eglBindAPI(EGL_OPENGL_ES_API);
	g_egl.create_platform_window = (PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC)eglGetProcAddress("eglCreatePlatformWindowSurfaceEXT");

	/* a configuration that can have window surfaces and pbuffers, else window surfaces, else any */
	if (!rvn_sg_egl_choose_config(EGL_WINDOW_BIT | EGL_PBUFFER_BIT, &g_egl.config) &&
		!rvn_sg_egl_choose_config(EGL_WINDOW_BIT, &g_egl.config) &&
		!rvn_sg_egl_choose_config(0, &g_egl.config)) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Sokol: the EGL display has no GLES 3.0 configuration");
		return R_FALSE;
	}
	no_config = rvn_sg_egl_has(eglQueryString(g_egl.display, EGL_EXTENSIONS), "EGL_KHR_no_config_context");
	if (no_config) context = eglCreateContext(g_egl.display, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, context_attributes);
	else context = eglCreateContext(g_egl.display, g_egl.config, EGL_NO_CONTEXT, context_attributes);
	if (context == EGL_NO_CONTEXT) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Sokol: cannot create a GLES 3.0 context");
		return R_FALSE;
	}
	g_egl.context = context;
	if (!eglMakeCurrent(g_egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, g_egl.context)) {
		/* no surfaceless contexts (EGL_KHR_surfaceless_context): a pbuffer of 1 x 1 pixel stands in */
		static const EGLint pbuffer_attributes[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
		g_egl.pbuffer = eglCreatePbufferSurface(g_egl.display, g_egl.config, pbuffer_attributes);
		if (g_egl.pbuffer == EGL_NO_SURFACE || !eglMakeCurrent(g_egl.display, g_egl.pbuffer, g_egl.pbuffer, g_egl.context)) {
			rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Sokol: cannot make a GLES 3.0 context current");
			return R_FALSE;
		}
		g_egl.current = g_egl.pbuffer;
	}
	return R_TRUE;
}

void rvn_sg_context_shutdown(void) {
	if (g_egl.display != EGL_NO_DISPLAY && g_egl.display) {
		eglMakeCurrent(g_egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (g_egl.pbuffer) eglDestroySurface(g_egl.display, g_egl.pbuffer);
		if (g_egl.context) eglDestroyContext(g_egl.display, g_egl.context);
		eglTerminate(g_egl.display);
	}
	memset(&g_egl, 0, sizeof(g_egl));
}

/* ---------------------------------------------------------------------------
 *  Windows
 * ------------------------------------------------------------------------- */

/* The configuration whose native visual is the one of the window (a see-through window has a 32-bit visual). */
static EGLConfig rvn_sg_egl_config_for(const RNativeWindow* native) {
	EGLConfig configs[256];
	EGLint count = 0, i;
	EGLConfig best = NULL;
	int best_score = -1;
	if (!eglGetConfigs(g_egl.display, configs, (EGLint)RVN_COUNT_OF(configs), &count)) return NULL;
	for (i = 0; i < count; i++) {
		EGLint renderable = 0, surface_type = 0, visual = 0, alpha = 0, depth = 0, samples = 0;
		int score;
		eglGetConfigAttrib(g_egl.display, configs[i], EGL_RENDERABLE_TYPE, &renderable);
		eglGetConfigAttrib(g_egl.display, configs[i], EGL_SURFACE_TYPE, &surface_type);
		if (!(renderable & EGL_OPENGL_ES3_BIT) || !(surface_type & EGL_WINDOW_BIT)) continue;
		eglGetConfigAttrib(g_egl.display, configs[i], EGL_NATIVE_VISUAL_ID, &visual);
		eglGetConfigAttrib(g_egl.display, configs[i], EGL_ALPHA_SIZE, &alpha);
		eglGetConfigAttrib(g_egl.display, configs[i], EGL_DEPTH_SIZE, &depth);
		eglGetConfigAttrib(g_egl.display, configs[i], EGL_SAMPLES, &samples);
		if ((uintptr_t)visual != native->visual) continue;
		/* the visual decides; then what the frame needs (alpha when see-through) and what it does not (depth, samples) */
		score = 100 + (native->transparent ? (alpha > 0 ? 10 : 0) : 0) - (depth > 0 ? 3 : 0) - (samples > 0 ? 3 : 0);
		if (score > best_score) {
			best_score = score;
			best = configs[i];
		}
	}
	return best;
}

int rvn_sg_present_create(rvn_sg_surface_t* surface, const RNativeWindow* native) {
	rvn_sg_egl_window_t* window;
	EGLConfig config;
	EGLSurface egl_surface;
	unsigned long xid;
	if (!g_egl.windows || native->system != R_WINDOW_SYSTEM_X11) return R_FALSE;
	config = rvn_sg_egl_config_for(native);
	if (!config) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_WARNING, "Sokol: no EGL configuration for the visual of the window");
		return R_FALSE;
	}
	xid = (unsigned long)native->window;
	if (g_egl.create_platform_window) egl_surface = g_egl.create_platform_window(g_egl.display, config, &xid, NULL);
	else egl_surface = eglCreateWindowSurface(g_egl.display, config, (EGLNativeWindowType)native->window, NULL);
	if (egl_surface == EGL_NO_SURFACE) return R_FALSE;
	window = (rvn_sg_egl_window_t*)rvn_calloc(sizeof(*window));
	window->surface = egl_surface;
	window->vsync = -1;
	surface->present = window;
	return R_TRUE;
}

void rvn_sg_present_destroy(rvn_sg_surface_t* surface) {
	rvn_sg_egl_window_t* window = (rvn_sg_egl_window_t*)surface->present;
	if (!window) return;
	if (g_egl.current == window->surface) {
		eglMakeCurrent(g_egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, g_egl.context);
		g_egl.current = EGL_NO_SURFACE;
	}
	eglDestroySurface(g_egl.display, window->surface);
	rvn_free(window);
	surface->present = NULL;
}

int rvn_sg_present_begin(rvn_sg_surface_t* surface, int vsync, sg_swapchain* swapchain) {
	rvn_sg_egl_window_t* window = (rvn_sg_egl_window_t*)surface->present;
	if (g_egl.current != window->surface) {
		if (!eglMakeCurrent(g_egl.display, window->surface, window->surface, g_egl.context)) return R_FALSE;
		g_egl.current = window->surface;
		/* (the binding of the framebuffer is not what sokol_gfx remembers any more) */
		sg_reset_state_cache();
	}
	if (window->vsync != vsync) {
		eglSwapInterval(g_egl.display, vsync ? 1 : 0);
		window->vsync = vsync;
	}
	swapchain->width = surface->width;
	swapchain->height = surface->height;
	swapchain->gl.framebuffer = 0;
	return R_TRUE;
}

void rvn_sg_present_end(rvn_sg_surface_t* surface) {
	rvn_sg_egl_window_t* window = (rvn_sg_egl_window_t*)surface->present;
	eglSwapBuffers(g_egl.display, window->surface);
}
