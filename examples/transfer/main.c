/**
 * Raven example - transfer
 * Swatches given to the other applications (transfer.xml):
 *
 *   drag out      a swatch dragged out of the window (Rtile_view_set_drag_out_callback,
 *                 Rdrag_begin_external) goes as a picture (image/png) into a
 *                 document, a mail or an image editor, and as a PNG file
 *                 (text/uri-list) into the Finder or the Explorer; inside the
 *                 window it is still a drag of the application (DragType):
 *                 the well on the right takes it
 *   copy, paste   Rclipboard_set_image, Rclipboard_get_image: any picture
 *                 copied elsewhere (a screenshot, an image in a browser)
 *                 pastes in the well
 *   formats       the name of a swatch as HTML and text (Rclipboard_set_data)
 *                 and a format of the application, read back by Raven only
 *
 * RAVEN_TRANSFER_DEMO=1 copies the third swatch and pastes it in the well at
 * the start (for the automated runs that capture the window: RAVEN_EXAMPLE_FRAMES,
 * RAVEN_EXAMPLE_CAPTURE in common/example.h). It replaces what the clipboard held.
 */

#include "../common/example.h"

#include <math.h>

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

#define SWATCH_COUNT 8
#define SWATCH_SIZE 96
/* the format only this application knows: the number of a swatch */
#define SWATCH_MIME "application/x-raven-swatch"

static const char* const g_names[SWATCH_COUNT] = { "Sunset", "Lagoon", "Forest", "Stripes", "Rings", "Checker", "Aurora", "Ember" };

static struct {
	Rid window, swatches, well, preview, caption, clipboard, status;
	uint8_t pixels[SWATCH_COUNT][SWATCH_SIZE * SWATCH_SIZE * 4];
	Rid textures[SWATCH_COUNT];
	/* the picture the well shows (pasted or dropped) */
	Rid shown;
} app;

/* ---------------------------------------------------------------------------
 *  The swatches: pictures made here
 * ------------------------------------------------------------------------- */

static uint8_t mix(float a, float b, float t) {
	float v = a + (b - a) * t;
	return (uint8_t)(v < 0.0f ? 0.0f : v > 255.0f ? 255.0f : v);
}

static void make_swatch(int index, uint8_t* pixels) {
	static const float colors[SWATCH_COUNT][2][3] = {
		{ { 255, 94, 58 }, { 255, 210, 90 } }, { { 0, 120, 200 }, { 80, 230, 210 } },
		{ { 20, 90, 50 }, { 150, 210, 90 } }, { { 90, 60, 200 }, { 250, 250, 255 } },
		{ { 230, 60, 140 }, { 255, 200, 230 } }, { { 30, 30, 40 }, { 230, 230, 235 } },
		{ { 40, 220, 160 }, { 120, 60, 220 } }, { { 140, 20, 10 }, { 255, 150, 40 } }
	};
	int x, y;
	for (y = 0; y < SWATCH_SIZE; y++) {
		for (x = 0; x < SWATCH_SIZE; x++) {
			float u = (float)x / (SWATCH_SIZE - 1), v = (float)y / (SWATCH_SIZE - 1), t;
			float dx = u - 0.5f, dy = v - 0.5f, r = sqrtf(dx * dx + dy * dy);
			uint8_t* p = pixels + (y * SWATCH_SIZE + x) * 4;
			switch (index) {
				case 3: t = ((x + y) / 12) % 2 ? 1.0f : 0.15f; break;
				case 4: t = 0.5f + 0.5f * sinf(r * 40.0f); break;
				case 5: t = ((x / 16) + (y / 16)) % 2 ? 1.0f : 0.0f; break;
				case 6: t = 0.5f + 0.5f * sinf(u * 6.0f + sinf(v * 5.0f) * 2.0f); break;
				default: t = (u + v) * 0.5f; break;
			}
			p[0] = mix(colors[index][0][0], colors[index][1][0], t);
			p[1] = mix(colors[index][0][1], colors[index][1][1], t);
			p[2] = mix(colors[index][0][2], colors[index][1][2], t);
			/* rounded corners: the picture has transparency */
			{
				float cx = fabsf(dx) * SWATCH_SIZE - (SWATCH_SIZE * 0.5f - 14.0f), cy = fabsf(dy) * SWATCH_SIZE - (SWATCH_SIZE * 0.5f - 14.0f);
				float out = (cx > 0.0f && cy > 0.0f) ? sqrtf(cx * cx + cy * cy) - 14.0f : 0.0f;
				p[3] = out <= 0.0f ? 255 : out >= 1.0f ? 0 : (uint8_t)(255.0f * (1.0f - out));
			}
		}
	}
}

static void tile_item(Rid tiles, int index, RTileItem* item, void* user) {
	(void)tiles;
	(void)user;
	item->text = g_names[index];
	item->detail = "96 x 96 PNG";
	item->icon = app.textures[index];
	item->icon_radius = 14.0f;
}

/* The swatch the buttons act on: the current one, else the first selected. */
static int current_swatch(void) {
	int index = Rtile_view_get_current(app.swatches);
	if (index < 0 && Rtile_view_get_selection_count(app.swatches) > 0) index = Rtile_view_get_selection(app.swatches, 0);
	return index >= 0 && index < SWATCH_COUNT ? index : 0;
}

/* ---------------------------------------------------------------------------
 *  What the clipboard holds, and what happened
 * ------------------------------------------------------------------------- */

static void say(const char* text) {
	Rlabel_set_text(app.status, text);
}

static void show_clipboard(Rid timer, void* user) {
	char text[256];
	(void)timer;
	(void)user;
	snprintf(text, sizeof(text), "On the clipboard:%s%s%s%s%s",
		Rclipboard_has_image() ? " a picture" : "",
		Rclipboard_has_data("text/html") ? " HTML" : "",
		Rclipboard_has_data("text/plain") ? " text" : "",
		Rclipboard_has_data("text/uri-list") ? " files" : "",
		Rclipboard_has_data(SWATCH_MIME) ? " a swatch of this example" : "");
	if (!strcmp(text, "On the clipboard:")) snprintf(text, sizeof(text), "The clipboard is empty");
	Rlabel_set_text(app.clipboard, text);
}

/* The well shows a picture (its own copy). */
static void show_picture(const uint8_t* rgba, int width, int height, const char* caption) {
	if (app.shown != R_INVALID_ID) Rtexture_destroy(app.shown);
	app.shown = Rtexture_create_from_pixels(rgba, width, height);
	Rimage_set_texture(app.preview, app.shown);
	Rlabel_set_text(app.caption, caption);
}

/* ---------------------------------------------------------------------------
 *  Copy and paste
 * ------------------------------------------------------------------------- */

static void on_copy(Rid sender, const char* action, void* user) {
	char text[96];
	int index = current_swatch();
	(void)sender;
	(void)action;
	(void)user;
	if (Rclipboard_set_image(app.pixels[index], SWATCH_SIZE, SWATCH_SIZE, 0)) snprintf(text, sizeof(text), "%s copied as a picture: paste it in an image editor, a document, a mail.", g_names[index]);
	else snprintf(text, sizeof(text), "This system takes no pictures on its clipboard.");
	say(text);
	show_clipboard(R_INVALID_ID, NULL);
}

static void on_paste(Rid sender, const char* action, void* user) {
	char text[96];
	int width = 0, height = 0;
	uint8_t* rgba = Rclipboard_get_image(&width, &height);
	(void)sender;
	(void)action;
	(void)user;
	if (!rgba) {
		say("No picture on the clipboard: copy one in another application (or here) first.");
		return;
	}
	snprintf(text, sizeof(text), "Pasted: %d x %d", width, height);
	show_picture(rgba, width, height, text);
	Rpixels_free(rgba);
	say("A picture from the clipboard is in the well.");
}

/* One content in three formats: the program that pastes takes the one it prefers. */
static void on_copy_html(Rid sender, const char* action, void* user) {
	char html[160], number[8];
	int index = current_swatch();
	RDataItem items[3];
	(void)sender;
	(void)action;
	(void)user;
	snprintf(html, sizeof(html), "<b style=\"color:#%02X%02X%02X\">%s</b>", app.pixels[index][0], app.pixels[index][1], app.pixels[index][2], g_names[index]);
	snprintf(number, sizeof(number), "%d", index);
	items[0].mime = "text/html";
	items[0].data = html;
	items[0].size = strlen(html);
	items[1].mime = "text/plain";
	items[1].data = g_names[index];
	items[1].size = strlen(g_names[index]);
	items[2].mime = SWATCH_MIME;
	items[2].data = number;
	items[2].size = strlen(number);
	Rclipboard_set_data(items, 3);
	say("The name copied as HTML (a word processor shows it in color), as text, and as a swatch of this example.");
	show_clipboard(R_INVALID_ID, NULL);
}

/* ---------------------------------------------------------------------------
 *  Dragging out of the window
 * ------------------------------------------------------------------------- */

/* A PNG file of the swatch in the temporary folder, for the file managers: its file:// URL is added to `list`. */
static void add_file(int index, char* list, size_t capacity) {
	static const char hex[] = "0123456789ABCDEF";
	const char* folder = getenv("TMPDIR");
	char path[512], url[1600];
	size_t size = 0, used = 0, length;
	uint8_t* png;
	FILE* file;
	const unsigned char* p;
	if (!folder || !folder[0]) folder = getenv("TEMP");
	if (!folder || !folder[0]) folder = "/tmp";
	length = strlen(folder);
	snprintf(path, sizeof(path), "%s%s%s.png", folder, length && (folder[length - 1] == '/' || folder[length - 1] == '\\') ? "" : "/", g_names[index]);
	png = Rimage_encode_png(app.pixels[index], SWATCH_SIZE, SWATCH_SIZE, &size);
	file = png ? fopen(path, "wb") : NULL;
	if (file) {
		fwrite(png, 1, size, file);
		fclose(file);
	}
	Rpixels_free(png);
	if (!file) return;
	/* file:///path, the characters other than letters, digits and - . _ ~ / written %XX ('\' as '/' on Windows) */
	used = (size_t)snprintf(url, sizeof(url), "file://%s", path[0] == '/' ? "" : "/");
	for (p = (const unsigned char*)path; *p && used + 4 < sizeof(url); p++) {
		unsigned char c = *p == '\\' ? '/' : *p;
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || strchr("-._~/:", c)) {
			url[used++] = (char)c;
		} else {
			url[used++] = '%';
			url[used++] = hex[c >> 4];
			url[used++] = hex[c & 15];
		}
	}
	url[used] = '\0';
	if (strlen(list) + used + 3 < capacity) {
		strcat(list, url);
		strcat(list, "\r\n");
	}
}

static void on_drag_finished(Rid source, RDragOperation operation, void* user) {
	(void)source;
	(void)user;
	switch (operation) {
		case R_DRAG_OPERATION_COPY: say("Dropped in another application: it made a copy."); break;
		case R_DRAG_OPERATION_MOVE: say("Dropped in another application: it took the file."); break;
		case R_DRAG_OPERATION_LINK: say("Dropped in another application: it made a link."); break;
		default: break;
	}
}

/* The selected swatches start being dragged: their data for the other applications. */
static void on_drag_out(Rid tiles, int index, void* user) {
	char list[4096] = "", names[256] = "", number[8];
	int count = Rtile_view_get_selection_count(tiles), i;
	size_t png_size = 0;
	uint8_t* png;
	RDataItem items[4];
	(void)user;
	for (i = 0; i < count; i++) {
		int selected = Rtile_view_get_selection(tiles, i);
		if (selected < 0 || selected >= SWATCH_COUNT) continue;
		add_file(selected, list, sizeof(list));
		if (names[0]) strncat(names, ", ", sizeof(names) - strlen(names) - 1);
		strncat(names, g_names[selected], sizeof(names) - strlen(names) - 1);
	}
	if (index < 0 || index >= SWATCH_COUNT) index = 0;
	snprintf(number, sizeof(number), "%d", index);
	png = Rimage_encode_png(app.pixels[index], SWATCH_SIZE, SWATCH_SIZE, &png_size);
	items[0].mime = "image/png";
	items[0].data = png;
	items[0].size = png_size;
	items[1].mime = "text/uri-list";
	items[1].data = list;
	items[1].size = strlen(list);
	items[2].mime = "text/plain";
	items[2].data = names;
	items[2].size = strlen(names);
	items[3].mime = SWATCH_MIME;
	items[3].data = number;
	items[3].size = strlen(number);
	/* (no file could be written: the picture and the names only) */
	if (!list[0]) items[1] = items[3];
	if (Rdrag_begin_external(tiles, items, list[0] ? 4 : 3, R_DRAG_OPERATION_COPY, on_drag_finished, NULL)) {
		Rdrag_set_image(app.pixels[index], SWATCH_SIZE, SWATCH_SIZE);
	}
	Rpixels_free(png);
}

/* The well takes a swatch dragged inside the application. */
static int on_well_drop(Rid target, const RDropEvent* event, void* user) {
	int index;
	char caption[64];
	(void)target;
	(void)user;
	if (!event->drag_type || strcmp(event->drag_type, "swatch") != 0) return R_FALSE;
	if (event->phase != R_DROP_DROPPED) return R_TRUE;
	/* (the data of the tile view: the indices of the tiles, the first one shown) */
	index = event->text ? atoi(event->text) : 0;
	if (index < 0 || index >= SWATCH_COUNT) return R_FALSE;
	snprintf(caption, sizeof(caption), "%s, dropped here", g_names[index]);
	show_picture(app.pixels[index], SWATCH_SIZE, SWATCH_SIZE, caption);
	say("A drag inside the window: it only leaves it with the pointer.");
	return R_TRUE;
}

int main(void) {
	int i;
	if (!example_init("Raven transfer")) return 1;
	app.shown = R_INVALID_ID;
	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/transfer.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.swatches = Rwidget_find(app.window, "swatches");
	app.well = Rwidget_find(app.window, "well");
	app.preview = Rwidget_find(app.window, "preview");
	app.caption = Rwidget_find(app.window, "caption");
	app.clipboard = Rwidget_find(app.window, "clipboard");
	app.status = Rwidget_find(app.window, "status");
	for (i = 0; i < SWATCH_COUNT; i++) {
		make_swatch(i, app.pixels[i]);
		app.textures[i] = Rtexture_create_from_pixels(app.pixels[i], SWATCH_SIZE, SWATCH_SIZE);
	}
	Rtile_view_set_items(app.swatches, SWATCH_COUNT, tile_item, NULL);
	Rtile_view_set_drag_out_callback(app.swatches, on_drag_out, NULL);
	Rwidget_set_drop_callback(app.well, on_well_drop, NULL);
	Raction_bind("transfer.copy", on_copy, NULL);
	Raction_bind("transfer.paste", on_paste, NULL);
	Raction_bind("transfer.copy_html", on_copy_html, NULL);
	say(Rdrag_external_supported() ? "Drag a swatch out of the window." : "This system cannot drag out of a window yet: copy and paste work.");
	show_clipboard(R_INVALID_ID, NULL);
	/* (other applications change the clipboard too) */
	Rtimer_create(1.0, R_TRUE, show_clipboard, NULL);

	if (getenv("RAVEN_TRANSFER_DEMO")) {
		Rtile_view_set_selected(app.swatches, 2, R_TRUE);
		on_copy(R_INVALID_ID, "transfer.copy", NULL);
		on_paste(R_INVALID_ID, "transfer.paste", NULL);
	}
	example_run(app.window, NULL, NULL);
	return 0;
}
