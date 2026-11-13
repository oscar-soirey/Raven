/**
 * Raven - xml/xml_attributes.c
 * Text to value conversions, and the attributes every widget understands
 * (name, visibility, size, slot, style overrides, action).
 *
 * Number parsing does not depend on the C locale: "1.5" is always valid,
 * whatever setlocale() the application called.
 */

#include "../widgets/common.h"
#include "xml.h"

/* ---------------------------------------------------------------------------
 *  Values
 * ------------------------------------------------------------------------- */

static const char* rvn_skip_spaces(const char* text) {
	while (*text == ' ' || *text == '\t' || *text == '\n' || *text == '\r') text++;
	return text;
}

/* Reads a number at *cursor and advances it. Returns R_FALSE when there is none. */
static int rvn_read_number(const char** cursor, float* value) {
	const char* p = rvn_skip_spaces(*cursor);
	double result = 0.0, scale = 1.0;
	int negative = 0, digits = 0;

	if (*p == '-' || *p == '+') {
		negative = *p == '-';
		p++;
	}
	while (*p >= '0' && *p <= '9') {
		result = result * 10.0 + (double)(*p - '0');
		digits++;
		p++;
	}
	if (*p == '.') {
		p++;
		while (*p >= '0' && *p <= '9') {
			scale *= 0.1;
			result += (double)(*p - '0') * scale;
			digits++;
			p++;
		}
	}
	if (digits == 0) return R_FALSE;
	if (*p == 'e' || *p == 'E') {
		const char* e = p + 1;
		int exponent = 0, exponent_negative = 0, exponent_digits = 0;
		if (*e == '-' || *e == '+') {
			exponent_negative = *e == '-';
			e++;
		}
		while (*e >= '0' && *e <= '9' && exponent < 100) {
			exponent = exponent * 10 + (*e - '0');
			exponent_digits++;
			e++;
		}
		if (exponent_digits > 0) {
			result *= pow(10.0, exponent_negative ? -exponent : exponent);
			p = e;
		}
	}
	*value = (float)(negative ? -result : result);
	*cursor = p;
	return R_TRUE;
}

int rvn_parse_float(const char* text, float* value) {
	float number;
	if (!text || !rvn_read_number(&text, &number)) return R_FALSE;
	if (*rvn_skip_spaces(text) != '\0') return R_FALSE;
	*value = number;
	return R_TRUE;
}

int rvn_parse_int(const char* text, int* value) {
	float number;
	if (!rvn_parse_float(text, &number)) return R_FALSE;
	number = number < 0.0f ? number - 0.5f : number + 0.5f;
	/* out of the range of an int, the conversion would be undefined */
	if (!(number >= -2147483648.0f && number < 2147483648.0f)) return R_FALSE;
	*value = (int)number;
	return R_TRUE;
}

int rvn_parse_bool(const char* text, int* value) {
	if (!text) return R_FALSE;
	if (rvn_stricmp(text, "true") == 0 || rvn_stricmp(text, "yes") == 0 || strcmp(text, "1") == 0) *value = R_TRUE;
	else if (rvn_stricmp(text, "false") == 0 || rvn_stricmp(text, "no") == 0 || strcmp(text, "0") == 0) *value = R_FALSE;
	else return R_FALSE;
	return R_TRUE;
}

int rvn_parse_floats(const char* text, float* values, int max) {
	int count = 0;
	if (!text) return 0;
	while (count < max) {
		if (!rvn_read_number(&text, &values[count])) return 0;
		count++;
		text = rvn_skip_spaces(text);
		if (*text == '\0') return count;
		/* numbers are separated by a comma or by spaces */
		if (*text == ',') text++;
	}
	return *rvn_skip_spaces(text) == '\0' ? count : 0;
}

int rvn_parse_margin(const char* text, rvn_margin_t* margin) {
	float values[4];
	switch (rvn_parse_floats(text, values, 4)) {
		case 1:
			*margin = rvn_margin(values[0], values[0], values[0], values[0]);
			return R_TRUE;
		case 2:
			*margin = rvn_margin(values[0], values[1], values[0], values[1]);
			return R_TRUE;
		case 4:
			*margin = rvn_margin(values[0], values[1], values[2], values[3]);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static int rvn_hex_digit(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

int rvn_parse_color(const char* text, rvn_color_t* color) {
	static const struct { const char* name; uint32_t rgb; uint32_t alpha; } named[] = {
		{ "transparent", 0x000000, 0 }, { "white", 0xFFFFFF, 255 }, { "black", 0x000000, 255 },
		{ "red", 0xE5484D, 255 }, { "green", 0x46A758, 255 }, { "blue", 0x3B82F6, 255 },
		{ "yellow", 0xF5D90A, 255 }, { "orange", 0xF76B15, 255 }, { "cyan", 0x05A2C2, 255 },
		{ "magenta", 0xD6409F, 255 }, { "gray", 0x808080, 255 }, { "grey", 0x808080, 255 }
	};
	uint32_t digits[8];
	size_t length, i;

	if (!text) return R_FALSE;
	text = rvn_skip_spaces(text);
	if (text[0] != '#') {
		for (i = 0; i < RVN_COUNT_OF(named); i++) {
			if (rvn_stricmp(text, named[i].name) == 0) {
				*color = rvn_hex(named[i].rgb, named[i].alpha);
				return R_TRUE;
			}
		}
		return R_FALSE;
	}
	text++;
	length = strlen(text);
	if (length != 3 && length != 4 && length != 6 && length != 8) return R_FALSE;
	for (i = 0; i < length; i++) {
		int digit = rvn_hex_digit(text[i]);
		if (digit < 0) return R_FALSE;
		digits[i] = (uint32_t)digit;
	}
	if (length <= 4) {
		/* #RGB and #RGBA: each digit is doubled */
		*color = rvn_rgba8(digits[0] * 17u, digits[1] * 17u, digits[2] * 17u, length == 4 ? digits[3] * 17u : 255u);
	} else {
		*color = rvn_rgba8(digits[0] * 16u + digits[1], digits[2] * 16u + digits[3], digits[4] * 16u + digits[5],
			length == 8 ? digits[6] * 16u + digits[7] : 255u);
	}
	return R_TRUE;
}

int rvn_parse_orientation(const char* text, ROrientation* orientation) {
	if (rvn_stricmp(text, "Horizontal") == 0) *orientation = R_HORIZONTAL;
	else if (rvn_stricmp(text, "Vertical") == 0) *orientation = R_VERTICAL;
	else return R_FALSE;
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Names of the theme values ("Color.Accent", "Metric.FontSize")
 * ------------------------------------------------------------------------- */

static const struct { const char* name; RColorRole role; } g_color_names[] = {
	{ "Window", R_COLOR_WINDOW }, { "Panel", R_COLOR_PANEL }, { "Border", R_COLOR_BORDER },
	{ "Text", R_COLOR_TEXT }, { "TextDisabled", R_COLOR_TEXT_DISABLED }, { "TextPlaceholder", R_COLOR_TEXT_PLACEHOLDER },
	{ "Accent", R_COLOR_ACCENT }, { "AccentHovered", R_COLOR_ACCENT_HOVERED }, { "AccentPressed", R_COLOR_ACCENT_PRESSED },
	{ "AccentText", R_COLOR_ACCENT_TEXT }, { "Control", R_COLOR_CONTROL }, { "ControlHovered", R_COLOR_CONTROL_HOVERED },
	{ "ControlPressed", R_COLOR_CONTROL_PRESSED }, { "ControlBorder", R_COLOR_CONTROL_BORDER }, { "Input", R_COLOR_INPUT },
	{ "Focus", R_COLOR_FOCUS }, { "Selection", R_COLOR_SELECTION }, { "RowHovered", R_COLOR_ROW_HOVERED },
	{ "Track", R_COLOR_TRACK }, { "Handle", R_COLOR_HANDLE }, { "HandleHovered", R_COLOR_HANDLE_HOVERED },
	{ "Separator", R_COLOR_SEPARATOR }, { "Header", R_COLOR_HEADER }, { "HeaderHovered", R_COLOR_HEADER_HOVERED },
	{ "TabActive", R_COLOR_TAB_ACTIVE }, { "Popup", R_COLOR_POPUP }, { "PopupBorder", R_COLOR_POPUP_BORDER },
	{ "Shadow", R_COLOR_SHADOW }, { "Tooltip", R_COLOR_TOOLTIP }, { "TooltipText", R_COLOR_TOOLTIP_TEXT }, { "Switch", R_COLOR_SWITCH },
	{ "Success", R_COLOR_SUCCESS }, { "Warning", R_COLOR_WARNING }, { "Danger", R_COLOR_DANGER }, { "Link", R_COLOR_LINK },
	{ "Code", R_COLOR_CODE }
};

static const struct { const char* name; RMetric metric; } g_metric_names[] = {
	{ "FontSize", R_METRIC_FONT_SIZE }, { "CornerRadius", R_METRIC_CORNER_RADIUS }, { "BorderWidth", R_METRIC_BORDER_WIDTH },
	{ "ControlHeight", R_METRIC_CONTROL_HEIGHT }, { "PaddingX", R_METRIC_PADDING_X }, { "PaddingY", R_METRIC_PADDING_Y },
	{ "Spacing", R_METRIC_SPACING }, { "ScrollbarSize", R_METRIC_SCROLLBAR_SIZE }, { "RowHeight", R_METRIC_ROW_HEIGHT },
	{ "Indent", R_METRIC_INDENT }, { "SplitterSize", R_METRIC_SPLITTER_SIZE }, { "CheckSize", R_METRIC_CHECK_SIZE },
	{ "SliderHandleSize", R_METRIC_SLIDER_HANDLE_SIZE }, { "PopupRadius", R_METRIC_POPUP_RADIUS },
	{ "GlassFrost", R_METRIC_GLASS_FROST }, { "GlassRefraction", R_METRIC_GLASS_REFRACTION },
	{ "GlassSaturation", R_METRIC_GLASS_SATURATION }, { "GlassRim", R_METRIC_GLASS_RIM },
	{ "GlassShadow", R_METRIC_GLASS_SHADOW }, { "BlurRadius", R_METRIC_BLUR_RADIUS }
};

int rvn_color_role_by_name(const char* name, RColorRole* role) {
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(g_color_names); i++) {
		if (rvn_stricmp(g_color_names[i].name, name) == 0) {
			*role = g_color_names[i].role;
			return R_TRUE;
		}
	}
	return R_FALSE;
}

int rvn_metric_by_name(const char* name, RMetric* metric) {
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(g_metric_names); i++) {
		if (rvn_stricmp(g_metric_names[i].name, name) == 0) {
			*metric = g_metric_names[i].metric;
			return R_TRUE;
		}
	}
	return R_FALSE;
}

const char* rvn_color_role_name(RColorRole role) {
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(g_color_names); i++) {
		if (g_color_names[i].role == role) return g_color_names[i].name;
	}
	return NULL;
}

const char* rvn_metric_name(RMetric metric) {
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(g_metric_names); i++) {
		if (g_metric_names[i].metric == metric) return g_metric_names[i].name;
	}
	return NULL;
}

/* ---------------------------------------------------------------------------
 *  Attributes shared by all widgets
 * ------------------------------------------------------------------------- */

static int rvn_parse_cursor(const char* value, RCursor* cursor) {
	static const struct { const char* name; RCursor cursor; } cursors[] = {
		{ "Arrow", R_CURSOR_ARROW }, { "IBeam", R_CURSOR_IBEAM }, { "Hand", R_CURSOR_HAND },
		{ "Crosshair", R_CURSOR_CROSSHAIR }, { "ResizeH", R_CURSOR_RESIZE_H }, { "ResizeV", R_CURSOR_RESIZE_V },
		{ "ResizeNWSE", R_CURSOR_RESIZE_NWSE }, { "ResizeNESW", R_CURSOR_RESIZE_NESW }, { "Move", R_CURSOR_MOVE },
		{ "NotAllowed", R_CURSOR_NOT_ALLOWED }, { "None", R_CURSOR_NONE }
	};
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(cursors); i++) {
		if (rvn_stricmp(value, cursors[i].name) == 0) {
			*cursor = cursors[i].cursor;
			return R_TRUE;
		}
	}
	return R_FALSE;
}

int rvn_parse_material(const char* text, RMaterial* material) {
	static const struct { const char* name; RMaterial material; } materials[] = {
		{ "Solid", R_MATERIAL_SOLID }, { "Glass", R_MATERIAL_GLASS }, { "ClearGlass", R_MATERIAL_GLASS },
		{ "FrostedGlass", R_MATERIAL_FROSTED_GLASS }, { "Frosted", R_MATERIAL_FROSTED_GLASS },
		{ "TintedGlass", R_MATERIAL_TINTED_GLASS }, { "Tinted", R_MATERIAL_TINTED_GLASS },
		{ "Default", R_MATERIAL_DEFAULT }, { "Theme", R_MATERIAL_DEFAULT }
	};
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(materials); i++) {
		if (rvn_stricmp(text, materials[i].name) == 0) {
			*material = materials[i].material;
			return R_TRUE;
		}
	}
	return R_FALSE;
}

/* "Slot.Xxx": how the widget is placed by its parent. `name` is the part after the dot. */
static int rvn_widget_set_slot_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_slot_t* slot = &widget->slot;
	float values[4];
	int flag, count;

	if (RVN_ATTR("Padding")) {
		if (!rvn_parse_margin(value, &slot->padding)) return R_FALSE;
	} else if (RVN_ATTR("HAlign")) {
		if (rvn_stricmp(value, "Fill") == 0) slot->halign = R_HALIGN_FILL;
		else if (rvn_stricmp(value, "Left") == 0) slot->halign = R_HALIGN_LEFT;
		else if (rvn_stricmp(value, "Center") == 0) slot->halign = R_HALIGN_CENTER;
		else if (rvn_stricmp(value, "Right") == 0) slot->halign = R_HALIGN_RIGHT;
		else return R_FALSE;
	} else if (RVN_ATTR("VAlign")) {
		if (rvn_stricmp(value, "Fill") == 0) slot->valign = R_VALIGN_FILL;
		else if (rvn_stricmp(value, "Top") == 0) slot->valign = R_VALIGN_TOP;
		else if (rvn_stricmp(value, "Center") == 0) slot->valign = R_VALIGN_CENTER;
		else if (rvn_stricmp(value, "Bottom") == 0) slot->valign = R_VALIGN_BOTTOM;
		else return R_FALSE;
	} else if (RVN_ATTR("Fill")) {
		if (!rvn_parse_float(value, &values[0]) || values[0] < 0.0f) return R_FALSE;
		slot->is_fill = R_TRUE;
		slot->fill = values[0];
	} else if (RVN_ATTR("Auto")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		slot->is_fill = !flag;
	} else if (RVN_ATTR("MaxSize")) {
		if (!rvn_parse_float(value, &slot->max_size)) return R_FALSE;
	} else if (RVN_ATTR("Column")) {
		/* a rejected value leaves the slot as it is; the limits are those of Rslot_set_grid(_span) */
		if (!rvn_parse_int(value, &count) || count < 0) return R_FALSE;
		slot->column = rvn_clampi(count, 0, 4095);
	} else if (RVN_ATTR("Row")) {
		if (!rvn_parse_int(value, &count) || count < 0) return R_FALSE;
		slot->row = rvn_clampi(count, 0, 4095);
	} else if (RVN_ATTR("ColumnSpan")) {
		if (!rvn_parse_int(value, &count) || count < 1) return R_FALSE;
		slot->column_span = rvn_clampi(count, 1, 4096);
	} else if (RVN_ATTR("RowSpan")) {
		if (!rvn_parse_int(value, &count) || count < 1) return R_FALSE;
		slot->row_span = rvn_clampi(count, 1, 4096);
	} else if (RVN_ATTR("Anchors")) {
		/* "x,y" pins the widget to a point, "minX,minY,maxX,maxY" stretches it */
		count = rvn_parse_floats(value, values, 4);
		if (count == 2) {
			values[2] = values[0];
			values[3] = values[1];
		} else if (count != 4) {
			return R_FALSE;
		}
		memcpy(slot->anchors, values, sizeof(slot->anchors));
	} else if (RVN_ATTR("Position")) {
		if (rvn_parse_floats(value, slot->position, 2) != 2) return R_FALSE;
	} else if (RVN_ATTR("Size")) {
		if (rvn_parse_floats(value, slot->size, 2) != 2) return R_FALSE;
	} else if (RVN_ATTR("Alignment")) {
		if (rvn_parse_floats(value, slot->alignment, 2) != 2) return R_FALSE;
	} else if (RVN_ATTR("Title")) {
		rvn_str_set(&slot->title, value);
	} else if (RVN_ATTR("Dock")) {
		if (rvn_stricmp(value, "Left") == 0) slot->dock = R_DOCK_LEFT;
		else if (rvn_stricmp(value, "Right") == 0) slot->dock = R_DOCK_RIGHT;
		else if (rvn_stricmp(value, "Top") == 0) slot->dock = R_DOCK_TOP;
		else if (rvn_stricmp(value, "Bottom") == 0) slot->dock = R_DOCK_BOTTOM;
		else if (rvn_stricmp(value, "Tab") == 0 || rvn_stricmp(value, "Center") == 0) slot->dock = R_DOCK_TAB;
		else return R_FALSE;
	} else if (RVN_ATTR("DockTo")) {
		rvn_str_set(&slot->dock_to, value[0] ? value : NULL);
	} else if (RVN_ATTR("DockSize")) {
		if (!rvn_parse_float(value, &slot->dock_size) || slot->dock_size < 0.0f || slot->dock_size >= 1.0f) return R_FALSE;
	} else if (RVN_ATTR("Closable")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		slot->dock_fixed = !flag;
	} else {
		return R_FALSE;
	}
	rvn_widget_invalidate_layout(widget->parent ? widget->parent : widget);
	return R_TRUE;
}

int rvn_widget_set_base_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_color_t color;
	RColorRole role;
	RMetric metric;
	float number;
	int flag;

	if (rvn_strnicmp(name, "Slot.", 5) == 0) return rvn_widget_set_slot_attribute(widget, name + 5, value);
	if (rvn_strnicmp(name, "Touch.", 6) == 0 || rvn_strnicmp(name, "Pen.", 4) == 0) {
		int handled = rvn_touch_attribute(widget, name, value);
		if (handled >= 0) return handled;
	}
	if (rvn_strnicmp(name, "Accessible", 10) == 0) {
		int handled = rvn_a11y_attribute(widget, name, value);
		if (handled >= 0) return handled;
	}

	if (rvn_strnicmp(name, "Color.", 6) == 0) {
		if (!rvn_color_role_by_name(name + 6, &role) || !rvn_parse_color(value, &color)) return R_FALSE;
		rvn_widget_set_color(widget, role, color);
		return R_TRUE;
	}
	if (rvn_strnicmp(name, "Metric.", 7) == 0) {
		if (!rvn_metric_by_name(name + 7, &metric) || !rvn_parse_float(value, &number)) return R_FALSE;
		rvn_widget_set_metric(widget, metric, number);
		return R_TRUE;
	}

	if (RVN_ATTR("Name") || RVN_ATTR("Id")) {
		rvn_str_set(&widget->name, value);
	} else if (RVN_ATTR("Visibility")) {
		RVisibility visibility;
		if (rvn_stricmp(value, "Visible") == 0) visibility = R_VISIBLE;
		else if (rvn_stricmp(value, "Hidden") == 0) visibility = R_HIDDEN;
		else if (rvn_stricmp(value, "Collapsed") == 0) visibility = R_COLLAPSED;
		else return R_FALSE;
		if (visibility != widget->visibility) {
			widget->visibility = visibility;
			rvn_a11y_structure_changed();
			if (visibility != R_VISIBLE && widget->window) rvn_window_forget_widget(widget->window, widget);
			rvn_widget_invalidate_layout(widget->parent ? widget->parent : widget);
		}
	} else if (RVN_ATTR("Enabled")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		if (flag) widget->flags &= ~RVN_WF_DISABLED;
		else widget->flags |= RVN_WF_DISABLED;
		rvn_widget_invalidate_paint(widget);
	} else if (RVN_ATTR("HitTestable")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		if (flag) widget->flags &= ~RVN_WF_NO_HIT;
		else widget->flags |= RVN_WF_NO_HIT;
	} else if (RVN_ATTR("Material")) {
		RMaterial material;
		if (!rvn_parse_material(value, &material)) return R_FALSE;
		widget->material = material == R_MATERIAL_DEFAULT ? (RMaterial)0 : material;
		rvn_widget_invalidate_layout(widget);
	} else if (RVN_ATTR("Class")) {
		if (rvn_str_set(&widget->classes, value[0] ? value : NULL)) {
			widget->style_generation = 0;
			rvn_widget_invalidate_layout(widget);
		}
	} else if (RVN_ATTR("Theme")) {
		const rvn_theme_t* theme = NULL;
		if (value[0] && rvn_stricmp(value, "Default") != 0 && rvn_stricmp(value, "Inherit") != 0) {
			theme = rvn_xml_theme(value);
			if (!theme) return R_FALSE;
		}
		rvn_widget_set_theme(widget, theme, theme != NULL);
	} else if (RVN_ATTR("WindowDrag")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		if (flag) {
			widget->flags |= RVN_WF_WINDOW_DRAG;
			widget->flags &= ~RVN_WF_NO_HIT;
		} else {
			widget->flags &= ~RVN_WF_WINDOW_DRAG;
		}
	} else if (RVN_ATTR("Focusable")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		if (flag) widget->flags |= RVN_WF_FOCUSABLE;
		else widget->flags &= ~RVN_WF_FOCUSABLE;
	} else if (RVN_ATTR("Clip")) {
		if (!rvn_parse_bool(value, &flag)) return R_FALSE;
		if (flag) widget->flags |= RVN_WF_CLIP;
		else widget->flags &= ~RVN_WF_CLIP;
		rvn_widget_invalidate_paint(widget);
	} else if (RVN_ATTR("Opacity")) {
		if (!rvn_parse_float(value, &number)) return R_FALSE;
		widget->opacity = rvn_clampf(number, 0.0f, 1.0f);
		rvn_widget_invalidate_paint(widget);
	} else if (RVN_ATTR("Appear")) {
		RTransition transition;
		if (!rvn_parse_transition(value, &transition)) return R_FALSE;
		rvn_widget_set_appear(widget, &transition, NULL);
	} else if (RVN_ATTR("AppearDelay")) {
		if (!rvn_parse_float(value, &number) || number < 0.0f) return R_FALSE;
		rvn_widget_set_appear(widget, NULL, &number);
	} else if (RVN_ATTR("Direction")) {
		RDirection direction;
		if (rvn_stricmp(value, "RTL") == 0 || rvn_stricmp(value, "RightToLeft") == 0) direction = R_DIRECTION_RTL;
		else if (rvn_stricmp(value, "LTR") == 0 || rvn_stricmp(value, "LeftToRight") == 0) direction = R_DIRECTION_LTR;
		else if (rvn_stricmp(value, "Inherit") == 0 || !value[0]) direction = R_DIRECTION_INHERIT;
		else return R_FALSE;
		Rwidget_set_direction(widget->id, direction);
	} else if (RVN_ATTR("Tooltip")) {
		rvn_str_set(&widget->tooltip, value[0] ? value : NULL);
	} else if (RVN_ATTR("Cursor")) {
		if (!rvn_parse_cursor(value, &widget->cursor)) return R_FALSE;
	} else if (RVN_ATTR("Width")) {
		if (!rvn_parse_float(value, &widget->fixed_size.x)) return R_FALSE;
		rvn_widget_invalidate_layout(widget);
	} else if (RVN_ATTR("Height")) {
		if (!rvn_parse_float(value, &widget->fixed_size.y)) return R_FALSE;
		rvn_widget_invalidate_layout(widget);
	} else if (RVN_ATTR("MinWidth")) {
		if (!rvn_parse_float(value, &widget->min_size.x)) return R_FALSE;
		rvn_widget_invalidate_layout(widget);
	} else if (RVN_ATTR("MinHeight")) {
		if (!rvn_parse_float(value, &widget->min_size.y)) return R_FALSE;
		rvn_widget_invalidate_layout(widget);
	} else if (RVN_ATTR("MaxWidth")) {
		if (!rvn_parse_float(value, &widget->max_size.x)) return R_FALSE;
		rvn_widget_invalidate_layout(widget);
	} else if (RVN_ATTR("MaxHeight")) {
		if (!rvn_parse_float(value, &widget->max_size.y)) return R_FALSE;
		rvn_widget_invalidate_layout(widget);
	} else if (RVN_ATTR("Action") || RVN_ATTR("OnClicked") || RVN_ATTR("OnChanged") ||
		RVN_ATTR("OnCommitted") || RVN_ATTR("OnActivated")) {
		rvn_str_set(&widget->action, value[0] ? value : NULL);
	} else {
		return R_FALSE;
	}
	return R_TRUE;
}

int rvn_widget_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	int localized;
	if (!name || !value) return R_FALSE;
	/* "@key": the translation of the key, following the language (core/locale.c) */
	if (rvn_locale_attribute(widget, name, value, &localized)) return localized;
	/* a widget built for a tag made by code: the code that made it knows its attributes */
	if (widget->xml_tag && rvn_xml_tag_attribute(widget, name, value)) return R_TRUE;
	/* the class goes first: "Text", "Spacing"... may mean something specific for it */
	if (widget->cls->set_attribute && widget->cls->set_attribute(widget, name, value)) return R_TRUE;
	return rvn_widget_set_base_attribute(widget, name, value);
}

int Rwidget_set_attribute(Rid id, const char* name, const char* value) {
	RVN_WIDGET(widget, id, 0, R_FALSE);
	if (!name || !value) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rwidget_set_attribute: NULL name or value");
		return R_FALSE;
	}
	return rvn_widget_set_attribute(widget, name, value);
}
