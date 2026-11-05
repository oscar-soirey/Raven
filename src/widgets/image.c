/**
 * Raven - widgets/image.c
 * Image (Slate SImage): a texture, or a part of one, fitted in the widget.
 */

#include "common.h"
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
} rvn_image_t;

static void rvn_image_init(rvn_widget_t* widget) {
	rvn_image_t* image = (rvn_image_t*)widget;
	image->texture = R_INVALID_ID;
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

static void rvn_image_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_image_t* image = (rvn_image_t*)widget;
	rvn_rect_t rect;
	float uv[4];
	rvn_color_t tint = image->tint;
	int clip;

	if (image->texture == R_INVALID_ID) return;
	rvn_image_fit(image->stretch, rvn_image_source_size(image), widget->rect.w, widget->rect.h, image->uv, &rect, uv);
	if (image->tint_text) {
		rvn_color_t text = rvn_widget_text_color(widget, painter);
		/* the alpha of `tint` still applies */
		tint = rvn_color_fade(text, (float)rvn_color_a(image->tint) / 255.0f);
	}
	if (!painter->enabled) tint = rvn_color_fade(tint, 0.4f);
	/* an unscaled image larger than the widget is cut at its bounds */
	clip = image->stretch == R_STRETCH_NONE && (rect.w > widget->rect.w || rect.h > widget->rect.h);
	if (clip) rvn_paint_push_clip(painter, rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h));
	rvn_paint_image(painter, rect, image->texture, uv, tint, image->radius);
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
		if (image->texture == R_INVALID_ID) return R_FALSE;
	} else if (RVN_ATTR("Tint")) {
		if (!rvn_parse_color(value, &image->tint)) return R_FALSE;
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
	.set_attribute = rvn_image_set_attribute
};

void Rimage_set_texture(Rid id, Rid texture) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_IMAGE);
	if (texture != R_INVALID_ID && !rvn_texture_get(texture)) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rimage_set_texture: invalid texture");
		return;
	}
	((rvn_image_t*)widget)->texture = texture;
	rvn_widget_invalidate_layout(widget);
}

void Rimage_set_tint_color(Rid id, float r, float g, float b, float a) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_IMAGE);
	((rvn_image_t*)widget)->tint = rvn_rgba(r, g, b, a);
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
