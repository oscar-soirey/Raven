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
typedef enum file_kind_t { FILE_INTERFACE, FILE_WIDGETS, FILE_THEME, FILE_TABLE, FILE_FOLDER } file_kind_t;

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
	/* the kind of files the tab in front makes ("New") */
	int view;
	/* the folders the user folded (relative paths): they stay folded when the tree is filled again */
	char** folded;
	int folded_count;
	/* set while the designer itself selects in the tree */
	int syncing;
	/* the folder a "New" makes its file in, and the file or folder the menu is about */
	char* target_dir;
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
			add_file(files, count, full, FILE_FOLDER);
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

static int is_folded(const char* relative) {
	int i;
	for (i = 0; i < g_f.folded_count; i++) if (!strcmp(g_f.folded[i], relative)) return R_TRUE;
	return R_FALSE;
}

static const char* relative_of(const char* path) {
	size_t root_length = g_folder ? strlen(g_folder) : 0;
	return strlen(path) > root_length ? path + root_length + 1 : "";
}

/* The folders the user folded, read from the tree before it is filled again. */
static void remember_folded(Rid item) {
	int i, count = item == R_INVALID_ID ? Rtree_view_get_item_count(g_f.tree) : Rtree_item_get_child_count(item);
	if (item != R_INVALID_ID && Rtree_item_get_child_count(item) > 0 && !Rtree_item_is_expanded(item)) {
		const char* path = (const char*)Rtree_item_get_user_handle(item);
		if (path && ds_is_dir(path) && !is_folded(relative_of(path))) {
			g_f.folded = (char**)realloc(g_f.folded, sizeof(char*) * (size_t)(g_f.folded_count + 1));
			g_f.folded[g_f.folded_count++] = ds_strdup(relative_of(path));
		}
	}
	for (i = 0; i < count; i++) remember_folded(item == R_INVALID_ID ? Rtree_view_get_item(g_f.tree, i) : Rtree_item_get_child(item, i));
}

static void forget_folded(void) {
	int i;
	for (i = 0; i < g_f.folded_count; i++) free(g_f.folded[i]);
	free(g_f.folded);
	g_f.folded = NULL;
	g_f.folded_count = 0;
}

static Rid item_of(Rid item, const char* path) {
	int i, count = item == R_INVALID_ID ? Rtree_view_get_item_count(g_f.tree) : Rtree_item_get_child_count(item);
	const char* own = item != R_INVALID_ID ? (const char*)Rtree_item_get_user_handle(item) : NULL;
	if (own && ds_same_path(own, path)) return item;
	for (i = 0; i < count; i++) {
		Rid found = item_of(item == R_INVALID_ID ? Rtree_view_get_item(g_f.tree, i) : Rtree_item_get_child(item, i), path);
		if (found != R_INVALID_ID) return found;
	}
	return R_INVALID_ID;
}

/* A file selected in the tree (by the designer: nothing opens). */
void folder_select(const char* path) {
	Rid item = path ? item_of(R_INVALID_ID, path) : R_INVALID_ID;
	g_f.syncing++;
	Rtree_view_set_selected_item(g_f.tree, item);
	if (item != R_INVALID_ID) Rtree_view_scroll_to_item(g_f.tree, item);
	g_f.syncing--;
}

/*
 * Files: the whole folder, its sub-folders as branches (those folded stay
 * so), and in them the interfaces, the widget libraries, the themes and the
 * tables, each with its picture.
 */
static void fill_tree(void) {
	Rid* folder_items;
	int i, k;
	char** folder_paths;
	int folder_count = 0, shown = 0;
	const char* open = designer_get_path();
	if (g_f.tree == R_INVALID_ID) return;
	remember_folded(R_INVALID_ID);
	g_f.syncing++;
	Rtree_view_clear_items(g_f.tree);
	for (i = 0; i < g_f.item_count; i++) free(g_f.item_paths[i]);
	g_f.item_count = 0;
	if (!g_folder) {
		g_f.syncing--;
		Rdock_area_set_panel_title(g_ds_dock, g_f.panel, "Files");
		return;
	}
	folder_items = (Rid*)malloc(sizeof(Rid) * (size_t)(g_f.file_count + 1));
	folder_paths = (char**)malloc(sizeof(char*) * (size_t)(g_f.file_count + 1));
	for (i = 0; i < g_f.file_count; i++) {
		static const char* const kinds[5] = { "#file.window", "#file.widgets", "#file.theme", "#file.table", "#folder" };
		const file_t* file = &g_f.files[i];
		char* normalized = ds_normalize(file->path);
		char* parent_path = ds_strdup(normalized);
		char* slash = strrchr(parent_path, '/');
		Rid parent = R_INVALID_ID, item;
		if (slash) *slash = '\0';
		/* (the scan lists a folder before what it holds) */
		for (k = folder_count - 1; k >= 0; k--) {
			if (!strcmp(folder_paths[k], parent_path)) {
				parent = folder_items[k];
				break;
			}
		}
		item = Rtree_view_add_item(g_f.tree, parent, ds_basename(normalized));
		icons_set_tree_item(item, kinds[file->kind]);
		remember_path(item, file->path);
		if (file->kind == FILE_FOLDER) {
			Rtree_item_set_expanded(item, !is_folded(relative_of(normalized)));
			folder_items[folder_count] = item;
			folder_paths[folder_count++] = ds_strdup(normalized);
		} else {
			shown++;
			if (open && ds_same_path(open, file->path)) Rtree_view_set_selected_item(g_f.tree, item);
		}
		free(parent_path);
		free(normalized);
	}
	for (i = 0; i < folder_count; i++) free(folder_paths[i]);
	free(folder_paths);
	free(folder_items);
	g_f.syncing--;
	{
		char title[64];
		snprintf(title, sizeof(title), "Files (%d)", shown);
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
	forget_folded();
	free(g_f.target_dir);
	g_f.target_dir = NULL;
	/* (the tree of the other folder is not read for what was folded in it) */
	g_f.syncing++;
	Rtree_view_clear_items(g_f.tree);
	g_f.syncing--;
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
	if (view <= 1) folder_select(designer_get_path());
}

/* The tags the designer declares for itself (its own interface): never forgotten. */
static char** g_own_tags;
static int g_own_tag_count = -1;

/*
 * The tags declared by the widget libraries of the folder, made known to the
 * windows; those of a library that went away, or of another folder, are
 * forgotten first.
 */
void folder_register_widgets(void) {
	int i;
	const char* open = designer_get_path();
	if (g_own_tag_count < 0) {
		g_own_tag_count = Rxml_get_registered_widget_count();
		g_own_tags = (char**)calloc((size_t)g_own_tag_count + 1, sizeof(char*));
		for (i = 0; i < g_own_tag_count; i++) g_own_tags[i] = ds_strdup(Rxml_get_registered_widget_name(i));
	}
	for (i = Rxml_get_registered_widget_count() - 1; i >= 0; i--) {
		char* name = ds_strdup(Rxml_get_registered_widget_name(i));
		int k, own = R_FALSE;
		for (k = 0; k < g_own_tag_count && !own; k++) own = !strcmp(g_own_tags[k], name);
		if (!own) Rxml_unregister_widget(name);
		free(name);
	}
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

/* ---------------------------------------------------------------------------
 *  Making, renaming, deleting
 * ------------------------------------------------------------------------- */

/* The folder a new file goes in: the one of the menu or of the selection (a file: its folder), else the top. */
static const char* new_file_folder(void) {
	return g_f.target_dir && ds_is_dir(g_f.target_dir) ? g_f.target_dir : g_folder;
}

/* A free name in a folder: "base.ext", else "base-2.ext"... (malloc), NULL when none. */
static char* free_name(const char* folder, const char* base, const char* extension) {
	int n;
	for (n = 1; n < 1000; n++) {
		char name[160];
		char* path;
		if (n == 1) snprintf(name, sizeof(name), "%s%s%s", base, extension[0] ? "." : "", extension);
		else snprintf(name, sizeof(name), "%s-%d%s%s", base, n, extension[0] ? "." : "", extension);
		path = ds_join(folder, name);
		if (!ds_file_exists(path) && !ds_is_dir(path)) return path;
		free(path);
	}
	return NULL;
}

/* A theme to start from, written in the folder and opened in the Theme tab. */
static int new_theme(void) {
	char* path = free_name(new_file_folder(), "theme", "xml");
	te_model model;
	te_text text;
	if (!path) return R_FALSE;
	te_model_init(&model);
	model.accent_set = 1;
	model.accent[0] = 0.49f;
	model.accent[1] = 0.36f;
	model.accent[2] = 1.0f;
	snprintf(model.name, sizeof(model.name), "%s", strcmp(ds_basename(path), "theme.xml") ? ds_basename(path) : "My theme");
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
	folder_select(path);
	ds_status("%s made: change it here, the windows of the folder can wear it.", ds_basename(path));
	free(path);
	return R_TRUE;
}

/* A table of translations: a column of keys, one of English. */
static int new_table(void) {
	char* path = free_name(new_file_folder(), "strings", "csv");
	if (!path || !Rfile_write(path, "key,en\n", 7)) {
		free(path);
		return R_FALSE;
	}
	folder_refresh();
	strings_reload();
	designer_show_tab(3);
	strings_open(path);
	folder_select(path);
	ds_status("%s made: add keys and languages in the Translations tab.", ds_basename(path));
	free(path);
	return R_TRUE;
}

int folder_new_interface(const char* kind) {
	char* path;
	const char* text = designer_template(kind);
	if (!g_folder) {
		ds_status("Open a folder first: the new file goes in it.");
		return R_FALSE;
	}
	if (!strcmp(kind, "theme")) return new_theme();
	if (!strcmp(kind, "table")) return new_table();
	if (!strcmp(kind, "folder")) return folder_new_folder(NULL) != NULL;
	path = free_name(new_file_folder(), kind, "xml");
	/* a library of tags is made in the Widget tab, a window in the Window tab */
	designer_show_tab(!strcmp(kind, "widgets") ? 1 : 0);
	if (!path || !Rfile_write(path, text, strlen(text))) {
		ds_status("Cannot write a new file in %s.", new_file_folder());
		free(path);
		return R_FALSE;
	}
	folder_refresh();
	if (!ds_confirm_unsaved("making a new file")) {
		free(path);
		return R_FALSE;
	}
	designer_open_file(path);
	folder_select(path);
	ds_status("%s made from the %s template.", ds_basename(path), kind);
	free(path);
	return R_TRUE;
}

/* A new folder, in the folder of the selection (or `name` there: the scripts); its path, or NULL. Static: valid until the next call. */
const char* folder_new_folder(const char* name) {
	static char* made;
	char* path;
	if (!g_folder) return NULL;
	if (name && name[0]) {
		path = ds_join(new_file_folder(), name);
		if (ds_is_dir(path) || ds_file_exists(path)) {
			free(path);
			path = NULL;
		}
	} else {
		path = free_name(new_file_folder(), "New folder", "");
	}
	if (!path || !ds_make_dirs(path)) {
		ds_status("Cannot make a folder in %s.", new_file_folder());
		free(path);
		return NULL;
	}
	free(made);
	made = path;
	folder_refresh();
	folder_select(made);
	ds_status("Folder %s made: right click it to make files in it, F2 to rename it.", ds_basename(made));
	return made;
}

/* The paths that begin with `from` (the file, or what was in the folder) now begin with `to`. */
static void moved(const char* from, const char* to) {
	designer_file_moved(from, to);
	folder_refresh();
	strings_reload();
}

/* Renames a file or a folder of the folder open. R_FALSE (with a word in the status bar) when it cannot. */
int folder_rename(const char* path, const char* new_name) {
	char* target;
	char* parent;
	char* slash;
	if (!path || !new_name || !new_name[0] || strchr(new_name, '/') || strchr(new_name, '\\') || !strcmp(new_name, ".") || !strcmp(new_name, "..")) {
		ds_status("A name without slashes, please.");
		return R_FALSE;
	}
	parent = ds_normalize(path);
	slash = strrchr(parent, '/');
	if (slash) *slash = '\0';
	target = ds_join(parent, new_name);
	free(parent);
	/* a file keeps its extension when the new name has none */
	if (!ds_is_dir(path) && !strchr(new_name, '.') && ds_extension(path)[0]) {
		char* with = (char*)malloc(strlen(target) + strlen(ds_extension(path)) + 2);
		sprintf(with, "%s.%s", target, ds_extension(path));
		free(target);
		target = with;
	}
	if (ds_same_path(target, path)) {
		free(target);
		return R_TRUE;
	}
	if (!ds_rename(path, target)) {
		ds_status("Cannot rename %s to %s (is there one with that name?).", ds_basename(path), ds_basename(target));
		free(target);
		return R_FALSE;
	}
	moved(path, target);
	folder_select(target);
	ds_status("Renamed to %s.", ds_basename(target));
	free(target);
	return R_TRUE;
}

/* Deletes a file, or a folder with what it holds (no trash: the caller asked). */
int folder_delete(const char* path) {
	int folder = ds_is_dir(path), ok;
	const char* open = designer_get_path();
	char* normalized;
	if (!g_folder || !path || ds_same_path(path, g_folder)) {
		ds_status("The folder open itself is not deleted here.");
		return R_FALSE;
	}
	normalized = ds_normalize(path);
	/* what is open there closes first */
	if (open) {
		char* o = ds_normalize(open);
		size_t n = strlen(normalized);
		if (!strcmp(o, normalized) || (folder && !strncmp(o, normalized, n) && o[n] == '/')) designer_close();
		free(o);
	}
	ok = folder ? ds_remove_tree(path) : ds_remove_file(path);
	if (!ok) ds_status("Cannot delete %s.", ds_basename(path));
	else ds_status("Deleted %s.", ds_basename(path));
	free(normalized);
	folder_refresh();
	strings_reload();
	return ok;
}

/* ---------------------------------------------------------------------------
 *  Opening what is in the tree
 * ------------------------------------------------------------------------- */

static const file_t* file_of(const char* path) {
	int i;
	for (i = 0; i < g_f.file_count; i++) if (ds_same_path(g_f.files[i].path, path)) return &g_f.files[i];
	return NULL;
}

/* What a file is, listed or not. */
static file_kind_t kind_of(const char* path) {
	const file_t* file = file_of(path);
	if (file) return file->kind;
	if (ds_is_dir(path)) return FILE_FOLDER;
	if (ds_iequal(ds_extension(path), "csv")) return FILE_TABLE;
	return xml_kind(path);
}

/* Opens a file where it is edited: an interface or a library in its tab, a table in Translations, a theme in the Theme tab. */
static void open_path(const char* path) {
	if (!path) return;
	switch (kind_of(path)) {
		case FILE_INTERFACE:
		case FILE_WIDGETS:
			designer_show_tab(kind_of(path) == FILE_WIDGETS ? 1 : 0);
			if (designer_get_path() && ds_same_path(designer_get_path(), path)) break;
			if (!ds_confirm_unsaved("opening another interface")) break;
			designer_open_file(path);
			break;
		case FILE_TABLE:
			designer_show_tab(3);
			strings_open(path);
			break;
		case FILE_THEME:
			designer_show_tab(2);
			designer_open_theme(path);
			break;
		case FILE_FOLDER:
			break;
	}
	/* (a cancel leaves the file open selected) */
	if (designer_get_tab() <= 1) folder_select(designer_get_path());
}

int folder_open_file(const char* path) {
	open_path(path);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  The panel: a click opens, a right click or the bar makes, renames, deletes
 * ------------------------------------------------------------------------- */

static void set_target(const char* path) {
	free(g_f.target_dir);
	g_f.target_dir = NULL;
	free(g_f.menu_path);
	g_f.menu_path = path ? ds_strdup(path) : NULL;
	if (!path) return;
	if (ds_is_dir(path)) {
		g_f.target_dir = ds_strdup(path);
	} else {
		char* folder = ds_normalize(path);
		char* slash = strrchr(folder, '/');
		if (slash) *slash = '\0';
		g_f.target_dir = folder;
	}
}

/* The selection of the tree is what the bar acts on. */
static const char* selected_path(void) {
	Rid item = Rtree_view_get_selected_item(g_f.tree);
	return item != R_INVALID_ID ? (const char*)Rtree_item_get_user_handle(item) : NULL;
}

/* A click on a file opens it (a folder opens or folds with a double click, or its arrow). */
static void on_selected(Rid tree, Rid item, void* user_data) {
	const char* path;
	(void)tree;
	(void)user_data;
	if (g_f.syncing || item == R_INVALID_ID) return;
	path = (const char*)Rtree_item_get_user_handle(item);
	set_target(path);
	if (path && !ds_is_dir(path)) open_path(path);
}

static void on_activated(Rid tree, Rid item, void* user_data) {
	(void)tree;
	(void)user_data;
	if (item != R_INVALID_ID) open_path((const char*)Rtree_item_get_user_handle(item));
}

static void fill_menu(int on_file, int on_folder) {
	static const struct { const char* text; const char* action; } make[] = {
		{ "New window", "files.new.window" }, { "New panel", "files.new.panel" }, { "New form", "files.new.form" },
		{ "New widget library", "files.new.widgets" }, { "New theme", "files.new.theme" }, { "New table of translations", "files.new.table" },
		{ "New folder", "files.new.folder" }
	};
	size_t i;
	if (g_f.menu != R_INVALID_ID && Rwidget_is_valid(g_f.menu)) Rwidget_destroy(g_f.menu);
	g_f.menu = Rwidget_create(R_WIDGET_MENU);
	if (on_file) {
		Rwidget_set_action(Rmenu_add_item(g_f.menu, "Open"), "files.open");
		Rmenu_add_separator(g_f.menu);
	}
	for (i = 0; i < sizeof(make) / sizeof(make[0]); i++) Rwidget_set_action(Rmenu_add_item(g_f.menu, make[i].text), make[i].action);
	if (on_file || on_folder) {
		Rmenu_add_separator(g_f.menu);
		Rwidget_set_action(Rmenu_add_item(g_f.menu, "Rename...    F2"), "files.rename");
		Rwidget_set_action(Rmenu_add_item(g_f.menu, "Delete...    Del"), "files.delete");
	}
	Rmenu_add_separator(g_f.menu);
	Rwidget_set_action(Rmenu_add_item(g_f.menu, "Show in the file manager"), "files.reveal");
}

static void on_context(Rid tree, Rid item, float x, float y, void* user_data) {
	const char* path = item != R_INVALID_ID ? (const char*)Rtree_item_get_user_handle(item) : NULL;
	(void)tree;
	(void)user_data;
	set_target(path);
	fill_menu(path && !ds_is_dir(path), path && ds_is_dir(path));
	Rmenu_open(g_f.menu, g_ds_window, x, y);
}

/* ---- asking a name ---- */

typedef struct ask_t {
	char* path;
	Rid input;
} ask_t;

static void on_ask_closed(Rid dialog, int result, void* user_data) {
	ask_t* ask = (ask_t*)user_data;
	(void)dialog;
	if (result == 1 && Rwidget_is_valid(ask->input)) folder_rename(ask->path, Rtext_input_get_text(ask->input));
	free(ask->path);
	free(ask);
}

static void on_ask_ok(Rid sender, const char* action, void* user_data) {
	(void)action;
	(void)user_data;
	Rdialog_close(sender, 1);
}

static void on_ask_cancel(Rid sender, const char* action, void* user_data) {
	(void)action;
	(void)user_data;
	Rdialog_close(sender, 0);
}

static void ask_rename(const char* path) {
	static const char xml[] =
		"<VBox Spacing=\"10\" Width=\"360\" Slot.Padding=\"18\">"
		"  <Label Name=\"ask.title\" TextSize=\"17\"/>"
		"  <TextInput Name=\"ask.input\" OnCommitted=\"files.ask.ok\"/>"
		"  <HBox Spacing=\"8\" Slot.HAlign=\"Right\">"
		"    <Button Text=\"Cancel\" OnClicked=\"files.ask.cancel\"/>"
		"    <Button Text=\"Rename\" Style=\"Primary\" OnClicked=\"files.ask.ok\"/>"
		"  </HBox>"
		"</VBox>";
	Rid content;
	ask_t* ask;
	char title[300];
	if (g_ds_fuzzing || !path) return;
	content = Rwidget_create_from_xml(xml, sizeof(xml) - 1);
	if (content == R_INVALID_ID) return;
	ask = (ask_t*)calloc(1, sizeof(ask_t));
	ask->path = ds_strdup(path);
	ask->input = Rwidget_find(content, "ask.input");
	snprintf(title, sizeof(title), "Rename %s", ds_basename(path));
	Rlabel_set_text(Rwidget_find(content, "ask.title"), title);
	fonts_title_on(Rwidget_find(content, "ask.title"));
	Rtext_input_set_text(ask->input, ds_basename(path));
	Rdialog_open(g_ds_window, content, on_ask_closed, ask);
	Rwidget_focus(ask->input);
	Rtext_input_select_all(ask->input);
}

static void ask_delete(const char* path) {
	char text[1200];
	int folder;
	if (!path || g_ds_fuzzing) return;
	folder = ds_is_dir(path);
	if (folder) snprintf(text, sizeof(text), "Delete the folder %s and everything in it? It does not go to the trash.", ds_basename(path));
	else snprintf(text, sizeof(text), "Delete %s? It does not go to the trash.", ds_basename(path));
	if (Rmessage_box(g_ds_window, folder ? "Delete the folder" : "Delete the file", text, R_MESSAGE_YES_NO) != R_MESSAGE_RESULT_YES) return;
	folder_delete(path);
}

static void action_files(Rid sender, const char* action, void* user_data) {
	/* (what the menu was opened on, else the selection of the tree) */
	const char* path = g_f.menu_path ? g_f.menu_path : selected_path();
	char* target = path ? ds_strdup(path) : NULL;
	crash_report_note("action %s", action);
	(void)user_data;
	if (!g_f.menu_path) set_target(target);
	if (!strcmp(action, "files.open")) {
		open_path(target);
	} else if (!strcmp(action, "files.delete")) {
		if (!target) ds_status("Select a file or a folder in Files to delete it.");
		else ask_delete(target);
	} else if (!strcmp(action, "files.f2")) {
		/* (F2 is for Files only when it has the keyboard) */
		if (folder_has_keyboard() && target) ask_rename(target);
	} else if (!strcmp(action, "files.rename")) {
		if (!target) ds_status("Select a file or a folder in Files to rename it.");
		else ask_rename(target);
	} else if (!strcmp(action, "files.reveal")) {
		const char* folder = g_f.target_dir ? g_f.target_dir : g_folder;
		if (folder) Ropen_url(folder);
	} else if (!strcmp(action, "files.new.menu")) {
		/* the button "New": the same menu, under it */
		float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
		fill_menu(R_FALSE, R_FALSE);
		if (sender != R_INVALID_ID && Rwidget_is_valid(sender)) Rwidget_get_rect(sender, &x, &y, &w, &h);
		Rmenu_open(g_f.menu, g_ds_window, x, y + h);
		free(target);
		/* (the menu opened keeps its target until one of its items is chosen) */
		return;
	} else if (!strcmp(action, "files.new")) {
		static const char* const kinds[4] = { "window", "widgets", "theme", "table" };
		folder_new_interface(kinds[g_f.view >= 0 && g_f.view < 4 ? g_f.view : 0]);
	} else if (!strncmp(action, "files.new.", 10)) {
		folder_new_interface(action + 10);
	} else if (!strcmp(action, "files.refresh")) {
		folder_refresh();
	}
	free(target);
	free(g_f.menu_path);
	g_f.menu_path = NULL;
}

/* What a click on a file or a folder of Files does (scripts of the tests). */
void folder_click(const char* path) {
	Rid item = path ? item_of(R_INVALID_ID, path) : R_INVALID_ID;
	if (item == R_INVALID_ID) return;
	g_f.syncing++;
	Rtree_view_set_selected_item(g_f.tree, item);
	g_f.syncing--;
	on_selected(g_f.tree, item, NULL);
}

/* Delete or F2 in Files: for its files, not for the elements of the preview. */
int folder_has_keyboard(void) {
	return g_f.tree != R_INVALID_ID && Rwidget_is_focused(g_f.tree);
}

void folder_init(void) {
	static const char* const actions[] = {
		"files.open", "files.delete", "files.rename", "files.reveal", "files.new", "files.new.menu", "files.new.window", "files.new.panel",
		"files.new.form", "files.new.widgets", "files.new.theme", "files.new.table", "files.new.folder", "files.refresh", "files.f2"
	};
	size_t i;
	g_f.panel = Rwidget_find(g_ds_window, "panel.files");
	g_f.tree = Rwidget_find(g_ds_window, "files");
	g_f.menu = R_INVALID_ID;
	fill_menu(R_FALSE, R_FALSE);
	Rtree_view_set_selection_changed_callback(g_f.tree, on_selected, NULL);
	Rtree_view_set_activated_callback(g_f.tree, on_activated, NULL);
	Rtree_view_set_context_callback(g_f.tree, on_context, NULL);
	for (i = 0; i < sizeof(actions) / sizeof(actions[0]); i++) Raction_bind(actions[i], action_files, NULL);
	Raction_bind("files.ask.ok", on_ask_ok, NULL);
	Raction_bind("files.ask.cancel", on_ask_cancel, NULL);
	g_f.timer = Rtimer_create(1.5, R_TRUE, on_timer, NULL);
}

void folder_shutdown(void) {
	int i;
	if (g_f.timer != R_INVALID_ID && Rtimer_is_valid(g_f.timer)) Rtimer_destroy(g_f.timer);
	free_files(g_f.files, g_f.file_count);
	forget_themes();
	forget_folded();
	for (i = 0; i < g_f.item_count; i++) free(g_f.item_paths[i]);
	free(g_f.item_paths);
	free(g_f.menu_path);
	free(g_f.target_dir);
	free(g_folder);
	g_folder = NULL;
	for (i = 0; i < g_own_tag_count; i++) free(g_own_tags[i]);
	free(g_own_tags);
	g_own_tags = NULL;
	g_own_tag_count = -1;
	memset(&g_f, 0, sizeof(g_f));
}
