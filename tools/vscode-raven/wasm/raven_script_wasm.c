/**
 * The core of Raven Script (src/script/rs_*.c) for the language server of
 * the VS Code extension, compiled to WebAssembly (scripts/build-wasm.js).
 *
 * The editor and the library compile scripts with the same code: what one
 * accepts, the other accepts; what one reports, the other reports.
 *
 * One analysis at a time: rsw_begin, rsw_declare / rsw_set_names, rsw_add
 * for each unit, rsw_compile, rsw_check_handler for each action of the
 * document, then rsw_analysis and rsw_complete, and rsw_end. The texts
 * returned stay valid until the next call of the same function.
 */

#include "../../../src/script/rs_internal.h"

#define RSW_EXPORT(name) __attribute__((export_name(#name)))

typedef struct rsw_declared {
	char* name;
	rs_sig sig;
	char names[256];
} rsw_declared;

static struct {
	rs_env* env;
	rs_batch* batch;
	rsw_declared* declared;
	uint32_t declared_count;
	uint32_t declared_capacity;
	char* names;
	int has_names;
	/* the diagnostics of the handler being checked */
	int collecting;
	rs_buf handler;
	int handler_count;
	char* results[6];
} g;

/* The actions Raven knows by itself (Raction_is_builtin) that a script may call (names without ':') */
static const char* const g_builtins[] = {
	"app.quit", "window.close", "window.minimize", "window.maximize", "window.restore", "window.picture_in_picture",
	"island.expand", "island.fold", "island.toggle", "theme.dark", "theme.light", "theme.toggle"
};

static const char* rsw_keep(int slot, char* text) {
	rs_free(g.results[slot]);
	g.results[slot] = text;
	return text;
}

static void rsw_json_location(rs_buf* out, const rs_location* location) {
	rs_buf_printf(out, "{\"unit\":%d,\"start\":%d,\"end\":%d,\"line\":%d,\"column\":%d}", location->unit, location->start, location->end, location->line, location->column);
}

static void rsw_diagnostic(void* user_data, const rs_diag* diag) {
	rs_buf* out = &g.handler;
	(void)user_data;
	if (!g.collecting) return;
	if (g.handler_count++ > 0) rs_buf_putc(out, ',');
	rs_buf_printf(out, "{\"severity\":%d,\"code\":", (int)diag->severity);
	rs_buf_json_string(out, diag->code);
	rs_buf_puts(out, ",\"title\":");
	rs_buf_json_string(out, diag->title);
	rs_buf_puts(out, ",\"message\":");
	rs_buf_json_string(out, diag->message);
	rs_buf_puts(out, ",\"help\":");
	rs_buf_json_string(out, diag->help ? diag->help : "");
	rs_buf_puts(out, ",\"related\":");
	if (diag->has_related) rsw_json_location(out, &diag->related);
	else rs_buf_puts(out, "null");
	rs_buf_putc(out, '}');
}

static rs_external_kind rsw_lookup(void* user_data, const char* name, rs_sig* sig) {
	uint32_t i;
	(void)user_data;
	for (i = 0; i < g.declared_count; i++) {
		if (strcmp(g.declared[i].name, name) == 0) {
			*sig = g.declared[i].sig;
			return RS_EXTERNAL_TYPED;
		}
	}
	for (i = 0; i < RS_COUNT_OF(g_builtins); i++) {
		if (strcmp(g_builtins[i], name) == 0) return RS_EXTERNAL_ACTION;
	}
	return RS_EXTERNAL_NONE;
}

static int rsw_element_exists(void* user_data, const char* name) {
	const char* p = g.names;
	size_t length = strlen(name);
	(void)user_data;
	if (!g.has_names || !p) return -1;
	while (*p) {
		const char* end = strchr(p, '\n');
		size_t part = end ? (size_t)(end - p) : strlen(p);
		if (part == length && memcmp(p, name, length) == 0) return 1;
		if (!end) break;
		p = end + 1;
	}
	return 0;
}

RSW_EXPORT(rsw_alloc) void* rsw_alloc(int size) {
	return rs_malloc(size > 0 ? (size_t)size : 1);
}

RSW_EXPORT(rsw_free) void rsw_free(void* pointer) {
	rs_free(pointer);
}

RSW_EXPORT(rsw_end) void rsw_end(void) {
	uint32_t i;
	if (g.batch) rs_batch_destroy(g.batch);
	if (g.env) rs_env_destroy(g.env);
	g.batch = NULL;
	g.env = NULL;
	for (i = 0; i < g.declared_count; i++) rs_free(g.declared[i].name);
	g.declared_count = 0;
	rs_free(g.names);
	g.names = NULL;
	g.has_names = 0;
	rs_buf_free(&g.handler);
}

RSW_EXPORT(rsw_begin) void rsw_begin(void) {
	rs_host host;
	rsw_end();
	memset(&host, 0, sizeof(host));
	host.diagnostic = rsw_diagnostic;
	host.lookup = rsw_lookup;
	host.element_exists = rsw_element_exists;
	g.env = rs_env_create(&host);
	g.batch = rs_batch_create(g.env, RS_BATCH_ANALYSIS);
}

/** A function the application binds (a setting of the editor): known to the type checker. 0 when the signature is invalid. */
RSW_EXPORT(rsw_declare) int rsw_declare(const char* name, const char* signature) {
	rsw_declared* declared;
	rs_sig sig;
	char names[256];
	int i;
	if (!rs_sig_parse(signature, &sig, names, sizeof(names), NULL, 0)) return 0;
	RS_GROW(g.declared, g.declared_count, g.declared_capacity);
	declared = &g.declared[g.declared_count++];
	declared->name = rs_strdup(name);
	memcpy(declared->names, names, sizeof(names));
	declared->sig = sig;
	for (i = 0; i < sig.count; i++) {
		if (sig.names[i]) declared->sig.names[i] = declared->names + (sig.names[i] - names);
	}
	return 1;
}

/** The names of the widgets of the document, one per line (RS301). */
RSW_EXPORT(rsw_set_names) void rsw_set_names(const char* names) {
	rs_free(g.names);
	g.names = rs_strdup(names);
	g.has_names = 1;
}

RSW_EXPORT(rsw_add) int rsw_add(const char* file, const char* text, int length, int line, int column) {
	return rs_batch_add(g.batch, file, text, (size_t)(length > 0 ? length : 0), line, column);
}

RSW_EXPORT(rsw_compile) int rsw_compile(void) {
	return rs_batch_compile(g.batch);
}

/** The diagnostics (a JSON array) of the action `action` of an element `tag` at line / column of the document. */
RSW_EXPORT(rsw_check_handler) const char* rsw_check_handler(const char* tag, const char* attribute, const char* action, int line, int column) {
	rs_location where;
	rs_buf result = { 0 };
	memset(&where, 0, sizeof(where));
	where.file = "";
	where.line = line;
	where.column = column;
	where.end_line = line;
	where.end_column = column;
	where.unit = -1;
	where.start = -1;
	where.end = -1;
	rs_buf_free(&g.handler);
	g.handler_count = 0;
	g.collecting = 1;
	rs_batch_check_handler(g.batch, tag, attribute, action, &where);
	g.collecting = 0;
	rs_buf_putc(&result, '[');
	if (g.handler.data) rs_buf_puts(&result, g.handler.data);
	rs_buf_putc(&result, ']');
	return rsw_keep(0, rs_buf_take(&result));
}

RSW_EXPORT(rsw_analysis) const char* rsw_analysis(void) {
	return rsw_keep(1, rs_batch_analysis_json(g.batch));
}

RSW_EXPORT(rsw_complete) const char* rsw_complete(int unit, int offset) {
	return rsw_keep(2, rs_batch_complete_json(g.batch, unit, (uint32_t)(offset > 0 ? offset : 0)));
}

/** NULL (0) when the text has a syntax error. */
RSW_EXPORT(rsw_format) const char* rsw_format(const char* text, int length, const char* indent, const char* prefix) {
	rs_format_options options;
	memset(&options, 0, sizeof(options));
	options.indent = indent && indent[0] ? indent : NULL;
	options.prefix = prefix && prefix[0] ? prefix : NULL;
	return rsw_keep(3, rs_format(text, (size_t)(length > 0 ? length : 0), &options));
}

RSW_EXPORT(rsw_language) const char* rsw_language(void) {
	return rsw_keep(4, rs_language_json());
}
