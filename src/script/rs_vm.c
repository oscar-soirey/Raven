/**
 * Raven Script - script/rs_vm.c
 * Values, the environment and the virtual machine.
 *
 * The machine runs validated bytecode (rs_chunk_validate) and still checks
 * the type of every value it works on: a function replaced by another with
 * a different signature, or a host that returns the wrong thing, gives a
 * run time error, never a crash. Script calls do not recurse in C: they
 * push frames on a stack of the environment, limited in depth; every
 * invocation has a budget of instructions; strings and arrays count against
 * a memory limit. An error ends the invocation it happened in and nothing
 * else.
 */

#include "rs_internal.h"

#include <math.h>
#include <locale.h>

/* ---------------------------------------------------------------------------
 *  Values
 * ------------------------------------------------------------------------- */

static int rs_memory_take(rs_env* env, size_t size) {
	if (!env) return 1;
	if (env->memory + size > env->max_memory) return 0;
	env->memory += size;
	return 1;
}

static void rs_memory_give(rs_env* env, size_t size) {
	if (!env) return;
	env->memory = env->memory >= size ? env->memory - size : 0;
}

rs_string* rs_string_new(rs_env* env, const char* text, size_t length) {
	rs_string* string;
	size_t size;
	if (length > 0x7FFFFFF0u) return NULL;
	size = sizeof(rs_string) + length;
	if (!rs_memory_take(env, size)) return NULL;
	string = (rs_string*)rs_malloc(size);
	string->refs = 1;
	string->length = (uint32_t)length;
	string->owner = env;
	if (length) memcpy(string->text, text, length);
	string->text[length] = '\0';
	return string;
}

rs_array* rs_array_new(rs_env* env, rs_type element, uint32_t capacity) {
	rs_array* array;
	size_t size = sizeof(rs_array) + (size_t)capacity * sizeof(rs_value);
	if (!rs_memory_take(env, size)) return NULL;
	array = (rs_array*)rs_calloc(1, sizeof(rs_array));
	array->refs = 1;
	array->owner = env;
	array->element = element;
	array->capacity = capacity;
	array->items = capacity ? (rs_value*)rs_calloc(capacity, sizeof(rs_value)) : NULL;
	return array;
}

int rs_array_push(rs_env* env, rs_array* array, rs_value value) {
	if (array->count >= array->capacity) {
		uint32_t capacity = array->capacity ? array->capacity * 2 : 4;
		if (capacity > 0x0FFFFFFFu) return 0;
		if (!rs_memory_take(env ? env : array->owner, (size_t)(capacity - array->capacity) * sizeof(rs_value))) return 0;
		array->items = (rs_value*)rs_realloc(array->items, (size_t)capacity * sizeof(rs_value));
		array->capacity = capacity;
	}
	array->items[array->count++] = value;
	return 1;
}

rs_value rs_value_void(void) {
	rs_value value;
	memset(&value, 0, sizeof(value));
	value.type = RS_V_VOID;
	return value;
}

rs_value rs_value_null(void) {
	rs_value value = rs_value_void();
	value.type = RS_V_NULL;
	return value;
}

rs_value rs_value_bool(int boolean) {
	rs_value value = rs_value_void();
	value.type = RS_V_BOOL;
	value.as.boolean = boolean ? 1 : 0;
	return value;
}

rs_value rs_value_number(double number) {
	rs_value value = rs_value_void();
	value.type = RS_V_NUMBER;
	value.as.number = number;
	return value;
}

rs_value rs_value_string(const char* text, size_t length) {
	rs_value value = rs_value_void();
	value.as.string = rs_string_new(NULL, text ? text : "", text ? length : 0);
	if (value.as.string) value.type = RS_V_STRING;
	return value;
}

rs_value rs_value_cstring(const char* text) {
	return rs_value_string(text ? text : "", text ? strlen(text) : 0);
}

void rs_value_retain(rs_value value) {
	if (value.type == RS_V_STRING) value.as.string->refs++;
	else if (value.type == RS_V_ARRAY) value.as.array->refs++;
}

void rs_value_release(rs_value value) {
	if (value.type == RS_V_STRING) {
		rs_string* string = value.as.string;
		if (--string->refs == 0) {
			rs_memory_give(string->owner, sizeof(rs_string) + string->length);
			rs_free(string);
		}
	} else if (value.type == RS_V_ARRAY) {
		rs_array* array = value.as.array;
		if (--array->refs == 0) {
			uint32_t i;
			/* items cannot hold their array (an array of T[] only holds T): no cycle, no deep recursion past the depth of the type */
			for (i = 0; i < array->count; i++) rs_value_release(array->items[i]);
			rs_memory_give(array->owner, sizeof(rs_array) + (size_t)array->capacity * sizeof(rs_value));
			rs_free(array->items);
			rs_free(array);
		}
	}
}

const char* rs_value_text(rs_value value) {
	return value.type == RS_V_STRING ? value.as.string->text : "";
}

rs_type rs_value_static_type(rs_value value) {
	switch (value.type) {
		case RS_V_VOID: return rs_type_make(RS_T_VOID, 0);
		case RS_V_NULL: return rs_type_make(RS_T_NULL, 0);
		case RS_V_BOOL: return rs_type_make(RS_T_BOOL, 0);
		case RS_V_NUMBER: return rs_type_make(RS_T_NUMBER, 0);
		case RS_V_STRING: return rs_type_make(RS_T_STRING, 0);
		case RS_V_ARRAY: return rs_type_make((rs_base)value.as.array->element.base, value.as.array->element.depth + 1);
		default: return rs_type_make(RS_T_ERROR, 0);
	}
}

/** Does a value have that type? (the error type accepts anything) */
static int rs_value_is(rs_value value, rs_type type) {
	if (rs_type_is_error(type)) return 1;
	return rs_type_equal(rs_value_static_type(value), type);
}

int rs_value_equal(rs_value a, rs_value b) {
	if (a.type != b.type) return 0;
	switch (a.type) {
		case RS_V_VOID:
		case RS_V_NULL: return 1;
		case RS_V_BOOL: return a.as.boolean == b.as.boolean;
		case RS_V_NUMBER: return a.as.number == b.as.number;
		case RS_V_STRING: return a.as.string->length == b.as.string->length && memcmp(a.as.string->text, b.as.string->text, a.as.string->length) == 0;
		case RS_V_ARRAY: return a.as.array == b.as.array;
		default: return 0;
	}
}

rs_value rs_value_default(rs_env* env, rs_type type) {
	if (type.depth > 0) {
		rs_value value = rs_value_void();
		value.as.array = rs_array_new(env, rs_type_element(type), 0);
		if (value.as.array) value.type = RS_V_ARRAY;
		return value;
	}
	switch (type.base) {
		case RS_T_NUMBER: return rs_value_number(0.0);
		case RS_T_BOOL: return rs_value_bool(0);
		case RS_T_STRING: {
			rs_value value = rs_value_void();
			value.as.string = rs_string_new(env, "", 0);
			if (value.as.string) value.type = RS_V_STRING;
			return value;
		}
		case RS_T_NULL: return rs_value_null();
		default: return rs_value_void();
	}
}

/* ---------------------------------------------------------------------------
 *  Environment
 * ------------------------------------------------------------------------- */

struct rs_frame {
	rs_chunk* chunk;
	uint32_t ip;
	/* stack index of the first local */
	uint32_t base;
	/* the type the caller expects back (checked on return) */
	rs_type expected;
};

rs_env* rs_env_create(const rs_host* host) {
	rs_env* env = (rs_env*)rs_calloc(1, sizeof(rs_env));
	if (host) env->host = *host;
	env->max_instructions = RS_DEFAULT_MAX_INSTRUCTIONS;
	env->max_depth = RS_DEFAULT_MAX_DEPTH;
	env->max_memory = RS_DEFAULT_MAX_MEMORY;
	return env;
}

static void rs_env_clear(rs_env* env, int notify) {
	uint32_t i;
	for (i = 0; i < env->function_count; i++) {
		rs_function* function = &env->functions[i];
		if (notify && env->host.function_removed) env->host.function_removed(env->host.user_data, function->name);
		rs_chunk_release(function->chunk);
		rs_free(function->name);
		rs_free(function->file);
	}
	for (i = 0; i < env->global_count; i++) {
		rs_value_release(env->globals[i].value);
		rs_free(env->globals[i].name);
		rs_free(env->globals[i].file);
	}
	env->function_count = 0;
	env->global_count = 0;
}

void rs_env_destroy(rs_env* env) {
	if (!env) return;
	rs_env_clear(env, 0);
	rs_free(env->functions);
	rs_free(env->globals);
	rs_free(env->stack);
	rs_free(env->frames);
	rs_free(env);
}

void rs_env_reset(rs_env* env) {
	if (env->running) return; /* (never while a script runs: its code would go away) */
	rs_env_clear(env, 1);
}

void rs_env_set_limits(rs_env* env, uint32_t max_instructions, int max_depth, size_t max_memory) {
	if (max_instructions) env->max_instructions = max_instructions;
	if (max_depth > 0) env->max_depth = max_depth > 100000 ? 100000 : max_depth;
	if (max_memory) env->max_memory = max_memory;
}

void rs_env_get_limits(const rs_env* env, uint32_t* max_instructions, int* max_depth, size_t* max_memory) {
	if (max_instructions) *max_instructions = env->max_instructions;
	if (max_depth) *max_depth = env->max_depth;
	if (max_memory) *max_memory = env->max_memory;
}

const rs_host* rs_env_host(const rs_env* env) {
	return &env->host;
}

size_t rs_env_memory(const rs_env* env) {
	return env->memory;
}

int rs_env_running(const rs_env* env) {
	return env->running;
}

void rs_env_report(rs_env* env, const rs_diag* diag) {
	if (env->host.diagnostic) env->host.diagnostic(env->host.user_data, diag);
}

int rs_env_function_index(const rs_env* env, const char* name) {
	uint32_t i;
	for (i = 0; i < env->function_count; i++) {
		if (strcmp(env->functions[i].name, name) == 0) return (int)i;
	}
	return -1;
}

int rs_env_find_function(const rs_env* env, const char* name, rs_sig* sig) {
	int index = rs_env_function_index(env, name);
	if (index < 0) return 0;
	if (sig) *sig = env->functions[index].chunk->sig;
	return 1;
}

int rs_env_function_location(const rs_env* env, const char* name, rs_location* location) {
	int index = rs_env_function_index(env, name);
	if (index < 0) return 0;
	memset(location, 0, sizeof(*location));
	location->file = env->functions[index].file;
	location->line = env->functions[index].line;
	location->column = env->functions[index].column;
	location->end_line = location->line;
	location->end_column = location->column;
	location->unit = -1;
	return 1;
}

int rs_env_function_count(const rs_env* env) {
	return (int)env->function_count;
}

const char* rs_env_function_name(const rs_env* env, int index) {
	return index >= 0 && (uint32_t)index < env->function_count ? env->functions[index].name : NULL;
}

int rs_env_global_index(const rs_env* env, const char* name) {
	uint32_t i;
	for (i = 0; i < env->global_count; i++) {
		if (strcmp(env->globals[i].name, name) == 0) return (int)i;
	}
	return -1;
}

void rs_env_define_global(rs_env* env, uint32_t slot, const char* name, rs_type type, int is_const, const char* file, int line, int column) {
	rs_global* global;
	if (slot >= env->global_count) {
		RS_GROW(env->globals, env->global_count, env->global_capacity);
		slot = env->global_count++;
		global = &env->globals[slot];
		memset(global, 0, sizeof(*global));
		global->name = rs_strdup(name);
	} else {
		global = &env->globals[slot];
		rs_value_release(global->value);
		rs_free(global->file);
	}
	global->type = type;
	global->is_const = is_const;
	global->value = rs_value_default(env, type);
	global->file = rs_strdup(file);
	global->line = line;
	global->column = column;
}

void rs_env_set_function(rs_env* env, const char* name, rs_chunk* chunk, const char* file, int line, int column) {
	int index = rs_env_function_index(env, name);
	rs_function* function;
	rs_chunk_retain(chunk);
	if (index >= 0) {
		function = &env->functions[index];
		/* a frame running the old code keeps its own reference */
		rs_chunk_release(function->chunk);
		rs_free(function->file);
	} else {
		RS_GROW(env->functions, env->function_count, env->function_capacity);
		function = &env->functions[env->function_count++];
		function->name = rs_strdup(name);
	}
	function->chunk = chunk;
	function->file = rs_strdup(file);
	function->line = line;
	function->column = column;
	if (env->host.function_installed) env->host.function_installed(env->host.user_data, name);
}

/* ---------------------------------------------------------------------------
 *  Errors while running
 * ------------------------------------------------------------------------- */

static void rs_runtime_error(rs_env* env, int base_frame, const char* code, const char* title, const char* help, const char* format, ...) {
	rs_diag diag;
	rs_buf message = { 0 };
	rs_buf line_text = { 0 };
	va_list arguments;
	int i;
	memset(&diag, 0, sizeof(diag));
	diag.severity = RS_SEVERITY_ERROR;
	diag.code = code;
	diag.title = title;
	diag.help = help;
	diag.runtime = 1;
	va_start(arguments, format);
	rs_buf_vprintf(&message, format, arguments);
	va_end(arguments);
	if (env->frame_count > base_frame) {
		rs_frame* frame = &env->frames[env->frame_count - 1];
		rs_chunk* chunk = frame->chunk;
		uint32_t ip = frame->ip ? frame->ip - 1 : 0;
		if (chunk->source && ip < chunk->count) {
			rs_unit_locate(&chunk->source->unit, -1, chunk->code[ip].start, chunk->code[ip].end, &diag.location, &line_text);
			diag.has_location = 1;
		}
		/* where it was called from */
		rs_buf_printf(&message, "\n\nIn '%s'", chunk->name);
		for (i = env->frame_count - 2; i >= base_frame; i--) {
			rs_frame* caller = &env->frames[i];
			uint32_t at = caller->ip ? caller->ip - 1 : 0;
			int line = 0, column = 0;
			if (caller->chunk->source && at < caller->chunk->count) rs_unit_position(&caller->chunk->source->unit, caller->chunk->code[at].start, &line, &column);
			rs_buf_printf(&message, ",\ncalled from '%s' (%s:%d:%d)", caller->chunk->name, caller->chunk->source ? caller->chunk->source->unit.file : "?", line, column);
			if (env->frame_count - i > 12 && i > base_frame + 2) {
				rs_buf_printf(&message, ",\n... (%d more)", i - base_frame);
				break;
			}
		}
		rs_buf_putc(&message, '.');
	}
	diag.message = message.data;
	rs_env_report(env, &diag);
	rs_buf_free(&message);
	rs_buf_free(&line_text);
}

/* ---------------------------------------------------------------------------
 *  Strings
 * ------------------------------------------------------------------------- */

/** Byte offset of the code point `index` (clamped to the length). */
static uint32_t rs_utf8_offset(const rs_string* string, double index) {
	uint32_t at = 0;
	double count = 0;
	if (index <= 0) return 0;
	while (at < string->length && count < index) {
		at++;
		while (at < string->length && ((unsigned char)string->text[at] & 0xC0u) == 0x80u) at++;
		count += 1;
	}
	return at;
}

static const char* rs_find(const char* haystack, size_t haystack_length, const char* needle, size_t needle_length) {
	size_t i;
	if (needle_length == 0) return haystack;
	if (needle_length > haystack_length) return NULL;
	for (i = 0; i + needle_length <= haystack_length; i++) {
		if (memcmp(haystack + i, needle, needle_length) == 0) return haystack + i;
	}
	return NULL;
}

/** Changes the case of ASCII and Latin-1 letters (é <-> É), copies the rest. */
static void rs_change_case(const rs_string* source, char* out, int upper) {
	uint32_t i;
	for (i = 0; i < source->length; i++) {
		unsigned char c = (unsigned char)source->text[i];
		if (upper && c >= 'a' && c <= 'z') c = (unsigned char)(c - 32);
		else if (!upper && c >= 'A' && c <= 'Z') c = (unsigned char)(c + 32);
		else if (c == 0xC3 && i + 1 < source->length) {
			/* U+00C0..U+00FF are C3 80..C3 BF: small letters are 0x20 above the capitals (not × ÷ ß ÿ) */
			unsigned char d = (unsigned char)source->text[i + 1];
			out[i] = (char)c;
			if (upper && d >= 0xA0 && d <= 0xBE && d != 0xB7) d = (unsigned char)(d - 0x20);
			else if (!upper && d >= 0x80 && d <= 0x9E && d != 0x97) d = (unsigned char)(d + 0x20);
			out[i + 1] = (char)d;
			i++;
			continue;
		}
		out[i] = (char)c;
	}
}

/* ---------------------------------------------------------------------------
 *  The machine
 * ------------------------------------------------------------------------- */

static int rs_stack_reserve(rs_env* env, uint32_t needed) {
	if (needed <= env->stack_size) return 1;
	{
		uint32_t size = env->stack_size ? env->stack_size : 256;
		while (size < needed) size *= 2;
		if (size > (1u << 24)) return 0;
		env->stack = (rs_value*)rs_realloc(env->stack, (size_t)size * sizeof(rs_value));
		memset(env->stack + env->stack_size, 0, (size_t)(size - env->stack_size) * sizeof(rs_value));
		env->stack_size = size;
	}
	return 1;
}

static int rs_push_frame(rs_env* env, rs_chunk* chunk, uint32_t base, rs_type expected) {
	rs_frame* frame;
	if ((env->frame_count + 1) % 16 == 1 || env->frames == NULL) {
		env->frames = (rs_frame*)rs_realloc(env->frames, (size_t)(env->frame_count + 16) * sizeof(rs_frame));
	}
	frame = &env->frames[env->frame_count++];
	frame->chunk = chunk;
	frame->ip = 0;
	frame->base = base;
	frame->expected = expected;
	rs_chunk_retain(chunk);
	env->depth++;
	return 1;
}

static void rs_pop_frame(rs_env* env) {
	rs_frame* frame = &env->frames[--env->frame_count];
	rs_chunk_release(frame->chunk);
	env->depth--;
}

static rs_type rs_decode_type(int32_t encoded) {
	return rs_type_make((rs_base)(encoded & 0xFF), (encoded >> 8) & 0xFF);
}

/** Calls the host: a native, an action bound by the application. */
static int rs_call_host(rs_env* env, int base_frame, const char* name, rs_value* args, int count, rs_type expected, rs_value* result) {
	rs_buf error = { 0 };
	rs_call_status status = RS_CALL_NOT_FOUND;
	*result = rs_value_void();
	if (env->host.call) status = env->host.call(env->host.user_data, name, args, count, result, &error);
	if (status == RS_CALL_NOT_FOUND) {
		rs_value_release(*result);
		*result = rs_value_void();
		rs_runtime_error(env, base_frame, "RS101", "Unknown function or variable.", "Help: bind it in C with Raction_bind_with_parameters (or Raction_bind) before it is called.",
			"The function '%s' is not bound by the application.", name);
		rs_buf_free(&error);
		return 0;
	}
	if (status != RS_CALL_OK) {
		/* a message "RS301: ..." gives its code (a widget that nobody named...) */
		const char* text = error.data && error.data[0] ? error.data : "no reason given.";
		rs_value_release(*result);
		*result = rs_value_void();
		if (strncmp(text, "RS301: ", 7) == 0) {
			rs_runtime_error(env, base_frame, "RS301", "Unknown UI element.", "Help: give the widget this name in the XML: Name=\"...\".", "%s", text + 7);
		} else if (strncmp(text, "RS104: ", 7) == 0) {
			rs_runtime_error(env, base_frame, "RS104", "Invalid conversion.", NULL, "%s", text + 7);
		} else {
			rs_runtime_error(env, base_frame, "RS205", "Native function failure.", NULL, "'%s' failed: %s", name, text);
		}
		rs_buf_free(&error);
		return 0;
	}
	rs_buf_free(&error);
	if (!rs_value_is(*result, expected)) {
		rs_type got = rs_value_static_type(*result);
		rs_value_release(*result);
		*result = rs_value_void();
		rs_runtime_error(env, base_frame, "RS205", "Native function failure.", NULL, "'%s' returned a '%s', but a '%s' was expected.", name, rs_type_name(got), rs_type_name(expected));
		return 0;
	}
	return 1;
}

#define RS_PUSH(v) (env->stack[env->stack_top++] = (v))
#define RS_POP() (env->stack[--env->stack_top])
#define RS_PEEK(n) (env->stack[env->stack_top - 1 - (n)])
#define RS_FAIL(code, title, help, ...) do { rs_runtime_error(env, base_frame, code, title, help, __VA_ARGS__); goto failed; } while (0)
#define RS_TYPE_FAIL() RS_FAIL("RS102", "Type mismatch.", NULL, "A value does not have the type the code expects (a function changed since this code was compiled?).")
#define RS_OOM() RS_FAIL("RS206", "Memory limit exceeded.", "Help: raise the limit with Rscript_set_limits.", "The scripts use more than %lu bytes.", (unsigned long)env->max_memory)

static rs_value rs_make_string(rs_env* env, const char* text, size_t length, int* ok) {
	rs_value value = rs_value_void();
	value.as.string = rs_string_new(env, text, length);
	if (value.as.string) value.type = RS_V_STRING;
	else *ok = 0;
	return value;
}

/** Runs from the frame on top until it returns. */
static rs_call_status rs_execute(rs_env* env, int base_frame, rs_value* result) {
	uint32_t budget = env->max_instructions;
	for (;;) {
		rs_frame* frame = &env->frames[env->frame_count - 1];
		rs_chunk* chunk = frame->chunk;
		const rs_instr* instr;
		if (budget-- == 0) {
			RS_FAIL("RS203", "Instruction limit exceeded.", "Help: look for a loop that never ends, or raise the limit with Rscript_set_limits.",
				"The script ran more than %lu instructions in one invocation: it was stopped.", (unsigned long)env->max_instructions);
		}
		instr = &chunk->code[frame->ip++];
		switch (instr->op) {
			case OP_NOP:
				break;
			case OP_CONST: {
				rs_value value = chunk->constants[instr->a];
				rs_value_retain(value);
				RS_PUSH(value);
				break;
			}
			case OP_NULL: RS_PUSH(rs_value_null()); break;
			case OP_TRUE: RS_PUSH(rs_value_bool(1)); break;
			case OP_FALSE: RS_PUSH(rs_value_bool(0)); break;
			case OP_POP: rs_value_release(RS_POP()); break;
			case OP_DUP2: {
				rs_value a = RS_PEEK(1), b = RS_PEEK(0);
				rs_value_retain(a);
				rs_value_retain(b);
				RS_PUSH(a);
				RS_PUSH(b);
				break;
			}
			case OP_LOAD_LOCAL: {
				rs_value value = env->stack[frame->base + (uint32_t)instr->a];
				rs_value_retain(value);
				RS_PUSH(value);
				break;
			}
			case OP_STORE_LOCAL: {
				rs_value* slot = &env->stack[frame->base + (uint32_t)instr->a];
				rs_value value = RS_POP();
				rs_value_release(*slot);
				*slot = value;
				break;
			}
			case OP_LOAD_GLOBAL: {
				rs_value value = env->globals[instr->a].value;
				rs_value_retain(value);
				RS_PUSH(value);
				break;
			}
			case OP_STORE_GLOBAL: {
				rs_global* global = &env->globals[instr->a];
				rs_value value = RS_POP();
				if (!rs_value_is(value, global->type)) {
					rs_value_release(value);
					RS_TYPE_FAIL();
				}
				rs_value_release(global->value);
				global->value = value;
				break;
			}
			case OP_ADD: {
				rs_value b = RS_POP(), a = RS_POP();
				if (a.type == RS_V_NUMBER && b.type == RS_V_NUMBER) {
					RS_PUSH(rs_value_number(a.as.number + b.as.number));
				} else if (a.type == RS_V_STRING && b.type == RS_V_STRING) {
					size_t length = (size_t)a.as.string->length + b.as.string->length;
					rs_string* string = NULL;
					rs_value value = rs_value_void();
					if (length <= 0x7FFFFFF0u && rs_memory_take(env, sizeof(rs_string) + length)) {
						string = (rs_string*)rs_malloc(sizeof(rs_string) + length);
						string->refs = 1;
						string->owner = env;
						string->length = (uint32_t)length;
						memcpy(string->text, a.as.string->text, a.as.string->length);
						memcpy(string->text + a.as.string->length, b.as.string->text, b.as.string->length);
						string->text[length] = '\0';
					}
					rs_value_release(a);
					rs_value_release(b);
					if (!string) RS_OOM();
					value.type = RS_V_STRING;
					value.as.string = string;
					RS_PUSH(value);
				} else {
					rs_value_release(a);
					rs_value_release(b);
					RS_TYPE_FAIL();
				}
				break;
			}
			case OP_SUB:
			case OP_MUL:
			case OP_DIV:
			case OP_MOD: {
				rs_value b = RS_POP(), a = RS_POP();
				double x, y, r = 0.0;
				if (a.type != RS_V_NUMBER || b.type != RS_V_NUMBER) {
					rs_value_release(a);
					rs_value_release(b);
					RS_TYPE_FAIL();
				}
				x = a.as.number;
				y = b.as.number;
				if ((instr->op == OP_DIV || instr->op == OP_MOD) && y == 0.0) {
					char shown[64];
					rs_format_number(x, shown, sizeof(shown));
					RS_FAIL("RS201", "Division by zero.", "Help: check the divisor before dividing.",
						instr->op == OP_DIV ? "%s was divided by zero." : "The remainder of %s divided by zero was asked.", shown);
				}
				if (instr->op == OP_SUB) r = x - y;
				else if (instr->op == OP_MUL) r = x * y;
				else if (instr->op == OP_DIV) r = x / y;
				else r = fmod(x, y);
				RS_PUSH(rs_value_number(r));
				break;
			}
			case OP_NEG: {
				rs_value a = RS_POP();
				if (a.type != RS_V_NUMBER) {
					rs_value_release(a);
					RS_TYPE_FAIL();
				}
				RS_PUSH(rs_value_number(-a.as.number));
				break;
			}
			case OP_NOT: {
				rs_value a = RS_POP();
				if (a.type != RS_V_BOOL) {
					rs_value_release(a);
					RS_TYPE_FAIL();
				}
				RS_PUSH(rs_value_bool(!a.as.boolean));
				break;
			}
			case OP_EQ:
			case OP_NE: {
				rs_value b = RS_POP(), a = RS_POP();
				int equal = rs_value_equal(a, b);
				rs_value_release(a);
				rs_value_release(b);
				RS_PUSH(rs_value_bool(instr->op == OP_EQ ? equal : !equal));
				break;
			}
			case OP_LT:
			case OP_LE:
			case OP_GT:
			case OP_GE: {
				rs_value b = RS_POP(), a = RS_POP();
				int order;
				if (a.type == RS_V_NUMBER && b.type == RS_V_NUMBER) {
					double x = a.as.number, y = b.as.number;
					int r = instr->op == OP_LT ? x < y : instr->op == OP_LE ? x <= y : instr->op == OP_GT ? x > y : x >= y;
					RS_PUSH(rs_value_bool(r));
					break;
				}
				if (a.type != RS_V_STRING || b.type != RS_V_STRING) {
					rs_value_release(a);
					rs_value_release(b);
					RS_TYPE_FAIL();
				}
				{
					uint32_t la = a.as.string->length, lb = b.as.string->length;
					int c = memcmp(a.as.string->text, b.as.string->text, la < lb ? la : lb);
					order = c != 0 ? c : (la < lb ? -1 : (la > lb ? 1 : 0));
				}
				rs_value_release(a);
				rs_value_release(b);
				RS_PUSH(rs_value_bool(instr->op == OP_LT ? order < 0 : instr->op == OP_LE ? order <= 0 : instr->op == OP_GT ? order > 0 : order >= 0));
				break;
			}
			case OP_JUMP:
				frame->ip = (uint32_t)instr->a;
				break;
			case OP_JUMP_IF_FALSE: {
				rs_value a = RS_POP();
				if (a.type != RS_V_BOOL) {
					rs_value_release(a);
					RS_TYPE_FAIL();
				}
				if (!a.as.boolean) frame->ip = (uint32_t)instr->a;
				break;
			}
			case OP_AND:
			case OP_OR: {
				rs_value a = RS_PEEK(0);
				if (a.type != RS_V_BOOL) RS_TYPE_FAIL();
				if ((instr->op == OP_AND && !a.as.boolean) || (instr->op == OP_OR && a.as.boolean)) frame->ip = (uint32_t)instr->a;
				else env->stack_top--;
				break;
			}
			case OP_ARRAY: {
				uint32_t count = (uint32_t)instr->a, i;
				rs_type element = rs_decode_type(instr->b);
				rs_array* array;
				rs_value value = rs_value_void();
				for (i = 0; i < count; i++) {
					if (!rs_value_is(env->stack[env->stack_top - count + i], element)) RS_TYPE_FAIL();
				}
				array = rs_array_new(env, element, count);
				if (!array) RS_OOM();
				/* the items move from the stack into the array */
				for (i = 0; i < count; i++) array->items[i] = env->stack[env->stack_top - count + i];
				array->count = count;
				env->stack_top -= count;
				value.type = RS_V_ARRAY;
				value.as.array = array;
				RS_PUSH(value);
				break;
			}
			case OP_INDEX: {
				rs_value index = RS_POP(), array = RS_POP();
				double i;
				rs_value item;
				if (array.type != RS_V_ARRAY || index.type != RS_V_NUMBER) {
					rs_value_release(index);
					rs_value_release(array);
					RS_TYPE_FAIL();
				}
				i = index.as.number;
				if (i != floor(i) || i < 0 || i >= (double)array.as.array->count) {
					char text[64];
					uint32_t count = array.as.array->count;
					rs_format_number(i, text, sizeof(text));
					rs_value_release(array);
					if (i != floor(i)) RS_FAIL("RS204", "Invalid array index.", NULL, "The index %s is not a whole number.", text);
					RS_FAIL("RS204", "Invalid array index.", "Help: check the index against .length first.", "The index %s is out of bounds: the array has %u item%s.", text, count, count == 1 ? "" : "s");
				}
				item = array.as.array->items[(uint32_t)i];
				rs_value_retain(item);
				rs_value_release(array);
				RS_PUSH(item);
				break;
			}
			case OP_STORE_INDEX: {
				rs_value value = RS_POP(), index = RS_POP(), array = RS_POP();
				double i;
				if (array.type != RS_V_ARRAY || index.type != RS_V_NUMBER || !rs_value_is(value, array.as.array->element)) {
					rs_value_release(value);
					rs_value_release(index);
					rs_value_release(array);
					RS_TYPE_FAIL();
				}
				i = index.as.number;
				if (i != floor(i) || i < 0 || i >= (double)array.as.array->count) {
					char text[64];
					uint32_t count = array.as.array->count;
					rs_format_number(i, text, sizeof(text));
					rs_value_release(value);
					rs_value_release(array);
					if (i != floor(i)) RS_FAIL("RS204", "Invalid array index.", NULL, "The index %s is not a whole number.", text);
					RS_FAIL("RS204", "Invalid array index.", "Help: add items with .push().", "The index %s is out of bounds: the array has %u item%s.", text, count, count == 1 ? "" : "s");
				}
				rs_value_release(array.as.array->items[(uint32_t)i]);
				array.as.array->items[(uint32_t)i] = value;
				rs_value_release(array);
				break;
			}
			case OP_CALL: {
				const char* name = chunk->constants[instr->a].as.string->text;
				int count = instr->b, index = rs_env_function_index(env, name), i;
				rs_chunk* callee;
				uint32_t base;
				if (index < 0) {
					RS_FAIL("RS101", "Unknown function or variable.", NULL, "The function '%s' no longer exists (its script was unloaded).", name);
				}
				callee = env->functions[index].chunk;
				if (callee->param_count != count) {
					RS_FAIL("RS106", "Invalid argument count.", NULL, "Function '%s' expects %d argument%s,\nbut %d %s provided (it changed since this code was compiled).",
						name, callee->param_count, callee->param_count == 1 ? "" : "s", count, count == 1 ? "was" : "were");
				}
				for (i = 0; i < count; i++) {
					rs_value argument = env->stack[env->stack_top - (uint32_t)count + (uint32_t)i];
					if (!rs_value_is(argument, callee->sig.params[i])) {
						RS_FAIL("RS102", "Argument type mismatch.", NULL, "Argument %d of '%s' expects '%s',\nbut got '%s' (it changed since this code was compiled).",
							i + 1, name, rs_type_name(callee->sig.params[i]), rs_type_name(rs_value_static_type(argument)));
					}
				}
				if (env->depth >= env->max_depth) {
					RS_FAIL("RS202", "Maximum recursion depth exceeded.", "Help: look for a function that calls itself without end.",
						"Calls are nested more than %d deep.", env->max_depth);
				}
				base = env->stack_top - (uint32_t)count;
				if (!rs_stack_reserve(env, base + (uint32_t)callee->local_count + (uint32_t)callee->max_stack + 1)) {
					RS_FAIL("RS202", "Maximum recursion depth exceeded.", NULL, "The stack of the scripts is full.");
				}
				for (i = count; i < callee->local_count; i++) env->stack[base + (uint32_t)i] = rs_value_void();
				env->stack_top = base + (uint32_t)callee->local_count;
				rs_push_frame(env, callee, base, rs_decode_type(instr->c));
				break;
			}
			case OP_CALL_EXTERNAL: {
				const char* name = chunk->constants[instr->a].as.string->text;
				int count = instr->b, i;
				rs_value returned;
				uint32_t base = env->stack_top - (uint32_t)count;
				int ok;
				rs_value args[RS_MAX_PARAMS];
				if (env->depth >= env->max_depth) {
					RS_FAIL("RS202", "Maximum recursion depth exceeded.", NULL, "Calls are nested more than %d deep.", env->max_depth);
				}
				/* the arguments stay on the stack (owned) while the host runs: a script it starts pushes above
				   them, and may move the stack, so the host gets a copy of the values */
				for (i = 0; i < count; i++) args[i] = env->stack[base + (uint32_t)i];
				env->depth++;
				ok = rs_call_host(env, base_frame, name, args, count, rs_decode_type(instr->c), &returned);
				env->depth--;
				frame = &env->frames[env->frame_count - 1];
				for (i = 0; i < count; i++) rs_value_release(env->stack[base + (uint32_t)i]);
				env->stack_top = base;
				if (!ok) goto failed_reported;
				RS_PUSH(returned);
				break;
			}
			case OP_INTRINSIC: {
				rs_value text = RS_POP();
				double number;
				if (text.type != RS_V_STRING) {
					rs_value_release(text);
					RS_TYPE_FAIL();
				}
				if (!rs_parse_number_text(text.as.string->text, &number)) {
					rs_buf shown = { 0 };
					rs_buf_put(&shown, text.as.string->text, text.as.string->length > 40 ? 40 : text.as.string->length);
					if (text.as.string->length > 40) rs_buf_puts(&shown, "...");
					rs_value_release(text);
					rs_runtime_error(env, base_frame, "RS104", "Invalid conversion.", "Help: check the text first, or give the user a hint of what is expected.", "'%s' is not a number.", shown.data ? shown.data : "");
					rs_buf_free(&shown);
					goto failed_reported;
				}
				rs_value_release(text);
				RS_PUSH(rs_value_number(number));
				break;
			}
			case OP_METHOD: {
				int ok = 1;
				rs_value result_value = rs_value_void();
				int count = instr->b, i;
				rs_value receiver = env->stack[env->stack_top - (uint32_t)count - 1];
				rs_value* args = env->stack + env->stack_top - count;
				switch (instr->a) {
					case RS_M_TO_STRING:
						if (receiver.type == RS_V_NUMBER) {
							char text[64];
							rs_format_number(receiver.as.number, text, sizeof(text));
							result_value = rs_make_string(env, text, strlen(text), &ok);
						} else if (receiver.type == RS_V_BOOL) {
							result_value = rs_make_string(env, receiver.as.boolean ? "true" : "false", receiver.as.boolean ? 4 : 5, &ok);
						} else if (receiver.type == RS_V_STRING) {
							result_value = receiver;
							rs_value_retain(result_value);
						} else {
							goto method_type;
						}
						break;
					case RS_M_LENGTH:
						if (receiver.type == RS_V_ARRAY) result_value = rs_value_number(receiver.as.array->count);
						else if (receiver.type == RS_V_STRING) result_value = rs_value_number((double)rs_utf8_length(receiver.as.string->text, receiver.as.string->length));
						else goto method_type;
						break;
					case RS_M_PUSH:
						if (receiver.type != RS_V_ARRAY || count != 1 || !rs_value_is(args[0], receiver.as.array->element)) goto method_type;
						rs_value_retain(args[0]);
						if (!rs_array_push(env, receiver.as.array, args[0])) {
							rs_value_release(args[0]);
							ok = 0;
						}
						break;
					case RS_M_POP:
						if (receiver.type != RS_V_ARRAY) goto method_type;
						if (receiver.as.array->count == 0) {
							RS_FAIL("RS204", "Invalid array index.", "Help: check .length before calling pop().", "pop() was called on an empty array.");
						}
						result_value = receiver.as.array->items[--receiver.as.array->count];
						break;
					case RS_M_TO_UPPER:
					case RS_M_TO_LOWER:
						if (receiver.type != RS_V_STRING) goto method_type;
						result_value = rs_make_string(env, receiver.as.string->text, receiver.as.string->length, &ok);
						if (ok) rs_change_case(receiver.as.string, result_value.as.string->text, instr->a == RS_M_TO_UPPER);
						break;
					case RS_M_TRIM: {
						const char* text;
						uint32_t start = 0, end;
						if (receiver.type != RS_V_STRING) goto method_type;
						text = receiver.as.string->text;
						end = receiver.as.string->length;
						while (start < end && (text[start] == ' ' || text[start] == '\t' || text[start] == '\n' || text[start] == '\r')) start++;
						while (end > start && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\n' || text[end - 1] == '\r')) end--;
						result_value = rs_make_string(env, text + start, end - start, &ok);
						break;
					}
					case RS_M_CONTAINS:
					case RS_M_STARTS_WITH:
					case RS_M_ENDS_WITH:
					case RS_M_INDEX_OF: {
						const rs_string* s;
						const rs_string* t;
						if (receiver.type != RS_V_STRING || count != 1 || args[0].type != RS_V_STRING) goto method_type;
						s = receiver.as.string;
						t = args[0].as.string;
						if (instr->a == RS_M_STARTS_WITH) result_value = rs_value_bool(t->length <= s->length && memcmp(s->text, t->text, t->length) == 0);
						else if (instr->a == RS_M_ENDS_WITH) result_value = rs_value_bool(t->length <= s->length && memcmp(s->text + s->length - t->length, t->text, t->length) == 0);
						else {
							const char* found = rs_find(s->text, s->length, t->text, t->length);
							if (instr->a == RS_M_CONTAINS) result_value = rs_value_bool(found != NULL);
							else result_value = rs_value_number(found ? (double)rs_utf8_length(s->text, (size_t)(found - s->text)) : -1.0);
						}
						break;
					}
					case RS_M_SUBSTRING: {
						double from, to;
						uint32_t a, b;
						if (receiver.type != RS_V_STRING || count != 2 || args[0].type != RS_V_NUMBER || args[1].type != RS_V_NUMBER) goto method_type;
						from = args[0].as.number != args[0].as.number ? 0 : floor(args[0].as.number);
						to = args[1].as.number != args[1].as.number ? 0 : floor(args[1].as.number);
						if (from > to) {
							double swap = from;
							from = to;
							to = swap;
						}
						a = rs_utf8_offset(receiver.as.string, from);
						b = rs_utf8_offset(receiver.as.string, to);
						result_value = rs_make_string(env, receiver.as.string->text + a, b - a, &ok);
						break;
					}
					case RS_M_SPLIT: {
						const rs_string* s;
						const rs_string* separator;
						rs_array* parts;
						uint32_t at = 0;
						if (receiver.type != RS_V_STRING || count != 1 || args[0].type != RS_V_STRING) goto method_type;
						s = receiver.as.string;
						separator = args[0].as.string;
						parts = rs_array_new(env, rs_type_make(RS_T_STRING, 0), 0);
						if (!parts) {
							ok = 0;
							break;
						}
						result_value.type = RS_V_ARRAY;
						result_value.as.array = parts;
						while (ok) {
							uint32_t next;
							rs_value part;
							if (separator->length == 0) {
								/* one item per character */
								if (at >= s->length) break;
								next = at + 1;
								while (next < s->length && ((unsigned char)s->text[next] & 0xC0u) == 0x80u) next++;
								part = rs_make_string(env, s->text + at, next - at, &ok);
								if (ok && !rs_array_push(env, parts, part)) {
									rs_value_release(part);
									ok = 0;
								}
								at = next;
								continue;
							}
							{
								const char* found = rs_find(s->text + at, s->length - at, separator->text, separator->length);
								next = found ? (uint32_t)(found - s->text) : s->length;
							}
							part = rs_make_string(env, s->text + at, next - at, &ok);
							if (ok && !rs_array_push(env, parts, part)) {
								rs_value_release(part);
								ok = 0;
							}
							if (next >= s->length) break;
							at = next + separator->length;
							if (at > s->length) break;
							if (at == s->length) {
								/* "a," gives ["a", ""] */
								part = rs_make_string(env, "", 0, &ok);
								if (ok && !rs_array_push(env, parts, part)) {
									rs_value_release(part);
									ok = 0;
								}
								break;
							}
						}
						break;
					}
					case RS_M_TO_FIXED: {
						char text[512];
						double digits;
						size_t k;
						const struct lconv* conv;
						if (receiver.type != RS_V_NUMBER || count != 1 || args[0].type != RS_V_NUMBER) goto method_type;
						digits = args[0].as.number;
						if (digits != floor(digits) || digits < 0 || digits > 20) {
							char shown[64];
							rs_format_number(digits, shown, sizeof(shown));
							RS_FAIL("RS104", "Invalid conversion.", NULL, "toFixed takes a whole number of digits from 0 to 20, but got %s.", shown);
						}
						if (receiver.as.number != receiver.as.number || fabs(receiver.as.number) >= 1e21) {
							rs_format_number(receiver.as.number, text, sizeof(text));
						} else {
							snprintf(text, sizeof(text), "%.*f", (int)digits, receiver.as.number);
							conv = localeconv();
							for (k = 0; text[k]; k++) {
								if (conv && conv->decimal_point && text[k] == conv->decimal_point[0]) text[k] = '.';
							}
							if (strcmp(text, "-0") == 0 || (text[0] == '-' && strspn(text + 1, "0.") == strlen(text + 1))) memmove(text, text + 1, strlen(text));
						}
						result_value = rs_make_string(env, text, strlen(text), &ok);
						break;
					}
					default:
						goto method_type;
				}
				if (!ok) {
					rs_value_release(result_value);
					RS_OOM();
				}
				/* pop the receiver and the arguments, push the result */
				for (i = 0; i <= count; i++) rs_value_release(RS_POP());
				RS_PUSH(result_value);
				break;
			method_type:
				RS_TYPE_FAIL();
			}
			case OP_RETURN:
			case OP_RETURN_VOID: {
				rs_value value = instr->op == OP_RETURN ? RS_POP() : rs_value_void();
				uint32_t base = frame->base;
				rs_type expected = frame->expected;
				if (!rs_value_is(value, chunk->sig.ret) || !rs_value_is(value, expected)) {
					rs_value_release(value);
					RS_TYPE_FAIL();
				}
				while (env->stack_top > base) rs_value_release(RS_POP());
				rs_pop_frame(env);
				if (env->frame_count == base_frame) {
					if (result) *result = value;
					else rs_value_release(value);
					return RS_CALL_OK;
				}
				RS_PUSH(value);
				break;
			}
			case OP_MISSING_RETURN:
				RS_FAIL("RS108", "Missing return.", NULL, "The function '%s' ended without returning a '%s'.", chunk->name, rs_type_name(chunk->sig.ret));
			default:
				RS_FAIL("RS000", "Internal error.", NULL, "Unknown instruction %d.", instr->op);
		}
	}
failed:
failed_reported:
	return RS_CALL_FAILED;
}

rs_call_status rs_vm_run(rs_env* env, rs_chunk* chunk, const rs_value* args, int count, rs_value* result) {
	int base_frame = env->frame_count, i;
	uint32_t base = env->stack_top;
	rs_call_status status;
	if (result) *result = rs_value_void();
	if (env->depth >= env->max_depth) {
		rs_runtime_error(env, base_frame, "RS202", "Maximum recursion depth exceeded.", NULL, "Calls are nested more than %d deep.", env->max_depth);
		return RS_CALL_FAILED;
	}
	if (!rs_stack_reserve(env, base + (uint32_t)chunk->local_count + (uint32_t)chunk->max_stack + 1)) {
		rs_runtime_error(env, base_frame, "RS202", "Maximum recursion depth exceeded.", NULL, "The stack of the scripts is full.");
		return RS_CALL_FAILED;
	}
	for (i = 0; i < chunk->local_count; i++) {
		rs_value value = i < count ? args[i] : rs_value_void();
		rs_value_retain(value);
		env->stack[base + (uint32_t)i] = value;
	}
	env->stack_top = base + (uint32_t)chunk->local_count;
	rs_push_frame(env, chunk, base, rs_type_make(RS_T_ERROR, 0));
	env->running++;
	status = rs_execute(env, base_frame, result);
	env->running--;
	if (status != RS_CALL_OK) {
		/* unwind what this invocation left */
		while (env->frame_count > base_frame) rs_pop_frame(env);
		while (env->stack_top > base) rs_value_release(env->stack[--env->stack_top]);
	}
	return status;
}

/* ---------------------------------------------------------------------------
 *  Calls from the host
 * ------------------------------------------------------------------------- */

static void rs_event_error(rs_env* env, const rs_function* function, const char* event, const char* format, ...) {
	rs_diag diag;
	rs_buf message = { 0 };
	va_list arguments;
	memset(&diag, 0, sizeof(diag));
	diag.severity = RS_SEVERITY_ERROR;
	diag.code = "RS103";
	diag.title = "Invalid event handler signature.";
	diag.runtime = 1;
	va_start(arguments, format);
	rs_buf_vprintf(&message, format, arguments);
	va_end(arguments);
	diag.message = message.data;
	if (function->file) {
		diag.has_location = 1;
		diag.location.file = function->file;
		diag.location.line = function->line;
		diag.location.column = function->column;
		diag.location.end_line = function->line;
		diag.location.end_column = function->column;
		diag.location.unit = -1;
	}
	RS_UNUSED(event);
	rs_env_report(env, &diag);
	rs_buf_free(&message);
}

rs_call_status rs_env_call(rs_env* env, const char* name, const rs_value* args, int count, rs_call_mode mode, const char* event, rs_value* result) {
	int index = rs_env_function_index(env, name), i;
	rs_function* function;
	rs_chunk* chunk;
	rs_call_status status;
	if (result) *result = rs_value_void();
	if (index < 0) return RS_CALL_NOT_FOUND;
	function = &env->functions[index];
	chunk = function->chunk;
	if (count < 0) count = 0;
	if (mode == RS_CALL_EVENT ? count < chunk->param_count : count != chunk->param_count) {
		if (mode == RS_CALL_EVENT) {
			rs_event_error(env, function, event, "%s provides %d argument%s,\nbut '%s' requires %d.\n\nRemove the additional required parameter%s.",
				event ? event : "The event", count, count == 1 ? "" : "s", name, chunk->param_count, chunk->param_count - count > 1 ? "s" : "");
		} else {
			rs_diag diag;
			rs_buf message = { 0 };
			memset(&diag, 0, sizeof(diag));
			diag.severity = RS_SEVERITY_ERROR;
			diag.code = "RS106";
			diag.title = "Invalid argument count.";
			diag.runtime = 1;
			rs_buf_printf(&message, "Function '%s' expects %d argument%s,\nbut %d %s provided.", name, chunk->param_count, chunk->param_count == 1 ? "" : "s", count, count == 1 ? "was" : "were");
			diag.message = message.data;
			rs_env_report(env, &diag);
			rs_buf_free(&message);
		}
		return RS_CALL_FAILED;
	}
	for (i = 0; i < chunk->param_count; i++) {
		if (!rs_value_is(args[i], chunk->sig.params[i])) {
			const char* got = rs_type_name(rs_value_static_type(args[i]));
			if (mode == RS_CALL_EVENT) {
				rs_event_error(env, function, event, "%s gives argument %d of type '%s',\nbut parameter '%s' of '%s' has type '%s'.",
					event ? event : "The event", i + 1, got, chunk->param_names[i] ? chunk->param_names[i] : "?", name, rs_type_name(chunk->sig.params[i]));
			} else {
				rs_diag diag;
				rs_buf message = { 0 };
				memset(&diag, 0, sizeof(diag));
				diag.severity = RS_SEVERITY_ERROR;
				diag.code = "RS102";
				diag.title = "Argument type mismatch.";
				diag.runtime = 1;
				rs_buf_printf(&message, "Argument %d of '%s' expects '%s',\nbut got '%s'.", i + 1, name, rs_type_name(chunk->sig.params[i]), got);
				diag.message = message.data;
				rs_env_report(env, &diag);
				rs_buf_free(&message);
			}
			return RS_CALL_FAILED;
		}
	}
	/* the chunk stays alive even when the function is replaced while it runs */
	rs_chunk_retain(chunk);
	status = rs_vm_run(env, chunk, args, chunk->param_count, result);
	rs_chunk_release(chunk);
	return status;
}
