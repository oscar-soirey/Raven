/**
 * Raven - platform/atspi.c
 * The windows of Raven for the screen readers of Linux and the BSDs (Orca):
 * AT-SPI 2, spoken over D-Bus.
 *
 * libdbus is loaded when it is needed (dlopen "libdbus-1.so.3", present on
 * every desktop): Raven links with nothing more. The bridge connects to the
 * accessibility bus only when the desktop says that assistive technologies
 * are on (org.a11y.Status IsEnabled / ScreenReaderEnabled), or when
 * AT_SPI_BUS_ADDRESS is set; otherwise it costs nothing.
 *
 * Every node of core/accessibility.h is an object:
 *     /org/a11y/atspi/accessible/root        the application (children: the windows)
 *     /org/a11y/atspi/accessible/<hex id>    a window, a widget, an item
 * with the interfaces Accessible, Component, Action, Value, Text and
 * EditableText where they apply. The focus, the changes of value and of
 * structure, and the announcements are sent as AT-SPI events.
 */

#if !defined(_WIN32) && !defined(__APPLE__)

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE /* popen, pclose */
#endif

#include "platform.h"
#include "atspi.h"
#include "../core/accessibility.h"
#include "../core/context.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#define T_INVALID 0
#define T_BYTE 'y'
#define T_BOOLEAN 'b'
#define T_INT16 'n'
#define T_INT32 'i'
#define T_UINT32 'u'
#define T_DOUBLE 'd'
#define T_STRING 's'
#define T_OBJECT_PATH 'o'
#define T_ARRAY 'a'
#define T_VARIANT 'v'
#define T_STRUCT 'r'
#define T_DICT_ENTRY 'e'

static struct {
	void* library;
	void (*error_init)(DBusError*);
	void (*error_free)(DBusError*);
	dbus_bool_t (*error_is_set)(const DBusError*);
	DBusConnection* (*bus_get_private)(int, DBusError*);
	DBusConnection* (*connection_open_private)(const char*, DBusError*);
	dbus_bool_t (*bus_register)(DBusConnection*, DBusError*);
	const char* (*bus_get_unique_name)(DBusConnection*);
	void (*connection_set_exit_on_disconnect)(DBusConnection*, dbus_bool_t);
	void (*connection_close)(DBusConnection*);
	void (*connection_unref)(DBusConnection*);
	dbus_bool_t (*connection_register_fallback)(DBusConnection*, const char*, const DBusObjectPathVTable*, void*);
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
	dbus_bool_t (*iter_open_container)(DBusMessageIter*, int, const char*, DBusMessageIter*);
	dbus_bool_t (*iter_close_container)(DBusMessageIter*, DBusMessageIter*);
	int (*iter_get_arg_type)(DBusMessageIter*);
	void (*iter_get_basic)(DBusMessageIter*, void*);
	dbus_bool_t (*iter_next)(DBusMessageIter*);
	void (*iter_recurse)(DBusMessageIter*, DBusMessageIter*);

	DBusConnection* bus;
	char unique[128];
	char parent_name[128];
	char parent_path[256];
	int app_id;
	int enabled;
	int failed;
} g_atspi;

#define ROOT_PATH "/org/a11y/atspi/accessible/root"
#define NODE_PREFIX "/org/a11y/atspi/accessible/"
#define NULL_PATH "/org/a11y/atspi/null"

static int rvn_atspi_load(void) {
	static const char* const names[] = { "libdbus-1.so.3", "libdbus-1.so" };
	size_t i;
	if (g_atspi.library) return R_TRUE;
	for (i = 0; i < RVN_COUNT_OF(names) && !g_atspi.library; i++) g_atspi.library = dlopen(names[i], RTLD_NOW | RTLD_LOCAL);
	if (!g_atspi.library) return R_FALSE;
#define LOAD(field, symbol) *(void**)(&g_atspi.field) = dlsym(g_atspi.library, symbol); if (!g_atspi.field) goto missing
	LOAD(error_init, "dbus_error_init");
	LOAD(error_free, "dbus_error_free");
	LOAD(error_is_set, "dbus_error_is_set");
	LOAD(bus_get_private, "dbus_bus_get_private");
	LOAD(connection_open_private, "dbus_connection_open_private");
	LOAD(bus_register, "dbus_bus_register");
	LOAD(bus_get_unique_name, "dbus_bus_get_unique_name");
	LOAD(connection_set_exit_on_disconnect, "dbus_connection_set_exit_on_disconnect");
	LOAD(connection_close, "dbus_connection_close");
	LOAD(connection_unref, "dbus_connection_unref");
	LOAD(connection_register_fallback, "dbus_connection_register_fallback");
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
	LOAD(iter_open_container, "dbus_message_iter_open_container");
	LOAD(iter_close_container, "dbus_message_iter_close_container");
	LOAD(iter_get_arg_type, "dbus_message_iter_get_arg_type");
	LOAD(iter_get_basic, "dbus_message_iter_get_basic");
	LOAD(iter_next, "dbus_message_iter_next");
	LOAD(iter_recurse, "dbus_message_iter_recurse");
#undef LOAD
	return R_TRUE;
missing:
	dlclose(g_atspi.library);
	g_atspi.library = NULL;
	return R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Writing values
 * ------------------------------------------------------------------------- */

static void put_string(DBusMessageIter* it, const char* s) {
	if (!s) s = "";
	g_atspi.iter_append_basic(it, T_STRING, &s);
}
static void put_path(DBusMessageIter* it, const char* s) { g_atspi.iter_append_basic(it, T_OBJECT_PATH, &s); }
static void put_i32(DBusMessageIter* it, int32_t v) { g_atspi.iter_append_basic(it, T_INT32, &v); }
static void put_u32(DBusMessageIter* it, uint32_t v) { g_atspi.iter_append_basic(it, T_UINT32, &v); }
static void put_bool(DBusMessageIter* it, int v) { dbus_bool_t b = v ? 1 : 0; g_atspi.iter_append_basic(it, T_BOOLEAN, &b); }
static void put_double(DBusMessageIter* it, double v) { g_atspi.iter_append_basic(it, T_DOUBLE, &v); }

static void node_path(rvn_a11y_id_t id, char* out, size_t cap) {
	if (!id) snprintf(out, cap, ROOT_PATH);
	else snprintf(out, cap, NODE_PREFIX "%016llx", (unsigned long long)id);
}

/* (so): a reference to an object of ours */
static void put_ref(DBusMessageIter* it, rvn_a11y_id_t id) {
	DBusMessageIter s;
	char path[96];
	node_path(id, path, sizeof(path));
	g_atspi.iter_open_container(it, T_STRUCT, NULL, &s);
	put_string(&s, g_atspi.unique);
	put_path(&s, path);
	g_atspi.iter_close_container(it, &s);
}

static void put_null_ref(DBusMessageIter* it) {
	DBusMessageIter s;
	g_atspi.iter_open_container(it, T_STRUCT, NULL, &s);
	put_string(&s, "");
	put_path(&s, NULL_PATH);
	g_atspi.iter_close_container(it, &s);
}

static void put_parent_ref(DBusMessageIter* it) {
	DBusMessageIter s;
	g_atspi.iter_open_container(it, T_STRUCT, NULL, &s);
	put_string(&s, g_atspi.parent_name);
	put_path(&s, g_atspi.parent_path[0] ? g_atspi.parent_path : NULL_PATH);
	g_atspi.iter_close_container(it, &s);
}

/* ---------------------------------------------------------------------------
 *  Nodes
 * ------------------------------------------------------------------------- */

/* The root (the application) is 0. Returns R_FALSE for a path of nobody. */
static int parse_path(const char* path, rvn_a11y_id_t* id) {
	if (!path) return R_FALSE;
	if (strcmp(path, ROOT_PATH) == 0) {
		*id = 0;
		return R_TRUE;
	}
	if (strncmp(path, NODE_PREFIX, strlen(NODE_PREFIX)) != 0) return R_FALSE;
	*id = (rvn_a11y_id_t)strtoull(path + strlen(NODE_PREFIX), NULL, 16);
	return *id && rvn_a11y_exists(*id);
}

static int app_child_count(void) {
	uint32_t i;
	int count = 0;
	for (i = 0; i < g_rvn.windows.used; i++) if (rvn_pool_at(&g_rvn.windows, i)) count++;
	return count;
}

static rvn_a11y_id_t app_child(int index) {
	uint32_t i;
	for (i = 0; i < g_rvn.windows.used; i++) {
		const Rid* window = (const Rid*)rvn_pool_at(&g_rvn.windows, i);
		if (!window) continue;
		if (index-- == 0) return (rvn_a11y_id_t)*window;
	}
	return 0;
}

static int child_count(rvn_a11y_id_t id) { return id ? rvn_a11y_child_count(id) : app_child_count(); }
static rvn_a11y_id_t child_at(rvn_a11y_id_t id, int index) { return id ? rvn_a11y_child(id, index) : app_child(index); }

static int index_in_parent(rvn_a11y_id_t id) {
	int i, n;
	if (!id) return -1;
	if (!rvn_a11y_parent(id)) {
		n = app_child_count();
		for (i = 0; i < n; i++) if (app_child(i) == id) return i;
		return -1;
	}
	return rvn_a11y_index_in_parent(id);
}

static uint32_t atspi_role(RAccessibleRole role, uint32_t states) {
	switch (role) {
		case R_ROLE_WINDOW: return 23;         /* FRAME */
		case R_ROLE_GROUP: return 39;          /* PANEL */
		case R_ROLE_TEXT: return 29;           /* LABEL */
		case R_ROLE_HEADING: return 83;
		case R_ROLE_LINK: return 88;
		case R_ROLE_BUTTON: return 43;         /* PUSH_BUTTON */
		case R_ROLE_TOGGLE_BUTTON: return 62;
		case R_ROLE_CHECKBOX: return 7;
		case R_ROLE_RADIO_BUTTON: return 44;
		case R_ROLE_SLIDER: return 51;
		case R_ROLE_PROGRESS_BAR: return 42;
		case R_ROLE_SPIN_BUTTON: return 52;
		case R_ROLE_TEXT_FIELD: return (states & RVN_A11Y_PASSWORD) ? 40 : 79; /* PASSWORD_TEXT, ENTRY */
		case R_ROLE_DOCUMENT: return 94;       /* DOCUMENT_TEXT */
		case R_ROLE_COMBO_BOX: return 11;
		case R_ROLE_LIST: return 98;           /* LIST_BOX */
		case R_ROLE_LIST_ITEM: return 32;
		case R_ROLE_TREE: return 65;
		case R_ROLE_TREE_ITEM: return 91;
		case R_ROLE_TABLE: return 55;
		case R_ROLE_MENU_BAR: return 34;
		case R_ROLE_MENU: return 33;
		case R_ROLE_MENU_ITEM: return (states & RVN_A11Y_CHECKABLE) ? 8 : 35;
		case R_ROLE_TAB_LIST: return 38;
		case R_ROLE_TAB: return 37;
		case R_ROLE_TAB_PANEL: return 39;
		case R_ROLE_TOOLBAR: return 63;
		case R_ROLE_STATUS_BAR: return 54;
		case R_ROLE_DIALOG: return 16;
		case R_ROLE_ALERT: return 2;
		case R_ROLE_IMAGE: return 27;
		case R_ROLE_CANVAS: return 6;
		case R_ROLE_SEPARATOR: return 50;
		case R_ROLE_SCROLL_BAR: return 48;
		case R_ROLE_PANE: return 39;
		default: return 67;                    /* UNKNOWN */
	}
}

static void put_states(DBusMessageIter* it, rvn_a11y_id_t id, const rvn_a11y_node_t* node) {
	DBusMessageIter array;
	uint64_t bits = 0;
#define S(n) (bits |= (uint64_t)1 << (n))
	if (!id) {
		/* the application */
	} else {
		uint32_t st = node->states;
		S(30); S(25);                                   /* VISIBLE, SHOWING */
		if (!(st & RVN_A11Y_DISABLED)) { S(8); S(24); } /* ENABLED, SENSITIVE */
		if (st & RVN_A11Y_FOCUSABLE) S(11);
		if (st & RVN_A11Y_FOCUSED) S(12);
		if (st & RVN_A11Y_CHECKABLE) S(41);
		if (st & RVN_A11Y_CHECKED) S(4);
		if (st & RVN_A11Y_EXPANDABLE) { S(9); if (st & RVN_A11Y_EXPANDED) S(10); else S(5); }
		if (st & RVN_A11Y_SELECTABLE) S(22);
		if (st & RVN_A11Y_SELECTED) S(23);
		if (st & RVN_A11Y_EDITABLE) S(7);
		if (st & RVN_A11Y_READONLY) S(43);
		if (st & RVN_A11Y_MODAL) S(16);
		if (st & RVN_A11Y_MULTILINE) S(17); else if (node->role == R_ROLE_TEXT_FIELD) S(26);
		if (node->role == R_ROLE_WINDOW) { S(1); S(21); } /* ACTIVE, RESIZABLE */
		if (node->role == R_ROLE_TEXT || node->role == R_ROLE_DOCUMENT) S(38);
	}
#undef S
	g_atspi.iter_open_container(it, T_ARRAY, "u", &array);
	put_u32(&array, (uint32_t)bits);
	put_u32(&array, (uint32_t)(bits >> 32));
	g_atspi.iter_close_container(it, &array);
}

static int has_action(const rvn_a11y_node_t* node) {
	return (node->states & (RVN_A11Y_INVOKABLE | RVN_A11Y_CHECKABLE | RVN_A11Y_EXPANDABLE)) != 0;
}

static const char* action_name(const rvn_a11y_node_t* node) {
	if (node->role == R_ROLE_CHECKBOX) return "toggle";
	if (node->role == R_ROLE_TAB || node->role == R_ROLE_LIST_ITEM || node->role == R_ROLE_TREE_ITEM) return "activate";
	if (node->states & RVN_A11Y_EXPANDABLE && !(node->states & RVN_A11Y_INVOKABLE)) return "expand or contract";
	return "click";
}

static int has_text(const rvn_a11y_node_t* node) {
	return node->role == R_ROLE_TEXT || node->role == R_ROLE_TEXT_FIELD || node->role == R_ROLE_DOCUMENT || node->role == R_ROLE_HEADING ||
		node->role == R_ROLE_SPIN_BUTTON || node->role == R_ROLE_COMBO_BOX || node->role == R_ROLE_LINK;
}

/* The text of the Text interface: the value of a field, the name of a label. */
static const char* node_text(const rvn_a11y_node_t* node) {
	if (node->role == R_ROLE_TEXT_FIELD || node->role == R_ROLE_DOCUMENT || node->role == R_ROLE_SPIN_BUTTON || node->role == R_ROLE_COMBO_BOX)
		return node->value;
	return node->name;
}

static int utf8_length(const char* s) {
	int n = 0;
	for (; *s; s++) if (((uint8_t)*s & 0xC0) != 0x80) n++;
	return n;
}

/* Byte offset of the character `index` (clamped). */
static int utf8_offset(const char* s, int index) {
	int i = 0, n = 0;
	if (index <= 0) return 0;
	for (; s[i]; i++) {
		if (((uint8_t)s[i] & 0xC0) != 0x80) {
			if (n == index) return i;
			n++;
		}
	}
	return i;
}

/* ---------------------------------------------------------------------------
 *  Properties
 * ------------------------------------------------------------------------- */

/* Writes the value of a property in a variant; R_FALSE when the property is not known. */
static int put_property(DBusMessageIter* it, rvn_a11y_id_t id, const rvn_a11y_node_t* node, const char* iface, const char* name) {
	DBusMessageIter v;
#define VARIANT(sig, body) do { g_atspi.iter_open_container(it, T_VARIANT, sig, &v); body; g_atspi.iter_close_container(it, &v); return R_TRUE; } while (0)
	if (strcmp(iface, "org.a11y.atspi.Accessible") == 0) {
		if (strcmp(name, "Name") == 0) VARIANT("s", put_string(&v, id ? node->name : "Raven application"));
		if (strcmp(name, "Description") == 0) VARIANT("s", put_string(&v, id ? node->description : ""));
		if (strcmp(name, "HelpText") == 0) VARIANT("s", put_string(&v, id ? node->description : ""));
		if (strcmp(name, "Locale") == 0) VARIANT("s", put_string(&v, rvn_a11y_locale()));
		if (strcmp(name, "AccessibleId") == 0) {
			char text[32];
			snprintf(text, sizeof(text), "%llx", (unsigned long long)id);
			VARIANT("s", put_string(&v, text));
		}
		if (strcmp(name, "ChildCount") == 0) VARIANT("i", put_i32(&v, child_count(id)));
		if (strcmp(name, "Parent") == 0) {
			if (!id) VARIANT("(so)", put_parent_ref(&v));
			VARIANT("(so)", put_ref(&v, rvn_a11y_parent(id)));
		}
	} else if (strcmp(iface, "org.a11y.atspi.Application") == 0) {
		if (strcmp(name, "ToolkitName") == 0) VARIANT("s", put_string(&v, "Raven"));
		if (strcmp(name, "Version") == 0) VARIANT("s", put_string(&v, Rget_version()));
		if (strcmp(name, "AtspiVersion") == 0) VARIANT("s", put_string(&v, "2.1"));
		if (strcmp(name, "Id") == 0) VARIANT("i", put_i32(&v, g_atspi.app_id));
	} else if (strcmp(iface, "org.a11y.atspi.Action") == 0) {
		if (strcmp(name, "NActions") == 0) VARIANT("i", put_i32(&v, has_action(node) ? 1 : 0));
	} else if (strcmp(iface, "org.a11y.atspi.Value") == 0) {
		if (strcmp(name, "CurrentValue") == 0) VARIANT("d", put_double(&v, node->number));
		if (strcmp(name, "MinimumValue") == 0) VARIANT("d", put_double(&v, node->minimum));
		if (strcmp(name, "MaximumValue") == 0) VARIANT("d", put_double(&v, node->maximum));
		if (strcmp(name, "MinimumIncrement") == 0) VARIANT("d", put_double(&v, node->step));
		if (strcmp(name, "Text") == 0) VARIANT("s", put_string(&v, node->value));
	} else if (strcmp(iface, "org.a11y.atspi.Text") == 0) {
		if (strcmp(name, "CharacterCount") == 0) VARIANT("i", put_i32(&v, utf8_length(node_text(node))));
		if (strcmp(name, "CaretOffset") == 0) VARIANT("i", put_i32(&v, utf8_length(node_text(node))));
	}
#undef VARIANT
	return R_FALSE;
}

static void interfaces_of(rvn_a11y_id_t id, const rvn_a11y_node_t* node, const char** out, int* count) {
	*count = 0;
	out[(*count)++] = "org.a11y.atspi.Accessible";
	if (!id) {
		out[(*count)++] = "org.a11y.atspi.Application";
		return;
	}
	out[(*count)++] = "org.a11y.atspi.Component";
	if (has_action(node)) out[(*count)++] = "org.a11y.atspi.Action";
	if (node->states & RVN_A11Y_HAS_RANGE) out[(*count)++] = "org.a11y.atspi.Value";
	if (has_text(node)) out[(*count)++] = "org.a11y.atspi.Text";
	if (node->states & RVN_A11Y_EDITABLE) out[(*count)++] = "org.a11y.atspi.EditableText";
}

/* ---------------------------------------------------------------------------
 *  Methods
 * ------------------------------------------------------------------------- */

static int next_i32(DBusMessageIter* it, int32_t* value) {
	if (g_atspi.iter_get_arg_type(it) != T_INT32 && g_atspi.iter_get_arg_type(it) != T_UINT32) return R_FALSE;
	g_atspi.iter_get_basic(it, value);
	g_atspi.iter_next(it);
	return R_TRUE;
}

static int next_string(DBusMessageIter* it, const char** value) {
	if (g_atspi.iter_get_arg_type(it) != T_STRING) return R_FALSE;
	g_atspi.iter_get_basic(it, (void*)value);
	g_atspi.iter_next(it);
	return R_TRUE;
}

static void reply_and_send(DBusMessage* reply) {
	if (!reply) return;
	g_atspi.connection_send(g_atspi.bus, reply, NULL);
	g_atspi.message_unref(reply);
}

/* Screen point -> the deepest node there. */
static rvn_a11y_id_t node_at_point(rvn_a11y_id_t id, int x, int y, uint32_t coords) {
	Rid window = rvn_a11y_window(id);
	int wx = 0, wy = 0;
	float scale = 1.0f, lx, ly;
	if (!window) return 0;
	rvn_a11y_window_origin(id, &wx, &wy, &scale);
	if (coords == 0) { lx = (float)(x - wx) / scale; ly = (float)(y - wy) / scale; }
	else { lx = (float)x / scale; ly = (float)y / scale; }
	return rvn_a11y_hit(window, lx, ly);
}

static void get_extents(rvn_a11y_id_t id, uint32_t coords, int* x, int* y, int* w, int* h) {
	rvn_a11y_screen_rect(id, x, y, w, h);
	if (coords != 0) {
		/* relative to the window */
		int wx, wy;
		float scale;
		rvn_a11y_window_origin(id, &wx, &wy, &scale);
		*x -= wx;
		*y -= wy;
	}
}

static int handle(DBusConnection* connection, DBusMessage* message, void* user) {
	const char* iface = g_atspi.message_get_interface(message);
	const char* member = g_atspi.message_get_member(message);
	const char* path = g_atspi.message_get_path(message);
	rvn_a11y_id_t id = 0;
	rvn_a11y_node_t node;
	DBusMessage* reply;
	DBusMessageIter in, out, sub, entry;
	(void)connection;
	(void)user;

	if (g_atspi.message_get_type(message) != DBUS_MESSAGE_TYPE_METHOD_CALL || !member) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	if (!parse_path(path, &id)) {
		reply_and_send(g_atspi.message_new_error(message, "org.freedesktop.DBus.Error.UnknownObject", "This object is gone"));
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	memset(&node, 0, sizeof(node));
	node.name = node.value = node.description = "";
	if (id && !rvn_a11y_describe(id, &node)) {
		reply_and_send(g_atspi.message_new_error(message, "org.freedesktop.DBus.Error.UnknownObject", "This object is gone"));
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	if (!id) node.role = R_ROLE_NONE;
	g_atspi.iter_init(message, &in);
	reply = g_atspi.message_new_method_return(message);
	g_atspi.iter_init_append(reply, &out);

	/* ---- properties ---- */
	if (iface && strcmp(iface, "org.freedesktop.DBus.Properties") == 0) {
		const char* piface = "";
		const char* pname = "";
		if (strcmp(member, "Get") == 0 && next_string(&in, &piface) && next_string(&in, &pname)) {
			if (put_property(&out, id, &node, piface, pname)) goto send;
			g_atspi.message_unref(reply);
			reply_and_send(g_atspi.message_new_error(message, "org.freedesktop.DBus.Error.UnknownProperty", pname));
			return DBUS_HANDLER_RESULT_HANDLED;
		}
		if (strcmp(member, "GetAll") == 0 && next_string(&in, &piface)) {
			static const char* const names[] = { "Name", "Description", "HelpText", "Locale", "AccessibleId", "ChildCount", "Parent",
				"ToolkitName", "Version", "AtspiVersion", "Id", "NActions", "CurrentValue", "MinimumValue", "MaximumValue",
				"MinimumIncrement", "Text", "CharacterCount", "CaretOffset" };
			size_t i;
			g_atspi.iter_open_container(&out, T_ARRAY, "{sv}", &sub);
			for (i = 0; i < RVN_COUNT_OF(names); i++) {
				DBusMessageIter probe;
				/* (an entry is only opened for the properties of this interface) */
				DBusMessage* scratch = g_atspi.message_new_method_return(message);
				g_atspi.iter_init_append(scratch, &probe);
				if (put_property(&probe, id, &node, piface, names[i])) {
					g_atspi.iter_open_container(&sub, T_DICT_ENTRY, NULL, &entry);
					put_string(&entry, names[i]);
					put_property(&entry, id, &node, piface, names[i]);
					g_atspi.iter_close_container(&sub, &entry);
				}
				g_atspi.message_unref(scratch);
			}
			g_atspi.iter_close_container(&out, &sub);
			goto send;
		}
		if (strcmp(member, "Set") == 0 && next_string(&in, &piface) && next_string(&in, &pname)) {
			DBusMessageIter variant;
			if (g_atspi.iter_get_arg_type(&in) == T_VARIANT) {
				g_atspi.iter_recurse(&in, &variant);
				if (strcmp(pname, "Id") == 0 && g_atspi.iter_get_arg_type(&variant) == T_INT32) {
					g_atspi.iter_get_basic(&variant, &g_atspi.app_id);
				} else if (strcmp(pname, "CurrentValue") == 0 && g_atspi.iter_get_arg_type(&variant) == T_DOUBLE) {
					double value;
					g_atspi.iter_get_basic(&variant, &value);
					rvn_a11y_set_number(id, value);
				}
			}
			goto send;
		}
		goto unknown;
	}

	/* ---- Accessible ---- */
	if (!iface || strcmp(iface, "org.a11y.atspi.Accessible") == 0) {
		if (strcmp(member, "GetChildAtIndex") == 0) {
			int32_t index = 0;
			rvn_a11y_id_t child;
			next_i32(&in, &index);
			child = child_at(id, index);
			if (child) put_ref(&out, child);
			else put_null_ref(&out);
			goto send;
		}
		if (strcmp(member, "GetChildren") == 0) {
			int i, n = child_count(id);
			g_atspi.iter_open_container(&out, T_ARRAY, "(so)", &sub);
			for (i = 0; i < n; i++) put_ref(&sub, child_at(id, i));
			g_atspi.iter_close_container(&out, &sub);
			goto send;
		}
		if (strcmp(member, "GetIndexInParent") == 0) { put_i32(&out, index_in_parent(id)); goto send; }
		if (strcmp(member, "GetRelationSet") == 0) {
			g_atspi.iter_open_container(&out, T_ARRAY, "(ua(so))", &sub);
			g_atspi.iter_close_container(&out, &sub);
			goto send;
		}
		if (strcmp(member, "GetRole") == 0) { put_u32(&out, id ? atspi_role(node.role, node.states) : 75u); goto send; }
		if (strcmp(member, "GetRoleName") == 0 || strcmp(member, "GetLocalizedRoleName") == 0) {
			put_string(&out, id ? rvn_a11y_role_name(node.role) : "application");
			goto send;
		}
		if (strcmp(member, "GetState") == 0) { put_states(&out, id, &node); goto send; }
		if (strcmp(member, "GetAttributes") == 0) {
			g_atspi.iter_open_container(&out, T_ARRAY, "{ss}", &sub);
			g_atspi.iter_open_container(&sub, T_DICT_ENTRY, NULL, &entry);
			put_string(&entry, "toolkit");
			put_string(&entry, "Raven");
			g_atspi.iter_close_container(&sub, &entry);
			g_atspi.iter_close_container(&out, &sub);
			goto send;
		}
		if (strcmp(member, "GetApplication") == 0) { put_ref(&out, 0); goto send; }
		if (strcmp(member, "GetInterfaces") == 0) {
			const char* names[8];
			int i, n;
			interfaces_of(id, &node, names, &n);
			g_atspi.iter_open_container(&out, T_ARRAY, "s", &sub);
			for (i = 0; i < n; i++) put_string(&sub, names[i]);
			g_atspi.iter_close_container(&out, &sub);
			goto send;
		}
		if (iface) goto unknown;
	}

	/* ---- Application ---- */
	if (iface && strcmp(iface, "org.a11y.atspi.Application") == 0) {
		if (strcmp(member, "GetLocale") == 0) { put_string(&out, rvn_a11y_locale()); goto send; }
		goto unknown;
	}

	/* ---- Component ---- */
	if (iface && strcmp(iface, "org.a11y.atspi.Component") == 0 && id) {
		int32_t x = 0, y = 0;
		uint32_t coords = 0;
		int ex, ey, ew, eh;
		if (strcmp(member, "GetExtents") == 0) {
			next_i32(&in, (int32_t*)&coords);
			get_extents(id, coords, &ex, &ey, &ew, &eh);
			g_atspi.iter_open_container(&out, T_STRUCT, NULL, &sub);
			put_i32(&sub, ex); put_i32(&sub, ey); put_i32(&sub, ew); put_i32(&sub, eh);
			g_atspi.iter_close_container(&out, &sub);
			goto send;
		}
		if (strcmp(member, "GetPosition") == 0) {
			next_i32(&in, (int32_t*)&coords);
			get_extents(id, coords, &ex, &ey, &ew, &eh);
			put_i32(&out, ex);
			put_i32(&out, ey);
			goto send;
		}
		if (strcmp(member, "GetSize") == 0) {
			get_extents(id, 0, &ex, &ey, &ew, &eh);
			put_i32(&out, ew);
			put_i32(&out, eh);
			goto send;
		}
		if (strcmp(member, "Contains") == 0) {
			next_i32(&in, &x); next_i32(&in, &y); next_i32(&in, (int32_t*)&coords);
			get_extents(id, coords, &ex, &ey, &ew, &eh);
			put_bool(&out, x >= ex && y >= ey && x < ex + ew && y < ey + eh);
			goto send;
		}
		if (strcmp(member, "GetAccessibleAtPoint") == 0) {
			rvn_a11y_id_t hit;
			next_i32(&in, &x); next_i32(&in, &y); next_i32(&in, (int32_t*)&coords);
			hit = node_at_point(id, x, y, coords);
			if (hit) put_ref(&out, hit);
			else put_null_ref(&out);
			goto send;
		}
		if (strcmp(member, "GetLayer") == 0) { put_u32(&out, node.role == R_ROLE_WINDOW ? 7u : 3u); goto send; }
		if (strcmp(member, "GetMDIZOrder") == 0) { int16_t z = 0; g_atspi.iter_append_basic(&out, T_INT16, &z); goto send; }
		if (strcmp(member, "GetAlpha") == 0) { put_double(&out, 1.0); goto send; }
		if (strcmp(member, "GrabFocus") == 0) { put_bool(&out, rvn_a11y_act(id, RVN_A11Y_FOCUS)); goto send; }
		if (strcmp(member, "ScrollTo") == 0 || strcmp(member, "ScrollToPoint") == 0) { put_bool(&out, 0); goto send; }
		goto unknown;
	}

	/* ---- Action ---- */
	if (iface && strcmp(iface, "org.a11y.atspi.Action") == 0 && id) {
		int32_t index = 0;
		if (strcmp(member, "GetActions") == 0) {
			g_atspi.iter_open_container(&out, T_ARRAY, "(sss)", &sub);
			if (has_action(&node)) {
				DBusMessageIter s;
				g_atspi.iter_open_container(&sub, T_STRUCT, NULL, &s);
				put_string(&s, action_name(&node));
				put_string(&s, "");
				put_string(&s, "");
				g_atspi.iter_close_container(&sub, &s);
			}
			g_atspi.iter_close_container(&out, &sub);
			goto send;
		}
		next_i32(&in, &index);
		if (strcmp(member, "GetName") == 0 || strcmp(member, "GetLocalizedName") == 0) { put_string(&out, index == 0 ? action_name(&node) : ""); goto send; }
		if (strcmp(member, "GetDescription") == 0 || strcmp(member, "GetKeyBinding") == 0) { put_string(&out, ""); goto send; }
		if (strcmp(member, "DoAction") == 0) {
			rvn_a11y_action_t action = node.role == R_ROLE_CHECKBOX ? RVN_A11Y_TOGGLE :
				((node.states & RVN_A11Y_EXPANDABLE) && !(node.states & RVN_A11Y_INVOKABLE)) ?
					((node.states & RVN_A11Y_EXPANDED) ? RVN_A11Y_COLLAPSE : RVN_A11Y_EXPAND) : RVN_A11Y_PRESS;
			put_bool(&out, index == 0 && rvn_a11y_act(id, action));
			goto send;
		}
		goto unknown;
	}

	/* ---- Value ---- */
	if (iface && strcmp(iface, "org.a11y.atspi.Value") == 0 && id) {
		if (strcmp(member, "SetCurrentValue") == 0 && g_atspi.iter_get_arg_type(&in) == T_DOUBLE) {
			double value;
			g_atspi.iter_get_basic(&in, &value);
			rvn_a11y_set_number(id, value);
			goto send;
		}
		goto unknown;
	}

	/* ---- Text ---- */
	if (iface && strcmp(iface, "org.a11y.atspi.Text") == 0 && id) {
		const char* text = node_text(&node);
		int length = utf8_length(text);
		int32_t start = 0, end = -1;
		if (strcmp(member, "GetText") == 0) {
			int a, b;
			char* part;
			next_i32(&in, &start);
			next_i32(&in, &end);
			if (end < 0 || end > length) end = length;
			if (start < 0) start = 0;
			if (start > end) start = end;
			a = utf8_offset(text, start);
			b = utf8_offset(text, end);
			part = rvn_strndup(text + a, (size_t)(b - a));
			put_string(&out, part);
			rvn_free(part);
			goto send;
		}
		if (strcmp(member, "GetTextAtOffset") == 0 || strcmp(member, "GetStringAtOffset") == 0) {
			/* the whole text, whatever the granularity: a line of a control */
			put_string(&out, text);
			put_i32(&out, 0);
			put_i32(&out, length);
			goto send;
		}
		if (strcmp(member, "GetCharacterAtOffset") == 0) {
			int a;
			uint32_t c = 0;
			const char* cursor;
			next_i32(&in, &start);
			a = utf8_offset(text, start);
			cursor = text + a;
			if (*cursor) c = rvn_utf8_decode(&cursor, text + strlen(text));
			put_i32(&out, (int32_t)c);
			goto send;
		}
		if (strcmp(member, "GetNSelections") == 0) { put_i32(&out, 0); goto send; }
		if (strcmp(member, "GetCaretOffset") == 0) { put_i32(&out, length); goto send; }
		if (strcmp(member, "GetAttributes") == 0 || strcmp(member, "GetDefaultAttributes") == 0) {
			g_atspi.iter_open_container(&out, T_ARRAY, "{ss}", &sub);
			g_atspi.iter_close_container(&out, &sub);
			if (strcmp(member, "GetAttributes") == 0) { put_i32(&out, 0); put_i32(&out, length); }
			goto send;
		}
		goto unknown;
	}

	/* ---- EditableText ---- */
	if (iface && strcmp(iface, "org.a11y.atspi.EditableText") == 0 && id) {
		const char* text = "";
		if (strcmp(member, "SetTextContents") == 0 && next_string(&in, &text)) {
			put_bool(&out, rvn_a11y_set_value(id, text));
			goto send;
		}
		goto unknown;
	}

unknown:
	g_atspi.message_unref(reply);
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
send:
	reply_and_send(reply);
	return DBUS_HANDLER_RESULT_HANDLED;
}

/* ---------------------------------------------------------------------------
 *  Connection
 * ------------------------------------------------------------------------- */

/* A property of org.a11y.Status on the session bus (IsEnabled, ScreenReaderEnabled). */
static int session_flag(DBusConnection* session, const char* name) {
	DBusMessage* call = g_atspi.message_new_method_call("org.a11y.Bus", "/org/a11y/bus", "org.freedesktop.DBus.Properties", "Get");
	DBusMessage* reply;
	DBusMessageIter it, variant;
	DBusError error;
	const char* iface = "org.a11y.Status";
	dbus_bool_t value = 0;
	if (!call) return R_FALSE;
	g_atspi.iter_init_append(call, &it);
	put_string(&it, iface);
	put_string(&it, name);
	g_atspi.error_init(&error);
	reply = g_atspi.connection_send_with_reply_and_block(session, call, 500, &error);
	g_atspi.message_unref(call);
	if (!reply) {
		g_atspi.error_free(&error);
		return R_FALSE;
	}
	if (g_atspi.iter_init(reply, &it) && g_atspi.iter_get_arg_type(&it) == T_VARIANT) {
		g_atspi.iter_recurse(&it, &variant);
		if (g_atspi.iter_get_arg_type(&variant) == T_BOOLEAN) g_atspi.iter_get_basic(&variant, &value);
	}
	g_atspi.message_unref(reply);
	return value != 0;
}

/* The address of the accessibility bus: AT_SPI_BUS_ADDRESS, else org.a11y.Bus GetAddress. */
static char* bus_address(DBusConnection* session) {
	const char* env = getenv("AT_SPI_BUS_ADDRESS");
	DBusMessage* call;
	DBusMessage* reply;
	DBusMessageIter it;
	DBusError error;
	char* address = NULL;
	if (env && env[0]) return rvn_strdup(env);
	if (!session) return NULL;
	call = g_atspi.message_new_method_call("org.a11y.Bus", "/org/a11y/bus", "org.a11y.Bus", "GetAddress");
	if (!call) return NULL;
	g_atspi.error_init(&error);
	reply = g_atspi.connection_send_with_reply_and_block(session, call, 500, &error);
	g_atspi.message_unref(call);
	if (!reply) {
		g_atspi.error_free(&error);
		return NULL;
	}
	if (g_atspi.iter_init(reply, &it) && g_atspi.iter_get_arg_type(&it) == T_STRING) {
		const char* text = NULL;
		g_atspi.iter_get_basic(&it, &text);
		if (text) address = rvn_strdup(text);
	}
	g_atspi.message_unref(reply);
	return address;
}

/* Tells the registry that the application is there; it answers with the desktop, our parent. */
static void embed(void) {
	DBusMessage* call = g_atspi.message_new_method_call("org.a11y.atspi.Registry", ROOT_PATH, "org.a11y.atspi.Socket", "Embed");
	DBusMessage* reply;
	DBusMessageIter it, s;
	DBusError error;
	if (!call) return;
	g_atspi.iter_init_append(call, &it);
	put_ref(&it, 0);
	g_atspi.error_init(&error);
	reply = g_atspi.connection_send_with_reply_and_block(g_atspi.bus, call, 1000, &error);
	g_atspi.message_unref(call);
	if (!reply) {
		/* no registry (no desktop): the objects are served all the same */
		g_atspi.error_free(&error);
		return;
	}
	if (g_atspi.iter_init(reply, &it) && g_atspi.iter_get_arg_type(&it) == T_STRUCT) {
		const char* name = NULL;
		const char* path = NULL;
		g_atspi.iter_recurse(&it, &s);
		if (g_atspi.iter_get_arg_type(&s) == T_STRING) g_atspi.iter_get_basic(&s, &name);
		g_atspi.iter_next(&s);
		if (g_atspi.iter_get_arg_type(&s) == T_OBJECT_PATH) g_atspi.iter_get_basic(&s, &path);
		if (name) snprintf(g_atspi.parent_name, sizeof(g_atspi.parent_name), "%s", name);
		if (path) snprintf(g_atspi.parent_path, sizeof(g_atspi.parent_path), "%s", path);
	}
	g_atspi.message_unref(reply);
}

int rvn_atspi_init(void) {
	DBusConnection* session = NULL;
	DBusError error;
	char* address;
	static const DBusObjectPathVTable vtable = { NULL, handle, NULL, NULL, NULL, NULL };
	if (g_atspi.bus) return R_TRUE;
	if (g_atspi.failed || !rvn_atspi_load()) {
		g_atspi.failed = R_TRUE;
		return R_FALSE;
	}
	g_atspi.error_init(&error);
	if (!getenv("AT_SPI_BUS_ADDRESS")) {
		session = g_atspi.bus_get_private(DBUS_BUS_SESSION, &error);
		if (!session) {
			g_atspi.error_free(&error);
			g_atspi.failed = R_TRUE;
			return R_FALSE;
		}
		g_atspi.connection_set_exit_on_disconnect(session, 0);
		/* nobody listens: nothing to do (this is asked again by Raccessibility_is_screen_reader_active) */
		if (!session_flag(session, "IsEnabled") && !session_flag(session, "ScreenReaderEnabled") && !getenv("RAVEN_ACCESSIBILITY")) {
			g_atspi.connection_close(session);
			g_atspi.connection_unref(session);
			return R_FALSE;
		}
	}
	address = bus_address(session);
	if (session) {
		g_atspi.connection_close(session);
		g_atspi.connection_unref(session);
	}
	if (!address) {
		g_atspi.failed = R_TRUE;
		return R_FALSE;
	}
	g_atspi.bus = g_atspi.connection_open_private(address, &error);
	rvn_free(address);
	if (!g_atspi.bus || !g_atspi.bus_register(g_atspi.bus, &error)) {
		if (g_atspi.error_is_set(&error)) g_atspi.error_free(&error);
		if (g_atspi.bus) {
			g_atspi.connection_close(g_atspi.bus);
			g_atspi.connection_unref(g_atspi.bus);
		}
		g_atspi.bus = NULL;
		g_atspi.failed = R_TRUE;
		return R_FALSE;
	}
	g_atspi.connection_set_exit_on_disconnect(g_atspi.bus, 0);
	snprintf(g_atspi.unique, sizeof(g_atspi.unique), "%s", g_atspi.bus_get_unique_name(g_atspi.bus));
	g_atspi.connection_register_fallback(g_atspi.bus, "/org/a11y/atspi/accessible", &vtable, NULL);
	embed();
	g_atspi.enabled = R_TRUE;
	return R_TRUE;
}

void rvn_atspi_shutdown(void) {
	if (g_atspi.bus) {
		g_atspi.connection_close(g_atspi.bus);
		g_atspi.connection_unref(g_atspi.bus);
	}
	g_atspi.bus = NULL;
	g_atspi.enabled = R_FALSE;
	/* (libdbus stays loaded: unloading it while it holds threads is not safe) */
}

int rvn_atspi_fd(void) {
	int fd = -1;
	if (!g_atspi.bus || !g_atspi.connection_get_unix_fd(g_atspi.bus, &fd)) return -1;
	return fd;
}

void rvn_atspi_dispatch(void) {
	int guard = 0;
	if (!g_atspi.bus) return;
	g_atspi.connection_read_write(g_atspi.bus, 0);
	while (g_atspi.connection_dispatch(g_atspi.bus) == DBUS_DISPATCH_DATA_REMAINS && guard++ < 1000) {}
	g_atspi.connection_flush(g_atspi.bus);
}

/* An event: path of the node, interface org.a11y.atspi.Event.<kind>, signature siiva{sv}. */
static void signal(rvn_a11y_id_t id, const char* kind, const char* member, const char* detail, int detail1, int detail2,
	char any_type, const void* any_value) {
	char path[96], iface[96];
	DBusMessage* message;
	DBusMessageIter it, variant, dict;
	char signature[2] = { any_type, 0 };
	node_path(id, path, sizeof(path));
	snprintf(iface, sizeof(iface), "org.a11y.atspi.Event.%s", kind);
	message = g_atspi.message_new_signal(path, iface, member);
	if (!message) return;
	g_atspi.iter_init_append(message, &it);
	put_string(&it, detail);
	put_i32(&it, detail1);
	put_i32(&it, detail2);
	if (any_type == T_STRUCT) {
		g_atspi.iter_open_container(&it, T_VARIANT, "(so)", &variant);
		put_ref(&variant, *(const rvn_a11y_id_t*)any_value);
	} else {
		g_atspi.iter_open_container(&it, T_VARIANT, signature, &variant);
		g_atspi.iter_append_basic(&variant, any_type, any_value);
	}
	g_atspi.iter_close_container(&it, &variant);
	g_atspi.iter_open_container(&it, T_ARRAY, "{sv}", &dict);
	g_atspi.iter_close_container(&it, &dict);
	g_atspi.connection_send(g_atspi.bus, message, NULL);
	g_atspi.message_unref(message);
}

void rvn_atspi_event(rvn_pwindow_t* window, int event, uint64_t id) {
	int32_t zero = 0;
	(void)window;
	if (!g_atspi.bus) return;
	switch (event) {
		case RVN_A11Y_EVENT_FOCUS:
			signal(id, "Object", "StateChanged", "focused", 1, 0, T_INT32, &zero);
			signal(id, "Focus", "Focus", "", 0, 0, T_INT32, &zero);
			break;
		case RVN_A11Y_EVENT_VALUE: {
			rvn_a11y_node_t node;
			if (!rvn_a11y_describe(id, &node)) break;
			if (node.states & RVN_A11Y_HAS_RANGE) {
				signal(id, "Object", "PropertyChange", "accessible-value", 0, 0, T_DOUBLE, &node.number);
			} else {
				const char* name = node.name;
				signal(id, "Object", "PropertyChange", "accessible-name", 0, 0, T_STRING, &name);
				if (node.states & RVN_A11Y_CHECKABLE) signal(id, "Object", "StateChanged", "checked", (node.states & RVN_A11Y_CHECKED) ? 1 : 0, 0, T_INT32, &zero);
				if (node.states & RVN_A11Y_EXPANDABLE) signal(id, "Object", "StateChanged", "expanded", (node.states & RVN_A11Y_EXPANDED) ? 1 : 0, 0, T_INT32, &zero);
				if (node.states & RVN_A11Y_EDITABLE) signal(id, "Object", "TextChanged", "insert", 0, 0, T_STRING, &node.value);
			}
			break;
		}
		case RVN_A11Y_EVENT_STRUCTURE:
			signal(id, "Object", "ChildrenChanged", "add", 0, 0, T_STRUCT, &id);
			break;
		default:
			break;
	}
	g_atspi.connection_flush(g_atspi.bus);
}

void rvn_atspi_announce(rvn_pwindow_t* window, const char* text, int assertive) {
	(void)window;
	if (!g_atspi.bus || !text) return;
	/* Object:Announcement (Orca 45+): detail1 is the politeness, 1 polite, 2 assertive */
	signal(0, "Object", "Announcement", "", assertive ? 2 : 1, 0, T_STRING, &text);
	g_atspi.connection_flush(g_atspi.bus);
}

int rvn_atspi_active(void) {
	if (!g_atspi.bus) rvn_atspi_init();
	return g_atspi.bus != NULL;
}

void rvn_atspi_settings(RAccessibilitySettings* settings) {
	const char* theme = getenv("GTK_THEME");
	const char* scale = getenv("GDK_DPI_SCALE");
	if (theme && (strstr(theme, "HighContrast") || strstr(theme, "highcontrast"))) settings->high_contrast = R_TRUE;
	if (scale && atof(scale) >= 0.5 && atof(scale) <= 4.0) settings->text_scale = (float)atof(scale);
	/* GNOME: animations turned off in the settings (Accessibility, Reduce Animation) */
	{
		FILE* pipe = popen("gsettings get org.gnome.desktop.interface enable-animations 2>/dev/null", "r");
		char line[32];
		if (pipe) {
			if (fgets(line, sizeof(line), pipe) && strncmp(line, "false", 5) == 0) settings->reduce_motion = R_TRUE;
			pclose(pipe);
		}
	}
}

#endif
