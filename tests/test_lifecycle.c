/* The lifecycle of a mobile application: pause and resume, low memory, the native surface lost and made again, Back. */
#include "test.h"
#include "../src/widget/widget.h"
#include "../src/render/renderer.h"

static RBackendVTable g_null;
static struct {
	int frames, surfaces_made, surfaces_destroyed, idle_waits;
	RAppEvent events[16];
	int event_count;
	/* what the back callback answers, and how many times it ran */
	int back_answer, back_calls;
	int closed, closed_result;
} g_seen;

static int count_create_surface(Rid window, const RNativeWindow* native, Ruint width, Ruint height) {
	g_seen.surfaces_made++;
	return g_null.CreateSurface(window, native, width, height);
}

static void count_destroy_surface(Rid window) {
	g_seen.surfaces_destroyed++;
	g_null.DestroySurface(window);
}

static void count_frame(const RDrawData* data, RCapture* capture) {
	g_seen.frames++;
	g_null.RenderFrame(data, capture);
}

static void count_wait_idle(void) {
	g_seen.idle_waits++;
	g_null.WaitIdle();
}

static void on_app_event(RAppEvent event, void* user_data) {
	(void)user_data;
	/* (the state is already the new one when the application is told) */
	if (event == R_APP_DID_PAUSE) CHECK(Ris_app_paused());
	if (event == R_APP_WILL_RESUME) CHECK(!Ris_app_paused());
	if (g_seen.event_count < 16) g_seen.events[g_seen.event_count++] = event;
}

static int on_back(Rid window, void* user_data) {
	(void)window;
	(void)user_data;
	g_seen.back_calls++;
	return g_seen.back_answer;
}

static void on_closed(Rid content, int result, void* user_data) {
	(void)content;
	(void)user_data;
	g_seen.closed++;
	g_seen.closed_result = result;
}

/* Sends an event to the core as the window system of a phone would. */
static int send_platform_event(Rid window, rvn_pevent_type_t type, int app_event) {
	rvn_pevent_t event;
	int accepted = -1;
	memset(&event, 0, sizeof(event));
	event.type = type;
	event.user = window != R_INVALID_ID ? rvn_window_check(window, "send_platform_event") : NULL;
	event.app_event = app_event;
	event.accepted = &accepted;
	rvn_window_on_platform_event(&event);
	return accepted;
}

static Rid make_window(Rid* button) {
	Rid window = Rwindow_create("lifecycle", 400, 300, R_WINDOW_DEFAULT);
	Rid column = Rwidget_create(R_WIDGET_VBOX);
	Rwindow_set_content(window, column);
	*button = Rbutton_create("Hello");
	Rwidget_add_child(column, *button);
	Rdraw_frame();
	return window;
}

/* One frame after asking for one: R_TRUE when it was drawn. */
static int drawn(Rid window) {
	int before = g_seen.frames;
	Rwindow_request_redraw(window);
	Rdraw_frame();
	return g_seen.frames > before;
}

static void test_pause_resume(void) {
	Rid button, window = make_window(&button);
	int idle;
	memset(&g_seen.events, 0, sizeof(g_seen.events));
	g_seen.event_count = 0;
	Rset_app_event_callback(on_app_event, NULL);
	CHECK(!Ris_app_paused());
	CHECK(drawn(window));

	/* in front but about to leave: still drawn */
	send_platform_event(R_INVALID_ID, RVN_PEVENT_APP, R_APP_WILL_PAUSE);
	CHECK(!Ris_app_paused());
	CHECK(drawn(window));

	/* in the background: the GPU is idle when the platform gets control back, and nothing is drawn */
	idle = g_seen.idle_waits;
	send_platform_event(R_INVALID_ID, RVN_PEVENT_APP, R_APP_DID_PAUSE);
	CHECK(Ris_app_paused());
	CHECK(g_seen.idle_waits == idle + 1);
	CHECK(!drawn(window));
	/* still laid out: a change of the content is there when it comes back */
	Rbutton_set_text(button, "Changed in the background");
	Rdraw_frame();
	CHECK(!drawn(window));
	{
		float x, y, w, h, before;
		Rwidget_get_rect(button, &x, &y, &w, &before);
		Rwidget_set_min_size(button, 0, 120);
		Rdraw_frame();
		Rwidget_get_rect(button, &x, &y, &w, &h);
		CHECK_NEAR(h, 120);
		CHECK(h != before);
	}

	/* coming back: drawn again, without being asked */
	send_platform_event(R_INVALID_ID, RVN_PEVENT_APP, R_APP_WILL_RESUME);
	CHECK(!Ris_app_paused());
	{
		int before = g_seen.frames;
		Rdraw_frame();
		CHECK(g_seen.frames > before);
	}
	send_platform_event(R_INVALID_ID, RVN_PEVENT_APP, R_APP_DID_RESUME);
	CHECK(drawn(window));

	/* a pause that ends without going to the background (notifications pulled down) */
	Rinject_app_event(R_APP_WILL_PAUSE);
	Rinject_app_event(R_APP_DID_RESUME);
	CHECK(!Ris_app_paused());
	/* a platform that misses WILL_RESUME: DID_RESUME draws again too */
	Rinject_app_event(R_APP_DID_PAUSE);
	Rinject_app_event(R_APP_DID_RESUME);
	CHECK(!Ris_app_paused());
	CHECK(drawn(window));

	CHECK(g_seen.event_count == 8);
	CHECK(g_seen.events[0] == R_APP_WILL_PAUSE);
	CHECK(g_seen.events[1] == R_APP_DID_PAUSE);
	CHECK(g_seen.events[2] == R_APP_WILL_RESUME);
	CHECK(g_seen.events[3] == R_APP_DID_RESUME);
	CHECK(g_seen.events[4] == R_APP_WILL_PAUSE);
	CHECK(g_seen.events[5] == R_APP_DID_RESUME);

	EXPECT_ERRORS_BEGIN();
	Rinject_app_event((RAppEvent)12345);
	EXPECT_ERRORS_END(1);
	CHECK(g_seen.event_count == 8);

	/* desktop: nothing of this happens without the events */
	Rset_app_event_callback(NULL, NULL);
	Rwindow_destroy(window);
	Rdraw_frame();
}

static void test_low_memory(void) {
	Rid button, window = make_window(&button);
	RMemoryStats before, after;
	int i;
	g_seen.event_count = 0;
	Rset_app_event_callback(on_app_event, NULL);
	/* a few frames: their draw lists are kept for the next ones */
	for (i = 0; i < 4; i++) CHECK(drawn(window));
	Rmemory_get_stats(&before);
	Rinject_app_event(R_APP_LOW_MEMORY);
	Rmemory_get_stats(&after);
	CHECK(g_seen.event_count == 1 && g_seen.events[0] == R_APP_LOW_MEMORY);
	CHECK(after.allocation_count < before.allocation_count);
	/* what was dropped is made again */
	CHECK(drawn(window));
	CHECK(Rwidget_is_valid(button));
	Rset_app_event_callback(NULL, NULL);
	Rwindow_destroy(window);
	Rdraw_frame();
}

static void test_surface_lost_and_ready(void) {
	Rid button, window = make_window(&button);
	int made = g_seen.surfaces_made, destroyed = g_seen.surfaces_destroyed;
	CHECK(drawn(window));

	/* the native surface goes away: the renderer's surface is destroyed before the platform goes on */
	send_platform_event(window, RVN_PEVENT_SURFACE_LOST, 0);
	CHECK(g_seen.surfaces_destroyed == destroyed + 1);
	CHECK(!drawn(window));
	/* (lost twice: nothing more to destroy) */
	send_platform_event(window, RVN_PEVENT_SURFACE_LOST, 0);
	CHECK(g_seen.surfaces_destroyed == destroyed + 1);

	/* the window and its widgets stay, and still take input */
	CHECK(Rwindow_is_valid(window));
	CHECK(Rwidget_is_valid(button));
	Rbutton_set_text(button, "Still here");
	Rdraw_frame();
	CHECK(!drawn(window));

	/* a new native surface: the same window draws on it */
	send_platform_event(window, RVN_PEVENT_SURFACE_READY, 0);
	CHECK(g_seen.surfaces_made == made + 1);
	{
		int before = g_seen.frames;
		Rdraw_frame();
		CHECK(g_seen.frames > before);
	}
	CHECK(Rwidget_is_valid(button));
	/* ready while it has one (a new handle at once): the old one goes first */
	send_platform_event(window, RVN_PEVENT_SURFACE_READY, 0);
	CHECK(g_seen.surfaces_destroyed == destroyed + 2);
	CHECK(g_seen.surfaces_made == made + 2);
	CHECK(drawn(window));

	/* lost while paused, back while paused: drawn only once resumed */
	Rinject_app_event(R_APP_DID_PAUSE);
	send_platform_event(window, RVN_PEVENT_SURFACE_LOST, 0);
	send_platform_event(window, RVN_PEVENT_SURFACE_READY, 0);
	CHECK(!drawn(window));
	Rinject_app_event(R_APP_WILL_RESUME);
	CHECK(drawn(window));
	Rwindow_destroy(window);
	Rdraw_frame();
}

static void test_back(void) {
	Rid button, content, window = make_window(&button);
	memset(&g_seen.events, 0, sizeof(g_seen.events));
	g_seen.back_calls = 0;

	/* nothing to go back to: the system does its default */
	CHECK(!Rwindow_wants_back(window));
	CHECK(!Rwindow_inject_back(window));
	CHECK(send_platform_event(window, RVN_PEVENT_BACK, 0) == R_FALSE);

	/* the application decides */
	Rwindow_set_back_callback(window, on_back, NULL);
	g_seen.back_answer = R_TRUE;
	CHECK(Rwindow_inject_back(window));
	CHECK(g_seen.back_calls == 1);
	g_seen.back_answer = R_FALSE;
	CHECK(send_platform_event(window, RVN_PEVENT_BACK, 0) == R_FALSE);
	CHECK(g_seen.back_calls == 2);
	CHECK(!Rwindow_wants_back(window));
	Rwindow_set_back_enabled(window, R_TRUE);
	CHECK(Rwindow_wants_back(window));
	Rwindow_set_back_enabled(window, R_FALSE);
	CHECK(!Rwindow_wants_back(window));

	/* a dialog closes first, as with Escape, and the application is not asked */
	content = Rlabel_create("Sure?");
	g_seen.closed = 0;
	g_seen.closed_result = -1;
	Rdialog_open(window, content, on_closed, NULL);
	CHECK(Rwindow_wants_back(window));
	g_seen.back_answer = R_TRUE;
	CHECK(send_platform_event(window, RVN_PEVENT_BACK, 0) == R_TRUE);
	CHECK(g_seen.closed == 1 && g_seen.closed_result == 0);
	CHECK(!Rwindow_has_dialog(window));
	CHECK(g_seen.back_calls == 2);
	CHECK(!Rwindow_wants_back(window));

	/* a popup too */
	content = Rlabel_create("A popup");
	Rpopup_open(window, content, 10.0f, 10.0f, NULL, NULL);
	CHECK(Rwindow_has_popup(window));
	CHECK(Rwindow_wants_back(window));
	CHECK(Rwindow_inject_back(window));
	CHECK(!Rwindow_has_popup(window));
	CHECK(g_seen.back_calls == 2);
	Rwidget_destroy(content);

	/* Back is not Escape: a widget waiting for Escape sees no key */
	CHECK(!Rwindow_is_key_down(window, R_KEY_ESCAPE));
	Rwindow_destroy(window);
	Rdraw_frame();
}

int main(void) {
	RInitDesc desc;
	g_null = *rvn_backend_null();
	{
		RBackendVTable backend = g_null;
		backend.CreateSurface = count_create_surface;
		backend.DestroySurface = count_destroy_surface;
		backend.RenderFrame = count_frame;
		backend.WaitIdle = count_wait_idle;
		Rbackend_register(&backend);
	}
	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_CUSTOM;
	desc.headless = R_TRUE;
	desc.worker_threads = 1;
	desc.disable_render_thread = R_TRUE;
	Rerror_set_callback(test_error_callback);
	if (!Rinit_ex(&desc)) return 1;
	Rset_animations_enabled(R_FALSE);
	RUN(test_pause_resume);
	RUN(test_low_memory);
	RUN(test_surface_lost_and_ready);
	RUN(test_back);
	Rshutdown();
	return test_report("test_lifecycle");
}
