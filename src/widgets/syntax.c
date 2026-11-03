/**
 * Raven - widgets/syntax.c
 * Languages of the text editor: what the editor knows about the text it
 * holds, to color it, to complete words and to indent.
 *
 * A language is a description (RLanguageDesc): its words, how its comments
 * and its texts are written, a few traits. Three lexers read these
 * descriptions:
 *
 *   code     C, C++, C#, Java, JavaScript, Python, Rust, JSON, GLSL... and
 *            any language declared by the application
 *   markup   XML and HTML (which holds JavaScript and CSS between its tags)
 *   styles   CSS
 *
 * A lexer reads one line and cuts it into spans, each with a kind of token.
 * What a line starts in (the middle of a comment, of a tag...) is one byte
 * handed from each line to the next, so a line is colored without reading
 * the lines before it again.
 */

#include "common.h"

#define RVN_LANGUAGE_MAX_MULTILINE 8

struct rvn_language_t {
	/** The description given to Rlanguage_register; its strings are owned by `strings`. */
	RLanguageDesc desc;
	rvn_arena_t strings;
	/** keywords, types, constants: sorted, for searching. */
	const char** words[3];
	uint32_t word_count[3];
	/** Texts that span lines: what opens each kind, and what closes it. */
	const char* multiline[RVN_LANGUAGE_MAX_MULTILINE];
	const char* multiline_close[RVN_LANGUAGE_MAX_MULTILINE];
	int multiline_count;
};

static struct {
	rvn_language_t** items;
	uint32_t count;
	uint32_t cap;
	/** Languages that were replaced: editors may still point to them until they look again. */
	rvn_language_t** retired;
	uint32_t retired_count;
	uint32_t retired_cap;
	uint32_t generation;
	int built_in;
} g_languages;

typedef struct rvn_lexer_t {
	const char* text;
	int length;
	rvn_span_t* spans;
	int max;
	int count;
} rvn_lexer_t;

static void rvn_lex_emit(rvn_lexer_t* lexer, int start, int end, rvn_token_t token) {
	if (end <= start || !lexer->spans || lexer->count >= lexer->max) return;
	lexer->spans[lexer->count].start = start;
	lexer->spans[lexer->count].end = end;
	lexer->spans[lexer->count].token = (uint8_t)token;
	lexer->count++;
}

static int rvn_lex_starts(const rvn_lexer_t* lexer, int at, int to, const char* word) {
	int length = (int)strlen(word);
	return length > 0 && at + length <= to && memcmp(lexer->text + at, word, (size_t)length) == 0;
}

/* First position of `word` in [at, to), or -1. */
static int rvn_lex_find(const rvn_lexer_t* lexer, int at, int to, const char* word) {
	int length = (int)strlen(word);
	if (length == 0) return -1;
	for (; at + length <= to; at++) {
		if (memcmp(lexer->text + at, word, (size_t)length) == 0) return at;
	}
	return -1;
}

static int rvn_lex_find_nocase(const rvn_lexer_t* lexer, int at, int to, const char* word) {
	int length = (int)strlen(word);
	for (; at + length <= to; at++) {
		if (rvn_strnicmp(lexer->text + at, word, (size_t)length) == 0) return at;
	}
	return -1;
}

static int rvn_lex_is_space(char c) {
	return c == ' ' || c == '\t';
}

static int rvn_lex_is_word(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '$' || (unsigned char)c >= 0x80;
}

static int rvn_lex_is_digit(char c) {
	return c >= '0' && c <= '9';
}

/* ---------------------------------------------------------------------------
 *  Words
 * ------------------------------------------------------------------------- */

static int rvn_language_compare(const void* a, const void* b) {
	return strcmp(*(const char* const*)a, *(const char* const*)b);
}

static int rvn_language_compare_nocase(const void* a, const void* b) {
	return rvn_stricmp(*(const char* const*)a, *(const char* const*)b);
}

/* Is text[0..length) one of the words of a list? */
static int rvn_language_has_word(const rvn_language_t* language, int list, const char* text, int length) {
	int ignore_case = (language->desc.flags & R_LANGUAGE_IGNORE_CASE) != 0;
	uint32_t low = 0, high = language->word_count[list];
	while (low < high) {
		uint32_t middle = (low + high) / 2;
		const char* word = language->words[list][middle];
		int order = ignore_case ? rvn_strnicmp(text, word, (size_t)length) : strncmp(text, word, (size_t)length);
		/* (equal over `length` characters: the word of the list may still be longer) */
		if (order == 0 && word[length] != '\0') order = -1;
		if (order == 0) return R_TRUE;
		if (order < 0) high = middle;
		else low = middle + 1;
	}
	return R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Code
 *
 *  State: 0 in code, 1..15 inside a block comment (the depth, for the
 *  languages whose comments nest), 16 + k inside the k-th kind of text that
 *  spans lines.
 * ------------------------------------------------------------------------- */

#define RVN_CODE_MULTILINE 16

/* End of a quoted text that starts at `at` (on the quote): the position after its closing quote, or `to`. */
static int rvn_lex_string_end(const rvn_lexer_t* lexer, int at, int to, char quote) {
	int end = at + 1;
	while (end < to && lexer->text[end] != quote) {
		if (lexer->text[end] == '\\' && end + 1 < to) end++;
		end++;
	}
	return end < to ? end + 1 : to;
}

/*
 * End of the k-th kind of text that spans lines, looked for from `at`: the
 * position after what closes it, or -1 when it does not end on this line.
 * A text closed by what opened it is an ordinary one, where a backslash
 * protects the next character; the others are raw.
 */
static int rvn_lex_multiline_end(const rvn_lexer_t* lexer, const rvn_language_t* language, int k, int at, int to) {
	const char* close = language->multiline_close[k];
	int length = (int)strlen(close);
	if (close != language->multiline[k]) {
		int end = rvn_lex_find(lexer, at, to, close);
		return end < 0 ? -1 : end + length;
	}
	while (at < to) {
		if (lexer->text[at] == '\\') at += 2;
		else if (rvn_lex_starts(lexer, at, to, close)) return at + length;
		else at++;
	}
	return -1;
}

static uint8_t rvn_lex_code(rvn_lexer_t* lexer, const rvn_language_t* language, uint8_t state, int from, int to) {
	const RLanguageDesc* desc = &language->desc;
	const char* text = lexer->text;
	const char* quotes = desc->string_quotes ? desc->string_quotes : "";
	int has_block = desc->block_comment_start && desc->block_comment_start[0] && desc->block_comment_end && desc->block_comment_end[0];
	int nested = (desc->flags & R_LANGUAGE_NESTED_COMMENTS) != 0;
	/* "--[[" opens a block where "--" would start a line comment: the longer mark is looked for first */
	int line_first = desc->line_comment && (!has_block || strlen(desc->line_comment) > strlen(desc->block_comment_start));
	int i = from, end, k;
	int first = R_TRUE;

	while (i < to) {
		char c = text[i];

		if (state >= 1 && state < RVN_CODE_MULTILINE) {
			/* inside a block comment: to its end, counting the comments opened inside it when they nest */
			int start = i;
			while (i < to && state >= 1) {
				if (rvn_lex_starts(lexer, i, to, desc->block_comment_end)) {
					i += (int)strlen(desc->block_comment_end);
					state--;
				} else if (nested && state < RVN_CODE_MULTILINE - 1 && rvn_lex_starts(lexer, i, to, desc->block_comment_start)) {
					i += (int)strlen(desc->block_comment_start);
					state++;
				} else {
					i++;
				}
			}
			rvn_lex_emit(lexer, start, i, RVN_TOKEN_COMMENT);
			continue;
		}
		if (state >= RVN_CODE_MULTILINE) {
			end = state - RVN_CODE_MULTILINE < language->multiline_count
				? rvn_lex_multiline_end(lexer, language, state - RVN_CODE_MULTILINE, i, to) : i;
			if (end < 0) {
				rvn_lex_emit(lexer, i, to, RVN_TOKEN_STRING);
				return state;
			}
			rvn_lex_emit(lexer, i, end, RVN_TOKEN_STRING);
			i = end;
			state = 0;
			continue;
		}
		if (rvn_lex_is_space(c)) {
			i++;
			continue;
		}
		if (line_first && rvn_lex_starts(lexer, i, to, desc->line_comment)) {
			rvn_lex_emit(lexer, i, to, RVN_TOKEN_COMMENT);
			return state;
		}
		if (has_block && rvn_lex_starts(lexer, i, to, desc->block_comment_start)) {
			/* (the opening is read again as the start of the comment, so that it gets its color) */
			int open = (int)strlen(desc->block_comment_start);
			int start = i;
			state = 1;
			i += open;
			while (i < to && state >= 1) {
				if (rvn_lex_starts(lexer, i, to, desc->block_comment_end)) {
					i += (int)strlen(desc->block_comment_end);
					state--;
				} else if (nested && state < RVN_CODE_MULTILINE - 1 && rvn_lex_starts(lexer, i, to, desc->block_comment_start)) {
					i += open;
					state++;
				} else {
					i++;
				}
			}
			rvn_lex_emit(lexer, start, i, RVN_TOKEN_COMMENT);
			first = R_FALSE;
			continue;
		}
		if (!line_first && desc->line_comment && rvn_lex_starts(lexer, i, to, desc->line_comment)) {
			rvn_lex_emit(lexer, i, to, RVN_TOKEN_COMMENT);
			return state;
		}
		if (first && desc->directive && rvn_lex_starts(lexer, i, to, desc->directive)) {
			/* a line for the preprocessor: #include <file>, #pragma once */
			end = i + (int)strlen(desc->directive);
			while (end < to && rvn_lex_is_space(text[end])) end++;
			while (end < to && rvn_lex_is_word(text[end])) end++;
			rvn_lex_emit(lexer, i, end, RVN_TOKEN_PREPROCESSOR);
			while (end < to && rvn_lex_is_space(text[end])) end++;
			if (end < to && text[end] == '<') {
				int close = end;
				while (close < to && text[close] != '>') close++;
				if (close < to) {
					rvn_lex_emit(lexer, end, close + 1, RVN_TOKEN_STRING);
					end = close + 1;
				}
			}
			i = end;
			first = R_FALSE;
			continue;
		}
		first = R_FALSE;
		if (desc->annotation && rvn_lex_starts(lexer, i, to, desc->annotation)) {
			size_t length = strlen(desc->annotation);
			end = i + (int)length;
			if (desc->annotation[length - 1] == '[') {
				/* #[derive(Debug)]: to the bracket that closes it */
				int depth = 1;
				while (end < to && depth > 0) {
					if (text[end] == '[') depth++;
					else if (text[end] == ']') depth--;
					end++;
				}
				rvn_lex_emit(lexer, i, end, RVN_TOKEN_PREPROCESSOR);
				i = end;
				continue;
			}
			if (end < to && rvn_lex_is_word(text[end]) && !rvn_lex_is_digit(text[end])) {
				/* @Override, @app.route */
				while (end < to && (rvn_lex_is_word(text[end]) || text[end] == '.')) end++;
				rvn_lex_emit(lexer, i, end, RVN_TOKEN_PREPROCESSOR);
				i = end;
				continue;
			}
		}
		for (k = 0; k < language->multiline_count; k++) {
			if (rvn_lex_starts(lexer, i, to, language->multiline[k])) break;
		}
		if (k < language->multiline_count) {
			end = rvn_lex_multiline_end(lexer, language, k, i + (int)strlen(language->multiline[k]), to);
			if (end < 0) {
				rvn_lex_emit(lexer, i, to, RVN_TOKEN_STRING);
				return (uint8_t)(RVN_CODE_MULTILINE + k);
			}
			rvn_lex_emit(lexer, i, end, RVN_TOKEN_STRING);
			i = end;
			continue;
		}
		if (c != '\0' && strchr(quotes, c)) {
			if (c == '\'' && (desc->flags & R_LANGUAGE_CHAR_LITERALS)) {
				/* one character between the quotes, or it is not a text: a lifetime, an apostrophe */
				int close = i + 1;
				if (close < to && text[close] == '\\') {
					close += 2;
					while (close < to && close - i < 12 && text[close] != '\'') close++;
				} else if (close < to) {
					close = rvn_utf8_next(text, to, close);
				}
				if (close < to && text[close] == '\'' && close > i + 1) {
					rvn_lex_emit(lexer, i, close + 1, RVN_TOKEN_STRING);
					i = close + 1;
				} else {
					i++;
				}
				continue;
			}
			end = rvn_lex_string_end(lexer, i, to, c);
			rvn_lex_emit(lexer, i, end, RVN_TOKEN_STRING);
			i = end;
			continue;
		}
		if (rvn_lex_is_digit(c) || (c == '.' && i + 1 < to && rvn_lex_is_digit(text[i + 1]))) {
			end = i + 1;
			while (end < to && (rvn_lex_is_word(text[end]) || text[end] == '.' ||
				((text[end] == '+' || text[end] == '-') && (text[end - 1] == 'e' || text[end - 1] == 'E') &&
					!(text[i] == '0' && i + 1 < to && (text[i + 1] == 'x' || text[i + 1] == 'X'))))) end++;
			rvn_lex_emit(lexer, i, end, RVN_TOKEN_NUMBER);
			i = end;
			continue;
		}
		if (rvn_lex_is_word(c)) {
			int after;
			end = i;
			while (end < to && rvn_lex_is_word(text[end])) end++;
			if (rvn_language_has_word(language, 0, text + i, end - i)) {
				rvn_lex_emit(lexer, i, end, RVN_TOKEN_KEYWORD);
			} else if (rvn_language_has_word(language, 1, text + i, end - i)) {
				rvn_lex_emit(lexer, i, end, RVN_TOKEN_TYPE);
			} else if (rvn_language_has_word(language, 2, text + i, end - i)) {
				rvn_lex_emit(lexer, i, end, RVN_TOKEN_NUMBER);
			} else {
				/* a name followed by a parenthesis is the name of a function (println! in Rust) */
				after = end;
				if (after < to && text[after] == '!') after++;
				while (after < to && rvn_lex_is_space(text[after])) after++;
				if (after < to && text[after] == '(') rvn_lex_emit(lexer, i, end, RVN_TOKEN_FUNCTION);
			}
			i = end;
			continue;
		}
		i++;
	}
	return state;
}

/* ---------------------------------------------------------------------------
 *  Style sheets (CSS)
 *
 *  State: bits 0..2 = how many braces are open, bit 3 = inside a comment.
 * ------------------------------------------------------------------------- */

#define RVN_CSS_COMMENT 8

static int rvn_css_is_name(char c) {
	return rvn_lex_is_word(c) || c == '-';
}

static uint8_t rvn_lex_css(rvn_lexer_t* lexer, const rvn_language_t* language, uint8_t state, int from, int to) {
	const char* text = lexer->text;
	int depth = state & 7, i = from, end;
	int in_value = R_FALSE;

	if (state & RVN_CSS_COMMENT) {
		end = rvn_lex_find(lexer, i, to, "*/");
		if (end < 0) {
			rvn_lex_emit(lexer, i, to, RVN_TOKEN_COMMENT);
			return state;
		}
		rvn_lex_emit(lexer, i, end + 2, RVN_TOKEN_COMMENT);
		i = end + 2;
	}
	while (i < to) {
		char c = text[i];
		/* inside braces, what comes before a ':' is a property, unless a '{' comes first (a nested rule) */
		int selector = depth == 0;
		if (!selector && !in_value) {
			int look = i;
			while (look < to && text[look] != ';' && text[look] != '{' && text[look] != '}') look++;
			if (look < to && text[look] == '{') selector = R_TRUE;
		}
		if (rvn_lex_is_space(c)) {
			i++;
		} else if (rvn_lex_starts(lexer, i, to, "/*")) {
			end = rvn_lex_find(lexer, i + 2, to, "*/");
			if (end < 0) {
				rvn_lex_emit(lexer, i, to, RVN_TOKEN_COMMENT);
				return (uint8_t)(depth | RVN_CSS_COMMENT);
			}
			rvn_lex_emit(lexer, i, end + 2, RVN_TOKEN_COMMENT);
			i = end + 2;
		} else if (c == '"' || c == '\'') {
			end = rvn_lex_string_end(lexer, i, to, c);
			rvn_lex_emit(lexer, i, end, RVN_TOKEN_STRING);
			i = end;
		} else if (c == '{') {
			if (depth < 7) depth++;
			in_value = R_FALSE;
			i++;
		} else if (c == '}') {
			if (depth > 0) depth--;
			in_value = R_FALSE;
			i++;
		} else if (c == ';') {
			in_value = R_FALSE;
			i++;
		} else if (c == '@') {
			end = i + 1;
			while (end < to && rvn_css_is_name(text[end])) end++;
			rvn_lex_emit(lexer, i, end, RVN_TOKEN_PREPROCESSOR);
			i = end;
		} else if (selector) {
			if (c == '.' || c == '#') {
				end = i + 1;
				while (end < to && rvn_css_is_name(text[end])) end++;
				rvn_lex_emit(lexer, i, end, RVN_TOKEN_ATTRIBUTE);
				i = end;
			} else if (c == ':') {
				end = i + 1;
				while (end < to && (rvn_css_is_name(text[end]) || text[end] == ':')) end++;
				rvn_lex_emit(lexer, i, end, RVN_TOKEN_KEYWORD);
				i = end;
			} else if (rvn_css_is_name(c)) {
				end = i;
				while (end < to && rvn_css_is_name(text[end])) end++;
				rvn_lex_emit(lexer, i, end, RVN_TOKEN_TAG);
				i = end;
			} else {
				i++;
			}
		} else if (!in_value) {
			if (rvn_css_is_name(c)) {
				end = i;
				while (end < to && rvn_css_is_name(text[end])) end++;
				rvn_lex_emit(lexer, i, end, RVN_TOKEN_ATTRIBUTE);
				i = end;
			} else {
				if (c == ':') in_value = R_TRUE;
				i++;
			}
		} else if (c == '#' || rvn_lex_is_digit(c) || ((c == '.' || c == '-') && i + 1 < to && rvn_lex_is_digit(text[i + 1]))) {
			/* a color, a number with its unit */
			end = i + 1;
			while (end < to && (rvn_lex_is_word(text[end]) || text[end] == '.' || text[end] == '%')) end++;
			rvn_lex_emit(lexer, i, end, RVN_TOKEN_NUMBER);
			i = end;
		} else if (c == '!') {
			end = i + 1;
			while (end < to && rvn_css_is_name(text[end])) end++;
			rvn_lex_emit(lexer, i, end, RVN_TOKEN_KEYWORD);
			i = end;
		} else if (rvn_css_is_name(c)) {
			end = i;
			while (end < to && rvn_css_is_name(text[end])) end++;
			if (end < to && text[end] == '(') rvn_lex_emit(lexer, i, end, RVN_TOKEN_FUNCTION);
			else if (rvn_language_has_word(language, 0, text + i, end - i)) rvn_lex_emit(lexer, i, end, RVN_TOKEN_KEYWORD);
			i = end;
		} else {
			i++;
		}
	}
	return (uint8_t)depth;
}

/* ---------------------------------------------------------------------------
 *  Markup (XML, HTML)
 *
 *  State: bits 0..2 = where in the markup, bits 3..4 = the tag being read
 *  opens JavaScript (1) or CSS (2); or 0x40 | state of the code lexer inside
 *  <script>, or 0x80 | state of the style lexer inside <style>.
 * ------------------------------------------------------------------------- */

enum {
	RVN_XML_CONTENT = 0,
	RVN_XML_COMMENT,
	RVN_XML_TAG,
	RVN_XML_DOUBLE_QUOTED,
	RVN_XML_SINGLE_QUOTED,
	RVN_XML_CDATA,
	RVN_XML_DECLARATION
};

#define RVN_XML_SCRIPT 0x40
#define RVN_XML_STYLE 0x80

static int rvn_xml_is_name(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
		c == '_' || c == '-' || c == '.' || c == ':' || (unsigned char)c >= 0x80;
}

static uint8_t rvn_lex_markup(rvn_lexer_t* lexer, const rvn_language_t* language, uint8_t full_state) {
	const char* text = lexer->text;
	int length = lexer->length, i = 0;
	int embeds = (language->desc.flags & R_LANGUAGE_EMBEDS) != 0;
	int state = full_state & 7, pending = (full_state >> 3) & 3;

	/* inside <script> or <style>: another language, until the closing tag */
	if (full_state & (RVN_XML_SCRIPT | RVN_XML_STYLE)) {
		int script = (full_state & RVN_XML_SCRIPT) != 0;
		const rvn_language_t* inner = rvn_language_find(script ? (language->desc.script_language ? language->desc.script_language : "javascript") : "css");
		int close = rvn_lex_find_nocase(lexer, 0, length, script ? "</script" : "</style");
		uint8_t sub = (uint8_t)(full_state & 0x3F);
		int to = close < 0 ? length : close;
		/* (a code lexer for the code: a language of markup there would not know what to do with it) */
		if (inner && script && (inner->desc.flags & (R_LANGUAGE_MARKUP | R_LANGUAGE_STYLE_SHEET | R_LANGUAGE_FORMATTED_TEXT))) inner = NULL;
		if (inner) sub = script ? rvn_lex_code(lexer, inner, sub, 0, to) : rvn_lex_css(lexer, inner, sub, 0, to);
		if (close < 0) return (uint8_t)((script ? RVN_XML_SCRIPT : RVN_XML_STYLE) | (sub & 0x3F));
		i = close;
		state = RVN_XML_CONTENT;
		pending = 0;
	}

	while (i < length) {
		int end;
		switch (state) {
			case RVN_XML_COMMENT:
				end = rvn_lex_find(lexer, i, length, "-->");
				if (end < 0) {
					rvn_lex_emit(lexer, i, length, RVN_TOKEN_COMMENT);
					return (uint8_t)state;
				}
				rvn_lex_emit(lexer, i, end + 3, RVN_TOKEN_COMMENT);
				i = end + 3;
				state = RVN_XML_CONTENT;
				break;
			case RVN_XML_CDATA:
				end = rvn_lex_find(lexer, i, length, "]]>");
				if (end < 0) return (uint8_t)state;
				rvn_lex_emit(lexer, end, end + 3, RVN_TOKEN_PUNCTUATION);
				i = end + 3;
				state = RVN_XML_CONTENT;
				break;
			case RVN_XML_DECLARATION:
				end = rvn_lex_find(lexer, i, length, ">");
				if (end < 0) {
					rvn_lex_emit(lexer, i, length, RVN_TOKEN_PREPROCESSOR);
					return (uint8_t)state;
				}
				rvn_lex_emit(lexer, i, end + 1, RVN_TOKEN_PREPROCESSOR);
				i = end + 1;
				state = RVN_XML_CONTENT;
				break;
			case RVN_XML_DOUBLE_QUOTED:
			case RVN_XML_SINGLE_QUOTED:
				end = rvn_lex_find(lexer, i, length, state == RVN_XML_DOUBLE_QUOTED ? "\"" : "'");
				if (end < 0) {
					rvn_lex_emit(lexer, i, length, RVN_TOKEN_STRING);
					return (uint8_t)(state | (pending << 3));
				}
				rvn_lex_emit(lexer, i, end + 1, RVN_TOKEN_STRING);
				i = end + 1;
				state = RVN_XML_TAG;
				break;
			case RVN_XML_TAG:
				if (rvn_lex_is_space(text[i])) {
					i++;
				} else if (text[i] == '>') {
					rvn_lex_emit(lexer, i, i + 1, RVN_TOKEN_PUNCTUATION);
					i++;
					if (pending) {
						/* the rest of the line, and the lines after it, are in the other language */
						uint8_t rest = (uint8_t)(pending == 1 ? RVN_XML_SCRIPT : RVN_XML_STYLE);
						rvn_lexer_t tail = *lexer;
						uint8_t out;
						tail.text = text + i;
						tail.length = length - i;
						tail.spans = lexer->spans ? lexer->spans + lexer->count : NULL;
						tail.max = lexer->max - lexer->count;
						tail.count = 0;
						out = rvn_lex_markup(&tail, language, rest);
						/* (the spans of the tail are counted from where the tail starts) */
						for (end = 0; end < tail.count; end++) {
							lexer->spans[lexer->count + end].start += i;
							lexer->spans[lexer->count + end].end += i;
						}
						lexer->count += tail.count;
						return out;
					}
					state = RVN_XML_CONTENT;
				} else if (rvn_lex_starts(lexer, i, length, "/>")) {
					rvn_lex_emit(lexer, i, i + 2, RVN_TOKEN_PUNCTUATION);
					i += 2;
					state = RVN_XML_CONTENT;
					pending = 0;
				} else if (text[i] == '"' || text[i] == '\'') {
					int close = i + 1;
					char quote = text[i];
					while (close < length && text[close] != quote) close++;
					if (close < length) {
						rvn_lex_emit(lexer, i, close + 1, RVN_TOKEN_STRING);
						i = close + 1;
					} else {
						rvn_lex_emit(lexer, i, length, RVN_TOKEN_STRING);
						return (uint8_t)((quote == '"' ? RVN_XML_DOUBLE_QUOTED : RVN_XML_SINGLE_QUOTED) | (pending << 3));
					}
				} else if (text[i] == '=') {
					rvn_lex_emit(lexer, i, i + 1, RVN_TOKEN_PUNCTUATION);
					i++;
				} else if (rvn_xml_is_name(text[i])) {
					end = i;
					while (end < length && rvn_xml_is_name(text[end])) end++;
					rvn_lex_emit(lexer, i, end, RVN_TOKEN_ATTRIBUTE);
					i = end;
				} else {
					i++;
				}
				break;
			case RVN_XML_CONTENT:
			default:
				if (rvn_lex_starts(lexer, i, length, "<!--")) {
					state = RVN_XML_COMMENT;
				} else if (rvn_lex_starts(lexer, i, length, "<![CDATA[")) {
					rvn_lex_emit(lexer, i, i + 9, RVN_TOKEN_PUNCTUATION);
					i += 9;
					state = RVN_XML_CDATA;
				} else if (rvn_lex_starts(lexer, i, length, "<?") || rvn_lex_starts(lexer, i, length, "<!")) {
					state = RVN_XML_DECLARATION;
				} else if (text[i] == '<') {
					int name = i + 1;
					int closing = name < length && text[name] == '/';
					if (closing) name++;
					rvn_lex_emit(lexer, i, name, RVN_TOKEN_PUNCTUATION);
					end = name;
					while (end < length && rvn_xml_is_name(text[end])) end++;
					rvn_lex_emit(lexer, name, end, RVN_TOKEN_TAG);
					pending = 0;
					if (embeds && !closing) {
						if (end - name == 6 && rvn_strnicmp(text + name, "script", 6) == 0) pending = 1;
						else if (end - name == 5 && rvn_strnicmp(text + name, "style", 5) == 0) pending = 2;
					}
					i = end;
					state = RVN_XML_TAG;
				} else if (text[i] == '&') {
					/* an entity: &amp; &#x20AC; */
					end = i + 1;
					while (end < length && end - i < 12 && text[end] != ';' && !rvn_lex_is_space(text[end]) && text[end] != '<') end++;
					if (end < length && text[end] == ';') {
						rvn_lex_emit(lexer, i, end + 1, RVN_TOKEN_NUMBER);
						i = end + 1;
					} else {
						i++;
					}
				} else {
					i++;
				}
				break;
		}
	}
	return (uint8_t)(state | (state == RVN_XML_TAG ? (pending << 3) : 0));
}

/* ---------------------------------------------------------------------------
 *  Entry points of the editor
 * ------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 *  Formatted text (Markdown), as it is written
 *
 *  State of a line: 0, or RVN_MD_IN_CODE between two fences, with the mark
 *  of the fence that opened (so that ~~~ inside ``` does not close it).
 * ------------------------------------------------------------------------- */

#define RVN_MD_IN_CODE 1u
#define RVN_MD_TILDE   2u

/* Where the mark that closes what opened before `from` is, or -1. */
static int rvn_lex_md_closer(const char* text, int from, int length, const char* mark, int mark_length) {
	int i;
	for (i = from; i + mark_length <= length; i++) {
		if (text[i] == '\\' && i + 1 < length) {
			i++;
			continue;
		}
		if (memcmp(text + i, mark, (size_t)mark_length) == 0 && i > from && text[i - 1] != ' ') return i;
	}
	return -1;
}

static uint8_t rvn_lex_markdown(rvn_lexer_t* lexer, uint8_t state) {
	const char* text = lexer->text;
	int length = lexer->length, i = 0, run;

	while (i < length && (text[i] == ' ' || text[i] == '\t')) i++;

	if (state & RVN_MD_IN_CODE) {
		char mark = (state & RVN_MD_TILDE) ? '~' : '`';
		run = 0;
		while (i + run < length && text[i + run] == mark) run++;
		if (run >= 3 && i + run == length) {
			rvn_lex_emit(lexer, i, length, RVN_TOKEN_PUNCTUATION);
			return 0;
		}
		rvn_lex_emit(lexer, 0, length, RVN_TOKEN_STRING);
		return state;
	}

	/* quotes */
	while (i < length && text[i] == '>') {
		rvn_lex_emit(lexer, i, i + 1, RVN_TOKEN_PUNCTUATION);
		i++;
		while (i < length && text[i] == ' ') i++;
	}
	if (i >= length) return 0;

	/* a fence */
	if (text[i] == '`' || text[i] == '~') {
		run = 0;
		while (i + run < length && text[i + run] == text[i]) run++;
		if (run >= 3 && (text[i] == '~' || !memchr(text + i + run, '`', (size_t)(length - i - run)))) {
			rvn_lex_emit(lexer, i, i + run, RVN_TOKEN_PUNCTUATION);
			rvn_lex_emit(lexer, i + run, length, RVN_TOKEN_KEYWORD);
			return (uint8_t)(RVN_MD_IN_CODE | (text[i] == '~' ? RVN_MD_TILDE : 0u));
		}
	}
	/* a title */
	if (text[i] == '#') {
		run = 0;
		while (i + run < length && text[i + run] == '#') run++;
		if (run <= 6 && (i + run == length || text[i + run] == ' ')) {
			rvn_lex_emit(lexer, i, length, RVN_TOKEN_HEADING);
			return 0;
		}
	}
	/* a rule, or the line under a title */
	{
		int k, marks = 0, other = 0;
		for (k = i; k < length; k++) {
			if (text[k] == '-' || text[k] == '*' || text[k] == '_' || text[k] == '=') marks += text[k] == text[i];
			else if (text[k] != ' ') other++;
		}
		if (!other && marks >= 3) {
			rvn_lex_emit(lexer, i, length, RVN_TOKEN_PUNCTUATION);
			return 0;
		}
	}
	/* the mark of a list item, and the box of a task */
	if ((text[i] == '-' || text[i] == '*' || text[i] == '+') && i + 1 < length && text[i + 1] == ' ') {
		rvn_lex_emit(lexer, i, i + 1, RVN_TOKEN_KEYWORD);
		i += 2;
		if (i + 2 < length && text[i] == '[' && text[i + 2] == ']' && (text[i + 1] == ' ' || text[i + 1] == 'x' || text[i + 1] == 'X')) {
			rvn_lex_emit(lexer, i, i + 3, RVN_TOKEN_KEYWORD);
			i += 3;
		}
	} else if (text[i] >= '0' && text[i] <= '9') {
		int k = i;
		while (k < length && text[k] >= '0' && text[k] <= '9') k++;
		if (k + 1 < length && (text[k] == '.' || text[k] == ')') && text[k + 1] == ' ') {
			rvn_lex_emit(lexer, i, k + 1, RVN_TOKEN_KEYWORD);
			i = k + 2;
		}
	}

	while (i < length) {
		char c = text[i];
		int end;
		if (c == '\\' && i + 1 < length) {
			rvn_lex_emit(lexer, i, i + 1, RVN_TOKEN_PUNCTUATION);
			i += 2;
			continue;
		}
		if (c == '`') {
			run = 0;
			while (i + run < length && text[i + run] == '`') run++;
			end = i + run;
			while (end < length) {
				int close = 0;
				while (end + close < length && text[end + close] == '`') close++;
				if (close == run) break;
				end += close ? close : 1;
			}
			if (end < length) {
				rvn_lex_emit(lexer, i, end + run, RVN_TOKEN_STRING);
				i = end + run;
				continue;
			}
			i += run;
			continue;
		}
		if ((c == '*' || c == '_') && i + 1 < length) {
			int strong = text[i + 1] == c;
			int mark_length = strong ? 2 : 1;
			char mark[2];
			int inside_word = c == '_' && i > 0 && ((text[i - 1] >= 'a' && text[i - 1] <= 'z') || (text[i - 1] >= 'A' && text[i - 1] <= 'Z') || (text[i - 1] >= '0' && text[i - 1] <= '9'));
			mark[0] = mark[1] = c;
			if (!inside_word && i + mark_length < length && text[i + mark_length] != ' ') {
				end = rvn_lex_md_closer(text, i + mark_length, length, mark, mark_length);
				if (end >= 0) {
					/* ("***both***": the third star belongs to what is inside) */
					int stop = end + mark_length;
					while (stop < length && text[stop] == c && stop - end < 3) stop++;
					rvn_lex_emit(lexer, i, stop, strong ? RVN_TOKEN_STRONG : RVN_TOKEN_EMPHASIS);
					i = stop;
					continue;
				}
			}
			i += mark_length;
			continue;
		}
		if (c == '~' && i + 1 < length && text[i + 1] == '~') {
			end = rvn_lex_md_closer(text, i + 2, length, "~~", 2);
			if (end >= 0) {
				rvn_lex_emit(lexer, i, end + 2, RVN_TOKEN_COMMENT);
				i = end + 2;
				continue;
			}
			i += 2;
			continue;
		}
		if (c == '[' || (c == '!' && i + 1 < length && text[i + 1] == '[')) {
			int close = i + 1, nest = 0;
			while (close < length) {
				if (text[close] == '[') nest++;
				else if (text[close] == ']' && --nest <= 0) break;
				close++;
			}
			if (close + 1 < length && text[close + 1] == '(') {
				end = close + 2;
				while (end < length && text[end] != ')') end++;
				if (end < length) {
					rvn_lex_emit(lexer, i, end + 1, RVN_TOKEN_LINK);
					i = end + 1;
					continue;
				}
			}
		}
		if (c == '<') {
			if (rvn_lex_starts(lexer, i, length, "<!--")) {
				end = rvn_lex_find(lexer, i + 4, length, "-->");
				end = end < 0 ? length : end + 3;
				rvn_lex_emit(lexer, i, end, RVN_TOKEN_COMMENT);
				i = end;
				continue;
			}
			end = i + 1;
			while (end < length && text[end] != '>' && text[end] != ' ') end++;
			if (end < length && text[end] == '>' && end > i + 1) {
				int address = memchr(text + i, ':', (size_t)(end - i)) != NULL || memchr(text + i, '@', (size_t)(end - i)) != NULL;
				rvn_lex_emit(lexer, i, end + 1, address ? RVN_TOKEN_LINK : RVN_TOKEN_TAG);
				i = end + 1;
				continue;
			}
		}
		if (c == 'h' && (rvn_lex_starts(lexer, i, length, "http://") || rvn_lex_starts(lexer, i, length, "https://"))) {
			end = i;
			while (end < length && text[end] != ' ' && text[end] != '\t' && text[end] != ')' && text[end] != '>') end++;
			rvn_lex_emit(lexer, i, end, RVN_TOKEN_LINK);
			i = end;
			continue;
		}
		if (c == '|') rvn_lex_emit(lexer, i, i + 1, RVN_TOKEN_PUNCTUATION);
		i++;
	}
	return 0;
}

uint8_t rvn_syntax_line(const rvn_language_t* language, const char* text, int length, uint8_t state,
	rvn_span_t* spans, int max, int* count) {

	rvn_lexer_t lexer;
	lexer.text = text;
	lexer.length = length;
	lexer.spans = spans;
	lexer.max = max;
	lexer.count = 0;
	if (!language) state = 0;
	else if (language->desc.flags & R_LANGUAGE_FORMATTED_TEXT) state = rvn_lex_markdown(&lexer, state);
	else if (language->desc.flags & R_LANGUAGE_MARKUP) state = rvn_lex_markup(&lexer, language, state);
	else if (language->desc.flags & R_LANGUAGE_STYLE_SHEET) state = rvn_lex_css(&lexer, language, state, 0, length);
	else state = rvn_lex_code(&lexer, language, state, 0, length);
	if (count) *count = lexer.count;
	return state;
}

rvn_color_t rvn_syntax_color(const rvn_theme_t* theme, rvn_token_t token, rvn_color_t text) {
	/* two palettes, for dark and for light backgrounds; 0 = the color of the text */
	static const uint32_t dark[RVN_TOKEN_COUNT] = {
		0, 0x7F8C98, 0xFC8E6D, 0xD9C97C, 0xFF7AB2, 0x6BDFFF, 0xDABAFF, 0x9AA5B1, 0xFFA14F, 0x67B7A4, 0x5DD8FF,
		0x5DD8FF, 0, 0, 0x6CB6FF
	};
	static const uint32_t light[RVN_TOKEN_COUNT] = {
		0, 0x6B7885, 0xC41A16, 0x1C00CF, 0x9B2393, 0x0B4F79, 0x6C36A9, 0x5C6873, 0x78492A, 0x326D74, 0x0F68A0,
		0x0F68A0, 0, 0, 0x0B62D6
	};
	uint32_t rgb;
	if (token <= RVN_TOKEN_TEXT || token >= RVN_TOKEN_COUNT) return text;
	/* keeps the opacity of the text (disabled editors) */
	if (theme->token_mask & (1u << token)) return rvn_color_fade(theme->tokens[token], (float)rvn_color_a(text) / 255.0f);
	rgb = theme->dark ? dark[token] : light[token];
	return rgb ? rvn_hex(rgb, rvn_color_a(text)) : text;
}

/* Public kinds of token, in the order of rvn_token_t. */
static const RToken g_public_tokens[RVN_TOKEN_COUNT] = {
	R_TOKEN_TEXT, R_TOKEN_COMMENT, R_TOKEN_STRING, R_TOKEN_NUMBER, R_TOKEN_KEYWORD, R_TOKEN_TAG, R_TOKEN_ATTRIBUTE,
	R_TOKEN_PUNCTUATION, R_TOKEN_DIRECTIVE, R_TOKEN_TYPE, R_TOKEN_FUNCTION, R_TOKEN_HEADING, R_TOKEN_STRONG,
	R_TOKEN_EMPHASIS, R_TOKEN_LINK
};

RToken rvn_token_public(rvn_token_t token) {
	return token < RVN_TOKEN_COUNT ? g_public_tokens[token] : R_TOKEN_TEXT;
}

static int rvn_token_internal(RToken token, rvn_token_t* internal) {
	int i;
	for (i = 1; i < RVN_TOKEN_COUNT; i++) {
		if (g_public_tokens[i] == token) {
			*internal = (rvn_token_t)i;
			return R_TRUE;
		}
	}
	return R_FALSE;
}

void Rtheme_set_token_color(RToken token, float r, float g, float b, float a) {
	rvn_token_t internal;
	RVN_API_GUARD_VOID();
	if (!rvn_token_internal(token, &internal)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtheme_set_token_color: invalid kind of token");
		return;
	}
	g_rvn.theme.tokens[internal] = rvn_rgba(r, g, b, a);
	g_rvn.theme.token_mask |= 1u << internal;
	rvn_repaint_all_windows();
}

void Rtheme_get_token_color(RToken token, float* r, float* g, float* b, float* a) {
	rvn_token_t internal;
	rvn_color_t color = 0;
	if (g_rvn.initialized && rvn_token_internal(token, &internal)) {
		color = rvn_syntax_color(&g_rvn.theme, internal, RVN_THEME_COLOR(R_COLOR_TEXT));
	}
	rvn_color_to_floats(color, r, g, b, a);
}

void Rtheme_clear_token_colors(void) {
	RVN_API_GUARD_VOID();
	g_rvn.theme.token_mask = 0;
	rvn_repaint_all_windows();
}

const RLanguageDesc* rvn_language_desc(const rvn_language_t* language) {
	return &language->desc;
}

uint32_t rvn_language_generation(void) {
	return g_languages.generation;
}

int rvn_language_word_count(const rvn_language_t* language, int list) {
	return (int)language->word_count[list];
}

const char* rvn_language_word(const rvn_language_t* language, int list, int index) {
	return language->words[list][index];
}

/* ---------------------------------------------------------------------------
 *  Registry
 * ------------------------------------------------------------------------- */

static const char* rvn_language_keep(rvn_language_t* language, const char* text) {
	if (!text) return NULL;
	return rvn_arena_strndup(&language->strings, text, strlen(text));
}

/* Cuts a list of words separated by spaces and sorts it. */
static void rvn_language_words(rvn_language_t* language, int list, const char* text) {
	char* copy;
	char* cursor;
	uint32_t count = 0, index = 0;
	if (!text || !text[0]) return;
	copy = rvn_arena_strndup(&language->strings, text, strlen(text));
	for (cursor = copy; *cursor;) {
		while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' || *cursor == ',') cursor++;
		if (!*cursor) break;
		count++;
		while (*cursor && *cursor != ' ' && *cursor != '\t' && *cursor != '\n' && *cursor != ',') cursor++;
	}
	if (count == 0) return;
	language->words[list] = (const char**)rvn_arena_alloc(&language->strings, sizeof(char*) * count);
	for (cursor = copy; *cursor && index < count;) {
		while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' || *cursor == ',') *cursor++ = '\0';
		if (!*cursor) break;
		language->words[list][index++] = cursor;
		while (*cursor && *cursor != ' ' && *cursor != '\t' && *cursor != '\n' && *cursor != ',') cursor++;
	}
	language->word_count[list] = index;
	qsort((void*)language->words[list], index, sizeof(char*),
		(language->desc.flags & R_LANGUAGE_IGNORE_CASE) ? rvn_language_compare_nocase : rvn_language_compare);
}

static rvn_language_t* rvn_language_build(const RLanguageDesc* desc) {
	rvn_language_t* language = (rvn_language_t*)rvn_calloc(sizeof(rvn_language_t));
	rvn_arena_init(&language->strings, 2048);
	language->desc.name = rvn_language_keep(language, desc->name);
	language->desc.extensions = rvn_language_keep(language, desc->extensions);
	language->desc.keywords = rvn_language_keep(language, desc->keywords);
	language->desc.types = rvn_language_keep(language, desc->types);
	language->desc.constants = rvn_language_keep(language, desc->constants);
	language->desc.line_comment = rvn_language_keep(language, desc->line_comment && desc->line_comment[0] ? desc->line_comment : NULL);
	language->desc.block_comment_start = rvn_language_keep(language, desc->block_comment_start);
	language->desc.block_comment_end = rvn_language_keep(language, desc->block_comment_end);
	language->desc.string_quotes = rvn_language_keep(language, desc->string_quotes);
	language->desc.multiline_strings = rvn_language_keep(language, desc->multiline_strings);
	language->desc.directive = rvn_language_keep(language, desc->directive && desc->directive[0] ? desc->directive : NULL);
	language->desc.annotation = rvn_language_keep(language, desc->annotation && desc->annotation[0] ? desc->annotation : NULL);
	language->desc.flags = desc->flags;
	language->desc.script_language = rvn_language_keep(language, desc->script_language && desc->script_language[0] ? desc->script_language : NULL);
	rvn_language_words(language, 0, desc->keywords);
	rvn_language_words(language, 1, desc->types);
	rvn_language_words(language, 2, desc->constants);
	if (desc->multiline_strings) {
		char* copy = rvn_arena_strndup(&language->strings, desc->multiline_strings, strlen(desc->multiline_strings));
		char* cursor = copy;
		while (*cursor && language->multiline_count < RVN_LANGUAGE_MAX_MULTILINE) {
			while (*cursor == ' ') *cursor++ = '\0';
			if (!*cursor) break;
			language->multiline[language->multiline_count] = cursor;
			language->multiline_close[language->multiline_count] = cursor;
			while (*cursor && *cursor != ' ') {
				/* R"(...)": what opens it, three dots, what closes it */
				if (cursor[0] == '.' && cursor[1] == '.' && cursor[2] == '.' && cursor != language->multiline[language->multiline_count] &&
					cursor[3] && cursor[3] != ' ') {
					*cursor = '\0';
					language->multiline_close[language->multiline_count] = cursor + 3;
					cursor += 3;
				}
				cursor++;
			}
			language->multiline_count++;
		}
	}
	return language;
}

static void rvn_language_free(rvn_language_t* language) {
	rvn_arena_free(&language->strings);
	rvn_free(language);
}

static void rvn_language_add(const RLanguageDesc* desc) {
	rvn_language_t* language = rvn_language_build(desc);
	uint32_t i;
	g_languages.generation++;
	for (i = 0; i < g_languages.count; i++) {
		if (rvn_stricmp(g_languages.items[i]->desc.name, desc->name) != 0) continue;
		/* replaced: kept aside, an editor may be painting with it */
		RVN_ARRAY_RESERVE(g_languages.retired, g_languages.retired_cap, g_languages.retired_count + 1);
		g_languages.retired[g_languages.retired_count++] = g_languages.items[i];
		g_languages.items[i] = language;
		return;
	}
	RVN_ARRAY_RESERVE(g_languages.items, g_languages.cap, g_languages.count + 1);
	g_languages.items[g_languages.count++] = language;
}

/* ---------------------------------------------------------------------------
 *  Built-in languages
 * ------------------------------------------------------------------------- */

#define RVN_C_KEYWORDS \
	"auto break case const continue default do else enum extern for goto if inline register restrict return sizeof " \
	"static struct switch typedef union volatile while _Alignas _Alignof _Atomic _Generic _Noreturn _Static_assert _Thread_local"
#define RVN_C_TYPES \
	"void char short int long float double signed unsigned bool _Bool size_t ssize_t ptrdiff_t intptr_t uintptr_t " \
	"int8_t int16_t int32_t int64_t uint8_t uint16_t uint32_t uint64_t wchar_t FILE va_list"

static void rvn_languages_built_in(void) {
	RLanguageDesc d;
	if (g_languages.built_in) return;
	g_languages.built_in = R_TRUE;

	memset(&d, 0, sizeof(d));
	d.name = "C";
	d.extensions = "c;h";
	d.keywords = RVN_C_KEYWORDS;
	d.types = RVN_C_TYPES;
	d.constants = "NULL true false";
	d.line_comment = "//";
	d.block_comment_start = "/*";
	d.block_comment_end = "*/";
	d.string_quotes = "\"'";
	d.directive = "#";
	d.flags = R_LANGUAGE_CHAR_LITERALS;
	rvn_language_add(&d);

	d.name = "C++";
	d.extensions = "cpp;cc;cxx;hpp;hh;hxx;inl;ixx";
	d.keywords = RVN_C_KEYWORDS " alignas alignof and and_eq asm bitand bitor catch class co_await co_return co_yield compl concept "
		"const_cast consteval constexpr constinit decltype delete dynamic_cast explicit export final friend mutable namespace new "
		"noexcept not not_eq operator or or_eq override private protected public reinterpret_cast requires static_assert static_cast "
		"template this thread_local throw try typeid typename using virtual xor xor_eq";
	d.types = RVN_C_TYPES " char8_t char16_t char32_t string string_view vector array map unordered_map set unordered_set optional "
		"variant tuple pair unique_ptr shared_ptr weak_ptr function span";
	d.constants = "NULL nullptr true false";
	d.multiline_strings = "R\"(...)\"";
	rvn_language_add(&d);

	d.name = "GLSL";
	d.extensions = "glsl;vert;frag;geom;comp;tesc;tese;hlsl";
	d.keywords = "attribute break case const continue default discard do else flat for if in inout invariant layout lowp mediump highp "
		"noperspective out precision return smooth struct subroutine switch uniform varying while buffer shared centroid sample patch";
	d.types = "void bool int uint float double vec2 vec3 vec4 ivec2 ivec3 ivec4 uvec2 uvec3 uvec4 bvec2 bvec3 bvec4 dvec2 dvec3 dvec4 "
		"mat2 mat3 mat4 mat2x3 mat2x4 mat3x2 mat3x4 mat4x2 mat4x3 sampler1D sampler2D sampler3D samplerCube sampler2DArray "
		"sampler2DShadow samplerCubeShadow image2D float2 float3 float4 float4x4 Texture2D SamplerState";
	d.constants = "true false gl_Position gl_FragCoord gl_FragDepth gl_VertexIndex gl_InstanceIndex gl_PointSize";
	rvn_language_add(&d);

	memset(&d, 0, sizeof(d));
	d.name = "C#";
	d.extensions = "cs";
	d.keywords = "abstract as async await base break case catch checked class const continue default delegate do else enum event "
		"explicit extern finally fixed for foreach get goto if implicit in init interface internal is lock namespace new operator out "
		"override params partial private protected public readonly record ref required return sealed set sizeof stackalloc static "
		"struct switch this throw try typeof unchecked unsafe using virtual volatile when where while with yield global nameof";
	d.types = "bool byte char decimal double dynamic float int long nint nuint object sbyte short string uint ulong ushort var void "
		"String Object Int32 Int64 Double Boolean List Dictionary IEnumerable Task Action Func Span Guid DateTime";
	d.constants = "true false null";
	d.line_comment = "//";
	d.block_comment_start = "/*";
	d.block_comment_end = "*/";
	d.string_quotes = "\"'";
	d.multiline_strings = "\"\"\" @\"...\"";
	d.directive = "#";
	d.flags = R_LANGUAGE_CHAR_LITERALS;
	rvn_language_add(&d);

	memset(&d, 0, sizeof(d));
	d.name = "Java";
	d.extensions = "java";
	d.keywords = "abstract assert break case catch class const continue default do else enum exports extends final finally for goto "
		"if implements import instanceof interface module native new non-sealed package permits private protected public record "
		"requires return sealed static strictfp super switch synchronized this throw throws transient try volatile while yield";
	d.types = "boolean byte char double float int long short var void String Object Integer Long Double Boolean Character List "
		"ArrayList Map HashMap Set HashSet Optional Stream";
	d.constants = "true false null";
	d.line_comment = "//";
	d.block_comment_start = "/*";
	d.block_comment_end = "*/";
	d.string_quotes = "\"'";
	d.multiline_strings = "\"\"\"";
	d.annotation = "@";
	d.flags = R_LANGUAGE_CHAR_LITERALS;
	rvn_language_add(&d);

	memset(&d, 0, sizeof(d));
	d.name = "JavaScript";
	d.extensions = "js;mjs;cjs;jsx";
	d.keywords = "async await break case catch class const continue debugger default delete do else export extends finally for from "
		"function get if import in instanceof let new of return set static super switch this throw try typeof var void while with yield";
	d.types = "Array Boolean Date Error Function JSON Map Math Number Object Promise Proxy Reflect RegExp Set String Symbol WeakMap "
		"WeakSet BigInt console document window globalThis";
	d.constants = "true false null undefined NaN Infinity";
	d.line_comment = "//";
	d.block_comment_start = "/*";
	d.block_comment_end = "*/";
	d.string_quotes = "\"'";
	d.multiline_strings = "`";
	rvn_language_add(&d);

	d.name = "TypeScript";
	d.extensions = "ts;tsx;mts";
	d.keywords = "abstract as asserts async await break case catch class const continue debugger declare default delete do else enum "
		"export extends finally for from function get if implements import in infer instanceof interface is keyof let module namespace "
		"new of override private protected public readonly return satisfies set static super switch this throw try type typeof var "
		"void while with yield";
	d.types = "any bigint boolean never number object string symbol unknown Array Date Error Map Promise Record Partial Readonly "
		"Set Pick Omit console";
	rvn_language_add(&d);

	memset(&d, 0, sizeof(d));
	d.name = "Python";
	d.extensions = "py;pyw;pyi";
	d.keywords = "and as assert async await break case class continue def del elif else except finally for from global if import in is "
		"lambda match nonlocal not or pass raise return try while with yield";
	d.types = "bool bytearray bytes complex dict float frozenset int list object set str tuple type range print len open super "
		"isinstance enumerate zip map filter sorted min max sum abs any all self cls";
	d.constants = "True False None NotImplemented Ellipsis __name__ __main__";
	d.line_comment = "#";
	d.string_quotes = "\"'";
	d.multiline_strings = "\"\"\" '''";
	d.annotation = "@";
	d.flags = R_LANGUAGE_INDENT_ON_COLON;
	rvn_language_add(&d);

	memset(&d, 0, sizeof(d));
	d.name = "Rust";
	d.extensions = "rs";
	d.keywords = "as async await break const continue crate dyn else enum extern fn for if impl in let loop match mod move mut pub ref "
		"return self Self static struct super trait type unsafe use where while macro_rules union";
	d.types = "bool char f32 f64 i8 i16 i32 i64 i128 isize str u8 u16 u32 u64 u128 usize String Vec Option Result Box Rc Arc Cell "
		"RefCell HashMap HashSet BTreeMap Path PathBuf";
	d.constants = "true false Some None Ok Err";
	d.line_comment = "//";
	d.block_comment_start = "/*";
	d.block_comment_end = "*/";
	d.string_quotes = "\"'";
	/* (an ordinary text may span lines too) */
	d.multiline_strings = "r#\"...\"# r\"...\" \"";
	d.annotation = "#[";
	d.flags = R_LANGUAGE_CHAR_LITERALS | R_LANGUAGE_NESTED_COMMENTS;
	rvn_language_add(&d);

	memset(&d, 0, sizeof(d));
	d.name = "JSON";
	d.extensions = "json;jsonc";
	d.constants = "true false null";
	d.line_comment = "//";
	d.block_comment_start = "/*";
	d.block_comment_end = "*/";
	d.string_quotes = "\"";
	rvn_language_add(&d);

	memset(&d, 0, sizeof(d));
	d.name = "CSS";
	d.extensions = "css;scss;less";
	d.keywords = "inherit initial unset revert auto none normal bold italic solid dashed dotted block inline flex grid absolute "
		"relative fixed sticky hidden visible center left right top bottom transparent currentColor";
	d.block_comment_start = "/*";
	d.block_comment_end = "*/";
	d.string_quotes = "\"'";
	d.flags = R_LANGUAGE_STYLE_SHEET;
	rvn_language_add(&d);

	memset(&d, 0, sizeof(d));
	d.name = "XML";
	d.extensions = "xml;xaml;svg;ui;xsd;xsl;plist;csproj;vcxproj";
	d.block_comment_start = "<!--";
	d.block_comment_end = "-->";
	d.flags = R_LANGUAGE_MARKUP;
	rvn_language_add(&d);

	d.name = "HTML";
	d.extensions = "html;htm;xhtml;vue";
	d.flags = R_LANGUAGE_MARKUP | R_LANGUAGE_EMBEDS | R_LANGUAGE_IGNORE_CASE;
	rvn_language_add(&d);

	memset(&d, 0, sizeof(d));
	d.name = "Markdown";
	d.extensions = "md;markdown;mdown;mkd";
	d.block_comment_start = "<!--";
	d.block_comment_end = "-->";
	d.flags = R_LANGUAGE_FORMATTED_TEXT;
	rvn_language_add(&d);
}

void rvn_languages_shutdown(void) {
	uint32_t i;
	for (i = 0; i < g_languages.count; i++) rvn_language_free(g_languages.items[i]);
	for (i = 0; i < g_languages.retired_count; i++) rvn_language_free(g_languages.retired[i]);
	rvn_free(g_languages.items);
	rvn_free(g_languages.retired);
	memset(&g_languages, 0, sizeof(g_languages));
}

/* Does a list "c;h;inl" hold an extension? */
static int rvn_language_has_extension(const char* list, const char* extension) {
	size_t length = strlen(extension);
	if (!list || length == 0) return R_FALSE;
	while (*list) {
		size_t item = strcspn(list, ";, ");
		if (item == length && rvn_strnicmp(list, extension, length) == 0) return R_TRUE;
		list += item;
		while (*list == ';' || *list == ',' || *list == ' ') list++;
	}
	return R_FALSE;
}

const rvn_language_t* rvn_language_find(const char* name) {
	/* other names people give to the built-in languages */
	static const struct { const char* alias; const char* name; } aliases[] = {
		{ "Cpp", "C++" }, { "CSharp", "C#" }, { "JS", "JavaScript" }, { "TS", "TypeScript" }, { "Shader", "GLSL" }, { "HLSL", "GLSL" }
	};
	uint32_t i;
	if (!name || !name[0] || rvn_stricmp(name, "None") == 0) return NULL;
	rvn_languages_built_in();
	for (i = 0; i < RVN_COUNT_OF(aliases); i++) {
		if (rvn_stricmp(name, aliases[i].alias) == 0) name = aliases[i].name;
	}
	for (i = 0; i < g_languages.count; i++) {
		if (rvn_stricmp(g_languages.items[i]->desc.name, name) == 0) return g_languages.items[i];
	}
	/* "py", "rs": the extension of its files names a language too */
	for (i = 0; i < g_languages.count; i++) {
		if (rvn_language_has_extension(g_languages.items[i]->desc.extensions, name)) return g_languages.items[i];
	}
	return NULL;
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

int Rlanguage_register(const RLanguageDesc* desc) {
	RVN_API_GUARD(R_FALSE);
	if (!desc || !desc->name || !desc->name[0]) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rlanguage_register: a language needs a name");
		return R_FALSE;
	}
	rvn_languages_built_in();
	rvn_language_add(desc);
	rvn_repaint_all_windows();
	return R_TRUE;
}

int Rlanguage_get(const char* name, RLanguageDesc* desc) {
	const rvn_language_t* language;
	if (desc) memset(desc, 0, sizeof(*desc));
	if (!g_rvn.initialized || !desc) return R_FALSE;
	language = rvn_language_find(name);
	if (!language) return R_FALSE;
	*desc = language->desc;
	return R_TRUE;
}

int Rlanguage_get_count(void) {
	if (!g_rvn.initialized) return 0;
	rvn_languages_built_in();
	return (int)g_languages.count;
}

const char* Rlanguage_get_name(int index) {
	if (!g_rvn.initialized) return NULL;
	rvn_languages_built_in();
	if (index < 0 || index >= (int)g_languages.count) return NULL;
	return g_languages.items[index]->desc.name;
}

const char* Rlanguage_for_file(const char* path) {
	const char* dot;
	const char* cursor;
	uint32_t i;
	if (!g_rvn.initialized || !path) return NULL;
	rvn_languages_built_in();
	dot = NULL;
	for (cursor = path; *cursor; cursor++) {
		if (*cursor == '.') dot = cursor;
		else if (*cursor == '/' || *cursor == '\\') dot = NULL;
	}
	if (!dot || !dot[1]) return NULL;
	for (i = 0; i < g_languages.count; i++) {
		if (rvn_language_has_extension(g_languages.items[i]->desc.extensions, dot + 1)) return g_languages.items[i]->desc.name;
	}
	return NULL;
}
