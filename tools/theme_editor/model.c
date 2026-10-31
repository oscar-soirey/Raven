/**
 * Raven theme editor - model.c
 * The theme being edited: what it holds, how it is written as a theme file
 * (docs/THEMES.md) and read back, the palette worked out from an accent, and
 * the contrast of two colors.
 */

#include "editor.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 *  Text
 * ------------------------------------------------------------------------- */

void te_addn(te_text* text, const char* s, size_t n) {
	if (text->length + n + 1 > text->capacity) {
		size_t capacity = text->capacity ? text->capacity * 2 : 256;
		char* data;
		while (capacity < text->length + n + 1) capacity *= 2;
		data = (char*)realloc(text->data, capacity);
		if (!data) return;
		text->data = data;
		text->capacity = capacity;
	}
	memcpy(text->data + text->length, s, n);
	text->length += n;
	text->data[text->length] = '\0';
}

void te_add(te_text* text, const char* s) {
	if (s) te_addn(text, s, strlen(s));
}

void te_printf(te_text* text, const char* format, ...) {
	char small[512];
	va_list args;
	int n;
	va_start(args, format);
	n = vsnprintf(small, sizeof(small), format, args);
	va_end(args);
	if (n < 0) return;
	if ((size_t)n < sizeof(small)) {
		te_addn(text, small, (size_t)n);
	} else {
		char* big = (char*)malloc((size_t)n + 1);
		if (!big) return;
		va_start(args, format);
		vsnprintf(big, (size_t)n + 1, format, args);
		va_end(args);
		te_addn(text, big, (size_t)n);
		free(big);
	}
}

void te_add_escaped(te_text* text, const char* s) {
	for (; s && *s; s++) {
		switch (*s) {
		case '&': te_add(text, "&amp;"); break;
		case '<': te_add(text, "&lt;"); break;
		case '>': te_add(text, "&gt;"); break;
		case '"': te_add(text, "&quot;"); break;
		default: te_addn(text, s, 1); break;
		}
	}
}

void te_text_free(te_text* text) {
	free(text->data);
	memset(text, 0, sizeof(*text));
}

char* te_strdup(const char* s) {
	size_t n = s ? strlen(s) : 0;
	char* copy = (char*)malloc(n + 1);
	if (!copy) return NULL;
	if (n) memcpy(copy, s, n);
	copy[n] = '\0';
	return copy;
}

void te_copy(char* dst, size_t size, const char* src) {
	if (!size) return;
	snprintf(dst, size, "%s", src ? src : "");
}

int te_stricmp(const char* a, const char* b) {
	for (;; a++, b++) {
		int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);
		if (ca != cb || !ca) return ca - cb;
	}
}

int te_contains(const char* haystack, const char* needle) {
	size_t n = strlen(needle), i;
	if (!n) return 1;
	for (; *haystack; haystack++) {
		for (i = 0; i < n; i++) {
			if (!haystack[i] || tolower((unsigned char)haystack[i]) != tolower((unsigned char)needle[i])) break;
		}
		if (i == n) return 1;
	}
	return 0;
}

const char* te_number(float value, char* buffer, size_t size) {
	char* end;
	snprintf(buffer, size, "%.3f", value);
	end = buffer + strlen(buffer) - 1;
	while (end > buffer && *end == '0') *end-- = '\0';
	if (end > buffer && *end == '.') *end = '\0';
	if (strcmp(buffer, "-0") == 0) te_copy(buffer, size, "0");
	return buffer;
}

/* ---------------------------------------------------------------------------
 *  Colors
 * ------------------------------------------------------------------------- */

static int hex_digit(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

int te_parse_hex(const char* text, float* rgba) {
	int digits[8], n = 0, i;
	if (!text) return 0;
	while (*text == ' ') text++;
	if (te_stricmp(text, "white") == 0) text = "#FFFFFF";
	else if (te_stricmp(text, "black") == 0) text = "#000000";
	else if (te_stricmp(text, "transparent") == 0) text = "#00000000";
	if (*text == '#') text++;
	for (; *text && *text != ' '; text++) {
		int d = hex_digit(*text);
		if (d < 0 || n == 8) return 0;
		digits[n++] = d;
	}
	if (n == 3 || n == 4) {
		for (i = 0; i < 4; i++) rgba[i] = i < n ? (float)(digits[i] * 17) / 255.0f : 1.0f;
		return 1;
	}
	if (n == 6 || n == 8) {
		for (i = 0; i < 4; i++) rgba[i] = i * 2 < n ? (float)(digits[i * 2] * 16 + digits[i * 2 + 1]) / 255.0f : 1.0f;
		return 1;
	}
	return 0;
}

static int byte_of(float v) {
	int b = (int)floorf(v * 255.0f + 0.5f);
	return b < 0 ? 0 : b > 255 ? 255 : b;
}

const char* te_hex(const float* rgba, char* buffer, size_t size) {
	if (byte_of(rgba[3]) == 255) snprintf(buffer, size, "#%02X%02X%02X", byte_of(rgba[0]), byte_of(rgba[1]), byte_of(rgba[2]));
	else snprintf(buffer, size, "#%02X%02X%02X%02X", byte_of(rgba[0]), byte_of(rgba[1]), byte_of(rgba[2]), byte_of(rgba[3]));
	return buffer;
}

static float clampf(float v, float lo, float hi) {
	return v < lo ? lo : v > hi ? hi : v;
}

void te_rgb_to_hsl(const float* rgb, float* h, float* s, float* l) {
	float mx = fmaxf(rgb[0], fmaxf(rgb[1], rgb[2]));
	float mn = fminf(rgb[0], fminf(rgb[1], rgb[2]));
	float d = mx - mn;
	*l = (mx + mn) * 0.5f;
	if (d < 1e-5f) {
		*h = 0.0f;
		*s = 0.0f;
		return;
	}
	*s = *l > 0.5f ? d / (2.0f - mx - mn) : d / (mx + mn);
	if (mx == rgb[0]) *h = (rgb[1] - rgb[2]) / d + (rgb[1] < rgb[2] ? 6.0f : 0.0f);
	else if (mx == rgb[1]) *h = (rgb[2] - rgb[0]) / d + 2.0f;
	else *h = (rgb[0] - rgb[1]) / d + 4.0f;
	*h *= 60.0f;
}

static float hue_channel(float p, float q, float t) {
	if (t < 0.0f) t += 1.0f;
	if (t > 1.0f) t -= 1.0f;
	if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
	if (t < 0.5f) return q;
	if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
	return p;
}

void te_hsl_to_rgb(float h, float s, float l, float* rgb) {
	float q, p;
	h = fmodf(h, 360.0f);
	if (h < 0.0f) h += 360.0f;
	s = clampf(s, 0.0f, 1.0f);
	l = clampf(l, 0.0f, 1.0f);
	if (s < 1e-5f) {
		rgb[0] = rgb[1] = rgb[2] = l;
		return;
	}
	q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
	p = 2.0f * l - q;
	rgb[0] = hue_channel(p, q, h / 360.0f + 1.0f / 3.0f);
	rgb[1] = hue_channel(p, q, h / 360.0f);
	rgb[2] = hue_channel(p, q, h / 360.0f - 1.0f / 3.0f);
}

void te_blend(const float* top, const float* bottom, float* out) {
	int i;
	for (i = 0; i < 3; i++) out[i] = top[i] * top[3] + bottom[i] * (1.0f - top[3]);
	out[3] = 1.0f;
}

static float luminance(const float* rgb) {
	float c[3];
	int i;
	for (i = 0; i < 3; i++) c[i] = rgb[i] <= 0.04045f ? rgb[i] / 12.92f : powf((rgb[i] + 0.055f) / 1.055f, 2.4f);
	return 0.2126f * c[0] + 0.7152f * c[1] + 0.0722f * c[2];
}

float te_contrast(const float* fg, const float* bg) {
	float top[4], a, b;
	te_blend(fg, bg, top);
	a = luminance(top);
	b = luminance(bg);
	return (fmaxf(a, b) + 0.05f) / (fminf(a, b) + 0.05f);
}

/* ---------------------------------------------------------------------------
 *  What a theme holds
 * ------------------------------------------------------------------------- */

const te_color_info g_te_colors[TE_COLOR_COUNT] = {
	{ R_COLOR_WINDOW, "Window", "Surfaces", "Behind everything: the background of windows" },
	{ R_COLOR_PANEL, "Panel", "Surfaces", "Panels, cards and side bars" },
	{ R_COLOR_BORDER, "Border", "Surfaces", "Outlines of panels and frames" },
	{ R_COLOR_SEPARATOR, "Separator", "Surfaces", "Lines between parts" },
	{ R_COLOR_SHADOW, "Shadow", "Surfaces", "Drop shadows (its alpha is their strength)" },
	{ R_COLOR_TEXT, "Text", "Text", "Most of the text" },
	{ R_COLOR_TEXT_DISABLED, "TextDisabled", "Text", "Text of disabled widgets" },
	{ R_COLOR_TEXT_PLACEHOLDER, "TextPlaceholder", "Text", "Hints in empty fields" },
	{ R_COLOR_LINK, "Link", "Text", "Links in labels and Markdown" },
	{ R_COLOR_CODE, "Code", "Text", "`code` in Markdown" },
	{ R_COLOR_ACCENT, "Accent", "Accent", "Primary buttons, checked boxes, sliders" },
	{ R_COLOR_ACCENT_HOVERED, "AccentHovered", "Accent", "The accent under the pointer" },
	{ R_COLOR_ACCENT_PRESSED, "AccentPressed", "Accent", "The accent while pressed" },
	{ R_COLOR_ACCENT_TEXT, "AccentText", "Accent", "Text and marks drawn on the accent" },
	{ R_COLOR_FOCUS, "Focus", "Accent", "The ring around the focused widget" },
	{ R_COLOR_SWITCH, "Switch", "Accent", "Switches that are on" },
	{ R_COLOR_CONTROL, "Control", "Controls", "Buttons, combo boxes, check boxes" },
	{ R_COLOR_CONTROL_HOVERED, "ControlHovered", "Controls", "A control under the pointer" },
	{ R_COLOR_CONTROL_PRESSED, "ControlPressed", "Controls", "A control while pressed" },
	{ R_COLOR_CONTROL_BORDER, "ControlBorder", "Controls", "Outlines of controls" },
	{ R_COLOR_INPUT, "Input", "Controls", "Text fields, spin boxes, editors" },
	{ R_COLOR_TRACK, "Track", "Controls", "Tracks of sliders, progress bars, scroll bars" },
	{ R_COLOR_HANDLE, "Handle", "Controls", "Scroll bar handles, splitters" },
	{ R_COLOR_HANDLE_HOVERED, "HandleHovered", "Controls", "A handle under the pointer" },
	{ R_COLOR_SELECTION, "Selection", "Lists and tabs", "Selected rows and text (often see-through)" },
	{ R_COLOR_ROW_HOVERED, "RowHovered", "Lists and tabs", "A row under the pointer" },
	{ R_COLOR_HEADER, "Header", "Lists and tabs", "Headers of tables, expanders, tab bars" },
	{ R_COLOR_HEADER_HOVERED, "HeaderHovered", "Lists and tabs", "A header under the pointer" },
	{ R_COLOR_TAB_ACTIVE, "TabActive", "Lists and tabs", "The tab in front" },
	{ R_COLOR_POPUP, "Popup", "Popups", "Menus and drop-down lists" },
	{ R_COLOR_POPUP_BORDER, "PopupBorder", "Popups", "Outlines of menus" },
	{ R_COLOR_TOOLTIP, "Tooltip", "Popups", "Background of tooltips" },
	{ R_COLOR_TOOLTIP_TEXT, "TooltipText", "Popups", "Text of tooltips" },
	{ R_COLOR_SUCCESS, "Success", "Status", "Things that went well" },
	{ R_COLOR_WARNING, "Warning", "Status", "Things to look at" },
	{ R_COLOR_DANGER, "Danger", "Status", "Errors, and buttons that destroy (Class=\"danger\")" }
};

const te_metric_info g_te_metrics[TE_METRIC_COUNT] = {
	{ R_METRIC_FONT_SIZE, "FontSize", "Font size", "Text and shapes", 8, 28, 0.5f, 1, 0, "Size of most of the text" },
	{ R_METRIC_CORNER_RADIUS, "CornerRadius", "Corner radius", "Text and shapes", 0, 24, 0.5f, 1, 0, "Roundness of controls" },
	{ R_METRIC_BORDER_WIDTH, "BorderWidth", "Border width", "Text and shapes", 0, 4, 0.5f, 1, 0, "Thickness of outlines" },
	{ R_METRIC_POPUP_RADIUS, "PopupRadius", "Popup radius", "Text and shapes", 0, 24, 0.5f, 1, 0, "Roundness of menus and tooltips" },
	{ R_METRIC_CONTROL_HEIGHT, "ControlHeight", "Control height", "Controls", 16, 48, 1, 0, 0, "Buttons, fields, combo boxes, tabs" },
	{ R_METRIC_PADDING_X, "PaddingX", "Padding across", "Controls", 0, 24, 1, 0, 0, "Space left and right inside controls" },
	{ R_METRIC_PADDING_Y, "PaddingY", "Padding down", "Controls", 0, 16, 1, 0, 0, "Space above and below inside controls" },
	{ R_METRIC_SPACING, "Spacing", "Spacing", "Controls", 0, 20, 1, 0, 0, "Between a mark and its text" },
	{ R_METRIC_CHECK_SIZE, "CheckSize", "Check size", "Controls", 10, 28, 1, 0, 0, "Check boxes and radio buttons" },
	{ R_METRIC_SLIDER_HANDLE_SIZE, "SliderHandleSize", "Slider knob", "Controls", 8, 30, 1, 0, 0, "The knob of sliders" },
	{ R_METRIC_ROW_HEIGHT, "RowHeight", "Row height", "Lists", 16, 48, 1, 0, 0, "Rows of lists, trees and menus" },
	{ R_METRIC_INDENT, "Indent", "Indent", "Lists", 6, 40, 1, 0, 0, "Per level of a tree" },
	{ R_METRIC_SCROLLBAR_SIZE, "ScrollbarSize", "Scroll bar", "Lists", 4, 20, 1, 0, 0, "Thickness of scroll bars" },
	{ R_METRIC_SPLITTER_SIZE, "SplitterSize", "Splitter", "Lists", 1, 12, 1, 0, 0, "Thickness of splitter handles" },
	{ R_METRIC_BLUR_RADIUS, "BlurRadius", "Blur radius", "Glass", 0, 64, 1, 0, 0, "Blur of a BackgroundBlur without a radius" },
	{ R_METRIC_GLASS_FROST, "Frost", "Frost", "Glass", 0, 3, 0.05f, 2, 1, "How much glass blurs (x the material)" },
	{ R_METRIC_GLASS_REFRACTION, "Refraction", "Refraction", "Glass", 0, 3, 0.05f, 2, 1, "How far its rim bends what is behind" },
	{ R_METRIC_GLASS_SATURATION, "Saturation", "Saturation", "Glass", 0, 3, 0.05f, 2, 1, "Of what is seen through" },
	{ R_METRIC_GLASS_RIM, "Rim", "Rim light", "Glass", 0, 3, 0.05f, 2, 1, "The light its edge catches" },
	{ R_METRIC_GLASS_SHADOW, "Shadow", "Shadow", "Glass", 0, 3, 0.05f, 2, 1, "The shadow it drops" }
};

const te_syntax_info g_te_syntax[TE_SYNTAX_COUNT] = {
	{ R_TOKEN_COMMENT, "Comment", "/* a comment */", 0x7F8C98, 0x6B7885 },
	{ R_TOKEN_STRING, "String", "\"text\"", 0xFC8E6D, 0xC41A16 },
	{ R_TOKEN_NUMBER, "Number", "42, 0.5f", 0xD9C97C, 0x1C00CF },
	{ R_TOKEN_KEYWORD, "Keyword", "if, return, static", 0xFF7AB2, 0x9B2393 },
	{ R_TOKEN_TYPE, "Type", "int, Rid", 0x5DD8FF, 0x0F68A0 },
	{ R_TOKEN_FUNCTION, "Function", "Rwindow_show()", 0x5DD8FF, 0x0F68A0 },
	{ R_TOKEN_TAG, "Tag", "<Button>", 0x6BDFFF, 0x0B4F79 },
	{ R_TOKEN_ATTRIBUTE, "Attribute", "Text=", 0xDABAFF, 0x6C36A9 },
	{ R_TOKEN_PUNCTUATION, "Punctuation", "{ } ; ,", 0x9AA5B1, 0x5C6873 },
	{ R_TOKEN_DIRECTIVE, "Directive", "#include", 0xFFA14F, 0x78492A },
	{ R_TOKEN_HEADING, "Heading", "# Markdown heading", 0x67B7A4, 0x326D74 },
	{ R_TOKEN_STRONG, "Strong", "**bold**", 0, 0 },
	{ R_TOKEN_EMPHASIS, "Emphasis", "*emphasis*", 0, 0 },
	{ R_TOKEN_LINK, "Link", "[a link](...)", 0x6CB6FF, 0x0B62D6 }
};

const te_look_info g_te_looks[] = {
	{ NULL, "Follows the application", R_LOOK_RAVEN_DARK, -1, 1 },
	{ "RavenDark", "Raven dark", R_LOOK_RAVEN_DARK, 1, 1 },
	{ "RavenLight", "Raven light", R_LOOK_RAVEN_LIGHT, 0, 1 },
	{ "RavenGlassDark", "Raven glass dark", R_LOOK_RAVEN_GLASS_DARK, 1, 1 },
	{ "RavenGlassLight", "Raven glass light", R_LOOK_RAVEN_GLASS_LIGHT, 0, 1 },
	{ "RavenHighContrastDark", "High contrast dark", R_LOOK_RAVEN_HIGH_CONTRAST_DARK, 1, 1 },
	{ "RavenHighContrastLight", "High contrast light", R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT, 0, 1 },
	{ "WindowsDark", "Windows 11 dark", R_LOOK_WINDOWS_DARK, 1, 0 },
	{ "WindowsLight", "Windows 11 light", R_LOOK_WINDOWS_LIGHT, 0, 0 },
	{ "MacOSDark", "macOS dark", R_LOOK_MACOS_DARK, 1, 0 },
	{ "MacOSLight", "macOS light", R_LOOK_MACOS_LIGHT, 0, 0 },
	{ "MacOSGlassDark", "macOS 26 Liquid Glass dark", R_LOOK_MACOS_GLASS_DARK, 1, 0 },
	{ "MacOSGlassLight", "macOS 26 Liquid Glass light", R_LOOK_MACOS_GLASS_LIGHT, 0, 0 },
	{ "GnomeDark", "GNOME dark", R_LOOK_GNOME_DARK, 1, 0 },
	{ "GnomeLight", "GNOME light", R_LOOK_GNOME_LIGHT, 0, 0 },
	{ "NativeDark", "The platform's, dark", R_LOOK_NATIVE_DARK, 1, 0 },
	{ "NativeLight", "The platform's, light", R_LOOK_NATIVE_LIGHT, 0, 0 }
};
const int g_te_look_count = (int)(sizeof(g_te_looks) / sizeof(g_te_looks[0]));

const char* const g_te_materials[] = { "", "Solid", "Glass", "FrostedGlass", "TintedGlass" };
const char* const g_te_motions[] = { "", "Springy", "Smooth" };
const char* const g_te_wallpapers[] = { "", "None", "Dark", "Light", "", "" };

/* ---------------------------------------------------------------------------
 *  The model
 * ------------------------------------------------------------------------- */

void te_model_init(te_model* model) {
	memset(model, 0, sizeof(*model));
	te_copy(model->name, sizeof(model->name), "My theme");
	model->look = 1;
	model->styles = te_strdup("");
}

void te_model_free(te_model* model) {
	free(model->styles);
	model->styles = NULL;
}

void te_model_copy(te_model* dst, const te_model* src) {
	char* styles = te_strdup(src->styles ? src->styles : "");
	free(dst->styles);
	*dst = *src;
	dst->styles = styles;
}

static int same_color(const te_color* a, const te_color* b) {
	return a->set == b->set && (!a->set || memcmp(a->c, b->c, sizeof(a->c)) == 0);
}

int te_model_equal(const te_model* a, const te_model* b) {
	int i;
	if (strcmp(a->name, b->name) || a->look != b->look || strcmp(a->base, b->base) || a->dark != b->dark ||
		a->material != b->material || a->motion != b->motion || a->wallpaper != b->wallpaper ||
		a->accent_set != b->accent_set || (a->accent_set && memcmp(a->accent, b->accent, sizeof(a->accent))) ||
		strcmp(a->wallpaper_text, b->wallpaper_text) || strcmp(a->font, b->font) ||
		strcmp(a->styles ? a->styles : "", b->styles ? b->styles : "")) return 0;
	for (i = 0; i < TE_COLOR_COUNT; i++) {
		if (!same_color(&a->colors[i], &b->colors[i])) return 0;
	}
	for (i = 0; i < TE_METRIC_COUNT; i++) {
		if (a->metric_set[i] != b->metric_set[i] || (a->metric_set[i] && a->metrics[i] != b->metrics[i])) return 0;
	}
	for (i = 0; i < TE_SYNTAX_COUNT; i++) {
		if (!same_color(&a->syntax[i], &b->syntax[i])) return 0;
	}
	return 1;
}

int te_model_is_dark(const te_model* model, int application_dark) {
	if (model->dark) return model->dark == 1;
	if (model->look > 0 && model->look < g_te_look_count) return g_te_looks[model->look].dark;
	return application_dark;
}

/* ---- writing ---- */

static int is_absolute(const char* path) {
	return path[0] == '/' || path[0] == '\\' || (path[0] && path[1] == ':');
}

static void add_path(te_text* out, const char* dir, const char* path) {
	if (dir && dir[0] && !is_absolute(path)) {
		te_add_escaped(out, dir);
		te_add(out, "/");
	}
	te_add_escaped(out, path);
}

static void add_color_attribute(te_text* out, const char* name, const float* rgba) {
	char hex[16];
	te_printf(out, "%s=\"%s\"", name, te_hex(rgba, hex, sizeof(hex)));
}

/* `count` attributes, three by line, in an element <tag .../> */
static void begin_list(te_text* out, const char* tag, int* written) {
	if (*written == 0) te_printf(out, "  <%s", tag);
	te_add(out, (*written % 3) == 0 ? "\n    " : " ");
	(*written)++;
}

static void end_list(te_text* out, int written) {
	if (written) te_add(out, "\n  />\n");
}

void te_model_write(const te_model* model, te_text* out, const char* name, int base_only, const char* dir) {
	char number[32];
	int i, n;
	if (!base_only) {
		te_add(out, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		te_add(out, "<!-- Made with the Raven theme editor. See docs/THEMES.md -->\n");
	}
	te_add(out, "<Theme Name=\"");
	te_add_escaped(out, name ? name : model->name);
	te_add(out, "\"");
	if (model->look > 0 && model->look < g_te_look_count) {
		te_printf(out, " Look=\"%s\"", g_te_looks[model->look].name);
	} else if (model->base[0]) {
		te_add(out, " Base=\"");
		te_add_escaped(out, model->base);
		te_add(out, "\"");
	}
	if (model->dark) te_printf(out, " Dark=\"%s\"", model->dark == 1 ? "true" : "false");
	if (model->accent_set) {
		float rgba[4];
		memcpy(rgba, model->accent, sizeof(model->accent));
		rgba[3] = 1.0f;
		te_add(out, " ");
		add_color_attribute(out, "Accent", rgba);
	}
	if (model->material > 0 && model->material < 5) te_printf(out, " Material=\"%s\"", g_te_materials[model->material]);
	if (model->motion > 0 && model->motion < 3) te_printf(out, " Motion=\"%s\"", g_te_motions[model->motion]);
	if (base_only) {
		te_add(out, "/>");
		return;
	}
	if (model->font[0]) {
		te_add(out, " Font=\"");
		add_path(out, dir, model->font);
		te_add(out, "\"");
	}
	switch (model->wallpaper) {
	case TE_WALLPAPER_NONE: case TE_WALLPAPER_DARK: case TE_WALLPAPER_LIGHT:
		te_printf(out, " Wallpaper=\"%s\"", g_te_wallpapers[model->wallpaper]);
		break;
	case TE_WALLPAPER_COLORS:
		if (model->wallpaper_text[0]) {
			te_add(out, " Wallpaper=\"");
			te_add_escaped(out, model->wallpaper_text);
			te_add(out, "\"");
		}
		break;
	case TE_WALLPAPER_IMAGE:
		if (model->wallpaper_text[0]) {
			te_add(out, " Wallpaper=\"");
			add_path(out, dir, model->wallpaper_text);
			te_add(out, "\"");
		}
		break;
	default:
		break;
	}
	te_add(out, ">\n");

	n = 0;
	for (i = 0; i < TE_COLOR_COUNT; i++) {
		if (!model->colors[i].set) continue;
		begin_list(out, "Colors", &n);
		add_color_attribute(out, g_te_colors[i].name, model->colors[i].c);
	}
	end_list(out, n);

	n = 0;
	for (i = 0; i < TE_METRIC_COUNT; i++) {
		if (!model->metric_set[i] || g_te_metrics[i].glass) continue;
		begin_list(out, "Metrics", &n);
		te_printf(out, "%s=\"%s\"", g_te_metrics[i].name, te_number(model->metrics[i], number, sizeof(number)));
	}
	end_list(out, n);

	n = 0;
	for (i = 0; i < TE_METRIC_COUNT; i++) {
		if (!model->metric_set[i] || !g_te_metrics[i].glass) continue;
		te_add(out, n++ ? " " : "  <Glass ");
		te_printf(out, "%s=\"%s\"", g_te_metrics[i].name, te_number(model->metrics[i], number, sizeof(number)));
	}
	if (n) te_add(out, "/>\n");

	n = 0;
	for (i = 0; i < TE_SYNTAX_COUNT; i++) {
		if (!model->syntax[i].set) continue;
		begin_list(out, "Syntax", &n);
		add_color_attribute(out, g_te_syntax[i].name, model->syntax[i].c);
	}
	end_list(out, n);

	/* the rules, each line indented */
	if (model->styles && model->styles[0]) {
		const char* line = model->styles;
		while (*line) {
			const char* end = strchr(line, '\n');
			size_t length = end ? (size_t)(end - line) : strlen(line);
			const char* start = line;
			while (length && (*start == ' ' || *start == '\t')) {
				start++;
				length--;
			}
			while (length && (start[length - 1] == ' ' || start[length - 1] == '\t' || start[length - 1] == '\r')) length--;
			if (length) {
				te_add(out, "  ");
				te_addn(out, start, length);
				te_add(out, "\n");
			}
			if (!end) break;
			line = end + 1;
		}
	}
	te_add(out, "</Theme>\n");
}

/* ---- reading ---- */

static int color_index(const char* name) {
	int i;
	/* (the name of Raven 0.2) */
	if (te_stricmp(name, "Background") == 0) name = "Window";
	for (i = 0; i < TE_COLOR_COUNT; i++) {
		if (te_stricmp(g_te_colors[i].name, name) == 0) return i;
	}
	return -1;
}

static int metric_index(const char* name, int glass) {
	int i;
	for (i = 0; i < TE_METRIC_COUNT; i++) {
		if (g_te_metrics[i].glass == glass && te_stricmp(g_te_metrics[i].name, name) == 0) return i;
	}
	return -1;
}

static int look_index(const char* name) {
	static const struct { const char* old; const char* name; } aliases[] = {
		{ "Dark", "RavenDark" }, { "Light", "RavenLight" }, { "GlassDark", "RavenGlassDark" }, { "GlassLight", "RavenGlassLight" },
		{ "HighContrastDark", "RavenHighContrastDark" }, { "HighContrastLight", "RavenHighContrastLight" },
		{ "Native", "NativeDark" }, { "Windows", "WindowsLight" }, { "macOS", "MacOSLight" }, { "GNOME", "GnomeLight" },
		{ "Tahoe", "MacOSGlassLight" }, { "LiquidGlass", "MacOSGlassLight" }, { "TahoeDark", "MacOSGlassDark" },
		{ "LiquidGlassDark", "MacOSGlassDark" }, { "TahoeLight", "MacOSGlassLight" }, { "LiquidGlassLight", "MacOSGlassLight" },
		{ "MacOSGlass", "MacOSGlassLight" }, { "Adwaita", "GnomeLight" }, { "AdwaitaDark", "GnomeDark" }, { "AdwaitaLight", "GnomeLight" }
	};
	size_t k;
	int i;
	for (k = 0; k < sizeof(aliases) / sizeof(aliases[0]); k++) {
		if (te_stricmp(aliases[k].old, name) == 0) {
			name = aliases[k].name;
			break;
		}
	}
	for (i = 1; i < g_te_look_count; i++) {
		if (te_stricmp(g_te_looks[i].name, name) == 0) return i;
	}
	return -1;
}

static int parse_number(const char* text, float* value) {
	char* end;
	double v = strtod(text, &end);
	if (end == text) return 0;
	*value = (float)v;
	return 1;
}

/* An element the editor does not edit, written back as it was read (its attributes). */
static void keep_element(te_text* out, Rid node) {
	int i, count = Rxml_node_get_attribute_count(node);
	te_printf(out, "<%s", Rxml_node_get_tag(node));
	for (i = 0; i < count; i++) {
		const char* name = Rxml_node_get_attribute_name(node, i);
		te_printf(out, " %s=\"", name);
		te_add_escaped(out, Rxml_node_get_attribute(node, name));
		te_add(out, "\"");
	}
	te_add(out, "/>\n");
}

/* A color or a metric by its name, in <Colors>, <Metrics> or on <Theme>. */
static int read_value(te_model* model, const char* name, const char* value, int colors, int metrics) {
	int index;
	float rgba[4], number;
	if (colors && (index = color_index(name)) >= 0) {
		if (!te_parse_hex(value, rgba)) return 0;
		model->colors[index].set = 1;
		memcpy(model->colors[index].c, rgba, sizeof(rgba));
		return 1;
	}
	if (metrics && (index = metric_index(name, 0)) >= 0) {
		if (!parse_number(value, &number)) return 0;
		model->metric_set[index] = 1;
		model->metrics[index] = number;
		return 1;
	}
	return 0;
}

int te_model_read(te_model* model, const char* xml, size_t size, char* error, size_t error_size) {
	RXmlError xml_error;
	Rid document, root;
	te_text styles;
	int i, count;

	memset(&xml_error, 0, sizeof(xml_error));
	document = Rxml_document_parse(xml, size, &xml_error);
	if (document == R_INVALID_ID) {
		snprintf(error, error_size, "line %d, column %d: %s", xml_error.line, xml_error.column, xml_error.message);
		return 0;
	}
	root = Rxml_document_get_root(document);
	if (root == R_INVALID_ID || te_stricmp(Rxml_node_get_tag(root), "Theme") != 0) {
		snprintf(error, error_size, "this is not a theme file: its root element is not <Theme>");
		Rxml_document_destroy(document);
		return 0;
	}

	te_model_free(model);
	te_model_init(model);
	model->look = 0;
	memset(&styles, 0, sizeof(styles));
	te_add(&styles, "");

	count = Rxml_node_get_attribute_count(root);
	for (i = 0; i < count; i++) {
		const char* name = Rxml_node_get_attribute_name(root, i);
		const char* value = Rxml_node_get_attribute(root, name);
		float rgba[4];
		int k;
		if (te_stricmp(name, "Name") == 0) {
			te_copy(model->name, sizeof(model->name), value);
		} else if (te_stricmp(name, "Look") == 0) {
			k = look_index(value);
			if (k > 0) model->look = k;
		} else if (te_stricmp(name, "Base") == 0) {
			/* a look by its old name ("Dark"), else a theme */
			k = look_index(value);
			if (k > 0 && model->look == 0) model->look = k;
			else if (k <= 0) te_copy(model->base, sizeof(model->base), value);
		} else if (te_stricmp(name, "Dark") == 0) {
			model->dark = (te_stricmp(value, "true") == 0 || strcmp(value, "1") == 0 || te_stricmp(value, "yes") == 0) ? 1 : 2;
		} else if (te_stricmp(name, "Accent") == 0) {
			if (te_parse_hex(value, rgba)) {
				model->accent_set = 1;
				memcpy(model->accent, rgba, sizeof(model->accent));
			}
		} else if (te_stricmp(name, "Material") == 0) {
			for (k = 1; k < 5; k++) {
				if (te_stricmp(g_te_materials[k], value) == 0) model->material = k;
			}
			if (te_stricmp(value, "Frosted") == 0) model->material = 3;
			if (te_stricmp(value, "Tinted") == 0) model->material = 4;
		} else if (te_stricmp(name, "Motion") == 0) {
			if (te_stricmp(value, "Springy") == 0 || te_stricmp(value, "Raven") == 0) model->motion = 1;
			else if (te_stricmp(value, "Smooth") == 0 || te_stricmp(value, "Native") == 0) model->motion = 2;
		} else if (te_stricmp(name, "Font") == 0) {
			te_copy(model->font, sizeof(model->font), value);
		} else if (te_stricmp(name, "Wallpaper") == 0) {
			if (te_stricmp(value, "None") == 0 || !value[0]) model->wallpaper = TE_WALLPAPER_NONE;
			else if (te_stricmp(value, "Dark") == 0) model->wallpaper = TE_WALLPAPER_DARK;
			else if (te_stricmp(value, "Light") == 0) model->wallpaper = TE_WALLPAPER_LIGHT;
			else {
				model->wallpaper = value[0] == '#' ? TE_WALLPAPER_COLORS : TE_WALLPAPER_IMAGE;
				te_copy(model->wallpaper_text, sizeof(model->wallpaper_text), value);
			}
		} else {
			/* colors and metrics may be given on <Theme> itself */
			read_value(model, name, value, 1, 1);
		}
	}

	count = Rxml_node_get_child_count(root);
	for (i = 0; i < count; i++) {
		Rid child = Rxml_node_get_child(root, i);
		const char* tag = Rxml_node_get_tag(child);
		int k, attributes;
		if (!tag || !tag[0]) continue;
		attributes = Rxml_node_get_attribute_count(child);
		if (te_stricmp(tag, "Colors") == 0 || te_stricmp(tag, "Metrics") == 0) {
			int colors = te_stricmp(tag, "Colors") == 0;
			for (k = 0; k < attributes; k++) {
				const char* name = Rxml_node_get_attribute_name(child, k);
				read_value(model, name, Rxml_node_get_attribute(child, name), colors, !colors);
			}
		} else if (te_stricmp(tag, "Glass") == 0) {
			for (k = 0; k < attributes; k++) {
				const char* name = Rxml_node_get_attribute_name(child, k);
				int index = metric_index(name, 1);
				float number;
				if (index >= 0 && parse_number(Rxml_node_get_attribute(child, name), &number)) {
					model->metric_set[index] = 1;
					model->metrics[index] = number;
				}
			}
		} else if (te_stricmp(tag, "Syntax") == 0) {
			for (k = 0; k < attributes; k++) {
				const char* name = Rxml_node_get_attribute_name(child, k);
				int s;
				for (s = 0; s < TE_SYNTAX_COUNT; s++) {
					if (te_stricmp(g_te_syntax[s].name, name) == 0 && te_parse_hex(Rxml_node_get_attribute(child, name), model->syntax[s].c)) {
						model->syntax[s].set = 1;
					}
				}
			}
		} else {
			keep_element(&styles, child);
		}
	}
	free(model->styles);
	model->styles = styles.data ? styles.data : te_strdup("");
	Rxml_document_destroy(document);
	return 1;
}

/* ---------------------------------------------------------------------------
 *  The palette worked out from the accent
 * ------------------------------------------------------------------------- */

static void set_color(te_model* model, RColorRole role, const float* rgb, float alpha) {
	int i;
	for (i = 0; i < TE_COLOR_COUNT; i++) {
		if (g_te_colors[i].role == role) {
			model->colors[i].set = 1;
			memcpy(model->colors[i].c, rgb, 3 * sizeof(float));
			model->colors[i].c[3] = alpha;
			return;
		}
	}
}

static void set_hsl(te_model* model, RColorRole role, float h, float s, float l, float alpha) {
	float rgb[3];
	te_hsl_to_rgb(h, s, l, rgb);
	set_color(model, role, rgb, alpha);
}

static te_color* color_of(te_model* model, RColorRole role) {
	int i;
	for (i = 0; i < TE_COLOR_COUNT; i++) {
		if (g_te_colors[i].role == role) return &model->colors[i];
	}
	return &model->colors[0];
}

/* Moves the lightness of `fg` away from `bg` until they reach `target` (the hue stays). */
static void ensure_contrast(te_model* model, RColorRole fg, RColorRole bg, float target, int dark) {
	te_color* f = color_of(model, fg);
	const te_color* b = color_of(model, bg);
	float h, s, l;
	int step;
	te_rgb_to_hsl(f->c, &h, &s, &l);
	for (step = 0; step < 60 && te_contrast(f->c, b->c) < target; step++) {
		l += dark ? 0.015f : -0.015f;
		if (l < 0.0f || l > 1.0f) break;
		te_hsl_to_rgb(h, s, l, f->c);
	}
}

/* Steps of lightness away from `bg` until `target`; -1 when that direction never reaches it. */
static int contrast_steps(const float* fg, const float* bg, float target, float direction, float* out) {
	float h, s, l;
	int step;
	memcpy(out, fg, 4 * sizeof(float));
	te_rgb_to_hsl(fg, &h, &s, &l);
	for (step = 0; step <= 100; step++) {
		if (te_contrast(out, bg) >= target) return step;
		l += direction * 0.01f;
		if (l < 0.0f || l > 1.0f) return -1;
		te_hsl_to_rgb(h, s, l, out);
	}
	return -1;
}

void te_fix_contrast(float* fg, const float* bg, float target) {
	float up[4], down[4], white[4] = { 1, 1, 1, 1 }, black[4] = { 0, 0, 0, 1 };
	int a = contrast_steps(fg, bg, target, 1.0f, up);
	int b = contrast_steps(fg, bg, target, -1.0f, down);
	if (a >= 0 && (b < 0 || a <= b)) memcpy(fg, up, 3 * sizeof(float));
	else if (b >= 0) memcpy(fg, down, 3 * sizeof(float));
	else memcpy(fg, te_contrast(white, bg) >= te_contrast(black, bg) ? white : black, 3 * sizeof(float));
}

void te_generate_palette(te_model* model, int dark, float tint, float contrast) {
	/* surfaces: [role, lightness steps from the window] */
	static const struct { RColorRole role; float dark_step, light_step; } surfaces[] = {
		{ R_COLOR_WINDOW, 0.0f, 0.0f }, { R_COLOR_PANEL, 1.0f, -1.0f }, { R_COLOR_INPUT, 0.4f, -1.4f },
		{ R_COLOR_CONTROL, 2.3f, 0.8f }, { R_COLOR_CONTROL_HOVERED, 3.1f, 1.6f }, { R_COLOR_CONTROL_PRESSED, 3.9f, 2.6f },
		{ R_COLOR_CONTROL_BORDER, 4.8f, 4.2f }, { R_COLOR_BORDER, 2.7f, 2.9f }, { R_COLOR_SEPARATOR, 2.1f, 2.2f },
		{ R_COLOR_HEADER, 1.6f, 0.7f }, { R_COLOR_HEADER_HOVERED, 2.6f, 1.6f }, { R_COLOR_TAB_ACTIVE, 2.2f, -1.0f },
		{ R_COLOR_ROW_HOVERED, 1.8f, 1.0f }, { R_COLOR_POPUP, 1.6f, -1.2f }, { R_COLOR_POPUP_BORDER, 4.1f, 3.6f },
		{ R_COLOR_TRACK, 2.8f, 2.8f }, { R_COLOR_HANDLE, 7.0f, 7.5f }
	};
	float h, s, l, ns, d, window, rgb[3], white[4] = { 1, 1, 1, 1 }, black[4] = { 0.07f, 0.07f, 0.09f, 1 }, accent[4];
	size_t i;

	if (!model->accent_set) {
		model->accent_set = 1;
		model->accent[0] = 0.45f;
		model->accent[1] = 0.36f;
		model->accent[2] = 1.0f;
	}
	if (tint < 0.0f) tint = 0.0f;
	if (tint > 1.0f) tint = 1.0f;
	if (contrast < 0.4f) contrast = 0.4f;
	if (contrast > 1.6f) contrast = 1.6f;
	te_rgb_to_hsl(model->accent, &h, &s, &l);
	/* neutrals: a little of the hue of the accent */
	ns = (0.03f + 0.32f * tint) * (s < 0.15f ? s / 0.15f : 1.0f);
	d = (dark ? 0.034f : 0.022f) * contrast;
	window = dark ? 0.075f : 0.955f;
	for (i = 0; i < sizeof(surfaces) / sizeof(surfaces[0]); i++) {
		float step = dark ? surfaces[i].dark_step : -surfaces[i].light_step;
		float lightness = window + step * d;
		if (!dark && surfaces[i].role == R_COLOR_HANDLE) lightness = window - 7.5f * d * 1.6f;
		set_hsl(model, surfaces[i].role, h, ns, lightness < 0.02f ? 0.02f : lightness > 1.0f ? 1.0f : lightness, 1.0f);
	}

	/* text */
	set_hsl(model, R_COLOR_TEXT, h, ns * 0.6f, dark ? 0.93f : 0.13f, 1.0f);
	set_hsl(model, R_COLOR_TEXT_DISABLED, h, ns * 0.5f, dark ? 0.50f : 0.58f, 1.0f);
	set_hsl(model, R_COLOR_TEXT_PLACEHOLDER, h, ns * 0.5f, dark ? 0.44f : 0.62f, 1.0f);
	set_hsl(model, R_COLOR_TOOLTIP, h, ns, dark ? window + 3.3f * d : 0.18f, 1.0f);
	set_hsl(model, R_COLOR_TOOLTIP_TEXT, h, ns * 0.4f, 0.96f, 1.0f);
	set_hsl(model, R_COLOR_SHADOW, 0.0f, 0.0f, 0.0f, dark ? 0.5f : 0.18f);

	/* the accent and its states */
	memcpy(accent, model->accent, sizeof(model->accent));
	accent[3] = 1.0f;
	set_color(model, R_COLOR_ACCENT, accent, 1.0f);
	set_color(model, R_COLOR_FOCUS, accent, 1.0f);
	set_color(model, R_COLOR_SWITCH, accent, 1.0f);
	set_hsl(model, R_COLOR_ACCENT_HOVERED, h, s, l + (dark ? 0.07f : -0.06f), 1.0f);
	set_hsl(model, R_COLOR_ACCENT_PRESSED, h, s, l + (dark ? -0.07f : -0.13f), 1.0f);
	set_color(model, R_COLOR_SELECTION, accent, dark ? 0.32f : 0.22f);
	set_hsl(model, R_COLOR_HANDLE_HOVERED, h, s, dark ? fmaxf(l, 0.6f) : fminf(l, 0.5f), 1.0f);
	/* the text on the accent: whichever reads better */
	set_color(model, R_COLOR_ACCENT_TEXT, te_contrast(white, accent) >= te_contrast(black, accent) ? white : black, 1.0f);
	set_hsl(model, R_COLOR_LINK, h, fmaxf(s, 0.55f), dark ? fmaxf(l, 0.70f) : fminf(l, 0.42f), 1.0f);
	set_hsl(model, R_COLOR_CODE, h + 28.0f, 0.55f, dark ? 0.80f : 0.36f, 1.0f);

	/* status, in the same lightness */
	set_hsl(model, R_COLOR_SUCCESS, 142.0f, 0.55f, dark ? 0.62f : 0.36f, 1.0f);
	set_hsl(model, R_COLOR_WARNING, 38.0f, 0.92f, dark ? 0.62f : 0.42f, 1.0f);
	set_hsl(model, R_COLOR_DANGER, 2.0f, 0.78f, dark ? 0.64f : 0.48f, 1.0f);

	/* what people read stays readable (WCAG 2: 4.5:1 for text, 3:1 for the parts of controls) */
	ensure_contrast(model, R_COLOR_TEXT_PLACEHOLDER, R_COLOR_INPUT, 3.0f, dark);
	ensure_contrast(model, R_COLOR_CONTROL_BORDER, R_COLOR_WINDOW, 3.0f, dark);
	/* (the accent stays the one chosen, even pale: the focus ring does not) */
	ensure_contrast(model, R_COLOR_FOCUS, R_COLOR_WINDOW, 3.0f, dark);
	ensure_contrast(model, R_COLOR_LINK, R_COLOR_WINDOW, 4.5f, dark);
	ensure_contrast(model, R_COLOR_DANGER, R_COLOR_PANEL, 3.0f, dark);
	ensure_contrast(model, R_COLOR_SUCCESS, R_COLOR_PANEL, 3.0f, dark);
	ensure_contrast(model, R_COLOR_WARNING, R_COLOR_PANEL, 3.0f, dark);

	/* the syntax colors: around the hue of the accent */
	{
		static const struct { float hue, sat, dl, ll; } shifts[TE_SYNTAX_COUNT] = {
			{ 0, 0.10f, 0.58f, 0.48f },    /* comment: a quiet neutral */
			{ 100, 0.55f, 0.70f, 0.36f },  /* string */
			{ 160, 0.60f, 0.72f, 0.38f },  /* number */
			{ 0, 0.80f, 0.74f, 0.45f },    /* keyword: the accent */
			{ -60, 0.65f, 0.74f, 0.38f },  /* type */
			{ 40, 0.70f, 0.76f, 0.38f },   /* function */
			{ 0, 0.80f, 0.74f, 0.42f },    /* tag */
			{ -60, 0.60f, 0.76f, 0.40f },  /* attribute */
			{ 0, 0.08f, 0.66f, 0.42f },    /* punctuation */
			{ 140, 0.65f, 0.70f, 0.40f },  /* directive */
			{ 0, 0.80f, 0.74f, 0.40f },    /* heading */
			{ 0, 0.10f, 0.95f, 0.10f },    /* strong */
			{ 0, 0.30f, 0.86f, 0.25f },    /* emphasis */
			{ 0, 0.75f, 0.72f, 0.42f }     /* link */
		};
		for (i = 0; i < TE_SYNTAX_COUNT; i++) {
			te_hsl_to_rgb(h + shifts[i].hue, shifts[i].sat, dark ? shifts[i].dl : shifts[i].ll, rgb);
			model->syntax[i].set = 1;
			memcpy(model->syntax[i].c, rgb, sizeof(rgb));
			model->syntax[i].c[3] = 1.0f;
		}
	}
}
