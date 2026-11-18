/**
 * Raven - core/file.c
 * Whole-file reading with UTF-8 paths on every platform: from memory, from
 * the disk, then from the application's package.
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
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

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

/* ---------------------------------------------------------------------------
 *  The application's package (rvn_file_set_provider): a relative path that
 *  names nothing in memory nor on disk is read from it.
 * ------------------------------------------------------------------------- */

/* (set by the platform at init, before any other thread reads a file) */
static const rvn_file_provider_t* g_file_provider;

void rvn_file_set_provider(const rvn_file_provider_t* provider) {
	g_file_provider = provider;
}

int rvn_file_has_provider(void) {
	return g_file_provider != NULL;
}

/* The name of `path` in the package: relative and normalized. R_FALSE when it cannot be in it. */
static int rvn_package_name(const char* path, char* out, size_t cap) {
	if (!path || !path[0] || strlen(path) >= cap) return R_FALSE;
	/* an absolute path, "~/...", a drive ("C:...") or a URL names a file elsewhere */
	if (path[0] == '/' || path[0] == '\\' || (path[0] == '~' && (path[1] == '/' || !path[1])) || strchr(path, ':')) return R_FALSE;
	rvn_memory_path_normalize(path, out, cap);
	if (!out[0] || (out[0] == '.' && out[1] == '.' && (out[2] == '/' || !out[2]))) return R_FALSE;
	return R_TRUE;
}

static int rvn_package_file_exists(const char* path) {
	const rvn_file_provider_t* provider = g_file_provider;
	char name[1024];
	void* file;
	if (!provider || !rvn_package_name(path, name, sizeof(name))) return R_FALSE;
	file = provider->open(name);
	if (!file) return R_FALSE;
	provider->close(file);
	return R_TRUE;
}

static char* rvn_package_read(const char* path, size_t* size) {
	const rvn_file_provider_t* provider = g_file_provider;
	char name[1024];
	void* file;
	size_t length, done = 0;
	char* data;
	if (!provider || !rvn_package_name(path, name, sizeof(name))) return NULL;
	file = provider->open(name);
	if (!file) return NULL;
	length = provider->size(file);
	if (length >= SIZE_MAX / 2) {
		provider->close(file);
		return NULL;
	}
	data = (char*)rvn_alloc(length + 1);
	while (done < length) {
		size_t read = provider->read(file, data + done, length - done);
		if (read == 0) break;
		done += read;
	}
	provider->close(file);
	if (done != length) {
		rvn_free(data);
		return NULL;
	}
	data[length] = '\0';
	if (size) *size = length;
	return data;
}

int rvn_file_is_virtual(const char* path) {
	FILE* file;
	if (!path) return R_FALSE;
	if (rvn_memory_file_exists(path)) return R_TRUE;
	if (!rvn_package_file_exists(path)) return R_FALSE;
	/* (the disk comes before the package) */
	file = rvn_fopen_read(path);
	if (!file) return R_TRUE;
	fclose(file);
	return R_FALSE;
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
	if (!file) return rvn_package_read(path, size);
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

#if defined(_WIN32)

/* (the fonts of Windows are a few MB: read whole) */
uint8_t* rvn_map_file(const char* path, size_t* size) {
	return (uint8_t*)rvn_read_file(path, size);
}

void rvn_unmap_file(uint8_t* data, size_t size) {
	RVN_UNUSED(size);
	rvn_free(data);
}

#else

uint8_t* rvn_map_file(const char* path, size_t* size) {
	struct stat info;
	void* data;
	int file;
	*size = 0;
	if (!path) return NULL;
	file = open(path, O_RDONLY);
	if (file < 0) return NULL;
	if (fstat(file, &info) != 0 || !S_ISREG(info.st_mode) || info.st_size <= 0 || (uint64_t)info.st_size >= (uint64_t)(SIZE_MAX / 2)) {
		close(file);
		return NULL;
	}
	data = mmap(NULL, (size_t)info.st_size, PROT_READ, MAP_PRIVATE, file, 0);
	close(file);
	if (data == MAP_FAILED) return NULL;
	*size = (size_t)info.st_size;
	return (uint8_t*)data;
}

void rvn_unmap_file(uint8_t* data, size_t size) {
	if (data) munmap(data, size);
}

#endif
