/**
 * Raven - widgets/rich_text.c
 * RichTextEditor: styled text edited (notes, mails, messages, comments).
 *
 * The document (rich_text.h, rich_text_doc.c) is an array of paragraphs,
 * each one a UTF-8 text and the runs of its looks. The widget lays out each
 * paragraph on the width of the view (lines of fragments, a fragment being
 * a piece of a line with one font), keeps that layout until the paragraph
 * or the width changes, and keeps the top of every paragraph: painting and
 * pointing look for the paragraphs on screen by halves, so a document of
 * thousands of paragraphs costs the few on screen. Typing lays out one
 * paragraph again and adds up the heights (a loop over floats).
 *
 * Every change goes between rvn_rt_begin and rvn_rt_end, which save copies
 * of the paragraphs it touches and the selection before and after: undoing
 * swaps the copies back. Characters typed in a row in one paragraph make
 * one step (up to RVN_RT_TYPING_GROUP of them).
 *
 * The toolbar and the bar where a link is typed are drawn by the widget
 * itself, above the text, as the find bar of TextEditor.
 */

#include "rich_text.h"
#include "../render/bidi.h"
#include "../render/renderer.h"
#include "../xml/xml.h"

#define RVN_RT_PAD 12.0f
/** Characters typed in a row are undone together, up to this many. */
#define RVN_RT_TYPING_GROUP 48
#define RVN_RT_MAX_UNDO 400

enum { RVN_RT_TOOL_STYLE, RVN_RT_TOOL_LINK, RVN_RT_TOOL_BLOCK, RVN_RT_TOOL_GAP };

static const struct { const char* icon; int kind; uint32_t value; } g_rt_tools[] = {
	{ "bold", RVN_RT_TOOL_STYLE, RVN_RT_BOLD },
	{ "italic", RVN_RT_TOOL_STYLE, RVN_RT_ITALIC },
	{ "underline", RVN_RT_TOOL_STYLE, RVN_RT_UNDERLINE },
	{ "strikethrough", RVN_RT_TOOL_STYLE, RVN_RT_STRIKE },
	{ "code", RVN_RT_TOOL_STYLE, RVN_RT_CODE },
	{ "link", RVN_RT_TOOL_LINK, 0 },
	{ NULL, RVN_RT_TOOL_GAP, 0 },
	{ "heading-1", RVN_RT_TOOL_BLOCK, RVN_RT_HEADING_1 },
	{ "heading-2", RVN_RT_TOOL_BLOCK, RVN_RT_HEADING_2 },
	{ "heading-3", RVN_RT_TOOL_BLOCK, RVN_RT_HEADING_3 },
	{ NULL, RVN_RT_TOOL_GAP, 0 },
	{ "list", RVN_RT_TOOL_BLOCK, RVN_RT_BULLET },
	{ "list-ordered", RVN_RT_TOOL_BLOCK, RVN_RT_NUMBERED },
	{ "quote", RVN_RT_TOOL_BLOCK, RVN_RT_QUOTE },
	{ "code-block", RVN_RT_TOOL_BLOCK, RVN_RT_CODE_BLOCK }
};
#define RVN_RT_TOOL_COUNT ((int)RVN_COUNT_OF(g_rt_tools))

typedef struct rvn_rt_sel_t {
	rvn_rt_pos_t anchor, caret;
} rvn_rt_sel_t;

/** A step of the history: the paragraphs first .. first + live_count were saved_count others (saved). */
typedef struct rvn_rt_step_t {
	uint32_t first;
	rvn_rt_para_t* saved;
	uint32_t saved_count;
	uint32_t live_count;
	rvn_rt_sel_t before, after;
	/** Characters typed into it (0: not a step of typing). */
	int typing;
} rvn_rt_step_t;

/** What the layout of a paragraph is made from: a word with the spaces after it, in one run. */
typedef struct rvn_rt_atom_t {
	int start, word_end, end;
	int run;
	/** A line break ('\n'). */
	int hard;
	Rid font;
	float size, height, base;
	float word_w, space_w;
} rvn_rt_atom_t;

typedef struct rvn_rich_text_t {
	rvn_widget_t base;
	rvn_rt_doc_t doc;
	rvn_rt_pos_t caret, anchor;
	/** Where the caret stays across while it goes up and down (-1: where it is). */
	float goal_x;
	/** The looks of what is typed next, chosen at the caret (Ctrl+B with no selection). */
	uint32_t typing_style;
	int typing_set;

	rvn_rt_step_t* undo;
	uint32_t undo_count, undo_cap;
	rvn_rt_step_t* redo;
	uint32_t redo_count, redo_cap;
	/** The step being made (rvn_rt_begin .. rvn_rt_end) and the number of paragraphs before it. */
	int editing;
	uint32_t count_before;

	int read_only;
	int toolbar;
	int shortcuts;
	char* placeholder;
	Rid font;
	float text_size;

	/* layout */
	rvn_scrollbar_t bar;
	float* tops;
	uint32_t tops_cap;
	int* numbers;
	uint32_t numbers_cap;
	int tops_dirty;
	float content_h;
	rvn_rt_atom_t* atoms;
	uint32_t atom_cap;
	int reveal;

	/* the mouse */
	int selecting;
	/** A double / triple click: what the drag extends by (1 character, 2 word, 3 paragraph) and what was selected first. */
	int unit;
	rvn_rt_pos_t unit_from, unit_to;
	int hot_tool, pressed_tool;
	Rid icons[RVN_COUNT_OF(g_rt_tools)];
	Rid blink_timer;
	double blink_start;

	/* the bar of the link */
	int link_open;
	int link_ready;
	rvn_textedit_t link_edit;
	rvn_rt_sel_t link_sel;
	int link_selecting;

	/* what was told to the application */
	uint32_t version;
	int changed_pending;
	rvn_rt_sel_t told;
	RRichTextChangedCallback callback;
	void* user_data;
	RRichTextChangedCallback selection_callback;
	void* selection_user_data;
	RRichTextLinkCallback link_callback;
	void* link_user_data;

	rvn_spelling_t spelling;
	/* the texts returned by the API */
	rvn_text_t markdown_cache, html_cache, text_cache, scratch;
} rvn_rich_text_t;

typedef struct rvn_rt_metrics_t {
	Rid font;
	float size;
	float button;
	rvn_rect_t toolbar;
	rvn_rect_t linkbar;
	/** Where the text shows. */
	rvn_rect_t view;
	rvn_rect_t track;
	/** The corner of the document in the widget, before scrolling. */
	float text_x, text_y;
	/** The width the paragraphs are laid out on. */
	float width;
} rvn_rt_metrics_t;

static const char* rvn_rt_tr(const char* key, const char* english) {
	return Rtr_exists(key) ? Rtr(key) : english;
}

/* ---------------------------------------------------------------------------
 *  Positions
 * ------------------------------------------------------------------------- */

static rvn_rt_pos_t rvn_rt_pos(int para, int offset) {
	rvn_rt_pos_t pos;
	pos.para = para;
	pos.offset = offset;
	return pos;
}

static rvn_rt_pos_t rvn_rt_clamp(const rvn_rich_text_t* rt, rvn_rt_pos_t pos) {
	const rvn_rt_para_t* para;
	pos.para = rvn_clampi(pos.para, 0, (int)rt->doc.count - 1);
	para = &rt->doc.paras[pos.para];
	pos.offset = rvn_clampi(pos.offset, 0, para->length);
	/* (on a character) */
	while (pos.offset > 0 && pos.offset < para->length && ((unsigned char)para->text[pos.offset] & 0xC0) == 0x80) pos.offset--;
	return pos;
}

static int rvn_rt_has_selection(const rvn_rich_text_t* rt) {
	return rvn_rt_pos_cmp(rt->caret, rt->anchor) != 0;
}

static rvn_rt_pos_t rvn_rt_sel_min(const rvn_rich_text_t* rt) {
	return rvn_rt_pos_cmp(rt->caret, rt->anchor) <= 0 ? rt->caret : rt->anchor;
}

static rvn_rt_pos_t rvn_rt_sel_max(const rvn_rich_text_t* rt) {
	return rvn_rt_pos_cmp(rt->caret, rt->anchor) >= 0 ? rt->caret : rt->anchor;
}

typedef enum rvn_rt_class_t { RVN_RT_SPACE, RVN_RT_WORD, RVN_RT_PUNCTUATION } rvn_rt_class_t;

static rvn_rt_class_t rvn_rt_class(char c) {
	if (c == ' ' || c == '\t' || c == '\n') return RVN_RT_SPACE;
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || (unsigned char)c >= 0x80) return RVN_RT_WORD;
	return RVN_RT_PUNCTUATION;
}

static rvn_rt_pos_t rvn_rt_char_left(const rvn_rich_text_t* rt, rvn_rt_pos_t pos) {
	if (pos.offset > 0) return rvn_rt_pos(pos.para, rvn_utf8_prev(rt->doc.paras[pos.para].text, pos.offset));
	if (pos.para > 0) return rvn_rt_pos(pos.para - 1, rt->doc.paras[pos.para - 1].length);
	return pos;
}

static rvn_rt_pos_t rvn_rt_char_right(const rvn_rich_text_t* rt, rvn_rt_pos_t pos) {
	const rvn_rt_para_t* para = &rt->doc.paras[pos.para];
	if (pos.offset < para->length) return rvn_rt_pos(pos.para, rvn_utf8_next(para->text, para->length, pos.offset));
	if (pos.para + 1 < (int)rt->doc.count) return rvn_rt_pos(pos.para + 1, 0);
	return pos;
}

static rvn_rt_pos_t rvn_rt_word_left(const rvn_rich_text_t* rt, rvn_rt_pos_t pos) {
	const char* text = rt->doc.paras[pos.para].text;
	int at = pos.offset;
	rvn_rt_class_t cls;
	if (at == 0) return rvn_rt_char_left(rt, pos);
	while (at > 0 && rvn_rt_class(text[at - 1]) == RVN_RT_SPACE) at--;
	if (at == 0) return rvn_rt_pos(pos.para, 0);
	cls = rvn_rt_class(text[at - 1]);
	while (at > 0 && rvn_rt_class(text[at - 1]) == cls) at--;
	return rvn_rt_pos(pos.para, at);
}

static rvn_rt_pos_t rvn_rt_word_right(const rvn_rich_text_t* rt, rvn_rt_pos_t pos) {
	const rvn_rt_para_t* para = &rt->doc.paras[pos.para];
	int at = pos.offset;
	if (at >= para->length) return rvn_rt_char_right(rt, pos);
	if (rvn_rt_class(para->text[at]) != RVN_RT_SPACE) {
		rvn_rt_class_t cls = rvn_rt_class(para->text[at]);
		while (at < para->length && rvn_rt_class(para->text[at]) == cls) at++;
	}
	while (at < para->length && rvn_rt_class(para->text[at]) == RVN_RT_SPACE) at++;
	return rvn_rt_pos(pos.para, at);
}

/* The word around a position (a double click). */
static void rvn_rt_word_at(const rvn_rich_text_t* rt, rvn_rt_pos_t pos, rvn_rt_pos_t* from, rvn_rt_pos_t* to) {
	const rvn_rt_para_t* para = &rt->doc.paras[pos.para];
	int a = pos.offset, b = pos.offset;
	rvn_rt_class_t cls;
	if (para->length == 0) {
		*from = *to = pos;
		return;
	}
	if (b >= para->length || (a > 0 && rvn_rt_class(para->text[a]) == RVN_RT_SPACE && rvn_rt_class(para->text[a - 1]) != RVN_RT_SPACE)) a--, b--;
	a = rvn_maxi(a, 0);
	cls = rvn_rt_class(para->text[a]);
	while (a > 0 && rvn_rt_class(para->text[a - 1]) == cls) a--;
	b = a;
	while (b < para->length && rvn_rt_class(para->text[b]) == cls) b++;
	*from = rvn_rt_pos(pos.para, a);
	*to = rvn_rt_pos(pos.para, b);
}

/* ---------------------------------------------------------------------------
 *  Metrics and layout
 * ------------------------------------------------------------------------- */

static void rvn_rt_metrics(rvn_rich_text_t* rt, rvn_rt_metrics_t* m) {
	rvn_widget_t* widget = &rt->base;
	float bar = rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE);
	float w = widget->rect.w, h = widget->rect.h, y = 1.0f;
	m->font = rt->font;
	m->size = rvn_widget_font_size(widget, rt->text_size);
	m->button = floorf(m->size * 2.0f + 0.5f);
	m->toolbar = rvn_rect(1.0f, 1.0f, rvn_maxf(0.0f, w - 2.0f), 0.0f);
	if (rt->toolbar) {
		m->toolbar.h = m->button + 10.0f;
		y += m->toolbar.h;
	}
	m->linkbar = rvn_rect(1.0f, y, rvn_maxf(0.0f, w - 2.0f), 0.0f);
	if (rt->link_open) {
		m->linkbar.h = floorf(m->size * 2.6f + 0.5f);
		y += m->linkbar.h;
	}
	m->view = rvn_rect(1.0f, y, rvn_maxf(0.0f, w - 2.0f), rvn_maxf(0.0f, h - 1.0f - y));
	m->track = rvn_rect(w - 1.0f - bar, m->view.y, bar, m->view.h);
	m->text_x = m->view.x + RVN_RT_PAD;
	m->text_y = m->view.y + RVN_RT_PAD;
	/* (room for the scrollbar is always kept: the text does not move when it comes) */
	m->width = rvn_maxf(40.0f, m->view.w - 2.0f * RVN_RT_PAD - bar);
}

static float rvn_rt_kind_size(int kind, float size) {
	switch (kind) {
		case RVN_RT_HEADING_1: return floorf(size * 1.6f + 0.5f);
		case RVN_RT_HEADING_2: return floorf(size * 1.32f + 0.5f);
		case RVN_RT_HEADING_3: return floorf(size * 1.14f + 0.5f);
		case RVN_RT_CODE_BLOCK: return size * 0.92f;
		default: return size;
	}
}

static int rvn_rt_kind_is_heading(int kind) {
	return kind == RVN_RT_HEADING_1 || kind == RVN_RT_HEADING_2 || kind == RVN_RT_HEADING_3;
}

static float rvn_rt_list_indent(float size) {
	return floorf(size * 1.6f);
}

/* Where the text of a paragraph starts across. */
static float rvn_rt_indent_x(const rvn_rt_para_t* para, float size) {
	if (rvn_rt_kind_is_list(para->kind)) return rvn_rt_list_indent(size) * (float)(para->indent + 1);
	if (para->kind == RVN_RT_QUOTE) return floorf(size * 1.2f);
	if (para->kind == RVN_RT_CODE_BLOCK) return floorf(size * 0.8f);
	return 0.0f;
}

static float rvn_rt_code_pad(float size) {
	return floorf(size * 0.6f);
}

static void rvn_rt_font_box(Rid font, float size, float* height, float* base) {
	rvn_font_metrics_t fm;
	rvn_font_metrics(rvn_font_resolve(font), size, &fm);
	*height = fm.line_height;
	*base = fm.ascent + (fm.line_height - fm.ascent - fm.descent) * 0.5f;
}

/* The font of a run of a paragraph, and its size. */
static Rid rvn_rt_run_font(const rvn_rt_metrics_t* m, const rvn_rt_para_t* para, uint32_t style, float* size) {
	float kind_size = rvn_rt_kind_size(para->kind, m->size);
	uint32_t variant = rvn_rt_kind_is_heading(para->kind) ? RVN_FONT_BOLD : 0u;
	if (para->kind == RVN_RT_CODE_BLOCK) {
		*size = kind_size;
		return g_rvn.mono_font;
	}
	if (style & RVN_RT_CODE) {
		*size = kind_size * 0.92f;
		return g_rvn.mono_font;
	}
	if (style & RVN_RT_BOLD) variant |= RVN_FONT_BOLD;
	if (style & RVN_RT_ITALIC) variant |= RVN_FONT_ITALIC;
	*size = kind_size;
	return rvn_font_variant(m->font, variant);
}

typedef struct rvn_rt_breaker_t {
	rvn_rt_para_t* para;
	float x0;
	float pen;
	int line_start;
	uint32_t line_first;
} rvn_rt_breaker_t;

static void rvn_rt_end_line(rvn_rt_breaker_t* b, int end) {
	rvn_rt_para_t* para = b->para;
	rvn_rt_line_t* line;
	RVN_ARRAY_RESERVE(para->lines, para->line_cap, para->line_count + 1);
	line = &para->lines[para->line_count++];
	memset(line, 0, sizeof(*line));
	line->start = b->line_start;
	line->end = end;
	line->first_frag = (int)b->line_first;
	line->frag_count = (int)(para->frag_count - b->line_first);
	line->x = b->x0;
	b->line_start = end;
	b->line_first = para->frag_count;
	b->pen = 0.0f;
}

static void rvn_rt_place(rvn_rt_breaker_t* b, const rvn_rt_atom_t* atom, int start, int end, float width) {
	rvn_rt_para_t* para = b->para;
	rvn_rt_frag_t* frag;
	if (para->frag_count > b->line_first) {
		frag = &para->frags[para->frag_count - 1];
		if (frag->run == atom->run && frag->font == atom->font && frag->start + frag->length == start) {
			frag->length += end - start;
			frag->w += width;
			b->pen += width;
			return;
		}
	}
	RVN_ARRAY_RESERVE(para->frags, para->frag_cap, para->frag_count + 1);
	frag = &para->frags[para->frag_count++];
	memset(frag, 0, sizeof(*frag));
	frag->x = b->x0 + b->pen;
	frag->w = width;
	frag->start = start;
	frag->length = end - start;
	frag->run = atom->run;
	frag->line = (int)para->line_count;
	frag->font = atom->font;
	frag->size = atom->size;
	frag->height = atom->height;
	frag->base = atom->base;
	b->pen += width;
}

/* Lays a paragraph out on `width`: its lines, its fragments, its height. */
static void rvn_rt_layout_para(rvn_rich_text_t* rt, const rvn_rt_metrics_t* m, rvn_rt_para_t* para) {
	rvn_rt_breaker_t b;
	float x0 = rvn_rt_indent_x(para, m->size), avail, base_size, base_height, base_base, y = 0.0f;
	Rid base_font;
	uint32_t count = 0, r, l;
	int at = 0, i;

	para->line_count = 0;
	para->frag_count = 0;
	avail = rvn_maxf(m->size * 4.0f, m->width - x0 - (para->kind == RVN_RT_CODE_BLOCK ? x0 : 0.0f));
	base_font = rvn_rt_run_font(m, para, 0, &base_size);
	rvn_rt_font_box(base_font, base_size, &base_height, &base_base);

	/* the atoms: words and their spaces, line breaks, run after run */
	for (r = 0; r < para->run_count; r++) {
		const rvn_rt_run_t* run = &para->runs[r];
		int stop = at + run->length, p = at;
		float size, height, base;
		Rid font = rvn_rt_run_font(m, para, run->style, &size);
		rvn_font_t* resolved = rvn_font_resolve(font);
		rvn_rt_font_box(font, size, &height, &base);
		while (p < stop) {
			rvn_rt_atom_t* atom;
			RVN_ARRAY_RESERVE(rt->atoms, rt->atom_cap, count + 1);
			atom = &rt->atoms[count++];
			memset(atom, 0, sizeof(*atom));
			atom->run = (int)r;
			atom->font = font;
			atom->size = size;
			atom->height = height;
			atom->base = base;
			atom->start = p;
			if (para->text[p] == '\n') {
				atom->hard = R_TRUE;
				atom->word_end = atom->end = p + 1;
				p++;
				continue;
			}
			while (p < stop && para->text[p] != ' ' && para->text[p] != '\n') p++;
			atom->word_end = p;
			while (p < stop && para->text[p] == ' ') p++;
			atom->end = p;
			atom->word_w = rvn_text_width(resolved, size, para->text + atom->start, atom->word_end - atom->start);
			atom->space_w = atom->end > atom->word_end ? rvn_text_width(resolved, size, para->text + atom->word_end, atom->end - atom->word_end) : 0.0f;
		}
		at = stop;
	}

	/* the lines: whole words while they fit, a word longer than the line cut between its characters */
	memset(&b, 0, sizeof(b));
	b.para = para;
	b.x0 = x0;
	i = 0;
	while (i < (int)count) {
		int j = i, k;
		float nospace = 0.0f;
		if (rt->atoms[i].hard) {
			rvn_rt_end_line(&b, rt->atoms[i].end);
			i++;
			continue;
		}
		/* a group: the atoms up to the first one followed by a space (a word may be in several runs) */
		while (j + 1 < (int)count && rt->atoms[j].end == rt->atoms[j].word_end && !rt->atoms[j + 1].hard) j++;
		for (k = i; k <= j; k++) nospace += rt->atoms[k].word_w + (k < j ? rt->atoms[k].space_w : 0.0f);
		if (b.pen > 0.0f && b.pen + nospace > avail + 0.5f) rvn_rt_end_line(&b, rt->atoms[i].start);
		if (nospace > avail + 0.5f) {
			for (k = i; k <= j; k++) {
				const rvn_rt_atom_t* atom = &rt->atoms[k];
				rvn_font_t* resolved = rvn_font_resolve(atom->font);
				int c = atom->start;
				while (c < atom->end) {
					int next = rvn_utf8_next(para->text, atom->end, c);
					float cw = rvn_text_width(resolved, atom->size, para->text + c, next - c);
					if (b.pen > 0.0f && b.pen + cw > avail + 0.5f && para->text[c] != ' ') rvn_rt_end_line(&b, c);
					rvn_rt_place(&b, atom, c, next, cw);
					c = next;
				}
			}
		} else {
			for (k = i; k <= j; k++) rvn_rt_place(&b, &rt->atoms[k], rt->atoms[k].start, rt->atoms[k].end, rt->atoms[k].word_w + rt->atoms[k].space_w);
		}
		i = j + 1;
	}
	rvn_rt_end_line(&b, para->length);

	/* heights: the tallest font of each line, the fragments on one baseline */
	for (l = 0; l < para->line_count; l++) {
		rvn_rt_line_t* line = &para->lines[l];
		float height = line->frag_count ? 0.0f : base_height, base = line->frag_count ? 0.0f : base_base, leading;
		int f;
		for (f = line->first_frag; f < line->first_frag + line->frag_count; f++) {
			height = rvn_maxf(height, para->frags[f].height);
			base = rvn_maxf(base, para->frags[f].base);
		}
		leading = floorf(height * 0.32f + 0.5f);
		line->y = y;
		line->h = ceilf(height) + leading;
		for (f = line->first_frag; f < line->first_frag + line->frag_count; f++) {
			para->frags[f].y = y + floorf(leading * 0.5f) + (base - para->frags[f].base);
		}
		y += line->h;
	}

	/* a paragraph written right to left is set against the right edge (its fragments are drawn in visual order by the text renderer) */
	if (para->kind != RVN_RT_CODE_BLOCK && para->length > 0 && rvn_bidi_paragraph_rtl(para->text, para->length)) {
		for (l = 0; l < para->line_count; l++) {
			rvn_rt_line_t* line = &para->lines[l];
			float right = x0, shift;
			int f;
			for (f = line->first_frag; f < line->first_frag + line->frag_count; f++) right = rvn_maxf(right, para->frags[f].x + para->frags[f].w);
			shift = rvn_maxf(0.0f, x0 + avail - right);
			line->x += shift;
			for (f = line->first_frag; f < line->first_frag + line->frag_count; f++) para->frags[f].x += shift;
		}
	}
	para->height = y;
}

/* The space above a paragraph, from the one before it. */
static float rvn_rt_space_before(const rvn_rt_para_t* previous, const rvn_rt_para_t* para, float size) {
	float space = floorf(size * 0.5f);
	if (!previous) return para->kind == RVN_RT_CODE_BLOCK ? rvn_rt_code_pad(size) : 0.0f;
	if (previous->kind == RVN_RT_CODE_BLOCK && para->kind == RVN_RT_CODE_BLOCK) return 0.0f;
	if (rvn_rt_kind_is_heading(para->kind)) space = floorf(size * (para->kind == RVN_RT_HEADING_1 ? 1.0f : 0.85f));
	else if (rvn_rt_kind_is_heading(previous->kind)) space = floorf(size * 0.35f);
	else if (rvn_rt_kind_is_list(previous->kind) && rvn_rt_kind_is_list(para->kind)) space = floorf(size * 0.2f);
	else if (previous->kind == RVN_RT_QUOTE && para->kind == RVN_RT_QUOTE) space = floorf(size * 0.4f);
	if (para->kind == RVN_RT_CODE_BLOCK || previous->kind == RVN_RT_CODE_BLOCK) space += rvn_rt_code_pad(size);
	return space;
}

/* Lays out what changed, and places the paragraphs one under the other. */
static void rvn_rt_ensure(rvn_rich_text_t* rt, const rvn_rt_metrics_t* m) {
	uint32_t stamp = g_rvn.style_generation, i;
	int counters[RVN_RT_MAX_INDENT + 1];
	float y = 0.0f;
	for (i = 0; i < rt->doc.count; i++) {
		rvn_rt_para_t* para = &rt->doc.paras[i];
		if (para->laid && para->laid_width == m->width && para->laid_stamp == stamp) continue;
		rvn_rt_layout_para(rt, m, para);
		para->laid = R_TRUE;
		para->laid_width = m->width;
		para->laid_stamp = stamp;
		rt->tops_dirty = R_TRUE;
	}
	if (!rt->tops_dirty) return;
	rt->tops_dirty = R_FALSE;
	RVN_ARRAY_RESERVE(rt->tops, rt->tops_cap, rt->doc.count);
	RVN_ARRAY_RESERVE(rt->numbers, rt->numbers_cap, rt->doc.count);
	memset(counters, 0, sizeof(counters));
	for (i = 0; i < rt->doc.count; i++) {
		const rvn_rt_para_t* para = &rt->doc.paras[i];
		int k;
		y += rvn_rt_space_before(i > 0 ? &rt->doc.paras[i - 1] : NULL, para, m->size);
		rt->tops[i] = y;
		y += para->height;
		/* the numbers of numbered lists: per level, started again by anything that is not an item */
		rt->numbers[i] = 0;
		if (!rvn_rt_kind_is_list(para->kind)) {
			for (k = 0; k <= RVN_RT_MAX_INDENT; k++) counters[k] = 0;
		} else {
			for (k = para->indent + 1; k <= RVN_RT_MAX_INDENT; k++) counters[k] = 0;
			if (para->kind == RVN_RT_BULLET) counters[para->indent] = 0;
			else rt->numbers[i] = ++counters[para->indent];
		}
	}
	if (rt->doc.count && rt->doc.paras[rt->doc.count - 1].kind == RVN_RT_CODE_BLOCK) y += rvn_rt_code_pad(m->size);
	rt->content_h = y;
}

static void rvn_rt_update_bar(rvn_rich_text_t* rt, const rvn_rt_metrics_t* m) {
	rt->bar.content = rt->content_h + 2.0f * RVN_RT_PAD;
	rt->bar.viewport = m->view.h;
	rvn_scrollbar_clamp(&rt->bar);
}

/* The line of a paragraph an offset is on (at a cut, the line after it). */
static int rvn_rt_line_of(const rvn_rt_para_t* para, int offset) {
	uint32_t l;
	for (l = 0; l + 1 < para->line_count; l++) {
		if (offset < para->lines[l].end) return (int)l;
	}
	return para->line_count ? (int)para->line_count - 1 : 0;
}

/* Where an offset is across, on a line of a paragraph. */
static float rvn_rt_x_at(const rvn_rt_para_t* para, int line_index, int offset) {
	const rvn_rt_line_t* line;
	int f;
	if (!para->line_count) return 0.0f;
	line = &para->lines[line_index];
	if (!line->frag_count) return line->x;
	for (f = line->first_frag; f < line->first_frag + line->frag_count; f++) {
		const rvn_rt_frag_t* frag = &para->frags[f];
		if (offset <= frag->start) return frag->x;
		if (offset <= frag->start + frag->length) {
			return frag->x + rvn_text_width(rvn_font_resolve(frag->font), frag->size, para->text + frag->start, offset - frag->start);
		}
	}
	f = line->first_frag + line->frag_count - 1;
	return para->frags[f].x + para->frags[f].w;
}

/* The last place of a line the caret can be at: before the space or the break it was cut on. */
static int rvn_rt_line_last(const rvn_rt_para_t* para, int line_index) {
	const rvn_rt_line_t* line = &para->lines[line_index];
	int end = line->end;
	if ((uint32_t)line_index + 1 < para->line_count && end > line->start && (para->text[end - 1] == ' ' || para->text[end - 1] == '\n')) end--;
	return end;
}

static int rvn_rt_offset_in_line(const rvn_rt_para_t* para, int line_index, float x) {
	const rvn_rt_line_t* line;
	int f, last;
	if (!para->line_count) return 0;
	line = &para->lines[line_index];
	last = rvn_rt_line_last(para, line_index);
	if (!line->frag_count) return line->start;
	for (f = line->first_frag; f < line->first_frag + line->frag_count; f++) {
		const rvn_rt_frag_t* frag = &para->frags[f];
		if (x < frag->x + frag->w || f == line->first_frag + line->frag_count - 1) {
			int offset;
			if (x <= frag->x) return rvn_mini(frag->start, last);
			offset = frag->start + rvn_text_offset_at(rvn_font_resolve(frag->font), frag->size, para->text + frag->start, frag->length, x - frag->x);
			return rvn_mini(offset, last);
		}
	}
	return last;
}

/* The paragraph at a height of the document. */
static int rvn_rt_para_at_y(const rvn_rich_text_t* rt, float y) {
	int lo = 0, hi = (int)rt->doc.count - 1;
	while (lo < hi) {
		int mid = (lo + hi + 1) / 2;
		if (rt->tops[mid] <= y) lo = mid;
		else hi = mid - 1;
	}
	/* in the space between two paragraphs: the nearer one */
	if (lo + 1 < (int)rt->doc.count) {
		float bottom = rt->tops[lo] + rt->doc.paras[lo].height;
		if (y > bottom && y - bottom > rt->tops[lo + 1] - y) lo++;
	}
	return lo;
}

/* The place in the document at a point of the document. */
static rvn_rt_pos_t rvn_rt_pos_at(const rvn_rich_text_t* rt, float x, float y) {
	int index = rvn_rt_para_at_y(rt, y);
	const rvn_rt_para_t* para = &rt->doc.paras[index];
	float local = y - rt->tops[index];
	uint32_t l = 0;
	while (l + 1 < para->line_count && local >= para->lines[l].y + para->lines[l].h) l++;
	return rvn_rt_pos(index, rvn_rt_offset_in_line(para, (int)l, x));
}

/* The box of the caret at a place, in the document. */
static rvn_rect_t rvn_rt_caret_box(const rvn_rich_text_t* rt, rvn_rt_pos_t pos) {
	const rvn_rt_para_t* para = &rt->doc.paras[pos.para];
	int l = rvn_rt_line_of(para, pos.offset);
	if (!para->line_count) return rvn_rect(0.0f, rt->tops[pos.para], 2.0f, 16.0f);
	return rvn_rect(rvn_rt_x_at(para, l, pos.offset), rt->tops[pos.para] + para->lines[l].y, 2.0f, para->lines[l].h);
}

static void rvn_rt_reveal_now(rvn_rich_text_t* rt, const rvn_rt_metrics_t* m) {
	rvn_rect_t box = rvn_rt_caret_box(rt, rt->caret);
	float top = box.y, bottom = box.y + box.h + 2.0f * RVN_RT_PAD;
	rt->reveal = R_FALSE;
	if (m->view.h <= 0.0f) return;
	if (top < rt->bar.offset) rvn_scrollbar_set(&rt->bar, top);
	else if (bottom > rt->bar.offset + m->view.h) rvn_scrollbar_set(&rt->bar, bottom - m->view.h);
}

/* Lays out and scrolls to the caret when asked: what every event and every call of the API ends with. */
static void rvn_rt_refresh(rvn_rich_text_t* rt, rvn_rt_metrics_t* m) {
	rvn_rt_metrics(rt, m);
	rvn_rt_ensure(rt, m);
	rvn_rt_update_bar(rt, m);
	if (rt->reveal && rt->base.rect.w > 0.0f) rvn_rt_reveal_now(rt, m);
}

/* ---------------------------------------------------------------------------
 *  Telling the application
 * ------------------------------------------------------------------------- */

static void rvn_rt_dirty(rvn_rich_text_t* rt) {
	rt->tops_dirty = R_TRUE;
	rt->changed_pending = R_TRUE;
	rt->version++;
	rvn_widget_invalidate_paint(&rt->base);
}

/* After an event or a call: the callbacks of what changed. R_FALSE when the widget is gone. */
static int rvn_rt_tell(rvn_widget_t* widget) {
	rvn_rich_text_t* rt = (rvn_rich_text_t*)widget;
	Rid id = widget->id;
	if (rt->changed_pending) {
		rt->changed_pending = R_FALSE;
		if (rt->callback) rt->callback(id, rt->user_data);
		if (!rvn_widget_get(id)) return R_FALSE;
		rvn_widget_fire_action(widget);
		if (!rvn_widget_get(id)) return R_FALSE;
	}
	if (rvn_rt_pos_cmp(rt->told.caret, rt->caret) != 0 || rvn_rt_pos_cmp(rt->told.anchor, rt->anchor) != 0) {
		rt->told.caret = rt->caret;
		rt->told.anchor = rt->anchor;
		if (rt->selection_callback) rt->selection_callback(id, rt->selection_user_data);
		if (!rvn_widget_get(id)) return R_FALSE;
	}
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  History
 * ------------------------------------------------------------------------- */

static void rvn_rt_step_free(rvn_rt_step_t* step) {
	uint32_t i;
	for (i = 0; i < step->saved_count; i++) rvn_rt_para_free(&step->saved[i]);
	rvn_free(step->saved);
	memset(step, 0, sizeof(*step));
}

static void rvn_rt_history_clear(rvn_rt_step_t* steps, uint32_t* count) {
	uint32_t i;
	for (i = 0; i < *count; i++) rvn_rt_step_free(&steps[i]);
	*count = 0;
}

static rvn_rt_sel_t rvn_rt_sel(const rvn_rich_text_t* rt) {
	rvn_rt_sel_t sel;
	sel.anchor = rt->anchor;
	sel.caret = rt->caret;
	return sel;
}

/* Stops the characters typed from joining the last step. */
static void rvn_rt_break_typing(rvn_rich_text_t* rt) {
	if (rt->undo_count) rt->undo[rt->undo_count - 1].typing = 0;
}

/*
 * Starts a change of the paragraphs first .. first + count (the change may
 * add or remove paragraphs among them, not touch the others). `typing`: a
 * character typed, which joins the step of the characters typed before it.
 */
static void rvn_rt_begin(rvn_rich_text_t* rt, int first, int count, int typing) {
	rvn_rt_step_t* last = rt->undo_count ? &rt->undo[rt->undo_count - 1] : NULL;
	rvn_rt_step_t* step;
	uint32_t i;
	rt->count_before = rt->doc.count;
	if (typing && last && last->typing > 0 && last->typing < RVN_RT_TYPING_GROUP && last->first == (uint32_t)first &&
		last->live_count == 1 && count == 1 && !rvn_rt_has_selection(rt) && rvn_rt_pos_cmp(last->after.caret, rt->caret) == 0) {
		last->typing++;
		rt->editing = (int)rt->undo_count - 1;
		return;
	}
	rvn_rt_history_clear(rt->redo, &rt->redo_count);
	if (rt->undo_count >= RVN_RT_MAX_UNDO) {
		rvn_rt_step_free(&rt->undo[0]);
		memmove(&rt->undo[0], &rt->undo[1], sizeof(rvn_rt_step_t) * (rt->undo_count - 1));
		rt->undo_count--;
	}
	RVN_ARRAY_RESERVE(rt->undo, rt->undo_cap, rt->undo_count + 1);
	step = &rt->undo[rt->undo_count++];
	memset(step, 0, sizeof(*step));
	step->first = (uint32_t)first;
	step->saved_count = (uint32_t)count;
	step->saved = (rvn_rt_para_t*)rvn_calloc(sizeof(rvn_rt_para_t) * (size_t)rvn_maxi(count, 1));
	for (i = 0; i < (uint32_t)count; i++) rvn_rt_para_copy(&step->saved[i], &rt->doc.paras[(uint32_t)first + i]);
	step->live_count = (uint32_t)count;
	step->before = rvn_rt_sel(rt);
	step->typing = typing ? 1 : 0;
	rt->editing = (int)rt->undo_count - 1;
}

static void rvn_rt_end(rvn_rich_text_t* rt) {
	rvn_rt_step_t* step;
	if (rt->editing < 0) return;
	step = &rt->undo[rt->editing];
	step->live_count = (uint32_t)((int)step->live_count + (int)rt->doc.count - (int)rt->count_before);
	rt->caret = rvn_rt_clamp(rt, rt->caret);
	rt->anchor = rvn_rt_clamp(rt, rt->anchor);
	step->after = rvn_rt_sel(rt);
	rt->editing = -1;
	rt->reveal = R_TRUE;
	rt->goal_x = -1.0f;
	rvn_rt_dirty(rt);
}

/* Puts the saved paragraphs of a step back, and keeps the ones they replace in it. */
static void rvn_rt_swap(rvn_rich_text_t* rt, rvn_rt_step_t* step) {
	rvn_rt_doc_t* doc = &rt->doc;
	uint32_t live = step->live_count, saved = step->saved_count, first = step->first, i;
	rvn_rt_para_t* out = (rvn_rt_para_t*)rvn_calloc(sizeof(rvn_rt_para_t) * (size_t)rvn_maxi((int)live, 1));
	memcpy(out, &doc->paras[first], sizeof(rvn_rt_para_t) * live);
	memmove(&doc->paras[first], &doc->paras[first + live], sizeof(rvn_rt_para_t) * (doc->count - first - live));
	doc->count -= live;
	rvn_rt_doc_insert(doc, first, saved);
	memcpy(&doc->paras[first], step->saved, sizeof(rvn_rt_para_t) * saved);
	for (i = 0; i < saved; i++) rvn_rt_para_touch(&doc->paras[first + i]);
	rvn_free(step->saved);
	step->saved = out;
	step->saved_count = live;
	step->live_count = saved;
}

static int rvn_rt_undo(rvn_rich_text_t* rt) {
	rvn_rt_step_t step;
	if (rt->undo_count == 0 || rt->read_only) return R_FALSE;
	step = rt->undo[--rt->undo_count];
	rvn_rt_swap(rt, &step);
	step.typing = 0;
	rt->anchor = rvn_rt_clamp(rt, step.before.anchor);
	rt->caret = rvn_rt_clamp(rt, step.before.caret);
	RVN_ARRAY_RESERVE(rt->redo, rt->redo_cap, rt->redo_count + 1);
	rt->redo[rt->redo_count++] = step;
	rt->typing_set = R_FALSE;
	rt->reveal = R_TRUE;
	rt->goal_x = -1.0f;
	rvn_rt_dirty(rt);
	return R_TRUE;
}

static int rvn_rt_redo(rvn_rich_text_t* rt) {
	rvn_rt_step_t step;
	if (rt->redo_count == 0 || rt->read_only) return R_FALSE;
	step = rt->redo[--rt->redo_count];
	rvn_rt_swap(rt, &step);
	rt->anchor = rvn_rt_clamp(rt, step.after.anchor);
	rt->caret = rvn_rt_clamp(rt, step.after.caret);
	RVN_ARRAY_RESERVE(rt->undo, rt->undo_cap, rt->undo_count + 1);
	rt->undo[rt->undo_count++] = step;
	rt->typing_set = R_FALSE;
	rt->reveal = R_TRUE;
	rt->goal_x = -1.0f;
	rvn_rt_dirty(rt);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Editing
 * ------------------------------------------------------------------------- */

/* Moves the caret (and the anchor unless `extend`). */
static void rvn_rt_go(rvn_rich_text_t* rt, rvn_rt_pos_t pos, int extend) {
	rt->caret = rvn_rt_clamp(rt, pos);
	if (!extend) rt->anchor = rt->caret;
	rt->typing_set = R_FALSE;
	rt->reveal = R_TRUE;
	rvn_rt_break_typing(rt);
	rt->blink_start = rvn_clock_seconds();
	rvn_widget_invalidate_paint(&rt->base);
}

/* The looks of what is typed at the caret: chosen there, else those of the character before (of the first one selected). */
static void rvn_rt_typing_look(const rvn_rich_text_t* rt, uint32_t* style, const char** link) {
	rvn_rt_pos_t pos = rvn_rt_sel_min(rt);
	const rvn_rt_para_t* para = &rt->doc.paras[pos.para];
	int run, start;
	*style = 0;
	*link = NULL;
	if (para->kind == RVN_RT_CODE_BLOCK) return;
	if (rt->typing_set && !rvn_rt_has_selection(rt)) {
		*style = rt->typing_style;
		return;
	}
	if (para->length == 0) return;
	if (rvn_rt_has_selection(rt)) {
		run = rvn_rt_para_run_at(para, pos.offset, &start);
		if (run < 0) return;
		*style = para->runs[run].style;
		*link = para->runs[run].link;
		return;
	}
	run = rvn_rt_para_run_at(para, pos.offset > 0 ? pos.offset - 1 : 0, &start);
	if (run < 0) return;
	*style = para->runs[run].style;
	/* (a link goes on only inside it: what is typed after its end is not part of it) */
	if (para->runs[run].link && pos.offset > 0 && pos.offset < start + para->runs[run].length) *link = para->runs[run].link;
}

/* Removes the selection (inside a step). */
static void rvn_rt_delete_selection(rvn_rich_text_t* rt) {
	rvn_rt_pos_t a = rvn_rt_sel_min(rt), b = rvn_rt_sel_max(rt);
	if (rvn_rt_pos_cmp(a, b) == 0) return;
	rvn_rt_doc_delete(&rt->doc, a, b);
	rt->caret = rt->anchor = a;
}

static void rvn_rt_begin_selection(rvn_rich_text_t* rt, int typing) {
	rvn_rt_pos_t a = rvn_rt_sel_min(rt), b = rvn_rt_sel_max(rt);
	rvn_rt_begin(rt, a.para, b.para - a.para + 1, typing);
}

/* Puts a document (`piece`, emptied) in place of the selection, as one step. */
static void rvn_rt_paste_doc(rvn_rich_text_t* rt, rvn_rt_doc_t* piece) {
	rvn_rt_pos_t end;
	uint32_t i;
	if (rt->read_only || piece->count == 0) {
		rvn_rt_doc_free(piece);
		return;
	}
	/* (into a line of code: plain text) */
	if (rt->doc.paras[rvn_rt_sel_min(rt).para].kind == RVN_RT_CODE_BLOCK && piece->count == 1) {
		rvn_rt_para_style(&piece->paras[0], 0, piece->paras[0].length, 0, RVN_RT_STYLES, R_TRUE, NULL);
	}
	for (i = 0; i < piece->count; i++) {
		if (piece->paras[i].kind == RVN_RT_CODE_BLOCK) rvn_rt_para_style(&piece->paras[i], 0, piece->paras[i].length, 0, RVN_RT_STYLES, R_TRUE, NULL);
	}
	rvn_rt_begin_selection(rt, R_FALSE);
	rvn_rt_delete_selection(rt);
	end = rvn_rt_doc_paste(&rt->doc, rt->caret, piece);
	rt->caret = rt->anchor = end;
	rvn_rt_end(rt);
	rvn_rt_break_typing(rt);
}

/* Types text in place of the selection, with the looks of the caret. `typing`: one character typed. */
static void rvn_rt_type(rvn_rich_text_t* rt, const char* text, int length, int typing) {
	uint32_t style;
	const char* link;
	char* kept_link;
	if (rt->read_only || length <= 0) return;
	rvn_rt_typing_look(rt, &style, &link);
	kept_link = link ? rvn_strdup(link) : NULL;
	if (memchr(text, '\n', (size_t)length) && rt->doc.paras[rvn_rt_sel_min(rt).para].kind != RVN_RT_CODE_BLOCK) {
		/* lines: paragraphs (a line break of Shift+Enter is typed by rvn_rt_line_break) */
		rvn_rt_doc_t piece;
		char* copy = rvn_strndup(text, (size_t)length);
		uint32_t i;
		memset(&piece, 0, sizeof(piece));
		rvn_rt_from_text(&piece, copy);
		rvn_free(copy);
		for (i = 0; i < piece.count; i++) rvn_rt_para_style(&piece.paras[i], 0, piece.paras[i].length, style, 0, R_TRUE, kept_link);
		rvn_rt_paste_doc(rt, &piece);
		rvn_free(kept_link);
		return;
	}
	if (memchr(text, '\n', (size_t)length)) {
		/* into code: each line a line of code */
		rvn_rt_doc_t piece;
		char* copy = rvn_strndup(text, (size_t)length);
		memset(&piece, 0, sizeof(piece));
		rvn_rt_from_text(&piece, copy);
		rvn_free(copy);
		{
			uint32_t i;
			for (i = 0; i < piece.count; i++) piece.paras[i].kind = RVN_RT_CODE_BLOCK;
		}
		rvn_rt_paste_doc(rt, &piece);
		rvn_free(kept_link);
		return;
	}
	rvn_rt_begin_selection(rt, typing);
	rvn_rt_delete_selection(rt);
	rvn_rt_para_insert(&rt->doc.paras[rt->caret.para], rt->caret.offset, text, length, style, kept_link);
	rt->caret.offset += length;
	rt->anchor = rt->caret;
	rvn_rt_end(rt);
	rvn_free(kept_link);
}

/* Shift+Enter: a line break in the paragraph (a new line in a block of code). */
static void rvn_rt_line_break(rvn_rich_text_t* rt) {
	rvn_rt_para_t* para = &rt->doc.paras[rvn_rt_sel_min(rt).para];
	uint32_t style;
	const char* link;
	if (rt->read_only) return;
	if (para->kind == RVN_RT_CODE_BLOCK || rvn_rt_kind_is_heading(para->kind)) {
		rvn_rt_begin_selection(rt, R_FALSE);
		rvn_rt_delete_selection(rt);
		rvn_rt_doc_split(&rt->doc, rt->caret);
		rt->caret = rt->anchor = rvn_rt_pos(rt->caret.para + 1, 0);
		rvn_rt_end(rt);
		return;
	}
	rvn_rt_typing_look(rt, &style, &link);
	rvn_rt_begin_selection(rt, R_FALSE);
	rvn_rt_delete_selection(rt);
	rvn_rt_para_insert(&rt->doc.paras[rt->caret.para], rt->caret.offset, "\n", 1, style & ~RVN_RT_CODE, NULL);
	rt->caret.offset++;
	rt->anchor = rt->caret;
	rvn_rt_end(rt);
	rvn_rt_break_typing(rt);
}

static void rvn_rt_set_kind_of(rvn_rt_para_t* para, int kind) {
	para->kind = (uint8_t)kind;
	if (!rvn_rt_kind_is_list(kind)) para->indent = 0;
	if (kind == RVN_RT_CODE_BLOCK) rvn_rt_para_style(para, 0, para->length, 0, RVN_RT_STYLES, R_TRUE, NULL);
	rvn_rt_para_touch(para);
}

/* Enter: a new paragraph; an empty item, quote or last line of code ends what it is in. */
static void rvn_rt_enter(rvn_rich_text_t* rt) {
	rvn_rt_para_t* para;
	int index;
	if (rt->read_only) return;
	index = rt->caret.para;
	para = &rt->doc.paras[index];
	if (!rvn_rt_has_selection(rt) && para->length == 0) {
		int next_is_code = index + 1 < (int)rt->doc.count && rt->doc.paras[index + 1].kind == RVN_RT_CODE_BLOCK;
		int previous_is_code = index > 0 && rt->doc.paras[index - 1].kind == RVN_RT_CODE_BLOCK;
		if (rvn_rt_kind_is_list(para->kind) || para->kind == RVN_RT_QUOTE ||
			(para->kind == RVN_RT_CODE_BLOCK && !next_is_code && previous_is_code)) {
			rvn_rt_begin(rt, index, 1, R_FALSE);
			para = &rt->doc.paras[index];
			if (rvn_rt_kind_is_list(para->kind) && para->indent > 0) para->indent--;
			else rvn_rt_set_kind_of(para, RVN_RT_PARAGRAPH);
			rvn_rt_para_touch(para);
			rvn_rt_end(rt);
			rvn_rt_break_typing(rt);
			return;
		}
	}
	rvn_rt_begin_selection(rt, R_FALSE);
	rvn_rt_delete_selection(rt);
	index = rt->caret.para;
	{
		int at_start = rt->caret.offset == 0 && rt->doc.paras[index].length > 0;
		rvn_rt_para_t* left;
		rvn_rt_para_t* right;
		rvn_rt_doc_split(&rt->doc, rt->caret);
		left = &rt->doc.paras[index];
		right = &rt->doc.paras[index + 1];
		/* a title is followed by text; Enter at the start of a title leaves an empty paragraph above it */
		if (rvn_rt_kind_is_heading(left->kind)) {
			if (at_start) rvn_rt_set_kind_of(left, RVN_RT_PARAGRAPH);
			else if (right->length == 0) rvn_rt_set_kind_of(right, RVN_RT_PARAGRAPH);
		}
	}
	rt->caret = rt->anchor = rvn_rt_pos(index + 1, 0);
	rvn_rt_end(rt);
	rvn_rt_break_typing(rt);
}

/* Backspace and Delete; `word`: a whole word. */
static void rvn_rt_erase(rvn_rich_text_t* rt, int backspace, int word) {
	rvn_rt_pos_t target;
	rvn_rt_para_t* para;
	if (rt->read_only) return;
	if (rvn_rt_has_selection(rt)) {
		rvn_rt_begin_selection(rt, R_FALSE);
		rvn_rt_delete_selection(rt);
		rvn_rt_end(rt);
		rvn_rt_break_typing(rt);
		return;
	}
	para = &rt->doc.paras[rt->caret.para];
	if (backspace && rt->caret.offset == 0) {
		int index = rt->caret.para;
		int previous_is_code = index > 0 && rt->doc.paras[index - 1].kind == RVN_RT_CODE_BLOCK;
		/* the start of an item, a title, a quote, the first line of code: back to a paragraph (an item first goes out a level) */
		if (para->kind != RVN_RT_PARAGRAPH && !(para->kind == RVN_RT_CODE_BLOCK && previous_is_code)) {
			rvn_rt_begin(rt, index, 1, R_FALSE);
			para = &rt->doc.paras[index];
			if (rvn_rt_kind_is_list(para->kind) && para->indent > 0) para->indent--;
			else rvn_rt_set_kind_of(para, RVN_RT_PARAGRAPH);
			rvn_rt_para_touch(para);
			rvn_rt_end(rt);
			rvn_rt_break_typing(rt);
			return;
		}
		if (index == 0) return;
		target = rvn_rt_pos(index - 1, rt->doc.paras[index - 1].length);
		rvn_rt_begin(rt, index - 1, 2, R_FALSE);
		rvn_rt_doc_delete(&rt->doc, target, rt->caret);
		rt->caret = rt->anchor = target;
		rvn_rt_end(rt);
		rvn_rt_break_typing(rt);
		return;
	}
	if (!backspace && rt->caret.offset == para->length) {
		int index = rt->caret.para;
		if (index + 1 >= (int)rt->doc.count) return;
		rvn_rt_begin(rt, index, 2, R_FALSE);
		rvn_rt_doc_delete(&rt->doc, rt->caret, rvn_rt_pos(index + 1, 0));
		rvn_rt_end(rt);
		rvn_rt_break_typing(rt);
		return;
	}
	if (backspace) target = word ? rvn_rt_word_left(rt, rt->caret) : rvn_rt_char_left(rt, rt->caret);
	else target = word ? rvn_rt_word_right(rt, rt->caret) : rvn_rt_char_right(rt, rt->caret);
	/* (within the paragraph: its edges were handled above) */
	if (target.para != rt->caret.para) target = rvn_rt_pos(rt->caret.para, backspace ? 0 : para->length);
	rvn_rt_begin(rt, rt->caret.para, 1, R_FALSE);
	{
		int from = rvn_mini(target.offset, rt->caret.offset), to = rvn_maxi(target.offset, rt->caret.offset);
		rvn_rt_para_delete(&rt->doc.paras[rt->caret.para], from, to);
		rt->caret = rt->anchor = rvn_rt_pos(rt->caret.para, from);
	}
	rvn_rt_end(rt);
	rvn_rt_break_typing(rt);
}

/* ---- looks ---- */

/* The looks the whole selection has; *any_link: some of it is a link. */
static uint32_t rvn_rt_selection_style(const rvn_rich_text_t* rt, int* any_link) {
	rvn_rt_pos_t a = rvn_rt_sel_min(rt), b = rvn_rt_sel_max(rt);
	uint32_t style = RVN_RT_STYLES;
	int p, seen = R_FALSE;
	*any_link = R_FALSE;
	if (!rvn_rt_has_selection(rt)) {
		const char* link;
		const rvn_rt_para_t* para = &rt->doc.paras[a.para];
		int start, run;
		rvn_rt_typing_look(rt, &style, &link);
		/* (the link the caret is in, even at its edge) */
		run = para->length ? rvn_rt_para_run_at(para, a.offset > 0 ? a.offset - 1 : 0, &start) : -1;
		if (run >= 0 && para->runs[run].link) *any_link = R_TRUE;
		return style;
	}
	for (p = a.para; p <= b.para; p++) {
		const rvn_rt_para_t* para = &rt->doc.paras[p];
		int from = p == a.para ? a.offset : 0, to = p == b.para ? b.offset : para->length, at = 0;
		uint32_t r;
		for (r = 0; r < para->run_count; r++) {
			const rvn_rt_run_t* run = &para->runs[r];
			if (at < to && at + run->length > from) {
				style &= run->style;
				if (run->link) *any_link = R_TRUE;
				seen = R_TRUE;
			}
			at += run->length;
		}
	}
	return seen ? style : 0u;
}

static void rvn_rt_apply_style(rvn_rich_text_t* rt, uint32_t set, uint32_t clear, int link, const char* url) {
	rvn_rt_pos_t a = rvn_rt_sel_min(rt), b = rvn_rt_sel_max(rt);
	int p;
	rvn_rt_begin_selection(rt, R_FALSE);
	for (p = a.para; p <= b.para; p++) {
		rvn_rt_para_t* para = &rt->doc.paras[p];
		if (para->kind == RVN_RT_CODE_BLOCK) continue;
		rvn_rt_para_style(para, p == a.para ? a.offset : 0, p == b.para ? b.offset : para->length, set, clear, link, url);
	}
	rvn_rt_end(rt);
	rvn_rt_break_typing(rt);
}

static void rvn_rt_set_style(rvn_rich_text_t* rt, uint32_t style, int enabled) {
	style &= RVN_RT_STYLES;
	if (!style || rt->read_only) return;
	if (!rvn_rt_has_selection(rt)) {
		const char* link;
		uint32_t current;
		rvn_rt_typing_look(rt, &current, &link);
		rt->typing_style = enabled ? (current | style) : (current & ~style);
		rt->typing_set = R_TRUE;
		rvn_widget_invalidate_paint(&rt->base);
		return;
	}
	if (enabled) rvn_rt_apply_style(rt, style, 0, R_FALSE, NULL);
	else rvn_rt_apply_style(rt, 0, style, R_FALSE, NULL);
}

static void rvn_rt_toggle_style(rvn_rich_text_t* rt, uint32_t style) {
	int any_link;
	uint32_t current = rvn_rt_selection_style(rt, &any_link);
	rvn_rt_set_style(rt, style, (current & style) != style);
}

/* The kind of the paragraphs of the selection; `toggle`: back to paragraphs when they all are of that kind. */
static void rvn_rt_set_block(rvn_rich_text_t* rt, int kind, int toggle) {
	rvn_rt_pos_t a = rvn_rt_sel_min(rt), b = rvn_rt_sel_max(rt);
	int p, all = R_TRUE;
	if (rt->read_only || kind < 0 || kind >= RVN_RT_KIND_COUNT) return;
	for (p = a.para; p <= b.para; p++) if (rt->doc.paras[p].kind != kind) all = R_FALSE;
	if (toggle && all) kind = RVN_RT_PARAGRAPH;
	else if (all) return;
	rvn_rt_begin_selection(rt, R_FALSE);
	for (p = a.para; p <= b.para; p++) rvn_rt_set_kind_of(&rt->doc.paras[p], kind);
	rvn_rt_end(rt);
	rvn_rt_break_typing(rt);
}

/* Tab and Shift+Tab on items of lists; R_FALSE when the selection holds none. */
static int rvn_rt_indent(rvn_rich_text_t* rt, int levels) {
	rvn_rt_pos_t a = rvn_rt_sel_min(rt), b = rvn_rt_sel_max(rt);
	int p, any = R_FALSE;
	if (rt->read_only) return R_FALSE;
	for (p = a.para; p <= b.para; p++) if (rvn_rt_kind_is_list(rt->doc.paras[p].kind)) any = R_TRUE;
	if (!any) return R_FALSE;
	rvn_rt_begin_selection(rt, R_FALSE);
	for (p = a.para; p <= b.para; p++) {
		rvn_rt_para_t* para = &rt->doc.paras[p];
		int indent, deepest;
		if (!rvn_rt_kind_is_list(para->kind)) continue;
		/* (an item goes at most one level deeper than the item above it) */
		deepest = p > 0 && rvn_rt_kind_is_list(rt->doc.paras[p - 1].kind) ? rt->doc.paras[p - 1].indent + 1 : 0;
		indent = rvn_clampi((int)para->indent + levels, 0, rvn_mini(RVN_RT_MAX_INDENT, rvn_maxi(deepest, levels < 0 ? (int)para->indent : 0)));
		para->indent = (uint8_t)indent;
		rvn_rt_para_touch(para);
	}
	rvn_rt_end(rt);
	rvn_rt_break_typing(rt);
	return R_TRUE;
}

/* The link the caret is in: from .. to of its paragraph. R_FALSE when it is in none. */
static int rvn_rt_link_extent(const rvn_rich_text_t* rt, rvn_rt_pos_t pos, int* from, int* to, const char** url) {
	const rvn_rt_para_t* para = &rt->doc.paras[pos.para];
	int start, run, r, at;
	if (!para->length) return R_FALSE;
	run = rvn_rt_para_run_at(para, pos.offset, &start);
	if (run >= 0 && !para->runs[run].link && pos.offset > 0) run = rvn_rt_para_run_at(para, pos.offset - 1, &start);
	if (run < 0 || !para->runs[run].link) return R_FALSE;
	*url = para->runs[run].link;
	/* (the runs of a link differ by their other looks) */
	*from = start;
	for (r = run - 1, at = start; r >= 0 && para->runs[r].link && strcmp(para->runs[r].link, *url) == 0; r--) at -= para->runs[r].length;
	*from = at;
	at = start + para->runs[run].length;
	for (r = run + 1; r < (int)para->run_count && para->runs[r].link && strcmp(para->runs[r].link, *url) == 0; r++) at += para->runs[r].length;
	*to = at;
	return R_TRUE;
}

static void rvn_rt_set_link(rvn_rich_text_t* rt, const char* url) {
	int from, to;
	const char* current;
	if (rt->read_only) return;
	if (url && !url[0]) url = NULL;
	if (!rvn_rt_has_selection(rt)) {
		if (rvn_rt_link_extent(rt, rt->caret, &from, &to, &current)) {
			rvn_rt_pos_t keep = rt->caret;
			rt->anchor = rvn_rt_pos(rt->caret.para, from);
			rt->caret = rvn_rt_pos(rt->caret.para, to);
			rvn_rt_apply_style(rt, 0, 0, R_TRUE, url);
			rt->caret = rt->anchor = keep;
			if (rt->undo_count) rt->undo[rt->undo_count - 1].after = rvn_rt_sel(rt);
			return;
		}
		if (!url || rt->doc.paras[rt->caret.para].kind == RVN_RT_CODE_BLOCK) return;
		/* nothing to link: the address, typed as a link */
		{
			uint32_t style;
			const char* link;
			rvn_rt_typing_look(rt, &style, &link);
			rvn_rt_begin(rt, rt->caret.para, 1, R_FALSE);
			rvn_rt_para_insert(&rt->doc.paras[rt->caret.para], rt->caret.offset, url, -1, style, url);
			rt->caret.offset += (int)strlen(url);
			rt->anchor = rt->caret;
			rvn_rt_end(rt);
			rvn_rt_break_typing(rt);
		}
		return;
	}
	rvn_rt_apply_style(rt, 0, 0, R_TRUE, url);
}

/* ---- Markdown typed ---- */

/* What a paragraph starts with when a space is typed after it: "# ", "- ", "1. ", "> ". */
static int rvn_rt_block_mark(const char* text, int length, int* kind) {
	int i;
	if (length >= 1 && length <= 3) {
		for (i = 0; i < length && text[i] == '#'; i++) {}
		if (i == length) {
			*kind = RVN_RT_HEADING_1 + length - 1;
			return R_TRUE;
		}
	}
	if (length == 1 && (text[0] == '-' || text[0] == '*' || text[0] == '+')) {
		*kind = RVN_RT_BULLET;
		return R_TRUE;
	}
	if (length == 1 && text[0] == '>') {
		*kind = RVN_RT_QUOTE;
		return R_TRUE;
	}
	if (length >= 2 && length <= 4 && (text[length - 1] == '.' || text[length - 1] == ')')) {
		for (i = 0; i < length - 1 && text[i] >= '0' && text[i] <= '9'; i++) {}
		if (i == length - 1) {
			*kind = RVN_RT_NUMBERED;
			return R_TRUE;
		}
	}
	return R_FALSE;
}

/* The text between two marks may take a look: not empty, not against a space, no code in it. */
static int rvn_rt_markable(const rvn_rt_para_t* para, int from, int to) {
	int at = 0;
	uint32_t r;
	if (to <= from || para->text[from] == ' ' || para->text[to - 1] == ' ') return R_FALSE;
	for (r = 0; r < para->run_count; r++) {
		if (at < to && at + para->runs[r].length > from - 1 && (para->runs[r].style & RVN_RT_CODE)) return R_FALSE;
		at += para->runs[r].length;
	}
	return R_TRUE;
}

/* After a character typed: what Markdown it ends is formatted, as a step of its own. */
static void rvn_rt_markdown_typed(rvn_rich_text_t* rt, uint32_t codepoint) {
	rvn_rt_para_t* para = &rt->doc.paras[rt->caret.para];
	int o = rt->caret.offset, index = rt->caret.para, kind;
	const char* t = para->text;
	char c = (char)codepoint;
	if (!rt->shortcuts || rvn_rt_has_selection(rt) || para->kind == RVN_RT_CODE_BLOCK || codepoint > 127) return;

	/* "# ", "- ", "1. ", "> " at the start of a paragraph */
	if (c == ' ' && para->kind == RVN_RT_PARAGRAPH && o >= 2 && rvn_rt_block_mark(t, o - 1, &kind)) {
		rvn_rt_begin(rt, index, 1, R_FALSE);
		para = &rt->doc.paras[index];
		rvn_rt_para_delete(para, 0, o);
		rvn_rt_set_kind_of(para, kind);
		rt->caret = rt->anchor = rvn_rt_pos(index, 0);
		rvn_rt_end(rt);
		rvn_rt_break_typing(rt);
		return;
	}
	/* "```" alone: a block of code */
	if (c == '`' && para->kind == RVN_RT_PARAGRAPH && o == 3 && memcmp(t, "```", 3) == 0) {
		rvn_rt_begin(rt, index, 1, R_FALSE);
		para = &rt->doc.paras[index];
		rvn_rt_para_delete(para, 0, 3);
		rvn_rt_set_kind_of(para, RVN_RT_CODE_BLOCK);
		rt->caret = rt->anchor = rvn_rt_pos(index, 0);
		rvn_rt_end(rt);
		rvn_rt_break_typing(rt);
		return;
	}
	/* **bold**, __bold__, *italic*, _italic_, ~~struck~~, `code` */
	if (c == '*' || c == '_' || c == '~' || c == '`') {
		int mark = 1, open = -1, j;
		uint32_t style;
		if ((c == '*' || c == '_' || c == '~') && o >= 2 && t[o - 2] == c) mark = 2;
		if (c == '~' && mark != 2) return;
		if (c == '`') {
			for (j = o - 2; j >= 0; j--) if (t[j] == '`') break;
			if (j >= 0 && j + 1 < o - 1) open = j;
			style = RVN_RT_CODE;
		} else if (mark == 2) {
			for (j = o - 4; j >= 0; j--) {
				if (t[j] == c && t[j + 1] == c && (j == 0 || t[j - 1] != c)) {
					open = j;
					break;
				}
			}
			style = c == '~' ? RVN_RT_STRIKE : RVN_RT_BOLD;
		} else {
			/* (one mark: not the second of a pair, not the first of a closing pair) */
			if (o >= 2 && t[o - 2] == c) return;
			for (j = o - 2; j >= 0; j--) {
				if (t[j] != c) continue;
				if ((j > 0 && t[j - 1] == c) || t[j + 1] == c) break;
				/* (an underscore inside a word is a letter: snake_case) */
				if (c == '_' && j > 0 && rvn_rt_class(t[j - 1]) == RVN_RT_WORD) break;
				open = j;
				break;
			}
			style = RVN_RT_ITALIC;
		}
		if (open < 0) return;
		{
			int content_from = open + (c == '`' ? 1 : mark), content_to = o - mark;
			if (c == '`' ? content_to <= content_from : !rvn_rt_markable(para, content_from, content_to)) return;
			rvn_rt_begin(rt, index, 1, R_FALSE);
			para = &rt->doc.paras[index];
			rvn_rt_para_delete(para, content_to, o);
			rvn_rt_para_delete(para, open, content_from);
			content_to -= content_from - open;
			rvn_rt_para_style(para, open, content_to, style, 0, R_FALSE, NULL);
			rt->caret = rt->anchor = rvn_rt_pos(index, content_to);
			rvn_rt_end(rt);
			rvn_rt_break_typing(rt);
			/* what is typed next is plain again */
			{
				const char* link;
				uint32_t current;
				rvn_rt_typing_look(rt, &current, &link);
				rt->typing_style = current & ~style;
				rt->typing_set = R_TRUE;
			}
		}
	}
}

/* A character typed (a key, or what an input method commits). */
static void rvn_rt_type_char(rvn_rich_text_t* rt, uint32_t codepoint) {
	char utf8[4];
	int length = rvn_utf8_encode(codepoint, utf8);
	uint32_t typing_style = rt->typing_style;
	int typing_set = rt->typing_set;
	rvn_rt_type(rt, utf8, length, R_TRUE);
	/* (the looks chosen at the caret stay for the next characters) */
	rt->typing_style = typing_style;
	rt->typing_set = typing_set;
	rvn_rt_markdown_typed(rt, codepoint);
}

/* ---- the clipboard ---- */

static void rvn_rt_copy(rvn_rich_text_t* rt) {
	rvn_rt_doc_t piece;
	rvn_text_t markdown = { NULL, 0, 0 }, html = { NULL, 0, 0 }, text = { NULL, 0, 0 };
	RDataItem items[3];
	if (!rvn_rt_has_selection(rt)) return;
	rvn_rt_doc_slice(&rt->doc, rvn_rt_sel_min(rt), rvn_rt_sel_max(rt), &piece);
	rvn_rt_to_text(&piece, &text);
	rvn_rt_to_html(&piece, &html);
	rvn_rt_to_markdown(&piece, &markdown);
	items[0].mime = "text/plain";
	items[0].data = text.data;
	items[0].size = text.length;
	items[1].mime = "text/html";
	items[1].data = html.data;
	items[1].size = html.length;
	items[2].mime = "text/markdown";
	items[2].data = markdown.data;
	items[2].size = markdown.length;
	Rclipboard_set_data(items, 3);
	rvn_free(text.data);
	rvn_free(html.data);
	rvn_free(markdown.data);
	rvn_rt_doc_free(&piece);
}

static void rvn_rt_paste(rvn_rich_text_t* rt) {
	size_t size = 0;
	char* html;
	if (rt->read_only) return;
	html = Rclipboard_has_data("text/html") ? Rclipboard_get_data("text/html", &size) : NULL;
	if (html && html[0]) {
		rvn_rt_doc_t piece;
		memset(&piece, 0, sizeof(piece));
		rvn_rt_from_html(&piece, html);
		rvn_free(html);
		rvn_rt_paste_doc(rt, &piece);
		return;
	}
	rvn_free(html);
	{
		const char* text = Rclipboard_get_text();
		if (text && text[0]) rvn_rt_type(rt, text, (int)strlen(text), R_FALSE);
		rvn_rt_break_typing(rt);
	}
}

/* ---------------------------------------------------------------------------
 *  The bar of the link
 * ------------------------------------------------------------------------- */

static void rvn_rt_link_close(rvn_rich_text_t* rt) {
	if (!rt->link_open) return;
	rt->link_open = R_FALSE;
	rt->link_selecting = R_FALSE;
	rvn_textedit_set_focused(&rt->link_edit, &rt->base, R_FALSE);
	rvn_widget_invalidate_layout(&rt->base);
	rvn_widget_invalidate_paint(&rt->base);
}

static void rvn_rt_link_open(rvn_rich_text_t* rt) {
	int from, to;
	const char* url = NULL;
	if (rt->read_only) return;
	if (!rt->link_ready) {
		rvn_textedit_init(&rt->link_edit);
		rt->link_ready = R_TRUE;
	}
	if (!rvn_rt_has_selection(rt) && rvn_rt_link_extent(rt, rt->caret, &from, &to, &url)) {
		/* (the whole link the caret is in) */
	} else if (rvn_rt_has_selection(rt)) {
		rvn_rt_pos_t a = rvn_rt_sel_min(rt);
		const rvn_rt_para_t* para = &rt->doc.paras[a.para];
		int start, run = para->length ? rvn_rt_para_run_at(para, a.offset, &start) : -1;
		url = run >= 0 ? para->runs[run].link : NULL;
	}
	rt->link_sel = rvn_rt_sel(rt);
	rvn_textedit_set_text(&rt->link_edit, url ? url : "");
	rvn_textedit_select_all(&rt->link_edit);
	rt->link_open = R_TRUE;
	rvn_textedit_set_focused(&rt->link_edit, &rt->base, R_TRUE);
	rvn_widget_invalidate_layout(&rt->base);
	rvn_widget_invalidate_paint(&rt->base);
}

static void rvn_rt_link_apply(rvn_rich_text_t* rt) {
	char* url = rvn_strdup(rt->link_edit.text ? rt->link_edit.text : "");
	rvn_rt_link_close(rt);
	rt->anchor = rvn_rt_clamp(rt, rt->link_sel.anchor);
	rt->caret = rvn_rt_clamp(rt, rt->link_sel.caret);
	rvn_rt_set_link(rt, url);
	rvn_free(url);
}

/* Follows a link: the callback, an action, the browser. */
static void rvn_rt_follow(rvn_widget_t* widget, const char* address) {
	rvn_rich_text_t* rt = (rvn_rich_text_t*)widget;
	char* url = rvn_strdup(address);
	if (rt->link_callback) {
		rt->link_callback(widget->id, url, rt->link_user_data);
	} else if (rvn_strnicmp(url, "action:", 7) == 0) {
		if (url[7]) rvn_fire_action(widget->id, url + 7);
	} else if (rvn_strnicmp(url, "http://", 7) == 0 || rvn_strnicmp(url, "https://", 8) == 0 || rvn_strnicmp(url, "mailto:", 7) == 0) {
		Ropen_url(url);
	}
	rvn_free(url);
}

/* The link under a point of the document, or NULL. */
static const char* rvn_rt_link_at(const rvn_rich_text_t* rt, float x, float y) {
	int index;
	const rvn_rt_para_t* para;
	float local;
	uint32_t l;
	int f;
	if (!rt->doc.count || y < 0.0f || y > rt->content_h) return NULL;
	index = rvn_rt_para_at_y(rt, y);
	para = &rt->doc.paras[index];
	local = y - rt->tops[index];
	for (l = 0; l < para->line_count; l++) {
		const rvn_rt_line_t* line = &para->lines[l];
		if (local < line->y || local >= line->y + line->h) continue;
		for (f = line->first_frag; f < line->first_frag + line->frag_count; f++) {
			const rvn_rt_frag_t* frag = &para->frags[f];
			if (x >= frag->x && x < frag->x + frag->w && frag->run < (int)para->run_count) return para->runs[frag->run].link;
		}
	}
	return NULL;
}

/* ---------------------------------------------------------------------------
 *  Painting
 * ------------------------------------------------------------------------- */

static void rvn_rt_tool_rect(const rvn_rt_metrics_t* m, int index, rvn_rect_t* rect) {
	float x = m->toolbar.x + 6.0f;
	int i;
	for (i = 0; i < index; i++) x += g_rt_tools[i].kind == RVN_RT_TOOL_GAP ? 11.0f : m->button + 2.0f;
	*rect = rvn_rect(x, m->toolbar.y + 5.0f, g_rt_tools[index].kind == RVN_RT_TOOL_GAP ? 11.0f : m->button, m->button);
}

static int rvn_rt_tool_at(const rvn_rich_text_t* rt, const rvn_rt_metrics_t* m, float x, float y) {
	int i;
	if (!rt->toolbar || !rvn_rect_contains(m->toolbar, x, y)) return -1;
	for (i = 0; i < RVN_RT_TOOL_COUNT; i++) {
		rvn_rect_t r;
		if (g_rt_tools[i].kind == RVN_RT_TOOL_GAP) continue;
		rvn_rt_tool_rect(m, i, &r);
		if (rvn_rect_contains(r, x, y) && r.x + r.w <= m->toolbar.x + m->toolbar.w) return i;
	}
	return -1;
}

static int rvn_rt_tool_active(const rvn_rich_text_t* rt, int index, uint32_t style, int any_link) {
	switch (g_rt_tools[index].kind) {
		case RVN_RT_TOOL_STYLE: return (style & g_rt_tools[index].value) != 0;
		case RVN_RT_TOOL_LINK: return any_link || rt->link_open;
		case RVN_RT_TOOL_BLOCK: return rt->doc.paras[rt->caret.para].kind == (int)g_rt_tools[index].value;
		default: return R_FALSE;
	}
}

static void rvn_rt_paint_toolbar(rvn_rich_text_t* rt, RPainter* painter, const rvn_rt_metrics_t* m) {
	rvn_widget_t* widget = &rt->base;
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	rvn_color_t accent = rvn_widget_color(widget, R_COLOR_ACCENT);
	rvn_color_t separator = rvn_widget_color(widget, R_COLOR_SEPARATOR);
	float radius = rvn_minf(rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS), 6.0f);
	int any_link, i;
	uint32_t style = rvn_rt_selection_style(rt, &any_link);

	rvn_paint_rect(painter, rvn_rect(m->toolbar.x, m->toolbar.y + m->toolbar.h - 1.0f, m->toolbar.w, 1.0f), separator);
	rvn_paint_push_clip(painter, m->toolbar);
	for (i = 0; i < RVN_RT_TOOL_COUNT; i++) {
		rvn_rect_t r;
		int active, hovered;
		rvn_color_t ink = text;
		rvn_rt_tool_rect(m, i, &r);
		if (g_rt_tools[i].kind == RVN_RT_TOOL_GAP) {
			rvn_paint_rect(painter, rvn_rect(r.x + 5.0f, r.y + 5.0f, 1.0f, r.h - 10.0f), separator);
			continue;
		}
		active = rvn_rt_tool_active(rt, i, style, any_link);
		hovered = rt->hot_tool == i;
		if (active) {
			rvn_paint_box(painter, r, radius, rvn_color_fade(accent, rt->pressed_tool == i ? 0.32f : 0.22f), 0.0f, 0);
			ink = accent;
		} else if (hovered && painter->enabled) {
			rvn_paint_box(painter, r, radius, rvn_widget_color(widget, rt->pressed_tool == i ? R_COLOR_CONTROL_PRESSED : R_COLOR_CONTROL_HOVERED), 0.0f, 0);
		}
		if (rt->read_only) ink = rvn_color_fade(ink, 0.45f);
		if (rt->icons[i] != R_INVALID_ID && Rtexture_is_valid(rt->icons[i])) {
			float side = floorf(m->button * 0.6f + 0.5f);
			rvn_rect_t icon = rvn_rect(floorf(r.x + (r.w - side) * 0.5f), floorf(r.y + (r.h - side) * 0.5f), side, side);
			rvn_paint_image(painter, icon, rt->icons[i], NULL, rvn_icon_tint(rt->icons[i], R_TRUE, ink, ink), 0.0f);
		}
	}
	rvn_paint_pop_clip(painter);
}

static void rvn_rt_link_field(const rvn_rt_metrics_t* m, float label_w, rvn_rect_t* frame) {
	float h = floorf(m->size * 1.9f + 0.5f);
	*frame = rvn_rect(m->linkbar.x + RVN_RT_PAD + label_w + 8.0f, m->linkbar.y + floorf((m->linkbar.h - h) * 0.5f),
		rvn_maxf(40.0f, m->linkbar.w - 2.0f * RVN_RT_PAD - label_w - 8.0f), h);
}

static float rvn_rt_link_label_w(const rvn_rich_text_t* rt, const rvn_rt_metrics_t* m) {
	return rvn_widget_text_width(&rt->base, R_INVALID_ID, m->size, rvn_rt_tr("Raven.Link", "Link"));
}

static void rvn_rt_paint_linkbar(rvn_rich_text_t* rt, RPainter* painter, const rvn_rt_metrics_t* m, int focused) {
	rvn_widget_t* widget = &rt->base;
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	float label_w = rvn_rt_link_label_w(rt, m), radius = rvn_minf(rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS), 6.0f);
	rvn_rect_t frame, inner;
	rvn_rt_link_field(m, label_w, &frame);
	inner = rvn_rect(frame.x + 6.0f, frame.y, frame.w - 12.0f, frame.h);
	rvn_paint_rect(painter, m->linkbar, rvn_color_fade(rvn_widget_color(widget, R_COLOR_PANEL), 0.6f));
	rvn_paint_rect(painter, rvn_rect(m->linkbar.x, m->linkbar.y + m->linkbar.h - 1.0f, m->linkbar.w, 1.0f), rvn_widget_color(widget, R_COLOR_SEPARATOR));
	rvn_paint_text_in(painter, R_INVALID_ID, m->size, rvn_rect(m->linkbar.x + RVN_RT_PAD, m->linkbar.y, label_w, m->linkbar.h), 0.0f,
		rvn_rt_tr("Raven.Link", "Link"), -1, text);
	rvn_paint_box(painter, frame, radius, rvn_widget_color(widget, R_COLOR_INPUT), focused ? 1.5f : 1.0f,
		rvn_widget_color(widget, focused ? R_COLOR_FOCUS : R_COLOR_BORDER));
	if (rt->link_edit.length == 0) {
		rvn_paint_text_in(painter, R_INVALID_ID, m->size, inner, 0.0f, rvn_rt_tr("Raven.LinkPlaceholder", "https://"), -1,
			rvn_widget_color(widget, R_COLOR_TEXT_PLACEHOLDER));
	}
	rvn_textedit_paint(&rt->link_edit, widget, painter, inner, R_INVALID_ID, m->size, text, focused);
}

/* The marker of an item: a disc, a ring, a square by level; or its number. */
static void rvn_rt_paint_marker(rvn_rich_text_t* rt, RPainter* painter, const rvn_rt_metrics_t* m, const rvn_rt_para_t* para,
	int number, float x, float y, rvn_color_t color) {

	float indent = rvn_rt_list_indent(m->size), x0 = x + rvn_rt_indent_x(para, m->size);
	const rvn_rt_line_t* line = para->line_count ? &para->lines[0] : NULL;
	float line_h = line ? line->h : m->size * 1.3f, mid = y + line_h * 0.5f;
	if (para->kind == RVN_RT_NUMBERED) {
		char label[16];
		float w;
		Rid font = rvn_font_variant(m->font, 0);
		snprintf(label, sizeof(label), "%d.", number);
		w = rvn_text_width(rvn_font_resolve(font), m->size, label, -1);
		/* (on the baseline of the first line) */
		if (line && line->frag_count) {
			const rvn_rt_frag_t* frag = &para->frags[line->first_frag];
			float height, base;
			rvn_rt_font_box(font, m->size, &height, &base);
			rvn_paint_text(painter, font, m->size, x0 - w - floorf(m->size * 0.35f), y + frag->y + frag->base - base, label, -1, color);
		} else {
			rvn_paint_text_in(painter, font, m->size, rvn_rect(x0 - w - floorf(m->size * 0.35f), y, w, line_h), 0.0f, label, -1, color);
		}
		(void)rt;
		return;
	}
	{
		float side = floorf(m->size * 0.36f), cx = x0 - floorf(indent * 0.45f);
		rvn_rect_t r = rvn_rect(floorf(cx - side * 0.5f), floorf(mid - side * 0.5f), side, side);
		switch (para->indent % 3) {
			case 0: rvn_paint_box(painter, r, side * 0.5f, color, 0.0f, 0); break;
			case 1: rvn_paint_box(painter, r, side * 0.5f, 0, 1.2f, color); break;
			default: rvn_paint_rect(painter, rvn_rect(r.x + 0.5f, r.y + 0.5f, side - 1.0f, side - 1.0f), color); break;
		}
	}
}

/* The selection over a paragraph whose top is at (x, y). */
static void rvn_rt_paint_selection(rvn_rich_text_t* rt, RPainter* painter, const rvn_rt_para_t* para, int index, float x, float y,
	rvn_color_t color, const rvn_rt_metrics_t* m) {

	rvn_rt_pos_t a = rvn_rt_sel_min(rt), b = rvn_rt_sel_max(rt);
	int from, to;
	uint32_t l;
	if (index < a.para || index > b.para) return;
	from = index == a.para ? a.offset : 0;
	to = index == b.para ? b.offset : para->length;
	for (l = 0; l < para->line_count; l++) {
		const rvn_rt_line_t* line = &para->lines[l];
		int s = rvn_maxi(from, line->start), e = rvn_mini(to, line->end);
		float x0, x1;
		int last = l + 1 == para->line_count;
		if (s > e || (s == e && !(last && index < b.para && s == para->length))) continue;
		x0 = rvn_rt_x_at(para, (int)l, s);
		x1 = rvn_rt_x_at(para, (int)l, e);
		/* the end of a paragraph selected: a little more, for its line break */
		if (last && index < b.para) x1 += floorf(m->size * 0.4f);
		if (x1 > x0) rvn_paint_rect(painter, rvn_rect(x + x0, y + line->y, x1 - x0, line->h), color);
	}
}

static rvn_color_t rvn_rt_run_color(const rvn_widget_t* widget, const rvn_rt_para_t* para, const rvn_rt_run_t* run, rvn_color_t text,
	rvn_color_t link, rvn_color_t dim) {
	if (run && run->link) return link;
	if (para->kind == RVN_RT_QUOTE) return dim;
	(void)widget;
	return text;
}

/* A paragraph whose top is at (x, y); `compose`: where a composition was put in it (-1: none) and its length. */
static void rvn_rt_paint_para(rvn_rich_text_t* rt, RPainter* painter, const rvn_rt_para_t* para, float x, float y,
	rvn_color_t text, int compose, int compose_length, float clip_top, float clip_bottom) {

	rvn_widget_t* widget = &rt->base;
	rvn_color_t link = painter->enabled ? rvn_widget_color(widget, R_COLOR_LINK) : text;
	rvn_color_t dim = rvn_color_fade(text, 0.72f);
	rvn_color_t code = rvn_widget_color(widget, R_COLOR_CODE);
	uint32_t l;
	for (l = 0; l < para->line_count; l++) {
		const rvn_rt_line_t* line = &para->lines[l];
		int f;
		if (y + line->y + line->h < clip_top || y + line->y > clip_bottom) continue;
		for (f = line->first_frag; f < line->first_frag + line->frag_count; f++) {
			const rvn_rt_frag_t* frag = &para->frags[f];
			const rvn_rt_run_t* run = frag->run < (int)para->run_count ? &para->runs[frag->run] : NULL;
			uint32_t style = run && para->kind != RVN_RT_CODE_BLOCK ? run->style : 0u;
			rvn_color_t color = rvn_rt_run_color(widget, para, run, text, link, dim);
			int length = frag->length;
			/* (what a line ends with, a break, draws nothing) */
			while (length > 0 && para->text[frag->start + length - 1] == '\n') length--;
			if (style & RVN_RT_CODE) {
				float w = rvn_text_width(rvn_font_resolve(frag->font), frag->size, para->text + frag->start, length);
				rvn_paint_box(painter, rvn_rect(x + frag->x - 2.0f, y + frag->y - 1.0f, w + 4.0f, frag->height + 2.0f), 4.0f, code, 0.0f, 0);
			}
			rvn_paint_text(painter, frag->font, frag->size, x + frag->x, y + frag->y, para->text + frag->start, length, color);
			if (style & RVN_RT_STRIKE) {
				rvn_paint_rect(painter, rvn_rect(x + frag->x, y + frag->y + floorf(frag->height * 0.55f), frag->w, 1.0f), color);
			}
			if ((style & RVN_RT_UNDERLINE) || (run && run->link)) {
				float w = frag->w;
				/* (not under the spaces a line ends with) */
				if (f == line->first_frag + line->frag_count - 1) {
					int end = frag->start + length;
					while (end > frag->start && para->text[end - 1] == ' ') end--;
					w = rvn_text_width(rvn_font_resolve(frag->font), frag->size, para->text + frag->start, end - frag->start);
				}
				rvn_paint_rect(painter, rvn_rect(x + frag->x, y + frag->y + frag->base + floorf(frag->size * 0.12f + 0.5f), w, 1.0f), color);
			}
			/* the composition of an input method, underlined */
			if (compose >= 0 && frag->start < compose + compose_length && frag->start + frag->length > compose) {
				int s = rvn_maxi(compose, frag->start), e = rvn_mini(compose + compose_length, frag->start + frag->length);
				rvn_font_t* font = rvn_font_resolve(frag->font);
				float x0 = x + frag->x + rvn_text_width(font, frag->size, para->text + frag->start, s - frag->start);
				float x1 = x + frag->x + rvn_text_width(font, frag->size, para->text + frag->start, e - frag->start);
				rvn_paint_composition_underline(painter, x0, x1, x0, x0, y + frag->y + frag->base + 2.0f, color);
			}
		}
	}
}

static void rvn_rt_paint_spelling(rvn_rich_text_t* rt, RPainter* painter, rvn_rt_para_t* para, int index, float x, float y) {
	rvn_color_t wave = rvn_widget_color(&rt->base, R_COLOR_SPELLING);
	int focused = (rt->base.flags & RVN_WF_FOCUSED) != 0, i;
	if (para->kind == RVN_RT_CODE_BLOCK || para->length == 0) return;
	if (!rvn_spell_line_current(&para->spell, para->text, para->length)) rvn_spell_line_update(&para->spell, &rt->spelling, para->text, para->length);
	for (i = 0; i < para->spell.count; i++) {
		int from = para->spell.ranges[i * 2], to = para->spell.ranges[i * 2 + 1];
		uint32_t l;
		/* (not the word being typed) */
		if (focused && index == rt->caret.para && rt->caret.offset >= from && rt->caret.offset <= to) continue;
		for (l = 0; l < para->line_count; l++) {
			const rvn_rt_line_t* line = &para->lines[l];
			int s = rvn_maxi(from, line->start), e = rvn_mini(to, line->end);
			if (e <= s) continue;
			rvn_paint_spelling(painter, x + rvn_rt_x_at(para, (int)l, s), x + rvn_rt_x_at(para, (int)l, e), y + line->y + line->h - 1.0f, wave);
		}
	}
}

static void rvn_rt_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_rich_text_t* rt = (rvn_rich_text_t*)widget;
	rvn_rt_metrics_t m;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	rvn_vec2_t origin = rvn_widget_origin(widget);
	float radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
	int focused = (widget->flags & RVN_WF_FOCUSED) != 0;
	int text_keys = focused && !rt->link_open;
	float focus = rvn_clampf(rvn_paint_spring(painter, &widget->press, focused ? 1.0f : 0.0f, R_MOTION_SNAPPY), 0.0f, 1.0f);
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	rvn_color_t selection = rvn_widget_color(widget, R_COLOR_SELECTION);
	rvn_color_t dim = rvn_color_fade(text, 0.55f);
	rvn_color_t accent = rvn_widget_color(widget, R_COLOR_ACCENT);
	int compose_start = 0, compose_end = 0;
	const char* composition = text_keys && !rt->read_only ? rvn_widget_composition(widget, &compose_start, &compose_end) : NULL;
	float scroll, clip_top, clip_bottom;
	int first, i, code_drawn = -1;
	rvn_rt_para_t composed;
	int has_composed = R_FALSE;

	rvn_rt_refresh(rt, &m);
	if (!text_keys) selection = rvn_color_fade(selection, 0.55f);

	/* frame */
	if (rvn_widget_glass(widget)) {
		rvn_paint_surface(painter, widget, rect, rvn_minf(radius, 14.0f), RVN_SURFACE_INPUT, 0.0f, 0.0f);
		if (focus > 0.01f) rvn_paint_box(painter, rect, rvn_minf(radius, 14.0f), 0, 1.5f, rvn_color_fade(rvn_widget_color(widget, R_COLOR_FOCUS), focus));
	} else {
		rvn_paint_box(painter, rect, radius, rvn_widget_color(widget, R_COLOR_INPUT), rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH),
			rvn_color_mix(rvn_widget_color(widget, R_COLOR_BORDER), rvn_widget_color(widget, R_COLOR_FOCUS), focus));
	}
	if (rt->toolbar) rvn_rt_paint_toolbar(rt, painter, &m);
	if (rt->link_open) rvn_rt_paint_linkbar(rt, painter, &m, focused);

	scroll = rvn_scrollbar_shown(&rt->bar, painter);
	scroll = floorf(scroll * painter->scale + 0.5f) / painter->scale;
	rvn_paint_push_clip(painter, m.view);
	clip_top = m.view.y;
	clip_bottom = m.view.y + m.view.h;

	/* the placeholder */
	if (rt->doc.count == 1 && rt->doc.paras[0].length == 0 && rt->placeholder && !composition) {
		rvn_paint_text(painter, R_INVALID_ID, m.size, m.text_x + rvn_rt_indent_x(&rt->doc.paras[0], m.size), m.text_y - scroll +
			(rt->doc.paras[0].line_count ? floorf((rt->doc.paras[0].lines[0].h - m.size * 1.25f) * 0.5f) : 0.0f), rt->placeholder, -1,
			rvn_widget_color(widget, R_COLOR_TEXT_PLACEHOLDER));
	}

	/* the composition of an input method: shown in the paragraph of the caret, not in the document */
	if (composition && composition[0]) {
		const char* link;
		uint32_t style;
		rvn_rt_typing_look(rt, &style, &link);
		rvn_rt_para_copy(&composed, &rt->doc.paras[rt->caret.para]);
		{
			rvn_rt_pos_t a = rvn_rt_sel_min(rt), b = rvn_rt_sel_max(rt);
			if (a.para == b.para) rvn_rt_para_delete(&composed, a.offset, b.offset);
			rvn_rt_para_insert(&composed, a.para == b.para ? a.offset : rt->caret.offset, composition, -1, style, NULL);
		}
		rvn_rt_layout_para(rt, &m, &composed);
		has_composed = R_TRUE;
	}

	first = rvn_rt_para_at_y(rt, scroll - RVN_RT_PAD);
	if (first > 0) first--;
	for (i = first; i < (int)rt->doc.count; i++) {
		rvn_rt_para_t* para = &rt->doc.paras[i];
		const rvn_rt_para_t* shown = has_composed && i == rt->caret.para ? &composed : para;
		float x = m.text_x, y = m.text_y + rt->tops[i] - scroll;
		if (y > clip_bottom + m.size) break;
		if (y + shown->height < clip_top - rvn_rt_code_pad(m.size) - m.size) continue;

		/* the box of a block of code, once for the lines on screen */
		if (para->kind == RVN_RT_CODE_BLOCK && i > code_drawn) {
			int s = i, e = i;
			float pad = rvn_rt_code_pad(m.size), top, bottom;
			while (s > 0 && rt->doc.paras[s - 1].kind == RVN_RT_CODE_BLOCK) s--;
			while (e + 1 < (int)rt->doc.count && rt->doc.paras[e + 1].kind == RVN_RT_CODE_BLOCK) e++;
			top = m.text_y + rt->tops[s] - scroll - pad;
			bottom = m.text_y + rt->tops[e] + rt->doc.paras[e].height - scroll + pad;
			rvn_paint_box(painter, rvn_rect(x, rvn_maxf(top, clip_top - 8.0f), m.width, rvn_minf(bottom, clip_bottom + 8.0f) - rvn_maxf(top, clip_top - 8.0f)),
				6.0f, rvn_widget_color(widget, R_COLOR_CODE), 1.0f, rvn_color_fade(rvn_widget_color(widget, R_COLOR_SEPARATOR), 0.6f));
			code_drawn = e;
		}
		/* the bar of a quote, joined to the quote above */
		if (para->kind == RVN_RT_QUOTE) {
			float top = y;
			if (i > 0 && rt->doc.paras[i - 1].kind == RVN_RT_QUOTE) top = m.text_y + rt->tops[i - 1] + rt->doc.paras[i - 1].height - scroll;
			rvn_paint_box(painter, rvn_rect(x + 2.0f, top, 3.0f, y + shown->height - top), 1.5f, rvn_color_fade(accent, 0.75f), 0.0f, 0);
		}
		/* a title of the first level is underlined across */
		if (para->kind == RVN_RT_HEADING_1) {
			rvn_paint_rect(painter, rvn_rect(x, y + shown->height + floorf(m.size * 0.15f), m.width, 1.0f), rvn_widget_color(widget, R_COLOR_SEPARATOR));
		}
		if (rvn_rt_kind_is_list(para->kind)) rvn_rt_paint_marker(rt, painter, &m, shown, rt->numbers[i], x, y, dim);
		if (rvn_rt_has_selection(rt) && !has_composed) rvn_rt_paint_selection(rt, painter, para, i, x, y, selection, &m);
		rvn_rt_paint_para(rt, painter, shown, x, y, text, shown == &composed ? rvn_rt_sel_min(rt).offset : -1,
			composition ? (int)strlen(composition) : 0, clip_top, clip_bottom);
		if (rvn_spelling_active(&rt->spelling) && shown == para) rvn_rt_paint_spelling(rt, painter, para, i, x, y);
	}

	/* the caret: blinks once it rests */
	if (text_keys && !rt->read_only) {
		rvn_rect_t box;
		if (has_composed) {
			int l, at = rvn_rt_sel_min(rt).offset + compose_end;
			l = rvn_rt_line_of(&composed, at);
			box = composed.line_count ? rvn_rect(rvn_rt_x_at(&composed, l, at), rt->tops[rt->caret.para] + composed.lines[l].y, 2.0f, composed.lines[l].h)
				: rvn_rt_caret_box(rt, rt->caret);
		} else {
			box = rvn_rt_caret_box(rt, rt->caret);
		}
		box = rvn_rect(m.text_x + box.x - 1.0f, m.text_y + box.y - scroll + 2.0f, 2.0f, rvn_maxf(4.0f, box.h - 4.0f));
		rvn_text_input_rect(widget, painter, box);
		if (g_rvn.test_deterministic || fmod(rvn_clock_seconds() - rt->blink_start, 1.0) < 0.55) {
			rvn_paint_box(painter, box, 1.0f, rvn_widget_color(widget, R_COLOR_ACCENT_HOVERED), 0.0f, 0);
		}
	}
	if (has_composed) rvn_rt_para_free(&composed);
	rvn_paint_pop_clip(painter);

	rvn_scrollbar_paint(&rt->bar, widget, painter, m.track, R_TRUE,
		widget->window ? widget->window->mouse_x - origin.x : -1.0f, widget->window ? widget->window->mouse_y - origin.y : -1.0f);
}

/* ---------------------------------------------------------------------------
 *  Keys
 * ------------------------------------------------------------------------- */

static void rvn_rt_blink(Rid timer, void* user_data) {
	rvn_widget_t* widget = rvn_widget_get((Rid)(uintptr_t)user_data);
	RVN_UNUSED(timer);
	if (widget) rvn_widget_invalidate_paint(widget);
}

static void rvn_rt_set_focused(rvn_rich_text_t* rt, int focused) {
	rvn_widget_t* widget = &rt->base;
	if (focused) {
		rt->blink_start = rvn_clock_seconds();
		if (rt->blink_timer == R_INVALID_ID) rt->blink_timer = Rtimer_create(0.5, R_TRUE, rvn_rt_blink, (void*)(uintptr_t)widget->id);
	} else {
		if (rt->blink_timer != R_INVALID_ID) {
			if (Rtimer_is_valid(rt->blink_timer)) Rtimer_destroy(rt->blink_timer);
			rt->blink_timer = R_INVALID_ID;
		}
		rt->selecting = R_FALSE;
		rvn_rt_break_typing(rt);
	}
	rvn_widget_invalidate_paint(widget);
}

/* A key that moves the caret. R_FALSE when it is not one. */
static int rvn_rt_motion(rvn_rich_text_t* rt, const rvn_rt_metrics_t* m, const REvent* event) {
	int shift = (event->modifiers & R_MOD_SHIFT) != 0;
	int super = (event->modifiers & R_MOD_SUPER) != 0;
	int word = (event->modifiers & (R_MOD_CTRL | R_MOD_ALT)) != 0;
	rvn_rt_pos_t target = rt->caret;
	int keep_goal = R_FALSE;
	const rvn_rt_para_t* para = &rt->doc.paras[rt->caret.para];
	switch (event->key) {
		case R_KEY_LEFT:
		case R_KEY_RIGHT: {
			int left = event->key == R_KEY_LEFT;
			if (!shift && rvn_rt_has_selection(rt) && !word && !super) {
				target = left ? rvn_rt_sel_min(rt) : rvn_rt_sel_max(rt);
			} else if (super) {
				/* Cmd+Left / Right: the ends of the line */
				int l = rvn_rt_line_of(para, rt->caret.offset);
				target.offset = left ? (para->line_count ? para->lines[l].start : 0) : (para->line_count ? rvn_rt_line_last(para, l) : 0);
			} else if (word) {
				target = left ? rvn_rt_word_left(rt, rt->caret) : rvn_rt_word_right(rt, rt->caret);
			} else {
				target = left ? rvn_rt_char_left(rt, rt->caret) : rvn_rt_char_right(rt, rt->caret);
			}
			break;
		}
		case R_KEY_UP:
		case R_KEY_DOWN: {
			int down = event->key == R_KEY_DOWN;
			if (super) {
				target = down ? rvn_rt_pos((int)rt->doc.count - 1, rt->doc.paras[rt->doc.count - 1].length) : rvn_rt_pos(0, 0);
				break;
			}
			if (event->modifiers & R_MOD_CTRL) {
				/* the start of the paragraph, of the next one */
				if (down) target = rt->caret.para + 1 < (int)rt->doc.count ? rvn_rt_pos(rt->caret.para + 1, 0) : rvn_rt_pos(rt->caret.para, para->length);
				else target = rt->caret.offset > 0 ? rvn_rt_pos(rt->caret.para, 0) : rvn_rt_pos(rvn_maxi(rt->caret.para - 1, 0), 0);
				break;
			}
			{
				int l = rvn_rt_line_of(para, rt->caret.offset), index = rt->caret.para;
				float goal = rt->goal_x >= 0.0f ? rt->goal_x : (para->line_count ? rvn_rt_x_at(para, l, rt->caret.offset) : 0.0f);
				rt->goal_x = goal;
				keep_goal = R_TRUE;
				if (down) {
					if (l + 1 < (int)para->line_count) l++;
					else if (index + 1 < (int)rt->doc.count) {
						index++;
						l = 0;
					} else {
						target = rvn_rt_pos(index, para->length);
						break;
					}
				} else {
					if (l > 0) l--;
					else if (index > 0) {
						index--;
						l = rvn_maxi((int)rt->doc.paras[index].line_count - 1, 0);
					} else {
						target = rvn_rt_pos(0, 0);
						break;
					}
				}
				target = rvn_rt_pos(index, rvn_rt_offset_in_line(&rt->doc.paras[index], l, goal));
			}
			break;
		}
		case R_KEY_PAGE_UP:
		case R_KEY_PAGE_DOWN: {
			rvn_rect_t box = rvn_rt_caret_box(rt, rt->caret);
			float page = rvn_maxf(m->view.h - m->size * 2.0f, m->size * 2.0f) * (event->key == R_KEY_PAGE_UP ? -1.0f : 1.0f);
			float goal = rt->goal_x >= 0.0f ? rt->goal_x : box.x;
			rt->goal_x = goal;
			keep_goal = R_TRUE;
			target = rvn_rt_pos_at(rt, goal, rvn_clampf(box.y + box.h * 0.5f + page, 0.0f, rt->content_h));
			rvn_scrollbar_set(&rt->bar, rt->bar.offset + page);
			break;
		}
		case R_KEY_HOME:
		case R_KEY_END: {
			int home = event->key == R_KEY_HOME;
			if (event->modifiers & (R_MOD_CTRL | R_MOD_SUPER)) {
				target = home ? rvn_rt_pos(0, 0) : rvn_rt_pos((int)rt->doc.count - 1, rt->doc.paras[rt->doc.count - 1].length);
			} else {
				int l = rvn_rt_line_of(para, rt->caret.offset);
				target.offset = home ? (para->line_count ? para->lines[l].start : 0) : (para->line_count ? rvn_rt_line_last(para, l) : 0);
			}
			break;
		}
		default:
			return R_FALSE;
	}
	rvn_rt_go(rt, target, shift);
	if (!keep_goal) rt->goal_x = -1.0f;
	return R_TRUE;
}

static int rvn_rt_link_key(rvn_rich_text_t* rt, const REvent* event) {
	int result;
	if (event->key == R_KEY_ENTER || event->key == R_KEY_KP_ENTER) {
		rvn_rt_link_apply(rt);
		return R_TRUE;
	}
	if (event->key == R_KEY_ESCAPE) {
		rvn_rt_link_close(rt);
		return R_TRUE;
	}
	if (event->key == R_KEY_TAB) return R_TRUE;
	result = rvn_textedit_key(&rt->link_edit, event);
	rvn_widget_invalidate_paint(&rt->base);
	return (result & RVN_TEXTEDIT_HANDLED) != 0 || !(event->modifiers & (R_MOD_CTRL | R_MOD_SUPER | R_MOD_ALT));
}

static int rvn_rt_key(rvn_rich_text_t* rt, const REvent* event) {
	int shift = (event->modifiers & R_MOD_SHIFT) != 0;
	int command = (event->modifiers & (R_MOD_CTRL | R_MOD_SUPER)) != 0;
	int alt = (event->modifiers & R_MOD_ALT) != 0;
	rvn_rt_metrics_t m;

	if (rt->link_open) return rvn_rt_link_key(rt, event);
	rvn_rt_refresh(rt, &m);
	if (rvn_rt_motion(rt, &m, event)) return R_TRUE;

	if (command && !alt) {
		switch (event->key) {
			case R_KEY_A:
				if (shift) return R_FALSE;
				rt->anchor = rvn_rt_pos(0, 0);
				rt->caret = rvn_rt_pos((int)rt->doc.count - 1, rt->doc.paras[rt->doc.count - 1].length);
				rt->typing_set = R_FALSE;
				rvn_rt_break_typing(rt);
				rvn_widget_invalidate_paint(&rt->base);
				return R_TRUE;
			case R_KEY_C:
				if (shift) return R_FALSE;
				rvn_rt_copy(rt);
				return R_TRUE;
			case R_KEY_X:
				if (shift) {
					rvn_rt_toggle_style(rt, RVN_RT_STRIKE);
					return R_TRUE;
				}
				rvn_rt_copy(rt);
				if (!rt->read_only && rvn_rt_has_selection(rt)) {
					rvn_rt_begin_selection(rt, R_FALSE);
					rvn_rt_delete_selection(rt);
					rvn_rt_end(rt);
					rvn_rt_break_typing(rt);
				}
				return R_TRUE;
			case R_KEY_V:
				rvn_rt_paste(rt);
				return R_TRUE;
			case R_KEY_Z:
				if (shift) rvn_rt_redo(rt);
				else rvn_rt_undo(rt);
				return R_TRUE;
			case R_KEY_Y:
				rvn_rt_redo(rt);
				return R_TRUE;
			case R_KEY_B:
				rvn_rt_toggle_style(rt, RVN_RT_BOLD);
				return R_TRUE;
			case R_KEY_I:
				rvn_rt_toggle_style(rt, RVN_RT_ITALIC);
				return R_TRUE;
			case R_KEY_U:
				rvn_rt_toggle_style(rt, RVN_RT_UNDERLINE);
				return R_TRUE;
			case R_KEY_E:
				rvn_rt_toggle_style(rt, RVN_RT_CODE);
				return R_TRUE;
			case R_KEY_K:
				rvn_rt_link_open(rt);
				return R_TRUE;
			case R_KEY_0:
			case R_KEY_1:
			case R_KEY_2:
			case R_KEY_3:
			case R_KEY_7:
			case R_KEY_8:
			case R_KEY_9: {
				static const int kinds[10] = { RVN_RT_PARAGRAPH, RVN_RT_HEADING_1, RVN_RT_HEADING_2, RVN_RT_HEADING_3, -1, -1, -1,
					RVN_RT_NUMBERED, RVN_RT_BULLET, RVN_RT_QUOTE };
				if (!shift) return R_FALSE;
				rvn_rt_set_block(rt, kinds[event->key - R_KEY_0], event->key != R_KEY_0);
				return R_TRUE;
			}
			default:
				return R_FALSE;
		}
	}
	if (command || (alt && event->key != R_KEY_BACKSPACE && event->key != R_KEY_DELETE)) return R_FALSE;

	switch (event->key) {
		case R_KEY_BACKSPACE:
		case R_KEY_DELETE:
			rvn_rt_erase(rt, event->key == R_KEY_BACKSPACE, alt);
			return R_TRUE;
		case R_KEY_ENTER:
		case R_KEY_KP_ENTER:
			if (rt->read_only) return R_FALSE;
			if (shift) rvn_rt_line_break(rt);
			else rvn_rt_enter(rt);
			return R_TRUE;
		case R_KEY_TAB: {
			const rvn_rt_para_t* para = &rt->doc.paras[rt->caret.para];
			if (rt->read_only) return R_FALSE;
			if (rvn_rt_indent(rt, shift ? -1 : 1)) return R_TRUE;
			if (para->kind == RVN_RT_CODE_BLOCK && !shift) {
				rvn_rt_type(rt, "    ", 4, R_FALSE);
				return R_TRUE;
			}
			/* (elsewhere Tab goes to the next widget) */
			return R_FALSE;
		}
		case R_KEY_ESCAPE:
			if (!rvn_rt_has_selection(rt)) return R_FALSE;
			rvn_rt_go(rt, rt->caret, R_FALSE);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

/* Tab at level 0 with Shift: an item becomes a paragraph again (rvn_rt_indent keeps it an item). */
static int rvn_rt_outdent_out(rvn_rich_text_t* rt, const REvent* event) {
	rvn_rt_para_t* para = &rt->doc.paras[rt->caret.para];
	if (event->key != R_KEY_TAB || !(event->modifiers & R_MOD_SHIFT) || rt->read_only || rvn_rt_has_selection(rt)) return R_FALSE;
	if (!rvn_rt_kind_is_list(para->kind) || para->indent > 0) return R_FALSE;
	rvn_rt_begin(rt, rt->caret.para, 1, R_FALSE);
	rvn_rt_set_kind_of(&rt->doc.paras[rt->caret.para], RVN_RT_PARAGRAPH);
	rvn_rt_end(rt);
	rvn_rt_break_typing(rt);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  The mouse
 * ------------------------------------------------------------------------- */

static void rvn_rt_select_units(rvn_rich_text_t* rt, rvn_rt_pos_t pos) {
	rvn_rt_pos_t from = pos, to = pos;
	if (rt->unit == 2) rvn_rt_word_at(rt, pos, &from, &to);
	else if (rt->unit >= 3) {
		from = rvn_rt_pos(pos.para, 0);
		to = rvn_rt_pos(pos.para, rt->doc.paras[pos.para].length);
	}
	if (rvn_rt_pos_cmp(from, rt->unit_from) < 0) {
		rt->anchor = rt->unit_to;
		rt->caret = from;
	} else {
		rt->anchor = rt->unit_from;
		rt->caret = rvn_rt_pos_cmp(to, rt->unit_to) > 0 ? to : rt->unit_to;
	}
}

static int rvn_rt_mouse(rvn_rich_text_t* rt, const REvent* event) {
	rvn_widget_t* widget = &rt->base;
	rvn_rt_metrics_t m;
	int changed = R_FALSE, tool;
	float dx, dy;
	rvn_rt_refresh(rt, &m);

	/* the scrollbar */
	if (!rt->selecting && !rt->link_selecting && rvn_scrollbar_event(&rt->bar, widget, event, m.track, R_TRUE, &changed)) {
		rvn_widget_invalidate_paint(widget);
		return R_TRUE;
	}
	/* the toolbar */
	tool = rt->selecting ? -1 : rvn_rt_tool_at(rt, &m, event->local_x, event->local_y);
	if (event->type == R_EVENT_MOUSE_MOVE && !rt->selecting && tool != rt->hot_tool) {
		rt->hot_tool = tool;
		rvn_widget_invalidate_paint(widget);
	}
	if (rt->toolbar && rvn_rect_contains(m.toolbar, event->local_x, event->local_y) && !rt->selecting && !rt->link_selecting) {
		widget->cursor = R_CURSOR_ARROW;
		if (event->type == R_EVENT_MOUSE_DOWN && event->button == R_MOUSE_LEFT) {
			rt->pressed_tool = tool;
			rvn_widget_invalidate_paint(widget);
		} else if (event->type == R_EVENT_MOUSE_UP && event->button == R_MOUSE_LEFT) {
			int pressed = rt->pressed_tool;
			rt->pressed_tool = -1;
			rvn_widget_invalidate_paint(widget);
			if (pressed >= 0 && pressed == tool && !rt->read_only) {
				switch (g_rt_tools[tool].kind) {
					case RVN_RT_TOOL_STYLE: rvn_rt_toggle_style(rt, g_rt_tools[tool].value); break;
					case RVN_RT_TOOL_LINK:
						if (rt->link_open) rvn_rt_link_close(rt);
						else rvn_rt_link_open(rt);
						break;
					case RVN_RT_TOOL_BLOCK: rvn_rt_set_block(rt, (int)g_rt_tools[tool].value, R_TRUE); break;
					default: break;
				}
			}
		}
		return R_TRUE;
	}
	if (event->type == R_EVENT_MOUSE_UP && rt->pressed_tool >= 0) {
		rt->pressed_tool = -1;
		rvn_widget_invalidate_paint(widget);
	}

	/* the bar of the link */
	if (rt->link_open && (rt->link_selecting || rvn_rect_contains(m.linkbar, event->local_x, event->local_y))) {
		rvn_rect_t frame;
		rvn_rt_link_field(&m, rvn_rt_link_label_w(rt, &m), &frame);
		widget->cursor = R_CURSOR_IBEAM;
		if (event->type == R_EVENT_MOUSE_DOWN && event->button == R_MOUSE_LEFT) {
			rvn_textedit_mouse_down(&rt->link_edit, widget, R_INVALID_ID, m.size, event->local_x - (frame.x + 6.0f), event->clicks,
				(event->modifiers & R_MOD_SHIFT) != 0);
			rt->link_selecting = R_TRUE;
			rvn_widget_capture_mouse(widget);
		} else if (event->type == R_EVENT_MOUSE_MOVE && rt->link_selecting) {
			rvn_textedit_mouse_drag(&rt->link_edit, widget, R_INVALID_ID, m.size, event->local_x - (frame.x + 6.0f));
		} else if (event->type == R_EVENT_MOUSE_UP && rt->link_selecting) {
			rt->link_selecting = R_FALSE;
			rvn_widget_release_mouse(widget);
		}
		rvn_widget_invalidate_paint(widget);
		return R_TRUE;
	}

	dx = event->local_x - m.text_x;
	dy = event->local_y - m.text_y + rt->bar.offset;
	switch (event->type) {
		case R_EVENT_MOUSE_DOWN: {
			rvn_rt_pos_t pos;
			const char* link;
			int command = (event->modifiers & (R_MOD_CTRL | R_MOD_SUPER)) != 0;
			if (!rvn_rect_contains(m.view, event->local_x, event->local_y)) return R_FALSE;
			if (event->button != R_MOUSE_LEFT) return event->button == R_MOUSE_RIGHT;
			if (rt->link_open) rvn_rt_link_close(rt);
			link = rvn_rt_link_at(rt, dx, dy);
			if (link && (command || rt->read_only) && event->clicks == 1) {
				rvn_rt_follow(widget, link);
				return R_TRUE;
			}
			pos = rvn_rt_pos_at(rt, dx, dy);
			rt->unit = rvn_clampi(event->clicks, 1, 3);
			if (rt->unit == 1) {
				rvn_rt_go(rt, pos, (event->modifiers & R_MOD_SHIFT) != 0);
				rt->unit_from = rt->unit_to = pos;
			} else {
				rvn_rt_pos_t from = pos, to = pos;
				if (rt->unit == 2) rvn_rt_word_at(rt, pos, &from, &to);
				else {
					from = rvn_rt_pos(pos.para, 0);
					to = rvn_rt_pos(pos.para, rt->doc.paras[pos.para].length);
				}
				rt->unit_from = from;
				rt->unit_to = to;
				rvn_rt_go(rt, to, R_FALSE);
				rt->anchor = from;
			}
			rt->goal_x = -1.0f;
			rt->reveal = R_FALSE;
			rt->selecting = R_TRUE;
			rvn_widget_capture_mouse(widget);
			return R_TRUE;
		}
		case R_EVENT_MOUSE_MOVE:
			if (rt->selecting) {
				rvn_rt_pos_t pos;
				/* past the top or the bottom: the text comes to the pointer */
				if (event->local_y < m.view.y) rvn_scrollbar_set(&rt->bar, rt->bar.offset - m.size);
				else if (event->local_y > m.view.y + m.view.h) rvn_scrollbar_set(&rt->bar, rt->bar.offset + m.size);
				dy = rvn_clampf(event->local_y, m.view.y, m.view.y + m.view.h) - m.text_y + rt->bar.offset;
				pos = rvn_rt_pos_at(rt, dx, dy);
				if (rt->unit == 1) rt->caret = pos;
				else rvn_rt_select_units(rt, pos);
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			{
				const char* link = rvn_rect_contains(m.view, event->local_x, event->local_y) ? rvn_rt_link_at(rt, dx, dy) : NULL;
				int command = (event->modifiers & (R_MOD_CTRL | R_MOD_SUPER)) != 0;
				widget->cursor = link && (command || rt->read_only) ? R_CURSOR_HAND :
					(rvn_rect_contains(m.view, event->local_x, event->local_y) ? R_CURSOR_IBEAM : R_CURSOR_ARROW);
			}
			return R_FALSE;
		case R_EVENT_MOUSE_UP:
			if (event->button == R_MOUSE_RIGHT && !rt->selecting && rvn_spelling_active(&rt->spelling) &&
				rvn_rect_contains(m.view, event->local_x, event->local_y)) {
				/* the corrections of a misspelled word */
				rvn_rt_pos_t pos = rvn_rt_pos_at(rt, dx, dy);
				rvn_rt_para_t* para = &rt->doc.paras[pos.para];
				int from, to;
				if (para->kind == RVN_RT_CODE_BLOCK) return R_FALSE;
				rvn_spell_line_update(&para->spell, &rt->spelling, para->text, para->length);
				if (!rvn_spell_line_hit(&para->spell, pos.offset, &from, &to)) return R_FALSE;
				rt->spelling.read_only = rt->read_only;
				return rvn_spelling_open_menu(&rt->spelling, widget, para->text, pos.para, from, to);
			}
			if (event->button != R_MOUSE_LEFT || !rt->selecting) return R_FALSE;
			rt->selecting = R_FALSE;
			rvn_widget_release_mouse(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_WHEEL:
			changed = rvn_scrollbar_wheel(&rt->bar, -event->wheel_y * m.size * 3.6f);
			rvn_widget_invalidate_paint(widget);
			return changed;
		default:
			return R_FALSE;
	}
}

static int rvn_rt_event(rvn_widget_t* widget, const REvent* event) {
	rvn_rich_text_t* rt = (rvn_rich_text_t*)widget;
	int used = R_FALSE;
	switch (event->type) {
		case R_EVENT_FOCUS_GAINED:
			rvn_rt_set_focused(rt, R_TRUE);
			return R_TRUE;
		case R_EVENT_FOCUS_LOST:
			rvn_rt_set_focused(rt, R_FALSE);
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			if (!(widget->flags & RVN_WF_FOCUSED)) return R_FALSE;
			rt->blink_start = rvn_clock_seconds();
			used = rvn_rt_outdent_out(rt, event) || rvn_rt_key(rt, event);
			break;
		case R_EVENT_TEXT:
			if (!(widget->flags & RVN_WF_FOCUSED)) return R_FALSE;
			if (event->codepoint < 32 || event->codepoint == 127) return R_FALSE;
			if (rt->link_open) {
				rvn_textedit_input(&rt->link_edit, event->codepoint);
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			if (rt->read_only) return R_FALSE;
			rvn_rt_type_char(rt, event->codepoint);
			used = R_TRUE;
			break;
		case R_EVENT_MOUSE_DOWN:
		case R_EVENT_MOUSE_MOVE:
		case R_EVENT_MOUSE_UP:
		case R_EVENT_MOUSE_WHEEL:
			used = rvn_rt_mouse(rt, event);
			break;
		case R_EVENT_MOUSE_LEAVE:
			if (rt->hot_tool >= 0) {
				rt->hot_tool = -1;
				rvn_widget_invalidate_paint(widget);
			}
			return R_FALSE;
		default:
			return R_FALSE;
	}
	if (rt->reveal) {
		rvn_rt_metrics_t m;
		rvn_rt_refresh(rt, &m);
	}
	rvn_widget_invalidate_paint(widget);
	if (!rvn_rt_tell(widget)) return R_TRUE;
	return used;
}

static int rvn_rt_pan(rvn_widget_t* widget, float dx, float dy, int coasting) {
	rvn_rich_text_t* rt = (rvn_rich_text_t*)widget;
	RVN_UNUSED(dx);
	return rvn_scrollbar_pan(&rt->bar, -dy, coasting);
}

/* ---------------------------------------------------------------------------
 *  The class
 * ------------------------------------------------------------------------- */

static void rvn_rt_spell_replace(rvn_widget_t* widget, int line, int from, int to, const char* word, const char* text) {
	rvn_rich_text_t* rt = (rvn_rich_text_t*)widget;
	rvn_rt_para_t* para;
	uint32_t style = 0;
	const char* link = NULL;
	char* kept;
	int run, start;
	if (rt->read_only || line < 0 || (uint32_t)line >= rt->doc.count) return;
	para = &rt->doc.paras[line];
	if (to > para->length || (int)strlen(word) != to - from || memcmp(para->text + from, word, (size_t)(to - from)) != 0) return;
	run = rvn_rt_para_run_at(para, from, &start);
	if (run >= 0) {
		style = para->runs[run].style;
		link = para->runs[run].link;
	}
	kept = link ? rvn_strdup(link) : NULL;
	rt->caret = rt->anchor = rvn_rt_pos(line, from);
	rvn_rt_begin(rt, line, 1, R_FALSE);
	para = &rt->doc.paras[line];
	rvn_rt_para_delete(para, from, to);
	rvn_rt_para_insert(para, from, text, -1, style, kept);
	rt->caret = rt->anchor = rvn_rt_pos(line, from + (int)strlen(text));
	rvn_rt_end(rt);
	rvn_rt_break_typing(rt);
	rvn_free(kept);
	rvn_rt_tell(widget);
}

static void rvn_rt_init(rvn_widget_t* widget) {
	rvn_rich_text_t* rt = (rvn_rich_text_t*)widget;
	int i;
	rvn_rt_doc_init(&rt->doc);
	rt->goal_x = -1.0f;
	rt->editing = -1;
	rt->toolbar = R_TRUE;
	rt->shortcuts = R_TRUE;
	rt->font = R_INVALID_ID;
	rt->hot_tool = -1;
	rt->pressed_tool = -1;
	rt->blink_timer = R_INVALID_ID;
	rt->tops_dirty = R_TRUE;
	for (i = 0; i < RVN_RT_TOOL_COUNT; i++) rt->icons[i] = R_INVALID_ID;
	rt->spelling.replace = rvn_rt_spell_replace;
	widget->cursor = R_CURSOR_IBEAM;
}

static void rvn_rt_destroy(rvn_widget_t* widget) {
	rvn_rich_text_t* rt = (rvn_rich_text_t*)widget;
	if (rt->blink_timer != R_INVALID_ID && Rtimer_is_valid(rt->blink_timer)) Rtimer_destroy(rt->blink_timer);
	if (rt->link_ready) {
		rvn_textedit_set_focused(&rt->link_edit, widget, R_FALSE);
		rvn_textedit_free(&rt->link_edit);
	}
	rvn_rt_doc_free(&rt->doc);
	rvn_rt_history_clear(rt->undo, &rt->undo_count);
	rvn_rt_history_clear(rt->redo, &rt->redo_count);
	rvn_free(rt->undo);
	rvn_free(rt->redo);
	rvn_free(rt->tops);
	rvn_free(rt->numbers);
	rvn_free(rt->atoms);
	rvn_free(rt->placeholder);
	rvn_free(rt->markdown_cache.data);
	rvn_free(rt->html_cache.data);
	rvn_free(rt->text_cache.data);
	rvn_free(rt->scratch.data);
	rvn_spelling_free(&rt->spelling);
}

static rvn_vec2_t rvn_rt_measure(rvn_widget_t* widget) {
	RVN_UNUSED(widget);
	/* (usually given its size by a fill slot) */
	return rvn_vec2(360.0f, 240.0f);
}

static void rvn_rt_arrange(rvn_widget_t* widget) {
	rvn_rich_text_t* rt = (rvn_rich_text_t*)widget;
	rvn_rt_metrics_t m;
	int i;
	/* the icons of the toolbar, made once */
	if (rt->toolbar) {
		for (i = 0; i < RVN_RT_TOOL_COUNT; i++) {
			if (g_rt_tools[i].icon && (rt->icons[i] == R_INVALID_ID || !Rtexture_is_valid(rt->icons[i]))) rt->icons[i] = Ricon_get(g_rt_tools[i].icon);
		}
	}
	rvn_rt_refresh(rt, &m);
}

/* A new document: the history goes, the caret goes to its start. */
static void rvn_rt_replaced(rvn_rich_text_t* rt) {
	rvn_rt_history_clear(rt->undo, &rt->undo_count);
	rvn_rt_history_clear(rt->redo, &rt->redo_count);
	rt->caret = rt->anchor = rvn_rt_pos(0, 0);
	rt->typing_set = R_FALSE;
	rt->goal_x = -1.0f;
	rvn_scrollbar_set(&rt->bar, 0.0f);
	rvn_rt_dirty(rt);
	rvn_widget_invalidate_layout(&rt->base);
}

static int rvn_rt_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_rich_text_t* rt = (rvn_rich_text_t*)widget;
	int flag;
	if (RVN_ATTR("Text") || RVN_ATTR("Markdown")) {
		rvn_rt_from_markdown(&rt->doc, value);
		rvn_rt_replaced(rt);
		return R_TRUE;
	}
	if (RVN_ATTR("Html")) {
		rvn_rt_from_html(&rt->doc, value);
		rvn_rt_replaced(rt);
		return R_TRUE;
	}
	if (RVN_ATTR("PlainText")) {
		rvn_rt_from_text(&rt->doc, value);
		rvn_rt_replaced(rt);
		return R_TRUE;
	}
	if (RVN_ATTR("Placeholder")) {
		rvn_str_set(&rt->placeholder, value && value[0] ? value : NULL);
		rvn_widget_invalidate_paint(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("ReadOnly")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		rt->read_only = flag;
		rvn_widget_invalidate_paint(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("Toolbar")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		rt->toolbar = flag;
		rvn_widget_invalidate_layout(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("MarkdownShortcuts")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		rt->shortcuts = flag;
		return R_TRUE;
	}
	if (RVN_ATTR("SpellCheck")) {
		if (!rvn_parse_bool(value, &rt->spelling.enabled)) return R_FALSE;
		rvn_widget_invalidate_paint(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("SpellLanguage")) {
		rvn_spelling_set_language(&rt->spelling, value);
		rvn_widget_invalidate_paint(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("Font")) {
		Rid font = rvn_xml_font(value);
		if (font == R_INVALID_ID) return R_FALSE;
		rt->font = font;
		rvn_widget_invalidate_layout(widget);
		return R_TRUE;
	}
	if (RVN_ATTR("TextSize") || RVN_ATTR("FontSize")) {
		float size;
		if (!rvn_parse_float(value, &size)) return R_FALSE;
		rt->text_size = rvn_maxf(0.0f, size);
		rvn_widget_invalidate_layout(widget);
		return R_TRUE;
	}
	return R_FALSE;
}

const rvn_widget_class_t rvn_class_rich_text_editor = {
	.type = R_WIDGET_RICH_TEXT_EDITOR,
	.name = "RichTextEditor",
	.size = sizeof(rvn_rich_text_t),
	/* (its paint lays out a composition and checks the spelling of the paragraphs on screen) */
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_SURFACE | RVN_CF_NO_MIRROR | RVN_CF_UI_PAINT,
	.init = rvn_rt_init,
	.destroy = rvn_rt_destroy,
	.measure = rvn_rt_measure,
	.arrange = rvn_rt_arrange,
	.paint = rvn_rt_paint,
	.event = rvn_rt_event,
	.set_attribute = rvn_rt_set_attribute,
	.pan = rvn_rt_pan
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

#define RVN_RICH(variable, id, result) \
	rvn_widget_t* widget = rvn_widget_check((id), R_WIDGET_RICH_TEXT_EDITOR, __func__); \
	rvn_rich_text_t* variable = (rvn_rich_text_t*)widget; \
	if (!widget) return result

#define RVN_RICH_VOID(variable, id) \
	rvn_widget_t* widget = rvn_widget_check((id), R_WIDGET_RICH_TEXT_EDITOR, __func__); \
	rvn_rich_text_t* variable = (rvn_rich_text_t*)widget; \
	if (!widget) return

Rid Rrich_text_editor_create(void) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_RICH_TEXT_EDITOR);
	return widget ? widget->id : R_INVALID_ID;
}

void Rrich_text_editor_set_markdown(Rid id, const char* markdown) {
	RVN_RICH_VOID(rt, id);
	rvn_rt_from_markdown(&rt->doc, markdown);
	rvn_rt_replaced(rt);
	rvn_rt_tell(widget);
}

const char* Rrich_text_editor_get_markdown(Rid id) {
	RVN_RICH(rt, id, "");
	rt->markdown_cache.length = 0;
	rvn_rt_to_markdown(&rt->doc, &rt->markdown_cache);
	return rt->markdown_cache.data ? rt->markdown_cache.data : "";
}

void Rrich_text_editor_set_html(Rid id, const char* html) {
	RVN_RICH_VOID(rt, id);
	rvn_rt_from_html(&rt->doc, html);
	rvn_rt_replaced(rt);
	rvn_rt_tell(widget);
}

const char* Rrich_text_editor_get_html(Rid id) {
	RVN_RICH(rt, id, "");
	rt->html_cache.length = 0;
	rvn_rt_to_html(&rt->doc, &rt->html_cache);
	return rt->html_cache.data ? rt->html_cache.data : "";
}

void Rrich_text_editor_set_text(Rid id, const char* text) {
	RVN_RICH_VOID(rt, id);
	rvn_rt_from_text(&rt->doc, text);
	rvn_rt_replaced(rt);
	rvn_rt_tell(widget);
}

const char* Rrich_text_editor_get_text(Rid id) {
	RVN_RICH(rt, id, "");
	rt->text_cache.length = 0;
	rvn_rt_to_text(&rt->doc, &rt->text_cache);
	return rt->text_cache.data ? rt->text_cache.data : "";
}

void Rrich_text_editor_insert_text(Rid id, const char* text) {
	RVN_RICH_VOID(rt, id);
	if (!text || !text[0]) return;
	rvn_rt_type(rt, text, (int)strlen(text), R_FALSE);
	rvn_rt_break_typing(rt);
	rvn_rt_tell(widget);
}

void Rrich_text_editor_insert_html(Rid id, const char* html) {
	rvn_rt_doc_t piece;
	RVN_RICH_VOID(rt, id);
	memset(&piece, 0, sizeof(piece));
	rvn_rt_from_html(&piece, html);
	rvn_rt_paste_doc(rt, &piece);
	rvn_rt_tell(widget);
}

void Rrich_text_editor_insert_markdown(Rid id, const char* markdown) {
	rvn_rt_doc_t piece;
	RVN_RICH_VOID(rt, id);
	memset(&piece, 0, sizeof(piece));
	rvn_rt_from_markdown(&piece, markdown);
	rvn_rt_paste_doc(rt, &piece);
	rvn_rt_tell(widget);
}

int Rrich_text_editor_get_paragraph_count(Rid id) {
	RVN_RICH(rt, id, 0);
	return (int)rt->doc.count;
}

const char* Rrich_text_editor_get_paragraph_text(Rid id, int paragraph) {
	RVN_RICH(rt, id, "");
	if (paragraph < 0 || paragraph >= (int)rt->doc.count) return "";
	rt->scratch.length = 0;
	rvn_text_append(&rt->scratch, rt->doc.paras[paragraph].text ? rt->doc.paras[paragraph].text : "", (size_t)rt->doc.paras[paragraph].length);
	return rt->scratch.data;
}

RRichTextBlock Rrich_text_editor_get_paragraph_block(Rid id, int paragraph) {
	RVN_RICH(rt, id, R_RICH_TEXT_PARAGRAPH);
	if (paragraph < 0 || paragraph >= (int)rt->doc.count) return R_RICH_TEXT_PARAGRAPH;
	return (RRichTextBlock)((int)R_RICH_TEXT_PARAGRAPH + rt->doc.paras[paragraph].kind);
}

int Rrich_text_editor_get_paragraph_indent(Rid id, int paragraph) {
	RVN_RICH(rt, id, 0);
	if (paragraph < 0 || paragraph >= (int)rt->doc.count) return 0;
	return rt->doc.paras[paragraph].indent;
}

Ruint Rrich_text_editor_get_style_at(Rid id, int paragraph, int offset) {
	const rvn_rt_para_t* para;
	int run, start;
	RVN_RICH(rt, id, 0);
	if (paragraph < 0 || paragraph >= (int)rt->doc.count) return 0;
	para = &rt->doc.paras[paragraph];
	if (offset < 0 || offset >= para->length) return 0;
	run = rvn_rt_para_run_at(para, offset, &start);
	if (run < 0) return 0;
	return para->runs[run].style | (para->runs[run].link ? RVN_RT_LINK : 0u);
}

void Rrich_text_editor_set_selection(Rid id, int anchor_paragraph, int anchor_offset, int caret_paragraph, int caret_offset) {
	RVN_RICH_VOID(rt, id);
	rt->anchor = rvn_rt_clamp(rt, rvn_rt_pos(anchor_paragraph, anchor_offset));
	rvn_rt_go(rt, rvn_rt_pos(caret_paragraph, caret_offset), R_TRUE);
	rt->goal_x = -1.0f;
	rvn_rt_tell(widget);
}

void Rrich_text_editor_get_selection(Rid id, int* anchor_paragraph, int* anchor_offset, int* caret_paragraph, int* caret_offset) {
	RVN_RICH_VOID(rt, id);
	if (anchor_paragraph) *anchor_paragraph = rt->anchor.para;
	if (anchor_offset) *anchor_offset = rt->anchor.offset;
	if (caret_paragraph) *caret_paragraph = rt->caret.para;
	if (caret_offset) *caret_offset = rt->caret.offset;
}

void Rrich_text_editor_select_all(Rid id) {
	RVN_RICH_VOID(rt, id);
	rt->anchor = rvn_rt_pos(0, 0);
	rvn_rt_go(rt, rvn_rt_pos((int)rt->doc.count - 1, rt->doc.paras[rt->doc.count - 1].length), R_TRUE);
	rvn_rt_tell(widget);
}

const char* Rrich_text_editor_get_selected_text(Rid id) {
	rvn_rt_doc_t piece;
	RVN_RICH(rt, id, "");
	rt->scratch.length = 0;
	rvn_rt_doc_slice(&rt->doc, rvn_rt_sel_min(rt), rvn_rt_sel_max(rt), &piece);
	rvn_rt_to_text(&piece, &rt->scratch);
	rvn_rt_doc_free(&piece);
	return rt->scratch.data ? rt->scratch.data : "";
}

void Rrich_text_editor_set_style(Rid id, Ruint style, int enabled) {
	RVN_RICH_VOID(rt, id);
	rvn_rt_set_style(rt, style, enabled);
	rvn_rt_tell(widget);
}

void Rrich_text_editor_toggle_style(Rid id, Ruint style) {
	RVN_RICH_VOID(rt, id);
	if (!(style & RVN_RT_STYLES)) return;
	rvn_rt_toggle_style(rt, style & RVN_RT_STYLES);
	rvn_rt_tell(widget);
}

Ruint Rrich_text_editor_get_style(Rid id) {
	int any_link;
	uint32_t style;
	RVN_RICH(rt, id, 0);
	style = rvn_rt_selection_style(rt, &any_link);
	return style | (any_link ? RVN_RT_LINK : 0u);
}

void Rrich_text_editor_set_block(Rid id, RRichTextBlock block) {
	RVN_RICH_VOID(rt, id);
	if ((int)block < (int)R_RICH_TEXT_PARAGRAPH || (int)block > (int)R_RICH_TEXT_CODE_BLOCK) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: unknown block %d", __func__, (int)block);
		return;
	}
	rvn_rt_set_block(rt, (int)block - (int)R_RICH_TEXT_PARAGRAPH, R_FALSE);
	rvn_rt_tell(widget);
}

RRichTextBlock Rrich_text_editor_get_block(Rid id) {
	RVN_RICH(rt, id, R_RICH_TEXT_PARAGRAPH);
	return (RRichTextBlock)((int)R_RICH_TEXT_PARAGRAPH + rt->doc.paras[rt->caret.para].kind);
}

void Rrich_text_editor_indent(Rid id, int levels) {
	RVN_RICH_VOID(rt, id);
	if (levels == 0) return;
	rvn_rt_indent(rt, levels);
	rvn_rt_tell(widget);
}

void Rrich_text_editor_set_link(Rid id, const char* url) {
	RVN_RICH_VOID(rt, id);
	rvn_rt_set_link(rt, url);
	rvn_rt_tell(widget);
}

const char* Rrich_text_editor_get_link(Rid id) {
	int from, to;
	const char* url = NULL;
	rvn_rt_pos_t a;
	const rvn_rt_para_t* para;
	RVN_RICH(rt, id, NULL);
	a = rvn_rt_sel_min(rt);
	para = &rt->doc.paras[a.para];
	if (rvn_rt_has_selection(rt)) {
		int start, run = para->length ? rvn_rt_para_run_at(para, a.offset, &start) : -1;
		url = run >= 0 ? para->runs[run].link : NULL;
	} else if (!rvn_rt_link_extent(rt, a, &from, &to, &url)) {
		url = NULL;
	}
	if (!url) return NULL;
	rt->scratch.length = 0;
	rvn_text_add(&rt->scratch, url);
	return rt->scratch.data;
}

void Rrich_text_editor_edit_link(Rid id) {
	RVN_RICH_VOID(rt, id);
	rvn_rt_link_open(rt);
}

int Rrich_text_editor_undo(Rid id) {
	int done;
	RVN_RICH(rt, id, R_FALSE);
	done = rvn_rt_undo(rt);
	rvn_rt_tell(widget);
	return done;
}

int Rrich_text_editor_redo(Rid id) {
	int done;
	RVN_RICH(rt, id, R_FALSE);
	done = rvn_rt_redo(rt);
	rvn_rt_tell(widget);
	return done;
}

int Rrich_text_editor_can_undo(Rid id) {
	RVN_RICH(rt, id, R_FALSE);
	return rt->undo_count > 0 && !rt->read_only;
}

int Rrich_text_editor_can_redo(Rid id) {
	RVN_RICH(rt, id, R_FALSE);
	return rt->redo_count > 0 && !rt->read_only;
}

void Rrich_text_editor_clear_history(Rid id) {
	RVN_RICH_VOID(rt, id);
	rvn_rt_history_clear(rt->undo, &rt->undo_count);
	rvn_rt_history_clear(rt->redo, &rt->redo_count);
}

void Rrich_text_editor_set_read_only(Rid id, int read_only) {
	RVN_RICH_VOID(rt, id);
	rt->read_only = read_only != 0;
	if (rt->read_only) rvn_rt_link_close(rt);
	rvn_widget_invalidate_paint(widget);
}

int Rrich_text_editor_is_read_only(Rid id) {
	RVN_RICH(rt, id, R_FALSE);
	return rt->read_only;
}

void Rrich_text_editor_set_placeholder(Rid id, const char* text) {
	RVN_RICH_VOID(rt, id);
	rvn_str_set(&rt->placeholder, text && text[0] ? text : NULL);
	rvn_widget_invalidate_paint(widget);
}

void Rrich_text_editor_set_toolbar(Rid id, int visible) {
	RVN_RICH_VOID(rt, id);
	rt->toolbar = visible != 0;
	rt->hot_tool = rt->pressed_tool = -1;
	rvn_widget_invalidate_layout(widget);
	rvn_widget_invalidate_paint(widget);
}

int Rrich_text_editor_get_toolbar(Rid id) {
	RVN_RICH(rt, id, R_FALSE);
	return rt->toolbar;
}

void Rrich_text_editor_set_markdown_shortcuts(Rid id, int enabled) {
	RVN_RICH_VOID(rt, id);
	rt->shortcuts = enabled != 0;
}

void Rrich_text_editor_set_spell_check(Rid id, int enabled) {
	RVN_RICH_VOID(rt, id);
	rt->spelling.enabled = enabled != 0;
	rvn_widget_invalidate_paint(widget);
}

void Rrich_text_editor_set_spell_language(Rid id, const char* language) {
	RVN_RICH_VOID(rt, id);
	if (rvn_spelling_set_language(&rt->spelling, language)) rvn_widget_invalidate_paint(widget);
}

void Rrich_text_editor_set_font(Rid id, Rid font, float size) {
	RVN_RICH_VOID(rt, id);
	rt->font = font;
	rt->text_size = rvn_maxf(0.0f, size);
	rvn_widget_invalidate_layout(widget);
	rvn_widget_invalidate_paint(widget);
}

void Rrich_text_editor_scroll_to_caret(Rid id) {
	rvn_rt_metrics_t m;
	RVN_RICH_VOID(rt, id);
	rt->reveal = R_TRUE;
	rvn_rt_refresh(rt, &m);
	rvn_widget_invalidate_paint(widget);
}

void Rrich_text_editor_set_changed_callback(Rid id, RRichTextChangedCallback callback, void* user_data) {
	RVN_RICH_VOID(rt, id);
	rt->callback = callback;
	rt->user_data = user_data;
}

void Rrich_text_editor_set_selection_callback(Rid id, RRichTextChangedCallback callback, void* user_data) {
	RVN_RICH_VOID(rt, id);
	rt->selection_callback = callback;
	rt->selection_user_data = user_data;
	rt->told = rvn_rt_sel(rt);
}

void Rrich_text_editor_set_link_callback(Rid id, RRichTextLinkCallback callback, void* user_data) {
	RVN_RICH_VOID(rt, id);
	rt->link_callback = callback;
	rt->link_user_data = user_data;
}
