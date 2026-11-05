/**
 * Raven - platform/win32/win32_taskbar.c
 * The taskbar of Windows (raven_shell.h, TASKBAR AND DOCK, TRAY ICON): the progress on
 * the button of a window and the badge (ITaskbarList3: the badge is an
 * overlay icon drawn here from its text), the flashing of the button
 * (FlashWindowEx) and the tasks of the jump list (ICustomDestinationList).
 *
 * The taskbar only takes calls for a button that exists: what was asked is
 * kept and given again when Windows says the button was made
 * (TaskbarButtonCreated), which it also says when Explorer starts again.
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objbase.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <windowsx.h>
#include <propsys.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "win32_taskbar.h"

/* the identifiers, here rather than from uuid.lib, which not every toolchain has with them */
static const GUID rvn_tb_clsid_taskbar = { 0x56fdf344, 0xfd6d, 0x11d0, { 0x95, 0x8a, 0x00, 0x60, 0x97, 0xc9, 0xa0, 0x90 } };
static const GUID rvn_tb_iid_taskbar3 = { 0xea1afb91, 0x9e28, 0x4b86, { 0x90, 0xe9, 0x9e, 0x9f, 0x8a, 0x5e, 0xef, 0xaf } };
static const GUID rvn_tb_clsid_destinations = { 0x77f10cf0, 0x3db5, 0x4966, { 0xb5, 0x20, 0xb7, 0xc5, 0x4f, 0xd3, 0x5e, 0xd6 } };
static const GUID rvn_tb_iid_destinations = { 0x6332debf, 0x87b5, 0x4670, { 0x90, 0xc0, 0x5e, 0x57, 0xb4, 0x08, 0xa4, 0x9e } };
static const GUID rvn_tb_clsid_collection = { 0x2d3468c1, 0x36a7, 0x43b6, { 0xac, 0x24, 0xd3, 0xf0, 0x2f, 0xd9, 0x60, 0x7a } };
static const GUID rvn_tb_iid_collection = { 0x5632b1a4, 0xe38a, 0x400a, { 0x92, 0x8a, 0xd4, 0xcd, 0x63, 0x23, 0x02, 0x95 } };
static const GUID rvn_tb_iid_array = { 0x92ca9dcd, 0x5622, 0x4bba, { 0xa8, 0x05, 0x5e, 0x9f, 0x54, 0x1b, 0xd8, 0xc9 } };
static const GUID rvn_tb_clsid_link = { 0x00021401, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID rvn_tb_iid_link = { 0x000214f9, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID rvn_tb_iid_store = { 0x886d8eeb, 0x8cf2, 0x4446, { 0x8d, 0x02, 0xcd, 0xba, 0x1d, 0xbd, 0xcf, 0x99 } };
static const PROPERTYKEY rvn_tb_key_title = { { 0xf29f85e0, 0x4ff9, 0x1068, { 0xab, 0x91, 0x08, 0x00, 0x2b, 0x27, 0xb3, 0xd9 } }, 2 };

static struct {
	/** 0: not tried yet, 1: COM is ready, -1: it is not. */
	int com;
	ITaskbarList3* taskbar;
	int taskbar_failed;
	UINT created_message;
	/** The badge: its text, and the icon drawn from it. */
	WCHAR badge[16];
	HICON badge_icon;
} g_tb;

static int rvn_tb_com(void) {
	if (g_tb.com == 0) {
		/* S_FALSE: the application had done it already (it is counted all the same) */
		HRESULT result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		g_tb.com = (result == S_OK || result == S_FALSE) ? 1 : -1;
	}
	return g_tb.com > 0;
}

static ITaskbarList3* rvn_tb_taskbar(void) {
	if (g_tb.taskbar || g_tb.taskbar_failed) return g_tb.taskbar;
	if (!rvn_tb_com() || FAILED(CoCreateInstance(&rvn_tb_clsid_taskbar, NULL, CLSCTX_INPROC_SERVER, &rvn_tb_iid_taskbar3,
		(void**)&g_tb.taskbar))) {
		g_tb.taskbar = NULL;
		g_tb.taskbar_failed = 1;
		return NULL;
	}
	if (FAILED(g_tb.taskbar->lpVtbl->HrInit(g_tb.taskbar))) {
		g_tb.taskbar->lpVtbl->Release(g_tb.taskbar);
		g_tb.taskbar = NULL;
		g_tb.taskbar_failed = 1;
	}
	return g_tb.taskbar;
}

UINT rvn_win32_taskbar_created_message(void) {
	if (!g_tb.created_message) g_tb.created_message = RegisterWindowMessageW(L"TaskbarButtonCreated");
	return g_tb.created_message;
}

int rvn_win32_taskbar_progress(HWND window, int state, float value) {
	static const TBPFLAG flags[5] = { TBPF_NOPROGRESS, TBPF_NORMAL, TBPF_PAUSED, TBPF_ERROR, TBPF_INDETERMINATE };
	ITaskbarList3* taskbar = rvn_tb_taskbar();
	if (!taskbar || state < 0 || state > 4) return 0;
	taskbar->lpVtbl->SetProgressState(taskbar, window, flags[state]);
	if (state >= 1 && state <= 3) {
		taskbar->lpVtbl->SetProgressValue(taskbar, window, (ULONGLONG)(value * 1000.0f + 0.5f), 1000);
	}
	return 1;
}

/*
 * The badge as an icon: a red disc with the text in white, drawn at the size
 * of the small icons. The disc is computed here (its edge is smooth); GDI
 * draws the text in a separate image whose brightness is how much of each
 * pixel the letters cover.
 */
static HICON rvn_tb_badge_icon(const WCHAR* text) {
	int size = GetSystemMetrics(SM_CXSMICON) * 2;
	BITMAPINFO info;
	void* pixels = NULL;
	void* ink = NULL;
	HDC screen = GetDC(NULL), dc = CreateCompatibleDC(screen);
	HBITMAP color, letters, mask, before;
	HFONT font, font_before;
	ICONINFO icon;
	HICON result = NULL;
	RECT rect;
	int x, y, length = (int)wcslen(text);
	float radius = (float)size * 0.5f;

	memset(&info, 0, sizeof(info));
	info.bmiHeader.biSize = sizeof(info.bmiHeader);
	info.bmiHeader.biWidth = size;
	info.bmiHeader.biHeight = -size;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;
	color = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &pixels, NULL, 0);
	letters = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &ink, NULL, 0);
	mask = CreateBitmap(size, size, 1, 1, NULL);
	if (!color || !letters || !mask || !pixels || !ink) goto done;

	/* the letters, white on black: the fewer they are, the larger */
	memset(ink, 0, (size_t)size * (size_t)size * 4);
	before = (HBITMAP)SelectObject(dc, letters);
	font = CreateFontW(-(int)((float)size * (length <= 1 ? 0.72f : length == 2 ? 0.6f : 0.46f)), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI");
	font_before = (HFONT)SelectObject(dc, font);
	SetBkMode(dc, TRANSPARENT);
	SetTextColor(dc, RGB(255, 255, 255));
	rect.left = rect.top = 0;
	rect.right = rect.bottom = size;
	DrawTextW(dc, text, length, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	GdiFlush();
	SelectObject(dc, font_before);
	SelectObject(dc, before);
	DeleteObject(font);

	/* the disc (straight alpha, as icons take it), the letters over it */
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++) {
			float dx = (float)x + 0.5f - radius, dy = (float)y + 0.5f - radius;
			float edge = radius - (float)sqrt(dx * dx + dy * dy);
			float coverage = edge >= 0.5f ? 1.0f : (edge <= -0.5f ? 0.0f : edge + 0.5f);
			const unsigned char* letter = (const unsigned char*)ink + ((size_t)y * (size_t)size + (size_t)x) * 4;
			unsigned char* out = (unsigned char*)pixels + ((size_t)y * (size_t)size + (size_t)x) * 4;
			float white = (float)letter[1] / 255.0f;
			/* red #D92D20 toward white where the letters are */
			out[0] = (unsigned char)(0x20 + (255 - 0x20) * white);
			out[1] = (unsigned char)(0x2D + (255 - 0x2D) * white);
			out[2] = (unsigned char)(0xD9 + (255 - 0xD9) * white);
			out[3] = (unsigned char)(255.0f * coverage + 0.5f);
		}
	}
	memset(&icon, 0, sizeof(icon));
	icon.fIcon = TRUE;
	icon.hbmColor = color;
	icon.hbmMask = mask;
	result = CreateIconIndirect(&icon);

done:
	if (color) DeleteObject(color);
	if (letters) DeleteObject(letters);
	if (mask) DeleteObject(mask);
	DeleteDC(dc);
	ReleaseDC(NULL, screen);
	return result;
}

int rvn_win32_taskbar_set_badge(const char* text) {
	WCHAR wide[16];
	wide[0] = 0;
	if (text && text[0] && MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, (int)(sizeof(wide) / sizeof(wide[0]))) <= 0) return 0;
	if (wcscmp(wide, g_tb.badge) == 0 && (!wide[0] || g_tb.badge_icon)) return 1;
	if (g_tb.badge_icon) DestroyIcon(g_tb.badge_icon);
	g_tb.badge_icon = NULL;
	memcpy(g_tb.badge, wide, sizeof(wide));
	if (wide[0]) g_tb.badge_icon = rvn_tb_badge_icon(wide);
	return !wide[0] || g_tb.badge_icon != NULL;
}

int rvn_win32_taskbar_overlay(HWND window) {
	ITaskbarList3* taskbar = rvn_tb_taskbar();
	if (!taskbar) return 0;
	taskbar->lpVtbl->SetOverlayIcon(taskbar, window, g_tb.badge[0] ? g_tb.badge_icon : NULL, g_tb.badge[0] ? g_tb.badge : NULL);
	return 1;
}

int rvn_win32_taskbar_flash(HWND window, int critical) {
	FLASHWINFO flash;
	if (GetForegroundWindow() == window) return 1;
	memset(&flash, 0, sizeof(flash));
	flash.cbSize = sizeof(flash);
	flash.hwnd = window;
	/* critical: until the window comes to the front; otherwise three times */
	flash.dwFlags = critical ? (FLASHW_TRAY | FLASHW_TIMERNOFG) : FLASHW_TRAY;
	flash.uCount = critical ? 0 : 3;
	FlashWindowEx(&flash);
	return 1;
}

static WCHAR* rvn_tb_widen(const char* text) {
	int length;
	WCHAR* wide;
	if (!text) text = "";
	length = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
	if (length <= 0) return NULL;
	wide = (WCHAR*)CoTaskMemAlloc(sizeof(WCHAR) * (size_t)length);
	if (wide) MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, length);
	return wide;
}

/* A task: a shortcut to this program with its arguments, named by its title. */
static IShellLinkW* rvn_tb_task(const WCHAR* program, const RJumpTask* task) {
	IShellLinkW* link = NULL;
	IPropertyStore* store = NULL;
	WCHAR* arguments = rvn_tb_widen(task->arguments);
	WCHAR* description = rvn_tb_widen(task->description);
	WCHAR* title = rvn_tb_widen(task->title);
	PROPVARIANT value;
	int ok = 0;
	if (!arguments || !description || !title) goto done;
	if (FAILED(CoCreateInstance(&rvn_tb_clsid_link, NULL, CLSCTX_INPROC_SERVER, &rvn_tb_iid_link, (void**)&link))) goto done;
	link->lpVtbl->SetPath(link, program);
	link->lpVtbl->SetArguments(link, arguments);
	link->lpVtbl->SetIconLocation(link, program, 0);
	if (task->description) link->lpVtbl->SetDescription(link, description);
	if (FAILED(link->lpVtbl->QueryInterface(link, &rvn_tb_iid_store, (void**)&store))) goto done;
	PropVariantInit(&value);
	value.vt = VT_LPWSTR;
	value.pwszVal = title;
	ok = SUCCEEDED(store->lpVtbl->SetValue(store, &rvn_tb_key_title, &value)) && SUCCEEDED(store->lpVtbl->Commit(store));
	store->lpVtbl->Release(store);
done:
	CoTaskMemFree(arguments);
	CoTaskMemFree(description);
	CoTaskMemFree(title);
	if (!ok && link) {
		link->lpVtbl->Release(link);
		link = NULL;
	}
	return link;
}

int rvn_win32_taskbar_jump_list(const RJumpTask* tasks, int count) {
	ICustomDestinationList* list = NULL;
	IObjectCollection* collection = NULL;
	IObjectArray* removed = NULL;
	IObjectArray* array = NULL;
	WCHAR program[MAX_PATH];
	UINT slots = 0;
	int i, ok = 0;
	if (!rvn_tb_com()) return 0;
	if (FAILED(CoCreateInstance(&rvn_tb_clsid_destinations, NULL, CLSCTX_INPROC_SERVER, &rvn_tb_iid_destinations, (void**)&list))) return 0;
	if (count == 0) {
		ok = SUCCEEDED(list->lpVtbl->DeleteList(list, NULL));
		list->lpVtbl->Release(list);
		return ok;
	}
	if (!GetModuleFileNameW(NULL, program, MAX_PATH)) goto done;
	if (FAILED(list->lpVtbl->BeginList(list, &slots, &rvn_tb_iid_array, (void**)&removed))) goto done;
	if (FAILED(CoCreateInstance(&rvn_tb_clsid_collection, NULL, CLSCTX_INPROC_SERVER, &rvn_tb_iid_collection, (void**)&collection))) {
		list->lpVtbl->AbortList(list);
		goto done;
	}
	for (i = 0; i < count; i++) {
		IShellLinkW* link = rvn_tb_task(program, &tasks[i]);
		if (!link) continue;
		collection->lpVtbl->AddObject(collection, (IUnknown*)link);
		link->lpVtbl->Release(link);
	}
	if (SUCCEEDED(collection->lpVtbl->QueryInterface(collection, &rvn_tb_iid_array, (void**)&array)) &&
		SUCCEEDED(list->lpVtbl->AddUserTasks(list, array))) {
		ok = SUCCEEDED(list->lpVtbl->CommitList(list));
	} else {
		list->lpVtbl->AbortList(list);
	}
done:
	if (array) array->lpVtbl->Release(array);
	if (collection) collection->lpVtbl->Release(collection);
	if (removed) removed->lpVtbl->Release(removed);
	list->lpVtbl->Release(list);
	return ok;
}

void rvn_win32_taskbar_shutdown(void) {
	if (g_tb.taskbar) g_tb.taskbar->lpVtbl->Release(g_tb.taskbar);
	if (g_tb.badge_icon) DestroyIcon(g_tb.badge_icon);
	/* (COM stays: the other parts of the platform count on it too) */
	g_tb.taskbar = NULL;
	g_tb.taskbar_failed = 0;
	g_tb.badge_icon = NULL;
	g_tb.badge[0] = 0;
}

/* ---------------------------------------------------------------------------
 *  Tray icons
 *
 *  Each icon is an entry of the notification area (NOTIFYICON_VERSION_4),
 *  told to a hidden window: a click (NIN_SELECT) is the left button, the
 *  menu of the system (WM_CONTEXTMENU) shows the menu of the icon at the
 *  place Windows gives, or is the right button when there is none. When
 *  Explorer starts again (TaskbarCreated), the icons are added again.
 * ------------------------------------------------------------------------- */

#ifndef NIN_SELECT
#define NIN_SELECT (WM_USER + 0)
#endif
#ifndef NIN_KEYSELECT
#define NIN_KEYSELECT (WM_USER + 1)
#endif
#ifndef NIF_SHOWTIP
#define NIF_SHOWTIP 0x00000080
#endif
#ifndef NOTIFYICON_VERSION_4
#define NOTIFYICON_VERSION_4 4
#endif

#define RVN_TRAY_MESSAGE (WM_APP + 0x52)
#define RVN_TRAY_CLASS L"RavenTray"

struct rvn_ptray_t {
	void* user;
	UINT id;
	HICON icon;
	WCHAR tip[128];
	HMENU menu;
	struct rvn_ptray_t* next;
};

static struct {
	HWND window;
	void (*emit)(rvn_pevent_t* event);
	UINT next_id;
	UINT taskbar_created;
	rvn_ptray_t* trays;
} g_tray;

void rvn_win32_tray_set_emit(void (*emit)(rvn_pevent_t* event)) {
	g_tray.emit = emit;
}

static rvn_ptray_t* rvn_tray_find(UINT id) {
	rvn_ptray_t* tray;
	for (tray = g_tray.trays; tray && tray->id != id; tray = tray->next) {}
	return tray;
}

static void rvn_tray_emit(rvn_pevent_t* event) {
	if (g_tray.emit) g_tray.emit(event);
}

static void rvn_tray_click(rvn_ptray_t* tray, RMouseButton button) {
	rvn_pevent_t event;
	memset(&event, 0, sizeof(event));
	event.type = RVN_PEVENT_TRAY;
	event.user = tray->user;
	event.button = (int)button;
	rvn_tray_emit(&event);
}

static void rvn_tray_show_menu(rvn_ptray_t* tray, int x, int y) {
	UINT flags = TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_BOTTOMALIGN |
		(GetSystemMetrics(SM_MENUDROPALIGNMENT) ? TPM_RIGHTALIGN : TPM_LEFTALIGN);
	UINT command;
	/* (without it, the menu does not close when the user clicks elsewhere) */
	SetForegroundWindow(g_tray.window);
	command = (UINT)TrackPopupMenuEx(tray->menu, flags, x, y, g_tray.window, NULL);
	PostMessageW(g_tray.window, WM_NULL, 0, 0);
	if (command) {
		rvn_pevent_t event;
		memset(&event, 0, sizeof(event));
		event.type = RVN_PEVENT_COMMAND;
		event.command = (Rid)command;
		rvn_tray_emit(&event);
	}
}

static void rvn_tray_notify(rvn_ptray_t* tray, DWORD action) {
	NOTIFYICONDATAW data;
	memset(&data, 0, sizeof(data));
	data.cbSize = sizeof(data);
	data.hWnd = g_tray.window;
	data.uID = tray->id;
	data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
	data.uCallbackMessage = RVN_TRAY_MESSAGE;
	data.hIcon = tray->icon ? tray->icon : LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
	memcpy(data.szTip, tray->tip, sizeof(data.szTip) < sizeof(tray->tip) ? sizeof(data.szTip) : sizeof(tray->tip));
	data.szTip[(sizeof(data.szTip) / sizeof(data.szTip[0])) - 1] = 0;
	Shell_NotifyIconW(action, &data);
	if (action == NIM_ADD) {
		data.uVersion = NOTIFYICON_VERSION_4;
		Shell_NotifyIconW(NIM_SETVERSION, &data);
	}
}

static LRESULT CALLBACK rvn_tray_proc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam) {
	if (message == RVN_TRAY_MESSAGE) {
		rvn_ptray_t* tray = rvn_tray_find(HIWORD(lparam));
		if (!tray) return 0;
		switch (LOWORD(lparam)) {
			case NIN_SELECT:
			case NIN_KEYSELECT:
				rvn_tray_click(tray, R_MOUSE_LEFT);
				break;
			case WM_CONTEXTMENU:
				if (tray->menu) rvn_tray_show_menu(tray, GET_X_LPARAM(wparam), GET_Y_LPARAM(wparam));
				else rvn_tray_click(tray, R_MOUSE_RIGHT);
				break;
			case WM_MBUTTONUP:
				rvn_tray_click(tray, R_MOUSE_MIDDLE);
				break;
			default:
				break;
		}
		return 0;
	}
	if (g_tray.taskbar_created && message == g_tray.taskbar_created) {
		rvn_ptray_t* tray;
		for (tray = g_tray.trays; tray; tray = tray->next) rvn_tray_notify(tray, NIM_ADD);
		return 0;
	}
	return DefWindowProcW(handle, message, wparam, lparam);
}

/* The hidden window of the icons: a top-level one (a message-only window does not hear TaskbarCreated). */
static int rvn_tray_window(void) {
	WNDCLASSEXW window_class;
	HINSTANCE instance = GetModuleHandleW(NULL);
	if (g_tray.window) return 1;
	memset(&window_class, 0, sizeof(window_class));
	window_class.cbSize = sizeof(window_class);
	window_class.lpfnWndProc = rvn_tray_proc;
	window_class.hInstance = instance;
	window_class.lpszClassName = RVN_TRAY_CLASS;
	RegisterClassExW(&window_class);
	g_tray.window = CreateWindowExW(WS_EX_TOOLWINDOW, RVN_TRAY_CLASS, L"", WS_POPUP, 0, 0, 0, 0, NULL, NULL, instance, NULL);
	g_tray.taskbar_created = RegisterWindowMessageW(L"TaskbarCreated");
	return g_tray.window != NULL;
}

rvn_ptray_t* rvn_win32_tray_create(void* user) {
	rvn_ptray_t* tray;
	if (!rvn_tray_window()) return NULL;
	tray = (rvn_ptray_t*)calloc(1, sizeof(rvn_ptray_t));
	if (!tray) return NULL;
	tray->user = user;
	tray->id = ++g_tray.next_id;
	tray->next = g_tray.trays;
	g_tray.trays = tray;
	rvn_tray_notify(tray, NIM_ADD);
	return tray;
}

void rvn_win32_tray_destroy(rvn_ptray_t* tray) {
	rvn_ptray_t** link;
	NOTIFYICONDATAW data;
	for (link = &g_tray.trays; *link; link = &(*link)->next) {
		if (*link == tray) {
			*link = tray->next;
			break;
		}
	}
	memset(&data, 0, sizeof(data));
	data.cbSize = sizeof(data);
	data.hWnd = g_tray.window;
	data.uID = tray->id;
	Shell_NotifyIconW(NIM_DELETE, &data);
	if (tray->icon) DestroyIcon(tray->icon);
	if (tray->menu) DestroyMenu(tray->menu);
	free(tray);
}

void rvn_win32_tray_set_icon(rvn_ptray_t* tray, const uint8_t* rgba, int width, int height) {
	BITMAPV5HEADER header;
	ICONINFO info;
	void* bits = NULL;
	HDC dc = GetDC(NULL);
	HBITMAP color, mask;
	HICON icon;
	int i;
	memset(&header, 0, sizeof(header));
	header.bV5Size = sizeof(header);
	header.bV5Width = width;
	header.bV5Height = -height;
	header.bV5Planes = 1;
	header.bV5BitCount = 32;
	header.bV5Compression = BI_BITFIELDS;
	header.bV5RedMask = 0x00FF0000;
	header.bV5GreenMask = 0x0000FF00;
	header.bV5BlueMask = 0x000000FF;
	header.bV5AlphaMask = 0xFF000000;
	color = CreateDIBSection(dc, (BITMAPINFO*)&header, DIB_RGB_COLORS, &bits, NULL, 0);
	ReleaseDC(NULL, dc);
	if (!color || !bits) {
		if (color) DeleteObject(color);
		return;
	}
	for (i = 0; i < width * height; i++) {
		unsigned char* out = (unsigned char*)bits + i * 4;
		const uint8_t* in = rgba + i * 4;
		out[0] = in[2];
		out[1] = in[1];
		out[2] = in[0];
		out[3] = in[3];
	}
	mask = CreateBitmap(width, height, 1, 1, NULL);
	memset(&info, 0, sizeof(info));
	info.fIcon = TRUE;
	info.hbmColor = color;
	info.hbmMask = mask;
	icon = CreateIconIndirect(&info);
	DeleteObject(color);
	if (mask) DeleteObject(mask);
	if (!icon) return;
	if (tray->icon) DestroyIcon(tray->icon);
	tray->icon = icon;
	rvn_tray_notify(tray, NIM_MODIFY);
}

void rvn_win32_tray_set_tooltip(rvn_ptray_t* tray, const char* text) {
	tray->tip[0] = 0;
	if (text && text[0] && MultiByteToWideChar(CP_UTF8, 0, text, -1, tray->tip, (int)(sizeof(tray->tip) / sizeof(tray->tip[0]))) <= 0) {
		/* (too long: cut where it fits) */
		MultiByteToWideChar(CP_UTF8, 0, text, 120, tray->tip, 127);
		tray->tip[127] = 0;
	}
	rvn_tray_notify(tray, NIM_MODIFY);
}

/* The text of an entry as a menu of Windows reads it: "&" marks the access key, "&&" is one. */
static WCHAR* rvn_tray_text(const char* text) {
	int length = MultiByteToWideChar(CP_UTF8, 0, text ? text : "", -1, NULL, 0), i, n = 0;
	WCHAR* wide;
	WCHAR* out;
	if (length <= 0) return NULL;
	wide = (WCHAR*)calloc((size_t)length, sizeof(WCHAR));
	out = (WCHAR*)calloc((size_t)length * 2, sizeof(WCHAR));
	if (!wide || !out) {
		free(wide);
		free(out);
		return NULL;
	}
	MultiByteToWideChar(CP_UTF8, 0, text ? text : "", -1, wide, length);
	for (i = 0; wide[i]; i++) {
		if (wide[i] == L'&') out[n++] = L'&';
		out[n++] = wide[i];
	}
	out[n] = 0;
	free(wide);
	return out;
}

static HMENU rvn_tray_menu(const rvn_pmenu_t* items, uint32_t count) {
	HMENU menu = CreatePopupMenu();
	uint32_t i;
	if (!menu) return NULL;
	for (i = 0; i < count; i++) {
		const rvn_pmenu_t* entry = &items[i];
		UINT flags = entry->enabled ? MF_ENABLED : MF_GRAYED;
		WCHAR* text;
		if (entry->separator) {
			AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
			continue;
		}
		text = rvn_tray_text(entry->text);
		if (entry->submenu) {
			HMENU sub = rvn_tray_menu(entry->items, entry->count);
			AppendMenuW(menu, MF_POPUP | flags, (UINT_PTR)sub, text ? text : L"");
		} else {
			/* (the id of the MenuItem is the command: TrackPopupMenuEx gives it back) */
			if (entry->checkable && entry->checked) flags |= MF_CHECKED;
			AppendMenuW(menu, MF_STRING | flags, (UINT_PTR)entry->id, text ? text : L"");
		}
		free(text);
	}
	return menu;
}

void rvn_win32_tray_set_menu(rvn_ptray_t* tray, const rvn_pmenu_t* items, uint32_t count) {
	if (tray->menu) DestroyMenu(tray->menu);
	tray->menu = count > 0 ? rvn_tray_menu(items, count) : NULL;
}

void rvn_win32_tray_shutdown(void) {
	while (g_tray.trays) rvn_win32_tray_destroy(g_tray.trays);
	if (g_tray.window) DestroyWindow(g_tray.window);
	UnregisterClassW(RVN_TRAY_CLASS, GetModuleHandleW(NULL));
	memset(&g_tray, 0, sizeof(g_tray));
}
