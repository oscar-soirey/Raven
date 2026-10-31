/**
 * Raven - core/file.c
 * Whole-file reading with UTF-8 paths on every platform.
 */

#include "base.h"
#include "thread.h"
#include "context.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

static FILE* rvn_fopen_read(const char* path) {
	wchar_t wide[1024];
	if (MultiByteToWideChar(CP_UTF8, 0, path, -1, wide, (int)RVN_COUNT_OF(wide)) <= 0) return NULL;
	return _wfopen(wide, L"rb");
}
#else
#include <sys/stat.h>

static FILE* rvn_fopen_read(const char* path) {
	struct stat info;
	FILE* file = fopen(path, "rb");
	/* (a folder opens too, and then says it is LONG_MAX bytes long: only regular files are read) */
	if (file && (fstat(fileno(file), &info) != 0 || !S_ISREG(info.st_mode))) {
		fclose(file);
		return NULL;
	}
	return file;
}
#endif

/* ---------------------------------------------------------------------------
 *  Files held in memory (Rfile_register_memory): resources embedded in the
 *  executable. Every file Raven reads is looked up here first.
 * ------------------------------------------------------------------------- */

typedef struct rvn_memory_file_t {
	char* path;
	const char* data;
	size_t size;
	int owned;
} rvn_memory_file_t;

static rvn_memory_file_t* g_memory_files;
static uint32_t g_memory_file_count;
static uint32_t g_memory_file_cap;
static rvn_mutex_t g_memory_files_lock;
static int g_memory_files_lock_ready;

/*
 * A path in a canonical spelling: '/' only, no "." segment, ".." folded into
 * the folder before it. "ui/themes/../fonts/a.ttf" -> "ui/fonts/a.ttf".
 */
static void rvn_memory_path_normalize(const char* path, char* out, size_t cap) {
	size_t length = 0;
	const char* segment = path;
	if (cap == 0) return;
	while (*segment) {
		const char* end = segment;
		size_t size;
		while (*end && *end != '/' && *end != '\\') end++;
		size = (size_t)(end - segment);
		if (size == 0 && segment == path) {
			/* a leading separator is kept: an absolute path */
			if (length + 1 < cap) out[length++] = '/';
		} else if (size == 1 && segment[0] == '.') {
			/* nothing */
		} else if (size == 2 && segment[0] == '.' && segment[1] == '.' && length > 0 &&
			!(length >= 2 && out[length - 1] == '.' && out[length - 2] == '.' && (length == 2 || out[length - 3] == '/'))) {
			/* back to the previous separator */
			while (length > 0 && out[length - 1] != '/') length--;
			if (length > 0 && !(length == 1 && out[0] == '/')) length--;
		} else if (size > 0) {
			if (length > 0 && out[length - 1] != '/' && length + 1 < cap) out[length++] = '/';
			if (length + size >= cap) size = cap - length - 1;
			memcpy(out + length, segment, size);
			length += size;
		}
		segment = *end ? end + 1 : end;
	}
	out[length] = '\0';
}

static int rvn_memory_path_equal(const char* a, const char* b) {
	char na[1024], nb[1024];
	rvn_memory_path_normalize(a, na, sizeof(na));
	rvn_memory_path_normalize(b, nb, sizeof(nb));
	return strcmp(na, nb) == 0;
}

static void rvn_memory_files_lock(void) {
	if (!g_memory_files_lock_ready) {
		rvn_mutex_init(&g_memory_files_lock);
		g_memory_files_lock_ready = R_TRUE;
	}
	rvn_mutex_lock(&g_memory_files_lock);
}

static int rvn_memory_file_find(const char* path) {
	uint32_t i;
	for (i = 0; i < g_memory_file_count; i++) {
		if (rvn_memory_path_equal(g_memory_files[i].path, path)) return (int)i;
	}
	return -1;
}

int rvn_memory_file_exists(const char* path) {
	int found;
	if (!path || !g_memory_file_count) return R_FALSE;
	rvn_memory_files_lock();
	found = rvn_memory_file_find(path) >= 0;
	rvn_mutex_unlock(&g_memory_files_lock);
	return found;
}

int Rfile_register_memory(const char* path, const void* data, size_t size, int copy) {
	int index;
	rvn_memory_file_t* file;
	if (!rvn_api_check("Rfile_register_memory")) return R_FALSE;
	if (!path || !path[0] || (!data && size)) return R_FALSE;
	rvn_memory_files_lock();
	index = rvn_memory_file_find(path);
	if (index >= 0) {
		file = &g_memory_files[index];
		if (file->owned) rvn_free((void*)file->data);
		rvn_free(file->path);
	} else {
		RVN_ARRAY_RESERVE(g_memory_files, g_memory_file_cap, g_memory_file_count + 1);
		file = &g_memory_files[g_memory_file_count++];
	}
	file->path = rvn_strdup(path);
	file->size = size;
	file->owned = copy ? R_TRUE : R_FALSE;
	if (copy) {
		char* bytes = (char*)rvn_alloc(size + 1);
		if (size) memcpy(bytes, data, size);
		bytes[size] = 0;
		file->data = bytes;
	} else {
		file->data = (const char*)data;
	}
	rvn_mutex_unlock(&g_memory_files_lock);
	return R_TRUE;
}

void Rfile_unregister_memory(const char* path) {
	int index;
	if (!path) return;
	rvn_memory_files_lock();
	index = rvn_memory_file_find(path);
	if (index >= 0) {
		rvn_memory_file_t* file = &g_memory_files[index];
		if (file->owned) rvn_free((void*)file->data);
		rvn_free(file->path);
		g_memory_files[index] = g_memory_files[--g_memory_file_count];
	}
	rvn_mutex_unlock(&g_memory_files_lock);
}

void rvn_memory_files_shutdown(void) {
	uint32_t i;
	if (!g_memory_files_lock_ready) return;
	for (i = 0; i < g_memory_file_count; i++) {
		if (g_memory_files[i].owned) rvn_free((void*)g_memory_files[i].data);
		rvn_free(g_memory_files[i].path);
	}
	rvn_free(g_memory_files);
	g_memory_files = NULL;
	g_memory_file_count = g_memory_file_cap = 0;
	rvn_mutex_destroy(&g_memory_files_lock);
	g_memory_files_lock_ready = R_FALSE;
}

int Rfile_is_memory(const char* path) {
	return rvn_memory_file_exists(path);
}

char* rvn_read_file(const char* path, size_t* size) {
	FILE* file;
	long length;
	char* data;
	size_t read;

	if (size) *size = 0;
	if (!path) return NULL;
	if (g_memory_file_count) {
		int index;
		rvn_memory_files_lock();
		index = rvn_memory_file_find(path);
		if (index >= 0) {
			const rvn_memory_file_t* memory = &g_memory_files[index];
			data = (char*)rvn_alloc(memory->size + 1);
			if (memory->size) memcpy(data, memory->data, memory->size);
			data[memory->size] = '\0';
			if (size) *size = memory->size;
			rvn_mutex_unlock(&g_memory_files_lock);
			return data;
		}
		rvn_mutex_unlock(&g_memory_files_lock);
	}
	file = rvn_fopen_read(path);
	if (!file) return NULL;
	if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 || (unsigned long)length >= (unsigned long)(SIZE_MAX / 2) ||
		fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		return NULL;
	}
	data = (char*)rvn_alloc((size_t)length + 1);
	read = fread(data, 1, (size_t)length, file);
	fclose(file);
	if (read != (size_t)length) {
		rvn_free(data);
		return NULL;
	}
	data[length] = '\0';
	if (size) *size = (size_t)length;
	return data;
}
