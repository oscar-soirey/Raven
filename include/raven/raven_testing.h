/**
 * Raven
 * raven_testing.h : tests of an interface. Find widgets, click, type, wait
 * until nothing moves, and compare the window with a reference image.
 *
 * A test is an ordinary program: it starts Raven, builds its interface, then
 * drives it as a user would and checks what it shows.
 *
 *     Rid window = Rwindow_create_from_xml_file("login.xml");
 *     Rtest_set_deterministic(R_TRUE);
 *     Rtest_type_text(Rtest_find(window, "user"), "ada");
 *     Rtest_click(Rtest_find(window, "form/login"));
 *     Rtest_settle(2.0);
 *     if (strcmp(Rtest_get_text(Rtest_find(window, "status")), "Welcome, ada") != 0) fail();
 *     if (!Rtest_expect_golden(window, "goldens/login.png", NULL, NULL)) fail();
 *
 * The input goes through the same path as the input of the window system
 * (the one of Rwindow_inject_mouse_button, Rwindow_inject_touch...): hover,
 * press, focus, double clicks, drags and gestures behave as they do for a
 * user, and no event is sent to the OS. The functions return as soon as the
 * input is handled; Rtest_pump and Rtest_settle then produce the frames
 * (animations, layout springs, inertia).
 *
 * Positions are in logical units. A `target` is a widget or a window: a
 * position is then relative to the top-left corner of the widget, or of the
 * window. A function that cannot act (a widget that is not valid, not shown,
 * or has no size yet) reports an error and returns R_FALSE.
 *
 * Golden images. Rtest_expect_golden captures the window (or a widget of it)
 * and compares it with a PNG file. When they differ it writes, beside the
 * golden (or in RGoldenOptions.output_directory), NAME.actual.png, what was
 * drawn, and NAME.diff.png, the golden faded with the different pixels in
 * red. With the environment variable RAVEN_UPDATE_GOLDENS=1 it writes the
 * golden instead, and the check passes.
 *
 * What is drawn depends on the backend and, slightly, on the GPU: keep the
 * goldens of one backend and one OS together (RGoldenOptions.per_platform
 * adds Rtest_get_platform_tag to the name), and allow a small tolerance for
 * the rounding of different GPUs. Text is rasterized by Raven itself with
 * its bundled fonts, so it does not change with the fonts of the system as
 * long as the interface uses the fonts of Raven or fonts it loads. The null
 * backend (R_BACKEND_NULL) draws nothing: its capture is the clear color.
 *
 * Every function must be called from the UI thread.
 */

#ifndef RAVEN_TESTING_H
#define RAVEN_TESTING_H

#include "raven_core.h"
#include "raven_window.h"

/** How Rtest_expect_golden compares. NULL options: a tolerance of 2 per channel and no different pixel. */
typedef struct RGoldenOptions {
	/** Largest difference on one channel (0..255) for which two pixels are still the same. */
	int channel_tolerance;
	/** Share of the pixels (0..1) that may differ while the images still match. */
	float max_diff_ratio;
	/** R_TRUE: the golden of this OS and backend, "button.png" read as "button.macos-metal.png" (Rtest_get_platform_tag). */
	int per_platform;
	/** Folder of the .actual.png and .diff.png written on a mismatch. NULL: beside the golden. */
	const char* output_directory;
} RGoldenOptions;

typedef enum RGoldenStatus{
	/** The capture matches the golden. */
	R_GOLDEN_MATCH = 0x0BA0,
	/** RAVEN_UPDATE_GOLDENS=1: the capture was written as the golden. */
	R_GOLDEN_UPDATED,
	/** Too many pixels differ: NAME.actual.png and NAME.diff.png were written. */
	R_GOLDEN_MISMATCH,
	/** The capture and the golden do not have the same size: NAME.actual.png was written. */
	R_GOLDEN_SIZE_MISMATCH,
	/** There is no golden yet: NAME.actual.png was written (run with RAVEN_UPDATE_GOLDENS=1 to keep it). */
	R_GOLDEN_MISSING,
	/** The window could not be captured, or a file could not be read or written. */
	R_GOLDEN_FAILED
}RGoldenStatus;

/** What Rtest_expect_golden found. */
typedef struct RGoldenResult {
	RGoldenStatus status;
	/** Size of the capture, in pixels. */
	int width, height;
	/** Pixels farther from the golden than the tolerance. */
	int differing_pixels;
	/** Largest difference found on one channel (0..255). */
	int max_channel_difference;
	/** The golden that was read or written (with the platform tag). Valid until the next call. */
	const char* golden_path;
	/** One line that says what happened, for the log of the test. Valid until the next call. */
	const char* message;
} RGoldenResult;

#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  SETUP
	 * ============================================================================ */

	/**
	 * @brief R_TRUE makes what is drawn depend only on what the test does:
	 * every frame advances the animations by 1/60 s (Rset_animation_time_step),
	 * and the carets of the text fields stay lit instead of blinking. R_FALSE
	 * gives the clock back (time step 0).
	 */
	RAVEN_API void Rtest_set_deterministic(int enabled);
	RAVEN_API int Rtest_is_deterministic(void);

	/**
	 * @return What the goldens of this run are made with, "OS-backend":
	 * "macos-metal", "linux-vulkan", "windows-vulkan", "web-webgl"... and
	 * "null" with the null backend.
	 */
	RAVEN_API const char* Rtest_get_platform_tag(void);

	/* ============================================================================
	 *  FINDING WIDGETS
	 * ============================================================================ */

	/**
	 * @brief Finds a widget by name (Rwidget_set_name, Name="" in XML). The
	 * path may have several names separated by '/': "settings/ok" is the
	 * widget named "ok" inside the one named "settings". In a window, the
	 * open popups (menus, dialogs) are searched first, top first, then the
	 * content.
	 * @param scope A window or a widget.
	 * @return R_INVALID_ID when nothing has that name (no error is reported).
	 */
	RAVEN_API Rid Rtest_find(Rid scope, const char* path);

	/* ============================================================================
	 *  MOUSE AND KEYBOARD
	 * ============================================================================ */

	/** @brief Moves the mouse to the middle of the widget, presses and releases the left button. */
	RAVEN_API int Rtest_click(Rid widget);
	/** @brief Two clicks in a row on the middle of the widget, seen as a double click. */
	RAVEN_API int Rtest_double_click(Rid widget);
	/** @brief A click of the right button on the middle of the widget (a context menu). */
	RAVEN_API int Rtest_right_click(Rid widget);
	/**
	 * @brief Clicks at a position of a widget or a window.
	 * @param clicks 1 for a click, 2 for a double click, 3 for a triple click.
	 * @param modifiers RModifier held during the clicks (R_MOD_SHIFT...).
	 */
	RAVEN_API int Rtest_click_at(Rid target, float x, float y, RMouseButton button, int clicks, Ruint modifiers);
	/** @brief Moves the mouse over the middle of the widget, without clicking. */
	RAVEN_API int Rtest_hover(Rid widget);
	/** @brief Moves the mouse to a position of a widget or a window. */
	RAVEN_API int Rtest_hover_at(Rid target, float x, float y);
	/**
	 * @brief Presses the left button on the middle of the widget, moves by
	 * (dx, dy) in several steps, and releases: a slider, a splitter, a
	 * window dragged by a widget.
	 */
	RAVEN_API int Rtest_drag(Rid widget, float dx, float dy);
	/** @brief Drags from the middle of one widget to the middle of another (drag and drop). */
	RAVEN_API int Rtest_drag_to(Rid from, Rid to);
	/**
	 * @brief Turns the mouse wheel over the middle of the widget: `wheel_y`
	 * notches (positive: toward the top, as a wheel turned away from the
	 * user), `wheel_x` notches sideways.
	 */
	RAVEN_API int Rtest_scroll(Rid widget, float wheel_x, float wheel_y);
	/**
	 * @brief Presses and releases a key, with RModifier held. A widget
	 * target gets the keyboard focus first; a window sends the key to what
	 * has the focus.
	 */
	RAVEN_API int Rtest_key(Rid target, RKey key, Ruint modifiers);
	/**
	 * @brief Types UTF-8 text as a keyboard would. A widget target gets the
	 * keyboard focus first. '\n', '\t' and '\b' are sent as the keys Enter,
	 * Tab and Backspace.
	 */
	RAVEN_API int Rtest_type_text(Rid target, const char* text);

	/* ============================================================================
	 *  TOUCH
	 * ============================================================================ */

	/** @brief A finger touches the middle of the widget and leaves at once. */
	RAVEN_API int Rtest_tap(Rid widget);
	/**
	 * @brief A finger put on the middle of the widget moves by (dx, dy) in
	 * `seconds`, and leaves: a fast swipe flicks a list, which then coasts
	 * (Rtest_settle lets it stop).
	 */
	RAVEN_API int Rtest_swipe(Rid widget, float dx, float dy, double seconds);

	/* ============================================================================
	 *  FRAMES
	 * ============================================================================ */

	/** @brief Produces `frames` frames (Rdraw_frame). @return R_FALSE once the application quits. */
	RAVEN_API int Rtest_pump(int frames);
	/**
	 * @return R_TRUE when nothing waits for a frame: no posted task, no layout
	 * to do, nothing to paint, no animation, no layout spring, no inertia.
	 * Windows that redraw all the time (Rwindow_set_continuous_redraw, a
	 * realtime viewport) and the timers are not counted.
	 */
	RAVEN_API int Rtest_is_idle(void);
	/**
	 * @brief Produces frames until Rtest_is_idle, each advancing the
	 * animations by the time step (1/60 s unless Rset_animation_time_step
	 * gives another), for at most `max_seconds` of that time.
	 * @return R_FALSE when it was still moving at the end.
	 */
	RAVEN_API int Rtest_settle(double max_seconds);

	/* ============================================================================
	 *  READING THE STATE
	 * ============================================================================ */

	/**
	 * @return What the user reads in the widget: the text of a field, else
	 * its label (as screen readers say it: the text of a Label, a Button, a
	 * Checkbox...). "" when it has none. Valid until the next call.
	 */
	RAVEN_API const char* Rtest_get_text(Rid widget);
	/**
	 * @return R_TRUE when the widget can be seen: in a window, Visible with
	 * all that holds it, on the page a Switcher or a TabView shows, and laid
	 * out with a size.
	 */
	RAVEN_API int Rtest_is_shown(Rid widget);
	/** @return R_TRUE for a checked Checkbox, Switch, RadioButton, toggle Button or menu item. */
	RAVEN_API int Rtest_is_checked(Rid widget);

	/* ============================================================================
	 *  GOLDEN IMAGES
	 * ============================================================================ */

	/**
	 * @brief Captures a window, or the rectangle of one widget, and compares
	 * it with the PNG file `golden_path` (see the top of this file).
	 * @param target A window or a widget.
	 * @param options NULL for the defaults.
	 * @param result Receives the details; may be NULL.
	 * @return R_TRUE when the capture matches (or the golden was written
	 * because RAVEN_UPDATE_GOLDENS=1).
	 */
	RAVEN_API int Rtest_expect_golden(Rid target, const char* golden_path, const RGoldenOptions* options, RGoldenResult* result);
	/**
	 * @brief Compares two RGBA8 images of the same size as Rtest_expect_golden
	 * does, without files.
	 * @param diff_rgba When not NULL, receives width * height * 4 bytes: the
	 * expected image faded, with the different pixels in red.
	 * @return The number of pixels farther apart than `channel_tolerance`;
	 * -1 when an image is missing.
	 */
	RAVEN_API int Rtest_compare_pixels(const uint8_t* actual, const uint8_t* expected, int width, int height,
		int channel_tolerance, int* max_channel_difference, uint8_t* diff_rgba);

	/* ============================================================================
	 *  THE TREE, PICTURES AND SCRIPTS
	 * ============================================================================ */

	/**
	 * @brief The widgets as JSON, what a test, a script or an agent reads of
	 * the interface. Each widget gives its "type" (its XML tag), "name" and
	 * "path" (for Rtest_find) when it has a name, its accessibility "role",
	 * its "text" (Rtest_get_text), "label" and "value" when they differ,
	 * "bounds" [x, y, width, height] in logical units of its window,
	 * "shown", "enabled", and when they apply "focused", "checked",
	 * "selected", "expanded", "number"/"minimum"/"maximum", the rows of a
	 * list ("items") and its "children", in the order of the layout.
	 * @param target A window or a widget; R_INVALID_ID for every window of
	 * the application: {"raven": version, "backend": tag, "windows": [...]},
	 * each with its "title", "size", "scale", "content", "popups".
	 * @return The JSON text, valid until the next call. "" on error.
	 */
	RAVEN_API const char* Rtest_dump_tree(Rid target);
	/** @brief Rtest_dump_tree written to a file ("-": the standard output). */
	RAVEN_API int Rtest_save_tree(Rid target, const char* path);
	/**
	 * @brief Captures a window, or the rectangle of a widget, into a PNG
	 * file. The null backend draws nothing: its picture is the clear color.
	 */
	RAVEN_API int Rtest_save_png(Rid target, const char* path);
	/**
	 * @brief Runs a script of steps, one per line, as RAVEN_SCRIPT does (see
	 * "Driving any application headlessly" in README.md):
	 *
	 *     click form/user
	 *     type "ada"
	 *     key Enter
	 *     expect status text "Welcome, ada"
	 *     snap after-login
	 *
	 * The frames are produced by the function (Rdraw_frame) until the script
	 * ends. A widget is named by its name or path, by "text:Sign in" (the
	 * first one shown that reads it), or a point of the window by "@x,y".
	 * @param script The text of the script.
	 * @param source_name The name its messages give it ("login.script"); may be NULL.
	 * @return R_TRUE when every step was done and every check was true;
	 * R_FALSE otherwise, with the reason in Rtest_get_script_error.
	 */
	RAVEN_API int Rtest_run_script(const char* script, const char* source_name);
	/** @return Why the last Rtest_run_script failed ("login.script:4: expect: ..."), "" when it passed. */
	RAVEN_API const char* Rtest_get_script_error(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_TESTING_H */
