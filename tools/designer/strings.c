/**
 * Raven Designer - strings.c
 * The Translations panel: a CSV table of the folder as a grid, one row per
 * key, one column per language. A cell left empty is marked (the fallback
 * language shows instead). Changes show in the preview while typing; the
 * language of the preview is chosen above it, right to left included.
 */

#include "designer.h"
#include "../crash_report.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COLUMNS 32
#define MAX_SHOWN 600

typedef struct row_t {
	char* cells[MAX_COLUMNS];  /* cells[0] is the key */
	int comment;               /* a line starting with #: kept as it is in cells[0] */
} row_t;

typedef struct table_t {
	char* path;
	int columns;
	char* headers[MAX_COLUMNS];
	row_t* rows;
	int row_count;
	int modified;
} table_t;

static struct {
	Rid panel, grid, file_combo, filter, new_language, language_combo, refresh_timer;
	table_t* tables;
	int table_count;
	int current;
	int syncing;
	char** keys;
	int key_count;
	char language[64];
} g_s;

/* ---------------------------------------------------------------------------
 *  CSV
 * ------------------------------------------------------------------------- */

/* One record: the fields, as CSV quotes them. Returns the start of the next record. */
static const char* read_record(const char* p, char** fields, int max, int* count) {
	ds_buffer_t field = { 0 };
	*count = 0;
	for (;;) {
		field.size = 0;
		if (field.data) field.data[0] = 0;
		if (*p == '"') {
			p++;
			for (;;) {
				if (!*p) break;
				if (*p == '"') {
					if (p[1] == '"') { ds_buffer_add(&field, "\"", 1); p += 2; continue; }
					p++;
					break;
				}
				ds_buffer_add(&field, p, 1);
				p++;
			}
			while (*p && *p != ',' && *p != '\n' && *p != '\r') p++;
		} else {
			const char* start = p;
			while (*p && *p != ',' && *p != '\n' && *p != '\r') p++;
			ds_buffer_add(&field, start, (size_t)(p - start));
		}
		if (*count < max) fields[(*count)++] = ds_strdup(field.data ? field.data : "");
		if (*p == ',') { p++; continue; }
		if (*p == '\r') p++;
		if (*p == '\n') p++;
		break;
	}
	ds_buffer_free(&field);
	return p;
}

static void free_table(table_t* t) {
	int i, j;
	free(t->path);
	for (i = 0; i < t->columns; i++) free(t->headers[i]);
	for (i = 0; i < t->row_count; i++) for (j = 0; j < MAX_COLUMNS; j++) free(t->rows[i].cells[j]);
	free(t->rows);
	memset(t, 0, sizeof(*t));
}

static int load_table(table_t* t, const char* path) {
	size_t size = 0;
	char* text = Rfile_read(path, &size);
	const char* p;
	memset(t, 0, sizeof(*t));
	if (!text) return R_FALSE;
	t->path = ds_strdup(path);
	p = text;
	if ((unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF) p += 3;
	/* the first record names the columns */
	while (*p == '#' || *p == '\n' || *p == '\r') {
		while (*p && *p != '\n') p++;
		if (*p) p++;
	}
	if (*p) p = read_record(p, t->headers, MAX_COLUMNS, &t->columns);
	while (*p) {
		row_t row;
		int count = 0, i;
		memset(&row, 0, sizeof(row));
		if (*p == '\n' || *p == '\r') { p++; continue; }
		if (*p == '#') {
			const char* end = p;
			while (*end && *end != '\n' && *end != '\r') end++;
			row.cells[0] = (char*)malloc((size_t)(end - p) + 1);
			memcpy(row.cells[0], p, (size_t)(end - p));
			row.cells[0][end - p] = 0;
			row.comment = 1;
			p = end;
		} else {
			p = read_record(p, row.cells, MAX_COLUMNS, &count);
		}
		for (i = 0; i < t->columns; i++) if (!row.cells[i]) row.cells[i] = ds_strdup("");
		t->rows = (row_t*)realloc(t->rows, sizeof(row_t) * (size_t)(t->row_count + 1));
		t->rows[t->row_count++] = row;
	}
	Rmemory_free(text);
	if (t->columns == 0) {
		t->headers[0] = ds_strdup("key");
		t->headers[1] = ds_strdup("en");
		t->columns = 2;
	}
	return R_TRUE;
}

static void write_field(ds_buffer_t* out, const char* text) {
	const char* p;
	int quote = strpbrk(text, ",\"\n\r") != NULL || (text[0] == ' ') || (text[0] && text[strlen(text) - 1] == ' ') || text[0] == '#';
	if (!quote) {
		ds_buffer_add(out, text, strlen(text));
		return;
	}
	ds_buffer_add(out, "\"", 1);
	for (p = text; *p; p++) {
		if (*p == '"') ds_buffer_add(out, "\"\"", 2);
		else ds_buffer_add(out, p, 1);
	}
	ds_buffer_add(out, "\"", 1);
}

static int save_table(table_t* t) {
	ds_buffer_t out = { 0 };
	int i, j, ok;
	for (i = 0; i < t->columns; i++) {
		if (i) ds_buffer_add(&out, ",", 1);
		write_field(&out, t->headers[i]);
	}
	ds_buffer_add(&out, "\n", 1);
	for (i = 0; i < t->row_count; i++) {
		if (t->rows[i].comment) {
			ds_buffer_add(&out, t->rows[i].cells[0], strlen(t->rows[i].cells[0]));
		} else {
			for (j = 0; j < t->columns; j++) {
				if (j) ds_buffer_add(&out, ",", 1);
				write_field(&out, t->rows[i].cells[j] ? t->rows[i].cells[j] : "");
			}
		}
		ds_buffer_add(&out, "\n", 1);
	}
	ok = Rfile_write(t->path, out.data, out.size);
	ds_buffer_free(&out);
	if (ok) t->modified = 0;
	return ok;
}

/* ---------------------------------------------------------------------------
 *  The grid
 * ------------------------------------------------------------------------- */

static table_t* current(void) {
	return g_s.current >= 0 && g_s.current < g_s.table_count ? &g_s.tables[g_s.current] : NULL;
}

static void update_title(void) {
	table_t* t = current();
	char title[300];
	/* (the tab says when a table has unsaved changes) */
	snprintf(title, sizeof(title), "Translations%s", t && t->modified ? " \xE2\x80\xA2" : "");
	Rbutton_set_text(Rwidget_find(g_ds_window, "tab.strings"), title);
}

static void mark_cell(Rid input, int missing) {
	if (missing) {
		float r, g, b, a;
		Rtheme_get_color(R_COLOR_DANGER, &r, &g, &b, &a);
		Rwidget_set_color(input, R_COLOR_CONTROL_BORDER, r, g, b, 0.9f);
		Rwidget_set_color(input, R_COLOR_INPUT, r, g, b, 0.12f);
		Rwidget_set_tooltip(input, "Missing: the program shows the text of the first language");
	} else {
		Rwidget_clear_color(input, R_COLOR_CONTROL_BORDER);
		Rwidget_clear_color(input, R_COLOR_INPUT);
		Rwidget_set_tooltip(input, "");
	}
}

static void on_refresh_timer(Rid timer, void* user_data) {
	(void)timer;
	(void)user_data;
	g_s.refresh_timer = R_INVALID_ID;
	designer_refresh_preview();
}

static void schedule_refresh(void) {
	if (g_s.refresh_timer != R_INVALID_ID && Rtimer_is_valid(g_s.refresh_timer)) Rtimer_destroy(g_s.refresh_timer);
	g_s.refresh_timer = Rtimer_create(0.35, R_FALSE, on_refresh_timer, NULL);
}

static void collect_keys(void);

/* user_data: row * MAX_COLUMNS + column */
static void on_cell(Rid input, const char* text, void* user_data) {
	table_t* t = current();
	int code = (int)(size_t)user_data, row = code / MAX_COLUMNS, column = code % MAX_COLUMNS;
	if (g_s.syncing || !t || row >= t->row_count || column >= t->columns) return;
	free(t->rows[row].cells[column]);
	t->rows[row].cells[column] = ds_strdup(text);
	if (column > 0) {
		mark_cell(input, !text[0] && column > 1);
		/* the preview shows it at once */
		Rlocale_set_text(t->headers[column], t->rows[row].cells[0], text);
		schedule_refresh();
	} else {
		collect_keys();
	}
	if (!t->modified) {
		t->modified = 1;
		update_title();
	}
}

static void rebuild_grid(void);

static void on_delete_row(Rid button, void* user_data) {
	table_t* t = current();
	int row = (int)(size_t)user_data, j;
	(void)button;
	if (!t || row >= t->row_count) return;
	for (j = 0; j < MAX_COLUMNS; j++) free(t->rows[row].cells[j]);
	memmove(&t->rows[row], &t->rows[row + 1], sizeof(row_t) * (size_t)(t->row_count - row - 1));
	t->row_count--;
	t->modified = 1;
	update_title();
	collect_keys();
	rebuild_grid();
}

static int matches(const row_t* row, int columns, const char* filter) {
	int i;
	if (!filter[0]) return 1;
	for (i = 0; i < columns; i++) {
		const char* cell = row->cells[i];
		size_t n = strlen(filter), k;
		if (!cell) continue;
		for (k = 0; cell[k]; k++) {
			size_t m;
			for (m = 0; m < n && cell[k + m] && tolower((unsigned char)cell[k + m]) == tolower((unsigned char)filter[m]); m++) {}
			if (m == n) return 1;
		}
	}
	return 0;
}

static void rebuild_grid(void) {
	table_t* t = current();
	const char* filter = Rtext_input_get_text(g_s.filter);
	Rid header;
	int i, j, shown = 0;
	g_s.syncing++;
	Rwidget_clear_children(g_s.grid);
	if (!t) {
		Rwidget_add_child(g_s.grid, Rlabel_create(g_folder ? "The folder has no .csv table: New table creates strings.csv."
			: "Open a folder to edit its translations."));
		g_s.syncing--;
		update_title();
		return;
	}
	header = Rwidget_create(R_WIDGET_HBOX);
	Rpanel_set_spacing(header, 6.0f);
	for (j = 0; j < t->columns; j++) {
		Rid label = Rlabel_create(j == 0 ? "Key" : t->headers[j]);
		Rwidget_set_attribute(label, "TextSize", "13");
		if (j == 0) Rwidget_set_size(label, 220.0f, -1.0f);
		else Rslot_set_fill(label, 1.0f);
		Rwidget_add_child(header, label);
	}
	{
		Rid spacer = Rwidget_create(R_WIDGET_SPACER);
		Rwidget_set_size(spacer, 28.0f, 1.0f);
		Rwidget_add_child(header, spacer);
	}
	Rwidget_add_child(g_s.grid, header);
	for (i = 0; i < t->row_count && shown < MAX_SHOWN; i++) {
		Rid line, remove;
		if (t->rows[i].comment || !matches(&t->rows[i], t->columns, filter)) continue;
		line = Rwidget_create(R_WIDGET_HBOX);
		Rpanel_set_spacing(line, 6.0f);
		for (j = 0; j < t->columns; j++) {
			Rid input = Rwidget_create(R_WIDGET_TEXT_INPUT);
			Rtext_input_set_text(input, t->rows[i].cells[j] ? t->rows[i].cells[j] : "");
			if (j == 0) Rwidget_set_size(input, 220.0f, -1.0f);
			else Rslot_set_fill(input, 1.0f);
			/* the column of a right-to-left language is written from the right */
			if (j > 0) {
				const char* code = t->headers[j];
				if (!strncmp(code, "ar", 2) || !strncmp(code, "he", 2) || !strncmp(code, "fa", 2) || !strncmp(code, "ur", 2))
					Rwidget_set_direction(input, R_DIRECTION_RTL);
			}
			if (j > 1) mark_cell(input, !(t->rows[i].cells[j] && t->rows[i].cells[j][0]));
			Rtext_input_set_changed_callback(input, on_cell, (void*)(size_t)(i * MAX_COLUMNS + j));
			Rwidget_add_child(line, input);
		}
		remove = Rwidget_create(R_WIDGET_BUTTON);
		Rbutton_set_text(remove, "x");
		Rwidget_set_attribute(remove, "Style", "Flat");
		Rwidget_set_tooltip(remove, "Delete this key");
		Rwidget_set_size(remove, 28.0f, -1.0f);
		Rbutton_set_clicked_callback(remove, on_delete_row, (void*)(size_t)i);
		Rwidget_add_child(line, remove);
		Rwidget_add_child(g_s.grid, line);
		shown++;
	}
	if (shown == MAX_SHOWN) Rwidget_add_child(g_s.grid, Rlabel_create("(more keys: type in the search box to find them)"));
	g_s.syncing--;
	update_title();
}

/* ---------------------------------------------------------------------------
 *  Tables of the folder
 * ------------------------------------------------------------------------- */

static void collect_keys(void) {
	int i, j;
	for (i = 0; i < g_s.key_count; i++) free(g_s.keys[i]);
	free(g_s.keys);
	g_s.keys = NULL;
	g_s.key_count = 0;
	for (i = 0; i < g_s.table_count; i++) {
		for (j = 0; j < g_s.tables[i].row_count; j++) {
			const row_t* row = &g_s.tables[i].rows[j];
			if (row->comment || !row->cells[0] || !row->cells[0][0]) continue;
			g_s.keys = (char**)realloc(g_s.keys, sizeof(char*) * (size_t)(g_s.key_count + 1));
			g_s.keys[g_s.key_count++] = ds_strdup(row->cells[0]);
		}
	}
}

int strings_key_count(void) {
	return g_s.key_count;
}

const char* strings_key(int index) {
	return index >= 0 && index < g_s.key_count ? g_s.keys[index] : "";
}

static void find_tables(const char* folder, int depth) {
	ds_entry_t* entries;
	int count = ds_list_dir(folder, &entries), i;
	for (i = 0; i < count; i++) {
		char* full;
		if (entries[i].name[0] == '.' || ds_iequal(entries[i].name, "build") || !strncmp(entries[i].name, "build-", 6)) continue;
		full = ds_join(folder, entries[i].name);
		if (entries[i].is_dir) {
			if (depth < 3) find_tables(full, depth + 1);
		} else if (ds_iequal(ds_extension(entries[i].name), "csv")) {
			table_t t;
			if (load_table(&t, full)) {
				g_s.tables = (table_t*)realloc(g_s.tables, sizeof(table_t) * (size_t)(g_s.table_count + 1));
				g_s.tables[g_s.table_count++] = t;
			}
		}
		free(full);
	}
	free(entries);
}

static void fill_language_combo(void) {
	int i, count = Rlocale_get_language_count(), selected = 0;
	g_s.syncing++;
	Rcombo_box_clear_items(g_s.language_combo);
	Rcombo_box_add_item(g_s.language_combo, "First column of the tables");
	for (i = 0; i < count; i++) {
		const char* code = Rlocale_get_language_at(i);
		char item[96];
		snprintf(item, sizeof(item), "%s", code);
		Rcombo_box_add_item(g_s.language_combo, item);
		if (g_s.language[0] && !strcmp(code, g_s.language)) selected = i + 1;
	}
	Rcombo_box_set_selected(g_s.language_combo, selected);
	g_s.syncing--;
}

static void apply_language(void) {
	if (g_s.language[0] && Rlocale_set_language(g_s.language)) {
		Rwidget_set_direction(designer_get_frame(), Rlocale_is_rtl() ? R_DIRECTION_RTL : R_DIRECTION_LTR);
	} else {
		if (Rlocale_get_language_count() > 0) Rlocale_set_language(Rlocale_get_language_at(0));
		Rwidget_set_direction(designer_get_frame(), Rlocale_get_language_count() > 0 && Rlocale_is_rtl() ? R_DIRECTION_RTL : R_DIRECTION_LTR);
	}
}

void strings_reload(void) {
	int i;
	char* previous_path = current() ? ds_strdup(current()->path) : NULL;
	for (i = 0; i < g_s.table_count; i++) free_table(&g_s.tables[i]);
	free(g_s.tables);
	g_s.tables = NULL;
	g_s.table_count = 0;
	g_s.current = -1;
	Rlocale_clear();
	if (g_folder) find_tables(g_folder, 0);
	for (i = 0; i < g_s.table_count; i++) {
		Rlocale_load_csv(g_s.tables[i].path);
		if (previous_path && ds_same_path(previous_path, g_s.tables[i].path)) g_s.current = i;
	}
	free(previous_path);
	if (g_s.current < 0 && g_s.table_count) g_s.current = 0;
	g_s.syncing++;
	Rcombo_box_clear_items(g_s.file_combo);
	for (i = 0; i < g_s.table_count; i++) Rcombo_box_add_item(g_s.file_combo, ds_basename(g_s.tables[i].path));
	if (g_s.current >= 0) Rcombo_box_set_selected(g_s.file_combo, g_s.current);
	g_s.syncing--;
	collect_keys();
	fill_language_combo();
	apply_language();
	rebuild_grid();
	designer_refresh_preview();
}

void strings_open(const char* path) {
	int i;
	for (i = 0; i < g_s.table_count; i++) {
		if (ds_same_path(g_s.tables[i].path, path)) {
			g_s.current = i;
			g_s.syncing++;
			Rcombo_box_set_selected(g_s.file_combo, i);
			g_s.syncing--;
			rebuild_grid();
			return;
		}
	}
	/* (a table outside of the folder) */
	ds_status("%s is not in the folder open.", ds_basename(path));
}

int strings_any_modified(void) {
	int i;
	for (i = 0; i < g_s.table_count; i++) if (g_s.tables[i].modified) return R_TRUE;
	return R_FALSE;
}

int strings_save(void) {
	int i, ok = R_TRUE;
	for (i = 0; i < g_s.table_count; i++) {
		if (g_s.tables[i].modified && !save_table(&g_s.tables[i])) {
			ds_status("Cannot write %s", g_s.tables[i].path);
			ok = R_FALSE;
		}
	}
	update_title();
	return ok;
}

/* ---------------------------------------------------------------------------
 *  Commands
 * ------------------------------------------------------------------------- */

static void add_key(void) {
	table_t* t = current();
	row_t row;
	char key[64];
	int n = 1, i, taken;
	if (!t) return;
	do {
		snprintf(key, sizeof(key), "new.key%d", n++);
		taken = 0;
		for (i = 0; i < t->row_count; i++) if (t->rows[i].cells[0] && !strcmp(t->rows[i].cells[0], key)) taken = 1;
	} while (taken);
	memset(&row, 0, sizeof(row));
	row.cells[0] = ds_strdup(key);
	for (i = 1; i < t->columns; i++) row.cells[i] = ds_strdup("");
	t->rows = (row_t*)realloc(t->rows, sizeof(row_t) * (size_t)(t->row_count + 1));
	t->rows[t->row_count++] = row;
	t->modified = 1;
	Rtext_input_set_text(g_s.filter, "");
	collect_keys();
	rebuild_grid();
	ds_status("Key %s added: rename it in the first column.", key);
}

static void add_language(void) {
	table_t* t = current();
	const char* code = Rtext_input_get_text(g_s.new_language);
	int i;
	if (!t) return;
	if (!code[0] || strchr(code, ',') || strchr(code, '"')) {
		ds_status("Type the code of the language first (fr, de, ar, pt-BR...).");
		return;
	}
	for (i = 0; i < t->columns; i++) {
		if (ds_iequal(t->headers[i], code)) {
			ds_status("The table already has %s.", code);
			return;
		}
	}
	if (t->columns >= MAX_COLUMNS) return;
	t->headers[t->columns] = ds_strdup(code);
	for (i = 0; i < t->row_count; i++) if (!t->rows[i].comment) t->rows[i].cells[t->columns] = ds_strdup("");
	t->columns++;
	t->modified = 1;
	Rtext_input_set_text(g_s.new_language, "");
	rebuild_grid();
	ds_status("Language %s added: its empty cells are marked.", code);
}

static void add_table(void) {
	char* path;
	if (!g_folder) {
		ds_status("Open a folder first.");
		return;
	}
	path = ds_join(g_folder, "strings.csv");
	if (!ds_file_exists(path)) Rfile_write(path, "key,en\n", 7);
	free(path);
	strings_reload();
	folder_refresh();
}

static void action_strings(Rid sender, const char* action, void* user_data) {
	crash_report_note("action %s", action);
	(void)sender;
	(void)user_data;
	if (!strcmp(action, "strings.add_key")) add_key();
	else if (!strcmp(action, "strings.add_language")) add_language();
	else if (!strcmp(action, "strings.add_table")) add_table();
	else if (!strcmp(action, "strings.save")) {
		if (strings_save()) {
			ds_status("Translations saved.");
			strings_reload();
		}
	} else if (!strcmp(action, "strings.filter")) {
		if (!g_s.syncing) rebuild_grid();
	} else if (!strcmp(action, "strings.reload")) {
		strings_reload();
	}
}

static void on_file_changed(Rid combo, int index, void* user_data) {
	(void)combo;
	(void)user_data;
	if (g_s.syncing || index < 0 || index >= g_s.table_count) return;
	g_s.current = index;
	rebuild_grid();
}

static void on_language_changed(Rid combo, int index, void* user_data) {
	(void)combo;
	(void)user_data;
	if (g_s.syncing) return;
	if (index <= 0) g_s.language[0] = 0;
	else snprintf(g_s.language, sizeof(g_s.language), "%s", Rlocale_get_language_at(index - 1));
	apply_language();
	designer_refresh_preview();
	ds_status(g_s.language[0] ? "The preview is in %s%s." : "The preview is in the first language.", g_s.language,
		g_s.language[0] && Rlocale_is_rtl() ? ", right to left" : "");
}

/* ---------------------------------------------------------------------------
 *  The keys under an input: its text starts with '@', the keys of the tables
 *  that hold what follows are offered (those that start with it first)
 * ------------------------------------------------------------------------- */

#define PICKER_MAX 14

static struct {
	Rid menu, input;
	/* the window of the input (an island in the camera housing has a window of its own) */
	Rid window;
	ds_key_applied_t applied;
	void* user_data;
	char keys[PICKER_MAX][128];
	int count;
	/* set while a key chosen is written: the input changes, the list does not come back */
	int picking;
} g_kp;

static int picker_shown(void) {
	int i, count;
	if (g_kp.menu == R_INVALID_ID || !Rwidget_is_valid(g_kp.menu)) return R_FALSE;
	if (!Rwindow_is_valid(g_kp.window)) return R_FALSE;
	count = Rwindow_get_popup_count(g_kp.window);
	for (i = 0; i < count; i++) if (Rwindow_get_popup(g_kp.window, i) == g_kp.menu) return R_TRUE;
	return R_FALSE;
}

static void picker_close(void) {
	if (picker_shown()) Rwindow_close_popups(g_kp.window);
}

static void on_key_picked(Rid item, void* user_data) {
	int index = (int)(size_t)user_data;
	char text[160];
	(void)item;
	if (index < 0 || index >= g_kp.count || g_kp.input == R_INVALID_ID || !Rwidget_is_valid(g_kp.input)) return;
	snprintf(text, sizeof(text), "@%s", g_kp.keys[index]);
	g_kp.picking++;
	Rtext_input_set_text(g_kp.input, text);
	if (g_kp.applied) g_kp.applied(g_kp.input, text, g_kp.user_data);
	g_kp.picking--;
	/* (the keyboard stays in the input, to go on) */
	if (Rwidget_is_valid(g_kp.input)) Rwidget_focus(g_kp.input);
}

static int key_starts_with(const char* key, const char* start) {
	for (; *start; key++, start++) {
		char x = *key, y = *start;
		if (!x) return R_FALSE;
		if (x >= 'A' && x <= 'Z') x = (char)(x + 32);
		if (y >= 'A' && y <= 'Z') y = (char)(y + 32);
		if (x != y) return R_FALSE;
	}
	return R_TRUE;
}

void strings_key_picker(Rid input, const char* text, ds_key_applied_t applied, void* user_data) {
	const char* filter;
	int i, pass, total = 0, exact = R_FALSE;
	float x, y, w, h;
	if (g_kp.picking) return;
	if (!text || text[0] != '@' || g_s.key_count == 0 || !Rwidget_is_focused(input)) {
		if (g_kp.input == input) picker_close();
		return;
	}
	filter = text + 1;
	g_kp.count = 0;
	for (pass = 0; pass < 2; pass++) {
		for (i = 0; i < g_s.key_count; i++) {
			const char* key = g_s.keys[i];
			int starts = key_starts_with(key, filter);
			if (pass == 0 ? !starts : (starts || !ds_contains(key, filter))) continue;
			if (ds_iequal(key, filter)) exact = R_TRUE;
			total++;
			if (g_kp.count < PICKER_MAX) snprintf(g_kp.keys[g_kp.count++], sizeof(g_kp.keys[0]), "%s", key);
		}
	}
	picker_close();
	/* nothing holds it, or the key is written in full */
	if (g_kp.count == 0 || (total == 1 && exact)) return;
	if (g_kp.menu != R_INVALID_ID && Rwidget_is_valid(g_kp.menu)) Rwidget_destroy(g_kp.menu);
	g_kp.menu = Rwidget_create(R_WIDGET_MENU);
	for (i = 0; i < g_kp.count; i++) {
		char label[300];
		const char* shown = Rtr(g_kp.keys[i]);
		Rid item;
		/* the key, and what it says in the language of the preview */
		if (shown && shown[0] && strcmp(shown, g_kp.keys[i]) != 0) snprintf(label, sizeof(label), "%s      \xE2\x80\x9C%.60s\xE2\x80\x9D", g_kp.keys[i], shown);
		else snprintf(label, sizeof(label), "%s", g_kp.keys[i]);
		item = Rmenu_add_item(g_kp.menu, label);
		Rmenu_item_set_clicked_callback(item, on_key_picked, (void*)(size_t)i);
	}
	if (total > g_kp.count) {
		char more[96];
		Rid item;
		snprintf(more, sizeof(more), "%d more: type to narrow", total - g_kp.count);
		Rmenu_add_separator(g_kp.menu);
		item = Rmenu_add_item(g_kp.menu, more);
		Rwidget_set_enabled(item, R_FALSE);
	}
	g_kp.input = input;
	g_kp.window = Rwidget_get_window(input) != R_INVALID_ID ? Rwidget_get_window(input) : g_ds_window;
	g_kp.applied = applied;
	g_kp.user_data = user_data;
	/* under the input; the keyboard stays in it (Up, Down and Enter choose, Escape closes) */
	Rwidget_get_rect(input, &x, &y, &w, &h);
	Rmenu_open(g_kp.menu, g_kp.window, x, y + h + 2.0f);
}

int strings_key_picker_count(void) {
	return picker_shown() ? g_kp.count : 0;
}

void strings_init(void) {
	g_kp.menu = R_INVALID_ID;
	g_kp.input = R_INVALID_ID;
	g_kp.window = R_INVALID_ID;
	g_s.panel = Rwidget_find(g_ds_window, "panel.strings");
	g_s.grid = Rwidget_find(g_ds_window, "strings.grid");
	g_s.file_combo = Rwidget_find(g_ds_window, "strings.file");
	g_s.filter = Rwidget_find(g_ds_window, "strings.filter");
	g_s.new_language = Rwidget_find(g_ds_window, "strings.language");
	g_s.language_combo = Rwidget_find(g_ds_window, "preview.language");
	g_s.refresh_timer = R_INVALID_ID;
	g_s.current = -1;
	Rcombo_box_set_changed_callback(g_s.file_combo, on_file_changed, NULL);
	Rcombo_box_set_changed_callback(g_s.language_combo, on_language_changed, NULL);
	Raction_bind("strings.add_key", action_strings, NULL);
	Raction_bind("strings.add_language", action_strings, NULL);
	Raction_bind("strings.add_table", action_strings, NULL);
	Raction_bind("strings.save", action_strings, NULL);
	Raction_bind("strings.filter", action_strings, NULL);
	Raction_bind("strings.reload", action_strings, NULL);
	fill_language_combo();
	rebuild_grid();
}

void strings_shutdown(void) {
	int i;
	if (g_kp.menu != R_INVALID_ID && Rwidget_is_valid(g_kp.menu)) {
		picker_close();
		Rwidget_destroy(g_kp.menu);
	}
	g_kp.menu = R_INVALID_ID;
	for (i = 0; i < g_s.table_count; i++) free_table(&g_s.tables[i]);
	free(g_s.tables);
	for (i = 0; i < g_s.key_count; i++) free(g_s.keys[i]);
	free(g_s.keys);
	memset(&g_s, 0, sizeof(g_s));
}

int strings_language_count(void) {
	return Rlocale_get_language_count();
}

const char* strings_language_at(int index) {
	return Rlocale_get_language_at(index);
}

void strings_set_preview_language(const char* code) {
	snprintf(g_s.language, sizeof(g_s.language), "%s", code ? code : "");
	fill_language_combo();
	apply_language();
	designer_refresh_preview();
}

/* For the scripts of the tests: a cell by key and column. */
int strings_set_cell(const char* key, int column, const char* text) {
	table_t* t = current();
	int i;
	if (!t || column <= 0 || column >= t->columns) return R_FALSE;
	for (i = 0; i < t->row_count; i++) {
		if (!t->rows[i].comment && t->rows[i].cells[0] && !strcmp(t->rows[i].cells[0], key)) {
			free(t->rows[i].cells[column]);
			t->rows[i].cells[column] = ds_strdup(text);
			Rlocale_set_text(t->headers[column], key, text);
			t->modified = 1;
			rebuild_grid();
			designer_refresh_preview();
			return R_TRUE;
		}
	}
	return R_FALSE;
}
