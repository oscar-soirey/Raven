/**
 * Raven Script - script/rs_parser.c
 * Recursive descent parser: tokens -> syntax tree. After an error it skips
 * to the next statement and goes on, so that one mistake does not hide the
 * others (and the editor keeps its completion while a line is unfinished).
 */

#include "rs_internal.h"

typedef struct rs_parser {
	rs_batch* batch;
	int unit;
	const char* text;
	rs_lexer lexer;
	rs_token current;
	rs_token previous;
	/** Set after an error until the parser finds its way again (no message meanwhile). */
	int panic;
	/** Loops around the statement being parsed (break / continue are checked by the checker). */
	int depth;
} rs_parser;

/** How deeply expressions and blocks can be nested: a script, which may come from anywhere, cannot overflow the stack. */
#define RS_MAX_NESTING 200

static rs_node* rs_parse_expression(rs_parser* parser);
static rs_node* rs_parse_block(rs_parser* parser);
static rs_node* rs_parse_statement(rs_parser* parser);

/* ---------------------------------------------------------------------------
 *  Tokens
 * ------------------------------------------------------------------------- */

static void rs_error_at(rs_parser* parser, rs_token token, const char* code, const char* title, const char* help, const char* format, ...) {
	va_list arguments;
	rs_buf message = { 0 };
	if (parser->panic) return;
	parser->panic = 1;
	va_start(arguments, format);
	rs_buf_vprintf(&message, format, arguments);
	va_end(arguments);
	rs_report(parser->batch, RS_SEVERITY_ERROR, code, title, parser->unit, token.start, token.end > token.start ? token.end : token.start + 1, help, "%s", message.data ? message.data : "");
	rs_buf_free(&message);
}

static void rs_advance(rs_parser* parser) {
	parser->previous = parser->current;
	for (;;) {
		parser->current = rs_lexer_next(&parser->lexer);
		if (parser->current.kind != TK_ERROR) break;
		/* always reported; the token is skipped, and what follows it is not reported (it is likely a consequence) */
		parser->panic = 0;
		rs_error_at(parser, parser->current, "RS003", "Invalid token.", NULL, "%s.", parser->current.error);
	}
}

static int rs_check_token(const rs_parser* parser, int kind) {
	return parser->current.kind == kind;
}

static int rs_match(rs_parser* parser, int kind) {
	if (!rs_check_token(parser, kind)) return 0;
	rs_advance(parser);
	return 1;
}

static void rs_describe_current(rs_parser* parser, rs_buf* out) {
	if (parser->current.kind == TK_EOF) {
		rs_buf_puts(out, "the end of the script");
	} else {
		uint32_t length = parser->current.end - parser->current.start;
		if (length > 24) length = 24;
		rs_buf_printf(out, "'%.*s'", (int)length, parser->text + parser->current.start);
	}
}

/** Reports "Expected X" where X is missing: just after the previous token when it ends a line, else on the current token. */
static void rs_expected(rs_parser* parser, const char* what, const char* context, const char* help) {
	rs_buf found = { 0 };
	rs_token at = parser->current;
	rs_describe_current(parser, &found);
	if (parser->previous.end > 0) {
		/* a missing ';' at the end of a line is shown where it should be */
		uint32_t i;
		int newline = 0;
		for (i = parser->previous.end; i < parser->current.start && i < parser->lexer.length; i++) {
			if (parser->text[i] == '\n') newline = 1;
		}
		if (newline || parser->current.kind == TK_EOF) {
			at.start = parser->previous.end;
			at.end = parser->previous.end;
		}
	}
	rs_error_at(parser, at, "RS002", "Expected token.", help, "Expected %s%s%s, but found %s.", what, context ? " " : "", context ? context : "", found.data);
	rs_buf_free(&found);
}

static int rs_expect(rs_parser* parser, int kind, const char* context) {
	if (rs_match(parser, kind)) return 1;
	rs_expected(parser, rs_tok_name(kind), context, NULL);
	return 0;
}

/** Skips to a place where a statement can start. */
static void rs_synchronize(rs_parser* parser) {
	parser->panic = 0;
	while (parser->current.kind != TK_EOF) {
		if (parser->previous.kind == TK_SEMICOLON) return;
		switch (parser->current.kind) {
			case TK_FN:
			case TK_LET:
			case TK_CONST:
			case TK_IF:
			case TK_FOR:
			case TK_WHILE:
			case TK_RETURN:
			case TK_BREAK:
			case TK_CONTINUE:
			case TK_DECLARE:
			case TK_RBRACE:
			case TK_LBRACE:
				return;
			default:
				break;
		}
		rs_advance(parser);
	}
}

static rs_node* rs_node_new(rs_parser* parser, int kind, rs_token token) {
	rs_node* node = (rs_node*)rs_arena_alloc(&parser->batch->arena, sizeof(rs_node));
	node->kind = (uint8_t)kind;
	node->unit = parser->unit;
	node->start = token.start;
	node->end = token.end;
	node->slot = -1;
	node->symbol = -1;
	return node;
}

static void rs_node_push(rs_parser* parser, rs_node* parent, rs_node*** list, uint32_t* count, uint32_t* capacity, rs_node* item) {
	RS_UNUSED(parent);
	if (*count >= *capacity) {
		uint32_t new_capacity = *capacity ? *capacity * 2 : 4;
		rs_node** items = (rs_node**)rs_arena_alloc(&parser->batch->arena, new_capacity * sizeof(rs_node*));
		if (*count) memcpy(items, *list, *count * sizeof(rs_node*));
		*list = items;
		*capacity = new_capacity;
	}
	(*list)[(*count)++] = item;
}

static char* rs_token_text(rs_parser* parser, rs_token token) {
	return rs_arena_strndup(&parser->batch->arena, parser->text + token.start, token.end - token.start);
}

static int rs_token_is(const rs_parser* parser, rs_token token, const char* word) {
	size_t length = strlen(word);
	return token.kind == TK_IDENT && token.end - token.start == length && memcmp(parser->text + token.start, word, length) == 0;
}

/* ---------------------------------------------------------------------------
 *  Types
 * ------------------------------------------------------------------------- */

/** type := (number | string | bool | void | null) ('[' ']')* */
static int rs_parse_type(rs_parser* parser, rs_type* type, uint32_t* start, uint32_t* end) {
	rs_token token = parser->current;
	int depth = 0;
	rs_base base;
	if (token.kind == TK_NULL) base = RS_T_NULL;
	else if (rs_token_is(parser, token, "number")) base = RS_T_NUMBER;
	else if (rs_token_is(parser, token, "string")) base = RS_T_STRING;
	else if (rs_token_is(parser, token, "bool")) base = RS_T_BOOL;
	else if (rs_token_is(parser, token, "void")) base = RS_T_VOID;
	else if (rs_token_is(parser, token, "boolean")) {
		rs_error_at(parser, token, "RS102", "Unknown type.", "Help: write 'bool'.", "'boolean' is not a type of Raven Script.");
		base = RS_T_BOOL;
	} else if (token.kind == TK_IDENT) {
		rs_buf name = { 0 };
		rs_buf_put(&name, parser->text + token.start, token.end - token.start);
		rs_error_at(parser, token, "RS102", "Unknown type.", "Help: the types are number, string, bool, void, null and arrays (number[]).", "'%s' is not a type.", name.data);
		rs_buf_free(&name);
		base = RS_T_ERROR;
	} else {
		rs_expected(parser, "a type", NULL, "Help: the types are number, string, bool, void, null and arrays (number[]).");
		return 0;
	}
	rs_advance(parser);
	*start = token.start;
	*end = token.end;
	while (rs_check_token(parser, TK_LBRACKET)) {
		rs_advance(parser);
		if (!rs_expect(parser, TK_RBRACKET, "to close the array type")) return 0;
		*end = parser->previous.end;
		depth++;
	}
	if (depth > RS_MAX_ARRAY_DEPTH) {
		rs_token whole = token;
		whole.end = *end;
		rs_error_at(parser, whole, "RS102", "Unknown type.", NULL, "Arrays are nested %d levels deep at most.", RS_MAX_ARRAY_DEPTH);
		depth = RS_MAX_ARRAY_DEPTH;
	}
	if (depth > 0 && (base == RS_T_VOID || base == RS_T_NULL)) {
		rs_token whole = token;
		whole.end = *end;
		rs_error_at(parser, whole, "RS102", "Unknown type.", NULL, "There are no arrays of %s.", base == RS_T_VOID ? "void" : "null");
		base = RS_T_ERROR;
	}
	*type = rs_type_make(base, depth);
	return 1;
}

/* ---------------------------------------------------------------------------
 *  Expressions
 * ------------------------------------------------------------------------- */

static rs_node* rs_parse_primary(rs_parser* parser) {
	rs_token token = parser->current;
	rs_node* node;
	switch (token.kind) {
		case TK_NUMBER:
			rs_advance(parser);
			node = rs_node_new(parser, RS_N_NUMBER, token);
			rs_parse_number(parser->text + token.start, token.end - token.start, &node->number);
			return node;
		case TK_STRING: {
			rs_buf value = { 0 };
			size_t bad = 0;
			rs_advance(parser);
			node = rs_node_new(parser, RS_N_STRING, token);
			if (!rs_unescape(parser->text + token.start + 1, token.end - token.start - 2, &value, &bad)) {
				rs_token at = token;
				at.start = token.start + 1 + (uint32_t)bad;
				at.end = at.start + 2;
				int panic = parser->panic;
				parser->panic = 0;
				rs_error_at(parser, at, "RS003", "Invalid token.", "Help: the escapes are \\n \\t \\r \\\\ \\\" \\' \\u00E9 and \\u{1F600}.", "This escape sequence is not valid.");
				parser->panic = panic;
			}
			node->text = rs_arena_strndup(&parser->batch->arena, value.data ? value.data : "", value.length);
			node->text_length = (uint32_t)value.length;
			rs_buf_free(&value);
			return node;
		}
		case TK_TRUE:
		case TK_FALSE:
			rs_advance(parser);
			node = rs_node_new(parser, RS_N_BOOL, token);
			node->number = token.kind == TK_TRUE ? 1.0 : 0.0;
			return node;
		case TK_NULL:
			rs_advance(parser);
			return rs_node_new(parser, RS_N_NULL, token);
		case TK_IDENT:
			rs_advance(parser);
			node = rs_node_new(parser, RS_N_NAME, token);
			node->text = rs_token_text(parser, token);
			node->name_start = token.start;
			node->name_end = token.end;
			return node;
		case TK_LPAREN: {
			rs_node* inner;
			rs_advance(parser);
			inner = rs_parse_expression(parser);
			rs_expect(parser, TK_RPAREN, "to close the parenthesis");
			if (inner) {
				inner->start = token.start;
				inner->end = parser->previous.end;
			}
			return inner;
		}
		case TK_LBRACKET: {
			uint32_t capacity = 0;
			rs_advance(parser);
			node = rs_node_new(parser, RS_N_ARRAY, token);
			if (!rs_check_token(parser, TK_RBRACKET)) {
				do {
					rs_node* item;
					if (rs_check_token(parser, TK_RBRACKET)) break; /* a trailing comma */
					item = rs_parse_expression(parser);
					if (!item) return NULL;
					rs_node_push(parser, node, &node->list, &node->count, &capacity, item);
				} while (rs_match(parser, TK_COMMA));
			}
			rs_expect(parser, TK_RBRACKET, "to close the array");
			node->end = parser->previous.end;
			return node;
		}
		default:
			if (token.kind == TK_PLUS_PLUS || token.kind == TK_MINUS_MINUS) {
				rs_error_at(parser, token, "RS001", "Unexpected token.", "Help: write 'i += 1' (or 'i -= 1').", "%s is not an operator of Raven Script.", rs_tok_name(token.kind));
			} else {
				rs_expected(parser, "an expression", NULL, NULL);
			}
			return NULL;
	}
}

static rs_node* rs_parse_postfix(rs_parser* parser) {
	rs_node* node = rs_parse_primary(parser);
	while (node) {
		if (rs_check_token(parser, TK_DOT)) {
			rs_token dot = parser->current;
			rs_node* member;
			rs_advance(parser);
			if (parser->current.kind != TK_IDENT) {
				rs_expected(parser, "a name", "after '.'", NULL);
				/* keep what was read (the editor completes "app.") */
				member = rs_node_new(parser, RS_N_MEMBER, dot);
				member->start = node->start;
				member->a = node;
				member->text = rs_arena_strndup(&parser->batch->arena, "", 0);
				member->name_start = dot.end;
				member->name_end = dot.end;
				return member;
			}
			member = rs_node_new(parser, RS_N_MEMBER, parser->current);
			member->start = node->start;
			member->a = node;
			member->text = rs_token_text(parser, parser->current);
			member->name_start = parser->current.start;
			member->name_end = parser->current.end;
			rs_advance(parser);
			node = member;
		} else if (rs_check_token(parser, TK_LPAREN)) {
			rs_node* call = rs_node_new(parser, RS_N_CALL, parser->current);
			uint32_t capacity = 0;
			call->start = node->start;
			call->a = node;
			/* the parenthesis: signature help */
			call->name_start = parser->current.start;
			rs_advance(parser);
			if (!rs_check_token(parser, TK_RPAREN)) {
				do {
					rs_node* argument = rs_parse_expression(parser);
					if (!argument) {
						call->end = parser->previous.end;
						call->name_end = parser->previous.end;
						return call;
					}
					rs_node_push(parser, call, &call->list, &call->count, &capacity, argument);
				} while (rs_match(parser, TK_COMMA));
			}
			rs_expect(parser, TK_RPAREN, "to close the arguments");
			call->end = parser->previous.end;
			call->name_end = parser->previous.end;
			node = call;
		} else if (rs_check_token(parser, TK_LBRACKET)) {
			rs_node* index = rs_node_new(parser, RS_N_INDEX, parser->current);
			rs_advance(parser);
			index->start = node->start;
			index->a = node;
			index->b = rs_parse_expression(parser);
			if (!index->b) return NULL;
			rs_expect(parser, TK_RBRACKET, "to close the index");
			index->end = parser->previous.end;
			node = index;
		} else {
			break;
		}
	}
	return node;
}

static rs_node* rs_parse_unary(rs_parser* parser) {
	if (rs_check_token(parser, TK_BANG) || rs_check_token(parser, TK_MINUS)) {
		rs_token token = parser->current;
		rs_node* node;
		if (++parser->depth > RS_MAX_NESTING) {
			rs_error_at(parser, token, "RS001", "Unexpected token.", NULL, "This expression is nested too deeply.");
			parser->depth--;
			return NULL;
		}
		rs_advance(parser);
		node = rs_node_new(parser, RS_N_UNARY, token);
		node->op = token.kind;
		node->a = rs_parse_unary(parser);
		parser->depth--;
		if (!node->a) return NULL;
		node->end = node->a->end;
		return node;
	}
	return rs_parse_postfix(parser);
}

static int rs_binary_precedence(int kind) {
	switch (kind) {
		case TK_OR: return 1;
		case TK_AND: return 2;
		case TK_EQ: case TK_NE: case TK_STRICT_EQ: case TK_STRICT_NE: return 3;
		case TK_LT: case TK_LE: case TK_GT: case TK_GE: return 4;
		case TK_PLUS: case TK_MINUS: return 5;
		case TK_STAR: case TK_SLASH: case TK_PERCENT: return 6;
		default: return 0;
	}
}

static rs_node* rs_parse_binary(rs_parser* parser, int min_precedence) {
	rs_node* left;
	if (++parser->depth > RS_MAX_NESTING) {
		rs_error_at(parser, parser->current, "RS001", "Unexpected token.", NULL, "This expression is nested too deeply.");
		parser->depth--;
		return NULL;
	}
	left = rs_parse_unary(parser);
	while (left) {
		rs_token op = parser->current;
		int precedence = rs_binary_precedence(op.kind);
		rs_node* node;
		if (precedence == 0 || precedence < min_precedence) break;
		if (op.kind == TK_STRICT_EQ || op.kind == TK_STRICT_NE) {
			int panic = parser->panic;
			parser->panic = 0;
			rs_error_at(parser, op, "RS001", "Unexpected token.", op.kind == TK_STRICT_EQ ? "Help: write '=='." : "Help: write '!='.",
				"%s does not exist in Raven Script: values of different types are never compared.", rs_tok_name(op.kind));
			parser->panic = panic;
			op.kind = op.kind == TK_STRICT_EQ ? TK_EQ : TK_NE;
		}
		rs_advance(parser);
		node = rs_node_new(parser, RS_N_BINARY, op);
		node->op = op.kind;
		node->a = left;
		node->b = rs_parse_binary(parser, precedence + 1);
		if (!node->b) {
			parser->depth--;
			return NULL;
		}
		node->start = left->start;
		node->end = node->b->end;
		/* the operator: where type errors point */
		node->name_start = op.start;
		node->name_end = op.end;
		left = node;
	}
	parser->depth--;
	return left;
}

static rs_node* rs_parse_expression(rs_parser* parser) {
	return rs_parse_binary(parser, 1);
}

/* ---------------------------------------------------------------------------
 *  Statements
 * ------------------------------------------------------------------------- */

static int rs_is_assignment(int kind) {
	return kind == TK_ASSIGN || kind == TK_PLUS_ASSIGN || kind == TK_MINUS_ASSIGN || kind == TK_STAR_ASSIGN || kind == TK_SLASH_ASSIGN || kind == TK_PERCENT_ASSIGN;
}

/** let / const without the ';' (for loops use it too). */
static rs_node* rs_parse_let(rs_parser* parser) {
	rs_token keyword = parser->current;
	rs_node* node;
	rs_advance(parser);
	node = rs_node_new(parser, RS_N_LET, keyword);
	node->op = keyword.kind == TK_CONST;
	if (parser->current.kind != TK_IDENT) {
		if (parser->current.kind >= TK_FN && parser->current.kind <= TK_DECLARE) {
			rs_error_at(parser, parser->current, "RS001", "Unexpected token.", NULL, "%s is a keyword: it cannot name a variable.", rs_tok_name(parser->current.kind));
		} else {
			rs_expected(parser, "the name of the variable", NULL, NULL);
		}
		return NULL;
	}
	node->text = rs_token_text(parser, parser->current);
	node->name_start = parser->current.start;
	node->name_end = parser->current.end;
	rs_advance(parser);
	if (rs_match(parser, TK_COLON)) {
		if (!rs_parse_type(parser, &node->annotation, &node->annotation_start, &node->annotation_end)) return NULL;
		node->has_annotation = 1;
	}
	if (rs_match(parser, TK_ASSIGN)) {
		node->a = rs_parse_expression(parser);
		if (!node->a) return NULL;
	} else if (rs_check_token(parser, TK_EQ)) {
		rs_error_at(parser, parser->current, "RS001", "Unexpected token.", "Help: write '='.", "'==' compares; '=' gives the value.");
		return NULL;
	}
	node->end = parser->previous.end;
	return node;
}

/** An expression, or an assignment (x = 1, a[i] += 2), without the ';'. */
static rs_node* rs_parse_simple(rs_parser* parser) {
	rs_token first = parser->current;
	rs_node* expression;
	if (rs_token_is(parser, first, "var")) {
		rs_error_at(parser, first, "RS001", "Unexpected token.", "Help: write 'let' (or 'const' for a value that never changes).", "'var' is not a keyword of Raven Script.");
		return NULL;
	}
	expression = rs_parse_expression(parser);
	if (!expression) return NULL;
	if (rs_is_assignment(parser->current.kind)) {
		rs_token op = parser->current;
		rs_node* node = rs_node_new(parser, RS_N_ASSIGN, op);
		rs_advance(parser);
		node->op = op.kind;
		node->a = expression;
		node->b = rs_parse_expression(parser);
		if (!node->b) return NULL;
		node->start = expression->start;
		node->end = node->b->end;
		node->name_start = op.start;
		node->name_end = op.end;
		return node;
	}
	if (rs_check_token(parser, TK_PLUS_PLUS) || rs_check_token(parser, TK_MINUS_MINUS)) {
		rs_error_at(parser, parser->current, "RS001", "Unexpected token.", rs_check_token(parser, TK_PLUS_PLUS) ? "Help: write 'i += 1'." : "Help: write 'i -= 1'.",
			"%s is not an operator of Raven Script.", rs_tok_name(parser->current.kind));
		return NULL;
	}
	{
		rs_node* node = rs_node_new(parser, RS_N_EXPR, first);
		node->a = expression;
		node->start = expression->start;
		node->end = expression->end;
		return node;
	}
}

static rs_node* rs_parse_body(rs_parser* parser, const char* what) {
	if (!rs_check_token(parser, TK_LBRACE)) {
		rs_buf context = { 0 };
		rs_buf_printf(&context, "for the body of %s", what);
		rs_expected(parser, "'{'", context.data, "Help: the bodies of if, else, for and while are blocks: { ... }.");
		rs_buf_free(&context);
		return NULL;
	}
	return rs_parse_block(parser);
}

static rs_node* rs_parse_if(rs_parser* parser) {
	rs_node* node = rs_node_new(parser, RS_N_IF, parser->current);
	rs_advance(parser);
	if (!rs_expect(parser, TK_LPAREN, "after 'if'")) return NULL;
	node->a = rs_parse_expression(parser);
	if (!node->a) return NULL;
	if (!rs_expect(parser, TK_RPAREN, "to close the condition")) return NULL;
	node->b = rs_parse_body(parser, "an if");
	if (!node->b) return NULL;
	if (rs_match(parser, TK_ELSE)) {
		if (rs_check_token(parser, TK_IF)) {
			if (++parser->depth > RS_MAX_NESTING) {
				rs_error_at(parser, parser->current, "RS001", "Unexpected token.", NULL, "Too many 'else if' in a row.");
				return NULL;
			}
			node->c = rs_parse_if(parser);
			parser->depth--;
		} else {
			node->c = rs_parse_body(parser, "an else");
		}
		if (!node->c) return NULL;
	}
	node->end = parser->previous.end;
	return node;
}

static rs_node* rs_parse_statement(rs_parser* parser) {
	rs_token token = parser->current;
	rs_node* node = NULL;
	switch (token.kind) {
		case TK_LET:
		case TK_CONST:
			node = rs_parse_let(parser);
			if (node && !rs_expect(parser, TK_SEMICOLON, "after the declaration")) return NULL;
			return node;
		case TK_IF:
			return rs_parse_if(parser);
		case TK_WHILE:
			rs_advance(parser);
			node = rs_node_new(parser, RS_N_WHILE, token);
			if (!rs_expect(parser, TK_LPAREN, "after 'while'")) return NULL;
			node->a = rs_parse_expression(parser);
			if (!node->a) return NULL;
			if (!rs_expect(parser, TK_RPAREN, "to close the condition")) return NULL;
			node->b = rs_parse_body(parser, "a while");
			if (!node->b) return NULL;
			node->end = parser->previous.end;
			return node;
		case TK_FOR:
			rs_advance(parser);
			node = rs_node_new(parser, RS_N_FOR, token);
			if (!rs_expect(parser, TK_LPAREN, "after 'for'")) return NULL;
			if (!rs_check_token(parser, TK_SEMICOLON)) {
				if (rs_check_token(parser, TK_LET) || rs_check_token(parser, TK_CONST)) node->a = rs_parse_let(parser);
				else node->a = rs_parse_simple(parser);
				if (!node->a) return NULL;
			}
			if (!rs_expect(parser, TK_SEMICOLON, "after the start of the loop")) return NULL;
			if (!rs_check_token(parser, TK_SEMICOLON)) {
				node->b = rs_parse_expression(parser);
				if (!node->b) return NULL;
			}
			if (!rs_expect(parser, TK_SEMICOLON, "after the condition of the loop")) return NULL;
			if (!rs_check_token(parser, TK_RPAREN)) {
				node->c = rs_parse_simple(parser);
				if (!node->c) return NULL;
			}
			if (!rs_expect(parser, TK_RPAREN, "to close the loop")) return NULL;
			node->d = rs_parse_body(parser, "a for");
			if (!node->d) return NULL;
			node->end = parser->previous.end;
			return node;
		case TK_BREAK:
		case TK_CONTINUE:
			rs_advance(parser);
			node = rs_node_new(parser, token.kind == TK_BREAK ? RS_N_BREAK : RS_N_CONTINUE, token);
			if (!rs_expect(parser, TK_SEMICOLON, token.kind == TK_BREAK ? "after 'break'" : "after 'continue'")) return NULL;
			return node;
		case TK_RETURN:
			rs_advance(parser);
			node = rs_node_new(parser, RS_N_RETURN, token);
			if (!rs_check_token(parser, TK_SEMICOLON) && !rs_check_token(parser, TK_RBRACE)) {
				node->a = rs_parse_expression(parser);
				if (!node->a) return NULL;
				node->end = node->a->end;
			}
			if (!rs_expect(parser, TK_SEMICOLON, "after the return")) return NULL;
			return node;
		case TK_LBRACE:
			return rs_parse_block(parser);
		case TK_FN:
			rs_error_at(parser, token, "RS001", "Unexpected token.", "Help: declare the function at the top level of the script.", "Functions cannot be declared inside a function.");
			return NULL;
		case TK_DECLARE:
			rs_error_at(parser, token, "RS001", "Unexpected token.", NULL, "'declare' is only allowed at the top level of the script.");
			return NULL;
		case TK_ELSE:
			rs_error_at(parser, token, "RS001", "Unexpected token.", NULL, "This 'else' has no 'if' before it.");
			return NULL;
		case TK_SEMICOLON:
			/* an empty statement */
			rs_advance(parser);
			return NULL;
		default:
			node = rs_parse_simple(parser);
			if (node && !rs_expect(parser, TK_SEMICOLON, "after the statement")) return NULL;
			return node;
	}
}

static rs_node* rs_parse_block(rs_parser* parser) {
	rs_node* block = rs_node_new(parser, RS_N_BLOCK, parser->current);
	uint32_t capacity = 0;
	if (++parser->depth > RS_MAX_NESTING) {
		rs_error_at(parser, parser->current, "RS001", "Unexpected token.", NULL, "Blocks are nested too deeply.");
		parser->depth--;
		return NULL;
	}
	rs_advance(parser); /* '{' */
	while (!rs_check_token(parser, TK_RBRACE) && !rs_check_token(parser, TK_EOF)) {
		uint32_t before = parser->current.start;
		rs_node* statement = rs_parse_statement(parser);
		if (statement) rs_node_push(parser, block, &block->list, &block->count, &capacity, statement);
		if (parser->panic) rs_synchronize(parser);
		/* always move on: a token no statement can start with is skipped */
		if (parser->current.start == before && !rs_check_token(parser, TK_RBRACE) && !rs_check_token(parser, TK_EOF)) {
			if (!parser->panic) rs_error_at(parser, parser->current, "RS001", "Unexpected token.", NULL, "A statement cannot start with this.");
			rs_advance(parser);
			parser->panic = 0;
		}
		/* a top level declaration inside a block that was never closed: give up the block */
		if (parser->current.kind == TK_FN || parser->current.kind == TK_DECLARE) break;
	}
	parser->depth--;
	if (!rs_check_token(parser, TK_RBRACE)) {
		rs_token open = { 0 };
		open.start = block->start;
		open.end = block->start + 1;
		rs_error_at(parser, open, "RS002", "Expected token.", "Help: add the '}' that closes it.", "This block is never closed.");
		block->end = parser->previous.end;
		return block;
	}
	rs_advance(parser);
	block->end = parser->previous.end;
	return block;
}

/* ---------------------------------------------------------------------------
 *  Top level
 * ------------------------------------------------------------------------- */

/** fn a.b.c(x: number): type { ... }   or   declare fn a.b(x: number): type; */
static rs_node* rs_parse_function(rs_parser* parser, int is_declare, rs_token first) {
	rs_node* node = rs_node_new(parser, RS_N_FN, first);
	rs_buf name = { 0 };
	uint32_t capacity = 0;
	node->is_declare = (uint8_t)is_declare;
	node->type = rs_type_make(RS_T_VOID, 0);
	rs_advance(parser); /* fn */
	if (parser->current.kind != TK_IDENT) {
		rs_expected(parser, "the name of the function", "after 'fn'", NULL);
		return NULL;
	}
	node->name_start = parser->current.start;
	rs_buf_put(&name, parser->text + parser->current.start, parser->current.end - parser->current.start);
	rs_advance(parser);
	while (rs_check_token(parser, TK_DOT)) {
		rs_advance(parser);
		if (parser->current.kind != TK_IDENT) {
			rs_expected(parser, "a name", "after '.' in the name of the function", NULL);
			rs_buf_free(&name);
			return NULL;
		}
		rs_buf_putc(&name, '.');
		rs_buf_put(&name, parser->text + parser->current.start, parser->current.end - parser->current.start);
		rs_advance(parser);
	}
	node->name_end = parser->previous.end;
	node->text = rs_arena_strndup(&parser->batch->arena, name.data, name.length);
	node->text_length = (uint32_t)name.length;
	rs_buf_free(&name);
	if (!rs_check_token(parser, TK_LPAREN)) {
		if (rs_check_token(parser, TK_LBRACE) || rs_check_token(parser, TK_COLON)) {
			rs_token at = parser->previous;
			rs_error_at(parser, at, "RS002", "Expected token.", "Help: write fn name() { ... }.",
				"Parentheses are mandatory, even when the function has no parameters.");
		} else {
			rs_expected(parser, "'('", "after the name of the function", NULL);
		}
		return NULL;
	}
	rs_advance(parser);
	if (!rs_check_token(parser, TK_RPAREN)) {
		do {
			rs_param param;
			memset(&param, 0, sizeof(param));
			if (parser->current.kind != TK_IDENT) {
				rs_expected(parser, "the name of a parameter", NULL, NULL);
				return NULL;
			}
			param.name = rs_token_text(parser, parser->current);
			param.start = parser->current.start;
			param.end = parser->current.end;
			rs_advance(parser);
			if (!rs_check_token(parser, TK_COLON)) {
				rs_expected(parser, "':' and the type of the parameter", NULL, "Help: parameters have a type: fn f(value: number).");
				return NULL;
			}
			rs_advance(parser);
			if (!rs_parse_type(parser, &param.type, &param.type_start, &param.type_end)) return NULL;
			if (node->param_count >= RS_MAX_PARAMS) {
				rs_token at = parser->previous;
				at.start = param.start;
				rs_error_at(parser, at, "RS106", "Invalid argument count.", NULL, "A function has %d parameters at most.", RS_MAX_PARAMS);
				return NULL;
			}
			if (node->param_count >= capacity) {
				uint32_t new_capacity = capacity ? capacity * 2 : 4;
				rs_param* params = (rs_param*)rs_arena_alloc(&parser->batch->arena, new_capacity * sizeof(rs_param));
				if (node->param_count) memcpy(params, node->params, node->param_count * sizeof(rs_param));
				node->params = params;
				capacity = new_capacity;
			}
			node->params[node->param_count++] = param;
		} while (rs_match(parser, TK_COMMA));
	}
	if (!rs_expect(parser, TK_RPAREN, "to close the parameters")) return NULL;
	if (rs_match(parser, TK_COLON)) {
		if (!rs_parse_type(parser, &node->type, &node->annotation_start, &node->annotation_end)) return NULL;
		node->has_annotation = 1;
	}
	if (is_declare) {
		if (rs_check_token(parser, TK_LBRACE)) {
			rs_error_at(parser, parser->current, "RS001", "Unexpected token.", "Help: remove 'declare' to write the function in the script.",
				"A declared function has no body: the application provides it.");
			return NULL;
		}
		if (!rs_expect(parser, TK_SEMICOLON, "after the declaration")) return NULL;
		node->end = parser->previous.end;
		return node;
	}
	if (!rs_check_token(parser, TK_LBRACE)) {
		rs_expected(parser, "'{'", "for the body of the function", NULL);
		return NULL;
	}
	node->a = rs_parse_block(parser);
	node->end = parser->previous.end;
	return node;
}

void rs_parse(rs_batch* batch, int unit, rs_program* program) {
	rs_parser parser;
	const rs_unit* source = &batch->units[unit];
	memset(&parser, 0, sizeof(parser));
	parser.batch = batch;
	parser.unit = unit;
	parser.text = source->text;
	rs_lexer_init(&parser.lexer, source->text, source->length, 0);
	rs_advance(&parser);
	parser.previous.end = 0;
	while (!rs_check_token(&parser, TK_EOF)) {
		rs_token token = parser.current;
		rs_node* item = NULL;
		if (token.kind == TK_FN) {
			item = rs_parse_function(&parser, 0, token);
		} else if (token.kind == TK_DECLARE) {
			rs_advance(&parser);
			if (!rs_check_token(&parser, TK_FN)) {
				rs_expected(&parser, "'fn'", "after 'declare'", NULL);
			} else {
				item = rs_parse_function(&parser, 1, token);
			}
		} else if (token.kind == TK_LET || token.kind == TK_CONST) {
			item = rs_parse_let(&parser);
			if (item && !rs_expect(&parser, TK_SEMICOLON, "after the declaration")) item = NULL;
		} else if (rs_token_is(&parser, token, "function")) {
			rs_error_at(&parser, token, "RS001", "Unexpected token.", "Help: write 'fn'.", "'function' is not a recognized keyword. Use 'fn'.");
			/* read the function anyway: its body is checked */
			parser.panic = 1;
			item = rs_parse_function(&parser, 0, token);
		} else if (token.kind == TK_RBRACE) {
			rs_error_at(&parser, token, "RS001", "Unexpected token.", NULL, "This '}' closes nothing.");
			rs_advance(&parser);
		} else {
			rs_error_at(&parser, token, "RS001", "Unexpected token.", "Help: put the code in a function: fn app.start() { ... }.",
				"Only declarations (fn, declare fn, let, const) are allowed at the top level of a script.");
			rs_advance(&parser);
		}
		if (item) {
			RS_GROW(program->items, program->count, program->capacity);
			program->items[program->count++] = item;
		}
		if (parser.panic) {
			rs_synchronize(&parser);
			/* at the top level, a stray '{' or '}' would stop the loop: skip it */
			if (parser.current.kind == TK_LBRACE || parser.current.kind == TK_RBRACE) {
				if (parser.current.kind == TK_LBRACE) {
					/* skip the whole block */
					int depth = 0;
					do {
						if (parser.current.kind == TK_LBRACE) depth++;
						else if (parser.current.kind == TK_RBRACE) depth--;
						rs_advance(&parser);
					} while (depth > 0 && parser.current.kind != TK_EOF);
				} else {
					rs_advance(&parser);
				}
			}
			while (parser.current.kind != TK_EOF && parser.current.kind != TK_FN && parser.current.kind != TK_DECLARE
				&& parser.current.kind != TK_LET && parser.current.kind != TK_CONST && !rs_token_is(&parser, parser.current, "function")) {
				rs_advance(&parser);
			}
			parser.panic = 0;
		}
	}
}
