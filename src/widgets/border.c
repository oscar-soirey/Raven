/**
 * Raven - widgets/border.c
 * Border (Slate SBorder): one child with a background, an optional outline,
 * rounded corners, a drop shadow and inner padding.
 */

#include "common.h"
#include "../render/renderer.h"
#include "../xml/xml.h"

typedef struct rvn_border_t {
	rvn_widget_t base;
	rvn_margin_t padding;
	float radii[4];
	int has_radii;
	float outline_width;
	rvn_color_t outline_color;
	float shadow_blur;
	rvn_vec2_t shadow_offset;
	/* an effect as the background (raven_shader.h), or R_INVALID_ID */
	Rid effect;
	RShaderLook look;
	/* the parts of the look the XML gave (the others come from the effect) */
	int look_color, look_color2, look_params;
	float params[4];
	int param_count;
	/* Effect="file.spv": its path, and whether it reads what is behind */
	char* effect_file;
	int effect_backdrop;
} rvn_border_t;

/* The look of the effect: the defaults of the effect, then what the XML or the code gave. */
static void rvn_border_apply_look(rvn_border_t* border, const RShaderLook* given) {
	RShaderLook look;
	int i;
	Rshader_look_defaults(border->effect, &look);
	if (given) {
		border->look = *given;
		return;
	}
	if (border->look_color) memcpy(look.color, border->look.color, sizeof(look.color));
	if (border->look_color2) memcpy(look.color2, border->look.color2, sizeof(look.color2));
	for (i = 0; i < border->param_count && i < 4; i++) look.params[i] = border->params[i];
	look.animate = border->look.animate;
	look.pointer = border->look.pointer;
	border->look = look;
}

static void rvn_border_load_effect_file(rvn_border_t* border) {
	if (!border->effect_file) return;
	border->effect = rvn_shader_from_file_cached(border->effect_file, R_SHADER_ANIMATED | (border->effect_backdrop ? R_SHADER_BACKDROP : 0u));
	rvn_border_apply_look(border, NULL);
}

static void rvn_border_init(rvn_widget_t* widget) {
	rvn_border_t* border = (rvn_border_t*)widget;
	border->effect = R_INVALID_ID;
	Rshader_look_init(&border->look);
}

static void rvn_border_destroy(rvn_widget_t* widget) {
	rvn_free(((rvn_border_t*)widget)->effect_file);
}


static rvn_vec2_t rvn_border_measure(rvn_widget_t* widget) {
	rvn_border_t* border = (rvn_border_t*)widget;
	rvn_vec2_t desired = rvn_vec2(0.0f, 0.0f);
	if (widget->child_count > 0) desired = rvn_widget_measure_slot(widget->children[0]);
	desired.x += border->padding.l + border->padding.r;
	desired.y += border->padding.t + border->padding.b;
	return desired;
}

static void rvn_border_arrange(rvn_widget_t* widget) {
	rvn_border_t* border = (rvn_border_t*)widget;
	if (widget->child_count > 0 && !rvn_widget_collapsed(widget->children[0])) {
		rvn_widget_arrange_slot(widget->children[0],
			rvn_rect_inset(rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h), border->padding));
	}
}

/* The shadow and the glow of the border: its own Shadow, then what a theme gives it
 * (<Style Widget="Border" ShadowColor="#f00" ShadowSize="20" Glow="1"/>). */
typedef struct rvn_border_drop_t {
	float blur;
	rvn_vec2_t offset;
	rvn_color_t color;
	float glow;
} rvn_border_drop_t;

static void rvn_border_drop(rvn_widget_t* widget, RPainter* painter, rvn_border_drop_t* drop) {
	rvn_border_t* border = (rvn_border_t*)widget;
	drop->blur = border->shadow_blur;
	drop->offset = border->shadow_offset;
	drop->color = rvn_widget_color(widget, R_COLOR_SHADOW);
	drop->glow = 0.0f;
	if (rvn_part_styled(widget, RVN_PART_BODY)) {
		rvn_part_look_t look;
		rvn_part_look_clear(&look);
		look.shadow_color = drop->color;
		rvn_part_apply_styles(widget, RVN_PART_BODY, rvn_widget_states(widget, painter, (widget->flags & RVN_WF_HOVERED) != 0, R_FALSE), &look);
		drop->color = look.shadow_color;
		/* (the Shadow of the element comes first) */
		if (drop->blur <= 0.0f && look.shadow > 0.0f) {
			drop->blur = look.shadow;
			drop->offset = rvn_vec2(0.0f, 2.0f);
		}
		drop->glow = look.glow;
	}
}

static void rvn_border_paint_drop(rvn_widget_t* widget, RPainter* painter, const rvn_border_drop_t* drop, rvn_rect_t rect, float radius) {
	if (drop->blur > 0.0f) {
		rvn_paint_shadow(painter, rvn_rect(drop->offset.x, drop->offset.y, rect.w, rect.h), radius, drop->blur, drop->color);
	}
	if (drop->glow > 0.0f) {
		/* a light of its color around it (the accent when it has none) */
		float glow = rvn_minf(drop->glow, 1.0f);
		rvn_color_t color = rvn_widget_color(widget, R_COLOR_PANEL);
		if (rvn_color_a(color) == 0) color = rvn_widget_color(widget, R_COLOR_ACCENT);
		rvn_paint_shadow(painter, rect, radius, 6.0f + 12.0f * glow, rvn_color_fade(color | 0xFF000000u, rvn_clampf(0.6f * glow, 0.0f, 0.9f)));
	}
}

static void rvn_border_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_border_t* border = (rvn_border_t*)widget;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	rvn_border_drop_t drop;
	float radii[4];
	int i;

	/* (a border without a color is only a padding) */
	if (rvn_color_a(rvn_widget_color(widget, R_COLOR_PANEL)) == 0 && border->outline_width <= 0.0f && border->effect == R_INVALID_ID) return;
	rvn_border_drop(widget, painter, &drop);
	if (border->effect != R_INVALID_ID && Rshader_is_valid(border->effect)) {
		/* the effect is the background: corners as given (else those of a style), the outline over it */
		for (i = 0; i < 4; i++) {
			radii[i] = border->has_radii ? border->radii[i] : (rvn_widget_has_metric(widget, R_METRIC_CORNER_RADIUS) ? rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS) : 0.0f);
		}
		rvn_border_paint_drop(widget, painter, &drop, rect, radii[0]);
		if (rvn_widget_has_color(widget, R_COLOR_PANEL)) rvn_paint_box4(painter, rect, radii, rvn_widget_color(widget, R_COLOR_PANEL), 0.0f, 0);
		{
			/* the pointer: over the border (or what it holds) and pressing it, eased */
			RShaderLook look = border->look;
			if (look.pointer) {
				float px = 0.0f, py = 0.0f;
				uint32_t buttons = 0;
				rvn_box_t box = rvn_box_intersect(rvn_paint_snap(painter, rect), painter->clip);
				/* Effects follow the visible rectangle, like Rdraw_shader. An overlaid
				 * sibling becoming the hit target must not pulse the effect beneath it. */
				int over = painter->window && rvn_window_pointer(painter->window, &px, &py, &buttons);
				int down;
				px *= painter->scale;
				py *= painter->scale;
				over = over && px >= box.x0 && py >= box.y0 && px < box.x1 && py < box.y1;
				down = over && (buttons & (1u << R_MOUSE_LEFT));
				if (look.hover < 0.0f) look.hover = rvn_widget_hover(widget, painter, over);
				if (look.press < 0.0f) look.press = rvn_widget_press(widget, painter, down);
			}
			rvn_paint_shader(painter, rect, radii, border->effect, &look);
		}
		if (border->outline_width > 0.0f) rvn_paint_box4(painter, rect, radii, 0, border->outline_width, border->outline_color);
		return;
	}
	if (rvn_widget_glass(widget)) {
		rvn_glass_t glass;
		rvn_glass_style(widget, RVN_SURFACE_PANEL, 0.0f, 0.0f, &glass);
		/* a panel of glass is rounded unless told otherwise */
		for (i = 0; i < 4; i++) {
			radii[i] = border->has_radii ? border->radii[i] : (rvn_widget_has_metric(widget, R_METRIC_CORNER_RADIUS) ? rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS) : 20.0f);
		}
		glass.shadow = drop.blur;
		rvn_paint_glass(painter, rect, radii, &glass);
		return;
	}
	/* square unless it was given corners: its own, or those of a style ("card") */
	for (i = 0; i < 4; i++) {
		radii[i] = border->has_radii ? border->radii[i] : (rvn_widget_has_metric(widget, R_METRIC_CORNER_RADIUS) ? rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS) : 0.0f);
	}
	rvn_border_paint_drop(widget, painter, &drop, rect, radii[0]);
	rvn_paint_box4(painter, rect, radii, rvn_widget_color(widget, R_COLOR_PANEL),
		border->outline_width, border->outline_width > 0.0f ? border->outline_color : 0);
}

static int rvn_border_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_border_t* border = (rvn_border_t*)widget;
	rvn_color_t color;
	float values[4];
	int count;

	if (RVN_ATTR("Padding")) {
		if (!rvn_parse_margin(value, &border->padding)) return R_FALSE;
		rvn_widget_invalidate_layout(widget);
	} else if (RVN_ATTR("Background")) {
		if (!rvn_parse_color(value, &color)) return R_FALSE;
		rvn_widget_set_color(widget, R_COLOR_PANEL, color);
	} else if (RVN_ATTR("BorderColor")) {
		if (!rvn_parse_color(value, &border->outline_color)) return R_FALSE;
		if (border->outline_width <= 0.0f) border->outline_width = 1.0f;
	} else if (RVN_ATTR("BorderWidth")) {
		if (!rvn_parse_float(value, &border->outline_width)) return R_FALSE;
		if (rvn_color_a(border->outline_color) == 0) border->outline_color = rvn_widget_color(widget, R_COLOR_BORDER);
	} else if (RVN_ATTR("CornerRadius")) {
		count = rvn_parse_floats(value, values, 4);
		if (count != 1 && count != 4) return R_FALSE;
		border->radii[0] = values[0];
		border->radii[1] = count == 4 ? values[1] : values[0];
		border->radii[2] = count == 4 ? values[2] : values[0];
		border->radii[3] = count == 4 ? values[3] : values[0];
		border->has_radii = R_TRUE;
	} else if (RVN_ATTR("Effect")) {
		rvn_free(border->effect_file);
		border->effect_file = NULL;
		if (!value[0] || rvn_stricmp(value, "none") == 0) {
			border->effect = R_INVALID_ID;
		} else {
			border->effect = Reffect_find(value);
			if (border->effect == R_INVALID_ID) {
				/* a .spv file, from the folder of the XML file */
				border->effect_file = rvn_xml_resolve_path(value);
				rvn_border_load_effect_file(border);
				if (border->effect == R_INVALID_ID) return R_FALSE;
			}
			rvn_border_apply_look(border, NULL);
		}
	} else if (RVN_ATTR("EffectColor") || RVN_ATTR("EffectColor2")) {
		int second = RVN_ATTR("EffectColor2");
		if (!rvn_parse_color(value, &color)) return R_FALSE;
		{
			float* out = second ? border->look.color2 : border->look.color;
			rvn_color_to_floats(color, &out[0], &out[1], &out[2], &out[3]);
		}
		if (second) border->look_color2 = R_TRUE;
		else border->look_color = R_TRUE;
		if (border->effect != R_INVALID_ID) rvn_border_apply_look(border, NULL);
	} else if (RVN_ATTR("EffectParams")) {
		border->param_count = rvn_parse_floats(value, border->params, 4);
		if (border->param_count < 1) return R_FALSE;
		if (border->effect != R_INVALID_ID) rvn_border_apply_look(border, NULL);
	} else if (RVN_ATTR("EffectAnimate")) {
		int animate;
		if (!rvn_parse_bool(value, &animate)) return R_FALSE;
		border->look.animate = animate;
	} else if (RVN_ATTR("EffectPointer")) {
		if (!rvn_parse_bool(value, &border->look.pointer)) return R_FALSE;
	} else if (RVN_ATTR("EffectBackdrop")) {
		if (!rvn_parse_bool(value, &border->effect_backdrop)) return R_FALSE;
		rvn_border_load_effect_file(border);
	} else if (RVN_ATTR("Shadow")) {
		count = rvn_parse_floats(value, values, 3);
		if (count < 1) return R_FALSE;
		border->shadow_blur = values[0];
		border->shadow_offset = rvn_vec2(count > 1 ? values[1] : 0.0f, count > 2 ? values[2] : 2.0f);
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_paint(widget);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_border = {
	.type = R_WIDGET_BORDER,
	.name = "Border",
	.size = sizeof(rvn_border_t),
	.flags = RVN_CF_SINGLE_CHILD | RVN_CF_SURFACE,
	.measure = rvn_border_measure,
	.arrange = rvn_border_arrange,
	.paint = rvn_border_paint,
	.set_attribute = rvn_border_set_attribute,
	.destroy = rvn_border_destroy,
	.init = rvn_border_init
};

void Rborder_set_effect(Rid id, Rid shader, const RShaderLook* look) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_BORDER);
	rvn_border_t* border = (rvn_border_t*)widget;
	rvn_free(border->effect_file);
	border->effect_file = NULL;
	border->effect = Rshader_is_valid(shader) ? shader : R_INVALID_ID;
	if (border->effect != R_INVALID_ID) rvn_border_apply_look(border, look);
	rvn_widget_invalidate_paint(widget);
}

Rid Rborder_get_effect(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_BORDER, R_INVALID_ID);
	return ((rvn_border_t*)widget)->effect;
}

void Rborder_set_padding(Rid id, float left, float top, float right, float bottom) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_BORDER);
	((rvn_border_t*)widget)->padding = rvn_margin(left, top, right, bottom);
	rvn_widget_invalidate_layout(widget);
}

void Rborder_set_background(Rid id, float r, float g, float b, float a) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_BORDER);
	rvn_widget_set_color(widget, R_COLOR_PANEL, rvn_rgba(r, g, b, a));
}

void Rborder_set_outline(Rid id, float width, float r, float g, float b, float a) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_BORDER);
	((rvn_border_t*)widget)->outline_width = width > 0.0f ? width : 0.0f;
	((rvn_border_t*)widget)->outline_color = rvn_rgba(r, g, b, a);
	rvn_widget_invalidate_paint(widget);
}

void Rborder_set_corner_radius(Rid id, float top_left, float top_right, float bottom_right, float bottom_left) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_BORDER);
	rvn_border_t* border = (rvn_border_t*)widget;
	border->radii[0] = top_left;
	border->radii[1] = top_right;
	border->radii[2] = bottom_right;
	border->radii[3] = bottom_left;
	border->has_radii = R_TRUE;
	rvn_widget_invalidate_paint(widget);
}

void Rborder_set_material(Rid id, RMaterial material) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_BORDER);
	Rwidget_set_material(id, material);
}

void Rborder_set_shadow(Rid id, float blur, float offset_x, float offset_y) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_BORDER);
	((rvn_border_t*)widget)->shadow_blur = blur > 0.0f ? blur : 0.0f;
	((rvn_border_t*)widget)->shadow_offset = rvn_vec2(offset_x, offset_y);
	rvn_widget_invalidate_paint(widget);
}
