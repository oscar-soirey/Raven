/**
 * Raven
 * Lightweight, multithreaded, retained-mode GUI library in C.
 *
 * raven_core.h : base types, lifecycle, main loop, threading, errors.
 *
 * Conventions:
 *   - every object is an opaque Rid handle, R_INVALID_ID on failure
 *   - functions are R<object>_<action>: Rwindow_create, Rwindow_destroy,
 *     Rwindow_is_valid, Rwindow_set_title, Rwindow_get_size...
 *   - types are RName (RInitDesc, REvent, RKey), callbacks are RNameCallback,
 *     constants and enum values are R_NAME
 *   - booleans are ints (R_TRUE / R_FALSE), colors are float r, g, b, a
 *   - strings are UTF-8 and always copied by the library
 *
 * Threading: unless a function is documented as thread-safe, it must be
 * called from the UI thread (the thread that called Rinit).
 */

#ifndef RAVEN_CORE_H
#define RAVEN_CORE_H

#define RAVEN_VERSION_MAJOR 0
#define RAVEN_VERSION_MINOR 2
#define RAVEN_VERSION_PATCH 2
#define RAVEN_API_VERSION "0.2"

#ifdef __cplusplus
 #include <cstdint>
 #include <cstddef>
#else
 #include <stdint.h>
 #include <stddef.h>
#endif


/** shared library export / import */

#if defined(_WIN32)
	#if defined(RAVEN_BUILD_DLL)
		#define RAVEN_API __declspec(dllexport)
	#elif defined(RAVEN_USE_DLL)
		#define RAVEN_API __declspec(dllimport)
	#else
		#define RAVEN_API
	#endif
#elif defined(__GNUC__) && defined(RAVEN_BUILD_DLL)
	#define RAVEN_API __attribute__((visibility("default")))
#else
	#define RAVEN_API
#endif


/**
 * Opaque handle to any Raven object (window, widget, font, texture, timer,
 * tree item). A handle carries the object type and a generation counter, so
 * using a deleted handle is detected and reported instead of crashing.
 * Both R_INVALID_ID and 0 are never valid handles.
 */
typedef uint32_t Rid;
typedef uint32_t Ruint;

#define R_FALSE ((int)0)
#define R_TRUE  ((int)1)

#define R_INVALID_ID ((Rid)-1)


typedef enum RBackend{
	/** Built-in Vulkan renderer. */
	R_BACKEND_VULKAN = 0x0001,
	/** Renders nothing. Used for headless runs and unit tests. */
	R_BACKEND_NULL,
	/** Backend supplied with Rbackend_register (see raven_backend.h). */
	R_BACKEND_CUSTOM,
    /** Browser renderer (Emscripten, WebGL 2). */
    R_BACKEND_WEBGL
}RBackend;

typedef enum RError{
	R_NO_ERROR = 0x0070,
	R_ERROR_INVALID_ID,
	R_ERROR_INVALID_ENUM,
	R_ERROR_INVALID_VALUE,
	R_ERROR_INVALID_OPERATION,
	R_ERROR_BACKEND,
	R_ERROR_PLATFORM,
	R_ERROR_OUT_OF_MEMORY,
	R_ERROR_INVALID_FILE_FORMAT,
	R_ERROR_FILE_NOT_FOUND,
	R_ERROR_XML,
	R_ERROR_WRONG_THREAD
}RError;

typedef enum RSeverity{
	R_SEVERITY_WEAK_WARNING = 0x0080,
	R_SEVERITY_WARNING,
	R_SEVERITY_ERROR,
	R_SEVERITY_FATAL
}RSeverity;


/**
 * Optional settings for Rinit_ex. A zero-initialized structure selects the
 * default for every field, so new fields can be added without breaking code.
 */
typedef struct RInitDesc {
	/** Rendering backend. 0 selects WebGL in browser builds, Vulkan otherwise. */
	RBackend backend;
	/** Application name reported to the graphics driver. May be NULL. */
	const char* app_name;
	/** Worker threads of the job system. 0 = one per spare CPU core, negative = none. */
	int worker_threads;
	/** R_TRUE renders on the UI thread instead of a dedicated render thread. */
	int disable_render_thread;
	/** R_TRUE presents without waiting for the vertical blank. */
	int disable_vsync;
	/** R_TRUE enables the backend debug layer (Vulkan validation) when installed. */
	int enable_validation;
	/** R_TRUE opens no OS window (null platform). Combine with R_BACKEND_NULL for tests. */
	int headless;
	/** Global UI scale multiplier applied on top of the monitor scale. 0 = 1.0. */
	float ui_scale;
} RInitDesc;

/** Memory functions of the application, to replace the allocator of Raven. See Rset_allocator. */
typedef struct RAllocator {
	void* (*alloc)(size_t size, void* user_data);
	void* (*realloc)(void* ptr, size_t size, void* user_data);
	void  (*free)(void* ptr, void* user_data);
	void* user_data;
} RAllocator;

/** What the library holds. See Rmemory_get_stats. */
typedef struct RMemoryStats {
	/** Bytes in the blocks currently allocated. 0 with the allocator of the application. */
	size_t used_bytes;
	/** Highest value of used_bytes since the start. */
	size_t peak_bytes;
	/** Bytes taken from the OS: used_bytes, plus what is free in the heap. */
	size_t reserved_bytes;
	/** Blocks currently allocated. 0 after Rshutdown: anything else is a leak of the library. */
	size_t allocation_count;
	/** Allocations made since the start. */
	uint64_t total_allocations;
} RMemoryStats;

/** Timings and counters of the last frame produced for a window. */
typedef struct RFrameStats {
	double layout_ms;
	double paint_ms;
	double render_ms;
	Ruint widget_count;
	Ruint instance_count;
	Ruint draw_call_count;
	/** Glass: times the frame was copied and blurred to be seen through (0 without glass). */
	Ruint backdrop_count;
	Ruint paint_job_count;
	uint64_t frame_index;
} RFrameStats;


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  INITIALIZATION & LIFECYCLE
	 * ============================================================================ */

	/**
	 * @brief Starts the library with default settings on the given backend.
	 * The calling thread becomes the UI thread.
	 * @return R_TRUE on success. On failure, see Rerror_get_last.
	 */
	RAVEN_API int Rinit(RBackend backend);

	/**
	 * @brief Starts the library with explicit settings.
	 * @param desc Settings, or NULL for the defaults.
	 */
	RAVEN_API int Rinit_ex(const RInitDesc* desc);

	/**
	 * @brief Destroys every window, widget and resource, stops the worker and
	 * render threads and releases the backend.
	 */
	RAVEN_API void Rshutdown(void);

	RAVEN_API int Ris_initialized(void);

	/** @return The library version, e.g. "0.1.0". */
	RAVEN_API const char* Rget_version(void);

	/* ============================================================================
	 *  MEMORY
	 * ============================================================================ */

	/**
	 * Raven does not call malloc. It has its own allocator: memory is taken
	 * from the OS by regions of 4 MB and handed out by a TLSF heap, which
	 * allocates and frees in constant time and merges free neighbours. All the
	 * memory of the library goes through it, the bundled font and image
	 * decoders included.
	 */

	/**
	 * @brief Replaces the allocator of Raven with the functions of the
	 * application (an engine allocator, a tracker...). NULL selects the
	 * allocator of Raven again. Must be called before Rinit, and before any
	 * other function that allocates. The functions are called from several
	 * threads; alloc must return memory aligned for any type (16 bytes).
	 */
	RAVEN_API void Rset_allocator(const RAllocator* allocator);

	/**
	 * @return An allocator that forwards to malloc, realloc and free of the C
	 * runtime: Rset_allocator(Rallocator_get_system()) is how memory tools
	 * that follow malloc (Valgrind, heap profilers) see the blocks of Raven.
	 * The environment variable RAVEN_ALLOCATOR=system does the same without
	 * recompiling, and it is the default in a build with AddressSanitizer.
	 */
	RAVEN_API const RAllocator* Rallocator_get_system(void);

	/**
	 * @brief Memory from the allocator of Raven (or from the one given to
	 * Rset_allocator), for the application: custom widgets, pixel buffers.
	 * Aligned on 16 bytes. Never returns NULL: running out of memory is a
	 * fatal error. Thread-safe.
	 */
	RAVEN_API void* Rmemory_alloc(size_t size);
	/** @brief Resizes a block, keeping its content. A NULL block is allocated. Thread-safe. */
	RAVEN_API void* Rmemory_realloc(void* ptr, size_t size);
	/** @brief Frees a block. NULL is ignored. Thread-safe. */
	RAVEN_API void Rmemory_free(void* ptr);

	/** @brief What the library holds right now. Thread-safe. */
	RAVEN_API void Rmemory_get_stats(RMemoryStats* stats);

	/**
	 * @brief Walks the whole heap of Raven and verifies its structure.
	 * @return R_FALSE when it is corrupted (a buffer overrun wrote over a block
	 * header). Slow: for tests and for hunting a bug. Thread-safe.
	 */
	RAVEN_API int Rmemory_check(void);

	/** @brief Returns to the OS the regions of the heap that hold nothing. Thread-safe. */
	RAVEN_API void Rmemory_trim(void);


	/* ============================================================================
	 *  MAIN LOOP
	 * ============================================================================ */

	/**
	 * Raven has no loop of its own: the application writes it and asks for one
	 * frame at a time.
	 *
	 *     while (Rdraw_frame()) {            a game or an engine: never waits,
	 *         update_and_render_my_scene();  call it once per frame of yours
	 *     }
	 *
	 *     while (Rwait_frame(-1.0)) {        a tool: sleeps until something
	 *     }                                  happens, costs nothing when idle
	 *
	 *     return Rget_exit_code();
	 */

	/**
	 * @brief Produces one frame: processes the pending OS events without
	 * waiting, runs the posted tasks and the timers, then lays out, paints and
	 * submits every window that changed. A window in which nothing changed is
	 * not drawn again, so the call is cheap when the UI is idle.
	 * @return R_FALSE once Rquit was called or the last window is closed.
	 */
	RAVEN_API int Rdraw_frame(void);

	/**
	 * @brief Same as Rdraw_frame, but first sleeps until there is something to
	 * do: an OS event, a posted task, a timer, an animation, a pending redraw.
	 * @param timeout_seconds Longest sleep. Negative sleeps without limit, 0
	 * does not sleep.
	 */
	RAVEN_API int Rwait_frame(double timeout_seconds);

	/** @brief Makes Rdraw_frame and Rwait_frame return R_FALSE. Thread-safe. */
	RAVEN_API void Rquit(int exit_code);

	/** @return The code given to Rquit, 0 when the last window was closed. */
	RAVEN_API int Rget_exit_code(void);

	/** @return R_FALSE once Rquit was called or every window is closed. */
	RAVEN_API int Ris_running(void);

	/**
	 * @brief Processes the pending OS events without producing a frame.
	 * Rdraw_frame does it already: only useful to keep the windows responsive
	 * during a long operation.
	 */
	RAVEN_API void Rpoll_events(void);

	/**
	 * @brief Sleeps until an OS event, a posted task, a timer or a pending
	 * redraw, and processes the OS events. Rwait_frame does it already.
	 * @param timeout_seconds Maximum wait. Negative waits without limit.
	 */
	RAVEN_API void Rwait_events(double timeout_seconds);

	/**
	 * @brief Registers a function called once per frame, before the layout.
	 * The body of your loop does the same job; the callback is also called
	 * while the OS keeps the thread in a loop of its own (on Windows, while
	 * the user moves or resizes a window), when your loop is not running.
	 */
	typedef void (*RFrameCallback)(double delta_seconds, void* user_data);
	RAVEN_API void Rset_frame_callback(RFrameCallback callback, void* user_data);

	/** @return Seconds between the last two frames. */
	RAVEN_API double Rget_delta_time(void);

	/** @return Seconds elapsed since Rinit, from a monotonic clock. Thread-safe. */
	RAVEN_API double Rget_time(void);


	/* ============================================================================
	 *  ANIMATIONS
	 * ============================================================================ */

	/**
	 * Controls move with springs: buttons squash when pressed and bounce back,
	 * switches slide and stretch, popups pop out of what opened them, lists
	 * scroll with inertia and stretch at their ends. Nothing has to be done
	 * for it; these functions only tune it. See also Rwidget_set_transition
	 * (widgets that fly in) and Rdraw_spring (custom widgets).
	 */

	/**
	 * @brief Scale of the whole interface, on top of the one of the monitor
	 * (RInitDesc.ui_scale at start): every window is laid out again. 0.25 .. 4.
	 */
	RAVEN_API void Rset_ui_scale(float scale);
	/**
	 * @brief R_TRUE: every frame waits for the vertical synchronization of the
	 * screen (no tearing, no wasted frames); R_FALSE: as fast as possible
	 * (RInitDesc.disable_vsync at start). Takes effect at the next frame.
	 */
	RAVEN_API void Rset_vsync(int enabled);
	RAVEN_API int Rget_vsync(void);
	RAVEN_API float Rget_ui_scale(void);

	/**
	 * @brief R_FALSE makes every change immediate: for users who ask for
	 * reduced motion (Raccessibility_set_reduce_motion), and for tests that
	 * read the state right after an input.
	 */
	RAVEN_API void Rset_animations_enabled(int enabled);
	RAVEN_API int Rget_animations_enabled(void);

	/** @brief Multiplies the speed of every animation. 1 is the default, 0.25 a slow motion. */
	RAVEN_API void Rset_animation_speed(float speed);
	RAVEN_API float Rget_animation_speed(void);

	/**
	 * @brief When positive, each frame advances the animations by exactly this
	 * many seconds instead of the time really elapsed: frames rendered one by
	 * one for a video or a screenshot, deterministic tests. 0 follows the clock.
	 */
	RAVEN_API void Rset_animation_time_step(double seconds);


	/* ============================================================================
	 *  THREADING
	 * ============================================================================ */

	typedef void (*RTaskCallback)(void* user_data);
	typedef void (*RParallelRangeCallback)(Ruint begin, Ruint end, void* user_data);

	/**
	 * @brief Queues a function to run on the UI thread during the next frame
	 * and wakes Rwait_frame. Thread-safe: this is how worker threads touch the UI.
	 */
	RAVEN_API void Rtask_post(RTaskCallback task, void* user_data);

	/** @brief Runs a function on a worker thread of the job system. Thread-safe. */
	RAVEN_API void Rtask_run_async(RTaskCallback task, void* user_data);

	/**
	 * @brief Splits [0, count) into ranges of at most `grain` items and runs them
	 * on the worker threads and the calling thread. Returns when all are done.
	 */
	RAVEN_API void Rparallel_for(Ruint count, Ruint grain, RParallelRangeCallback function, void* user_data);

	/** @brief Wakes Rwait_frame and Rwait_events from any thread. */
	RAVEN_API void Rwake_up(void);

	RAVEN_API int Rget_worker_thread_count(void);
	RAVEN_API int Ris_ui_thread(void);


	/* ============================================================================
	 *  TIMERS
	 * ============================================================================ */

	typedef void (*RTimerCallback)(Rid timer, void* user_data);

	/**
	 * @brief Calls a function on the UI thread after `interval_seconds`.
	 * @param repeat R_TRUE to fire repeatedly, R_FALSE to fire once and delete itself.
	 */
	RAVEN_API Rid Rtimer_create(double interval_seconds, int repeat, RTimerCallback callback, void* user_data);
	RAVEN_API void Rtimer_destroy(Rid timer);
	RAVEN_API int Rtimer_is_valid(Rid timer);


	/* ============================================================================
	 *  ACTIONS
	 * ============================================================================ */

	/**
	 * Actions connect widgets to application code by name. A widget carries an
	 * action name (set from XML with OnClicked="file.save", or from C with
	 * Rwidget_set_action); when its main event fires, the function bound to that
	 * name is called. Names are resolved when the event fires, so actions can be
	 * bound before or after the widgets are created.
	 */
	typedef void (*RActionCallback)(Rid sender, const char* action, void* user_data);

	RAVEN_API void Raction_bind(const char* name, RActionCallback callback, void* user_data);
	RAVEN_API void Raction_unbind(const char* name);
	/**
	 * @brief Fires an action as a widget would: the function bound to the
	 * name is called, or the default handler. What a tool bar button, a
	 * script or a test uses to do what a menu item does.
	 * @param sender Passed to the function; R_INVALID_ID when there is none.
	 */
	RAVEN_API void Raction_fire(const char* name, Rid sender);

	/** @brief Receives every action that has no bound function and is not one of the built-in actions (e.g. a script bridge). */
	RAVEN_API void Raction_set_default_handler(RActionCallback callback, void* user_data);

	/**
	 * @brief The actions Raven knows by itself: an interface written in XML
	 * uses them with no C at all (OnClicked="app.quit").
	 *
	 *   app.quit                          quits (Rquit(0))
	 *   window.close, window.minimize,    the window of the widget
	 *   window.maximize, window.restore   (maximize goes back when it already is)
	 *   window.picture_in_picture         in and out of picture in picture
	 *   island.expand, island.fold,       the island of that window
	 *   island.toggle
	 *   theme.dark, theme.light,          the look of Raven, dark or light
	 *   theme.toggle
	 *   theme:NAME                        Rtheme_use(NAME): a theme loaded or built in
	 *   language:CODE                     Rlocale_set_language(CODE)
	 *   open:ADDRESS                      Ropen_url (https:, mailto:, a file)
	 *   copy:TEXT                         the text to the clipboard
	 *   show:NAME, hide:NAME, toggle:NAME the widget named NAME in the window (hidden: Collapsed)
	 *   enable:NAME, disable:NAME
	 *   focus:NAME
	 *   page:NAME=2, page:NAME=+1         what a Switcher, a TabView, a ComboBox or a ListView
	 *                                     shows; +1 and -1 step, and go round
	 *
	 * A function bound to one of these names (Raction_bind) replaces it; the
	 * default handler receives the others only.
	 * @param enabled R_FALSE turns them all off: a design tool that shows
	 * interfaces without running them. On by default.
	 */
	RAVEN_API void Raction_set_builtins(int enabled);
	/** @return R_TRUE when the name is one of the built-in actions (Raction_set_builtins), whether they are on or not. */
	RAVEN_API int Raction_is_builtin(const char* name);


	/* ============================================================================
	 *  CLIPBOARD
	 * ============================================================================ */

	/**
	 * @brief Opens an address with the program the user has for it: a web
	 * page in the browser, a mailto: in the mail program, a file in its
	 * application. Returns at once.
	 * @return R_FALSE when the OS could not be asked (nothing tells whether
	 * a program then really opened it).
	 */
	RAVEN_API int Ropen_url(const char* url);

	RAVEN_API void Rclipboard_set_text(const char* text);

	/**
	 * @brief Copies one content in two formats: `html` for the programs that
	 * paste formatting (word processors, chat, mail) and `text` for the
	 * others (editors, terminals). Each program takes the format it prefers.
	 * Where the window system has no clipboard formats (Win32 and macOS for
	 * now), or `html` is NULL, it is Rclipboard_set_text(text).
	 */
	RAVEN_API void Rclipboard_set_html(const char* text, const char* html);

	/** @return The clipboard text, or "" when empty. Valid until the next call. */
	RAVEN_API const char* Rclipboard_get_text(void);


	/* ============================================================================
	 *  ERROR HANDLING
	 * ============================================================================ */

	/**
	 * @brief Retrieves and clears the last recorded error.
	 * @param detail   Optional output: human-readable description.
	 * @param severity Optional output: severity of the error.
	 * @return R_NO_ERROR when nothing was recorded.
	 */
	RAVEN_API RError Rerror_get_last(const char** detail, RSeverity* severity);

	RAVEN_API const char* Rerror_to_string(RError error);
	RAVEN_API const char* Rseverity_to_string(RSeverity severity);

	/**
	 * @brief Registers a function called whenever an error is raised.
	 * The function may be registered before Rinit. It is called on the
	 * thread that raised the error: errors of the renderer (and the messages
	 * of the Vulkan validation layers) come from the render thread.
	 */
	typedef void (*RErrorCallback)(RError code, RSeverity severity, const char* detail);
	RAVEN_API void Rerror_set_callback(RErrorCallback callback);

	/**
	 * @brief Registers a function called after Raven recovered from the loss
	 * of the GPU device (a driver reset or update, a GPU removed, a long sleep).
	 *
	 * Raven then starts the backend again and remakes what it can by itself:
	 * the surfaces of the windows, the fonts, the shaders, the textures made
	 * from image files and the small textures (up to 256 x 256). The pixels of
	 * the larger textures made from pixels are lost: the textures keep their
	 * handle and size but are transparent until Rtexture_update fills them
	 * again, which is what this function is for. Textures that wrap objects of
	 * the backend (raven_vulkan.h) must be made again, and the Vulkan handles
	 * returned by Rvulkan_get_device... have changed.
	 *
	 * Called on the UI thread, during Rdraw_frame / Rwait_frame. Set it after
	 * Rinit; NULL removes it.
	 */
	typedef void (*RDeviceResetCallback)(void* user_data);
	RAVEN_API void Rset_device_reset_callback(RDeviceResetCallback callback, void* user_data);


	/* ============================================================================
	 *  DIAGNOSTICS
	 * ============================================================================ */

	RAVEN_API void Rwindow_get_frame_stats(Rid window, RFrameStats* stats);

#ifdef __cplusplus
} /* extern "C" */
#endif


/**
 * @brief Prints the last Raven error to stdout, if any, with the source location.
 * Requires <stdio.h>.
 */
#define Rerror_check() do { \
	const char* rvn_e_ = 0; \
	RSeverity rvn_sev_ = R_SEVERITY_WEAK_WARNING; \
	RError rvn_err_ = Rerror_get_last(&rvn_e_, &rvn_sev_); \
	if (rvn_err_ != R_NO_ERROR) \
		printf("[Raven] %s | %s | %s | %s:%d\n", Rerror_to_string(rvn_err_), \
			Rseverity_to_string(rvn_sev_), rvn_e_, __FILE__, __LINE__); \
	} while(0)

#endif /* RAVEN_CORE_H */
