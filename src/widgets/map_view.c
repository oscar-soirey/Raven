/**
 * Raven - widgets/map_view.c
 * MapView: a map of slippy-map tiles (Web Mercator, z / x / y), panned and
 * zoomed, with markers and an attribution line.
 *
 * Raven fetches nothing from the network: the tiles come from the
 * application (the tile callback asks, Rmap_view_set_tile* answers, from any
 * thread) or from local files named by a template (read on a worker thread).
 *
 * The view keeps a table of tiles (z, x, y) -> state: asked for (pending),
 * loaded (a texture), or failed. Each paint works out the tiles seen at the
 * whole zoom nearest the current one, plus a ring of one tile around them:
 * those are "wanted". A wanted tile not in the table is asked for once; a
 * pending tile that is no longer wanted is dropped (and the application told),
 * so an answer that comes later finds nothing and is ignored. A missing tile
 * is drawn from the nearest coarser tile that is loaded (a part of it,
 * stretched), with the finer ones that are loaded over it, else a faint grid.
 *
 * The loaded tiles are a cache bounded in bytes: the least recently drawn are
 * dropped first, never those drawn or wanted in the last frame.
 *
 * The center is kept in normalized Mercator coordinates (0..1 across the
 * world, y going south), in double precision: at zoom 24 the world is 2^32
 * logical units wide.
 *
 * The paint calls the application (the tile callback): RVN_CF_UI_PAINT.
 */

#include "common.h"
#include "../core/jobs.h"
#include "../xml/xml.h"

#include <limits.h>
#include <stdlib.h>

#define RVN_MAP_PI 3.14159265358979323846
/** The latitude where the Mercator square ends: atan(sinh(pi)). */
#define RVN_MAP_MAX_LAT 85.05112877980659
#define RVN_MAP_LIMIT_ZOOM 24.0
#define RVN_MAP_BUCKETS 1024
/** Entries of the table (failed tiles cost no pixels but are bounded too). */
#define RVN_MAP_MAX_ENTRIES 4096
/** How many coarser zooms are searched for a tile to show in place of a missing one. */
#define RVN_MAP_FALLBACK_LEVELS 10
#define RVN_MAP_DEFAULT_CACHE ((size_t)64 * 1024 * 1024)
/** A tile larger than this is refused. */
#define RVN_MAP_MAX_TILE_PIXELS 4096

#define RVN_MAP_ANIM_NONE 0
/** A zoom around a point of the widget (wheel, double click, keys). */
#define RVN_MAP_ANIM_ZOOM 1
/** To a center and a zoom (Rmap_view_animate_to). */
#define RVN_MAP_ANIM_FLY 2

typedef struct rvn_map_tile_t {
	int z, x, y;
	/** R_MAP_TILE_NONE: a free slot. */
	RMapTileState state;
	Rid texture;
	size_t bytes;
	/** The frame it was last wanted or drawn in. */
	uint32_t used;
	/** Next entry of the bucket (or of the free list), -1 at the end. */
	int next;
} rvn_map_tile_t;

typedef struct rvn_map_marker_t {
	int id;
	double latitude, longitude;
	char* label;
	rvn_color_t color;
	int has_color;
} rvn_map_marker_t;

/** A tile asked for during a paint, asked once the table is up to date. */
typedef struct rvn_map_request_t {
	int z, x, y;
	float distance;
} rvn_map_request_t;

typedef struct rvn_map_view_t {
	rvn_widget_t base;
	/** Center in normalized Mercator coordinates: x 0..1 east from -180, y 0..1 south from the top. */
	double cx, cy;
	double zoom, min_zoom, max_zoom;
	int tile_size;

	rvn_map_tile_t* tiles;
	int tile_cap;
	int tile_live;
	int free_head;
	int buckets[RVN_MAP_BUCKETS];
	size_t cache_size, cache_bytes;
	uint32_t frame;
	/** The tiles wanted at the last paint: level, columns (not wrapped) and rows. */
	int want_valid, want_z, want_x0, want_x1, want_y0, want_y1;
	/** Changes when the source of the tiles does: the files read for another source are dropped. */
	uint32_t generation;
	rvn_map_request_t* requests;
	int request_count, request_cap;

	char* tile_path;
	RMapTileCallback request_callback;
	void* request_user;
	RMapTileCallback cancel_callback;
	void* cancel_user;

	rvn_map_marker_t* markers;
	int marker_count, marker_cap, next_marker;
	RMapMarkerCallback marker_callback;
	void* marker_user;
	RMapViewCallback view_callback;
	void* view_user;
	char* attribution;

	/* the mouse */
	int dragging, moved, pressed_marker;
	float press_x, press_y, last_x, last_y;
	double last_time;
	/* speed of the drag, then of the glide after it (logical units per second) */
	float vx, vy;
	/* what the drag moved since the last paint: the speed is measured by frames, on the clock of the animations */
	float drag_dx, drag_dy;
	int coasting;
	/* the end of an animation or of a glide, found as it painted: told at the next frame */
	int changed_posted;
	float wheel_rest;

	/* animation */
	int anim;
	double anim_zoom;
	double anchor_mx, anchor_my;
	float anchor_x, anchor_y;
	double fly_x, fly_y;

	/* what the last paint drew, for the tests */
	int drawn_loaded, drawn_fallback, drawn_empty;
} rvn_map_view_t;

/* Unique across the views, so that a job of a view that is gone cannot match a new one. */
static uint32_t g_map_generation;

/* ---------------------------------------------------------------------------
 *  Projection
 * ------------------------------------------------------------------------- */

static int rvn_map_finite(double value) {
	return value == value && value - value == 0.0;
}

static double rvn_map_lon_to_x(double longitude) {
	return (longitude + 180.0) / 360.0;
}

static double rvn_map_lat_to_y(double latitude) {
	double s;
	if (latitude > RVN_MAP_MAX_LAT) latitude = RVN_MAP_MAX_LAT;
	if (latitude < -RVN_MAP_MAX_LAT) latitude = -RVN_MAP_MAX_LAT;
	s = sin(latitude * RVN_MAP_PI / 180.0);
	return 0.5 - log((1.0 + s) / (1.0 - s)) / (4.0 * RVN_MAP_PI);
}

static double rvn_map_x_to_lon(double x) {
	x -= floor(x);
	return x * 360.0 - 180.0;
}

static double rvn_map_y_to_lat(double y) {
	return atan(sinh(RVN_MAP_PI * (1.0 - 2.0 * y))) * 180.0 / RVN_MAP_PI;
}

static int rvn_map_wrap(int x, int n) {
	x %= n;
	return x < 0 ? x + n : x;
}

/* Logical units across the whole world at the current zoom. */
static double rvn_map_scale(const rvn_map_view_t* view) {
	return (double)view->tile_size * pow(2.0, view->zoom);
}

static void rvn_map_clamp_center(rvn_map_view_t* view) {
	view->cx -= floor(view->cx);
	if (view->cy < 0.0) view->cy = 0.0;
	if (view->cy > 1.0) view->cy = 1.0;
}

static int rvn_map_top_level(const rvn_map_view_t* view) {
	return (int)floor(view->max_zoom + 1e-9);
}

/* The whole zoom of the tiles drawn: the nearest one, so that they are scaled by 0.7 to 1.4. */
static int rvn_map_level(const rvn_map_view_t* view) {
	int level = (int)floor(view->zoom + 0.5);
	return rvn_clampi(level, 0, rvn_map_top_level(view));
}

/* A point of the widget in normalized coordinates (not wrapped). */
static void rvn_map_point_to_world(const rvn_map_view_t* view, float x, float y, double* wx, double* wy) {
	double scale = rvn_map_scale(view);
	*wx = view->cx + ((double)x - view->base.rect.w * 0.5) / scale;
	*wy = view->cy + ((double)y - view->base.rect.h * 0.5) / scale;
}

/* Where a normalized point is in the widget: the copy of the world nearest the middle. */
static void rvn_map_world_to_point(const rvn_map_view_t* view, double wx, double wy, float* x, float* y) {
	double scale = rvn_map_scale(view);
	double dx = wx - view->cx;
	dx -= floor(dx + 0.5);
	*x = (float)(view->base.rect.w * 0.5 + dx * scale);
	*y = (float)(view->base.rect.h * 0.5 + (wy - view->cy) * scale);
}

/* ---------------------------------------------------------------------------
 *  The table of tiles
 * ------------------------------------------------------------------------- */

static unsigned rvn_map_hash(int z, int x, int y) {
	uint32_t h = (uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u ^ (uint32_t)z * 83492791u;
	return (unsigned)(h & (RVN_MAP_BUCKETS - 1));
}

static int rvn_map_find(const rvn_map_view_t* view, int z, int x, int y) {
	int i = view->buckets[rvn_map_hash(z, x, y)];
	while (i >= 0) {
		const rvn_map_tile_t* tile = &view->tiles[i];
		if (tile->z == z && tile->x == x && tile->y == y) return i;
		i = tile->next;
	}
	return -1;
}

static int rvn_map_insert(rvn_map_view_t* view, int z, int x, int y) {
	unsigned bucket = rvn_map_hash(z, x, y);
	rvn_map_tile_t* tile;
	int i;
	if (view->free_head < 0) {
		int cap = view->tile_cap ? view->tile_cap * 2 : 64;
		rvn_map_tile_t* tiles = (rvn_map_tile_t*)rvn_realloc(view->tiles, sizeof(rvn_map_tile_t) * (size_t)cap);
		if (!tiles) {
			rvn_error(R_ERROR_OUT_OF_MEMORY, R_SEVERITY_ERROR, "MapView: out of memory");
			return -1;
		}
		/* the new slots go to the free list, the first one at its head */
		for (i = cap - 1; i >= view->tile_cap; i--) {
			memset(&tiles[i], 0, sizeof(tiles[i]));
			tiles[i].state = R_MAP_TILE_NONE;
			tiles[i].texture = R_INVALID_ID;
			tiles[i].next = view->free_head;
			view->free_head = i;
		}
		view->tiles = tiles;
		view->tile_cap = cap;
	}
	i = view->free_head;
	tile = &view->tiles[i];
	view->free_head = tile->next;
	tile->z = z;
	tile->x = x;
	tile->y = y;
	tile->state = R_MAP_TILE_PENDING;
	tile->texture = R_INVALID_ID;
	tile->bytes = 0;
	tile->used = view->frame;
	tile->next = view->buckets[bucket];
	view->buckets[bucket] = i;
	view->tile_live++;
	return i;
}

static void rvn_map_remove(rvn_map_view_t* view, int index) {
	rvn_map_tile_t* tile = &view->tiles[index];
	int* link = &view->buckets[rvn_map_hash(tile->z, tile->x, tile->y)];
	while (*link >= 0 && *link != index) link = &view->tiles[*link].next;
	if (*link == index) *link = tile->next;
	if (tile->texture != R_INVALID_ID) Rtexture_destroy(tile->texture);
	view->cache_bytes -= tile->bytes;
	tile->texture = R_INVALID_ID;
	tile->bytes = 0;
	tile->state = R_MAP_TILE_NONE;
	tile->next = view->free_head;
	view->free_head = index;
	view->tile_live--;
}

static void rvn_map_clear(rvn_map_view_t* view) {
	int i;
	for (i = 0; i < view->tile_cap; i++)
		if (view->tiles[i].state != R_MAP_TILE_NONE) rvn_map_remove(view, i);
	view->want_valid = R_FALSE;
	view->generation = ++g_map_generation;
}

/* Drops the least recently used tiles over the budget; never those of the last frame. */
static void rvn_map_evict(rvn_map_view_t* view) {
	while (view->cache_bytes > view->cache_size || view->tile_live > RVN_MAP_MAX_ENTRIES) {
		int i, oldest = -1;
		for (i = 0; i < view->tile_cap; i++) {
			const rvn_map_tile_t* tile = &view->tiles[i];
			if (tile->state != R_MAP_TILE_LOADED && tile->state != R_MAP_TILE_FAILED) continue;
			if (tile->used == view->frame) continue;
			if (oldest < 0 || (int32_t)(tile->used - view->tiles[oldest].used) < 0) oldest = i;
		}
		if (oldest < 0) break;
		rvn_map_remove(view, oldest);
	}
}

/* R_TRUE when the tile is among those the last paint wanted. */
static int rvn_map_wanted(const rvn_map_view_t* view, int z, int x, int y) {
	int n, span;
	if (!view->want_valid || z != view->want_z || y < view->want_y0 || y > view->want_y1) return R_FALSE;
	n = 1 << z;
	span = view->want_x1 - view->want_x0;
	if (span + 1 >= n) return R_TRUE;
	return rvn_map_wrap(x - view->want_x0, n) <= span;
}

/* An answer (pixels, or none) for a tile, on the UI thread. `pixels` is copied. */
static void rvn_map_store(rvn_map_view_t* view, int z, int x, int y, const uint8_t* pixels, int width, int height) {
	int index = rvn_map_find(view, z, x, y);
	rvn_map_tile_t* tile;
	if (index < 0) {
		/* not asked for, or no longer wanted: kept only when it is seen now */
		if (!rvn_map_wanted(view, z, x, y)) return;
		index = rvn_map_insert(view, z, x, y);
		if (index < 0) return;
	}
	tile = &view->tiles[index];
	if (!pixels) {
		if (tile->texture != R_INVALID_ID) Rtexture_destroy(tile->texture);
		view->cache_bytes -= tile->bytes;
		tile->texture = R_INVALID_ID;
		tile->bytes = 0;
		tile->state = R_MAP_TILE_FAILED;
	} else {
		size_t bytes = (size_t)width * (size_t)height * 4;
		if (tile->texture != R_INVALID_ID) {
			Rtexture_update(tile->texture, pixels, width, height);
		} else {
			tile->texture = Rtexture_create_from_pixels(pixels, width, height);
		}
		view->cache_bytes -= tile->bytes;
		if (tile->texture == R_INVALID_ID) {
			tile->bytes = 0;
			tile->state = R_MAP_TILE_FAILED;
		} else {
			tile->bytes = bytes;
			tile->state = R_MAP_TILE_LOADED;
			view->cache_bytes += bytes;
		}
	}
	tile->used = view->frame;
	rvn_map_evict(view);
	rvn_widget_invalidate_paint(&view->base);
}

/* ---------------------------------------------------------------------------
 *  Answers from other threads, and the files read on a worker thread
 * ------------------------------------------------------------------------- */

typedef struct rvn_map_job_t {
	Rid map;
	/** For the files: the source they were read for (0: an answer of the application). */
	uint32_t generation;
	int z, x, y;
	char* path;
	uint8_t* pixels;
	int width, height;
} rvn_map_job_t;

static void rvn_map_job_free(rvn_map_job_t* job) {
	rvn_free(job->path);
	rvn_free(job->pixels);
	rvn_free(job);
}

/* UI thread: the answer goes to the view, unless it is gone or its source changed. */
static void rvn_map_job_finish(void* arg) {
	rvn_map_job_t* job = (rvn_map_job_t*)arg;
	rvn_widget_t* widget = rvn_widget_get(job->map);
	if (widget && widget->cls->type == R_WIDGET_MAP_VIEW) {
		rvn_map_view_t* view = (rvn_map_view_t*)widget;
		if (job->generation == 0 || job->generation == view->generation)
			rvn_map_store(view, job->z, job->x, job->y, job->pixels, job->width, job->height);
	}
	rvn_map_job_free(job);
}

/* Worker thread: reads and decodes a file (no pixels: the tile failed). */
static void rvn_map_job_read(void* arg) {
	rvn_map_job_t* job = (rvn_map_job_t*)arg;
	size_t size = 0;
	char* data = rvn_read_file(job->path, &size);
	if (data) {
		job->pixels = Rimage_decode(data, size, &job->width, &job->height);
		rvn_free(data);
		if (job->pixels && (job->width <= 0 || job->height <= 0 ||
			job->width > RVN_MAP_MAX_TILE_PIXELS || job->height > RVN_MAP_MAX_TILE_PIXELS)) {
			Rpixels_free(job->pixels);
			job->pixels = NULL;
		}
	}
	Rtask_post(rvn_map_job_finish, job);
}

/* The template with {z}, {x}, {y} and {-y} replaced. */
static char* rvn_map_expand_path(const char* pattern, int z, int x, int y) {
	size_t cap = strlen(pattern) + 64, length = 0;
	char* out = (char*)rvn_alloc(cap);
	const char* p = pattern;
	if (!out) return NULL;
	while (*p) {
		int value = 0, token = 0;
		if (strncmp(p, "{z}", 3) == 0) { value = z; token = 3; }
		else if (strncmp(p, "{x}", 3) == 0) { value = x; token = 3; }
		else if (strncmp(p, "{y}", 3) == 0) { value = y; token = 3; }
		else if (strncmp(p, "{-y}", 4) == 0) { value = (1 << z) - 1 - y; token = 4; }
		if (length + 16 >= cap) {
			char* grown = (char*)rvn_realloc(out, cap * 2);
			if (!grown) {
				rvn_free(out);
				return NULL;
			}
			out = grown;
			cap *= 2;
		}
		if (token) {
			length += (size_t)snprintf(out + length, cap - length, "%d", value);
			p += token;
		} else {
			out[length++] = *p++;
		}
	}
	out[length] = '\0';
	return out;
}

/* ---------------------------------------------------------------------------
 *  The view
 * ------------------------------------------------------------------------- */

static void rvn_map_view_init(rvn_widget_t* widget) {
	rvn_map_view_t* view = (rvn_map_view_t*)widget;
	int i;
	view->cx = 0.5;
	view->cy = 0.5;
	view->zoom = 1.0;
	view->min_zoom = 0.0;
	view->max_zoom = 19.0;
	view->tile_size = 256;
	view->free_head = -1;
	for (i = 0; i < RVN_MAP_BUCKETS; i++) view->buckets[i] = -1;
	view->cache_size = RVN_MAP_DEFAULT_CACHE;
	view->next_marker = 1;
	view->pressed_marker = -1;
	view->generation = ++g_map_generation;
}

static void rvn_map_free_markers(rvn_map_view_t* view) {
	int i;
	for (i = 0; i < view->marker_count; i++) rvn_free(view->markers[i].label);
	view->marker_count = 0;
}

static void rvn_map_view_destroy(rvn_widget_t* widget) {
	rvn_map_view_t* view = (rvn_map_view_t*)widget;
	rvn_map_clear(view);
	rvn_free(view->tiles);
	rvn_free(view->requests);
	rvn_map_free_markers(view);
	rvn_free(view->markers);
	rvn_free(view->tile_path);
	rvn_free(view->attribution);
}

static rvn_vec2_t rvn_map_view_measure(rvn_widget_t* widget) {
	RVN_UNUSED(widget);
	return rvn_vec2(400.0f, 300.0f);
}

/* The view moved because of the user: the application is told. */
static void rvn_map_changed(rvn_map_view_t* view) {
	rvn_widget_t* widget = &view->base;
	if (view->view_callback)
		view->view_callback(widget->id, rvn_map_y_to_lat(view->cy), rvn_map_x_to_lon(view->cx), view->zoom, view->view_user);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

/* UI thread, at the next frame: what the paint found is told outside it, if the view is still there. */
static void rvn_map_changed_task(void* user_data) {
	rvn_widget_t* widget = rvn_widget_get((Rid)(uintptr_t)user_data);
	if (widget && widget->cls->type == R_WIDGET_MAP_VIEW) {
		rvn_map_view_t* view = (rvn_map_view_t*)widget;
		view->changed_posted = R_FALSE;
		rvn_map_changed(view);
	}
}

/*
 * The view moved as it painted (a zoom, a flight or a glide that goes on or ends): the
 * application is told at the next frame. Its callback and its action may change the tree,
 * which must not change while it is painted (a sibling destroyed under the loop that paints it).
 * One telling per frame at most, with the view as it is then.
 */
static void rvn_map_changed_later(rvn_map_view_t* view) {
	if (view->changed_posted) return;
	view->changed_posted = R_TRUE;
	Rtask_post(rvn_map_changed_task, (void*)(uintptr_t)view->base.id);
}

static void rvn_map_stop(rvn_map_view_t* view) {
	view->anim = RVN_MAP_ANIM_NONE;
	view->coasting = R_FALSE;
	view->vx = view->vy = 0.0f;
}

/* Moves what is shown by (dx, dy) logical units, as a hand dragging it. R_TRUE when it moved. */
static int rvn_map_pan_by(rvn_map_view_t* view, float dx, float dy) {
	double scale = rvn_map_scale(view);
	double x = view->cx, y = view->cy;
	view->cx -= (double)dx / scale;
	view->cy -= (double)dy / scale;
	rvn_map_clamp_center(view);
	if (view->cx == x && view->cy == y) return R_FALSE;
	rvn_widget_invalidate_paint(&view->base);
	return R_TRUE;
}

/* Puts the anchor of a zoom (anchor_mx, anchor_my) back under its point (anchor_x, anchor_y). */
static void rvn_map_place_anchor(rvn_map_view_t* view) {
	double scale = rvn_map_scale(view);
	view->cx = view->anchor_mx - ((double)view->anchor_x - view->base.rect.w * 0.5) / scale;
	view->cy = view->anchor_my - ((double)view->anchor_y - view->base.rect.h * 0.5) / scale;
	rvn_map_clamp_center(view);
}

static double rvn_map_clamp_zoom(const rvn_map_view_t* view, double zoom) {
	if (zoom < view->min_zoom) zoom = view->min_zoom;
	if (zoom > view->max_zoom) zoom = view->max_zoom;
	return zoom;
}

/* Sets the zoom at once, the place under (x, y) staying there. R_TRUE when it changed. */
static int rvn_map_zoom_at(rvn_map_view_t* view, double zoom, float x, float y) {
	zoom = rvn_map_clamp_zoom(view, zoom);
	if (zoom == view->zoom) return R_FALSE;
	rvn_map_point_to_world(view, x, y, &view->anchor_mx, &view->anchor_my);
	view->anchor_x = x;
	view->anchor_y = y;
	view->zoom = zoom;
	rvn_map_place_anchor(view);
	rvn_widget_invalidate_paint(&view->base);
	return R_TRUE;
}

/* Zooms to `zoom` around (x, y): animated when animations are on, else at once (and the application told). */
static void rvn_map_zoom_to(rvn_map_view_t* view, double zoom, float x, float y) {
	zoom = rvn_map_clamp_zoom(view, zoom);
	view->coasting = R_FALSE;
	if (!Rget_animations_enabled()) {
		view->anim = RVN_MAP_ANIM_NONE;
		if (rvn_map_zoom_at(view, zoom, x, y)) rvn_map_changed(view);
		return;
	}
	rvn_map_point_to_world(view, x, y, &view->anchor_mx, &view->anchor_my);
	view->anchor_x = x;
	view->anchor_y = y;
	view->anim_zoom = zoom;
	view->anim = RVN_MAP_ANIM_ZOOM;
	rvn_widget_invalidate_paint(&view->base);
}

/* One step of the zoom (a notch of the wheel, a key, a double click): to the next whole zoom. */
static void rvn_map_zoom_step(rvn_map_view_t* view, int direction, float x, float y) {
	double from = view->anim == RVN_MAP_ANIM_ZOOM ? view->anim_zoom : view->zoom;
	double to = direction > 0 ? floor(from + 1e-6) + 1.0 : ceil(from - 1e-6) - 1.0;
	rvn_map_zoom_to(view, to, x, y);
}

/* Advances the animations and the glide; while painting. */
static void rvn_map_step(rvn_map_view_t* view, RPainter* painter) {
	double dt = rvn_clampf(painter->dt, 0.0f, 0.1f);
	double k = 1.0 - exp(-dt * 12.0);
	int finished = R_FALSE, moved = R_FALSE;

	if (view->anim == RVN_MAP_ANIM_ZOOM) {
		view->zoom += (view->anim_zoom - view->zoom) * k;
		if (fabs(view->anim_zoom - view->zoom) < 0.002) {
			view->zoom = view->anim_zoom;
			view->anim = RVN_MAP_ANIM_NONE;
			finished = R_TRUE;
		}
		rvn_map_place_anchor(view);
	} else if (view->anim == RVN_MAP_ANIM_FLY) {
		double dx = view->fly_x - view->cx, dy, scale;
		dx -= floor(dx + 0.5);
		view->cx += dx * k;
		view->cy += (view->fly_y - view->cy) * k;
		view->zoom += (view->anim_zoom - view->zoom) * k;
		rvn_map_clamp_center(view);
		scale = rvn_map_scale(view);
		dx = view->fly_x - view->cx;
		dx -= floor(dx + 0.5);
		dy = view->fly_y - view->cy;
		if (fabs(dx) * scale < 0.25 && fabs(dy) * scale < 0.25 && fabs(view->anim_zoom - view->zoom) < 0.002) {
			view->cx = view->fly_x;
			view->cy = view->fly_y;
			view->zoom = view->anim_zoom;
			view->anim = RVN_MAP_ANIM_NONE;
			finished = R_TRUE;
		}
	}
	if (view->dragging && view->moved && dt > 0.0) {
		/* the speed of the hand: the newest frames weigh most */
		view->vx = view->vx * 0.4f + view->drag_dx / (float)dt * 0.6f;
		view->vy = view->vy * 0.4f + view->drag_dy / (float)dt * 0.6f;
		view->drag_dx = view->drag_dy = 0.0f;
	}
	if (view->coasting) {
		float decay = (float)exp(-dt * 4.0);
		moved = rvn_map_pan_by(view, view->vx * (float)dt, view->vy * (float)dt);
		view->vx *= decay;
		view->vy *= decay;
		if (!moved || fabsf(view->vx) + fabsf(view->vy) < 12.0f) {
			view->coasting = R_FALSE;
			finished = R_TRUE;
		}
	}
	if (view->anim != RVN_MAP_ANIM_NONE || view->coasting) rvn_paint_keep_animating(painter);
	if (finished || moved) rvn_map_changed_later(view);
}

/* ---- markers ---- */

static int rvn_map_marker_index(const rvn_map_view_t* view, int id) {
	int i;
	for (i = 0; i < view->marker_count; i++) if (view->markers[i].id == id) return i;
	return -1;
}

static float rvn_map_label_size(const rvn_widget_t* widget) {
	return rvn_widget_font_size(widget, 0.0f) * 0.92f;
}

/* The pin and the label of a marker whose tip is at (x, y). */
static void rvn_map_marker_rects(const rvn_map_view_t* view, const rvn_map_marker_t* marker, float x, float y,
	rvn_rect_t* pin, rvn_rect_t* label) {
	const rvn_widget_t* widget = &view->base;
	*pin = rvn_rect(x - 10.0f, y - 30.0f, 20.0f, 30.0f);
	if (marker->label && marker->label[0]) {
		float size = rvn_map_label_size(widget);
		float width = rvn_widget_text_width(widget, R_INVALID_ID, size, marker->label);
		float height = rvn_widget_line_height(widget, R_INVALID_ID, size);
		*label = rvn_rect(x + 12.0f, y - 20.0f - height * 0.5f - 2.0f, width + 12.0f, height + 4.0f);
	} else {
		*label = rvn_rect(x, y, 0.0f, 0.0f);
	}
}

/* The copies of the world side by side that may show a point: offsets (in worlds) from the nearest one. */
static int rvn_map_copies(const rvn_map_view_t* view) {
	double scale = rvn_map_scale(view);
	return scale >= view->base.rect.w ? 0 : (int)ceil(view->base.rect.w / scale * 0.5) + 1;
}

static int rvn_map_inside(rvn_rect_t rect, float x, float y) {
	return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
}

/* The marker under a point of the widget (the last drawn first), or -1. */
static int rvn_map_marker_at(const rvn_map_view_t* view, float x, float y) {
	double scale = rvn_map_scale(view);
	int copies = rvn_map_copies(view), i, k;
	for (i = view->marker_count - 1; i >= 0; i--) {
		const rvn_map_marker_t* marker = &view->markers[i];
		float mx, my;
		rvn_map_world_to_point(view, rvn_map_lon_to_x(marker->longitude), rvn_map_lat_to_y(marker->latitude), &mx, &my);
		for (k = -copies; k <= copies; k++) {
			rvn_rect_t pin, label;
			rvn_map_marker_rects(view, marker, mx + (float)(k * scale), my, &pin, &label);
			if (rvn_map_inside(pin, x, y) || rvn_map_inside(label, x, y)) return i;
		}
	}
	return -1;
}

int rvn_map_view_marker_info(const rvn_widget_t* widget, int index, const char** label, rvn_rect_t* rect) {
	const rvn_map_view_t* view = (const rvn_map_view_t*)widget;
	const rvn_map_marker_t* marker;
	rvn_rect_t text;
	float x, y;
	if (widget->cls->type != R_WIDGET_MAP_VIEW || index < 0 || index >= view->marker_count) return R_FALSE;
	marker = &view->markers[index];
	rvn_map_world_to_point(view, rvn_map_lon_to_x(marker->longitude), rvn_map_lat_to_y(marker->latitude), &x, &y);
	rvn_map_marker_rects(view, marker, x, y, rect, &text);
	*label = marker->label ? marker->label : "";
	return R_TRUE;
}

void rvn_map_view_press_marker(rvn_widget_t* widget, int index) {
	rvn_map_view_t* view = (rvn_map_view_t*)widget;
	if (widget->cls->type != R_WIDGET_MAP_VIEW || index < 0 || index >= view->marker_count) return;
	if (view->marker_callback) view->marker_callback(widget->id, view->markers[index].id, view->marker_user);
}

/** For the tests: what the last paint drew (tiles of their own, coarser ones stretched, nothing). */
int rvn_map_view_debug_counts(Rid id, int* loaded, int* fallback, int* empty);
int rvn_map_view_debug_counts(Rid id, int* loaded, int* fallback, int* empty) {
	rvn_widget_t* widget = rvn_widget_get(id);
	rvn_map_view_t* view = (rvn_map_view_t*)widget;
	if (!widget || widget->cls->type != R_WIDGET_MAP_VIEW) return R_FALSE;
	*loaded = view->drawn_loaded;
	*fallback = view->drawn_fallback;
	*empty = view->drawn_empty;
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Paint
 * ------------------------------------------------------------------------- */

/* Where the tiles of the drawn level lie: the corner of the world in the widget, and the size of a tile. */
typedef struct rvn_map_grid_t {
	int level, n;
	double ox, oy, tile;
	int x0, x1, y0, y1;
} rvn_map_grid_t;

static void rvn_map_grid(const rvn_map_view_t* view, rvn_map_grid_t* grid) {
	const rvn_widget_t* widget = &view->base;
	double scale = rvn_map_scale(view);
	grid->level = rvn_map_level(view);
	grid->n = 1 << grid->level;
	grid->tile = scale / grid->n;
	grid->ox = widget->rect.w * 0.5 - view->cx * scale;
	grid->oy = widget->rect.h * 0.5 - view->cy * scale;
	grid->x0 = (int)floor(-grid->ox / grid->tile);
	grid->x1 = (int)floor((widget->rect.w - grid->ox) / grid->tile);
	grid->y0 = rvn_maxi(0, (int)floor(-grid->oy / grid->tile));
	grid->y1 = rvn_mini(grid->n - 1, (int)floor((widget->rect.h - grid->oy) / grid->tile));
}

static int rvn_map_request_order(const void* a, const void* b) {
	float da = ((const rvn_map_request_t*)a)->distance, db = ((const rvn_map_request_t*)b)->distance;
	return da < db ? -1 : da > db ? 1 : 0;
}

static void rvn_map_add_request(rvn_map_view_t* view, int z, int x, int y, float distance) {
	rvn_map_request_t* request;
	if (view->request_count == view->request_cap) {
		int cap = view->request_cap ? view->request_cap * 2 : 64;
		rvn_map_request_t* grown = (rvn_map_request_t*)rvn_realloc(view->requests, sizeof(rvn_map_request_t) * (size_t)cap);
		if (!grown) return;
		view->requests = grown;
		view->request_cap = cap;
	}
	request = &view->requests[view->request_count++];
	request->z = z;
	request->x = x;
	request->y = y;
	request->distance = distance;
}

/*
 * Brings the table up to date with what is seen: the wanted tiles are marked
 * (those missing are asked for, the nearest the middle first), the pending
 * ones no longer wanted are forgotten. Calls the application.
 */
static void rvn_map_update_tiles(rvn_map_view_t* view) {
	rvn_widget_t* widget = &view->base;
	Rid id = widget->id;
	uint32_t generation = view->generation;
	rvn_map_grid_t grid;
	int x, y, i, columns, first;
	double middle_x, middle_y;

	memset(&grid, 0, sizeof(grid));
	view->request_count = 0;
	if (widget->rect.w <= 0.0f || widget->rect.h <= 0.0f || (!view->request_callback && !view->tile_path)) {
		view->want_valid = R_FALSE;
	} else {
		rvn_map_grid(view, &grid);
		view->want_valid = R_TRUE;
		view->want_z = grid.level;
		view->want_x0 = grid.x0 - 1;
		view->want_x1 = grid.x1 + 1;
		view->want_y0 = rvn_maxi(0, grid.y0 - 1);
		view->want_y1 = rvn_mini(grid.n - 1, grid.y1 + 1);
		if (view->want_y1 < view->want_y0) view->want_valid = R_FALSE;
	}

	/* the pending tiles no longer wanted are forgotten (an answer for them will be dropped) */
	for (i = 0; i < view->tile_cap; i++) {
		rvn_map_tile_t* tile = &view->tiles[i];
		if (tile->state == R_MAP_TILE_PENDING && !rvn_map_wanted(view, tile->z, tile->x, tile->y)) {
			rvn_map_add_request(view, tile->z, tile->x, tile->y, -1.0f);
			rvn_map_remove(view, i);
		}
	}
	if (view->cancel_callback) {
		RMapTileCallback cancel = view->cancel_callback;
		void* user = view->cancel_user;
		int count = view->request_count;
		for (i = 0; i < count && i < view->request_count; i++) {
			rvn_map_request_t request = view->requests[i];
			cancel(id, request.z, request.x, request.y, user);
			if (rvn_widget_get(id) != widget || view->generation != generation) return;
		}
	}
	view->request_count = 0;
	if (!view->want_valid) return;

	/* the wanted tiles: marked as used, the missing ones asked for */
	columns = view->want_x1 - view->want_x0 + 1;
	first = view->want_x0;
	if (columns > grid.n) {
		columns = grid.n;
		first = 0;
	}
	middle_x = (widget->rect.w * 0.5 - grid.ox) / grid.tile;
	middle_y = (widget->rect.h * 0.5 - grid.oy) / grid.tile;
	for (y = view->want_y0; y <= view->want_y1; y++) {
		for (x = first; x < first + columns; x++) {
			int wx = rvn_map_wrap(x, grid.n);
			int index = rvn_map_find(view, grid.level, wx, y);
			if (index >= 0) {
				view->tiles[index].used = view->frame;
				continue;
			}
			index = rvn_map_insert(view, grid.level, wx, y);
			if (index < 0) return;
			{
				double dx = x + 0.5 - middle_x, dy = y + 0.5 - middle_y;
				rvn_map_add_request(view, grid.level, wx, y, (float)(dx * dx + dy * dy));
			}
		}
	}
	if (view->request_count > 1)
		qsort(view->requests, (size_t)view->request_count, sizeof(rvn_map_request_t), rvn_map_request_order);

	if (view->tile_path) {
		for (i = 0; i < view->request_count; i++) {
			rvn_map_request_t* request = &view->requests[i];
			rvn_map_job_t* job = (rvn_map_job_t*)rvn_calloc(sizeof(rvn_map_job_t));
			if (!job) break;
			job->map = id;
			job->generation = generation;
			job->z = request->z;
			job->x = request->x;
			job->y = request->y;
			job->path = rvn_map_expand_path(view->tile_path, request->z, request->x, request->y);
			if (!job->path) {
				rvn_free(job);
				break;
			}
			rvn_jobs_submit(NULL, rvn_map_job_read, job);
		}
	} else if (view->request_callback) {
		/* (the application may answer at once, change the source, or move the map: nothing is held across) */
		RMapTileCallback request_tile = view->request_callback;
		void* user = view->request_user;
		int count = view->request_count;
		for (i = 0; i < count && i < view->request_count; i++) {
			rvn_map_request_t request = view->requests[i];
			int index = rvn_map_find(view, request.z, request.x, request.y);
			if (index < 0 || view->tiles[index].state != R_MAP_TILE_PENDING) continue;
			request_tile(id, request.z, request.x, request.y, user);
			if (rvn_widget_get(id) != widget || view->generation != generation) return;
		}
	}
	view->request_count = 0;
}

/* Edges snapped to the pixels, so that the tiles meet without seams. */
static float rvn_map_snap(double value, float scale) {
	return (float)(floor(value * scale + 0.5) / scale);
}

static int rvn_map_loaded(rvn_map_view_t* view, int z, int x, int y) {
	int index = rvn_map_find(view, z, x, y);
	if (index < 0 || view->tiles[index].state != R_MAP_TILE_LOADED) return -1;
	view->tiles[index].used = view->frame;
	return index;
}

/* Draws the tile (level, x, y) of the grid; x not wrapped (a copy of the world). */
static void rvn_map_paint_tile(rvn_map_view_t* view, RPainter* painter, const rvn_map_grid_t* grid, int x, int y,
	float pixel, rvn_color_t line) {
	static const float whole[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
	int wx = rvn_map_wrap(x, grid->n);
	float left = rvn_map_snap(grid->ox + x * grid->tile, pixel), right = rvn_map_snap(grid->ox + (x + 1) * grid->tile, pixel);
	float top = rvn_map_snap(grid->oy + y * grid->tile, pixel), bottom = rvn_map_snap(grid->oy + (y + 1) * grid->tile, pixel);
	rvn_rect_t rect = rvn_rect(left, top, right - left, bottom - top);
	int index = rvn_map_loaded(view, grid->level, wx, y);
	int k, cx, cy;

	if (index >= 0) {
		rvn_paint_image(painter, rect, view->tiles[index].texture, whole, 0xFFFFFFFFu, 0.0f);
		view->drawn_loaded++;
		return;
	}
	/* a coarser tile that is there, the part of it that covers this one */
	for (k = 1; k <= RVN_MAP_FALLBACK_LEVELS && grid->level - k >= 0; k++) {
		int px = wx >> k, py = y >> k, parent = rvn_map_loaded(view, grid->level - k, px, py);
		if (parent >= 0) {
			float part = 1.0f / (float)(1 << k);
			float uv[4];
			uv[0] = (float)(wx - (px << k)) * part;
			uv[1] = (float)(y - (py << k)) * part;
			uv[2] = uv[0] + part;
			uv[3] = uv[1] + part;
			rvn_paint_image(painter, rect, view->tiles[parent].texture, uv, 0xFFFFFFFFu, 0.0f);
			view->drawn_fallback++;
			break;
		}
	}
	if (k > RVN_MAP_FALLBACK_LEVELS || grid->level - k < 0) {
		/* nothing: a faint grid */
		rvn_paint_line(painter, left, top, right, top, 1.0f, line);
		rvn_paint_line(painter, left, top, left, bottom, 1.0f, line);
		view->drawn_empty++;
	}
	/* the finer tiles that are there (zooming out) */
	if (grid->level + 1 <= rvn_map_top_level(view)) {
		for (cy = 0; cy < 2; cy++) {
			for (cx = 0; cx < 2; cx++) {
				int child = rvn_map_loaded(view, grid->level + 1, wx * 2 + cx, y * 2 + cy);
				if (child >= 0) {
					double half = grid->tile * 0.5;
					float l = rvn_map_snap(grid->ox + x * grid->tile + cx * half, pixel);
					float r = rvn_map_snap(grid->ox + x * grid->tile + (cx + 1) * half, pixel);
					float t = rvn_map_snap(grid->oy + y * grid->tile + cy * half, pixel);
					float b = rvn_map_snap(grid->oy + y * grid->tile + (cy + 1) * half, pixel);
					rvn_paint_image(painter, rvn_rect(l, t, r - l, b - t), view->tiles[child].texture, whole, 0xFFFFFFFFu, 0.0f);
				}
			}
		}
	}
}

static void rvn_map_paint_marker(const rvn_map_view_t* view, RPainter* painter, const rvn_map_marker_t* marker, float x, float y) {
	const rvn_widget_t* widget = &view->base;
	rvn_color_t color = marker->has_color ? marker->color : rvn_widget_color(widget, R_COLOR_ACCENT);
	rvn_color_t edge = rvn_color_mix(color, 0xFF000000u, 0.35f);
	rvn_rect_t pin, label;
	float head_y = y - 20.0f;

	rvn_map_marker_rects(view, marker, x, y, &pin, &label);
	/* the shadow on the ground, the point, the head, the hole */
	rvn_paint_box(painter, rvn_rect(x - 6.0f, y - 2.5f, 12.0f, 5.0f), 2.5f, rvn_rgba8(0, 0, 0, 70), 0.0f, 0);
	rvn_paint_triangle(painter, x - 7.4f, head_y + 4.6f, x + 7.4f, head_y + 4.6f, x, y, edge);
	rvn_paint_triangle(painter, x - 6.2f, head_y + 4.0f, x + 6.2f, head_y + 4.0f, x, y - 1.6f, color);
	rvn_paint_box(painter, rvn_rect(x - 9.0f, head_y - 9.0f, 18.0f, 18.0f), 9.0f, color, 1.0f, edge);
	rvn_paint_box(painter, rvn_rect(x - 3.5f, head_y - 3.5f, 7.0f, 7.0f), 3.5f, 0xFFFFFFFFu, 0.0f, 0);

	if (label.w > 0.0f) {
		float size = rvn_map_label_size(widget);
		rvn_color_t back = rvn_color_fade(rvn_widget_color(widget, R_COLOR_PANEL), 0.88f);
		rvn_paint_shadow(painter, label, label.h * 0.5f, 6.0f, rvn_rgba8(0, 0, 0, 50));
		rvn_paint_box(painter, label, label.h * 0.5f, back, 1.0f, rvn_color_fade(rvn_widget_color(widget, R_COLOR_BORDER), 0.6f));
		rvn_paint_text(painter, R_INVALID_ID, size, label.x + 6.0f, label.y + 2.0f, marker->label, -1, rvn_widget_color(widget, R_COLOR_TEXT));
	}
}

static void rvn_map_view_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_map_view_t* view = (rvn_map_view_t*)widget;
	float w = widget->rect.w, h = widget->rect.h;
	float pixel = painter->scale > 0.0f ? painter->scale : 1.0f;
	rvn_color_t back = rvn_widget_color(widget, R_COLOR_PANEL);
	rvn_color_t line = rvn_color_fade(rvn_widget_color(widget, R_COLOR_BORDER), 0.55f);
	rvn_map_grid_t grid;
	int x, y, i, k, copies;
	double scale;

	rvn_map_step(view, painter);
	view->frame++;
	rvn_map_update_tiles(view);
	if (!rvn_widget_alive(widget)) return;

	view->drawn_loaded = view->drawn_fallback = view->drawn_empty = 0;
	rvn_paint_rect(painter, rvn_rect(0.0f, 0.0f, w, h), back);
	rvn_paint_push_clip(painter, rvn_rect(0.0f, 0.0f, w, h));
	rvn_map_grid(view, &grid);
	if (grid.y1 >= grid.y0 && grid.x1 - grid.x0 < 4096) {
		for (y = grid.y0; y <= grid.y1; y++)
			for (x = grid.x0; x <= grid.x1; x++) rvn_map_paint_tile(view, painter, &grid, x, y, pixel, line);
	}

	/* the markers, on every copy of the world that is seen */
	scale = rvn_map_scale(view);
	copies = rvn_map_copies(view);
	for (i = 0; i < view->marker_count; i++) {
		const rvn_map_marker_t* marker = &view->markers[i];
		float mx, my;
		rvn_map_world_to_point(view, rvn_map_lon_to_x(marker->longitude), rvn_map_lat_to_y(marker->latitude), &mx, &my);
		if (my < -4.0f || my > h + 40.0f) continue;
		for (k = -copies; k <= copies; k++) {
			float px = mx + (float)(k * scale);
			if (px < -300.0f || px > w + 40.0f) continue;
			rvn_map_paint_marker(view, painter, marker, px, my);
		}
	}

	/* the credit of the tiles */
	if (view->attribution && view->attribution[0]) {
		float size = rvn_widget_font_size(widget, 0.0f) * 0.8f;
		float tw = rvn_widget_text_width(widget, R_INVALID_ID, size, view->attribution);
		float th = rvn_widget_line_height(widget, R_INVALID_ID, size);
		rvn_rect_t box = rvn_rect(w - tw - 10.0f, h - th - 4.0f, tw + 10.0f, th + 4.0f);
		rvn_paint_rect(painter, box, rvn_color_fade(back, 0.75f));
		rvn_paint_text(painter, R_INVALID_ID, size, box.x + 5.0f, box.y + 2.0f, view->attribution, -1,
			rvn_widget_color(widget, R_COLOR_TEXT));
	}
	rvn_paint_pop_clip(painter);
	rvn_paint_box(painter, rvn_rect(0.0f, 0.0f, w, h), 0.0f, 0, 1.0f, rvn_widget_color(widget, R_COLOR_BORDER));
	rvn_paint_focus(widget, painter, 0.0f);

	rvn_map_evict(view);
}

/* ---------------------------------------------------------------------------
 *  Input
 * ------------------------------------------------------------------------- */

static int rvn_map_view_event(rvn_widget_t* widget, const REvent* event) {
	rvn_map_view_t* view = (rvn_map_view_t*)widget;
	float x = event->local_x, y = event->local_y;

	switch (event->type) {
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			rvn_map_stop(view);
			if (event->clicks == 2 && rvn_map_marker_at(view, x, y) < 0) {
				/* a double click zooms in (out with Shift) around the pointer; on a marker, it is two clicks */
				view->dragging = R_FALSE;
				rvn_map_zoom_step(view, (event->modifiers & R_MOD_SHIFT) ? -1 : 1, x, y);
				return R_TRUE;
			}
			view->dragging = R_TRUE;
			view->moved = R_FALSE;
			view->press_x = view->last_x = x;
			view->press_y = view->last_y = y;
			view->last_time = Rget_time();
			view->drag_dx = view->drag_dy = 0.0f;
			view->pressed_marker = rvn_map_marker_at(view, x, y);
			rvn_widget_capture_mouse(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_MOVE: {
			float dx, dy;
			if (!view->dragging) return R_FALSE;
			if (!view->moved && fabsf(x - view->press_x) + fabsf(y - view->press_y) < 4.0f) return R_TRUE;
			if (!view->moved) {
				/* the hand takes the map where the pointer was pressed */
				view->moved = R_TRUE;
				view->last_x = view->press_x;
				view->last_y = view->press_y;
			}
			dx = x - view->last_x;
			dy = y - view->last_y;
			view->drag_dx += dx;
			view->drag_dy += dy;
			view->last_time = Rget_time();
			view->last_x = x;
			view->last_y = y;
			if (rvn_map_pan_by(view, dx, dy)) rvn_map_changed(view);
			return R_TRUE;
		}
		case R_EVENT_MOUSE_UP: {
			int marker = view->pressed_marker;
			if (!view->dragging || event->button != R_MOUSE_LEFT) return R_FALSE;
			view->dragging = R_FALSE;
			view->pressed_marker = -1;
			rvn_widget_release_mouse(widget);
			if (!view->moved) {
				if (marker >= 0 && rvn_map_marker_at(view, x, y) == marker) rvn_map_view_press_marker(widget, marker);
				return R_TRUE;
			}
			/* flung (still moving when let go): it glides on */
			if (Rget_animations_enabled() && Rget_time() - view->last_time < 0.08 && fabsf(view->vx) + fabsf(view->vy) > 80.0f) {
				float speed = sqrtf(view->vx * view->vx + view->vy * view->vy);
				if (speed > 4000.0f) {
					view->vx *= 4000.0f / speed;
					view->vy *= 4000.0f / speed;
				}
				view->coasting = R_TRUE;
				rvn_widget_invalidate_paint(widget);
			}
			return R_TRUE;
		}
		case R_EVENT_MOUSE_WHEEL: {
			int steps;
			if (view->dragging) return R_TRUE;
			if (event->wheel_precise && !(event->modifiers & (R_MOD_CTRL | R_MOD_SUPER))) {
				/* a trackpad: two fingers move the map (they pinch to zoom) */
				view->coasting = R_FALSE;
				if (rvn_map_pan_by(view, event->wheel_x * 48.0f, event->wheel_y * 48.0f)) rvn_map_changed(view);
				return R_TRUE;
			}
			if (event->wheel_precise) {
				/* Ctrl and a trackpad (the pinch of some systems): smoothly */
				rvn_map_stop(view);
				if (rvn_map_zoom_at(view, view->zoom + event->wheel_y * 0.32, x, y)) rvn_map_changed(view);
				return R_TRUE;
			}
			/* a wheel: one whole zoom per notch */
			steps = rvn_wheel_notches(&view->wheel_rest, event->wheel_y != 0.0f ? event->wheel_y : event->wheel_x);
			while (steps > 0) {
				rvn_map_zoom_step(view, 1, x, y);
				steps--;
			}
			while (steps < 0) {
				rvn_map_zoom_step(view, -1, x, y);
				steps++;
			}
			return R_TRUE;
		}
		case R_EVENT_FOCUS_GAINED:
		case R_EVENT_FOCUS_LOST:
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_KEY_DOWN: {
			float cx = widget->rect.w * 0.5f, cy = widget->rect.h * 0.5f;
			int moved = R_FALSE;
			if (event->modifiers & (R_MOD_CTRL | R_MOD_SUPER | R_MOD_ALT)) return R_FALSE;
			switch (event->key) {
				case R_KEY_EQUAL:
				case R_KEY_KP_ADD:
					rvn_map_zoom_step(view, 1, cx, cy);
					return R_TRUE;
				case R_KEY_MINUS:
				case R_KEY_KP_SUBTRACT:
					rvn_map_zoom_step(view, -1, cx, cy);
					return R_TRUE;
				case R_KEY_LEFT: rvn_map_stop(view); moved = rvn_map_pan_by(view, 80.0f, 0.0f); break;
				case R_KEY_RIGHT: rvn_map_stop(view); moved = rvn_map_pan_by(view, -80.0f, 0.0f); break;
				case R_KEY_UP: rvn_map_stop(view); moved = rvn_map_pan_by(view, 0.0f, 80.0f); break;
				case R_KEY_DOWN: rvn_map_stop(view); moved = rvn_map_pan_by(view, 0.0f, -80.0f); break;
				default: return R_FALSE;
			}
			if (moved) rvn_map_changed(view);
			return R_TRUE;
		}
		default:
			return R_FALSE;
	}
}

static int rvn_map_view_pan(rvn_widget_t* widget, float dx, float dy, int coasting) {
	rvn_map_view_t* view = (rvn_map_view_t*)widget;
	if (!coasting) view->anim = RVN_MAP_ANIM_NONE;
	view->coasting = R_FALSE;
	if (!rvn_map_pan_by(view, dx, dy)) return R_FALSE;
	rvn_map_changed(view);
	return R_TRUE;
}

static int rvn_map_view_zoom(rvn_widget_t* widget, float factor, float x, float y) {
	rvn_map_view_t* view = (rvn_map_view_t*)widget;
	if (!(factor > 0.0f)) return R_FALSE;
	rvn_map_stop(view);
	if (!rvn_map_zoom_at(view, view->zoom + log((double)factor) / log(2.0), x, y)) return R_FALSE;
	rvn_map_changed(view);
	return R_TRUE;
}

static int rvn_map_view_touch_drags(rvn_widget_t* widget, float x, float y) {
	/* a finger on a marker clicks it */
	return rvn_map_marker_at((rvn_map_view_t*)widget, x, y) >= 0;
}

/* ---------------------------------------------------------------------------
 *  XML
 * ------------------------------------------------------------------------- */

/*
 * A number of an attribute, in double precision: a place and a zoom are doubles in the API, and
 * a float holds 7 digits (a longitude of 120.000001 would be 120: 90 logical units at zoom 24).
 */
static int rvn_map_parse_double(const char* text, double* value) {
	char* end;
	double number;
	if (!text) return R_FALSE;
	number = strtod(text, &end);
	if (end == text) return R_FALSE;
	while (*end == ' ' || *end == '\t') end++;
	if (*end || !rvn_map_finite(number)) return R_FALSE;
	*value = number;
	return R_TRUE;
}

static int rvn_map_view_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_map_view_t* view = (rvn_map_view_t*)widget;
	double number;
	int whole;
	if (RVN_ATTR("Latitude") || RVN_ATTR("Lat")) {
		if (!rvn_map_parse_double(value, &number)) return R_FALSE;
		Rmap_view_set_center(widget->id, number, rvn_map_x_to_lon(view->cx));
	} else if (RVN_ATTR("Longitude") || RVN_ATTR("Lon") || RVN_ATTR("Lng")) {
		if (!rvn_map_parse_double(value, &number)) return R_FALSE;
		Rmap_view_set_center(widget->id, rvn_map_y_to_lat(view->cy), number);
	} else if (RVN_ATTR("Zoom")) {
		if (!rvn_map_parse_double(value, &number)) return R_FALSE;
		Rmap_view_set_zoom(widget->id, number);
	} else if (RVN_ATTR("MinZoom")) {
		if (!rvn_map_parse_double(value, &number) || number < 0.0 || number > view->max_zoom) return R_FALSE;
		Rmap_view_set_zoom_range(widget->id, number, view->max_zoom);
	} else if (RVN_ATTR("MaxZoom")) {
		if (!rvn_map_parse_double(value, &number) || number < view->min_zoom || number > RVN_MAP_LIMIT_ZOOM) return R_FALSE;
		Rmap_view_set_zoom_range(widget->id, view->min_zoom, number);
	} else if (RVN_ATTR("TilePath")) {
		char* path = value && value[0] ? rvn_xml_resolve_path(value) : NULL;
		Rmap_view_set_tile_path(widget->id, path ? path : value);
		rvn_free(path);
	} else if (RVN_ATTR("TileSize")) {
		if (!rvn_parse_int(value, &whole)) return R_FALSE;
		Rmap_view_set_tile_size(widget->id, rvn_clampi(whole, 16, 1024));
	} else if (RVN_ATTR("Attribution")) {
		Rmap_view_set_attribution(widget->id, value);
	} else {
		return R_FALSE;
	}
	return R_TRUE;
}

static int rvn_map_view_xml_child(rvn_widget_t* widget, const rvn_xml_node_t* node) {
	rvn_map_view_t* view = (rvn_map_view_t*)widget;
	const char* value;
	double latitude, longitude;
	int id, index;
	if (rvn_stricmp(node->name, "Marker") != 0) return R_FALSE;
	if (!(value = rvn_xml_attribute(node, "Latitude")) || !rvn_map_parse_double(value, &latitude)) return R_FALSE;
	if (!(value = rvn_xml_attribute(node, "Longitude")) || !rvn_map_parse_double(value, &longitude)) return R_FALSE;
	id = Rmap_view_add_marker(widget->id, latitude, longitude, rvn_xml_attribute(node, "Label"));
	index = rvn_map_marker_index(view, id);
	if (index < 0) return R_FALSE;
	if ((value = rvn_xml_attribute(node, "Color"))) {
		if (!rvn_parse_color(value, &view->markers[index].color)) return R_FALSE;
		view->markers[index].has_color = R_TRUE;
	}
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_map_view = {
	.type = R_WIDGET_MAP_VIEW,
	.name = "MapView",
	.size = sizeof(rvn_map_view_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_UI_PAINT,
	.init = rvn_map_view_init,
	.destroy = rvn_map_view_destroy,
	.measure = rvn_map_view_measure,
	.paint = rvn_map_view_paint,
	.event = rvn_map_view_event,
	.set_attribute = rvn_map_view_set_attribute,
	.xml_child = rvn_map_view_xml_child,
	.pan = rvn_map_view_pan,
	.zoom = rvn_map_view_zoom,
	.touch_drags = rvn_map_view_touch_drags
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

static rvn_map_view_t* rvn_map_check(Rid id, const char* function) {
	return (rvn_map_view_t*)rvn_widget_check(id, R_WIDGET_MAP_VIEW, function);
}

#define RVN_MAP(variable, id, result) \
	rvn_map_view_t* variable = rvn_map_check((id), __func__); \
	if (!variable) return result
#define RVN_MAP_VOID(variable, id) \
	rvn_map_view_t* variable = rvn_map_check((id), __func__); \
	if (!variable) return

Rid Rmap_view_create(void) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_MAP_VIEW);
	return widget ? widget->id : R_INVALID_ID;
}

void Rmap_view_set_center(Rid id, double latitude, double longitude) {
	RVN_MAP_VOID(view, id);
	if (!rvn_map_finite(latitude) || !rvn_map_finite(longitude)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rmap_view_set_center: not a number");
		return;
	}
	rvn_map_stop(view);
	view->cx = rvn_map_lon_to_x(longitude);
	view->cy = rvn_map_lat_to_y(latitude);
	rvn_map_clamp_center(view);
	rvn_widget_invalidate_paint(&view->base);
}

void Rmap_view_get_center(Rid id, double* latitude, double* longitude) {
	if (latitude) *latitude = 0.0;
	if (longitude) *longitude = 0.0;
	{
		RVN_MAP_VOID(view, id);
		if (latitude) *latitude = rvn_map_y_to_lat(view->cy);
		if (longitude) *longitude = rvn_map_x_to_lon(view->cx);
	}
}

void Rmap_view_set_zoom(Rid id, double zoom) {
	RVN_MAP_VOID(view, id);
	if (!rvn_map_finite(zoom)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rmap_view_set_zoom: not a number");
		return;
	}
	rvn_map_stop(view);
	view->zoom = rvn_map_clamp_zoom(view, zoom);
	rvn_widget_invalidate_paint(&view->base);
}

double Rmap_view_get_zoom(Rid id) {
	RVN_MAP(view, id, 0.0);
	return view->zoom;
}

void Rmap_view_set_zoom_range(Rid id, double min_zoom, double max_zoom) {
	RVN_MAP_VOID(view, id);
	if (!rvn_map_finite(min_zoom) || !rvn_map_finite(max_zoom) || min_zoom < 0.0 || max_zoom > RVN_MAP_LIMIT_ZOOM || min_zoom > max_zoom) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rmap_view_set_zoom_range: the range must lie within 0 .. 24, the smallest first");
		return;
	}
	view->min_zoom = min_zoom;
	view->max_zoom = max_zoom;
	view->zoom = rvn_map_clamp_zoom(view, view->zoom);
	if (view->anim != RVN_MAP_ANIM_NONE) view->anim_zoom = rvn_map_clamp_zoom(view, view->anim_zoom);
	rvn_widget_invalidate_paint(&view->base);
}

void Rmap_view_get_zoom_range(Rid id, double* min_zoom, double* max_zoom) {
	if (min_zoom) *min_zoom = 0.0;
	if (max_zoom) *max_zoom = 0.0;
	{
		RVN_MAP_VOID(view, id);
		if (min_zoom) *min_zoom = view->min_zoom;
		if (max_zoom) *max_zoom = view->max_zoom;
	}
}

void Rmap_view_animate_to(Rid id, double latitude, double longitude, double zoom) {
	RVN_MAP_VOID(view, id);
	if (!rvn_map_finite(latitude) || !rvn_map_finite(longitude) || !rvn_map_finite(zoom)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rmap_view_animate_to: not a number");
		return;
	}
	rvn_map_stop(view);
	view->fly_x = rvn_map_lon_to_x(longitude);
	view->fly_x -= floor(view->fly_x);
	view->fly_y = rvn_map_lat_to_y(latitude);
	view->anim_zoom = rvn_map_clamp_zoom(view, zoom);
	if (!Rget_animations_enabled()) {
		view->cx = view->fly_x;
		view->cy = view->fly_y;
		view->zoom = view->anim_zoom;
		rvn_widget_invalidate_paint(&view->base);
		rvn_map_changed(view);
		return;
	}
	view->anim = RVN_MAP_ANIM_FLY;
	rvn_widget_invalidate_paint(&view->base);
}

void Rmap_view_screen_to_latlon(Rid id, float x, float y, double* latitude, double* longitude) {
	if (latitude) *latitude = 0.0;
	if (longitude) *longitude = 0.0;
	{
		double wx, wy;
		RVN_MAP_VOID(view, id);
		rvn_map_point_to_world(view, x, y, &wx, &wy);
		if (latitude) *latitude = rvn_map_y_to_lat(wy < 0.0 ? 0.0 : wy > 1.0 ? 1.0 : wy);
		if (longitude) *longitude = rvn_map_x_to_lon(wx);
	}
}

void Rmap_view_latlon_to_screen(Rid id, double latitude, double longitude, float* x, float* y) {
	if (x) *x = 0.0f;
	if (y) *y = 0.0f;
	{
		float px, py;
		RVN_MAP_VOID(view, id);
		if (!rvn_map_finite(latitude) || !rvn_map_finite(longitude)) {
			rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rmap_view_latlon_to_screen: not a number");
			return;
		}
		rvn_map_world_to_point(view, rvn_map_lon_to_x(longitude), rvn_map_lat_to_y(latitude), &px, &py);
		if (x) *x = px;
		if (y) *y = py;
	}
}

void Rmap_view_set_tile_callback(Rid id, RMapTileCallback callback, void* user_data) {
	RVN_MAP_VOID(view, id);
	view->request_callback = callback;
	view->request_user = user_data;
	rvn_map_clear(view);
	rvn_widget_invalidate_paint(&view->base);
}

void Rmap_view_set_tile_cancel_callback(Rid id, RMapTileCallback callback, void* user_data) {
	RVN_MAP_VOID(view, id);
	view->cancel_callback = callback;
	view->cancel_user = user_data;
}

/* The checks of an answer: the tile exists at its zoom. */
static int rvn_map_check_tile(const char* function, int z, int x, int y) {
	if (z < 0 || z > (int)RVN_MAP_LIMIT_ZOOM || x < 0 || y < 0 || x >= (1 << z) || y >= (1 << z)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: there is no tile %d/%d/%d", function, z, x, y);
		return R_FALSE;
	}
	return R_TRUE;
}

/* An answer from another thread: applied on the UI thread. `pixels` is taken. */
static void rvn_map_post(Rid id, int z, int x, int y, uint8_t* pixels, int width, int height) {
	rvn_map_job_t* job = (rvn_map_job_t*)rvn_calloc(sizeof(rvn_map_job_t));
	if (!job) {
		rvn_free(pixels);
		return;
	}
	job->map = id;
	job->z = z;
	job->x = x;
	job->y = y;
	job->pixels = pixels;
	job->width = width;
	job->height = height;
	Rtask_post(rvn_map_job_finish, job);
}

void Rmap_view_set_tile(Rid id, int z, int x, int y, const uint8_t* rgba, int width, int height) {
	if (!rvn_map_check_tile(__func__, z, x, y)) return;
	if (!rgba || width <= 0 || height <= 0 || width > RVN_MAP_MAX_TILE_PIXELS || height > RVN_MAP_MAX_TILE_PIXELS) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rmap_view_set_tile: no pixels, or not 1 to 4096 pixels on a side");
		return;
	}
	if (!Ris_ui_thread()) {
		size_t bytes = (size_t)width * (size_t)height * 4;
		uint8_t* copy = (uint8_t*)rvn_alloc(bytes);
		if (!copy) return;
		memcpy(copy, rgba, bytes);
		rvn_map_post(id, z, x, y, copy, width, height);
		return;
	}
	{
		RVN_MAP_VOID(view, id);
		rvn_map_store(view, z, x, y, rgba, width, height);
	}
}

void Rmap_view_set_tile_data(Rid id, int z, int x, int y, const void* data, size_t data_size) {
	uint8_t* pixels;
	int width = 0, height = 0;
	if (!rvn_map_check_tile(__func__, z, x, y)) return;
	if (!data || data_size == 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rmap_view_set_tile_data: no data");
		return;
	}
	pixels = Rimage_decode(data, data_size, &width, &height);
	if (!pixels || width > RVN_MAP_MAX_TILE_PIXELS || height > RVN_MAP_MAX_TILE_PIXELS) {
		Rpixels_free(pixels);
		rvn_error(R_ERROR_INVALID_FILE_FORMAT, R_SEVERITY_ERROR, "Rmap_view_set_tile_data: tile %d/%d/%d is not an image Raven can read", z, x, y);
		pixels = NULL;
	}
	if (!Ris_ui_thread()) {
		rvn_map_post(id, z, x, y, pixels, width, height);
		return;
	}
	{
		rvn_map_view_t* view = rvn_map_check(id, __func__);
		if (view) rvn_map_store(view, z, x, y, pixels, width, height);
	}
	Rpixels_free(pixels);
}

void Rmap_view_set_tile_failed(Rid id, int z, int x, int y) {
	if (!rvn_map_check_tile(__func__, z, x, y)) return;
	if (!Ris_ui_thread()) {
		rvn_map_post(id, z, x, y, NULL, 0, 0);
		return;
	}
	{
		RVN_MAP_VOID(view, id);
		rvn_map_store(view, z, x, y, NULL, 0, 0);
	}
}

void Rmap_view_set_tile_path(Rid id, const char* path_template) {
	RVN_MAP_VOID(view, id);
	if (path_template && (strncmp(path_template, "http://", 7) == 0 || strncmp(path_template, "https://", 8) == 0)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rmap_view_set_tile_path: local files only (give the tiles of a server with the tile callback)");
		return;
	}
	rvn_str_set(&view->tile_path, path_template && path_template[0] ? path_template : NULL);
	rvn_map_clear(view);
	rvn_widget_invalidate_paint(&view->base);
}

const char* Rmap_view_get_tile_path(Rid id) {
	RVN_MAP(view, id, "");
	return view->tile_path ? view->tile_path : "";
}

void Rmap_view_set_tile_size(Rid id, int size) {
	RVN_MAP_VOID(view, id);
	if (size < 16 || size > 1024) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rmap_view_set_tile_size: %d is not within 16 .. 1024", size);
		return;
	}
	view->tile_size = size;
	rvn_widget_invalidate_paint(&view->base);
}

int Rmap_view_get_tile_size(Rid id) {
	RVN_MAP(view, id, 256);
	return view->tile_size;
}

void Rmap_view_set_cache_size(Rid id, size_t bytes) {
	RVN_MAP_VOID(view, id);
	view->cache_size = bytes;
	rvn_map_evict(view);
}

size_t Rmap_view_get_cache_usage(Rid id) {
	RVN_MAP(view, id, 0);
	return view->cache_bytes;
}

void Rmap_view_clear_tiles(Rid id) {
	RVN_MAP_VOID(view, id);
	rvn_map_clear(view);
	rvn_widget_invalidate_paint(&view->base);
}

RMapTileState Rmap_view_get_tile_state(Rid id, int z, int x, int y) {
	int index;
	RVN_MAP(view, id, R_MAP_TILE_NONE);
	if (z < 0 || z > (int)RVN_MAP_LIMIT_ZOOM) return R_MAP_TILE_NONE;
	index = rvn_map_find(view, z, x, y);
	return index >= 0 ? view->tiles[index].state : R_MAP_TILE_NONE;
}

int Rmap_view_add_marker(Rid id, double latitude, double longitude, const char* label) {
	rvn_map_marker_t* marker;
	RVN_MAP(view, id, -1);
	if (!rvn_map_finite(latitude) || !rvn_map_finite(longitude)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rmap_view_add_marker: not a number");
		return -1;
	}
	if (view->marker_count == view->marker_cap) {
		int cap = view->marker_cap ? view->marker_cap * 2 : 8;
		rvn_map_marker_t* grown = (rvn_map_marker_t*)rvn_realloc(view->markers, sizeof(rvn_map_marker_t) * (size_t)cap);
		if (!grown) {
			rvn_error(R_ERROR_OUT_OF_MEMORY, R_SEVERITY_ERROR, "Rmap_view_add_marker: out of memory");
			return -1;
		}
		view->markers = grown;
		view->marker_cap = cap;
	}
	marker = &view->markers[view->marker_count++];
	memset(marker, 0, sizeof(*marker));
	marker->id = view->next_marker++;
	marker->latitude = latitude;
	marker->longitude = longitude;
	marker->label = label && label[0] ? rvn_strdup(label) : NULL;
	rvn_a11y_structure_changed();
	rvn_widget_invalidate_paint(&view->base);
	return marker->id;
}

/* The index of a marker for a public function, or -1 after reporting it. */
static int rvn_map_marker_for(const rvn_map_view_t* view, int marker, const char* function) {
	int index = rvn_map_marker_index(view, marker);
	if (index < 0) rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: no marker %d", function, marker);
	return index;
}

void Rmap_view_remove_marker(Rid id, int marker) {
	int index;
	RVN_MAP_VOID(view, id);
	index = rvn_map_marker_for(view, marker, __func__);
	if (index < 0) return;
	rvn_free(view->markers[index].label);
	memmove(&view->markers[index], &view->markers[index + 1], sizeof(rvn_map_marker_t) * (size_t)(view->marker_count - index - 1));
	view->marker_count--;
	view->pressed_marker = -1;
	rvn_a11y_structure_changed();
	rvn_widget_invalidate_paint(&view->base);
}

void Rmap_view_clear_markers(Rid id) {
	RVN_MAP_VOID(view, id);
	rvn_map_free_markers(view);
	view->pressed_marker = -1;
	rvn_a11y_structure_changed();
	rvn_widget_invalidate_paint(&view->base);
}

int Rmap_view_get_marker_count(Rid id) {
	RVN_MAP(view, id, 0);
	return view->marker_count;
}

void Rmap_view_set_marker_position(Rid id, int marker, double latitude, double longitude) {
	int index;
	RVN_MAP_VOID(view, id);
	index = rvn_map_marker_for(view, marker, __func__);
	if (index < 0) return;
	if (!rvn_map_finite(latitude) || !rvn_map_finite(longitude)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rmap_view_set_marker_position: not a number");
		return;
	}
	view->markers[index].latitude = latitude;
	view->markers[index].longitude = longitude;
	rvn_widget_invalidate_paint(&view->base);
}

void Rmap_view_set_marker_label(Rid id, int marker, const char* label) {
	int index;
	RVN_MAP_VOID(view, id);
	index = rvn_map_marker_for(view, marker, __func__);
	if (index < 0) return;
	rvn_str_set(&view->markers[index].label, label && label[0] ? label : NULL);
	rvn_widget_invalidate_paint(&view->base);
}

void Rmap_view_set_marker_color(Rid id, int marker, float r, float g, float b, float a) {
	int index;
	RVN_MAP_VOID(view, id);
	index = rvn_map_marker_for(view, marker, __func__);
	if (index < 0) return;
	view->markers[index].color = rvn_rgba(r, g, b, a);
	view->markers[index].has_color = R_TRUE;
	rvn_widget_invalidate_paint(&view->base);
}

void Rmap_view_set_marker_clicked_callback(Rid id, RMapMarkerCallback callback, void* user_data) {
	RVN_MAP_VOID(view, id);
	view->marker_callback = callback;
	view->marker_user = user_data;
}

void Rmap_view_set_view_changed_callback(Rid id, RMapViewCallback callback, void* user_data) {
	RVN_MAP_VOID(view, id);
	view->view_callback = callback;
	view->view_user = user_data;
}

void Rmap_view_set_attribution(Rid id, const char* text) {
	RVN_MAP_VOID(view, id);
	rvn_str_set(&view->attribution, text && text[0] ? text : NULL);
	rvn_widget_invalidate_paint(&view->base);
}

const char* Rmap_view_get_attribution(Rid id) {
	RVN_MAP(view, id, "");
	return view->attribution ? view->attribution : "";
}
