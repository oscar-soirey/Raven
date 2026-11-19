/*
 * The icons shipped with Raven (Ricon_get) and SVGs in currentColor: every
 * icon is drawn, in white (a mask); the widgets that show one tint it with
 * the color of their text, which follows the theme and the states, without
 * drawing the texture again. Headless: a backend of the test records the
 * textures it is given and the instances of each frame.
 */
#include "test.h"

#define MAX_TEXTURES 1024

typedef struct texture_record_t {
	Rid id;
	int width, height;
	/* pixels that are painted (alpha > 0), and those of them that are not white */
	int painted, colored;
	/* the color of the most opaque pixel */
	uint8_t rgba[4];
} texture_record_t;

static texture_record_t g_textures[MAX_TEXTURES];
static int g_texture_count;
static int g_creates;

/* what the last frame drew with a texture (the first instance that samples it) and the glyphs */
static Rid g_watched = R_INVALID_ID;
static int g_watched_drawn;
static uint32_t g_watched_color;
static uint32_t g_glyph_color;
static int g_glyphs;

static int record_texture(Rid texture, const RTextureDesc* desc, const void* pixels) {
	texture_record_t* record = NULL;
	int i;
	g_creates++;
	for (i = 0; i < g_texture_count; i++)
		if (g_textures[i].id == texture) record = &g_textures[i];
	if (!record && g_texture_count < MAX_TEXTURES) record = &g_textures[g_texture_count++];
	if (!record) return R_TRUE;
	memset(record, 0, sizeof(*record));
	record->id = texture;
	record->width = (int)desc->width;
	record->height = (int)desc->height;
	if (pixels && desc->format == R_TEXTURE_RGBA8) {
		const uint8_t* p = (const uint8_t*)pixels;
		size_t n, count = (size_t)desc->width * desc->height;
		int best = -1;
		for (n = 0; n < count; n++, p += 4) {
			if (p[3] == 0) continue;
			record->painted++;
			if (p[0] != 255 || p[1] != 255 || p[2] != 255) record->colored++;
			if (p[3] > best) {
				best = p[3];
				memcpy(record->rgba, p, 4);
			}
		}
	}
	return R_TRUE;
}

static const texture_record_t* find_texture(Rid texture) {
	int i;
	for (i = 0; i < g_texture_count; i++)
		if (g_textures[i].id == texture) return &g_textures[i];
	return NULL;
}

static void observe_frame(const RDrawData* data, RCapture* capture) {
	Ruint c, i;
	(void)capture;
	g_watched_drawn = 0;
	g_glyphs = 0;
	for (c = 0; c < data->command_count; c++) {
		const RDrawCommand* command = &data->commands[c];
		if (command->type != R_DRAW_INSTANCES) continue;
		for (i = command->first_instance; i < command->first_instance + command->instance_count; i++) {
			const RDrawInstance* instance = &data->instances[i];
			if ((instance->flags & R_PRIM_MASK) == R_PRIM_GLYPH) {
				g_glyph_color = instance->color;
				g_glyphs++;
			} else if (command->texture == g_watched && (instance->flags & R_INSTANCE_TEXTURED) && !g_watched_drawn) {
				g_watched_color = instance->color;
				g_watched_drawn = 1;
			}
		}
	}
}

static uint32_t pack(float r, float g, float b, float a) {
	return (uint32_t)(r * 255.0f + 0.5f) | ((uint32_t)(g * 255.0f + 0.5f) << 8) | ((uint32_t)(b * 255.0f + 0.5f) << 16) |
		((uint32_t)(a * 255.0f + 0.5f) << 24);
}

static uint32_t theme_color(RColorRole role) {
	float r, g, b, a;
	Rtheme_get_color(role, &r, &g, &b, &a);
	return pack(r, g, b, a);
}

/* Every icon parses, is drawn with something in it, in white, and says it is in currentColor. */
static void test_every_icon(void) {
	int count = Ricon_get_count(), i;
	CHECK(count >= 40);
	for (i = 0; i < count; i++) {
		const char* name = Ricon_get_name(i);
		const texture_record_t* record;
		Rid icon;
		int w = 0, h = 0;
		CHECK(name != NULL);
		if (!name) continue;
		/* alphabetical: Ricon_get finds them by halves */
		if (i > 0) CHECK(strcmp(Ricon_get_name(i - 1), name) < 0);
		icon = Ricon_get(name);
		CHECK(Rtexture_is_valid(icon));
		CHECK(Rtexture_uses_current_color(icon));
		/* 24 x 24 logical units, drawn at a density of 2 before it is shown */
		Rtexture_get_size(icon, &w, &h);
		CHECK(w == 48 && h == 48);
		CHECK(Rtexture_get_density(icon) == 2.0f);
		record = find_texture(icon);
		CHECK(record != NULL);
		if (!record) continue;
		/* a stroke of 2 on 24 x 24: at least a few dozen pixels, not the whole square */
		if (record->painted < 40 || record->painted > 48 * 48 * 3 / 4) printf("  icon %s: %d pixels painted\n", name, record->painted);
		CHECK(record->painted >= 40);
		CHECK(record->painted <= 48 * 48 * 3 / 4);
		CHECK(record->colored == 0);
		/* shared: the same texture the second time, and with the "raven:" of XML */
		CHECK(Ricon_get(name) == icon);
		{
			char reference[64];
			snprintf(reference, sizeof(reference), "raven:%s", name);
			CHECK(Ricon_get(reference) == icon);
		}
	}
	CHECK(Ricon_get_name(-1) == NULL);
	CHECK(Ricon_get_name(count) == NULL);
}

static void test_unknown_icons(void) {
	EXPECT_ERRORS_BEGIN();
	CHECK(Ricon_get("no-such-icon") == R_INVALID_ID);
	CHECK(Ricon_get("") == R_INVALID_ID);
	CHECK(Ricon_get("raven:") == R_INVALID_ID);
	CHECK(Ricon_get("Search") == R_INVALID_ID); /* names are in lower case */
	CHECK(Ricon_get(NULL) == R_INVALID_ID);
	EXPECT_ERRORS_END(5);
	/* destroyed by the application: made again */
	{
		Rid icon = Ricon_get("star");
		Rtexture_destroy(icon);
		CHECK(!Rtexture_is_valid(icon));
		icon = Ricon_get("star");
		CHECK(Rtexture_is_valid(icon) && Rtexture_uses_current_color(icon));
	}
}

/* currentColor in SVGs of the application: alone it is a white mask; mixed with other colors it is black. */
static void test_current_color_svg(void) {
	static const char* alone[] = {
		"<svg xmlns='http://www.w3.org/2000/svg' width='10' height='10'><rect width='10' height='10' fill='currentColor'/></svg>",
		"<svg width='10' height='10'><rect width='10' height='10' style='fill: CurrentColor'/></svg>",
		"<svg width='10' height='10' fill='currentColor'><g><rect width='10' height='10' fill-opacity='0.5'/></g></svg>",
		"<svg width='10' height='10'><path d='M0 5 L10 5' fill='none' stroke='currentcolor' stroke-width='4'/></svg>",
	};
	static const char mixed[] =
		"<svg width='10' height='10'><rect width='5' height='10' fill='currentColor'/><rect x='5' width='5' height='10' fill='#ff0000'/></svg>";
	static const char plain[] = "<svg width='10' height='10'><rect width='10' height='10'/></svg>";
	size_t i;
	for (i = 0; i < sizeof(alone) / sizeof(alone[0]); i++) {
		Rid texture = Rtexture_create_from_svg(alone[i], strlen(alone[i]), 1.0f);
		const texture_record_t* record = find_texture(texture);
		CHECK(Rtexture_uses_current_color(texture));
		CHECK(record && record->painted > 0 && record->colored == 0);
		Rtexture_destroy(texture);
	}
	/* the opacity is kept */
	{
		Rid texture = Rtexture_create_from_svg(alone[2], strlen(alone[2]), 1.0f);
		const texture_record_t* record = find_texture(texture);
		CHECK(record && record->rgba[3] >= 126 && record->rgba[3] <= 129);
		Rtexture_destroy(texture);
	}
	{
		Rid texture = Rtexture_create_from_svg(mixed, sizeof(mixed) - 1, 1.0f);
		const texture_record_t* record = find_texture(texture);
		int w = 0, h = 0;
		uint8_t* pixels;
		CHECK(!Rtexture_uses_current_color(texture));
		CHECK(record && record->colored == record->painted);
		Rtexture_destroy(texture);
		/* currentColor is black there, as in an SVG shown on its own */
		pixels = Rimage_decode(mixed, sizeof(mixed) - 1, &w, &h);
		CHECK(pixels != NULL);
		if (pixels) {
			const uint8_t* left = pixels + ((size_t)(h / 2) * (size_t)w + 1) * 4;
			const uint8_t* right = pixels + ((size_t)(h / 2) * (size_t)w + (size_t)w - 2) * 4;
			CHECK(left[0] == 0 && left[1] == 0 && left[2] == 0 && left[3] == 255);
			CHECK(right[0] == 255 && right[1] == 0 && right[2] == 0 && right[3] == 255);
			Rpixels_free(pixels);
		}
	}
	{
		Rid texture = Rtexture_create_from_svg(plain, sizeof(plain) - 1, 1.0f);
		CHECK(Rtexture_is_valid(texture) && !Rtexture_uses_current_color(texture));
		Rtexture_destroy(texture);
	}
	/* decoded on a worker */
	{
		Rid texture = Rtexture_create_async(alone[0], strlen(alone[0]));
		Rtexture_wait(texture);
		CHECK(Rtexture_uses_current_color(texture));
		Rtexture_destroy(texture);
	}
	CHECK(!Rtexture_uses_current_color(R_INVALID_ID));
}

/* Draws the window (even when nothing changed: what the frame shows is read back). */
static void frame(Rid window) {
	int i;
	g_watched_drawn = 0;
	/* (the SVG drawn again at the density it is shown with comes back as a task) */
	for (i = 0; i < 3; i++) {
		Rwindow_request_redraw(window);
		Rdraw_frame();
	}
}

/*
 * An icon in a button takes the color of the text of the button, whatever
 * its state and the theme, and changing it never draws the texture again.
 */
static void test_icon_follows_text(void) {
	Rid window = Rwindow_create("Icons", 300, 200, 0);
	Rid root = Rwidget_create(R_WIDGET_VBOX);
	Rid button = Rwidget_add_child(root, Rbutton_create(NULL));
	Rid row = Rwidget_add_child(button, Rwidget_create(R_WIDGET_HBOX));
	Rid image = Rwidget_add_child(row, Rwidget_create(R_WIDGET_IMAGE));
	Rid label = Rwidget_add_child(row, Rwidget_create(R_WIDGET_LABEL));
	Rid icon = Ricon_get("save");
	float x, y, w, h;
	int creates;
	uint32_t normal;

	Rimage_set_texture(image, icon);
	Rlabel_set_text(label, "Save");
	Rwindow_set_content(window, root);
	g_watched = icon;
	frame(window);
	CHECK(g_watched_drawn);
	CHECK(g_glyphs > 0);
	/* the icon is drawn in the color of the text beside it */
	CHECK(g_watched_color == g_glyph_color);
	normal = g_watched_color;
	creates = g_creates;

	/* hovered, then pressed */
	Rwidget_get_rect(button, &x, &y, &w, &h);
	Rwindow_inject_mouse_move(window, x + w * 0.5f, y + h * 0.5f);
	frame(window);
	CHECK(g_watched_drawn && g_watched_color == g_glyph_color);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	frame(window);
	CHECK(g_watched_drawn && g_watched_color == g_glyph_color);
	Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
	Rwindow_inject_mouse_move(window, 290, 190);
	frame(window);

	/* disabled: the color of disabled text, not dimmed twice */
	Rwidget_set_enabled(button, R_FALSE);
	frame(window);
	CHECK(g_watched_drawn && g_watched_color == g_glyph_color);
	CHECK(g_watched_color == theme_color(R_COLOR_TEXT_DISABLED));
	Rwidget_set_enabled(button, R_TRUE);

	/* the other theme: the icon follows */
	Rtheme_apply(R_THEME_LIGHT);
	frame(window);
	CHECK(g_watched_drawn && g_watched_color == g_glyph_color);
	CHECK(g_watched_color != normal);
	Rtheme_apply(R_THEME_DARK);
	frame(window);
	CHECK(g_watched_drawn && g_watched_color == normal);

	/* a text color given to a panel reaches the icon */
	Rwidget_set_color(row, R_COLOR_TEXT, 1.0f, 0.0f, 0.0f, 1.0f);
	frame(window);
	CHECK(g_watched_drawn && g_watched_color == 0xFF0000FFu);
	Rwidget_set_color(row, R_COLOR_TEXT, 0.0f, 0.0f, 1.0f, 1.0f);
	frame(window);
	CHECK(g_watched_drawn && g_watched_color == 0xFFFF0000u);

	/* a tint given to the image wins */
	Rimage_set_tint_color(image, 0.0f, 1.0f, 0.0f, 1.0f);
	frame(window);
	CHECK(g_watched_drawn && g_watched_color == 0xFF00FF00u);

	/* none of this drew the icon again */
	CHECK(g_creates == creates);
	g_watched = R_INVALID_ID;
	Rwindow_destroy(window);
}

/* The Icon attributes of XML: "raven:" names, in the color of the text of the row. */
static void test_xml_icons(void) {
	static const char xml[] =
		"<Window Title='Icons' Width='400' Height='300'>"
		"  <VBox>"
		"    <Image Name='img' Source='raven:search'/>"
		"    <TabView Name='tabs' Documents='true' Slot.Fill='1'>"
		"      <TreeView Name='tree' Slot.Title='Files' Slot.Icon='raven:folder' Slot.Closable='false'>"
		"        <Item Text='Documents' Icon='raven:file'/>"
		"      </TreeView>"
		"    </TabView>"
		"    <MenuBar><Menu Title='File'><MenuItem Name='open' Text='Open' Icon='raven:open'/></Menu></MenuBar>"
		"  </VBox>"
		"</Window>";
	static const char bad[] = "<Window><Image Source='raven:no-such-icon'/></Window>";
	Rid window = Rwindow_create_from_xml(xml, sizeof(xml) - 1);
	CHECK(Rwindow_is_valid(window));
	g_watched = Ricon_get("file");
	frame(window);
	/* the tree row: its icon in the color of its text (the only glyphs after the tab title are its own) */
	CHECK(g_watched_drawn);
	CHECK(g_watched_color == theme_color(R_COLOR_TEXT));
	g_watched = Ricon_get("folder");
	frame(window);
	CHECK(g_watched_drawn);
	g_watched = Ricon_get("search");
	frame(window);
	CHECK(g_watched_drawn && g_watched_color == theme_color(R_COLOR_TEXT));
	g_watched = R_INVALID_ID;
	Rwindow_destroy(window);

	EXPECT_ERRORS_BEGIN();
	window = Rwindow_create_from_xml(bad, sizeof(bad) - 1);
	EXPECT_ERRORS_END(1);
	if (Rwindow_is_valid(window)) Rwindow_destroy(window);
}

/* A backend that draws nothing (public API only: the test also runs with the shared library). */
static int backend_init(const RBackendInitInfo* info) { (void)info; return R_TRUE; }
static void backend_shutdown(void) {}
static int backend_create_surface(Rid window, const RNativeWindow* native, Ruint width, Ruint height) {
	(void)window; (void)native; (void)width; (void)height;
	return R_TRUE;
}
static void backend_destroy_surface(Rid window) { (void)window; }
static void backend_update_texture(Rid texture, Ruint x, Ruint y, Ruint width, Ruint height, const void* pixels) {
	(void)texture; (void)x; (void)y; (void)width; (void)height; (void)pixels;
}
static void backend_set_texture_filter(Rid texture, RFilterType filter) { (void)texture; (void)filter; }
static void backend_destroy_texture(Rid texture) { (void)texture; }
static void backend_wait_idle(void) {}

int main(void) {
	RBackendVTable backend;
	RInitDesc desc;
	memset(&backend, 0, sizeof(backend));
	backend.name = "test";
	backend.Init = backend_init;
	backend.Shutdown = backend_shutdown;
	backend.CreateSurface = backend_create_surface;
	backend.DestroySurface = backend_destroy_surface;
	backend.UpdateTexture = backend_update_texture;
	backend.SetTextureFilter = backend_set_texture_filter;
	backend.DestroyTexture = backend_destroy_texture;
	backend.WaitIdle = backend_wait_idle;
	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_CUSTOM;
	desc.headless = R_TRUE;
	desc.worker_threads = -1;
	desc.disable_render_thread = R_TRUE;
	backend.CreateTexture = record_texture;
	backend.RenderFrame = observe_frame;
	Rbackend_register(&backend);
	Rerror_set_callback(test_error_callback);
	if (!Rinit_ex(&desc)) return 1;
	Rset_animations_enabled(R_FALSE);
	Rtheme_apply(R_THEME_DARK);
#if !defined(RAVEN_TEST_NO_SVG)
	RUN(test_every_icon);
	RUN(test_unknown_icons);
	RUN(test_current_color_svg);
	RUN(test_icon_follows_text);
	RUN(test_xml_icons);
#endif
	Rshutdown();
	return test_report("test_icons");
}
