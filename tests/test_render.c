/**
 * Raven tests - rendering with the Vulkan backend.
 *
 * Frames are rendered for real and read back with Rwindow_capture, then a
 * few well-chosen pixels are compared with the expected colors. The test
 * first tries to run without a window system (VK_EXT_headless_surface), then
 * with a real window. Without a usable Vulkan driver it is skipped (exit
 * code 77), so that the test suite still passes on machines without a GPU.
 */

#include "test.h"

#include <stdlib.h>

#if !defined(_WIN32)
#include <unistd.h>
#endif

#define SKIP_CODE 77

static uint8_t* g_pixels;
static int g_width, g_height;
static float g_scale = 1.0f;

/* Renders the window and keeps its pixels for the CHECK_PIXEL that follow. */
static void capture(Rid window) {
	Rpixels_free(g_pixels);
	Rdraw_frame();
	g_pixels = Rwindow_capture(window, &g_width, &g_height);
	g_scale = Rwindow_get_scale(window);
	CHECK(g_pixels != NULL);
}

static const uint8_t* pixel_at(float x, float y) {
	static const uint8_t none[4] = { 0, 0, 0, 0 };
	int px = (int)(x * g_scale), py = (int)(y * g_scale);
	if (!g_pixels || px < 0 || py < 0 || px >= g_width || py >= g_height) return none;
	return g_pixels + ((size_t)py * (size_t)g_width + (size_t)px) * 4;
}

/* Position in logical units, expected color, with a tolerance for rounding in the blending. */
#define CHECK_PIXEL(x, y, er, eg, eb) do { \
	const uint8_t* test_p_ = pixel_at((float)(x), (float)(y)); \
	g_test_checks++; \
	if ((abs((int)test_p_[0] - (er)) > 3 || abs((int)test_p_[1] - (eg)) > 3 || abs((int)test_p_[2] - (eb)) > 3)) { \
		g_test_failures++; \
		printf("  FAIL %s:%d: pixel (%g, %g) = (%d, %d, %d), expected (%d, %d, %d)\n", __FILE__, __LINE__, \
			(double)(x), (double)(y), test_p_[0], test_p_[1], test_p_[2], (er), (eg), (eb)); \
	} \
} while (0)

static Rid window_from(const char* xml) {
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	CHECK(Rwindow_is_valid(window));
	return window;
}

/* ---------------------------------------------------------------------------
 *  Tests
 * ------------------------------------------------------------------------- */

static void test_shapes(void) {
	Rid window = window_from(
		"<Window Width=\"400\" Height=\"300\" ClearColor=\"#336699\">"
		"  <Canvas>"
		/* plain rectangle */
		"    <Border Slot.Position=\"10,10\" Slot.Size=\"100,60\" Background=\"#FF0000\" CornerRadius=\"0\" BorderWidth=\"0\"/>"
		/* disc: a square with a radius of half its side */
		"    <Border Slot.Position=\"150,10\" Slot.Size=\"100,100\" Background=\"#00FF00\" CornerRadius=\"50\" BorderWidth=\"0\"/>"
		/* outline */
		"    <Border Slot.Position=\"10,150\" Slot.Size=\"100,100\" Background=\"#000000\" CornerRadius=\"0\" BorderWidth=\"10\" BorderColor=\"#FFFFFF\"/>"
		/* half-transparent white over black */
		"    <Border Slot.Position=\"150,150\" Slot.Size=\"100,100\" Background=\"#000000\" CornerRadius=\"0\" BorderWidth=\"0\">"
		"      <Border Background=\"#FFFFFF\" Opacity=\"0.5\" CornerRadius=\"0\" BorderWidth=\"0\"/>"
		"    </Border>"
		"  </Canvas>"
		"</Window>");
	capture(window);
	CHECK(g_width == (int)(400 * g_scale) && g_height == (int)(300 * g_scale));

	/* clear color */
	CHECK_PIXEL(390, 290, 0x33, 0x66, 0x99);
	CHECK_PIXEL(5, 5, 0x33, 0x66, 0x99);
	/* rectangle: inside, just inside each edge, just outside */
	CHECK_PIXEL(60, 40, 255, 0, 0);
	CHECK_PIXEL(10.5, 10.5, 255, 0, 0);
	CHECK_PIXEL(109.5, 69.5, 255, 0, 0);
	CHECK_PIXEL(9.5, 40, 0x33, 0x66, 0x99);
	CHECK_PIXEL(110.5, 40, 0x33, 0x66, 0x99);
	CHECK_PIXEL(60, 70.5, 0x33, 0x66, 0x99);
	/* disc: center filled, corners of its square untouched */
	CHECK_PIXEL(200, 60, 0, 255, 0);
	CHECK_PIXEL(200, 12, 0, 255, 0);
	CHECK_PIXEL(153, 13, 0x33, 0x66, 0x99);
	CHECK_PIXEL(247, 107, 0x33, 0x66, 0x99);
	/* outline: white band of 10 units, black inside */
	CHECK_PIXEL(15, 200, 255, 255, 255);
	CHECK_PIXEL(60, 245, 255, 255, 255);
	CHECK_PIXEL(60, 200, 0, 0, 0);
	CHECK_PIXEL(22, 162, 0, 0, 0);
	/* opacity */
	CHECK_PIXEL(200, 200, 128, 128, 128);

	Rwindow_destroy(window);
}

static void test_clipping(void) {
	/* the red child is larger than its clipping parent: only the part inside is drawn */
	Rid window = window_from(
		"<Window Width=\"300\" Height=\"200\" ClearColor=\"#000000\">"
		"  <Canvas>"
		"    <Canvas Slot.Position=\"50,50\" Slot.Size=\"100,100\" Clip=\"true\">"
		"      <Border Slot.Position=\"-40,-40\" Slot.Size=\"300,300\" Background=\"#FF0000\" CornerRadius=\"0\" BorderWidth=\"0\"/>"
		"    </Canvas>"
		"  </Canvas>"
		"</Window>");
	capture(window);
	CHECK_PIXEL(100, 100, 255, 0, 0);
	CHECK_PIXEL(51, 51, 255, 0, 0);
	CHECK_PIXEL(149, 149, 255, 0, 0);
	CHECK_PIXEL(49, 100, 0, 0, 0);
	CHECK_PIXEL(151, 100, 0, 0, 0);
	CHECK_PIXEL(100, 49, 0, 0, 0);
	CHECK_PIXEL(100, 151, 0, 0, 0);
	CHECK_PIXEL(20, 20, 0, 0, 0);
	Rwindow_destroy(window);
}

static void test_textures(void) {
	/* 2 x 2 texels: red, green / blue, half-transparent white */
	static const uint8_t texels[16] = {
		255, 0, 0, 255,   0, 255, 0, 255,
		0, 0, 255, 255,   255, 255, 255, 128
	};
	static const uint8_t yellow[16] = {
		255, 255, 0, 255,  255, 255, 0, 255,  255, 255, 0, 255,  255, 255, 0, 255
	};
	Rid window = window_from(
		"<Window Width=\"300\" Height=\"200\" ClearColor=\"#000000\">"
		"  <Canvas><Image Name=\"image\" Slot.Position=\"20,20\" Slot.Size=\"100,100\"/></Canvas>"
		"</Window>");
	Rid image = Rwidget_find(window, "image");
	Rid texture = Rtexture_create_from_pixels(texels, 2, 2);
	int width = 0, height = 0;

	CHECK(Rtexture_is_valid(texture) && Rtexture_is_ready(texture));
	Rtexture_get_size(texture, &width, &height);
	CHECK(width == 2 && height == 2);
	/* no smoothing: each texel is a flat quarter of the image */
	Rtexture_set_filter(texture, R_FILTER_NEAREST);
	Rimage_set_texture(image, texture);
	Rimage_set_stretch(image, R_STRETCH_FILL);
	capture(window);
	CHECK_PIXEL(45, 45, 255, 0, 0);
	CHECK_PIXEL(95, 45, 0, 255, 0);
	CHECK_PIXEL(45, 95, 0, 0, 255);
	/* the alpha of the texture blends with what is behind (black) */
	CHECK_PIXEL(95, 95, 128, 128, 128);
	CHECK_PIXEL(130, 60, 0, 0, 0);

	/* the tint multiplies the texture */
	Rimage_set_tint_color(image, 1.0f, 0.0f, 0.0f, 1.0f);
	capture(window);
	CHECK_PIXEL(45, 45, 255, 0, 0);
	CHECK_PIXEL(95, 45, 0, 0, 0);
	Rimage_set_tint_color(image, 1.0f, 1.0f, 1.0f, 1.0f);

	/* new pixels, same size */
	Rtexture_update(texture, yellow, 2, 2);
	capture(window);
	CHECK_PIXEL(45, 45, 255, 255, 0);
	CHECK_PIXEL(95, 95, 255, 255, 0);

	/* a deleted texture is simply not drawn */
	Rtexture_destroy(texture);
	CHECK(!Rtexture_is_valid(texture));
	capture(window);
	CHECK_PIXEL(45, 45, 0, 0, 0);

	Rwindow_destroy(window);
}

/* Pixels of a square of the capture far from mid-gray: what shimmers when an image is scaled down. */
static int count_far_from_gray(float x0, float y0, float size) {
	int far = 0;
	float x, y;
	for (y = y0 + 2.0f; y < y0 + size - 2.0f; y += 1.0f) {
		for (x = x0 + 2.0f; x < x0 + size - 2.0f; x += 1.0f) {
			const uint8_t* p = pixel_at(x, y);
			if (abs((int)p[0] - 128) > 40) far++;
		}
	}
	return far;
}

static void test_mipmaps(void) {
	/* stripes one texel wide, shown about seven times smaller */
	const int size = 256;
	uint8_t* stripes = (uint8_t*)malloc((size_t)size * (size_t)size * 4);
	Rid window = window_from(
		"<Window Width=\"200\" Height=\"100\" ClearColor=\"#000000\">"
		"  <Canvas><Image Name=\"image\" Slot.Position=\"20,20\" Slot.Size=\"37,37\"/></Canvas>"
		"</Window>");
	Rid image = Rwidget_find(window, "image");
	Rid texture;
	int i, far_linear, far_mipmap;
	for (i = 0; i < size * size; i++) {
		uint8_t v = (i % size) % 2 ? 255 : 0;
		stripes[i * 4 + 0] = stripes[i * 4 + 1] = stripes[i * 4 + 2] = v;
		stripes[i * 4 + 3] = 255;
	}
	texture = Rtexture_create_from_pixels(stripes, size, size);
	Rimage_set_texture(image, texture);
	Rimage_set_stretch(image, R_STRETCH_FILL);

	/* without mipmaps: one sample of the stripes per pixel, black or white by chance */
	capture(window);
	far_linear = count_far_from_gray(20.0f, 20.0f, 37.0f);
	CHECK(far_linear > 100);

	/* the switch adds the mip levels on the GPU: the stripes average to gray */
	Rtexture_set_filter(texture, R_FILTER_MIPMAP);
	capture(window);
	far_mipmap = count_far_from_gray(20.0f, 20.0f, 37.0f);
	CHECK(far_mipmap == 0);
	if (far_mipmap != 0) printf("  far from gray: %d with mipmaps, %d without\n", far_mipmap, far_linear);

	/* new pixels: the levels are made again (white stripes on white: all white) */
	for (i = 0; i < size * size * 4; i++) stripes[i] = 255;
	Rtexture_update(texture, stripes, size, size);
	capture(window);
	CHECK_PIXEL(38, 38, 255, 255, 255);
	Rtexture_destroy(texture);

	/* created with mipmaps from the start (as the textures from image files are) */
	for (i = 0; i < size * size; i++) {
		uint8_t v = (i % size) % 2 ? 255 : 0;
		stripes[i * 4 + 0] = stripes[i * 4 + 1] = stripes[i * 4 + 2] = v;
	}
	texture = Rtexture_create_from_pixels(stripes, size, size);
	Rtexture_set_filter(texture, R_FILTER_MIPMAP);
	Rtexture_update(texture, stripes, size, size);
	Rimage_set_texture(image, texture);
	capture(window);
	CHECK(count_far_from_gray(20.0f, 20.0f, 37.0f) == 0);
	/* back to plain smoothing: the first level only */
	Rtexture_set_filter(texture, R_FILTER_LINEAR);
	capture(window);
	CHECK(count_far_from_gray(20.0f, 20.0f, 37.0f) > 100);

	Rtexture_destroy(texture);
	free(stripes);
	Rwindow_destroy(window);
}

static void test_text(void) {
	Rid window = window_from(
		"<Window Width=\"300\" Height=\"120\" ClearColor=\"#000000\">"
		"  <Canvas><Label Name=\"label\" Slot.Position=\"20,20\" TextSize=\"40\" Color=\"#FFFFFF\">Raven</Label></Canvas>"
		"</Window>");
	float x, y, w, h;
	int lit_inside = 0, lit_outside = 0, px, py;

	capture(window);
	Rwidget_get_rect(Rwidget_find(window, "label"), &x, &y, &w, &h);
	CHECK(w > 40 && h > 30);
	for (py = 0; py < g_height; py++) {
		for (px = 0; px < g_width; px++) {
			const uint8_t* p = g_pixels + ((size_t)py * (size_t)g_width + (size_t)px) * 4;
			float lx = (float)px / g_scale, ly = (float)py / g_scale;
			int inside = lx >= x - 1 && ly >= y - 1 && lx < x + w + 1 && ly < y + h + 1;
			if (p[0] > 128) {
				if (inside) lit_inside++;
				else lit_outside++;
			}
		}
	}
	/* three capital letters of 40 units cover a few hundred pixels, all inside the label */
	CHECK(lit_inside > 300);
	CHECK(lit_outside == 0);
	Rwindow_destroy(window);
}

static void paint_custom(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	(void)widget;
	(void)width;
	(void)height;
	(void)user_data;
	Rdraw_triangle(painter, 10, 10, 90, 10, 50, 90, 1, 0, 0, 1);
	Rdraw_line(painter, 110, 50, 190, 50, 10, 0, 1, 0, 1);
	Rdraw_circle(painter, 250, 50, 40, 0, 0, 1, 1);
	Rdraw_rect_outline(painter, 10, 110, 80, 60, 0, 4, 1, 1, 1, 1);
	Rdraw_push_clip_rect(painter, 110, 110, 40, 60);
	Rdraw_rect(painter, 110, 110, 80, 60, 0, 1, 1, 0, 1);
	Rdraw_pop_clip_rect(painter);
}

static void test_custom_drawing(void) {
	Rid window = window_from(
		"<Window Width=\"300\" Height=\"200\" ClearColor=\"#000000\"><Custom Name=\"custom\"/></Window>");
	Rcustom_set_paint_callback(Rwidget_find(window, "custom"), paint_custom, NULL);
	capture(window);
	/* triangle */
	CHECK_PIXEL(50, 30, 255, 0, 0);
	CHECK_PIXEL(50, 80, 255, 0, 0);
	CHECK_PIXEL(15, 80, 0, 0, 0);
	CHECK_PIXEL(85, 80, 0, 0, 0);
	/* line of thickness 10 with round caps */
	CHECK_PIXEL(150, 50, 0, 255, 0);
	CHECK_PIXEL(150, 46, 0, 255, 0);
	CHECK_PIXEL(150, 58, 0, 0, 0);
	/* circle */
	CHECK_PIXEL(250, 50, 0, 0, 255);
	CHECK_PIXEL(250, 15, 0, 0, 255);
	CHECK_PIXEL(215, 15, 0, 0, 0);
	/* outline */
	CHECK_PIXEL(12, 140, 255, 255, 255);
	CHECK_PIXEL(50, 140, 0, 0, 0);
	/* clip rectangle: only the left half of the yellow rectangle */
	CHECK_PIXEL(130, 140, 255, 255, 0);
	CHECK_PIXEL(170, 140, 0, 0, 0);
	Rwindow_destroy(window);
}

static void paint_glass(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	(void)widget;
	(void)width;
	(void)height;
	(void)user_data;
	/* clear glass: no tint */
	Rdraw_glass(painter, 100, 50, 200, 100, 20, 0, 0, 0, 0);
}

static void test_glass(void) {
	/* a slab of glass lies across the border between a red and a blue half */
	Rid window = window_from(
		"<Window Width=\"400\" Height=\"200\" ClearColor=\"#000000\">"
		"  <Overlay>"
		"    <Canvas>"
		"      <Border Slot.Position=\"0,0\" Slot.Size=\"200,200\" Background=\"#FF0000\" CornerRadius=\"0\" BorderWidth=\"0\"/>"
		"      <Border Slot.Position=\"200,0\" Slot.Size=\"200,200\" Background=\"#0000FF\" CornerRadius=\"0\" BorderWidth=\"0\"/>"
		"    </Canvas>"
		"    <Custom Name=\"glass\"/>"
		"  </Overlay>"
		"</Window>");
	Rid custom = Rwidget_find(window, "glass");
	const uint8_t* p;

	Rcustom_set_paint_callback(custom, paint_glass, NULL);
	capture(window);

	/* what is not under the glass is untouched, and the border stays sharp there */
	CHECK_PIXEL(20, 100, 255, 0, 0);
	CHECK_PIXEL(380, 100, 0, 0, 255);
	CHECK_PIXEL(198, 20, 255, 0, 0);
	CHECK_PIXEL(202, 20, 0, 0, 255);
	CHECK_PIXEL(198, 180, 255, 0, 0);
	CHECK_PIXEL(202, 180, 0, 0, 255);
	CHECK_PIXEL(130, 47, 255, 0, 0);

	/* under the glass the border is frosted: both colors on both sides of it */
	p = pixel_at(197, 100);
	CHECK(p[0] > 50 && p[2] > 50 && p[1] < 40);
	p = pixel_at(203, 100);
	CHECK(p[0] > 50 && p[2] > 50 && p[1] < 40);
	/* red wins on the left, blue on the right */
	p = pixel_at(185, 100);
	CHECK(p[0] > p[2]);
	p = pixel_at(215, 100);
	CHECK(p[2] > p[0]);
	/* far from the border, the flat middle of the slab shows the color behind it */
	p = pixel_at(130, 100);
	CHECK(p[0] > 235 && p[1] < 25 && p[2] < 25);
	p = pixel_at(270, 100);
	CHECK(p[2] > 235 && p[0] < 25 && p[1] < 25);
	/* the rim catches the light: a bright line on the upper edge */
	p = pixel_at(130, 50.5f);
	CHECK(p[1] > 40);

	/* without the glass, the frame is drawn straight to the window again */
	Rwidget_set_visibility(custom, R_HIDDEN);
	capture(window);
	CHECK_PIXEL(198, 100, 255, 0, 0);
	CHECK_PIXEL(202, 100, 0, 0, 255);
	CHECK_PIXEL(130, 50.5, 255, 0, 0);

	/* ... and with it again */
	Rwidget_set_visibility(custom, R_VISIBLE);
	capture(window);
	p = pixel_at(200, 100);
	CHECK(p[0] > 50 && p[2] > 50);

	Rwindow_destroy(window);
}

static void test_glass_theme(void) {
	Rid window;
	const uint8_t* p;

	/* every control is glass, and the window gets a wallpaper for it to show */
	Rtheme_apply(R_THEME_GLASS_DARK);
	window = window_from(
		"<Window Width=\"420\" Height=\"320\">"
		"  <Overlay>"
		"    <VBox Spacing=\"10\" Slot.Padding=\"20\">"
		"      <HBox Spacing=\"8\"><Button Text=\"Glass\"/><Button Text=\"Tinted\" Style=\"Primary\"/>"
		"        <Checkbox Text=\"Switch\" Style=\"Switch\" Checked=\"true\"/></HBox>"
		"      <Slider Value=\"0.5\"/>"
		"      <TextInput Placeholder=\"Text\"/>"
		"      <ComboBox Selected=\"0\"><Item Text=\"One\"/><Item Text=\"Two\"/></ComboBox>"
		"      <TabView Slot.Fill=\"1\"><Label Slot.Title=\"First\">Page</Label><Label Slot.Title=\"Second\">Other</Label></TabView>"
		"    </VBox>"
		"    <Border Name=\"panel\" Material=\"Glass\" Width=\"160\" Height=\"90\" Slot.HAlign=\"Right\" Slot.VAlign=\"Bottom\" Slot.Padding=\"12\"/>"
		"  </Overlay>"
		"</Window>");
	capture(window);
	CHECK(g_width == (int)(420 * g_scale) && g_height == (int)(320 * g_scale));

	/* the background of the dark glass is a plain grey (#2A2A2E): no colors behind the glass */
	p = pixel_at(4, 4);
	CHECK(abs((int)p[0] - 0x2A) < 8 && abs((int)p[1] - 0x2A) < 8 && abs((int)p[2] - 0x2E) < 8);
	p = pixel_at(410, 6);
	CHECK(abs((int)p[0] - 0x2A) < 8 && abs((int)p[1] - 0x2A) < 8 && abs((int)p[2] - 0x2E) < 8);

	/* a second frame of the same window: the offscreen images are reused */
	Rwidget_invalidate(Rwidget_find(window, "panel"));
	capture(window);
	CHECK(g_pixels != NULL);

	/* the same window with a flat theme, then with the light glass */
	Rtheme_apply(R_THEME_DARK);
	capture(window);
	CHECK(g_pixels != NULL);
	Rtheme_apply(R_THEME_GLASS_LIGHT);
	capture(window);
	CHECK(g_pixels != NULL);

	Rwindow_destroy(window);
	Rtheme_apply(R_THEME_DARK);
}

/* How much a row of pixels changes from one pixel to the next: sharp edges give a high value. */
static int row_contrast(float x0, float x1, float y) {
	int total = 0;
	float x;
	for (x = x0; x + 1.0f < x1; x += 1.0f) {
		const uint8_t* a = pixel_at(x, y);
		const uint8_t* c = pixel_at(x + 1.0f, y);
		int r = a[0], g = a[1], b = a[2];
		total += abs(r - c[0]) + abs(g - c[1]) + abs(b - c[2]);
	}
	return total;
}

static void test_materials(void) {
	/* four panels over the same stripes: each material lets them through in its own way */
	Rid window = window_from(
		"<Window Width=\"440\" Height=\"120\" ClearColor=\"#000000\">"
		"  <Overlay>"
		"    <HBox Name=\"stripes\"/>"
		"    <HBox Slot.Padding=\"10,30\" Spacing=\"20\">"
		"      <Border Name=\"solid\" Material=\"Solid\" Width=\"90\" CornerRadius=\"0\" Background=\"#202020\"/>"
		"      <Border Name=\"clear\" Material=\"Glass\" Width=\"90\" CornerRadius=\"0\" Background=\"#20202040\"/>"
		"      <Border Name=\"frosted\" Material=\"FrostedGlass\" Width=\"90\" CornerRadius=\"0\" Background=\"#20202040\"/>"
		"      <VBox Name=\"tinted\" Material=\"TintedGlass\" Width=\"90\"/>"
		"    </HBox>"
		"  </Overlay>"
		"</Window>");
	Rid stripes = Rwidget_find(window, "stripes");
	RFrameStats stats;
	const uint8_t* p;
	int i, behind, solid, clear, frosted;

	for (i = 0; i < 44; i++) {
		Rid stripe = Rwidget_add_child(stripes, Rwidget_create(R_WIDGET_BORDER));
		Rwidget_set_size(stripe, 10.0f, -1.0f);
		Rborder_set_background(stripe, (i & 1) ? 1.0f : 0.0f, (i & 1) ? 1.0f : 0.0f, (i & 1) ? 1.0f : 0.0f, 1.0f);
	}
	CHECK(Rwidget_get_material(Rwidget_find(window, "solid")) == R_MATERIAL_SOLID);
	CHECK(Rwidget_get_material(Rwidget_find(window, "clear")) == R_MATERIAL_GLASS);
	CHECK(Rwidget_get_material(Rwidget_find(window, "tinted")) == R_MATERIAL_TINTED_GLASS);
	CHECK(Rwidget_get_material(stripes) == R_MATERIAL_SOLID);
	capture(window);
	Rwindow_get_frame_stats(window, &stats);
	/* the theme is flat, and yet three widgets are glass */
	CHECK(stats.backdrop_count >= 1);

	behind = row_contrast(15.0f, 95.0f, 10.0f);
	solid = row_contrast(15.0f, 95.0f, 60.0f);
	clear = row_contrast(125.0f, 205.0f, 60.0f);
	frosted = row_contrast(235.0f, 315.0f, 60.0f);
	CHECK(behind > 2000);
	/* solid hides the stripes, clear glass keeps them readable, frosted glass melts them */
	CHECK(solid < 50);
	CHECK(clear > frosted * 2);
	CHECK(clear > behind / 6);
	CHECK(frosted < behind / 4);
	/* the widget that draws nothing by itself became a panel, colored by the accent: more blue than red */
	p = pixel_at(390, 60);
	CHECK(p[2] > p[0] + 20);

	/* the material of the theme is the one of every widget that has none */
	Rwidget_set_material(Rwidget_find(window, "solid"), R_MATERIAL_DEFAULT);
	CHECK(Rwidget_get_material(Rwidget_find(window, "solid")) == R_MATERIAL_SOLID);
	Rtheme_set_material(R_MATERIAL_FROSTED_GLASS);
	CHECK(Rtheme_get_material() == R_MATERIAL_FROSTED_GLASS);
	CHECK(Rwidget_get_material(Rwidget_find(window, "solid")) == R_MATERIAL_FROSTED_GLASS);
	CHECK(Rwidget_get_material(Rwidget_find(window, "clear")) == R_MATERIAL_GLASS);
	capture(window);
	CHECK(row_contrast(15.0f, 95.0f, 60.0f) > 50);
	Rtheme_apply(R_THEME_DARK);
	CHECK(Rtheme_get_material() == R_MATERIAL_SOLID);

	EXPECT_ERRORS_BEGIN();
	Rtheme_set_material(R_MATERIAL_DEFAULT);
	Rwidget_set_material(stripes, (RMaterial)7);
	EXPECT_ERRORS_END(2);
	Rwindow_destroy(window);
}

static void paint_curves(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	(void)widget;
	(void)width;
	(void)height;
	(void)user_data;
	/* a straight one, then one that bends from the top left to the bottom right */
	Rdraw_curve(painter, 20, 30, 70, 30, 130, 30, 180, 30, 8, 1, 0, 0, 1);
	Rdraw_curve(painter, 20, 80, 100, 80, 100, 160, 180, 160, 6, 0, 1, 0, 1);
}

static void test_curves_and_blur(void) {
	Rid window = window_from(
		"<Window Width=\"200\" Height=\"200\" ClearColor=\"#000000\"><Custom Name=\"custom\"/></Window>");
	Rid stripes;
	int i, behind, plain, frosted;

	Rcustom_set_paint_callback(Rwidget_find(window, "custom"), paint_curves, NULL);
	capture(window);
	/* the straight curve: its middle, its thickness, its round ends */
	CHECK_PIXEL(100, 30, 255, 0, 0);
	CHECK_PIXEL(100, 33, 255, 0, 0);
	CHECK_PIXEL(100, 38, 0, 0, 0);
	CHECK_PIXEL(18, 30, 255, 0, 0);
	CHECK_PIXEL(12, 30, 0, 0, 0);
	/* the bent one passes through its ends and its middle, and leaves the corners of its box alone */
	CHECK_PIXEL(25, 80, 0, 255, 0);
	CHECK_PIXEL(100, 120, 0, 255, 0);
	CHECK_PIXEL(175, 160, 0, 255, 0);
	CHECK_PIXEL(170, 90, 0, 0, 0);
	CHECK_PIXEL(30, 150, 0, 0, 0);
	Rwindow_destroy(window);

	/* a panel that blurs what is behind it: the stripes melt under it */
	window = window_from(
		"<Window Width=\"320\" Height=\"120\" ClearColor=\"#000000\">"
		"  <Overlay>"
		"    <HBox Name=\"stripes\"/>"
		"    <HBox Slot.Padding=\"10,30\" Spacing=\"20\">"
		"      <BackgroundBlur Name=\"plain\" Style=\"Plain\" Radius=\"16\" Width=\"130\" CornerRadius=\"0\"/>"
		"      <BackgroundBlur Name=\"frosted\" Style=\"Frosted\" Radius=\"16\" Width=\"130\" CornerRadius=\"0\" Grain=\"0\"/>"
		"    </HBox>"
		"  </Overlay>"
		"</Window>");
	stripes = Rwidget_find(window, "stripes");
	for (i = 0; i < 32; i++) {
		Rid stripe = Rwidget_add_child(stripes, Rwidget_create(R_WIDGET_BORDER));
		Rwidget_set_size(stripe, 10.0f, -1.0f);
		Rborder_set_background(stripe, (i & 1) ? 1.0f : 0.0f, (i & 1) ? 1.0f : 0.0f, (i & 1) ? 1.0f : 0.0f, 1.0f);
	}
	capture(window);
	behind = row_contrast(20.0f, 130.0f, 10.0f);
	plain = row_contrast(20.0f, 130.0f, 60.0f);
	frosted = row_contrast(170.0f, 280.0f, 60.0f);
	CHECK(behind > 2000);
	CHECK(plain < behind / 5);
	CHECK(frosted < behind / 5);
	/* blurred, not hidden: half white, half black gives a gray */
	{
		const uint8_t* p = pixel_at(75, 60);
		CHECK(p[0] > 60 && p[0] < 200);
	}
	Rwindow_destroy(window);
}

static void test_node_graph(void) {
	Rid window = window_from(
		"<Window Width=\"520\" Height=\"220\" ClearColor=\"#000000\">"
		"  <NodeGraph Name=\"g\" Grid=\"false\" Flow=\"false\" Color.Input=\"#000000\">"
		"    <Type Name=\"signal\" Color=\"#00FF00\"/>"
		"    <Node Name=\"a\" Title=\"A\" X=\"30\" Y=\"40\" Color=\"#FF0000\"><Output Name=\"o\" Label=\"Out\" Type=\"signal\"/></Node>"
		"    <Node Name=\"b\" Title=\"B\" X=\"330\" Y=\"40\" Color=\"#0000FF\"><Input Name=\"i\" Label=\"In\" Type=\"signal\"/></Node>"
		"    <Link From=\"a.o\" To=\"b.i\"/>"
		"  </NodeGraph>"
		"</Window>");
	Rid graph = Rwidget_find(window, "g");
	float ax, ay, bx, by, w, h;
	const uint8_t* p;
	int y, found = 0;

	capture(window);
	Rnode_graph_get_node_size(graph, Rnode_graph_find_node(graph, "a"), &w, &h);
	/* the headers wear the colors of their nodes */
	p = pixel_at(30.0f + w - 20.0f, 40.0f + 15.0f);
	CHECK(p[0] > 150 && p[1] < 90 && p[2] < 90);
	p = pixel_at(330.0f + w - 20.0f, 40.0f + 15.0f);
	CHECK(p[2] > 150 && p[0] < 90 && p[1] < 110);
	/* the canvas between the nodes is empty, except for the wire: it has the color of its type and hangs a little under the straight line */
	Rnode_graph_get_pin_position(graph, Rnode_graph_find_pin(graph, Rnode_graph_find_node(graph, "a"), "o"), &ax, &ay);
	Rnode_graph_get_pin_position(graph, Rnode_graph_find_pin(graph, Rnode_graph_find_node(graph, "b"), "i"), &bx, &by);
	CHECK(ay == by && bx > ax + 100.0f);
	for (y = (int)ay - 2; y < (int)ay + 26 && !found; y++) {
		p = pixel_at((ax + bx) * 0.5f, (float)y);
		if (p[1] > 200 && p[0] < 120 && p[2] < 120) found = y;
	}
	CHECK(found > (int)ay);
	p = pixel_at((ax + bx) * 0.5f, ay - 30.0f);
	CHECK(p[0] < 12 && p[1] < 12 && p[2] < 12);
	p = pixel_at((ax + bx) * 0.5f, ay + 60.0f);
	CHECK(p[0] < 12 && p[1] < 12 && p[2] < 12);
	/* the pins are dots of that color on the edges */
	p = pixel_at(ax, ay);
	CHECK(p[1] > 180 && p[0] < 140);

	/* zoomed out around the middle, everything is twice as close to it */
	Rnode_graph_set_view(graph, 260.0f, 110.0f, 0.5f);
	capture(window);
	p = pixel_at(260.0f + (30.0f + w - 20.0f - 260.0f) * 0.5f, 110.0f + (40.0f + 15.0f - 110.0f) * 0.5f);
	CHECK(p[0] > 150 && p[1] < 90 && p[2] < 90);
	p = pixel_at(30.0f + w - 20.0f, 40.0f + 15.0f);
	CHECK(p[0] < 12 && p[1] < 12 && p[2] < 12);
	Rwindow_destroy(window);
}

static int g_callback_calls;
static RViewportDrawInfo g_callback_info;

/* Render thread. */
static void on_viewport_draw(const RViewportDrawInfo* info, void* user_data) {
	(void)user_data;
	g_callback_info = *info;
	g_callback_calls++;
}

static void test_viewport(void) {
	static uint8_t pixels[8 * 8 * 4];
	Rid window = window_from(
		"<Window Width=\"300\" Height=\"200\" ClearColor=\"#000000\">"
		"  <Canvas>"
		"    <Viewport Name=\"texture\" Slot.Position=\"10,10\" Slot.Size=\"100,100\"/>"
		"    <Viewport Name=\"callback\" Slot.Position=\"150,20\" Slot.Size=\"120,80\"/>"
		"  </Canvas>"
		"</Window>");
	Rid texture_viewport = Rwidget_find(window, "texture");
	Rid texture;
	int width = 0, height = 0, i;

	/* texture mode */
	for (i = 0; i < 64; i++) {
		pixels[i * 4 + 0] = 0;
		pixels[i * 4 + 1] = 200;
		pixels[i * 4 + 2] = 100;
		pixels[i * 4 + 3] = 255;
	}
	texture = Rtexture_create_from_pixels(pixels, 8, 8);
	Rviewport_set_texture(texture_viewport, texture);
	/* callback mode */
	Rviewport_set_draw_callback(Rwidget_find(window, "callback"), on_viewport_draw, NULL);

	capture(window);
	CHECK_PIXEL(60, 60, 0, 200, 100);
	CHECK_PIXEL(120, 60, 0, 0, 0);
	Rviewport_get_pixel_size(texture_viewport, &width, &height);
	CHECK(width == (int)(100 * g_scale) && height == (int)(100 * g_scale));

	/* the capture waited for the frame, so the callback already ran on the render thread */
	CHECK(g_callback_calls > 0);
	CHECK(g_callback_info.backend == R_BACKEND_VULKAN);
	CHECK(g_callback_info.window == window);
	CHECK(g_callback_info.command_buffer != NULL && g_callback_info.render_pass != NULL);
	CHECK(g_callback_info.x == (int)(150 * g_scale) && g_callback_info.y == (int)(20 * g_scale));
	CHECK(g_callback_info.width == (int)(120 * g_scale) && g_callback_info.height == (int)(80 * g_scale));
	CHECK(g_callback_info.clip_width == g_callback_info.width);

	Rtexture_destroy(texture);
	Rwindow_destroy(window);
}

static void test_windows(void) {
	Rid first = window_from("<Window Width=\"200\" Height=\"100\" ClearColor=\"#FF0000\"/>");
	Rid second = window_from("<Window Width=\"120\" Height=\"160\" ClearColor=\"#0000FF\"/>");
	int i;

	/* each window has its own swapchain */
	capture(second);
	{
		/* the window system may make a window larger than asked (Windows has a minimum width): the swapchain follows the window */
		int fw = 0, fh = 0;
		Rwindow_get_framebuffer_size(second, &fw, &fh);
		CHECK(g_width == fw && g_height == fh);
		CHECK(g_width >= (int)(120 * g_scale) && g_height >= (int)(160 * g_scale));
	}
	CHECK_PIXEL(60, 80, 0, 0, 255);
	capture(first);
	CHECK_PIXEL(100, 50, 255, 0, 0);

	/* resizing recreates the swapchain; a real window system confirms the size with an event */
	Rwindow_set_size(first, 260, 140);
	for (i = 0; i < 50; i++) {
		int width = 0, height = 0;
		Rpoll_events();
		Rdraw_frame();
		Rwindow_get_size(first, &width, &height);
		if (width == 260 && height == 140) break;
#if !defined(_WIN32)
		usleep(10000);
#endif
	}
	capture(first);
	CHECK(g_width == (int)(260 * g_scale) && g_height == (int)(140 * g_scale));
	CHECK_PIXEL(250, 130, 255, 0, 0);

	Rwindow_set_clear_color(first, 0.0f, 1.0f, 0.0f, 1.0f);
	capture(first);
	CHECK_PIXEL(10, 10, 0, 255, 0);

	Rwindow_destroy(second);
	capture(first);
	CHECK_PIXEL(10, 10, 0, 255, 0);
	Rwindow_destroy(first);
}

static void test_vulkan_api(void) {
	Ruint count = 0;
	const char* const* names;
	Rid shared;

	CHECK(Rvulkan_get_instance() != NULL);
	CHECK(Rvulkan_get_physical_device() != NULL);
	CHECK(Rvulkan_get_device() != NULL);
	CHECK(Rvulkan_get_graphics_queue() != NULL);
	names = Rvulkan_get_required_instance_extensions(&count);
	CHECK(names != NULL && count >= 2 && strcmp(names[0], "VK_KHR_surface") == 0);
	names = Rvulkan_get_required_device_extensions(&count);
	CHECK(names != NULL && count == 1 && strcmp(names[0], "VK_KHR_swapchain") == 0);

	/* optional feature: not every device can export memory */
	EXPECT_ERRORS_BEGIN();
	shared = Rvulkan_texture_create_shared(64, 32);
	EXPECT_ERRORS_END(0);
	if (shared != R_INVALID_ID) {
		RSharedTextureInfo info;
		memset(&info, 0, sizeof(info));
		CHECK(Rvulkan_texture_get_shared_info(shared, &info));
		CHECK(info.width == 64 && info.height == 32);
		CHECK(info.allocation_size >= 64 * 32 * 4);
#if !defined(_WIN32)
		CHECK(info.handle >= 0);
		if (info.handle >= 0) close((int)info.handle);
#endif
		Rtexture_destroy(shared);
		printf("  shared textures: supported\n");
	} else {
		printf("  shared textures: not supported by this device\n");
	}
}

/* ---------------------------------------------------------------------------
 *  Start-up
 * ------------------------------------------------------------------------- */

/* Starts the Vulkan backend and checks that a window can really be rendered. */
/* ---------------------------------------------------------------------------
 *  Loss of the device
 * ------------------------------------------------------------------------- */

static int g_resets;
static Rid g_big_texture;
static uint8_t* g_big_pixels;

static void on_device_reset(void* user_data) {
	(void)user_data;
	g_resets++;
	/* the pixels only the application has: given again */
	Rtexture_update(g_big_texture, g_big_pixels, 300, 300);
}

static int lit_pixels(float x0, float y0, float w, float h) {
	int lit = 0;
	float x, y;
	for (y = y0; y < y0 + h; y += 1.0f) {
		for (x = x0; x < x0 + w; x += 1.0f) {
			if (pixel_at(x, y)[0] > 128) lit++;
		}
	}
	return lit;
}

static void test_device_lost(void) {
	static const uint8_t small[16] = {
		255, 0, 0, 255,   255, 0, 0, 255,
		255, 0, 0, 255,   255, 0, 0, 255
	};
	Rid window = window_from(
		"<Window Width=\"400\" Height=\"200\" ClearColor=\"#000000\">"
		"  <Canvas>"
		"    <Image Name=\"small\" Slot.Position=\"10,10\" Slot.Size=\"40,40\"/>"
		"    <Image Name=\"big\" Slot.Position=\"60,10\" Slot.Size=\"40,40\"/>"
		"    <Label Name=\"label\" Slot.Position=\"120,10\" TextSize=\"40\" Color=\"#FFFFFF\">Raven</Label>"
		"    <Border Name=\"effect\" Slot.Position=\"10,120\" Slot.Size=\"60,60\" Effect=\"plasma\"/>"
		"  </Canvas>"
		"</Window>");
	Rid small_texture = Rtexture_create_from_pixels(small, 2, 2);
	void* device_before = Rvulkan_get_device();
	int i, lit_before, lit_after;
	const uint8_t* p;

	g_big_pixels = (uint8_t*)malloc(300 * 300 * 4);
	for (i = 0; i < 300 * 300; i++) {
		g_big_pixels[i * 4 + 0] = 0;
		g_big_pixels[i * 4 + 1] = 255;
		g_big_pixels[i * 4 + 2] = 0;
		g_big_pixels[i * 4 + 3] = 255;
	}
	g_big_texture = Rtexture_create_from_pixels(g_big_pixels, 300, 300);
	Rimage_set_texture(Rwidget_find(window, "small"), small_texture);
	Rimage_set_texture(Rwidget_find(window, "big"), g_big_texture);
	Rimage_set_stretch(Rwidget_find(window, "small"), R_STRETCH_FILL);
	Rimage_set_stretch(Rwidget_find(window, "big"), R_STRETCH_FILL);
	capture(window);
	CHECK_PIXEL(30, 30, 255, 0, 0);
	CHECK_PIXEL(80, 30, 0, 255, 0);
	lit_before = lit_pixels(120.0f, 10.0f, 200.0f, 60.0f);
	CHECK(lit_before > 300);
	p = pixel_at(40, 150);
	CHECK(p[0] + p[1] + p[2] > 60);

	/* lost: Raven makes a new device at the next frame, and everything again */
	g_resets = 0;
	Rset_device_reset_callback(on_device_reset, NULL);
	EXPECT_ERRORS_BEGIN();
	Rvulkan_simulate_device_lost();
	Rdraw_frame();
	EXPECT_ERRORS_END(1);
	CHECK(g_resets == 1);
	CHECK(Rvulkan_get_device() != NULL);
	if (Rvulkan_get_device() == device_before) printf("  (the driver gave the same VkDevice handle again)\n");

	capture(window);
	/* a small texture: from the copy of its pixels */
	CHECK_PIXEL(30, 30, 255, 0, 0);
	/* a large one: given again by the application in the callback */
	CHECK_PIXEL(80, 30, 0, 255, 0);
	/* the glyphs: from the copy the fonts keep */
	lit_after = lit_pixels(120.0f, 10.0f, 200.0f, 60.0f);
	CHECK(lit_after == lit_before);
	/* the shader of the effect */
	p = pixel_at(40, 150);
	CHECK(p[0] + p[1] + p[2] > 60);

	/* without the callback, the large texture comes back transparent */
	Rset_device_reset_callback(NULL, NULL);
	EXPECT_ERRORS_BEGIN();
	Rvulkan_simulate_device_lost();
	Rdraw_frame();
	EXPECT_ERRORS_END(1);
	capture(window);
	CHECK_PIXEL(30, 30, 255, 0, 0);
	CHECK_PIXEL(80, 30, 0, 0, 0);
	/* and works as before when it is filled again */
	Rtexture_update(g_big_texture, g_big_pixels, 300, 300);
	capture(window);
	CHECK_PIXEL(80, 30, 0, 255, 0);

	/* a window opened after the recovery */
	Rwindow_destroy(window);
	window = window_from("<Window Width=\"100\" Height=\"100\" ClearColor=\"#0000FF\"/>");
	capture(window);
	CHECK_PIXEL(50, 50, 0, 0, 255);
	Rwindow_destroy(window);

	Rtexture_destroy(small_texture);
	Rtexture_destroy(g_big_texture);
	free(g_big_pixels);
	g_big_pixels = NULL;
}

static int start(int headless) {
	RInitDesc desc;
	Rid probe;
	uint8_t* pixels;
	int width = 0, height = 0;

	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_VULKAN;
	desc.headless = headless;
	desc.app_name = "test_render";
	Rerror_set_callback(test_error_callback);
	if (!Rinit_ex(&desc)) return 0;
	probe = Rwindow_create("probe", 64, 64, R_WINDOW_DEFAULT);
	pixels = probe != R_INVALID_ID ? Rwindow_capture(probe, &width, &height) : NULL;
	Rpixels_free(pixels);
	if (probe != R_INVALID_ID) Rwindow_destroy(probe);
	if (!pixels) {
		Rshutdown();
		return 0;
	}
	/* pixels are compared right after a change: no animation in between */
	Rset_animations_enabled(R_FALSE);
	return 1;
}

/* The look of Windows, and a style given to a part: drawn as said. */
static void test_windows_look(void) {
	Rid window;
	Rtheme_apply(R_THEME_DARK);
	Rlook_set(R_LOOK_WINDOWS_LIGHT);
	window = window_from(
		"<Window Width=\"300\" Height=\"200\">"
		"  <Canvas>"
		"    <Button Text=\"\" Slot.Position=\"20,20\" Slot.Size=\"120,32\"/>"
		"    <Button Text=\"\" Class=\"red\" Slot.Position=\"20,80\" Slot.Size=\"120,32\"/>"
		"    <Button Text=\"\" Style=\"Primary\" Slot.Position=\"160,20\" Slot.Size=\"120,32\"/>"
		"  </Canvas>"
		"</Window>");
	CHECK(Rtheme_load_from_memory("<Theme Name=\"RedButtons\"><Style Widget=\"Button\" Class=\"red\" Background=\"#FF0000\"/></Theme>", 0) != NULL);
	CHECK(Rtheme_use("RedButtons"));
	capture(window);
	/* the window: Mica, light */
	CHECK_PIXEL(5, 5, 0xF3, 0xF3, 0xF3);
	/* a button: white at 70 % over it */
	CHECK_PIXEL(80, 36, 0xFB, 0xFB, 0xFB);
	/* the accent button: the accent of Windows (or the one of the system) */
	CHECK(abs((int)pixel_at(220, 36)[0] - 0xF3) + abs((int)pixel_at(220, 36)[1] - 0xF3) + abs((int)pixel_at(220, 36)[2] - 0xF3) > 60);
	/* the class "red": its Background */
	CHECK_PIXEL(80, 96, 0xFF, 0x00, 0x00);
	Rwindow_destroy(window);
	Rtheme_apply(R_THEME_DARK);
}

int main(void) {
	/* failures to start are expected on machines without Vulkan: stay quiet */
	/* RAVEN_TEST_WINDOWED=1 forces a real window, to test the window system path too */
	int windowed = getenv("RAVEN_TEST_WINDOWED") != NULL;
	EXPECT_ERRORS_BEGIN();
	if (!windowed && start(R_TRUE)) {
		printf("test_render: Vulkan without a window system (headless surface)\n");
	} else if (start(R_FALSE)) {
		printf("test_render: Vulkan with a real window\n");
	} else {
		printf("test_render: SKIPPED, no usable Vulkan driver or display\n");
		return SKIP_CODE;
	}
	g_test_expect_errors = 0;
	g_test_checks = 0;
	g_test_failures = 0;

	RUN(test_shapes);
	RUN(test_clipping);
	RUN(test_textures);
	RUN(test_mipmaps);
	RUN(test_text);
	RUN(test_custom_drawing);
	RUN(test_glass);
	RUN(test_windows_look);
	RUN(test_glass_theme);
	RUN(test_materials);
	RUN(test_curves_and_blur);
	RUN(test_node_graph);
	RUN(test_viewport);
	RUN(test_windows);
	RUN(test_vulkan_api);
	RUN(test_device_lost);
	/* everything works on the new device */
	RUN(test_shapes);
	RUN(test_text);
	RUN(test_glass);

	Rpixels_free(g_pixels);
	Rshutdown();
	return test_report("test_render");
}
