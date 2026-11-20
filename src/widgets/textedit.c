/**
 * Raven - widgets/textedit.c
 * Single-line text editing model: buffer, caret, selection, clipboard and
 * painting. TextInput and SpinBox embed one.
 */

#include "common.h"

#define RVN_BULLET "\xE2\x80\xA2" /* U+2022, shown instead of password characters */

void rvn_textedit_init(rvn_textedit_t* edit) {
	memset(edit, 0, sizeof(*edit));
	edit->cap = 32;
	edit->text = (char*)rvn_alloc((size_t)edit->cap);
	edit->text[0] = '\0';
	edit->blink_timer = R_INVALID_ID;
}

void rvn_textedit_free(rvn_textedit_t* edit) {
	if (edit->blink_timer != R_INVALID_ID && Rtimer_is_valid(edit->blink_timer)) Rtimer_destroy(edit->blink_timer);
	rvn_free(edit->text);
	memset(edit, 0, sizeof(*edit));
	edit->blink_timer = R_INVALID_ID;
}

/* ---------------------------------------------------------------------------
 *  Buffer
 * ------------------------------------------------------------------------- */

static int rvn_te_min(const rvn_textedit_t* edit) { return rvn_mini(edit->caret, edit->anchor); }
static int rvn_te_max(const rvn_textedit_t* edit) { return rvn_maxi(edit->caret, edit->anchor); }
static int rvn_te_has_selection(const rvn_textedit_t* edit) { return edit->caret != edit->anchor; }

static void rvn_te_delete(rvn_textedit_t* edit, int from, int to) {
	if (to <= from) return;
	memmove(edit->text + from, edit->text + to, (size_t)(edit->length - to) + 1);
	edit->length -= to - from;
	edit->caret = edit->anchor = from;
}

/* Inserts at the caret, replacing the selection. Returns R_TRUE when the text changed: something was
 * inserted, or the selection was deleted even if nothing then fit in its place. */
static int rvn_te_insert(rvn_textedit_t* edit, const char* text, int length) {
	int deleted = rvn_te_has_selection(edit);
	if (deleted) rvn_te_delete(edit, rvn_te_min(edit), rvn_te_max(edit));
	if (edit->max_chars > 0) {
		/* cut the insertion at a character boundary */
		int room = edit->max_chars - rvn_utf8_count(edit->text, edit->length);
		int fit = 0;
		while (room > 0 && fit < length) {
			fit = rvn_utf8_next(text, length, fit);
			room--;
		}
		length = fit;
	}
	if (length <= 0) return deleted;
	if (edit->length + length + 1 > edit->cap) {
		while (edit->length + length + 1 > edit->cap) edit->cap *= 2;
		edit->text = (char*)rvn_realloc(edit->text, (size_t)edit->cap);
	}
	memmove(edit->text + edit->caret + length, edit->text + edit->caret, (size_t)(edit->length - edit->caret) + 1);
	memcpy(edit->text + edit->caret, text, (size_t)length);
	edit->length += length;
	edit->caret += length;
	edit->anchor = edit->caret;
	return R_TRUE;
}

int rvn_textedit_set_text(rvn_textedit_t* edit, const char* text) {
	int length;
	if (!text) text = "";
	if (strcmp(edit->text, text) == 0) return R_FALSE;
	length = (int)strlen(text);
	if (length + 1 > edit->cap) {
		while (length + 1 > edit->cap) edit->cap *= 2;
		edit->text = (char*)rvn_realloc(edit->text, (size_t)edit->cap);
	}
	/* memmove: the text may be a part of the current one (Rtext_input_get_text(input) + 2);
	 * then it is shorter and the buffer was not reallocated */
	memmove(edit->text, text, (size_t)length + 1);
	edit->length = length;
	edit->caret = edit->anchor = length;
	edit->scroll = 0.0f;
	return R_TRUE;
}

/* Moves an offset back to the start of its UTF-8 character, inside the text. */
static int rvn_te_boundary(const rvn_textedit_t* edit, int offset) {
	offset = rvn_clampi(offset, 0, edit->length);
	while (offset > 0 && offset < edit->length && ((unsigned char)edit->text[offset] & 0xC0) == 0x80) offset--;
	return offset;
}

void rvn_textedit_set_selection(rvn_textedit_t* edit, int start, int end) {
	edit->anchor = rvn_te_boundary(edit, start);
	edit->caret = rvn_te_boundary(edit, end);
	edit->blink_start = rvn_clock_seconds();
}

int rvn_textedit_replace(rvn_textedit_t* edit, int start, int end, const char* text) {
	int from, to, length = text ? (int)strlen(text) : 0;
	from = rvn_te_boundary(edit, rvn_mini(start, end));
	to = rvn_te_boundary(edit, rvn_maxi(start, end));
	edit->anchor = from;
	edit->caret = to;
	edit->blink_start = rvn_clock_seconds();
	if (length == 0) {
		if (to <= from) return R_FALSE;
		rvn_te_delete(edit, from, to);
		return R_TRUE;
	}
	return rvn_te_insert(edit, text, length);
}

void rvn_textedit_select_all(rvn_textedit_t* edit) {
	edit->anchor = 0;
	edit->caret = edit->length;
}

static int rvn_te_is_space(char c) {
	return c == ' ' || c == '\t';
}

static int rvn_te_word_left(const rvn_textedit_t* edit, int offset) {
	while (offset > 0 && rvn_te_is_space(edit->text[offset - 1])) offset--;
	while (offset > 0 && !rvn_te_is_space(edit->text[offset - 1])) offset--;
	return offset;
}

static int rvn_te_word_right(const rvn_textedit_t* edit, int offset) {
	while (offset < edit->length && !rvn_te_is_space(edit->text[offset])) offset++;
	while (offset < edit->length && rvn_te_is_space(edit->text[offset])) offset++;
	return offset;
}

/* ---------------------------------------------------------------------------
 *  Focus and caret blinking
 * ------------------------------------------------------------------------- */

static void rvn_te_blink(Rid timer, void* user_data) {
	rvn_widget_t* widget = rvn_widget_get((Rid)(uintptr_t)user_data);
	RVN_UNUSED(timer);
	if (widget) rvn_widget_invalidate_paint(widget);
}

void rvn_textedit_set_focused(rvn_textedit_t* edit, rvn_widget_t* widget, int focused) {
	if (focused) {
		edit->blink_start = rvn_clock_seconds();
		if (edit->blink_timer == R_INVALID_ID) {
			edit->blink_timer = Rtimer_create(0.5, R_TRUE, rvn_te_blink, (void*)(uintptr_t)widget->id);
		}
	} else {
		if (edit->blink_timer != R_INVALID_ID) {
			if (Rtimer_is_valid(edit->blink_timer)) Rtimer_destroy(edit->blink_timer);
			edit->blink_timer = R_INVALID_ID;
		}
		edit->mouse_selecting = R_FALSE;
		edit->anchor = edit->caret;
		edit->scroll = 0.0f;
	}
	rvn_widget_invalidate_paint(widget);
}

/* ---------------------------------------------------------------------------
 *  Keyboard
 * ------------------------------------------------------------------------- */

static void rvn_te_copy(const rvn_textedit_t* edit) {
	char* copy;
	if (edit->password || !rvn_te_has_selection(edit)) return;
	copy = rvn_strndup(edit->text + rvn_te_min(edit), (size_t)(rvn_te_max(edit) - rvn_te_min(edit)));
	Rclipboard_set_text(copy);
	rvn_free(copy);
}

static int rvn_te_paste(rvn_textedit_t* edit) {
	const char* clip = Rclipboard_get_text();
	char* line;
	size_t i, length;
	int changed;
	if (!clip || !clip[0]) return R_FALSE;
	/* a single-line editor keeps the first line and turns tabs into spaces */
	length = strcspn(clip, "\r\n");
	line = rvn_strndup(clip, length);
	for (i = 0; i < length; i++) {
		if ((unsigned char)line[i] < 32) line[i] = ' ';
	}
	changed = rvn_te_insert(edit, line, (int)length);
	rvn_free(line);
	return changed;
}

int rvn_textedit_key(rvn_textedit_t* edit, const REvent* event) {
	int shift = (event->modifiers & R_MOD_SHIFT) != 0;
	int command = (event->modifiers & (R_MOD_CTRL | R_MOD_SUPER)) != 0;
	int alt = (event->modifiers & R_MOD_ALT) != 0;
	int result = RVN_TEXTEDIT_HANDLED;
	int target;

	if (event->type != R_EVENT_KEY_DOWN) return 0;

	switch (event->key) {
		case R_KEY_LEFT:
			if (!shift && rvn_te_has_selection(edit)) target = rvn_te_min(edit);
			else target = command ? rvn_te_word_left(edit, edit->caret) : rvn_utf8_prev(edit->text, edit->caret);
			edit->caret = target;
			if (!shift) edit->anchor = target;
			break;
		case R_KEY_RIGHT:
			if (!shift && rvn_te_has_selection(edit)) target = rvn_te_max(edit);
			else target = command ? rvn_te_word_right(edit, edit->caret) : rvn_utf8_next(edit->text, edit->length, edit->caret);
			edit->caret = target;
			if (!shift) edit->anchor = target;
			break;
		case R_KEY_HOME:
			edit->caret = 0;
			if (!shift) edit->anchor = 0;
			break;
		case R_KEY_END:
			edit->caret = edit->length;
			if (!shift) edit->anchor = edit->length;
			break;
		case R_KEY_BACKSPACE:
			if (edit->read_only) break;
			if (rvn_te_has_selection(edit)) {
				rvn_te_delete(edit, rvn_te_min(edit), rvn_te_max(edit));
				result |= RVN_TEXTEDIT_CHANGED;
			} else if (edit->caret > 0) {
				rvn_te_delete(edit, command ? rvn_te_word_left(edit, edit->caret) : rvn_utf8_prev(edit->text, edit->caret), edit->caret);
				result |= RVN_TEXTEDIT_CHANGED;
			}
			break;
		case R_KEY_DELETE:
			if (edit->read_only) break;
			if (rvn_te_has_selection(edit)) {
				rvn_te_delete(edit, rvn_te_min(edit), rvn_te_max(edit));
				result |= RVN_TEXTEDIT_CHANGED;
			} else if (edit->caret < edit->length) {
				rvn_te_delete(edit, edit->caret, command ? rvn_te_word_right(edit, edit->caret) : rvn_utf8_next(edit->text, edit->length, edit->caret));
				result |= RVN_TEXTEDIT_CHANGED;
			}
			break;
		case R_KEY_ENTER:
		case R_KEY_KP_ENTER:
			result |= RVN_TEXTEDIT_COMMITTED;
			break;
		case R_KEY_A:
			if (!command) goto printable;
			rvn_textedit_select_all(edit);
			break;
		case R_KEY_C:
			if (!command) goto printable;
			rvn_te_copy(edit);
			break;
		case R_KEY_X:
			if (!command) goto printable;
			rvn_te_copy(edit);
			if (!edit->read_only && !edit->password && rvn_te_has_selection(edit)) {
				rvn_te_delete(edit, rvn_te_min(edit), rvn_te_max(edit));
				result |= RVN_TEXTEDIT_CHANGED;
			}
			break;
		case R_KEY_V:
			if (!command) goto printable;
			if (!edit->read_only && rvn_te_paste(edit)) result |= RVN_TEXTEDIT_CHANGED;
			break;
		default:
		printable:
			/* keys that type a character are consumed so that they never trigger
			 * a window shortcut; the character itself arrives as a text event */
			if (command || alt || (int)event->key >= 256 || event->key == R_KEY_UNKNOWN) return 0;
			return RVN_TEXTEDIT_HANDLED;
	}
	edit->blink_start = rvn_clock_seconds();
	return result;
}

int rvn_textedit_input(rvn_textedit_t* edit, uint32_t codepoint) {
	char encoded[4];
	int length;
	if (edit->read_only || codepoint < 32 || codepoint == 127) return 0;
	length = rvn_utf8_encode(codepoint, encoded);
	edit->blink_start = rvn_clock_seconds();
	return rvn_te_insert(edit, encoded, length) ? (RVN_TEXTEDIT_HANDLED | RVN_TEXTEDIT_CHANGED) : RVN_TEXTEDIT_HANDLED;
}

/* ---------------------------------------------------------------------------
 *  Geometry (password mode shows one bullet per character)
 * ------------------------------------------------------------------------- */

static float rvn_te_text_x(rvn_font_t* font, float size, int password, const char* text, int offset) {
	if (password) {
		return rvn_text_width(font, size, RVN_BULLET, -1) * (float)rvn_utf8_count(text, offset);
	}
	return rvn_text_width(font, size, text, offset);
}

static float rvn_te_offset_to_x(const rvn_textedit_t* edit, rvn_font_t* font, float size, int offset) {
	return rvn_te_text_x(font, size, edit->password, edit->text, offset);
}

static int rvn_te_x_to_offset(const rvn_textedit_t* edit, rvn_font_t* font, float size, float x) {
	if (edit->password) {
		float bullet = rvn_text_width(font, size, RVN_BULLET, -1);
		int count = rvn_utf8_count(edit->text, edit->length);
		int index = bullet > 0.0f ? rvn_clampi((int)(x / bullet + 0.5f), 0, count) : 0;
		int offset = 0;
		while (index-- > 0) offset = rvn_utf8_next(edit->text, edit->length, offset);
		return offset;
	}
	return rvn_text_offset_at(font, size, edit->text, edit->length, x);
}

void rvn_textedit_mouse_down(rvn_textedit_t* edit, const rvn_widget_t* widget, Rid font_id, float size,
	float x, int clicks, int extend) {
	rvn_font_t* font = rvn_font_resolve(font_id);
	int offset;
	if (!font) return;
	size = rvn_widget_font_size(widget, size);
	offset = rvn_te_x_to_offset(edit, font, size, x + edit->scroll);
	if (clicks >= 3) {
		rvn_textedit_select_all(edit);
	} else if (clicks == 2 && !edit->password) {
		edit->anchor = rvn_te_word_left(edit, rvn_mini(offset + 1, edit->length));
		edit->caret = rvn_te_word_right(edit, edit->anchor);
		/* a double click selects the word, not the spaces after it */
		while (edit->caret > edit->anchor && rvn_te_is_space(edit->text[edit->caret - 1])) edit->caret--;
	} else {
		edit->caret = offset;
		if (!extend) edit->anchor = offset;
		edit->mouse_selecting = R_TRUE;
	}
	edit->blink_start = rvn_clock_seconds();
}

int rvn_textedit_offset_at(const rvn_textedit_t* edit, const rvn_widget_t* widget, Rid font_id, float size, float x) {
	rvn_font_t* font = rvn_font_resolve(font_id);
	if (!font) return 0;
	return rvn_te_x_to_offset(edit, font, rvn_widget_font_size(widget, size), x + edit->scroll);
}

void rvn_textedit_mouse_drag(rvn_textedit_t* edit, const rvn_widget_t* widget, Rid font_id, float size, float x) {
	rvn_font_t* font = rvn_font_resolve(font_id);
	if (!font || !edit->mouse_selecting) return;
	size = rvn_widget_font_size(widget, size);
	edit->caret = rvn_te_x_to_offset(edit, font, size, x + edit->scroll);
}

/* ---------------------------------------------------------------------------
 *  Paint
 * ------------------------------------------------------------------------- */

void rvn_text_input_rect(const rvn_widget_t* widget, const RPainter* painter, rvn_rect_t caret) {
	rvn_window_t* window = widget->window;
	if (!window || window->focus != widget || !painter->enabled) return;
	window->text_input_rect = rvn_box_intersect(rvn_paint_snap(painter, caret), painter->clip);
	window->text_input_owner = widget->id;
}

const char* rvn_widget_composition(const rvn_widget_t* widget, int* start, int* end) {
	const rvn_window_t* window = widget->window;
	if (!window || window->focus != widget || !window->composition) return NULL;
	if (start) *start = window->composition_start;
	if (end) *end = window->composition_end;
	return window->composition;
}

void rvn_paint_composition_underline(RPainter* painter, float x0, float x1, float target_x0, float target_x1,
	float baseline, rvn_color_t color) {
	/* the whole composition thin, the part the input method works on thicker (as the systems draw it) */
	if (x1 > x0) rvn_paint_rect(painter, rvn_rect(x0, baseline, x1 - x0, 1.0f), color);
	if (target_x1 > target_x0) rvn_paint_rect(painter, rvn_rect(target_x0, baseline - 0.5f, target_x1 - target_x0, 2.0f), color);
}

void rvn_textedit_paint(rvn_textedit_t* edit, const rvn_widget_t* widget, RPainter* painter,
	rvn_rect_t area, Rid font_id, float size, rvn_color_t color, int focused) {

	rvn_font_t* font = rvn_font_resolve(font_id);
	rvn_font_metrics_t metrics;
	float caret_x, text_width, max_scroll, text_y;
	/* what is shown: the text, or the text with the composition of an input method in place of the selection */
	const char* shown = edit->text;
	const char* composition = NULL;
	char* composed = NULL;
	int shown_length = edit->length, caret_at = edit->caret;
	int mark_from = 0, mark_to = 0, target_from = 0, target_to = 0;

	if (!font) return;
	size = rvn_widget_font_size(widget, size);
	rvn_font_metrics(font, size, &metrics);
	text_y = area.y + (area.h - metrics.line_height) * 0.5f;
	if (focused && !edit->read_only) composition = rvn_widget_composition(widget, &target_from, &target_to);
	if (composition) {
		int from = rvn_te_min(edit), to = rvn_te_max(edit), length = (int)strlen(composition);
		shown_length = edit->length - (to - from) + length;
		composed = (char*)rvn_alloc((size_t)shown_length + 1);
		memcpy(composed, edit->text, (size_t)from);
		memcpy(composed + from, composition, (size_t)length);
		memcpy(composed + from + length, edit->text + to, (size_t)(edit->length - to) + 1);
		shown = composed;
		mark_from = from;
		mark_to = from + length;
		target_from += from;
		target_to += from;
		caret_at = target_to;
	}
	caret_x = rvn_te_text_x(font, size, edit->password, shown, caret_at);
	text_width = rvn_te_text_x(font, size, edit->password, shown, shown_length);

	/* scroll horizontally so that the caret stays visible */
	if (focused) {
		if (caret_x - edit->scroll > area.w - 1.0f) edit->scroll = caret_x - area.w + 1.0f;
		if (caret_x - edit->scroll < 0.0f) edit->scroll = caret_x;
	}
	max_scroll = rvn_maxf(0.0f, text_width - area.w + 1.0f);
	edit->scroll = rvn_clampf(edit->scroll, 0.0f, max_scroll);

	rvn_paint_push_clip(painter, area);
	if (focused && !composition && rvn_te_has_selection(edit)) {
		float x0 = rvn_te_offset_to_x(edit, font, size, rvn_te_min(edit));
		float x1 = rvn_te_offset_to_x(edit, font, size, rvn_te_max(edit));
		rvn_paint_rect(painter, rvn_rect(area.x + x0 - edit->scroll, text_y, x1 - x0, metrics.line_height),
			rvn_widget_color(widget, R_COLOR_SELECTION));
	}
	if (edit->password) {
		int count = rvn_utf8_count(shown, shown_length), i;
		char* bullets = (char*)rvn_alloc((size_t)count * 3 + 1);
		for (i = 0; i < count; i++) memcpy(bullets + i * 3, RVN_BULLET, 3);
		bullets[count * 3] = '\0';
		rvn_paint_text(painter, font_id, size, area.x - edit->scroll, text_y, bullets, count * 3, color);
		rvn_free(bullets);
	} else {
		rvn_paint_text(painter, font_id, size, area.x - edit->scroll, text_y, shown, shown_length, color);
	}
	if (edit->spelling && !edit->password && !composition) {
		/* misspelled words, but not the one being typed at the caret */
		const rvn_spell_line_t* spelling = edit->spelling;
		rvn_color_t wave = rvn_widget_color(widget, R_COLOR_SPELLING);
		int i;
		for (i = 0; i < spelling->count; i++) {
			int from = spelling->ranges[i * 2], to = spelling->ranges[i * 2 + 1];
			if (to > edit->length || (focused && edit->caret >= from && edit->caret <= to)) continue;
			rvn_paint_spelling(painter, area.x - edit->scroll + rvn_te_offset_to_x(edit, font, size, from),
				area.x - edit->scroll + rvn_te_offset_to_x(edit, font, size, to), text_y + metrics.line_height, wave);
		}
	}
	if (composition) {
		float x = area.x - edit->scroll;
		rvn_paint_composition_underline(painter,
			x + rvn_te_text_x(font, size, edit->password, shown, mark_from), x + rvn_te_text_x(font, size, edit->password, shown, mark_to),
			x + rvn_te_text_x(font, size, edit->password, shown, target_from), x + rvn_te_text_x(font, size, edit->password, shown, target_to),
			text_y + metrics.line_height - 2.0f, color);
		rvn_free(composed);
	}
	if (focused && !edit->read_only) {
		rvn_rect_t caret = rvn_rect(area.x + caret_x - edit->scroll, text_y, 1.0f, metrics.line_height);
		rvn_text_input_rect(widget, painter, caret);
		if (g_rvn.test_deterministic || fmod(rvn_clock_seconds() - edit->blink_start, 1.0) < 0.5) rvn_paint_rect(painter, caret, color);
	}
	rvn_paint_pop_clip(painter);
}
