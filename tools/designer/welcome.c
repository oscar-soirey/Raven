/**
 * Raven Designer - welcome.c
 * The start page: what the designer shows when nothing is open. Open a
 * folder or an interface, and the recent folders (the last opened first,
 * with how many interfaces they hold; a click opens one, the cross takes it
 * out of the list).
 *
 * Also the preferences of the designer, kept between two runs in ds_config_dir():
 *   designer.ini         key=value (the theme of the designer, accessibility)
 *   recent-folders.txt   one folder per line: "<seconds since 1970>\t<folder>"
 */

#include "designer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_RECENT 30
#define MAX_PREFS 64
#define ROW_HEIGHT 62.0f
#define ROW_GAP 4.0f

typedef struct recent_t {
	char* path;
	char* name;
	int interfaces;         /* the .xml files at its top */
	double opened;          /* seconds since 1970 */
	int missing;            /* the folder is not there any more */
} recent_t;

static struct {
	Rid page, list, search, empty, back;
	recent_t recent[MAX_RECENT];
	int count;
	/* the rows shown (the search filters them): indices into recent */
	int shown[MAX_RECENT];
	int shown_count;
	int hovered, hovered_remove, selected;
	char* keys[MAX_PREFS];
	char* values[MAX_PREFS];
	int pref_count;
	int prefs_loaded;
} g_w;

/* ---------------------------------------------------------------------------
 *  Preferences
 * ------------------------------------------------------------------------- */

static char* config_file(const char* name) {
	return ds_join(ds_config_dir(), name);
}

static void prefs_load(void) {
	char* path;
	char* text;
	size_t size = 0;
	if (g_w.prefs_loaded) return;
	g_w.prefs_loaded = R_TRUE;
	path = config_file("designer.ini");
	text = Rfile_read(path, &size);
	free(path);
	if (!text) return;
	{
		char* line = text;
		while (line && *line && g_w.pref_count < MAX_PREFS) {
			char* end = strchr(line, '\n');
			char* equal;
			if (end) *end = 0;
			if (end && end > line && end[-1] == '\r') end[-1] = 0;
			equal = strchr(line, '=');
			if (equal && equal > line) {
				*equal = 0;
				g_w.keys[g_w.pref_count] = ds_strdup(line);
				g_w.values[g_w.pref_count] = ds_strdup(equal + 1);
				g_w.pref_count++;
			}
			line = end ? end + 1 : NULL;
		}
	}
	Rmemory_free(text);
}

const char* prefs_get(const char* key) {
	int i;
	prefs_load();
	for (i = 0; i < g_w.pref_count; i++) if (!strcmp(g_w.keys[i], key)) return g_w.values[i];
	return "";
}

void prefs_set(const char* key, const char* value) {
	ds_buffer_t out = { 0 };
	char* path;
	int i;
	prefs_load();
	for (i = 0; i < g_w.pref_count; i++) if (!strcmp(g_w.keys[i], key)) break;
	if (i < g_w.pref_count) {
		if (!strcmp(g_w.values[i], value ? value : "")) return;
		free(g_w.values[i]);
		g_w.values[i] = ds_strdup(value);
	} else if (g_w.pref_count < MAX_PREFS) {
		g_w.keys[g_w.pref_count] = ds_strdup(key);
		g_w.values[g_w.pref_count] = ds_strdup(value);
		g_w.pref_count++;
	}
	for (i = 0; i < g_w.pref_count; i++) ds_buffer_printf(&out, "%s=%s\n", g_w.keys[i], g_w.values[i]);
	path = config_file("designer.ini");
	Rfile_write(path, out.data ? out.data : "", out.size);
	free(path);
	ds_buffer_free(&out);
}

/* ---------------------------------------------------------------------------
 *  Recent folders
 * ------------------------------------------------------------------------- */

/* How many interfaces (.xml) a folder holds at its top. */
static int count_interfaces(const char* folder) {
	ds_entry_t* entries;
	int count = ds_list_dir(folder, &entries), i, found = 0;
	for (i = 0; i < count; i++) if (!entries[i].is_dir && ds_iequal(ds_extension(entries[i].name), "xml")) found++;
	free(entries);
	return found;
}

static void recent_free(recent_t* r) {
	free(r->path);
	free(r->name);
	memset(r, 0, sizeof(*r));
}

static void recent_fill(recent_t* r, const char* path, double opened) {
	r->path = ds_normalize(path);
	{
		size_t n = strlen(r->path);
		while (n > 1 && r->path[n - 1] == '/') r->path[--n] = 0;
	}
	r->name = ds_strdup(ds_basename(r->path));
	r->opened = opened;
	r->missing = !ds_is_dir(r->path);
	r->interfaces = r->missing ? 0 : count_interfaces(r->path);
}

static void recent_save(void) {
	ds_buffer_t out = { 0 };
	char* path = config_file("recent-folders.txt");
	int i;
	for (i = 0; i < g_w.count; i++) ds_buffer_printf(&out, "%.0f\t%s\n", g_w.recent[i].opened, g_w.recent[i].path);
	Rfile_write(path, out.data ? out.data : "", out.size);
	free(path);
	ds_buffer_free(&out);
}

static void recent_load(void) {
	char* path = config_file("recent-folders.txt");
	size_t size = 0;
	char* text = Rfile_read(path, &size);
	free(path);
	if (!text) return;
	{
		char* line = text;
		while (line && *line && g_w.count < MAX_RECENT) {
			char* end = strchr(line, '\n');
			char* tab;
			if (end) *end = 0;
			if (end && end > line && end[-1] == '\r') end[-1] = 0;
			tab = strchr(line, '\t');
			if (tab && tab[1]) {
				int i, duplicate = 0;
				for (i = 0; i < g_w.count; i++) if (ds_same_path(g_w.recent[i].path, tab + 1)) duplicate = 1;
				if (!duplicate) recent_fill(&g_w.recent[g_w.count++], tab + 1, atof(line));
			}
			line = end ? end + 1 : NULL;
		}
	}
	Rmemory_free(text);
}

static int recent_find(const char* folder) {
	int i;
	char* normalized = ds_normalize(folder);
	size_t n = strlen(normalized);
	while (n > 1 && normalized[n - 1] == '/') normalized[--n] = 0;
	for (i = 0; i < g_w.count; i++) {
		if (ds_same_path(g_w.recent[i].path, normalized)) break;
	}
	free(normalized);
	return i < g_w.count ? i : -1;
}

static void recent_remove_at(int index) {
	if (index < 0 || index >= g_w.count) return;
	recent_free(&g_w.recent[index]);
	memmove(&g_w.recent[index], &g_w.recent[index + 1], sizeof(recent_t) * (size_t)(g_w.count - index - 1));
	g_w.count--;
	memset(&g_w.recent[g_w.count], 0, sizeof(recent_t));
	recent_save();
}

static void refresh_list(void);

void recent_add(const char* folder) {
	recent_t entry;
	int at = recent_find(folder);
	if (at >= 0) recent_remove_at(at);
	if (g_w.count == MAX_RECENT) recent_free(&g_w.recent[--g_w.count]);
	memset(&entry, 0, sizeof(entry));
	recent_fill(&entry, folder, (double)time(NULL));
	memmove(&g_w.recent[1], &g_w.recent[0], sizeof(recent_t) * (size_t)g_w.count);
	g_w.recent[0] = entry;
	g_w.count++;
	recent_save();
	refresh_list();
}

/* ---------------------------------------------------------------------------
 *  The list
 * ------------------------------------------------------------------------- */

/* A color of its own for each folder, from its name. */
static void folder_color(const char* name, float* r, float* gr, float* b) {
	static const float colors[][3] = {
		{ 0.33f, 0.47f, 0.80f }, { 0.80f, 0.42f, 0.24f }, { 0.22f, 0.60f, 0.45f }, { 0.55f, 0.36f, 0.78f },
		{ 0.16f, 0.62f, 0.75f }, { 0.85f, 0.35f, 0.50f }, { 0.80f, 0.60f, 0.15f }, { 0.40f, 0.45f, 0.55f }
	};
	unsigned hash = 5381;
	for (; *name; name++) hash = hash * 33u + (unsigned char)*name;
	*r = colors[hash % 8][0];
	*gr = colors[hash % 8][1];
	*b = colors[hash % 8][2];
}

static void time_ago(double then, char* out, size_t size) {
	double seconds = (double)time(NULL) - then;
	if (then <= 0.0) { snprintf(out, size, "%s", ""); return; }
	if (seconds < 60.0) snprintf(out, size, "just now");
	else if (seconds < 3600.0) snprintf(out, size, "%d min ago", (int)(seconds / 60.0));
	else if (seconds < 86400.0) snprintf(out, size, "%d h ago", (int)(seconds / 3600.0));
	else if (seconds < 86400.0 * 2) snprintf(out, size, "yesterday");
	else if (seconds < 86400.0 * 31) snprintf(out, size, "%d days ago", (int)(seconds / 86400.0));
	else {
		time_t t = (time_t)then;
		struct tm* tm = localtime(&t);
		if (tm) strftime(out, size, "%d %b %Y", tm);
		else out[0] = 0;
	}
}

/* The text cut on the left ("...ments/My App/ui") to fit a width. */
static void fit_left(const char* text, float size, float width, char* out, size_t out_size) {
	float w = 0.0f, h = 0.0f;
	size_t length = strlen(text), cut;
	Rfont_measure_text(R_INVALID_ID, size, text, &w, &h);
	if (w <= width) {
		snprintf(out, out_size, "%s", text);
		return;
	}
	for (cut = 1; cut < length; cut++) {
		/* (not inside a character of several bytes) */
		if (((unsigned char)text[cut] & 0xC0) == 0x80) continue;
		snprintf(out, out_size, "...%s", text + cut);
		Rfont_measure_text(R_INVALID_ID, size, out, &w, &h);
		if (w <= width) return;
	}
	snprintf(out, out_size, "...");
}

static int matches(const recent_t* r, const char* filter) {
	size_t n = strlen(filter), i, j;
	const char* texts[2];
	int t;
	if (!n) return 1;
	texts[0] = r->name;
	texts[1] = r->path;
	for (t = 0; t < 2; t++) {
		const char* text = texts[t];
		for (i = 0; text[i]; i++) {
			for (j = 0; j < n && text[i + j]; j++) if (tolower((unsigned char)text[i + j]) != tolower((unsigned char)filter[j])) break;
			if (j == n) return 1;
		}
	}
	return 0;
}

static void refresh_list(void) {
	const char* filter;
	int i;
	if (g_w.list == R_INVALID_ID || !Rwidget_is_valid(g_w.list)) return;
	filter = Rtext_input_get_text(g_w.search);
	g_w.shown_count = 0;
	for (i = 0; i < g_w.count; i++) if (matches(&g_w.recent[i], filter)) g_w.shown[g_w.shown_count++] = i;
	if (g_w.selected >= g_w.shown_count) g_w.selected = g_w.shown_count - 1;
	g_w.hovered = -1;
	Rwidget_set_size(g_w.list, -1.0f, g_w.shown_count ? g_w.shown_count * (ROW_HEIGHT + ROW_GAP) : 1.0f);
	Rlabel_set_text(g_w.empty, g_w.count == 0 ? "No recent folder yet. Open the folder of your interfaces: it will be listed here."
		: g_w.shown_count == 0 ? "No recent folder matches the search." : "");
	Rwidget_set_visibility(g_w.empty, g_w.shown_count == 0 ? R_VISIBLE : R_COLLAPSED);
	Rwidget_invalidate(g_w.list);
}

static void row_rect(int row, float width, float* x, float* y, float* w, float* h) {
	*x = 0.0f;
	*y = row * (ROW_HEIGHT + ROW_GAP);
	*w = width;
	*h = ROW_HEIGHT;
}

/* The little cross that takes a folder out of the list, at the right of a row. */
static void remove_rect(float width, float row_y, float* x, float* y, float* size) {
	*size = 26.0f;
	*x = width - 14.0f - *size;
	*y = row_y + (ROW_HEIGHT - *size) * 0.5f;
}

static void list_paint(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	float tr, tg, tb, ta, hr, hg, hb, ha, sr, sg, sb, sa, dr, dg, db, da;
	Rid bold = Rfont_get_variant(Rfont_get_default(), R_FONT_BOLD);
	int row;
	(void)widget;
	(void)height;
	(void)user_data;
	Rtheme_get_color(R_COLOR_TEXT, &tr, &tg, &tb, &ta);
	Rtheme_get_color(R_COLOR_ROW_HOVERED, &hr, &hg, &hb, &ha);
	Rtheme_get_color(R_COLOR_SELECTION, &sr, &sg, &sb, &sa);
	Rtheme_get_color(R_COLOR_DANGER, &dr, &dg, &db, &da);
	for (row = 0; row < g_w.shown_count; row++) {
		const recent_t* r = &g_w.recent[g_w.shown[row]];
		float x, y, w, h, br, bg, bb, tw = 0.0f, th = 0.0f, text_x, right;
		char text[1200], when[64];
		char badge[8];
		row_rect(row, width, &x, &y, &w, &h);
		if (row == g_w.hovered) Rdraw_rect(painter, x, y, w, h, 12.0f, hr, hg, hb, ha);
		else if (row == g_w.selected) Rdraw_rect(painter, x, y, w, h, 12.0f, sr, sg, sb, sa * 0.6f);
		/* the badge: how many interfaces the folder holds */
		snprintf(badge, sizeof(badge), r->interfaces > 99 ? "99+" : "%d", r->interfaces);
		folder_color(r->name, &br, &bg, &bb);
		Rdraw_rect(painter, x + 12.0f, y + 11.0f, 40.0f, 40.0f, 10.0f, br, bg, bb, r->missing ? 0.35f : 1.0f);
		Rfont_measure_text(bold, 14.0f, badge, &tw, &th);
		Rdraw_text(painter, bold, 14.0f, x + 32.0f - tw * 0.5f, y + 31.0f - th * 0.5f, badge, 1.0f, 1.0f, 1.0f, 1.0f);
		/* the time, or the cross under the pointer */
		right = w - 16.0f;
		if (row == g_w.hovered) {
			float cx, cy, size;
			remove_rect(width, y, &cx, &cy, &size);
			if (g_w.hovered_remove) Rdraw_rect(painter, cx, cy, size, size, size * 0.5f, tr, tg, tb, 0.12f);
			Rdraw_line(painter, cx + 9.0f, cy + 9.0f, cx + size - 9.0f, cy + size - 9.0f, 1.6f, tr, tg, tb, 0.75f);
			Rdraw_line(painter, cx + size - 9.0f, cy + 9.0f, cx + 9.0f, cy + size - 9.0f, 1.6f, tr, tg, tb, 0.75f);
			right = cx - 10.0f;
		} else {
			time_ago(r->opened, when, sizeof(when));
			Rfont_measure_text(R_INVALID_ID, 12.0f, when, &tw, &th);
			Rdraw_text(painter, R_INVALID_ID, 12.0f, right - tw, y + 23.0f, when, tr, tg, tb, 0.5f);
			right -= tw + 16.0f;
		}
		/* the name, and where it is */
		text_x = x + 66.0f;
		fit_left(r->name, 15.0f, right - text_x, text, sizeof(text));
		Rdraw_text(painter, bold, 15.0f, text_x, y + 12.0f, text, tr, tg, tb, r->missing ? 0.5f : 1.0f);
		if (r->missing) {
			fit_left("The folder is not there any more: the cross takes it out of the list", 12.0f, right - text_x, text, sizeof(text));
			Rdraw_text(painter, R_INVALID_ID, 12.0f, text_x, y + 35.0f, text, dr, dg, db, 0.9f);
		} else {
			char pretty[1100], line[1200];
			ds_pretty_path(r->path, pretty, sizeof(pretty));
			snprintf(line, sizeof(line), "%d interface%s  ·  %s", r->interfaces, r->interfaces == 1 ? "" : "s", pretty);
			fit_left(line, 12.0f, right - text_x, text, sizeof(text));
			Rdraw_text(painter, R_INVALID_ID, 12.0f, text_x, y + 35.0f, text, tr, tg, tb, 0.55f);
		}
	}
}

static int row_at(float y) {
	int row = (int)(y / (ROW_HEIGHT + ROW_GAP));
	if (y < 0.0f || row >= g_w.shown_count) return -1;
	if (y - row * (ROW_HEIGHT + ROW_GAP) > ROW_HEIGHT) return -1;
	return row;
}

static void open_row(int row) {
	char* path;
	if (row < 0 || row >= g_w.shown_count) return;
	path = ds_strdup(g_w.recent[g_w.shown[row]].path);
	if (!ds_is_dir(path)) {
		g_w.recent[g_w.shown[row]].missing = R_TRUE;
		Rwidget_invalidate(g_w.list);
		ds_status("%s is not there any more.", path);
	} else {
		folder_open(path);
	}
	free(path);
}

static int list_event(Rid widget, const REvent* event, void* user_data) {
	float x, y, w, h;
	int row;
	(void)user_data;
	Rwidget_get_rect(widget, &x, &y, &w, &h);
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE: {
			int on_remove = 0;
			row = row_at(event->local_y);
			if (row >= 0) {
				float cx, cy, size;
				remove_rect(w, row * (ROW_HEIGHT + ROW_GAP), &cx, &cy, &size);
				on_remove = event->local_x >= cx && event->local_x <= cx + size && event->local_y >= cy && event->local_y <= cy + size;
			}
			if (row != g_w.hovered || on_remove != g_w.hovered_remove) {
				g_w.hovered = row;
				g_w.hovered_remove = on_remove;
				Rwidget_invalidate(widget);
			}
			Rwidget_set_cursor(widget, row >= 0 ? R_CURSOR_HAND : R_CURSOR_ARROW);
			return R_TRUE;
		}
		case R_EVENT_MOUSE_LEAVE:
			g_w.hovered = -1;
			g_w.hovered_remove = 0;
			Rwidget_invalidate(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			Rwidget_focus(widget);
			if (event->button != R_MOUSE_LEFT) return R_TRUE;
			row = row_at(event->local_y);
			if (row < 0) return R_TRUE;
			g_w.selected = row;
			if (g_w.hovered_remove && row == g_w.hovered) {
				recent_remove_at(g_w.shown[row]);
				refresh_list();
			} else {
				open_row(row);
			}
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			if (event->key == R_KEY_DOWN && g_w.shown_count) g_w.selected = g_w.selected + 1 < g_w.shown_count ? g_w.selected + 1 : g_w.shown_count - 1;
			else if (event->key == R_KEY_UP && g_w.shown_count) g_w.selected = g_w.selected > 0 ? g_w.selected - 1 : 0;
			else if (event->key == R_KEY_ENTER) open_row(g_w.selected);
			else if (event->key == R_KEY_DELETE && g_w.selected >= 0 && g_w.selected < g_w.shown_count) {
				recent_remove_at(g_w.shown[g_w.selected]);
				refresh_list();
			} else return R_FALSE;
			Rwidget_invalidate(widget);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static void on_search(Rid input, const char* text, void* user_data) {
	(void)input;
	(void)text;
	(void)user_data;
	g_w.selected = 0;
	refresh_list();
}

/* Enter in the search box: the first folder that matches. */
static void on_search_committed(Rid input, const char* text, void* user_data) {
	(void)input;
	(void)text;
	(void)user_data;
	if (g_w.shown_count) open_row(g_w.selected >= 0 ? g_w.selected : 0);
}

/* ---------------------------------------------------------------------------
 *  The page
 * ------------------------------------------------------------------------- */

static const char WELCOME_XML[] =
"<ScrollBox Slot.Fill=\"1\">"
"  <HBox Spacing=\"56\" Slot.Padding=\"64,48\">"
"    <VBox Spacing=\"10\" Width=\"290\">"
"      <Label Text=\"Raven Designer\" TextSize=\"34\"/>"
"      <Label Name=\"welcome.version\" Opacity=\"0.55\"/>"
"      <Label Text=\"Interfaces made by looking at them. The program that loads them is written in any language, elsewhere.\" Wrap=\"true\" Opacity=\"0.7\"/>"
"      <Spacer Height=\"18\"/>"
"      <Button Text=\"Open a folder...\" Style=\"Primary\" OnClicked=\"folder.open\" Tooltip=\"The folder of your interfaces, themes and translations (Ctrl+Shift+O)\"/>"
"      <Button Text=\"Open an interface...\" OnClicked=\"file.open\" Tooltip=\"A .xml file: its folder opens with it (Ctrl+O)\"/>"
"      <Button Name=\"welcome.back\" Style=\"Flat\" OnClicked=\"welcome.back\" Visibility=\"Collapsed\"/>"
"      <Spacer Height=\"18\"/>"
"      <Label Text=\"In the designer\" Opacity=\"0.55\"/>"
"      <Label Wrap=\"true\" Opacity=\"0.8\" Markdown=\"true\">**Ctrl+K** searches everything: a widget to add, a file to open, a command. **F7** runs the interface in a window of its own.</Label>"
"    </VBox>"
"    <VBox Spacing=\"14\" Slot.Fill=\"1\">"
"      <HBox Spacing=\"12\">"
"        <Label Text=\"Recent folders\" TextSize=\"20\" Slot.Fill=\"1\" Slot.VAlign=\"Center\"/>"
"        <TextInput Name=\"welcome.search\" Placeholder=\"Search\" Width=\"240\"/>"
"      </HBox>"
"      <VBox Name=\"welcome.rows\"/>"
"      <Label Name=\"welcome.empty\" Opacity=\"0.55\" Wrap=\"true\" Slot.Padding=\"4,8\"/>"
"    </VBox>"
"  </HBox>"
"</ScrollBox>";

static void action_welcome(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)user_data;
	if (!strcmp(action, "welcome.back")) {
		welcome_hide();
		ds_status("Back to %s.", g_folder ? ds_basename(g_folder) : "the interface");
	}
}

void welcome_init(void) {
	Rid content, rows;
	g_w.page = Rwidget_find(g_ds_window, "page.welcome");
	g_w.list = R_INVALID_ID;
	g_w.hovered = -1;
	g_w.selected = 0;
	recent_load();
	content = Rwidget_create_from_xml(WELCOME_XML, sizeof(WELCOME_XML) - 1);
	if (content == R_INVALID_ID) return;
	Rslot_set_fill(content, 1.0f);
	Rwidget_add_child(g_w.page, content);
	g_w.search = Rwidget_find(content, "welcome.search");
	g_w.empty = Rwidget_find(content, "welcome.empty");
	g_w.back = Rwidget_find(content, "welcome.back");
	{
		char version[96];
		snprintf(version, sizeof(version), "Version %s", Rget_version());
		Rlabel_set_text(Rwidget_find(content, "welcome.version"), version);
	}
	rows = Rwidget_find(content, "welcome.rows");
	g_w.list = Rwidget_create(R_WIDGET_CUSTOM);
	Rcustom_set_paint_callback(g_w.list, list_paint, NULL);
	Rwidget_set_event_callback(g_w.list, list_event, NULL);
	Rwidget_set_focusable(g_w.list, R_TRUE);
	Rwidget_add_child(rows, g_w.list);
	Rtext_input_set_changed_callback(g_w.search, on_search, NULL);
	Rtext_input_set_committed_callback(g_w.search, on_search_committed, NULL);
	Raction_bind("welcome.back", action_welcome, NULL);
	refresh_list();
}

void welcome_shutdown(void) {
	int i;
	for (i = 0; i < g_w.count; i++) recent_free(&g_w.recent[i]);
	g_w.count = 0;
	for (i = 0; i < g_w.pref_count; i++) {
		free(g_w.keys[i]);
		free(g_w.values[i]);
	}
	g_w.pref_count = 0;
}

void welcome_show(void) {
	Rid pages = Rwidget_find(g_ds_window, "pages");
	int i;
	/* folders may have come or gone since */
	for (i = 0; i < g_w.count; i++) {
		g_w.recent[i].missing = !ds_is_dir(g_w.recent[i].path);
		if (!g_w.recent[i].missing) g_w.recent[i].interfaces = count_interfaces(g_w.recent[i].path);
	}
	Rswitcher_set_index(pages, 0);
	if (g_folder) {
		char text[400];
		snprintf(text, sizeof(text), "Back to %s", ds_basename(g_folder));
		Rbutton_set_text(g_w.back, text);
		Rwidget_set_visibility(g_w.back, R_VISIBLE);
	} else if (designer_has_document()) {
		Rbutton_set_text(g_w.back, "Back to the interface");
		Rwidget_set_visibility(g_w.back, R_VISIBLE);
	} else {
		Rwidget_set_visibility(g_w.back, R_COLLAPSED);
	}
	refresh_list();
	Rwidget_focus(g_w.list);
}

void welcome_hide(void) {
	Rid pages = Rwidget_find(g_ds_window, "pages");
	if (pages == R_INVALID_ID || Rswitcher_get_index(pages) != 0) return;
	/* the page of the tab in front */
	Rswitcher_set_index(pages, designer_tab_page());
}

int welcome_is_shown(void) {
	Rid pages = g_ds_window != R_INVALID_ID ? Rwidget_find(g_ds_window, "pages") : R_INVALID_ID;
	return pages != R_INVALID_ID && Rswitcher_get_index(pages) == 0;
}
