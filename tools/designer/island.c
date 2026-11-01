/*
 * Raven Designer - island.c
 * The island at the top of the window (raven_shell.h): the command center
 * of the designer.
 *
 * Folded, a capsule hanging from the top edge: a dot (gray: nothing open,
 * green: the interface builds without a problem, orange: it has problems),
 * the file and whether it has unsaved changes, the element selected, and
 * the number of problems.
 *
 * Unfolded (a click, or Ctrl+K from anywhere):
 *   - one search for everything: a widget to add inside the selection, an
 *     interface of the folder to open, a command (save, run, undo, wrap in,
 *     move, new...), a language or a theme for the preview. Up and Down
 *     choose, Enter does it, Escape folds the island;
 *   - the selection: where it is, its Name and its Text, edited in place,
 *     and what is done most to it (duplicate, wrap, delete, run);
 *   - the look of the designer, and the theme and the language of the
 *     preview (main.c and strings.c fill and follow these lists);
 *   - the first problem, and the way to it.
 */

#include "designer.h"
#include "../crash_report.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RESULTS 40

static const char ISLAND_XML[] =
"<Island Name=\"island\" Attached=\"true\" ExpandOnHover=\"true\">"
"  <HBox Spacing=\"8\" Slot.Padding=\"14,5\">"
"    <Border Name=\"island.dot\" Width=\"8\" Height=\"8\" CornerRadius=\"4\" Background=\"#8E8E93\" Slot.VAlign=\"Center\" AccessibleHidden=\"true\"/>"
"    <Label Name=\"island.title\" Text=\"Raven Designer\" Slot.VAlign=\"Center\"/>"
"    <Label Name=\"island.selection\" Opacity=\"0.6\" Slot.VAlign=\"Center\" Visibility=\"Collapsed\"/>"
"    <Label Name=\"island.state\" Opacity=\"0.6\" Slot.VAlign=\"Center\" Visibility=\"Collapsed\"/>"
"    <Label Text=\"Ctrl+K\" TextSize=\"11\" Opacity=\"0.4\" Slot.VAlign=\"Center\"/>"
"  </HBox>"
"  <VBox Name=\"island.body\" Spacing=\"10\" Slot.Padding=\"16,14\" Width=\"480\">"
"    <TextInput Name=\"island.search\" Placeholder=\"Add a widget, open a file, run a command...\"/>"
"    <ListView Name=\"island.results\" Height=\"196\"/>"
"    <Separator/>"
"    <VBox Name=\"island.selected\" Spacing=\"8\">"
"      <Label Name=\"island.where\" Opacity=\"0.55\" TextSize=\"11\"/>"
"      <Grid Columns=\"auto,1\" Spacing=\"8\">"
"        <Label Text=\"Name\" Slot.VAlign=\"Center\"/>"
"        <TextInput Name=\"island.name\" Placeholder=\"For Rwidget_find, in the code\" Slot.Column=\"1\"/>"
"        <Label Text=\"Text\" Slot.Row=\"1\" Slot.VAlign=\"Center\"/>"
"        <TextInput Name=\"island.text\" Placeholder=\"What it shows (@key: a translation)\" Slot.Row=\"1\" Slot.Column=\"1\"/>"
"      </Grid>"
"      <HBox Spacing=\"6\">"
"        <Button Text=\"Duplicate\" Style=\"Flat\" OnClicked=\"edit.duplicate\" Tooltip=\"Ctrl+D\"/>"
"        <Button Text=\"Wrap in a VBox\" Style=\"Flat\" OnClicked=\"edit.wrap.VBox\"/>"
"        <Button Text=\"Delete\" Style=\"Flat\" OnClicked=\"edit.delete\" Tooltip=\"Delete\"/>"
"        <Spacer Slot.Fill=\"1\"/>"
"        <Button Text=\"Run\" Style=\"Primary\" OnClicked=\"view.run\" Tooltip=\"The interface in a window of its own (F7)\"/>"
"      </HBox>"
"    </VBox>"
"    <Separator/>"
"    <Label Text=\"LOOK AND LANGUAGE\" TextSize=\"11\" Opacity=\"0.55\"/>"
"    <Grid Columns=\"auto,1\" Spacing=\"8\">"
"      <Label Text=\"Designer\" Slot.VAlign=\"Center\"/>"
"      <ComboBox Name=\"appearance\" Slot.Column=\"1\" Tooltip=\"The look of the designer itself: any look of Raven, or a theme of the folder\"/>"
"      <Label Text=\"Preview\" Slot.Row=\"1\" Slot.VAlign=\"Center\"/>"
"      <ComboBox Name=\"preview.theme\" Slot.Row=\"1\" Slot.Column=\"1\" Tooltip=\"The look or the theme the preview wears: the themes of the folder are listed too\"/>"
"      <Label Text=\"Language\" Slot.Row=\"2\" Slot.VAlign=\"Center\"/>"
"      <ComboBox Name=\"preview.language\" Slot.Row=\"2\" Slot.Column=\"1\" Tooltip=\"The language of the preview: the tables of the folder, right to left included\"/>"
"    </Grid>"
"    <HBox Name=\"island.problem.row\" Spacing=\"8\">"
"      <Border Width=\"8\" Height=\"8\" CornerRadius=\"4\" Background=\"#FF9F0A\" Slot.VAlign=\"Center\" AccessibleHidden=\"true\"/>"
"      <Label Name=\"island.problem\" Wrap=\"true\" Slot.Fill=\"1\" Slot.VAlign=\"Center\" TextSize=\"12\"/>"
"      <Button Text=\"Show\" Style=\"Flat\" OnClicked=\"island.problems\" Slot.VAlign=\"Center\"/>"
"    </HBox>"
"  </VBox>"
"</Island>";

typedef enum result_kind_t { RESULT_WIDGET, RESULT_FILE, RESULT_ACTION, RESULT_LANGUAGE, RESULT_THEME } result_kind_t;

typedef struct result_t {
	result_kind_t kind;
	char text[200];
	/* the tag, the path, the action, the code of the language, the name of the theme */
	char what[1024];
} result_t;

/* The commands the search finds: what is said, what is fired, other words that find it. */
static const struct { const char* text; const char* action; const char* words; } COMMANDS[] = {
	{ "Save", "file.save", "write ctrl+s" },
	{ "Save as...", "file.save_as", "write copy" },
	{ "Preview in a window", "view.run", "f7 test try launch start run live" },
	{ "Use the preview", "view.interact", "f6 interact click try" },
	{ "Undo", "edit.undo", "back ctrl+z" },
	{ "Redo", "edit.redo", "again ctrl+y" },
	{ "Duplicate the selection", "edit.duplicate", "copy clone ctrl+d" },
	{ "Delete the selection", "edit.delete", "remove erase" },
	{ "Move up", "edit.up", "before order" },
	{ "Move down", "edit.down", "after order" },
	{ "Move out of its parent", "edit.out", "parent level" },
	{ "Move into the element before", "edit.in", "child level" },
	{ "Wrap in a VBox", "edit.wrap.VBox", "column vertical box" },
	{ "Wrap in an HBox", "edit.wrap.HBox", "row horizontal box" },
	{ "Wrap in a Border", "edit.wrap.Border", "frame background padding" },
	{ "Wrap in a ScrollBox", "edit.wrap.ScrollBox", "scroll" },
	{ "Wrap in an Overlay", "edit.wrap.Overlay", "stack layer" },
	{ "Unwrap", "edit.unwrap", "remove parent flatten" },
	{ "Select nothing", "edit.deselect", "deselect none clear escape" },
	{ "Copy as XML", "edit.copy", "clipboard" },
	{ "Paste XML inside", "edit.paste", "clipboard insert" },
	{ "New window", "files.new.window", "interface create file" },
	{ "New panel", "files.new.panel", "interface create file" },
	{ "New form", "files.new.form", "interface create file declared widget" },
	{ "Open a folder...", "folder.open", "project directory" },
	{ "Open an interface...", "file.open", "file xml" },
	{ "Start page", "file.welcome", "recent welcome home" },
	{ "Window tab", "tab.window", "windows interfaces ctrl+1" },
	{ "Widget tab", "tab.widget", "declared tags library components ctrl+2" },
	{ "Theme tab", "tab.theme", "themes colors editor ctrl+3" },
	{ "Translations tab", "tab.strings", "csv languages strings locale ctrl+4" },
	{ "New widget library", "files.new.widgets", "declared tags component create" },
	{ "New theme", "files.new.theme", "colors create" },
	{ "Problems", "island.problems", "errors warnings mistakes" },
	{ "Reset the panels", "view.reset", "layout dock" },
	{ "Accessibility...", "view.accessibility", "text size contrast motion" },
	{ "Designer in dark", "view.theme.dark", "look appearance night" },
	{ "Designer in light", "view.theme.light", "look appearance day" }
};
#define COMMAND_COUNT ((int)(sizeof(COMMANDS) / sizeof(COMMANDS[0])))

static struct {
	Rid island, timer, search, results, name, text;
	result_t results_list[MAX_RESULTS];
	int result_count;
	int syncing;
} g_i;

static Rid find(const char* name) {
	return Rwidget_find(g_i.island, name);
}

/* (a text is set only when it changes: no layout for nothing, four times a second) */
static void set_text(const char* name, const char* text) {
	Rid label = find(name);
	const char* now = Rlabel_get_text(label);
	if (!now || strcmp(now, text) != 0) Rlabel_set_text(label, text);
}

static void set_shown(const char* name, int shown) {
	Rwidget_set_visibility(find(name), shown ? R_VISIBLE : R_COLLAPSED);
}

/* ---------------------------------------------------------------------------
 *  The search
 * ------------------------------------------------------------------------- */

static void add_result(result_kind_t kind, const char* text, const char* what) {
	result_t* r;
	if (g_i.result_count >= MAX_RESULTS) return;
	r = &g_i.results_list[g_i.result_count++];
	r->kind = kind;
	snprintf(r->text, sizeof(r->text), "%s", text);
	snprintf(r->what, sizeof(r->what), "%s", what);
}

/* Every word of the query is in one of the texts. */
static int matches(const char* query, const char* a, const char* b) {
	char word[64];
	const char* walk = query;
	for (;;) {
		size_t length;
		while (*walk == ' ') walk++;
		length = strcspn(walk, " ");
		if (!length) break;
		snprintf(word, sizeof(word), "%.*s", (int)(length < sizeof(word) - 1 ? length : sizeof(word) - 1), walk);
		if (!ds_contains(a, word) && !(b && ds_contains(b, word))) return 0;
		walk += length;
	}
	return 1;
}

static int starts_with(const char* text, const char* start) {
	for (; *start; text++, start++) {
		char x = *text, y = *start;
		if (!x) return 0;
		if (x >= 'A' && x <= 'Z') x = (char)(x + 32);
		if (y >= 'A' && y <= 'Z') y = (char)(y + 32);
		if (x != y) return 0;
	}
	return 1;
}

static void fill_results(void) {
	const char* query = Rtext_input_get_text(g_i.search);
	char text[300];
	int i, count;
	while (*query == ' ') query++;
	g_i.result_count = 0;
	if (!query[0]) {
		/* nothing typed: what is done most */
		static const int first[] = { 2, 0, 4, 5, 12 };
		static const char* const common[] = { "Button", "Label", "TextInput", "VBox", "HBox", "Checkbox", "Image" };
		for (i = 0; i < (int)(sizeof(first) / sizeof(first[0])); i++) add_result(RESULT_ACTION, COMMANDS[first[i]].text, COMMANDS[first[i]].action);
		for (i = 0; i < (int)(sizeof(common) / sizeof(common[0])); i++) {
			snprintf(text, sizeof(text), "Add %s", common[i]);
			add_result(RESULT_WIDGET, text, common[i]);
		}
	} else {
		/* "add ..." and "open ..." only say which kind */
		int want_add = starts_with(query, "add ") || ds_iequal(query, "add");
		int want_open = starts_with(query, "open ") || ds_iequal(query, "open");
		const char* words = want_add ? query + 3 : want_open ? query + 4 : query;
		int pass;
		while (*words == ' ') words++;
		count = designer_widget_count();
		/* widgets: those whose name starts with what is typed first, then those that speak of it */
		for (pass = 0; pass < 2 && !want_open; pass++) {
			for (i = 0; i < count; i++) {
				const char* tag = designer_widget_at(i);
				int starts = words[0] && starts_with(tag, words);
				if (pass == 0 ? !starts : (starts || !matches(words, tag, Rxml_get_tag_description(tag)))) continue;
				snprintf(text, sizeof(text), "Add %s", tag);
				add_result(RESULT_WIDGET, text, tag);
			}
		}
		if (!want_add) {
			count = folder_interface_count();
			for (i = 0; i < count; i++) {
				const char* path = folder_interface_at(i);
				if (!matches(words, ds_basename(path), "interface file")) continue;
				snprintf(text, sizeof(text), "Open %s", ds_basename(path));
				add_result(RESULT_FILE, text, path);
			}
		}
		if (!want_add && !want_open) {
			for (i = 0; i < COMMAND_COUNT; i++) {
				if (matches(words, COMMANDS[i].text, COMMANDS[i].words)) add_result(RESULT_ACTION, COMMANDS[i].text, COMMANDS[i].action);
			}
			count = strings_language_count();
			for (i = 0; i < count; i++) {
				const char* code = strings_language_at(i);
				snprintf(text, sizeof(text), "Preview in %s", code);
				if (matches(words, text, "language translation")) add_result(RESULT_LANGUAGE, text, code);
			}
			count = folder_theme_count();
			for (i = 0; i < count; i++) {
				const char* name = folder_theme_name(i);
				snprintf(text, sizeof(text), "Preview with the theme %s", name);
				if (matches(words, text, "look colors")) add_result(RESULT_THEME, text, name);
			}
			if (matches(words, "Preview with the look of the designer", "theme default")) add_result(RESULT_THEME, "Preview with the look of the designer", "");
		}
	}
	g_i.syncing++;
	Rlist_view_clear_items(g_i.results);
	for (i = 0; i < g_i.result_count; i++) {
		static const char* const kinds[] = { "widget", "file", "command", "language", "theme" };
		snprintf(text, sizeof(text), "%s      %s", g_i.results_list[i].text, kinds[g_i.results_list[i].kind]);
		Rlist_view_add_item(g_i.results, text);
	}
	if (g_i.result_count == 0) Rlist_view_add_item(g_i.results, "Nothing matches.");
	else Rlist_view_set_selected(g_i.results, 0);
	g_i.syncing--;
}

static void run_result(int index) {
	result_t r;
	if (index < 0 || index >= g_i.result_count) return;
	r = g_i.results_list[index];
	/* folded first: what is done may open a dialog or another file */
	g_i.syncing++;
	Rtext_input_set_text(g_i.search, "");
	g_i.syncing--;
	Risland_set_expanded(g_i.island, R_FALSE);
	switch (r.kind) {
		case RESULT_WIDGET:
			designer_add_widget(r.what);
			break;
		case RESULT_FILE:
			if (designer_get_path() && ds_same_path(designer_get_path(), r.what)) break;
			if (ds_confirm_unsaved("opening another interface")) designer_open_file(r.what);
			break;
		case RESULT_ACTION:
			Raction_fire(r.what, R_INVALID_ID);
			break;
		case RESULT_LANGUAGE:
			strings_set_preview_language(r.what);
			ds_status("The preview is in %s.", r.what);
			break;
		case RESULT_THEME:
			designer_set_preview_theme(r.what);
			if (r.what[0]) ds_status("The preview wears the theme %s.", r.what);
			else ds_status("The preview wears the look of the designer.");
			break;
	}
	fill_results();
	island_refresh();
}

static void on_search(Rid input, const char* text, void* user_data) {
	(void)input;
	(void)text;
	(void)user_data;
	if (!g_i.syncing) fill_results();
}

static void on_search_committed(Rid input, const char* text, void* user_data) {
	int selected = Rlist_view_get_selected(g_i.results);
	(void)input;
	(void)text;
	(void)user_data;
	run_result(selected >= 0 ? selected : 0);
}

/* Up and Down choose a result while the keyboard is in the search; Escape folds. */
static int search_event(Rid widget, const REvent* event, void* user_data) {
	int selected;
	(void)widget;
	(void)user_data;
	if (event->type != R_EVENT_KEY_DOWN) return R_FALSE;
	selected = Rlist_view_get_selected(g_i.results);
	if (event->key == R_KEY_DOWN || event->key == R_KEY_UP) {
		if (g_i.result_count == 0) return R_TRUE;
		selected += event->key == R_KEY_DOWN ? 1 : -1;
		if (selected < 0) selected = 0;
		if (selected >= g_i.result_count) selected = g_i.result_count - 1;
		Rlist_view_set_selected(g_i.results, selected);
		Rlist_view_scroll_to_item(g_i.results, selected);
		return R_TRUE;
	}
	if (event->key == R_KEY_ESCAPE) {
		Risland_set_expanded(g_i.island, R_FALSE);
		return R_TRUE;
	}
	return R_FALSE;
}

static void on_result_activated(Rid list, int index, void* user_data) {
	(void)list;
	(void)user_data;
	run_result(index);
}

/* ---------------------------------------------------------------------------
 *  The selection
 * ------------------------------------------------------------------------- */

static void on_field(Rid input, const char* text, void* user_data) {
	(void)input;
	if (g_i.syncing || designer_get_selected() == R_INVALID_ID) return;
	designer_set_attribute((const char*)user_data, text[0] ? text : NULL);
}

/* The fields follow the selection (not while one is being typed in). */
static void update_fields(void) {
	Rid node = designer_get_selected();
	Rid focus = Rwindow_get_focused_widget(g_ds_window);
	int has = node != R_INVALID_ID && Rxml_node_is_valid(node) && !Rxml_node_is_comment(node);
	char where[400];
	set_shown("island.selected", designer_has_document());
	if (!designer_has_document()) return;
	designer_selection_path(where, sizeof(where));
	set_text("island.where", has ? where : "Nothing selected: click an element of the preview");
	Rwidget_set_enabled(g_i.name, has);
	Rwidget_set_enabled(g_i.text, has);
	g_i.syncing++;
	if (focus != g_i.name) {
		const char* value = has ? Rxml_node_get_attribute(node, "Name") : NULL;
		if (strcmp(Rtext_input_get_text(g_i.name), value ? value : "")) Rtext_input_set_text(g_i.name, value ? value : "");
	}
	if (focus != g_i.text) {
		const char* value = has ? Rxml_node_get_attribute(node, "Text") : NULL;
		if (strcmp(Rtext_input_get_text(g_i.text), value ? value : "")) Rtext_input_set_text(g_i.text, value ? value : "");
	}
	g_i.syncing--;
}

/* ---------------------------------------------------------------------------
 *  What is going on
 * ------------------------------------------------------------------------- */

static void refresh(void) {
	const char* path = designer_get_path();
	int open = designer_has_document();
	int problems = open ? designer_problem_count() : 0;
	char title[300], state[64], selection[160];
	Rid node = designer_get_selected();

	if (open) snprintf(title, sizeof(title), "%s%s", path ? ds_basename(path) : "Untitled", designer_is_modified() || strings_any_modified() ? " \xE2\x80\xA2" : "");
	else if (g_folder) snprintf(title, sizeof(title), "%s", ds_basename(g_folder));
	else snprintf(title, sizeof(title), "Raven Designer");
	set_text("island.title", title);

	selection[0] = '\0';
	if (open && node != R_INVALID_ID && Rxml_node_is_valid(node) && !Rxml_node_is_comment(node)) {
		const char* name = Rxml_node_get_attribute(node, "Name");
		snprintf(selection, sizeof(selection), "\xC2\xB7  %s%s%s", Rxml_node_get_tag(node), name ? "  " : "", name ? name : "");
	}
	set_text("island.selection", selection);
	set_shown("island.selection", selection[0] != '\0');

	state[0] = '\0';
	if (problems) snprintf(state, sizeof(state), "\xC2\xB7  %d problem%s", problems, problems == 1 ? "" : "s");
	set_text("island.state", state);
	set_shown("island.state", state[0] != '\0');
	Rwidget_set_attribute(find("island.dot"), "Background", !open ? "#8E8E93" : problems ? "#FF9F0A" : "#30D158");

	/* (what follows is only seen unfolded) */
	if (!Risland_is_expanded(g_i.island)) return;
	update_fields();
	set_shown("island.problem.row", problems > 0);
	if (problems) set_text("island.problem", designer_problem_text(0));
}

static void on_timer(Rid timer, void* user_data) {
	(void)timer;
	(void)user_data;
	if (g_i.island != R_INVALID_ID && Rwidget_is_valid(g_i.island)) refresh();
}

static void on_changed(Rid island, int expanded, void* user_data) {
	(void)island;
	(void)user_data;
	if (expanded) {
		fill_results();
		refresh();
	}
}

static void action_island(Rid sender, const char* action, void* user_data) {
	crash_report_note("action %s", action);
	(void)sender;
	(void)user_data;
	if (!strcmp(action, "island.problems")) {
		Risland_set_expanded(g_i.island, R_FALSE);
		designer_show_problems();
	} else if (!strcmp(action, "island.search")) {
		island_open_search();
	}
}

void island_init(void) {
	memset(&g_i, 0, sizeof(g_i));
	g_i.timer = R_INVALID_ID;
	g_i.island = Rwidget_create_from_xml(ISLAND_XML, sizeof(ISLAND_XML) - 1);
	if (g_i.island == R_INVALID_ID) return;
	Rwindow_set_island(g_ds_window, g_i.island);
	g_i.search = find("island.search");
	g_i.results = find("island.results");
	g_i.name = find("island.name");
	g_i.text = find("island.text");
	Rtext_input_set_changed_callback(g_i.search, on_search, NULL);
	Rtext_input_set_committed_callback(g_i.search, on_search_committed, NULL);
	Rwidget_set_event_callback(g_i.search, search_event, NULL);
	Rlist_view_set_activated_callback(g_i.results, on_result_activated, NULL);
	Rtext_input_set_changed_callback(g_i.name, on_field, (void*)"Name");
	Rtext_input_set_changed_callback(g_i.text, on_field, (void*)"Text");
	Raction_bind("island.problems", action_island, NULL);
	Raction_bind("island.search", action_island, NULL);
	Risland_set_changed_callback(g_i.island, on_changed, NULL);
	fill_results();
	refresh();
	g_i.timer = Rtimer_create(0.25, R_TRUE, on_timer, NULL);
}

void island_refresh(void) {
	if (g_i.island != R_INVALID_ID && Rwidget_is_valid(g_i.island)) refresh();
}

void island_open_search(void) {
	if (g_i.island == R_INVALID_ID || !Rwidget_is_valid(g_i.island)) return;
	Risland_set_expanded(g_i.island, R_TRUE);
	fill_results();
	refresh();
	Rwidget_focus(g_i.search);
}

int island_run_search(const char* text) {
	if (g_i.island == R_INVALID_ID || !Rwidget_is_valid(g_i.island)) return R_FALSE;
	island_open_search();
	g_i.syncing++;
	Rtext_input_set_text(g_i.search, text);
	g_i.syncing--;
	fill_results();
	if (g_i.result_count == 0) return R_FALSE;
	run_result(0);
	return R_TRUE;
}

const char* island_get_title(void) {
	static char text[600];
	if (g_i.island == R_INVALID_ID || !Rwidget_is_valid(g_i.island)) return "";
	snprintf(text, sizeof(text), "%s %s %s", Rlabel_get_text(find("island.title")), Rlabel_get_text(find("island.selection")), Rlabel_get_text(find("island.state")));
	return text;
}

void island_shutdown(void) {
	if (g_i.timer != R_INVALID_ID && Rtimer_is_valid(g_i.timer)) Rtimer_destroy(g_i.timer);
	g_i.timer = R_INVALID_ID;
}
