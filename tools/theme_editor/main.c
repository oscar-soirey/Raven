/**
 * Raven theme editor - main.c
 * Makes Raven theme files (docs/THEMES.md) by looking at them: every color,
 * size, glass setting, syntax color and style rule of a theme, edited on the
 * left and shown live on the right, on pages of real widgets drawn with the
 * theme being made.
 *
 *   raven_theme_editor [theme.xml] [--null | --headless] [--frames N] [--script "..."]
 *
 *   --null             no window system and no GPU (the tests)
 *   --headless         no window system
 *   --frames N         stops after N frames
 *   --script "..."     commands run before the frames: see run_script
 *
 * How the preview works: the theme being edited is written as a theme file
 * and loaded with Rtheme_load_from_memory under a name of its own
 * ("ThemeEditor.Preview"), given to the preview with Rwidget_set_theme (the
 * Theme attribute). Loading a theme again under the same name changes it in
 * place, so the preview follows each change while the editor keeps its own
 * look. What a theme inherits from its look is read the same way, from a
 * second theme that only names the look, the accent and the material
 * ("ThemeEditor.Base"), on a hidden widget.
 */

#include "editor.h"
#include "../crash_report.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PREVIEW_THEME "ThemeEditor.Preview"
#define BASE_THEME "ThemeEditor.Base"
#define UNDO_DEPTH 64

/* The pairs of colors whose contrast is checked (WCAG 2). */
typedef struct check_pair {
	RColorRole fg, bg;
	/* the background is the selection laid over the panel */
	int over_panel;
	const char* label;
	/* 4.5 for text, 3 for large text and the parts of controls */
	float target;
} check_pair;

static const check_pair g_pairs[] = {
	{ R_COLOR_TEXT, R_COLOR_WINDOW, 0, "Text on the window", 4.5f },
	{ R_COLOR_TEXT, R_COLOR_PANEL, 0, "Text on panels", 4.5f },
	{ R_COLOR_TEXT, R_COLOR_CONTROL, 0, "Text on buttons", 4.5f },
	{ R_COLOR_TEXT, R_COLOR_INPUT, 0, "Text in fields", 4.5f },
	{ R_COLOR_ACCENT_TEXT, R_COLOR_ACCENT, 0, "Text on the accent", 4.5f },
	{ R_COLOR_TEXT, R_COLOR_SELECTION, 1, "Selected rows", 4.5f },
	{ R_COLOR_TEXT, R_COLOR_POPUP, 0, "Text in menus", 4.5f },
	{ R_COLOR_TOOLTIP_TEXT, R_COLOR_TOOLTIP, 0, "Tooltips", 4.5f },
	{ R_COLOR_LINK, R_COLOR_WINDOW, 0, "Links", 4.5f },
	{ R_COLOR_TEXT_PLACEHOLDER, R_COLOR_INPUT, 0, "Hints in empty fields", 3.0f },
	{ R_COLOR_TEXT_DISABLED, R_COLOR_PANEL, 0, "Disabled text (no rule: kept apart)", 2.0f },
	{ R_COLOR_ACCENT, R_COLOR_WINDOW, 0, "The accent on the window", 3.0f },
	{ R_COLOR_FOCUS, R_COLOR_WINDOW, 0, "The focus ring", 3.0f },
	{ R_COLOR_CONTROL_BORDER, R_COLOR_WINDOW, 0, "Outlines of controls", 3.0f },
	{ R_COLOR_DANGER, R_COLOR_PANEL, 0, "Danger on panels", 3.0f },
	{ R_COLOR_SUCCESS, R_COLOR_PANEL, 0, "Success on panels", 3.0f }
};
#define PAIR_COUNT ((int)(sizeof(g_pairs) / sizeof(g_pairs[0])))

/* Rules offered by "Add a rule" in the Styles tab ({accent} is the accent in use). */
static const struct { const char* label; const char* rule; } g_snippets[] = {
	{ "Add a rule...", NULL },
	{ "Buttons that destroy (Class=\"danger\")", "<Style Widget=\"Button\" Class=\"danger\" Accent=\"#E5484D\" AccentHovered=\"#F0606A\"/>" },
	{ "Pill-shaped buttons", "<Style Widget=\"Button\" CornerRadius=\"14\"/>" },
	{ "Cards (Class=\"card\")", "<Style Widget=\"Border\" Class=\"card\" Background=\"#8080801A\" CornerRadius=\"12\"/>" },
	{ "Quiet labels (Class=\"dim\")", "<Style Widget=\"Label\" Class=\"dim\" Text=\"#8A8F98\"/>" },
	{ "Slider knob under the pointer", "<Style Widget=\"Slider\" Part=\"Thumb\" State=\"Hovered\" Background=\"{accent}\"/>" },
	{ "Thicker boxes when checked", "<Style Widget=\"Checkbox\" Part=\"Box\" State=\"Checked\" BorderWidth=\"2\"/>" },
	{ "The tab in front, in the accent", "<Style Widget=\"TabView\" Part=\"Tab\" State=\"Selected\" Foreground=\"{accent}\"/>" },
	{ "Square text fields", "<Style Widget=\"TextInput\" CornerRadius=\"0\"/>" }
};
#define SNIPPET_COUNT ((int)(sizeof(g_snippets) / sizeof(g_snippets[0])))

static const char* const g_backdrops[] = { "Automatic", "The window color", "Soft gradient", "Vivid gradient", "Checkerboard" };
#define BACKDROP_COUNT 5

static struct {
	/* the window, and what holds the editor in it (the window itself, or a panel of the designer) */
	Rid window, root, preview_window;
	int embedded;
	Rid preview_root, preview_backdrop, probe, preview_button;
	Rid tabs, name, look, dark, material, motion, accent_own, accent, tint, contrast, tint_value, contrast_value;
	Rid font, wallpaper, wallpaper_text, wallpaper_browse, look_hint;
	Rid filter, styles, styles_error, snippets, xml, xml_apply, xml_revert, status, file_label, check_summary, backdrop;
	Rid preview_enabled, undo_button, redo_button;
	Rid color_row[TE_COLOR_COUNT], color_picker[TE_COLOR_COUNT], color_hex[TE_COLOR_COUNT], color_reset[TE_COLOR_COUNT];
	Rid color_group[16];
	int color_group_count;
	Rid metric_slider[TE_METRIC_COUNT], metric_spin[TE_METRIC_COUNT], metric_reset[TE_METRIC_COUNT], metric_state[TE_METRIC_COUNT];
	Rid syntax_picker[TE_SYNTAX_COUNT], syntax_hex[TE_SYNTAX_COUNT], syntax_reset[TE_SYNTAX_COUNT];
	Rid check_swatch[PAIR_COUNT], check_sample[PAIR_COUNT], check_ratio[PAIR_COUNT], check_badge[PAIR_COUNT], check_fix[PAIR_COUNT];
	Rid compare;
	Rid styles_timer, backdrop_texture;

	te_model model, saved;
	te_model undo[UNDO_DEPTH], redo[UNDO_DEPTH];
	int undo_count, redo_count;
	char last_change[64];
	double last_change_time;

	float base_colors[TE_COLOR_COUNT][4];
	float base_metrics[TE_METRIC_COUNT];
	char* base_xml;
	char path[1024], dir[1024];
	char backdrop_key[64];

	int syncing, xml_edited, scripted;
	/* random input (the fuzz command): no file dialog, no message box, no quitting */
	int fuzzing;
	/* the control the user is moving: the model follows it, not the reverse */
	Rid source;
	te_text errors;
} g;

static void refresh(void);
static void ui_from_model(void);
static void changed(void);

/* ---------------------------------------------------------------------------
 *  Small things
 * ------------------------------------------------------------------------- */

static Rid W(const char* name) {
	Rid widget = Rwidget_find(g.root, name);
	if (widget == R_INVALID_ID) fprintf(stderr, "raven_theme_editor: no widget '%s'\n", name);
	return widget;
}

static void on_error(RError code, RSeverity severity, const char* detail) {
	(void)code;
	(void)severity;
	if (g.errors.length < 4000) {
		if (g.errors.length) te_add(&g.errors, "\n");
		te_add(&g.errors, detail);
	}
	if (g.scripted) fprintf(stderr, "raven: %s\n", detail);
}

static void set_status(const char* format, ...) {
	char text[600];
	va_list args;
	va_start(args, format);
	vsnprintf(text, sizeof(text), format, args);
	va_end(args);
	Rlabel_set_text(g.status, text);
}

static void effective_color(int index, float* rgba) {
	memcpy(rgba, g.model.colors[index].set ? g.model.colors[index].c : g.base_colors[index], 4 * sizeof(float));
}

static void role_color(RColorRole role, float* rgba) {
	int i;
	for (i = 0; i < TE_COLOR_COUNT; i++) {
		if (g_te_colors[i].role == role) {
			effective_color(i, rgba);
			return;
		}
	}
	rgba[0] = rgba[1] = rgba[2] = 0.0f;
	rgba[3] = 1.0f;
}

static float effective_metric(int index) {
	return g.model.metric_set[index] ? g.model.metrics[index] : g.base_metrics[index];
}

static int model_dark(void) {
	return te_model_is_dark(&g.model, Rtheme_is_dark());
}

static void syntax_default(int index, float* rgba) {
	unsigned rgb = model_dark() ? g_te_syntax[index].dark : g_te_syntax[index].light;
	if (!rgb) {
		role_color(R_COLOR_TEXT, rgba);
		return;
	}
	rgba[0] = (float)((rgb >> 16) & 255) / 255.0f;
	rgba[1] = (float)((rgb >> 8) & 255) / 255.0f;
	rgba[2] = (float)(rgb & 255) / 255.0f;
	rgba[3] = 1.0f;
}

static void effective_syntax(int index, float* rgba) {
	if (g.model.syntax[index].set) memcpy(rgba, g.model.syntax[index].c, 4 * sizeof(float));
	else syntax_default(index, rgba);
}

/* ---------------------------------------------------------------------------
 *  Undo
 * ------------------------------------------------------------------------- */

static void push(te_model* stack, int* count, const te_model* model) {
	if (*count == UNDO_DEPTH) {
		te_model_free(&stack[0]);
		memmove(stack, stack + 1, (UNDO_DEPTH - 1) * sizeof(te_model));
		(*count)--;
		memset(&stack[*count], 0, sizeof(te_model));
	}
	memset(&stack[*count], 0, sizeof(te_model));
	te_model_copy(&stack[*count], model);
	(*count)++;
}

static void clear_stack(te_model* stack, int* count) {
	while (*count > 0) te_model_free(&stack[--(*count)]);
}

/*
 * Called before the model changes. Changes of the same thing that follow
 * each other closely (a slider dragged, a color picked) are one step.
 */
static void begin_change(const char* what) {
	double now = Rget_time();
	if (strcmp(what, g.last_change) != 0 || now - g.last_change_time > 1.0) {
		push(g.undo, &g.undo_count, &g.model);
		clear_stack(g.redo, &g.redo_count);
	}
	te_copy(g.last_change, sizeof(g.last_change), what);
	g.last_change_time = now;
}

static void undo_redo(int redo) {
	te_model* from = redo ? g.redo : g.undo;
	int* count = redo ? &g.redo_count : &g.undo_count;
	if (*count == 0) {
		set_status(redo ? "Nothing to redo." : "Nothing to undo.");
		return;
	}
	push(redo ? g.undo : g.redo, redo ? &g.undo_count : &g.redo_count, &g.model);
	te_model_copy(&g.model, &from[*count - 1]);
	te_model_free(&from[--(*count)]);
	g.last_change[0] = '\0';
	ui_from_model();
	refresh();
	set_status(redo ? "Redone." : "Undone.");
}

/* ---------------------------------------------------------------------------
 *  The interface
 * ------------------------------------------------------------------------- */

static void add_eyebrow(te_text* x, const char* text) {
	te_printf(x, "<Label Text=\"%s\" TextSize=\"11\" Opacity=\"0.55\" Slot.Padding=\"0,10,0,0\"/>", text);
}

static void add_hint(te_text* x, const char* text) {
	te_add(x, "<Label Wrap=\"true\" Opacity=\"0.6\" TextSize=\"12\" Text=\"");
	te_add_escaped(x, text);
	te_add(x, "\"/>");
}

static void add_items(te_text* x, const char* const* items, int count) {
	int i;
	for (i = 0; i < count; i++) {
		te_add(x, "<Item Text=\"");
		te_add_escaped(x, items[i]);
		te_add(x, "\"/>");
	}
}

/* The pages of the preview: real widgets, drawn with the theme being edited. */
static void add_preview_pages(te_text* x, const char* prefix) {
	te_add(x, "<TabView Name=\"");
	te_add(x, prefix);
	te_add(x, "pages\">");

	/* controls */
	te_add(x,
		"<ScrollBox Slot.Title=\"Controls\"><VBox Spacing=\"14\" Slot.Padding=\"22\">"
		"<VBox Spacing=\"2\"><Label Text=\"Every control, every state\" TextSize=\"20\"/>"
		"<Label Class=\"dim\" Opacity=\"0.7\" Text=\"Hover, press, type and drag: the preview is alive.\"/></VBox>"
		"<Border Class=\"card\" Padding=\"16\"><VBox Spacing=\"12\">"
		"<WrapBox Spacing=\"8\">");
	te_printf(x, "<Button Name=\"%sbutton\" Text=\"Primary\" Style=\"Primary\" Tooltip=\"A tooltip, in the colors of the theme\"/>", prefix);
	te_add(x,
		"<Button Text=\"Default\"/><Button Text=\"Flat\" Style=\"Flat\"/>"
		"<Button Text=\"Delete\" Style=\"Primary\" Class=\"danger\" Tooltip=\"Class=&quot;danger&quot;\"/>"
		"<Button Text=\"Disabled\" Enabled=\"false\"/></WrapBox>"
		"<WrapBox Spacing=\"14\">"
		"<Checkbox Text=\"Checked\" Checked=\"true\"/><Checkbox Text=\"Not checked\"/>"
		"<Checkbox Text=\"Switch\" Style=\"Switch\" Checked=\"true\"/><Checkbox Text=\"Off\" Style=\"Switch\"/>"
		"<Checkbox Text=\"Radio\" Style=\"Radio\" Checked=\"true\"/><Checkbox Text=\"Radio\" Style=\"Radio\"/>"
		"<Checkbox Text=\"Disabled\" Checked=\"true\" Enabled=\"false\"/></WrapBox>"
		"<Grid Columns=\"auto,1\" Spacing=\"10\">"
		"<Label Text=\"Volume\" Slot.Column=\"0\" Slot.Row=\"0\" Slot.VAlign=\"Center\"/><Slider Value=\"0.62\" Slot.Column=\"1\" Slot.Row=\"0\"/>"
		"<Label Text=\"Upload\" Slot.Column=\"0\" Slot.Row=\"1\" Slot.VAlign=\"Center\"/><ProgressBar Value=\"0.42\" Slot.Column=\"1\" Slot.Row=\"1\" Slot.VAlign=\"Center\"/>"
		"</Grid>"
		"<HBox Spacing=\"8\"><TextInput Placeholder=\"Search...\" Slot.Fill=\"1\"/><TextInput Text=\"Typed text\" Slot.Fill=\"1\"/>"
		"<ComboBox Items=\"Monthly|Weekly|Daily\" Selected=\"0\" Width=\"120\"/><SpinBox Min=\"0\" Max=\"100\" Value=\"42\" Width=\"90\"/></HBox>"
		"<HBox Spacing=\"10\"><ColorPicker Popup=\"true\" Color=\"#FF9F0A\"/>"
		"<Label Markdown=\"true\" Wrap=\"true\" Slot.Fill=\"1\" Slot.VAlign=\"Center\">Markdown: **strong**, *emphasis*, `code` and [a link](https://example.org).</Label></HBox>"
		"</VBox></Border>"
		"<HBox Spacing=\"14\">"
		"<Border Class=\"card\" Padding=\"16\" Slot.Fill=\"1\"><VBox Spacing=\"8\"><Label Text=\"Notifications\" TextSize=\"15\"/>"
		"<Checkbox Text=\"Messages\" Style=\"Switch\" Checked=\"true\"/><Checkbox Text=\"Mentions\" Style=\"Switch\" Checked=\"true\"/>"
		"<Checkbox Text=\"Newsletter\" Style=\"Switch\"/></VBox></Border>"
		"<Border Class=\"card\" Padding=\"16\" Slot.Fill=\"1\"><VBox Spacing=\"8\"><Label Text=\"Storage\" TextSize=\"15\"/>"
		"<ProgressBar Value=\"0.78\"/><Label Class=\"dim\" Opacity=\"0.7\" Text=\"78 GB of 100 GB used\"/>"
		"<Button Text=\"Manage...\" Style=\"Flat\" Slot.HAlign=\"Left\"/></VBox></Border>"
		"</HBox>"
		"</VBox></ScrollBox>");

	/* an application */
	te_add(x,
		"<HBox Slot.Title=\"Application\">"
		"<VBox Width=\"210\" Spacing=\"6\" Slot.Padding=\"12\">"
		"<Label Text=\"MAILBOXES\" TextSize=\"11\" Opacity=\"0.55\"/>"
		"<ListView Items=\"Inbox|Starred|Sent|Drafts|Archive|Spam|Trash\" Selected=\"0\" Slot.Fill=\"1\"/>"
		"<Button Text=\"New message\" Style=\"Primary\"/></VBox>"
		"<Separator Orientation=\"Vertical\"/>"
		"<VBox Slot.Fill=\"1\" Spacing=\"12\" Slot.Padding=\"16\">"
		"<HBox Spacing=\"8\"><Label Text=\"Inbox\" TextSize=\"20\" Slot.Fill=\"1\" Slot.VAlign=\"Center\"/>"
		"<TextInput Placeholder=\"Search mail\" Width=\"200\"/><Button Text=\"Filter\"/></HBox>"
		"<WrapBox Spacing=\"10\">"
		"<Border Class=\"card\" Padding=\"14\" Width=\"150\"><VBox Spacing=\"2\"><Label Class=\"dim\" Opacity=\"0.7\" Text=\"Unread\"/><Label Text=\"128\" TextSize=\"24\"/></VBox></Border>"
		"<Border Class=\"card\" Padding=\"14\" Width=\"150\"><VBox Spacing=\"2\"><Label Class=\"dim\" Opacity=\"0.7\" Text=\"Today\"/><Label Text=\"36\" TextSize=\"24\"/></VBox></Border>"
		"<Border Class=\"card\" Padding=\"14\" Width=\"150\"><VBox Spacing=\"2\"><Label Class=\"dim\" Opacity=\"0.7\" Text=\"Flagged\"/><Label Text=\"7\" TextSize=\"24\"/></VBox></Border>"
		"</WrapBox>"
		"<TreeView Slot.Fill=\"1\">"
		"<Item Text=\"Projects\" Expanded=\"true\"><Item Text=\"Raven 1.0 release notes\"/><Item Text=\"Theme editor feedback\"/><Item Text=\"Wayland testing\"/></Item>"
		"<Item Text=\"Team\" Expanded=\"true\"><Item Text=\"Planning for next week\"/><Item Text=\"Lunch on Friday?\"/></Item>"
		"<Item Text=\"Receipts\"><Item Text=\"October\"/></Item></TreeView>"
		"<HBox Spacing=\"8\"><Label Class=\"dim\" Opacity=\"0.6\" Text=\"Synced a minute ago\" Slot.Fill=\"1\" Slot.VAlign=\"Center\"/>"
		"<Button Text=\"Archive\" Style=\"Flat\"/><Button Text=\"Reply\" Style=\"Primary\"/></HBox>"
		"</VBox></HBox>");

	/* code */
	te_add(x,
		"<Splitter Slot.Title=\"Code\" Orientation=\"Vertical\">"
		"<TextEditor Language=\"C\" LineNumbers=\"true\" Slot.Fill=\"3\" Text=\""
		"#include &lt;Raven.h&gt;&#10;&#10;"
		"/* Opens a window with a button that counts. */&#10;"
		"static int g_clicks = 0;&#10;&#10;"
		"static void on_click(Rid button, void* data) {&#10;"
		"\tchar text[64];&#10;"
		"\tsnprintf(text, sizeof(text), &quot;Clicked %d times&quot;, ++g_clicks);&#10;"
		"\tRbutton_set_text(button, text);&#10;"
		"}&#10;&#10;"
		"int main(void) {&#10;"
		"\tif (!Rinit()) return 1;&#10;"
		"\tRid window = Rwindow_create(&quot;Hello&quot;, 640, 480, 0);&#10;"
		"\tRid button = Rbutton_create(&quot;Click me&quot;);&#10;"
		"\tRbutton_set_clicked_callback(button, on_click, NULL);&#10;"
		"\tRwindow_set_content(window, button);&#10;"
		"\twhile (Rwait_frame(-1.0)) {}&#10;"
		"\treturn 0;&#10;"
		"}&#10;\"/>"
		"<TextEditor Markdown=\"true\" Wrap=\"true\" Slot.Fill=\"2\" Text=\""
		"# Release notes&#10;&#10;"
		"A **theme** is a file of colors and sizes laid over a *look*.&#10;&#10;"
		"- Edit it on the left&#10;- See it here, live&#10;- Save it as `my-theme.xml`&#10;&#10;"
		"Read [THEMES.md](https://example.org) for everything a theme can do.&#10;\"/>"
		"</Splitter>");

	/* lists and data */
	te_add(x,
		"<HBox Slot.Title=\"Lists\" Spacing=\"14\" Slot.Padding=\"16\">"
		"<VBox Slot.Fill=\"1\" Spacing=\"8\"><Label Text=\"Files\" TextSize=\"15\"/>"
		"<TreeView Slot.Fill=\"1\">"
		"<Item Text=\"src\" Expanded=\"true\"><Item Text=\"core\" Expanded=\"true\"><Item Text=\"context.c\"/><Item Text=\"memory.c\"/></Item>"
		"<Item Text=\"widgets\"><Item Text=\"button.c\"/><Item Text=\"slider.c\"/></Item><Item Text=\"main.c\"/></Item>"
		"<Item Text=\"docs\" Expanded=\"true\"><Item Text=\"THEMES.md\"/><Item Text=\"XML.md\"/></Item>"
		"<Item Text=\"README.md\"/></TreeView></VBox>"
		"<VBox Slot.Fill=\"1\" Spacing=\"8\"><Label Text=\"People\" TextSize=\"15\"/>"
		"<ListView Selected=\"2\" Slot.Fill=\"1\" Items=\"Ada Lovelace|Alan Turing|Grace Hopper|Margaret Hamilton|Dennis Ritchie|Ken Thompson|Barbara Liskov|Donald Knuth|Edsger Dijkstra|Frances Allen|John Backus|Niklaus Wirth\"/>"
		"<Expander Title=\"Details\" Expanded=\"true\"><VBox Spacing=\"6\" Slot.Padding=\"8\">"
		"<Label Text=\"Grace Hopper\" TextSize=\"15\"/><Label Class=\"dim\" Opacity=\"0.7\" Wrap=\"true\" Text=\"Wrote the first compiler, and found the first bug.\"/>"
		"</VBox></Expander></VBox>"
		"</HBox>");

	/* a dialog */
	te_add(x,
		"<Overlay Slot.Title=\"Dialog\">"
		"<VBox Spacing=\"10\" Slot.Padding=\"24\" Opacity=\"0.35\">"
		"<Label Text=\"Documents\" TextSize=\"20\"/><ListView Items=\"Budget.xlsx|Notes.md|Plan.pdf\" Height=\"120\"/></VBox>"
		"<Border Class=\"card dialog\" Padding=\"22\" Width=\"420\" Slot.HAlign=\"Center\" Slot.VAlign=\"Center\" Shadow=\"24,0,8\">"
		"<VBox Spacing=\"12\">"
		"<Label Text=\"Delete &quot;Plan.pdf&quot;?\" TextSize=\"18\"/>"
		"<Label Wrap=\"true\" Opacity=\"0.75\" Text=\"The file goes to the trash. You can bring it back from there for 30 days.\"/>"
		"<Checkbox Text=\"Do not ask again\"/>"
		"<HBox Spacing=\"8\"><Spacer Slot.Fill=\"1\"/><Button Text=\"Cancel\"/><Button Text=\"Delete\" Style=\"Primary\" Class=\"danger\"/></HBox>"
		"</VBox></Border></Overlay>");

	te_add(x, "</TabView>");
}

static void add_color_row(te_text* x, const char* id, int index, const char* name, const char* help, const char* group) {
	(void)group;
	te_printf(x, "<HBox Name=\"%s.row.%d\" Spacing=\"10\" Slot.Padding=\"0,2\">", id, index);
	te_printf(x, "<ColorPicker Name=\"%s.pick.%d\" Popup=\"true\" Width=\"46\" Slot.VAlign=\"Center\"/>", id, index);
	te_add(x, "<VBox Slot.Fill=\"1\" Slot.VAlign=\"Center\" Spacing=\"0\"><Label Text=\"");
	te_add_escaped(x, name);
	te_add(x, "\"/><Label TextSize=\"11\" Opacity=\"0.55\" Text=\"");
	te_add_escaped(x, help);
	te_add(x, "\"/></VBox>");
	te_printf(x, "<Label Name=\"%s.hex.%d\" Font=\"monospace\" TextSize=\"11\" Opacity=\"0.8\" Slot.VAlign=\"Center\"/>", id, index);
	te_printf(x, "<Button Name=\"%s.reset.%d\" Text=\"\xC3\x97\" Style=\"Flat\" Width=\"26\" Slot.VAlign=\"Center\" Tooltip=\"Back to the value of the look\"/>", id, index);
	te_add(x, "</HBox>");
}

static void build_content(te_text* x);

#ifndef TE_EMBEDDED
static void build_interface(te_text* x) {
	int i;

	te_add(x,
		"<Window Title=\"Raven Theme Editor\" Width=\"1460\" Height=\"900\" MinWidth=\"1040\" MinHeight=\"640\" CustomHeader=\"true\">"
		"<VBox>"
		"<HeaderBar Name=\"header\" Title=\"Raven Theme Editor\">"
		"<MenuBar>"
		"<Menu Text=\"File\"><Menu Text=\"New from a look\">");
	for (i = 1; i < g_te_look_count; i++) {
		te_printf(x, "<MenuItem Text=\"%s\" OnClicked=\"te.new.%d\"/>", g_te_looks[i].label, i);
	}
	te_add(x,
		"</Menu>"
		"<MenuItem Text=\"Open...\" Shortcut=\"Ctrl+O\" OnClicked=\"te.open\"/>"
		"<Separator/>"
		"<MenuItem Text=\"Save\" Shortcut=\"Ctrl+S\" OnClicked=\"te.save\"/>"
		"<MenuItem Text=\"Save as...\" Shortcut=\"Ctrl+Shift+S\" OnClicked=\"te.save_as\"/>"
		"<Separator/>"
		"<MenuItem Text=\"Quit\" Shortcut=\"Ctrl+Q\" OnClicked=\"te.quit\"/></Menu>"
		"<Menu Text=\"Edit\">"
		"<MenuItem Text=\"Undo\" Shortcut=\"Ctrl+Z\" OnClicked=\"te.undo\"/>"
		"<MenuItem Text=\"Redo\" Shortcut=\"Ctrl+Y\" OnClicked=\"te.redo\"/>"
		"<Separator/>"
		"<MenuItem Text=\"Copy the theme file\" Shortcut=\"Ctrl+Shift+C\" OnClicked=\"te.copy\"/>"
		"<MenuItem Text=\"Copy as C code\" OnClicked=\"te.copy.c\" Tooltip=\"The theme in a C string, loaded with Rtheme_load_from_memory: no file to ship\"/>"
		"<MenuItem Text=\"Generate the palette from the accent\" Shortcut=\"Ctrl+G\" OnClicked=\"te.generate\"/>"
		"<MenuItem Text=\"Surprise me\" Shortcut=\"Ctrl+R\" OnClicked=\"te.surprise\"/>"
		"<Separator/>"
		"<MenuItem Text=\"Reset every color\" OnClicked=\"te.reset.colors\"/>"
		"<MenuItem Text=\"Reset every size\" OnClicked=\"te.reset.metrics\"/>"
		"<MenuItem Text=\"Reset the syntax colors\" OnClicked=\"te.reset.syntax\"/></Menu>"
		"<Menu Text=\"View\">"
		"<MenuItem Text=\"Preview in a window\" Shortcut=\"F7\" OnClicked=\"te.window\"/>"
		"<Separator/>"
		"<MenuItem Text=\"Editor in dark\" OnClicked=\"te.ui.dark\"/>"
		"<MenuItem Text=\"Editor in light\" OnClicked=\"te.ui.light\"/></Menu>"
		"</MenuBar>"
		"<HBox Spacing=\"6\" Slot.HAlign=\"Right\" Slot.VAlign=\"Center\" Slot.Padding=\"8,0\">"
		"<Button Name=\"undo\" Text=\"Undo\" Style=\"Flat\" OnClicked=\"te.undo\" Tooltip=\"Ctrl+Z\" Metric.ControlHeight=\"26\"/>"
		"<Button Name=\"redo\" Text=\"Redo\" Style=\"Flat\" OnClicked=\"te.redo\" Tooltip=\"Ctrl+Y\" Metric.ControlHeight=\"26\"/>"
		"<Button Text=\"Open in a window\" OnClicked=\"te.window\" Tooltip=\"The preview in a window of its own, with the wallpaper of the theme (F7)\" Metric.ControlHeight=\"26\"/>"
		"<Button Text=\"Save\" Style=\"Primary\" OnClicked=\"te.save\" Tooltip=\"Ctrl+S\" Metric.ControlHeight=\"26\"/>"
		"</HBox></HeaderBar>");
	build_content(x);
	te_add(x, "</VBox></Window>");
}
#endif

/* What the editor is made of, in its window or in the Theme tab of the designer. */
static void build_content(te_text* x) {
	int i;
	const char* group = "";
	static const char* const darks[] = { "As the look says", "Dark", "Light" };
	static const char* const materials[] = { "From the look", "Solid", "Glass", "Frosted glass", "Tinted glass" };
	static const char* const motions[] = { "From the look", "Springy", "Smooth" };
	static const char* const wallpapers[] = { "From the look", "None", "Dark", "Light", "Gradient of colors", "Image" };

	te_add(x, "<VBox Slot.Fill=\"1\">");
	if (g.embedded) {
		/* (no header bar of its own: what it holds comes here) */
		te_add(x, "<HBox Spacing=\"6\" Slot.Padding=\"10,6\">"
			"<Label Name=\"embedded.title\" TextSize=\"15\" Slot.Fill=\"1\" Slot.VAlign=\"Center\"/>"
			"<Button Name=\"undo\" Text=\"Undo\" Style=\"Flat\" OnClicked=\"te.undo\" Tooltip=\"Ctrl+Z\"/>"
			"<Button Name=\"redo\" Text=\"Redo\" Style=\"Flat\" OnClicked=\"te.redo\" Tooltip=\"Ctrl+Y\"/>"
			"<Button Text=\"Generate the palette\" Style=\"Flat\" OnClicked=\"te.generate\" Tooltip=\"Every color from the accent\"/>"
			"<Button Text=\"Copy as C\" Style=\"Flat\" OnClicked=\"te.copy.c\"/>"
			"<Button Text=\"Open in a window\" OnClicked=\"te.window\" Tooltip=\"The preview in a window of its own, with the wallpaper of the theme\"/>"
			"<Button Text=\"Save\" Style=\"Primary\" OnClicked=\"te.save\" Tooltip=\"Ctrl+S\"/>"
			"</HBox><Separator/>");
	}
	te_add(x,
		"<Splitter Slot.Fill=\"1\">"
		"<TabView Name=\"tabs\" Slot.Fill=\"0.62\">");

	/* ---- the theme ---- */
	te_add(x, "<ScrollBox Slot.Title=\"Theme\"><VBox Spacing=\"8\" Slot.Padding=\"16\">");
	add_eyebrow(x, "THEME");
	te_add(x, "<Grid Columns=\"auto,1\" Spacing=\"8\">"
		"<Label Text=\"Name\" Slot.Column=\"0\" Slot.Row=\"0\" Slot.VAlign=\"Center\"/>"
		"<TextInput Name=\"name\" Placeholder=\"The Name of the theme\" Slot.Column=\"1\" Slot.Row=\"0\"/>"
		"<Label Text=\"Look\" Slot.Column=\"0\" Slot.Row=\"1\" Slot.VAlign=\"Center\"/>"
		"<ComboBox Name=\"look\" Slot.Column=\"1\" Slot.Row=\"1\">");
	for (i = 0; i < g_te_look_count; i++) {
		te_printf(x, "<Item Text=\"%s\"/>", g_te_looks[i].label);
	}
	te_add(x, "</ComboBox>"
		"<Label Text=\"Dark\" Slot.Column=\"0\" Slot.Row=\"2\" Slot.VAlign=\"Center\"/>"
		"<ComboBox Name=\"dark\" Slot.Column=\"1\" Slot.Row=\"2\">");
	add_items(x, darks, 3);
	te_add(x, "</ComboBox>"
		"<Label Text=\"Material\" Slot.Column=\"0\" Slot.Row=\"3\" Slot.VAlign=\"Center\"/>"
		"<ComboBox Name=\"material\" Slot.Column=\"1\" Slot.Row=\"3\" Tooltip=\"What surfaces are made of. Only the looks of Raven are made of glass.\">");
	add_items(x, materials, 5);
	te_add(x, "</ComboBox>"
		"<Label Text=\"Motion\" Slot.Column=\"0\" Slot.Row=\"4\" Slot.VAlign=\"Center\"/>"
		"<ComboBox Name=\"motion\" Slot.Column=\"1\" Slot.Row=\"4\" Tooltip=\"Springy: animations that bounce a little. Smooth: without bounce.\">");
	add_items(x, motions, 3);
	te_add(x, "</ComboBox></Grid>"
		"<Label Name=\"look.hint\" Wrap=\"true\" Opacity=\"0.6\" TextSize=\"12\"/>");

	add_eyebrow(x, "ACCENT");
	te_add(x, "<Checkbox Name=\"accent.own\" Text=\"An accent of its own\" Tooltip=\"Off: the accent of the look (on Windows and macOS, the one the user chose)\"/>"
		"<ColorPicker Name=\"accent\" Alpha=\"false\" Numbers=\"HSV\" "
		"Swatches=\"#0A84FF, #5E5CE6, #BF5AF2, #FF375F, #FF453A, #FF9F0A, #FFD60A, #30D158, #64D2FF, #22D3EE, #8E8E93\"/>");

	add_eyebrow(x, "PALETTE FROM THE ACCENT");
	add_hint(x, "Works out the 36 colors and the syntax colors from the accent: surfaces tinted with its hue, readable text, matching states and status colors. Fine-tune them in Colors afterwards.");
	te_add(x, "<Grid Columns=\"auto,1,auto\" Spacing=\"8\">"
		"<Label Text=\"Tint\" Slot.Column=\"0\" Slot.Row=\"0\" Slot.VAlign=\"Center\"/>"
		"<Slider Name=\"tint\" Min=\"0\" Max=\"1\" Value=\"0.35\" Slot.Column=\"1\" Slot.Row=\"0\" Tooltip=\"How much the surfaces take the hue of the accent\"/>"
		"<Label Name=\"tint.value\" Font=\"monospace\" TextSize=\"11\" Slot.Column=\"2\" Slot.Row=\"0\" Slot.VAlign=\"Center\"/>"
		"<Label Text=\"Depth\" Slot.Column=\"0\" Slot.Row=\"1\" Slot.VAlign=\"Center\"/>"
		"<Slider Name=\"contrast\" Min=\"0.4\" Max=\"1.6\" Value=\"1\" Slot.Column=\"1\" Slot.Row=\"1\" Tooltip=\"How far apart the surfaces are\"/>"
		"<Label Name=\"contrast.value\" Font=\"monospace\" TextSize=\"11\" Slot.Column=\"2\" Slot.Row=\"1\" Slot.VAlign=\"Center\"/>"
		"</Grid>"
		"<HBox Spacing=\"8\"><Button Text=\"Generate the palette\" Style=\"Primary\" OnClicked=\"te.generate\" Tooltip=\"Ctrl+G\"/>"
		"<Button Text=\"Surprise me\" OnClicked=\"te.surprise\" Tooltip=\"Another accent, and its palette (Ctrl+R)\"/></HBox>");

	add_eyebrow(x, "FONT AND WALLPAPER");
	te_add(x, "<HBox Spacing=\"6\"><TextInput Name=\"font\" Placeholder=\"The font of the look (or a .ttf file)\" Slot.Fill=\"1\"/>"
		"<Button Text=\"Browse...\" OnClicked=\"te.font.browse\"/></HBox>"
		"<ComboBox Name=\"wallpaper\" Tooltip=\"What glass looks show behind the content of windows\">");
	add_items(x, wallpapers, 6);
	te_add(x, "</ComboBox>"
		"<HBox Spacing=\"6\"><TextInput Name=\"wallpaper.text\" Slot.Fill=\"1\"/>"
		"<Button Name=\"wallpaper.browse\" Text=\"Browse...\" OnClicked=\"te.wallpaper.browse\"/></HBox>"
		"</VBox></ScrollBox>");

	/* ---- colors ---- */
	te_add(x, "<VBox Slot.Title=\"Colors\" Spacing=\"8\" Slot.Padding=\"12\">"
		"<HBox Spacing=\"8\"><TextInput Name=\"filter\" Placeholder=\"Filter the 36 colors\" Slot.Fill=\"1\"/>"
		"<Button Text=\"Reset all\" Style=\"Flat\" OnClicked=\"te.reset.colors\"/></HBox>"
		"<ScrollBox Slot.Fill=\"1\"><VBox Spacing=\"2\" Slot.Padding=\"0,0,8,0\">");
	for (i = 0; i < TE_COLOR_COUNT; i++) {
		if (strcmp(group, g_te_colors[i].group) != 0) {
			group = g_te_colors[i].group;
			te_printf(x, "<Label Name=\"color.group.%d\" Text=\"%s\" TextSize=\"11\" Opacity=\"0.55\" Slot.Padding=\"0,12,0,4\"/>", g.color_group_count++, group);
		}
		add_color_row(x, "color", i, g_te_colors[i].name, g_te_colors[i].help, group);
	}
	te_add(x, "</VBox></ScrollBox></VBox>");

	/* ---- sizes ---- */
	te_add(x, "<ScrollBox Slot.Title=\"Sizes\"><VBox Spacing=\"4\" Slot.Padding=\"14\">");
	group = "";
	for (i = 0; i < TE_METRIC_COUNT; i++) {
		const te_metric_info* m = &g_te_metrics[i];
		if (strcmp(group, m->group) != 0) {
			group = m->group;
			add_eyebrow(x, group);
			if (strcmp(group, "Glass") == 0) add_hint(x, "Multipliers of what the material gives (1 = unchanged), for the glass looks of Raven and Liquid Glass.");
		}
		te_printf(x, "<HBox Spacing=\"8\" Tooltip=\"%s\">", m->help);
		te_printf(x, "<Label Name=\"metric.state.%d\" Text=\"%s\" Width=\"118\" Slot.VAlign=\"Center\"/>", i, m->label);
		te_printf(x, "<Slider Name=\"metric.slider.%d\" Min=\"%g\" Max=\"%g\" Step=\"%g\" Slot.Fill=\"1\" Slot.VAlign=\"Center\"/>", i, m->minimum, m->maximum, m->step);
		te_printf(x, "<SpinBox Name=\"metric.spin.%d\" Min=\"%g\" Max=\"%g\" Step=\"%g\" Decimals=\"%d\" Width=\"84\"/>", i, m->minimum, m->maximum, m->step, m->decimals);
		te_printf(x, "<Button Name=\"metric.reset.%d\" Text=\"\xC3\x97\" Style=\"Flat\" Width=\"26\" Tooltip=\"Back to the value of the look\"/>", i);
		te_add(x, "</HBox>");
	}
	te_add(x, "<Button Text=\"Reset every size\" Style=\"Flat\" OnClicked=\"te.reset.metrics\" Slot.HAlign=\"Left\" Slot.Padding=\"0,10,0,0\"/>"
		"</VBox></ScrollBox>");

	/* ---- syntax ---- */
	te_add(x, "<VBox Slot.Title=\"Code\" Spacing=\"8\" Slot.Padding=\"12\">");
	add_hint(x, "The colors of code and Markdown in text editors. Without a color, Raven picks one that suits a dark or a light theme.");
	te_add(x, "<ScrollBox Slot.Fill=\"3\"><VBox Spacing=\"2\" Slot.Padding=\"0,0,8,0\">");
	for (i = 0; i < TE_SYNTAX_COUNT; i++) add_color_row(x, "syntax", i, g_te_syntax[i].name, g_te_syntax[i].sample, "");
	te_add(x, "<Button Text=\"Reset the syntax colors\" Style=\"Flat\" OnClicked=\"te.reset.syntax\" Slot.HAlign=\"Left\" Slot.Padding=\"0,8,0,0\"/>"
		"</VBox></ScrollBox>"
		"<TextEditor Theme=\"" PREVIEW_THEME "\" Language=\"XML\" LineNumbers=\"true\" Slot.Fill=\"2\" Text=\""
		"&lt;!-- XML, in the colors above --&gt;&#10;"
		"&lt;Window Title=&quot;Hello&quot; Width=&quot;640&quot;&gt;&#10;"
		"\t&lt;Button Text=&quot;Click me&quot; Style=&quot;Primary&quot;/&gt;&#10;"
		"&lt;/Window&gt;&#10;\"/>"
		"</VBox>");

	/* ---- styles ---- */
	te_add(x, "<VBox Slot.Title=\"Styles\" Spacing=\"8\" Slot.Padding=\"12\">");
	add_hint(x, "Rules for some widgets, written as in a theme file: <Style Widget=\"Button\" Class=\"danger\" Accent=\"#E5484D\"/>. A Part and a State reach the pieces of a widget (Thumb, Track, Box, Mark, Tab...) in a state (Hovered, Pressed, Checked, Selected, Disabled...). The preview follows as you type.");
	te_add(x, "<ComboBox Name=\"snippets\">");
	for (i = 0; i < SNIPPET_COUNT; i++) {
		te_add(x, "<Item Text=\"");
		te_add_escaped(x, g_snippets[i].label);
		te_add(x, "\"/>");
	}
	te_add(x, "</ComboBox>"
		"<TextEditor Name=\"styles\" Language=\"XML\" LineNumbers=\"true\" Slot.Fill=\"1\" Placeholder=\"&lt;Style Widget=&quot;Button&quot; CornerRadius=&quot;12&quot;/&gt;\"/>"
		"<Label Name=\"styles.error\" Wrap=\"true\" TextSize=\"12\"/>"
		"</VBox>");

	/* ---- check ---- */
	te_add(x, "<VBox Slot.Title=\"Check\" Spacing=\"8\" Slot.Padding=\"14\">"
		"<Label Name=\"check.summary\" TextSize=\"16\"/>");
	add_hint(x, "Contrast of the pairs of colors people read, as WCAG 2 measures it: 4.5:1 for text (7:1 for AAA), 3:1 for large text and the parts of controls. Fix changes the color in front as little as it takes.");
	te_add(x, "<Button Text=\"Fix every pair\" OnClicked=\"te.fix.all\" Slot.HAlign=\"Left\"/>");
	te_add(x, "<ScrollBox Slot.Fill=\"1\"><VBox Spacing=\"6\" Slot.Padding=\"0,4,8,0\">");
	for (i = 0; i < PAIR_COUNT; i++) {
		te_printf(x, "<HBox Spacing=\"10\"><Border Name=\"check.swatch.%d\" Padding=\"12,4\" CornerRadius=\"6\" Width=\"58\">"
			"<Label Name=\"check.sample.%d\" Text=\"Aa\" TextSize=\"15\" TextAlign=\"Center\"/></Border>", i, i);
		te_printf(x, "<Label Text=\"%s\" Slot.Fill=\"1\" Slot.VAlign=\"Center\"/>", g_pairs[i].label);
		te_printf(x, "<Label Name=\"check.ratio.%d\" Font=\"monospace\" TextSize=\"12\" Slot.VAlign=\"Center\"/>", i);
		te_printf(x, "<Label Name=\"check.badge.%d\" Width=\"44\" TextAlign=\"Right\" Slot.VAlign=\"Center\"/>", i);
		te_printf(x, "<Button Name=\"check.fix.%d\" Text=\"Fix\" Style=\"Flat\" Width=\"44\" Slot.VAlign=\"Center\" "
			"Tooltip=\"Changes the lightness of %s as little as it takes\"/></HBox>", i, "the color in front");
	}
	te_add(x, "</VBox></ScrollBox></VBox>");

	/* ---- XML ---- */
	te_add(x, "<VBox Slot.Title=\"XML\" Spacing=\"8\" Slot.Padding=\"12\">"
		"<HBox Spacing=\"8\"><Button Text=\"Copy\" OnClicked=\"te.copy\" Tooltip=\"Ctrl+Shift+C\"/>"
		"<Button Text=\"Copy as C\" OnClicked=\"te.copy.c\" Tooltip=\"The theme in a C string, loaded with Rtheme_load_from_memory\"/>"
		"<Button Name=\"xml.apply\" Text=\"Apply my edits\" Style=\"Primary\" OnClicked=\"te.xml.apply\" Enabled=\"false\"/>"
		"<Button Name=\"xml.revert\" Text=\"Revert\" Style=\"Flat\" OnClicked=\"te.xml.revert\" Enabled=\"false\"/></HBox>");
	add_hint(x, "The theme file, as it will be saved. Edit it here too, then apply.");
	te_add(x, "<TextEditor Name=\"xml\" Language=\"XML\" LineNumbers=\"true\" Slot.Fill=\"1\"/></VBox>");

	te_add(x, "</TabView>");

	/* ---- the preview ---- */
	te_add(x, "<VBox Slot.Fill=\"1\">"
		"<HBox Spacing=\"8\" Slot.Padding=\"12,8\">"
		"<Label Text=\"PREVIEW\" TextSize=\"11\" Opacity=\"0.55\" Slot.VAlign=\"Center\" Slot.Fill=\"1\"/>"
		"<Checkbox Name=\"compare\" Text=\"Saved version\" Style=\"Switch\" Tooltip=\"On: the preview shows the theme as it was last opened or saved, to compare\"/>"
		"<Checkbox Name=\"preview.enabled\" Text=\"Enabled\" Style=\"Switch\" Checked=\"true\" Tooltip=\"Off: every widget of the preview disabled\"/>"
		"<Label Text=\"Behind\" Opacity=\"0.7\" Slot.VAlign=\"Center\"/>"
		"<ComboBox Name=\"backdrop\" Width=\"170\" Tooltip=\"What the preview is drawn over (glass shows it through)\">");
	add_items(x, g_backdrops, BACKDROP_COUNT);
	te_add(x, "</ComboBox></HBox>"
		"<Overlay Slot.Fill=\"1\">"
		"<Image Name=\"preview.backdrop\" Stretch=\"Cover\"/>"
		"<Border Name=\"preview.root\" Theme=\"" PREVIEW_THEME "\">");
	add_preview_pages(x, "preview.");
	te_add(x, "</Border>"
		"<VBox Name=\"probe\" Theme=\"" BASE_THEME "\" Visibility=\"Collapsed\"/>"
		"</Overlay></VBox>"
		"</Splitter>"
		"<Separator/>"
		"<HBox Spacing=\"12\" Slot.Padding=\"12,5\">"
		"<Label Name=\"status\" Slot.Fill=\"1\" Slot.VAlign=\"Center\" TextSize=\"12\"/>"
		"<Label Name=\"file\" Opacity=\"0.55\" TextSize=\"12\" Slot.VAlign=\"Center\"/></HBox>"
		"</VBox>");
}

/* ---------------------------------------------------------------------------
 *  The backdrop of the preview
 * ------------------------------------------------------------------------- */

static int backdrop_kind(void) {
	int kind = Rcombo_box_get_selected(g.backdrop);
	if (kind <= 0) {
		/* glass needs something behind it */
		int glass = (g.model.look > 0 && (g_te_looks[g.model.look].look == R_LOOK_RAVEN_GLASS_DARK || g_te_looks[g.model.look].look == R_LOOK_RAVEN_GLASS_LIGHT ||
			g_te_looks[g.model.look].look == R_LOOK_MACOS_GLASS_DARK || g_te_looks[g.model.look].look == R_LOOK_MACOS_GLASS_LIGHT)) || g.model.material >= 2;
		kind = glass ? 2 : 1;
	}
	return kind;
}

static void update_backdrop(void) {
	enum { BW = 320, BH = 200 };
	float window[4], accent[4], h, s, l;
	char key[64];
	int kind = backdrop_kind(), x, y, dark = model_dark();
	uint8_t* pixels;

	role_color(R_COLOR_WINDOW, window);
	role_color(R_COLOR_ACCENT, accent);
	if (kind == 1) {
		Rwidget_set_visibility(g.preview_backdrop, R_COLLAPSED);
		Rborder_set_background(g.preview_root, window[0], window[1], window[2], 1.0f);
		return;
	}
	Rborder_set_background(g.preview_root, 0, 0, 0, 0);
	Rwidget_set_visibility(g.preview_backdrop, R_VISIBLE);
	snprintf(key, sizeof(key), "%d %d %.3f %.3f %.3f %.3f %.3f %.3f", kind, dark, window[0], window[1], window[2], accent[0], accent[1], accent[2]);
	if (strcmp(key, g.backdrop_key) == 0 && g.backdrop_texture != R_INVALID_ID) return;
	te_copy(g.backdrop_key, sizeof(g.backdrop_key), key);

	pixels = (uint8_t*)malloc((size_t)BW * BH * 4);
	if (!pixels) return;
	te_rgb_to_hsl(accent, &h, &s, &l);
	for (y = 0; y < BH; y++) {
		for (x = 0; x < BW; x++) {
			float c[3], u = (float)x / BW, v = (float)y / BH;
			int k;
			if (kind == 4) {
				float a = ((x / 16 + y / 16) & 1) ? (dark ? 0.22f : 0.86f) : (dark ? 0.16f : 0.96f);
				c[0] = c[1] = c[2] = a;
			} else {
				/* soft blobs of the accent and of two neighbouring hues over the window color */
				static const float blobs[4][4] = { { 0.18f, 0.25f, 0.0f, 0.42f }, { 0.82f, 0.30f, 55.0f, 0.36f }, { 0.55f, 0.92f, -50.0f, 0.40f }, { 0.05f, 0.95f, 160.0f, 0.30f } };
				float vivid = kind == 3 ? 1.0f : 0.55f;
				memcpy(c, window, sizeof(c));
				for (k = 0; k < 4; k++) {
					float dx = u - blobs[k][0], dy = (v - blobs[k][1]) * 0.62f, rgb[3];
					float w = expf(-(dx * dx + dy * dy) / (blobs[k][3] * blobs[k][3] * 0.25f)) * vivid;
					int j;
					te_hsl_to_rgb(h + blobs[k][2], fmaxf(s, 0.5f) * (kind == 3 ? 1.0f : 0.8f), dark ? 0.45f : 0.68f, rgb);
					for (j = 0; j < 3; j++) c[j] = c[j] * (1.0f - w) + rgb[j] * w;
				}
			}
			pixels[(y * BW + x) * 4 + 0] = (uint8_t)(fminf(fmaxf(c[0], 0.0f), 1.0f) * 255.0f);
			pixels[(y * BW + x) * 4 + 1] = (uint8_t)(fminf(fmaxf(c[1], 0.0f), 1.0f) * 255.0f);
			pixels[(y * BW + x) * 4 + 2] = (uint8_t)(fminf(fmaxf(c[2], 0.0f), 1.0f) * 255.0f);
			pixels[(y * BW + x) * 4 + 3] = 255;
		}
	}
	{
		Rid texture = Rtexture_create_from_pixels(pixels, BW, BH);
		if (texture != R_INVALID_ID) {
			Rimage_set_texture(g.preview_backdrop, texture);
			if (g.backdrop_texture != R_INVALID_ID) Rtexture_destroy(g.backdrop_texture);
			g.backdrop_texture = texture;
		}
	}
	free(pixels);
}

/* ---------------------------------------------------------------------------
 *  From the model to the screen
 * ------------------------------------------------------------------------- */

/* What the theme inherits: read again when what it starts from changed. */
static void update_base(void) {
	te_text xml;
	int i;
	memset(&xml, 0, sizeof(xml));
	te_model_write(&g.model, &xml, BASE_THEME, 1, NULL);
	if (!g.base_xml || strcmp(g.base_xml, xml.data) != 0) {
		Rtheme_load_from_memory(xml.data, xml.length);
		free(g.base_xml);
		g.base_xml = xml.data;
		xml.data = NULL;
	}
	te_text_free(&xml);
	if (g.probe == R_INVALID_ID) return;
	for (i = 0; i < TE_COLOR_COUNT; i++) {
		float* c = g.base_colors[i];
		Rwidget_get_color(g.probe, g_te_colors[i].role, &c[0], &c[1], &c[2], &c[3]);
	}
	for (i = 0; i < TE_METRIC_COUNT; i++) g.base_metrics[i] = Rwidget_get_metric(g.probe, g_te_metrics[i].metric);
}

static void update_color_row(Rid picker, Rid hex, Rid reset, const float* color, int set) {
	char text[40], code[16];
	if (picker != g.source) Rcolor_picker_set_color(picker, color[0], color[1], color[2], color[3]);
	te_hex(color, code, sizeof(code));
	if (set) snprintf(text, sizeof(text), "%s", code);
	else snprintf(text, sizeof(text), "look  %s", code);
	Rlabel_set_text(hex, text);
	Rwidget_set_opacity(hex, set ? 0.95f : 0.5f);
	Rwidget_set_visibility(reset, set ? R_VISIBLE : R_HIDDEN);
}

static void update_rows(void) {
	int i;
	g.syncing++;
	for (i = 0; i < TE_COLOR_COUNT; i++) {
		float c[4];
		effective_color(i, c);
		update_color_row(g.color_picker[i], g.color_hex[i], g.color_reset[i], c, g.model.colors[i].set);
	}
	for (i = 0; i < TE_METRIC_COUNT; i++) {
		float value = effective_metric(i);
		if (g.metric_slider[i] != g.source) Rslider_set_value(g.metric_slider[i], value);
		if (g.metric_spin[i] != g.source) Rspin_box_set_value(g.metric_spin[i], value);
		Rwidget_set_opacity(g.metric_state[i], g.model.metric_set[i] ? 1.0f : 0.6f);
		Rwidget_set_visibility(g.metric_reset[i], g.model.metric_set[i] ? R_VISIBLE : R_HIDDEN);
	}
	for (i = 0; i < TE_SYNTAX_COUNT; i++) {
		float c[4];
		effective_syntax(i, c);
		update_color_row(g.syntax_picker[i], g.syntax_hex[i], g.syntax_reset[i], c, g.model.syntax[i].set);
	}
	if (!g.model.accent_set) {
		float a[4];
		role_color(R_COLOR_ACCENT, a);
		Rcolor_picker_set_color(g.accent, a[0], a[1], a[2], 1.0f);
	}
	g.syncing--;
}

/* The background of a pair, opaque (the selection is laid over the panel). */
static void pair_background(int i, float* bg) {
	role_color(g_pairs[i].bg, bg);
	if (g_pairs[i].over_panel) {
		float panel[4], over[4];
		role_color(R_COLOR_PANEL, panel);
		te_blend(bg, panel, over);
		memcpy(bg, over, 4 * sizeof(float));
	}
	bg[3] = 1.0f;
}

static int fix_pair(int i) {
	float fg[4], bg[4];
	int k;
	role_color(g_pairs[i].fg, fg);
	pair_background(i, bg);
	if (te_contrast(fg, bg) >= g_pairs[i].target) return 0;
	te_fix_contrast(fg, bg, g_pairs[i].target);
	for (k = 0; k < TE_COLOR_COUNT; k++) {
		if (g_te_colors[k].role == g_pairs[i].fg) {
			g.model.colors[k].set = 1;
			memcpy(g.model.colors[k].c, fg, sizeof(fg));
		}
	}
	return 1;
}

static void on_fix(Rid button, void* user_data) {
	int i = (int)(intptr_t)user_data;
	(void)button;
	begin_change("fix");
	if (fix_pair(i)) {
		changed();
		set_status("%s: fixed.", g_pairs[i].label);
	}
}

static void update_check(void) {
	float success[4], warning[4], danger[4];
	int i, pass = 0;
	Rtheme_get_color(R_COLOR_SUCCESS, &success[0], &success[1], &success[2], &success[3]);
	Rtheme_get_color(R_COLOR_WARNING, &warning[0], &warning[1], &warning[2], &warning[3]);
	Rtheme_get_color(R_COLOR_DANGER, &danger[0], &danger[1], &danger[2], &danger[3]);
	for (i = 0; i < PAIR_COUNT; i++) {
		float fg[4], bg[4], ratio;
		const float* badge_color;
		const char* badge;
		char text[32];
		role_color(g_pairs[i].fg, fg);
		pair_background(i, bg);
		ratio = te_contrast(fg, bg);
		Rborder_set_background(g.check_swatch[i], bg[0], bg[1], bg[2], 1.0f);
		Rlabel_set_color(g.check_sample[i], fg[0], fg[1], fg[2], fg[3]);
		snprintf(text, sizeof(text), "%.2f : 1", ratio);
		Rlabel_set_text(g.check_ratio[i], text);
		if (ratio >= g_pairs[i].target) {
			badge = g_pairs[i].target >= 4.5f ? (ratio >= 7.0f ? "AAA" : "AA") : "OK";
			badge_color = success;
			pass++;
		} else if (ratio >= g_pairs[i].target * 0.7f) {
			badge = "Low";
			badge_color = warning;
		} else {
			badge = "Fail";
			badge_color = danger;
		}
		Rlabel_set_text(g.check_badge[i], badge);
		Rwidget_set_visibility(g.check_fix[i], ratio >= g_pairs[i].target ? R_HIDDEN : R_VISIBLE);
		Rlabel_set_color(g.check_badge[i], badge_color[0], badge_color[1], badge_color[2], 1.0f);
	}
	{
		char text[96];
		snprintf(text, sizeof(text), pass == PAIR_COUNT ? "Every pair reads well (%d of %d)" : "%d of %d pairs read well", pass, PAIR_COUNT);
		Rlabel_set_text(g.check_summary, text);
	}
}

static void update_title(void) {
	char text[1200];
	int dirty = !te_model_equal(&g.model, &g.saved);
	snprintf(text, sizeof(text), "%s%s - Raven Theme Editor", dirty ? "\xE2\x80\xA2 " : "", g.model.name[0] ? g.model.name : "Untitled");
	if (g.embedded) {
		snprintf(text, sizeof(text), "%s%s", g.model.name[0] ? g.model.name : "Untitled", dirty ? "  \xE2\x80\xA2" : "");
		Rlabel_set_text(W("embedded.title"), text);
	} else {
		Rwindow_set_title(g.window, text);
	}
	Rlabel_set_text(g.file_label, g.path[0] ? g.path : "Not saved yet");
	Rwidget_set_enabled(g.undo_button, g.undo_count > 0);
	Rwidget_set_enabled(g.redo_button, g.redo_count > 0);
}

static void update_look_hint(void) {
	const char* hint;
	const te_look_info* look = &g_te_looks[g.model.look > 0 && g.model.look < g_te_look_count ? g.model.look : 0];
	if (g.model.look <= 0) {
		hint = g.model.base[0] ? "Starts from another theme (Base): the values that are not set come from it."
			: "Laid over the look of the application, whichever it is: it follows when the look changes.";
	} else if (!look->raven) {
		hint = "A look that imitates a platform: its shapes stay; colors, sizes and styles change. Glass materials belong to the looks of Raven.";
	} else {
		hint = "A look of Raven: everything can change, glass included.";
	}
	Rlabel_set_text(g.look_hint, hint);
	Rwidget_set_enabled(g.material, look->raven || g.model.look <= 0);
	Rwidget_set_enabled(g.wallpaper_text, g.model.wallpaper == TE_WALLPAPER_COLORS || g.model.wallpaper == TE_WALLPAPER_IMAGE);
	Rwidget_set_enabled(g.wallpaper_browse, g.model.wallpaper == TE_WALLPAPER_IMAGE);
	Rtext_input_set_placeholder(g.wallpaper_text, g.model.wallpaper == TE_WALLPAPER_IMAGE ? "The path of an image" : "Colors, top to bottom: #1E1B4B, #4C1D95, #0F172A");
}

/* The theme file of the model, loaded for the preview; everything that shows it follows. */
static void refresh(void) {
	te_text xml;
	int failed;

	update_base();

	memset(&xml, 0, sizeof(xml));
	te_model_write(g.compare != R_INVALID_ID && Rcheckbox_is_checked(g.compare) ? &g.saved : &g.model, &xml, PREVIEW_THEME, 0, g.dir[0] ? g.dir : NULL);
	g.errors.length = 0;
	if (g.errors.data) g.errors.data[0] = '\0';
	failed = Rtheme_load_from_memory(xml.data, xml.length) == NULL;
	te_text_free(&xml);
	if (g.errors.length) {
		float danger[4];
		Rtheme_get_color(R_COLOR_DANGER, &danger[0], &danger[1], &danger[2], &danger[3]);
		Rlabel_set_text(g.styles_error, g.errors.data);
		Rlabel_set_color(g.styles_error, danger[0], danger[1], danger[2], 1.0f);
		set_status(failed ? "The theme cannot be read: %.400s" : "%.400s", g.errors.data);
	} else {
		Rlabel_set_text(g.styles_error, "");
	}

	update_rows();
	update_check();
	update_backdrop();
	update_look_hint();

	/* the file, unless it is being edited by hand */
	if (!g.xml_edited) {
		memset(&xml, 0, sizeof(xml));
		te_model_write(&g.model, &xml, NULL, 0, NULL);
		g.syncing++;
		Rtext_editor_set_text(g.xml, xml.data);
		g.syncing--;
		te_text_free(&xml);
	}
	update_title();
}

/* Every control of the editor from the model (after undo, open, new...). */
static void ui_from_model(void) {
	g.syncing++;
	Rtext_input_set_text(g.name, g.model.name);
	Rcombo_box_set_selected(g.look, g.model.look);
	Rcombo_box_set_selected(g.dark, g.model.dark);
	Rcombo_box_set_selected(g.material, g.model.material);
	Rcombo_box_set_selected(g.motion, g.model.motion);
	Rcheckbox_set_checked(g.accent_own, g.model.accent_set);
	if (g.model.accent_set) Rcolor_picker_set_color(g.accent, g.model.accent[0], g.model.accent[1], g.model.accent[2], 1.0f);
	Rtext_input_set_text(g.font, g.model.font);
	Rcombo_box_set_selected(g.wallpaper, g.model.wallpaper);
	Rtext_input_set_text(g.wallpaper_text, g.model.wallpaper_text);
	Rtext_editor_set_text(g.styles, g.model.styles ? g.model.styles : "");
	g.syncing--;
}

/* After a change of the model made by a control of the editor. */
static void changed(void) {
	refresh();
}

static void changed_by(Rid source) {
	g.source = source;
	refresh();
	g.source = R_INVALID_ID;
}

/* ---------------------------------------------------------------------------
 *  The controls
 * ------------------------------------------------------------------------- */

static void on_color(Rid picker, float r, float g_, float b, float a, void* user_data) {
	int i = (int)(intptr_t)user_data;
	char what[32];
	if (g.syncing) return;
	snprintf(what, sizeof(what), "color.%d", i);
	begin_change(what);
	g.model.colors[i].set = 1;
	g.model.colors[i].c[0] = r;
	g.model.colors[i].c[1] = g_;
	g.model.colors[i].c[2] = b;
	g.model.colors[i].c[3] = a;
	changed_by(picker);
}

static void on_color_reset(Rid button, void* user_data) {
	int i = (int)(intptr_t)user_data;
	(void)button;
	begin_change("reset");
	g.model.colors[i].set = 0;
	changed();
	set_status("%s: back to the value of the look.", g_te_colors[i].name);
}

static void on_syntax(Rid picker, float r, float g_, float b, float a, void* user_data) {
	int i = (int)(intptr_t)user_data;
	char what[32];
	if (g.syncing) return;
	snprintf(what, sizeof(what), "syntax.%d", i);
	begin_change(what);
	g.model.syntax[i].set = 1;
	g.model.syntax[i].c[0] = r;
	g.model.syntax[i].c[1] = g_;
	g.model.syntax[i].c[2] = b;
	g.model.syntax[i].c[3] = a;
	changed_by(picker);
}

static void on_syntax_reset(Rid button, void* user_data) {
	int i = (int)(intptr_t)user_data;
	(void)button;
	begin_change("reset");
	g.model.syntax[i].set = 0;
	changed();
}

static void set_metric(int i, float value, Rid source) {
	char what[32];
	snprintf(what, sizeof(what), "metric.%d", i);
	begin_change(what);
	g.model.metric_set[i] = 1;
	g.model.metrics[i] = value;
	changed_by(source);
}

static void on_metric_slider(Rid slider, float value, void* user_data) {
	int i = (int)(intptr_t)user_data;
	float step = g_te_metrics[i].step;
	if (g.syncing) return;
	if (step > 0.0f) value = floorf(value / step + 0.5f) * step;
	set_metric(i, value, slider);
}

static void on_metric_spin(Rid spin, double value, void* user_data) {
	if (g.syncing) return;
	set_metric((int)(intptr_t)user_data, (float)value, spin);
}

static void on_metric_reset(Rid button, void* user_data) {
	int i = (int)(intptr_t)user_data;
	(void)button;
	begin_change("reset");
	g.model.metric_set[i] = 0;
	changed();
}

static void on_name(Rid input, const char* text, void* user_data) {
	(void)input;
	(void)user_data;
	if (g.syncing) return;
	begin_change("name");
	te_copy(g.model.name, sizeof(g.model.name), text);
	changed();
}

static void on_combo(Rid combo, int index, void* user_data) {
	int* field = (int*)user_data;
	if (g.syncing || index < 0) return;
	begin_change("combo");
	*field = index;
	/* a look of a platform has no glass */
	if (combo == g.look && index > 0 && !g_te_looks[index].raven) g.model.material = 0;
	if (combo == g.look || combo == g.wallpaper) {
		g.syncing++;
		Rcombo_box_set_selected(g.material, g.model.material);
		g.syncing--;
	}
	changed();
}

static void on_accent(Rid picker, float r, float g_, float b, float a, void* user_data) {
	(void)picker;
	(void)a;
	(void)user_data;
	if (g.syncing) return;
	begin_change("accent");
	g.model.accent_set = 1;
	g.model.accent[0] = r;
	g.model.accent[1] = g_;
	g.model.accent[2] = b;
	g.syncing++;
	Rcheckbox_set_checked(g.accent_own, 1);
	g.syncing--;
	changed();
}

static void on_accent_own(Rid checkbox, int checked, void* user_data) {
	(void)checkbox;
	(void)user_data;
	if (g.syncing) return;
	begin_change("accent.own");
	g.model.accent_set = checked;
	if (checked) Rcolor_picker_get_color(g.accent, &g.model.accent[0], &g.model.accent[1], &g.model.accent[2], NULL);
	changed();
}

static void on_slider_label(Rid slider, float value, void* user_data) {
	char text[16];
	(void)slider;
	snprintf(text, sizeof(text), "%.2f", value);
	Rlabel_set_text((Rid)(uintptr_t)user_data, text);
}

/* A path that names a file which is there (a file being typed is loaded once it is complete). */
static int is_file(const char* path) {
	FILE* file;
	if (!path[0]) return 1;
	file = fopen(path, "rb");
	if (!file) return 0;
	fclose(file);
	return 1;
}

static void set_path_field(char* field, const char* text) {
	if (strcmp(field, text) == 0) return;
	begin_change(field == g.model.font ? "font" : "wallpaper.text");
	te_copy(field, 512, text);
	changed();
}

/*
 * A font or an image is loaded each time the theme is: not at each key typed,
 * only when the path names a file, or on Enter (the colors of a gradient, at once).
 */
static void on_path(Rid input, const char* text, void* user_data) {
	char* field = (char*)user_data;
	(void)input;
	if (g.syncing) return;
	if (field == g.model.wallpaper_text && g.model.wallpaper == TE_WALLPAPER_COLORS) set_path_field(field, text);
	else if (is_file(text)) set_path_field(field, text);
}

static void on_path_committed(Rid input, const char* text, void* user_data) {
	(void)input;
	if (g.syncing) return;
	set_path_field((char*)user_data, text);
}

static void apply_styles(Rid timer, void* user_data) {
	const char* text = Rtext_editor_get_text(g.styles);
	(void)timer;
	(void)user_data;
	g.styles_timer = R_INVALID_ID;
	if (strcmp(text, g.model.styles ? g.model.styles : "") == 0) return;
	begin_change("styles");
	free(g.model.styles);
	g.model.styles = te_strdup(text);
	changed();
}

static void on_styles(Rid editor, void* user_data) {
	(void)editor;
	(void)user_data;
	if (g.syncing) return;
	/* (a little after the last key: a rule half typed is not an error) */
	if (g.styles_timer != R_INVALID_ID && Rtimer_is_valid(g.styles_timer)) Rtimer_destroy(g.styles_timer);
	g.styles_timer = Rtimer_create(0.35, R_FALSE, apply_styles, NULL);
}

static void add_snippet(int index) {
	te_text text;
	char accent[16];
	const char* rule = g_snippets[index].rule;
	float a[4];
	if (!rule) return;
	role_color(R_COLOR_ACCENT, a);
	te_hex(a, accent, sizeof(accent));
	memset(&text, 0, sizeof(text));
	te_add(&text, g.model.styles ? g.model.styles : "");
	if (text.length && text.data[text.length - 1] != '\n') te_add(&text, "\n");
	while (*rule) {
		if (strncmp(rule, "{accent}", 8) == 0) {
			te_add(&text, accent);
			rule += 8;
		} else {
			te_addn(&text, rule++, 1);
		}
	}
	te_add(&text, "\n");
	begin_change("snippet");
	free(g.model.styles);
	g.model.styles = text.data ? text.data : te_strdup("");
	g.syncing++;
	Rtext_editor_set_text(g.styles, g.model.styles);
	g.syncing--;
	changed();
	set_status("Rule added: %s", g_snippets[index].label);
}

static void on_snippet(Rid combo, int index, void* user_data) {
	(void)user_data;
	if (g.syncing || index <= 0) return;
	add_snippet(index);
	g.syncing++;
	Rcombo_box_set_selected(combo, 0);
	g.syncing--;
}

static void on_xml_edited(Rid editor, void* user_data) {
	(void)editor;
	(void)user_data;
	if (g.syncing) return;
	if (!g.xml_edited) set_status("The file was edited by hand: Apply my edits to use it, Revert to drop it.");
	g.xml_edited = 1;
	Rwidget_set_enabled(g.xml_apply, 1);
	Rwidget_set_enabled(g.xml_revert, 1);
}

static void on_filter(Rid input, const char* text, void* user_data) {
	int i;
	(void)input;
	(void)user_data;
	for (i = 0; i < TE_COLOR_COUNT; i++) {
		int shown = !text[0] || te_contains(g_te_colors[i].name, text) || te_contains(g_te_colors[i].help, text) || te_contains(g_te_colors[i].group, text);
		Rwidget_set_visibility(g.color_row[i], shown ? R_VISIBLE : R_COLLAPSED);
	}
	for (i = 0; i < g.color_group_count; i++) Rwidget_set_visibility(g.color_group[i], text[0] ? R_COLLAPSED : R_VISIBLE);
}

static void on_preview_enabled(Rid checkbox, int checked, void* user_data) {
	(void)checkbox;
	(void)user_data;
	Rwidget_set_enabled(Rwidget_find(g.preview_root, "preview.pages"), checked);
}

static void on_compare(Rid checkbox, int checked, void* user_data) {
	(void)checkbox;
	(void)user_data;
	refresh();
	set_status(checked ? "The preview shows the saved version. Switch it off to see your changes." : "The preview shows your changes.");
}

static void on_backdrop(Rid combo, int index, void* user_data) {
	(void)combo;
	(void)index;
	(void)user_data;
	update_backdrop();
}

/* ---------------------------------------------------------------------------
 *  Actions: files, palette, undo...
 * ------------------------------------------------------------------------- */

static void set_path(const char* path) {
	char copy[sizeof(g.path)];
	char* slash;
	/* (path may be g.path itself: Save on a theme that has a file) */
	te_copy(copy, sizeof(copy), path);
	path = copy;
	te_copy(g.path, sizeof(g.path), path);
	te_copy(g.dir, sizeof(g.dir), path);
	slash = strrchr(g.dir, '/');
	if (!slash || (strrchr(g.dir, '\\') && strrchr(g.dir, '\\') > slash)) slash = strrchr(g.dir, '\\');
	if (slash) *slash = '\0';
	else g.dir[0] = '\0';
}

static void reset_history(void) {
	clear_stack(g.undo, &g.undo_count);
	clear_stack(g.redo, &g.redo_count);
	g.last_change[0] = '\0';
}

static int load_text(const char* xml, size_t size, const char* source) {
	char error[256];
	te_model model;
	te_model_init(&model);
	if (!te_model_read(&model, xml, size, error, sizeof(error))) {
		set_status("%s: %s", source, error);
		te_model_free(&model);
		return 0;
	}
	te_model_copy(&g.model, &model);
	te_model_free(&model);
	return 1;
}

static int open_file(const char* path) {
	size_t size = 0;
	char* text = Rfile_read(path, &size);
	if (!text) {
		set_status("Cannot read %s", path);
		return 0;
	}
	if (!load_text(text, size, path)) {
		Rmemory_free(text);
		return 0;
	}
	Rmemory_free(text);
	set_path(path);
	reset_history();
	te_model_copy(&g.saved, &g.model);
	g.xml_edited = 0;
	ui_from_model();
	refresh();
	set_status("Opened %s", path);
	return 1;
}

static int save_to(const char* path) {
	te_text xml;
	int ok;
	memset(&xml, 0, sizeof(xml));
	te_model_write(&g.model, &xml, NULL, 0, NULL);
	ok = Rfile_write(path, xml.data, xml.length);
	te_text_free(&xml);
	if (!ok) {
		set_status("Cannot write %s", path);
		return 0;
	}
	set_path(path);
	te_model_copy(&g.saved, &g.model);
	update_title();
	set_status("Saved %s", path);
	return 1;
}

static const RFileFilter g_theme_filters[] = { { "Raven themes", "*.xml" }, { "Every file", "*" } };

static int save_as(void) {
	RFileDialogDesc desc;
	char name[128];
	const char* path;
	size_t i;
	te_copy(name, sizeof(name), g.model.name[0] ? g.model.name : "theme");
	for (i = 0; name[i]; i++) {
		if (name[i] == ' ') name[i] = '-';
		else if (name[i] >= 'A' && name[i] <= 'Z') name[i] = (char)(name[i] - 'A' + 'a');
	}
	te_copy(name + strlen(name), sizeof(name) - strlen(name), ".xml");
	memset(&desc, 0, sizeof(desc));
	desc.parent = g.window;
	desc.title = "Save the theme";
	desc.file_name = name;
	desc.directory = g.dir[0] ? g.dir : NULL;
	desc.filters = g_theme_filters;
	desc.filter_count = 2;
	desc.default_extension = "xml";
	path = Rfile_dialog_save(&desc);
	return path ? save_to(path) : 0;
}

static void action_file(Rid sender, const char* action, void* user_data) {
	crash_report_note("theme editor: action %s", action);
	(void)sender;
	(void)user_data;
	if (g.fuzzing && (strcmp(action, "te.open") == 0 || strcmp(action, "te.save_as") == 0 || strcmp(action, "te.quit") == 0 ||
		(strcmp(action, "te.save") == 0 && !g.path[0]))) return;
	if (strcmp(action, "te.open") == 0) {
		RFileDialogDesc desc;
		const char* path;
		memset(&desc, 0, sizeof(desc));
		desc.parent = g.window;
		desc.title = "Open a theme";
		desc.directory = g.dir[0] ? g.dir : NULL;
		desc.filters = g_theme_filters;
		desc.filter_count = 2;
		path = Rfile_dialog_open(&desc);
		if (path) open_file(path);
	} else if (strcmp(action, "te.save") == 0) {
		if (g.path[0]) save_to(g.path);
		else save_as();
	} else if (strcmp(action, "te.save_as") == 0) {
		save_as();
	} else if (strcmp(action, "te.quit") == 0) {
		if (!g.embedded) Rwindow_close(g.window);
	} else if (strncmp(action, "te.new.", 7) == 0) {
		int look = atoi(action + 7);
		if (look <= 0 || look >= g_te_look_count) return;
		begin_change("new");
		te_model_free(&g.model);
		te_model_init(&g.model);
		g.model.look = look;
		g.path[0] = g.dir[0] = '\0';
		g.xml_edited = 0;
		ui_from_model();
		refresh();
		set_status("A new theme over %s: change anything, the preview follows.", g_te_looks[look].label);
	}
}

static void pick_file(char* field, const char* title, const char* filter_name, const char* patterns) {
	RFileDialogDesc desc;
	RFileFilter filters[2];
	const char* path;
	if (g.fuzzing) return;
	filters[0].name = filter_name;
	filters[0].patterns = patterns;
	filters[1].name = "Every file";
	filters[1].patterns = "*";
	memset(&desc, 0, sizeof(desc));
	desc.parent = g.window;
	desc.title = title;
	desc.filters = filters;
	desc.filter_count = 2;
	path = Rfile_dialog_open(&desc);
	if (!path) return;
	begin_change("path");
	te_copy(field, 512, path);
	ui_from_model();
	changed();
}

static void action_edit(Rid sender, const char* action, void* user_data) {
	int i;
	crash_report_note("theme editor: action %s", action);
	(void)sender;
	(void)user_data;
	if (strcmp(action, "te.undo") == 0) {
		undo_redo(0);
	} else if (strcmp(action, "te.redo") == 0) {
		undo_redo(1);
	} else if (strcmp(action, "te.copy") == 0) {
		te_text xml;
		memset(&xml, 0, sizeof(xml));
		te_model_write(&g.model, &xml, NULL, 0, NULL);
		Rclipboard_set_text(xml.data);
		te_text_free(&xml);
		set_status("The theme file is in the clipboard.");
	} else if (strcmp(action, "te.copy.c") == 0) {
		te_text xml, code;
		const char* walk;
		char name[96];
		size_t k;
		memset(&xml, 0, sizeof(xml));
		memset(&code, 0, sizeof(code));
		te_model_write(&g.model, &xml, NULL, 0, NULL);
		/* an identifier from the name: "Night Owl" -> night_owl_theme */
		te_copy(name, sizeof(name), g.model.name[0] ? g.model.name : "my");
		for (k = 0; name[k]; k++) {
			char c = name[k];
			if (c >= 'A' && c <= 'Z') name[k] = (char)(c - 'A' + 'a');
			else if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) name[k] = '_';
		}
		te_printf(&code, "/* The theme \"%s\", made with the Raven theme editor. */\nstatic const char %s%s_theme[] =\n", g.model.name, (name[0] >= '0' && name[0] <= '9') ? "t_" : "", name);
		for (walk = xml.data; *walk;) {
			const char* end = strchr(walk, '\n');
			size_t length = end ? (size_t)(end - walk) : strlen(walk);
			te_add(&code, "\t\"");
			for (k = 0; k < length; k++) {
				if (walk[k] == '"' || walk[k] == '\\') te_add(&code, "\\");
				te_addn(&code, walk + k, 1);
			}
			te_add(&code, "\\n\"\n");
			if (!end) break;
			walk = end + 1;
		}
		te_add(&code, "\t;\n\n/* after Rinit: */\n");
		te_printf(&code, "Rtheme_load_from_memory(%s%s_theme, sizeof(%s%s_theme) - 1);\nRtheme_use(\"%s\");\n",
			(name[0] >= '0' && name[0] <= '9') ? "t_" : "", name, (name[0] >= '0' && name[0] <= '9') ? "t_" : "", name, g.model.name);
		Rclipboard_set_text(code.data);
		te_text_free(&xml);
		te_text_free(&code);
		set_status("C code that loads the theme is in the clipboard.");
	} else if (strcmp(action, "te.fix.all") == 0) {
		int fixed = 0, pass;
		begin_change("fix");
		/* (fixing one color can change another pair: a second pass) */
		for (pass = 0; pass < 2; pass++) {
			for (i = 0; i < PAIR_COUNT; i++) {
				/* the accent is a choice: only what is drawn on it changes */
				if (g_pairs[i].fg == R_COLOR_ACCENT) continue;
				fixed += fix_pair(i);
			}
		}
		changed();
		if (fixed) set_status("Fixed %d pair(s); the accent itself is left as chosen.", fixed);
		else set_status("Nothing to fix.");
	} else if (strcmp(action, "te.generate") == 0 || strcmp(action, "te.surprise") == 0) {
		begin_change("generate");
		if (strcmp(action, "te.surprise") == 0) {
			float h, s, l, rgb[3];
			/* a hue far enough from the one there is */
			te_rgb_to_hsl(g.model.accent_set ? g.model.accent : g.base_colors[10], &h, &s, &l);
			h += 60.0f + (float)(rand() % 240);
			te_hsl_to_rgb(h, 0.62f + (float)(rand() % 30) / 100.0f, 0.52f + (float)(rand() % 14) / 100.0f, rgb);
			g.model.accent_set = 1;
			memcpy(g.model.accent, rgb, sizeof(rgb));
			Rslider_set_value(g.tint, (float)(rand() % 70) / 100.0f);
		}
		te_generate_palette(&g.model, model_dark(), Rslider_get_value(g.tint), Rslider_get_value(g.contrast));
		ui_from_model();
		changed();
		set_status("A palette worked out from the accent %s. Undo (Ctrl+Z) to go back.", "");
	} else if (strcmp(action, "te.reset.colors") == 0) {
		begin_change("reset.all");
		for (i = 0; i < TE_COLOR_COUNT; i++) g.model.colors[i].set = 0;
		changed();
		set_status("Every color is back to the look.");
	} else if (strcmp(action, "te.reset.metrics") == 0) {
		begin_change("reset.all");
		for (i = 0; i < TE_METRIC_COUNT; i++) g.model.metric_set[i] = 0;
		changed();
		set_status("Every size is back to the look.");
	} else if (strcmp(action, "te.reset.syntax") == 0) {
		begin_change("reset.all");
		for (i = 0; i < TE_SYNTAX_COUNT; i++) g.model.syntax[i].set = 0;
		changed();
	} else if (strcmp(action, "te.xml.apply") == 0) {
		const char* text = Rtext_editor_get_text(g.xml);
		te_model before;
		memset(&before, 0, sizeof(before));
		te_model_copy(&before, &g.model);
		if (load_text(text, strlen(text), "The XML")) {
			te_model now;
			memset(&now, 0, sizeof(now));
			te_model_copy(&now, &g.model);
			te_model_copy(&g.model, &before);
			begin_change("xml");
			te_model_copy(&g.model, &now);
			te_model_free(&now);
			g.xml_edited = 0;
			Rwidget_set_enabled(g.xml_apply, 0);
			Rwidget_set_enabled(g.xml_revert, 0);
			ui_from_model();
			refresh();
			set_status("The edits of the file are applied.");
		}
		te_model_free(&before);
	} else if (strcmp(action, "te.xml.revert") == 0) {
		g.xml_edited = 0;
		Rwidget_set_enabled(g.xml_apply, 0);
		Rwidget_set_enabled(g.xml_revert, 0);
		refresh();
	} else if (strcmp(action, "te.font.browse") == 0) {
		pick_file(g.model.font, "A font for the theme", "Fonts", "*.ttf;*.otf");
	} else if (strcmp(action, "te.wallpaper.browse") == 0) {
		pick_file(g.model.wallpaper_text, "An image behind glass", "Images", "*.png;*.jpg;*.jpeg;*.bmp");
	}
}

static void action_view(Rid sender, const char* action, void* user_data) {
	crash_report_note("theme editor: action %s", action);
	(void)sender;
	(void)user_data;
	if (strcmp(action, "te.window") == 0) {
		te_text x;
		if (g.preview_window != R_INVALID_ID && Rwindow_is_valid(g.preview_window)) {
			Rwindow_show(g.preview_window);
			return;
		}
		memset(&x, 0, sizeof(x));
		te_add(&x, "<Window Title=\"Preview\" Width=\"960\" Height=\"680\" Theme=\"" PREVIEW_THEME "\">");
		add_preview_pages(&x, "window.");
		te_add(&x, "</Window>");
		g.preview_window = Rwindow_create_from_xml(x.data, x.length);
		te_text_free(&x);
		set_status("The preview in a window: it follows every change, with the wallpaper of the theme.");
	} else if (strcmp(action, "te.ui.dark") == 0) {
		Rlook_set(R_LOOK_RAVEN_DARK);
		refresh();
	} else if (strcmp(action, "te.ui.light") == 0) {
		Rlook_set(R_LOOK_RAVEN_LIGHT);
		refresh();
	}
}

static int on_close(Rid window, void* user_data) {
	(void)window;
	(void)user_data;
	if (g.fuzzing) return R_FALSE;
	if (!g.scripted && !te_model_equal(&g.model, &g.saved)) {
		RMessageResult answer = Rmessage_box(g.window, "Save the theme?", "The theme has changes that are not saved.", R_MESSAGE_YES_NO_CANCEL);
		if (answer == R_MESSAGE_RESULT_CANCEL) return R_FALSE;
		if (answer == R_MESSAGE_RESULT_YES && !(g.path[0] ? save_to(g.path) : save_as())) return R_FALSE;
	}
	if (g.preview_window != R_INVALID_ID && Rwindow_is_valid(g.preview_window)) Rwindow_close(g.preview_window);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Start
 * ------------------------------------------------------------------------- */

static int find_widgets(void) {
	char name[64];
	int i;
	g.preview_root = W("preview.root");
	g.preview_backdrop = W("preview.backdrop");
	g.preview_button = W("preview.button");
	g.probe = W("probe");
	g.tabs = W("tabs");
	g.name = W("name");
	g.look = W("look");
	g.dark = W("dark");
	g.material = W("material");
	g.motion = W("motion");
	g.look_hint = W("look.hint");
	g.accent_own = W("accent.own");
	g.accent = W("accent");
	g.tint = W("tint");
	g.contrast = W("contrast");
	g.tint_value = W("tint.value");
	g.contrast_value = W("contrast.value");
	g.font = W("font");
	g.wallpaper = W("wallpaper");
	g.wallpaper_text = W("wallpaper.text");
	g.wallpaper_browse = W("wallpaper.browse");
	g.filter = W("filter");
	g.styles = W("styles");
	g.styles_error = W("styles.error");
	g.snippets = W("snippets");
	g.xml = W("xml");
	g.xml_apply = W("xml.apply");
	g.xml_revert = W("xml.revert");
	g.status = W("status");
	g.file_label = W("file");
	g.check_summary = W("check.summary");
	g.backdrop = W("backdrop");
	g.preview_enabled = W("preview.enabled");
	g.compare = W("compare");
	g.undo_button = W("undo");
	g.redo_button = W("redo");
	for (i = 0; i < TE_COLOR_COUNT; i++) {
		snprintf(name, sizeof(name), "color.row.%d", i); g.color_row[i] = W(name);
		snprintf(name, sizeof(name), "color.pick.%d", i); g.color_picker[i] = W(name);
		snprintf(name, sizeof(name), "color.hex.%d", i); g.color_hex[i] = W(name);
		snprintf(name, sizeof(name), "color.reset.%d", i); g.color_reset[i] = W(name);
		Rcolor_picker_set_changed_callback(g.color_picker[i], on_color, (void*)(intptr_t)i);
		Rbutton_set_clicked_callback(g.color_reset[i], on_color_reset, (void*)(intptr_t)i);
	}
	for (i = 0; i < g.color_group_count; i++) {
		snprintf(name, sizeof(name), "color.group.%d", i);
		g.color_group[i] = W(name);
	}
	for (i = 0; i < TE_METRIC_COUNT; i++) {
		snprintf(name, sizeof(name), "metric.slider.%d", i); g.metric_slider[i] = W(name);
		snprintf(name, sizeof(name), "metric.spin.%d", i); g.metric_spin[i] = W(name);
		snprintf(name, sizeof(name), "metric.reset.%d", i); g.metric_reset[i] = W(name);
		snprintf(name, sizeof(name), "metric.state.%d", i); g.metric_state[i] = W(name);
		Rslider_set_changed_callback(g.metric_slider[i], on_metric_slider, (void*)(intptr_t)i);
		Rspin_box_set_changed_callback(g.metric_spin[i], on_metric_spin, (void*)(intptr_t)i);
		Rbutton_set_clicked_callback(g.metric_reset[i], on_metric_reset, (void*)(intptr_t)i);
	}
	for (i = 0; i < TE_SYNTAX_COUNT; i++) {
		snprintf(name, sizeof(name), "syntax.pick.%d", i); g.syntax_picker[i] = W(name);
		snprintf(name, sizeof(name), "syntax.hex.%d", i); g.syntax_hex[i] = W(name);
		snprintf(name, sizeof(name), "syntax.reset.%d", i); g.syntax_reset[i] = W(name);
		Rcolor_picker_set_changed_callback(g.syntax_picker[i], on_syntax, (void*)(intptr_t)i);
		Rbutton_set_clicked_callback(g.syntax_reset[i], on_syntax_reset, (void*)(intptr_t)i);
	}
	for (i = 0; i < PAIR_COUNT; i++) {
		snprintf(name, sizeof(name), "check.swatch.%d", i); g.check_swatch[i] = W(name);
		snprintf(name, sizeof(name), "check.sample.%d", i); g.check_sample[i] = W(name);
		snprintf(name, sizeof(name), "check.ratio.%d", i); g.check_ratio[i] = W(name);
		snprintf(name, sizeof(name), "check.badge.%d", i); g.check_badge[i] = W(name);
		snprintf(name, sizeof(name), "check.fix.%d", i); g.check_fix[i] = W(name);
		Rbutton_set_clicked_callback(g.check_fix[i], on_fix, (void*)(intptr_t)i);
	}
	if (g.probe == R_INVALID_ID || g.preview_root == R_INVALID_ID || g.status == R_INVALID_ID) return 0;

	Rtext_input_set_changed_callback(g.name, on_name, NULL);
	Rcombo_box_set_changed_callback(g.look, on_combo, &g.model.look);
	Rcombo_box_set_changed_callback(g.dark, on_combo, &g.model.dark);
	Rcombo_box_set_changed_callback(g.material, on_combo, &g.model.material);
	Rcombo_box_set_changed_callback(g.motion, on_combo, &g.model.motion);
	Rcombo_box_set_changed_callback(g.wallpaper, on_combo, &g.model.wallpaper);
	Rcolor_picker_set_changed_callback(g.accent, on_accent, NULL);
	Rcheckbox_set_changed_callback(g.accent_own, on_accent_own, NULL);
	Rslider_set_changed_callback(g.tint, on_slider_label, (void*)(uintptr_t)g.tint_value);
	Rslider_set_changed_callback(g.contrast, on_slider_label, (void*)(uintptr_t)g.contrast_value);
	on_slider_label(g.tint, Rslider_get_value(g.tint), (void*)(uintptr_t)g.tint_value);
	on_slider_label(g.contrast, Rslider_get_value(g.contrast), (void*)(uintptr_t)g.contrast_value);
	Rtext_input_set_changed_callback(g.font, on_path, g.model.font);
	Rtext_input_set_changed_callback(g.wallpaper_text, on_path, g.model.wallpaper_text);
	Rtext_input_set_committed_callback(g.font, on_path_committed, g.model.font);
	Rtext_input_set_committed_callback(g.wallpaper_text, on_path_committed, g.model.wallpaper_text);
	Rtext_input_set_changed_callback(g.filter, on_filter, NULL);
	Rtext_editor_set_changed_callback(g.styles, on_styles, NULL);
	Rtext_editor_set_changed_callback(g.xml, on_xml_edited, NULL);
	Rcombo_box_set_changed_callback(g.snippets, on_snippet, NULL);
	Rcombo_box_set_changed_callback(g.backdrop, on_backdrop, NULL);
	Rcheckbox_set_changed_callback(g.preview_enabled, on_preview_enabled, NULL);
	Rcheckbox_set_changed_callback(g.compare, on_compare, NULL);
	Rcombo_box_set_selected(g.snippets, 0);
	Rcombo_box_set_selected(g.backdrop, 0);
	if (!g.embedded) Rwindow_set_close_callback(g.window, on_close, NULL);
	return 1;
}

/* ---------------------------------------------------------------------------
 *  Starting, alone or inside the designer
 * ------------------------------------------------------------------------- */

static void bind_actions(void) {
	int i;
	static const char* const edits[] = { "te.undo", "te.redo", "te.copy", "te.generate", "te.surprise", "te.reset.colors",
		"te.reset.metrics", "te.reset.syntax", "te.xml.apply", "te.xml.revert", "te.font.browse", "te.wallpaper.browse",
		"te.copy.c", "te.fix.all" };
	Raction_bind("te.open", action_file, NULL);
	Raction_bind("te.save", action_file, NULL);
	Raction_bind("te.save_as", action_file, NULL);
	Raction_bind("te.quit", action_file, NULL);
	for (i = 1; i < g_te_look_count; i++) {
		char name[32];
		snprintf(name, sizeof(name), "te.new.%d", i);
		Raction_bind(name, action_file, NULL);
	}
	for (i = 0; i < (int)(sizeof(edits) / sizeof(edits[0])); i++) Raction_bind(edits[i], action_edit, NULL);
	Raction_bind("te.window", action_view, NULL);
	Raction_bind("te.ui.dark", action_view, NULL);
	Raction_bind("te.ui.light", action_view, NULL);
}

/* The model to start from, and the two themes the interface names (before it is made). */
static void prepare(void) {
	te_text x;
	srand((unsigned)(Rget_time() * 1000.0) ^ 0x5EED);
	/* a theme to start from: Raven dark, an accent of its own */
	te_model_init(&g.model);
	g.model.accent_set = 1;
	g.model.accent[0] = 0.49f;
	g.model.accent[1] = 0.36f;
	g.model.accent[2] = 1.0f;
	te_model_init(&g.saved);
	g.probe = R_INVALID_ID;
	g.preview_window = R_INVALID_ID;
	g.styles_timer = R_INVALID_ID;
	g.backdrop_texture = R_INVALID_ID;
	g.source = R_INVALID_ID;
	g.compare = R_INVALID_ID;
	update_base();
	memset(&x, 0, sizeof(x));
	te_model_write(&g.model, &x, PREVIEW_THEME, 0, NULL);
	Rtheme_load_from_memory(x.data, x.length);
	te_text_free(&x);
}

static void finish(void) {
	bind_actions();
	te_model_copy(&g.saved, &g.model);
	ui_from_model();
	refresh();
	set_status("Pick a look and an accent, or generate a whole palette: the preview on the right follows every change.");
}

static void release(void) {
	if (g.backdrop_texture != R_INVALID_ID && Rtexture_is_valid(g.backdrop_texture)) Rtexture_destroy(g.backdrop_texture);
	g.backdrop_texture = R_INVALID_ID;
	clear_stack(g.undo, &g.undo_count);
	clear_stack(g.redo, &g.redo_count);
	te_model_free(&g.model);
	te_model_free(&g.saved);
	te_text_free(&g.errors);
	free(g.base_xml);
	g.base_xml = NULL;
}

Rid te_embed(Rid window) {
	te_text x;
	memset(&g, 0, sizeof(g));
	g.embedded = 1;
	g.window = window;
	prepare();
	memset(&x, 0, sizeof(x));
	build_content(&x);
	g.root = Rwidget_create_from_xml(x.data, x.length);
	te_text_free(&x);
	if (g.root == R_INVALID_ID || !find_widgets()) return R_INVALID_ID;
	finish();
	return g.root;
}

int te_open(const char* path) {
	return open_file(path);
}

int te_is_modified(void) {
	return g.root != R_INVALID_ID && !te_model_equal(&g.model, &g.saved);
}

int te_save(void) {
	/* (no dialog during the random runs of the tests) */
	if (!g.path[0] && g.fuzzing) return 0;
	return g.path[0] ? save_to(g.path) : save_as();
}

const char* te_get_path(void) {
	return g.path;
}

void te_set_scripted(int scripted) {
	g.scripted = scripted;
}

void te_set_fuzzing(int fuzzing) {
	g.fuzzing = fuzzing;
}

void te_error(RError code, RSeverity severity, const char* detail) {
	/* (nothing before te_embed nor after te_shutdown) */
	if (!g.embedded || g.root == R_INVALID_ID) return;
	on_error(code, severity, detail);
}

void te_shutdown(void) {
	if (g.preview_window != R_INVALID_ID && Rwindow_is_valid(g.preview_window)) Rwindow_destroy(g.preview_window);
	release();
	g.root = R_INVALID_ID;
}

#ifndef TE_EMBEDDED

/* ---------------------------------------------------------------------------
 *  Scripted runs (the tests)
 * ------------------------------------------------------------------------- */

static void run_frames(int count) {
	int i;
	for (i = 0; i < count && Ris_running(); i++) {
		Rwindow_request_redraw(g.window);
		Rdraw_frame();
	}
}

static int index_of_color(const char* name) {
	int i;
	for (i = 0; i < TE_COLOR_COUNT; i++) {
		if (te_stricmp(g_te_colors[i].name, name) == 0) return i;
	}
	return -1;
}

static int index_of_metric(const char* name) {
	int i;
	for (i = 0; i < TE_METRIC_COUNT; i++) {
		if (te_stricmp(g_te_metrics[i].name, name) == 0) return i;
	}
	return -1;
}

static int index_of_look(const char* name) {
	int i;
	for (i = 1; i < g_te_look_count; i++) {
		if (te_stricmp(g_te_looks[i].name, name) == 0) return i;
	}
	return -1;
}

/* xorshift: the same seed gives the same run, to replay a crash */
static uint32_t g_fuzz_state = 1;
static uint32_t fuzz_next(void) {
	g_fuzz_state ^= g_fuzz_state << 13;
	g_fuzz_state ^= g_fuzz_state >> 17;
	g_fuzz_state ^= g_fuzz_state << 5;
	return g_fuzz_state;
}
static int fuzz_int(int n) {
	return n > 0 ? (int)(fuzz_next() % (uint32_t)n) : 0;
}

/*
 * `steps` random things a person could do: clicks, drags and wheels anywhere
 * in the windows (menus, popups and pickers included), keys and text, and the
 * actions of the editor. Under a sanitizer, this finds what crashes.
 */
static void fuzz(int steps, uint32_t seed) {
	static const RKey keys[] = { R_KEY_TAB, R_KEY_ENTER, R_KEY_ESCAPE, R_KEY_BACKSPACE, R_KEY_DELETE, R_KEY_LEFT, R_KEY_RIGHT,
		R_KEY_UP, R_KEY_DOWN, R_KEY_HOME, R_KEY_END, R_KEY_PAGE_DOWN, R_KEY_SPACE, R_KEY_A, R_KEY_Z, R_KEY_Y, R_KEY_C, R_KEY_V, R_KEY_F7 };
	static const char* const texts[] = { "a", "#", "FF", "<Style Widget=\"Button\" CornerRadius=\"8\"/>", "<", "\"", "/>", "12", " ", "\xC3\xA9", "<Theme>", "&amp;" };
	static const char* const actions[] = { "te.undo", "te.redo", "te.generate", "te.surprise", "te.reset.colors", "te.reset.metrics",
		"te.reset.syntax", "te.copy", "te.copy.c", "te.fix.all", "te.window", "te.ui.dark", "te.ui.light", "te.xml.apply", "te.xml.revert", "te.save" };
	int step;
	float width = 1460.0f, height = 900.0f;
	g_fuzz_state = seed ? seed : 1;
	g.fuzzing = 1;
	for (step = 0; step < steps && Ris_running(); step++) {
		Rid window = g.window;
		int what = fuzz_int(100);
		float x, y;
		if (g.preview_window != R_INVALID_ID && Rwindow_is_valid(g.preview_window) && fuzz_int(6) == 0) window = g.preview_window;
		x = (float)fuzz_int((int)width);
		y = (float)fuzz_int((int)height);
		if (what < 30) {
			/* a click, sometimes a double one */
			int k, clicks = fuzz_int(5) == 0 ? 2 : 1;
			Rwindow_inject_mouse_move(window, x, y);
			for (k = 0; k < clicks; k++) {
				Rwindow_inject_mouse_button(window, fuzz_int(8) == 0 ? R_MOUSE_RIGHT : R_MOUSE_LEFT, 1);
				Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 0);
				Rwindow_inject_mouse_button(window, R_MOUSE_RIGHT, 0);
			}
		} else if (what < 45) {
			/* a drag (sliders, pickers, splitters, text) */
			int k;
			Rwindow_inject_mouse_move(window, x, y);
			Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 1);
			for (k = 0; k < 4; k++) {
				Rwindow_inject_mouse_move(window, x + (float)(fuzz_int(200) - 100), y + (float)(fuzz_int(120) - 60));
				Rdraw_frame();
			}
			Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 0);
		} else if (what < 50) {
			Rwindow_inject_mouse_move(window, x, y);
			Rwindow_inject_mouse_wheel(window, 0.0f, (float)(fuzz_int(7) - 3));
		} else if (what < 62) {
			RKey key = keys[fuzz_int((int)(sizeof(keys) / sizeof(keys[0])))];
			Ruint mods = fuzz_int(4) == 0 ? R_MOD_CTRL : fuzz_int(6) == 0 ? R_MOD_SHIFT : 0;
			/* (Ctrl+Q would quit, Ctrl+O and Ctrl+Shift+S open file dialogs: the actions refuse them while fuzzing) */
			Rwindow_inject_key(window, key, 1, mods);
			Rwindow_inject_key(window, key, 0, mods);
		} else if (what < 72) {
			Rwindow_inject_text(window, texts[fuzz_int((int)(sizeof(texts) / sizeof(texts[0])))]);
		} else if (what < 80) {
			Raction_fire(actions[fuzz_int((int)(sizeof(actions) / sizeof(actions[0])))], R_INVALID_ID);
		} else if (what < 84) {
			int look = fuzz_int(g_te_look_count);
			Rcombo_box_set_selected(g.look, look);
			on_combo(g.look, look, &g.model.look);
		} else if (what < 87) {
			char action[32];
			snprintf(action, sizeof(action), "te.new.%d", 1 + fuzz_int(g_te_look_count - 1));
			Raction_fire(action, R_INVALID_ID);
		} else if (what < 91) {
			Rtab_view_set_active(g.tabs, fuzz_int(7));
		} else if (what < 94) {
			Rid pages = Rwidget_find(g.preview_root, "preview.pages");
			if (pages != R_INVALID_ID) Rtab_view_set_active(pages, fuzz_int(5));
		} else if (what < 96) {
			Rcheckbox_set_checked(g.compare, fuzz_int(2));
			on_compare(g.compare, Rcheckbox_is_checked(g.compare), NULL);
		} else if (what < 98) {
			int index = fuzz_int(TE_COLOR_COUNT);
			on_color(g.color_picker[index], (float)fuzz_int(256) / 255.0f, (float)fuzz_int(256) / 255.0f, (float)fuzz_int(256) / 255.0f,
				(float)fuzz_int(256) / 255.0f, (void*)(intptr_t)index);
		} else {
			/* the preview window closed by the user */
			if (g.preview_window != R_INVALID_ID && Rwindow_is_valid(g.preview_window)) Rwindow_close(g.preview_window);
		}
		if (fuzz_int(3) == 0) Rdraw_frame();
		/* (a timer of the Styles tab may be waiting: let time pass now and then) */
		if (fuzz_int(40) == 0) {
			int k;
			for (k = 0; k < 25; k++) Rdraw_frame();
		}
	}
	g.fuzzing = 0;
}

static int same_rgb(const float* a, const float* b) {
	return fabsf(a[0] - b[0]) < 0.006f && fabsf(a[1] - b[1]) < 0.006f && fabsf(a[2] - b[2]) < 0.006f;
}

static void tilde(char* word) {
	for (; *word; word++) {
		if (*word == '~') *word = ' ';
	}
}

/*
 * Commands, separated by spaces ('~' stands for a space in a word):
 *   f N                    N frames
 *   do ACTION              fires an action (te.undo, te.generate, te.new.3...)
 *   name TEXT              types the name of the theme
 *   look NAME              picks a look (RavenDark, WindowsLight...)
 *   accent #RRGGBB         picks the accent
 *   color ROLE #RRGGBBAA   picks a color
 *   reset ROLE             puts a color back to the look
 *   metric NAME VALUE      drags the slider of a size
 *   syntax NAME #RRGGBB    picks a syntax color
 *   styles TEXT            types the rules of the Styles tab ('-': none)
 *   snippet INDEX          adds a rule of "Add a rule"
 *   save PATH / open PATH  writes / reads a theme file
 *   tab N / page N         shows a tab of the editor / a page of the preview
 *   fuzz STEPS SEED        random clicks, drags, keys, text and actions (the same seed replays the same run)
 *   fix N                  the Fix button of the pair N of the Check tab
 *   compare 0|1            the preview shows the saved version (1) or the changes (0)
 *   expectclipboard TEXT   stops unless the clipboard contains TEXT
 *   window                 opens the preview in a window
 *   expectxml TEXT         stops unless the theme file contains TEXT
 *   noxml TEXT             stops if the theme file contains TEXT
 *   expectpreview ROLE #RRGGBB   stops unless the primary button of the preview has that color
 *   expectmetric NAME VALUE      stops unless the preview has that size
 *   expectbase ROLE #RRGGBB      stops unless the look gives that color
 *   expectfile PATH TEXT   stops unless the file contains TEXT
 *   expectcheck N          stops unless N pairs of the Check tab read well
 *   expecterror TEXT       stops unless the Styles tab shows an error with TEXT ('-': no error)
 *   expectundo N           stops unless N steps can be undone
 *   expectsize NAME W H    stops unless the widget NAME is at least W x H
 */
static int run_script(char* script) {
	char* words[2048];
	int count = 0, i, ok = 1;
	char* walk = strtok(script, " \t\r\n");
	while (walk && count < 2048) {
		words[count++] = walk;
		walk = strtok(NULL, " \t\r\n");
	}
#define ARG(k) (i + (k) < count ? words[i + (k)] : "")
#define FAIL(...) do { fprintf(stderr, "raven_theme_editor: script, '%s': ", words[i]); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); ok = 0; } while (0)
	for (i = 0; i < count && ok; i++) {
		const char* command = words[i];
		float rgba[4] = { 0, 0, 0, 1 };
		if (strcmp(command, "f") == 0) {
			run_frames(atoi(ARG(1)));
			i++;
		} else if (strcmp(command, "do") == 0) {
			Raction_fire(ARG(1), R_INVALID_ID);
			i++;
		} else if (strcmp(command, "name") == 0) {
			if (i + 1 < count) tilde(words[i + 1]);
			Rtext_input_set_text(g.name, ARG(1));
			on_name(g.name, ARG(1), NULL);
			i++;
		} else if (strcmp(command, "look") == 0) {
			int look = index_of_look(ARG(1));
			if (look < 0) FAIL("no look '%s'", ARG(1));
			else {
				Rcombo_box_set_selected(g.look, look);
				on_combo(g.look, look, &g.model.look);
			}
			i++;
		} else if (strcmp(command, "accent") == 0) {
			if (!te_parse_hex(ARG(1), rgba)) FAIL("not a color");
			else {
				Rcolor_picker_set_color(g.accent, rgba[0], rgba[1], rgba[2], 1.0f);
				on_accent(g.accent, rgba[0], rgba[1], rgba[2], 1.0f, NULL);
			}
			i++;
		} else if (strcmp(command, "color") == 0 || strcmp(command, "syntax") == 0) {
			int index = -1, k;
			if (command[0] == 'c') index = index_of_color(ARG(1));
			else for (k = 0; k < TE_SYNTAX_COUNT; k++) if (te_stricmp(g_te_syntax[k].name, ARG(1)) == 0) index = k;
			if (index < 0 || !te_parse_hex(ARG(2), rgba)) FAIL("'%s %s'", ARG(1), ARG(2));
			else if (command[0] == 'c') on_color(g.color_picker[index], rgba[0], rgba[1], rgba[2], rgba[3], (void*)(intptr_t)index);
			else on_syntax(g.syntax_picker[index], rgba[0], rgba[1], rgba[2], rgba[3], (void*)(intptr_t)index);
			i += 2;
		} else if (strcmp(command, "reset") == 0) {
			int index = index_of_color(ARG(1));
			if (index < 0) FAIL("no color '%s'", ARG(1));
			else on_color_reset(g.color_reset[index], (void*)(intptr_t)index);
			i++;
		} else if (strcmp(command, "metric") == 0) {
			int index = index_of_metric(ARG(1));
			if (index < 0) FAIL("no size '%s'", ARG(1));
			else {
				Rslider_set_value(g.metric_slider[index], (float)atof(ARG(2)));
				on_metric_slider(g.metric_slider[index], (float)atof(ARG(2)), (void*)(intptr_t)index);
			}
			i += 2;
		} else if (strcmp(command, "styles") == 0) {
			if (i + 1 < count) tilde(words[i + 1]);
			Rtext_editor_set_text(g.styles, strcmp(ARG(1), "-") == 0 ? "" : ARG(1));
			apply_styles(R_INVALID_ID, NULL);
			i++;
		} else if (strcmp(command, "snippet") == 0) {
			add_snippet(atoi(ARG(1)));
			i++;
		} else if (strcmp(command, "save") == 0) {
			if (!save_to(ARG(1))) FAIL("cannot write %s", ARG(1));
			i++;
		} else if (strcmp(command, "open") == 0) {
			if (!open_file(ARG(1))) FAIL("cannot open %s", ARG(1));
			i++;
		} else if (strcmp(command, "fuzz") == 0) {
			fuzz(atoi(ARG(1)), (uint32_t)strtoul(ARG(2), NULL, 10));
			i += 2;
		} else if (strcmp(command, "fix") == 0) {
			on_fix(g.check_fix[atoi(ARG(1))], (void*)(intptr_t)atoi(ARG(1)));
			i++;
		} else if (strcmp(command, "compare") == 0) {
			Rcheckbox_set_checked(g.compare, atoi(ARG(1)));
			on_compare(g.compare, atoi(ARG(1)), NULL);
			i++;
		} else if (strcmp(command, "expectclipboard") == 0) {
			const char* text = Rclipboard_get_text();
			if (i + 1 < count) tilde(words[i + 1]);
			if (!text || !strstr(text, ARG(1))) FAIL("'%s' not in the clipboard:\n%s", ARG(1), text ? text : "(empty)");
			i++;
		} else if (strcmp(command, "tab") == 0) {
			Rtab_view_set_active(g.tabs, atoi(ARG(1)));
			i++;
		} else if (strcmp(command, "page") == 0) {
			Rtab_view_set_active(Rwidget_find(g.preview_root, "preview.pages"), atoi(ARG(1)));
			i++;
		} else if (strcmp(command, "window") == 0) {
			Raction_fire("te.window", R_INVALID_ID);
			if (g.preview_window == R_INVALID_ID) FAIL("no window");
		} else if (strcmp(command, "expectxml") == 0 || strcmp(command, "noxml") == 0) {
			te_text xml;
			int found;
			memset(&xml, 0, sizeof(xml));
			te_model_write(&g.model, &xml, NULL, 0, NULL);
			if (i + 1 < count) tilde(words[i + 1]);
			found = strstr(xml.data, ARG(1)) != NULL;
			if (found != (command[0] == 'e')) FAIL("'%s'%s in:\n%s", ARG(1), found ? "" : " not found", xml.data);
			te_text_free(&xml);
			i++;
		} else if (strcmp(command, "expectpreview") == 0 || strcmp(command, "expectbase") == 0) {
			float got[4];
			int index = index_of_color(ARG(1));
			if (index < 0 || !te_parse_hex(ARG(2), rgba)) FAIL("'%s %s'", ARG(1), ARG(2));
			else {
				char hex[16];
				if (command[6] == 'p') Rwidget_get_color(g.preview_button, g_te_colors[index].role, &got[0], &got[1], &got[2], &got[3]);
				else memcpy(got, g.base_colors[index], sizeof(got));
				if (!same_rgb(got, rgba)) FAIL("%s is %s, not %s", ARG(1), te_hex(got, hex, sizeof(hex)), ARG(2));
			}
			i += 2;
		} else if (strcmp(command, "expectmetric") == 0) {
			int index = index_of_metric(ARG(1));
			float got = index >= 0 ? Rwidget_get_metric(g.preview_button, g_te_metrics[index].metric) : 0.0f;
			if (index < 0 || fabsf(got - (float)atof(ARG(2))) > 0.01f) FAIL("%s is %g, not %s", ARG(1), got, ARG(2));
			i += 2;
		} else if (strcmp(command, "expectfile") == 0) {
			size_t size = 0;
			char* text = Rfile_read(ARG(1), &size);
			if (i + 2 < count) tilde(words[i + 2]);
			if (!text || !strstr(text, ARG(2))) FAIL("'%s' not in %s", ARG(2), ARG(1));
			Rmemory_free(text);
			i += 2;
		} else if (strcmp(command, "expectcheck") == 0) {
			const char* summary = Rlabel_get_text(g.check_summary);
			char want[16];
			snprintf(want, sizeof(want), "%s of", ARG(1));
			if (!strstr(summary, want) && !(strstr(summary, "Every") && atoi(ARG(1)) == PAIR_COUNT)) {
				int k;
				FAIL("the check says '%s'", summary);
				for (k = 0; k < PAIR_COUNT; k++) fprintf(stderr, "  %-40s %s %s\n", g_pairs[k].label, Rlabel_get_text(g.check_ratio[k]), Rlabel_get_text(g.check_badge[k]));
			}
			i++;
		} else if (strcmp(command, "expecterror") == 0) {
			const char* shown = Rlabel_get_text(g.styles_error);
			if (i + 1 < count) tilde(words[i + 1]);
			if (strcmp(ARG(1), "-") == 0 ? shown[0] != '\0' : !te_contains(shown, ARG(1))) FAIL("the error is '%s'", shown);
			i++;
		} else if (strcmp(command, "expectsize") == 0) {
			Rid widget = Rwidget_find(g.window, ARG(1));
			float x, y, w, h;
			if (widget == R_INVALID_ID) FAIL("no widget '%s'", ARG(1));
			else {
				Rwidget_get_rect(widget, &x, &y, &w, &h);
				if (w < (float)atof(ARG(2)) || h < (float)atof(ARG(3))) FAIL("%s is %gx%g at %g,%g", ARG(1), w, h, x, y);
			}
			i += 3;
		} else if (strcmp(command, "expectundo") == 0) {
			if (g.undo_count != atoi(ARG(1))) FAIL("%d steps can be undone", g.undo_count);
			i++;
		} else {
			FAIL("unknown command");
		}
	}
#undef ARG
#undef FAIL
	return ok;
}

int main(int argc, char** argv) {
	RInitDesc desc;
	te_text x;
	const char* file = NULL;
	char* script = NULL;
	int frames = -1, i, ok = 1, code;

	memset(&g, 0, sizeof(g));
	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_VULKAN;
	desc.app_name = "Raven Theme Editor";
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--null") == 0) {
			desc.backend = R_BACKEND_NULL;
			desc.headless = R_TRUE;
		} else if (strcmp(argv[i], "--headless") == 0) {
			desc.headless = R_TRUE;
		} else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
			frames = atoi(argv[++i]);
		} else if (strcmp(argv[i], "--script") == 0 && i + 1 < argc) {
			script = te_strdup(argv[++i]);
		} else if (argv[i][0] == '-') {
			fprintf(stderr, "usage: raven_theme_editor [theme.xml] [--null | --headless] [--frames N] [--script \"...\"]\n");
			return 2;
		} else {
			file = argv[i];
		}
	}
	g.scripted = frames >= 0 || script != NULL;
	/* a crash leaves a report (raven_theme_editor-crash.txt) in the folder of the settings of Raven */
	if (!g.scripted) {
		char folder[1024];
#if defined(_WIN32)
		const char* base = getenv("APPDATA");
		snprintf(folder, sizeof(folder), "%s/Raven", base && base[0] ? base : ".");
#else
		const char* base = getenv("XDG_CONFIG_HOME");
		if (base && base[0]) snprintf(folder, sizeof(folder), "%s/raven", base);
		else snprintf(folder, sizeof(folder), "%s/.config/raven", getenv("HOME") ? getenv("HOME") : ".");
#endif
		crash_report_install(folder, "raven_theme_editor");
	}
	Rerror_set_callback(on_error);
	if (!Rinit_ex(&desc)) {
		fprintf(stderr, "raven_theme_editor: cannot start Raven (is a Vulkan driver installed?)\n");
		return 1;
	}
	if (g.scripted) Rset_animation_time_step(1.0 / 60.0);
	Rlook_set(R_LOOK_RAVEN_DARK);
	prepare();

	memset(&x, 0, sizeof(x));
	build_interface(&x);
	g.window = Rwindow_create_from_xml(x.data, x.length);
	g.root = g.window;
	te_text_free(&x);
	if (g.window == R_INVALID_ID || !find_widgets()) {
		fprintf(stderr, "raven_theme_editor: cannot build the window\n");
		Rshutdown();
		return 1;
	}

	finish();
	if (file) open_file(file);
	{
		const char* report = crash_report_previous();
		if (report) set_status("The theme editor stopped on a crash last time. What happened is in %s: send it with the .dmp next to it.", report);
	}

	if (script) ok = run_script(script);
	if (frames >= 0) run_frames(frames);
	else if (!script) {
		while (Rwait_frame(-1.0)) {}
	}

	code = Rget_exit_code();
	if (g.backdrop_texture != R_INVALID_ID) Rtexture_destroy(g.backdrop_texture);
	g.backdrop_texture = R_INVALID_ID;
	Rshutdown();
	release();
	free(script);
	return ok ? code : 1;
}

#endif /* TE_EMBEDDED */
