/**
 * Raven - widgets/spelling.c
 * Spell checking in TextInput and TextEditor: the results kept per line of
 * text, the wavy underline, and the menu of corrections of a right click.
 *
 * A line is checked again only when its text changed (its hash) or when the
 * results may have (rvn_spell_generation: a word learned, the language).
 * The widgets ask only for the lines they show, a few per frame
 * (rvn_spell_line_update), so that typing in a long text stays fast.
 */

#include "common.h"
#include "../platform/spell.h"

/** Corrections shown in the menu. */
#define RVN_SPELLING_MENU_GUESSES 5

static uint32_t rvn_spell_hash(const char* text, int length) {
	uint32_t hash = 2166136261u;
	int i;
	for (i = 0; i < length; i++) {
		hash ^= (unsigned char)text[i];
		hash *= 16777619u;
	}
	return hash ^ (uint32_t)length;
}

void rvn_spell_line_free(rvn_spell_line_t* line) {
	rvn_free(line->ranges);
	memset(line, 0, sizeof(*line));
}

int rvn_spelling_active(const rvn_spelling_t* spelling) {
	return spelling && spelling->enabled && rvn_spell_available();
}

int rvn_spell_line_current(const rvn_spell_line_t* line, const char* text, int length) {
	return line->checked && line->generation == rvn_spell_generation() && line->hash == rvn_spell_hash(text, length);
}

typedef struct rvn_spell_line_job_t {
	rvn_spell_line_t* line;
	int cap;
} rvn_spell_line_job_t;

static void rvn_spell_line_add(void* user, int start, int end) {
	rvn_spell_line_job_t* job = (rvn_spell_line_job_t*)user;
	rvn_spell_line_t* line = job->line;
	if (line->count * 2 + 2 > job->cap) {
		job->cap = job->cap ? job->cap * 2 : 8;
		line->ranges = (int*)rvn_realloc(line->ranges, sizeof(int) * (size_t)job->cap);
	}
	line->ranges[line->count * 2] = start;
	line->ranges[line->count * 2 + 1] = end;
	line->count++;
}

int rvn_spell_line_update(rvn_spell_line_t* line, const rvn_spelling_t* spelling, const char* text, int length) {
	rvn_spell_line_job_t job;
	uint32_t hash = rvn_spell_hash(text, length);
	uint32_t generation = rvn_spell_generation();
	if (line->checked && line->generation == generation && line->hash == hash) return R_FALSE;
	rvn_free(line->ranges);
	line->ranges = NULL;
	line->count = 0;
	job.line = line;
	job.cap = 0;
	if (length > 0) rvn_spell_check_text(spelling->language, text, length, rvn_spell_line_add, &job);
	line->hash = hash;
	line->generation = generation;
	line->checked = R_TRUE;
	return R_TRUE;
}

int rvn_spell_line_hit(const rvn_spell_line_t* line, int offset, int* from, int* to) {
	int i;
	if (!line->checked) return R_FALSE;
	for (i = 0; i < line->count; i++) {
		if (offset >= line->ranges[i * 2] && offset <= line->ranges[i * 2 + 1]) {
			*from = line->ranges[i * 2];
			*to = line->ranges[i * 2 + 1];
			return R_TRUE;
		}
	}
	return R_FALSE;
}

void rvn_paint_spelling(RPainter* painter, float x0, float x1, float bottom, rvn_color_t color) {
	/* a zigzag of 2 units up and down every 2 units, as the systems draw it under misspelled words */
	float x;
	if (x1 - x0 < 2.0f) x1 = x0 + 2.0f;
	for (x = x0; x < x1; x += 2.0f) {
		float next = rvn_minf(x + 2.0f, x1);
		int up = ((int)((x - x0) / 2.0f + 0.5f) & 1) != 0;
		rvn_paint_line(painter, x, bottom - (up ? 2.5f : 0.5f), next, bottom - (up ? 0.5f : 2.5f), 1.0f, color);
	}
}

/* ---------------------------------------------------------------------------
 *  The menu of corrections
 * ------------------------------------------------------------------------- */

/* A text of the locale when it has the key, the English one otherwise. */
static const char* rvn_spelling_text(const char* key, const char* english) {
	return Rtr_exists(key) ? Rtr(key) : english;
}

static int rvn_spelling_owner_alive(rvn_spelling_t* spelling) {
	return spelling->owner && rvn_widget_alive(spelling->owner) && spelling->word;
}

static void rvn_spelling_replace(Rid item, void* user_data) {
	rvn_spelling_t* spelling = (rvn_spelling_t*)user_data;
	char* text;
	if (!rvn_spelling_owner_alive(spelling) || !spelling->replace) return;
	text = rvn_strdup(Rmenu_item_get_text(item));
	spelling->replace(spelling->owner, spelling->line, spelling->from, spelling->to, spelling->word, text);
	rvn_free(text);
}

static void rvn_spelling_ignore(Rid item, void* user_data) {
	rvn_spelling_t* spelling = (rvn_spelling_t*)user_data;
	RVN_UNUSED(item);
	if (!rvn_spelling_owner_alive(spelling)) return;
	rvn_spell_ignore_word(spelling->word);
	rvn_widget_invalidate_paint(spelling->owner);
}

static void rvn_spelling_learn(Rid item, void* user_data) {
	rvn_spelling_t* spelling = (rvn_spelling_t*)user_data;
	RVN_UNUSED(item);
	if (!rvn_spelling_owner_alive(spelling)) return;
	rvn_spell_learn_word(spelling->language, spelling->word);
	rvn_widget_invalidate_paint(spelling->owner);
}

typedef struct rvn_spelling_guesses_t {
	Rid menu;
	rvn_spelling_t* spelling;
	int count;
} rvn_spelling_guesses_t;

static void rvn_spelling_add_guess(void* user, const char* word) {
	rvn_spelling_guesses_t* guesses = (rvn_spelling_guesses_t*)user;
	char name[32];
	Rid item = Rmenu_add_item(guesses->menu, word);
	snprintf(name, sizeof(name), "spelling.%d", guesses->count++);
	Rwidget_set_name(item, name);
	Rmenu_item_set_clicked_callback(item, rvn_spelling_replace, guesses->spelling);
}

int rvn_spelling_open_menu(rvn_spelling_t* spelling, rvn_widget_t* owner, const char* text, int line, int from, int to) {
	rvn_spelling_guesses_t guesses;
	rvn_window_t* window = owner->window;
	Rid item;
	if (!window || to <= from) return R_FALSE;
	if (!spelling->menu) {
		spelling->menu = rvn_widget_create(R_WIDGET_MENU);
		if (!spelling->menu) return R_FALSE;
		spelling->menu->flags |= RVN_WF_INTERNAL;
		Rwidget_set_name(spelling->menu->id, "spelling");
	}
	Rwidget_clear_children(spelling->menu->id);
	spelling->owner = owner;
	rvn_free(spelling->word);
	spelling->word = rvn_strndup(text + from, (size_t)(to - from));
	spelling->line = line;
	spelling->from = from;
	spelling->to = to;

	guesses.menu = spelling->menu->id;
	guesses.spelling = spelling;
	guesses.count = 0;
	if (!spelling->read_only) {
		rvn_spell_suggest_word(spelling->language, spelling->word, RVN_SPELLING_MENU_GUESSES, rvn_spelling_add_guess, &guesses);
	}
	if (guesses.count == 0) {
		item = Rmenu_add_item(guesses.menu, rvn_spelling_text("Raven.SpellNoGuesses", "No Guesses Found"));
		Rwidget_set_enabled(item, R_FALSE);
	}
	Rmenu_add_separator(guesses.menu);
	item = Rmenu_add_item(guesses.menu, rvn_spelling_text("Raven.SpellIgnore", "Ignore Spelling"));
	Rwidget_set_name(item, "spelling.ignore");
	Rmenu_item_set_clicked_callback(item, rvn_spelling_ignore, spelling);
	item = Rmenu_add_item(guesses.menu, rvn_spelling_text("Raven.SpellLearn", "Add to Dictionary"));
	Rwidget_set_name(item, "spelling.learn");
	Rmenu_item_set_clicked_callback(item, rvn_spelling_learn, spelling);

	rvn_menu_popup(spelling->menu, window, window->mouse_x, window->mouse_y);
	return R_TRUE;
}

int rvn_spelling_set_language(rvn_spelling_t* spelling, const char* language) {
	if (!rvn_str_set(&spelling->language, language && language[0] ? language : NULL)) return R_FALSE;
	rvn_spell_touch();
	return R_TRUE;
}

void rvn_spelling_free(rvn_spelling_t* spelling) {
	if (spelling->menu) rvn_widget_destroy(spelling->menu);
	rvn_free(spelling->language);
	rvn_free(spelling->word);
	memset(spelling, 0, sizeof(*spelling));
}
