/**
 * Raven - widgets/label.c
 * Label (Slate STextBlock): one or several lines of text, optionally
 * wrapped on the width it receives; or, with Markdown turned on, formatted
 * text (widgets/markdown.c). A selectable label (Rlabel_set_selectable)
 * lets the mouse select its text, as on a web page, and copies it.
 */

#include "common.h"

typedef struct rvn_label_t {
	rvn_widget_t base;
	char* text;
	Rid font;
	float size;
	RTextAlign align;
	int wrap;
	/** R_TRUE once the application chose whether to wrap: Markdown then stops deciding. */
	int wrap_chosen;
	/** Width received at the last arrange, used to wrap; 0 before the first one. */
	float wrap_width;

	int markdown;
	/** The text, parsed and laid out; made when first measured, forgotten when the text changes. */
	rvn_md_t* md;
	/** Link under the pointer, link the button went down on; -1 = none. */
	int hot_link;
	int pressed_link;
	RLabelLinkCallback link_callback;
	void* link_user;
	/** Result of Rlabel_get_plain_text. */
	char* plain;

	/* selection: offsets into the text (into the text shown, with Markdown) */
	int selectable;
	int anchor, caret;
	/** A drag is selecting; by characters (1), words (2) or paragraphs (3). */
	int selecting;
	int unit;
	/** The word or the paragraph a double or triple click started on: the drag extends from it. */
	int unit_from, unit_to;
	/** R_TRUE when the label made itself focusable to be selectable. */
	int made_focusable;
	/** Result of Rlabel_get_selected_text. */
	char* selected;
} rvn_label_t;

static float rvn_label_md_x(const rvn_label_t* label);

static void rvn_label_init(rvn_widget_t* widget) {
	rvn_label_t* label = (rvn_label_t*)widget;
	label->font = R_INVALID_ID;
	label->align = R_TEXT_ALIGN_LEFT;
	label->hot_link = -1;
	label->pressed_link = -1;
}

/* The label takes the mouse when its text can be selected, or when it has links to click. */
static void rvn_label_update_hit(rvn_label_t* label) {
	if (label->selectable || (label->markdown && label->md && rvn_md_link_count(label->md) > 0)) label->base.flags &= ~RVN_WF_NO_HIT;
	else label->base.flags |= RVN_WF_NO_HIT;
	label->base.cursor = label->selectable ? R_CURSOR_IBEAM : (RCursor)0;
}

static void rvn_label_forget(rvn_label_t* label) {
	rvn_md_free(label->md);
	label->md = NULL;
	label->hot_link = -1;
	label->pressed_link = -1;
	/* (the offsets were in the old text) */
	label->anchor = label->caret = 0;
	label->selecting = 0;
	rvn_label_update_hit(label);
}

static void rvn_label_destroy(rvn_widget_t* widget) {
	rvn_label_t* label = (rvn_label_t*)widget;
	rvn_md_free(label->md);
	rvn_free(label->text);
	rvn_free(label->plain);
	rvn_free(label->selected);
}

static int rvn_label_wraps(const rvn_label_t* label) {
	/* formatted text is made of paragraphs: it wraps unless told not to */
	return label->wrap_chosen ? label->wrap : (label->wrap || label->markdown);
}

static rvn_md_t* rvn_label_md(rvn_label_t* label) {
	if (!label->md) {
		label->md = rvn_md_parse(label->text);
		/* a label does not take the mouse, unless it has links to click (or its text can be selected) */
		rvn_label_update_hit(label);
	}
	return label->md;
}

typedef void (*rvn_label_line_fn)(const char* line, int length, float width, int index, void* context);

/**
 * Splits the text into lines (explicit '\n' and, when `wrap_width` is
 * positive, word wrapping) and calls `visit` for each. Returns the number of
 * lines and the width of the widest one.
 */
static int rvn_label_lines(const rvn_label_t* label, rvn_font_t* font, float size, float wrap_width,
	float* max_width, rvn_label_line_fn visit, void* context) {
	const char* text = label->text ? label->text : "";
	int remaining = (int)strlen(text);
	int count = 0;
	float widest = 0.0f;

	do {
		float width = 0.0f;
		int consumed = rvn_text_wrap(font, size, text, remaining, wrap_width, &width);
		int length = consumed;
		/* the line itself excludes the newline and the spaces it was cut on */
		while (length > 0 && (text[length - 1] == '\n' || text[length - 1] == ' ' || text[length - 1] == '\t')) length--;
		if (visit) visit(text, length, width, count, context);
		widest = rvn_maxf(widest, width);
		count++;
		/* a text ending with '\n' has one more, empty, line */
		if (consumed == remaining && consumed > 0 && text[consumed - 1] == '\n') {
			if (visit) visit(text + consumed, 0, 0.0f, count, context);
			count++;
		}
		text += consumed;
		remaining -= consumed;
	} while (remaining > 0);

	if (max_width) *max_width = widest;
	return count;
}

static rvn_vec2_t rvn_label_measure(rvn_widget_t* widget) {
	rvn_label_t* label = (rvn_label_t*)widget;
	rvn_font_t* font = rvn_font_resolve(label->font);
	float size = rvn_widget_font_size(widget, label->size);
	rvn_font_metrics_t metrics;
	float width = 0.0f;
	int lines, wraps = rvn_label_wraps(label);

	if (!font) return rvn_vec2(0.0f, 0.0f);
	if (label->markdown) return rvn_md_layout(rvn_label_md(label), widget, label->font, size, wraps ? label->wrap_width : 0.0f);
	rvn_font_metrics(font, size, &metrics);
	lines = rvn_label_lines(label, font, size, wraps ? label->wrap_width : 0.0f, &width, NULL, NULL);
	/* a wrapped label keeps the width it was given, or it would be re-wrapped
	 * narrower and narrower by a parent that sizes it to its content */
	if (wraps && label->wrap_width > 0.0f && lines > 1) width = rvn_maxf(width, label->wrap_width);
	return rvn_vec2(ceilf(width), ceilf(metrics.line_height * (float)lines));
}

static void rvn_label_arrange(rvn_widget_t* widget) {
	rvn_label_t* label = (rvn_label_t*)widget;
	if (rvn_label_wraps(label) && fabsf(label->wrap_width - widget->rect.w) > 0.01f) {
		label->wrap_width = widget->rect.w;
		/* the height we announced was computed for another width */
		rvn_widget_invalidate_layout(widget);
	}
}

/* Where a line starts across: 0 left, 0.5 centered, 1 right, the other way round from right to left. */
static float rvn_label_align(const rvn_label_t* label, int rtl) {
	float align = label->align == R_TEXT_ALIGN_CENTER ? 0.5f : (label->align == R_TEXT_ALIGN_RIGHT ? 1.0f : 0.0f);
	return rtl ? 1.0f - align : align;
}

static int rvn_label_sel_from(const rvn_label_t* label) {
	return rvn_mini(label->anchor, label->caret);
}

static int rvn_label_sel_to(const rvn_label_t* label) {
	return rvn_maxi(label->anchor, label->caret);
}

typedef struct rvn_label_paint_t {
	rvn_label_t* label;
	RPainter* painter;
	rvn_font_t* font;
	float size;
	float line_height;
	float align;
	rvn_color_t color;
	int from, to;
	rvn_color_t selection;
} rvn_label_paint_t;

static void rvn_label_paint_line(const char* line, int length, float width, int index, void* context) {
	rvn_label_paint_t* paint = (rvn_label_paint_t*)context;
	rvn_label_t* label = paint->label;
	float x = (label->base.rect.w - width) * paint->align;
	float y = paint->line_height * (float)index;
	if (paint->from < paint->to) {
		/* the selection, behind the text */
		int start = (int)(line - label->text);
		int from = rvn_maxi(paint->from, start), to = rvn_mini(paint->to, start + length);
		/* past the end of a line it goes on to the next: a little of its end is shown too */
		int beyond = paint->to > start + length && start + length < (int)strlen(label->text);
		if (from < to || (beyond && paint->from <= start + length)) {
			float x0 = x + rvn_text_width(paint->font, paint->size, line, from - start);
			float x1 = from < to ? x + rvn_text_width(paint->font, paint->size, line, to - start) : x0;
			if (beyond) x1 += floorf(paint->size * 0.3f);
			rvn_paint_rect(paint->painter, rvn_rect(x0, y, x1 - x0, paint->line_height), paint->selection);
		}
	}
	if (length <= 0) return;
	rvn_paint_text(paint->painter, label->font, paint->size, x, y, line, length, paint->color);
}

/* ---- the lines of a plain label, for a point under the mouse ---- */

typedef struct rvn_label_hit_t {
	const char* text;
	float x, y, line_height, label_width, align;
	rvn_font_t* font;
	float size;
	int line;
	int offset;
} rvn_label_hit_t;

static void rvn_label_hit_line(const char* line, int length, float width, int index, void* context) {
	rvn_label_hit_t* hit = (rvn_label_hit_t*)context;
	float x0 = (hit->label_width - width) * hit->align;
	int start = (int)(line - hit->text);
	/* the line under the point (the first or the last one when it is above or below them) */
	if (index != hit->line) return;
	if (hit->x <= x0) hit->offset = start;
	else hit->offset = start + rvn_mini(length, rvn_text_offset_at(hit->font, hit->size, line, length, hit->x - x0));
}

/* Offset of the text nearest to a point of the label. */
static int rvn_label_offset_at(rvn_label_t* label, float x, float y) {
	rvn_widget_t* widget = &label->base;
	rvn_font_t* font = rvn_font_resolve(label->font);
	rvn_font_metrics_t metrics;
	rvn_label_hit_t hit;
	int count;
	if (!font || !label->text) return 0;
	if (label->markdown) {
		float ox = rvn_widget_is_rtl(widget) ? rvn_maxf(0.0f, widget->rect.w - widget->desired.x) - rvn_label_md_x(label) : rvn_label_md_x(label);
		return label->md ? rvn_md_offset_at(label->md, x - ox, y) : 0;
	}
	memset(&hit, 0, sizeof(hit));
	hit.text = label->text;
	hit.font = font;
	hit.size = rvn_widget_font_size(widget, label->size);
	rvn_font_metrics(font, hit.size, &metrics);
	hit.line_height = metrics.line_height;
	hit.label_width = widget->rect.w;
	hit.align = rvn_label_align(label, rvn_widget_is_rtl(widget));
	hit.x = x;
	hit.y = y;
	count = rvn_label_lines(label, font, hit.size, rvn_label_wraps(label) ? widget->rect.w : 0.0f, NULL, NULL, NULL);
	hit.line = rvn_clampi((int)floorf(y / hit.line_height), 0, count - 1);
	if (y < 0.0f) return 0;
	if (y >= hit.line_height * (float)count) return (int)strlen(label->text);
	rvn_label_lines(label, font, hit.size, rvn_label_wraps(label) ? widget->rect.w : 0.0f, NULL, rvn_label_hit_line, &hit);
	return hit.offset;
}

/* The text the label shows, as offsets of the selection count in it. */
static const char* rvn_label_shown(const rvn_label_t* label, int* length) {
	if (label->markdown && label->md) {
		*length = rvn_md_text_length(label->md);
		return rvn_md_text(label->md);
	}
	*length = label->text ? (int)strlen(label->text) : 0;
	return label->text ? label->text : "";
}

static int rvn_label_is_word(unsigned char c) {
	return c >= 0x80 || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/* The word around an offset (a double click); a space or a sign alone when it is on one. */
static void rvn_label_word_at(const rvn_label_t* label, int offset, int* from, int* to) {
	int length;
	const char* text = rvn_label_shown(label, &length);
	int start = rvn_clampi(offset, 0, length), end;
	if (start < length && !rvn_label_is_word((unsigned char)text[start]) && start > 0 && rvn_label_is_word((unsigned char)text[start - 1])) start--;
	if (start >= length) {
		*from = *to = length;
		if (length > 0 && rvn_label_is_word((unsigned char)text[length - 1])) start = length - 1;
		else return;
	}
	if (!rvn_label_is_word((unsigned char)text[start])) {
		*from = start;
		*to = rvn_utf8_next(text, length, start);
		return;
	}
	end = start;
	while (start > 0 && rvn_label_is_word((unsigned char)text[start - 1])) start--;
	while (end < length && rvn_label_is_word((unsigned char)text[end])) end++;
	if (label->markdown && label->md) {
		/* (the paragraphs of Markdown follow each other in the text with nothing between them) */
		int paragraph_from, paragraph_to;
		rvn_md_paragraph_at(label->md, offset, &paragraph_from, &paragraph_to);
		start = rvn_maxi(start, paragraph_from);
		end = rvn_mini(end, paragraph_to);
	}
	*from = start;
	*to = end;
}

/* The paragraph around an offset (a triple click): the line, between two line breaks. */
static void rvn_label_paragraph_at(const rvn_label_t* label, int offset, int* from, int* to) {
	int length;
	const char* text;
	if (label->markdown && label->md) {
		rvn_md_paragraph_at(label->md, offset, from, to);
		return;
	}
	text = rvn_label_shown(label, &length);
	*from = rvn_clampi(offset, 0, length);
	*to = *from;
	while (*from > 0 && text[*from - 1] != '\n') (*from)--;
	while (*to < length && text[*to] != '\n') (*to)++;
}

/* The selection, as it is copied (the marks of Markdown left out); allocated. */
static char* rvn_label_selection_text(const rvn_label_t* label) {
	int from = rvn_label_sel_from(label), to = rvn_label_sel_to(label);
	rvn_text_t out;
	if (from >= to) return rvn_strdup("");
	if (label->markdown) {
		if (!label->md) return rvn_strdup("");
		memset(&out, 0, sizeof(out));
		rvn_md_selected_text(label->md, from, to, &out);
		return out.data;
	}
	return rvn_strndup(label->text + from, (size_t)(to - from));
}

static void rvn_label_copy(const rvn_label_t* label) {
	char* text = rvn_label_selection_text(label);
	if (text && text[0]) Rclipboard_set_text(text);
	rvn_free(text);
}

/* Where formatted text starts across, when it is narrower than the label. */
static float rvn_label_md_x(const rvn_label_t* label) {
	float align = label->align == R_TEXT_ALIGN_CENTER ? 0.5f : (label->align == R_TEXT_ALIGN_RIGHT ? 1.0f : 0.0f);
	return floorf(rvn_maxf(0.0f, label->base.rect.w - label->base.desired.x) * align);
}

static void rvn_label_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_label_t* label = (rvn_label_t*)widget;
	rvn_font_t* font = rvn_font_resolve(label->font);
	rvn_font_metrics_t metrics;
	rvn_label_paint_t paint;
	int clipped;

	if (!font || !label->text || !label->text[0]) return;
	/* text larger than the space it received is cut at the bounds of the label */
	clipped = widget->desired.x > widget->rect.w + 0.5f || widget->desired.y > widget->rect.h + 0.5f;
	if (label->markdown) {
		float ox;
		/* (laid out when it was measured) */
		if (!label->md) return;
		ox = painter->rtl ? rvn_maxf(0.0f, widget->rect.w - widget->desired.x) - rvn_label_md_x(label) : rvn_label_md_x(label);
		if (clipped) rvn_paint_push_clip(painter, rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h));
		if (label->anchor != label->caret) {
			rvn_md_paint_selection(label->md, painter, ox, 0.0f, rvn_label_sel_from(label), rvn_label_sel_to(label), rvn_widget_color(widget, R_COLOR_SELECTION));
		}
		rvn_md_paint(label->md, widget, painter, ox, 0.0f, rvn_widget_text_color(widget, painter),
			painter->enabled ? label->hot_link : -1);
		if (clipped) rvn_paint_pop_clip(painter);
		return;
	}
	paint.label = label;
	paint.painter = painter;
	paint.font = font;
	paint.size = rvn_widget_font_size(widget, label->size);
	rvn_font_metrics(font, paint.size, &metrics);
	paint.line_height = metrics.line_height;
	paint.align = rvn_label_align(label, painter->rtl);
	paint.color = rvn_widget_text_color(widget, painter);
	paint.from = rvn_label_sel_from(label);
	paint.to = rvn_label_sel_to(label);
	paint.selection = rvn_widget_color(widget, R_COLOR_SELECTION);

	if (clipped) rvn_paint_push_clip(painter, rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h));
	rvn_label_lines(label, font, paint.size, rvn_label_wraps(label) ? widget->rect.w : 0.0f, NULL, rvn_label_paint_line, &paint);
	if (clipped) rvn_paint_pop_clip(painter);
}

static void rvn_label_set_hot(rvn_widget_t* widget, int link) {
	rvn_label_t* label = (rvn_label_t*)widget;
	if (label->hot_link == link) return;
	label->hot_link = link;
	widget->cursor = link >= 0 ? R_CURSOR_HAND : (label->selectable ? R_CURSOR_IBEAM : (RCursor)0);
	rvn_widget_invalidate_paint(widget);
}

static void rvn_label_follow(rvn_widget_t* widget, int link) {
	rvn_label_t* label = (rvn_label_t*)widget;
	const char* address = rvn_md_link(label->md, link);
	char* url;
	if (!address) return;
	/* (the callback may change the text, and the links with it) */
	url = rvn_strdup(address);
	if (label->link_callback) {
		label->link_callback(widget->id, url, label->link_user);
	} else if (rvn_strnicmp(url, "action:", 7) == 0) {
		if (url[7]) rvn_fire_action(widget->id, url + 7);
	} else if (rvn_strnicmp(url, "http://", 7) == 0 || rvn_strnicmp(url, "https://", 8) == 0 || rvn_strnicmp(url, "mailto:", 7) == 0) {
		Ropen_url(url);
	}
	rvn_free(url);
}

/* A press, a drag: the selection from the word or the paragraph a double or triple click started on. */
static void rvn_label_select_to(rvn_label_t* label, int offset) {
	int from = offset, to = offset;
	if (label->unit == 2) rvn_label_word_at(label, offset, &from, &to);
	else if (label->unit == 3) rvn_label_paragraph_at(label, offset, &from, &to);
	if (label->unit <= 1) {
		label->caret = offset;
	} else if (from < label->unit_from) {
		label->anchor = label->unit_to;
		label->caret = from;
	} else {
		label->anchor = label->unit_from;
		label->caret = rvn_maxi(to, label->unit_to);
	}
	rvn_widget_invalidate_paint(&label->base);
}

static void rvn_label_clear(rvn_label_t* label) {
	if (label->anchor == label->caret && !label->selecting) return;
	label->anchor = label->caret = 0;
	label->selecting = 0;
	rvn_widget_invalidate_paint(&label->base);
}

/* The events of a selectable label: R_TRUE when they were used. */
static int rvn_label_select_event(rvn_widget_t* widget, const REvent* event) {
	rvn_label_t* label = (rvn_label_t*)widget;
	int command = (event->modifiers & (R_MOD_CTRL | R_MOD_SUPER)) != 0;
	int offset, length;
	switch (event->type) {
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			if (label->markdown) rvn_label_md(label);
			offset = rvn_label_offset_at(label, event->local_x, event->local_y);
			label->unit = event->clicks >= 3 ? 3 : (event->clicks == 2 ? 2 : 1);
			if (label->unit == 1) {
				if (!(event->modifiers & R_MOD_SHIFT)) label->anchor = offset;
				label->caret = offset;
			} else {
				if (label->unit == 2) rvn_label_word_at(label, offset, &label->unit_from, &label->unit_to);
				else rvn_label_paragraph_at(label, offset, &label->unit_from, &label->unit_to);
				label->anchor = label->unit_from;
				label->caret = label->unit_to;
			}
			label->selecting = R_TRUE;
			rvn_widget_capture_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_MOVE:
			if (!label->selecting) return R_FALSE;
			rvn_label_select_to(label, rvn_label_offset_at(label, event->local_x, event->local_y));
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (event->button != R_MOUSE_LEFT || !label->selecting) return R_FALSE;
			label->selecting = R_FALSE;
			rvn_widget_release_mouse(widget);
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			if (!command) {
				if (event->key == R_KEY_ESCAPE && label->anchor != label->caret) {
					rvn_label_clear(label);
					return R_TRUE;
				}
				return R_FALSE;
			}
			if (event->key == R_KEY_C && label->anchor != label->caret) {
				rvn_label_copy(label);
				return R_TRUE;
			}
			if (event->key == R_KEY_A) {
				rvn_label_shown(label, &length);
				label->anchor = 0;
				label->caret = length;
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			return R_FALSE;
		case R_EVENT_FOCUS_LOST:
			/* a click elsewhere: as on a web page, the selection goes away */
			if (label->selecting) rvn_widget_release_mouse(widget);
			rvn_label_clear(label);
			return R_FALSE;
		default:
			return R_FALSE;
	}
}

static int rvn_label_event(rvn_widget_t* widget, const REvent* event) {
	rvn_label_t* label = (rvn_label_t*)widget;
	int link;
	if (label->markdown && label->md) {
		switch (event->type) {
			case R_EVENT_MOUSE_MOVE:
				rvn_label_set_hot(widget, label->selecting ? -1 : rvn_md_link_at(label->md, event->local_x - rvn_label_md_x(label), event->local_y));
				break;
			case R_EVENT_MOUSE_LEAVE:
				rvn_label_set_hot(widget, -1);
				break;
			case R_EVENT_MOUSE_DOWN:
				if (event->button != R_MOUSE_LEFT) break;
				link = rvn_md_link_at(label->md, event->local_x - rvn_label_md_x(label), event->local_y);
				label->pressed_link = link;
				/* a link is clicked, not selected (text that is not a link lets the click go to what holds the label) */
				if (link >= 0) {
					if (label->selectable) rvn_label_clear(label);
					return R_TRUE;
				}
				break;
			case R_EVENT_MOUSE_UP:
				if (event->button != R_MOUSE_LEFT) break;
				link = rvn_md_link_at(label->md, event->local_x - rvn_label_md_x(label), event->local_y);
				if (link >= 0 && link == label->pressed_link) {
					label->pressed_link = -1;
					rvn_label_follow(widget, link);
					return R_TRUE;
				}
				label->pressed_link = -1;
				break;
			default:
				break;
		}
	}
	return label->selectable ? rvn_label_select_event(widget, event) : R_FALSE;
}

static void rvn_label_set_markdown(rvn_widget_t* widget, int enabled) {
	rvn_label_t* label = (rvn_label_t*)widget;
	enabled = enabled != 0;
	if (label->markdown == enabled) return;
	label->markdown = enabled;
	rvn_label_forget(label);
	rvn_widget_invalidate_layout(widget);
}

static void rvn_label_set_selectable(rvn_widget_t* widget, int selectable) {
	rvn_label_t* label = (rvn_label_t*)widget;
	selectable = selectable != 0;
	if (label->selectable == selectable) return;
	label->selectable = selectable;
	if (selectable) {
		/* the keyboard comes to it after a click (Ctrl+C, Ctrl+A), not with Tab */
		if (!(widget->flags & RVN_WF_FOCUSABLE)) {
			widget->flags |= RVN_WF_FOCUSABLE | RVN_WF_NO_TAB;
			label->made_focusable = R_TRUE;
		}
	} else {
		if (label->selecting) rvn_widget_release_mouse(widget);
		label->anchor = label->caret = 0;
		label->selecting = 0;
		if (label->made_focusable) {
			if (widget->window && widget->window->focus == widget) rvn_window_set_focus(widget->window, NULL);
			widget->flags &= ~(RVN_WF_FOCUSABLE | RVN_WF_NO_TAB);
			label->made_focusable = R_FALSE;
		}
	}
	rvn_label_update_hit(label);
	rvn_widget_invalidate_paint(widget);
}

static int rvn_label_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_label_t* label = (rvn_label_t*)widget;
	rvn_color_t color;
	int flag;
	if (RVN_ATTR("Text")) {
		if (rvn_str_set(&label->text, value)) rvn_label_forget(label);
	} else if (RVN_ATTR("Font")) {
		Rid font = rvn_xml_font(value);
		if (font == R_INVALID_ID) return R_FALSE;
		label->font = font;
	} else if (RVN_ATTR("FontSize") || RVN_ATTR("TextSize")) {
		if (!rvn_parse_float(value, &label->size)) return R_FALSE;
	} else if (RVN_ATTR("Align") || RVN_ATTR("TextAlign")) {
		if (rvn_stricmp(value, "Left") == 0) label->align = R_TEXT_ALIGN_LEFT;
		else if (rvn_stricmp(value, "Center") == 0) label->align = R_TEXT_ALIGN_CENTER;
		else if (rvn_stricmp(value, "Right") == 0) label->align = R_TEXT_ALIGN_RIGHT;
		else return R_FALSE;
	} else if (RVN_ATTR("Wrap")) {
		if (!rvn_parse_bool(value, &label->wrap)) return R_FALSE;
		label->wrap_chosen = R_TRUE;
	} else if (RVN_ATTR("Markdown")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		rvn_label_set_markdown(widget, flag);
	} else if (RVN_ATTR("Selectable")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		rvn_label_set_selectable(widget, flag);
		return R_TRUE;
	} else if (RVN_ATTR("Color")) {
		if (!rvn_parse_color(value, &color)) return R_FALSE;
		rvn_widget_set_color(widget, R_COLOR_TEXT, color);
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_layout(widget);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_label = {
	.type = R_WIDGET_LABEL,
	.name = "Label",
	.size = sizeof(rvn_label_t),
	.flags = RVN_CF_NO_HIT | RVN_CF_NO_CHILDREN,
	.init = rvn_label_init,
	.destroy = rvn_label_destroy,
	.measure = rvn_label_measure,
	.arrange = rvn_label_arrange,
	.paint = rvn_label_paint,
	.event = rvn_label_event,
	.set_attribute = rvn_label_set_attribute
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

Rid Rlabel_create(const char* text) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_LABEL);
	if (!widget) return R_INVALID_ID;
	((rvn_label_t*)widget)->text = rvn_strdup(text);
	return widget->id;
}

void Rlabel_set_text(Rid id, const char* text) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LABEL);
	if (rvn_str_set(&((rvn_label_t*)widget)->text, text)) {
		rvn_label_forget((rvn_label_t*)widget);
		rvn_widget_invalidate_layout(widget);
	}
}

const char* Rlabel_get_text(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_LABEL, "");
	return ((rvn_label_t*)widget)->text ? ((rvn_label_t*)widget)->text : "";
}

void Rlabel_set_font(Rid id, Rid font) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LABEL);
	if (font != R_INVALID_ID && !Rfont_is_valid(font)) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rlabel_set_font: invalid font");
		return;
	}
	((rvn_label_t*)widget)->font = font;
	rvn_widget_invalidate_layout(widget);
}

void Rlabel_set_text_size(Rid id, float size) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LABEL);
	((rvn_label_t*)widget)->size = size > 0.0f ? size : 0.0f;
	rvn_widget_invalidate_layout(widget);
}

void Rlabel_set_text_align(Rid id, RTextAlign align) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LABEL);
	if (align < R_TEXT_ALIGN_LEFT || align > R_TEXT_ALIGN_RIGHT) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rlabel_set_text_align: invalid alignment");
		return;
	}
	((rvn_label_t*)widget)->align = align;
	rvn_widget_invalidate_paint(widget);
}

void Rlabel_set_wrap(Rid id, int wrap) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LABEL);
	((rvn_label_t*)widget)->wrap = wrap != 0;
	((rvn_label_t*)widget)->wrap_chosen = R_TRUE;
	rvn_widget_invalidate_layout(widget);
}

void Rlabel_set_color(Rid id, float r, float g, float b, float a) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LABEL);
	rvn_widget_set_color(widget, R_COLOR_TEXT, rvn_rgba(r, g, b, a));
}

void Rlabel_set_markdown(Rid id, int enabled) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LABEL);
	rvn_label_set_markdown(widget, enabled);
}

int Rlabel_get_markdown(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_LABEL, R_FALSE);
	return ((rvn_label_t*)widget)->markdown;
}

void Rlabel_set_link_callback(Rid id, RLabelLinkCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LABEL);
	((rvn_label_t*)widget)->link_callback = callback;
	((rvn_label_t*)widget)->link_user = user_data;
}

const char* Rlabel_get_plain_text(Rid id) {
	rvn_label_t* label;
	rvn_text_t out;
	RVN_WIDGET(widget, id, R_WIDGET_LABEL, "");
	label = (rvn_label_t*)widget;
	if (!label->markdown) return label->text ? label->text : "";
	memset(&out, 0, sizeof(out));
	rvn_md_plain_text(rvn_label_md(label), &out);
	rvn_free(label->plain);
	label->plain = out.data;
	return label->plain ? label->plain : "";
}

void Rlabel_set_selectable(Rid id, int selectable) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LABEL);
	rvn_label_set_selectable(widget, selectable);
}

int Rlabel_get_selectable(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_LABEL, R_FALSE);
	return ((rvn_label_t*)widget)->selectable;
}

const char* Rlabel_get_selected_text(Rid id) {
	rvn_label_t* label;
	RVN_WIDGET(widget, id, R_WIDGET_LABEL, "");
	label = (rvn_label_t*)widget;
	if (label->markdown) rvn_label_md(label);
	rvn_free(label->selected);
	label->selected = rvn_label_selection_text(label);
	return label->selected ? label->selected : "";
}

void Rlabel_select_all(Rid id) {
	rvn_label_t* label;
	int length;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LABEL);
	label = (rvn_label_t*)widget;
	if (!label->selectable) return;
	if (label->markdown) rvn_label_md(label);
	rvn_label_shown(label, &length);
	label->anchor = 0;
	label->caret = length;
	rvn_widget_invalidate_paint(widget);
}

void Rlabel_clear_selection(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_LABEL);
	rvn_label_clear((rvn_label_t*)widget);
}
