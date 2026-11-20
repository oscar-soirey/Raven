/**
 * Raven - widgets/diff_view.c
 * Diff view: the differences between two texts (compared by core/diff.c),
 * or a patch read as it is, in one column (unified) or side by side (split).
 *
 * The model is a list of entries, one per line shown in unified mode: a
 * line on both sides (its two texts may differ when spaces or case do not
 * count), a removed line, an added line. Each change (the removed and added
 * lines between two kept ones) pairs its removed lines with its added lines
 * in order: the pairs face each other in split mode, and their words are
 * compared when they are first drawn.
 *
 * The rows are made from the entries for the mode, the context and the
 * folds opened: a line, a pair of lines, a fold of unchanged lines, the
 * header of a hunk or of a file of a patch, a note. Every row has the same
 * height, so only the rows on screen are visited to draw, and the row under
 * a point is a division; the rows are made again (a pass over the entries)
 * only when the mode, the context, a fold or a note changes.
 */

#include "common.h"
#include "../core/diff.h"
#include "../xml/xml.h"

#include <limits.h>

#define RVN_DV_TAB_SIZE 4
/* the bar of the current change, at the left of the gutter */
#define RVN_DV_MARK 3.0f
#define RVN_DV_PAD 6.0f
#define RVN_DV_MAX_SPANS 128
#define RVN_DV_MAX_WORDS 64

enum {
	RVN_DV_ROW_LINE,   /* unified: entry a */
	RVN_DV_ROW_PAIR,   /* split: entry a on the left, entry b on the right (-1: none; a == b: a kept line) */
	RVN_DV_ROW_FOLD,   /* b entries hidden from entry a */
	RVN_DV_ROW_HUNK,   /* hunk a */
	RVN_DV_ROW_FILE,   /* file a */
	RVN_DV_ROW_NOTE    /* note a */
};

typedef struct rvn_dv_entry_t {
	uint8_t kind;
	/* the first line of a hunk or of a file: the colors of the code start again there */
	uint8_t starts;
	uint8_t lex_left, lex_right;
	int left, right;
	const char* left_text;
	const char* right_text;
	int left_length, right_length;
	uint32_t file;
	/* the entry facing it in its change, -1 */
	int pair;
	/* its changed words: ranges[words .. words + word_count); -1: not compared yet, -2: none */
	int words;
	int word_count;
} rvn_dv_entry_t;

typedef struct rvn_dv_row_t {
	uint8_t kind;
	int a;
	int b;
} rvn_dv_row_t;

typedef struct rvn_dv_file_t {
	char* left_name;
	char* right_name;
	const rvn_language_t* language;
} rvn_dv_file_t;

typedef struct rvn_dv_hunk_t {
	uint32_t entry;
	const char* header;
	int header_length;
} rvn_dv_hunk_t;

typedef struct rvn_dv_change_t {
	uint32_t first, end;
	int left, left_count, right, right_count;
} rvn_dv_change_t;

typedef struct rvn_dv_note_t {
	int side;
	int line;
	char* text;
} rvn_dv_note_t;

typedef struct rvn_diff_view_t {
	rvn_widget_t base;
	/* what is compared */
	char* left;
	char* right;
	char* patch_text;
	int is_patch;
	char* left_name;
	char* right_name;
	char* language_name;
	uint32_t flags;
	RDiffViewMode mode;
	int context;
	int word_diff;
	int line_numbers;

	/* the model */
	int dirty;
	int rows_dirty;
	rvn_diff_lines_t left_lines, right_lines;
	rvn_patch_t patch;
	rvn_dv_entry_t* entries;
	uint32_t entry_count, entry_cap;
	rvn_dv_file_t* files;
	uint32_t file_count, file_cap;
	rvn_dv_hunk_t* hunks;
	uint32_t hunk_count, hunk_cap;
	rvn_dv_change_t* changes;
	uint32_t change_count, change_cap;
	uint8_t* revealed;
	rvn_diff_range_t* ranges;
	uint32_t range_count, range_cap;
	uint32_t lex_valid;
	uint8_t lex_run_left, lex_run_right;
	uint32_t language_generation;
	int max_columns;

	/* the rows */
	rvn_dv_row_t* rows;
	uint32_t row_count, row_cap;
	rvn_dv_note_t* notes;
	uint32_t note_count, note_cap;

	/* the view */
	rvn_scrollbar_t bar, hbar;
	int current;
	/* a change gone to before the widget had its size: scrolled to when it has it */
	int pending;
	int hot_row;
	int selecting;
	int has_selection;
	int sel_side;
	int anchor_row, anchor_col, caret_row, caret_col;
	rvn_text_t scratch;
	RDiffViewLineCallback callback;
	void* user_data;
} rvn_diff_view_t;

typedef struct rvn_dv_metrics_t {
	const rvn_theme_t* theme;
	Rid font_id;
	rvn_font_t* font;
	float size;
	float row;
	float char_w;
	float tab;
	float header;
	rvn_rect_t body;
	rvn_rect_t vtrack, htrack;
	float number_w;
	float sign_w;
	int panes;
	float pane_w;
	/* the gutter and the text of each pane, from the left of the widget */
	float pane_x[2];
	float text_x[2];
	float text_w;
} rvn_dv_metrics_t;

static void rvn_dv_init(rvn_widget_t* widget) {
	rvn_diff_view_t* dv = (rvn_diff_view_t*)widget;
	dv->mode = R_DIFF_VIEW_UNIFIED;
	dv->context = 3;
	dv->word_diff = R_TRUE;
	dv->line_numbers = R_TRUE;
	dv->current = -1;
	dv->pending = -1;
	dv->hot_row = -1;
	dv->dirty = R_TRUE;
	dv->rows_dirty = R_TRUE;
	widget->cursor = R_CURSOR_IBEAM;
}

static void rvn_dv_free_model(rvn_diff_view_t* dv) {
	uint32_t i;
	rvn_diff_lines_free(&dv->left_lines);
	rvn_diff_lines_free(&dv->right_lines);
	rvn_patch_free(&dv->patch);
	for (i = 0; i < dv->file_count; i++) {
		rvn_free(dv->files[i].left_name);
		rvn_free(dv->files[i].right_name);
	}
	dv->file_count = 0;
	dv->entry_count = 0;
	dv->hunk_count = 0;
	dv->change_count = 0;
	dv->range_count = 0;
	rvn_free(dv->revealed);
	dv->revealed = NULL;
	dv->lex_valid = 0;
}

static void rvn_dv_destroy(rvn_widget_t* widget) {
	rvn_diff_view_t* dv = (rvn_diff_view_t*)widget;
	uint32_t i;
	rvn_dv_free_model(dv);
	rvn_free(dv->entries);
	rvn_free(dv->files);
	rvn_free(dv->hunks);
	rvn_free(dv->changes);
	rvn_free(dv->ranges);
	rvn_free(dv->rows);
	for (i = 0; i < dv->note_count; i++) rvn_free(dv->notes[i].text);
	rvn_free(dv->notes);
	rvn_free(dv->left);
	rvn_free(dv->right);
	rvn_free(dv->patch_text);
	rvn_free(dv->left_name);
	rvn_free(dv->right_name);
	rvn_free(dv->language_name);
	rvn_free(dv->scratch.data);
}

/* ---------------------------------------------------------------------------
 *  The model
 * ------------------------------------------------------------------------- */

/* "a/src/x.c" -> "src/x.c": the prefixes of git say nothing to the reader. */
static const char* rvn_dv_display_name(const char* name) {
	if (!name) return NULL;
	if ((name[0] == 'a' || name[0] == 'b') && name[1] == '/') return name + 2;
	return name;
}

static const rvn_language_t* rvn_dv_guess_language(const rvn_diff_view_t* dv, const char* left_name, const char* right_name) {
	const char* names[2];
	int i;
	if (dv->language_name) return rvn_language_find(dv->language_name);
	names[0] = right_name;
	names[1] = left_name;
	for (i = 0; i < 2; i++) {
		const char* name = rvn_dv_display_name(names[i]);
		const char* dot;
		const rvn_language_t* language;
		if (!name || strcmp(name, "/dev/null") == 0) continue;
		dot = strrchr(name, '.');
		if (!dot || !dot[1] || strchr(dot, '/') || strchr(dot, '\\')) continue;
		language = rvn_language_find(dot + 1);
		if (language) return language;
	}
	return NULL;
}

static rvn_dv_entry_t* rvn_dv_add_entry(rvn_diff_view_t* dv, uint8_t kind, uint32_t file) {
	rvn_dv_entry_t* e;
	RVN_ARRAY_RESERVE(dv->entries, dv->entry_cap, dv->entry_count + 1);
	e = &dv->entries[dv->entry_count++];
	memset(e, 0, sizeof(*e));
	e->kind = kind;
	e->file = file;
	e->pair = -1;
	e->words = -1;
	return e;
}

/* Display columns of a line: characters, tabs to the next stop. */
static int rvn_dv_columns(const char* text, int length) {
	int i, columns = 0;
	for (i = 0; i < length; i++) {
		unsigned char c = (unsigned char)text[i];
		if (c == '\t') columns = (columns / RVN_DV_TAB_SIZE + 1) * RVN_DV_TAB_SIZE;
		else if ((c & 0xC0u) != 0x80u && c != '\r') columns++;
	}
	return columns;
}

/* The changes, and the pairs of lines inside them. */
static void rvn_dv_find_changes(rvn_diff_view_t* dv) {
	uint32_t i = 0;
	int last_left = 0, last_right = 0;
	while (i < dv->entry_count) {
		rvn_dv_entry_t* e = &dv->entries[i];
		rvn_dv_change_t* change;
		uint32_t j, removed = 0, added = 0, k;
		if (e->starts) last_left = last_right = 0;
		if (e->kind == RVN_DIFF_SAME) {
			last_left = e->left;
			last_right = e->right;
			i++;
			continue;
		}
		RVN_ARRAY_RESERVE(dv->changes, dv->change_cap, dv->change_count + 1);
		change = &dv->changes[dv->change_count++];
		memset(change, 0, sizeof(*change));
		change->first = i;
		change->left = last_left;
		change->right = last_right;
		for (j = i; j < dv->entry_count && dv->entries[j].kind != RVN_DIFF_SAME && (j == i || !dv->entries[j].starts); j++) {
			rvn_dv_entry_t* x = &dv->entries[j];
			if (x->kind == RVN_DIFF_REMOVED) {
				if (change->left_count++ == 0) change->left = x->left;
				last_left = x->left;
			} else {
				if (change->right_count++ == 0) change->right = x->right;
				last_right = x->right;
			}
		}
		change->end = j;
		/* the k-th removed line faces the k-th added line */
		for (k = i; k < j; k++) {
			if (dv->entries[k].kind != RVN_DIFF_REMOVED) continue;
			{
				uint32_t want = removed++, seen = 0, m;
				for (m = i; m < j; m++) {
					if (dv->entries[m].kind != RVN_DIFF_ADDED) continue;
					if (seen++ == want) {
						dv->entries[k].pair = (int)m;
						dv->entries[m].pair = (int)k;
						break;
					}
				}
			}
		}
		(void)added;
		i = j;
	}
}

static void rvn_dv_build(rvn_diff_view_t* dv) {
	uint32_t i;
	rvn_dv_free_model(dv);
	dv->max_columns = 0;
	if (dv->is_patch) {
		uint32_t f, h;
		if (dv->patch_text) rvn_patch_parse(dv->patch_text, strlen(dv->patch_text), &dv->patch);
		for (f = 0; f < dv->patch.file_count; f++) {
			const rvn_patch_file_t* pf = &dv->patch.files[f];
			rvn_dv_file_t* file;
			RVN_ARRAY_RESERVE(dv->files, dv->file_cap, dv->file_count + 1);
			file = &dv->files[dv->file_count++];
			file->left_name = rvn_strdup(pf->left_name);
			file->right_name = rvn_strdup(pf->right_name);
			file->language = rvn_dv_guess_language(dv, pf->left_name, pf->right_name);
			for (h = pf->first_hunk; h < pf->first_hunk + pf->hunk_count; h++) {
				const rvn_patch_hunk_t* ph = &dv->patch.hunks[h];
				rvn_dv_hunk_t* hunk;
				uint32_t l;
				RVN_ARRAY_RESERVE(dv->hunks, dv->hunk_cap, dv->hunk_count + 1);
				hunk = &dv->hunks[dv->hunk_count++];
				hunk->entry = dv->entry_count;
				hunk->header = ph->header;
				hunk->header_length = ph->header_length;
				for (l = ph->first; l < ph->first + ph->count; l++) {
					const rvn_patch_line_t* pl = &dv->patch.lines[l];
					rvn_dv_entry_t* e = rvn_dv_add_entry(dv, pl->kind, dv->file_count - 1);
					e->starts = (uint8_t)(l == ph->first);
					e->left = pl->left;
					e->right = pl->right;
					if (pl->kind != RVN_DIFF_ADDED) {
						e->left_text = pl->text;
						e->left_length = pl->length;
					}
					if (pl->kind != RVN_DIFF_REMOVED) {
						e->right_text = pl->text;
						e->right_length = pl->length;
					}
				}
			}
		}
	} else {
		rvn_diff_t diff;
		rvn_dv_file_t* file;
		RVN_ARRAY_RESERVE(dv->files, dv->file_cap, 1);
		file = &dv->files[dv->file_count++];
		file->left_name = rvn_strdup(dv->left_name);
		file->right_name = rvn_strdup(dv->right_name);
		file->language = rvn_dv_guess_language(dv, dv->left_name, dv->right_name);
		rvn_diff_split(dv->left, dv->left ? strlen(dv->left) : 0, &dv->left_lines);
		rvn_diff_split(dv->right, dv->right ? strlen(dv->right) : 0, &dv->right_lines);
		rvn_diff_compute(dv->left_lines.lines, dv->left_lines.count, dv->right_lines.lines, dv->right_lines.count, dv->flags, &diff);
		RVN_ARRAY_RESERVE(dv->entries, dv->entry_cap, (uint32_t)(dv->left_lines.count + dv->right_lines.count) + 1u);
		for (i = 0; i < diff.count; i++) {
			const rvn_diff_run_t* run = &diff.runs[i];
			int n;
			for (n = 0; n < run->count; n++) {
				rvn_dv_entry_t* e = rvn_dv_add_entry(dv, run->kind, 0);
				if (run->kind != RVN_DIFF_ADDED) {
					const rvn_diff_line_t* line = &dv->left_lines.lines[run->left + n];
					e->left = run->left + n + 1;
					e->left_text = line->text;
					e->left_length = line->length;
				}
				if (run->kind != RVN_DIFF_REMOVED) {
					const rvn_diff_line_t* line = &dv->right_lines.lines[run->right + n];
					e->right = run->right + n + 1;
					e->right_text = line->text;
					e->right_length = line->length;
				}
			}
		}
		if (dv->entry_count > 0) dv->entries[0].starts = R_TRUE;
		rvn_diff_free(&diff);
	}
	for (i = 0; i < dv->entry_count; i++) {
		const rvn_dv_entry_t* e = &dv->entries[i];
		int columns = rvn_dv_columns(e->left_text, e->left_length);
		if (e->right_text) columns = rvn_maxi(columns, rvn_dv_columns(e->right_text, e->right_length));
		if (columns > dv->max_columns) dv->max_columns = columns;
	}
	rvn_dv_find_changes(dv);
	dv->revealed = (uint8_t*)rvn_calloc((size_t)dv->entry_count + 1u);
	dv->language_generation = rvn_language_generation();
	dv->dirty = R_FALSE;
	dv->rows_dirty = R_TRUE;
	if (dv->current >= (int)dv->change_count) dv->current = -1;
}

/* The entry of a line of a side, -1 when no line has that number. */
static int rvn_dv_entry_of_line(const rvn_diff_view_t* dv, int side, int line) {
	uint32_t i;
	for (i = 0; i < dv->entry_count; i++) {
		const rvn_dv_entry_t* e = &dv->entries[i];
		if (side == R_DIFF_LEFT ? (e->left == line && e->left_text) : (e->right == line && e->right_text)) return (int)i;
	}
	return -1;
}

static void rvn_dv_push_row(rvn_diff_view_t* dv, uint8_t kind, int a, int b) {
	RVN_ARRAY_RESERVE(dv->rows, dv->row_cap, dv->row_count + 1);
	dv->rows[dv->row_count].kind = kind;
	dv->rows[dv->row_count].a = a;
	dv->rows[dv->row_count].b = b;
	dv->row_count++;
}

static void rvn_dv_push_notes(rvn_diff_view_t* dv, const int* note_entry, int entry) {
	uint32_t n;
	if (!note_entry || entry < 0) return;
	for (n = 0; n < dv->note_count; n++) if (note_entry[n] == entry) rvn_dv_push_row(dv, RVN_DV_ROW_NOTE, (int)n, 0);
}

static void rvn_dv_build_rows(rvn_diff_view_t* dv) {
	uint8_t* shown;
	int* note_entry = NULL;
	uint32_t i, hunk = 0, file = (uint32_t)-1;
	int distance;

	dv->row_count = 0;
	dv->rows_dirty = R_FALSE;
	dv->hot_row = -1;
	dv->has_selection = R_FALSE;
	dv->selecting = R_FALSE;
	if (dv->entry_count == 0) return;

	if (dv->note_count > 0) {
		note_entry = (int*)rvn_alloc(dv->note_count * sizeof(int));
		for (i = 0; i < dv->note_count; i++) note_entry[i] = rvn_dv_entry_of_line(dv, dv->notes[i].side, dv->notes[i].line);
	}

	/* which lines are shown: the changes, the lines near them, the folds opened, the lines with a note */
	shown = (uint8_t*)rvn_alloc(dv->entry_count);
	if (dv->is_patch || dv->context < 0) {
		memset(shown, 1, dv->entry_count);
	} else {
		distance = INT_MAX / 2;
		for (i = 0; i < dv->entry_count; i++) {
			if (dv->entries[i].starts && i > 0) distance = INT_MAX / 2;
			if (dv->entries[i].kind != RVN_DIFF_SAME) distance = 0;
			else if (distance < INT_MAX / 2) distance++;
			shown[i] = (uint8_t)(distance <= dv->context || dv->revealed[i]);
		}
		distance = INT_MAX / 2;
		for (i = dv->entry_count; i-- > 0;) {
			if (dv->entries[i].kind != RVN_DIFF_SAME) distance = 0;
			else if (distance < INT_MAX / 2) distance++;
			if (distance <= dv->context) shown[i] = 1;
			if (dv->entries[i].starts) distance = INT_MAX / 2;
		}
		for (i = 0; i < dv->note_count; i++) if (note_entry[i] >= 0) shown[note_entry[i]] = 1;
	}

	for (i = 0; i < dv->entry_count;) {
		rvn_dv_entry_t* e = &dv->entries[i];
		if (dv->is_patch && e->file != file) {
			file = e->file;
			rvn_dv_push_row(dv, RVN_DV_ROW_FILE, (int)file, 0);
		}
		if (dv->is_patch && hunk < dv->hunk_count && dv->hunks[hunk].entry == i) {
			rvn_dv_push_row(dv, RVN_DV_ROW_HUNK, (int)hunk, 0);
			hunk++;
		}
		if (!shown[i]) {
			uint32_t j = i;
			while (j < dv->entry_count && !shown[j] && (j == i || !dv->entries[j].starts)) j++;
			/* one line is shown rather than folded: the fold would take its row anyway */
			if (j - i >= 2) {
				rvn_dv_push_row(dv, RVN_DV_ROW_FOLD, (int)i, (int)(j - i));
				i = j;
				continue;
			}
		}
		if (dv->mode == R_DIFF_VIEW_UNIFIED || e->kind == RVN_DIFF_SAME) {
			if (dv->mode == R_DIFF_VIEW_UNIFIED) rvn_dv_push_row(dv, RVN_DV_ROW_LINE, (int)i, 0);
			else rvn_dv_push_row(dv, RVN_DV_ROW_PAIR, (int)i, (int)i);
			rvn_dv_push_notes(dv, note_entry, (int)i);
			i++;
			continue;
		}
		{
			/* split: the lines of a change face each other, removed on the left, added on the right */
			uint32_t end = i, r = i, a = i;
			while (end < dv->entry_count && dv->entries[end].kind != RVN_DIFF_SAME && (end == i || !dv->entries[end].starts)) end++;
			for (;;) {
				int left = -1, right = -1;
				while (r < end && dv->entries[r].kind != RVN_DIFF_REMOVED) r++;
				while (a < end && dv->entries[a].kind != RVN_DIFF_ADDED) a++;
				if (r >= end && a >= end) break;
				if (r < end) left = (int)r++;
				if (a < end) right = (int)a++;
				rvn_dv_push_row(dv, RVN_DV_ROW_PAIR, left, right);
				rvn_dv_push_notes(dv, note_entry, left);
				rvn_dv_push_notes(dv, note_entry, right);
			}
			i = end;
		}
	}
	rvn_free(shown);
	rvn_free(note_entry);
}

static void rvn_dv_ensure(rvn_diff_view_t* dv) {
	if (!dv->dirty && dv->language_generation != rvn_language_generation()) {
		uint32_t f;
		for (f = 0; f < dv->file_count; f++) dv->files[f].language = rvn_dv_guess_language(dv, dv->files[f].left_name, dv->files[f].right_name);
		dv->language_generation = rvn_language_generation();
		dv->lex_valid = 0;
	}
	if (dv->dirty) rvn_dv_build(dv);
	if (dv->rows_dirty) rvn_dv_build_rows(dv);
}

static void rvn_dv_changed(rvn_widget_t* widget, int model) {
	rvn_diff_view_t* dv = (rvn_diff_view_t*)widget;
	if (model) dv->dirty = R_TRUE;
	dv->rows_dirty = R_TRUE;
	rvn_widget_invalidate_layout(widget);
}

/* The colors of the code: the state each line of each side starts in, up to `entry`. */
static void rvn_dv_lex_until(rvn_diff_view_t* dv, uint32_t entry) {
	while (dv->lex_valid <= entry && dv->lex_valid < dv->entry_count) {
		rvn_dv_entry_t* e = &dv->entries[dv->lex_valid];
		const rvn_language_t* language = dv->files[e->file].language;
		if (e->starts || dv->lex_valid == 0) dv->lex_run_left = dv->lex_run_right = 0;
		e->lex_left = dv->lex_run_left;
		e->lex_right = dv->lex_run_right;
		if (language) {
			if (e->left_text) dv->lex_run_left = rvn_syntax_line(language, e->left_text, e->left_length, dv->lex_run_left, NULL, 0, NULL);
			if (e->right_text) dv->lex_run_right = rvn_syntax_line(language, e->right_text, e->right_length, dv->lex_run_right, NULL, 0, NULL);
		}
		dv->lex_valid++;
	}
}

/* The changed words of an entry and of the one facing it, compared on first use. */
static void rvn_dv_words(rvn_diff_view_t* dv, int entry) {
	rvn_dv_entry_t* e = &dv->entries[entry];
	rvn_dv_entry_t* removed;
	rvn_dv_entry_t* added;
	rvn_diff_range_t a[RVN_DV_MAX_WORDS], b[RVN_DV_MAX_WORDS];
	int na = 0, nb = 0;
	if (e->words != -1) return;
	if (e->pair < 0 || !dv->word_diff) {
		e->words = -2;
		return;
	}
	removed = e->kind == RVN_DIFF_REMOVED ? e : &dv->entries[e->pair];
	added = e->kind == RVN_DIFF_ADDED ? e : &dv->entries[e->pair];
	removed->words = added->words = -2;
	if (!rvn_diff_words(removed->left_text, removed->left_length, added->right_text, added->right_length, dv->flags,
		a, &na, b, &nb, RVN_DV_MAX_WORDS)) return;
	RVN_ARRAY_RESERVE(dv->ranges, dv->range_cap, dv->range_count + (uint32_t)(na + nb));
	removed->words = (int)dv->range_count;
	removed->word_count = na;
	memcpy(dv->ranges + dv->range_count, a, (size_t)na * sizeof(rvn_diff_range_t));
	dv->range_count += (uint32_t)na;
	added->words = (int)dv->range_count;
	added->word_count = nb;
	memcpy(dv->ranges + dv->range_count, b, (size_t)nb * sizeof(rvn_diff_range_t));
	dv->range_count += (uint32_t)nb;
}

/* ---------------------------------------------------------------------------
 *  Geometry
 * ------------------------------------------------------------------------- */

static int rvn_dv_has_header(const rvn_diff_view_t* dv) {
	return !dv->is_patch && ((dv->left_name && dv->left_name[0]) || (dv->right_name && dv->right_name[0]));
}

static int rvn_dv_digits(int value) {
	int digits = 1;
	while (value >= 10) {
		value /= 10;
		digits++;
	}
	return digits;
}

static void rvn_dv_metrics(rvn_diff_view_t* dv, rvn_dv_metrics_t* m) {
	rvn_widget_t* widget = &dv->base;
	rvn_font_metrics_t fm;
	float bar = rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE);
	float inner_w = rvn_maxf(0.0f, widget->rect.w - 2.0f), inner_h, content, wide;
	int vertical, horizontal, largest = 1, p;
	uint32_t i;

	m->theme = rvn_widget_theme(widget);
	m->font_id = g_rvn.mono_font;
	m->font = rvn_font_resolve(m->font_id);
	m->size = rvn_maxf(8.0f, rvn_widget_font_size(widget, 0.0f) - 1.0f);
	rvn_font_metrics(m->font, m->size, &fm);
	m->row = ceilf(fm.line_height + 4.0f);
	m->char_w = rvn_text_width(m->font, m->size, "0", 1);
	if (m->char_w <= 0.0f) m->char_w = m->size * 0.6f;
	m->tab = m->char_w * (float)RVN_DV_TAB_SIZE;
	m->header = rvn_dv_has_header(dv) ? m->row + 6.0f : 0.0f;
	inner_h = rvn_maxf(0.0f, widget->rect.h - 2.0f - m->header);

	for (i = 0; i < dv->entry_count; i++) {
		if (dv->entries[i].left > largest) largest = dv->entries[i].left;
		if (dv->entries[i].right > largest) largest = dv->entries[i].right;
	}
	m->number_w = dv->line_numbers ? ceilf(m->char_w * (float)rvn_maxi(rvn_dv_digits(largest), 2) + RVN_DV_PAD * 1.5f) : 0.0f;
	m->sign_w = ceilf(m->char_w * 2.0f);
	m->panes = dv->mode == R_DIFF_VIEW_SPLIT ? 2 : 1;

	content = m->row * (float)dv->row_count;
	wide = (float)dv->max_columns * m->char_w + RVN_DV_PAD * 2.0f;
	vertical = content > inner_h + 0.5f;
	for (p = 0; p < 2; p++) {
		float body_w = rvn_maxf(0.0f, inner_w - (vertical ? bar : 0.0f));
		float pane_w = floorf(body_w / (float)m->panes);
		float gutter = RVN_DV_MARK + (m->panes == 1 ? 2.0f * m->number_w : m->number_w) + m->sign_w;
		m->pane_w = pane_w;
		m->text_w = rvn_maxf(0.0f, pane_w - gutter);
		horizontal = wide > m->text_w + 0.5f;
		if (horizontal && !vertical && content > inner_h - bar + 0.5f) {
			vertical = R_TRUE;
			continue;
		}
		break;
	}
	m->body = rvn_rect(1.0f, 1.0f + m->header, rvn_maxf(0.0f, inner_w - (vertical ? bar : 0.0f)),
		rvn_maxf(0.0f, inner_h - (horizontal ? bar : 0.0f)));
	m->vtrack = rvn_rect(widget->rect.w - 1.0f - bar, m->body.y, vertical ? bar : 0.0f, m->body.h);
	m->htrack = rvn_rect(1.0f, m->body.y + m->body.h, m->body.w, horizontal ? bar : 0.0f);
	for (p = 0; p < m->panes; p++) {
		float gutter = RVN_DV_MARK + (m->panes == 1 ? 2.0f * m->number_w : m->number_w) + m->sign_w;
		m->pane_x[p] = m->body.x + m->pane_w * (float)p;
		m->text_x[p] = m->pane_x[p] + gutter;
	}
	if (m->panes == 1) {
		m->pane_x[1] = m->pane_x[0];
		m->text_x[1] = m->text_x[0];
		m->pane_w = m->body.w;
	}
	dv->bar.content = content;
	dv->bar.viewport = m->body.h;
	dv->hbar.content = wide;
	dv->hbar.viewport = m->text_w;
}

static rvn_vec2_t rvn_dv_measure(rvn_widget_t* widget) {
	rvn_dv_ensure((rvn_diff_view_t*)widget);
	/* usually given its size by a fill slot */
	return rvn_vec2(480.0f, 320.0f);
}

static void rvn_dv_go_to(rvn_diff_view_t* dv, int change);

static void rvn_dv_arrange(rvn_widget_t* widget) {
	rvn_diff_view_t* dv = (rvn_diff_view_t*)widget;
	rvn_dv_metrics_t m;
	rvn_dv_ensure(dv);
	rvn_dv_metrics(dv, &m);
	if (dv->pending >= 0 && m.body.h > 0.0f) {
		int change = dv->pending;
		dv->pending = -1;
		rvn_dv_go_to(dv, change);
	}
	rvn_scrollbar_clamp(&dv->bar);
	rvn_scrollbar_clamp(&dv->hbar);
}

static int rvn_dv_row_at(rvn_diff_view_t* dv, const rvn_dv_metrics_t* m, float x, float y) {
	int row;
	if (!rvn_rect_contains(m->body, x, y)) return -1;
	row = (int)floorf((y - m->body.y + dv->bar.offset) / m->row);
	return row >= 0 && row < (int)dv->row_count ? row : -1;
}

/* ---------------------------------------------------------------------------
 *  Lines of the rows
 * ------------------------------------------------------------------------- */

/* The entry a row shows on a side (unified: its one line), -1 for none. */
static int rvn_dv_row_entry(const rvn_diff_view_t* dv, int row, int side) {
	const rvn_dv_row_t* r;
	if (row < 0 || row >= (int)dv->row_count) return -1;
	r = &dv->rows[row];
	if (r->kind == RVN_DV_ROW_LINE) return r->a;
	if (r->kind == RVN_DV_ROW_PAIR) return side == R_DIFF_LEFT ? r->a : r->b;
	return -1;
}

/* The text an entry shows on a side (unified: the side it is on, the right one for a kept line); its '\r' left out. */
static const char* rvn_dv_entry_text(const rvn_diff_view_t* dv, int entry, int side, int unified, int* length) {
	const rvn_dv_entry_t* e = &dv->entries[entry];
	const char* text;
	int n;
	if (unified) side = e->kind == RVN_DIFF_REMOVED ? R_DIFF_LEFT : R_DIFF_RIGHT;
	text = side == R_DIFF_LEFT ? e->left_text : e->right_text;
	n = side == R_DIFF_LEFT ? e->left_length : e->right_length;
	if (!text) {
		*length = 0;
		return NULL;
	}
	if (n > 0 && text[n - 1] == '\r') n--;
	*length = n;
	return text;
}

/* Horizontal position after text[0..to), tabs to the next stop. */
static float rvn_dv_advance(const rvn_dv_metrics_t* m, const char* text, int to) {
	float x = 0.0f;
	int i, run = 0;
	for (i = 0; i < to; i++) {
		if (text[i] != '\t') continue;
		if (i > run) x += rvn_text_width(m->font, m->size, text + run, i - run);
		x = (floorf(x / m->tab + 0.001f) + 1.0f) * m->tab;
		run = i + 1;
	}
	if (to > run) x += rvn_text_width(m->font, m->size, text + run, to - run);
	return x;
}

/* The byte offset of a line nearest to the position x. */
static int rvn_dv_offset_at(const rvn_dv_metrics_t* m, const char* text, int length, float target) {
	float x = 0.0f;
	int i = 0;
	while (i < length) {
		int next = rvn_utf8_next(text, length, i);
		float after = text[i] == '\t' ? (floorf(x / m->tab + 0.001f) + 1.0f) * m->tab : x + rvn_text_width(m->font, m->size, text + i, next - i);
		if (target < (x + after) * 0.5f) return i;
		x = after;
		i = next;
	}
	return length;
}

/* ---------------------------------------------------------------------------
 *  Painting
 * ------------------------------------------------------------------------- */

static void rvn_dv_paint_text(RPainter* painter, const rvn_dv_metrics_t* m, const char* text, int length, float x, float y,
	const rvn_span_t* spans, int span_count, rvn_color_t color) {

	float pen = 0.0f;
	int i = 0, span = 0;
	while (i < length) {
		int stop = length;
		rvn_color_t run_color = color;
		const char* tab;
		while (span < span_count && spans[span].end <= i) span++;
		if (span < span_count) {
			if (spans[span].start <= i) {
				run_color = rvn_syntax_color(m->theme, (rvn_token_t)spans[span].token, color);
				stop = rvn_mini(stop, spans[span].end);
			} else {
				stop = rvn_mini(stop, spans[span].start);
			}
		}
		if (text[i] == '\t') {
			pen = (floorf(pen / m->tab + 0.001f) + 1.0f) * m->tab;
			i++;
			continue;
		}
		tab = (const char*)memchr(text + i, '\t', (size_t)(stop - i));
		if (tab) stop = (int)(tab - text);
		rvn_paint_text(painter, m->font_id, m->size, x + pen, y, text + i, stop - i, run_color);
		pen += rvn_text_width(m->font, m->size, text + i, stop - i);
		i = stop;
	}
}

static rvn_color_t rvn_dv_kind_color(const rvn_widget_t* widget, int kind, int word) {
	if (kind == RVN_DIFF_ADDED) return rvn_widget_color(widget, word ? R_COLOR_DIFF_ADDED_WORD : R_COLOR_DIFF_ADDED);
	if (kind == RVN_DIFF_REMOVED) return rvn_widget_color(widget, word ? R_COLOR_DIFF_REMOVED_WORD : R_COLOR_DIFF_REMOVED);
	return 0;
}

/* The selection of a row on a side, as a byte range of its text; R_FALSE when none. */
static int rvn_dv_row_selection(const rvn_diff_view_t* dv, int row, int side, int length, int* from, int* to) {
	int r0 = dv->anchor_row, c0 = dv->anchor_col, r1 = dv->caret_row, c1 = dv->caret_col;
	if (!dv->has_selection || (dv->mode == R_DIFF_VIEW_SPLIT && side != dv->sel_side)) return R_FALSE;
	if (r1 < r0 || (r1 == r0 && c1 < c0)) {
		int t = r0;
		r0 = r1;
		r1 = t;
		t = c0;
		c0 = c1;
		c1 = t;
	}
	if (row < r0 || row > r1) return R_FALSE;
	*from = row == r0 ? rvn_mini(c0, length) : 0;
	*to = row == r1 ? rvn_mini(c1, length) : length;
	/* a whole line in the middle shows its end too */
	return *to > *from || (row != r1);
}

/* One side of a line row: its background, numbers, sign, marks of words, selection and text. */
static void rvn_dv_paint_line(rvn_diff_view_t* dv, RPainter* painter, const rvn_dv_metrics_t* m, int row, int pane, int entry,
	float y, float scroll_x, rvn_color_t text_color, rvn_color_t dim, rvn_color_t selection) {

	rvn_widget_t* widget = &dv->base;
	int unified = m->panes == 1;
	float x0 = m->pane_x[pane], gutter_x = x0 + RVN_DV_MARK, text_x = m->text_x[pane] + RVN_DV_PAD * 0.5f - scroll_x;
	rvn_rect_t text_area = rvn_rect(m->text_x[pane], y, m->text_w, m->row);
	const rvn_dv_entry_t* e;
	const char* text;
	int length, side = pane == 0 && !unified ? R_DIFF_LEFT : R_DIFF_RIGHT, kind, from, to;
	rvn_color_t back;
	char number[16];
	float text_y = y + 2.0f;

	if (entry < 0) {
		/* nothing on this side of the pair */
		rvn_paint_rect(painter, rvn_rect(x0, y, m->pane_w, m->row), rvn_color_fade(rvn_widget_color(widget, R_COLOR_TRACK), 0.55f));
		return;
	}
	e = &dv->entries[entry];
	kind = e->kind;
	back = rvn_dv_kind_color(widget, kind, R_FALSE);
	if (rvn_color_a(back)) {
		rvn_paint_rect(painter, rvn_rect(x0, y, m->pane_w, m->row), back);
		/* the gutter a little stronger */
		rvn_paint_rect(painter, rvn_rect(x0, y, m->text_x[pane] - x0, m->row), rvn_color_fade(back, 0.8f));
	}
	if (dv->line_numbers) {
		if (unified) {
			if (e->left) {
				snprintf(number, sizeof(number), "%d", e->left);
				rvn_paint_text_in(painter, m->font_id, m->size, rvn_rect(gutter_x, y, m->number_w - RVN_DV_PAD, m->row), 1.0f, number, -1, dim);
			}
			if (e->right) {
				snprintf(number, sizeof(number), "%d", e->right);
				rvn_paint_text_in(painter, m->font_id, m->size, rvn_rect(gutter_x + m->number_w, y, m->number_w - RVN_DV_PAD, m->row), 1.0f, number, -1, dim);
			}
		} else {
			int value = side == R_DIFF_LEFT ? e->left : e->right;
			if (value) {
				snprintf(number, sizeof(number), "%d", value);
				rvn_paint_text_in(painter, m->font_id, m->size, rvn_rect(gutter_x, y, m->number_w - RVN_DV_PAD, m->row), 1.0f, number, -1, dim);
			}
		}
	}
	if (kind != RVN_DIFF_SAME) {
		rvn_paint_text_in(painter, m->font_id, m->size, rvn_rect(m->text_x[pane] - m->sign_w, y, m->sign_w, m->row), 0.5f,
			kind == RVN_DIFF_ADDED ? "+" : "-", 1, dim);
	}

	text = rvn_dv_entry_text(dv, entry, side, unified, &length);
	if (!text) return;
	rvn_paint_push_clip(painter, text_area);
	/* the words that changed */
	if (kind != RVN_DIFF_SAME && dv->word_diff && e->pair >= 0) {
		int w;
		rvn_dv_words(dv, entry);
		e = &dv->entries[entry];
		if (e->words >= 0) {
			rvn_color_t word = rvn_dv_kind_color(widget, kind, R_TRUE);
			for (w = 0; w < e->word_count; w++) {
				const rvn_diff_range_t* range = &dv->ranges[e->words + w];
				int start = rvn_mini(range->start, length), end = rvn_mini(range->end, length);
				float a = rvn_dv_advance(m, text, start), b = rvn_dv_advance(m, text, end);
				if (end > start) rvn_paint_box(painter, rvn_rect(text_x + a, y + 1.0f, rvn_maxf(2.0f, b - a), m->row - 2.0f), 2.0f, word, 0.0f, 0);
			}
		}
	}
	/* the selection */
	if (rvn_dv_row_selection(dv, row, side, length, &from, &to)) {
		float a = rvn_dv_advance(m, text, from), b = rvn_dv_advance(m, text, to);
		if (to >= length && row != (dv->caret_row > dv->anchor_row ? dv->caret_row : dv->anchor_row)) b += m->char_w * 0.6f;
		rvn_paint_rect(painter, rvn_rect(text_x + a, y, rvn_maxf(0.0f, b - a), m->row), selection);
	}
	/* the text, in the colors of the code */
	{
		rvn_span_t spans[RVN_DV_MAX_SPANS];
		int span_count = 0;
		const rvn_language_t* language = dv->files[e->file].language;
		if (language && length <= RVN_DIFF_MAX_WORD_LINE) {
			int use_left = text == e->left_text;
			rvn_dv_lex_until(dv, (uint32_t)entry);
			e = &dv->entries[entry];
			rvn_syntax_line(language, text, length, use_left ? e->lex_left : e->lex_right, spans, RVN_DV_MAX_SPANS, &span_count);
		}
		rvn_dv_paint_text(painter, m, text, length, text_x, text_y, spans, span_count, text_color);
	}
	rvn_paint_pop_clip(painter);
}

/* The label of a row that is not a line (a fold, a hunk, a file, a note). */
static void rvn_dv_row_label(rvn_diff_view_t* dv, const rvn_dv_row_t* r, rvn_text_t* out) {
	out->length = 0;
	if (out->data) out->data[0] = '\0';
	switch (r->kind) {
		case RVN_DV_ROW_FOLD:
			rvn_text_printf(out, r->b == 1 ? "%d unchanged line" : "%d unchanged lines", r->b);
			break;
		case RVN_DV_ROW_HUNK:
			rvn_text_append(out, dv->hunks[r->a].header, (size_t)dv->hunks[r->a].header_length);
			break;
		case RVN_DV_ROW_FILE: {
			const rvn_dv_file_t* file = &dv->files[r->a];
			const char* left = rvn_dv_display_name(file->left_name);
			const char* right = rvn_dv_display_name(file->right_name);
			if (left && right && strcmp(left, right) != 0 && strcmp(left, "/dev/null") != 0 && strcmp(right, "/dev/null") != 0) {
				rvn_text_printf(out, "%s \xE2\x86\x92 %s", left, right);
			} else if (right && strcmp(right, "/dev/null") == 0) {
				rvn_text_printf(out, "%s (deleted)", left ? left : "");
			} else if (left && strcmp(left, "/dev/null") == 0) {
				rvn_text_printf(out, "%s (new)", right ? right : "");
			} else {
				rvn_text_add(out, right ? right : left ? left : "");
			}
			break;
		}
		case RVN_DV_ROW_NOTE:
			rvn_text_add(out, dv->notes[r->a].text ? dv->notes[r->a].text : "");
			break;
		default:
			break;
	}
	if (!out->data) rvn_text_add(out, "");
}

/* The change a row belongs to, -1 for none. */
static int rvn_dv_row_change(const rvn_diff_view_t* dv, int row) {
	int entry = rvn_dv_row_entry(dv, row, R_DIFF_LEFT), lo = 0, hi;
	if (entry < 0) entry = rvn_dv_row_entry(dv, row, R_DIFF_RIGHT);
	if (entry < 0 || dv->entries[entry].kind == RVN_DIFF_SAME) return -1;
	hi = (int)dv->change_count - 1;
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		if ((uint32_t)entry < dv->changes[mid].first) hi = mid - 1;
		else if ((uint32_t)entry >= dv->changes[mid].end) lo = mid + 1;
		else return mid;
	}
	return -1;
}

static void rvn_dv_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_diff_view_t* dv = (rvn_diff_view_t*)widget;
	rvn_dv_metrics_t m;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	rvn_color_t dim = rvn_widget_color(widget, R_COLOR_TEXT_PLACEHOLDER);
	rvn_color_t separator = rvn_widget_color(widget, R_COLOR_SEPARATOR);
	rvn_color_t header = rvn_widget_color(widget, R_COLOR_HEADER);
	rvn_color_t accent = rvn_widget_color(widget, R_COLOR_ACCENT);
	rvn_color_t selection = rvn_widget_color(widget, R_COLOR_SELECTION);
	rvn_vec2_t origin = rvn_widget_origin(widget);
	float mouse_x = widget->window ? widget->window->mouse_x - origin.x : -1.0f;
	float mouse_y = widget->window ? widget->window->mouse_y - origin.y : -1.0f;
	float offset, scroll_x;
	int focused = (widget->flags & RVN_WF_FOCUSED) != 0, first, last, i;
	float ui_size = rvn_widget_font_size(widget, 0.0f);

	rvn_dv_ensure(dv);
	rvn_dv_metrics(dv, &m);
	offset = rvn_scrollbar_shown(&dv->bar, painter);
	scroll_x = rvn_scrollbar_shown(&dv->hbar, painter);

	rvn_paint_box(painter, rect, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS), rvn_widget_color(widget, R_COLOR_INPUT),
		rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH), rvn_widget_color(widget, focused ? R_COLOR_FOCUS : R_COLOR_BORDER));

	/* the names over the sides */
	if (m.header > 0.0f) {
		rvn_rect_t bar_rect = rvn_rect(1.0f, 1.0f, rect.w - 2.0f, m.header);
		rvn_paint_push_clip(painter, bar_rect);
		rvn_paint_rect(painter, bar_rect, header);
		if (m.panes == 2) {
			const char* names[2];
			int p;
			names[0] = dv->left_name;
			names[1] = dv->right_name;
			for (p = 0; p < 2; p++) {
				rvn_rect_t cell = rvn_rect(m.pane_x[p] + RVN_DV_PAD + RVN_DV_MARK, 1.0f, m.pane_w - RVN_DV_PAD * 2.0f, m.header);
				if (names[p]) rvn_paint_text_in(painter, R_INVALID_ID, ui_size, cell, 0.0f, names[p], -1, text);
			}
			rvn_paint_rect(painter, rvn_rect(m.pane_x[1] - 1.0f, 1.0f, 1.0f, m.header), separator);
		} else {
			rvn_text_t* label = &dv->scratch;
			label->length = 0;
			if (dv->left_name && dv->right_name && strcmp(dv->left_name, dv->right_name) != 0) {
				rvn_text_printf(label, "%s \xE2\x86\x92 %s", dv->left_name, dv->right_name);
			} else {
				rvn_text_add(label, dv->right_name ? dv->right_name : dv->left_name ? dv->left_name : "");
			}
			rvn_paint_text_in(painter, R_INVALID_ID, ui_size, rvn_rect(RVN_DV_PAD + RVN_DV_MARK, 1.0f, rect.w - RVN_DV_PAD * 2.0f, m.header),
				0.0f, label->data ? label->data : "", -1, text);
		}
		rvn_paint_rect(painter, rvn_rect(1.0f, m.header, rect.w - 2.0f, 1.0f), separator);
		rvn_paint_pop_clip(painter);
	}

	/* the rows on screen */
	rvn_paint_push_clip(painter, m.body);
	first = rvn_maxi((int)floorf(offset / m.row), 0);
	last = rvn_mini((int)dv->row_count, first + (int)(m.body.h / m.row) + 3);
	for (i = first; i < last; i++) {
		const rvn_dv_row_t* r = &dv->rows[i];
		float y = m.body.y + m.row * (float)i - offset;
		rvn_rect_t line = rvn_rect(m.body.x, y, m.body.w, m.row);
		switch (r->kind) {
			case RVN_DV_ROW_LINE:
				rvn_dv_paint_line(dv, painter, &m, i, 0, r->a, y, scroll_x, text, dim, selection);
				break;
			case RVN_DV_ROW_PAIR:
				rvn_dv_paint_line(dv, painter, &m, i, 0, r->a, y, scroll_x, text, dim, selection);
				rvn_dv_paint_line(dv, painter, &m, i, 1, r->b, y, scroll_x, text, dim, selection);
				break;
			case RVN_DV_ROW_FOLD:
			case RVN_DV_ROW_HUNK:
			case RVN_DV_ROW_FILE: {
				int hovered = r->kind == RVN_DV_ROW_FOLD && i == dv->hot_row && (widget->flags & RVN_WF_HOVERED) && painter->enabled;
				float x = m.body.x + RVN_DV_MARK + RVN_DV_PAD;
				rvn_dv_row_label(dv, r, &dv->scratch);
				rvn_paint_rect(painter, line, rvn_widget_color(widget, hovered ? R_COLOR_HEADER_HOVERED : R_COLOR_HEADER));
				if (r->kind == RVN_DV_ROW_FOLD) {
					/* two chevrons, up and down: the lines open both ways */
					rvn_paint_icon(painter, RVN_ICON_CHEVRON_UP, x + 5.0f, y + m.row * 0.5f - 3.0f, 5.0f, dim);
					rvn_paint_icon(painter, RVN_ICON_CHEVRON_DOWN, x + 5.0f, y + m.row * 0.5f + 3.0f, 5.0f, dim);
					x += 16.0f;
				}
				if (r->kind == RVN_DV_ROW_FILE) {
					rvn_paint_rect(painter, rvn_rect(line.x, y, line.w, 1.0f), separator);
					rvn_paint_text_in(painter, R_INVALID_ID, ui_size, rvn_rect(x, y, line.w - x - RVN_DV_PAD, m.row), 0.0f,
						dv->scratch.data, -1, text);
				} else {
					rvn_paint_text_in(painter, r->kind == RVN_DV_ROW_HUNK ? m.font_id : R_INVALID_ID,
						r->kind == RVN_DV_ROW_HUNK ? m.size : ui_size - 1.0f,
						rvn_rect(x, y, line.w - x - RVN_DV_PAD, m.row), 0.0f, dv->scratch.data, -1, dim);
				}
				rvn_paint_rect(painter, rvn_rect(line.x, y + m.row - 1.0f, line.w, 1.0f), rvn_color_fade(separator, 0.6f));
				break;
			}
			case RVN_DV_ROW_NOTE: {
				/* under its line: in split mode on the side of that line */
				int pane = (m.panes == 2 && dv->notes[r->a].side == R_DIFF_RIGHT) ? 1 : 0;
				float x = m.text_x[pane], right = m.panes == 2 ? m.pane_x[pane] + m.pane_w : m.body.x + m.body.w;
				rvn_rect_t band = rvn_rect(m.pane_x[pane], y, right - m.pane_x[pane], m.row);
				rvn_dv_row_label(dv, r, &dv->scratch);
				if (m.panes == 2) {
					rvn_rect_t other = rvn_rect(m.pane_x[1 - pane], y, m.pane_w, m.row);
					rvn_paint_rect(painter, other, rvn_color_fade(rvn_widget_color(widget, R_COLOR_TRACK), 0.55f));
				}
				rvn_paint_rect(painter, band, rvn_color_fade(accent, 0.10f));
				rvn_paint_rect(painter, rvn_rect(x - 4.0f, y + 2.0f, 3.0f, m.row - 4.0f), accent);
				rvn_paint_push_clip(painter, band);
				rvn_paint_text_in(painter, R_INVALID_ID, ui_size - 1.0f, rvn_rect(x + RVN_DV_PAD, y, right - x - RVN_DV_PAD * 2.0f, m.row), 0.0f,
					dv->scratch.data, -1, text);
				rvn_paint_pop_clip(painter);
				break;
			}
			default:
				break;
		}
		/* the current change: a bar of the accent at the left */
		if (dv->current >= 0 && (r->kind == RVN_DV_ROW_LINE || r->kind == RVN_DV_ROW_PAIR) && rvn_dv_row_change(dv, i) == dv->current) {
			rvn_paint_rect(painter, rvn_rect(m.body.x, y, RVN_DV_MARK, m.row), accent);
		}
	}
	/* a mark in the gutter of the lines that have a note */
	if (dv->note_count > 0) {
		for (i = first; i < last; i++) {
			if (i + 1 < (int)dv->row_count && dv->rows[i + 1].kind == RVN_DV_ROW_NOTE && dv->rows[i].kind != RVN_DV_ROW_NOTE) {
				float y = m.body.y + m.row * (float)i - offset;
				int pane = (m.panes == 2 && dv->notes[dv->rows[i + 1].a].side == R_DIFF_RIGHT) ? 1 : 0;
				rvn_paint_box(painter, rvn_rect(m.pane_x[pane] + RVN_DV_MARK + 3.0f, y + m.row * 0.5f - 3.0f, 6.0f, 6.0f), 3.0f, accent, 0.0f, 0);
			}
		}
	}
	if (m.panes == 2) {
		float bottom = rvn_minf(m.body.h, m.row * (float)dv->row_count - offset);
		if (bottom > 0.0f) rvn_paint_rect(painter, rvn_rect(m.pane_x[1] - 1.0f, m.body.y, 1.0f, bottom), separator);
	}
	if (dv->row_count == 0) {
		rvn_paint_text_in(painter, R_INVALID_ID, ui_size, m.body, 0.5f, dv->is_patch ? "No changes in this patch" : "No differences", -1, dim);
	}
	rvn_paint_pop_clip(painter);

	if (m.vtrack.w > 0.0f) rvn_scrollbar_paint(&dv->bar, widget, painter, m.vtrack, R_TRUE, mouse_x, mouse_y);
	if (m.htrack.h > 0.0f) rvn_scrollbar_paint(&dv->hbar, widget, painter, m.htrack, R_FALSE, mouse_x, mouse_y);
}

/* ---------------------------------------------------------------------------
 *  Changes, rows, selection
 * ------------------------------------------------------------------------- */

/* The first row of a change, -1 when it is not shown. */
static int rvn_dv_change_row(const rvn_diff_view_t* dv, int change) {
	uint32_t i;
	for (i = 0; i < dv->row_count; i++) {
		int a = rvn_dv_row_entry(dv, (int)i, R_DIFF_LEFT), b = rvn_dv_row_entry(dv, (int)i, R_DIFF_RIGHT);
		uint32_t first = dv->changes[change].first, end = dv->changes[change].end;
		if ((a >= 0 && (uint32_t)a >= first && (uint32_t)a < end) || (b >= 0 && (uint32_t)b >= first && (uint32_t)b < end)) return (int)i;
	}
	return -1;
}

static void rvn_dv_go_to(rvn_diff_view_t* dv, int change) {
	rvn_dv_metrics_t m;
	int row;
	rvn_dv_ensure(dv);
	if (change < 0 || change >= (int)dv->change_count) return;
	dv->current = change;
	row = rvn_dv_change_row(dv, change);
	rvn_dv_metrics(dv, &m);
	if (m.body.h <= 0.0f) dv->pending = change;
	if (row >= 0) {
		/* a quarter down the view, with what comes before it in sight */
		rvn_scrollbar_set(&dv->bar, m.row * (float)row - rvn_maxf(0.0f, m.body.h * 0.25f));
		rvn_scrollbar_clamp(&dv->bar);
	}
	rvn_widget_invalidate_paint(&dv->base);
}

/* The change after (step 1) or before (step -1) the current one, from what is on screen when none is. */
static int rvn_dv_step(rvn_diff_view_t* dv, int step) {
	int next;
	rvn_dv_ensure(dv);
	if (dv->change_count == 0) return -1;
	if (dv->current < 0) next = step > 0 ? 0 : (int)dv->change_count - 1;
	else next = dv->current + step;
	if (next < 0 || next >= (int)dv->change_count) return -1;
	rvn_dv_go_to(dv, next);
	return next;
}

static void rvn_dv_selected_text(rvn_diff_view_t* dv, rvn_text_t* out) {
	int r0 = dv->anchor_row, c0 = dv->anchor_col, r1 = dv->caret_row, c1 = dv->caret_col, row, first = R_TRUE;
	out->length = 0;
	if (out->data) out->data[0] = '\0';
	if (!dv->has_selection) return;
	if (r1 < r0 || (r1 == r0 && c1 < c0)) {
		int t = r0;
		r0 = r1;
		r1 = t;
		t = c0;
		c0 = c1;
		c1 = t;
	}
	for (row = r0; row <= r1 && row < (int)dv->row_count; row++) {
		int entry = rvn_dv_row_entry(dv, row, dv->sel_side), length, from, to;
		const char* text;
		if (entry < 0) continue;
		text = rvn_dv_entry_text(dv, entry, dv->sel_side, dv->mode == R_DIFF_VIEW_UNIFIED, &length);
		if (!text) continue;
		from = row == r0 ? rvn_mini(c0, length) : 0;
		to = row == r1 ? rvn_mini(c1, length) : length;
		if (!first) rvn_text_append(out, "\n", 1);
		first = R_FALSE;
		if (to > from) rvn_text_append(out, text + from, (size_t)(to - from));
	}
}

/* Where a point is: the row, the side, the byte of its line (-1 when not on a line), and whether it is in the gutter. */
static int rvn_dv_hit(rvn_diff_view_t* dv, const rvn_dv_metrics_t* m, float x, float y, int* side, int* column, int* in_gutter) {
	int row = rvn_dv_row_at(dv, m, x, y), pane, entry, length;
	const char* text;
	*column = -1;
	*in_gutter = R_FALSE;
	pane = (m->panes == 2 && x >= m->pane_x[1]) ? 1 : 0;
	*side = m->panes == 2 ? (pane == 0 ? R_DIFF_LEFT : R_DIFF_RIGHT) : R_DIFF_RIGHT;
	if (row < 0) return -1;
	entry = rvn_dv_row_entry(dv, row, *side);
	*in_gutter = x < m->text_x[pane];
	if (m->panes == 1 && entry >= 0) {
		const rvn_dv_entry_t* e = &dv->entries[entry];
		/* unified: the column of the left numbers names the left line */
		if (*in_gutter && e->left && (x < m->pane_x[0] + RVN_DV_MARK + m->number_w || !e->right)) *side = R_DIFF_LEFT;
		else if (!*in_gutter && e->kind == RVN_DIFF_REMOVED) *side = R_DIFF_LEFT;
	}
	if (entry < 0) return row;
	text = rvn_dv_entry_text(dv, entry, *side, m->panes == 1, &length);
	if (text) *column = rvn_dv_offset_at(m, text, length, x - (m->text_x[pane] + RVN_DV_PAD * 0.5f - dv->hbar.offset));
	else *column = 0;
	return row;
}

/* ---------------------------------------------------------------------------
 *  Input
 * ------------------------------------------------------------------------- */

static void rvn_dv_scroll_rows(rvn_diff_view_t* dv, const rvn_dv_metrics_t* m, float rows) {
	rvn_scrollbar_set(&dv->bar, dv->bar.offset + rows * m->row);
	rvn_widget_invalidate_paint(&dv->base);
}

static int rvn_dv_event(rvn_widget_t* widget, const REvent* event) {
	rvn_diff_view_t* dv = (rvn_diff_view_t*)widget;
	rvn_dv_metrics_t m;
	int changed = R_FALSE, row, side, column, in_gutter;

	rvn_dv_ensure(dv);
	rvn_dv_metrics(dv, &m);
	if (!dv->selecting && (event->type == R_EVENT_MOUSE_DOWN || event->type == R_EVENT_MOUSE_MOVE || event->type == R_EVENT_MOUSE_UP)) {
		if ((m.vtrack.w > 0.0f || dv->bar.dragging) && rvn_scrollbar_event(&dv->bar, widget, event, m.vtrack, R_TRUE, &changed)) {
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		}
		if ((m.htrack.h > 0.0f || dv->hbar.dragging) && rvn_scrollbar_event(&dv->hbar, widget, event, m.htrack, R_FALSE, &changed)) {
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		}
	}
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			if (dv->selecting) {
				float y = rvn_clampf(event->local_y, m.body.y, m.body.y + m.body.h - 1.0f);
				/* past the top or the bottom edge: the rows come to the pointer */
				if (event->local_y < m.body.y) rvn_dv_scroll_rows(dv, &m, -1.0f);
				else if (event->local_y > m.body.y + m.body.h) rvn_dv_scroll_rows(dv, &m, 1.0f);
				row = rvn_dv_hit(dv, &m, rvn_clampf(event->local_x, m.body.x, m.body.x + m.body.w - 1.0f), y, &side, &column, &in_gutter);
				if (dv->mode == R_DIFF_VIEW_SPLIT) {
					/* the side where the selection started, whatever pane the pointer is over */
					int entry = rvn_dv_row_entry(dv, row, dv->sel_side), length;
					const char* text = entry >= 0 ? rvn_dv_entry_text(dv, entry, dv->sel_side, R_FALSE, &length) : NULL;
					int pane = dv->sel_side == R_DIFF_LEFT ? 0 : 1;
					column = text ? rvn_dv_offset_at(&m, text, length, event->local_x - (m.text_x[pane] + RVN_DV_PAD * 0.5f - dv->hbar.offset)) : 0;
				}
				if (row >= 0) {
					dv->caret_row = row;
					dv->caret_col = rvn_maxi(column, 0);
					dv->has_selection = dv->caret_row != dv->anchor_row || dv->caret_col != dv->anchor_col;
				}
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			row = rvn_dv_row_at(dv, &m, event->local_x, event->local_y);
			widget->cursor = (row >= 0 && dv->rows[row].kind == RVN_DV_ROW_FOLD) ? R_CURSOR_HAND : R_CURSOR_IBEAM;
			if (row != dv->hot_row) {
				dv->hot_row = row;
				rvn_widget_invalidate_paint(widget);
			}
			return R_FALSE;
		case R_EVENT_MOUSE_LEAVE:
			dv->hot_row = -1;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN: {
			int entry;
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			row = rvn_dv_hit(dv, &m, event->local_x, event->local_y, &side, &column, &in_gutter);
			if (row < 0) return R_TRUE;
			if (dv->rows[row].kind == RVN_DV_ROW_FOLD) {
				Rdiff_view_expand(widget->id, row);
				return R_TRUE;
			}
			entry = rvn_dv_row_entry(dv, row, dv->mode == R_DIFF_VIEW_SPLIT ? side : R_DIFF_RIGHT);
			if (entry < 0) {
				dv->has_selection = R_FALSE;
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			if ((event->modifiers & R_MOD_SHIFT) && dv->has_selection && (dv->mode == R_DIFF_VIEW_UNIFIED || side == dv->sel_side)) {
				dv->caret_row = row;
				dv->caret_col = rvn_maxi(column, 0);
			} else {
				dv->sel_side = dv->mode == R_DIFF_VIEW_SPLIT ? side : R_DIFF_RIGHT;
				dv->anchor_row = dv->caret_row = row;
				dv->anchor_col = dv->caret_col = rvn_maxi(column, 0);
				dv->has_selection = R_FALSE;
				if (event->clicks == 2 || in_gutter) {
					/* the whole line */
					int length;
					rvn_dv_entry_text(dv, entry, dv->sel_side, dv->mode == R_DIFF_VIEW_UNIFIED, &length);
					dv->anchor_col = 0;
					dv->caret_col = length;
					dv->has_selection = R_TRUE;
				}
			}
			dv->selecting = !in_gutter && event->clicks < 2;
			if (dv->selecting) rvn_widget_capture_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			{
				const rvn_dv_entry_t* e = &dv->entries[entry];
				int line = side == R_DIFF_LEFT ? e->left : e->right;
				if (!line) {
					line = e->left ? e->left : e->right;
					side = e->left ? R_DIFF_LEFT : R_DIFF_RIGHT;
				}
				if (dv->callback) dv->callback(widget->id, (RDiffSide)side, line, in_gutter, dv->user_data);
				if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
			}
			return R_TRUE;
		}
		case R_EVENT_MOUSE_UP:
			if (event->button != R_MOUSE_LEFT || !dv->selecting) return R_FALSE;
			dv->selecting = R_FALSE;
			rvn_widget_release_mouse(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_WHEEL: {
			float along = event->wheel_y, across = event->wheel_x;
			if (event->modifiers & R_MOD_SHIFT) {
				across = along;
				along = 0.0f;
			}
			if (along != 0.0f) changed |= rvn_scrollbar_wheel(&dv->bar, -along * m.row * 3.0f);
			if (across != 0.0f) changed |= rvn_scrollbar_wheel(&dv->hbar, -across * 40.0f);
			rvn_widget_invalidate_paint(widget);
			return changed;
		}
		case R_EVENT_FOCUS_GAINED:
		case R_EVENT_FOCUS_LOST:
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_KEY_DOWN: {
			int command = (event->modifiers & (R_MOD_CTRL | R_MOD_SUPER)) != 0;
			int alt = (event->modifiers & R_MOD_ALT) != 0;
			int page = rvn_maxi(1, (int)(m.body.h / m.row) - 1);
			if (event->key == R_KEY_F7 || (alt && (event->key == R_KEY_DOWN || event->key == R_KEY_UP))) {
				int back = event->key == R_KEY_UP || (event->key == R_KEY_F7 && (event->modifiers & R_MOD_SHIFT));
				rvn_dv_step(dv, back ? -1 : 1);
				return R_TRUE;
			}
			if (command && event->key == R_KEY_C) {
				if (dv->has_selection) {
					rvn_dv_selected_text(dv, &dv->scratch);
					Rclipboard_set_text(dv->scratch.data ? dv->scratch.data : "");
				}
				return R_TRUE;
			}
			if (command && event->key == R_KEY_A) {
				int last_row = (int)dv->row_count - 1;
				if (last_row < 0) return R_TRUE;
				dv->sel_side = dv->mode == R_DIFF_VIEW_SPLIT && dv->has_selection ? dv->sel_side : R_DIFF_RIGHT;
				dv->anchor_row = 0;
				dv->anchor_col = 0;
				dv->caret_row = last_row;
				dv->caret_col = INT_MAX / 2;
				dv->has_selection = R_TRUE;
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			if (command || alt) return R_FALSE;
			switch (event->key) {
				case R_KEY_UP: rvn_dv_scroll_rows(dv, &m, -1.0f); return R_TRUE;
				case R_KEY_DOWN: rvn_dv_scroll_rows(dv, &m, 1.0f); return R_TRUE;
				case R_KEY_PAGE_UP: rvn_dv_scroll_rows(dv, &m, -(float)page); return R_TRUE;
				case R_KEY_PAGE_DOWN: rvn_dv_scroll_rows(dv, &m, (float)page); return R_TRUE;
				case R_KEY_HOME:
					rvn_scrollbar_set(&dv->bar, 0.0f);
					rvn_widget_invalidate_paint(widget);
					return R_TRUE;
				case R_KEY_END:
					rvn_scrollbar_set(&dv->bar, rvn_scrollbar_max(&dv->bar));
					rvn_widget_invalidate_paint(widget);
					return R_TRUE;
				case R_KEY_LEFT:
				case R_KEY_RIGHT:
					if (!rvn_scrollbar_needed(&dv->hbar)) return R_FALSE;
					rvn_scrollbar_set(&dv->hbar, dv->hbar.offset + (event->key == R_KEY_LEFT ? -40.0f : 40.0f));
					rvn_widget_invalidate_paint(widget);
					return R_TRUE;
				default:
					return R_FALSE;
			}
		}
		default:
			return R_FALSE;
	}
}

static int rvn_dv_pan(rvn_widget_t* widget, float dx, float dy, int coasting) {
	rvn_diff_view_t* dv = (rvn_diff_view_t*)widget;
	int moved = rvn_scrollbar_pan(&dv->bar, -dy, coasting);
	moved |= rvn_scrollbar_pan(&dv->hbar, -dx, coasting);
	return moved;
}

/* ---------------------------------------------------------------------------
 *  XML
 * ------------------------------------------------------------------------- */

static char* rvn_dv_read(const char* value) {
	char* path = rvn_xml_resolve_path(value);
	char* text = rvn_read_file(path ? path : value, NULL);
	rvn_free(path);
	return text;
}

static int rvn_dv_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_diff_view_t* dv = (rvn_diff_view_t*)widget;
	int number;
	if (RVN_ATTR("Left") || RVN_ATTR("Right")) {
		rvn_str_set(RVN_ATTR("Left") ? &dv->left : &dv->right, value);
		dv->is_patch = R_FALSE;
		rvn_dv_changed(widget, R_TRUE);
		return R_TRUE;
	}
	if (RVN_ATTR("LeftSource") || RVN_ATTR("RightSource")) {
		int left = RVN_ATTR("LeftSource");
		char* text = value && value[0] ? rvn_dv_read(value) : NULL;
		char** name_slot = left ? &dv->left_name : &dv->right_name;
		rvn_free(left ? dv->left : dv->right);
		if (left) dv->left = text;
		else dv->right = text;
		if (!*name_slot) rvn_str_set(name_slot, value);
		dv->is_patch = R_FALSE;
		rvn_dv_changed(widget, R_TRUE);
		/* a file that cannot be read is compared as empty: the attribute itself is right */
		if (!text && value && value[0]) rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_WARNING, "DiffView: cannot read \"%s\"", value);
		return R_TRUE;
	}
	if (RVN_ATTR("LeftName") || RVN_ATTR("RightName")) {
		rvn_str_set(RVN_ATTR("LeftName") ? &dv->left_name : &dv->right_name, value && value[0] ? value : NULL);
		rvn_dv_changed(widget, R_TRUE);
		return R_TRUE;
	}
	if (RVN_ATTR("Patch")) {
		Rdiff_view_set_patch(widget->id, value);
		return R_TRUE;
	}
	if (RVN_ATTR("PatchSource")) {
		char* text = value && value[0] ? rvn_dv_read(value) : NULL;
		if (!text && value && value[0]) rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_WARNING, "DiffView: cannot read \"%s\"", value);
		Rdiff_view_set_patch(widget->id, text ? text : "");
		rvn_free(text);
		return R_TRUE;
	}
	if (RVN_ATTR("Mode")) {
		if (rvn_stricmp(value, "Unified") == 0 || rvn_stricmp(value, "Inline") == 0) Rdiff_view_set_mode(widget->id, R_DIFF_VIEW_UNIFIED);
		else if (rvn_stricmp(value, "Split") == 0 || rvn_stricmp(value, "SideBySide") == 0) Rdiff_view_set_mode(widget->id, R_DIFF_VIEW_SPLIT);
		else return R_FALSE;
		return R_TRUE;
	}
	if (RVN_ATTR("Context")) {
		if (rvn_stricmp(value, "All") == 0) number = -1;
		else if (!rvn_parse_int(value, &number)) return R_FALSE;
		Rdiff_view_set_context(widget->id, number);
		return R_TRUE;
	}
	if (RVN_ATTR("IgnoreWhitespace")) {
		uint32_t flags = dv->flags & ~(uint32_t)(R_DIFF_IGNORE_TRAILING_SPACE | R_DIFF_IGNORE_SPACE);
		if (rvn_stricmp(value, "Trailing") == 0) flags |= R_DIFF_IGNORE_TRAILING_SPACE;
		else if (rvn_stricmp(value, "All") == 0 || rvn_stricmp(value, "true") == 0) flags |= R_DIFF_IGNORE_SPACE;
		else if (rvn_stricmp(value, "None") != 0 && rvn_stricmp(value, "false") != 0) return R_FALSE;
		Rdiff_view_set_flags(widget->id, flags);
		return R_TRUE;
	}
	if (RVN_ATTR("IgnoreCase")) {
		if (!rvn_parse_bool(value, &number)) return R_FALSE;
		Rdiff_view_set_flags(widget->id, number ? (dv->flags | R_DIFF_IGNORE_CASE) : (dv->flags & ~(uint32_t)R_DIFF_IGNORE_CASE));
		return R_TRUE;
	}
	if (RVN_ATTR("WordDiff")) {
		if (!rvn_parse_bool(value, &number)) return R_FALSE;
		Rdiff_view_set_word_diff(widget->id, number);
		return R_TRUE;
	}
	if (RVN_ATTR("LineNumbers")) {
		if (!rvn_parse_bool(value, &number)) return R_FALSE;
		Rdiff_view_set_line_numbers(widget->id, number);
		return R_TRUE;
	}
	if (RVN_ATTR("Language")) {
		Rdiff_view_set_language(widget->id, value && value[0] ? value : NULL);
		return R_TRUE;
	}
	return R_FALSE;
}

/* <Left>...</Left>, <Right>...</Right>, <Patch>...</Patch>: texts that hold several lines. */
static int rvn_dv_xml_child(rvn_widget_t* widget, const rvn_xml_node_t* node) {
	if (rvn_stricmp(node->name, "Left") == 0 || rvn_stricmp(node->name, "Right") == 0 || rvn_stricmp(node->name, "Patch") == 0) {
		return rvn_dv_set_attribute(widget, node->name, node->text ? node->text : "");
	}
	return R_FALSE;
}

const rvn_widget_class_t rvn_class_diff_view = {
	.type = R_WIDGET_DIFF_VIEW,
	.name = "DiffView",
	.size = sizeof(rvn_diff_view_t),
	/* (its paint compares the words of the lines on screen and keeps them) */
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_UI_PAINT | RVN_CF_SURFACE,
	.init = rvn_dv_init,
	.destroy = rvn_dv_destroy,
	.measure = rvn_dv_measure,
	.arrange = rvn_dv_arrange,
	.paint = rvn_dv_paint,
	.event = rvn_dv_event,
	.pan = rvn_dv_pan,
	.set_attribute = rvn_dv_set_attribute,
	.xml_child = rvn_dv_xml_child
};

/* ---------------------------------------------------------------------------
 *  Accessibility (core/accessibility.c)
 * ------------------------------------------------------------------------- */

int rvn_diff_view_row_at(rvn_widget_t* widget, float x, float y) {
	rvn_diff_view_t* dv = (rvn_diff_view_t*)widget;
	rvn_dv_metrics_t m;
	rvn_dv_ensure(dv);
	rvn_dv_metrics(dv, &m);
	return rvn_dv_row_at(dv, &m, x, y);
}

rvn_rect_t rvn_diff_view_row_rect(rvn_widget_t* widget, int row) {
	rvn_diff_view_t* dv = (rvn_diff_view_t*)widget;
	rvn_dv_metrics_t m;
	rvn_dv_ensure(dv);
	rvn_dv_metrics(dv, &m);
	return rvn_rect_intersect(m.body, rvn_rect(m.body.x, m.body.y + m.row * (float)row - dv->bar.offset, m.body.w, m.row));
}

static void rvn_dv_describe_side(rvn_diff_view_t* dv, int entry, int side, int unified, rvn_text_t* out) {
	const rvn_dv_entry_t* e = &dv->entries[entry];
	const char* text;
	int length;
	if (unified) side = e->kind == RVN_DIFF_REMOVED ? R_DIFF_LEFT : R_DIFF_RIGHT;
	text = rvn_dv_entry_text(dv, entry, side, R_FALSE, &length);
	if (e->kind == RVN_DIFF_ADDED) rvn_text_printf(out, "added line %d: ", e->right);
	else if (e->kind == RVN_DIFF_REMOVED) rvn_text_printf(out, "removed line %d: ", e->left);
	else rvn_text_printf(out, "line %d: ", side == R_DIFF_LEFT ? e->left : e->right);
	if (text && length > 0) rvn_text_append(out, text, (size_t)rvn_mini(length, 400));
	else rvn_text_add(out, "(empty)");
}

/* What a screen reader says of a row: "added line 12: return 0;". */
void rvn_diff_view_describe_row(rvn_widget_t* widget, int row, rvn_text_t* out) {
	rvn_diff_view_t* dv = (rvn_diff_view_t*)widget;
	const rvn_dv_row_t* r;
	rvn_dv_ensure(dv);
	if (row < 0 || row >= (int)dv->row_count) return;
	r = &dv->rows[row];
	switch (r->kind) {
		case RVN_DV_ROW_LINE:
			rvn_dv_describe_side(dv, r->a, R_DIFF_RIGHT, R_TRUE, out);
			break;
		case RVN_DV_ROW_PAIR:
			if (r->a >= 0 && r->a == r->b) {
				rvn_dv_describe_side(dv, r->a, R_DIFF_RIGHT, R_FALSE, out);
				break;
			}
			if (r->a >= 0) rvn_dv_describe_side(dv, r->a, R_DIFF_LEFT, R_FALSE, out);
			if (r->a >= 0 && r->b >= 0) rvn_text_add(out, "; ");
			if (r->b >= 0) rvn_dv_describe_side(dv, r->b, R_DIFF_RIGHT, R_FALSE, out);
			break;
		case RVN_DV_ROW_NOTE:
			rvn_text_add(out, "note: ");
			rvn_text_add(out, dv->notes[r->a].text ? dv->notes[r->a].text : "");
			break;
		default: {
			rvn_text_t label = { NULL, 0, 0 };
			rvn_dv_row_label(dv, r, &label);
			rvn_text_add(out, label.data);
			if (r->kind == RVN_DV_ROW_FOLD) rvn_text_add(out, ", folded");
			rvn_free(label.data);
			break;
		}
	}
}

int rvn_diff_view_row_is_fold(rvn_widget_t* widget, int row) {
	rvn_diff_view_t* dv = (rvn_diff_view_t*)widget;
	rvn_dv_ensure(dv);
	return row >= 0 && row < (int)dv->row_count && dv->rows[row].kind == RVN_DV_ROW_FOLD;
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

Rid Rdiff_view_create(void) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_DIFF_VIEW);
	return widget ? widget->id : R_INVALID_ID;
}

void Rdiff_view_set_texts(Rid id, const char* left, const char* right) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DIFF_VIEW);
	{
		rvn_diff_view_t* dv = (rvn_diff_view_t*)widget;
		char* a = rvn_strdup(left ? left : "");
		char* b = rvn_strdup(right ? right : "");
		rvn_free(dv->left);
		rvn_free(dv->right);
		dv->left = a;
		dv->right = b;
		dv->is_patch = R_FALSE;
		dv->current = -1;
		rvn_scrollbar_set(&dv->bar, 0.0f);
		rvn_dv_changed(widget, R_TRUE);
	}
}

void Rdiff_view_set_names(Rid id, const char* left_name, const char* right_name) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DIFF_VIEW);
	{
		rvn_diff_view_t* dv = (rvn_diff_view_t*)widget;
		rvn_str_set(&dv->left_name, left_name && left_name[0] ? left_name : NULL);
		rvn_str_set(&dv->right_name, right_name && right_name[0] ? right_name : NULL);
		rvn_dv_changed(widget, R_TRUE);
	}
}

int Rdiff_view_load_files(Rid id, const char* left_path, const char* right_path) {
	RVN_WIDGET(widget, id, R_WIDGET_DIFF_VIEW, R_FALSE);
	{
		rvn_diff_view_t* dv = (rvn_diff_view_t*)widget;
		char* a = left_path ? rvn_read_file(left_path, NULL) : NULL;
		char* b = right_path ? rvn_read_file(right_path, NULL) : NULL;
		int ok = a != NULL && b != NULL;
		rvn_free(dv->left);
		rvn_free(dv->right);
		dv->left = a;
		dv->right = b;
		if (!dv->left_name && left_path) dv->left_name = rvn_strdup(left_path);
		if (!dv->right_name && right_path) dv->right_name = rvn_strdup(right_path);
		dv->is_patch = R_FALSE;
		dv->current = -1;
		rvn_scrollbar_set(&dv->bar, 0.0f);
		rvn_dv_changed(widget, R_TRUE);
		if (!ok) rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_WARNING, "Rdiff_view_load_files: cannot read \"%s\"", a ? right_path : left_path);
		return ok;
	}
}

int Rdiff_view_set_patch(Rid id, const char* patch) {
	RVN_WIDGET(widget, id, R_WIDGET_DIFF_VIEW, R_FALSE);
	{
		rvn_diff_view_t* dv = (rvn_diff_view_t*)widget;
		char* copy = rvn_strdup(patch ? patch : "");
		rvn_free(dv->patch_text);
		dv->patch_text = copy;
		dv->is_patch = R_TRUE;
		dv->current = -1;
		rvn_scrollbar_set(&dv->bar, 0.0f);
		rvn_dv_changed(widget, R_TRUE);
		rvn_dv_ensure(dv);
		return dv->patch.hunk_count > 0;
	}
}

void Rdiff_view_set_mode(Rid id, RDiffViewMode mode) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DIFF_VIEW);
	if (mode != R_DIFF_VIEW_UNIFIED && mode != R_DIFF_VIEW_SPLIT) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rdiff_view_set_mode: invalid mode %d", (int)mode);
		return;
	}
	if (((rvn_diff_view_t*)widget)->mode == mode) return;
	((rvn_diff_view_t*)widget)->mode = mode;
	rvn_dv_changed(widget, R_FALSE);
}

RDiffViewMode Rdiff_view_get_mode(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_DIFF_VIEW, R_DIFF_VIEW_UNIFIED);
	return ((rvn_diff_view_t*)widget)->mode;
}

void Rdiff_view_set_context(Rid id, int lines) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DIFF_VIEW);
	((rvn_diff_view_t*)widget)->context = lines < 0 ? -1 : lines;
	rvn_dv_changed(widget, R_FALSE);
}

int Rdiff_view_get_context(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_DIFF_VIEW, 0);
	return ((rvn_diff_view_t*)widget)->context;
}

void Rdiff_view_set_flags(Rid id, uint32_t flags) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DIFF_VIEW);
	flags &= (uint32_t)(R_DIFF_IGNORE_TRAILING_SPACE | R_DIFF_IGNORE_SPACE | R_DIFF_IGNORE_CASE);
	if (((rvn_diff_view_t*)widget)->flags == flags) return;
	((rvn_diff_view_t*)widget)->flags = flags;
	((rvn_diff_view_t*)widget)->current = -1;
	rvn_dv_changed(widget, R_TRUE);
}

uint32_t Rdiff_view_get_flags(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_DIFF_VIEW, 0);
	return ((rvn_diff_view_t*)widget)->flags;
}

void Rdiff_view_set_word_diff(Rid id, int enabled) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DIFF_VIEW);
	((rvn_diff_view_t*)widget)->word_diff = enabled ? R_TRUE : R_FALSE;
	rvn_widget_invalidate_paint(widget);
}

void Rdiff_view_set_line_numbers(Rid id, int visible) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DIFF_VIEW);
	((rvn_diff_view_t*)widget)->line_numbers = visible ? R_TRUE : R_FALSE;
	rvn_widget_invalidate_layout(widget);
}

void Rdiff_view_set_language(Rid id, const char* language) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DIFF_VIEW);
	rvn_str_set(&((rvn_diff_view_t*)widget)->language_name, language && language[0] ? language : NULL);
	rvn_dv_changed(widget, R_TRUE);
}

int Rdiff_view_get_change_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_DIFF_VIEW, 0);
	rvn_dv_ensure((rvn_diff_view_t*)widget);
	return (int)((rvn_diff_view_t*)widget)->change_count;
}

int Rdiff_view_get_change(Rid id, int change, int* left_line, int* left_count, int* right_line, int* right_count) {
	rvn_diff_view_t* dv;
	const rvn_dv_change_t* c;
	RVN_WIDGET(widget, id, R_WIDGET_DIFF_VIEW, R_FALSE);
	dv = (rvn_diff_view_t*)widget;
	rvn_dv_ensure(dv);
	if (change < 0 || change >= (int)dv->change_count) return R_FALSE;
	c = &dv->changes[change];
	if (left_line) *left_line = c->left;
	if (left_count) *left_count = c->left_count;
	if (right_line) *right_line = c->right;
	if (right_count) *right_count = c->right_count;
	return R_TRUE;
}

void Rdiff_view_go_to_change(Rid id, int change) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DIFF_VIEW);
	rvn_dv_ensure((rvn_diff_view_t*)widget);
	if (change < 0 || change >= (int)((rvn_diff_view_t*)widget)->change_count) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rdiff_view_go_to_change: invalid change %d", change);
		return;
	}
	rvn_dv_go_to((rvn_diff_view_t*)widget, change);
}

int Rdiff_view_get_current_change(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_DIFF_VIEW, -1);
	rvn_dv_ensure((rvn_diff_view_t*)widget);
	return ((rvn_diff_view_t*)widget)->current;
}

int Rdiff_view_next_change(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_DIFF_VIEW, -1);
	return rvn_dv_step((rvn_diff_view_t*)widget, 1);
}

int Rdiff_view_prev_change(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_DIFF_VIEW, -1);
	return rvn_dv_step((rvn_diff_view_t*)widget, -1);
}

int Rdiff_view_get_row_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_DIFF_VIEW, 0);
	rvn_dv_ensure((rvn_diff_view_t*)widget);
	return (int)((rvn_diff_view_t*)widget)->row_count;
}

RDiffRowKind Rdiff_view_get_row(Rid id, int row, int* left_line, int* right_line) {
	rvn_diff_view_t* dv;
	const rvn_dv_row_t* r;
	int left = 0, right = 0;
	RDiffRowKind kind = R_DIFF_ROW_SAME;
	RVN_WIDGET(widget, id, R_WIDGET_DIFF_VIEW, R_DIFF_ROW_SAME);
	dv = (rvn_diff_view_t*)widget;
	rvn_dv_ensure(dv);
	if (left_line) *left_line = 0;
	if (right_line) *right_line = 0;
	if (row < 0 || row >= (int)dv->row_count) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rdiff_view_get_row: invalid row %d", row);
		return R_DIFF_ROW_SAME;
	}
	r = &dv->rows[row];
	switch (r->kind) {
		case RVN_DV_ROW_LINE: {
			const rvn_dv_entry_t* e = &dv->entries[r->a];
			left = e->left;
			right = e->right;
			kind = e->kind == RVN_DIFF_ADDED ? R_DIFF_ROW_ADDED : e->kind == RVN_DIFF_REMOVED ? R_DIFF_ROW_REMOVED : R_DIFF_ROW_SAME;
			break;
		}
		case RVN_DV_ROW_PAIR:
			if (r->a >= 0) left = dv->entries[r->a].left;
			if (r->b >= 0) right = dv->entries[r->b].right;
			if (r->a >= 0 && r->a == r->b) kind = R_DIFF_ROW_SAME;
			else if (r->a >= 0 && r->b >= 0) kind = R_DIFF_ROW_CHANGED;
			else kind = r->a >= 0 ? R_DIFF_ROW_REMOVED : R_DIFF_ROW_ADDED;
			break;
		case RVN_DV_ROW_FOLD:
			left = dv->entries[r->a].left;
			right = dv->entries[r->a].right;
			kind = R_DIFF_ROW_FOLD;
			break;
		case RVN_DV_ROW_HUNK: kind = R_DIFF_ROW_HUNK; break;
		case RVN_DV_ROW_FILE: kind = R_DIFF_ROW_FILE; break;
		case RVN_DV_ROW_NOTE:
			kind = R_DIFF_ROW_NOTE;
			if (dv->notes[r->a].side == R_DIFF_LEFT) left = dv->notes[r->a].line;
			else right = dv->notes[r->a].line;
			break;
		default: break;
	}
	if (left_line) *left_line = left;
	if (right_line) *right_line = right;
	return kind;
}

const char* Rdiff_view_get_row_text(Rid id, int row, RDiffSide side) {
	rvn_diff_view_t* dv;
	const rvn_dv_row_t* r;
	RVN_WIDGET(widget, id, R_WIDGET_DIFF_VIEW, "");
	dv = (rvn_diff_view_t*)widget;
	rvn_dv_ensure(dv);
	if (row < 0 || row >= (int)dv->row_count) return "";
	r = &dv->rows[row];
	if (r->kind == RVN_DV_ROW_LINE || r->kind == RVN_DV_ROW_PAIR) {
		int entry = rvn_dv_row_entry(dv, row, side == R_DIFF_LEFT ? R_DIFF_LEFT : R_DIFF_RIGHT), length;
		const char* text;
		dv->scratch.length = 0;
		if (dv->scratch.data) dv->scratch.data[0] = '\0';
		if (entry < 0) return "";
		text = rvn_dv_entry_text(dv, entry, side == R_DIFF_LEFT ? R_DIFF_LEFT : R_DIFF_RIGHT, r->kind == RVN_DV_ROW_LINE, &length);
		if (!text) return "";
		rvn_text_append(&dv->scratch, text, (size_t)length);
		if (!dv->scratch.data) rvn_text_add(&dv->scratch, "");
		return dv->scratch.data;
	}
	rvn_dv_row_label(dv, r, &dv->scratch);
	return dv->scratch.data;
}

int Rdiff_view_expand(Rid id, int row) {
	rvn_diff_view_t* dv;
	const rvn_dv_row_t* r;
	RVN_WIDGET(widget, id, R_WIDGET_DIFF_VIEW, R_FALSE);
	dv = (rvn_diff_view_t*)widget;
	rvn_dv_ensure(dv);
	if (row < 0 || row >= (int)dv->row_count || dv->rows[row].kind != RVN_DV_ROW_FOLD) return R_FALSE;
	r = &dv->rows[row];
	memset(dv->revealed + r->a, 1, (size_t)r->b);
	rvn_dv_changed(widget, R_FALSE);
	return R_TRUE;
}

int Rdiff_view_get_first_visible_row(Rid id) {
	rvn_diff_view_t* dv;
	rvn_dv_metrics_t m;
	RVN_WIDGET(widget, id, R_WIDGET_DIFF_VIEW, 0);
	dv = (rvn_diff_view_t*)widget;
	rvn_dv_ensure(dv);
	rvn_dv_metrics(dv, &m);
	return rvn_mini((int)floorf(dv->bar.offset / m.row + 0.001f), rvn_maxi((int)dv->row_count - 1, 0));
}

void Rdiff_view_scroll_to_row(Rid id, int row) {
	rvn_diff_view_t* dv;
	rvn_dv_metrics_t m;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DIFF_VIEW);
	dv = (rvn_diff_view_t*)widget;
	rvn_dv_ensure(dv);
	rvn_dv_metrics(dv, &m);
	rvn_scrollbar_set(&dv->bar, m.row * (float)rvn_maxi(row, 0));
	rvn_scrollbar_clamp(&dv->bar);
	rvn_widget_invalidate_paint(widget);
}

void Rdiff_view_add_annotation(Rid id, RDiffSide side, int line, const char* text) {
	rvn_diff_view_t* dv;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DIFF_VIEW);
	dv = (rvn_diff_view_t*)widget;
	if (side != R_DIFF_LEFT && side != R_DIFF_RIGHT) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rdiff_view_add_annotation: invalid side %d", (int)side);
		return;
	}
	RVN_ARRAY_RESERVE(dv->notes, dv->note_cap, dv->note_count + 1);
	dv->notes[dv->note_count].side = (int)side;
	dv->notes[dv->note_count].line = line;
	dv->notes[dv->note_count].text = rvn_strdup(text ? text : "");
	dv->note_count++;
	rvn_dv_changed(widget, R_FALSE);
}

void Rdiff_view_clear_annotations(Rid id) {
	rvn_diff_view_t* dv;
	uint32_t i;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DIFF_VIEW);
	dv = (rvn_diff_view_t*)widget;
	for (i = 0; i < dv->note_count; i++) rvn_free(dv->notes[i].text);
	dv->note_count = 0;
	rvn_dv_changed(widget, R_FALSE);
}

const char* Rdiff_view_get_selected_text(Rid id) {
	rvn_diff_view_t* dv;
	RVN_WIDGET(widget, id, R_WIDGET_DIFF_VIEW, "");
	dv = (rvn_diff_view_t*)widget;
	rvn_dv_ensure(dv);
	rvn_dv_selected_text(dv, &dv->scratch);
	return dv->scratch.data ? dv->scratch.data : "";
}

void Rdiff_view_set_line_clicked_callback(Rid id, RDiffViewLineCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_DIFF_VIEW);
	((rvn_diff_view_t*)widget)->callback = callback;
	((rvn_diff_view_t*)widget)->user_data = user_data;
}
