/**
 * Raven - script/rs_common.c
 * Memory, texts, types, signatures, numbers, units and the formatting of
 * diagnostics.
 */

#include "rs_internal.h"

#include <ctype.h>
#include <locale.h>
#include <math.h>

/* ---------------------------------------------------------------------------
 *  Memory
 * ------------------------------------------------------------------------- */

static void* (*g_rs_realloc)(void*, size_t) = NULL;
static void (*g_rs_free)(void*) = NULL;

void rs_set_allocator(void* (*realloc_fn)(void*, size_t), void (*free_fn)(void*)) {
	if (realloc_fn && free_fn) {
		g_rs_realloc = realloc_fn;
		g_rs_free = free_fn;
	} else {
		g_rs_realloc = NULL;
		g_rs_free = NULL;
	}
}

static void rs_out_of_memory(void) {
	/* Nothing sensible can go on: the C library would crash on NULL anyway. */
	fprintf(stderr, "Raven Script: out of memory\n");
	abort();
}

void* rs_malloc(size_t size) {
	void* pointer;
	if (size == 0) size = 1;
	pointer = g_rs_realloc ? g_rs_realloc(NULL, size) : malloc(size);
	if (!pointer) rs_out_of_memory();
	return pointer;
}

void* rs_calloc(size_t count, size_t size) {
	void* pointer;
	if (size && count > (size_t)-1 / size) rs_out_of_memory();
	pointer = rs_malloc(count * size);
	memset(pointer, 0, count * size > 0 ? count * size : 1);
	return pointer;
}

void* rs_realloc(void* pointer, size_t size) {
	void* result;
	if (size == 0) size = 1;
	result = g_rs_realloc ? g_rs_realloc(pointer, size) : realloc(pointer, size);
	if (!result) rs_out_of_memory();
	return result;
}

void rs_free(void* pointer) {
	if (!pointer) return;
	if (g_rs_free) g_rs_free(pointer);
	else free(pointer);
}

char* rs_strndup(const char* text, size_t length) {
	char* copy = (char*)rs_malloc(length + 1);
	if (length) memcpy(copy, text, length);
	copy[length] = '\0';
	return copy;
}

char* rs_strdup(const char* text) {
	return rs_strndup(text ? text : "", text ? strlen(text) : 0);
}

int rs_stricmp(const char* a, const char* b) {
	for (;; a++, b++) {
		int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);
		if (ca != cb) return ca - cb;
		if (!ca) return 0;
	}
}

/* ---------------------------------------------------------------------------
 *  Growing texts
 * ------------------------------------------------------------------------- */

static void rs_buf_reserve(rs_buf* buffer, size_t extra) {
	size_t needed = buffer->length + extra + 1;
	if (needed <= buffer->capacity) return;
	{
		size_t capacity = buffer->capacity ? buffer->capacity : 64;
		while (capacity < needed) capacity *= 2;
		buffer->data = (char*)rs_realloc(buffer->data, capacity);
		buffer->capacity = capacity;
	}
}

void rs_buf_put(rs_buf* buffer, const char* bytes, size_t count) {
	rs_buf_reserve(buffer, count);
	if (count) memcpy(buffer->data + buffer->length, bytes, count);
	buffer->length += count;
	buffer->data[buffer->length] = '\0';
}

void rs_buf_puts(rs_buf* buffer, const char* text) {
	rs_buf_put(buffer, text ? text : "", text ? strlen(text) : 0);
}

void rs_buf_putc(rs_buf* buffer, char c) {
	rs_buf_put(buffer, &c, 1);
}

void rs_buf_vprintf(rs_buf* buffer, const char* format, va_list arguments) {
	va_list copy;
	int needed;
	va_copy(copy, arguments);
	needed = vsnprintf(NULL, 0, format, copy);
	va_end(copy);
	if (needed <= 0) {
		rs_buf_reserve(buffer, 0);
		buffer->data[buffer->length] = '\0';
		return;
	}
	rs_buf_reserve(buffer, (size_t)needed);
	vsnprintf(buffer->data + buffer->length, (size_t)needed + 1, format, arguments);
	buffer->length += (size_t)needed;
}

void rs_buf_printf(rs_buf* buffer, const char* format, ...) {
	va_list arguments;
	va_start(arguments, format);
	rs_buf_vprintf(buffer, format, arguments);
	va_end(arguments);
}

void rs_buf_json_string(rs_buf* buffer, const char* text) {
	const unsigned char* p = (const unsigned char*)(text ? text : "");
	rs_buf_putc(buffer, '"');
	for (; *p; p++) {
		switch (*p) {
			case '"': rs_buf_puts(buffer, "\\\""); break;
			case '\\': rs_buf_puts(buffer, "\\\\"); break;
			case '\n': rs_buf_puts(buffer, "\\n"); break;
			case '\r': rs_buf_puts(buffer, "\\r"); break;
			case '\t': rs_buf_puts(buffer, "\\t"); break;
			default:
				if (*p < 0x20) rs_buf_printf(buffer, "\\u%04x", (unsigned)*p);
				else rs_buf_putc(buffer, (char)*p);
		}
	}
	rs_buf_putc(buffer, '"');
}

char* rs_buf_take(rs_buf* buffer) {
	char* data = buffer->data;
	if (!data) data = rs_strdup("");
	buffer->data = NULL;
	buffer->length = 0;
	buffer->capacity = 0;
	return data;
}

void rs_buf_free(rs_buf* buffer) {
	rs_free(buffer->data);
	buffer->data = NULL;
	buffer->length = 0;
	buffer->capacity = 0;
}

size_t rs_utf8_length(const char* text, size_t bytes) {
	size_t i, count = 0;
	for (i = 0; i < bytes; i++) {
		if (((unsigned char)text[i] & 0xC0u) != 0x80u) count++;
	}
	return count;
}

/* ---------------------------------------------------------------------------
 *  Arena
 * ------------------------------------------------------------------------- */

void* rs_arena_alloc(rs_arena* arena, size_t size) {
	rs_arena_block* block = arena->head;
	size = (size + 15u) & ~(size_t)15u;
	if (!block || block->used + size > block->size) {
		size_t block_size = size > 16384 ? size : 16384;
		block = (rs_arena_block*)rs_malloc(sizeof(rs_arena_block) + 16 + block_size);
		block->next = arena->head;
		block->used = 0;
		block->size = block_size;
		arena->head = block;
	}
	{
		/* the memory starts 16-aligned after the header */
		char* base = (char*)block + ((sizeof(rs_arena_block) + 15u) & ~(size_t)15u);
		void* pointer = base + block->used;
		block->used += size;
		memset(pointer, 0, size);
		return pointer;
	}
}

char* rs_arena_strndup(rs_arena* arena, const char* text, size_t length) {
	char* copy = (char*)rs_arena_alloc(arena, length + 1);
	if (length) memcpy(copy, text, length);
	copy[length] = '\0';
	return copy;
}

void rs_arena_free(rs_arena* arena) {
	rs_arena_block* block = arena->head;
	while (block) {
		rs_arena_block* next = block->next;
		rs_free(block);
		block = next;
	}
	arena->head = NULL;
}

/* ---------------------------------------------------------------------------
 *  Types
 * ------------------------------------------------------------------------- */

rs_type rs_type_make(rs_base base, int depth) {
	rs_type type;
	type.base = (uint8_t)base;
	type.depth = (uint8_t)(depth < 0 ? 0 : (depth > RS_MAX_ARRAY_DEPTH ? RS_MAX_ARRAY_DEPTH : depth));
	return type;
}

int rs_type_equal(rs_type a, rs_type b) {
	return a.base == b.base && a.depth == b.depth;
}

int rs_type_is_error(rs_type type) {
	return type.base == RS_T_ERROR;
}

int rs_type_assignable(rs_type target, rs_type value) {
	if (rs_type_is_error(target) || rs_type_is_error(value)) return 1;
	return rs_type_equal(target, value);
}

int rs_type_is_array(rs_type type) {
	return type.depth > 0 && !rs_type_is_error(type);
}

rs_type rs_type_element(rs_type type) {
	if (type.depth == 0) return rs_type_make(RS_T_ERROR, 0);
	return rs_type_make((rs_base)type.base, type.depth - 1);
}

static const char* const g_rs_base_names[] = { "<error>", "void", "null", "bool", "number", "string" };

const char* rs_type_name(rs_type type) {
	static char buffers[4][64];
	static int next = 0;
	char* out = buffers[next];
	int i;
	next = (next + 1) % 4;
	if (type.base > RS_T_STRING) type.base = RS_T_ERROR;
	snprintf(out, 64, "%s", g_rs_base_names[type.base]);
	for (i = 0; i < type.depth && strlen(out) + 2 < 64; i++) strcat(out, "[]");
	return out;
}

int rs_type_parse(const char* text, size_t length, rs_type* type) {
	size_t i, base_length = 0;
	int depth = 0;
	rs_base base;
	while (base_length < length && rs_is_ident_char((unsigned char)text[base_length])) base_length++;
	if (base_length == 6 && memcmp(text, "number", 6) == 0) base = RS_T_NUMBER;
	else if (base_length == 6 && memcmp(text, "string", 6) == 0) base = RS_T_STRING;
	else if (base_length == 4 && memcmp(text, "bool", 4) == 0) base = RS_T_BOOL;
	else if (base_length == 4 && memcmp(text, "void", 4) == 0) base = RS_T_VOID;
	else if (base_length == 4 && memcmp(text, "null", 4) == 0) base = RS_T_NULL;
	else return 0;
	i = base_length;
	while (i < length) {
		while (i < length && (text[i] == ' ' || text[i] == '\t')) i++;
		if (i >= length) break;
		if (i + 1 < length && text[i] == '[' && text[i + 1] == ']') {
			depth++;
			i += 2;
		} else {
			return 0;
		}
	}
	if (depth > RS_MAX_ARRAY_DEPTH) return 0;
	if (depth > 0 && (base == RS_T_VOID || base == RS_T_NULL)) return 0;
	*type = rs_type_make(base, depth);
	return 1;
}

void rs_sig_format(const rs_sig* sig, rs_buf* out) {
	int i;
	rs_buf_putc(out, '(');
	for (i = 0; i < sig->count; i++) {
		if (i) rs_buf_puts(out, ", ");
		if (sig->names[i] && sig->names[i][0]) rs_buf_printf(out, "%s: ", sig->names[i]);
		rs_buf_puts(out, rs_type_name(sig->params[i]));
	}
	rs_buf_printf(out, "): %s", rs_type_name(sig->ret));
}

static void rs_sig_error(char* error, size_t size, const char* message) {
	if (error && size) snprintf(error, size, "%s", message);
}

int rs_sig_parse(const char* text, rs_sig* sig, char* storage, size_t storage_size, char* error, size_t error_size) {
	const char* p = text ? text : "";
	size_t used = 0;
	memset(sig, 0, sizeof(*sig));
	sig->ret = rs_type_make(RS_T_VOID, 0);
	while (*p == ' ' || *p == '\t') p++;
	if (*p == '\0') return 1; /* no signature: () : void */
	if (*p != '(') {
		rs_sig_error(error, error_size, "a signature starts with '('");
		return 0;
	}
	p++;
	for (;;) {
		const char* name = NULL;
		size_t name_length = 0;
		const char* type_start;
		rs_type type;
		while (*p == ' ' || *p == '\t') p++;
		if (*p == ')') {
			if (sig->count > 0) {
				rs_sig_error(error, error_size, "a parameter is missing after ','");
				return 0;
			}
			p++;
			break;
		}
		/* "name: type" or "type" */
		type_start = p;
		while (rs_is_ident_char((unsigned char)*p)) p++;
		if (p == type_start) {
			rs_sig_error(error, error_size, "expected a parameter type");
			return 0;
		}
		{
			const char* q = p;
			while (*q == ' ' || *q == '\t') q++;
			if (*q == ':') {
				name = type_start;
				name_length = (size_t)(p - type_start);
				p = q + 1;
				while (*p == ' ' || *p == '\t') p++;
				type_start = p;
				while (rs_is_ident_char((unsigned char)*p)) p++;
			}
		}
		while (*p == ' ' || *p == '[' || *p == ']') p++;
		if (!rs_type_parse(type_start, (size_t)(p - type_start), &type) || type.base == RS_T_VOID || type.base == RS_T_NULL) {
			rs_sig_error(error, error_size, "a parameter is number, string or bool");
			return 0;
		}
		if (type.depth > 0) {
			rs_sig_error(error, error_size, "native functions do not take arrays");
			return 0;
		}
		if (sig->count >= RS_MAX_PARAMS) {
			rs_sig_error(error, error_size, "too many parameters (16 at most)");
			return 0;
		}
		sig->params[sig->count] = type;
		if (name && storage && used + name_length + 1 <= storage_size) {
			memcpy(storage + used, name, name_length);
			storage[used + name_length] = '\0';
			sig->names[sig->count] = storage + used;
			used += name_length + 1;
		}
		sig->count++;
		while (*p == ' ' || *p == '\t') p++;
		if (*p == ',') {
			p++;
			continue;
		}
		if (*p == ')') {
			p++;
			break;
		}
		rs_sig_error(error, error_size, "expected ',' or ')'");
		return 0;
	}
	while (*p == ' ' || *p == '\t') p++;
	if (*p == ':') {
		const char* type_start;
		p++;
		while (*p == ' ' || *p == '\t') p++;
		type_start = p;
		while (*p && *p != ' ' && *p != '\t') p++;
		if (!rs_type_parse(type_start, (size_t)(p - type_start), &sig->ret) || sig->ret.base == RS_T_NULL) {
			rs_sig_error(error, error_size, "the return type is void, number, string or bool");
			return 0;
		}
		if (sig->ret.depth > 0) {
			rs_sig_error(error, error_size, "native functions do not return arrays");
			return 0;
		}
		while (*p == ' ' || *p == '\t') p++;
	}
	if (*p != '\0') {
		rs_sig_error(error, error_size, "unexpected text after the signature");
		return 0;
	}
	return 1;
}

rs_handler_result rs_handler_check(const rs_sig* function, const rs_base* event_types, int event_count, int* bad_index) {
	int i;
	if (bad_index) *bad_index = 0;
	if (function->count > event_count) {
		if (bad_index) *bad_index = event_count;
		return RS_HANDLER_TOO_MANY_PARAMETERS;
	}
	for (i = 0; i < function->count; i++) {
		if (function->params[i].depth != 0 || function->params[i].base != event_types[i]) {
			if (bad_index) *bad_index = i;
			return RS_HANDLER_WRONG_TYPE;
		}
	}
	return RS_HANDLER_OK;
}

/* ---------------------------------------------------------------------------
 *  Numbers
 * ------------------------------------------------------------------------- */

/* strtod and printf follow the locale: '.' is swapped for its decimal point. */
static char rs_decimal_point(void) {
	const struct lconv* conv = localeconv();
	return conv && conv->decimal_point && conv->decimal_point[0] ? conv->decimal_point[0] : '.';
}

int rs_parse_number(const char* text, size_t length, double* value) {
	char buffer[128];
	char* end = NULL;
	size_t i = 0;
	char point = rs_decimal_point();
	if (length == 0 || length >= sizeof(buffer)) {
		if (length >= sizeof(buffer)) {
			/* a very long literal: digits only are accepted, read in pieces */
			double result = 0.0;
			for (i = 0; i < length; i++) {
				if (text[i] < '0' || text[i] > '9') return 0;
				result = result * 10.0 + (text[i] - '0');
			}
			*value = result;
			return 1;
		}
		return 0;
	}
	if (length > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
		double result = 0.0;
		for (i = 2; i < length; i++) {
			int c = (unsigned char)text[i], digit;
			if (c >= '0' && c <= '9') digit = c - '0';
			else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
			else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
			else return 0;
			result = result * 16.0 + digit;
		}
		*value = result;
		return 1;
	}
	/* digits [. digits] [e [+-] digits]: checked here, strtod accepts more (inf, nan, hex floats) */
	{
		size_t digits = 0;
		while (i < length && text[i] >= '0' && text[i] <= '9') { i++; digits++; }
		if (i < length && text[i] == '.') {
			i++;
			while (i < length && text[i] >= '0' && text[i] <= '9') { i++; digits++; }
		}
		if (digits == 0) return 0;
		if (i < length && (text[i] == 'e' || text[i] == 'E')) {
			size_t exponent_digits = 0;
			i++;
			if (i < length && (text[i] == '+' || text[i] == '-')) i++;
			while (i < length && text[i] >= '0' && text[i] <= '9') { i++; exponent_digits++; }
			if (exponent_digits == 0) return 0;
		}
		if (i != length) return 0;
	}
	memcpy(buffer, text, length);
	buffer[length] = '\0';
	for (i = 0; i < length; i++) {
		if (buffer[i] == '.') buffer[i] = point;
	}
	*value = strtod(buffer, &end);
	return end == buffer + length;
}

int rs_parse_number_text(const char* text, double* value) {
	const char* start = text ? text : "";
	const char* end;
	int negative = 0;
	double result;
	while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') start++;
	end = start + strlen(start);
	while (end > start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\n' || end[-1] == '\r')) end--;
	if (start < end && (*start == '-' || *start == '+')) {
		negative = *start == '-';
		start++;
	}
	if (start == end) return 0;
	if ((size_t)(end - start) == 8 && memcmp(start, "Infinity", 8) == 0) result = HUGE_VAL;
	else if (!rs_parse_number(start, (size_t)(end - start), &result)) return 0;
	*value = negative ? -result : result;
	return 1;
}

void rs_format_number(double value, char* out, size_t size) {
	char point = rs_decimal_point();
	size_t i;
	if (value != value) {
		snprintf(out, size, "NaN");
		return;
	}
	if (value == HUGE_VAL || value == -HUGE_VAL) {
		snprintf(out, size, value > 0 ? "Infinity" : "-Infinity");
		return;
	}
	if (value == 0.0) {
		/* -0 too */
		snprintf(out, size, "0");
		return;
	}
	if (fabs(value) < 1e15 && value == floor(value)) {
		snprintf(out, size, "%.0f", value);
		return;
	}
	{
		int precision;
		for (precision = 15; precision <= 17; precision++) {
			double back;
			char* end = NULL;
			snprintf(out, size, "%.*g", precision, value);
			back = strtod(out, &end);
			if (back == value) break;
		}
	}
	for (i = 0; out[i]; i++) {
		if (out[i] == point) out[i] = '.';
	}
}

/* ---------------------------------------------------------------------------
 *  Units
 * ------------------------------------------------------------------------- */

void rs_unit_init(rs_unit* unit, const char* file, const char* text, size_t length, int line, int column) {
	uint32_t i, capacity = 16;
	memset(unit, 0, sizeof(*unit));
	unit->file = rs_strdup(file && file[0] ? file : "<script>");
	unit->text = rs_strndup(text ? text : "", text ? length : 0);
	unit->length = (uint32_t)(text ? length : 0);
	unit->base_line = line > 0 ? line : 1;
	unit->base_column = column > 0 ? column : 1;
	unit->line_starts = (uint32_t*)rs_malloc(capacity * sizeof(uint32_t));
	unit->line_starts[0] = 0;
	unit->line_count = 1;
	for (i = 0; i < unit->length; i++) {
		if (unit->text[i] == '\n') {
			RS_GROW(unit->line_starts, unit->line_count, capacity);
			unit->line_starts[unit->line_count++] = i + 1;
		}
	}
}

void rs_unit_free(rs_unit* unit) {
	rs_free(unit->file);
	rs_free(unit->text);
	rs_free(unit->line_starts);
	memset(unit, 0, sizeof(*unit));
}

uint32_t rs_unit_line_of(const rs_unit* unit, uint32_t offset) {
	uint32_t low = 0, high = unit->line_count;
	if (offset > unit->length) offset = unit->length;
	while (high - low > 1) {
		uint32_t middle = (low + high) / 2;
		if (unit->line_starts[middle] <= offset) low = middle;
		else high = middle;
	}
	return low + 1;
}

void rs_unit_position(const rs_unit* unit, uint32_t offset, int* line, int* column) {
	uint32_t index = rs_unit_line_of(unit, offset) - 1;
	uint32_t start = unit->line_starts[index];
	int col;
	if (offset > unit->length) offset = unit->length;
	col = 1 + (int)rs_utf8_length(unit->text + start, offset - start);
	*line = unit->base_line + (int)index;
	*column = index == 0 ? unit->base_column + col - 1 : col;
}

void rs_unit_locate(const rs_unit* unit, int index, uint32_t start, uint32_t end, rs_location* location, rs_buf* storage) {
	uint32_t line_index, line_start, line_end;
	memset(location, 0, sizeof(*location));
	if (start > unit->length) start = unit->length;
	if (end < start) end = start;
	if (end > unit->length) end = unit->length;
	location->file = unit->file;
	location->unit = index;
	location->start = (int)start;
	location->end = (int)end;
	rs_unit_position(unit, start, &location->line, &location->column);
	rs_unit_position(unit, end, &location->end_line, &location->end_column);
	line_index = rs_unit_line_of(unit, start) - 1;
	line_start = unit->line_starts[line_index];
	line_end = line_index + 1 < unit->line_count ? unit->line_starts[line_index + 1] : unit->length;
	while (line_end > line_start && (unit->text[line_end - 1] == '\n' || unit->text[line_end - 1] == '\r')) line_end--;
	if (storage) {
		storage->length = 0;
		/* the first line of a unit starts after the tag in its file: pad it so that the columns match */
		if (line_index == 0) {
			int pad;
			for (pad = 1; pad < unit->base_column; pad++) rs_buf_putc(storage, ' ');
		}
		rs_buf_put(storage, unit->text + line_start, line_end - line_start);
		location->line_text = storage->data;
	}
}

/* ---------------------------------------------------------------------------
 *  Diagnostics
 * ------------------------------------------------------------------------- */

static int rs_digits(int number) {
	int digits = 1;
	while (number >= 10) {
		number /= 10;
		digits++;
	}
	return digits;
}

/* The width a line takes up to a column (tabs as 4 spaces); a column past the end counts its spaces when `past_end`. */
static int rs_visual_width(const char* text, int column, int past_end) {
	int width = 0, current = 1;
	const unsigned char* p = (const unsigned char*)text;
	while (*p && current < column) {
		if (*p == '\t') width += 4;
		else if ((*p & 0xC0u) != 0x80u) width++;
		if ((*p & 0xC0u) != 0x80u) current++;
		p++;
		while ((*p & 0xC0u) == 0x80u) p++;
	}
	if (past_end && column > current) width += column - current;
	return width;
}

static void rs_diag_excerpt(const rs_location* location, int gutter, rs_buf* out) {
	const char* p;
	int caret_start, caret_end, i;
	rs_buf_printf(out, "%*s--> %s:%d:%d\n", gutter, "", location->file ? location->file : "<script>", location->line, location->column);
	if (!location->line_text) return;
	rs_buf_printf(out, "%*s |\n", gutter, "");
	rs_buf_printf(out, "%*d | ", gutter, location->line);
	for (p = location->line_text; *p; p++) {
		if (*p == '\t') rs_buf_puts(out, "    ");
		else rs_buf_putc(out, *p);
	}
	rs_buf_putc(out, '\n');
	caret_start = rs_visual_width(location->line_text, location->column, 1);
	if (location->end_line == location->line && location->end_column > location->column) {
		caret_end = rs_visual_width(location->line_text, location->end_column, 1);
	} else {
		caret_end = rs_visual_width(location->line_text, 1 << 30, 0);
		if (location->end_line == location->line || caret_end <= caret_start) caret_end = caret_start + 1;
	}
	rs_buf_printf(out, "%*s | %*s", gutter, "", caret_start, "");
	for (i = caret_start; i < caret_end; i++) rs_buf_putc(out, '^');
	rs_buf_putc(out, '\n');
}

static void rs_diag_indented(const char* text, rs_buf* out) {
	const char* p = text;
	while (*p) {
		const char* end = strchr(p, '\n');
		size_t length = end ? (size_t)(end - p) : strlen(p);
		if (length) rs_buf_puts(out, "  ");
		rs_buf_put(out, p, length);
		rs_buf_putc(out, '\n');
		if (!end) break;
		p = end + 1;
	}
}

void rs_diag_format(const rs_diag* diag, rs_buf* out) {
	static const char* const names[] = { "error", "warning", "info" };
	int gutter = 1;
	if (diag->has_location) gutter = rs_digits(diag->location.line);
	if (diag->has_related && rs_digits(diag->related.line) > gutter) gutter = rs_digits(diag->related.line);
	rs_buf_printf(out, "%s[%s]: %s\n", names[diag->severity <= RS_SEVERITY_INFO ? diag->severity : 0], diag->code ? diag->code : "RS000", diag->title ? diag->title : "");
	if (diag->has_location) {
		rs_buf_putc(out, '\n');
		rs_diag_excerpt(&diag->location, gutter + 1, out);
	}
	if (diag->message && diag->message[0]) {
		rs_buf_putc(out, '\n');
		rs_diag_indented(diag->message, out);
	}
	if (diag->has_related) {
		rs_buf_putc(out, '\n');
		rs_diag_excerpt(&diag->related, gutter + 1, out);
		if (diag->related_note && diag->related_note[0]) rs_diag_indented(diag->related_note, out);
	}
	if (diag->help && diag->help[0]) {
		rs_buf_putc(out, '\n');
		rs_diag_indented(diag->help, out);
	}
}
