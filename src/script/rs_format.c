/**
 * Raven Script - script/rs_format.c
 * The formatter: one statement per line, blocks indented, one space
 * around binary operators and after commas, the style of the
 * specification (else on its own line). Comments are kept where they are;
 * a statement the author split on several lines stays split (indented one
 * level more); at most one blank line is kept between statements. A text
 * with a syntax error is not formatted (NULL), so nothing is lost.
 */

#include "rs_internal.h"

typedef struct rs_formatter {
	const char* text;
	rs_buf out;
	const char* indent;
	const char* prefix;
	int level;
	/* parentheses and brackets open (a ';' inside does not end the line) */
	int nesting;
	int at_line_start;
	int has_output;
} rs_formatter;

static void rs_format_newline(rs_formatter* f, int blank) {
	if (!f->has_output) return;
	rs_buf_putc(&f->out, '\n');
	if (blank) rs_buf_putc(&f->out, '\n');
	f->at_line_start = 1;
}

static void rs_format_indent(rs_formatter* f, int extra) {
	int i;
	if (f->prefix) rs_buf_puts(&f->out, f->prefix);
	for (i = 0; i < f->level + extra; i++) rs_buf_puts(&f->out, f->indent);
	f->at_line_start = 0;
}

static int rs_newlines_between(const char* text, uint32_t from, uint32_t to) {
	int count = 0;
	uint32_t i;
	for (i = from; i < to; i++) {
		if (text[i] == '\n') count++;
	}
	return count;
}

/* A token after which a '-' is binary (it ends an operand). */
static int rs_ends_operand(int kind) {
	return kind == TK_IDENT || kind == TK_NUMBER || kind == TK_STRING || kind == TK_TRUE || kind == TK_FALSE || kind == TK_NULL || kind == TK_RPAREN || kind == TK_RBRACKET;
}

static int rs_space_between(int previous, int previous_unary, int current) {
	if (previous == TK_EOF) return 0;
	switch (current) {
		case TK_RPAREN:
		case TK_RBRACKET:
		case TK_COMMA:
		case TK_SEMICOLON:
		case TK_DOT:
		case TK_COLON:
			return 0;
		default:
			break;
	}
	switch (previous) {
		case TK_LPAREN:
		case TK_LBRACKET:
		case TK_DOT:
			return 0;
		case TK_BANG:
			return 0;
		case TK_MINUS:
			if (previous_unary) return 0;
			break;
		default:
			break;
	}
	if (current == TK_LPAREN) {
		/* a call or the parameters of a function: f(x); a keyword: if (x) */
		if (previous == TK_IDENT || previous == TK_RPAREN || previous == TK_RBRACKET) return 0;
		return 1;
	}
	if (current == TK_LBRACKET) {
		/* an index or an array type: a[0], number[]; an array literal: = [1] */
		if (previous == TK_IDENT || previous == TK_RPAREN || previous == TK_RBRACKET || previous == TK_STRING || previous == TK_NULL) return 0;
		return 1;
	}
	return 1;
}

char* rs_format(const char* text, size_t length, const rs_format_options* options) {
	rs_formatter f;
	rs_lexer lexer;
	rs_token token, previous;
	int previous_unary = 0;
	int continuation = 0;
	/* syntax errors: nothing is formatted */
	{
		rs_env* env = rs_env_create(NULL);
		rs_batch* batch = rs_batch_create(env, 0);
		rs_program program;
		int errors;
		memset(&program, 0, sizeof(program));
		rs_batch_add(batch, "<format>", text, length, 1, 1);
		rs_parse(batch, 0, &program);
		errors = batch->errors;
		rs_free(program.items);
		rs_batch_destroy(batch);
		rs_env_destroy(env);
		if (errors > 0) return NULL;
	}
	memset(&f, 0, sizeof(f));
	f.text = text;
	f.indent = options && options->indent ? options->indent : "    ";
	f.prefix = options && options->prefix && options->prefix[0] ? options->prefix : NULL;
	f.at_line_start = 1;
	rs_lexer_init(&lexer, text, length, 1);
	memset(&previous, 0, sizeof(previous));
	previous.kind = TK_EOF;
	for (;;) {
		int newlines, unary = 0, statement_end;
		token = rs_lexer_next(&lexer);
		if (token.kind == TK_EOF) break;
		if (token.kind == TK_ERROR) {
			rs_buf_free(&f.out);
			return NULL;
		}
		newlines = previous.kind == TK_EOF ? 0 : rs_newlines_between(text, previous.end, token.start);
		if (token.kind == TK_MINUS && !rs_ends_operand(previous.kind)) unary = 1;
		if (token.kind == TK_BANG) unary = 1;

		/* where the previous token ends a line */
		statement_end = previous.kind == TK_LBRACE || previous.kind == TK_RBRACE || (previous.kind == TK_SEMICOLON && f.nesting == 0)
			|| (previous.kind == TK_COMMENT && (text[previous.start + 1] == '/' || newlines > 0));
		if (token.kind == TK_RBRACE) {
			if (f.level > 0) f.level--;
			if (previous.kind == TK_LBRACE) {
				/* {} */
				rs_buf_putc(&f.out, '}');
				previous = token;
				previous_unary = 0;
				continuation = 0;
				continue;
			}
			rs_format_newline(&f, 0);
			rs_format_indent(&f, 0);
			continuation = 0;
		} else if (token.kind == TK_COMMENT && newlines == 0 && previous.kind != TK_EOF) {
			/* a comment at the end of a line stays there */
			rs_buf_putc(&f.out, ' ');
		} else if (statement_end && previous.kind != TK_EOF) {
			/* one blank line at most, none at the start of a block */
			rs_format_newline(&f, newlines >= 2 && previous.kind != TK_LBRACE);
			rs_format_indent(&f, 0);
			continuation = 0;
		} else if (newlines > 0 && previous.kind != TK_EOF) {
			/* the author split the statement: keep it split, one level deeper */
			rs_format_newline(&f, 0);
			if (token.kind == TK_RPAREN || token.kind == TK_RBRACKET) rs_format_indent(&f, f.nesting > 0 ? f.nesting - 1 + (continuation ? 0 : 0) : 0);
			else rs_format_indent(&f, f.nesting > 0 ? f.nesting : 1);
			continuation = 1;
		} else if (f.at_line_start) {
			rs_format_indent(&f, 0);
		} else if (rs_space_between(previous.kind, previous_unary, token.kind)) {
			rs_buf_putc(&f.out, ' ');
		}
		rs_buf_put(&f.out, text + token.start, token.end - token.start);
		f.has_output = 1;
		switch (token.kind) {
			case TK_LBRACE: f.level++; break;
			case TK_LPAREN: case TK_LBRACKET: f.nesting++; break;
			case TK_RPAREN: case TK_RBRACKET: if (f.nesting > 0) f.nesting--; break;
			default: break;
		}
		previous = token;
		previous_unary = unary;
	}
	if (f.has_output) rs_buf_putc(&f.out, '\n');
	return rs_buf_take(&f.out);
}
