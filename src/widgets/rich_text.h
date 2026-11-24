/**
 * Raven - widgets/rich_text.h
 * The document of a RichTextEditor (widgets/rich_text_doc.c) and what the
 * widget (widgets/rich_text.c) keeps of its layout.
 *
 * A document is an array of paragraphs; a paragraph is one UTF-8 buffer and
 * the runs of its text, each run a length with one look (bold, italic,
 * underline, struck, code, a link). The runs always cover the text exactly,
 * never hold nothing, and two runs side by side never look the same (they
 * are merged), so comparing two paragraphs is comparing their runs.
 *
 * Why not a piece table: the text of notes, mails and messages is made of
 * paragraphs of a few hundred bytes. Typing, erasing and styling touch one
 * of them (a memmove of a few hundred bytes and a few runs); Enter and
 * pasting insert paragraphs into the array (a memmove of pointers). The
 * layout and the spelling are kept per paragraph and made again only for
 * the ones that changed, and the history saves whole paragraphs (copies of
 * the ones an edit touches), which is simple and always right.
 */

#ifndef RVN_RICH_TEXT_H
#define RVN_RICH_TEXT_H

#include "common.h"

/* The looks of a run: the values of RRichTextStyle. */
#define RVN_RT_BOLD      (1u << 0)
#define RVN_RT_ITALIC    (1u << 1)
#define RVN_RT_UNDERLINE (1u << 2)
#define RVN_RT_STRIKE    (1u << 3)
#define RVN_RT_CODE      (1u << 4)
/** Only in what is asked: the run has a link (its address is in the run). */
#define RVN_RT_LINK      (1u << 5)
#define RVN_RT_STYLES    (RVN_RT_BOLD | RVN_RT_ITALIC | RVN_RT_UNDERLINE | RVN_RT_STRIKE | RVN_RT_CODE)

/** The kinds of paragraphs, in the order of RRichTextBlock. */
typedef enum rvn_rt_kind_t {
	RVN_RT_PARAGRAPH,
	RVN_RT_HEADING_1,
	RVN_RT_HEADING_2,
	RVN_RT_HEADING_3,
	RVN_RT_BULLET,
	RVN_RT_NUMBERED,
	RVN_RT_QUOTE,
	RVN_RT_CODE_BLOCK,
	RVN_RT_KIND_COUNT
} rvn_rt_kind_t;

#define RVN_RT_MAX_INDENT 7

typedef struct rvn_rt_run_t {
	int length;
	uint32_t style;
	/** The address of a link, owned, or NULL. */
	char* link;
} rvn_rt_run_t;

/** A piece of a line laid out with one font: bytes start .. start + length. */
typedef struct rvn_rt_frag_t {
	float x, w;
	/** Its top in the paragraph. */
	float y;
	int start, length;
	int run;
	int line;
	Rid font;
	float size;
	/** Height of its font (the tile of inline code), from the top of its box to its baseline. */
	float height;
	float base;
} rvn_rt_frag_t;

typedef struct rvn_rt_line_t {
	/** Top and height in the paragraph. */
	float y, h;
	/** Its bytes; the last line ends at the end of the paragraph. */
	int start, end;
	int first_frag, frag_count;
	/** Where the text starts across (no fragment: an empty line). */
	float x;
} rvn_rt_line_t;

typedef struct rvn_rt_para_t {
	char* text;
	int length, cap;
	rvn_rt_run_t* runs;
	uint32_t run_count, run_cap;
	uint8_t kind;
	/** Lists: how deep the item is (0 .. RVN_RT_MAX_INDENT). */
	uint8_t indent;

	/* layout (widgets/rich_text.c): made again when `laid` is false or the width or the look changed */
	int laid;
	float laid_width;
	uint32_t laid_stamp;
	float height;
	rvn_rt_line_t* lines;
	uint32_t line_count, line_cap;
	rvn_rt_frag_t* frags;
	uint32_t frag_count, frag_cap;
	/** Its misspelled words. */
	rvn_spell_line_t spell;
} rvn_rt_para_t;

typedef struct rvn_rt_doc_t {
	rvn_rt_para_t* paras;
	uint32_t count, cap;
} rvn_rt_doc_t;

/** A place in the document: a paragraph and a byte offset in its text. */
typedef struct rvn_rt_pos_t {
	int para;
	int offset;
} rvn_rt_pos_t;

/* ---- paragraphs ---- */

void rvn_rt_para_free(rvn_rt_para_t* para);
/** A deep copy, without the layout. */
void rvn_rt_para_copy(rvn_rt_para_t* out, const rvn_rt_para_t* para);
/** The layout must be made again. */
void rvn_rt_para_touch(rvn_rt_para_t* para);
/** Inserts text with one look at `offset`. */
void rvn_rt_para_insert(rvn_rt_para_t* para, int offset, const char* text, int length, uint32_t style, const char* link);
/** Removes bytes from .. to. */
void rvn_rt_para_delete(rvn_rt_para_t* para, int from, int to);
/** Appends bytes from .. to of `source` with their looks. */
void rvn_rt_para_append(rvn_rt_para_t* para, const rvn_rt_para_t* source, int from, int to);
/** Adds (`set`) and removes (`clear`) looks from .. to; `link`: 0 keeps the links, 1 sets `url` (NULL removes). */
void rvn_rt_para_style(rvn_rt_para_t* para, int from, int to, uint32_t set, uint32_t clear, int link, const char* url);
/** The run holding the byte at `offset` (the last one at the end), -1 when there is none; *run_start receives where it starts. */
int  rvn_rt_para_run_at(const rvn_rt_para_t* para, int offset, int* run_start);
/** Merges the runs that look the same, drops the empty ones. */
void rvn_rt_para_normalize(rvn_rt_para_t* para);
int  rvn_rt_kind_is_list(int kind);

/* ---- documents ---- */

void rvn_rt_doc_init(rvn_rt_doc_t* doc);
void rvn_rt_doc_free(rvn_rt_doc_t* doc);
/** Makes room for `count` paragraphs at `index` (zeroed). */
void rvn_rt_doc_insert(rvn_rt_doc_t* doc, uint32_t index, uint32_t count);
/** Frees and removes `count` paragraphs at `index`. */
void rvn_rt_doc_remove(rvn_rt_doc_t* doc, uint32_t index, uint32_t count);
/** Splits paragraph `pos.para` at `pos.offset`: the second part becomes the next paragraph, of the same kind. */
void rvn_rt_doc_split(rvn_rt_doc_t* doc, rvn_rt_pos_t pos);
/** Removes from a to b (a before b); the paragraphs between are joined. */
void rvn_rt_doc_delete(rvn_rt_doc_t* doc, rvn_rt_pos_t a, rvn_rt_pos_t b);
/** A copy of what lies between a and b, as a document of its own. */
void rvn_rt_doc_slice(const rvn_rt_doc_t* doc, rvn_rt_pos_t a, rvn_rt_pos_t b, rvn_rt_doc_t* out);
/**
 * Inserts a document at `pos` (`piece` is emptied): its first paragraph
 * goes into the one at `pos`, its last one takes the rest of that
 * paragraph. Returns where it ends.
 */
rvn_rt_pos_t rvn_rt_doc_paste(rvn_rt_doc_t* doc, rvn_rt_pos_t pos, rvn_rt_doc_t* piece);
int  rvn_rt_pos_cmp(rvn_rt_pos_t a, rvn_rt_pos_t b);

/* ---- formats ---- */

/** Reads Markdown (CommonMark: what the document can hold) into `doc` (emptied first). */
void rvn_rt_from_markdown(rvn_rt_doc_t* doc, const char* markdown);
void rvn_rt_to_markdown(const rvn_rt_doc_t* doc, rvn_text_t* out);
/** Reads HTML, keeping only what the document can hold (no attribute but href, no script, no style...). */
void rvn_rt_from_html(rvn_rt_doc_t* doc, const char* html);
void rvn_rt_to_html(const rvn_rt_doc_t* doc, rvn_text_t* out);
/** Plain text: a line per paragraph. */
void rvn_rt_from_text(rvn_rt_doc_t* doc, const char* text);
void rvn_rt_to_text(const rvn_rt_doc_t* doc, rvn_text_t* out);

#endif
