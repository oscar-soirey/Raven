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

static struct {
	RNotificationFallback fallback;
	void* fallback_user;
} g_notify;

int Rnotification_show(const char* title, const char* body, RNotificationUrgency urgency) {
	int shown = R_FALSE;
	RVN_API_GUARD(R_FALSE);
	if ((!title || !*title) && (!body || !*body)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rnotification_show: no title and no text");
		return R_FALSE;
	}
	if (urgency < R_NOTIFICATION_LOW || urgency > R_NOTIFICATION_CRITICAL) urgency = R_NOTIFICATION_NORMAL;
	if (g_rvn.platform && g_rvn.platform->notify) {
		shown = g_rvn.platform->notify(g_rvn.app_name && *g_rvn.app_name ? g_rvn.app_name : "Raven",
			title ? title : "", body ? body : "", (int)(urgency - R_NOTIFICATION_LOW));
	}
	if (!shown && g_notify.fallback) g_notify.fallback(title ? title : "", body ? body : "", urgency, g_notify.fallback_user);
	return shown;
}

void Rnotification_set_fallback(RNotificationFallback callback, void* user_data) {
	RVN_API_GUARD_VOID();
	g_notify.fallback = callback;
	g_notify.fallback_user = user_data;
}

int Rnotification_is_supported(void) {
	RVN_API_GUARD(R_FALSE);
	return g_rvn.platform && g_rvn.platform->notify != NULL;
}

/* ===========================================================================
 *  Linux and the BSDs: org.freedesktop.Notifications
 * ========================================================================= */

#if !defined(_WIN32) && !defined(__APPLE__)

#include <dlfcn.h>

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
} g_dbus;

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
#undef LOAD
	g_dbus.error_init(&error);
	g_dbus.bus = g_dbus.bus_get_private(DBUS_BUS_SESSION, &error);
	g_dbus.error_free(&error);
	if (!g_dbus.bus) return R_FALSE;
	/* a session bus that goes away must not end the application */
	g_dbus.connection_set_exit_on_disconnect(g_dbus.bus, 0);
	g_dbus.failed = R_FALSE;
	return R_TRUE;
}

int rvn_dbus_notify(const char* app, const char* title, const char* body, int urgency) {
	DBusMessage* call;
	DBusMessage* reply;
	DBusMessageIter args, actions, hints, entry, variant;
	DBusError error;
	const char* icon = "";
	const char* key = "urgency";
	uint32_t replaces = 0;
	int32_t timeout = -1;
	unsigned char level = (unsigned char)rvn_clampi(urgency, 0, 2);
	if (!rvn_notify_connect()) return R_FALSE;
	call = g_dbus.message_new_method_call("org.freedesktop.Notifications", "/org/freedesktop/Notifications",
		"org.freedesktop.Notifications", "Notify");
	if (!call) return R_FALSE;
	/* Notify(app_name s, replaces_id u, app_icon s, summary s, body s, actions as, hints a{sv}, expire_timeout i) */
	g_dbus.iter_init_append(call, &args);
	g_dbus.iter_append_basic(&args, 's', &app);
	g_dbus.iter_append_basic(&args, 'u', &replaces);
	g_dbus.iter_append_basic(&args, 's', &icon);
	g_dbus.iter_append_basic(&args, 's', &title);
	g_dbus.iter_append_basic(&args, 's', &body);
	g_dbus.iter_open_container(&args, 'a', "s", &actions);
	g_dbus.iter_close_container(&args, &actions);
	g_dbus.iter_open_container(&args, 'a', "{sv}", &hints);
	g_dbus.iter_open_container(&hints, 'e', NULL, &entry);
	g_dbus.iter_append_basic(&entry, 's', &key);
	g_dbus.iter_open_container(&entry, 'v', "y", &variant);
	g_dbus.iter_append_basic(&variant, 'y', &level);
	g_dbus.iter_close_container(&entry, &variant);
	g_dbus.iter_close_container(&hints, &entry);
	g_dbus.iter_close_container(&args, &hints);
	g_dbus.iter_append_basic(&args, 'i', &timeout);
	/* the answer says whether a notification server is there (it may be started for the call) */
	g_dbus.error_init(&error);
	reply = g_dbus.connection_send_with_reply_and_block(g_dbus.bus, call, 1000, &error);
	g_dbus.error_free(&error);
	g_dbus.message_unref(call);
	if (!reply) return R_FALSE;
	g_dbus.message_unref(reply);
	return R_TRUE;
}

void rvn_dbus_notify_shutdown(void) {
	if (g_dbus.bus) {
		g_dbus.connection_close(g_dbus.bus);
		g_dbus.connection_unref(g_dbus.bus);
		g_dbus.bus = NULL;
	}
	/* (the library stays loaded: atspi.c may use it, and unloading libdbus is not safe) */
	g_dbus.failed = R_FALSE;
}

#endif
