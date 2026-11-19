/**
 * Raven example - notifications
 * Notifications of the system with a tag, buttons and an icon
 * (Rnotification_show_desc), and what the user does with them
 * (Rnotification_set_callback): a click on the notification, which also
 * brings this window back, a click on one of its buttons, or its dismissal.
 *
 * Where the system shows none (Linux without a notification server, macOS
 * outside of an application bundle, the null platform), they are shown in the
 * Toast at the bottom right of the window instead, with the same buttons and
 * the same callback (Rnotification_set_fallback_toast).
 *
 * The automated runs (RAVEN_EXAMPLE_FRAMES in common/example.h) show two of
 * them when the window opens.
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static struct {
	Rid window;
	Rid log;
	char text[1024];
} app;

static const RNotificationAction g_message_actions[] = { { "reply", "Reply" }, { "later", "Remind me" } };
static const RNotificationAction g_build_actions[] = { { "open", "Open folder" } };

static int notify(const char* tag, const char* title, const char* body, RNotificationUrgency urgency,
	const RNotificationAction* actions, int action_count, const char* icon) {
	RNotificationDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.size = (Ruint)sizeof(desc);
	desc.tag = tag;
	desc.title = title;
	desc.body = body;
	desc.urgency = urgency;
	desc.actions = actions;
	desc.action_count = action_count;
	desc.icon = icon;
	desc.window = app.window;
	return Rnotification_show_desc(&desc);
}

/* The newest line first, the older ones below. */
static void log_line(const char* line) {
	char text[sizeof(app.text)];
	snprintf(text, sizeof(text), app.text[0] ? "%s\n%s" : "%s%s", line, app.text);
	snprintf(app.text, sizeof(app.text), "%s", text);
	Rlabel_set_text(app.log, app.text);
}

static void on_notification(const char* tag, RNotificationEvent event, const char* action, void* user_data) {
	char line[256];
	(void)user_data;
	switch (event) {
		case R_NOTIFICATION_ACTIVATED:
			snprintf(line, sizeof(line), "%s: clicked", tag);
			break;
		case R_NOTIFICATION_ACTION_INVOKED:
			snprintf(line, sizeof(line), "%s: button \"%s\"", tag, action);
			break;
		default:
			snprintf(line, sizeof(line), "%s: dismissed", tag);
			break;
	}
	log_line(line);
}

static void on_action(Rid sender, const char* action, void* user_data) {
	(void)user_data;
	if (strcmp(action, "notify.message") == 0) {
		notify("chat-ada", "Ada Lovelace", "Are we still on for tonight?", R_NOTIFICATION_NORMAL, g_message_actions, 2,
			RAVEN_EXAMPLE_DIR "/chat.svg");
	} else if (strcmp(action, "notify.replace") == 0) {
		/* the same tag: the notification shown is replaced, not added to */
		notify("chat-ada", "Ada Lovelace", "Running a little late: 8 pm?", R_NOTIFICATION_NORMAL, g_message_actions, 2,
			RAVEN_EXAMPLE_DIR "/chat.svg");
	} else if (strcmp(action, "notify.close") == 0) {
		Rnotification_close("chat-ada");
	} else if (strcmp(action, "notify.build") == 0) {
		notify("build", "Build finished", "raven_notifications built in 4.2 s", R_NOTIFICATION_LOW, g_build_actions, 1, NULL);
	} else if (strcmp(action, "notify.critical") == 0) {
		notify("disk", "Disk almost full", "Less than 1 GB is left on the system disk", R_NOTIFICATION_CRITICAL, NULL, 0, NULL);
	} else if (strcmp(action, "notify.front") == 0) {
		Rnotification_set_bring_to_front(Rcheckbox_is_checked(sender));
	}
}

int main(void) {
	char where[256];
	if (!example_init("Raven notifications")) return 1;
	Rtheme_apply(R_THEME_DARK);
	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/notifications.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.log = Rwidget_find(app.window, "log");
	if (Rnotification_is_supported()) {
		snprintf(where, sizeof(where), "Shown by the system (buttons: %s, dismissals reported: %s); by the toast below when it cannot",
			Rnotification_supports(R_NOTIFICATION_FEATURE_ACTIONS) ? "yes" : "no",
			Rnotification_supports(R_NOTIFICATION_FEATURE_DISMISSED) ? "yes" : "no");
	} else {
		snprintf(where, sizeof(where), "The system shows none here: they are shown in the toast below, with their buttons");
	}
	Rlabel_set_text(Rwidget_find(app.window, "where"), where);

	Rnotification_set_fallback_toast(Rwidget_find(app.window, "toast"));
	Rnotification_set_callback(on_notification, NULL);
	Raction_bind("notify.message", on_action, NULL);
	Raction_bind("notify.replace", on_action, NULL);
	Raction_bind("notify.close", on_action, NULL);
	Raction_bind("notify.build", on_action, NULL);
	Raction_bind("notify.critical", on_action, NULL);
	Raction_bind("notify.front", on_action, NULL);
	if (getenv("RAVEN_EXAMPLE_FRAMES")) {
		notify("build", "Build finished", "raven_notifications built in 4.2 s", R_NOTIFICATION_LOW, g_build_actions, 1, NULL);
		notify("chat-ada", "Ada Lovelace", "Are we still on for tonight?", R_NOTIFICATION_NORMAL, g_message_actions, 2,
			RAVEN_EXAMPLE_DIR "/chat.svg");
	}

	example_run(app.window, NULL, NULL);
	return 0;
}
