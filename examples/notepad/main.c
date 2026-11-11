/**
 * Raven example - notepad
 * A small text editor: what a real one is made of.
 *
 *   custom header     the title bar is a widget of the window (notepad.xml)
 *   TextEditor        several lines, undo, clipboard; with a language: colors,
 *                     proposals while typing, brackets, indentation, Ctrl+/
 *   languages         the list of the library, chosen from the name of the file
 *                     (Rlanguage_for_file), plus one declared here (INI)
 *   completion        a callback adds its own proposals to those of the editor
 *   markers           the lines that say TODO or FIXME are underlined
 *   file dialogs      the ones of the system (Rfile_dialog_open / save)
 *   message box       "discard the changes?" before losing them
 *   drag and drop     a file dropped on the window is opened
 *
 * Run it with a file to open it:  raven_notepad main.c
 */

#include "../common/example.h"

#include <ctype.h>

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static struct {
	Rid window, editor, header, language, status, notes, position;
	char path[1024];
	int modified;
	/** Set while the program itself changes the editor or the list of languages. */
	int loading;
	Rid scan_timer;
	int last_line, last_column;
} app;

/* ---------------------------------------------------------------------------
 *  Title, status
 * ------------------------------------------------------------------------- */

static const char* file_name(const char* path) {
	const char* slash = strrchr(path, '/');
	const char* backslash = strrchr(path, '\\');
	if (backslash && (!slash || backslash > slash)) slash = backslash;
	return slash ? slash + 1 : path;
}

static void update_title(void) {
	char title[1100];
	snprintf(title, sizeof(title), "%s%s - Raven Notepad", app.path[0] ? file_name(app.path) : "Untitled", app.modified ? " *" : "");
	/* the header bar shows the title of its window */
	Rwindow_set_title(app.window, title);
}

static void set_status(const char* format, const char* detail) {
	char text[1200];
	snprintf(text, sizeof(text), format, detail);
	Rlabel_set_text(app.status, text);
}

/* The position of the cursor, read once per frame: cheap, and nothing to wire. */
static void on_frame(double delta_seconds, void* user_data) {
	int line = 0, column = 0;
	char text[64];
	(void)delta_seconds;
	(void)user_data;
	if (!Rwidget_is_valid(app.editor)) return;
	Rtext_editor_get_cursor(app.editor, &line, &column);
	if (line == app.last_line && column == app.last_column) return;
	app.last_line = line;
	app.last_column = column;
	snprintf(text, sizeof(text), "Line %d, column %d", line + 1, column + 1);
	Rlabel_set_text(app.position, text);
}

/* ---------------------------------------------------------------------------
 *  Languages
 * ------------------------------------------------------------------------- */

/* A language of our own: the editor only needs to be told what its words and comments look like. */
static void register_ini(void) {
	RLanguageDesc ini;
	memset(&ini, 0, sizeof(ini));
	ini.name = "INI";
	ini.extensions = "ini;cfg;conf";
	ini.constants = "true false yes no on off";
	ini.line_comment = ";";
	ini.string_quotes = "\"";
	ini.flags = R_LANGUAGE_IGNORE_CASE;
	Rlanguage_register(&ini);
}

static void fill_languages(void) {
	int i;
	app.loading++;
	Rcombo_box_clear_items(app.language);
	Rcombo_box_add_item(app.language, "Plain text");
	for (i = 0; i < Rlanguage_get_count(); i++) Rcombo_box_add_item(app.language, Rlanguage_get_name(i));
	Rcombo_box_set_selected(app.language, 0);
	app.loading--;
}

static void set_language(const char* name) {
	int i;
	Rtext_editor_set_language(app.editor, name);
	app.loading++;
	Rcombo_box_set_selected(app.language, 0);
	for (i = 0; name && i < Rlanguage_get_count(); i++) {
		if (strcmp(Rlanguage_get_name(i), name) == 0) Rcombo_box_set_selected(app.language, i + 1);
	}
	app.loading--;
}

static void action_language(Rid sender, const char* action, void* user_data) {
	int index = Rcombo_box_get_selected(app.language);
	(void)sender;
	(void)action;
	(void)user_data;
	if (app.loading) return;
	Rtext_editor_set_language(app.editor, index > 0 ? Rlanguage_get_name(index - 1) : NULL);
	Rwidget_focus(app.editor);
}

/*
 * Proposals of the application, listed before those of the editor (the words
 * of the language and the names already in the text): here, a few snippets.
 * A language server would answer in the same place.
 */
static void on_complete(Rid editor, const char* prefix, void* user_data) {
	static const struct { const char* language; const char* text; const char* detail; } snippets[] = {
		{ "C", "printf(\"%d\\n\", );", "snippet" }, { "C", "for (int i = 0; i < n; i++) {", "snippet" },
		{ "C", "#include <stdio.h>", "snippet" }, { "C++", "std::cout << ", "snippet" },
		{ "Python", "print(f\"{}\")", "snippet" }, { "Python", "if __name__ == \"__main__\":", "snippet" },
		{ "Rust", "println!(\"{}\", );", "snippet" }, { "JavaScript", "console.log();", "snippet" },
		{ "HTML", "<!DOCTYPE html>", "snippet" }, { "Java", "System.out.println();", "snippet" }
	};
	const char* language = Rtext_editor_get_language(editor);
	size_t i;
	(void)user_data;
	(void)prefix;
	/* (the editor only keeps those that start with what is typed) */
	for (i = 0; i < sizeof(snippets) / sizeof(snippets[0]); i++) {
		if (strcmp(snippets[i].language, language) == 0) Rtext_editor_add_completion(editor, snippets[i].text, snippets[i].detail);
	}
	Rtext_editor_add_completion(editor, "TODO", "note");
	Rtext_editor_add_completion(editor, "FIXME", "note");
}

/* ---------------------------------------------------------------------------
 *  Markers: the notes left in the text
 * ------------------------------------------------------------------------- */

static void scan_notes(Rid timer, void* user_data) {
	int count = Rtext_editor_get_line_count(app.editor), line, found = 0;
	char text[64];
	(void)timer;
	(void)user_data;
	app.scan_timer = R_INVALID_ID;
	Rtext_editor_clear_markers(app.editor);
	for (line = 0; line < count && found < 200; line++) {
		const char* content = Rtext_editor_get_line(app.editor, line);
		const char* todo = content ? strstr(content, "TODO") : NULL;
		const char* fixme = content ? strstr(content, "FIXME") : NULL;
		if (fixme) {
			Rtext_editor_add_marker(app.editor, line, (int)(fixme - content), -1, R_MARKER_WARNING, "Something to fix");
			found++;
		} else if (todo) {
			Rtext_editor_add_marker(app.editor, line, (int)(todo - content), -1, R_MARKER_INFO, "Something left to do");
			found++;
		}
	}
	if (found) snprintf(text, sizeof(text), "%d note%s", found, found > 1 ? "s" : "");
	else text[0] = '\0';
	Rlabel_set_text(app.notes, text);
}

static void on_changed(Rid editor, void* user_data) {
	(void)editor;
	(void)user_data;
	if (!app.modified) {
		app.modified = R_TRUE;
		update_title();
	}
	/* scanned once the typing pauses */
	if (app.scan_timer != R_INVALID_ID && Rtimer_is_valid(app.scan_timer)) Rtimer_destroy(app.scan_timer);
	app.scan_timer = Rtimer_create(0.4, R_FALSE, scan_notes, NULL);
}

/* ---------------------------------------------------------------------------
 *  Files
 * ------------------------------------------------------------------------- */

#ifndef __EMSCRIPTEN__
static const RFileFilter g_filters[] = {
	{ "Text and code", "*.txt;*.md;*.c;*.h;*.cpp;*.hpp;*.cs;*.java;*.js;*.ts;*.py;*.rs;*.html;*.css;*.xml;*.json;*.ini" },
	{ "All files", "*" }
};
#endif

/* R_TRUE when the text can be replaced: nothing to lose, or the user agreed to lose it. */
static int can_discard(void) {
	if (!app.modified) return R_TRUE;
#ifdef __EMSCRIPTEN__
	return EM_ASM_INT({ return window.confirm("The text has changes that were not saved. Discard them?"); });
#else
	return Rmessage_box(app.window, "Unsaved changes", "The text has changes that were not saved. Discard them?", R_MESSAGE_OK_CANCEL)
		== R_MESSAGE_RESULT_OK;
#endif
}

static void open_path(const char* path) {
	size_t size = 0;
	char* text = Rfile_read(path, &size);
	if (!text) {
		set_status("Cannot read %s", path);
		return;
	}
	Rtext_editor_set_text(app.editor, text);
	Rmemory_free(text);
	snprintf(app.path, sizeof(app.path), "%s", path);
	app.modified = R_FALSE;
	/* "main.rs" is Rust, "notes.txt" is nothing in particular */
	set_language(Rlanguage_for_file(path));
	update_title();
	scan_notes(R_INVALID_ID, NULL);
	set_status("Opened %s", path);
	Rwidget_focus(app.editor);
}

#ifdef __EMSCRIPTEN__
static void imported(int success, const char* path, void* user) {
	(void)user;
	if (success && can_discard()) open_path(path);
}
#endif

static int save(int ask) {
	const char* text;
	char target[sizeof(app.path)];
	snprintf(target, sizeof(target), "%s", app.path);
	if (ask || !target[0]) {
#ifdef __EMSCRIPTEN__
		char* chosen = (char*)EM_ASM_PTR({
			const name = window.prompt('Save the text as', UTF8ToString($0));
			return name ? stringToNewUTF8(name) : 0;
		}, app.path[0] ? file_name(app.path) : "untitled.txt");
		if (!chosen) return R_FALSE;
		snprintf(target, sizeof(target), "/%s", file_name(chosen));
		free(chosen);
#else
		RFileDialogDesc desc;
		const char* chosen;
		memset(&desc, 0, sizeof(desc));
		desc.parent = app.window;
		desc.title = "Save the text";
		desc.file_name = app.path[0] ? file_name(app.path) : "untitled.txt";
		desc.filters = g_filters;
		desc.filter_count = 2;
		desc.default_extension = "txt";
		chosen = Rfile_dialog_save(&desc);
		if (!chosen) return R_FALSE;
		snprintf(target, sizeof(target), "%s", chosen);
#endif
	}
	text = Rtext_editor_get_text(app.editor);
	if (!Rfile_write(target, text, (size_t)Rtext_editor_get_length(app.editor))) {
		set_status("Cannot write %s", target);
		return R_FALSE;
	}
#ifdef __EMSCRIPTEN__
	if (!Rweb_download_file(target, file_name(target))) {
		set_status("Cannot download %s", target);
		return R_FALSE;
	}
#endif
	/* only once it is written: a failed Save As leaves the text with the file it had */
	if (strcmp(target, app.path) != 0) {
		memcpy(app.path, target, sizeof(app.path));
		set_language(Rlanguage_for_file(app.path));
	}
	app.modified = R_FALSE;
	update_title();
	set_status("Saved %s", app.path);
	return R_TRUE;
}

static void action_file(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)user_data;
	if (strcmp(action, "file.new") == 0) {
		if (!can_discard()) return;
		Rtext_editor_set_text(app.editor, "");
		app.path[0] = '\0';
		app.modified = R_FALSE;
		set_language(NULL);
		update_title();
		scan_notes(R_INVALID_ID, NULL);
		set_status("%s", "New text.");
	} else if (strcmp(action, "file.open") == 0) {
#ifdef __EMSCRIPTEN__
		Rweb_import_file("", imported, NULL);
#else
		RFileDialogDesc desc;
		const char* chosen;
		if (!can_discard()) return;
		memset(&desc, 0, sizeof(desc));
		desc.parent = app.window;
		desc.title = "Open a text";
		desc.filters = g_filters;
		desc.filter_count = 2;
		chosen = Rfile_dialog_open(&desc);
		if (chosen) open_path(chosen);
#endif
	} else if (strcmp(action, "file.save") == 0) {
		save(R_FALSE);
	} else if (strcmp(action, "file.save_as") == 0) {
		save(R_TRUE);
	} else if (strcmp(action, "file.quit") == 0) {
		if (can_discard()) Rquit(0);
	}
}

/* The close button of the header bar, Alt+F4: the same question. */
static int on_close(Rid window, void* user_data) {
	(void)window;
	(void)user_data;
	if (!can_discard()) return R_FALSE;
	Rquit(0);
	return R_TRUE;
}

/*
 * Something is dragged over the window from another application. Returning
 * R_TRUE while it moves says "I take it" (the window is then highlighted);
 * the files are known when it is dropped.
 */
static int on_drop(Rid target, const RDropEvent* event, void* user_data) {
	(void)target;
	(void)user_data;
	if (event->phase != R_DROP_DROPPED) return R_TRUE;
	if (event->path_count > 0) {
		if (can_discard()) open_path(event->paths[0]);
	} else if (event->text) {
		Rtext_editor_insert_text(app.editor, event->text);
	}
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Edit, view
 * ------------------------------------------------------------------------- */

static void action_edit(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)user_data;
	Rwidget_focus(app.editor);
	if (strcmp(action, "edit.undo") == 0) Rtext_editor_undo(app.editor);
	else if (strcmp(action, "edit.redo") == 0) Rtext_editor_redo(app.editor);
	else if (strcmp(action, "edit.select_all") == 0) Rtext_editor_select_all(app.editor);
	else if (strcmp(action, "edit.complete") == 0) Rtext_editor_show_completions(app.editor);
}

static void action_view(Rid sender, const char* action, void* user_data) {
	(void)user_data;
	if (strcmp(action, "view.numbers") == 0) Rtext_editor_set_line_numbers(app.editor, Rmenu_item_is_checked(sender));
	else if (strcmp(action, "view.wrap") == 0) Rtext_editor_set_word_wrap(app.editor, Rmenu_item_is_checked(sender));
	else if (strcmp(action, "view.dark") == 0) Rtheme_apply(R_THEME_DARK);
	else if (strcmp(action, "view.light") == 0) Rtheme_apply(R_THEME_LIGHT);
	else if (strcmp(action, "view.glass_dark") == 0) Rtheme_apply(R_THEME_GLASS_DARK);
	else if (strcmp(action, "view.glass_light") == 0) Rtheme_apply(R_THEME_GLASS_LIGHT);
}

/* ---------------------------------------------------------------------------
 *  Start
 * ------------------------------------------------------------------------- */

static const char WELCOME[] =
	"/* Raven Notepad: this text is C, so the editor colors it. */\n"
	"#include <stdio.h>\n"
	"\n"
	"/* TODO: type \"pri\" on an empty line and choose a proposal with Enter */\n"
	"static int square(int value) {\n"
	"\treturn value * value;\n"
	"}\n"
	"\n"
	"int main(void) {\n"
	"\tfor (int i = 0; i < 4; i++) {\n"
	"\t\tprintf(\"%d squared is %d\\n\", i, square(i));\n"
	"\t}\n"
	"\treturn 0; /* FIXME: put the cursor after a brace to see the one it closes */\n"
	"}\n";

int main(int argc, char** argv) {
	static const char* const file_actions[] = { "file.new", "file.open", "file.save", "file.save_as", "file.quit" };
	static const char* const edit_actions[] = { "edit.undo", "edit.redo", "edit.select_all", "edit.complete" };
	static const char* const view_actions[] = { "view.numbers", "view.wrap", "view.dark", "view.light", "view.glass_dark", "view.glass_light" };
	size_t i;

	if (!example_init("notepad")) return 1;
	register_ini();

	for (i = 0; i < sizeof(file_actions) / sizeof(file_actions[0]); i++) Raction_bind(file_actions[i], action_file, NULL);
	for (i = 0; i < sizeof(edit_actions) / sizeof(edit_actions[0]); i++) Raction_bind(edit_actions[i], action_edit, NULL);
	for (i = 0; i < sizeof(view_actions) / sizeof(view_actions[0]); i++) Raction_bind(view_actions[i], action_view, NULL);
	Raction_bind("view.language", action_language, NULL);

	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/notepad.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.editor = Rwidget_find(app.window, "editor");
	app.header = Rwidget_find(app.window, "header");
	app.language = Rwidget_find(app.window, "language");
	app.status = Rwidget_find(app.window, "status");
	app.notes = Rwidget_find(app.window, "notes");
	app.position = Rwidget_find(app.window, "position");
	app.scan_timer = R_INVALID_ID;
	app.last_line = -1;

	fill_languages();
	Rtext_editor_set_changed_callback(app.editor, on_changed, NULL);
	Rtext_editor_set_completion_callback(app.editor, on_complete, NULL);
	Rwindow_set_close_callback(app.window, on_close, NULL);
	Rwindow_set_drop_callback(app.window, on_drop, NULL);
	Rset_frame_callback(on_frame, NULL);

	if (argc > 1 && argv[1][0] != '-') {
		open_path(argv[1]);
	} else {
		Rtext_editor_set_text(app.editor, WELCOME);
		set_language("C");
		scan_notes(R_INVALID_ID, NULL);
		set_status("%s", "Drop a file on the window to open it.");
	}
	update_title();
	Rwidget_focus(app.editor);

	example_run(app.window, NULL, NULL);
	return 0;
}
