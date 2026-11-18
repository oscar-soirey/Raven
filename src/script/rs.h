/**
 * Raven - script/rs.h
 * Raven Script: the core of the language (lexer, parser, type checker,
 * bytecode compiler, virtual machine, diagnostics, formatter, analysis).
 *
 * The core knows nothing of widgets or windows: what it needs from the
 * application (native functions, widget names, where diagnostics go) comes
 * through an rs_host. The same files are compiled into the library (with
 * script/script_raven.c as the host), into the command line tool and, as
 * WebAssembly, into the language server of the VS Code extension, so the
 * editor and the program always agree about what a script means.
 *
 * Everything here runs on one thread (the UI thread in the library).
 */

#ifndef RS_H
#define RS_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

/* ---------------------------------------------------------------------------
 *  Memory and text helpers
 * ------------------------------------------------------------------------- */

/** Every allocation of the core goes through these (rs_set_allocator to route them elsewhere). */
void* rs_malloc(size_t size);
void* rs_calloc(size_t count, size_t size);
void* rs_realloc(void* pointer, size_t size);
void  rs_free(void* pointer);
/** realloc_fn(NULL, n) allocates, realloc_fn(p, 0) is never called: free_fn frees. NULL restores malloc / free. */
void  rs_set_allocator(void* (*realloc_fn)(void*, size_t), void (*free_fn)(void*));
char* rs_strdup(const char* text);
char* rs_strndup(const char* text, size_t length);
int   rs_stricmp(const char* a, const char* b);

/** A text that grows; `data` is always null-terminated once something was written. */
typedef struct rs_buf {
	char* data;
	size_t length;
	size_t capacity;
} rs_buf;

void rs_buf_put(rs_buf* buffer, const char* bytes, size_t count);
void rs_buf_puts(rs_buf* buffer, const char* text);
void rs_buf_putc(rs_buf* buffer, char c);
void rs_buf_printf(rs_buf* buffer, const char* format, ...);
void rs_buf_vprintf(rs_buf* buffer, const char* format, va_list arguments);
/** A JSON string literal (with the quotes). */
void rs_buf_json_string(rs_buf* buffer, const char* text);
/** Takes the text (the caller frees it with rs_free) and empties the buffer. */
char* rs_buf_take(rs_buf* buffer);
void rs_buf_free(rs_buf* buffer);

/** Number of code points of UTF-8 text (invalid bytes count as one each). */
size_t rs_utf8_length(const char* text, size_t bytes);

/* ---------------------------------------------------------------------------
 *  Types
 * ------------------------------------------------------------------------- */

typedef enum rs_base {
	/** The type of an expression that already produced an error: compatible with everything, so one mistake gives one message. */
	RS_T_ERROR = 0,
	RS_T_VOID,
	RS_T_NULL,
	RS_T_BOOL,
	RS_T_NUMBER,
	RS_T_STRING
} rs_base;

/** Deepest array type (number[][][][]...). */
#define RS_MAX_ARRAY_DEPTH 8

/** A type: a base and how many [] follow it (0: not an array). */
typedef struct rs_type {
	uint8_t base;
	uint8_t depth;
} rs_type;

rs_type rs_type_make(rs_base base, int depth);
int  rs_type_equal(rs_type a, rs_type b);
/** a value of type `value` can be stored where `target` is expected (equal, or one of them is the error type). */
int  rs_type_assignable(rs_type target, rs_type value);
int  rs_type_is_error(rs_type type);
int  rs_type_is_array(rs_type type);
/** The type of the elements of an array type. */
rs_type rs_type_element(rs_type type);
/** "number", "string[]"... into a static buffer (one of four, rotating). */
const char* rs_type_name(rs_type type);
/** Parses "number", "bool[][]"...: R_FALSE (0) when it is not a type. */
int  rs_type_parse(const char* text, size_t length, rs_type* type);

/** Most parameters a function can have. */
#define RS_MAX_PARAMS 16

/** The signature of a function: what the type checker and the event dispatcher compare. */
typedef struct rs_sig {
	rs_type ret;
	int count;
	rs_type params[RS_MAX_PARAMS];
	/** Names of the parameters, when known (may be NULL each); not owned. */
	const char* names[RS_MAX_PARAMS];
} rs_sig;

/** "(value: number, text: string): void" into `out`. */
void rs_sig_format(const rs_sig* sig, rs_buf* out);
/**
 * Parses the signature of a native function: "(string, number): bool",
 * "(message: string)", "()" (a missing return type is void). Arrays are
 * refused (natives take and return numbers, strings and booleans).
 * Returns 0 and writes the reason in `error` (may be NULL) when invalid.
 * The names point into `names_storage` (a copy kept by the caller).
 */
int rs_sig_parse(const char* text, rs_sig* sig, char* names_storage, size_t storage_size, char* error, size_t error_size);

/* ---------------------------------------------------------------------------
 *  Values (what the virtual machine works on)
 * ------------------------------------------------------------------------- */

typedef enum rs_vtype {
	RS_V_VOID = 0,
	RS_V_NULL,
	RS_V_BOOL,
	RS_V_NUMBER,
	RS_V_STRING,
	RS_V_ARRAY
} rs_vtype;

typedef struct rs_string rs_string;
typedef struct rs_array rs_array;

typedef struct rs_value {
	uint8_t type;
	union {
		int boolean;
		double number;
		rs_string* string;
		rs_array* array;
	} as;
} rs_value;

struct rs_env;

struct rs_string {
	uint32_t refs;
	uint32_t length; /* bytes, without the terminating 0 */
	/** The environment whose memory it counts against (NULL: not counted). */
	struct rs_env* owner;
	char text[1];
};

struct rs_array {
	uint32_t refs;
	struct rs_env* owner;
	rs_type element;
	uint32_t count;
	uint32_t capacity;
	rs_value* items;
};

rs_value rs_value_void(void);
rs_value rs_value_null(void);
rs_value rs_value_bool(int value);
rs_value rs_value_number(double value);
/** A new string value holding a copy of the text (NULL when out of memory: check the type). */
rs_value rs_value_string(const char* text, size_t length);
rs_value rs_value_cstring(const char* text);
void rs_value_retain(rs_value value);
void rs_value_release(rs_value value);
/** The text of a string value, or "" for another type. */
const char* rs_value_text(rs_value value);
/** The type a value has (arrays: their element type plus one). */
rs_type rs_value_static_type(rs_value value);

/* ---------------------------------------------------------------------------
 *  Diagnostics
 * ------------------------------------------------------------------------- */

typedef enum rs_severity {
	RS_SEVERITY_ERROR = 0,
	RS_SEVERITY_WARNING,
	RS_SEVERITY_INFO
} rs_severity;

/** A place in a file: 1-based lines and columns (columns count code points). */
typedef struct rs_location {
	const char* file;
	int line;
	int column;
	int end_line;
	int end_column;
	/** The text of the line, for the excerpt (NULL when unknown). */
	const char* line_text;
	/** Byte offsets in the unit (-1 when the place is not in a unit), for tools. */
	int unit;
	int start;
	int end;
} rs_location;

typedef struct rs_diag {
	rs_severity severity;
	/** "RS102" */
	const char* code;
	/** "Type mismatch." */
	const char* title;
	/** The explanation; several lines are allowed. */
	const char* message;
	/** A suggestion ("Help: ..."), or NULL. */
	const char* help;
	int has_location;
	rs_location location;
	/** A second place (the declaration of what is wrong), with its note. */
	int has_related;
	rs_location related;
	const char* related_note;
	/** R_TRUE for an error found while running (not while compiling). */
	int runtime;
} rs_diag;

/** The diagnostic as the command line and the error callback show it (rustc-like, see the specification). */
void rs_diag_format(const rs_diag* diag, rs_buf* out);

/* ---------------------------------------------------------------------------
 *  Events of the widgets (what each widget gives the function of its action)
 * ------------------------------------------------------------------------- */

typedef struct rs_event_sig {
	/** The tag of the widget in XML ("Slider"); aliases are listed after it, separated by '|'. */
	const char* tags;
	/** The attribute that names the action in the documentation (all of Action, OnClicked, OnChanged, OnCommitted, OnActivated work). */
	const char* attribute;
	/** What the event is, for the messages: "Slider.OnChanged". */
	const char* event;
	int count;
	rs_base types[4];
	const char* names[4];
	/** When the action fires. */
	const char* description;
} rs_event_sig;

/** The event of a widget tag (case ignored, aliases accepted); NULL when the widget gives no argument (a Button). */
const rs_event_sig* rs_event_find(const char* tag);
int rs_event_count(void);
const rs_event_sig* rs_event_at(int index);
/** The attributes that name the action of a widget in XML. */
int rs_is_action_attribute(const char* name);

typedef enum rs_handler_result {
	RS_HANDLER_OK = 0,
	/** The function requires more arguments than the event gives. */
	RS_HANDLER_TOO_MANY_PARAMETERS,
	/** A parameter does not have the type of the argument the event gives. */
	RS_HANDLER_WRONG_TYPE
} rs_handler_result;

/**
 * The rule of the specification (12.4): a handler may ignore arguments the
 * event gives, but cannot require arguments it does not give, and the types
 * of those it takes must match. `bad_index` receives the first bad parameter.
 */
rs_handler_result rs_handler_check(const rs_sig* function, const rs_base* event_types, int event_count, int* bad_index);

/* ---------------------------------------------------------------------------
 *  Native functions known to the language (the API of Raven)
 * ------------------------------------------------------------------------- */

typedef struct rs_native {
	const char* name;
	const char* signature;
	const char* doc;
} rs_native;

int rs_native_count(void);
const rs_native* rs_native_at(int index);
/** The native function of that name and its parsed signature; NULL when there is none. */
const rs_native* rs_native_find(const char* name, rs_sig* sig);

/* ---------------------------------------------------------------------------
 *  The host: what the application provides
 * ------------------------------------------------------------------------- */

typedef enum rs_call_status {
	RS_CALL_OK = 0,
	/** No function of that name. */
	RS_CALL_NOT_FOUND,
	/** The function failed: `error` says why. */
	RS_CALL_FAILED
} rs_call_status;

typedef enum rs_external_kind {
	RS_EXTERNAL_NONE = 0,
	/** A function with a signature (Raction_bind_with_parameters, a native of the API). */
	RS_EXTERNAL_TYPED,
	/** An action without parameters (Raction_bind, a built-in action): called with no argument. */
	RS_EXTERNAL_ACTION
} rs_external_kind;

typedef struct rs_host {
	void* user_data;
	/** Where the diagnostics go. NULL: they are dropped. */
	void (*diagnostic)(void* user_data, const rs_diag* diag);
	/**
	 * Compile time: what the application knows of a name that is not a script
	 * function (functions it bound). NULL: nothing.
	 */
	rs_external_kind (*lookup)(void* user_data, const char* name, rs_sig* sig);
	/**
	 * Run time: calls a function that is not a script function (the natives
	 * of the language included). `result` is void on entry; set it for a
	 * function that returns a value (the core takes the reference).
	 */
	rs_call_status (*call)(void* user_data, const char* name, const rs_value* args, int count, rs_value* result, rs_buf* error);
	/** A script function was installed (the host binds it as an action) or removed. */
	void (*function_installed)(void* user_data, const char* name);
	void (*function_removed)(void* user_data, const char* name);
	/** 1 when a widget has that name, 0 when none has, -1 when unknown (no warning). */
	int (*element_exists)(void* user_data, const char* name);
} rs_host;

/* ---------------------------------------------------------------------------
 *  Environment: the functions and globals of every script loaded
 * ------------------------------------------------------------------------- */

typedef struct rs_env rs_env;
typedef struct rs_batch rs_batch;

/** Defaults of the limits (rs_env_set_limits). */
#define RS_DEFAULT_MAX_INSTRUCTIONS 10000000u
#define RS_DEFAULT_MAX_DEPTH 200
#define RS_DEFAULT_MAX_MEMORY (64u * 1024u * 1024u)

rs_env* rs_env_create(const rs_host* host);
void    rs_env_destroy(rs_env* env);
/** 0 keeps a value. Instructions: per invocation; depth: nested calls; memory: bytes of strings and arrays alive. */
void    rs_env_set_limits(rs_env* env, uint32_t max_instructions, int max_depth, size_t max_memory);
void    rs_env_get_limits(const rs_env* env, uint32_t* max_instructions, int* max_depth, size_t* max_memory);
/** Removes every function and global (the host is told of each function removed). */
void    rs_env_reset(rs_env* env);
const rs_host* rs_env_host(const rs_env* env);

/** The script function of that name; fills its signature. 0 when there is none. */
int  rs_env_find_function(const rs_env* env, const char* name, rs_sig* sig);
/** Where the function was declared (file, line...). 0 when unknown. */
int  rs_env_function_location(const rs_env* env, const char* name, rs_location* location);
int  rs_env_function_count(const rs_env* env);
const char* rs_env_function_name(const rs_env* env, int index);
/** Bytes of strings and arrays alive. */
size_t rs_env_memory(const rs_env* env);

typedef enum rs_call_mode {
	/** A call from code: the arguments must match exactly (count and types). */
	RS_CALL_STRICT = 0,
	/** An event: arguments past the parameters are dropped (specification 12.4). */
	RS_CALL_EVENT
} rs_call_mode;

/**
 * Calls a script function. Errors (a wrong argument, a run time error) are
 * reported to the host and end this call only. Returns RS_CALL_NOT_FOUND
 * when no script function has that name, RS_CALL_FAILED after an error.
 * `result` (may be NULL) receives the value returned (release it).
 * `event` (may be NULL) names the event for the messages ("Slider.OnChanged").
 */
rs_call_status rs_env_call(rs_env* env, const char* name, const rs_value* args, int count, rs_call_mode mode, const char* event, rs_value* result);

/** How deep script calls are nested right now (0: no script runs). */
int rs_env_running(const rs_env* env);

/* ---------------------------------------------------------------------------
 *  Batches: the scripts of one document, compiled together
 * ------------------------------------------------------------------------- */

typedef enum rs_batch_flags {
	/** Record what the editor needs (symbols, references, scopes, hovers). */
	RS_BATCH_ANALYSIS = 1
} rs_batch_flags;

rs_batch* rs_batch_create(rs_env* env, int flags);
void      rs_batch_destroy(rs_batch* batch);
/**
 * Adds the text of a script. `file` names it in the messages; `line` and
 * `column` (1-based) are where the text starts in that file (a <Script> of
 * an XML document starts after its tag). Returns the index of the unit.
 */
int  rs_batch_add(rs_batch* batch, const char* file, const char* text, size_t length, int line, int column);
/** Parses, checks and compiles every unit. Returns the number of errors (0: it can be installed). */
int  rs_batch_compile(rs_batch* batch);
/** Number of errors / warnings reported by rs_batch_compile. */
int  rs_batch_error_count(const rs_batch* batch);
int  rs_batch_warning_count(const rs_batch* batch);
/**
 * Puts the functions and globals into the environment (replacing those of
 * the same name loaded before); the globals hold the default value of
 * their type (0, "", false, [], null) until rs_batch_initialize. Refused
 * (returns 0) when the compilation found errors.
 */
int  rs_batch_install(rs_batch* batch);
/**
 * Runs the initializers of the globals of an installed batch, once (a
 * document runs them after its widgets are built). A run time error in an
 * initializer is reported and stops the initializers of its unit; what was
 * installed stays. Returns 0 after an error.
 */
int  rs_batch_initialize(rs_batch* batch);

/** The functions a batch declares (after rs_batch_compile), for the checks of the document. */
int  rs_batch_function_count(const rs_batch* batch);
const char* rs_batch_function_name(const rs_batch* batch, int index);
int  rs_batch_function_sig(const rs_batch* batch, int index, rs_sig* sig);
int  rs_batch_function_location(const rs_batch* batch, int index, rs_location* location);
/** Looks a function up in the batch, then in the environment. */
int  rs_batch_find_function(const rs_batch* batch, const char* name, rs_sig* sig, rs_location* location);

/**
 * Checks the function of the action of a widget (specification 12.5):
 * reports RS103 when its signature does not fit the event of the tag.
 * `attribute` is the attribute that names the action (OnChanged...), `where`
 * the place of the element in the document. Returns 0 after an error. Names that are not script functions are left alone (the
 * application may bind them).
 */
int  rs_batch_check_handler(rs_batch* batch, const char* tag, const char* attribute, const char* action, const rs_location* where);

/** Reports a diagnostic through the host of the batch (counted as an error or a warning). */
void rs_batch_report(rs_batch* batch, const rs_diag* diag);

/** The location of a byte range of a unit, mapped to its file. */
void rs_batch_locate(const rs_batch* batch, int unit, uint32_t start, uint32_t end, rs_location* location);

/* ---------------------------------------------------------------------------
 *  Analysis (the language server)
 * ------------------------------------------------------------------------- */

/**
 * After rs_batch_compile with RS_BATCH_ANALYSIS: everything the editor needs
 * as JSON (offsets are byte offsets in each unit):
 *   { "diagnostics": [...], "symbols": [...], "references": [...],
 *     "hovers": [...], "calls": [...], "scopes": [...] }
 * Free with rs_free.
 */
char* rs_batch_analysis_json(rs_batch* batch);
/** Completion items at a byte offset of a unit, as a JSON array. Free with rs_free. */
char* rs_batch_complete_json(rs_batch* batch, int unit, uint32_t offset);
/** The events of the widgets and the natives, as JSON (for the editor). Free with rs_free. */
char* rs_language_json(void);

/* ---------------------------------------------------------------------------
 *  Formatter
 * ------------------------------------------------------------------------- */

typedef struct rs_format_options {
	/** One level of indentation ("    " when NULL). */
	const char* indent;
	/** Put before every line (the indentation of a <Script> in XML); NULL: nothing. */
	const char* prefix;
} rs_format_options;

/**
 * The text formatted (rs_free), or NULL when it cannot be (a syntax error:
 * nothing is changed in a text that does not parse).
 */
char* rs_format(const char* text, size_t length, const rs_format_options* options);

/** Version of the language. */
#define RS_LANGUAGE_VERSION "0.1"

#endif /* RS_H */
