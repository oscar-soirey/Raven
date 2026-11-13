/**
 * Raven - widgets/pdf_view.c
 * PdfView: the pages of a PDF document one under the other, scrolled and
 * zoomed (fit to the width, to the page, or a zoom of its own).
 *
 * The pages are drawn by PDFium, the PDF library of Chromium (BSD-3 /
 * Apache-2.0), opened when the first document is loaded: nothing of it is
 * needed to build Raven, and a program that shows no PDF never loads it.
 * Where it is not found, the view says so and Rpdf_is_available returns
 * R_FALSE.
 *
 * Only the pages that are seen are drawn, at the pixels of the screen; a
 * page that would make a huge texture (a strong zoom) is drawn only around
 * the part that is seen. While a page is drawn again (after a zoom), the
 * texture it had is shown stretched; the textures of pages far from the
 * view are given back.
 *
 * PDFium is called from the UI thread only (it is not thread-safe): the
 * class paints on the UI thread (RVN_CF_UI_PAINT).
 */

#if !defined(_WIN32) && !defined(__APPLE__)
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE /* readlink */
#endif
#endif

#include "common.h"
#include "../render/renderer.h"
#include "../xml/xml.h"

#include <limits.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif !defined(__EMSCRIPTEN__)
#include <dlfcn.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#endif

/* ---------------------------------------------------------------------------
 *  PDFium, opened at run time (the few functions of fpdfview.h, fpdf_doc.h
 *  and fpdf_text.h that are used; their headers are not needed)
 * ------------------------------------------------------------------------- */

#if defined(_WIN32) && !defined(_WIN64)
#define RVN_PDF_CALL __stdcall
#else
#define RVN_PDF_CALL
#endif

#define RVN_FPDF_ANNOT 0x01
#define RVN_FPDF_REVERSE_BYTE_ORDER 0x10
#define RVN_FPDF_ERR_PASSWORD 4
#define RVN_FPDF_BITMAP_BGRA 4

typedef struct rvn_pdfium_t {
	int tried;
	int ready;
	void* library;
	void (RVN_PDF_CALL *init_library)(void);
	void (RVN_PDF_CALL *destroy_library)(void);
	void* (RVN_PDF_CALL *load_mem_document)(const void* data, int size, const char* password);
	unsigned long (RVN_PDF_CALL *get_last_error)(void);
	void (RVN_PDF_CALL *close_document)(void* document);
	int (RVN_PDF_CALL *get_page_count)(void* document);
	int (RVN_PDF_CALL *get_page_size_by_index)(void* document, int index, double* width, double* height);
	void* (RVN_PDF_CALL *load_page)(void* document, int index);
	void (RVN_PDF_CALL *close_page)(void* page);
	void* (RVN_PDF_CALL *bitmap_create_ex)(int width, int height, int format, void* first_scan, int stride);
	int (RVN_PDF_CALL *bitmap_fill_rect)(void* bitmap, int left, int top, int width, int height, unsigned long color);
	void (RVN_PDF_CALL *bitmap_destroy)(void* bitmap);
	void (RVN_PDF_CALL *render_page_bitmap)(void* bitmap, void* page, int x, int y, int width, int height, int rotate, int flags);
	unsigned long (RVN_PDF_CALL *get_meta_text)(void* document, const char* tag, void* buffer, unsigned long length);
	void* (RVN_PDF_CALL *text_load_page)(void* page);
	void (RVN_PDF_CALL *text_close_page)(void* text_page);
	int (RVN_PDF_CALL *text_count_chars)(void* text_page);
	int (RVN_PDF_CALL *text_get_text)(void* text_page, int start, int count, unsigned short* result);
} rvn_pdfium_t;

static rvn_pdfium_t g_pdfium;

#if defined(__EMSCRIPTEN__)

static void* rvn_pdf_open(const char* path) {
	RVN_UNUSED(path);
	return NULL;
}
static void* rvn_pdf_symbol(void* library, const char* name) {
	RVN_UNUSED(library);
	RVN_UNUSED(name);
	return NULL;
}
static void rvn_pdf_unload(void* library) {
	RVN_UNUSED(library);
}
static int rvn_pdf_env(char* out, size_t cap) {
	RVN_UNUSED(out);
	RVN_UNUSED(cap);
	return R_FALSE;
}
static int rvn_pdf_program_dir(char* out, size_t cap) {
	RVN_UNUSED(out);
	RVN_UNUSED(cap);
	return R_FALSE;
}
#define RVN_PDFIUM_NAME "libpdfium.so"

#elif defined(_WIN32)

static void* rvn_pdf_open(const char* path) {
	wchar_t wide[1024];
	if (!MultiByteToWideChar(CP_UTF8, 0, path, -1, wide, (int)RVN_COUNT_OF(wide))) return NULL;
	return (void*)LoadLibraryW(wide);
}
static void* rvn_pdf_symbol(void* library, const char* name) {
	return (void*)(uintptr_t)GetProcAddress((HMODULE)library, name);
}
static void rvn_pdf_unload(void* library) {
	FreeLibrary((HMODULE)library);
}
/* RAVEN_PDFIUM in UTF-8 (getenv gives the code page of the system). */
static int rvn_pdf_env(char* out, size_t cap) {
	wchar_t wide[1024];
	DWORD length = GetEnvironmentVariableW(L"RAVEN_PDFIUM", wide, (DWORD)RVN_COUNT_OF(wide));
	if (length == 0 || length >= RVN_COUNT_OF(wide)) return R_FALSE;
	return WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, (int)cap, NULL, NULL) > 0;
}
static int rvn_pdf_program_dir(char* out, size_t cap) {
	wchar_t wide[1024];
	DWORD length = GetModuleFileNameW(NULL, wide, (DWORD)RVN_COUNT_OF(wide));
	char* slash;
	if (length == 0 || length >= RVN_COUNT_OF(wide)) return R_FALSE;
	if (!WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, (int)cap, NULL, NULL)) return R_FALSE;
	slash = strrchr(out, '\\');
	if (!slash) return R_FALSE;
	slash[1] = '\0';
	return R_TRUE;
}
#define RVN_PDFIUM_NAME "pdfium.dll"

#else

static void* rvn_pdf_open(const char* path) {
	return dlopen(path, RTLD_NOW | RTLD_LOCAL);
}
static void* rvn_pdf_symbol(void* library, const char* name) {
	return dlsym(library, name);
}
static void rvn_pdf_unload(void* library) {
	dlclose(library);
}
static int rvn_pdf_env(char* out, size_t cap) {
	const char* value = getenv("RAVEN_PDFIUM");
	size_t length = value ? strlen(value) : 0;
	if (length == 0 || length >= cap) return R_FALSE;
	memcpy(out, value, length + 1);
	return R_TRUE;
}
static int rvn_pdf_program_dir(char* out, size_t cap) {
	char* slash;
#if defined(__APPLE__)
	uint32_t size = (uint32_t)cap;
	if (_NSGetExecutablePath(out, &size) != 0) return R_FALSE;
#else
	ssize_t length = readlink("/proc/self/exe", out, cap - 1);
	if (length <= 0) return R_FALSE;
	out[length] = '\0';
#endif
	slash = strrchr(out, '/');
	if (!slash) return R_FALSE;
	slash[1] = '\0';
	return R_TRUE;
}
#if defined(__APPLE__)
#define RVN_PDFIUM_NAME "libpdfium.dylib"
#else
#define RVN_PDFIUM_NAME "libpdfium.so"
#endif

#endif

/* Opens PDFium once: the file named by RAVEN_PDFIUM, else next to the program, else where the system looks. */
static int rvn_pdfium_load(void) {
	static const char name[] = RVN_PDFIUM_NAME;
	char path[4096];
	void* library = NULL;
	int ok = R_TRUE;

	if (g_pdfium.tried) return g_pdfium.ready;
	g_pdfium.tried = R_TRUE;

	if (rvn_pdf_env(path, sizeof(path))) {
		/* a path named on purpose: nothing else is tried */
		library = rvn_pdf_open(path);
	} else {
		if (rvn_pdf_program_dir(path, sizeof(path) - sizeof(name))) {
			memcpy(path + strlen(path), name, sizeof(name));
			library = rvn_pdf_open(path);
		}
		if (!library) library = rvn_pdf_open(name);
	}
	if (!library) return R_FALSE;

#define RVN_PDF_LOAD(field, symbol) \
	*(void**)(&g_pdfium.field) = rvn_pdf_symbol(library, symbol); \
	if (!g_pdfium.field) ok = R_FALSE
	RVN_PDF_LOAD(init_library, "FPDF_InitLibrary");
	RVN_PDF_LOAD(destroy_library, "FPDF_DestroyLibrary");
	RVN_PDF_LOAD(load_mem_document, "FPDF_LoadMemDocument");
	RVN_PDF_LOAD(get_last_error, "FPDF_GetLastError");
	RVN_PDF_LOAD(close_document, "FPDF_CloseDocument");
	RVN_PDF_LOAD(get_page_count, "FPDF_GetPageCount");
	RVN_PDF_LOAD(get_page_size_by_index, "FPDF_GetPageSizeByIndex");
	RVN_PDF_LOAD(load_page, "FPDF_LoadPage");
	RVN_PDF_LOAD(close_page, "FPDF_ClosePage");
	RVN_PDF_LOAD(bitmap_create_ex, "FPDFBitmap_CreateEx");
	RVN_PDF_LOAD(bitmap_fill_rect, "FPDFBitmap_FillRect");
	RVN_PDF_LOAD(bitmap_destroy, "FPDFBitmap_Destroy");
	RVN_PDF_LOAD(render_page_bitmap, "FPDF_RenderPageBitmap");
	RVN_PDF_LOAD(get_meta_text, "FPDF_GetMetaText");
	RVN_PDF_LOAD(text_load_page, "FPDFText_LoadPage");
	RVN_PDF_LOAD(text_close_page, "FPDFText_ClosePage");
	RVN_PDF_LOAD(text_count_chars, "FPDFText_CountChars");
	RVN_PDF_LOAD(text_get_text, "FPDFText_GetText");
#undef RVN_PDF_LOAD
	if (!ok) {
		rvn_error(R_ERROR_PLATFORM, R_SEVERITY_WARNING, "PdfView: the PDFium library found lacks functions; it is not used");
		rvn_pdf_unload(library);
		memset(&g_pdfium, 0, sizeof(g_pdfium));
		g_pdfium.tried = R_TRUE;
		return R_FALSE;
	}
	g_pdfium.library = library;
	g_pdfium.init_library();
	g_pdfium.ready = R_TRUE;
	return R_TRUE;
}

/** Rshutdown: closes PDFium (the views, and their documents, are gone by then). */
void rvn_pdf_shutdown(void) {
	if (g_pdfium.ready) {
		g_pdfium.destroy_library();
		rvn_pdf_unload(g_pdfium.library);
	}
	memset(&g_pdfium, 0, sizeof(g_pdfium));
}

int Rpdf_is_available(void) {
	return rvn_pdfium_load();
}

/* UTF-16 (PDFium's text) to a new UTF-8 string, `count` units or up to a 0. */
static char* rvn_pdf_utf16_to_utf8(const unsigned short* units, int count) {
	char* text = (char*)rvn_alloc((size_t)count * 3 + 1);
	int i, length = 0;
	if (!text) return NULL;
	for (i = 0; i < count && units[i]; i++) {
		uint32_t code = units[i];
		if (code >= 0xD800 && code < 0xDC00 && i + 1 < count && units[i + 1] >= 0xDC00 && units[i + 1] < 0xE000) {
			code = 0x10000 + ((code - 0xD800) << 10) + (uint32_t)(units[i + 1] - 0xDC00);
			i++;
		} else if (code >= 0xD800 && code < 0xE000) {
			code = 0xFFFD;
		}
		length += rvn_utf8_encode(code, text + length);
	}
	text[length] = '\0';
	return text;
}

/* ---------------------------------------------------------------------------
 *  The widget
 * ------------------------------------------------------------------------- */

/** Logical units per point at a zoom of 1: a page shows at its printed size (96 units per inch). */
#define RVN_PDF_UNITS_PER_POINT (96.0f / 72.0f)
#define RVN_PDF_GAP 12.0f
#define RVN_PDF_MIN_ZOOM 0.05f
#define RVN_PDF_MAX_ZOOM 32.0f
/** A page rendered whole up to this many pixels; beyond, only around what is seen. */
#define RVN_PDF_WHOLE_PIXELS (4096.0f * 4096.0f)
#define RVN_PDF_MAX_SIDE 8192.0f

typedef struct rvn_pdf_page_t {
	/** Size in points. */
	float width, height;
	Rid texture;
	int texture_w, texture_h;
	/** Size of the whole page, in pixels, the texture was drawn for, and the part of it the texture holds. */
	float full_w, full_h;
	float region_x, region_y;
} rvn_pdf_page_t;

typedef struct rvn_pdf_view_t {
	rvn_widget_t base;
	void* document;
	char* data;
	size_t size;
	RPdfStatus status;
	int page_count;
	rvn_pdf_page_t* pages;
	/** Top of each page in units of the content at the current zoom (page_count + 1 values, the last one the end). */
	float* tops;
	float widest;   /* widest page, in points */
	float tallest;

	float zoom;
	RPdfFit fit;
	rvn_scrollbar_t bar;
	rvn_scrollbar_t hbar;
	int page;
	/** Page to show at the top once the view has a size (Rpdf_view_set_page before the first layout). */
	int reveal;

	int dragging;
	float drag_x, drag_y;

	char* title;
	char* text;
	char* source;
	char* password;
	RPdfPageChangedCallback callback;
	void* user_data;
} rvn_pdf_view_t;

static void rvn_pdf_view_init(rvn_widget_t* widget) {
	rvn_pdf_view_t* view = (rvn_pdf_view_t*)widget;
	view->zoom = 1.0f;
	view->fit = R_PDF_FIT_WIDTH;
	view->reveal = -1;
	view->status = R_PDF_EMPTY;
}

static void rvn_pdf_release_textures(rvn_pdf_view_t* view, int keep_first, int keep_last) {
	int i;
	for (i = 0; i < view->page_count; i++) {
		rvn_pdf_page_t* page = &view->pages[i];
		if (i >= keep_first && i <= keep_last) continue;
		if (page->texture != R_INVALID_ID) {
			Rtexture_destroy(page->texture);
			page->texture = R_INVALID_ID;
		}
	}
}

static void rvn_pdf_close(rvn_pdf_view_t* view) {
	rvn_pdf_release_textures(view, 1, 0);
	if (view->document) g_pdfium.close_document(view->document);
	view->document = NULL;
	rvn_free(view->data);
	view->data = NULL;
	view->size = 0;
	rvn_free(view->pages);
	view->pages = NULL;
	rvn_free(view->tops);
	view->tops = NULL;
	view->page_count = 0;
	view->widest = view->tallest = 0.0f;
	rvn_free(view->title);
	view->title = NULL;
	rvn_free(view->text);
	view->text = NULL;
	view->page = 0;
	view->reveal = -1;
	view->dragging = R_FALSE;
	view->bar.offset = view->hbar.offset = 0.0f;
	rvn_spring_set(&view->bar.shown, 0.0f);
	rvn_spring_set(&view->hbar.shown, 0.0f);
	view->status = R_PDF_EMPTY;
}

static void rvn_pdf_view_destroy(rvn_widget_t* widget) {
	rvn_pdf_view_t* view = (rvn_pdf_view_t*)widget;
	rvn_pdf_close(view);
	rvn_free(view->source);
	rvn_free(view->password);
}

/* Units per point at the current zoom. */
static float rvn_pdf_unit(const rvn_pdf_view_t* view) {
	return RVN_PDF_UNITS_PER_POINT * view->zoom;
}

/* The zoom a fit mode gives for the size of the widget; the current zoom for R_PDF_FIT_NONE. */
static float rvn_pdf_fit_zoom(const rvn_pdf_view_t* view) {
	const rvn_widget_t* widget = &view->base;
	float avail_w = widget->rect.w - RVN_PDF_GAP * 2.0f;
	float avail_h = widget->rect.h - RVN_PDF_GAP * 2.0f;
	float zoom = view->zoom;
	if (view->widest <= 0.0f || avail_w <= 0.0f) return zoom;
	if (view->fit == R_PDF_FIT_WIDTH) {
		zoom = avail_w / (view->widest * RVN_PDF_UNITS_PER_POINT);
	} else if (view->fit == R_PDF_FIT_PAGE && avail_h > 0.0f && view->tallest > 0.0f) {
		zoom = rvn_minf(avail_w / (view->widest * RVN_PDF_UNITS_PER_POINT), avail_h / (view->tallest * RVN_PDF_UNITS_PER_POINT));
	}
	return rvn_clampf(zoom, RVN_PDF_MIN_ZOOM, RVN_PDF_MAX_ZOOM);
}

/* Places the pages for the zoom: tops, the length of the content in both directions. */
static void rvn_pdf_layout(rvn_pdf_view_t* view) {
	rvn_widget_t* widget = &view->base;
	float unit = rvn_pdf_unit(view), y = RVN_PDF_GAP;
	int i;
	for (i = 0; i < view->page_count; i++) {
		view->tops[i] = y;
		y += view->pages[i].height * unit + RVN_PDF_GAP;
	}
	if (view->tops) view->tops[view->page_count] = y;
	view->bar.content = view->page_count > 0 ? y : 0.0f;
	view->bar.viewport = rvn_maxf(0.0f, widget->rect.h);
	view->hbar.content = view->page_count > 0 ? view->widest * unit + RVN_PDF_GAP * 2.0f : 0.0f;
	view->hbar.viewport = rvn_maxf(0.0f, widget->rect.w);
	rvn_scrollbar_clamp(&view->bar);
	rvn_scrollbar_clamp(&view->hbar);
}

/* Rectangle of a page in widget coordinates, for scroll offsets. */
static rvn_rect_t rvn_pdf_page_rect(const rvn_pdf_view_t* view, int index, float x_offset, float y_offset) {
	const rvn_widget_t* widget = &view->base;
	float unit = rvn_pdf_unit(view);
	float w = view->pages[index].width * unit, h = view->pages[index].height * unit;
	float x;
	if (view->hbar.content <= widget->rect.w) x = (widget->rect.w - w) * 0.5f;
	else x = RVN_PDF_GAP + (view->widest * unit - w) * 0.5f - x_offset;
	return rvn_rect(x, view->tops[index] - y_offset, w, h);
}

/* Page under a height of the content (the one whose area, gap below included, holds it). */
static int rvn_pdf_page_at_y(const rvn_pdf_view_t* view, float y) {
	int low = 0, high = view->page_count - 1;
	if (view->page_count == 0) return -1;
	while (low < high) {
		int middle = (low + high + 1) / 2;
		if (view->tops[middle] <= y) low = middle;
		else high = middle - 1;
	}
	return low;
}

/* The current page: the one under the line a third down the view. */
static int rvn_pdf_current(const rvn_pdf_view_t* view) {
	if (view->page_count == 0) return 0;
	return rvn_maxi(0, rvn_pdf_page_at_y(view, view->bar.offset + view->bar.viewport * 0.33f));
}

/* After the view moved: tells the application when the current page changed. */
static void rvn_pdf_track_page(rvn_widget_t* widget) {
	rvn_pdf_view_t* view = (rvn_pdf_view_t*)widget;
	int page = rvn_pdf_current(view);
	if (page == view->page) return;
	view->page = page;
	if (view->callback) view->callback(widget->id, page, view->user_data);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

/* Scrolls so that the top of a page is at the top of the view. */
static void rvn_pdf_show_page(rvn_pdf_view_t* view, int page) {
	page = rvn_clampi(page, 0, view->page_count - 1);
	rvn_scrollbar_set(&view->bar, view->tops[page] - RVN_PDF_GAP);
}

/*
 * A point of the widget as a place in the document: the page and where in it
 * (0..1). Returns R_FALSE without pages.
 */
static int rvn_pdf_point_to_page(const rvn_pdf_view_t* view, float x, float y, int* page, float* u, float* v) {
	rvn_rect_t rect;
	*page = rvn_pdf_page_at_y(view, view->bar.offset + y);
	if (*page < 0) return R_FALSE;
	rect = rvn_pdf_page_rect(view, *page, view->hbar.offset, view->bar.offset);
	*u = rect.w > 0.0f ? (x - rect.x) / rect.w : 0.0f;
	*v = rect.h > 0.0f ? (y - rect.y) / rect.h : 0.0f;
	return R_TRUE;
}

/* Scrolls so that a place of the document is at a point of the widget. */
static void rvn_pdf_page_to_point(rvn_pdf_view_t* view, int page, float u, float v, float x, float y) {
	rvn_rect_t rect = rvn_pdf_page_rect(view, page, 0.0f, 0.0f);
	rvn_scrollbar_set(&view->bar, rect.y + v * rect.h - y);
	if (view->hbar.content > view->base.rect.w) rvn_scrollbar_set(&view->hbar, rect.x + u * rect.w - x);
}

/* Sets the zoom (the fit mode stays as it is), keeping the place under (x, y) where it is. */
static void rvn_pdf_zoom_at(rvn_pdf_view_t* view, float zoom, float x, float y) {
	int page;
	float u = 0.0f, v = 0.0f;
	int anchored = view->page_count > 0 && rvn_pdf_point_to_page(view, x, y, &page, &u, &v);
	view->zoom = rvn_clampf(zoom, RVN_PDF_MIN_ZOOM, RVN_PDF_MAX_ZOOM);
	if (view->page_count == 0) return;
	rvn_pdf_layout(view);
	if (anchored) {
		rvn_pdf_page_to_point(view, page, u, v, x, y);
		rvn_spring_set(&view->bar.shown, view->bar.offset);
		rvn_spring_set(&view->hbar.shown, view->hbar.offset);
	}
}

static rvn_vec2_t rvn_pdf_view_measure(rvn_widget_t* widget) {
	RVN_UNUSED(widget);
	return rvn_vec2(320.0f, 240.0f);
}

static void rvn_pdf_view_arrange(rvn_widget_t* widget) {
	rvn_pdf_view_t* view = (rvn_pdf_view_t*)widget;
	int page = -1;
	float u = 0.0f, v = 0.0f;
	float old_zoom = view->zoom, zoom;
	if (view->page_count == 0) {
		rvn_pdf_layout(view);
		return;
	}
	zoom = rvn_pdf_fit_zoom(view);
	/* a new zoom from the fit: the place at the top stays at the top */
	if (zoom != old_zoom && view->tops && view->bar.content > 0.0f) rvn_pdf_point_to_page(view, 0.0f, 0.0f, &page, &u, &v);
	view->zoom = zoom;
	rvn_pdf_layout(view);
	if (view->reveal >= 0 && widget->rect.h > 0.0f) {
		rvn_pdf_show_page(view, view->reveal);
		view->reveal = -1;
		rvn_spring_set(&view->bar.shown, view->bar.offset);
	} else if (page >= 0) {
		rvn_scrollbar_set(&view->bar, view->tops[page] + v * (view->tops[page + 1] - view->tops[page] - RVN_PDF_GAP));
		rvn_spring_set(&view->bar.shown, view->bar.offset);
	}
}

static rvn_rect_t rvn_pdf_track(const rvn_widget_t* widget) {
	float bar = rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE);
	return rvn_rect(widget->rect.w - bar - 1.0f, 1.0f, bar, rvn_maxf(0.0f, widget->rect.h - 2.0f - bar));
}

static rvn_rect_t rvn_pdf_htrack(const rvn_widget_t* widget) {
	float bar = rvn_widget_metric(widget, R_METRIC_SCROLLBAR_SIZE);
	return rvn_rect(1.0f, widget->rect.h - bar - 1.0f, rvn_maxf(0.0f, widget->rect.w - 2.0f - bar), bar);
}

/*
 * Draws a page into its texture: the whole page at `full_w` x `full_h`
 * pixels, or the part of it (region) that the view needs. R_FALSE when it
 * could not.
 */
static int rvn_pdf_render(rvn_pdf_view_t* view, int index, float full_w, float full_h, float rx, float ry, int width, int height) {
	rvn_pdf_page_t* page = &view->pages[index];
	uint8_t* pixels;
	void* pdf_page;
	void* bitmap;
	if (width <= 0 || height <= 0) return R_FALSE;
	pixels = (uint8_t*)rvn_alloc((size_t)width * (size_t)height * 4);
	if (!pixels) return R_FALSE;
	pdf_page = g_pdfium.load_page(view->document, index);
	if (!pdf_page) {
		rvn_free(pixels);
		return R_FALSE;
	}
	bitmap = g_pdfium.bitmap_create_ex(width, height, RVN_FPDF_BITMAP_BGRA, pixels, width * 4);
	if (!bitmap) {
		g_pdfium.close_page(pdf_page);
		rvn_free(pixels);
		return R_FALSE;
	}
	g_pdfium.bitmap_fill_rect(bitmap, 0, 0, width, height, 0xFFFFFFFFul);
	/* RGBA bytes (REVERSE_BYTE_ORDER), annotations (form fields, notes) drawn */
	g_pdfium.render_page_bitmap(bitmap, pdf_page, -(int)rx, -(int)ry, (int)full_w, (int)full_h, 0,
		RVN_FPDF_ANNOT | RVN_FPDF_REVERSE_BYTE_ORDER);
	g_pdfium.bitmap_destroy(bitmap);
	g_pdfium.close_page(pdf_page);

	if (page->texture != R_INVALID_ID && page->texture_w == width && page->texture_h == height) {
		Rtexture_update(page->texture, pixels, width, height);
	} else {
		if (page->texture != R_INVALID_ID) Rtexture_destroy(page->texture);
		page->texture = Rtexture_create_from_pixels(pixels, width, height);
	}
	rvn_free(pixels);
	if (page->texture == R_INVALID_ID) return R_FALSE;
	page->texture_w = width;
	page->texture_h = height;
	page->full_w = full_w;
	page->full_h = full_h;
	page->region_x = (float)(int)rx;
	page->region_y = (float)(int)ry;
	return R_TRUE;
}

static const char* rvn_pdf_text(const char* key, const char* english) {
	return Rtr_exists(key) ? Rtr(key) : english;
}

static void rvn_pdf_view_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_pdf_view_t* view = (rvn_pdf_view_t*)widget;
	float y_offset = rvn_scrollbar_shown(&view->bar, painter);
	float x_offset = rvn_scrollbar_shown(&view->hbar, painter);
	float scale = painter->scale > 0.0f ? painter->scale : 1.0f;
	rvn_color_t page_shadow = rvn_widget_color(widget, R_COLOR_SHADOW);
	rvn_color_t border = rvn_widget_color(widget, R_COLOR_BORDER);
	rvn_vec2_t origin = rvn_widget_origin(widget);
	float mouse_x = widget->window ? widget->window->mouse_x - origin.x : -1.0f;
	float mouse_y = widget->window ? widget->window->mouse_y - origin.y : -1.0f;
	int first, last, i, budget = 2;
	int stale_left = R_FALSE;

	rvn_paint_box(painter, rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h), 0.0f,
		rvn_widget_color(widget, R_COLOR_PANEL), 1.0f, border);

	if (view->status != R_PDF_LOADED) {
		const char* message = NULL;
		if (view->status == R_PDF_UNAVAILABLE)
			message = rvn_pdf_text("raven.pdf.unavailable", "PDF pages cannot be shown: the PDFium library was not found.");
		else if (view->status == R_PDF_NEEDS_PASSWORD)
			message = rvn_pdf_text("raven.pdf.password", "This document is protected by a password.");
		else if (view->status == R_PDF_ERROR)
			message = rvn_pdf_text("raven.pdf.error", "This file could not be opened as a PDF document.");
		if (message) {
			float size = rvn_widget_font_size(widget, 0.0f);
			rvn_paint_text_in(painter, R_INVALID_ID, size, rvn_rect(RVN_PDF_GAP, 0.0f, rvn_maxf(0.0f, widget->rect.w - RVN_PDF_GAP * 2.0f), widget->rect.h),
				0.5f, message, -1, rvn_widget_color(widget, R_COLOR_TEXT_DISABLED));
		}
		return;
	}

	rvn_paint_push_clip(painter, rvn_rect(1.0f, 1.0f, rvn_maxf(0.0f, widget->rect.w - 2.0f), rvn_maxf(0.0f, widget->rect.h - 2.0f)));
	first = rvn_maxi(0, rvn_pdf_page_at_y(view, y_offset));
	last = first;
	while (last + 1 < view->page_count && view->tops[last + 1] < y_offset + widget->rect.h) last++;

	for (i = first; i <= last; i++) {
		rvn_pdf_page_t* page = &view->pages[i];
		rvn_rect_t rect = rvn_pdf_page_rect(view, i, x_offset, y_offset);
		/* pixels of the whole page, and of the part of it that is seen */
		float full_w = floorf(rect.w * scale + 0.5f), full_h = floorf(rect.h * scale + 0.5f);
		float vx0 = rvn_maxf(0.0f, -rect.x) * scale, vy0 = rvn_maxf(0.0f, -rect.y) * scale;
		float vx1 = rvn_minf(rect.w, widget->rect.w - rect.x) * scale, vy1 = rvn_minf(rect.h, widget->rect.h - rect.y) * scale;
		float rx = 0.0f, ry = 0.0f, rw = full_w, rh = full_h;
		int whole = full_w * full_h <= RVN_PDF_WHOLE_PIXELS && full_w <= RVN_PDF_MAX_SIDE && full_h <= RVN_PDF_MAX_SIDE;
		int stale;

		rvn_paint_shadow(painter, rect, 0.0f, 8.0f, rvn_color_fade(page_shadow, 0.5f));
		rvn_paint_rect(painter, rect, 0xFFFFFFFFu);

		if (!whole) {
			/* the part that is seen, and half a view around it */
			float mx = widget->rect.w * scale * 0.5f, my = widget->rect.h * scale * 0.5f;
			rx = floorf(rvn_maxf(0.0f, vx0 - mx));
			ry = floorf(rvn_maxf(0.0f, vy0 - my));
			rw = rvn_minf(full_w, ceilf(vx1 + mx)) - rx;
			rh = rvn_minf(full_h, ceilf(vy1 + my)) - ry;
		}
		stale = page->texture == R_INVALID_ID || fabsf(page->full_w - full_w) > 0.5f || fabsf(page->full_h - full_h) > 0.5f;
		if (!stale && !whole) {
			/* still holds what is seen? */
			stale = vx0 < page->region_x || vy0 < page->region_y ||
				vx1 > page->region_x + (float)page->texture_w || vy1 > page->region_y + (float)page->texture_h;
		}
		if (stale && rw >= 1.0f && rh >= 1.0f) {
			/* a few pages per frame: the others show white (or their old texture) until the next one */
			if (budget > 0) {
				budget--;
				rvn_pdf_render(view, i, full_w, full_h, rx, ry, (int)rw, (int)rh);
			} else {
				stale_left = R_TRUE;
			}
		}
		if (page->texture != R_INVALID_ID && page->full_w > 0.0f && page->full_h > 0.0f) {
			float sx = rect.w / page->full_w, sy = rect.h / page->full_h;
			rvn_rect_t shown = rvn_rect(rect.x + page->region_x * sx, rect.y + page->region_y * sy,
				(float)page->texture_w * sx, (float)page->texture_h * sy);
			static const float uv[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
			rvn_paint_image(painter, shown, page->texture, uv, 0xFFFFFFFFu, 0.0f);
		}
	}
	rvn_paint_pop_clip(painter);
	if (stale_left) rvn_paint_keep_animating(painter);
	/* the pages far from the view give their textures back */
	rvn_pdf_release_textures(view, first - 2, last + 2);

	rvn_scrollbar_paint(&view->bar, widget, painter, rvn_pdf_track(widget), R_TRUE, mouse_x, mouse_y);
	if (rvn_scrollbar_needed(&view->hbar))
		rvn_scrollbar_paint(&view->hbar, widget, painter, rvn_pdf_htrack(widget), R_FALSE, mouse_x, mouse_y);
}

/* The user zooms: the fit mode is left for a zoom of its own. */
static void rvn_pdf_user_zoom(rvn_widget_t* widget, float zoom, float x, float y) {
	rvn_pdf_view_t* view = (rvn_pdf_view_t*)widget;
	view->fit = R_PDF_FIT_NONE;
	rvn_pdf_zoom_at(view, zoom, x, y);
	rvn_widget_invalidate_paint(widget);
	rvn_pdf_track_page(widget);
}

static int rvn_pdf_view_event(rvn_widget_t* widget, const REvent* event) {
	rvn_pdf_view_t* view = (rvn_pdf_view_t*)widget;
	int changed = R_FALSE;

	if (event->type == R_EVENT_MOUSE_DOWN || event->type == R_EVENT_MOUSE_MOVE || event->type == R_EVENT_MOUSE_UP) {
		if (!view->dragging && rvn_scrollbar_event(&view->bar, widget, event, rvn_pdf_track(widget), R_TRUE, &changed)) {
			if (changed) rvn_pdf_track_page(widget);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		}
		if (!view->dragging && rvn_scrollbar_needed(&view->hbar) &&
			rvn_scrollbar_event(&view->hbar, widget, event, rvn_pdf_htrack(widget), R_FALSE, &changed)) {
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		}
	}

	switch (event->type) {
		case R_EVENT_MOUSE_DOWN:
			/* the pages are dragged by the hand */
			if (event->button != R_MOUSE_LEFT || view->page_count == 0) return R_FALSE;
			view->dragging = R_TRUE;
			view->drag_x = event->local_x;
			view->drag_y = event->local_y;
			return R_TRUE;
		case R_EVENT_MOUSE_MOVE:
			if (!view->dragging) return R_FALSE;
			changed = rvn_scrollbar_set(&view->bar, view->bar.offset - (event->local_y - view->drag_y));
			changed |= rvn_scrollbar_set(&view->hbar, view->hbar.offset - (event->local_x - view->drag_x));
			view->drag_x = event->local_x;
			view->drag_y = event->local_y;
			if (changed) {
				rvn_spring_set(&view->bar.shown, view->bar.offset);
				rvn_spring_set(&view->hbar.shown, view->hbar.offset);
				rvn_pdf_track_page(widget);
				rvn_widget_invalidate_paint(widget);
			}
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (!view->dragging) return R_FALSE;
			view->dragging = R_FALSE;
			return R_TRUE;
		case R_EVENT_MOUSE_WHEEL:
			if (view->page_count == 0) return R_FALSE;
			if (event->modifiers & (R_MOD_CTRL | R_MOD_SUPER)) {
				rvn_pdf_user_zoom(widget, view->zoom * powf(1.1f, event->wheel_y), event->local_x, event->local_y);
				return R_TRUE;
			}
			if ((event->modifiers & R_MOD_SHIFT) || event->wheel_x != 0.0f) {
				float amount = (event->modifiers & R_MOD_SHIFT) ? event->wheel_y : -event->wheel_x;
				changed = rvn_scrollbar_wheel(&view->hbar, -amount * 48.0f);
			} else {
				changed = rvn_scrollbar_wheel(&view->bar, -event->wheel_y * 48.0f);
			}
			rvn_widget_invalidate_paint(widget);
			if (!changed) return R_FALSE;
			rvn_pdf_track_page(widget);
			return R_TRUE;
		case R_EVENT_FOCUS_GAINED:
		case R_EVENT_FOCUS_LOST:
			rvn_widget_invalidate_paint(widget);
			return R_FALSE;
		case R_EVENT_KEY_DOWN: {
			int zoom_keys = (event->modifiers & (R_MOD_CTRL | R_MOD_SUPER)) != 0;
			float page = rvn_maxf(40.0f, view->bar.viewport * 0.9f);
			if (view->page_count == 0) return R_FALSE;
			if (zoom_keys && (event->key == R_KEY_EQUAL || event->key == R_KEY_KP_ADD)) {
				rvn_pdf_user_zoom(widget, view->zoom * 1.25f, widget->rect.w * 0.5f, widget->rect.h * 0.5f);
				return R_TRUE;
			}
			if (zoom_keys && (event->key == R_KEY_MINUS || event->key == R_KEY_KP_SUBTRACT)) {
				rvn_pdf_user_zoom(widget, view->zoom / 1.25f, widget->rect.w * 0.5f, widget->rect.h * 0.5f);
				return R_TRUE;
			}
			if (zoom_keys && event->key == R_KEY_0) {
				view->fit = R_PDF_FIT_WIDTH;
				rvn_widget_invalidate_layout(widget);
				return R_TRUE;
			}
			switch (event->key) {
				case R_KEY_UP: changed = rvn_scrollbar_set(&view->bar, view->bar.offset - 40.0f); break;
				case R_KEY_DOWN: changed = rvn_scrollbar_set(&view->bar, view->bar.offset + 40.0f); break;
				case R_KEY_LEFT: changed = rvn_scrollbar_set(&view->hbar, view->hbar.offset - 40.0f); break;
				case R_KEY_RIGHT: changed = rvn_scrollbar_set(&view->hbar, view->hbar.offset + 40.0f); break;
				case R_KEY_PAGE_UP: changed = rvn_scrollbar_set(&view->bar, view->bar.offset - page); break;
				case R_KEY_PAGE_DOWN:
				case R_KEY_SPACE:
					changed = rvn_scrollbar_set(&view->bar, view->bar.offset + ((event->key == R_KEY_SPACE && (event->modifiers & R_MOD_SHIFT)) ? -page : page));
					break;
				case R_KEY_HOME: changed = rvn_scrollbar_set(&view->bar, 0.0f); break;
				case R_KEY_END: changed = rvn_scrollbar_set(&view->bar, rvn_scrollbar_max(&view->bar)); break;
				default: return R_FALSE;
			}
			if (changed) {
				rvn_pdf_track_page(widget);
				rvn_widget_invalidate_paint(widget);
			}
			return R_TRUE;
		}
		default:
			return R_FALSE;
	}
}

static int rvn_touch_pdf_view_pan(rvn_widget_t* widget, float dx, float dy, int coasting) {
	rvn_pdf_view_t* view = (rvn_pdf_view_t*)widget;
	int moved = rvn_scrollbar_pan(&view->bar, -dy, coasting);
	if (rvn_scrollbar_needed(&view->hbar)) moved |= rvn_scrollbar_pan(&view->hbar, -dx, coasting);
	if (moved) rvn_pdf_track_page(widget);
	return moved;
}

static int rvn_touch_pdf_view_zoom(rvn_widget_t* widget, float factor, float x, float y) {
	rvn_pdf_view_t* view = (rvn_pdf_view_t*)widget;
	float before = view->zoom;
	if (view->page_count == 0) return R_FALSE;
	rvn_pdf_user_zoom(widget, view->zoom * factor, x, y);
	return view->zoom != before;
}

static int rvn_pdf_parse_fit(const char* value, RPdfFit* fit) {
	if (rvn_stricmp(value, "Width") == 0) *fit = R_PDF_FIT_WIDTH;
	else if (rvn_stricmp(value, "Page") == 0) *fit = R_PDF_FIT_PAGE;
	else if (rvn_stricmp(value, "None") == 0) *fit = R_PDF_FIT_NONE;
	else return R_FALSE;
	return R_TRUE;
}

static int rvn_pdf_view_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_pdf_view_t* view = (rvn_pdf_view_t*)widget;
	float number;
	if (RVN_ATTR("Source")) {
		char* path;
		rvn_str_set(&view->source, value);
		if (!value || !value[0]) {
			Rpdf_view_close(widget->id);
			return R_TRUE;
		}
		path = rvn_xml_resolve_path(value);
		Rpdf_view_load_file(widget->id, path ? path : value, view->password);
		rvn_free(path);
		/* a file that is not there, or not a PDF, shows as such: the attribute itself is right */
		return R_TRUE;
	} else if (RVN_ATTR("Password")) {
		rvn_str_set(&view->password, value);
		/* given after Source: the document waiting for it opens now */
		if (view->status == R_PDF_NEEDS_PASSWORD && view->source && view->source[0]) {
			char* path = rvn_xml_resolve_path(view->source);
			Rpdf_view_load_file(widget->id, path ? path : view->source, view->password);
			rvn_free(path);
		}
	} else if (RVN_ATTR("Fit")) {
		RPdfFit fit;
		if (!rvn_pdf_parse_fit(value, &fit)) return R_FALSE;
		Rpdf_view_set_fit(widget->id, fit);
	} else if (RVN_ATTR("Zoom")) {
		if (!rvn_parse_float(value, &number) || number <= 0.0f) return R_FALSE;
		Rpdf_view_set_zoom(widget->id, number);
	} else if (RVN_ATTR("Page")) {
		if (!rvn_parse_float(value, &number)) return R_FALSE;
		/* counted from 1 in XML, as they are shown */
		Rpdf_view_set_page(widget->id, (int)number - 1);
	} else {
		return R_FALSE;
	}
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_pdf_view = {
	.type = R_WIDGET_PDF_VIEW,
	.name = "PdfView",
	.size = sizeof(rvn_pdf_view_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_UI_PAINT,
	.init = rvn_pdf_view_init,
	.destroy = rvn_pdf_view_destroy,
	.measure = rvn_pdf_view_measure,
	.arrange = rvn_pdf_view_arrange,
	.paint = rvn_pdf_view_paint,
	.event = rvn_pdf_view_event,
	.pan = rvn_touch_pdf_view_pan,
	.zoom = rvn_touch_pdf_view_zoom,
	.set_attribute = rvn_pdf_view_set_attribute
};

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

/* Opens a document from bytes the view now owns (`data` is taken, freed on failure). */
static int rvn_pdf_open_document(rvn_widget_t* widget, char* data, size_t size, const char* password) {
	rvn_pdf_view_t* view = (rvn_pdf_view_t*)widget;
	unsigned long length;
	int i;

	int reveal = view->reveal;
	rvn_pdf_close(view);
	/* a page asked for before the document came (XML: Page before Source) */
	view->reveal = reveal;
	rvn_widget_invalidate_layout(widget);
	if (!rvn_pdfium_load()) {
		rvn_free(data);
		view->status = R_PDF_UNAVAILABLE;
		return R_FALSE;
	}
	if (size > (size_t)INT_MAX) {
		rvn_free(data);
		view->status = R_PDF_ERROR;
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "PdfView: the document is too large");
		return R_FALSE;
	}
	/* PDFium reads the bytes while the document is open: they are kept */
	view->document = g_pdfium.load_mem_document(data, (int)size, password && password[0] ? password : NULL);
	if (!view->document) {
		unsigned long code = g_pdfium.get_last_error();
		rvn_free(data);
		view->status = code == RVN_FPDF_ERR_PASSWORD ? R_PDF_NEEDS_PASSWORD : R_PDF_ERROR;
		if (view->status == R_PDF_ERROR) rvn_error(R_ERROR_INVALID_FILE_FORMAT, R_SEVERITY_ERROR, "PdfView: not a PDF document PDFium can read (error %lu)", code);
		return R_FALSE;
	}
	view->data = data;
	view->size = size;
	view->page_count = rvn_maxi(0, g_pdfium.get_page_count(view->document));
	view->pages = (rvn_pdf_page_t*)rvn_calloc(sizeof(rvn_pdf_page_t) * (size_t)rvn_maxi(1, view->page_count));
	view->tops = (float*)rvn_calloc(sizeof(float) * (size_t)(view->page_count + 1));
	if (!view->pages || !view->tops) {
		rvn_pdf_close(view);
		view->status = R_PDF_ERROR;
		rvn_error(R_ERROR_OUT_OF_MEMORY, R_SEVERITY_ERROR, "PdfView: out of memory");
		return R_FALSE;
	}
	for (i = 0; i < view->page_count; i++) {
		double w = 0.0, h = 0.0;
		rvn_pdf_page_t* page = &view->pages[i];
		/* a page without a size takes the size of a sheet (A4) */
		if (!g_pdfium.get_page_size_by_index(view->document, i, &w, &h) || w <= 0.0 || h <= 0.0) {
			w = 595.0;
			h = 842.0;
		}
		page->width = (float)w;
		page->height = (float)h;
		page->texture = R_INVALID_ID;
		view->widest = rvn_maxf(view->widest, page->width);
		view->tallest = rvn_maxf(view->tallest, page->height);
	}
	/* the title of the document (UTF-16 in PDFium) */
	length = g_pdfium.get_meta_text(view->document, "Title", NULL, 0);
	if (length > 2) {
		unsigned short* units = (unsigned short*)rvn_alloc(length);
		if (units) {
			g_pdfium.get_meta_text(view->document, "Title", units, length);
			view->title = rvn_pdf_utf16_to_utf8(units, (int)(length / 2));
			rvn_free(units);
		}
	}
	view->status = R_PDF_LOADED;
	view->page = 0;
	if (view->reveal >= view->page_count) view->reveal = view->page_count - 1;
	if (view->reveal > 0) view->page = view->reveal;
	rvn_pdf_layout(view);
	return R_TRUE;
}

int Rpdf_view_load_file(Rid id, const char* path, const char* password) {
	size_t size = 0;
	char* data;
	RVN_WIDGET(widget, id, R_WIDGET_PDF_VIEW, R_FALSE);
	if (!path || !path[0]) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rpdf_view_load_file: no path");
		return R_FALSE;
	}
	data = rvn_read_file(path, &size);
	if (!data) {
		rvn_pdf_close((rvn_pdf_view_t*)widget);
		((rvn_pdf_view_t*)widget)->status = R_PDF_ERROR;
		rvn_widget_invalidate_layout(widget);
		rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "Rpdf_view_load_file: cannot read '%s'", path);
		return R_FALSE;
	}
	return rvn_pdf_open_document(widget, data, size, password);
}

int Rpdf_view_load_memory(Rid id, const char* data, size_t data_size, const char* password) {
	char* copy;
	RVN_WIDGET(widget, id, R_WIDGET_PDF_VIEW, R_FALSE);
	if (!data || data_size == 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rpdf_view_load_memory: no data");
		return R_FALSE;
	}
	copy = (char*)rvn_alloc(data_size);
	if (!copy) {
		rvn_error(R_ERROR_OUT_OF_MEMORY, R_SEVERITY_ERROR, "Rpdf_view_load_memory: out of memory");
		return R_FALSE;
	}
	memcpy(copy, data, data_size);
	return rvn_pdf_open_document(widget, copy, data_size, password);
}

void Rpdf_view_close(Rid id) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PDF_VIEW);
	rvn_pdf_close((rvn_pdf_view_t*)widget);
	((rvn_pdf_view_t*)widget)->reveal = -1;
	rvn_widget_invalidate_layout(widget);
}

RPdfStatus Rpdf_view_get_status(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_PDF_VIEW, R_PDF_EMPTY);
	return ((rvn_pdf_view_t*)widget)->status;
}

int Rpdf_view_get_page_count(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_PDF_VIEW, 0);
	return ((rvn_pdf_view_t*)widget)->page_count;
}

void Rpdf_view_get_page_size(Rid id, int page, float* width, float* height) {
	rvn_pdf_view_t* view;
	if (width) *width = 0.0f;
	if (height) *height = 0.0f;
	{
		RVN_WIDGET_VOID(widget, id, R_WIDGET_PDF_VIEW);
		view = (rvn_pdf_view_t*)widget;
	}
	if (page < 0 || page >= view->page_count) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rpdf_view_get_page_size: no page %d", page);
		return;
	}
	if (width) *width = view->pages[page].width;
	if (height) *height = view->pages[page].height;
}

void Rpdf_view_set_page(Rid id, int page) {
	rvn_pdf_view_t* view;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PDF_VIEW);
	view = (rvn_pdf_view_t*)widget;
	if (page < 0) page = 0;
	if (view->page_count == 0) {
		/* shown when a document is loaded */
		view->reveal = page;
		return;
	}
	page = rvn_clampi(page, 0, view->page_count - 1);
	if (widget->rect.h <= 0.0f) {
		/* not laid out yet: once it is */
		view->reveal = page;
		view->page = page;
		rvn_widget_invalidate_layout(widget);
		return;
	}
	rvn_pdf_show_page(view, page);
	rvn_spring_set(&view->bar.shown, view->bar.offset);
	/* the page asked for is the current one, even when the end of the document keeps it lower in the view */
	view->page = page;
	rvn_widget_invalidate_paint(widget);
}

int Rpdf_view_get_page(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_PDF_VIEW, 0);
	return ((rvn_pdf_view_t*)widget)->page;
}

void Rpdf_view_set_zoom(Rid id, float zoom) {
	rvn_pdf_view_t* view;
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PDF_VIEW);
	view = (rvn_pdf_view_t*)widget;
	if (!(zoom > 0.0f)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rpdf_view_set_zoom: the zoom must be above 0");
		return;
	}
	view->fit = R_PDF_FIT_NONE;
	rvn_pdf_zoom_at(view, zoom, 0.0f, 0.0f);
	rvn_widget_invalidate_layout(widget);
}

float Rpdf_view_get_zoom(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_PDF_VIEW, 1.0f);
	return ((rvn_pdf_view_t*)widget)->zoom;
}

void Rpdf_view_set_fit(Rid id, RPdfFit fit) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PDF_VIEW);
	if (fit < R_PDF_FIT_WIDTH || fit > R_PDF_FIT_NONE) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rpdf_view_set_fit: invalid fit");
		return;
	}
	((rvn_pdf_view_t*)widget)->fit = fit;
	rvn_widget_invalidate_layout(widget);
}

RPdfFit Rpdf_view_get_fit(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_PDF_VIEW, R_PDF_FIT_WIDTH);
	return ((rvn_pdf_view_t*)widget)->fit;
}

const char* Rpdf_view_get_title(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_PDF_VIEW, "");
	return ((rvn_pdf_view_t*)widget)->title ? ((rvn_pdf_view_t*)widget)->title : "";
}

const char* Rpdf_view_get_page_text(Rid id, int page) {
	rvn_pdf_view_t* view;
	void* pdf_page;
	void* text_page;
	RVN_WIDGET(widget, id, R_WIDGET_PDF_VIEW, "");
	view = (rvn_pdf_view_t*)widget;
	if (page < 0 || page >= view->page_count) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rpdf_view_get_page_text: no page %d", page);
		return "";
	}
	rvn_free(view->text);
	view->text = NULL;
	pdf_page = g_pdfium.load_page(view->document, page);
	if (!pdf_page) return "";
	text_page = g_pdfium.text_load_page(pdf_page);
	if (text_page) {
		int count = g_pdfium.text_count_chars(text_page);
		if (count > 0) {
			unsigned short* units = (unsigned short*)rvn_alloc(((size_t)count + 1) * sizeof(unsigned short));
			if (units) {
				int written = g_pdfium.text_get_text(text_page, 0, count, units);
				view->text = rvn_pdf_utf16_to_utf8(units, rvn_clampi(written, 0, count + 1));
				rvn_free(units);
			}
		}
		g_pdfium.text_close_page(text_page);
	}
	g_pdfium.close_page(pdf_page);
	return view->text ? view->text : "";
}

void Rpdf_view_set_page_changed_callback(Rid id, RPdfPageChangedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PDF_VIEW);
	((rvn_pdf_view_t*)widget)->callback = callback;
	((rvn_pdf_view_t*)widget)->user_data = user_data;
}
