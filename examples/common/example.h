/**
 * Raven examples - helpers shared by the examples (not part of the library).
 *
 *   example_init          starts Raven and prints its errors to stderr
 *   example_automated     scripted runs of the examples (see below)
 *
 * For automated runs, two environment variables make an example render a
 * fixed number of frames, optionally save the last one, and exit:
 *
 *   RAVEN_EXAMPLE_FRAMES=30 RAVEN_EXAMPLE_CAPTURE=shot.ppm ./raven_editor
 *
 * RAVEN_EXAMPLE_FILM=prefix also saves one frame out of six as prefix000.ppm,
 * prefix001.ppm..., with the animations advancing by 1/60 s per frame.
 */

#ifndef RVN_EXAMPLE_H
#define RVN_EXAMPLE_H

#include <Raven.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <raven/raven_emscripten.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void example_on_error(RError code, RSeverity severity, const char* detail) {
	fprintf(stderr, "[Raven] %s (%s): %s\n", Rerror_to_string(code), Rseverity_to_string(severity), detail);
}

/**
 * Starts Raven. The backend is the default one (WebGL in a browser build, else Vulkan), or the one named by the
 * environment variable RAVEN_BACKEND (RAVEN_BACKEND=sokol: the sokol_gfx backend, in a build with
 * -DRAVEN_BACKEND_SOKOL=ON). Returns R_FALSE on failure.
 */
static int example_init(const char* name) {
	RInitDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.backend = 0;
	desc.app_name = name;
#ifndef NDEBUG
	desc.enable_validation = R_TRUE;
#endif
	Rerror_set_callback(example_on_error);
	if (!Rinit_ex(&desc)) {
		fprintf(stderr, "%s: cannot start Raven (is a graphics driver installed? RAVEN_BACKEND=%s)\n", name, getenv("RAVEN_BACKEND") ? getenv("RAVEN_BACKEND") : "default");
		return R_FALSE;
	}
	return R_TRUE;
}

/** Saves a window as a binary PPM image; as a PAM image, which keeps the alpha of a window that is seen through, when the name ends with ".pam". */
static void example_capture(Rid window, const char* path) {
	int width = 0, height = 0, i;
	uint8_t* pixels = Rwindow_capture(window, &width, &height);
	FILE* file = pixels ? fopen(path, "wb") : NULL;
	size_t length = strlen(path);
	if (file && length > 4 && strcmp(path + length - 4, ".pam") == 0) {
		fprintf(file, "P7\nWIDTH %d\nHEIGHT %d\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n", width, height);
		fwrite(pixels, 4, (size_t)width * (size_t)height, file);
		fclose(file);
	} else if (file) {
		fprintf(file, "P6\n%d %d\n255\n", width, height);
		for (i = 0; i < width * height; i++) fwrite(pixels + i * 4, 1, 3, file);
		fclose(file);
	}
	Rpixels_free(pixels);
}

static void example_report(Rid window) {
	if (Rwindow_is_valid(window)) {
		RFrameStats stats;
		Rwindow_get_frame_stats(window, &stats);
		printf("last frame: %u widgets, %u primitives, %u draw calls, %u backdrop captures, paint %.2f ms, render %.2f ms\n",
			stats.widget_count, stats.instance_count, stats.draw_call_count, stats.backdrop_count,
			stats.paint_ms, stats.render_ms);
	}
}

/**
 * Automated runs (RAVEN_EXAMPLE_FRAMES): renders a fixed number of frames,
 * optionally saves the last one, and returns R_TRUE: the example then skips
 * its loop. Returns R_FALSE when the example should run normally.
 */
#ifndef __EMSCRIPTEN__
static int example_automated(Rid window) {
	const char* frames = getenv("RAVEN_EXAMPLE_FRAMES");
	const char* capture = getenv("RAVEN_EXAMPLE_CAPTURE");
	const char* film = getenv("RAVEN_EXAMPLE_FILM");
	int count, i;
	if (!frames) return R_FALSE;
	count = atoi(frames);
	/* a film: the animations advance by 1/60 s per frame whatever the time it takes to render one */
	if (film) Rset_animation_time_step(1.0 / 60.0);
	for (i = 0; i < count && Ris_running(); i++) {
		Rwindow_request_redraw(window);
		Rwait_frame(0.01);
		if (film && i % 6 == 0 && Rwindow_is_valid(window)) {
			char path[512];
			snprintf(path, sizeof(path), "%s%03d.ppm", film, i / 6);
			example_capture(window, path);
		}
	}
	if (capture && Rwindow_is_valid(window)) example_capture(window, capture);
	example_report(window);
	return R_TRUE;
}

#endif /* !__EMSCRIPTEN__ */

#ifdef __EMSCRIPTEN__
static struct {
    Rid window;
    void (*frame)(void);
    void (*shutdown)(void);
    int automated, count, index;
    const char* capture;
    const char* film;
} example_loop;

static void example_browser_frame(void) {
    int running = R_FALSE;
    if (example_loop.automated && example_loop.index >= example_loop.count)
        goto finished;
    if (example_loop.automated)
        Rwindow_request_redraw(example_loop.window);
    running = Rdraw_frame();
    if (running && example_loop.frame) example_loop.frame();
    if (example_loop.film && example_loop.index % 6 == 0 &&
        Rwindow_is_valid(example_loop.window)) {
        char path[512];
        snprintf(path, sizeof(path), "%s%03d.ppm", example_loop.film,
                 example_loop.index / 6);
        example_capture(example_loop.window, path);
    }
    example_loop.index++;
    if (running && (!example_loop.automated ||
                    example_loop.index < example_loop.count)) return;
finished:
    emscripten_cancel_main_loop();
    if (example_loop.capture && Rwindow_is_valid(example_loop.window))
        example_capture(example_loop.window, example_loop.capture);
    if (example_loop.automated) example_report(example_loop.window);
    if (example_loop.shutdown) example_loop.shutdown();
    else Rshutdown();
}
#endif

#ifdef RAVEN_EXAMPLE_SYSTEM_LOOP
/* Rrun, for the platforms where the system owns the loop (iOS, the web platform of the Sokol backend) */
static struct {
    void (*frame)(void);
    void (*shutdown)(void);
} example_system;

static void example_system_frame(double delta_seconds, void* user_data) {
    (void)delta_seconds;
    (void)user_data;
    if (example_system.frame) example_system.frame();
}

static void example_system_finished(int exit_code, void* user_data) {
    (void)exit_code;
    (void)user_data;
    if (example_system.shutdown) example_system.shutdown();
    else Rshutdown();
}
#endif

/** Runs the application without blocking browser events.
 * frame runs after each drawn frame. shutdown owns Rshutdown when supplied. */
static void example_run(Rid window, void (*frame)(void),
                        void (*shutdown)(void)) {
#if defined(__EMSCRIPTEN__) && defined(RAVEN_EXAMPLE_SYSTEM_LOOP)
    /* the web platform: the browser calls the frames of Rrun, which returns at once;
     * the application ends in example_system_finished */
    (void)window;
    example_system.frame = frame;
    example_system.shutdown = shutdown;
    if (frame) Rset_frame_callback(example_system_frame, NULL);
    Rrun(example_system_finished, NULL);
#elif defined(__EMSCRIPTEN__)
    const char* frames = getenv("RAVEN_EXAMPLE_FRAMES");
    example_loop.window = window;
    example_loop.frame = frame;
    example_loop.shutdown = shutdown;
    example_loop.automated = frames != NULL;
    example_loop.count = frames ? atoi(frames) : 0;
    example_loop.index = 0;
    example_loop.capture = frames ? getenv("RAVEN_EXAMPLE_CAPTURE") : NULL;
    example_loop.film = frames ? getenv("RAVEN_EXAMPLE_FILM") : NULL;
    if (example_loop.film) Rset_animation_time_step(1.0 / 60.0);
    emscripten_set_main_loop(example_browser_frame, 0, 0);
#elif defined(RAVEN_EXAMPLE_SYSTEM_LOOP)
    /* the system owns the loop (iOS): Rrun hands the frames to it and returns at once,
     * and the application ends in example_system_finished */
    example_system.frame = frame;
    example_system.shutdown = shutdown;
    if (example_automated(window)) {
        example_system_finished(0, NULL);
        return;
    }
    if (frame) Rset_frame_callback(example_system_frame, NULL);
    Rrun(example_system_finished, NULL);
#else
    if (!example_automated(window)) {
        while (Rwait_frame(-1.0)) {
            if (frame) frame();
        }
    }
    if (shutdown) shutdown();
    else Rshutdown();
#endif
}

#endif /* RVN_EXAMPLE_H */
