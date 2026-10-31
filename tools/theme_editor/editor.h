/**
 * Raven theme editor - editor.h
 * What the files of the theme editor share: the theme being edited (the
 * model), the tables of what a theme holds, and small text helpers.
 *
 *   model.c   the model: theme files written and read, the palette worked out
 *             from an accent, contrast
 *   main.c    the window: the panels that edit the model, the preview, files,
 *             undo, and the scripted runs of the tests
 */

#ifndef RAVEN_THEME_EDITOR_H
#define RAVEN_THEME_EDITOR_H

#include <Raven.h>

#include <stdarg.h>
#include <stddef.h>

#define TE_COLOR_COUNT 36
#define TE_METRIC_COUNT 20
#define TE_SYNTAX_COUNT 14

/* ---- text ---- */

typedef struct te_text {
	char* data;
	size_t length, capacity;
} te_text;

void te_add(te_text* text, const char* s);
void te_addn(te_text* text, const char* s, size_t n);
void te_printf(te_text* text, const char* format, ...);
/* s with & < > " escaped, for an attribute */
void te_add_escaped(te_text* text, const char* s);
void te_text_free(te_text* text);
char* te_strdup(const char* s);
void te_copy(char* dst, size_t size, const char* src);
int te_stricmp(const char* a, const char* b);
/* R_TRUE when `needle` is found in `haystack`, ignoring case */
int te_contains(const char* haystack, const char* needle);
/* "1.5", "14", "0.25": a number without trailing zeros */
const char* te_number(float value, char* buffer, size_t size);

/* ---- colors ---- */

typedef struct te_color {
	int set;
	float c[4];
} te_color;

/* "#RGB", "#RGBA", "#RRGGBB", "#RRGGBBAA" */
int te_parse_hex(const char* text, float* rgba);
/* "#RRGGBB", or "#RRGGBBAA" when not opaque */
const char* te_hex(const float* rgba, char* buffer, size_t size);
void te_rgb_to_hsl(const float* rgb, float* h, float* s, float* l);
void te_hsl_to_rgb(float h, float s, float l, float* rgb);
/* WCAG contrast of `fg` over `bg` (the alpha of fg blends it over bg first) */
float te_contrast(const float* fg, const float* bg);
/* `top` laid over `bottom` with its alpha */
void te_blend(const float* top, const float* bottom, float* out);
/* Changes the lightness of `fg` (its hue, saturation and alpha stay) as little as it takes to reach `target` over `bg` */
void te_fix_contrast(float* fg, const float* bg, float target);

/* ---- what a theme holds ---- */

typedef struct te_color_info {
	RColorRole role;
	const char* name;
	const char* group;
	const char* help;
} te_color_info;

typedef struct te_metric_info {
	RMetric metric;
	/* the attribute: in <Metrics>, or in <Glass> when `glass` */
	const char* name;
	const char* label;
	const char* group;
	float minimum, maximum, step;
	int decimals, glass;
	const char* help;
} te_metric_info;

typedef struct te_syntax_info {
	RToken token;
	const char* name;
	const char* sample;
	/* the colors Raven gives without a theme (0: the color of the text) */
	unsigned dark, light;
} te_syntax_info;

typedef struct te_look_info {
	/* the name in a theme file (NULL: the theme follows the look of the application) */
	const char* name;
	const char* label;
	RLook look;
	int dark, raven;
} te_look_info;

extern const te_color_info g_te_colors[TE_COLOR_COUNT];
extern const te_metric_info g_te_metrics[TE_METRIC_COUNT];
extern const te_syntax_info g_te_syntax[TE_SYNTAX_COUNT];
extern const te_look_info g_te_looks[];
extern const int g_te_look_count;

extern const char* const g_te_materials[];   /* "", "Solid", "Glass", "FrostedGlass", "TintedGlass" */
extern const char* const g_te_motions[];     /* "", "Springy", "Smooth" */
extern const char* const g_te_wallpapers[];  /* "", "None", "Dark", "Light", (colors), (image) */

enum { TE_WALLPAPER_LOOK, TE_WALLPAPER_NONE, TE_WALLPAPER_DARK, TE_WALLPAPER_LIGHT, TE_WALLPAPER_COLORS, TE_WALLPAPER_IMAGE };

/* ---- the model ---- */

typedef struct te_model {
	char name[96];
	/* index in g_te_looks; 0: follows the look of the application */
	int look;
	/* without a look: the theme it starts from (Base="Ember") */
	char base[96];
	/* 0: as the look says, 1: dark, 2: light */
	int dark;
	/* indices in g_te_materials, g_te_motions, g_te_wallpapers */
	int material, motion, wallpaper;
	int accent_set;
	float accent[3];
	/* the colors of the wallpaper ("#123, #456") or the path of its image */
	char wallpaper_text[512];
	/* the path of a .ttf file; empty: the font of the look */
	char font[512];
	te_color colors[TE_COLOR_COUNT];
	int metric_set[TE_METRIC_COUNT];
	float metrics[TE_METRIC_COUNT];
	te_color syntax[TE_SYNTAX_COUNT];
	/* the <Style> rules, as they are written in the file (and anything else the editor does not edit) */
	char* styles;
} te_model;

void te_model_init(te_model* model);
void te_model_free(te_model* model);
void te_model_copy(te_model* dst, const te_model* src);
int te_model_equal(const te_model* a, const te_model* b);
/* dark or light, from Dark=, else from the look (the look of the application when it follows) */
int te_model_is_dark(const te_model* model, int application_dark);

/*
 * The theme file. `name`: the Name written (NULL: the one of the model).
 * `base_only`: only what the colors and the sizes start from (the look, the
 * accent, the material...), for reading the values a theme inherits.
 * `dir`: the folder relative paths are read from (NULL: as written).
 */
void te_model_write(const te_model* model, te_text* out, const char* name, int base_only, const char* dir);
/* Fills the model from a theme file. R_FALSE (and a message in `error`) when it is not one. */
int te_model_read(te_model* model, const char* xml, size_t size, char* error, size_t error_size);

/*
 * Every color worked out from the accent: surfaces tinted with its hue
 * (`tint` 0..1), as far apart as `contrast` says (0.5..1.5), readable text,
 * states, selection, and status colors that match.
 */
void te_generate_palette(te_model* model, int dark, float tint, float contrast);

/* ---- the editor inside another program (the Theme tab of the designer) ---- */

/** Builds the editor into a widget for `window` (its dialogs belong to it); R_INVALID_ID on failure. */
Rid  te_embed(Rid window);
/** Opens a theme file. R_FALSE when it cannot be read. */
int  te_open(const char* path);
int  te_is_modified(void);
/** Saves into its file (asks where when it has none). */
int  te_save(void);
/** The file of the theme ("" when not saved yet). */
const char* te_get_path(void);
/** R_TRUE: no message box (the scripted runs of the tests). */
void te_set_scripted(int scripted);
/** R_TRUE during the random runs of the tests: no dialog opens. */
void te_set_fuzzing(int fuzzing);
/** What the library reports: the host passes it on (the Styles tab shows the mistakes). */
void te_error(RError code, RSeverity severity, const char* detail);
void te_shutdown(void);

#endif
