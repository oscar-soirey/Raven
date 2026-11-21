"""
Lua (5.1 to 5.5, and LuaJIT): a C module, bindings/lua/raven_lua.c.

Standard Lua has no FFI: a C module does the conversions, as the addon of
Node.js does (gen_napi.py, from which the keeping of callbacks is taken). The
names are those of the LuaJIT binding (gen_luajit.py): every function, every
item of an enumeration and every constant under its C name, so that a program
written for one runs on the other once its FFI parts are removed. See
bindings/lua/README.md for the conventions.
"""

from cparse import classify
from gen_bun import NATIVE_ONLY
from gen_napi import callback_rule

# The callbacks that may arrive on another thread: written by hand below.
OFF_THREAD = ("RTaskCallback", "RErrorCallback")

# Callbacks that return a text for which NULL means something ("none", "keep
# the default"): nil gives NULL. The others get "" for nil.
NULLABLE_TEXT = ("RTileDragDataCallback", "RTextEditorHoverCallback")

# How a scalar of the C API is read from Lua, its C type in a wrapper, and how it is pushed.
READ = {
    "int": ("int", "rl_int(L, %d, NULL)"), "char": ("int", "rl_int(L, %d, NULL)"),
    "float": ("float", "(float)rl_number(L, %d, NULL)"), "double": ("double", "rl_number(L, %d, NULL)"),
    "uint8": ("uint8_t", "(uint8_t)rl_u32(L, %d, NULL)"), "uint32": ("uint32_t", "rl_u32(L, %d, NULL)"),
    "id": ("uint32_t", "rl_u32(L, %d, NULL)"), "size_t": ("size_t", "(size_t)rl_u64(L, %d, NULL)"),
    "uintptr": ("uintptr_t", "(uintptr_t)rl_u64(L, %d, NULL)"), "uint64": ("uint64_t", "rl_u64(L, %d, NULL)"),
}
PUSH = {
    "int": "lua_pushinteger(L, (lua_Integer)%s)", "char": "lua_pushinteger(L, (lua_Integer)%s)",
    "float": "lua_pushnumber(L, (lua_Number)%s)", "double": "lua_pushnumber(L, (lua_Number)%s)",
    "uint8": "lua_pushinteger(L, (lua_Integer)%s)", "uint32": "rl_push_u32(L, (uint32_t)%s)",
    "id": "rl_push_u32(L, (uint32_t)%s)", "size_t": "rl_push_u64(L, (uint64_t)%s)",
    "uintptr": "rl_push_u64(L, (uint64_t)%s)", "uint64": "rl_push_u64(L, (uint64_t)%s)",
}
# A scalar read from the value at stack index `i`, with a field name for the errors.
READ_AT = {
    "int": "rl_int(L, %s, %s)", "char": "(char)rl_int(L, %s, %s)", "float": "(float)rl_number(L, %s, %s)",
    "double": "rl_number(L, %s, %s)", "uint8": "(uint8_t)rl_u32(L, %s, %s)", "uint32": "rl_u32(L, %s, %s)",
    "id": "rl_u32(L, %s, %s)", "size_t": "(size_t)rl_u64(L, %s, %s)", "uintptr": "(uintptr_t)rl_u64(L, %s, %s)",
    "uint64": "rl_u64(L, %s, %s)",
}
NUMBER_C = {"double": "double", "float": "float", "int": "int", "uint32": "uint32_t"}

PRELUDE = r'''/*
 * Raven for Lua: the C module (Lua 5.1 to 5.5, and LuaJIT).
 *
 * Generated from the C headers by tools/bindgen (Raven %(version)s): do not edit.
 *
 * Every function of the C API is in the table of the module under its own
 * name, with the items of the enumerations and the constants, as in the
 * LuaJIT binding (bindings/luajit). What C passes through pointers travels
 * as Lua values: see bindings/lua/README.md.
 *
 *   local r = require("raven")
 *   r.Rinit(r.R_BACKEND_DEFAULT)
 *   local xml = [[<Window Title="Hi"><Button Text="Quit" OnClicked="quit"/></Window>]]
 *   r.Rwindow_create_from_xml(xml)
 *   r.Raction_bind("quit", function(sender, action) r.Rquit(0) end)
 *   while r.Rwait_frame(-1.0) ~= 0 do end
 *   r.Rshutdown()
 *
 * Two ways to build it:
 *
 *   - as the module raven.so / raven.dll (what LuaRocks builds): it opens the
 *     shared library of Raven when it is loaded, from the RAVEN_LIBRARY
 *     environment variable, else where the system finds libraries
 *     (libraven.so, libraven.dylib, raven.dll). Only the headers of Lua
 *     and of Raven are needed to compile it;
 *   - with RAVEN_LUA_LINK defined, inside a program that embeds Lua and links
 *     Raven itself: luaopen_raven (raven_lua.h) then calls Raven directly.
 */

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"

#include "Raven.h"
#include "raven_lua.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#ifndef RAVEN_LUA_LINK
#include <dlfcn.h>
#endif
#endif

/* ---- what differs between the versions of Lua */

#ifndef LUA_OK
#define LUA_OK 0
#endif

#if LUA_VERSION_NUM >= 502
#define rl_rawlen(L, i) lua_rawlen(L, i)
#else
#define rl_rawlen(L, i) lua_objlen(L, i)
#endif

#define rl_abs(L, i) ((i) > 0 || (i) <= LUA_REGISTRYINDEX ? (i) : lua_gettop(L) + (i) + 1)

static void rl_setfuncs(lua_State* L, const luaL_Reg* functions) {
	for (; functions->name; functions++) {
		lua_pushcfunction(L, functions->func);
		lua_setfield(L, -2, functions->name);
	}
}

/* ---- the library: every function through RL(), from a table filled by rl_load, or directly */

#ifdef RAVEN_LUA_LINK
#define RL(name) name
#else
typedef struct rl_table {
%(table)s
} rl_table;

static rl_table rl;

static const struct {
	const char* name;
	size_t offset;
} rl_symbols[] = {
%(symbols)s
};

#define RL(name) rl.name
#endif

/* ---- the thread of Lua: the one that loaded the module, which must be the UI thread of Raven */

#ifdef _WIN32
static DWORD rl_thread;
static int rl_on_thread(void) { return GetCurrentThreadId() == rl_thread; }
static void rl_thread_set(void) { rl_thread = GetCurrentThreadId(); }
#else
static pthread_t rl_thread;
static int rl_on_thread(void) { return pthread_equal(pthread_self(), rl_thread); }
static void rl_thread_set(void) { rl_thread = pthread_self(); }
#endif
static int rl_ready;

/* ---- reading arguments (and fields: `field` names them in the errors, NULL for an argument) */

/** The registry key of the names of the items of the enumerations, for those given as strings. */
#define RL_NAMES "raven.names"
#define RL_MAIN "raven.main"
#define RL_SENTINEL "raven.sentinel"

static int rl_type_error(lua_State* L, int i, const char* field, const char* expected) {
	if (field) return luaL_error(L, "raven: field '%%s': %%s expected, got %%s", field, expected, luaL_typename(L, i));
	return luaL_argerror(L, i, lua_pushfstring(L, "%%s expected, got %%s", expected, luaL_typename(L, i)));
}

static lua_Number rl_number(lua_State* L, int i, const char* field) {
	switch (lua_type(L, i)) {
		case LUA_TNUMBER: return lua_tonumber(L, i);
		case LUA_TBOOLEAN: return (lua_Number)lua_toboolean(L, i);
		case LUA_TSTRING: if (lua_isnumber(L, i)) return lua_tonumber(L, i); break;
		default: break;
	}
	rl_type_error(L, i, field, "number");
	return 0;
}

static int rl_int(lua_State* L, int i, const char* field) {
	lua_Number n;
#if LUA_VERSION_NUM >= 503
	if (lua_isinteger(L, i)) {
		lua_Integer v = lua_tointeger(L, i);
		return v < INT_MIN ? INT_MIN : v > INT_MAX ? INT_MAX : (int)v;
	}
#endif
	n = rl_number(L, i, field);
	if (!(n == n)) return 0;
	return n <= (lua_Number)INT_MIN ? INT_MIN : n >= (lua_Number)INT_MAX ? INT_MAX : (int)n;
}

/** 64 bits: integers of Lua 5.3 and later as they are; numbers (Lua 5.1, 5.2) exact up to 2^53. */
static uint64_t rl_u64(lua_State* L, int i, const char* field) {
	lua_Number n;
#if LUA_VERSION_NUM >= 503
	if (lua_isinteger(L, i)) return (uint64_t)lua_tointeger(L, i);
#endif
	n = rl_number(L, i, field);
	if (!(n == n)) return 0;
	if (n < 0) return n <= -9223372036854775808.0 ? (uint64_t)INT64_MIN : (uint64_t)(int64_t)n;
	return n >= 18446744073709551615.0 ? UINT64_MAX : (uint64_t)n;
}

/** Handles (Rid) and other unsigned 32-bit values: exact in every version of Lua. -1 is R_INVALID_ID. */
static uint32_t rl_u32(lua_State* L, int i, const char* field) {
	return (uint32_t)rl_u64(L, i, field);
}

static int rl_enum(lua_State* L, int i, const char* field) {
	i = rl_abs(L, i);
	if (lua_type(L, i) == LUA_TSTRING && !lua_isnumber(L, i)) {
		int value;
		lua_getfield(L, LUA_REGISTRYINDEX, RL_NAMES);
		lua_pushvalue(L, i);
		lua_rawget(L, -2);
		if (lua_type(L, -1) != LUA_TNUMBER) {
			if (field) return luaL_error(L, "raven: field '%%s': no item named '%%s'", field, lua_tostring(L, i));
			return luaL_argerror(L, i, lua_pushfstring(L, "no item named '%%s'", lua_tostring(L, i)));
		}
		value = (int)lua_tonumber(L, -1);
		lua_pop(L, 2);
		return value;
	}
	return rl_int(L, i, field);
}

/** A text, or NULL for nil. It lives as long as the value stays on the stack (or in its table). */
static const char* rl_lstr(lua_State* L, int i, size_t* size, const char* field) {
	size_t n = 0;
	const char* text = NULL;
	switch (lua_type(L, i)) {
		case LUA_TNONE: case LUA_TNIL: break;
		case LUA_TSTRING: case LUA_TNUMBER: text = lua_tolstring(L, i, &n); break;
		default: rl_type_error(L, i, field, "string or nil");
	}
	if (size) *size = n;
	return text;
}

static const char* rl_str(lua_State* L, int i, const char* field) {
	return rl_lstr(L, i, NULL, field);
}

/** An address: a light userdata (what Raven returned), the block of a full userdata, or NULL for nil. */
static void* rl_ptr(lua_State* L, int i, const char* field) {
	switch (lua_type(L, i)) {
		case LUA_TNONE: case LUA_TNIL: return NULL;
		case LUA_TLIGHTUSERDATA: case LUA_TUSERDATA: return lua_touserdata(L, i);
		default: rl_type_error(L, i, field, "userdata or nil");
	}
	return NULL;
}

/** The size or count that follows data: nil for all of it, never more than there is. */
static size_t rl_size_opt(lua_State* L, int i, size_t have) {
	uint64_t wanted;
	if (lua_isnoneornil(L, i)) return have;
	wanted = rl_u64(L, i, NULL);
	return wanted < (uint64_t)have ? (size_t)wanted : have;
}

static int rl_count_opt(lua_State* L, int i, int have) {
	int wanted;
	if (lua_isnoneornil(L, i)) return have;
	wanted = rl_int(L, i, NULL);
	return wanted < 0 ? 0 : wanted < have ? wanted : have;
}

/** Memory for the time of a call: a userdata left on the stack, collected afterwards. */
static void* rl_scratch(lua_State* L, size_t size) {
	void* block = lua_newuserdata(L, size ? size : 1);
	memset(block, 0, size ? size : 1);
	return block;
}

/** Pixels (RGBA8) given as a string: at least width * height * 4 bytes. */
static void rl_check_pixels(lua_State* L, int i, const char* data, size_t have, int width, int height) {
	size_t need = width > 0 && height > 0 ? (size_t)width * (size_t)height * 4 : 0;
	if (data && have < need) luaL_argerror(L, i, "fewer bytes than width * height * 4");
}

/** Several texts: every pointer and its text in one block, for the time of the call. */
static const char** rl_strings(lua_State* L, int i, int* count) {
	size_t total, n;
	int k, length;
	char* text;
	const char** list;
	*count = 0;
	if (lua_isnoneornil(L, i)) return NULL;
	if (!lua_istable(L, i)) {
		rl_type_error(L, i, NULL, "table of strings or nil");
		return NULL;
	}
	length = (int)rl_rawlen(L, i);
	if (length <= 0) return NULL;
	total = (size_t)length * sizeof(char*);
	for (k = 1; k <= length; k++) {
		lua_rawgeti(L, i, k);
		if (!lua_isstring(L, -1)) luaL_argerror(L, i, "every element must be a string");
		lua_tolstring(L, -1, &n);
		total += n + 1;
		lua_pop(L, 1);
	}
	list = (const char**)rl_scratch(L, total);
	text = (char*)list + (size_t)length * sizeof(char*);
	for (k = 1; k <= length; k++) {
		const char* s;
		lua_rawgeti(L, i, k);
		s = lua_tolstring(L, -1, &n);
		memcpy(text, s, n + 1);
		list[k - 1] = text;
		text += n + 1;
		lua_pop(L, 1);
	}
	*count = length;
	return list;
}

/** `count` numbers from a table (missing ones are 0), or NULL for nil or no count. */
#define RL_NUMBERS(type, name) \
	static type* name(lua_State* L, int i, int count) { \
		type* values; \
		int k; \
		if (lua_isnoneornil(L, i) || count <= 0) return NULL; \
		if (!lua_istable(L, i)) { \
			rl_type_error(L, i, NULL, "table of numbers or nil"); \
			return NULL; \
		} \
		values = (type*)rl_scratch(L, (size_t)count * sizeof(type)); \
		for (k = 0; k < count; k++) { \
			lua_rawgeti(L, i, k + 1); \
			if (!lua_isnil(L, -1)) values[k] = (type)rl_number(L, lua_gettop(L), NULL); \
			lua_pop(L, 1); \
		} \
		return values; \
	}
RL_NUMBERS(double, rl_doubles)
RL_NUMBERS(float, rl_floats)

/* ---- making results */

static void rl_push_u32(lua_State* L, uint32_t value) {
#if LUA_VERSION_NUM >= 503
	lua_pushinteger(L, (lua_Integer)value);
#else
	lua_pushnumber(L, (lua_Number)value);
#endif
}

static void rl_push_u64(lua_State* L, uint64_t value) {
#if LUA_VERSION_NUM >= 503
	lua_pushinteger(L, (lua_Integer)value);
#else
	lua_pushnumber(L, (lua_Number)value);
#endif
}

/** A text of Raven, copied into a Lua string at once; nil for NULL. */
static void rl_push_text(lua_State* L, const char* text) {
	if (text) lua_pushstring(L, text);
	else lua_pushnil(L);
}

static void rl_push_ptr(lua_State* L, const void* address) {
	if (address) lua_pushlightuserdata(L, (void*)address);
	else lua_pushnil(L);
}

/** A fixed array of chars of a structure, which may fill it without a zero. */
static void rl_push_chars(lua_State* L, const char* chars, size_t size) {
	size_t n = 0;
	while (n < size && chars[n]) n++;
	lua_pushlstring(L, chars, n);
}

/* ---- callbacks
 *
 * rl_cb is what the library calls back: the Lua function, kept in the
 * registry, and the user_data it was given (any Lua value, given back to the
 * function as its last argument). It is reachable from the C side of Raven for
 * as long as Raven may call it, so it is released when (and only when) Raven
 * forgets it, as in the Node.js binding:
 *
 *   - by a slot: an owner (a widget, a window, a name, the application...) and
 *     the setter that was called. Setting that slot again, or clearing it,
 *     releases what was there; the destruction of the owner releases all of
 *     them. A widget tells when it is gone through its destroy callback, which
 *     this file owns (Rwidget_set_destroy_callback goes in a separate field);
 *   - by its one call: Rtimer_create without repeat, Rtask_post, Rtask_run_async, Rdialog_open;
 *   - by Rshutdown, which forgets everything.
 *
 * The functions run on the main thread of their Lua state, protected: an
 * error never crosses the frames of Raven, it goes to the error callback
 * (Rerror_set_callback) as R_ERROR_SCRIPT, else to stderr. When the Lua state
 * is closed (lua_close) its callbacks stop being called.
 */

typedef struct rl_cb {
	lua_State* L;    /* the main thread of the Lua state; NULL once the state is closed */
	int fn[2];       /* references of the functions in the registry */
	int user;        /* reference of the user_data */
	int keep;        /* reference of what C may still read from the last call (texts of a structure) */
	char* text;      /* what was last returned to C by a callback that returns a text */
	size_t text_cap;
	int depth;
	int dead;
	int once;
	int bound;
	uint64_t key;
	struct rl_cb* task_prev;
	struct rl_cb* task_next;
	int task_listed;
} rl_cb;

typedef struct rl_slot {
	struct rl_slot* next;
	uint32_t id;
	char* name;
	rl_cb* cb;
} rl_slot;

enum { RL_OWNER_PLAIN, RL_OWNER_WIDGET, RL_OWNER_WINDOW };

typedef struct rl_owner {
	struct rl_owner* next;
	struct rl_owner* window_next;
	uint64_t key;
	int kind;
	int hooked;
	rl_slot* slots;
	rl_cb* destroy_cb;
	RWidgetDestroyCallback native_destroy;
	void* native_destroy_user;
} rl_owner;

#define RL_BUCKETS 4096
static rl_owner* rl_owners[RL_BUCKETS];
static rl_owner* rl_windows;
static rl_cb* rl_tasks;
static rl_cb* rl_error_cb;
/** The memory returned last by a callback now released: C may still be reading it. */
static char* rl_retired_text;

static void rl_widget_destroyed(Rid widget, void* user_data);

static size_t rl_bucket(uint64_t key) {
	return (size_t)((key ^ (key >> 20) ^ (key >> 41)) %% RL_BUCKETS);
}

static rl_owner* rl_owner_find(uint64_t key) {
	rl_owner* owner;
	for (owner = rl_owners[rl_bucket(key)]; owner; owner = owner->next) {
		if (owner->key == key) return owner;
	}
	return NULL;
}

static rl_owner* rl_owner_add(uint64_t key, int kind) {
	rl_owner* owner = (rl_owner*)calloc(1, sizeof(rl_owner));
	size_t bucket = rl_bucket(key);
	if (!owner) return NULL;
	owner->key = key;
	owner->kind = kind;
	owner->next = rl_owners[bucket];
	rl_owners[bucket] = owner;
	if (kind == RL_OWNER_WINDOW) {
		owner->window_next = rl_windows;
		rl_windows = owner;
	}
	return owner;
}

static void rl_owner_unlink(rl_owner* owner) {
	rl_owner** p;
	for (p = &rl_owners[rl_bucket(owner->key)]; *p; p = &(*p)->next) {
		if (*p == owner) {
			*p = owner->next;
			break;
		}
	}
	if (owner->kind == RL_OWNER_WINDOW) {
		for (p = &rl_windows; *p; p = &(*p)->window_next) {
			if (*p == owner) {
				*p = owner->window_next;
				break;
			}
		}
	}
}

static void rl_unref(lua_State* L, int* ref) {
	if (L && *ref != LUA_NOREF && *ref != LUA_REFNIL && lua_checkstack(L, 2)) luaL_unref(L, LUA_REGISTRYINDEX, *ref);
	*ref = LUA_NOREF;
}

static void rl_cb_free(rl_cb* cb) {
	rl_unref(cb->L, &cb->fn[0]);
	rl_unref(cb->L, &cb->fn[1]);
	rl_unref(cb->L, &cb->user);
	rl_unref(cb->L, &cb->keep);
	if (cb->text) {
		/* a callback released during its own call may still be returning this */
		free(rl_retired_text);
		rl_retired_text = cb->text;
	}
	free(cb);
}

/** Takes a callback out of the table or the list of tasks, without releasing it. */
static void rl_cb_detach(rl_cb* cb) {
	if (cb->bound) {
		rl_owner* owner = rl_owner_find(cb->key);
		rl_slot** p;
		cb->bound = 0;
		if (owner) {
			for (p = &owner->slots; *p; p = &(*p)->next) {
				if ((*p)->cb == cb) {
					rl_slot* slot = *p;
					*p = slot->next;
					free(slot->name);
					free(slot);
					break;
				}
			}
			if (!owner->slots && !owner->hooked && !owner->destroy_cb && !owner->native_destroy) {
				rl_owner_unlink(owner);
				free(owner);
			}
		}
	}
	if (cb->task_listed) {
		cb->task_listed = 0;
		if (cb->task_prev) cb->task_prev->task_next = cb->task_next;
		else rl_tasks = cb->task_next;
		if (cb->task_next) cb->task_next->task_prev = cb->task_prev;
	}
}

static void rl_cb_release(rl_cb* cb) {
	if (!cb) return;
	if (cb->depth > 0) cb->dead = 1;
	else rl_cb_free(cb);
}

/** Starts a call of a callback: 0 when its Lua state is gone. */
static int rl_cb_enter(rl_cb* cb) {
	if (!cb || !cb->L) return 0;
	cb->depth++;
	return 1;
}

/** Ends a call of a callback. */
static void rl_cb_leave(rl_cb* cb) {
	if (--cb->depth > 0) return;
	if (cb->once && !cb->dead) {
		rl_cb_detach(cb);
		cb->dead = 1;
	}
	if (cb->dead) rl_cb_free(cb);
}

static void rl_owner_free(rl_owner* owner) {
	rl_slot* slot = owner->slots;
	while (slot) {
		rl_slot* next = slot->next;
		if (slot->cb) {
			slot->cb->bound = 0;
			rl_cb_release(slot->cb);
		}
		free(slot->name);
		free(slot);
		slot = next;
	}
	rl_cb_release(owner->destroy_cb);
	free(owner);
}

static int rl_same_name(const char* a, const char* b) {
	return a == b || (a && b && strcmp(a, b) == 0);
}

static char* rl_strdup(const char* text) {
	size_t n = strlen(text) + 1;
	char* copy = (char*)malloc(n);
	if (copy) memcpy(copy, text, n);
	return copy;
}

static void rl_hook_widget(rl_owner* owner) {
	if (owner->hooked) return;
	owner->hooked = 1;
	RL(Rwidget_set_destroy_callback)((Rid)owner->key, rl_widget_destroyed, owner);
}

/** Sets a slot: what was there is released. A null `cb` only clears it. */
static void rl_bind(uint64_t key, int kind, uint32_t id, const char* name, rl_cb* cb) {
	rl_owner* owner = rl_owner_find(key);
	rl_slot** p;
	if (!owner) {
		if (!cb) return;
		owner = rl_owner_add(key, kind);
		if (!owner) {
			rl_cb_release(cb);
			return;
		}
	}
	if (cb) {
		cb->bound = 1;
		cb->key = key;
		if (kind == RL_OWNER_WIDGET) rl_hook_widget(owner);
	}
	for (p = &owner->slots; *p; p = &(*p)->next) {
		if ((*p)->id == id && rl_same_name((*p)->name, name)) {
			rl_slot* slot = *p;
			rl_cb* old = slot->cb;
			if (old) old->bound = 0;
			if (cb) {
				slot->cb = cb;
			} else {
				*p = slot->next;
				free(slot->name);
				free(slot);
			}
			rl_cb_release(old);
			if (!owner->slots && !owner->hooked && !owner->destroy_cb && !owner->native_destroy) {
				rl_owner_unlink(owner);
				free(owner);
			}
			return;
		}
	}
	if (cb) {
		rl_slot* slot = (rl_slot*)malloc(sizeof(rl_slot));
		if (!slot) {
			cb->bound = 0;
			rl_cb_release(cb);
			return;
		}
		slot->id = id;
		slot->name = name ? rl_strdup(name) : NULL;
		slot->cb = cb;
		slot->next = owner->slots;
		owner->slots = slot;
	}
}

/** The owner of a name (an action, a tag of XML): the handles of Raven are 32 bits, these are above. */
static uint64_t rl_name_key(const char* name) {
	uint32_t hash = 2166136261u;
	for (; *name; name++) hash = (hash ^ (uint8_t)*name) * 16777619u;
	return (((uint64_t)1) << 40) | hash;
}

#define RL_KEY_APPLICATION 0

/** Called by Raven when a widget is gone: its callbacks go with it. */
static void rl_t_RWidgetDestroyCallback(Rid widget, void* user_data);
static void rl_widget_destroyed(Rid widget, void* user_data) {
	rl_owner* owner = (rl_owner*)user_data;
	rl_owner_unlink(owner);
	if (owner->destroy_cb) rl_t_RWidgetDestroyCallback(widget, owner->destroy_cb);
	else if (owner->native_destroy) owner->native_destroy(widget, owner->native_destroy_user);
	rl_owner_free(owner);
}

/** Forgets the callbacks of the windows that are gone. */
static void rl_windows_sweep(void) {
	rl_owner* owner = rl_windows;
	while (owner) {
		rl_owner* next = owner->window_next;
		if (!RL(Rwindow_is_valid)((Rid)owner->key)) {
			rl_owner_unlink(owner);
			rl_owner_free(owner);
		}
		owner = next;
	}
}

/** After Rshutdown: nothing is left that can call back. */
static void rl_release_all(void) {
	size_t i;
	for (i = 0; i < RL_BUCKETS; i++) {
		rl_owner* owner = rl_owners[i];
		rl_owners[i] = NULL;
		while (owner) {
			rl_owner* next = owner->next;
			rl_owner_free(owner);
			owner = next;
		}
	}
	rl_windows = NULL;
	while (rl_tasks) {
		rl_cb* cb = rl_tasks;
		rl_tasks = cb->task_next;
		cb->task_listed = 0;
		rl_cb_release(cb);
	}
}

static void rl_tasks_add(rl_cb* cb) {
	cb->task_next = rl_tasks;
	cb->task_prev = NULL;
	if (rl_tasks) rl_tasks->task_prev = cb;
	rl_tasks = cb;
	cb->task_listed = 1;
}

/** Is a dialog for this widget still waiting for its callback? */
static int rl_dialog_pending(Rid content, uint32_t id) {
	rl_owner* owner = rl_owner_find(content);
	rl_slot* slot;
	if (!owner) return 0;
	for (slot = owner->slots; slot; slot = slot->next) {
		if (slot->id == id) return 1;
	}
	return 0;
}

static void rl_tray_forget(Rid tray) {
	rl_owner* owner = rl_owner_find(tray);
	if (owner) {
		rl_owner_unlink(owner);
		rl_owner_free(owner);
	}
}

/** The main thread of a Lua state, on whose stack the callbacks run. */
static lua_State* rl_main(lua_State* L) {
	lua_State* main;
#if LUA_VERSION_NUM >= 502
	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
#else
	/* (Lua 5.1 cannot tell: the thread that loaded the module, kept by luaopen_raven) */
	lua_getfield(L, LUA_REGISTRYINDEX, RL_MAIN);
#endif
	main = lua_tothread(L, -1);
	lua_pop(L, 1);
	return main ? main : L;
}

/** When a Lua state is closed: its callbacks stay where Raven has them, but do nothing any more. */
static void rl_forget_cb(rl_cb* cb, lua_State* main) {
	if (cb && cb->L == main) {
		cb->L = NULL;
		cb->fn[0] = cb->fn[1] = cb->user = cb->keep = LUA_NOREF;
	}
}

static int rl_state_closed(lua_State* L) {
	lua_State* main = rl_main(L);
	size_t i;
	rl_owner* owner;
	rl_slot* slot;
	rl_cb* cb;
	for (i = 0; i < RL_BUCKETS; i++) {
		for (owner = rl_owners[i]; owner; owner = owner->next) {
			for (slot = owner->slots; slot; slot = slot->next) rl_forget_cb(slot->cb, main);
			rl_forget_cb(owner->destroy_cb, main);
		}
	}
	for (cb = rl_tasks; cb; cb = cb->task_next) rl_forget_cb(cb, main);
	rl_forget_cb(rl_error_cb, main);
	return 0;
}

/* What a callback argument was given as. */
enum { RL_CALLBACK_NULL, RL_CALLBACK_NATIVE, RL_CALLBACK_FUNCTION };

/** Reads a callback argument: a Lua function (or a callable value), a native function (light userdata), or nil. */
static int rl_callback_arg(lua_State* L, int i, void** native) {
	*native = NULL;
	switch (lua_type(L, i)) {
		case LUA_TNONE: case LUA_TNIL: return RL_CALLBACK_NULL;
		case LUA_TFUNCTION: return RL_CALLBACK_FUNCTION;
		case LUA_TLIGHTUSERDATA:
			*native = lua_touserdata(L, i);
			return *native ? RL_CALLBACK_NATIVE : RL_CALLBACK_NULL;
		default:
			if (luaL_getmetafield(L, i, "__call")) {
				lua_pop(L, 1);
				return RL_CALLBACK_FUNCTION;
			}
	}
	return luaL_argerror(L, i, "expected a function, a native function (light userdata) or nil");
}

static void rl_check_thread(lua_State* L) {
	if (!rl_on_thread()) luaL_error(L, "raven: a Lua callback can only be given from the thread that loaded raven");
}

/** A callback for the functions at f0 and f1 (0: none) and the user_data at `user` (0: none). */
static rl_cb* rl_cb_new(lua_State* L, int f0, int f1, int user) {
	int refs[3] = {LUA_NOREF, LUA_NOREF, LUA_NOREF};
	int positions[3];
	int i;
	rl_cb* cb;
	positions[0] = f0;
	positions[1] = f1;
	positions[2] = user;
	rl_check_thread(L);
	for (i = 0; i < 3; i++) {
		if (positions[i] > 0 && !lua_isnoneornil(L, positions[i])) {
			lua_pushvalue(L, positions[i]);
			refs[i] = luaL_ref(L, LUA_REGISTRYINDEX);
		}
	}
	cb = (rl_cb*)calloc(1, sizeof(rl_cb));
	if (!cb) {
		for (i = 0; i < 3; i++) luaL_unref(L, LUA_REGISTRYINDEX, refs[i]);
		luaL_error(L, "raven: out of memory");
		return NULL;
	}
	cb->L = rl_main(L);
	cb->fn[0] = refs[0];
	cb->fn[1] = refs[1];
	cb->user = refs[2];
	cb->keep = LUA_NOREF;
	return cb;
}

/** Pushes the function `index` of a callback; 0 when it has none. */
static int rl_push_fn(lua_State* L, rl_cb* cb, int index) {
	if (cb->fn[index] == LUA_NOREF) return 0;
	lua_rawgeti(L, LUA_REGISTRYINDEX, cb->fn[index]);
	return 1;
}

static void rl_push_user(lua_State* L, rl_cb* cb) {
	if (cb->user == LUA_NOREF || cb->user == LUA_REFNIL) lua_pushnil(L);
	else lua_rawgeti(L, LUA_REGISTRYINDEX, cb->user);
}

/** Keeps the values from `first` to the top (texts of a structure filled by Lua) until the next call. */
static void rl_keep(lua_State* L, rl_cb* cb, int first) {
	int top = lua_gettop(L), i;
	rl_unref(L, &cb->keep);
	if (top < first) return;
	lua_createtable(L, top - first + 1, 0);
	for (i = first; i <= top; i++) {
		lua_pushvalue(L, i);
		lua_rawseti(L, -2, i - first + 1);
	}
	cb->keep = luaL_ref(L, LUA_REGISTRYINDEX);
}

static int rl_result_int(lua_State* L, int i) {
	switch (lua_type(L, i)) {
		case LUA_TBOOLEAN: return lua_toboolean(L, i);
		case LUA_TNUMBER: return rl_int(L, i, NULL);
		default: return 0;
	}
}

static Rid rl_result_id(lua_State* L, int i) {
	if (lua_type(L, i) != LUA_TNUMBER) return R_INVALID_ID;
	return (Rid)rl_u32(L, i, NULL);
}

/** A text returned to C: it lives until the next call of this callback. */
static const char* rl_result_text(lua_State* L, rl_cb* cb, int i, int nullable) {
	size_t n;
	const char* text;
	if (!lua_isstring(L, i)) return nullable ? NULL : "";
	text = lua_tolstring(L, i, &n);
	if (n + 1 > cb->text_cap) {
		char* grown = (char*)realloc(cb->text, n + 1);
		if (!grown) return "";
		cb->text = grown;
		cb->text_cap = n + 1;
	}
	memcpy(cb->text, text, n + 1);
	return cb->text;
}

/** What a callback raised: to the error callback of the program, else to stderr. */
static int rl_reporting;

static void rl_report(lua_State* L, const char* message) {
	rl_cb* handler = rl_error_cb;
	if (handler && handler->L == rl_main(L) && !rl_reporting && rl_cb_enter(handler)) {
		rl_reporting = 1;
		if (rl_push_fn(L, handler, 0)) {
			lua_pushinteger(L, (lua_Integer)R_ERROR_SCRIPT);
			lua_pushinteger(L, (lua_Integer)R_SEVERITY_ERROR);
			lua_pushfstring(L, "error in a Lua callback: %%s", message);
			if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
				fprintf(stderr, "Raven: error in a Lua callback: %%s\n(and in the error callback: %%s)\n", message, lua_tostring(L, -1));
				lua_pop(L, 1);
			}
		}
		rl_reporting = 0;
		rl_cb_leave(handler);
		return;
	}
	fprintf(stderr, "Raven: error in a Lua callback: %%s\n", message);
	fflush(stderr);
}

static int rl_traceback(lua_State* L) {
	const char* message = lua_tostring(L, 1);
	if (!message) {
		if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING) return 1;
		message = lua_pushfstring(L, "(error object is a %%s value)", luaL_typename(L, 1));
	}
#if LUA_VERSION_NUM >= 502
	luaL_traceback(L, L, message, 1);
#else
	lua_getglobal(L, "debug");
	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, "traceback");
		if (lua_isfunction(L, -1)) {
			lua_pushstring(L, message);
			lua_pushinteger(L, 2);
			lua_call(L, 2, 1);
			return 1;
		}
	}
	lua_pushstring(L, message);
#endif
	return 1;
}

/** One call of a callback: `k` pushes the arguments, calls the function and reads its results. */
typedef struct rl_job {
	lua_CFunction k;
	void* context;
} rl_job;

static int rl_job_run(lua_State* L) {
	rl_job* job = (rl_job*)lua_touserdata(L, 1);
	int handler;
	lua_pushcfunction(L, rl_traceback);
	handler = lua_gettop(L);
	lua_pushcfunction(L, job->k);
	lua_pushlightuserdata(L, job->context);
	if (lua_pcall(L, 1, 0, handler) != LUA_OK) {
		const char* message = lua_tostring(L, -1);
		rl_report(L, message ? message : "(no message)");
	}
	return 0;
}

/** Runs `k` protected on the main thread of the Lua state of `cb`: nothing raised leaves it. */
static void rl_run(rl_cb* cb, lua_CFunction k, void* context) {
	lua_State* L = cb->L;
	rl_job job;
	int top, status;
	if (!L) return;
	if (!rl_on_thread()) {
		fputs("Raven: a Lua callback was called on another thread than the one of Lua; it was left out\n", stderr);
		return;
	}
	if (!lua_checkstack(L, 8)) return;
	top = lua_gettop(L);
	job.k = k;
	job.context = context;
#if LUA_VERSION_NUM >= 502
	/* (light C functions and light userdata allocate nothing: this cannot raise) */
	lua_pushcfunction(L, rl_job_run);
	lua_pushlightuserdata(L, &job);
	status = lua_pcall(L, 1, 0, 0);
#else
	status = lua_cpcall(L, rl_job_run, &job);
#endif
	if (status != LUA_OK) fputs("Raven: not enough memory to call a Lua callback\n", stderr);
	lua_settop(L, top);
}

/* ---- the two callbacks that can come from other threads: brought to the thread of Lua by Rtask_post */

static int rl_k_task(lua_State* L) {
	rl_cb* cb = (rl_cb*)lua_touserdata(L, 1);
	if (!rl_push_fn(L, cb, 0)) return 0;
	rl_push_user(L, cb);
	lua_call(L, 1, 0);
	return 0;
}

static void rl_run_task(void* user_data) {
	rl_cb* cb = (rl_cb*)user_data;
	cb->depth++;
	rl_run(cb, rl_k_task, cb);
	rl_cb_leave(cb);
}

/* Rtask_post runs it on the UI thread; Rtask_run_async on a worker, which posts it. */
static void rl_t_RTaskCallback(void* user_data) {
	if (rl_on_thread()) rl_run_task(user_data);
	else RL(Rtask_post)(rl_run_task, user_data);
}

typedef struct rl_error_call {
	RError code;
	RSeverity severity;
	const char* detail;
} rl_error_call;

static int rl_k_error(lua_State* L) {
	rl_error_call* call = (rl_error_call*)lua_touserdata(L, 1);
	if (!rl_error_cb || !rl_push_fn(L, rl_error_cb, 0)) return 0;
	lua_pushinteger(L, (lua_Integer)call->code);
	lua_pushinteger(L, (lua_Integer)call->severity);
	rl_push_text(L, call->detail);
	lua_call(L, 3, 0);
	return 0;
}

static void rl_run_error(RError code, RSeverity severity, const char* detail) {
	rl_cb* cb = rl_error_cb;
	rl_error_call call;
	if (!rl_cb_enter(cb)) return;
	call.code = code;
	call.severity = severity;
	call.detail = detail;
	rl_run(cb, rl_k_error, &call);
	rl_cb_leave(cb);
}

typedef struct rl_posted_error {
	RError code;
	RSeverity severity;
	char* detail;
} rl_posted_error;

static void rl_run_posted_error(void* user_data) {
	rl_posted_error* posted = (rl_posted_error*)user_data;
	rl_run_error(posted->code, posted->severity, posted->detail);
	free(posted->detail);
	free(posted);
}

static void rl_t_RErrorCallback(RError code, RSeverity severity, const char* detail) {
	rl_posted_error* posted;
	if (rl_on_thread()) {
		rl_run_error(code, severity, detail);
		return;
	}
	if (!RL(Ris_initialized)()) return;
	posted = (rl_posted_error*)calloc(1, sizeof(rl_posted_error));
	if (!posted) return;
	posted->code = code;
	posted->severity = severity;
	posted->detail = detail ? rl_strdup(detail) : NULL;
	RL(Rtask_post)(rl_run_posted_error, posted);
}
'''

OPEN = r'''
/* ---- opening the library (unless Raven is linked: RAVEN_LUA_LINK) */

#ifndef RAVEN_LUA_LINK
static int rl_opened;

#ifdef _WIN32
/** The functions of the library; NULL when one is missing (`missing` says which). */
static int rl_resolve(HMODULE library, char* missing, size_t size) {
	size_t i;
	for (i = 0; i < sizeof(rl_symbols) / sizeof(rl_symbols[0]); i++) {
		FARPROC symbol = GetProcAddress(library, rl_symbols[i].name);
		if (!symbol) {
			snprintf(missing, size, "the library has no function %%s (another version of Raven?)", rl_symbols[i].name);
			return 0;
		}
		memcpy((char*)&rl + rl_symbols[i].offset, &symbol, sizeof(symbol));
	}
	return 1;
}

/** This module may itself be raven.dll: the library of Raven is then another file of that name. */
static HMODULE rl_load_beside(const wchar_t* folder, const wchar_t* self) {
	wchar_t path[MAX_PATH + 16];
	size_t n = wcslen(folder);
	if (n == 0 || n > MAX_PATH) return NULL;
	memcpy(path, folder, n * sizeof(wchar_t));
	if (path[n - 1] != L'\\' && path[n - 1] != L'/') path[n++] = L'\\';
	memcpy(path + n, L"raven.dll", sizeof(L"raven.dll"));
	if (_wcsicmp(path, self) == 0 || GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) return NULL;
	return LoadLibraryExW(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
}

static const char* rl_load(char* message, size_t size) {
	HMODULE self = NULL, library = NULL;
	wchar_t self_path[MAX_PATH], buffer[MAX_PATH];
	const wchar_t* explicit_path = _wgetenv(L"RAVEN_LIBRARY");
	GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCWSTR)(void*)&rl_load, &self);
	self_path[0] = 0;
	if (self) GetModuleFileNameW(self, self_path, MAX_PATH);
	if (explicit_path && explicit_path[0]) {
		library = LoadLibraryExW(explicit_path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
		if (!library) {
			snprintf(message, size, "cannot open RAVEN_LIBRARY (error %%lu)", GetLastError());
			return message;
		}
	} else {
		/* the folder of the program, then those of PATH */
		DWORD n = GetModuleFileNameW(NULL, buffer, MAX_PATH);
		if (n > 0 && n < MAX_PATH) {
			wchar_t* slash = wcsrchr(buffer, L'\\');
			if (slash) {
				*slash = 0;
				library = rl_load_beside(buffer, self_path);
			}
		}
		if (!library) {
			DWORD length = GetEnvironmentVariableW(L"PATH", NULL, 0);
			wchar_t* paths = length ? (wchar_t*)malloc(length * sizeof(wchar_t)) : NULL;
			if (paths && GetEnvironmentVariableW(L"PATH", paths, length) > 0) {
				wchar_t* folder = paths;
				while (folder && !library) {
					wchar_t* end = wcschr(folder, L';');
					if (end) *end = 0;
					library = rl_load_beside(folder, self_path);
					folder = end ? end + 1 : NULL;
				}
			}
			free(paths);
		}
		if (!library) {
			library = LoadLibraryW(L"raven.dll");
			if (library == self) library = NULL;
		}
		if (!library) {
			snprintf(message, size, "cannot find raven.dll (set RAVEN_LIBRARY to its path)");
			return message;
		}
	}
	if (!rl_resolve(library, message, size)) return message;
	return NULL;
}
#else
static const char* rl_load(char* message, size_t size) {
	static const char* const names[] = {
#ifdef __APPLE__
		"libraven.dylib", "libraven.0.dylib",
#else
		"libraven.so", "libraven.so.0",
#endif
	};
	const char* explicit_path = getenv("RAVEN_LIBRARY");
	void* library = NULL;
	size_t i;
	if (explicit_path && explicit_path[0]) {
		library = dlopen(explicit_path, RTLD_NOW | RTLD_LOCAL);
	} else {
		for (i = 0; i < sizeof(names) / sizeof(names[0]) && !library; i++) library = dlopen(names[i], RTLD_NOW | RTLD_LOCAL);
	}
	if (!library) {
		const char* reason = dlerror();
		snprintf(message, size, "cannot open the Raven library (%%s); set RAVEN_LIBRARY to its path", reason ? reason : "not found");
		return message;
	}
	for (i = 0; i < sizeof(rl_symbols) / sizeof(rl_symbols[0]); i++) {
		void* symbol = dlsym(library, rl_symbols[i].name);
		if (!symbol) {
			snprintf(message, size, "the library has no function %%s (another version of Raven?)", rl_symbols[i].name);
			return message;
		}
		memcpy((char*)&rl + rl_symbols[i].offset, &symbol, sizeof(symbol));
	}
	return NULL;
}
#endif
#endif

/* ---- the module */

typedef struct rl_constant {
	const char* name;
	lua_Number value;
} rl_constant;

static const rl_constant rl_constants[] = {
%(constants)s
	{NULL, 0}
};

RAVEN_LUA_API int luaopen_raven(lua_State* L) {
	const rl_constant* constant;
#ifndef RAVEN_LUA_LINK
	if (!rl_opened) {
		char message[512];
		const char* error = rl_load(message, sizeof(message));
		if (error) return luaL_error(L, "raven: %%s", error);
		rl_opened = 1;
	}
#endif
	if (!rl_ready) {
		rl_thread_set();
		rl_ready = 1;
	} else if (!rl_on_thread()) {
		return luaL_error(L, "raven: already loaded on another thread (Raven has one UI thread)");
	}
	luaL_checkstack(L, 8, "raven");

#if LUA_VERSION_NUM < 502
	/* the thread on whose stack the callbacks run: the main one when it loads the module */
	lua_getfield(L, LUA_REGISTRYINDEX, RL_MAIN);
	if (lua_isnil(L, -1)) {
		lua_pushthread(L);
		lua_setfield(L, LUA_REGISTRYINDEX, RL_MAIN);
	}
	lua_pop(L, 1);
#endif

	/* when the state is closed, its callbacks are left alone */
	lua_getfield(L, LUA_REGISTRYINDEX, RL_SENTINEL);
	if (lua_isnil(L, -1)) {
		lua_newuserdata(L, 1);
		lua_createtable(L, 0, 1);
		lua_pushcfunction(L, rl_state_closed);
		lua_setfield(L, -2, "__gc");
		lua_setmetatable(L, -2);
		lua_setfield(L, LUA_REGISTRYINDEX, RL_SENTINEL);
	}
	lua_pop(L, 1);

	lua_createtable(L, 0, (int)(sizeof(rl_constants) / sizeof(rl_constants[0])));
	for (constant = rl_constants; constant->name; constant++) {
		lua_pushnumber(L, constant->value);
		lua_setfield(L, -2, constant->name);
	}
	lua_setfield(L, LUA_REGISTRYINDEX, RL_NAMES);

	lua_createtable(L, 0, (int)(sizeof(rl_functions) / sizeof(rl_functions[0]) + sizeof(rl_constants) / sizeof(rl_constants[0])));
	rl_setfuncs(L, rl_functions);
	for (constant = rl_constants; constant->name; constant++) {
#if LUA_VERSION_NUM >= 503
		lua_pushinteger(L, (lua_Integer)constant->value);
#else
		lua_pushnumber(L, constant->value);
#endif
		lua_setfield(L, -2, constant->name);
	}
%(strings)s
	return 1;
}
'''


# ---- the parameters of a function


class Item:
    def __init__(self, index, p, kind, detail):
        self.index = index
        self.p = p
        self.kind = kind
        self.detail = detail
        self.shape = None
        self.pos = None       # its position among the Lua arguments
        self.extra = {}


def find_param(f, *names):
    for i, p in enumerate(f.params):
        if p.name in names:
            return i
    return None


def plan(api, f):
    """How each parameter of `f` travels; the same rules as the Node.js addon where they meet."""
    items = []
    skip = set()
    width = find_param(f, "width", "pixels_width")
    height = find_param(f, "height", "pixels_height")
    for index, p in enumerate(f.params):
        if index in skip:
            continue
        kind, detail = classify(p.ctype, api)
        following = f.params[index + 1] if index + 1 < len(f.params) else None
        it = Item(index, p, kind, detail)
        items.append(it)
        if kind == "out":
            it.shape = "out"
        elif kind == "cstr_out":
            it.shape = "cstr_out"
        elif kind == "cstr_array":
            it.shape = "cstr_array"
            if following is not None and classify(following.ctype, api) == ("scalar", "int") and "count" in following.name:
                it.extra["count"] = index + 1
                skip.add(index + 1)
        elif kind == "cstruct_ptr" and following is not None and following.name == "count" and classify(following.ctype, api) == ("scalar", "int"):
            it.shape = "struct_array"
            it.extra["count"] = index + 1
            skip.add(index + 1)
        elif kind in ("cstr", "cptr", "cbytes") and following is not None and following.ctype == "size_t" and following.name.endswith("size"):
            it.shape = "sized"
            it.extra["size"] = index + 1
            skip.add(index + 1)
        elif kind == "cbytes" and width is not None and height is not None:
            it.shape = "pixels"
        elif kind == "bytes" and width is not None and height is not None:
            it.shape = "pixels_out"
        elif kind == "carray":
            if detail not in ("double", "float"):
                raise ValueError("Lua: no array of %s (%s)" % (detail, f.name))
            it.shape = "carray"
        elif kind == "callback" and f.name in NATIVE_ONLY:
            it.shape = "native_callback"
        elif kind == "callback":
            it.shape = "callback"
        elif kind == "cstruct_ptr":
            it.shape = "struct_in"
        elif kind == "struct_ptr":
            it.shape = "struct_inout"
        elif kind == "scalar":
            it.shape = "scalar"
        elif kind == "enum":
            it.shape = "enum"
        elif kind == "cstr":
            it.shape = "cstr"
        elif kind in ("ptr", "cptr", "bytes", "cbytes", "opaque_ptr"):
            it.shape = "ptr"
        else:
            raise ValueError("Lua: no parameter of the type %s (%s)" % (p.ctype, f.name))
    # positions of the Lua arguments: the results written through pointers take none
    pos = 1
    for it in items:
        if it.shape in ("out", "cstr_out"):
            continue
        it.pos = pos
        pos += 1
        if "count" in it.extra or "size" in it.extra:
            it.extra["opt_pos"] = pos
            pos += 1
    return items


def array_length(f, it):
    """The C expression of how many numbers an array parameter holds."""
    count = find_param(f, "count")
    if count is not None:
        return "a%d" % count
    rows, columns = find_param(f, "rows"), find_param(f, "columns")
    if rows is not None and columns is not None:
        return "(a%d > 0 && a%d > 0 ? a%d * a%d : 0)" % (rows, columns, rows, columns)
    raise ValueError("Lua: how long is the array %s of %s?" % (it.p.name, f.name))


def pixels_expr(f):
    w = find_param(f, "width", "pixels_width")
    h = find_param(f, "height", "pixels_height")
    return "a%d" % w, "a%d" % h


# ---- structures


def struct_fields(api, s):
    """[(field, kind, detail, count_field)]: a list and its count (paths, path_count) become one field."""
    result = []
    skip = set()
    for i, fld in enumerate(s.fields):
        if i in skip:
            continue
        following = s.fields[i + 1] if i + 1 < len(s.fields) else None
        if fld.ctype == "fnptr":
            result.append((fld, "fnptr", None, None))
            continue
        if fld.array:
            if fld.ctype == "char":
                result.append((fld, "chars", None, None))
            else:
                kind, detail = classify(fld.ctype, api)
                if kind != "scalar":
                    raise ValueError("Lua: no array of %s in %s" % (fld.ctype, s.name))
                result.append((fld, "numbers", detail, None))
            continue
        kind, detail = classify(fld.ctype, api)
        if kind in ("cstr_array", "cstruct_ptr") and following is not None and following.name.endswith("count"):
            result.append((fld, "list_" + kind, detail, following))
            skip.add(i + 1)
            continue
        if kind in ("cptr", "cbytes") and following is not None and following.ctype == "size_t" and following.name.endswith("size"):
            # bytes and their size (RDataItem.data, size) become one string
            result.append((fld, "sized", detail, following))
            skip.add(i + 1)
            continue
        if kind in ("scalar", "enum", "cstr", "ptr"):
            result.append((fld, kind, detail, None))
            continue
        raise ValueError("Lua: no field of the type %s in %s" % (fld.ctype, s.name))
    return result


def struct_code(api, s, wanted_in, wanted_out):
    """rl_set_X (C to Lua, into the table at `t`) and rl_get_X (the table at `t` to C)."""
    out = []
    fields = struct_fields(api, s)
    if s.name in wanted_out:
        out.append("static void rl_set_%s(lua_State* L, int t, const %s* s) {" % (s.name, s.name))
        out.append("\tluaL_checkstack(L, 4, \"raven\");")
        for fld, kind, detail, count in fields:
            n = fld.name
            if kind == "fnptr":
                continue
            if kind == "scalar":
                out.append("\t%s;" % (PUSH[detail] % ("s->" + n)))
            elif kind == "enum":
                out.append("\tlua_pushinteger(L, (lua_Integer)s->%s);" % n)
            elif kind == "cstr":
                out.append("\trl_push_text(L, s->%s);" % n)
            elif kind == "ptr":
                out.append("\trl_push_ptr(L, s->%s);" % n)
            elif kind == "sized":
                out.append("\tif (s->%s) lua_pushlstring(L, (const char*)s->%s, s->%s);" % (n, n, count.name))
                out.append("\telse lua_pushnil(L);")
            elif kind == "chars":
                out.append("\trl_push_chars(L, s->%s, sizeof(s->%s));" % (n, n))
            elif kind == "numbers":
                out.append("\t{")
                out.append("\t\tint k;")
                out.append("\t\tlua_createtable(L, %d, 0);" % fld.array)
                out.append("\t\tfor (k = 0; k < %d; k++) {" % fld.array)
                out.append("\t\t\t%s;" % (PUSH[detail] % ("s->%s[k]" % n)))
                out.append("\t\t\tlua_rawseti(L, -2, k + 1);")
                out.append("\t\t}")
                out.append("\t}")
            elif kind == "list_cstr_array":
                out.append("\t{")
                out.append("\t\tint k, count = s->%s && s->%s > 0 ? s->%s : 0;" % (n, count.name, count.name))
                out.append("\t\tlua_createtable(L, count, 0);")
                out.append("\t\tfor (k = 0; k < count; k++) {")
                out.append("\t\t\trl_push_text(L, s->%s[k]);" % n)
                out.append("\t\t\tlua_rawseti(L, -2, k + 1);")
                out.append("\t\t}")
                out.append("\t}")
            elif kind == "list_cstruct_ptr":
                out.append("\t{")
                out.append("\t\tint k, count = s->%s && s->%s > 0 ? s->%s : 0;" % (n, count.name, count.name))
                out.append("\t\tlua_createtable(L, count, 0);")
                out.append("\t\tfor (k = 0; k < count; k++) {")
                out.append("\t\t\tlua_createtable(L, 0, 4);")
                out.append("\t\t\trl_set_%s(L, lua_gettop(L), &s->%s[k]);" % (detail, n))
                out.append("\t\t\tlua_rawseti(L, -2, k + 1);")
                out.append("\t\t}")
                out.append("\t}")
            out.append('\tlua_setfield(L, t, "%s");' % n)
            if count is not None:
                out.append("\tlua_pushinteger(L, (lua_Integer)s->%s);" % count.name)
                out.append('\tlua_setfield(L, t, "%s");' % count.name)
        out.append("}")
        out.append("")
        out.append("static void rl_push_%s(lua_State* L, const %s* s) {" % (s.name, s.name))
        out.append("\tif (!s) {")
        out.append("\t\tlua_pushnil(L);")
        out.append("\t\treturn;")
        out.append("\t}")
        out.append("\tlua_createtable(L, 0, %d);" % len(s.fields))
        out.append("\trl_set_%s(L, lua_gettop(L), s);" % s.name)
        out.append("}")
        out.append("")
    if s.name in wanted_in:
        # the texts and the lists stay on the stack above the table: they must outlive the call
        out.append("static void rl_get_%s(lua_State* L, int t, %s* s) {" % (s.name, s.name))
        out.append("\tint v;")
        out.append("\tmemset(s, 0, sizeof(*s));")
        out.append("\tluaL_checkstack(L, %d, \"raven\");" % (len(s.fields) + 4))
        for fld, kind, detail, count in fields:
            n = fld.name
            label = '"%s.%s"' % (s.name, n)
            out.append('\tlua_getfield(L, t, "%s");' % n)
            out.append("\tv = lua_gettop(L);")
            keep = False
            if kind == "scalar":
                out.append("\tif (!lua_isnil(L, v)) s->%s = %s;" % (n, READ_AT[detail] % ("v", label)))
            elif kind == "enum":
                out.append("\tif (!lua_isnil(L, v)) s->%s = (%s)rl_enum(L, v, %s);" % (n, detail, label))
            elif kind == "cstr":
                out.append("\ts->%s = rl_str(L, v, %s);" % (n, label))
                keep = True
            elif kind == "ptr":
                out.append("\ts->%s = rl_ptr(L, v, %s);" % (n, label))
            elif kind == "sized":
                out.append("\tif (!lua_isnil(L, v)) {")
                out.append("\t\tsize_t n;")
                out.append("\t\ts->%s = rl_lstr(L, v, &n, %s);" % (n, label))
                out.append("\t\ts->%s = n;" % count.name)
                out.append("\t}")
                keep = True
            elif kind == "fnptr":
                out.append("\t{")
                out.append("\t\tvoid* address = rl_ptr(L, v, %s);" % label)
                out.append("\t\tmemcpy(&s->%s, &address, sizeof(address));" % n)
                out.append("\t}")
            elif kind == "chars":
                out.append("\tif (!lua_isnil(L, v)) {")
                out.append("\t\tsize_t n;")
                out.append("\t\tconst char* text = rl_lstr(L, v, &n, %s);" % label)
                out.append("\t\tif (n >= sizeof(s->%s)) n = sizeof(s->%s) - 1;" % (n, n))
                out.append("\t\tmemcpy(s->%s, text, n);" % n)
                out.append("\t}")
            elif kind == "numbers":
                out.append("\tif (lua_istable(L, v)) {")
                out.append("\t\tint k;")
                out.append("\t\tfor (k = 0; k < %d; k++) {" % fld.array)
                out.append("\t\t\tlua_rawgeti(L, v, k + 1);")
                out.append("\t\t\tif (!lua_isnil(L, -1)) s->%s[k] = %s;" % (n, READ_AT[detail] % ("lua_gettop(L)", label)))
                out.append("\t\t\tlua_pop(L, 1);")
                out.append("\t\t}")
                out.append("\t}")
            elif kind == "list_cstr_array":
                out.append("\tif (lua_istable(L, v)) {")
                out.append("\t\tint count;")
                out.append("\t\ts->%s = (const char* const*)rl_strings(L, v, &count);" % n)
                out.append("\t\ts->%s = count;" % count.name)
                out.append("\t}")
                keep = True
            elif kind == "list_cstruct_ptr":
                out.append("\tif (lua_istable(L, v)) {")
                out.append("\t\tint k, count = (int)rl_rawlen(L, v);")
                out.append("\t\t%s* list = (%s*)rl_scratch(L, (size_t)(count > 0 ? count : 0) * sizeof(%s));" % (detail, detail, detail))
                out.append("\t\tfor (k = 0; k < count; k++) {")
                out.append("\t\t\tlua_rawgeti(L, v, k + 1);")
                out.append("\t\t\tif (lua_istable(L, -1)) rl_get_%s(L, lua_gettop(L), &list[k]);" % detail)
                out.append("\t\t}")
                out.append("\t\ts->%s = count > 0 ? list : NULL;" % n)
                out.append("\t\ts->%s = count;" % count.name)
                out.append("\t}")
                keep = True
            if not keep:
                out.append("\tlua_settop(L, v - 1);")
        out.append("}")
        out.append("")
    return out


def struct_needs(api):
    """Which structures go from Lua to C (wanted_in) and from C to Lua (wanted_out)."""
    wanted_in, wanted_out = set(), set()
    for f in api.functions:
        for it in plan(api, f):
            if it.shape in ("struct_in", "struct_array"):
                wanted_in.add(it.detail)
            elif it.shape == "struct_inout":
                wanted_in.add(it.detail)
                wanted_out.add(it.detail)
    for c in api.callbacks:
        if c.name not in used_callbacks(api):
            continue
        for p in c.params:
            kind, detail = classify(p.ctype, api)
            if kind == "cstruct_ptr":
                wanted_out.add(detail)
            elif kind == "struct_ptr":
                wanted_out.add(detail)
                wanted_in.add(detail)
    # the structures inside them
    changed = True
    while changed:
        changed = False
        for s in api.structs:
            for fld, kind, detail, count in struct_fields(api, s):
                if kind == "list_cstruct_ptr":
                    for wanted in (wanted_in, wanted_out):
                        if s.name in wanted and detail not in wanted:
                            wanted.add(detail)
                            changed = True
    return wanted_in, wanted_out


# ---- the callbacks


def used_callbacks(api):
    """The callback types that a Lua function can be given for."""
    used = set()
    for f in api.functions:
        if f.name in NATIVE_ONLY:
            continue
        for p in f.params:
            if classify(p.ctype, api)[0] == "callback":
                used.add(p.ctype)
    return used


def trampoline(api, c, index):
    """The C function Raven calls, and the protected part that calls the Lua function."""
    ret_kind, ret_detail = classify(c.ret, api)
    user_index = None
    for i, p in enumerate(c.params):
        if p.name == "user_data" and classify(p.ctype, api)[0] == "ptr":
            user_index = i
    if user_index is None:
        raise ValueError("Lua: the callback %s has no user_data" % c.name)
    out = []
    # what the protected part needs
    out.append("typedef struct rl_c_%s {" % c.name)
    out.append("\trl_cb* cb;")
    for i, p in enumerate(c.params):
        if i != user_index:
            out.append("\t%s a%d;" % (p.ctype, i))
    if ret_kind != "void":
        out.append("\t%s r;" % c.ret)
    out.append("} rl_c_%s;" % c.name)
    out.append("")
    out.append("static int rl_k_%s(lua_State* L) {" % c.name)
    out.append("\trl_c_%s* c = (rl_c_%s*)lua_touserdata(L, 1);" % (c.name, c.name))
    writable = [(i, p) for i, p in enumerate(c.params) if classify(p.ctype, api)[0] == "struct_ptr"]
    outs = [(i, p) for i, p in enumerate(c.params) if classify(p.ctype, api)[0] == "out"]
    if writable:
        out.append("\tint first;")
    out.append("\tluaL_checkstack(L, %d, \"raven\");" % (len(c.params) + len(writable) + 8))
    # the structures the function fills: a table each, below the function, read back after
    tables = {}
    for n, (i, p) in enumerate(writable):
        detail = classify(p.ctype, api)[1]
        out.append("\trl_push_%s(L, c->a%d);" % (detail, i))
        tables[i] = n + 2
    out.append("\tif (!rl_push_fn(L, c->cb, %d)) return 0;" % index)
    for i, p in enumerate(c.params):
        kind, detail = classify(p.ctype, api)
        a = "c->a%d" % i
        if i == user_index:
            out.append("\trl_push_user(L, c->cb);")
        elif kind == "scalar":
            out.append("\t%s;" % (PUSH[detail] % a))
        elif kind == "enum":
            out.append("\tlua_pushinteger(L, (lua_Integer)%s);" % a)
        elif kind == "cstr":
            out.append("\trl_push_text(L, %s);" % a)
        elif kind == "cstruct_ptr":
            out.append("\trl_push_%s(L, %s);" % (detail, a))
        elif kind == "struct_ptr":
            out.append("\tlua_pushvalue(L, %d);" % tables[i])
        elif kind == "out":
            if detail[0] == "enum":
                out.append("\tif (%s) lua_pushinteger(L, (lua_Integer)*%s); else lua_pushnil(L);" % (a, a))
            else:
                out.append("\tif (%s) %s; else lua_pushnil(L);" % (a, PUSH[detail[1]] % ("*" + a)))
        elif kind in ("ptr", "cptr", "bytes", "cbytes", "opaque_ptr"):
            out.append("\trl_push_ptr(L, %s);" % a)
        else:
            raise ValueError("Lua: no parameter of the type %s in a callback (%s)" % (p.ctype, c.name))
    results = (1 if ret_kind != "void" else 0) + len(outs)
    out.append("\tlua_call(L, %d, %d);" % (len(c.params), results))
    position = -results
    if ret_kind != "void":
        if ret_kind == "cstr":
            out.append("\tc->r = rl_result_text(L, c->cb, %d, %d);" % (position, 1 if c.name in NULLABLE_TEXT else 0))
        elif ret_detail == "id":
            out.append("\tc->r = rl_result_id(L, %d);" % position)
        elif ret_detail == "int":
            out.append("\tc->r = rl_result_int(L, %d);" % position)
        else:
            raise ValueError("Lua: no result of the type %s for a callback (%s)" % (c.ret, c.name))
        position += 1
    for i, p in outs:
        kind, detail = classify(p.ctype, api)
        base = p.ctype[:-1]
        if detail[0] == "enum":
            value = "(%s)rl_enum(L, %d, NULL)" % (base, position)
        else:
            value = "(%s)rl_number(L, lua_gettop(L) + %d + 1, NULL)" % (base, position)
        out.append("\tif (c->a%d && !lua_isnil(L, %d)) *c->a%d = %s;" % (i, position, i, value))
        position += 1
    if writable:
        out.append("\tlua_pop(L, %d);" % results)
        out.append("\tfirst = lua_gettop(L) + 1;")
        for i, p in writable:
            detail = classify(p.ctype, api)[1]
            out.append("\tif (c->a%d) rl_get_%s(L, %d, c->a%d);" % (i, detail, tables[i], i))
        out.append("\trl_keep(L, c->cb, first);")
    out.append("\treturn 0;")
    out.append("}")
    out.append("")
    params = ", ".join("%s a%d" % (p.ctype, i) for i, p in enumerate(c.params))
    out.append("static %s rl_t_%s(%s) {" % (c.ret, c.name, params))
    out.append("\trl_cb* cb = (rl_cb*)a%d;" % user_index)
    out.append("\trl_c_%s c;" % c.name)
    out.append("\tc.cb = cb;")
    for i, p in enumerate(c.params):
        if i != user_index:
            out.append("\tc.a%d = a%d;" % (i, i))
    if ret_kind != "void":
        if ret_kind == "cstr":
            out.append("\tc.r = %s;" % ("NULL" if c.name in NULLABLE_TEXT else '""'))
        elif ret_detail == "id":
            out.append("\tc.r = R_INVALID_ID;")
        else:
            out.append("\tc.r = 0;")
    out.append("\tif (!rl_cb_enter(cb)) return%s;" % ("" if ret_kind == "void" else " c.r"))
    out.append("\trl_run(cb, rl_k_%s, &c);" % c.name)
    out.append("\trl_cb_leave(cb);")
    if ret_kind != "void":
        out.append("\treturn c.r;")
    out.append("}")
    return "\n".join(out)


# ---- the functions


def wrapper(api, f, slots):
    items = plan(api, f)
    ret_kind, ret_detail = classify(f.ret, api)
    decls, reads, late, exprs = [], [], [], {}

    for it in items:
        a = "a%d" % it.index
        p = it.p
        if it.shape == "scalar":
            ctype, read = READ[it.detail]
            decls.append("%s %s;" % (ctype, a))
            reads.append("%s = %s;" % (a, read % it.pos))
            exprs[it.index] = "(%s)%s" % (p.ctype, a)
        elif it.shape == "enum":
            decls.append("int %s;" % a)
            reads.append("%s = rl_enum(L, %d, NULL);" % (a, it.pos))
            exprs[it.index] = "(%s)%s" % (it.detail, a)
        elif it.shape == "cstr":
            decls.append("const char* %s;" % a)
            reads.append("%s = rl_str(L, %d, NULL);" % (a, it.pos))
            exprs[it.index] = a
        elif it.shape == "sized":
            decls.append("const char* %s;" % a)
            decls.append("size_t %s_size;" % a)
            reads.append("%s = rl_lstr(L, %d, &%s_size, NULL);" % (a, it.pos, a))
            reads.append("%s_size = rl_size_opt(L, %d, %s_size);" % (a, it.extra["opt_pos"], a))
            exprs[it.index] = "(%s)%s" % (p.ctype, a)
            exprs[it.extra["size"]] = "%s_size" % a
        elif it.shape == "pixels":
            w, h = pixels_expr(f)
            decls.append("const char* %s;" % a)
            decls.append("size_t %s_size;" % a)
            reads.append("%s = rl_lstr(L, %d, &%s_size, NULL);" % (a, it.pos, a))
            late.append("rl_check_pixels(L, %d, %s, %s_size, %s, %s);" % (it.pos, a, a, w, h))
            exprs[it.index] = "(%s)%s" % (p.ctype, a)
        elif it.shape == "pixels_out":
            w, h = pixels_expr(f)
            decls.append("uint8_t* %s = NULL;" % a)
            decls.append("size_t %s_size = 0;" % a)
            late.append("if (lua_toboolean(L, %d)) {" % it.pos)
            late.append("\t%s_size = %s > 0 && %s > 0 ? (size_t)%s * (size_t)%s * 4 : 0;" % (a, w, h, w, h))
            late.append("\t%s = (uint8_t*)rl_scratch(L, %s_size);" % (a, a))
            late.append("}")
            exprs[it.index] = a
        elif it.shape == "carray":
            ctype = NUMBER_C[it.detail]
            decls.append("%s* %s;" % (ctype, a))
            late.append("%s = rl_%ss(L, %d, %s);" % (a, it.detail, it.pos, array_length(f, it)))
            exprs[it.index] = "(%s)%s" % (p.ctype, a)
        elif it.shape == "cstr_array":
            decls.append("const char** %s;" % a)
            decls.append("int %s_count;" % a)
            reads.append("%s = rl_strings(L, %d, &%s_count);" % (a, it.pos, a))
            if "count" in it.extra:
                reads.append("%s_count = rl_count_opt(L, %d, %s_count);" % (a, it.extra["opt_pos"], a))
                exprs[it.extra["count"]] = "%s_count" % a
            exprs[it.index] = "(const char* const*)%s" % a
        elif it.shape == "struct_array":
            s = it.detail
            decls.append("%s* %s = NULL;" % (s, a))
            decls.append("int %s_count = 0;" % a)
            reads.append("if (!lua_isnoneornil(L, %d)) {" % it.pos)
            reads.append("\tint k_;")
            reads.append("\tluaL_checktype(L, %d, LUA_TTABLE);" % it.pos)
            reads.append("\t%s_count = (int)rl_rawlen(L, %d);" % (a, it.pos))
            reads.append("\t%s = (%s*)rl_scratch(L, (size_t)%s_count * sizeof(%s));" % (a, s, a, s))
            reads.append("\tfor (k_ = 0; k_ < %s_count; k_++) {" % a)
            reads.append("\t\tluaL_checkstack(L, 4, \"raven\");")
            reads.append("\t\tlua_rawgeti(L, %d, k_ + 1);" % it.pos)
            reads.append("\t\tif (!lua_istable(L, -1)) luaL_argerror(L, %d, \"every element must be a table\");" % it.pos)
            reads.append("\t\trl_get_%s(L, lua_gettop(L), &%s[k_]);" % (s, a))
            reads.append("\t}")
            reads.append("}")
            reads.append("%s_count = rl_count_opt(L, %d, %s_count);" % (a, it.extra["opt_pos"], a))
            exprs[it.index] = "%s_count > 0 ? %s : NULL" % (a, a)
            exprs[it.extra["count"]] = "%s_count" % a
        elif it.shape == "struct_in":
            s = it.detail
            decls.append("%s %s_value;" % (s, a))
            decls.append("const %s* %s = NULL;" % (s, a))
            reads.append("if (lua_istable(L, %d)) {" % it.pos)
            reads.append("\trl_get_%s(L, %d, &%s_value);" % (s, it.pos, a))
            reads.append("\t%s = &%s_value;" % (a, a))
            reads.append("} else {")
            reads.append("\t%s = (const %s*)rl_ptr(L, %d, NULL);" % (a, s, it.pos))
            reads.append("}")
            exprs[it.index] = a
        elif it.shape == "struct_inout":
            s = it.detail
            decls.append("%s %s;" % (s, a))
            reads.append("if (lua_istable(L, %d)) rl_get_%s(L, %d, &%s);" % (it.pos, s, it.pos, a))
            reads.append("else if (!lua_isnoneornil(L, %d)) luaL_argerror(L, %d, \"table or nil expected\");" % (it.pos, it.pos))
            reads.append("else memset(&%s, 0, sizeof(%s));" % (a, a))
            exprs[it.index] = "&%s" % a
        elif it.shape == "ptr":
            decls.append("void* %s;" % a)
            reads.append("%s = rl_ptr(L, %d, NULL);" % (a, it.pos))
            exprs[it.index] = "(%s)%s" % (p.ctype, a)
        elif it.shape == "native_callback":
            decls.append("void* %s;" % a)
            reads.append("%s = rl_ptr(L, %d, NULL);" % (a, it.pos))
            exprs[it.index] = "(%s)%s" % (p.ctype, a)
        elif it.shape == "out":
            ctype = p.ctype[:-1]
            decls.append("%s %s;" % (ctype, a))
            reads.append("memset(&%s, 0, sizeof(%s));" % (a, a))
            exprs[it.index] = "&%s" % a
        elif it.shape == "cstr_out":
            decls.append("const char* %s = NULL;" % a)
            exprs[it.index] = "&%s" % a
        elif it.shape == "callback":
            pass
        else:
            raise ValueError("Lua: %s" % it.shape)

    binding = callback_binding(api, f, items, exprs, decls, reads, slots)
    lines = ["static int rl_w_%s(lua_State* L) {" % f.name]
    for d in decls:
        lines.append("\t" + d)
    lines.append("\tint results_ = 0;")
    lines.append("\t(void)L;")
    # (exactly the arguments of the function: what is pushed while reading them never takes the place of one left out)
    arguments = max([it.pos for it in items if it.pos] + [it.extra["opt_pos"] for it in items if "opt_pos" in it.extra] + [0])
    if arguments:
        lines.append("\tlua_settop(L, %d);" % arguments)
    for r in reads + late:
        for part in r.split("\n"):
            lines.append("\t" + part)
    for r in binding.get("reads", []):
        lines.append("\t" + r)

    args = ", ".join(exprs[i] for i in range(len(f.params)))
    call = "RL(%s)(%s)" % (f.name, args)
    after = ["\t" + line for line in binding.get("after", [])] + ["\t" + line for line in after_call(f, slots)]

    # the outs that a result consumes (the size of bytes, the pixels' width and height stay)
    consumed = set()
    if ret_kind == "void":
        lines.append("\t%s;" % call)
        lines += after
    elif ret_kind == "scalar":
        lines.append("\t{")
        lines.append("\t\t%s value_ = %s;" % (f.ret, call))
        lines += ["\t" + x for x in after]
        lines.append("\t\t%s;" % (PUSH[ret_detail] % "value_"))
        lines.append("\t\tresults_++;")
        lines.append("\t}")
    elif ret_kind == "enum":
        lines.append("\t{")
        lines.append("\t\t%s value_ = %s;" % (f.ret, call))
        lines += ["\t" + x for x in after]
        lines.append("\t\tlua_pushinteger(L, (lua_Integer)value_);")
        lines.append("\t\tresults_++;")
        lines.append("\t}")
    elif ret_kind == "cstr":
        lines.append("\t{")
        lines.append("\t\tconst char* value_ = %s;" % call)
        lines += ["\t" + x for x in after]
        lines.append("\t\trl_push_text(L, value_);")
        lines.append("\t\tresults_++;")
        lines.append("\t}")
    elif ret_kind in ("ptr", "cptr", "opaque_ptr", "cstruct_ptr", "struct_ptr"):
        lines.append("\t{")
        lines.append("\t\tconst void* value_ = %s;" % call)
        lines += ["\t" + x for x in after]
        lines.append("\t\trl_push_ptr(L, value_);")
        lines.append("\t\tresults_++;")
        lines.append("\t}")
    elif ret_kind in ("bytes", "owned_str"):
        size = next((it for it in items if it.shape == "out" and it.p.name == "size"), None)
        if size is not None:
            length = "(size_t)a%d" % size.index
            consumed.add(size.index)
        elif ret_kind == "bytes" and find_param(f, "width") is not None and find_param(f, "height") is not None:
            w, h = "a%d" % find_param(f, "width"), "a%d" % find_param(f, "height")
            length = "(%s > 0 && %s > 0 ? (size_t)%s * (size_t)%s * 4 : 0)" % (w, h, w, h)
        else:
            raise ValueError("Lua: how many bytes does %s return?" % f.name)
        free_fn = "Rpixels_free" if ret_kind == "bytes" else "Rmemory_free"
        lines.append("\t{")
        lines.append("\t\t%s value_ = %s;" % (f.ret, call))
        lines += ["\t" + x for x in after]
        lines.append("\t\tif (value_) {")
        lines.append("\t\t\tlua_pushlstring(L, (const char*)value_, %s);" % length)
        lines.append("\t\t\tRL(%s)(value_);" % free_fn)
        lines.append("\t\t} else {")
        lines.append("\t\t\tlua_pushnil(L);")
        lines.append("\t\t}")
        lines.append("\t\tresults_++;")
        lines.append("\t}")
    else:
        raise ValueError("Lua: no result of the type %s (%s)" % (f.ret, f.name))

    # what was written through pointers follows the value of the function, in the order of the parameters
    pushes = []
    for it in items:
        if it.index in consumed:
            continue
        if it.shape in ("out", "cstr_out", "pixels_out", "struct_inout"):
            pushes.append(it)
    if pushes:
        lines.append("\tluaL_checkstack(L, %d, \"raven\");" % (len(pushes) + 2))
    for it in pushes:
        a = "a%d" % it.index
        if it.shape == "out":
            if it.detail[0] == "enum":
                lines.append("\tlua_pushinteger(L, (lua_Integer)%s);" % a)
            else:
                lines.append("\t%s;" % (PUSH[it.detail[1]] % a))
        elif it.shape == "cstr_out":
            lines.append("\trl_push_text(L, %s);" % a)
        elif it.shape == "pixels_out":
            lines.append("\tif (%s) lua_pushlstring(L, (const char*)%s, %s_size); else lua_pushnil(L);" % (a, a, a))
        elif it.shape == "struct_inout":
            lines.append("\tif (lua_istable(L, %d)) {" % it.pos)
            lines.append("\t\trl_set_%s(L, %d, &%s);" % (it.detail, it.pos, a))
            lines.append("\t\tlua_pushvalue(L, %d);" % it.pos)
            lines.append("\t} else {")
            lines.append("\t\trl_push_%s(L, &%s);" % (it.detail, a))
            lines.append("\t}")
        lines.append("\tresults_++;")
    lines.append("\treturn results_;")
    lines.append("}")
    text = "\n".join(lines)
    return text


def callback_binding(api, f, items, exprs, decls, reads, slots):
    """What a function that takes a callback does around its call. Fills `exprs` for its callback parameters."""
    callbacks = [it for it in items if it.shape == "callback"]
    if not callbacks:
        return {}
    rule = callback_rule(api, f)
    slot = slots[f.name]
    user = next((it for it in items if it.shape == "ptr" and it.p.name == "user_data"), None)
    out_reads = []
    decls.append("rl_cb* cb_ = NULL;")
    for n, it in enumerate(callbacks):
        decls.append("void* n%d_;" % n)
        decls.append("int k%d_;" % n)
        out_reads.append("k%d_ = rl_callback_arg(L, %d, &n%d_);" % (n, it.pos, n))
    if user is not None:
        # a Lua function gets the Lua value back; a native function gets the address
        reads[:] = [r for r in reads if not r.startswith("a%d = rl_ptr(" % user.index)]
        out_reads.append("a%d = lua_touserdata(L, %d);" % (user.index, user.pos))
    any_function = " || ".join("k%d_ == RL_CALLBACK_FUNCTION" % n for n in range(len(callbacks)))
    if len(callbacks) == 2:
        out_reads.append("if ((k0_ == RL_CALLBACK_FUNCTION && k1_ == RL_CALLBACK_NATIVE) || (k0_ == RL_CALLBACK_NATIVE && k1_ == RL_CALLBACK_FUNCTION)) {")
        out_reads.append("\treturn luaL_error(L, \"raven: give both as Lua functions or both as native functions\");")
        out_reads.append("}")

    if rule in ("widget", "destroy"):
        guard = "RL(Rwidget_is_valid)(a0)"
    elif rule == "window":
        guard = "RL(Rwindow_is_valid)(a0)"
    elif rule == "tray":
        guard = "RL(Rtray_is_valid)(a0)"
    elif rule == "dialog":
        guard = "RL(Rwindow_is_valid)(a0) && RL(Rwidget_is_valid)(a1) && !rl_dialog_pending(a1, %d)" % slot
    elif rule == "name":
        name = next(it for it in items if it.shape == "cstr")
        guard = "a%d && a%d[0]" % (name.index, name.index)
    elif rule == "task":
        guard = "RL(Ris_initialized)()"
    else:
        guard = "1"

    fn_positions = [str(it.pos) for it in callbacks] + ["0"] * (2 - len(callbacks))
    out_reads.append("if ((%s) && %s) {" % (any_function, guard))
    out_reads.append("\tcb_ = rl_cb_new(L, %s, %s, %s);" % (fn_positions[0], fn_positions[1], user.pos if user is not None else 0))
    out_reads.append("}")
    if user is not None:
        exprs[user.index] = "cb_ ? (void*)cb_ : a%d" % user.index
    for n, it in enumerate(callbacks):
        ctype = it.p.ctype
        exprs[it.index] = "cb_ && k%d_ == RL_CALLBACK_FUNCTION ? (%s)rl_t_%s : (%s)n%d_" % (n, ctype, ctype, ctype, n)

    after = []
    if rule == "widget":
        after.append("rl_bind(a0, RL_OWNER_WIDGET, %d, NULL, cb_);" % slot)
    elif rule == "window":
        after.append("rl_bind(a0, RL_OWNER_WINDOW, %d, NULL, cb_);" % slot)
        after.append("rl_windows_sweep();")
    elif rule == "tray":
        after.append("rl_bind(a0, RL_OWNER_PLAIN, %d, NULL, cb_);" % slot)
    elif rule == "application":
        after.append("rl_bind(RL_KEY_APPLICATION, RL_OWNER_PLAIN, %d, NULL, cb_);" % slot)
    elif rule == "name":
        name = next(it for it in items if it.shape == "cstr")
        idx = name.index
        if f.ret != "void":
            # (Rxml_register_tag refuses a name that is taken by Raven)
            after.append("if (cb_ && !value_) {")
            after.append("\trl_cb_release(cb_);")
            after.append("\tcb_ = NULL;")
            after.append("}")
        after.append("if (a%d && a%d[0]) rl_bind(rl_name_key(a%d), RL_OWNER_PLAIN, %d, a%d, cb_);" % (idx, idx, idx, slot, idx))
    elif rule == "dialog":
        after.append("if (cb_) {")
        after.append("\tcb_->once = 1;")
        after.append("\trl_bind(a1, RL_OWNER_WIDGET, %d, NULL, cb_);" % slot)
        after.append("}")
    elif rule == "timer":
        after.append("if (cb_) {")
        after.append("\tif (value_ == R_INVALID_ID) {")
        after.append("\t\trl_cb_release(cb_);")
        after.append("\t} else {")
        after.append("\t\tcb_->once = !a1;")
        after.append("\t\trl_bind(value_, RL_OWNER_PLAIN, 0, NULL, cb_);")
        after.append("\t}")
        after.append("}")
    elif rule == "task":
        # one call: released after it, or by Rshutdown when it never comes
        out_reads.append("if (cb_) {")
        out_reads.append("\tcb_->once = 1;")
        out_reads.append("\trl_tasks_add(cb_);")
        out_reads.append("}")
    elif rule == "error":
        after.append("{")
        after.append("\trl_cb* old_ = rl_error_cb;")
        after.append("\trl_error_cb = cb_;")
        after.append("\trl_cb_release(old_);")
        after.append("}")
    elif rule == "destroy":
        # the slot of the C function is ours: what the program wants goes beside it
        decls.append("rl_owner* owner_ = NULL;")
        decls.append("rl_cb* previous_ = NULL;")
        out_reads.append("if (!RL(Rwidget_is_valid)(a0)) {")
        out_reads.append("\tcb_ = NULL;")
        out_reads.append("\tk0_ = RL_CALLBACK_NULL;")
        out_reads.append("\tn0_ = NULL;")
        out_reads.append("} else {")
        out_reads.append("\towner_ = rl_owner_find(a0);")
        out_reads.append("\tif (!owner_) owner_ = rl_owner_add(a0, RL_OWNER_WIDGET);")
        out_reads.append("\tif (owner_) {")
        out_reads.append("\t\tprevious_ = owner_->destroy_cb;")
        out_reads.append("\t\towner_->destroy_cb = cb_;")
        out_reads.append("\t\towner_->native_destroy = k0_ == RL_CALLBACK_NATIVE ? (RWidgetDestroyCallback)n0_ : NULL;")
        out_reads.append("\t\towner_->native_destroy_user = a2;")
        out_reads.append("\t\trl_hook_widget(owner_);")
        out_reads.append("\t\trl_cb_release(previous_);")
        out_reads.append("\t} else {")
        out_reads.append("\t\trl_cb_release(cb_);")
        out_reads.append("\t}")
        out_reads.append("\treturn 0;")
        out_reads.append("}")
    else:
        raise ValueError("Lua: the rule %s of %s" % (rule, f.name))
    return {"reads": out_reads, "after": after}


def after_call(f, slots):
    """What must be released right after a function that makes Raven forget callbacks."""
    n = f.name
    if n in ("Raction_unbind", "Rxml_unregister_tag"):
        bound = "Raction_bind" if n == "Raction_unbind" else "Rxml_register_tag"
        code = ["if (a0 && a0[0]) rl_bind(rl_name_key(a0), RL_OWNER_PLAIN, %d, a0, NULL);" % slots[bound]]
        if n == "Raction_unbind" and "Raction_bind_with_parameters" in slots:
            # (one name, bound one way or the other: both are released)
            code.append("if (a0 && a0[0]) rl_bind(rl_name_key(a0), RL_OWNER_PLAIN, %d, a0, NULL);" % slots["Raction_bind_with_parameters"])
        return code
    return {
        "Rshutdown": ["rl_release_all();"],
        "Rtimer_destroy": ["rl_bind(a0, RL_OWNER_PLAIN, 0, NULL, NULL);"],
        "Rtray_destroy": ["rl_tray_forget(a0);"],
        "Rwait_frame": ["rl_windows_sweep();"],
        "Rdraw_frame": ["rl_windows_sweep();"],
        "Rwindow_destroy": ["rl_windows_sweep();"],
    }.get(n, [])


def generate_c(api):
    table, symbols = [], []
    slots, callback_index, used = {}, {}, set()
    for f in api.functions:
        params = ", ".join(p.ctype for p in f.params) or "void"
        table.append("\t%s (*%s)(%s);" % (f.ret, f.name, params))
        symbols.append('\t{"%s", offsetof(rl_table, %s)},' % (f.name, f.name))
        callbacks = [p for p in f.params if classify(p.ctype, api)[0] == "callback"]
        if callbacks and f.name not in NATIVE_ONLY:
            slots[f.name] = len(slots) + 1
            for position, p in enumerate(callbacks):
                previous = callback_index.get(p.ctype)
                if previous is not None and previous != position:
                    raise ValueError("Lua: the callback type %s is at two positions" % p.ctype)
                callback_index[p.ctype] = position
                used.add(p.ctype)

    task = api.callback("RTaskCallback")
    if len(task.params) != 1 or task.ret != "void":
        raise ValueError("Lua: RTaskCallback has changed")
    error = api.callback("RErrorCallback")
    if [p.ctype for p in error.params] != ["RError", "RSeverity", "const char*"] or error.ret != "void":
        raise ValueError("Lua: RErrorCallback has changed")

    out = [PRELUDE.replace("%%", "\x00").replace("%(version)s", api.version).replace("%(table)s", "\n".join(table))
           .replace("%(symbols)s", "\n".join(symbols)).replace("\x00", "%")]

    wanted_in, wanted_out = struct_needs(api)
    out.append("/* ---- structures: tables with the names of their fields */\n")
    # a structure may hold a list of another: those first
    ordered = []
    for s in api.structs:
        for fld, kind, detail, count in struct_fields(api, s):
            if kind == "list_cstruct_ptr" and detail not in ordered:
                ordered.append(detail)
    for s in api.structs:
        if s.name not in ordered:
            ordered.append(s.name)
    for name in ordered:
        s = api.struct(name)
        out += struct_code(api, s, wanted_in, wanted_out)

    out.append("/* ---- the callbacks */\n")
    for c in api.callbacks:
        if c.name in used and c.name not in OFF_THREAD:
            out.append(trampoline(api, c, callback_index[c.name]))
            out.append("")

    out.append("/* ---- the functions */\n")
    for f in api.functions:
        out.append(wrapper(api, f, slots))
        out.append("")

    out.append("static const luaL_Reg rl_functions[] = {")
    for f in api.functions:
        out.append('\t{"%s", rl_w_%s},' % (f.name, f.name))
    out.append("\t{NULL, NULL}")
    out.append("};")

    constants, strings = [], []
    for name, kind, value in api.defines:
        if kind == "str":
            strings.append('\tlua_pushstring(L, "%s");' % value)
            strings.append('\tlua_setfield(L, -2, "%s");' % name)
        else:
            constants.append('\t{"%s", %d.0},' % (name, value))
    for e in api.enums:
        for item, value, doc in e.items:
            constants.append('\t{"%s", %d.0},' % (item, value))
    out.append(OPEN.replace("%%", "\x00").replace("%(constants)s", "\n".join(constants))
               .replace("%(strings)s", "\n".join(strings)).replace("\x00", "%"))
    return "\n".join(out)


def generate(api):
    return {"lua/raven_lua.c": generate_c(api)}
