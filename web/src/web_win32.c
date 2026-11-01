/**
 * Raven Web - web_win32.c
 * Windows: the browser process and its two pipes, the reading thread,
 * folders.
 *
 * The browser is told the handles of its ends of the pipes on its command
 * line (--remote-debugging-io-pipes), and inherits them. It is put in a job
 * that is closed with the application, so that it never outlives it.
 */

#if defined(_WIN32)

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined(UNICODE)
#define UNICODE
#endif
#if !defined(_UNICODE)
#define _UNICODE
#endif

#include "web_internal.h"

#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>

/* ---------------------------------------------------------------------------
 *  Text: UTF-8 here, UTF-16 for Windows
 * ------------------------------------------------------------------------- */

static wchar_t* rw_wide(const char* text) {
	int length;
	wchar_t* wide;
	if (!text) return NULL;
	length = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
	if (length <= 0) return NULL;
	wide = (wchar_t*)rw_alloc(sizeof(wchar_t) * (size_t)length);
	if (wide) MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, length);
	return wide;
}

static char* rw_narrow(const wchar_t* wide) {
	int length;
	char* text;
	if (!wide) return NULL;
	length = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
	if (length <= 0) return NULL;
	text = (char*)rw_alloc((size_t)length);
	if (text) WideCharToMultiByte(CP_UTF8, 0, wide, -1, text, length, NULL, NULL);
	return text;
}

/* ---------------------------------------------------------------------------
 *  Process
 * ------------------------------------------------------------------------- */

struct rw_process_t {
	HANDLE process;
	HANDLE job;
	HANDLE to_child;
	HANDLE from_child;
};

/* One argument, quoted the way the C runtime of the child will read it back. */
static void rw_quote(rw_text_t* line, const char* argument) {
	const char* cursor;
	int slashes = 0;
	if (line->length) rw_text_puts(line, " ");
	if (argument[0] && !strpbrk(argument, " \t\"")) {
		rw_text_puts(line, argument);
		return;
	}
	rw_text_puts(line, "\"");
	for (cursor = argument; *cursor; cursor++) {
		if (*cursor == '\\') {
			slashes++;
		} else if (*cursor == '"') {
			/* the backslashes before a quote are doubled, and the quote escaped */
			while (slashes-- > 0) rw_text_puts(line, "\\");
			rw_text_puts(line, "\\");
			slashes = 0;
		} else {
			slashes = 0;
		}
		rw_text_add(line, cursor, 1);
	}
	/* those before the closing quote are doubled too */
	while (slashes-- > 0) rw_text_puts(line, "\\");
	rw_text_puts(line, "\"");
}

rw_process_t* rw_process_start(const char* const* arguments) {
	SECURITY_ATTRIBUTES inherit;
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits;
	STARTUPINFOW startup;
	PROCESS_INFORMATION information;
	HANDLE child_read = NULL, our_write = NULL, our_read = NULL, child_write = NULL;
	rw_process_t* process;
	rw_text_t line;
	wchar_t* wide;
	int i, started;

	memset(&inherit, 0, sizeof(inherit));
	inherit.nLength = sizeof(inherit);
	inherit.bInheritHandle = TRUE;
	if (!CreatePipe(&child_read, &our_write, &inherit, 0)) return NULL;
	if (!CreatePipe(&our_read, &child_write, &inherit, 1 << 20)) {
		CloseHandle(child_read);
		CloseHandle(our_write);
		return NULL;
	}
	/* our ends stay ours */
	SetHandleInformation(our_write, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(our_read, HANDLE_FLAG_INHERIT, 0);

	memset(&line, 0, sizeof(line));
	for (i = 0; arguments[i]; i++) rw_quote(&line, arguments[i]);
	rw_text_printf(&line, " --remote-debugging-io-pipes=%u,%u", (unsigned)(uintptr_t)child_read, (unsigned)(uintptr_t)child_write);
	wide = rw_wide(line.data);
	rw_text_free(&line);

	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&information, 0, sizeof(information));
	started = wide && CreateProcessW(NULL, wide, NULL, NULL, TRUE, CREATE_NO_WINDOW | CREATE_SUSPENDED, NULL, NULL, &startup, &information);
	rw_free(wide);
	CloseHandle(child_read);
	CloseHandle(child_write);
	if (!started) {
		CloseHandle(our_write);
		CloseHandle(our_read);
		return NULL;
	}

	process = (rw_process_t*)rw_alloc(sizeof(rw_process_t));
	if (!process) {
		TerminateProcess(information.hProcess, 1);
		CloseHandle(information.hThread);
		CloseHandle(information.hProcess);
		CloseHandle(our_write);
		CloseHandle(our_read);
		return NULL;
	}
	process->process = information.hProcess;
	process->to_child = our_write;
	process->from_child = our_read;
	/* the browser and the processes it starts die when the job closes: at the latest when the application ends */
	process->job = CreateJobObjectW(NULL, NULL);
	if (process->job) {
		memset(&limits, 0, sizeof(limits));
		limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
		SetInformationJobObject(process->job, JobObjectExtendedLimitInformation, &limits, sizeof(limits));
		AssignProcessToJobObject(process->job, process->process);
	}
	ResumeThread(information.hThread);
	CloseHandle(information.hThread);
	return process;
}

int rw_process_write(rw_process_t* process, const void* data, size_t size) {
	const char* cursor = (const char*)data;
	while (size > 0) {
		DWORD written = 0, chunk = size > 0x100000 ? 0x100000 : (DWORD)size;
		if (!process->to_child || !WriteFile(process->to_child, cursor, chunk, &written, NULL) || written == 0) return 0;
		cursor += written;
		size -= written;
	}
	return 1;
}

int rw_process_read(rw_process_t* process, void* buffer, size_t size) {
	DWORD got = 0;
	if (!ReadFile(process->from_child, buffer, size > 0x100000 ? 0x100000 : (DWORD)size, &got, NULL)) return 0;
	return (int)got;
}

void rw_process_hang_up(rw_process_t* process) {
	/* the child sees the end of its input, and the read that waits in the other thread gives up */
	if (process->to_child) {
		/* (a write that waits in the other thread gives up first) */
		CancelIoEx(process->to_child, NULL);
		CloseHandle(process->to_child);
		process->to_child = NULL;
	}
	CancelIoEx(process->from_child, NULL);
}

int rw_process_exit_code(rw_process_t* process, int wait_ms, long* code) {
	DWORD exit_code = 0;
	if (WaitForSingleObject(process->process, wait_ms > 0 ? (DWORD)wait_ms : 0) != WAIT_OBJECT_0) return 0;
	if (!GetExitCodeProcess(process->process, &exit_code)) return 0;
	*code = (long)exit_code;
	return 1;
}

void rw_process_stop(rw_process_t* process) {
	rw_process_hang_up(process);
	/* a browser leaves by itself when its pipe closes: it is given two seconds */
	if (WaitForSingleObject(process->process, 2000) != WAIT_OBJECT_0) TerminateProcess(process->process, 1);
	if (process->job) CloseHandle(process->job);
	CloseHandle(process->from_child);
	CloseHandle(process->process);
	rw_free(process);
}

/* ---------------------------------------------------------------------------
 *  Thread and lock
 * ------------------------------------------------------------------------- */

struct rw_thread_t {
	HANDLE handle;
	void (*function)(void*);
	void* argument;
};

static unsigned __stdcall rw_thread_main(void* argument) {
	rw_thread_t* thread = (rw_thread_t*)argument;
	thread->function(thread->argument);
	return 0;
}

rw_thread_t* rw_thread_start(void (*function)(void*), void* argument) {
	rw_thread_t* thread = (rw_thread_t*)rw_alloc(sizeof(rw_thread_t));
	if (!thread) return NULL;
	thread->function = function;
	thread->argument = argument;
	thread->handle = (HANDLE)_beginthreadex(NULL, 0, rw_thread_main, thread, 0, NULL);
	if (!thread->handle) {
		rw_free(thread);
		return NULL;
	}
	return thread;
}

void rw_thread_join(rw_thread_t* thread) {
	if (!thread) return;
	WaitForSingleObject(thread->handle, INFINITE);
	CloseHandle(thread->handle);
	rw_free(thread);
}

struct rw_mutex_t {
	CRITICAL_SECTION section;
};

rw_mutex_t* rw_mutex_create(void) {
	rw_mutex_t* mutex = (rw_mutex_t*)rw_alloc(sizeof(rw_mutex_t));
	if (mutex) InitializeCriticalSection(&mutex->section);
	return mutex;
}

void rw_mutex_destroy(rw_mutex_t* mutex) {
	if (!mutex) return;
	DeleteCriticalSection(&mutex->section);
	rw_free(mutex);
}

void rw_mutex_lock(rw_mutex_t* mutex) {
	EnterCriticalSection(&mutex->section);
}

void rw_mutex_unlock(rw_mutex_t* mutex) {
	LeaveCriticalSection(&mutex->section);
}

/* ---------------------------------------------------------------------------
 *  Files
 * ------------------------------------------------------------------------- */

int rw_is_program(const char* path) {
	wchar_t* wide = rw_wide(path);
	DWORD attributes = wide && wide[0] ? GetFileAttributesW(wide) : INVALID_FILE_ATTRIBUTES;
	rw_free(wide);
	return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

char* rw_find_browser(void) {
	static const wchar_t* const roots[] = { L"ProgramFiles(x86)", L"ProgramFiles", L"LocalAppData" };
	/* Edge first: it is on every Windows 10 and 11 */
	static const wchar_t* const programs[] = {
		L"\\Microsoft\\Edge\\Application\\msedge.exe",
		L"\\Google\\Chrome\\Application\\chrome.exe",
		L"\\Chromium\\Application\\chrome.exe",
		L"\\BraveSoftware\\Brave-Browser\\Application\\brave.exe",
		L"\\Vivaldi\\Application\\vivaldi.exe"
	};
	wchar_t root[MAX_PATH], path[MAX_PATH * 2];
	size_t p, r;
	for (p = 0; p < sizeof(programs) / sizeof(programs[0]); p++) {
		for (r = 0; r < sizeof(roots) / sizeof(roots[0]); r++) {
			DWORD length = GetEnvironmentVariableW(roots[r], root, MAX_PATH), attributes;
			if (length == 0 || length >= MAX_PATH) continue;
			if (wcslen(root) + wcslen(programs[p]) + 1 > sizeof(path) / sizeof(path[0])) continue;
			wcscpy(path, root);
			wcscat(path, programs[p]);
			attributes = GetFileAttributesW(path);
			if (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY)) return rw_narrow(path);
		}
	}
	return NULL;
}

char* rw_temp_folder(void) {
	wchar_t base[MAX_PATH], path[MAX_PATH + 64];
	DWORD length = GetTempPathW(MAX_PATH, base);
	int attempt;
	if (length == 0 || length >= MAX_PATH) return NULL;
	for (attempt = 0; attempt < 16; attempt++) {
		_snwprintf(path, sizeof(path) / sizeof(path[0]), L"%lsraven-web-%lu-%lu", base, (unsigned long)GetCurrentProcessId(), (unsigned long)GetTickCount() + (unsigned long)attempt);
		path[sizeof(path) / sizeof(path[0]) - 1] = 0;
		if (CreateDirectoryW(path, NULL)) return rw_narrow(path);
	}
	return NULL;
}

static void rw_remove_folder_wide(const wchar_t* path) {
	WIN32_FIND_DATAW found;
	wchar_t pattern[MAX_PATH * 2];
	HANDLE search;
	if (wcslen(path) + 3 > sizeof(pattern) / sizeof(pattern[0])) return;
	wcscpy(pattern, path);
	wcscat(pattern, L"\\*");
	search = FindFirstFileW(pattern, &found);
	if (search != INVALID_HANDLE_VALUE) {
		do {
			wchar_t child[MAX_PATH * 2];
			if (wcscmp(found.cFileName, L".") == 0 || wcscmp(found.cFileName, L"..") == 0) continue;
			if (wcslen(path) + wcslen(found.cFileName) + 2 > sizeof(child) / sizeof(child[0])) continue;
			wcscpy(child, path);
			wcscat(child, L"\\");
			wcscat(child, found.cFileName);
			/* (a link to a folder is removed, never followed) */
			if ((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(found.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
				rw_remove_folder_wide(child);
			} else {
				if (found.dwFileAttributes & FILE_ATTRIBUTE_READONLY) SetFileAttributesW(child, found.dwFileAttributes & ~(DWORD)FILE_ATTRIBUTE_READONLY);
				if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) RemoveDirectoryW(child);
				else DeleteFileW(child);
			}
		} while (FindNextFileW(search, &found));
		FindClose(search);
	}
	RemoveDirectoryW(path);
}

void rw_remove_folder(const char* path) {
	wchar_t* wide = rw_wide(path);
	if (wide && wide[0]) rw_remove_folder_wide(wide);
	rw_free(wide);
}

char* rw_file_url(const char* path) {
	wchar_t* wide = rw_wide(path);
	wchar_t absolute[MAX_PATH * 2];
	char* narrow;
	rw_text_t url;
	const unsigned char* cursor;
	DWORD length;
	if (!wide) return NULL;
	length = GetFullPathNameW(wide, sizeof(absolute) / sizeof(absolute[0]), absolute, NULL);
	rw_free(wide);
	if (length == 0 || length >= sizeof(absolute) / sizeof(absolute[0])) return NULL;
	narrow = rw_narrow(absolute);
	if (!narrow) return NULL;
	memset(&url, 0, sizeof(url));
	/* "C:\folder\page.html" -> "file:///C:/folder/page.html"; "\\server\share" -> "file://server/share" */
	if (narrow[0] == '\\' && narrow[1] == '\\') {
		rw_text_puts(&url, "file:");
	} else {
		rw_text_puts(&url, "file:///");
	}
	for (cursor = (const unsigned char*)narrow; *cursor; cursor++) {
		unsigned char c = *cursor;
		if (c == '\\') rw_text_puts(&url, "/");
		else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || strchr("/-_.~:", c)) rw_text_add(&url, (const char*)&c, 1);
		else rw_text_printf(&url, "%%%02X", (unsigned)c);
	}
	rw_free(narrow);
	return url.data;
}

int rw_needs_no_sandbox(void) {
	return 0;
}

#else
typedef int rw_win32_unused_t;
#endif /* _WIN32 */
