"""
Node.js: a Node-API addon in C, an ES module over it and its typings.

The addon does the marshalling (scalars, texts, pointers, typed arrays, the
values returned through pointers), the module adds what is simpler in
JavaScript (structures, loading). The TypeScript surface is the one of Bun
(gen_bun.py): same names, same tuples, same classes, so that a program written
against one runs on the other.
"""

from cparse import classify, doc_lines
from gen_bun import ACCESS, DEFAULTS, NATIVE_ONLY, RUN_LOOP, SCALAR, THREADSAFE, comment, from_bigint, js_name, layout, scalar_type, sibling_count

# The TypeScript surface follows gen_bun.py: the names come from its tables, the
# rest (the signatures) is repeated below because the output differs (a .js and
# a .d.ts instead of one .ts).

# The callbacks that may arrive on another thread: see "Threads" in bindings/node/README.md.
# A worker of the job system (Rtask_run_async) and any thread that raises an error.
OFF_THREAD = ("RTaskCallback", "RErrorCallback")

# C type of a scalar in a wrapper, and how it is read from JavaScript.
C_SCALAR = {
    "int": ("int", "rn_get_int"), "float": ("float", "rn_get_float"), "double": ("double", "rn_get_double"),
    "char": ("int", "rn_get_int"), "uint8": ("uint32_t", "rn_get_uint32"), "uint32": ("uint32_t", "rn_get_uint32"),
    "id": ("uint32_t", "rn_get_uint32"), "size_t": ("uint64_t", "rn_get_uint64"), "uintptr": ("uint64_t", "rn_get_uint64"),
    "uint64": ("uint64_t", "rn_get_uint64"),
}
# How a scalar becomes a JavaScript value.
C_MAKE = {
    "int": "napi_create_int32(env, (int32_t)%s, &%s)", "char": "napi_create_int32(env, (int32_t)%s, &%s)",
    "float": "napi_create_double(env, (double)%s, &%s)", "double": "napi_create_double(env, %s, &%s)",
    "uint8": "napi_create_uint32(env, (uint32_t)%s, &%s)", "uint32": "napi_create_uint32(env, %s, &%s)",
    "id": "napi_create_uint32(env, %s, &%s)", "size_t": "napi_create_double(env, (double)%s, &%s)",
    "uintptr": "napi_create_double(env, (double)%s, &%s)", "uint64": "napi_create_bigint_uint64(env, %s, &%s)",
}
NUMBER_ARRAY = {"double": "napi_float64_array", "float": "napi_float32_array", "int": "napi_int32_array", "uint32": "napi_uint32_array"}

C_HEADER = '''/*
 * Raven for Node.js: the addon.
 *
 * Generated from the C headers by tools/bindgen (Raven %(version)s): do not edit.
 *
 * It opens the shared library of Raven at run time (see _open), so that one
 * .node per platform works with any build of Raven of this version, and
 * converts between JavaScript and C in every function. The conventions are
 * those of bindings/bun: see bindings/node/README.md.
 */

#define NAPI_VERSION 6
#include <node_api.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Raven.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <pthread.h>
#endif

/* ---- the library: every function through this table, filled by _open */

static struct {
%(table)s
} rn;

static const struct {
	const char* name;
	size_t offset;
} rn_symbols[] = {
%(symbols)s
};

/* ---- threads */

#ifdef _WIN32
typedef DWORD rn_thread_t;
static rn_thread_t rn_self(void) { return GetCurrentThreadId(); }
static int rn_thread_equal(rn_thread_t a, rn_thread_t b) { return a == b; }
#else
typedef pthread_t rn_thread_t;
static rn_thread_t rn_self(void) { return pthread_self(); }
static int rn_thread_equal(rn_thread_t a, rn_thread_t b) { return pthread_equal(a, b); }
#endif

/** The thread that loaded the library: the JavaScript thread that calls Rinit. */
static rn_thread_t rn_js_thread;
static int rn_opened;

static int rn_on_js_thread(void) { return rn_opened && rn_thread_equal(rn_self(), rn_js_thread); }

/* ---- reading arguments: 1, or 0 with a JavaScript exception thrown */

/** How many bytes of a text fit in a wrapper without an allocation. */
#define RN_TEXT_STACK 512

typedef struct rn_str {
	char stack[RN_TEXT_STACK];
	char* p;      /* NULL for null or undefined */
	size_t n;     /* bytes, without the zero that ends it */
	char* heap;   /* set when the text did not fit */
} rn_str;

static int rn_type_error(napi_env env, const char* message) {
	if (napi_throw_type_error(env, NULL, message) != napi_ok) napi_throw_error(env, NULL, message);
	return 0;
}

static int rn_int_slow(napi_env env, napi_value v, int* out) {
	bool value;
	if (napi_get_value_bool(env, v, &value) == napi_ok) {
		*out = value ? 1 : 0;
		return 1;
	}
	return rn_type_error(env, "Raven: expected a number");
}

static inline int rn_get_int(napi_env env, napi_value v, int* out) {
	if (napi_get_value_int32(env, v, out) == napi_ok) return 1;
	return rn_int_slow(env, v, out);
}

static inline int rn_get_uint32(napi_env env, napi_value v, uint32_t* out) {
	if (napi_get_value_uint32(env, v, out) == napi_ok) return 1;
	return rn_int_slow(env, v, (int*)out);
}

static inline int rn_get_double(napi_env env, napi_value v, double* out) {
	if (napi_get_value_double(env, v, out) == napi_ok) return 1;
	return rn_type_error(env, "Raven: expected a number");
}

static inline int rn_get_float(napi_env env, napi_value v, float* out) {
	double value;
	if (napi_get_value_double(env, v, &value) != napi_ok) return rn_type_error(env, "Raven: expected a number");
	*out = (float)value;
	return 1;
}

static int rn_get_uint64(napi_env env, napi_value v, uint64_t* out) {
	double value;
	bool lossless;
	if (napi_get_value_double(env, v, &value) == napi_ok) {
		*out = value < 0 ? (uint64_t)(int64_t)value : (uint64_t)value;
		return 1;
	}
	if (napi_get_value_bigint_uint64(env, v, out, &lossless) == napi_ok) return 1;
	return rn_type_error(env, "Raven: expected a number or a bigint");
}

/** A null or undefined value, or a mistake: only for arguments that are not what was first tried. */
static int rn_str_slow(napi_env env, napi_value v, rn_str* s) {
	napi_valuetype type;
	s->p = NULL;
	s->n = 0;
	if (napi_typeof(env, v, &type) == napi_ok && (type == napi_null || type == napi_undefined)) return 1;
	return rn_type_error(env, "Raven: expected a string or null");
}

static int rn_str_long(napi_env env, napi_value v, rn_str* s) {
	size_t n = 0;
	if (napi_get_value_string_utf8(env, v, NULL, 0, &n) != napi_ok) return rn_type_error(env, "Raven: expected a string or null");
	s->heap = (char*)malloc(n + 1);
	if (!s->heap) {
		napi_throw_error(env, NULL, "Raven: out of memory");
		return 0;
	}
	napi_get_value_string_utf8(env, v, s->heap, n + 1, &s->n);
	s->p = s->heap;
	return 1;
}

static inline int rn_str_get(napi_env env, napi_value v, rn_str* s) {
	size_t n;
	if (napi_get_value_string_utf8(env, v, s->stack, RN_TEXT_STACK, &n) != napi_ok) return rn_str_slow(env, v, s);
	/* (a text that did not fit stops short of the end by up to three bytes, at the edge of a character) */
	if (n + 4 >= RN_TEXT_STACK) return rn_str_long(env, v, s);
	s->p = s->stack;
	s->n = n;
	return 1;
}

/** The bytes of a typed array, a DataView or an ArrayBuffer. */
static int rn_get_view(napi_env env, napi_value v, void** data, size_t* size) {
	bool is;
	if (napi_is_typedarray(env, v, &is) == napi_ok && is) {
		static const size_t sizes[] = {1, 1, 1, 2, 2, 4, 4, 4, 8, 8, 8};
		napi_typedarray_type type;
		size_t length;
		if (napi_get_typedarray_info(env, v, &type, &length, data, NULL, NULL) != napi_ok) return 0;
		*size = length * ((size_t)type < sizeof(sizes) / sizeof(sizes[0]) ? sizes[type] : 1);
		return 1;
	}
	if (napi_is_dataview(env, v, &is) == napi_ok && is) {
		return napi_get_dataview_info(env, v, size, data, NULL, NULL) == napi_ok;
	}
	if (napi_is_arraybuffer(env, v, &is) == napi_ok && is) {
		return napi_get_arraybuffer_info(env, v, data, size) == napi_ok;
	}
	return 0;
}

static int rn_ptr_slow(napi_env env, napi_value v, void** out) {
	napi_valuetype type;
	size_t size;
	if (napi_typeof(env, v, &type) != napi_ok) return rn_type_error(env, "Raven: expected a pointer");
	if (type == napi_null || type == napi_undefined) {
		*out = NULL;
		return 1;
	}
	if (type == napi_bigint) {
		uint64_t value;
		bool lossless;
		napi_get_value_bigint_uint64(env, v, &value, &lossless);
		*out = (void*)(uintptr_t)value;
		return 1;
	}
	if (type == napi_object && rn_get_view(env, v, out, &size)) return 1;
	return rn_type_error(env, "Raven: expected an address, a typed array or null");
}

/** An address given as a number (what the library returned), memory of JavaScript for the call, or NULL. */
static inline int rn_get_ptr(napi_env env, napi_value v, void** out) {
	double value;
	if (napi_get_value_double(env, v, &value) == napi_ok) {
		*out = (void*)(uintptr_t)value;
		return 1;
	}
	return rn_ptr_slow(env, v, out);
}

/** Bytes given with their size: null and no bytes both give (NULL, 0). */
static int rn_get_bytes(napi_env env, napi_value v, void** out, size_t* size) {
	napi_valuetype type;
	*out = NULL;
	*size = 0;
	if (napi_typeof(env, v, &type) != napi_ok) return rn_type_error(env, "Raven: expected a typed array or null");
	if (type == napi_null || type == napi_undefined) return 1;
	if (type != napi_object || !rn_get_view(env, v, out, size)) return rn_type_error(env, "Raven: expected a typed array or null");
	if (*size == 0) *out = NULL;
	return 1;
}

/** Numbers: a typed array of the right kind as it is, a list copied into `*heap`, nothing for no numbers. */
static int rn_get_numbers(napi_env env, napi_value v, napi_typedarray_type wanted, void** out, void** heap) {
	napi_valuetype type;
	napi_typedarray_type kind;
	size_t length;
	uint32_t count, i;
	bool is;
	*out = NULL;
	*heap = NULL;
	if (napi_typeof(env, v, &type) != napi_ok) return rn_type_error(env, "Raven: expected numbers or null");
	if (type == napi_null || type == napi_undefined) return 1;
	if (type != napi_object) return rn_type_error(env, "Raven: expected numbers or null");
	if (napi_is_typedarray(env, v, &is) == napi_ok && is) {
		if (napi_get_typedarray_info(env, v, &kind, &length, out, NULL, NULL) != napi_ok || kind != wanted) {
			*out = NULL;
			return rn_type_error(env, "Raven: a typed array of another type of number");
		}
		if (length == 0) *out = NULL;
		return 1;
	}
	if (napi_is_array(env, v, &is) != napi_ok || !is) return rn_type_error(env, "Raven: expected numbers or null");
	napi_get_array_length(env, v, &count);
	if (count == 0) return 1;
	*heap = malloc(count * 8);
	if (!*heap) {
		napi_throw_error(env, NULL, "Raven: out of memory");
		return 0;
	}
	for (i = 0; i < count; i++) {
		napi_value element;
		double value;
		if (napi_get_element(env, v, i, &element) != napi_ok || napi_get_value_double(env, element, &value) != napi_ok) {
			return rn_type_error(env, "Raven: expected numbers or null");
		}
		switch (wanted) {
			case napi_float64_array: ((double*)*heap)[i] = value; break;
			case napi_float32_array: ((float*)*heap)[i] = (float)value; break;
			case napi_int32_array: ((int32_t*)*heap)[i] = (int32_t)value; break;
			default: ((uint32_t*)*heap)[i] = (uint32_t)value; break;
		}
	}
	*out = *heap;
	return 1;
}

/** Several texts: every pointer and its text in one allocation, freed once with free(). */
static int rn_get_strings(napi_env env, napi_value v, const char*** out, int* count_out, void** heap) {
	napi_valuetype type;
	uint32_t count, i;
	size_t total;
	bool is;
	char* text;
	*out = NULL;
	*count_out = 0;
	*heap = NULL;
	if (napi_typeof(env, v, &type) != napi_ok) return rn_type_error(env, "Raven: expected a list of strings or null");
	if (type == napi_null || type == napi_undefined) return 1;
	if (napi_is_array(env, v, &is) != napi_ok || !is) return rn_type_error(env, "Raven: expected a list of strings or null");
	napi_get_array_length(env, v, &count);
	if (count == 0) return 1;
	total = count * sizeof(char*);
	for (i = 0; i < count; i++) {
		napi_value element;
		size_t n;
		if (napi_get_element(env, v, i, &element) != napi_ok ||
			napi_get_value_string_utf8(env, element, NULL, 0, &n) != napi_ok) {
			return rn_type_error(env, "Raven: expected a list of strings or null");
		}
		total += n + 1;
	}
	*heap = malloc(total);
	if (!*heap) {
		napi_throw_error(env, NULL, "Raven: out of memory");
		return 0;
	}
	*out = (const char**)*heap;
	text = (char*)*heap + count * sizeof(char*);
	for (i = 0; i < count; i++) {
		napi_value element;
		size_t n = 0;
		napi_get_element(env, v, i, &element);
		napi_get_value_string_utf8(env, element, text, (size_t)((char*)*heap + total - text), &n);
		(*out)[i] = text;
		text += n + 1;
	}
	*count_out = (int)count;
	return 1;
}

/* ---- making results */

static inline napi_value rn_null(napi_env env) {
	napi_value v;
	napi_get_null(env, &v);
	return v;
}

static inline napi_value rn_undefined(napi_env env) {
	napi_value v;
	napi_get_undefined(env, &v);
	return v;
}

static inline napi_value rn_make_text(napi_env env, const char* text) {
	napi_value v;
	if (!text) return rn_null(env);
	napi_create_string_utf8(env, text, NAPI_AUTO_LENGTH, &v);
	return v;
}

static inline napi_value rn_make_ptr(napi_env env, const void* address) {
	napi_value v;
	if (!address) return rn_null(env);
	napi_create_double(env, (double)(uintptr_t)address, &v);
	return v;
}

static napi_value rn_tuple(napi_env env, uint32_t count, const napi_value* values) {
	napi_value array;
	uint32_t i;
	napi_create_array_with_length(env, count, &array);
	for (i = 0; i < count; i++) napi_set_element(env, array, i, values[i]);
	return array;
}

/* ---- callbacks
 *
 * rn_cb is what the library calls back: the JavaScript function, kept alive by
 * a reference, and the user_data it was given. It is reachable from the C side
 * of Raven for as long as Raven may call it, so it is released when (and only
 * when) Raven forgets it:
 *
 *   - by a slot: an owner (a widget, a window, a name, the application...) and
 *     the setter that was called. Setting that slot again, or clearing it,
 *     releases what was there; the destruction of the owner releases all of
 *     them. A widget tells when it is gone through its destroy callback, which
 *     this file owns (Rwidget_set_destroy_callback goes in a separate field);
 *   - by its one call: Rtimer_create without repeat, Rtask_post, Rdialog_open.
 *
 * rn_cb is only touched on the JavaScript thread, and a release while it is
 * running (a callback that replaces itself) waits for the end of the call.
 */

typedef struct rn_cb {
	napi_env env;
	napi_ref fn[2];
	void* user_data;
	char* text;      /* what was last returned to C by a callback that returns a text */
	size_t text_cap;
	int depth;
	int dead;
	int once;
	int asynchronous;      /* Rtask_run_async: delivered once, whatever the fate of the rest */
	int bound;
	uint64_t key;
	struct rn_cb* task_prev;
	struct rn_cb* task_next;
	int task_listed;
} rn_cb;

typedef struct rn_slot {
	struct rn_slot* next;
	uint32_t id;
	char* name;
	rn_cb* cb;
} rn_slot;

enum { RN_OWNER_PLAIN, RN_OWNER_WIDGET, RN_OWNER_WINDOW };

typedef struct rn_owner {
	struct rn_owner* next;
	struct rn_owner* window_next;
	uint64_t key;
	int kind;
	int hooked;
	rn_slot* slots;
	rn_cb* destroy_cb;
	RWidgetDestroyCallback native_destroy;
	void* native_destroy_user;
} rn_owner;

#define RN_BUCKETS 4096
static rn_owner* rn_table[RN_BUCKETS];
static rn_owner* rn_windows;
static rn_cb* rn_tasks;
static rn_cb* rn_error_cb;
/** The memory returned last by a callback now released: C may still be reading it. */
static char* rn_retired_text;

static void rn_widget_destroyed(Rid widget, void* user_data);

static size_t rn_bucket(uint64_t key) {
	return (size_t)((key ^ (key >> 20) ^ (key >> 41)) % RN_BUCKETS);
}

static rn_owner* rn_owner_find(uint64_t key) {
	rn_owner* owner;
	for (owner = rn_table[rn_bucket(key)]; owner; owner = owner->next) {
		if (owner->key == key) return owner;
	}
	return NULL;
}

static rn_owner* rn_owner_add(uint64_t key, int kind) {
	rn_owner* owner = (rn_owner*)calloc(1, sizeof(rn_owner));
	size_t bucket = rn_bucket(key);
	if (!owner) return NULL;
	owner->key = key;
	owner->kind = kind;
	owner->next = rn_table[bucket];
	rn_table[bucket] = owner;
	if (kind == RN_OWNER_WINDOW) {
		owner->window_next = rn_windows;
		rn_windows = owner;
	}
	return owner;
}

static void rn_owner_unlink(rn_owner* owner) {
	rn_owner** p;
	for (p = &rn_table[rn_bucket(owner->key)]; *p; p = &(*p)->next) {
		if (*p == owner) {
			*p = owner->next;
			break;
		}
	}
	if (owner->kind == RN_OWNER_WINDOW) {
		for (p = &rn_windows; *p; p = &(*p)->window_next) {
			if (*p == owner) {
				*p = owner->window_next;
				break;
			}
		}
	}
}

static void rn_cb_free(rn_cb* cb) {
	int i;
	for (i = 0; i < 2; i++) {
		if (cb->fn[i]) napi_delete_reference(cb->env, cb->fn[i]);
	}
	if (cb->text) {
		/* a callback released during its own call may still be returning this */
		free(rn_retired_text);
		rn_retired_text = cb->text;
	}
	free(cb);
}

/** Takes a callback out of the table or the list of tasks, without releasing it. */
static void rn_cb_detach(rn_cb* cb) {
	if (cb->bound) {
		rn_owner* owner = rn_owner_find(cb->key);
		rn_slot** p;
		cb->bound = 0;
		if (owner) {
			for (p = &owner->slots; *p; p = &(*p)->next) {
				if ((*p)->cb == cb) {
					rn_slot* slot = *p;
					*p = slot->next;
					free(slot->name);
					free(slot);
					break;
				}
			}
			if (!owner->slots && !owner->hooked && !owner->destroy_cb && !owner->native_destroy) {
				rn_owner_unlink(owner);
				free(owner);
			}
		}
	}
	if (cb->task_listed) {
		cb->task_listed = 0;
		if (cb->task_prev) cb->task_prev->task_next = cb->task_next;
		else rn_tasks = cb->task_next;
		if (cb->task_next) cb->task_next->task_prev = cb->task_prev;
	}
}

static void rn_cb_release(rn_cb* cb) {
	if (!cb) return;
	if (cb->depth > 0) cb->dead = 1;
	else rn_cb_free(cb);
}

/** Ends a call of a callback. */
static void rn_cb_leave(rn_cb* cb) {
	if (--cb->depth > 0) return;
	if (cb->once && !cb->dead) {
		rn_cb_detach(cb);
		cb->dead = 1;
	}
	if (cb->dead) rn_cb_free(cb);
}

static void rn_owner_free(rn_owner* owner) {
	rn_slot* slot = owner->slots;
	while (slot) {
		rn_slot* next = slot->next;
		if (slot->cb) {
			slot->cb->bound = 0;
			rn_cb_release(slot->cb);
		}
		free(slot->name);
		free(slot);
		slot = next;
	}
	rn_cb_release(owner->destroy_cb);
	free(owner);
}

static int rn_same_name(const char* a, const char* b) {
	return a == b || (a && b && strcmp(a, b) == 0);
}

static void rn_hook_widget(rn_owner* owner) {
	if (owner->hooked) return;
	owner->hooked = 1;
	rn.Rwidget_set_destroy_callback((Rid)owner->key, rn_widget_destroyed, owner);
}

/** Sets a slot: what was there is released. A null `cb` only clears it. */
static void rn_bind(uint64_t key, int kind, uint32_t id, const char* name, rn_cb* cb) {
	rn_owner* owner = rn_owner_find(key);
	rn_slot** p;
	if (!owner) {
		if (!cb) return;
		owner = rn_owner_add(key, kind);
		if (!owner) {
			rn_cb_release(cb);
			return;
		}
	}
	if (cb) {
		cb->bound = 1;
		cb->key = key;
		if (kind == RN_OWNER_WIDGET) rn_hook_widget(owner);
	}
	for (p = &owner->slots; *p; p = &(*p)->next) {
		if ((*p)->id == id && rn_same_name((*p)->name, name)) {
			rn_slot* slot = *p;
			rn_cb* old = slot->cb;
			if (old) old->bound = 0;
			if (cb) {
				slot->cb = cb;
			} else {
				*p = slot->next;
				free(slot->name);
				free(slot);
			}
			rn_cb_release(old);
			if (!owner->slots && !owner->hooked && !owner->destroy_cb && !owner->native_destroy) {
				rn_owner_unlink(owner);
				free(owner);
			}
			return;
		}
	}
	if (cb) {
		rn_slot* slot = (rn_slot*)malloc(sizeof(rn_slot));
		if (!slot) {
			cb->bound = 0;
			rn_cb_release(cb);
			return;
		}
		slot->id = id;
		slot->name = name ? strdup(name) : NULL;
		slot->cb = cb;
		slot->next = owner->slots;
		owner->slots = slot;
	}
}

/** The owner of a name (an action, a tag of XML): the handles of Raven are 32 bits, these are above. */
static uint64_t rn_name_key(const char* name) {
	uint32_t hash = 2166136261u;
	for (; *name; name++) hash = (hash ^ (uint8_t)*name) * 16777619u;
	return (((uint64_t)1) << 40) | hash;
}

#define RN_KEY_APPLICATION 0

/** Called by Raven when a widget is gone: its callbacks go with it. */
static void rn_t_RWidgetDestroyCallback(Rid widget, void* user_data);
static void rn_widget_destroyed(Rid widget, void* user_data) {
	rn_owner* owner = (rn_owner*)user_data;
	rn_owner_unlink(owner);
	if (owner->destroy_cb) rn_t_RWidgetDestroyCallback(widget, owner->destroy_cb);
	else if (owner->native_destroy) owner->native_destroy(widget, owner->native_destroy_user);
	rn_owner_free(owner);
}

/** Forgets the callbacks of the windows that are gone. */
static void rn_windows_sweep(void) {
	rn_owner* owner = rn_windows;
	while (owner) {
		rn_owner* next = owner->window_next;
		if (!rn.Rwindow_is_valid((Rid)owner->key)) {
			rn_owner_unlink(owner);
			rn_owner_free(owner);
		}
		owner = next;
	}
}

/** After Rshutdown: nothing is left that can call back. */
static void rn_release_all(void) {
	size_t i;
	for (i = 0; i < RN_BUCKETS; i++) {
		rn_owner* owner = rn_table[i];
		rn_table[i] = NULL;
		while (owner) {
			rn_owner* next = owner->next;
			rn_owner_free(owner);
			owner = next;
		}
	}
	rn_windows = NULL;
	while (rn_tasks) {
		rn_cb* cb = rn_tasks;
		rn_tasks = cb->task_next;
		cb->task_listed = 0;
		rn_cb_release(cb);
	}
}

static rn_cb* rn_cb_new(napi_env env, const napi_value* fns, int count, void* user_data) {
	rn_cb* cb = (rn_cb*)calloc(1, sizeof(rn_cb));
	int i;
	if (!cb) {
		napi_throw_error(env, NULL, "Raven: out of memory");
		return NULL;
	}
	cb->env = env;
	cb->user_data = user_data;
	for (i = 0; i < count; i++) {
		napi_valuetype type;
		if (napi_typeof(env, fns[i], &type) == napi_ok && type == napi_function) {
			napi_create_reference(env, fns[i], 1, &cb->fn[i]);
		}
	}
	return cb;
}

static void rn_tasks_add(rn_cb* cb) {
	cb->task_next = rn_tasks;
	cb->task_prev = NULL;
	if (rn_tasks) rn_tasks->task_prev = cb;
	rn_tasks = cb;
	cb->task_listed = 1;
}

/* What a callback argument was given as. */
enum { RN_CALLBACK_THROWN, RN_CALLBACK_NULL, RN_CALLBACK_ADDRESS, RN_CALLBACK_FUNCTION };

/** Reads a callback argument: a JavaScript function, the address of a native one, or nothing. */
static int rn_callback_arg(napi_env env, napi_value v, void** native) {
	napi_valuetype type;
	*native = NULL;
	if (napi_typeof(env, v, &type) != napi_ok) return RN_CALLBACK_THROWN;
	if (type == napi_function) return RN_CALLBACK_FUNCTION;
	if (type == napi_null || type == napi_undefined) return RN_CALLBACK_NULL;
	if (type == napi_number || type == napi_bigint) return rn_get_ptr(env, v, native) ? RN_CALLBACK_ADDRESS : RN_CALLBACK_THROWN;
	rn_type_error(env, "Raven: expected a function, the address of a native function or null");
	return RN_CALLBACK_THROWN;
}

static int rn_check_thread(napi_env env) {
	if (rn_on_js_thread()) return 1;
	napi_throw_error(env, NULL, "Raven: a callback can only be set from the thread that loaded Raven");
	return 0;
}

/** Prints what a callback threw: an exception must not cross the frames of C. */
static napi_ref rn_reporter;

static void rn_report(napi_env env) {
	napi_value error, reporter, ignored, undefined;
	if (napi_get_and_clear_last_exception(env, &error) != napi_ok) return;
	if (rn_reporter && napi_get_reference_value(env, rn_reporter, &reporter) == napi_ok && reporter) {
		undefined = rn_undefined(env);
		if (napi_call_function(env, undefined, reporter, 1, &error, &ignored) == napi_pending_exception) {
			napi_get_and_clear_last_exception(env, &ignored);
		}
	} else {
		fputs("Raven: exception in a callback\\n", stderr);
	}
}

/** Calls the JavaScript function of a callback. Returns NULL when it threw: reported here. */
static napi_value rn_call(rn_cb* cb, int index, size_t argc, const napi_value* argv) {
	napi_env env = cb->env;
	napi_value fn, undefined = rn_undefined(env), result = NULL;
	napi_status status;
	if (napi_get_reference_value(env, cb->fn[index], &fn) != napi_ok || !fn) return NULL;
	status = napi_call_function(env, undefined, fn, argc, argv, &result);
	if (status == napi_ok) return result;
	if (status == napi_pending_exception) rn_report(env);
	return NULL;
}

static int rn_result_int(napi_env env, napi_value value) {
	int result = 0;
	bool flag;
	double number;
	if (!value) return 0;
	if (napi_get_value_int32(env, value, &result) == napi_ok) return result;
	if (napi_get_value_bool(env, value, &flag) == napi_ok) return flag;
	if (napi_coerce_to_number(env, value, &value) == napi_ok && napi_get_value_double(env, value, &number) == napi_ok) return (int)number;
	return 0;
}

static Rid rn_result_id(napi_env env, napi_value value) {
	uint32_t result;
	if (!value || napi_get_value_uint32(env, value, &result) != napi_ok) return R_INVALID_ID;
	return result;
}

/** A text returned to C: it lives until the next call of this callback. */
static const char* rn_result_text(rn_cb* cb, napi_env env, napi_value value) {
	size_t n = 0;
	if (value && napi_get_value_string_utf8(env, value, NULL, 0, &n) == napi_ok) {
		if (n + 1 > cb->text_cap) {
			char* grown = (char*)realloc(cb->text, n + 1);
			if (!grown) return "";
			cb->text = grown;
			cb->text_cap = n + 1;
		}
		napi_get_value_string_utf8(env, value, cb->text, cb->text_cap, &n);
		return cb->text;
	}
	return cb->text ? cb->text : "";
}

/* ---- what arrives from other threads: queued for the JavaScript thread */

typedef struct rn_post {
	rn_cb* task;
	RError code;
	RSeverity severity;
	char* detail;
	int is_error;
} rn_post;

/** Shared by every callback that can come from another thread; created on first need. */
static napi_threadsafe_function rn_tsfn;
/** Tasks on their way from a worker: while there are some, the process stays alive. */
static int rn_pending_tasks;

static void rn_run_task(rn_cb* cb);
static void rn_run_error(RError code, RSeverity severity, const char* detail);

static void rn_tsfn_call(napi_env env, napi_value js, void* context, void* data) {
	rn_post* post = (rn_post*)data;
	(void)js;
	(void)context;
	if (env) {
		if (post->is_error) rn_run_error(post->code, post->severity, post->detail);
		else rn_run_task(post->task);
	}
	free(post->detail);
	free(post);
}

static int rn_tsfn_ensure(napi_env env) {
	napi_value name;
	if (rn_tsfn) return 1;
	napi_create_string_utf8(env, "raven", NAPI_AUTO_LENGTH, &name);
	if (napi_create_threadsafe_function(env, NULL, NULL, name, 0, 1, NULL, NULL, NULL, rn_tsfn_call, &rn_tsfn) != napi_ok) {
		napi_throw_error(env, NULL, "Raven: cannot create a thread-safe function");
		return 0;
	}
	/* it must not keep the process alive by itself */
	napi_unref_threadsafe_function(env, rn_tsfn);
	return 1;
}

static void rn_post_queue(rn_post* post) {
	if (!rn_tsfn || napi_call_threadsafe_function(rn_tsfn, post, napi_tsfn_nonblocking) != napi_ok) {
		free(post->detail);
		free(post);
	}
}

static void rn_task_started(napi_env env) {
	if (rn_pending_tasks++ == 0 && rn_tsfn) napi_ref_threadsafe_function(env, rn_tsfn);
}

static void rn_task_finished(napi_env env) {
	if (--rn_pending_tasks == 0 && rn_tsfn) napi_unref_threadsafe_function(env, rn_tsfn);
}

/* A task runs on the JavaScript thread: called directly when it is already there (Rtask_post, a zero-worker job). */
static void rn_run_task(rn_cb* cb) {
	napi_env env = cb->env;
	napi_handle_scope scope;
	napi_value argv[1];
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	argv[0] = rn_make_ptr(env, cb->user_data);
	rn_call(cb, 0, 1, argv);
	if (cb->asynchronous) rn_task_finished(env);
	rn_cb_leave(cb);
	napi_close_handle_scope(env, scope);
}

static void rn_t_RTaskCallback(void* user_data) {
	rn_post* post;
	if (rn_on_js_thread()) {
		rn_run_task((rn_cb*)user_data);
		return;
	}
	post = (rn_post*)calloc(1, sizeof(rn_post));
	if (!post) return;
	post->task = (rn_cb*)user_data;
	rn_post_queue(post);
}

static void rn_run_error(RError code, RSeverity severity, const char* detail) {
	rn_cb* cb = rn_error_cb;
	napi_env env;
	napi_handle_scope scope;
	napi_value argv[3];
	if (!cb) return;
	env = cb->env;
	napi_open_handle_scope(env, &scope);
	cb->depth++;
	napi_create_int32(env, (int32_t)code, &argv[0]);
	napi_create_int32(env, (int32_t)severity, &argv[1]);
	argv[2] = rn_make_text(env, detail);
	rn_call(cb, 0, 3, argv);
	rn_cb_leave(cb);
	napi_close_handle_scope(env, scope);
}

static void rn_t_RErrorCallback(RError code, RSeverity severity, const char* detail) {
	rn_post* post;
	if (rn_on_js_thread()) {
		rn_run_error(code, severity, detail);
		return;
	}
	post = (rn_post*)calloc(1, sizeof(rn_post));
	if (!post) return;
	post->is_error = 1;
	post->code = code;
	post->severity = severity;
	post->detail = detail ? strdup(detail) : NULL;
	rn_post_queue(post);
}
'''

C_NATIVE_HELPERS = '''
/* ---- what raven.js needs from the addon */

static napi_value rn_w__open(napi_env env, napi_callback_info info) {
	size_t argc = 1, i;
	napi_value argv[1], result;
	rn_str path;
	void* handle;
	char missing[256];
	path.heap = NULL;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_str_get(env, argv[0], &path) || !path.p) goto fail;
#ifdef _WIN32
	{
		/* the path is UTF-8: LoadLibraryA would read it in the ANSI code page */
		int count = MultiByteToWideChar(CP_UTF8, 0, path.p, -1, NULL, 0);
		wchar_t* wide = count > 0 ? (wchar_t*)malloc(sizeof(wchar_t) * (size_t)count) : NULL;
		handle = NULL;
		if (wide && MultiByteToWideChar(CP_UTF8, 0, path.p, -1, wide, count) > 0) handle = (void*)LoadLibraryW(wide);
		free(wide);
	}
#else
	handle = dlopen(path.p, RTLD_NOW | RTLD_LOCAL);
#endif
	if (!handle) {
#ifdef _WIN32
		snprintf(missing, sizeof(missing), "cannot open the library (error %lu)", GetLastError());
#else
		snprintf(missing, sizeof(missing), "%s", dlerror());
#endif
		goto report;
	}
	for (i = 0; i < sizeof(rn_symbols) / sizeof(rn_symbols[0]); i++) {
#ifdef _WIN32
		void* symbol = (void*)GetProcAddress((HMODULE)handle, rn_symbols[i].name);
#else
		void* symbol = dlsym(handle, rn_symbols[i].name);
#endif
		if (!symbol) {
			snprintf(missing, sizeof(missing), "the library has no function %s", rn_symbols[i].name);
			goto report;
		}
		*(void**)((char*)&rn + rn_symbols[i].offset) = symbol;
	}
	rn_js_thread = rn_self();
	rn_opened = 1;
	napi_get_boolean(env, true, &result);
	free(path.heap);
	return result;

report:
	napi_create_string_utf8(env, missing, NAPI_AUTO_LENGTH, &result);
	free(path.heap);
	return result;
fail:
	free(path.heap);
	return NULL;
}

static napi_value rn_w__ptr(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	void* address;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &address)) return NULL;
	return rn_make_ptr(env, address);
}

static napi_value rn_w__text(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	void* address;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &address)) return NULL;
	return rn_make_text(env, (const char*)address);
}

static int rn_copied_views;

/** An ArrayBuffer over the memory of the library; a copy of it where the runtime forbids that (Electron). */
static napi_value rn_w__view(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	napi_value argv[2], result;
	void* address;
	double size;
	void* copy;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &address) || !rn_get_double(env, argv[1], &size)) return NULL;
	if (!rn_copied_views && napi_create_external_arraybuffer(env, address, (size_t)size, NULL, NULL, &result) == napi_ok) return result;
	rn_copied_views = 1;
	if (napi_create_arraybuffer(env, (size_t)size, &copy, &result) != napi_ok) return NULL;
	if (size > 0) memcpy(copy, address, (size_t)size);
	return result;
}

/** Whether views of the memory of the library are copies: tried here, on memory of this file. */
static napi_value rn_w__copied(napi_env env, napi_callback_info info) {
	static char probe[8];
	napi_value result;
	(void)info;
	if (napi_create_external_arraybuffer(env, probe, sizeof(probe), NULL, NULL, &result) != napi_ok) rn_copied_views = 1;
	napi_get_boolean(env, rn_copied_views != 0, &result);
	return result;
}

static napi_value rn_w__poke(napi_env env, napi_callback_info info) {
	size_t argc = 2, size = 0;
	napi_value argv[2];
	void* address;
	void* bytes;
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (!rn_get_ptr(env, argv[0], &address) || !rn_get_bytes(env, argv[1], &bytes, &size)) return NULL;
	if (address && bytes && size) memcpy(address, bytes, size);
	return NULL;
}

static napi_value rn_w__reporter(napi_env env, napi_callback_info info) {
	size_t argc = 1;
	napi_value argv[1];
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	if (rn_reporter) napi_delete_reference(env, rn_reporter);
	napi_create_reference(env, argv[0], 1, &rn_reporter);
	return NULL;
}

/** At the end of the program: nothing arrives any more from the threads of Raven. */
static void rn_cleanup(void* arg) {
	(void)arg;
	if (rn_opened) rn.Rerror_set_callback(NULL);
	rn_tsfn = NULL;
}
'''


def plan_function(api, f):
    """How each parameter travels: mirrors generate_function in gen_bun.py, which is what has to match."""
    items = []
    signature = []
    skip = False
    outs = []
    for index, p in enumerate(f.params):
        kind, detail = classify(p.ctype, api)
        n = js_name(p.name)
        last = index == len(f.params) - 1
        following = f.params[index + 1] if not last else None
        if skip:
            skip = False
            continue
        item = {"index": index, "p": p, "kind": kind, "detail": detail, "js": n}
        if kind == "out":
            item["shape"] = "out"
            outs.append(item)
        elif kind == "cstr_out":
            item["shape"] = "cstr_out"
            outs.append(item)
        elif kind == "cstr_array":
            item["shape"] = "cstr_array"
            signature.append("%s: readonly string[] | null" % n)
            if following is not None and classify(following.ctype, api) == ("scalar", "int") and "count" in following.name:
                item["count"] = index + 1
                skip = True
        elif kind == "cstruct_ptr" and following is not None and following.name == "count" and classify(following.ctype, api) == ("scalar", "int"):
            item["shape"] = "struct_array"
            item["struct"] = detail
            item["count"] = index + 1
            signature.append("%s: readonly %s[] | null" % (n, detail))
            skip = True
        elif kind == "cstr" and following is not None and following.ctype == "size_t" and following.name in ("size", "data_size"):
            item["shape"] = "cstr_sized"
            item["size"] = index + 1
            signature.append("%s: string | null" % n)
            skip = True
        elif kind in ("cptr", "cbytes") and following is not None and following.ctype == "size_t" and following.name in ("size", "data_size"):
            item["shape"] = "bytes_sized"
            item["size"] = index + 1
            signature.append("%s: NodeJS.TypedArray | null" % n)
            skip = True
        elif kind == "carray":
            item["shape"] = "carray"
            signature.append("%s: %s | readonly number[] | null" % (n, ACCESS[detail][1]))
        elif kind == "callback" and f.name in NATIVE_ONLY:
            item["shape"] = "native_callback"
            signature.append("%s: Pointer | null" % n)
        elif kind == "callback":
            item["shape"] = "callback"
            signature.append("%s: %s | Pointer | null" % (n, detail))
        elif kind in ("struct_ptr", "cstruct_ptr"):
            item["shape"] = "struct"
            item["struct"] = detail
            signature.append("%s: %s | null" % (n, detail))
        elif kind == "scalar":
            item["shape"] = "scalar"
            signature.append("%s: %s" % (n, scalar_type(detail)))
        elif kind == "enum":
            item["shape"] = "enum"
            signature.append("%s: %s" % (n, detail))
        elif kind == "cstr":
            item["shape"] = "cstr"
            signature.append("%s: string | null" % n)
        elif kind in ("ptr", "cptr", "bytes", "cbytes", "opaque_ptr"):
            item["shape"] = "ptr"
            optional = kind == "ptr" and p.name == "user_data" and last
            signature.append("%s: RPointer%s" % (n, " = null" if optional else ""))
        else:
            raise ValueError("no parameter of the type %s for Node (%s)" % (p.ctype, f.name))
        items.append(item)
    return items, signature, outs


def callback_rule(api, f):
    """Who keeps the callback of a function, and until when. A new shape of setter must be added here."""
    n = f.name
    if n in ("Rtask_post", "Rtask_run_async"):
        return "task"
    if n == "Rerror_set_callback":
        return "error"
    if n == "Rwidget_set_destroy_callback":
        return "destroy"
    if n == "Rtimer_create":
        return "timer"
    if n == "Rdialog_open":
        return "dialog"
    if n in ("Raction_bind", "Rxml_register_tag"):
        return "name"
    first = f.params[0] if f.params else None
    if first is not None and classify(first.ctype, api) == ("scalar", "id"):
        if n.startswith("Rwindow_"):
            return "window"
        if n.startswith("Rtray_"):
            return "tray"
        return "widget"
    if all(classify(p.ctype, api)[0] in ("callback", "ptr") for p in f.params):
        return "application"
    raise ValueError("Node: who keeps the callback of %s? Add it to callback_rule." % n)


def c_getter(api, item, native_index, goto):
    """(declarations, statements) that read one argument. Returns the C expression and cleanup lines as well."""
    kind = item["kind"]
    detail = item["detail"]
    i = item["index"]
    a = "a%d" % i
    v = "argv[%d]" % native_index if native_index is not None else None
    if item["shape"] == "scalar":
        ctype, getter = C_SCALAR[detail]
        decl = ["%s %s;" % (ctype, a)]
        return decl, ["if (!%s(env, %s, &%s)) %s;" % (getter, v, a, goto)], "(%s)%s" % (item["p"].ctype, a), []
    if item["shape"] == "enum":
        return ["int %s;" % a], ["if (!rn_get_int(env, %s, &%s)) %s;" % (v, a, goto)], "(%s)%s" % (detail, a), []
    if item["shape"] == "cstr":
        return ["rn_str %s;" % a], ["if (!rn_str_get(env, %s, &%s)) %s;" % (v, a, goto)], "%s.p" % a, ["free(%s.heap);" % a]
    if item["shape"] == "cstr_sized":
        return ["rn_str %s;" % a], ["if (!rn_str_get(env, %s, &%s)) %s;" % (v, a, goto)], "%s.p, %s.n" % (a, a), ["free(%s.heap);" % a]
    if item["shape"] in ("ptr", "struct_array", "struct"):
        cast = "(%s)%s" % (item["p"].ctype, a)
        return ["void* %s;" % a], ["if (!rn_get_ptr(env, %s, &%s)) %s;" % (v, a, goto)], cast, []
    if item["shape"] == "bytes_sized":
        return ["void* %s;" % a, "size_t %s_size;" % a], ["if (!rn_get_bytes(env, %s, &%s, &%s_size)) %s;" % (v, a, a, goto)], "(%s)%s, %s_size" % (item["p"].ctype, a, a), []
    if item["shape"] == "cstr_array":
        decl = ["const char** %s;" % a, "int %s_count;" % a, "void* %s_heap;" % a]
        read = ["if (!rn_get_strings(env, %s, &%s, &%s_count, &%s_heap)) %s;" % (v, a, a, a, goto)]
        expr = "(const char* const*)%s" % a
        if "count" in item:
            expr += ", %s_count" % a
        return decl, read, expr, ["free(%s_heap);" % a]
    if item["shape"] == "carray":
        if detail not in NUMBER_ARRAY:
            raise ValueError("no array of %s for Node" % detail)
        decl = ["void* %s;" % a, "void* %s_heap;" % a]
        read = ["if (!rn_get_numbers(env, %s, %s, &%s, &%s_heap)) %s;" % (v, NUMBER_ARRAY[detail], a, a, goto)]
        return decl, read, "(%s)%s" % (item["p"].ctype, a), ["free(%s_heap);" % a]
    if item["shape"] == "out":
        ctype = item["p"].ctype.rstrip("*")
        if detail[0] == "enum":
            return ["%s %s = (%s)0;" % (ctype, a, ctype)], [], "&%s" % a, []
        return ["%s %s = 0;" % (ctype, a)], [], "&%s" % a, []
    if item["shape"] == "cstr_out":
        return ["const char* %s = NULL;" % a], [], "&%s" % a, []
    if item["shape"] == "native_callback":
        return ["void* %s;" % a], ["if (!rn_get_ptr(env, %s, &%s)) %s;" % (v, a, goto)], "(%s)%s" % (item["p"].ctype, a), []
    raise ValueError("no way to read %s for Node" % item["shape"])


def c_out_value(api, item, target):
    """The statement that turns the value written by C into a JavaScript value."""
    a = "a%d" % item["index"]
    if item["shape"] == "cstr_out":
        return "%s = rn_make_text(env, %s);" % (target, a)
    detail = item["detail"]
    if detail[0] == "enum":
        return "napi_create_int32(env, (int32_t)%s, &%s);" % (a, target)
    return C_MAKE[detail[1]] % (a, target) + ";"


def c_wrapper(api, f, slots, callback_indices):
    items, _signature, outs = plan_function(api, f)
    ret_kind, ret_detail = classify(f.ret, api)
    if ret_kind == "owned_str" and not any(o["p"].name == "size" for o in outs):
        raise ValueError("no size for the bytes of %s for Node" % f.name)
    natives = [it for it in items if it["shape"] not in ("out", "cstr_out")]
    # every parameter is one argument of the addon, and an array of structures two: its bytes and its count
    native_count = 0
    native_index = {}
    for it in natives:
        native_index[it["index"]] = native_count
        native_count += 1
        if it["shape"] == "struct_array":
            native_count += 1

    needs_cleanup = any(it["shape"] in ("cstr", "cstr_sized", "cstr_array", "carray") for it in items)
    goto = "goto done" if needs_cleanup else "return NULL"
    decls = []
    reads = []
    cleanups = []
    exprs = {}
    for it in items:
        if it["shape"] == "callback":
            continue
        d, r, e, c = c_getter(api, it, native_index.get(it["index"]), goto)
        decls += d
        reads += r
        cleanups += c
        exprs[it["index"]] = e
        if it["shape"] == "struct_array":
            k = it["count"]
            count_index = native_index[it["index"]] + 1
            decls.append("int a%d;" % k)
            reads.append("if (!rn_get_int(env, argv[%d], &a%d)) %s;" % (count_index, k, goto))
            exprs[it["index"]] = "(%s)a%d, a%d" % (it["p"].ctype, it["index"], k)

    lines = []
    lines.append("static napi_value rn_w_%s(napi_env env, napi_callback_info info) {" % f.name)
    if native_count:
        lines.append("\tsize_t argc = %d;" % native_count)
        lines.append("\tnapi_value argv[%d];" % native_count)
    lines.append("\tnapi_value result_ = NULL;")
    lines.append("\t(void)env;")
    callback_items = [it for it in items if it["shape"] == "callback"]
    binding = emit_callback_binding(api, f, callback_items, native_index, exprs, decls, reads, slots, goto, items)
    for d in decls:
        lines.append("\t" + d)
    if needs_cleanup:
        for it in items:
            if it["shape"] in ("cstr", "cstr_sized"):
                lines.append("\ta%d.heap = NULL;" % it["index"])
            elif it["shape"] == "cstr_array":
                lines.append("\ta%d_heap = NULL;" % it["index"])
            elif it["shape"] == "carray":
                lines.append("\ta%d_heap = NULL;" % it["index"])
    if native_count:
        lines.append("\tnapi_get_cb_info(env, info, &argc, argv, NULL, NULL);")
    else:
        lines.append("\t(void)info;")
    for r in reads:
        lines.append("\t" + r)

    args = ", ".join(exprs[i] for i in sorted(exprs))
    call = "rn.%s(%s)" % (f.name, args)

    def after_call():
        result = []
        for line in binding.get("after", []):
            result.append("\t" + line)
        return result

    if ret_kind == "void":
        lines.append("\t%s;" % call)
        lines += after_call()
    elif ret_kind == "scalar":
        ctype = f.ret
        lines.append("\t%s value_ = %s;" % (ctype, call))
        lines += after_call()
        lines.append("\t" + C_MAKE[ret_detail] % ("value_", "result_") + ";")
    elif ret_kind == "enum":
        lines.append("\t{")
        lines.append("\t\tint value_ = (int)%s;" % call)
        lines.append("\t\tnapi_create_int32(env, value_, &result_);")
        lines.append("\t}")
        lines += after_call()
    elif ret_kind == "cstr":
        lines.append("\tresult_ = rn_make_text(env, %s);" % call)
        lines += after_call()
    elif ret_kind in ("ptr", "cptr", "bytes", "cbytes", "opaque_ptr", "struct_ptr", "cstruct_ptr"):
        lines.append("\tresult_ = rn_make_ptr(env, %s);" % call)
        lines += after_call()
    elif ret_kind == "owned_str":
        size_out = next(o for o in outs if o["p"].name == "size")
        lines.append("\t{")
        lines.append("\t\tchar* owned_ = %s;" % call)
        lines.append("\t\tvoid* copy_;")
        lines.append("\t\tnapi_value buffer_;")
        lines.append("\t\tif (!owned_) {")
        lines.append("\t\t\tresult_ = rn_null(env);")
        lines.append("\t\t} else {")
        lines.append("\t\t\tsize_t size_ = (size_t)a%d;" % size_out["index"])
        lines.append("\t\t\tnapi_create_arraybuffer(env, size_, &copy_, &buffer_);")
        lines.append("\t\t\tif (size_ > 0) memcpy(copy_, owned_, size_);")
        lines.append("\t\t\trn.Rmemory_free(owned_);")
        lines.append("\t\t\tnapi_create_typedarray(env, napi_uint8_array, size_, buffer_, 0, &result_);")
        lines.append("\t\t}")
        lines.append("\t}")
    else:
        raise ValueError("no result of the type %s for Node (%s)" % (f.ret, f.name))

    # the results written through pointers follow the value of the function
    if ret_kind != "owned_str" and outs:
        lines.append("\t{")
        names = []
        if ret_kind != "void":
            names.append("result_")
        for pos, o in enumerate(outs):
            name = "o%d_" % pos
            lines.append("\t\tnapi_value %s;" % name)
            lines.append("\t\t" + c_out_value(api, o, name))
            names.append(name)
        if len(names) == 1:
            lines.append("\t\tresult_ = %s;" % names[0])
        else:
            lines.append("\t\t{")
            lines.append("\t\t\tconst napi_value values_[%d] = {%s};" % (len(names), ", ".join(names)))
            lines.append("\t\t\tresult_ = rn_tuple(env, %d, values_);" % len(names))
            lines.append("\t\t}")
        lines.append("\t}")

    if needs_cleanup:
        lines.append("done:")
    for c in cleanups:
        lines.append("\t" + c)
    lines.append("\treturn result_;")
    lines.append("}")
    return "\n".join(lines)


def emit_callback_binding(api, f, callback_items, native_index, exprs, decls, reads, slots, goto, items):
    """What a function that takes a callback does beyond the call. Fills `exprs` for its callback parameters."""
    if not callback_items:
        return {}
    rule = callback_rule(api, f)
    slot = slots[f.name]

    # the user_data: a JavaScript function gets back what it was given, C gets the rn_cb
    user_item = next((it for it in items if it["kind"] == "ptr" and it["p"].name == "user_data"), None)
    decls.append("rn_cb* cb_ = NULL;")
    for pos, it in enumerate(callback_items):
        decls.append("void* n%d_;" % pos)
        decls.append("int k%d_;" % pos)
        reads.append("k%d_ = rn_callback_arg(env, argv[%d], &n%d_);" % (pos, native_index[it["index"]], pos))
        reads.append("if (k%d_ == RN_CALLBACK_THROWN) %s;" % (pos, goto))
        reads.append("if (k%d_ == RN_CALLBACK_FUNCTION && !rn_check_thread(env)) %s;" % (pos, goto))

    any_function = "k0_ == RN_CALLBACK_FUNCTION"
    if len(callback_items) == 2:
        reads.append("if ((k0_ == RN_CALLBACK_FUNCTION && k1_ == RN_CALLBACK_ADDRESS) || (k0_ == RN_CALLBACK_ADDRESS && k1_ == RN_CALLBACK_FUNCTION)) {")
        reads.append("\trn_type_error(env, \"Raven: give both as JavaScript functions or both as addresses\");")
        reads.append("\t%s;" % goto)
        reads.append("}")

    if rule == "widget" or rule == "destroy":
        guard = "rn.Rwidget_is_valid(a0)"
    elif rule == "window":
        guard = "rn.Rwindow_is_valid(a0)"
    elif rule == "tray":
        guard = "rn.Rtray_is_valid(a0)"
    elif rule == "dialog":
        guard = "rn.Rwindow_is_valid(a0) && rn.Rwidget_is_valid(a1) && !rn_dialog_pending(a1, %d)" % slot
    elif rule == "name":
        name = next(it for it in items if it["shape"] == "cstr")
        guard = "a%d.p && a%d.p[0]" % (name["index"], name["index"])
    elif rule == "task":
        guard = "rn.Ris_initialized()"
    else:
        guard = "1"

    # the callback itself
    if user_item is not None:
        user_expr = "a%d" % user_item["index"]
        exprs_user = "cb_ ? (void*)cb_ : %s" % user_expr
        exprs[user_item["index"]] = exprs_user
    else:
        user_expr = "NULL"

    if len(callback_items) == 1:
        fns = "&argv[%d], 1" % native_index[callback_items[0]["index"]]
        reads_code = ["if (%s && %s) {" % (any_function, guard)]
        reads_code.append("\tcb_ = rn_cb_new(env, %s, %s);" % (fns, user_expr))
        reads_code.append("\tif (!cb_) %s;" % goto)
        reads_code.append("}")
    else:
        # the two functions of Rxml_register_tag share one user_data
        reads_code = ["if (%s && %s) {" % (any_function, guard)]
        reads_code.append("\tnapi_value fns_[2];")
        reads_code.append("\tfns_[0] = argv[%d];" % native_index[callback_items[0]["index"]])
        reads_code.append("\tfns_[1] = argv[%d];" % native_index[callback_items[1]["index"]])
        reads_code.append("\tcb_ = rn_cb_new(env, fns_, 2, %s);" % user_expr)
        reads_code.append("\tif (!cb_) %s;" % goto)
        reads_code.append("}")
    reads.extend(reads_code)

    for pos, it in enumerate(callback_items):
        ctype = it["p"].ctype
        tramp = "rn_t_%s" % ctype
        if rule == "task":
            exprs[it["index"]] = "k%d_ == RN_CALLBACK_FUNCTION ? (%s)%s : (%s)n%d_" % (pos, ctype, tramp, ctype, pos)
        else:
            exprs[it["index"]] = "cb_ ? (%s)%s : (%s)n%d_" % (ctype, tramp, ctype, pos)

    after = []
    if rule == "widget":
        after.append("rn_bind(a0, RN_OWNER_WIDGET, %d, NULL, cb_);" % slot)
    elif rule == "window":
        after.append("rn_bind(a0, RN_OWNER_WINDOW, %d, NULL, cb_);" % slot)
        after.append("rn_windows_sweep();")
    elif rule == "tray":
        after.append("rn_bind(a0, RN_OWNER_PLAIN, %d, NULL, cb_);" % slot)
    elif rule == "application":
        after.append("rn_bind(RN_KEY_APPLICATION, RN_OWNER_PLAIN, %d, NULL, cb_);" % slot)
    elif rule == "name":
        name = next(it for it in items if it["shape"] == "cstr")
        idx = name["index"]
        if f.ret != "void":
            # (Rxml_register_tag refuses a name that is taken by Raven)
            after.append("if (cb_ && !value_) {")
            after.append("\trn_cb_release(cb_);")
            after.append("\tcb_ = NULL;")
            after.append("}")
        after.append("if (a%d.p && a%d.p[0]) rn_bind(rn_name_key(a%d.p), RN_OWNER_PLAIN, %d, a%d.p, cb_);" % (idx, idx, idx, slot, idx))
    elif rule == "dialog":
        after.append("if (cb_) {")
        after.append("\tcb_->once = 1;")
        after.append("\trn_bind(a1, RN_OWNER_WIDGET, %d, NULL, cb_);" % slot)
        after.append("}")
    elif rule == "timer":
        after.append("if (cb_) {")
        after.append("\tif (value_ == R_INVALID_ID) {")
        after.append("\t\trn_cb_release(cb_);")
        after.append("\t} else {")
        after.append("\t\tcb_->once = !a1;")
        after.append("\t\trn_bind(value_, RN_OWNER_PLAIN, 0, NULL, cb_);")
        after.append("\t}")
        after.append("}")
    elif rule == "task":
        if f.name == "Rtask_post":
            after.append("if (cb_) {")
            after.append("\tcb_->once = 1;")
            after.append("\trn_tasks_add(cb_);")
            after.append("}")
        else:
            # one call, whatever happens to the application: delivered and released on the JavaScript thread
            reads.append("if (cb_) {")
            reads.append("\tcb_->once = 1;")
            reads.append("\tcb_->asynchronous = 1;")
            reads.append("\tif (!rn_tsfn_ensure(env)) {")
            reads.append("\t\trn_cb_release(cb_);")
            reads.append("\t\tcb_ = NULL;")
            reads.append("\t\t%s;" % goto)
            reads.append("\t}")
            reads.append("\trn_task_started(env);")
            reads.append("}")
    elif rule == "error":
        reads.append("if (k0_ == RN_CALLBACK_FUNCTION && !rn_tsfn_ensure(env)) {")
        reads.append("\trn_cb_release(cb_);")
        reads.append("\tcb_ = NULL;")
        reads.append("\t%s;" % goto)
        reads.append("}")
        after.append("{")
        after.append("\trn_cb* old_ = rn_error_cb;")
        after.append("\trn_error_cb = cb_;")
        after.append("\trn_cb_release(old_);")
        after.append("}")
    elif rule == "destroy":
        # the slot of the C function is ours: what the program wants goes beside it
        decls.append("rn_owner* owner_ = NULL;")
        decls.append("rn_cb* previous_ = NULL;")
        reads.append("if (!rn.Rwidget_is_valid(a0)) {")
        reads.append("\tcb_ = NULL;")
        reads.append("\tk0_ = RN_CALLBACK_NULL;")
        reads.append("\tn0_ = NULL;")
        reads.append("} else {")
        reads.append("\towner_ = rn_owner_find(a0);")
        reads.append("\tif (!owner_) owner_ = rn_owner_add(a0, RN_OWNER_WIDGET);")
        reads.append("\tif (owner_) {")
        reads.append("\t\tprevious_ = owner_->destroy_cb;")
        reads.append("\t\towner_->destroy_cb = cb_;")
        reads.append("\t\towner_->native_destroy = k0_ == RN_CALLBACK_ADDRESS ? (RWidgetDestroyCallback)n0_ : NULL;")
        reads.append("\t\towner_->native_destroy_user = a2;")
        reads.append("\t\trn_hook_widget(owner_);")
        reads.append("\t\trn_cb_release(previous_);")
        reads.append("\t}")
        reads.append("\tresult_ = NULL;")
        reads.append("\t%s;" % ("goto done" if goto.startswith("goto") else "return NULL"))
        reads.append("}")
    return {"after": after}


# The functions after which something has been forgotten by Raven.
AFTER_CALL = {
    "Rshutdown": "rn_release_all();",
    "Rtimer_destroy": "rn_bind(a0, RN_OWNER_PLAIN, 0, NULL, NULL);",
    "Rtray_destroy": "rn_tray_forget(a0);",
    "Rwait_frame": "rn_windows_sweep();",
    "Rdraw_frame": "rn_windows_sweep();",
    "Rwindow_destroy": "rn_windows_sweep();",
}


def c_add_after(lines_text, f, slots):
    """Adds what a function that forgets callbacks must release, right after its call to C."""
    n = f.name
    if n in ("Raction_unbind", "Rxml_unregister_tag"):
        bound = "Raction_bind" if n == "Raction_unbind" else "Rxml_register_tag"
        code = "if (a0.p && a0.p[0]) rn_bind(rn_name_key(a0.p), RN_OWNER_PLAIN, %d, a0.p, NULL);" % slots[bound]
    elif n in AFTER_CALL:
        code = AFTER_CALL[n]
    else:
        return lines_text
    call = "\trn.%s(" % n
    if call in lines_text:
        start = lines_text.index(call)
        end = lines_text.index(";\n", start) + 2
        return lines_text[:end] + "\t" + code + "\n" + lines_text[end:]
    assign = "value_ = rn.%s(" % n
    if assign not in lines_text:
        raise ValueError("Node: cannot place the release for %s" % n)
    start = lines_text.index(assign)
    end = lines_text.index(";\n", start) + 2
    return lines_text[:end] + "\t" + code + "\n" + lines_text[end:]


def c_trampoline(api, c, callback_index):
    """The C function Raven calls: it builds the arguments and calls the JavaScript function."""
    ret_kind, ret_detail = classify(c.ret, api)
    params = ", ".join("%s a%d" % (p.ctype, i) for i, p in enumerate(c.params))
    if not params:
        params = "void"
    lines = ["static %s rn_t_%s(%s) {" % (c.ret, c.name, params)]
    user_index = None
    for i, p in enumerate(c.params):
        if p.name == "user_data" and classify(p.ctype, api)[0] == "ptr":
            user_index = i
    if user_index is None:
        raise ValueError("Node: the callback %s has no user_data: it needs a slot of its own" % c.name)
    count = len(c.params)
    lines.append("\trn_cb* cb = (rn_cb*)a%d;" % user_index)
    lines.append("\tnapi_env env = cb->env;")
    lines.append("\tnapi_handle_scope scope;")
    lines.append("\tnapi_value argv[%d];" % count)
    if ret_kind != "void":
        lines.append("\tnapi_value result;")
        lines.append("\t%s value;" % c.ret)
    lines.append("\tnapi_open_handle_scope(env, &scope);")
    lines.append("\tcb->depth++;")
    for i, p in enumerate(c.params):
        kind, detail = classify(p.ctype, api)
        target = "argv[%d]" % i
        name = "a%d" % i
        if i == user_index:
            lines.append("\t%s = rn_make_ptr(env, cb->user_data);" % target)
        elif kind == "scalar":
            lines.append("\t" + C_MAKE[detail] % (name, target) + ";")
        elif kind == "enum":
            lines.append("\tnapi_create_int32(env, (int32_t)%s, &%s);" % (name, target))
        elif kind == "cstr":
            lines.append("\t%s = rn_make_text(env, %s);" % (target, name))
        elif kind in ("struct_ptr", "cstruct_ptr", "out", "ptr", "opaque_ptr", "cptr", "bytes", "cbytes"):
            lines.append("\t%s = rn_make_ptr(env, %s);" % (target, name))
        else:
            raise ValueError("no parameter of the type %s in a callback for Node (%s)" % (p.ctype, c.name))
    call = "rn_call(cb, %d, %d, argv)" % (callback_index, count)
    if ret_kind == "void":
        lines.append("\t%s;" % call)
    else:
        lines.append("\tresult = %s;" % call)
        if ret_kind == "cstr":
            lines.append("\tvalue = rn_result_text(cb, env, result);")
        elif ret_detail == "id":
            lines.append("\tvalue = rn_result_id(env, result);")
        elif ret_detail == "int":
            lines.append("\tvalue = rn_result_int(env, result);")
        else:
            raise ValueError("no result of the type %s for a callback for Node (%s)" % (c.ret, c.name))
    lines.append("\tnapi_close_handle_scope(env, scope);")
    lines.append("\trn_cb_leave(cb);")
    if ret_kind != "void":
        lines.append("\treturn value;")
    lines.append("}")
    return "\n".join(lines)


def generate_c(api):
    # the table, and the order of the slots of callbacks
    table = []
    symbols = []
    slots = {}
    callback_index = {}
    used_callbacks = set()
    for f in api.functions:
        params = ", ".join(p.ctype for p in f.params) or "void"
        table.append("\t%s (*%s)(%s);" % (f.ret, f.name, params))
        symbols.append('\t{"%s", offsetof(__typeof__(rn), %s)},' % (f.name, f.name))
        callbacks = [p for p in f.params if classify(p.ctype, api)[0] == "callback"]
        if callbacks and f.name not in NATIVE_ONLY:
            slots[f.name] = len(slots) + 1
            for position, p in enumerate(callbacks):
                previous = callback_index.get(p.ctype)
                if previous is not None and previous != position:
                    raise ValueError("Node: the callback type %s is at two positions" % p.ctype)
                callback_index[p.ctype] = position
                used_callbacks.add(p.ctype)
    out = [C_HEADER.replace("%(version)s", api.version).replace("%(table)s", "\n".join(table)).replace("%(symbols)s", "\n".join(symbols))]

    out.append('''
/** Is a dialog for this widget still waiting for its callback? */
static int rn_dialog_pending(Rid content, uint32_t id) {
	rn_owner* owner = rn_owner_find(content);
	rn_slot* slot;
	if (!owner) return 0;
	for (slot = owner->slots; slot; slot = slot->next) {
		if (slot->id == id) return 1;
	}
	return 0;
}

static void rn_tray_forget(Rid tray) {
	rn_owner* owner = rn_owner_find(tray);
	if (owner) {
		rn_owner_unlink(owner);
		rn_owner_free(owner);
	}
}
''')

    # the two callbacks that come from other threads are written above: their shapes are known
    task = api.callback("RTaskCallback")
    if len(task.params) != 1 or task.ret != "void":
        raise ValueError("Node: RTaskCallback has changed")
    error = api.callback("RErrorCallback")
    if [p.ctype for p in error.params] != ["RError", "RSeverity", "const char*"] or error.ret != "void":
        raise ValueError("Node: RErrorCallback has changed")

    out.append("/* ---- the callbacks */\n")
    for c in api.callbacks:
        if c.name in used_callbacks and c.name not in OFF_THREAD:
            out.append("static %s rn_t_%s(%s);" % (c.ret, c.name, ", ".join(p.ctype for p in c.params) or "void"))
    out.append("")
    for c in api.callbacks:
        if c.name in used_callbacks and c.name not in OFF_THREAD:
            out.append(c_trampoline(api, c, callback_index[c.name]))
            out.append("")

    out.append("/* ---- the functions */\n")
    for f in api.functions:
        text = c_wrapper(api, f, slots, callback_index)
        out.append(c_add_after(text, f, slots))
        out.append("")

    out.append(C_NATIVE_HELPERS)

    out.append("static const napi_property_descriptor rn_properties[] = {")
    for f in api.functions:
        out.append('\t{"%s", NULL, rn_w_%s, NULL, NULL, NULL, napi_enumerable, NULL},' % (f.name, f.name))
    for name in ("_open", "_ptr", "_text", "_view", "_copied", "_poke", "_reporter"):
        out.append('\t{"%s", NULL, rn_w_%s, NULL, NULL, NULL, napi_enumerable, NULL},' % (name, name))
    out.append("};")
    out.append("")
    out.append("NAPI_MODULE_INIT() {")
    out.append("\tnapi_define_properties(env, exports, sizeof(rn_properties) / sizeof(rn_properties[0]), rn_properties);")
    out.append("\tnapi_add_env_cleanup_hook(env, rn_cleanup, NULL);")
    out.append("\treturn exports;")
    out.append("}")
    return "\n".join(out)


# ---- the module


JS_HEADER = '''/**
 * Raven for Node.js.
 *
 * Generated from the C headers by tools/bindgen (Raven %(version)s): do not edit.
 * The functions are in the addon (raven_napi.node), under their own names; this
 * module loads it and adds the structures. See README.md and raven.d.ts.
 */

import { Buffer } from "node:buffer";
import { createRequire } from "node:module";
import { dirname, join } from "node:path";
import process from "node:process";
import { fileURLToPath } from "node:url";

const _here = dirname(fileURLToPath(import.meta.url));

/** The addon: RAVEN_ADDON, then beside this file, then the folders of a build. */
function _load_addon() {
	const load = createRequire(import.meta.url);
	const explicit = process.env.RAVEN_ADDON;
	const candidates = [...(explicit ? [explicit] : []), join(_here, "raven_napi.node"), join(_here, "..", "build", "raven_napi.node"), join(_here, "..", "build", "Release", "raven_napi.node")];
	const tried = [];
	for (const candidate of candidates) {
		try {
			return load(candidate);
		} catch (error) {
			tried.push(`${candidate} (${error.message})`);
		}
	}
	throw new Error("the Raven addon was not found; set RAVEN_ADDON to its path. Tried:\\n  " + tried.join("\\n  "));
}

/** Opens the library: RAVEN_LIBRARY, then beside this file, then where the system looks. */
function _open_library(lib) {
	const names = process.platform === "win32" ? ["raven.dll", "libraven.dll"] : process.platform === "darwin" ? ["libraven.dylib"] : ["libraven.so", "libraven.so.0"];
	const explicit = process.env.RAVEN_LIBRARY;
	const candidates = [...(explicit ? [explicit] : []), ...names.map((name) => join(_here, name)), ...names];
	const tried = [];
	for (const candidate of candidates) {
		const result = lib._open(candidate);
		if (result === true) {
			return;
		}
		tried.push(`${candidate} (${result})`);
	}
	throw new Error("the Raven library was not found; set RAVEN_LIBRARY to its path. Tried:\\n  " + tried.join("\\n  "));
}

const _lib = _load_addon();
_open_library(_lib);

/** Prints what a callback threw: an exception must not cross the frames of C. */
_lib._reporter((error) => console.error("Raven: exception in a callback:", error));

/** An ArrayBuffer over memory of the library: a copy where the runtime does not allow that view. */
const _view_of = _lib._view;
const _copied = _lib._copied();

/** A text for the library. */
function _s(text) {
	return text === null ? null : Buffer.from(text + "\\0");
}

/** A text from the library. */
function _text(pointer) {
	return pointer ? _lib._text(pointer) : null;
}

/** An address read from memory, as a pointer. */
function _address(value) {
	return value === 0n ? null : Number(value);
}

/** Texts written by the library as an array of addresses. */
function _read_strings(pointer, count) {
	if (!pointer || count <= 0) {
		return [];
	}
	const addresses = new BigUint64Array(_view_of(pointer, count * 8));
	return Array.from(addresses, (address) => _text(_address(address)) ?? "");
}

/** A structure of the C API over its own zeroed memory, or over memory of the library when built from an address. */
export class RStruct {
	constructor(size, pointer) {
		const buffer = pointer ? _view_of(pointer, size) : new ArrayBuffer(size);
		/** Its bytes: what is given to the library. */
		this._bytes = new Uint8Array(buffer);
		this._view = new DataView(buffer);
		/** The texts and arrays its fields point to. */
		this._kept = null;
	}

	/** Remembers what a field points to, and writes its address there. */
	_point(field, offset, value) {
		(this._kept ??= new Map()).set(field, value ?? this._bytes);
		this._view.setBigUint64(offset, value === null ? 0n : BigInt(_lib._ptr(value)), true);
	}
}

/** A structure for the library, or nothing. */
function _p(value) {
	return value === null || value === undefined ? null : value._bytes;
}

/** Several structures for the library, one after the other. */
function _structs(values, size) {
	if (values === null || values === undefined || values.length === 0) {
		return null;
	}
	const block = new Uint8Array(values.length * size);
	values.forEach((value, index) => block.set(value._bytes, index * size));
	return block;
}

/** What a callback wrote into a structure, given back to the library where the structure is a copy. */
function _back(value, pointer) {
	if (_copied && value && pointer) {
		_lib._poke(pointer, value._bytes ?? value);
	}
}

/** The bytes at an address of the library, as bun:ffi gives them: valid as long as that memory is. */
export function toArrayBuffer(pointer, byteOffset = 0, byteLength) {
	return _view_of(pointer + byteOffset, byteLength);
}

/** The address of memory of JavaScript, as bun:ffi gives it: what the memory is, it stays while the view lives. */
export function ptr(view, byteOffset = 0) {
	return _lib._ptr(view) + byteOffset;
}

/** An enumeration, as TypeScript makes them: each name, and each value to its name. */
function _enum(items) {
	const result = {};
	for (const [name, value] of Object.entries(items)) {
		result[name] = value;
		result[value] = name;
	}
	return Object.freeze(result);
}
'''

JS_RINIT = '''
/**
 * Starts Raven, on this thread and without a render thread: Rinit_ex gives the choice.
 */
export function Rinit(backend) {
	const desc = new RInitDesc();
	desc.backend = backend;
	return Rinit_ex(desc);
}
'''

DTS_HEADER = '''/**
 * Raven for Node.js and Bun, through Node-API.
 *
 * Generated from the C headers by tools/bindgen (Raven %(version)s): do not edit.
 * Every function of the C API is here under its own name, with the conventions
 * of the Bun binding (bindings/bun), so that a program can use either:
 *
 *   - texts are strings on both sides (UTF-8 underneath);
 *   - a JavaScript function can be given wherever the C API takes a callback.
 *     An exception it throws is printed and the callback returns 0;
 *   - a parameter the C function writes through (int*, float*...) is not
 *     passed: its value is returned, after the result of the function if it
 *     has one;
 *   - the size that follows a text or bytes is not passed either;
 *   - a structure is a class with an accessor per field. One received by a
 *     callback reads the memory of the library: it is valid during the call;
 *   - `user_data` can be left out.
 *
 * See README.md for threads, the event loop and how long callbacks live.
 */

export type Pointer = number & { __pointer__: null };
/** A handle: every object of Raven is one. */
export type Rid = number;
/** An address, or memory of JavaScript given to the library for the time of the call. */
export type RPointer = NodeJS.TypedArray | Pointer | null;

/** The bytes at an address of the library (Rwindow_capture, Rimage_decode...): valid as long as that memory is. */
export function toArrayBuffer(pointer: Pointer, byteOffset: number | undefined, byteLength: number): ArrayBuffer;
/** The address of memory of JavaScript, for a function that takes a pointer as a number. */
export function ptr(view: NodeJS.TypedArray | ArrayBuffer | DataView, byteOffset?: number): Pointer;

/** A structure of the C API over its own zeroed memory, or over memory of the library when it is built from an address. */
export abstract class RStruct {
	/** Its bytes: what is given to the library. */
	readonly _bytes: Uint8Array;
	readonly _view: DataView;
	/** The texts and arrays its fields point to. */
	protected _kept: Map<string, object> | null;
	protected constructor(size: number, pointer?: Pointer);
	/** Remembers what a field points to, and writes its address there. */
	protected _point(field: string, offset: number, value: NodeJS.TypedArray | null): void;
}
'''

DTS_RUN_LOOP = '''
/**
 * Runs the interface until the last window closes or Rquit is called, leaving
 * the event loop of Node.js running: timers, promises, fetch and the rest go
 * on beside the interface.
 */
export function runLoop(interval_seconds?: number): Promise<void>;
'''

DTS_RINIT = '''
/**
 * Starts Raven, on this thread and without a render thread: Rinit_ex gives the choice.
 */
export function Rinit(backend: RBackend): number;
'''


def js_struct_field(api, s, f, offset, out):
    kind, detail = classify(f.ctype, api)
    view = "this._view"
    if f.array:
        if kind == "scalar" and detail == "char":
            out.append("\tget %s() {" % f.name)
            out.append("\t\tconst bytes = this._bytes.subarray(%d, %d);" % (offset, offset + f.array))
            out.append("\t\tconst end = bytes.indexOf(0);")
            out.append("\t\treturn Buffer.from(end < 0 ? bytes : bytes.subarray(0, end)).toString();")
            out.append("\t}")
            return
        if kind == "scalar" and detail == "float":
            out.append("\tget %s() {" % f.name)
            out.append("\t\treturn new Float32Array(this._bytes.buffer, %d, %d);" % (offset, f.array))
            out.append("\t}")
            out.append("\tset %s(value) {" % f.name)
            out.append("\t\tthis.%s.set(value);" % f.name)
            out.append("\t}")
            return
        raise ValueError("no array of %s in a structure for Node (%s.%s)" % (f.ctype, s.name, f.name))
    if kind == "scalar":
        method, _ = ACCESS[detail]
        little = ", true" if SCALAR[detail][1] > 1 else ""
        value = from_bigint(detail, "%s.get%s(%d%s)" % (view, method, offset, little))
        written = "BigInt(value)" if detail in ("size_t", "uintptr") else "value"
        out.append("\tget %s() {" % f.name)
        out.append("\t\treturn %s;" % value)
        out.append("\t}")
        out.append("\tset %s(value) {" % f.name)
        out.append("\t\t%s.set%s(%d, %s%s);" % (view, method, offset, written, little))
        out.append("\t}")
    elif kind == "enum":
        out.append("\tget %s() {" % f.name)
        out.append("\t\treturn %s.getInt32(%d, true);" % (view, offset))
        out.append("\t}")
        out.append("\tset %s(value) {" % f.name)
        out.append("\t\t%s.setInt32(%d, value, true);" % (view, offset))
        out.append("\t}")
    elif kind == "cstr":
        out.append("\tget %s() {" % f.name)
        out.append("\t\treturn _text(_address(%s.getBigUint64(%d, true)));" % (view, offset))
        out.append("\t}")
        out.append("\tset %s(value) {" % f.name)
        out.append('\t\tthis._point("%s", %d, _s(value));' % (f.name, offset))
        out.append("\t}")
    elif kind == "cstr_array":
        count = sibling_count(s, f)
        if not count:
            raise ValueError("no field for the length of %s.%s" % (s.name, f.name))
        out.append("\tget %s() {" % f.name)
        out.append("\t\treturn _read_strings(_address(%s.getBigUint64(%d, true)), this.%s);" % (view, offset, count))
        out.append("\t}")
    elif kind == "cstruct_ptr" and sibling_count(s, f):
        out.append("\tget %s() {" % f.name)
        out.append("\t\treturn _address(%s.getBigUint64(%d, true));" % (view, offset))
        out.append("\t}")
        out.append("\tset %s(value) {" % f.name)
        out.append('\t\tif (typeof value === "number") {')
        out.append("\t\t\t%s.setBigUint64(%d, BigInt(value), true);" % (view, offset))
        out.append("\t\t\treturn;")
        out.append("\t\t}")
        out.append('\t\tthis._point("%s", %d, _structs(value, %s.SIZE));' % (f.name, offset, detail))
        out.append('\t\t(this._kept ??= new Map()).set("%s.items", value ?? this._bytes);' % f.name)
        out.append("\t\tthis.%s = value?.length ?? 0;" % sibling_count(s, f))
        out.append("\t}")
    elif kind in ("fnptr", "ptr", "cptr", "cstruct_ptr", "struct_ptr", "opaque_ptr"):
        out.append("\tget %s() {" % f.name)
        out.append("\t\treturn _address(%s.getBigUint64(%d, true));" % (view, offset))
        out.append("\t}")
        out.append("\tset %s(value) {" % f.name)
        out.append("\t\t%s.setBigUint64(%d, BigInt(value ?? 0), true);" % (view, offset))
        out.append("\t}")
    else:
        raise ValueError("no field of the type %s for Node (%s.%s)" % (f.ctype, s.name, f.name))


def dts_struct_field(api, s, f, out):
    kind, detail = classify(f.ctype, api)
    out.extend(comment(doc_lines(f.doc), "\t"))
    if f.array:
        if detail == "char":
            out.append("\tget %s(): string;" % f.name)
        else:
            out.append("\tget %s(): Float32Array;" % f.name)
            out.append("\tset %s(value: ArrayLike<number>);" % f.name)
        return
    if kind == "scalar":
        t = scalar_type(detail)
        out.append("\tget %s(): %s;" % (f.name, t))
        out.append("\tset %s(value: %s);" % (f.name, t))
    elif kind == "enum":
        out.append("\tget %s(): %s;" % (f.name, detail))
        out.append("\tset %s(value: %s);" % (f.name, detail))
    elif kind == "cstr":
        out.append("\tget %s(): string | null;" % f.name)
        out.append("\tset %s(value: string | null);" % f.name)
    elif kind == "cstr_array":
        out.append("\tget %s(): string[];" % f.name)
    elif kind == "cstruct_ptr" and sibling_count(s, f):
        out.append("\tget %s(): Pointer | null;" % f.name)
        out.append("\tset %s(value: readonly %s[] | Pointer | null);" % (f.name, detail))
    else:
        out.append("\tget %s(): Pointer | null;" % f.name)
        out.append("\tset %s(value: Pointer | null);" % f.name)


def callback_public(api, c):
    """What a JavaScript function of this callback type receives and returns: the Bun signature."""
    public = []
    for p in c.params:
        kind, detail = classify(p.ctype, api)
        n = js_name(p.name)
        if kind == "scalar":
            public.append("%s: %s" % (n, scalar_type(detail)))
        elif kind == "enum":
            public.append("%s: %s" % (n, detail))
        elif kind == "cstr":
            public.append("%s: string | null" % n)
        elif kind in ("struct_ptr", "cstruct_ptr"):
            public.append("%s: %s | null" % (n, detail))
        elif kind == "out":
            array = "Int32Array" if detail[0] == "enum" else ACCESS[detail[1]][1]
            public.append("%s: %s | null" % (n, array))
        elif kind in ("ptr", "opaque_ptr", "cptr", "bytes", "cbytes"):
            public.append("%s: Pointer | null" % n)
        else:
            raise ValueError("no parameter of the type %s in a callback for Node (%s)" % (p.ctype, c.name))
    ret_kind, ret_detail = classify(c.ret, api)
    if ret_kind == "void":
        ret = "void"
    elif ret_kind == "cstr":
        ret = "string | null"
    elif ret_kind == "scalar" and ret_detail == "id":
        ret = "Rid"
    elif ret_kind == "scalar" and ret_detail == "int":
        ret = "number | boolean"
    else:
        raise ValueError("no result of the type %s for a callback for Node (%s)" % (c.ret, c.name))
    return public, ret


def js_adapter(api, c, out):
    """A callback that receives a structure or a place to write: JavaScript wraps the address."""
    needs = any(classify(p.ctype, api)[0] in ("struct_ptr", "cstruct_ptr", "out") for p in c.params)
    if not needs:
        return False
    names = [js_name(p.name) for p in c.params]
    out.append("")
    out.append("function _adapt_%s(function_) {" % c.name)
    out.append('\tif (typeof function_ !== "function") {')
    out.append("\t\treturn function_;")
    out.append("\t}")
    body = []
    backs = []
    passed = []
    for p, n in zip(c.params, names):
        kind, detail = classify(p.ctype, api)
        if kind in ("struct_ptr", "cstruct_ptr"):
            body.append("const %s_ = %s.from(%s);" % (n, detail, n))
            passed.append("%s_" % n)
            if kind == "struct_ptr":
                backs.append("_back(%s_, %s);" % (n, n))
        elif kind == "out":
            array = "Int32Array" if detail[0] == "enum" else ACCESS[detail[1]][1]
            body.append("const %s_ = %s ? new %s(_view_of(%s, %s.BYTES_PER_ELEMENT)) : null;" % (n, n, array, n, array))
            passed.append("%s_" % n)
            backs.append("_back(%s_, %s);" % (n, n))
        else:
            passed.append(n)
    out.append("\treturn (%s) => {" % ", ".join(names))
    for b in body:
        out.append("\t\t%s" % b)
    if backs:
        out.append("\t\tconst result_ = function_(%s);" % ", ".join(passed))
        for b in backs:
            out.append("\t\t%s" % b)
        out.append("\t\treturn result_;")
    else:
        out.append("\t\treturn function_(%s);" % ", ".join(passed))
    out.append("\t};")
    out.append("}")
    return True


def js_function(api, f, adapted):
    """JavaScript for one function: the export of the addon, or a wrapper when the module has to add something."""
    if f.name == "Rinit":
        return JS_RINIT.rstrip("\n")
    items, _sig, _outs = plan_function(api, f)
    ret_kind, ret_detail = classify(f.ret, api)
    needs_wrapper = ret_kind in ("struct_ptr", "cstruct_ptr")
    for it in items:
        if it["shape"] in ("struct", "struct_array"):
            needs_wrapper = True
        if it["shape"] == "callback" and it["detail"] in adapted:
            needs_wrapper = True
    if not needs_wrapper:
        return "export const %s = _lib.%s;" % (f.name, f.name)

    def default(it):
        last = it["index"] == len(f.params) - 1
        if it["shape"] == "ptr" and it["kind"] == "ptr" and it["p"].name == "user_data" and last:
            return " = null"
        return ""

    params = ["%s%s" % (it["js"], default(it)) for it in items if it["shape"] not in ("out", "cstr_out")]
    args = []
    for it in items:
        if it["shape"] in ("out", "cstr_out"):
            continue
        if it["shape"] == "struct":
            args.append("_p(%s)" % it["js"])
        elif it["shape"] == "struct_array":
            args.append("_structs(%s, %s.SIZE)" % (it["js"], it["struct"]))
            args.append("%s?.length ?? 0" % it["js"])
        elif it["shape"] == "callback" and it["detail"] in adapted:
            args.append("_adapt_%s(%s)" % (it["detail"], it["js"]))
        else:
            args.append(it["js"])
    call = "_lib.%s(%s)" % (f.name, ", ".join(args))
    if ret_kind in ("struct_ptr", "cstruct_ptr"):
        call = "%s.from(%s)" % (ret_detail, call)
    return "export function %s(%s) {\n\treturn %s;\n}" % (f.name, ", ".join(params), call)


def dts_function(api, f, out):
    if f.name == "Rinit":
        out.append(DTS_RINIT.rstrip("\n"))
        return
    items, signature, outs = plan_function(api, f)
    ret_kind, ret_detail = classify(f.ret, api)
    typed = None
    if ret_kind == "void":
        typed = None
    elif ret_kind == "scalar":
        typed = scalar_type(ret_detail)
    elif ret_kind == "enum":
        typed = ret_detail
    elif ret_kind == "cstr":
        typed = "string | null"
    elif ret_kind in ("ptr", "cptr", "bytes", "cbytes", "opaque_ptr"):
        typed = "Pointer | null"
    elif ret_kind in ("struct_ptr", "cstruct_ptr"):
        typed = "%s | null" % ret_detail
    elif ret_kind == "owned_str":
        typed = "Uint8Array | null"
    else:
        raise ValueError("no result of the type %s for Node (%s)" % (f.ret, f.name))

    lines = doc_lines(f.doc)
    if f.name in NATIVE_ONLY:
        lines = lines + (["", NATIVE_ONLY[f.name]] if lines else [NATIVE_ONLY[f.name]])
    if f.name in THREADSAFE:
        lines = lines + (["", THREADSAFE[f.name]] if lines else [THREADSAFE[f.name]])

    if ret_kind == "owned_str":
        annotation = "Uint8Array | null"
    elif not outs:
        annotation = typed or "void"
    else:
        entries = []
        if typed:
            entries.append(("result", typed))
        for o in outs:
            if o["shape"] == "cstr_out":
                entries.append((o["js"], "string | null"))
            elif o["detail"][0] == "enum":
                entries.append((o["js"], o["detail"][1]))
            else:
                entries.append((o["js"], scalar_type(o["detail"][1])))
        if len(entries) == 1:
            annotation = entries[0][1]
        else:
            annotation = "[%s]" % ", ".join("%s: %s" % e for e in entries)
    out.append("")
    out.extend(comment(lines))
    out.append("export function %s(%s): %s;" % (f.name, sig_for_declaration(signature), annotation))


def sig_for_declaration(signature):
    """An initializer is not allowed in a declaration: a parameter that has one is optional there."""
    parts = []
    for s in signature:
        if s.endswith(" = null"):
            name, typed = s[:-len(" = null")].split(": ", 1)
            parts.append("%s?: %s" % (name, typed))
        else:
            parts.append(s)
    return ", ".join(parts)


def generate_js(api):
    out = [JS_HEADER.replace("%(version)s", api.version)]
    for name, kind, value in api.defines:
        if kind == "str":
            out.append('export const %s = "%s";' % (name, value))
        else:
            out.append("export const %s = %s;" % (name, hex(value) if kind == "id" else value))
    for e in api.enums:
        out.append("")
        out.append("export const %s = _enum({" % e.name)
        for item, value, _doc in e.items:
            out.append("\t%s: %s," % (item, value))
        out.append("});")
        out.append("export const { %s } = %s;" % (", ".join(item for item, _v, _d in e.items), e.name))
    for s in api.structs:
        offsets, size = layout(api, s)
        out.append("")
        out.append("export class %s extends RStruct {" % s.name)
        out.append("\tstatic SIZE = %d;" % size)
        out.append("")
        out.append("\tconstructor(pointer) {")
        out.append("\t\tsuper(%s.SIZE, pointer);" % s.name)
        defaults = DEFAULTS.get(s.name)
        if defaults:
            out.append("\t\tif (!pointer) {")
            for name, value in defaults.items():
                out.append("\t\t\tthis.%s = %s;" % (name, value))
            out.append("\t\t}")
        out.append("\t}")
        out.append("")
        out.append("\t/** The structure at an address of the library, or null. */")
        out.append("\tstatic from(pointer) {")
        out.append("\t\treturn pointer ? new %s(pointer) : null;" % s.name)
        out.append("\t}")
        for f in s.fields:
            out.append("")
            js_struct_field(api, s, f, offsets[f.name], out)
        out.append("}")
    adapted = set()
    for c in api.callbacks:
        if any(p.ctype == c.name for f in api.functions if f.name not in NATIVE_ONLY for p in f.params) and js_adapter(api, c, out):
            adapted.add(c.name)
    out.append("")
    for f in api.functions:
        out.append(js_function(api, f, adapted))
    out.append(RUN_LOOP.replace("(interval_seconds: number = 1 / 60): Promise<void>", "(interval_seconds = 1 / 60)").replace("new Promise<void>(", "new Promise(").replace(" of Bun running", " of Node.js running"))
    return "\n".join(out)


def generate_dts(api):
    out = [DTS_HEADER.replace("%(version)s", api.version)]
    for name, kind, value in api.defines:
        out.append("export const %s: %s;" % (name, "string" if kind == "str" else "number"))
    for e in api.enums:
        out.append("")
        out.extend(comment(doc_lines(e.doc)))
        out.append("export enum %s {" % e.name)
        for item, value, doc in e.items:
            out.extend(comment(doc_lines(doc), "\t"))
            out.append("\t%s = %s," % (item, "0x%04X" % value if value > 9 else value))
        out.append("}")
        for item, value, doc in e.items:
            out.append("export const %s: %s.%s;" % (item, e.name, item))
    for s in api.structs:
        _offsets, size = layout(api, s)
        out.append("")
        out.extend(comment(doc_lines(s.doc)))
        out.append("export class %s extends RStruct {" % s.name)
        out.append("\tstatic readonly SIZE = %d;" % size)
        out.append("\tconstructor(pointer?: Pointer);")
        out.append("\t/** The structure at an address of the library, or null. */")
        out.append("\tstatic from(pointer: Pointer | null): %s | null;" % s.name)
        for f in s.fields:
            dts_struct_field(api, s, f, out)
        out.append("}")
    for c in api.callbacks:
        public, ret = callback_public(api, c)
        out.append("")
        out.extend(comment(doc_lines(c.doc)))
        out.append("export type %s = (%s) => %s;" % (c.name, ", ".join(public), ret))
    for f in api.functions:
        dts_function(api, f, out)
    out.append(DTS_RUN_LOOP.rstrip("\n"))
    return "\n".join(out)


def generate(api):
    return {
        "node/src/raven_napi.c": generate_c(api),
        "node/src/raven.js": generate_js(api),
        "node/src/raven.d.ts": generate_dts(api),
    }
