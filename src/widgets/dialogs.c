/**
 * Raven - widgets/dialogs.c
 * Dialogs shown over a window, message boxes and file dialogs.
 *
 * A file dialog is the one of the operating system where there is one:
 *   Win32   GetOpenFileNameW / GetSaveFileNameW, IFileOpenDialog for folders
 *   Linux   zenity or kdialog, started as a child process
 * and otherwise (or with R_FILE_DIALOG_RAVEN) a FileBrowser in a dialog of
 * the window.
 */

#include "common.h"
#include "../core/fs.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef COBJMACROS
#define COBJMACROS
#endif
#include <windows.h>
#include <commdlg.h>
#include <objbase.h>
#include <shobjidl.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

/** What the last file dialog returned, and where it was. */
static struct {
	char** paths;
	int count;
	int cap;
	char* directory;
} g_dialogs;

static void rvn_dialogs_clear_results(void) {
	int i;
	for (i = 0; i < g_dialogs.count; i++) rvn_free(g_dialogs.paths[i]);
	g_dialogs.count = 0;
}

static void rvn_dialogs_add_result(const char* path) {
	if (!path || !path[0]) return;
	if (g_dialogs.count + 1 > g_dialogs.cap) {
		g_dialogs.cap = g_dialogs.cap ? g_dialogs.cap * 2 : 8;
		g_dialogs.paths = (char**)rvn_realloc(g_dialogs.paths, sizeof(char*) * (size_t)g_dialogs.cap);
	}
	g_dialogs.paths[g_dialogs.count++] = rvn_strdup(path);
}

void rvn_dialogs_shutdown(void) {
	rvn_dialogs_clear_results();
	rvn_free(g_dialogs.paths);
	rvn_free(g_dialogs.directory);
	memset(&g_dialogs, 0, sizeof(g_dialogs));
}

/* ---------------------------------------------------------------------------
 *  Dialogs
 * ------------------------------------------------------------------------- */

typedef struct rvn_dialog_wait_t {
	int done;
	int result;
} rvn_dialog_wait_t;

static void rvn_dialog_wait_closed(Rid content, int result, void* user_data) {
	rvn_dialog_wait_t* wait = (rvn_dialog_wait_t*)user_data;
	RVN_UNUSED(content);
	wait->done = R_TRUE;
	wait->result = result;
}

/* One frame of the application, from inside a function that waits. */
static void rvn_dialog_frame(double timeout) {
	Rwait_events(timeout);
	rvn_update();
}

/* Shows a dialog and runs the application until it closes. */
static int rvn_dialog_wait(rvn_window_t* window, rvn_widget_t* content) {
	rvn_dialog_wait_t wait;
	Rid window_id = window->id, content_id = content->id;
	/* (called from a timer or a task, the frames must still run) */
	int in_update = g_rvn.in_update;

	wait.done = R_FALSE;
	wait.result = 0;
	rvn_window_open_dialog(window, content, rvn_dialog_wait_closed, &wait);
	if (rvn_window_popup_index(window, content) < 0) {
		/* it could not be shown */
		if (rvn_widget_get(content_id)) rvn_widget_destroy(content);
		return 0;
	}
	g_rvn.in_update = R_FALSE;
	while (!wait.done && Ris_running() && rvn_window_get(window_id)) rvn_dialog_frame(-1.0);
	g_rvn.in_update = in_update;
	if (!wait.done) {
		/* the application is quitting: nothing may keep a pointer to this stack */
		content = rvn_widget_get(content_id);
		if (content) rvn_window_close_dialog(content, 0);
	}
	return wait.result;
}

/* A window to show a dialog in: the one given, else any, else a new one (*created). */
static rvn_window_t* rvn_dialog_window(Rid id, const char* title, int width, int height, int* created) {
	rvn_window_t* window = rvn_window_get(id);
	uint32_t i;
	*created = R_FALSE;
	for (i = 0; !window && i < g_rvn.windows.used; i++) {
		rvn_window_t* candidate = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (candidate && !candidate->dead && candidate->visible) window = candidate;
	}
	if (!window) {
		window = rvn_window_get(Rwindow_create(title, width, height, R_WINDOW_DEFAULT));
		*created = window != NULL;
	}
	return window;
}

void Rdialog_open(Rid window_id, Rid content_id, RDialogClosedCallback closed, void* user_data) {
	rvn_window_t* window;
	rvn_widget_t* content;
	RVN_API_GUARD_VOID();
	window = rvn_window_check(window_id, __func__);
	content = rvn_widget_check(content_id, 0, __func__);
	if (!window || !content) return;
	rvn_window_open_dialog(window, content, closed, user_data);
}

void Rdialog_close(Rid id, int result) {
	RVN_WIDGET_VOID(widget, id, 0);
	rvn_window_close_dialog(widget, result);
}

void Rpopup_open(Rid window_id, Rid content_id, float x, float y, RDialogClosedCallback closed, void* user_data) {
	rvn_window_t* window;
	rvn_widget_t* content;
	rvn_popup_t* popup;
	RVN_API_GUARD_VOID();
	window = rvn_window_check(window_id, __func__);
	content = rvn_widget_check(content_id, 0, __func__);
	if (!window || !content) return;
	if (content->parent) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rpopup_open: the content is already inside another widget");
		return;
	}
	rvn_window_open_popup(window, content, NULL, rvn_rect(x, y, 0.0f, 0.0f), RVN_POPUP_AT, 0.0f);
	popup = &window->popups[window->popup_count - 1];
	if (popup->root != content) return;
	popup->closed = closed;
	popup->closed_user = user_data;
}

void Rpopup_close(Rid id, int result) {
	rvn_window_t* window;
	uint32_t i;
	RVN_WIDGET_VOID(widget, id, 0);
	window = widget->window;
	if (!window) return;
	for (i = window->popup_count; i > 0; i--) {
		rvn_popup_t* popup = &window->popups[i - 1];
		if (popup->placement == RVN_POPUP_DIALOG || !rvn_widget_is_inside(widget, popup->root)) continue;
		popup->result = result;
		rvn_window_close_popup(window, popup->root);
		return;
	}
}

int Rpopup_is_open(Rid id) {
	RVN_WIDGET(widget, id, 0, R_FALSE);
	return (widget->flags & RVN_WF_POPUP) != 0;
}

int Rdialog_run(Rid window_id, Rid content_id) {
	rvn_window_t* window;
	rvn_widget_t* content;
	RVN_API_GUARD(0);
	window = rvn_window_check(window_id, __func__);
	content = rvn_widget_check(content_id, 0, __func__);
	if (!window || !content) return 0;
	return rvn_dialog_wait(window, content);
}

int Rwindow_has_dialog(Rid id) {
	rvn_window_t* window;
	RVN_API_GUARD(R_FALSE);
	window = rvn_window_check(id, __func__);
	return window && rvn_window_top_dialog(window) != NULL;
}

/* ---------------------------------------------------------------------------
 *  Message box
 * ------------------------------------------------------------------------- */

static void rvn_message_clicked(Rid button, void* user_data) {
	rvn_widget_t* widget = rvn_widget_get(button);
	if (widget) rvn_window_close_dialog(widget, (int)(intptr_t)user_data);
}

/* Enter presses the main button of the box. */
static int rvn_message_event(Rid widget_id, const REvent* event, void* user_data) {
	rvn_widget_t* widget = rvn_widget_get(widget_id);
	if (!widget || event->type != R_EVENT_KEY_DOWN) return R_FALSE;
	if (event->key != R_KEY_ENTER && event->key != R_KEY_KP_ENTER) return R_FALSE;
	rvn_window_close_dialog(widget, (int)(intptr_t)user_data);
	return R_TRUE;
}

static void rvn_message_button(rvn_widget_t* row, const char* text, RMessageResult result, int primary) {
	rvn_widget_t* button = rvn_widget_create(R_WIDGET_BUTTON);
	rvn_widget_set_attribute(button, "Text", text);
	if (primary) rvn_widget_set_attribute(button, "Style", "Primary");
	button->min_size.x = 84.0f;
	Rbutton_set_clicked_callback(button->id, rvn_message_clicked, (void*)(intptr_t)result);
	rvn_widget_add_child(row, button, -1);
}

RMessageResult Rmessage_box(Rid window_id, const char* title, const char* text, RMessageButtons buttons) {
	rvn_window_t* window;
	rvn_widget_t* box;
	rvn_widget_t* label;
	rvn_widget_t* row;
	RMessageResult main_result = R_MESSAGE_RESULT_OK;
	Rid id;
	int created, result;

	RVN_API_GUARD(R_MESSAGE_RESULT_CANCEL);
	if (buttons < R_MESSAGE_OK || buttons > R_MESSAGE_YES_NO_CANCEL) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rmessage_box: invalid buttons");
		return R_MESSAGE_RESULT_CANCEL;
	}
	window = rvn_dialog_window(window_id, title ? title : "", 460, 240, &created);
	if (!window) return R_MESSAGE_RESULT_CANCEL;

	box = rvn_widget_create(R_WIDGET_VBOX);
	rvn_widget_set_attribute(box, "Spacing", "12");
	box->fixed_size.x = 380.0f;
	if (title && title[0]) {
		label = rvn_widget_create(R_WIDGET_LABEL);
		rvn_widget_set_attribute(label, "Text", title);
		rvn_widget_set_attribute(label, "TextSize", "17");
		rvn_widget_add_child(box, label, -1);
	}
	label = rvn_widget_create(R_WIDGET_LABEL);
	rvn_widget_set_attribute(label, "Text", text ? text : "");
	rvn_widget_set_attribute(label, "Wrap", "true");
	label->opacity = (title && title[0]) ? 0.8f : 1.0f;
	rvn_widget_add_child(box, label, -1);

	row = rvn_widget_create(R_WIDGET_HBOX);
	rvn_widget_set_attribute(row, "Spacing", "8");
	row->slot.halign = R_HALIGN_RIGHT;
	row->slot.padding.t = 6.0f;
	switch (buttons) {
		case R_MESSAGE_OK:
			rvn_message_button(row, "OK", R_MESSAGE_RESULT_OK, R_TRUE);
			break;
		case R_MESSAGE_OK_CANCEL:
			rvn_message_button(row, "Cancel", R_MESSAGE_RESULT_CANCEL, R_FALSE);
			rvn_message_button(row, "OK", R_MESSAGE_RESULT_OK, R_TRUE);
			break;
		case R_MESSAGE_YES_NO:
			rvn_message_button(row, "No", R_MESSAGE_RESULT_NO, R_FALSE);
			rvn_message_button(row, "Yes", R_MESSAGE_RESULT_YES, R_TRUE);
			main_result = R_MESSAGE_RESULT_YES;
			break;
		default:
			rvn_message_button(row, "Cancel", R_MESSAGE_RESULT_CANCEL, R_FALSE);
			rvn_message_button(row, "No", R_MESSAGE_RESULT_NO, R_FALSE);
			rvn_message_button(row, "Yes", R_MESSAGE_RESULT_YES, R_TRUE);
			main_result = R_MESSAGE_RESULT_YES;
			break;
	}
	rvn_widget_add_child(box, row, -1);
	/* (a panel does not take the mouse, but keys reach the root of a dialog) */
	box->event_callback = rvn_message_event;
	box->event_user = (void*)(intptr_t)main_result;

	id = window->id;
	result = rvn_dialog_wait(window, box);
	/* (the user may have closed the window while it waited) */
	window = rvn_window_get(id);
	if (created && window && !window->dead) rvn_window_destroy(window);
	/* a question dismissed with Escape is answered "no" when there is no Cancel */
	if (result == 0 && buttons == R_MESSAGE_YES_NO) return R_MESSAGE_RESULT_NO;
	return (RMessageResult)result;
}

/* ---------------------------------------------------------------------------
 *  File dialog: the browser of Raven in a dialog
 * ------------------------------------------------------------------------- */

static const char* rvn_file_dialog_title(const RFileDialogDesc* desc, RFileBrowserMode mode) {
	if (desc->title && desc->title[0]) return desc->title;
	switch (mode) {
		case R_FILE_BROWSER_SAVE: return "Save as";
		case R_FILE_BROWSER_FOLDER: return "Select a folder";
		default: return (desc->flags & R_FILE_DIALOG_MULTIPLE) ? "Open files" : "Open";
	}
}

static void rvn_file_dialog_accepted(Rid browser, const char* path, void* user_data) {
	rvn_widget_t* widget = rvn_widget_get(browser);
	int count = Rfile_browser_get_selection_count(browser), i, added = 0;
	RVN_UNUSED(user_data);
	rvn_dialogs_clear_results();
	/* several files: the whole selection, when the chosen path is part of it */
	for (i = 0; i < count && count > 1; i++) {
		const char* selected = Rfile_browser_get_selection(browser, i);
		if (selected && !rvn_path_is_dir(selected)) {
			rvn_dialogs_add_result(selected);
			added++;
		}
	}
	if (added < 2) {
		rvn_dialogs_clear_results();
		rvn_dialogs_add_result(path);
	}
	if (widget) rvn_window_close_dialog(widget, 1);
}

static void rvn_file_dialog_cancelled(Rid browser, const char* path, void* user_data) {
	rvn_widget_t* widget = rvn_widget_get(browser);
	RVN_UNUSED(path);
	RVN_UNUSED(user_data);
	if (widget) rvn_window_close_dialog(widget, 0);
}

static void rvn_file_dialog_raven(const RFileDialogDesc* desc, RFileBrowserMode mode, const char* directory) {
	const char* title = rvn_file_dialog_title(desc, mode);
	rvn_window_t* window;
	rvn_widget_t* box;
	rvn_widget_t* label;
	rvn_widget_t* browser;
	rvn_vec2_t size;
	int created;

	window = rvn_dialog_window(desc->parent, title, 820, 560, &created);
	if (!window) return;
	size = rvn_window_size(window);

	box = rvn_widget_create(R_WIDGET_VBOX);
	rvn_widget_set_attribute(box, "Spacing", "12");
	label = rvn_widget_create(R_WIDGET_LABEL);
	rvn_widget_set_attribute(label, "Text", title);
	rvn_widget_set_attribute(label, "TextSize", "17");
	rvn_widget_add_child(box, label, -1);

	browser = rvn_widget_create(R_WIDGET_FILE_BROWSER);
	browser->slot.is_fill = R_TRUE;
	browser->slot.fill = 1.0f;
	rvn_widget_add_child(box, browser, -1);
	/* as large as a file dialog usually is, when the window has the room */
	box->fixed_size = rvn_vec2(rvn_minf(760.0f, rvn_maxf(size.x - 84.0f, 200.0f)), rvn_minf(500.0f, rvn_maxf(size.y - 84.0f, 160.0f)));

	Rfile_browser_set_mode(browser->id, mode);
	Rfile_browser_set_multiple(browser->id, mode == R_FILE_BROWSER_OPEN && (desc->flags & R_FILE_DIALOG_MULTIPLE));
	Rfile_browser_set_show_hidden(browser->id, (desc->flags & R_FILE_DIALOG_SHOW_HIDDEN) != 0);
	if (desc->filters && desc->filter_count > 0) Rfile_browser_set_filters(browser->id, desc->filters, desc->filter_count);
	if (desc->default_extension) Rfile_browser_set_default_extension(browser->id, desc->default_extension);
	if (directory) Rfile_browser_set_directory(browser->id, directory);
	if (desc->file_name) Rfile_browser_set_file_name(browser->id, desc->file_name);
	Rfile_browser_set_accepted_callback(browser->id, rvn_file_dialog_accepted, NULL);
	Rfile_browser_set_cancelled_callback(browser->id, rvn_file_dialog_cancelled, NULL);

	{
		Rid window_id = window->id, browser_id = browser->id;
		rvn_dialog_wait_t wait;
		int in_update = g_rvn.in_update;
		wait.done = R_FALSE;
		wait.result = 0;
		rvn_window_open_dialog(window, box, rvn_dialog_wait_closed, &wait);
		/* the list has the keyboard: arrows, letters and Enter work at once */
		if (rvn_widget_get(browser_id)) rvn_widget_focus(browser);
		g_rvn.in_update = R_FALSE;
		while (!wait.done && Ris_running() && rvn_window_get(window_id)) rvn_dialog_frame(-1.0);
		g_rvn.in_update = in_update;
		if (!wait.done && rvn_widget_get(browser_id)) rvn_window_close_dialog(browser, 0);
		if (wait.result == 0) rvn_dialogs_clear_results();
		window = rvn_window_get(window_id);
	}
	if (created && window) rvn_window_destroy(window);
}

/* ---------------------------------------------------------------------------
 *  File dialog: Win32
 * ------------------------------------------------------------------------- */

#if defined(_WIN32)

static WCHAR* rvn_dialog_wide(const char* text) {
	int length;
	WCHAR* wide;
	if (!text) return NULL;
	length = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
	if (length <= 0) return NULL;
	wide = (WCHAR*)rvn_alloc(sizeof(WCHAR) * (size_t)length);
	MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, length);
	return wide;
}

static char* rvn_dialog_narrow(const WCHAR* wide) {
	int length = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
	char* text;
	if (length <= 0) return NULL;
	text = (char*)rvn_alloc((size_t)length);
	WideCharToMultiByte(CP_UTF8, 0, wide, -1, text, length, NULL, NULL);
	return text;
}

static void rvn_dialog_add_wide(const WCHAR* wide) {
	char* path = rvn_dialog_narrow(wide);
	rvn_dialogs_add_result(path);
	rvn_free(path);
}

static HWND rvn_dialog_owner(Rid parent) {
	rvn_window_t* window = rvn_window_get(parent);
	RNativeWindow native;
	if (!window || !window->platform) return NULL;
	memset(&native, 0, sizeof(native));
	g_rvn.platform->window_get_native(window->platform, &native);
	return native.system == R_WINDOW_SYSTEM_WIN32 ? (HWND)native.window : NULL;
}

/* "Images\0*.png;*.jpg\0All files\0*\0\0" */
static WCHAR* rvn_dialog_filter_string(const RFileDialogDesc* desc) {
	WCHAR* result;
	size_t used = 0, cap = 2;
	int i;
	if (!desc->filters || desc->filter_count <= 0) return NULL;
	for (i = 0; i < desc->filter_count; i++) {
		cap += (desc->filters[i].name ? strlen(desc->filters[i].name) : 0) + 1;
		cap += (desc->filters[i].patterns ? strlen(desc->filters[i].patterns) : 1) * 2 + 4;
	}
	result = (WCHAR*)rvn_calloc(sizeof(WCHAR) * cap);
	for (i = 0; i < desc->filter_count; i++) {
		const char* patterns = (desc->filters[i].patterns && desc->filters[i].patterns[0]) ? desc->filters[i].patterns : "*";
		const char* name = (desc->filters[i].name && desc->filters[i].name[0]) ? desc->filters[i].name : patterns;
		used += (size_t)MultiByteToWideChar(CP_UTF8, 0, name, -1, result + used, (int)(cap - used));
		/* "*" alone would hide the files without extension from old dialogs */
		if (strcmp(patterns, "*") == 0) patterns = "*.*";
		used += (size_t)MultiByteToWideChar(CP_UTF8, 0, patterns, -1, result + used, (int)(cap - used));
	}
	result[used] = 0;
	return result;
}

static int rvn_file_dialog_win32_files(const RFileDialogDesc* desc, RFileBrowserMode mode, const char* directory) {
	enum { BUFFER = 32768 };
	OPENFILENAMEW ofn;
	WCHAR* buffer = (WCHAR*)rvn_calloc(sizeof(WCHAR) * BUFFER);
	WCHAR* filters = rvn_dialog_filter_string(desc);
	WCHAR* title = rvn_dialog_wide(desc->title);
	WCHAR* folder = rvn_dialog_wide(directory);
	WCHAR* extension = rvn_dialog_wide(desc->default_extension);
	BOOL ok;

	if (desc->file_name) MultiByteToWideChar(CP_UTF8, 0, desc->file_name, -1, buffer, BUFFER - 1);
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = rvn_dialog_owner(desc->parent);
	ofn.lpstrFilter = filters;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buffer;
	ofn.nMaxFile = BUFFER;
	ofn.lpstrInitialDir = folder;
	ofn.lpstrTitle = title;
	ofn.lpstrDefExt = (extension && extension[0] == L'.') ? extension + 1 : extension;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
	if (desc->flags & R_FILE_DIALOG_SHOW_HIDDEN) ofn.Flags |= OFN_FORCESHOWHIDDEN;
	if (mode == R_FILE_BROWSER_SAVE) {
		ofn.Flags |= OFN_OVERWRITEPROMPT;
		ok = GetSaveFileNameW(&ofn);
	} else {
		ofn.Flags |= OFN_FILEMUSTEXIST;
		if (desc->flags & R_FILE_DIALOG_MULTIPLE) ofn.Flags |= OFN_ALLOWMULTISELECT;
		ok = GetOpenFileNameW(&ofn);
	}
	if (ok) {
		size_t first = wcslen(buffer);
		if ((ofn.Flags & OFN_ALLOWMULTISELECT) && buffer[first + 1] != 0) {
			/* several files: the folder, then each name, each ended by a zero */
			const WCHAR* name = buffer + first + 1;
			while (*name) {
				WCHAR full[2048];
				size_t length = wcslen(name);
				if (first + length + 2 < RVN_COUNT_OF(full)) {
					memcpy(full, buffer, sizeof(WCHAR) * first);
					full[first] = L'\\';
					memcpy(full + first + (buffer[first - 1] == L'\\' ? 0 : 1), name, sizeof(WCHAR) * (length + 1));
					rvn_dialog_add_wide(full);
				}
				name += length + 1;
			}
		} else {
			rvn_dialog_add_wide(buffer);
		}
	}
	rvn_free(buffer);
	rvn_free(filters);
	rvn_free(title);
	rvn_free(folder);
	rvn_free(extension);
	/* an error of the dialog itself (not a cancel) falls back to the browser of Raven */
	return ok || CommDlgExtendedError() == 0;
}

/* (defined here so that no import library of GUIDs is needed) */
static const GUID rvn_clsid_file_open_dialog = { 0xdc1c5a9c, 0xe88a, 0x4dde, { 0xa5, 0xa1, 0x60, 0xf8, 0x2a, 0x20, 0xae, 0xf7 } };
static const GUID rvn_iid_file_open_dialog = { 0xd57c7288, 0xd4ad, 0x4768, { 0xbe, 0x02, 0x9d, 0x96, 0x95, 0x32, 0xd9, 0x60 } };
static const GUID rvn_iid_shell_item = { 0x43826d1e, 0xe718, 0x42ee, { 0xbc, 0x55, 0xa1, 0xe2, 0x61, 0xc3, 0x7b, 0xfe } };

static int rvn_file_dialog_win32_folder(const RFileDialogDesc* desc, const char* directory) {
	IFileOpenDialog* dialog = NULL;
	IShellItem* item = NULL;
	HRESULT init = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	HRESULT hr;
	int shown = R_FALSE;

	hr = CoCreateInstance(&rvn_clsid_file_open_dialog, NULL, CLSCTX_INPROC_SERVER, &rvn_iid_file_open_dialog, (void**)&dialog);
	if (SUCCEEDED(hr) && dialog) {
		FILEOPENDIALOGOPTIONS options = 0;
		WCHAR* title = rvn_dialog_wide(desc->title);
		WCHAR* folder = rvn_dialog_wide(directory);
		IFileOpenDialog_GetOptions(dialog, &options);
		IFileOpenDialog_SetOptions(dialog, options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR);
		if (title) IFileOpenDialog_SetTitle(dialog, title);
		if (folder && SUCCEEDED(SHCreateItemFromParsingName(folder, NULL, &rvn_iid_shell_item, (void**)&item)) && item) {
			IFileOpenDialog_SetFolder(dialog, item);
			IShellItem_Release(item);
			item = NULL;
		}
		hr = IFileOpenDialog_Show(dialog, rvn_dialog_owner(desc->parent));
		/* a cancel is an answer too */
		shown = SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_CANCELLED);
		if (SUCCEEDED(hr) && SUCCEEDED(IFileOpenDialog_GetResult(dialog, &item)) && item) {
			LPWSTR path = NULL;
			if (SUCCEEDED(IShellItem_GetDisplayName(item, SIGDN_FILESYSPATH, &path)) && path) {
				rvn_dialog_add_wide(path);
				CoTaskMemFree(path);
			}
			IShellItem_Release(item);
		}
		IFileOpenDialog_Release(dialog);
		rvn_free(title);
		rvn_free(folder);
	}
	if (SUCCEEDED(init)) CoUninitialize();
	return shown;
}

static int rvn_file_dialog_native(const RFileDialogDesc* desc, RFileBrowserMode mode, const char* directory) {
	if (mode == R_FILE_BROWSER_FOLDER) return rvn_file_dialog_win32_folder(desc, directory);
	return rvn_file_dialog_win32_files(desc, mode, directory);
}

/* ---------------------------------------------------------------------------
 *  File dialog: zenity / kdialog
 * ------------------------------------------------------------------------- */

#else

#define RVN_DIALOG_MAX_ARGS 48

static int rvn_dialog_tool_exists(const char* name) {
	const char* path = getenv("PATH");
	char candidate[1024];
	if (!path) return R_FALSE;
	while (*path) {
		size_t length = strcspn(path, ":");
		if (length > 0 && length + strlen(name) + 2 < sizeof(candidate)) {
			memcpy(candidate, path, length);
			candidate[length] = '/';
			strcpy(candidate + length + 1, name);
			if (access(candidate, X_OK) == 0) return R_TRUE;
		}
		path += length;
		if (*path == ':') path++;
	}
	return R_FALSE;
}

typedef struct rvn_dialog_args_t {
	char* items[RVN_DIALOG_MAX_ARGS + 1];
	int count;
} rvn_dialog_args_t;

static void rvn_dialog_arg(rvn_dialog_args_t* args, const char* prefix, const char* value) {
	size_t prefix_length = strlen(prefix), value_length = value ? strlen(value) : 0;
	char* item;
	if (args->count >= RVN_DIALOG_MAX_ARGS) return;
	item = (char*)rvn_alloc(prefix_length + value_length + 1);
	memcpy(item, prefix, prefix_length);
	if (value) memcpy(item + prefix_length, value, value_length);
	item[prefix_length + value_length] = '\0';
	args->items[args->count++] = item;
	args->items[args->count] = NULL;
}

/* "*.png;*.jpg" -> "*.png *.jpg" */
static char* rvn_dialog_spaced(const char* patterns) {
	char* copy = rvn_strdup(patterns && patterns[0] ? patterns : "*");
	char* cursor;
	for (cursor = copy; *cursor; cursor++) {
		if (*cursor == ';' || *cursor == ',') *cursor = ' ';
	}
	return copy;
}

/* Folder shown first, with the proposed name: "/home/me/" or "/home/me/name.txt". */
static char* rvn_dialog_start_path(const RFileDialogDesc* desc, const char* directory) {
	char* folder = rvn_path_child(directory ? directory : "/", "");
	char* path;
	if (!desc->file_name || !desc->file_name[0]) return folder;
	path = rvn_path_child(folder, desc->file_name);
	rvn_free(folder);
	return path;
}

static void rvn_dialog_args_zenity(rvn_dialog_args_t* args, const RFileDialogDesc* desc, RFileBrowserMode mode, const char* directory) {
	char* start = rvn_dialog_start_path(desc, directory);
	int i;
	rvn_dialog_arg(args, "zenity", NULL);
	rvn_dialog_arg(args, "--file-selection", NULL);
	rvn_dialog_arg(args, "--title=", rvn_file_dialog_title(desc, mode));
	rvn_dialog_arg(args, "--filename=", start);
	if (mode == R_FILE_BROWSER_SAVE) {
		rvn_dialog_arg(args, "--save", NULL);
		rvn_dialog_arg(args, "--confirm-overwrite", NULL);
	} else if (mode == R_FILE_BROWSER_FOLDER) {
		rvn_dialog_arg(args, "--directory", NULL);
	} else if (desc->flags & R_FILE_DIALOG_MULTIPLE) {
		rvn_dialog_arg(args, "--multiple", NULL);
		rvn_dialog_arg(args, "--separator=", "\n");
	}
	for (i = 0; i < desc->filter_count && desc->filters && mode != R_FILE_BROWSER_FOLDER; i++) {
		char* patterns = rvn_dialog_spaced(desc->filters[i].patterns);
		char filter[512];
		snprintf(filter, sizeof(filter), "%.160s | %.300s", desc->filters[i].name ? desc->filters[i].name : patterns, patterns);
		rvn_dialog_arg(args, "--file-filter=", filter);
		rvn_free(patterns);
	}
	rvn_free(start);
}

static void rvn_dialog_args_kdialog(rvn_dialog_args_t* args, const RFileDialogDesc* desc, RFileBrowserMode mode, const char* directory) {
	char* start = rvn_dialog_start_path(desc, directory);
	char filter[1024];
	size_t used = 0;
	int i;
	rvn_dialog_arg(args, "kdialog", NULL);
	rvn_dialog_arg(args, "--title", NULL);
	rvn_dialog_arg(args, rvn_file_dialog_title(desc, mode), NULL);
	if (mode == R_FILE_BROWSER_FOLDER) {
		rvn_dialog_arg(args, "--getexistingdirectory", NULL);
		rvn_dialog_arg(args, start, NULL);
	} else {
		if (mode == R_FILE_BROWSER_OPEN && (desc->flags & R_FILE_DIALOG_MULTIPLE)) {
			rvn_dialog_arg(args, "--multiple", NULL);
			rvn_dialog_arg(args, "--separate-output", NULL);
		}
		rvn_dialog_arg(args, mode == R_FILE_BROWSER_SAVE ? "--getsavefilename" : "--getopenfilename", NULL);
		rvn_dialog_arg(args, start, NULL);
		/* "Images (*.png *.jpg)|All files (*)" */
		filter[0] = '\0';
		for (i = 0; i < desc->filter_count && desc->filters; i++) {
			char* patterns = rvn_dialog_spaced(desc->filters[i].patterns);
			int written = snprintf(filter + used, sizeof(filter) - used, "%s%.160s (%.300s)", i ? "|" : "",
				desc->filters[i].name ? desc->filters[i].name : "Files", patterns);
			rvn_free(patterns);
			if (written < 0 || (size_t)written >= sizeof(filter) - used) break;
			used += (size_t)written;
		}
		if (filter[0]) rvn_dialog_arg(args, filter, NULL);
	}
	rvn_free(start);
}

/*
 * Runs the dialog program and reads the chosen paths from its output, one per
 * line. The application keeps running its frames meanwhile, without taking
 * input. Returns R_FALSE when the program could not show a dialog.
 */
static int rvn_dialog_run_tool(rvn_dialog_args_t* args) {
	int fds[2], status = 0, shown = R_FALSE;
	char* output = NULL;
	size_t used = 0, cap = 0;
	pid_t child;

	if (pipe(fds) != 0) return R_FALSE;
	child = fork();
	if (child < 0) {
		close(fds[0]);
		close(fds[1]);
		return R_FALSE;
	}
	if (child == 0) {
		int null_fd = open("/dev/null", O_WRONLY);
		dup2(fds[1], STDOUT_FILENO);
		if (null_fd >= 0) dup2(null_fd, STDERR_FILENO);
		close(fds[0]);
		close(fds[1]);
		execvp(args->items[0], args->items);
		_exit(127);
	}
	close(fds[1]);
	fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK);

	g_rvn.input_blocked++;
	{
		int in_update = g_rvn.in_update;
		g_rvn.in_update = R_FALSE;
		for (;;) {
			struct pollfd poll_fd;
			char chunk[1024];
			ssize_t count;
			poll_fd.fd = fds[0];
			poll_fd.events = POLLIN;
			poll_fd.revents = 0;
			if (poll(&poll_fd, 1, 0) > 0) {
				count = read(fds[0], chunk, sizeof(chunk));
				if (count > 0) {
					if (used + (size_t)count + 1 > cap) {
						cap = (used + (size_t)count + 1) * 2;
						output = (char*)rvn_realloc(output, cap);
					}
					memcpy(output + used, chunk, (size_t)count);
					used += (size_t)count;
					continue;
				}
				if (count == 0 || (errno != EAGAIN && errno != EINTR)) break;
			}
			/* the windows of the application stay alive behind the dialog */
			rvn_dialog_frame(0.03);
		}
		g_rvn.in_update = in_update;
	}
	g_rvn.input_blocked--;
	close(fds[0]);
	while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}

	if (WIFEXITED(status) && WEXITSTATUS(status) != 127) {
		/* 0: chosen, 1: cancelled */
		shown = WEXITSTATUS(status) == 0 || WEXITSTATUS(status) == 1;
		if (WEXITSTATUS(status) == 0 && output) {
			char* line = output;
			output[used] = '\0';
			while (*line) {
				size_t length = strcspn(line, "\n");
				char saved = line[length];
				line[length] = '\0';
				rvn_dialogs_add_result(line);
				line[length] = saved;
				line += length;
				if (*line == '\n') line++;
			}
		}
	}
	rvn_free(output);
	return shown;
}

static int rvn_file_dialog_native(const RFileDialogDesc* desc, RFileBrowserMode mode, const char* directory) {
	rvn_dialog_args_t args;
	const char* desktop = getenv("XDG_CURRENT_DESKTOP");
	int kde = desktop && (strstr(desktop, "KDE") != NULL);
	int has_zenity, has_kdialog, shown, i;

	if (!getenv("DISPLAY") && !getenv("WAYLAND_DISPLAY")) return R_FALSE;
	has_zenity = rvn_dialog_tool_exists("zenity");
	has_kdialog = rvn_dialog_tool_exists("kdialog");
	if (!has_zenity && !has_kdialog) return R_FALSE;

	memset(&args, 0, sizeof(args));
	if (has_kdialog && (kde || !has_zenity)) rvn_dialog_args_kdialog(&args, desc, mode, directory);
	else rvn_dialog_args_zenity(&args, desc, mode, directory);
	shown = rvn_dialog_run_tool(&args);
	for (i = 0; i < args.count; i++) rvn_free(args.items[i]);
	return shown;
}

#endif

/* ---------------------------------------------------------------------------
 *  File dialogs: public API
 * ------------------------------------------------------------------------- */

static const char* rvn_file_dialog(const RFileDialogDesc* user_desc, RFileBrowserMode mode) {
	RFileDialogDesc desc;
	char* directory = NULL;
	int shown = R_FALSE;

	memset(&desc, 0, sizeof(desc));
	if (user_desc) desc = *user_desc;
	if (desc.filter_count < 0 || !desc.filters) desc.filter_count = 0;
	rvn_dialogs_clear_results();

	if (desc.directory && desc.directory[0]) directory = rvn_path_absolute(desc.directory);
	if ((!directory || !rvn_path_is_dir(directory)) && g_dialogs.directory) {
		rvn_free(directory);
		directory = rvn_strdup(g_dialogs.directory);
	}
	if (!directory || !rvn_path_is_dir(directory)) {
		rvn_free(directory);
		directory = rvn_path_home();
	}

	/* without a window system there is nobody to show a native dialog to */
	if (!(desc.flags & R_FILE_DIALOG_RAVEN) && g_rvn.platform->system != R_WINDOW_SYSTEM_NONE) {
		shown = rvn_file_dialog_native(&desc, mode, directory);
		if (!shown) rvn_dialogs_clear_results();
	}
	if (!shown) rvn_file_dialog_raven(&desc, mode, directory);
	rvn_free(directory);

	if (g_dialogs.count == 0) return NULL;
	/* the next dialog starts where this one ended */
	rvn_free(g_dialogs.directory);
	g_dialogs.directory = mode == R_FILE_BROWSER_FOLDER ? rvn_strdup(g_dialogs.paths[0]) : rvn_path_parent(g_dialogs.paths[0]);
	return g_dialogs.paths[0];
}

const char* Rfile_dialog_open(const RFileDialogDesc* desc) {
	RVN_API_GUARD(NULL);
	return rvn_file_dialog(desc, R_FILE_BROWSER_OPEN);
}

const char* Rfile_dialog_save(const RFileDialogDesc* desc) {
	RVN_API_GUARD(NULL);
	return rvn_file_dialog(desc, R_FILE_BROWSER_SAVE);
}

const char* Rfile_dialog_select_folder(const RFileDialogDesc* desc) {
	RVN_API_GUARD(NULL);
	return rvn_file_dialog(desc, R_FILE_BROWSER_FOLDER);
}

int Rfile_dialog_get_result_count(void) {
	return g_rvn.initialized ? g_dialogs.count : 0;
}

const char* Rfile_dialog_get_result(int index) {
	if (!g_rvn.initialized || index < 0 || index >= g_dialogs.count) return NULL;
	return g_dialogs.paths[index];
}

char* Rfile_read(const char* path, size_t* size) {
	return rvn_read_file(path, size);
}

int Rfile_write(const char* path, const void* data, size_t size) {
	if (!path || (!data && size > 0)) return R_FALSE;
	return rvn_write_file(path, data, size);
}
