/**
 * Raven - widgets/island.c
 * Island: a small dark capsule, usually at the top of a window, that unfolds
 * into a panel and folds back, like the notch of a laptop turned into a place
 * for live information.
 *
 * It has two contents: the first child is shown while it is folded, the
 * second while it is unfolded. The capsule goes from the size of one to the
 * size of the other on two springs, and paints both contents itself: each
 * one fades and scales with how far the capsule has gone, clipped to the
 * capsule, so that nothing is ever cut by its edge or drawn outside of it.
 *
 * An island is dark whatever the theme: what is inside it is drawn with the
 * dark counterpart of the theme (rvn_theme_dark_twin) unless it was given a
 * theme of its own.
 *
 * What keeps it unfolded, any of:
 *   - pinned: unfolded by a click or by Risland_set_expanded;
 *   - held: Risland_expand_for, until its time is over;
 *   - the pointer: over it (when it unfolds on hover, or once it is unfolded
 *     by anything else), or over a menu opened from inside it.
 * When nothing does, it folds, after a short delay if the pointer just left.
 *
 * In the camera housing of a screen (Risland_set_notch, on a MacBook): the
 * window puts the island in an overlay anchored to the screen (widget/
 * window.c) and tells it the size of the housing (rvn_island_set_notch).
 * Folded, it is the housing grown into two ears: the first Spacer of the
 * folded content becomes the room of the camera, what is before it shows on
 * the left of the camera and what is after on the right (without a Spacer
 * the folded content is on the left). Unfolded, its content starts under
 * the camera. It is black and square at the top, as the housing is.
 */

#include "common.h"

/** Time the pointer may spend outside before an island it kept open folds back. */
#define RVN_ISLAND_LEAVE_DELAY 0.28

typedef struct rvn_island_t {
	rvn_widget_t base;
	int expanded;
	int no_hover;
	int no_click;
	/** Glued to the edge it hangs from: square corners there (a notch). */
	int attached;

	int pinned;
	/** Risland_expand_for: when its time is over (seconds of rvn_clock_seconds), 0 = not held. */
	double hold_until;
	/** The pointer left at this time; it folds RVN_ISLAND_LEAVE_DELAY later. */
	double left_at;
	/** Folded by a click while under the pointer: the pointer does not unfold it again until it has left. */
	int hover_blocked;

	/** The size it is drawn with; it runs after the size of the content shown. */
	RSpring width, height;
	int sized;
	float goal_w, goal_h;
	/** Size of the capsule around each content, from the last measure. */
	rvn_vec2_t folded_size, unfolded_size;
	/** Risland_set_notch: it may live in the camera housing of the screen. */
	int notch_wanted;
	/** It lives there now: the size of the housing (0: it does not). */
	float notch_w, notch_h;
	/** Folded in the housing: from the left of the folded content to the middle of the camera, and from there to its right (last arrange). */
	float ear_left, ear_right;
	Rid timer;
	RIslandChangedCallback callback;
	void* user_data;
} rvn_island_t;

/** Islands that are unfolded, to fold the pinned ones when the user presses elsewhere. */
static struct {
	Rid* ids;
	uint32_t count;
	uint32_t cap;
} g_open_islands;

static rvn_widget_t* rvn_island_content(const rvn_widget_t* widget, int expanded) {
	uint32_t index = expanded ? 1u : 0u;
	/* with a single child, the island is an empty capsule that unfolds into it */
	if (widget->child_count == 1) return expanded ? widget->children[0] : NULL;
	return index < widget->child_count ? widget->children[index] : NULL;
}

/* In the housing, the margin between the camera and what the ears show, and the width of an ear when it shows nothing. */
#define RVN_ISLAND_NOTCH_GAP 10.0f
#define RVN_ISLAND_NOTCH_EAR 36.0f

/* The Spacer of the folded content that stands for the camera, or NULL. */
static rvn_widget_t* rvn_island_notch_spacer(const rvn_widget_t* widget) {
	rvn_widget_t* content = rvn_island_content(widget, R_FALSE);
	uint32_t i;
	if (!content) return NULL;
	for (i = 0; i < content->child_count; i++) {
		rvn_widget_t* child = content->children[i];
		if (child->cls->type == R_WIDGET_SPACER && !rvn_widget_collapsed(child)) return child;
	}
	return NULL;
}

/* Size of the capsule around one of its contents. */
static rvn_vec2_t rvn_island_size_for(const rvn_widget_t* widget, int expanded) {
	const rvn_island_t* island = (const rvn_island_t*)widget;
	rvn_widget_t* content = rvn_island_content(widget, expanded);
	rvn_vec2_t size = content && !rvn_widget_collapsed(content) ? rvn_widget_measure_slot(content) : rvn_vec2(0.0f, 0.0f);
	if (island->notch_w > 0.0f) {
		/* in the housing: as tall as it is when folded, the content under it when unfolded, always wider than it */
		if (expanded) {
			size.x = rvn_maxf(rvn_maxf(size.x + 36.0f, 180.0f), island->notch_w + 2.0f * RVN_ISLAND_NOTCH_EAR + 40.0f);
			size.y = rvn_maxf(size.y + 30.0f, 64.0f) + island->notch_h;
		} else {
			float ear = rvn_maxf(rvn_maxf(island->ear_left, island->ear_right), island->notch_w * 0.5f + RVN_ISLAND_NOTCH_EAR);
			if (!rvn_island_notch_spacer(widget) && content) ear = rvn_maxf(ear, island->notch_w * 0.5f + RVN_ISLAND_NOTCH_GAP + size.x + 14.0f);
			size.x = ear * 2.0f;
			size.y = rvn_maxf(island->notch_h, size.y + 4.0f);
		}
		return size;
	}
	if (expanded) {
		size.x = rvn_maxf(size.x + 36.0f, 180.0f);
		size.y = rvn_maxf(size.y + 30.0f, 64.0f);
	} else {
		size.x = rvn_maxf(size.x + 28.0f, 96.0f);
		size.y = rvn_maxf(size.y + 10.0f, 30.0f);
	}
	return size;
}

static void rvn_island_stop_timer(rvn_island_t* island) {
	if (island->timer != R_INVALID_ID && Rtimer_is_valid(island->timer)) Rtimer_destroy(island->timer);
	island->timer = R_INVALID_ID;
}

static void rvn_island_track(rvn_widget_t* widget, int open) {
	uint32_t i;
	for (i = 0; i < g_open_islands.count; i++) {
		if (g_open_islands.ids[i] != widget->id) continue;
		if (!open) g_open_islands.ids[i] = g_open_islands.ids[--g_open_islands.count];
		return;
	}
	if (!open) return;
	RVN_ARRAY_RESERVE(g_open_islands.ids, g_open_islands.cap, g_open_islands.count + 1);
	g_open_islands.ids[g_open_islands.count++] = widget->id;
}

static void rvn_island_set(rvn_widget_t* widget, int expanded, int notify) {
	rvn_island_t* island = (rvn_island_t*)widget;
	rvn_widget_t* leaving;
	expanded = expanded != 0;
	if (island->expanded == expanded) return;
	/* what was inside stops taking the mouse and the keyboard; it fades away while the capsule changes */
	leaving = rvn_island_content(widget, island->expanded);
	if (leaving && widget->window) rvn_window_forget_widget(widget->window, leaving);
	island->expanded = expanded;
	rvn_island_track(widget, expanded);
	rvn_widget_invalidate_layout(widget);
	if (notify) {
		if (island->callback) island->callback(widget->id, expanded, island->user_data);
		if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
	}
}

/* Is a menu or a list opened from inside the island still there? The pointer is then busy with the island. */
static int rvn_island_has_popup(const rvn_widget_t* widget) {
	const rvn_window_t* window = widget->window;
	uint32_t i;
	if (!window) return R_FALSE;
	for (i = 0; i < window->popup_count; i++) {
		const rvn_widget_t* owner = window->popups[i].owner;
		if (owner && rvn_widget_is_inside(owner, widget)) return R_TRUE;
	}
	return R_FALSE;
}

static void rvn_island_timer(Rid timer, void* user_data);

/* Looks at what keeps the island unfolded and unfolds it, folds it, or waits. */
static void rvn_island_update(rvn_widget_t* widget, int notify) {
	rvn_island_t* island = (rvn_island_t*)widget;
	double now = rvn_clock_seconds();
	int under = (widget->flags & RVN_WF_HOVERED) != 0;
	int held, hovered, busy;
	double wait = -1.0;

	if (!under) island->hover_blocked = R_FALSE;
	if (island->hold_until > 0.0 && now + 0.002 >= island->hold_until) island->hold_until = 0.0;
	held = island->hold_until > 0.0;
	/* the pointer unfolds it when it may; once unfolded, it always keeps it so */
	hovered = under && !island->hover_blocked && (!island->no_hover || island->expanded);
	busy = island->expanded && rvn_island_has_popup(widget);

	rvn_island_stop_timer(island);
	if (island->pinned || held || hovered || busy) {
		rvn_island_set(widget, R_TRUE, notify);
		if (!rvn_widget_alive(widget)) return;
		if (!island->pinned && !hovered) {
			if (busy) wait = RVN_ISLAND_LEAVE_DELAY;
			if (held && (wait < 0.0 || island->hold_until - now < wait)) wait = island->hold_until - now;
		}
	} else if (island->expanded) {
		/* nothing keeps it: it folds, a moment after the pointer left */
		double fold_at = island->left_at > 0.0 ? island->left_at + RVN_ISLAND_LEAVE_DELAY : 0.0;
		if (now + 0.002 >= fold_at) {
			island->left_at = 0.0;
			rvn_island_set(widget, R_FALSE, notify);
		} else {
			wait = fold_at - now;
		}
	}
	if (wait >= 0.0 && rvn_widget_alive(widget)) {
		island->timer = Rtimer_create(rvn_maxf((float)wait, 0.001f), R_FALSE, rvn_island_timer, (void*)(uintptr_t)widget->id);
	}
}

static void rvn_island_timer(Rid timer, void* user_data) {
	rvn_widget_t* widget = rvn_widget_get((Rid)(uintptr_t)user_data);
	RVN_UNUSED(timer);
	if (!widget || widget->cls->type != R_WIDGET_ISLAND) return;
	((rvn_island_t*)widget)->timer = R_INVALID_ID;
	rvn_island_update(widget, R_TRUE);
}

/* Folds it now, whatever kept it unfolded. */
static void rvn_island_fold(rvn_widget_t* widget, int notify) {
	rvn_island_t* island = (rvn_island_t*)widget;
	island->pinned = R_FALSE;
	island->hold_until = 0.0;
	island->left_at = 0.0;
	/* (still under the pointer: it must leave before it unfolds it again) */
	island->hover_blocked = (widget->flags & RVN_WF_HOVERED) != 0;
	rvn_island_stop_timer(island);
	rvn_island_set(widget, R_FALSE, notify);
}

void rvn_islands_pressed(rvn_window_t* window, rvn_widget_t* hit, int on_popup) {
	uint32_t i = 0;
	/* (a press in a menu is not a press outside of the island the menu belongs to) */
	if (on_popup) return;
	while (i < g_open_islands.count) {
		rvn_widget_t* widget = rvn_widget_get(g_open_islands.ids[i]);
		if (!widget) {
			g_open_islands.ids[i] = g_open_islands.ids[--g_open_islands.count];
			continue;
		}
		i++;
		/* (an island in the camera housing is in an overlay of its window: a press in the window is a press elsewhere) */
		if ((widget->window != window && !(widget->window && widget->window->notch_owner == window)) ||
			(hit && rvn_widget_is_inside(hit, widget))) continue;
		/* a press anywhere else puts it away, as it closes a menu */
		rvn_island_fold(widget, R_TRUE);
		i = 0;
		if (window->dead) return;
	}
}

void rvn_islands_shutdown(void) {
	rvn_free(g_open_islands.ids);
	memset(&g_open_islands, 0, sizeof(g_open_islands));
}

static void rvn_island_init(rvn_widget_t* widget) {
	((rvn_island_t*)widget)->timer = R_INVALID_ID;
	/* dark whatever the theme of the application */
	rvn_widget_set_theme(widget, rvn_theme_dark_twin(), R_TRUE);
}

static void rvn_island_destroy(rvn_widget_t* widget) {
	rvn_island_track(widget, R_FALSE);
	/* (at shutdown the timers are released with the library) */
	if (g_rvn.initialized) rvn_island_stop_timer((rvn_island_t*)widget);
}

static rvn_vec2_t rvn_island_measure(rvn_widget_t* widget) {
	rvn_island_t* island = (rvn_island_t*)widget;
	rvn_vec2_t folded = rvn_island_size_for(widget, R_FALSE);
	rvn_vec2_t unfolded = rvn_island_size_for(widget, R_TRUE);
	rvn_vec2_t target = island->expanded ? unfolded : folded;

	island->folded_size = folded;
	island->unfolded_size = unfolded;
	if (!island->sized) {
		rvn_spring_set(&island->width, target.x);
		rvn_spring_set(&island->height, target.y);
		island->sized = R_TRUE;
		island->goal_w = target.x;
		island->goal_h = target.y;
	} else if (island->goal_w != target.x || island->goal_h != target.y) {
		/* (asked once per target: each request gives the window another layout pass) */
		island->goal_w = target.x;
		island->goal_h = target.y;
		/* it springs open and settles shut */
		rvn_widget_animate_layout(widget, &island->width, target.x, island->expanded ? R_MOTION_BOUNCY : R_MOTION_SMOOTH);
		rvn_widget_animate_layout(widget, &island->height, target.y, island->expanded ? R_MOTION_BOUNCY : R_MOTION_SMOOTH);
	}
	return rvn_vec2(rvn_maxf(island->width.value, rvn_minf(folded.x, unfolded.x)), rvn_maxf(island->height.value, rvn_minf(folded.y, unfolded.y)));
}

/*
 * Folded in the housing: the folded content at its own width, placed so that
 * the middle of its Spacer is the middle of the camera (or on the left of the
 * camera when it has none). How far it goes on each side gives the ears.
 */
static void rvn_island_arrange_ears(rvn_widget_t* widget, rvn_widget_t* child) {
	rvn_island_t* island = (rvn_island_t*)widget;
	rvn_vec2_t natural = rvn_widget_measure_slot(child);
	rvn_widget_t* spacer = rvn_island_notch_spacer(widget);
	float center = widget->rect.w * 0.5f, y = floorf((island->folded_size.y - natural.y) * 0.5f), middle, left, right;
	if (spacer) {
		/* placed once at 0 to learn where its Spacer falls, then for good */
		rvn_widget_arrange_slot(child, rvn_rect(0.0f, y, natural.x, natural.y));
		middle = child->rect.x + spacer->rect.x + spacer->rect.w * 0.5f;
	} else {
		middle = natural.x + island->notch_w * 0.5f + RVN_ISLAND_NOTCH_GAP;
	}
	rvn_widget_arrange_slot(child, rvn_rect(floorf(center - middle), y, natural.x, natural.y));
	left = middle + 14.0f;
	right = spacer ? natural.x - middle + 14.0f : 0.0f;
	if (fabsf(left - island->ear_left) > 0.5f || fabsf(right - island->ear_right) > 0.5f) {
		island->ear_left = left;
		island->ear_right = right;
		/* the capsule takes the size of its ears: one more pass of layout */
		rvn_widget_invalidate_layout(widget);
	}
}

static void rvn_island_arrange(rvn_widget_t* widget) {
	rvn_island_t* island = (rvn_island_t*)widget;
	uint32_t i;
	if (island->notch_w > 0.0f) {
		for (i = 0; i < widget->child_count && i < 2; i++) {
			rvn_widget_t* child = widget->children[i];
			int expanded = widget->child_count == 1 || i == 1;
			if (rvn_widget_collapsed(child)) continue;
			if (!expanded) {
				rvn_island_arrange_ears(widget, child);
			} else {
				/* unfolded: under the camera */
				rvn_vec2_t size = rvn_island_size_for(widget, R_TRUE);
				rvn_widget_arrange_slot(child, rvn_rect(floorf((widget->rect.w - size.x) * 0.5f) + 18.0f, island->notch_h + 15.0f,
					rvn_maxf(0.0f, size.x - 36.0f), rvn_maxf(0.0f, size.y - island->notch_h - 30.0f)));
			}
		}
		return;
	}
	/* each content keeps the place it has in its own capsule, hung from the top and centered: the capsule opens around it */
	for (i = 0; i < widget->child_count && i < 2; i++) {
		rvn_widget_t* child = widget->children[i];
		int expanded = widget->child_count == 1 || i == 1;
		rvn_vec2_t size = rvn_island_size_for(widget, expanded);
		float pad_x = expanded ? 18.0f : 14.0f, pad_y = expanded ? 15.0f : 5.0f;
		if (rvn_widget_collapsed(child)) continue;
		rvn_widget_arrange_slot(child, rvn_rect(floorf((widget->rect.w - size.x) * 0.5f) + pad_x, pad_y,
			rvn_maxf(0.0f, size.x - pad_x * 2.0f), rvn_maxf(0.0f, size.y - pad_y * 2.0f)));
	}
}

static int rvn_island_child_active(rvn_widget_t* widget, rvn_widget_t* child) {
	return rvn_island_content(widget, ((rvn_island_t*)widget)->expanded) == child;
}

static float rvn_smoothstep(float from, float to, float x) {
	float t = rvn_clampf((x - from) / (to - from), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

/* One of the two contents, faded and scaled around a point of the island. */
static void rvn_island_paint_content(rvn_widget_t* child, const RPainter* painter, float opacity, float scale, float pivot_x, float pivot_y) {
	RPainter inner = *painter;
	uint32_t first_instance = painter->list->instance_count, first_command = painter->list->command_count;
	float px = (painter->ox + pivot_x) * painter->scale, py = (painter->oy + pivot_y) * painter->scale;
	if (opacity <= 0.004f || rvn_box_empty(painter->clip)) return;
	inner.opacity *= opacity;
	inner.clip_depth = 0;
	if (scale != 1.0f) {
		/* it is painted at its full size and scaled afterwards: what the capsule lets through is that much larger for now */
		inner.clip = rvn_box(px + (painter->clip.x0 - px) / scale, py + (painter->clip.y0 - py) / scale,
			px + (painter->clip.x1 - px) / scale, py + (painter->clip.y1 - py) / scale);
	}
	rvn_widget_paint(child, &inner);
	if (scale != 1.0f) {
		rvn_drawlist_transform(painter->list, first_instance, first_command, scale, scale, px, py, 0.0f, 0.0f, painter->clip);
	}
}

static void rvn_island_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_island_t* island = (rvn_island_t*)widget;
	const rvn_theme_t* theme = rvn_widget_theme(widget);
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	rvn_vec2_t folded = island->folded_size, unfolded = island->unfolded_size;
	float span = (unfolded.x - folded.x) + (unfolded.y - folded.y);
	/* how far the capsule has gone from its folded size to its unfolded one (past 1 while it bounces) */
	float t = span > 1.0f ? ((rect.w - folded.x) + (rect.h - folded.y)) / span : (island->expanded ? 1.0f : 0.0f);
	float open = rvn_clampf(t, 0.0f, 1.0f);
	float hover = rvn_widget_hover(widget, painter, (widget->flags & RVN_WF_HOVERED) && painter->enabled && !island->expanded);
	/* a capsule when folded, a rounded panel when unfolded */
	float radius = rvn_minf(rect.h * 0.5f, 15.0f + 13.0f * open);
	float radii[4];
	rvn_widget_t* small = rvn_island_content(widget, R_FALSE);
	rvn_widget_t* large = rvn_island_content(widget, R_TRUE);
	RMaterial material = rvn_widget_material(widget);

	radii[0] = radii[1] = island->attached || island->notch_w > 0.0f ? 0.0f : radius;
	radii[2] = radii[3] = radius;

	if (island->notch_w > 0.0f) {
		/* in the housing: black as the housing itself, square at the top, no rim (it would draw around the camera) */
		rvn_paint_shadow(painter, rvn_rect(rect.x, rect.y + 4.0f + 8.0f * open, rect.w, rect.h), radius, 10.0f + 26.0f * open,
			rvn_rgba(0.0f, 0.0f, 0.0f, 0.22f + 0.2f * open));
		rvn_paint_box4(painter, rect, radii, rvn_rgba(0.0f, 0.0f, 0.0f, 1.0f), 0.0f, rvn_rgba(0.0f, 0.0f, 0.0f, 0.0f));
	} else if (!theme->dark) {
		/* it was given a light theme: a panel of that theme */
		if (material == R_MATERIAL_SOLID) {
			rvn_paint_shadow(painter, rvn_rect(rect.x, rect.y + 3.0f + 5.0f * open, rect.w, rect.h), radius, 12.0f + 18.0f * open, rvn_widget_color(widget, R_COLOR_SHADOW));
			rvn_paint_box4(painter, rect, radii, rvn_widget_color(widget, R_COLOR_POPUP), 1.0f, rvn_widget_color(widget, R_COLOR_POPUP_BORDER));
		} else {
			rvn_glass_t glass;
			rvn_glass_style(widget, RVN_SURFACE_POPUP, hover, 0.0f, &glass);
			glass.shadow = 12.0f + 14.0f * open;
			rvn_paint_glass(painter, rect, radii, &glass);
		}
	} else if (material == R_MATERIAL_SOLID) {
		rvn_color_t fill = rvn_widget_has_color(widget, R_COLOR_PANEL) ? rvn_widget_color(widget, R_COLOR_PANEL) : rvn_hex(0x08080A, 252);
		/* two shadows: a tight one that sets it on the window, a wide one that lifts it when it opens */
		rvn_paint_shadow(painter, rvn_rect(rect.x, rect.y + 1.0f, rect.w, rect.h), radius, 5.0f, rvn_rgba(0.0f, 0.0f, 0.0f, 0.30f));
		rvn_paint_shadow(painter, rvn_rect(rect.x, rect.y + 4.0f + 8.0f * open, rect.w, rect.h), radius, 14.0f + 26.0f * open,
			rvn_rgba(0.0f, 0.0f, 0.0f, 0.26f + 0.16f * open));
		rvn_paint_box4(painter, rect, radii, fill, 1.0f, rvn_rgba(1.0f, 1.0f, 1.0f, 0.085f + 0.05f * rvn_clampf(hover, 0.0f, 1.0f)));
	} else {
		/* smoked glass: the window shows through, darkened enough for white text whatever is behind */
		rvn_glass_t glass;
		rvn_color_t tint = rvn_widget_has_color(widget, R_COLOR_PANEL) ? rvn_widget_color(widget, R_COLOR_PANEL) : rvn_hex(0x07070B, 184);
		memset(&glass, 0, sizeof(glass));
		if (material == R_MATERIAL_TINTED_GLASS) tint = rvn_color_mix(tint, rvn_color_fade(rvn_widget_color(widget, R_COLOR_ACCENT), 0.7f), 0.45f);
		glass.tint = tint;
		glass.frost = material == R_MATERIAL_GLASS ? 0.3f : 1.0f;
		glass.refraction = material == R_MATERIAL_GLASS ? 18.0f : 12.0f;
		glass.bevel = 7.0f + 3.0f * open;
		glass.saturation = 1.6f;
		glass.rim = rvn_rgba(1.0f, 1.0f, 1.0f, 0.34f + 0.12f * rvn_clampf(hover, 0.0f, 1.0f));
		glass.glow = 0.02f * rvn_clampf(hover, 0.0f, 1.0f);
		glass.shadow = 12.0f + 20.0f * open;
		glass.shadow_color = rvn_rgba(0.0f, 0.0f, 0.0f, 0.34f + 0.1f * open);
		rvn_glass_tune(widget, &glass);
		rvn_paint_glass(painter, rect, radii, &glass);
	}

	/* the contents, kept inside the capsule: the largest rectangle that stays clear of its round corners */
	{
		RPainter inner = *painter;
		float inset = radius * 0.2929f;
		float top = island->attached || island->notch_w > 0.0f ? 0.0f : inset;
		float center = rect.w * 0.5f;
		rvn_paint_push_clip(&inner, rvn_rect(inset, top, rect.w - inset * 2.0f, rect.h - top - inset));
		/* the small one leaves early, growing a little as if it were coming closer */
		if (small) {
			rvn_island_paint_content(small, &inner, 1.0f - rvn_smoothstep(0.0f, 0.3f, t), 1.0f + 0.16f * open, center, folded.y * 0.5f);
		}
		/* the large one is hung from the top and always fits: it grows with the capsule, and shows as it gets readable */
		if (large) {
			float fit = rvn_minf(rect.w / rvn_maxf(unfolded.x, 1.0f), rect.h / rvn_maxf(unfolded.y, 1.0f));
			rvn_island_paint_content(large, &inner, rvn_smoothstep(0.22f, 0.8f, t), rvn_clampf(fit, 0.3f, 1.0f), center, 0.0f);
		}
	}

	/* under the pointer, a folded island swells a little: it is about to open (in the housing, only sideways: it stays against the top of the screen) */
	widget->xform.grow_x = 0.035f * hover;
	widget->xform.grow_y = island->notch_w > 0.0f ? 0.0f : 0.06f * hover;
}

static int rvn_island_event(rvn_widget_t* widget, const REvent* event) {
	rvn_island_t* island = (rvn_island_t*)widget;
	switch (event->type) {
		case R_EVENT_MOUSE_ENTER:
			island->left_at = 0.0;
			rvn_island_update(widget, R_TRUE);
			if (rvn_widget_alive(widget)) rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_LEAVE:
			island->left_at = rvn_clock_seconds();
			rvn_island_update(widget, R_TRUE);
			if (rvn_widget_alive(widget)) rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			/* a press on the island itself, not on a control inside it */
			if (event->button != R_MOUSE_LEFT || event->target != widget->id) return event->target == widget->id;
			if (island->no_click) return R_TRUE;
			if (island->expanded && island->pinned) {
				rvn_island_fold(widget, R_TRUE);
			} else {
				/* folded: it unfolds; unfolded by the pointer or for a while: it now stays */
				island->pinned = R_TRUE;
				island->hover_blocked = R_FALSE;
				rvn_island_update(widget, R_TRUE);
			}
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static int rvn_island_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_island_t* island = (rvn_island_t*)widget;
	int flag;
	if (RVN_ATTR("Theme")) {
		/* its own theme, or "Default" for the one of the window instead of the dark one */
		const rvn_theme_t* theme = NULL;
		if (rvn_stricmp(value, "Dark twin") == 0 || rvn_stricmp(value, "Auto") == 0 || !value[0]) {
			rvn_widget_set_theme(widget, rvn_theme_dark_twin(), R_TRUE);
			return R_TRUE;
		}
		if (rvn_stricmp(value, "Default") != 0 && rvn_stricmp(value, "Inherit") != 0) {
			theme = rvn_xml_theme(value);
			if (!theme) return R_FALSE;
		}
		rvn_widget_set_theme(widget, theme, theme != NULL);
		return R_TRUE;
	}
	if (!rvn_parse_bool(value, &flag)) return R_FALSE;
	if (RVN_ATTR("Expanded")) {
		if (flag) {
			island->pinned = R_TRUE;
			rvn_island_update(widget, R_FALSE);
		} else {
			rvn_island_fold(widget, R_FALSE);
		}
	} else if (RVN_ATTR("ExpandOnHover")) {
		island->no_hover = !flag;
	} else if (RVN_ATTR("ExpandOnClick")) {
		island->no_click = !flag;
	} else if (RVN_ATTR("Attached")) {
		island->attached = flag;
		rvn_widget_invalidate_paint(widget);
	} else if (RVN_ATTR("Notch")) {
		island->notch_wanted = flag;
		if (widget->window) rvn_window_notch_changed(widget->window);
	} else {
		return R_FALSE;
	}
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_island = {
	.type = R_WIDGET_ISLAND,
	.name = "Island",
	.size = sizeof(rvn_island_t),
	.flags = RVN_CF_CLIP | RVN_CF_SURFACE | RVN_CF_OWN_CHILDREN,
	.init = rvn_island_init,
	.destroy = rvn_island_destroy,
	.measure = rvn_island_measure,
	.arrange = rvn_island_arrange,
	.paint = rvn_island_paint,
	.event = rvn_island_event,
	.set_attribute = rvn_island_set_attribute,
	.child_active = rvn_island_child_active
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

Rid Risland_create(void) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_ISLAND);
	return widget ? widget->id : R_INVALID_ID;
}

void Risland_set_expanded(Rid id, int expanded) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_ISLAND);
	if (expanded) {
		((rvn_island_t*)widget)->pinned = R_TRUE;
		((rvn_island_t*)widget)->hover_blocked = R_FALSE;
		rvn_island_update(widget, R_FALSE);
	} else {
		rvn_island_fold(widget, R_FALSE);
	}
}

int Risland_is_expanded(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_ISLAND, R_FALSE);
	return ((rvn_island_t*)widget)->expanded;
}

void Risland_expand_for(Rid id, double seconds) {
	rvn_island_t* island;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_ISLAND);
	island = (rvn_island_t*)widget;
	island->hold_until = rvn_clock_seconds() + (seconds > 0.0 ? seconds : 0.0);
	island->hover_blocked = R_FALSE;
	/* (when its time is over it folds a moment after the pointer left, or at once if it is not there) */
	island->left_at = 0.0;
	rvn_island_update(widget, R_FALSE);
}

void Risland_set_expand_on_hover(Rid id, int enabled) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_ISLAND);
	((rvn_island_t*)widget)->no_hover = !enabled;
}

void Risland_set_expand_on_click(Rid id, int enabled) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_ISLAND);
	((rvn_island_t*)widget)->no_click = !enabled;
}

void Risland_set_attached(Rid id, int attached) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_ISLAND);
	((rvn_island_t*)widget)->attached = attached != 0;
	rvn_widget_invalidate_paint(widget);
}

void Risland_set_notch(Rid id, int enabled) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_ISLAND);
	((rvn_island_t*)widget)->notch_wanted = enabled != 0;
	if (widget->window) rvn_window_notch_changed(widget->window);
}

int Risland_get_notch(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_ISLAND, R_FALSE);
	return ((rvn_island_t*)widget)->notch_wanted;
}

int Risland_is_in_notch(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_ISLAND, R_FALSE);
	return ((rvn_island_t*)widget)->notch_w > 0.0f;
}

/* ---- for widget/window.c ---- */

int rvn_island_wants_notch(const rvn_widget_t* widget) {
	return widget && widget->cls->type == R_WIDGET_ISLAND && ((const rvn_island_t*)widget)->notch_wanted;
}

void rvn_island_set_notch(rvn_widget_t* widget, float width, float height) {
	rvn_island_t* island = (rvn_island_t*)widget;
	rvn_widget_t* spacer;
	if (!widget || widget->cls->type != R_WIDGET_ISLAND) return;
	if (width <= 0.0f || height <= 0.0f) width = height = 0.0f;
	if (fabsf(island->notch_w - width) < 0.01f && fabsf(island->notch_h - height) < 0.01f) return;
	island->notch_w = width;
	island->notch_h = height;
	island->ear_left = island->ear_right = 0.0f;
	/* the Spacer of the folded content is the room of the camera, and nothing outside of the housing */
	spacer = rvn_island_notch_spacer(widget);
	if (spacer) Rspacer_set_size(spacer->id, width > 0.0f ? width + 2.0f * RVN_ISLAND_NOTCH_GAP : 0.0f, 0.0f);
	/* the sizes change at once: no spring from the capsule in the window to the one in the housing */
	island->sized = R_FALSE;
	rvn_widget_invalidate_layout(widget);
}

rvn_vec2_t rvn_island_extent(const rvn_widget_t* widget) {
	const rvn_island_t* island = (const rvn_island_t*)widget;
	if (!widget || widget->cls->type != R_WIDGET_ISLAND) return rvn_vec2(0.0f, 0.0f);
	return rvn_vec2(rvn_maxf(island->folded_size.x, island->unfolded_size.x), rvn_maxf(island->folded_size.y, island->unfolded_size.y));
}

void Risland_set_changed_callback(Rid id, RIslandChangedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_ISLAND);
	((rvn_island_t*)widget)->callback = callback;
	((rvn_island_t*)widget)->user_data = user_data;
}
