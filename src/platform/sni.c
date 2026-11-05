/**
 * Raven - platform/sni.c
 * Tray icons of Linux and the BSDs: StatusNotifierItem, the protocol of the
 * panels of KDE, of GNOME with the AppIndicator extension, of Cinnamon, XFCE,
 * LXQt, waybar... spoken over the session bus.
 *
 * Each icon is an object of the application on the bus:
 *     /org/raven/tray/<n>        org.kde.StatusNotifierItem (its picture, its
 *                                tooltip, Activate on a click...)
 *     /org/raven/tray/<n>/menu   com.canonical.dbusmenu (its menu, which the
 *                                panel draws and whose entries it says chosen)
 * registered with org.kde.StatusNotifierWatcher, again whenever a watcher
 * starts (the panel restarted, or came after the application).
 *
 * As for the screen readers (atspi.c), libdbus is loaded when the first icon
 * is made: Raven links with nothing more. The calls of the panel are
 * answered when the platform pumps its events (rvn_sni_fd, rvn_sni_dispatch).
 */

#if !defined(_WIN32) && !defined(__APPLE__)

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include "platform.h"
#include "sni.h"
#include "../core/context.h"

#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>

/* ---------------------------------------------------------------------------
 *  The part of libdbus that is used (its ABI is stable since 1.0)
 * ------------------------------------------------------------------------- */

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
typedef int (*DBusHandleMessageFunction)(DBusConnection*, DBusMessage*, void*);
typedef struct DBusObjectPathVTable {
	void (*unregister_function)(DBusConnection*, void*);
	DBusHandleMessageFunction message_function;
	void (*pad1)(void*);
	void (*pad2)(void*);
	void (*pad3)(void*);
	void (*pad4)(void*);
} DBusObjectPathVTable;

#define DBUS_BUS_SESSION 0
#define DBUS_HANDLER_RESULT_HANDLED 0
#define DBUS_HANDLER_RESULT_NOT_YET_HANDLED 1
#define DBUS_DISPATCH_DATA_REMAINS 0
#define DBUS_MESSAGE_TYPE_METHOD_CALL 1
#define DBUS_MESSAGE_TYPE_SIGNAL 4
#define DBUS_NAME_FLAG_DO_NOT_QUEUE 4

static struct {
	void* library;
	int failed;
	void (*error_init)(DBusError*);
	void (*error_free)(DBusError*);
	DBusConnection* (*bus_get_private)(int, DBusError*);
	const char* (*bus_get_unique_name)(DBusConnection*);
	void (*bus_add_match)(DBusConnection*, const char*, DBusError*);
	dbus_bool_t (*bus_name_has_owner)(DBusConnection*, const char*, DBusError*);
	void (*connection_set_exit_on_disconnect)(DBusConnection*, dbus_bool_t);
	void (*connection_close)(DBusConnection*);
	void (*connection_unref)(DBusConnection*);
	dbus_bool_t (*connection_register_fallback)(DBusConnection*, const char*, const DBusObjectPathVTable*, void*);
	dbus_bool_t (*connection_add_filter)(DBusConnection*, DBusHandleMessageFunction, void*, void (*)(void*));
	dbus_bool_t (*connection_read_write)(DBusConnection*, int);
	int (*connection_dispatch)(DBusConnection*);
	dbus_bool_t (*connection_send)(DBusConnection*, DBusMessage*, uint32_t*);
	void (*connection_flush)(DBusConnection*);
	DBusMessage* (*connection_send_with_reply_and_block)(DBusConnection*, DBusMessage*, int, DBusError*);
	dbus_bool_t (*connection_get_unix_fd)(DBusConnection*, int*);
	DBusMessage* (*message_new_method_call)(const char*, const char*, const char*, const char*);
	DBusMessage* (*message_new_method_return)(DBusMessage*);
	DBusMessage* (*message_new_error)(DBusMessage*, const char*, const char*);
	DBusMessage* (*message_new_signal)(const char*, const char*, const char*);
	void (*message_unref)(DBusMessage*);
	int (*message_get_type)(DBusMessage*);
	const char* (*message_get_interface)(DBusMessage*);
	const char* (*message_get_member)(DBusMessage*);
	const char* (*message_get_path)(DBusMessage*);
	dbus_bool_t (*iter_init)(DBusMessage*, DBusMessageIter*);
	void (*iter_init_append)(DBusMessage*, DBusMessageIter*);
	dbus_bool_t (*iter_append_basic)(DBusMessageIter*, int, const void*);
	dbus_bool_t (*iter_append_fixed_array)(DBusMessageIter*, int, const void*, int);
	dbus_bool_t (*iter_open_container)(DBusMessageIter*, int, const char*, DBusMessageIter*);
	dbus_bool_t (*iter_close_container)(DBusMessageIter*, DBusMessageIter*);
	int (*iter_get_arg_type)(DBusMessageIter*);
	void (*iter_get_basic)(DBusMessageIter*, void*);
	dbus_bool_t (*iter_next)(DBusMessageIter*);
	void (*iter_recurse)(DBusMessageIter*, DBusMessageIter*);

	DBusConnection* bus;
	rvn_pevent_handler_t handler;
	rvn_ptray_t* trays;
	uint32_t next_index;
} g_sni;

#define RVN_SNI_ROOT "/org/raven/tray"
#define RVN_SNI_WATCHER "org.kde.StatusNotifierWatcher"
#define RVN_SNI_ITEM "org.kde.StatusNotifierItem"
#define RVN_SNI_MENU "com.canonical.dbusmenu"
#define RVN_SNI_PROPERTIES "org.freedesktop.DBus.Properties"

/* An entry of a menu, flattened: entry 0 is the root, the others in order, each knows its children. */
typedef struct rvn_sni_entry_t {
	int32_t parent;
	Rid command;
	char* label;
	int separator, enabled, checkable, checked, submenu;
	int32_t* children;
	uint32_t child_count, child_cap;
} rvn_sni_entry_t;

struct rvn_ptray_t {
	void* user;
	uint32_t index;
	char path[64];
	char menu_path[80];
	char* tooltip;
	/** The picture as the protocol takes it: ARGB32, in network byte order. */
	uint8_t* argb;
	int width, height;
	rvn_sni_entry_t* entries;
	uint32_t entry_count, entry_cap;
	uint32_t revision;
	struct rvn_ptray_t* next;
};

/* ---------------------------------------------------------------------------
 *  The connection
 * ------------------------------------------------------------------------- */

static int rvn_sni_message(DBusConnection* connection, DBusMessage* message, void* data);
static int rvn_sni_filter(DBusConnection* connection, DBusMessage* message, void* data);

static int rvn_sni_connect(void) {
	static const char* const names[] = { "libdbus-1.so.3", "libdbus-1.so" };
	static const DBusObjectPathVTable vtable = { NULL, rvn_sni_message, NULL, NULL, NULL, NULL };
	DBusError error;
	size_t i;
	if (g_sni.bus) return R_TRUE;
	if (g_sni.failed) return R_FALSE;
	g_sni.failed = R_TRUE;
	for (i = 0; i < RVN_COUNT_OF(names) && !g_sni.library; i++) g_sni.library = dlopen(names[i], RTLD_NOW | RTLD_LOCAL);
	if (!g_sni.library) return R_FALSE;
#define LOAD(field, symbol) \
	do { \
		*(void**)(&g_sni.field) = dlsym(g_sni.library, symbol); \
		if (!g_sni.field) return R_FALSE; \
	} while (0)
	LOAD(error_init, "dbus_error_init");
	LOAD(error_free, "dbus_error_free");
	LOAD(bus_get_private, "dbus_bus_get_private");
	LOAD(bus_get_unique_name, "dbus_bus_get_unique_name");
	LOAD(bus_add_match, "dbus_bus_add_match");
	LOAD(bus_name_has_owner, "dbus_bus_name_has_owner");
	LOAD(connection_set_exit_on_disconnect, "dbus_connection_set_exit_on_disconnect");
	LOAD(connection_close, "dbus_connection_close");
	LOAD(connection_unref, "dbus_connection_unref");
	LOAD(connection_register_fallback, "dbus_connection_register_fallback");
	LOAD(connection_add_filter, "dbus_connection_add_filter");
	LOAD(connection_read_write, "dbus_connection_read_write");
	LOAD(connection_dispatch, "dbus_connection_dispatch");
	LOAD(connection_send, "dbus_connection_send");
	LOAD(connection_flush, "dbus_connection_flush");
	LOAD(connection_send_with_reply_and_block, "dbus_connection_send_with_reply_and_block");
	LOAD(connection_get_unix_fd, "dbus_connection_get_unix_fd");
	LOAD(message_new_method_call, "dbus_message_new_method_call");
	LOAD(message_new_method_return, "dbus_message_new_method_return");
	LOAD(message_new_error, "dbus_message_new_error");
	LOAD(message_new_signal, "dbus_message_new_signal");
	LOAD(message_unref, "dbus_message_unref");
	LOAD(message_get_type, "dbus_message_get_type");
	LOAD(message_get_interface, "dbus_message_get_interface");
	LOAD(message_get_member, "dbus_message_get_member");
	LOAD(message_get_path, "dbus_message_get_path");
	LOAD(iter_init, "dbus_message_iter_init");
	LOAD(iter_init_append, "dbus_message_iter_init_append");
	LOAD(iter_append_basic, "dbus_message_iter_append_basic");
	LOAD(iter_append_fixed_array, "dbus_message_iter_append_fixed_array");
	LOAD(iter_open_container, "dbus_message_iter_open_container");
	LOAD(iter_close_container, "dbus_message_iter_close_container");
	LOAD(iter_get_arg_type, "dbus_message_iter_get_arg_type");
	LOAD(iter_get_basic, "dbus_message_iter_get_basic");
	LOAD(iter_next, "dbus_message_iter_next");
	LOAD(iter_recurse, "dbus_message_iter_recurse");
#undef LOAD
	g_sni.error_init(&error);
	g_sni.bus = g_sni.bus_get_private(DBUS_BUS_SESSION, &error);
	g_sni.error_free(&error);
	if (!g_sni.bus) return R_FALSE;
	/* a session bus that goes away must not end the application */
	g_sni.connection_set_exit_on_disconnect(g_sni.bus, 0);
	g_sni.connection_register_fallback(g_sni.bus, RVN_SNI_ROOT, &vtable, NULL);
	/* a watcher that starts (a panel that restarts) is told about the icons again */
	g_sni.connection_add_filter(g_sni.bus, rvn_sni_filter, NULL, NULL);
	g_sni.error_init(&error);
	g_sni.bus_add_match(g_sni.bus, "type='signal',sender='org.freedesktop.DBus',interface='org.freedesktop.DBus',"
		"member='NameOwnerChanged',arg0='" RVN_SNI_WATCHER "'", &error);
	g_sni.error_free(&error);
	g_sni.failed = R_FALSE;
	return R_TRUE;
}

/* Tells the watcher about an icon (by its path: the watcher takes our unique name with it). */
static void rvn_sni_register(rvn_ptray_t* tray) {
	DBusMessage* call = g_sni.message_new_method_call(RVN_SNI_WATCHER, "/StatusNotifierWatcher", RVN_SNI_WATCHER,
		"RegisterStatusNotifierItem");
	DBusMessage* reply;
	DBusMessageIter args;
	DBusError error;
	const char* path = tray->path;
	if (!call) return;
	g_sni.iter_init_append(call, &args);
	g_sni.iter_append_basic(&args, 's', &path);
	g_sni.error_init(&error);
	reply = g_sni.connection_send_with_reply_and_block(g_sni.bus, call, 500, &error);
	g_sni.error_free(&error);
	g_sni.message_unref(call);
	if (reply) g_sni.message_unref(reply);
}

static void rvn_sni_signal(const char* path, const char* iface, const char* member) {
	DBusMessage* signal = g_sni.message_new_signal(path, iface, member);
	if (!signal) return;
	g_sni.connection_send(g_sni.bus, signal, NULL);
	g_sni.message_unref(signal);
	g_sni.connection_flush(g_sni.bus);
}

/* ---------------------------------------------------------------------------
 *  Writing values
 * ------------------------------------------------------------------------- */

static void rvn_sni_variant(DBusMessageIter* iter, int type, const void* value) {
	DBusMessageIter variant;
	char signature[2];
	signature[0] = (char)type;
	signature[1] = 0;
	g_sni.iter_open_container(iter, 'v', signature, &variant);
	g_sni.iter_append_basic(&variant, type, value);
	g_sni.iter_close_container(iter, &variant);
}

/* a(iiay): the picture, or no picture. */
static void rvn_sni_pixmaps(DBusMessageIter* iter, const rvn_ptray_t* tray) {
	DBusMessageIter array, item, bytes;
	g_sni.iter_open_container(iter, 'a', "(iiay)", &array);
	if (tray && tray->argb) {
		int32_t width = tray->width, height = tray->height;
		const uint8_t* data = tray->argb;
		g_sni.iter_open_container(&array, 'r', NULL, &item);
		g_sni.iter_append_basic(&item, 'i', &width);
		g_sni.iter_append_basic(&item, 'i', &height);
		g_sni.iter_open_container(&item, 'a', "y", &bytes);
		g_sni.iter_append_fixed_array(&bytes, 'y', &data, width * height * 4);
		g_sni.iter_close_container(&item, &bytes);
		g_sni.iter_close_container(&array, &item);
	}
	g_sni.iter_close_container(iter, &array);
}

static const char* rvn_sni_app_id(void) {
	const char* id = getenv("RAVEN_APP_ID");
#if defined(__GLIBC__)
	if (!id || !id[0]) id = program_invocation_short_name;
#endif
	return id && id[0] ? id : "raven";
}

static const char* rvn_sni_title(const rvn_ptray_t* tray) {
	return tray->tooltip && tray->tooltip[0] ? tray->tooltip : rvn_sni_app_id();
}

/* One property of org.kde.StatusNotifierItem, as a variant. R_FALSE: there is no such property. */
static int rvn_sni_item_property(DBusMessageIter* iter, const rvn_ptray_t* tray, const char* name) {
	static const char* const empty = "";
	DBusMessageIter variant;
	if (strcmp(name, "Category") == 0) {
		const char* value = "ApplicationStatus";
		rvn_sni_variant(iter, 's', &value);
	} else if (strcmp(name, "Id") == 0) {
		const char* value = rvn_sni_app_id();
		rvn_sni_variant(iter, 's', &value);
	} else if (strcmp(name, "Title") == 0) {
		const char* value = rvn_sni_title(tray);
		rvn_sni_variant(iter, 's', &value);
	} else if (strcmp(name, "Status") == 0) {
		const char* value = "Active";
		rvn_sni_variant(iter, 's', &value);
	} else if (strcmp(name, "WindowId") == 0) {
		int32_t value = 0;
		rvn_sni_variant(iter, 'i', &value);
	} else if (strcmp(name, "IconName") == 0 || strcmp(name, "OverlayIconName") == 0 || strcmp(name, "AttentionIconName") == 0 ||
		strcmp(name, "AttentionMovieName") == 0 || strcmp(name, "IconThemePath") == 0) {
		rvn_sni_variant(iter, 's', &empty);
	} else if (strcmp(name, "IconPixmap") == 0 || strcmp(name, "OverlayIconPixmap") == 0 || strcmp(name, "AttentionIconPixmap") == 0) {
		g_sni.iter_open_container(iter, 'v', "a(iiay)", &variant);
		rvn_sni_pixmaps(&variant, strcmp(name, "IconPixmap") == 0 ? tray : NULL);
		g_sni.iter_close_container(iter, &variant);
	} else if (strcmp(name, "ToolTip") == 0) {
		/* (s icon name, a(iiay) icon, s title, s text) */
		DBusMessageIter tip;
		const char* title = rvn_sni_title(tray);
		g_sni.iter_open_container(iter, 'v', "(sa(iiay)ss)", &variant);
		g_sni.iter_open_container(&variant, 'r', NULL, &tip);
		g_sni.iter_append_basic(&tip, 's', &empty);
		rvn_sni_pixmaps(&tip, NULL);
		g_sni.iter_append_basic(&tip, 's', &title);
		g_sni.iter_append_basic(&tip, 's', &empty);
		g_sni.iter_close_container(&variant, &tip);
		g_sni.iter_close_container(iter, &variant);
	} else if (strcmp(name, "ItemIsMenu") == 0) {
		/* (a click activates; the menu is for the right button) */
		dbus_bool_t value = 0;
		rvn_sni_variant(iter, 'b', &value);
	} else if (strcmp(name, "Menu") == 0) {
		/* the convention of libappindicator for "no menu": the panel then calls ContextMenu */
		const char* value = tray->entry_count > 1 ? tray->menu_path : "/NO_DBUSMENU";
		rvn_sni_variant(iter, 'o', &value);
	} else {
		return R_FALSE;
	}
	return R_TRUE;
}

static int rvn_sni_menu_property(DBusMessageIter* iter, const char* name) {
	DBusMessageIter variant, array;
	if (strcmp(name, "Version") == 0) {
		uint32_t value = 3;
		rvn_sni_variant(iter, 'u', &value);
	} else if (strcmp(name, "TextDirection") == 0) {
		const char* value = "ltr";
		rvn_sni_variant(iter, 's', &value);
	} else if (strcmp(name, "Status") == 0) {
		const char* value = "normal";
		rvn_sni_variant(iter, 's', &value);
	} else if (strcmp(name, "IconThemePath") == 0) {
		g_sni.iter_open_container(iter, 'v', "as", &variant);
		g_sni.iter_open_container(&variant, 'a', "s", &array);
		g_sni.iter_close_container(&variant, &array);
		g_sni.iter_close_container(iter, &variant);
	} else {
		return R_FALSE;
	}
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  The menu (com.canonical.dbusmenu)
 * ------------------------------------------------------------------------- */

static void rvn_sni_free_entries(rvn_ptray_t* tray) {
	uint32_t i;
	for (i = 0; i < tray->entry_count; i++) {
		rvn_free(tray->entries[i].label);
		rvn_free(tray->entries[i].children);
	}
	rvn_free(tray->entries);
	tray->entries = NULL;
	tray->entry_count = tray->entry_cap = 0;
}

static int32_t rvn_sni_add_entry(rvn_ptray_t* tray, int32_t parent) {
	rvn_sni_entry_t* entry;
	int32_t index = (int32_t)tray->entry_count;
	RVN_ARRAY_RESERVE(tray->entries, tray->entry_cap, tray->entry_count + 1);
	entry = &tray->entries[tray->entry_count++];
	memset(entry, 0, sizeof(*entry));
	entry->parent = parent;
	entry->enabled = R_TRUE;
	if (parent >= 0) {
		rvn_sni_entry_t* holder = &tray->entries[parent];
		RVN_ARRAY_RESERVE(holder->children, holder->child_cap, holder->child_count + 1);
		holder->children[holder->child_count++] = index;
	}
	return index;
}

/* The label as dbusmenu reads it: "_" marks the access key, "__" is an underscore. */
static char* rvn_sni_label(const char* text) {
	size_t length = 0, i;
	char* label;
	char* out;
	for (i = 0; text && text[i]; i++) length += text[i] == '_' ? 2 : 1;
	label = (char*)rvn_alloc(length + 1);
	out = label;
	for (i = 0; text && text[i]; i++) {
		if (text[i] == '_') *out++ = '_';
		*out++ = text[i];
	}
	*out = 0;
	return label;
}

static void rvn_sni_add_entries(rvn_ptray_t* tray, int32_t parent, const rvn_pmenu_t* items, uint32_t count) {
	uint32_t i;
	for (i = 0; i < count; i++) {
		int32_t index = rvn_sni_add_entry(tray, parent);
		rvn_sni_entry_t* entry = &tray->entries[index];
		entry->command = items[i].id;
		entry->label = rvn_sni_label(items[i].text);
		entry->separator = items[i].separator;
		entry->enabled = items[i].enabled;
		entry->checkable = items[i].checkable;
		entry->checked = items[i].checked;
		entry->submenu = items[i].submenu;
		if (items[i].submenu) rvn_sni_add_entries(tray, index, items[i].items, items[i].count);
	}
}

static int rvn_sni_wanted(DBusMessageIter* names, const char* name) {
	DBusMessageIter it = *names;
	int any = R_FALSE;
	while (g_sni.iter_get_arg_type(&it) == 's') {
		const char* wanted = NULL;
		g_sni.iter_get_basic(&it, &wanted);
		any = R_TRUE;
		if (wanted && strcmp(wanted, name) == 0) return R_TRUE;
		g_sni.iter_next(&it);
	}
	/* (no names: all of them) */
	return !any;
}

/* The properties an entry has, in the order they are given. */
static const char* const g_sni_entry_names[] = { "children-display", "type", "label", "enabled", "visible", "toggle-type", "toggle-state" };

/* One property of an entry as a variant. R_FALSE when the entry does not have it. */
static int rvn_sni_entry_property(DBusMessageIter* iter, const rvn_ptray_t* tray, int32_t index, const char* name) {
	const rvn_sni_entry_t* entry = &tray->entries[index];
	if (strcmp(name, "children-display") == 0) {
		const char* value = "submenu";
		if (index != 0 && !entry->submenu) return R_FALSE;
		rvn_sni_variant(iter, 's', &value);
		return R_TRUE;
	}
	if (index == 0) return R_FALSE;
	if (strcmp(name, "type") == 0) {
		const char* value = "separator";
		if (!entry->separator) return R_FALSE;
		rvn_sni_variant(iter, 's', &value);
	} else if (strcmp(name, "label") == 0) {
		const char* value = entry->label ? entry->label : "";
		if (entry->separator) return R_FALSE;
		rvn_sni_variant(iter, 's', &value);
	} else if (strcmp(name, "enabled") == 0) {
		dbus_bool_t value = entry->enabled ? 1 : 0;
		rvn_sni_variant(iter, 'b', &value);
	} else if (strcmp(name, "visible") == 0) {
		dbus_bool_t value = 1;
		rvn_sni_variant(iter, 'b', &value);
	} else if (strcmp(name, "toggle-type") == 0) {
		const char* value = "checkmark";
		if (!entry->checkable) return R_FALSE;
		rvn_sni_variant(iter, 's', &value);
	} else if (strcmp(name, "toggle-state") == 0) {
		int32_t value = entry->checked ? 1 : 0;
		if (!entry->checkable) return R_FALSE;
		rvn_sni_variant(iter, 'i', &value);
	} else {
		return R_FALSE;
	}
	return R_TRUE;
}

/* a{sv}: the properties of an entry (those named in `names`, all when it names none). */
static void rvn_sni_entry_properties(DBusMessageIter* iter, const rvn_ptray_t* tray, int32_t index, DBusMessageIter* names) {
	DBusMessageIter dict;
	size_t i;
	g_sni.iter_open_container(iter, 'a', "{sv}", &dict);
	for (i = 0; i < RVN_COUNT_OF(g_sni_entry_names); i++) {
		DBusMessageIter entry;
		const char* name = g_sni_entry_names[i];
		/* (an entry is written only once it is known to have the property: a dictionary entry cannot be taken back) */
		if (!rvn_sni_wanted(names, name)) continue;
		if (index == 0 && strcmp(name, "children-display") != 0) continue;
		if (index > 0) {
			const rvn_sni_entry_t* e = &tray->entries[index];
			if (strcmp(name, "children-display") == 0 && !e->submenu) continue;
			if (strcmp(name, "type") == 0 && !e->separator) continue;
			if (strcmp(name, "label") == 0 && e->separator) continue;
			if ((strcmp(name, "toggle-type") == 0 || strcmp(name, "toggle-state") == 0) && !e->checkable) continue;
		}
		g_sni.iter_open_container(&dict, 'e', NULL, &entry);
		g_sni.iter_append_basic(&entry, 's', &name);
		rvn_sni_entry_property(&entry, tray, index, name);
		g_sni.iter_close_container(&dict, &entry);
	}
	g_sni.iter_close_container(iter, &dict);
}

/* (ia{sv}av): an entry and, down to `depth` levels (-1: all), its children. */
static void rvn_sni_layout(DBusMessageIter* iter, const rvn_ptray_t* tray, int32_t index, int32_t depth, DBusMessageIter* names) {
	const rvn_sni_entry_t* entry = &tray->entries[index];
	DBusMessageIter node, children;
	uint32_t i;
	g_sni.iter_open_container(iter, 'r', NULL, &node);
	g_sni.iter_append_basic(&node, 'i', &index);
	rvn_sni_entry_properties(&node, tray, index, names);
	g_sni.iter_open_container(&node, 'a', "v", &children);
	if (depth != 0) {
		for (i = 0; i < entry->child_count; i++) {
			DBusMessageIter variant;
			g_sni.iter_open_container(&children, 'v', "(ia{sv}av)", &variant);
			rvn_sni_layout(&variant, tray, entry->children[i], depth < 0 ? -1 : depth - 1, names);
			g_sni.iter_close_container(&children, &variant);
		}
	}
	g_sni.iter_close_container(&node, &children);
	g_sni.iter_close_container(iter, &node);
}

static void rvn_sni_emit(rvn_pevent_t* event) {
	if (g_sni.handler) g_sni.handler(event);
}

/* An entry the panel says chosen. */
static void rvn_sni_clicked(const rvn_ptray_t* tray, int32_t index) {
	rvn_pevent_t event;
	const rvn_sni_entry_t* entry;
	if (index <= 0 || (uint32_t)index >= tray->entry_count) return;
	entry = &tray->entries[index];
	if (entry->separator || entry->submenu || !entry->enabled) return;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_COMMAND;
	event.command = entry->command;
	rvn_sni_emit(&event);
}

/* ---------------------------------------------------------------------------
 *  The calls of the panel
 * ------------------------------------------------------------------------- */

static const char g_sni_item_xml[] =
	"<node>"
	"<interface name=\"org.freedesktop.DBus.Introspectable\"><method name=\"Introspect\"><arg type=\"s\" direction=\"out\"/></method></interface>"
	"<interface name=\"org.freedesktop.DBus.Properties\">"
	"<method name=\"Get\"><arg type=\"s\" direction=\"in\"/><arg type=\"s\" direction=\"in\"/><arg type=\"v\" direction=\"out\"/></method>"
	"<method name=\"GetAll\"><arg type=\"s\" direction=\"in\"/><arg type=\"a{sv}\" direction=\"out\"/></method>"
	"</interface>"
	"<interface name=\"org.kde.StatusNotifierItem\">"
	"<property name=\"Category\" type=\"s\" access=\"read\"/><property name=\"Id\" type=\"s\" access=\"read\"/>"
	"<property name=\"Title\" type=\"s\" access=\"read\"/><property name=\"Status\" type=\"s\" access=\"read\"/>"
	"<property name=\"WindowId\" type=\"i\" access=\"read\"/><property name=\"IconName\" type=\"s\" access=\"read\"/>"
	"<property name=\"IconPixmap\" type=\"a(iiay)\" access=\"read\"/><property name=\"ToolTip\" type=\"(sa(iiay)ss)\" access=\"read\"/>"
	"<property name=\"ItemIsMenu\" type=\"b\" access=\"read\"/><property name=\"Menu\" type=\"o\" access=\"read\"/>"
	"<method name=\"Activate\"><arg type=\"i\" direction=\"in\"/><arg type=\"i\" direction=\"in\"/></method>"
	"<method name=\"SecondaryActivate\"><arg type=\"i\" direction=\"in\"/><arg type=\"i\" direction=\"in\"/></method>"
	"<method name=\"ContextMenu\"><arg type=\"i\" direction=\"in\"/><arg type=\"i\" direction=\"in\"/></method>"
	"<method name=\"Scroll\"><arg type=\"i\" direction=\"in\"/><arg type=\"s\" direction=\"in\"/></method>"
	"<signal name=\"NewIcon\"/><signal name=\"NewToolTip\"/><signal name=\"NewTitle\"/>"
	"</interface></node>";

static const char g_sni_menu_xml[] =
	"<node>"
	"<interface name=\"org.freedesktop.DBus.Introspectable\"><method name=\"Introspect\"><arg type=\"s\" direction=\"out\"/></method></interface>"
	"<interface name=\"org.freedesktop.DBus.Properties\">"
	"<method name=\"Get\"><arg type=\"s\" direction=\"in\"/><arg type=\"s\" direction=\"in\"/><arg type=\"v\" direction=\"out\"/></method>"
	"<method name=\"GetAll\"><arg type=\"s\" direction=\"in\"/><arg type=\"a{sv}\" direction=\"out\"/></method>"
	"</interface>"
	"<interface name=\"com.canonical.dbusmenu\">"
	"<property name=\"Version\" type=\"u\" access=\"read\"/><property name=\"TextDirection\" type=\"s\" access=\"read\"/>"
	"<property name=\"Status\" type=\"s\" access=\"read\"/><property name=\"IconThemePath\" type=\"as\" access=\"read\"/>"
	"<method name=\"GetLayout\"><arg type=\"i\" direction=\"in\"/><arg type=\"i\" direction=\"in\"/><arg type=\"as\" direction=\"in\"/>"
	"<arg type=\"u\" direction=\"out\"/><arg type=\"(ia{sv}av)\" direction=\"out\"/></method>"
	"<method name=\"GetGroupProperties\"><arg type=\"ai\" direction=\"in\"/><arg type=\"as\" direction=\"in\"/>"
	"<arg type=\"a(ia{sv})\" direction=\"out\"/></method>"
	"<method name=\"GetProperty\"><arg type=\"i\" direction=\"in\"/><arg type=\"s\" direction=\"in\"/><arg type=\"v\" direction=\"out\"/></method>"
	"<method name=\"Event\"><arg type=\"i\" direction=\"in\"/><arg type=\"s\" direction=\"in\"/><arg type=\"v\" direction=\"in\"/>"
	"<arg type=\"u\" direction=\"in\"/></method>"
	"<method name=\"EventGroup\"><arg type=\"a(isvu)\" direction=\"in\"/><arg type=\"ai\" direction=\"out\"/></method>"
	"<method name=\"AboutToShow\"><arg type=\"i\" direction=\"in\"/><arg type=\"b\" direction=\"out\"/></method>"
	"<method name=\"AboutToShowGroup\"><arg type=\"ai\" direction=\"in\"/><arg type=\"ai\" direction=\"out\"/><arg type=\"ai\" direction=\"out\"/></method>"
	"<signal name=\"LayoutUpdated\"><arg type=\"u\"/><arg type=\"i\"/></signal>"
	"<signal name=\"ItemsPropertiesUpdated\"><arg type=\"a(ia{sv})\"/><arg type=\"a(ias)\"/></signal>"
	"</interface></node>";

static rvn_ptray_t* rvn_sni_tray_of(const char* path, int* menu) {
	rvn_ptray_t* tray;
	*menu = R_FALSE;
	if (!path) return NULL;
	for (tray = g_sni.trays; tray; tray = tray->next) {
		if (strcmp(path, tray->path) == 0) return tray;
		if (strcmp(path, tray->menu_path) == 0) {
			*menu = R_TRUE;
			return tray;
		}
	}
	return NULL;
}

/* The arguments of a call, read one after the other. */
static int32_t rvn_sni_arg_int(DBusMessageIter* it) {
	int32_t value = 0;
	if (g_sni.iter_get_arg_type(it) == 'i') g_sni.iter_get_basic(it, &value);
	g_sni.iter_next(it);
	return value;
}

static const char* rvn_sni_arg_string(DBusMessageIter* it) {
	const char* value = "";
	if (g_sni.iter_get_arg_type(it) == 's') g_sni.iter_get_basic(it, &value);
	g_sni.iter_next(it);
	return value ? value : "";
}

static int rvn_sni_reply(DBusMessage* call, DBusMessage* reply) {
	if (!reply) return DBUS_HANDLER_RESULT_HANDLED;
	g_sni.connection_send(g_sni.bus, reply, NULL);
	g_sni.message_unref(reply);
	(void)call;
	return DBUS_HANDLER_RESULT_HANDLED;
}

static int rvn_sni_properties(DBusMessage* message, const rvn_ptray_t* tray, int menu, const char* member) {
	DBusMessageIter args, out, dict;
	DBusMessage* reply;
	const char* iface;
	g_sni.iter_init(message, &args);
	iface = rvn_sni_arg_string(&args);
	if (strcmp(member, "Get") == 0) {
		const char* name = rvn_sni_arg_string(&args);
		reply = g_sni.message_new_method_return(message);
		if (!reply) return DBUS_HANDLER_RESULT_HANDLED;
		g_sni.iter_init_append(reply, &out);
		if (!(menu ? rvn_sni_menu_property(&out, name) : rvn_sni_item_property(&out, tray, name))) {
			g_sni.message_unref(reply);
			reply = g_sni.message_new_error(message, "org.freedesktop.DBus.Error.UnknownProperty", name);
		}
		return rvn_sni_reply(message, reply);
	}
	if (strcmp(member, "GetAll") == 0) {
		static const char* const item_names[] = {
			"Category", "Id", "Title", "Status", "WindowId", "IconName", "IconPixmap", "OverlayIconName",
			"OverlayIconPixmap", "AttentionIconName", "AttentionIconPixmap", "AttentionMovieName", "ToolTip",
			"ItemIsMenu", "Menu"
		};
		static const char* const menu_names[] = { "Version", "TextDirection", "Status", "IconThemePath" };
		const char* const* names = menu ? menu_names : item_names;
		size_t count = menu ? RVN_COUNT_OF(menu_names) : RVN_COUNT_OF(item_names), i;
		RVN_UNUSED(iface);
		reply = g_sni.message_new_method_return(message);
		if (!reply) return DBUS_HANDLER_RESULT_HANDLED;
		g_sni.iter_init_append(reply, &out);
		g_sni.iter_open_container(&out, 'a', "{sv}", &dict);
		for (i = 0; i < count; i++) {
			DBusMessageIter entry;
			g_sni.iter_open_container(&dict, 'e', NULL, &entry);
			g_sni.iter_append_basic(&entry, 's', &names[i]);
			if (menu) rvn_sni_menu_property(&entry, names[i]);
			else rvn_sni_item_property(&entry, tray, names[i]);
			g_sni.iter_close_container(&dict, &entry);
		}
		g_sni.iter_close_container(&out, &dict);
		return rvn_sni_reply(message, reply);
	}
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static int rvn_sni_menu_call(DBusMessage* message, const rvn_ptray_t* tray, const char* member) {
	DBusMessageIter args, out, list, element;
	DBusMessage* reply;
	g_sni.iter_init(message, &args);
	if (strcmp(member, "GetLayout") == 0) {
		int32_t parent = rvn_sni_arg_int(&args), depth = rvn_sni_arg_int(&args);
		DBusMessageIter names;
		uint32_t revision = tray->revision;
		if (g_sni.iter_get_arg_type(&args) == 'a') g_sni.iter_recurse(&args, &names);
		else names = args;
		if (parent < 0 || (uint32_t)parent >= tray->entry_count) parent = 0;
		reply = g_sni.message_new_method_return(message);
		if (!reply) return DBUS_HANDLER_RESULT_HANDLED;
		g_sni.iter_init_append(reply, &out);
		g_sni.iter_append_basic(&out, 'u', &revision);
		rvn_sni_layout(&out, tray, parent, depth, &names);
		return rvn_sni_reply(message, reply);
	}
	if (strcmp(member, "GetGroupProperties") == 0) {
		DBusMessageIter ids, names;
		g_sni.iter_recurse(&args, &ids);
		g_sni.iter_next(&args);
		if (g_sni.iter_get_arg_type(&args) == 'a') g_sni.iter_recurse(&args, &names);
		else names = args;
		reply = g_sni.message_new_method_return(message);
		if (!reply) return DBUS_HANDLER_RESULT_HANDLED;
		g_sni.iter_init_append(reply, &out);
		g_sni.iter_open_container(&out, 'a', "(ia{sv})", &list);
		while (g_sni.iter_get_arg_type(&ids) == 'i') {
			int32_t id = 0;
			g_sni.iter_get_basic(&ids, &id);
			if (id >= 0 && (uint32_t)id < tray->entry_count) {
				g_sni.iter_open_container(&list, 'r', NULL, &element);
				g_sni.iter_append_basic(&element, 'i', &id);
				rvn_sni_entry_properties(&element, tray, id, &names);
				g_sni.iter_close_container(&list, &element);
			}
			g_sni.iter_next(&ids);
		}
		g_sni.iter_close_container(&out, &list);
		return rvn_sni_reply(message, reply);
	}
	if (strcmp(member, "GetProperty") == 0) {
		int32_t id = rvn_sni_arg_int(&args);
		const char* name = rvn_sni_arg_string(&args);
		reply = NULL;
		if (id >= 0 && (uint32_t)id < tray->entry_count) {
			reply = g_sni.message_new_method_return(message);
			if (reply) {
				g_sni.iter_init_append(reply, &out);
				if (!rvn_sni_entry_property(&out, tray, id, name)) {
					g_sni.message_unref(reply);
					reply = NULL;
				}
			}
		}
		if (!reply) reply = g_sni.message_new_error(message, "org.freedesktop.DBus.Error.InvalidArgs", "no such entry or property");
		return rvn_sni_reply(message, reply);
	}
	if (strcmp(member, "Event") == 0) {
		int32_t id = rvn_sni_arg_int(&args);
		const char* kind = rvn_sni_arg_string(&args);
		/* (answered first: what the entry does may take its time, or open a dialog) */
		rvn_sni_reply(message, g_sni.message_new_method_return(message));
		g_sni.connection_flush(g_sni.bus);
		if (strcmp(kind, "clicked") == 0) rvn_sni_clicked(tray, id);
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	if (strcmp(member, "EventGroup") == 0) {
		DBusMessageIter events;
		int32_t clicked[16];
		int count = 0, i;
		g_sni.iter_recurse(&args, &events);
		while (g_sni.iter_get_arg_type(&events) == 'r' && count < 16) {
			DBusMessageIter event;
			int32_t id;
			const char* kind;
			g_sni.iter_recurse(&events, &event);
			id = rvn_sni_arg_int(&event);
			kind = rvn_sni_arg_string(&event);
			if (strcmp(kind, "clicked") == 0) clicked[count++] = id;
			g_sni.iter_next(&events);
		}
		reply = g_sni.message_new_method_return(message);
		if (reply) {
			g_sni.iter_init_append(reply, &out);
			g_sni.iter_open_container(&out, 'a', "i", &list);
			g_sni.iter_close_container(&out, &list);
		}
		rvn_sni_reply(message, reply);
		g_sni.connection_flush(g_sni.bus);
		for (i = 0; i < count; i++) rvn_sni_clicked(tray, clicked[i]);
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	if (strcmp(member, "AboutToShow") == 0) {
		dbus_bool_t update = 0;
		reply = g_sni.message_new_method_return(message);
		if (reply) {
			g_sni.iter_init_append(reply, &out);
			g_sni.iter_append_basic(&out, 'b', &update);
		}
		return rvn_sni_reply(message, reply);
	}
	if (strcmp(member, "AboutToShowGroup") == 0) {
		reply = g_sni.message_new_method_return(message);
		if (reply) {
			g_sni.iter_init_append(reply, &out);
			g_sni.iter_open_container(&out, 'a', "i", &list);
			g_sni.iter_close_container(&out, &list);
			g_sni.iter_open_container(&out, 'a', "i", &list);
			g_sni.iter_close_container(&out, &list);
		}
		return rvn_sni_reply(message, reply);
	}
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static int rvn_sni_message(DBusConnection* connection, DBusMessage* message, void* data) {
	const char* iface = g_sni.message_get_interface(message);
	const char* member = g_sni.message_get_member(message);
	int menu;
	rvn_ptray_t* tray = rvn_sni_tray_of(g_sni.message_get_path(message), &menu);
	RVN_UNUSED(connection);
	RVN_UNUSED(data);
	if (!tray || !member || g_sni.message_get_type(message) != DBUS_MESSAGE_TYPE_METHOD_CALL) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	if (iface && strcmp(iface, "org.freedesktop.DBus.Introspectable") == 0 && strcmp(member, "Introspect") == 0) {
		DBusMessage* reply = g_sni.message_new_method_return(message);
		const char* xml = menu ? g_sni_menu_xml : g_sni_item_xml;
		DBusMessageIter out;
		if (reply) {
			g_sni.iter_init_append(reply, &out);
			g_sni.iter_append_basic(&out, 's', &xml);
		}
		return rvn_sni_reply(message, reply);
	}
	if (iface && strcmp(iface, RVN_SNI_PROPERTIES) == 0) return rvn_sni_properties(message, tray, menu, member);
	if (menu) return rvn_sni_menu_call(message, tray, member);
	if (!iface || strcmp(iface, RVN_SNI_ITEM) == 0) {
		rvn_pevent_t event;
		int button;
		if (strcmp(member, "Activate") == 0) button = (int)R_MOUSE_LEFT;
		else if (strcmp(member, "SecondaryActivate") == 0) button = (int)R_MOUSE_MIDDLE;
		else if (strcmp(member, "ContextMenu") == 0) button = (int)R_MOUSE_RIGHT;
		else if (strcmp(member, "Scroll") == 0) button = -1;
		else return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		rvn_sni_reply(message, g_sni.message_new_method_return(message));
		g_sni.connection_flush(g_sni.bus);
		if (button < 0) return DBUS_HANDLER_RESULT_HANDLED;
		memset(&event, 0, sizeof(event));
		event.type = RVN_PEVENT_TRAY;
		event.user = tray->user;
		event.button = button;
		rvn_sni_emit(&event);
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/* A watcher that starts: the icons are registered with it. */
static int rvn_sni_filter(DBusConnection* connection, DBusMessage* message, void* data) {
	const char* member = g_sni.message_get_member(message);
	DBusMessageIter args;
	RVN_UNUSED(connection);
	RVN_UNUSED(data);
	if (g_sni.message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL || !member || strcmp(member, "NameOwnerChanged") != 0) {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}
	if (g_sni.iter_init(message, &args)) {
		const char* name = rvn_sni_arg_string(&args);
		const char* before = rvn_sni_arg_string(&args);
		const char* after = rvn_sni_arg_string(&args);
		RVN_UNUSED(before);
		if (strcmp(name, RVN_SNI_WATCHER) == 0 && after[0]) {
			rvn_ptray_t* tray;
			for (tray = g_sni.trays; tray; tray = tray->next) rvn_sni_register(tray);
		}
	}
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/* ---------------------------------------------------------------------------
 *  The entries of the platform
 * ------------------------------------------------------------------------- */

void rvn_sni_set_handler(rvn_pevent_handler_t handler) {
	g_sni.handler = handler;
}

int rvn_sni_supported(void) {
	DBusError error;
	int owned;
	if (!rvn_sni_connect()) return R_FALSE;
	g_sni.error_init(&error);
	owned = g_sni.bus_name_has_owner(g_sni.bus, RVN_SNI_WATCHER, &error) ? R_TRUE : R_FALSE;
	g_sni.error_free(&error);
	return owned;
}

rvn_ptray_t* rvn_sni_create(void* user) {
	rvn_ptray_t* tray;
	if (!rvn_sni_connect()) return NULL;
	tray = (rvn_ptray_t*)rvn_calloc(sizeof(rvn_ptray_t));
	tray->user = user;
	tray->index = ++g_sni.next_index;
	snprintf(tray->path, sizeof(tray->path), RVN_SNI_ROOT "/%u", (unsigned)tray->index);
	snprintf(tray->menu_path, sizeof(tray->menu_path), "%s/menu", tray->path);
	/* the root of the menu, always there */
	rvn_sni_add_entry(tray, -1);
	tray->revision = 1;
	tray->next = g_sni.trays;
	g_sni.trays = tray;
	/* (no watcher yet: the icon is registered when one starts) */
	rvn_sni_register(tray);
	return tray;
}

void rvn_sni_destroy(rvn_ptray_t* tray) {
	rvn_ptray_t** link;
	for (link = &g_sni.trays; *link; link = &(*link)->next) {
		if (*link == tray) {
			*link = tray->next;
			break;
		}
	}
	/* (the watcher sees the object gone when it next asks; the panels drop it with an empty status) */
	rvn_sni_free_entries(tray);
	rvn_free(tray->tooltip);
	rvn_free(tray->argb);
	rvn_free(tray);
}

void rvn_sni_set_icon(rvn_ptray_t* tray, const uint8_t* rgba, int width, int height) {
	size_t i, count = (size_t)width * (size_t)height;
	rvn_free(tray->argb);
	tray->argb = (uint8_t*)rvn_alloc(count * 4);
	tray->width = width;
	tray->height = height;
	for (i = 0; i < count; i++) {
		/* ARGB32 in network byte order: A, R, G, B */
		tray->argb[i * 4 + 0] = rgba[i * 4 + 3];
		tray->argb[i * 4 + 1] = rgba[i * 4 + 0];
		tray->argb[i * 4 + 2] = rgba[i * 4 + 1];
		tray->argb[i * 4 + 3] = rgba[i * 4 + 2];
	}
	rvn_sni_signal(tray->path, RVN_SNI_ITEM, "NewIcon");
}

void rvn_sni_set_tooltip(rvn_ptray_t* tray, const char* text) {
	rvn_str_set(&tray->tooltip, text);
	rvn_sni_signal(tray->path, RVN_SNI_ITEM, "NewToolTip");
	rvn_sni_signal(tray->path, RVN_SNI_ITEM, "NewTitle");
}

void rvn_sni_set_menu(rvn_ptray_t* tray, const rvn_pmenu_t* items, uint32_t count) {
	DBusMessage* signal;
	DBusMessageIter args;
	int had = tray->entry_count > 1;
	int32_t parent = 0;
	rvn_sni_free_entries(tray);
	rvn_sni_add_entry(tray, -1);
	rvn_sni_add_entries(tray, 0, items, count);
	tray->revision++;
	signal = g_sni.message_new_signal(tray->menu_path, RVN_SNI_MENU, "LayoutUpdated");
	if (signal) {
		g_sni.iter_init_append(signal, &args);
		g_sni.iter_append_basic(&args, 'u', &tray->revision);
		g_sni.iter_append_basic(&args, 'i', &parent);
		g_sni.connection_send(g_sni.bus, signal, NULL);
		g_sni.message_unref(signal);
	}
	/* (a menu that comes or goes: the Menu property changed, which the panels read again with the icon) */
	if (had != (tray->entry_count > 1)) rvn_sni_signal(tray->path, RVN_SNI_ITEM, "NewIcon");
	g_sni.connection_flush(g_sni.bus);
}

int rvn_sni_fd(void) {
	int fd = -1;
	if (!g_sni.bus || !g_sni.trays || !g_sni.connection_get_unix_fd(g_sni.bus, &fd)) return -1;
	return fd;
}

void rvn_sni_dispatch(void) {
	int guard = 0;
	if (!g_sni.bus) return;
	g_sni.connection_read_write(g_sni.bus, 0);
	while (g_sni.connection_dispatch(g_sni.bus) == DBUS_DISPATCH_DATA_REMAINS && guard++ < 1000) {}
	g_sni.connection_flush(g_sni.bus);
}

void rvn_sni_shutdown(void) {
	while (g_sni.trays) rvn_sni_destroy(g_sni.trays);
	if (g_sni.bus) {
		g_sni.connection_close(g_sni.bus);
		g_sni.connection_unref(g_sni.bus);
		g_sni.bus = NULL;
	}
	/* (libdbus stays loaded: unloading it is not safe) */
	g_sni.failed = R_FALSE;
	g_sni.next_index = 0;
	g_sni.handler = NULL;
}

#endif
