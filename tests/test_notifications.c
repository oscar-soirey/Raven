/**
 * Raven tests - notifications with tags, buttons and an icon
 * (Rnotification_show_desc), through the fallback toast of the application:
 * the null platform has no notifications of its own, so every one goes to
 * the toast, which reports to the same callback as the system would.
 *
 * Also, with the static library, what the platforms report
 * (rvn_notification_event) and what they share: the actions of
 * org.freedesktop.Notifications and the file:// URI of an icon.
 */

#include "test.h"

#include <stdio.h>
#include <stdlib.h>

#if !defined(RAVEN_TEST_SHARED)
#include "../src/platform/platform.h"
#endif

static Rid g_window;
static Rid g_toast;

static int g_events;
static char g_tag[64];
static RNotificationEvent g_event;
static char g_action[64];
static int g_clicks;
static int g_fallbacks;

static void on_event(const char* tag, RNotificationEvent event, const char* action, void* user_data) {
	CHECK(user_data == &g_events);
	g_events++;
	snprintf(g_tag, sizeof(g_tag), "%s", tag ? tag : "(null)");
	g_event = event;
	snprintf(g_action, sizeof(g_action), "%s", action ? action : "(null)");
}

static void on_clicked(void* user_data) {
	(void)user_data;
	g_clicks++;
}

static void on_fallback(const char* title, const char* body, RNotificationUrgency urgency, void* user_data) {
	(void)title;
	(void)body;
	(void)urgency;
	(void)user_data;
	g_fallbacks++;
}

static void reset_events(void) {
	g_events = 0;
	g_clicks = 0;
	g_tag[0] = 0;
	g_action[0] = 0;
	g_event = (RNotificationEvent)0;
}

static const RNotificationAction g_actions[] = { { "reply", "Reply" }, { "later", "Later" } };

static int show(const char* tag, const char* title, const char* body, int action_count, Rid window) {
	RNotificationDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.size = (Ruint)sizeof(desc);
	desc.tag = tag;
	desc.title = title;
	desc.body = body;
	desc.actions = action_count > 0 ? g_actions : NULL;
	desc.action_count = action_count;
	desc.window = window;
	return Rnotification_show_desc(&desc);
}

/* The last button of the newest notification (at the bottom right of the toast). */
static void click_last_button(void) {
	float x, y, w, h;
	float padding = Rtheme_get_metric(R_METRIC_PADDING_X) + 4.0f, button = Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT);
	Rdraw_frame();
	Rwidget_get_rect(g_toast, &x, &y, &w, &h);
	test_click_at(g_window, x + w - padding - 6.0f, y + h - 10.0f - button * 0.5f);
	Rdraw_frame();
}

/* The body of the newest notification: its top left part, away from the buttons. */
static void click_body(void) {
	float x, y, w, h;
	Rdraw_frame();
	Rwidget_get_rect(g_toast, &x, &y, &w, &h);
	test_click_at(g_window, x + 40.0f, y + h - 8.0f - Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT) - 20.0f);
	Rdraw_frame();
}

static void test_without_system(void) {
	/* the null platform: no notifications of the system, so no feature of theirs */
	CHECK(!Rnotification_is_supported());
	CHECK(!Rnotification_supports(R_NOTIFICATION_FEATURE_ACTIONS));
	CHECK(!Rnotification_supports(R_NOTIFICATION_FEATURE_REPLACE));
	CHECK(!Rnotification_supports(R_NOTIFICATION_FEATURE_DISMISSED));
	/* without a fallback they go nowhere, quietly */
	CHECK(!show("a", "Title", "Body", 2, R_INVALID_ID));
}

static void test_invalid_descriptions(void) {
	RNotificationDesc desc;
	RNotificationAction actions[4] = { { "a", "A" }, { "b", "B" }, { "c", "C" }, { "d", "D" } };
	memset(&desc, 0, sizeof(desc));
	desc.title = "Title";
	EXPECT_ERRORS_BEGIN();
	/* the size must be given */
	CHECK(!Rnotification_show_desc(&desc));
	CHECK(!Rnotification_show_desc(NULL));
	desc.size = (Ruint)sizeof(desc);
	/* at most 3 buttons */
	desc.actions = actions;
	desc.action_count = 4;
	CHECK(!Rnotification_show_desc(&desc));
	desc.action_count = -1;
	CHECK(!Rnotification_show_desc(&desc));
	/* "default" is the click on the notification itself */
	actions[0].id = "default";
	desc.action_count = 1;
	CHECK(!Rnotification_show_desc(&desc));
	/* two buttons of the same id, a button without a label */
	actions[0].id = "b";
	desc.action_count = 2;
	CHECK(!Rnotification_show_desc(&desc));
	actions[0].id = "a";
	actions[0].label = "";
	CHECK(!Rnotification_show_desc(&desc));
	/* nothing to show */
	desc.title = NULL;
	desc.action_count = 0;
	CHECK(!Rnotification_show_desc(&desc));
	Rnotification_close(NULL);
	Rnotification_close("");
	Rnotification_set_fallback_toast(g_window);
	EXPECT_ERRORS_END(10);
	CHECK(Rtoast_get_count(g_toast) == 0);
}

static void test_fallback_toast(void) {
	reset_events();
	g_fallbacks = 0;
	Rnotification_set_fallback(on_fallback, NULL);
	Rnotification_set_fallback_toast(g_toast);
	Rnotification_set_callback(on_event, &g_events);
	/* not shown by the system: in the toast, which takes the place of the fallback callback */
	CHECK(!show("chat-42", "Ada", "Are we still on for tonight?", 2, R_INVALID_ID));
	Rdraw_frame();
	CHECK(Rtoast_is_shown(g_toast));
	CHECK(Rtoast_get_count(g_toast) == 1);
	CHECK_STR(Rtoast_get_text(g_toast), "Are we still on for tonight?");
	CHECK(g_fallbacks == 0);
	CHECK(g_events == 0);
	/* the same tag: replaced where it is */
	CHECK(!show("chat-42", "Ada", "Running late", 2, R_INVALID_ID));
	Rdraw_frame();
	CHECK(Rtoast_get_count(g_toast) == 1);
	CHECK_STR(Rtoast_get_text(g_toast), "Running late");
	CHECK(g_events == 0);
	/* another tag: one more (the toast stacks them) */
	CHECK(!show("build", "Build", "Finished in 42 s", 0, R_INVALID_ID));
	Rdraw_frame();
	CHECK(Rtoast_get_count(g_toast) == 2);
	/* closed by the application: gone, nothing reported */
	Rnotification_close("build");
	Rdraw_frame();
	CHECK(Rtoast_get_count(g_toast) == 1);
	CHECK(g_events == 0);
	Rnotification_close("unknown");
	CHECK(Rtoast_get_count(g_toast) == 1);

	/* a button: its id, then it goes */
	click_last_button();
	CHECK(g_events == 1);
	CHECK_STR(g_tag, "chat-42");
	CHECK(g_event == R_NOTIFICATION_ACTION_INVOKED);
	CHECK_STR(g_action, "later");
	CHECK(Rtoast_get_count(g_toast) == 0);
	CHECK(g_clicks == 0);

	/* a notification without a tag gets one of its own, a new one each time */
	reset_events();
	CHECK(!show(NULL, "One", "First", 0, R_INVALID_ID));
	CHECK(!show(NULL, "Two", "Second", 0, R_INVALID_ID));
	CHECK(Rtoast_get_count(g_toast) == 2);
	/* the toast hides the newest one: dismissed */
	Rtoast_hide(g_toast);
	CHECK(g_events == 1 && g_event == R_NOTIFICATION_DISMISSED);
	CHECK(strncmp(g_tag, "raven.", 6) == 0);
	CHECK_STR(g_action, "(null)");
	Rtoast_clear(g_toast);
	CHECK(g_events == 2 && g_event == R_NOTIFICATION_DISMISSED);
	Rdraw_frame();

	/* the old Rnotification_show goes there too */
	reset_events();
	CHECK(!Rnotification_show("Done", "The export is finished", R_NOTIFICATION_LOW));
	CHECK(Rtoast_get_count(g_toast) == 1);
	CHECK_STR(Rtoast_get_text(g_toast), "The export is finished");
	/* a message of the application in the Replace mode takes its place: dismissed */
	Rtoast_set_queue(g_toast, R_TOAST_REPLACE);
	Rtoast_show(g_toast, "Saved", 0.0);
	CHECK(g_events == 1 && g_event == R_NOTIFICATION_DISMISSED);
	CHECK(Rtoast_get_count(g_toast) == 1);
	Rtoast_clear(g_toast);
	/* (a message of Rtoast_show reports nothing) */
	CHECK(g_events == 1);
	Rtoast_set_queue(g_toast, R_TOAST_STACK);
	Rdraw_frame();

	/* without the toast, the fallback callback gets them again (no buttons there) */
	Rnotification_set_fallback_toast(R_INVALID_ID);
	CHECK(!show("x", "Title", "Body", 2, R_INVALID_ID));
	CHECK(g_fallbacks == 1);
	CHECK(Rtoast_get_count(g_toast) == 0);
	Rnotification_set_fallback(NULL, NULL);
	Rnotification_set_fallback_toast(g_toast);
}

/* A click on a notification brings its window back: out of the Dock / taskbar, in front, with the keyboard. */
static void test_click_brings_window_back(void) {
	Rid other = Rwindow_create("other", 300, 200, R_WINDOW_DEFAULT);
	reset_events();
	Rnotification_set_clicked_callback(on_clicked, NULL);
	/* (the toast is in the main window: the notification names the other one) */
	Rwindow_minimize(other);
	Rdraw_frame();
	CHECK(Rwindow_is_minimized(other));
	CHECK(!show("mail", "Mail", "3 new messages", 2, other));
	click_body();
	CHECK(g_events == 1);
	CHECK(g_event == R_NOTIFICATION_ACTIVATED);
	CHECK_STR(g_tag, "mail");
	CHECK_STR(g_action, "(null)");
	CHECK(g_clicks == 1);
	CHECK(Rtoast_get_count(g_toast) == 0);
	CHECK(!Rwindow_is_minimized(other));
	CHECK(Rwindow_is_active(other));

	/* a button does not bring it back (the application decides) */
	Rwindow_minimize(other);
	Rdraw_frame();
	CHECK(!show("other", "Other", "Again", 2, other));
	click_last_button();
	CHECK(g_events == 2 && g_event == R_NOTIFICATION_ACTION_INVOKED);
	CHECK(Rwindow_is_minimized(other));
	CHECK(g_clicks == 1);

	/* turned off: the application does it */
	Rnotification_set_bring_to_front(R_FALSE);
	CHECK(!show("other", "Other", "And again", 0, other));
	click_body();
	CHECK(g_events == 3 && g_event == R_NOTIFICATION_ACTIVATED);
	CHECK(g_clicks == 2);
	CHECK(Rwindow_is_minimized(other));
	Rnotification_set_bring_to_front(R_TRUE);

	/* a hidden window (an application that hides it in the tray) is shown again */
	Rwindow_restore(other);
	Rwindow_hide(other);
	Rdraw_frame();
	CHECK(!show("tray", "Tray", "Back from the tray", 0, other));
	click_body();
	CHECK(g_events == 4 && g_event == R_NOTIFICATION_ACTIVATED);
	CHECK(Rwindow_is_active(other));
	Rnotification_set_clicked_callback(NULL, NULL);
	Rwindow_destroy(other);
	Rdraw_frame();
}

static void test_missing_icon(void) {
	RNotificationDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.size = (Ruint)sizeof(desc);
	desc.tag = "icon";
	desc.title = "Icon";
	desc.icon = "no-such-icon-for-the-test.png";
	/* reported, and shown without it */
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rnotification_show_desc(&desc));
	EXPECT_ERRORS_END(1);
	CHECK(Rtoast_get_count(g_toast) == 1);
	CHECK_STR(Rtoast_get_text(g_toast), "Icon");
	Rdraw_frame();
	Rnotification_close("icon");
	CHECK(Rtoast_get_count(g_toast) == 0);
}

/* A larger description (from a later Raven): the fields this one knows are read. */
static void test_larger_size(void) {
	struct {
		RNotificationDesc desc;
		int later_field;
	} bigger;
	memset(&bigger, 0, sizeof(bigger));
	bigger.desc.size = (Ruint)sizeof(bigger);
	bigger.desc.tag = "bigger";
	bigger.desc.body = "From a later version";
	bigger.later_field = 7;
	reset_events();
	CHECK(!Rnotification_show_desc(&bigger.desc));
	CHECK(Rtoast_get_count(g_toast) == 1);
	Rnotification_close("bigger");
	CHECK(g_events == 0);
}

#if !defined(RAVEN_TEST_SHARED)
/* What a platform reports: by tag, or the last one shown (a system that shows one at a time). */
static void test_platform_events(void) {
	reset_events();
	Rtoast_clear(g_toast);
	Rnotification_set_fallback_toast(R_INVALID_ID);
	Rwindow_minimize(g_window);
	Rdraw_frame();
	CHECK(!show("sys", "System", "Shown by the system", 2, R_INVALID_ID));
	rvn_notification_event("sys", R_NOTIFICATION_ACTION_INVOKED, "reply");
	CHECK(g_events == 1 && g_event == R_NOTIFICATION_ACTION_INVOKED);
	CHECK_STR(g_action, "reply");
	CHECK(Rwindow_is_minimized(g_window));
	CHECK(!show("sys", "System", "Again", 0, R_INVALID_ID));
	rvn_notification_event(NULL, R_NOTIFICATION_ACTIVATED, NULL);
	CHECK(g_events == 2 && g_event == R_NOTIFICATION_ACTIVATED);
	CHECK_STR(g_tag, "sys");
	/* (no window named: the main one, the first opened) */
	CHECK(!Rwindow_is_minimized(g_window));
	CHECK(Rwindow_is_active(g_window));
	/* an unknown tag still reaches the application (the window: the main one) */
	rvn_notification_event("earlier", R_NOTIFICATION_DISMISSED, NULL);
	CHECK(g_events == 3 && g_event == R_NOTIFICATION_DISMISSED);
	CHECK_STR(g_tag, "earlier");
	/* an unknown event goes nowhere */
	rvn_notification_event("sys", 12345, NULL);
	CHECK(g_events == 3);
	Rnotification_set_fallback_toast(g_toast);
}

/* The actions array of org.freedesktop.Notifications.Notify: "default" first, then id and label of each button. */
static void test_dbus_actions(void) {
	const char* ids[] = { "reply", "later" };
	const char* labels[] = { "Reply", "Later" };
	const char* out[8];
	rvn_notification_t notification;
	memset(&notification, 0, sizeof(notification));
	notification.action_ids = ids;
	notification.action_labels = labels;
	notification.action_count = 2;
	CHECK(rvn_notification_dbus_actions(&notification, out, 8) == 6);
	CHECK_STR(out[0], "default");
	CHECK_STR(out[1], "");
	CHECK_STR(out[2], "reply");
	CHECK_STR(out[3], "Reply");
	CHECK_STR(out[4], "later");
	CHECK_STR(out[5], "Later");
	/* only whole pairs fit */
	CHECK(rvn_notification_dbus_actions(&notification, out, 5) == 4);
	CHECK(rvn_notification_dbus_actions(&notification, out, 1) == 0);
	notification.action_count = 0;
	CHECK(rvn_notification_dbus_actions(&notification, out, 8) == 2);
}

static void test_file_uri(void) {
	char* uri = rvn_notification_file_uri("/home/ada/My Pictures/caf\xC3\xA9 #1.png");
	CHECK_STR(uri, "file:///home/ada/My%20Pictures/caf%C3%A9%20%231.png");
	Rmemory_free(uri);
	uri = rvn_notification_file_uri("/a-b_c.d~e/f");
	CHECK_STR(uri, "file:///a-b_c.d~e/f");
	Rmemory_free(uri);
}
#endif

int main(void) {
	static const char xml[] =
		"<Overlay><Label Text=\"Content\"/>"
		"<Toast Name=\"toast\" Queue=\"Stack\" Duration=\"0\" Slot.HAlign=\"Right\" Slot.VAlign=\"Bottom\" Slot.Padding=\"12\"/></Overlay>";
	Rid root;
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	g_window = Rwindow_create("notifications", 640, 480, R_WINDOW_DEFAULT);
	root = Rwidget_create_from_xml(xml, strlen(xml));
	Rwindow_set_content(g_window, root);
	g_toast = Rwidget_find(root, "toast");
	Rdraw_frame();

	RUN(test_without_system);
	RUN(test_invalid_descriptions);
	RUN(test_fallback_toast);
	RUN(test_click_brings_window_back);
	RUN(test_missing_icon);
	RUN(test_larger_size);
#if !defined(RAVEN_TEST_SHARED)
	RUN(test_platform_events);
	RUN(test_dbus_actions);
	RUN(test_file_uri);
#endif

	Rnotification_set_callback(NULL, NULL);
	Rnotification_set_fallback_toast(R_INVALID_ID);
	Rdraw_frame();
	Rshutdown();
	return test_report("test_notifications");
}
