/**
 * Raven - widgets/block_editor.c
 * Block editor: programs made of blocks that snap into each other, in the
 * style of Scratch. A palette of kinds of blocks on the left, the canvas on
 * the right (it pans and zooms).
 *
 *   Statement   a step: it snaps under another one (a notch on top, a tab under)
 *   Hat         starts a script: nothing above it
 *   Cap         ends a script: nothing under it
 *   C           wraps one or more stacks in its mouths: "repeat %n{10}", "if %b then|else"
 *   Reporter    a rounded value that goes into a slot: "x position", "%n + %n"
 *   Boolean     a hexagonal condition: "touching %s ?", "%b and %b"
 *
 * Labels: words, and slots: %n number, %s text, %b boolean, %r any value,
 * %m a choice in a menu (Rblock_editor_set_slot_options); {text} right after
 * a slot is its default value; | separates the sections of a C block (each
 * section is followed by a mouth).
 *
 * Dragging a statement takes it and everything under it. While it is carried,
 * the place it would go opens (the blocks below glide away) and a shadow of
 * it waits there; a value slot that would take a reporter lights up. Dropped
 * on the palette, it is deleted. Everything moves with springs.
 *
 * Mouse: left click selects (Ctrl / Shift: adds), left drag on the canvas
 * draws a selection rectangle, dragging a block of a selection moves the
 * whole selection; right drag (or middle, or Alt + left) pans, a right click
 * opens the context menu; Ctrl + wheel zooms.
 */

#include "common.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RVN_BE_MAX_SLOTS 12
#define RVN_BE_MAX_MOUTHS 6
#define RVN_BE_PALETTE_W 210.0f
#define RVN_BE_SNAP 34.0f
#define RVN_BE_ARM 18.0f
#define RVN_BE_BOTTOM 18.0f
#define RVN_BE_MOUTH_MIN 26.0f
#define RVN_BE_NOTCH_X 14.0f
#define RVN_BE_NOTCH_W 18.0f
#define RVN_BE_NOTCH_D 5.0f
#define RVN_BE_RADIUS 5.0f
#define RVN_BE_GRID 24.0f

/* ---------------------------------------------------------------------------
 *  Data
 * ------------------------------------------------------------------------- */

typedef struct rvn_be_token_t {
	int section;
	/** Index of the slot, or -1 for a word. */
	int slot;
	char* text;
} rvn_be_token_t;

typedef struct rvn_be_kind_t {
	char* name;
	char* label;
	char* category;
	RBlockShape shape;
	rvn_be_token_t* tokens;
	uint32_t token_count;
	int sections;
	int slot_count;
	char slot_types[RVN_BE_MAX_SLOTS];
	char* defaults[RVN_BE_MAX_SLOTS];
	/** %m slots: the choices, separated by '\n'. */
	char* options[RVN_BE_MAX_SLOTS];
	/** Not in the palette (the blocks of that kind stay). */
	int hidden;
} rvn_be_kind_t;

typedef struct rvn_be_category_t {
	char* name;
	rvn_color_t color;
	int has_color;
} rvn_be_category_t;

typedef struct rvn_be_block_t {
	int id;
	int kind;
	int palette;
	/** Where it is attached: 0 = a script of its own (at x, y). */
	int parent;
	RBlockPlace place;
	int index;
	int next;
	int body[RVN_BE_MAX_MOUTHS];
	int input[RVN_BE_MAX_SLOTS];
	char* value[RVN_BE_MAX_SLOTS];
	float x, y;

	/* ---- layout (canvas units; palette blocks : palette units) ---- */
	float lx, ly, w, h;
	float row_y[RVN_BE_MAX_MOUTHS + 1], row_h[RVN_BE_MAX_MOUTHS + 1];
	float mouth_y[RVN_BE_MAX_MOUTHS], mouth_h[RVN_BE_MAX_MOUTHS];
	float slot_x[RVN_BE_MAX_SLOTS], slot_y[RVN_BE_MAX_SLOTS], slot_w[RVN_BE_MAX_SLOTS], slot_h[RVN_BE_MAX_SLOTS];

	/* ---- what is shown ---- */
	RSpring sx, sy;
	RSpring appear, hover, glow_shown, fade;
	int glow;
	int dying;
	int placed;
	/** Selected; sel_base : selected before the rectangle started. */
	int sel, sel_base;
	/** Cannot be deleted by the user (the entry of a function...). */
	int locked;
} rvn_be_block_t;

typedef enum rvn_be_drag_t {
	RVN_BE_NONE = 0,
	RVN_BE_PRESS,       /* button down on a block, not moved yet */
	RVN_BE_PRESS_SLOT,  /* on a value slot: opens its editor if not moved */
	RVN_BE_PRESS_PALETTE,
	RVN_BE_BLOCKS,      /* carrying a stack */
	RVN_BE_PAN,
	RVN_BE_PRESS_RIGHT, /* right button down : a pan if it moves, else the context menu */
	RVN_BE_MARQUEE,     /* a selection rectangle */
	RVN_BE_GROUP        /* moving the selection */
} rvn_be_drag_t;

typedef struct rvn_block_editor_t {
	rvn_widget_t base;

	rvn_be_kind_t* kinds;
	uint32_t kind_count, kind_cap;
	rvn_be_category_t* categories;
	uint32_t category_count, category_cap;
	rvn_be_block_t* blocks;
	uint32_t block_count, block_cap;
	int next_id;
	/** Scripts in drawing order (top blocks). */
	int* order;
	uint32_t order_count, order_cap;
	/** Blocks in drawing order, filled by the layout (hit tests go the other way). */
	int* drawn;
	uint32_t drawn_count, drawn_cap;

	float pan_x, pan_y, zoom;
	RSpring shown_zoom;
	float palette_scroll;
	float palette_height;
	/** The finger that pans came down on the palette (it scrolls the palette, not the canvas). */
	int touch_palette;
	int palette_hidden;
	int layout_dirty;
	int painted;

	/* ---- gesture ---- */
	rvn_be_drag_t drag;
	int press_block;
	int press_slot;
	float press_x, press_y;
	float grab_x, grab_y;       /* pointer - top left of the carried block (canvas units) */
	float pointer_x, pointer_y; /* in the widget */
	float pan_from_x, pan_from_y;
	int carried;                /* top block of the carried stack */
	float carried_h;
	/* where it would go */
	int target;
	RBlockPlace target_place;
	int target_index;
	int target_top;             /* above a script: that script */
	float ghost_x, ghost_y;
	RSpring ghost_shown;
	RSpring trash;
	int hot_block;
	int selected;

	/* ---- the editor of a value slot ---- */
	rvn_widget_t* field;
	int edit_block, edit_slot;
	/* ---- the menu of a %m slot ---- */
	Rid menu;
	int menu_block, menu_slot;
	/* ---- selection rectangle / moving the selection (canvas units) ---- */
	float marquee_x0, marquee_y0, marquee_x1, marquee_y1;
	float group_x, group_y;
	int* group;
	uint32_t group_count, group_cap;

	RBlockEditorCallback changed_callback;
	void* changed_user;
	RBlockEditorCallback clicked_callback;
	void* clicked_user;
	RBlockEditorContextCallback context_callback;
	void* context_user;

	rvn_text_t saved;
} rvn_block_editor_t;

static const uint32_t g_be_palette[] = {
	0x4C97FF, 0x9966FF, 0xCF63CF, 0xFFBF00, 0xFFAB19, 0x5CB1D6, 0x59C059, 0xFF8C1A, 0xFF6680, 0x0FBD8C
};

/* ---------------------------------------------------------------------------
 *  Helpers
 * ------------------------------------------------------------------------- */

static rvn_be_block_t* rvn_be_block(rvn_block_editor_t* e, int id) {
	uint32_t i;
	if (id <= 0) return NULL;
	for (i = 0; i < e->block_count; i++) if (e->blocks[i].id == id) return &e->blocks[i];
	return NULL;
}

static rvn_be_kind_t* rvn_be_kind_of(rvn_block_editor_t* e, const rvn_be_block_t* b) {
	return b && b->kind >= 0 && (uint32_t)b->kind < e->kind_count ? &e->kinds[b->kind] : NULL;
}

static int rvn_be_find_kind(rvn_block_editor_t* e, const char* name) {
	uint32_t i;
	if (!name) return -1;
	for (i = 0; i < e->kind_count; i++) if (strcmp(e->kinds[i].name, name) == 0) return (int)i;
	return -1;
}

static int rvn_be_is_value(RBlockShape shape) { return shape == R_BLOCK_REPORTER || shape == R_BLOCK_BOOLEAN; }
static int rvn_be_has_next(RBlockShape shape) { return shape == R_BLOCK_STATEMENT || shape == R_BLOCK_HAT || shape == R_BLOCK_C; }
static int rvn_be_has_top(RBlockShape shape) { return shape == R_BLOCK_STATEMENT || shape == R_BLOCK_CAP || shape == R_BLOCK_C; }

static void rvn_be_changed(rvn_block_editor_t* e) {
	e->layout_dirty = R_TRUE;
	rvn_widget_invalidate_paint(&e->base);
}

static void rvn_be_notify(rvn_block_editor_t* e, int block) {
	if (e->changed_callback && rvn_widget_alive(&e->base)) e->changed_callback(e->base.id, block, e->changed_user);
	if (rvn_widget_alive(&e->base)) rvn_widget_fire_action(&e->base);
}

static rvn_color_t rvn_be_category_color(rvn_block_editor_t* e, const char* category) {
	uint32_t i;
	for (i = 0; i < e->category_count; i++)
		if (strcmp(e->categories[i].name, category ? category : "") == 0 && e->categories[i].has_color) return e->categories[i].color;
	return rvn_hex(g_be_palette[rvn_hash_str(category ? category : "") % RVN_COUNT_OF(g_be_palette)], 255);
}

static int rvn_be_light(rvn_color_t color) {
	float r, g, b, a;
	rvn_color_to_floats(color, &r, &g, &b, &a);
	return 0.2126f * r + 0.7152f * g + 0.0722f * b > 0.66f;
}

static int rvn_be_dark_theme(const rvn_widget_t* widget) {
	return rvn_be_light(rvn_widget_color(widget, R_COLOR_TEXT));
}

/* The top block of the script a block is in. */
static rvn_be_block_t* rvn_be_top_of(rvn_block_editor_t* e, rvn_be_block_t* b) {
	int guard = 0;
	while (b && b->parent && guard++ < 100000) b = rvn_be_block(e, b->parent);
	return b;
}

/* R_TRUE when `inner` is `outer` or hangs under it (next, mouths, inputs). */
static int rvn_be_inside(rvn_block_editor_t* e, int inner, int outer) {
	rvn_be_block_t* b = rvn_be_block(e, inner);
	int guard = 0;
	while (b && guard++ < 100000) {
		if (b->id == outer) return R_TRUE;
		b = rvn_be_block(e, b->parent);
	}
	return R_FALSE;
}

static rvn_be_block_t* rvn_be_last_of_chain(rvn_block_editor_t* e, rvn_be_block_t* b) {
	int guard = 0;
	while (b && b->next && guard++ < 100000) b = rvn_be_block(e, b->next);
	return b;
}

static void rvn_be_order_add(rvn_block_editor_t* e, int id) {
	RVN_ARRAY_RESERVE(e->order, e->order_cap, e->order_count + 1);
	e->order[e->order_count++] = id;
}

static void rvn_be_order_remove(rvn_block_editor_t* e, int id) {
	uint32_t i;
	for (i = 0; i < e->order_count; i++) {
		if (e->order[i] != id) continue;
		memmove(&e->order[i], &e->order[i + 1], (e->order_count - i - 1) * sizeof(int));
		e->order_count--;
		return;
	}
}

static void rvn_be_order_raise(rvn_block_editor_t* e, int id) {
	rvn_be_order_remove(e, id);
	rvn_be_order_add(e, id);
}

/* ---------------------------------------------------------------------------
 *  Kinds : the label is cut into words and slots
 * ------------------------------------------------------------------------- */

static void rvn_be_add_token(rvn_be_kind_t* k, int section, int slot, const char* text, size_t length, uint32_t* cap) {
	rvn_be_token_t* t;
	if (slot < 0 && length == 0) return;
	RVN_ARRAY_RESERVE(k->tokens, *cap, k->token_count + 1);
	t = &k->tokens[k->token_count++];
	t->section = section;
	t->slot = slot;
	t->text = slot < 0 ? rvn_strndup(text, length) : NULL;
}

static void rvn_be_parse_label(rvn_be_kind_t* k) {
	const char* c = k->label ? k->label : "";
	const char* word = c;
	uint32_t cap = 0;
	int section = 0;
	k->sections = 1;
	while (*c) {
		if (*c == '|') {
			rvn_be_add_token(k, section, -1, word, (size_t)(c - word), &cap);
			section++;
			k->sections = section + 1;
			c++;
			word = c;
			continue;
		}
		if (*c == '%' && (c[1] == 'n' || c[1] == 's' || c[1] == 'b' || c[1] == 'r' || c[1] == 'm') && k->slot_count < RVN_BE_MAX_SLOTS) {
			int slot = k->slot_count++;
			rvn_be_add_token(k, section, -1, word, (size_t)(c - word), &cap);
			k->slot_types[slot] = c[1];
			c += 2;
			if (*c == '{') {
				const char* end = strchr(c, '}');
				if (end) {
					k->defaults[slot] = rvn_strndup(c + 1, (size_t)(end - c - 1));
					c = end + 1;
				}
			}
			if (!k->defaults[slot]) k->defaults[slot] = rvn_strdup(k->slot_types[slot] == 'n' ? "0" : "");
			rvn_be_add_token(k, section, slot, NULL, 0, &cap);
			word = c;
			continue;
		}
		c++;
	}
	rvn_be_add_token(k, section, -1, word, (size_t)(c - word), &cap);
	if (k->shape != R_BLOCK_C) k->sections = 1;
}

static void rvn_be_free_kind(rvn_be_kind_t* k) {
	uint32_t i;
	int s;
	for (i = 0; i < k->token_count; i++) rvn_free(k->tokens[i].text);
	rvn_free(k->tokens);
	for (s = 0; s < RVN_BE_MAX_SLOTS; s++) rvn_free(k->defaults[s]);
	rvn_free(k->name);
	rvn_free(k->label);
	rvn_free(k->category);
	for (s = 0; s < RVN_BE_MAX_SLOTS; s++) rvn_free(k->options[s]);
}

/* ---------------------------------------------------------------------------
 *  Blocks
 * ------------------------------------------------------------------------- */

static rvn_be_block_t* rvn_be_new_block(rvn_block_editor_t* e, int kind, int palette) {
	rvn_be_block_t* b;
	const rvn_be_kind_t* k = &e->kinds[kind];
	int s;
	RVN_ARRAY_RESERVE(e->blocks, e->block_cap, e->block_count + 1);
	b = &e->blocks[e->block_count++];
	memset(b, 0, sizeof(*b));
	b->id = e->next_id++;
	b->kind = kind;
	b->palette = palette;
	b->place = R_BLOCK_NEXT;
	for (s = 0; s < k->slot_count; s++) b->value[s] = rvn_strdup(k->defaults[s] ? k->defaults[s] : "");
	rvn_spring_set(&b->fade, 1.0f);
	if (!e->painted || palette) rvn_spring_set(&b->appear, 1.0f);
	return b;
}

/* Takes a block out of where it is attached (with what hangs under it). */
static void rvn_be_detach(rvn_block_editor_t* e, rvn_be_block_t* b) {
	rvn_be_block_t* p;
	if (!b) return;
	if (!b->parent) {
		rvn_be_order_remove(e, b->id);
		return;
	}
	p = rvn_be_block(e, b->parent);
	if (p) {
		if (b->place == R_BLOCK_NEXT && p->next == b->id) p->next = 0;
		else if (b->place == R_BLOCK_BODY && b->index < RVN_BE_MAX_MOUTHS && p->body[b->index] == b->id) p->body[b->index] = 0;
		else if (b->place == R_BLOCK_INPUT && b->index < RVN_BE_MAX_SLOTS && p->input[b->index] == b->id) p->input[b->index] = 0;
	}
	b->parent = 0;
	b->place = R_BLOCK_NEXT;
	b->index = 0;
}

static void rvn_be_make_top(rvn_block_editor_t* e, rvn_be_block_t* b, float x, float y) {
	rvn_be_detach(e, b);
	b->x = x;
	b->y = y;
	rvn_be_order_add(e, b->id);
}

/* Puts a block (a script of its own) at a place. Returns R_FALSE when it cannot go there. */
static int rvn_be_attach(rvn_block_editor_t* e, rvn_be_block_t* b, rvn_be_block_t* target, RBlockPlace place, int index) {
	const rvn_be_kind_t* kb = rvn_be_kind_of(e, b);
	const rvn_be_kind_t* kt = rvn_be_kind_of(e, target);
	int id = b ? b->id : 0;
	if (!b || !target || !kb || !kt || target->palette || b->palette || rvn_be_inside(e, target->id, b->id)) return R_FALSE;
	if (place == R_BLOCK_INPUT) {
		char type;
		if (!rvn_be_is_value(kb->shape) || index < 0 || index >= kt->slot_count) return R_FALSE;
		type = kt->slot_types[index];
		if (type == 'm') return R_FALSE;
		if (type == 'b' && kb->shape != R_BLOCK_BOOLEAN) return R_FALSE;
		if ((type == 'n' || type == 's') && kb->shape != R_BLOCK_REPORTER) return R_FALSE;
		rvn_be_detach(e, b);
		b = rvn_be_block(e, id);
		target = rvn_be_block(e, target->id);
		if (target->input[index]) {
			/* the value that was there jumps out, beside */
			rvn_be_block_t* old = rvn_be_block(e, target->input[index]);
			if (old) rvn_be_make_top(e, old, target->lx + target->w + 20.0f, target->ly);
		}
		target->input[index] = id;
	} else {
		rvn_be_block_t* last;
		int after;
		if (rvn_be_is_value(kb->shape) || kb->shape == R_BLOCK_HAT || rvn_be_is_value(kt->shape)) return R_FALSE;
		if (place == R_BLOCK_NEXT && !rvn_be_has_next(kt->shape)) return R_FALSE;
		if (place == R_BLOCK_BODY && (kt->shape != R_BLOCK_C || index < 0 || index >= kt->sections)) return R_FALSE;
		rvn_be_detach(e, b);
		b = rvn_be_block(e, id);
		target = rvn_be_block(e, target->id);
		after = place == R_BLOCK_NEXT ? target->next : target->body[index];
		last = rvn_be_last_of_chain(e, b);
		if (after) {
			rvn_be_block_t* rest = rvn_be_block(e, after);
			const rvn_be_kind_t* kl = rvn_be_kind_of(e, last);
			if (rest && kl && kl->shape == R_BLOCK_C && last == b && !b->body[0]) {
				/* a lone C block wraps what was under it (Scratch) */
				b->body[0] = after;
				rest->parent = b->id;
				rest->place = R_BLOCK_BODY;
				rest->index = 0;
			} else if (rest && kl && rvn_be_has_next(kl->shape)) {
				last->next = after;
				rest->parent = last->id;
				rest->place = R_BLOCK_NEXT;
				rest->index = 0;
			} else if (rest) {
				/* (a cap block ends the stack : what was under goes beside) */
				rest->parent = 0;
				rest->x = target->lx + target->w + 30.0f;
				rest->y = target->ly + target->h;
				rvn_be_order_add(e, rest->id);
			}
		}
		if (place == R_BLOCK_NEXT) target->next = id;
		else target->body[index] = id;
	}
	b->parent = target->id;
	b->place = place;
	b->index = index;
	rvn_be_changed(e);
	return R_TRUE;
}

static void rvn_be_free_block_data(rvn_be_block_t* b) {
	int s;
	for (s = 0; s < RVN_BE_MAX_SLOTS; s++) rvn_free(b->value[s]);
}

static void rvn_be_remove_index(rvn_block_editor_t* e, uint32_t index) {
	rvn_be_free_block_data(&e->blocks[index]);
	memmove(&e->blocks[index], &e->blocks[index + 1], (e->block_count - index - 1) * sizeof(rvn_be_block_t));
	e->block_count--;
}

/* Marks a block and what hangs under it as going away (they fade out). */
static void rvn_be_kill(rvn_block_editor_t* e, int id, int animate) {
	rvn_be_block_t* b = rvn_be_block(e, id);
	int s, next, body[RVN_BE_MAX_MOUTHS], input[RVN_BE_MAX_SLOTS];
	if (!b) return;
	next = b->next;
	memcpy(body, b->body, sizeof(body));
	memcpy(input, b->input, sizeof(input));
	b->dying = R_TRUE;
	b->sel = b->sel_base = R_FALSE;
	if (!animate) rvn_spring_set(&b->fade, 0.0f);
	if (e->selected == id) e->selected = 0;
	if (e->edit_block == id) e->edit_block = 0;
	if (next) rvn_be_kill(e, next, animate);
	for (s = 0; s < RVN_BE_MAX_MOUTHS; s++) if (body[s]) rvn_be_kill(e, body[s], animate);
	for (s = 0; s < RVN_BE_MAX_SLOTS; s++) if (input[s]) rvn_be_kill(e, input[s], animate);
}

static void rvn_be_delete_script(rvn_block_editor_t* e, int id, int animate) {
	rvn_be_block_t* b = rvn_be_block(e, id);
	if (!b) return;
	rvn_be_detach(e, b);
	rvn_be_kill(e, id, animate);
	rvn_be_changed(e);
}

static void rvn_be_purge(rvn_block_editor_t* e, int all) {
	uint32_t i;
	for (i = e->block_count; i > 0; i--) {
		rvn_be_block_t* b = &e->blocks[i - 1];
		if ((b->dying && (all || b->fade.value < 0.01f)) || (all && !b->palette)) rvn_be_remove_index(e, i - 1);
	}
}

/* ---------------------------------------------------------------------------
 *  Layout
 * ------------------------------------------------------------------------- */

typedef struct rvn_be_metrics_t {
	float font;
	float row;      /* height of a row of a statement */
	float slot;     /* height of a value slot */
	float pad;
} rvn_be_metrics_t;

static rvn_be_metrics_t rvn_be_metrics(const rvn_widget_t* widget) {
	rvn_be_metrics_t m;
	m.font = rvn_widget_font_size(widget, 0.0f);
	m.slot = floorf(m.font + 10.0f);
	m.row = rvn_maxf(36.0f, m.slot + 12.0f);
	m.pad = 10.0f;
	return m;
}

static void rvn_be_measure_block(rvn_block_editor_t* e, rvn_be_block_t* b, const rvn_be_metrics_t* m);

/* Width and height of a chain of statements (a mouth, a script). */
static void rvn_be_measure_chain(rvn_block_editor_t* e, int first, const rvn_be_metrics_t* m, float* w, float* h) {
	rvn_be_block_t* b = rvn_be_block(e, first);
	int guard = 0;
	*w = 0.0f;
	*h = 0.0f;
	while (b && guard++ < 100000) {
		rvn_be_measure_block(e, b, m);
		*w = rvn_maxf(*w, b->w);
		*h += b->h;
		if (e->drag == RVN_BE_BLOCKS && e->target == b->id && e->target_place == R_BLOCK_NEXT) *h += e->carried_h;
		b = rvn_be_block(e, b->next);
	}
}

static float rvn_be_slot_extent(rvn_block_editor_t* e, rvn_be_block_t* b, const rvn_be_kind_t* k, int slot, const rvn_be_metrics_t* m, float* height) {
	rvn_be_block_t* input = rvn_be_block(e, b->input[slot]);
	if (input && !input->dying) {
		rvn_be_measure_block(e, input, m);
		*height = input->h;
		return input->w;
	}
	*height = m->slot;
	if (k->slot_types[slot] == 'b') return 40.0f;
	return rvn_maxf(m->slot + 4.0f, rvn_widget_text_width(&e->base, R_INVALID_ID, m->font, b->value[slot] ? b->value[slot] : "") +
		(k->slot_types[slot] == 'm' ? 30.0f : 16.0f));
}

static void rvn_be_measure_block(rvn_block_editor_t* e, rvn_be_block_t* b, const rvn_be_metrics_t* m) {
	const rvn_be_kind_t* k = rvn_be_kind_of(e, b);
	int value = k && rvn_be_is_value(k->shape);
	int s;
	float width = 0.0f, y = 0.0f;
	if (!k) return;
	for (s = 0; s < k->sections; s++) {
		float x = value ? (k->shape == R_BLOCK_BOOLEAN ? m->slot * 0.55f + 6.0f : m->pad + 2.0f) : m->pad;
		float row = value ? m->slot : m->row;
		uint32_t t;
		for (t = 0; t < k->token_count; t++) {
			const rvn_be_token_t* tok = &k->tokens[t];
			if (tok->section != s) continue;
			if (tok->slot < 0) {
				x += rvn_widget_text_width(&e->base, R_INVALID_ID, m->font, tok->text) + 4.0f;
			} else {
				float sh = 0.0f, sw = rvn_be_slot_extent(e, b, k, tok->slot, m, &sh);
				b->slot_x[tok->slot] = x;
				b->slot_w[tok->slot] = sw;
				b->slot_h[tok->slot] = sh;
				row = rvn_maxf(row, sh + (value ? 6.0f : 10.0f));
				x += sw + 5.0f;
			}
		}
		if (k->shape == R_BLOCK_HAT && s == 0) row += 14.0f;
		x += value ? (k->shape == R_BLOCK_BOOLEAN ? m->slot * 0.55f + 2.0f : m->pad - 2.0f) : m->pad;
		width = rvn_maxf(width, x);
		b->row_y[s] = y;
		b->row_h[s] = row;
		/* the slots of the row are centered on it */
		for (t = 0; t < k->token_count; t++) {
			const rvn_be_token_t* tok = &k->tokens[t];
			if (tok->section == s && tok->slot >= 0) b->slot_y[tok->slot] = y + (k->shape == R_BLOCK_HAT && s == 0 ? 14.0f : 0.0f) +
				((row - (k->shape == R_BLOCK_HAT && s == 0 ? 14.0f : 0.0f)) - b->slot_h[tok->slot]) * 0.5f;
		}
		y += row;
		if (k->shape == R_BLOCK_C) {
			float mw = 0.0f, mh = 0.0f;
			rvn_be_measure_chain(e, b->body[s], m, &mw, &mh);
			if (e->drag == RVN_BE_BLOCKS && e->target == b->id && e->target_place == R_BLOCK_BODY && e->target_index == s) mh += e->carried_h;
			b->mouth_y[s] = y;
			b->mouth_h[s] = rvn_maxf(RVN_BE_MOUTH_MIN, mh);
			y += b->mouth_h[s];
			width = rvn_maxf(width, RVN_BE_ARM + 60.0f);
			(void)mw;
		}
	}
	if (k->shape == R_BLOCK_C) y += RVN_BE_BOTTOM;
	b->w = rvn_maxf(width, value ? m->slot : 64.0f);
	b->h = y;
}

static void rvn_be_place_block(rvn_block_editor_t* e, rvn_be_block_t* b, float x, float y, const rvn_be_metrics_t* m);

/* The chain under a place; `gap_at` : a block before which the carried stack would open a gap. */
static void rvn_be_place_chain(rvn_block_editor_t* e, int first, float x, float y, const rvn_be_metrics_t* m) {
	rvn_be_block_t* b = rvn_be_block(e, first);
	int guard = 0;
	while (b && guard++ < 100000) {
		rvn_be_place_block(e, b, x, y, m);
		y += b->h;
		if (e->drag == RVN_BE_BLOCKS && e->target == b->id && e->target_place == R_BLOCK_NEXT) {
			e->ghost_x = x;
			e->ghost_y = y;
			y += e->carried_h;
		}
		b = rvn_be_block(e, b->next);
	}
}

static void rvn_be_place_block(rvn_block_editor_t* e, rvn_be_block_t* b, float x, float y, const rvn_be_metrics_t* m) {
	const rvn_be_kind_t* k = rvn_be_kind_of(e, b);
	int s;
	if (!k) return;
	b->lx = x;
	b->ly = y;
	RVN_ARRAY_RESERVE(e->drawn, e->drawn_cap, e->drawn_count + 1);
	e->drawn[e->drawn_count++] = b->id;
	for (s = 0; s < k->slot_count; s++) {
		rvn_be_block_t* input = rvn_be_block(e, b->input[s]);
		if (input && !input->dying) rvn_be_place_block(e, input, x + b->slot_x[s], y + b->slot_y[s], m);
	}
	if (k->shape == R_BLOCK_C) {
		for (s = 0; s < k->sections; s++) {
			float my = y + b->mouth_y[s];
			if (e->drag == RVN_BE_BLOCKS && e->target == b->id && e->target_place == R_BLOCK_BODY && e->target_index == s) {
				e->ghost_x = x + RVN_BE_ARM;
				e->ghost_y = my;
				my += e->carried_h;
			}
			rvn_be_place_chain(e, b->body[s], x + RVN_BE_ARM, my, m);
		}
	}
}

/* The carried stack opens a gap : the blocks of a place are measured with it. */
static void rvn_be_layout(rvn_block_editor_t* e) {
	rvn_be_metrics_t m = rvn_be_metrics(&e->base);
	uint32_t i;
	float py = 10.0f;
	const char* category = NULL;

	e->drawn_count = 0;
	/* sizes first (they include the gap the carried stack opens) */
	for (i = 0; i < e->order_count; i++) {
		rvn_be_block_t* top = rvn_be_block(e, e->order[i]);
		float w, h;
		if (top) rvn_be_measure_chain(e, top->id, &m, &w, &h);
	}
	/* then places */
	for (i = 0; i < e->order_count; i++) {
		rvn_be_block_t* top = rvn_be_block(e, e->order[i]);
		float x, y;
		if (!top || top->dying) continue;
		x = top->x;
		y = top->y;
		if (e->drag == RVN_BE_BLOCKS && e->target_top == top->id) {
			e->ghost_x = x;
			e->ghost_y = y;
			y += e->carried_h;
		}
		rvn_be_place_chain(e, top->id, x, y, &m);
	}
	/* the palette : one of each kind, by category */
	for (i = 0; i < e->block_count; i++) {
		rvn_be_block_t* b = &e->blocks[i];
		const rvn_be_kind_t* k;
		if (!b->palette) continue;
		k = rvn_be_kind_of(e, b);
		if (!k || k->hidden) {
			b->lx = b->ly = -100000.0f;
			continue;
		}
		if (!category || strcmp(category, k->category ? k->category : "") != 0) {
			category = k->category ? k->category : "";
			py += category[0] ? 30.0f : 6.0f;
		}
		rvn_be_measure_block(e, b, &m);
		b->lx = 14.0f;
		b->ly = py;
		py += b->h + 10.0f;
	}
	e->palette_height = py + 20.0f;
	e->layout_dirty = R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  View
 * ------------------------------------------------------------------------- */

static float rvn_be_canvas_left(const rvn_block_editor_t* e) { return e->palette_hidden ? 0.0f : RVN_BE_PALETTE_W; }

static void rvn_be_to_canvas(const rvn_block_editor_t* e, float x, float y, float* cx, float* cy) {
	float z = e->zoom > 0.0f ? e->zoom : 1.0f;
	*cx = (x - rvn_be_canvas_left(e)) / z + e->pan_x;
	*cy = y / z + e->pan_y;
}

static int rvn_be_in_palette(const rvn_block_editor_t* e, float x) { return !e->palette_hidden && x < RVN_BE_PALETTE_W; }

/* ---------------------------------------------------------------------------
 *  Hit tests
 * ------------------------------------------------------------------------- */

/* Is (cx, cy) on the block itself (not in one of its mouths) ? Positions as shown. */
static int rvn_be_on_block(rvn_block_editor_t* e, const rvn_be_block_t* b, float bx, float by, float cx, float cy) {
	const rvn_be_kind_t* k = rvn_be_kind_of(e, b);
	int s;
	if (!k || cx < bx || cy < by || cx > bx + b->w || cy > by + b->h) return R_FALSE;
	if (k->shape == R_BLOCK_C)
		for (s = 0; s < k->sections; s++)
			if (cx > bx + RVN_BE_ARM && cy > by + b->mouth_y[s] && cy < by + b->mouth_y[s] + b->mouth_h[s]) return R_FALSE;
	return R_TRUE;
}

/* The block under the pointer, the deepest first; *slot : a value slot under it, or -1. */
static int rvn_be_block_at(rvn_block_editor_t* e, float x, float y, int* slot) {
	float cx, cy;
	uint32_t i;
	*slot = -1;
	if (rvn_be_in_palette(e, x)) {
		float py = y + e->palette_scroll;
		for (i = 0; i < e->block_count; i++) {
			const rvn_be_block_t* b = &e->blocks[i];
			const rvn_be_kind_t* k = rvn_be_kind_of(e, b);
			if (b->palette && k && !k->hidden && rvn_be_on_block(e, b, b->lx, b->ly, x, py)) return b->id;
		}
		return 0;
	}
	rvn_be_to_canvas(e, x, y, &cx, &cy);
	for (i = e->drawn_count; i > 0; i--) {
		rvn_be_block_t* b = rvn_be_block(e, e->drawn[i - 1]);
		const rvn_be_kind_t* k = rvn_be_kind_of(e, b);
		int s;
		if (!b || b->dying || b->id == e->carried || rvn_be_inside(e, b->id, e->carried)) continue;
		if (!rvn_be_on_block(e, b, b->sx.value, b->sy.value, cx, cy)) continue;
		for (s = 0; k && s < k->slot_count; s++) {
			float sx = b->sx.value + b->slot_x[s], sy = b->sy.value + b->slot_y[s];
			if (!b->input[s] && cx >= sx && cx <= sx + b->slot_w[s] && cy >= sy && cy <= sy + b->slot_h[s]) *slot = s;
		}
		return b->id;
	}
	return 0;
}

/* Where the carried stack would go if released now. */
static void rvn_be_find_target(rvn_block_editor_t* e) {
	rvn_be_block_t* d = rvn_be_block(e, e->carried);
	const rvn_be_kind_t* kd = rvn_be_kind_of(e, d);
	float best = RVN_BE_SNAP * RVN_BE_SNAP;
	int target = 0, index = 0, top = 0;
	RBlockPlace place = R_BLOCK_NEXT;
	rvn_be_block_t* last;
	const rvn_be_kind_t* kl;
	uint32_t i;
	float dx, dy;
	e->target = 0;
	e->target_top = 0;
	if (!d || !kd || rvn_be_in_palette(e, e->pointer_x)) return;
	rvn_be_to_canvas(e, e->pointer_x, e->pointer_y, &dx, &dy);
	dx -= e->grab_x;
	dy -= e->grab_y;
	last = rvn_be_last_of_chain(e, d);
	kl = rvn_be_kind_of(e, last);

	for (i = 0; i < e->drawn_count; i++) {
		rvn_be_block_t* t = rvn_be_block(e, e->drawn[i]);
		const rvn_be_kind_t* kt = rvn_be_kind_of(e, t);
		int s;
		if (!t || !kt || t->dying || rvn_be_inside(e, t->id, d->id)) continue;
		if (rvn_be_is_value(kd->shape)) {
			/* a value : an empty slot that takes it, the left of the value on the left of the slot */
			for (s = 0; s < kt->slot_count; s++) {
				char type = kt->slot_types[s];
				float sx = t->lx + t->slot_x[s], sy = t->ly + t->slot_y[s] + t->slot_h[s] * 0.5f;
				float ex = dx - sx, ey = dy + d->h * 0.5f - sy, dist = ex * ex + ey * ey;
				if (t->input[s] || type == 'm') continue;
				if (type == 'b' && kd->shape != R_BLOCK_BOOLEAN) continue;
				if ((type == 'n' || type == 's') && kd->shape != R_BLOCK_REPORTER) continue;
				if (dist < best) { best = dist; target = t->id; place = R_BLOCK_INPUT; index = s; }
			}
			continue;
		}
		if (kd->shape == R_BLOCK_HAT || rvn_be_is_value(kt->shape)) continue;
		/* under a block */
		if (rvn_be_has_next(kt->shape) && !(t->next && (!kl || !rvn_be_has_next(kl->shape)))) {
			float ex = dx - t->lx, ey = dy - (t->ly + t->h), dist = ex * ex + ey * ey;
			if (dist < best) { best = dist; target = t->id; place = R_BLOCK_NEXT; index = 0; }
		}
		/* in a mouth */
		if (kt->shape == R_BLOCK_C)
			for (s = 0; s < kt->sections; s++) {
				float ex = dx - (t->lx + RVN_BE_ARM), ey = dy - (t->ly + t->mouth_y[s]), dist = ex * ex + ey * ey;
				if (t->body[s] && (!kl || !rvn_be_has_next(kl->shape))) continue;
				if (dist < best) { best = dist; target = t->id; place = R_BLOCK_BODY; index = s; }
			}
	}
	/* above a script (its top takes a block over it) */
	if (!rvn_be_is_value(kd->shape) && kl && rvn_be_has_next(kl->shape))
		for (i = 0; i < e->order_count; i++) {
			rvn_be_block_t* t = rvn_be_block(e, e->order[i]);
			const rvn_be_kind_t* kt = rvn_be_kind_of(e, t);
			float h, ex, ey, dist;
			if (!t || !kt || t->dying || t->id == d->id || !rvn_be_has_top(kt->shape)) continue;
			h = 0.0f;
			{
				rvn_be_block_t* c = d;
				int guard = 0;
				while (c && guard++ < 100000) { h += c->h; c = rvn_be_block(e, c->next); }
			}
			ex = dx - t->x;
			ey = dy + h - t->y;
			dist = ex * ex + ey * ey;
			if (dist < best) { best = dist; top = t->id; target = 0; }
		}
	e->target = top ? 0 : target;
	e->target_place = place;
	e->target_index = index;
	e->target_top = top;
}

/* ---------------------------------------------------------------------------
 *  Painting
 * ------------------------------------------------------------------------- */

typedef struct rvn_be_look_t {
	rvn_be_metrics_t m;
	float ox, oy, scale;
	rvn_color_t text_dark, text_light;
	rvn_color_t slot_fill, slot_text, slot_line;
	rvn_color_t glow;
	int dark;
	float opacity;
} rvn_be_look_t;

#define BX(v) (look->ox + (v) * look->scale)
#define BY(v) (look->oy + (v) * look->scale)
#define BS(v) ((v) * look->scale)

static void rvn_be_trapezoid(RPainter* p, float x0, float x1, float y0, float x2, float x3, float y1, rvn_color_t color) {
	/* top edge x0..x1 at y0, bottom edge x2..x3 at y1 */
	rvn_paint_triangle(p, x0, y0, x1, y0, x3, y1, color);
	rvn_paint_triangle(p, x0, y0, x3, y1, x2, y1, color);
}

/* The silhouette of a statement / hat / cap / C block, filled with `fill`. */
static void rvn_be_paint_shape(RPainter* p, rvn_block_editor_t* e, const rvn_be_block_t* b, const rvn_be_kind_t* k,
	float x, float y, const rvn_be_look_t* look, rvn_color_t fill, rvn_color_t edge, int notch_top, rvn_color_t behind) {
	float r = BS(RVN_BE_RADIUS), w = BS(b->w), h = BS(b->h);
	float nx = BS(RVN_BE_NOTCH_X), nw = BS(RVN_BE_NOTCH_W), nd = BS(RVN_BE_NOTCH_D);
	float X = BX(x), Y = BY(y);
	int s;
	(void)e;
	if (k->shape == R_BLOCK_REPORTER) {
		rvn_paint_box(p, rvn_rect(X, Y, w, h), h * 0.5f, fill, BS(1.0f), edge);
		return;
	}
	if (k->shape == R_BLOCK_BOOLEAN) {
		float tip = rvn_minf(h * 0.5f, w * 0.3f);
		rvn_paint_rect(p, rvn_rect(X + tip, Y, w - tip * 2.0f, h), fill);
		rvn_paint_triangle(p, X, Y + h * 0.5f, X + tip, Y, X + tip, Y + h, fill);
		rvn_paint_triangle(p, X + w, Y + h * 0.5f, X + w - tip, Y + h, X + w - tip, Y, fill);
		rvn_paint_line(p, X, Y + h * 0.5f, X + tip, Y, BS(1.0f), edge);
		rvn_paint_line(p, X, Y + h * 0.5f, X + tip, Y + h, BS(1.0f), edge);
		rvn_paint_line(p, X + w, Y + h * 0.5f, X + w - tip, Y, BS(1.0f), edge);
		rvn_paint_line(p, X + w, Y + h * 0.5f, X + w - tip, Y + h, BS(1.0f), edge);
		rvn_paint_line(p, X + tip, Y, X + w - tip, Y, BS(1.0f), edge);
		rvn_paint_line(p, X + tip, Y + h, X + w - tip, Y + h, BS(1.0f), edge);
		return;
	}
	if (k->shape == R_BLOCK_C) {
		/* the rows, the arm on the left, the bottom */
		for (s = 0; s < k->sections; s++) {
			float ry = BY(y + b->row_y[s]), rh = BS(b->row_h[s]);
			float radii[4] = { s == 0 ? r : 0.0f, s == 0 ? r : 0.0f, r, 0.0f };
			rvn_paint_box4(p, rvn_rect(X, ry, w, rh), radii, fill, 0.0f, 0);
			/* the tab that enters the mouth */
			rvn_be_trapezoid(p, X + BS(RVN_BE_ARM) + nx, X + BS(RVN_BE_ARM) + nx + nw, ry + rh - 0.5f,
				X + BS(RVN_BE_ARM) + nx + nd, X + BS(RVN_BE_ARM) + nx + nw - nd, ry + rh + nd, fill);
			rvn_paint_rect(p, rvn_rect(X, BY(y + b->mouth_y[s]) - 0.5f, BS(RVN_BE_ARM), BS(b->mouth_h[s]) + 1.0f), fill);
		}
		{
			float by = BY(y + b->h - RVN_BE_BOTTOM);
			float radii[4] = { 0.0f, r, r, r };
			rvn_paint_box4(p, rvn_rect(X, by, w, BS(RVN_BE_BOTTOM)), radii, fill, 0.0f, 0);
			rvn_be_trapezoid(p, X + nx, X + nx + nw, by + BS(RVN_BE_BOTTOM) - 0.5f, X + nx + nd, X + nx + nw - nd, by + BS(RVN_BE_BOTTOM) + nd, fill);
		}
	} else {
		float radii[4] = { r, r, r, r };
		if (k->shape == R_BLOCK_HAT) {
			/* a round cap on top of the first row */
			float cap = BS(14.0f);
			rvn_paint_box(p, rvn_rect(X, Y, rvn_minf(w, BS(90.0f)), cap * 2.0f), cap, fill, 0.0f, 0);
			rvn_paint_box4(p, rvn_rect(X, Y + cap, w, h - cap), radii, fill, 0.0f, 0);
		} else {
			rvn_paint_box4(p, rvn_rect(X, Y, w, h), radii, fill, 0.0f, 0);
		}
		if (rvn_be_has_next(k->shape))
			rvn_be_trapezoid(p, X + nx, X + nx + nw, Y + h - 0.5f, X + nx + nd, X + nx + nw - nd, Y + h + nd, fill);
	}
	/* the notch on top : what is behind shows through */
	if (notch_top && rvn_be_has_top(k->shape))
		rvn_be_trapezoid(p, X + nx, X + nx + nw, Y - 0.5f, X + nx + nd, X + nx + nw - nd, Y + nd, behind);
	(void)edge;
}

static void rvn_be_paint_block(RPainter* p, rvn_block_editor_t* e, rvn_be_block_t* b, float x, float y, const rvn_be_look_t* look,
	int notch_top, rvn_color_t behind) {
	const rvn_be_kind_t* k = rvn_be_kind_of(e, b);
	rvn_color_t base, fill, edge, text;
	float hover = rvn_clampf(rvn_paint_spring(p, &b->hover, e->hot_block == b->id && e->drag == RVN_BE_NONE ? 1.0f : 0.0f, R_MOTION_SNAPPY), 0.0f, 1.0f);
	float glow = rvn_clampf(rvn_paint_spring(p, &b->glow_shown, b->glow ? 1.0f : 0.0f, R_MOTION_SMOOTH), 0.0f, 1.0f);
	float saved = p->opacity;
	uint32_t t;
	int s;
	if (!k) return;
	base = rvn_be_category_color(e, k->category);
	fill = rvn_color_mix(base, rvn_rgba(1.0f, 1.0f, 1.0f, 1.0f), 0.10f * hover);
	edge = rvn_color_mix(base, rvn_rgba(0.0f, 0.0f, 0.0f, 1.0f), 0.25f);
	text = rvn_be_light(base) ? look->text_dark : look->text_light;
	p->opacity *= rvn_clampf(b->fade.value, 0.0f, 1.0f) * look->opacity;

	if (glow > 0.01f) {
		/* running : a breathing halo */
		float breath = 0.5f + 0.5f * sinf((float)rvn_clock_seconds() * 5.0f);
		rvn_paint_shadow(p, rvn_rect(BX(x) - BS(3.0f), BY(y) - BS(3.0f), BS(b->w + 6.0f), BS(b->h + 6.0f)), BS(RVN_BE_RADIUS + 3.0f),
			BS(8.0f + 6.0f * breath), rvn_color_fade(look->glow, (0.55f + 0.35f * breath) * glow));
	}
	rvn_be_paint_shape(p, e, b, k, x, y, look, fill, edge, notch_top, behind);
	if (b->sel && !b->palette) {
		float sh = rvn_be_is_value(k->shape) ? b->h : (k->shape == R_BLOCK_C ? b->h : b->row_h[0]);
		rvn_paint_box(p, rvn_rect(BX(x) - 2.0f, BY(y) - 2.0f, BS(b->w) + 4.0f, BS(sh) + 4.0f),
			k->shape == R_BLOCK_REPORTER ? BS(sh * 0.5f) + 2.0f : BS(RVN_BE_RADIUS + 1.0f), 0, 2.0f, rvn_widget_color(&e->base, R_COLOR_ACCENT));
	}

	/* the words and the empty slots of each row */
	for (t = 0; t < k->token_count; t++) {
		const rvn_be_token_t* tok = &k->tokens[t];
		int sec = tok->section;
		float ry = y + b->row_y[sec] + (k->shape == R_BLOCK_HAT && sec == 0 ? 14.0f : 0.0f);
		float rh = b->row_h[sec] - (k->shape == R_BLOCK_HAT && sec == 0 ? 14.0f : 0.0f);
		if (tok->slot < 0) {
			/* (words are placed between the slots : the x is found again from the tokens) */
			float tx = rvn_be_is_value(k->shape) ? (k->shape == R_BLOCK_BOOLEAN ? look->m.slot * 0.55f + 6.0f : look->m.pad + 2.0f) : look->m.pad;
			uint32_t u;
			for (u = 0; u < t; u++) {
				const rvn_be_token_t* o = &k->tokens[u];
				if (o->section != sec) continue;
				tx += o->slot < 0 ? rvn_widget_text_width(&e->base, R_INVALID_ID, look->m.font, o->text) + 4.0f : b->slot_w[o->slot] + 5.0f;
			}
			rvn_paint_text_in(p, R_INVALID_ID, BS(look->m.font), rvn_rect(BX(x + tx), BY(ry), BS(400.0f), BS(rh)), 0.0f, tok->text, -1, text);
		} else if (!b->input[tok->slot] || !rvn_be_block(e, b->input[tok->slot]) || rvn_be_block(e, b->input[tok->slot])->dying) {
			int slot = tok->slot;
			char type = k->slot_types[slot];
			float sx = BX(x + b->slot_x[slot]), sy = BY(y + b->slot_y[slot]), sw = BS(b->slot_w[slot]), sh = BS(b->slot_h[slot]);
			int hot = e->drag == RVN_BE_BLOCKS && e->target == b->id && e->target_place == R_BLOCK_INPUT && e->target_index == slot;
			float light = rvn_clampf(rvn_paint_spring_from(p, &e->ghost_shown, 1.0f, R_MOTION_SNAPPY), 0.0f, 1.0f);
			if (type == 'b') {
				float tip = sh * 0.5f;
				rvn_color_t c = rvn_color_mix(base, rvn_rgba(0.0f, 0.0f, 0.0f, 1.0f), 0.32f);
				rvn_paint_rect(p, rvn_rect(sx + tip, sy, sw - tip * 2.0f, sh), c);
				rvn_paint_triangle(p, sx, sy + tip, sx + tip, sy, sx + tip, sy + sh, c);
				rvn_paint_triangle(p, sx + sw, sy + tip, sx + sw - tip, sy + sh, sx + sw - tip, sy, c);
			} else if (type == 'm') {
				/* a choice : darker than the block, an arrow on the right */
				rvn_color_t c = rvn_color_mix(base, rvn_rgba(0.0f, 0.0f, 0.0f, 1.0f), 0.22f);
				float ax = sx + sw - BS(12.0f), ay = sy + sh * 0.5f, aw = BS(4.0f);
				rvn_paint_box(p, rvn_rect(sx, sy, sw, sh), BS(4.0f), c, BS(1.0f), rvn_color_mix(base, rvn_rgba(0.0f, 0.0f, 0.0f, 1.0f), 0.35f));
				rvn_paint_text_in(p, R_INVALID_ID, BS(look->m.font), rvn_rect(sx + BS(6.0f), sy, sw - BS(22.0f), sh), 0.0f,
					b->value[slot] ? b->value[slot] : "", -1, text);
				rvn_paint_triangle(p, ax - aw, ay - aw * 0.5f, ax + aw, ay - aw * 0.5f, ax, ay + aw * 0.7f, text);
			} else {
				rvn_paint_box(p, rvn_rect(sx, sy, sw, sh), type == 'n' ? sh * 0.5f : BS(4.0f), look->slot_fill, BS(1.0f), look->slot_line);
				rvn_paint_text_in(p, R_INVALID_ID, BS(look->m.font), rvn_rect(sx, sy, sw, sh), 0.5f, b->value[slot] ? b->value[slot] : "", -1, look->slot_text);
			}
			if (hot) {
				rvn_paint_box(p, rvn_rect(sx - 2.0f, sy - 2.0f, sw + 4.0f, sh + 4.0f), sh * 0.5f + 2.0f, 0, 2.5f,
					rvn_color_fade(rvn_rgba(1.0f, 1.0f, 1.0f, 1.0f), 0.9f * light));
			}
		}
	}
	(void)s;
	p->opacity = saved;
}

/* A block and what hangs under it, at their shown positions. `behind`: the color the notches show. */
static void rvn_be_paint_tree(RPainter* p, rvn_block_editor_t* e, rvn_be_block_t* b, const rvn_be_look_t* look, int notch_top, rvn_color_t behind) {
	const rvn_be_kind_t* k = rvn_be_kind_of(e, b);
	int s;
	if (!b || !k) return;
	rvn_be_paint_block(p, e, b, b->sx.value, b->sy.value, look, notch_top, behind);
	for (s = 0; s < k->slot_count; s++) {
		rvn_be_block_t* input = rvn_be_block(e, b->input[s]);
		if (input && !input->dying) rvn_be_paint_tree(p, e, input, look, R_FALSE, behind);
	}
	if (k->shape == R_BLOCK_C)
		for (s = 0; s < k->sections; s++) {
			rvn_be_block_t* c = rvn_be_block(e, b->body[s]);
			int first = R_TRUE, guard = 0;
			while (c && guard++ < 100000) {
				rvn_be_paint_tree(p, e, c, look, first, rvn_be_category_color(e, k->category));
				first = R_FALSE;
				c = rvn_be_block(e, c->next);
			}
		}
	if (b->next) {
		rvn_be_block_t* n = rvn_be_block(e, b->next);
		if (n && !n->dying) rvn_be_paint_tree(p, e, n, look, R_FALSE, behind);
	}
}

/* Moves the shown positions of a script toward their layout. */
static void rvn_be_animate(RPainter* p, rvn_block_editor_t* e) {
	uint32_t i;
	for (i = 0; i < e->block_count; i++) {
		rvn_be_block_t* b = &e->blocks[i];
		if (b->palette) {
			rvn_spring_set(&b->sx, b->lx);
			rvn_spring_set(&b->sy, b->ly);
			continue;
		}
		if (!b->placed) {
			rvn_spring_set(&b->sx, b->lx);
			rvn_spring_set(&b->sy, b->ly);
			b->placed = R_TRUE;
		} else if (e->drag == RVN_BE_BLOCKS && rvn_be_inside(e, b->id, e->carried)) {
			/* (the carried stack sticks to the pointer) */
			rvn_spring_set(&b->sx, b->lx);
			rvn_spring_set(&b->sy, b->ly);
		} else {
			rvn_paint_spring(p, &b->sx, b->lx, R_MOTION_SNAPPY);
			rvn_paint_spring(p, &b->sy, b->ly, R_MOTION_SNAPPY);
		}
		rvn_paint_spring(p, &b->appear, 1.0f, R_MOTION_BOUNCY);
		if (b->dying) {
			rvn_paint_spring(p, &b->fade, 0.0f, R_MOTION_SMOOTH);
			rvn_paint_keep_animating(p);
		}
		if (b->glow) rvn_paint_keep_animating(p);
	}
}

static void rvn_block_editor_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_block_editor_t* e = (rvn_block_editor_t*)widget;
	rvn_rect_t bounds = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	rvn_color_t canvas = rvn_widget_color(widget, R_COLOR_INPUT);
	rvn_be_look_t look;
	float left = rvn_be_canvas_left(e), zoom;
	uint32_t i;

	/* the carried stack follows the pointer : its top is laid out there */
	if (e->drag == RVN_BE_BLOCKS) {
		rvn_be_block_t* d = rvn_be_block(e, e->carried);
		if (d) {
			float cx, cy;
			rvn_be_to_canvas(e, e->pointer_x, e->pointer_y, &cx, &cy);
			d->x = cx - e->grab_x;
			d->y = cy - e->grab_y;
		}
		rvn_be_find_target(e);
	}
	rvn_be_layout(e);
	rvn_be_animate(painter, e);
	rvn_be_purge(e, R_FALSE);
	e->painted = R_TRUE;

	memset(&look, 0, sizeof(look));
	look.m = rvn_be_metrics(widget);
	look.dark = rvn_be_dark_theme(widget);
	look.text_light = rvn_rgba(1.0f, 1.0f, 1.0f, 1.0f);
	look.text_dark = rvn_hex(0x1C1D22, 255);
	look.slot_fill = look.dark ? rvn_color_mix(canvas, rvn_rgba(0.0f, 0.0f, 0.0f, 1.0f), 0.25f) : rvn_rgba(1.0f, 1.0f, 1.0f, 1.0f);
	look.slot_text = look.dark ? rvn_rgba(0.95f, 0.95f, 0.97f, 1.0f) : rvn_hex(0x575E75, 255);
	look.slot_line = rvn_color_fade(rvn_rgba(0.0f, 0.0f, 0.0f, 1.0f), 0.18f);
	look.glow = rvn_widget_color(widget, R_COLOR_WARNING);
	look.opacity = 1.0f;

	/* ---- the canvas : the color of a field, a grid of dots ---- */
	rvn_paint_box(painter, bounds, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS), canvas, 1.0f, rvn_widget_color(widget, R_COLOR_BORDER));
	zoom = rvn_clampf(rvn_paint_spring(painter, &e->shown_zoom, e->zoom, R_MOTION_SNAPPY), 0.2f, 4.0f);
	rvn_paint_push_clip(painter, rvn_rect(left + 1.0f, 1.0f, bounds.w - left - 2.0f, bounds.h - 2.0f));
	{
		float step = RVN_BE_GRID * zoom, x, y, fx, fy;
		rvn_color_t dot = rvn_color_fade(rvn_widget_color(widget, R_COLOR_TEXT), 0.10f);
		while (step < 12.0f) step *= 2.0f;
		fx = left + fmodf(-e->pan_x * zoom, step);
		fy = fmodf(-e->pan_y * zoom, step);
		if (fx < left) fx += step;
		if (fy < 0.0f) fy += step;
		for (y = fy; y < bounds.h; y += step)
			for (x = fx; x < bounds.w; x += step) rvn_paint_rect(painter, rvn_rect(x - 1.0f, y - 1.0f, 2.0f, 2.0f), dot);
	}
	look.ox = left - e->pan_x * zoom;
	look.oy = -e->pan_y * zoom;
	look.scale = zoom;

	/* ---- the scripts ---- */
	for (i = 0; i < e->order_count; i++) {
		rvn_be_block_t* top = rvn_be_block(e, e->order[i]);
		if (!top || top->id == e->carried) continue;
		rvn_be_paint_tree(painter, e, top, &look, R_TRUE, canvas);
	}
	/* blocks that go away (dropped on the palette) */
	for (i = 0; i < e->block_count; i++) {
		rvn_be_block_t* b = &e->blocks[i];
		if (b->dying && !b->palette) rvn_be_paint_block(painter, e, b, b->sx.value, b->sy.value, &look, R_FALSE, canvas);
	}
	/* the shadow of the carried stack where it would go */
	{
		rvn_be_block_t* d = rvn_be_block(e, e->carried);
		int showing = e->drag == RVN_BE_BLOCKS && d && (e->target || e->target_top) && e->target_place != R_BLOCK_INPUT;
		float shown = rvn_clampf(rvn_paint_spring(painter, &e->ghost_shown, showing ? 1.0f : 0.0f, R_MOTION_SNAPPY), 0.0f, 1.0f);
		if (showing && shown > 0.02f) {
			const rvn_be_kind_t* k = rvn_be_kind_of(e, d);
			rvn_color_t shade = rvn_color_fade(rvn_widget_color(widget, R_COLOR_TEXT), 0.16f * shown);
			if (k) {
				float h = 0.0f;
				rvn_be_block_t* c = d;
				int guard = 0;
				while (c && guard++ < 100000) { h += c->h; c = rvn_be_block(e, c->next); }
				rvn_paint_box(painter, rvn_rect(look.ox + e->ghost_x * zoom, look.oy + e->ghost_y * zoom, d->w * zoom, h * zoom * shown),
					RVN_BE_RADIUS * zoom, shade, 1.0f, rvn_color_fade(rvn_widget_color(widget, R_COLOR_TEXT), 0.3f * shown));
			}
		}
	}
	/* the selection rectangle */
	if (e->drag == RVN_BE_MARQUEE) {
		float x0 = rvn_minf(e->marquee_x0, e->marquee_x1), x1 = rvn_maxf(e->marquee_x0, e->marquee_x1);
		float y0 = rvn_minf(e->marquee_y0, e->marquee_y1), y1 = rvn_maxf(e->marquee_y0, e->marquee_y1);
		rvn_color_t accent = rvn_widget_color(widget, R_COLOR_ACCENT);
		rvn_paint_box(painter, rvn_rect(look.ox + x0 * zoom, look.oy + y0 * zoom, (x1 - x0) * zoom, (y1 - y0) * zoom), 2.0f,
			rvn_color_fade(accent, 0.12f), 1.0f, rvn_color_fade(accent, 0.8f));
	}
	/* the carried stack, lifted */
	if (e->drag == RVN_BE_BLOCKS) {
		rvn_be_block_t* d = rvn_be_block(e, e->carried);
		if (d) {
			float h = 0.0f;
			rvn_be_block_t* c = d;
			int guard = 0;
			while (c && guard++ < 100000) { h += c->h; c = rvn_be_block(e, c->next); }
			rvn_paint_shadow(painter, rvn_rect(look.ox + d->sx.value * zoom + 4.0f, look.oy + d->sy.value * zoom + 8.0f, d->w * zoom, h * zoom),
				RVN_BE_RADIUS * zoom, 18.0f, rvn_rgba(0.0f, 0.0f, 0.0f, 0.35f));
			look.opacity = rvn_be_in_palette(e, e->pointer_x) ? 0.5f : 0.96f;
			rvn_be_paint_tree(painter, e, d, &look, R_FALSE, canvas);
			look.opacity = 1.0f;
		}
	}
	rvn_paint_pop_clip(painter);

	/* ---- the palette ---- */
	if (!e->palette_hidden) {
		rvn_be_look_t pl = look;
		float trash = rvn_clampf(rvn_paint_spring(painter, &e->trash, e->drag == RVN_BE_BLOCKS && rvn_be_in_palette(e, e->pointer_x) ? 1.0f : 0.0f, R_MOTION_SNAPPY), 0.0f, 1.0f);
		const char* category = NULL;
		float radii[4] = { rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS), 0.0f, 0.0f, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS) };
		rvn_color_t panel = rvn_color_mix(rvn_widget_color(widget, R_COLOR_PANEL), rvn_widget_color(widget, R_COLOR_DANGER), 0.18f * trash);
		rvn_paint_box4(painter, rvn_rect(0.0f, 0.0f, RVN_BE_PALETTE_W, bounds.h), radii, panel, 0.0f, 0);
		rvn_paint_rect(painter, rvn_rect(RVN_BE_PALETTE_W - 1.0f, 0.0f, 1.0f, bounds.h), rvn_widget_color(widget, R_COLOR_BORDER));
		rvn_paint_push_clip(painter, rvn_rect(0.0f, 1.0f, RVN_BE_PALETTE_W - 1.0f, bounds.h - 2.0f));
		pl.ox = 0.0f;
		pl.oy = -e->palette_scroll;
		pl.scale = 1.0f;
		for (i = 0; i < e->block_count; i++) {
			rvn_be_block_t* b = &e->blocks[i];
			const rvn_be_kind_t* k;
			if (!b->palette) continue;
			k = rvn_be_kind_of(e, b);
			if (!k || k->hidden) continue;
			if (!category || strcmp(category, k->category ? k->category : "") != 0) {
				category = k->category ? k->category : "";
				if (category[0]) {
					rvn_color_t c = rvn_be_category_color(e, category);
					float hy = b->ly - 24.0f - e->palette_scroll;
					rvn_paint_box(painter, rvn_rect(14.0f, hy + 6.0f, 10.0f, 10.0f), 5.0f, c, 0.0f, 0);
					rvn_paint_text_in(painter, R_INVALID_ID, look.m.font, rvn_rect(30.0f, hy, RVN_BE_PALETTE_W - 40.0f, 22.0f), 0.0f, category, -1,
						rvn_widget_color(widget, R_COLOR_TEXT));
				}
			}
			pl.opacity = 1.0f - 0.6f * trash;
			rvn_be_paint_block(painter, e, b, b->lx, b->ly, &pl, R_TRUE, panel);
		}
		if (trash > 0.02f) {
			rvn_paint_text_in(painter, R_INVALID_ID, look.m.font * 1.1f, rvn_rect(0.0f, bounds.h * 0.5f - 20.0f, RVN_BE_PALETTE_W, 40.0f), 0.5f,
				"Drop here to delete", -1, rvn_color_fade(rvn_widget_color(widget, R_COLOR_TEXT), trash));
		}
		rvn_paint_pop_clip(painter);
	}

	/* the editor of a slot follows the block (pan, zoom, layout) */
	if (e->field && e->field->visibility == R_VISIBLE) rvn_widget_invalidate_layout(widget);
}

/* ---------------------------------------------------------------------------
 *  The editor of a value slot (a text field over it)
 * ------------------------------------------------------------------------- */

static void rvn_be_close_field(rvn_block_editor_t* e) {
	if (!e->field) return;
	e->edit_block = 0;
	e->field->visibility = R_COLLAPSED;
	rvn_widget_invalidate_layout(&e->base);
}

/* Keeps what was typed in the slot being edited (a number slot keeps a number), and closes the editor. */
static void rvn_be_apply_field(rvn_block_editor_t* e, const char* text) {
	rvn_be_block_t* b = rvn_be_block(e, e->edit_block);
	const rvn_be_kind_t* k = rvn_be_kind_of(e, b);
	const char* value = text ? text : "";
	char number[64];
	int changed = R_FALSE;
	if (b && k && e->edit_slot >= 0 && e->edit_slot < k->slot_count) {
		if (k->slot_types[e->edit_slot] == 'n') {
			char* end = NULL;
			double v = strtod(value, &end);
			if (end == value) v = 0.0;
			snprintf(number, sizeof(number), "%g", v);
			value = number;
		}
		if (!b->value[e->edit_slot] || strcmp(b->value[e->edit_slot], value) != 0) {
			rvn_str_set(&b->value[e->edit_slot], value);
			changed = R_TRUE;
		}
	}
	rvn_be_close_field(e);
	if (changed) {
		rvn_be_changed(e);
		rvn_be_notify(e, b->id);
	}
}

/* A click elsewhere keeps the edit too (Scratch). */
static void rvn_be_finish_field(rvn_block_editor_t* e) {
	if (!e->field || !e->edit_block || e->field->visibility != R_VISIBLE) {
		rvn_be_close_field(e);
		return;
	}
	{
		char* text = rvn_strdup(Rtext_input_get_text(e->field->id));
		rvn_be_apply_field(e, text);
		rvn_free(text);
	}
}

static void rvn_be_field_committed(Rid field, const char* text, void* user) {
	rvn_block_editor_t* e = (rvn_block_editor_t*)rvn_widget_check((Rid)(uintptr_t)user, R_WIDGET_BLOCK_EDITOR, "block editor");
	(void)field;
	if (!e || !e->edit_block) return;
	rvn_be_apply_field(e, text);
}

static void rvn_be_open_field(rvn_block_editor_t* e, int block, int slot) {
	rvn_be_block_t* b = rvn_be_block(e, block);
	if (!b || slot < 0) return;
	if (!e->field) {
		e->field = rvn_widget_create(R_WIDGET_TEXT_INPUT);
		if (!e->field) return;
		rvn_widget_add_child(&e->base, e->field, -1);
		Rtext_input_set_committed_callback(e->field->id, rvn_be_field_committed, (void*)(uintptr_t)e->base.id);
	}
	e->edit_block = block;
	e->edit_slot = slot;
	Rtext_input_set_text(e->field->id, b->value[slot] ? b->value[slot] : "");
	Rtext_input_select_all(e->field->id);
	e->field->visibility = R_VISIBLE;
	rvn_widget_invalidate_layout(&e->base);
	rvn_widget_focus(e->field);
}

/* ---------------------------------------------------------------------------
 *  Selection
 * ------------------------------------------------------------------------- */

static void rvn_be_clear_selection(rvn_block_editor_t* e) {
	uint32_t i;
	for (i = 0; i < e->block_count; i++) e->blocks[i].sel = R_FALSE;
	e->selected = 0;
}

static void rvn_be_select(rvn_block_editor_t* e, int id, int on) {
	rvn_be_block_t* b = rvn_be_block(e, id);
	if (!b || b->palette || b->dying) return;
	b->sel = on;
	if (on) e->selected = id;
	else if (e->selected == id) e->selected = 0;
}

static int rvn_be_selection_count(const rvn_block_editor_t* e) {
	uint32_t i;
	int n = 0;
	for (i = 0; i < e->block_count; i++)
		if (e->blocks[i].sel && !e->blocks[i].palette && !e->blocks[i].dying) n++;
	return n;
}

/* A selected block none of whose ancestors is selected : it goes with what hangs under it. */
static int rvn_be_is_selection_root(rvn_block_editor_t* e, const rvn_be_block_t* b) {
	const rvn_be_block_t* p;
	int guard = 0;
	if (!b->sel || b->palette || b->dying) return R_FALSE;
	p = rvn_be_block(e, b->parent);
	while (p && guard++ < 100000) {
		if (p->sel) return R_FALSE;
		p = rvn_be_block(e, p->parent);
	}
	return R_TRUE;
}

static void rvn_be_collect_roots(rvn_block_editor_t* e) {
	uint32_t i;
	e->group_count = 0;
	for (i = 0; i < e->block_count; i++) {
		const rvn_be_block_t* b = &e->blocks[i];
		if (!rvn_be_is_selection_root(e, b)) continue;
		RVN_ARRAY_RESERVE(e->group, e->group_cap, e->group_count + 1);
		e->group[e->group_count++] = b->id;
	}
}

/* The blocks the rectangle touches are selected (with those selected before it, Ctrl / Shift). */
static void rvn_be_update_marquee(rvn_block_editor_t* e) {
	float x0 = rvn_minf(e->marquee_x0, e->marquee_x1), x1 = rvn_maxf(e->marquee_x0, e->marquee_x1);
	float y0 = rvn_minf(e->marquee_y0, e->marquee_y1), y1 = rvn_maxf(e->marquee_y0, e->marquee_y1);
	uint32_t i;
	e->selected = 0;
	for (i = 0; i < e->block_count; i++) {
		rvn_be_block_t* b = &e->blocks[i];
		int hit;
		if (b->palette || b->dying) continue;
		hit = b->sx.value < x1 && b->sx.value + b->w > x0 && b->sy.value < y1 && b->sy.value + b->h > y0;
		b->sel = b->sel_base || hit;
		if (b->sel) e->selected = b->id;
	}
}

/* Moving the selection : its roots become scripts of their own, they follow the pointer. */
static void rvn_be_begin_group(rvn_block_editor_t* e, float x, float y) {
	uint32_t i;
	rvn_be_finish_field(e);
	rvn_be_collect_roots(e);
	for (i = 0; i < e->group_count; i++) {
		rvn_be_block_t* b = rvn_be_block(e, e->group[i]);
		if (b && b->parent) rvn_be_make_top(e, b, b->sx.value, b->sy.value);
	}
	rvn_be_to_canvas(e, x, y, &e->group_x, &e->group_y);
	e->drag = RVN_BE_GROUP;
	rvn_be_changed(e);
}

static void rvn_be_move_group(rvn_block_editor_t* e, float x, float y) {
	float cx, cy;
	uint32_t i;
	rvn_be_to_canvas(e, x, y, &cx, &cy);
	for (i = 0; i < e->group_count; i++) {
		rvn_be_block_t* b = rvn_be_block(e, e->group[i]);
		if (!b || b->parent) continue;
		b->x += cx - e->group_x;
		b->y += cy - e->group_y;
	}
	e->group_x = cx;
	e->group_y = cy;
	rvn_be_changed(e);
}

static void rvn_be_delete_selection(rvn_block_editor_t* e) {
	uint32_t i;
	rvn_be_collect_roots(e);
	for (i = 0; i < e->group_count; i++) {
		rvn_be_block_t* b = rvn_be_block(e, e->group[i]);
		if (!b) continue;
		if (b->locked) {
			/* it stays; what hangs under it goes if it was selected too */
			rvn_be_block_t* n = rvn_be_block(e, b->next);
			b->sel = R_FALSE;
			if (n && n->sel) rvn_be_delete_script(e, n->id, R_TRUE);
			continue;
		}
		rvn_be_delete_script(e, e->group[i], R_TRUE);
	}
	e->selected = 0;
}

/* ---------------------------------------------------------------------------
 *  The menu of a %m slot
 * ------------------------------------------------------------------------- */

static void rvn_be_menu_clicked(Rid item, void* user) {
	rvn_block_editor_t* e = (rvn_block_editor_t*)rvn_widget_check((Rid)(uintptr_t)user, R_WIDGET_BLOCK_EDITOR, "block editor");
	rvn_be_block_t* b;
	const char* text;
	if (!e) return;
	b = rvn_be_block(e, e->menu_block);
	text = Rmenu_item_get_text(item);
	if (!b || !text || e->menu_slot < 0 || e->menu_slot >= RVN_BE_MAX_SLOTS) return;
	if (!b->value[e->menu_slot] || strcmp(b->value[e->menu_slot], text) != 0) {
		rvn_str_set(&b->value[e->menu_slot], text);
		rvn_be_changed(e);
		rvn_be_notify(e, b->id);
	}
}

/* The choices of the slot under it ; a slot without choices is typed in. */
static void rvn_be_open_menu(rvn_block_editor_t* e, int block, int slot) {
	rvn_be_block_t* b = rvn_be_block(e, block);
	const rvn_be_kind_t* k = rvn_be_kind_of(e, b);
	const char* c;
	rvn_vec2_t origin;
	float x, y, zoom;
	int items = 0;
	if (!b || !k || slot < 0 || slot >= k->slot_count) return;
	if (!k->options[slot] || !k->options[slot][0]) {
		rvn_be_open_field(e, block, slot);
		return;
	}
	if (e->menu != R_INVALID_ID && Rwidget_is_valid(e->menu)) Rwidget_destroy(e->menu);
	e->menu = Rwidget_create(R_WIDGET_MENU);
	if (e->menu == R_INVALID_ID) return;
	e->menu_block = block;
	e->menu_slot = slot;
	for (c = k->options[slot]; *c;) {
		const char* end = strchr(c, '\n');
		size_t n = end ? (size_t)(end - c) : strlen(c);
		if (n > 0) {
			char* text = rvn_strndup(c, n);
			Rid item = Rmenu_add_item(e->menu, text);
			if (b->value[slot] && strcmp(b->value[slot], text) == 0) {
				Rmenu_item_set_checkable(item, R_TRUE);
				Rmenu_item_set_checked(item, R_TRUE);
			}
			Rmenu_item_set_clicked_callback(item, rvn_be_menu_clicked, (void*)(uintptr_t)e->base.id);
			rvn_free(text);
			items++;
		}
		c += n;
		if (*c == '\n') c++;
	}
	if (!items) return;
	origin = rvn_widget_origin(&e->base);
	zoom = e->shown_zoom.value > 0.0f ? e->shown_zoom.value : 1.0f;
	x = origin.x + rvn_be_canvas_left(e) + (b->sx.value + b->slot_x[slot] - e->pan_x) * zoom;
	y = origin.y + (b->sy.value + b->slot_y[slot] + b->slot_h[slot] - e->pan_y) * zoom + 2.0f;
	rvn_widget_from_virtual(&e->base, &x, &y);
	Rmenu_open(e->menu, Rwidget_get_window(e->base.id), x, y);
}

/* ---------------------------------------------------------------------------
 *  Widget class
 * ------------------------------------------------------------------------- */

static void rvn_block_editor_init(rvn_widget_t* widget) {
	rvn_block_editor_t* e = (rvn_block_editor_t*)widget;
	e->next_id = 1;
	e->zoom = 1.0f;
	rvn_spring_set(&e->shown_zoom, 1.0f);
	e->pan_x = -20.0f;
	e->pan_y = -20.0f;
	e->layout_dirty = R_TRUE;
	e->menu = R_INVALID_ID;
}

static void rvn_block_editor_destroy(rvn_widget_t* widget) {
	rvn_block_editor_t* e = (rvn_block_editor_t*)widget;
	uint32_t i;
	for (i = 0; i < e->block_count; i++) rvn_be_free_block_data(&e->blocks[i]);
	for (i = 0; i < e->kind_count; i++) rvn_be_free_kind(&e->kinds[i]);
	for (i = 0; i < e->category_count; i++) rvn_free(e->categories[i].name);
	rvn_free(e->blocks);
	rvn_free(e->kinds);
	rvn_free(e->categories);
	rvn_free(e->order);
	rvn_free(e->drawn);
	rvn_free(e->group);
	rvn_free(e->saved.data);
	if (e->menu != R_INVALID_ID && Rwidget_is_valid(e->menu)) Rwidget_destroy(e->menu);
}

static rvn_vec2_t rvn_block_editor_measure(rvn_widget_t* widget) {
	(void)widget;
	return rvn_vec2(560.0f, 360.0f);
}

static void rvn_block_editor_arrange(rvn_widget_t* widget) {
	rvn_block_editor_t* e = (rvn_block_editor_t*)widget;
	rvn_be_block_t* b;
	float x, y, zoom;
	if (!e->field) return;
	b = rvn_be_block(e, e->edit_block);
	if (!b || e->field->visibility != R_VISIBLE) {
		rvn_widget_arrange(e->field, 0.0f, 0.0f, 0.0f, 0.0f);
		return;
	}
	zoom = e->shown_zoom.value > 0.0f ? e->shown_zoom.value : 1.0f;
	x = rvn_be_canvas_left(e) + (b->sx.value + b->slot_x[e->edit_slot] - e->pan_x) * zoom;
	y = (b->sy.value + b->slot_y[e->edit_slot] - e->pan_y) * zoom;
	rvn_widget_measure(e->field);
	rvn_widget_arrange(e->field, x - 2.0f, y + (b->slot_h[e->edit_slot] * zoom - e->field->desired.y) * 0.5f,
		rvn_maxf(60.0f, b->slot_w[e->edit_slot] * zoom + 30.0f), e->field->desired.y);
}

/* Starts carrying a block : it leaves its place, with what is under it (statements). */
static void rvn_be_begin_carry(rvn_block_editor_t* e, rvn_be_block_t* b, float x, float y) {
	float cx, cy;
	int id = b->id;
	rvn_be_to_canvas(e, x, y, &cx, &cy);
	e->grab_x = cx - b->sx.value;
	e->grab_y = cy - b->sy.value;
	rvn_be_make_top(e, b, b->sx.value, b->sy.value);
	b = rvn_be_block(e, id);
	rvn_be_order_raise(e, id);
	e->carried = id;
	e->drag = RVN_BE_BLOCKS;
	rvn_be_clear_selection(e);
	rvn_be_select(e, id, R_TRUE);
	rvn_be_finish_field(e);
	rvn_be_changed(e);
}

static void rvn_be_end_carry(rvn_block_editor_t* e, int cancel) {
	rvn_be_block_t* d = rvn_be_block(e, e->carried);
	int id = e->carried;
	e->drag = RVN_BE_NONE;
	e->carried = 0;
	if (!d) return;
	if (!cancel && rvn_be_in_palette(e, e->pointer_x) && d->locked) {
		/* it cannot be deleted : back on the canvas */
		float cx, cy;
		rvn_be_to_canvas(e, RVN_BE_PALETTE_W + 20.0f, e->pointer_y, &cx, &cy);
		d->x = cx;
		e->target = e->target_top = 0;
		rvn_be_changed(e);
		rvn_be_notify(e, id);
		return;
	}
	if (!cancel && rvn_be_in_palette(e, e->pointer_x)) {
		/* on the palette : deleted (it fades away) */
		rvn_be_delete_script(e, id, R_TRUE);
		e->target = e->target_top = 0;
		rvn_be_notify(e, 0);
		return;
	}
	if (!cancel && e->target) {
		rvn_be_attach(e, d, rvn_be_block(e, e->target), e->target_place, e->target_index);
	} else if (!cancel && e->target_top) {
		/* above a script : the script hangs under the carried stack */
		rvn_be_block_t* below = rvn_be_block(e, e->target_top);
		rvn_be_block_t* last = rvn_be_last_of_chain(e, d);
		if (below && last) {
			/* (the script stays where it is : the carried stack goes over it) */
			d->x = below->x;
			d->y = below->y - e->carried_h;
			rvn_be_order_remove(e, below->id);
			last->next = below->id;
			below->parent = last->id;
			below->place = R_BLOCK_NEXT;
			below->index = 0;
		}
	}
	e->target = e->target_top = 0;
	rvn_be_changed(e);
	rvn_be_notify(e, id);
}

static float rvn_be_stack_height(rvn_block_editor_t* e, rvn_be_block_t* b) {
	float h = 0.0f;
	int guard = 0;
	rvn_be_metrics_t m = rvn_be_metrics(&e->base);
	while (b && guard++ < 100000) {
		rvn_be_measure_block(e, b, &m);
		h += b->h;
		b = rvn_be_block(e, b->next);
	}
	return h;
}

static int rvn_block_editor_event(rvn_widget_t* widget, const REvent* event) {
	rvn_block_editor_t* e = (rvn_block_editor_t*)widget;
	rvn_vec2_t origin = rvn_widget_origin(widget);
	float x = event->x - origin.x, y = event->y - origin.y;
	int slot = -1, id;
	int add = (event->modifiers & (R_MOD_CTRL | R_MOD_SHIFT)) != 0;
	uint32_t i;

	switch (event->type) {
		case R_EVENT_MOUSE_MOVE: {
			int moved;
			e->pointer_x = x;
			e->pointer_y = y;
			moved = fabsf(x - e->press_x) + fabsf(y - e->press_y) > 5.0f;
			if (e->drag == RVN_BE_PRESS || e->drag == RVN_BE_PRESS_SLOT || e->drag == RVN_BE_PRESS_PALETTE) {
				if (moved) {
					rvn_be_block_t* b = rvn_be_block(e, e->press_block);
					if (b && e->drag == RVN_BE_PRESS_PALETTE) {
						/* a new block from the palette : a copy, carried from where it shows */
						rvn_be_block_t* n = rvn_be_new_block(e, b->kind, R_FALSE);
						float bx, by;
						int nid = n->id;
						b = rvn_be_block(e, e->press_block);
						rvn_be_to_canvas(e, b->lx, b->ly - e->palette_scroll, &bx, &by);
						n = rvn_be_block(e, nid);
						n->x = bx;
						n->y = by;
						rvn_spring_set(&n->sx, bx);
						rvn_spring_set(&n->sy, by);
						n->placed = R_TRUE;
						rvn_be_order_add(e, nid);
						e->layout_dirty = R_TRUE;
						rvn_be_layout(e);
						n = rvn_be_block(e, nid);
						rvn_spring_set(&n->sx, bx);
						rvn_spring_set(&n->sy, by);
						rvn_be_begin_carry(e, n, e->press_x, e->press_y);
					} else if (b && b->sel && rvn_be_selection_count(e) > 1) {
						/* a block of a selection : the whole selection moves */
						rvn_be_begin_group(e, e->press_x, e->press_y);
						rvn_be_move_group(e, x, y);
					} else if (b) {
						rvn_be_begin_carry(e, b, e->press_x, e->press_y);
					} else {
						e->drag = RVN_BE_NONE;
					}
					if (e->drag == RVN_BE_BLOCKS) e->carried_h = rvn_be_stack_height(e, rvn_be_block(e, e->carried));
				}
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			if (e->drag == RVN_BE_BLOCKS) {
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			if (e->drag == RVN_BE_GROUP) {
				rvn_be_move_group(e, x, y);
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			if (e->drag == RVN_BE_PRESS_RIGHT) {
				if (!moved) return R_TRUE;
				e->drag = RVN_BE_PAN;
			}
			if (e->drag == RVN_BE_PAN) {
				float zoom = e->zoom > 0.0f ? e->zoom : 1.0f;
				e->pan_x = e->pan_from_x - (x - e->press_x) / zoom;
				e->pan_y = e->pan_from_y - (y - e->press_y) / zoom;
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			if (e->drag == RVN_BE_MARQUEE) {
				rvn_be_to_canvas(e, x, y, &e->marquee_x1, &e->marquee_y1);
				rvn_be_update_marquee(e);
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			id = rvn_be_block_at(e, x, y, &slot);
			if (id != e->hot_block) {
				e->hot_block = id;
				rvn_widget_invalidate_paint(widget);
			}
			return R_FALSE;
		}
		case R_EVENT_MOUSE_LEAVE:
			e->hot_block = 0;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_WHEEL:
			if (rvn_be_in_palette(e, x)) {
				float most = rvn_maxf(0.0f, e->palette_height - widget->rect.h);
				e->palette_scroll = rvn_clampf(e->palette_scroll - event->wheel_y * 48.0f, 0.0f, most);
			} else if (event->modifiers & R_MOD_CTRL) {
				/* zoom around the pointer */
				float before_x, before_y, after_x, after_y;
				rvn_be_to_canvas(e, x, y, &before_x, &before_y);
				/* (as far as the wheel turned: the small turns of a trackpad zoom a little) */
				e->zoom = rvn_clampf(e->zoom * powf(1.12f, rvn_clampf(event->wheel_y, -4.0f, 4.0f)), 0.3f, 3.0f);
				rvn_spring_set(&e->shown_zoom, e->zoom);
				rvn_be_to_canvas(e, x, y, &after_x, &after_y);
				e->pan_x += before_x - after_x;
				e->pan_y += before_y - after_y;
			} else if (event->wheel_precise) {
				/* a trackpad: the canvas follows the two fingers, both ways at once */
				e->pan_x -= event->wheel_x * RVN_WHEEL_LINE / e->zoom;
				e->pan_y -= event->wheel_y * RVN_WHEEL_LINE / e->zoom;
			} else if (event->modifiers & R_MOD_SHIFT || event->wheel_x != 0.0f) {
				e->pan_x -= (event->wheel_x != 0.0f ? event->wheel_x : event->wheel_y) * 48.0f / e->zoom;
			} else {
				e->pan_y -= event->wheel_y * 48.0f / e->zoom;
			}
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_DOWN: {
			rvn_be_block_t* b;
			if (e->drag != RVN_BE_NONE) return R_TRUE;
			rvn_widget_focus(widget);
			e->pointer_x = e->press_x = x;
			e->pointer_y = e->press_y = y;
			if (event->button == R_MOUSE_MIDDLE || (event->button == R_MOUSE_LEFT && (event->modifiers & R_MOD_ALT))) {
				e->drag = RVN_BE_PAN;
				e->pan_from_x = e->pan_x;
				e->pan_from_y = e->pan_y;
				rvn_widget_capture_mouse(widget);
				return R_TRUE;
			}
			if (event->button == R_MOUSE_RIGHT) {
				/* a pan if it moves, the context menu otherwise (on release) */
				if (rvn_be_in_palette(e, x)) return R_TRUE;
				rvn_be_finish_field(e);
				if (!rvn_widget_alive(widget)) return R_TRUE;
				e->drag = RVN_BE_PRESS_RIGHT;
				e->pan_from_x = e->pan_x;
				e->pan_from_y = e->pan_y;
				rvn_widget_capture_mouse(widget);
				return R_TRUE;
			}
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			id = rvn_be_block_at(e, x, y, &slot);
			/* closing the field calls the application back, which may add or remove blocks:
			 * look the pressed block up again afterwards */
			rvn_be_finish_field(e);
			if (!rvn_widget_alive(widget)) return R_TRUE;
			b = rvn_be_block(e, id);
			if (!b) {
				if (rvn_be_in_palette(e, x)) return R_TRUE;
				/* a selection rectangle (Ctrl / Shift : added to the selection) */
				if (!add) rvn_be_clear_selection(e);
				for (i = 0; i < e->block_count; i++) e->blocks[i].sel_base = e->blocks[i].sel;
				rvn_be_to_canvas(e, x, y, &e->marquee_x0, &e->marquee_y0);
				e->marquee_x1 = e->marquee_x0;
				e->marquee_y1 = e->marquee_y0;
				e->drag = RVN_BE_MARQUEE;
				rvn_widget_capture_mouse(widget);
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			e->press_block = id;
			e->press_slot = slot;
			e->drag = b->palette ? RVN_BE_PRESS_PALETTE : (slot >= 0 ? RVN_BE_PRESS_SLOT : RVN_BE_PRESS);
			rvn_widget_capture_mouse(widget);
			return R_TRUE;
		}
		case R_EVENT_MOUSE_UP: {
			rvn_be_drag_t drag = e->drag;
			if (drag == RVN_BE_NONE) return R_FALSE;
			rvn_widget_release_mouse(widget);
			e->pointer_x = x;
			e->pointer_y = y;
			if (drag == RVN_BE_BLOCKS) {
				rvn_be_end_carry(e, R_FALSE);
			} else if (drag == RVN_BE_GROUP) {
				e->drag = RVN_BE_NONE;
				rvn_be_changed(e);
				rvn_be_notify(e, 0);
			} else if (drag == RVN_BE_MARQUEE) {
				e->drag = RVN_BE_NONE;
			} else if (drag == RVN_BE_PRESS_RIGHT) {
				rvn_be_block_t* b;
				e->drag = RVN_BE_NONE;
				id = rvn_be_block_at(e, e->press_x, e->press_y, &slot);
				b = rvn_be_block(e, id);
				if (b && b->palette) {
					b = NULL;
					id = 0;
				}
				if (b && !b->sel) {
					rvn_be_clear_selection(e);
					rvn_be_select(e, id, R_TRUE);
				}
				rvn_widget_invalidate_paint(widget);
				if (e->context_callback) e->context_callback(widget->id, id, e->press_x, e->press_y, e->context_user);
				return R_TRUE;
			} else if (drag == RVN_BE_PRESS_SLOT) {
				rvn_be_block_t* b = rvn_be_block(e, e->press_block);
				const rvn_be_kind_t* k = rvn_be_kind_of(e, b);
				e->drag = RVN_BE_NONE;
				if (b && !b->sel && !add) {
					rvn_be_clear_selection(e);
					rvn_be_select(e, b->id, R_TRUE);
				}
				if (k && e->press_slot >= 0 && e->press_slot < k->slot_count && k->slot_types[e->press_slot] == 'm')
					rvn_be_open_menu(e, e->press_block, e->press_slot);
				else
					rvn_be_open_field(e, e->press_block, e->press_slot);
			} else if (drag == RVN_BE_PRESS) {
				/* a click : selects (Ctrl / Shift : adds or removes), and the application may run the script (Scratch) */
				rvn_be_block_t* b = rvn_be_block(e, e->press_block);
				rvn_be_block_t* top = rvn_be_top_of(e, b);
				e->drag = RVN_BE_NONE;
				if (b && add) {
					rvn_be_select(e, b->id, !b->sel);
				} else if (b) {
					rvn_be_clear_selection(e);
					rvn_be_select(e, b->id, R_TRUE);
				}
				if (top && e->clicked_callback) e->clicked_callback(widget->id, top->id, e->clicked_user);
			} else {
				e->drag = RVN_BE_NONE;
			}
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		}
		case R_EVENT_KEY_DOWN:
			if (event->target != widget->id) return R_FALSE;
			if (event->key == R_KEY_ESCAPE) {
				if (e->drag == RVN_BE_BLOCKS) {
					rvn_be_end_carry(e, R_TRUE);
					return R_TRUE;
				}
				if (rvn_be_selection_count(e) > 0) {
					rvn_be_clear_selection(e);
					rvn_widget_invalidate_paint(widget);
					return R_TRUE;
				}
				return R_FALSE;
			}
			if ((event->key == R_KEY_DELETE || event->key == R_KEY_BACKSPACE) && e->drag == RVN_BE_NONE && rvn_be_selection_count(e) > 0) {
				/* the selected blocks and what is under them */
				rvn_be_delete_selection(e);
				rvn_be_notify(e, 0);
				return R_TRUE;
			}
			if (event->key == R_KEY_A && (event->modifiers & R_MOD_CTRL) && e->drag == RVN_BE_NONE) {
				for (i = 0; i < e->block_count; i++)
					if (!e->blocks[i].palette && !e->blocks[i].dying) e->blocks[i].sel = R_TRUE;
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			if (event->key == R_KEY_HOME) {
				e->pan_x = -20.0f;
				e->pan_y = -20.0f;
				e->zoom = 1.0f;
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			return R_FALSE;
		default:
			return R_FALSE;
	}
}

static int rvn_block_editor_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_block_editor_t* e = (rvn_block_editor_t*)widget;
	float number;
	int flag;
	if (RVN_ATTR("Zoom")) {
		if (!rvn_parse_float(value, &number) || number <= 0.0f) return R_FALSE;
		e->zoom = rvn_clampf(number, 0.3f, 3.0f);
		rvn_spring_set(&e->shown_zoom, e->zoom);
	} else if (RVN_ATTR("Palette")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		e->palette_hidden = !flag;
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_paint(widget);
	return R_TRUE;
}

/* <Kind Name="move" Label="move %n{10} steps" Shape="Statement" Category="Motion"/>, <Category Name="Motion" Color="#4C97FF"/> */
static int rvn_block_editor_xml_child(rvn_widget_t* widget, const struct rvn_xml_node_t* element);

/* A finger on the empty canvas (or the palette) moves it; on a block, it drags the block. */
static int rvn_block_editor_pan(rvn_widget_t* widget, float dx, float dy, int coasting) {
	rvn_block_editor_t* e = (rvn_block_editor_t*)widget;
	float zoom = e->zoom > 0.0f ? e->zoom : 1.0f;
	RVN_UNUSED(coasting);
	if (e->drag != RVN_BE_NONE || (dx == 0.0f && dy == 0.0f)) return R_FALSE;
	if (e->touch_palette) {
		float most = rvn_maxf(0.0f, e->palette_height - widget->rect.h), before = e->palette_scroll;
		e->palette_scroll = rvn_clampf(e->palette_scroll - dy, 0.0f, most);
		return e->palette_scroll != before;
	}
	e->pan_x -= dx / zoom;
	e->pan_y -= dy / zoom;
	return R_TRUE;
}

static int rvn_block_editor_zoom(rvn_widget_t* widget, float factor, float x, float y) {
	rvn_block_editor_t* e = (rvn_block_editor_t*)widget;
	float before_x, before_y, after_x, after_y, before = e->zoom;
	if (e->drag != RVN_BE_NONE || rvn_be_in_palette(e, x)) return R_FALSE;
	rvn_be_to_canvas(e, x, y, &before_x, &before_y);
	e->zoom = rvn_clampf((e->zoom > 0.0f ? e->zoom : 1.0f) * factor, 0.3f, 3.0f);
	rvn_spring_set(&e->shown_zoom, e->zoom);
	rvn_be_to_canvas(e, x, y, &after_x, &after_y);
	e->pan_x += before_x - after_x;
	e->pan_y += before_y - after_y;
	return e->zoom != before;
}

static int rvn_block_editor_touch_drags(rvn_widget_t* widget, float x, float y) {
	rvn_block_editor_t* e = (rvn_block_editor_t*)widget;
	int slot;
	e->touch_palette = rvn_be_in_palette(e, x);
	return rvn_be_block_at(e, x, y, &slot) != 0;
}

const rvn_widget_class_t rvn_class_block_editor = {
	.type = R_WIDGET_BLOCK_EDITOR,
	.name = "BlockEditor",
	.size = sizeof(rvn_block_editor_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_CLIP | RVN_CF_NO_MIRROR,
	.init = rvn_block_editor_init,
	.destroy = rvn_block_editor_destroy,
	.measure = rvn_block_editor_measure,
	.arrange = rvn_block_editor_arrange,
	.paint = rvn_block_editor_paint,
	.event = rvn_block_editor_event,
	.set_attribute = rvn_block_editor_set_attribute,
	.xml_child = rvn_block_editor_xml_child,
	.pan = rvn_block_editor_pan,
	.zoom = rvn_block_editor_zoom,
	.touch_drags = rvn_block_editor_touch_drags
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

#define RVN_BLOCK_EDITOR(variable, id, result) \
	rvn_block_editor_t* variable = (rvn_block_editor_t*)rvn_widget_check((id), R_WIDGET_BLOCK_EDITOR, __func__); \
	if (!variable) return result

#define RVN_BLOCK_EDITOR_VOID(variable, id) \
	rvn_block_editor_t* variable = (rvn_block_editor_t*)rvn_widget_check((id), R_WIDGET_BLOCK_EDITOR, __func__); \
	if (!variable) return

static rvn_be_block_t* rvn_be_block_checked(rvn_block_editor_t* e, int id, const char* function) {
	rvn_be_block_t* b = rvn_be_block(e, id);
	if (!b || b->palette || b->dying) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: the block editor has no block %d", function, id);
		return NULL;
	}
	return b;
}

Rid Rblock_editor_create(void) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_BLOCK_EDITOR);
	return widget ? widget->id : R_INVALID_ID;
}

int Rblock_editor_add_kind(Rid id, const char* name, const char* label, RBlockShape shape, const char* category) {
	rvn_be_kind_t* k;
	uint32_t i;
	int index;
	RVN_BLOCK_EDITOR(e, id, -1);
	if (!name || !name[0]) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rblock_editor_add_kind: a kind needs a name");
		return -1;
	}
	if (rvn_be_find_kind(e, name) >= 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rblock_editor_add_kind: the kind \"%s\" already exists", name);
		return -1;
	}
	RVN_ARRAY_RESERVE(e->kinds, e->kind_cap, e->kind_count + 1);
	index = (int)e->kind_count;
	k = &e->kinds[e->kind_count++];
	memset(k, 0, sizeof(*k));
	k->name = rvn_strdup(name);
	k->label = rvn_strdup(label ? label : name);
	k->category = rvn_strdup(category ? category : "");
	k->shape = (shape >= R_BLOCK_STATEMENT && shape <= R_BLOCK_BOOLEAN) ? shape : R_BLOCK_STATEMENT;
	rvn_be_parse_label(k);
	/* its block in the palette, after the others of its category */
	{
		rvn_be_block_t* p = rvn_be_new_block(e, index, R_TRUE);
		rvn_be_block_t copy = *p;
		uint32_t at = e->block_count - 1;
		for (i = 0; i + 1 < e->block_count; i++) {
			const rvn_be_block_t* o = &e->blocks[i];
			if (o->palette && strcmp(e->kinds[o->kind].category, k->category) == 0) at = i + 1;
		}
		if (at < e->block_count - 1) {
			memmove(&e->blocks[at + 1], &e->blocks[at], (e->block_count - 1 - at) * sizeof(rvn_be_block_t));
			e->blocks[at] = copy;
		}
	}
	rvn_be_changed(e);
	return index;
}

void Rblock_editor_set_category_color(Rid id, const char* category, float r, float g, float b) {
	uint32_t i;
	RVN_BLOCK_EDITOR_VOID(e, id);
	for (i = 0; i < e->category_count; i++)
		if (strcmp(e->categories[i].name, category ? category : "") == 0) break;
	if (i == e->category_count) {
		RVN_ARRAY_RESERVE(e->categories, e->category_cap, e->category_count + 1);
		memset(&e->categories[i], 0, sizeof(e->categories[i]));
		e->categories[i].name = rvn_strdup(category ? category : "");
		e->category_count++;
	}
	e->categories[i].color = rvn_rgba(r, g, b, 1.0f);
	e->categories[i].has_color = R_TRUE;
	rvn_widget_invalidate_paint(&e->base);
}

void Rblock_editor_set_palette_visible(Rid id, int visible) {
	RVN_BLOCK_EDITOR_VOID(e, id);
	e->palette_hidden = !visible;
	rvn_be_changed(e);
}

int Rblock_editor_add_block(Rid id, const char* kind, float x, float y) {
	rvn_be_block_t* b;
	int k;
	RVN_BLOCK_EDITOR(e, id, 0);
	k = rvn_be_find_kind(e, kind);
	if (k < 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rblock_editor_add_block: no kind \"%s\"", kind ? kind : "(null)");
		return 0;
	}
	b = rvn_be_new_block(e, k, R_FALSE);
	b->x = x;
	b->y = y;
	rvn_be_order_add(e, b->id);
	rvn_be_changed(e);
	return b->id;
}

void Rblock_editor_remove_block(Rid id, int block) {
	RVN_BLOCK_EDITOR_VOID(e, id);
	if (!rvn_be_block_checked(e, block, __func__)) return;
	rvn_be_delete_script(e, block, R_FALSE);
	rvn_be_purge(e, R_FALSE);
}

void Rblock_editor_clear(Rid id) {
	RVN_BLOCK_EDITOR_VOID(e, id);
	rvn_be_close_field(e);
	e->order_count = 0;
	e->drawn_count = 0;
	e->drag = RVN_BE_NONE;
	e->carried = e->selected = e->hot_block = 0;
	rvn_be_purge(e, R_TRUE);
	rvn_be_changed(e);
}

int Rblock_editor_attach(Rid id, int block, int target, RBlockPlace place, int index) {
	rvn_be_block_t *b, *t;
	RVN_BLOCK_EDITOR(e, id, R_FALSE);
	b = rvn_be_block_checked(e, block, __func__);
	t = b ? rvn_be_block_checked(e, target, __func__) : NULL;
	if (!b || !t) return R_FALSE;
	return rvn_be_attach(e, b, t, place, index);
}

void Rblock_editor_detach(Rid id, int block, float x, float y) {
	rvn_be_block_t* b;
	RVN_BLOCK_EDITOR_VOID(e, id);
	b = rvn_be_block_checked(e, block, __func__);
	if (!b) return;
	rvn_be_make_top(e, b, x, y);
	rvn_be_changed(e);
}

int Rblock_editor_get_script_count(Rid id) {
	RVN_BLOCK_EDITOR(e, id, 0);
	return (int)e->order_count;
}

int Rblock_editor_get_script(Rid id, int index) {
	RVN_BLOCK_EDITOR(e, id, 0);
	return index >= 0 && (uint32_t)index < e->order_count ? e->order[index] : 0;
}

const char* Rblock_editor_get_kind(Rid id, int block) {
	rvn_be_block_t* b;
	RVN_BLOCK_EDITOR(e, id, NULL);
	b = rvn_be_block_checked(e, block, __func__);
	return b ? e->kinds[b->kind].name : NULL;
}

int Rblock_editor_get_next(Rid id, int block) {
	rvn_be_block_t* b;
	RVN_BLOCK_EDITOR(e, id, 0);
	b = rvn_be_block_checked(e, block, __func__);
	return b ? b->next : 0;
}

int Rblock_editor_get_body(Rid id, int block, int mouth) {
	rvn_be_block_t* b;
	RVN_BLOCK_EDITOR(e, id, 0);
	b = rvn_be_block_checked(e, block, __func__);
	return b && mouth >= 0 && mouth < RVN_BE_MAX_MOUTHS ? b->body[mouth] : 0;
}

int Rblock_editor_get_input(Rid id, int block, int slot) {
	rvn_be_block_t* b;
	RVN_BLOCK_EDITOR(e, id, 0);
	b = rvn_be_block_checked(e, block, __func__);
	return b && slot >= 0 && slot < RVN_BE_MAX_SLOTS ? b->input[slot] : 0;
}

int Rblock_editor_get_slot_count(Rid id, int block) {
	rvn_be_block_t* b;
	RVN_BLOCK_EDITOR(e, id, 0);
	b = rvn_be_block_checked(e, block, __func__);
	return b ? e->kinds[b->kind].slot_count : 0;
}

int Rblock_editor_get_mouth_count(Rid id, int block) {
	rvn_be_block_t* b;
	RVN_BLOCK_EDITOR(e, id, 0);
	b = rvn_be_block_checked(e, block, __func__);
	return b && e->kinds[b->kind].shape == R_BLOCK_C ? e->kinds[b->kind].sections : 0;
}

const char* Rblock_editor_get_value(Rid id, int block, int slot) {
	rvn_be_block_t* b;
	RVN_BLOCK_EDITOR(e, id, NULL);
	b = rvn_be_block_checked(e, block, __func__);
	return b && slot >= 0 && slot < e->kinds[b->kind].slot_count ? (b->value[slot] ? b->value[slot] : "") : NULL;
}

void Rblock_editor_set_value(Rid id, int block, int slot, const char* value) {
	rvn_be_block_t* b;
	RVN_BLOCK_EDITOR_VOID(e, id);
	b = rvn_be_block_checked(e, block, __func__);
	if (!b || slot < 0 || slot >= e->kinds[b->kind].slot_count) return;
	rvn_str_set(&b->value[slot], value ? value : "");
	rvn_be_changed(e);
}

int Rblock_editor_get_parent(Rid id, int block, RBlockPlace* place, int* index) {
	rvn_be_block_t* b;
	RVN_BLOCK_EDITOR(e, id, 0);
	b = rvn_be_block_checked(e, block, __func__);
	if (!b) return 0;
	if (place) *place = b->place;
	if (index) *index = b->index;
	return b->parent;
}

void Rblock_editor_get_position(Rid id, int block, float* x, float* y) {
	rvn_be_block_t* b;
	RVN_BLOCK_EDITOR_VOID(e, id);
	b = rvn_be_block_checked(e, block, __func__);
	if (x) *x = b ? (b->parent ? b->lx : b->x) : 0.0f;
	if (y) *y = b ? (b->parent ? b->ly : b->y) : 0.0f;
}

void Rblock_editor_set_block_glow(Rid id, int block, int glow) {
	rvn_be_block_t* b;
	RVN_BLOCK_EDITOR_VOID(e, id);
	b = rvn_be_block_checked(e, block, __func__);
	if (!b || b->glow == (glow != 0)) return;
	b->glow = glow != 0;
	rvn_widget_invalidate_paint(&e->base);
}

void Rblock_editor_set_changed_callback(Rid id, RBlockEditorCallback callback, void* user_data) {
	RVN_BLOCK_EDITOR_VOID(e, id);
	e->changed_callback = callback;
	e->changed_user = user_data;
}

void Rblock_editor_set_clicked_callback(Rid id, RBlockEditorCallback callback, void* user_data) {
	RVN_BLOCK_EDITOR_VOID(e, id);
	e->clicked_callback = callback;
	e->clicked_user = user_data;
}

void Rblock_editor_set_context_callback(Rid id, RBlockEditorContextCallback callback, void* user_data) {
	RVN_BLOCK_EDITOR_VOID(e, id);
	e->context_callback = callback;
	e->context_user = user_data;
}

static rvn_be_kind_t* rvn_be_kind_named(rvn_block_editor_t* e, const char* name, const char* function) {
	int k = rvn_be_find_kind(e, name);
	if (k < 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: the block editor has no kind \"%s\"", function, name ? name : "");
		return NULL;
	}
	return &e->kinds[k];
}

void Rblock_editor_set_slot_options(Rid id, const char* kind, int slot, const char* options) {
	rvn_be_kind_t* k;
	RVN_BLOCK_EDITOR_VOID(e, id);
	k = rvn_be_kind_named(e, kind, __func__);
	if (!k || slot < 0 || slot >= RVN_BE_MAX_SLOTS) return;
	rvn_str_set(&k->options[slot], options ? options : "");
}

void Rblock_editor_set_kind_visible(Rid id, const char* kind, int visible) {
	rvn_be_kind_t* k;
	RVN_BLOCK_EDITOR_VOID(e, id);
	k = rvn_be_kind_named(e, kind, __func__);
	if (!k || k->hidden == !visible) return;
	k->hidden = !visible;
	rvn_be_changed(e);
}

int Rblock_editor_set_kind_label(Rid id, const char* kind, const char* label) {
	rvn_be_kind_t* k;
	uint32_t i;
	int s, index;
	RVN_BLOCK_EDITOR(e, id, R_FALSE);
	k = rvn_be_kind_named(e, kind, __func__);
	if (!k) return R_FALSE;
	if (k->label && label && strcmp(k->label, label) == 0) return R_TRUE;
	for (i = 0; i < k->token_count; i++) rvn_free(k->tokens[i].text);
	rvn_free(k->tokens);
	k->tokens = NULL;
	k->token_count = 0;
	for (s = 0; s < RVN_BE_MAX_SLOTS; s++) {
		rvn_free(k->defaults[s]);
		k->defaults[s] = NULL;
	}
	k->slot_count = 0;
	k->sections = 1;
	rvn_str_set(&k->label, label ? label : k->name);
	rvn_be_parse_label(k);
	index = (int)(k - e->kinds);
	/* the blocks of that kind : new slots take their default, the values of the slots gone jump out */
	for (i = 0; i < e->block_count; i++) {
		rvn_be_block_t* b = &e->blocks[i];
		if (b->kind != index) continue;
		for (s = 0; s < RVN_BE_MAX_SLOTS; s++) {
			if (s < k->slot_count) {
				if (!b->value[s] || b->palette) rvn_str_set(&b->value[s], k->defaults[s] ? k->defaults[s] : "");
			} else {
				rvn_free(b->value[s]);
				b->value[s] = NULL;
				if (b->input[s]) {
					rvn_be_block_t* input = rvn_be_block(e, b->input[s]);
					if (input) rvn_be_make_top(e, input, b->lx + b->w + 20.0f, b->ly);
					b = &e->blocks[i];
					b->input[s] = 0;
				}
			}
		}
	}
	rvn_be_changed(e);
	return R_TRUE;
}

void Rblock_editor_set_block_deletable(Rid id, int block, int deletable) {
	rvn_be_block_t* b;
	RVN_BLOCK_EDITOR_VOID(e, id);
	b = rvn_be_block_checked(e, block, __func__);
	if (b) b->locked = !deletable;
}

int Rblock_editor_get_selection_count(Rid id) {
	RVN_BLOCK_EDITOR(e, id, 0);
	return rvn_be_selection_count(e);
}

int Rblock_editor_get_selected(Rid id, int index) {
	uint32_t i;
	int n = 0;
	RVN_BLOCK_EDITOR(e, id, 0);
	for (i = 0; i < e->block_count; i++) {
		const rvn_be_block_t* b = &e->blocks[i];
		if (!b->sel || b->palette || b->dying) continue;
		if (n++ == index) return b->id;
	}
	return 0;
}

void Rblock_editor_select(Rid id, int block, int add) {
	RVN_BLOCK_EDITOR_VOID(e, id);
	if (!add) rvn_be_clear_selection(e);
	if (block > 0) rvn_be_select(e, block, R_TRUE);
	rvn_widget_invalidate_paint(&e->base);
}

void Rblock_editor_get_view(Rid id, float* x, float* y, float* zoom) {
	RVN_BLOCK_EDITOR_VOID(e, id);
	if (x) *x = e->pan_x;
	if (y) *y = e->pan_y;
	if (zoom) *zoom = e->zoom;
}

void Rblock_editor_set_view(Rid id, float x, float y, float zoom) {
	RVN_BLOCK_EDITOR_VOID(e, id);
	e->pan_x = x;
	e->pan_y = y;
	if (zoom > 0.0f) {
		e->zoom = rvn_clampf(zoom, 0.3f, 3.0f);
		rvn_spring_set(&e->shown_zoom, e->zoom);
	}
	rvn_widget_invalidate_paint(&e->base);
}

/* ---------------------------------------------------------------------------
 *  Save / load : (script X Y (block "kind" ("v0" "v1") (in 0 (block ...)) (body 0 (block ...) (block ...))) (block ...))
 * ------------------------------------------------------------------------- */

static void rvn_be_quote(rvn_text_t* out, const char* text) {
	const char* c;
	rvn_text_add(out, "\"");
	for (c = text ? text : ""; *c; c++) {
		if (*c == '"' || *c == '\\') rvn_text_append(out, "\\", 1);
		if (*c == '\n') { rvn_text_add(out, "\\n"); continue; }
		rvn_text_append(out, c, 1);
	}
	rvn_text_add(out, "\"");
}

static void rvn_be_save_block(rvn_block_editor_t* e, rvn_text_t* out, const rvn_be_block_t* b, int depth);

static void rvn_be_save_chain(rvn_block_editor_t* e, rvn_text_t* out, int first, int depth) {
	const rvn_be_block_t* b = rvn_be_block(e, first);
	int guard = 0;
	while (b && guard++ < 100000) {
		if (!b->dying) rvn_be_save_block(e, out, b, depth);
		b = rvn_be_block(e, b->next);
	}
}

static void rvn_be_indent(rvn_text_t* out, int depth) {
	int i;
	rvn_text_add(out, "\n");
	for (i = 0; i < depth; i++) rvn_text_add(out, "  ");
}

static void rvn_be_save_block(rvn_block_editor_t* e, rvn_text_t* out, const rvn_be_block_t* b, int depth) {
	const rvn_be_kind_t* k = &e->kinds[b->kind];
	int s;
	rvn_be_indent(out, depth);
	rvn_text_add(out, "(block ");
	rvn_be_quote(out, k->name);
	rvn_text_add(out, " (");
	for (s = 0; s < k->slot_count; s++) {
		if (s) rvn_text_add(out, " ");
		rvn_be_quote(out, b->value[s]);
	}
	rvn_text_add(out, ")");
	for (s = 0; s < k->slot_count; s++) {
		const rvn_be_block_t* input = rvn_be_block(e, b->input[s]);
		if (!input || input->dying) continue;
		rvn_be_indent(out, depth + 1);
		rvn_text_printf(out, "(in %d", s);
		rvn_be_save_block(e, out, input, depth + 2);
		rvn_text_add(out, ")");
	}
	if (k->shape == R_BLOCK_C)
		for (s = 0; s < k->sections; s++) {
			if (!b->body[s]) continue;
			rvn_be_indent(out, depth + 1);
			rvn_text_printf(out, "(body %d", s);
			rvn_be_save_chain(e, out, b->body[s], depth + 2);
			rvn_text_add(out, ")");
		}
	rvn_text_add(out, ")");
}

const char* Rblock_editor_save(Rid id) {
	uint32_t i;
	RVN_BLOCK_EDITOR(e, id, "");
	e->saved.length = 0;
	if (e->saved.data) e->saved.data[0] = '\0';
	for (i = 0; i < e->order_count; i++) {
		const rvn_be_block_t* top = rvn_be_block(e, e->order[i]);
		if (!top || top->dying || top->id == e->carried) continue;
		rvn_text_printf(&e->saved, "(script %g %g", (double)top->x, (double)top->y);
		rvn_be_save_chain(e, &e->saved, top->id, 1);
		rvn_text_add(&e->saved, ")\n");
	}
	return e->saved.data ? e->saved.data : "";
}

typedef struct rvn_be_reader_t {
	const char* c;
	int ok;
} rvn_be_reader_t;

static void rvn_be_skip(rvn_be_reader_t* r) {
	while (*r->c == ' ' || *r->c == '\t' || *r->c == '\n' || *r->c == '\r') r->c++;
}

static int rvn_be_expect(rvn_be_reader_t* r, char ch) {
	rvn_be_skip(r);
	if (*r->c != ch) { r->ok = R_FALSE; return R_FALSE; }
	r->c++;
	return R_TRUE;
}

static int rvn_be_word(rvn_be_reader_t* r, const char* word) {
	size_t n = strlen(word);
	rvn_be_skip(r);
	if (strncmp(r->c, word, n) != 0) return R_FALSE;
	r->c += n;
	return R_TRUE;
}

static char* rvn_be_string(rvn_be_reader_t* r) {
	rvn_text_t t = { 0 };
	rvn_be_skip(r);
	if (*r->c != '"') { r->ok = R_FALSE; return NULL; }
	r->c++;
	while (*r->c && *r->c != '"') {
		if (*r->c == '\\' && r->c[1]) {
			r->c++;
			if (*r->c == 'n') { rvn_text_add(&t, "\n"); r->c++; continue; }
		}
		rvn_text_append(&t, r->c, 1);
		r->c++;
	}
	if (*r->c != '"') { r->ok = R_FALSE; rvn_free(t.data); return NULL; }
	r->c++;
	return t.data ? t.data : rvn_strdup("");
}

static double rvn_be_number(rvn_be_reader_t* r) {
	char* end = NULL;
	double v;
	rvn_be_skip(r);
	v = strtod(r->c, &end);
	if (end == r->c) { r->ok = R_FALSE; return 0.0; }
	r->c = end;
	return v;
}

/* (block "kind" (values) (in i ...) (body i ...)) : returns the block, 0 on an error. */
static int rvn_be_read_block(rvn_block_editor_t* e, rvn_be_reader_t* r) {
	char* kind_name;
	int kind, id, s = 0;
	rvn_be_block_t* b;
	if (!rvn_be_expect(r, '(') || !rvn_be_word(r, "block")) { r->ok = R_FALSE; return 0; }
	kind_name = rvn_be_string(r);
	if (!r->ok) return 0;
	kind = rvn_be_find_kind(e, kind_name);
	if (kind < 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_WARNING, "Rblock_editor_load: no kind \"%s\"", kind_name);
		rvn_free(kind_name);
		r->ok = R_FALSE;
		return 0;
	}
	rvn_free(kind_name);
	b = rvn_be_new_block(e, kind, R_FALSE);
	id = b->id;
	if (!rvn_be_expect(r, '(')) return 0;
	for (;;) {
		char* value;
		rvn_be_skip(r);
		if (*r->c == ')') { r->c++; break; }
		value = rvn_be_string(r);
		if (!r->ok) return 0;
		b = rvn_be_block(e, id);
		if (s < e->kinds[kind].slot_count) {
			rvn_free(b->value[s]);
			b->value[s] = value;
		} else {
			rvn_free(value);
		}
		s++;
	}
	for (;;) {
		rvn_be_skip(r);
		if (*r->c == ')') { r->c++; return id; }
		if (!rvn_be_expect(r, '(')) return 0;
		if (rvn_be_word(r, "in")) {
			int slot = (int)rvn_be_number(r);
			int child = r->ok ? rvn_be_read_block(e, r) : 0;
			rvn_be_block_t* p = rvn_be_block(e, id);
			rvn_be_block_t* c = rvn_be_block(e, child);
			if (!child || !rvn_be_expect(r, ')')) return 0;
			if (p && c && slot >= 0 && slot < RVN_BE_MAX_SLOTS) {
				p->input[slot] = child;
				c->parent = id;
				c->place = R_BLOCK_INPUT;
				c->index = slot;
			}
		} else if (rvn_be_word(r, "body")) {
			int mouth = (int)rvn_be_number(r), previous = 0;
			for (;;) {
				int child;
				rvn_be_skip(r);
				if (*r->c == ')') { r->c++; break; }
				child = rvn_be_read_block(e, r);
				if (!child) return 0;
				if (mouth >= 0 && mouth < RVN_BE_MAX_MOUTHS) {
					rvn_be_block_t* c = rvn_be_block(e, child);
					if (!previous) {
						rvn_be_block_t* p = rvn_be_block(e, id);
						p->body[mouth] = child;
						c->parent = id;
						c->place = R_BLOCK_BODY;
						c->index = mouth;
					} else {
						rvn_be_block_t* p = rvn_be_block(e, previous);
						p->next = child;
						c->parent = previous;
						c->place = R_BLOCK_NEXT;
						c->index = 0;
					}
				}
				previous = child;
			}
		} else {
			r->ok = R_FALSE;
			return 0;
		}
	}
}

int Rblock_editor_load(Rid id, const char* text) {
	rvn_be_reader_t r;
	RVN_BLOCK_EDITOR(e, id, R_FALSE);
	Rblock_editor_clear(id);
	r.c = text ? text : "";
	r.ok = R_TRUE;
	for (;;) {
		float x, y;
		int previous = 0;
		rvn_be_skip(&r);
		if (!*r.c) break;
		if (!rvn_be_expect(&r, '(') || !rvn_be_word(&r, "script")) { r.ok = R_FALSE; break; }
		x = (float)rvn_be_number(&r);
		y = (float)rvn_be_number(&r);
		if (!r.ok) break;
		for (;;) {
			int block;
			rvn_be_skip(&r);
			if (*r.c == ')') { r.c++; break; }
			block = rvn_be_read_block(e, &r);
			if (!block) break;
			if (!previous) {
				rvn_be_block_t* b = rvn_be_block(e, block);
				b->x = x;
				b->y = y;
				rvn_be_order_add(e, block);
			} else {
				rvn_be_block_t* p = rvn_be_block(e, previous);
				rvn_be_block_t* c = rvn_be_block(e, block);
				p->next = block;
				c->parent = previous;
				c->place = R_BLOCK_NEXT;
				c->index = 0;
			}
			previous = block;
		}
		if (!r.ok) break;
	}
	rvn_be_changed(e);
	if (!r.ok) rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_WARNING, "Rblock_editor_load: the text is not a block program (what was read is kept)");
	return r.ok;
}

/* ---------------------------------------------------------------------------
 *  XML
 * ------------------------------------------------------------------------- */

#include "../xml/xml.h"

static RBlockShape rvn_be_parse_shape(const char* text) {
	if (!text) return R_BLOCK_STATEMENT;
	if (rvn_stricmp(text, "Hat") == 0) return R_BLOCK_HAT;
	if (rvn_stricmp(text, "Cap") == 0) return R_BLOCK_CAP;
	if (rvn_stricmp(text, "C") == 0 || rvn_stricmp(text, "Wrap") == 0) return R_BLOCK_C;
	if (rvn_stricmp(text, "Reporter") == 0 || rvn_stricmp(text, "Value") == 0) return R_BLOCK_REPORTER;
	if (rvn_stricmp(text, "Boolean") == 0 || rvn_stricmp(text, "Condition") == 0) return R_BLOCK_BOOLEAN;
	return R_BLOCK_STATEMENT;
}

static int rvn_block_editor_xml_child(rvn_widget_t* widget, const struct rvn_xml_node_t* element) {
	const char* tag = element->name;
	if (rvn_stricmp(tag, "Kind") == 0 || rvn_stricmp(tag, "Block") == 0) {
		Rblock_editor_add_kind(widget->id, rvn_xml_attribute(element, "Name"), rvn_xml_attribute(element, "Label"),
			rvn_be_parse_shape(rvn_xml_attribute(element, "Shape")), rvn_xml_attribute(element, "Category"));
		return R_TRUE;
	}
	if (rvn_stricmp(tag, "Category") == 0) {
		rvn_color_t color;
		const char* text = rvn_xml_attribute(element, "Color");
		uint32_t i;
		const char* name = rvn_xml_attribute(element, "Name");
		rvn_block_editor_t* e = (rvn_block_editor_t*)widget;
		if (!text || !rvn_parse_color(text, &color)) return R_TRUE;
		for (i = 0; i < e->category_count; i++)
			if (strcmp(e->categories[i].name, name ? name : "") == 0) break;
		if (i == e->category_count) {
			RVN_ARRAY_RESERVE(e->categories, e->category_cap, e->category_count + 1);
			memset(&e->categories[i], 0, sizeof(e->categories[i]));
			e->categories[i].name = rvn_strdup(name ? name : "");
			e->category_count++;
		}
		e->categories[i].color = color;
		e->categories[i].has_color = R_TRUE;
		return R_TRUE;
	}
	return R_FALSE;
}
