/**
 * Raven Script - script/rs_analysis.c
 * What the language server needs of a batch compiled with
 * RS_BATCH_ANALYSIS, as JSON: diagnostics, symbols and their references
 * (go to definition, find references, rename), hovers, call sites
 * (signature help) and completion. Offsets are byte offsets in each unit:
 * the server maps them to the positions of its documents.
 */

#include "rs_internal.h"

static const char* const g_symbol_kinds[] = {
	"function", "declared", "native", "global", "const", "local", "localConst", "parameter", "external"
};

char* rs_batch_analysis_json(rs_batch* batch) {
	rs_buf out = { 0 };
	uint32_t i, j;
	rs_buf_puts(&out, "{\"diagnostics\":[");
	if (batch->diagnostics_json.data) rs_buf_puts(&out, batch->diagnostics_json.data);
	rs_buf_puts(&out, "],\"units\":[");
	for (i = 0; i < batch->unit_count; i++) {
		if (i) rs_buf_putc(&out, ',');
		rs_buf_puts(&out, "{\"file\":");
		rs_buf_json_string(&out, batch->units[i].file);
		rs_buf_printf(&out, ",\"length\":%u}", batch->units[i].length);
	}
	rs_buf_puts(&out, "],\"symbols\":[");
	for (i = 0; i < batch->symbol_count; i++) {
		const rs_symbol* symbol = &batch->symbols[i];
		if (i) rs_buf_putc(&out, ',');
		rs_buf_printf(&out, "{\"id\":%u,\"kind\":\"%s\",\"name\":", i, g_symbol_kinds[symbol->kind]);
		rs_buf_json_string(&out, symbol->name);
		rs_buf_puts(&out, ",\"type\":");
		rs_buf_json_string(&out, rs_type_name(symbol->type));
		if (symbol->has_sig) {
			rs_buf sig = { 0 };
			rs_sig_format(&symbol->sig, &sig);
			rs_buf_puts(&out, ",\"signature\":");
			rs_buf_json_string(&out, sig.data);
			rs_buf_free(&sig);
		}
		rs_buf_printf(&out, ",\"unit\":%d,\"start\":%u,\"end\":%u,\"scopeEnd\":%u,\"function\":%d", symbol->unit, symbol->start, symbol->end, symbol->scope_end, symbol->function);
		if (symbol->doc) {
			rs_buf_puts(&out, ",\"doc\":");
			rs_buf_json_string(&out, symbol->doc);
		}
		if (symbol->file) {
			rs_buf_puts(&out, ",\"file\":");
			rs_buf_json_string(&out, symbol->file);
			rs_buf_printf(&out, ",\"line\":%d", symbol->line);
		}
		rs_buf_putc(&out, '}');
	}
	rs_buf_puts(&out, "],\"references\":[");
	for (i = 0; i < batch->reference_count; i++) {
		const rs_reference* reference = &batch->references[i];
		if (i) rs_buf_putc(&out, ',');
		rs_buf_printf(&out, "[%d,%d,%u,%u]", reference->symbol, reference->unit, reference->start, reference->end);
	}
	rs_buf_puts(&out, "],\"hovers\":[");
	for (i = 0; i < batch->hover_count; i++) {
		const rs_hover* hover = &batch->hovers[i];
		if (i) rs_buf_putc(&out, ',');
		rs_buf_printf(&out, "{\"unit\":%d,\"start\":%u,\"end\":%u,\"text\":", hover->unit, hover->start, hover->end);
		rs_buf_json_string(&out, hover->text);
		rs_buf_putc(&out, '}');
	}
	rs_buf_puts(&out, "],\"calls\":[");
	for (i = 0; i < batch->call_count; i++) {
		const rs_call_site* call = &batch->calls[i];
		if (i) rs_buf_putc(&out, ',');
		rs_buf_printf(&out, "{\"unit\":%d,\"open\":%u,\"close\":%u,\"commas\":[", call->unit, call->open, call->close);
		for (j = 0; j < call->comma_count; j++) rs_buf_printf(&out, "%s%u", j ? "," : "", call->commas[j]);
		rs_buf_puts(&out, "],\"label\":");
		rs_buf_json_string(&out, call->label);
		rs_buf_puts(&out, ",\"doc\":");
		rs_buf_json_string(&out, call->doc);
		rs_buf_puts(&out, ",\"parameters\":[");
		for (j = 0; j < (uint32_t)call->param_count; j++) rs_buf_printf(&out, "%s[%d,%d]", j ? "," : "", call->param_start[j], call->param_end[j]);
		rs_buf_puts(&out, "]}");
	}
	rs_buf_puts(&out, "],\"functions\":[");
	for (i = 0; i < batch->function_count; i++) {
		const rs_fn_info* info = &batch->functions[i];
		rs_buf sig = { 0 };
		if (i) rs_buf_putc(&out, ',');
		rs_sig_format(&info->sig, &sig);
		rs_buf_puts(&out, "{\"name\":");
		rs_buf_json_string(&out, info->name);
		rs_buf_puts(&out, ",\"signature\":");
		rs_buf_json_string(&out, sig.data);
		rs_buf_printf(&out, ",\"declare\":%s,\"symbol\":%d,\"unit\":%d,\"start\":%u,\"end\":%u}", info->node->is_declare ? "true" : "false", info->symbol, info->unit, info->name_start, info->name_end);
		rs_buf_free(&sig);
	}
	rs_buf_puts(&out, "]}");
	return rs_buf_take(&out);
}

/* ---------------------------------------------------------------------------
 *  Completion
 * ------------------------------------------------------------------------- */

typedef struct rs_items {
	rs_buf out;
	int count;
	/* labels already given (no duplicate) */
	char** labels;
	uint32_t label_count;
	uint32_t label_capacity;
} rs_items;

static void rs_item(rs_items* items, const char* label, const char* kind, const char* detail, const char* doc, const char* insert, int snippet) {
	uint32_t i;
	for (i = 0; i < items->label_count; i++) {
		if (strcmp(items->labels[i], label) == 0) return;
	}
	RS_GROW(items->labels, items->label_count, items->label_capacity);
	items->labels[items->label_count++] = rs_strdup(label);
	if (items->count++ > 0) rs_buf_putc(&items->out, ',');
	rs_buf_puts(&items->out, "{\"label\":");
	rs_buf_json_string(&items->out, label);
	rs_buf_printf(&items->out, ",\"kind\":\"%s\"", kind);
	if (detail) {
		rs_buf_puts(&items->out, ",\"detail\":");
		rs_buf_json_string(&items->out, detail);
	}
	if (doc && doc[0]) {
		rs_buf_puts(&items->out, ",\"doc\":");
		rs_buf_json_string(&items->out, doc);
	}
	if (insert) {
		rs_buf_puts(&items->out, ",\"insertText\":");
		rs_buf_json_string(&items->out, insert);
		rs_buf_printf(&items->out, ",\"snippet\":%s", snippet ? "true" : "false");
	}
	rs_buf_putc(&items->out, '}');
}

static char* rs_items_take(rs_items* items) {
	rs_buf result = { 0 };
	uint32_t i;
	rs_buf_putc(&result, '[');
	if (items->out.data) rs_buf_puts(&result, items->out.data);
	rs_buf_putc(&result, ']');
	for (i = 0; i < items->label_count; i++) rs_free(items->labels[i]);
	rs_free(items->labels);
	rs_buf_free(&items->out);
	return rs_buf_take(&result);
}

/** A function name: after the chain "ui" (prefix "ui."), offer the next segment. */
static void rs_offer_function(rs_items* items, const char* name, const char* chain, const rs_sig* sig, const char* doc) {
	size_t chain_length = strlen(chain);
	const char* rest;
	const char* dot;
	if (chain_length) {
		if (strncmp(name, chain, chain_length) != 0 || name[chain_length] != '.') return;
		rest = name + chain_length + 1;
	} else {
		rest = name;
	}
	dot = strchr(rest, '.');
	if (dot) {
		char segment[128];
		size_t length = (size_t)(dot - rest);
		if (length >= sizeof(segment)) return;
		memcpy(segment, rest, length);
		segment[length] = '\0';
		rs_item(items, segment, "module", "namespace", NULL, NULL, 0);
	} else {
		rs_buf detail = { 0 };
		rs_buf insert = { 0 };
		rs_buf_puts(&detail, name);
		if (sig) rs_sig_format(sig, &detail);
		if (sig && sig->count > 0) rs_buf_printf(&insert, "%s($1)", rest);
		else rs_buf_printf(&insert, "%s()", rest);
		rs_item(items, rest, "function", detail.data, doc, insert.data, 1);
		rs_buf_free(&detail);
		rs_buf_free(&insert);
	}
}

static void rs_offer_functions(rs_batch* batch, rs_items* items, const char* chain) {
	uint32_t i;
	int n;
	for (i = 0; i < batch->function_count; i++) rs_offer_function(items, batch->functions[i].name, chain, &batch->functions[i].sig, NULL);
	for (i = 0; i < batch->env->function_count; i++) rs_offer_function(items, batch->env->functions[i].name, chain, &batch->env->functions[i].chunk->sig, NULL);
	for (n = 0; n < rs_native_count(); n++) {
		rs_sig sig;
		const rs_native* native = rs_native_at(n);
		rs_native_find(native->name, &sig);
		rs_offer_function(items, native->name, chain, &sig, native->doc);
	}
	for (n = 0; n < RS_I_COUNT_; n++) {
		rs_sig sig;
		rs_intrinsic_find(rs_intrinsic_info_of(n)->name, &sig);
		rs_offer_function(items, rs_intrinsic_info_of(n)->name, chain, &sig, rs_intrinsic_info_of(n)->doc);
	}
}

/** The type of a variable visible at an offset, or 0. */
static int rs_variable_type(rs_batch* batch, int unit, uint32_t offset, const char* name, rs_type* type) {
	uint32_t i;
	int best = -1;
	for (i = 0; i < batch->symbol_count; i++) {
		const rs_symbol* symbol = &batch->symbols[i];
		if (strcmp(symbol->name, name) != 0) continue;
		if (symbol->kind == RS_SYM_LOCAL || symbol->kind == RS_SYM_LOCAL_CONST || symbol->kind == RS_SYM_PARAMETER) {
			if (symbol->unit != unit || symbol->end > offset || offset > symbol->scope_end) continue;
			if (best < 0 || batch->symbols[best].start < symbol->start || batch->symbols[best].kind == RS_SYM_GLOBAL || batch->symbols[best].kind == RS_SYM_CONST) best = (int)i;
		} else if ((symbol->kind == RS_SYM_GLOBAL || symbol->kind == RS_SYM_CONST) && best < 0) {
			best = (int)i;
		}
	}
	if (best >= 0) {
		*type = batch->symbols[best].type;
		return 1;
	}
	{
		int index = rs_env_global_index(batch->env, name);
		if (index >= 0) {
			*type = batch->env->globals[index].type;
			return 1;
		}
	}
	return 0;
}

char* rs_batch_complete_json(rs_batch* batch, int unit, uint32_t offset) {
	rs_items items;
	const rs_unit* source;
	uint32_t start, i;
	rs_lexer lexer;
	memset(&items, 0, sizeof(items));
	if (unit < 0 || (uint32_t)unit >= batch->unit_count) return rs_items_take(&items);
	source = &batch->units[unit];
	if (offset > source->length) offset = source->length;
	/* nothing inside a string or a comment */
	rs_lexer_init(&lexer, source->text, source->length, 1);
	for (;;) {
		rs_token token = rs_lexer_next(&lexer);
		if (token.kind == TK_EOF || token.start >= offset) break;
		if ((token.kind == TK_STRING || token.kind == TK_COMMENT || (token.kind == TK_ERROR && (source->text[token.start] == '"' || source->text[token.start] == '\'' || source->text[token.start] == '/')))
			&& offset > token.start && (offset < token.end || (token.kind != TK_STRING && offset <= token.end && source->text[token.start + 1] == '/'))) {
			return rs_items_take(&items);
		}
	}
	/* the word being typed, and what is before it */
	start = offset;
	while (start > 0 && rs_is_ident_char((unsigned char)source->text[start - 1])) start--;
	{
		uint32_t before = start;
		while (before > 0 && (source->text[before - 1] == ' ' || source->text[before - 1] == '\t')) before--;
		/* a type after ':' */
		if (before > 0 && source->text[before - 1] == ':') {
			static const char* const types[] = { "number", "string", "bool", "void", "null" };
			for (i = 0; i < RS_COUNT_OF(types); i++) rs_item(&items, types[i], "type", NULL, NULL, NULL, 0);
			return rs_items_take(&items);
		}
		/* a name being declared: nothing to offer */
		if (before >= 3) {
			uint32_t word_end = before, word_start = before;
			while (word_start > 0 && rs_is_ident_char((unsigned char)source->text[word_start - 1])) word_start--;
			if ((word_end - word_start == 3 && memcmp(source->text + word_start, "let", 3) == 0) ||
				(word_end - word_start == 5 && memcmp(source->text + word_start, "const", 5) == 0) ||
				(word_end - word_start == 2 && memcmp(source->text + word_start, "fn", 2) == 0)) {
				return rs_items_take(&items);
			}
		}
	}
	if (start > 0 && source->text[start - 1] == '.') {
		/* the chain before the dot: "ui", "app", "counter", "math.vector" */
		uint32_t chain_start = start - 1;
		char chain[256];
		size_t length;
		rs_type type;
		while (chain_start > 0 && (rs_is_ident_char((unsigned char)source->text[chain_start - 1]) || source->text[chain_start - 1] == '.')) chain_start--;
		length = start - 1 - chain_start;
		if (length == 0 || length >= sizeof(chain)) {
			/* (a call or an index before the dot: its type is not known here) */
			return rs_items_take(&items);
		}
		memcpy(chain, source->text + chain_start, length);
		chain[length] = '\0';
		if (!strchr(chain, '.') && rs_variable_type(batch, unit, start, chain, &type)) {
			static const char* const methods[] = { "length", "toString", "push", "pop", "toUpperCase", "toLowerCase", "trim", "contains", "startsWith", "endsWith", "indexOf", "substring", "split", "toFixed" };
			for (i = 0; i < RS_COUNT_OF(methods); i++) {
				rs_sig sig;
				int method = rs_method_lookup(type, methods[i], &sig);
				rs_buf detail = { 0 };
				rs_buf insert = { 0 };
				if (method < 0) continue;
				if (rs_method_info_of(method)->property) {
					rs_buf_printf(&detail, "(property) %s: %s", methods[i], rs_type_name(sig.ret));
					rs_item(&items, methods[i], "property", detail.data, rs_method_info_of(method)->doc, NULL, 0);
				} else {
					rs_buf_printf(&detail, "(method) %s.%s", rs_type_name(type), methods[i]);
					rs_sig_format(&sig, &detail);
					rs_buf_printf(&insert, sig.count > 0 ? "%s($1)" : "%s()", methods[i]);
					rs_item(&items, methods[i], "method", detail.data, rs_method_info_of(method)->doc, insert.data, 1);
				}
				rs_buf_free(&detail);
				rs_buf_free(&insert);
			}
			return rs_items_take(&items);
		}
		rs_offer_functions(batch, &items, chain);
		return rs_items_take(&items);
	}
	/* a word: keywords, variables in scope, functions and namespaces */
	{
		static const char* const keywords[] = { "fn", "let", "const", "if", "else", "for", "while", "break", "continue", "return", "true", "false", "null", "declare" };
		for (i = 0; i < RS_COUNT_OF(keywords); i++) rs_item(&items, keywords[i], "keyword", NULL, NULL, NULL, 0);
	}
	for (i = 0; i < batch->symbol_count; i++) {
		const rs_symbol* symbol = &batch->symbols[i];
		const char* kind;
		rs_buf detail = { 0 };
		switch (symbol->kind) {
			case RS_SYM_LOCAL: case RS_SYM_LOCAL_CONST: case RS_SYM_PARAMETER:
				if (symbol->unit != unit || symbol->end > start || start > symbol->scope_end) continue;
				kind = symbol->kind == RS_SYM_PARAMETER ? "parameter" : (symbol->kind == RS_SYM_LOCAL_CONST ? "constant" : "variable");
				break;
			case RS_SYM_GLOBAL: kind = "variable"; break;
			case RS_SYM_CONST: kind = "constant"; break;
			default: continue;
		}
		/* (the word being typed is not a suggestion of itself) */
		if (symbol->unit == unit && symbol->start == start) continue;
		rs_buf_printf(&detail, "%s: %s", symbol->name, rs_type_name(symbol->type));
		rs_item(&items, symbol->name, kind, detail.data, NULL, NULL, 0);
		rs_buf_free(&detail);
	}
	for (i = 0; i < batch->env->global_count; i++) {
		const rs_global* global = &batch->env->globals[i];
		rs_buf detail = { 0 };
		rs_buf_printf(&detail, "%s: %s", global->name, rs_type_name(global->type));
		rs_item(&items, global->name, global->is_const ? "constant" : "variable", detail.data, NULL, NULL, 0);
		rs_buf_free(&detail);
	}
	rs_offer_functions(batch, &items, "");
	return rs_items_take(&items);
}
