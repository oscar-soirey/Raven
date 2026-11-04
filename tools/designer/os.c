/**
 * Raven Designer - os.c
 * Folders and files, for Windows and for POSIX systems. Paths are UTF-8; on
 * Windows they go through the wide functions.
 */

#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#endif

#include "designer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#endif

/* ---------------------------------------------------------------------------
 *  Small things
 * ------------------------------------------------------------------------- */

char* ds_strdup(const char* text) {
	size_t length = text ? strlen(text) : 0;
	char* copy = (char*)malloc(length + 1);
	if (length) memcpy(copy, text, length);
	copy[length] = '\0';
	return copy;
}

int ds_contains(const char* text, const char* part) {
	size_t length = strlen(part), i, j;
	if (!length) return 1;
	for (i = 0; text[i]; i++) {
		for (j = 0; j < length; j++) if (!text[i + j] || tolower((unsigned char)text[i + j]) != tolower((unsigned char)part[j])) break;
		if (j == length) return 1;
	}
	return 0;
}

int ds_iequal(const char* a, const char* b) {
	if (!a || !b) return a == b;
	for (; *a && *b; a++, b++) if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
	return *a == *b;
}

void ds_buffer_add(ds_buffer_t* buffer, const char* text, size_t length) {
	if (buffer->size + length + 1 > buffer->cap) {
		size_t cap = buffer->cap ? buffer->cap * 2 : 1024;
		while (cap < buffer->size + length + 1) cap *= 2;
		buffer->data = (char*)realloc(buffer->data, cap);
		buffer->cap = cap;
	}
	if (length) memcpy(buffer->data + buffer->size, text, length);
	buffer->size += length;
	buffer->data[buffer->size] = '\0';
}

void ds_buffer_printf(ds_buffer_t* buffer, const char* format, ...) {
	char small[2048];
	va_list args;
	int n;
	va_start(args, format);
	n = vsnprintf(small, sizeof(small), format, args);
	va_end(args);
	if (n < 0) return;
	if ((size_t)n < sizeof(small)) {
		ds_buffer_add(buffer, small, (size_t)n);
	} else {
		char* big = (char*)malloc((size_t)n + 1);
		va_start(args, format);
		vsnprintf(big, (size_t)n + 1, format, args);
		va_end(args);
		ds_buffer_add(buffer, big, (size_t)n);
		free(big);
	}
}

void ds_buffer_free(ds_buffer_t* buffer) {
	free(buffer->data);
	memset(buffer, 0, sizeof(*buffer));
}

char* ds_normalize(const char* path) {
	char* copy = ds_strdup(path);
	char* p;
	for (p = copy; *p; p++) if (*p == '\\') *p = '/';
	return copy;
}

char* ds_join(const char* a, const char* b) {
	size_t la = strlen(a), lb = strlen(b);
	char* out = (char*)malloc(la + lb + 2);
	memcpy(out, a, la);
	if (la && a[la - 1] != '/' && a[la - 1] != '\\') out[la++] = '/';
	memcpy(out + la, b, lb + 1);
	return out;
}

const char* ds_basename(const char* path) {
	const char* base = path;
	const char* p;
	for (p = path; *p; p++) if (*p == '/' || *p == '\\') base = p + 1;
	return base;
}

const char* ds_extension(const char* path) {
	static char ext[16];
	const char* base = ds_basename(path);
	const char* dot = strrchr(base, '.');
	size_t i;
	if (!dot || dot == base) return "";
	for (i = 0; dot[1 + i] && i + 1 < sizeof(ext); i++) ext[i] = (char)tolower((unsigned char)dot[1 + i]);
	ext[i] = '\0';
	return ext;
}

int ds_same_path(const char* a, const char* b) {
	if (!a || !b) return 0;
	for (; *a && *b; a++, b++) {
		char x = *a == '\\' ? '/' : *a, y = *b == '\\' ? '/' : *b;
#if defined(_WIN32)
		x = (char)tolower((unsigned char)x);
		y = (char)tolower((unsigned char)y);
#endif
		if (x != y) return 0;
	}
	return *a == *b;
}

static int compare_entries(const void* a, const void* b) {
	const ds_entry_t* x = (const ds_entry_t*)a;
	const ds_entry_t* y = (const ds_entry_t*)b;
	const char *p, *q;
	if (x->is_dir != y->is_dir) return y->is_dir - x->is_dir;
	for (p = x->name, q = y->name; *p && *q; p++, q++) {
		int c = tolower((unsigned char)*p) - tolower((unsigned char)*q);
		if (c) return c;
	}
	return (int)(unsigned char)*p - (int)(unsigned char)*q;
}

/* ---------------------------------------------------------------------------
 *  Windows
 * ------------------------------------------------------------------------- */

#if defined(_WIN32)

static wchar_t* widen(const char* text) {
	int n = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
	wchar_t* wide = (wchar_t*)malloc(sizeof(wchar_t) * (size_t)(n > 0 ? n : 1));
	if (n <= 0 || !MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, n)) wide[0] = 0;
	return wide;
}

static void narrow(const wchar_t* wide, char* out, size_t size) {
	if (!WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, (int)size, NULL, NULL)) out[0] = 0;
}

int ds_list_dir(const char* path, ds_entry_t** entries) {
	WIN32_FIND_DATAW data;
	char* pattern = ds_join(path, "*");
	wchar_t* wide = widen(pattern);
	HANDLE find = FindFirstFileW(wide, &data);
	int count = 0, cap = 0;
	free(pattern);
	free(wide);
	*entries = NULL;
	if (find == INVALID_HANDLE_VALUE) return -1;
	do {
		ds_entry_t entry;
		narrow(data.cFileName, entry.name, sizeof(entry.name));
		if (!strcmp(entry.name, ".") || !strcmp(entry.name, "..")) continue;
		entry.is_dir = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		if (count == cap) {
			cap = cap ? cap * 2 : 32;
			*entries = (ds_entry_t*)realloc(*entries, sizeof(ds_entry_t) * (size_t)cap);
		}
		(*entries)[count++] = entry;
	} while (FindNextFileW(find, &data));
	FindClose(find);
	if (count) qsort(*entries, (size_t)count, sizeof(ds_entry_t), compare_entries);
	return count;
}

int ds_is_dir(const char* path) {
	wchar_t* wide = widen(path);
	DWORD attributes = GetFileAttributesW(wide);
	free(wide);
	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

int ds_file_exists(const char* path) {
	wchar_t* wide = widen(path);
	DWORD attributes = GetFileAttributesW(wide);
	free(wide);
	return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

int ds_make_dirs(const char* path) {
	char* copy = ds_strdup(path);
	char* p;
	int ok;
	for (p = copy; *p; p++) {
		if ((*p == '/' || *p == '\\') && p > copy && p[-1] != ':') {
			char keep = *p;
			wchar_t* wide;
			*p = 0;
			wide = widen(copy);
			CreateDirectoryW(wide, NULL);
			free(wide);
			*p = keep;
		}
	}
	{
		wchar_t* wide = widen(copy);
		CreateDirectoryW(wide, NULL);
		free(wide);
	}
	ok = ds_is_dir(copy);
	free(copy);
	return ok;
}

double ds_file_time(const char* path) {
	WIN32_FILE_ATTRIBUTE_DATA data;
	wchar_t* wide = widen(path);
	BOOL ok = GetFileAttributesExW(wide, GetFileExInfoStandard, &data);
	ULARGE_INTEGER t;
	free(wide);
	if (!ok) return 0.0;
	t.LowPart = data.ftLastWriteTime.dwLowDateTime;
	t.HighPart = data.ftLastWriteTime.dwHighDateTime;
	/* (to the tenth of a microsecond: two writes in the same second are two changes) */
	return (double)t.QuadPart / 10000000.0 - 11644473600.0;
}

const char* ds_exe_dir(void) {
	static char dir[1024];
	if (!dir[0]) {
		wchar_t wide[1024];
		char* slash;
		GetModuleFileNameW(NULL, wide, 1024);
		narrow(wide, dir, sizeof(dir));
		slash = strrchr(dir, '\\');
		if (!slash) slash = strrchr(dir, '/');
		if (slash) *slash = 0;
	}
	return dir;
}

static void env_folder(const wchar_t* name, char* out, size_t size) {
	wchar_t value[1024];
	DWORD n = GetEnvironmentVariableW(name, value, 1024);
	out[0] = 0;
	if (n > 0 && n < 1024) narrow(value, out, size);
}

const char* ds_home_dir(void) {
	static char dir[1024];
	if (!dir[0]) {
		char* n;
		env_folder(L"USERPROFILE", dir, sizeof(dir));
		if (!dir[0]) snprintf(dir, sizeof(dir), "C:/");
		n = ds_normalize(dir);
		snprintf(dir, sizeof(dir), "%s", n);
		free(n);
	}
	return dir;
}

const char* ds_config_dir(void) {
	static char dir[1100];
	if (!dir[0]) {
		char base[1024];
		char* n;
		env_folder(L"APPDATA", base, sizeof(base));
		if (!base[0]) snprintf(base, sizeof(base), "%s/AppData/Roaming", ds_home_dir());
		n = ds_normalize(base);
		snprintf(dir, sizeof(dir), "%s/Raven", n);
		free(n);
		ds_make_dirs(dir);
	}
	return dir;
}

int ds_remove_file(const char* path) {
	wchar_t* wide = widen(path);
	BOOL ok = DeleteFileW(wide);
	free(wide);
	return ok != 0;
}

int ds_remove_empty_dir(const char* path) {
	wchar_t* wide = widen(path);
	BOOL ok = RemoveDirectoryW(wide);
	free(wide);
	return ok != 0;
}

int ds_rename(const char* from, const char* to) {
	wchar_t* a = widen(from);
	wchar_t* b = widen(to);
	BOOL ok = MoveFileExW(a, b, 0);
	free(a);
	free(b);
	return ok != 0;
}

/* ---------------------------------------------------------------------------
 *  POSIX
 * ------------------------------------------------------------------------- */

#else

int ds_list_dir(const char* path, ds_entry_t** entries) {
	DIR* dir = opendir(path);
	struct dirent* item;
	int count = 0, cap = 0;
	*entries = NULL;
	if (!dir) return -1;
	while ((item = readdir(dir)) != NULL) {
		ds_entry_t entry;
		char* full;
		if (!strcmp(item->d_name, ".") || !strcmp(item->d_name, "..")) continue;
		snprintf(entry.name, sizeof(entry.name), "%s", item->d_name);
		full = ds_join(path, item->d_name);
		entry.is_dir = ds_is_dir(full);
		free(full);
		if (count == cap) {
			cap = cap ? cap * 2 : 32;
			*entries = (ds_entry_t*)realloc(*entries, sizeof(ds_entry_t) * (size_t)cap);
		}
		(*entries)[count++] = entry;
	}
	closedir(dir);
	if (count) qsort(*entries, (size_t)count, sizeof(ds_entry_t), compare_entries);
	return count;
}

int ds_is_dir(const char* path) {
	struct stat s;
	return stat(path, &s) == 0 && S_ISDIR(s.st_mode);
}

int ds_file_exists(const char* path) {
	struct stat s;
	return stat(path, &s) == 0 && S_ISREG(s.st_mode);
}

int ds_make_dirs(const char* path) {
	char* copy;
	char* p;
	int ok;
	/* (an empty path is no folder: copy + 1 would be past its end) */
	if (!path || !path[0]) return 0;
	copy = ds_strdup(path);
	for (p = copy + 1; *p; p++) {
		if (*p == '/') {
			*p = 0;
			mkdir(copy, 0755);
			*p = '/';
		}
	}
	mkdir(copy, 0755);
	ok = ds_is_dir(copy);
	free(copy);
	return ok;
}

double ds_file_time(const char* path) {
	struct stat s;
	if (stat(path, &s) != 0) return 0.0;
	/* (to the nanosecond where the system keeps it: two writes in the same second are two changes) */
#if defined(__APPLE__)
	return (double)s.st_mtimespec.tv_sec + (double)s.st_mtimespec.tv_nsec * 1e-9;
#else
	return (double)s.st_mtim.tv_sec + (double)s.st_mtim.tv_nsec * 1e-9;
#endif
}

const char* ds_exe_dir(void) {
	static char dir[1024];
	if (!dir[0]) {
		char* slash;
#if defined(__APPLE__)
		uint32_t size = sizeof(dir);
		if (_NSGetExecutablePath(dir, &size) != 0) dir[0] = 0;
#else
		ssize_t n = readlink("/proc/self/exe", dir, sizeof(dir) - 1);
		dir[n > 0 ? n : 0] = 0;
#endif
		slash = strrchr(dir, '/');
		if (slash) *slash = 0;
		if (!dir[0]) strcpy(dir, ".");
	}
	return dir;
}

const char* ds_home_dir(void) {
	static char dir[1024];
	if (!dir[0]) {
		const char* home = getenv("HOME");
		snprintf(dir, sizeof(dir), "%s", home && home[0] ? home : "/tmp");
	}
	return dir;
}

const char* ds_config_dir(void) {
	static char dir[1100];
	if (!dir[0]) {
#if defined(__APPLE__)
		snprintf(dir, sizeof(dir), "%s/Library/Application Support/Raven", ds_home_dir());
#else
		const char* xdg = getenv("XDG_CONFIG_HOME");
		if (xdg && xdg[0] == '/') snprintf(dir, sizeof(dir), "%s/raven", xdg);
		else snprintf(dir, sizeof(dir), "%s/.config/raven", ds_home_dir());
#endif
		ds_make_dirs(dir);
	}
	return dir;
}

int ds_remove_file(const char* path) {
	return unlink(path) == 0;
}

int ds_remove_empty_dir(const char* path) {
	return rmdir(path) == 0;
}

int ds_rename(const char* from, const char* to) {
	/* (never over a file that is there: as on Windows) */
	if (ds_file_exists(to) || ds_is_dir(to)) return 0;
	return rename(from, to) == 0;
}

#endif

/* A folder and everything in it. */
int ds_remove_tree(const char* path) {
	ds_entry_t* entries;
	int count = ds_list_dir(path, &entries), i, ok = 1;
	for (i = 0; i < count; i++) {
		char* full = ds_join(path, entries[i].name);
		ok &= entries[i].is_dir ? ds_remove_tree(full) : ds_remove_file(full);
		free(full);
	}
	free(entries);
	return ds_remove_empty_dir(path) && ok;
}

/* "C:/Users/ada/Documents/x" -> "~/Documents/x": shorter, for the eyes only. */
void ds_pretty_path(const char* path, char* out, size_t size) {
	char* normalized = ds_normalize(path);
	char* home = ds_normalize(ds_home_dir());
	size_t length = strlen(home);
	if (length > 1 && !strncmp(normalized, home, length) && (normalized[length] == '/' || !normalized[length]))
		snprintf(out, size, "~%s", normalized + length);
	else
		snprintf(out, size, "%s", normalized);
	free(normalized);
	free(home);
}
