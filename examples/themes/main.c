/**
 * Raven example - themes
 * Theme files, styles, classes, Markdown, and files loaded again when they change.
 *
 *   Rlook_set(look)            the look: Raven's (flat, glass, high contrast), Windows 11, macOS, GNOME, the platform's
 *   Rtheme_load(path)          reads a theme file and keeps it under its name
 *   Rtheme_use(name)           makes a named theme the theme of the application
 *   Rtheme_set_accent(r,g,b)   one color, and the shades that follow from it
 *   Rtheme_save(path, name)    writes the theme in use as a theme file
 *   Class="danger", Theme="paper.xml"       in the XML
 *   Markdown="true"            on a Label (rendered) and on a TextEditor (styled in place)
 *
 * A theme file that names no look (tweaks.xml) is laid over the look in use
 * and follows it; one that names a look (ocean.xml, paper.xml) imposes it.
 *
 * Every file of this example is watched while it runs: themes.xml, ocean.xml,
 * paper.xml, tweaks.xml and notes.md. Save one and the window follows.
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static struct {
	Rid window, themes, looks, status, notes, editor, watch;
	char* notes_text;
} app;

static const RLook g_looks[] = {
	R_LOOK_RAVEN_DARK, R_LOOK_RAVEN_LIGHT, R_LOOK_RAVEN_GLASS_DARK, R_LOOK_RAVEN_GLASS_LIGHT,
	R_LOOK_RAVEN_HIGH_CONTRAST_DARK, R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT,
	R_LOOK_WINDOWS_DARK, R_LOOK_WINDOWS_LIGHT, R_LOOK_MACOS_DARK, R_LOOK_MACOS_LIGHT, R_LOOK_GNOME_DARK, R_LOOK_GNOME_LIGHT,
	R_LOOK_NATIVE_DARK, R_LOOK_NATIVE_LIGHT, R_LOOK_MACOS_GLASS_DARK, R_LOOK_MACOS_GLASS_LIGHT
};

static void fill_looks(void) {
	int i;
	for (i = 0; i < (int)(sizeof(g_looks) / sizeof(g_looks[0])); i++) {
		if (g_looks[i] == Rlook_get()) Rcombo_box_set_selected(app.looks, i);
	}
}

static void fill_themes(void) {
	const char* current = Rtheme_get_name();
	int i;
	Rcombo_box_clear_items(app.themes);
	for (i = 0; i < Rtheme_get_count(); i++) {
		const char* name = Rtheme_get_name_at(i);
		Rcombo_box_add_item(app.themes, name);
		if (current && strcmp(name, current) == 0) Rcombo_box_set_selected(app.themes, i);
	}
}

/* notes.md is not XML: the example reads it itself, again when it changes. */
static void load_notes(void) {
	size_t size = 0;
	char* text = (char*)Rfile_read(RAVEN_EXAMPLE_DIR "/notes.md", &size);
	if (!text) return;
	if (!app.notes_text || strcmp(app.notes_text, text) != 0) {
		Rlabel_set_text(app.notes, text);
		if (!app.notes_text) Rtext_editor_set_text(app.editor, "## Try it\n\nType **bold**, *emphasis*, `code`,\n- a list\n- [a link](https://example.org)\n");
		Rmemory_free(app.notes_text);
		app.notes_text = text;
	} else {
		Rmemory_free(text);
	}
}

static void on_notes_timer(Rid timer, void* user_data) {
	(void)timer;
	(void)user_data;
	load_notes();
}

static void find_widgets(void) {
	app.themes = Rwidget_find(app.window, "themes");
	app.looks = Rwidget_find(app.window, "looks");
	fill_looks();
	app.status = Rwidget_find(app.window, "status");
	app.notes = Rwidget_find(app.window, "notes");
	app.editor = Rwidget_find(app.window, "editor");
	fill_themes();
}

/* A file was loaded again: an interface file may have replaced widgets, a theme file may have a new name. */
static void on_reload(const char* path, int ok, void* user_data) {
	char text[300];
	(void)user_data;
	snprintf(text, sizeof(text), ok ? "Loaded again: %.200s" : "Not loaded (see the error): %.200s", path);
	if (ok) find_widgets();
	Rlabel_set_text(app.status, text);
}

static void action_use(Rid sender, const char* action, void* user_data) {
	int index = Rcombo_box_get_selected(sender);
	(void)action;
	(void)user_data;
	if (index >= 0) Rtheme_use(Rcombo_box_get_item_text(sender, index));
}

static void action_look(Rid sender, const char* action, void* user_data) {
	int index = Rcombo_box_get_selected(sender);
	(void)action;
	(void)user_data;
	if (index >= 0 && index < (int)(sizeof(g_looks) / sizeof(g_looks[0]))) Rlook_set(g_looks[index]);
	fill_themes();
}

static void action_accent(Rid sender, const char* action, void* user_data) {
	float r, g, b, a;
	(void)action;
	(void)user_data;
	Rcolor_picker_get_color(sender, &r, &g, &b, &a);
	Rtheme_set_accent(r, g, b);
}

static void action_save(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	Rlabel_set_text(app.status, Rtheme_save("my-theme.xml", "Mine") ? "Written: my-theme.xml (in the working folder)" : "Could not write my-theme.xml");
}

static void action_docs(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	Rlabel_set_text(app.status, "A link of the Markdown text fired the action \"themes.docs\".");
}

int main(void) {
	const char* theme = getenv("RAVEN_EXAMPLE_THEME");
	if (!example_init("Raven themes")) return 1;

	/* the theme files: each becomes a theme with a name, next to the four built-in ones */
	Rtheme_load(RAVEN_EXAMPLE_DIR "/ocean.xml");
	Rtheme_load(RAVEN_EXAMPLE_DIR "/paper.xml");
	Rtheme_load(RAVEN_EXAMPLE_DIR "/tweaks.xml");
	Rtheme_use(theme ? theme : "Ocean");

	Raction_bind("themes.use", action_use, NULL);
	Raction_bind("themes.look", action_look, NULL);
	Raction_bind("themes.accent", action_accent, NULL);
	Raction_bind("themes.save", action_save, NULL);
	Raction_bind("themes.docs", action_docs, NULL);

	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/themes.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	find_widgets();
	load_notes();
	Rxml_set_reload_callback(on_reload, NULL);
	app.watch = Rtimer_create(0.5, R_TRUE, on_notes_timer, NULL);
	Rwindow_show(app.window);

	if (!example_automated(app.window)) {
		while (Rwait_frame(-1.0)) {
		}
	}
	Rmemory_free(app.notes_text);
	Rshutdown();
	return Rget_exit_code();
}
