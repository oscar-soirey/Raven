/* Issue #53: the GDI surface under a transparent frame must not remain white.
 * This exercises the surface used by desktop capture, without requiring a GPU.
 * It does not automate Snipping Tool or validate the final DWM composition. */
#include "test.h"
#include <windows.h>

static void fill_white(HWND window) {
	RECT rect;
	HDC dc = GetDCEx(window, NULL, DCX_CACHE);
	CHECK(dc != NULL);
	if (!dc) return;
	CHECK(GetClientRect(window, &rect));
	CHECK(FillRect(dc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH)) != 0);
	GdiFlush();
	ReleaseDC(window, dc);
}

static COLORREF surface_pixel(HWND window, int x, int y) {
	HDC dc = GetDCEx(window, NULL, DCX_CACHE);
	COLORREF pixel = dc ? GetPixel(dc, x, y) : CLR_INVALID;
	if (dc) ReleaseDC(window, dc);
	return pixel;
}

static void test_surface(Rid id, HWND window) {
	RBackdrop backdrop;
	for (backdrop = R_BACKDROP_TRANSPARENT; backdrop <= R_BACKDROP_MICA; backdrop++) {
		fill_white(window);
		CHECK(surface_pixel(window, 10, 10) == RGB(255, 255, 255));
		CHECK(Rwindow_set_backdrop(id, backdrop));
		CHECK(surface_pixel(window, 10, 10) == RGB(0, 0, 0));

		fill_white(window);
		SendMessageW(window, WM_ERASEBKGND, 0, 0);
		CHECK(surface_pixel(window, 10, 10) == RGB(0, 0, 0));

		fill_white(window);
		InvalidateRect(window, NULL, FALSE);
		UpdateWindow(window); /* WM_PAINT */
		CHECK(surface_pixel(window, 10, 10) == RGB(0, 0, 0));

		fill_white(window);
		Rwindow_set_size(id, 240 + (int)(backdrop - R_BACKDROP_TRANSPARENT) * 10, 160);
		CHECK(surface_pixel(window, 10, 10) == RGB(0, 0, 0));
	}
}

static void test_opaque(void) {
	Rid id = Rwindow_create("Opaque control", 240, 160, R_WINDOW_DEFAULT);
	RNativeWindow native;
	HWND window;
	CHECK(Rwindow_get_native_handle(id, &native));
	window = (HWND)native.window;
	Rwindow_show(id);
	UpdateWindow(window);
	fill_white(window);
	SendMessageW(window, WM_ERASEBKGND, 0, 0);
	CHECK(surface_pixel(window, 10, 10) == RGB(255, 255, 255));
	InvalidateRect(window, NULL, FALSE);
	UpdateWindow(window);
	CHECK(surface_pixel(window, 10, 10) == RGB(255, 255, 255));
	Rwindow_destroy(id);
}

int main(void) {
	RInitDesc desc = {0};
	RNativeWindow native;
	Rid id;
	desc.backend = R_BACKEND_NULL;
	desc.worker_threads = -1;
	Rerror_set_callback(test_error_callback);
	if (!Rinit_ex(&desc)) return 1;
	id = Rwindow_create("Backdrop surface", 220, 160, R_WINDOW_TRANSPARENT);
	CHECK(Rwindow_get_native_handle(id, &native));
	if (!native.transparent) {
		Rwindow_destroy(id);
		Rshutdown();
		puts("SKIP: desktop composition is unavailable");
		return 77;
	}
	Rwindow_show(id);
	UpdateWindow((HWND)native.window);
	test_surface(id, (HWND)native.window);
	Rwindow_destroy(id);
	RUN(test_opaque);
	Rshutdown();
	return test_report("test_win32_backdrop");
}
