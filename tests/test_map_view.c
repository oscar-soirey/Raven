/**
 * Raven tests - MapView: a map of slippy-map tiles.
 *
 * Headless: the projection, which tiles are asked for (once, only those
 * seen and a ring around them), answers that come now, later, from another
 * thread or too late, the coarser tile shown in place of a missing one, the
 * cache under its budget, tiles read from files, the mouse, the wheel and
 * the keys, markers, XML, and the errors. No tile server is ever contacted.
 */

#include "test.h"
#include <stdlib.h>
#if !defined(RAVEN_TEST_SHARED)
#include "../src/core/accessibility.h"
/* (widgets/map_view.c: what the last paint drew) */
int rvn_map_view_debug_counts(Rid id, int* loaded, int* fallback, int* empty);
#endif

#define MAP_W 512
#define MAP_H 512

static Rid g_window;

/* ---- what the map asked for ---- */

#define MAX_LOG 4096
static struct {
	int z, x, y;
} g_requests[MAX_LOG], g_cancels[MAX_LOG];
static int g_request_count, g_cancel_count;
/* answer at once, with tiles of this many pixels on a side (0: do not answer) */
static int g_auto_answer;
static int g_view_calls;
static double g_view_zoom;
static int g_marker_clicks, g_last_marker;
/* raised by a widget painted just before the map, lowered by one painted just after it */
static int g_painting, g_view_calls_while_painting;

static void on_request(Rid map, int z, int x, int y, void* user_data) {
	CHECK(user_data == &g_request_count);
	if (g_request_count < MAX_LOG) {
		g_requests[g_request_count].z = z;
		g_requests[g_request_count].x = x;
		g_requests[g_request_count].y = y;
	}
	g_request_count++;
	if (g_auto_answer > 0) {
		static uint8_t pixels[64 * 64 * 4];
		memset(pixels, (z * 40 + x * 7 + y * 3) & 0xFF, sizeof(pixels));
		Rmap_view_set_tile(map, z, x, y, pixels, g_auto_answer, g_auto_answer);
	}
}

static void on_cancel(Rid map, int z, int x, int y, void* user_data) {
	(void)map;
	CHECK(user_data == &g_cancel_count);
	if (g_cancel_count < MAX_LOG) {
		g_cancels[g_cancel_count].z = z;
		g_cancels[g_cancel_count].x = x;
		g_cancels[g_cancel_count].y = y;
	}
	g_cancel_count++;
}

static void on_view(Rid map, double latitude, double longitude, double zoom, void* user_data) {
	(void)map;
	(void)latitude;
	(void)longitude;
	CHECK(user_data == &g_view_calls);
	g_view_calls++;
	g_view_zoom = zoom;
	if (g_painting) g_view_calls_while_painting++;
}

static void on_paint_before(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	(void)widget;
	(void)painter;
	(void)width;
	(void)height;
	(void)user_data;
	g_painting = R_TRUE;
}

static void on_paint_after(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	(void)widget;
	(void)painter;
	(void)width;
	(void)height;
	(void)user_data;
	g_painting = R_FALSE;
}

static void on_marker(Rid map, int marker, void* user_data) {
	(void)map;
	CHECK(user_data == &g_marker_clicks);
	g_marker_clicks++;
	g_last_marker = marker;
}

static int was_requested(int z, int x, int y) {
	int i, count = 0;
	for (i = 0; i < g_request_count && i < MAX_LOG; i++)
		if (g_requests[i].z == z && g_requests[i].x == x && g_requests[i].y == y) count++;
	return count;
}

/* A map filling a window of MAP_W x MAP_H. */
static Rid new_map(void) {
	Rid map = Rmap_view_create();
	Rwindow_set_content(g_window, map);
	Rdraw_frame();
	return map;
}

/* Frames until a tile is no longer pending (the files and the other threads answer through the UI thread). */
static RMapTileState settle(Rid map, int z, int x, int y) {
	int i;
	for (i = 0; i < 400 && Rmap_view_get_tile_state(map, z, x, y) == R_MAP_TILE_PENDING; i++) Rwait_frame(0.005);
	return Rmap_view_get_tile_state(map, z, x, y);
}

/* The latitude and longitude of the middle of a tile. */
static void tile_middle(int z, int x, int y, double* latitude, double* longitude) {
	const double pi = 3.14159265358979323846;
	double n = (double)(1 << z);
	*longitude = (x + 0.5) / n * 360.0 - 180.0;
	*latitude = atan(sinh(pi * (1.0 - 2.0 * (y + 0.5) / n))) * 180.0 / pi;
}

#define CHECK_CLOSE(actual, expected, tolerance) do { \
	double test_a_ = (double)(actual), test_e_ = (double)(expected); \
	g_test_checks++; \
	if (!(fabs(test_a_ - test_e_) <= (tolerance))) { \
		g_test_failures++; \
		printf("  FAIL %s:%d: %s = %.9g, expected %.9g\n", __FILE__, __LINE__, #actual, test_a_, test_e_); \
	} \
} while (0)

/* ---- a PNG written here: stored (uncompressed) deflate blocks ---- */

static uint32_t crc_table[256];

static uint32_t crc32_of(const uint8_t* data, size_t size, uint32_t crc) {
	size_t i;
	if (!crc_table[1]) {
		uint32_t n, k, c;
		for (n = 0; n < 256; n++) {
			c = n;
			for (k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
			crc_table[n] = c;
		}
	}
	crc ^= 0xFFFFFFFFu;
	for (i = 0; i < size; i++) crc = crc_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
	return crc ^ 0xFFFFFFFFu;
}

static void put32(uint8_t* out, uint32_t value) {
	out[0] = (uint8_t)(value >> 24);
	out[1] = (uint8_t)(value >> 16);
	out[2] = (uint8_t)(value >> 8);
	out[3] = (uint8_t)value;
}

static size_t chunk(uint8_t* out, const char* type, const uint8_t* data, uint32_t size) {
	put32(out, size);
	memcpy(out + 4, type, 4);
	if (size) memcpy(out + 8, data, size);
	put32(out + 8 + size, crc32_of(out + 4, size + 4, 0));
	return 12 + size;
}

/* A `side` x `side` PNG of one color (side <= 32, at most 4300 bytes). Returns its size. */
static size_t make_png(uint8_t* out, int side, uint8_t r, uint8_t g, uint8_t b) {
	static const uint8_t signature[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
	uint8_t header[13], raw[32 * (32 * 4 + 1)], zlib[sizeof(raw) + 16];
	size_t n = 0, raw_size = 0, z = 0;
	uint32_t a = 1, s = 0;
	int row, col;
	for (row = 0; row < side; row++) {
		raw[raw_size++] = 0;
		for (col = 0; col < side; col++) {
			raw[raw_size++] = r;
			raw[raw_size++] = g;
			raw[raw_size++] = b;
			raw[raw_size++] = 255;
		}
	}
	zlib[z++] = 0x78;
	zlib[z++] = 0x01;
	zlib[z++] = 1; /* the last block, stored */
	zlib[z++] = (uint8_t)(raw_size & 0xFF);
	zlib[z++] = (uint8_t)(raw_size >> 8);
	zlib[z++] = (uint8_t)(~raw_size & 0xFF);
	zlib[z++] = (uint8_t)((~raw_size >> 8) & 0xFF);
	memcpy(zlib + z, raw, raw_size);
	z += raw_size;
	for (col = 0; col < (int)raw_size; col++) {
		a = (a + raw[col]) % 65521u;
		s = (s + a) % 65521u;
	}
	put32(zlib + z, (s << 16) | a);
	z += 4;
	put32(header, (uint32_t)side);
	put32(header + 4, (uint32_t)side);
	header[8] = 8;
	header[9] = 6;
	header[10] = header[11] = header[12] = 0;
	memcpy(out, signature, 8);
	n = 8;
	n += chunk(out + n, "IHDR", header, 13);
	n += chunk(out + n, "IDAT", zlib, (uint32_t)z);
	n += chunk(out + n, "IEND", NULL, 0);
	return n;
}

/* ---------------------------------------------------------------------------
 *  Tests
 * ------------------------------------------------------------------------- */

static void test_projection(void) {
	Rid map = new_map();
	static const double zooms[] = { 2.5, 5.0, 10.0, 17.25 };
	double lat, lon, min_zoom, max_zoom;
	float x, y, w, h;
	size_t i;
	int px, py;

	Rwidget_get_rect(map, &x, &y, &w, &h);
	CHECK_NEAR(w, MAP_W);
	CHECK_NEAR(h, MAP_H);
	CHECK(Rwidget_get_type(map) == R_WIDGET_MAP_VIEW);

	/* the whole world in one tile at zoom 0: its corners are the limits of the projection */
	Rmap_view_set_zoom(map, 0.0);
	Rmap_view_set_center(map, 0.0, 0.0);
	Rmap_view_screen_to_latlon(map, MAP_W * 0.5f - 128.0f, MAP_H * 0.5f - 128.0f, &lat, &lon);
	CHECK_CLOSE(lat, 85.0511287798, 1e-6);
	CHECK_CLOSE(lon, -180.0, 1e-6);
	Rmap_view_latlon_to_screen(map, 0.0, 90.0, &x, &y);
	CHECK_CLOSE(x, MAP_W * 0.5 + 64.0, 1e-3);
	CHECK_CLOSE(y, MAP_H * 0.5, 1e-3);

	/* the center, set and read */
	Rmap_view_set_center(map, 48.8566, 2.3522);
	Rmap_view_get_center(map, &lat, &lon);
	CHECK_CLOSE(lat, 48.8566, 1e-9);
	CHECK_CLOSE(lon, 2.3522, 1e-9);
	/* latitudes stop at the limits of Mercator; longitudes wrap */
	Rmap_view_set_center(map, 89.0, 190.0);
	Rmap_view_get_center(map, &lat, &lon);
	CHECK_CLOSE(lat, 85.0511287798, 1e-6);
	CHECK_CLOSE(lon, -170.0, 1e-9);

	/* points of the widget to places and back, at several zooms */
	for (i = 0; i < sizeof(zooms) / sizeof(zooms[0]); i++) {
		Rmap_view_set_zoom(map, zooms[i]);
		Rmap_view_set_center(map, -33.8688, 151.2093);
		CHECK_CLOSE(Rmap_view_get_zoom(map), zooms[i], 1e-12);
		Rmap_view_latlon_to_screen(map, -33.8688, 151.2093, &x, &y);
		CHECK_CLOSE(x, MAP_W * 0.5, 1e-3);
		CHECK_CLOSE(y, MAP_H * 0.5, 1e-3);
		for (py = 0; py <= MAP_H; py += 128) {
			for (px = 0; px <= MAP_W; px += 128) {
				Rmap_view_screen_to_latlon(map, (float)px, (float)py, &lat, &lon);
				Rmap_view_latlon_to_screen(map, lat, lon, &x, &y);
				CHECK_CLOSE(x, px, 2e-3);
				CHECK_CLOSE(y, py, 2e-3);
			}
		}
		/* one tile further east is 256 units right (the scale of the zoom) */
		Rmap_view_screen_to_latlon(map, MAP_W * 0.5f + 256.0f, MAP_H * 0.5f, &lat, &lon);
		CHECK_CLOSE(fmod(lon - 151.2093 + 720.0, 360.0), 360.0 / pow(2.0, zooms[i]), 1e-6);
	}

	/* the zoom and its range */
	Rmap_view_get_zoom_range(map, &min_zoom, &max_zoom);
	CHECK(min_zoom == 0.0 && max_zoom == 19.0);
	Rmap_view_set_zoom(map, 30.0);
	CHECK(Rmap_view_get_zoom(map) == 19.0);
	Rmap_view_set_zoom_range(map, 2.0, 12.0);
	CHECK(Rmap_view_get_zoom(map) == 12.0);
	Rmap_view_set_zoom(map, 0.5);
	CHECK(Rmap_view_get_zoom(map) == 2.0);
	CHECK(Rmap_view_get_tile_size(map) == 256);
	Rdraw_frame();
}

static void test_requests(void) {
	Rid map = new_map();
	uint8_t pixels[8 * 8 * 4];
	int i, j, ok = 1;
	double lat, lon;

	g_request_count = g_cancel_count = 0;
	g_auto_answer = 0;
	Rmap_view_set_zoom(map, 4.0);
	tile_middle(4, 8, 8, &lat, &lon);
	Rmap_view_set_center(map, lat, lon);
	/* without a source nothing is asked */
	Rdraw_frame();
	CHECK(Rmap_view_get_tile_state(map, 4, 8, 8) == R_MAP_TILE_NONE);

	Rmap_view_set_tile_callback(map, on_request, &g_request_count);
	Rmap_view_set_tile_cancel_callback(map, on_cancel, &g_cancel_count);
	Rdraw_frame();
	/*
	 * zoom 4: 16 x 16 tiles of 256; the middle of tile 8/8 at the middle of
	 * 512 x 512: tiles 7..9 are seen in each direction, 6..10 wanted
	 */
	CHECK(g_request_count == 25);
	for (i = 0; i < g_request_count && i < MAX_LOG; i++) {
		if (g_requests[i].z != 4 || g_requests[i].x < 6 || g_requests[i].x > 10 || g_requests[i].y < 6 || g_requests[i].y > 10) ok = 0;
		for (j = 0; j < i; j++)
			if (g_requests[j].x == g_requests[i].x && g_requests[j].y == g_requests[i].y) ok = 0;
	}
	CHECK(ok);
	/* the nearest the middle first */
	CHECK(g_requests[0].x == 8 && g_requests[0].y == 8);
	CHECK(Rmap_view_get_tile_state(map, 4, 8, 8) == R_MAP_TILE_PENDING);
	CHECK(Rmap_view_get_tile_state(map, 4, 5, 8) == R_MAP_TILE_NONE);

	/* asked once: more frames and a small move ask nothing more */
	Rdraw_frame();
	Rmap_view_set_center(map, lat + 0.5, lon + 0.5);
	Rdraw_frame();
	Rdraw_frame();
	CHECK(g_request_count == 25);
	CHECK(g_cancel_count == 0);

	/* an answer now */
	memset(pixels, 200, sizeof(pixels));
	Rmap_view_set_tile(map, 4, 8, 8, pixels, 8, 8);
	CHECK(Rmap_view_get_tile_state(map, 4, 8, 8) == R_MAP_TILE_LOADED);
	CHECK(Rmap_view_get_cache_usage(map) == sizeof(pixels));
	Rmap_view_set_tile_failed(map, 4, 9, 9);
	CHECK(Rmap_view_get_tile_state(map, 4, 9, 9) == R_MAP_TILE_FAILED);
	CHECK(Rmap_view_get_cache_usage(map) == sizeof(pixels));
	/* an answer again replaces the pixels */
	Rmap_view_set_tile(map, 4, 8, 8, pixels, 4, 4);
	CHECK(Rmap_view_get_cache_usage(map) == 4 * 4 * 4);
#if !defined(RAVEN_TEST_NO_IMAGE_DECODER)
	{
		uint8_t png[8192];
		size_t size = make_png(png, 16, 10, 120, 200);
		Rmap_view_set_tile_data(map, 4, 7, 7, png, size);
		CHECK(Rmap_view_get_tile_state(map, 4, 7, 7) == R_MAP_TILE_LOADED);
		CHECK(Rmap_view_get_cache_usage(map) == 4 * 4 * 4 + 16 * 16 * 4);
	}
#endif
	Rdraw_frame();
	/* answered tiles are not asked again */
	CHECK(g_request_count == 25);

	/* elsewhere: the pending tiles of here are forgotten, the loaded ones kept */
	Rmap_view_set_center(map, lat, 120.0);
	Rdraw_frame();
	CHECK(g_cancel_count >= 20);
	CHECK(Rmap_view_get_tile_state(map, 4, 10, 10) == R_MAP_TILE_NONE);
	CHECK(Rmap_view_get_tile_state(map, 4, 8, 8) == R_MAP_TILE_LOADED);
	CHECK(was_requested(4, 13, 8) == 1);
	ok = 1;
	for (i = 0; i < g_cancel_count && i < MAX_LOG; i++)
		if (g_cancels[i].x < 6 || g_cancels[i].x > 10 || was_requested(g_cancels[i].z, g_cancels[i].x, g_cancels[i].y) != 1) ok = 0;
	CHECK(ok);
	/* a late answer for a tile no longer wanted is dropped, quietly */
	Rmap_view_set_tile(map, 4, 10, 10, pixels, 8, 8);
	Rmap_view_set_tile_failed(map, 4, 6, 6);
	CHECK(Rmap_view_get_tile_state(map, 4, 10, 10) == R_MAP_TILE_NONE);
	CHECK(Rmap_view_get_tile_state(map, 4, 6, 6) == R_MAP_TILE_NONE);
	/* the answer for a tile of here is kept */
	Rmap_view_set_tile(map, 4, 12, 7, pixels, 8, 8);
	CHECK(Rmap_view_get_tile_state(map, 4, 12, 7) == R_MAP_TILE_LOADED);

	/* longitudes wrap: at the antimeridian, columns 15 and 0 are asked for */
	g_request_count = 0;
	Rmap_view_set_center(map, 0.0, 180.0);
	Rdraw_frame();
	CHECK(Rmap_view_get_tile_state(map, 4, 15, 8) == R_MAP_TILE_PENDING);
	CHECK(was_requested(4, 0, 8) == 1);
	CHECK(was_requested(4, 1, 8) == 1);

	/* a new source forgets everything */
	Rmap_view_set_tile_callback(map, on_request, &g_request_count);
	CHECK(Rmap_view_get_tile_state(map, 4, 8, 8) == R_MAP_TILE_NONE);
	CHECK(Rmap_view_get_cache_usage(map) == 0);
	Rmap_view_clear_tiles(map);
	Rdraw_frame();
}

/* Another thread answers, as a loader would. */
typedef struct {
	Rid map;
	int z, x, y;
	int failed;
} answer_t;

static void answer_from_thread(void* user_data) {
	answer_t* answer = (answer_t*)user_data;
	uint8_t pixels[8 * 8 * 4];
	memset(pixels, 90, sizeof(pixels));
	if (answer->failed) Rmap_view_set_tile_failed(answer->map, answer->z, answer->x, answer->y);
	else Rmap_view_set_tile(answer->map, answer->z, answer->x, answer->y, pixels, 8, 8);
}

static void test_async(void) {
	Rid map = new_map();
	static answer_t answers[3];
	g_request_count = g_cancel_count = 0;
	g_auto_answer = 0;
	Rmap_view_set_zoom(map, 3.0);
	Rmap_view_set_center(map, 0.0, 0.0);
	Rmap_view_set_tile_callback(map, on_request, &g_request_count);
	Rdraw_frame();
	CHECK(Rmap_view_get_tile_state(map, 3, 4, 4) == R_MAP_TILE_PENDING);

	answers[0].map = map; answers[0].z = 3; answers[0].x = 4; answers[0].y = 4; answers[0].failed = 0;
	answers[1].map = map; answers[1].z = 3; answers[1].x = 3; answers[1].y = 3; answers[1].failed = 1;
	/* a tile nobody wants, from a thread: dropped */
	answers[2].map = map; answers[2].z = 9; answers[2].x = 1; answers[2].y = 1; answers[2].failed = 0;
	Rtask_run_async(answer_from_thread, &answers[0]);
	Rtask_run_async(answer_from_thread, &answers[1]);
	Rtask_run_async(answer_from_thread, &answers[2]);
	CHECK(settle(map, 3, 4, 4) == R_MAP_TILE_LOADED);
	CHECK(settle(map, 3, 3, 3) == R_MAP_TILE_FAILED);
	Rwait_frame(0.01);
	Rdraw_frame();
	CHECK(Rmap_view_get_tile_state(map, 9, 1, 1) == R_MAP_TILE_NONE);

	/* an answer for a map that is gone, from a thread: harmless */
	answers[0].map = map;
	Rwindow_set_content(g_window, Rwidget_create(R_WIDGET_VBOX));
	Rdraw_frame();
	Rtask_run_async(answer_from_thread, &answers[0]);
	Rwait_frame(0.05);
	Rdraw_frame();
}

static void test_fallback_and_cache(void) {
	Rid map = new_map();
	size_t tile = 8 * 8 * 4;
	int loaded = 0, fallback = 0, empty = 0, old = 0, x, y;
	double lat, lon;

	g_request_count = 0;
	g_auto_answer = 8;
	Rmap_view_set_zoom(map, 3.0);
	tile_middle(4, 8, 8, &lat, &lon);
	Rmap_view_set_center(map, lat, lon);
	Rmap_view_set_tile_callback(map, on_request, &g_request_count);
	Rdraw_frame();
	/* zoom 3: tiles 3..5 seen in each direction, 2..6 wanted */
	CHECK(g_request_count == 25);
	CHECK(Rmap_view_get_tile_state(map, 3, 4, 4) == R_MAP_TILE_LOADED);
#if !defined(RAVEN_TEST_SHARED)
	Rdraw_frame();
	CHECK(rvn_map_view_debug_counts(map, &loaded, &fallback, &empty));
	CHECK(loaded == 9 && fallback == 0 && empty == 0);
#endif

	/* zoom 4, not answered: the tiles of zoom 3 are shown stretched in their place */
	g_auto_answer = 0;
	Rmap_view_set_zoom(map, 4.0);
	Rdraw_frame();
	CHECK(Rmap_view_get_tile_state(map, 4, 8, 8) == R_MAP_TILE_PENDING);
#if !defined(RAVEN_TEST_SHARED)
	CHECK(rvn_map_view_debug_counts(map, &loaded, &fallback, &empty));
	CHECK(loaded == 0 && fallback == 9 && empty == 0);
	/* (a fractional zoom draws the nearest whole one, scaled) */
	Rmap_view_set_zoom(map, 4.3);
	Rdraw_frame();
	CHECK(rvn_map_view_debug_counts(map, &loaded, &fallback, &empty));
	CHECK(loaded == 0 && fallback > 0 && empty == 0);
	Rmap_view_set_zoom(map, 4.0);
	/* nothing at all: the grid */
	Rmap_view_clear_tiles(map);
	Rdraw_frame();
	CHECK(rvn_map_view_debug_counts(map, &loaded, &fallback, &empty));
	CHECK(loaded == 0 && fallback == 0 && empty == 9);
#else
	(void)loaded;
	(void)fallback;
	(void)empty;
#endif

	/* the cache: room for 30 tiles of 8 x 8, 25 wanted at a time */
	Rmap_view_clear_tiles(map);
	Rmap_view_set_cache_size(map, 30 * tile);
	g_auto_answer = 8;
	Rdraw_frame();
	CHECK(Rmap_view_get_cache_usage(map) == 25 * tile);
	CHECK(Rmap_view_get_tile_state(map, 4, 8, 8) == R_MAP_TILE_LOADED);
	Rmap_view_set_center(map, lat, 120.0);
	Rdraw_frame();
	CHECK(Rmap_view_get_cache_usage(map) <= 30 * tile);
	CHECK(Rmap_view_get_cache_usage(map) >= 25 * tile);
	/* the tiles seen now are all there; most of the old ones went */
	CHECK(Rmap_view_get_tile_state(map, 4, 13, 8) == R_MAP_TILE_LOADED);
	for (y = 6; y <= 10; y++)
		for (x = 6; x <= 10; x++) old += Rmap_view_get_tile_state(map, 4, x, y) == R_MAP_TILE_LOADED;
	CHECK(old <= 5);
	/* no room at all: only what the last frame used stays */
	Rmap_view_set_cache_size(map, 0);
	CHECK(Rmap_view_get_cache_usage(map) == 25 * tile);
	Rdraw_frame();
	CHECK(Rmap_view_get_cache_usage(map) == 25 * tile);
	g_auto_answer = 0;
}

static void test_tile_path(void) {
	Rid map = new_map();
	char dir[512], path[700], pattern[700];
	const char* temp = getenv("TMPDIR");
	uint8_t png[8192];
	size_t size;
	FILE* file;
	static const int files[][3] = { { 4, 8, 8 }, { 4, 7, 7 }, { 4, 9, 7 } };
	size_t i;

	if (!temp || !temp[0]) temp = getenv("TEMP");
	if (!temp || !temp[0]) temp = ".";
	snprintf(dir, sizeof(dir), "%s", temp);
	if (dir[strlen(dir) - 1] == '/' || dir[strlen(dir) - 1] == '\\') dir[strlen(dir) - 1] = '\0';
	for (i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
		snprintf(path, sizeof(path), "%s/raven_map_test_%d_%d_%d.png", dir, files[i][0], files[i][1], files[i][2]);
		size = make_png(png, 32, (uint8_t)(i * 80), 160, 90);
		file = fopen(path, "wb");
		CHECK(file != NULL);
		if (!file) return;
		fwrite(png, 1, size, file);
		fclose(file);
	}
	/* a file that is not an image */
	snprintf(path, sizeof(path), "%s/raven_map_test_4_8_7.png", dir);
	file = fopen(path, "wb");
	if (file) {
		fputs("not a png", file);
		fclose(file);
	}

	snprintf(pattern, sizeof(pattern), "%s/raven_map_test_{z}_{x}_{y}.png", dir);
	Rmap_view_set_zoom(map, 4.0);
	Rmap_view_set_center(map, 0.0, 0.0);
	Rmap_view_set_tile_path(map, pattern);
	CHECK_STR(Rmap_view_get_tile_path(map), pattern);
	Rdraw_frame();
#if !defined(RAVEN_TEST_NO_IMAGE_DECODER)
	CHECK(settle(map, 4, 8, 8) == R_MAP_TILE_LOADED);
	CHECK(settle(map, 4, 7, 7) == R_MAP_TILE_LOADED);
	CHECK(Rmap_view_get_cache_usage(map) >= 2 * 32 * 32 * 4);
#endif
	/* missing, or not an image: failed (without an error: a cache has holes) */
	CHECK(settle(map, 4, 9, 9) == R_MAP_TILE_FAILED);
	CHECK(settle(map, 4, 8, 7) == R_MAP_TILE_FAILED);

	/* rows counted from the south (TMS): tile 4/9/8 is the file of row 15 - 8 = 7 */
	snprintf(pattern, sizeof(pattern), "%s/raven_map_test_{z}_{x}_{-y}.png", dir);
	Rmap_view_set_tile_path(map, pattern);
	CHECK(Rmap_view_get_tile_state(map, 4, 8, 8) == R_MAP_TILE_NONE);
	Rdraw_frame();
#if !defined(RAVEN_TEST_NO_IMAGE_DECODER)
	CHECK(settle(map, 4, 9, 8) == R_MAP_TILE_LOADED);
#endif
	CHECK(settle(map, 4, 8, 8) == R_MAP_TILE_FAILED);

	/* local files only; "" goes back to the callback (none here: nothing is asked) */
	EXPECT_ERRORS_BEGIN();
	Rmap_view_set_tile_path(map, "https://tile.example.org/{z}/{x}/{y}.png");
	EXPECT_ERRORS_END(1);
	Rmap_view_set_tile_path(map, "");
	CHECK_STR(Rmap_view_get_tile_path(map), "");
	Rdraw_frame();
	CHECK(Rmap_view_get_tile_state(map, 4, 8, 8) == R_MAP_TILE_NONE);

	for (i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
		snprintf(path, sizeof(path), "%s/raven_map_test_%d_%d_%d.png", dir, files[i][0], files[i][1], files[i][2]);
		remove(path);
	}
	snprintf(path, sizeof(path), "%s/raven_map_test_4_8_7.png", dir);
	remove(path);
}

static void test_input(void) {
	Rid map = new_map();
	double lat, lon, lat2, lon2;
	float x, y, w, h, sx, sy;
	float ax = 100.0f, ay = 120.0f;

	Rwidget_get_rect(map, &x, &y, &w, &h);
	Rmap_view_set_zoom(map, 4.0);
	Rmap_view_set_center(map, 20.0, 10.0);
	Rmap_view_set_view_changed_callback(map, on_view, &g_view_calls);
	Rdraw_frame();

	/* a drag: the place that was under the pointer follows it */
	g_view_calls = 0;
	Rmap_view_screen_to_latlon(map, w * 0.5f, h * 0.5f, &lat, &lon);
	Rwindow_inject_mouse_move(g_window, x + w * 0.5f, y + h * 0.5f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(g_window, x + w * 0.5f + 30.0f, y + h * 0.5f + 10.0f);
	Rwindow_inject_mouse_move(g_window, x + w * 0.5f + 70.0f, y + h * 0.5f + 30.0f);
	Rwindow_inject_mouse_move(g_window, x + w * 0.5f + 100.0f, y + h * 0.5f + 50.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	Rdraw_frame();
	Rmap_view_latlon_to_screen(map, lat, lon, &sx, &sy);
	CHECK_CLOSE(sx, w * 0.5f + 100.0f, 0.01);
	CHECK_CLOSE(sy, h * 0.5f + 50.0f, 0.01);
	CHECK(g_view_calls >= 3);
	CHECK(Rmap_view_get_zoom(map) == 4.0);

	/* the wheel zooms by whole steps around the pointer: the place under it stays */
	Rmap_view_screen_to_latlon(map, ax, ay, &lat, &lon);
	Rwindow_inject_mouse_move(g_window, x + ax, y + ay);
	g_view_calls = 0;
	Rwindow_inject_mouse_wheel(g_window, 0.0f, 1.0f);
	CHECK_CLOSE(Rmap_view_get_zoom(map), 5.0, 1e-9);
	CHECK(g_view_calls == 1);
	CHECK_CLOSE(g_view_zoom, 5.0, 1e-9);
	Rmap_view_latlon_to_screen(map, lat, lon, &sx, &sy);
	CHECK_CLOSE(sx, ax, 0.01);
	CHECK_CLOSE(sy, ay, 0.01);
	Rwindow_inject_mouse_wheel(g_window, 0.0f, -2.0f);
	CHECK_CLOSE(Rmap_view_get_zoom(map), 3.0, 1e-9);
	Rmap_view_latlon_to_screen(map, lat, lon, &sx, &sy);
	CHECK_CLOSE(sx, ax, 0.01);
	CHECK_CLOSE(sy, ay, 0.01);
	/* from a fractional zoom, a notch goes to the next whole one */
	Rmap_view_set_zoom(map, 3.4);
	Rwindow_inject_mouse_wheel(g_window, 0.0f, 1.0f);
	CHECK_CLOSE(Rmap_view_get_zoom(map), 4.0, 1e-9);
	Rwindow_inject_mouse_wheel(g_window, 0.0f, -1.0f);
	CHECK_CLOSE(Rmap_view_get_zoom(map), 3.0, 1e-9);
	Rdraw_frame();

	/* two fingers on a trackpad move the map */
	Rmap_view_get_center(map, &lat, &lon);
	Rwindow_inject_trackpad_scroll(g_window, 1.0f, 0.0f);
	Rwindow_inject_trackpad_scroll_end(g_window);
	Rdraw_frame();
	Rmap_view_get_center(map, &lat2, &lon2);
	CHECK(lon2 < lon);
	CHECK(Rmap_view_get_zoom(map) == 3.0);

	/* a double click zooms in around the pointer */
	Rmap_view_screen_to_latlon(map, ax, ay, &lat, &lon);
	Rwindow_inject_mouse_move(g_window, x + ax, y + ay);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK_CLOSE(Rmap_view_get_zoom(map), 4.0, 1e-9);
	Rmap_view_latlon_to_screen(map, lat, lon, &sx, &sy);
	CHECK_CLOSE(sx, ax, 0.01);
	CHECK_CLOSE(sy, ay, 0.01);
	Rdraw_frame();

	/* the keys, with the focus */
	Rwidget_focus(map);
	test_key(g_window, R_KEY_EQUAL, 0);
	CHECK_CLOSE(Rmap_view_get_zoom(map), 5.0, 1e-9);
	test_key(g_window, R_KEY_MINUS, 0);
	test_key(g_window, R_KEY_MINUS, 0);
	CHECK_CLOSE(Rmap_view_get_zoom(map), 3.0, 1e-9);
	Rmap_view_get_center(map, &lat, &lon);
	test_key(g_window, R_KEY_RIGHT, 0);
	Rmap_view_get_center(map, &lat2, &lon2);
	CHECK_CLOSE(lon2 - lon, 80.0 * 360.0 / (256.0 * 8.0), 1e-6);
	test_key(g_window, R_KEY_UP, 0);
	Rmap_view_get_center(map, &lat, &lon);
	CHECK(lat > lat2);
	/* the zoom stops at the end of the range */
	Rmap_view_set_zoom_range(map, 0.0, 3.0);
	test_key(g_window, R_KEY_EQUAL, 0);
	CHECK(Rmap_view_get_zoom(map) == 3.0);
	Rmap_view_set_zoom_range(map, 0.0, 19.0);

	/* animate_to arrives at once when animations are off */
	g_view_calls = 0;
	Rmap_view_animate_to(map, 51.5074, -0.1278, 9.0);
	Rmap_view_get_center(map, &lat, &lon);
	CHECK_CLOSE(lat, 51.5074, 1e-9);
	CHECK_CLOSE(lon, -0.1278, 1e-9);
	CHECK(Rmap_view_get_zoom(map) == 9.0);
	CHECK(g_view_calls == 1);
	Rdraw_frame();
	Rmap_view_set_view_changed_callback(map, NULL, NULL);
}

/*
 * A zoom, a flight or a glide moves the view as the map is painted: the application is told
 * after the paint (at the next frame), where its callback may change the tree.
 */
static void test_told_outside_paint(void) {
	Rid box = Rwidget_create(R_WIDGET_VBOX);
	Rid before = Rwidget_add_child(box, Rwidget_create(R_WIDGET_CUSTOM));
	Rid map = Rwidget_add_child(box, Rmap_view_create());
	Rid after = Rwidget_add_child(box, Rwidget_create(R_WIDGET_CUSTOM));
	int i;

	Rwidget_set_size(before, 10.0f, 10.0f);
	Rwidget_set_size(after, 10.0f, 10.0f);
	Rslot_set_fill(map, 1.0f);
	Rcustom_set_paint_callback(before, on_paint_before, NULL);
	Rcustom_set_paint_callback(after, on_paint_after, NULL);
	Rwindow_set_content(g_window, box);
	Rmap_view_set_zoom(map, 3.0);
	Rmap_view_set_view_changed_callback(map, on_view, &g_view_calls);
	Rdraw_frame();

	Rset_animations_enabled(R_TRUE);
	Rset_animation_time_step(1.0 / 60.0);
	g_view_calls = g_view_calls_while_painting = 0;
	g_painting = R_FALSE;
	Rmap_view_animate_to(map, 48.8566, 2.3522, 6.0);
	for (i = 0; i < 240; i++) {
		Rwindow_request_redraw(g_window);
		Rdraw_frame();
	}
	CHECK(Rmap_view_get_zoom(map) == 6.0);
	CHECK(g_view_calls >= 1);
	CHECK_CLOSE(g_view_zoom, 6.0, 1e-9);
	CHECK(g_view_calls_while_painting == 0);
	Rset_animation_time_step(0.0);
	Rset_animations_enabled(R_FALSE);

	/* a map destroyed with a telling still to come: nothing is told */
	g_view_calls = 0;
	Rmap_view_animate_to(map, 40.0, 10.0, 4.0);
	Rset_animations_enabled(R_TRUE);
	Rset_animation_time_step(1.0 / 60.0);
	Rmap_view_animate_to(map, 10.0, 20.0, 5.0);
	Rwindow_request_redraw(g_window);
	Rdraw_frame();
	g_view_calls = 0;
	Rwidget_destroy(map);
	Rdraw_frame();
	Rdraw_frame();
	CHECK(g_view_calls == 0);
	Rset_animation_time_step(0.0);
	Rset_animations_enabled(R_FALSE);
}

static void test_markers(void) {
	Rid map = new_map();
	float x, y, w, h;
	int paris, rome, tokyo;
	double zero = 0.0, not_a_number = zero / zero;

	Rwidget_get_rect(map, &x, &y, &w, &h);
	Rmap_view_set_zoom(map, 5.0);
	Rmap_view_set_center(map, 48.8566, 2.3522);
	paris = Rmap_view_add_marker(map, 48.8566, 2.3522, "Paris");
	rome = Rmap_view_add_marker(map, 41.9028, 12.4964, "Rome");
	tokyo = Rmap_view_add_marker(map, 35.6762, 139.6503, NULL);
	CHECK(paris == 1 && rome == 2 && tokyo == 3);
	CHECK(Rmap_view_get_marker_count(map) == 3);
	Rmap_view_set_marker_color(map, rome, 0.2f, 0.6f, 0.3f, 1.0f);
	Rmap_view_set_marker_label(map, tokyo, "Tokyo");
	Rmap_view_set_marker_clicked_callback(map, on_marker, &g_marker_clicks);
	Rdraw_frame();

	/* a click on the head of the pin of Paris (20 units over its point, at the middle) */
	g_marker_clicks = 0;
	test_click_at(g_window, x + w * 0.5f, y + h * 0.5f - 20.0f);
	CHECK(g_marker_clicks == 1);
	CHECK(g_last_marker == paris);
	/* and on its label */
	test_click_at(g_window, x + w * 0.5f + 20.0f, y + h * 0.5f - 20.0f);
	CHECK(g_marker_clicks == 2);
	/* beside it: nothing */
	test_click_at(g_window, x + w * 0.5f - 60.0f, y + h * 0.5f + 60.0f);
	CHECK(g_marker_clicks == 2);
	/* a drag that starts on a marker moves the map and clicks nothing */
	Rwindow_inject_mouse_move(g_window, x + w * 0.5f, y + h * 0.5f - 20.0f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_TRUE);
	Rwindow_inject_mouse_move(g_window, x + w * 0.5f + 40.0f, y + h * 0.5f);
	Rwindow_inject_mouse_button(g_window, R_MOUSE_LEFT, R_FALSE);
	CHECK(g_marker_clicks == 2);

	/* moved, removed */
	Rmap_view_set_center(map, 48.8566, 2.3522);
	Rmap_view_set_marker_position(map, paris, 0.0, 0.0);
	/* (not where the last press was: that would be a double click) */
	test_click_at(g_window, x + w * 0.5f + 6.0f, y + h * 0.5f - 12.0f);
	CHECK(g_marker_clicks == 2);
	Rmap_view_remove_marker(map, rome);
	CHECK(Rmap_view_get_marker_count(map) == 2);
	EXPECT_ERRORS_BEGIN();
	Rmap_view_remove_marker(map, rome);
	Rmap_view_set_marker_color(map, 99, 1, 1, 1, 1);
	Rmap_view_set_marker_position(map, paris, not_a_number, 0.0);
	EXPECT_ERRORS_END(3);
	CHECK(Rmap_view_add_marker(map, 1.0, 2.0, "x") == 4);
	Rdraw_frame();

#if !defined(RAVEN_TEST_SHARED)
	{
		/* a screen reader sees the map, its place, and the markers as buttons */
		rvn_a11y_node_t node;
		int clicks = g_marker_clicks;
		CHECK(rvn_a11y_describe(map, &node));
		CHECK(node.role == R_ROLE_GROUP);
		CHECK_STR(node.name, "Map");
		CHECK(strstr(node.value, "zoom 5.0") != NULL);
		CHECK(rvn_a11y_child_count(map) == 3);
		CHECK(rvn_a11y_describe(rvn_a11y_child(map, 1), &node));
		CHECK(node.role == R_ROLE_BUTTON);
		CHECK_STR(node.name, "Tokyo");
		CHECK(rvn_a11y_act(rvn_a11y_child(map, 0), RVN_A11Y_PRESS));
		CHECK(g_marker_clicks == clicks + 1 && g_last_marker == paris);
	}
#endif

	Rmap_view_clear_markers(map);
	CHECK(Rmap_view_get_marker_count(map) == 0);
	Rdraw_frame();
}

static void test_xml(void) {
	static const char* xml =
		"<MapView Name='map' Latitude='48.85' Longitude='2.35' Zoom='5' MinZoom='2' MaxZoom='12' TileSize='512'"
		" Attribution='\xC2\xA9 OpenStreetMap contributors' TilePath='tiles/{z}/{x}/{y}.png'>"
		"<Marker Latitude='48.85' Longitude='2.35' Label='Paris' Color='#e74c3c'/>"
		"<Marker Latitude='-33.87' Longitude='151.21'/>"
		"</MapView>";
	Rid map = Rwidget_create_from_xml(xml, strlen(xml));
	double lat, lon, min_zoom, max_zoom;
	CHECK(map != R_INVALID_ID);
	if (map == R_INVALID_ID) return;
	Rwindow_set_content(g_window, map);
	Rdraw_frame();
	Rmap_view_get_center(map, &lat, &lon);
	CHECK_CLOSE(lat, 48.85, 1e-5);
	CHECK_CLOSE(lon, 2.35, 1e-5);
	CHECK(Rmap_view_get_zoom(map) == 5.0);
	Rmap_view_get_zoom_range(map, &min_zoom, &max_zoom);
	CHECK(min_zoom == 2.0 && max_zoom == 12.0);
	CHECK(Rmap_view_get_tile_size(map) == 512);
	CHECK_STR(Rmap_view_get_attribution(map), "\xC2\xA9 OpenStreetMap contributors");
	CHECK(strstr(Rmap_view_get_tile_path(map), "tiles/{z}/{x}/{y}.png") != NULL);
	CHECK(Rmap_view_get_marker_count(map) == 2);
	/* (the files are not there: the tiles fail, quietly) */
	Rdraw_frame();

	/* a place and a zoom in double precision: a float would make 120.000001 120 (90 units at zoom 24) */
	Rmap_view_set_zoom_range(map, 0.0, 24.0);
	CHECK(Rwidget_set_attribute(map, "Longitude", "120.000001"));
	CHECK(Rwidget_set_attribute(map, "Latitude", "45.0000013"));
	CHECK(Rwidget_set_attribute(map, "Zoom", "23.0000001"));
	Rmap_view_get_center(map, &lat, &lon);
	CHECK_CLOSE(lon, 120.000001, 1e-9);
	CHECK_CLOSE(lat, 45.0000013, 1e-9);
	CHECK_CLOSE(Rmap_view_get_zoom(map), 23.0000001, 1e-12);
	Rdraw_frame();

	/* the other names of the tag */
	map = Rwidget_create_from_xml("<Map/>", 6);
	CHECK(Rwidget_get_type(map) == R_WIDGET_MAP_VIEW);
	Rwidget_destroy(map);
	map = Rwidget_create_from_xml("<TileMap/>", 10);
	CHECK(Rwidget_get_type(map) == R_WIDGET_MAP_VIEW);
	Rwidget_destroy(map);

	/* wrong values */
	EXPECT_ERRORS_BEGIN();
	map = Rwidget_create_from_xml("<MapView Zoom='far'/>", 21);
	if (map != R_INVALID_ID) Rwidget_destroy(map);
	map = Rwidget_create_from_xml("<MapView><Marker Label='nowhere'/></MapView>", 44);
	if (map != R_INVALID_ID) Rwidget_destroy(map);
	EXPECT_ERRORS_END(2);
}

static void test_errors(void) {
	Rid map = new_map();
	Rid label = Rwidget_create(R_WIDGET_LABEL);
	uint8_t pixels[16];
	double zero = 0.0, not_a_number = zero / zero;
	memset(pixels, 0, sizeof(pixels));

	EXPECT_ERRORS_BEGIN();
	Rmap_view_set_center(map, not_a_number, 0.0);
	Rmap_view_set_zoom(map, not_a_number);
	Rmap_view_set_zoom_range(map, 5.0, 2.0);
	Rmap_view_set_zoom_range(map, 0.0, 30.0);
	Rmap_view_set_tile_size(map, 8);
	Rmap_view_set_tile(map, -1, 0, 0, pixels, 2, 2);
	Rmap_view_set_tile(map, 2, 4, 0, pixels, 2, 2);
	Rmap_view_set_tile(map, 2, 0, 0, NULL, 2, 2);
	Rmap_view_set_tile(map, 2, 0, 0, pixels, 0, 2);
	Rmap_view_set_tile_data(map, 2, 0, 0, "not an image", 12);
	Rmap_view_set_tile_data(map, 2, 0, 0, NULL, 0);
	Rmap_view_set_tile_failed(map, 1, 2, 0);
	CHECK(Rmap_view_add_marker(map, 0.0, not_a_number, "x") == -1);
	Rmap_view_animate_to(map, 0.0, 0.0, not_a_number);
	CHECK(Rmap_view_get_zoom(label) == 0.0);
	Rmap_view_set_center(label, 0.0, 0.0);
	EXPECT_ERRORS_END(16);
	/* nothing changed */
	CHECK(Rmap_view_get_zoom(map) == 1.0);
	CHECK(Rmap_view_get_tile_size(map) == 256);
	CHECK(Rmap_view_get_tile_state(map, 2, 0, 0) == R_MAP_TILE_NONE);
	CHECK(Rmap_view_get_tile_state(map, 99, 0, 0) == R_MAP_TILE_NONE);
	Rwidget_destroy(label);
	Rdraw_frame();
}

int main(void) {
	if (!test_init(2)) return 1;
	g_window = Rwindow_create("MapView", MAP_W, MAP_H, R_WINDOW_DEFAULT);
	RUN(test_projection);
	RUN(test_requests);
	RUN(test_async);
	RUN(test_fallback_and_cache);
	RUN(test_tile_path);
	RUN(test_input);
	RUN(test_told_outside_paint);
	RUN(test_markers);
	RUN(test_xml);
	RUN(test_errors);
	Rwindow_destroy(g_window);
	Rshutdown();
	return test_report("test_map_view");
}
