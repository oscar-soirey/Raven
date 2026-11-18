/*
 * Raven mobile showcase: the UI shared by the iOS and the Android app.
 * assets/showcase.xml holds the layout; this file fills and wires it.
 *
 * Tabs (bottom bar): List (1,000 rows + a detail page), Controls, Glass, Form,
 * Device (permissions, camera, pickers, haptics, links, sharing, clipboard,
 * an image from the network: raven_device.h; every answer comes later, in a
 * callback, and says "not supported here" where the platform has no such
 * service).
 * "Back" = the Back button, Escape, a swipe from the left edge, or the system's
 * Back (Android: Rwindow_set_back_callback; the app tells Raven when it has
 * somewhere to go back to, so that at the root the system leaves the app).
 * The List page shows the last events of the application's lifecycle.
 */
#include "showcase.h"

#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#define ROW_COUNT 1000
#define TAB_COUNT 5
/* a small, stable public image (Raven's logo, 37 KB); Android needs INTERNET in its manifest */
#define NET_IMAGE_URL "https://raw.githubusercontent.com/oscar-soirey/Raven/main/docs/images/logo-black.png"
#define RAVEN_PAGE_URL "https://github.com/oscar-soirey/Raven"

static struct {
	Rid window, root, pages, listnav, list, detail, list_page;
	Rid detail_title, detail_subtitle, detail_body, detail_swatch;
	Rid tabs[TAB_COUNT];
	Rid counter_label, slider_value, progress, progress_value, dialog_result;
	Rid blur_value, frame_stats, blurs[4];
	Rid name, email, phone, city, note, form_state, keyboard_state;
	Rid toast, lifecycle;
	Rid circle;
	Rid progress_timer, stats_timer;
	int tab, taps, detail_index, dark;
	float progress_now;
	/* edge swipe on the detail page */
	int swipe_active;
	float swipe_x0, swipe_y0;
	/* frame times seen by the frame callback (Glass tab) */
	double frame_sum, frame_max;
	int frame_count;
	/* the last lifecycle events, oldest first, and how many low-memory warnings came */
	RAppEvent events[4];
	int event_count, low_memory;
	/* device tab */
	Rid dev_status, perm_camera, perm_notify, perm_location, photo_info, photo_view, file_info, file_view, clip_info, net_info, net_view;
	Rid share_button, net_timer;
	Rid photo_texture, file_texture, net_texture;
	char photo_path[1024];
} sc;

void showcase_log(const char* format, ...) {
	va_list args;
	va_start(args, format);
#ifdef __ANDROID__
	__android_log_vprint(ANDROID_LOG_INFO, "RavenShowcase", format, args);
#else
	fprintf(stderr, "[showcase] ");
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
#endif
	va_end(args);
}

/* ---------------------------------------------------------------- the data */

static const char* const g_first[] = {
	"Amber", "Bright", "Cobalt", "Dusty", "Electric", "Frosty", "Golden", "Hidden", "Ivory", "Jade",
	"Kind", "Lunar", "Misty", "Noble", "Olive", "Polar", "Quiet", "Rusty", "Silver", "Tidal"
};
static const char* const g_second[] = {
	"Falcon", "River", "Meadow", "Harbor", "Canyon", "Lantern", "Orchard", "Comet", "Willow", "Summit",
	"Garden", "Island", "Beacon", "Forest", "Valley", "Glacier", "Prairie", "Reef", "Tundra", "Dune"
};

static void row_title(int index, char* out, size_t size) {
	snprintf(out, size, "%s %s", g_first[index % 20], g_second[(index / 20 + index * 7) % 20]);
}
static void row_subtitle(int index, char* out, size_t size) {
	snprintf(out, size, "Row %d of %d  ·  updated %d min ago", index + 1, ROW_COUNT, (index * 37) % 60 + 1);
}
static void row_color(int index, float* r, float* g, float* b) {
	float h = fmodf((float)index * 0.137f, 1.0f) * 6.0f;
	float x = 1.0f - fabsf(fmodf(h, 2.0f) - 1.0f);
	float c = 0.75f, m = 0.2f;
	float rr = 0, gg = 0, bb = 0;
	if (h < 1) { rr = c; gg = c * x; } else if (h < 2) { rr = c * x; gg = c; } else if (h < 3) { gg = c; bb = c * x; }
	else if (h < 4) { gg = c * x; bb = c; } else if (h < 5) { rr = c * x; bb = c; } else { rr = c; bb = c * x; }
	*r = rr + m; *g = gg + m; *b = bb + m;
}

/* A white disc (tinted per row): the "avatar" of each row. */
static Rid make_circle(void) {
	enum { N = 64 };
	static uint8_t pixels[N * N * 4];
	int x, y;
	for (y = 0; y < N; y++) {
		for (x = 0; x < N; x++) {
			float dx = (float)x + 0.5f - N * 0.5f, dy = (float)y + 0.5f - N * 0.5f;
			float a = N * 0.5f - sqrtf(dx * dx + dy * dy);
			uint8_t* p = pixels + (y * N + x) * 4;
			a = a < 0 ? 0 : (a > 1 ? 1 : a);
			p[0] = p[1] = p[2] = 255;
			p[3] = (uint8_t)(a * 255.0f);
		}
	}
	return Rtexture_create_from_pixels(pixels, N, N);
}

static void on_tile_item(Rid tile_view, int index, RTileItem* item, void* user_data) {
	static char title[64], detail[96], badge[16];
	(void)tile_view; (void)user_data;
	row_title(index, title, sizeof(title));
	row_subtitle(index, detail, sizeof(detail));
	snprintf(badge, sizeof(badge), "#%d", index + 1);
	item->text = title;
	item->detail = detail;
	item->badge = badge;
	item->icon = sc.circle;
	item->uv[0] = 0; item->uv[1] = 0; item->uv[2] = 1; item->uv[3] = 1;
	row_color(index, &item->tint[0], &item->tint[1], &item->tint[2]);
	item->tint[3] = 1.0f;
}

/* ---------------------------------------------------------------- navigation */

static void update_back(void);

static void open_detail(int index) {
	char text[160];
	float r, g, b;
	sc.detail_index = index;
	row_title(index, text, sizeof(text));
	Rlabel_set_text(sc.detail_title, text);
	row_subtitle(index, text, sizeof(text));
	Rlabel_set_text(sc.detail_subtitle, text);
	snprintf(text, sizeof(text), "This is the detail page of row %d. It was opened by a tap on the list; "
		"the list keeps its scroll position while this page is shown.", index + 1);
	Rlabel_set_text(sc.detail_body, text);
	row_color(index, &r, &g, &b);
	Rborder_set_background(sc.detail_swatch, r, g, b, 1.0f);
	Rswitcher_set_index(sc.listnav, 1);
	Rwidget_play_transition(sc.detail, R_TRANSITION_SLIDE_LEFT, 0.0f);
	update_back();
	showcase_log("open detail %d", index);
}

static int detail_shown(void) {
	return sc.tab == 0 && Rswitcher_get_index(sc.listnav) == 1;
}

static void close_detail(void) {
	Rswitcher_set_index(sc.listnav, 0);
	/* (so that a tap on the same row opens it again) */
	Rtile_view_clear_selection(sc.list);
	Rwidget_play_transition(sc.list_page, R_TRANSITION_SLIDE_RIGHT, 0.0f);
	showcase_log("back to the list");
}

static void set_tab(int tab);

/* Tells Raven whether "back" has somewhere to go (Android asks for Back only then). */
static void update_back(void) {
	Rwindow_set_back_enabled(sc.window, detail_shown() || sc.tab != 0);
}

/* What "back" does: R_TRUE when it went somewhere, R_FALSE when there is nothing left. */
static int go_back(void) {
	int went = R_TRUE;
	if (detail_shown()) close_detail();
	else if (sc.tab != 0) set_tab(0);
	else went = R_FALSE;
	update_back();
	return went;
}

/* The system's Back (Android), when no dialog or popup of Raven took it. R_FALSE: the system leaves the app. */
static int on_system_back(Rid window, void* user_data) {
	(void)window; (void)user_data;
	if (go_back()) return R_TRUE;
	showcase_log("back: nothing left, the system leaves the app");
	return R_FALSE;
}

static void on_list_selection(Rid tile_view, int index, void* user_data) {
	(void)tile_view; (void)user_data;
	if (index >= 0 && !detail_shown()) open_detail(index);
}

static void on_back(Rid button, void* user_data) {
	(void)button; (void)user_data;
	go_back();
}

/* Edge swipe (iOS convention) on the detail page: a finger from the left edge moving right. */
static int on_detail_touch(Rid widget, const RTouchEvent* event, void* user_data) {
	(void)widget; (void)user_data;
	if (event->phase == R_TOUCH_DOWN) {
		RInsets safe;
		Rwindow_get_safe_area(sc.window, &safe);
		sc.swipe_active = event->x < safe.left + 24.0f;
		sc.swipe_x0 = event->x;
		sc.swipe_y0 = event->y;
		showcase_log("detail touch down at %.0f,%.0f (edge swipe %d)", event->x, event->y, sc.swipe_active);
		return sc.swipe_active;
	}
	if (!sc.swipe_active) return R_FALSE;
	if (event->phase == R_TOUCH_UP) {
		sc.swipe_active = R_FALSE;
		if (event->x - sc.swipe_x0 > 80.0f && fabsf(event->y - sc.swipe_y0) < 120.0f) {
			showcase_log("edge swipe: back");
			go_back();
		}
	} else if (event->phase == R_TOUCH_CANCEL) {
		sc.swipe_active = R_FALSE;
	}
	return R_TRUE;
}

/* Every input event of the window, before the widgets: Escape of a hardware keyboard goes back too, and a trace of keys / text. */
static int on_window_event(Rid window, const REvent* event, void* user_data) {
	(void)window; (void)user_data;
	if (event->type == R_EVENT_KEY_DOWN || event->type == R_EVENT_KEY_UP)
		showcase_log("key %s %d (focused widget %llu)", event->type == R_EVENT_KEY_DOWN ? "down" : "up", (int)event->key,
			(unsigned long long)Rwindow_get_focused_widget(sc.window));
	else if (event->type == R_EVENT_TEXT)
		showcase_log("text U+%04X", (unsigned)event->codepoint);
	/* (a dialog or popup open: Raven closes it on Escape itself) */
	if (event->type == R_EVENT_KEY_DOWN && event->key == R_KEY_ESCAPE && !Rwindow_has_popup(sc.window)) return go_back();
	return R_FALSE;
}

/* ---------------------------------------------------------------- lifecycle */

static const char* app_event_name(RAppEvent event) {
	switch (event) {
	case R_APP_WILL_PAUSE: return "will pause";
	case R_APP_DID_PAUSE: return "paused";
	case R_APP_WILL_RESUME: return "will resume";
	case R_APP_DID_RESUME: return "resumed";
	case R_APP_LOW_MEMORY: return "low memory";
	case R_APP_WILL_TERMINATE: return "will terminate";
	default: return "?";
	}
}

static void set_tab_timers(void);
static void show_permissions(void);

/* The application went to the background or came back (Rset_app_event_callback). */
static void on_app_event(RAppEvent event, void* user_data) {
	char text[160];
	int i;
	size_t used;
	(void)user_data;
	showcase_log("lifecycle: %s", app_event_name(event));
	if (event == R_APP_LOW_MEMORY) sc.low_memory++;
	if (sc.event_count == 4) memmove(sc.events, sc.events + 1, 3 * sizeof(sc.events[0]));
	else sc.event_count++;
	sc.events[sc.event_count - 1] = event;
	used = (size_t)snprintf(text, sizeof(text), "Lifecycle:");
	for (i = 0; i < sc.event_count && used < sizeof(text); i++)
		used += (size_t)snprintf(text + used, sizeof(text) - used, "%s %s", i ? "," : "", app_event_name(sc.events[i]));
	/* (the warnings pushed out of the line are still counted) */
	for (i = 0; i < sc.event_count && sc.events[i] != R_APP_LOW_MEMORY; i++) {}
	if (sc.low_memory > 0 && i == sc.event_count && used < sizeof(text))
		snprintf(text + used, sizeof(text) - used, " (low memory %dx before)", sc.low_memory);
	/* (while paused it is laid out, and seen when the app comes back) */
	Rlabel_set_text(sc.lifecycle, text);
	/* the timers that only move what is drawn stop in the background */
	if (event == R_APP_DID_PAUSE || event == R_APP_WILL_RESUME) set_tab_timers();
	/* (the user may have changed a permission in the Settings meanwhile) */
	if (event == R_APP_DID_RESUME) show_permissions();
}

/* ---------------------------------------------------------------- tabs */

static void on_progress_timer(Rid timer, void* user_data) {
	char text[16];
	(void)timer; (void)user_data;
	sc.progress_now += 0.01f;
	if (sc.progress_now > 1.0001f) sc.progress_now = 0.0f;
	Rprogress_bar_set_value(sc.progress, sc.progress_now);
	snprintf(text, sizeof(text), "%d %%", (int)(sc.progress_now * 100.0f + 0.5f));
	Rlabel_set_text(sc.progress_value, text);
}

static void on_stats_timer(Rid timer, void* user_data) {
	RFrameStats stats;
	char text[200];
	(void)timer; (void)user_data;
	Rwindow_get_frame_stats(sc.window, &stats);
	snprintf(text, sizeof(text), "last frame: layout %.2f ms, paint %.2f ms, render %.2f ms, %u draw calls, %u blurs",
		stats.layout_ms, stats.paint_ms, stats.render_ms, stats.draw_call_count, stats.backdrop_count);
	if (sc.frame_count > 0) {
		size_t used = strlen(text);
		snprintf(text + used, sizeof(text) - used, "; %d frames in 2 s, avg %.1f ms, max %.1f ms",
			sc.frame_count, sc.frame_sum / sc.frame_count * 1000.0, sc.frame_max * 1000.0);
	}
	sc.frame_count = 0; sc.frame_sum = 0; sc.frame_max = 0;
	Rlabel_set_text(sc.frame_stats, text);
	showcase_log("glass stats: %s", text);
}

/* The progress bar moves and the stats are taken only while they can be seen (else Raven stays idle). */
static void set_tab_timers(void) {
	int seen = !Ris_app_paused();
	if (seen && sc.tab == 1 && !sc.progress_timer) sc.progress_timer = Rtimer_create(1.0 / 30.0, R_TRUE, on_progress_timer, NULL);
	if ((!seen || sc.tab != 1) && sc.progress_timer) { Rtimer_destroy(sc.progress_timer); sc.progress_timer = 0; }
	if (seen && sc.tab == 2 && !sc.stats_timer) sc.stats_timer = Rtimer_create(2.0, R_TRUE, on_stats_timer, NULL);
	if ((!seen || sc.tab != 2) && sc.stats_timer) { Rtimer_destroy(sc.stats_timer); sc.stats_timer = 0; }
}

static void set_tab(int tab) {
	int i;
	sc.tab = tab;
	Rswitcher_set_index(sc.pages, tab);
	for (i = 0; i < TAB_COUNT; i++) Rbutton_set_style(sc.tabs[i], i == tab ? R_BUTTON_PRIMARY : R_BUTTON_FLAT);
	set_tab_timers();
	if (tab == 4) show_permissions();
	update_back();
	showcase_log("tab %d", tab);
}

static void on_tab(Rid button, void* user_data) {
	(void)button;
	set_tab((int)(intptr_t)user_data);
}

/* ---------------------------------------------------------------- controls */

static void on_counter(Rid button, void* user_data) {
	char text[64];
	(void)button; (void)user_data;
	sc.taps++;
	snprintf(text, sizeof(text), "Tapped %d time%s", sc.taps, sc.taps == 1 ? "" : "s");
	Rlabel_set_text(sc.counter_label, text);
	showcase_log("counter %d", sc.taps);
}

static void on_switch(Rid checkbox, int checked, void* user_data) {
	(void)user_data;
	showcase_log("%s %d", Rwidget_get_name(checkbox), checked);
}

static void on_slider(Rid slider, float value, void* user_data) {
	char text[16];
	(void)slider; (void)user_data;
	snprintf(text, sizeof(text), "%d", (int)(value + 0.5f));
	Rlabel_set_text(sc.slider_value, text);
}

static void on_dialog_button(Rid button, void* user_data) {
	Rdialog_close(button, (int)(intptr_t)user_data);
}

static void on_dialog_closed(Rid content, int result, void* user_data) {
	const char* what = result == 2 ? "OK" : result == 1 ? "Cancel" : "dismissed (Escape / back)";
	char text[64];
	(void)content; (void)user_data;
	snprintf(text, sizeof(text), "Dialog: %s", what);
	Rlabel_set_text(sc.dialog_result, text);
	Rtoast_show(sc.toast, text, 2.0);
	showcase_log("dialog closed: %d", result);
}

static const char g_dialog_xml[] =
	"<VBox Spacing=\"14\" MaxWidth=\"320\">"
	"  <Label Text=\"Delete 3 photos?\" TextSize=\"20\"/>"
	"  <Label Text=\"This is a modal dialog: the page behind it does not take taps until it closes.\" Wrap=\"true\" Opacity=\"0.75\"/>"
	"  <HBox Spacing=\"8\" Slot.HAlign=\"Right\">"
	"    <Button Name=\"cancel\" Text=\"Cancel\"/>"
	"    <Button Name=\"ok\" Text=\"OK\" Style=\"Primary\"/>"
	"  </HBox>"
	"</VBox>";

static void on_show_dialog(Rid button, void* user_data) {
	Rid content = Rwidget_create_from_xml(g_dialog_xml, sizeof(g_dialog_xml) - 1);
	(void)button; (void)user_data;
	if (content == R_INVALID_ID) { showcase_log("dialog XML failed"); return; }
	Rbutton_set_clicked_callback(Rwidget_find(content, "cancel"), on_dialog_button, (void*)(intptr_t)1);
	Rbutton_set_clicked_callback(Rwidget_find(content, "ok"), on_dialog_button, (void*)(intptr_t)2);
	Rdialog_open(sc.window, content, on_dialog_closed, NULL);
	showcase_log("dialog opened");
}

static void on_show_toast(Rid button, void* user_data) {
	static int count;
	char text[64];
	(void)button; (void)user_data;
	snprintf(text, sizeof(text), "Hello from Raven (toast %d)", ++count);
	Rtoast_show(sc.toast, text, 2.5);
}

/* ---------------------------------------------------------------- glass */

static void on_blur_slider(Rid slider, float value, void* user_data) {
	char text[16];
	int i;
	(void)slider; (void)user_data;
	for (i = 0; i < 4; i++) if (sc.blurs[i]) Rbackground_blur_set_radius(sc.blurs[i], value);
	snprintf(text, sizeof(text), "%d", (int)(value + 0.5f));
	Rlabel_set_text(sc.blur_value, text);
}

static void on_frame(double delta, void* user_data) {
	(void)user_data;
	if (Rtheme_is_dark() != sc.dark) {
		sc.dark = Rtheme_is_dark();
		showcase_log("theme switched (dark %d): control height %.0f, font size %.0f", sc.dark,
			Rwidget_get_metric(sc.tabs[0], R_METRIC_CONTROL_HEIGHT), Rtheme_get_metric(R_METRIC_FONT_SIZE));
	}
	if (sc.tab != 2 || delta <= 0.0 || delta > 1.0) return;
	sc.frame_sum += delta;
	if (delta > sc.frame_max) sc.frame_max = delta;
	sc.frame_count++;
}

/* ---------------------------------------------------------------- form */

static void show_form_state(const char* why) {
	char text[256];
	int x = 0, y = 0, w = 0, h = 0;
	Rid focused = Rwindow_get_focused_widget(sc.window);
	const char* name = focused == sc.name ? "name" : focused == sc.email ? "email" : focused == sc.phone ? "phone" :
		focused == sc.city ? "city" : focused == sc.note ? "note" :
		(focused && focused != R_INVALID_ID) ? "other" : "none";
	int has_rect = Rwindow_get_text_input_rect(sc.window, &x, &y, &w, &h);
	snprintf(text, sizeof(text), "%s: focus %s, wants text input %d, caret rect %s %d,%d %dx%d",
		why, name, Rwindow_wants_text_input(sc.window), has_rect ? "yes" : "no", x, y, w, h);
	Rlabel_set_text(sc.form_state, text);
	showcase_log("form: %s", text);
}

/* The caret rectangle is known once the field has been painted with the focus: ask again a moment later. */
static void on_form_state_timer(Rid timer, void* user_data) {
	(void)timer; (void)user_data;
	show_form_state("after paint");
}

static int on_input_event(Rid widget, const REvent* event, void* user_data) {
	(void)widget; (void)user_data;
	if (event->type == R_EVENT_FOCUS_GAINED) {
		show_form_state("focus gained");
		Rtimer_create(0.1, R_FALSE, on_form_state_timer, NULL);
	} else if (event->type == R_EVENT_FOCUS_LOST) show_form_state("focus lost");
	return R_FALSE;
}

static void on_text_changed(Rid input, const char* text, void* user_data) {
	(void)user_data;
	showcase_log("text of %s: \"%s\"", Rwidget_get_name(input), text);
}

static void on_submit(Rid button, void* user_data) {
	char text[256];
	const char* name = Rtext_input_get_text(sc.name);
	const char* email = Rtext_input_get_text(sc.email);
	const char* phone = Rtext_input_get_text(sc.phone);
	(void)button; (void)user_data;
	snprintf(text, sizeof(text), "Submitted: name \"%s\", email \"%s\", phone \"%s\"",
		name ? name : "", email ? email : "", phone ? phone : "");
	Rtoast_show(sc.toast, text, 3.0);
	showcase_log("%s", text);
	/* done with the form: no field keeps the focus, so the on-screen keyboard goes away */
	Rwindow_clear_focus(sc.window);
}

/* Enter (the return key of the on-screen keyboard): name -> email -> phone -> city -> note -> submit. */
static void on_committed(Rid input, const char* text, void* user_data) {
	(void)text; (void)user_data;
	showcase_log("committed in %s", Rwidget_get_name(input));
	if (input == sc.name) Rwidget_focus(sc.email);
	else if (input == sc.email) Rwidget_focus(sc.phone);
	else if (input == sc.phone) Rwidget_focus(sc.city);
	else if (input == sc.city) Rwidget_focus(sc.note);
	else on_submit(R_INVALID_ID, NULL);
}

/* The on-screen keyboard appeared, changed height or went away. */
static void on_keyboard(Rid window, int visible, float height, void* user_data) {
	char text[96];
	(void)window; (void)user_data;
	if (visible) snprintf(text, sizeof(text), "Keyboard: shown, %.0f units high", height);
	else snprintf(text, sizeof(text), "Keyboard: hidden");
	Rlabel_set_text(sc.keyboard_state, text);
	showcase_log("%s", text);
}

/* ---------------------------------------------------------------- device */

static Rid find(const char* name);

static void dev_status(const char* format, ...) {
	char text[300];
	va_list args;
	va_start(args, format);
	vsnprintf(text, sizeof(text), format, args);
	va_end(args);
	Rlabel_set_text(sc.dev_status, text);
	showcase_log("device: %s", text);
}

static const char* result_name(RDeviceResult result) {
	switch (result) {
	case R_DEVICE_DONE: return "done";
	case R_DEVICE_CANCELLED: return "cancelled";
	case R_DEVICE_DENIED: return "refused (permission)";
	case R_DEVICE_UNSUPPORTED: return "not supported here (yet)";
	case R_DEVICE_BUSY: return "busy: another screen of the system is open";
	case R_DEVICE_FAILED: return "failed (see the log)";
	default: return "?";
	}
}

static void show_permissions(void) {
	char text[128];
	if (!sc.perm_camera) return;
	snprintf(text, sizeof(text), "Camera: %s%s", Rpermission_status_to_string(Rpermission_get_status(R_PERMISSION_CAMERA)),
		Rcamera_is_available() ? "" : " (no camera here)");
	Rlabel_set_text(sc.perm_camera, text);
	snprintf(text, sizeof(text), "Notifications: %s", Rpermission_status_to_string(Rpermission_get_status(R_PERMISSION_NOTIFICATIONS)));
	Rlabel_set_text(sc.perm_notify, text);
	snprintf(text, sizeof(text), "Location: %s", Rpermission_status_to_string(Rpermission_get_status(R_PERMISSION_LOCATION)));
	Rlabel_set_text(sc.perm_location, text);
}

static void on_permission_answer(RPermission permission, RPermissionStatus status, void* user_data) {
	(void)user_data;
	dev_status("%s: %s", Rpermission_to_string(permission), Rpermission_status_to_string(status));
	show_permissions();
}

static void on_ask(Rid button, void* user_data) {
	RPermission permission = (RPermission)(intptr_t)user_data;
	(void)button;
	dev_status("asking for %s...", Rpermission_to_string(permission));
	Rpermission_request(permission, on_permission_answer, NULL);
}

/* Notifications the system could not show (refused, none here) come back as a toast. */
static void on_notification_fallback(const char* title, const char* body, RNotificationUrgency urgency, void* user_data) {
	char text[200];
	(void)urgency; (void)user_data;
	snprintf(text, sizeof(text), "%s: %s (shown here: the system did not)", title, body);
	Rtoast_show(sc.toast, text, 3.0);
}

static void on_notify(Rid button, void* user_data) {
	(void)button; (void)user_data;
	if (Rnotification_show("Raven Showcase", "Hello from the Device tab", R_NOTIFICATION_NORMAL)) dev_status("notification handed to the system");
	else dev_status("the system took no notification");
}

static void on_settings(Rid button, void* user_data) {
	(void)button; (void)user_data;
	if (!Rpermission_open_settings()) dev_status("Settings: no page of the app to open here");
}

static int is_image(const RPickedFile* file) {
	static const char* const extensions[] = { ".png", ".jpg", ".jpeg", ".gif", ".bmp", ".tga", ".svg", ".webp" };
	const char* name = file->name && *file->name ? file->name : file->path;
	size_t length = name ? strlen(name) : 0, i;
	if (file->mime_type && strncmp(file->mime_type, "image/", 6) == 0 && strcmp(file->mime_type, "image/heic") != 0) return 1;
	for (i = 0; name && i < sizeof(extensions) / sizeof(extensions[0]); i++) {
		size_t n = strlen(extensions[i]), k;
		int same = length > n;
		for (k = 0; same && k < n; k++) same = (name[length - n + k] | 0x20) == extensions[i][k];
		if (same) return 1;
	}
	return 0;
}

/* Shows a file Raven can read in an Image (decoded on a worker thread), replacing what it showed. */
static void show_image(Rid view, Rid* texture, const char* path) {
	if (*texture) Rtexture_destroy(*texture);
	*texture = path ? Rtexture_create_from_file_async(path) : 0;
	Rimage_set_texture(view, *texture ? *texture : R_INVALID_ID);
	Rwidget_set_visibility(view, *texture ? R_VISIBLE : R_COLLAPSED);
}

static void on_photo(RDeviceResult result, const RPickedFile* files, int count, void* user_data) {
	char text[300];
	if (result != R_DEVICE_DONE || count < 1 || !files[0].path) {
		dev_status("%s: %s", (const char*)user_data, result_name(result));
		return;
	}
	snprintf(text, sizeof(text), "%s: %s, %.1f KB%s", (const char*)user_data, files[0].name,
		(double)files[0].size / 1024.0, files[0].width > 0 ? "" : " (size in pixels not told)");
	if (files[0].width > 0) {
		size_t used = strlen(text);
		snprintf(text + used, sizeof(text) - used, ", %d x %d", files[0].width, files[0].height);
	}
	Rlabel_set_text(sc.photo_info, text);
	snprintf(sc.photo_path, sizeof(sc.photo_path), "%s", files[0].path);
	show_image(sc.photo_view, &sc.photo_texture, files[0].path);
	dev_status("%s: done", (const char*)user_data);
}

static void on_take_photo(Rid button, void* user_data) {
	(void)button; (void)user_data;
	dev_status("camera...");
	Rcamera_take_photo(on_photo, (void*)"camera");
}

static void on_pick_photo(Rid button, void* user_data) {
	(void)button; (void)user_data;
	dev_status("photo library...");
	Rphoto_pick(1, on_photo, (void*)"photo");
}

static void on_file(RDeviceResult result, const RPickedFile* files, int count, void* user_data) {
	char text[400];
	(void)user_data;
	if (result != R_DEVICE_DONE || count < 1) {
		dev_status("pick a file: %s", result_name(result));
		return;
	}
	snprintf(text, sizeof(text), "%s: %llu bytes%s%s", files[0].name, (unsigned long long)files[0].size,
		files[0].mime_type ? ", " : "", files[0].mime_type ? files[0].mime_type : "");
	if (!is_image(&files[0]) && files[0].path && files[0].size > 0 && files[0].size < 65536) {
		/* (a small text: its first line) */
		size_t size = 0, used = strlen(text), n;
		char* content = Rfile_read(files[0].path, &size);
		if (content) {
			n = strcspn(content, "\r\n");
			if (n > 60) n = 60;
			snprintf(text + used, sizeof(text) - used, "\nFirst line: %.*s", (int)n, content);
			Rmemory_free(content);
		}
	}
	Rlabel_set_text(sc.file_info, text);
	show_image(sc.file_view, &sc.file_texture, is_image(&files[0]) ? files[0].path : NULL);
	dev_status("pick a file: done");
}

static void on_open_file(Rid button, void* user_data) {
	(void)button; (void)user_data;
	dev_status("file picker...");
	Rfile_open_async(NULL, on_file, NULL);
}

static void on_saved(RDeviceResult result, const RPickedFile* files, int count, void* user_data) {
	(void)user_data;
	if (result == R_DEVICE_DONE && count > 0)
		dev_status("saved as %s%s%s", files[0].name, files[0].path ? " in " : "", files[0].path ? files[0].path : "");
	else
		dev_status("save: %s", result_name(result));
}

static void on_save_file(Rid button, void* user_data) {
	static const RFileFilter filter = { "Text", "*.txt" };
	RFileDialogDesc desc;
	char text[160];
	(void)button; (void)user_data;
	memset(&desc, 0, sizeof(desc));
	desc.file_name = "raven-showcase.txt";
	desc.filters = &filter;
	desc.filter_count = 1;
	desc.default_extension = "txt";
	snprintf(text, sizeof(text), "Saved by the Raven showcase, %.0f seconds after it started.\n", Rget_time());
	dev_status("save...");
	Rfile_save_async(&desc, text, strlen(text), on_saved, NULL);
}

static const struct { const char* name; const char* label; RHaptic haptic; } g_haptics[] = {
	{ "haptic_light", "light", R_HAPTIC_LIGHT }, { "haptic_medium", "medium", R_HAPTIC_MEDIUM },
	{ "haptic_heavy", "heavy", R_HAPTIC_HEAVY }, { "haptic_selection", "selection", R_HAPTIC_SELECTION },
	{ "haptic_success", "success", R_HAPTIC_SUCCESS }, { "haptic_warning", "warning", R_HAPTIC_WARNING },
	{ "haptic_error", "error", R_HAPTIC_ERROR }
};

static void on_haptic(Rid button, void* user_data) {
	int index = (int)(intptr_t)user_data;
	(void)button;
	if (Rhaptic_play(g_haptics[index].haptic)) dev_status("haptic: %s", g_haptics[index].label);
	else dev_status("haptic: none on this device (yet)");
}

static void on_open_url(Rid button, void* user_data) {
	(void)button; (void)user_data;
	dev_status(Ropen_url(RAVEN_PAGE_URL) ? "opening %s" : "could not open %s", RAVEN_PAGE_URL);
}

static void on_shared(RDeviceResult result, const RPickedFile* files, int count, void* user_data) {
	(void)files; (void)count; (void)user_data;
	dev_status("share: %s", result_name(result));
}

static void on_share(Rid button, void* user_data) {
	RShareDesc share;
	(void)user_data;
	memset(&share, 0, sizeof(share));
	share.text = "Raven, a GUI library in C: " RAVEN_PAGE_URL;
	/* (the last photo too, when there is one) */
	share.path = sc.photo_path[0] ? sc.photo_path : NULL;
	share.title = "Share from Raven";
	share.anchor = button;
	dev_status("share...");
	Rshare_show(&share, on_shared, NULL);
}

static void on_copy(Rid button, void* user_data) {
	char text[96];
	(void)button; (void)user_data;
	snprintf(text, sizeof(text), "Copied from the Raven showcase at %.1f s", Rget_time());
	Rclipboard_set_text(text);
	Rlabel_set_text(sc.clip_info, "Clipboard: copied. Paste it here, or in another app.");
	dev_status("copied");
}

/* A paste of the user: the moment to read the clipboard (iOS shows no question then... or asks once). */
static void on_paste(Rid button, void* user_data) {
	char text[200];
	(void)button; (void)user_data;
	if (!Rclipboard_has_text()) {
		Rlabel_set_text(sc.clip_info, "Clipboard: no text in it.");
		return;
	}
	snprintf(text, sizeof(text), "Clipboard: \"%.120s\"", Rclipboard_get_text());
	Rlabel_set_text(sc.clip_info, text);
	dev_status("pasted");
}

static void on_net_timer(Rid timer, void* user_data) {
	char text[200];
	int width = 0, height = 0;
	(void)user_data;
	if (!sc.net_texture || !Rtexture_is_ready(sc.net_texture)) return;
	Rtimer_destroy(timer);
	sc.net_timer = 0;
	Rtexture_get_size(sc.net_texture, &width, &height);
	if (width > 0) snprintf(text, sizeof(text), "Loaded %d x %d from %s", width, height, NET_IMAGE_URL);
	else snprintf(text, sizeof(text), "Could not be downloaded (no network, or not supported here yet: see the log).");
	Rlabel_set_text(sc.net_info, text);
	Rwidget_set_visibility(sc.net_view, width > 0 ? R_VISIBLE : R_COLLAPSED);
	dev_status("network image: %s", width > 0 ? "loaded" : "failed");
}

static void on_load_url(Rid button, void* user_data) {
	(void)button; (void)user_data;
	if (sc.net_texture) Rtexture_destroy(sc.net_texture);
	sc.net_texture = Rtexture_create_from_url_async(NET_IMAGE_URL);
	Rimage_set_texture(sc.net_view, sc.net_texture);
	Rlabel_set_text(sc.net_info, "Downloading...");
	if (!sc.net_timer) sc.net_timer = Rtimer_create(0.1, R_TRUE, on_net_timer, NULL);
}

static void device_create(void) {
	size_t i;
	sc.dev_status = find("dev_status");
	sc.perm_camera = find("perm_camera");
	sc.perm_notify = find("perm_notify");
	sc.perm_location = find("perm_location");
	sc.photo_info = find("photo_info");
	sc.photo_view = find("photo_view");
	sc.file_info = find("file_info");
	sc.file_view = find("file_view");
	sc.clip_info = find("clip_info");
	sc.net_info = find("net_info");
	sc.net_view = find("net_view");
	Rbutton_set_clicked_callback(find("ask_camera"), on_ask, (void*)(intptr_t)R_PERMISSION_CAMERA);
	Rbutton_set_clicked_callback(find("ask_notify"), on_ask, (void*)(intptr_t)R_PERMISSION_NOTIFICATIONS);
	Rbutton_set_clicked_callback(find("ask_location"), on_ask, (void*)(intptr_t)R_PERMISSION_LOCATION);
	Rbutton_set_clicked_callback(find("notify"), on_notify, NULL);
	Rbutton_set_clicked_callback(find("settings"), on_settings, NULL);
	Rnotification_set_fallback(on_notification_fallback, NULL);
	Rbutton_set_clicked_callback(find("take_photo"), on_take_photo, NULL);
	Rbutton_set_clicked_callback(find("pick_photo"), on_pick_photo, NULL);
	Rbutton_set_clicked_callback(find("open_file"), on_open_file, NULL);
	Rbutton_set_clicked_callback(find("save_file"), on_save_file, NULL);
	for (i = 0; i < sizeof(g_haptics) / sizeof(g_haptics[0]); i++)
		Rbutton_set_clicked_callback(find(g_haptics[i].name), on_haptic, (void*)(intptr_t)i);
	Rbutton_set_clicked_callback(find("open_url"), on_open_url, NULL);
	Rbutton_set_clicked_callback(find("share"), on_share, NULL);
	Rbutton_set_clicked_callback(find("copy"), on_copy, NULL);
	Rbutton_set_clicked_callback(find("paste"), on_paste, NULL);
	Rbutton_set_clicked_callback(find("load_url"), on_load_url, NULL);
	show_permissions();
}

/* ---------------------------------------------------------------- create */

static Rid find(const char* name) {
	Rid id = Rwidget_find(sc.root, name);
	if (id == R_INVALID_ID) showcase_log("widget not found: %s", name);
	return id;
}

Rid showcase_create(void) {
	char name[8];
	int i;

	/* (Android can run android_main again in the same process: start from zero) */
	memset(&sc, 0, sizeof(sc));

	/*
	 * Touch sizes come from Raven on iOS and Android (RTouchSettings::min_touch_size:
	 * controls and rows 44 / 48 high). Only the text is made larger here; it
	 * stays through the light / dark switches of Rtheme_follow_system.
	 */
	Rtheme_set_metric(R_METRIC_FONT_SIZE, 16.0f);
	sc.dark = Rtheme_is_dark();

	/* the size is ignored on phones: the window takes the screen */
	sc.window = Rwindow_create("Raven Showcase", 400, 800, R_WINDOW_DEFAULT);
	if (sc.window == R_INVALID_ID) return R_INVALID_ID;
	/*
	 * a file of the app's package (shared/assets/: the APK's assets/, the iOS
	 * bundle), as are the picture and the font it names
	 */
	sc.root = Rwidget_create_from_xml_file("showcase.xml");
	if (sc.root == R_INVALID_ID) {
		showcase_log("showcase.xml could not be read");
		return R_INVALID_ID;
	}
	Rwindow_set_content(sc.window, sc.root);

	sc.pages = find("pages");
	sc.listnav = find("listnav");
	sc.list = find("list");
	sc.list_page = Rwidget_get_parent(sc.list);
	sc.detail = find("detail");
	sc.detail_title = find("detail_title");
	sc.detail_subtitle = find("detail_subtitle");
	sc.detail_body = find("detail_body");
	sc.detail_swatch = find("detail_swatch");
	sc.counter_label = find("counter_label");
	sc.slider_value = find("slider_value");
	sc.progress = find("progress");
	sc.progress_value = find("progress_value");
	sc.dialog_result = find("dialog_result");
	sc.blur_value = find("blur_value");
	sc.frame_stats = find("frame_stats");
	sc.blurs[0] = find("blur_title");
	sc.blurs[1] = find("blur_plain");
	sc.blurs[2] = find("blur_glass");
	sc.name = find("name");
	sc.email = find("email");
	sc.phone = find("phone");
	sc.city = find("city");
	sc.note = find("note");
	sc.form_state = find("form_state");
	sc.keyboard_state = find("keyboard_state");
	sc.toast = find("toast");
	sc.lifecycle = find("lifecycle");

	/* list: 1,000 rows, virtual (only the rows in view are asked for) */
	sc.circle = make_circle();
	Rtile_view_set_items(sc.list, ROW_COUNT, on_tile_item, NULL);
	Rtile_view_set_selection_changed_callback(sc.list, on_list_selection, NULL);
	Rbutton_set_clicked_callback(find("back"), on_back, NULL);
	Rwidget_set_touch_callback(sc.detail, on_detail_touch, NULL);

	/* tab bar */
	for (i = 0; i < TAB_COUNT; i++) {
		snprintf(name, sizeof(name), "tab%d", i);
		sc.tabs[i] = find(name);
		Rbutton_set_clicked_callback(sc.tabs[i], on_tab, (void*)(intptr_t)i);
	}

	/* controls */
	Rbutton_set_clicked_callback(find("counter"), on_counter, NULL);
	Rcheckbox_set_changed_callback(find("switch"), on_switch, NULL);
	Rcheckbox_set_changed_callback(find("check"), on_switch, NULL);
	Rslider_set_changed_callback(find("slider"), on_slider, NULL);
	Rbutton_set_clicked_callback(find("show_dialog"), on_show_dialog, NULL);
	Rbutton_set_clicked_callback(find("show_toast"), on_show_toast, NULL);

	/* glass */
	Rslider_set_changed_callback(find("blur_slider"), on_blur_slider, NULL);
	Rset_frame_callback(on_frame, NULL);

	/* form */
	Rwidget_set_event_callback(sc.name, on_input_event, NULL);
	Rwidget_set_event_callback(sc.email, on_input_event, NULL);
	Rwidget_set_event_callback(sc.phone, on_input_event, NULL);
	Rwidget_set_event_callback(sc.city, on_input_event, NULL);
	Rwidget_set_event_callback(sc.note, on_input_event, NULL);
	Rtext_input_set_changed_callback(sc.name, on_text_changed, NULL);
	Rtext_input_set_changed_callback(sc.email, on_text_changed, NULL);
	Rtext_input_set_changed_callback(sc.phone, on_text_changed, NULL);
	Rtext_input_set_changed_callback(sc.city, on_text_changed, NULL);
	Rtext_input_set_changed_callback(sc.note, on_text_changed, NULL);
	Rbutton_set_clicked_callback(find("submit"), on_submit, NULL);
	Rtext_input_set_committed_callback(sc.name, on_committed, NULL);
	Rtext_input_set_committed_callback(sc.email, on_committed, NULL);
	Rtext_input_set_committed_callback(sc.phone, on_committed, NULL);
	Rtext_input_set_committed_callback(sc.city, on_committed, NULL);
	Rtext_input_set_committed_callback(sc.note, on_committed, NULL);
	Rwindow_set_keyboard_callback(sc.window, on_keyboard, NULL);

	/* device */
	device_create();

	/* back: the system's (Android), and Escape */
	Rwindow_set_back_callback(sc.window, on_system_back, NULL);
	Rwindow_set_event_hook(sc.window, on_window_event, NULL);
	Rset_app_event_callback(on_app_event, NULL);

	set_tab(0);
	showcase_log("created: window %llu, system dark %d", (unsigned long long)sc.window, Rtheme_get_system_dark());
	return sc.window;
}

void showcase_report(const char* why) {
	int width = 0, height = 0;
	RInsets safe;
	Rwindow_get_size(sc.window, &width, &height);
	Rwindow_get_safe_area(sc.window, &safe);
	showcase_log("%s: window %dx%d, safe area top %.0f right %.0f bottom %.0f left %.0f, dark %d (system %d)",
		why, width, height, safe.top, safe.right, safe.bottom, safe.left, Rtheme_is_dark(), Rtheme_get_system_dark());
}
