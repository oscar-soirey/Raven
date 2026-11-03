/**
 * Raven - widget/look.c
 * Parts and states of widgets, and what the looks of the platforms need.
 *
 * A widget is drawn in parts (a slider: its track, what fills it, its thumb).
 * For each part and the states it is in, the widget asks the look of its
 * theme for a rvn_part_look_t (rvn_part_look): the family of the look gives
 * the base (a function of the widget, from the colors and metrics of the
 * theme), then the styles of the theme for that part and those states are
 * laid over it, the most specific last:
 *
 *   <Style Widget="Slider" Part="Thumb" State="Hovered" Background="#FFB340"/>
 *
 * The states that come and go with the pointer (HOVERED, PRESSED) are eased:
 * rvn_part_look_eased blends the looks of the states with the springs of the
 * widget, so a color given to a state fades in as the ones of the look do.
 */

#if !defined(_WIN32) && !defined(_DEFAULT_SOURCE)
#define _DEFAULT_SOURCE /* popen, pclose */
#endif

#include "widget.h"
#include "../widgets/common.h"
#include "../render/font.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#endif

/* ---------------------------------------------------------------------------
 *  Names
 * ------------------------------------------------------------------------- */

static const char* const g_part_names[RVN_PART_COUNT] = {
	"Body", "Box", "Mark", "Radio", "Dot", "Track", "Fill", "Thumb", "Knob", "Arrow", "Popup", "Item",
	"Indicator", "Tab", "Header", "Scrollbar", "ScrollThumb", "Tooltip", "CaptionButton", "CloseButton", "Focus"
};

int rvn_part_by_name(const char* name, int* part) {
	int i;
	if (!name) return R_FALSE;
	for (i = 0; i < RVN_PART_COUNT; i++) {
		if (rvn_stricmp(g_part_names[i], name) == 0) {
			*part = i;
			return R_TRUE;
		}
	}
	/* (a few other words people reach for) */
	if (rvn_stricmp(name, "Handle") == 0) *part = RVN_PART_THUMB;
	else if (rvn_stricmp(name, "Check") == 0 || rvn_stricmp(name, "CheckMark") == 0) *part = RVN_PART_MARK;
	else if (rvn_stricmp(name, "Row") == 0) *part = RVN_PART_ITEM;
	else if (rvn_stricmp(name, "Chevron") == 0) *part = RVN_PART_ARROW;
	else if (rvn_stricmp(name, "Rail") == 0 || rvn_stricmp(name, "Groove") == 0) *part = RVN_PART_TRACK;
	else return R_FALSE;
	return R_TRUE;
}

const char* rvn_part_name(int part) {
	return part >= 0 && part < RVN_PART_COUNT ? g_part_names[part] : "Body";
}

static const struct { const char* name; uint32_t bit; } g_state_names[] = {
	{ "Hovered", RVN_STATE_HOVERED }, { "Pressed", RVN_STATE_PRESSED }, { "Focused", RVN_STATE_FOCUSED },
	{ "Disabled", RVN_STATE_DISABLED }, { "Checked", RVN_STATE_CHECKED }, { "Selected", RVN_STATE_SELECTED },
	{ "Open", RVN_STATE_OPEN }, { "Primary", RVN_STATE_PRIMARY }, { "Flat", RVN_STATE_FLAT }
};

int rvn_states_parse(const char* text, uint32_t* states, char* bad, size_t bad_size) {
	*states = 0;
	while (*text) {
		const char* start;
		size_t length, i;
		int found = R_FALSE;
		while (*text == ' ' || *text == '\t' || *text == ',' || *text == '|') text++;
		start = text;
		while (*text && *text != ' ' && *text != '\t' && *text != ',' && *text != '|') text++;
		length = (size_t)(text - start);
		if (length == 0) break;
		for (i = 0; i < RVN_COUNT_OF(g_state_names); i++) {
			if (strlen(g_state_names[i].name) == length && rvn_strnicmp(g_state_names[i].name, start, length) == 0) {
				*states |= g_state_names[i].bit;
				found = R_TRUE;
				break;
			}
		}
		/* (Hover, Press, Focus, Checked by another name) */
		if (!found) {
			if (length == 5 && rvn_strnicmp(start, "Hover", 5) == 0) *states |= RVN_STATE_HOVERED;
			else if (length == 5 && rvn_strnicmp(start, "Press", 5) == 0) *states |= RVN_STATE_PRESSED;
			else if (length == 5 && rvn_strnicmp(start, "Focus", 5) == 0) *states |= RVN_STATE_FOCUSED;
			else if (length == 2 && rvn_strnicmp(start, "On", 2) == 0) *states |= RVN_STATE_CHECKED;
			else {
				if (bad && bad_size) {
					size_t copy = length < bad_size - 1 ? length : bad_size - 1;
					memcpy(bad, start, copy);
					bad[copy] = '\0';
				}
				return R_FALSE;
			}
		}
	}
	return R_TRUE;
}

void rvn_states_write(uint32_t states, rvn_text_t* out) {
	size_t i;
	int first = R_TRUE;
	for (i = 0; i < RVN_COUNT_OF(g_state_names); i++) {
		if (!(states & g_state_names[i].bit)) continue;
		if (!first) rvn_text_add(out, " ");
		rvn_text_add(out, g_state_names[i].name);
		first = R_FALSE;
	}
}

/* In the order of the RVN_LOOK_ bits. */
static const char* const g_property_names[12] = {
	"Background", "Foreground", "BorderColor", "BorderTopColor", "BorderBottomColor", "ShadowColor",
	"BorderWidth", "BorderBottomWidth", "CornerRadius", "Size", "Opacity", "ShadowSize"
};

const char* rvn_part_property_name(uint32_t bit) {
	int i;
	for (i = 0; i < 12; i++) {
		if (bit == (1u << i)) return g_property_names[i];
	}
	return "";
}

int rvn_part_property(const char* name, const char* value, rvn_part_look_t* look, uint32_t* bit, int* bad_value) {
	rvn_color_t* colors[6];
	float* numbers[6];
	int i;
	colors[0] = &look->background;
	colors[1] = &look->foreground;
	colors[2] = &look->border;
	colors[3] = &look->border_top;
	colors[4] = &look->border_bottom;
	colors[5] = &look->shadow_color;
	numbers[0] = &look->border_width;
	numbers[1] = &look->border_bottom_width;
	numbers[2] = &look->radius;
	numbers[3] = &look->size;
	numbers[4] = &look->opacity;
	numbers[5] = &look->shadow;
	*bad_value = R_FALSE;
	/* (a few shorter names) */
	if (rvn_stricmp(name, "Color") == 0 || rvn_stricmp(name, "Fill") == 0) name = "Background";
	else if (rvn_stricmp(name, "Border") == 0) name = "BorderColor";
	else if (rvn_stricmp(name, "Radius") == 0) name = "CornerRadius";
	else if (rvn_stricmp(name, "Shadow") == 0) name = "ShadowSize";
	for (i = 0; i < 12; i++) {
		if (rvn_stricmp(g_property_names[i], name) != 0) continue;
		*bit = 1u << i;
		if (i < 6) {
			/* "None" and "Transparent": nothing drawn */
			if (rvn_stricmp(value, "None") == 0 || rvn_stricmp(value, "Transparent") == 0) *colors[i] = 0;
			else if (!rvn_parse_color(value, colors[i])) *bad_value = R_TRUE;
		} else {
			if (!rvn_parse_float(value, numbers[i - 6]) || *numbers[i - 6] < 0.0f) *bad_value = R_TRUE;
		}
		return R_TRUE;
	}
	return R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Looks of the parts
 * ------------------------------------------------------------------------- */

void rvn_part_look_clear(rvn_part_look_t* look) {
	memset(look, 0, sizeof(*look));
	look->opacity = 1.0f;
}

uint32_t rvn_widget_states(const rvn_widget_t* widget, const RPainter* painter, int hovered, int pressed) {
	uint32_t states = 0;
	if (hovered) states |= RVN_STATE_HOVERED;
	if (pressed) states |= RVN_STATE_PRESSED;
	if (widget->flags & RVN_WF_FOCUSED) states |= RVN_STATE_FOCUSED;
	if (painter && !painter->enabled) states |= RVN_STATE_DISABLED;
	return states;
}

int rvn_part_styled(const rvn_widget_t* widget, int part) {
	const rvn_style_match_t* match = rvn_widget_match(widget);
	const rvn_theme_t* theme = rvn_widget_theme(widget);
	uint32_t i;
	if (!match) return R_FALSE;
	for (i = 0; i < match->part_rule_count; i++) {
		if (theme->rules[match->part_rules[i]].part == part) return R_TRUE;
	}
	return R_FALSE;
}

void rvn_part_apply_styles(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_look_t* look) {
	const rvn_style_match_t* match = rvn_widget_match(widget);
	const rvn_theme_t* theme = rvn_widget_theme(widget);
	uint32_t i;
	if (!match) return;
	for (i = 0; i < match->part_rule_count; i++) {
		const rvn_style_rule_t* rule = &theme->rules[match->part_rules[i]];
		uint32_t mask = rule->look_mask;
		if (rule->part != part || (rule->states & ~states)) continue;
		if (mask & RVN_LOOK_BACKGROUND) look->background = rule->look.background;
		if (mask & RVN_LOOK_FOREGROUND) look->foreground = rule->look.foreground;
		if (mask & RVN_LOOK_BORDER) look->border = rule->look.border;
		if (mask & RVN_LOOK_BORDER_TOP) look->border_top = rule->look.border_top;
		if (mask & RVN_LOOK_BORDER_BOTTOM) look->border_bottom = rule->look.border_bottom;
		if (mask & RVN_LOOK_SHADOW_COLOR) look->shadow_color = rule->look.shadow_color;
		if (mask & RVN_LOOK_BORDER_WIDTH) look->border_width = rule->look.border_width;
		if (mask & RVN_LOOK_BOTTOM_WIDTH) look->border_bottom_width = rule->look.border_bottom_width;
		if (mask & RVN_LOOK_RADIUS) look->radius = rule->look.radius;
		if (mask & RVN_LOOK_SIZE) look->size = rule->look.size;
		if (mask & RVN_LOOK_OPACITY) look->opacity = rule->look.opacity;
		if (mask & RVN_LOOK_SHADOW) look->shadow = rule->look.shadow;
		/* a border color given alone is the whole border: the edges of the look do not stay */
		if ((mask & RVN_LOOK_BORDER) && !(mask & RVN_LOOK_BORDER_TOP)) look->border_top = 0;
		if ((mask & RVN_LOOK_BORDER) && !(mask & RVN_LOOK_BORDER_BOTTOM)) look->border_bottom = 0;
		if ((mask & (RVN_LOOK_BORDER | RVN_LOOK_BORDER_TOP | RVN_LOOK_BORDER_BOTTOM)) && !(mask & RVN_LOOK_BORDER_WIDTH) &&
			look->border_width <= 0.0f) look->border_width = 1.0f;
	}
}

void rvn_part_look(const rvn_widget_t* widget, int part, uint32_t states, rvn_part_base_t base, const void* context, rvn_part_look_t* look) {
	rvn_part_look_clear(look);
	if (base) base(widget, part, states, look, context);
	rvn_part_apply_styles(widget, part, states, look);
}

/* A color between two, its transparency included; a transparent end takes the color of the other. */
static rvn_color_t rvn_look_mix_color(rvn_color_t a, rvn_color_t b, float t) {
	if (rvn_color_a(a) == 0 && rvn_color_a(b) == 0) return 0;
	if (rvn_color_a(a) == 0) a = b & 0x00FFFFFFu;
	if (rvn_color_a(b) == 0) b = a & 0x00FFFFFFu;
	return rvn_color_mix(a, b, t);
}

void rvn_part_look_mix(rvn_part_look_t* a, const rvn_part_look_t* b, float t) {
	t = rvn_clampf(t, 0.0f, 1.0f);
	if (t <= 0.0f) return;
	if (t >= 1.0f) {
		*a = *b;
		return;
	}
	a->background = rvn_look_mix_color(a->background, b->background, t);
	a->foreground = rvn_look_mix_color(a->foreground, b->foreground, t);
	a->border = rvn_look_mix_color(a->border, b->border, t);
	a->border_top = rvn_look_mix_color(a->border_top, b->border_top, t);
	a->border_bottom = rvn_look_mix_color(a->border_bottom, b->border_bottom, t);
	a->shadow_color = rvn_look_mix_color(a->shadow_color, b->shadow_color, t);
	a->border_width += (b->border_width - a->border_width) * t;
	a->border_bottom_width += (b->border_bottom_width - a->border_bottom_width) * t;
	a->radius += (b->radius - a->radius) * t;
	a->size += (b->size - a->size) * t;
	a->opacity += (b->opacity - a->opacity) * t;
	a->shadow += (b->shadow - a->shadow) * t;
}

void rvn_part_look_eased(const rvn_widget_t* widget, int part, uint32_t states, float hover, float press,
	rvn_part_base_t base, const void* context, rvn_part_look_t* look) {

	uint32_t rest = states & ~(RVN_STATE_HOVERED | RVN_STATE_PRESSED);
	rvn_part_look_t other;
	hover = rvn_clampf(hover, 0.0f, 1.0f);
	press = rvn_clampf(press, 0.0f, 1.0f);
	rvn_part_look(widget, part, rest, base, context, look);
	if (hover > 0.001f) {
		rvn_part_look(widget, part, rest | RVN_STATE_HOVERED, base, context, &other);
		rvn_part_look_mix(look, &other, hover);
	}
	if (press > 0.001f) {
		rvn_part_look(widget, part, rest | RVN_STATE_HOVERED | RVN_STATE_PRESSED, base, context, &other);
		rvn_part_look_mix(look, &other, press);
	}
}

/* ---------------------------------------------------------------------------
 *  Drawing a part
 * ------------------------------------------------------------------------- */

/* The outline of a rounded rectangle, only inside a band of it. */
static void rvn_paint_border_band(RPainter* painter, rvn_rect_t rect, float radius, float width, rvn_color_t color, rvn_rect_t band) {
	if (!rvn_color_a(color) || width <= 0.0f || band.h <= 0.0f) return;
	rvn_paint_push_clip(painter, band);
	rvn_paint_box(painter, rect, radius, 0, width, color);
	rvn_paint_pop_clip(painter);
}

void rvn_paint_part(RPainter* painter, rvn_rect_t rect, const rvn_part_look_t* look) {
	float opacity = painter->opacity;
	float radius = rvn_minf(look->radius, rvn_minf(rect.w, rect.h) * 0.5f);
	float width = look->border_width;
	int edges = (rvn_color_a(look->border_top) && look->border_top != look->border) ||
		(rvn_color_a(look->border_bottom) && look->border_bottom != look->border) || look->border_bottom_width > width;

	if (rect.w <= 0.0f || rect.h <= 0.0f) return;
	painter->opacity *= rvn_clampf(look->opacity, 0.0f, 1.0f);
	if (look->shadow > 0.0f && rvn_color_a(look->shadow_color)) {
		rvn_paint_shadow(painter, rvn_rect(rect.x, rect.y + look->shadow * 0.25f, rect.w, rect.h), radius, look->shadow, look->shadow_color);
	}
	if (!edges) {
		if (rvn_color_a(look->background) || (width > 0.0f && rvn_color_a(look->border))) {
			rvn_paint_box(painter, rect, radius, look->background, rvn_color_a(look->border) ? width : 0.0f, look->border);
		}
	} else {
		/*
		 * Edges of their own (the controls of Windows: a darker line at the
		 * bottom, or a lighter one at the top in the dark): the outline is
		 * drawn in three bands, the top, the middle and the bottom.
		 */
		float bottom_width = rvn_maxf(look->border_bottom_width, width);
		float top_band = rvn_color_a(look->border_top) ? rvn_maxf(width, 1.0f) + 0.5f : 0.0f;
		float bottom_band = (rvn_color_a(look->border_bottom) || look->border_bottom_width > width) ? bottom_width + 0.5f : 0.0f;
		rvn_color_t bottom = rvn_color_a(look->border_bottom) ? look->border_bottom : look->border;
		if (rvn_color_a(look->background)) rvn_paint_box(painter, rect, radius, look->background, 0.0f, 0);
		rvn_paint_border_band(painter, rect, radius, width, look->border_top, rvn_rect(rect.x - 1.0f, rect.y - 1.0f, rect.w + 2.0f, top_band + 1.0f));
		rvn_paint_border_band(painter, rect, radius, width, look->border,
			rvn_rect(rect.x - 1.0f, rect.y + top_band, rect.w + 2.0f, rect.h - top_band - bottom_band));
		if (look->border_bottom_width > width) {
			/* a thicker bottom line (a focused field): drawn as a bar under the rounding */
			rvn_rect_t band = rvn_rect(rect.x - 1.0f, rect.y + rect.h - bottom_band, rect.w + 2.0f, bottom_band + 1.0f);
			rvn_paint_border_band(painter, rect, radius, width, look->border, band);
			rvn_paint_push_clip(painter, rvn_rect(rect.x, rect.y + rect.h - bottom_width, rect.w, bottom_width));
			rvn_paint_box(painter, rect, radius, bottom, 0.0f, 0);
			rvn_paint_pop_clip(painter);
		} else {
			rvn_paint_border_band(painter, rect, radius, width, bottom, rvn_rect(rect.x - 1.0f, rect.y + rect.h - bottom_band, rect.w + 2.0f, bottom_band + 1.0f));
		}
	}
	painter->opacity = opacity;
}

/* ---------------------------------------------------------------------------
 *  Motion
 * ------------------------------------------------------------------------- */

int rvn_widget_springy(const rvn_widget_t* widget) {
	return rvn_widget_theme(widget)->motion != RVN_MOTION_SMOOTH;
}

RMotion rvn_widget_motion(const rvn_widget_t* widget, RMotion motion) {
	if (rvn_widget_springy(widget)) return motion;
	/* the platforms do not bounce: short transitions that settle at once */
	return motion == R_MOTION_SMOOTH ? R_MOTION_SMOOTH : R_MOTION_SNAPPY;
}

/* ---------------------------------------------------------------------------
 *  Fonts of the platforms
 *
 *  Each system has its own and only that one: Segoe UI on Windows, San
 *  Francisco on macOS, the font of the desktop on Linux (GNOME: Adwaita
 *  Sans, Cantarell, Ubuntu...). The look of a platform shown elsewhere
 *  keeps the font of Raven.
 * ------------------------------------------------------------------------- */

static int rvn_look_file_exists(const char* path) {
	FILE* probe = fopen(path, "rb");
	if (!probe) return R_FALSE;
	fclose(probe);
	return R_TRUE;
}

static Rid rvn_look_open_path(const char* path) {
	if (!path || !path[0] || !rvn_look_file_exists(path)) return R_INVALID_ID;
	return Rfont_create_from_file(path);
}

#if !defined(_WIN32) && !defined(__APPLE__)
/* The first line a command writes, without its end; R_FALSE when it fails or writes nothing. */
static int rvn_look_run(const char* command, char* out, size_t size) {
	FILE* pipe = popen(command, "r");
	size_t length;
	if (!pipe) return R_FALSE;
	if (!fgets(out, (int)size, pipe)) out[0] = '\0';
	pclose(pipe);
	length = strlen(out);
	while (length > 0 && (out[length - 1] == '\n' || out[length - 1] == '\r' || out[length - 1] == ' ')) out[--length] = '\0';
	return length > 0;
}

/* A key of the settings of GNOME, without the quotes gsettings writes around strings. */
static int rvn_gnome_setting(const char* schema, const char* key, char* out, size_t size) {
	char command[256];
	size_t length;
	snprintf(command, sizeof(command), "gsettings get %s %s 2>/dev/null", schema, key);
	if (!rvn_look_run(command, out, size)) return R_FALSE;
	length = strlen(out);
	if (length >= 2 && out[0] == '\'' && out[length - 1] == '\'') {
		memmove(out, out + 1, length - 2);
		out[length - 2] = '\0';
	}
	return out[0] != '\0';
}
#endif

static Rid rvn_look_open_system_font(int family, float* points) {
	*points = 0.0f;
#if defined(_WIN32)
	if (family == RVN_FAMILY_WINDOWS) {
		/* Segoe UI (Windows 11 shows Segoe UI Variable, drawn the same at the size of controls) */
		static const char* const files[] = { "segoeui.ttf", "SegUIVar.ttf" };
		char folder[MAX_PATH], path[MAX_PATH + 64];
		UINT length = GetWindowsDirectoryA(folder, MAX_PATH);
		size_t i;
		if (length == 0 || length >= MAX_PATH - 32) return R_INVALID_ID;
		for (i = 0; i < RVN_COUNT_OF(files); i++) {
			Rid font;
			snprintf(path, sizeof(path), "%s\\Fonts\\%s", folder, files[i]);
			font = rvn_look_open_path(path);
			if (font != R_INVALID_ID) return font;
		}
	}
#elif defined(__APPLE__)
	if (family == RVN_FAMILY_MACOS) {
		/* San Francisco; Helvetica Neue before it */
		static const char* const files[] = {
			"/System/Library/Fonts/SFNS.ttf", "/System/Library/Fonts/SFNSText.ttf", "/System/Library/Fonts/HelveticaNeue.ttc"
		};
		size_t i;
		for (i = 0; i < RVN_COUNT_OF(files); i++) {
			Rid font = rvn_look_open_path(files[i]);
			if (font != R_INVALID_ID) return font;
		}
	}
#else
	if (family == RVN_FAMILY_GNOME) {
		/* the font of the desktop ("Cantarell 11"), found by fontconfig; else the fonts GNOME ships */
		static const char* const files[] = {
			"/usr/share/fonts/adwaita-sans-fonts/AdwaitaSans-Regular.ttf", "/usr/share/fonts/truetype/adwaita-sans/AdwaitaSans-Regular.ttf",
			"/usr/share/fonts/cantarell/Cantarell-VF.otf", "/usr/share/fonts/opentype/cantarell/Cantarell-VF.otf",
			"/usr/share/fonts/cantarell/Cantarell-Regular.otf", "/usr/share/fonts/opentype/cantarell/Cantarell-Regular.otf",
			"/usr/share/fonts/TTF/Cantarell-Regular.ttf", "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf"
		};
		char setting[200], command[300], path[512];
		size_t i;
		if (rvn_gnome_setting("org.gnome.desktop.interface", "font-name", setting, sizeof(setting))) {
			/* "Family Name 11": the size is the last word */
			char* space = strrchr(setting, ' ');
			if (space && atof(space + 1) > 4.0) {
				*points = (float)atof(space + 1);
				*space = '\0';
			}
			if (!strchr(setting, '\'') && !strchr(setting, '"')) {
				snprintf(command, sizeof(command), "fc-match -f '%%{file}' '%s:style=Regular' 2>/dev/null", setting);
				if (rvn_look_run(command, path, sizeof(path))) {
					Rid font = rvn_look_open_path(path);
					if (font != R_INVALID_ID) return font;
				}
			}
		}
		for (i = 0; i < RVN_COUNT_OF(files); i++) {
			Rid font = rvn_look_open_path(files[i]);
			if (font != R_INVALID_ID) return font;
		}
	}
#endif
	(void)family;
	return R_INVALID_ID;
}

static Rid rvn_look_open_icon_font(int family) {
#if defined(_WIN32)
	/* Segoe Fluent Icons (Windows 11), Segoe MDL2 Assets (Windows 10): the same code points */
	static const char* const files[] = { "SegoeIcons.ttf", "segmdl2.ttf" };
	char folder[MAX_PATH], path[MAX_PATH + 64];
	UINT length = GetWindowsDirectoryA(folder, MAX_PATH);
	size_t i;
	if (family != RVN_FAMILY_WINDOWS || length == 0 || length >= MAX_PATH - 32) return R_INVALID_ID;
	for (i = 0; i < RVN_COUNT_OF(files); i++) {
		Rid font;
		snprintf(path, sizeof(path), "%s\\Fonts\\%s", folder, files[i]);
		font = rvn_look_open_path(path);
		if (font != R_INVALID_ID) return font;
	}
#endif
	/* (macOS and GNOME draw their symbols as vectors: SF Symbols and the symbolic icons are not fonts to open) */
	(void)family;
	return R_INVALID_ID;
}

static void rvn_look_open_fonts(int family) {
	if (g_rvn.platform_fonts_tried) return;
	g_rvn.platform_fonts_tried = R_TRUE;
	g_rvn.platform_font = rvn_look_open_system_font(family, &g_rvn.platform_font_points);
	g_rvn.platform_icon_font = rvn_look_open_icon_font(family);
	/* characters the font of the system lacks come from the font of Raven, then from the system */
	if (g_rvn.platform_font != R_INVALID_ID && g_rvn.default_font != R_INVALID_ID) Rfont_set_fallback(g_rvn.platform_font, g_rvn.default_font);
}

/* The family of the platform this program is built for: only its fonts exist here. */
static int rvn_look_home_family(void) {
#if defined(_WIN32)
	return RVN_FAMILY_WINDOWS;
#elif defined(__APPLE__)
	return RVN_FAMILY_MACOS;
#else
	return RVN_FAMILY_GNOME;
#endif
}

Rid rvn_look_platform_font(int family) {
	if (family != rvn_look_home_family() || !g_rvn.initialized || g_rvn.default_font == R_INVALID_ID) return R_INVALID_ID;
	rvn_look_open_fonts(family);
	return g_rvn.platform_font;
}

Rid rvn_look_icon_font(int family) {
	if (family != rvn_look_home_family() || !g_rvn.initialized || g_rvn.default_font == R_INVALID_ID) return R_INVALID_ID;
	rvn_look_open_fonts(family);
	return g_rvn.platform_icon_font;
}

int rvn_paint_look_icon(RPainter* painter, const rvn_widget_t* widget, uint32_t codepoint, float cx, float cy, float size, rvn_color_t color) {
	Rid font = rvn_look_icon_font(rvn_widget_family(widget));
	char text[5];
	int length;
	if (font == R_INVALID_ID) return R_FALSE;
	length = (int)rvn_utf8_encode(codepoint, text);
	if (length <= 0) return R_FALSE;
	/* the glyphs of the icon fonts fill their em square: centered as a square of that size */
	rvn_paint_text_in(painter, font, size, rvn_rect(cx - size, cy - size * 0.5f, size * 2.0f, size), 0.5f, text, length, color);
	return R_TRUE;
}

void rvn_look_shutdown(void) {
	g_rvn.platform_font = R_INVALID_ID;
	g_rvn.platform_icon_font = R_INVALID_ID;
	g_rvn.platform_fonts_tried = R_FALSE;
	g_rvn.platform_font_points = 0.0f;
}

/* ---------------------------------------------------------------------------
 *  The settings of the system: accent color, size of the text
 * ------------------------------------------------------------------------- */

#if defined(_WIN32)
typedef LONG (WINAPI *rvn_reg_open_t)(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
typedef LONG (WINAPI *rvn_reg_query_t)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef LONG (WINAPI *rvn_reg_close_t)(HKEY);

/*
 * The shades of the accent color the user chose, as Windows keeps them
 * (HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Accent,
 * AccentPalette): eight colors, the lightest first; 3 is the accent itself.
 */
static int rvn_win32_accent_palette(uint32_t shades[8]) {
	HMODULE advapi = LoadLibraryW(L"advapi32.dll");
	rvn_reg_open_t open_key;
	rvn_reg_query_t query;
	rvn_reg_close_t close_key;
	HKEY key;
	BYTE data[32];
	DWORD size = sizeof(data), type = 0;
	int ok = R_FALSE, i;
	if (!advapi) return R_FALSE;
	open_key = (rvn_reg_open_t)(void*)GetProcAddress(advapi, "RegOpenKeyExW");
	query = (rvn_reg_query_t)(void*)GetProcAddress(advapi, "RegQueryValueExW");
	close_key = (rvn_reg_close_t)(void*)GetProcAddress(advapi, "RegCloseKey");
	if (open_key && query && close_key &&
		open_key(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent", 0, KEY_READ, &key) == ERROR_SUCCESS) {
		if (query(key, L"AccentPalette", NULL, &type, data, &size) == ERROR_SUCCESS && type == REG_BINARY && size >= 32) {
			for (i = 0; i < 8; i++) shades[i] = ((uint32_t)data[i * 4] << 16) | ((uint32_t)data[i * 4 + 1] << 8) | (uint32_t)data[i * 4 + 2];
			ok = R_TRUE;
		}
		close_key(key);
	}
	FreeLibrary(advapi);
	return ok;
}
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
/* The accent of GNOME (47 and later: org.gnome.desktop.interface accent-color), as libadwaita draws it. */
static int rvn_gnome_accent(uint32_t* rgb) {
	static const struct { const char* name; uint32_t rgb; } accents[] = {
		{ "blue", 0x3584E4 }, { "teal", 0x2190A4 }, { "green", 0x3A944A }, { "yellow", 0xC88800 }, { "orange", 0xED5B00 },
		{ "red", 0xE62D42 }, { "pink", 0xD56199 }, { "purple", 0x9141AC }, { "slate", 0x6F8396 }
	};
	char value[64];
	size_t i;
	if (!rvn_gnome_setting("org.gnome.desktop.interface", "accent-color", value, sizeof(value))) return R_FALSE;
	for (i = 0; i < RVN_COUNT_OF(accents); i++) {
		if (rvn_stricmp(value, accents[i].name) == 0) {
			*rgb = accents[i].rgb;
			return R_TRUE;
		}
	}
	return R_FALSE;
}
#endif

void rvn_look_system_accent(rvn_theme_t* theme) {
	if (theme->family == RVN_FAMILY_RAVEN || theme->family != rvn_look_home_family() || !g_rvn.initialized) return;
#if defined(_WIN32)
	{
		uint32_t shades[8];
		if (!rvn_win32_accent_palette(shades)) return;
		/* what Windows 11 itself uses: Dark1 on light surfaces, Light2 on dark ones */
		rvn_theme_set_accent(theme, rvn_hex(theme->dark ? shades[1] : shades[4], 255));
		theme->colors[(int)R_COLOR_LINK - (int)R_COLOR_WINDOW] = rvn_hex(theme->dark ? shades[0] : shades[5], 255);
	}
#elif defined(__APPLE__)
	{
		float rgb[3];
		if (g_rvn.platform && g_rvn.platform->accent_color && g_rvn.platform->accent_color(rgb)) {
			rvn_theme_set_accent(theme, rvn_rgba(rgb[0], rgb[1], rgb[2], 1.0f));
		}
	}
#else
	{
		uint32_t rgb;
		if (rvn_gnome_accent(&rgb)) rvn_theme_set_accent(theme, rvn_hex(rgb, 255));
		/* the size of the text of the desktop ("Cantarell 11": 11 points, at 96 per inch) */
		rvn_look_platform_font(theme->family);
		if (g_rvn.platform_font_points > 4.0f && g_rvn.platform_font_points < 40.0f) {
			theme->metrics[(int)R_METRIC_FONT_SIZE - (int)R_METRIC_FONT_SIZE] = g_rvn.platform_font_points * 96.0f / 72.0f;
		}
	}
#endif
}
