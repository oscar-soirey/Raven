/**
 * Raven - script/rs_internal.h
 * What the files of the core of Raven Script share: tokens, syntax tree,
 * bytecode, the environment.
 */

#ifndef RS_INTERNAL_H
#define RS_INTERNAL_H

#include "rs.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define RS_COUNT_OF(a) (sizeof(a) / sizeof((a)[0]))
#define RS_UNUSED(x) ((void)(x))

/** Grows an array: `array` of `count` items, `capacity` items allocated. */
#define RS_GROW(array, count, capacity) do { \
	if ((count) >= (capacity)) { \
		size_t rs_new_capacity_ = (capacity) ? (size_t)(capacity) * 2 : 8; \
		(array) = rs_realloc((array), rs_new_capacity_ * sizeof(*(array))); \
		(capacity) = (uint32_t)rs_new_capacity_; \
	} \
} while (0)

/* ---------------------------------------------------------------------------
 *  Arena: the syntax tree and the strings of a batch, freed at once
 * ------------------------------------------------------------------------- */

typedef struct rs_arena_block {
	struct rs_arena_block* next;
	size_t used;
	size_t size;
	/* the memory follows */
} rs_arena_block;

typedef struct rs_arena {
	rs_arena_block* head;
} rs_arena;

void* rs_arena_alloc(rs_arena* arena, size_t size);
char* rs_arena_strndup(rs_arena* arena, const char* text, size_t length);
void  rs_arena_free(rs_arena* arena);

/* ---------------------------------------------------------------------------
 *  Numbers (independent of the locale of the C library)
 * ------------------------------------------------------------------------- */

/** Parses a whole number literal ("42", "3.14", "1e-3", "0x1F"). 0 when it is not one. */
int  rs_parse_number(const char* text, size_t length, double* value);
/** Parses the text given to number.parse: white space around it is allowed. */
int  rs_parse_number_text(const char* text, double* value);
/** The shortest text that reads back as the same number ("42", "0.1", "NaN"). */
void rs_format_number(double value, char* out, size_t size);

/* ---------------------------------------------------------------------------
 *  Tokens
 * ------------------------------------------------------------------------- */

typedef enum rs_tok {
	TK_EOF = 0,
	TK_ERROR,
	TK_IDENT,
	TK_NUMBER,
	TK_STRING,
	TK_COMMENT,
	/* keywords */
	TK_FN,
	TK_LET,
	TK_CONST,
	TK_IF,
	TK_ELSE,
	TK_FOR,
	TK_WHILE,
	TK_BREAK,
	TK_CONTINUE,
	TK_RETURN,
	TK_TRUE,
	TK_FALSE,
	TK_NULL,
	TK_DECLARE,
	/* punctuation */
	TK_LPAREN,
	TK_RPAREN,
	TK_LBRACE,
	TK_RBRACE,
	TK_LBRACKET,
	TK_RBRACKET,
	TK_COMMA,
	TK_DOT,
	TK_SEMICOLON,
	TK_COLON,
	TK_PLUS,
	TK_MINUS,
	TK_STAR,
	TK_SLASH,
	TK_PERCENT,
	TK_BANG,
	TK_ASSIGN,
	TK_PLUS_ASSIGN,
	TK_MINUS_ASSIGN,
	TK_STAR_ASSIGN,
	TK_SLASH_ASSIGN,
	TK_PERCENT_ASSIGN,
	TK_EQ,
	TK_NE,
	TK_LT,
	TK_LE,
	TK_GT,
	TK_GE,
	TK_AND,
	TK_OR,
	/* not part of the language: recognized for a better message */
	TK_PLUS_PLUS,
	TK_MINUS_MINUS,
	TK_STRICT_EQ,
	TK_STRICT_NE,
	TK_COUNT_
} rs_tok;

typedef struct rs_token {
	uint8_t kind;
	uint32_t start;
	uint32_t end;
	/** For TK_ERROR: what is wrong. */
	const char* error;
} rs_token;

typedef struct rs_lexer {
	const char* text;
	uint32_t length;
	uint32_t cursor;
	/** Return comments as TK_COMMENT tokens (the formatter); otherwise they are skipped. */
	int keep_comments;
} rs_lexer;

void rs_lexer_init(rs_lexer* lexer, const char* text, size_t length, int keep_comments);
rs_token rs_lexer_next(rs_lexer* lexer);
/** The spelling of a token kind for the messages ("'{'", "identifier"). */
const char* rs_tok_name(int kind);
int rs_is_ident_start(int c);
int rs_is_ident_char(int c);
/** The keyword of an identifier, or TK_IDENT. */
int rs_keyword(const char* text, size_t length);
/**
 * Decodes the escapes of a string literal (the text between the quotes).
 * Returns 0 and the offset (in `bad`) of a wrong escape.
 */
int rs_unescape(const char* text, size_t length, rs_buf* out, size_t* bad);

/* ---------------------------------------------------------------------------
 *  Units and their lines
 * ------------------------------------------------------------------------- */

typedef struct rs_unit {
	char* file;
	char* text;
	uint32_t length;
	int base_line;
	int base_column;
	uint32_t* line_starts;
	uint32_t line_count;
} rs_unit;

/** A copy of a unit shared by the functions compiled from it (their run time errors show its lines). */
typedef struct rs_source {
	uint32_t refs;
	rs_unit unit;
} rs_source;

rs_source* rs_source_create(const rs_unit* unit);
void rs_source_retain(rs_source* source);
void rs_source_release(rs_source* source);

void rs_unit_init(rs_unit* unit, const char* file, const char* text, size_t length, int line, int column);
void rs_unit_free(rs_unit* unit);
/** Line (1-based, in the unit) of a byte offset. */
uint32_t rs_unit_line_of(const rs_unit* unit, uint32_t offset);
/** File position (1-based line and column in code points) of an offset of the unit. */
void rs_unit_position(const rs_unit* unit, uint32_t offset, int* line, int* column);
/** Fills a location for a range of the unit (the text of the line is a pointer into a copy owned by `storage`). */
void rs_unit_locate(const rs_unit* unit, int index, uint32_t start, uint32_t end, rs_location* location, rs_buf* storage);

/* ---------------------------------------------------------------------------
 *  Syntax tree
 * ------------------------------------------------------------------------- */

typedef enum rs_node_kind {
	/* expressions */
	RS_N_NUMBER = 1,
	RS_N_STRING,
	RS_N_BOOL,
	RS_N_NULL,
	RS_N_NAME,
	RS_N_MEMBER,
	RS_N_CALL,
	RS_N_INDEX,
	RS_N_UNARY,
	RS_N_BINARY,
	RS_N_ARRAY,
	/* statements */
	RS_N_LET,
	RS_N_ASSIGN,
	RS_N_EXPR,
	RS_N_IF,
	RS_N_WHILE,
	RS_N_FOR,
	RS_N_BREAK,
	RS_N_CONTINUE,
	RS_N_RETURN,
	RS_N_BLOCK,
	/* top level */
	RS_N_FN
} rs_node_kind;

/** How a name or a call was resolved by the type checker. */
typedef enum rs_resolved {
	RS_R_NONE = 0,
	RS_R_LOCAL,
	RS_R_GLOBAL,
	/** A call of a script function (by name, at run time). */
	RS_R_FUNCTION,
	/** A call of something the host runs (a native, a bound action). */
	RS_R_EXTERNAL,
	/** A method or a property of a value (toString, length, push...). */
	RS_R_METHOD,
	/** number.parse and the other functions the core runs itself. */
	RS_R_INTRINSIC
} rs_resolved;

typedef struct rs_node rs_node;
typedef struct rs_param {
	char* name;
	uint32_t start, end;
	rs_type type;
	uint32_t type_start, type_end;
} rs_param;

struct rs_node {
	uint8_t kind;
	/** The token of an operator (unary, binary, compound assignment); for RS_N_LET: R_TRUE for const. */
	uint8_t op;
	/** Resolution (rs_resolved), filled by the checker. */
	uint8_t resolved;
	/** RS_N_FN: a declaration (declare fn) without a body. */
	uint8_t is_declare;
	/** Set when the type is written (let x: number). */
	uint8_t has_annotation;
	/** A name or an assignment of a global of this batch (its slot is relocated when the batch is installed). */
	uint8_t batch_global;
	int unit;
	uint32_t start;
	uint32_t end;
	/** Type of an expression (checker); the declared or inferred type of a let; the return type of a fn. */
	rs_type type;
	rs_type annotation;
	uint32_t annotation_start, annotation_end;
	/** Literal value. */
	double number;
	/** Identifier, member name, function name, string value (decoded). */
	char* text;
	uint32_t text_length;
	/** Where the name is (let x, fn a.b, member .x): for the references. */
	uint32_t name_start, name_end;
	/** Children: condition / left / object / callee / initializer... */
	rs_node* a;
	rs_node* b;
	rs_node* c;
	rs_node* d;
	/** Arguments, items, statements. */
	rs_node** list;
	uint32_t count;
	/** fn: parameters. */
	rs_param* params;
	uint32_t param_count;
	/** Checker: local slot, global slot, method id, symbol id (analysis). */
	int slot;
	int symbol;
	/** Checker: the dotted name of a call of a function. */
	char* path;
	/** fn: number of local slots; for loops: whether a break leaves it. */
	int locals;
	int has_break;
};

typedef struct rs_program {
	rs_node** items;
	uint32_t count;
	uint32_t capacity;
} rs_program;

struct rs_batch;
/** Parses a unit into `program` (items are RS_N_FN and RS_N_LET). Errors are reported to the batch. */
void rs_parse(struct rs_batch* batch, int unit, rs_program* program);

/* ---------------------------------------------------------------------------
 *  Bytecode
 * ------------------------------------------------------------------------- */

typedef enum rs_op {
	OP_NOP = 0,
	OP_CONST,         /* a: constant index */
	OP_NULL,
	OP_TRUE,
	OP_FALSE,
	OP_POP,
	OP_DUP2,          /* a b -> a b a b */
	OP_LOAD_LOCAL,    /* a: slot */
	OP_STORE_LOCAL,   /* a: slot (pops) */
	OP_LOAD_GLOBAL,   /* a: global slot of the environment */
	OP_STORE_GLOBAL,  /* a: global slot (pops) */
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_NEG,
	OP_NOT,
	OP_EQ,
	OP_NE,
	OP_LT,
	OP_LE,
	OP_GT,
	OP_GE,
	OP_JUMP,          /* a: target */
	OP_JUMP_IF_FALSE, /* a: target (pops) */
	OP_AND,           /* a: target: false stays and jumps, true is popped */
	OP_OR,            /* a: target: true stays and jumps, false is popped */
	OP_ARRAY,         /* a: count, b: element type (base | depth << 8) */
	OP_INDEX,         /* array index -> value */
	OP_STORE_INDEX,   /* array index value -> */
	OP_CALL,          /* a: constant index of the name, b: argument count, c: return type expected */
	OP_CALL_EXTERNAL, /* a: constant index of the name, b: argument count, c: return type expected */
	OP_METHOD,        /* a: method id, b: argument count (the receiver is under them) */
	OP_INTRINSIC,     /* a: intrinsic id, b: argument count */
	OP_RETURN,        /* returns the value on the stack */
	OP_RETURN_VOID,
	OP_MISSING_RETURN,/* the end of a function that should have returned */
	OP_COUNT_
} rs_op;

typedef struct rs_instr {
	uint8_t op;
	int32_t a;
	int32_t b;
	/** OP_CALL, OP_CALL_EXTERNAL: the type the caller expects back (base | depth << 8), checked when it returns. */
	int32_t c;
	/** Where it comes from (the error messages). */
	uint32_t start;
	uint32_t end;
} rs_instr;

/** The bytecode of a function (or of the initializers of a batch). Shared, counted. */
typedef struct rs_chunk {
	uint32_t refs;
	char* name;
	rs_instr* code;
	uint32_t count;
	uint32_t capacity;
	rs_value* constants;
	uint32_t constant_count;
	uint32_t constant_capacity;
	int param_count;
	int local_count;
	int max_stack;
	rs_sig sig;
	/** Owned copies of the names of the parameters. */
	char* param_names[RS_MAX_PARAMS];
	/** The text the code comes from (run time errors show it). */
	rs_source* source;
} rs_chunk;

rs_chunk* rs_chunk_create(const char* name);
void rs_chunk_retain(rs_chunk* chunk);
void rs_chunk_release(rs_chunk* chunk);
/** Checks a chunk before it runs: operands in range, jumps on instructions, stack within bounds. 0 when invalid. */
int  rs_chunk_validate(const rs_chunk* chunk, uint32_t global_count, char* error, size_t error_size);

/* methods of the values and functions the core runs itself */
typedef enum rs_method {
	RS_M_TO_STRING = 0,
	RS_M_LENGTH,
	RS_M_PUSH,
	RS_M_POP,
	RS_M_TO_UPPER,
	RS_M_TO_LOWER,
	RS_M_TRIM,
	RS_M_CONTAINS,
	RS_M_STARTS_WITH,
	RS_M_ENDS_WITH,
	RS_M_INDEX_OF,
	RS_M_SUBSTRING,
	RS_M_SPLIT,
	RS_M_TO_FIXED,
	RS_M_COUNT_
} rs_method;

typedef struct rs_method_info {
	const char* name;
	/** R_TRUE: written without parentheses (numbers.length). */
	int property;
	const char* doc;
} rs_method_info;

const rs_method_info* rs_method_info_of(int method);
/**
 * The method `name` of a value of type `receiver`: its id and signature
 * (the result type depends on the receiver). -1 when it has none.
 */
int rs_method_lookup(rs_type receiver, const char* name, rs_sig* sig);

typedef enum rs_intrinsic {
	RS_I_NUMBER_PARSE = 0,
	RS_I_COUNT_
} rs_intrinsic;

typedef struct rs_intrinsic_info {
	const char* name;
	const char* signature;
	const char* doc;
} rs_intrinsic_info;

const rs_intrinsic_info* rs_intrinsic_info_of(int intrinsic);
int rs_intrinsic_find(const char* name, rs_sig* sig);

/* ---------------------------------------------------------------------------
 *  Environment
 * ------------------------------------------------------------------------- */

typedef struct rs_function {
	char* name;
	rs_chunk* chunk;
	/** Where it was declared. */
	char* file;
	int line;
	int column;
} rs_function;

typedef struct rs_global {
	char* name;
	rs_type type;
	int is_const;
	rs_value value;
	char* file;
	int line;
	int column;
} rs_global;

typedef struct rs_frame rs_frame;

struct rs_env {
	rs_host host;
	rs_function* functions;
	uint32_t function_count;
	uint32_t function_capacity;
	rs_global* globals;
	uint32_t global_count;
	uint32_t global_capacity;
	uint32_t max_instructions;
	int max_depth;
	size_t max_memory;
	size_t memory;
	/** Nested invocations (an event while a script runs). */
	int running;
	int depth;
	/** The value stack, shared by nested invocations. */
	rs_value* stack;
	uint32_t stack_size;
	uint32_t stack_top;
	rs_frame* frames;
	int frame_count;
};

/** Index of the function, or -1. */
int  rs_env_function_index(const rs_env* env, const char* name);
/** Index of the current global of that name, or -1. */
int  rs_env_global_index(const rs_env* env, const char* name);
/**
 * Defines the global of slot `slot`: a new one when `slot` is the count of
 * globals, else the global of that slot is declared again (its value goes
 * back to the default of its new type).
 */
void rs_env_define_global(rs_env* env, uint32_t slot, const char* name, rs_type type, int is_const, const char* file, int line, int column);
void rs_env_set_function(rs_env* env, const char* name, rs_chunk* chunk, const char* file, int line, int column);
/** Reports a diagnostic through the host. */
void rs_env_report(rs_env* env, const rs_diag* diag);

/* memory of the values (counted against the limit of the environment) */
rs_string* rs_string_new(rs_env* env, const char* text, size_t length);
rs_array*  rs_array_new(rs_env* env, rs_type element, uint32_t capacity);
int  rs_array_push(rs_env* env, rs_array* array, rs_value value);
/** The default value of a type (0, "", false, [], null): what a global holds before its initializer. */
rs_value rs_value_default(rs_env* env, rs_type type);
/** Equality as == sees it (strings by content, arrays by identity). */
int  rs_value_equal(rs_value a, rs_value b);

/** Runs a chunk with the arguments on the stack. */
rs_call_status rs_vm_run(rs_env* env, rs_chunk* chunk, const rs_value* args, int count, rs_value* result);

/* ---------------------------------------------------------------------------
 *  Batch
 * ------------------------------------------------------------------------- */

/** What the editor needs to know of a name (analysis). */
typedef enum rs_symbol_kind {
	RS_SYM_FUNCTION = 0,
	RS_SYM_DECLARED,
	RS_SYM_NATIVE,
	RS_SYM_GLOBAL,
	RS_SYM_CONST,
	RS_SYM_LOCAL,
	RS_SYM_LOCAL_CONST,
	RS_SYM_PARAMETER,
	RS_SYM_EXTERNAL
} rs_symbol_kind;

typedef struct rs_symbol {
	int kind;
	char* name;
	rs_type type;
	rs_sig sig;
	int has_sig;
	/** Declaration: unit -1 when it is not in the batch (a native, an earlier script). */
	int unit;
	uint32_t start, end;
	/** Where the name is visible (locals): from `start` to `scope_end` of the unit. */
	uint32_t scope_end;
	/** The function it belongs to (locals), or -1. */
	int function;
	const char* doc;
	char* file;
	int line;
} rs_symbol;

typedef struct rs_reference {
	int symbol;
	int unit;
	uint32_t start, end;
} rs_reference;

typedef struct rs_hover {
	int unit;
	uint32_t start, end;
	char* text;
} rs_hover;

typedef struct rs_call_site {
	int unit;
	uint32_t open;
	uint32_t close;
	/** Offsets of the commas. */
	uint32_t* commas;
	uint32_t comma_count;
	char* label;
	char* doc;
	int param_count;
	/** Offsets (into label) of each parameter. */
	int param_start[RS_MAX_PARAMS];
	int param_end[RS_MAX_PARAMS];
} rs_call_site;

typedef struct rs_fn_info {
	char* name;
	rs_sig sig;
	char* names_storage;
	int unit;
	uint32_t name_start, name_end;
	rs_node* node;
	rs_chunk* chunk;
	int symbol;
} rs_fn_info;

typedef struct rs_global_info {
	rs_node* node;
	int slot;
	int symbol;
} rs_global_info;

struct rs_batch {
	rs_env* env;
	int flags;
	rs_unit* units;
	uint32_t unit_count;
	uint32_t unit_capacity;
	rs_arena arena;
	rs_program* programs;
	rs_fn_info* functions;
	uint32_t function_count;
	uint32_t function_capacity;
	rs_global_info* globals;
	uint32_t global_count;
	uint32_t global_capacity;
	/** The initializers of the globals: one chunk per unit (unit_count entries, NULL for a unit without globals). */
	rs_chunk** inits;
	/** One shared copy of each unit for the chunks (unit_count entries, NULL until compiled). */
	rs_source** sources;
	int compiled;
	int installed;
	int initialized;
	int errors;
	/** The errors of the compilation (those reported later, by the checks of handlers, do not prevent installing). */
	int compile_errors;
	int warnings;
	/** Global slots this batch will take when installed: env->global_count + index. */
	uint32_t global_base;
	/* analysis */
	rs_symbol* symbols;
	uint32_t symbol_count;
	uint32_t symbol_capacity;
	rs_reference* references;
	uint32_t reference_count;
	uint32_t reference_capacity;
	rs_hover* hovers;
	uint32_t hover_count;
	uint32_t hover_capacity;
	rs_call_site* calls;
	uint32_t call_count;
	uint32_t call_capacity;
	/** Diagnostics kept for the analysis (JSON). */
	rs_buf diagnostics_json;
	int diagnostic_count;
};

/** Reports an error / warning at a range of a unit. `help` may be NULL. */
void rs_report(rs_batch* batch, rs_severity severity, const char* code, const char* title, int unit, uint32_t start, uint32_t end, const char* help, const char* format, ...);
/** The same with a second place (unit -1: none). */
void rs_report_related(rs_batch* batch, rs_severity severity, const char* code, const char* title, int unit, uint32_t start, uint32_t end,
	int related_unit, uint32_t related_start, uint32_t related_end, const char* related_note, const char* help, const char* format, ...);

/** Type checks the programs of the batch, filling the tree. */
void rs_check(rs_batch* batch);
/** Compiles the checked programs into chunks. */
void rs_compile(rs_batch* batch);

/* analysis */
int  rs_symbol_add(rs_batch* batch, int kind, const char* name, rs_type type, const rs_sig* sig, int unit, uint32_t start, uint32_t end);
void rs_reference_add(rs_batch* batch, int symbol, int unit, uint32_t start, uint32_t end);
void rs_hover_add(rs_batch* batch, int unit, uint32_t start, uint32_t end, const char* format, ...);

#endif /* RS_INTERNAL_H */
