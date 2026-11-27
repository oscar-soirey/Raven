/**
 * Raven Script - script/rs_lexer.c
 * Tokens: identifiers, keywords, numbers, strings, operators, comments.
 */

#include "rs_internal.h"

static const struct { const char* text; int kind; } g_keywords[] = {
	{ "fn", TK_FN },
	{ "let", TK_LET },
	{ "const", TK_CONST },
	{ "if", TK_IF },
	{ "else", TK_ELSE },
	{ "for", TK_FOR },
	{ "while", TK_WHILE },
	{ "break", TK_BREAK },
	{ "continue", TK_CONTINUE },
	{ "return", TK_RETURN },
	{ "true", TK_TRUE },
	{ "false", TK_FALSE },
	{ "null", TK_NULL },
	{ "declare", TK_DECLARE }
};

int rs_keyword(const char* text, size_t length) {
	size_t i;
	for (i = 0; i < RS_COUNT_OF(g_keywords); i++) {
		if (strlen(g_keywords[i].text) == length && memcmp(g_keywords[i].text, text, length) == 0) return g_keywords[i].kind;
	}
	return TK_IDENT;
}

int rs_is_ident_start(int c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

int rs_is_ident_char(int c) {
	return rs_is_ident_start(c) || (c >= '0' && c <= '9');
}

const char* rs_tok_name(int kind) {
	switch (kind) {
		case TK_EOF: return "the end of the script";
		case TK_ERROR: return "an invalid character";
		case TK_IDENT: return "an identifier";
		case TK_NUMBER: return "a number";
		case TK_STRING: return "a string";
		case TK_COMMENT: return "a comment";
		case TK_FN: return "'fn'";
		case TK_LET: return "'let'";
		case TK_CONST: return "'const'";
		case TK_IF: return "'if'";
		case TK_ELSE: return "'else'";
		case TK_FOR: return "'for'";
		case TK_WHILE: return "'while'";
		case TK_BREAK: return "'break'";
		case TK_CONTINUE: return "'continue'";
		case TK_RETURN: return "'return'";
		case TK_TRUE: return "'true'";
		case TK_FALSE: return "'false'";
		case TK_NULL: return "'null'";
		case TK_DECLARE: return "'declare'";
		case TK_LPAREN: return "'('";
		case TK_RPAREN: return "')'";
		case TK_LBRACE: return "'{'";
		case TK_RBRACE: return "'}'";
		case TK_LBRACKET: return "'['";
		case TK_RBRACKET: return "']'";
		case TK_COMMA: return "','";
		case TK_DOT: return "'.'";
		case TK_SEMICOLON: return "';'";
		case TK_COLON: return "':'";
		case TK_PLUS: return "'+'";
		case TK_MINUS: return "'-'";
		case TK_STAR: return "'*'";
		case TK_SLASH: return "'/'";
		case TK_PERCENT: return "'%'";
		case TK_BANG: return "'!'";
		case TK_ASSIGN: return "'='";
		case TK_PLUS_ASSIGN: return "'+='";
		case TK_MINUS_ASSIGN: return "'-='";
		case TK_STAR_ASSIGN: return "'*='";
		case TK_SLASH_ASSIGN: return "'/='";
		case TK_PERCENT_ASSIGN: return "'%='";
		case TK_EQ: return "'=='";
		case TK_NE: return "'!='";
		case TK_LT: return "'<'";
		case TK_LE: return "'<='";
		case TK_GT: return "'>'";
		case TK_GE: return "'>='";
		case TK_AND: return "'&&'";
		case TK_OR: return "'||'";
		case TK_PLUS_PLUS: return "'++'";
		case TK_MINUS_MINUS: return "'--'";
		case TK_STRICT_EQ: return "'==='";
		case TK_STRICT_NE: return "'!=='";
		default: return "a token";
	}
}

void rs_lexer_init(rs_lexer* lexer, const char* text, size_t length, int keep_comments) {
	lexer->text = text ? text : "";
	lexer->length = (uint32_t)(text ? length : 0);
	lexer->cursor = 0;
	lexer->keep_comments = keep_comments;
}

static int rs_peek(const rs_lexer* lexer, uint32_t ahead) {
	uint32_t at = lexer->cursor + ahead;
	return at < lexer->length ? (unsigned char)lexer->text[at] : 0;
}

static rs_token rs_make(int kind, uint32_t start, uint32_t end) {
	rs_token token;
	token.kind = (uint8_t)kind;
	token.start = start;
	token.end = end;
	token.error = NULL;
	return token;
}

static rs_token rs_error_token(uint32_t start, uint32_t end, const char* message) {
	rs_token token = rs_make(TK_ERROR, start, end);
	token.error = message;
	return token;
}

#define RS_LEXER_ZERO_BYTE "a 0 byte cannot be part of a script"

rs_token rs_lexer_next(rs_lexer* lexer) {
	for (;;) {
		uint32_t start;
		int c;
		/* white space */
		while (lexer->cursor < lexer->length) {
			c = (unsigned char)lexer->text[lexer->cursor];
			if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') lexer->cursor++;
			/* a byte order mark (a file saved by an editor) */
			else if (c == 0xEF && rs_peek(lexer, 1) == 0xBB && rs_peek(lexer, 2) == 0xBF) lexer->cursor += 3;
			else break;
		}
		start = lexer->cursor;
		if (lexer->cursor >= lexer->length) return rs_make(TK_EOF, start, start);
		c = (unsigned char)lexer->text[lexer->cursor];

		/* comments (a 0 byte is in none: a script is also read as a C string, which would end there) */
		if (c == '/' && rs_peek(lexer, 1) == '/') {
			int zero = 0;
			while (lexer->cursor < lexer->length && lexer->text[lexer->cursor] != '\n') {
				if (lexer->text[lexer->cursor] == '\0') zero = 1;
				lexer->cursor++;
			}
			if (zero) return rs_error_token(start, lexer->cursor, RS_LEXER_ZERO_BYTE);
			if (lexer->keep_comments) {
				uint32_t end = lexer->cursor;
				/* (every carriage return before the line feed: "//\r\r" kept one, which the formatter then
				 * wrote back and the next formatting took away) */
				while (end > start && lexer->text[end - 1] == '\r') end--;
				return rs_make(TK_COMMENT, start, end);
			}
			continue;
		}
		if (c == '/' && rs_peek(lexer, 1) == '*') {
			int zero = 0;
			lexer->cursor += 2;
			for (;;) {
				if (lexer->cursor + 1 >= lexer->length) {
					lexer->cursor = lexer->length;
					return rs_error_token(start, start + 2, "this comment is never closed: '*/' is missing");
				}
				if (lexer->text[lexer->cursor] == '*' && lexer->text[lexer->cursor + 1] == '/') {
					lexer->cursor += 2;
					break;
				}
				if (lexer->text[lexer->cursor] == '\0') zero = 1;
				lexer->cursor++;
			}
			if (zero) return rs_error_token(start, lexer->cursor, RS_LEXER_ZERO_BYTE);
			if (lexer->keep_comments) return rs_make(TK_COMMENT, start, lexer->cursor);
			continue;
		}

		/* identifiers and keywords */
		if (rs_is_ident_start(c)) {
			while (lexer->cursor < lexer->length && rs_is_ident_char((unsigned char)lexer->text[lexer->cursor])) lexer->cursor++;
			return rs_make(rs_keyword(lexer->text + start, lexer->cursor - start), start, lexer->cursor);
		}

		/* numbers: 42, 3.14, 1e-3, 0x1F (a '.' is part of the number only when a digit follows it) */
		if (c >= '0' && c <= '9') {
			if (c == '0' && (rs_peek(lexer, 1) == 'x' || rs_peek(lexer, 1) == 'X')) {
				lexer->cursor += 2;
				while (lexer->cursor < lexer->length && rs_is_ident_char((unsigned char)lexer->text[lexer->cursor])) lexer->cursor++;
			} else {
				while (lexer->cursor < lexer->length && lexer->text[lexer->cursor] >= '0' && lexer->text[lexer->cursor] <= '9') lexer->cursor++;
				if (rs_peek(lexer, 0) == '.' && rs_peek(lexer, 1) >= '0' && rs_peek(lexer, 1) <= '9') {
					lexer->cursor++;
					while (lexer->cursor < lexer->length && lexer->text[lexer->cursor] >= '0' && lexer->text[lexer->cursor] <= '9') lexer->cursor++;
				}
				if (rs_peek(lexer, 0) == 'e' || rs_peek(lexer, 0) == 'E') {
					uint32_t ahead = 1;
					if (rs_peek(lexer, 1) == '+' || rs_peek(lexer, 1) == '-') ahead = 2;
					if (rs_peek(lexer, ahead) >= '0' && rs_peek(lexer, ahead) <= '9') {
						lexer->cursor += ahead;
						while (lexer->cursor < lexer->length && lexer->text[lexer->cursor] >= '0' && lexer->text[lexer->cursor] <= '9') lexer->cursor++;
					}
				}
				/* letters glued to a number: 12abc */
				while (lexer->cursor < lexer->length && rs_is_ident_char((unsigned char)lexer->text[lexer->cursor])) lexer->cursor++;
			}
			{
				double value;
				if (!rs_parse_number(lexer->text + start, lexer->cursor - start, &value)) return rs_error_token(start, lexer->cursor, "this is not a valid number");
			}
			return rs_make(TK_NUMBER, start, lexer->cursor);
		}

		/* strings: "..." or '...' on one line */
		if (c == '"' || c == '\'') {
			int quote = c, zero = 0;
			lexer->cursor++;
			for (;;) {
				int d;
				if (lexer->cursor >= lexer->length || lexer->text[lexer->cursor] == '\n') {
					return rs_error_token(start, lexer->cursor, "this string is never closed: the quote is missing at the end of the line");
				}
				d = (unsigned char)lexer->text[lexer->cursor];
				if (d == 0 || (d == '\\' && rs_peek(lexer, 1) == 0)) zero = 1;
				if (d == '\\') {
					lexer->cursor += 2;
					if (lexer->cursor > lexer->length) lexer->cursor = lexer->length;
					continue;
				}
				lexer->cursor++;
				if (d == quote) break;
			}
			if (zero) return rs_error_token(start, lexer->cursor, RS_LEXER_ZERO_BYTE);
			return rs_make(TK_STRING, start, lexer->cursor);
		}

		lexer->cursor++;
		switch (c) {
			case '(': return rs_make(TK_LPAREN, start, lexer->cursor);
			case ')': return rs_make(TK_RPAREN, start, lexer->cursor);
			case '{': return rs_make(TK_LBRACE, start, lexer->cursor);
			case '}': return rs_make(TK_RBRACE, start, lexer->cursor);
			case '[': return rs_make(TK_LBRACKET, start, lexer->cursor);
			case ']': return rs_make(TK_RBRACKET, start, lexer->cursor);
			case ',': return rs_make(TK_COMMA, start, lexer->cursor);
			case '.': return rs_make(TK_DOT, start, lexer->cursor);
			case ';': return rs_make(TK_SEMICOLON, start, lexer->cursor);
			case ':': return rs_make(TK_COLON, start, lexer->cursor);
			case '+':
				if (rs_peek(lexer, 0) == '=') { lexer->cursor++; return rs_make(TK_PLUS_ASSIGN, start, lexer->cursor); }
				if (rs_peek(lexer, 0) == '+') { lexer->cursor++; return rs_make(TK_PLUS_PLUS, start, lexer->cursor); }
				return rs_make(TK_PLUS, start, lexer->cursor);
			case '-':
				if (rs_peek(lexer, 0) == '=') { lexer->cursor++; return rs_make(TK_MINUS_ASSIGN, start, lexer->cursor); }
				if (rs_peek(lexer, 0) == '-') { lexer->cursor++; return rs_make(TK_MINUS_MINUS, start, lexer->cursor); }
				return rs_make(TK_MINUS, start, lexer->cursor);
			case '*':
				if (rs_peek(lexer, 0) == '=') { lexer->cursor++; return rs_make(TK_STAR_ASSIGN, start, lexer->cursor); }
				return rs_make(TK_STAR, start, lexer->cursor);
			case '/':
				if (rs_peek(lexer, 0) == '=') { lexer->cursor++; return rs_make(TK_SLASH_ASSIGN, start, lexer->cursor); }
				return rs_make(TK_SLASH, start, lexer->cursor);
			case '%':
				if (rs_peek(lexer, 0) == '=') { lexer->cursor++; return rs_make(TK_PERCENT_ASSIGN, start, lexer->cursor); }
				return rs_make(TK_PERCENT, start, lexer->cursor);
			case '!':
				if (rs_peek(lexer, 0) == '=') {
					lexer->cursor++;
					if (rs_peek(lexer, 0) == '=') { lexer->cursor++; return rs_make(TK_STRICT_NE, start, lexer->cursor); }
					return rs_make(TK_NE, start, lexer->cursor);
				}
				return rs_make(TK_BANG, start, lexer->cursor);
			case '=':
				if (rs_peek(lexer, 0) == '=') {
					lexer->cursor++;
					if (rs_peek(lexer, 0) == '=') { lexer->cursor++; return rs_make(TK_STRICT_EQ, start, lexer->cursor); }
					return rs_make(TK_EQ, start, lexer->cursor);
				}
				return rs_make(TK_ASSIGN, start, lexer->cursor);
			case '<':
				if (rs_peek(lexer, 0) == '=') { lexer->cursor++; return rs_make(TK_LE, start, lexer->cursor); }
				return rs_make(TK_LT, start, lexer->cursor);
			case '>':
				if (rs_peek(lexer, 0) == '=') { lexer->cursor++; return rs_make(TK_GE, start, lexer->cursor); }
				return rs_make(TK_GT, start, lexer->cursor);
			case '&':
				if (rs_peek(lexer, 0) == '&') { lexer->cursor++; return rs_make(TK_AND, start, lexer->cursor); }
				return rs_error_token(start, lexer->cursor, "'&' is not an operator: write '&&' for a logical AND");
			case '|':
				if (rs_peek(lexer, 0) == '|') { lexer->cursor++; return rs_make(TK_OR, start, lexer->cursor); }
				return rs_error_token(start, lexer->cursor, "'|' is not an operator: write '||' for a logical OR");
			default:
				break;
		}
		/* a whole UTF-8 character */
		while (lexer->cursor < lexer->length && ((unsigned char)lexer->text[lexer->cursor] & 0xC0u) == 0x80u) lexer->cursor++;
		return rs_error_token(start, lexer->cursor, c == '`' ? "template strings are not supported: join texts with '+'" : "this character is not part of the language");
	}
}

static int rs_hex_digit(int c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

static void rs_put_utf8(rs_buf* out, uint32_t code) {
	char bytes[4];
	if (code < 0x80) {
		bytes[0] = (char)code;
		rs_buf_put(out, bytes, 1);
	} else if (code < 0x800) {
		bytes[0] = (char)(0xC0 | (code >> 6));
		bytes[1] = (char)(0x80 | (code & 0x3F));
		rs_buf_put(out, bytes, 2);
	} else if (code < 0x10000) {
		bytes[0] = (char)(0xE0 | (code >> 12));
		bytes[1] = (char)(0x80 | ((code >> 6) & 0x3F));
		bytes[2] = (char)(0x80 | (code & 0x3F));
		rs_buf_put(out, bytes, 3);
	} else {
		bytes[0] = (char)(0xF0 | (code >> 18));
		bytes[1] = (char)(0x80 | ((code >> 12) & 0x3F));
		bytes[2] = (char)(0x80 | ((code >> 6) & 0x3F));
		bytes[3] = (char)(0x80 | (code & 0x3F));
		rs_buf_put(out, bytes, 4);
	}
}

int rs_unescape(const char* text, size_t length, rs_buf* out, size_t* bad) {
	size_t i = 0;
	rs_buf_put(out, "", 0);
	while (i < length) {
		char c = text[i];
		if (c != '\\') {
			rs_buf_putc(out, c);
			i++;
			continue;
		}
		if (i + 1 >= length) {
			if (bad) *bad = i;
			return 0;
		}
		c = text[i + 1];
		switch (c) {
			case 'n': rs_buf_putc(out, '\n'); i += 2; break;
			case 't': rs_buf_putc(out, '\t'); i += 2; break;
			case 'r': rs_buf_putc(out, '\r'); i += 2; break;
			case '0': rs_buf_put(out, "", 0); i += 2; break; /* a NUL would cut the text: dropped */
			case '\\': rs_buf_putc(out, '\\'); i += 2; break;
			case '"': rs_buf_putc(out, '"'); i += 2; break;
			case '\'': rs_buf_putc(out, '\''); i += 2; break;
			case 'u': {
				uint32_t code = 0;
				size_t j, escape = i;
				if (i + 2 < length && text[i + 2] == '{') {
					/* \u{1F600} */
					size_t digits = 0;
					for (j = i + 3; j < length && text[j] != '}'; j++) {
						int digit = rs_hex_digit((unsigned char)text[j]);
						if (digit < 0 || ++digits > 6) {
							if (bad) *bad = i;
							return 0;
						}
						code = code * 16 + (uint32_t)digit;
					}
					if (j >= length || digits == 0) {
						if (bad) *bad = i;
						return 0;
					}
					i = j + 1;
				} else {
					/* é */
					if (i + 6 > length) {
						if (bad) *bad = i;
						return 0;
					}
					for (j = i + 2; j < i + 6; j++) {
						int digit = rs_hex_digit((unsigned char)text[j]);
						if (digit < 0) {
							if (bad) *bad = i;
							return 0;
						}
						code = code * 16 + (uint32_t)digit;
					}
					i += 6;
				}
				if (code == 0 || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) {
					if (bad) *bad = escape;
					return 0;
				}
				rs_put_utf8(out, code);
				break;
			}
			default:
				if (bad) *bad = i;
				return 0;
		}
	}
	return 1;
}
