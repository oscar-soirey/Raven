/**
 * Raven - platform/preedit.h
 * The bookkeeping of the input methods of Linux and the BSDs, apart from
 * the window systems so that it can be tested anywhere:
 *
 *   - Wayland, text-input-unstable-v3: the events of the compositor change a
 *     pending state, applied all at once on `done`; what to delete around the
 *     caret; the part of the text sent as surrounding text;
 *   - X11, XIM with on-the-spot preedit: the composition is edited piece by
 *     piece (characters chg_first .. chg_first + chg_length replaced).
 *
 * Offsets are bytes of UTF-8 text unless noted.
 */

#ifndef RVN_PREEDIT_H
#define RVN_PREEDIT_H

#include "../core/base.h"

/** `offset` clamped to [0, length] and moved back to the first byte of its character. */
int rvn_preedit_boundary(const char* text, int length, int offset);

/** The byte offset of character `index` of `text` (clamped to [0, length]). */
int rvn_preedit_char_offset(const char* text, int length, int index);

/* ---------------------------------------------------------------------------
 *  Wayland: zwp_text_input_v3
 * ------------------------------------------------------------------------- */

typedef struct rvn_ti3_t {
	/* pending: what the events since the last done asked for */
	char* pending_preedit;
	int pending_begin, pending_end;
	char* pending_commit;
	uint32_t pending_before, pending_after;

	/* what the last done applied (valid until the next one) */
	char* preedit;              /* NULL: no composition */
	int preedit_begin, preedit_end;
	char* commit;               /* NULL: nothing to type */
	uint32_t delete_before, delete_after;

	/* the commit requests sent: a done whose serial is this has seen them all */
	uint32_t commits;
} rvn_ti3_t;

/** Frees what it holds; the count of commits is kept (the serials go on for the object). */
void rvn_ti3_clear(rvn_ti3_t* ti);

/* the double-buffered events: they replace their pending value */
void rvn_ti3_preedit_string(rvn_ti3_t* ti, const char* text, int cursor_begin, int cursor_end);
void rvn_ti3_commit_string(rvn_ti3_t* ti, const char* text);
void rvn_ti3_delete_surrounding(rvn_ti3_t* ti, uint32_t before, uint32_t after);

/**
 * done: the pending state becomes the applied one (preedit, commit,
 * delete_before / delete_after) and the pending state is back to its
 * initial value (no composition, nothing to type or delete): what an event
 * did not set again is gone. The caller then applies it in the order of the
 * protocol: the composition removed, the text around the caret deleted, the
 * commit typed, the new composition shown.
 *
 * Returns R_TRUE when `serial` is the number of commit requests sent: the
 * compositor saw the last state of the client, which may answer with its
 * new state. Otherwise the changes are applied all the same.
 */
int rvn_ti3_done(rvn_ti3_t* ti, uint32_t serial);

/**
 * The bytes to delete for delete_surrounding_text: `before` bytes before the
 * selection [start, end) and `after` bytes after it (the selection between
 * them, which the commit replaces anyway). Clamped to the text and to
 * character boundaries. R_FALSE when there is nothing to delete.
 */
int rvn_ti3_delete_range(const char* text, int start, int end, uint32_t before, uint32_t after, int* from, int* to);

/**
 * The part of `text` sent as surrounding text: at most `max_bytes`, on
 * character boundaries, around the selection [start, end) (all of it when it
 * fits). *offset and *length give the part, *cursor (the end of the
 * selection) and *anchor (its start) are offsets in it.
 */
void rvn_ti3_surrounding(const char* text, int start, int end, int max_bytes, int* offset, int* length, int* cursor, int* anchor);

/* ---------------------------------------------------------------------------
 *  X11: XIM on the spot
 * ------------------------------------------------------------------------- */

/**
 * XIMPreeditDrawCallback: replaces characters [first, first + count) of the
 * composition `*text` (UTF-8, NULL for empty) with `insert` (UTF-8, NULL:
 * only deletes). Counts out of range are clamped. *text is reallocated; it
 * becomes NULL when the composition is empty.
 */
void rvn_preedit_replace_chars(char** text, int first, int count, const char* insert);

/**
 * The same edit on one value per character (the feedback of XIM: which
 * characters are the clause being converted): values [first, first + count)
 * of *marks (*total of them) replaced by `insert_count` values of `insert`
 * (NULL: zeros). Counts out of range are clamped.
 */
void rvn_preedit_replace_marks(uint32_t** marks, int* total, int first, int count, const uint32_t* insert, int insert_count);

/**
 * The clause being converted: the first run of characters whose mark has
 * one of the bits of `mask`, as bytes of `text` (the composition the marks
 * belong to). R_FALSE when there is none.
 */
int rvn_preedit_marked_run(const char* text, const uint32_t* marks, int total, uint32_t mask, int* start, int* end);

#endif /* RVN_PREEDIT_H */
