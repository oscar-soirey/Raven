/**
 * Raven - core/fs.c
 * Folders, paths and the usual places, for Win32 and POSIX.
 */

#include "fs.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#include <winhttp.h>
#else
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

/* ---------------------------------------------------------------------------
 *  Shared
 * ------------------------------------------------------------------------- */

void rvn_dir_free(rvn_dir_entry_t* entries, uint32_t count) {
	uint32_t i;
	for (i = 0; i < count; i++) rvn_free(entries[i].name);
	rvn_free(entries);
}

const char* rvn_path_name(const char* path) {
	const char* name = path;
	const char* cursor;
	for (cursor = path; *cursor; cursor++) {
		if (rvn_path_is_separator(*cursor) && cursor[1]) name = cursor + 1;
	}
	return name;
}

char* rvn_path_child(const char* folder, const char* name) {
	size_t folder_length = strlen(folder), name_length = strlen(name);
	int separator = folder_length > 0 && !rvn_path_is_separator(folder[folder_length - 1]);
	char* path = (char*)rvn_alloc(folder_length + name_length + 2);
	memcpy(path, folder, folder_length);
	if (separator) path[folder_length] = RVN_PATH_SEPARATOR;
	memcpy(path + folder_length + (separator ? 1 : 0), name, name_length + 1);
	return path;
}

/* Length of the root at the start of an absolute path: "/" or "C:\" or "\\server\share\". 0 when there is none. */
static size_t rvn_path_root_length(const char* path) {
#if defined(_WIN32)
	if (((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':') {
		return rvn_path_is_separator(path[2]) ? 3 : 2;
	}
	if (rvn_path_is_separator(path[0]) && rvn_path_is_separator(path[1])) {
		/* \\server\share */
		size_t i = 2;
		int parts = 0;
		while (path[i] && parts < 2) {
			while (path[i] && !rvn_path_is_separator(path[i])) i++;
			parts++;
			if (path[i] && parts < 2) i++;
		}
		if (path[i]) i++;
		return i;
	}
	return 0;
#else
	return path[0] == '/' ? 1 : 0;
#endif
}

char* rvn_path_parent(const char* path) {
	size_t root = rvn_path_root_length(path);
	size_t length = strlen(path);
	while (length > root && rvn_path_is_separator(path[length - 1])) length--;
	if (length <= root) return NULL;
	while (length > root && !rvn_path_is_separator(path[length - 1])) length--;
	while (length > root && rvn_path_is_separator(path[length - 1])) length--;
	if (length == 0) return NULL;
	return rvn_strndup(path, length);
}

/* Removes "." and ".." and doubled or trailing separators from an absolute path, in place. */
static void rvn_path_clean(char* path) {
	size_t root = rvn_path_root_length(path);
	char* out = path + root;
	const char* in = path + root;

	while (*in) {
		const char* end = in;
		size_t length;
		while (*end && !rvn_path_is_separator(*end)) end++;
		length = (size_t)(end - in);
		if (length == 0 || (length == 1 && in[0] == '.')) {
			/* nothing */
		} else if (length == 2 && in[0] == '.' && in[1] == '.') {
			/* back to the previous component */
			if (out > path + root) {
				out--;
				while (out > path + root && !rvn_path_is_separator(out[-1])) out--;
				if (out > path + root) out--;
			}
		} else {
			if (out > path + root) *out++ = RVN_PATH_SEPARATOR;
			memmove(out, in, length);
			out += length;
		}
		in = *end ? end + 1 : end;
	}
	*out = '\0';
#if defined(_WIN32)
	{
		char* cursor;
		for (cursor = path; *cursor; cursor++) {
			if (*cursor == '/') *cursor = '\\';
		}
	}
#endif
}

int rvn_pattern_match(const char* patterns, const char* name) {
	size_t name_length = strlen(name);
	const char* cursor = patterns;
	if (!patterns || !patterns[0]) return R_TRUE;
	while (*cursor) {
		const char* end = cursor;
		size_t length;
		while (*end && *end != ';' && *end != ',' && *end != ' ') end++;
		length = (size_t)(end - cursor);
		if (length == 1 && cursor[0] == '*') return R_TRUE;
		if (length == 3 && memcmp(cursor, "*.*", 3) == 0) return R_TRUE;
		if (length >= 2 && cursor[0] == '*') {
			/* "*.ext": the name ends with it */
			size_t suffix = length - 1;
			if (name_length >= suffix && rvn_strnicmp(name + name_length - suffix, cursor + 1, suffix) == 0) return R_TRUE;
		} else if (length > 0 && length == name_length && rvn_strnicmp(name, cursor, length) == 0) {
			return R_TRUE;
		}
		cursor = end;
		while (*cursor == ';' || *cursor == ',' || *cursor == ' ') cursor++;
	}
	return R_FALSE;
}

static void rvn_places_add(rvn_place_t** places, uint32_t* count, uint32_t* cap, const char* label, char* path) {
	uint32_t i;
	if (!path) return;
	for (i = 0; i < *count; i++) {
		if (strcmp((*places)[i].path, path) == 0) {
			rvn_free(path);
			return;
		}
	}
	RVN_ARRAY_RESERVE(*places, *cap, *count + 1);
	(*places)[*count].label = rvn_strdup(label);
	(*places)[*count].path = path;
	(*count)++;
}

/* A folder of the home that exists, or NULL. */
static char* rvn_places_home_folder(const char* home, const char* name) {
	char* path = rvn_path_child(home, name);
	if (rvn_path_is_dir(path)) return path;
	rvn_free(path);
	return NULL;
}

void rvn_fs_places_free(rvn_place_t* places, uint32_t count) {
	uint32_t i;
	for (i = 0; i < count; i++) {
		rvn_free(places[i].label);
		rvn_free(places[i].path);
	}
	rvn_free(places);
}

/* ---------------------------------------------------------------------------
 *  Win32
 * ------------------------------------------------------------------------- */

#if defined(_WIN32)

static WCHAR* rvn_fs_wide(const char* text) {
	int length = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
	WCHAR* wide;
	if (length <= 0) return NULL;
	wide = (WCHAR*)rvn_alloc(sizeof(WCHAR) * (size_t)length);
	MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, length);
	return wide;
}

static char* rvn_fs_narrow(const WCHAR* wide) {
	int length = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
	char* text;
	if (length <= 0) return NULL;
	text = (char*)rvn_alloc((size_t)length);
	WideCharToMultiByte(CP_UTF8, 0, wide, -1, text, length, NULL, NULL);
	return text;
}

/* 100 ns ticks since 1601 -> seconds since 1970 */
static int64_t rvn_fs_unix_time(FILETIME time) {
	uint64_t ticks = ((uint64_t)time.dwHighDateTime << 32) | (uint64_t)time.dwLowDateTime;
	return (int64_t)(ticks / 10000000u) - 11644473600ll;
}

int rvn_dir_read(const char* path, rvn_dir_entry_t** entries, uint32_t* count) {
	char* pattern = rvn_path_child(path, "*");
	WCHAR* wide = rvn_fs_wide(pattern);
	WIN32_FIND_DATAW data;
	HANDLE find;
	uint32_t cap = 0;

	*entries = NULL;
	*count = 0;
	rvn_free(pattern);
	if (!wide) return R_FALSE;
	find = FindFirstFileW(wide, &data);
	rvn_free(wide);
	if (find == INVALID_HANDLE_VALUE) return GetLastError() == ERROR_FILE_NOT_FOUND;
	do {
		rvn_dir_entry_t* entry;
		if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0) continue;
		RVN_ARRAY_RESERVE(*entries, cap, *count + 1);
		entry = &(*entries)[*count];
		entry->name = rvn_fs_narrow(data.cFileName);
		if (!entry->name) continue;
		entry->is_dir = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		entry->hidden = (data.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) != 0;
		entry->size = ((uint64_t)data.nFileSizeHigh << 32) | (uint64_t)data.nFileSizeLow;
		entry->modified = rvn_fs_unix_time(data.ftLastWriteTime);
		(*count)++;
	} while (FindNextFileW(find, &data));
	FindClose(find);
	return R_TRUE;
}

static DWORD rvn_fs_attributes(const char* path) {
	WCHAR* wide = rvn_fs_wide(path);
	DWORD attributes;
	if (!wide) return INVALID_FILE_ATTRIBUTES;
	attributes = GetFileAttributesW(wide);
	rvn_free(wide);
	return attributes;
}

int rvn_path_exists(const char* path) {
	return rvn_fs_attributes(path) != INVALID_FILE_ATTRIBUTES;
}

int rvn_file_stamp(const char* path, uint64_t* modified, uint64_t* size) {
	WIN32_FILE_ATTRIBUTE_DATA data;
	WCHAR* wide = rvn_fs_wide(path);
	BOOL ok;
	if (!wide) return R_FALSE;
	ok = GetFileAttributesExW(wide, GetFileExInfoStandard, &data);
	rvn_free(wide);
	if (!ok || (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) return R_FALSE;
	*modified = ((uint64_t)data.ftLastWriteTime.dwHighDateTime << 32) | (uint64_t)data.ftLastWriteTime.dwLowDateTime;
	*size = ((uint64_t)data.nFileSizeHigh << 32) | (uint64_t)data.nFileSizeLow;
	return R_TRUE;
}

int rvn_path_is_dir(const char* path) {
	DWORD attributes = rvn_fs_attributes(path);
	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

char* rvn_path_absolute(const char* path) {
	WCHAR* wide = rvn_fs_wide(path && path[0] ? path : ".");
	WCHAR full[2048];
	char* result;
	DWORD length;
	if (!wide) return NULL;
	length = GetFullPathNameW(wide, (DWORD)RVN_COUNT_OF(full), full, NULL);
	rvn_free(wide);
	if (length == 0 || length >= RVN_COUNT_OF(full)) return NULL;
	result = rvn_fs_narrow(full);
	if (result) rvn_path_clean(result);
	return result;
}

char* rvn_path_home(void) {
	WCHAR home[1024];
	DWORD length = GetEnvironmentVariableW(L"USERPROFILE", home, (DWORD)RVN_COUNT_OF(home));
	if (length == 0 || length >= RVN_COUNT_OF(home)) return rvn_strdup("C:\\");
	return rvn_fs_narrow(home);
}

char* rvn_env_get(const char* name) {
	WCHAR* wide_name = rvn_fs_wide(name);
	WCHAR* value;
	DWORD length;
	char* result;
	if (!wide_name) return NULL;
	length = GetEnvironmentVariableW(wide_name, NULL, 0);
	if (length == 0) {
		rvn_free(wide_name);
		return NULL;
	}
	value = (WCHAR*)rvn_alloc(sizeof(WCHAR) * (size_t)length);
	if (GetEnvironmentVariableW(wide_name, value, length) >= length) value[0] = 0;
	rvn_free(wide_name);
	result = value[0] ? rvn_fs_narrow(value) : NULL;
	rvn_free(value);
	return result;
}

int rvn_make_dir(const char* path) {
	WCHAR* wide = rvn_fs_wide(path);
	int ok;
	if (!wide) return R_FALSE;
	ok = CreateDirectoryW(wide, NULL) != 0;
	rvn_free(wide);
	return ok;
}

int rvn_write_file(const char* path, const void* data, size_t size) {
	WCHAR* wide = rvn_fs_wide(path);
	FILE* file;
	int ok;
	if (!wide) return R_FALSE;
	file = _wfopen(wide, L"wb");
	rvn_free(wide);
	if (!file) return R_FALSE;
	ok = size == 0 || fwrite(data, 1, size, file) == size;
	if (fclose(file) != 0) ok = R_FALSE;
	return ok;
}

int Ropen_url(const char* url) {
	WCHAR* wide;
	HINSTANCE result;
	if (!url || !url[0]) return R_FALSE;
	wide = rvn_fs_wide(url);
	if (!wide) return R_FALSE;
	result = ShellExecuteW(NULL, L"open", wide, NULL, NULL, SW_SHOWNORMAL);
	rvn_free(wide);
	return (INT_PTR)result > 32;
}

void rvn_fs_places(rvn_place_t** places, uint32_t* count) {
	char* home = rvn_path_home();
	uint32_t cap = 0;
	DWORD drives = GetLogicalDrives();
	int i;

	*places = NULL;
	*count = 0;
	rvn_places_add(places, count, &cap, "Home", rvn_strdup(home));
	rvn_places_add(places, count, &cap, "Desktop", rvn_places_home_folder(home, "Desktop"));
	rvn_places_add(places, count, &cap, "Documents", rvn_places_home_folder(home, "Documents"));
	rvn_places_add(places, count, &cap, "Downloads", rvn_places_home_folder(home, "Downloads"));
	rvn_places_add(places, count, &cap, "Pictures", rvn_places_home_folder(home, "Pictures"));
	for (i = 0; i < 26; i++) {
		char drive[4];
		if (!(drives & (1u << i))) continue;
		drive[0] = (char)('A' + i);
		drive[1] = ':';
		drive[2] = '\\';
		drive[3] = '\0';
		rvn_places_add(places, count, &cap, drive, rvn_strdup(drive));
	}
	rvn_free(home);
}

/* ---------------------------------------------------------------------------
 *  POSIX
 * ------------------------------------------------------------------------- */

#else

int rvn_dir_read(const char* path, rvn_dir_entry_t** entries, uint32_t* count) {
	DIR* dir = opendir(path);
	struct dirent* item;
	uint32_t cap = 0;

	*entries = NULL;
	*count = 0;
	if (!dir) return R_FALSE;
	while ((item = readdir(dir)) != NULL) {
		rvn_dir_entry_t* entry;
		struct stat info;
		char* full;
		if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0) continue;
		RVN_ARRAY_RESERVE(*entries, cap, *count + 1);
		entry = &(*entries)[*count];
		memset(entry, 0, sizeof(*entry));
		entry->name = rvn_strdup(item->d_name);
		entry->hidden = item->d_name[0] == '.';
		/* (stat follows links: a link to a folder is shown as a folder) */
		full = rvn_path_child(path, item->d_name);
		if (stat(full, &info) == 0) {
			entry->is_dir = S_ISDIR(info.st_mode);
			entry->size = (uint64_t)info.st_size;
			entry->modified = (int64_t)info.st_mtime;
		}
		rvn_free(full);
		(*count)++;
	}
	closedir(dir);
	return R_TRUE;
}

int rvn_path_exists(const char* path) {
	struct stat info;
	return stat(path, &info) == 0;
}

int rvn_file_stamp(const char* path, uint64_t* modified, uint64_t* size) {
	struct stat info;
	if (!path || stat(path, &info) != 0 || S_ISDIR(info.st_mode)) return R_FALSE;
#if defined(__APPLE__)
	*modified = (uint64_t)info.st_mtimespec.tv_sec * 1000000000ull + (uint64_t)info.st_mtimespec.tv_nsec;
#else
	*modified = (uint64_t)info.st_mtim.tv_sec * 1000000000ull + (uint64_t)info.st_mtim.tv_nsec;
#endif
	*size = (uint64_t)info.st_size;
	return R_TRUE;
}

int rvn_path_is_dir(const char* path) {
	struct stat info;
	return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

char* rvn_path_absolute(const char* path) {
	char* result;
	if (!path || !path[0]) path = ".";
	if (path[0] == '~' && (path[1] == '/' || path[1] == '\0')) {
		/* "~/x": the home of the user, as a shell would understand it */
		char* home = rvn_path_home();
		result = rvn_path_child(home, path[1] ? path + 2 : "");
		rvn_free(home);
	} else if (path[0] == '/') {
		result = rvn_strdup(path);
	} else {
		char current[4096];
		if (!getcwd(current, sizeof(current))) return NULL;
		result = rvn_path_child(current, path);
	}
	rvn_path_clean(result);
	return result;
}

char* rvn_path_home(void) {
	const char* home = getenv("HOME");
	return rvn_strdup(home && home[0] ? home : "/");
}

char* rvn_env_get(const char* name) {
	const char* value = name ? getenv(name) : NULL;
	return value && value[0] ? rvn_strdup(value) : NULL;
}

int rvn_make_dir(const char* path) {
	return mkdir(path, 0777) == 0;
}

int rvn_write_file(const char* path, const void* data, size_t size) {
	FILE* file = fopen(path, "wb");
	int ok;
	if (!file) return R_FALSE;
	ok = size == 0 || fwrite(data, 1, size, file) == size;
	if (fclose(file) != 0) ok = R_FALSE;
	return ok;
}

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
int Ropen_url(const char* url) {
	if (!url || !url[0]) return R_FALSE;
	return EM_ASM_INT({
		try {
			const url = new URL(UTF8ToString($0), location.href);
			if (!['https:', 'http:', 'mailto:'].includes(url.protocol)) return 0;
			window.open(url.href, '_blank', 'noopener,noreferrer'); return 1;
		} catch (_) { return 0; }
	}, url);
}
#elif defined(RVN_NO_SUBPROCESS)
/* iOS: UIApplication openURL: belongs to the platform layer; not available yet */
int Ropen_url(const char* url) {
	RVN_UNUSED(url);
	return R_FALSE;
}
#else
int Ropen_url(const char* url) {
	pid_t child;
	if (!url || !url[0]) return R_FALSE;
	child = fork();
	if (child < 0) return R_FALSE;
	if (child == 0) {
		/* a grandchild runs the opener, so that nothing is left to wait for */
		pid_t opener = fork();
		if (opener == 0) {
#if defined(__APPLE__)
			execlp("open", "open", url, (char*)NULL);
#else
			execlp("xdg-open", "xdg-open", url, (char*)NULL);
#endif
			_exit(127);
		}
		_exit(opener < 0 ? 1 : 0);
	}
	{
		int status = 0;
		waitpid(child, &status, 0);
		return WIFEXITED(status) && WEXITSTATUS(status) == 0;
	}
}
#endif

void rvn_fs_places(rvn_place_t** places, uint32_t* count) {
	char* home = rvn_path_home();
	uint32_t cap = 0;
	*places = NULL;
	*count = 0;
	rvn_places_add(places, count, &cap, "Home", rvn_strdup(home));
	rvn_places_add(places, count, &cap, "Desktop", rvn_places_home_folder(home, "Desktop"));
	rvn_places_add(places, count, &cap, "Documents", rvn_places_home_folder(home, "Documents"));
	rvn_places_add(places, count, &cap, "Downloads", rvn_places_home_folder(home, "Downloads"));
	rvn_places_add(places, count, &cap, "Pictures", rvn_places_home_folder(home, "Pictures"));
	rvn_places_add(places, count, &cap, "Computer", rvn_strdup("/"));
	rvn_free(home);
}

#endif


int rvn_is_http_url(const char* value) {
	if (!value) return R_FALSE;
	return (rvn_strnicmp(value, "https://", 8) == 0 || rvn_strnicmp(value, "http://", 7) == 0);
}

#define RVN_FS_DOWNLOAD_MAX (32u * 1024u * 1024u)

#if defined(_WIN32)
int rvn_fs_download_url(const char* url, uint8_t** data, size_t* size) {
	WCHAR* wide_url = NULL;
	URL_COMPONENTS parts;
	HINTERNET session = NULL, connection = NULL, request = NULL;
	WCHAR* host = NULL;
	WCHAR* object = NULL;
	DWORD status = 0, status_size = sizeof(status), read_count = 0;
	uint8_t* bytes = NULL;
	size_t used = 0, cap = 0;
	int ok = R_FALSE;
	if (data) *data = NULL;
	if (size) *size = 0;
	if (!url || !data || !size || !rvn_is_http_url(url)) return R_FALSE;
	wide_url = rvn_fs_wide(url);
	if (!wide_url) goto done;
	memset(&parts, 0, sizeof(parts));
	parts.dwStructSize = sizeof(parts);
	parts.dwSchemeLength = (DWORD)-1;
	parts.dwHostNameLength = (DWORD)-1;
	parts.dwUrlPathLength = (DWORD)-1;
	parts.dwExtraInfoLength = (DWORD)-1;
	if (!WinHttpCrackUrl(wide_url, 0, 0, &parts)) goto done;
	host = (WCHAR*)rvn_alloc(sizeof(WCHAR) * ((size_t)parts.dwHostNameLength + 1));
	memcpy(host, parts.lpszHostName, sizeof(WCHAR) * parts.dwHostNameLength);
	host[parts.dwHostNameLength] = L'\0';
	{
		size_t path_len = parts.dwUrlPathLength ? (size_t)parts.dwUrlPathLength : 1u;
		size_t object_len = path_len + (size_t)parts.dwExtraInfoLength;
		object = (WCHAR*)rvn_alloc(sizeof(WCHAR) * (object_len + 1));
		if (parts.dwUrlPathLength) memcpy(object, parts.lpszUrlPath, sizeof(WCHAR) * parts.dwUrlPathLength);
		else object[0] = L'/';
		if (parts.dwExtraInfoLength) memcpy(object + path_len, parts.lpszExtraInfo, sizeof(WCHAR) * parts.dwExtraInfoLength);
		object[object_len] = L'\0';
	}
	session = WinHttpOpen(L"Raven/0.1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!session) goto done;
	WinHttpSetTimeouts(session, 5000, 5000, 10000, 10000);
	connection = WinHttpConnect(session, host, parts.nPort, 0);
	if (!connection) goto done;
	request = WinHttpOpenRequest(connection, L"GET", object, NULL, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES, parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
	if (!request) goto done;
	if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
		!WinHttpReceiveResponse(request, NULL)) goto done;
	if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX) || status < 200 || status >= 300) goto done;
	for (;;) {
		uint8_t chunk[8192];
		if (!WinHttpReadData(request, chunk, (DWORD)sizeof(chunk), &read_count)) goto done;
		if (!read_count) break;
		if (used + (size_t)read_count > RVN_FS_DOWNLOAD_MAX) goto done;
		if (used + (size_t)read_count > cap) {
			size_t next = cap ? cap * 2 : 16384;
			while (next < used + (size_t)read_count) next *= 2;
			if (next > RVN_FS_DOWNLOAD_MAX) next = RVN_FS_DOWNLOAD_MAX;
			bytes = (uint8_t*)rvn_realloc(bytes, next);
			cap = next;
		}
		memcpy(bytes + used, chunk, read_count);
		used += read_count;
	}
	if (used == 0) goto done;
	*data = bytes;
	*size = used;
	bytes = NULL;
	ok = R_TRUE;
done:
	if (request) WinHttpCloseHandle(request);
	if (connection) WinHttpCloseHandle(connection);
	if (session) WinHttpCloseHandle(session);
	rvn_free(wide_url); rvn_free(host); rvn_free(object); rvn_free(bytes);
	return ok;
}
#elif defined(RVN_NO_SUBPROCESS) || defined(__EMSCRIPTEN__)
/*
 * No curl to start (iOS, a browser), and a blocking download would freeze the only thread of a browser: URLs are
 * not available here yet. The web build will fetch asynchronously (emscripten_fetch / the fetch() of the
 * browser) and iOS will use NSURLSession, handing the bytes over like the other platforms do
 * (docs/SOKOL_BACKEND_PLAN.md, 3.8).
 */
int rvn_fs_download_url(const char* url, uint8_t** data, size_t* size) {
	RVN_UNUSED(url);
	if (data) *data = NULL;
	if (size) *size = 0;
	return R_FALSE;
}
#else
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int rvn_fs_download_url(const char* url, uint8_t** data, size_t* size) {
	int pipes[2] = {-1, -1};
	pid_t child;
	uint8_t* bytes = NULL;
	size_t used = 0, cap = 0;
	int status = 0, ok = R_FALSE, overflow = R_FALSE;
	if (data) *data = NULL;
	if (size) *size = 0;
	if (!url || !data || !size || !rvn_is_http_url(url)) return R_FALSE;
	if (pipe(pipes) != 0) return R_FALSE;
	child = fork();
	if (child < 0) { close(pipes[0]); close(pipes[1]); return R_FALSE; }
	if (child == 0) {
		int null_fd;
		close(pipes[0]);
		if (dup2(pipes[1], STDOUT_FILENO) < 0) _exit(126);
		close(pipes[1]);
		null_fd = open("/dev/null", O_WRONLY);
		if (null_fd >= 0) { dup2(null_fd, STDERR_FILENO); close(null_fd); }
		execlp("curl", "curl", "--fail", "--silent", "--show-error", "--location",
			"--max-redirs", "5", "--max-time", "20", "--max-filesize", "33554432",
			"--proto", "=http,https", "--proto-redir", "=http,https", "--output", "-", "--", url, (char*)NULL);
		_exit(127);
	}
	close(pipes[1]);
	for (;;) {
		uint8_t chunk[8192];
		ssize_t n = read(pipes[0], chunk, sizeof(chunk));
		if (n < 0) { if (errno == EINTR) continue; break; }
		if (n == 0) break;
		if (used + (size_t)n > RVN_FS_DOWNLOAD_MAX) { overflow = R_TRUE; kill(child, SIGKILL); break; }
		if (used + (size_t)n > cap) {
			size_t next = cap ? cap * 2 : 16384;
			while (next < used + (size_t)n) next *= 2;
			if (next > RVN_FS_DOWNLOAD_MAX) next = RVN_FS_DOWNLOAD_MAX;
			bytes = (uint8_t*)rvn_realloc(bytes, next);
			cap = next;
		}
		memcpy(bytes + used, chunk, (size_t)n);
		used += (size_t)n;
	}
	close(pipes[0]);
	while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
	if (!overflow && WIFEXITED(status) && WEXITSTATUS(status) == 0 && used > 0) {
		*data = bytes; *size = used; bytes = NULL; ok = R_TRUE;
	}
	rvn_free(bytes);
	return ok;
}
#endif
