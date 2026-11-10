/**
 * Raven example - workspace
 * The frame of a tool: panels that dock, a file browser, materials.
 *
 *   DockArea       panels arranged by the user (workspace.xml); the
 *                  arrangement is a short text to save and load
 *                  (Rdock_area_save_layout / load_layout); panels added
 *                  and closed from C
 *   FileBrowser    the folders and files of the disk, as a widget: a double
 *                  click on a file opens it in a new panel
 *   materials      Rtheme_set_material for everything, Rwidget_set_material
 *                  for one widget and what it contains
 *   Island         tells what just happened, then folds back
 *   HeaderBar      without background: the bar is the top of the window
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static struct {
	Rid window, dock, welcome, files, log, island, look;
	char* saved_layout;
	int notes;
} app;

static void say(const char* title, const char* detail) {
	char line[600];
	snprintf(line, sizeof(line), "%s: %s", title, detail);
	Rlist_view_add_item(app.log, line);
	Rlist_view_scroll_to_item(app.log, Rlist_view_get_item_count(app.log) - 1);
	Rlabel_set_text(Rwidget_find(app.window, "island.text"), title);
	Rlabel_set_text(Rwidget_find(app.window, "island.title"), title);
	Rlabel_set_text(Rwidget_find(app.window, "island.detail"), detail);
	Risland_expand_for(app.island, 2.0);
}

/* ---------------------------------------------------------------------------
 *  Panels
 * ------------------------------------------------------------------------- */

/* A new panel beside the editor, as a tab of the same group. */
static Rid add_editor_panel(const char* title, const char* text, const char* language) {
	Rid editor = Rtext_editor_create(text);
	Rtext_editor_set_line_numbers(editor, R_TRUE);
	Rtext_editor_set_language(editor, language);
	/* the first panel may have been closed: then the new one takes the middle of the area */
	Rdock_area_add_panel(app.dock, editor, title, R_DOCK_TAB, Rwidget_is_valid(app.welcome) ? app.welcome : R_INVALID_ID, 0.0f);
	Rdock_area_show_panel(app.dock, editor);
	return editor;
}

static void on_file_accepted(Rid browser, const char* path, void* user_data) {
	size_t size = 0;
	char* text = Rfile_read(path, &size);
	const char* name = strrchr(path, '/');
	const char* other = strrchr(path, '\\');
	(void)browser;
	(void)user_data;
	if (other && (!name || other > name)) name = other;
	name = name ? name + 1 : path;
	if (!text) {
		say("Cannot open", path);
		return;
	}
	if (size > 2 * 1024 * 1024) {
		say("Too large to show", name);
	} else {
		add_editor_panel(name, text, Rlanguage_for_file(path));
		say("Opened", name);
	}
	Rmemory_free(text);
}

/* Returning R_FALSE would keep the panel open (unsaved changes...). */
static int on_panel_close(Rid area, Rid panel, void* user_data) {
	(void)area;
	(void)user_data;
	if (panel == app.welcome) app.welcome = R_INVALID_ID;
	return R_TRUE;
}

static void on_dock_changed(Rid area, void* user_data) {
	(void)user_data;
	/* what would be written to a settings file */
	Rlabel_set_text(Rwidget_find(app.window, "layout.text"), Rdock_area_save_layout(area));
}

static void action_layout(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)user_data;
	if (strcmp(action, "layout.save") == 0) {
		const char* layout = Rdock_area_save_layout(app.dock);
		free(app.saved_layout);
		app.saved_layout = (char*)malloc(strlen(layout) + 1);
		strcpy(app.saved_layout, layout);
		say("Layout saved", layout);
	} else if (strcmp(action, "layout.restore") == 0) {
		if (!app.saved_layout) say("Nothing saved yet", "Use \"Save layout\" first.");
		else if (Rdock_area_load_layout(app.dock, app.saved_layout)) say("Layout restored", "The panels fly back to where they were.");
	} else {
		char title[32];
		snprintf(title, sizeof(title), "Note %d", ++app.notes);
		add_editor_panel(title, "", NULL);
		say("New panel", title);
	}
}

/* ---------------------------------------------------------------------------
 *  Materials
 * ------------------------------------------------------------------------- */

static void action_look(Rid sender, const char* action, void* user_data) {
	static const RTheme themes[4] = { R_THEME_DARK, R_THEME_LIGHT, R_THEME_GLASS_DARK, R_THEME_GLASS_LIGHT };
	static const RMaterial materials[5] = { R_MATERIAL_DEFAULT, R_MATERIAL_SOLID, R_MATERIAL_GLASS, R_MATERIAL_FROSTED_GLASS, R_MATERIAL_TINTED_GLASS };
	int index = Rcombo_box_get_selected(sender);
	(void)user_data;
	if (index < 0) return;
	if (strcmp(action, "look.theme") == 0) {
		Rtheme_apply(themes[index]);
		/* (a theme comes with its material: the choice below starts over) */
		Rcombo_box_set_selected(Rwidget_find(app.window, "material"), 0);
		say("Theme", Rcombo_box_get_item_text(sender, index));
	} else if (strcmp(action, "look.material") == 0) {
		/* every widget that was not given a material of its own */
		if (index == 0) Rtheme_apply(themes[Rcombo_box_get_selected(Rwidget_find(app.window, "theme"))]);
		else Rtheme_set_material(materials[index]);
		say("Material of everything", Rcombo_box_get_item_text(sender, index));
	} else {
		/* this panel, and what it contains */
		Rwidget_set_material(app.look, materials[index]);
		say("Material of the panel", Rcombo_box_get_item_text(sender, index));
	}
}

static void example_shutdown(void) {
	free(app.saved_layout);
	Rshutdown();
}

int main(void) {
	static const char WELCOME[] =
		"/*\n"
		" * Drag the tab of a panel:\n"
		" *   - onto a tab bar, to put it with the tabs that are there;\n"
		" *   - onto the left, right, top or bottom of a panel, to split it;\n"
		" *   - onto an edge of the window, to take the whole side.\n"
		" * Drag the line between two panels to resize them.\n"
		" * Double click a file on the left to open it in a new panel.\n"
		" */\n"
		"int main(void) {\n"
		"\treturn 0;\n"
		"}\n";

	if (!example_init("workspace")) return 1;
	Rtheme_apply(R_THEME_GLASS_DARK);

	Raction_bind("layout.save", action_layout, NULL);
	Raction_bind("layout.restore", action_layout, NULL);
	Raction_bind("note.new", action_layout, NULL);
	Raction_bind("look.theme", action_look, NULL);
	Raction_bind("look.material", action_look, NULL);
	Raction_bind("look.panel", action_look, NULL);

	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/workspace.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.dock = Rwidget_find(app.window, "dock");
	app.welcome = Rwidget_find(app.window, "welcome");
	app.files = Rwidget_find(app.window, "files");
	app.log = Rwidget_find(app.window, "log");
	app.island = Rwidget_find(app.window, "island");
	app.look = Rwidget_find(app.window, "look");

	Rtext_editor_set_text(app.welcome, WELCOME);
	Rfile_browser_set_directory(app.files, RAVEN_EXAMPLE_DIR "/..");
	Rfile_browser_set_accepted_callback(app.files, on_file_accepted, NULL);
	Rdock_area_set_close_callback(app.dock, on_panel_close, NULL);
	Rdock_area_set_changed_callback(app.dock, on_dock_changed, NULL);
	on_dock_changed(app.dock, NULL);
	Rdock_area_show_panel(app.dock, app.log);
	Rlist_view_add_item(app.log, "Ready.");

	example_run(app.window, NULL, example_shutdown);
	return 0;
}
