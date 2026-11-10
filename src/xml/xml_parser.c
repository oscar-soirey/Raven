/**
 * Raven - xml/xml_parser.c
 * Recursive-descent XML reader.
 */

#include "xml.h"

typedef struct rvn_xml_reader_t {
	rvn_xml_document_t* document;
	const char* cursor;
	const char* end;
	int line;
	int column;
	int failed;
	/* scratch buffer for decoded text and attribute values */
	char* buffer;
	uint32_t buffer_count;
	uint32_t buffer_cap;
} rvn_xml_reader_t;

/* ---------------------------------------------------------------------------
 *  Reading
 * ------------------------------------------------------------------------- */

static void rvn_xml_fail(rvn_xml_reader_t* reader, const char* message) {
	if (reader->failed) return;
	reader->failed = R_TRUE;
	snprintf(reader->document->error, sizeof(reader->document->error), "%s", message);
	reader->document->error_line = reader->line;
	reader->document->error_column = reader->column;
}

static int rvn_xml_eof(const rvn_xml_reader_t* reader) {
	return reader->cursor >= reader->end;
}

static char rvn_xml_peek(const rvn_xml_reader_t* reader) {
	return reader->cursor < reader->end ? *reader->cursor : '\0';
}

static void rvn_xml_advance(rvn_xml_reader_t* reader) {
	if (reader->cursor >= reader->end) return;
	if (*reader->cursor == '\n') {
		reader->line++;
		reader->column = 1;
	} else if (((unsigned char)*reader->cursor & 0xC0u) != 0x80u) {
		/* count characters, not bytes */
		reader->column++;
	}
	reader->cursor++;
}

static int rvn_xml_starts_with(const rvn_xml_reader_t* reader, const char* literal) {
	size_t length = strlen(literal);
	return (size_t)(reader->end - reader->cursor) >= length && memcmp(reader->cursor, literal, length) == 0;
}

static void rvn_xml_skip(rvn_xml_reader_t* reader, size_t count) {
	while (count-- > 0) rvn_xml_advance(reader);
}

static int rvn_xml_is_space(char c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void rvn_xml_skip_space(rvn_xml_reader_t* reader) {
	while (!rvn_xml_eof(reader) && rvn_xml_is_space(*reader->cursor)) rvn_xml_advance(reader);
}

/* Skips to just after `terminator`. Returns R_FALSE when it is missing. */
static int rvn_xml_skip_until(rvn_xml_reader_t* reader, const char* terminator) {
	while (!rvn_xml_eof(reader)) {
		if (rvn_xml_starts_with(reader, terminator)) {
			rvn_xml_skip(reader, strlen(terminator));
			return R_TRUE;
		}
		rvn_xml_advance(reader);
	}
	return R_FALSE;
}

static int rvn_xml_is_name_char(char c) {
	unsigned char u = (unsigned char)c;
	return (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9') ||
		u == '_' || u == '-' || u == '.' || u == ':' || u >= 0x80u;
}

static const char* rvn_xml_read_name(rvn_xml_reader_t* reader) {
	const char* start = reader->cursor;
	while (!rvn_xml_eof(reader) && rvn_xml_is_name_char(*reader->cursor)) rvn_xml_advance(reader);
	if (reader->cursor == start) {
		rvn_xml_fail(reader, "expected a name");
		return "";
	}
	return rvn_arena_strndup(&reader->document->arena, start, (size_t)(reader->cursor - start));
}

/* ---------------------------------------------------------------------------
 *  Scratch buffer
 * ------------------------------------------------------------------------- */

static void rvn_xml_buffer_push(rvn_xml_reader_t* reader, const char* bytes, size_t count) {
	RVN_ARRAY_RESERVE(reader->buffer, reader->buffer_cap, reader->buffer_count + (uint32_t)count + 1);
	memcpy(reader->buffer + reader->buffer_count, bytes, count);
	reader->buffer_count += (uint32_t)count;
}

/* Decodes "&name;" at the cursor into the buffer. */
static void rvn_xml_read_entity(rvn_xml_reader_t* reader) {
	static const struct { const char* name; char value; } named[] = {
		{ "&lt;", '<' }, { "&gt;", '>' }, { "&amp;", '&' }, { "&quot;", '"' }, { "&apos;", '\'' }
	};
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(named); i++) {
		if (rvn_xml_starts_with(reader, named[i].name)) {
			rvn_xml_buffer_push(reader, &named[i].value, 1);
			rvn_xml_skip(reader, strlen(named[i].name));
			return;
		}
	}
	if (rvn_xml_starts_with(reader, "&#")) {
		const char* p = reader->cursor + 2;
		uint32_t codepoint = 0;
		int digits = 0, hex = 0;
		char encoded[4];
		if (p < reader->end && (*p == 'x' || *p == 'X')) {
			hex = 1;
			p++;
		}
		while (p < reader->end && *p != ';' && digits < 8) {
			char c = *p;
			uint32_t digit;
			if (c >= '0' && c <= '9') digit = (uint32_t)(c - '0');
			else if (hex && c >= 'a' && c <= 'f') digit = (uint32_t)(c - 'a' + 10);
			else if (hex && c >= 'A' && c <= 'F') digit = (uint32_t)(c - 'A' + 10);
			else break;
			codepoint = codepoint * (hex ? 16u : 10u) + digit;
			digits++;
			p++;
		}
		if (digits > 0 && p < reader->end && *p == ';' && codepoint > 0 && codepoint <= 0x10FFFFu) {
			rvn_xml_buffer_push(reader, encoded, (size_t)rvn_utf8_encode(codepoint, encoded));
			rvn_xml_skip(reader, (size_t)(p + 1 - reader->cursor));
			return;
		}
	}
	rvn_xml_fail(reader, "unknown entity");
}

/* ---------------------------------------------------------------------------
 *  Grammar
 * ------------------------------------------------------------------------- */

static void rvn_xml_read_attributes(rvn_xml_reader_t* reader, rvn_xml_node_t* node) {
	rvn_xml_attribute_t* last = NULL;
	for (;;) {
		rvn_xml_attribute_t* attribute;
		char quote;

		rvn_xml_skip_space(reader);
		if (rvn_xml_eof(reader) || rvn_xml_peek(reader) == '>' || rvn_xml_peek(reader) == '/' || reader->failed) return;

		attribute = (rvn_xml_attribute_t*)rvn_arena_alloc(&reader->document->arena, sizeof(rvn_xml_attribute_t));
		attribute->next = NULL;
		attribute->value = "";
		attribute->name = rvn_xml_read_name(reader);
		if (reader->failed) return;

		rvn_xml_skip_space(reader);
		if (rvn_xml_peek(reader) != '=') {
			rvn_xml_fail(reader, "expected '=' after the attribute name");
			return;
		}
		rvn_xml_advance(reader);
		rvn_xml_skip_space(reader);
		quote = rvn_xml_peek(reader);
		if (quote != '"' && quote != '\'') {
			rvn_xml_fail(reader, "attribute values must be quoted");
			return;
		}
		rvn_xml_advance(reader);

		reader->buffer_count = 0;
		while (!rvn_xml_eof(reader) && rvn_xml_peek(reader) != quote && !reader->failed) {
			if (rvn_xml_peek(reader) == '&') {
				rvn_xml_read_entity(reader);
			} else if (rvn_xml_peek(reader) == '<') {
				rvn_xml_fail(reader, "'<' is not allowed in an attribute value");
			} else {
				rvn_xml_buffer_push(reader, reader->cursor, 1);
				rvn_xml_advance(reader);
			}
		}
		if (reader->failed) return;
		if (rvn_xml_eof(reader)) {
			rvn_xml_fail(reader, "unterminated attribute value");
			return;
		}
		rvn_xml_advance(reader);
		attribute->value = rvn_arena_strndup(&reader->document->arena, reader->buffer ? reader->buffer : "", reader->buffer_count);

		if (last) last->next = attribute;
		else node->attributes = attribute;
		last = attribute;
	}
}

static char* rvn_xml_trim(rvn_xml_reader_t* reader, const char* text, size_t length) {
	while (length > 0 && rvn_xml_is_space(text[0])) {
		text++;
		length--;
	}
	while (length > 0 && rvn_xml_is_space(text[length - 1])) length--;
	return rvn_arena_strndup(&reader->document->arena, text, length);
}

/* Skips a comment; returns a node that holds it when the document keeps its comments. */
static rvn_xml_node_t* rvn_xml_read_comment(rvn_xml_reader_t* reader, rvn_xml_node_t* parent) {
	rvn_xml_node_t* node;
	const char* start;
	int line = reader->line, column = reader->column;
	rvn_xml_skip(reader, 4);
	start = reader->cursor;
	if (!rvn_xml_skip_until(reader, "-->")) {
		rvn_xml_fail(reader, "unterminated comment");
		return NULL;
	}
	if (!reader->document->keep_comments) return NULL;
	node = (rvn_xml_node_t*)rvn_arena_alloc(&reader->document->arena, sizeof(rvn_xml_node_t));
	memset(node, 0, sizeof(*node));
	node->name = RVN_XML_COMMENT;
	node->text = rvn_arena_strndup(&reader->document->arena, start, (size_t)(reader->cursor - 3 - start));
	node->parent = parent;
	node->line = line;
	node->column = column;
	return node;
}

static rvn_xml_node_t* rvn_xml_read_element(rvn_xml_reader_t* reader, rvn_xml_node_t* parent, int depth) {
	rvn_xml_node_t* node;
	char* text = NULL;
	uint32_t text_count = 0, text_cap = 0;

	if (depth > RVN_XML_MAX_DEPTH) {
		rvn_xml_fail(reader, "elements are nested too deeply");
		return NULL;
	}
	node = (rvn_xml_node_t*)rvn_arena_alloc(&reader->document->arena, sizeof(rvn_xml_node_t));
	memset(node, 0, sizeof(*node));
	node->text = "";
	node->parent = parent;
	node->line = reader->line;
	node->column = reader->column;

	rvn_xml_advance(reader); /* '<' */
	node->name = rvn_xml_read_name(reader);
	rvn_xml_read_attributes(reader, node);
	if (reader->failed) return NULL;

	if (rvn_xml_starts_with(reader, "/>")) {
		rvn_xml_skip(reader, 2);
		return node;
	}
	if (rvn_xml_peek(reader) != '>') {
		rvn_xml_fail(reader, "expected '>' to close the tag");
		return NULL;
	}
	rvn_xml_advance(reader);

	/* content: text, child elements, comments, CDATA, until the closing tag */
	while (!reader->failed) {
		if (rvn_xml_eof(reader)) {
			rvn_xml_fail(reader, "missing closing tag");
			break;
		}
		if (rvn_xml_starts_with(reader, "<!--")) {
			rvn_xml_node_t* comment = rvn_xml_read_comment(reader, node);
			if (comment) {
				if (node->last_child) node->last_child->next = comment;
				else node->first_child = comment;
				node->last_child = comment;
			}
		} else if (rvn_xml_starts_with(reader, "<![CDATA[")) {
			const char* start;
			rvn_xml_skip(reader, 9);
			start = reader->cursor;
			if (!rvn_xml_skip_until(reader, "]]>")) {
				rvn_xml_fail(reader, "unterminated CDATA section");
			} else {
				size_t length = (size_t)(reader->cursor - 3 - start);
				RVN_ARRAY_RESERVE(text, text_cap, text_count + (uint32_t)length + 1);
				memcpy(text + text_count, start, length);
				text_count += (uint32_t)length;
			}
		} else if (rvn_xml_starts_with(reader, "</")) {
			const char* name;
			rvn_xml_skip(reader, 2);
			name = rvn_xml_read_name(reader);
			rvn_xml_skip_space(reader);
			if (!reader->failed && strcmp(name, node->name) != 0) {
				char message[160];
				snprintf(message, sizeof(message), "closing tag </%.40s> does not match <%.40s>", name, node->name);
				rvn_xml_fail(reader, message);
			} else if (!reader->failed && rvn_xml_peek(reader) != '>') {
				rvn_xml_fail(reader, "expected '>' to close the tag");
			} else {
				rvn_xml_advance(reader);
			}
			break;
		} else if (rvn_xml_starts_with(reader, "<?")) {
			if (!rvn_xml_skip_until(reader, "?>")) rvn_xml_fail(reader, "unterminated processing instruction");
		} else if (rvn_xml_peek(reader) == '<') {
			rvn_xml_node_t* child = rvn_xml_read_element(reader, node, depth + 1);
			if (child) {
				if (node->last_child) node->last_child->next = child;
				else node->first_child = child;
				node->last_child = child;
			}
		} else if (rvn_xml_peek(reader) == '&') {
			reader->buffer_count = 0;
			rvn_xml_read_entity(reader);
			if (!reader->failed) {
				RVN_ARRAY_RESERVE(text, text_cap, text_count + reader->buffer_count + 1);
				memcpy(text + text_count, reader->buffer, reader->buffer_count);
				text_count += reader->buffer_count;
			}
		} else {
			RVN_ARRAY_RESERVE(text, text_cap, text_count + 2);
			text[text_count++] = *reader->cursor;
			rvn_xml_advance(reader);
		}
	}
	if (text_count > 0 && !reader->failed) node->text = rvn_xml_trim(reader, text, text_count);
	rvn_free(text);
	return reader->failed ? NULL : node;
}

static int rvn_xml_parse_with(rvn_xml_document_t* document, const char* text, size_t size, int keep_comments);

int rvn_xml_parse(rvn_xml_document_t* document, const char* text, size_t size) {
	return rvn_xml_parse_with(document, text, size, R_FALSE);
}

int rvn_xml_parse_keeping_comments(rvn_xml_document_t* document, const char* text, size_t size) {
	return rvn_xml_parse_with(document, text, size, R_TRUE);
}

static int rvn_xml_parse_with(rvn_xml_document_t* document, const char* text, size_t size, int keep_comments) {
	rvn_xml_reader_t reader;

	memset(document, 0, sizeof(*document));
	document->keep_comments = keep_comments;
	rvn_arena_init(&document->arena, 16384);
	memset(&reader, 0, sizeof(reader));
	reader.document = document;
	reader.cursor = text ? text : "";
	reader.end = reader.cursor + (text ? size : 0);
	reader.line = 1;
	reader.column = 1;

	/* UTF-8 byte order mark */
	if (rvn_xml_starts_with(&reader, "\xEF\xBB\xBF")) reader.cursor += 3;

	/* prolog: declaration, comments, doctype */
	for (;;) {
		rvn_xml_skip_space(&reader);
		if (rvn_xml_starts_with(&reader, "<?")) {
			if (rvn_xml_starts_with(&reader, "<?xml")) document->has_declaration = R_TRUE;
			if (!rvn_xml_skip_until(&reader, "?>")) rvn_xml_fail(&reader, "unterminated XML declaration");
		} else if (rvn_xml_starts_with(&reader, "<!--")) {
			rvn_xml_node_t* comment = rvn_xml_read_comment(&reader, NULL);
			if (comment) {
				if (document->prolog_last) document->prolog_last->next = comment;
				else document->prolog = comment;
				document->prolog_last = comment;
			}
		} else if (rvn_xml_starts_with(&reader, "<!")) {
			if (!rvn_xml_skip_until(&reader, ">")) rvn_xml_fail(&reader, "unterminated declaration");
		} else {
			break;
		}
		if (reader.failed) break;
	}

	if (!reader.failed) {
		if (rvn_xml_peek(&reader) != '<') rvn_xml_fail(&reader, "expected a root element");
		else document->root = rvn_xml_read_element(&reader, NULL, 0);
	}
	if (!reader.failed) {
		/* only comments and white space may follow the root element */
		for (;;) {
			rvn_xml_skip_space(&reader);
			if (rvn_xml_starts_with(&reader, "<!--")) {
				/* (a comment after the root is not kept: nothing would hold it) */
				rvn_xml_skip(&reader, 4);
				if (!rvn_xml_skip_until(&reader, "-->")) rvn_xml_fail(&reader, "unterminated comment");
			} else {
				break;
			}
			if (reader.failed) break;
		}
		if (!reader.failed && !rvn_xml_eof(&reader)) rvn_xml_fail(&reader, "unexpected content after the root element");
	}

	rvn_free(reader.buffer);
	if (reader.failed) {
		document->root = NULL;
		return R_FALSE;
	}
	return R_TRUE;
}

void rvn_xml_free(rvn_xml_document_t* document) {
	rvn_arena_free(&document->arena);
	document->root = NULL;
}

const char* rvn_xml_attribute(const rvn_xml_node_t* node, const char* name) {
	const rvn_xml_attribute_t* attribute;
	for (attribute = node->attributes; attribute; attribute = attribute->next) {
		if (rvn_stricmp(attribute->name, name) == 0) return attribute->value;
	}
	return NULL;
}
