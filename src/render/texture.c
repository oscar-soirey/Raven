/**
 * Raven - render/texture.c
 * Texture objects and the public texture API, including asynchronous
 * decoding on the job system.
 */

#include "renderer.h"
#include "../core/context.h"
#include "../core/fs.h"
#if defined(__EMSCRIPTEN__)
#include <emscripten/fetch.h>
#endif

void rvn_tasks_run(void); /* core/context.c */

rvn_texture_t* rvn_texture_get(Rid id) {
	return (rvn_texture_t*)rvn_pool_get(&g_rvn.textures, id);
}

rvn_texture_t* rvn_texture_register(void) {
	rvn_texture_t* texture = (rvn_texture_t*)rvn_calloc(sizeof(rvn_texture_t));
	texture->format = R_TEXTURE_RGBA8;
	texture->filter = R_FILTER_LINEAR;
	texture->id = rvn_pool_add(&g_rvn.textures, texture);
	if (texture->id == R_INVALID_ID) {
		rvn_free(texture);
		return NULL;
	}
	return texture;
}

/* Textures up to this many pixels keep a copy of their pixels, to be made again after the loss of the device. */
#define RVN_TEXTURE_KEEP_PIXELS (256 * 256)
/* Encoded files up to this size are kept for the same reason (Rtexture_create from memory). */
#define RVN_TEXTURE_KEEP_ENCODED ((size_t)4 * 1024 * 1024)

/* Keeps (or forgets) the copy of the pixels of a small RGBA8 texture. */
static void rvn_texture_keep_pixels(rvn_texture_t* texture, int width, int height, const void* pixels) {
	size_t size = (size_t)width * (size_t)height * 4;
	if (!pixels || texture->format != R_TEXTURE_RGBA8 || texture->external || (size_t)width * (size_t)height > RVN_TEXTURE_KEEP_PIXELS) {
		rvn_free(texture->kept_pixels);
		texture->kept_pixels = NULL;
		return;
	}
	texture->kept_pixels = (uint8_t*)rvn_realloc(texture->kept_pixels, size);
	if (texture->kept_pixels) memcpy(texture->kept_pixels, pixels, size);
}

static void rvn_texture_keep_source(rvn_texture_t* texture, const char* path, const void* data, size_t size) {
	if (path) texture->source_path = rvn_strdup(path);
	if (data && size > 0 && size <= RVN_TEXTURE_KEEP_ENCODED) {
		texture->source_data = (uint8_t*)rvn_alloc(size);
		memcpy(texture->source_data, data, size);
		texture->source_size = size;
	}
}

static void rvn_texture_free(rvn_texture_t* texture) {
	rvn_free(texture->source_path);
	rvn_free(texture->source_data);
	rvn_free(texture->kept_pixels);
	rvn_free(texture);
}

/*
 * The filter given to the backend: a backend supplied by the application was
 * written for the filters it knew, and R_FILTER_MIPMAP came later.
 */
static RFilterType rvn_texture_backend_filter(RFilterType filter) {
	if (filter == R_FILTER_MIPMAP && g_rvn.backend == R_BACKEND_CUSTOM) return R_FILTER_LINEAR;
	return filter;
}

/* Sends the pixels to the renderer. Takes ownership of `pixels`. */
static void rvn_texture_upload(rvn_texture_t* texture, int width, int height, void* pixels) {
	RTextureDesc desc;
	if (texture->created) rvn_renderer_destroy_texture(texture->id);
	texture->width = width;
	texture->height = height;
	desc.width = (Ruint)width;
	desc.height = (Ruint)height;
	desc.format = texture->format;
	desc.filter = rvn_texture_backend_filter(texture->filter);
	rvn_texture_keep_pixels(texture, width, height, pixels);
	rvn_renderer_create_texture(texture->id, &desc, pixels);
	texture->created = R_TRUE;
	rvn_atomic_store(&texture->ready, 1);
}

Rid rvn_texture_create_raw(int width, int height, RTextureFormat format, RFilterType filter, void* pixels) {
	rvn_texture_t* texture;
	if (width <= 0 || height <= 0 || width > 16384 || height > 16384) {
		rvn_free(pixels);
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "invalid texture size %d x %d", width, height);
		return R_INVALID_ID;
	}
	texture = rvn_texture_register();
	if (!texture) {
		rvn_free(pixels);
		return R_INVALID_ID;
	}
	texture->format = format;
	texture->filter = filter;
	rvn_texture_upload(texture, width, height, pixels);
	return texture->id;
}

void rvn_textures_shutdown(void) {
	uint32_t i;
	for (i = 0; i < g_rvn.textures.used; i++) {
		rvn_texture_t* texture = (rvn_texture_t*)rvn_pool_at(&g_rvn.textures, i);
		if (!texture) continue;
		if (texture->created) rvn_renderer_destroy_texture(texture->id);
		rvn_pool_remove(&g_rvn.textures, texture->id);
		rvn_texture_free(texture);
	}
}

/*
 * After the loss of the device: the backend texture is gone; makes it again
 * from what the texture kept, or transparent with the same size.
 */
void rvn_texture_restore(rvn_texture_t* texture) {
	uint8_t* pixels = NULL;
	int width = texture->width, height = texture->height;
	texture->created = R_FALSE;
	if (texture->external || width <= 0 || height <= 0) return;
	if (texture->kept_pixels) {
		size_t size = (size_t)width * (size_t)height * 4;
		pixels = (uint8_t*)rvn_alloc(size);
		memcpy(pixels, texture->kept_pixels, size);
	} else if (texture->source_data || texture->source_path) {
		int w = 0, h = 0;
		if (texture->source_data) {
			pixels = rvn_image_decode_at(texture->source_data, texture->source_size, texture->density, &w, &h, NULL);
		} else if (!rvn_is_http_url(texture->source_path)) {
			size_t size = 0;
			char* data = rvn_read_file(texture->source_path, &size);
			if (data) pixels = rvn_image_decode_at((const uint8_t*)data, size, texture->density, &w, &h, NULL);
			rvn_free(data);
		}
		/* (the file changed size meanwhile: what was shown keeps its size, transparent) */
		if (pixels && (w != width || h != height)) {
			rvn_free(pixels);
			pixels = NULL;
		}
	}
	/* no pixels: the backend makes it transparent */
	rvn_texture_upload(texture, width, height, pixels);
}

/* ---------------------------------------------------------------------------
 *  Asynchronous decoding
 * ------------------------------------------------------------------------- */

typedef struct rvn_texture_job_t {
	Rid id;
	uint8_t* data;
	size_t size;
	char* path;
	uint8_t* pixels;
	int width, height;
	float density;
	/** The scale an SVG is drawn at (0: RVN_SVG_DENSITY); R_TRUE when the file is an SVG. */
	float svg_scale;
	int svg;
	/** Drawing an SVG again at another scale (rvn_textures_update_svg), not making the texture. */
	int redraw;
} rvn_texture_job_t;

/* UI thread: the decoded pixels become the texture, unless it was deleted meanwhile. */
static void rvn_texture_job_finish(void* arg) {
	rvn_texture_job_t* job = (rvn_texture_job_t*)arg;
	rvn_texture_t* texture = g_rvn.initialized ? rvn_texture_get(job->id) : NULL;
	if (job->redraw) {
		/* (a failure leaves the pixels it had: they still show the image) */
		if (texture) texture->svg_pending = R_FALSE;
		if (texture && job->pixels && texture->svg_auto) {
			texture->density = job->density;
			rvn_texture_upload(texture, job->width, job->height, job->pixels);
			job->pixels = NULL;
			rvn_invalidate_all_windows();
		}
	} else if (texture && job->pixels) {
		texture->density = job->density;
		texture->svg_auto = job->svg;
		rvn_texture_upload(texture, job->width, job->height, job->pixels);
		job->pixels = NULL;
		rvn_invalidate_all_windows();
	} else if (texture) {
		rvn_atomic_store(&texture->ready, 1);
		rvn_error(R_ERROR_INVALID_FILE_FORMAT, R_SEVERITY_ERROR, "asynchronous texture: cannot decode %s",
			job->path ? job->path : "the image");
	}
	rvn_free(job->pixels);
	rvn_free(job->path);
	rvn_free(job);
}

/* Worker thread. */
static void rvn_texture_job_run(void* arg) {
	rvn_texture_job_t* job = (rvn_texture_job_t*)arg;
	if (job->path) {
		if (rvn_is_http_url(job->path)) {
			size_t downloaded_size = 0;
			uint8_t* downloaded = NULL;
			if (rvn_fs_download_url(job->path, &downloaded, &downloaded_size)) {
				job->data = downloaded;
				job->size = downloaded_size;
			}
		} else job->data = (uint8_t*)rvn_read_file(job->path, &job->size);
	}
	if (job->data) {
		job->svg = rvn_svg_detect(job->data, job->size);
		job->pixels = rvn_image_decode_at(job->data, job->size, job->svg_scale, &job->width, &job->height, &job->density);
	}
	rvn_free(job->data);
	job->data = NULL;
	Rtask_post(rvn_texture_job_finish, job);
}

#if defined(__EMSCRIPTEN__)
typedef struct rvn_web_image_request_t {
    emscripten_fetch_t* fetch;
    rvn_texture_job_t* job;
    struct rvn_web_image_request_t* next;
} rvn_web_image_request_t;
static rvn_web_image_request_t* g_image_requests;

static void rvn_web_image_complete(emscripten_fetch_t* fetch, int success) {
    rvn_web_image_request_t* request = fetch->userData;
    rvn_web_image_request_t** link = &g_image_requests;
    rvn_texture_job_t* job = request->job;
    while (*link && *link != request) link = &(*link)->next;
    if (*link) *link = request->next;
    if (success && fetch->status >= 200 && fetch->status < 300 &&
        fetch->numBytes > 0 && fetch->numBytes <= 33554432) {
        rvn_texture_t* texture = rvn_texture_get(job->id);
        job->svg = rvn_svg_detect((const uint8_t*)fetch->data,
                                  (size_t)fetch->numBytes);
        job->pixels = rvn_image_decode_at((const uint8_t*)fetch->data,
            (size_t)fetch->numBytes, job->svg_scale,
            &job->width, &job->height, &job->density);
        if (texture && job->pixels) {
            /* Recovery and SVG rescaling must not need synchronous networking. */
            rvn_free(texture->source_path); texture->source_path = NULL;
            rvn_free(texture->source_data);
            texture->source_size = (size_t)fetch->numBytes;
            texture->source_data = rvn_alloc(texture->source_size);
            memcpy(texture->source_data, fetch->data, texture->source_size);
        }
    }
    emscripten_fetch_close(fetch);
    rvn_free(request);
    rvn_texture_job_finish(job);
}

static void rvn_web_image_done(emscripten_fetch_t* fetch) {
    rvn_web_image_complete(fetch, R_TRUE);
}
static void rvn_web_image_progress(emscripten_fetch_t* fetch) {
    const uint64_t limit = 33554432;
    if (fetch->totalBytes > limit || fetch->dataOffset > limit ||
        fetch->numBytes > limit - fetch->dataOffset)
        rvn_web_image_complete(fetch, R_FALSE);
}

void rvn_web_images_shutdown(void) {
    while (g_image_requests) {
        rvn_web_image_request_t* request = g_image_requests;
        g_image_requests = request->next;
        emscripten_fetch_close(request->fetch);
        rvn_free(request->job->path);
        rvn_free(request->job);
        rvn_free(request);
    }
}

static void rvn_web_image_start(rvn_texture_job_t* job) {
    emscripten_fetch_attr_t attrs;
    rvn_web_image_request_t* request = rvn_calloc(sizeof(*request));
    request->job = job;
    request->next = g_image_requests;
    g_image_requests = request;
    emscripten_fetch_attr_init(&attrs);
    strcpy(attrs.requestMethod, "GET");
    attrs.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attrs.timeoutMSecs = 20000;
    attrs.onsuccess = rvn_web_image_done;
    attrs.onerror = rvn_web_image_done;
    attrs.onprogress = rvn_web_image_progress;
    attrs.userData = request;
    request->fetch = emscripten_fetch(&attrs, job->path);
    if (!request->fetch) {
        g_image_requests = request->next;
        rvn_free(request);
        rvn_texture_job_finish(job);
    }
}
#endif

static Rid rvn_texture_create_async(const char* data, size_t size, const char* path) {
	rvn_texture_t* texture = rvn_texture_register();
	rvn_texture_job_t* job;
	if (!texture) return R_INVALID_ID;
	/* a picture from a file: it may well be shown smaller than it is */
	texture->filter = R_FILTER_MIPMAP;
	job = (rvn_texture_job_t*)rvn_calloc(sizeof(rvn_texture_job_t));
	job->id = texture->id;
	rvn_texture_keep_source(texture, path, data, size);
	if (path) {
		job->path = rvn_strdup(path);
	} else {
		job->data = (uint8_t*)rvn_alloc(size);
		memcpy(job->data, data, size);
		job->size = size;
	}
#if defined(__EMSCRIPTEN__)
    if (path && rvn_is_http_url(path)) rvn_web_image_start(job);
    else
#endif
    rvn_jobs_submit(NULL, rvn_texture_job_run, job);
    return texture->id;
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

/* An SVG drawn at the size it is shown keeps its file, to be drawn again. */
static void rvn_texture_set_svg_auto(rvn_texture_t* texture, const char* path, const char* data, size_t data_size) {
	texture->svg_auto = R_TRUE;
	if (!texture->source_path && !texture->source_data) rvn_texture_keep_source(texture, path, data, data_size);
	/* (a file too big to keep in memory, and not read from a path: it stays at its first scale) */
	if (!texture->source_path && !texture->source_data) texture->svg_auto = R_FALSE;
}

/* An image file in memory; an SVG drawn at `svg_scale` (0: at the size it is shown). */
static Rid rvn_texture_create_decoded(const char* data, size_t data_size, float svg_scale, const char* function) {
	int width = 0, height = 0;
	float density = 1.0f;
	uint8_t* pixels = rvn_image_decode_at((const uint8_t*)data, data_size, svg_scale, &width, &height, &density);
	if (!pixels) {
		rvn_error(R_ERROR_INVALID_FILE_FORMAT, R_SEVERITY_ERROR, "%s: cannot decode the image", function);
		return R_INVALID_ID;
	}
	{
		Rid id = rvn_texture_create_raw(width, height, R_TEXTURE_RGBA8, R_FILTER_MIPMAP, pixels);
		rvn_texture_t* texture = rvn_texture_get(id);
		if (texture) texture->density = density;
		if (texture && svg_scale <= 0.0f && rvn_svg_detect((const uint8_t*)data, data_size)) rvn_texture_set_svg_auto(texture, NULL, data, data_size);
		else if (texture && !texture->kept_pixels) rvn_texture_keep_source(texture, NULL, data, data_size);
		return id;
	}
}

Rid Rtexture_create(const char* data, size_t data_size) {
	RVN_API_GUARD(R_INVALID_ID);
	return rvn_texture_create_decoded(data, data_size, 0.0f, "Rtexture_create");
}

Rid Rtexture_create_from_svg(const char* svg, size_t svg_size, float scale) {
	RVN_API_GUARD(R_INVALID_ID);
	if (!svg || !rvn_svg_detect((const uint8_t*)svg, svg_size)) {
		rvn_error(R_ERROR_INVALID_FILE_FORMAT, R_SEVERITY_ERROR, "Rtexture_create_from_svg: not an SVG image");
		return R_INVALID_ID;
	}
	return rvn_texture_create_decoded(svg, svg_size, scale > 0.0f ? scale : 0.0f, "Rtexture_create_from_svg");
}

float Rtexture_get_density(Rid id) {
	rvn_texture_t* texture = g_rvn.initialized ? rvn_texture_get(id) : NULL;
	return texture && texture->density > 0.0f ? texture->density : 1.0f;
}

Rid Rtexture_create_async(const char* data, size_t data_size) {
	RVN_API_GUARD(R_INVALID_ID);
	if (!data || data_size == 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtexture_create_async: empty buffer");
		return R_INVALID_ID;
	}
	return rvn_texture_create_async(data, data_size, NULL);
}


Rid rvn_texture_create_from_url_async(const char* url) {
	RVN_API_GUARD(R_INVALID_ID);
	if (!url || !rvn_is_http_url(url)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtexture_create_from_url_async: URL must use http:// or https://");
		return R_INVALID_ID;
	}
	return rvn_texture_create_async(NULL, 0, url);
}

Rid Rtexture_create_from_file(const char* path) {
	size_t size = 0;
	char* data;
	int width = 0, height = 0;
	float density = 1.0f;
	uint8_t* pixels;
	int svg;
	RVN_API_GUARD(R_INVALID_ID);
	data = rvn_read_file(path, &size);
	if (!data) {
		rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "Rtexture_create_from_file: cannot read '%s'", path ? path : "");
		return R_INVALID_ID;
	}
	pixels = rvn_image_decode_at((const uint8_t*)data, size, 0.0f, &width, &height, &density);
	svg = rvn_svg_detect((const uint8_t*)data, size);
	rvn_free(data);
	if (!pixels) {
		rvn_error(R_ERROR_INVALID_FILE_FORMAT, R_SEVERITY_ERROR, "Rtexture_create_from_file: cannot decode '%s'", path);
		return R_INVALID_ID;
	}
	{
		Rid id = rvn_texture_create_raw(width, height, R_TEXTURE_RGBA8, R_FILTER_MIPMAP, pixels);
		rvn_texture_t* texture = rvn_texture_get(id);
		if (texture) texture->density = density;
		if (texture && svg) rvn_texture_set_svg_auto(texture, path, NULL, 0);
		else if (texture && !texture->kept_pixels) rvn_texture_keep_source(texture, path, NULL, 0);
		return id;
	}
}

Rid Rtexture_create_from_file_async(const char* path) {
	RVN_API_GUARD(R_INVALID_ID);
	if (!path) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtexture_create_from_file_async: NULL path");
		return R_INVALID_ID;
	}
	return rvn_texture_create_async(NULL, 0, path);
}

Rid Rtexture_create_from_pixels(const uint8_t* rgba, int width, int height) {
	uint8_t* copy;
	size_t size;
	RVN_API_GUARD(R_INVALID_ID);
	if (!rgba || width <= 0 || height <= 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtexture_create_from_pixels: invalid arguments");
		return R_INVALID_ID;
	}
	size = (size_t)width * (size_t)height * 4;
	copy = (uint8_t*)rvn_alloc(size);
	memcpy(copy, rgba, size);
	return rvn_texture_create_raw(width, height, R_TEXTURE_RGBA8, R_FILTER_LINEAR, copy);
}

void Rtexture_update(Rid id, const uint8_t* rgba, int width, int height) {
	rvn_texture_t* texture;
	uint8_t* copy;
	size_t size;
	RVN_API_GUARD_VOID();
	texture = rvn_texture_get(id);
	if (!texture) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rtexture_update: invalid texture");
		return;
	}
	if (texture->external || texture->format != R_TEXTURE_RGBA8) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rtexture_update: this texture cannot be updated");
		return;
	}
	if (!rgba || width <= 0 || height <= 0 || width > 16384 || height > 16384) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtexture_update: invalid arguments");
		return;
	}
	size = (size_t)width * (size_t)height * 4;
	copy = (uint8_t*)rvn_alloc(size);
	memcpy(copy, rgba, size);
	texture->density = 1.0f;
	texture->svg_auto = R_FALSE;
	/* the pixels come from the application now: the file is no longer what it shows */
	rvn_free(texture->source_path);
	rvn_free(texture->source_data);
	texture->source_path = NULL;
	texture->source_data = NULL;
	texture->source_size = 0;
	if (texture->created && texture->width == width && texture->height == height) {
		rvn_texture_keep_pixels(texture, width, height, copy);
		rvn_renderer_update_texture(id, 0, 0, (uint32_t)width, (uint32_t)height, copy);
		rvn_repaint_all_windows();
	} else {
		/* widgets sized after the texture must be measured again */
		rvn_texture_upload(texture, width, height, copy);
		rvn_invalidate_all_windows();
	}
}

int Rtexture_is_ready(Rid id) {
	rvn_texture_t* texture = g_rvn.initialized ? rvn_texture_get(id) : NULL;
	return texture ? rvn_atomic_load(&texture->ready) != 0 : R_FALSE;
}

void Rtexture_wait(Rid id) {
	RVN_API_GUARD_VOID();
	for (;;) {
		rvn_texture_t* texture = rvn_texture_get(id);
		if (!texture || rvn_atomic_load(&texture->ready)) return;
		/* the result is delivered by a task posted to this thread */
		rvn_tasks_run();
		texture = rvn_texture_get(id);
        if (!texture || rvn_atomic_load(&texture->ready)) return;
#if defined(__EMSCRIPTEN__)
        rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR,
                  "Rtexture_wait cannot block the browser: poll Rtexture_is_ready");
        return;
#else
        rvn_sleep_ms(1);
#endif
	}
}

void Rtexture_destroy(Rid id) {
	rvn_texture_t* texture;
	RVN_API_GUARD_VOID();
	texture = rvn_texture_get(id);
	if (!texture) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rtexture_destroy: invalid texture");
		return;
	}
	if (texture->created) rvn_renderer_destroy_texture(id);
	/* the image behind an external texture belongs to the application, which is
	 * about to destroy it: return only once the backend no longer uses it */
	if (texture->external) rvn_renderer_sync();
	rvn_pool_remove(&g_rvn.textures, id);
	rvn_texture_free(texture);
	rvn_invalidate_all_windows();
}

int Rtexture_is_valid(Rid id) {
	return g_rvn.initialized && rvn_texture_get(id) != NULL;
}

void Rtexture_get_size(Rid id, int* width, int* height) {
	rvn_texture_t* texture = g_rvn.initialized ? rvn_texture_get(id) : NULL;
	if (width) *width = texture ? texture->width : 0;
	if (height) *height = texture ? texture->height : 0;
}

void Rtexture_set_filter(Rid id, RFilterType filter) {
	rvn_texture_t* texture;
	RVN_API_GUARD_VOID();
	texture = rvn_texture_get(id);
	if (!texture) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rtexture_set_filter: invalid texture");
		return;
	}
	if (filter != R_FILTER_LINEAR && filter != R_FILTER_NEAREST && filter != R_FILTER_MIPMAP) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtexture_set_filter: invalid filter");
		return;
	}
	if (texture->filter == filter) return;
	texture->filter = filter;
	if (texture->created) rvn_renderer_set_texture_filter(id, rvn_texture_backend_filter(filter));
	rvn_repaint_all_windows();
}

uint8_t* Rimage_decode(const void* data, size_t data_size, int* width, int* height) {
	int w = 0, h = 0;
	uint8_t* pixels = rvn_image_decode((const uint8_t*)data, data_size, &w, &h);
	if (width) *width = pixels ? w : 0;
	if (height) *height = pixels ? h : 0;
	return pixels;
}

/* ---------------------------------------------------------------------------
 *  SVGs drawn at the size they are shown
 * ------------------------------------------------------------------------- */

/* Scales an SVG is drawn at: quarters of a pixel per unit, from 1/4 to 16. */
static float rvn_svg_quantize(float scale) {
	return rvn_clampf(ceilf(scale * 4.0f - 0.01f) / 4.0f, 0.25f, 16.0f);
}

/* The biggest side an SVG is drawn with, in pixels. */
#define RVN_SVG_MAX_PIXELS 4096.0f

void rvn_textures_update_svg(void) {
	uint32_t i;
	for (i = 0; i < g_rvn.textures.used; i++) {
		rvn_texture_t* texture = (rvn_texture_t*)rvn_pool_at(&g_rvn.textures, i);
		float density, wanted, target, logical;
		rvn_texture_job_t* job;
		if (!texture || !texture->svg_auto) continue;
		wanted = texture->svg_wanted;
		texture->svg_wanted = 0.0f;
		/* (not shown this frame, or already being drawn again) */
		if (wanted <= 0.0f || texture->svg_pending || !texture->created) continue;
		density = texture->density > 0.0f ? texture->density : 1.0f;
		/* blurred when shown bigger than its pixels; drawn again smaller only when it is far too big */
		if (wanted <= density * 1.01f && wanted >= density * 0.6f) continue;
		target = rvn_svg_quantize(wanted);
		logical = rvn_maxf((float)texture->width, (float)texture->height) / density;
		if (logical > 0.0f && logical * target > RVN_SVG_MAX_PIXELS) target = rvn_maxf(0.25f, floorf(RVN_SVG_MAX_PIXELS / logical * 4.0f) / 4.0f);
		if (fabsf(target - density) < 0.01f) continue;
		job = (rvn_texture_job_t*)rvn_calloc(sizeof(rvn_texture_job_t));
		job->id = texture->id;
		job->redraw = R_TRUE;
		job->svg_scale = target;
		if (texture->source_data) {
			job->data = (uint8_t*)rvn_alloc(texture->source_size);
			memcpy(job->data, texture->source_data, texture->source_size);
			job->size = texture->source_size;
		} else {
			job->path = rvn_strdup(texture->source_path);
		}
		texture->svg_pending = R_TRUE;
		rvn_jobs_submit(NULL, rvn_texture_job_run, job);
	}
}
