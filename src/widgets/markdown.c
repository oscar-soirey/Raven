/**
 * Raven - widgets/markdown.c
 * Formatted text: reads Markdown, lays it out on a width and paints it. Used
 * by the labels that have Markdown turned on.
 *
 * What is understood:
 *   blocks   paragraphs, # titles (1 to 6), lists (-, *, +, 1.) and lists in
 *            lists, task lists (- [ ] and - [x]), > quotes, ``` code ``` with
 *            the colors of its language, --- rules, | tables | with the
 *            alignment of their columns, images alone on their line
 *   inline   **strong**, *emphasis*, ***both***, `code`, ~~struck~~,
 *            [links](url), <url>, bare http(s) addresses, \escapes, two
 *            spaces or a backslash at the end of a line for a line break,
 *            the usual &entities;
 *
 * Three steps, each kept by the next: the text is parsed once into blocks
 * and spans; the blocks are laid out when the width changes, into runs of
 * text and boxes placed in the label; painting draws those.
 */

#include "common.h"

#define RVN_MD_BOLD   (1u << 0)
#define RVN_MD_ITALIC (1u << 1)
#define RVN_MD_CODE   (1u << 2)
#define RVN_MD_STRIKE (1u << 3)
/** The line ends after this span. */
#define RVN_MD_BREAK  (1u << 4)
/** The span is a formula: a picture the application drew, on the line it is in. */
#define RVN_MD_MATH   (1u << 5)
/** The formula was written with $$, so it copies back with them. */
#define RVN_MD_DISPLAY (1u << 6)

typedef enum rvn_md_block_type_t {
	RVN_MD_PARAGRAPH,
	RVN_MD_HEADING,
	RVN_MD_CODE_BLOCK,
	RVN_MD_RULE,
	RVN_MD_TABLE,
	RVN_MD_IMAGE,
	/** A formula alone in a paragraph ($$...$$): centered, on its own line. */
	RVN_MD_MATH_BLOCK
} rvn_md_block_type_t;

enum { RVN_MD_NO_MARKER, RVN_MD_BULLET, RVN_MD_NUMBER, RVN_MD_TASK, RVN_MD_TASK_DONE };

/** A piece of text with one look. `start` and `length` are in rvn_md_t::text. */
typedef struct rvn_md_span_t {
	uint32_t start;
	uint32_t length;
	uint16_t flags;
	/** Index in rvn_md_t::links, or -1. */
	int16_t link;
} rvn_md_span_t;

typedef struct rvn_md_block_t {
	uint8_t type;
	/** Title: 1 to 6. */
	uint8_t level;
	/** How many quotes it is in. */
	uint8_t quote;
	/** How many lists it is in. */
	uint8_t list;
	/** What is drawn before it when it starts a list item. */
	uint8_t marker;
	int number;
	/** Text of a paragraph or of a title; alternative text of an image. */
	uint32_t first_span;
	uint32_t span_count;
	/** Code: its lines, in rvn_md_t::text, and the name of its language (0 length = none). */
	uint32_t code_start, code_length;
	uint32_t language_start, language_length;
	/** Table: its cells, row after row, in rvn_md_t::cells; the first row is the header. */
	uint32_t first_cell;
	uint16_t columns, rows;
	/** Image: its address, in rvn_md_t::links. */
	int16_t link;
} rvn_md_block_t;

typedef struct rvn_md_cell_t {
	uint32_t first_span;
	uint32_t span_count;
	/** 0 left, 1 center, 2 right. */
	uint8_t align;
} rvn_md_cell_t;

enum {
	RVN_MD_COLOR_TEXT,
	RVN_MD_COLOR_LINK,
	RVN_MD_COLOR_QUOTE,
	/** A kind of token of the code: RVN_MD_COLOR_TOKEN + rvn_token_t. */
	RVN_MD_COLOR_TOKEN
};

/** Text placed by the layout: a part of a line with one font. */
typedef struct rvn_md_run_t {
	float x, y, w;
	/** The line it is on: where a click is on it. */
	float line_y, line_h;
	uint32_t start, length;
	Rid font;
	float size;
	uint8_t color;
	uint8_t flags;
	int16_t link;
	/** Code: nothing is drawn past this x (0 = no limit). */
	float clip_right;
	/** Index in rvn_md_t::maths, or -1: a formula, whose picture is drawn instead of text. */
	int16_t math;
} rvn_md_run_t;

enum {
	RVN_MD_BOX_CODE,
	RVN_MD_BOX_INLINE_CODE,
	RVN_MD_BOX_QUOTE,
	RVN_MD_BOX_RULE,
	RVN_MD_BOX_BULLET,
	RVN_MD_BOX_TASK,
	RVN_MD_BOX_TASK_DONE,
	RVN_MD_BOX_TABLE_HEADER,
	RVN_MD_BOX_TABLE_LINE,
	RVN_MD_BOX_IMAGE,
	/** A formula, in a line of text. */
	RVN_MD_BOX_MATH
};

/** A formula, as the application drew it: what the layout places instead of words. */
typedef struct rvn_md_math_t {
	/** Where its source is, in rvn_md_t::text. */
	uint32_t start, length;
	/** The size it was asked at (0: not asked yet), and the box it takes. */
	float size;
	/** It was written with $$: the application sets it as display math. */
	int display;
	float width, height, ascent;
	Rid texture;
	/** The application could not draw it: its source is shown as text. */
	int failed;
} rvn_md_math_t;

typedef struct rvn_md_box_t {
	float x, y, w, h;
	uint8_t kind;
	Rid texture;
} rvn_md_box_t;

struct rvn_md_t {
	/** Every character shown, escapes resolved. Spans and runs point into it. */
	rvn_text_t text;
	/** Its length after parsing: the layout adds the numbers of the lists after it. */
	size_t parsed_length;
	rvn_md_span_t* spans;
	uint32_t span_count, span_cap;
	rvn_md_block_t* blocks;
	uint32_t block_count, block_cap;
	rvn_md_cell_t* cells;
	uint32_t cell_count, cell_cap;
	char** links;
	uint32_t link_count, link_cap;
	/** The formulas of the text, and the pictures the application drew for them:
	 * kept from one layout to the next, by source and size (see rvn_md_math_resolve). */
	rvn_md_math_t* maths;
	uint32_t math_count, math_cap;
	/** The text holds formulas: a $ is read as one (see rvn_md_parse). */
	int math;

	/* layout */
	rvn_md_run_t* runs;
	uint32_t run_count, run_cap;
	rvn_md_box_t* boxes;
	uint32_t box_count, box_cap;
	float laid_width;
	float laid_size;
	Rid laid_font;
	uint32_t laid_generation;
	int laid;
	rvn_vec2_t size;
};

/* ---------------------------------------------------------------------------
 *  Parsing: inline
 * ------------------------------------------------------------------------- */

typedef struct rvn_md_inline_t {
	rvn_md_t* md;
	uint32_t flags;
	int link;
	/** Start, in md->text, of the span being filled. */
	uint32_t span_start;
	/** Nothing was written on the line yet: white space does not start it. */
	int fresh;
} rvn_md_inline_t;

static int rvn_md_add_link(rvn_md_t* md, const char* url, size_t length) {
	if (md->link_count >= 32000u) return -1;
	RVN_ARRAY_RESERVE(md->links, md->link_cap, md->link_count + 1);
	md->links[md->link_count] = rvn_strndup(url, length);
	return (int)md->link_count++;
}

/* Closes the span being filled, if it holds anything. */
static void rvn_md_flush(rvn_md_inline_t* in, uint32_t extra_flags) {
	rvn_md_t* md = in->md;
	uint32_t end = (uint32_t)md->text.length;
	if (end > in->span_start || (extra_flags & RVN_MD_BREAK)) {
		rvn_md_span_t* span;
		RVN_ARRAY_RESERVE(md->spans, md->span_cap, md->span_count + 1);
		span = &md->spans[md->span_count++];
		span->start = in->span_start;
		span->length = end - in->span_start;
		span->flags = (uint16_t)(in->flags | extra_flags);
		span->link = (int16_t)in->link;
	}
	in->span_start = end;
}

static void rvn_md_put(rvn_md_inline_t* in, const char* bytes, size_t length) {
	rvn_text_append(&in->md->text, bytes, length);
	if (length) in->fresh = R_FALSE;
}

static int rvn_md_is_space(char c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int rvn_md_is_punct(char c) {
	return (c >= '!' && c <= '/') || (c >= ':' && c <= '@') || (c >= '[' && c <= '`') || (c >= '{' && c <= '~');
}

static int rvn_md_is_word(char c) {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || ((unsigned char)c & 0x80u);
}

/*
 * A formula at `p` (on its first '$'): $...$ or $$...$$. Gives where its source
 * is and where what follows the closing marks is, or NULL when it is not one.
 *
 * Inside the marks the escaping is the escaping of the formula, not of the
 * Markdown: \$ is a $ that closes nothing, and every other backslash is part
 * of the source, untouched. A $ that follows a space, or that a space
 * precedes, is a $ of the text ("it costs $5 and $10" or "a $ b $"), and so
 * is a $ a digit follows.
 */
static const char* rvn_md_math_parts(const char* p, const char* end, const char** source, size_t* length, int* display) {
	const char* close;
	int marks = end - p > 1 && p[1] == '$' ? 2 : 1;
	if (display) *display = marks == 2;
	*source = p + marks;
	close = *source;
	while (close < end) {
		if (*close == '\\' && close + 1 < end) {
			close += 2;
			continue;
		}
		if (*close != '$') {
			/* (a formula inside a line does not run over the lines around it) */
			if (marks == 1 && *close == '\n') return NULL;
			close++;
			continue;
		}
		if (close + 1 < end && close[1] == '$' && marks == 2) {
			if (close == *source) return NULL;
			*length = (size_t)(close - *source);
			return close + 2;
		}
		if (marks == 2) {
			close++;
			continue;
		}
		if (close == *source || rvn_md_is_space(close[-1])) return NULL;
		if (rvn_md_is_space((*source)[0])) return NULL;
		if (close + 1 < end && close[1] >= '0' && close[1] <= '9') return NULL;
		*length = (size_t)(close - *source);
		return close + 1;
	}
	return NULL;
}

/* Is there, after `from`, the same run of `count` marks that could close what one opens here? */
static int rvn_md_has_closer(const char* from, const char* end, char mark, int count) {
	const char* p = from;
	while (p < end) {
		if (*p == '\\' && p + 1 < end) {
			p += 2;
			continue;
		}
		if (*p == '`') {
			/* marks inside code do not count */
			const char* close;
			int ticks = 0;
			while (p + ticks < end && p[ticks] == '`') ticks++;
			close = p + ticks;
			while (close < end) {
				int run = 0;
				while (close + run < end && close[run] == '`') run++;
				if (run == ticks) break;
				close += run ? run : 1;
			}
			p = close < end ? close + ticks : p + ticks;
			continue;
		}
		if (*p == mark) {
			int run = 0;
			while (p + run < end && p[run] == mark) run++;
			if (run >= count && p > from && !rvn_md_is_space(p[-1])) return R_TRUE;
			p += run;
			continue;
		}
		p++;
	}
	return R_FALSE;
}

static const struct { const char* name; const char* text; } g_md_entities[] = {
	{ "amp", "&" }, { "lt", "<" }, { "gt", ">" }, { "quot", "\"" }, { "apos", "'" }, { "nbsp", "\xC2\xA0" },
	{ "copy", "\xC2\xA9" }, { "reg", "\xC2\xAE" }, { "hellip", "\xE2\x80\xA6" }, { "mdash", "\xE2\x80\x94" },
	{ "ndash", "\xE2\x80\x93" }, { "rarr", "\xE2\x86\x92" }, { "larr", "\xE2\x86\x90" }, { "times", "\xC3\x97" }
};

/* An entity at `p` ("&amp;"): writes what it stands for and returns its length, or 0. */
static size_t rvn_md_entity(rvn_md_inline_t* in, const char* p, const char* end) {
	const char* semi = p + 1;
	size_t i, length;
	while (semi < end && semi - p < 10 && *semi != ';' && !rvn_md_is_space(*semi)) semi++;
	if (semi >= end || *semi != ';') return 0;
	length = (size_t)(semi - p - 1);
	if (length >= 2 && p[1] == '#') {
		uint32_t code = 0;
		char utf8[4];
		const char* digit = p + 2;
		int hex = *digit == 'x' || *digit == 'X';
		if (hex) digit++;
		if (digit == semi) return 0;
		for (; digit < semi; digit++) {
			char c = *digit;
			if (c >= '0' && c <= '9') code = code * (hex ? 16u : 10u) + (uint32_t)(c - '0');
			else if (hex && c >= 'a' && c <= 'f') code = code * 16u + (uint32_t)(c - 'a' + 10);
			else if (hex && c >= 'A' && c <= 'F') code = code * 16u + (uint32_t)(c - 'A' + 10);
			else return 0;
		}
		if (code == 0 || code > 0x10FFFFu) return 0;
		rvn_md_put(in, utf8, (size_t)rvn_utf8_encode(code, utf8));
		return (size_t)(semi - p) + 1;
	}
	for (i = 0; i < RVN_COUNT_OF(g_md_entities); i++) {
		if (strlen(g_md_entities[i].name) == length && memcmp(g_md_entities[i].name, p + 1, length) == 0) {
			rvn_md_put(in, g_md_entities[i].text, strlen(g_md_entities[i].text));
			return length + 2;
		}
	}
	return 0;
}

/* The end of an address that starts at `p` (http://...), without the punctuation that ends a sentence. */
static const char* rvn_md_url_end(const char* p, const char* end) {
	const char* stop = p;
	while (stop < end && !rvn_md_is_space(*stop) && *stop != '<' && *stop != '>' && *stop != '"') stop++;
	while (stop > p && (stop[-1] == '.' || stop[-1] == ',' || stop[-1] == ';' || stop[-1] == ':' || stop[-1] == '!' ||
		stop[-1] == '?' || stop[-1] == ')' || stop[-1] == '\'' || stop[-1] == '*' || stop[-1] == '_')) stop--;
	return stop;
}

static void rvn_md_inline(rvn_md_inline_t* in, const char* p, const char* end, int depth);

/*
 * "[text](url)" or "![alt](url)" at `p` (on the '['). Returns where it ends,
 * or NULL when it is not one. `text_end` is the ']' and `url`, `url_length`
 * the address.
 */
static const char* rvn_md_link_parts(const char* p, const char* end, const char** text_end, const char** url, size_t* url_length) {
	const char* close = p + 1;
	const char* q;
	int nest = 1;
	while (close < end) {
		if (*close == '\\' && close + 1 < end) {
			close += 2;
			continue;
		}
		if (*close == '[') nest++;
		else if (*close == ']' && --nest == 0) break;
		close++;
	}
	if (close >= end || close + 1 >= end || close[1] != '(') return NULL;
	q = close + 2;
	while (q < end && (*q == ' ' || *q == '\n')) q++;
	*url = q;
	if (q < end && *q == '<') {
		*url = ++q;
		while (q < end && *q != '>' && *q != '\n') q++;
		if (q >= end || *q != '>') return NULL;
		*url_length = (size_t)(q - *url);
		q++;
	} else {
		int parens = 0;
		while (q < end && !rvn_md_is_space(*q)) {
			if (*q == '(') parens++;
			else if (*q == ')' && parens-- == 0) break;
			q++;
		}
		*url_length = (size_t)(q - *url);
	}
	/* an optional title, ignored: ("title") */
	while (q < end && (*q == ' ' || *q == '\n')) q++;
	if (q < end && (*q == '"' || *q == '\'')) {
		char quote = *q++;
		while (q < end && *q != quote) q++;
		if (q < end) q++;
		while (q < end && (*q == ' ' || *q == '\n')) q++;
	}
	if (q >= end || *q != ')') return NULL;
	*text_end = close;
	return q + 1;
}

/* Reads inline text into spans, with the looks that are on when it starts. */
static void rvn_md_inline(rvn_md_inline_t* in, const char* p, const char* end, int depth) {
	const char* start = p;
	int space_pending = R_FALSE;

	while (p < end) {
		char c = *p;
		char previous = p > start ? p[-1] : ' ';

		/* white space, new lines included, is one space; a line that ends with two spaces or a backslash is a line break */
		if (rvn_md_is_space(c)) {
			const char* q = p;
			int newline = R_FALSE, spaces = 0;
			while (q < end && rvn_md_is_space(*q)) {
				if (*q == '\n') {
					if (!newline && spaces >= 2) {
						rvn_md_flush(in, RVN_MD_BREAK);
						in->fresh = R_TRUE;
					}
					newline = R_TRUE;
				} else if (!newline) {
					spaces++;
				}
				q++;
			}
			space_pending = !in->fresh;
			p = q;
			continue;
		}
		if (space_pending) {
			rvn_md_put(in, " ", 1);
			space_pending = R_FALSE;
		}

		if (c == '\\' && p + 1 < end) {
			if (p[1] == '\n') {
				rvn_md_flush(in, RVN_MD_BREAK);
				in->fresh = R_TRUE;
				p += 2;
				continue;
			}
			if (rvn_md_is_punct(p[1])) {
				rvn_md_put(in, p + 1, 1);
				p += 2;
				continue;
			}
		}

		if (c == '`') {
			const char* close;
			const char* content;
			int ticks = 0;
			while (p + ticks < end && p[ticks] == '`') ticks++;
			content = p + ticks;
			close = content;
			while (close < end) {
				int run = 0;
				while (close + run < end && close[run] == '`') run++;
				if (run == ticks) break;
				close += run ? run : 1;
			}
			if (close < end) {
				const char* q;
				size_t length = (size_t)(close - content);
				/* one space of padding on each side is not part of the code */
				if (length >= 2 && content[0] == ' ' && content[length - 1] == ' ') {
					content++;
					length -= 2;
				}
				rvn_md_flush(in, 0);
				in->flags |= RVN_MD_CODE;
				for (q = content; q < content + length; q++) rvn_md_put(in, *q == '\n' ? " " : q, 1);
				rvn_md_flush(in, 0);
				in->flags &= ~RVN_MD_CODE;
				p = close + ticks;
				continue;
			}
		}

		if (c == '$' && in->md->math) {
			const char* source = NULL;
			const char* after;
			size_t length = 0;
			int display = R_FALSE;
			after = rvn_md_math_parts(p, end, &source, &length, &display);
			if (after) {
				/* (display marks: the formula has a line of its own, and so has the text before it) */
				if (display && in->md->text.length > in->span_start) rvn_md_flush(in, RVN_MD_BREAK);
				else rvn_md_flush(in, 0);
				in->flags |= RVN_MD_MATH | (display ? RVN_MD_DISPLAY : 0u);
				in->fresh = R_FALSE;
				rvn_md_put(in, source, length);
				rvn_md_flush(in, display ? RVN_MD_BREAK : 0);
				in->flags &= ~(RVN_MD_MATH | RVN_MD_DISPLAY);
				p = after;
				continue;
			}
		}

		if (c == '*' || c == '_' || c == '~') {
			int run = 0, used = 0;
			char next;
			while (p + run < end && p[run] == c) run++;
			next = p + run < end ? p[run] : ' ';
			while (used < run) {
				int left = run - used;
				int count = c == '~' ? 2 : (left >= 2 ? 2 : 1);
				uint32_t flag = c == '~' ? RVN_MD_STRIKE : (count == 2 ? RVN_MD_BOLD : RVN_MD_ITALIC);
				int inside_word = c == '_' && rvn_md_is_word(previous) && rvn_md_is_word(next);
				if (left < count) break;
				if (in->flags & flag) {
					/* closes: what is before must be text */
					if (rvn_md_is_space(previous) || inside_word) break;
					rvn_md_flush(in, 0);
					in->flags &= ~flag;
				} else {
					if (rvn_md_is_space(next) || inside_word || !rvn_md_has_closer(p + run, end, c, count)) break;
					rvn_md_flush(in, 0);
					in->flags |= flag;
				}
				used += count;
			}
			/* marks that neither open nor close anything are text */
			if (used < run) rvn_md_put(in, p + used, (size_t)(run - used));
			p += run;
			continue;
		}

		if ((c == '[' || (c == '!' && p + 1 < end && p[1] == '[')) && depth < 4) {
			const char* open = c == '!' ? p + 1 : p;
			const char* text_end = NULL;
			const char* url = NULL;
			size_t url_length = 0;
			const char* after = rvn_md_link_parts(open, end, &text_end, &url, &url_length);
			if (after) {
				int outer = in->link;
				rvn_md_flush(in, 0);
				in->link = rvn_md_add_link(in->md, url, url_length);
				/* (an image inside a line of text is shown by its alternative text) */
				rvn_md_inline(in, open + 1, text_end, depth + 1);
				rvn_md_flush(in, 0);
				in->link = outer;
				p = after;
				continue;
			}
		}

		if (c == '<') {
			const char* close = p + 1;
			while (close < end && *close != '>' && !rvn_md_is_space(*close)) close++;
			if (close < end && *close == '>' && close - p > 4 && (memchr(p + 1, ':', (size_t)(close - p - 1)) || memchr(p + 1, '@', (size_t)(close - p - 1)))) {
				int outer = in->link;
				rvn_md_flush(in, 0);
				in->link = rvn_md_add_link(in->md, p + 1, (size_t)(close - p - 1));
				rvn_md_put(in, p + 1, (size_t)(close - p - 1));
				rvn_md_flush(in, 0);
				in->link = outer;
				p = close + 1;
				continue;
			}
		}

		if (c == 'h' && in->link < 0 && !rvn_md_is_word(previous) && end - p > 8 &&
			(memcmp(p, "http://", 7) == 0 || memcmp(p, "https://", 8) == 0)) {
			const char* stop = rvn_md_url_end(p, end);
			if (stop - p > 8) {
				rvn_md_flush(in, 0);
				in->link = rvn_md_add_link(in->md, p, (size_t)(stop - p));
				rvn_md_put(in, p, (size_t)(stop - p));
				rvn_md_flush(in, 0);
				in->link = -1;
				p = stop;
				continue;
			}
		}

		if (c == '&') {
			size_t used = rvn_md_entity(in, p, end);
			if (used) {
				p += used;
				continue;
			}
		}

		rvn_md_put(in, p, 1);
		p++;
	}
}

/* Reads the inline text of a block; returns its first span and their number. */
static void rvn_md_read_inline(rvn_md_t* md, const char* text, size_t length, uint32_t* first, uint32_t* count) {
	rvn_md_inline_t in;
	in.md = md;
	in.flags = 0;
	in.link = -1;
	in.fresh = R_TRUE;
	in.span_start = (uint32_t)md->text.length;
	*first = md->span_count;
	rvn_md_inline(&in, text, text + length, 0);
	rvn_md_flush(&in, 0);
	*count = md->span_count - *first;
}

/* ---------------------------------------------------------------------------
 *  Parsing: blocks
 * ------------------------------------------------------------------------- */

typedef struct rvn_md_parser_t {
	rvn_md_t* md;
	/** Lines of the paragraph being read, joined by '\n'. */
	rvn_text_t paragraph;
	int paragraph_open;
	uint8_t quote;
	uint8_t list;
	uint8_t marker;
	int number;
	uint8_t heading;
	/** The lists that are open: the indentation of their markers. */
	int indents[8];
	int depth;
} rvn_md_parser_t;

static rvn_md_block_t* rvn_md_add_block(rvn_md_t* md, int type) {
	rvn_md_block_t* block;
	RVN_ARRAY_RESERVE(md->blocks, md->block_cap, md->block_count + 1);
	block = &md->blocks[md->block_count++];
	memset(block, 0, sizeof(*block));
	block->type = (uint8_t)type;
	block->link = -1;
	return block;
}

/* Is the paragraph nothing but an image? Gives its parts. */
static int rvn_md_lone_image(const char* text, size_t length, const char** alt, size_t* alt_length, const char** url, size_t* url_length) {
	const char* end = text + length;
	const char* text_end = NULL;
	const char* after;
	while (text < end && rvn_md_is_space(*text)) text++;
	while (end > text && rvn_md_is_space(end[-1])) end--;
	if (end - text < 5 || text[0] != '!' || text[1] != '[') return R_FALSE;
	after = rvn_md_link_parts(text + 1, end, &text_end, url, url_length);
	if (!after || after != end) return R_FALSE;
	*alt = text + 2;
	*alt_length = (size_t)(text_end - text - 2);
	return R_TRUE;
}

/* Is the paragraph nothing but a formula, $$...$$? Gives its source. */
static int rvn_md_lone_math(const char* text, size_t length, const char** source, size_t* source_length) {
	const char* end = text + length;
	int display = R_FALSE;
	while (text < end && rvn_md_is_space(*text)) text++;
	while (end > text && rvn_md_is_space(end[-1])) end--;
	if (end - text < 5 || text[0] != '$' || text[1] != '$') return R_FALSE;
	return rvn_md_math_parts(text, end, source, source_length, &display) == end && display;
}

/*
 * The one span of a display formula. Its source is copied as it was written: the
 * escaping inside a formula is the escaping of the formula, and reading it as
 * Markdown again would eat the backslashes TeX is made of.
 */
static void rvn_md_math_block(rvn_md_t* md, rvn_md_block_t* block, const char* source, size_t length) {
	rvn_md_span_t* span;
	block->first_span = md->span_count;
	block->span_count = 1;
	RVN_ARRAY_RESERVE(md->spans, md->span_cap, md->span_count + 1);
	span = &md->spans[md->span_count++];
	span->start = (uint32_t)md->text.length;
	span->length = (uint32_t)length;
	span->flags = RVN_MD_MATH | RVN_MD_DISPLAY;
	span->link = -1;
	rvn_text_append(&md->text, source, length);
}

static void rvn_md_end_paragraph(rvn_md_parser_t* parser) {
	rvn_md_t* md = parser->md;
	rvn_md_block_t* block;
	const char* alt;
	const char* url;
	const char* source;
	size_t alt_length, url_length, source_length;
	if (!parser->paragraph_open) return;
	parser->paragraph_open = R_FALSE;

	if (!parser->heading && md->math && rvn_md_lone_math(parser->paragraph.data, parser->paragraph.length, &source, &source_length)) {
		block = rvn_md_add_block(md, RVN_MD_MATH_BLOCK);
		rvn_md_math_block(md, block, source, source_length);
	} else if (!parser->heading && rvn_md_lone_image(parser->paragraph.data, parser->paragraph.length, &alt, &alt_length, &url, &url_length)) {
		int link = rvn_md_add_link(md, url, url_length);
		block = rvn_md_add_block(md, RVN_MD_IMAGE);
		block->link = (int16_t)link;
		rvn_md_read_inline(md, alt, alt_length, &block->first_span, &block->span_count);
	} else {
		block = rvn_md_add_block(md, parser->heading ? RVN_MD_HEADING : RVN_MD_PARAGRAPH);
		block->level = parser->heading;
		rvn_md_read_inline(md, parser->paragraph.data ? parser->paragraph.data : "", parser->paragraph.length, &block->first_span, &block->span_count);
	}
	block->quote = parser->quote;
	block->list = parser->list;
	block->marker = parser->marker;
	block->number = parser->number;
	parser->marker = RVN_MD_NO_MARKER;
	parser->heading = 0;
	parser->paragraph.length = 0;
	if (parser->paragraph.data) parser->paragraph.data[0] = '\0';
}

static void rvn_md_paragraph_line(rvn_md_parser_t* parser, const char* line, size_t length) {
	if (parser->paragraph_open) rvn_text_append(&parser->paragraph, "\n", 1);
	rvn_text_append(&parser->paragraph, line, length);
	parser->paragraph_open = R_TRUE;
}

/* "---", "***", "___", with or without spaces: a rule. */
static int rvn_md_is_rule(const char* line, size_t length) {
	char mark = 0;
	int count = 0;
	size_t i;
	for (i = 0; i < length; i++) {
		char c = line[i];
		if (c == ' ' || c == '\t') continue;
		if (c != '-' && c != '*' && c != '_') return R_FALSE;
		if (mark && c != mark) return R_FALSE;
		mark = c;
		count++;
	}
	return count >= 3;
}

/* "|---|:-:|--:|": the line under the header of a table. Counts its columns. */
static int rvn_md_is_table_rule(const char* line, size_t length, int* columns) {
	size_t i = 0;
	int count = 0, dashes = 0, any_pipe = R_FALSE;
	while (i < length && (line[i] == ' ' || line[i] == '\t')) i++;
	if (i < length && line[i] == '|') {
		any_pipe = R_TRUE;
		i++;
	}
	for (;;) {
		dashes = 0;
		while (i < length && (line[i] == ' ' || line[i] == '\t')) i++;
		if (i < length && line[i] == ':') i++;
		while (i < length && line[i] == '-') {
			dashes++;
			i++;
		}
		if (i < length && line[i] == ':') i++;
		while (i < length && (line[i] == ' ' || line[i] == '\t')) i++;
		if (dashes == 0) break;
		count++;
		if (i < length && line[i] == '|') {
			any_pipe = R_TRUE;
			i++;
			continue;
		}
		break;
	}
	while (i < length && (line[i] == ' ' || line[i] == '\t')) i++;
	if (i != length || count == 0 || !any_pipe) return R_FALSE;
	*columns = count;
	return R_TRUE;
}

/* Cuts a row of a table into cells; calls back for each, at most `max`. Returns their number. */
static int rvn_md_split_row(const char* line, size_t length, const char** starts, size_t* lengths, int max) {
	size_t i = 0, cell_start;
	int count = 0;
	while (i < length && (line[i] == ' ' || line[i] == '\t')) i++;
	if (i < length && line[i] == '|') i++;
	cell_start = i;
	for (; i <= length; i++) {
		int at_end = i == length;
		if (!at_end && line[i] == '\\' && i + 1 < length) {
			i++;
			continue;
		}
		if (at_end || line[i] == '|') {
			size_t a = cell_start, b = i;
			while (a < b && (line[a] == ' ' || line[a] == '\t')) a++;
			while (b > a && (line[b - 1] == ' ' || line[b - 1] == '\t')) b--;
			/* (the pipe that closes the row is not followed by a cell) */
			if (at_end && a == b && count > 0) break;
			if (count < max) {
				starts[count] = line + a;
				lengths[count] = b - a;
			}
			count++;
			cell_start = i + 1;
		}
	}
	return count;
}

static void rvn_md_cell_align(const char* rule, size_t length, int column, uint8_t* align) {
	const char* starts[64];
	size_t lengths[64];
	int count = rvn_md_split_row(rule, length, starts, lengths, 64);
	*align = 0;
	if (column < count && column < 64 && lengths[column] > 0) {
		int left = starts[column][0] == ':', right = starts[column][lengths[column] - 1] == ':';
		*align = (uint8_t)(left && right ? 1 : (right ? 2 : 0));
	}
}

#define RVN_MD_MAX_COLUMNS 32

/*
 * Text written inside an XML element is indented with the element. Returns
 * the text without the indentation its lines share (tabs count as one), to
 * release with rvn_free; NULL when there is none to remove.
 */
static char* rvn_md_dedent(const char* source) {
	const char* p = source;
	size_t common = (size_t)-1;
	rvn_text_t out;
	while (*p) {
		const char* line_end = strchr(p, '\n');
		size_t indent = 0;
		if (!line_end) line_end = p + strlen(p);
		while (p + indent < line_end && (p[indent] == ' ' || p[indent] == '\t')) indent++;
		/* (blank lines say nothing about the indentation; neither does the first line, which follows the tag) */
		if (p + indent < line_end && p != source && indent < common) common = indent;
		p = *line_end ? line_end + 1 : line_end;
	}
	if (common == (size_t)-1 || common == 0) return NULL;
	memset(&out, 0, sizeof(out));
	p = source;
	while (*p) {
		const char* line_end = strchr(p, '\n');
		size_t indent = 0;
		if (!line_end) line_end = p + strlen(p);
		while (indent < common && p + indent < line_end && (p[indent] == ' ' || p[indent] == '\t')) indent++;
		rvn_text_append(&out, p + indent, (size_t)(line_end - p - indent));
		if (*line_end) rvn_text_append(&out, "\n", 1);
		p = *line_end ? line_end + 1 : line_end;
	}
	return out.data;
}

rvn_md_t* rvn_md_parse(const char* source, int math) {
	rvn_md_t* md = (rvn_md_t*)rvn_calloc(sizeof(rvn_md_t));
	rvn_md_parser_t parser;
	char* dedented = source ? rvn_md_dedent(source) : NULL;
	const char* p = dedented ? dedented : (source ? source : "");
	const char* end = p + strlen(p);
	/* code between fences */
	int in_code = R_FALSE, fence_length = 0, fence_indent = 0;
	char fence_mark = 0;
	rvn_md_block_t* code = NULL;
	int blank_before = R_TRUE;

	memset(&parser, 0, sizeof(parser));
	parser.md = md;
	md->math = math != 0;
	/* (offset 0 of the text is never the start of a span that matters: keeps "0 length = none" simple) */
	rvn_text_append(&md->text, " ", 1);

	while (p < end) {
		const char* line = p;
		const char* line_end = (const char*)memchr(p, '\n', (size_t)(end - p));
		const char* next = line_end ? line_end + 1 : end;
		const char* q;
		size_t length;
		int indent = 0, quote = 0, columns = 0;

		if (!line_end) line_end = end;
		if (line_end > line && line_end[-1] == '\r') line_end--;
		p = next;

		/* the quotes the line is in */
		q = line;
		for (;;) {
			const char* mark = q;
			int spaces = 0;
			while (mark < line_end && *mark == ' ' && spaces < 3) {
				mark++;
				spaces++;
			}
			if (mark < line_end && *mark == '>' && !in_code) {
				quote++;
				q = mark + 1;
				if (q < line_end && *q == ' ') q++;
				continue;
			}
			break;
		}
		line = q;
		while (q < line_end && (*q == ' ' || *q == '\t')) {
			indent += *q == '\t' ? 4 : 1;
			q++;
		}
		length = (size_t)(line_end - q);

		if (in_code) {
			size_t run = 0, rest;
			while (run < length && q[run] == fence_mark) run++;
			rest = run;
			while (rest < length && (q[rest] == ' ' || q[rest] == '\t')) rest++;
			/* the fence that closes: at least as long as the one that opened, alone on its line */
			if (run >= (size_t)fence_length && rest == length) {
				in_code = R_FALSE;
			} else {
				/* the line as written, less the indentation of the fence */
				const char* raw = line;
				int skip = fence_indent;
				while (skip > 0 && raw < line_end && *raw == ' ') {
					raw++;
					skip--;
				}
				if (code->code_length > 0) rvn_text_append(&md->text, "\n", 1);
				rvn_text_append(&md->text, raw, (size_t)(line_end - raw));
				code->code_length = (uint32_t)md->text.length - code->code_start;
			}
			if (!in_code) {
				/* (an empty block still shows as one empty line) */
				blank_before = R_FALSE;
			}
			continue;
		}

		if (length == 0) {
			rvn_md_end_paragraph(&parser);
			blank_before = R_TRUE;
			continue;
		}
		if (quote != parser.quote) {
			rvn_md_end_paragraph(&parser);
			/* (a quote starts over: the lists that were open are not its lists) */
			if (blank_before || quote > parser.quote) {
				parser.depth = 0;
				parser.list = 0;
			}
			parser.quote = (uint8_t)rvn_mini(quote, 8);
		}

		/* fenced code */
		if ((q[0] == '`' || q[0] == '~') && length >= 3) {
			int run = 0;
			while ((size_t)run < length && q[run] == q[0]) run++;
			if (run >= 3 && (q[0] == '~' || !memchr(q + run, '`', length - (size_t)run))) {
				const char* info = q + run;
				const char* info_end;
				rvn_md_end_paragraph(&parser);
				while (info < line_end && (*info == ' ' || *info == '\t')) info++;
				info_end = info;
				while (info_end < line_end && !rvn_md_is_space(*info_end) && *info_end != '{' && *info_end != ',') info_end++;
				code = rvn_md_add_block(md, RVN_MD_CODE_BLOCK);
				code->quote = parser.quote;
				code->list = indent >= 2 && parser.depth > 0 ? parser.list : 0;
				code->language_start = (uint32_t)md->text.length;
				code->language_length = (uint32_t)(info_end - info);
				rvn_text_append(&md->text, info, (size_t)(info_end - info));
				rvn_text_append(&md->text, "\n", 1);
				code->code_start = (uint32_t)md->text.length;
				in_code = R_TRUE;
				fence_mark = q[0];
				fence_length = run;
				fence_indent = indent;
				blank_before = R_FALSE;
				continue;
			}
		}

		/* a title */
		if (q[0] == '#') {
			int level = 0;
			while ((size_t)level < length && q[level] == '#') level++;
			if (level <= 6 && ((size_t)level == length || q[level] == ' ' || q[level] == '\t')) {
				const char* text = q + level;
				const char* text_end = line_end;
				while (text < text_end && (*text == ' ' || *text == '\t')) text++;
				/* closing marks: "## Title ##" */
				while (text_end > text && (text_end[-1] == ' ' || text_end[-1] == '\t')) text_end--;
				{
					const char* hashes = text_end;
					while (hashes > text && hashes[-1] == '#') hashes--;
					if (hashes < text_end && (hashes == text || hashes[-1] == ' ')) text_end = hashes;
				}
				while (text_end > text && (text_end[-1] == ' ' || text_end[-1] == '\t')) text_end--;
				rvn_md_end_paragraph(&parser);
				parser.depth = 0;
				parser.list = 0;
				parser.heading = (uint8_t)level;
				rvn_md_paragraph_line(&parser, text, (size_t)(text_end - text));
				rvn_md_end_paragraph(&parser);
				blank_before = R_FALSE;
				continue;
			}
		}

		/* a rule (a line of dashes right under a paragraph makes it a title instead) */
		if (rvn_md_is_rule(q, length)) {
			if (parser.paragraph_open && q[0] == '-' && !memchr(q, ' ', length)) {
				parser.heading = 2;
				rvn_md_end_paragraph(&parser);
			} else {
				rvn_md_block_t* rule;
				rvn_md_end_paragraph(&parser);
				parser.depth = 0;
				parser.list = 0;
				rule = rvn_md_add_block(md, RVN_MD_RULE);
				rule->quote = parser.quote;
			}
			blank_before = R_FALSE;
			continue;
		}
		if (parser.paragraph_open && length >= 2 && q[0] == '=' && strspn(q, "= \t") >= length) {
			parser.heading = 1;
			rvn_md_end_paragraph(&parser);
			blank_before = R_FALSE;
			continue;
		}

		/* a table: a row, then the line of dashes */
		if (!parser.paragraph_open && memchr(q, '|', length) && next < end) {
			const char* rule = next;
			const char* rule_end = (const char*)memchr(rule, '\n', (size_t)(end - rule));
			const char* rule_text;
			int rule_quote = 0;
			if (!rule_end) rule_end = end;
			if (rule_end > rule && rule_end[-1] == '\r') rule_end--;
			/* (inside a quote, the rule line has its marks too) */
			rule_text = rule;
			while (rule_quote < quote && rule_text < rule_end) {
				while (rule_text < rule_end && *rule_text == ' ') rule_text++;
				if (rule_text < rule_end && *rule_text == '>') {
					rule_text++;
					rule_quote++;
				} else {
					break;
				}
			}
			if (rule_quote == quote && rvn_md_is_table_rule(rule_text, (size_t)(rule_end - rule_text), &columns)) {
				const char* starts[RVN_MD_MAX_COLUMNS];
				size_t lengths[RVN_MD_MAX_COLUMNS];
				rvn_md_block_t* table;
				uint32_t table_index;
				const char* row = q;
				size_t row_length = length;
				int c, rows = 0;
				if (columns > RVN_MD_MAX_COLUMNS) columns = RVN_MD_MAX_COLUMNS;
				table = rvn_md_add_block(md, RVN_MD_TABLE);
				table_index = md->block_count - 1;
				table->quote = parser.quote;
				table->columns = (uint16_t)columns;
				table->first_cell = md->cell_count;
				p = rule_end < end ? rule_end + 1 : end;
				for (;;) {
					int found = rvn_md_split_row(row, row_length, starts, lengths, RVN_MD_MAX_COLUMNS);
					RVN_ARRAY_RESERVE(md->cells, md->cell_cap, md->cell_count + (uint32_t)columns);
					for (c = 0; c < columns; c++) {
						rvn_md_cell_t* cell = &md->cells[md->cell_count++];
						memset(cell, 0, sizeof(*cell));
						rvn_md_cell_align(rule_text, (size_t)(rule_end - rule_text), c, &cell->align);
						if (c < found) rvn_md_read_inline(md, starts[c], lengths[c], &cell->first_span, &cell->span_count);
					}
					rows++;
					/* the next row, as long as there is one */
					if (p >= end) break;
					{
						const char* row_end = (const char*)memchr(p, '\n', (size_t)(end - p));
						const char* text = p;
						int row_quote = 0;
						if (!row_end) row_end = end;
						while (row_quote < quote && text < row_end) {
							while (text < row_end && *text == ' ') text++;
							if (text < row_end && *text == '>') {
								text++;
								row_quote++;
							} else {
								break;
							}
						}
						while (text < row_end && (*text == ' ' || *text == '\t')) text++;
						row_length = (size_t)(row_end - text);
						if (row_length > 0 && text[row_length - 1] == '\r') row_length--;
						if (row_quote != quote || row_length == 0 || !memchr(text, '|', row_length)) break;
						row = text;
						p = row_end < end ? row_end + 1 : end;
					}
				}
				md->blocks[table_index].rows = (uint16_t)rows;
				blank_before = R_FALSE;
				continue;
			}
		}

		/* a list item */
		{
			const char* text = NULL;
			int marker = RVN_MD_NO_MARKER, number = 0;
			if ((q[0] == '-' || q[0] == '*' || q[0] == '+') && length >= 2 && (q[1] == ' ' || q[1] == '\t')) {
				marker = RVN_MD_BULLET;
				text = q + 2;
			} else if (q[0] >= '0' && q[0] <= '9') {
				const char* digit = q;
				while (digit < line_end && *digit >= '0' && *digit <= '9' && digit - q < 9) {
					number = number * 10 + (*digit - '0');
					digit++;
				}
				if (digit + 1 < line_end && (*digit == '.' || *digit == ')') && (digit[1] == ' ' || digit[1] == '\t')) {
					marker = RVN_MD_NUMBER;
					text = digit + 2;
				}
			}
			/* (a number in the middle of a paragraph does not start a list unless it is 1) */
			if (marker == RVN_MD_NUMBER && parser.paragraph_open && parser.depth == 0 && number != 1) marker = RVN_MD_NO_MARKER;
			if (marker != RVN_MD_NO_MARKER) {
				rvn_md_end_paragraph(&parser);
				while (parser.depth > 0 && parser.indents[parser.depth - 1] > indent) parser.depth--;
				if (parser.depth == 0 || indent >= parser.indents[parser.depth - 1] + 2) {
					if (parser.depth < 8) parser.indents[parser.depth++] = indent;
				}
				parser.list = (uint8_t)parser.depth;
				while (text < line_end && (*text == ' ' || *text == '\t')) text++;
				if (marker == RVN_MD_BULLET && line_end - text >= 3 && text[0] == '[' && text[2] == ']' &&
					(text[1] == ' ' || text[1] == 'x' || text[1] == 'X') && (line_end - text == 3 || text[3] == ' ')) {
					marker = text[1] == ' ' ? RVN_MD_TASK : RVN_MD_TASK_DONE;
					text += 3;
					while (text < line_end && (*text == ' ' || *text == '\t')) text++;
				}
				parser.marker = (uint8_t)marker;
				parser.number = number;
				rvn_md_paragraph_line(&parser, text, (size_t)(line_end - text));
				blank_before = R_FALSE;
				continue;
			}
		}

		/* text: more of the paragraph, or a new one */
		if (!parser.paragraph_open) {
			if (parser.depth > 0 && blank_before && indent < 2) {
				/* not indented after a blank line: the lists are over */
				parser.depth = 0;
				parser.list = 0;
			}
			parser.marker = RVN_MD_NO_MARKER;
		}
		/* (with the spaces at its end: two of them ask for a line break) */
		rvn_md_paragraph_line(&parser, q, length);
		blank_before = R_FALSE;
	}
	rvn_md_end_paragraph(&parser);
	rvn_free(parser.paragraph.data);
	rvn_free(dedented);
	md->parsed_length = md->text.length;
	return md;
}

void rvn_md_free(rvn_md_t* md) {
	uint32_t i;
	if (!md) return;
	for (i = 0; i < md->link_count; i++) rvn_free(md->links[i]);
	rvn_free(md->links);
	rvn_free(md->text.data);
	rvn_free(md->spans);
	rvn_free(md->blocks);
	rvn_free(md->cells);
	rvn_free(md->runs);
	rvn_free(md->boxes);
	rvn_free(md->maths);
	rvn_free(md);
}

/* ---------------------------------------------------------------------------
 *  Layout
 * ------------------------------------------------------------------------- */

typedef struct rvn_md_font_t {
	Rid id;
	rvn_font_t* font;
	/** From the top of its line box to its baseline, and the height of that box. */
	float base;
	float height;
	float size;
} rvn_md_font_t;

typedef struct rvn_md_layout_t {
	rvn_md_t* md;
	const rvn_widget_t* widget;
	Rid font;
	Rid mono;
	float size;
	/** Width of the widest thing placed so far. */
	float widest;
	/** Draws the formulas, or NULL (the sources are shown as text). */
	RMathCallback math;
	void* math_user;
} rvn_md_layout_t;

static void rvn_md_font(rvn_md_font_t* out, Rid id, float size) {
	rvn_font_metrics_t metrics;
	out->id = id;
	out->font = rvn_font_resolve(id);
	out->size = size;
	rvn_font_metrics(out->font, size, &metrics);
	out->height = metrics.line_height;
	out->base = metrics.ascent + (metrics.line_height - metrics.ascent - metrics.descent) * 0.5f;
}

static rvn_md_box_t* rvn_md_add_box(rvn_md_t* md, int kind, float x, float y, float w, float h) {
	rvn_md_box_t* box;
	RVN_ARRAY_RESERVE(md->boxes, md->box_cap, md->box_count + 1);
	box = &md->boxes[md->box_count++];
	box->kind = (uint8_t)kind;
	box->x = x;
	box->y = y;
	box->w = w;
	box->h = h;
	box->texture = R_INVALID_ID;
	return box;
}

static rvn_md_run_t* rvn_md_add_run(rvn_md_t* md) {
	rvn_md_run_t* run;
	RVN_ARRAY_RESERVE(md->runs, md->run_cap, md->run_count + 1);
	run = &md->runs[md->run_count++];
	memset(run, 0, sizeof(*run));
	run->link = -1;
	run->math = -1;
	return run;
}

/*
 * Asks the application for the picture of a formula, at a size (in em: the
 * box it gives is in em too, and is multiplied by the size of the text here).
 */
static void rvn_md_math_draw(rvn_md_layout_t* layout, rvn_md_math_t* math, float size) {
	RMathBox box;
	math->size = size;
	math->failed = R_TRUE;
	if (!layout->math) return;
	memset(&box, 0, sizeof(box));
	box.texture = R_INVALID_ID;
	if (!layout->math(layout->widget->id, layout->md->text.data + math->start, (int)math->length, size, math->display, &box, layout->math_user)) return;
	if (box.texture == R_INVALID_ID || box.width <= 0.0f || box.ascent + box.descent <= 0.0f) return;
	math->texture = box.texture;
	math->width = box.width * size;
	math->ascent = box.ascent * size;
	math->height = (box.ascent + box.descent) * size;
	math->failed = R_FALSE;
}

/*
 * The formula whose source is [start, start + length), drawn at `size`. The
 * pictures are kept from one layout to the next: a label lays out again at
 * every width, and the application is not asked twice for the same box.
 */
static const rvn_md_math_t* rvn_md_math_resolve(rvn_md_layout_t* layout, uint32_t start, uint32_t length, int display, float size) {
	rvn_md_t* md = layout->md;
	rvn_md_math_t* math;
	uint32_t i;
	/* (half sizes only: a label that shrinks a formula to fit asks for a few of them, not for hundreds) */
	float asked = floorf(size * 2.0f + 0.5f) * 0.5f;
	for (i = 0; i < md->math_count; i++) {
		math = &md->maths[i];
		if (math->start != start || math->length != length) continue;
		if (math->size != asked) rvn_md_math_draw(layout, math, asked);
		return math;
	}
	RVN_ARRAY_RESERVE(md->maths, md->math_cap, md->math_count + 1);
	math = &md->maths[md->math_count++];
	memset(math, 0, sizeof(*math));
	math->start = start;
	math->length = length;
	math->display = display;
	math->texture = R_INVALID_ID;
	rvn_md_math_draw(layout, math, asked);
	return math;
}

/* The application drew the formulas differently: they are asked for again. */
void rvn_md_forget_math(rvn_md_t* md) {
	if (!md) return;
	md->math_count = 0;
	md->laid = R_FALSE;
}

/* Ends a line of a flow: gives the runs since `first` their height and their place across. */
static float rvn_md_end_line(rvn_md_layout_t* layout, uint32_t first, float x0, float y, float width, float pen, int align,
	float min_height, float min_base, const float* bases, const float* heights) {

	rvn_md_t* md = layout->md;
	float height = min_height, base = min_base, shift = 0.0f, leading;
	uint32_t i;
	for (i = first; i < md->run_count; i++) {
		base = rvn_maxf(base, bases[i - first]);
		height = rvn_maxf(height, heights[i - first]);
	}
	/* air between the lines */
	leading = floorf(height * 0.26f + 0.5f);
	height = ceilf(height) + leading;
	if (width > 0.0f && align) shift = rvn_maxf(0.0f, (width - pen) * (align == 1 ? 0.5f : 1.0f));
	for (i = first; i < md->run_count; i++) {
		rvn_md_run_t* run = &md->runs[i];
		run->x += shift;
		run->y = y + leading * 0.5f + (base - bases[i - first]);
		run->line_y = y;
		run->line_h = height;
		if (run->math >= 0) {
			/* the formula: its picture, where the run sits, on the baseline of the line */
			const rvn_md_math_t* math = &md->maths[run->math];
			rvn_md_box_t* box = rvn_md_add_box(md, RVN_MD_BOX_MATH, run->x, run->y, math->width, math->height);
			box->texture = math->texture;
		} else if (run->flags & RVN_MD_CODE) {
			/* the tile behind a piece of code */
			rvn_md_add_box(md, RVN_MD_BOX_INLINE_CODE, run->x - 3.0f, y + leading * 0.5f - 1.0f, run->w + 6.0f, height - leading + 2.0f);
		}
	}
	layout->widest = rvn_maxf(layout->widest, x0 + pen + shift);
	return height;
}

#define RVN_MD_LINE_RUNS 256

/*
 * Lays spans out from (x0, y), wrapped on `width` (0: on one line). Returns
 * the height it took.
 */
static float rvn_md_flow(rvn_md_layout_t* layout, uint32_t first_span, uint32_t span_count, float x0, float y, float width,
	float size, uint32_t forced, int align, int color) {

	rvn_md_t* md = layout->md;
	const char* text = md->text.data;
	rvn_md_font_t fonts[4], mono, plain;
	float bases[RVN_MD_LINE_RUNS], heights[RVN_MD_LINE_RUNS];
	uint32_t line_first = md->run_count, s;
	float pen = 0.0f, top = y;
	int made[4] = { 0, 0, 0, 0 };

	rvn_md_font(&plain, rvn_font_variant(layout->font, forced), size);
	rvn_md_font(&mono, layout->mono, size * 0.94f);

	for (s = 0; s < span_count; s++) {
		const rvn_md_span_t* span = &md->spans[first_span + s];
		uint32_t style = ((span->flags & RVN_MD_BOLD) ? RVN_FONT_BOLD : 0u) | ((span->flags & RVN_MD_ITALIC) ? RVN_FONT_ITALIC : 0u) | forced;
		const rvn_md_font_t* font;
		uint32_t at = span->start, stop = span->start + span->length;
		rvn_md_run_t* run = NULL;

		if (span->flags & RVN_MD_CODE) {
			font = &mono;
		} else {
			if (!made[style]) {
				rvn_md_font(&fonts[style], rvn_font_variant(layout->font, style), size);
				made[style] = R_TRUE;
			}
			font = &fonts[style];
		}

		/* a formula is one word: it goes to the next line whole, and is never cut */
		if (span->flags & RVN_MD_MATH) {
			const rvn_md_math_t* math = rvn_md_math_resolve(layout, span->start, span->length, (span->flags & RVN_MD_DISPLAY) != 0, size);
			if (math && !math->failed) {
				float word_width = math->width;
				/* (wider than the text can be: the application draws it again, smaller) */
				if (width > 0.0f && word_width > width) {
					const rvn_md_math_t* smaller = rvn_md_math_resolve(layout, span->start, span->length, (span->flags & RVN_MD_DISPLAY) != 0, size * rvn_maxf(width / word_width, 0.5f));
					if (smaller && !smaller->failed) {
						math = smaller;
						word_width = math->width;
					}
				}
				if (width > 0.0f && pen > 0.0f && pen + word_width > width + 0.5f) {
					y += rvn_md_end_line(layout, line_first, x0, y, width, pen, align, plain.height, plain.base, bases, heights);
					line_first = md->run_count;
					pen = 0.0f;
					run = NULL;
				}
				if (md->run_count - line_first >= RVN_MD_LINE_RUNS) {
					y += rvn_md_end_line(layout, line_first, x0, y, width, pen, align, plain.height, plain.base, bases, heights);
					line_first = md->run_count;
					pen = 0.0f;
					run = NULL;
				}
				run = rvn_md_add_run(md);
				run->x = x0 + pen;
				run->start = span->start;
				run->math = (int16_t)(math - md->maths);
				run->w = word_width;
				run->font = font->id;
				run->size = size;
				run->link = span->link;
				run->color = (uint8_t)(span->link >= 0 ? RVN_MD_COLOR_LINK : color);
				/* (the box it takes, above and below the baseline of the line: see rvn_md_end_line) */
				bases[md->run_count - 1 - line_first] = math->ascent;
				heights[md->run_count - 1 - line_first] = math->height;
				pen += word_width;
				at = stop;
				run = NULL;
			}
		}

		while (at < stop) {
			/* a word and the spaces after it */
			uint32_t word_end = at, next;
			float word_width, full_width;
			while (word_end < stop && text[word_end] != ' ') word_end++;
			next = word_end;
			while (next < stop && text[next] == ' ') next++;
			word_width = rvn_text_width(font->font, font->size, text + at, (int)(word_end - at));
			full_width = word_width + rvn_text_width(font->font, font->size, text + word_end, (int)(next - word_end));

			if (width > 0.0f && pen > 0.0f && pen + word_width > width + 0.5f) {
				y += rvn_md_end_line(layout, line_first, x0, y, width, pen, align, plain.height, plain.base, bases, heights);
				line_first = md->run_count;
				pen = 0.0f;
				run = NULL;
				/* (the spaces a line was cut on do not start the next one) */
				if (word_end == at) {
					at = next;
					continue;
				}
			}
			if (!run || md->run_count - line_first >= RVN_MD_LINE_RUNS) {
				if (md->run_count - line_first >= RVN_MD_LINE_RUNS) {
					y += rvn_md_end_line(layout, line_first, x0, y, width, pen, align, plain.height, plain.base, bases, heights);
					line_first = md->run_count;
					pen = 0.0f;
				}
				run = rvn_md_add_run(md);
				run->x = x0 + pen;
				run->start = at;
				run->font = font->id;
				run->size = font->size;
				run->flags = (uint8_t)(span->flags & (RVN_MD_CODE | RVN_MD_STRIKE));
				run->link = span->link;
				run->color = (uint8_t)(span->link >= 0 ? RVN_MD_COLOR_LINK : color);
				bases[md->run_count - 1 - line_first] = font->base;
				heights[md->run_count - 1 - line_first] = font->height;
			}
			run->length += next - at;
			run->w += full_width;
			pen += full_width;
			at = next;
		}
		if (span->flags & RVN_MD_BREAK) {
			y += rvn_md_end_line(layout, line_first, x0, y, width, pen, align, plain.height, plain.base, bases, heights);
			line_first = md->run_count;
			pen = 0.0f;
		}
	}
	if (md->run_count > line_first || y == top) {
		y += rvn_md_end_line(layout, line_first, x0, y, width, pen, align, plain.height, plain.base, bases, heights);
	}
	return y - top;
}

/* Width spans take on one line. */
static float rvn_md_natural_width(rvn_md_layout_t* layout, uint32_t first_span, uint32_t span_count, float size, uint32_t forced) {
	rvn_md_t* md = layout->md;
	float width = 0.0f;
	uint32_t s;
	for (s = 0; s < span_count; s++) {
		const rvn_md_span_t* span = &md->spans[first_span + s];
		uint32_t style = ((span->flags & RVN_MD_BOLD) ? RVN_FONT_BOLD : 0u) | ((span->flags & RVN_MD_ITALIC) ? RVN_FONT_ITALIC : 0u) | forced;
		rvn_font_t* font = rvn_font_resolve((span->flags & RVN_MD_CODE) ? layout->mono : rvn_font_variant(layout->font, style));
		if (span->flags & RVN_MD_MATH) {
			/* (a column of a table is as wide as its formulas, or they would be cut) */
			const rvn_md_math_t* math = rvn_md_math_resolve(layout, span->start, span->length, (span->flags & RVN_MD_DISPLAY) != 0, size);
			if (math && !math->failed) {
				width += math->width;
				continue;
			}
		}
		width += rvn_text_width(font, (span->flags & RVN_MD_CODE) ? size * 0.94f : size, md->text.data + span->start, (int)span->length);
	}
	return width;
}

static float rvn_md_code_block(rvn_md_layout_t* layout, const rvn_md_block_t* block, float x0, float y, float width) {
	rvn_md_t* md = layout->md;
	const char* text = md->text.data;
	rvn_md_font_t mono;
	const rvn_language_t* language = NULL;
	char name[32];
	float pad_x = floorf(layout->size * 0.8f), pad_y = floorf(layout->size * 0.6f);
	float top = y, widest = 0.0f, line_height;
	uint32_t at = block->code_start, stop = block->code_start + block->code_length;
	uint32_t box_index = md->box_count;
	uint8_t state = 0;

	rvn_md_font(&mono, layout->mono, layout->size * 0.94f);
	line_height = ceilf(mono.height * 1.18f);
	if (block->language_length > 0 && block->language_length < sizeof(name)) {
		memcpy(name, text + block->language_start, block->language_length);
		name[block->language_length] = '\0';
		language = rvn_language_find(name);
	}
	rvn_md_add_box(md, RVN_MD_BOX_CODE, x0, y, 0.0f, 0.0f);
	y += pad_y;
	for (;;) {
		uint32_t line_end = at;
		rvn_span_t spans[96];
		int span_count = 0, i, cursor = 0, length;
		float pen = 0.0f;
		while (line_end < stop && text[line_end] != '\n') line_end++;
		length = (int)(line_end - at);
		if (language) state = rvn_syntax_line(language, text + at, length, state, spans, 96, &span_count);
		/* the line, cut where its color changes */
		for (i = 0; i <= span_count; i++) {
			int piece_start = i < span_count ? spans[i].start : length;
			int piece_end = i < span_count ? spans[i].end : length;
			int k;
			for (k = 0; k < 2; k++) {
				int a = k == 0 ? cursor : piece_start, b = k == 0 ? piece_start : piece_end;
				rvn_md_run_t* run;
				if (b <= a) continue;
				run = rvn_md_add_run(md);
				run->x = x0 + pad_x + pen;
				run->y = y + (line_height - mono.height) * 0.5f;
				run->line_y = y;
				run->line_h = line_height;
				run->start = at + (uint32_t)a;
				run->length = (uint32_t)(b - a);
				run->font = mono.id;
				run->size = mono.size;
				run->w = rvn_text_width(mono.font, mono.size, text + at + a, b - a);
				run->color = (uint8_t)(k == 0 || i >= span_count ? RVN_MD_COLOR_TEXT : RVN_MD_COLOR_TOKEN + spans[i].token);
				/* (set once the width of the block is known) */
				run->clip_right = -1.0f;
				pen += run->w;
			}
			cursor = piece_end;
		}
		widest = rvn_maxf(widest, pen);
		y += line_height;
		if (line_end >= stop) break;
		at = line_end + 1;
	}
	y += pad_y;
	{
		/* as wide as the text around it; without wrapping, as wide as its longest line */
		float box_width = width > 0.0f ? width : widest + pad_x * 2.0f;
		uint32_t i;
		md->boxes[box_index].w = box_width;
		md->boxes[box_index].h = y - top;
		for (i = 0; i < md->run_count; i++) {
			if (md->runs[i].clip_right < 0.0f) md->runs[i].clip_right = x0 + box_width - pad_x * 0.5f;
		}
		layout->widest = rvn_maxf(layout->widest, x0 + box_width);
	}
	return y - top;
}

static float rvn_md_table(rvn_md_layout_t* layout, const rvn_md_block_t* block, float x0, float y, float width) {
	rvn_md_t* md = layout->md;
	float widths[RVN_MD_MAX_COLUMNS];
	float pad_x = floorf(layout->size * 0.75f), pad_y = floorf(layout->size * 0.36f);
	float total = 0.0f, top = y, x;
	int columns = block->columns, rows = block->rows, r, c;

	if (columns <= 0 || rows <= 0) return 0.0f;
	for (c = 0; c < columns; c++) {
		widths[c] = layout->size * 2.0f;
		for (r = 0; r < rows; r++) {
			const rvn_md_cell_t* cell = &md->cells[block->first_cell + (uint32_t)(r * columns + c)];
			widths[c] = rvn_maxf(widths[c], rvn_md_natural_width(layout, cell->first_span, cell->span_count, layout->size, r == 0 ? RVN_FONT_BOLD : 0u) + 1.0f);
		}
		total += widths[c] + pad_x * 2.0f;
	}
	/* too wide: the columns give up room in proportion, and their text wraps */
	if (width > 0.0f && total > width) {
		float room = rvn_maxf(width - pad_x * 2.0f * (float)columns, layout->size * 2.0f * (float)columns);
		float sum = total - pad_x * 2.0f * (float)columns;
		total = 0.0f;
		for (c = 0; c < columns; c++) {
			widths[c] = rvn_maxf(layout->size * 2.0f, floorf(widths[c] * room / sum));
			total += widths[c] + pad_x * 2.0f;
		}
	}
	for (r = 0; r < rows; r++) {
		float row_height = 0.0f;
		uint32_t header_box = 0;
		if (r == 0) {
			header_box = md->box_count;
			rvn_md_add_box(md, RVN_MD_BOX_TABLE_HEADER, x0, y, total, 0.0f);
		}
		x = x0;
		for (c = 0; c < columns; c++) {
			const rvn_md_cell_t* cell = &md->cells[block->first_cell + (uint32_t)(r * columns + c)];
			float height = rvn_md_flow(layout, cell->first_span, cell->span_count, x + pad_x, y + pad_y, widths[c], layout->size,
				r == 0 ? RVN_FONT_BOLD : 0u, cell->align, RVN_MD_COLOR_TEXT);
			row_height = rvn_maxf(row_height, height);
			x += widths[c] + pad_x * 2.0f;
		}
		row_height += pad_y * 2.0f;
		if (r == 0) md->boxes[header_box].h = row_height;
		y += row_height;
		rvn_md_add_box(md, RVN_MD_BOX_TABLE_LINE, x0, y - 1.0f, total, 1.0f);
	}
	rvn_md_add_box(md, RVN_MD_BOX_TABLE_LINE, x0, top, total, 1.0f);
	/* the lines between the columns, and around */
	x = x0;
	for (c = 0; c <= columns; c++) {
		rvn_md_add_box(md, RVN_MD_BOX_TABLE_LINE, c == columns ? x - 1.0f : x, top, 1.0f, y - top);
		if (c < columns) x += widths[c] + pad_x * 2.0f;
	}
	layout->widest = rvn_maxf(layout->widest, x0 + total);
	return y - top;
}

/* The span of a paragraph that holds its formula ($$...$$), or NULL. */
static const rvn_md_span_t* rvn_md_math_span(const rvn_md_t* md, const rvn_md_block_t* block) {
	uint32_t s;
	for (s = 0; s < block->span_count; s++) {
		const rvn_md_span_t* span = &md->spans[block->first_span + s];
		if (span->flags & RVN_MD_MATH) return span;
	}
	return NULL;
}

rvn_vec2_t rvn_md_layout(rvn_md_t* md, const rvn_widget_t* widget, Rid font, float size, float width,
	RMathCallback math, void* math_user) {
	static const float heading_scale[7] = { 1.0f, 1.9f, 1.52f, 1.26f, 1.1f, 1.0f, 0.92f };
	rvn_md_layout_t layout;
	float y = 0.0f, gap = 0.0f;
	float quote_indent, list_indent;
	uint32_t b;
	uint8_t open_quote = 0;
	float quote_top[9];
	rvn_font_t* resolved = rvn_font_resolve(font);

	if (!resolved) return rvn_vec2(0.0f, 0.0f);
	if (md->laid && md->laid_width == width && md->laid_size == size && md->laid_font == font &&
		md->laid_generation == g_rvn.style_generation) return md->size;
	md->run_count = 0;
	md->box_count = 0;
	md->text.length = md->parsed_length;
	layout.md = md;
	layout.widget = widget;
	layout.font = font;
	layout.mono = g_rvn.mono_font;
	layout.size = size;
	layout.widest = 0.0f;
	layout.math = md->math ? math : NULL;
	layout.math_user = math_user;
	quote_indent = floorf(size * 1.15f);
	list_indent = floorf(size * 1.7f);

	for (b = 0; b <= md->block_count; b++) {
		const rvn_md_block_t* block = b < md->block_count ? &md->blocks[b] : NULL;
		uint8_t quote = block ? block->quote : 0;
		float x0, room, height = 0.0f, before;

		/* the bars of the quotes that end here */
		while (open_quote > quote) {
			float top = quote_top[open_quote];
			rvn_md_add_box(md, RVN_MD_BOX_QUOTE, quote_indent * (float)(open_quote - 1), top, 3.0f, y - top);
			open_quote--;
		}
		if (!block) break;

		/* the space above: more before a title, less between the items of a list */
		before = size * 0.72f;
		if (block->type == RVN_MD_HEADING) before = size * (block->level <= 2 ? 1.25f : 1.0f);
		else if (block->type == RVN_MD_MATH_BLOCK) before = size * 0.9f;
		else if (block->list > 0 && b > 0 && md->blocks[b - 1].list > 0) before = size * (block->marker ? 0.3f : 0.4f);
		if (b > 0 && md->blocks[b - 1].type == RVN_MD_HEADING) before = size * 0.5f;
		if (b == 0) before = 0.0f;
		y += rvn_maxf(before, gap);
		gap = 0.0f;
		while (open_quote < quote) quote_top[++open_quote] = y;

		x0 = quote_indent * (float)quote + list_indent * (float)block->list;
		room = width > 0.0f ? rvn_maxf(width - x0, size * 4.0f) : 0.0f;

		switch (block->type) {
			case RVN_MD_HEADING: {
				float heading_size = floorf(size * heading_scale[rvn_clampi(block->level, 1, 6)] + 0.5f);
				height = rvn_md_flow(&layout, block->first_span, block->span_count, x0, y, room, heading_size, RVN_FONT_BOLD, 0, RVN_MD_COLOR_TEXT);
				/* the two largest are underlined across */
				if (block->level <= 2) {
					height += floorf(size * 0.3f);
					rvn_md_add_box(md, RVN_MD_BOX_RULE, x0, y + height, room > 0.0f ? room : -1.0f, 1.0f);
					height += 1.0f;
				}
				break;
			}
			case RVN_MD_CODE_BLOCK:
				height = rvn_md_code_block(&layout, block, x0, y, room);
				break;
			case RVN_MD_RULE:
				rvn_md_add_box(md, RVN_MD_BOX_RULE, x0, y + floorf(size * 0.3f), room > 0.0f ? room : -1.0f, 1.0f);
				height = floorf(size * 0.6f) + 1.0f;
				break;
			case RVN_MD_TABLE:
				height = rvn_md_table(&layout, block, x0, y, room);
				break;
			case RVN_MD_IMAGE: {
				Rid texture = block->link >= 0 ? rvn_xml_texture(md->links[block->link]) : R_INVALID_ID;
				int image_width = 0, image_height = 0;
				if (texture != R_INVALID_ID) Rtexture_get_size(texture, &image_width, &image_height);
				if (image_width > 0 && image_height > 0) {
					float w = (float)image_width, h = (float)image_height;
					rvn_md_box_t* box;
					if (room > 0.0f && w > room) {
						h = floorf(h * room / w);
						w = room;
					}
					box = rvn_md_add_box(md, RVN_MD_BOX_IMAGE, x0, y, w, h);
					box->texture = texture;
					layout.widest = rvn_maxf(layout.widest, x0 + w);
					height = h;
				} else {
					/* an image that cannot be shown is told by its alternative text */
					height = rvn_md_flow(&layout, block->first_span, block->span_count, x0, y, room, size, RVN_FONT_ITALIC, 0, RVN_MD_COLOR_QUOTE);
				}
				break;
			}
			case RVN_MD_MATH_BLOCK: {
				const rvn_md_span_t* span = rvn_md_math_span(md, block);
				const rvn_md_math_t* math = span ? rvn_md_math_resolve(&layout, span->start, span->length, R_TRUE, size) : NULL;
				float box_width = math ? math->width : 0.0f, x = x0;
				/* (wider than the column: the application draws it again, smaller) */
				if (math && !math->failed && room > 0.0f && box_width > room) {
					const rvn_md_math_t* smaller = rvn_md_math_resolve(&layout, span->start, span->length, R_TRUE, size * rvn_maxf(room / box_width, 0.5f));
					if (smaller && !smaller->failed) {
						math = smaller;
						box_width = math->width;
					}
				}
				if (math && !math->failed) {
					rvn_md_box_t* box;
					float top = floorf(size * 0.25f);
					if (room > 0.0f) x = x0 + floorf((room - box_width) * 0.5f);
					box = rvn_md_add_box(md, RVN_MD_BOX_MATH, x, y + top, box_width, math->height);
					box->texture = math->texture;
					layout.widest = rvn_maxf(layout.widest, x + box_width);
					height = top + math->height;
				} else {
					/* (a formula that cannot be drawn is told by its source, as an image by its alternative text) */
					height = rvn_md_flow(&layout, block->first_span, block->span_count, x0, y, room, size, RVN_FONT_ITALIC, 0, RVN_MD_COLOR_QUOTE);
				}
				break;
			}
			case RVN_MD_PARAGRAPH:
			default:
				if (block->marker != RVN_MD_NO_MARKER) {
					rvn_md_font_t regular;
					float marker_x = x0 - list_indent;
					rvn_md_font(&regular, font, size);
					if (block->marker == RVN_MD_NUMBER) {
						/* the number, right against the text */
						char number[16];
						uint32_t start = (uint32_t)md->text.length;
						rvn_md_run_t* run;
						float number_width;
						snprintf(number, sizeof(number), "%d.", block->number);
						rvn_text_add(&md->text, number);
						number_width = rvn_text_width(regular.font, size, number, -1);
						run = rvn_md_add_run(md);
						run->x = x0 - number_width - floorf(size * 0.4f);
						run->y = y + floorf(regular.height * 0.13f + 0.5f);
						run->line_y = y;
						run->line_h = regular.height;
						run->w = number_width;
						run->start = start;
						run->length = (uint32_t)strlen(number);
						run->font = regular.id;
						run->size = size;
						run->color = RVN_MD_COLOR_QUOTE;
					} else {
						float line = ceilf(regular.height) + floorf(regular.height * 0.26f + 0.5f);
						float side = block->marker == RVN_MD_BULLET ? floorf(size * 0.36f) : floorf(size * 0.95f);
						int kind = block->marker == RVN_MD_BULLET ? RVN_MD_BOX_BULLET : (block->marker == RVN_MD_TASK ? RVN_MD_BOX_TASK : RVN_MD_BOX_TASK_DONE);
						/* (the bullets of lists in lists are smaller rings: see the painting) */
						rvn_md_box_t* box = rvn_md_add_box(md, kind, marker_x + floorf((list_indent - side) * 0.55f), y + floorf((line - side) * 0.5f), side, side);
						box->texture = (Rid)block->list;
					}
				}
				height = rvn_md_flow(&layout, block->first_span, block->span_count, x0, y, room, size, 0u, 0,
					quote > 0 ? RVN_MD_COLOR_QUOTE : RVN_MD_COLOR_TEXT);
				break;
		}
		y += height;
	}

	/* rules asked to span the text: now that its width is known */
	{
		uint32_t i;
		float full = width > 0.0f ? width : layout.widest;
		for (i = 0; i < md->box_count; i++) {
			if (md->boxes[i].kind == RVN_MD_BOX_RULE && md->boxes[i].w < 0.0f) md->boxes[i].w = rvn_maxf(full - md->boxes[i].x, 0.0f);
		}
	}
	md->size = rvn_vec2(ceilf(layout.widest), ceilf(y));
	md->laid = R_TRUE;
	md->laid_width = width;
	md->laid_size = size;
	md->laid_font = font;
	md->laid_generation = g_rvn.style_generation;
	return md->size;
}

/* ---------------------------------------------------------------------------
 *  Painting and pointing
 * ------------------------------------------------------------------------- */

void rvn_md_paint(const rvn_md_t* md, const rvn_widget_t* widget, RPainter* painter, float ox, float oy, rvn_color_t color, int hot_link) {
	const rvn_theme_t* theme = rvn_widget_theme(widget);
	rvn_color_t link = painter->enabled ? rvn_widget_color(widget, R_COLOR_LINK) : color;
	rvn_color_t dim = rvn_color_fade(color, 0.72f);
	rvn_color_t line = rvn_widget_color(widget, R_COLOR_SEPARATOR);
	rvn_color_t code = rvn_widget_color(widget, R_COLOR_CODE);
	rvn_color_t accent = rvn_widget_color(widget, R_COLOR_ACCENT);
	uint32_t i;

	for (i = 0; i < md->box_count; i++) {
		const rvn_md_box_t* box = &md->boxes[i];
		rvn_rect_t rect = rvn_rect(ox + box->x, oy + box->y, box->w, box->h);
		switch (box->kind) {
			case RVN_MD_BOX_CODE:
				rvn_paint_box(painter, rect, 7.0f, code, 1.0f, rvn_color_fade(line, 0.6f));
				break;
			case RVN_MD_BOX_INLINE_CODE:
				rvn_paint_box(painter, rect, 4.0f, code, 0.0f, 0);
				break;
			case RVN_MD_BOX_QUOTE:
				rvn_paint_box(painter, rect, 1.5f, rvn_color_fade(accent, 0.75f), 0.0f, 0);
				break;
			case RVN_MD_BOX_RULE:
			case RVN_MD_BOX_TABLE_LINE:
				rvn_paint_rect(painter, rect, line);
				break;
			case RVN_MD_BOX_TABLE_HEADER:
				rvn_paint_rect(painter, rect, rvn_color_fade(code, 0.8f));
				break;
			case RVN_MD_BOX_BULLET:
				/* a disc, then rings for the lists inside lists */
				if ((uint32_t)box->texture <= 1u) rvn_paint_box(painter, rect, rect.w * 0.5f, dim, 0.0f, 0);
				else rvn_paint_box(painter, rect, rect.w * 0.5f, 0, 1.2f, dim);
				break;
			case RVN_MD_BOX_TASK:
				rvn_paint_box(painter, rect, 3.5f, 0, 1.3f, dim);
				break;
			case RVN_MD_BOX_TASK_DONE:
				rvn_paint_box(painter, rect, 3.5f, accent, 0.0f, 0);
				rvn_paint_icon(painter, RVN_ICON_CHECK, rect.x + rect.w * 0.5f, rect.y + rect.h * 0.5f, rect.w * 0.62f,
					rvn_widget_color(widget, R_COLOR_ACCENT_TEXT));
				break;
			case RVN_MD_BOX_IMAGE:
				rvn_paint_image(painter, rect, box->texture, NULL, rvn_hex(0xFFFFFF, 255), 6.0f);
				break;
			case RVN_MD_BOX_MATH:
				/* (drawn in white by the application: the color of the text tints it) */
				rvn_paint_image(painter, rect, box->texture, NULL, color, 0.0f);
				break;
			default:
				break;
		}
	}

	for (i = 0; i < md->run_count; i++) {
		const rvn_md_run_t* run = &md->runs[i];
		rvn_color_t run_color = color;
		int clipped = run->clip_right > 0.0f;
		/* (a formula has no text: its picture was drawn with the boxes) */
		if (run->math >= 0) continue;
		if (run->color == RVN_MD_COLOR_LINK) run_color = link;
		else if (run->color == RVN_MD_COLOR_QUOTE) run_color = dim;
		else if (run->color >= RVN_MD_COLOR_TOKEN) run_color = rvn_syntax_color(theme, (rvn_token_t)(run->color - RVN_MD_COLOR_TOKEN), color);
		if (clipped) {
			if (run->x >= run->clip_right) continue;
			rvn_paint_push_clip(painter, rvn_rect(ox + run->x - 2.0f, oy + run->line_y, run->clip_right - run->x + 2.0f, run->line_h));
		}
		rvn_paint_text(painter, run->font, run->size, ox + run->x, oy + run->y, md->text.data + run->start, (int)run->length, run_color);
		if (clipped) rvn_paint_pop_clip(painter);
		if (run->flags & RVN_MD_STRIKE) {
			rvn_paint_rect(painter, rvn_rect(ox + run->x, oy + run->line_y + floorf(run->line_h * 0.54f), run->w, 1.0f), run_color);
		}
		/* the link under the pointer is underlined */
		if (run->link >= 0 && run->link == hot_link) {
			rvn_paint_rect(painter, rvn_rect(ox + run->x, oy + run->line_y + run->line_h - floorf(run->line_h * 0.2f), run->w, 1.0f), run_color);
		}
	}
}

int rvn_md_link_at(const rvn_md_t* md, float x, float y) {
	uint32_t i;
	for (i = 0; i < md->run_count; i++) {
		const rvn_md_run_t* run = &md->runs[i];
		if (run->link < 0) continue;
		if (x >= run->x && x < run->x + run->w && y >= run->line_y && y < run->line_y + run->line_h) return run->link;
	}
	return -1;
}

const char* rvn_md_link(const rvn_md_t* md, int link) {
	return link >= 0 && (uint32_t)link < md->link_count ? md->links[link] : NULL;
}

int rvn_md_link_count(const rvn_md_t* md) {
	return (int)md->link_count;
}

void rvn_md_plain_text(const rvn_md_t* md, rvn_text_t* out) {
	uint32_t b, s;
	for (b = 0; b < md->block_count; b++) {
		const rvn_md_block_t* block = &md->blocks[b];
		if (b > 0) rvn_text_append(out, "\n", 1);
		if (block->type == RVN_MD_CODE_BLOCK) {
			rvn_text_append(out, md->text.data + block->code_start, block->code_length);
		} else if (block->type == RVN_MD_TABLE) {
			uint32_t c, cells = (uint32_t)block->columns * block->rows;
			for (c = 0; c < cells; c++) {
				const rvn_md_cell_t* cell = &md->cells[block->first_cell + c];
				if (c > 0) rvn_text_append(out, c % block->columns ? "\t" : "\n", 1);
				for (s = 0; s < cell->span_count; s++) {
					const rvn_md_span_t* span = &md->spans[cell->first_span + s];
					int math = (span->flags & RVN_MD_MATH) != 0;
					const char* marks = (span->flags & RVN_MD_DISPLAY) ? "$$" : "$";
					int marks_length = (span->flags & RVN_MD_DISPLAY) ? 2 : 1;
					if (math) rvn_text_append(out, marks, (size_t)marks_length);
					rvn_text_append(out, md->text.data + span->start, span->length);
					if (math) rvn_text_append(out, marks, (size_t)marks_length);
				}
			}
		} else {
			int display = block->type == RVN_MD_MATH_BLOCK;
			for (s = 0; s < block->span_count; s++) {
				const rvn_md_span_t* span = &md->spans[block->first_span + s];
				int math = (span->flags & RVN_MD_MATH) != 0;
				const char* marks = (span->flags & RVN_MD_DISPLAY) ? "$$" : "$";
				int marks_length = (span->flags & RVN_MD_DISPLAY) ? 2 : 1;
				/* (a formula copies as it was written, marks and all) */
				if (math) rvn_text_append(out, marks, (size_t)marks_length);
				rvn_text_append(out, md->text.data + span->start, span->length);
				if (math) rvn_text_append(out, marks, (size_t)marks_length);
				if (span->flags & RVN_MD_BREAK && !display) rvn_text_append(out, "\n", 1);
			}
		}
	}
	if (!out->data) rvn_text_append(out, "", 0);
}
