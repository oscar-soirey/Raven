/**
 * Raven - widgets/image.c
 * Image (Slate SImage): a texture, or a part of one, fitted in the widget.
 *
 * An animated GIF or APNG plays: each Image has its own playback (the frame,
 * the time spent on it, the plays done) and composes its frames into a
 * texture of its own, so that two Images of the same file can be at
 * different frames. The time comes from the clock of the animations (the
 * fixed step of Rset_animation_time_step / Rtest_set_deterministic, the
 * speed of Rset_animation_speed). The frames are only asked for while the
 * Image is painted: hidden, scrolled away, in a window that is not shown,
 * it costs nothing and resumes where it was. Without animations (reduced
 * motion) it stays on its frame.
 */

#include "common.h"
#include "../core/context.h"
#include "../render/renderer.h"

typedef struct rvn_image_t {
	rvn_widget_t base;
	Rid texture;
	rvn_color_t tint;
	RStretch stretch;
	float uv[4];
	float radius;
	/** R_TRUE: tinted with the text color (white glyphs that follow the theme), times `tint`. */
	int tint_text;
	/** R_TRUE once a tint was given: an SVG in currentColor is then drawn in it, not in the text color. */
	int tint_set;

	/* ---- playback of an animated texture ---- */
	/** Plays when its texture is set (XML Animate). */
	int autoplay;
	int playing;
	/** Times it plays, given by the application: -1 the file's, 0 forever. */
	int loop_override;
	/** Frame shown, the seconds it has been shown, the plays completed. */
	int frame;
	double frame_time;
	int plays;
	/** The animation the state is about (rvn_anim_t.serial; 0: none seen yet). */
	uint32_t anim_serial;
	rvn_anim_cursor_t cursor;
	/** Texture of the frames, the frame it holds (-1: none) and the restores of the device it was made under. */
	Rid frame_texture;
	int texture_frame;
	uint32_t frame_restores;
	/** When it was last painted (window frame and clock): the time of the animation goes on only from one frame to the next. */
	int painted;
	uint64_t paint_frame;
	double paint_time;
	/** Painted on the UI thread (rvn_widget_add_ui_paint), for the texture of its frames. */
	int ui_paint;
	RImageFinishedCallback finished;
	void* finished_user;
} rvn_image_t;

static void rvn_image_init(rvn_widget_t* widget) {
	rvn_image_t* image = (rvn_image_t*)widget;
	image->texture = R_INVALID_ID;
	image->frame_texture = R_INVALID_ID;
	image->texture_frame = -1;
	image->cursor.index = -1;
	image->autoplay = R_TRUE;
	image->playing = R_TRUE;
	image->loop_override = -1;
	image->tint = 0xFFFFFFFFu;
	image->stretch = R_STRETCH_FIT;
	image->uv[2] = 1.0f;
	image->uv[3] = 1.0f;
}

/* Size of the shown part of the texture, in texels. */
static rvn_vec2_t rvn_image_source_size(const rvn_image_t* image) {
	rvn_texture_t* texture = rvn_texture_get(image->texture);
	float density;
	if (!texture) return rvn_vec2(0.0f, 0.0f);
	/* (an SVG shows at the size it gives, whatever scale it is drawn at) */
	density = texture->density > 0.0f ? texture->density : 1.0f;
	return rvn_vec2((float)texture->width / density * fabsf(image->uv[2] - image->uv[0]),
		(float)texture->height / density * fabsf(image->uv[3] - image->uv[1]));
}

static rvn_vec2_t rvn_image_measure(rvn_widget_t* widget) {
	/* one texel per logical unit unless the widget is given another size */
	return rvn_image_source_size((rvn_image_t*)widget);
}

/**
 * Computes where a source of `size` texels goes in a `width` x `height` area
 * and which part of it is visible. Shared with the viewport widget.
 */
void rvn_image_fit(RStretch stretch, rvn_vec2_t size, float width, float height, const float uv_in[4],
	rvn_rect_t* rect, float uv_out[4]) {
	float scale;
	memcpy(uv_out, uv_in, sizeof(float) * 4);
	*rect = rvn_rect(0.0f, 0.0f, width, height);
	if (size.x <= 0.0f || size.y <= 0.0f || width <= 0.0f || height <= 0.0f) return;

	switch (stretch) {
		case R_STRETCH_FIT:
			scale = rvn_minf(width / size.x, height / size.y);
			rect->w = size.x * scale;
			rect->h = size.y * scale;
			rect->x = (width - rect->w) * 0.5f;
			rect->y = (height - rect->h) * 0.5f;
			break;
		case R_STRETCH_COVER: {
			/* keep the whole area, show the centered part of the source that has its ratio */
			float visible_u, visible_v, du = uv_in[2] - uv_in[0], dv = uv_in[3] - uv_in[1];
			scale = rvn_maxf(width / size.x, height / size.y);
			visible_u = width / (size.x * scale);
			visible_v = height / (size.y * scale);
			uv_out[0] = uv_in[0] + du * (1.0f - visible_u) * 0.5f;
			uv_out[2] = uv_in[2] - du * (1.0f - visible_u) * 0.5f;
			uv_out[1] = uv_in[1] + dv * (1.0f - visible_v) * 0.5f;
			uv_out[3] = uv_in[3] - dv * (1.0f - visible_v) * 0.5f;
			break;
		}
		case R_STRETCH_NONE:
			rect->w = size.x;
			rect->h = size.y;
			rect->x = floorf((width - size.x) * 0.5f);
			rect->y = floorf((height - size.y) * 0.5f);
			break;
		case R_STRETCH_FILL:
		default:
			break;
	}
}

/* ---------------------------------------------------------------------------
 *  Animation
 * ------------------------------------------------------------------------- */

static const rvn_anim_t* rvn_image_anim(const rvn_image_t* image) {
	rvn_texture_t* texture = image->texture != R_INVALID_ID ? rvn_texture_get(image->texture) : NULL;
	return texture ? texture->anim : NULL;
}

static int rvn_image_loops(const rvn_image_t* image, const rvn_anim_t* anim) {
	return image->loop_override >= 0 ? image->loop_override : anim->loop_count;
}

/* The frames are composed no more: the memory goes (the next frame starts again from the first). */
static void rvn_image_drop_frames(rvn_image_t* image) {
	rvn_anim_cursor_free(&image->cursor);
	if (image->frame_texture != R_INVALID_ID) rvn_texture_release(image->frame_texture);
	image->frame_texture = R_INVALID_ID;
	image->texture_frame = -1;
}

static void rvn_image_destroy(rvn_widget_t* widget) {
	rvn_image_drop_frames((rvn_image_t*)widget);
}

/* The callback and the action, at the next frame: they may change the tree, which must not change while it is painted. */
static void rvn_image_finished_task(void* user_data) {
	rvn_widget_t* widget = rvn_widget_get((Rid)(uintptr_t)user_data);
	rvn_image_t* image = (rvn_image_t*)widget;
	if (!widget || widget->cls->type != R_WIDGET_IMAGE) return;
	if (image->finished) image->finished(widget->id, image->finished_user);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

/* Moves the playback on by `dt` seconds. */
static void rvn_image_advance(rvn_widget_t* widget, rvn_image_t* image, const rvn_anim_t* anim, double dt) {
	int loops = rvn_image_loops(image, anim);
	double duration = (double)anim->duration_ms / 1000.0;
	/* (more than a whole play at once, after a long stall: one play at most) */
	image->frame_time += dt < duration ? dt : duration;
	while (image->playing) {
		double delay = (double)anim->frames[image->frame].delay_ms / 1000.0;
		/* (a small margin: the fixed steps of the clock add up to the delays they are meant to) */
		if (image->frame_time + 1e-6 < delay) break;
		image->frame_time -= delay;
		if (image->frame + 1 < anim->frame_count) {
			image->frame++;
			continue;
		}
		image->plays++;
		if (loops > 0 && image->plays >= loops) {
			/* the last frame stays */
			image->playing = R_FALSE;
			image->frame_time = 0.0;
			Rtask_post(rvn_image_finished_task, (void*)(uintptr_t)widget->id);
			break;
		}
		image->frame = 0;
	}
}

/*
 * The texture to draw for an animation: its own texture when it shows the
 * first frame and does not play, else the texture of its frames, brought to
 * the frame of now. Asks for the frame that comes next.
 */
static Rid rvn_image_animate(rvn_widget_t* widget, rvn_image_t* image, rvn_texture_t* texture, RPainter* painter) {
	const rvn_anim_t* anim = texture->anim;
	rvn_window_t* window = painter->window;
	int moving;

	/* (the texture of the frames is made on the UI thread; rvn_image_update_ui_paint sees to it) */
	if (!Ris_ui_thread()) return image->texture_frame >= 0 ? image->frame_texture : image->texture;
	if (image->anim_serial != anim->serial) {
		/* another animation in the same texture: from its start */
		if (image->anim_serial != 0) {
			image->frame = 0;
			image->frame_time = 0.0;
			image->plays = 0;
		}
		image->anim_serial = anim->serial;
		rvn_image_drop_frames(image);
	}
	if (image->frame >= anim->frame_count) image->frame = anim->frame_count - 1;
	moving = image->playing && Rget_animations_enabled();
	if (moving && window) {
		/* the time of the animations, only from one painted frame to the next: hidden, it did not go on */
		int continued = image->painted && image->paint_frame + 1 == window->frame_index;
		double dt;
		if (g_rvn.animation_time_step > 0.0) {
			dt = g_rvn.animation_time_step * (double)Rget_animation_speed();
		} else {
			/* (the clock: a window shown again after a while, or a long stall, goes on with the next frame rather than far ahead) */
			double left = (double)anim->frames[image->frame].delay_ms / 1000.0 - image->frame_time;
			dt = (window->paint_time - image->paint_time) * (double)Rget_animation_speed();
			if (dt > left + 0.25) dt = left + 0.25;
		}
		if (continued && dt > 0.0) rvn_image_advance(widget, image, anim, dt);
	}
	if (window) {
		/* (still, it counts no time: the next frame it moves in starts the clock again) */
		image->painted = moving;
		image->paint_frame = window->frame_index;
		image->paint_time = window->paint_time;
	}
	moving = moving && image->playing;
	if (moving) {
		double left = (double)anim->frames[image->frame].delay_ms / 1000.0 - image->frame_time;
		/* a fixed step: every frame is a step of the animation; the clock: a frame when the next one is due */
		if (g_rvn.animation_time_step > 0.0) rvn_paint_keep_animating(painter);
		else rvn_paint_wake_after(painter, left / (double)Rget_animation_speed());
	}

	/* the first frame, still: the texture holds it */
	if (image->frame == 0 && !moving && image->texture_frame < 0) return image->texture;
	{
		rvn_texture_t* frames = image->frame_texture != R_INVALID_ID ? rvn_texture_get(image->frame_texture) : NULL;
		if (!frames || image->texture_frame != image->frame || frames->restores != image->frame_restores) {
			size_t size = (size_t)anim->width * (size_t)anim->height * 4;
			uint8_t* pixels = (uint8_t*)rvn_alloc(size);
			memcpy(pixels, rvn_anim_compose(anim, &image->cursor, image->frame), size);
			if (frames && frames->width == anim->width && frames->height == anim->height) {
				rvn_texture_replace_pixels(image->frame_texture, pixels);
			} else {
				if (frames) rvn_texture_release(image->frame_texture);
				image->frame_texture = rvn_texture_create_raw(anim->width, anim->height, R_TEXTURE_RGBA8, texture->filter, pixels);
				frames = rvn_texture_get(image->frame_texture);
			}
			if (!frames) return image->texture;
			image->texture_frame = image->frame;
			image->frame_restores = frames->restores;
		}
	}
	return image->frame_texture;
}

/* Paints on the UI thread while it shows an animation, or a texture that may become one (still decoding). */
static void rvn_image_update_ui_paint(rvn_widget_t* widget) {
	rvn_image_t* image = (rvn_image_t*)widget;
	rvn_texture_t* texture = image->texture != R_INVALID_ID ? rvn_texture_get(image->texture) : NULL;
	int wanted = texture && (texture->anim || !rvn_atomic_load(&texture->ready));
	if (wanted == image->ui_paint) return;
	image->ui_paint = wanted;
	rvn_widget_add_ui_paint(widget, wanted ? 1 : -1);
}

/* A new texture: its animation plays (or not) from the start. */
static void rvn_image_reset(rvn_widget_t* widget) {
	rvn_image_t* image = (rvn_image_t*)widget;
	rvn_image_drop_frames(image);
	image->anim_serial = 0;
	image->frame = 0;
	image->frame_time = 0.0;
	image->plays = 0;
	image->playing = image->autoplay;
	image->painted = R_FALSE;
	rvn_image_update_ui_paint(widget);
}

static void rvn_image_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_image_t* image = (rvn_image_t*)widget;
	rvn_rect_t rect;
	float uv[4];
	rvn_color_t tint = image->tint;
	rvn_texture_t* texture;
	Rid shown;
	int clip;

	if (image->texture == R_INVALID_ID) return;
	texture = rvn_texture_get(image->texture);
	shown = texture && texture->anim ? rvn_image_animate(widget, image, texture, painter) : image->texture;
	/* (the texture is no longer an animation: Rtexture_update) */
	if ((!texture || !texture->anim) && image->frame_texture != R_INVALID_ID && Ris_ui_thread()) rvn_image_drop_frames(image);
	rvn_image_fit(image->stretch, rvn_image_source_size(image), widget->rect.w, widget->rect.h, image->uv, &rect, uv);
	if (rvn_texture_is_current_color(image->texture) && (image->tint_text || !image->tint_set)) {
		/* an SVG in currentColor: the color of the text, which follows the theme and the
		 * states (hovered, pressed, disabled) of the button around it; the alpha of `tint` still applies */
		tint = rvn_color_fade(rvn_widget_text_color(widget, painter), (float)rvn_color_a(image->tint) / 255.0f);
	} else {
		if (image->tint_text) {
			rvn_color_t text = rvn_widget_text_color(widget, painter);
			/* the alpha of `tint` still applies */
			tint = rvn_color_fade(text, (float)rvn_color_a(image->tint) / 255.0f);
		}
		if (!painter->enabled) tint = rvn_color_fade(tint, 0.4f);
	}
	/* an unscaled image larger than the widget is cut at its bounds */
	clip = image->stretch == R_STRETCH_NONE && (rect.w > widget->rect.w || rect.h > widget->rect.h);
	if (clip) rvn_paint_push_clip(painter, rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h));
	rvn_paint_image(painter, rect, shown, uv, tint, image->radius);
	if (clip) rvn_paint_pop_clip(painter);
}

int rvn_parse_stretch(const char* value, RStretch* stretch) {
	if (rvn_stricmp(value, "Fill") == 0) *stretch = R_STRETCH_FILL;
	else if (rvn_stricmp(value, "Fit") == 0) *stretch = R_STRETCH_FIT;
	else if (rvn_stricmp(value, "Cover") == 0) *stretch = R_STRETCH_COVER;
	else if (rvn_stricmp(value, "None") == 0) *stretch = R_STRETCH_NONE;
	else return R_FALSE;
	return R_TRUE;
}

static int rvn_image_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_image_t* image = (rvn_image_t*)widget;
	if (RVN_ATTR("Source")) {
		image->texture = rvn_xml_texture(value);
		rvn_image_reset(widget);
		if (image->texture == R_INVALID_ID) return R_FALSE;
	} else if (RVN_ATTR("Animate")) {
		if (!rvn_parse_bool(value, &image->autoplay)) return R_FALSE;
		image->playing = image->autoplay;
	} else if (RVN_ATTR("Loop")) {
		int plays;
		if (rvn_stricmp(value, "Forever") == 0) image->loop_override = 0;
		else if (rvn_stricmp(value, "Once") == 0) image->loop_override = 1;
		else if (rvn_parse_int(value, &plays) && plays >= 0) image->loop_override = plays;
		else if (rvn_parse_bool(value, &plays)) image->loop_override = plays ? 0 : 1;
		else return R_FALSE;
	} else if (RVN_ATTR("Tint")) {
		if (!rvn_parse_color(value, &image->tint)) return R_FALSE;
		image->tint_set = R_TRUE;
	} else if (RVN_ATTR("TintText")) {
		if (!rvn_parse_bool(value, &image->tint_text)) return R_FALSE;
	} else if (RVN_ATTR("Stretch")) {
		if (!rvn_parse_stretch(value, &image->stretch)) return R_FALSE;
	} else if (RVN_ATTR("Region")) {
		if (rvn_parse_floats(value, image->uv, 4) != 4) return R_FALSE;
	} else if (RVN_ATTR("CornerRadius")) {
		if (!rvn_parse_float(value, &image->radius)) return R_FALSE;
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_layout(widget);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_image = {
	.type = R_WIDGET_IMAGE,
	.name = "Image",
	.size = sizeof(rvn_image_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_NO_HIT,
	.init = rvn_image_init,
	.measure = rvn_image_measure,
	.paint = rvn_image_paint,
	.set_attribute = rvn_image_set_attribute,
	.destroy = rvn_image_destroy
};

void Rimage_set_texture(Rid id, Rid texture) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_IMAGE);
	if (texture != R_INVALID_ID && !rvn_texture_get(texture)) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rimage_set_texture: invalid texture");
		return;
	}
	((rvn_image_t*)widget)->texture = texture;
	rvn_image_reset(widget);
	rvn_widget_invalidate_layout(widget);
}

void Rimage_set_tint_color(Rid id, float r, float g, float b, float a) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_IMAGE);
	((rvn_image_t*)widget)->tint = rvn_rgba(r, g, b, a);
	((rvn_image_t*)widget)->tint_set = R_TRUE;
	rvn_widget_invalidate_paint(widget);
}

void Rimage_set_stretch(Rid id, RStretch stretch) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_IMAGE);
	if (stretch < R_STRETCH_FILL || stretch > R_STRETCH_NONE) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rimage_set_stretch: invalid stretch mode");
		return;
	}
	((rvn_image_t*)widget)->stretch = stretch;
	rvn_widget_invalidate_paint(widget);
}

void Rimage_set_region(Rid id, float min_u, float min_v, float max_u, float max_v) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_IMAGE);
	rvn_image_t* image = (rvn_image_t*)widget;
	image->uv[0] = min_u;
	image->uv[1] = min_v;
	image->uv[2] = max_u;
	image->uv[3] = max_v;
	rvn_widget_invalidate_layout(widget);
}

void Rimage_set_corner_radius(Rid id, float radius) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_IMAGE);
	((rvn_image_t*)widget)->radius = radius > 0.0f ? radius : 0.0f;
	rvn_widget_invalidate_paint(widget);
}

void Rimage_set_tint_text(Rid id, int enabled) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_IMAGE);
	((rvn_image_t*)widget)->tint_text = enabled ? R_TRUE : R_FALSE;
	rvn_widget_invalidate_paint(widget);
}

/* ---------------------------------------------------------------------------
 *  Animated images
 * ------------------------------------------------------------------------- */

int Rimage_is_animated(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_IMAGE, R_FALSE);
	return rvn_image_anim((rvn_image_t*)widget) != NULL;
}

int Rimage_get_frame_count(Rid id) {
	const rvn_anim_t* anim;
	RVN_WIDGET(widget, id, R_WIDGET_IMAGE, 0);
	anim = rvn_image_anim((rvn_image_t*)widget);
	if (anim) return anim->frame_count;
	return rvn_texture_get(((rvn_image_t*)widget)->texture) ? 1 : 0;
}

double Rimage_get_duration(Rid id) {
	const rvn_anim_t* anim;
	RVN_WIDGET(widget, id, R_WIDGET_IMAGE, 0.0);
	anim = rvn_image_anim((rvn_image_t*)widget);
	return anim ? (double)anim->duration_ms / 1000.0 : 0.0;
}

double Rimage_get_frame_delay(Rid id, int frame) {
	const rvn_anim_t* anim;
	RVN_WIDGET(widget, id, R_WIDGET_IMAGE, 0.0);
	anim = rvn_image_anim((rvn_image_t*)widget);
	if (!anim || frame < 0 || frame >= anim->frame_count) return 0.0;
	return (double)anim->frames[frame].delay_ms / 1000.0;
}

void Rimage_play(Rid id) {
	rvn_image_t* image;
	const rvn_anim_t* anim;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_IMAGE);
	image = (rvn_image_t*)widget;
	if (image->playing) return;
	anim = rvn_image_anim(image);
	/* a play that ended starts again */
	if (anim) {
		int loops = rvn_image_loops(image, anim);
		if (loops > 0 && image->plays >= loops) {
			image->plays = 0;
			image->frame = 0;
			image->frame_time = 0.0;
		}
	}
	image->playing = R_TRUE;
	/* (the time it was paused is not counted) */
	image->painted = R_FALSE;
	rvn_widget_invalidate_paint(widget);
}

void Rimage_pause(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_IMAGE);
	((rvn_image_t*)widget)->playing = R_FALSE;
	rvn_widget_invalidate_paint(widget);
}

void Rimage_stop(Rid id) {
	rvn_image_t* image;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_IMAGE);
	image = (rvn_image_t*)widget;
	image->playing = R_FALSE;
	image->frame = 0;
	image->frame_time = 0.0;
	image->plays = 0;
	/* the texture shows the first frame: the frames are let go */
	rvn_image_drop_frames(image);
	rvn_widget_invalidate_paint(widget);
}

int Rimage_is_playing(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_IMAGE, R_FALSE);
	return ((rvn_image_t*)widget)->playing && rvn_image_anim((rvn_image_t*)widget) != NULL;
}

void Rimage_set_frame(Rid id, int frame) {
	rvn_image_t* image;
	const rvn_anim_t* anim;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_IMAGE);
	image = (rvn_image_t*)widget;
	anim = rvn_image_anim(image);
	if (frame < 0 || (anim && frame >= anim->frame_count)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rimage_set_frame: frame %d out of range", frame);
		return;
	}
	image->frame = frame;
	image->frame_time = 0.0;
	rvn_widget_invalidate_paint(widget);
}

int Rimage_get_frame(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_IMAGE, 0);
	return ((rvn_image_t*)widget)->frame;
}

void Rimage_set_loop_count(Rid id, int count) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_IMAGE);
	((rvn_image_t*)widget)->loop_override = count < 0 ? -1 : count;
}

int Rimage_get_loop_count(Rid id) {
	rvn_image_t* image;
	const rvn_anim_t* anim;
	RVN_WIDGET(widget, id, R_WIDGET_IMAGE, 0);
	image = (rvn_image_t*)widget;
	anim = rvn_image_anim(image);
	if (image->loop_override >= 0) return image->loop_override;
	return anim ? anim->loop_count : 1;
}

void Rimage_set_finished_callback(Rid id, RImageFinishedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_IMAGE);
	((rvn_image_t*)widget)->finished = callback;
	((rvn_image_t*)widget)->finished_user = user_data;
}
