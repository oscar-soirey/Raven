/**
 * Raven - core/diff.h
 * Differences between two texts (core/diff.c): line by line, then word by
 * word inside the lines that changed; unified diffs written and read.
 * Nothing here knows about widgets: the diff view (widgets/diff_view.c) and
 * Rdiff_unified are built on it.
 */

#ifndef RVN_DIFF_H
#define RVN_DIFF_H

#include "base.h"

/* ---------------------------------------------------------------------------
 *  Lines
 * ------------------------------------------------------------------------- */

/** A line of a text: where it starts in the text, its length without the '\n' (a '\r' before it is kept). */
typedef struct rvn_diff_line_t {
	const char* text;
	int length;
	/** R_TRUE for a last line that has no '\n' after it. */
	int no_newline;
} rvn_diff_line_t;

typedef struct rvn_diff_lines_t {
	rvn_diff_line_t* lines;
	int count;
} rvn_diff_lines_t;

/** Cuts a text into lines that point into it ("" has no line, "a\n" one, "a\nb" two). */
void rvn_diff_split(const char* text, size_t length, rvn_diff_lines_t* out);
void rvn_diff_lines_free(rvn_diff_lines_t* lines);

/** R_TRUE when two lines are the same for the flags (RDiffFlags). */
int rvn_diff_line_equal(const rvn_diff_line_t* a, const rvn_diff_line_t* b, uint32_t flags);

/* ---------------------------------------------------------------------------
 *  Line by line
 * ------------------------------------------------------------------------- */

typedef enum rvn_diff_kind_t {
	RVN_DIFF_SAME = 0,
	RVN_DIFF_REMOVED,
	RVN_DIFF_ADDED
} rvn_diff_kind_t;

/**
 * A run of lines. SAME: `count` lines of the left text from `left`, equal to
 * as many of the right text from `right`. REMOVED: lines of the left text
 * from `left` (`right` is where the right text is). ADDED: lines of the right
 * text from `right`. Between two SAME runs the REMOVED run comes first.
 */
typedef struct rvn_diff_run_t {
	uint8_t kind;
	int left;
	int right;
	int count;
} rvn_diff_run_t;

typedef struct rvn_diff_t {
	rvn_diff_run_t* runs;
	uint32_t count;
	uint32_t cap;
	/** R_TRUE when the comparison ran out of time somewhere and kept a longer (still correct) result there. */
	int coarse;
} rvn_diff_t;

/**
 * Compares two lists of lines. Always gives a correct result (applying it
 * to the left lines gives the right ones); the fewest changes unless the
 * work limit was reached (`coarse`).
 */
void rvn_diff_compute(const rvn_diff_line_t* left, int left_count, const rvn_diff_line_t* right, int right_count,
	uint32_t flags, rvn_diff_t* out);
void rvn_diff_free(rvn_diff_t* diff);

/**
 * The same over sequences of integers (classes: equal items have equal
 * numbers). Marks in `changed_a` / `changed_b` (zeroed by the caller) the
 * items that are not in the common subsequence found. `budget`: the work
 * allowed (in steps along diagonals), 0 for the default one. Returns R_TRUE
 * when the work limit was reached somewhere.
 */
int rvn_diff_sequences(const int* a, int n, const int* b, int m, uint8_t* changed_a, uint8_t* changed_b, long budget);

/** The number of changes: runs of REMOVED and / or ADDED lines between two SAME runs. */
int rvn_diff_change_count(const rvn_diff_t* diff);

/* ---------------------------------------------------------------------------
 *  Word by word
 * ------------------------------------------------------------------------- */

typedef struct rvn_diff_range_t {
	int start;
	int end;
} rvn_diff_range_t;

/** Lines longer than this (in bytes) are not compared word by word. */
#define RVN_DIFF_MAX_WORD_LINE 4000

/**
 * The parts of two lines that differ, as byte ranges of each line (words,
 * runs of spaces, punctuation; neighbouring ranges separated by spaces
 * only are joined). Returns R_FALSE, with no range, when the lines have too
 * little in common for marks to help (or are too long): the whole line is
 * the change. At most `max` ranges on each side.
 */
int rvn_diff_words(const char* a, int a_length, const char* b, int b_length, uint32_t flags,
	rvn_diff_range_t* a_ranges, int* a_count, rvn_diff_range_t* b_ranges, int* b_count, int max);

/* ---------------------------------------------------------------------------
 *  Unified diffs
 * ------------------------------------------------------------------------- */

/** Writes the patch of `diff` (from left to right) with `context` lines around the changes. Nothing when there is no change. */
void rvn_diff_write_unified(rvn_text_t* out, const rvn_diff_lines_t* left, const rvn_diff_lines_t* right, const rvn_diff_t* diff,
	const char* left_name, const char* right_name, int context);

/** A line of a patch. */
typedef struct rvn_patch_line_t {
	uint8_t kind;
	uint8_t no_newline;
	/** Numbers of the line on each side, from 1; 0 on a side it is not on. */
	int left;
	int right;
	const char* text;
	int length;
} rvn_patch_line_t;

typedef struct rvn_patch_hunk_t {
	/** Its lines: patch->lines[first .. first + count). */
	uint32_t first;
	uint32_t count;
	int left_start, left_count, right_start, right_count;
	/** "@@ -1,7 +1,8 @@ int main(void)": the line as written, without its end. */
	const char* header;
	int header_length;
} rvn_patch_hunk_t;

typedef struct rvn_patch_file_t {
	/** Owned; "/dev/null" for a file created or deleted. NULL when the patch did not say. */
	char* left_name;
	char* right_name;
	uint32_t first_hunk;
	uint32_t hunk_count;
} rvn_patch_file_t;

/** A patch read from a text; its lines and headers point into that text. */
typedef struct rvn_patch_t {
	rvn_patch_file_t* files;
	uint32_t file_count, file_cap;
	rvn_patch_hunk_t* hunks;
	uint32_t hunk_count, hunk_cap;
	rvn_patch_line_t* lines;
	uint32_t line_count, line_cap;
} rvn_patch_t;

/**
 * Reads a unified diff: `diff --git`, `---` / `+++`, `@@ -a,b +c,d @@`,
 * lines starting with ' ', '-', '+', and "\ No newline at end of file";
 * everything else (index lines, modes, text around the patch) is skipped.
 * Returns R_FALSE when there is no hunk.
 */
int rvn_patch_parse(const char* text, size_t length, rvn_patch_t* out);
void rvn_patch_free(rvn_patch_t* patch);

#endif /* RVN_DIFF_H */
