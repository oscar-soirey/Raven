/**
 * Raven - widgets/rich_text_doc.c
 * The document of a RichTextEditor (see rich_text.h): paragraphs and runs,
 * the edits made on them, and the formats it is read from and written to
 * (Markdown, HTML, plain text). Nothing here draws or knows of a widget.
 *
 * Markdown is read by the parser of the Markdown labels (widgets/markdown.c),
 * and written so that this parser, and any CommonMark one, reads it back to
 * the same document: the marks of a look are put against the words they
 * cover (never against a space), what would be read as a mark is escaped,
 * and an empty paragraph is written "&nbsp;". Underlined text, which
 * Markdown has no mark for, is written <u>...</u>.
 *
 * HTML is written plainly (h1-h3, p, ul / ol / li nested, blockquote,
 * pre / code, strong, em, u, s, code, a href, br) and read leniently: every
 * tag the document has a meaning for, the bold / italic / underline /
 * line-through / monospace of a style attribute (what word processors and
 * browsers put on the clipboard), and nothing else: no other attribute is
 * kept, what is inside script, style, head... is dropped, and a link that
 * would run code (javascript:, vbscript:, data:) is dropped too.
 */

#include "rich_text.h"

#include <ctype.h>

/* ---------------------------------------------------------------------------
 *  Paragraphs
 * ------------------------------------------------------------------------- */

int rvn_rt_kind_is_list(int kind) {
	return kind == RVN_RT_BULLET || kind == RVN_RT_NUMBERED;
}

void rvn_rt_para_touch(rvn_rt_para_t* para) {
	para->laid = R_FALSE;
}

static void rvn_rt_para_layout_free(rvn_rt_para_t* para) {
	rvn_free(para->lines);
	rvn_free(para->frags);
	para->lines = NULL;
	para->frags = NULL;
	para->line_count = para->line_cap = 0;
	para->frag_count = para->frag_cap = 0;
	para->laid = R_FALSE;
}

void rvn_rt_para_free(rvn_rt_para_t* para) {
	uint32_t i;
	for (i = 0; i < para->run_count; i++) rvn_free(para->runs[i].link);
	rvn_free(para->runs);
	rvn_free(para->text);
	rvn_rt_para_layout_free(para);
	rvn_spell_line_free(&para->spell);
	memset(para, 0, sizeof(*para));
}

void rvn_rt_para_copy(rvn_rt_para_t* out, const rvn_rt_para_t* para) {
	uint32_t i;
	memset(out, 0, sizeof(*out));
	out->kind = para->kind;
	out->indent = para->indent;
	if (para->length > 0) {
		out->cap = para->length + 1;
		out->text = (char*)rvn_calloc((size_t)out->cap);
		memcpy(out->text, para->text, (size_t)para->length);
		out->text[para->length] = '\0';
		out->length = para->length;
	}
	if (para->run_count) {
		RVN_ARRAY_RESERVE(out->runs, out->run_cap, para->run_count);
		for (i = 0; i < para->run_count; i++) {
			out->runs[i] = para->runs[i];
			out->runs[i].link = para->runs[i].link ? rvn_strdup(para->runs[i].link) : NULL;
		}
		out->run_count = para->run_count;
	}
}

static int rvn_rt_same_look(const rvn_rt_run_t* a, const rvn_rt_run_t* b) {
	if (a->style != b->style) return R_FALSE;
	if (!a->link || !b->link) return a->link == b->link;
	return strcmp(a->link, b->link) == 0;
}

void rvn_rt_para_normalize(rvn_rt_para_t* para) {
	uint32_t i, kept = 0;
	for (i = 0; i < para->run_count; i++) {
		rvn_rt_run_t* run = &para->runs[i];
		if (run->length <= 0) {
			rvn_free(run->link);
			continue;
		}
		if (kept > 0 && rvn_rt_same_look(&para->runs[kept - 1], run)) {
			para->runs[kept - 1].length += run->length;
			rvn_free(run->link);
			continue;
		}
		para->runs[kept++] = *run;
	}
	para->run_count = kept;
	rvn_rt_para_touch(para);
}

int rvn_rt_para_run_at(const rvn_rt_para_t* para, int offset, int* run_start) {
	uint32_t i;
	int at = 0;
	if (run_start) *run_start = 0;
	for (i = 0; i < para->run_count; i++) {
		if (offset < at + para->runs[i].length || i + 1 == para->run_count) {
			if (run_start) *run_start = at;
			return (int)i;
		}
		at += para->runs[i].length;
	}
	return -1;
}

/* A boundary of runs at `offset`: returns the index of the run that starts there (run_count at the end). */
static uint32_t rvn_rt_para_cut(rvn_rt_para_t* para, int offset) {
	uint32_t i;
	int at = 0;
	for (i = 0; i < para->run_count; i++) {
		rvn_rt_run_t* run = &para->runs[i];
		if (offset == at) return i;
		if (offset < at + run->length) {
			RVN_ARRAY_RESERVE(para->runs, para->run_cap, para->run_count + 1);
			run = &para->runs[i];
			memmove(&para->runs[i + 1], run, sizeof(*run) * (para->run_count - i));
			para->run_count++;
			para->runs[i].length = offset - at;
			para->runs[i + 1].length -= offset - at;
			para->runs[i + 1].link = para->runs[i].link ? rvn_strdup(para->runs[i].link) : NULL;
			return i + 1;
		}
		at += run->length;
	}
	return para->run_count;
}

void rvn_rt_para_insert(rvn_rt_para_t* para, int offset, const char* text, int length, uint32_t style, const char* link) {
	uint32_t index;
	if (length < 0) length = (int)strlen(text);
	if (length <= 0) return;
	offset = rvn_clampi(offset, 0, para->length);
	if (para->length + length + 1 > para->cap) {
		int cap = rvn_maxi(para->cap * 2, para->length + length + 16);
		para->text = (char*)rvn_realloc(para->text, (size_t)cap);
		para->cap = cap;
	}
	memmove(para->text + offset + length, para->text + offset, (size_t)(para->length - offset));
	memcpy(para->text + offset, text, (size_t)length);
	para->length += length;
	para->text[para->length] = '\0';

	index = rvn_rt_para_cut(para, offset);
	RVN_ARRAY_RESERVE(para->runs, para->run_cap, para->run_count + 1);
	memmove(&para->runs[index + 1], &para->runs[index], sizeof(rvn_rt_run_t) * (para->run_count - index));
	para->run_count++;
	para->runs[index].length = length;
	para->runs[index].style = style & RVN_RT_STYLES;
	para->runs[index].link = link && link[0] ? rvn_strdup(link) : NULL;
	rvn_rt_para_normalize(para);
}

void rvn_rt_para_delete(rvn_rt_para_t* para, int from, int to) {
	uint32_t a, b, i;
	from = rvn_clampi(from, 0, para->length);
	to = rvn_clampi(to, from, para->length);
	if (to == from) return;
	a = rvn_rt_para_cut(para, from);
	b = rvn_rt_para_cut(para, to);
	for (i = a; i < b; i++) rvn_free(para->runs[i].link);
	memmove(&para->runs[a], &para->runs[b], sizeof(rvn_rt_run_t) * (para->run_count - b));
	para->run_count -= b - a;
	memmove(para->text + from, para->text + to, (size_t)(para->length - to));
	para->length -= to - from;
	para->text[para->length] = '\0';
	rvn_rt_para_normalize(para);
}

void rvn_rt_para_append(rvn_rt_para_t* para, const rvn_rt_para_t* source, int from, int to) {
	uint32_t i;
	int at = 0;
	from = rvn_clampi(from, 0, source->length);
	to = rvn_clampi(to, from, source->length);
	for (i = 0; i < source->run_count && at < to; i++) {
		const rvn_rt_run_t* run = &source->runs[i];
		int start = rvn_maxi(at, from), end = rvn_mini(at + run->length, to);
		if (end > start) rvn_rt_para_insert(para, para->length, source->text + start, end - start, run->style, run->link);
		at += run->length;
	}
}

void rvn_rt_para_style(rvn_rt_para_t* para, int from, int to, uint32_t set, uint32_t clear, int link, const char* url) {
	uint32_t a, b, i;
	from = rvn_clampi(from, 0, para->length);
	to = rvn_clampi(to, from, para->length);
	if (to == from) return;
	a = rvn_rt_para_cut(para, from);
	b = rvn_rt_para_cut(para, to);
	for (i = a; i < b; i++) {
		rvn_rt_run_t* run = &para->runs[i];
		run->style = ((run->style | set) & ~clear) & RVN_RT_STYLES;
		if (link) {
			rvn_free(run->link);
			run->link = url && url[0] ? rvn_strdup(url) : NULL;
		}
	}
	rvn_rt_para_normalize(para);
}

/* ---------------------------------------------------------------------------
 *  Documents
 * ------------------------------------------------------------------------- */

int rvn_rt_pos_cmp(rvn_rt_pos_t a, rvn_rt_pos_t b) {
	if (a.para != b.para) return a.para < b.para ? -1 : 1;
	if (a.offset != b.offset) return a.offset < b.offset ? -1 : 1;
	return 0;
}

void rvn_rt_doc_insert(rvn_rt_doc_t* doc, uint32_t index, uint32_t count) {
	if (index > doc->count) index = doc->count;
	RVN_ARRAY_RESERVE(doc->paras, doc->cap, doc->count + count);
	memmove(&doc->paras[index + count], &doc->paras[index], sizeof(rvn_rt_para_t) * (doc->count - index));
	memset(&doc->paras[index], 0, sizeof(rvn_rt_para_t) * count);
	doc->count += count;
}

void rvn_rt_doc_remove(rvn_rt_doc_t* doc, uint32_t index, uint32_t count) {
	uint32_t i;
	if (index >= doc->count) return;
	if (count > doc->count - index) count = doc->count - index;
	for (i = 0; i < count; i++) rvn_rt_para_free(&doc->paras[index + i]);
	memmove(&doc->paras[index], &doc->paras[index + count], sizeof(rvn_rt_para_t) * (doc->count - index - count));
	doc->count -= count;
}

void rvn_rt_doc_init(rvn_rt_doc_t* doc) {
	memset(doc, 0, sizeof(*doc));
	rvn_rt_doc_insert(doc, 0, 1);
}

void rvn_rt_doc_free(rvn_rt_doc_t* doc) {
	uint32_t i;
	for (i = 0; i < doc->count; i++) rvn_rt_para_free(&doc->paras[i]);
	rvn_free(doc->paras);
	memset(doc, 0, sizeof(*doc));
}

void rvn_rt_doc_split(rvn_rt_doc_t* doc, rvn_rt_pos_t pos) {
	rvn_rt_para_t* para;
	rvn_rt_para_t* next;
	rvn_rt_doc_insert(doc, (uint32_t)pos.para + 1, 1);
	para = &doc->paras[pos.para];
	next = &doc->paras[pos.para + 1];
	next->kind = para->kind;
	next->indent = para->indent;
	rvn_rt_para_append(next, para, pos.offset, para->length);
	rvn_rt_para_delete(para, pos.offset, para->length);
	rvn_rt_para_touch(para);
	rvn_rt_para_touch(next);
}

void rvn_rt_doc_delete(rvn_rt_doc_t* doc, rvn_rt_pos_t a, rvn_rt_pos_t b) {
	rvn_rt_para_t* first;
	if (rvn_rt_pos_cmp(a, b) >= 0) return;
	first = &doc->paras[a.para];
	if (a.para == b.para) {
		rvn_rt_para_delete(first, a.offset, b.offset);
		return;
	}
	rvn_rt_para_delete(first, a.offset, first->length);
	rvn_rt_para_append(first, &doc->paras[b.para], b.offset, doc->paras[b.para].length);
	rvn_rt_doc_remove(doc, (uint32_t)a.para + 1, (uint32_t)(b.para - a.para));
}

void rvn_rt_doc_slice(const rvn_rt_doc_t* doc, rvn_rt_pos_t a, rvn_rt_pos_t b, rvn_rt_doc_t* out) {
	int i;
	memset(out, 0, sizeof(*out));
	if (rvn_rt_pos_cmp(a, b) > 0) {
		rvn_rt_pos_t t = a;
		a = b;
		b = t;
	}
	rvn_rt_doc_insert(out, 0, (uint32_t)(b.para - a.para + 1));
	for (i = a.para; i <= b.para; i++) {
		const rvn_rt_para_t* source = &doc->paras[i];
		rvn_rt_para_t* para = &out->paras[i - a.para];
		para->kind = source->kind;
		para->indent = source->indent;
		rvn_rt_para_append(para, source, i == a.para ? a.offset : 0, i == b.para ? b.offset : source->length);
	}
}

rvn_rt_pos_t rvn_rt_doc_paste(rvn_rt_doc_t* doc, rvn_rt_pos_t pos, rvn_rt_doc_t* piece) {
	rvn_rt_para_t* target = &doc->paras[pos.para];
	rvn_rt_pos_t end = pos;
	uint32_t n = piece->count, i;
	int was_empty = target->length == 0;

	if (n == 0) return pos;
	if (n == 1) {
		rvn_rt_para_t rest;
		memset(&rest, 0, sizeof(rest));
		rvn_rt_para_append(&rest, target, pos.offset, target->length);
		rvn_rt_para_delete(target, pos.offset, target->length);
		rvn_rt_para_append(target, &piece->paras[0], 0, piece->paras[0].length);
		end.offset = target->length;
		rvn_rt_para_append(target, &rest, 0, rest.length);
		rvn_rt_para_free(&rest);
		if (was_empty) {
			target->kind = piece->paras[0].kind;
			target->indent = piece->paras[0].indent;
		}
		rvn_rt_doc_free(piece);
		return end;
	}

	rvn_rt_doc_split(doc, pos);
	target = &doc->paras[pos.para];
	rvn_rt_para_append(target, &piece->paras[0], 0, piece->paras[0].length);
	if (was_empty) {
		target->kind = piece->paras[0].kind;
		target->indent = piece->paras[0].indent;
	}
	/* the paragraphs between, moved in */
	rvn_rt_doc_insert(doc, (uint32_t)pos.para + 1, n - 2);
	for (i = 1; i + 1 < n; i++) {
		doc->paras[(uint32_t)pos.para + i] = piece->paras[i];
		memset(&piece->paras[i], 0, sizeof(rvn_rt_para_t));
	}
	/* the last one, then the rest of the paragraph pasted into */
	{
		rvn_rt_para_t* last = &doc->paras[(uint32_t)pos.para + n - 1];
		rvn_rt_para_t merged = piece->paras[n - 1];
		memset(&piece->paras[n - 1], 0, sizeof(rvn_rt_para_t));
		if (merged.kind == RVN_RT_PARAGRAPH && last->length > 0) {
			merged.kind = last->kind;
			merged.indent = last->indent;
		}
		end.para = pos.para + (int)n - 1;
		end.offset = merged.length;
		rvn_rt_para_append(&merged, last, 0, last->length);
		rvn_rt_para_free(last);
		*last = merged;
		rvn_rt_para_touch(last);
	}
	rvn_rt_doc_free(piece);
	return end;
}

/* ---------------------------------------------------------------------------
 *  Plain text
 * ------------------------------------------------------------------------- */

void rvn_rt_from_text(rvn_rt_doc_t* doc, const char* text) {
	const char* p = text ? text : "";
	rvn_rt_doc_free(doc);
	for (;;) {
		const char* end = strchr(p, '\n');
		int length = end ? (int)(end - p) : (int)strlen(p);
		rvn_rt_para_t* para;
		rvn_rt_doc_insert(doc, doc->count, 1);
		para = &doc->paras[doc->count - 1];
		if (length > 0 && p[length - 1] == '\r') length--;
		rvn_rt_para_insert(para, 0, p, length, 0, NULL);
		if (!end) break;
		p = end + 1;
	}
}

void rvn_rt_to_text(const rvn_rt_doc_t* doc, rvn_text_t* out) {
	uint32_t i;
	for (i = 0; i < doc->count; i++) {
		if (i > 0) rvn_text_append(out, "\n", 1);
		if (doc->paras[i].length) rvn_text_append(out, doc->paras[i].text, (size_t)doc->paras[i].length);
	}
	if (!out->data) rvn_text_append(out, "", 0);
}

/* ---------------------------------------------------------------------------
 *  Markdown: reading (the parser of widgets/markdown.c)
 * ------------------------------------------------------------------------- */

static uint32_t rvn_rt_style_of_md(uint32_t flags) {
	uint32_t style = 0;
	if (flags & RVN_MD_BOLD) style |= RVN_RT_BOLD;
	if (flags & RVN_MD_ITALIC) style |= RVN_RT_ITALIC;
	if (flags & RVN_MD_UNDERLINE) style |= RVN_RT_UNDERLINE;
	if (flags & RVN_MD_STRIKE) style |= RVN_RT_STRIKE;
	if (flags & RVN_MD_CODE) style |= RVN_RT_CODE;
	return style;
}

static void rvn_rt_md_spans(rvn_rt_para_t* para, const rvn_md_t* md, int block, int cell, const char* between) {
	int count = rvn_md_block_span(md, block, cell, -1, NULL, NULL, NULL, NULL), s;
	if (between && para->length > 0) rvn_rt_para_insert(para, para->length, between, -1, 0, NULL);
	for (s = 0; s < count; s++) {
		const char* text;
		const char* link;
		int length;
		uint32_t flags;
		rvn_md_block_span(md, block, cell, s, &text, &length, &flags, &link);
		rvn_rt_para_insert(para, para->length, text, length, rvn_rt_style_of_md(flags), link);
		if (flags & RVN_MD_BREAK) rvn_rt_para_insert(para, para->length, "\n", 1, 0, NULL);
	}
	/* (a break at the very end shows nothing) */
	while (para->length > 0 && para->text[para->length - 1] == '\n') rvn_rt_para_delete(para, para->length - 1, para->length);
	/* "&nbsp;" alone is an empty paragraph */
	if (para->length == 2 && memcmp(para->text, "\xC2\xA0", 2) == 0) rvn_rt_para_delete(para, 0, 2);
}

static rvn_rt_para_t* rvn_rt_add_para(rvn_rt_doc_t* doc, int kind, int indent) {
	rvn_rt_para_t* para;
	rvn_rt_doc_insert(doc, doc->count, 1);
	para = &doc->paras[doc->count - 1];
	para->kind = (uint8_t)kind;
	para->indent = (uint8_t)rvn_clampi(indent, 0, RVN_RT_MAX_INDENT);
	return para;
}

void rvn_rt_from_markdown(rvn_rt_doc_t* doc, const char* markdown) {
	rvn_text_t source = { NULL, 0, 0 };
	rvn_md_t* md;
	int b, count;

	rvn_rt_doc_free(doc);
	/* (a first line of its own: the parser takes the indentation common to all the lines away, but never looks at the first one) */
	rvn_text_add(&source, "\n");
	if (markdown) rvn_text_add(&source, markdown);
	md = rvn_md_parse(source.data);
	rvn_free(source.data);
	count = rvn_md_block_count(md);
	for (b = 0; b < count; b++) {
		rvn_md_block_info_t info;
		rvn_rt_para_t* para;
		rvn_md_block_info(md, b, &info);
		switch (info.type) {
			case RVN_MD_INFO_HEADING:
				para = rvn_rt_add_para(doc, RVN_RT_HEADING_1 + rvn_clampi(info.level, 1, 3) - 1, 0);
				rvn_rt_md_spans(para, md, b, -1, NULL);
				/* (a title holds no line break) */
				{
					int i;
					for (i = 0; i < para->length; i++) if (para->text[i] == '\n') para->text[i] = ' ';
				}
				break;
			case RVN_MD_INFO_CODE: {
				const char* p = info.code;
				const char* end = info.code + info.code_length;
				for (;;) {
					const char* line_end = (const char*)memchr(p, '\n', (size_t)(end - p));
					para = rvn_rt_add_para(doc, RVN_RT_CODE_BLOCK, 0);
					rvn_rt_para_insert(para, 0, p, (int)((line_end ? line_end : end) - p), 0, NULL);
					if (!line_end) break;
					p = line_end + 1;
				}
				break;
			}
			case RVN_MD_INFO_RULE:
				/* (the document has no rules) */
				break;
			case RVN_MD_INFO_TABLE: {
				/* a paragraph per row, its cells separated by " | " */
				int row, column;
				for (row = 0; row < info.rows; row++) {
					para = rvn_rt_add_para(doc, RVN_RT_PARAGRAPH, 0);
					for (column = 0; column < info.columns; column++) rvn_rt_md_spans(para, md, b, row * info.columns + column, column > 0 ? " | " : NULL);
				}
				break;
			}
			case RVN_MD_INFO_IMAGE:
			case RVN_MD_INFO_PARAGRAPH:
			default: {
				int kind = RVN_RT_PARAGRAPH, indent = 0;
				if (info.marker == 1) kind = RVN_RT_BULLET;
				else if (info.marker == 2) kind = RVN_RT_NUMBERED;
				else if (info.quote > 0) kind = RVN_RT_QUOTE;
				if (rvn_rt_kind_is_list(kind)) indent = info.list - 1;
				para = rvn_rt_add_para(doc, kind, indent);
				rvn_rt_md_spans(para, md, b, -1, NULL);
				break;
			}
		}
	}
	rvn_md_free(md);
	if (doc->count == 0) rvn_rt_add_para(doc, RVN_RT_PARAGRAPH, 0);
}

/* ---------------------------------------------------------------------------
 *  Markdown: writing
 * ------------------------------------------------------------------------- */

/* What the marks of a look are, outermost first (a link holds the others). */
enum { RVN_RT_MARK_LINK, RVN_RT_MARK_BOLD, RVN_RT_MARK_ITALIC, RVN_RT_MARK_STRIKE, RVN_RT_MARK_UNDERLINE, RVN_RT_MARK_COUNT };

static const uint32_t g_rt_mark_style[RVN_RT_MARK_COUNT] = { RVN_RT_LINK, RVN_RT_BOLD, RVN_RT_ITALIC, RVN_RT_STRIKE, RVN_RT_UNDERLINE };

typedef struct rvn_rt_writer_t {
	rvn_text_t* out;
	int html;
	/** The marks open, in the order they were opened. */
	int open[RVN_RT_MARK_COUNT];
	int open_count;
	const char* link;
	/** Markdown: what continues a line after a break (the indentation of a list item, "> " in a quote). */
	const char* continuation;
	/** Markdown: nothing was written on the line yet (its first character may need escaping). */
	int line_start;
	/** Markdown: a title holds no line break. */
	int no_breaks;
} rvn_rt_writer_t;

static void rvn_rt_md_url(rvn_text_t* out, const char* url) {
	const char* p;
	for (p = url; *p; p++) {
		unsigned char c = (unsigned char)*p;
		if (c <= ' ' || c == '(' || c == ')' || c == '<' || c == '>' || c == '\\' || c == 127) rvn_text_printf(out, "%%%02X", c);
		else rvn_text_append(out, p, 1);
	}
}

static void rvn_rt_html_escape(rvn_text_t* out, const char* text, int length, int attribute) {
	int i, from = 0;
	for (i = 0; i < length; i++) {
		const char* entity = NULL;
		switch (text[i]) {
			case '&': entity = "&amp;"; break;
			case '<': entity = "&lt;"; break;
			case '>': entity = "&gt;"; break;
			case '"': entity = attribute ? "&quot;" : NULL; break;
			default: break;
		}
		if (!entity) continue;
		rvn_text_append(out, text + from, (size_t)(i - from));
		rvn_text_add(out, entity);
		from = i + 1;
	}
	rvn_text_append(out, text + from, (size_t)(length - from));
}

static void rvn_rt_mark(rvn_rt_writer_t* w, int mark, int opening) {
	static const char* const md_open[RVN_RT_MARK_COUNT] = { "[", "**", "*", "~~", "<u>" };
	static const char* const md_close[RVN_RT_MARK_COUNT] = { "", "**", "*", "~~", "</u>" };
	static const char* const html_open[RVN_RT_MARK_COUNT] = { "", "<strong>", "<em>", "<s>", "<u>" };
	static const char* const html_close[RVN_RT_MARK_COUNT] = { "</a>", "</strong>", "</em>", "</s>", "</u>" };
	if (w->html) {
		if (mark == RVN_RT_MARK_LINK && opening) {
			rvn_text_add(w->out, "<a href=\"");
			rvn_rt_html_escape(w->out, w->link, (int)strlen(w->link), R_TRUE);
			rvn_text_add(w->out, "\">");
		} else {
			rvn_text_add(w->out, opening ? html_open[mark] : html_close[mark]);
		}
		return;
	}
	if (mark == RVN_RT_MARK_LINK && !opening) {
		rvn_text_add(w->out, "](");
		rvn_rt_md_url(w->out, w->link);
		rvn_text_add(w->out, ")");
		return;
	}
	rvn_text_add(w->out, opening ? md_open[mark] : md_close[mark]);
	w->line_start = R_FALSE;
}

static void rvn_rt_close_from(rvn_rt_writer_t* w, int index) {
	while (w->open_count > index) {
		int mark = w->open[--w->open_count];
		rvn_rt_mark(w, mark, R_FALSE);
		if (mark == RVN_RT_MARK_LINK) w->link = NULL;
	}
}

/* Makes the marks open those of `style` and `link`: closes down to the first one that differs, then opens what is missing. */
static void rvn_rt_set_marks(rvn_rt_writer_t* w, uint32_t style, const char* link) {
	int i, m;
	if (link) style |= RVN_RT_LINK;
	for (i = 0; i < w->open_count; i++) {
		int mark = w->open[i];
		if (!(style & g_rt_mark_style[mark])) break;
		if (mark == RVN_RT_MARK_LINK && strcmp(w->link, link) != 0) break;
	}
	rvn_rt_close_from(w, i);
	for (m = 0; m < RVN_RT_MARK_COUNT; m++) {
		int is_open = R_FALSE;
		if (!(style & g_rt_mark_style[m])) continue;
		for (i = 0; i < w->open_count; i++) if (w->open[i] == m) is_open = R_TRUE;
		if (is_open) continue;
		if (m == RVN_RT_MARK_LINK) w->link = link;
		rvn_rt_mark(w, m, R_TRUE);
		w->open[w->open_count++] = m;
	}
}

static int rvn_rt_md_escapes(char c) {
	return c == '\\' || c == '`' || c == '*' || c == '_' || c == '[' || c == ']' || c == '<' || c == '>' ||
		c == '~' || c == '&' || c == '|';
}

/* Text written as it is read: the marks escaped, and what would start a block at the start of a line. */
static void rvn_rt_md_text(rvn_rt_writer_t* w, const char* text, int length) {
	int i;
	for (i = 0; i < length; i++) {
		char c = text[i];
		if (c == '\n') {
			if (w->no_breaks) {
				rvn_text_append(w->out, " ", 1);
			} else {
				rvn_text_add(w->out, "\\\n");
				rvn_text_add(w->out, w->continuation ? w->continuation : "");
				w->line_start = R_TRUE;
			}
			continue;
		}
		if (w->line_start) {
			int digits = 0;
			while (i + digits < length && text[i + digits] >= '0' && text[i + digits] <= '9') digits++;
			if (c == '#' || c == '-' || c == '+' || c == '=' || c == '>') {
				rvn_text_append(w->out, "\\", 1);
			} else if (digits > 0 && i + digits < length && (text[i + digits] == '.' || text[i + digits] == ')')) {
				/* "1." would start a list: "1\." */
				rvn_text_append(w->out, text + i, (size_t)digits);
				rvn_text_append(w->out, "\\", 1);
				i += digits;
				c = text[i];
			}
			w->line_start = R_FALSE;
		}
		if (rvn_rt_md_escapes(c) || (c == ':' && i + 2 < length && text[i + 1] == '/' && text[i + 2] == '/')) rvn_text_append(w->out, "\\", 1);
		rvn_text_append(w->out, &c, 1);
	}
}

/* Inline code: between enough backquotes, padded when it starts or ends with one. */
static void rvn_rt_md_code(rvn_rt_writer_t* w, const char* text, int length) {
	int longest = 0, run = 0, i, ticks, pad;
	for (i = 0; i < length; i++) {
		run = text[i] == '`' ? run + 1 : 0;
		if (run > longest) longest = run;
	}
	ticks = longest + 1;
	pad = length > 0 && (text[0] == '`' || text[length - 1] == '`' || (text[0] == ' ' && text[length - 1] == ' '));
	for (i = 0; i < ticks; i++) rvn_text_append(w->out, "`", 1);
	if (pad) rvn_text_append(w->out, " ", 1);
	for (i = 0; i < length; i++) rvn_text_append(w->out, text[i] == '\n' ? " " : text + i, 1);
	if (pad) rvn_text_append(w->out, " ", 1);
	for (i = 0; i < ticks; i++) rvn_text_append(w->out, "`", 1);
	w->line_start = R_FALSE;
}

static void rvn_rt_write_piece(rvn_rt_writer_t* w, const char* text, int length, uint32_t style, const char* link) {
	if (length <= 0) return;
	rvn_rt_set_marks(w, style, link);
	if (w->html) {
		int i, from = 0;
		if (style & RVN_RT_CODE) rvn_text_add(w->out, "<code>");
		for (i = 0; i <= length; i++) {
			if (i < length && text[i] != '\n') continue;
			rvn_rt_html_escape(w->out, text + from, i - from, R_FALSE);
			if (i < length) rvn_text_add(w->out, "<br>");
			from = i + 1;
		}
		if (style & RVN_RT_CODE) rvn_text_add(w->out, "</code>");
	} else if (style & RVN_RT_CODE) {
		rvn_rt_md_code(w, text, length);
	} else {
		rvn_rt_md_text(w, text, length);
	}
}

/* The looks of the first character of a paragraph from `offset` that is not a space (0: none). */
static uint32_t rvn_rt_style_after(const rvn_rt_para_t* para, int offset) {
	uint32_t r;
	int start = 0;
	for (r = 0; r < para->run_count; r++) {
		int end = start + para->runs[r].length, p;
		for (p = rvn_maxi(start, offset); p < end; p++) {
			if (para->text[p] != ' ' && para->text[p] != '\n') return para->runs[r].style;
		}
		start = end;
	}
	return 0;
}

/*
 * The text of a paragraph with its looks. A mark is never put against a
 * space (it would not be one): the spaces at the edges of a run keep only
 * the marks that go on on both sides of them.
 */
static void rvn_rt_write_inline(rvn_rt_writer_t* w, const rvn_rt_para_t* para) {
	const uint32_t edges = RVN_RT_BOLD | RVN_RT_ITALIC | RVN_RT_STRIKE | RVN_RT_UNDERLINE;
	uint32_t i, before = 0;
	int at = 0;
	w->open_count = 0;
	w->link = NULL;
	for (i = 0; i < para->run_count; i++) {
		const rvn_rt_run_t* run = &para->runs[i];
		const char* text = para->text + at;
		int length = run->length, lead = 0, trail = 0;
		uint32_t kept;
		if (run->style & RVN_RT_CODE) {
			rvn_rt_write_piece(w, text, length, run->style, run->link);
			before = run->style;
			at += length;
			continue;
		}
		while (lead < length && (text[lead] == ' ' || text[lead] == '\n')) lead++;
		while (trail < length - lead && (text[length - 1 - trail] == ' ' || text[length - 1 - trail] == '\n')) trail++;
		if (lead) {
			kept = run->style & edges & before & rvn_rt_style_after(para, at + lead);
			rvn_rt_write_piece(w, text, lead, (run->style & ~edges) | kept, run->link);
		}
		if (length - lead - trail > 0) {
			rvn_rt_write_piece(w, text + lead, length - lead - trail, run->style, run->link);
			before = run->style;
		}
		if (trail) {
			kept = run->style & edges & before & rvn_rt_style_after(para, at + length);
			rvn_rt_write_piece(w, text + length - trail, trail, (run->style & ~edges) | kept, run->link);
		}
		at += length;
	}
	rvn_rt_close_from(w, 0);
}

/* The numbers of the items of numbered lists, as shown: counted per level, started again by anything else. */
static int rvn_rt_number(int* counters, const rvn_rt_para_t* para) {
	int level = para->indent, k;
	if (!rvn_rt_kind_is_list(para->kind)) {
		for (k = 0; k <= RVN_RT_MAX_INDENT; k++) counters[k] = 0;
		return 0;
	}
	for (k = level + 1; k <= RVN_RT_MAX_INDENT; k++) counters[k] = 0;
	if (para->kind == RVN_RT_BULLET) {
		counters[level] = 0;
		return 0;
	}
	return ++counters[level];
}

void rvn_rt_to_markdown(const rvn_rt_doc_t* doc, rvn_text_t* out) {
	rvn_rt_writer_t w;
	int counters[RVN_RT_MAX_INDENT + 1], columns[RVN_RT_MAX_INDENT + 1];
	char continuation[64];
	uint32_t i;
	int k;

	memset(&w, 0, sizeof(w));
	memset(counters, 0, sizeof(counters));
	for (k = 0; k <= RVN_RT_MAX_INDENT; k++) columns[k] = 2 * (k + 1);
	w.out = out;
	for (i = 0; i < doc->count; i++) {
		const rvn_rt_para_t* para = &doc->paras[i];
		const rvn_rt_para_t* previous = i > 0 ? &doc->paras[i - 1] : NULL;
		const rvn_rt_para_t* next = i + 1 < doc->count ? &doc->paras[i + 1] : NULL;
		int number = rvn_rt_number(counters, para);

		/* between two paragraphs: one line in a list or a code block, a line holding ">" in a quote, else a blank line */
		if (previous) {
			/* (a list of the other kind starts after a blank line) */
			if (rvn_rt_kind_is_list(previous->kind) && rvn_rt_kind_is_list(para->kind) && (para->indent > 0 || previous->kind == para->kind || previous->indent > 0))
				rvn_text_add(out, "\n");
			else if (previous->kind == RVN_RT_CODE_BLOCK && para->kind == RVN_RT_CODE_BLOCK) rvn_text_add(out, "\n");
			else if (previous->kind == RVN_RT_QUOTE && para->kind == RVN_RT_QUOTE) rvn_text_add(out, "\n>\n");
			else rvn_text_add(out, "\n\n");
		}
		w.continuation = NULL;
		w.line_start = R_TRUE;
		w.no_breaks = R_FALSE;

		switch (para->kind) {
			case RVN_RT_CODE_BLOCK: {
				/* a fence longer than any line of backquotes in the block */
				int fence = 3, run;
				uint32_t j;
				if (!previous || previous->kind != RVN_RT_CODE_BLOCK) {
					for (j = i; j < doc->count && doc->paras[j].kind == RVN_RT_CODE_BLOCK; j++) {
						const rvn_rt_para_t* line = &doc->paras[j];
						run = 0;
						while (run < line->length && line->text[run] == '`') run++;
						if (run >= fence) fence = run + 1;
					}
					for (run = 0; run < fence; run++) rvn_text_append(out, "`", 1);
					rvn_text_add(out, "\n");
				}
				if (para->length) rvn_text_append(out, para->text, (size_t)para->length);
				if (!next || next->kind != RVN_RT_CODE_BLOCK) {
					/* (the same fence: counted again from the first line of the block) */
					uint32_t first = i;
					fence = 3;
					while (first > 0 && doc->paras[first - 1].kind == RVN_RT_CODE_BLOCK) first--;
					for (j = first; j <= i; j++) {
						const rvn_rt_para_t* line = &doc->paras[j];
						run = 0;
						while (run < line->length && line->text[run] == '`') run++;
						if (run >= fence) fence = run + 1;
					}
					rvn_text_add(out, "\n");
					for (run = 0; run < fence; run++) rvn_text_append(out, "`", 1);
				}
				continue;
			}
			case RVN_RT_HEADING_1:
			case RVN_RT_HEADING_2:
			case RVN_RT_HEADING_3:
				rvn_text_add(out, para->kind == RVN_RT_HEADING_1 ? "# " : (para->kind == RVN_RT_HEADING_2 ? "## " : "### "));
				w.no_breaks = R_TRUE;
				break;
			case RVN_RT_QUOTE:
				rvn_text_add(out, "> ");
				w.continuation = "> ";
				break;
			case RVN_RT_BULLET:
			case RVN_RT_NUMBERED: {
				/* indented to the text of the item it is in */
				int level = para->indent, indent = level > 0 ? columns[level - 1] : 0, length;
				char marker[16];
				if (para->kind == RVN_RT_BULLET) snprintf(marker, sizeof(marker), "- ");
				else snprintf(marker, sizeof(marker), "%d. ", number);
				length = (int)strlen(marker);
				for (k = 0; k < indent; k++) rvn_text_append(out, " ", 1);
				rvn_text_add(out, marker);
				columns[level] = indent + length;
				for (k = level + 1; k <= RVN_RT_MAX_INDENT; k++) columns[k] = columns[k - 1] + 2;
				indent = rvn_mini(indent + length, (int)sizeof(continuation) - 1);
				memset(continuation, ' ', (size_t)indent);
				continuation[indent] = '\0';
				w.continuation = continuation;
				break;
			}
			default:
				break;
		}
		if (para->length == 0) {
			rvn_text_add(out, "&nbsp;");
			continue;
		}
		rvn_rt_write_inline(&w, para);
	}
	/* (an empty document is an empty text) */
	if (doc->count == 1 && doc->paras[0].length == 0 && doc->paras[0].kind == RVN_RT_PARAGRAPH) out->length = 0;
	if (out->length > 0) rvn_text_add(out, "\n");
	else rvn_text_append(out, "", 0);
	if (out->data) out->data[out->length] = '\0';
}

/* ---------------------------------------------------------------------------
 *  HTML: writing
 * ------------------------------------------------------------------------- */

void rvn_rt_to_html(const rvn_rt_doc_t* doc, rvn_text_t* out) {
	rvn_rt_writer_t w;
	/* the lists open: their kinds, level after level */
	int lists[RVN_RT_MAX_INDENT + 1];
	int depth = 0;
	uint32_t i;

	memset(&w, 0, sizeof(w));
	w.out = out;
	w.html = R_TRUE;
	for (i = 0; i < doc->count; i++) {
		const rvn_rt_para_t* para = &doc->paras[i];
		const rvn_rt_para_t* previous = i > 0 ? &doc->paras[i - 1] : NULL;
		const rvn_rt_para_t* next = i + 1 < doc->count ? &doc->paras[i + 1] : NULL;

		if (rvn_rt_kind_is_list(para->kind)) {
			int level = para->indent;
			/* deeper lists end; a list of the other kind at this level ends */
			while (depth > level + 1) {
				rvn_text_add(out, lists[depth - 1] == RVN_RT_NUMBERED ? "</li></ol>" : "</li></ul>");
				depth--;
			}
			if (depth == level + 1 && lists[level] != para->kind) {
				rvn_text_add(out, lists[level] == RVN_RT_NUMBERED ? "</li></ol>" : "</li></ul>");
				depth--;
				if (depth == 0) rvn_text_add(out, "\n");
			}
			if (depth == level + 1) {
				rvn_text_add(out, "</li>\n<li>");
			} else {
				while (depth < level + 1) {
					/* (a level skipped is a list in an item of its own) */
					rvn_text_add(out, para->kind == RVN_RT_NUMBERED && depth == level ? "<ol>" : "<ul>");
					lists[depth] = depth == level ? para->kind : RVN_RT_BULLET;
					rvn_text_add(out, "\n<li>");
					depth++;
				}
			}
			if (para->length) rvn_rt_write_inline(&w, para);
			continue;
		}
		while (depth > 0) {
			rvn_text_add(out, lists[depth - 1] == RVN_RT_NUMBERED ? "</li></ol>" : "</li></ul>");
			depth--;
			rvn_text_add(out, "\n");
		}
		switch (para->kind) {
			case RVN_RT_CODE_BLOCK:
				if (!previous || previous->kind != RVN_RT_CODE_BLOCK) rvn_text_add(out, "<pre><code>");
				rvn_rt_html_escape(out, para->text ? para->text : "", para->length, R_FALSE);
				if (next && next->kind == RVN_RT_CODE_BLOCK) rvn_text_add(out, "\n");
				else rvn_text_add(out, "</code></pre>\n");
				continue;
			case RVN_RT_HEADING_1:
			case RVN_RT_HEADING_2:
			case RVN_RT_HEADING_3: {
				int level = para->kind - RVN_RT_HEADING_1 + 1;
				rvn_text_printf(out, "<h%d>", level);
				rvn_rt_write_inline(&w, para);
				rvn_text_printf(out, "</h%d>\n", level);
				continue;
			}
			case RVN_RT_QUOTE:
				if (!previous || previous->kind != RVN_RT_QUOTE) rvn_text_add(out, "<blockquote>");
				rvn_text_add(out, "<p>");
				if (para->length) rvn_rt_write_inline(&w, para);
				else rvn_text_add(out, "<br>");
				rvn_text_add(out, "</p>");
				rvn_text_add(out, next && next->kind == RVN_RT_QUOTE ? "\n" : "</blockquote>\n");
				continue;
			default:
				rvn_text_add(out, "<p>");
				if (para->length) rvn_rt_write_inline(&w, para);
				else rvn_text_add(out, "<br>");
				rvn_text_add(out, "</p>\n");
				continue;
		}
	}
	while (depth > 0) {
		rvn_text_add(out, lists[depth - 1] == RVN_RT_NUMBERED ? "</li></ol>\n" : "</li></ul>\n");
		depth--;
	}
	if (!out->data) rvn_text_append(out, "", 0);
}

/* ---------------------------------------------------------------------------
 *  HTML: reading
 * ------------------------------------------------------------------------- */

#define RVN_RT_HTML_DEPTH 64

typedef struct rvn_rt_element_t {
	char name[12];
	/** Looks it adds and takes away. */
	uint32_t add, remove;
	/** Its link (owned), or NULL. */
	char* link;
	/** It is a block: what is after it starts a new paragraph. */
	uint8_t block;
	uint8_t heading;
	/** ul / ol: the kind of its items. */
	uint8_t list;
	uint8_t item;
	uint8_t quote;
	uint8_t pre;
} rvn_rt_element_t;

typedef struct rvn_rt_html_t {
	rvn_rt_doc_t* doc;
	/** The paragraph being filled (an index in doc), or -1. */
	int para;
	/** It holds something, or was asked for (<br> in an empty block, a line of <pre>). */
	int kept;
	int space;
	rvn_rt_element_t stack[RVN_RT_HTML_DEPTH];
	int depth;
} rvn_rt_html_t;

/* The paragraph being filled ends: its spaces at the end go, and it goes too when nothing asked for it. */
static void rvn_rt_html_end(rvn_rt_html_t* h) {
	rvn_rt_para_t* para;
	if (h->para < 0) return;
	para = &h->doc->paras[h->para];
	if (para->kind != RVN_RT_CODE_BLOCK) {
		while (para->length > 0 && para->text[para->length - 1] == ' ') rvn_rt_para_delete(para, para->length - 1, para->length);
		if (para->length > 0 && para->text[para->length - 1] == '\n') rvn_rt_para_delete(para, para->length - 1, para->length);
	}
	if (para->length == 0 && !h->kept) rvn_rt_doc_remove(h->doc, (uint32_t)h->para, 1);
	h->para = -1;
	h->kept = R_FALSE;
	h->space = R_FALSE;
}

/* The paragraph to write into: the one being filled, or a new one of the kind the open elements give. */
static rvn_rt_para_t* rvn_rt_html_para(rvn_rt_html_t* h) {
	int kind = RVN_RT_PARAGRAPH, indent = 0, lists = 0, quote = 0, d;
	if (h->para >= 0) return &h->doc->paras[h->para];
	for (d = 0; d < h->depth; d++) {
		const rvn_rt_element_t* e = &h->stack[d];
		if (e->list) lists++;
		if (e->quote) quote = R_TRUE;
		if (e->heading) kind = RVN_RT_HEADING_1 + rvn_mini(e->heading, 3) - 1;
		if (e->item) {
			/* the kind of the list the item is in */
			int k, list_kind = RVN_RT_BULLET;
			for (k = d - 1; k >= 0; k--) {
				if (h->stack[k].list) {
					list_kind = h->stack[k].list;
					break;
				}
			}
			kind = list_kind;
			indent = rvn_maxi(lists - 1, 0);
		}
	}
	for (d = 0; d < h->depth; d++) if (h->stack[d].pre) kind = RVN_RT_CODE_BLOCK;
	if (kind == RVN_RT_PARAGRAPH && quote) kind = RVN_RT_QUOTE;
	if (!rvn_rt_kind_is_list(kind)) indent = 0;
	rvn_rt_add_para(h->doc, kind, indent);
	h->para = (int)h->doc->count - 1;
	h->kept = R_FALSE;
	h->space = R_FALSE;
	return &h->doc->paras[h->para];
}

static int rvn_rt_html_in_pre(const rvn_rt_html_t* h) {
	int d;
	for (d = 0; d < h->depth; d++) if (h->stack[d].pre) return R_TRUE;
	return R_FALSE;
}

static void rvn_rt_html_look(const rvn_rt_html_t* h, uint32_t* style, const char** link) {
	int d;
	*style = 0;
	*link = NULL;
	for (d = 0; d < h->depth; d++) {
		*style = (*style | h->stack[d].add) & ~h->stack[d].remove;
		if (h->stack[d].link) *link = h->stack[d].link;
	}
}

static void rvn_rt_html_put(rvn_rt_html_t* h, const char* text, int length) {
	rvn_rt_para_t* para = rvn_rt_html_para(h);
	uint32_t style;
	const char* link;
	rvn_rt_html_look(h, &style, &link);
	if (para->kind == RVN_RT_CODE_BLOCK) {
		style = 0;
		link = NULL;
	}
	rvn_rt_para_insert(para, para->length, text, length, style, link);
	h->kept = R_TRUE;
}

static void rvn_rt_html_text(rvn_rt_html_t* h, const char* text, int length) {
	int i, from = 0;
	if (rvn_rt_html_in_pre(h)) {
		/* each line of preformatted text is a line of code */
		for (i = 0; i <= length; i++) {
			int stop = i;
			if (i < length && text[i] != '\n') continue;
			if (stop > from && text[stop - 1] == '\r') stop--;
			if (stop > from) rvn_rt_html_put(h, text + from, stop - from);
			if (i < length) {
				rvn_rt_html_para(h);
				h->kept = R_TRUE;
				rvn_rt_html_end(h);
			}
			from = i + 1;
		}
		return;
	}
	/* white space is one space, written with the looks it is in (none at the start of a paragraph) */
	for (i = 0; i < length; i++) {
		char c = text[i];
		if (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\f') continue;
		if (i > from) rvn_rt_html_put(h, text + from, i - from);
		from = i + 1;
		if (h->para >= 0) {
			const rvn_rt_para_t* para = &h->doc->paras[h->para];
			if (para->length > 0 && para->text[para->length - 1] != '\n' && para->text[para->length - 1] != ' ') rvn_rt_html_put(h, " ", 1);
		}
	}
	if (length > from) rvn_rt_html_put(h, text + from, length - from);
}

static const struct { const char* name; const char* text; } g_rt_entities[] = {
	{ "amp", "&" }, { "lt", "<" }, { "gt", ">" }, { "quot", "\"" }, { "apos", "'" }, { "nbsp", "\xC2\xA0" },
	{ "copy", "\xC2\xA9" }, { "reg", "\xC2\xAE" }, { "trade", "\xE2\x84\xA2" }, { "hellip", "\xE2\x80\xA6" },
	{ "mdash", "\xE2\x80\x94" }, { "ndash", "\xE2\x80\x93" }, { "lsquo", "\xE2\x80\x98" }, { "rsquo", "\xE2\x80\x99" },
	{ "ldquo", "\xE2\x80\x9C" }, { "rdquo", "\xE2\x80\x9D" }, { "bull", "\xE2\x80\xA2" }, { "middot", "\xC2\xB7" },
	{ "euro", "\xE2\x82\xAC" }, { "laquo", "\xC2\xAB" }, { "raquo", "\xC2\xBB" }, { "times", "\xC3\x97" },
	{ "rarr", "\xE2\x86\x92" }, { "larr", "\xE2\x86\x90" }, { "deg", "\xC2\xB0" }, { "shy", "" }
};

/* Text with its entities decoded, appended to `out`. */
static void rvn_rt_html_decode(rvn_text_t* out, const char* p, const char* end) {
	while (p < end) {
		const char* amp = (const char*)memchr(p, '&', (size_t)(end - p));
		const char* semi;
		size_t i, length;
		if (!amp) {
			rvn_text_append(out, p, (size_t)(end - p));
			return;
		}
		rvn_text_append(out, p, (size_t)(amp - p));
		semi = amp + 1;
		while (semi < end && semi - amp < 12 && *semi != ';' && *semi != '&' && *semi != '<' && !isspace((unsigned char)*semi)) semi++;
		if (semi >= end || *semi != ';') {
			rvn_text_append(out, "&", 1);
			p = amp + 1;
			continue;
		}
		length = (size_t)(semi - amp - 1);
		if (length >= 2 && amp[1] == '#') {
			uint32_t code = 0;
			const char* digit = amp + 2;
			int hex = *digit == 'x' || *digit == 'X', valid = R_TRUE;
			char utf8[4];
			if (hex) digit++;
			if (digit == semi) valid = R_FALSE;
			for (; digit < semi && valid; digit++) {
				char c = *digit;
				if (c >= '0' && c <= '9') code = code * (hex ? 16u : 10u) + (uint32_t)(c - '0');
				else if (hex && c >= 'a' && c <= 'f') code = code * 16u + (uint32_t)(c - 'a' + 10);
				else if (hex && c >= 'A' && c <= 'F') code = code * 16u + (uint32_t)(c - 'A' + 10);
				else valid = R_FALSE;
				if (code > 0x10FFFFu) valid = R_FALSE;
			}
			if (valid && code >= 32) rvn_text_append(out, utf8, (size_t)rvn_utf8_encode(code, utf8));
			else if (valid && (code == '\n' || code == '\t')) rvn_text_append(out, " ", 1);
			p = semi + 1;
			continue;
		}
		for (i = 0; i < RVN_COUNT_OF(g_rt_entities); i++) {
			if (strlen(g_rt_entities[i].name) == length && memcmp(g_rt_entities[i].name, amp + 1, length) == 0) break;
		}
		if (i < RVN_COUNT_OF(g_rt_entities)) {
			rvn_text_add(out, g_rt_entities[i].text);
			p = semi + 1;
		} else {
			rvn_text_append(out, "&", 1);
			p = amp + 1;
		}
	}
}

/* An address that cannot run anything: the schemes that do are refused. */
static char* rvn_rt_safe_url(const char* value, size_t length) {
	rvn_text_t decoded = { NULL, 0, 0 };
	char scheme[16];
	size_t i, n = 0;
	const char* p;
	rvn_rt_html_decode(&decoded, value, value + length);
	if (!decoded.data) return NULL;
	/* (the spaces and control characters a browser skips: "java\tscript:") */
	for (p = decoded.data; *p && n < sizeof(scheme) - 1; p++) {
		unsigned char c = (unsigned char)*p;
		if (c <= ' ') continue;
		if (c == ':') break;
		scheme[n++] = (char)tolower(c);
	}
	scheme[n] = '\0';
	if (*p == ':' && (strcmp(scheme, "javascript") == 0 || strcmp(scheme, "vbscript") == 0 || strcmp(scheme, "data") == 0)) {
		rvn_free(decoded.data);
		return NULL;
	}
	/* trimmed */
	for (i = 0; decoded.data[i] && (unsigned char)decoded.data[i] <= ' '; i++) {}
	{
		char* url = rvn_strdup(decoded.data + i);
		size_t end = strlen(url);
		while (end > 0 && (unsigned char)url[end - 1] <= ' ') url[--end] = '\0';
		rvn_free(decoded.data);
		if (!url[0]) {
			rvn_free(url);
			return NULL;
		}
		return url;
	}
}

/* The looks a style attribute gives: font-weight, font-style, text-decoration, a monospace font-family. */
static void rvn_rt_css(rvn_rt_element_t* e, const char* style, size_t length) {
	rvn_text_t lower = { NULL, 0, 0 };
	const char* p;
	size_t i;
	rvn_text_append(&lower, style, length);
	if (!lower.data) return;
	for (i = 0; i < lower.length; i++) lower.data[i] = (char)tolower((unsigned char)lower.data[i]);
	p = lower.data;
	while (*p) {
		const char* colon = strchr(p, ':');
		const char* semi;
		char property[32], value[96];
		size_t n;
		if (!colon) break;
		semi = strchr(colon, ';');
		if (!semi) semi = colon + strlen(colon);
		for (n = 0; p < colon && n < sizeof(property) - 1; p++) if (!isspace((unsigned char)*p)) property[n++] = *p;
		property[n] = '\0';
		for (n = 0, p = colon + 1; p < semi && n < sizeof(value) - 1; p++) value[n++] = *p;
		value[n] = '\0';
		if (strcmp(property, "font-weight") == 0) {
			int weight = atoi(value);
			if (strstr(value, "bold") || weight >= 600) {
				e->add |= RVN_RT_BOLD;
				e->remove &= ~RVN_RT_BOLD;
			} else if (strstr(value, "normal") || (weight > 0 && weight < 600)) {
				e->remove |= RVN_RT_BOLD;
				e->add &= ~RVN_RT_BOLD;
			}
		} else if (strcmp(property, "font-style") == 0) {
			if (strstr(value, "italic") || strstr(value, "oblique")) {
				e->add |= RVN_RT_ITALIC;
				e->remove &= ~RVN_RT_ITALIC;
			} else if (strstr(value, "normal")) {
				e->remove |= RVN_RT_ITALIC;
				e->add &= ~RVN_RT_ITALIC;
			}
		} else if (strcmp(property, "text-decoration") == 0 || strcmp(property, "text-decoration-line") == 0) {
			if (strstr(value, "underline")) e->add |= RVN_RT_UNDERLINE;
			if (strstr(value, "line-through")) e->add |= RVN_RT_STRIKE;
			if (strstr(value, "none")) e->remove |= RVN_RT_UNDERLINE | RVN_RT_STRIKE;
		} else if (strcmp(property, "font-family") == 0) {
			if (strstr(value, "mono") || strstr(value, "courier") || strstr(value, "consolas") || strstr(value, "menlo")) e->add |= RVN_RT_CODE;
		}
		p = *semi ? semi + 1 : semi;
	}
	rvn_free(lower.data);
}

static int rvn_rt_tag_is(const char* name, const char* const* list) {
	for (; *list; list++) if (strcmp(name, *list) == 0) return R_TRUE;
	return R_FALSE;
}

static void rvn_rt_html_pop(rvn_rt_html_t* h) {
	rvn_rt_element_t* e = &h->stack[--h->depth];
	if (e->block) rvn_rt_html_end(h);
	else if (e->pre) rvn_rt_html_end(h);
	rvn_free(e->link);
	e->link = NULL;
}

void rvn_rt_from_html(rvn_rt_doc_t* doc, const char* html) {
	static const char* const skipped[] = { "script", "style", "head", "title", "template", "noscript", "textarea", "select",
		"svg", "math", "iframe", "object", "button", NULL };
	static const char* const empty[] = { "br", "hr", "img", "meta", "link", "input", "wbr", "col", "area", "base", "source",
		"param", "track", "embed", NULL };
	static const char* const blocks[] = { "p", "div", "section", "article", "header", "footer", "main", "nav", "aside",
		"address", "figure", "figcaption", "dl", "dt", "dd", "table", "thead", "tbody", "tfoot", "tr", "caption", "form",
		"fieldset", "details", "summary", "center", "body", "html", NULL };
	rvn_rt_html_t h;
	rvn_text_t text = { NULL, 0, 0 };
	const char* p = html ? html : "";
	const char* end = p + strlen(p);

	rvn_rt_doc_free(doc);
	memset(&h, 0, sizeof(h));
	h.doc = doc;
	h.para = -1;

	while (p < end) {
		const char* lt = (const char*)memchr(p, '<', (size_t)(end - p));
		const char* q;
		char name[12];
		size_t n = 0;
		int closing = R_FALSE, self_closing = R_FALSE;
		rvn_rt_element_t e;

		/* text up to the next tag */
		if (lt != p) {
			const char* stop = lt ? lt : end;
			text.length = 0;
			rvn_rt_html_decode(&text, p, stop);
			if (text.length) rvn_rt_html_text(&h, text.data, (int)text.length);
			p = stop;
			if (!lt) break;
		}
		/* comments, <!DOCTYPE>, <?xml?> */
		if (end - p >= 4 && memcmp(p, "<!--", 4) == 0) {
			const char* close = strstr(p + 4, "-->");
			p = close ? close + 3 : end;
			continue;
		}
		if (p + 1 < end && (p[1] == '!' || p[1] == '?')) {
			const char* close = (const char*)memchr(p, '>', (size_t)(end - p));
			p = close ? close + 1 : end;
			continue;
		}
		q = p + 1;
		if (q < end && *q == '/') {
			closing = R_TRUE;
			q++;
		}
		if (q >= end || !isalpha((unsigned char)*q)) {
			/* a '<' that starts no tag is text */
			rvn_rt_html_text(&h, "<", 1);
			p++;
			continue;
		}
		while (q < end && (isalnum((unsigned char)*q) || *q == '-' || *q == ':')) {
			if (n < sizeof(name) - 1) name[n++] = (char)tolower((unsigned char)*q);
			q++;
		}
		name[n] = '\0';

		/* the attributes: only href and style are read */
		memset(&e, 0, sizeof(e));
		memcpy(e.name, name, n + 1);
		while (q < end && *q != '>') {
			const char* attribute;
			size_t attribute_length;
			const char* value = NULL;
			size_t value_length = 0;
			if (isspace((unsigned char)*q)) {
				q++;
				continue;
			}
			if (*q == '/') {
				self_closing = R_TRUE;
				q++;
				continue;
			}
			attribute = q;
			while (q < end && !isspace((unsigned char)*q) && *q != '=' && *q != '>' && *q != '/') q++;
			attribute_length = (size_t)(q - attribute);
			while (q < end && isspace((unsigned char)*q)) q++;
			if (q < end && *q == '=') {
				q++;
				while (q < end && isspace((unsigned char)*q)) q++;
				if (q < end && (*q == '"' || *q == '\'')) {
					char quote = *q++;
					value = q;
					while (q < end && *q != quote) q++;
					value_length = (size_t)(q - value);
					if (q < end) q++;
				} else {
					value = q;
					while (q < end && !isspace((unsigned char)*q) && *q != '>') q++;
					value_length = (size_t)(q - value);
				}
			}
			if (!value || closing) continue;
			if (attribute_length == 4 && rvn_strnicmp(attribute, "href", 4) == 0 && strcmp(name, "a") == 0) {
				rvn_free(e.link);
				e.link = rvn_rt_safe_url(value, value_length);
			} else if (attribute_length == 5 && rvn_strnicmp(attribute, "style", 5) == 0) {
				rvn_rt_css(&e, value, value_length);
			}
		}
		p = q < end ? q + 1 : end;

		if (closing) {
			int d;
			for (d = h.depth - 1; d >= 0; d--) if (strcmp(h.stack[d].name, name) == 0) break;
			/* (a tag closed that was never opened is nothing) */
			if (d >= 0) while (h.depth > d) rvn_rt_html_pop(&h);
			rvn_free(e.link);
			continue;
		}

		if (rvn_rt_tag_is(name, skipped)) {
			/* what is inside is dropped, up to the tag that closes it */
			const char* close = p;
			rvn_free(e.link);
			for (;;) {
				close = strstr(close, "</");
				if (!close) {
					p = end;
					break;
				}
				if (rvn_strnicmp(close + 2, name, n) == 0 && !isalnum((unsigned char)close[2 + n])) {
					const char* gt = strchr(close, '>');
					p = gt ? gt + 1 : end;
					break;
				}
				close += 2;
			}
			continue;
		}
		if (rvn_rt_tag_is(name, empty)) {
			rvn_free(e.link);
			if (strcmp(name, "br") == 0) {
				rvn_rt_para_t* para = rvn_rt_html_para(&h);
				if (para->kind == RVN_RT_CODE_BLOCK) {
					h.kept = R_TRUE;
					rvn_rt_html_end(&h);
				} else if (para->length > 0) {
					rvn_rt_html_put(&h, "\n", 1);
				} else {
					h.kept = R_TRUE;
				}
				h.space = R_FALSE;
			} else if (strcmp(name, "hr") == 0) {
				rvn_rt_html_end(&h);
			}
			continue;
		}

		/* what the element means */
		if (rvn_rt_tag_is(name, blocks)) e.block = R_TRUE;
		if (name[0] == 'h' && name[1] >= '1' && name[1] <= '6' && !name[2]) {
			e.block = R_TRUE;
			e.heading = (uint8_t)(name[1] - '0');
		} else if (strcmp(name, "ul") == 0 || strcmp(name, "ol") == 0 || strcmp(name, "menu") == 0) {
			e.block = R_TRUE;
			e.list = (uint8_t)(name[0] == 'o' ? RVN_RT_NUMBERED : RVN_RT_BULLET);
		} else if (strcmp(name, "li") == 0) {
			e.block = R_TRUE;
			e.item = R_TRUE;
		} else if (strcmp(name, "blockquote") == 0) {
			e.block = R_TRUE;
			e.quote = R_TRUE;
		} else if (strcmp(name, "pre") == 0) {
			e.block = R_TRUE;
			e.pre = R_TRUE;
		} else if (strcmp(name, "b") == 0 || strcmp(name, "strong") == 0) {
			if (!(e.remove & RVN_RT_BOLD)) e.add |= RVN_RT_BOLD;
		} else if (strcmp(name, "i") == 0 || strcmp(name, "em") == 0 || strcmp(name, "cite") == 0 || strcmp(name, "dfn") == 0) {
			if (!(e.remove & RVN_RT_ITALIC)) e.add |= RVN_RT_ITALIC;
		} else if (strcmp(name, "u") == 0 || strcmp(name, "ins") == 0) {
			e.add |= RVN_RT_UNDERLINE;
		} else if (strcmp(name, "s") == 0 || strcmp(name, "strike") == 0 || strcmp(name, "del") == 0) {
			e.add |= RVN_RT_STRIKE;
		} else if (strcmp(name, "code") == 0 || strcmp(name, "tt") == 0 || strcmp(name, "kbd") == 0 || strcmp(name, "samp") == 0) {
			e.add |= RVN_RT_CODE;
		}
		if (e.block) rvn_rt_html_end(&h);
		if (self_closing || h.depth >= RVN_RT_HTML_DEPTH) {
			rvn_free(e.link);
			continue;
		}
		h.stack[h.depth++] = e;
	}
	while (h.depth > 0) rvn_rt_html_pop(&h);
	rvn_rt_html_end(&h);
	rvn_free(text.data);
	if (doc->count == 0) rvn_rt_add_para(doc, RVN_RT_PARAGRAPH, 0);
}
