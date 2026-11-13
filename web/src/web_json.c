/**
 * Raven Web - web_json.c
 * Memory, growing text, a JSON reader that works in place, a JSON writer,
 * base64. Thread-safe as long as each object is used by one thread.
 */

#include "web_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* ---------------------------------------------------------------------------
 *  Memory: the allocator of Raven, so that the statistics of the application
 *  see this library too
 * ------------------------------------------------------------------------- */

void* rw_alloc(size_t size) {
	return Rmemory_alloc(size ? size : 1);
}

void* rw_realloc(void* pointer, size_t size) {
	return Rmemory_realloc(pointer, size ? size : 1);
}

void rw_free(void* pointer) {
	if (pointer) Rmemory_free(pointer);
}

char* rw_strdup(const char* text) {
	size_t length;
	char* copy;
	if (!text) return NULL;
	length = strlen(text);
	copy = (char*)rw_alloc(length + 1);
	if (copy) memcpy(copy, text, length + 1);
	return copy;
}

/* ---------------------------------------------------------------------------
 *  Text
 * ------------------------------------------------------------------------- */

static int rw_text_reserve(rw_text_t* text, size_t more) {
	size_t needed = text->length + more + 1, capacity = text->capacity ? text->capacity : 128;
	char* data;
	if (needed <= text->capacity) return 1;
	while (capacity < needed) capacity *= 2;
	data = (char*)rw_realloc(text->data, capacity);
	if (!data) return 0;
	text->data = data;
	text->capacity = capacity;
	return 1;
}

void rw_text_add(rw_text_t* text, const char* data, size_t size) {
	if (!rw_text_reserve(text, size)) return;
	memcpy(text->data + text->length, data, size);
	text->length += size;
	text->data[text->length] = 0;
}

void rw_text_puts(rw_text_t* text, const char* string) {
	if (string) rw_text_add(text, string, strlen(string));
}

void rw_text_printf(rw_text_t* text, const char* format, ...) {
	char small[256];
	va_list arguments;
	int length;
	va_start(arguments, format);
	length = vsnprintf(small, sizeof(small), format, arguments);
	va_end(arguments);
	if (length < 0) return;
	if ((size_t)length < sizeof(small)) {
		rw_text_add(text, small, (size_t)length);
		return;
	}
	if (!rw_text_reserve(text, (size_t)length)) return;
	va_start(arguments, format);
	vsnprintf(text->data + text->length, (size_t)length + 1, format, arguments);
	va_end(arguments);
	text->length += (size_t)length;
}

void rw_text_json(rw_text_t* text, const char* string) {
	const unsigned char* cursor = (const unsigned char*)(string ? string : "");
	const unsigned char* run = cursor;
	rw_text_add(text, "\"", 1);
	for (; *cursor; cursor++) {
		const char* escape = NULL;
		char code[8];
		switch (*cursor) {
			case '"': escape = "\\\""; break;
			case '\\': escape = "\\\\"; break;
			case '\n': escape = "\\n"; break;
			case '\r': escape = "\\r"; break;
			case '\t': escape = "\\t"; break;
			default:
				if (*cursor < 0x20) {
					snprintf(code, sizeof(code), "\\u%04x", (unsigned)*cursor);
					escape = code;
				}
				break;
		}
		if (!escape) continue;
		rw_text_add(text, (const char*)run, (size_t)(cursor - run));
		rw_text_puts(text, escape);
		run = cursor + 1;
	}
	rw_text_add(text, (const char*)run, (size_t)(cursor - run));
	rw_text_add(text, "\"", 1);
}

void rw_text_free(rw_text_t* text) {
	rw_free(text->data);
	text->data = NULL;
	text->length = text->capacity = 0;
}

/* ---------------------------------------------------------------------------
 *  JSON reader
 * ------------------------------------------------------------------------- */

#define RW_JSON_BLOCK 64
#define RW_JSON_DEPTH 64

typedef struct rw_json_block_t {
	struct rw_json_block_t* next;
	int used;
	rw_json_t values[RW_JSON_BLOCK];
} rw_json_block_t;

typedef struct rw_json_reader_t {
	rw_json_document_t* document;
	char* cursor;
	char* end;
	int failed;
} rw_json_reader_t;

static rw_json_t* rw_json_new(rw_json_reader_t* reader) {
	rw_json_block_t* block = (rw_json_block_t*)reader->document->blocks;
	rw_json_t* value;
	if (!block || block->used == RW_JSON_BLOCK) {
		block = (rw_json_block_t*)rw_alloc(sizeof(rw_json_block_t));
		if (!block) {
			reader->failed = 1;
			return NULL;
		}
		block->next = (rw_json_block_t*)reader->document->blocks;
		block->used = 0;
		reader->document->blocks = block;
	}
	value = &block->values[block->used++];
	memset(value, 0, sizeof(*value));
	return value;
}

static void rw_json_space(rw_json_reader_t* reader) {
	while (reader->cursor < reader->end && (*reader->cursor == ' ' || *reader->cursor == '\n' || *reader->cursor == '\r' || *reader->cursor == '\t')) reader->cursor++;
}

static int rw_json_hex(const char* text, unsigned* value) {
	unsigned result = 0;
	int i;
	for (i = 0; i < 4; i++) {
		char c = text[i];
		result <<= 4;
		if (c >= '0' && c <= '9') result |= (unsigned)(c - '0');
		else if (c >= 'a' && c <= 'f') result |= (unsigned)(c - 'a' + 10);
		else if (c >= 'A' && c <= 'F') result |= (unsigned)(c - 'A' + 10);
		else return 0;
	}
	*value = result;
	return 1;
}

static char* rw_json_utf8(char* out, unsigned code) {
	if (code < 0x80) {
		*out++ = (char)code;
	} else if (code < 0x800) {
		*out++ = (char)(0xC0 | (code >> 6));
		*out++ = (char)(0x80 | (code & 0x3F));
	} else if (code < 0x10000) {
		*out++ = (char)(0xE0 | (code >> 12));
		*out++ = (char)(0x80 | ((code >> 6) & 0x3F));
		*out++ = (char)(0x80 | (code & 0x3F));
	} else {
		*out++ = (char)(0xF0 | (code >> 18));
		*out++ = (char)(0x80 | ((code >> 12) & 0x3F));
		*out++ = (char)(0x80 | ((code >> 6) & 0x3F));
		*out++ = (char)(0x80 | (code & 0x3F));
	}
	return out;
}

/* A string, resolved where it stands: the result is never longer than the source. */
static int rw_json_read_string(rw_json_reader_t* reader, const char** string, size_t* length) {
	char* in = reader->cursor + 1;
	char* out;
	char* start = in;
	/* the common case: nothing to resolve (the picture of a page is megabytes of base64) */
	while (in < reader->end && *in != '"' && *in != '\\') in++;
	out = in;
	while (in < reader->end && *in != '"') {
		if (*in != '\\') {
			*out++ = *in++;
			continue;
		}
		if (in + 1 >= reader->end) return 0;
		in++;
		switch (*in++) {
			case '"': *out++ = '"'; break;
			case '\\': *out++ = '\\'; break;
			case '/': *out++ = '/'; break;
			case 'b': *out++ = '\b'; break;
			case 'f': *out++ = '\f'; break;
			case 'n': *out++ = '\n'; break;
			case 'r': *out++ = '\r'; break;
			case 't': *out++ = '\t'; break;
			case 'u': {
				unsigned code, low;
				if (in + 4 > reader->end || !rw_json_hex(in, &code)) return 0;
				in += 4;
				/* two halves of a character outside the basic plane */
				if (code >= 0xD800 && code < 0xDC00 && in + 6 <= reader->end && in[0] == '\\' && in[1] == 'u' && rw_json_hex(in + 2, &low) && low >= 0xDC00 && low < 0xE000) {
					code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
					in += 6;
				}
				out = rw_json_utf8(out, code);
				break;
			}
			default:
				return 0;
		}
	}
	if (in >= reader->end) return 0;
	*out = 0;
	*string = start;
	*length = (size_t)(out - start);
	reader->cursor = in + 1;
	return 1;
}

static rw_json_t* rw_json_read(rw_json_reader_t* reader, int depth) {
	rw_json_t* value;
	rw_json_t* last = NULL;
	char c;

	rw_json_space(reader);
	if (reader->cursor >= reader->end || depth > RW_JSON_DEPTH) {
		reader->failed = 1;
		return NULL;
	}
	value = rw_json_new(reader);
	if (!value) return NULL;
	c = *reader->cursor;
	if (c == '{' || c == '[') {
		char close = c == '{' ? '}' : ']';
		value->type = c == '{' ? RW_JSON_OBJECT : RW_JSON_ARRAY;
		reader->cursor++;
		rw_json_space(reader);
		if (reader->cursor < reader->end && *reader->cursor == close) {
			reader->cursor++;
			return value;
		}
		for (;;) {
			rw_json_t* child;
			const char* key = NULL;
			size_t key_length = 0;
			if (value->type == RW_JSON_OBJECT) {
				rw_json_space(reader);
				if (reader->cursor >= reader->end || *reader->cursor != '"' || !rw_json_read_string(reader, &key, &key_length)) break;
				rw_json_space(reader);
				if (reader->cursor >= reader->end || *reader->cursor != ':') break;
				reader->cursor++;
			}
			child = rw_json_read(reader, depth + 1);
			if (!child) return NULL;
			child->key = key;
			if (last) last->next = child;
			else value->first = child;
			last = child;
			rw_json_space(reader);
			if (reader->cursor >= reader->end) break;
			if (*reader->cursor == ',') {
				reader->cursor++;
				continue;
			}
			if (*reader->cursor == close) {
				reader->cursor++;
				return value;
			}
			break;
		}
		reader->failed = 1;
		return NULL;
	}
	if (c == '"') {
		value->type = RW_JSON_STRING;
		if (!rw_json_read_string(reader, &value->string, &value->length)) {
			reader->failed = 1;
			return NULL;
		}
		return value;
	}
	if (reader->end - reader->cursor >= 4 && memcmp(reader->cursor, "true", 4) == 0) {
		value->type = RW_JSON_BOOL;
		value->number = 1.0;
		reader->cursor += 4;
		return value;
	}
	if (reader->end - reader->cursor >= 5 && memcmp(reader->cursor, "false", 5) == 0) {
		value->type = RW_JSON_BOOL;
		reader->cursor += 5;
		return value;
	}
	if (reader->end - reader->cursor >= 4 && memcmp(reader->cursor, "null", 4) == 0) {
		reader->cursor += 4;
		return value;
	}
	if (c == '-' || (c >= '0' && c <= '9')) {
		char number[40];
		size_t length = 0;
		while (reader->cursor < reader->end && length + 1 < sizeof(number) &&
			((*reader->cursor >= '0' && *reader->cursor <= '9') || *reader->cursor == '-' || *reader->cursor == '+' || *reader->cursor == '.' || *reader->cursor == 'e' || *reader->cursor == 'E')) {
			number[length++] = *reader->cursor++;
		}
		number[length] = 0;
		value->type = RW_JSON_NUMBER;
		value->number = strtod(number, NULL);
		return value;
	}
	reader->failed = 1;
	return NULL;
}

int rw_json_parse(rw_json_document_t* document, char* text, size_t size) {
	rw_json_reader_t reader;
	memset(document, 0, sizeof(*document));
	reader.document = document;
	reader.cursor = text;
	reader.end = text + size;
	reader.failed = 0;
	document->root = rw_json_read(&reader, 0);
	if (!document->root || reader.failed) {
		rw_json_free(document);
		return 0;
	}
	return 1;
}

void rw_json_free(rw_json_document_t* document) {
	rw_json_block_t* block = (rw_json_block_t*)document->blocks;
	while (block) {
		rw_json_block_t* next = block->next;
		rw_free(block);
		block = next;
	}
	document->blocks = NULL;
	document->root = NULL;
}

const rw_json_t* rw_json_get(const rw_json_t* value, const char* path) {
	while (value && path && *path) {
		const char* dot = strchr(path, '.');
		size_t length = dot ? (size_t)(dot - path) : strlen(path);
		const rw_json_t* child;
		if (value->type != RW_JSON_OBJECT) return NULL;
		for (child = value->first; child; child = child->next) {
			if (child->key && strlen(child->key) == length && memcmp(child->key, path, length) == 0) break;
		}
		value = child;
		path = dot ? dot + 1 : NULL;
	}
	return value;
}

const char* rw_json_string(const rw_json_t* value, const char* path, const char* fallback) {
	const rw_json_t* found = rw_json_get(value, path);
	return found && found->type == RW_JSON_STRING ? found->string : fallback;
}

double rw_json_number(const rw_json_t* value, const char* path, double fallback) {
	const rw_json_t* found = rw_json_get(value, path);
	return found && found->type == RW_JSON_NUMBER ? found->number : fallback;
}

int rw_json_bool(const rw_json_t* value, const char* path, int fallback) {
	const rw_json_t* found = rw_json_get(value, path);
	return found && found->type == RW_JSON_BOOL ? found->number != 0.0 : fallback;
}

void rw_json_write(rw_text_t* text, const rw_json_t* value) {
	const rw_json_t* child;
	if (!value) {
		rw_text_puts(text, "null");
		return;
	}
	switch (value->type) {
		case RW_JSON_NULL:
			rw_text_puts(text, "null");
			break;
		case RW_JSON_BOOL:
			rw_text_puts(text, value->number != 0.0 ? "true" : "false");
			break;
		case RW_JSON_NUMBER:
			if (value->number > -1e15 && value->number < 1e15 && value->number == (double)(long long)value->number) rw_text_printf(text, "%lld", (long long)value->number);
			else rw_text_printf(text, "%.17g", value->number);
			break;
		case RW_JSON_STRING:
			rw_text_json(text, value->string);
			break;
		case RW_JSON_ARRAY:
		case RW_JSON_OBJECT:
			rw_text_puts(text, value->type == RW_JSON_ARRAY ? "[" : "{");
			for (child = value->first; child; child = child->next) {
				if (child != value->first) rw_text_puts(text, ",");
				if (value->type == RW_JSON_OBJECT) {
					rw_text_json(text, child->key);
					rw_text_puts(text, ":");
				}
				rw_json_write(text, child);
			}
			rw_text_puts(text, value->type == RW_JSON_ARRAY ? "]" : "}");
			break;
	}
}

/* ---------------------------------------------------------------------------
 *  Base64
 * ------------------------------------------------------------------------- */

uint8_t* rw_base64_decode(const char* text, size_t length, size_t* size) {
	static int8_t table[256];
	static int ready = 0;
	uint8_t* out;
	size_t written = 0, i;
	uint32_t bits = 0;
	int count = 0;

	if (!ready) {
		const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		int k;
		/* (only the thread that reads the browser decodes) */
		memset(table, -1, sizeof(table));
		for (k = 0; k < 64; k++) table[(unsigned char)alphabet[k]] = (int8_t)k;
		ready = 1;
	}
	out = (uint8_t*)rw_alloc(length / 4 * 3 + 3);
	if (!out) return NULL;
	for (i = 0; i < length; i++) {
		int value = table[(unsigned char)text[i]];
		if (text[i] == '=') break;
		if (value < 0) {
			if (text[i] == '\n' || text[i] == '\r') continue;
			rw_free(out);
			return NULL;
		}
		bits = (bits << 6) | (uint32_t)value;
		if (++count == 4) {
			out[written++] = (uint8_t)(bits >> 16);
			out[written++] = (uint8_t)(bits >> 8);
			out[written++] = (uint8_t)bits;
			bits = 0;
			count = 0;
		}
	}
	if (count == 3) {
		out[written++] = (uint8_t)(bits >> 10);
		out[written++] = (uint8_t)(bits >> 2);
	} else if (count == 2) {
		out[written++] = (uint8_t)(bits >> 4);
	}
	*size = written;
	return out;
}
