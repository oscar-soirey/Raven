/**
 * Raven - widgets/timeline.c
 * Timeline: a sequencer. A ruler of time, a playhead, tracks of clips and
 * keys, markers.
 *
 * Layout of the widget:
 *
 *   +-----------+--------------------------------------------+
 *   |           |  ruler: ticks, markers, head of playhead   |
 *   +-----------+--------------------------------------------+---+
 *   | header of |  the track: clips and keys                 | s |
 *   | the track |                                            | c |
 *   +-----------+--------------------------------------------+ r |
 *   |    ...    |  ...                                       | . |
 *   +-----------+--------------------------------------------+---+
 *   |           |  scrollbar of time                         |
 *   +-----------+--------------------------------------------+
 *
 * Times are doubles, in seconds. What is shown is `view_start` and
 * `view_span`; what is drawn glides to them (`shown_start`, `shown_span`), so
 * that zooming is seen happening. Positions asked by the mouse use what is
 * drawn.
 *
 * One gesture at a time (rvn_tl_drag_t): scrubbing the playhead, moving the
 * selected items, stretching a clip by one end, a selection rectangle, or
 * panning. Moves remember where every selected item started, so that they
 * can snap as a whole and be undone with Escape.
 */

#include "common.h"
#include "../xml/xml.h"

/*
 * Sizes for the flat theme, whose text is 13 units high. They are scaled by
 * rvn_tl_unit: the ruler, the tracks and their labels follow the size of the
 * text of the theme and the text scale of the accessibility settings. The
 * application can set the ruler, the header and the tracks itself
 * (Rtimeline_set_ruler_height...).
 */
#define RVN_TL_BASE_FONT 13.0f
#define RVN_TL_RULER 30.0f
#define RVN_TL_TRACK 38.0f
#define RVN_TL_EDGE 7.0f
#define RVN_TL_SNAP_PIXELS 8.0f
#define RVN_TL_KEY 11.0f
#define RVN_TL_TOGGLE 20.0f
#define RVN_TL_FLAG 15.0f
#define RVN_TL_HEADER_MIN 96.0f
#define RVN_TL_HEADER_MAX 240.0f
#define RVN_TL_MIN_TRACK 16.0f

typedef enum rvn_tl_drag_t {
	RVN_TL_DRAG_NONE,
	RVN_TL_DRAG_SCRUB,
	RVN_TL_DRAG_MOVE,
	RVN_TL_DRAG_LEFT,
	RVN_TL_DRAG_RIGHT,
	RVN_TL_DRAG_BOX,
	RVN_TL_DRAG_PAN
} rvn_tl_drag_t;

enum { RVN_TL_PART_NONE, RVN_TL_PART_BODY, RVN_TL_PART_LEFT, RVN_TL_PART_RIGHT };

typedef struct rvn_tl_track_t {
	int id;
	char* name;
	rvn_color_t color;
	int has_color;
	float height;
	int locked;
	int muted;
} rvn_tl_track_t;

typedef struct rvn_tl_item_t {
	int id;
	RTimelineItem type;
	/** Id of its track; 0 for a marker. */
	int track;
	double start;
	double length;
	char* label;
	rvn_color_t color;
	int has_color;
	int selected;
	/* where it was when the gesture started */
	double from_start, from_length;
	int from_track;
	/** How much it stands out: under the pointer, selected, held. */
	RSpring lift;
} rvn_tl_item_t;

typedef struct rvn_timeline_t {
	rvn_widget_t base;
	double duration;
	double time;
	double fps;
	int no_snapping;
	int playing;
	int loop;
	double view_start, view_span;
	RSpring shown_start, shown_span;
	/** What the last paint drew: the mouse works with this. */
	double draw_start, draw_span;
	/** 0: automatic (fits the names of the tracks). */
	float header_width;
	/** Height of the tracks that have none of their own; 0: automatic. */
	float track_height;
	/** 0: automatic. */
	float ruler_height;

	rvn_tl_track_t* tracks;
	uint32_t track_count, track_cap;
	rvn_tl_item_t* items;
	uint32_t item_count, item_cap;
	int next_id;

	rvn_scrollbar_t time_bar;
	rvn_scrollbar_t track_bar;

	rvn_tl_drag_t drag;
	int drag_item;
	int drag_moved;
	float press_x, press_y;
	double press_time;
	/** Track under the pointer when the gesture started (index). */
	int press_row;
	double pan_start;
	float pan_scroll;
	/** Selection rectangle, in the widget. */
	rvn_rect_t box;
	/** Where the move snapped, to show it; negative = nowhere. */
	double guide;
	int hot_item;
	int hot_part;
	/** Toggle of a track header under the pointer: track index * 2 + (0 mute, 1 lock), or -1. */
	int hot_toggle;
	/** Something changed during the gesture: the action fires when it ends. */
	int gesture_changed;

	Rid play_timer;
	double play_clock;
	RSpring playhead_shown;

	RTimelineTimeCallback time_callback;
	void* time_user;
	RTimelineItemCallback changed_callback;
	void* changed_user;
	RTimelineItemCallback selection_callback;
	void* selection_user;
	RTimelineItemCallback activated_callback;
	void* activated_user;
	RTimelineItemCallback removed_callback;
	void* removed_user;
} rvn_timeline_t;

static const uint32_t g_tl_palette[] = { 0x0A84FF, 0xFF9F0A, 0x30D158, 0xBF5AF2, 0xFF375F, 0x64D2FF, 0xFFD60A, 0xAC8E68 };

/* ---------------------------------------------------------------------------
 *  Model
 * ------------------------------------------------------------------------- */

static int rvn_tl_track_index(const rvn_timeline_t* tl, int id) {
	uint32_t i;
	for (i = 0; i < tl->track_count; i++) {
		if (tl->tracks[i].id == id) return (int)i;
	}
	return -1;
}

static rvn_tl_track_t* rvn_tl_track(rvn_timeline_t* tl, int id) {
	int index = id > 0 ? rvn_tl_track_index(tl, id) : -1;
	return index >= 0 ? &tl->tracks[index] : NULL;
}

static rvn_tl_item_t* rvn_tl_item(rvn_timeline_t* tl, int id) {
	uint32_t i;
	if (id <= 0) return NULL;
	for (i = 0; i < tl->item_count; i++) {
		if (tl->items[i].id == id) return &tl->items[i];
	}
	return NULL;
}

static rvn_tl_item_t* rvn_tl_add_item(rvn_timeline_t* tl, RTimelineItem type, int track, double start, double length, const char* label) {
	rvn_tl_item_t* item;
	RVN_ARRAY_RESERVE(tl->items, tl->item_cap, tl->item_count + 1);
	item = &tl->items[tl->item_count++];
	memset(item, 0, sizeof(*item));
	item->id = tl->next_id++;
	item->type = type;
	item->track = track;
	item->start = start < 0.0 ? 0.0 : start;
	item->length = length < 0.0 ? 0.0 : length;
	item->label = label && label[0] ? rvn_strdup(label) : NULL;
	rvn_widget_invalidate_paint(&tl->base);
	return item;
}

static void rvn_tl_remove_item_at(rvn_timeline_t* tl, uint32_t index) {
	if (tl->hot_item == tl->items[index].id) tl->hot_item = 0;
	if (tl->drag_item == tl->items[index].id) tl->drag_item = 0;
	rvn_free(tl->items[index].label);
	memmove(&tl->items[index], &tl->items[index + 1], sizeof(rvn_tl_item_t) * (tl->item_count - index - 1));
	tl->item_count--;
}

/* How much bigger than the flat theme the sizes of the timeline are. */
static float rvn_tl_unit(const rvn_timeline_t* tl) {
	float font = rvn_widget_metric(&tl->base, R_METRIC_FONT_SIZE);
	return rvn_maxf(font / RVN_TL_BASE_FONT * rvn_a11y_text_scale(), 0.5f);
}

static float rvn_tl_ruler_height(const rvn_timeline_t* tl) {
	return tl->ruler_height > 0.0f ? tl->ruler_height : roundf(RVN_TL_RULER * rvn_tl_unit(tl));
}

static float rvn_tl_default_track_height(const rvn_timeline_t* tl) {
	return tl->track_height > 0.0f ? tl->track_height : roundf(RVN_TL_TRACK * rvn_tl_unit(tl));
}

static float rvn_tl_track_height(const rvn_timeline_t* tl, const rvn_tl_track_t* track) {
	return track->height > 0.0f ? track->height : rvn_tl_default_track_height(tl);
}

/* Text on a colored clip or flag: the darker (or lighter) of the text and window colors of the theme. */
static rvn_color_t rvn_tl_ink(const rvn_widget_t* widget, int dark) {
	rvn_color_t text = rvn_widget_color(widget, R_COLOR_TEXT), window = rvn_widget_color(widget, R_COLOR_WINDOW);
	float r, g, b, a, text_light, window_light;
	rvn_color_to_floats(text, &r, &g, &b, &a);
	text_light = 0.2126f * r + 0.7152f * g + 0.0722f * b;
	rvn_color_to_floats(window, &r, &g, &b, &a);
	window_light = 0.2126f * r + 0.7152f * g + 0.0722f * b;
	/* (opaque: the window color of a glass theme is see-through) */
	return ((text_light < window_light) == (dark != 0) ? text : window) | 0xFF000000u;
}

static float rvn_tl_tracks_height(const rvn_timeline_t* tl) {
	float height = 0.0f;
	uint32_t i;
	for (i = 0; i < tl->track_count; i++) height += rvn_tl_track_height(tl, &tl->tracks[i]);
	return height;
}

static rvn_color_t rvn_tl_track_color(const rvn_timeline_t* tl, int index) {
	if (index >= 0 && tl->tracks[index].has_color) return tl->tracks[index].color;
	return rvn_hex(g_tl_palette[(uint32_t)(index < 0 ? 0 : index) % RVN_COUNT_OF(g_tl_palette)], 255);
}

static rvn_color_t rvn_tl_item_color(const rvn_timeline_t* tl, const rvn_tl_item_t* item) {
	if (item->has_color) return item->color;
	if (item->type == R_TIMELINE_MARKER) return rvn_widget_color(&tl->base, R_COLOR_WARNING);
	return rvn_tl_track_color(tl, rvn_tl_track_index(tl, item->track));
}

/* ---------------------------------------------------------------------------
 *  Geometry
 * ------------------------------------------------------------------------- */

typedef struct rvn_tl_layout_t {
	/** The tracks, without their headers: where time runs. */
	rvn_rect_t lanes;
	rvn_rect_t ruler;
	rvn_rect_t headers;
	rvn_rect_t time_track;
	rvn_rect_t track_track;
	float bar;
	/* sizes for the theme and the text scale (see rvn_tl_unit) */
	float unit;
	float ruler_h;
	float key;
	float edge;
	float toggle;
	float flag;
} rvn_tl_layout_t;

static void rvn_tl_layout(const rvn_timeline_t* tl, rvn_tl_layout_t* layout) {
	const rvn_widget_t* widget = &tl->base;
	float bar = rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE);
	float header = tl->header_width;
	float unit = rvn_tl_unit(tl);
	float ruler = rvn_tl_ruler_height(tl);
	float lanes_height, lanes_width;
	int scroll;
	layout->unit = unit;
	layout->ruler_h = ruler;
	layout->key = RVN_TL_KEY * unit;
	layout->edge = RVN_TL_EDGE * unit;
	layout->toggle = roundf(RVN_TL_TOGGLE * unit);
	layout->flag = roundf(RVN_TL_FLAG * unit);
	if (header <= 0.0f) {
		/* as wide as the longest name asks, within reason */
		rvn_font_t* font = rvn_font_resolve(R_INVALID_ID);
		float size = rvn_widget_font_size(widget, 0.0f);
		uint32_t i;
		/* the dot before the name, then the two toggles after it */
		float around = 26.0f * unit + layout->toggle * 2.0f + 22.0f * unit;
		header = RVN_TL_HEADER_MIN * unit;
		for (i = 0; i < tl->track_count; i++) {
			if (tl->tracks[i].name) header = rvn_maxf(header, rvn_text_width(font, size, tl->tracks[i].name, -1) + around);
		}
		header = ceilf(rvn_minf(header, RVN_TL_HEADER_MAX * unit));
	}
	header = rvn_minf(header, widget->rect.w * 0.5f);
	lanes_height = rvn_maxf(0.0f, widget->rect.h - ruler - bar);
	scroll = rvn_tl_tracks_height(tl) > lanes_height + 0.5f;
	lanes_width = rvn_maxf(1.0f, widget->rect.w - header - (scroll ? bar : 0.0f));
	layout->bar = bar;
	layout->ruler = rvn_rect(header, 0.0f, lanes_width, ruler);
	layout->lanes = rvn_rect(header, ruler, lanes_width, lanes_height);
	layout->headers = rvn_rect(0.0f, ruler, header, lanes_height);
	layout->time_track = rvn_rect(header, widget->rect.h - bar, lanes_width, bar);
	layout->track_track = rvn_rect(widget->rect.w - bar, ruler, bar, lanes_height);
}

static double rvn_tl_span(const rvn_timeline_t* tl) {
	return tl->draw_span > 1e-9 ? tl->draw_span : (tl->view_span > 1e-9 ? tl->view_span : 1.0);
}

static float rvn_tl_x(const rvn_timeline_t* tl, const rvn_tl_layout_t* layout, double time) {
	return layout->lanes.x + (float)((time - tl->draw_start) / rvn_tl_span(tl)) * layout->lanes.w;
}

static double rvn_tl_time_at(const rvn_timeline_t* tl, const rvn_tl_layout_t* layout, float x) {
	return tl->draw_start + (double)((x - layout->lanes.x) / layout->lanes.w) * rvn_tl_span(tl);
}

static double rvn_tl_per_pixel(const rvn_timeline_t* tl, const rvn_tl_layout_t* layout) {
	return rvn_tl_span(tl) / (double)layout->lanes.w;
}

/* Top of a track in the widget, and its height. */
static float rvn_tl_track_y(const rvn_timeline_t* tl, const rvn_tl_layout_t* layout, int index, float* height) {
	float y = layout->lanes.y - tl->track_bar.offset;
	int i;
	for (i = 0; i < index && i < (int)tl->track_count; i++) y += rvn_tl_track_height(tl, &tl->tracks[i]);
	if (height) *height = index >= 0 && index < (int)tl->track_count ? rvn_tl_track_height(tl, &tl->tracks[index]) : rvn_tl_default_track_height(tl);
	return y;
}

/* Index of the track at a height of the widget; -1 above the first, track_count below the last. */
static int rvn_tl_row_at(const rvn_timeline_t* tl, const rvn_tl_layout_t* layout, float y) {
	float top = layout->lanes.y - tl->track_bar.offset;
	uint32_t i;
	if (y < top) return -1;
	for (i = 0; i < tl->track_count; i++) {
		top += rvn_tl_track_height(tl, &tl->tracks[i]);
		if (y < top) return (int)i;
	}
	return (int)tl->track_count;
}

static void rvn_tl_clamp_view(rvn_timeline_t* tl) {
	double minimum = tl->fps > 0.0 ? 4.0 / tl->fps : 0.05;
	if (tl->duration < minimum) minimum = tl->duration > 0.0 ? tl->duration : 1.0;
	if (tl->view_span > tl->duration) tl->view_span = tl->duration;
	if (tl->view_span < minimum) tl->view_span = minimum;
	if (tl->view_start + tl->view_span > tl->duration) tl->view_start = tl->duration - tl->view_span;
	if (tl->view_start < 0.0) tl->view_start = 0.0;
}

/* The scrollbars follow the view and the tracks. */
static void rvn_tl_sync_bars(rvn_timeline_t* tl) {
	rvn_tl_layout_t layout;
	double per_pixel;
	rvn_tl_layout(tl, &layout);
	per_pixel = tl->view_span / (double)layout.lanes.w;
	tl->time_bar.viewport = layout.lanes.w;
	tl->time_bar.content = (float)(tl->duration / per_pixel);
	tl->time_bar.offset = (float)(tl->view_start / per_pixel);
	tl->track_bar.viewport = layout.lanes.h;
	tl->track_bar.content = rvn_tl_tracks_height(tl);
	rvn_scrollbar_clamp(&tl->track_bar);
}

static void rvn_tl_set_view(rvn_timeline_t* tl, double start, double span, int animate) {
	tl->view_start = start;
	tl->view_span = span;
	rvn_tl_clamp_view(tl);
	if (!animate) {
		rvn_spring_set(&tl->shown_start, (float)tl->view_start);
		rvn_spring_set(&tl->shown_span, (float)tl->view_span);
		tl->shown_start.started = R_TRUE;
		tl->shown_span.started = R_TRUE;
		tl->draw_start = tl->view_start;
		tl->draw_span = tl->view_span;
	}
	rvn_tl_sync_bars(tl);
	rvn_widget_invalidate_paint(&tl->base);
}

/* ---------------------------------------------------------------------------
 *  The ruler
 * ------------------------------------------------------------------------- */

/* The step between two labelled marks of the ruler, and how many small marks it is cut into. */
static double rvn_tl_major_step(const rvn_timeline_t* tl, double per_pixel, int* minor) {
	static const double seconds[] = { 0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 15.0, 30.0, 60.0, 120.0, 300.0, 600.0, 1800.0, 3600.0, 7200.0, 21600.0 };
	static const int seconds_minor[] = { 5, 2, 5, 5, 2, 5, 5, 2, 5, 5, 2, 5, 5, 3, 3, 6, 2, 5, 5, 6, 6, 4, 6 };
	static const int frames[] = { 1, 2, 5, 10 };
	static const int frames_minor[] = { 1, 2, 5, 5 };
	double wanted = per_pixel * 84.0;
	size_t i;
	if (tl->fps > 0.0) {
		for (i = 0; i < RVN_COUNT_OF(frames); i++) {
			double step = (double)frames[i] / tl->fps;
			if (step >= wanted && (double)frames[i] < tl->fps) {
				*minor = frames_minor[i];
				return step;
			}
		}
	}
	for (i = 0; i < RVN_COUNT_OF(seconds); i++) {
		/* (with frames, nothing finer than a second is left here) */
		if (tl->fps > 0.0 && seconds[i] < 1.0) continue;
		if (seconds[i] >= wanted) {
			*minor = seconds_minor[i];
			if (tl->fps > 0.0 && seconds[i] == 1.0) *minor = tl->fps <= 30.0 ? (int)(tl->fps + 0.5) : (tl->fps <= 60.0 ? (int)(tl->fps / 2.0 + 0.5) : 10);
			/* small marks too close to tell apart are not drawn */
			while (*minor > 1 && seconds[i] / (double)*minor < per_pixel * 5.0) *minor = *minor % 2 == 0 ? *minor / 2 : 1;
			return seconds[i];
		}
	}
	*minor = 6;
	return seconds[RVN_COUNT_OF(seconds) - 1];
}

static void rvn_tl_format_time(const rvn_timeline_t* tl, double time, double step, char* buffer, size_t size) {
	if (time < 0.0) time = 0.0;
	if (tl->fps > 0.0) {
		long frame = (long)floor(time * tl->fps + 0.5);
		long per_second = (long)(tl->fps + 0.5);
		long whole = per_second > 0 ? frame / per_second : 0, rest = per_second > 0 ? frame % per_second : 0;
		if (whole >= 3600) snprintf(buffer, size, "%ld:%02ld:%02ld:%02ld", whole / 3600, (whole / 60) % 60, whole % 60, rest);
		else if (whole >= 60) snprintf(buffer, size, "%ld:%02ld:%02ld", whole / 60, whole % 60, rest);
		else snprintf(buffer, size, "%ld:%02ld", whole, rest);
	} else if (step < 1.0) {
		int decimals = step < 0.01 ? 3 : (step < 0.1 ? 2 : 1);
		if (time >= 60.0) snprintf(buffer, size, "%d:%0*.*f", (int)(time / 60.0), decimals + 3, decimals, fmod(time, 60.0));
		else snprintf(buffer, size, "%.*fs", decimals, time);
	} else {
		long whole = (long)floor(time + 0.5);
		if (whole >= 3600) snprintf(buffer, size, "%ld:%02ld:%02ld", whole / 3600, (whole / 60) % 60, whole % 60);
		else if (whole >= 60) snprintf(buffer, size, "%ld:%02ld", whole / 60, whole % 60);
		else snprintf(buffer, size, "%lds", whole);
	}
}

/* ---------------------------------------------------------------------------
 *  Snapping
 * ------------------------------------------------------------------------- */

static double rvn_tl_quantize(const rvn_timeline_t* tl, const rvn_tl_layout_t* layout, double time) {
	if (tl->fps > 0.0) return floor(time * tl->fps + 0.5) / tl->fps;
	{
		int minor = 1;
		double step = rvn_tl_major_step(tl, rvn_tl_per_pixel(tl, layout), &minor) / (double)rvn_maxi(minor, 1);
		return floor(time / step + 0.5) * step;
	}
}

/*
 * What a time is drawn to: the playhead, a marker, an end of a clip that is
 * not moving, the ends of the sequence. Returns R_TRUE and the time when one
 * is within reach.
 */
static int rvn_tl_magnet(const rvn_timeline_t* tl, const rvn_tl_layout_t* layout, double time, int with_playhead, double* found) {
	double reach = rvn_tl_per_pixel(tl, layout) * RVN_TL_SNAP_PIXELS, best = reach;
	int any = R_FALSE;
	uint32_t i;
#define RVN_TL_TRY(candidate) do { double distance = fabs((candidate) - time); if (distance < best) { best = distance; *found = (candidate); any = R_TRUE; } } while (0)
	RVN_TL_TRY(0.0);
	RVN_TL_TRY(tl->duration);
	if (with_playhead) RVN_TL_TRY(tl->time);
	for (i = 0; i < tl->item_count; i++) {
		const rvn_tl_item_t* item = &tl->items[i];
		/* (what moves with the gesture is not something to snap to) */
		if (item->selected && tl->drag != RVN_TL_DRAG_SCRUB) continue;
		RVN_TL_TRY(item->start);
		if (item->type == R_TIMELINE_CLIP) RVN_TL_TRY(item->start + item->length);
	}
#undef RVN_TL_TRY
	return any;
}

/* ---------------------------------------------------------------------------
 *  Notifications
 * ------------------------------------------------------------------------- */

static void rvn_tl_notify_time(rvn_widget_t* widget) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	if (tl->time_callback) tl->time_callback(widget->id, tl->time, tl->time_user);
}

static void rvn_tl_notify_item(rvn_widget_t* widget, RTimelineItemCallback callback, void* user, int item) {
	if (callback && rvn_widget_alive(widget)) callback(widget->id, item, user);
}

static void rvn_tl_set_time(rvn_widget_t* widget, double time, int notify) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	if (time < 0.0) time = 0.0;
	if (time > tl->duration) time = tl->duration;
	if (time == tl->time) return;
	tl->time = time;
	rvn_widget_invalidate_paint(widget);
	if (notify) rvn_tl_notify_time(widget);
}

static void rvn_tl_select_only(rvn_timeline_t* tl, int id) {
	uint32_t i;
	for (i = 0; i < tl->item_count; i++) tl->items[i].selected = tl->items[i].id == id;
}

/* ---------------------------------------------------------------------------
 *  Playing
 * ------------------------------------------------------------------------- */

static void rvn_tl_stop_timer(rvn_timeline_t* tl) {
	if (tl->play_timer != R_INVALID_ID && Rtimer_is_valid(tl->play_timer)) Rtimer_destroy(tl->play_timer);
	tl->play_timer = R_INVALID_ID;
}

static void rvn_tl_play_tick(Rid timer, void* user_data) {
	rvn_widget_t* widget = rvn_widget_get((Rid)(uintptr_t)user_data);
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	double now = rvn_clock_seconds(), time;
	RVN_UNUSED(timer);
	if (!widget || widget->cls->type != R_WIDGET_TIMELINE) return;
	time = tl->time + (now - tl->play_clock);
	tl->play_clock = now;
	if (time >= tl->duration) {
		if (tl->loop && tl->duration > 0.0) {
			time = fmod(time, tl->duration);
		} else {
			time = tl->duration;
			tl->playing = R_FALSE;
			rvn_tl_stop_timer(tl);
		}
	}
	/* the view follows the playhead: a page at a time */
	if (time < tl->view_start || time > tl->view_start + tl->view_span) {
		rvn_tl_set_view(tl, time < tl->view_start ? time : time - tl->view_span * 0.05, tl->view_span, R_TRUE);
	}
	rvn_tl_set_time(widget, time, R_TRUE);
}

static void rvn_tl_set_playing(rvn_widget_t* widget, int playing) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	playing = playing != 0;
	if (tl->playing == playing) return;
	tl->playing = playing;
	rvn_tl_stop_timer(tl);
	if (playing) {
		if (tl->time >= tl->duration) tl->time = 0.0;
		tl->play_clock = rvn_clock_seconds();
		tl->play_timer = Rtimer_create(1.0 / 60.0, R_TRUE, rvn_tl_play_tick, (void*)(uintptr_t)widget->id);
	}
	rvn_widget_invalidate_paint(widget);
}

/* ---------------------------------------------------------------------------
 *  Widget
 * ------------------------------------------------------------------------- */

static void rvn_timeline_init(rvn_widget_t* widget) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	tl->duration = 10.0;
	tl->view_start = 0.0;
	tl->view_span = 10.0;
	tl->draw_span = 10.0;
	tl->next_id = 1;
	tl->guide = -1.0;
	tl->hot_toggle = -1;
	tl->play_timer = R_INVALID_ID;
}

static void rvn_tl_clear(rvn_timeline_t* tl) {
	uint32_t i;
	for (i = 0; i < tl->item_count; i++) rvn_free(tl->items[i].label);
	for (i = 0; i < tl->track_count; i++) rvn_free(tl->tracks[i].name);
	tl->item_count = 0;
	tl->track_count = 0;
	tl->hot_item = 0;
	tl->drag_item = 0;
}

static void rvn_timeline_destroy(rvn_widget_t* widget) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	if (g_rvn.initialized) rvn_tl_stop_timer(tl);
	rvn_tl_clear(tl);
	rvn_free(tl->items);
	rvn_free(tl->tracks);
}

static rvn_vec2_t rvn_timeline_measure(rvn_widget_t* widget) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	float track = rvn_tl_default_track_height(tl);
	float tracks = rvn_clampf(rvn_tl_tracks_height(tl), track * 2.0f, track * 8.0f);
	return rvn_vec2(480.0f * rvn_tl_unit(tl), rvn_tl_ruler_height(tl) + tracks + rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE));
}

static void rvn_timeline_arrange(rvn_widget_t* widget) {
	rvn_tl_sync_bars((rvn_timeline_t*)widget);
}

/* Is a color light enough for dark text on it? */
static int rvn_tl_light(rvn_color_t color) {
	float r, g, b, a;
	rvn_color_to_floats(color, &r, &g, &b, &a);
	return 0.2126f * r + 0.7152f * g + 0.0722f * b > 0.62f;
}

/* Width of the flag of a marker: the paint and the pointer use the same. */
static float rvn_tl_flag_width(const rvn_tl_layout_t* layout, rvn_font_t* font, float size, const rvn_tl_item_t* item) {
	return item->label ? rvn_text_width(font, size, item->label, -1) + 12.0f * layout->unit : 10.0f * layout->unit;
}

static void rvn_tl_paint_diamond(RPainter* painter, float cx, float cy, float half, rvn_color_t color) {
	rvn_paint_triangle(painter, cx - half, cy, cx, cy - half, cx + half, cy, color);
	rvn_paint_triangle(painter, cx - half, cy, cx, cy + half, cx + half, cy, color);
}

static void rvn_timeline_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	rvn_tl_layout_t layout;
	rvn_font_t* font = rvn_font_resolve(R_INVALID_ID);
	float size = rvn_widget_font_size(widget, 0.0f), small = floorf(size * 0.86f);
	rvn_font_metrics_t metrics;
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	rvn_color_t dim = rvn_color_fade(text, 0.55f);
	rvn_color_t line = rvn_widget_color(widget, R_COLOR_SEPARATOR);
	rvn_color_t accent = rvn_widget_color(widget, R_COLOR_ACCENT);
	rvn_color_t back = rvn_widget_color(widget, R_COLOR_INPUT);
	rvn_color_t panel = rvn_widget_color(widget, R_COLOR_HEADER);
	rvn_color_t head = rvn_widget_color(widget, R_COLOR_DANGER);
	rvn_color_t dark_ink = rvn_tl_ink(widget, R_TRUE), light_ink = rvn_tl_ink(widget, R_FALSE);
	float radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
	double per_pixel, major, first;
	int minor = 1, k;
	uint32_t i;
	char label[48];

	rvn_tl_layout(tl, &layout);
	/* the view glides to where it was asked to go */
	tl->draw_start = (double)rvn_paint_spring(painter, &tl->shown_start, (float)tl->view_start, R_MOTION_SNAPPY);
	tl->draw_span = (double)rvn_maxf(rvn_paint_spring(painter, &tl->shown_span, (float)tl->view_span, R_MOTION_SNAPPY), 1e-4f);
	per_pixel = rvn_tl_per_pixel(tl, &layout);
	major = rvn_tl_major_step(tl, per_pixel, &minor);
	rvn_font_metrics(font, small, &metrics);

	/* the whole widget is one rounded surface */
	rvn_paint_box(painter, rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h), radius, back, 1.0f, rvn_widget_color(widget, R_COLOR_BORDER));

	/* ---- the lanes ---- */
	rvn_paint_push_clip(painter, layout.lanes);
	for (i = 0; i < tl->track_count; i++) {
		float height, y = rvn_tl_track_y(tl, &layout, (int)i, &height);
		if (y > layout.lanes.y + layout.lanes.h || y + height < layout.lanes.y) continue;
		if (i % 2 == 1) rvn_paint_rect(painter, rvn_rect(layout.lanes.x, y, layout.lanes.w, height), rvn_color_fade(text, 0.028f));
		rvn_paint_rect(painter, rvn_rect(layout.lanes.x, y + height - 1.0f, layout.lanes.w, 1.0f), rvn_color_fade(line, 0.6f));
	}
	/* the marks of the ruler run down the lanes */
	first = floor(tl->draw_start / major) * major;
	for (k = 0; first + (double)k * major / (double)minor <= tl->draw_start + tl->draw_span; k++) {
		double time = first + (double)k * major / (double)minor;
		float x = floorf(rvn_tl_x(tl, &layout, time));
		if (x < layout.lanes.x) continue;
		rvn_paint_rect(painter, rvn_rect(x, layout.lanes.y, 1.0f, layout.lanes.h), rvn_color_fade(line, k % minor == 0 ? 0.75f : 0.3f));
	}
	/* what is past the end of the sequence */
	{
		float end = rvn_tl_x(tl, &layout, tl->duration);
		if (end < layout.lanes.x + layout.lanes.w) {
			rvn_paint_rect(painter, rvn_rect(end, layout.lanes.y, layout.lanes.x + layout.lanes.w - end, layout.lanes.h), rvn_rgba(0.0f, 0.0f, 0.0f, 0.25f));
		}
	}
	for (i = 0; i < tl->item_count; i++) {
		rvn_tl_item_t* item = &tl->items[i];
		if (item->type != R_TIMELINE_MARKER) continue;
		rvn_paint_rect(painter, rvn_rect(floorf(rvn_tl_x(tl, &layout, item->start)), layout.lanes.y, 1.0f, layout.lanes.h),
			rvn_color_fade(rvn_tl_item_color(tl, item), item->selected ? 0.95f : 0.55f));
	}

	/* clips, then keys over them; what is held is drawn last, over the rest */
	for (k = 0; k < 4; k++) {
		for (i = 0; i < tl->item_count; i++) {
			rvn_tl_item_t* item = &tl->items[i];
			int index = rvn_tl_track_index(tl, item->track);
			int held = item->selected && (tl->drag == RVN_TL_DRAG_MOVE || tl->drag == RVN_TL_DRAG_LEFT || tl->drag == RVN_TL_DRAG_RIGHT) && tl->drag_moved;
			int hot = tl->hot_item == item->id && painter->enabled && tl->drag == RVN_TL_DRAG_NONE;
			float height, y, lift, opacity;
			rvn_color_t color;
			if (item->type == R_TIMELINE_MARKER || index < 0) continue;
			if ((item->type == R_TIMELINE_KEY) != (k % 2 == 1) || held != (k >= 2)) continue;
			y = rvn_tl_track_y(tl, &layout, index, &height);
			if (y > layout.lanes.y + layout.lanes.h || y + height < layout.lanes.y) continue;
			lift = rvn_clampf(rvn_paint_spring(painter, &item->lift, held ? 1.0f : (hot ? 0.45f : 0.0f), R_MOTION_SNAPPY), 0.0f, 1.2f);
			color = rvn_tl_item_color(tl, item);
			opacity = (tl->tracks[index].muted ? 0.38f : 1.0f) * (painter->enabled ? 1.0f : 0.5f);

			if (item->type == R_TIMELINE_CLIP) {
				float x0 = rvn_tl_x(tl, &layout, item->start), x1 = rvn_tl_x(tl, &layout, item->start + item->length);
				float inset = rvn_minf(4.0f * layout.unit, height * 0.2f);
				rvn_rect_t rect = rvn_rect(x0, y + inset - lift * 1.5f, rvn_maxf(x1 - x0, 3.0f), height - inset * 2.0f - 1.0f);
				float clip_radius = rvn_minf(rvn_minf(7.0f * layout.unit, rect.h * 0.5f), rect.w * 0.5f);
				rvn_color_t fill = rvn_color_fade(rvn_color_mix(color, rvn_hex(0x000000, 255), 0.12f), opacity * (item->selected ? 1.0f : 0.86f));
				if (x1 < layout.lanes.x || x0 > layout.lanes.x + layout.lanes.w) continue;
				if (lift > 0.02f || item->selected) {
					rvn_paint_shadow(painter, rvn_rect(rect.x, rect.y + 2.0f + lift * 4.0f, rect.w, rect.h), clip_radius, 6.0f + lift * 12.0f,
						rvn_color_fade(item->selected ? color : rvn_hex(0x000000, 255), (item->selected ? 0.45f : 0.3f) * opacity));
				}
				rvn_paint_box(painter, rect, clip_radius, fill, item->selected ? 1.5f : 1.0f,
					item->selected ? rvn_color_fade(light_ink, 0.92f * opacity) : rvn_color_fade(rvn_color_mix(color, rvn_hex(0xFFFFFF, 255), 0.35f), opacity));
				/* a band of light along the top: the clip is a little raised */
				if (rect.h > 12.0f && rect.w > 8.0f) {
					rvn_paint_box(painter, rvn_rect(rect.x + 2.0f, rect.y + 2.0f, rect.w - 4.0f, rvn_minf(rect.h * 0.42f, 12.0f)), rvn_maxf(clip_radius - 2.0f, 1.0f),
						rvn_rgba(1.0f, 1.0f, 1.0f, 0.09f * opacity), 0.0f, 0);
				}
				if (item->label && rect.w > 26.0f) {
					/* the name stays in sight while the clip starts before what is shown */
					float left = rvn_minf(rvn_maxf(rect.x, layout.lanes.x), rect.x + rect.w - 40.0f);
					rvn_paint_push_clip(painter, rvn_rect(rect.x + 7.0f, rect.y, rect.w - 14.0f, rect.h));
					rvn_paint_text_in(painter, R_INVALID_ID, small, rvn_rect(left + 9.0f, rect.y, rect.w, rect.h), 0.0f, item->label, -1,
						rvn_color_fade(rvn_tl_light(color) ? dark_ink : light_ink, opacity));
					rvn_paint_pop_clip(painter);
				}
				/* the ends that stretch it show under the pointer */
				if (hot && rect.w > 22.0f && !tl->tracks[index].locked) {
					rvn_color_t grip = rvn_rgba(1.0f, 1.0f, 1.0f, tl->hot_part == RVN_TL_PART_BODY ? 0.35f : 0.9f);
					if (tl->hot_part != RVN_TL_PART_RIGHT) rvn_paint_box(painter, rvn_rect(rect.x + 3.0f, rect.y + rect.h * 0.28f, 2.5f, rect.h * 0.44f), 1.25f, grip, 0.0f, 0);
					if (tl->hot_part != RVN_TL_PART_LEFT) rvn_paint_box(painter, rvn_rect(rect.x + rect.w - 5.5f, rect.y + rect.h * 0.28f, 2.5f, rect.h * 0.44f), 1.25f, grip, 0.0f, 0);
				}
			} else {
				float x = rvn_tl_x(tl, &layout, item->start), cy = y + height * 0.5f - lift * 1.5f;
				float half = layout.key * 0.5f + lift * 2.0f + (item->selected ? 1.0f : 0.0f);
				if (x < layout.lanes.x - 12.0f || x > layout.lanes.x + layout.lanes.w + 12.0f) continue;
				rvn_tl_paint_diamond(painter, x, cy + 1.5f, half + 2.0f, rvn_rgba(0.0f, 0.0f, 0.0f, 0.3f * opacity));
				rvn_tl_paint_diamond(painter, x, cy, half + 1.5f, item->selected ? rvn_color_fade(light_ink, opacity) : rvn_color_fade(rvn_color_mix(color, rvn_hex(0x000000, 255), 0.45f), opacity));
				rvn_tl_paint_diamond(painter, x, cy, half - 0.5f, rvn_color_fade(color, opacity));
				rvn_tl_paint_diamond(painter, x, cy - half * 0.28f, half * 0.34f, rvn_rgba(1.0f, 1.0f, 1.0f, 0.4f * opacity));
				if (item->label && item->label[0]) {
					rvn_paint_text_in(painter, R_INVALID_ID, small, rvn_rect(x + half + 6.0f, y, rvn_maxf(layout.lanes.x + layout.lanes.w - (x + half + 6.0f), 0.0f), height), 0.0f, item->label, -1,
						rvn_color_fade(item->selected ? text : dim, opacity));
				}
			}
		}
	}

	if (tl->drag == RVN_TL_DRAG_BOX && tl->drag_moved) {
		rvn_paint_box(painter, tl->box, 3.0f, rvn_color_fade(accent, 0.16f), 1.0f, rvn_color_fade(accent, 0.85f));
	}
	if (tl->guide >= 0.0) {
		rvn_paint_rect(painter, rvn_rect(floorf(rvn_tl_x(tl, &layout, tl->guide)), layout.lanes.y, 1.0f, layout.lanes.h), rvn_color_fade(accent, 0.9f));
	}
	/* the playhead */
	{
		float x = floorf(rvn_tl_x(tl, &layout, tl->time));
		rvn_paint_rect(painter, rvn_rect(x - 1.0f, layout.lanes.y, 3.0f, layout.lanes.h), rvn_color_fade(head, 0.18f));
		rvn_paint_rect(painter, rvn_rect(x, layout.lanes.y, 1.0f, layout.lanes.h), head);
	}
	rvn_paint_pop_clip(painter);

	/* ---- the headers of the tracks ---- */
	{
		/* the column of the headers goes down to the bottom of the widget */
		float corners[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		corners[3] = rvn_maxf(radius - 1.0f, 0.0f);
		rvn_paint_box4(painter, rvn_rect(1.0f, layout.ruler_h, layout.headers.w - 1.0f, widget->rect.h - layout.ruler_h - 1.0f), corners, panel, 0.0f, 0);
	}
	rvn_paint_push_clip(painter, layout.headers);
	for (i = 0; i < tl->track_count; i++) {
		const rvn_tl_track_t* track = &tl->tracks[i];
		float height, y = rvn_tl_track_y(tl, &layout, (int)i, &height);
		rvn_color_t color = rvn_tl_track_color(tl, (int)i);
		float toggle = layout.toggle, right = layout.headers.w - 8.0f * layout.unit;
		float dot = 10.0f * layout.unit;
		int t;
		if (y > layout.headers.y + layout.headers.h || y + height < layout.headers.y) continue;
		rvn_paint_box(painter, rvn_rect(8.0f * layout.unit, y + (height - dot) * 0.5f, dot, dot), dot * 0.5f, rvn_color_fade(color, track->muted ? 0.4f : 1.0f), 0.0f, 0);
		rvn_paint_push_clip(painter, rvn_rect(24.0f * layout.unit, y, rvn_maxf(right - toggle * 2.0f - 32.0f * layout.unit, 0.0f), height));
		rvn_paint_text_in(painter, R_INVALID_ID, size, rvn_rect(26.0f * layout.unit, y, layout.headers.w, height), 0.0f, track->name ? track->name : "", -1,
			track->muted ? dim : text);
		rvn_paint_pop_clip(painter);
		/* mute and lock: two small toggles */
		for (t = 0; t < 2; t++) {
			int on = t == 0 ? track->muted : track->locked;
			int hot = tl->hot_toggle == (int)i * 2 + t && painter->enabled;
			rvn_rect_t rect = rvn_rect(right - toggle * (float)(2 - t) - (t == 0 ? 3.0f * layout.unit : 0.0f), y + (height - toggle) * 0.5f, toggle, toggle);
			rvn_color_t fill = on ? (t == 0 ? rvn_widget_color(widget, R_COLOR_WARNING) : accent) : rvn_color_fade(text, hot ? 0.16f : 0.07f);
			rvn_paint_box(painter, rect, 6.0f * layout.unit, fill, 0.0f, 0);
			rvn_paint_text_in(painter, R_INVALID_ID, small, rect, 0.5f, t == 0 ? "M" : "L", 1, on ? (rvn_tl_light(fill) ? dark_ink : light_ink) : (hot ? text : dim));
		}
		rvn_paint_rect(painter, rvn_rect(0.0f, y + height - 1.0f, layout.headers.w, 1.0f), rvn_color_fade(line, 0.6f));
	}
	rvn_paint_pop_clip(painter);
	rvn_paint_rect(painter, rvn_rect(layout.headers.w - 1.0f, 0.0f, 1.0f, widget->rect.h), line);

	/* ---- the ruler ---- */
	rvn_paint_push_clip(painter, rvn_rect(0.0f, 0.0f, widget->rect.w, layout.ruler_h));
	{
		float corners[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		corners[0] = corners[1] = rvn_maxf(radius - 1.0f, 0.0f);
		rvn_paint_box4(painter, rvn_rect(1.0f, 1.0f, widget->rect.w - 2.0f, layout.ruler_h - 1.0f), corners, panel, 0.0f, 0);
	}
	rvn_paint_rect(painter, rvn_rect(0.0f, layout.ruler_h - 1.0f, widget->rect.w, 1.0f), line);
	/* the time of the playhead, where the headers leave room for it */
	rvn_tl_format_time(tl, tl->time, tl->fps > 0.0 ? 1.0 : 0.01, label, sizeof(label));
	rvn_paint_text_in(painter, g_rvn.mono_font, size, rvn_rect(10.0f * layout.unit, 0.0f, layout.headers.w - 16.0f * layout.unit, layout.ruler_h), 0.0f, label, -1, text);
	rvn_paint_pop_clip(painter);

	rvn_paint_push_clip(painter, layout.ruler);
	for (k = 0; first + (double)k * major / (double)minor <= tl->draw_start + tl->draw_span; k++) {
		double time = first + (double)k * major / (double)minor;
		float x = floorf(rvn_tl_x(tl, &layout, time));
		int labelled = k % minor == 0;
		float tick = (labelled ? 11.0f : 5.0f) * layout.unit;
		if (x < layout.ruler.x - 60.0f * layout.unit) continue;
		rvn_paint_rect(painter, rvn_rect(x, layout.ruler_h - tick, 1.0f, tick), labelled ? dim : rvn_color_fade(dim, 0.6f));
		if (labelled) {
			rvn_tl_format_time(tl, time, major, label, sizeof(label));
			rvn_paint_text(painter, R_INVALID_ID, small, x + 4.0f * layout.unit, 4.0f * layout.unit, label, -1, dim);
		}
	}
	for (i = 0; i < tl->item_count; i++) {
		rvn_tl_item_t* item = &tl->items[i];
		rvn_color_t color;
		float x, width;
		if (item->type != R_TIMELINE_MARKER) continue;
		color = rvn_tl_item_color(tl, item);
		x = floorf(rvn_tl_x(tl, &layout, item->start));
		width = rvn_tl_flag_width(&layout, font, small, item);
		/* a flag on its pole */
		rvn_paint_rect(painter, rvn_rect(x, 3.0f, 1.0f, layout.ruler_h - 3.0f), color);
		rvn_paint_box(painter, rvn_rect(x, 3.0f, width, layout.flag), rvn_minf(4.0f * layout.unit, layout.flag * 0.5f), color, item->selected ? 1.5f : 0.0f, light_ink);
		if (item->label) rvn_paint_text_in(painter, R_INVALID_ID, small, rvn_rect(x + 6.0f * layout.unit, 3.0f, width, layout.flag), 0.0f, item->label, -1,
			rvn_tl_light(color) ? dark_ink : light_ink);
	}
	{
		/* the head of the playhead: a tab that points down */
		float x = floorf(rvn_tl_x(tl, &layout, tl->time)) + 0.5f;
		float u = layout.unit, r = layout.ruler_h;
		float grow = tl->drag == RVN_TL_DRAG_SCRUB ? 1.5f * u : 0.0f;
		rvn_paint_box(painter, rvn_rect(x - 6.0f * u - grow, r - 17.0f * u - grow, 12.0f * u + grow * 2.0f, 11.0f * u + grow), 3.0f * u, head, 0.0f, 0);
		rvn_paint_triangle(painter, x - 6.0f * u - grow, r - 7.0f * u, x + 6.0f * u + grow, r - 7.0f * u, x, r - 0.5f, head);
	}
	rvn_paint_pop_clip(painter);
	RVN_UNUSED(metrics);
}

static void rvn_timeline_paint_overlay(rvn_widget_t* widget, RPainter* painter) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	rvn_tl_layout_t layout;
	rvn_vec2_t origin = rvn_widget_origin(widget);
	float mouse_x = widget->window ? widget->window->mouse_x - origin.x : -1.0f;
	float mouse_y = widget->window ? widget->window->mouse_y - origin.y : -1.0f;
	rvn_tl_layout(tl, &layout);
	rvn_scrollbar_paint(&tl->time_bar, widget, painter, layout.time_track, R_FALSE, mouse_x, mouse_y);
	rvn_scrollbar_paint(&tl->track_bar, widget, painter, layout.track_track, R_TRUE, mouse_x, mouse_y);
	rvn_paint_focus(widget, painter, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS));
}

/* ---------------------------------------------------------------------------
 *  Pointing
 * ------------------------------------------------------------------------- */

/* The item at a point of the widget, and which part of it. */
static int rvn_tl_hit(const rvn_timeline_t* tl, const rvn_tl_layout_t* layout, float x, float y, int* part) {
	uint32_t i;
	int pass;
	*part = RVN_TL_PART_NONE;
	if (y < layout->ruler_h) {
		rvn_font_t* font = rvn_font_resolve(R_INVALID_ID);
		float small = floorf(rvn_widget_font_size(&tl->base, 0.0f) * 0.86f);
		if (x < layout->ruler.x) return 0;
		for (i = tl->item_count; i > 0; i--) {
			const rvn_tl_item_t* item = &tl->items[i - 1];
			float mx, width;
			if (item->type != R_TIMELINE_MARKER) continue;
			mx = rvn_tl_x(tl, layout, item->start);
			width = rvn_tl_flag_width(layout, font, small, item);
			/* the flag as it is painted, and a little around it */
			if (x >= mx - 3.0f && x <= mx + width && y >= 2.0f && y <= 4.0f + layout->flag) {
				*part = RVN_TL_PART_BODY;
				return item->id;
			}
		}
		return 0;
	}
	if (!rvn_rect_contains(layout->lanes, x, y)) return 0;
	/* keys are over the clips, and what was added last is over the rest */
	for (pass = 0; pass < 2; pass++) {
		for (i = tl->item_count; i > 0; i--) {
			const rvn_tl_item_t* item = &tl->items[i - 1];
			int index = rvn_tl_track_index(tl, item->track);
			float height, top;
			if (index < 0 || item->type == R_TIMELINE_MARKER || (item->type == R_TIMELINE_KEY) != (pass == 0)) continue;
			top = rvn_tl_track_y(tl, layout, index, &height);
			if (y < top || y >= top + height) continue;
			if (item->type == R_TIMELINE_KEY) {
				if (fabsf(x - rvn_tl_x(tl, layout, item->start)) <= layout->key * 0.5f + 3.0f) {
					*part = RVN_TL_PART_BODY;
					return item->id;
				}
			} else {
				float x0 = rvn_tl_x(tl, layout, item->start), x1 = rvn_maxf(rvn_tl_x(tl, layout, item->start + item->length), x0 + 3.0f);
				if (x >= x0 && x <= x1) {
					/* a narrow clip is all body: it can always be moved */
					float edge = x1 - x0 > layout->edge * 3.0f ? layout->edge : 0.0f;
					*part = x < x0 + edge ? RVN_TL_PART_LEFT : (x > x1 - edge ? RVN_TL_PART_RIGHT : RVN_TL_PART_BODY);
					return item->id;
				}
			}
		}
	}
	return 0;
}

static int rvn_tl_toggle_at(const rvn_timeline_t* tl, const rvn_tl_layout_t* layout, float x, float y) {
	float toggle = layout->toggle, right = layout->headers.w - 8.0f * layout->unit, height, top;
	int row;
	if (!rvn_rect_contains(layout->headers, x, y)) return -1;
	row = rvn_tl_row_at(tl, layout, y);
	if (row < 0 || row >= (int)tl->track_count) return -1;
	top = rvn_tl_track_y(tl, layout, row, &height);
	if (y < top + (height - toggle) * 0.5f || y > top + (height + toggle) * 0.5f) return -1;
	if (x >= right - toggle * 2.0f - 3.0f * layout->unit && x < right - toggle - 3.0f * layout->unit) return row * 2;
	if (x >= right - toggle && x < right) return row * 2 + 1;
	return -1;
}

static int rvn_tl_item_locked(rvn_timeline_t* tl, const rvn_tl_item_t* item) {
	const rvn_tl_track_t* track = rvn_tl_track(tl, item->track);
	return track && track->locked;
}

static void rvn_tl_update_hot(rvn_widget_t* widget, float x, float y) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	rvn_tl_layout_t layout;
	int part = RVN_TL_PART_NONE, item, toggle;
	RCursor cursor = (RCursor)0;
	rvn_tl_layout(tl, &layout);
	item = rvn_tl_hit(tl, &layout, x, y, &part);
	toggle = rvn_tl_toggle_at(tl, &layout, x, y);
	if (item) {
		rvn_tl_item_t* found = rvn_tl_item(tl, item);
		if (found && !rvn_tl_item_locked(tl, found) && (part == RVN_TL_PART_LEFT || part == RVN_TL_PART_RIGHT)) cursor = R_CURSOR_RESIZE_H;
	}
	if (toggle >= 0) cursor = R_CURSOR_HAND;
	if (item != tl->hot_item || part != tl->hot_part || toggle != tl->hot_toggle) {
		tl->hot_item = item;
		tl->hot_part = part;
		tl->hot_toggle = toggle;
		rvn_widget_invalidate_paint(widget);
	}
	widget->cursor = cursor;
}

/* ---------------------------------------------------------------------------
 *  Gestures
 * ------------------------------------------------------------------------- */

static void rvn_tl_remember(rvn_timeline_t* tl) {
	uint32_t i;
	for (i = 0; i < tl->item_count; i++) {
		tl->items[i].from_start = tl->items[i].start;
		tl->items[i].from_length = tl->items[i].length;
		tl->items[i].from_track = tl->items[i].track;
	}
}

static void rvn_tl_restore(rvn_timeline_t* tl) {
	uint32_t i;
	for (i = 0; i < tl->item_count; i++) {
		tl->items[i].start = tl->items[i].from_start;
		tl->items[i].length = tl->items[i].from_length;
		tl->items[i].track = tl->items[i].from_track;
	}
}

static double rvn_tl_minimum_length(const rvn_timeline_t* tl, const rvn_tl_layout_t* layout) {
	return tl->fps > 0.0 ? 1.0 / tl->fps : rvn_tl_per_pixel(tl, layout) * 4.0;
}

/* Moves every selected item by the same time, and the clips and keys by the same number of tracks. */
static void rvn_tl_drag_move(rvn_widget_t* widget, const rvn_tl_layout_t* layout, float x, float y, int free_move) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	rvn_tl_item_t* lead = rvn_tl_item(tl, tl->drag_item);
	double delta = rvn_tl_time_at(tl, layout, x) - tl->press_time, low = 0.0, high = 0.0;
	int rows, row_low = 0, row_high = 0, first = R_TRUE;
	uint32_t i;
	if (!lead) return;

	tl->guide = -1.0;
	if (!free_move && !tl->no_snapping) {
		/* the item under the hand decides: by its start, or by its end when that one is closer to something */
		double start = lead->from_start + delta, end = start + lead->from_length, found_start = 0.0, found_end = 0.0;
		int at_start = rvn_tl_magnet(tl, layout, start, R_TRUE, &found_start);
		int at_end = lead->type == R_TIMELINE_CLIP && rvn_tl_magnet(tl, layout, end, R_TRUE, &found_end);
		if (at_start && (!at_end || fabs(found_start - start) <= fabs(found_end - end))) {
			delta += found_start - start;
			tl->guide = found_start;
		} else if (at_end) {
			delta += found_end - end;
			tl->guide = found_end;
		} else {
			delta = rvn_tl_quantize(tl, layout, start) - lead->from_start;
		}
	}
	/* nothing leaves the sequence, and nothing changes place with regard to the rest */
	rows = rvn_tl_row_at(tl, layout, y) - tl->press_row;
	for (i = 0; i < tl->item_count; i++) {
		const rvn_tl_item_t* item = &tl->items[i];
		double room_before, room_after;
		int row;
		if (!item->selected) continue;
		room_before = -item->from_start;
		room_after = tl->duration - (item->from_start + item->from_length);
		if (first || room_before > low) low = room_before;
		if (first || room_after < high) high = room_after;
		if (item->type != R_TIMELINE_MARKER) {
			row = rvn_tl_track_index(tl, item->from_track);
			if (first || -row > row_low) row_low = -row;
			if (first || (int)tl->track_count - 1 - row < row_high) row_high = (int)tl->track_count - 1 - row;
		}
		first = R_FALSE;
	}
	if (delta < low) delta = low;
	if (delta > high) delta = high;
	if (lead->type == R_TIMELINE_MARKER) rows = 0;
	rows = rvn_clampi(rows, row_low, rvn_maxi(row_low, row_high));

	for (i = 0; i < tl->item_count; i++) {
		rvn_tl_item_t* item = &tl->items[i];
		double start;
		int track;
		if (!item->selected) continue;
		start = item->from_start + delta;
		track = item->from_track;
		if (item->type != R_TIMELINE_MARKER && rows != 0) {
			int target = rvn_tl_track_index(tl, item->from_track) + rows;
			/* (a locked track takes nothing) */
			if (target >= 0 && target < (int)tl->track_count && !tl->tracks[target].locked) track = tl->tracks[target].id;
		}
		if (start != item->start || track != item->track) {
			item->start = start;
			item->track = track;
			tl->gesture_changed = R_TRUE;
			rvn_tl_notify_item(widget, tl->changed_callback, tl->changed_user, item->id);
			if (!rvn_widget_alive(widget)) return;
		}
	}
	rvn_widget_invalidate_paint(widget);
}

static void rvn_tl_drag_stretch(rvn_widget_t* widget, const rvn_tl_layout_t* layout, float x, int free_move) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	rvn_tl_item_t* item = rvn_tl_item(tl, tl->drag_item);
	double minimum = rvn_tl_minimum_length(tl, layout), time, found = 0.0;
	double start, length;
	if (!item) return;
	time = rvn_tl_time_at(tl, layout, x);
	tl->guide = -1.0;
	if (!free_move && !tl->no_snapping) {
		/* (its own ends are among the candidates while it is not marked as moving) */
		int was = item->selected;
		item->selected = R_TRUE;
		if (rvn_tl_magnet(tl, layout, time, R_TRUE, &found)) {
			time = found;
			tl->guide = found;
		} else {
			time = rvn_tl_quantize(tl, layout, time);
		}
		item->selected = was;
	}
	start = item->from_start;
	length = item->from_length;
	if (tl->drag == RVN_TL_DRAG_LEFT) {
		double end = item->from_start + item->from_length;
		if (time < 0.0) time = 0.0;
		if (time > end - minimum) time = end - minimum;
		start = time;
		length = end - time;
	} else {
		if (time > tl->duration) time = tl->duration;
		if (time < item->from_start + minimum) time = item->from_start + minimum;
		length = time - item->from_start;
	}
	if (start != item->start || length != item->length) {
		item->start = start;
		item->length = length;
		tl->gesture_changed = R_TRUE;
		rvn_tl_notify_item(widget, tl->changed_callback, tl->changed_user, item->id);
	}
	rvn_widget_invalidate_paint(widget);
}

static void rvn_tl_drag_box(rvn_widget_t* widget, const rvn_tl_layout_t* layout, float x, float y) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	uint32_t i;
	int changed = R_FALSE;
	x = rvn_clampf(x, layout->lanes.x, layout->lanes.x + layout->lanes.w);
	y = rvn_clampf(y, layout->lanes.y, layout->lanes.y + layout->lanes.h);
	tl->box = rvn_rect(rvn_minf(tl->press_x, x), rvn_minf(tl->press_y, y), fabsf(x - tl->press_x), fabsf(y - tl->press_y));
	for (i = 0; i < tl->item_count; i++) {
		rvn_tl_item_t* item = &tl->items[i];
		int index = rvn_tl_track_index(tl, item->track), inside;
		float height, top, x0, x1;
		if (index < 0 || item->type == R_TIMELINE_MARKER) continue;
		top = rvn_tl_track_y(tl, layout, index, &height);
		x0 = rvn_tl_x(tl, layout, item->start);
		x1 = item->type == R_TIMELINE_CLIP ? rvn_tl_x(tl, layout, item->start + item->length) : x0;
		inside = x1 >= tl->box.x && x0 <= tl->box.x + tl->box.w && top + height > tl->box.y && top < tl->box.y + tl->box.h;
		/* (from_track doubles as "was selected before the rectangle": Shift adds to it) */
		inside = inside || item->from_track < 0;
		if (inside != item->selected) {
			item->selected = inside;
			changed = R_TRUE;
		}
	}
	if (changed) rvn_tl_notify_item(widget, tl->selection_callback, tl->selection_user, 0);
	rvn_widget_invalidate_paint(widget);
}

static void rvn_tl_end_drag(rvn_widget_t* widget, int cancel) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	int changed = tl->gesture_changed;
	if (tl->drag == RVN_TL_DRAG_NONE) return;
	if (tl->drag == RVN_TL_DRAG_BOX) {
		uint32_t i;
		for (i = 0; i < tl->item_count; i++) tl->items[i].from_track = tl->items[i].track;
	} else if (cancel && (tl->drag == RVN_TL_DRAG_MOVE || tl->drag == RVN_TL_DRAG_LEFT || tl->drag == RVN_TL_DRAG_RIGHT)) {
		rvn_tl_restore(tl);
	}
	tl->drag = RVN_TL_DRAG_NONE;
	tl->drag_item = 0;
	tl->guide = -1.0;
	tl->gesture_changed = R_FALSE;
	rvn_widget_release_mouse(widget);
	rvn_widget_invalidate_paint(widget);
	/* one action for the whole gesture */
	if (changed && rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

static void rvn_tl_zoom(rvn_timeline_t* tl, const rvn_tl_layout_t* layout, float x, float factor) {
	double anchor = rvn_tl_time_at(tl, layout, x);
	double fraction = (double)rvn_clampf((x - layout->lanes.x) / layout->lanes.w, 0.0f, 1.0f);
	double span = tl->view_span * (double)factor;
	tl->view_span = span;
	rvn_tl_clamp_view(tl);
	rvn_tl_set_view(tl, anchor - fraction * tl->view_span, tl->view_span, R_TRUE);
}

static void rvn_tl_remove_selected(rvn_widget_t* widget) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	uint32_t i = 0;
	int any = R_FALSE;
	if (!tl->removed_callback) return;
	while (i < tl->item_count) {
		int id;
		if (!tl->items[i].selected || rvn_tl_item_locked(tl, &tl->items[i])) {
			i++;
			continue;
		}
		id = tl->items[i].id;
		tl->removed_callback(widget->id, id, tl->removed_user);
		if (!rvn_widget_alive(widget)) return;
		/* (the callback may have removed it itself, or others) */
		for (i = 0; i < tl->item_count && tl->items[i].id != id; i++) {}
		if (i < tl->item_count) rvn_tl_remove_item_at(tl, i);
		any = R_TRUE;
		i = 0;
	}
	if (any) {
		rvn_widget_invalidate_paint(widget);
		rvn_tl_notify_item(widget, tl->selection_callback, tl->selection_user, 0);
		if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
	}
}

static int rvn_timeline_event(rvn_widget_t* widget, const REvent* event) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	rvn_tl_layout_t layout;
	float x = event->local_x, y = event->local_y;
	int changed = R_FALSE, used, part = RVN_TL_PART_NONE;

	rvn_tl_layout(tl, &layout);
	switch (event->type) {
		case R_EVENT_MOUSE_WHEEL: {
			float amount = event->wheel_y != 0.0f ? event->wheel_y : event->wheel_x;
			if (event->modifiers & R_MOD_CTRL) {
				rvn_tl_zoom(tl, &layout, x, powf(0.82f, amount));
				return R_TRUE;
			}
			/* the wheel scrolls the tracks when there are more than fit, else it moves along the time */
			if (!(event->modifiers & R_MOD_SHIFT) && event->wheel_y != 0.0f && rvn_scrollbar_needed(&tl->track_bar)) {
				changed = rvn_scrollbar_wheel(&tl->track_bar, -event->wheel_y * rvn_tl_default_track_height(tl));
				rvn_widget_invalidate_paint(widget);
				return changed;
			}
			{
				double before = tl->view_start;
				rvn_tl_set_view(tl, tl->view_start - (double)amount * tl->view_span * 0.12, tl->view_span, R_TRUE);
				return tl->view_start != before;
			}
		}
		case R_EVENT_MOUSE_DOWN:
			if (tl->drag != RVN_TL_DRAG_NONE) return R_TRUE;
			if (event->button == R_MOUSE_LEFT) {
				used = rvn_scrollbar_event(&tl->time_bar, widget, event, layout.time_track, R_FALSE, &changed);
				if (!used) used = rvn_scrollbar_event(&tl->track_bar, widget, event, layout.track_track, R_TRUE, &changed);
				if (used) {
					if (changed) rvn_tl_set_view(tl, (double)tl->time_bar.offset * (tl->view_span / (double)layout.lanes.w), tl->view_span, R_FALSE);
					return R_TRUE;
				}
			}
			tl->press_x = x;
			tl->press_y = y;
			tl->press_time = rvn_tl_time_at(tl, &layout, x);
			tl->press_row = rvn_tl_row_at(tl, &layout, y);
			tl->drag_moved = R_FALSE;
			tl->gesture_changed = R_FALSE;
			if (event->button == R_MOUSE_MIDDLE) {
				tl->drag = RVN_TL_DRAG_PAN;
				tl->pan_start = tl->view_start;
				tl->pan_scroll = tl->track_bar.offset;
				rvn_widget_capture_mouse(widget);
				return R_TRUE;
			}
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			{
				int toggle = rvn_tl_toggle_at(tl, &layout, x, y);
				if (toggle >= 0) {
					rvn_tl_track_t* track = &tl->tracks[toggle / 2];
					if (toggle % 2 == 0) track->muted = !track->muted;
					else track->locked = !track->locked;
					rvn_widget_invalidate_paint(widget);
					rvn_widget_fire_action(widget);
					return R_TRUE;
				}
			}
			{
				int id = rvn_tl_hit(tl, &layout, x, y, &part);
				rvn_tl_item_t* item = rvn_tl_item(tl, id);
				if (item) {
					int add = (event->modifiers & (R_MOD_CTRL | R_MOD_SHIFT)) != 0;
					if (event->clicks >= 2) {
						rvn_tl_notify_item(widget, tl->activated_callback, tl->activated_user, id);
						return R_TRUE;
					}
					if (add) {
						item->selected = !item->selected;
						rvn_tl_notify_item(widget, tl->selection_callback, tl->selection_user, id);
					} else if (!item->selected) {
						rvn_tl_select_only(tl, id);
						rvn_tl_notify_item(widget, tl->selection_callback, tl->selection_user, id);
					}
					if (!rvn_widget_alive(widget)) return R_TRUE;
					item = rvn_tl_item(tl, id);
					rvn_widget_invalidate_paint(widget);
					if (item && item->selected && !rvn_tl_item_locked(tl, item)) {
						tl->drag = part == RVN_TL_PART_LEFT ? RVN_TL_DRAG_LEFT : (part == RVN_TL_PART_RIGHT ? RVN_TL_DRAG_RIGHT : RVN_TL_DRAG_MOVE);
						tl->drag_item = id;
						rvn_tl_remember(tl);
						rvn_widget_capture_mouse(widget);
					}
					return R_TRUE;
				}
			}
			if (y < layout.ruler_h && x >= layout.ruler.x) {
				/* the ruler: the playhead comes under the pointer and follows it */
				double time = tl->press_time, found = 0.0;
				tl->drag = RVN_TL_DRAG_SCRUB;
				rvn_widget_capture_mouse(widget);
				if (!tl->no_snapping && !(event->modifiers & R_MOD_ALT)) {
					if (rvn_tl_magnet(tl, &layout, time, R_FALSE, &found)) time = found;
					else time = rvn_tl_quantize(tl, &layout, time);
				}
				rvn_tl_set_time(widget, time, R_TRUE);
				return R_TRUE;
			}
			if (rvn_rect_contains(layout.lanes, x, y)) {
				/* empty space: a selection rectangle; a simple click only forgets the selection */
				uint32_t i;
				int add = (event->modifiers & (R_MOD_CTRL | R_MOD_SHIFT)) != 0, had = R_FALSE;
				for (i = 0; i < tl->item_count; i++) {
					had = had || tl->items[i].selected;
					tl->items[i].from_track = add && tl->items[i].selected ? -1 : tl->items[i].track;
					if (!add) tl->items[i].selected = R_FALSE;
				}
				tl->drag = RVN_TL_DRAG_BOX;
				tl->box = rvn_rect(x, y, 0.0f, 0.0f);
				rvn_widget_capture_mouse(widget);
				rvn_widget_invalidate_paint(widget);
				if (had && !add) rvn_tl_notify_item(widget, tl->selection_callback, tl->selection_user, 0);
				return R_TRUE;
			}
			return R_TRUE;
		case R_EVENT_MOUSE_MOVE:
			if (tl->drag == RVN_TL_DRAG_NONE) {
				used = rvn_scrollbar_event(&tl->time_bar, widget, event, layout.time_track, R_FALSE, &changed);
				if (!used) used = rvn_scrollbar_event(&tl->track_bar, widget, event, layout.track_track, R_TRUE, &changed);
				if (changed) rvn_tl_set_view(tl, (double)tl->time_bar.offset * (tl->view_span / (double)layout.lanes.w), tl->view_span, R_FALSE);
				if (used) {
					rvn_widget_invalidate_paint(widget);
					return R_TRUE;
				}
				rvn_tl_update_hot(widget, x, y);
				/* (the scrollbars show when the pointer comes near them) */
				rvn_widget_invalidate_paint(widget);
				return R_FALSE;
			}
			if (!tl->drag_moved && fabsf(x - tl->press_x) < 3.0f && fabsf(y - tl->press_y) < 3.0f && tl->drag != RVN_TL_DRAG_SCRUB) return R_TRUE;
			tl->drag_moved = R_TRUE;
			switch (tl->drag) {
				case RVN_TL_DRAG_SCRUB: {
					double time = rvn_tl_time_at(tl, &layout, x), found = 0.0;
					if (!tl->no_snapping && !(event->modifiers & R_MOD_ALT)) {
						if (rvn_tl_magnet(tl, &layout, time, R_FALSE, &found)) time = found;
						else time = rvn_tl_quantize(tl, &layout, time);
					}
					rvn_tl_set_time(widget, time, R_TRUE);
					break;
				}
				case RVN_TL_DRAG_MOVE:
					rvn_tl_drag_move(widget, &layout, x, y, (event->modifiers & R_MOD_ALT) != 0);
					break;
				case RVN_TL_DRAG_LEFT:
				case RVN_TL_DRAG_RIGHT:
					rvn_tl_drag_stretch(widget, &layout, x, (event->modifiers & R_MOD_ALT) != 0);
					break;
				case RVN_TL_DRAG_BOX:
					rvn_tl_drag_box(widget, &layout, x, y);
					break;
				case RVN_TL_DRAG_PAN:
					rvn_tl_set_view(tl, tl->pan_start - (double)(x - tl->press_x) * (tl->view_span / (double)layout.lanes.w), tl->view_span, R_FALSE);
					rvn_scrollbar_set(&tl->track_bar, tl->pan_scroll - (y - tl->press_y));
					break;
				default:
					break;
			}
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (tl->drag == RVN_TL_DRAG_NONE) {
				used = rvn_scrollbar_event(&tl->time_bar, widget, event, layout.time_track, R_FALSE, &changed);
				if (!used) used = rvn_scrollbar_event(&tl->track_bar, widget, event, layout.track_track, R_TRUE, &changed);
				return used;
			}
			if ((tl->drag == RVN_TL_DRAG_PAN) != (event->button == R_MOUSE_MIDDLE)) return R_TRUE;
			/* a click on one of several selected items, without moving: it becomes the only one */
			if (tl->drag == RVN_TL_DRAG_MOVE && !tl->drag_moved && !(event->modifiers & (R_MOD_CTRL | R_MOD_SHIFT))) {
				int id = tl->drag_item;
				uint32_t i, count = 0;
				for (i = 0; i < tl->item_count; i++) count += tl->items[i].selected ? 1u : 0u;
				if (count > 1) {
					rvn_tl_select_only(tl, id);
					rvn_tl_notify_item(widget, tl->selection_callback, tl->selection_user, id);
					if (!rvn_widget_alive(widget)) return R_TRUE;
				}
			}
			rvn_tl_end_drag(widget, R_FALSE);
			if (rvn_widget_alive(widget)) rvn_tl_update_hot(widget, x, y);
			return R_TRUE;
		case R_EVENT_MOUSE_LEAVE:
			if (tl->hot_item || tl->hot_toggle >= 0) {
				tl->hot_item = 0;
				tl->hot_toggle = -1;
			}
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_KEY_DOWN: {
			double step = tl->fps > 0.0 ? 1.0 / tl->fps : rvn_tl_per_pixel(tl, &layout) * 10.0;
			if (event->modifiers & R_MOD_SHIFT) step *= 10.0;
			switch (event->key) {
				case R_KEY_ESCAPE:
					if (tl->drag == RVN_TL_DRAG_NONE) return R_FALSE;
					rvn_tl_end_drag(widget, R_TRUE);
					return R_TRUE;
				case R_KEY_LEFT:
					rvn_tl_set_time(widget, tl->time - step, R_TRUE);
					return R_TRUE;
				case R_KEY_RIGHT:
					rvn_tl_set_time(widget, tl->time + step, R_TRUE);
					return R_TRUE;
				case R_KEY_HOME:
					rvn_tl_set_time(widget, 0.0, R_TRUE);
					if (rvn_widget_alive(widget)) rvn_tl_set_view(tl, 0.0, tl->view_span, R_TRUE);
					return R_TRUE;
				case R_KEY_END:
					rvn_tl_set_time(widget, tl->duration, R_TRUE);
					if (rvn_widget_alive(widget)) rvn_tl_set_view(tl, tl->duration, tl->view_span, R_TRUE);
					return R_TRUE;
				case R_KEY_SPACE:
					rvn_tl_set_playing(widget, !tl->playing);
					return R_TRUE;
				case R_KEY_DELETE:
				case R_KEY_BACKSPACE:
					rvn_tl_remove_selected(widget);
					return R_TRUE;
				case R_KEY_A:
					if (event->modifiers & R_MOD_CTRL) {
						uint32_t i;
						for (i = 0; i < tl->item_count; i++) tl->items[i].selected = R_TRUE;
						rvn_widget_invalidate_paint(widget);
						rvn_tl_notify_item(widget, tl->selection_callback, tl->selection_user, 0);
						return R_TRUE;
					}
					return R_FALSE;
				default:
					return R_FALSE;
			}
		}
		default:
			return R_FALSE;
	}
}

/* ---------------------------------------------------------------------------
 *  XML
 * ------------------------------------------------------------------------- */

static int rvn_tl_xml_double(const rvn_xml_node_t* node, const char* name, double* value) {
	const char* text = rvn_xml_attribute(node, name);
	float parsed;
	if (!text) return R_FALSE;
	if (!rvn_parse_float(text, &parsed)) {
		rvn_error(R_ERROR_XML, R_SEVERITY_WARNING, "XML line %d: <%.40s %.40s=\"%.40s\">: not a number", node->line, node->name, name, text);
		return R_FALSE;
	}
	*value = (double)parsed;
	return R_TRUE;
}

static void rvn_tl_xml_item(rvn_timeline_t* tl, const rvn_xml_node_t* node, rvn_tl_item_t* item) {
	const char* text = rvn_xml_attribute(node, "Color");
	int flag;
	if (text && rvn_parse_color(text, &item->color)) item->has_color = R_TRUE;
	text = rvn_xml_attribute(node, "Selected");
	if (text && rvn_parse_bool(text, &flag)) item->selected = flag;
	RVN_UNUSED(tl);
}

static int rvn_timeline_xml_child(rvn_widget_t* widget, const rvn_xml_node_t* node) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	const rvn_xml_node_t* child;
	const char* text;
	double start = 0.0, length = 1.0, end = 0.0;
	int flag;

	if (rvn_stricmp(node->name, "Marker") == 0) {
		rvn_tl_xml_double(node, "Time", &start);
		text = rvn_xml_attribute(node, "Label");
		rvn_tl_xml_item(tl, node, rvn_tl_add_item(tl, R_TIMELINE_MARKER, 0, start, 0.0, text ? text : node->text));
		return R_TRUE;
	}
	if (rvn_stricmp(node->name, "Track") == 0) {
		rvn_tl_track_t* track;
		float height;
		int id;
		text = rvn_xml_attribute(node, "Name");
		id = Rtimeline_add_track(widget->id, text ? text : "");
		track = rvn_tl_track(tl, id);
		if (!track) return R_TRUE;
		text = rvn_xml_attribute(node, "Color");
		if (text && rvn_parse_color(text, &track->color)) track->has_color = R_TRUE;
		text = rvn_xml_attribute(node, "Height");
		if (text && rvn_parse_float(text, &height)) track->height = height;
		text = rvn_xml_attribute(node, "Locked");
		if (text && rvn_parse_bool(text, &flag)) track->locked = flag;
		text = rvn_xml_attribute(node, "Muted");
		if (text && rvn_parse_bool(text, &flag)) track->muted = flag;
		for (child = node->first_child; child; child = child->next) {
			if (rvn_stricmp(child->name, "Clip") == 0) {
				start = 0.0;
				length = 1.0;
				rvn_tl_xml_double(child, "Start", &start);
				if (rvn_tl_xml_double(child, "End", &end)) length = end - start;
				rvn_tl_xml_double(child, "Length", &length);
				text = rvn_xml_attribute(child, "Label");
				rvn_tl_xml_item(tl, child, rvn_tl_add_item(tl, R_TIMELINE_CLIP, id, start, length, text ? text : child->text));
			} else if (rvn_stricmp(child->name, "Key") == 0) {
				start = 0.0;
				rvn_tl_xml_double(child, "Time", &start);
				text = rvn_xml_attribute(child, "Label");
				rvn_tl_xml_item(tl, child, rvn_tl_add_item(tl, R_TIMELINE_KEY, id, start, 0.0, text));
			} else {
				rvn_error(R_ERROR_XML, R_SEVERITY_WARNING, "XML line %d: a <Track> holds <Clip> and <Key>, not <%.40s>", child->line, child->name);
			}
		}
		return R_TRUE;
	}
	return R_FALSE;
}

static int rvn_timeline_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	float number;
	int flag;
	if (RVN_ATTR("Duration")) {
		if (!rvn_parse_float(value, &number) || number <= 0.0f) return R_FALSE;
		Rtimeline_set_duration(widget->id, (double)number);
	} else if (RVN_ATTR("Time")) {
		if (!rvn_parse_float(value, &number)) return R_FALSE;
		rvn_tl_set_time(widget, (double)number, R_FALSE);
	} else if (RVN_ATTR("FrameRate") || RVN_ATTR("Fps")) {
		if (!rvn_parse_float(value, &number) || number < 0.0f) return R_FALSE;
		tl->fps = (double)number;
	} else if (RVN_ATTR("Snapping") || RVN_ATTR("Snap")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		tl->no_snapping = !flag;
	} else if (RVN_ATTR("ViewStart")) {
		if (!rvn_parse_float(value, &number)) return R_FALSE;
		rvn_tl_set_view(tl, (double)number, tl->view_span, R_FALSE);
	} else if (RVN_ATTR("ViewEnd")) {
		if (!rvn_parse_float(value, &number)) return R_FALSE;
		rvn_tl_set_view(tl, tl->view_start, (double)number - tl->view_start, R_FALSE);
	} else if (RVN_ATTR("Loop")) {
		if (!rvn_parse_bool(value, &tl->loop)) return R_FALSE;
	} else if (RVN_ATTR("Playing")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		rvn_tl_set_playing(widget, flag);
	} else if (RVN_ATTR("HeaderWidth")) {
		if (!rvn_parse_float(value, &number) || number < 0.0f) return R_FALSE;
		Rtimeline_set_header_width(widget->id, number);
	} else if (RVN_ATTR("TrackHeight")) {
		if (!rvn_parse_float(value, &number) || number < 0.0f) return R_FALSE;
		Rtimeline_set_default_track_height(widget->id, number);
	} else if (RVN_ATTR("RulerHeight")) {
		if (!rvn_parse_float(value, &number) || number < 0.0f) return R_FALSE;
		Rtimeline_set_ruler_height(widget->id, number);
	} else {
		return R_FALSE;
	}
	rvn_tl_sync_bars(tl);
	rvn_widget_invalidate_paint(widget);
	return R_TRUE;
}

/* A finger moves along the time and through the tracks; two fingers zoom the time. */
static int rvn_timeline_pan(rvn_widget_t* widget, float dx, float dy, int coasting) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	rvn_tl_layout_t layout;
	double before = tl->view_start;
	int moved = R_FALSE;
	rvn_tl_layout(tl, &layout);
	if (dx != 0.0f && layout.lanes.w > 1.0f) {
		rvn_tl_set_view(tl, tl->view_start - (double)dx * tl->view_span / (double)layout.lanes.w, tl->view_span, R_FALSE);
		moved = tl->view_start != before;
	}
	if (dy != 0.0f && rvn_scrollbar_needed(&tl->track_bar)) moved |= rvn_scrollbar_pan(&tl->track_bar, -dy, coasting);
	return moved;
}

static int rvn_timeline_zoom(rvn_widget_t* widget, float factor, float x, float y) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	rvn_tl_layout_t layout;
	double before = tl->view_span;
	RVN_UNUSED(y);
	if (!(factor > 0.0f)) return R_FALSE;
	rvn_tl_layout(tl, &layout);
	rvn_tl_zoom(tl, &layout, x, 1.0f / factor);
	return tl->view_span != before;
}

/* The ruler (the playhead), the items and the scrollbars drag; the empty lanes pan. */
static int rvn_timeline_touch_drags(rvn_widget_t* widget, float x, float y) {
	rvn_timeline_t* tl = (rvn_timeline_t*)widget;
	rvn_tl_layout_t layout;
	int part;
	rvn_tl_layout(tl, &layout);
	if (y < layout.ruler_h || rvn_rect_contains(layout.time_track, x, y) || rvn_rect_contains(layout.track_track, x, y)) return R_TRUE;
	if (rvn_rect_contains(layout.headers, x, y)) return R_FALSE;
	return rvn_tl_hit(tl, &layout, x, y, &part) != 0 || part != RVN_TL_PART_NONE;
}

const rvn_widget_class_t rvn_class_timeline = {
	.type = R_WIDGET_TIMELINE,
	.name = "Timeline",
	.size = sizeof(rvn_timeline_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_CLIP | RVN_CF_SURFACE | RVN_CF_NO_MIRROR,
	.init = rvn_timeline_init,
	.destroy = rvn_timeline_destroy,
	.measure = rvn_timeline_measure,
	.arrange = rvn_timeline_arrange,
	.paint = rvn_timeline_paint,
	.paint_overlay = rvn_timeline_paint_overlay,
	.event = rvn_timeline_event,
	.set_attribute = rvn_timeline_set_attribute,
	.xml_child = rvn_timeline_xml_child,
	.pan = rvn_timeline_pan,
	.zoom = rvn_timeline_zoom,
	.touch_drags = rvn_timeline_touch_drags
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

#define RVN_TIMELINE(variable, id, result) \
	rvn_timeline_t* variable = (rvn_timeline_t*)rvn_widget_check((id), R_WIDGET_TIMELINE, __func__); \
	if (!variable) return result

#define RVN_TIMELINE_VOID(variable, id) \
	rvn_timeline_t* variable = (rvn_timeline_t*)rvn_widget_check((id), R_WIDGET_TIMELINE, __func__); \
	if (!variable) return

Rid Rtimeline_create(void) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_TIMELINE);
	return widget ? widget->id : R_INVALID_ID;
}

void Rtimeline_set_duration(Rid id, double seconds) {
	int whole;
	RVN_TIMELINE_VOID(tl, id);
	if (!(seconds > 0.0)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtimeline_set_duration: the duration must be positive");
		return;
	}
	/* (a view that showed everything goes on showing everything) */
	whole = tl->view_start <= 0.0 && tl->view_span >= tl->duration;
	tl->duration = seconds;
	if (tl->time > seconds) tl->time = seconds;
	rvn_tl_set_view(tl, whole ? 0.0 : tl->view_start, whole ? seconds : tl->view_span, R_FALSE);
}

double Rtimeline_get_duration(Rid id) {
	RVN_TIMELINE(tl, id, 0.0);
	return tl->duration;
}

void Rtimeline_set_time(Rid id, double seconds) {
	RVN_TIMELINE_VOID(tl, id);
	rvn_tl_set_time(&tl->base, seconds, R_FALSE);
}

double Rtimeline_get_time(Rid id) {
	RVN_TIMELINE(tl, id, 0.0);
	return tl->time;
}

void Rtimeline_set_frame_rate(Rid id, double frames_per_second) {
	RVN_TIMELINE_VOID(tl, id);
	tl->fps = frames_per_second > 0.0 ? frames_per_second : 0.0;
	rvn_tl_set_view(tl, tl->view_start, tl->view_span, R_FALSE);
}

double Rtimeline_get_frame_rate(Rid id) {
	RVN_TIMELINE(tl, id, 0.0);
	return tl->fps;
}

void Rtimeline_set_snapping(Rid id, int enabled) {
	RVN_TIMELINE_VOID(tl, id);
	tl->no_snapping = !enabled;
}

void Rtimeline_set_view(Rid id, double start, double end) {
	RVN_TIMELINE_VOID(tl, id);
	if (!(end > start)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtimeline_set_view: the end must be after the start");
		return;
	}
	rvn_tl_set_view(tl, start, end - start, R_FALSE);
}

void Rtimeline_get_view(Rid id, double* start, double* end) {
	rvn_timeline_t* tl = (rvn_timeline_t*)rvn_widget_check(id, R_WIDGET_TIMELINE, __func__);
	if (start) *start = tl ? tl->view_start : 0.0;
	if (end) *end = tl ? tl->view_start + tl->view_span : 0.0;
}

void Rtimeline_set_playing(Rid id, int playing) {
	RVN_TIMELINE_VOID(tl, id);
	rvn_tl_set_playing(&tl->base, playing);
}

int Rtimeline_is_playing(Rid id) {
	RVN_TIMELINE(tl, id, R_FALSE);
	return tl->playing;
}

void Rtimeline_set_loop(Rid id, int loop) {
	RVN_TIMELINE_VOID(tl, id);
	tl->loop = loop != 0;
}

/* ---- tracks ---- */

int Rtimeline_add_track(Rid id, const char* name) {
	rvn_tl_track_t* track;
	RVN_TIMELINE(tl, id, 0);
	RVN_ARRAY_RESERVE(tl->tracks, tl->track_cap, tl->track_count + 1);
	track = &tl->tracks[tl->track_count++];
	memset(track, 0, sizeof(*track));
	track->id = tl->next_id++;
	track->name = rvn_strdup(name ? name : "");
	rvn_tl_sync_bars(tl);
	rvn_widget_invalidate_layout(&tl->base);
	return track->id;
}

void Rtimeline_remove_track(Rid id, int track_id) {
	int index;
	uint32_t i = 0;
	RVN_TIMELINE_VOID(tl, id);
	index = rvn_tl_track_index(tl, track_id);
	if (index < 0) return;
	while (i < tl->item_count) {
		if (tl->items[i].track == track_id) rvn_tl_remove_item_at(tl, i);
		else i++;
	}
	rvn_free(tl->tracks[index].name);
	memmove(&tl->tracks[index], &tl->tracks[index + 1], sizeof(rvn_tl_track_t) * (tl->track_count - (uint32_t)index - 1));
	tl->track_count--;
	tl->hot_toggle = -1;
	rvn_tl_sync_bars(tl);
	rvn_widget_invalidate_layout(&tl->base);
}

int Rtimeline_get_track_count(Rid id) {
	RVN_TIMELINE(tl, id, 0);
	return (int)tl->track_count;
}

int Rtimeline_get_track(Rid id, int index) {
	RVN_TIMELINE(tl, id, 0);
	return index >= 0 && index < (int)tl->track_count ? tl->tracks[index].id : 0;
}

void Rtimeline_set_track_name(Rid id, int track_id, const char* name) {
	rvn_tl_track_t* track;
	RVN_TIMELINE_VOID(tl, id);
	track = rvn_tl_track(tl, track_id);
	if (track && rvn_str_set(&track->name, name ? name : "")) rvn_widget_invalidate_paint(&tl->base);
}

const char* Rtimeline_get_track_name(Rid id, int track_id) {
	rvn_tl_track_t* track;
	RVN_TIMELINE(tl, id, "");
	track = rvn_tl_track(tl, track_id);
	return track && track->name ? track->name : "";
}

void Rtimeline_set_track_color(Rid id, int track_id, float r, float g, float b) {
	rvn_tl_track_t* track;
	RVN_TIMELINE_VOID(tl, id);
	track = rvn_tl_track(tl, track_id);
	if (!track) return;
	track->color = rvn_rgba(r, g, b, 1.0f);
	track->has_color = R_TRUE;
	rvn_widget_invalidate_paint(&tl->base);
}

void Rtimeline_set_header_width(Rid id, float width) {
	RVN_TIMELINE_VOID(tl, id);
	tl->header_width = width > 0.0f ? width : 0.0f;
	rvn_tl_sync_bars(tl);
	rvn_widget_invalidate_layout(&tl->base);
}

float Rtimeline_get_header_width(Rid id) {
	rvn_tl_layout_t layout;
	RVN_TIMELINE(tl, id, 0.0f);
	rvn_tl_layout(tl, &layout);
	return layout.headers.w;
}

void Rtimeline_set_ruler_height(Rid id, float height) {
	RVN_TIMELINE_VOID(tl, id);
	tl->ruler_height = height > 0.0f ? rvn_maxf(height, RVN_TL_MIN_TRACK) : 0.0f;
	rvn_tl_sync_bars(tl);
	rvn_widget_invalidate_layout(&tl->base);
}

float Rtimeline_get_ruler_height(Rid id) {
	RVN_TIMELINE(tl, id, 0.0f);
	return rvn_tl_ruler_height(tl);
}

void Rtimeline_set_default_track_height(Rid id, float height) {
	RVN_TIMELINE_VOID(tl, id);
	tl->track_height = height > 0.0f ? rvn_maxf(height, RVN_TL_MIN_TRACK) : 0.0f;
	rvn_tl_sync_bars(tl);
	rvn_widget_invalidate_layout(&tl->base);
}

float Rtimeline_get_default_track_height(Rid id) {
	RVN_TIMELINE(tl, id, 0.0f);
	return rvn_tl_default_track_height(tl);
}

void Rtimeline_set_track_height(Rid id, int track_id, float height) {
	rvn_tl_track_t* track;
	RVN_TIMELINE_VOID(tl, id);
	track = rvn_tl_track(tl, track_id);
	if (!track) return;
	track->height = height > 0.0f ? rvn_maxf(height, RVN_TL_MIN_TRACK) : 0.0f;
	rvn_tl_sync_bars(tl);
	rvn_widget_invalidate_layout(&tl->base);
}

void Rtimeline_set_track_locked(Rid id, int track_id, int locked) {
	rvn_tl_track_t* track;
	RVN_TIMELINE_VOID(tl, id);
	track = rvn_tl_track(tl, track_id);
	if (!track) return;
	track->locked = locked != 0;
	rvn_widget_invalidate_paint(&tl->base);
}

void Rtimeline_set_track_muted(Rid id, int track_id, int muted) {
	rvn_tl_track_t* track;
	RVN_TIMELINE_VOID(tl, id);
	track = rvn_tl_track(tl, track_id);
	if (!track) return;
	track->muted = muted != 0;
	rvn_widget_invalidate_paint(&tl->base);
}

int Rtimeline_is_track_muted(Rid id, int track_id) {
	rvn_tl_track_t* track;
	RVN_TIMELINE(tl, id, R_FALSE);
	track = rvn_tl_track(tl, track_id);
	return track ? track->muted : R_FALSE;
}

/* ---- items ---- */

int Rtimeline_add_clip(Rid id, int track, double start, double length, const char* label) {
	RVN_TIMELINE(tl, id, 0);
	if (!rvn_tl_track(tl, track)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtimeline_add_clip: no such track");
		return 0;
	}
	return rvn_tl_add_item(tl, R_TIMELINE_CLIP, track, start, length, label)->id;
}

int Rtimeline_add_key(Rid id, int track, double time) {
	RVN_TIMELINE(tl, id, 0);
	if (!rvn_tl_track(tl, track)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtimeline_add_key: no such track");
		return 0;
	}
	return rvn_tl_add_item(tl, R_TIMELINE_KEY, track, time, 0.0, NULL)->id;
}

int Rtimeline_add_marker(Rid id, double time, const char* label) {
	RVN_TIMELINE(tl, id, 0);
	return rvn_tl_add_item(tl, R_TIMELINE_MARKER, 0, time, 0.0, label)->id;
}

void Rtimeline_remove_item(Rid id, int item_id) {
	uint32_t i;
	RVN_TIMELINE_VOID(tl, id);
	for (i = 0; i < tl->item_count; i++) {
		if (tl->items[i].id == item_id) {
			rvn_tl_remove_item_at(tl, i);
			rvn_widget_invalidate_paint(&tl->base);
			return;
		}
	}
}

void Rtimeline_clear(Rid id) {
	RVN_TIMELINE_VOID(tl, id);
	if (tl->drag != RVN_TL_DRAG_NONE) rvn_tl_end_drag(&tl->base, R_TRUE);
	rvn_tl_clear(tl);
	rvn_tl_sync_bars(tl);
	rvn_widget_invalidate_layout(&tl->base);
}

RTimelineItem Rtimeline_get_item_type(Rid id, int item_id) {
	rvn_tl_item_t* item;
	RVN_TIMELINE(tl, id, R_TIMELINE_NONE);
	item = rvn_tl_item(tl, item_id);
	return item ? item->type : R_TIMELINE_NONE;
}

int Rtimeline_get_item_track(Rid id, int item_id) {
	rvn_tl_item_t* item;
	RVN_TIMELINE(tl, id, 0);
	item = rvn_tl_item(tl, item_id);
	return item ? item->track : 0;
}

double Rtimeline_get_item_start(Rid id, int item_id) {
	rvn_tl_item_t* item;
	RVN_TIMELINE(tl, id, 0.0);
	item = rvn_tl_item(tl, item_id);
	return item ? item->start : 0.0;
}

double Rtimeline_get_item_length(Rid id, int item_id) {
	rvn_tl_item_t* item;
	RVN_TIMELINE(tl, id, 0.0);
	item = rvn_tl_item(tl, item_id);
	return item ? item->length : 0.0;
}

void Rtimeline_set_item(Rid id, int item_id, int track, double start, double length) {
	rvn_tl_item_t* item;
	RVN_TIMELINE_VOID(tl, id);
	item = rvn_tl_item(tl, item_id);
	if (!item) return;
	if (track != 0 && item->type != R_TIMELINE_MARKER) {
		if (!rvn_tl_track(tl, track)) {
			rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtimeline_set_item: no such track");
			return;
		}
		item->track = track;
	}
	item->start = start < 0.0 ? 0.0 : start;
	if (length >= 0.0 && item->type == R_TIMELINE_CLIP) item->length = length;
	rvn_widget_invalidate_paint(&tl->base);
}

void Rtimeline_set_item_label(Rid id, int item_id, const char* label) {
	rvn_tl_item_t* item;
	RVN_TIMELINE_VOID(tl, id);
	item = rvn_tl_item(tl, item_id);
	if (item && rvn_str_set(&item->label, label && label[0] ? label : NULL)) rvn_widget_invalidate_paint(&tl->base);
}

const char* Rtimeline_get_item_label(Rid id, int item_id) {
	rvn_tl_item_t* item;
	RVN_TIMELINE(tl, id, "");
	item = rvn_tl_item(tl, item_id);
	return item && item->label ? item->label : "";
}

void Rtimeline_set_item_color(Rid id, int item_id, float r, float g, float b) {
	rvn_tl_item_t* item;
	RVN_TIMELINE_VOID(tl, id);
	item = rvn_tl_item(tl, item_id);
	if (!item) return;
	item->color = rvn_rgba(r, g, b, 1.0f);
	item->has_color = R_TRUE;
	rvn_widget_invalidate_paint(&tl->base);
}

int Rtimeline_get_item_count(Rid id) {
	RVN_TIMELINE(tl, id, 0);
	return (int)tl->item_count;
}

int Rtimeline_get_item(Rid id, int index) {
	RVN_TIMELINE(tl, id, 0);
	return index >= 0 && index < (int)tl->item_count ? tl->items[index].id : 0;
}

/* ---- selection ---- */

void Rtimeline_select(Rid id, int item_id, int add) {
	rvn_tl_item_t* item;
	RVN_TIMELINE_VOID(tl, id);
	item = rvn_tl_item(tl, item_id);
	if (!item) return;
	if (add) item->selected = R_TRUE;
	else rvn_tl_select_only(tl, item_id);
	rvn_widget_invalidate_paint(&tl->base);
}

void Rtimeline_clear_selection(Rid id) {
	RVN_TIMELINE_VOID(tl, id);
	rvn_tl_select_only(tl, 0);
	rvn_widget_invalidate_paint(&tl->base);
}

int Rtimeline_is_selected(Rid id, int item_id) {
	rvn_tl_item_t* item;
	RVN_TIMELINE(tl, id, R_FALSE);
	item = rvn_tl_item(tl, item_id);
	return item ? item->selected : R_FALSE;
}

int Rtimeline_get_selection_count(Rid id) {
	uint32_t i;
	int count = 0;
	RVN_TIMELINE(tl, id, 0);
	for (i = 0; i < tl->item_count; i++) count += tl->items[i].selected ? 1 : 0;
	return count;
}

int Rtimeline_get_selected(Rid id, int index) {
	uint32_t i;
	RVN_TIMELINE(tl, id, 0);
	for (i = 0; i < tl->item_count; i++) {
		if (tl->items[i].selected && index-- == 0) return tl->items[i].id;
	}
	return 0;
}

/* ---- callbacks ---- */

void Rtimeline_set_time_callback(Rid id, RTimelineTimeCallback callback, void* user_data) {
	RVN_TIMELINE_VOID(tl, id);
	tl->time_callback = callback;
	tl->time_user = user_data;
}

void Rtimeline_set_changed_callback(Rid id, RTimelineItemCallback callback, void* user_data) {
	RVN_TIMELINE_VOID(tl, id);
	tl->changed_callback = callback;
	tl->changed_user = user_data;
}

void Rtimeline_set_selection_callback(Rid id, RTimelineItemCallback callback, void* user_data) {
	RVN_TIMELINE_VOID(tl, id);
	tl->selection_callback = callback;
	tl->selection_user = user_data;
}

void Rtimeline_set_activated_callback(Rid id, RTimelineItemCallback callback, void* user_data) {
	RVN_TIMELINE_VOID(tl, id);
	tl->activated_callback = callback;
	tl->activated_user = user_data;
}

void Rtimeline_set_removed_callback(Rid id, RTimelineItemCallback callback, void* user_data) {
	RVN_TIMELINE_VOID(tl, id);
	tl->removed_callback = callback;
	tl->removed_user = user_data;
}
