/**
 * Raven - widgets/text_editor.c
 * TextEditor: multi-line text editing.
 *
 * The text is an array of lines, each one a small UTF-8 buffer: inserting or
 * deleting touches one line (and moves pointers when lines are added), which
 * stays fast for texts of tens of thousands of lines.
 *
 * A position is (line, column), the column being a byte offset in the line.
 * With word wrap a line is shown on several rows; the places where it is cut
 * are computed once per line and per width, and kept until the line changes.
 * Only the rows that are visible are measured, colored and painted.
 *
 * Every change goes through one function, rvn_ed_replace_exact, which also records
 * what is needed to undo it.
 *
 * Search and replace count the matches of each line once per version of the
 * line, so that counting again after an edit only reads the lines that
 * changed; the find bar is drawn by the editor itself, over its text, and
 * its two fields are single-line editing models (textedit.c).
 */

#include "common.h"

#define RVN_ED_PAD_X 8.0f
#define RVN_ED_PAD_Y 6.0f
#define RVN_ED_MAX_SPANS 192
/** Characters typed in a row are undone together, up to this many. */
#define RVN_ED_TYPING_GROUP 48
#define RVN_ED_MAX_UNDO 512

typedef struct rvn_ed_pos_t {
	int line;
	int column;
} rvn_ed_pos_t;

typedef struct rvn_ed_line_t {
	char* text;
	int length;
	int cap;
	/** Width of the line on one row, or a negative value when it is not known. */
	float width;
	/** Word wrap: number of rows (0 = not computed) and where the rows after the first begin. */
	int row_count;
	int* breaks;
	/** What the lexer is in at the start of the line (syntax colors). */
	uint8_t lex_in;
	/** Number of matches of the search in the line, counted for the search whose stamp is search_stamp (0: not counted). */
	uint32_t search_stamp;
	int search_count;
} rvn_ed_line_t;

/** A part of a line underlined with a message (Rtext_editor_add_marker). */
typedef struct rvn_ed_marker_t {
	int line;
	int from;
	/** Negative: to the end of the line. */
	int to;
	RMarkerKind kind;
	char* message;
} rvn_ed_marker_t;

/** One step of the history: `removed` was replaced by `inserted` at `start`. */
typedef struct rvn_ed_edit_t {
	rvn_ed_pos_t start;
	char* removed;
	int removed_length;
	char* inserted;
	int inserted_length;
	rvn_ed_pos_t caret_before, anchor_before, caret_after;
	/** A run of typed characters that the next one may join. */
	int typing;
} rvn_ed_edit_t;

/** What is searched for (Rtext_editor_set_search). */
typedef struct rvn_ed_search_t {
	char* query;
	int length;
	Ruint flags;
	/** Without R_SEARCH_MATCH_CASE: the characters of the query, in lower case. */
	uint32_t* folded;
	int folded_count;
	/** The query starts / ends with a character of a word: R_SEARCH_WHOLE_WORD looks at what is beside it. */
	int word_start;
	int word_end;
	/** Changes with the query and the options: what was counted for another search is not used. Never 0. */
	uint32_t stamp;
	/** Number of matches of the whole text, counted for (count_version, count_stamp). */
	uint32_t count_version;
	uint32_t count_stamp;
	int count;
	/** R_TRUE while the matches are not shown (the find bar was closed). */
	int hidden;
} rvn_ed_search_t;

typedef struct rvn_text_editor_t {
	rvn_widget_t base;
	rvn_ed_line_t* lines;
	uint32_t line_count;
	uint32_t line_cap;

	/** The selection is the range between the two; the caret is where typing goes. */
	rvn_ed_pos_t caret;
	rvn_ed_pos_t anchor;
	/** Horizontal position kept while moving up and down; negative = take it from the caret. */
	float goal_x;

	int read_only;
	int wrap;
	int line_numbers;
	int tab_size;
	int tab_spaces;
	/** Name of the language, or NULL; what it was found to be, and when it was looked for. */
	char* language_name;
	const rvn_language_t* language;
	uint32_t language_generation;
	int no_pairs;
	int no_completion;
	/** Counts the changes of the text: what was computed for a version is forgotten with it. */
	uint32_t version;
	/** The bracket next to the caret and the one that closes it, found for (match_version, match_caret). */
	uint32_t match_version;
	rvn_ed_pos_t match_caret;
	int match_found;
	rvn_ed_pos_t match_a, match_b;
	/** Parts of the text underlined with a message. */
	rvn_ed_marker_t* markers;
	uint32_t marker_count;
	uint32_t marker_cap;
	/** The tooltip of the widget is the message of a marker. */
	int marker_tip;
	/** The list of proposals (a popup of the window), created on first use. */
	rvn_widget_t* complete;
	RCompletionCallback completion_callback;
	void* completion_user;
	/** R_TRUE: the proposals are only the application's (Rtext_editor_set_completion_words). */
	int no_completion_words;
	/** Text for the pointer resting on the text where no marker is (Rtext_editor_set_hover_callback). */
	RTextEditorHoverCallback hover_callback;
	void* hover_user;
	Rid font;
	float font_size;
	/** Zoom of the text, a factor of the font size (0: 1). Ctrl + wheel, Ctrl + '+' / '-' / 0. */
	float zoom;
	int no_zoom;
	RTextEditorZoomCallback zoom_callback;
	void* zoom_user;
	char* placeholder;

	/** The search, and the find bar drawn over the text (0: closed, 1: find, 2: find and replace). */
	rvn_ed_search_t search;
	int find_open;
	/** Where the keys go: 0 the text, 1 the field of the search, 2 the field of the replacement. */
	int find_focus;
	/** R_TRUE: no find bar and none of its keys (Rtext_editor_set_find_bar). */
	int no_find_bar;
	/** The two fields, made on first use; the part of the bar under the pointer; a field being selected with the mouse. */
	int find_ready;
	rvn_textedit_t find_query;
	rvn_textedit_t find_replace;
	int find_hover;
	int find_dragging;
	/** Where the selection was when the bar opened: typing in the field searches from there. */
	rvn_ed_pos_t find_origin;

	rvn_scrollbar_t vbar;
	rvn_scrollbar_t hbar;
	/** Word wrap: width the rows were cut for, and the first row of each line. */
	float wrap_width;
	uint32_t* row_starts;
	uint32_t row_starts_cap;
	uint32_t total_rows;
	int rows_dirty;
	/** Without word wrap: width of the longest line. */
	float max_width;
	/** Lines whose lex_in is up to date (always a prefix of the text). */
	uint32_t lex_valid;
	/** The caret must be scrolled into view at the next layout. */
	int reveal_pending;

	rvn_ed_edit_t* undo;
	uint32_t undo_count;
	uint32_t undo_cap;
	rvn_ed_edit_t* redo;
	uint32_t redo_count;
	uint32_t redo_cap;

	int selecting;
	Rid blink_timer;
	double blink_start;
	/** Where the caret is drawn, in the coordinates of the text: it glides to its place. */
	RSpring caret_x;
	RSpring caret_y;

	/** Results of Rtext_editor_get_text / get_selected_text. */
	char* text_cache;
	int text_cache_length;
	int text_cache_valid;
	char* selection_cache;

	RTextEditorChangedCallback callback;
	void* user_data;
} rvn_text_editor_t;

/** What is needed to measure and place text, computed once per function. */
typedef struct rvn_ed_metrics_t {
	rvn_font_t* font;
	Rid font_id;
	float size;
	float line_height;
	float space;
	float tab;
	/** Width of the margin of the line numbers, 0 without it. */
	float gutter;
	/** Where the text is drawn, in the widget. */
	rvn_rect_t area;
	float bar;
	/** Theme of the editor: the colors of the code come from it. */
	const rvn_theme_t* theme;
	/** The bold and the italic of the font, for formatted text; the font itself otherwise. */
	Rid bold_id;
	Rid italic_id;
} rvn_ed_metrics_t;

/* ---------------------------------------------------------------------------
 *  Positions
 * ------------------------------------------------------------------------- */

static rvn_ed_pos_t rvn_ed_pos(int line, int column) {
	rvn_ed_pos_t pos;
	pos.line = line;
	pos.column = column;
	return pos;
}

static int rvn_ed_pos_cmp(rvn_ed_pos_t a, rvn_ed_pos_t b) {
	if (a.line != b.line) return a.line < b.line ? -1 : 1;
	if (a.column != b.column) return a.column < b.column ? -1 : 1;
	return 0;
}

static rvn_ed_pos_t rvn_ed_clamp(const rvn_text_editor_t* ed, rvn_ed_pos_t pos) {
	pos.line = rvn_clampi(pos.line, 0, (int)ed->line_count - 1);
	pos.column = rvn_clampi(pos.column, 0, ed->lines[pos.line].length);
	/* never inside a character */
	while (pos.column > 0 && pos.column < ed->lines[pos.line].length &&
		((unsigned char)ed->lines[pos.line].text[pos.column] & 0xC0) == 0x80) pos.column--;
	return pos;
}

static rvn_ed_pos_t rvn_ed_sel_min(const rvn_text_editor_t* ed) {
	return rvn_ed_pos_cmp(ed->caret, ed->anchor) <= 0 ? ed->caret : ed->anchor;
}

static rvn_ed_pos_t rvn_ed_sel_max(const rvn_text_editor_t* ed) {
	return rvn_ed_pos_cmp(ed->caret, ed->anchor) <= 0 ? ed->anchor : ed->caret;
}

static int rvn_ed_has_selection(const rvn_text_editor_t* ed) {
	return rvn_ed_pos_cmp(ed->caret, ed->anchor) != 0;
}

static rvn_ed_pos_t rvn_ed_end(const rvn_text_editor_t* ed) {
	return rvn_ed_pos((int)ed->line_count - 1, ed->lines[ed->line_count - 1].length);
}

/* Position reached after inserting `text` at `start`, byte for byte (as rvn_ed_replace_exact does). */
static rvn_ed_pos_t rvn_ed_pos_after(rvn_ed_pos_t start, const char* text, int length) {
	int i, begin = 0;
	for (i = 0; i <= length; i++) {
		if (i == length || text[i] == '\n') {
			start.column += i - begin;
			if (i < length) {
				start.line++;
				start.column = 0;
			}
			begin = i + 1;
		}
	}
	return start;
}

/* ---------------------------------------------------------------------------
 *  Lines
 * ------------------------------------------------------------------------- */

static void rvn_ed_line_dirty(rvn_ed_line_t* line) {
	line->width = -1.0f;
	line->row_count = 0;
	line->search_stamp = 0;
}

static void rvn_ed_line_reserve(rvn_ed_line_t* line, int length) {
	if (length + 1 > line->cap) {
		int cap = line->cap ? line->cap : 16;
		while (cap < length + 1) cap *= 2;
		line->text = (char*)rvn_realloc(line->text, (size_t)cap);
		line->cap = cap;
	}
}

static void rvn_ed_line_set(rvn_ed_line_t* line, const char* text, int length) {
	rvn_ed_line_reserve(line, length);
	if (length > 0) memcpy(line->text, text, (size_t)length);
	line->text[length] = '\0';
	line->length = length;
	rvn_ed_line_dirty(line);
}

static void rvn_ed_line_free(rvn_ed_line_t* line) {
	rvn_free(line->text);
	rvn_free(line->breaks);
}

/* Opens `count` empty lines at `index`. */
static void rvn_ed_lines_insert(rvn_text_editor_t* ed, uint32_t index, uint32_t count) {
	uint32_t i;
	if (count == 0) return;
	RVN_ARRAY_RESERVE(ed->lines, ed->line_cap, ed->line_count + count);
	memmove(ed->lines + index + count, ed->lines + index, sizeof(rvn_ed_line_t) * (ed->line_count - index));
	memset(ed->lines + index, 0, sizeof(rvn_ed_line_t) * count);
	for (i = 0; i < count; i++) rvn_ed_line_set(&ed->lines[index + i], "", 0);
	ed->line_count += count;
}

static void rvn_ed_lines_remove(rvn_text_editor_t* ed, uint32_t index, uint32_t count) {
	uint32_t i;
	if (count == 0) return;
	for (i = 0; i < count; i++) rvn_ed_line_free(&ed->lines[index + i]);
	memmove(ed->lines + index, ed->lines + index + count, sizeof(rvn_ed_line_t) * (ed->line_count - index - count));
	ed->line_count -= count;
}

/* The text between two ordered positions, in a new block. */
static char* rvn_ed_range(const rvn_text_editor_t* ed, rvn_ed_pos_t from, rvn_ed_pos_t to, int* length_out) {
	size_t length = 0;
	char* out;
	char* write;
	int line;

	for (line = from.line; line <= to.line; line++) {
		int begin = line == from.line ? from.column : 0;
		int end = line == to.line ? to.column : ed->lines[line].length;
		length += (size_t)(end - begin) + (line < to.line ? 1u : 0u);
	}
	out = (char*)rvn_alloc(length + 1);
	write = out;
	for (line = from.line; line <= to.line; line++) {
		int begin = line == from.line ? from.column : 0;
		int end = line == to.line ? to.column : ed->lines[line].length;
		memcpy(write, ed->lines[line].text + begin, (size_t)(end - begin));
		write += end - begin;
		if (line < to.line) *write++ = '\n';
	}
	*write = '\0';
	if (length_out) *length_out = (int)length;
	return out;
}

/* ---------------------------------------------------------------------------
 *  History
 * ------------------------------------------------------------------------- */

static void rvn_ed_edit_free(rvn_ed_edit_t* edit) {
	rvn_free(edit->removed);
	rvn_free(edit->inserted);
}

static void rvn_ed_history_clear(rvn_ed_edit_t* edits, uint32_t* count) {
	uint32_t i;
	for (i = 0; i < *count; i++) rvn_ed_edit_free(&edits[i]);
	*count = 0;
}

static void rvn_ed_changed_state(rvn_text_editor_t* ed, int first_line) {
	ed->rows_dirty = R_TRUE;
	ed->text_cache_valid = R_FALSE;
	ed->version++;
	if ((uint32_t)first_line + 1 < ed->lex_valid) ed->lex_valid = (uint32_t)first_line + 1;
	rvn_widget_invalidate_paint(&ed->base);
}

/*
 * The one function that changes the text: replaces what lies between two
 * positions by `text` (either may be empty) and returns the position after
 * the inserted text. `record` stores the step in the history. The text is
 * inserted byte for byte, so that undo and redo give back exactly what was
 * there; new text goes through rvn_ed_replace, which cleans it first.
 */
static rvn_ed_pos_t rvn_ed_replace_exact(rvn_text_editor_t* ed, rvn_ed_pos_t from, rvn_ed_pos_t to,
	const char* text, int length, int record) {

	rvn_ed_pos_t end;
	rvn_ed_line_t* line;
	char* tail;
	int tail_length, pieces = 1, i, begin, index;

	from = rvn_ed_clamp(ed, from);
	to = rvn_ed_clamp(ed, to);
	if (rvn_ed_pos_cmp(from, to) > 0) {
		rvn_ed_pos_t swap = from;
		from = to;
		to = swap;
	}
	if (length < 0) length = (int)strlen(text);

	if (record) {
		rvn_ed_edit_t* last = ed->undo_count ? &ed->undo[ed->undo_count - 1] : NULL;
		int typed = length > 0 && rvn_ed_pos_cmp(from, to) == 0 && !memchr(text, '\n', (size_t)length) &&
			rvn_utf8_count(text, length) == 1;
		rvn_ed_history_clear(ed->redo, &ed->redo_count);
		if (typed && last && last->typing && last->removed_length == 0 && last->inserted_length < RVN_ED_TYPING_GROUP &&
			rvn_ed_pos_cmp(last->caret_after, from) == 0) {
			/* one more character of a run that is being typed: same step */
			last->inserted = (char*)rvn_realloc(last->inserted, (size_t)(last->inserted_length + length) + 1);
			memcpy(last->inserted + last->inserted_length, text, (size_t)length);
			last->inserted_length += length;
			last->inserted[last->inserted_length] = '\0';
			last->caret_after = rvn_ed_pos(from.line, from.column + length);
		} else {
			rvn_ed_edit_t* edit;
			if (ed->undo_count == RVN_ED_MAX_UNDO) {
				rvn_ed_edit_free(&ed->undo[0]);
				memmove(ed->undo, ed->undo + 1, sizeof(rvn_ed_edit_t) * (ed->undo_count - 1));
				ed->undo_count--;
			}
			RVN_ARRAY_RESERVE(ed->undo, ed->undo_cap, ed->undo_count + 1);
			edit = &ed->undo[ed->undo_count++];
			memset(edit, 0, sizeof(*edit));
			edit->start = from;
			edit->removed = rvn_ed_range(ed, from, to, &edit->removed_length);
			edit->inserted = rvn_strndup(text ? text : "", (size_t)length);
			edit->inserted_length = length;
			edit->caret_before = ed->caret;
			edit->anchor_before = ed->anchor;
			edit->caret_after = rvn_ed_pos_after(from, text, length);
			edit->typing = typed;
		}
	}

	for (i = 0; i < length; i++) {
		if (text[i] == '\n') pieces++;
	}
	/* the markers below follow their lines; those of the lines that change are out of date */
	{
		int delta = (pieces - 1) - (to.line - from.line);
		uint32_t m = 0;
		while (m < ed->marker_count) {
			rvn_ed_marker_t* marker = &ed->markers[m];
			if (marker->line > to.line) {
				marker->line += delta;
				m++;
			} else if (marker->line >= from.line) {
				rvn_free(marker->message);
				*marker = ed->markers[--ed->marker_count];
			} else {
				m++;
			}
		}
	}

	/* what follows the range on its last line is kept aside, and put back after the new text */
	tail_length = ed->lines[to.line].length - to.column;
	tail = rvn_strndup(ed->lines[to.line].text + to.column, (size_t)tail_length);
	rvn_ed_lines_remove(ed, (uint32_t)from.line + 1, (uint32_t)(to.line - from.line));
	line = &ed->lines[from.line];
	line->length = from.column;
	line->text[line->length] = '\0';

	rvn_ed_lines_insert(ed, (uint32_t)from.line + 1, (uint32_t)(pieces - 1));

	/* the pieces between line breaks */
	begin = 0;
	index = from.line;
	end = from;
	for (i = 0; i <= length; i++) {
		if (i == length || text[i] == '\n') {
			int piece = i - begin;
			line = &ed->lines[index];
			rvn_ed_line_reserve(line, line->length + piece + (i == length ? tail_length : 0));
			if (piece > 0) memcpy(line->text + line->length, text + begin, (size_t)piece);
			line->length += piece;
			line->text[line->length] = '\0';
			rvn_ed_line_dirty(line);
			end = rvn_ed_pos(index, line->length);
			begin = i + 1;
			index++;
		}
	}
	line = &ed->lines[end.line];
	if (tail_length > 0) memcpy(line->text + line->length, tail, (size_t)tail_length);
	line->length += tail_length;
	line->text[line->length] = '\0';
	rvn_free(tail);

	rvn_ed_changed_state(ed, from.line);
	return end;
}

/*
 * Replaces a range by new text, as rvn_ed_replace_exact does, after dropping
 * the carriage returns that end each line of the text (CRLF line breaks).
 */
static rvn_ed_pos_t rvn_ed_replace(rvn_text_editor_t* ed, rvn_ed_pos_t from, rvn_ed_pos_t to,
	const char* text, int length, int record) {

	rvn_ed_pos_t end;
	char* clean = NULL;

	if (!text) text = "";
	if (length < 0) length = (int)strlen(text);
	if (length > 0 && memchr(text, '\r', (size_t)length)) {
		int i, begin = 0, out = 0;
		clean = (char*)rvn_calloc((size_t)length + 1);
		for (i = 0; i <= length; i++) {
			if (i == length || text[i] == '\n') {
				int piece = i - begin;
				while (piece > 0 && text[begin + piece - 1] == '\r') piece--;
				if (piece > 0) memcpy(clean + out, text + begin, (size_t)piece);
				out += piece;
				if (i < length) clean[out++] = '\n';
				begin = i + 1;
			}
		}
		text = clean;
		length = out;
	}
	end = rvn_ed_replace_exact(ed, from, to, text, length, record);
	rvn_free(clean);
	return end;
}

/* Tells the application that the user changed the text. */
static void rvn_ed_notify(rvn_widget_t* widget) {
	rvn_text_editor_t* ed = (rvn_text_editor_t*)widget;
	if (ed->callback) ed->callback(widget->id, ed->user_data);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

static void rvn_ed_break_typing(rvn_text_editor_t* ed) {
	if (ed->undo_count) ed->undo[ed->undo_count - 1].typing = R_FALSE;
}

static void rvn_ed_move_edit(rvn_ed_edit_t** to, uint32_t* to_count, uint32_t* to_cap, rvn_ed_edit_t* from, uint32_t* from_count) {
	RVN_ARRAY_RESERVE(*to, *to_cap, *to_count + 1);
	(*to)[(*to_count)++] = from[--(*from_count)];
}

static int rvn_ed_undo(rvn_text_editor_t* ed) {
	rvn_ed_edit_t* edit;
	if (ed->undo_count == 0 || ed->read_only) return R_FALSE;
	edit = &ed->undo[ed->undo_count - 1];
	edit->typing = R_FALSE;
	rvn_ed_replace_exact(ed, edit->start, rvn_ed_pos_after(edit->start, edit->inserted, edit->inserted_length),
		edit->removed, edit->removed_length, R_FALSE);
	ed->caret = rvn_ed_clamp(ed, edit->caret_before);
	ed->anchor = rvn_ed_clamp(ed, edit->anchor_before);
	rvn_ed_move_edit(&ed->redo, &ed->redo_count, &ed->redo_cap, ed->undo, &ed->undo_count);
	return R_TRUE;
}

static int rvn_ed_redo(rvn_text_editor_t* ed) {
	rvn_ed_edit_t* edit;
	if (ed->redo_count == 0 || ed->read_only) return R_FALSE;
	edit = &ed->redo[ed->redo_count - 1];
	rvn_ed_replace_exact(ed, edit->start, rvn_ed_pos_after(edit->start, edit->removed, edit->removed_length),
		edit->inserted, edit->inserted_length, R_FALSE);
	ed->caret = ed->anchor = rvn_ed_clamp(ed, edit->caret_after);
	rvn_ed_move_edit(&ed->undo, &ed->undo_count, &ed->undo_cap, ed->redo, &ed->redo_count);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Metrics and horizontal positions
 * ------------------------------------------------------------------------- */

static Rid rvn_ed_font_id(const rvn_text_editor_t* ed) {
	if (ed->font != R_INVALID_ID && Rfont_is_valid(ed->font)) return ed->font;
	/* text that has a language is code: it lines up better with a fixed width */
	return ed->language_name ? g_rvn.mono_font : R_INVALID_ID;
}

/* The language of the editor, looked for again when the languages changed. */
static const rvn_language_t* rvn_ed_language(rvn_text_editor_t* ed) {
	if (ed->language_generation != rvn_language_generation()) {
		ed->language = ed->language_name ? rvn_language_find(ed->language_name) : NULL;
		ed->language_generation = rvn_language_generation();
		ed->lex_valid = 0;
	}
	return ed->language;
}

static const RLanguageDesc* rvn_ed_language_desc(rvn_text_editor_t* ed) {
	const rvn_language_t* language = rvn_ed_language(ed);
	return language ? rvn_language_desc(language) : NULL;
}

static void rvn_ed_metrics(const rvn_text_editor_t* ed, rvn_ed_metrics_t* m) {
	const rvn_widget_t* widget = &ed->base;
	rvn_font_metrics_t font_metrics;
	float digit;
	int digits = 1;
	uint32_t count;

	m->theme = rvn_widget_theme(widget);
	m->font_id = rvn_ed_font_id(ed);
	m->font = rvn_font_resolve(m->font_id);
	m->bold_id = m->italic_id = m->font_id;
	{
		/* (the styles of a font are made when first asked for, here: the editor is measured before it is painted) */
		const rvn_language_t* language = rvn_ed_language((rvn_text_editor_t*)(uintptr_t)ed);
		if (language && (rvn_language_desc(language)->flags & R_LANGUAGE_FORMATTED_TEXT)) {
			m->bold_id = rvn_font_variant(m->font_id, RVN_FONT_BOLD);
			m->italic_id = rvn_font_variant(m->font_id, RVN_FONT_ITALIC);
		}
	}
	m->size = rvn_widget_font_size(widget, ed->font_size) * (ed->zoom > 0.0f ? ed->zoom : 1.0f);
	rvn_font_metrics(m->font, m->size, &font_metrics);
	m->line_height = ceilf(font_metrics.line_height + 2.0f);
	m->space = rvn_text_width(m->font, m->size, " ", 1);
	if (m->space <= 0.0f) m->space = m->size * 0.3f;
	m->tab = m->space * (float)rvn_maxi(ed->tab_size, 1);
	m->bar = rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE);

	m->gutter = 0.0f;
	if (ed->line_numbers) {
		digit = rvn_text_width(m->font, m->size, "0", 1);
		for (count = ed->line_count; count >= 10; count /= 10) digits++;
		m->gutter = ceilf(digit * (float)rvn_maxi(digits, 2) + 18.0f);
	}
	m->area.x = m->gutter + RVN_ED_PAD_X;
	m->area.y = RVN_ED_PAD_Y;
	/* with word wrap the room of the scrollbar is always kept: the text does not flow again when it appears */
	m->area.w = rvn_maxf(0.0f, widget->rect.w - m->area.x - RVN_ED_PAD_X -
		((ed->wrap || rvn_scrollbar_needed(&ed->vbar)) ? m->bar : 0.0f));
	m->area.h = rvn_maxf(0.0f, widget->rect.h - RVN_ED_PAD_Y * 2.0f -
		((!ed->wrap && rvn_scrollbar_needed(&ed->hbar)) ? m->bar : 0.0f));
}

/* Horizontal position reached after text[from..to), starting at `x` on the row: tabs jump to the next stop. */
static float rvn_ed_advance(const rvn_ed_metrics_t* m, const char* text, int from, int to, float x) {
	int i, run = from;
	for (i = from; i < to; i++) {
		if (text[i] != '\t') continue;
		if (i > run) x += rvn_text_width(m->font, m->size, text + run, i - run);
		x = (floorf(x / m->tab + 0.001f) + 1.0f) * m->tab;
		run = i + 1;
	}
	if (to > run) x += rvn_text_width(m->font, m->size, text + run, to - run);
	return x;
}

/* Byte offset in text[from..to) nearest to the horizontal position `target`. */
static int rvn_ed_offset_at(const rvn_ed_metrics_t* m, const char* text, int from, int to, float target) {
	float x = 0.0f;
	int i = from;
	while (i < to) {
		int next = rvn_utf8_next(text, to, i);
		float after = rvn_ed_advance(m, text, i, next, x);
		if (target < (x + after) * 0.5f) return i;
		x = after;
		i = next;
	}
	return to;
}

/* ---------------------------------------------------------------------------
 *  Rows (word wrap)
 * ------------------------------------------------------------------------- */

/* Cuts one line into rows no wider than `width`, at spaces when it can. */
static void rvn_ed_wrap_line(const rvn_ed_metrics_t* m, rvn_ed_line_t* line, float width) {
	int breaks_cap = 0, count = 0, i = 0, row_start = 0, last_space = -1;
	float x = 0.0f;
	int* breaks = NULL;

	while (i < line->length) {
		int next = rvn_utf8_next(line->text, line->length, i);
		float after = rvn_ed_advance(m, line->text, i, next, x);
		if (after > width && i > row_start) {
			int cut = last_space > row_start ? last_space : i;
			if (count + 1 > breaks_cap) {
				breaks_cap = breaks_cap ? breaks_cap * 2 : 4;
				breaks = (int*)rvn_realloc(breaks, sizeof(int) * (size_t)breaks_cap);
			}
			breaks[count++] = cut;
			row_start = cut;
			last_space = -1;
			x = rvn_ed_advance(m, line->text, cut, i, 0.0f);
			continue;
		}
		if (line->text[i] == ' ' || line->text[i] == '\t') last_space = next;
		x = after;
		i = next;
	}
	rvn_free(line->breaks);
	line->breaks = breaks;
	line->row_count = count + 1;
}

/* Bytes of a line shown on one of its rows. */
static void rvn_ed_row_range(const rvn_text_editor_t* ed, int line, int row, int* begin, int* end) {
	const rvn_ed_line_t* l = &ed->lines[line];
	if (!ed->wrap || l->row_count <= 1) {
		*begin = 0;
		*end = l->length;
		return;
	}
	*begin = row > 0 ? l->breaks[row - 1] : 0;
	*end = row + 1 < l->row_count ? l->breaks[row] : l->length;
}

/* Row of a line that shows a column: a column on a cut is at the start of the row after it. */
static int rvn_ed_row_of(const rvn_text_editor_t* ed, int line, int column) {
	const rvn_ed_line_t* l = &ed->lines[line];
	int row = 0;
	if (!ed->wrap || l->row_count <= 1) return 0;
	while (row + 1 < l->row_count && l->breaks[row] <= column) row++;
	return row;
}

/* Brings the row tables and the size of the content up to date. */
static void rvn_ed_update_rows(rvn_text_editor_t* ed, const rvn_ed_metrics_t* m) {
	uint32_t i, rows = 0;
	float widest = 0.0f;

	if (ed->wrap && ed->wrap_width != m->area.w) {
		ed->wrap_width = m->area.w;
		for (i = 0; i < ed->line_count; i++) ed->lines[i].row_count = 0;
		ed->rows_dirty = R_TRUE;
	}
	if (!ed->rows_dirty) return;
	ed->rows_dirty = R_FALSE;

	if (ed->wrap) {
		RVN_ARRAY_RESERVE(ed->row_starts, ed->row_starts_cap, ed->line_count + 1);
		for (i = 0; i < ed->line_count; i++) {
			if (ed->lines[i].row_count == 0) rvn_ed_wrap_line(m, &ed->lines[i], rvn_maxf(ed->wrap_width, m->space * 4.0f));
			ed->row_starts[i] = rows;
			rows += (uint32_t)ed->lines[i].row_count;
		}
		ed->row_starts[ed->line_count] = rows;
		ed->total_rows = rows;
		ed->max_width = 0.0f;
	} else {
		for (i = 0; i < ed->line_count; i++) {
			rvn_ed_line_t* line = &ed->lines[i];
			if (line->width < 0.0f) line->width = rvn_ed_advance(m, line->text, 0, line->length, 0.0f);
			if (line->width > widest) widest = line->width;
		}
		ed->total_rows = ed->line_count;
		ed->max_width = widest;
	}
}

static uint32_t rvn_ed_first_row(const rvn_text_editor_t* ed, int line) {
	return ed->wrap ? ed->row_starts[line] : (uint32_t)line;
}

/* Line and row in the line of a row counted from the top of the text. */
static void rvn_ed_locate_row(const rvn_text_editor_t* ed, uint32_t global_row, int* line, int* row) {
	uint32_t low = 0, high = ed->line_count - 1;
	if (global_row >= ed->total_rows) global_row = ed->total_rows - 1;
	if (!ed->wrap) {
		*line = (int)global_row;
		*row = 0;
		return;
	}
	while (low < high) {
		uint32_t middle = (low + high + 1) / 2;
		if (ed->row_starts[middle] <= global_row) low = middle;
		else high = middle - 1;
	}
	*line = (int)low;
	*row = (int)(global_row - ed->row_starts[low]);
}

/* Where a position is drawn, in the coordinates of the text (before scrolling). */
static rvn_vec2_t rvn_ed_pos_point(const rvn_text_editor_t* ed, const rvn_ed_metrics_t* m, rvn_ed_pos_t pos) {
	int row = rvn_ed_row_of(ed, pos.line, pos.column), begin, end;
	rvn_ed_row_range(ed, pos.line, row, &begin, &end);
	return rvn_vec2(rvn_ed_advance(m, ed->lines[pos.line].text, begin, pos.column, 0.0f),
		(float)(rvn_ed_first_row(ed, pos.line) + (uint32_t)row) * m->line_height);
}

/* Position under a point given in the coordinates of the text. */
static rvn_ed_pos_t rvn_ed_point_pos(const rvn_text_editor_t* ed, const rvn_ed_metrics_t* m, float x, float y) {
	int line, row, begin, end, column;
	float global = floorf(y / m->line_height);
	if (global < 0.0f) global = 0.0f;
	if (global > (float)ed->total_rows) global = (float)ed->total_rows;
	rvn_ed_locate_row(ed, (uint32_t)global, &line, &row);
	rvn_ed_row_range(ed, line, row, &begin, &end);
	column = rvn_ed_offset_at(m, ed->lines[line].text, begin, end, x);
	/* the end of a wrapped row is the position before the cut, not the start of the next row */
	if (ed->wrap && column == end && row + 1 < ed->lines[line].row_count && column > begin) {
		column = rvn_utf8_prev(ed->lines[line].text, column);
	}
	return rvn_ed_pos(line, column);
}

static void rvn_ed_update_bars(rvn_text_editor_t* ed, const rvn_ed_metrics_t* m) {
	ed->vbar.content = (float)ed->total_rows * m->line_height;
	ed->vbar.viewport = m->area.h;
	rvn_scrollbar_clamp(&ed->vbar);
	/* room for the caret after the longest line */
	ed->hbar.content = ed->wrap ? 0.0f : ed->max_width + m->space * 2.0f;
	ed->hbar.viewport = m->area.w;
	rvn_scrollbar_clamp(&ed->hbar);
}

/* Scrolls as little as needed to show the caret. Before the editor has its size, this waits for the layout. */
static void rvn_ed_reveal(rvn_text_editor_t* ed, int in_layout) {
	rvn_ed_metrics_t m;
	rvn_vec2_t point;
	if (!in_layout && (ed->base.rect.w <= 0.0f || ed->base.rect.h <= 0.0f || (ed->base.flags & RVN_WF_LAYOUT_DIRTY))) {
		ed->reveal_pending = R_TRUE;
		return;
	}
	rvn_ed_metrics(ed, &m);
	rvn_ed_update_rows(ed, &m);
	rvn_ed_update_bars(ed, &m);
	point = rvn_ed_pos_point(ed, &m, ed->caret);
	if (point.y < ed->vbar.offset) rvn_scrollbar_set(&ed->vbar, point.y);
	else if (point.y + m.line_height > ed->vbar.offset + ed->vbar.viewport) {
		rvn_scrollbar_set(&ed->vbar, point.y + m.line_height - ed->vbar.viewport);
	}
	if (!ed->wrap) {
		if (point.x < ed->hbar.offset + m.space) rvn_scrollbar_set(&ed->hbar, point.x - m.space * 4.0f);
		else if (point.x > ed->hbar.offset + ed->hbar.viewport - m.space * 2.0f) {
			rvn_scrollbar_set(&ed->hbar, point.x - ed->hbar.viewport + m.space * 6.0f);
		}
	}
	ed->reveal_pending = R_FALSE;
}

/* The caret moved: keep it visible and solid for a moment. */
static void rvn_ed_caret_moved(rvn_text_editor_t* ed, int keep_goal) {
	if (!keep_goal) ed->goal_x = -1.0f;
	ed->blink_start = rvn_clock_seconds();
	rvn_ed_reveal(ed, R_FALSE);
	rvn_widget_invalidate_paint(&ed->base);
}

/* ---------------------------------------------------------------------------
 *  Layout
 * ------------------------------------------------------------------------- */

static rvn_vec2_t rvn_text_editor_measure(rvn_widget_t* widget) {
	rvn_text_editor_t* ed = (rvn_text_editor_t*)widget;
	rvn_ed_metrics_t m;
	rvn_ed_metrics(ed, &m);
	/* an editor is given its size by its parent; by itself it asks for a few lines */
	return rvn_vec2(220.0f, m.line_height * 5.0f + RVN_ED_PAD_Y * 2.0f);
}

static void rvn_text_editor_arrange(rvn_widget_t* widget) {
	rvn_text_editor_t* ed = (rvn_text_editor_t*)widget;
	rvn_ed_metrics_t m;
	int pass;
	/* the scrollbars take room, which changes what is needed: twice is enough to settle */
	for (pass = 0; pass < 2; pass++) {
		rvn_ed_metrics(ed, &m);
		rvn_ed_update_rows(ed, &m);
		rvn_ed_update_bars(ed, &m);
	}
	if (ed->reveal_pending) rvn_ed_reveal(ed, R_TRUE);
}

static void rvn_ed_tracks(const rvn_text_editor_t* ed, const rvn_ed_metrics_t* m, rvn_rect_t* vertical, rvn_rect_t* horizontal) {
	const rvn_widget_t* widget = &ed->base;
	int h_needed = !ed->wrap && rvn_scrollbar_needed(&ed->hbar);
	*vertical = rvn_rect(widget->rect.w - m->bar - 1.0f, 1.0f, m->bar, widget->rect.h - 2.0f - (h_needed ? m->bar : 0.0f));
	*horizontal = rvn_rect(m->gutter + 1.0f, widget->rect.h - m->bar - 1.0f,
		widget->rect.w - m->gutter - 2.0f - (rvn_scrollbar_needed(&ed->vbar) ? m->bar : 0.0f), m->bar);
}

/* ---------------------------------------------------------------------------
 *  Paint
 * ------------------------------------------------------------------------- */

/* Makes the lexer state of the lines up to `line` valid. */
static void rvn_ed_lex_until(rvn_text_editor_t* ed, uint32_t line) {
	const rvn_language_t* language = rvn_ed_language(ed);
	if (ed->lex_valid == 0) {
		ed->lines[0].lex_in = 0;
		ed->lex_valid = 1;
	}
	while (ed->lex_valid <= line && ed->lex_valid < ed->line_count) {
		const rvn_ed_line_t* previous = &ed->lines[ed->lex_valid - 1];
		ed->lines[ed->lex_valid].lex_in = rvn_syntax_line(language, previous->text, previous->length, previous->lex_in, NULL, 0, NULL);
		ed->lex_valid++;
	}
}

/* ---------------------------------------------------------------------------
 *  Brackets
 * ------------------------------------------------------------------------- */

/* R_TRUE when a column of a line is code: not inside a comment or a text. */
static int rvn_ed_is_code(rvn_text_editor_t* ed, int line, int column) {
	const rvn_language_t* language = rvn_ed_language(ed);
	rvn_span_t spans[RVN_ED_MAX_SPANS];
	int count = 0, i;
	if (!language) return R_TRUE;
	rvn_ed_lex_until(ed, (uint32_t)line);
	rvn_syntax_line(language, ed->lines[line].text, ed->lines[line].length, ed->lines[line].lex_in, spans, RVN_ED_MAX_SPANS, &count);
	for (i = 0; i < count; i++) {
		if (column >= spans[i].start && column < spans[i].end) {
			return spans[i].token != RVN_TOKEN_COMMENT && spans[i].token != RVN_TOKEN_STRING;
		}
	}
	return R_TRUE;
}

/* The bracket that closes (or opens) the one at `at`. Looks no further than a few thousand lines. */
static int rvn_ed_match_bracket(rvn_text_editor_t* ed, rvn_ed_pos_t at, rvn_ed_pos_t* match) {
	static const char open[] = "([{", close[] = ")]}";
	const rvn_language_t* language = rvn_ed_language(ed);
	const rvn_ed_line_t* l = &ed->lines[at.line];
	rvn_span_t spans[RVN_ED_MAX_SPANS];
	const char* found;
	char bracket, other;
	int forward, depth = 1, line = at.line, column, budget = 3000, span_count = 0, i;

	if (at.column < 0 || at.column >= l->length) return R_FALSE;
	bracket = l->text[at.column];
	if ((found = strchr(open, bracket)) != NULL && bracket) {
		forward = R_TRUE;
		other = close[found - open];
	} else if ((found = strchr(close, bracket)) != NULL && bracket) {
		forward = R_FALSE;
		other = open[found - close];
	} else {
		return R_FALSE;
	}
	if (!rvn_ed_is_code(ed, at.line, at.column)) return R_FALSE;

	column = at.column + (forward ? 1 : -1);
	while (line >= 0 && line < (int)ed->line_count && budget-- > 0) {
		l = &ed->lines[line];
		if (language) {
			rvn_ed_lex_until(ed, (uint32_t)line);
			rvn_syntax_line(language, l->text, l->length, l->lex_in, spans, RVN_ED_MAX_SPANS, &span_count);
		}
		for (; column >= 0 && column < l->length; column += forward ? 1 : -1) {
			char c = l->text[column];
			int code = R_TRUE;
			if (c != bracket && c != other) continue;
			for (i = 0; i < span_count && code; i++) {
				if (column >= spans[i].start && column < spans[i].end) {
					code = spans[i].token != RVN_TOKEN_COMMENT && spans[i].token != RVN_TOKEN_STRING;
				}
			}
			if (!code) continue;
			depth += c == bracket ? 1 : -1;
			if (depth == 0) {
				*match = rvn_ed_pos(line, column);
				return R_TRUE;
			}
		}
		line += forward ? 1 : -1;
		if (line >= 0 && line < (int)ed->line_count) column = forward ? 0 : ed->lines[line].length - 1;
	}
	return R_FALSE;
}

/* The pair of brackets to show for the place of the caret: the one after it, else the one before it. */
static void rvn_ed_update_match(rvn_text_editor_t* ed) {
	rvn_ed_pos_t at = ed->caret;
	if (ed->match_version == ed->version && rvn_ed_pos_cmp(ed->match_caret, ed->caret) == 0) return;
	ed->match_version = ed->version;
	ed->match_caret = ed->caret;
	ed->match_found = rvn_ed_match_bracket(ed, at, &ed->match_b);
	if (!ed->match_found && at.column > 0) {
		at.column--;
		ed->match_found = rvn_ed_match_bracket(ed, at, &ed->match_b);
	}
	ed->match_a = at;
}

/* Draws text[begin..end) of a line at (x, y), tabs expanded, each span in its color. */
static void rvn_ed_paint_text(RPainter* painter, const rvn_ed_metrics_t* m, const char* text, int begin, int end,
	float x, float y, const rvn_span_t* spans, int span_count, rvn_color_t color) {

	float pen = 0.0f;
	int i = begin, span = 0;

	while (i < end) {
		int stop = end;
		rvn_color_t run_color = color;
		Rid font = m->font_id;
		const char* tab;
		while (span < span_count && spans[span].end <= i) span++;
		if (span < span_count) {
			if (spans[span].start <= i) {
				rvn_token_t token = (rvn_token_t)spans[span].token;
				run_color = rvn_syntax_color(m->theme, token, color);
				/* formatted text: the styles have the advances of the font, so the columns do not move */
				if (token == RVN_TOKEN_STRONG || token == RVN_TOKEN_HEADING) font = m->bold_id;
				else if (token == RVN_TOKEN_EMPHASIS) font = m->italic_id;
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
		rvn_paint_text(painter, font, m->size, x + pen, y + 1.0f, text + i, stop - i, run_color);
		pen += rvn_text_width(m->font, m->size, text + i, stop - i);
		i = stop;
	}
}

static rvn_color_t rvn_ed_marker_color(RMarkerKind kind, const rvn_widget_t* widget) {
	switch (kind) {
		case R_MARKER_ERROR: return rvn_widget_color(widget, R_COLOR_DANGER);
		case R_MARKER_WARNING: return rvn_widget_color(widget, R_COLOR_WARNING);
		default: return rvn_widget_color(widget, R_COLOR_ACCENT);
	}
}

/* Search and replace, below. */
static int rvn_ed_find_has_keys(const rvn_text_editor_t* ed);
static void rvn_ed_paint_matches(rvn_text_editor_t* ed, RPainter* painter, const rvn_ed_metrics_t* m, int line, int begin, int end,
	float x, float y);
static void rvn_ed_find_paint(rvn_text_editor_t* ed, RPainter* painter, const rvn_ed_metrics_t* m);

static void rvn_text_editor_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_text_editor_t* ed = (rvn_text_editor_t*)widget;
	rvn_ed_metrics_t m;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	rvn_rect_t vertical, horizontal;
	rvn_vec2_t origin = rvn_widget_origin(widget);
	/* (the text has the keyboard; the frame shows the focus of the widget, which may be in the find bar) */
	int focused = (widget->flags & RVN_WF_FOCUSED) != 0 && !rvn_ed_find_has_keys(ed);
	int has_selection = rvn_ed_has_selection(ed);
	rvn_ed_pos_t sel_min = rvn_ed_sel_min(ed), sel_max = rvn_ed_sel_max(ed);
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	rvn_color_t selection = rvn_widget_color(widget, R_COLOR_SELECTION);
	rvn_color_t dim = rvn_widget_color(widget, R_COLOR_TEXT_PLACEHOLDER);
	float radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
	float focus = rvn_clampf(rvn_paint_spring(painter, &widget->press, (widget->flags & RVN_WF_FOCUSED) ? 1.0f : 0.0f, R_MOTION_SNAPPY), 0.0f, 1.0f);
	float scroll_x, scroll_y, y;
	uint32_t row, first_row, last_row, marker;
	rvn_span_t spans[RVN_ED_MAX_SPANS];
	int span_count = 0, span_line = -1;
	const rvn_language_t* language = rvn_ed_language(ed);

	/* the composition of an input method: shown at the caret (in place of the selection), not in the text */
	int compose_start = 0, compose_end = 0;
	const char* composition = focused && !ed->read_only ? rvn_widget_composition(widget, &compose_start, &compose_end) : NULL;
	uint32_t compose_row = 0;

	rvn_ed_metrics(ed, &m);
	rvn_ed_update_rows(ed, &m);
	if (!focused) selection = rvn_color_fade(selection, 0.55f);
	if (composition) {
		has_selection = R_FALSE;
		compose_row = rvn_ed_first_row(ed, ed->caret.line) + (uint32_t)rvn_ed_row_of(ed, ed->caret.line, ed->caret.column);
	}

	/* frame */
	if (rvn_widget_glass(widget)) {
		rvn_paint_surface(painter, widget, rect, rvn_minf(radius, 14.0f), RVN_SURFACE_INPUT, 0.0f, 0.0f);
		if (focus > 0.01f) {
			rvn_paint_box(painter, rect, rvn_minf(radius, 14.0f), 0, 1.5f, rvn_color_fade(rvn_widget_color(widget, R_COLOR_FOCUS), focus));
		}
	} else {
		rvn_paint_box(painter, rect, radius, rvn_widget_color(widget, R_COLOR_INPUT), rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH),
			rvn_color_mix(rvn_widget_color(widget, R_COLOR_BORDER), rvn_widget_color(widget, R_COLOR_FOCUS), focus));
	}

	scroll_y = rvn_scrollbar_shown(&ed->vbar, painter);
	scroll_x = ed->wrap ? 0.0f : rvn_scrollbar_shown(&ed->hbar, painter);
	/* whole pixels: the text stays sharp while it scrolls */
	scroll_y = floorf(scroll_y * painter->scale + 0.5f) / painter->scale;
	scroll_x = floorf(scroll_x * painter->scale + 0.5f) / painter->scale;

	first_row = scroll_y > 0.0f ? (uint32_t)(scroll_y / m.line_height) : 0;
	last_row = (uint32_t)rvn_maxf(0.0f, (scroll_y + m.area.h) / m.line_height) + 1;
	if (last_row > ed->total_rows) last_row = ed->total_rows;

	/* margin of the line numbers */
	if (ed->line_numbers) {
		rvn_paint_push_clip(painter, rvn_rect(1.0f, 1.0f, m.gutter, rect.h - 2.0f));
		rvn_paint_rect(painter, rvn_rect(m.gutter - 1.0f, 4.0f, 1.0f, rect.h - 8.0f), rvn_widget_color(widget, R_COLOR_SEPARATOR));
		for (row = first_row; row < last_row; row++) {
			int line, line_row;
			char number[16];
			rvn_ed_locate_row(ed, row, &line, &line_row);
			if (line_row != 0) continue;
			snprintf(number, sizeof(number), "%d", line + 1);
			rvn_paint_text_in(painter, m.font_id, m.size,
				rvn_rect(0.0f, m.area.y + (float)row * m.line_height - scroll_y, m.gutter - 9.0f, m.line_height), 1.0f, number, -1,
				line == ed->caret.line ? text : dim);
			/* a dot in the margin of the lines that have a marker */
			for (marker = 0; marker < ed->marker_count; marker++) {
				if (ed->markers[marker].line != line) continue;
				rvn_paint_box(painter, rvn_rect(4.0f, m.area.y + (float)row * m.line_height - scroll_y + m.line_height * 0.5f - 3.0f, 6.0f, 6.0f),
					3.0f, rvn_ed_marker_color(ed->markers[marker].kind, widget), 0.0f, 0);
				break;
			}
		}
		rvn_paint_pop_clip(painter);
	}

	rvn_paint_push_clip(painter, rvn_rect(m.gutter + 1.0f, 1.0f, rect.w - m.gutter - 2.0f, rect.h - 2.0f));

	if (ed->line_count == 1 && ed->lines[0].length == 0 && ed->placeholder) {
		rvn_paint_text(painter, m.font_id, m.size, m.area.x, m.area.y + 1.0f, ed->placeholder, -1, dim);
	}

	if (language && last_row > 0) {
		int line, line_row;
		rvn_ed_locate_row(ed, last_row - 1, &line, &line_row);
		rvn_ed_lex_until(ed, (uint32_t)line);
	}
	/* the bracket next to the caret and the one that goes with it */
	if (focused && !has_selection && language) rvn_ed_update_match(ed);
	else ed->match_found = R_FALSE;

	for (row = first_row; row < last_row; row++) {
		const rvn_ed_line_t* l;
		int line, line_row, begin, end;
		rvn_ed_locate_row(ed, row, &line, &line_row);
		rvn_ed_row_range(ed, line, line_row, &begin, &end);
		l = &ed->lines[line];
		y = m.area.y + (float)row * m.line_height - scroll_y;

		/* the line of the caret, lightly marked */
		if (focused && !has_selection && line == ed->caret.line && !ed->read_only) {
			rvn_paint_rect(painter, rvn_rect(m.gutter + 1.0f, y, rect.w - m.gutter - 2.0f, m.line_height),
				rvn_color_fade(rvn_widget_color(widget, R_COLOR_ROW_HOVERED), 0.6f));
		}
		/* the part of the selection that lies on this row */
		if (has_selection && line >= sel_min.line && line <= sel_max.line) {
			int from = line == sel_min.line ? rvn_maxi(sel_min.column, begin) : begin;
			int to = line == sel_max.line ? rvn_mini(sel_max.column, end) : end;
			if (from <= to && !(line == sel_max.line && sel_max.column < begin) && !(line == sel_min.line && sel_min.column > end)) {
				float x0 = rvn_ed_advance(&m, l->text, begin, from, 0.0f);
				float x1 = rvn_ed_advance(&m, l->text, from, to, x0);
				/* a selected line break shows as a little more than the text */
				if (line < sel_max.line && end == l->length) x1 += m.space;
				if (x1 > x0) {
					rvn_paint_box(painter, rvn_rect(m.area.x + x0 - scroll_x, y, x1 - x0, m.line_height), 3.0f, selection, 0.0f, 0);
				}
			}
		}
		/* the matches of the search on this row */
		rvn_ed_paint_matches(ed, painter, &m, line, begin, end, m.area.x - scroll_x, y);
		if (ed->match_found) {
			int which;
			for (which = 0; which < 2; which++) {
				rvn_ed_pos_t at = which ? ed->match_b : ed->match_a;
				float x0;
				if (at.line != line || at.column < begin || at.column >= end) continue;
				x0 = rvn_ed_advance(&m, l->text, begin, at.column, 0.0f);
				rvn_paint_box(painter, rvn_rect(m.area.x + x0 - scroll_x - 1.0f, y + 1.0f,
					rvn_ed_advance(&m, l->text, at.column, at.column + 1, x0) - x0 + 2.0f, m.line_height - 2.0f), 3.0f,
					rvn_color_fade(rvn_widget_color(widget, R_COLOR_ACCENT), 0.22f), 1.0f, rvn_color_fade(rvn_widget_color(widget, R_COLOR_ACCENT), 0.7f));
			}
		}
		if (end > begin && language && span_line != line) {
			rvn_syntax_line(language, l->text, l->length, l->lex_in, spans, RVN_ED_MAX_SPANS, &span_count);
			span_line = line;
		}
		if (composition && row == compose_row) {
			/* the row of the caret: its text before the caret, the composition, the rest moved after it */
			int at = rvn_clampi(ed->caret.column, begin, end);
			float x = m.area.x - scroll_x;
			float before = rvn_ed_advance(&m, l->text, begin, at, 0.0f);
			float width = rvn_text_width(m.font, m.size, composition, -1);
			if (at > begin) rvn_ed_paint_text(painter, &m, l->text, begin, at, x, y, spans, language ? span_count : 0, text);
			rvn_paint_text(painter, m.font_id, m.size, x + before, y, composition, -1, text);
			if (end > at) rvn_ed_paint_text(painter, &m, l->text, at, end, x + before + width, y, spans, language ? span_count : 0, text);
			rvn_paint_composition_underline(painter, x + before, x + before + width,
				x + before + rvn_text_width(m.font, m.size, composition, compose_start),
				x + before + rvn_text_width(m.font, m.size, composition, compose_end),
				y + m.line_height - 2.0f, text);
		} else if (end > begin) {
			rvn_ed_paint_text(painter, &m, l->text, begin, end, m.area.x - scroll_x, y,
				spans, language ? span_count : 0, text);
		}
		/* markers: a wavy line under what they are about */
		for (marker = 0; marker < ed->marker_count; marker++) {
			const rvn_ed_marker_t* mark = &ed->markers[marker];
			int from, to;
			float x0, x1, wave;
			rvn_color_t color;
			if (mark->line != line) continue;
			from = rvn_clampi(mark->from, begin, end);
			to = mark->to < 0 ? end : rvn_clampi(mark->to, begin, end);
			if (to < from || (mark->from >= end && end > begin && line_row + 1 < l->row_count)) continue;
			x0 = rvn_ed_advance(&m, l->text, begin, from, 0.0f);
			x1 = rvn_ed_advance(&m, l->text, from, to, x0);
			/* (something to see even for an empty range) */
			if (x1 - x0 < m.space * 1.5f) x1 = x0 + m.space * 1.5f;
			color = rvn_ed_marker_color(mark->kind, widget);
			for (wave = x0; wave < x1; wave += 4.0f) {
				float next = rvn_minf(wave + 4.0f, x1);
				int up = ((int)((wave - x0) / 4.0f) & 1) != 0;
				rvn_paint_line(painter, m.area.x + wave - scroll_x, y + m.line_height - (up ? 3.5f : 1.5f),
					m.area.x + next - scroll_x, y + m.line_height - (up ? 1.5f : 3.5f), 1.2f, color);
			}
		}
	}

	/* the caret glides to its place and blinks once it rests there */
	if (focused && !ed->read_only) {
		rvn_vec2_t point = rvn_ed_pos_point(ed, &m, ed->caret);
		float x;
		if (composition) point.x += rvn_text_width(m.font, m.size, composition, compose_end);
		x = rvn_paint_spring(painter, &ed->caret_x, point.x, R_MOTION_SNAPPY);
		float caret_y = rvn_paint_spring(painter, &ed->caret_y, point.y, R_MOTION_SNAPPY);
		int moving = rvn_spring_moving(&ed->caret_x, point.x) || rvn_spring_moving(&ed->caret_y, point.y);
		rvn_rect_t caret = rvn_rect(m.area.x + x - scroll_x - 1.0f, m.area.y + caret_y - scroll_y + 1.0f, 2.0f, m.line_height - 2.0f);
		rvn_text_input_rect(widget, painter, caret);
		if (moving || g_rvn.test_deterministic || fmod(rvn_clock_seconds() - ed->blink_start, 1.0) < 0.55) {
			rvn_paint_box(painter, caret,
				1.0f, rvn_widget_color(widget, R_COLOR_ACCENT_HOVERED), 0.0f, 0);
		}
	} else {
		ed->caret_x.started = R_FALSE;
		ed->caret_y.started = R_FALSE;
	}
	rvn_paint_pop_clip(painter);

	rvn_ed_tracks(ed, &m, &vertical, &horizontal);
	rvn_scrollbar_paint(&ed->vbar, widget, painter, vertical, R_TRUE,
		widget->window ? widget->window->mouse_x - origin.x : -1.0f, widget->window ? widget->window->mouse_y - origin.y : -1.0f);
	if (!ed->wrap) {
		rvn_scrollbar_paint(&ed->hbar, widget, painter, horizontal, R_FALSE,
			widget->window ? widget->window->mouse_x - origin.x : -1.0f, widget->window ? widget->window->mouse_y - origin.y : -1.0f);
	}
	rvn_ed_find_paint(ed, painter, &m);
}

/* ---------------------------------------------------------------------------
 *  Editing commands
 * ------------------------------------------------------------------------- */

typedef enum rvn_ed_class_t {
	RVN_ED_SPACE,
	RVN_ED_WORD,
	RVN_ED_PUNCTUATION
} rvn_ed_class_t;

static rvn_ed_class_t rvn_ed_class(char c) {
	if (c == ' ' || c == '\t') return RVN_ED_SPACE;
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || (unsigned char)c >= 0x80) return RVN_ED_WORD;
	return RVN_ED_PUNCTUATION;
}

static rvn_ed_pos_t rvn_ed_char_left(const rvn_text_editor_t* ed, rvn_ed_pos_t pos) {
	if (pos.column > 0) return rvn_ed_pos(pos.line, rvn_utf8_prev(ed->lines[pos.line].text, pos.column));
	if (pos.line > 0) return rvn_ed_pos(pos.line - 1, ed->lines[pos.line - 1].length);
	return pos;
}

static rvn_ed_pos_t rvn_ed_char_right(const rvn_text_editor_t* ed, rvn_ed_pos_t pos) {
	const rvn_ed_line_t* line = &ed->lines[pos.line];
	if (pos.column < line->length) return rvn_ed_pos(pos.line, rvn_utf8_next(line->text, line->length, pos.column));
	if (pos.line + 1 < (int)ed->line_count) return rvn_ed_pos(pos.line + 1, 0);
	return pos;
}

static rvn_ed_pos_t rvn_ed_word_left(const rvn_text_editor_t* ed, rvn_ed_pos_t pos) {
	const char* text = ed->lines[pos.line].text;
	rvn_ed_class_t kind;
	if (pos.column == 0) return rvn_ed_char_left(ed, pos);
	while (pos.column > 0 && rvn_ed_class(text[pos.column - 1]) == RVN_ED_SPACE) pos.column--;
	if (pos.column == 0) return pos;
	kind = rvn_ed_class(text[pos.column - 1]);
	while (pos.column > 0 && rvn_ed_class(text[pos.column - 1]) == kind) pos.column--;
	return pos;
}

static rvn_ed_pos_t rvn_ed_word_right(const rvn_text_editor_t* ed, rvn_ed_pos_t pos) {
	const rvn_ed_line_t* line = &ed->lines[pos.line];
	rvn_ed_class_t kind;
	if (pos.column >= line->length) return rvn_ed_char_right(ed, pos);
	kind = rvn_ed_class(line->text[pos.column]);
	while (pos.column < line->length && rvn_ed_class(line->text[pos.column]) == kind) pos.column++;
	while (pos.column < line->length && rvn_ed_class(line->text[pos.column]) == RVN_ED_SPACE) pos.column++;
	return pos;
}

/* Up or down by `rows` rows, keeping the horizontal position. */
static rvn_ed_pos_t rvn_ed_move_rows(rvn_text_editor_t* ed, const rvn_ed_metrics_t* m, rvn_ed_pos_t pos, int rows) {
	rvn_vec2_t point = rvn_ed_pos_point(ed, m, pos);
	int global = (int)(rvn_ed_first_row(ed, pos.line) + (uint32_t)rvn_ed_row_of(ed, pos.line, pos.column)) + rows;
	if (ed->goal_x < 0.0f) ed->goal_x = point.x;
	if (global < 0) return rvn_ed_pos(0, 0);
	if (global >= (int)ed->total_rows) return rvn_ed_end(ed);
	return rvn_ed_point_pos(ed, m, ed->goal_x, ((float)global + 0.5f) * m->line_height);
}

static void rvn_ed_copy(const rvn_text_editor_t* ed) {
	char* text;
	if (!rvn_ed_has_selection(ed)) return;
	text = rvn_ed_range(ed, rvn_ed_sel_min(ed), rvn_ed_sel_max(ed), NULL);
	Rclipboard_set_text(text);
	rvn_free(text);
}

/* Replaces the selection by a text, as one step of the history. */
static void rvn_ed_insert(rvn_text_editor_t* ed, const char* text, int length) {
	ed->caret = ed->anchor = rvn_ed_replace(ed, rvn_ed_sel_min(ed), rvn_ed_sel_max(ed), text, length, R_TRUE);
}

static int rvn_ed_delete_selection(rvn_text_editor_t* ed) {
	if (!rvn_ed_has_selection(ed)) return R_FALSE;
	rvn_ed_insert(ed, "", 0);
	return R_TRUE;
}

/* Tab and Shift+Tab over a selection of several lines: one level in or out for each of them. */
static void rvn_ed_indent_lines(rvn_text_editor_t* ed, int outdent) {
	rvn_ed_pos_t from = rvn_ed_sel_min(ed), to = rvn_ed_sel_max(ed);
	int first = from.line, last = to.line, line, length = 0, cap = 0;
	char* out = NULL;
	char unit[17];
	int unit_length = 1;

	/* a selection that ends at the start of a line does not include that line */
	if (last > first && to.column == 0) last--;
	unit[0] = '\t';
	if (ed->tab_spaces) {
		unit_length = rvn_clampi(ed->tab_size, 1, 16);
		memset(unit, ' ', (size_t)unit_length);
	}
	for (line = first; line <= last; line++) {
		const rvn_ed_line_t* l = &ed->lines[line];
		int skip = 0, add = 0;
		if (outdent) {
			if (l->length > 0 && l->text[0] == '\t') skip = 1;
			else while (skip < l->length && skip < ed->tab_size && l->text[skip] == ' ') skip++;
		} else if (l->length > 0) {
			add = unit_length;
		}
		if (length + add + (l->length - skip) + 2 > cap) {
			cap = (length + add + l->length + 2) * 2;
			out = (char*)rvn_realloc(out, (size_t)cap);
		}
		memcpy(out + length, unit, (size_t)add);
		length += add;
		memcpy(out + length, l->text + skip, (size_t)(l->length - skip));
		length += l->length - skip;
		if (line < last) out[length++] = '\n';
	}
	rvn_ed_break_typing(ed);
	rvn_ed_replace(ed, rvn_ed_pos(first, 0), rvn_ed_pos(last, ed->lines[last].length), out ? out : "", length, R_TRUE);
	rvn_ed_break_typing(ed);
	rvn_free(out);
	/* the lines stay selected, to indent again */
	ed->anchor = rvn_ed_pos(first, 0);
	ed->caret = last + 1 < (int)ed->line_count && to.column == 0 && to.line > first ? rvn_ed_pos(last + 1, 0)
		: rvn_ed_pos(last, ed->lines[last].length);
}

/* ---------------------------------------------------------------------------
 *  Help while typing: indentation, pairs, comments
 * ------------------------------------------------------------------------- */

/* One level of indentation, as the Tab key writes it. Returns its length. */
static int rvn_ed_indent_unit(const rvn_text_editor_t* ed, char unit[17]) {
	int length = 1;
	unit[0] = '\t';
	if (ed->tab_spaces) {
		length = rvn_clampi(ed->tab_size, 1, 16);
		memset(unit, ' ', (size_t)length);
	}
	unit[length] = '\0';
	return length;
}

static int rvn_ed_is_blank(char c) {
	return c == ' ' || c == '\t';
}

/* The character that closes an opening bracket or quote, or 0. */
static char rvn_ed_closer(char c) {
	switch (c) {
		case '(': return ')';
		case '[': return ']';
		case '{': return '}';
		case '"': return '"';
		case '\'': return '\'';
		case '`': return '`';
		default: return 0;
	}
}

/*
 * Enter: the new line starts with the indentation of the one it comes from,
 * one level deeper after an opening bracket (or a colon in Python, or an
 * opening tag). Between a bracket and the one that closes it, the closing
 * one goes down to a line of its own.
 */
static void rvn_ed_new_line(rvn_text_editor_t* ed) {
	rvn_ed_pos_t from = rvn_ed_sel_min(ed);
	const rvn_ed_line_t* line = &ed->lines[from.line];
	const RLanguageDesc* desc = ed->no_pairs ? NULL : rvn_ed_language_desc(ed);
	char buffer[320], unit[17];
	int indent = 0, length = 0, unit_length = rvn_ed_indent_unit(ed, unit);
	int deeper = R_FALSE, split = R_FALSE;

	while (indent < line->length && indent < from.column && indent < 128 && rvn_ed_is_blank(line->text[indent])) indent++;
	if (desc && !rvn_ed_has_selection(ed)) {
		int last = from.column;
		char before, after = from.column < line->length ? line->text[from.column] : '\0';
		while (last > 0 && rvn_ed_is_blank(line->text[last - 1])) last--;
		before = last > 0 ? line->text[last - 1] : '\0';
		if (before == '{' || before == '[' || before == '(') {
			deeper = R_TRUE;
			split = after != '\0' && after == rvn_ed_closer(before);
		} else if (before == ':' && (desc->flags & R_LANGUAGE_INDENT_ON_COLON)) {
			deeper = R_TRUE;
		} else if (before == '>' && (desc->flags & R_LANGUAGE_MARKUP) && last >= 2 && line->text[last - 2] != '/') {
			/* after an opening tag, not after a closing one */
			int open = last - 2;
			while (open > 0 && line->text[open] != '<') open--;
			if (line->text[open] == '<' && line->text[open + 1] != '/' && line->text[open + 1] != '!' && line->text[open + 1] != '?') {
				deeper = R_TRUE;
				split = after == '<' && from.column + 1 < line->length && line->text[from.column + 1] == '/';
			}
		}
	}
	buffer[length++] = '\n';
	memcpy(buffer + length, line->text, (size_t)indent);
	length += indent;
	if (deeper) {
		memcpy(buffer + length, unit, (size_t)unit_length);
		length += unit_length;
	}
	if (split) {
		buffer[length++] = '\n';
		memcpy(buffer + length, line->text, (size_t)indent);
		length += indent;
	}
	rvn_ed_break_typing(ed);
	rvn_ed_insert(ed, buffer, length);
	rvn_ed_break_typing(ed);
	if (split) ed->caret = ed->anchor = rvn_ed_pos(from.line + 1, indent + unit_length);
}

/* Is a quote character one that the language writes its texts with? */
static int rvn_ed_is_quote(const RLanguageDesc* desc, char c) {
	if (c != '"' && c != '\'' && c != '`') return R_FALSE;
	if (desc->flags & R_LANGUAGE_MARKUP) return c != '`';
	/* (an apostrophe is rarely the start of a text where it holds a single character) */
	if (c == '\'' && (desc->flags & R_LANGUAGE_CHAR_LITERALS)) return R_FALSE;
	if (desc->string_quotes && strchr(desc->string_quotes, c)) return R_TRUE;
	/* or one of the marks of its texts that span lines, when it is this character alone (a backquote) */
	if (desc->multiline_strings) {
		const char* mark = desc->multiline_strings;
		while (*mark) {
			if (mark[0] == c && (mark == desc->multiline_strings || mark[-1] == ' ') && (mark[1] == ' ' || mark[1] == '\0')) return R_TRUE;
			mark++;
		}
	}
	return R_FALSE;
}

/* Elements of HTML that have no closing tag. */
static int rvn_ed_is_void_tag(const char* name, int length) {
	static const char* const tags[] = { "br", "hr", "img", "input", "meta", "link", "area", "base", "col", "embed", "source", "track", "wbr" };
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(tags); i++) {
		if ((int)strlen(tags[i]) == length && rvn_strnicmp(tags[i], name, (size_t)length) == 0) return R_TRUE;
	}
	return R_FALSE;
}

/* A character typed by the user: with a language, brackets and quotes come in pairs. */
static void rvn_ed_type(rvn_text_editor_t* ed, uint32_t codepoint) {
	const RLanguageDesc* desc = ed->no_pairs ? NULL : rvn_ed_language_desc(ed);
	const rvn_ed_line_t* line = &ed->lines[ed->caret.line];
	int selection = rvn_ed_has_selection(ed);
	char encoded[4];
	int length = rvn_utf8_encode(codepoint, encoded);
	char c = codepoint < 128 ? (char)codepoint : '\0';
	char next = (!selection && ed->caret.column < line->length) ? line->text[ed->caret.column] : '\0';
	char previous = (!selection && ed->caret.column > 0) ? line->text[ed->caret.column - 1] : '\0';
	char closer = rvn_ed_closer(c);
	int is_quote = desc && rvn_ed_is_quote(desc, c);
	int room = next == '\0' || rvn_ed_is_blank(next) || strchr(")]}>,;", next) != NULL;

	if (!desc || c == '\0') {
		rvn_ed_insert(ed, encoded, length);
		return;
	}
	/* the closing character is already there: step over it */
	if (!selection && next == c && (c == ')' || c == ']' || c == '}' || is_quote)) {
		ed->caret = ed->anchor = rvn_ed_pos(ed->caret.line, ed->caret.column + 1);
		return;
	}
	if (closer && (is_quote || c == '(' || c == '[' || c == '{')) {
		if (selection) {
			/* around what is selected */
			char* inner = rvn_ed_range(ed, rvn_ed_sel_min(ed), rvn_ed_sel_max(ed), NULL);
			size_t inner_length = strlen(inner);
			char* wrapped = (char*)rvn_alloc(inner_length + 3);
			wrapped[0] = c;
			memcpy(wrapped + 1, inner, inner_length);
			wrapped[inner_length + 1] = closer;
			wrapped[inner_length + 2] = '\0';
			rvn_ed_break_typing(ed);
			rvn_ed_insert(ed, wrapped, (int)inner_length + 2);
			rvn_ed_break_typing(ed);
			rvn_free(wrapped);
			rvn_free(inner);
			return;
		}
		/* in a comment or in a text, a bracket is only a character */
		if (room && rvn_ed_is_code(ed, ed->caret.line, rvn_maxi(ed->caret.column - 1, 0)) &&
			(!is_quote || (rvn_ed_class(previous) != RVN_ED_WORD && previous != c && previous != '\\'))) {
			char pair[2];
			pair[0] = c;
			pair[1] = closer;
			rvn_ed_break_typing(ed);
			rvn_ed_insert(ed, pair, 2);
			rvn_ed_break_typing(ed);
			ed->caret = ed->anchor = rvn_ed_pos(ed->caret.line, ed->caret.column - 1);
			return;
		}
	}
	if (c == '}' && !selection && ed->caret.column > 0) {
		/* alone on its line, a closing brace comes back one level */
		int column = ed->caret.column, cut = 0;
		int blank = R_TRUE, i;
		for (i = 0; i < column && blank; i++) blank = rvn_ed_is_blank(line->text[i]);
		if (blank) {
			if (line->text[column - 1] == '\t') cut = 1;
			else while (cut < ed->tab_size && cut < column && line->text[column - 1 - cut] == ' ') cut++;
			rvn_ed_break_typing(ed);
			ed->caret = ed->anchor = rvn_ed_replace(ed, rvn_ed_pos(ed->caret.line, column - cut), ed->caret, "}", 1, R_TRUE);
			rvn_ed_break_typing(ed);
			return;
		}
	}
	if (c == '>' && !selection && (desc->flags & R_LANGUAGE_MARKUP) && previous != '/' && previous != '-' && previous != '?') {
		/* the end of an opening tag: its closing tag comes with it */
		int open = ed->caret.column - 1, name_end;
		while (open >= 0 && line->text[open] != '<' && line->text[open] != '>') open--;
		if (open >= 0 && line->text[open] == '<' && open + 1 < ed->caret.column && rvn_ed_class(line->text[open + 1]) == RVN_ED_WORD) {
			name_end = open + 1;
			while (name_end < ed->caret.column && !rvn_ed_is_blank(line->text[name_end])) name_end++;
			if (!((desc->flags & R_LANGUAGE_EMBEDS) && rvn_ed_is_void_tag(line->text + open + 1, name_end - open - 1)) && name_end - open < 80) {
				char closing[96];
				int closing_length = snprintf(closing, sizeof(closing), "></%.*s>", name_end - open - 1, line->text + open + 1);
				rvn_ed_pos_t caret = ed->caret;
				rvn_ed_break_typing(ed);
				rvn_ed_insert(ed, closing, closing_length);
				rvn_ed_break_typing(ed);
				ed->caret = ed->anchor = rvn_ed_pos(caret.line, caret.column + 1);
				return;
			}
		}
	}
	rvn_ed_insert(ed, encoded, length);
}

/* Ctrl+/: comments the selected lines out, or back in when they all are. Returns R_FALSE when the language has no such comment. */
static int rvn_ed_toggle_comment(rvn_text_editor_t* ed) {
	const RLanguageDesc* desc = rvn_ed_language_desc(ed);
	rvn_ed_pos_t from = rvn_ed_sel_min(ed), to = rvn_ed_sel_max(ed);
	int first = from.line, last = to.line, line, mark_length, min_indent = 1 << 20, all = R_TRUE, any = R_FALSE;
	int length = 0, cap = 0;
	char* out = NULL;

	if (!desc || !desc->line_comment) return R_FALSE;
	mark_length = (int)strlen(desc->line_comment);
	if (last > first && to.column == 0) last--;
	for (line = first; line <= last; line++) {
		const rvn_ed_line_t* l = &ed->lines[line];
		int indent = 0;
		while (indent < l->length && rvn_ed_is_blank(l->text[indent])) indent++;
		if (indent == l->length) continue;
		any = R_TRUE;
		if (indent < min_indent) min_indent = indent;
		if (l->length - indent < mark_length || memcmp(l->text + indent, desc->line_comment, (size_t)mark_length) != 0) all = R_FALSE;
	}
	if (!any) return R_TRUE;
	for (line = first; line <= last; line++) {
		const rvn_ed_line_t* l = &ed->lines[line];
		int indent = 0;
		while (indent < l->length && rvn_ed_is_blank(l->text[indent])) indent++;
		if (length + l->length + mark_length + 4 > cap) {
			cap = (length + l->length + mark_length + 4) * 2;
			out = (char*)rvn_realloc(out, (size_t)cap);
		}
		if (indent == l->length) {
			memcpy(out + length, l->text, (size_t)l->length);
			length += l->length;
		} else if (all) {
			int skip = indent + mark_length;
			if (skip < l->length && l->text[skip] == ' ') skip++;
			memcpy(out + length, l->text, (size_t)indent);
			length += indent;
			memcpy(out + length, l->text + skip, (size_t)(l->length - skip));
			length += l->length - skip;
		} else {
			/* the marks line up, at the indentation of the least indented line */
			memcpy(out + length, l->text, (size_t)min_indent);
			length += min_indent;
			memcpy(out + length, desc->line_comment, (size_t)mark_length);
			length += mark_length;
			out[length++] = ' ';
			memcpy(out + length, l->text + min_indent, (size_t)(l->length - min_indent));
			length += l->length - min_indent;
		}
		if (line < last) out[length++] = '\n';
	}
	rvn_ed_break_typing(ed);
	rvn_ed_replace(ed, rvn_ed_pos(first, 0), rvn_ed_pos(last, ed->lines[last].length), out ? out : "", length, R_TRUE);
	rvn_ed_break_typing(ed);
	rvn_free(out);
	if (first == last && rvn_ed_pos_cmp(from, to) == 0) {
		ed->caret = ed->anchor = rvn_ed_clamp(ed, rvn_ed_pos(first, from.column + (all ? -(mark_length + 1) : mark_length + 1)));
	} else {
		ed->anchor = rvn_ed_pos(first, 0);
		ed->caret = rvn_ed_pos(last, ed->lines[last].length);
	}
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Proposals (completion)
 *
 *  The list is a popup of the window, owned by the editor. The editor keeps
 *  the keyboard: the popup only takes the keys that move in the list or
 *  choose from it, and every character typed narrows the list down.
 * ------------------------------------------------------------------------- */

#define RVN_EDC_MAX_ITEMS 400
#define RVN_EDC_MAX_ROWS 8

typedef struct rvn_edc_item_t {
	char* text;
	char* detail;
	/** What choosing it writes in place of the word, when not `text` (Rtext_editor_add_completion_insert). */
	char* insert;
	/** 0: given by the application; 1: starts as what was typed; 2: starts so, whatever the case. */
	int rank;
	/** When it was added: the proposals of the application stay in the order it gave them. */
	uint32_t order;
} rvn_edc_item_t;

typedef struct rvn_ed_complete_t {
	rvn_widget_t base;
	rvn_text_editor_t* editor;
	rvn_edc_item_t* items;
	uint32_t count;
	uint32_t cap;
	int selected;
	rvn_scrollbar_t bar;
	RSpring select_y;
	/** Where the word being completed starts, and how many bytes of it are typed. */
	rvn_ed_pos_t start;
	int prefix_length;
	/** R_TRUE while the callback of the application may add proposals. */
	int collecting;
} rvn_ed_complete_t;

static void rvn_edc_clear(rvn_ed_complete_t* list) {
	uint32_t i;
	for (i = 0; i < list->count; i++) {
		rvn_free(list->items[i].text);
		rvn_free(list->items[i].detail);
		rvn_free(list->items[i].insert);
	}
	list->count = 0;
}

static void rvn_edc_add(rvn_ed_complete_t* list, const char* text, int length, const char* detail, int rank) {
	rvn_edc_item_t* item;
	if (list->count >= RVN_EDC_MAX_ITEMS || length <= 0) return;
	RVN_ARRAY_RESERVE(list->items, list->cap, list->count + 1);
	item = &list->items[list->count++];
	item->text = rvn_strndup(text, (size_t)length);
	item->detail = detail ? rvn_strdup(detail) : NULL;
	item->insert = NULL;
	item->rank = rank;
	item->order = list->count;
}

static int rvn_edc_compare(const void* a, const void* b) {
	const rvn_edc_item_t* x = (const rvn_edc_item_t*)a;
	const rvn_edc_item_t* y = (const rvn_edc_item_t*)b;
	int order;
	if (x->rank != y->rank) return x->rank < y->rank ? -1 : 1;
	if (x->rank == 0) return x->order < y->order ? -1 : x->order > y->order;
	order = rvn_stricmp(x->text, y->text);
	if (order == 0) order = strcmp(x->text, y->text);
	/* the same word twice: the one that says what it is comes first, and is the one kept */
	if (order == 0) order = (x->detail ? 0 : 1) - (y->detail ? 0 : 1);
	return order;
}

static float rvn_edc_row_height(const rvn_widget_t* widget) {
	return rvn_widget_metric(widget, R_METRIC_ROW_HEIGHT);
}

static float rvn_edc_pad(const rvn_widget_t* widget) {
	return rvn_widget_glass(widget) ? 5.0f : 1.0f;
}

static rvn_vec2_t rvn_edc_measure(rvn_widget_t* widget) {
	rvn_ed_complete_t* list = (rvn_ed_complete_t*)widget;
	rvn_font_t* font = rvn_font_resolve(g_rvn.mono_font);
	float size = rvn_widget_font_size(widget, 0.0f), width = 0.0f;
	uint32_t i, rows = list->count < RVN_EDC_MAX_ROWS ? list->count : RVN_EDC_MAX_ROWS;
	for (i = 0; i < list->count && i < 64; i++) {
		float item = rvn_text_width(font, size, list->items[i].text, -1);
		if (list->items[i].detail) item += rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, list->items[i].detail) + 28.0f;
		if (item > width) width = item;
	}
	width = rvn_clampf(width + rvn_widget_metric(widget, R_METRIC_PADDING_X) * 2.0f + 14.0f, 180.0f, 460.0f);
	return rvn_vec2(ceilf(width), rvn_edc_row_height(widget) * (float)(rows ? rows : 1) + rvn_edc_pad(widget) * 2.0f);
}

static void rvn_edc_reveal(rvn_widget_t* widget) {
	rvn_ed_complete_t* list = (rvn_ed_complete_t*)widget;
	float row = rvn_edc_row_height(widget), top = row * (float)list->selected;
	if (top < list->bar.offset) rvn_scrollbar_set(&list->bar, top);
	else if (top + row > list->bar.offset + list->bar.viewport) rvn_scrollbar_set(&list->bar, top + row - list->bar.viewport);
}

static void rvn_edc_arrange(rvn_widget_t* widget) {
	rvn_ed_complete_t* list = (rvn_ed_complete_t*)widget;
	list->bar.content = rvn_edc_row_height(widget) * (float)list->count;
	list->bar.viewport = rvn_maxf(0.0f, widget->rect.h - rvn_edc_pad(widget) * 2.0f);
	rvn_scrollbar_clamp(&list->bar);
	rvn_edc_reveal(widget);
}

static rvn_rect_t rvn_edc_track(const rvn_widget_t* widget) {
	float bar = rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE), pad = rvn_edc_pad(widget);
	return rvn_rect(widget->rect.w - bar - 1.0f, pad, bar, rvn_maxf(0.0f, widget->rect.h - pad * 2.0f));
}

static void rvn_edc_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_ed_complete_t* list = (rvn_ed_complete_t*)widget;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	float row = rvn_edc_row_height(widget), padding = rvn_widget_metric(widget, R_METRIC_PADDING_X) * 0.8f;
	float radius = rvn_widget_metric(widget, R_METRIC_POPUP_RADIUS), pad = rvn_edc_pad(widget);
	float size = rvn_widget_font_size(widget, 0.0f);
	float offset = rvn_scrollbar_shown(&list->bar, painter);
	rvn_font_t* font = rvn_font_resolve(g_rvn.mono_font);
	rvn_color_t text = rvn_widget_color(widget, R_COLOR_TEXT);
	rvn_color_t accent = rvn_widget_color(widget, R_COLOR_ACCENT_HOVERED);
	rvn_vec2_t origin = rvn_widget_origin(widget);
	int first = (int)floorf(offset / row), last, i;

	if (rvn_widget_glass(widget)) {
		rvn_paint_surface(painter, widget, rect, radius, RVN_SURFACE_POPUP, 0.0f, 0.0f);
	} else {
		rvn_paint_shadow(painter, rvn_rect(0.0f, 3.0f, rect.w, rect.h), radius, 12.0f, rvn_widget_color(widget, R_COLOR_SHADOW));
		rvn_paint_box(painter, rect, radius, rvn_widget_color(widget, R_COLOR_POPUP), 1.0f, rvn_widget_color(widget, R_COLOR_POPUP_BORDER));
	}
	rvn_paint_push_clip(painter, rvn_rect(1.0f, pad, rect.w - 2.0f, rect.h - pad * 2.0f));
	if (list->selected >= 0 && list->selected < (int)list->count) {
		float y = rvn_paint_spring(painter, &list->select_y, row * (float)list->selected, R_MOTION_SNAPPY);
		rvn_paint_row(painter, widget, rvn_rect(1.0f, pad + y - offset, rect.w - 2.0f, row), rvn_widget_color(widget, R_COLOR_SELECTION));
	}
	last = rvn_mini((int)list->count, first + (int)(list->bar.viewport / row) + 3);
	for (i = rvn_maxi(first, 0); i < last; i++) {
		const rvn_edc_item_t* item = &list->items[i];
		rvn_rect_t line = rvn_rect(padding, pad + row * (float)i - offset, rect.w - padding * 2.0f, row);
		int typed = rvn_mini(list->prefix_length, (int)strlen(item->text));
		float typed_width = rvn_text_width(font, size, item->text, typed);
		/* what is already typed stands out from what choosing the word would add */
		rvn_paint_text_in(painter, g_rvn.mono_font, size, line, 0.0f, item->text, typed, accent);
		rvn_paint_text_in(painter, g_rvn.mono_font, size, rvn_rect(line.x + typed_width, line.y, line.w - typed_width, row), 0.0f,
			item->text + typed, -1, text);
		if (item->detail) {
			rvn_paint_text_in(painter, R_INVALID_ID, size - 1.0f, rvn_rect(line.x, line.y, line.w - 6.0f, row), 1.0f, item->detail, -1,
				rvn_color_fade(text, 0.5f));
		}
	}
	rvn_paint_pop_clip(painter);
	rvn_scrollbar_paint(&list->bar, widget, painter, rvn_edc_track(widget), R_TRUE,
		widget->window ? widget->window->mouse_x - origin.x : -1.0f, widget->window ? widget->window->mouse_y - origin.y : -1.0f);
}

static int rvn_ed_completing(const rvn_text_editor_t* ed) {
	return ed->complete && (ed->complete->flags & RVN_WF_POPUP);
}

static void rvn_ed_complete_close(rvn_text_editor_t* ed) {
	if (rvn_ed_completing(ed) && ed->complete->window) rvn_window_close_popup(ed->complete->window, ed->complete);
}

/* Puts the chosen word in place of what was typed of it. */
static void rvn_ed_complete_accept(rvn_text_editor_t* ed, int index) {
	rvn_ed_complete_t* list = (rvn_ed_complete_t*)ed->complete;
	rvn_widget_t* widget = &ed->base;
	char* text;
	if (!list || index < 0 || index >= (int)list->count) return;
	text = rvn_strdup(list->items[index].insert ? list->items[index].insert : list->items[index].text);
	rvn_ed_complete_close(ed);
	if (list->start.line == ed->caret.line && list->start.column <= ed->caret.column && !ed->read_only) {
		rvn_ed_break_typing(ed);
		ed->caret = ed->anchor = rvn_ed_replace(ed, list->start, ed->caret, text, -1, R_TRUE);
		rvn_ed_break_typing(ed);
		rvn_ed_caret_moved(ed, R_FALSE);
		rvn_free(text);
		rvn_ed_notify(widget);
		return;
	}
	rvn_free(text);
}

static int rvn_edc_event(rvn_widget_t* widget, const REvent* event) {
	rvn_ed_complete_t* list = (rvn_ed_complete_t*)widget;
	int count = (int)list->count, changed = R_FALSE, row;
	float pad = rvn_edc_pad(widget);

	if (event->type == R_EVENT_MOUSE_DOWN || event->type == R_EVENT_MOUSE_MOVE || event->type == R_EVENT_MOUSE_UP) {
		if (rvn_scrollbar_event(&list->bar, widget, event, rvn_edc_track(widget), R_TRUE, &changed)) {
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		}
	}
	switch (event->type) {
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_TRUE;
			row = (int)floorf((event->local_y - pad + list->bar.offset) / rvn_edc_row_height(widget));
			if (row >= 0 && row < count) rvn_ed_complete_accept(list->editor, row);
			return R_TRUE;
		case R_EVENT_MOUSE_WHEEL:
			rvn_scrollbar_wheel(&list->bar, -event->wheel_y * rvn_edc_row_height(widget) * 3.0f);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			if (count == 0 || (event->modifiers & (R_MOD_CTRL | R_MOD_ALT | R_MOD_SUPER))) return R_FALSE;
			switch (event->key) {
				case R_KEY_UP: row = list->selected <= 0 ? count - 1 : list->selected - 1; break;
				case R_KEY_DOWN: row = list->selected + 1 >= count ? 0 : list->selected + 1; break;
				case R_KEY_PAGE_UP: row = rvn_maxi(list->selected - RVN_EDC_MAX_ROWS + 1, 0); break;
				case R_KEY_PAGE_DOWN: row = rvn_mini(list->selected + RVN_EDC_MAX_ROWS - 1, count - 1); break;
				case R_KEY_ENTER:
				case R_KEY_KP_ENTER:
				case R_KEY_TAB:
					rvn_ed_complete_accept(list->editor, list->selected);
					return R_TRUE;
				default:
					/* everything else is for the editor: typing goes on */
					return R_FALSE;
			}
			list->selected = row;
			rvn_edc_reveal(widget);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static void rvn_edc_destroy(rvn_widget_t* widget) {
	rvn_ed_complete_t* list = (rvn_ed_complete_t*)widget;
	rvn_edc_clear(list);
	rvn_free(list->items);
}

static const rvn_widget_class_t rvn_class_ed_complete = {
	.type = RVN_WIDGET_INTERNAL_COMPLETION,
	.name = "TextEditorCompletion",
	.size = sizeof(rvn_ed_complete_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_SURFACE | RVN_CF_NO_MIRROR,
	.destroy = rvn_edc_destroy,
	.measure = rvn_edc_measure,
	.arrange = rvn_edc_arrange,
	.paint = rvn_edc_paint,
	.event = rvn_edc_event
};

static int rvn_ed_is_word_char(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '$' || (unsigned char)c >= 0x80;
}

/* Adds the names of the text that start with `prefix`, from the lines around the caret. */
static void rvn_ed_complete_document(rvn_text_editor_t* ed, rvn_ed_complete_t* list, const char* prefix, int prefix_length) {
	int first = rvn_maxi(ed->caret.line - 4000, 0), last = rvn_mini(ed->caret.line + 4000, (int)ed->line_count - 1), line;
	for (line = first; line <= last && list->count < RVN_EDC_MAX_ITEMS; line++) {
		const rvn_ed_line_t* l = &ed->lines[line];
		int i = 0;
		while (i < l->length) {
			int start;
			if (!rvn_ed_is_word_char(l->text[i])) {
				i++;
				continue;
			}
			start = i;
			while (i < l->length && rvn_ed_is_word_char(l->text[i])) i++;
			/* long enough to be worth choosing, not a number, and not the word being typed itself */
			if (i - start < 3 || i - start <= prefix_length || (l->text[start] >= '0' && l->text[start] <= '9')) continue;
			if (line == ed->caret.line && start == list->start.column) continue;
			if (rvn_strnicmp(l->text + start, prefix, (size_t)prefix_length) != 0) continue;
			rvn_edc_add(list, l->text + start, i - start, NULL, memcmp(l->text + start, prefix, (size_t)prefix_length) == 0 ? 1 : 2);
		}
	}
}

/*
 * Gathers the proposals for the word before the caret and shows them, or
 * hides the list when there is nothing to propose.
 * mode 0: after a character was typed (two characters at least are needed)
 * mode 1: asked for (Ctrl+Space): any start, even none
 * mode 2: the list is shown and the word changed
 */
static void rvn_ed_complete_update(rvn_widget_t* widget, int mode) {
	rvn_text_editor_t* ed = (rvn_text_editor_t*)widget;
	const rvn_language_t* language = rvn_ed_language(ed);
	const rvn_ed_line_t* line = &ed->lines[ed->caret.line];
	static const char* const details[3] = { "keyword", "type", "constant" };
	rvn_ed_complete_t* list;
	char prefix[128];
	int start = ed->caret.column, prefix_length, k, i;
	uint32_t write, read;

	if (!widget->window || ed->read_only || rvn_ed_has_selection(ed) || (mode == 0 && (ed->no_completion || !language))) {
		rvn_ed_complete_close(ed);
		return;
	}
	while (start > 0 && rvn_ed_is_word_char(line->text[start - 1])) start--;
	prefix_length = ed->caret.column - start;
	if (prefix_length >= (int)sizeof(prefix) || (mode == 0 && prefix_length < 2) || (mode == 2 && prefix_length < 1) ||
		(prefix_length > 0 && line->text[start] >= '0' && line->text[start] <= '9')) {
		rvn_ed_complete_close(ed);
		return;
	}
	memcpy(prefix, line->text + start, (size_t)prefix_length);
	prefix[prefix_length] = '\0';

	if (!ed->complete) {
		ed->complete = rvn_widget_create_class(&rvn_class_ed_complete);
		if (!ed->complete) return;
		ed->complete->flags |= RVN_WF_INTERNAL;
		((rvn_ed_complete_t*)ed->complete)->editor = ed;
	}
	list = (rvn_ed_complete_t*)ed->complete;
	/* another word than the one the list was shown for: it moves there */
	if (rvn_ed_completing(ed) && (list->start.line != ed->caret.line || list->start.column != start)) rvn_ed_complete_close(ed);
	rvn_edc_clear(list);
	list->start = rvn_ed_pos(ed->caret.line, start);
	list->prefix_length = prefix_length;

	if (ed->completion_callback) {
		Rid id = widget->id;
		list->collecting = R_TRUE;
		ed->completion_callback(id, prefix, ed->completion_user);
		if (!rvn_widget_get(id)) return;
		list->collecting = R_FALSE;
	}
	if (language && !ed->no_completion_words) {
		for (k = 0; k < 3; k++) {
			int count = rvn_language_word_count(language, k);
			for (i = 0; i < count; i++) {
				const char* word = rvn_language_word(language, k, i);
				if ((int)strlen(word) <= prefix_length || rvn_strnicmp(word, prefix, (size_t)prefix_length) != 0) continue;
				rvn_edc_add(list, word, (int)strlen(word), details[k], memcmp(word, prefix, (size_t)prefix_length) == 0 ? 1 : 2);
			}
		}
	}
	if (prefix_length > 0 && !ed->no_completion_words) rvn_ed_complete_document(ed, list, prefix, prefix_length);

	if (list->count > 1) qsort(list->items, list->count, sizeof(rvn_edc_item_t), rvn_edc_compare);
	for (write = 0, read = 0; read < list->count; read++) {
		int twice = write > 0 && strcmp(list->items[write - 1].text, list->items[read].text) == 0;
		uint32_t given;
		/* (what the application proposed is not listed a second time among the words of the language or of the text) */
		for (given = 0; !twice && given < write && list->items[given].rank == 0; given++) {
			twice = strcmp(list->items[given].text, list->items[read].text) == 0;
		}
		if (twice) {
			rvn_free(list->items[read].text);
			rvn_free(list->items[read].detail);
			rvn_free(list->items[read].insert);
			continue;
		}
		list->items[write++] = list->items[read];
	}
	list->count = write;
	if (list->count == 0) {
		rvn_ed_complete_close(ed);
		return;
	}
	list->selected = 0;
	list->bar.offset = 0.0f;
	list->select_y.started = R_FALSE;
	list->base.material = widget->material;

	if (!rvn_ed_completing(ed)) {
		rvn_ed_metrics_t m;
		rvn_vec2_t origin = rvn_widget_origin(widget), point;
		rvn_ed_metrics(ed, &m);
		rvn_ed_update_rows(ed, &m);
		point = rvn_ed_pos_point(ed, &m, list->start);
		rvn_window_open_popup(widget->window, ed->complete, widget,
			rvn_rect(origin.x + m.area.x + point.x - ed->hbar.offset - 6.0f, origin.y + m.area.y + point.y - ed->vbar.offset,
				10.0f, m.line_height + 3.0f), RVN_POPUP_BELOW, 0.0f);
	}
	rvn_widget_invalidate_layout(ed->complete);
	widget->window->needs_layout = R_TRUE;
}

/* The marker under a point of the text, or NULL. */
static const rvn_ed_marker_t* rvn_ed_marker_at(const rvn_text_editor_t* ed, rvn_ed_pos_t pos) {
	uint32_t i;
	for (i = 0; i < ed->marker_count; i++) {
		const rvn_ed_marker_t* marker = &ed->markers[i];
		if (marker->line != pos.line || pos.column < marker->from) continue;
		if (marker->to < 0 || pos.column <= marker->to) return marker;
	}
	return NULL;
}

/* Is the caret between an opening bracket or quote and the one that closes it, with nothing in between? */
static int rvn_ed_between_pair(rvn_text_editor_t* ed) {
	const RLanguageDesc* desc = ed->no_pairs ? NULL : rvn_ed_language_desc(ed);
	const rvn_ed_line_t* line = &ed->lines[ed->caret.line];
	char before, after;
	if (!desc || ed->caret.column <= 0 || ed->caret.column >= line->length) return R_FALSE;
	before = line->text[ed->caret.column - 1];
	after = line->text[ed->caret.column];
	if (!rvn_ed_closer(before) || rvn_ed_closer(before) != after) return R_FALSE;
	return before == '(' || before == '[' || before == '{' || rvn_ed_is_quote(desc, before);
}

/* ---------------------------------------------------------------------------
 *  Search and replace
 *
 * A match lies on one line. The matches of a line are taken from its start,
 * one after the other without overlapping, and counted once per version of
 * the line and per search (rvn_ed_line_t.search_stamp): after an edit only
 * the lines that changed are read again.
 * ------------------------------------------------------------------------- */

/*
 * A character in lower case, for a search that ignores the case: ASCII,
 * Latin-1, the Latin letters of Unicode, Greek, Cyrillic, Armenian, the
 * letters of full width. A character and its lower case have the same
 * length in UTF-8, so a match has the length of the query.
 */
static uint32_t rvn_ed_fold(uint32_t c) {
	if (c < 0x80u) return (c >= 'A' && c <= 'Z') ? c + 32u : c;
	if (c < 0x100u) return (c >= 0xC0u && c <= 0xDEu && c != 0xD7u) ? c + 32u : c;
	if (c < 0x250u) {
		if (c == 0x178u) return 0xFFu;
		/* pairs of an upper case and its lower case: the upper case odd, or even */
		if ((c >= 0x139u && c <= 0x148u) || (c >= 0x179u && c <= 0x17Eu) || (c >= 0x1CDu && c <= 0x1DCu)) return (c & 1u) ? c + 1u : c;
		if ((c >= 0x100u && c <= 0x12Fu) || (c >= 0x132u && c <= 0x137u) || (c >= 0x14Au && c <= 0x177u) ||
			(c >= 0x1DEu && c <= 0x1EFu) || (c >= 0x1F8u && c <= 0x21Fu) || (c >= 0x222u && c <= 0x233u)) return (c & 1u) ? c : c + 1u;
		return c;
	}
	if (c >= 0x370u && c < 0x400u) {
		if (c >= 0x391u && c <= 0x3ABu && c != 0x3A2u) return c + 32u;
		if (c == 0x386u) return 0x3ACu;
		if (c >= 0x388u && c <= 0x38Au) return c + 37u;
		if (c == 0x38Cu) return 0x3CCu;
		if (c == 0x38Eu || c == 0x38Fu) return c + 63u;
		/* the final sigma is a sigma */
		if (c == 0x3C2u) return 0x3C3u;
		return c;
	}
	if (c >= 0x400u && c < 0x530u) {
		if (c < 0x410u) return c + 80u;
		if (c < 0x430u) return c + 32u;
		if ((c >= 0x460u && c <= 0x481u) || (c >= 0x48Au && c <= 0x4BFu) || (c >= 0x4D0u && c <= 0x52Fu)) return (c & 1u) ? c : c + 1u;
		if (c == 0x4C0u) return 0x4CFu;
		if (c >= 0x4C1u && c <= 0x4CEu) return (c & 1u) ? c + 1u : c;
		return c;
	}
	if (c >= 0x531u && c <= 0x556u) return c + 48u;
	if ((c >= 0x1E00u && c <= 0x1E95u) || (c >= 0x1EA0u && c <= 0x1EFFu)) return (c & 1u) ? c : c + 1u;
	if (c >= 0xFF21u && c <= 0xFF3Au) return c + 32u;
	return c;
}

static int rvn_ed_word_byte(char c) {
	return rvn_ed_class(c) == RVN_ED_WORD;
}

static void rvn_ed_search_set(rvn_text_editor_t* ed, const char* query, Ruint flags) {
	rvn_ed_search_t* search = &ed->search;
	int length = query ? (int)strlen(query) : 0;
	char* copy;

	flags &= (Ruint)(R_SEARCH_MATCH_CASE | R_SEARCH_WHOLE_WORD);
	search->hidden = R_FALSE;
	rvn_widget_invalidate_paint(&ed->base);
	if (length == search->length && flags == search->flags && (length == 0 || memcmp(query, search->query, (size_t)length) == 0)) return;

	/* (the query may be the editor's own: Rtext_editor_get_search) */
	copy = rvn_strndup(query ? query : "", (size_t)length);
	rvn_free(search->query);
	search->query = copy;
	search->length = length;
	search->flags = flags;
	rvn_free(search->folded);
	search->folded = NULL;
	search->folded_count = 0;
	if (!(flags & R_SEARCH_MATCH_CASE) && length > 0) {
		const char* at = copy;
		const char* end = copy + length;
		search->folded = (uint32_t*)rvn_alloc(sizeof(uint32_t) * (size_t)length);
		while (at < end) search->folded[search->folded_count++] = rvn_ed_fold(rvn_utf8_decode(&at, end));
	}
	search->word_start = length > 0 && rvn_ed_word_byte(copy[0]);
	search->word_end = length > 0 && rvn_ed_word_byte(copy[length - 1]);
	if (++search->stamp == 0) search->stamp = 1;
}

/* The end of a match that starts at `at`, or -1. */
static int rvn_ed_match_at(const rvn_ed_search_t* search, const char* text, int length, int at) {
	const char* read;
	const char* end;
	int i;
	if (search->flags & R_SEARCH_MATCH_CASE) {
		if (length - at < search->length || memcmp(text + at, search->query, (size_t)search->length) != 0) return -1;
		return at + search->length;
	}
	read = text + at;
	end = text + length;
	for (i = 0; i < search->folded_count; i++) {
		if (read >= end || rvn_ed_fold(rvn_utf8_decode(&read, end)) != search->folded[i]) return -1;
	}
	return (int)(read - text);
}

/* The first match in text[from..length): its start and its end. R_FALSE when there is none. */
static int rvn_ed_search_line(const rvn_ed_search_t* search, const char* text, int length, int from, int* start, int* end) {
	int at = from;
	if (search->length == 0) return R_FALSE;
	while (at < length) {
		int stop;
		/* the next place where the first character of the query is */
		if (search->flags & R_SEARCH_MATCH_CASE) {
			const char* found = (const char*)memchr(text + at, search->query[0], (size_t)(length - at));
			if (!found) return R_FALSE;
			at = (int)(found - text);
		} else if (search->folded[0] < 0x80u) {
			unsigned char lower = (unsigned char)search->folded[0];
			unsigned char upper = (lower >= 'a' && lower <= 'z') ? (unsigned char)(lower - 32u) : lower;
			while (at < length && (unsigned char)text[at] != lower && (unsigned char)text[at] != upper) at++;
			if (at >= length) return R_FALSE;
		} else if (((unsigned char)text[at] & 0xC0u) == 0x80u) {
			/* (never inside a character) */
			at++;
			continue;
		}
		stop = rvn_ed_match_at(search, text, length, at);
		if (stop > at && (!(search->flags & R_SEARCH_WHOLE_WORD) ||
			((!search->word_start || at == 0 || !rvn_ed_word_byte(text[at - 1])) &&
			 (!search->word_end || stop == length || !rvn_ed_word_byte(text[stop]))))) {
			*start = at;
			*end = stop;
			return R_TRUE;
		}
		at++;
	}
	return R_FALSE;
}

/* Number of matches of a line. */
static int rvn_ed_line_matches(rvn_text_editor_t* ed, int line) {
	rvn_ed_line_t* l = &ed->lines[line];
	if (ed->search.length == 0) return 0;
	if (l->search_stamp != ed->search.stamp) {
		int count = 0, at = 0, start, end;
		while (rvn_ed_search_line(&ed->search, l->text, l->length, at, &start, &end)) {
			count++;
			at = end;
		}
		l->search_count = count;
		l->search_stamp = ed->search.stamp;
	}
	return l->search_count;
}

/* The match of a line that has this index in it. */
static int rvn_ed_line_match(const rvn_text_editor_t* ed, int line, int index, int* start, int* end) {
	const rvn_ed_line_t* l = &ed->lines[line];
	int at = 0;
	while (rvn_ed_search_line(&ed->search, l->text, l->length, at, start, end)) {
		if (index-- == 0) return R_TRUE;
		at = *end;
	}
	return R_FALSE;
}

static int rvn_ed_match_count(rvn_text_editor_t* ed) {
	rvn_ed_search_t* search = &ed->search;
	if (search->length == 0) return 0;
	if (search->count_version != ed->version || search->count_stamp != search->stamp) {
		uint32_t line;
		int count = 0;
		for (line = 0; line < ed->line_count; line++) count += rvn_ed_line_matches(ed, (int)line);
		search->count = count;
		search->count_version = ed->version;
		search->count_stamp = search->stamp;
	}
	return search->count;
}

/* Index in its line of the match that the selection is, or -1. */
static int rvn_ed_selected_match(const rvn_text_editor_t* ed) {
	rvn_ed_pos_t from = rvn_ed_sel_min(ed), to = rvn_ed_sel_max(ed);
	const rvn_ed_line_t* l = &ed->lines[from.line];
	int at = 0, index = 0, start, end;
	if (ed->search.length == 0 || from.line != to.line || !rvn_ed_has_selection(ed)) return -1;
	while (rvn_ed_search_line(&ed->search, l->text, l->length, at, &start, &end)) {
		if (start == from.column && end == to.column) return index;
		if (start > from.column) break;
		index++;
		at = end;
	}
	return -1;
}

static int rvn_ed_match_index(rvn_text_editor_t* ed) {
	int index = rvn_ed_selected_match(ed), line, last;
	if (index < 0) return -1;
	last = rvn_ed_sel_min(ed).line;
	for (line = 0; line < last; line++) index += rvn_ed_line_matches(ed, line);
	return index;
}

/* Line and columns of the match that has this index in the text. */
static int rvn_ed_find_match(rvn_text_editor_t* ed, int index, int* line_out, int* start, int* end) {
	uint32_t line;
	if (index < 0 || ed->search.length == 0) return R_FALSE;
	for (line = 0; line < ed->line_count; line++) {
		int count = rvn_ed_line_matches(ed, (int)line);
		if (index < count) {
			*line_out = (int)line;
			return rvn_ed_line_match(ed, (int)line, index, start, end);
		}
		index -= count;
	}
	return R_FALSE;
}

/* Selects a match and scrolls to it: a third of the way down when it was out of sight. */
static void rvn_ed_select_match(rvn_text_editor_t* ed, int line, int start, int end) {
	rvn_widget_t* widget = &ed->base;
	ed->anchor = rvn_ed_pos(line, start);
	ed->caret = rvn_ed_pos(line, end);
	rvn_ed_break_typing(ed);
	rvn_ed_complete_close(ed);
	if (widget->rect.w > 0.0f && widget->rect.h > 0.0f && !(widget->flags & RVN_WF_LAYOUT_DIRTY)) {
		rvn_ed_metrics_t m;
		float y;
		rvn_ed_metrics(ed, &m);
		rvn_ed_update_rows(ed, &m);
		rvn_ed_update_bars(ed, &m);
		y = rvn_ed_pos_point(ed, &m, ed->caret).y;
		if (y < ed->vbar.offset || y + m.line_height > ed->vbar.offset + ed->vbar.viewport) {
			rvn_scrollbar_set(&ed->vbar, y - ed->vbar.viewport * 0.33f);
		}
	}
	rvn_ed_caret_moved(ed, R_FALSE);
}

/*
 * The next match after the selection (forward) or the one before it,
 * going on from the other end of the text. Selects it; R_FALSE when the
 * text has none.
 */
static int rvn_ed_find(rvn_text_editor_t* ed, int forward) {
	rvn_ed_pos_t from = forward ? rvn_ed_sel_max(ed) : rvn_ed_sel_min(ed);
	int line_count = (int)ed->line_count, step;
	if (ed->search.length == 0) return R_FALSE;
	/* every line once, from the one of the selection, which comes again last (for what is on its other side) */
	for (step = 0; step <= line_count; step++) {
		int line = forward ? (from.line + step) % line_count : ((from.line - step) % line_count + line_count) % line_count;
		const rvn_ed_line_t* l = &ed->lines[line];
		int at = 0, start, end, found = -1, found_end = 0;
		if (rvn_ed_line_matches(ed, line) == 0) continue;
		while (rvn_ed_search_line(&ed->search, l->text, l->length, at, &start, &end)) {
			if (forward) {
				if (step > 0 || start >= from.column) {
					found = start;
					found_end = end;
					break;
				}
			} else {
				if (step > 0 || start < from.column) {
					found = start;
					found_end = end;
				} else {
					break;
				}
			}
			at = end;
		}
		if (found >= 0) {
			rvn_ed_select_match(ed, line, found, found_end);
			return R_TRUE;
		}
	}
	return R_FALSE;
}

/* Replaces the selected match, then goes to the next one. R_TRUE when something was replaced. */
static int rvn_ed_replace_match(rvn_text_editor_t* ed, const char* replacement) {
	int replaced = R_FALSE;
	if (ed->search.length == 0) return R_FALSE;
	if (rvn_ed_selected_match(ed) >= 0) {
		/* (the replacement may be a line of the editor, which the edit changes) */
		char* copy = rvn_strdup(replacement ? replacement : "");
		rvn_ed_break_typing(ed);
		ed->caret = ed->anchor = rvn_ed_replace(ed, rvn_ed_sel_min(ed), rvn_ed_sel_max(ed), copy, -1, R_TRUE);
		rvn_ed_break_typing(ed);
		rvn_free(copy);
		replaced = R_TRUE;
	}
	if (!rvn_ed_find(ed, R_TRUE)) rvn_ed_caret_moved(ed, R_FALSE);
	return replaced;
}

/*
 * Replaces every match: the text from the first match to the end of the
 * last one is replaced at once, which is one step of the history.
 * Returns the number of matches replaced.
 */
static int rvn_ed_replace_all(rvn_text_editor_t* ed, const char* replacement) {
	int first = -1, last = -1, line, count = 0, length = 0, cap = 0, replacement_length;
	rvn_ed_pos_t from, to;
	char* copy;
	char* out = NULL;

	if (rvn_ed_match_count(ed) == 0) return 0;
	for (line = 0; line < (int)ed->line_count; line++) {
		if (rvn_ed_line_matches(ed, line) == 0) continue;
		if (first < 0) first = line;
		last = line;
	}
	copy = rvn_strdup(replacement ? replacement : "");
	replacement_length = (int)strlen(copy);
	from = rvn_ed_pos(first, 0);
	to = rvn_ed_pos(last, 0);
	for (line = first; line <= last; line++) {
		const rvn_ed_line_t* l = &ed->lines[line];
		int at = 0, kept = 0, start, end;
		while (rvn_ed_search_line(&ed->search, l->text, l->length, at, &start, &end)) {
			int need;
			if (count == 0) {
				/* what comes before the first match stays out of the edit */
				from.column = start;
				kept = start;
			}
			need = length + (start - kept) + replacement_length + 1;
			if (need > cap) {
				cap = need * 2 + 64;
				out = (char*)rvn_realloc(out, (size_t)cap);
			}
			memcpy(out + length, l->text + kept, (size_t)(start - kept));
			length += start - kept;
			memcpy(out + length, copy, (size_t)replacement_length);
			length += replacement_length;
			kept = end;
			at = end;
			count++;
		}
		if (line < last) {
			int need = length + (l->length - kept) + 2;
			if (need > cap) {
				cap = need * 2 + 64;
				out = (char*)rvn_realloc(out, (size_t)cap);
			}
			memcpy(out + length, l->text + kept, (size_t)(l->length - kept));
			length += l->length - kept;
			out[length++] = '\n';
		} else {
			/* and what comes after the last one */
			to.column = kept;
		}
	}
	rvn_ed_break_typing(ed);
	ed->caret = ed->anchor = rvn_ed_replace(ed, from, to, out ? out : "", length, R_TRUE);
	rvn_ed_break_typing(ed);
	rvn_free(out);
	rvn_free(copy);
	rvn_ed_caret_moved(ed, R_FALSE);
	return count;
}

static int rvn_ed_search_shown(const rvn_text_editor_t* ed) {
	return ed->search.length > 0 && !ed->search.hidden;
}

/* The matches that lie on one row of a line (text[begin..end)), drawn at (x, y). */
static void rvn_ed_paint_matches(rvn_text_editor_t* ed, RPainter* painter, const rvn_ed_metrics_t* m, int line, int begin, int end,
	float x, float y) {

	const rvn_widget_t* widget = &ed->base;
	const rvn_ed_line_t* l = &ed->lines[line];
	rvn_ed_pos_t sel_min, sel_max;
	rvn_color_t match, current;
	int at, start, stop;

	if (!rvn_ed_search_shown(ed) || rvn_ed_line_matches(ed, line) == 0) return;
	match = rvn_widget_color(widget, R_COLOR_SEARCH_MATCH);
	current = rvn_widget_color(widget, R_COLOR_SEARCH_CURRENT);
	sel_min = rvn_ed_sel_min(ed);
	sel_max = rvn_ed_sel_max(ed);
	/* only what can reach the row is read (a match has the length of the query); a long line is not read from its start for each row */
	at = begin > 0 ? rvn_ed_clamp(ed, rvn_ed_pos(line, rvn_maxi(begin - ed->search.length + 1, 0))).column : 0;
	while (rvn_ed_search_line(&ed->search, l->text, l->length, at, &start, &stop) && start < end) {
		at = stop;
		if (stop > begin || (stop == begin && start == begin)) {
			int from = rvn_maxi(start, begin), to = rvn_mini(stop, end);
			int is_current = sel_min.line == line && sel_max.line == line && sel_min.column == start && sel_max.column == stop;
			float x0 = rvn_ed_advance(m, l->text, begin, from, 0.0f);
			float x1 = rvn_ed_advance(m, l->text, from, to, x0);
			if (x1 <= x0) continue;
			if (is_current) {
				rvn_paint_box(painter, rvn_rect(x + x0, y, x1 - x0, m->line_height), 3.0f, current, 1.0f,
					rvn_color_fade(current, 1.0f / rvn_maxf(0.25f, (float)rvn_color_a(current) / 255.0f)));
			} else {
				rvn_paint_box(painter, rvn_rect(x + x0, y, x1 - x0, m->line_height), 3.0f, match, 0.0f, 0);
			}
		}
	}
}

/* ---------------------------------------------------------------------------
 *  The find bar
 *
 * Drawn by the editor over the top right of its text: a field for what is
 * searched for, the number of matches, the options, previous / next and
 * close; a second row with a field for the replacement, replace and replace
 * all. The editor keeps the keyboard focus; find_focus tells which of its
 * fields the keys go to.
 * ------------------------------------------------------------------------- */

enum {
	RVN_ED_FIND_NONE,
	RVN_ED_FIND_EXPAND,
	RVN_ED_FIND_QUERY,
	RVN_ED_FIND_CASE,
	RVN_ED_FIND_WORD,
	RVN_ED_FIND_PREVIOUS,
	RVN_ED_FIND_NEXT,
	RVN_ED_FIND_CLOSE,
	RVN_ED_FIND_REPLACE,
	RVN_ED_FIND_REPLACE_ONE,
	RVN_ED_FIND_REPLACE_ALL,
	RVN_ED_FIND_PARTS
};

typedef struct rvn_ed_find_layout_t {
	rvn_rect_t bar;
	rvn_rect_t parts[RVN_ED_FIND_PARTS];
	rvn_rect_t counter;
	float size;
} rvn_ed_find_layout_t;

/* A text of the locale when it has the key, the English one otherwise. */
static const char* rvn_ed_text(const char* key, const char* english) {
	return Rtr_exists(key) ? Rtr(key) : english;
}

static int rvn_ed_find_has_keys(const rvn_text_editor_t* ed) {
	return ed->find_open && ed->find_focus != 0;
}

static rvn_textedit_t* rvn_ed_find_field(rvn_text_editor_t* ed, int which) {
	return which == 2 ? &ed->find_replace : &ed->find_query;
}

/* "3 of 12", "No results", or "" without a search. `none` is set when nothing is found. */
static void rvn_ed_find_counter(rvn_text_editor_t* ed, char* out, size_t cap, int* none) {
	int count = rvn_ed_match_count(ed), index;
	char a[16], b[16];
	*none = R_FALSE;
	out[0] = '\0';
	if (ed->search.length == 0) return;
	if (count == 0) {
		*none = R_TRUE;
		snprintf(out, cap, "%s", rvn_ed_text("Raven.FindNoResults", "No results"));
		return;
	}
	index = rvn_ed_match_index(ed);
	if (index >= 0) snprintf(a, sizeof(a), "%d", index + 1);
	else snprintf(a, sizeof(a), "?");
	snprintf(b, sizeof(b), "%d", count);
	if (Rtr_exists("Raven.FindCount")) {
		const char* args[2];
		args[0] = a;
		args[1] = b;
		snprintf(out, cap, "%s", Rtr_format("Raven.FindCount", args, 2));
	} else {
		snprintf(out, cap, "%s of %s", a, b);
	}
}

static void rvn_ed_find_layout(rvn_text_editor_t* ed, const rvn_ed_metrics_t* m, rvn_ed_find_layout_t* f) {
	const rvn_widget_t* widget = &ed->base;
	const char* replace_text = rvn_ed_text("Raven.Replace", "Replace");
	const char* all_text = rvn_ed_text("Raven.ReplaceAll", "Replace all");
	char counter[96];
	float row = ceilf(rvn_widget_line_height(widget, R_INVALID_ID, 0.0f) + 10.0f);
	float pad = 4.0f, gap = 2.0f, expand = 18.0f, button = row;
	float counter_w, replace_w, all_w, row1, row2, field, right, x, y;
	int none, i;

	memset(f, 0, sizeof(*f));
	f->size = rvn_widget_font_size(widget, 0.0f);
	rvn_ed_find_counter(ed, counter, sizeof(counter), &none);
	counter_w = ceilf(rvn_maxf(rvn_widget_text_width(widget, R_INVALID_ID, f->size, counter),
		rvn_widget_text_width(widget, R_INVALID_ID, f->size, "888 of 888")) + 10.0f);
	replace_w = ceilf(rvn_widget_text_width(widget, R_INVALID_ID, f->size, replace_text) + 18.0f);
	all_w = ceilf(rvn_widget_text_width(widget, R_INVALID_ID, f->size, all_text) + 18.0f);
	row1 = gap + counter_w + 5.0f * (button + gap);
	row2 = gap + replace_w + gap + all_w;
	right = widget->rect.w - 1.0f - ((ed->wrap || rvn_scrollbar_needed(&ed->vbar)) ? m->bar : 0.0f) - 8.0f;
	field = rvn_clampf(right - (m->gutter + 8.0f) - (pad * 2.0f + expand + gap + rvn_maxf(row1, row2)), 80.0f, 240.0f);

	f->bar.w = pad * 2.0f + expand + gap + field + rvn_maxf(row1, row2);
	f->bar.h = pad * 2.0f + row + (ed->find_open == 2 ? row + pad : 0.0f);
	f->bar.x = right - f->bar.w;
	f->bar.y = 4.0f;

	x = f->bar.x + pad;
	y = f->bar.y + pad;
	f->parts[RVN_ED_FIND_EXPAND] = rvn_rect(x, y, expand, ed->find_open == 2 ? row * 2.0f + pad : row);
	x += expand + gap;
	f->parts[RVN_ED_FIND_QUERY] = rvn_rect(x, y, field, row);
	x += field + gap;
	f->counter = rvn_rect(x, y, counter_w, row);
	x += counter_w + gap;
	for (i = RVN_ED_FIND_CASE; i <= RVN_ED_FIND_CLOSE; i++) {
		f->parts[i] = rvn_rect(x, y, button, row);
		x += button + gap;
	}
	if (ed->find_open == 2) {
		y += row + pad;
		x = f->parts[RVN_ED_FIND_QUERY].x;
		f->parts[RVN_ED_FIND_REPLACE] = rvn_rect(x, y, field, row);
		x += field + gap * 2.0f;
		f->parts[RVN_ED_FIND_REPLACE_ONE] = rvn_rect(x, y, replace_w, row);
		x += replace_w + gap;
		f->parts[RVN_ED_FIND_REPLACE_ALL] = rvn_rect(x, y, all_w, row);
	}
	/* a read-only text is not replaced */
	if (ed->read_only) f->parts[RVN_ED_FIND_EXPAND] = rvn_rect(0.0f, 0.0f, 0.0f, 0.0f);
}

/* The part of the bar under a point of the widget, RVN_ED_FIND_NONE outside of it. Sets *in_bar. */
static int rvn_ed_find_hit(rvn_text_editor_t* ed, float x, float y, int* in_bar) {
	rvn_ed_find_layout_t f;
	rvn_ed_metrics_t m;
	int i;
	*in_bar = R_FALSE;
	if (!ed->find_open) return RVN_ED_FIND_NONE;
	rvn_ed_metrics(ed, &m);
	rvn_ed_find_layout(ed, &m, &f);
	if (!rvn_rect_contains(f.bar, x, y)) return RVN_ED_FIND_NONE;
	*in_bar = R_TRUE;
	for (i = 1; i < RVN_ED_FIND_PARTS; i++) {
		if (f.parts[i].w > 0.0f && rvn_rect_contains(f.parts[i], x, y)) return i;
	}
	return RVN_ED_FIND_NONE;
}

static void rvn_ed_find_paint(rvn_text_editor_t* ed, RPainter* painter, const rvn_ed_metrics_t* m) {
	rvn_widget_t* widget = &ed->base;
	rvn_ed_find_layout_t f;
	char counter[96];
	int none, i, which, count;
	int widget_focused = (widget->flags & RVN_WF_FOCUSED) != 0;
	float radius = rvn_minf(rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS), 8.0f);
	rvn_color_t text = rvn_widget_color(widget, R_COLOR_TEXT);
	rvn_color_t dim = rvn_widget_color(widget, R_COLOR_TEXT_PLACEHOLDER);
	rvn_color_t accent = rvn_widget_color(widget, R_COLOR_ACCENT);
	rvn_color_t danger = rvn_widget_color(widget, R_COLOR_DANGER);

	if (!ed->find_open || !ed->find_ready) return;
	rvn_ed_find_layout(ed, m, &f);
	rvn_ed_find_counter(ed, counter, sizeof(counter), &none);
	count = rvn_ed_match_count(ed);

	/* the surface of a popup */
	if (rvn_widget_glass(widget)) {
		rvn_paint_surface(painter, widget, f.bar, radius + 2.0f, RVN_SURFACE_POPUP, 0.0f, 0.0f);
	} else {
		rvn_paint_shadow(painter, rvn_rect(f.bar.x, f.bar.y + 2.0f, f.bar.w, f.bar.h), radius + 2.0f, 10.0f, rvn_widget_color(widget, R_COLOR_SHADOW));
		rvn_paint_box(painter, f.bar, radius + 2.0f, rvn_widget_color(widget, R_COLOR_POPUP), 1.0f, rvn_widget_color(widget, R_COLOR_POPUP_BORDER));
	}

	/* the fields */
	for (which = 1; which <= ed->find_open; which++) {
		rvn_rect_t frame = f.parts[which == 1 ? RVN_ED_FIND_QUERY : RVN_ED_FIND_REPLACE];
		rvn_rect_t inner = rvn_rect(frame.x + 6.0f, frame.y, frame.w - 12.0f, frame.h);
		rvn_textedit_t* edit = rvn_ed_find_field(ed, which);
		int has_keys = widget_focused && ed->find_focus == which;
		rvn_color_t border = has_keys ? rvn_widget_color(widget, R_COLOR_FOCUS)
			: (which == 1 && none) ? danger : rvn_widget_color(widget, R_COLOR_BORDER);
		rvn_paint_box(painter, frame, radius, rvn_widget_color(widget, R_COLOR_INPUT), has_keys ? 1.5f : 1.0f, border);
		if (edit->length == 0) {
			rvn_paint_text_in(painter, R_INVALID_ID, f.size, inner, 0.0f,
				which == 1 ? rvn_ed_text("Raven.Find", "Find") : rvn_ed_text("Raven.Replace", "Replace"), -1, dim);
		}
		rvn_textedit_paint(edit, widget, painter, inner, R_INVALID_ID, f.size, text, has_keys);
	}
	rvn_paint_text_in(painter, R_INVALID_ID, f.size, f.counter, 0.5f, counter, -1, none ? danger : dim);

	/* the buttons */
	for (i = RVN_ED_FIND_EXPAND; i < RVN_ED_FIND_PARTS; i++) {
		rvn_rect_t r = f.parts[i];
		float cx = r.x + r.w * 0.5f, cy = r.y + r.h * 0.5f, icon = rvn_minf(r.h * 0.42f, 10.0f);
		int hovered = ed->find_hover == i;
		int on = (i == RVN_ED_FIND_CASE && (ed->search.flags & R_SEARCH_MATCH_CASE)) ||
			(i == RVN_ED_FIND_WORD && (ed->search.flags & R_SEARCH_WHOLE_WORD));
		int disabled = (i == RVN_ED_FIND_PREVIOUS || i == RVN_ED_FIND_NEXT || i == RVN_ED_FIND_REPLACE_ONE || i == RVN_ED_FIND_REPLACE_ALL) && count == 0;
		rvn_color_t ink = disabled ? rvn_widget_color(widget, R_COLOR_TEXT_DISABLED) : text;
		if (r.w <= 0.0f || i == RVN_ED_FIND_QUERY || i == RVN_ED_FIND_REPLACE) continue;
		if (i == RVN_ED_FIND_REPLACE_ONE || i == RVN_ED_FIND_REPLACE_ALL) {
			rvn_paint_box(painter, r, radius, rvn_widget_color(widget, hovered && !disabled ? R_COLOR_CONTROL_HOVERED : R_COLOR_CONTROL), 1.0f,
				rvn_widget_color(widget, R_COLOR_CONTROL_BORDER));
			rvn_paint_text_in(painter, R_INVALID_ID, f.size, r, 0.5f,
				i == RVN_ED_FIND_REPLACE_ONE ? rvn_ed_text("Raven.Replace", "Replace") : rvn_ed_text("Raven.ReplaceAll", "Replace all"), -1, ink);
			continue;
		}
		if (on) {
			rvn_paint_box(painter, r, radius, rvn_color_fade(accent, 0.25f), 1.0f, accent);
		} else if (hovered && !disabled) {
			rvn_paint_box(painter, r, radius, rvn_widget_color(widget, R_COLOR_CONTROL_HOVERED), 0.0f, 0);
		}
		switch (i) {
			case RVN_ED_FIND_EXPAND:
				rvn_paint_icon(painter, ed->find_open == 2 ? RVN_ICON_CHEVRON_DOWN : RVN_ICON_CHEVRON_RIGHT, cx, cy, icon * 0.9f, text);
				break;
			case RVN_ED_FIND_CASE:
				rvn_paint_text_in(painter, R_INVALID_ID, f.size, r, 0.5f, "Aa", 2, ink);
				break;
			case RVN_ED_FIND_WORD: {
				/* "ab" between the marks of the edges of a word */
				float w = rvn_widget_text_width(widget, R_INVALID_ID, f.size, "ab");
				rvn_paint_text_in(painter, R_INVALID_ID, f.size, rvn_rect(r.x, r.y - 1.0f, r.w, r.h), 0.5f, "ab", 2, ink);
				rvn_paint_line(painter, cx - w * 0.5f - 2.0f, cy + icon * 0.75f, cx + w * 0.5f + 2.0f, cy + icon * 0.75f, 1.2f, ink);
				rvn_paint_line(painter, cx - w * 0.5f - 2.0f, cy + icon * 0.75f, cx - w * 0.5f - 2.0f, cy + icon * 0.35f, 1.2f, ink);
				rvn_paint_line(painter, cx + w * 0.5f + 2.0f, cy + icon * 0.75f, cx + w * 0.5f + 2.0f, cy + icon * 0.35f, 1.2f, ink);
				break;
			}
			case RVN_ED_FIND_PREVIOUS:
				rvn_paint_icon(painter, RVN_ICON_CHEVRON_UP, cx, cy, icon, ink);
				break;
			case RVN_ED_FIND_NEXT:
				rvn_paint_icon(painter, RVN_ICON_CHEVRON_DOWN, cx, cy, icon, ink);
				break;
			case RVN_ED_FIND_CLOSE:
				rvn_paint_icon(painter, RVN_ICON_CROSS, cx, cy, icon * 0.9f, ink);
				break;
			default:
				break;
		}
	}
}

static void rvn_ed_find_init(rvn_text_editor_t* ed) {
	if (ed->find_ready) return;
	rvn_textedit_init(&ed->find_query);
	rvn_textedit_init(&ed->find_replace);
	ed->find_ready = R_TRUE;
}

/* Gives the keys to a field of the bar (0: back to the text). */
static void rvn_ed_find_focus(rvn_text_editor_t* ed, int which) {
	if (which == 2 && ed->find_open != 2) which = 1;
	if (ed->find_focus == which) return;
	if (ed->find_focus) {
		rvn_textedit_t* old = rvn_ed_find_field(ed, ed->find_focus);
		old->anchor = old->caret;
		old->mouse_selecting = R_FALSE;
	}
	ed->find_focus = which;
	if (which) rvn_ed_find_field(ed, which)->blink_start = rvn_clock_seconds();
	ed->blink_start = rvn_clock_seconds();
	rvn_widget_invalidate_paint(&ed->base);
}

/* The text of the field changed: search it, from where the selection was when the bar opened. */
static void rvn_ed_find_query_changed(rvn_text_editor_t* ed) {
	rvn_ed_search_set(ed, ed->find_query.text, ed->search.flags);
	ed->find_origin = rvn_ed_clamp(ed, ed->find_origin);
	ed->caret = ed->anchor = ed->find_origin;
	if (!rvn_ed_find(ed, R_TRUE)) rvn_ed_caret_moved(ed, R_FALSE);
}

/* Opens the bar (with the row to replace when `replace`), the selection on one line becoming the search. */
static void rvn_ed_find_open(rvn_text_editor_t* ed, int replace) {
	rvn_ed_pos_t from = rvn_ed_sel_min(ed), to = rvn_ed_sel_max(ed);
	int seeded = R_FALSE;
	if (ed->no_find_bar) return;
	rvn_ed_find_init(ed);
	rvn_ed_complete_close(ed);
	if (replace && !ed->read_only) ed->find_open = 2;
	else if (!ed->find_open) ed->find_open = 1;
	ed->find_origin = from;
	if (rvn_ed_has_selection(ed) && from.line == to.line) {
		char* selected = rvn_ed_range(ed, from, to, NULL);
		rvn_textedit_set_text(&ed->find_query, selected);
		rvn_free(selected);
		seeded = R_TRUE;
	} else if (ed->find_query.length == 0 && ed->search.length > 0 && !memchr(ed->search.query, '\n', (size_t)ed->search.length)) {
		rvn_textedit_set_text(&ed->find_query, ed->search.query);
	}
	rvn_ed_search_set(ed, ed->find_query.text, ed->search.flags);
	rvn_textedit_select_all(&ed->find_query);
	/* the replacement is what is typed next when the search is already known */
	rvn_ed_find_focus(ed, ed->find_open == 2 && (seeded || ed->find_query.length > 0) && replace ? 2 : 1);
	if (ed->find_focus == 2) rvn_textedit_select_all(&ed->find_replace);
	rvn_widget_invalidate_paint(&ed->base);
}

static void rvn_ed_find_close(rvn_text_editor_t* ed) {
	if (!ed->find_open) return;
	rvn_ed_find_focus(ed, 0);
	ed->find_open = 0;
	ed->find_hover = RVN_ED_FIND_NONE;
	ed->find_dragging = R_FALSE;
	ed->search.hidden = R_TRUE;
	ed->base.cursor = R_CURSOR_IBEAM;
	rvn_widget_invalidate_paint(&ed->base);
}

/* Next or previous match, by the keys or the buttons: with no search yet, the selection becomes it, or the bar opens. */
static void rvn_ed_find_step(rvn_text_editor_t* ed, int forward) {
	if (ed->search.length == 0) {
		rvn_ed_pos_t from = rvn_ed_sel_min(ed), to = rvn_ed_sel_max(ed);
		if (rvn_ed_has_selection(ed) && from.line == to.line) {
			char* selected = rvn_ed_range(ed, from, to, NULL);
			rvn_ed_find_init(ed);
			rvn_textedit_set_text(&ed->find_query, selected);
			rvn_ed_search_set(ed, selected, ed->search.flags);
			rvn_free(selected);
		} else {
			rvn_ed_find_open(ed, R_FALSE);
			return;
		}
	}
	if (ed->find_open) ed->search.hidden = R_FALSE;
	rvn_ed_find(ed, forward);
	rvn_widget_invalidate_paint(&ed->base);
}

static void rvn_ed_find_toggle(rvn_text_editor_t* ed, Ruint flag) {
	rvn_ed_search_set(ed, ed->search.query, ed->search.flags ^ flag);
}

/* Replace and replace all, from the bar. */
static void rvn_ed_find_replace(rvn_text_editor_t* ed, int all) {
	rvn_widget_t* widget = &ed->base;
	int changed;
	if (ed->read_only || ed->find_open != 2) return;
	ed->search.hidden = R_FALSE;
	changed = all ? rvn_ed_replace_all(ed, ed->find_replace.text) > 0 : rvn_ed_replace_match(ed, ed->find_replace.text);
	if (changed) rvn_ed_notify(widget);
}

/* A click on a part of the bar. */
static void rvn_ed_find_press(rvn_text_editor_t* ed, int part, const REvent* event, const rvn_ed_find_layout_t* f) {
	switch (part) {
		case RVN_ED_FIND_QUERY:
		case RVN_ED_FIND_REPLACE: {
			int which = part == RVN_ED_FIND_QUERY ? 1 : 2;
			rvn_rect_t frame = f->parts[part];
			rvn_ed_find_focus(ed, which);
			rvn_textedit_mouse_down(rvn_ed_find_field(ed, which), &ed->base, R_INVALID_ID, f->size, event->local_x - (frame.x + 6.0f),
				event->clicks, (event->modifiers & R_MOD_SHIFT) != 0);
			ed->find_dragging = R_TRUE;
			rvn_widget_capture_mouse(&ed->base);
			break;
		}
		case RVN_ED_FIND_EXPAND:
			ed->find_open = ed->find_open == 2 ? 1 : 2;
			if (ed->find_open == 1 && ed->find_focus == 2) rvn_ed_find_focus(ed, 1);
			break;
		case RVN_ED_FIND_CASE: rvn_ed_find_toggle(ed, R_SEARCH_MATCH_CASE); break;
		case RVN_ED_FIND_WORD: rvn_ed_find_toggle(ed, R_SEARCH_WHOLE_WORD); break;
		case RVN_ED_FIND_PREVIOUS: rvn_ed_find_step(ed, R_FALSE); break;
		case RVN_ED_FIND_NEXT: rvn_ed_find_step(ed, R_TRUE); break;
		case RVN_ED_FIND_CLOSE: rvn_ed_find_close(ed); break;
		case RVN_ED_FIND_REPLACE_ONE: rvn_ed_find_replace(ed, R_FALSE); break;
		case RVN_ED_FIND_REPLACE_ALL: rvn_ed_find_replace(ed, R_TRUE); break;
		default: break;
	}
	rvn_widget_invalidate_paint(&ed->base);
}

static const char* rvn_ed_find_tip(int part) {
	switch (part) {
		case RVN_ED_FIND_EXPAND: return rvn_ed_text("Raven.FindToggleReplace", "Toggle replace");
		case RVN_ED_FIND_CASE: return rvn_ed_text("Raven.FindMatchCase", "Match case");
		case RVN_ED_FIND_WORD: return rvn_ed_text("Raven.FindWholeWord", "Whole word");
		case RVN_ED_FIND_PREVIOUS: return rvn_ed_text("Raven.FindPrevious", "Previous match (Shift+Enter)");
		case RVN_ED_FIND_NEXT: return rvn_ed_text("Raven.FindNext", "Next match (Enter)");
		case RVN_ED_FIND_CLOSE: return rvn_ed_text("Raven.FindClose", "Close (Escape)");
		default: return NULL;
	}
}

/*
 * The mouse over the bar. Returns R_TRUE when the event was the bar's (the
 * text does not see it).
 */
static int rvn_ed_find_mouse(rvn_text_editor_t* ed, const REvent* event) {
	rvn_widget_t* widget = &ed->base;
	rvn_ed_find_layout_t f;
	rvn_ed_metrics_t m;
	int in_bar, part;

	if (!ed->find_open) return R_FALSE;
	rvn_ed_metrics(ed, &m);
	rvn_ed_find_layout(ed, &m, &f);
	if (ed->find_dragging) {
		if (event->type == R_EVENT_MOUSE_MOVE && ed->find_focus) {
			rvn_rect_t frame = f.parts[ed->find_focus == 1 ? RVN_ED_FIND_QUERY : RVN_ED_FIND_REPLACE];
			rvn_textedit_mouse_drag(rvn_ed_find_field(ed, ed->find_focus), widget, R_INVALID_ID, f.size, event->local_x - (frame.x + 6.0f));
			rvn_widget_invalidate_paint(widget);
		} else if (event->type == R_EVENT_MOUSE_UP) {
			ed->find_dragging = R_FALSE;
			if (ed->find_focus) rvn_ed_find_field(ed, ed->find_focus)->mouse_selecting = R_FALSE;
			rvn_widget_release_mouse(widget);
		}
		return R_TRUE;
	}
	part = rvn_ed_find_hit(ed, event->local_x, event->local_y, &in_bar);
	if (event->type == R_EVENT_MOUSE_MOVE) {
		const char* tip = rvn_ed_find_tip(part);
		if (part != ed->find_hover) {
			ed->find_hover = part;
			rvn_widget_invalidate_paint(widget);
		}
		widget->cursor = (!in_bar || part == RVN_ED_FIND_QUERY || part == RVN_ED_FIND_REPLACE) ? R_CURSOR_IBEAM : R_CURSOR_ARROW;
		if (!in_bar) return R_FALSE;
		/* the tooltip of the editor tells what a button does, as it tells the message of a marker */
		if (tip ? (!widget->tooltip || strcmp(widget->tooltip, tip) != 0) : ed->marker_tip) {
			rvn_str_set(&widget->tooltip, tip);
			ed->marker_tip = tip != NULL;
			if (widget->window) rvn_window_rearm_tooltip(widget->window);
		}
		return R_TRUE;
	}
	if (!in_bar) return R_FALSE;
	if (event->type == R_EVENT_MOUSE_DOWN && event->button == R_MOUSE_LEFT) rvn_ed_find_press(ed, part, event, &f);
	return R_TRUE;
}

/*
 * The keys of the search: Ctrl+F, Ctrl+H, F3, Cmd+G, Escape, and every key
 * while a field of the bar has them. Returns 1 when the key was used, -1
 * when it goes to the shortcuts of the window, 0 when it is for the text.
 */
static int rvn_ed_find_key(rvn_text_editor_t* ed, const REvent* event) {
	rvn_widget_t* widget = &ed->base;
	int shift = (event->modifiers & R_MOD_SHIFT) != 0;
	int command = (event->modifiers & (R_MOD_CTRL | R_MOD_SUPER)) != 0;
	int alt = (event->modifiers & R_MOD_ALT) != 0;
	rvn_textedit_t* edit;
	int result;

	if (ed->no_find_bar) return 0;
	/* Ctrl+F, Cmd+F; Ctrl+H, Cmd+Option+F (Cmd+H hides the application on macOS) */
	if (command && event->key == R_KEY_F) {
		rvn_ed_find_open(ed, alt);
		return 1;
	}
	if (command && !alt && event->key == R_KEY_H) {
		rvn_ed_find_open(ed, R_TRUE);
		return 1;
	}
	if ((event->key == R_KEY_F3 && !command && !alt) || (command && !alt && event->key == R_KEY_G)) {
		rvn_ed_find_step(ed, !shift);
		return 1;
	}
	if (!ed->find_open) return 0;
	if (event->key == R_KEY_ESCAPE) {
		rvn_ed_find_close(ed);
		return 1;
	}
	if (!ed->find_focus) return 0;

	edit = rvn_ed_find_field(ed, ed->find_focus);
	switch (event->key) {
		case R_KEY_ENTER:
		case R_KEY_KP_ENTER:
			if (ed->find_focus == 2) rvn_ed_find_replace(ed, command);
			else rvn_ed_find_step(ed, !shift);
			return 1;
		case R_KEY_TAB:
			if (command || alt) return -1;
			if (ed->find_open == 2) rvn_ed_find_focus(ed, ed->find_focus == 1 ? 2 : 1);
			return 1;
		case R_KEY_Z:
		case R_KEY_Y:
			/* (the history is the text's: nothing to undo in a field) */
			if (command) return 1;
			break;
		default:
			break;
	}
	result = rvn_textedit_key(edit, event);
	rvn_widget_invalidate_paint(widget);
	if (result & RVN_TEXTEDIT_CHANGED) {
		if (ed->find_focus == 1) rvn_ed_find_query_changed(ed);
		return 1;
	}
	if (result & RVN_TEXTEDIT_HANDLED) return 1;
	/* what a field does not use never moves the text */
	return (command || alt) ? -1 : 1;
}

/* A character typed into a field of the bar. Returns R_TRUE when it went there. */
static int rvn_ed_find_text(rvn_text_editor_t* ed, uint32_t codepoint) {
	int result;
	if (!rvn_ed_find_has_keys(ed)) return R_FALSE;
	result = rvn_textedit_input(rvn_ed_find_field(ed, ed->find_focus), codepoint);
	if ((result & RVN_TEXTEDIT_CHANGED) && ed->find_focus == 1) rvn_ed_find_query_changed(ed);
	rvn_widget_invalidate_paint(&ed->base);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Events
 * ------------------------------------------------------------------------- */

static void rvn_ed_blink(Rid timer, void* user_data) {
	rvn_widget_t* widget = rvn_widget_get((Rid)(uintptr_t)user_data);
	RVN_UNUSED(timer);
	if (widget) rvn_widget_invalidate_paint(widget);
}

static void rvn_ed_set_focused(rvn_widget_t* widget, int focused) {
	rvn_text_editor_t* ed = (rvn_text_editor_t*)widget;
	if (focused) {
		ed->blink_start = rvn_clock_seconds();
		if (ed->blink_timer == R_INVALID_ID) ed->blink_timer = Rtimer_create(0.5, R_TRUE, rvn_ed_blink, (void*)(uintptr_t)widget->id);
	} else {
		if (ed->blink_timer != R_INVALID_ID) {
			if (Rtimer_is_valid(ed->blink_timer)) Rtimer_destroy(ed->blink_timer);
			ed->blink_timer = R_INVALID_ID;
		}
		ed->selecting = R_FALSE;
		rvn_ed_break_typing(ed);
	}
	rvn_widget_invalidate_paint(widget);
}

/* Moves the caret to `target`, extending the selection with Shift. */
static void rvn_ed_go(rvn_text_editor_t* ed, rvn_ed_pos_t target, int extend, int keep_goal) {
	ed->caret = rvn_ed_clamp(ed, target);
	if (!extend) ed->anchor = ed->caret;
	rvn_ed_break_typing(ed);
	rvn_ed_complete_close(ed);
	rvn_ed_caret_moved(ed, keep_goal);
}

/* ---------------------------------------------------------------------------
 *  Zoom
 * ------------------------------------------------------------------------- */

#define RVN_ED_ZOOM_MIN 0.5f
#define RVN_ED_ZOOM_MAX 4.0f
/* one notch of the wheel, one press of Ctrl + '+' */
#define RVN_ED_ZOOM_STEP 1.1f

static void rvn_ed_relayout(rvn_text_editor_t* ed);

/*
 * Zooms the text, the point (anchor_x, anchor_y) of the widget staying over
 * the same text: under the pointer for the wheel, at the caret for the keys.
 * Returns R_TRUE when the zoom changed; `notify` calls the zoom callback.
 */
static int rvn_ed_set_zoom(rvn_text_editor_t* ed, float zoom, float anchor_x, float anchor_y, int notify) {
	rvn_widget_t* widget = &ed->base;
	float before_zoom = ed->zoom > 0.0f ? ed->zoom : 1.0f, ax, ay, tx, ty, fx, fy;
	rvn_ed_metrics_t before, after;
	rvn_ed_pos_t pos;
	rvn_vec2_t point;
	zoom = rvn_clampf(zoom, RVN_ED_ZOOM_MIN, RVN_ED_ZOOM_MAX);
	/* (back to 100 % when it passes near it: the wheel finds it again) */
	if (fabsf(zoom - 1.0f) < 0.03f) zoom = 1.0f;
	zoom = floorf(zoom * 100.0f + 0.5f) / 100.0f;
	if (fabsf(zoom - before_zoom) < 0.001f) return R_FALSE;

	rvn_ed_metrics(ed, &before);
	rvn_ed_update_rows(ed, &before);
	ax = rvn_clampf(anchor_x - before.area.x, 0.0f, before.area.w);
	ay = rvn_clampf(anchor_y - before.area.y, 0.0f, before.area.h);
	/*
	 * What is under the anchor: a place in the text, and where in its row (a
	 * fraction of the row, a width in font sizes). Rows would not do: with
	 * word wrap, the lines do not have the same rows at another size.
	 */
	tx = ed->hbar.offset + ax;
	ty = ed->vbar.offset + ay;
	pos = rvn_ed_point_pos(ed, &before, tx, ty);
	point = rvn_ed_pos_point(ed, &before, pos);
	fy = (ty - point.y) / before.line_height;
	fx = (tx - point.x) / before.size;

	ed->zoom = zoom;
	rvn_ed_relayout(ed);
	rvn_ed_metrics(ed, &after);
	rvn_ed_update_rows(ed, &after);
	/* (the gutter of the line numbers grows with the text: measured again) */
	rvn_ed_metrics(ed, &after);
	rvn_ed_update_bars(ed, &after);
	/* the same place of the text under the anchor again */
	point = rvn_ed_pos_point(ed, &after, pos);
	rvn_scrollbar_set(&ed->vbar, point.y + fy * after.line_height - ay);
	if (!ed->wrap) rvn_scrollbar_set(&ed->hbar, point.x + fx * after.size - ax);
	/* no glide from where things were at the other size: they are drawn in their place at once */
	ed->vbar.shown.started = R_FALSE;
	ed->hbar.shown.started = R_FALSE;
	ed->caret_x.started = R_FALSE;
	ed->caret_y.started = R_FALSE;
	rvn_ed_complete_close(ed);
	rvn_widget_invalidate_paint(widget);
	if (notify && ed->zoom_callback) ed->zoom_callback(widget->id, zoom, ed->zoom_user);
	return R_TRUE;
}

/* Ctrl + '+' (or '='), Ctrl + '-', Ctrl + 0, also on the keypad: around the caret, or the middle when it is out of view. */
static int rvn_ed_zoom_key(rvn_text_editor_t* ed, const REvent* event) {
	float zoom = ed->zoom > 0.0f ? ed->zoom : 1.0f, x, y;
	rvn_ed_metrics_t m;
	rvn_vec2_t point;
	int in = event->key == R_KEY_EQUAL || event->key == R_KEY_KP_ADD || event->codepoint == '+' || event->codepoint == '=';
	int out = event->key == R_KEY_MINUS || event->key == R_KEY_KP_SUBTRACT || event->codepoint == '-';
	int reset = event->key == R_KEY_0 || event->key == R_KEY_KP_0 || event->codepoint == '0';
	if (ed->no_zoom || !(event->modifiers & (R_MOD_CTRL | R_MOD_SUPER)) || (event->modifiers & R_MOD_ALT)) return R_FALSE;
	if (!in && !out && !reset) return R_FALSE;
	rvn_ed_metrics(ed, &m);
	rvn_ed_update_rows(ed, &m);
	point = rvn_ed_pos_point(ed, &m, ed->caret);
	x = m.area.x + point.x - ed->hbar.offset;
	y = m.area.y + point.y - ed->vbar.offset;
	if (y < m.area.y || y > m.area.y + m.area.h) {
		x = m.area.x;
		y = m.area.y + m.area.h * 0.5f;
	}
	rvn_ed_set_zoom(ed, reset ? 1.0f : in ? zoom * RVN_ED_ZOOM_STEP : zoom / RVN_ED_ZOOM_STEP, x, y, R_TRUE);
	return R_TRUE;
}

static int rvn_ed_key(rvn_widget_t* widget, const REvent* event) {
	rvn_text_editor_t* ed = (rvn_text_editor_t*)widget;
	int shift = (event->modifiers & R_MOD_SHIFT) != 0;
	int command = (event->modifiers & (R_MOD_CTRL | R_MOD_SUPER)) != 0;
	int alt = (event->modifiers & R_MOD_ALT) != 0;
	int changed = R_FALSE;
	Rid id = widget->id;
	rvn_ed_metrics_t m;
	rvn_ed_pos_t target;
	int find = rvn_ed_find_key(ed, event);

	if (find) return find > 0;
	if (rvn_ed_zoom_key(ed, event)) return R_TRUE;

	rvn_ed_metrics(ed, &m);
	rvn_ed_update_rows(ed, &m);

	switch (event->key) {
		case R_KEY_SPACE:
			if (!command) goto printable;
			rvn_ed_complete_update(widget, 1);
			return R_TRUE;
		case R_KEY_SLASH:
			if (!command) goto printable;
			if (ed->read_only || !rvn_ed_toggle_comment(ed)) return R_TRUE;
			changed = R_TRUE;
			break;
		case R_KEY_LEFT:
			if (!shift && !command && rvn_ed_has_selection(ed)) target = rvn_ed_sel_min(ed);
			else target = command ? rvn_ed_word_left(ed, ed->caret) : rvn_ed_char_left(ed, ed->caret);
			rvn_ed_go(ed, target, shift, R_FALSE);
			return R_TRUE;
		case R_KEY_RIGHT:
			if (!shift && !command && rvn_ed_has_selection(ed)) target = rvn_ed_sel_max(ed);
			else target = command ? rvn_ed_word_right(ed, ed->caret) : rvn_ed_char_right(ed, ed->caret);
			rvn_ed_go(ed, target, shift, R_FALSE);
			return R_TRUE;
		case R_KEY_UP:
		case R_KEY_DOWN:
			/* Alt+Up / Alt+Down are left to the window shortcuts (previous / next...) */
			if (alt) return R_FALSE;
			if (command) {
				/* Ctrl + arrows scroll without moving the caret */
				rvn_scrollbar_wheel(&ed->vbar, event->key == R_KEY_UP ? -m.line_height : m.line_height);
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			rvn_ed_go(ed, rvn_ed_move_rows(ed, &m, ed->caret, event->key == R_KEY_UP ? -1 : 1), shift, R_TRUE);
			return R_TRUE;
		case R_KEY_PAGE_UP:
		case R_KEY_PAGE_DOWN: {
			int page = rvn_maxi((int)(m.area.h / m.line_height) - 1, 1);
			rvn_ed_go(ed, rvn_ed_move_rows(ed, &m, ed->caret, event->key == R_KEY_PAGE_UP ? -page : page), shift, R_TRUE);
			return R_TRUE;
		}
		case R_KEY_HOME:
			if (command) {
				target = rvn_ed_pos(0, 0);
			} else {
				/* the start of the row, or the first character that is not a space when already there */
				const rvn_ed_line_t* line = &ed->lines[ed->caret.line];
				int begin, end, indent;
				rvn_ed_row_range(ed, ed->caret.line, rvn_ed_row_of(ed, ed->caret.line, ed->caret.column), &begin, &end);
				indent = begin;
				while (indent < end && (line->text[indent] == ' ' || line->text[indent] == '\t')) indent++;
				target = rvn_ed_pos(ed->caret.line, ed->caret.column == indent || indent == end ? begin : indent);
			}
			rvn_ed_go(ed, target, shift, R_FALSE);
			return R_TRUE;
		case R_KEY_END:
			if (command) {
				target = rvn_ed_end(ed);
			} else {
				int begin, end, row = rvn_ed_row_of(ed, ed->caret.line, ed->caret.column);
				rvn_ed_row_range(ed, ed->caret.line, row, &begin, &end);
				/* (the last position of a wrapped row is before its cut) */
				if (ed->wrap && row + 1 < ed->lines[ed->caret.line].row_count && end > begin) {
					end = rvn_utf8_prev(ed->lines[ed->caret.line].text, end);
				}
				target = rvn_ed_pos(ed->caret.line, end);
			}
			rvn_ed_go(ed, target, shift, R_FALSE);
			return R_TRUE;
		case R_KEY_BACKSPACE:
		case R_KEY_DELETE:
			if (ed->read_only) return R_TRUE;
			if (!rvn_ed_has_selection(ed)) {
				if (event->key == R_KEY_BACKSPACE) ed->anchor = command ? rvn_ed_word_left(ed, ed->caret) : rvn_ed_char_left(ed, ed->caret);
				else ed->anchor = command ? rvn_ed_word_right(ed, ed->caret) : rvn_ed_char_right(ed, ed->caret);
				if (event->key == R_KEY_BACKSPACE && !command && rvn_ed_between_pair(ed)) {
					/* a pair with nothing in it goes away as it came: together */
					ed->caret = rvn_ed_char_right(ed, ed->caret);
				}
			}
			rvn_ed_break_typing(ed);
			changed = rvn_ed_delete_selection(ed);
			break;
		case R_KEY_ENTER:
		case R_KEY_KP_ENTER:
			/* Ctrl/Alt+Enter is left to the window shortcuts (send, run...) */
			if (command || alt) return R_FALSE;
			if (ed->read_only) return R_TRUE;
			rvn_ed_new_line(ed);
			changed = R_TRUE;
			break;
		case R_KEY_TAB:
			if (command || alt) return R_FALSE;
			if (ed->read_only) return R_FALSE;
			if (shift || rvn_ed_sel_min(ed).line != rvn_ed_sel_max(ed).line) {
				rvn_ed_indent_lines(ed, shift);
			} else if (ed->tab_spaces) {
				/* up to the next tab stop */
				char spaces[16];
				const char* line_text = ed->lines[rvn_ed_sel_min(ed).line].text;
				int count = rvn_clampi(ed->tab_size, 1, 16);
				int columns = 0, at = 0, stop = rvn_ed_sel_min(ed).column;
				/* the column on screen: a tab before the caret counts for what it shows */
				while (at < stop) {
					if (line_text[at] == '\t') columns += count - columns % count;
					else columns++;
					at = rvn_utf8_next(line_text, stop, at);
				}
				count -= columns % count;
				memset(spaces, ' ', (size_t)count);
				rvn_ed_break_typing(ed);
				rvn_ed_insert(ed, spaces, count);
			} else {
				rvn_ed_break_typing(ed);
				rvn_ed_insert(ed, "\t", 1);
			}
			changed = R_TRUE;
			break;
		case R_KEY_A:
			if (!command) goto printable;
			ed->anchor = rvn_ed_pos(0, 0);
			ed->caret = rvn_ed_end(ed);
			rvn_ed_break_typing(ed);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_KEY_C:
			if (!command) goto printable;
			rvn_ed_copy(ed);
			return R_TRUE;
		case R_KEY_X:
			if (!command) goto printable;
			rvn_ed_copy(ed);
			if (ed->read_only) return R_TRUE;
			rvn_ed_break_typing(ed);
			changed = rvn_ed_delete_selection(ed);
			break;
		case R_KEY_V: {
			const char* clip;
			if (!command) goto printable;
			if (ed->read_only) return R_TRUE;
			clip = Rclipboard_get_text();
			if (!clip || !clip[0]) return R_TRUE;
			rvn_ed_break_typing(ed);
			rvn_ed_insert(ed, clip, -1);
			rvn_ed_break_typing(ed);
			changed = R_TRUE;
			break;
		}
		case R_KEY_Z:
			if (!command) goto printable;
			changed = shift ? rvn_ed_redo(ed) : rvn_ed_undo(ed);
			if (!changed) return R_TRUE;
			break;
		case R_KEY_Y:
			if (!command) goto printable;
			changed = rvn_ed_redo(ed);
			if (!changed) return R_TRUE;
			break;
		default:
		printable:
			/* keys that type a character are consumed so that they never trigger
			 * a window shortcut; the character itself arrives as a text event */
			if (command || alt || (int)event->key >= 256 || event->key == R_KEY_UNKNOWN) return R_FALSE;
			return R_TRUE;
	}
	rvn_ed_caret_moved(ed, R_FALSE);
	if (changed) rvn_ed_notify(widget);
	if (!rvn_widget_get(id)) return R_TRUE;
	/* erasing narrows the list of proposals back; anything else is another matter */
	if (event->key == R_KEY_BACKSPACE || event->key == R_KEY_DELETE) {
		if (rvn_ed_completing(ed)) rvn_ed_complete_update(widget, 2);
	} else {
		rvn_ed_complete_close(ed);
	}
	return R_TRUE;
}

static int rvn_text_editor_event(rvn_widget_t* widget, const REvent* event) {
	rvn_text_editor_t* ed = (rvn_text_editor_t*)widget;
	rvn_ed_metrics_t m;
	rvn_rect_t vertical, horizontal;
	int changed = R_FALSE;

	switch (event->type) {
		case R_EVENT_FOCUS_GAINED:
			rvn_ed_set_focused(widget, R_TRUE);
			return R_TRUE;
		case R_EVENT_FOCUS_LOST:
			rvn_ed_complete_close(ed);
			rvn_ed_set_focused(widget, R_FALSE);
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			if (!(widget->flags & RVN_WF_FOCUSED)) return R_FALSE;
			return rvn_ed_key(widget, event);
		case R_EVENT_TEXT: {
			Rid id = widget->id;
			uint32_t version = ed->version;
			if (!(widget->flags & RVN_WF_FOCUSED)) return R_FALSE;
			if (event->codepoint < 32 || event->codepoint == 127) return R_FALSE;
			/* (a field of the find bar takes what is typed, even in a read-only text) */
			if (rvn_ed_find_text(ed, event->codepoint)) return R_TRUE;
			if (ed->read_only) return R_FALSE;
			rvn_ed_type(ed, event->codepoint);
			rvn_ed_caret_moved(ed, R_FALSE);
			if (ed->version != version) rvn_ed_notify(widget);
			if (!rvn_widget_get(id)) return R_TRUE;
			/* each character of a word narrows the proposals down; anything else ends the word */
			if (event->codepoint >= 128 || rvn_ed_is_word_char((char)event->codepoint)) {
				rvn_ed_complete_update(widget, rvn_ed_completing(ed) ? 2 : 0);
			} else {
				rvn_ed_complete_close(ed);
			}
			return R_TRUE;
		}
		case R_EVENT_MOUSE_WHEEL:
			rvn_ed_complete_close(ed);
			/* Ctrl + wheel zooms, around the pointer; it never scrolls what contains the editor */
			if ((event->modifiers & R_MOD_CTRL) && !ed->no_zoom) {
				float zoom = ed->zoom > 0.0f ? ed->zoom : 1.0f;
				if (event->wheel_y != 0.0f) rvn_ed_set_zoom(ed, zoom * powf(RVN_ED_ZOOM_STEP, event->wheel_y), event->local_x, event->local_y, R_TRUE);
				return R_TRUE;
			}
			rvn_ed_metrics(ed, &m);
			if ((event->modifiers & R_MOD_SHIFT) && !ed->wrap) {
				changed = rvn_scrollbar_wheel(&ed->hbar, -event->wheel_y * m.space * 12.0f);
			} else {
				changed = rvn_scrollbar_wheel(&ed->vbar, -event->wheel_y * m.line_height * 3.0f);
				if (event->wheel_x != 0.0f && !ed->wrap) changed |= rvn_scrollbar_wheel(&ed->hbar, -event->wheel_x * m.space * 12.0f);
			}
			rvn_widget_invalidate_paint(widget);
			/* at the end of its range, the wheel goes to what contains the editor */
			return changed;
		case R_EVENT_MOUSE_DOWN:
		case R_EVENT_MOUSE_MOVE:
		case R_EVENT_MOUSE_UP: {
			rvn_ed_pos_t pos;
			int used;
			/* the find bar lies over the text and its scrollbar */
			if (!ed->selecting && rvn_ed_find_mouse(ed, event)) return R_TRUE;
			rvn_ed_metrics(ed, &m);
			rvn_ed_update_rows(ed, &m);
			rvn_ed_tracks(ed, &m, &vertical, &horizontal);
			if (!ed->selecting) {
				used = rvn_scrollbar_event(&ed->vbar, widget, event, vertical, R_TRUE, &changed);
				if (!used && !ed->wrap) used = rvn_scrollbar_event(&ed->hbar, widget, event, horizontal, R_FALSE, &changed);
				if (used) {
					rvn_widget_invalidate_paint(widget);
					return R_TRUE;
				}
			}
			if (event->type == R_EVENT_MOUSE_UP) {
				if (event->button != R_MOUSE_LEFT || !ed->selecting) return R_FALSE;
				ed->selecting = R_FALSE;
				rvn_widget_release_mouse(widget);
				return R_TRUE;
			}
			if (event->type == R_EVENT_MOUSE_MOVE && !ed->selecting) {
				if (ed->marker_count > 0 || ed->marker_tip || ed->hover_callback) {
					/* the message of the marker under the pointer is the tooltip of the editor; elsewhere on the
					   text, what the hover callback says */
					const rvn_ed_marker_t* marker = NULL;
					const char* message;
					int over_text = R_FALSE;
					if (rvn_rect_contains(m.area, event->local_x, event->local_y)) {
						float tx = event->local_x - m.area.x + ed->hbar.offset, ty = event->local_y - m.area.y + ed->vbar.offset;
						rvn_vec2_t point;
						pos = rvn_ed_point_pos(ed, &m, tx, ty);
						point = rvn_ed_pos_point(ed, &m, pos);
						/* (the nearest position is not under the pointer when it is past the end of the line, or below the text) */
						if (ty >= point.y && ty < point.y + m.line_height && fabsf(tx - point.x) <= m.space) {
							marker = rvn_ed_marker_at(ed, pos);
							over_text = R_TRUE;
						}
					}
					message = marker ? marker->message : NULL;
					if (!message && over_text && ed->hover_callback) {
						Rid id = widget->id;
						message = ed->hover_callback(id, pos.line, pos.column, ed->hover_user);
						if (!rvn_widget_get(id)) return R_TRUE;
						if (message && !message[0]) message = NULL;
					}
					if (message ? (!widget->tooltip || strcmp(widget->tooltip, message) != 0) : ed->marker_tip) {
						rvn_str_set(&widget->tooltip, message);
						ed->marker_tip = message != NULL;
						if (widget->window) rvn_window_rearm_tooltip(widget->window);
					}
				}
				/* the scrollbars react to the pointer */
				rvn_widget_invalidate_paint(widget);
				return R_FALSE;
			}
			if (event->type == R_EVENT_MOUSE_DOWN && event->button != R_MOUSE_LEFT) return R_FALSE;

			pos = rvn_ed_point_pos(ed, &m, event->local_x - m.area.x + ed->hbar.offset, event->local_y - m.area.y + ed->vbar.offset);
			if (event->type == R_EVENT_MOUSE_DOWN) {
				/* a click on the text gives it back the keys */
				rvn_ed_find_focus(ed, 0);
				if (event->clicks >= 3) {
					/* the whole line, with its line break */
					ed->anchor = rvn_ed_pos(pos.line, 0);
					ed->caret = pos.line + 1 < (int)ed->line_count ? rvn_ed_pos(pos.line + 1, 0) : rvn_ed_pos(pos.line, ed->lines[pos.line].length);
				} else if (event->clicks == 2) {
					const rvn_ed_line_t* line = &ed->lines[pos.line];
					int begin = pos.column, end = pos.column;
					if (line->length > 0) {
						rvn_ed_class_t kind;
						if (begin >= line->length) begin = line->length - 1;
						kind = rvn_ed_class(line->text[begin]);
						end = begin;
						while (begin > 0 && rvn_ed_class(line->text[begin - 1]) == kind) begin--;
						while (end < line->length && rvn_ed_class(line->text[end]) == kind) end++;
					}
					ed->anchor = rvn_ed_pos(pos.line, begin);
					ed->caret = rvn_ed_pos(pos.line, end);
				} else {
					ed->caret = pos;
					if (!(event->modifiers & R_MOD_SHIFT)) ed->anchor = pos;
					ed->selecting = R_TRUE;
					rvn_widget_capture_mouse(widget);
				}
				rvn_ed_complete_close(ed);
				rvn_ed_break_typing(ed);
			} else {
				ed->caret = pos;
			}
			rvn_ed_caret_moved(ed, R_FALSE);
			return R_TRUE;
		}
		default:
			return R_FALSE;
	}
}

/* ---------------------------------------------------------------------------
 *  Class
 * ------------------------------------------------------------------------- */

static void rvn_ed_set_text(rvn_text_editor_t* ed, const char* text) {
	rvn_ed_history_clear(ed->undo, &ed->undo_count);
	rvn_ed_history_clear(ed->redo, &ed->redo_count);
	rvn_ed_replace(ed, rvn_ed_pos(0, 0), rvn_ed_end(ed), text ? text : "", -1, R_FALSE);
	ed->caret = ed->anchor = rvn_ed_pos(0, 0);
	ed->vbar.offset = 0.0f;
	ed->hbar.offset = 0.0f;
	ed->goal_x = -1.0f;
	rvn_widget_invalidate_layout(&ed->base);
}

static void rvn_text_editor_init(rvn_widget_t* widget) {
	rvn_text_editor_t* ed = (rvn_text_editor_t*)widget;
	ed->tab_size = 4;
	ed->font = R_INVALID_ID;
	ed->goal_x = -1.0f;
	ed->blink_timer = R_INVALID_ID;
	ed->rows_dirty = R_TRUE;
	rvn_ed_lines_insert(ed, 0, 1);
	widget->cursor = R_CURSOR_IBEAM;
}

static void rvn_text_editor_destroy(rvn_widget_t* widget) {
	rvn_text_editor_t* ed = (rvn_text_editor_t*)widget;
	uint32_t i;
	if (ed->blink_timer != R_INVALID_ID && Rtimer_is_valid(ed->blink_timer)) Rtimer_destroy(ed->blink_timer);
	for (i = 0; i < ed->line_count; i++) rvn_ed_line_free(&ed->lines[i]);
	rvn_free(ed->lines);
	rvn_free(ed->row_starts);
	rvn_ed_history_clear(ed->undo, &ed->undo_count);
	rvn_ed_history_clear(ed->redo, &ed->redo_count);
	rvn_free(ed->undo);
	rvn_free(ed->redo);
	rvn_free(ed->placeholder);
	rvn_free(ed->text_cache);
	rvn_free(ed->selection_cache);
	rvn_free(ed->language_name);
	for (i = 0; i < ed->marker_count; i++) rvn_free(ed->markers[i].message);
	rvn_free(ed->markers);
	if (ed->complete) rvn_widget_destroy(ed->complete);
	rvn_free(ed->search.query);
	rvn_free(ed->search.folded);
	if (ed->find_ready) {
		rvn_textedit_free(&ed->find_query);
		rvn_textedit_free(&ed->find_replace);
	}
}

/* Gives the editor a language by name; NULL, "" or "None" removes it. Returns R_FALSE when the name is unknown. */
static int rvn_ed_set_language(rvn_text_editor_t* ed, const char* name) {
	const rvn_language_t* language = NULL;
	if (name && name[0] && rvn_stricmp(name, "None") != 0) {
		language = rvn_language_find(name);
		if (!language) return R_FALSE;
	}
	rvn_str_set(&ed->language_name, language ? rvn_language_desc(language)->name : NULL);
	/* (looked for again, and everything that was computed with the other one with it) */
	ed->language_generation = rvn_language_generation() - 1u;
	ed->lex_valid = 0;
	ed->match_version = ed->version - 1u;
	rvn_ed_complete_close(ed);
	return R_TRUE;
}

static int rvn_text_editor_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_text_editor_t* ed = (rvn_text_editor_t*)widget;
	float number;
	int flag;
	if (RVN_ATTR("Text")) {
		rvn_ed_set_text(ed, value);
		return R_TRUE;
	}
	if (RVN_ATTR("Placeholder")) {
		rvn_str_set(&ed->placeholder, value);
	} else if (RVN_ATTR("ReadOnly")) {
		if (!rvn_parse_bool(value, &ed->read_only)) return R_FALSE;
	} else if (RVN_ATTR("Wrap") || RVN_ATTR("WordWrap")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		Rtext_editor_set_word_wrap(widget->id, flag);
	} else if (RVN_ATTR("LineNumbers")) {
		if (!rvn_parse_bool(value, &ed->line_numbers)) return R_FALSE;
	} else if (RVN_ATTR("TabSize")) {
		if (!rvn_parse_float(value, &number) || number < 1.0f || number > 16.0f) return R_FALSE;
		ed->tab_size = (int)number;
	} else if (RVN_ATTR("TabSpaces")) {
		if (!rvn_parse_bool(value, &ed->tab_spaces)) return R_FALSE;
	} else if (RVN_ATTR("Language") || RVN_ATTR("Syntax")) {
		if (!rvn_ed_set_language(ed, value)) return R_FALSE;
	} else if (RVN_ATTR("Markdown")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		if (!rvn_ed_set_language(ed, flag ? "Markdown" : "")) return R_FALSE;
	} else if (RVN_ATTR("Completion")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		ed->no_completion = !flag;
	} else if (RVN_ATTR("AutoPairs")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		ed->no_pairs = !flag;
	} else if (RVN_ATTR("Font")) {
		Rid font = rvn_xml_font(value);
		if (font == R_INVALID_ID) return R_FALSE;
		ed->font = font;
	} else if (RVN_ATTR("FontSize") || RVN_ATTR("TextSize")) {
		if (!rvn_parse_float(value, &number) || number < 0.0f) return R_FALSE;
		ed->font_size = number;
	} else if (RVN_ATTR("Zoom")) {
		if (!rvn_parse_float(value, &number) || number <= 0.0f) return R_FALSE;
		ed->zoom = rvn_clampf(number, RVN_ED_ZOOM_MIN, RVN_ED_ZOOM_MAX);
	} else if (RVN_ATTR("Zoomable")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		ed->no_zoom = !flag;
	} else if (RVN_ATTR("FindBar")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		Rtext_editor_set_find_bar(widget->id, flag);
		return R_TRUE;
	} else if (RVN_ATTR("Search")) {
		rvn_ed_search_set(ed, value, ed->search.flags);
		return R_TRUE;
	} else {
		return R_FALSE;
	}
	/* anything above may change the size of the rows */
	{
		uint32_t i;
		for (i = 0; i < ed->line_count; i++) rvn_ed_line_dirty(&ed->lines[i]);
		ed->rows_dirty = R_TRUE;
	}
	rvn_widget_invalidate_layout(widget);
	return R_TRUE;
}

/* A finger scrolls the text; two fingers zoom it, as Ctrl + wheel does. */
static int rvn_text_editor_pan(rvn_widget_t* widget, float dx, float dy, int coasting) {
	rvn_text_editor_t* ed = (rvn_text_editor_t*)widget;
	int moved = rvn_scrollbar_pan(&ed->vbar, -dy, coasting);
	if (!ed->wrap) moved |= rvn_scrollbar_pan(&ed->hbar, -dx, coasting);
	if (moved) rvn_ed_complete_close(ed);
	return moved;
}

static int rvn_text_editor_zoom(rvn_widget_t* widget, float factor, float x, float y) {
	rvn_text_editor_t* ed = (rvn_text_editor_t*)widget;
	float zoom = ed->zoom > 0.0f ? ed->zoom : 1.0f;
	if (ed->no_zoom) return R_FALSE;
	rvn_ed_complete_close(ed);
	return rvn_ed_set_zoom(ed, zoom * factor, x, y, R_TRUE);
}

const rvn_widget_class_t rvn_class_text_editor = {
	.type = R_WIDGET_TEXT_EDITOR,
	.name = "TextEditor",
	.size = sizeof(rvn_text_editor_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_SURFACE | RVN_CF_NO_MIRROR,
	.init = rvn_text_editor_init,
	.destroy = rvn_text_editor_destroy,
	.measure = rvn_text_editor_measure,
	.arrange = rvn_text_editor_arrange,
	.paint = rvn_text_editor_paint,
	.event = rvn_text_editor_event,
	.set_attribute = rvn_text_editor_set_attribute,
	.pan = rvn_text_editor_pan,
	.zoom = rvn_text_editor_zoom
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

#define RVN_EDITOR(variable, id, result) \
	rvn_widget_t* widget = rvn_widget_check((id), R_WIDGET_TEXT_EDITOR, __func__); \
	rvn_text_editor_t* variable = (rvn_text_editor_t*)widget; \
	if (!widget) return result

#define RVN_EDITOR_VOID(variable, id) \
	rvn_widget_t* widget = rvn_widget_check((id), R_WIDGET_TEXT_EDITOR, __func__); \
	rvn_text_editor_t* variable = (rvn_text_editor_t*)widget; \
	if (!widget) return

/* An option that changes how the lines are laid out. */
static void rvn_ed_relayout(rvn_text_editor_t* ed) {
	uint32_t i;
	for (i = 0; i < ed->line_count; i++) rvn_ed_line_dirty(&ed->lines[i]);
	ed->rows_dirty = R_TRUE;
	rvn_widget_invalidate_layout(&ed->base);
}

Rid Rtext_editor_create(const char* text) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_TEXT_EDITOR);
	if (!widget) return R_INVALID_ID;
	if (text && text[0]) rvn_ed_set_text((rvn_text_editor_t*)widget, text);
	return widget->id;
}

void Rtext_editor_set_text(Rid id, const char* text) {
	char* copy;
	RVN_EDITOR_VOID(ed, id);
	/* the text may be one of the editor's own lines (Rtext_editor_get_line), which the edit frees or rewrites */
	copy = text ? rvn_strdup(text) : NULL;
	rvn_ed_set_text(ed, copy);
	rvn_free(copy);
}

const char* Rtext_editor_get_text(Rid id) {
	RVN_EDITOR(ed, id, "");
	if (!ed->text_cache_valid) {
		rvn_free(ed->text_cache);
		ed->text_cache = rvn_ed_range(ed, rvn_ed_pos(0, 0), rvn_ed_end(ed), &ed->text_cache_length);
		ed->text_cache_valid = R_TRUE;
	}
	return ed->text_cache;
}

int Rtext_editor_get_length(Rid id) {
	if (!rvn_widget_check(id, R_WIDGET_TEXT_EDITOR, __func__)) return 0;
	Rtext_editor_get_text(id);
	return ((rvn_text_editor_t*)rvn_widget_get(id))->text_cache_length;
}

int Rtext_editor_get_line_count(Rid id) {
	RVN_EDITOR(ed, id, 0);
	return (int)ed->line_count;
}

const char* Rtext_editor_get_line(Rid id, int line) {
	RVN_EDITOR(ed, id, NULL);
	if (line < 0 || line >= (int)ed->line_count) return NULL;
	return ed->lines[line].text;
}

void Rtext_editor_insert_text(Rid id, const char* text) {
	char* copy;
	RVN_EDITOR_VOID(ed, id);
	if (!text) return;
	/* as in Rtext_editor_set_text, the text may live in a line that the edit changes */
	copy = rvn_strdup(text);
	rvn_ed_break_typing(ed);
	rvn_ed_insert(ed, copy, -1);
	rvn_free(copy);
	rvn_ed_break_typing(ed);
	rvn_ed_caret_moved(ed, R_FALSE);
	rvn_ed_notify(widget);
}

void Rtext_editor_set_cursor(Rid id, int line, int column) {
	RVN_EDITOR_VOID(ed, id);
	ed->caret = ed->anchor = rvn_ed_clamp(ed, rvn_ed_pos(line, column));
	rvn_ed_break_typing(ed);
	rvn_ed_caret_moved(ed, R_FALSE);
}

void Rtext_editor_get_cursor(Rid id, int* line, int* column) {
	rvn_widget_t* widget;
	if (line) *line = 0;
	if (column) *column = 0;
	widget = rvn_widget_check(id, R_WIDGET_TEXT_EDITOR, __func__);
	if (!widget) return;
	if (line) *line = ((rvn_text_editor_t*)widget)->caret.line;
	if (column) *column = ((rvn_text_editor_t*)widget)->caret.column;
}

void Rtext_editor_select(Rid id, int from_line, int from_column, int to_line, int to_column) {
	RVN_EDITOR_VOID(ed, id);
	ed->anchor = rvn_ed_clamp(ed, rvn_ed_pos(from_line, from_column));
	ed->caret = rvn_ed_clamp(ed, rvn_ed_pos(to_line, to_column));
	rvn_ed_break_typing(ed);
	rvn_ed_caret_moved(ed, R_FALSE);
}

void Rtext_editor_select_all(Rid id) {
	RVN_EDITOR_VOID(ed, id);
	ed->anchor = rvn_ed_pos(0, 0);
	ed->caret = rvn_ed_end(ed);
	rvn_ed_break_typing(ed);
	rvn_widget_invalidate_paint(widget);
}

int Rtext_editor_get_selection(Rid id, int* from_line, int* from_column, int* to_line, int* to_column) {
	rvn_ed_pos_t from, to;
	RVN_EDITOR(ed, id, R_FALSE);
	from = rvn_ed_sel_min(ed);
	to = rvn_ed_sel_max(ed);
	if (from_line) *from_line = from.line;
	if (from_column) *from_column = from.column;
	if (to_line) *to_line = to.line;
	if (to_column) *to_column = to.column;
	return rvn_ed_has_selection(ed);
}

const char* Rtext_editor_get_selected_text(Rid id) {
	RVN_EDITOR(ed, id, "");
	rvn_free(ed->selection_cache);
	ed->selection_cache = rvn_ed_range(ed, rvn_ed_sel_min(ed), rvn_ed_sel_max(ed), NULL);
	return ed->selection_cache;
}

void Rtext_editor_scroll_to_line(Rid id, int line) {
	rvn_ed_metrics_t m;
	rvn_vec2_t point;
	RVN_EDITOR_VOID(ed, id);
	if (widget->window && widget->window->needs_layout) rvn_window_layout(widget->window);
	rvn_ed_metrics(ed, &m);
	rvn_ed_update_rows(ed, &m);
	rvn_ed_update_bars(ed, &m);
	point = rvn_ed_pos_point(ed, &m, rvn_ed_clamp(ed, rvn_ed_pos(line, 0)));
	/* a third of the way down: the lines before it give some context */
	rvn_scrollbar_set(&ed->vbar, point.y - ed->vbar.viewport * 0.33f);
	rvn_widget_invalidate_paint(widget);
}

void Rtext_editor_undo(Rid id) {
	RVN_EDITOR_VOID(ed, id);
	if (!rvn_ed_undo(ed)) return;
	rvn_ed_caret_moved(ed, R_FALSE);
	rvn_ed_notify(widget);
}

void Rtext_editor_redo(Rid id) {
	RVN_EDITOR_VOID(ed, id);
	if (!rvn_ed_redo(ed)) return;
	rvn_ed_caret_moved(ed, R_FALSE);
	rvn_ed_notify(widget);
}

int Rtext_editor_can_undo(Rid id) {
	RVN_EDITOR(ed, id, R_FALSE);
	return ed->undo_count > 0 && !ed->read_only;
}

int Rtext_editor_can_redo(Rid id) {
	RVN_EDITOR(ed, id, R_FALSE);
	return ed->redo_count > 0 && !ed->read_only;
}

void Rtext_editor_set_read_only(Rid id, int read_only) {
	RVN_EDITOR_VOID(ed, id);
	ed->read_only = read_only != 0;
	rvn_widget_invalidate_paint(widget);
}

void Rtext_editor_set_word_wrap(Rid id, int enabled) {
	RVN_EDITOR_VOID(ed, id);
	if (ed->wrap == (enabled != 0)) return;
	ed->wrap = enabled != 0;
	ed->wrap_width = -1.0f;
	ed->hbar.offset = 0.0f;
	rvn_ed_relayout(ed);
}

void Rtext_editor_set_line_numbers(Rid id, int enabled) {
	RVN_EDITOR_VOID(ed, id);
	ed->line_numbers = enabled != 0;
	rvn_ed_relayout(ed);
}

void Rtext_editor_set_tab_size(Rid id, int spaces) {
	RVN_EDITOR_VOID(ed, id);
	if (spaces < 1 || spaces > 16) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtext_editor_set_tab_size: the size must be between 1 and 16");
		return;
	}
	ed->tab_size = spaces;
	rvn_ed_relayout(ed);
}

void Rtext_editor_set_tab_inserts_spaces(Rid id, int enabled) {
	RVN_EDITOR_VOID(ed, id);
	ed->tab_spaces = enabled != 0;
}

void Rtext_editor_set_syntax(Rid id, RSyntax syntax) {
	RVN_EDITOR_VOID(ed, id);
	if (syntax < R_SYNTAX_NONE || syntax > R_SYNTAX_C) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtext_editor_set_syntax: invalid syntax");
		return;
	}
	rvn_ed_set_language(ed, syntax == R_SYNTAX_XML ? "XML" : syntax == R_SYNTAX_C ? "C" : NULL);
	/* (the font may change with it) */
	rvn_ed_relayout(ed);
}

void Rtext_editor_set_language(Rid id, const char* name) {
	RVN_EDITOR_VOID(ed, id);
	if (!rvn_ed_set_language(ed, name)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_WARNING, "Rtext_editor_set_language: unknown language \"%s\"", name);
		return;
	}
	rvn_ed_relayout(ed);
}

void Rtext_editor_set_markdown(Rid id, int enabled) {
	RVN_EDITOR_VOID(ed, id);
	rvn_ed_set_language(ed, enabled ? "Markdown" : "");
	rvn_ed_relayout(ed);
}

int Rtext_editor_get_markdown(Rid id) {
	const rvn_language_t* language;
	RVN_EDITOR(ed, id, R_FALSE);
	language = rvn_ed_language(ed);
	return language && (rvn_language_desc(language)->flags & R_LANGUAGE_FORMATTED_TEXT) != 0;
}

const char* Rtext_editor_get_language(Rid id) {
	RVN_EDITOR(ed, id, "");
	return ed->language_name ? ed->language_name : "";
}

RToken Rtext_editor_get_token(Rid id, int line, int column) {
	rvn_span_t spans[RVN_ED_MAX_SPANS];
	const rvn_language_t* language;
	int count = 0, i;
	RVN_EDITOR(ed, id, R_TOKEN_TEXT);
	language = rvn_ed_language(ed);
	if (!language || line < 0 || line >= (int)ed->line_count || column < 0 || column >= ed->lines[line].length) return R_TOKEN_TEXT;
	rvn_ed_lex_until(ed, (uint32_t)line);
	rvn_syntax_line(language, ed->lines[line].text, ed->lines[line].length, ed->lines[line].lex_in, spans, RVN_ED_MAX_SPANS, &count);
	for (i = 0; i < count; i++) {
		if (column >= spans[i].start && column < spans[i].end) return rvn_token_public((rvn_token_t)spans[i].token);
	}
	return R_TOKEN_TEXT;
}

void Rtext_editor_set_completion(Rid id, int enabled) {
	RVN_EDITOR_VOID(ed, id);
	ed->no_completion = !enabled;
	if (!enabled) rvn_ed_complete_close(ed);
}

void Rtext_editor_show_completions(Rid id) {
	RVN_EDITOR_VOID(ed, id);
	RVN_UNUSED(ed);
	if (widget->window && widget->window->needs_layout) rvn_window_layout(widget->window);
	rvn_ed_complete_update(widget, 1);
}

int Rtext_editor_is_completing(Rid id) {
	RVN_EDITOR(ed, id, R_FALSE);
	return rvn_ed_completing(ed);
}

void Rtext_editor_set_completion_callback(Rid id, RCompletionCallback callback, void* user_data) {
	RVN_EDITOR_VOID(ed, id);
	ed->completion_callback = callback;
	ed->completion_user = user_data;
}

void Rtext_editor_add_completion(Rid id, const char* text, const char* detail) {
	rvn_ed_complete_t* list;
	RVN_EDITOR_VOID(ed, id);
	list = (rvn_ed_complete_t*)ed->complete;
	if (!list || !list->collecting) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rtext_editor_add_completion: only from a completion callback");
		return;
	}
	if (!text || !text[0]) return;
	/* (only what continues the word being typed) */
	if ((int)strlen(text) < list->prefix_length || rvn_strnicmp(text, ed->lines[list->start.line].text + list->start.column,
		(size_t)list->prefix_length) != 0) return;
	rvn_edc_add(list, text, (int)strlen(text), detail && detail[0] ? detail : NULL, 0);
}

void Rtext_editor_add_completion_insert(Rid id, const char* text, const char* detail, const char* insert) {
	rvn_ed_complete_t* list;
	uint32_t count;
	RVN_EDITOR_VOID(ed, id);
	list = (rvn_ed_complete_t*)ed->complete;
	if (!list || !list->collecting) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rtext_editor_add_completion_insert: only from a completion callback");
		return;
	}
	count = list->count;
	Rtext_editor_add_completion(id, text, detail);
	if (list->count > count && insert && insert[0]) list->items[list->count - 1].insert = rvn_strdup(insert);
}

void Rtext_editor_set_completion_words(Rid id, int enabled) {
	RVN_EDITOR_VOID(ed, id);
	ed->no_completion_words = !enabled;
}

void Rtext_editor_set_hover_callback(Rid id, RTextEditorHoverCallback callback, void* user_data) {
	RVN_EDITOR_VOID(ed, id);
	ed->hover_callback = callback;
	ed->hover_user = user_data;
	if (!callback && ed->marker_tip) {
		rvn_str_set(&widget->tooltip, NULL);
		ed->marker_tip = R_FALSE;
		if (widget->window) rvn_window_rearm_tooltip(widget->window);
	}
}

void Rtext_editor_set_auto_pairs(Rid id, int enabled) {
	RVN_EDITOR_VOID(ed, id);
	ed->no_pairs = !enabled;
}

void Rtext_editor_add_marker(Rid id, int line, int from_column, int to_column, RMarkerKind kind, const char* message) {
	rvn_ed_marker_t* marker;
	RVN_EDITOR_VOID(ed, id);
	if (kind < R_MARKER_ERROR || kind > R_MARKER_INFO) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtext_editor_add_marker: invalid kind");
		return;
	}
	line = rvn_clampi(line, 0, (int)ed->line_count - 1);
	from_column = rvn_clampi(from_column, 0, ed->lines[line].length);
	if (to_column >= 0) to_column = rvn_clampi(to_column, from_column, ed->lines[line].length);
	RVN_ARRAY_RESERVE(ed->markers, ed->marker_cap, ed->marker_count + 1);
	marker = &ed->markers[ed->marker_count++];
	marker->line = line;
	marker->from = from_column;
	marker->to = to_column < 0 ? -1 : to_column;
	marker->kind = kind;
	marker->message = message && message[0] ? rvn_strdup(message) : NULL;
	rvn_widget_invalidate_paint(widget);
}

void Rtext_editor_clear_markers(Rid id) {
	uint32_t i;
	RVN_EDITOR_VOID(ed, id);
	for (i = 0; i < ed->marker_count; i++) rvn_free(ed->markers[i].message);
	ed->marker_count = 0;
	if (ed->marker_tip) {
		rvn_str_set(&widget->tooltip, NULL);
		ed->marker_tip = R_FALSE;
		if (widget->window) rvn_window_rearm_tooltip(widget->window);
	}
	rvn_widget_invalidate_paint(widget);
}

int Rtext_editor_get_marker_count(Rid id) {
	RVN_EDITOR(ed, id, 0);
	return (int)ed->marker_count;
}

void Rtext_editor_set_font(Rid id, Rid font, float size) {
	RVN_EDITOR_VOID(ed, id);
	ed->font = font;
	ed->font_size = size > 0.0f ? size : 0.0f;
	rvn_ed_relayout(ed);
}

void Rtext_editor_set_zoom(Rid id, float zoom) {
	RVN_EDITOR_VOID(ed, id);
	if (zoom <= 0.0f) zoom = 1.0f;
	/* (from the program: the top of the view stays where it is, nobody is told) */
	if (widget->rect.w > 0.0f && widget->rect.h > 0.0f) {
		rvn_ed_set_zoom(ed, zoom, 0.0f, 0.0f, R_FALSE);
	} else {
		ed->zoom = rvn_clampf(zoom, RVN_ED_ZOOM_MIN, RVN_ED_ZOOM_MAX);
		rvn_ed_relayout(ed);
	}
}

float Rtext_editor_get_zoom(Rid id) {
	RVN_EDITOR(ed, id, 1.0f);
	return ed->zoom > 0.0f ? ed->zoom : 1.0f;
}

void Rtext_editor_set_zoomable(Rid id, int zoomable) {
	RVN_EDITOR_VOID(ed, id);
	ed->no_zoom = !zoomable;
}

void Rtext_editor_set_zoom_callback(Rid id, RTextEditorZoomCallback callback, void* user_data) {
	RVN_EDITOR_VOID(ed, id);
	ed->zoom_callback = callback;
	ed->zoom_user = user_data;
}

void Rtext_editor_set_placeholder(Rid id, const char* text) {
	RVN_EDITOR_VOID(ed, id);
	rvn_str_set(&ed->placeholder, text);
	rvn_widget_invalidate_paint(widget);
}

void Rtext_editor_set_changed_callback(Rid id, RTextEditorChangedCallback callback, void* user_data) {
	RVN_EDITOR_VOID(ed, id);
	ed->callback = callback;
	ed->user_data = user_data;
}

/* ---- search and replace ---- */

void Rtext_editor_set_search(Rid id, const char* query, Ruint flags) {
	RVN_EDITOR_VOID(ed, id);
	rvn_ed_search_set(ed, query, flags);
	/* the field of the find bar shows it */
	if (ed->find_ready && (!query || !memchr(query, '\n', strlen(query)))) {
		char* copy = rvn_strdup(ed->search.query ? ed->search.query : "");
		rvn_textedit_set_text(&ed->find_query, copy);
		rvn_free(copy);
	}
}

const char* Rtext_editor_get_search(Rid id) {
	RVN_EDITOR(ed, id, "");
	return ed->search.query ? ed->search.query : "";
}

Ruint Rtext_editor_get_search_flags(Rid id) {
	RVN_EDITOR(ed, id, 0);
	return ed->search.flags;
}

int Rtext_editor_find_next(Rid id) {
	RVN_EDITOR(ed, id, R_FALSE);
	return rvn_ed_find(ed, R_TRUE);
}

int Rtext_editor_find_previous(Rid id) {
	RVN_EDITOR(ed, id, R_FALSE);
	return rvn_ed_find(ed, R_FALSE);
}

int Rtext_editor_get_match_count(Rid id) {
	RVN_EDITOR(ed, id, 0);
	return rvn_ed_match_count(ed);
}

int Rtext_editor_get_match_index(Rid id) {
	RVN_EDITOR(ed, id, -1);
	return rvn_ed_match_index(ed);
}

int Rtext_editor_get_match(Rid id, int index, int* line, int* from_column, int* to_column) {
	int l = 0, start = 0, end = 0, found;
	RVN_EDITOR(ed, id, R_FALSE);
	found = rvn_ed_find_match(ed, index, &l, &start, &end);
	if (line) *line = found ? l : 0;
	if (from_column) *from_column = found ? start : 0;
	if (to_column) *to_column = found ? end : 0;
	return found;
}

int Rtext_editor_select_match(Rid id, int index) {
	int line, start, end;
	RVN_EDITOR(ed, id, R_FALSE);
	if (!rvn_ed_find_match(ed, index, &line, &start, &end)) return R_FALSE;
	rvn_ed_select_match(ed, line, start, end);
	return R_TRUE;
}

int Rtext_editor_replace(Rid id, const char* replacement) {
	int replaced;
	RVN_EDITOR(ed, id, R_FALSE);
	replaced = rvn_ed_replace_match(ed, replacement);
	if (replaced) rvn_ed_notify(widget);
	return replaced;
}

int Rtext_editor_replace_all(Rid id, const char* replacement) {
	int count;
	RVN_EDITOR(ed, id, 0);
	count = rvn_ed_replace_all(ed, replacement);
	if (count > 0) rvn_ed_notify(widget);
	return count;
}

void Rtext_editor_show_find(Rid id, int replace) {
	RVN_EDITOR_VOID(ed, id);
	if (ed->no_find_bar) return;
	if (widget->window && widget->window->focus != widget && rvn_widget_is_enabled(widget)) rvn_window_set_focus(widget->window, widget);
	rvn_ed_find_open(ed, replace);
}

void Rtext_editor_hide_find(Rid id) {
	RVN_EDITOR_VOID(ed, id);
	rvn_ed_find_close(ed);
}

int Rtext_editor_is_find_shown(Rid id) {
	RVN_EDITOR(ed, id, R_FALSE);
	return ed->find_open != 0;
}

void Rtext_editor_set_find_bar(Rid id, int enabled) {
	RVN_EDITOR_VOID(ed, id);
	if (!enabled) rvn_ed_find_close(ed);
	ed->no_find_bar = !enabled;
}
