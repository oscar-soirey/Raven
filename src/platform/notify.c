/**
 * Raven - platform/notify.c
 * The notifications of the system (Rnotification_show): the platform shows
 * them when it can (rvn_platform_t::notify); otherwise the fallback of the
 * application is called, which may show them in a window (a Toast).
 *
 * Also the notifications of Linux and the BSDs, for the X11 and Wayland
 * platforms: org.freedesktop.Notifications over the session bus. As for
 * the screen readers (atspi.c), libdbus is loaded when it is first needed:
 * Raven links with nothing more.
 */

#if !defined(_WIN32) && !defined(__APPLE__)
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#endif

#include "platform.h"
#include "../core/context.h"
#include "../core/fs.h"
#include "../widget/widget.h"

#define RVN_NOTIFY_MAX_ACTIONS 3
/* The notifications whose window is remembered (the newest ones). */
#define RVN_NOTIFY_RECENT 32

typedef struct rvn_notify_recent_t {
	char* tag;
	Rid window;
} rvn_notify_recent_t;

static struct {
	RNotificationFallback fallback;
	void* fallback_user;
	RNotificationClickedCallback clicked;
	void* clicked_user;
	RNotificationCallback callback;
	void* callback_user;
	/* (zero, the default: a click brings the window back) */
	int stay_behind;
	Rid toast;
	unsigned serial;
	/* the newest last */
	rvn_notify_recent_t recent[RVN_NOTIFY_RECENT];
	int recent_count;
} g_notify;

static void rvn_notify_forget(int index) {
	rvn_free(g_notify.recent[index].tag);
	memmove(&g_notify.recent[index], &g_notify.recent[index + 1], (size_t)(g_notify.recent_count - index - 1) * sizeof(rvn_notify_recent_t));
	g_notify.recent_count--;
}

static int rvn_notify_find(const char* tag) {
	int i;
	for (i = g_notify.recent_count - 1; i >= 0; i--) {
		if (strcmp(g_notify.recent[i].tag, tag) == 0) return i;
	}
	return -1;
}

/* Remembers which window a notification brings back (the newest at the end). */
static void rvn_notify_remember(const char* tag, Rid window) {
	int index = rvn_notify_find(tag);
	if (index >= 0) rvn_notify_forget(index);
	if (g_notify.recent_count == RVN_NOTIFY_RECENT) rvn_notify_forget(0);
	g_notify.recent[g_notify.recent_count].tag = rvn_strdup(tag);
	g_notify.recent[g_notify.recent_count].window = window;
	g_notify.recent_count++;
}

void rvn_notify_shutdown(void) {
	while (g_notify.recent_count > 0) rvn_notify_forget(g_notify.recent_count - 1);
	g_notify.toast = R_INVALID_ID;
}

/* The window a click brings back: the one given, else the first window opened that is still there. */
static Rid rvn_notify_window(Rid window) {
	uint32_t i;
	if (window != R_INVALID_ID && rvn_window_get(window)) return window;
	for (i = 0; i < g_rvn.windows.used; i++) {
		rvn_window_t* candidate = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (!candidate || candidate->dead || candidate->owner != R_INVALID_ID || candidate->notch_owner) continue;
		return candidate->id;
	}
	return R_INVALID_ID;
}

static void rvn_notify_bring_back(Rid id) {
	rvn_window_t* window = rvn_window_get(rvn_notify_window(id));
	if (!window) return;
	/* (where the system has a focus request, it also takes the window out of the Dock or the taskbar) */
	if (window->state == RVN_PWINDOW_MINIMIZED && !g_rvn.platform->window_focus) Rwindow_restore(window->id);
	Rwindow_focus(window->id);
}

void rvn_notification_event(const char* tag, int event, const char* action) {
	int index;
	Rid window = R_INVALID_ID;
	/* (on macOS it comes from the main queue, which may run after Rshutdown) */
	if (!g_rvn.initialized) return;
	if (event < R_NOTIFICATION_ACTIVATED || event > R_NOTIFICATION_DISMISSED) return;
	if (!tag) {
		/* a system that shows one at a time: the last one shown */
		if (g_notify.recent_count == 0) return;
		tag = g_notify.recent[g_notify.recent_count - 1].tag;
	}
	{
		/* (the callbacks may show or close notifications, the same tag again too: the tag is kept aside,
		 * and the notification forgotten first: it is gone, a click takes it away too) */
		char* name = rvn_strdup(tag);
		index = rvn_notify_find(name);
		if (index >= 0) {
			window = g_notify.recent[index].window;
			rvn_notify_forget(index);
		}
		if (event == R_NOTIFICATION_ACTIVATED) {
			if (!g_notify.stay_behind) rvn_notify_bring_back(window);
			if (g_notify.clicked) g_notify.clicked(g_notify.clicked_user);
		}
		if (g_notify.callback && g_rvn.initialized) {
			g_notify.callback(name, (RNotificationEvent)event, event == R_NOTIFICATION_ACTION_INVOKED ? (action ? action : "") : NULL,
				g_notify.callback_user);
		}
		rvn_free(name);
	}
}

static int rvn_notify_to_fallback(const rvn_notification_t* notification) {
	if (g_notify.toast != R_INVALID_ID && rvn_toast_show_notification(g_notify.toast, notification)) return R_TRUE;
	if (g_notify.fallback) {
		g_notify.fallback(notification->title, notification->body, (RNotificationUrgency)(notification->urgency + R_NOTIFICATION_LOW),
			g_notify.fallback_user);
		return R_TRUE;
	}
	return R_FALSE;
}

void rvn_notification_fallback(const rvn_notification_t* notification, const char* reason) {
	if (!g_rvn.initialized) return;
	if (reason && *reason) rvn_error(R_ERROR_PLATFORM, R_SEVERITY_WARNING, "Rnotification_show: the system shows no notifications: %s", reason);
	rvn_notify_to_fallback(notification);
}

static int rvn_notify_show(const RNotificationDesc* desc, const char* function) {
	rvn_notification_t notification;
	const char* ids[RVN_NOTIFY_MAX_ACTIONS];
	const char* labels[RVN_NOTIFY_MAX_ACTIONS];
	char tag[32];
	char* icon = NULL;
	int i, shown = R_FALSE;
	RNotificationUrgency urgency = desc->urgency;
	if ((!desc->title || !*desc->title) && (!desc->body || !*desc->body)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: no title and no text", function);
		return R_FALSE;
	}
	if (desc->action_count < 0 || desc->action_count > RVN_NOTIFY_MAX_ACTIONS || (desc->action_count > 0 && !desc->actions)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: from 0 to %d actions", function, RVN_NOTIFY_MAX_ACTIONS);
		return R_FALSE;
	}
	for (i = 0; i < desc->action_count; i++) {
		const char* id = desc->actions[i].id;
		int k;
		if (!id || !*id || strcmp(id, "default") == 0 || !desc->actions[i].label || !*desc->actions[i].label) {
			rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: an action needs an id (not \"default\") and a label", function);
			return R_FALSE;
		}
		for (k = 0; k < i; k++) {
			if (strcmp(ids[k], id) == 0) {
				rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: two actions have the id \"%s\"", function, id);
				return R_FALSE;
			}
		}
		ids[i] = id;
		labels[i] = desc->actions[i].label;
	}
	if (urgency < R_NOTIFICATION_LOW || urgency > R_NOTIFICATION_CRITICAL) urgency = R_NOTIFICATION_NORMAL;
	if (desc->icon && *desc->icon) {
		icon = rvn_path_absolute(desc->icon);
		if (icon && !rvn_path_exists(icon)) {
			rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_WARNING, "%s: no icon %s", function, desc->icon);
			rvn_free(icon);
			icon = NULL;
		}
	}
	if (!desc->tag || !*desc->tag) snprintf(tag, sizeof(tag), "raven.%u", ++g_notify.serial);
	memset(&notification, 0, sizeof(notification));
	notification.app = g_rvn.app_name && *g_rvn.app_name ? g_rvn.app_name : "Raven";
	notification.tag = desc->tag && *desc->tag ? desc->tag : tag;
	notification.title = desc->title ? desc->title : "";
	notification.body = desc->body ? desc->body : "";
	notification.urgency = (int)(urgency - R_NOTIFICATION_LOW);
	notification.icon = icon ? icon : "";
	notification.action_ids = ids;
	notification.action_labels = labels;
	notification.action_count = desc->action_count;
	rvn_notify_remember(notification.tag, desc->window);
	if (g_rvn.platform && g_rvn.platform->notify) shown = g_rvn.platform->notify(&notification);
	if (!shown) rvn_notify_to_fallback(&notification);
	rvn_free(icon);
	return shown;
}

int Rnotification_show(const char* title, const char* body, RNotificationUrgency urgency) {
	RNotificationDesc desc;
	RVN_API_GUARD(R_FALSE);
	memset(&desc, 0, sizeof(desc));
	desc.size = (Ruint)sizeof(desc);
	desc.title = title;
	desc.body = body;
	desc.urgency = urgency;
	return rvn_notify_show(&desc, "Rnotification_show");
}

int Rnotification_show_desc(const RNotificationDesc* desc) {
	RNotificationDesc copy;
	RVN_API_GUARD(R_FALSE);
	if (!desc || desc->size < (Ruint)sizeof(RNotificationDesc)) {
		/* (this is the first version: every field is there) */
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rnotification_show_desc: %s",
			desc ? "RNotificationDesc::size must be sizeof(RNotificationDesc)" : "no description");
		return R_FALSE;
	}
	/* (a size larger than this Raven knows: the fields it does not know are left out) */
	memcpy(&copy, desc, sizeof(copy));
	return rvn_notify_show(&copy, "Rnotification_show_desc");
}

void Rnotification_close(const char* tag) {
	int index;
	RVN_API_GUARD_VOID();
	if (!tag || !*tag) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rnotification_close: no tag");
		return;
	}
	if (g_rvn.platform && g_rvn.platform->notify_close) g_rvn.platform->notify_close(tag);
	if (g_notify.toast != R_INVALID_ID) rvn_toast_close_notification(g_notify.toast, tag);
	index = rvn_notify_find(tag);
	if (index >= 0) rvn_notify_forget(index);
}

void Rnotification_set_fallback(RNotificationFallback callback, void* user_data) {
	RVN_API_GUARD_VOID();
	g_notify.fallback = callback;
	g_notify.fallback_user = user_data;
}

void Rnotification_set_fallback_toast(Rid toast) {
	RVN_API_GUARD_VOID();
	if (toast != R_INVALID_ID && Rwidget_get_type(toast) != R_WIDGET_TOAST) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rnotification_set_fallback_toast: not a Toast");
		return;
	}
	g_notify.toast = toast;
}

void Rnotification_set_clicked_callback(RNotificationClickedCallback callback, void* user_data) {
	RVN_API_GUARD_VOID();
	g_notify.clicked = callback;
	g_notify.clicked_user = user_data;
}

void Rnotification_set_callback(RNotificationCallback callback, void* user_data) {
	RVN_API_GUARD_VOID();
	g_notify.callback = callback;
	g_notify.callback_user = user_data;
}

void Rnotification_set_bring_to_front(int enabled) {
	RVN_API_GUARD_VOID();
	g_notify.stay_behind = !enabled;
}

int Rnotification_is_supported(void) {
	RVN_API_GUARD(R_FALSE);
	if (!g_rvn.platform || !g_rvn.platform->notify) return R_FALSE;
	/* (a platform that tells what its notifications can do has none when it says nothing: no
	 * notification server on Linux, an executable outside of a bundle on macOS) */
	return !g_rvn.platform->notify_features || g_rvn.platform->notify_features() != 0;
}

int Rnotification_supports(RNotificationFeature feature) {
	unsigned features;
	RVN_API_GUARD(R_FALSE);
	if (!g_rvn.platform || !g_rvn.platform->notify || !g_rvn.platform->notify_features) return R_FALSE;
	features = g_rvn.platform->notify_features();
	return feature != 0 && (features & (unsigned)feature) == (unsigned)feature;
}

/* ---------------------------------------------------------------------------
 *  What the platforms share
 * ------------------------------------------------------------------------- */

int rvn_notification_dbus_actions(const rvn_notification_t* notification, const char** out, int capacity) {
	int i, n = 0;
	/* the click on the notification itself, without a label (as GLib does: no button is shown for it) */
	if (n + 2 <= capacity) {
		out[n++] = "default";
		out[n++] = "";
	}
	for (i = 0; i < notification->action_count && n + 2 <= capacity; i++) {
		out[n++] = notification->action_ids[i];
		out[n++] = notification->action_labels[i];
	}
	return n;
}

char* rvn_notification_file_uri(const char* path) {
	static const char hex[] = "0123456789ABCDEF";
	size_t i, n = 0, length = strlen(path);
	char* uri = (char*)rvn_alloc(length * 3 + 10);
	memcpy(uri, "file://", 7);
	n = 7;
#if defined(_WIN32)
	/* C:\a -> file:///C:/a */
	if (length > 1 && path[1] == ':') uri[n++] = '/';
#endif
	for (i = 0; i < length; i++) {
		unsigned char c = (unsigned char)path[i];
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '/' || c == '-' || c == '_' || c == '.' ||
			c == '~' || (c == ':' && i == 1)) {
			uri[n++] = (char)c;
		} else if (c == '\\') {
			uri[n++] = '/';
		} else {
			uri[n++] = '%';
			uri[n++] = hex[c >> 4];
			uri[n++] = hex[c & 15];
		}
	}
	uri[n] = 0;
	return uri;
}

/* ===========================================================================
 *  Linux and the BSDs: org.freedesktop.Notifications
 * ========================================================================= */

#if !defined(_WIN32) && !defined(__APPLE__)

#include <dlfcn.h>
#include <errno.h>

typedef struct DBusConnection DBusConnection;
typedef struct DBusMessage DBusMessage;
typedef uint32_t dbus_bool_t;
typedef struct DBusError {
	const char* name;
	const char* message;
	unsigned int dummy;
	void* padding1;
} DBusError;
/* (larger than the real one, which libdbus fills) */
typedef struct DBusMessageIter {
	void* dummy[16];
} DBusMessageIter;

#define DBUS_BUS_SESSION 0

static struct {
	void* library;
	int failed;
	DBusConnection* bus;
	void (*error_init)(DBusError*);
	void (*error_free)(DBusError*);
	DBusConnection* (*bus_get_private)(int, DBusError*);
	void (*connection_set_exit_on_disconnect)(DBusConnection*, dbus_bool_t);
	void (*connection_close)(DBusConnection*);
	void (*connection_unref)(DBusConnection*);
	DBusMessage* (*connection_send_with_reply_and_block)(DBusConnection*, DBusMessage*, int, DBusError*);
	DBusMessage* (*message_new_method_call)(const char*, const char*, const char*, const char*);
	void (*message_unref)(DBusMessage*);
	void (*iter_init_append)(DBusMessage*, DBusMessageIter*);
	dbus_bool_t (*iter_append_basic)(DBusMessageIter*, int, const void*);
	dbus_bool_t (*iter_open_container)(DBusMessageIter*, int, const char*, DBusMessageIter*);
	dbus_bool_t (*iter_close_container)(DBusMessageIter*, DBusMessageIter*);
	DBusMessage* (*message_new_signal)(const char*, const char*, const char*);
	dbus_bool_t (*connection_send)(DBusConnection*, DBusMessage*, uint32_t*);
	void (*connection_flush)(DBusConnection*);
	dbus_bool_t (*connection_read_write)(DBusConnection*, int);
	DBusMessage* (*connection_pop_message)(DBusConnection*);
	dbus_bool_t (*connection_get_unix_fd)(DBusConnection*, int*);
	void (*bus_add_match)(DBusConnection*, const char*, DBusError*);
	dbus_bool_t (*message_is_signal)(DBusMessage*, const char*, const char*);
	dbus_bool_t (*iter_init)(DBusMessage*, DBusMessageIter*);
	int (*iter_get_arg_type)(DBusMessageIter*);
	void (*iter_get_basic)(DBusMessageIter*, void*);
	dbus_bool_t (*iter_next)(DBusMessageIter*);
	void (*iter_recurse)(DBusMessageIter*, DBusMessageIter*);
	/* GetCapabilities was asked (1), and what it said: RNotificationFeature bits */
	int asked;
	unsigned features;
} g_dbus;

/* The notifications shown and not closed yet: their tag and the id the server gave them. */
typedef struct rvn_dbus_shown_t {
	char* tag;
	uint32_t id;
	/* a button or the notification itself was clicked: its closing is not a dismissal */
	int answered;
} rvn_dbus_shown_t;

#define RVN_DBUS_SHOWN_MAX 64

static struct {
	rvn_dbus_shown_t items[RVN_DBUS_SHOWN_MAX];
	int count;
} g_shown;

static void rvn_dbus_shown_remove(int index) {
	rvn_free(g_shown.items[index].tag);
	memmove(&g_shown.items[index], &g_shown.items[index + 1], (size_t)(g_shown.count - index - 1) * sizeof(rvn_dbus_shown_t));
	g_shown.count--;
}

static int rvn_dbus_shown_by_tag(const char* tag) {
	int i;
	for (i = 0; i < g_shown.count; i++) {
		if (strcmp(g_shown.items[i].tag, tag) == 0) return i;
	}
	return -1;
}

static int rvn_dbus_shown_by_id(uint32_t id) {
	int i;
	for (i = 0; i < g_shown.count; i++) {
		if (g_shown.items[i].id == id) return i;
	}
	return -1;
}

static int rvn_notify_connect(void) {
	static const char* const names[] = { "libdbus-1.so.3", "libdbus-1.so" };
	DBusError error;
	size_t i;
	if (g_dbus.bus) return R_TRUE;
	if (g_dbus.failed) return R_FALSE;
	g_dbus.failed = R_TRUE;
	for (i = 0; i < RVN_COUNT_OF(names) && !g_dbus.library; i++) g_dbus.library = dlopen(names[i], RTLD_NOW | RTLD_LOCAL);
	if (!g_dbus.library) return R_FALSE;
#define LOAD(field, symbol) \
	do { \
		*(void**)(&g_dbus.field) = dlsym(g_dbus.library, symbol); \
		if (!g_dbus.field) return R_FALSE; \
	} while (0)
	LOAD(error_init, "dbus_error_init");
	LOAD(error_free, "dbus_error_free");
	LOAD(bus_get_private, "dbus_bus_get_private");
	LOAD(connection_set_exit_on_disconnect, "dbus_connection_set_exit_on_disconnect");
	LOAD(connection_close, "dbus_connection_close");
	LOAD(connection_unref, "dbus_connection_unref");
	LOAD(connection_send_with_reply_and_block, "dbus_connection_send_with_reply_and_block");
	LOAD(message_new_method_call, "dbus_message_new_method_call");
	LOAD(message_unref, "dbus_message_unref");
	LOAD(iter_init_append, "dbus_message_iter_init_append");
	LOAD(iter_append_basic, "dbus_message_iter_append_basic");
	LOAD(iter_open_container, "dbus_message_iter_open_container");
	LOAD(iter_close_container, "dbus_message_iter_close_container");
	LOAD(message_new_signal, "dbus_message_new_signal");
	LOAD(connection_send, "dbus_connection_send");
	LOAD(connection_flush, "dbus_connection_flush");
	LOAD(connection_read_write, "dbus_connection_read_write");
	LOAD(connection_pop_message, "dbus_connection_pop_message");
	LOAD(connection_get_unix_fd, "dbus_connection_get_unix_fd");
	LOAD(bus_add_match, "dbus_bus_add_match");
	LOAD(message_is_signal, "dbus_message_is_signal");
	LOAD(iter_init, "dbus_message_iter_init");
	LOAD(iter_get_arg_type, "dbus_message_iter_get_arg_type");
	LOAD(iter_get_basic, "dbus_message_iter_get_basic");
	LOAD(iter_next, "dbus_message_iter_next");
	LOAD(iter_recurse, "dbus_message_iter_recurse");
#undef LOAD
	g_dbus.error_init(&error);
	g_dbus.bus = g_dbus.bus_get_private(DBUS_BUS_SESSION, &error);
	g_dbus.error_free(&error);
	if (!g_dbus.bus) return R_FALSE;
	/* a session bus that goes away must not end the application */
	g_dbus.connection_set_exit_on_disconnect(g_dbus.bus, 0);
	/* what the user does with the notifications: ActionInvoked, NotificationClosed (read by rvn_dbus_notify_dispatch) */
	g_dbus.error_init(&error);
	g_dbus.bus_add_match(g_dbus.bus, "type='signal',interface='org.freedesktop.Notifications',path='/org/freedesktop/Notifications'", &error);
	g_dbus.error_free(&error);
	g_dbus.failed = R_FALSE;
	return R_TRUE;
}

static void rvn_dbus_hint_string(DBusMessageIter* hints, const char* key, const char* value) {
	DBusMessageIter entry, variant;
	g_dbus.iter_open_container(hints, 'e', NULL, &entry);
	g_dbus.iter_append_basic(&entry, 's', &key);
	g_dbus.iter_open_container(&entry, 'v', "s", &variant);
	g_dbus.iter_append_basic(&variant, 's', &value);
	g_dbus.iter_close_container(&entry, &variant);
	g_dbus.iter_close_container(hints, &entry);
}

/* The first argument of a reply when it is a uint32 (the id given by Notify). */
static int rvn_dbus_reply_uint(DBusMessage* reply, uint32_t* value) {
	DBusMessageIter it;
	if (!g_dbus.iter_init(reply, &it) || g_dbus.iter_get_arg_type(&it) != 'u') return R_FALSE;
	g_dbus.iter_get_basic(&it, value);
	return R_TRUE;
}

int rvn_dbus_notify(const rvn_notification_t* notification) {
	DBusMessage* call;
	DBusMessage* reply;
	DBusMessageIter args, actions, hints, entry, variant;
	DBusError error;
	const char* strings[2 + 2 * 8];
	const char* empty = "";
	const char* key = "urgency";
	char* image = NULL;
	uint32_t replaces = 0, id = 0;
	int32_t timeout = -1;
	int i, count, index;
	unsigned char level = (unsigned char)rvn_clampi(notification->urgency, 0, 2);
	if (!rvn_notify_connect()) return R_FALSE;
	call = g_dbus.message_new_method_call("org.freedesktop.Notifications", "/org/freedesktop/Notifications",
		"org.freedesktop.Notifications", "Notify");
	if (!call) return R_FALSE;
	/* the same tag: the notification still shown is replaced */
	index = rvn_dbus_shown_by_tag(notification->tag);
	if (index >= 0) replaces = g_shown.items[index].id;
	/* Notify(app_name s, replaces_id u, app_icon s, summary s, body s, actions as, hints a{sv}, expire_timeout i) */
	g_dbus.iter_init_append(call, &args);
	g_dbus.iter_append_basic(&args, 's', &notification->app);
	g_dbus.iter_append_basic(&args, 'u', &replaces);
	g_dbus.iter_append_basic(&args, 's', &empty);
	g_dbus.iter_append_basic(&args, 's', &notification->title);
	g_dbus.iter_append_basic(&args, 's', &notification->body);
	g_dbus.iter_open_container(&args, 'a', "s", &actions);
	count = rvn_notification_dbus_actions(notification, strings, (int)RVN_COUNT_OF(strings));
	for (i = 0; i < count; i++) g_dbus.iter_append_basic(&actions, 's', &strings[i]);
	g_dbus.iter_close_container(&args, &actions);
	g_dbus.iter_open_container(&args, 'a', "{sv}", &hints);
	g_dbus.iter_open_container(&hints, 'e', NULL, &entry);
	g_dbus.iter_append_basic(&entry, 's', &key);
	g_dbus.iter_open_container(&entry, 'v', "y", &variant);
	g_dbus.iter_append_basic(&variant, 'y', &level);
	g_dbus.iter_close_container(&entry, &variant);
	g_dbus.iter_close_container(&hints, &entry);
	if (notification->icon[0]) {
		/* (the image of the notification, as GLib gives it: a file:// URI) */
		image = rvn_notification_file_uri(notification->icon);
		rvn_dbus_hint_string(&hints, "image-path", image);
	}
	g_dbus.iter_close_container(&args, &hints);
	g_dbus.iter_append_basic(&args, 'i', &timeout);
	/* the answer says whether a notification server is there (it may be started for the call) */
	g_dbus.error_init(&error);
	reply = g_dbus.connection_send_with_reply_and_block(g_dbus.bus, call, 1000, &error);
	g_dbus.error_free(&error);
	g_dbus.message_unref(call);
	rvn_free(image);
	if (!reply) return R_FALSE;
	if (rvn_dbus_reply_uint(reply, &id) && id != 0) {
		/* (the newest at the end; the oldest one forgotten when there are too many) */
		index = rvn_dbus_shown_by_tag(notification->tag);
		if (index >= 0) rvn_dbus_shown_remove(index);
		index = rvn_dbus_shown_by_id(id);
		if (index >= 0) rvn_dbus_shown_remove(index);
		if (g_shown.count == RVN_DBUS_SHOWN_MAX) rvn_dbus_shown_remove(0);
		g_shown.items[g_shown.count].tag = rvn_strdup(notification->tag);
		g_shown.items[g_shown.count].id = id;
		g_shown.items[g_shown.count].answered = R_FALSE;
		g_shown.count++;
	}
	g_dbus.message_unref(reply);
	return R_TRUE;
}

void rvn_dbus_notify_close(const char* tag) {
	DBusMessage* call;
	DBusMessageIter args;
	int index = rvn_dbus_shown_by_tag(tag);
	uint32_t id;
	if (index < 0 || !g_dbus.bus) return;
	id = g_shown.items[index].id;
	/* (forgotten first: the NotificationClosed that follows is not reported) */
	rvn_dbus_shown_remove(index);
	call = g_dbus.message_new_method_call("org.freedesktop.Notifications", "/org/freedesktop/Notifications",
		"org.freedesktop.Notifications", "CloseNotification");
	if (!call) return;
	g_dbus.iter_init_append(call, &args);
	g_dbus.iter_append_basic(&args, 'u', &id);
	g_dbus.connection_send(g_dbus.bus, call, NULL);
	g_dbus.connection_flush(g_dbus.bus);
	g_dbus.message_unref(call);
}

unsigned rvn_dbus_notify_features(void) {
	DBusMessage* call;
	DBusMessage* reply;
	DBusMessageIter it, list;
	DBusError error;
	if (g_dbus.asked) return g_dbus.features;
	if (!rvn_notify_connect()) return 0;
	g_dbus.asked = R_TRUE;
	g_dbus.features = 0;
	call = g_dbus.message_new_method_call("org.freedesktop.Notifications", "/org/freedesktop/Notifications",
		"org.freedesktop.Notifications", "GetCapabilities");
	if (!call) return 0;
	g_dbus.error_init(&error);
	reply = g_dbus.connection_send_with_reply_and_block(g_dbus.bus, call, 1000, &error);
	g_dbus.error_free(&error);
	g_dbus.message_unref(call);
	if (!reply) return 0;
	/* a server is there: tags, closing, an image and the dismissals are in the protocol itself */
	g_dbus.features = R_NOTIFICATION_FEATURE_REPLACE | R_NOTIFICATION_FEATURE_CLOSE | R_NOTIFICATION_FEATURE_ICON |
		R_NOTIFICATION_FEATURE_DISMISSED;
	/* GetCapabilities() -> as: "actions" where buttons are shown and clicks reported */
	if (g_dbus.iter_init(reply, &it) && g_dbus.iter_get_arg_type(&it) == 'a') {
		g_dbus.iter_recurse(&it, &list);
		while (g_dbus.iter_get_arg_type(&list) == 's') {
			const char* name = NULL;
			g_dbus.iter_get_basic(&list, &name);
			if (name && strcmp(name, "actions") == 0) g_dbus.features |= R_NOTIFICATION_FEATURE_ACTIONS | R_NOTIFICATION_FEATURE_ACTIVATED;
			if (!g_dbus.iter_next(&list)) break;
		}
	}
	g_dbus.message_unref(reply);
	return g_dbus.features;
}

int rvn_dbus_notify_fd(void) {
	int fd = -1;
	if (!g_dbus.bus || g_shown.count == 0 || !g_dbus.connection_get_unix_fd(g_dbus.bus, &fd)) return -1;
	return fd;
}

/* Two arguments of a signal: a uint32 then a uint32 or a string. */
static int rvn_dbus_signal_args(DBusMessage* message, uint32_t* id, int type, void* second) {
	DBusMessageIter it;
	if (!g_dbus.iter_init(message, &it) || g_dbus.iter_get_arg_type(&it) != 'u') return R_FALSE;
	g_dbus.iter_get_basic(&it, id);
	if (!g_dbus.iter_next(&it) || g_dbus.iter_get_arg_type(&it) != type) return R_FALSE;
	g_dbus.iter_get_basic(&it, second);
	return R_TRUE;
}

static void rvn_dbus_notify_message(DBusMessage* message) {
	uint32_t id = 0, reason = 0;
	const char* action = NULL;
	int index;
	char* tag;
	if (g_dbus.message_is_signal(message, "org.freedesktop.Notifications", "ActionInvoked")) {
		/* ActionInvoked(id u, action_key s): "default" is a click on the notification itself */
		if (!rvn_dbus_signal_args(message, &id, 's', &action) || !action) return;
		index = rvn_dbus_shown_by_id(id);
		/* (a notification of another application) */
		if (index < 0) return;
		g_shown.items[index].answered = R_TRUE;
		/* (the callback may show or close notifications: nothing of the list is held across it) */
		tag = rvn_strdup(g_shown.items[index].tag);
		if (strcmp(action, "default") == 0) rvn_notification_event(tag, R_NOTIFICATION_ACTIVATED, NULL);
		else rvn_notification_event(tag, R_NOTIFICATION_ACTION_INVOKED, action);
		rvn_free(tag);
	} else if (g_dbus.message_is_signal(message, "org.freedesktop.Notifications", "NotificationClosed")) {
		/* NotificationClosed(id u, reason u): 1 expired, 2 dismissed by the user, 3 CloseNotification, 4 other */
		int answered;
		if (!rvn_dbus_signal_args(message, &id, 'u', &reason)) return;
		index = rvn_dbus_shown_by_id(id);
		if (index < 0) return;
		answered = g_shown.items[index].answered;
		tag = rvn_strdup(g_shown.items[index].tag);
		rvn_dbus_shown_remove(index);
		if (!answered && (reason == 1 || reason == 2)) rvn_notification_event(tag, R_NOTIFICATION_DISMISSED, NULL);
		rvn_free(tag);
	}
	/* (ActivationToken(id u, token s) comes before ActionInvoked: the Wayland platform has no
	 * xdg-activation to give it to, so the window is brought back as Rwindow_focus does) */
}

void rvn_dbus_notify_dispatch(void) {
	DBusMessage* message;
	int guard = 0;
	if (!g_dbus.bus) return;
	g_dbus.connection_read_write(g_dbus.bus, 0);
	while (guard++ < 256 && (message = g_dbus.connection_pop_message(g_dbus.bus)) != NULL) {
		rvn_dbus_notify_message(message);
		g_dbus.message_unref(message);
		/* (a callback may have ended Raven) */
		if (!g_dbus.bus) break;
	}
}

/* ---------------------------------------------------------------------------
 *  The launchers: com.canonical.Unity.LauncherEntry
 *
 *  A signal, Update(app_uri s, properties a{sv}), names the .desktop file of
 *  the application ("application://ID.desktop", ID as the application id of
 *  the windows: RAVEN_APP_ID, else the name of the program) with progress,
 *  count and urgent. KDE, the Ubuntu Dock, Dash to Dock and Plank show it.
 *  Nothing answers a signal: R_FALSE only when there is no session bus.
 * ------------------------------------------------------------------------- */

static struct {
	double progress;
	int progress_visible;
	int64_t count;
	int count_visible;
	int urgent;
} g_launcher;

static void rvn_launcher_entry(DBusMessageIter* dict, const char* key, int type, const char* signature, const void* value) {
	DBusMessageIter entry, variant;
	g_dbus.iter_open_container(dict, 'e', NULL, &entry);
	g_dbus.iter_append_basic(&entry, 's', &key);
	g_dbus.iter_open_container(&entry, 'v', signature, &variant);
	g_dbus.iter_append_basic(&variant, type, value);
	g_dbus.iter_close_container(&entry, &variant);
	g_dbus.iter_close_container(dict, &entry);
}

static int rvn_launcher_send(void) {
	char uri[300], path[96];
	const char* id = getenv("RAVEN_APP_ID");
	const char* text = uri;
	DBusMessage* signal;
	DBusMessageIter args, dict;
	dbus_bool_t progress_visible = g_launcher.progress_visible ? 1 : 0;
	dbus_bool_t count_visible = g_launcher.count_visible ? 1 : 0;
	dbus_bool_t urgent = g_launcher.urgent ? 1 : 0;
	uint32_t hash = 5381;
	const char* c;
	if (!rvn_notify_connect()) return R_FALSE;
#if defined(__GLIBC__)
	if (!id || !id[0]) id = program_invocation_short_name;
#endif
	snprintf(uri, sizeof(uri), "application://%s.desktop", id && id[0] ? id : "raven");
	/* (any object path will do: one per application) */
	for (c = uri; *c; c++) hash = hash * 33u + (unsigned char)*c;
	snprintf(path, sizeof(path), "/com/canonical/unity/launcherentry/%u", (unsigned)hash);
	signal = g_dbus.message_new_signal(path, "com.canonical.Unity.LauncherEntry", "Update");
	if (!signal) return R_FALSE;
	g_dbus.iter_init_append(signal, &args);
	g_dbus.iter_append_basic(&args, 's', &text);
	g_dbus.iter_open_container(&args, 'a', "{sv}", &dict);
	rvn_launcher_entry(&dict, "progress", 'd', "d", &g_launcher.progress);
	rvn_launcher_entry(&dict, "progress-visible", 'b', "b", &progress_visible);
	rvn_launcher_entry(&dict, "count", 'x', "x", &g_launcher.count);
	rvn_launcher_entry(&dict, "count-visible", 'b', "b", &count_visible);
	rvn_launcher_entry(&dict, "urgent", 'b', "b", &urgent);
	g_dbus.iter_close_container(&args, &dict);
	g_dbus.connection_send(g_dbus.bus, signal, NULL);
	g_dbus.connection_flush(g_dbus.bus);
	g_dbus.message_unref(signal);
	return R_TRUE;
}

int rvn_dbus_launcher_progress(int state, float value) {
	/* (the launchers know no paused, failed or indeterminate bar: a bar, full when indeterminate) */
	g_launcher.progress_visible = state != 0;
	g_launcher.progress = state == 4 ? 1.0 : (double)value;
	return rvn_launcher_send();
}

int rvn_dbus_launcher_badge(const char* text) {
	char* end = NULL;
	long long count = text ? strtoll(text, &end, 10) : 0;
	/* a number only: a text that is not one shows nothing */
	g_launcher.count_visible = text && text[0] && end && *end == '\0' && count >= 0;
	g_launcher.count = g_launcher.count_visible ? (int64_t)count : 0;
	if (!rvn_launcher_send()) return R_FALSE;
	return g_launcher.count_visible || !text;
}

int rvn_dbus_launcher_urgent(int urgent) {
	g_launcher.urgent = urgent ? R_TRUE : R_FALSE;
	return rvn_launcher_send();
}

void rvn_dbus_notify_shutdown(void) {
	if (g_dbus.bus) {
		g_dbus.connection_close(g_dbus.bus);
		g_dbus.connection_unref(g_dbus.bus);
		g_dbus.bus = NULL;
	}
	/* (the library stays loaded: atspi.c may use it, and unloading libdbus is not safe) */
	g_dbus.failed = R_FALSE;
	g_dbus.asked = R_FALSE;
	g_dbus.features = 0;
	while (g_shown.count > 0) rvn_dbus_shown_remove(g_shown.count - 1);
	memset(&g_launcher, 0, sizeof(g_launcher));
}

#endif
