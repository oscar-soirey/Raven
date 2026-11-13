/**
 * Raven - render/draw.c
 * Draw list storage and painter primitives.
 */

#include "draw.h"
#include "font.h"
#include "renderer.h"
#include "../core/context.h"

/* ---------------------------------------------------------------------------
 *  Draw list
 * ------------------------------------------------------------------------- */

void rvn_drawlist_init(rvn_drawlist_t* list) {
	memset(list, 0, sizeof(*list));
}

void rvn_drawlist_free(rvn_drawlist_t* list) {
	rvn_free(list->instances);
	rvn_free(list->commands);
	memset(list, 0, sizeof(*list));
}

void rvn_drawlist_reset(rvn_drawlist_t* list) {
	list->instance_count = 0;
	list->command_count = 0;
}

static RDrawCommand* rvn_drawlist_new_command(rvn_drawlist_t* list, Rid texture) {
	RDrawCommand* command;
	RVN_ARRAY_RESERVE(list->commands, list->command_cap, list->command_count + 1);
	command = &list->commands[list->command_count++];
	memset(command, 0, sizeof(*command));
	command->type = R_DRAW_INSTANCES;
	command->texture = texture;
	command->viewport = R_INVALID_ID;
	command->shader = R_INVALID_ID;
	command->first_instance = list->instance_count;
	return command;
}

RDrawInstance* rvn_drawlist_add(rvn_drawlist_t* list, Rid texture) {
	RDrawCommand* command = list->command_count ? &list->commands[list->command_count - 1] : NULL;

	if (!command || command->type != R_DRAW_INSTANCES) {
		command = rvn_drawlist_new_command(list, texture);
	} else if (texture != R_INVALID_ID && command->texture != texture) {
		/* a batch that only holds untextured instances can adopt the texture */
		if (command->texture == R_INVALID_ID) command->texture = texture;
		else command = rvn_drawlist_new_command(list, texture);
	}
	RVN_ARRAY_RESERVE(list->instances, list->instance_cap, list->instance_count + 1);
	command->instance_count++;
	return &list->instances[list->instance_count++];
}

RDrawInstance* rvn_drawlist_add_erase(rvn_drawlist_t* list) {
	/* a command of its own: it is drawn with another blending */
	RDrawCommand* command = rvn_drawlist_new_command(list, R_INVALID_ID);
	command->type = R_DRAW_ERASE;
	RVN_ARRAY_RESERVE(list->instances, list->instance_cap, list->instance_count + 1);
	command->instance_count = 1;
	return &list->instances[list->instance_count++];
}

RDrawInstance* rvn_drawlist_add_shader(rvn_drawlist_t* list, Rid shader, Rid texture) {
	RDrawCommand* command = list->command_count ? &list->commands[list->command_count - 1] : NULL;
	/* the effects that follow one another with the same shader and texture are one draw */
	if (!command || command->type != R_DRAW_SHADER || command->shader != shader || command->texture != texture) {
		command = rvn_drawlist_new_command(list, texture);
		command->type = R_DRAW_SHADER;
		command->shader = shader;
	}
	RVN_ARRAY_RESERVE(list->instances, list->instance_cap, list->instance_count + 1);
	command->instance_count++;
	return &list->instances[list->instance_count++];
}

void rvn_drawlist_add_callback(rvn_drawlist_t* list, const RDrawCommand* callback) {
	RDrawCommand* command;
	RVN_ARRAY_RESERVE(list->commands, list->command_cap, list->command_count + 1);
	command = &list->commands[list->command_count++];
	*command = *callback;
	command->type = R_DRAW_CALLBACK;
	command->first_instance = list->instance_count;
	command->instance_count = 0;
}

void rvn_drawlist_add_backdrop(rvn_drawlist_t* list, rvn_box_t area) {
	RDrawCommand* command;
	RVN_ARRAY_RESERVE(list->commands, list->command_cap, list->command_count + 1);
	command = &list->commands[list->command_count++];
	memset(command, 0, sizeof(*command));
	command->type = R_DRAW_BACKDROP;
	command->texture = R_INVALID_ID;
	command->viewport = R_INVALID_ID;
	command->first_instance = list->instance_count;
	command->rect[0] = (int)floorf(area.x0);
	command->rect[1] = (int)floorf(area.y0);
	command->rect[2] = (int)ceilf(area.x1 - area.x0);
	command->rect[3] = (int)ceilf(area.y1 - area.y0);
}

void rvn_drawlist_append(rvn_drawlist_t* list, const rvn_drawlist_t* source) {
	uint32_t base = list->instance_count;
	if (source->instance_count) {
		RVN_ARRAY_RESERVE(list->instances, list->instance_cap, list->instance_count + source->instance_count);
		memcpy(list->instances + list->instance_count, source->instances,
			sizeof(RDrawInstance) * source->instance_count);
		list->instance_count += source->instance_count;
	}
	rvn_drawlist_append_commands(list, source, base);
}

void rvn_drawlist_append_commands(rvn_drawlist_t* list, const rvn_drawlist_t* source, uint32_t base) {
	uint32_t i;
	for (i = 0; i < source->command_count; i++) {
		const RDrawCommand* in = &source->commands[i];
		RDrawCommand* last = list->command_count ? &list->commands[list->command_count - 1] : NULL;
		if (in->type == R_DRAW_INSTANCES && in->instance_count == 0) continue;
		if (in->type == R_DRAW_INSTANCES && last && last->type == R_DRAW_INSTANCES &&
			last->first_instance + last->instance_count == base + in->first_instance &&
			(last->texture == in->texture || last->texture == R_INVALID_ID || in->texture == R_INVALID_ID)) {
			if (last->texture == R_INVALID_ID) last->texture = in->texture;
			last->instance_count += in->instance_count;
			continue;
		}
		RVN_ARRAY_RESERVE(list->commands, list->command_cap, list->command_count + 1);
		list->commands[list->command_count] = *in;
		list->commands[list->command_count].first_instance = base + in->first_instance;
		list->command_count++;
	}
}

/* ---------------------------------------------------------------------------
 *  Backdrop captures
 * ------------------------------------------------------------------------- */

/*
 * Where the frame differs from its captured copy: what was drawn since the
 * last capture of that place, as a few rectangles. Exactness does not
 * matter: a rectangle that is too large only costs a capture that could have
 * been avoided, or one that is larger than needed.
 */
#define RVN_DIRTY_MAX 16
#define RVN_DIRTY_NEAR 8.0f

typedef struct rvn_dirty_t {
	rvn_box_t boxes[RVN_DIRTY_MAX];
	int count;
	/* box that received the previous rectangle: runs of glyphs and rows extend it */
	int last;
} rvn_dirty_t;

static rvn_box_t rvn_box_union(rvn_box_t a, rvn_box_t b) {
	return rvn_box(rvn_minf(a.x0, b.x0), rvn_minf(a.y0, b.y0), rvn_maxf(a.x1, b.x1), rvn_maxf(a.y1, b.y1));
}

static float rvn_box_area(rvn_box_t box) {
	return (box.x1 - box.x0) * (box.y1 - box.y0);
}

static void rvn_dirty_add(rvn_dirty_t* dirty, rvn_box_t box) {
	int i, best = 0;
	float best_growth = 0.0f;

	if (rvn_box_empty(box)) return;
	if (dirty->last >= 0 && dirty->last < dirty->count) {
		rvn_box_t* last = &dirty->boxes[dirty->last];
		if (box.x0 <= last->x1 + RVN_DIRTY_NEAR && box.x1 >= last->x0 - RVN_DIRTY_NEAR &&
			box.y0 <= last->y1 + RVN_DIRTY_NEAR && box.y1 >= last->y0 - RVN_DIRTY_NEAR) {
			*last = rvn_box_union(*last, box);
			return;
		}
	}
	for (i = 0; i < dirty->count; i++) {
		const rvn_box_t* other = &dirty->boxes[i];
		if (box.x0 >= other->x0 && box.y0 >= other->y0 && box.x1 <= other->x1 && box.y1 <= other->y1) {
			dirty->last = i;
			return;
		}
	}
	if (dirty->count < RVN_DIRTY_MAX) {
		dirty->last = dirty->count;
		dirty->boxes[dirty->count++] = box;
		return;
	}
	/* full: grow the rectangle that grows the least */
	for (i = 0; i < dirty->count; i++) {
		float growth = rvn_box_area(rvn_box_union(dirty->boxes[i], box)) - rvn_box_area(dirty->boxes[i]);
		if (i == 0 || growth < best_growth) {
			best = i;
			best_growth = growth;
		}
	}
	dirty->boxes[best] = rvn_box_union(dirty->boxes[best], box);
	dirty->last = best;
}

/* Pixels an instance can touch. */
static rvn_box_t rvn_instance_bounds(const RDrawInstance* instance) {
	rvn_box_t box = rvn_box(instance->rect[0], instance->rect[1], instance->rect[2], instance->rect[3]);
	float grow = 1.0f;
	switch (instance->flags & R_PRIM_MASK) {
		case R_PRIM_SHADOW:
			grow = instance->border_width + 1.0f;
			break;
		case R_PRIM_LINE:
			box = rvn_box(rvn_minf(instance->rect[0], instance->rect[2]), rvn_minf(instance->rect[1], instance->rect[3]),
				rvn_maxf(instance->rect[0], instance->rect[2]), rvn_maxf(instance->rect[1], instance->rect[3]));
			grow = instance->radii[0] * 0.5f + 1.0f;
			break;
		case R_PRIM_TRIANGLE:
			box = rvn_box(rvn_minf(rvn_minf(instance->rect[0], instance->rect[2]), instance->radii[0]),
				rvn_minf(rvn_minf(instance->rect[1], instance->rect[3]), instance->radii[1]),
				rvn_maxf(rvn_maxf(instance->rect[0], instance->rect[2]), instance->radii[0]),
				rvn_maxf(rvn_maxf(instance->rect[1], instance->rect[3]), instance->radii[1]));
			break;
		case R_PRIM_CURVE:
			box = rvn_box(rvn_minf(rvn_minf(instance->rect[0], instance->rect[2]), rvn_minf(instance->radii[0], instance->radii[2])),
				rvn_minf(rvn_minf(instance->rect[1], instance->rect[3]), rvn_minf(instance->radii[1], instance->radii[3])),
				rvn_maxf(rvn_maxf(instance->rect[0], instance->rect[2]), rvn_maxf(instance->radii[0], instance->radii[2])),
				rvn_maxf(rvn_maxf(instance->rect[1], instance->rect[3]), rvn_maxf(instance->radii[1], instance->radii[3])));
			grow = instance->border_width * 0.5f + instance->uv[0] + 1.0f;
			break;
		default:
			break;
	}
	box = rvn_box(box.x0 - grow, box.y0 - grow, box.x1 + grow, box.y1 + grow);
	return rvn_box_intersect(box, rvn_box(instance->clip[0], instance->clip[1], instance->clip[2], instance->clip[3]));
}

/*
 * A capture is needed where glass covers something that is not in the copy
 * yet. It then takes the whole of those rectangles, not only the part under
 * the glass: the ten buttons of a glass bar cost one capture of the bar, not
 * ten. Returns R_FALSE when the copy is already up to date under `area`.
 */
static int rvn_dirty_take(rvn_dirty_t* dirty, rvn_box_t area, rvn_box_t* region) {
	int i, found = R_FALSE;
	*region = area;
	for (i = 0; i < dirty->count;) {
		if (rvn_box_overlaps(dirty->boxes[i], area)) {
			*region = rvn_box_union(*region, dirty->boxes[i]);
			dirty->boxes[i] = dirty->boxes[--dirty->count];
			found = R_TRUE;
		} else {
			i++;
		}
	}
	if (found) dirty->last = -1;
	return found;
}

uint32_t rvn_drawlist_resolve_backdrops(rvn_drawlist_t* list, rvn_box_t frame, uint32_t max) {
	rvn_dirty_t dirty;
	uint32_t captures = 0, kept = 0, i, k;
	int has_backdrop = R_FALSE;

	for (i = 0; i < list->command_count && !has_backdrop; i++) has_backdrop = list->commands[i].type == R_DRAW_BACKDROP;
	if (!has_backdrop) return 0;

	/* nothing of the frame is in the copy yet: the first capture takes it all */
	dirty.boxes[0] = frame;
	dirty.count = 1;
	dirty.last = 0;
	for (i = 0; i < list->command_count; i++) {
		RDrawCommand command = list->commands[i];
		RDrawCommand* previous = kept ? &list->commands[kept - 1] : NULL;

		if (command.type == R_DRAW_BACKDROP) {
			rvn_box_t area = rvn_box((float)command.rect[0], (float)command.rect[1],
				(float)(command.rect[0] + command.rect[2]), (float)(command.rect[1] + command.rect[3]));
			rvn_box_t region;
			if (captures >= max || !rvn_dirty_take(&dirty, area, &region)) continue;
			region = rvn_box_intersect(region, frame);
			command.rect[0] = (int)floorf(region.x0);
			command.rect[1] = (int)floorf(region.y0);
			command.rect[2] = (int)ceilf(region.x1) - command.rect[0];
			command.rect[3] = (int)ceilf(region.y1) - command.rect[1];
			captures++;
		} else if (command.type == R_DRAW_CALLBACK) {
			rvn_dirty_add(&dirty, rvn_box((float)command.clip[0], (float)command.clip[1],
				(float)(command.clip[0] + command.clip[2]), (float)(command.clip[1] + command.clip[3])));
		} else {
			for (k = 0; k < command.instance_count; k++) {
				rvn_dirty_add(&dirty, rvn_instance_bounds(&list->instances[command.first_instance + k]));
			}
			/* two batches that were only separated by a removed capture become one again */
			if (command.type == R_DRAW_INSTANCES && previous && previous->type == R_DRAW_INSTANCES &&
				previous->first_instance + previous->instance_count == command.first_instance &&
				(previous->texture == command.texture || previous->texture == R_INVALID_ID || command.texture == R_INVALID_ID)) {
				if (previous->texture == R_INVALID_ID) previous->texture = command.texture;
				previous->instance_count += command.instance_count;
				continue;
			}
		}
		list->commands[kept++] = command;
	}
	list->command_count = kept;
	return captures;
}

/* ---------------------------------------------------------------------------
 *  Transform of a range
 * ------------------------------------------------------------------------- */

void rvn_drawlist_transform(rvn_drawlist_t* list, uint32_t first_instance, uint32_t first_command,
	float sx, float sy, float pivot_x, float pivot_y, float dx, float dy, rvn_box_t limit) {

	/* lengths without a direction (corner radii, line widths, blur) follow the smaller scale */
	float s = rvn_minf(fabsf(sx), fabsf(sy));
	float tx = pivot_x - pivot_x * sx + dx, ty = pivot_y - pivot_y * sy + dy;
	uint32_t i;

	if (sx == 1.0f && sy == 1.0f && dx == 0.0f && dy == 0.0f) return;
	for (i = first_instance; i < list->instance_count; i++) {
		RDrawInstance* instance = &list->instances[i];
		uint32_t kind = instance->flags & R_PRIM_MASK;
		rvn_box_t clip;

		instance->rect[0] = instance->rect[0] * sx + tx;
		instance->rect[1] = instance->rect[1] * sy + ty;
		instance->rect[2] = instance->rect[2] * sx + tx;
		instance->rect[3] = instance->rect[3] * sy + ty;
		switch (kind) {
			case R_PRIM_LINE:
				/* two points and a width */
				instance->radii[0] *= s;
				break;
			case R_PRIM_TRIANGLE:
				/* the third point is in the radii */
				instance->radii[0] = instance->radii[0] * sx + tx;
				instance->radii[1] = instance->radii[1] * sy + ty;
				break;
			case R_PRIM_CURVE:
				/* the control points are in the radii */
				instance->radii[0] = instance->radii[0] * sx + tx;
				instance->radii[1] = instance->radii[1] * sy + ty;
				instance->radii[2] = instance->radii[2] * sx + tx;
				instance->radii[3] = instance->radii[3] * sy + ty;
				instance->border_width *= s;
				instance->uv[0] *= s;
				break;
			case R_PRIM_BLUR:
				instance->uv[0] *= s;
				instance->radii[0] *= s;
				instance->radii[1] *= s;
				instance->radii[2] *= s;
				instance->radii[3] *= s;
				instance->border_width *= s;
				break;
			case R_PRIM_GLASS:
				instance->uv[1] *= s;
				/* fall through */
			default:
				instance->radii[0] *= s;
				instance->radii[1] *= s;
				instance->radii[2] *= s;
				instance->radii[3] *= s;
				instance->border_width *= s;
				break;
		}
		clip = rvn_box(instance->clip[0] * sx + tx, instance->clip[1] * sy + ty,
			instance->clip[2] * sx + tx, instance->clip[3] * sy + ty);
		clip = rvn_box_intersect(clip, limit);
		instance->clip[0] = clip.x0;
		instance->clip[1] = clip.y0;
		instance->clip[2] = clip.x1;
		instance->clip[3] = clip.y1;
	}
	for (i = first_command; i < list->command_count; i++) {
		RDrawCommand* command = &list->commands[i];
		/* (viewport callbacks draw by themselves and stay where they are) */
		if (command->type == R_DRAW_BACKDROP) {
			float x0 = (float)command->rect[0] * sx + tx, y0 = (float)command->rect[1] * sy + ty;
			float x1 = (float)(command->rect[0] + command->rect[2]) * sx + tx;
			float y1 = (float)(command->rect[1] + command->rect[3]) * sy + ty;
			command->rect[0] = (int)floorf(x0);
			command->rect[1] = (int)floorf(y0);
			command->rect[2] = (int)ceilf(x1) - command->rect[0];
			command->rect[3] = (int)ceilf(y1) - command->rect[1];
		}
	}
}

/* ---------------------------------------------------------------------------
 *  Painter helpers
 * ------------------------------------------------------------------------- */

rvn_box_t rvn_paint_snap(const RPainter* painter, rvn_rect_t rect) {
	rvn_box_t box;
	box.x0 = floorf((painter->ox + rect.x) * painter->scale + 0.5f);
	box.y0 = floorf((painter->oy + rect.y) * painter->scale + 0.5f);
	box.x1 = floorf((painter->ox + rect.x + rect.w) * painter->scale + 0.5f);
	box.y1 = floorf((painter->oy + rect.y + rect.h) * painter->scale + 0.5f);
	return box;
}

/** Thickness in pixels of a logical line width: never thinner than one pixel. */
static float rvn_paint_stroke(const RPainter* painter, float width) {
	float pixels;
	if (width <= 0.0f) return 0.0f;
	pixels = floorf(width * painter->scale + 0.5f);
	return pixels < 1.0f ? 1.0f : pixels;
}

static rvn_color_t rvn_paint_color(const RPainter* painter, rvn_color_t color) {
	return painter->opacity < 0.999f ? rvn_color_fade(color, painter->opacity) : color;
}

static void rvn_instance_clip(RDrawInstance* instance, const RPainter* painter) {
	instance->clip[0] = painter->clip.x0;
	instance->clip[1] = painter->clip.y0;
	instance->clip[2] = painter->clip.x1;
	instance->clip[3] = painter->clip.y1;
}

/* Appends an instance unless `bounds` is entirely clipped. */
static RDrawInstance* rvn_paint_emit(RPainter* painter, rvn_box_t bounds, Rid texture) {
	RDrawInstance* instance;
	if (!rvn_box_overlaps(bounds, painter->clip)) return NULL;
	instance = rvn_drawlist_add(painter->list, texture);
	memset(instance, 0, sizeof(*instance));
	rvn_instance_clip(instance, painter);
	return instance;
}

/* ---------------------------------------------------------------------------
 *  Primitives
 * ------------------------------------------------------------------------- */

void rvn_paint_box4(RPainter* painter, rvn_rect_t rect, const float radii[4], rvn_color_t fill,
	float border_width, rvn_color_t border_color) {

	RDrawInstance* instance;
	rvn_box_t box;
	float limit;
	int i;

	fill = rvn_paint_color(painter, fill);
	border_color = rvn_paint_color(painter, border_color);
	if (border_width <= 0.0f || rvn_color_a(border_color) == 0) {
		border_width = 0.0f;
		border_color = 0;
	}
	if (rvn_color_a(fill) == 0 && border_width <= 0.0f) return;

	box = rvn_paint_snap(painter, rect);
	if (box.x1 <= box.x0 || box.y1 <= box.y0) return;
	instance = rvn_paint_emit(painter, box, R_INVALID_ID);
	if (!instance) return;

	limit = rvn_minf(box.x1 - box.x0, box.y1 - box.y0) * 0.5f;
	instance->rect[0] = box.x0;
	instance->rect[1] = box.y0;
	instance->rect[2] = box.x1;
	instance->rect[3] = box.y1;
	for (i = 0; i < 4; i++) {
		instance->radii[i] = radii ? rvn_clampf(radii[i] * painter->scale, 0.0f, limit) : 0.0f;
	}
	instance->color = fill;
	instance->border_color = border_color;
	instance->border_width = rvn_paint_stroke(painter, border_width);
	instance->flags = R_PRIM_RECT;
}

void rvn_paint_box(RPainter* painter, rvn_rect_t rect, float radius, rvn_color_t fill,
	float border_width, rvn_color_t border_color) {
	float radii[4];
	radii[0] = radii[1] = radii[2] = radii[3] = radius;
	rvn_paint_box4(painter, rect, radii, fill, border_width, border_color);
}

void rvn_paint_rect(RPainter* painter, rvn_rect_t rect, rvn_color_t color) {
	rvn_paint_box4(painter, rect, NULL, color, 0.0f, 0);
}

void rvn_paint_shadow(RPainter* painter, rvn_rect_t rect, float radius, float blur, rvn_color_t color) {
	RDrawInstance* instance;
	rvn_box_t box, bounds;
	float blur_px = blur * painter->scale;
	float limit;

	color = rvn_paint_color(painter, color);
	if (rvn_color_a(color) == 0 || blur_px <= 0.0f) return;
	box = rvn_paint_snap(painter, rect);
	bounds = rvn_box(box.x0 - blur_px, box.y0 - blur_px, box.x1 + blur_px, box.y1 + blur_px);
	instance = rvn_paint_emit(painter, bounds, R_INVALID_ID);
	if (!instance) return;
	limit = rvn_minf(box.x1 - box.x0, box.y1 - box.y0) * 0.5f;
	instance->rect[0] = box.x0;
	instance->rect[1] = box.y0;
	instance->rect[2] = box.x1;
	instance->rect[3] = box.y1;
	instance->radii[0] = instance->radii[1] = instance->radii[2] = instance->radii[3] =
		rvn_clampf(radius * painter->scale, 0.0f, limit);
	instance->color = color;
	instance->border_width = blur_px;
	instance->flags = R_PRIM_SHADOW;
}

void rvn_paint_glass(RPainter* painter, rvn_rect_t rect, const float radii[4], const rvn_glass_t* glass) {
	RDrawInstance* instance;
	rvn_box_t box = rvn_paint_snap(painter, rect);
	float half, bevel, refraction, margin;
	int i;

	if (box.x1 <= box.x0 || box.y1 <= box.y0 || painter->opacity <= 0.0f) return;
	if (!rvn_box_overlaps(box, painter->clip)) return;

	half = rvn_minf(box.x1 - box.x0, box.y1 - box.y0) * 0.5f;
	bevel = glass->bevel > 0.0f ? glass->bevel : rvn_clampf(rvn_minf(rect.w, rect.h) * 0.42f, 3.0f, 14.0f);
	bevel = rvn_minf(bevel * painter->scale, half * 0.95f);
	refraction = glass->refraction * painter->scale;

	/* the backdrop is taken before the shadow, so that the glass does not see its own shadow */
	/* (what the shader reads around the slab: the bent rim, and the reflection on it) */
	margin = rvn_maxf(refraction, bevel * 1.5f + 6.0f) + 2.0f;
	rvn_drawlist_add_backdrop(painter->list, rvn_box_intersect(painter->clip,
		rvn_box(box.x0 - margin, box.y0 - margin, box.x1 + margin, box.y1 + margin)));
	if (glass->shadow > 0.0f) {
		rvn_paint_shadow(painter, rvn_rect(rect.x, rect.y + glass->shadow * 0.25f, rect.w, rect.h),
			radii ? radii[0] : 0.0f, glass->shadow, glass->shadow_color);
	}

	instance = rvn_paint_emit(painter, box, R_INVALID_ID);
	if (!instance) return;
	instance->rect[0] = box.x0;
	instance->rect[1] = box.y0;
	instance->rect[2] = box.x1;
	instance->rect[3] = box.y1;
	for (i = 0; i < 4; i++) instance->radii[i] = radii ? rvn_clampf(radii[i] * painter->scale, 0.0f, half) : 0.0f;
	instance->uv[0] = rvn_clampf(glass->frost, 0.0f, 1.0f);
	instance->uv[1] = refraction;
	instance->uv[2] = glass->saturation;
	instance->uv[3] = glass->glow;
	instance->color = glass->tint;
	instance->border_color = glass->rim;
	instance->border_width = bevel;
	instance->flags = R_PRIM_GLASS | ((uint32_t)(rvn_clampf(painter->opacity, 0.0f, 1.0f) * 255.0f + 0.5f) << 8);
}

void rvn_paint_blur(RPainter* painter, rvn_rect_t rect, const float radii[4], const rvn_blur_t* blur) {
	RDrawInstance* instance;
	rvn_box_t box = rvn_paint_snap(painter, rect);
	float half, radius, margin;
	int i;

	if (box.x1 <= box.x0 || box.y1 <= box.y0 || painter->opacity <= 0.0f) return;
	if (!rvn_box_overlaps(box, painter->clip)) return;
	half = rvn_minf(box.x1 - box.x0, box.y1 - box.y0) * 0.5f;
	radius = rvn_clampf(blur->radius, 0.0f, 96.0f) * painter->scale;

	/* (what the shader reads around the shape: as far as the blur reaches) */
	margin = radius + 4.0f;
	rvn_drawlist_add_backdrop(painter->list, rvn_box_intersect(painter->clip,
		rvn_box(box.x0 - margin, box.y0 - margin, box.x1 + margin, box.y1 + margin)));
	if (blur->shadow > 0.0f) {
		rvn_paint_shadow(painter, rvn_rect(rect.x, rect.y + blur->shadow * 0.25f, rect.w, rect.h),
			radii ? radii[0] : 0.0f, blur->shadow, blur->shadow_color);
	}
	instance = rvn_paint_emit(painter, box, R_INVALID_ID);
	if (!instance) return;
	instance->rect[0] = box.x0;
	instance->rect[1] = box.y0;
	instance->rect[2] = box.x1;
	instance->rect[3] = box.y1;
	for (i = 0; i < 4; i++) instance->radii[i] = radii ? rvn_clampf(radii[i] * painter->scale, 0.0f, half) : 0.0f;
	instance->uv[0] = radius;
	instance->uv[1] = rvn_clampf(blur->grain, 0.0f, 1.0f);
	instance->uv[2] = blur->saturation;
	instance->uv[3] = blur->light;
	instance->color = blur->tint;
	instance->border_color = blur->border;
	instance->border_width = blur->border_width > 0.0f ? rvn_maxf(1.0f, floorf(blur->border_width * painter->scale + 0.5f)) : 0.0f;
	instance->flags = R_PRIM_BLUR | ((uint32_t)(rvn_clampf(painter->opacity, 0.0f, 1.0f) * 255.0f + 0.5f) << 8);
}

void rvn_paint_erase(RPainter* painter, rvn_rect_t rect, const float radii[4], float amount) {
	RDrawInstance* instance;
	rvn_box_t box = rvn_paint_snap(painter, rect);
	float half;
	int i;

	amount = rvn_clampf(amount * painter->opacity, 0.0f, 1.0f);
	if (box.x1 <= box.x0 || box.y1 <= box.y0 || amount <= 0.0f) return;
	if (!rvn_box_overlaps(box, painter->clip)) return;
	half = rvn_minf(box.x1 - box.x0, box.y1 - box.y0) * 0.5f;
	instance = rvn_drawlist_add_erase(painter->list);
	memset(instance, 0, sizeof(*instance));
	instance->rect[0] = box.x0;
	instance->rect[1] = box.y0;
	instance->rect[2] = box.x1;
	instance->rect[3] = box.y1;
	instance->clip[0] = painter->clip.x0;
	instance->clip[1] = painter->clip.y0;
	instance->clip[2] = painter->clip.x1;
	instance->clip[3] = painter->clip.y1;
	for (i = 0; i < 4; i++) instance->radii[i] = radii ? rvn_clampf(radii[i] * painter->scale, 0.0f, half) : 0.0f;
	/* (only its alpha counts: how much of what is there goes) */
	instance->color = rvn_rgba(0.0f, 0.0f, 0.0f, amount);
	instance->flags = R_PRIM_RECT;
}

void rvn_paint_line(RPainter* painter, float x0, float y0, float x1, float y1, float thickness, rvn_color_t color) {
	RDrawInstance* instance;
	rvn_box_t bounds;
	float ax = (painter->ox + x0) * painter->scale, ay = (painter->oy + y0) * painter->scale;
	float bx = (painter->ox + x1) * painter->scale, by = (painter->oy + y1) * painter->scale;
	float width = rvn_maxf(1.0f, thickness * painter->scale);
	float extent = width * 0.5f + 1.0f;

	color = rvn_paint_color(painter, color);
	if (rvn_color_a(color) == 0) return;
	bounds = rvn_box(rvn_minf(ax, bx) - extent, rvn_minf(ay, by) - extent,
		rvn_maxf(ax, bx) + extent, rvn_maxf(ay, by) + extent);
	instance = rvn_paint_emit(painter, bounds, R_INVALID_ID);
	if (!instance) return;
	instance->rect[0] = ax;
	instance->rect[1] = ay;
	instance->rect[2] = bx;
	instance->rect[3] = by;
	instance->radii[0] = width;
	instance->color = color;
	instance->flags = R_PRIM_LINE;
}

rvn_vec2_t rvn_curve_point(const float points[8], float t) {
	float u = 1.0f - t;
	float a = u * u * u, b = 3.0f * u * u * t, c = 3.0f * u * t * t, d = t * t * t;
	return rvn_vec2(a * points[0] + b * points[2] + c * points[4] + d * points[6],
		a * points[1] + b * points[3] + c * points[5] + d * points[7]);
}

void rvn_paint_curve(RPainter* painter, const float points[8], const rvn_curve_t* curve) {
	RDrawInstance* instance;
	rvn_box_t bounds;
	float pixels[8];
	float width = rvn_maxf(1.0f, curve->thickness * painter->scale);
	float halo = rvn_maxf(curve->halo, 0.0f) * painter->scale;
	float extent = width * 0.5f + halo + 1.0f;
	rvn_color_t from = rvn_paint_color(painter, curve->from), to = rvn_paint_color(painter, curve->to);
	int i;

	if (rvn_color_a(from) == 0 && rvn_color_a(to) == 0) return;
	for (i = 0; i < 4; i++) {
		pixels[i * 2] = (painter->ox + points[i * 2]) * painter->scale;
		pixels[i * 2 + 1] = (painter->oy + points[i * 2 + 1]) * painter->scale;
	}
	/* a curve stays inside its four points */
	bounds = rvn_box(pixels[0], pixels[1], pixels[0], pixels[1]);
	for (i = 1; i < 4; i++) {
		bounds.x0 = rvn_minf(bounds.x0, pixels[i * 2]);
		bounds.y0 = rvn_minf(bounds.y0, pixels[i * 2 + 1]);
		bounds.x1 = rvn_maxf(bounds.x1, pixels[i * 2]);
		bounds.y1 = rvn_maxf(bounds.y1, pixels[i * 2 + 1]);
	}
	bounds = rvn_box(bounds.x0 - extent, bounds.y0 - extent, bounds.x1 + extent, bounds.y1 + extent);
	instance = rvn_paint_emit(painter, bounds, R_INVALID_ID);
	if (!instance) return;
	instance->rect[0] = pixels[0];
	instance->rect[1] = pixels[1];
	instance->rect[2] = pixels[6];
	instance->rect[3] = pixels[7];
	instance->radii[0] = pixels[2];
	instance->radii[1] = pixels[3];
	instance->radii[2] = pixels[4];
	instance->radii[3] = pixels[5];
	instance->border_width = width;
	instance->color = from;
	instance->border_color = to;
	instance->uv[0] = halo;
	instance->uv[1] = rvn_clampf(curve->halo_opacity, 0.0f, 1.0f);
	instance->uv[2] = curve->phase - floorf(curve->phase);
	instance->uv[3] = rvn_maxf(curve->pulses, 0.0f);
	instance->flags = R_PRIM_CURVE | ((uint32_t)(rvn_clampf(curve->pulse_strength, 0.0f, 1.0f) * 255.0f + 0.5f) << 8);
}

void rvn_paint_triangle(RPainter* painter, float x0, float y0, float x1, float y1, float x2, float y2, rvn_color_t color) {
	RDrawInstance* instance;
	rvn_box_t bounds;
	float ax = (painter->ox + x0) * painter->scale, ay = (painter->oy + y0) * painter->scale;
	float bx = (painter->ox + x1) * painter->scale, by = (painter->oy + y1) * painter->scale;
	float cx = (painter->ox + x2) * painter->scale, cy = (painter->oy + y2) * painter->scale;

	color = rvn_paint_color(painter, color);
	if (rvn_color_a(color) == 0) return;
	bounds = rvn_box(rvn_minf(ax, rvn_minf(bx, cx)) - 1.0f, rvn_minf(ay, rvn_minf(by, cy)) - 1.0f,
		rvn_maxf(ax, rvn_maxf(bx, cx)) + 1.0f, rvn_maxf(ay, rvn_maxf(by, cy)) + 1.0f);
	instance = rvn_paint_emit(painter, bounds, R_INVALID_ID);
	if (!instance) return;
	instance->rect[0] = ax;
	instance->rect[1] = ay;
	instance->rect[2] = bx;
	instance->rect[3] = by;
	instance->radii[0] = cx;
	instance->radii[1] = cy;
	instance->color = color;
	instance->flags = R_PRIM_TRIANGLE;
}

void rvn_paint_image(RPainter* painter, rvn_rect_t rect, Rid texture, const float uv[4],
	rvn_color_t tint, float radius) {

	RDrawInstance* instance;
	rvn_texture_t* object = rvn_texture_get(texture);
	rvn_box_t box;
	float limit, r;

	/* a texture still being decoded draws nothing */
	if (!object || !object->created) return;
	tint = rvn_paint_color(painter, tint);
	if (rvn_color_a(tint) == 0) return;
	box = rvn_paint_snap(painter, rect);
	if (box.x1 <= box.x0 || box.y1 <= box.y0) return;
	instance = rvn_paint_emit(painter, box, texture);
	if (!instance) return;
	if (object->svg_auto && object->width > 0 && object->height > 0) {
		/* the pixels per logical unit this SVG is shown with, here: the next frame draws it at that scale */
		float span_x = fabsf((uv ? uv[2] - uv[0] : 1.0f)) * (float)object->width;
		float span_y = fabsf((uv ? uv[3] - uv[1] : 1.0f)) * (float)object->height;
		float density = object->density > 0.0f ? object->density : 1.0f;
		float wanted = 0.0f;
		if (span_x > 0.0f) wanted = (box.x1 - box.x0) / span_x * density;
		if (span_y > 0.0f) wanted = rvn_maxf(wanted, (box.y1 - box.y0) / span_y * density);
		if (wanted > 0.0f) {
			int32_t bits;
			memcpy(&bits, &wanted, sizeof(bits));
			rvn_atomic_max(&object->svg_wanted, (int)bits);
		}
	}
	limit = rvn_minf(box.x1 - box.x0, box.y1 - box.y0) * 0.5f;
	r = rvn_clampf(radius * painter->scale, 0.0f, limit);
	instance->rect[0] = box.x0;
	instance->rect[1] = box.y0;
	instance->rect[2] = box.x1;
	instance->rect[3] = box.y1;
	instance->uv[0] = uv ? uv[0] : 0.0f;
	instance->uv[1] = uv ? uv[1] : 0.0f;
	instance->uv[2] = uv ? uv[2] : 1.0f;
	instance->uv[3] = uv ? uv[3] : 1.0f;
	instance->radii[0] = instance->radii[1] = instance->radii[2] = instance->radii[3] = r;
	instance->color = tint;
	instance->flags = R_PRIM_RECT | R_INSTANCE_TEXTURED;
}

void rvn_paint_text(RPainter* painter, Rid font_id, float size, float x, float y,
	const char* text, int length, rvn_color_t color) {
	rvn_font_t* font = rvn_font_resolve(font_id);
	if (!font || !text) return;
	size = rvn_font_size_resolve(size);
	color = rvn_paint_color(painter, color);
	rvn_text_draw(painter->list, font, size * painter->scale,
		floorf((painter->ox + x) * painter->scale + 0.5f),
		floorf((painter->oy + y) * painter->scale + 0.5f),
		text, length, color, painter->clip);
}

void rvn_paint_text_in(RPainter* painter, Rid font_id, float size, rvn_rect_t rect, float align,
	const char* text, int length, rvn_color_t color) {
	rvn_font_t* font = rvn_font_resolve(font_id);
	rvn_font_metrics_t metrics;
	float width;
	if (!font || !text) return;
	if (painter->rtl) align = 1.0f - align;
	size = rvn_font_size_resolve(size);
	rvn_font_metrics(font, size, &metrics);
	width = align > 0.0f ? rvn_text_width(font, size, text, length) : 0.0f;
	rvn_paint_text(painter, font_id, size, rect.x + (rect.w - width) * align,
		rect.y + (rect.h - metrics.line_height) * 0.5f, text, length, color);
}

void rvn_paint_icon(RPainter* painter, rvn_icon_t icon, float cx, float cy, float size, rvn_color_t color) {
	float h = size * 0.5f, q = size * 0.25f;
	float t = rvn_maxf(1.25f, size * 0.14f);
	switch (icon) {
		case RVN_ICON_CHEVRON_RIGHT:
			rvn_paint_line(painter, cx - q, cy - h, cx + q, cy, t, color);
			rvn_paint_line(painter, cx + q, cy, cx - q, cy + h, t, color);
			break;
		case RVN_ICON_CHEVRON_LEFT:
			rvn_paint_line(painter, cx + q, cy - h, cx - q, cy, t, color);
			rvn_paint_line(painter, cx - q, cy, cx + q, cy + h, t, color);
			break;
		case RVN_ICON_CHEVRON_DOWN:
			rvn_paint_line(painter, cx - h, cy - q, cx, cy + q, t, color);
			rvn_paint_line(painter, cx, cy + q, cx + h, cy - q, t, color);
			break;
		case RVN_ICON_CHEVRON_UP:
			rvn_paint_line(painter, cx - h, cy + q, cx, cy - q, t, color);
			rvn_paint_line(painter, cx, cy - q, cx + h, cy + q, t, color);
			break;
		case RVN_ICON_CHECK:
			rvn_paint_line(painter, cx - h, cy, cx - h * 0.25f, cy + h * 0.7f, t, color);
			rvn_paint_line(painter, cx - h * 0.25f, cy + h * 0.7f, cx + h, cy - h * 0.6f, t, color);
			break;
		case RVN_ICON_CROSS:
			rvn_paint_line(painter, cx - h, cy - h, cx + h, cy + h, t, color);
			rvn_paint_line(painter, cx - h, cy + h, cx + h, cy - h, t, color);
			break;
	}
}

void rvn_paint_chevron(RPainter* painter, float cx, float cy, float size, float angle, rvn_color_t color) {
	float h = size * 0.5f, q = size * 0.25f;
	float t = rvn_maxf(1.25f, size * 0.14f);
	float c = cosf(angle), s = sinf(angle);
	/* (-q, -h) -> (q, 0) -> (-q, h), rotated */
	float ax = cx + (-q) * c - (-h) * s, ay = cy + (-q) * s + (-h) * c;
	float bx = cx + q * c, by = cy + q * s;
	float dx = cx + (-q) * c - h * s, dy = cy + (-q) * s + h * c;
	rvn_paint_line(painter, ax, ay, bx, by, t, color);
	rvn_paint_line(painter, bx, by, dx, dy, t, color);
}

void rvn_paint_push_clip(RPainter* painter, rvn_rect_t rect) {
	if (painter->clip_depth < RVN_PAINTER_CLIP_STACK) {
		painter->clip_stack[painter->clip_depth] = painter->clip;
	}
	painter->clip_depth++;
	painter->clip = rvn_box_intersect(painter->clip, rvn_paint_snap(painter, rect));
}

void rvn_paint_pop_clip(RPainter* painter) {
	if (painter->clip_depth <= 0) return;
	painter->clip_depth--;
	if (painter->clip_depth < RVN_PAINTER_CLIP_STACK) {
		painter->clip = painter->clip_stack[painter->clip_depth];
	}
}

/* ---------------------------------------------------------------------------
 *  Public drawing API (custom widgets)
 * ------------------------------------------------------------------------- */

void Rdraw_rect(RPainter* painter, float x, float y, float width, float height,
	float radius, float r, float g, float b, float a) {
	if (!painter) return;
	rvn_paint_box(painter, rvn_rect(x, y, width, height), radius, rvn_rgba(r, g, b, a), 0.0f, 0);
}

void Rdraw_glass(RPainter* painter, float x, float y, float width, float height,
	float radius, float r, float g, float b, float a) {
	rvn_glass_t glass;
	float radii[4];
	if (!painter) return;
	memset(&glass, 0, sizeof(glass));
	glass.tint = rvn_rgba(r, g, b, a);
	glass.rim = rvn_rgba(1.0f, 1.0f, 1.0f, g_rvn.theme.dark ? 0.55f : 0.85f);
	glass.frost = 1.0f;
	glass.refraction = 12.0f;
	glass.saturation = 1.5f;
	radii[0] = radii[1] = radii[2] = radii[3] = radius;
	rvn_paint_glass(painter, rvn_rect(x, y, width, height), radii, &glass);
}

void Rdraw_rect_outline(RPainter* painter, float x, float y, float width, float height,
	float radius, float thickness, float r, float g, float b, float a) {
	if (!painter) return;
	rvn_paint_box(painter, rvn_rect(x, y, width, height), radius, 0, thickness, rvn_rgba(r, g, b, a));
}

void Rdraw_circle(RPainter* painter, float center_x, float center_y, float radius,
	float r, float g, float b, float a) {
	if (!painter) return;
	rvn_paint_box(painter, rvn_rect(center_x - radius, center_y - radius, radius * 2.0f, radius * 2.0f),
		radius, rvn_rgba(r, g, b, a), 0.0f, 0);
}

void Rdraw_line(RPainter* painter, float x0, float y0, float x1, float y1,
	float thickness, float r, float g, float b, float a) {
	if (!painter) return;
	rvn_paint_line(painter, x0, y0, x1, y1, thickness, rvn_rgba(r, g, b, a));
}

void Rdraw_curve(RPainter* painter, float x0, float y0, float cx0, float cy0, float cx1, float cy1, float x1, float y1,
	float thickness, float r, float g, float b, float a) {
	float points[8];
	rvn_curve_t curve;
	if (!painter) return;
	points[0] = x0; points[1] = y0;
	points[2] = cx0; points[3] = cy0;
	points[4] = cx1; points[5] = cy1;
	points[6] = x1; points[7] = y1;
	memset(&curve, 0, sizeof(curve));
	curve.thickness = thickness;
	curve.from = curve.to = rvn_rgba(r, g, b, a);
	rvn_paint_curve(painter, points, &curve);
}

void Rdraw_triangle(RPainter* painter, float x0, float y0, float x1, float y1,
	float x2, float y2, float r, float g, float b, float a) {
	if (!painter) return;
	rvn_paint_triangle(painter, x0, y0, x1, y1, x2, y2, rvn_rgba(r, g, b, a));
}

void Rdraw_image(RPainter* painter, Rid texture, float x, float y, float width, float height,
	float r, float g, float b, float a) {
	if (!painter) return;
	rvn_paint_image(painter, rvn_rect(x, y, width, height), texture, NULL, rvn_rgba(r, g, b, a), 0.0f);
}

void Rdraw_text(RPainter* painter, Rid font, float size, float x, float y,
	const char* text, float r, float g, float b, float a) {
	rvn_font_t* resolved;
	rvn_font_metrics_t metrics;
	const char* line = text;
	if (!painter || !text) return;
	resolved = rvn_font_resolve(font);
	if (!resolved) return;
	size = rvn_font_size_resolve(size);
	rvn_font_metrics(resolved, size, &metrics);
	for (;;) {
		const char* newline = strchr(line, '\n');
		int length = newline ? (int)(newline - line) : (int)strlen(line);
		rvn_paint_text(painter, font, size, x, y, line, length, rvn_rgba(r, g, b, a));
		if (!newline) break;
		line = newline + 1;
		y += metrics.line_height;
	}
}

void Rdraw_push_clip_rect(RPainter* painter, float x, float y, float width, float height) {
	if (!painter) return;
	rvn_paint_push_clip(painter, rvn_rect(x, y, width, height));
}

void Rdraw_pop_clip_rect(RPainter* painter) {
	if (!painter) return;
	rvn_paint_pop_clip(painter);
}
