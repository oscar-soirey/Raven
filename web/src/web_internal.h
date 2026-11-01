/**
 * Raven Web - web_internal.h
 * What the files of the library share: the little that depends on the
 * system (a child process with two pipes, a thread, a lock, folders), JSON,
 * and the connection to the browser.
 *
 * Prefix: rw_.
 */

#ifndef RAVEN_WEB_INTERNAL_H
#define RAVEN_WEB_INTERNAL_H

#include "../include/raven_web.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 *  System (web_posix.c, web_win32.c)
 * ------------------------------------------------------------------------- */

typedef struct rw_process_t rw_process_t;

/**
 * Starts a program with the two pipes of the DevTools protocol: the child
 * reads what rw_process_write sends, rw_process_read receives what it
 * writes. `arguments` ends with NULL and starts with the program. On Unix
 * the child finds the pipes as descriptors 3 and 4; on Windows it is told
 * their handles by an argument added here. NULL when it cannot be started.
 */
rw_process_t* rw_process_start(const char* const* arguments);
/** Sends all the bytes. R_FALSE when the child is gone. */
int rw_process_write(rw_process_t* process, const void* data, size_t size);
/** Waits for bytes. Returns how many were read; 0 when the child closed its side or rw_process_hang_up was called. */
int rw_process_read(rw_process_t* process, void* buffer, size_t size);
/** Makes a rw_process_read that is waiting (in another thread) return 0, and the child see the end of its input. */
void rw_process_hang_up(rw_process_t* process);
/** Waits a little for the child to leave by itself, kills it otherwise, and releases everything. */
void rw_process_stop(rw_process_t* process);
/**
 * Waits up to `wait_ms` for the child to end, and gives how it ended: its
 * exit code (a negative signal number on Unix). R_FALSE when it still runs.
 */
int rw_process_exit_code(rw_process_t* process, int wait_ms, long* code);

typedef struct rw_thread_t rw_thread_t;
rw_thread_t* rw_thread_start(void (*function)(void*), void* argument);
void rw_thread_join(rw_thread_t* thread);

typedef struct rw_mutex_t rw_mutex_t;
rw_mutex_t* rw_mutex_create(void);
void rw_mutex_destroy(rw_mutex_t* mutex);
void rw_mutex_lock(rw_mutex_t* mutex);
void rw_mutex_unlock(rw_mutex_t* mutex);

/** A browser of the Chromium family found on this computer (to release with rw_free), or NULL. */
char* rw_find_browser(void);
/** R_TRUE when the file exists and can be run. */
int rw_is_program(const char* path);
/** Makes a new, empty folder for a temporary profile and returns its path (rw_free), or NULL. */
char* rw_temp_folder(void);
/** Deletes a folder and all it contains. */
void rw_remove_folder(const char* path);
/** The "file://" address of a file (rw_free). */
char* rw_file_url(const char* path);
/** R_TRUE when the browser must be told to run without its sandbox (root on Unix: it refuses to start otherwise). */
int rw_needs_no_sandbox(void);

/* ---------------------------------------------------------------------------
 *  Memory and text (web_json.c)
 * ------------------------------------------------------------------------- */

void* rw_alloc(size_t size);
void* rw_realloc(void* pointer, size_t size);
void rw_free(void* pointer);
char* rw_strdup(const char* text);

/** A text that grows. */
typedef struct rw_text_t {
	char* data;
	size_t length, capacity;
} rw_text_t;

void rw_text_add(rw_text_t* text, const char* data, size_t size);
void rw_text_puts(rw_text_t* text, const char* string);
void rw_text_printf(rw_text_t* text, const char* format, ...);
/** Appends a string as JSON writes it, quotes included. */
void rw_text_json(rw_text_t* text, const char* string);
void rw_text_free(rw_text_t* text);

/* ---------------------------------------------------------------------------
 *  JSON (web_json.c)
 * ------------------------------------------------------------------------- */

typedef enum rw_json_type_t {
	RW_JSON_NULL = 0,
	RW_JSON_BOOL,
	RW_JSON_NUMBER,
	RW_JSON_STRING,
	RW_JSON_ARRAY,
	RW_JSON_OBJECT
} rw_json_type_t;

typedef struct rw_json_t {
	rw_json_type_t type;
	/** Name of the member, inside an object. */
	const char* key;
	double number;
	/** Text of a string (it stays in the buffer that was parsed), and its length. */
	const char* string;
	size_t length;
	struct rw_json_t* first;
	struct rw_json_t* next;
} rw_json_t;

typedef struct rw_json_document_t {
	rw_json_t* root;
	/** Blocks of values, released together. */
	void* blocks;
} rw_json_document_t;

/**
 * Parses a text IN PLACE: the strings of the values point into `text`, which
 * is modified (escapes resolved, ends marked) and must outlive the document.
 * R_FALSE when the text is not JSON.
 */
int rw_json_parse(rw_json_document_t* document, char* text, size_t size);
void rw_json_free(rw_json_document_t* document);
/** The member of an object reached by a path such as "params.frame.url"; NULL when it is not there. */
const rw_json_t* rw_json_get(const rw_json_t* value, const char* path);
const char* rw_json_string(const rw_json_t* value, const char* path, const char* fallback);
double rw_json_number(const rw_json_t* value, const char* path, double fallback);
int rw_json_bool(const rw_json_t* value, const char* path, int fallback);
/** Writes a value back as JSON text. */
void rw_json_write(rw_text_t* text, const rw_json_t* value);

/** Decodes base64 into a new buffer (rw_free). NULL when the text is not base64. */
uint8_t* rw_base64_decode(const char* text, size_t length, size_t* size);

/* ---------------------------------------------------------------------------
 *  The browser (web_browser.c)
 *
 *  One process, one thread that reads what it says. What it says is handed
 *  to the UI thread (Rtask_post), which is the only one to touch the views.
 * ------------------------------------------------------------------------- */

/** What the reading thread hands to the UI thread. */
typedef struct rw_incoming_t {
	struct rw_incoming_t* next;
	/** A message of the browser, as text; or NULL for a picture. */
	char* message;
	size_t size;
	/** A picture of a page, decoded: the session it is for, its pixels. */
	char* session;
	uint8_t* pixels;
	int width, height;
	/** The browser closed the pipe: nothing will come any more. */
	int closed;
} rw_incoming_t;

/** Starts the browser if it is not running. R_FALSE (and rw_browser_error tells why) when it cannot. */
int rw_browser_start(void);
void rw_browser_stop(void);
int rw_browser_running(void);
const char* rw_browser_error(void);
/** The browser closed its pipe by itself: "The browser stopped.", with how it ended when it is known. */
const char* rw_browser_stop_reason(void);
/**
 * Sends a command: `params` is the JSON object of its parameters (or NULL),
 * `session` the page it is for (or NULL for the browser itself). Returns
 * the number that the answer will carry, or 0 when nothing could be sent.
 */
int rw_browser_send(const char* session, const char* method, const char* params);
/** Called on the UI thread for each thing the browser said (web_view.c). */
void rw_views_incoming(rw_incoming_t* incoming);

/* settings (web_browser.c) */
extern char* g_rw_browser_path;
extern char* g_rw_profile_folder;
extern char* g_rw_arguments;

#endif /* RAVEN_WEB_INTERNAL_H */
