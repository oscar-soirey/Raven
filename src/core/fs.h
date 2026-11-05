/**
 * Raven - core/fs.h
 * The file system as the file browser and the dialogs need it: folders,
 * paths, the usual places of the user. Paths are UTF-8 on every platform.
 */

#ifndef RVN_FS_H
#define RVN_FS_H

#include "base.h"

#if defined(_WIN32)
#define RVN_PATH_SEPARATOR '\\'
/** Between the folders of a list such as the PATH variable. */
#define RVN_PATH_LIST_SEPARATOR ';'
#else
#define RVN_PATH_SEPARATOR '/'
#define RVN_PATH_LIST_SEPARATOR ':'
#endif

typedef struct rvn_dir_entry_t {
	char* name;
	int is_dir;
	/** Hidden by the conventions of the OS (dot files, the hidden attribute). */
	int hidden;
	uint64_t size;
	/** Last modification, in seconds since 1970 (UTC). */
	int64_t modified;
} rvn_dir_entry_t;

/** Lists a folder, without "." and "..", in no particular order. R_FALSE when it cannot be read. */
int  rvn_dir_read(const char* path, rvn_dir_entry_t** entries, uint32_t* count);
void rvn_dir_free(rvn_dir_entry_t* entries, uint32_t count);

int  rvn_path_exists(const char* path);
int  rvn_path_is_dir(const char* path);
RVN_INLINE int rvn_path_is_separator(char c) {
#if defined(_WIN32)
	return c == '\\' || c == '/';
#else
	return c == '/';
#endif
}
/** Absolute path without "." or "..", without a trailing separator except for a root. Free with rvn_free. */
char* rvn_path_absolute(const char* path);
/** Folder containing a path (given absolute), or NULL for a root. Free with rvn_free. */
char* rvn_path_parent(const char* path);
/** A name inside a folder. Free with rvn_free. */
char* rvn_path_child(const char* folder, const char* name);
/** Last component of a path: points inside `path`. */
const char* rvn_path_name(const char* path);
/** Home folder of the user. Free with rvn_free. */
char* rvn_path_home(void);
/** Value of an environment variable in UTF-8 (also on Windows), or NULL when it is not set or empty. Free with rvn_free. */
char* rvn_env_get(const char* name);
/**
 * When a file was last written (finer than a second where the file system
 * allows) and its size: both change when the file does. R_FALSE when it
 * cannot be read.
 */
int  rvn_file_stamp(const char* path, uint64_t* modified, uint64_t* size);
int  rvn_make_dir(const char* path);
int  rvn_write_file(const char* path, const void* data, size_t size);
/** True when the address is an HTTP or HTTPS URL (case-insensitive scheme). */
int  rvn_is_http_url(const char* value);
/** Downloads a URL into an rvn_alloc'ed buffer. Limited to 32 MiB and 20 seconds. */
int  rvn_fs_download_url(const char* url, uint8_t** data, size_t* size);

/** A folder the user goes to often: home, desktop, documents, a drive... */
typedef struct rvn_place_t {
	char* label;
	char* path;
} rvn_place_t;

void rvn_fs_places(rvn_place_t** places, uint32_t* count);
void rvn_fs_places_free(rvn_place_t* places, uint32_t count);

/** Does a file name match one of the patterns ("*.xml;*.ui", "*")? Case is ignored. An empty list matches all. */
int rvn_pattern_match(const char* patterns, const char* name);

#endif /* RVN_FS_H */
