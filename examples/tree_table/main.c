/**
 * Raven example - tree table
 * A file explorer made of a TreeTable (tree_table.xml): a generated disk of
 * folders and files with their size, kind and date, icons, the share of its
 * folder each one takes (a progress column), sorting that keeps every file
 * in its folder, a selection of several rows, and a network share whose
 * content is only made when it is opened (the expanded callback).
 *
 * The status line shows the rows selected and their size. For the
 * automated runs (RAVEN_EXAMPLE_FRAMES, RAVEN_EXAMPLE_CAPTURE in
 * common/example.h, or RAVEN_HEADLESS / RAVEN_SNAP): RAVEN_TREE_THEME=light
 * shows the light look.
 */

#include "../common/example.h"

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

enum { NAME, SIZE, KIND, MODIFIED, SHARE };

static struct {
	Rid window;
	Rid files, status;
	Rid folder, file;
	int share, widgets;
	unsigned seed;
} app;

/* A small generator: the same disk at every run. */
static unsigned next_random(void) {
	app.seed = app.seed * 1103515245u + 12345u;
	return (app.seed >> 16) & 0x7FFF;
}

static const char* kind_of(const char* name) {
	static const struct { const char* extension; const char* kind; } kinds[] = {
		{ ".c", "C source" }, { ".h", "C header" }, { ".md", "Markdown" }, { ".png", "PNG image" },
		{ ".jpg", "JPEG image" }, { ".mp3", "MP3 audio" }, { ".pdf", "PDF document" }, { ".txt", "Text" },
		{ ".zip", "ZIP archive" }, { ".xml", "Raven interface" }, { ".mov", "Movie" }
	};
	const char* dot = strrchr(name, '.');
	size_t i;
	for (i = 0; dot && i < sizeof(kinds) / sizeof(kinds[0]); i++)
		if (strcmp(dot, kinds[i].extension) == 0) return kinds[i].kind;
	return "Document";
}

/* A row: "name|size|kind|date|". */
static int add(int parent, const char* name, long size, int folder) {
	char cells[256];
	unsigned day = 1 + next_random() % 24, hour = 8 + next_random() % 11, minute = next_random() % 60;
	int node;
	snprintf(cells, sizeof(cells), "%s|%ld|%s|2026-11-%02u %02u:%02u|", name, size, folder ? "Folder" : kind_of(name), day, hour, minute);
	node = Rtree_table_add_node(app.files, parent, cells);
	Rtree_table_set_node_icon(app.files, node, folder ? app.folder : app.file);
	return node;
}

/* Sizes of folders: the sum of what they hold; each child shows its share of the folder. */
static long total(int node) {
	int i, count = Rtree_table_get_child_count(app.files, node);
	long sum = 0;
	char text[32];
	if (node && count == 0 && strcmp(Rtree_table_get_node_cell(app.files, node, KIND), "Folder") != 0)
		return atol(Rtree_table_get_node_cell(app.files, node, SIZE));
	for (i = 0; i < count; i++) sum += total(Rtree_table_get_child(app.files, node, i));
	for (i = 0; i < count && sum > 0; i++) {
		int child = Rtree_table_get_child(app.files, node, i);
		snprintf(text, sizeof(text), "%.3f", (double)atol(Rtree_table_get_node_cell(app.files, child, SIZE)) / (double)sum);
		Rtree_table_set_node_cell(app.files, child, SHARE, text);
	}
	if (node) {
		snprintf(text, sizeof(text), "%ld", sum);
		Rtree_table_set_node_cell(app.files, node, SIZE, text);
	}
	return sum;
}

static void files(int parent, const char* const* names, int count, long smallest, long largest) {
	int i;
	for (i = 0; i < count; i++) add(parent, names[i], smallest + (long)(next_random() % (unsigned)(largest - smallest + 1)), 0);
}

static void make_disk(void) {
	static const char* const sources[] = { "table.c", "tree_table.c", "tree_view.c", "list_view.c", "label.c", "button.c", "text_input.c" };
	static const char* const headers[] = { "raven_controls.h", "raven_widget.h", "raven_core.h", "raven_style.h" };
	static const char* const docs[] = { "XML.md", "THEMES.md", "ROADMAP.md", "README.md" };
	static const char* const photos[] = { "harbour.jpg", "lighthouse.jpg", "market.jpg", "snow.jpg", "dunes.jpg", "bridge.jpg" };
	static const char* const shots[] = { "tree_table.png", "diff_view.png", "charts.png" };
	static const char* const songs[] = { "01 Overture.mp3", "02 Morning.mp3", "03 The long road.mp3", "04 Static.mp3", "05 Coda.mp3" };
	static const char* const papers[] = { "invoice-2026-10.pdf", "lease.pdf", "tax-return.pdf", "notes.txt", "plan.md" };
	static const char* const downloads[] = { "installer.zip", "fonts.zip", "talk.mov", "slides.pdf" };
	int projects, raven, src, widgets, pictures, trip, music, album, documents;
	app.seed = 2026;

	projects = add(0, "Projects", 0, 1);
	raven = add(projects, "raven", 0, 1);
	src = add(raven, "src", 0, 1);
	widgets = app.widgets = add(src, "widgets", 0, 1);
	files(widgets, sources, 7, 18, 120);
	files(src, sources + 4, 2, 6, 40);
	files(add(raven, "include", 0, 1), headers, 4, 20, 90);
	files(add(raven, "docs", 0, 1), docs, 4, 8, 60);
	add(raven, "CMakeLists.txt", 12, 0);
	files(add(projects, "website", 0, 1), docs + 3, 1, 4, 9);

	pictures = add(0, "Pictures", 0, 1);
	trip = add(pictures, "Trip to Lisbon", 0, 1);
	files(trip, photos, 6, 2400, 6800);
	files(add(pictures, "Screenshots", 0, 1), shots, 3, 180, 420);

	music = add(0, "Music", 0, 1);
	album = add(music, "Night drive", 0, 1);
	files(album, songs, 5, 3000, 9000);

	documents = add(0, "Documents", 0, 1);
	files(documents, papers, 5, 3, 900);
	files(add(0, "Downloads", 0, 1), downloads, 4, 1500, 96000);
	total(0);

	/* not read yet: a chevron without children, filled when opened */
	app.share = add(0, "Network share", 0, 1);
	Rtree_table_set_node_cell(app.files, app.share, KIND, "Server (not read yet)");
	Rtree_table_set_node_has_children(app.files, app.share, R_TRUE);

	Rtree_table_set_expanded(app.files, projects, R_TRUE);
	Rtree_table_set_expanded(app.files, raven, R_TRUE);
	Rtree_table_set_expanded(app.files, src, R_TRUE);
	Rtree_table_set_expanded(app.files, widgets, R_TRUE);
	Rtree_table_set_expanded(app.files, pictures, R_TRUE);
	Rtree_table_set_expanded(app.files, trip, R_TRUE);
}

/* The network share is read the first time it is opened. */
static void on_expanded(Rid table, int node, int expanded, void* user_data) {
	static const char* const names[] = { "backups", "team.md", "budget.pdf", "logo.png" };
	int i;
	(void)user_data;
	if (!expanded || node != app.share || Rtree_table_get_child_count(table, node) > 0) return;
	for (i = 0; i < 4; i++) add(node, names[i], 40 + 300 * (long)i, i == 0);
	Rtree_table_set_node_has_children(table, Rtree_table_get_child(table, node, 0), R_TRUE);
	Rtree_table_set_node_cell(table, node, KIND, "Server");
	total(node);
}

static void show_status(void) {
	char text[160];
	int i, count = Rtable_get_selection_count(app.files);
	long size = 0;
	for (i = 0; i < count; i++) size += atol(Rtable_get_cell(app.files, Rtable_get_selection(app.files, i), SIZE));
	if (count > 0) snprintf(text, sizeof(text), "%d selected, %ld KB  -  %d nodes, %d rows shown", count, size,
		Rtree_table_get_node_count(app.files), Rtable_get_row_count(app.files));
	else snprintf(text, sizeof(text), "%d nodes, %d rows shown", Rtree_table_get_node_count(app.files), Rtable_get_row_count(app.files));
	Rlabel_set_text(app.status, text);
}

static void on_selection(Rid table, int row, void* user_data) {
	(void)table;
	(void)row;
	(void)user_data;
	show_status();
}

static void on_action(Rid sender, const char* action, void* user_data) {
	(void)user_data;
	if (strcmp(action, "files.expand") == 0) Rtree_table_expand_all(app.files, 0);
	else if (strcmp(action, "files.collapse") == 0) Rtree_table_collapse_all(app.files, 0);
	else if (strcmp(action, "files.guides") == 0) Rtree_table_set_guide_lines(app.files, Rcheckbox_is_checked(sender));
	Rwidget_focus(app.files);
	show_status();
}

int main(void) {
	const char* theme = getenv("RAVEN_TREE_THEME");
	int row;
	if (!example_init("Raven tree table")) return 1;
	Rlook_set(theme && strcmp(theme, "light") == 0 ? R_LOOK_RAVEN_LIGHT : R_LOOK_RAVEN_DARK);
	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/tree_table.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.files = Rwidget_find(app.window, "files");
	app.status = Rwidget_find(app.window, "status");
	app.folder = Ricon_get("folder");
	app.file = Ricon_get("file");
	Rtree_table_set_expanded_callback(app.files, on_expanded, NULL);
	Rtable_set_selection_changed_callback(app.files, on_selection, NULL);
	Raction_bind("files.expand", on_action, NULL);
	Raction_bind("files.collapse", on_action, NULL);
	Raction_bind("files.guides", on_action, NULL);
	make_disk();

	/* the largest first, in every folder; two files of the widgets selected */
	Rtable_sort(app.files, SIZE, R_TRUE);
	row = Rtree_table_get_node_row(app.files, Rtree_table_get_child(app.files, app.widgets, 0));
	if (row >= 0) Rtable_select_range(app.files, row, row + 1, R_TRUE);
	Rwidget_focus(app.files);
	show_status();

	example_run(app.window, NULL, NULL);
	return 0;
}
