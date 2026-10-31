/**
 * Raven - widget/theme.c
 * Named themes, styles (values for a type of widget and / or a class), themes
 * given to a part of the widget tree, and theme files.
 *
 * The theme in use is g_rvn.theme. Named themes live in g_rvn.themes, at
 * addresses that never change: a widget given a theme of its own points to
 * one of them, and so does everything inside it.
 */

#include "widget.h"
#include "../widgets/common.h"
#include "../xml/xml.h"
#include "../xml/xml_watch.h"
#include "../render/renderer.h"

#define RVN_COLOR_INDEX(role) ((int)(role) - (int)R_COLOR_WINDOW)
#define RVN_METRIC_INDEX(metric) ((int)(metric) - (int)R_METRIC_FONT_SIZE)

static const struct { const char* name; RTheme theme; } g_builtin_themes[] = {
	{ "Dark", R_THEME_DARK }, { "Light", R_THEME_LIGHT },
	{ "GlassDark", R_THEME_GLASS_DARK }, { "GlassLight", R_THEME_GLASS_LIGHT },
	{ "HighContrastDark", R_THEME_HIGH_CONTRAST_DARK }, { "HighContrastLight", R_THEME_HIGH_CONTRAST_LIGHT }
};

/** See rvn_theme_dark_twin. */
static rvn_theme_t g_dark_twin;

/* ---------------------------------------------------------------------------
 *  Life of a theme
 * ------------------------------------------------------------------------- */

static void rvn_theme_free_matches(rvn_theme_t* theme) {
	uint32_t i;
	for (i = 0; i < theme->match_count; i++) {
		rvn_free(theme->matches[i]->classes);
		rvn_free(theme->matches[i]->part_rules);
		rvn_free(theme->matches[i]);
	}
	rvn_free(theme->matches);
	theme->matches = NULL;
	theme->match_count = 0;
	theme->match_cap = 0;
}

static void rvn_theme_free_rules(rvn_theme_t* theme) {
	uint32_t i;
	for (i = 0; i < theme->rule_count; i++) rvn_free(theme->rules[i].cls);
	rvn_free(theme->rules);
	theme->rules = NULL;
	theme->rule_count = 0;
	theme->rule_cap = 0;
}

void rvn_theme_clear(rvn_theme_t* theme) {
	rvn_theme_free_matches(theme);
	rvn_theme_free_rules(theme);
	rvn_free(theme->name);
	rvn_free(theme->path);
	rvn_free(theme->source);
	memset(theme, 0, sizeof(*theme));
}

void rvn_theme_copy(rvn_theme_t* destination, const rvn_theme_t* source) {
	char* name = destination->name;
	char* path = destination->path;
	uint32_t i;
	if (destination == source) return;
	rvn_theme_free_matches(destination);
	rvn_theme_free_rules(destination);
	rvn_free(destination->source);
	*destination = *source;
	destination->source = NULL;
	if (source->source) {
		destination->source = (char*)rvn_alloc(source->source_size + 1);
		memcpy(destination->source, source->source, source->source_size);
		destination->source[source->source_size] = '\0';
	}
	destination->name = name;
	destination->path = path;
	destination->matches = NULL;
	destination->match_count = 0;
	destination->match_cap = 0;
	destination->rules = NULL;
	destination->rule_cap = 0;
	if (source->rule_count) {
		destination->rules = (rvn_style_rule_t*)rvn_alloc(sizeof(rvn_style_rule_t) * source->rule_count);
		destination->rule_cap = source->rule_count;
		for (i = 0; i < source->rule_count; i++) {
			destination->rules[i] = source->rules[i];
			destination->rules[i].cls = rvn_strdup(source->rules[i].cls);
		}
	}
}

void rvn_themes_init(void) {
	rvn_mutex_init(&g_rvn.style_mutex);
	g_rvn.style_generation = 1;
	memset(&g_dark_twin, 0, sizeof(g_dark_twin));
}

void rvn_themes_shutdown(void) {
	uint32_t i;
	for (i = 0; i < g_rvn.theme_count; i++) {
		rvn_theme_clear(g_rvn.themes[i]);
		rvn_free(g_rvn.themes[i]);
	}
	rvn_free(g_rvn.themes);
	g_rvn.themes = NULL;
	g_rvn.theme_count = 0;
	g_rvn.theme_cap = 0;
	rvn_theme_clear(&g_rvn.theme);
	rvn_theme_clear(&g_dark_twin);
	rvn_free(g_rvn.theme_name);
	g_rvn.theme_name = NULL;
	rvn_mutex_destroy(&g_rvn.style_mutex);
}

static rvn_theme_t* rvn_theme_add(const char* name) {
	rvn_theme_t* theme = (rvn_theme_t*)rvn_calloc(sizeof(rvn_theme_t));
	theme->name = rvn_strdup(name);
	RVN_ARRAY_RESERVE(g_rvn.themes, g_rvn.theme_cap, g_rvn.theme_count + 1);
	g_rvn.themes[g_rvn.theme_count++] = theme;
	return theme;
}

rvn_theme_t* rvn_theme_find(const char* name) {
	uint32_t i;
	size_t k;
	if (!name || !name[0]) return NULL;
	for (i = 0; i < g_rvn.theme_count; i++) {
		if (rvn_stricmp(g_rvn.themes[i]->name, name) == 0) return g_rvn.themes[i];
	}
	/* the built-in ones are made when first asked for */
	for (k = 0; k < RVN_COUNT_OF(g_builtin_themes); k++) {
		if (rvn_stricmp(g_builtin_themes[k].name, name) == 0) {
			rvn_theme_t* theme = rvn_theme_add(g_builtin_themes[k].name);
			rvn_theme_build(theme, g_builtin_themes[k].theme);
			return theme;
		}
	}
	/* and so are the looks, by their names ("WindowsDark"...) */
	{
		RLook look;
		if (rvn_look_by_name(name, &look)) {
			rvn_theme_t* theme = rvn_theme_add(name);
			rvn_theme_build_look(theme, look);
			return theme;
		}
	}
	return NULL;
}

static void rvn_theme_builtins(void) {
	size_t k;
	for (k = 0; k < RVN_COUNT_OF(g_builtin_themes); k++) rvn_theme_find(g_builtin_themes[k].name);
}

/* ---------------------------------------------------------------------------
 *  Changes
 * ------------------------------------------------------------------------- */

void rvn_styles_changed(void) {
	uint32_t i;
	rvn_mutex_lock(&g_rvn.style_mutex);
	rvn_theme_free_matches(&g_rvn.theme);
	rvn_theme_free_matches(&g_dark_twin);
	for (i = 0; i < g_rvn.theme_count; i++) rvn_theme_free_matches(g_rvn.themes[i]);
	/* (0 is the generation of a widget that never looked) */
	if (++g_rvn.style_generation == 0) g_rvn.style_generation = 1;
	rvn_mutex_unlock(&g_rvn.style_mutex);
}

/*
 * What must stay dark whatever the theme (the island) uses the dark
 * counterpart of the theme in use: its own colors when it is dark, else the
 * built-in dark palette with its accent, its sizes, its material and its
 * styles.
 */
static void rvn_theme_refresh_twin(void) {
	static const RColorRole kept[] = {
		R_COLOR_ACCENT, R_COLOR_ACCENT_HOVERED, R_COLOR_ACCENT_PRESSED, R_COLOR_ACCENT_TEXT, R_COLOR_FOCUS
	};
	const rvn_theme_t* theme = &g_rvn.theme;
	size_t i;
	if (theme->dark) {
		rvn_theme_copy(&g_dark_twin, theme);
		return;
	}
	/* (the dark look of the same family; a glass look stays glass) */
	rvn_theme_build_look(&g_dark_twin, theme->family == RVN_FAMILY_RAVEN ? (theme->glass ? R_LOOK_RAVEN_GLASS_DARK : R_LOOK_RAVEN_DARK) :
		rvn_look_dark_counterpart(theme->look));
	for (i = 0; i < RVN_COUNT_OF(kept); i++) g_dark_twin.colors[RVN_COLOR_INDEX(kept[i])] = theme->colors[RVN_COLOR_INDEX(kept[i])];
	memcpy(g_dark_twin.metrics, theme->metrics, sizeof(g_dark_twin.metrics));
	g_dark_twin.material = theme->material;
	g_dark_twin.glass = theme->glass;
	g_dark_twin.font = theme->font;
	g_dark_twin.wallpaper = R_INVALID_ID;
}

const rvn_theme_t* rvn_theme_dark_twin(void) {
	return &g_dark_twin;
}

void rvn_theme_changed(void) {
	rvn_theme_refresh_twin();
	rvn_styles_changed();
	rvn_windows_theme_changed();
	rvn_invalidate_all_windows();
}

/*
 * The theme in use was just replaced: when it names no font, it keeps the
 * one that was in use, unless that one came with the look (Segoe UI) and the
 * new theme is not of a platform: the font of Raven comes back.
 */
static void rvn_theme_settle_font(Rid previous, int previous_from_look) {
	if (g_rvn.theme.font != R_INVALID_ID && rvn_pool_get(&g_rvn.fonts, g_rvn.theme.font)) return;
	g_rvn.theme.look_font = R_FALSE;
	g_rvn.theme.font = previous_from_look ? g_rvn.default_font : previous;
	if (g_rvn.theme.font == R_INVALID_ID || !rvn_pool_get(&g_rvn.fonts, g_rvn.theme.font)) g_rvn.theme.font = g_rvn.default_font;
}

static float rvn_color_luminance(rvn_color_t color) {
	float r, g, b, a;
	rvn_color_to_floats(color, &r, &g, &b, &a);
	return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

void rvn_theme_set_accent(rvn_theme_t* theme, rvn_color_t accent) {
	rvn_color_t* colors = theme->colors;
	rvn_color_t opaque = (accent & 0x00FFFFFFu) | 0xFF000000u;
	rvn_color_t white = rvn_hex(0xFFFFFF, 255), black = rvn_hex(0x000000, 255);
	uint32_t selection_alpha = rvn_color_a(colors[RVN_COLOR_INDEX(R_COLOR_SELECTION)]);
	uint32_t focus_alpha = rvn_color_a(colors[RVN_COLOR_INDEX(R_COLOR_FOCUS)]);
	int switch_follows = colors[RVN_COLOR_INDEX(R_COLOR_SWITCH)] == colors[RVN_COLOR_INDEX(R_COLOR_ACCENT)];

	if (theme->family == RVN_FAMILY_MACOS) {
		/*
		 * macOS: no change under the pointer, darker (lighter in the dark)
		 * when pressed; the focus ring and the selection of text are the
		 * accent, faded; links are the accent.
		 */
		colors[RVN_COLOR_INDEX(R_COLOR_ACCENT)] = opaque;
		colors[RVN_COLOR_INDEX(R_COLOR_ACCENT_HOVERED)] = opaque;
		colors[RVN_COLOR_INDEX(R_COLOR_ACCENT_PRESSED)] = theme->dark ? rvn_color_mix(opaque, white, 0.2f) : rvn_color_mix(opaque, black, 0.2f);
		colors[RVN_COLOR_INDEX(R_COLOR_ACCENT_TEXT)] = rvn_color_luminance(opaque) > 0.62f ? rvn_hex(0x000000, 217) : white;
		colors[RVN_COLOR_INDEX(R_COLOR_FOCUS)] = rvn_color_fade(opaque, 0.5f);
		colors[RVN_COLOR_INDEX(R_COLOR_SELECTION)] = theme->dark ? rvn_color_mix(opaque, rvn_hex(0x1E1E1E, 255), 0.55f) : rvn_color_mix(opaque, white, 0.7f);
		colors[RVN_COLOR_INDEX(R_COLOR_LINK)] = theme->dark ? rvn_color_mix(opaque, white, 0.2f) : rvn_color_mix(opaque, black, 0.12f);
		if (switch_follows) colors[RVN_COLOR_INDEX(R_COLOR_SWITCH)] = opaque;
		return;
	}
	if (theme->family == RVN_FAMILY_GNOME) {
		/*
		 * GNOME: the accent lightened under the pointer, darkened pressed;
		 * the focus ring and the selection, the accent seen through; the
		 * text in the accent (links) a little darker, or lighter in the dark.
		 */
		colors[RVN_COLOR_INDEX(R_COLOR_ACCENT)] = opaque;
		colors[RVN_COLOR_INDEX(R_COLOR_ACCENT_HOVERED)] = rvn_color_mix(opaque, white, 0.1f);
		colors[RVN_COLOR_INDEX(R_COLOR_ACCENT_PRESSED)] = rvn_color_mix(opaque, black, 0.2f);
		colors[RVN_COLOR_INDEX(R_COLOR_ACCENT_TEXT)] = rvn_color_luminance(opaque) > 0.62f ? rvn_hex(0x000006, 204) : white;
		colors[RVN_COLOR_INDEX(R_COLOR_LINK)] = theme->dark ? rvn_color_mix(opaque, white, 0.35f) : rvn_color_mix(opaque, black, 0.15f);
		colors[RVN_COLOR_INDEX(R_COLOR_FOCUS)] = rvn_color_fade(colors[RVN_COLOR_INDEX(R_COLOR_LINK)], 0.5f);
		colors[RVN_COLOR_INDEX(R_COLOR_SELECTION)] = rvn_color_fade(opaque, 0.25f);
		if (switch_follows) colors[RVN_COLOR_INDEX(R_COLOR_SWITCH)] = opaque;
		return;
	}
	if (theme->family == RVN_FAMILY_WINDOWS) {
		/*
		 * Windows: the shades are the accent seen through (90 %, 80 %), the
		 * focus stays black or white, the selection of text is the accent
		 * faded, the links are the accent pushed toward the text.
		 */
		colors[RVN_COLOR_INDEX(R_COLOR_ACCENT)] = opaque;
		colors[RVN_COLOR_INDEX(R_COLOR_ACCENT_HOVERED)] = rvn_color_fade(opaque, 0.9f);
		colors[RVN_COLOR_INDEX(R_COLOR_ACCENT_PRESSED)] = rvn_color_fade(opaque, 0.8f);
		colors[RVN_COLOR_INDEX(R_COLOR_ACCENT_TEXT)] = rvn_color_luminance(opaque) > 0.45f ? black : white;
		colors[RVN_COLOR_INDEX(R_COLOR_SELECTION)] = rvn_color_fade(opaque, theme->dark ? 0.32f : 0.36f);
		colors[RVN_COLOR_INDEX(R_COLOR_LINK)] = theme->dark ? rvn_color_mix(opaque, white, 0.35f) : rvn_color_mix(opaque, black, 0.3f);
		if (switch_follows) colors[RVN_COLOR_INDEX(R_COLOR_SWITCH)] = opaque;
		return;
	}

	colors[RVN_COLOR_INDEX(R_COLOR_ACCENT)] = opaque;
	colors[RVN_COLOR_INDEX(R_COLOR_ACCENT_HOVERED)] = rvn_color_mix(opaque, white, 0.18f);
	colors[RVN_COLOR_INDEX(R_COLOR_ACCENT_PRESSED)] = rvn_color_mix(opaque, black, 0.18f);
	/* what is written on it must stay readable */
	colors[RVN_COLOR_INDEX(R_COLOR_ACCENT_TEXT)] = rvn_color_luminance(opaque) > 0.62f ? rvn_hex(0x101014, 255) : white;
	colors[RVN_COLOR_INDEX(R_COLOR_FOCUS)] = rvn_color_fade(opaque, (float)(focus_alpha ? focus_alpha : 255u) / 255.0f);
	if (selection_alpha < 255u) {
		colors[RVN_COLOR_INDEX(R_COLOR_SELECTION)] = rvn_color_fade(opaque, (float)selection_alpha / 255.0f);
	} else {
		colors[RVN_COLOR_INDEX(R_COLOR_SELECTION)] = rvn_color_mix(colors[RVN_COLOR_INDEX(R_COLOR_WINDOW)], opaque, theme->dark ? 0.45f : 0.3f);
	}
	colors[RVN_COLOR_INDEX(R_COLOR_LINK)] = theme->dark ? rvn_color_mix(opaque, white, 0.3f) : rvn_color_mix(opaque, black, 0.15f);
	if (switch_follows) colors[RVN_COLOR_INDEX(R_COLOR_SWITCH)] = opaque;
}

/* ---------------------------------------------------------------------------
 *  Styles
 * ------------------------------------------------------------------------- */

static int rvn_classes_have(const char* classes, const char* cls) {
	size_t length;
	if (!classes || !cls || !cls[0]) return R_FALSE;
	length = strlen(cls);
	while (*classes) {
		const char* start;
		while (*classes == ' ' || *classes == '\t' || *classes == ',') classes++;
		start = classes;
		while (*classes && *classes != ' ' && *classes != '\t' && *classes != ',') classes++;
		if ((size_t)(classes - start) == length && memcmp(start, cls, length) == 0) return R_TRUE;
	}
	return R_FALSE;
}

int rvn_widget_has_class(const rvn_widget_t* widget, const char* cls) {
	return rvn_classes_have(widget->classes, cls);
}

static void rvn_style_overlay(rvn_style_match_t* match, const rvn_style_rule_t* rule) {
	int i;
	for (i = 0; i < R_COLOR_COUNT; i++) {
		if (rule->style.color_mask & ((uint64_t)1 << i)) match->style.colors[i] = rule->style.colors[i];
	}
	for (i = 0; i < R_METRIC_COUNT; i++) {
		if (rule->style.metric_mask & (1u << i)) match->style.metrics[i] = rule->style.metrics[i];
	}
	match->style.color_mask |= rule->style.color_mask;
	match->style.metric_mask |= rule->style.metric_mask;
	if (rule->material) match->material = rule->material;
}

static int rvn_rule_weight(const rvn_style_rule_t* rule) {
	int weight = (rule->type ? 1 : 0) + (rule->cls ? 2 : 0);
	uint32_t states = rule->states;
	while (states) {
		weight += 2;
		states &= states - 1;
	}
	return weight;
}

/* What the styles of a theme give to a type of widget with these classes. Called with the style mutex held. */
static const rvn_style_match_t* rvn_theme_match(rvn_theme_t* theme, int type, const char* classes) {
	rvn_style_match_t* match;
	uint32_t i;
	int pass;

	if (theme->rule_count == 0) return NULL;
	if (classes && !classes[0]) classes = NULL;
	for (i = 0; i < theme->match_count; i++) {
		match = theme->matches[i];
		if (match->type != type) continue;
		if ((!classes && !match->classes) || (classes && match->classes && strcmp(classes, match->classes) == 0)) {
			return (match->style.color_mask || match->style.metric_mask || match->material || match->part_rule_count) ? match : NULL;
		}
	}
	match = (rvn_style_match_t*)rvn_calloc(sizeof(rvn_style_match_t));
	match->type = type;
	match->classes = rvn_strdup(classes);
	/* from the least to the most specific: everything, a type, a class, a type and a class */
	for (pass = 0; pass < 4; pass++) {
		for (i = 0; i < theme->rule_count; i++) {
			const rvn_style_rule_t* rule = &theme->rules[i];
			int specificity = (rule->type ? 1 : 0) + (rule->cls ? 2 : 0);
			if (specificity != pass) continue;
			if (rule->type && rule->type != type) continue;
			if (rule->cls && !rvn_classes_have(classes, rule->cls)) continue;
			/* (a rule for a state is only about parts) */
			if (!rule->states) rvn_style_overlay(match, rule);
		}
	}
	/*
	 * The rules for parts and states, kept in the order they apply: a type
	 * counts 1, a class or a state 2 each (as in CSS: a state is as specific
	 * as a class); between equals, the last one written wins.
	 */
	for (i = 0; i < theme->rule_count; i++) {
		const rvn_style_rule_t* rule = &theme->rules[i];
		if (!rule->look_mask) continue;
		if (rule->type && rule->type != type) continue;
		if (rule->cls && !rvn_classes_have(classes, rule->cls)) continue;
		match->part_rules = (uint32_t*)rvn_realloc(match->part_rules, sizeof(uint32_t) * (match->part_rule_count + 1));
		match->part_rules[match->part_rule_count++] = i;
	}
	{
		uint32_t a, b;
		for (a = 1; a < match->part_rule_count; a++) {
			uint32_t index = match->part_rules[a];
			int weight = rvn_rule_weight(&theme->rules[index]);
			for (b = a; b > 0 && rvn_rule_weight(&theme->rules[match->part_rules[b - 1]]) > weight; b--) {
				match->part_rules[b] = match->part_rules[b - 1];
			}
			match->part_rules[b] = index;
		}
	}
	RVN_ARRAY_RESERVE(theme->matches, theme->match_cap, theme->match_count + 1);
	theme->matches[theme->match_count++] = match;
	return (match->style.color_mask || match->style.metric_mask || match->material || match->part_rule_count) ? match : NULL;
}

void rvn_widget_restyle(rvn_widget_t* widget) {
	/* (widgets are painted on several threads at once: the caches of the themes are shared) */
	rvn_mutex_lock(&g_rvn.style_mutex);
	widget->match = rvn_theme_match((rvn_theme_t*)(uintptr_t)rvn_widget_theme(widget), (int)widget->cls->type, widget->classes);
	widget->style_generation = g_rvn.style_generation;
	rvn_mutex_unlock(&g_rvn.style_mutex);
}

/* The rule of a theme for a type, a class, a part and states, created when missing. `widget` and `cls` may be NULL. */
static rvn_style_rule_t* rvn_theme_part_rule(rvn_theme_t* theme, const char* widget, const char* cls, int part, uint32_t states, const char* function) {
	int type = 0;
	uint32_t i;
	rvn_style_rule_t* rule;
	if (widget && !widget[0]) widget = NULL;
	if (cls && !cls[0]) cls = NULL;
	if (widget) {
		const rvn_widget_class_t* widget_class = rvn_widget_class_by_name(widget);
		if (!widget_class) {
			rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: no widget is called '%s'", function, widget);
			return NULL;
		}
		type = (int)widget_class->type;
	}
	for (i = 0; i < theme->rule_count; i++) {
		rule = &theme->rules[i];
		if (rule->type != type || rule->part != part || rule->states != states) continue;
		if ((!cls && !rule->cls) || (cls && rule->cls && strcmp(cls, rule->cls) == 0)) return rule;
	}
	RVN_ARRAY_RESERVE(theme->rules, theme->rule_cap, theme->rule_count + 1);
	rule = &theme->rules[theme->rule_count++];
	memset(rule, 0, sizeof(*rule));
	rule->type = type;
	rule->cls = rvn_strdup(cls);
	rule->part = part;
	rule->states = states;
	return rule;
}

static rvn_style_rule_t* rvn_theme_rule(rvn_theme_t* theme, const char* widget, const char* cls, const char* function) {
	return rvn_theme_part_rule(theme, widget, cls, RVN_PART_BODY, 0, function);
}

/* ---------------------------------------------------------------------------
 *  Themes of parts of the tree
 * ------------------------------------------------------------------------- */

static void rvn_theme_spread(rvn_widget_t* widget, const rvn_theme_t* theme) {
	uint32_t i;
	widget->theme = theme;
	widget->style_generation = 0;
	widget->flags |= RVN_WF_LAYOUT_DIRTY;
	for (i = 0; i < widget->child_count; i++) {
		if (!widget->children[i]->theme_own) rvn_theme_spread(widget->children[i], theme);
	}
}

void rvn_widget_set_theme(rvn_widget_t* widget, const rvn_theme_t* theme, int own) {
	widget->theme_own = own ? 1 : 0;
	if (!own) theme = widget->parent ? widget->parent->theme : NULL;
	rvn_theme_spread(widget, theme);
	rvn_widget_invalidate_layout(widget);
}

void rvn_widget_inherit_theme(rvn_widget_t* widget, const rvn_theme_t* theme) {
	if (widget->theme_own || widget->theme == theme) return;
	rvn_theme_spread(widget, theme);
}

/* ---------------------------------------------------------------------------
 *  Theme files
 * ------------------------------------------------------------------------- */

static const struct { const char* name; rvn_token_t token; } g_token_names[] = {
	{ "Comment", RVN_TOKEN_COMMENT }, { "String", RVN_TOKEN_STRING }, { "Number", RVN_TOKEN_NUMBER },
	{ "Keyword", RVN_TOKEN_KEYWORD }, { "Tag", RVN_TOKEN_TAG }, { "Attribute", RVN_TOKEN_ATTRIBUTE },
	{ "Punctuation", RVN_TOKEN_PUNCTUATION }, { "Directive", RVN_TOKEN_PREPROCESSOR }, { "Type", RVN_TOKEN_TYPE },
	{ "Function", RVN_TOKEN_FUNCTION }, { "Heading", RVN_TOKEN_HEADING }, { "Strong", RVN_TOKEN_STRONG },
	{ "Emphasis", RVN_TOKEN_EMPHASIS }, { "Link", RVN_TOKEN_LINK }
};

static int rvn_token_by_name(const char* name, rvn_token_t* token) {
	size_t i;
	if (rvn_stricmp(name, "Preprocessor") == 0) name = "Directive";
	for (i = 0; i < RVN_COUNT_OF(g_token_names); i++) {
		if (rvn_stricmp(g_token_names[i].name, name) == 0) {
			*token = g_token_names[i].token;
			return R_TRUE;
		}
	}
	return R_FALSE;
}

typedef struct rvn_theme_reader_t {
	rvn_theme_t* theme;
	const char* source;
	int problems;
} rvn_theme_reader_t;

/* R_TRUE while theme files are read again for a new look: what was wrong in them was said when they were loaded. */
static int g_theme_quiet;

static void rvn_theme_problem(rvn_theme_reader_t* reader, const rvn_xml_node_t* node, const char* format, const char* a, const char* b) {
	char message[200];
	if (g_theme_quiet) {
		reader->problems++;
		return;
	}
	snprintf(message, sizeof(message), format, a ? a : "", b ? b : "");
	rvn_error(R_ERROR_XML, R_SEVERITY_WARNING, "theme %s:%d: %s", reader->source, node->line, message);
	reader->problems++;
}

/* "#101020, #3040C0, #C02080": a base color and up to five blobs. */
static Rid rvn_theme_wallpaper_from_colors(const char* text) {
	uint32_t colors[6];
	int count = 0;
	while (*text && count < 6) {
		char word[16];
		size_t length = 0;
		rvn_color_t color;
		float r, g, b, a;
		while (*text == ' ' || *text == ',') text++;
		while (text[length] && text[length] != ',' && text[length] != ' ' && length + 1 < sizeof(word)) length++;
		if (length == 0) break;
		memcpy(word, text, length);
		word[length] = '\0';
		text += length;
		if (!rvn_parse_color(word, &color)) return R_INVALID_ID;
		rvn_color_to_floats(color, &r, &g, &b, &a);
		colors[count++] = ((uint32_t)(r * 255.0f + 0.5f) << 16) | ((uint32_t)(g * 255.0f + 0.5f) << 8) | (uint32_t)(b * 255.0f + 0.5f);
	}
	if (count == 0) return R_INVALID_ID;
	return rvn_wallpaper_create_colors(colors[0], colors + 1, count - 1);
}

/* A color or a metric named by an attribute, into a style. R_FALSE when the name is neither. */
static int rvn_theme_read_value(rvn_theme_reader_t* reader, const rvn_xml_node_t* node, rvn_style_t* style,
	const char* name, const char* value, int colors, int metrics) {
	RColorRole role;
	RMetric metric;
	if (colors && rvn_color_role_by_name(name, &role)) {
		rvn_color_t color;
		if (!rvn_parse_color(value, &color)) {
			rvn_theme_problem(reader, node, "'%s' is not a color (%s)", value, name);
			return R_TRUE;
		}
		style->colors[RVN_COLOR_INDEX(role)] = color;
		style->color_mask |= (uint64_t)1 << RVN_COLOR_INDEX(role);
		return R_TRUE;
	}
	if (metrics && rvn_metric_by_name(name, &metric)) {
		float number;
		if (!rvn_parse_float(value, &number)) {
			rvn_theme_problem(reader, node, "'%s' is not a number (%s)", value, name);
			return R_TRUE;
		}
		style->metrics[RVN_METRIC_INDEX(metric)] = number;
		style->metric_mask |= 1u << RVN_METRIC_INDEX(metric);
		return R_TRUE;
	}
	return R_FALSE;
}

/* The values of a style written straight into the theme. */
static void rvn_theme_take(rvn_theme_t* theme, const rvn_style_t* style) {
	int i;
	for (i = 0; i < R_COLOR_COUNT; i++) {
		if (style->color_mask & ((uint64_t)1 << i)) theme->colors[i] = style->colors[i];
	}
	for (i = 0; i < R_METRIC_COUNT; i++) {
		if (style->metric_mask & (1u << i)) theme->metrics[i] = style->metrics[i];
	}
}

/* A property of a part ("Background", "Foreground"...) into a rule. R_FALSE when the name is none. */
static int rvn_theme_read_part_value(rvn_theme_reader_t* reader, const rvn_xml_node_t* node, rvn_style_rule_t* rule,
	const char* name, const char* value) {
	uint32_t bit = 0;
	int bad = R_FALSE;
	/* (the text of a part is its foreground) */
	if (rvn_stricmp(name, "Text") == 0) name = "Foreground";
	if (!rvn_part_property(name, value, &rule->look, &bit, &bad)) return R_FALSE;
	if (bad) rvn_theme_problem(reader, node, "'%s' is not a valid value for %s", value, name);
	else rule->look_mask |= bit;
	return R_TRUE;
}

/*
 * <Style Widget="Slider" Class="big" Part="Thumb" State="Hovered" Background="#fff"/>
 *
 * Without Part nor State, a style gives colors and metrics to the widgets it
 * names (Accent, CornerRadius...), and properties to the widget itself
 * (Background, Foreground, BorderColor...). With a Part or a State, it only
 * gives properties to that part in those states.
 */
static void rvn_theme_read_style(rvn_theme_reader_t* reader, const rvn_xml_node_t* node) {
	const char* widget = rvn_xml_attribute(node, "Widget");
	const char* cls = rvn_xml_attribute(node, "Class");
	const char* part_name = rvn_xml_attribute(node, "Part");
	const char* state_text = rvn_xml_attribute(node, "State");
	const rvn_xml_attribute_t* attribute;
	rvn_style_rule_t* rule;
	int part = RVN_PART_BODY, about_part;
	uint32_t states = 0;
	char bad[40];

	if (widget && widget[0] && !rvn_widget_class_by_name(widget)) {
		rvn_theme_problem(reader, node, "<Style Widget=\"%s\">: no widget has that name%s", widget, "");
		return;
	}
	if (part_name && part_name[0] && !rvn_part_by_name(part_name, &part)) {
		rvn_theme_problem(reader, node, "<Style Part=\"%s\">: no part has that name%s", part_name, "");
		return;
	}
	if (state_text && !rvn_states_parse(state_text, &states, bad, sizeof(bad))) {
		rvn_theme_problem(reader, node, "<Style State=\"%s\">: '%s' is not a state", state_text, bad);
		return;
	}
	about_part = part != RVN_PART_BODY || states != 0;
	rule = rvn_theme_part_rule(reader->theme, widget, cls, part, states, "theme");
	if (!rule) return;
	for (attribute = node->attributes; attribute; attribute = attribute->next) {
		const char* name = attribute->name;
		if (rvn_stricmp(name, "Widget") == 0 || rvn_stricmp(name, "Class") == 0 ||
			rvn_stricmp(name, "Part") == 0 || rvn_stricmp(name, "State") == 0) continue;
		if (about_part) {
			if (!rvn_theme_read_part_value(reader, node, rule, name, attribute->value)) {
				rvn_theme_problem(reader, node, "<Style Part / State>: '%s' is not a property of a part "
					"(Background, Foreground, BorderColor, BorderTopColor, BorderBottomColor, BorderWidth, BorderBottomWidth, "
					"CornerRadius, Size, Opacity, ShadowColor, ShadowSize)%s", name, "");
			}
			continue;
		}
		if (rvn_stricmp(name, "Material") == 0) {
			RMaterial material;
			if (!rvn_parse_material(attribute->value, &material)) {
				rvn_theme_problem(reader, node, "'%s' is not a material%s", attribute->value, "");
			} else {
				rule->material = material == R_MATERIAL_DEFAULT ? (RMaterial)0 : material;
			}
			continue;
		}
		/* the colors and metrics first ("Border", "CornerRadius" keep their meaning of Raven 0.2) */
		if (rvn_theme_read_value(reader, node, &rule->style, name, attribute->value, R_TRUE, R_TRUE)) continue;
		if (rvn_theme_read_part_value(reader, node, rule, name, attribute->value)) continue;
		rvn_theme_problem(reader, node, "<Style>: '%s' is neither a color, a metric nor a property of a part%s", name, "");
	}
}

static void rvn_theme_read_glass(rvn_theme_reader_t* reader, const rvn_xml_node_t* node) {
	static const struct { const char* name; RMetric metric; } names[] = {
		{ "Frost", R_METRIC_GLASS_FROST }, { "Refraction", R_METRIC_GLASS_REFRACTION },
		{ "Saturation", R_METRIC_GLASS_SATURATION }, { "Rim", R_METRIC_GLASS_RIM }, { "Shadow", R_METRIC_GLASS_SHADOW }
	};
	const rvn_xml_attribute_t* attribute;
	for (attribute = node->attributes; attribute; attribute = attribute->next) {
		size_t i;
		float number;
		for (i = 0; i < RVN_COUNT_OF(names); i++) {
			if (rvn_stricmp(names[i].name, attribute->name) == 0) break;
		}
		if (i == RVN_COUNT_OF(names)) {
			rvn_theme_problem(reader, node, "<Glass>: unknown attribute '%s'%s", attribute->name, "");
		} else if (!rvn_parse_float(attribute->value, &number)) {
			rvn_theme_problem(reader, node, "'%s' is not a number (%s)", attribute->value, attribute->name);
		} else {
			reader->theme->metrics[RVN_METRIC_INDEX(names[i].metric)] = number;
		}
	}
}

static void rvn_theme_read_syntax(rvn_theme_reader_t* reader, const rvn_xml_node_t* node) {
	const rvn_xml_attribute_t* attribute;
	for (attribute = node->attributes; attribute; attribute = attribute->next) {
		rvn_token_t token;
		rvn_color_t color;
		if (!rvn_token_by_name(attribute->name, &token)) {
			rvn_theme_problem(reader, node, "<Syntax>: unknown kind of token '%s'%s", attribute->name, "");
		} else if (!rvn_parse_color(attribute->value, &color)) {
			rvn_theme_problem(reader, node, "'%s' is not a color (%s)", attribute->value, attribute->name);
		} else {
			reader->theme->tokens[token] = color;
			reader->theme->token_mask |= 1u << token;
		}
	}
}

/*
 * Fills `theme` (zeroed) from the text of a theme file. `path` is the file
 * it comes from, or NULL. Returns R_FALSE after reporting the error.
 */
static int rvn_theme_read(rvn_theme_t* theme, const char* text, size_t size, const char* path) {
	rvn_xml_document_t document;
	rvn_theme_reader_t reader;
	const rvn_xml_node_t* root;
	const rvn_xml_node_t* child;
	const rvn_xml_attribute_t* attribute;
	const char* value;
	rvn_style_t direct;
	rvn_color_t accent;
	int has_accent = R_FALSE, has_dark = R_FALSE, flag;
	char* previous_dir = NULL;

	reader.theme = theme;
	reader.source = path ? path : "(memory)";
	reader.problems = 0;
	if (!rvn_xml_parse(&document, text, size)) {
		rvn_error(R_ERROR_XML, R_SEVERITY_ERROR, "theme %s:%d:%d: %s", reader.source, document.error_line, document.error_column, document.error);
		rvn_xml_free(&document);
		return R_FALSE;
	}
	root = document.root;
	if (!root || rvn_stricmp(root->name, "Theme") != 0) {
		rvn_error(R_ERROR_XML, R_SEVERITY_ERROR, "theme %s: the root element must be <Theme>", reader.source);
		rvn_xml_free(&document);
		return R_FALSE;
	}

	/*
	 * What it starts from: the look it names (Look="WindowsDark"), the theme
	 * it names (Base="Ember", or a look by its old name: Base="GlassDark"),
	 * else the look in use, which it then follows (it is read again over
	 * each new look).
	 */
	{
		const char* look_name = rvn_xml_attribute(root, "Look");
		const char* base_name = rvn_xml_attribute(root, "Base");
		RLook look = rvn_look_valid(g_rvn.look) ? g_rvn.look : R_LOOK_RAVEN_DARK;
		const rvn_theme_t* base = NULL;
		int follows = R_FALSE;
		if (look_name && look_name[0]) {
			if (!rvn_look_by_name(look_name, &look)) {
				rvn_theme_problem(&reader, root, "Look=\"%s\": no look has that name; the look in use is kept%s", look_name, "");
				look = rvn_look_valid(g_rvn.look) ? g_rvn.look : R_LOOK_RAVEN_DARK;
				follows = R_TRUE;
			}
		} else if (base_name && base_name[0]) {
			if (!rvn_look_by_name(base_name, &look)) {
				base = rvn_theme_find(base_name);
				if (!base) {
					rvn_theme_problem(&reader, root, "Base=\"%s\": no theme has that name; starting from the look in use%s", base_name, "");
					look = rvn_look_valid(g_rvn.look) ? g_rvn.look : R_LOOK_RAVEN_DARK;
					follows = R_TRUE;
				}
			}
		} else {
			follows = R_TRUE;
		}
		if (base) {
			rvn_theme_copy(theme, base);
			follows = base->follows_look;
		} else {
			rvn_theme_build_look(theme, look);
		}
		/* (a theme names its font itself; the one of a look comes with the look) */
		if (!theme->look_font) theme->font = R_INVALID_ID;
		rvn_free(theme->source);
		theme->source = NULL;
		theme->source_size = 0;
		theme->follows_look = follows;
		if (follows) {
			theme->source = rvn_strndup(text, size);
			theme->source_size = size;
		}
	}

	/* (fonts and images named in the file are looked for next to it) */
	if (path) rvn_xml_push_file_dir(path, &previous_dir);
	memset(&direct, 0, sizeof(direct));
	for (attribute = root->attributes; attribute; attribute = attribute->next) {
		const char* name = attribute->name;
		value = attribute->value;
		if (rvn_stricmp(name, "Name") == 0 || rvn_stricmp(name, "Base") == 0 || rvn_stricmp(name, "Look") == 0) {
			continue;
		} else if (rvn_stricmp(name, "Material") == 0) {
			RMaterial material;
			if (!rvn_parse_material(value, &material) || material == R_MATERIAL_DEFAULT) {
				rvn_theme_problem(&reader, root, "'%s' is not a material%s", value, "");
			} else if (theme->family != RVN_FAMILY_RAVEN && material != R_MATERIAL_SOLID) {
				/* (said once: a theme that follows the look is read again at each change) */
				if (!g_rvn.look_glass_warned) {
					rvn_theme_problem(&reader, root, "Material=\"%s\": the look %s is opaque; only the looks of Raven are made of glass", value, rvn_look_name(theme->look));
					g_rvn.look_glass_warned = R_TRUE;
				}
			} else {
				theme->material = material;
				theme->glass = material != R_MATERIAL_SOLID;
			}
		} else if (rvn_stricmp(name, "Motion") == 0) {
			if (rvn_stricmp(value, "Springy") == 0 || rvn_stricmp(value, "Raven") == 0) theme->motion = RVN_MOTION_SPRINGY;
			else if (rvn_stricmp(value, "Smooth") == 0 || rvn_stricmp(value, "Native") == 0) theme->motion = RVN_MOTION_SMOOTH;
			else rvn_theme_problem(&reader, root, "Motion=\"%s\": expected Springy or Smooth%s", value, "");
		} else if (rvn_stricmp(name, "Dark") == 0) {
			if (!rvn_parse_bool(value, &flag)) rvn_theme_problem(&reader, root, "Dark=\"%s\": expected true or false%s", value, "");
			else {
				theme->dark = flag;
				has_dark = R_TRUE;
			}
		} else if (rvn_stricmp(name, "Accent") == 0) {
			if (!rvn_parse_color(value, &accent)) rvn_theme_problem(&reader, root, "'%s' is not a color (Accent)%s", value, "");
			else has_accent = R_TRUE;
		} else if (rvn_stricmp(name, "Font") == 0) {
			theme->font = rvn_xml_font(value);
			if (theme->font == R_INVALID_ID) rvn_theme_problem(&reader, root, "Font=\"%s\": cannot load this font%s", value, "");
		} else if (rvn_stricmp(name, "Wallpaper") == 0) {
			if (rvn_stricmp(value, "None") == 0 || !value[0]) theme->wallpaper = R_INVALID_ID;
			else if (rvn_stricmp(value, "Dark") == 0) theme->wallpaper = rvn_theme_builtin_wallpaper(R_TRUE);
			else if (rvn_stricmp(value, "Light") == 0) theme->wallpaper = rvn_theme_builtin_wallpaper(R_FALSE);
			else if (value[0] == '#') {
				theme->wallpaper = rvn_theme_wallpaper_from_colors(value);
				if (theme->wallpaper == R_INVALID_ID) rvn_theme_problem(&reader, root, "Wallpaper=\"%s\": expected colors separated by commas%s", value, "");
			} else {
				theme->wallpaper = rvn_xml_texture(value);
				if (theme->wallpaper == R_INVALID_ID) rvn_theme_problem(&reader, root, "Wallpaper=\"%s\": cannot load this image%s", value, "");
			}
		} else if (!rvn_theme_read_value(&reader, root, &direct, name, value, R_TRUE, R_TRUE)) {
			rvn_theme_problem(&reader, root, "<Theme>: unknown attribute '%s'%s", name, "");
		}
	}
	if (path) rvn_xml_pop_file_dir(previous_dir);

	/* the accent first: a shade given by the file replaces the one worked out from it */
	if (has_accent) rvn_theme_set_accent(theme, accent);
	rvn_theme_take(theme, &direct);

	for (child = root->first_child; child; child = child->next) {
		if (rvn_stricmp(child->name, "Colors") == 0 || rvn_stricmp(child->name, "Metrics") == 0) {
			int colors = rvn_stricmp(child->name, "Colors") == 0;
			memset(&direct, 0, sizeof(direct));
			for (attribute = child->attributes; attribute; attribute = attribute->next) {
				if (!rvn_theme_read_value(&reader, child, &direct, attribute->name, attribute->value, colors, !colors)) {
					rvn_theme_problem(&reader, child, colors ? "<Colors>: '%s' is not a color role%s" : "<Metrics>: '%s' is not a metric%s", attribute->name, "");
				}
			}
			rvn_theme_take(theme, &direct);
		} else if (rvn_stricmp(child->name, "Glass") == 0) {
			rvn_theme_read_glass(&reader, child);
		} else if (rvn_stricmp(child->name, "Syntax") == 0) {
			rvn_theme_read_syntax(&reader, child);
		} else if (rvn_stricmp(child->name, "Style") == 0) {
			rvn_theme_read_style(&reader, child);
		} else {
			rvn_theme_problem(&reader, child, "unknown element <%s>%s", child->name, "");
		}
	}
	/* dark or light: said by the file, else read from the color of the text */
	if (!has_dark && !rvn_xml_attribute(root, "Base") && !rvn_xml_attribute(root, "Look")) theme->dark = rvn_color_luminance(theme->colors[RVN_COLOR_INDEX(R_COLOR_TEXT)]) > 0.5f;

	value = rvn_xml_attribute(root, "Name");
	if (value && value[0]) {
		rvn_str_set(&theme->name, value);
	} else if (path) {
		/* the name of the file without its folder and its extension */
		const char* start = path;
		const char* walk;
		const char* dot = NULL;
		for (walk = path; *walk; walk++) {
			if (*walk == '/' || *walk == '\\') start = walk + 1;
		}
		for (walk = start; *walk; walk++) {
			if (*walk == '.') dot = walk;
		}
		rvn_free(theme->name);
		theme->name = rvn_strndup(start, dot && dot > start ? (size_t)(dot - start) : strlen(start));
	} else {
		rvn_str_set(&theme->name, "Theme");
	}
	rvn_xml_free(&document);
	return R_TRUE;
}

/* Puts a theme that was just read among the named ones, in place of the one of the same name. */
static rvn_theme_t* rvn_theme_install(rvn_theme_t* loaded, const char* path) {
	rvn_theme_t* theme = NULL;
	uint32_t i;
	size_t k;
	RLook look;
	for (k = 0; k < RVN_COUNT_OF(g_builtin_themes); k++) {
		if (rvn_stricmp(g_builtin_themes[k].name, loaded->name) == 0 || rvn_look_by_name(loaded->name, &look)) {
			rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "theme: '%s' is the name of a built-in theme or of a look; give the theme another Name", loaded->name);
			rvn_theme_clear(loaded);
			return NULL;
		}
	}
	for (i = 0; i < g_rvn.theme_count && !theme; i++) {
		if (rvn_stricmp(g_rvn.themes[i]->name, loaded->name) == 0) theme = g_rvn.themes[i];
		else if (path && g_rvn.themes[i]->path && strcmp(g_rvn.themes[i]->path, path) == 0) theme = g_rvn.themes[i];
	}
	if (!theme) theme = rvn_theme_add(loaded->name);
	/* (the address stays: widgets may point to it) */
	rvn_theme_copy(theme, loaded);
	rvn_str_set(&theme->name, loaded->name);
	rvn_str_set(&theme->path, path);
	rvn_theme_clear(loaded);

	/* the theme in use is a copy of it: it follows */
	if (g_rvn.theme_name && rvn_stricmp(g_rvn.theme_name, theme->name) == 0) {
		Rid font = g_rvn.theme.font;
		int look_font = g_rvn.theme.look_font;
		rvn_theme_copy(&g_rvn.theme, theme);
		rvn_theme_settle_font(font, look_font);
	}
	rvn_theme_changed();
	return theme;
}

/*
 * The theme files that name no look are read again over the look in use.
 * (In the order they were loaded: one made from another one comes after it.)
 */
static void rvn_themes_follow_look(void) {
	uint32_t i;
	g_theme_quiet = R_TRUE;
	for (i = 0; i < g_rvn.theme_count; i++) {
		rvn_theme_t* theme = g_rvn.themes[i];
		rvn_theme_t loaded;
		char* text;
		char* path;
		size_t size;
		if (!theme->follows_look || !theme->source) continue;
		text = rvn_strndup(theme->source, theme->source_size);
		size = theme->source_size;
		path = rvn_strdup(theme->path);
		memset(&loaded, 0, sizeof(loaded));
		/* (the address, the name and the path of the theme stay: widgets point to it) */
		if (rvn_theme_read(&loaded, text, size, path)) rvn_theme_copy(theme, &loaded);
		rvn_theme_clear(&loaded);
		rvn_free(text);
		rvn_free(path);
	}
	g_theme_quiet = R_FALSE;
}

void rvn_look_apply(RLook look, int keep_theme) {
	Rid font = g_rvn.theme.font;
	int look_font = g_rvn.theme.look_font;
	rvn_theme_t* file = NULL;
	uint32_t i;

	g_rvn.look = look;
	rvn_themes_follow_look();
	for (i = 0; keep_theme && i < g_rvn.theme_count && g_rvn.theme_name; i++) {
		if (rvn_stricmp(g_rvn.themes[i]->name, g_rvn.theme_name) == 0) file = g_rvn.themes[i];
	}
	if (file && file->follows_look && file->source) {
		/* the theme file in use stays, laid over the new look */
		rvn_theme_copy(&g_rvn.theme, file);
	} else {
		/* (named after the look it is: "WindowsLight" for NativeLight on Windows) */
		RLook resolved = rvn_look_resolve(look);
		const char* name = rvn_look_old_name(resolved);
		rvn_theme_build_look(&g_rvn.theme, look);
		rvn_str_set(&g_rvn.theme_name, name ? name : rvn_look_name(resolved));
	}
	rvn_theme_settle_font(font, look_font);
	/* the looks of the platforms follow the system: no animation when the user asked for none */
	if (g_rvn.theme.family != RVN_FAMILY_RAVEN && g_rvn.platform && g_rvn.platform->a11y_settings) {
		RAccessibilitySettings settings;
		memset(&settings, 0, sizeof(settings));
		g_rvn.platform->a11y_settings(&settings);
		if (settings.reduce_motion && !Raccessibility_get_reduce_motion()) Raccessibility_set_reduce_motion(R_TRUE);
	}
	rvn_theme_changed();
}

static rvn_theme_t* rvn_theme_load_file(const char* path) {
	rvn_theme_t loaded;
	size_t size = 0;
	char* text = rvn_read_file(path, &size);
	int ok;
	if (!text) {
		rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "theme: cannot read '%s'", path);
		return NULL;
	}
	memset(&loaded, 0, sizeof(loaded));
	ok = rvn_theme_read(&loaded, text, size, path);
	rvn_free(text);
	if (!ok) {
		rvn_theme_clear(&loaded);
		return NULL;
	}
	return rvn_theme_install(&loaded, path);
}

static int rvn_theme_watch(const char* path, void* user) {
	RVN_UNUSED(user);
	/* a file caught half written: quietly tried again before saying anything */
	if (rvn_watch_quiet()) {
		rvn_xml_document_t document;
		size_t size = 0;
		char* text = rvn_read_file(path, &size);
		int parsed;
		if (!text) return R_FALSE;
		parsed = rvn_xml_parse(&document, text, size);
		rvn_xml_free(&document);
		rvn_free(text);
		if (!parsed) return R_FALSE;
	}
	rvn_theme_load_file(path);
	return R_TRUE;
}

const rvn_theme_t* rvn_xml_theme(const char* value) {
	rvn_theme_t* theme = rvn_theme_find(value);
	char* path;
	uint32_t i;
	if (theme) return theme;
	if (!strchr(value, '.') && !strchr(value, '/') && !strchr(value, '\\')) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Theme=\"%s\": no theme has that name", value);
		return NULL;
	}
	/* a theme file, named from the document that uses it */
	path = rvn_xml_resolve_path(value);
	for (i = 0; i < g_rvn.theme_count; i++) {
		if (g_rvn.themes[i]->path && strcmp(g_rvn.themes[i]->path, path) == 0) {
			rvn_free(path);
			return g_rvn.themes[i];
		}
	}
	theme = rvn_theme_load_file(path);
	if (theme) rvn_watch_add(path, rvn_theme_watch, NULL);
	rvn_free(path);
	return theme;
}

/* ---------------------------------------------------------------------------
 *  Writing a theme
 * ------------------------------------------------------------------------- */

static void rvn_theme_write_color(rvn_text_t* out, const char* name, rvn_color_t color) {
	float r, g, b, a;
	rvn_color_to_floats(color, &r, &g, &b, &a);
	rvn_text_printf(out, " %s=\"#%02X%02X%02X", name, (unsigned)(r * 255.0f + 0.5f), (unsigned)(g * 255.0f + 0.5f), (unsigned)(b * 255.0f + 0.5f));
	if (a < 0.998f) rvn_text_printf(out, "%02X", (unsigned)(a * 255.0f + 0.5f));
	rvn_text_add(out, "\"");
}

static void rvn_theme_write_style(rvn_text_t* out, const rvn_style_t* style, int colors, int metrics, const char* indent) {
	int i, column = 0;
	for (i = 0; i < R_COLOR_COUNT && colors; i++) {
		const char* name = rvn_color_role_name((RColorRole)((int)R_COLOR_WINDOW + i));
		if (!name || !(style->color_mask & ((uint64_t)1 << i))) continue;
		if (indent && column++ % 4 == 0) rvn_text_printf(out, "\n%s", indent);
		rvn_theme_write_color(out, name, style->colors[i]);
	}
	for (i = 0; i < R_METRIC_COUNT && metrics; i++) {
		const char* name = rvn_metric_name((RMetric)((int)R_METRIC_FONT_SIZE + i));
		if (!name || !(style->metric_mask & (1u << i))) continue;
		if (indent && column++ % 4 == 0) rvn_text_printf(out, "\n%s", indent);
		rvn_text_printf(out, " %s=\"%g\"", name, (double)style->metrics[i]);
	}
}

/* The properties a rule gives to a part. */
static void rvn_theme_write_look(rvn_text_t* out, const rvn_style_rule_t* rule) {
	const rvn_part_look_t* look = &rule->look;
	const rvn_color_t colors[6] = { look->background, look->foreground, look->border, look->border_top, look->border_bottom, look->shadow_color };
	const float numbers[6] = { look->border_width, look->border_bottom_width, look->radius, look->size, look->opacity, look->shadow };
	int i;
	for (i = 0; i < 6; i++) {
		if (rule->look_mask & (1u << i)) rvn_theme_write_color(out, rvn_part_property_name(1u << i), colors[i]);
	}
	for (i = 0; i < 6; i++) {
		if (rule->look_mask & (1u << (i + 6))) rvn_text_printf(out, " %s=\"%g\"", rvn_part_property_name(1u << (i + 6)), (double)numbers[i]);
	}
}

static const char* rvn_material_name(RMaterial material) {
	switch (material) {
		case R_MATERIAL_GLASS: return "Glass";
		case R_MATERIAL_FROSTED_GLASS: return "FrostedGlass";
		case R_MATERIAL_TINTED_GLASS: return "TintedGlass";
		default: return "Solid";
	}
}

static void rvn_theme_write(const rvn_theme_t* theme, const char* name, rvn_text_t* out) {
	rvn_style_t all;
	uint32_t i;
	size_t k;
	/* (a theme of the look of Raven names its look by the name of the material it is in) */
	RLook look = theme->family != RVN_FAMILY_RAVEN ? theme->look :
		theme->glass ? (theme->dark ? R_LOOK_RAVEN_GLASS_DARK : R_LOOK_RAVEN_GLASS_LIGHT) : (theme->dark ? R_LOOK_RAVEN_DARK : R_LOOK_RAVEN_LIGHT);
	if (theme->family == RVN_FAMILY_RAVEN && (theme->look == R_LOOK_RAVEN_HIGH_CONTRAST_DARK || theme->look == R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT)) look = theme->look;

	rvn_text_printf(out, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Theme Name=\"%s\" Look=\"%s\" Material=\"%s\" Dark=\"%s\" Motion=\"%s\"",
		name, rvn_look_name(look), rvn_material_name(theme->material), theme->dark ? "true" : "false",
		theme->motion == RVN_MOTION_SMOOTH ? "Smooth" : "Springy");
	if (theme->glass && theme->wallpaper == R_INVALID_ID) rvn_text_add(out, " Wallpaper=\"None\"");
	rvn_text_add(out, ">\n");

	memset(&all, 0, sizeof(all));
	memcpy(all.colors, theme->colors, sizeof(all.colors));
	memcpy(all.metrics, theme->metrics, sizeof(all.metrics));
	all.color_mask = ~(uint64_t)0;
	all.metric_mask = ~0u;
	rvn_text_add(out, "\t<Colors");
	rvn_theme_write_style(out, &all, R_TRUE, R_FALSE, "\t\t");
	rvn_text_add(out, "/>\n\t<Metrics");
	rvn_theme_write_style(out, &all, R_FALSE, R_TRUE, "\t\t");
	rvn_text_add(out, "/>\n");

	if (theme->token_mask) {
		rvn_text_add(out, "\t<Syntax");
		for (k = 0; k < RVN_COUNT_OF(g_token_names); k++) {
			if (theme->token_mask & (1u << g_token_names[k].token)) rvn_theme_write_color(out, g_token_names[k].name, theme->tokens[g_token_names[k].token]);
		}
		rvn_text_add(out, "/>\n");
	}
	for (i = 0; i < theme->rule_count; i++) {
		const rvn_style_rule_t* rule = &theme->rules[i];
		const rvn_widget_class_t* cls = rule->type ? rvn_widget_class_by_type((RWidgetType)rule->type) : NULL;
		if (!rule->style.color_mask && !rule->style.metric_mask && !rule->material && !rule->look_mask) continue;
		rvn_text_add(out, "\t<Style");
		if (cls) rvn_text_printf(out, " Widget=\"%s\"", cls->name);
		if (rule->cls) rvn_text_printf(out, " Class=\"%s\"", rule->cls);
		if (rule->part) rvn_text_printf(out, " Part=\"%s\"", rvn_part_name(rule->part));
		if (rule->states) {
			rvn_text_add(out, " State=\"");
			rvn_states_write(rule->states, out);
			rvn_text_add(out, "\"");
		}
		if (rule->material) rvn_text_printf(out, " Material=\"%s\"", rvn_material_name(rule->material));
		if (!rule->states) rvn_theme_write_style(out, &rule->style, R_TRUE, R_TRUE, NULL);
		rvn_theme_write_look(out, rule);
		rvn_text_add(out, "/>\n");
	}
	rvn_text_add(out, "</Theme>\n");
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

void Rtheme_set_accent(float r, float g, float b) {
	RVN_API_GUARD_VOID();
	rvn_theme_set_accent(&g_rvn.theme, rvn_rgba(r, g, b, 1.0f));
	rvn_theme_changed();
}

void Rtheme_set_dark(int dark) {
	RVN_API_GUARD_VOID();
	g_rvn.theme.dark = dark != 0;
	rvn_theme_changed();
}

int Rtheme_is_dark(void) {
	return g_rvn.initialized ? g_rvn.theme.dark : R_FALSE;
}

void Rtheme_set_wallpaper(Rid texture) {
	RVN_API_GUARD_VOID();
	if (texture != R_INVALID_ID && !rvn_texture_get(texture)) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rtheme_set_wallpaper: invalid texture");
		return;
	}
	g_rvn.theme.wallpaper = texture;
	rvn_repaint_all_windows();
}

Rid Rtheme_get_wallpaper(void) {
	return g_rvn.initialized ? g_rvn.theme.wallpaper : R_INVALID_ID;
}

void Rtheme_set_style_color(const char* widget, const char* style_class, RColorRole role, float r, float g, float b, float a) {
	rvn_style_rule_t* rule;
	RVN_API_GUARD_VOID();
	if (role < R_COLOR_WINDOW || role >= R_COLOR_LAST_) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtheme_set_style_color: invalid color role");
		return;
	}
	rule = rvn_theme_rule(&g_rvn.theme, widget, style_class, "Rtheme_set_style_color");
	if (!rule) return;
	rule->style.colors[RVN_COLOR_INDEX(role)] = rvn_rgba(r, g, b, a);
	rule->style.color_mask |= (uint64_t)1 << RVN_COLOR_INDEX(role);
	rvn_theme_changed();
}

void Rtheme_set_style_metric(const char* widget, const char* style_class, RMetric metric, float value) {
	rvn_style_rule_t* rule;
	RVN_API_GUARD_VOID();
	if (metric < R_METRIC_FONT_SIZE || metric >= R_METRIC_LAST_) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtheme_set_style_metric: invalid metric");
		return;
	}
	rule = rvn_theme_rule(&g_rvn.theme, widget, style_class, "Rtheme_set_style_metric");
	if (!rule) return;
	rule->style.metrics[RVN_METRIC_INDEX(metric)] = value;
	rule->style.metric_mask |= 1u << RVN_METRIC_INDEX(metric);
	rvn_theme_changed();
}

void Rtheme_set_style_material(const char* widget, const char* style_class, RMaterial material) {
	rvn_style_rule_t* rule;
	RVN_API_GUARD_VOID();
	if (material < R_MATERIAL_SOLID || material > R_MATERIAL_DEFAULT) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtheme_set_style_material: invalid material");
		return;
	}
	rule = rvn_theme_rule(&g_rvn.theme, widget, style_class, "Rtheme_set_style_material");
	if (!rule) return;
	rule->material = material == R_MATERIAL_DEFAULT ? (RMaterial)0 : material;
	rvn_theme_changed();
}

void Rtheme_clear_styles(void) {
	RVN_API_GUARD_VOID();
	rvn_mutex_lock(&g_rvn.style_mutex);
	rvn_theme_free_rules(&g_rvn.theme);
	rvn_mutex_unlock(&g_rvn.style_mutex);
	rvn_theme_changed();
}

const char* Rtheme_load(const char* path) {
	rvn_theme_t* theme;
	RVN_API_GUARD(NULL);
	if (!path) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtheme_load: NULL path");
		return NULL;
	}
	theme = rvn_theme_load_file(path);
	if (!theme) return NULL;
	rvn_watch_add(path, rvn_theme_watch, NULL);
	return theme->name;
}

const char* Rtheme_load_from_memory(const char* xml, size_t size) {
	rvn_theme_t loaded;
	rvn_theme_t* theme;
	RVN_API_GUARD(NULL);
	if (!xml) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtheme_load_from_memory: NULL text");
		return NULL;
	}
	memset(&loaded, 0, sizeof(loaded));
	if (!rvn_theme_read(&loaded, xml, size ? size : strlen(xml), NULL)) {
		rvn_theme_clear(&loaded);
		return NULL;
	}
	theme = rvn_theme_install(&loaded, NULL);
	return theme ? theme->name : NULL;
}

int Rtheme_use(const char* name) {
	rvn_theme_t* theme;
	Rid font;
	int look_font;
	RLook look;
	RVN_API_GUARD(R_FALSE);
	theme = rvn_theme_find(name);
	if (!theme) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtheme_use: no theme is called '%s'", name ? name : "(null)");
		return R_FALSE;
	}
	/* the name of a look ("WindowsDark", "Dark"): that look becomes the one of the application */
	if (!theme->path && !theme->source && rvn_look_by_name(name, &look)) {
		rvn_look_apply(look, R_FALSE);
		return R_TRUE;
	}
	font = g_rvn.theme.font;
	look_font = g_rvn.theme.look_font;
	rvn_theme_copy(&g_rvn.theme, theme);
	/* a theme that names no font keeps the one in use */
	rvn_theme_settle_font(font, look_font);
	rvn_str_set(&g_rvn.theme_name, theme->name);
	rvn_theme_changed();
	return R_TRUE;
}

const char* Rtheme_get_name(void) {
	return g_rvn.initialized && g_rvn.theme_name ? g_rvn.theme_name : "";
}

int Rtheme_get_count(void) {
	RVN_API_GUARD(0);
	rvn_theme_builtins();
	return (int)g_rvn.theme_count;
}

const char* Rtheme_get_name_at(int index) {
	RVN_API_GUARD(NULL);
	rvn_theme_builtins();
	if (index < 0 || index >= (int)g_rvn.theme_count) return NULL;
	return g_rvn.themes[index]->name;
}

int Rtheme_save(const char* path, const char* name) {
	rvn_text_t out;
	int ok;
	RVN_API_GUARD(R_FALSE);
	if (!path) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtheme_save: NULL path");
		return R_FALSE;
	}
	memset(&out, 0, sizeof(out));
	rvn_theme_write(&g_rvn.theme, name && name[0] ? name : (g_rvn.theme_name ? g_rvn.theme_name : "Theme"), &out);
	ok = Rfile_write(path, out.data, out.length);
	rvn_free(out.data);
	return ok;
}

/* ---- widgets ---- */

static void rvn_widget_classes_changed(rvn_widget_t* widget) {
	widget->style_generation = 0;
	rvn_widget_invalidate_layout(widget);
}

void Rwidget_set_class(Rid id, const char* classes) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (rvn_str_set(&widget->classes, classes && classes[0] ? classes : NULL)) rvn_widget_classes_changed(widget);
}

const char* Rwidget_get_class(Rid id) {
	RVN_WIDGET(widget, id, 0, "");
	return widget->classes ? widget->classes : "";
}

static int rvn_class_name_ok(const char* cls, const char* function) {
	if (!cls || !cls[0] || strchr(cls, ' ') || strchr(cls, '\t') || strchr(cls, ',')) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: a class is one word", function);
		return R_FALSE;
	}
	return R_TRUE;
}

void Rwidget_add_class(Rid id, const char* style_class) {
	rvn_text_t text;
	RVN_WIDGET_VOID(widget, id, 0);
	if (!rvn_class_name_ok(style_class, "Rwidget_add_class") || rvn_classes_have(widget->classes, style_class)) return;
	memset(&text, 0, sizeof(text));
	if (widget->classes) {
		rvn_text_add(&text, widget->classes);
		rvn_text_add(&text, " ");
	}
	rvn_text_add(&text, style_class);
	rvn_free(widget->classes);
	widget->classes = text.data;
	rvn_widget_classes_changed(widget);
}

void Rwidget_remove_class(Rid id, const char* style_class) {
	rvn_text_t text;
	const char* walk;
	size_t length;
	RVN_WIDGET_VOID(widget, id, 0);
	if (!rvn_class_name_ok(style_class, "Rwidget_remove_class") || !rvn_classes_have(widget->classes, style_class)) return;
	memset(&text, 0, sizeof(text));
	length = strlen(style_class);
	walk = widget->classes;
	while (*walk) {
		const char* start;
		while (*walk == ' ' || *walk == '\t' || *walk == ',') walk++;
		start = walk;
		while (*walk && *walk != ' ' && *walk != '\t' && *walk != ',') walk++;
		if (walk == start) break;
		if ((size_t)(walk - start) == length && memcmp(start, style_class, length) == 0) continue;
		if (text.length) rvn_text_add(&text, " ");
		rvn_text_append(&text, start, (size_t)(walk - start));
	}
	rvn_free(widget->classes);
	widget->classes = text.data;
	rvn_widget_classes_changed(widget);
}

int Rwidget_has_class(Rid id, const char* style_class) {
	RVN_WIDGET(widget, id, 0, R_FALSE);
	return rvn_classes_have(widget->classes, style_class);
}

int Rwidget_set_theme(Rid id, const char* name) {
	const rvn_theme_t* theme = NULL;
	RVN_WIDGET(widget, id, 0, R_FALSE);
	if (name && name[0]) {
		theme = rvn_theme_find(name);
		if (!theme) {
			rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rwidget_set_theme: no theme is called '%s'", name);
			return R_FALSE;
		}
	}
	rvn_widget_set_theme(widget, theme, theme != NULL);
	return R_TRUE;
}

const char* Rwidget_get_theme(Rid id) {
	RVN_WIDGET(widget, id, 0, "");
	if (widget->theme && widget->theme->name) return widget->theme->name;
	return g_rvn.theme_name ? g_rvn.theme_name : "";
}
