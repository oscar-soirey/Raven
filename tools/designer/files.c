/**
 * Raven Designer - files.c
 * The folder: the Files panel shows its interfaces (.xml), its themes (theme
 * files, made with raven_theme_editor) and its translation tables (.csv),
 * in its sub-folders too. A double click opens an interface in the visual
 * editor, a table in the Translations panel; a theme becomes the theme of
 * the preview.
 *
 * The folder is followed live: every second and a half the designer looks
 * at it again. A file that comes or goes shows; a theme file is loaded (and
 * Raven reloads it when it changes, the preview with it); the tables are
 * read again when one changed; the interface open is read again when it was
 * changed by another program and has no unsaved changes here.
 */

#include "designer.h"
#include "../crash_report.h"
#include "../theme_editor/editor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DEPTH 4
#define MAX_FILES 2000

/* (in the order of the tabs: what each one shows in Files) */
typedef enum file_kind_t { FILE_INTERFACE, FILE_WIDGETS, FILE_THEME, FILE_TABLE } file_kind_t;

typedef struct file_t {
	char* path;
	file_kind_t kind;
	double time;
} file_t;

static struct {
	Rid panel, tree, timer, menu;
	file_t* files;
	int file_count;
	/* the themes loaded from the folder: their file and their name */
	char** theme_paths;
	char** theme_names;
	int theme_count;
	/* the file the menu of the tree was opened on */
	char* menu_path;
	/* what the tree shows: the paths behind its items (freed when it is filled again) */
	char** item_paths;
	int item_count, item_cap;
	double tables_time;
	/* the kind of files the tab in front shows */
	int view;
} g_f;

/* ---------------------------------------------------------------------------
 *  Looking at the folder
 * ------------------------------------------------------------------------- */

/* What a .xml is, by its root element: <Theme>, <Widgets> or <Widget> (a library of tags), else an interface. */
static file_kind_t xml_kind(const char* path) {
	size_t size = 0;
	char* text = Rfile_read(path, &size);
	const char* p;
	int theme = FILE_INTERFACE;
	if (!text) return FILE_INTERFACE;
	/* past the declaration and the comments */
	p = text;
	for (;;) {
		while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || (unsigned char)*p == 0xEF || (unsigned char)*p == 0xBB || (unsigned char)*p == 0xBF) p++;
		if (!strncmp(p, "<?", 2)) {
			const char* end = strstr(p, "?>");
			if (!end) break;
			p = end + 2;
		} else if (!strncmp(p, "<!--", 4)) {
			const char* end = strstr(p, "-->");
			if (!end) break;
			p = end + 3;
		} else {
			break;
		}
	}
#define ROOT_IS(word, n) (!strncmp(p, word, n) && (p[n] == ' ' || p[n] == '>' || p[n] == '\t' || p[n] == '\r' || p[n] == '\n' || p[n] == '/'))
	theme = ROOT_IS("<Theme", 6) ? FILE_THEME : (ROOT_IS("<Widgets", 8) || ROOT_IS("<Widget", 7)) ? FILE_WIDGETS : FILE_INTERFACE;
#undef ROOT_IS
	Rmemory_free(text);
	return (file_kind_t)theme;
}

static int skipped_folder(const char* name) {
	return name[0] == '.' || ds_iequal(name, "build") || !strncmp(name, "build-", 6) || ds_iequal(name, "node_modules") ||
		ds_iequal(name, "target") || ds_iequal(name, "bin") || ds_iequal(name, "obj");
}

static void add_file(file_t** files, int* count, const char* path, file_kind_t kind) {
	if (*count >= MAX_FILES) return;
	*files = (file_t*)realloc(*files, sizeof(file_t) * (size_t)(*count + 1));
	(*files)[*count].path = ds_strdup(path);
	(*files)[*count].kind = kind;
	(*files)[*count].time = ds_file_time(path);
	(*count)++;
}

static void scan(const char* folder, int depth, file_t** files, int* count) {
	ds_entry_t* entries;
	int n = ds_list_dir(folder, &entries), i;
	for (i = 0; i < n; i++) {
		char* full;
		if (entries[i].is_dir && (depth >= MAX_DEPTH || skipped_folder(entries[i].name))) continue;
		full = ds_join(folder, entries[i].name);
		if (entries[i].is_dir) {
			scan(full, depth + 1, files, count);
		} else if (ds_iequal(ds_extension(entries[i].name), "xml")) {
			add_file(files, count, full, xml_kind(full));
		} else if (ds_iequal(ds_extension(entries[i].name), "csv")) {
			add_file(files, count, full, FILE_TABLE);
		}
		free(full);
	}
	free(entries);
}

static void free_files(file_t* files, int count) {
	int i;
	for (i = 0; i < count; i++) free(files[i].path);
	free(files);
}

/* ---------------------------------------------------------------------------
 *  The tree
 * ------------------------------------------------------------------------- */

static void remember_path(Rid item, const char* path) {
	if (g_f.item_count == g_f.item_cap) {
		g_f.item_cap = g_f.item_cap ? g_f.item_cap * 2 : 64;
		g_f.item_paths = (char**)realloc(g_f.item_paths, sizeof(char*) * (size_t)g_f.item_cap);
	}
	g_f.item_paths[g_f.item_count] = ds_strdup(path);
	Rtree_item_set_user_handle(item, g_f.item_paths[g_f.item_count]);
	g_f.item_count++;
}

/* The item of a folder (relative to the folder open), made on the way. */
static Rid folder_item(const char* relative_folder, Rid* folders, char** names, int* count) {
	int i;
	Rid parent = R_INVALID_ID;
	char so_far[1024] = "";
	const char* walk = relative_folder;
	while (*walk) {
		const char* slash = strchr(walk, '/');
		size_t length = slash ? (size_t)(slash - walk) : strlen(walk);
		size_t used = strlen(so_far);
		Rid found = R_INVALID_ID;
		snprintf(so_far + used, sizeof(so_far) - used, "%s%.*s", used ? "/" : "", (int)length, walk);
		for (i = 0; i < *count; i++) {
			if (!strcmp(names[i], so_far)) {
				found = folders[i];
				break;
			}
		}
		if (found == R_INVALID_ID && *count < 256) {
			char caption[300];
			snprintf(caption, sizeof(caption), "%.*s", (int)length, walk);
			found = Rtree_view_add_item(g_f.tree, parent, caption);
			Rtree_item_set_expanded(found, R_TRUE);
			folders[*count] = found;
			names[*count] = ds_strdup(so_far);
			(*count)++;
		}
		parent = found;
		if (!slash) break;
		walk = slash + 1;
	}
	return parent;
}

static void fill_tree(void) {
	Rid folders[256];
	char* names[256];
	int folder_count = 0, i;
	size_t root_length = g_folder ? strlen(g_folder) : 0;
	const char* open = designer_get_path();
	for (i = 0; i < g_f.item_count; i++) free(g_f.item_paths[i]);
	g_f.item_count = 0;
	Rtree_view_clear_items(g_f.tree);
	if (!g_folder) return;
	for (i = 0; i < g_f.file_count; i++) {
		const file_t* file = &g_f.files[i];
		char* normalized;
		if ((int)file->kind != g_f.view) continue;
		normalized = ds_normalize(file->path);
		const char* relative = normalized + (strlen(normalized) > root_length ? root_length + 1 : 0);
		const char* slash = strrchr(relative, '/');
		char sub[1024], caption[400];
		Rid parent = R_INVALID_ID, item;
		if (slash) {
			snprintf(sub, sizeof(sub), "%.*s", (int)(slash - relative), relative);
			parent = folder_item(sub, folders, names, &folder_count);
		}
		snprintf(caption, sizeof(caption), "%s", ds_basename(relative));
		item = Rtree_view_add_item(g_f.tree, parent, caption);
		remember_path(item, file->path);
		if (open && ds_same_path(open, file->path)) Rtree_view_set_selected_item(g_f.tree, item);
		free(normalized);
	}
	for (i = 0; i < folder_count; i++) free(names[i]);
	{
		static const char* const titles[4] = { "Windows", "Widget libraries", "Themes", "Tables" };
		char title[64];
		int shown = 0;
		for (i = 0; i < g_f.file_count; i++) shown += (int)g_f.files[i].kind == g_f.view;
		snprintf(title, sizeof(title), "%s (%d)", titles[g_f.view >= 0 && g_f.view < 4 ? g_f.view : 0], shown);
		Rdock_area_set_panel_title(g_ds_dock, g_f.panel, title);
	}
}

/* ---------------------------------------------------------------------------
 *  Themes
 * ------------------------------------------------------------------------- */

static void forget_themes(void) {
	int i;
	for (i = 0; i < g_f.theme_count; i++) {
		free(g_f.theme_paths[i]);
		free(g_f.theme_names[i]);
	}
	free(g_f.theme_paths);
	free(g_f.theme_names);
	g_f.theme_paths = g_f.theme_names = NULL;
	g_f.theme_count = 0;
}

/* The theme files of the folder are loaded (once each: Raven reloads them when they change). */
static int load_themes(void) {
	int i, k, changed = 0;
	for (i = 0; i < g_f.file_count; i++) {
		const char* name;
		int known = 0;
		if (g_f.files[i].kind != FILE_THEME) continue;
		for (k = 0; k < g_f.theme_count && !known; k++) known = ds_same_path(g_f.theme_paths[k], g_f.files[i].path);
		if (known) continue;
		name = Rtheme_load(g_f.files[i].path);
		if (!name) continue;
		g_f.theme_paths = (char**)realloc(g_f.theme_paths, sizeof(char*) * (size_t)(g_f.theme_count + 1));
		g_f.theme_names = (char**)realloc(g_f.theme_names, sizeof(char*) * (size_t)(g_f.theme_count + 1));
		g_f.theme_paths[g_f.theme_count] = ds_strdup(g_f.files[i].path);
		g_f.theme_names[g_f.theme_count] = ds_strdup(name);
		g_f.theme_count++;
		changed = 1;
	}
	/* (a theme whose file went away stays loaded, but is not offered any more) */
	for (k = 0; k < g_f.theme_count; k++) {
		int there = 0;
		for (i = 0; i < g_f.file_count && !there; i++) there = g_f.files[i].kind == FILE_THEME && ds_same_path(g_f.theme_paths[k], g_f.files[i].path);
		if (!there) {
			free(g_f.theme_paths[k]);
			free(g_f.theme_names[k]);
			memmove(&g_f.theme_paths[k], &g_f.theme_paths[k + 1], sizeof(char*) * (size_t)(g_f.theme_count - k - 1));
			memmove(&g_f.theme_names[k], &g_f.theme_names[k + 1], sizeof(char*) * (size_t)(g_f.theme_count - k - 1));
			g_f.theme_count--;
			k--;
			changed = 1;
		}
	}
	return changed;
}

/* ---------------------------------------------------------------------------
 *  Following the folder
 * ------------------------------------------------------------------------- */

static double tables_time(void) {
	double latest = 0.0;
	int i;
	for (i = 0; i < g_f.file_count; i++) {
		if (g_f.files[i].kind == FILE_TABLE && g_f.files[i].time > latest) latest = g_f.files[i].time;
	}
	return latest;
}

static int same_listing(const file_t* a, int a_count, const file_t* b, int b_count) {
	int i;
	if (a_count != b_count) return 0;
	for (i = 0; i < a_count; i++) {
		if (a[i].kind != b[i].kind || strcmp(a[i].path, b[i].path) != 0) return 0;
	}
	return 1;
}

void folder_refresh(void) {
	file_t* files = NULL;
	int count = 0, i, listing_changed, tables = 0, widgets = 0;
	const char* open = designer_get_path();
	double open_time = 0.0, open_was = 0.0;
	if (!g_folder) return;
	scan(g_folder, 0, &files, &count);
	listing_changed = !same_listing(files, count, g_f.files, g_f.file_count);
	/* what changed in the files that are still there */
	for (i = 0; i < count; i++) {
		int k;
		for (k = 0; k < g_f.file_count; k++) {
			if (strcmp(files[i].path, g_f.files[k].path) != 0) continue;
			if (files[i].kind == FILE_TABLE && files[i].time != g_f.files[k].time) tables = 1;
			if (files[i].kind == FILE_WIDGETS && files[i].time != g_f.files[k].time) widgets = 1;
			if (open && ds_same_path(open, files[i].path)) {
				open_time = files[i].time;
				open_was = g_f.files[k].time;
			}
			break;
		}
		if (k == g_f.file_count && files[i].kind == FILE_TABLE) tables = 1;
	}
	if (!listing_changed && !tables && !widgets && open_time == open_was) {
		free_files(files, count);
		return;
	}
	{
		int had_tables = 0;
		for (i = 0; i < g_f.file_count; i++) had_tables |= g_f.files[i].kind == FILE_TABLE;
		free_files(g_f.files, g_f.file_count);
		g_f.files = files;
		g_f.file_count = count;
		/* a table went away */
		if (listing_changed && had_tables) tables = 1;
	}
	if (listing_changed) {
		fill_tree();
		if (load_themes()) designer_themes_changed();
	}
	if (listing_changed || widgets) {
		folder_register_widgets();
		designer_widgets_changed();
	}
	/* (the tables with unsaved changes here are not read over them) */
	if (tables && !strings_any_modified()) strings_reload();
	if (open_time != open_was && open_time > 0.0) designer_file_changed();
	g_f.tables_time = tables_time();
}

static void on_timer(Rid timer, void* user_data) {
	(void)timer;
	(void)user_data;
	folder_refresh();
}

int folder_open(const char* path) {
	char* normalized;
	size_t n;
	if (!path || !ds_is_dir(path)) {
		ds_status("%s is not a folder.", path ? path : "(nothing)");
		return R_FALSE;
	}
	if (!ds_confirm_unsaved("opening another folder")) return R_FALSE;
	normalized = ds_normalize(path);
	n = strlen(normalized);
	while (n > 1 && normalized[n - 1] == '/') normalized[--n] = '\0';
	designer_forget_tabs();
	designer_close();
	free(g_folder);
	g_folder = normalized;
	free_files(g_f.files, g_f.file_count);
	g_f.files = NULL;
	g_f.file_count = 0;
	forget_themes();
	scan(g_folder, 0, &g_f.files, &g_f.file_count);
	fill_tree();
	load_themes();
	folder_register_widgets();
	designer_themes_changed();
	strings_reload();
	g_f.tables_time = tables_time();
	recent_add(g_folder);
	welcome_hide();
	designer_show_tab(0);
	Rdock_area_show_panel(g_ds_dock, g_f.panel);
	designer_update_title();
	/* a window of the folder opens by itself: there is something to look at */
	folder_open_first(0);
	ds_status("%s: %d interface%s, %d theme%s.", ds_basename(g_folder), folder_interface_count(), folder_interface_count() == 1 ? "" : "s",
		g_f.theme_count, g_f.theme_count == 1 ? "" : "s");
	return R_TRUE;
}

void folder_close(void) {
	free(g_folder);
	g_folder = NULL;
	free_files(g_f.files, g_f.file_count);
	g_f.files = NULL;
	g_f.file_count = 0;
	forget_themes();
	designer_themes_changed();
	fill_tree();
	strings_reload();
}

int folder_interface_count(void) {
	int i, count = 0;
	for (i = 0; i < g_f.file_count; i++) count += g_f.files[i].kind == FILE_INTERFACE;
	return count;
}

const char* folder_interface_at(int index) {
	int i;
	for (i = 0; i < g_f.file_count; i++) {
		if (g_f.files[i].kind != FILE_INTERFACE) continue;
		if (index-- == 0) return g_f.files[i].path;
	}
	return NULL;
}

int folder_theme_count(void) {
	return g_f.theme_count;
}

const char* folder_theme_name(int index) {
	return index >= 0 && index < g_f.theme_count ? g_f.theme_names[index] : "";
}

const char* folder_theme_path(int index) {
	return index >= 0 && index < g_f.theme_count ? g_f.theme_paths[index] : "";
}

/* The first file of a kind (main.xml first, then the top of the folder), opened: the tab has something to show. */
int folder_open_first(int view) {
	int best = -1, best_rank = 4;
	size_t n;
	if (!g_folder) return R_FALSE;
	for (n = 0; n < (size_t)g_f.file_count; n++) {
		const char* relative = g_f.files[n].path + strlen(g_folder) + 1;
		int top = !strchr(relative, '/') && !strchr(relative, '\\');
		int rank = (int)g_f.files[n].kind != view ? 4 : top && ds_iequal(ds_basename(relative), "main.xml") ? 0 : top ? 1 : 2;
		if (rank < best_rank) {
			best_rank = rank;
			best = (int)n;
		}
	}
	if (best < 0) return R_FALSE;
	designer_open_file(g_f.files[best].path);
	return R_TRUE;
}

void folder_set_view(int view) {
	if (view < 0 || view > 3) return;
	g_f.view = view;
	fill_tree();
}

void folder_register_widgets(void) {
	int i;
	const char* open = designer_get_path();
	for (i = 0; i < g_f.file_count; i++) {
		size_t size = 0;
		char* text;
		if (g_f.files[i].kind != FILE_WIDGETS) continue;
		/* (the library being edited is made known from its text, not from its file) */
		if (open && designer_get_tab() == 1 && ds_same_path(open, g_f.files[i].path)) continue;
		text = Rfile_read(g_f.files[i].path, &size);
		if (!text) continue;
		Rxml_register_widgets(text, size);
		Rmemory_free(text);
	}
}

/* A theme to start from, written in the folder and opened in the Theme tab. */
static int new_theme(void) {
	char name[64];
	char* path = NULL;
	te_model model;
	te_text text;
	int n;
	for (n = 1; n < 1000; n++) {
		if (n == 1) snprintf(name, sizeof(name), "theme.xml");
		else snprintf(name, sizeof(name), "theme-%d.xml", n);
		path = ds_join(g_folder, name);
		if (!ds_file_exists(path)) break;
		free(path);
		path = NULL;
	}
	if (!path) return R_FALSE;
	te_model_init(&model);
	model.accent_set = 1;
	model.accent[0] = 0.49f;
	model.accent[1] = 0.36f;
	model.accent[2] = 1.0f;
	snprintf(model.name, sizeof(model.name), "%s", n == 1 ? "My theme" : name);
	memset(&text, 0, sizeof(text));
	te_model_write(&model, &text, NULL, 0, NULL);
	te_model_free(&model);
	if (!Rfile_write(path, text.data, text.length)) {
		te_text_free(&text);
		free(path);
		return R_FALSE;
	}
	te_text_free(&text);
	folder_refresh();
	designer_show_tab(2);
	designer_open_theme(path);
	ds_status("%s made: change it here, the windows of the folder can wear it.", name);
	free(path);
	return R_TRUE;
}

int folder_new_interface(const char* kind) {
	char name[64];
	char* path = NULL;
	const char* text = designer_template(kind);
	int n;
	if (!g_folder) {
		ds_status("Open a folder first: the new file goes in it.");
		return R_FALSE;
	}
	if (!strcmp(kind, "theme")) return new_theme();
	if (!strcmp(kind, "table")) {
		Raction_fire("strings.add_table", R_INVALID_ID);
		designer_show_tab(3);
		return R_TRUE;
	}
	/* a library of tags is made in the Widget tab, a window in the Window tab */
	designer_show_tab(!strcmp(kind, "widgets") ? 1 : 0);
	for (n = 1; n < 1000; n++) {
		if (n == 1) snprintf(name, sizeof(name), "%s.xml", kind);
		else snprintf(name, sizeof(name), "%s-%d.xml", kind, n);
		path = ds_join(g_folder, name);
		if (!ds_file_exists(path)) break;
		free(path);
		path = NULL;
	}
	if (!path || !Rfile_write(path, text, strlen(text))) {
		ds_status("Cannot write a new file in %s.", g_folder);
		free(path);
		return R_FALSE;
	}
	folder_refresh();
	if (!ds_confirm_unsaved("making a new file")) {
		free(path);
		return R_FALSE;
	}
	designer_open_file(path);
	ds_status("%s made from the %s template.", name, kind);
	free(path);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Opening what is in the tree
 * ------------------------------------------------------------------------- */

static const file_t* file_of(const char* path) {
	int i;
	for (i = 0; i < g_f.file_count; i++) if (ds_same_path(g_f.files[i].path, path)) return &g_f.files[i];
	return NULL;
}

static void open_path(const char* path) {
	const file_t* file = path ? file_of(path) : NULL;
	int i;
	if (!file) return;
	switch (file->kind) {
		case FILE_INTERFACE:
		case FILE_WIDGETS:
			designer_show_tab(file->kind == FILE_WIDGETS ? 1 : 0);
			if (designer_get_path() && ds_same_path(designer_get_path(), path)) return;
			if (!ds_confirm_unsaved("opening another interface")) return;
			designer_open_file(path);
			break;
		case FILE_TABLE:
			designer_show_tab(3);
			strings_open(path);
			break;
		case FILE_THEME:
			(void)i;
			designer_show_tab(2);
			designer_open_theme(path);
			break;
	}
}

static void on_activated(Rid tree, Rid item, void* user_data) {
	(void)tree;
	(void)user_data;
	if (item != R_INVALID_ID) open_path((const char*)Rtree_item_get_user_handle(item));
}

static void on_context(Rid tree, Rid item, float x, float y, void* user_data) {
	const char* path = item != R_INVALID_ID ? (const char*)Rtree_item_get_user_handle(item) : NULL;
	(void)tree;
	(void)user_data;
	free(g_f.menu_path);
	g_f.menu_path = path ? ds_strdup(path) : NULL;
	Rmenu_open(g_f.menu, g_ds_window, x, y);
}

static void action_files(Rid sender, const char* action, void* user_data) {
	crash_report_note("action %s", action);
	(void)sender;
	(void)user_data;
	if (!strcmp(action, "files.open")) {
		Rid item = Rtree_view_get_selected_item(g_f.tree);
		open_path(g_f.menu_path ? g_f.menu_path : item != R_INVALID_ID ? (const char*)Rtree_item_get_user_handle(item) : NULL);
	} else if (!strcmp(action, "files.delete")) {
		const char* path = g_f.menu_path;
		/* (no message box during the random runs of the tests) */
		if (g_ds_fuzzing) return;
		char text[1200];
		if (!path || !file_of(path)) {
			ds_status("Right click a file to delete it.");
			return;
		}
		snprintf(text, sizeof(text), "Delete %s? It does not go to the trash.", ds_basename(path));
		if (Rmessage_box(g_ds_window, "Delete the file", text, R_MESSAGE_YES_NO) != R_MESSAGE_RESULT_YES) return;
		if (designer_get_path() && ds_same_path(designer_get_path(), path)) designer_close();
		if (!ds_remove_file(path)) ds_status("Cannot delete %s.", path);
		else ds_status("Deleted %s.", ds_basename(path));
		folder_refresh();
	} else if (!strcmp(action, "files.new")) {
		static const char* const kinds[4] = { "window", "widgets", "theme", "table" };
		folder_new_interface(kinds[g_f.view >= 0 && g_f.view < 4 ? g_f.view : 0]);
	} else if (!strncmp(action, "files.new.", 10)) {
		folder_new_interface(action + 10);
	} else if (!strcmp(action, "files.refresh")) {
		folder_refresh();
	}
	free(g_f.menu_path);
	g_f.menu_path = NULL;
}

void folder_init(void) {
	static const struct { const char* text; const char* action; } items[] = {
		{ "Open", "files.open" }, { NULL, NULL }, { "New", "files.new" }, { NULL, NULL }, { "Delete...", "files.delete" }
	};
	size_t i;
	g_f.panel = Rwidget_find(g_ds_window, "panel.files");
	g_f.tree = Rwidget_find(g_ds_window, "files");
	g_f.menu = Rwidget_create(R_WIDGET_MENU);
	for (i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
		if (!items[i].text) Rmenu_add_separator(g_f.menu);
		else Rwidget_set_action(Rmenu_add_item(g_f.menu, items[i].text), items[i].action);
	}
	Rtree_view_set_activated_callback(g_f.tree, on_activated, NULL);
	Rtree_view_set_context_callback(g_f.tree, on_context, NULL);
	Raction_bind("files.open", action_files, NULL);
	Raction_bind("files.delete", action_files, NULL);
	Raction_bind("files.new.window", action_files, NULL);
	Raction_bind("files.new.panel", action_files, NULL);
	Raction_bind("files.new.form", action_files, NULL);
	Raction_bind("files.new", action_files, NULL);
	Raction_bind("files.new.widgets", action_files, NULL);
	Raction_bind("files.new.theme", action_files, NULL);
	Raction_bind("files.new.table", action_files, NULL);
	Raction_bind("files.refresh", action_files, NULL);
	g_f.timer = Rtimer_create(1.5, R_TRUE, on_timer, NULL);
}

void folder_shutdown(void) {
	int i;
	if (g_f.timer != R_INVALID_ID && Rtimer_is_valid(g_f.timer)) Rtimer_destroy(g_f.timer);
	free_files(g_f.files, g_f.file_count);
	forget_themes();
	for (i = 0; i < g_f.item_count; i++) free(g_f.item_paths[i]);
	free(g_f.item_paths);
	free(g_f.menu_path);
	free(g_folder);
	g_folder = NULL;
	memset(&g_f, 0, sizeof(g_f));
}
