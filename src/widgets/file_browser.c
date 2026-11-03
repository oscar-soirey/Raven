/**
 * Raven - widgets/file_browser.c
 * FileBrowser: an explorer of the files of the computer. It is what the
 * file dialogs show where the OS has no dialog of its own, and a widget that
 * applications put in their own layouts.
 *
 * The browser draws the list of files and the navigation buttons itself;
 * the fields around the list (path, places, name, filter, buttons) are
 * ordinary widgets that it owns as children and arranges by hand.
 */

#include "common.h"
#include "../core/fs.h"

#include <time.h>

enum { RVN_FB_BACK, RVN_FB_FORWARD, RVN_FB_UP, RVN_FB_NAV_COUNT };
enum { RVN_FB_COLUMN_NAME, RVN_FB_COLUMN_SIZE, RVN_FB_COLUMN_DATE };

#define RVN_FB_GAP 8.0f
#define RVN_FB_PLACES_WIDTH 150.0f
#define RVN_FB_SIZE_WIDTH 84.0f
#define RVN_FB_DATE_WIDTH 132.0f
#define RVN_FB_HISTORY_MAX 64

typedef struct rvn_file_browser_t {
	rvn_widget_t base;
	RFileBrowserMode mode;
	char* directory;
	/* the entries shown: filtered and sorted */
	rvn_dir_entry_t* entries;
	uint32_t entry_count;
	uint8_t* selected;
	/** Entry the keyboard is on, entry a range selection started from, entry under the pointer; -1 = none. */
	int cursor;
	int anchor;
	int hot;
	int sort_column;
	int sort_descending;
	int show_hidden;
	int multiple;
	int controls_hidden;
	int places_hidden;
	char** filter_names;
	char** filter_patterns;
	int filter_count;
	int filter_index;
	char* default_extension;
	rvn_place_t* places;
	uint32_t place_count;
	/* folders visited, for Back and Forward */
	char* history[RVN_FB_HISTORY_MAX];
	int history_count;
	int history_at;

	/* children */
	rvn_widget_t* path_input;
	rvn_widget_t* places_list;
	rvn_widget_t* name_input;
	rvn_widget_t* filter_combo;
	rvn_widget_t* accept_button;
	rvn_widget_t* cancel_button;
	/** R_TRUE while the browser itself changes its fields: their callbacks do nothing. */
	int updating;

	/* geometry, computed by arrange */
	rvn_rect_t nav[RVN_FB_NAV_COUNT];
	rvn_rect_t list;
	float header_height;
	rvn_scrollbar_t bar;
	RSpring select_y;
	int nav_hot;
	int nav_down;
	RSpring nav_hover[RVN_FB_NAV_COUNT];
	RSpring nav_press[RVN_FB_NAV_COUNT];

	/* typing the first letters of a name moves to it */
	char typed[32];
	double typed_time;

	char** selection_paths;
	uint32_t selection_count;
	RFileBrowserCallback accepted;
	void* accepted_user;
	RFileBrowserCallback cancelled;
	void* cancelled_user;
	RFileBrowserCallback selection_changed;
	void* selection_user;
} rvn_file_browser_t;

static void rvn_fb_navigate(rvn_widget_t* widget, const char* path, int remember);
static void rvn_fb_accept(rvn_widget_t* widget);

static rvn_widget_t* rvn_fb_from_user(void* user_data) {
	rvn_widget_t* widget = rvn_widget_get((Rid)(uintptr_t)user_data);
	return (widget && widget->cls->type == R_WIDGET_FILE_BROWSER) ? widget : NULL;
}

/* ---------------------------------------------------------------------------
 *  Entries
 * ------------------------------------------------------------------------- */

static int g_fb_sort_column;
static int g_fb_sort_descending;

static int rvn_fb_compare(const void* a, const void* b) {
	const rvn_dir_entry_t* x = (const rvn_dir_entry_t*)a;
	const rvn_dir_entry_t* y = (const rvn_dir_entry_t*)b;
	int result = 0;
	/* folders first, whatever the order */
	if (x->is_dir != y->is_dir) return x->is_dir ? -1 : 1;
	if (g_fb_sort_column == RVN_FB_COLUMN_SIZE && !x->is_dir) result = x->size < y->size ? -1 : (x->size > y->size ? 1 : 0);
	else if (g_fb_sort_column == RVN_FB_COLUMN_DATE) result = x->modified < y->modified ? -1 : (x->modified > y->modified ? 1 : 0);
	if (result == 0) {
		result = rvn_stricmp(x->name, y->name);
		if (result == 0) result = strcmp(x->name, y->name);
		if (g_fb_sort_column != RVN_FB_COLUMN_NAME) return result;
	}
	return g_fb_sort_descending ? -result : result;
}

static void rvn_fb_free_selection_paths(rvn_file_browser_t* browser) {
	uint32_t i;
	for (i = 0; i < browser->selection_count; i++) rvn_free(browser->selection_paths[i]);
	rvn_free(browser->selection_paths);
	browser->selection_paths = NULL;
	browser->selection_count = 0;
}

static void rvn_fb_free_entries(rvn_file_browser_t* browser) {
	rvn_dir_free(browser->entries, browser->entry_count);
	rvn_free(browser->selected);
	browser->entries = NULL;
	browser->selected = NULL;
	browser->entry_count = 0;
	browser->cursor = browser->anchor = browser->hot = -1;
	rvn_fb_free_selection_paths(browser);
}

static const char* rvn_fb_patterns(const rvn_file_browser_t* browser) {
	if (browser->filter_index < 0 || browser->filter_index >= browser->filter_count) return NULL;
	return browser->filter_patterns[browser->filter_index];
}

/* Reads the current folder again. Returns R_FALSE when it cannot be read (the list is then empty). */
static int rvn_fb_load(rvn_widget_t* widget) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	rvn_dir_entry_t* all = NULL;
	uint32_t count = 0, kept = 0, i;
	char* keep = NULL;
	int ok;

	/* the entry the cursor is on stays selected through a refresh */
	if (browser->cursor >= 0) keep = rvn_strdup(browser->entries[browser->cursor].name);
	rvn_fb_free_entries(browser);
	ok = browser->directory && rvn_dir_read(browser->directory, &all, &count);
	for (i = 0; i < count; i++) {
		rvn_dir_entry_t* entry = &all[i];
		int show = browser->show_hidden || !entry->hidden;
		if (show && !entry->is_dir) {
			show = browser->mode != R_FILE_BROWSER_FOLDER && rvn_pattern_match(rvn_fb_patterns(browser), entry->name);
		}
		if (show) {
			all[kept++] = *entry;
		} else {
			rvn_free(entry->name);
		}
	}
	browser->entries = all;
	browser->entry_count = kept;
	browser->selected = (uint8_t*)rvn_calloc(kept ? kept : 1);
	g_fb_sort_column = browser->sort_column;
	g_fb_sort_descending = browser->sort_descending;
	if (kept > 1) qsort(all, kept, sizeof(rvn_dir_entry_t), rvn_fb_compare);
	if (keep) {
		for (i = 0; i < kept; i++) {
			if (strcmp(all[i].name, keep) == 0) {
				browser->cursor = browser->anchor = (int)i;
				browser->selected[i] = 1;
				break;
			}
		}
		rvn_free(keep);
	}
	browser->bar.offset = 0.0f;
	browser->select_y.started = R_FALSE;
	rvn_widget_invalidate_layout(widget);
	return ok;
}

static char* rvn_fb_entry_path(const rvn_file_browser_t* browser, int index) {
	return rvn_path_child(browser->directory, browser->entries[index].name);
}

/* ---------------------------------------------------------------------------
 *  Fields
 * ------------------------------------------------------------------------- */

static void rvn_fb_set_field(rvn_file_browser_t* browser, rvn_widget_t* field, const char* text) {
	browser->updating++;
	rvn_widget_set_attribute(field, "Text", text ? text : "");
	browser->updating--;
}

static const char* rvn_fb_accept_label(const rvn_file_browser_t* browser) {
	switch (browser->mode) {
		case R_FILE_BROWSER_SAVE: return "Save";
		case R_FILE_BROWSER_FOLDER: return "Select folder";
		default: return "Open";
	}
}

/* Brings the fields in line with the state: path, places, button label, visibility of the controls. */
static void rvn_fb_sync_fields(rvn_widget_t* widget) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	RVisibility controls = browser->controls_hidden ? R_COLLAPSED : R_VISIBLE;
	uint32_t i;
	int place = -1;

	rvn_fb_set_field(browser, browser->path_input, browser->directory);
	rvn_fb_set_field(browser, browser->accept_button, rvn_fb_accept_label(browser));
	browser->name_input->visibility = controls;
	browser->accept_button->visibility = controls;
	browser->cancel_button->visibility = controls;
	browser->filter_combo->visibility = (browser->controls_hidden || browser->filter_count == 0 ||
		browser->mode == R_FILE_BROWSER_FOLDER) ? R_COLLAPSED : R_VISIBLE;
	browser->places_list->visibility = browser->places_hidden ? R_COLLAPSED : R_VISIBLE;
	for (i = 0; i < browser->place_count && browser->directory; i++) {
		if (strcmp(browser->places[i].path, browser->directory) == 0) place = (int)i;
	}
	browser->updating++;
	Rlist_view_set_selected(browser->places_list->id, place);
	browser->updating--;
	rvn_widget_invalidate_layout(widget);
}

/* A field that cannot take what was asked shakes its head. */
static void rvn_fb_refuse(rvn_widget_t* field) {
	rvn_widget_play_transition(field, R_TRANSITION_POP, 0.0f);
}

static void rvn_fb_notify_selection(rvn_widget_t* widget) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	rvn_fb_free_selection_paths(browser);
	/* the name of what is selected goes to the name field, when it is the kind of thing being chosen */
	if (browser->cursor >= 0) {
		const rvn_dir_entry_t* entry = &browser->entries[browser->cursor];
		if (entry->is_dir == (browser->mode == R_FILE_BROWSER_FOLDER)) rvn_fb_set_field(browser, browser->name_input, entry->name);
	}
	rvn_widget_invalidate_paint(widget);
	if (browser->selection_changed) {
		char* path = browser->cursor >= 0 ? rvn_fb_entry_path(browser, browser->cursor) : NULL;
		browser->selection_changed(widget->id, path, browser->selection_user);
		rvn_free(path);
	}
}

/* ---------------------------------------------------------------------------
 *  Navigation
 * ------------------------------------------------------------------------- */

static void rvn_fb_navigate(rvn_widget_t* widget, const char* path, int remember) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	char* absolute = rvn_path_absolute(path);
	char* select = NULL;
	int i;

	if (!absolute) return;
	/* a path to a file shows its folder and selects the file */
	if (rvn_path_exists(absolute) && !rvn_path_is_dir(absolute)) {
		char* parent = rvn_path_parent(absolute);
		select = rvn_strdup(rvn_path_name(absolute));
		rvn_free(absolute);
		absolute = parent;
		if (!absolute) {
			rvn_free(select);
			return;
		}
	}
	if (!rvn_path_is_dir(absolute)) {
		rvn_free(absolute);
		rvn_free(select);
		rvn_fb_set_field(browser, browser->path_input, browser->directory);
		rvn_fb_refuse(browser->path_input);
		return;
	}
	if (browser->directory && strcmp(browser->directory, absolute) == 0) {
		rvn_free(absolute);
	} else {
		rvn_free(browser->directory);
		browser->directory = absolute;
		if (remember) {
			/* a new visit forgets where Forward was going */
			for (i = browser->history_at + 1; i < browser->history_count; i++) rvn_free(browser->history[i]);
			browser->history_count = browser->history_at + 1;
			if (browser->history_count == RVN_FB_HISTORY_MAX) {
				rvn_free(browser->history[0]);
				memmove(&browser->history[0], &browser->history[1], sizeof(char*) * (RVN_FB_HISTORY_MAX - 1));
				browser->history_count--;
			}
			browser->history[browser->history_count++] = rvn_strdup(browser->directory);
			browser->history_at = browser->history_count - 1;
		}
		browser->cursor = -1;
	}
	rvn_fb_load(widget);
	if (select) {
		for (i = 0; i < (int)browser->entry_count; i++) {
			if (strcmp(browser->entries[i].name, select) == 0) {
				memset(browser->selected, 0, browser->entry_count);
				browser->selected[i] = 1;
				browser->cursor = browser->anchor = i;
			}
		}
		rvn_fb_set_field(browser, browser->name_input, select);
		rvn_free(select);
	}
	rvn_fb_sync_fields(widget);
	rvn_fb_notify_selection(widget);
}

static int rvn_fb_nav_enabled(const rvn_file_browser_t* browser, int button) {
	char* parent;
	int has_parent;
	switch (button) {
		case RVN_FB_BACK: return browser->history_at > 0;
		case RVN_FB_FORWARD: return browser->history_at + 1 < browser->history_count;
		default:
			parent = browser->directory ? rvn_path_parent(browser->directory) : NULL;
			has_parent = parent != NULL;
			rvn_free(parent);
			return has_parent;
	}
}

static void rvn_fb_nav(rvn_widget_t* widget, int button) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	if (!rvn_fb_nav_enabled(browser, button)) return;
	if (button == RVN_FB_UP) {
		char* parent = rvn_path_parent(browser->directory);
		char* from = rvn_strdup(browser->directory);
		rvn_fb_navigate(widget, parent, R_TRUE);
		/* the folder that was left is the one selected in its parent */
		Rfile_browser_select(widget->id, rvn_path_name(from));
		rvn_free(parent);
		rvn_free(from);
	} else {
		browser->history_at += button == RVN_FB_BACK ? -1 : 1;
		rvn_fb_navigate(widget, browser->history[browser->history_at], R_FALSE);
	}
}

/* ---------------------------------------------------------------------------
 *  Choosing
 * ------------------------------------------------------------------------- */

static void rvn_fb_fire_accepted(rvn_widget_t* widget, const char* path) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	char* copy = rvn_strdup(path);
	rvn_dispatch_begin();
	if (browser->accepted) browser->accepted(widget->id, copy, browser->accepted_user);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
	rvn_dispatch_end();
	rvn_free(copy);
}

/* What the name field means: a path, relative to the folder shown unless it is absolute. */
static char* rvn_fb_resolve(const rvn_file_browser_t* browser, const char* name) {
	char* joined;
	char* absolute;
	int is_absolute = name[0] == '~' || rvn_path_is_separator(name[0]);
#if defined(_WIN32)
	if (name[0] && name[1] == ':') is_absolute = R_TRUE;
#endif
	if (is_absolute) return rvn_path_absolute(name);
	joined = rvn_path_child(browser->directory, name);
	absolute = rvn_path_absolute(joined);
	rvn_free(joined);
	return absolute;
}

/* Name with the default extension, when it was typed without one. Free with rvn_free. */
static char* rvn_fb_with_extension(const rvn_file_browser_t* browser, const char* name) {
	const char* extension = browser->default_extension;
	const char* patterns = rvn_fb_patterns(browser);
	char first[32];
	size_t length;
	char* result;

	if (strchr(rvn_path_name(name), '.')) return rvn_strdup(name);
	if ((!extension || !extension[0]) && patterns && patterns[0] == '*' && patterns[1] == '.') {
		/* the first pattern of the active filter: "*.xml;*.ui" -> "xml" */
		size_t i = 0;
		while (patterns[2 + i] && patterns[2 + i] != ';' && patterns[2 + i] != ',' && patterns[2 + i] != '*' && i + 1 < sizeof(first)) {
			first[i] = patterns[2 + i];
			i++;
		}
		first[i] = '\0';
		extension = first;
	}
	if (!extension || !extension[0]) return rvn_strdup(name);
	if (extension[0] == '.') extension++;
	length = strlen(name);
	result = (char*)rvn_alloc(length + strlen(extension) + 2);
	memcpy(result, name, length);
	result[length] = '.';
	strcpy(result + length + 1, extension);
	return result;
}

static const char* rvn_fb_name_text(const rvn_file_browser_t* browser) {
	const char* text = Rtext_input_get_text(browser->name_input->id);
	while (*text == ' ') text++;
	return text;
}

static void rvn_fb_accept(rvn_widget_t* widget) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	const char* name = rvn_fb_name_text(browser);
	char* path;

	if (!browser->directory) return;
	if (browser->mode == R_FILE_BROWSER_FOLDER) {
		if (!name[0]) {
			rvn_fb_fire_accepted(widget, browser->directory);
			return;
		}
		path = rvn_fb_resolve(browser, name);
		if (path && rvn_path_is_dir(path)) rvn_fb_fire_accepted(widget, path);
		else rvn_fb_refuse(browser->name_input);
		rvn_free(path);
		return;
	}

	if (!name[0]) {
		/* nothing typed: Open enters the folder the cursor is on */
		if (browser->cursor >= 0 && browser->entries[browser->cursor].is_dir) {
			path = rvn_fb_entry_path(browser, browser->cursor);
			rvn_fb_navigate(widget, path, R_TRUE);
			rvn_free(path);
		} else {
			rvn_fb_refuse(browser->name_input);
		}
		return;
	}
	if (browser->mode == R_FILE_BROWSER_SAVE) {
		char* full = rvn_fb_with_extension(browser, name);
		path = rvn_fb_resolve(browser, full);
		rvn_free(full);
	} else {
		path = rvn_fb_resolve(browser, name);
	}
	if (!path) return;
	if (rvn_path_is_dir(path)) {
		rvn_fb_set_field(browser, browser->name_input, "");
		rvn_fb_navigate(widget, path, R_TRUE);
	} else if (browser->mode == R_FILE_BROWSER_OPEN) {
		if (rvn_path_exists(path)) rvn_fb_fire_accepted(widget, path);
		else rvn_fb_refuse(browser->name_input);
	} else {
		int write = R_TRUE;
		if (rvn_path_exists(path) && widget->window) {
			char message[512];
			Rid id = widget->id;
			snprintf(message, sizeof(message), "\"%.300s\" already exists.\nDo you want to replace it?", rvn_path_name(path));
			write = Rmessage_box(widget->window->id, "Replace the file?", message, R_MESSAGE_YES_NO) == R_MESSAGE_RESULT_YES;
			/* (the application went on running while the question was shown) */
			if (!rvn_widget_get(id)) write = R_FALSE;
		}
		if (write) rvn_fb_fire_accepted(widget, path);
	}
	rvn_free(path);
}

/* A double click or Enter on an entry of the list. */
static void rvn_fb_activate(rvn_widget_t* widget, int index) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	char* path;
	if (index < 0 || index >= (int)browser->entry_count) return;
	path = rvn_fb_entry_path(browser, index);
	if (browser->entries[index].is_dir) {
		rvn_fb_navigate(widget, path, R_TRUE);
	} else if (browser->mode == R_FILE_BROWSER_SAVE) {
		rvn_fb_set_field(browser, browser->name_input, browser->entries[index].name);
		rvn_fb_accept(widget);
	} else {
		rvn_fb_fire_accepted(widget, path);
	}
	rvn_free(path);
}

/* ---------------------------------------------------------------------------
 *  Callbacks of the fields
 * ------------------------------------------------------------------------- */

static void rvn_fb_on_path(Rid input, const char* text, void* user_data) {
	rvn_widget_t* widget = rvn_fb_from_user(user_data);
	RVN_UNUSED(input);
	if (!widget || ((rvn_file_browser_t*)widget)->updating) return;
	rvn_fb_navigate(widget, text, R_TRUE);
}

static void rvn_fb_on_name(Rid input, const char* text, void* user_data) {
	rvn_widget_t* widget = rvn_fb_from_user(user_data);
	RVN_UNUSED(input);
	RVN_UNUSED(text);
	if (!widget || ((rvn_file_browser_t*)widget)->updating) return;
	rvn_fb_accept(widget);
}

static void rvn_fb_on_place(Rid list, int index, void* user_data) {
	rvn_widget_t* widget = rvn_fb_from_user(user_data);
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	RVN_UNUSED(list);
	if (!widget || browser->updating || index < 0 || index >= (int)browser->place_count) return;
	rvn_fb_navigate(widget, browser->places[index].path, R_TRUE);
}

static void rvn_fb_on_filter(Rid combo, int index, void* user_data) {
	rvn_widget_t* widget = rvn_fb_from_user(user_data);
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	RVN_UNUSED(combo);
	if (!widget || browser->updating) return;
	browser->filter_index = index;
	rvn_fb_load(widget);
}

static void rvn_fb_on_accept(Rid button, void* user_data) {
	rvn_widget_t* widget = rvn_fb_from_user(user_data);
	RVN_UNUSED(button);
	if (widget) rvn_fb_accept(widget);
}

static void rvn_fb_on_cancel(Rid button, void* user_data) {
	rvn_widget_t* widget = rvn_fb_from_user(user_data);
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	RVN_UNUSED(button);
	if (widget && browser->cancelled) browser->cancelled(widget->id, NULL, browser->cancelled_user);
}

/* ---------------------------------------------------------------------------
 *  Class
 * ------------------------------------------------------------------------- */

static rvn_widget_t* rvn_fb_child(rvn_widget_t* widget, RWidgetType type) {
	rvn_widget_t* child = rvn_widget_create(type);
	rvn_widget_add_child(widget, child, -1);
	return child;
}

static void rvn_file_browser_init(rvn_widget_t* widget) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	void* self = (void*)(uintptr_t)widget->id;
	char* home;
	uint32_t i;

	browser->mode = R_FILE_BROWSER_OPEN;
	browser->cursor = browser->anchor = browser->hot = -1;
	browser->nav_hot = browser->nav_down = -1;
	browser->history_at = -1;

	browser->path_input = rvn_fb_child(widget, R_WIDGET_TEXT_INPUT);
	browser->places_list = rvn_fb_child(widget, R_WIDGET_LIST_VIEW);
	browser->name_input = rvn_fb_child(widget, R_WIDGET_TEXT_INPUT);
	browser->filter_combo = rvn_fb_child(widget, R_WIDGET_COMBO_BOX);
	browser->cancel_button = rvn_fb_child(widget, R_WIDGET_BUTTON);
	browser->accept_button = rvn_fb_child(widget, R_WIDGET_BUTTON);
	rvn_widget_set_attribute(browser->name_input, "Placeholder", "File name");
	rvn_widget_set_attribute(browser->cancel_button, "Text", "Cancel");
	rvn_widget_set_attribute(browser->accept_button, "Style", "Primary");
	Rtext_input_set_committed_callback(browser->path_input->id, rvn_fb_on_path, self);
	Rtext_input_set_committed_callback(browser->name_input->id, rvn_fb_on_name, self);
	Rlist_view_set_selection_changed_callback(browser->places_list->id, rvn_fb_on_place, self);
	Rcombo_box_set_changed_callback(browser->filter_combo->id, rvn_fb_on_filter, self);
	Rbutton_set_clicked_callback(browser->accept_button->id, rvn_fb_on_accept, self);
	Rbutton_set_clicked_callback(browser->cancel_button->id, rvn_fb_on_cancel, self);

	rvn_fs_places(&browser->places, &browser->place_count);
	for (i = 0; i < browser->place_count; i++) Rlist_view_add_item(browser->places_list->id, browser->places[i].label);

	home = rvn_path_home();
	rvn_fb_navigate(widget, home, R_TRUE);
	rvn_free(home);
}

static void rvn_file_browser_destroy(rvn_widget_t* widget) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	int i;
	rvn_fb_free_entries(browser);
	rvn_fs_places_free(browser->places, browser->place_count);
	for (i = 0; i < browser->filter_count; i++) {
		rvn_free(browser->filter_names[i]);
		rvn_free(browser->filter_patterns[i]);
	}
	rvn_free(browser->filter_names);
	rvn_free(browser->filter_patterns);
	for (i = 0; i < browser->history_count; i++) rvn_free(browser->history[i]);
	rvn_free(browser->default_extension);
	rvn_free(browser->directory);
}

static float rvn_fb_row_height(const rvn_widget_t* widget) {
	return rvn_widget_metric(widget, R_METRIC_ROW_HEIGHT);
}

static rvn_vec2_t rvn_file_browser_measure(rvn_widget_t* widget) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	uint32_t i;
	/* (the children are arranged by hand, but they still need their own measure) */
	for (i = 0; i < widget->child_count; i++) rvn_widget_measure(widget->children[i]);
	if (browser->controls_hidden) return rvn_vec2(browser->places_hidden ? 300.0f : 460.0f, 320.0f);
	return rvn_vec2(720.0f, 440.0f);
}

static void rvn_file_browser_arrange(rvn_widget_t* widget) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	float control = rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT);
	float width = widget->rect.w, height = widget->rect.h;
	float top = control + RVN_FB_GAP, bottom = height, x = 0.0f, list_x = 0.0f;
	int i, places;

	for (i = 0; i < RVN_FB_NAV_COUNT; i++) {
		browser->nav[i] = rvn_rect(x, 0.0f, control, control);
		x += control + 4.0f;
	}
	x += 4.0f;
	rvn_widget_arrange(browser->path_input, x, 0.0f, rvn_maxf(0.0f, width - x), control);

	if (!browser->controls_hidden) {
		float accept = rvn_maxf(browser->accept_button->desired.x, 96.0f);
		float cancel = rvn_maxf(browser->cancel_button->desired.x, 84.0f);
		float filter = rvn_widget_collapsed(browser->filter_combo) ? 0.0f : rvn_minf(210.0f, width * 0.35f);
		float row = height - control;
		rvn_widget_arrange(browser->accept_button, width - accept, row, accept, control);
		rvn_widget_arrange(browser->cancel_button, width - accept - RVN_FB_GAP - cancel, row, cancel, control);
		row -= control + RVN_FB_GAP;
		if (filter > 0.0f) rvn_widget_arrange(browser->filter_combo, width - filter, row, filter, control);
		rvn_widget_arrange(browser->name_input, 0.0f, row, rvn_maxf(0.0f, width - (filter > 0.0f ? filter + RVN_FB_GAP : 0.0f)), control);
		bottom = row - RVN_FB_GAP;
	}

	/* the places leave when the browser is too narrow for them */
	places = !browser->places_hidden && width >= 420.0f;
	if (places) {
		rvn_widget_arrange(browser->places_list, 0.0f, top, RVN_FB_PLACES_WIDTH, rvn_maxf(0.0f, bottom - top));
		list_x = RVN_FB_PLACES_WIDTH + RVN_FB_GAP;
	} else {
		rvn_widget_arrange(browser->places_list, 0.0f, top, 0.0f, 0.0f);
	}
	browser->list = rvn_rect(list_x, top, rvn_maxf(0.0f, width - list_x), rvn_maxf(0.0f, bottom - top));
	browser->header_height = rvn_fb_row_height(widget) + 2.0f;
	browser->bar.content = rvn_fb_row_height(widget) * (float)browser->entry_count;
	browser->bar.viewport = rvn_maxf(0.0f, browser->list.h - browser->header_height - 2.0f);
	rvn_scrollbar_clamp(&browser->bar);
}

static int rvn_file_browser_child_active(rvn_widget_t* widget, rvn_widget_t* child) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	if (child == browser->places_list) return child->rect.w > 0.0f;
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  List: geometry
 * ------------------------------------------------------------------------- */

static rvn_rect_t rvn_fb_track(const rvn_widget_t* widget) {
	const rvn_file_browser_t* browser = (const rvn_file_browser_t*)widget;
	float bar = rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE);
	return rvn_rect(browser->list.x + browser->list.w - bar - 1.0f, browser->list.y + browser->header_height,
		bar, rvn_maxf(0.0f, browser->list.h - browser->header_height - 1.0f));
}

/* Top of the rows, in widget coordinates, for a scroll offset of 0. */
static float rvn_fb_rows_top(const rvn_file_browser_t* browser) {
	return browser->list.y + browser->header_height;
}

static int rvn_fb_row_at(const rvn_widget_t* widget, float x, float y) {
	const rvn_file_browser_t* browser = (const rvn_file_browser_t*)widget;
	float top = rvn_fb_rows_top(browser);
	int row;
	if (x < browser->list.x || x >= browser->list.x + browser->list.w || y < top || y >= browser->list.y + browser->list.h - 1.0f) return -1;
	row = (int)floorf((y - top + browser->bar.offset) / rvn_fb_row_height(widget));
	return (row >= 0 && row < (int)browser->entry_count) ? row : -1;
}

/* Column of the header under a point, or -1. */
static int rvn_fb_column_at(const rvn_widget_t* widget, float x, float y) {
	const rvn_file_browser_t* browser = (const rvn_file_browser_t*)widget;
	float right = browser->list.x + browser->list.w;
	if (x < browser->list.x || x >= right || y < browser->list.y || y >= browser->list.y + browser->header_height) return -1;
	if (browser->list.w < 380.0f) return RVN_FB_COLUMN_NAME;
	if (x >= right - RVN_FB_DATE_WIDTH) return RVN_FB_COLUMN_DATE;
	if (x >= right - RVN_FB_DATE_WIDTH - RVN_FB_SIZE_WIDTH) return RVN_FB_COLUMN_SIZE;
	return RVN_FB_COLUMN_NAME;
}

static int rvn_fb_nav_at(const rvn_widget_t* widget, float x, float y) {
	const rvn_file_browser_t* browser = (const rvn_file_browser_t*)widget;
	int i;
	for (i = 0; i < RVN_FB_NAV_COUNT; i++) {
		if (rvn_rect_contains(browser->nav[i], x, y)) return i;
	}
	return -1;
}

static void rvn_fb_reveal(rvn_widget_t* widget, int row) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	float height = rvn_fb_row_height(widget), top = height * (float)row;
	if (row < 0) return;
	browser->bar.content = height * (float)browser->entry_count;
	if (top < browser->bar.offset) rvn_scrollbar_set(&browser->bar, top);
	else if (top + height > browser->bar.offset + browser->bar.viewport) rvn_scrollbar_set(&browser->bar, top + height - browser->bar.viewport);
	rvn_widget_invalidate_paint(widget);
}

/* Moves the cursor to a row: alone, added to the selection (toggle), or as the end of a range. */
static void rvn_fb_select_row(rvn_widget_t* widget, int row, int toggle, int range) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	int i;
	if (row < 0 || row >= (int)browser->entry_count) return;
	if (!browser->multiple) toggle = range = R_FALSE;
	if (range && browser->anchor >= 0) {
		int from = rvn_mini(browser->anchor, row), to = rvn_maxi(browser->anchor, row);
		for (i = 0; i < (int)browser->entry_count; i++) browser->selected[i] = (uint8_t)(i >= from && i <= to);
	} else if (toggle) {
		browser->selected[row] = (uint8_t)!browser->selected[row];
		browser->anchor = row;
	} else {
		memset(browser->selected, 0, browser->entry_count);
		browser->selected[row] = 1;
		browser->anchor = row;
	}
	browser->cursor = row;
	rvn_fb_reveal(widget, row);
	rvn_fb_notify_selection(widget);
}

/* ---------------------------------------------------------------------------
 *  Paint
 * ------------------------------------------------------------------------- */

static void rvn_fb_format_size(uint64_t size, char* out, size_t out_size) {
	static const char* const units[] = { "B", "KB", "MB", "GB", "TB" };
	double value = (double)size;
	int unit = 0;
	while (value >= 1024.0 && unit < 4) {
		value /= 1024.0;
		unit++;
	}
	if (unit == 0) snprintf(out, out_size, "%u B", (unsigned)size);
	else snprintf(out, out_size, value < 10.0 ? "%.1f %s" : "%.0f %s", value, units[unit]);
}

static void rvn_fb_format_date(int64_t seconds, char* out, size_t out_size) {
	time_t time = (time_t)seconds;
	struct tm* local = seconds > 0 ? localtime(&time) : NULL;
	out[0] = '\0';
	if (local) strftime(out, out_size, "%Y-%m-%d %H:%M", local);
}

/* A folder or a sheet of paper, in a 16 x 14 box. */
static void rvn_fb_paint_icon(RPainter* painter, int is_dir, float x, float y, rvn_color_t accent, rvn_color_t text) {
	if (is_dir) {
		rvn_paint_box(painter, rvn_rect(x, y, 7.5f, 5.0f), 1.5f, accent, 0.0f, 0);
		rvn_paint_box(painter, rvn_rect(x, y + 2.5f, 16.0f, 11.0f), 2.0f, accent, 0.0f, 0);
		rvn_paint_rect(painter, rvn_rect(x + 1.0f, y + 4.5f, 14.0f, 1.0f), rvn_rgba(1.0f, 1.0f, 1.0f, 0.25f));
	} else {
		rvn_color_t line = rvn_color_fade(text, 0.55f);
		rvn_paint_box(painter, rvn_rect(x + 2.5f, y - 0.5f, 11.0f, 14.5f), 2.0f, rvn_color_fade(text, 0.08f), 1.0f, line);
		rvn_paint_rect(painter, rvn_rect(x + 5.0f, y + 4.0f, 6.0f, 1.0f), line);
		rvn_paint_rect(painter, rvn_rect(x + 5.0f, y + 7.0f, 6.0f, 1.0f), line);
		rvn_paint_rect(painter, rvn_rect(x + 5.0f, y + 10.0f, 4.0f, 1.0f), line);
	}
}

static void rvn_fb_paint_nav(rvn_widget_t* widget, RPainter* painter, int button) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	static const float angles[RVN_FB_NAV_COUNT] = { 3.14159265f, 0.0f, -1.57079633f };
	rvn_rect_t rect = browser->nav[button];
	int enabled = painter->enabled && rvn_fb_nav_enabled(browser, button);
	float hover = rvn_paint_spring(painter, &browser->nav_hover[button], enabled && browser->nav_hot == button ? 1.0f : 0.0f, R_MOTION_BOUNCY);
	float press = rvn_paint_spring(painter, &browser->nav_press[button],
		enabled && browser->nav_down == button && browser->nav_hot == button ? 1.0f : 0.0f, R_MOTION_SNAPPY);
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	uint32_t first_instance = painter->list->instance_count, first_command = painter->list->command_count;
	float cx = rect.x + rect.w * 0.5f, cy = rect.y + rect.h * 0.5f, scale;

	if (rvn_widget_glass(widget)) {
		float shown = rvn_clampf(rvn_maxf(hover, press), 0.0f, 1.0f);
		if (shown > 0.01f) {
			float opacity = painter->opacity;
			painter->opacity *= shown;
			rvn_paint_surface(painter, widget, rect, rect.h * 0.5f, RVN_SURFACE_CONTROL, hover, press);
			painter->opacity = opacity;
		}
	} else {
		rvn_color_t fill = rvn_color_mix(rvn_color_fade(rvn_widget_color(widget, R_COLOR_CONTROL_HOVERED), rvn_clampf(hover, 0.0f, 1.0f)),
			rvn_widget_color(widget, R_COLOR_CONTROL_PRESSED), rvn_clampf(press, 0.0f, 1.0f));
		rvn_paint_box(painter, rect, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS), fill, 0.0f, 0);
	}
	rvn_paint_chevron(painter, cx, cy, 10.0f, angles[button], enabled ? text : rvn_color_fade(text, 0.3f));
	scale = 1.0f + 0.16f * hover - 0.2f * press;
	if (scale < 0.999f || scale > 1.001f) {
		rvn_drawlist_transform(painter->list, first_instance, first_command, scale, scale,
			(painter->ox + cx) * painter->scale, (painter->oy + cy) * painter->scale, 0.0f, 0.0f, painter->clip);
	}
}

static void rvn_file_browser_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	rvn_rect_t list = browser->list;
	float row = rvn_fb_row_height(widget), padding = rvn_widget_metric(widget, R_METRIC_PADDING_X) * 0.8f;
	float font = rvn_widget_font_size(widget, 0.0f);
	float bar_width = rvn_scrollbar_needed(&browser->bar) ? rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE) : 0.0f;
	float content_width = list.w - 2.0f - bar_width;
	float offset = rvn_scrollbar_shown(&browser->bar, painter);
	float top = rvn_fb_rows_top(browser);
	int focused = (widget->flags & RVN_WF_FOCUSED) != 0;
	int columns = list.w >= 380.0f;
	float name_width = content_width - (columns ? RVN_FB_SIZE_WIDTH + RVN_FB_DATE_WIDTH : 0.0f);
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	rvn_color_t dim = rvn_color_fade(text, 0.6f);
	rvn_color_t accent = rvn_widget_color(widget, R_COLOR_ACCENT);
	rvn_color_t selection = rvn_widget_color(widget, R_COLOR_SELECTION);
	rvn_vec2_t origin = rvn_widget_origin(widget);
	int first, last, i;

	for (i = 0; i < RVN_FB_NAV_COUNT; i++) rvn_fb_paint_nav(widget, painter, i);
	if (list.w <= 2.0f || list.h <= 2.0f) return;

	rvn_paint_box(painter, list, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS), rvn_widget_color(widget, R_COLOR_INPUT),
		rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH), rvn_widget_color(widget, focused ? R_COLOR_FOCUS : R_COLOR_BORDER));

	/* header: the names of the columns; a chevron shows which one orders the list */
	{
		static const char* const titles[] = { "Name", "Size", "Modified" };
		float x[3], w[3];
		x[0] = list.x + 1.0f;
		w[0] = name_width;
		x[1] = x[0] + name_width;
		w[1] = RVN_FB_SIZE_WIDTH;
		x[2] = x[1] + RVN_FB_SIZE_WIDTH;
		w[2] = RVN_FB_DATE_WIDTH;
		for (i = 0; i < (columns ? 3 : 1); i++) {
			rvn_rect_t cell = rvn_rect(x[i] + padding, list.y + 1.0f, rvn_maxf(0.0f, w[i] - padding * 2.0f), browser->header_height - 1.0f);
			rvn_paint_text_in(painter, R_INVALID_ID, font, cell, i == RVN_FB_COLUMN_SIZE ? 1.0f : 0.0f, titles[i], -1, dim);
			if (browser->sort_column == i) {
				float title_width = rvn_widget_text_width(widget, R_INVALID_ID, 0.0f, titles[i]);
				float cx = i == RVN_FB_COLUMN_SIZE ? cell.x + cell.w - title_width - 10.0f : cell.x + title_width + 10.0f;
				rvn_paint_chevron(painter, cx, cell.y + cell.h * 0.5f, 7.0f, browser->sort_descending ? 1.57079633f : -1.57079633f, dim);
			}
		}
		rvn_paint_rect(painter, rvn_rect(list.x + 1.0f, list.y + browser->header_height - 1.0f, list.w - 2.0f, 1.0f),
			rvn_widget_color(widget, R_COLOR_SEPARATOR));
	}

	rvn_paint_push_clip(painter, rvn_rect(list.x + 1.0f, top, list.w - 2.0f, rvn_maxf(0.0f, list.h - browser->header_height - 1.0f)));
	first = rvn_maxi((int)floorf(offset / row), 0);
	last = rvn_mini((int)browser->entry_count, first + (int)(browser->bar.viewport / row) + 3);
	if (browser->cursor >= 0 && browser->selected[browser->cursor]) {
		/* the row under the cursor glides from where it was */
		float y = rvn_paint_spring(painter, &browser->select_y, row * (float)browser->cursor, R_MOTION_SNAPPY);
		rvn_paint_row(painter, widget, rvn_rect(list.x + 1.0f, top + y - offset, content_width, row),
			focused ? selection : rvn_color_mix(selection, rvn_widget_color(widget, R_COLOR_INPUT), 0.45f));
	} else {
		browser->select_y.started = R_FALSE;
	}
	for (i = first; i < last; i++) {
		const rvn_dir_entry_t* entry = &browser->entries[i];
		rvn_rect_t line = rvn_rect(list.x + 1.0f, top + row * (float)i - offset, content_width, row);
		char buffer[48];
		if (browser->selected[i] && i != browser->cursor) {
			rvn_paint_row(painter, widget, line, rvn_color_mix(selection, rvn_widget_color(widget, R_COLOR_INPUT), focused ? 0.2f : 0.45f));
		} else if (i == browser->hot && !browser->selected[i] && (widget->flags & RVN_WF_HOVERED) && painter->enabled) {
			rvn_paint_row(painter, widget, line, rvn_widget_color(widget, R_COLOR_ROW_HOVERED));
		}
		rvn_fb_paint_icon(painter, entry->is_dir, line.x + padding, line.y + floorf((row - 14.0f) * 0.5f), accent, text);
		rvn_paint_push_clip(painter, rvn_rect(line.x, line.y, rvn_maxf(0.0f, name_width - 6.0f), row));
		rvn_paint_text_in(painter, R_INVALID_ID, font, rvn_rect(line.x + padding + 24.0f, line.y, name_width, row), 0.0f, entry->name, -1, text);
		rvn_paint_pop_clip(painter);
		if (!columns) continue;
		if (!entry->is_dir) {
			rvn_fb_format_size(entry->size, buffer, sizeof(buffer));
			rvn_paint_text_in(painter, R_INVALID_ID, font,
				rvn_rect(line.x + name_width + padding, line.y, RVN_FB_SIZE_WIDTH - padding * 2.0f, row), 1.0f, buffer, -1, dim);
		}
		rvn_fb_format_date(entry->modified, buffer, sizeof(buffer));
		rvn_paint_text_in(painter, R_INVALID_ID, font,
			rvn_rect(line.x + name_width + RVN_FB_SIZE_WIDTH + padding, line.y, RVN_FB_DATE_WIDTH - padding * 2.0f, row), 0.0f, buffer, -1, dim);
	}
	if (browser->entry_count == 0) {
		rvn_paint_text_in(painter, R_INVALID_ID, font, rvn_rect(list.x, top, list.w, rvn_minf(row * 3.0f, list.h - browser->header_height)),
			0.5f, "This folder is empty", -1, dim);
	}
	rvn_paint_pop_clip(painter);
	rvn_scrollbar_paint(&browser->bar, widget, painter, rvn_fb_track(widget), R_TRUE,
		widget->window ? widget->window->mouse_x - origin.x : -1.0f, widget->window ? widget->window->mouse_y - origin.y : -1.0f);
}

/* ---------------------------------------------------------------------------
 *  Events
 * ------------------------------------------------------------------------- */

/* Typing the start of a name moves the cursor to the first entry that has it. */
static int rvn_fb_type(rvn_widget_t* widget, uint32_t codepoint) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	double now = rvn_clock_seconds();
	char encoded[4];
	size_t length = strlen(browser->typed);
	int bytes, i;
	if (codepoint < 32) return R_FALSE;
	if (now - browser->typed_time > 0.9) length = 0;
	bytes = rvn_utf8_encode(codepoint, encoded);
	if (length + (size_t)bytes >= sizeof(browser->typed)) length = 0;
	memcpy(browser->typed + length, encoded, (size_t)bytes);
	length += (size_t)bytes;
	browser->typed[length] = '\0';
	browser->typed_time = now;
	for (i = 0; i < (int)browser->entry_count; i++) {
		if (rvn_strnicmp(browser->entries[i].name, browser->typed, length) == 0) {
			rvn_fb_select_row(widget, i, R_FALSE, R_FALSE);
			break;
		}
	}
	return R_TRUE;
}

static int rvn_file_browser_event(rvn_widget_t* widget, const REvent* event) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	int own = event->target == widget->id;
	int count = (int)browser->entry_count, changed = R_FALSE, row, page;

	if (own && (event->type == R_EVENT_MOUSE_DOWN || event->type == R_EVENT_MOUSE_MOVE || event->type == R_EVENT_MOUSE_UP)) {
		if (rvn_scrollbar_event(&browser->bar, widget, event, rvn_fb_track(widget), R_TRUE, &changed)) {
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		}
	}
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE: {
			int nav = own ? rvn_fb_nav_at(widget, event->local_x, event->local_y) : -1;
			row = own ? rvn_fb_row_at(widget, event->local_x, event->local_y) : -1;
			if (row != browser->hot || nav != browser->nav_hot) {
				browser->hot = row;
				browser->nav_hot = nav;
				rvn_widget_invalidate_paint(widget);
			}
			return browser->nav_down >= 0;
		}
		case R_EVENT_MOUSE_LEAVE:
			browser->hot = -1;
			if (browser->nav_down < 0) browser->nav_hot = -1;
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN: {
			int nav, column;
			if (!own || event->button != R_MOUSE_LEFT) return R_FALSE;
			nav = rvn_fb_nav_at(widget, event->local_x, event->local_y);
			if (nav >= 0) {
				browser->nav_down = browser->nav_hot = nav;
				rvn_widget_capture_mouse(widget);
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			column = rvn_fb_column_at(widget, event->local_x, event->local_y);
			if (column >= 0) {
				/* a second click on the same column reverses the order */
				if (browser->sort_column == column) browser->sort_descending = !browser->sort_descending;
				else browser->sort_descending = R_FALSE;
				browser->sort_column = column;
				rvn_fb_load(widget);
				return R_TRUE;
			}
			row = rvn_fb_row_at(widget, event->local_x, event->local_y);
			if (row >= 0) {
				if (event->clicks == 2) {
					rvn_fb_activate(widget, row);
				} else {
					rvn_fb_select_row(widget, row, (event->modifiers & R_MOD_CTRL) != 0, (event->modifiers & R_MOD_SHIFT) != 0);
				}
			}
			return R_TRUE;
		}
		case R_EVENT_MOUSE_UP: {
			int down = browser->nav_down;
			if (event->button != R_MOUSE_LEFT || down < 0) return R_FALSE;
			browser->nav_down = -1;
			rvn_widget_release_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			if (rvn_fb_nav_at(widget, event->local_x, event->local_y) == down) {
				rvn_spring_kick(&browser->nav_hover[down], 6.0f);
				rvn_fb_nav(widget, down);
			}
			return R_TRUE;
		}
		case R_EVENT_MOUSE_WHEEL:
			if (!own) return R_FALSE;
			if (!rvn_scrollbar_wheel(&browser->bar, -event->wheel_y * rvn_fb_row_height(widget) * 3.0f)) {
				rvn_widget_invalidate_paint(widget);
				return R_FALSE;
			}
			browser->hot = rvn_fb_row_at(widget, event->local_x, event->local_y);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_FOCUS_GAINED:
		case R_EVENT_FOCUS_LOST:
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_TEXT:
			if (!own || (event->modifiers & (R_MOD_CTRL | R_MOD_ALT))) return R_FALSE;
			return rvn_fb_type(widget, event->codepoint);
		case R_EVENT_KEY_DOWN:
			if (!own) return R_FALSE;
			page = rvn_maxi(1, (int)(browser->bar.viewport / rvn_fb_row_height(widget)) - 1);
			switch (event->key) {
				case R_KEY_UP: row = browser->cursor < 0 ? count - 1 : browser->cursor - 1; break;
				case R_KEY_DOWN: row = browser->cursor + 1; break;
				case R_KEY_PAGE_UP: row = browser->cursor - page; break;
				case R_KEY_PAGE_DOWN: row = browser->cursor + page; break;
				case R_KEY_HOME: row = 0; break;
				case R_KEY_END: row = count - 1; break;
				case R_KEY_ENTER:
				case R_KEY_KP_ENTER:
					if (browser->cursor >= 0) rvn_fb_activate(widget, browser->cursor);
					return R_TRUE;
				case R_KEY_BACKSPACE:
					rvn_fb_nav(widget, RVN_FB_UP);
					return R_TRUE;
				case R_KEY_LEFT:
				case R_KEY_RIGHT:
					if (!(event->modifiers & R_MOD_ALT)) return R_FALSE;
					rvn_fb_nav(widget, event->key == R_KEY_LEFT ? RVN_FB_BACK : RVN_FB_FORWARD);
					return R_TRUE;
				case R_KEY_F5:
					rvn_fb_load(widget);
					return R_TRUE;
				case R_KEY_H:
					if (!(event->modifiers & R_MOD_CTRL)) return R_FALSE;
					browser->show_hidden = !browser->show_hidden;
					rvn_fb_load(widget);
					return R_TRUE;
				case R_KEY_A:
					if (!(event->modifiers & R_MOD_CTRL) || !browser->multiple) return R_FALSE;
					for (row = 0; row < count; row++) browser->selected[row] = (uint8_t)!browser->entries[row].is_dir;
					rvn_fb_notify_selection(widget);
					return R_TRUE;
				default:
					return R_FALSE;
			}
			if (count > 0) rvn_fb_select_row(widget, rvn_clampi(row, 0, count - 1), R_FALSE, (event->modifiers & R_MOD_SHIFT) != 0);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static void rvn_fb_set_mode(rvn_widget_t* widget, RFileBrowserMode mode) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	if (browser->mode == mode) return;
	browser->mode = mode;
	rvn_widget_set_attribute(browser->name_input, "Placeholder", mode == R_FILE_BROWSER_FOLDER ? "Folder" : "File name");
	rvn_fb_load(widget);
	rvn_fb_sync_fields(widget);
}

static void rvn_fb_set_filters(rvn_widget_t* widget, const RFileFilter* filters, int count) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	int i;
	for (i = 0; i < browser->filter_count; i++) {
		rvn_free(browser->filter_names[i]);
		rvn_free(browser->filter_patterns[i]);
	}
	rvn_free(browser->filter_names);
	rvn_free(browser->filter_patterns);
	browser->filter_names = browser->filter_patterns = NULL;
	browser->filter_count = 0;
	browser->filter_index = 0;
	browser->updating++;
	Rcombo_box_clear_items(browser->filter_combo->id);
	if (filters && count > 0) {
		browser->filter_names = (char**)rvn_calloc(sizeof(char*) * (size_t)count);
		browser->filter_patterns = (char**)rvn_calloc(sizeof(char*) * (size_t)count);
		for (i = 0; i < count; i++) {
			char label[256];
			const char* patterns = filters[i].patterns ? filters[i].patterns : "*";
			browser->filter_patterns[i] = rvn_strdup(patterns);
			/* "Images (*.png;*.jpg)" */
			if (filters[i].name && filters[i].name[0]) snprintf(label, sizeof(label), "%.120s (%.100s)", filters[i].name, patterns);
			else snprintf(label, sizeof(label), "%.200s", patterns);
			browser->filter_names[i] = rvn_strdup(label);
			Rcombo_box_add_item(browser->filter_combo->id, label);
		}
		browser->filter_count = count;
		Rcombo_box_set_selected(browser->filter_combo->id, 0);
	}
	browser->updating--;
	rvn_fb_load(widget);
	rvn_fb_sync_fields(widget);
}

static int rvn_file_browser_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_file_browser_t* browser = (rvn_file_browser_t*)widget;
	int flag;
	if (RVN_ATTR("Directory") || RVN_ATTR("Path")) {
		rvn_fb_navigate(widget, value, R_TRUE);
		return R_TRUE;
	}
	if (RVN_ATTR("Mode")) {
		if (rvn_stricmp(value, "Open") == 0) rvn_fb_set_mode(widget, R_FILE_BROWSER_OPEN);
		else if (rvn_stricmp(value, "Save") == 0) rvn_fb_set_mode(widget, R_FILE_BROWSER_SAVE);
		else if (rvn_stricmp(value, "Folder") == 0) rvn_fb_set_mode(widget, R_FILE_BROWSER_FOLDER);
		else return R_FALSE;
		return R_TRUE;
	}
	if (RVN_ATTR("Filter")) {
		RFileFilter filter;
		filter.name = NULL;
		filter.patterns = value;
		rvn_fb_set_filters(widget, value[0] ? &filter : NULL, value[0] ? 1 : 0);
		return R_TRUE;
	}
	if (RVN_ATTR("FileName")) {
		rvn_fb_set_field(browser, browser->name_input, value);
		return R_TRUE;
	}
	if (RVN_ATTR("DefaultExtension")) {
		rvn_str_set(&browser->default_extension, value);
		return R_TRUE;
	}
	if (!rvn_parse_bool(value, &flag)) return R_FALSE;
	if (RVN_ATTR("ShowHidden")) {
		browser->show_hidden = flag;
		rvn_fb_load(widget);
	} else if (RVN_ATTR("Multiple")) {
		browser->multiple = flag;
	} else if (RVN_ATTR("Controls")) {
		browser->controls_hidden = !flag;
		rvn_fb_sync_fields(widget);
	} else if (RVN_ATTR("Places")) {
		browser->places_hidden = !flag;
		rvn_fb_sync_fields(widget);
	} else {
		return R_FALSE;
	}
	return R_TRUE;
}

static int rvn_touch_file_browser_pan(rvn_widget_t* widget, float dx, float dy, int coasting) {
	RVN_UNUSED(dx);
	return rvn_scrollbar_pan(&((rvn_file_browser_t*)widget)->bar, -dy, coasting);
}

const rvn_widget_class_t rvn_class_file_browser = {
	.type = R_WIDGET_FILE_BROWSER,
	.name = "FileBrowser",
	.size = sizeof(rvn_file_browser_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_SURFACE,
	.init = rvn_file_browser_init,
	.destroy = rvn_file_browser_destroy,
	.measure = rvn_file_browser_measure,
	.arrange = rvn_file_browser_arrange,
	.paint = rvn_file_browser_paint,
	.event = rvn_file_browser_event,
	.pan = rvn_touch_file_browser_pan,
	.set_attribute = rvn_file_browser_set_attribute,
	.child_active = rvn_file_browser_child_active
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

Rid Rfile_browser_create(void) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_FILE_BROWSER);
	return widget ? widget->id : R_INVALID_ID;
}

void Rfile_browser_set_mode(Rid id, RFileBrowserMode mode) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_FILE_BROWSER);
	if (mode < R_FILE_BROWSER_OPEN || mode > R_FILE_BROWSER_FOLDER) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rfile_browser_set_mode: invalid mode");
		return;
	}
	rvn_fb_set_mode(widget, mode);
}

int Rfile_browser_set_directory(Rid id, const char* path) {
	char* absolute;
	int ok;
	RVN_WIDGET(widget, id, R_WIDGET_FILE_BROWSER, R_FALSE);
	if (!path) return R_FALSE;
	absolute = rvn_path_absolute(path);
	ok = absolute && rvn_path_exists(absolute);
	rvn_free(absolute);
	if (ok) rvn_fb_navigate(widget, path, R_TRUE);
	return ok;
}

const char* Rfile_browser_get_directory(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_FILE_BROWSER, "");
	return ((rvn_file_browser_t*)widget)->directory ? ((rvn_file_browser_t*)widget)->directory : "";
}

void Rfile_browser_refresh(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_FILE_BROWSER);
	rvn_fb_load(widget);
}

void Rfile_browser_set_filters(Rid id, const RFileFilter* filters, int count) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_FILE_BROWSER);
	rvn_fb_set_filters(widget, filters, count);
}

void Rfile_browser_set_show_hidden(Rid id, int show) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_FILE_BROWSER);
	((rvn_file_browser_t*)widget)->show_hidden = show != 0;
	rvn_fb_load(widget);
}

void Rfile_browser_set_multiple(Rid id, int multiple) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_FILE_BROWSER);
	((rvn_file_browser_t*)widget)->multiple = multiple != 0;
}

void Rfile_browser_set_controls_visible(Rid id, int visible) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_FILE_BROWSER);
	((rvn_file_browser_t*)widget)->controls_hidden = !visible;
	rvn_fb_sync_fields(widget);
}

void Rfile_browser_set_places_visible(Rid id, int visible) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_FILE_BROWSER);
	((rvn_file_browser_t*)widget)->places_hidden = !visible;
	rvn_fb_sync_fields(widget);
}

void Rfile_browser_set_file_name(Rid id, const char* name) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_FILE_BROWSER);
	rvn_fb_set_field((rvn_file_browser_t*)widget, ((rvn_file_browser_t*)widget)->name_input, name);
}

void Rfile_browser_set_default_extension(Rid id, const char* extension) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_FILE_BROWSER);
	rvn_str_set(&((rvn_file_browser_t*)widget)->default_extension, extension);
}

static void rvn_fb_build_selection(rvn_file_browser_t* browser) {
	uint32_t i, count = 0;
	if (browser->selection_paths) return;
	for (i = 0; i < browser->entry_count; i++) count += browser->selected[i];
	browser->selection_paths = (char**)rvn_calloc(sizeof(char*) * (count ? count : 1));
	for (i = 0; i < browser->entry_count; i++) {
		if (browser->selected[i]) browser->selection_paths[browser->selection_count++] = rvn_fb_entry_path(browser, (int)i);
	}
}

int Rfile_browser_get_selection_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_FILE_BROWSER, 0);
	rvn_fb_build_selection((rvn_file_browser_t*)widget);
	return (int)((rvn_file_browser_t*)widget)->selection_count;
}

const char* Rfile_browser_get_selection(Rid id, int index) {
	rvn_file_browser_t* browser;
	RVN_WIDGET(widget, id, R_WIDGET_FILE_BROWSER, NULL);
	browser = (rvn_file_browser_t*)widget;
	rvn_fb_build_selection(browser);
	if (index < 0 || index >= (int)browser->selection_count) return NULL;
	return browser->selection_paths[index];
}

void Rfile_browser_select(Rid id, const char* name) {
	rvn_file_browser_t* browser;
	uint32_t i;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_FILE_BROWSER);
	browser = (rvn_file_browser_t*)widget;
	if (!name) return;
	for (i = 0; i < browser->entry_count; i++) {
		if (strcmp(browser->entries[i].name, name) == 0) {
			rvn_fb_select_row(widget, (int)i, R_FALSE, R_FALSE);
			return;
		}
	}
}

void Rfile_browser_set_accepted_callback(Rid id, RFileBrowserCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_FILE_BROWSER);
	((rvn_file_browser_t*)widget)->accepted = callback;
	((rvn_file_browser_t*)widget)->accepted_user = user_data;
}

void Rfile_browser_set_cancelled_callback(Rid id, RFileBrowserCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_FILE_BROWSER);
	((rvn_file_browser_t*)widget)->cancelled = callback;
	((rvn_file_browser_t*)widget)->cancelled_user = user_data;
}

void Rfile_browser_set_selection_changed_callback(Rid id, RFileBrowserCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_FILE_BROWSER);
	((rvn_file_browser_t*)widget)->selection_changed = callback;
	((rvn_file_browser_t*)widget)->selection_user = user_data;
}
