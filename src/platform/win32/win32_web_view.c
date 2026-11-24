/**
 * Raven - platform/win32/win32_web_view.c
 * The web view of the system on Windows: Microsoft Edge WebView2 kept over a
 * SystemWebView widget (see widgets/system_web_view.c), for the Win32
 * platform. platform/apple/webkit_web_view.m does the same with WKWebView.
 *
 * Nothing of WebView2 is needed to build or link Raven: the interfaces used
 * are declared in webview2_com.h, and the runtime is found when the first
 * view is made, in this order:
 *
 *   1. WebView2Loader.dll next to the executable, when the application ships
 *      it (the loader of the WebView2 SDK): its documented
 *      CreateCoreWebView2EnvironmentWithOptions is used, which also knows the
 *      group policies, the fixed version runtimes and the packaged ones.
 *   2. Otherwise, as the loader does it itself: the installed Evergreen
 *      WebView2 Runtime (stable channel), whose folder is the value
 *      "EBWebView" of HKLM, then HKCU,
 *      SOFTWARE\Microsoft\EdgeUpdate\ClientState\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}
 *      (32-bit view of the registry), e.g. "C:\Program Files (x86)\Microsoft\
 *      EdgeWebView\Application\131.0.2903.70"; or the folder of a fixed
 *      version runtime given by WEBVIEW2_BROWSER_EXECUTABLE_FOLDER. The DLL
 *      EBWebView\<x64|x86|arm64>\EmbeddedBrowserWebView.dll of that folder
 *      exports CreateWebViewEnvironmentWithOptionsInternal(bool, runtime type
 *      (0 installed, 1 fixed version), user data folder, options, handler),
 *      which is what the loader calls. That function is not documented: its
 *      signature and use are those of the open source loaders,
 *      OpenWebView2Loader (ISC, github.com/jchv/OpenWebView2Loader,
 *      Source/WebView2Loader.cpp: CreateWebViewEnvironmentWithClientDll,
 *      FindInstalledClientDllForChannel) and webview (MIT,
 *      github.com/webview/webview,
 *      core/include/webview/detail/platform/windows/webview2/loader.hh).
 *
 * One environment is made for the process (asynchronously), then one
 * controller per view (asynchronously too): what the view is asked before it
 * is ready (an address, a page, scripts) waits and is done then, in order.
 * The user data folder (cookies, cache) is %LOCALAPPDATA%\<name of the
 * executable>\WebView2, unless WEBVIEW2_USER_DATA_FOLDER says otherwise.
 *
 * The view sits in a container window of its own, a child of the Raven
 * window: the container is the part of the widget that is seen, the WebView2
 * inside it the whole widget, so that a ScrollBox cuts the page as it cuts
 * the other widgets (the approach of webkit_web_view.m). Out of any window,
 * the container waits in a hidden window of its own.
 *
 * What WebView2 tells (title, address, loading, messages, results) is sent to
 * the core later, from a message posted to that hidden window, never during
 * a call of the core into the view: WebView2 may raise an event inside
 * Navigate.
 *
 * Scripts run with ExecuteScript inside a wrapper that evaluates them (a
 * promise is waited for, its result coming back as a message of the page)
 * and catches what they throw; on a page whose Content-Security-Policy
 * forbids eval, the script runs as it is. Pages given as text with an
 * http(s) base address are served to WebView2 at that address
 * (WebResourceRequested), so that the address, the origin and the relative
 * links are the base's, and there is no limit of size; without one they go
 * through NavigateToString (2 MB at most).
 *
 * RAVEN_WEBVIEW2_LOG=1 prints each step (the runtime found, the HRESULTs) on
 * stderr.
 */

#include "win32_web_view.h"

#if defined(_WIN32)

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "webview2_com.h"

#define RVN_WV2_CLASS L"Raven_WebView2"
#define RVN_WV2_HIDDEN_CLASS L"Raven_WebView2_Hidden"
/** Posted to the hidden window: events wait to be told to the core. */
#define RVN_WV2_DRAIN (WM_APP + 0x57)
#define RVN_WV2_PATH 1024
/** The stable channel of the Evergreen runtime. */
#define RVN_WV2_CLIENT_STATE L"SOFTWARE\\Microsoft\\EdgeUpdate\\ClientState\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"
/* (the first runtime the loader of the SDK accepts: 86.0.616.0) */
#define RVN_WV2_MINIMUM_MAJOR 86

#if defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__)
#define RVN_WV2_CLIENT_DLL L"\\EBWebView\\x64\\EmbeddedBrowserWebView.dll"
#elif defined(_M_ARM64) || defined(__aarch64__)
#define RVN_WV2_CLIENT_DLL L"\\EBWebView\\arm64\\EmbeddedBrowserWebView.dll"
#elif defined(_M_IX86) || defined(__i386__)
#define RVN_WV2_CLIENT_DLL L"\\EBWebView\\x86\\EmbeddedBrowserWebView.dll"
#endif

/* GetProcAddress returns a generic function pointer: convert it without a warning. */
#define RVN_WV2_FUNCTION(type, pointer) ((type)(void (*)(void))(pointer))

/* CreateWebViewEnvironmentWithOptionsInternal of EmbeddedBrowserWebView.dll (the first argument is a C++ bool, true) */
typedef HRESULT (STDMETHODCALLTYPE *rvn_wv2_create_internal_fn)(int unknown, int runtime_type, LPCWSTR user_data_folder,
	IUnknown* options, rvn_wv2_environment_done_t* handler);
/* CreateCoreWebView2EnvironmentWithOptions and GetAvailableCoreWebView2BrowserVersionString of WebView2Loader.dll (STDAPI) */
typedef HRESULT (STDMETHODCALLTYPE *rvn_wv2_create_fn)(LPCWSTR browser_folder, LPCWSTR user_data_folder, IUnknown* options,
	rvn_wv2_environment_done_t* handler);
typedef HRESULT (STDMETHODCALLTYPE *rvn_wv2_version_fn)(LPCWSTR browser_folder, LPWSTR* version);
/* SHCreateMemStream of shlwapi.dll */
typedef IStream* (WINAPI *rvn_wv2_memory_stream_fn)(const BYTE* data, UINT size);

/* window.raven.postMessage(value) in each page (the main frame), before its own scripts: a string as it is, anything else as JSON. */
static const char RVN_WV2_BRIDGE_SCRIPT[] =
	"(function () {"
	"  if (window.top !== window) return;"
	"  var host = window.chrome && window.chrome.webview;"
	"  if (!host) return;"
	"  var raven = { postMessage: function (value) {"
	"    host.postMessage(typeof value === 'string' ? value : JSON.stringify(value === undefined ? null : value));"
	"  } };"
	"  Object.defineProperty(window, 'raven', { value: Object.freeze(raven), writable: false, configurable: false });"
	"})();";

/*
 * The wrapper of Rsystem_web_view_run_javascript: %1 the token of the view,
 * %2 the request, %3 the script as a string of JavaScript. Its value (what
 * ExecuteScript gives back, as JSON) is a string: "v" + the value as JSON,
 * "e" + the error, "a" (a promise: its end comes as a message
 * "\1raven-result:<token>:<request>:" + "v" or "e"...), or "n" (eval is
 * forbidden here: run the script as it is). Indirect eval runs the script as
 * a program of the page and gives its last value, as on macOS.
 */
static const char RVN_WV2_RUN_SCRIPT[] =
	"(function () {"
	"  var post = function (kind, value) { try { window.chrome.webview.postMessage('\\u0001raven-result:%1:%2:' + kind + value); } catch (x) {} };"
	"  var text = function (e) { try { var t = String(e); return t ? t : 'the script failed'; } catch (x) { return 'the script failed'; } };"
	"  var json = function (v) { try { var j = JSON.stringify(v === undefined ? null : v); return j === undefined ? 'null' : j; } catch (x) { return 'null'; } };"
	"  var r;"
	"  try { r = (0, eval)(%3); }"
	"  catch (e) {"
	"    if (e instanceof EvalError && /unsafe-eval|Content Security Policy/.test(String(e.message))) return 'n';"
	"    return 'e' + text(e);"
	"  }"
	"  if (r !== null && (typeof r === 'object' || typeof r === 'function') && typeof r.then === 'function') {"
	"    Promise.resolve(r).then(function (v) { post('v', json(v)); }, function (e) { post('e', text(e)); });"
	"    return 'a';"
	"  }"
	"  return 'v' + json(r);"
	"})()";

static const char RVN_WV2_RESULT_PREFIX[] = "\x01" "raven-result:";

static const IID rvn_wv2_iid_unknown = { 0x00000000, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

/* The events of ICoreWebView2 a view listens to. */
enum {
	RVN_WV2_ON_STARTING,
	RVN_WV2_ON_COMPLETED,
	RVN_WV2_ON_SOURCE,
	RVN_WV2_ON_TITLE,
	RVN_WV2_ON_MESSAGE,
	RVN_WV2_ON_NEW_WINDOW,
	RVN_WV2_ON_PROCESS,
	RVN_WV2_ON_RESOURCE,
	RVN_WV2_ON_COUNT
};

/* Where a view is. */
enum {
	/** Waits for the environment of the process. */
	RVN_WV2_WAITING,
	/** Its controller is being made. */
	RVN_WV2_CREATING,
	/** window.raven is being added to the pages. */
	RVN_WV2_PREPARING,
	RVN_WV2_READY,
	/** WebView2 could not be started: everything asked fails. */
	RVN_WV2_FAILED
};

/* Where the environment of the process is. */
enum {
	RVN_WV2_ENV_NONE,
	RVN_WV2_ENV_CREATING,
	RVN_WV2_ENV_READY
};

/* A handler of WebView2 that is a part of a view (it holds the view alive through the count of the view). */
typedef struct rvn_wv2_event_part_t {
	rvn_wv2_event_handler_t iface;
	struct rvn_pweb_view_t* view;
	int kind;
} rvn_wv2_event_part_t;

typedef struct rvn_wv2_controller_part_t {
	rvn_wv2_controller_done_t iface;
	struct rvn_pweb_view_t* view;
} rvn_wv2_controller_part_t;

typedef struct rvn_wv2_bridge_part_t {
	rvn_wv2_string_done_t iface;
	struct rvn_pweb_view_t* view;
} rvn_wv2_bridge_part_t;

/* A script that waits for the view to be ready. */
typedef struct rvn_wv2_script_t {
	char* script;
	uint64_t request;
} rvn_wv2_script_t;

struct rvn_pweb_view_t {
	LONG refs;
	void* user;
	int closed;
	int state;
	struct rvn_pweb_view_t* next;

	HWND container;
	rvn_wv2_controller_t* controller;
	rvn_wv2_core_t* core;

	rvn_wv2_controller_part_t controller_part;
	rvn_wv2_bridge_part_t bridge_part;
	rvn_wv2_event_part_t on[RVN_WV2_ON_COUNT];
	rvn_wv2_token_t tokens[RVN_WV2_ON_COUNT];
	int listening[RVN_WV2_ON_COUNT];

	/** Where the page is in the container (pixels), and whether it is shown. */
	RECT bounds;
	int visible;

	/** What was asked before the view was ready: the last address or page, then the scripts. */
	int pending;
	char* pending_url;
	char* pending_html;
	char* pending_base;
	rvn_wv2_script_t* scripts;
	uint32_t script_count, script_cap;

	/** The page given as text with an http(s) base address, served at that address. */
	char* html;
	size_t html_size;
	WCHAR* html_uri;
	/** The address as WebView2 wrote it when it went there, and whether the next navigation is the one. */
	WCHAR* html_seen;
	int html_expect;
	int html_filter;

	/** Scripts whose promise is waited for: they fail when the page changes. */
	uint64_t* waiting;
	uint32_t waiting_count, waiting_cap;
	/** Written in the results the page sends, so that other messages are not taken for them. */
	char token[17];
};

enum { RVN_WV2_PENDING_NONE, RVN_WV2_PENDING_URL, RVN_WV2_PENDING_HTML };

/* An event that waits to be told to the core. */
typedef struct rvn_wv2_event_t {
	struct rvn_wv2_event_t* next;
	rvn_pweb_view_t* view;
	void* user;
	rvn_pweb_event_type_t type;
	char* text;
	char* error;
	uint64_t request;
} rvn_wv2_event_t;

static struct {
	/** rvn_wv2_supported was asked, and what it found. */
	int checked;
	int available;
	/** 1: the loader next to the program; else the DLL of the runtime and its type. */
	HMODULE loader;
	rvn_wv2_create_fn loader_create;
	WCHAR client[RVN_WV2_PATH];
	int runtime_type;
	HMODULE client_module;
	/** 0: not tried, 1: COM is ready on this thread, -1: it cannot be (not a single-threaded apartment). */
	int com;
	int env_state;
	rvn_wv2_environment_t* environment;
	WCHAR* user_data;
	HMODULE shlwapi;
	rvn_wv2_memory_stream_fn memory_stream;
	/** The window posted to, parent of the containers out of any window. */
	HWND hidden;
	int classes;
	rvn_pweb_view_t* views;
	rvn_wv2_event_t* head;
	rvn_wv2_event_t* tail;
	/** A call of the core into a view is running; a drain was posted; one came while busy. */
	int busy;
	int drain_posted;
	int drain_missed;
	int log;
} g_wv2;

/* ---------------------------------------------------------------------------
 *  Helpers
 * ------------------------------------------------------------------------- */

static void rvn_wv2_log(const char* format, ...) {
	va_list args;
	if (g_wv2.log == 0) {
		char value[8];
		DWORD length = GetEnvironmentVariableA("RAVEN_WEBVIEW2_LOG", value, (DWORD)sizeof(value));
		g_wv2.log = (length > 0 && length < sizeof(value) && value[0] != '0') ? 1 : -1;
	}
	if (g_wv2.log < 0) return;
	va_start(args, format);
	fputs("[raven webview2] ", stderr);
	vfprintf(stderr, format, args);
	fputc('\n', stderr);
	fflush(stderr);
	va_end(args);
}

/* UTF-8 -> UTF-16. Free with rvn_free. */
static WCHAR* rvn_wv2_widen(const char* text) {
	int count;
	WCHAR* wide;
	if (!text) text = "";
	count = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
	wide = (WCHAR*)rvn_calloc(sizeof(WCHAR) * (size_t)(count > 0 ? count : 1));
	if (count > 0) MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, count);
	return wide;
}

/* UTF-16 -> UTF-8. Free with rvn_free. */
static char* rvn_wv2_narrow(const WCHAR* wide) {
	int count;
	char* text;
	if (!wide) wide = L"";
	count = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
	text = (char*)rvn_calloc((size_t)(count > 0 ? count : 1));
	if (count > 0) WideCharToMultiByte(CP_UTF8, 0, wide, -1, text, count, NULL, NULL);
	return text;
}

/* A string WebView2 gave (CoTaskMemAlloc'ed), in UTF-8 (rvn_free), and freed. */
static char* rvn_wv2_take(LPWSTR wide) {
	char* text = rvn_wv2_narrow(wide);
	if (wide) CoTaskMemFree(wide);
	return text;
}

static WCHAR* rvn_wv2_wcsdup(const WCHAR* text) {
	size_t length = text ? wcslen(text) : 0;
	WCHAR* copy = (WCHAR*)rvn_alloc(sizeof(WCHAR) * (length + 1));
	if (length) memcpy(copy, text, sizeof(WCHAR) * length);
	copy[length] = 0;
	return copy;
}

/* a + b + c, NULL taken as "". Free with rvn_free. */
static WCHAR* rvn_wv2_join(const WCHAR* a, const WCHAR* b, const WCHAR* c) {
	size_t la = a ? wcslen(a) : 0, lb = b ? wcslen(b) : 0, lc = c ? wcslen(c) : 0;
	WCHAR* out = (WCHAR*)rvn_alloc(sizeof(WCHAR) * (la + lb + lc + 1));
	if (la) memcpy(out, a, sizeof(WCHAR) * la);
	if (lb) memcpy(out + la, b, sizeof(WCHAR) * lb);
	if (lc) memcpy(out + la + lb, c, sizeof(WCHAR) * lc);
	out[la + lb + lc] = 0;
	return out;
}

static int rvn_wv2_same_iid(REFIID a, const IID* b) {
	return memcmp(a, b, sizeof(IID)) == 0;
}

static int rvn_wv2_starts_with(const char* text, const char* prefix) {
	size_t i;
	for (i = 0; prefix[i]; i++) {
		char c = text[i];
		if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
		if (c != prefix[i]) return 0;
	}
	return 1;
}

/*
 * The objects WebView2 holds (a view, a script running) may be released by
 * it after Raven has shut down: they are allocated with the C runtime, not
 * with rvn_alloc, whose count must be zero by then.
 */
static char* rvn_wv2_crt_strdup(const char* text) {
	size_t size = strlen(text ? text : "") + 1;
	char* copy = (char*)malloc(size);
	if (copy) memcpy(copy, text ? text : "", size);
	return copy;
}

static void rvn_wv2_view_add_ref(rvn_pweb_view_t* view) {
	InterlockedIncrement(&view->refs);
}

static void rvn_wv2_view_release(rvn_pweb_view_t* view) {
	/* (everything else was freed by rvn_wv2_destroy: what WebView2 still holds only reaches `closed`) */
	if (InterlockedDecrement(&view->refs) == 0) free(view);
}

/* ---------------------------------------------------------------------------
 *  Events to the core: queued, told from the loop of messages
 * ------------------------------------------------------------------------- */

static void rvn_wv2_post_drain(void) {
	if (g_wv2.drain_posted || !g_wv2.hidden) return;
	if (PostMessageW(g_wv2.hidden, RVN_WV2_DRAIN, 0, 0)) g_wv2.drain_posted = 1;
	else rvn_wv2_log("PostMessage to the hidden window failed (error %lu)", (unsigned long)GetLastError());
}

static const char* rvn_wv2_event_name(rvn_pweb_event_type_t type) {
	switch (type) {
		case RVN_PWEB_URL: return "url";
		case RVN_PWEB_TITLE: return "title";
		case RVN_PWEB_LOADING: return "loading";
		case RVN_PWEB_LOADED: return "loaded";
		case RVN_PWEB_FAILED: return "failed";
		case RVN_PWEB_MESSAGE: return "message";
		case RVN_PWEB_RESULT: return "result";
		default: return "?";
	}
}

static void rvn_wv2_send(rvn_pweb_view_t* view, rvn_pweb_event_type_t type, const char* text, const char* error, uint64_t request) {
	rvn_wv2_event_t* event;
	if (!view || view->closed) return;
	rvn_wv2_log("view %p: queued %s%s%.120s%s%.120s (request %lu)", (void*)view, rvn_wv2_event_name(type), text ? " text=" : "",
		text ? text : "", error ? " error=" : "", error ? error : "", (unsigned long)request);
	event = (rvn_wv2_event_t*)rvn_calloc(sizeof(rvn_wv2_event_t));
	event->view = view;
	event->user = view->user;
	event->type = type;
	event->text = text ? rvn_strdup(text) : NULL;
	event->error = error ? rvn_strdup(error) : NULL;
	event->request = request;
	if (g_wv2.tail) g_wv2.tail->next = event;
	else g_wv2.head = event;
	g_wv2.tail = event;
	rvn_wv2_post_drain();
}

static void rvn_wv2_free_event(rvn_wv2_event_t* event) {
	rvn_free(event->text);
	rvn_free(event->error);
	rvn_free(event);
}

/* The events of a view that goes are forgotten. */
static void rvn_wv2_forget_events(rvn_pweb_view_t* view) {
	rvn_wv2_event_t* previous = NULL;
	rvn_wv2_event_t* event = g_wv2.head;
	while (event) {
		rvn_wv2_event_t* next = event->next;
		if (event->view == view) {
			if (previous) previous->next = next;
			else g_wv2.head = next;
			if (g_wv2.tail == event) g_wv2.tail = previous;
			rvn_wv2_free_event(event);
		} else {
			previous = event;
		}
		event = next;
	}
}

/* Tells the core what waited (the events there when it starts; those they cause go next time). */
static void rvn_wv2_drain(void) {
	rvn_wv2_event_t* last = g_wv2.tail;
	g_wv2.drain_posted = 0;
	if (g_wv2.busy) {
		rvn_wv2_log("drain while the core calls a view: later");
		g_wv2.drain_missed = 1;
		return;
	}
	while (g_wv2.head) {
		rvn_wv2_event_t* event = g_wv2.head;
		int was_last = event == last;
		rvn_pweb_event_t told;
		g_wv2.head = event->next;
		if (!g_wv2.head) g_wv2.tail = NULL;
		memset(&told, 0, sizeof(told));
		told.type = event->type;
		told.user = event->user;
		told.text = event->text;
		told.error = event->error;
		told.request = event->request;
		/* (the callbacks may destroy views: their events are taken out of the queue) */
		rvn_wv2_log("view %p: told to the core: %s", (void*)event->view, rvn_wv2_event_name(event->type));
		rvn_web_view_event(&told);
		rvn_wv2_free_event(event);
		if (was_last) break;
	}
	if (g_wv2.head) rvn_wv2_post_drain();
}

/* Around each call of the core into a view: what WebView2 says meanwhile waits. */
static void rvn_wv2_enter(void) {
	g_wv2.busy++;
}

static void rvn_wv2_leave(void) {
	if (--g_wv2.busy == 0 && g_wv2.drain_missed) {
		g_wv2.drain_missed = 0;
		rvn_wv2_post_drain();
	}
}

/* ---------------------------------------------------------------------------
 *  Windows: the hidden one, the containers
 * ------------------------------------------------------------------------- */

static LRESULT CALLBACK rvn_wv2_hidden_proc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam) {
	if (message == RVN_WV2_DRAIN) {
		rvn_wv2_drain();
		return 0;
	}
	return DefWindowProcW(handle, message, wparam, lparam);
}

static LRESULT CALLBACK rvn_wv2_container_proc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam) {
	return DefWindowProcW(handle, message, wparam, lparam);
}

static int rvn_wv2_windows(void) {
	HINSTANCE instance = GetModuleHandleW(NULL);
	WNDCLASSEXW window_class;
	if (g_wv2.hidden) return 1;
	if (!g_wv2.classes) {
		memset(&window_class, 0, sizeof(window_class));
		window_class.cbSize = sizeof(window_class);
		window_class.lpfnWndProc = rvn_wv2_hidden_proc;
		window_class.hInstance = instance;
		window_class.lpszClassName = RVN_WV2_HIDDEN_CLASS;
		if (!RegisterClassExW(&window_class)) {
			rvn_wv2_log("cannot register the hidden window class (error %lu)", (unsigned long)GetLastError());
			return 0;
		}
		memset(&window_class, 0, sizeof(window_class));
		window_class.cbSize = sizeof(window_class);
		window_class.lpfnWndProc = rvn_wv2_container_proc;
		window_class.hInstance = instance;
		window_class.hCursor = LoadCursorW(NULL, IDC_ARROW);
		/* (seen only until the page is drawn) */
		window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		window_class.lpszClassName = RVN_WV2_CLASS;
		if (!RegisterClassExW(&window_class)) {
			rvn_wv2_log("cannot register the container class (error %lu)", (unsigned long)GetLastError());
			UnregisterClassW(RVN_WV2_HIDDEN_CLASS, instance);
			return 0;
		}
		g_wv2.classes = 1;
	}
	/* a top-level window that is never shown: a message-only window could not be the parent of a WebView2 */
	g_wv2.hidden = CreateWindowExW(WS_EX_TOOLWINDOW, RVN_WV2_HIDDEN_CLASS, L"", WS_POPUP, 0, 0, 0, 0, NULL, NULL, instance, NULL);
	if (!g_wv2.hidden) rvn_wv2_log("cannot create the hidden window (error %lu)", (unsigned long)GetLastError());
	return g_wv2.hidden != NULL;
}

/* ---------------------------------------------------------------------------
 *  Finding the runtime
 * ------------------------------------------------------------------------- */

/* WebView2Loader.dll next to the executable. */
static int rvn_wv2_find_loader(void) {
	WCHAR path[RVN_WV2_PATH];
	static const WCHAR name[] = L"WebView2Loader.dll";
	DWORD length = GetModuleFileNameW(NULL, path, RVN_WV2_PATH);
	WCHAR* slash;
	HMODULE module;
	rvn_wv2_create_fn create;
	rvn_wv2_version_fn version;
	LPWSTR info = NULL;
	HRESULT result;
	if (length == 0 || length >= RVN_WV2_PATH) return 0;
	slash = wcsrchr(path, L'\\');
	if (!slash || (size_t)(slash + 1 - path) + sizeof(name) / sizeof(WCHAR) > RVN_WV2_PATH) return 0;
	memcpy(slash + 1, name, sizeof(name));
	if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) {
		rvn_wv2_log("no WebView2Loader.dll next to the program: looking for the installed runtime");
		return 0;
	}
	module = LoadLibraryW(path);
	if (!module) {
		rvn_wv2_log("WebView2Loader.dll cannot be loaded (error %lu)", (unsigned long)GetLastError());
		return 0;
	}
	create = RVN_WV2_FUNCTION(rvn_wv2_create_fn, GetProcAddress(module, "CreateCoreWebView2EnvironmentWithOptions"));
	version = RVN_WV2_FUNCTION(rvn_wv2_version_fn, GetProcAddress(module, "GetAvailableCoreWebView2BrowserVersionString"));
	if (!create || !version) {
		rvn_wv2_log("WebView2Loader.dll lacks CreateCoreWebView2EnvironmentWithOptions");
		FreeLibrary(module);
		return 0;
	}
	result = version(NULL, &info);
	if (FAILED(result) || !info) {
		rvn_wv2_log("WebView2Loader.dll finds no runtime (0x%08lX)", (unsigned long)result);
		if (info) CoTaskMemFree(info);
		FreeLibrary(module);
		return 0;
	}
	{
		char* text = rvn_wv2_take(info);
		rvn_wv2_log("WebView2Loader.dll next to the program, runtime %s", text);
		rvn_free(text);
	}
	g_wv2.loader = module;
	g_wv2.loader_create = create;
	return 1;
}

/* The DLL of a runtime in `folder` (its version folder, or a fixed version runtime). */
static int rvn_wv2_client_in(const WCHAR* folder, int runtime_type) {
#ifdef RVN_WV2_CLIENT_DLL
	WCHAR* path = rvn_wv2_join(folder, RVN_WV2_CLIENT_DLL, NULL);
	char* text = rvn_wv2_narrow(path);
	int found = wcslen(path) < RVN_WV2_PATH && GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
	rvn_wv2_log("%s %s", found ? "runtime:" : "no runtime at", text);
	if (found) {
		memcpy(g_wv2.client, path, sizeof(WCHAR) * (wcslen(path) + 1));
		g_wv2.runtime_type = runtime_type;
	}
	rvn_free(text);
	rvn_free(path);
	return found;
#else
	(void)folder;
	(void)runtime_type;
	rvn_wv2_log("no WebView2 runtime for this processor");
	return 0;
#endif
}

/* The installed Evergreen runtime, as the registry gives it under `root`. */
static int rvn_wv2_find_installed(HKEY root, const char* root_name) {
	HKEY key;
	WCHAR value[RVN_WV2_PATH];
	DWORD type = 0, size = (DWORD)(sizeof(value) - sizeof(WCHAR));
	LONG status;
	size_t length;
	WCHAR* version;
	char* text;
	long major;
	status = RegOpenKeyExW(root, RVN_WV2_CLIENT_STATE, 0, KEY_READ | KEY_WOW64_32KEY, &key);
	if (status != ERROR_SUCCESS) {
		rvn_wv2_log("%s: no WebView2 runtime registered (%ld)", root_name, (long)status);
		return 0;
	}
	memset(value, 0, sizeof(value));
	status = RegQueryValueExW(key, L"EBWebView", NULL, &type, (BYTE*)value, &size);
	RegCloseKey(key);
	if (status != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ) || !value[0]) {
		rvn_wv2_log("%s: no EBWebView value (%ld)", root_name, (long)status);
		return 0;
	}
	value[RVN_WV2_PATH - 1] = 0;
	length = wcslen(value);
	while (length > 0 && (value[length - 1] == L'\\' || value[length - 1] == L'/')) value[--length] = 0;
	version = wcsrchr(value, L'\\');
	version = version ? version + 1 : value;
	major = wcstol(version, NULL, 10);
	text = rvn_wv2_narrow(value);
	rvn_wv2_log("%s: EBWebView = %s", root_name, text);
	rvn_free(text);
	if (major < RVN_WV2_MINIMUM_MAJOR) {
		rvn_wv2_log("%s: runtime too old or not installed (%ld)", root_name, major);
		return 0;
	}
	return rvn_wv2_client_in(value, 0);
}

static int rvn_wv2_find_runtime(void) {
	WCHAR folder[RVN_WV2_PATH];
	DWORD length = GetEnvironmentVariableW(L"WEBVIEW2_BROWSER_EXECUTABLE_FOLDER", folder, RVN_WV2_PATH);
	if (length > 0 && length < RVN_WV2_PATH) {
		while (length > 0 && (folder[length - 1] == L'\\' || folder[length - 1] == L'/')) folder[--length] = 0;
		/* a fixed version runtime */
		if (rvn_wv2_client_in(folder, 1)) return 1;
	}
	if (rvn_wv2_find_installed(HKEY_LOCAL_MACHINE, "HKLM")) return 1;
	if (rvn_wv2_find_installed(HKEY_CURRENT_USER, "HKCU")) return 1;
	rvn_wv2_log("the Microsoft Edge WebView2 Runtime is not installed");
	return 0;
}

int rvn_wv2_supported(void) {
	if (!g_wv2.checked) {
		g_wv2.checked = 1;
		g_wv2.available = rvn_wv2_find_loader() || rvn_wv2_find_runtime();
	}
	return g_wv2.available;
}

/* COM in a single-threaded apartment on this thread, as WebView2 requires. */
static int rvn_wv2_com(void) {
	if (g_wv2.com == 0) {
		/* S_FALSE: the application (or another part of Raven) had done it already; it is counted all the same */
		HRESULT result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		g_wv2.com = (result == S_OK || result == S_FALSE) ? 1 : -1;
		rvn_wv2_log("CoInitializeEx: 0x%08lX%s", (unsigned long)result,
			result == RPC_E_CHANGED_MODE ? " (the thread is in the multithreaded apartment: WebView2 needs a single-threaded one)" : "");
	}
	return g_wv2.com > 0;
}

/* %LOCALAPPDATA%\<name of the executable>\WebView2 (the default, next to the executable, may not be writable). */
static WCHAR* rvn_wv2_user_data_folder(void) {
	WCHAR base[RVN_WV2_PATH], exe[RVN_WV2_PATH];
	WCHAR* name;
	WCHAR* folder;
	WCHAR* result;
	size_t length;
	DWORD count = GetEnvironmentVariableW(L"WEBVIEW2_USER_DATA_FOLDER", base, RVN_WV2_PATH);
	if (count > 0 && count < RVN_WV2_PATH) return rvn_wv2_wcsdup(base);
	count = GetEnvironmentVariableW(L"LOCALAPPDATA", base, RVN_WV2_PATH);
	if (count == 0 || count >= RVN_WV2_PATH) {
		count = GetTempPathW(RVN_WV2_PATH, base);
		if (count == 0 || count >= RVN_WV2_PATH) return NULL;
	}
	length = wcslen(base);
	while (length > 0 && (base[length - 1] == L'\\' || base[length - 1] == L'/')) base[--length] = 0;
	count = GetModuleFileNameW(NULL, exe, RVN_WV2_PATH);
	exe[RVN_WV2_PATH - 1] = 0;
	if (count == 0 || count >= RVN_WV2_PATH) exe[0] = 0;
	name = wcsrchr(exe, L'\\');
	name = name ? name + 1 : exe;
	length = wcslen(name);
	if (length > 4 && _wcsicmp(name + length - 4, L".exe") == 0) name[length - 4] = 0;
	if (!name[0]) name = (WCHAR*)L"Raven";
	folder = rvn_wv2_join(base, L"\\", name);
	/* (WebView2 makes the last folder itself) */
	CreateDirectoryW(folder, NULL);
	result = rvn_wv2_join(folder, L"\\WebView2", NULL);
	rvn_free(folder);
	return result;
}

/* ---------------------------------------------------------------------------
 *  Script results
 * ------------------------------------------------------------------------- */

/* A JSON string ("...") as UTF-8 text; NULL when `json` is not one. */
static char* rvn_wv2_json_string(const WCHAR* json) {
	size_t length, i = 1, o = 0;
	WCHAR* out;
	char* text;
	if (!json || json[0] != L'"') return NULL;
	length = wcslen(json);
	out = (WCHAR*)rvn_alloc(sizeof(WCHAR) * (length + 1));
	while (i < length && json[i] != L'"') {
		WCHAR c = json[i++];
		if (c == L'\\') {
			if (i >= length) break;
			c = json[i++];
			switch (c) {
				case L'b': c = 0x08; break;
				case L'f': c = 0x0C; break;
				case L'n': c = 0x0A; break;
				case L'r': c = 0x0D; break;
				case L't': c = 0x09; break;
				case L'u': {
					unsigned value = 0;
					int k;
					for (k = 0; k < 4 && i < length; k++, i++) {
						WCHAR h = json[i];
						value <<= 4;
						if (h >= L'0' && h <= L'9') value |= (unsigned)(h - L'0');
						else if (h >= L'a' && h <= L'f') value |= (unsigned)(h - L'a' + 10);
						else if (h >= L'A' && h <= L'F') value |= (unsigned)(h - L'A' + 10);
						else break;
					}
					if (k < 4) {
						rvn_free(out);
						return NULL;
					}
					/* (a surrogate pair is two \u: UTF-16 puts them back together) */
					c = (WCHAR)value;
					break;
				}
				default:
					/* \" \\ \/ */
					break;
			}
		}
		out[o++] = c;
	}
	if (i >= length) {
		rvn_free(out);
		return NULL;
	}
	out[o] = 0;
	text = rvn_wv2_narrow(out);
	rvn_free(out);
	return text;
}

/* `text` as a string of JavaScript ("..."). Free with rvn_free. */
static char* rvn_wv2_js_string(const char* text) {
	static const char hex[] = "0123456789abcdef";
	size_t length = strlen(text), i, o = 0;
	char* out = (char*)rvn_alloc(length * 6 + 3);
	out[o++] = '"';
	for (i = 0; i < length; i++) {
		unsigned char c = (unsigned char)text[i];
		if (c == '"' || c == '\\') {
			out[o++] = '\\';
			out[o++] = (char)c;
		} else if (c < 0x20) {
			out[o++] = '\\';
			out[o++] = 'u';
			out[o++] = '0';
			out[o++] = '0';
			out[o++] = hex[c >> 4];
			out[o++] = hex[c & 15];
		} else if (c == 0xE2 && i + 2 < length && (unsigned char)text[i + 1] == 0x80 &&
			((unsigned char)text[i + 2] == 0xA8 || (unsigned char)text[i + 2] == 0xA9)) {
			/* U+2028 and U+2029 end a line in older JavaScript strings */
			memcpy(out + o, (unsigned char)text[i + 2] == 0xA8 ? "\\u2028" : "\\u2029", 6);
			o += 6;
			i += 2;
		} else {
			out[o++] = (char)c;
		}
	}
	out[o++] = '"';
	out[o] = '\0';
	return out;
}

static void rvn_wv2_u64(char out[24], uint64_t value) {
	char digits[24];
	int n = 0, i;
	do {
		digits[n++] = (char)('0' + (int)(value % 10));
		value /= 10;
	} while (value && n < 23);
	for (i = 0; i < n; i++) out[i] = digits[n - 1 - i];
	out[n] = '\0';
}

/* The wrapper of a script (RVN_WV2_RUN_SCRIPT with its holes filled). Free with rvn_free. */
static char* rvn_wv2_wrap(rvn_pweb_view_t* view, const char* script, uint64_t request) {
	char number[24];
	char* quoted = rvn_wv2_js_string(script);
	size_t size = sizeof(RVN_WV2_RUN_SCRIPT) + strlen(quoted) + sizeof(view->token) + sizeof(number);
	char* out = (char*)rvn_alloc(size);
	const char* at;
	size_t o = 0;
	rvn_wv2_u64(number, request);
	for (at = RVN_WV2_RUN_SCRIPT; *at; at++) {
		const char* hole = NULL;
		if (at[0] == '%' && (at[1] == '1' || at[1] == '2' || at[1] == '3')) {
			hole = at[1] == '1' ? view->token : at[1] == '2' ? number : quoted;
			at++;
		}
		if (hole) {
			size_t length = strlen(hole);
			memcpy(out + o, hole, length);
			o += length;
		} else {
			out[o++] = *at;
		}
	}
	out[o] = '\0';
	rvn_free(quoted);
	return out;
}

static void rvn_wv2_forget_waiting(rvn_pweb_view_t* view, uint64_t request) {
	uint32_t i;
	for (i = 0; i < view->waiting_count; i++) {
		if (view->waiting[i] != request) continue;
		view->waiting[i] = view->waiting[--view->waiting_count];
		return;
	}
}

/* The page changed: the promises waited for will not end. */
static void rvn_wv2_fail_waiting(rvn_pweb_view_t* view) {
	uint32_t i;
	for (i = 0; i < view->waiting_count; i++) {
		rvn_wv2_send(view, RVN_PWEB_RESULT, NULL, "the page changed before the script ended", view->waiting[i]);
	}
	view->waiting_count = 0;
}

/* A message of the wrapper (the end of a promise)? Then it is told as the result of its script. */
static int rvn_wv2_result_message(rvn_pweb_view_t* view, const char* text) {
	size_t prefix = sizeof(RVN_WV2_RESULT_PREFIX) - 1;
	const char* at;
	char* end;
	uint64_t request;
	if (strncmp(text, RVN_WV2_RESULT_PREFIX, prefix) != 0) return 0;
	at = text + prefix;
	/* (not this view's: dropped) */
	if (strncmp(at, view->token, 16) != 0 || at[16] != ':') return 1;
	at += 17;
	request = (uint64_t)strtoull(at, &end, 10);
	if (end == at || end[0] != ':' || (end[1] != 'v' && end[1] != 'e')) return 1;
	rvn_wv2_forget_waiting(view, request);
	if (end[1] == 'v') rvn_wv2_send(view, RVN_PWEB_RESULT, end[2] ? end + 2 : "null", NULL, request);
	else rvn_wv2_send(view, RVN_PWEB_RESULT, NULL, end[2] ? end + 2 : "the script failed", request);
	return 1;
}

/* One ExecuteScript: the handler of its end (ICoreWebView2ExecuteScriptCompletedHandler). */
typedef struct rvn_wv2_call_t {
	rvn_wv2_string_done_t iface;
	LONG refs;
	rvn_pweb_view_t* view;
	uint64_t request;
	/** The script runs as it is (no wrapper): its value is what ExecuteScript gives. */
	int raw;
	/** The script, to run it again as it is when eval is forbidden. */
	char* script;
} rvn_wv2_call_t;

static void rvn_wv2_execute(rvn_pweb_view_t* view, const char* script, uint64_t request, int raw);

static HRESULT STDMETHODCALLTYPE rvn_wv2_call_query(rvn_wv2_string_done_t* self, REFIID riid, void** object) {
	if (!object) return E_POINTER;
	if (rvn_wv2_same_iid(riid, &rvn_wv2_iid_unknown) || rvn_wv2_same_iid(riid, &rvn_wv2_iid_execute_script_done)) {
		*object = self;
		self->lpVtbl->AddRef(self);
		return S_OK;
	}
	*object = NULL;
	return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE rvn_wv2_call_add_ref(rvn_wv2_string_done_t* self) {
	return (ULONG)InterlockedIncrement(&((rvn_wv2_call_t*)self)->refs);
}

static ULONG STDMETHODCALLTYPE rvn_wv2_call_release(rvn_wv2_string_done_t* self) {
	rvn_wv2_call_t* call = (rvn_wv2_call_t*)self;
	LONG refs = InterlockedDecrement(&call->refs);
	if (refs == 0) {
		rvn_wv2_view_release(call->view);
		free(call->script);
		free(call);
	}
	return (ULONG)refs;
}

static HRESULT STDMETHODCALLTYPE rvn_wv2_call_invoke(rvn_wv2_string_done_t* self, HRESULT error, LPCWSTR result) {
	rvn_wv2_call_t* call = (rvn_wv2_call_t*)self;
	rvn_pweb_view_t* view = call->view;
	char* text;
	if (g_wv2.log > 0) {
		char* told = rvn_wv2_narrow(result);
		rvn_wv2_log("view %p: ExecuteScript %lu done: 0x%08lX %.160s", (void*)view, (unsigned long)call->request, (unsigned long)error, told);
		rvn_free(told);
	}
	if (view->closed) return S_OK;
	if (FAILED(error)) {
		char message[96];
		snprintf(message, sizeof(message), "the script could not run (0x%08lX)", (unsigned long)error);
		rvn_wv2_log("ExecuteScript: 0x%08lX", (unsigned long)error);
		rvn_wv2_send(view, RVN_PWEB_RESULT, NULL, message, call->request);
		return S_OK;
	}
	if (call->raw) {
		text = rvn_wv2_narrow(result);
		rvn_wv2_send(view, RVN_PWEB_RESULT, text[0] ? text : "null", NULL, call->request);
		rvn_free(text);
		return S_OK;
	}
	text = rvn_wv2_json_string(result);
	if (!text || !text[0]) {
		/* (the page went away while the wrapper ran) */
		rvn_wv2_send(view, RVN_PWEB_RESULT, NULL, "the script did not run", call->request);
	} else if (text[0] == 'v') {
		rvn_wv2_send(view, RVN_PWEB_RESULT, text[1] ? text + 1 : "null", NULL, call->request);
	} else if (text[0] == 'e') {
		rvn_wv2_send(view, RVN_PWEB_RESULT, NULL, text[1] ? text + 1 : "the script failed", call->request);
	} else if (text[0] == 'a') {
		/* the end of the promise comes as a message (perhaps already came: the core takes a result once) */
		RVN_ARRAY_RESERVE(view->waiting, view->waiting_cap, view->waiting_count + 1);
		view->waiting[view->waiting_count++] = call->request;
	} else if (text[0] == 'n') {
		rvn_wv2_log("eval is forbidden by the page: the script runs as it is");
		if (call->script) rvn_wv2_execute(view, call->script, call->request, 1);
	} else {
		rvn_wv2_send(view, RVN_PWEB_RESULT, NULL, "the script did not run", call->request);
	}
	rvn_free(text);
	return S_OK;
}

static const rvn_wv2_string_done_vtbl_t g_rvn_wv2_call_vtbl = {
	rvn_wv2_call_query, rvn_wv2_call_add_ref, rvn_wv2_call_release, rvn_wv2_call_invoke
};

static void rvn_wv2_execute(rvn_pweb_view_t* view, const char* script, uint64_t request, int raw) {
	rvn_wv2_call_t* call;
	char* source;
	WCHAR* wide;
	HRESULT result;
	if (!view->core) return;
	call = (rvn_wv2_call_t*)calloc(1, sizeof(rvn_wv2_call_t));
	if (!call) {
		rvn_wv2_send(view, RVN_PWEB_RESULT, NULL, "out of memory", request);
		return;
	}
	call->iface.lpVtbl = &g_rvn_wv2_call_vtbl;
	call->refs = 1;
	call->view = view;
	rvn_wv2_view_add_ref(view);
	call->request = request;
	call->raw = raw;
	call->script = raw ? NULL : rvn_wv2_crt_strdup(script);
	source = raw ? rvn_strdup(script) : rvn_wv2_wrap(view, script, request);
	wide = rvn_wv2_widen(source);
	result = view->core->lpVtbl->ExecuteScript(view->core, wide, &call->iface);
	rvn_wv2_log("view %p: ExecuteScript %lu%s: 0x%08lX", (void*)view, (unsigned long)request, raw ? " (as it is)" : "", (unsigned long)result);
	if (FAILED(result)) {
		char message[96];
		snprintf(message, sizeof(message), "the script could not run (0x%08lX)", (unsigned long)result);
		rvn_wv2_log("ExecuteScript: 0x%08lX", (unsigned long)result);
		rvn_wv2_send(view, RVN_PWEB_RESULT, NULL, message, request);
	}
	rvn_free(wide);
	rvn_free(source);
	call->iface.lpVtbl->Release(&call->iface);
}

/* ---------------------------------------------------------------------------
 *  Loading
 * ------------------------------------------------------------------------- */

static void rvn_wv2_forget_html(rvn_pweb_view_t* view) {
	rvn_free(view->html);
	rvn_free(view->html_uri);
	rvn_free(view->html_seen);
	view->html = NULL;
	view->html_uri = NULL;
	view->html_seen = NULL;
	view->html_size = 0;
	view->html_expect = 0;
}

/* The page given as text is forgotten, and so is the filter that served it. */
static void rvn_wv2_drop_html(rvn_pweb_view_t* view) {
	if (view->html_filter && view->html_uri && view->core) {
		view->core->lpVtbl->RemoveWebResourceRequestedFilter(view->core, view->html_uri, RVN_WV2_RESOURCE_DOCUMENT);
	}
	view->html_filter = 0;
	rvn_wv2_forget_html(view);
}

static void rvn_wv2_navigate_failed(rvn_pweb_view_t* view, HRESULT result, const char* what) {
	char message[160];
	rvn_wv2_log("%s: 0x%08lX", what, (unsigned long)result);
	if (result == E_INVALIDARG) snprintf(message, sizeof(message), "not a valid address");
	else snprintf(message, sizeof(message), "the page could not be loaded (0x%08lX)", (unsigned long)result);
	rvn_wv2_send(view, RVN_PWEB_FAILED, NULL, message, 0);
}

static void rvn_wv2_navigate_now(rvn_pweb_view_t* view, const char* url) {
	WCHAR* wide = rvn_wv2_widen(url);
	HRESULT result;
	/* the address of a page given as text is the page from the network again when asked for by itself */
	if (view->html_uri && _wcsicmp(wide, view->html_uri) == 0) rvn_wv2_drop_html(view);
	rvn_wv2_log("Navigate %s", url);
	result = view->core->lpVtbl->Navigate(view->core, wide);
	if (FAILED(result)) rvn_wv2_navigate_failed(view, result, "Navigate");
	rvn_free(wide);
}

static void rvn_wv2_html_now(rvn_pweb_view_t* view, const char* html, const char* base_url) {
	WCHAR* wide;
	HRESULT result;
	if (base_url && (rvn_wv2_starts_with(base_url, "http://") || rvn_wv2_starts_with(base_url, "https://")) &&
		g_wv2.memory_stream && g_wv2.environment) {
		/*
		 * Only the base address goes through the handler (a filter "*" would
		 * also take the other documents, files among them, through it).
		 */
		rvn_wv2_drop_html(view);
		view->html_uri = rvn_wv2_widen(base_url);
		result = view->core->lpVtbl->AddWebResourceRequestedFilter(view->core, view->html_uri, RVN_WV2_RESOURCE_DOCUMENT);
		rvn_wv2_log("AddWebResourceRequestedFilter %s: 0x%08lX", base_url, (unsigned long)result);
		view->html_filter = SUCCEEDED(result);
		if (view->html_filter) {
			view->html = rvn_strdup(html ? html : "");
			view->html_size = strlen(view->html);
			view->html_expect = 1;
			rvn_wv2_log("Navigate %s (the page given as text, %u bytes)", base_url, (unsigned)view->html_size);
			result = view->core->lpVtbl->Navigate(view->core, view->html_uri);
			if (FAILED(result)) {
				view->html_expect = 0;
				rvn_wv2_navigate_failed(view, result, "Navigate");
			}
			return;
		}
		rvn_wv2_forget_html(view);
	}
	/* (a base that is not http(s) is not known to NavigateToString: relative addresses do not resolve) */
	wide = rvn_wv2_widen(html);
	rvn_wv2_log("NavigateToString (%u bytes)", (unsigned)strlen(html ? html : ""));
	result = view->core->lpVtbl->NavigateToString(view->core, wide);
	if (FAILED(result)) {
		char message[200];
		rvn_wv2_log("NavigateToString: 0x%08lX", (unsigned long)result);
		snprintf(message, sizeof(message),
			"the page could not be shown (0x%08lX): without an http(s) base address, a page given as text is 2 MB at most", (unsigned long)result);
		rvn_wv2_send(view, RVN_PWEB_FAILED, NULL, message, 0);
	}
	rvn_free(wide);
}

/* The page given as text, when WebView2 asks for its address. */
static void rvn_wv2_serve(rvn_pweb_view_t* view, rvn_wv2_resource_args_t* args) {
	rvn_wv2_resource_request_t* request = NULL;
	LPWSTR uri = NULL;
	HRESULT result;
	if (FAILED(args->lpVtbl->get_Request(args, &request)) || !request) return;
	if (SUCCEEDED(request->lpVtbl->get_Uri(request, &uri)) && uri) {
		char* text = rvn_wv2_narrow(uri);
		rvn_wv2_log("view %p: WebResourceRequested %s", (void*)view, text);
		rvn_free(text);
	}
	if (view->html && g_wv2.environment && g_wv2.memory_stream && uri &&
		((view->html_seen && wcscmp(uri, view->html_seen) == 0) || (view->html_uri && wcscmp(uri, view->html_uri) == 0))) {
		IStream* stream = g_wv2.memory_stream((const BYTE*)view->html, (UINT)view->html_size);
		rvn_wv2_resource_response_t* response = NULL;
		result = E_OUTOFMEMORY;
		if (stream) {
			result = g_wv2.environment->lpVtbl->CreateWebResourceResponse(g_wv2.environment, stream, 200, L"OK",
				L"Content-Type: text/html; charset=utf-8", &response);
			if (SUCCEEDED(result) && response) {
				result = args->lpVtbl->put_Response(args, response);
				response->lpVtbl->Release(response);
			}
			stream->lpVtbl->Release(stream);
		}
		rvn_wv2_log("the page given as text is served: 0x%08lX", (unsigned long)result);
	}
	if (uri) CoTaskMemFree(uri);
	request->lpVtbl->Release(request);
}

/* ---------------------------------------------------------------------------
 *  The events of WebView2
 * ------------------------------------------------------------------------- */

static const char* rvn_wv2_error_text(int status) {
	switch (status) {
		case RVN_WV2_ERROR_HOST_NAME_NOT_RESOLVED: return "the server could not be found";
		case RVN_WV2_ERROR_SERVER_UNREACHABLE:
		case RVN_WV2_ERROR_CANNOT_CONNECT: return "the server could not be reached";
		case RVN_WV2_ERROR_TIMEOUT: return "the server took too long to answer";
		case RVN_WV2_ERROR_CONNECTION_ABORTED:
		case RVN_WV2_ERROR_CONNECTION_RESET:
		case RVN_WV2_ERROR_DISCONNECTED: return "the connection was lost";
		case 1: case 2: case 3: case 4: case 5: return "the certificate of the server is not valid";
		default: return NULL;
	}
}

static void rvn_wv2_on(rvn_pweb_view_t* view, int kind, void* args) {
	rvn_wv2_core_t* core = view->core;
	switch (kind) {
		case RVN_WV2_ON_STARTING: {
			rvn_wv2_navigation_starting_args_t* a = (rvn_wv2_navigation_starting_args_t*)args;
			if (g_wv2.log > 0) {
				LPWSTR uri = NULL;
				if (SUCCEEDED(a->lpVtbl->get_Uri(a, &uri)) && uri) {
					char* text = rvn_wv2_take(uri);
					rvn_wv2_log("  to %s", text);
					rvn_free(text);
				}
			}
			if (view->html_expect) {
				LPWSTR uri = NULL;
				view->html_expect = 0;
				if (SUCCEEDED(a->lpVtbl->get_Uri(a, &uri)) && uri) {
					rvn_free(view->html_seen);
					view->html_seen = rvn_wv2_wcsdup(uri);
					CoTaskMemFree(uri);
				}
			}
			rvn_wv2_fail_waiting(view);
			rvn_wv2_send(view, RVN_PWEB_LOADING, NULL, NULL, 0);
			break;
		}
		case RVN_WV2_ON_COMPLETED: {
			rvn_wv2_navigation_completed_args_t* a = (rvn_wv2_navigation_completed_args_t*)args;
			BOOL success = FALSE;
			int status = RVN_WV2_ERROR_UNKNOWN;
			a->lpVtbl->get_IsSuccess(a, &success);
			rvn_wv2_log("  success %d", (int)success);
			if (success) {
				rvn_wv2_send(view, RVN_PWEB_LOADED, NULL, NULL, 0);
				break;
			}
			a->lpVtbl->get_WebErrorStatus(a, &status);
			rvn_wv2_log("navigation failed: status %d", status);
			/* a load stopped by another one (or by stop) is not a failure */
			if (status == RVN_WV2_ERROR_OPERATION_CANCELED) break;
			{
				char message[96];
				const char* text = rvn_wv2_error_text(status);
				if (!text) {
					snprintf(message, sizeof(message), "the page could not be loaded (status %d)", status);
					text = message;
				}
				rvn_wv2_send(view, RVN_PWEB_FAILED, NULL, text, 0);
			}
			break;
		}
		case RVN_WV2_ON_SOURCE: {
			LPWSTR uri = NULL;
			if (SUCCEEDED(core->lpVtbl->get_Source(core, &uri))) {
				char* text = rvn_wv2_take(uri);
				rvn_wv2_send(view, RVN_PWEB_URL, text, NULL, 0);
				rvn_free(text);
			}
			break;
		}
		case RVN_WV2_ON_TITLE: {
			LPWSTR title = NULL;
			if (SUCCEEDED(core->lpVtbl->get_DocumentTitle(core, &title))) {
				char* text = rvn_wv2_take(title);
				rvn_wv2_send(view, RVN_PWEB_TITLE, text, NULL, 0);
				rvn_free(text);
			}
			break;
		}
		case RVN_WV2_ON_MESSAGE: {
			rvn_wv2_message_args_t* a = (rvn_wv2_message_args_t*)args;
			LPWSTR value = NULL;
			char* text;
			if (SUCCEEDED(a->lpVtbl->TryGetWebMessageAsString(a, &value)) && value) {
				text = rvn_wv2_take(value);
				if (!rvn_wv2_result_message(view, text)) rvn_wv2_send(view, RVN_PWEB_MESSAGE, text, NULL, 0);
				rvn_free(text);
				break;
			}
			if (value) CoTaskMemFree(value);
			value = NULL;
			/* (chrome.webview.postMessage of something else than a string: its JSON) */
			if (SUCCEEDED(a->lpVtbl->get_WebMessageAsJson(a, &value)) && value) {
				text = rvn_wv2_take(value);
				rvn_wv2_send(view, RVN_PWEB_MESSAGE, text, NULL, 0);
				rvn_free(text);
			}
			break;
		}
		case RVN_WV2_ON_NEW_WINDOW: {
			/* a link to a new window (target="_blank", window.open) opens in this view: there is no other */
			rvn_wv2_new_window_args_t* a = (rvn_wv2_new_window_args_t*)args;
			LPWSTR uri = NULL;
			a->lpVtbl->put_Handled(a, TRUE);
			if (SUCCEEDED(a->lpVtbl->get_Uri(a, &uri)) && uri) {
				if (uri[0] && wcscmp(uri, L"about:blank") != 0) core->lpVtbl->Navigate(core, uri);
				CoTaskMemFree(uri);
			}
			break;
		}
		case RVN_WV2_ON_PROCESS: {
			rvn_wv2_process_failed_args_t* a = (rvn_wv2_process_failed_args_t*)args;
			int failed = -1;
			a->lpVtbl->get_ProcessFailedKind(a, &failed);
			rvn_wv2_log("a process of WebView2 stopped (kind %d)", failed);
			/* (the others, the GPU or a utility, are started again by WebView2: the page stays) */
			if (failed == RVN_WV2_BROWSER_PROCESS_EXITED || failed == RVN_WV2_RENDER_PROCESS_EXITED ||
				failed == RVN_WV2_RENDER_PROCESS_UNRESPONSIVE) {
				rvn_wv2_send(view, RVN_PWEB_FAILED, NULL, "the process that showed the page stopped", 0);
			}
			break;
		}
		case RVN_WV2_ON_RESOURCE:
			rvn_wv2_serve(view, (rvn_wv2_resource_args_t*)args);
			break;
		default:
			break;
	}
}

static const IID* rvn_wv2_event_iid(int kind) {
	switch (kind) {
		case RVN_WV2_ON_STARTING: return &rvn_wv2_iid_navigation_starting;
		case RVN_WV2_ON_COMPLETED: return &rvn_wv2_iid_navigation_completed;
		case RVN_WV2_ON_SOURCE: return &rvn_wv2_iid_source_changed;
		case RVN_WV2_ON_TITLE: return &rvn_wv2_iid_title_changed;
		case RVN_WV2_ON_MESSAGE: return &rvn_wv2_iid_message_received;
		case RVN_WV2_ON_NEW_WINDOW: return &rvn_wv2_iid_new_window;
		case RVN_WV2_ON_PROCESS: return &rvn_wv2_iid_process_failed;
		default: return &rvn_wv2_iid_resource_requested;
	}
}

static HRESULT STDMETHODCALLTYPE rvn_wv2_event_query(rvn_wv2_event_handler_t* self, REFIID riid, void** object) {
	rvn_wv2_event_part_t* part = (rvn_wv2_event_part_t*)self;
	if (!object) return E_POINTER;
	if (rvn_wv2_same_iid(riid, &rvn_wv2_iid_unknown) || rvn_wv2_same_iid(riid, rvn_wv2_event_iid(part->kind))) {
		*object = self;
		rvn_wv2_view_add_ref(part->view);
		return S_OK;
	}
	*object = NULL;
	return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE rvn_wv2_event_add_ref(rvn_wv2_event_handler_t* self) {
	rvn_wv2_view_add_ref(((rvn_wv2_event_part_t*)self)->view);
	return 2;
}

static ULONG STDMETHODCALLTYPE rvn_wv2_event_release(rvn_wv2_event_handler_t* self) {
	rvn_wv2_view_release(((rvn_wv2_event_part_t*)self)->view);
	return 1;
}

static HRESULT STDMETHODCALLTYPE rvn_wv2_event_invoke(rvn_wv2_event_handler_t* self, rvn_wv2_core_t* sender, void* args) {
	rvn_wv2_event_part_t* part = (rvn_wv2_event_part_t*)self;
	static const char* names[RVN_WV2_ON_COUNT] = {
		"NavigationStarting", "NavigationCompleted", "SourceChanged", "DocumentTitleChanged",
		"WebMessageReceived", "NewWindowRequested", "ProcessFailed", "WebResourceRequested"
	};
	(void)sender;
	rvn_wv2_log("view %p: WebView2 event %s%s", (void*)part->view, names[part->kind], part->view->closed ? " (view closed)" : "");
	/* (DocumentTitleChanged has no arguments: its args are always NULL) */
	if (part->view->closed || !part->view->core || (!args && part->kind != RVN_WV2_ON_TITLE)) return S_OK;
	rvn_wv2_on(part->view, part->kind, args);
	return S_OK;
}

static const rvn_wv2_event_handler_vtbl_t g_rvn_wv2_event_vtbl = {
	rvn_wv2_event_query, rvn_wv2_event_add_ref, rvn_wv2_event_release, rvn_wv2_event_invoke
};

static HRESULT rvn_wv2_listen(rvn_pweb_view_t* view, int kind) {
	rvn_wv2_core_t* core = view->core;
	rvn_wv2_event_handler_t* handler = &view->on[kind].iface;
	rvn_wv2_token_t* token = &view->tokens[kind];
	switch (kind) {
		case RVN_WV2_ON_STARTING: return core->lpVtbl->add_NavigationStarting(core, handler, token);
		case RVN_WV2_ON_COMPLETED: return core->lpVtbl->add_NavigationCompleted(core, handler, token);
		case RVN_WV2_ON_SOURCE: return core->lpVtbl->add_SourceChanged(core, handler, token);
		case RVN_WV2_ON_TITLE: return core->lpVtbl->add_DocumentTitleChanged(core, handler, token);
		case RVN_WV2_ON_MESSAGE: return core->lpVtbl->add_WebMessageReceived(core, handler, token);
		case RVN_WV2_ON_NEW_WINDOW: return core->lpVtbl->add_NewWindowRequested(core, handler, token);
		case RVN_WV2_ON_PROCESS: return core->lpVtbl->add_ProcessFailed(core, handler, token);
		case RVN_WV2_ON_RESOURCE: return core->lpVtbl->add_WebResourceRequested(core, handler, token);
		default: return E_INVALIDARG;
	}
}

static void rvn_wv2_unlisten(rvn_pweb_view_t* view, int kind) {
	rvn_wv2_core_t* core = view->core;
	rvn_wv2_token_t token = view->tokens[kind];
	switch (kind) {
		case RVN_WV2_ON_STARTING: core->lpVtbl->remove_NavigationStarting(core, token); break;
		case RVN_WV2_ON_COMPLETED: core->lpVtbl->remove_NavigationCompleted(core, token); break;
		case RVN_WV2_ON_SOURCE: core->lpVtbl->remove_SourceChanged(core, token); break;
		case RVN_WV2_ON_TITLE: core->lpVtbl->remove_DocumentTitleChanged(core, token); break;
		case RVN_WV2_ON_MESSAGE: core->lpVtbl->remove_WebMessageReceived(core, token); break;
		case RVN_WV2_ON_NEW_WINDOW: core->lpVtbl->remove_NewWindowRequested(core, token); break;
		case RVN_WV2_ON_PROCESS: core->lpVtbl->remove_ProcessFailed(core, token); break;
		case RVN_WV2_ON_RESOURCE: core->lpVtbl->remove_WebResourceRequested(core, token); break;
		default: break;
	}
}

/* ---------------------------------------------------------------------------
 *  Making a view: environment, controller, window.raven
 * ------------------------------------------------------------------------- */

static void rvn_wv2_free_pending(rvn_pweb_view_t* view) {
	uint32_t i;
	rvn_free(view->pending_url);
	rvn_free(view->pending_html);
	rvn_free(view->pending_base);
	view->pending_url = view->pending_html = view->pending_base = NULL;
	view->pending = RVN_WV2_PENDING_NONE;
	for (i = 0; i < view->script_count; i++) rvn_free(view->scripts[i].script);
	view->script_count = 0;
}

/* WebView2 could not be started for this view: it says so, and so does each script waiting. */
static void rvn_wv2_fail(rvn_pweb_view_t* view, const char* what, HRESULT result) {
	char message[200];
	uint32_t i;
	snprintf(message, sizeof(message), "%s (Microsoft Edge WebView2, 0x%08lX)", what, (unsigned long)result);
	rvn_wv2_log("%s", message);
	view->state = RVN_WV2_FAILED;
	rvn_wv2_send(view, RVN_PWEB_FAILED, NULL, message, 0);
	for (i = 0; i < view->script_count; i++) rvn_wv2_send(view, RVN_PWEB_RESULT, NULL, message, view->scripts[i].request);
	rvn_wv2_free_pending(view);
}

/* The view is ready: what was asked meanwhile is done now, in order. */
static void rvn_wv2_ready(rvn_pweb_view_t* view) {
	uint32_t i;
	view->state = RVN_WV2_READY;
	rvn_wv2_log("view %p ready", (void*)view);
	if (view->pending == RVN_WV2_PENDING_URL) rvn_wv2_navigate_now(view, view->pending_url);
	else if (view->pending == RVN_WV2_PENDING_HTML) rvn_wv2_html_now(view, view->pending_html, view->pending_base);
	for (i = 0; i < view->script_count && !view->closed; i++) rvn_wv2_execute(view, view->scripts[i].script, view->scripts[i].request, 0);
	rvn_wv2_free_pending(view);
}

/* window.raven was added to the pages (ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler). */
static HRESULT STDMETHODCALLTYPE rvn_wv2_bridge_query(rvn_wv2_string_done_t* self, REFIID riid, void** object) {
	if (!object) return E_POINTER;
	if (rvn_wv2_same_iid(riid, &rvn_wv2_iid_unknown) || rvn_wv2_same_iid(riid, &rvn_wv2_iid_add_script_done)) {
		*object = self;
		rvn_wv2_view_add_ref(((rvn_wv2_bridge_part_t*)self)->view);
		return S_OK;
	}
	*object = NULL;
	return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE rvn_wv2_bridge_add_ref(rvn_wv2_string_done_t* self) {
	rvn_wv2_view_add_ref(((rvn_wv2_bridge_part_t*)self)->view);
	return 2;
}

static ULONG STDMETHODCALLTYPE rvn_wv2_bridge_release(rvn_wv2_string_done_t* self) {
	rvn_wv2_view_release(((rvn_wv2_bridge_part_t*)self)->view);
	return 1;
}

static HRESULT STDMETHODCALLTYPE rvn_wv2_bridge_invoke(rvn_wv2_string_done_t* self, HRESULT error, LPCWSTR id) {
	rvn_pweb_view_t* view = ((rvn_wv2_bridge_part_t*)self)->view;
	(void)id;
	rvn_wv2_log("AddScriptToExecuteOnDocumentCreated done: 0x%08lX", (unsigned long)error);
	if (view->closed || view->state != RVN_WV2_PREPARING) return S_OK;
	/* (without it the pages cannot send messages, but they are shown) */
	rvn_wv2_ready(view);
	return S_OK;
}

static const rvn_wv2_string_done_vtbl_t g_rvn_wv2_bridge_vtbl = {
	rvn_wv2_bridge_query, rvn_wv2_bridge_add_ref, rvn_wv2_bridge_release, rvn_wv2_bridge_invoke
};

/* The controller of a view is made (ICoreWebView2CreateCoreWebView2ControllerCompletedHandler). */
static HRESULT STDMETHODCALLTYPE rvn_wv2_controller_query(rvn_wv2_controller_done_t* self, REFIID riid, void** object) {
	if (!object) return E_POINTER;
	if (rvn_wv2_same_iid(riid, &rvn_wv2_iid_unknown) || rvn_wv2_same_iid(riid, &rvn_wv2_iid_controller_done)) {
		*object = self;
		rvn_wv2_view_add_ref(((rvn_wv2_controller_part_t*)self)->view);
		return S_OK;
	}
	*object = NULL;
	return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE rvn_wv2_controller_add_ref(rvn_wv2_controller_done_t* self) {
	rvn_wv2_view_add_ref(((rvn_wv2_controller_part_t*)self)->view);
	return 2;
}

static ULONG STDMETHODCALLTYPE rvn_wv2_controller_release(rvn_wv2_controller_done_t* self) {
	rvn_wv2_view_release(((rvn_wv2_controller_part_t*)self)->view);
	return 1;
}

static HRESULT STDMETHODCALLTYPE rvn_wv2_controller_invoke(rvn_wv2_controller_done_t* self, HRESULT error, rvn_wv2_controller_t* controller) {
	rvn_pweb_view_t* view = ((rvn_wv2_controller_part_t*)self)->view;
	rvn_wv2_core_t* core = NULL;
	WCHAR* bridge;
	HRESULT result;
	int kind;
	rvn_wv2_log("controller of view %p created: 0x%08lX", (void*)view, (unsigned long)error);
	if (view->closed) {
		/* destroyed while it was being made */
		if (SUCCEEDED(error) && controller) controller->lpVtbl->Close(controller);
		return S_OK;
	}
	if (FAILED(error) || !controller) {
		rvn_wv2_fail(view, "the web view could not be created", FAILED(error) ? error : E_FAIL);
		return S_OK;
	}
	result = controller->lpVtbl->get_CoreWebView2(controller, &core);
	if (FAILED(result) || !core) {
		controller->lpVtbl->Close(controller);
		rvn_wv2_fail(view, "the web view could not be created", FAILED(result) ? result : E_FAIL);
		return S_OK;
	}
	controller->lpVtbl->AddRef(controller);
	view->controller = controller;
	view->core = core;
	for (kind = 0; kind < RVN_WV2_ON_COUNT; kind++) {
		result = rvn_wv2_listen(view, kind);
		view->listening[kind] = SUCCEEDED(result);
		if (FAILED(result)) rvn_wv2_log("listening to event %d: 0x%08lX", kind, (unsigned long)result);
	}
	controller->lpVtbl->put_Bounds(controller, view->bounds);
	controller->lpVtbl->put_IsVisible(controller, view->visible ? TRUE : FALSE);
	view->state = RVN_WV2_PREPARING;
	bridge = rvn_wv2_widen(RVN_WV2_BRIDGE_SCRIPT);
	result = core->lpVtbl->AddScriptToExecuteOnDocumentCreated(core, bridge, &view->bridge_part.iface);
	rvn_free(bridge);
	if (FAILED(result)) {
		rvn_wv2_log("AddScriptToExecuteOnDocumentCreated: 0x%08lX", (unsigned long)result);
		rvn_wv2_ready(view);
	}
	return S_OK;
}

static const rvn_wv2_controller_done_vtbl_t g_rvn_wv2_controller_vtbl = {
	rvn_wv2_controller_query, rvn_wv2_controller_add_ref, rvn_wv2_controller_release, rvn_wv2_controller_invoke
};

static void rvn_wv2_create_controller(rvn_pweb_view_t* view) {
	HRESULT result;
	view->state = RVN_WV2_CREATING;
	result = g_wv2.environment->lpVtbl->CreateCoreWebView2Controller(g_wv2.environment, view->container, &view->controller_part.iface);
	rvn_wv2_log("CreateCoreWebView2Controller: 0x%08lX", (unsigned long)result);
	if (FAILED(result) && view->state == RVN_WV2_CREATING) rvn_wv2_fail(view, "the web view could not be created", result);
}

/* The environment of the process is made (ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler). */
static HRESULT STDMETHODCALLTYPE rvn_wv2_environment_query(rvn_wv2_environment_done_t* self, REFIID riid, void** object) {
	if (!object) return E_POINTER;
	if (rvn_wv2_same_iid(riid, &rvn_wv2_iid_unknown) || rvn_wv2_same_iid(riid, &rvn_wv2_iid_environment_done)) {
		*object = self;
		return S_OK;
	}
	*object = NULL;
	return E_NOINTERFACE;
}

/* (a static object: it is never freed) */
static ULONG STDMETHODCALLTYPE rvn_wv2_environment_add_ref(rvn_wv2_environment_done_t* self) {
	(void)self;
	return 2;
}

static ULONG STDMETHODCALLTYPE rvn_wv2_environment_release(rvn_wv2_environment_done_t* self) {
	(void)self;
	return 1;
}

static void rvn_wv2_environment_failed(HRESULT result) {
	rvn_pweb_view_t* view;
	/* the next view tries again */
	g_wv2.env_state = RVN_WV2_ENV_NONE;
	for (view = g_wv2.views; view; view = view->next) {
		if (view->state == RVN_WV2_WAITING) rvn_wv2_fail(view, "the WebView2 Runtime could not be started", result);
	}
}

static HRESULT STDMETHODCALLTYPE rvn_wv2_environment_invoke(rvn_wv2_environment_done_t* self, HRESULT error, rvn_wv2_environment_t* environment) {
	rvn_pweb_view_t* view;
	(void)self;
	rvn_wv2_log("environment created: 0x%08lX", (unsigned long)error);
	if (g_wv2.env_state != RVN_WV2_ENV_CREATING) return S_OK;
	if (FAILED(error) || !environment) {
		rvn_wv2_environment_failed(FAILED(error) ? error : E_FAIL);
		return S_OK;
	}
	environment->lpVtbl->AddRef(environment);
	g_wv2.environment = environment;
	g_wv2.env_state = RVN_WV2_ENV_READY;
	{
		LPWSTR version = NULL;
		if (SUCCEEDED(environment->lpVtbl->get_BrowserVersionString(environment, &version)) && version) {
			char* text = rvn_wv2_take(version);
			rvn_wv2_log("browser version %s", text);
			rvn_free(text);
		}
	}
	for (view = g_wv2.views; view; view = view->next) {
		if (view->state == RVN_WV2_WAITING) rvn_wv2_create_controller(view);
	}
	return S_OK;
}

static const rvn_wv2_environment_done_vtbl_t g_rvn_wv2_environment_vtbl = {
	rvn_wv2_environment_query, rvn_wv2_environment_add_ref, rvn_wv2_environment_release, rvn_wv2_environment_invoke
};
static rvn_wv2_environment_done_t g_rvn_wv2_environment_done = { &g_rvn_wv2_environment_vtbl };

static void rvn_wv2_create_environment(void) {
	HRESULT result = E_FAIL;
	g_wv2.env_state = RVN_WV2_ENV_CREATING;
	/* (kept: the environment is made after the call returns) */
	if (!g_wv2.user_data) g_wv2.user_data = rvn_wv2_user_data_folder();
	if (g_wv2.user_data) {
		char* text = rvn_wv2_narrow(g_wv2.user_data);
		rvn_wv2_log("user data folder %s", text);
		rvn_free(text);
	}
	if (g_wv2.loader_create) {
		result = g_wv2.loader_create(NULL, g_wv2.user_data, NULL, &g_rvn_wv2_environment_done);
		rvn_wv2_log("CreateCoreWebView2EnvironmentWithOptions: 0x%08lX", (unsigned long)result);
	} else {
		rvn_wv2_create_internal_fn create = NULL;
		if (!g_wv2.client_module) {
			g_wv2.client_module = LoadLibraryW(g_wv2.client);
			if (!g_wv2.client_module) {
				result = HRESULT_FROM_WIN32(GetLastError());
				rvn_wv2_log("EmbeddedBrowserWebView.dll cannot be loaded: 0x%08lX", (unsigned long)result);
			}
		}
		if (g_wv2.client_module) {
			create = RVN_WV2_FUNCTION(rvn_wv2_create_internal_fn, GetProcAddress(g_wv2.client_module, "CreateWebViewEnvironmentWithOptionsInternal"));
			if (!create) {
				result = HRESULT_FROM_WIN32(GetLastError());
				rvn_wv2_log("CreateWebViewEnvironmentWithOptionsInternal not found: 0x%08lX", (unsigned long)result);
			}
		}
		if (create) {
			result = create(1, g_wv2.runtime_type, g_wv2.user_data, NULL, &g_rvn_wv2_environment_done);
			rvn_wv2_log("CreateWebViewEnvironmentWithOptionsInternal: 0x%08lX", (unsigned long)result);
		}
	}
	/* (the handler may have been called already, with the error) */
	if (FAILED(result) && g_wv2.env_state == RVN_WV2_ENV_CREATING) rvn_wv2_environment_failed(result);
}

/* ---------------------------------------------------------------------------
 *  The functions of the platform
 * ------------------------------------------------------------------------- */

rvn_pweb_view_t* rvn_wv2_create(void* user) {
	rvn_pweb_view_t* view;
	int kind;
	if (!rvn_wv2_supported() || !rvn_wv2_com() || !rvn_wv2_windows()) return NULL;
	if (!g_wv2.shlwapi) {
		g_wv2.shlwapi = LoadLibraryW(L"shlwapi.dll");
		if (g_wv2.shlwapi) g_wv2.memory_stream = RVN_WV2_FUNCTION(rvn_wv2_memory_stream_fn, GetProcAddress(g_wv2.shlwapi, "SHCreateMemStream"));
	}
	view = (rvn_pweb_view_t*)calloc(1, sizeof(rvn_pweb_view_t));
	if (!view) return NULL;
	view->refs = 1;
	view->user = user;
	view->container = CreateWindowExW(0, RVN_WV2_CLASS, L"", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		0, 0, 64, 64, g_wv2.hidden, NULL, GetModuleHandleW(NULL), NULL);
	if (!view->container) {
		rvn_wv2_log("cannot create the container window (error %lu)", (unsigned long)GetLastError());
		free(view);
		return NULL;
	}
	view->controller_part.iface.lpVtbl = &g_rvn_wv2_controller_vtbl;
	view->controller_part.view = view;
	view->bridge_part.iface.lpVtbl = &g_rvn_wv2_bridge_vtbl;
	view->bridge_part.view = view;
	for (kind = 0; kind < RVN_WV2_ON_COUNT; kind++) {
		view->on[kind].iface.lpVtbl = &g_rvn_wv2_event_vtbl;
		view->on[kind].view = view;
		view->on[kind].kind = kind;
	}
	{
		/* (not a secret the page cannot learn; it keeps the messages of the page from being taken for results) */
		static const char hex[] = "0123456789abcdef";
		LARGE_INTEGER counter;
		uint64_t seed;
		int i;
		QueryPerformanceCounter(&counter);
		seed = (uint64_t)counter.QuadPart ^ ((uint64_t)(uintptr_t)view << 13) ^ ((uint64_t)GetCurrentProcessId() << 40);
		seed += 0x9E3779B97F4A7C15ull;
		seed = (seed ^ (seed >> 30)) * 0xBF58476D1CE4E5B9ull;
		seed = (seed ^ (seed >> 27)) * 0x94D049BB133111EBull;
		seed ^= seed >> 31;
		for (i = 0; i < 16; i++) view->token[i] = hex[(seed >> (i * 4)) & 15];
		view->token[16] = '\0';
	}
	view->state = RVN_WV2_WAITING;
	view->next = g_wv2.views;
	g_wv2.views = view;
	rvn_wv2_enter();
	if (g_wv2.env_state == RVN_WV2_ENV_READY) rvn_wv2_create_controller(view);
	else if (g_wv2.env_state == RVN_WV2_ENV_NONE) rvn_wv2_create_environment();
	rvn_wv2_leave();
	return view;
}

void rvn_wv2_destroy(rvn_pweb_view_t* view) {
	rvn_pweb_view_t** link;
	int kind;
	if (!view || view->closed) return;
	rvn_wv2_enter();
	view->closed = 1;
	rvn_wv2_forget_events(view);
	for (link = &g_wv2.views; *link; link = &(*link)->next) {
		if (*link == view) {
			*link = view->next;
			break;
		}
	}
	if (view->core) {
		for (kind = 0; kind < RVN_WV2_ON_COUNT; kind++) {
			if (view->listening[kind]) rvn_wv2_unlisten(view, kind);
			view->listening[kind] = 0;
		}
		rvn_wv2_drop_html(view);
		view->core->lpVtbl->Release(view->core);
		view->core = NULL;
	}
	if (view->controller) {
		view->controller->lpVtbl->Close(view->controller);
		view->controller->lpVtbl->Release(view->controller);
		view->controller = NULL;
	}
	if (view->container) DestroyWindow(view->container);
	view->container = NULL;
	rvn_wv2_free_pending(view);
	rvn_free(view->scripts);
	view->scripts = NULL;
	rvn_wv2_forget_html(view);
	rvn_free(view->waiting);
	view->waiting = NULL;
	rvn_wv2_leave();
	/* (the handlers WebView2 still holds keep the structure until they are released) */
	rvn_wv2_view_release(view);
}

void rvn_wv2_set_parent_window(rvn_pweb_view_t* view, void* parent) {
	HWND window = (HWND)parent;
	if (!view || view->closed) return;
	rvn_wv2_enter();
	ShowWindow(view->container, SW_HIDE);
	view->visible = 0;
	if (view->controller) view->controller->lpVtbl->put_IsVisible(view->controller, FALSE);
	if (!window) window = g_wv2.hidden;
	if (GetParent(view->container) != window) {
		SetParent(view->container, window);
		if (view->controller) view->controller->lpVtbl->NotifyParentWindowPositionChanged(view->controller);
	}
	rvn_wv2_leave();
}

void rvn_wv2_set_rect(rvn_pweb_view_t* view, const int rect[4], const int clip[4], float scale, int visible) {
	(void)scale;
	if (!view || view->closed) return;
	rvn_wv2_enter();
	if (!visible) {
		view->visible = 0;
		ShowWindow(view->container, SW_HIDE);
		if (view->controller) view->controller->lpVtbl->put_IsVisible(view->controller, FALSE);
		rvn_wv2_leave();
		return;
	}
	/* pixels of the client area of the window (Raven and WebView2 are both per-monitor DPI aware) */
	view->bounds.left = rect[0] - clip[0];
	view->bounds.top = rect[1] - clip[1];
	view->bounds.right = view->bounds.left + rect[2];
	view->bounds.bottom = view->bounds.top + rect[3];
	view->visible = 1;
	SetWindowPos(view->container, NULL, clip[0], clip[1], clip[2] > 1 ? clip[2] : 1, clip[3] > 1 ? clip[3] : 1,
		SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
	if (view->controller) {
		view->controller->lpVtbl->put_Bounds(view->controller, view->bounds);
		view->controller->lpVtbl->put_IsVisible(view->controller, TRUE);
	}
	rvn_wv2_leave();
}

void rvn_wv2_load_url(rvn_pweb_view_t* view, const char* url) {
	if (!view || view->closed) return;
	rvn_wv2_enter();
	if (view->state == RVN_WV2_READY) {
		rvn_wv2_navigate_now(view, url ? url : "about:blank");
	} else if (view->state == RVN_WV2_FAILED) {
		rvn_wv2_send(view, RVN_PWEB_FAILED, NULL, "Microsoft Edge WebView2 could not be started", 0);
	} else {
		rvn_free(view->pending_url);
		rvn_free(view->pending_html);
		rvn_free(view->pending_base);
		view->pending_html = view->pending_base = NULL;
		view->pending_url = rvn_strdup(url ? url : "about:blank");
		view->pending = RVN_WV2_PENDING_URL;
	}
	rvn_wv2_leave();
}

void rvn_wv2_load_html(rvn_pweb_view_t* view, const char* html, const char* base_url) {
	if (!view || view->closed) return;
	rvn_wv2_enter();
	if (view->state == RVN_WV2_READY) {
		rvn_wv2_html_now(view, html ? html : "", base_url);
	} else if (view->state == RVN_WV2_FAILED) {
		rvn_wv2_send(view, RVN_PWEB_FAILED, NULL, "Microsoft Edge WebView2 could not be started", 0);
	} else {
		rvn_free(view->pending_url);
		rvn_free(view->pending_html);
		rvn_free(view->pending_base);
		view->pending_url = NULL;
		view->pending_html = rvn_strdup(html ? html : "");
		view->pending_base = base_url ? rvn_strdup(base_url) : NULL;
		view->pending = RVN_WV2_PENDING_HTML;
	}
	rvn_wv2_leave();
}

void rvn_wv2_command(rvn_pweb_view_t* view, rvn_pweb_command_t command) {
	rvn_wv2_core_t* core;
	HRESULT result = S_OK;
	if (!view || view->closed || view->state != RVN_WV2_READY) return;
	core = view->core;
	rvn_wv2_enter();
	switch (command) {
		case RVN_PWEB_BACK: result = core->lpVtbl->GoBack(core); break;
		case RVN_PWEB_FORWARD: result = core->lpVtbl->GoForward(core); break;
		case RVN_PWEB_RELOAD: result = core->lpVtbl->Reload(core); break;
		case RVN_PWEB_STOP: result = core->lpVtbl->Stop(core); break;
	}
	if (FAILED(result)) rvn_wv2_log("command %d: 0x%08lX", (int)command, (unsigned long)result);
	rvn_wv2_leave();
}

int rvn_wv2_can(rvn_pweb_view_t* view, rvn_pweb_command_t command) {
	BOOL can = FALSE;
	if (!view || view->closed || view->state != RVN_WV2_READY) return 0;
	rvn_wv2_enter();
	if (command == RVN_PWEB_BACK) view->core->lpVtbl->get_CanGoBack(view->core, &can);
	else if (command == RVN_PWEB_FORWARD) view->core->lpVtbl->get_CanGoForward(view->core, &can);
	else can = TRUE;
	rvn_wv2_leave();
	return can ? 1 : 0;
}

void rvn_wv2_run_javascript(rvn_pweb_view_t* view, const char* script, uint64_t request) {
	if (!view || view->closed) return;
	rvn_wv2_enter();
	if (view->state == RVN_WV2_READY) {
		rvn_wv2_execute(view, script ? script : "", request, 0);
	} else if (view->state == RVN_WV2_FAILED) {
		rvn_wv2_send(view, RVN_PWEB_RESULT, NULL, "Microsoft Edge WebView2 could not be started", request);
	} else {
		RVN_ARRAY_RESERVE(view->scripts, view->script_cap, view->script_count + 1);
		view->scripts[view->script_count].script = rvn_strdup(script ? script : "");
		view->scripts[view->script_count].request = request;
		view->script_count++;
	}
	rvn_wv2_leave();
}

void rvn_wv2_focus(rvn_pweb_view_t* view, int focused) {
	if (!view || view->closed) return;
	rvn_wv2_enter();
	if (focused) {
		if (view->controller) view->controller->lpVtbl->MoveFocus(view->controller, RVN_WV2_FOCUS_PROGRAMMATIC);
	} else {
		HWND focus = GetFocus();
		HWND window = GetParent(view->container);
		int inside = focus && (focus == view->container || IsChild(view->container, focus));
		/* (NULL: the keyboard is in a window of another thread, the one of WebView2 when its input is not attached to this one) */
		if (!focus && window && GetForegroundWindow() == window) inside = 1;
		/* the keyboard goes back to the Raven window */
		if (inside && window && window != g_wv2.hidden) SetFocus(window);
	}
	rvn_wv2_leave();
}

void* rvn_wv2_native(rvn_pweb_view_t* view) {
	return view && !view->closed ? (void*)view->core : NULL;
}

void rvn_wv2_shutdown(void) {
	rvn_pweb_view_t* view;
	while ((view = g_wv2.views) != NULL) rvn_wv2_destroy(view);
	while (g_wv2.head) {
		rvn_wv2_event_t* event = g_wv2.head;
		g_wv2.head = event->next;
		rvn_wv2_free_event(event);
	}
	g_wv2.tail = NULL;
	if (g_wv2.environment) g_wv2.environment->lpVtbl->Release(g_wv2.environment);
	g_wv2.environment = NULL;
	g_wv2.env_state = RVN_WV2_ENV_NONE;
	if (g_wv2.hidden) DestroyWindow(g_wv2.hidden);
	g_wv2.hidden = NULL;
	if (g_wv2.classes) {
		UnregisterClassW(RVN_WV2_CLASS, GetModuleHandleW(NULL));
		UnregisterClassW(RVN_WV2_HIDDEN_CLASS, GetModuleHandleW(NULL));
		g_wv2.classes = 0;
	}
	rvn_free(g_wv2.user_data);
	g_wv2.user_data = NULL;
	g_wv2.drain_posted = g_wv2.drain_missed = 0;
	/* (the DLLs stay loaded: WebView2 may still finish on its threads; COM stays as other parts of Raven leave it) */
}

#endif /* _WIN32 */
