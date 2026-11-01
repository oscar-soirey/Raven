/**
 * Raven Designer - designer.h
 * What the parts of the designer share. The designer is a tool to make the
 * interfaces of Raven (.xml) by looking at them, in a folder: nothing is
 * built or compiled here; the program that loads the interface is written
 * elsewhere, in any language.
 *
 *   main.c            the window, the visual editor of an interface (widgets, tree,
 *                     preview, properties, XML, problems), the scripted runs
 *   files.c           the folder: its interfaces, themes and translation tables, followed live
 *   island.c          the island at the top: the command search (Ctrl+K), the selection,
 *                     the look of the designer, the theme and language of the preview,
 *                     what is going on
 *   icons.c           the colored pictures of the tags and of the files
 *   strings.c         the Translations panel (CSV tables) and the language of the preview
 *   welcome.c         the start page (recent folders) and the preferences
 *   accessibility.c   View > Accessibility
 *   os.c              folders and files
 */

#ifndef RAVEN_DESIGNER_H
#define RAVEN_DESIGNER_H

#include <Raven.h>

#include <stdarg.h>
#include <stddef.h>

/* ---- os.c ---- */

typedef struct ds_entry_t {
	char name[256];
	int is_dir;
} ds_entry_t;

/** The entries of a folder, folders first then by name; free with free(). Returns the count, -1 when it cannot be read. */
int  ds_list_dir(const char* path, ds_entry_t** entries);
int  ds_is_dir(const char* path);
int  ds_file_exists(const char* path);
/** Makes a folder and the folders above it. */
int  ds_make_dirs(const char* path);
/** Seconds since 1970 of the last change of a file, 0 when missing. */
double ds_file_time(const char* path);
/** The folder of the program itself, with no slash at the end. */
const char* ds_exe_dir(void);
/** "a/b" + "c" -> "a/b/c" (malloc). */
char* ds_join(const char* a, const char* b);
/** The part after the last slash. */
const char* ds_basename(const char* path);
/** The extension, lowercase, without the dot ("" when none). */
const char* ds_extension(const char* path);
/** Same file names, whatever the slashes (and the case on Windows). */
int  ds_same_path(const char* a, const char* b);
/** A copy with forward slashes. */
char* ds_normalize(const char* path);
/** The folder of the user (forward slashes). */
const char* ds_home_dir(void);
/** Where the designer keeps its preferences and its recent folders (made when missing). */
const char* ds_config_dir(void);
/** A path for the eyes: the folder of the user becomes "~". */
void ds_pretty_path(const char* path, char* out, size_t size);
int  ds_remove_file(const char* path);

/* ---- main.c (the visual editor) ---- */

extern Rid g_ds_window;
extern Rid g_ds_dock;
/** The folder open (forward slashes), NULL when none. */
extern char* g_folder;
/** R_TRUE during the random runs of the tests (the fuzz command): no dialog may open. */
extern int g_ds_fuzzing;

void ds_status(const char* format, ...);
/** Opens an interface (.xml) in the visual editor. */
void designer_open_file(const char* path);
/** The file the visual editor shows, or NULL. */
const char* designer_get_path(void);
int  designer_is_modified(void);
/** R_TRUE when the visual editor shows an interface. */
int  designer_has_document(void);
/** The file changed on disk: read again when it has no unsaved changes (the selection stays). */
void designer_file_changed(void);
/** The problems of the interface. */
int  designer_problem_count(void);
const char* designer_problem_text(int index);
/** Shows the Problems panel (and the line of the first problem). */
void designer_show_problems(void);
/** Builds the preview again (the translations, the language or a theme changed). */
void designer_refresh_preview(void);
/** The frame of the preview: the strings panel sets its direction. */
Rid  designer_get_frame(void);
/** Saves the interface into its file (asks where when it has none). */
int  designer_save(void);
/** No interface is shown any more. */
void designer_close(void);
void designer_update_title(void);
/** Unsaved changes (the interface, the tables): asks to save them. R_FALSE when the user cancels. */
int  ds_confirm_unsaved(const char* why);
/** The text a new interface starts from: "window", "panel", "form". */
const char* designer_template(const char* kind);

/* the selection, for the island */
/** The element selected (an Rxml node), or R_INVALID_ID. */
Rid  designer_get_selected(void);
/** "Window > VBox > Button": where the selection is. */
void designer_selection_path(char* out, size_t size);
/** Writes an attribute of the selection (NULL removes it), as the Properties panel does. */
void designer_set_attribute(const char* name, const char* value);
/** Adds a widget inside the selection (or after it), as the Widgets panel does. */
void designer_add_widget(const char* tag);
/** The tags the Widgets panel offers (those the document declares first). */
int  designer_widget_count(void);
const char* designer_widget_at(int index);

/** The tabs: 0 Window, 1 Widget, 2 Theme, 3 Translations. */
void designer_show_tab(int tab);
int  designer_get_tab(void);
/** The page of the window the tab in front shows (the start page is 0). */
int  designer_tab_page(void);
/** Opens a theme file in the Theme tab. */
void designer_open_theme(const char* path);
/** The designer wears a look ("Raven dark"...) or a theme of the folder ("Theme: Night"). */
void designer_set_appearance(const char* name);

/** The widget libraries of the folder changed: the palette and the preview follow. */
void designer_widgets_changed(void);
/** Another folder opens: what the Window and Widget tabs held goes. */
void designer_forget_tabs(void);

/** The themes of the preview changed (files.c found or lost a theme file). */
void designer_themes_changed(void);
/** The preview wears this theme ("" : the look of the designer). */
void designer_set_preview_theme(const char* name);

/* ---- files.c: the folder ---- */

void folder_init(void);
void folder_shutdown(void);
/** Opens a folder: its files, its translation tables, its themes. R_FALSE when it is not a folder. */
int  folder_open(const char* path);
void folder_close(void);
/** Looks at the folder now (else it does, every second and a half). */
void folder_refresh(void);
/** The interfaces of the folder (full paths), for the command search of the island. */
int  folder_interface_count(void);
const char* folder_interface_at(int index);
/** The themes the folder holds (theme files): their names, as Rtheme_load gave them. */
int  folder_theme_count(void);
const char* folder_theme_name(int index);
/** What Files shows: the windows (0), the widget libraries (1), the themes (2), the tables (3). */
void folder_set_view(int view);
/** The tags declared by the widget libraries of the folder, made known to the windows. */
void folder_register_widgets(void);
const char* folder_theme_path(int index);
/** Opens the first file of a kind (0 windows, 1 widget libraries): main.xml, else the top of the folder. */
int  folder_open_first(int view);
/** Makes a new interface in the folder from a template ("window", "panel", "form") and opens it. */
int  folder_new_interface(const char* kind);

/* ---- island.c: the island at the top of the window ---- */

void island_init(void);
void island_shutdown(void);
/** Shows what changed now (else it follows four times a second). */
void island_refresh(void);
/** Unfolds the island on its search, the keyboard in it (Ctrl+K). */
void island_open_search(void);
/** What the folded island says (the file, the selection, the problems), for the scripts of the tests. */
const char* island_get_title(void);
/** Types in the search of the island and runs the first result (scripts of the tests). R_FALSE when nothing matched. */
int  island_run_search(const char* text);

/* ---- strings.c ---- */

void strings_init(void);
void strings_shutdown(void);
/** Reads the CSV tables of the folder (after it was opened, or a table changed). */
void strings_reload(void);
/** Opens a table in the Translations panel. */
void strings_open(const char* path);
int  strings_any_modified(void);
int  strings_save(void);
/** The language of the preview ("" : the first column). */
void strings_set_preview_language(const char* code);
/** The languages of the tables, for the island ("en", "fr"...). */
int  strings_language_count(void);
const char* strings_language_at(int index);
/** Writes a cell (scripts of the tests). */
int  strings_set_cell(const char* key, int column, const char* text);
/** The keys of the tables (completion of @keys). */
int  strings_key_count(void);
const char* strings_key(int index);

/* ---- welcome.c ---- */

void welcome_init(void);
void welcome_shutdown(void);
/** The start page: open a folder or a file, the recent folders. */
void welcome_show(void);
/** The panels of the designer (a folder or a file is open). */
void welcome_hide(void);
int  welcome_is_shown(void);
/** A folder was opened: first in the list of recent folders. */
void recent_add(const char* folder);
/** A preference of the designer (designer.ini in ds_config_dir), "" when not set. */
const char* prefs_get(const char* key);
void prefs_set(const char* key, const char* value);

/* ---- icons.c: the colored pictures of the tags and of the files ---- */

/** Draws the pictures into one texture (once, after Rinit). */
void icons_init(void);
void icons_shutdown(void);
Rid  icons_texture(void);
/** The region of the texture (u0, v0, u1, v1) of a tag ("Button"), or of "#folder", "#comment", "#file.window", "#file.widgets", "#file.theme", "#file.table". */
void icons_region(const char* key, float uv[4]);
/** The picture before the text of an item of a tree view. */
void icons_set_tree_item(Rid item, const char* key);
/** An Image shows the picture. */
void icons_set_image(Rid image, const char* key);
/** The groups of the palette: "Your widgets" (0), "Layout", "Controls"... "Other" (the last). */
int  icons_group_count(void);
const char* icons_group_name(int group);
/** A tag whose picture stands for the group. */
const char* icons_group_key(int group);
/** The group a tag goes in. */
int  icons_group_of(const char* tag);
/** The tags of Raven a group lists, in their order (not the declared ones, not the group "Other"). */
int  icons_group_tag_count(int group);
const char* icons_group_tag_at(int group, int index);
/** Saves the pictures in a .ppm (scripts of the tests). */
int  icons_save(const char* path);

/* ---- accessibility.c ---- */

void accessibility_init(void);
void accessibility_shutdown(void);
/** The settings kept from the last time (designer.ini, a11y.*), applied at start. */
void accessibility_apply_saved(void);
/** View > Accessibility: every setting, live. */
void accessibility_window(void);
/** The menu follows the settings (Reduce motion). */
void accessibility_sync_menu(void);

/* ---- small things ---- */

char* ds_strdup(const char* text);
int   ds_iequal(const char* a, const char* b);
/** R_TRUE when `part` is in `text`, whatever the case. */
int   ds_contains(const char* text, const char* part);
/** Appends to a growing buffer. */
typedef struct ds_buffer_t {
	char* data;
	size_t size, cap;
} ds_buffer_t;
void ds_buffer_add(ds_buffer_t* buffer, const char* text, size_t length);
void ds_buffer_printf(ds_buffer_t* buffer, const char* format, ...);
void ds_buffer_free(ds_buffer_t* buffer);

#endif
