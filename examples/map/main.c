/**
 * Raven example - map
 * A MapView that works offline: its tiles are drawn by the example itself,
 * on worker threads, when the map asks for them: the sea and the land of a
 * coarse outline of the world (world.h) roughened by noise, a graticule, and
 * the z/x/y of each tile written in its corner, so that the tiling shows.
 * Markers for a few cities; a click on one flies there.
 *
 *   ./raven_map
 *
 * RAVEN_MAP_TILE_DIR=/path/to/tiles reads real tiles from files instead
 * ({z}/{x}/{y}.png under that folder, as a tile cache or an extracted MBTiles
 * keeps them; RAVEN_MAP_ATTRIBUTION gives their credit). Nothing is ever
 * fetched from the network.
 *
 * RAVEN_MAP_LAT, RAVEN_MAP_LON and RAVEN_MAP_ZOOM give the first view;
 * RAVEN_MAP_DEMO=1 plays a short tour (a drag, the wheel, a flight).
 */

#include "../common/example.h"
#include "world.h"

#include <math.h>

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

#define PI 3.14159265358979323846
#define TILE 256

/* The land, rasterized once from world.h: a quarter of a degree per cell, and a blurred copy for the shallows. */
#define MASK_W 1440
#define MASK_H 720
static unsigned char g_mask[MASK_H][MASK_W];
static float g_near[MASK_H][MASK_W];

static struct {
	Rid window, map, place, status;
	int asked, cancelled, drawn;
	int demo, frame;
} app;

/* ---------------------------------------------------------------------------
 *  The world
 * ------------------------------------------------------------------------- */

static int compare_floats(const void* a, const void* b) {
	float fa = *(const float*)a, fb = *(const float*)b;
	return fa < fb ? -1 : fa > fb ? 1 : 0;
}

/* Fills the rings of one shape (even-odd) into the mask, row by row. */
static void fill_shape(const float* points, int count) {
	static float crossings[512];
	int row;
	for (row = 0; row < MASK_H; row++) {
		float lat = 90.0f - ((float)row + 0.5f) * 180.0f / MASK_H;
		int n = 0, i, start = 0;
		for (i = 0; i < count; i++) {
			const float* a;
			const float* b;
			int next;
			if (points[i * 2] == WORLD_END) {
				start = i + 1;
				continue;
			}
			next = (i + 1 < count && points[(i + 1) * 2] != WORLD_END) ? i + 1 : start;
			a = &points[i * 2];
			b = &points[next * 2];
			if ((a[1] <= lat) != (b[1] <= lat) && n < 512)
				crossings[n++] = a[0] + (lat - a[1]) / (b[1] - a[1]) * (b[0] - a[0]);
		}
		qsort(crossings, (size_t)n, sizeof(float), compare_floats);
		for (i = 0; i + 1 < n; i += 2) {
			int x0 = (int)ceil((crossings[i] + 180.0f) * MASK_W / 360.0f - 0.5f);
			int x1 = (int)floor((crossings[i + 1] + 180.0f) * MASK_W / 360.0f - 0.5f);
			int x;
			if (x0 < 0) x0 = 0;
			if (x1 > MASK_W - 1) x1 = MASK_W - 1;
			for (x = x0; x <= x1; x++) g_mask[row][x] = 1;
		}
	}
}

static void build_world(void) {
	static float scratch[MASK_W > MASK_H ? MASK_W : MASK_H];
	size_t pairs = sizeof(g_world) / sizeof(g_world[0]) / 2, i, start = 0;
	int x, y, pass, k;
	/* the pairs up to a WORLD_SHAPE are one shape */
	for (i = 0; i < pairs; i++) {
		if (g_world[i * 2] == WORLD_SHAPE_END) {
			fill_shape(&g_world[start * 2], (int)(i - start));
			start = i + 1;
		}
	}
	/* how near the land is: the mask blurred (three box passes of 9 cells, across the antimeridian) */
	for (y = 0; y < MASK_H; y++)
		for (x = 0; x < MASK_W; x++) g_near[y][x] = g_mask[y][x];
	for (pass = 0; pass < 3; pass++) {
		for (y = 0; y < MASK_H; y++) {
			for (x = 0; x < MASK_W; x++) {
				float sum = 0.0f;
				for (k = -4; k <= 4; k++) sum += g_near[y][(x + k + MASK_W) % MASK_W];
				scratch[x] = sum / 9.0f;
			}
			for (x = 0; x < MASK_W; x++) g_near[y][x] = scratch[x];
		}
		for (x = 0; x < MASK_W; x++) {
			for (y = 0; y < MASK_H; y++) {
				float sum = 0.0f;
				for (k = -4; k <= 4; k++) {
					int yy = y + k < 0 ? 0 : y + k >= MASK_H ? MASK_H - 1 : y + k;
					sum += g_near[yy][x];
				}
				scratch[y] = sum / 9.0f;
			}
			for (y = 0; y < MASK_H; y++) g_near[y][x] = scratch[y];
		}
	}
}

/* Bilinear sample of a grid in degrees (longitude wraps). */
static float sample_mask(double lon, double lat) {
	double fx = (lon + 180.0) * MASK_W / 360.0 - 0.5, fy = (90.0 - lat) * MASK_H / 180.0 - 0.5;
	int x0 = (int)floor(fx), y0 = (int)floor(fy);
	float tx = (float)(fx - x0), ty = (float)(fy - y0);
	int x1 = ((x0 + 1) % MASK_W + MASK_W) % MASK_W, y1 = y0 + 1;
	x0 = (x0 % MASK_W + MASK_W) % MASK_W;
	if (y0 < 0) y0 = 0;
	if (y1 < 0) y1 = 0;
	if (y0 >= MASK_H) y0 = MASK_H - 1;
	if (y1 >= MASK_H) y1 = MASK_H - 1;
	return (g_mask[y0][x0] * (1 - tx) + g_mask[y0][x1] * tx) * (1 - ty) + (g_mask[y1][x0] * (1 - tx) + g_mask[y1][x1] * tx) * ty;
}

static float sample_near(double lon, double lat) {
	double fx = (lon + 180.0) * MASK_W / 360.0 - 0.5, fy = (90.0 - lat) * MASK_H / 180.0 - 0.5;
	int x0 = (int)floor(fx), y0 = (int)floor(fy);
	float tx = (float)(fx - x0), ty = (float)(fy - y0);
	int x1 = ((x0 + 1) % MASK_W + MASK_W) % MASK_W, y1 = y0 + 1;
	x0 = (x0 % MASK_W + MASK_W) % MASK_W;
	if (y0 < 0) y0 = 0;
	if (y1 < 0) y1 = 0;
	if (y0 >= MASK_H) y0 = MASK_H - 1;
	if (y1 >= MASK_H) y1 = MASK_H - 1;
	return (g_near[y0][x0] * (1 - tx) + g_near[y0][x1] * tx) * (1 - ty) + (g_near[y1][x0] * (1 - tx) + g_near[y1][x1] * tx) * ty;
}

/* Value noise: smooth, in -1..1, from a hash of the lattice. */
static float hash2(int x, int y) {
	uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u;
	h = (h ^ (h >> 13)) * 1274126177u;
	return (float)((h ^ (h >> 16)) & 0xFFFF) / 32767.5f - 1.0f;
}

static float noise(double x, double y) {
	int ix = (int)floor(x), iy = (int)floor(y);
	float fx = (float)(x - ix), fy = (float)(y - iy);
	float sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
	float a = hash2(ix, iy), b = hash2(ix + 1, iy), c = hash2(ix, iy + 1), d = hash2(ix + 1, iy + 1);
	return (a + (b - a) * sx) * (1 - sy) + (c + (d - c) * sx) * sy;
}

/* Octaves of noise, from `frequency` cycles per degree up to what a pixel of `pixel` degrees can show. */
static float fbm(double lon, double lat, double frequency, double pixel) {
	float sum = 0.0f, amplitude = 0.5f;
	int octave;
	for (octave = 0; octave < 12 && frequency * pixel < 0.2; octave++) {
		sum += amplitude * noise(lon * frequency + octave * 17.3, lat * frequency - octave * 9.1);
		frequency *= 2.0;
		amplitude *= 0.5f;
	}
	return sum;
}

static float smooth(float edge0, float edge1, float x) {
	float t = (x - edge0) / (edge1 - edge0);
	t = t < 0 ? 0 : t > 1 ? 1 : t;
	return t * t * (3 - 2 * t);
}

static float mix(float a, float b, float t) {
	return a + (b - a) * t;
}

/* ---------------------------------------------------------------------------
 *  Tiles
 * ------------------------------------------------------------------------- */

/* Digits and '/' of 3 x 5 cells, a row per 3 bits. */
static const unsigned short g_glyphs[11] = {
	075557, 026227, 071747, 071717, 055711, 074717, 074757, 071111, 075757, 075717, 011244
};

static void put_text(uint8_t* pixels, int x, int y, const char* text, int scale) {
	int pass;
	/* a dark halo, then the white letters */
	for (pass = 0; pass < 2; pass++) {
		int cx = x;
		const char* c;
		for (c = text; *c; c++) {
			int glyph = *c == '/' ? 10 : *c - '0', row, col, dx, dy, o;
			if (glyph < 0 || glyph > 10) continue;
			for (row = 0; row < 5; row++) {
				for (col = 0; col < 3; col++) {
					if (!((g_glyphs[glyph] >> ((4 - row) * 3 + (2 - col))) & 1)) continue;
					for (o = 0; o < (pass == 0 ? 9 : 1); o++) {
						int ox = pass == 0 ? o % 3 - 1 : 0, oy = pass == 0 ? o / 3 - 1 : 0;
						for (dy = 0; dy < scale; dy++) {
							for (dx = 0; dx < scale; dx++) {
								int px = cx + col * scale + dx + ox, py = y + row * scale + dy + oy;
								uint8_t* p;
								if (px < 0 || py < 0 || px >= TILE || py >= TILE) continue;
								p = pixels + (py * TILE + px) * 4;
								if (pass == 0) {
									p[0] = (uint8_t)(p[0] * 0.45f);
									p[1] = (uint8_t)(p[1] * 0.45f);
									p[2] = (uint8_t)(p[2] * 0.45f);
								} else {
									p[0] = p[1] = p[2] = 255;
								}
							}
						}
					}
				}
			}
			cx += 4 * scale;
		}
	}
}

/* Degrees between two lines of the graticule at a zoom. */
static double graticule_step(int z) {
	if (z <= 2) return 30.0;
	if (z <= 4) return 10.0;
	if (z <= 6) return 5.0;
	if (z <= 8) return 1.0;
	if (z <= 11) return 0.25;
	return 0.05;
}

static void draw_tile(uint8_t* pixels, int z, int tx, int ty) {
	double n = (double)(1 << z);
	double pixel = 360.0 / (n * TILE);
	double step = graticule_step(z);
	/* the noise that roughens the coast: features smaller than the mask, down to the pixel */
	double rough = 0.35;
	int px, py;
	char label[48];

	for (py = 0; py < TILE; py++) {
		double my = (ty + (py + 0.5) / TILE) / n;
		double lat = atan(sinh(PI * (1.0 - 2.0 * my))) * 180.0 / PI;
		double pixel_lat = pixel * cos(lat * PI / 180.0);
		double grid_lat = fabs(lat / step - floor(lat / step + 0.5)) * step / pixel_lat;
		float alat = (float)fabs(lat);
		for (px = 0; px < TILE; px++) {
			double lon = (tx + (px + 0.5) / TILE) / n * 360.0 - 180.0;
			uint8_t* p = pixels + (py * TILE + px) * 4;
			float dx = fbm(lon, lat, 0.6, pixel) * (float)rough, dy = fbm(lon + 40.0, lat - 25.0, 0.6, pixel) * (float)rough;
			float land = sample_mask(lon + dx, lat + dy) + fbm(lon, lat, 2.0, pixel) * 0.25f;
			float edge = (float)(pixel / 0.25) * 0.6f;
			float a = smooth(0.5f - edge - 0.01f, 0.5f + edge + 0.01f, land);
			float nearby = sample_near(lon, lat);
			float r, g, b, lr, lg, lb, shade, dry, ice, grid_lon;
			/* the sea: deep, lighter over the shelves near the coast */
			float shallow = smooth(0.02f, 0.45f, nearby);
			r = mix(22, 70, shallow);
			g = mix(64, 140, shallow);
			b = mix(118, 186, shallow);
			/* the land: forests at the equator, deserts in the horse latitudes, tundra, then ice */
			dry = smooth(0.15f, 0.55f, fbm(lon * 0.5, lat * 0.5, 0.08, pixel) + 0.5f) *
				(1.0f - smooth(8, 16, fabsf(alat - 24.0f)));
			lr = mix(74, 104, smooth(10, 35, alat));
			lg = mix(132, 150, smooth(10, 35, alat));
			lb = mix(66, 82, smooth(10, 35, alat));
			lr = mix(lr, 222, dry);
			lg = mix(lg, 196, dry);
			lb = mix(lb, 140, dry);
			lr = mix(lr, 150, smooth(52, 64, alat));
			lg = mix(lg, 156, smooth(52, 64, alat));
			lb = mix(lb, 132, smooth(52, 64, alat));
			ice = lat < 0 ? smooth(-60, -64, (float)lat) : smooth(70, 76, alat);
			lr = mix(lr, 238, ice);
			lg = mix(lg, 242, ice);
			lb = mix(lb, 247, ice);
			shade = 0.9f + 0.22f * fbm(lon * 3.0, lat * 3.0, 0.5, pixel);
			r = mix(r, lr * shade, a);
			g = mix(g, lg * shade, a);
			b = mix(b, lb * shade, a);
			/* the graticule: the equator and the prime meridian stronger */
			grid_lon = (float)(fabs(lon / step - floor(lon / step + 0.5)) * step / pixel);
			if (grid_lon < 0.8f || grid_lat < 0.8) {
				int main_line = (grid_lon < 0.8f && fabs(lon) < step * 0.5) || (grid_lat < 0.8 && fabs(lat) < step * 0.5);
				float t = main_line ? 0.55f : 0.28f;
				r = mix(r, 255, t);
				g = mix(g, 255, t);
				b = mix(b, 255, t);
			}
			/* the edge of the tile */
			if (px == 0 || py == 0) {
				r *= 0.8f;
				g *= 0.8f;
				b *= 0.8f;
			}
			p[0] = (uint8_t)(r > 255 ? 255 : r);
			p[1] = (uint8_t)(g > 255 ? 255 : g);
			p[2] = (uint8_t)(b > 255 ? 255 : b);
			p[3] = 255;
		}
	}
	snprintf(label, sizeof(label), "%d/%d/%d", z, tx, ty);
	put_text(pixels, 6, 6, label, 2);
}

typedef struct tile_job_t {
	Rid map;
	int z, x, y;
} tile_job_t;

/* A worker thread draws the tile and gives it to the map (thread-safe: it is applied on the UI thread). */
static void make_tile(void* user_data) {
	tile_job_t* job = (tile_job_t*)user_data;
	uint8_t* pixels = (uint8_t*)malloc(TILE * TILE * 4);
	if (pixels) {
		draw_tile(pixels, job->z, job->x, job->y);
		Rmap_view_set_tile(job->map, job->z, job->x, job->y, pixels, TILE, TILE);
		free(pixels);
	}
	free(job);
}

/* The map needs a tile: it is drawn away from the UI thread. */
static void on_tile(Rid map, int z, int x, int y, void* user_data) {
	tile_job_t* job = (tile_job_t*)malloc(sizeof(tile_job_t));
	(void)user_data;
	app.asked++;
	if (!job) {
		Rmap_view_set_tile_failed(map, z, x, y);
		return;
	}
	job->map = map;
	job->z = z;
	job->x = x;
	job->y = y;
	Rtask_run_async(make_tile, job);
}

/* The user went elsewhere before the tile came: the map will drop it. (A loader would stop the download here.) */
static void on_tile_cancelled(Rid map, int z, int x, int y, void* user_data) {
	(void)map;
	(void)z;
	(void)x;
	(void)y;
	(void)user_data;
	app.cancelled++;
}

/* ---------------------------------------------------------------------------
 *  The window
 * ------------------------------------------------------------------------- */

static void show_place(double lat, double lon, double zoom) {
	char text[128];
	snprintf(text, sizeof(text), "%.4f\xC2\xB0 %c   %.4f\xC2\xB0 %c   zoom %.2f", fabs(lat), lat < 0 ? 'S' : 'N',
		fabs(lon), lon < 0 ? 'W' : 'E', zoom);
	Rlabel_set_text(app.place, text);
}

static void on_view(Rid map, double lat, double lon, double zoom, void* user_data) {
	(void)map;
	(void)user_data;
	show_place(lat, lon, zoom);
}

static void on_marker(Rid map, int marker, void* user_data) {
	/* the markers of map.xml, in their order: where they are */
	static const double places[][2] = {
		{ 51.5074, -0.1278 }, { 48.8566, 2.3522 }, { 40.7128, -74.0060 }, { 34.0522, -118.2437 },
		{ 19.4326, -99.1332 }, { -22.9068, -43.1729 }, { -34.6037, -58.3816 }, { 30.0444, 31.2357 },
		{ 6.5244, 3.3792 }, { -33.9249, 18.4241 }, { 55.7558, 37.6173 }, { 19.0760, 72.8777 },
		{ 39.9042, 116.4074 }, { 35.6762, 139.6503 }, { 1.3521, 103.8198 }, { -33.8688, 151.2093 }
	};
	(void)user_data;
	if (marker >= 1 && marker <= (int)(sizeof(places) / sizeof(places[0]))) {
		double zoom = Rmap_view_get_zoom(map);
		Rmap_view_animate_to(map, places[marker - 1][0], places[marker - 1][1], zoom < 6.0 ? 6.0 : zoom);
	}
}

static void on_action(Rid sender, const char* action, void* user_data) {
	double lat, lon;
	(void)sender;
	(void)user_data;
	Rmap_view_get_center(app.map, &lat, &lon);
	if (strcmp(action, "map.world") == 0) Rmap_view_animate_to(app.map, 25.0, 10.0, 2.0);
	else if (strcmp(action, "map.europe") == 0) Rmap_view_animate_to(app.map, 50.0, 10.0, 4.0);
	else if (strcmp(action, "map.americas") == 0) Rmap_view_animate_to(app.map, 10.0, -80.0, 3.0);
	else if (strcmp(action, "map.asia") == 0) Rmap_view_animate_to(app.map, 25.0, 110.0, 3.0);
	else if (strcmp(action, "map.zoom_in") == 0) Rmap_view_animate_to(app.map, lat, lon, floor(Rmap_view_get_zoom(app.map)) + 1.0);
	else if (strcmp(action, "map.zoom_out") == 0) Rmap_view_animate_to(app.map, lat, lon, ceil(Rmap_view_get_zoom(app.map)) - 1.0);
}

static void update_status(Rid timer, void* user_data) {
	char text[160];
	(void)timer;
	(void)user_data;
	snprintf(text, sizeof(text), "tiles asked %d   cancelled %d   in memory %.1f MB", app.asked, app.cancelled,
		(double)Rmap_view_get_cache_usage(app.map) / (1024.0 * 1024.0));
	Rlabel_set_text(app.status, text);
}

/* RAVEN_MAP_DEMO: a short tour, by frames (a film made with RAVEN_EXAMPLE_FILM advances 1/60 s per frame). */
static void demo_frame(double delta, void* user_data) {
	float x, y, w, h;
	int f = app.frame++;
	(void)delta;
	(void)user_data;
	if (!app.demo || !Rwindow_is_valid(app.window)) return;
	Rwidget_get_rect(app.map, &x, &y, &w, &h);
	if (f == 40) {
		Rmap_view_animate_to(app.map, 48.0, 6.0, 4.0);
	} else if (f >= 130 && f <= 160) {
		/* a hand drags the map to the west, and lets it glide */
		float t = (float)(f - 130);
		if (f == 130) {
			Rwindow_inject_mouse_move(app.window, x + w * 0.6f, y + h * 0.5f);
			Rwindow_inject_mouse_button(app.window, R_MOUSE_LEFT, R_TRUE);
		}
		Rwindow_inject_mouse_move(app.window, x + w * 0.6f - t * 9.0f, y + h * 0.5f + t * 2.0f);
		if (f == 160) Rwindow_inject_mouse_button(app.window, R_MOUSE_LEFT, R_FALSE);
	} else if (f == 220 || f == 270) {
		/* the wheel, over Paris */
		float px = 0.0f, py = 0.0f;
		Rmap_view_latlon_to_screen(app.map, 48.8566, 2.3522, &px, &py);
		Rwindow_inject_mouse_move(app.window, x + px, y + py - 20.0f);
		Rwindow_inject_mouse_wheel(app.window, 0.0f, 1.0f);
	} else if (f == 340) {
		Rmap_view_animate_to(app.map, 41.0, -40.0, 3.0);
	} else if (f == 420) {
		Rmap_view_animate_to(app.map, 40.7128, -74.0060, 5.0);
	}
}

static void shutdown(void) {
	Rshutdown();
}

int main(void) {
	const char* tile_dir = getenv("RAVEN_MAP_TILE_DIR");
	const char* value;
	double lat, lon;
	if (!example_init("Raven map")) return 1;
	build_world();
	app.window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/map.xml");
	if (app.window == R_INVALID_ID) {
		Rshutdown();
		return 1;
	}
	app.map = Rwidget_find(app.window, "map");
	app.place = Rwidget_find(app.window, "place");
	app.status = Rwidget_find(app.window, "status");

	if (tile_dir && tile_dir[0]) {
		/* real tiles from files: a tile cache, an extracted MBTiles (use {-y} for TMS rows) */
		char pattern[1024];
		const char* credit = getenv("RAVEN_MAP_ATTRIBUTION");
		snprintf(pattern, sizeof(pattern), "%s/{z}/{x}/{y}.png", tile_dir);
		Rmap_view_set_tile_path(app.map, pattern);
		Rmap_view_set_attribution(app.map, credit ? credit : "\xC2\xA9 OpenStreetMap contributors");
	} else {
		Rmap_view_set_tile_callback(app.map, on_tile, NULL);
		Rmap_view_set_tile_cancel_callback(app.map, on_tile_cancelled, NULL);
	}
	Rmap_view_set_view_changed_callback(app.map, on_view, NULL);
	Rmap_view_set_marker_clicked_callback(app.map, on_marker, NULL);

	Rmap_view_get_center(app.map, &lat, &lon);
	if ((value = getenv("RAVEN_MAP_LAT"))) lat = atof(value);
	if ((value = getenv("RAVEN_MAP_LON"))) lon = atof(value);
	Rmap_view_set_center(app.map, lat, lon);
	if ((value = getenv("RAVEN_MAP_ZOOM"))) Rmap_view_set_zoom(app.map, atof(value));
	show_place(lat, lon, Rmap_view_get_zoom(app.map));
	app.demo = getenv("RAVEN_MAP_DEMO") != NULL;
	Rset_frame_callback(demo_frame, NULL);

	Raction_bind("map.world", on_action, NULL);
	Raction_bind("map.europe", on_action, NULL);
	Raction_bind("map.americas", on_action, NULL);
	Raction_bind("map.asia", on_action, NULL);
	Raction_bind("map.zoom_in", on_action, NULL);
	Raction_bind("map.zoom_out", on_action, NULL);
	Rtimer_create(0.25, R_TRUE, update_status, NULL);
	update_status(R_INVALID_ID, NULL);

	example_run(app.window, NULL, shutdown);
	return 0;
}
