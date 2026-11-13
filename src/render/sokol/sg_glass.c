/**
 * Raven - render/sokol/sg_glass.c
 * Render targets and the backdrop of the glass primitives (the counterpart of vulkan/vk_glass.c).
 *
 * Glass shows, bent and frosted, what is behind it. A fragment shader cannot read the image it draws
 * into, so a window that contains glass is rendered differently:
 *
 *   1. the frame is drawn into an offscreen image, the *scene*;
 *   2. at each R_DRAW_BACKDROP command, drawing pauses and the scene is copied into two images: a
 *      half-size copy (the "sharp" backdrop) and a blurred one, built by halving the image a few times
 *      and enlarging it back (dual filtering);
 *   3. drawing continues in the scene; glass instances sample the two backdrop images;
 *   4. the finished scene is drawn into the frame.
 */

#include "sg_backend.h"

#include <math.h>

/* ---------------------------------------------------------------------------
 *  Targets
 * ------------------------------------------------------------------------- */

void rvn_sg_target_destroy(rvn_sg_target_t* target) {
	if (!target->image.id) return;
	sg_destroy_view(target->color);
	sg_destroy_view(target->sample);
	sg_destroy_image(target->image);
	memset(target, 0, sizeof(*target));
}

int rvn_sg_target_create(rvn_sg_target_t* target, int width, int height, const char* label) {
	sg_image_desc image;
	sg_view_desc view;
	memset(target, 0, sizeof(*target));
	memset(&image, 0, sizeof(image));
	image.type = SG_IMAGETYPE_2D;
	image.width = width;
	image.height = height;
	image.pixel_format = SG_PIXELFORMAT_RGBA8;
	image.usage.color_attachment = true;
	/* (a level of a texture is made in one of these, then copied: sg_copy_image_to_image) */
	image.usage.copy_src = true;
	image.label = label;
	target->image = sg_make_image(&image);
	if (sg_query_image_state(target->image) != SG_RESOURCESTATE_VALID) {
		rvn_sg_target_destroy(target);
		return R_FALSE;
	}
	memset(&view, 0, sizeof(view));
	view.color_attachment.image = target->image;
	target->color = sg_make_view(&view);
	memset(&view, 0, sizeof(view));
	view.texture.image = target->image;
	target->sample = sg_make_view(&view);
	target->width = width;
	target->height = height;
	return R_TRUE;
}

void rvn_sg_glass_release(rvn_sg_surface_t* surface) {
	int i;
	rvn_sg_target_destroy(&surface->scene);
	for (i = 0; i < RVN_SG_BLUR_LEVELS; i++) rvn_sg_target_destroy(&surface->down[i]);
	for (i = 0; i < RVN_SG_BLUR_LEVELS - 1; i++) rvn_sg_target_destroy(&surface->up[i]);
	surface->glass_ready = R_FALSE;
	surface->glass_filled = R_FALSE;
}

int rvn_sg_glass_prepare(rvn_sg_surface_t* surface) {
	int i;
	if (surface->glass_ready) return R_TRUE;
	if (!rvn_sg_target_create(&surface->scene, surface->width, surface->height, "raven-scene")) goto failed;
	for (i = 0; i < RVN_SG_BLUR_LEVELS; i++) {
		int width = rvn_maxi(surface->width >> (i + 1), 1), height = rvn_maxi(surface->height >> (i + 1), 1);
		if (!rvn_sg_target_create(&surface->down[i], width, height, "raven-down")) goto failed;
		/* enlarged copies exist for every size but the smallest */
		if (i + 1 < RVN_SG_BLUR_LEVELS && !rvn_sg_target_create(&surface->up[i], width, height, "raven-up")) goto failed;
	}
	surface->glass_ready = R_TRUE;
	surface->glass_filled = R_FALSE;
	return R_TRUE;
failed:
	rvn_sg_glass_release(surface);
	return R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Passes
 * ------------------------------------------------------------------------- */

/*
 * Filters `source` into `target`: all of it, or only `area` (pixels of the target), the rest keeping its
 * content. mode: 0 = reduce, 1 = enlarge, 2 = copy (blur.frag).
 */
void rvn_sg_glass_filter(const rvn_sg_target_t* source, const rvn_sg_target_t* target, int mode, float spread, const int* area) {
	sg_pass pass;
	sg_bindings bindings;
	float params[4];
	sg_range range;

	memset(&pass, 0, sizeof(pass));
	pass.action.colors[0].load_action = area ? SG_LOADACTION_LOAD : SG_LOADACTION_DONTCARE;
	pass.action.colors[0].store_action = SG_STOREACTION_STORE;
	pass.attachments.colors[0] = target->color;
	sg_begin_pass(&pass);
	sg_apply_viewport(0, 0, target->width, target->height, rvn_sg_origin());
	if (area) sg_apply_scissor_rect(area[0], area[1], area[2], area[3], rvn_sg_origin());
	sg_apply_pipeline(g_sg.blur_pipeline);
	memset(&bindings, 0, sizeof(bindings));
	bindings.views[0] = source->sample;
	bindings.samplers[0] = g_sg.linear;
	sg_apply_bindings(&bindings);
	/* offsets are in texels of the source: a whole one when reducing, halves when enlarging */
	params[0] = spread * (mode == 0 ? 1.0f : 0.5f) / (float)source->width;
	params[1] = spread * (mode == 0 ? 1.0f : 0.5f) / (float)source->height;
	params[2] = (float)mode;
	params[3] = 0.0f;
	range.ptr = params;
	range.size = sizeof(params);
	sg_apply_uniforms(0, &range);
	sg_draw(0, 3, 1);
	sg_end_pass();
}

/* The part of `target` that shows the pixels x0..x1, y0..y1 of the frame. R_FALSE: nothing. */
static int rvn_sg_glass_area(const rvn_sg_surface_t* surface, const rvn_sg_target_t* target,
	float x0, float y0, float x1, float y1, int area[4]) {

	float fx = (float)target->width / (float)surface->width;
	float fy = (float)target->height / (float)surface->height;
	int ax0 = rvn_maxi((int)floorf(x0 * fx), 0), ay0 = rvn_maxi((int)floorf(y0 * fy), 0);
	int ax1 = rvn_mini((int)ceilf(x1 * fx), target->width), ay1 = rvn_mini((int)ceilf(y1 * fy), target->height);
	if (ax1 <= ax0 || ay1 <= ay0) return R_FALSE;
	area[0] = ax0;
	area[1] = ay0;
	area[2] = ax1 - ax0;
	area[3] = ay1 - ay0;
	return R_TRUE;
}

void rvn_sg_glass_capture(rvn_sg_surface_t* surface, float scale, const int rect[4]) {
	/* the blur is sized in pixels: follow the scale of the window, within reason */
	float spread = rvn_clampf(0.5f + scale * 0.5f, 1.0f, 2.5f);
	/*
	 * A pixel of the blurred image depends on the frame up to about 18 x spread pixels away, plus the width
	 * of the bilinear samples: refreshing an area means filtering that much around it.
	 */
	float margin = 18.0f * spread + 20.0f;
	float x0 = (float)rect[0] - margin, y0 = (float)rect[1] - margin;
	float x1 = (float)(rect[0] + rect[2]) + margin, y1 = (float)(rect[1] + rect[3]) + margin;
	float width = (float)surface->width, height = (float)surface->height;
	int partial, i, area[4];

	if (rect[2] <= 0 || rect[3] <= 0) {
		/* no area given: everything */
		x0 = y0 = 0.0f;
		x1 = width;
		y1 = height;
	}
	x0 = rvn_maxf(x0, 0.0f);
	y0 = rvn_maxf(y0, 0.0f);
	x1 = rvn_minf(x1, width);
	y1 = rvn_minf(y1, height);
	if (x1 <= x0 || y1 <= y0) return;
	/* an area is only worth its bookkeeping when it is clearly smaller than the frame,
	 * and only possible once the images hold a whole capture */
	partial = surface->glass_filled && (x1 - x0) * (y1 - y0) < width * height * 0.8f;

	/* scene -> 1/2 -> 1/4 -> 1/8: down[0] doubles as the sharp backdrop */
	for (i = 0; i < RVN_SG_BLUR_LEVELS; i++) {
		const rvn_sg_target_t* source = i == 0 ? &surface->scene : &surface->down[i - 1];
		if (partial && !rvn_sg_glass_area(surface, &surface->down[i], x0, y0, x1, y1, area)) continue;
		rvn_sg_glass_filter(source, &surface->down[i], 0, i == 0 ? 1.0f : spread, partial ? area : NULL);
	}
	/* ... and back up to half size: up[0] is the blurred backdrop */
	for (i = RVN_SG_BLUR_LEVELS - 2; i >= 0; i--) {
		const rvn_sg_target_t* source = i == RVN_SG_BLUR_LEVELS - 2 ? &surface->down[RVN_SG_BLUR_LEVELS - 1] : &surface->up[i + 1];
		if (partial && !rvn_sg_glass_area(surface, &surface->up[i], x0, y0, x1, y1, area)) continue;
		rvn_sg_glass_filter(source, &surface->up[i], 1, spread, partial ? area : NULL);
	}
	surface->glass_filled = R_TRUE;
}

void rvn_sg_glass_present(rvn_sg_surface_t* surface) {
	/* the whole scene into the frame: the "filter" in copy mode, no spread */
	sg_pass pass;
	sg_bindings bindings;
	/* copy, no spread, no flip */
	float params[4] = { 0.0f, 0.0f, 2.0f, 0.0f };
	sg_range range = { params, sizeof(params) };

	memset(&pass, 0, sizeof(pass));
	pass.action.colors[0].load_action = SG_LOADACTION_DONTCARE;
	pass.action.colors[0].store_action = SG_STOREACTION_STORE;
	pass.attachments.colors[0] = surface->frame.color;
	sg_begin_pass(&pass);
	sg_apply_viewport(0, 0, surface->width, surface->height, rvn_sg_origin());
	sg_apply_pipeline(g_sg.blur_pipeline);
	memset(&bindings, 0, sizeof(bindings));
	bindings.views[0] = surface->scene.sample;
	bindings.samplers[0] = g_sg.linear;
	sg_apply_bindings(&bindings);
	sg_apply_uniforms(0, &range);
	sg_draw(0, 3, 1);
	sg_end_pass();
}
