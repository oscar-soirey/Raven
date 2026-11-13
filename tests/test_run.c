/**
 * Raven tests - Rrun, the loop of the application.
 *
 * On a desktop Rrun runs the loop and returns when the application quits. Where the system owns the loop (a
 * browser: this test built with Emscripten) Rrun returns at once and the frames come later, so the same
 * program checks the other behavior there: its checks run in the `finished` callback.
 *
 * Headless (null platform, null backend).
 */

#include "test.h"

#include <stdlib.h>

static int g_frames;
static int g_finished_calls;
static int g_finished_code;
static void* g_finished_user;

static void on_frame(double delta, void* user) {
	(void)delta;
	(void)user;
	g_frames++;
	if (g_frames == 5) Rquit(7);
	/* an idle application sleeps until something happens: ask for the next frame (Rquit wakes the loop itself) */
	else Rwake_up();
}

#if defined(__EMSCRIPTEN__)
static void finish_test(void) {
	CHECK(g_frames == 5);
	CHECK(g_finished_calls == 1);
	CHECK(g_finished_code == 7);
	exit(test_report("test_run (browser loop)"));
}
#endif

static void on_finished(int exit_code, void* user) {
	g_finished_calls++;
	g_finished_code = exit_code;
	g_finished_user = user;
	Rshutdown();
#if defined(__EMSCRIPTEN__)
	finish_test();
#endif
}

#if !defined(__EMSCRIPTEN__)
/* Rrun without a callback returns the exit code and leaves the library running, for the application to shut down */
static void test_without_callback(void) {
	int code;
	g_frames = 0;
	CHECK(test_init(1));
	Rwindow_create("window", 100, 80, R_WINDOW_DEFAULT);
	Rset_frame_callback(on_frame, NULL);
	code = Rrun(NULL, NULL);
	CHECK(code == 7);
	CHECK(Rget_exit_code() == 7);
	CHECK(!Ris_running());
	Rshutdown();
}

/* the last window closing ends the application too: exit code 0 */
static void test_last_window_closed(void) {
	Rid window;
	int code;
	CHECK(test_init(1));
	window = Rwindow_create("window", 100, 80, R_WINDOW_DEFAULT);
	Rwindow_close(window);
	g_finished_calls = 0;
	code = Rrun(on_finished, NULL);
	CHECK(code == 0);
	CHECK(g_finished_calls == 1 && g_finished_code == 0);
}
#endif

int main(void) {
	static int token;
#if !defined(__EMSCRIPTEN__)
	int code;
#endif
	Rerror_set_callback(test_error_callback);
	CHECK(test_init(1));
	Rwindow_create("window", 100, 80, R_WINDOW_DEFAULT);
	Rset_frame_callback(on_frame, NULL);

#if defined(__EMSCRIPTEN__)
	/* the browser owns the loop: Rrun returns at once, with nothing run yet; the frames and `finished` come later */
	CHECK(Rrun(on_finished, &token) == 0);
	CHECK(g_frames == 0 && g_finished_calls == 0);
	return 0;
#else
	code = Rrun(on_finished, &token);
	/* the loop ran here, `finished` ran before Rrun returned, with what it was given */
	CHECK(code == 7);
	CHECK(g_frames == 5);
	CHECK(g_finished_calls == 1 && g_finished_code == 7 && g_finished_user == &token);
	test_without_callback();
	test_last_window_closed();
	return test_report("test_run");
#endif
}
