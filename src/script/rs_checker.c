/**
 * Raven Script - script/rs_checker.c
 * Static type checking: resolves every name, gives every expression its
 * type, and reports what would go wrong before anything runs.
 *
 * Three passes over the batch:
 *   1. the declarations: functions (fn, declare fn) and globals, all units;
 *   2. the initializers of the globals, in order (they give inferred types);
 *   3. the bodies of the functions.
 */

#include "rs_internal.h"

typedef struct rs_var {
	const char* name;
	rs_type type;
	int is_const;
	int slot;
	int symbol;
	int unit;
	uint32_t start, end;
	int is_param;
} rs_var;

typedef struct rs_checker {
	rs_batch* batch;
	/* the function being checked (NULL: an initializer of a global) */
	rs_fn_info* function;
	int function_index;
	rs_type ret;
	/* locals: a stack of variables; scopes are marks in it */
	rs_var* vars;
	uint32_t var_count;
	uint32_t var_capacity;
	uint32_t* scopes;
	uint32_t scope_count;
	uint32_t scope_capacity;
	int next_slot;
	int max_slot;
	/* loops around the statement (break sets has_break on the innermost) */
	rs_node** loops;
	uint32_t loop_count;
	uint32_t loop_capacity;
	/* while checking the initializer of global `n` of the batch: later globals are not declared yet */
	int initializing;
	uint32_t initializing_index;
} rs_checker;

static rs_type rs_check_expression(rs_checker* checker, rs_node* node, rs_type expected);
static void rs_check_block(rs_checker* checker, rs_node* block, int new_scope);

static rs_type T(rs_base base) {
	return rs_type_make(base, 0);
}

static int rs_is(rs_type type, rs_base base) {
	return type.depth == 0 && type.base == base;
}

/* ---------------------------------------------------------------------------
 *  Analysis records
 * ------------------------------------------------------------------------- */

int rs_symbol_add(rs_batch* batch, int kind, const char* name, rs_type type, const rs_sig* sig, int unit, uint32_t start, uint32_t end) {
	rs_symbol* symbol;
	if (!(batch->flags & RS_BATCH_ANALYSIS)) return -1;
	RS_GROW(batch->symbols, batch->symbol_count, batch->symbol_capacity);
	symbol = &batch->symbols[batch->symbol_count];
	memset(symbol, 0, sizeof(*symbol));
	symbol->kind = kind;
	symbol->name = rs_strdup(name);
	symbol->type = type;
	if (sig) {
		symbol->sig = *sig;
		symbol->has_sig = 1;
	}
	symbol->unit = unit;
	symbol->start = start;
	symbol->end = end;
	symbol->scope_end = end;
	symbol->function = -1;
	return (int)batch->symbol_count++;
}

void rs_reference_add(rs_batch* batch, int symbol, int unit, uint32_t start, uint32_t end) {
	rs_reference* reference;
	if (!(batch->flags & RS_BATCH_ANALYSIS) || symbol < 0) return;
	RS_GROW(batch->references, batch->reference_count, batch->reference_capacity);
	reference = &batch->references[batch->reference_count++];
	reference->symbol = symbol;
	reference->unit = unit;
	reference->start = start;
	reference->end = end;
}

void rs_hover_add(rs_batch* batch, int unit, uint32_t start, uint32_t end, const char* format, ...) {
	rs_hover* hover;
	rs_buf text = { 0 };
	va_list arguments;
	if (!(batch->flags & RS_BATCH_ANALYSIS) || end <= start) return;
	va_start(arguments, format);
	rs_buf_vprintf(&text, format, arguments);
	va_end(arguments);
	RS_GROW(batch->hovers, batch->hover_count, batch->hover_capacity);
	hover = &batch->hovers[batch->hover_count++];
	hover->unit = unit;
	hover->start = start;
	hover->end = end;
	hover->text = rs_buf_take(&text);
}

/** The symbol of something declared outside the batch (a native, a function loaded before), made once. */
static int rs_symbol_outside(rs_batch* batch, int kind, const char* name, const rs_sig* sig, const char* doc) {
	uint32_t i;
	int id;
	if (!(batch->flags & RS_BATCH_ANALYSIS)) return -1;
	for (i = 0; i < batch->symbol_count; i++) {
		if (batch->symbols[i].unit == -1 && batch->symbols[i].kind == kind && strcmp(batch->symbols[i].name, name) == 0) return (int)i;
	}
	id = rs_symbol_add(batch, kind, name, sig ? sig->ret : T(RS_T_VOID), sig, -1, 0, 0);
	if (id >= 0) {
		batch->symbols[id].doc = doc;
		if (kind == RS_SYM_FUNCTION) {
			rs_location location;
			if (rs_env_function_location(batch->env, name, &location)) {
				batch->symbols[id].file = rs_strdup(location.file);
				batch->symbols[id].line = location.line;
			}
		}
	}
	return id;
}

static void rs_hover_function(rs_batch* batch, int unit, uint32_t start, uint32_t end, const char* prefix, const char* name, const rs_sig* sig, const char* doc) {
	rs_buf text = { 0 };
	if (!(batch->flags & RS_BATCH_ANALYSIS)) return;
	rs_buf_printf(&text, "%s%s", prefix, name);
	rs_sig_format(sig, &text);
	rs_hover_add(batch, unit, start, end, "```rvs\n%s\n```%s%s", text.data, doc && doc[0] ? "\n\n" : "", doc ? doc : "");
	rs_buf_free(&text);
}

/** Remembers a call (signature help): the parentheses, the commas, the parameters. */
static void rs_call_site_add(rs_checker* checker, rs_node* call, const char* label_name, const rs_sig* sig, const char* doc) {
	rs_batch* batch = checker->batch;
	rs_call_site* site;
	rs_buf label = { 0 };
	int i;
	if (!(batch->flags & RS_BATCH_ANALYSIS)) return;
	RS_GROW(batch->calls, batch->call_count, batch->call_capacity);
	site = &batch->calls[batch->call_count++];
	memset(site, 0, sizeof(*site));
	site->unit = call->unit;
	site->open = call->name_start;
	site->close = call->name_end;
	rs_buf_printf(&label, "%s(", label_name);
	for (i = 0; i < sig->count; i++) {
		if (i) rs_buf_puts(&label, ", ");
		site->param_start[i] = (int)label.length;
		if (sig->names[i] && sig->names[i][0]) rs_buf_printf(&label, "%s: ", sig->names[i]);
		rs_buf_puts(&label, rs_type_name(sig->params[i]));
		site->param_end[i] = (int)label.length;
	}
	rs_buf_printf(&label, "): %s", rs_type_name(sig->ret));
	site->param_count = sig->count;
	site->label = rs_buf_take(&label);
	site->doc = rs_strdup(doc ? doc : "");
	/* the commas between the arguments, from the source */
	{
		const rs_unit* unit = &batch->units[call->unit];
		uint32_t at, depth = 0, capacity = 0;
		int in_string = 0;
		char quote = 0;
		for (at = call->name_start + 1; at < call->name_end && at < unit->length; at++) {
			char c = unit->text[at];
			if (in_string) {
				if (c == '\\') at++;
				else if (c == quote) in_string = 0;
				continue;
			}
			if (c == '"' || c == '\'') {
				in_string = 1;
				quote = c;
			} else if (c == '(' || c == '[' || c == '{') {
				depth++;
			} else if (c == ')' || c == ']' || c == '}') {
				if (depth == 0) break;
				depth--;
			} else if (c == ',' && depth == 0) {
				RS_GROW(site->commas, site->comma_count, capacity);
				site->commas[site->comma_count++] = at;
			}
		}
	}
}

/* ---------------------------------------------------------------------------
 *  Scopes
 * ------------------------------------------------------------------------- */

static void rs_scope_push(rs_checker* checker) {
	RS_GROW(checker->scopes, checker->scope_count, checker->scope_capacity);
	checker->scopes[checker->scope_count++] = checker->var_count;
}

static void rs_scope_pop(rs_checker* checker, uint32_t scope_end) {
	uint32_t mark = checker->scopes[--checker->scope_count];
	rs_batch* batch = checker->batch;
	while (checker->var_count > mark) {
		rs_var* var = &checker->vars[--checker->var_count];
		if (var->symbol >= 0 && (batch->flags & RS_BATCH_ANALYSIS)) batch->symbols[var->symbol].scope_end = scope_end;
	}
}

static rs_var* rs_var_find(rs_checker* checker, const char* name) {
	uint32_t i = checker->var_count;
	while (i-- > 0) {
		if (strcmp(checker->vars[i].name, name) == 0) return &checker->vars[i];
	}
	return NULL;
}

/** The variable declared with that name in the innermost scope, or NULL. */
static rs_var* rs_var_in_scope(rs_checker* checker, const char* name) {
	uint32_t i, mark = checker->scope_count ? checker->scopes[checker->scope_count - 1] : 0;
	for (i = checker->var_count; i > mark; i--) {
		if (strcmp(checker->vars[i - 1].name, name) == 0) return &checker->vars[i - 1];
	}
	return NULL;
}

static rs_var* rs_var_declare(rs_checker* checker, const char* name, rs_type type, int is_const, int unit, uint32_t start, uint32_t end, int is_param) {
	rs_var* var;
	rs_var* existing = rs_var_in_scope(checker, name);
	if (existing) {
		rs_report_related(checker->batch, RS_SEVERITY_ERROR, "RS107", "Duplicate declaration.", unit, start, end,
			existing->unit, existing->start, existing->end, "It is declared here.", "Help: give it another name.",
			"'%s' is already declared in this scope.", name);
	}
	RS_GROW(checker->vars, checker->var_count, checker->var_capacity);
	var = &checker->vars[checker->var_count++];
	var->name = name;
	var->type = type;
	var->is_const = is_const;
	var->slot = checker->next_slot++;
	if (checker->next_slot > checker->max_slot) checker->max_slot = checker->next_slot;
	var->unit = unit;
	var->start = start;
	var->end = end;
	var->is_param = is_param;
	var->symbol = rs_symbol_add(checker->batch, is_param ? RS_SYM_PARAMETER : (is_const ? RS_SYM_LOCAL_CONST : RS_SYM_LOCAL), name, type, NULL, unit, start, end);
	if (var->symbol >= 0) {
		checker->batch->symbols[var->symbol].function = checker->function_index;
		rs_reference_add(checker->batch, var->symbol, unit, start, end);
		rs_hover_add(checker->batch, unit, start, end, "```rvs\n%s %s: %s\n```", is_param ? "(parameter)" : (is_const ? "const" : "let"), name, rs_type_name(type));
	}
	return var;
}

/* ---------------------------------------------------------------------------
 *  Names
 * ------------------------------------------------------------------------- */

static rs_global_info* rs_batch_global(rs_batch* batch, const char* name) {
	uint32_t i;
	for (i = 0; i < batch->global_count; i++) {
		if (strcmp(batch->globals[i].node->text, name) == 0) return &batch->globals[i];
	}
	return NULL;
}

static rs_fn_info* rs_batch_function(rs_batch* batch, const char* name) {
	uint32_t i;
	for (i = 0; i < batch->function_count; i++) {
		if (strcmp(batch->functions[i].name, name) == 0) return &batch->functions[i];
	}
	return NULL;
}

static int rs_edit_distance(const char* a, const char* b) {
	size_t la = strlen(a), lb = strlen(b), i, j;
	int row[65];
	if (la > 64 || lb > 64) return 99;
	for (j = 0; j <= lb; j++) row[j] = (int)j;
	for (i = 1; i <= la; i++) {
		int previous = row[0];
		row[0] = (int)i;
		for (j = 1; j <= lb; j++) {
			int cost = a[i - 1] == b[j - 1] ? 0 : 1;
			int best = row[j] + 1;
			if (row[j - 1] + 1 < best) best = row[j - 1] + 1;
			if (previous + cost < best) best = previous + cost;
			previous = row[j];
			row[j] = best;
		}
	}
	return row[lb];
}

static void rs_suggest(const char* wanted, const char* candidate, const char** best, int* best_distance) {
	int distance = rs_edit_distance(wanted, candidate);
	int limit = (int)strlen(wanted) / 3 + 1;
	if (limit > 3) limit = 3;
	if (distance <= limit && distance < *best_distance && strcmp(wanted, candidate) != 0) {
		*best = candidate;
		*best_distance = distance;
	}
}

/** A name close to `wanted` among the variables (functions = 0) or the functions (functions = 1) known here. */
static const char* rs_closest_name(rs_checker* checker, const char* wanted, int functions) {
	const char* best = NULL;
	int best_distance = 99;
	uint32_t i;
	rs_batch* batch = checker->batch;
	if (!functions) {
		for (i = 0; i < checker->var_count; i++) rs_suggest(wanted, checker->vars[i].name, &best, &best_distance);
		for (i = 0; i < batch->global_count; i++) rs_suggest(wanted, batch->globals[i].node->text, &best, &best_distance);
		for (i = 0; i < batch->env->global_count; i++) {
			rs_suggest(wanted, batch->env->globals[i].name, &best, &best_distance);
		}
	} else {
		int n;
		for (i = 0; i < batch->function_count; i++) rs_suggest(wanted, batch->functions[i].name, &best, &best_distance);
		for (i = 0; i < batch->env->function_count; i++) rs_suggest(wanted, batch->env->functions[i].name, &best, &best_distance);
		for (n = 0; n < rs_native_count(); n++) rs_suggest(wanted, rs_native_at(n)->name, &best, &best_distance);
		for (n = 0; n < RS_I_COUNT_; n++) rs_suggest(wanted, rs_intrinsic_info_of(n)->name, &best, &best_distance);
	}
	return best;
}

/** The dotted name of a chain of identifiers (a.b.c), or 0 when the expression is something else. */
static int rs_chain_path(const rs_node* node, rs_buf* out) {
	if (node->kind == RS_N_NAME) {
		rs_buf_puts(out, node->text);
		return 1;
	}
	if (node->kind == RS_N_MEMBER && node->a) {
		if (!rs_chain_path(node->a, out)) return 0;
		rs_buf_putc(out, '.');
		rs_buf_puts(out, node->text);
		return 1;
	}
	return 0;
}

static const rs_node* rs_chain_root(const rs_node* node) {
	while (node->kind == RS_N_MEMBER && node->a) node = node->a;
	return node;
}

/** Is `name` a variable here (a local, a global of the batch declared already, a global loaded before)? */
static int rs_is_variable(rs_checker* checker, const char* name) {
	rs_global_info* global;
	if (rs_var_find(checker, name)) return 1;
	global = rs_batch_global(checker->batch, name);
	if (global) return 1;
	return rs_env_global_index(checker->batch->env, name) >= 0;
}

/* ---------------------------------------------------------------------------
 *  Expressions
 * ------------------------------------------------------------------------- */

static rs_type rs_error_type(void) {
	return T(RS_T_ERROR);
}

static rs_type rs_check_name(rs_checker* checker, rs_node* node) {
	rs_batch* batch = checker->batch;
	rs_var* var = rs_var_find(checker, node->text);
	rs_global_info* global;
	int index;
	if (var) {
		node->resolved = RS_R_LOCAL;
		node->slot = var->slot;
		rs_reference_add(batch, var->symbol, node->unit, node->start, node->end);
		rs_hover_add(batch, node->unit, node->start, node->end, "```rvs\n%s %s: %s\n```", var->is_param ? "(parameter)" : (var->is_const ? "const" : "let"), var->name, rs_type_name(var->type));
		return node->type = var->type;
	}
	global = rs_batch_global(batch, node->text);
	if (global) {
		uint32_t at = (uint32_t)(global - batch->globals);
		if (checker->initializing && at >= checker->initializing_index) {
			rs_report_related(batch, RS_SEVERITY_ERROR, "RS101", "Unknown function or variable.", node->unit, node->start, node->end,
				global->node->unit, global->node->name_start, global->node->name_end, "It is declared here.", "Help: declare it before.",
				at == checker->initializing_index ? "'%s' is used in its own initializer." : "'%s' is used before its declaration.", node->text);
			return node->type = rs_error_type();
		}
		node->resolved = RS_R_GLOBAL;
		node->slot = (int)at;
		node->batch_global = 1;
		rs_reference_add(batch, global->symbol, node->unit, node->start, node->end);
		rs_hover_add(batch, node->unit, node->start, node->end, "```rvs\n%s %s: %s\n```", global->node->op ? "const" : "let", node->text, rs_type_name(global->node->type));
		return node->type = global->node->type;
	}
	index = rs_env_global_index(batch->env, node->text);
	if (index >= 0) {
		const rs_global* g = &batch->env->globals[index];
		int symbol = rs_symbol_outside(batch, g->is_const ? RS_SYM_CONST : RS_SYM_GLOBAL, g->name, NULL, NULL);
		if (symbol >= 0) batch->symbols[symbol].type = g->type;
		node->resolved = RS_R_GLOBAL;
		node->slot = index;
		node->batch_global = 0;
		rs_reference_add(batch, symbol, node->unit, node->start, node->end);
		rs_hover_add(batch, node->unit, node->start, node->end, "```rvs\n%s %s: %s\n```", g->is_const ? "const" : "let", g->name, rs_type_name(g->type));
		return node->type = g->type;
	}
	{
		const char* close = rs_closest_name(checker, node->text, 0);
		rs_buf help = { 0 };
		if (close) rs_buf_printf(&help, "Help: did you mean '%s'?", close);
		if (rs_batch_function(batch, node->text) || rs_env_function_index(batch->env, node->text) >= 0) {
			rs_report(batch, RS_SEVERITY_ERROR, "RS101", "Unknown function or variable.", node->unit, node->start, node->end, "Help: functions are not values: call it with ().", "'%s' is a function, not a variable.", node->text);
		} else {
			rs_report(batch, RS_SEVERITY_ERROR, "RS101", "Unknown function or variable.", node->unit, node->start, node->end, help.data, "There is no variable named '%s'.", node->text);
		}
		rs_buf_free(&help);
	}
	return node->type = rs_error_type();
}

static void rs_check_arguments(rs_checker* checker, rs_node* call, const char* name, const rs_sig* sig) {
	uint32_t i;
	rs_batch* batch = checker->batch;
	if (call->count != (uint32_t)sig->count) {
		rs_report(batch, RS_SEVERITY_ERROR, "RS106", "Invalid argument count.", call->unit, call->start, call->end, NULL,
			"Function '%s' expects %d argument%s,\nbut %u %s provided.", name, sig->count, sig->count == 1 ? "" : "s", call->count, call->count == 1 ? "was" : "were");
	}
	for (i = 0; i < call->count; i++) {
		rs_type expected = i < (uint32_t)sig->count ? sig->params[i] : rs_error_type();
		rs_type got = rs_check_expression(checker, call->list[i], expected);
		if (i < (uint32_t)sig->count && !rs_type_assignable(expected, got)) {
			rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Argument type mismatch.", call->list[i]->unit, call->list[i]->start, call->list[i]->end,
				rs_is(expected, RS_T_STRING) && rs_is(got, RS_T_NUMBER) ? "Help: convert it with .toString()." :
				(rs_is(expected, RS_T_NUMBER) && rs_is(got, RS_T_STRING) ? "Help: convert it with number.parse()." : NULL),
				"Argument %u of '%s' expects '%s',\nbut got '%s'.", i + 1, name, rs_type_name(expected), rs_type_name(got));
		}
	}
}

/* ui.* natives: a literal name that no widget has gets a warning (RS301). */
static void rs_check_element(rs_checker* checker, rs_node* call, const char* name, const rs_sig* sig) {
	rs_batch* batch = checker->batch;
	rs_node* argument;
	if (strncmp(name, "ui.", 3) != 0 || sig->count == 0 || !sig->names[0] || strcmp(sig->names[0], "id") != 0) return;
	/* (ui.exists asks the question: no warning) */
	if (strcmp(name, "ui.exists") == 0) return;
	if (call->count == 0 || !batch->env->host.element_exists) return;
	argument = call->list[0];
	if (argument->kind != RS_N_STRING) return;
	if (batch->env->host.element_exists(batch->env->host.user_data, argument->text) == 0) {
		rs_report(batch, RS_SEVERITY_WARNING, "RS301", "Unknown UI element.", argument->unit, argument->start, argument->end,
			"Help: give the widget this name in the XML: Name=\"...\".", "No widget is named '%s' in this document.", argument->text);
	}
}

static rs_type rs_check_call(rs_checker* checker, rs_node* node) {
	rs_batch* batch = checker->batch;
	rs_node* callee = node->a;
	rs_buf path = { 0 };
	rs_sig sig;
	memset(&sig, 0, sizeof(sig));

	/* a method of a value: receiver.name(...) */
	if (callee->kind == RS_N_MEMBER) {
		const rs_node* root = rs_chain_root(callee);
		int is_method = !(root->kind == RS_N_NAME && !rs_is_variable(checker, root->text));
		if (is_method) {
			rs_type receiver = rs_check_expression(checker, callee->a, rs_error_type());
			int method;
			if (rs_type_is_error(receiver)) {
				uint32_t i;
				for (i = 0; i < node->count; i++) rs_check_expression(checker, node->list[i], rs_error_type());
				return node->type = rs_error_type();
			}
			method = rs_method_lookup(receiver, callee->text, &sig);
			if (method < 0) {
				uint32_t i;
				if (callee->text[0]) {
					rs_report(batch, RS_SEVERITY_ERROR, "RS101", "Unknown function or variable.", callee->unit, callee->name_start, callee->name_end, NULL,
						"A value of type '%s' has no method '%s'.", rs_type_name(receiver), callee->text);
				}
				for (i = 0; i < node->count; i++) rs_check_expression(checker, node->list[i], rs_error_type());
				return node->type = rs_error_type();
			}
			if (rs_method_info_of(method)->property) {
				rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", node->unit, callee->name_start, node->end, "Help: remove the parentheses.",
					"'%s' is a property, not a method: write .%s.", callee->text, callee->text);
				return node->type = sig.ret;
			}
			{
				rs_buf label = { 0 };
				rs_buf_printf(&label, "%s.%s", rs_type_name(receiver), callee->text);
				if (method == RS_M_PUSH && node->count == 1) {
					/* the message of the specification (10) */
					rs_type got = rs_check_expression(checker, node->list[0], sig.params[0]);
					if (!rs_type_assignable(sig.params[0], got)) {
						rs_buf array = { 0 };
						if (!rs_chain_path(callee->a, &array)) rs_buf_puts(&array, "this array");
						rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Array type mismatch.", node->list[0]->unit, node->list[0]->start, node->list[0]->end, NULL,
							"Expected '%s', but got '%s'.\n\nArray '%s' only accepts values of type '%s'.", rs_type_name(sig.params[0]), rs_type_name(got), array.data, rs_type_name(sig.params[0]));
						rs_buf_free(&array);
					}
				} else {
					rs_check_arguments(checker, node, label.data, &sig);
				}
				rs_hover_function(batch, callee->unit, callee->name_start, callee->name_end, "(method) ", label.data, &sig, rs_method_info_of(method)->doc);
				rs_call_site_add(checker, node, label.data, &sig, rs_method_info_of(method)->doc);
				rs_buf_free(&label);
			}
			node->resolved = RS_R_METHOD;
			node->slot = method;
			return node->type = sig.ret;
		}
	}

	if (!rs_chain_path(callee, &path)) {
		uint32_t i;
		rs_check_expression(checker, callee, rs_error_type());
		rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", callee->unit, callee->start, callee->end, NULL, "This expression cannot be called: only functions can.");
		for (i = 0; i < node->count; i++) rs_check_expression(checker, node->list[i], rs_error_type());
		rs_buf_free(&path);
		return node->type = rs_error_type();
	}
	if (callee->kind == RS_N_NAME && rs_is_variable(checker, callee->text)) {
		uint32_t i;
		rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", callee->unit, callee->start, callee->end, NULL, "'%s' is a variable, not a function.", callee->text);
		for (i = 0; i < node->count; i++) rs_check_expression(checker, node->list[i], rs_error_type());
		rs_buf_free(&path);
		return node->type = rs_error_type();
	}
	node->path = rs_arena_strndup(&batch->arena, path.data, path.length);
	rs_buf_free(&path);
	{
		const char* name = node->path;
		/* the range of the whole dotted name */
		uint32_t name_start = callee->start, name_end = callee->kind == RS_N_MEMBER ? callee->name_end : callee->end;
		rs_fn_info* function = rs_batch_function(batch, name);
		const rs_native* native;
		int intrinsic;
		if (function) {
			sig = function->sig;
			node->resolved = function->node->is_declare ? RS_R_EXTERNAL : RS_R_FUNCTION;
			rs_reference_add(batch, function->symbol, callee->unit, name_start, name_end);
			rs_hover_function(batch, callee->unit, name_start, name_end, function->node->is_declare ? "declare fn " : "fn ", name, &sig, NULL);
			rs_call_site_add(checker, node, name, &sig, NULL);
		} else if (rs_env_find_function(batch->env, name, &sig)) {
			node->resolved = RS_R_FUNCTION;
			rs_reference_add(batch, rs_symbol_outside(batch, RS_SYM_FUNCTION, name, &sig, NULL), callee->unit, name_start, name_end);
			rs_hover_function(batch, callee->unit, name_start, name_end, "fn ", name, &sig, NULL);
			rs_call_site_add(checker, node, name, &sig, NULL);
		} else if ((intrinsic = rs_intrinsic_find(name, &sig)) >= 0) {
			node->resolved = RS_R_INTRINSIC;
			node->slot = intrinsic;
			rs_reference_add(batch, rs_symbol_outside(batch, RS_SYM_NATIVE, name, &sig, rs_intrinsic_info_of(intrinsic)->doc), callee->unit, name_start, name_end);
			rs_hover_function(batch, callee->unit, name_start, name_end, "fn ", name, &sig, rs_intrinsic_info_of(intrinsic)->doc);
			rs_call_site_add(checker, node, name, &sig, rs_intrinsic_info_of(intrinsic)->doc);
		} else if ((native = rs_native_find(name, &sig)) != NULL) {
			node->resolved = RS_R_EXTERNAL;
			rs_reference_add(batch, rs_symbol_outside(batch, RS_SYM_NATIVE, name, &sig, native->doc), callee->unit, name_start, name_end);
			rs_hover_function(batch, callee->unit, name_start, name_end, "(native) fn ", name, &sig, native->doc);
			rs_call_site_add(checker, node, name, &sig, native->doc);
			rs_check_element(checker, node, name, &sig);
		} else {
			rs_external_kind kind = RS_EXTERNAL_NONE;
			if (batch->env->host.lookup) kind = batch->env->host.lookup(batch->env->host.user_data, name, &sig);
			if (kind == RS_EXTERNAL_NONE) {
				uint32_t i;
				const char* close = rs_closest_name(checker, name, 1);
				rs_buf help = { 0 };
				if (close) rs_buf_printf(&help, "Help: did you mean '%s'?", close);
				else rs_buf_printf(&help, "Help: if the application binds it in C, declare it: declare fn %s(...);", name);
				rs_report(batch, RS_SEVERITY_ERROR, "RS101", "Unknown function or variable.", callee->unit, name_start, name_end, help.data, "There is no function named '%s'.", name);
				rs_buf_free(&help);
				for (i = 0; i < node->count; i++) rs_check_expression(checker, node->list[i], rs_error_type());
				return node->type = rs_error_type();
			}
			if (kind == RS_EXTERNAL_ACTION) {
				memset(&sig, 0, sizeof(sig));
				sig.ret = T(RS_T_VOID);
			}
			node->resolved = RS_R_EXTERNAL;
			rs_reference_add(batch, rs_symbol_outside(batch, RS_SYM_EXTERNAL, name, &sig, "Bound by the application."), callee->unit, name_start, name_end);
			rs_hover_function(batch, callee->unit, name_start, name_end, "(action) fn ", name, &sig, "Bound by the application.");
			rs_call_site_add(checker, node, name, &sig, NULL);
		}
		rs_check_arguments(checker, node, name, &sig);
		return node->type = sig.ret;
	}
}

static rs_type rs_check_member(rs_checker* checker, rs_node* node) {
	rs_batch* batch = checker->batch;
	const rs_node* root = rs_chain_root(node);
	rs_type receiver;
	rs_sig sig;
	int method;
	if (root->kind == RS_N_NAME && !rs_is_variable(checker, root->text)) {
		/* a.b without a call: a function without its parentheses, or an unknown name */
		rs_buf path = { 0 };
		rs_sig ignored;
		rs_chain_path(node, &path);
		if (rs_batch_function(batch, path.data) || rs_env_find_function(batch->env, path.data, &ignored) || rs_native_find(path.data, NULL) || rs_intrinsic_find(path.data, NULL) >= 0) {
			rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", node->unit, node->start, node->end, "Help: add the parentheses: a function is called.",
				"'%s' is a function: functions are not values.", path.data);
		} else if (!node->text[0]) {
			/* "app." being typed: the parser said what is missing */
		} else {
			rs_report(batch, RS_SEVERITY_ERROR, "RS101", "Unknown function or variable.", root->unit, root->start, root->end, NULL, "There is no variable named '%s'.", root->text);
		}
		rs_buf_free(&path);
		return node->type = rs_error_type();
	}
	receiver = rs_check_expression(checker, node->a, rs_error_type());
	if (rs_type_is_error(receiver)) return node->type = rs_error_type();
	method = rs_method_lookup(receiver, node->text, &sig);
	if (method < 0) {
		if (node->text[0]) {
			rs_report(batch, RS_SEVERITY_ERROR, "RS101", "Unknown function or variable.", node->unit, node->name_start, node->name_end, NULL,
				"A value of type '%s' has no property '%s'.", rs_type_name(receiver), node->text);
		}
		return node->type = rs_error_type();
	}
	if (!rs_method_info_of(method)->property) {
		rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", node->unit, node->name_start, node->name_end, "Help: add the parentheses.",
			"'%s' is a method: call it with %s().", node->text, node->text);
		return node->type = rs_error_type();
	}
	node->resolved = RS_R_METHOD;
	node->slot = method;
	rs_hover_add(batch, node->unit, node->name_start, node->name_end, "```rvs\n(property) %s.%s: %s\n```\n\n%s", rs_type_name(receiver), node->text, rs_type_name(sig.ret), rs_method_info_of(method)->doc);
	return node->type = sig.ret;
}

static rs_type rs_check_array(rs_checker* checker, rs_node* node, rs_type expected) {
	rs_batch* batch = checker->batch;
	rs_type element = rs_error_type();
	int known = 0;
	uint32_t i;
	if (rs_type_is_array(expected)) {
		element = rs_type_element(expected);
		known = 1;
	}
	if (node->count == 0) {
		if (!known) {
			if (rs_type_is_error(expected)) {
				rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", node->unit, node->start, node->end, "Help: give the type: let items: number[] = [];",
					"The type of an empty array cannot be inferred.");
			}
			return node->type = rs_error_type();
		}
		return node->type = expected;
	}
	for (i = 0; i < node->count; i++) {
		rs_type item = rs_check_expression(checker, node->list[i], known ? element : rs_error_type());
		if (rs_is(item, RS_T_VOID)) {
			rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", node->list[i]->unit, node->list[i]->start, node->list[i]->end, NULL, "A function without a value (void) cannot be an item of an array.");
			continue;
		}
		if (rs_type_is_error(item)) continue;
		if (!known) {
			if (item.depth >= RS_MAX_ARRAY_DEPTH) {
				rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", node->unit, node->start, node->end, NULL, "Arrays are nested %d levels deep at most.", RS_MAX_ARRAY_DEPTH);
				return node->type = rs_error_type();
			}
			element = item;
			known = 1;
		} else if (!rs_type_assignable(element, item)) {
			rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Array type mismatch.", node->list[i]->unit, node->list[i]->start, node->list[i]->end, NULL,
				"Expected '%s', but got '%s'.\n\nThe items of an array all have the same type.", rs_type_name(element), rs_type_name(item));
		}
	}
	if (!known || rs_type_is_error(element)) return node->type = rs_error_type();
	return node->type = rs_type_make((rs_base)element.base, element.depth + 1);
}

static const char* rs_operator_verb(int op) {
	switch (op) {
		case TK_PLUS: return "add";
		case TK_MINUS: return "subtract";
		case TK_STAR: return "multiply";
		case TK_SLASH: return "divide";
		case TK_PERCENT: return "take the remainder of";
		default: return "combine";
	}
}

/** The type of `left op right`, or the error type after a report. `at` gives the range reported. */
static rs_type rs_binary_type(rs_checker* checker, int op, rs_type left, rs_type right, const rs_node* at, uint32_t op_start, uint32_t op_end) {
	rs_batch* batch = checker->batch;
	RS_UNUSED(op_start);
	RS_UNUSED(op_end);
	if (rs_type_is_error(left) || rs_type_is_error(right)) {
		if (op == TK_EQ || op == TK_NE || op == TK_LT || op == TK_LE || op == TK_GT || op == TK_GE || op == TK_AND || op == TK_OR) return T(RS_T_BOOL);
		if (op == TK_PLUS) return rs_is(left, RS_T_STRING) || rs_is(right, RS_T_STRING) ? T(RS_T_STRING) : (rs_is(left, RS_T_NUMBER) || rs_is(right, RS_T_NUMBER) ? T(RS_T_NUMBER) : rs_error_type());
		return T(RS_T_NUMBER);
	}
	switch (op) {
		case TK_PLUS:
			if (rs_is(left, RS_T_NUMBER) && rs_is(right, RS_T_NUMBER)) return T(RS_T_NUMBER);
			if (rs_is(left, RS_T_STRING) && rs_is(right, RS_T_STRING)) return T(RS_T_STRING);
			rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Invalid operation.", at->unit, at->start, at->end,
				rs_is(left, RS_T_NUMBER) && rs_is(right, RS_T_STRING) ? "Hint: Convert the string to a number first (number.parse), or the number to a string (.toString())." :
				(rs_is(left, RS_T_STRING) && rs_is(right, RS_T_NUMBER) ? "Hint: Convert the number to a string first: value.toString()." :
				(rs_is(left, RS_T_STRING) || rs_is(right, RS_T_STRING) ? "Hint: Convert the value to a string first: value.toString()." : NULL)),
				"Cannot add values of type '%s' and '%s'.", rs_type_name(left), rs_type_name(right));
			return rs_error_type();
		case TK_MINUS:
		case TK_STAR:
		case TK_SLASH:
		case TK_PERCENT:
			if (rs_is(left, RS_T_NUMBER) && rs_is(right, RS_T_NUMBER)) return T(RS_T_NUMBER);
			rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Invalid operation.", at->unit, at->start, at->end, NULL,
				"Cannot %s values of type '%s' and '%s'.", rs_operator_verb(op), rs_type_name(left), rs_type_name(right));
			return rs_error_type();
		case TK_EQ:
		case TK_NE:
			if (rs_type_equal(left, right) || rs_is(left, RS_T_NULL) || rs_is(right, RS_T_NULL)) {
				if (rs_is(left, RS_T_VOID) || rs_is(right, RS_T_VOID)) {
					rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Invalid operation.", at->unit, at->start, at->end, NULL, "A function without a value (void) cannot be compared.");
				}
				return T(RS_T_BOOL);
			}
			rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Invalid operation.", at->unit, at->start, at->end, "Help: convert one of them first.",
				"Cannot compare values of type '%s' and '%s'.", rs_type_name(left), rs_type_name(right));
			return T(RS_T_BOOL);
		case TK_LT:
		case TK_LE:
		case TK_GT:
		case TK_GE:
			if ((rs_is(left, RS_T_NUMBER) && rs_is(right, RS_T_NUMBER)) || (rs_is(left, RS_T_STRING) && rs_is(right, RS_T_STRING))) return T(RS_T_BOOL);
			rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Invalid operation.", at->unit, at->start, at->end, NULL,
				"Cannot order values of type '%s' and '%s': numbers and strings can be ordered.", rs_type_name(left), rs_type_name(right));
			return T(RS_T_BOOL);
		case TK_AND:
		case TK_OR:
			if (rs_is(left, RS_T_BOOL) && rs_is(right, RS_T_BOOL)) return T(RS_T_BOOL);
			rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Invalid operation.", at->unit, at->start, at->end, "Hint: Use an explicit comparison.",
				"'%s' takes two values of type 'bool', but got '%s' and '%s'.", op == TK_AND ? "&&" : "||", rs_type_name(left), rs_type_name(right));
			return T(RS_T_BOOL);
		default:
			return rs_error_type();
	}
}

static rs_type rs_check_expression(rs_checker* checker, rs_node* node, rs_type expected) {
	rs_batch* batch = checker->batch;
	if (!node) return rs_error_type();
	switch (node->kind) {
		case RS_N_NUMBER: return node->type = T(RS_T_NUMBER);
		case RS_N_STRING: return node->type = T(RS_T_STRING);
		case RS_N_BOOL: return node->type = T(RS_T_BOOL);
		case RS_N_NULL: return node->type = T(RS_T_NULL);
		case RS_N_NAME: return rs_check_name(checker, node);
		case RS_N_MEMBER: return rs_check_member(checker, node);
		case RS_N_CALL: return rs_check_call(checker, node);
		case RS_N_ARRAY: return rs_check_array(checker, node, expected);
		case RS_N_INDEX: {
			rs_type array = rs_check_expression(checker, node->a, rs_error_type());
			rs_type index = rs_check_expression(checker, node->b, T(RS_T_NUMBER));
			if (!rs_type_is_error(index) && !rs_is(index, RS_T_NUMBER)) {
				rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", node->b->unit, node->b->start, node->b->end, NULL, "An index is a 'number', but got '%s'.", rs_type_name(index));
			}
			if (rs_type_is_error(array)) return node->type = rs_error_type();
			if (!rs_type_is_array(array)) {
				rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", node->a->unit, node->a->start, node->a->end,
					rs_is(array, RS_T_STRING) ? "Help: take a character with .substring(i, i + 1)." : NULL, "Only arrays can be indexed, but this is a '%s'.", rs_type_name(array));
				return node->type = rs_error_type();
			}
			return node->type = rs_type_element(array);
		}
		case RS_N_UNARY: {
			rs_type operand = rs_check_expression(checker, node->a, rs_error_type());
			if (node->op == TK_BANG) {
				if (!rs_type_is_error(operand) && !rs_is(operand, RS_T_BOOL)) {
					rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Invalid operation.", node->unit, node->start, node->end, "Hint: Use an explicit comparison.",
						"'!' takes a value of type 'bool', but got '%s'.", rs_type_name(operand));
				}
				return node->type = T(RS_T_BOOL);
			}
			if (!rs_type_is_error(operand) && !rs_is(operand, RS_T_NUMBER)) {
				rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Invalid operation.", node->unit, node->start, node->end, NULL,
					"'-' takes a value of type 'number', but got '%s'.", rs_type_name(operand));
			}
			return node->type = T(RS_T_NUMBER);
		}
		case RS_N_BINARY: {
			rs_type left = rs_check_expression(checker, node->a, rs_error_type());
			rs_type right = rs_check_expression(checker, node->b, rs_type_is_array(left) ? left : rs_error_type());
			return node->type = rs_binary_type(checker, node->op, left, right, node, node->name_start, node->name_end);
		}
		default:
			return node->type = rs_error_type();
	}
}

/** A value used where it must be something: void is refused. */
static rs_type rs_check_value(rs_checker* checker, rs_node* node, rs_type expected, const char* what) {
	rs_type type = rs_check_expression(checker, node, expected);
	if (rs_is(type, RS_T_VOID)) {
		rs_report(checker->batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", node->unit, node->start, node->end, NULL, "This function returns nothing (void): it cannot be %s.", what);
		return rs_error_type();
	}
	return type;
}

static void rs_check_condition(rs_checker* checker, rs_node* node) {
	rs_type type = rs_check_value(checker, node, T(RS_T_BOOL), "a condition");
	if (!rs_type_is_error(type) && !rs_is(type, RS_T_BOOL)) {
		rs_report(checker->batch, RS_SEVERITY_ERROR, "RS102", "Invalid condition.", node->unit, node->start, node->end, "Hint: Use an explicit comparison.",
			"Expected 'bool', but got '%s'.", rs_type_name(type));
	}
}

/* ---------------------------------------------------------------------------
 *  Statements
 * ------------------------------------------------------------------------- */

static void rs_report_mismatch(rs_checker* checker, rs_node* value, rs_type expected, rs_type got) {
	const char* help = NULL;
	if (rs_is(expected, RS_T_NUMBER) && rs_is(got, RS_T_STRING)) help = value->kind == RS_N_STRING ? "Help: Remove the quotes or convert the value\nto a number." : "Help: convert it with number.parse().";
	else if (rs_is(expected, RS_T_STRING) && (rs_is(got, RS_T_NUMBER) || rs_is(got, RS_T_BOOL))) help = "Help: convert it with .toString().";
	rs_report(checker->batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", value->unit, value->start, value->end, help,
		"Expected '%s', but got '%s'.", rs_type_name(expected), rs_type_name(got));
}

/** let / const: the type of the variable (from its annotation or its value). */
static rs_type rs_check_declaration_type(rs_checker* checker, rs_node* node) {
	rs_type type = node->has_annotation ? node->annotation : rs_error_type();
	if (node->has_annotation && rs_is(type, RS_T_VOID)) {
		rs_report(checker->batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", node->unit, node->annotation_start, node->annotation_end, NULL, "A variable cannot be of type 'void'.");
		type = rs_error_type();
	}
	if (node->a) {
		rs_type value = rs_check_value(checker, node->a, type, "a value");
		if (node->has_annotation) {
			if (!rs_type_assignable(type, value)) rs_report_mismatch(checker, node->a, type, value);
		} else {
			type = value;
		}
	} else if (node->op) {
		rs_report(checker->batch, RS_SEVERITY_ERROR, "RS002", "Expected token.", node->unit, node->name_start, node->name_end, "Help: write const name = value;",
			"The constant '%s' must be given a value.", node->text);
	} else if (!node->has_annotation) {
		rs_report(checker->batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", node->unit, node->name_start, node->name_end, "Help: write let name: number; or let name = 0;",
			"The variable '%s' needs a type or a value.", node->text);
	}
	return type;
}

/* error[RS105]: Cannot assign to constant 'counter'. (the title names it, as in the specification) */
static void rs_report_constant(rs_checker* checker, const rs_node* target, int related_unit, uint32_t related_start, uint32_t related_end) {
	rs_buf title = { 0 };
	rs_buf_printf(&title, "Cannot assign to constant '%s'.", target->text);
	rs_report_related(checker->batch, RS_SEVERITY_ERROR, "RS105", title.data, target->unit, target->start, target->end,
		related_unit, related_start, related_end, "", NULL, "'%s' was declared as a constant.", target->text);
	rs_buf_free(&title);
}

static void rs_check_assign(rs_checker* checker, rs_node* node) {
	rs_batch* batch = checker->batch;
	rs_node* target = node->a;
	rs_type target_type;
	rs_type value;
	if (target->kind == RS_N_NAME) {
		rs_var* var = rs_var_find(checker, target->text);
		target_type = rs_check_name(checker, target);
		if (var && var->is_const) {
			rs_report_constant(checker, target, var->unit, var->start, var->end);
		} else if (!var && target->resolved == RS_R_GLOBAL) {
			int is_const = 0;
			int related_unit = -1;
			uint32_t related_start = 0, related_end = 0;
			if (target->batch_global) {
				rs_global_info* global = &batch->globals[target->slot];
				is_const = global->node->op;
				related_unit = global->node->unit;
				related_start = global->node->name_start;
				related_end = global->node->name_end;
			} else {
				is_const = batch->env->globals[target->slot].is_const;
			}
			if (is_const) rs_report_constant(checker, target, related_unit, related_start, related_end);
		}
	} else if (target->kind == RS_N_INDEX) {
		target_type = rs_check_expression(checker, target, rs_error_type());
	} else {
		rs_check_expression(checker, target, rs_error_type());
		rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", target->unit, target->start, target->end, NULL,
			target->kind == RS_N_MEMBER ? "A property cannot be assigned." : "Only a variable or an item of an array can be assigned.");
		rs_check_expression(checker, node->b, rs_error_type());
		return;
	}
	value = rs_check_value(checker, node->b, target_type, "a value");
	if (node->op == TK_ASSIGN) {
		if (!rs_type_assignable(target_type, value)) rs_report_mismatch(checker, node->b, target_type, value);
		return;
	}
	{
		int op = node->op == TK_PLUS_ASSIGN ? TK_PLUS : node->op == TK_MINUS_ASSIGN ? TK_MINUS : node->op == TK_STAR_ASSIGN ? TK_STAR : node->op == TK_SLASH_ASSIGN ? TK_SLASH : TK_PERCENT;
		rs_type result = rs_binary_type(checker, op, target_type, value, node, node->name_start, node->name_end);
		if (!rs_type_is_error(result) && !rs_type_assignable(target_type, result)) rs_report_mismatch(checker, node, target_type, result);
	}
}

static int rs_is_true_literal(const rs_node* node) {
	return !node || (node->kind == RS_N_BOOL && node->number != 0.0);
}

/** Whether a statement returns on every path (a loop that never ends counts). */
static int rs_always_returns(const rs_node* node) {
	uint32_t i;
	if (!node) return 0;
	switch (node->kind) {
		case RS_N_RETURN: return 1;
		case RS_N_BLOCK:
			for (i = 0; i < node->count; i++) {
				if (rs_always_returns(node->list[i])) return 1;
			}
			return 0;
		case RS_N_IF: return node->c && rs_always_returns(node->b) && rs_always_returns(node->c);
		case RS_N_WHILE: return rs_is_true_literal(node->a) && !node->has_break;
		case RS_N_FOR: return rs_is_true_literal(node->b) && !node->has_break;
		default: return 0;
	}
}

static void rs_check_statement(rs_checker* checker, rs_node* node) {
	rs_batch* batch = checker->batch;
	switch (node->kind) {
		case RS_N_LET: {
			rs_type type = rs_check_declaration_type(checker, node);
			rs_var* var = rs_var_declare(checker, node->text, type, node->op, node->unit, node->name_start, node->name_end, 0);
			node->type = type;
			node->slot = var->slot;
			break;
		}
		case RS_N_ASSIGN:
			rs_check_assign(checker, node);
			break;
		case RS_N_EXPR: {
			rs_node* expression = node->a;
			rs_check_expression(checker, expression, rs_error_type());
			if (expression->kind != RS_N_CALL) {
				rs_report(batch, RS_SEVERITY_WARNING, "RS111", "Unused expression.", expression->unit, expression->start, expression->end, NULL, "This expression does nothing: its value is not used.");
			}
			break;
		}
		case RS_N_IF:
			rs_check_condition(checker, node->a);
			rs_check_block(checker, node->b, 1);
			if (node->c) {
				if (node->c->kind == RS_N_IF) rs_check_statement(checker, node->c);
				else rs_check_block(checker, node->c, 1);
			}
			break;
		case RS_N_WHILE:
			rs_check_condition(checker, node->a);
			RS_GROW(checker->loops, checker->loop_count, checker->loop_capacity);
			checker->loops[checker->loop_count++] = node;
			rs_check_block(checker, node->b, 1);
			checker->loop_count--;
			break;
		case RS_N_FOR:
			rs_scope_push(checker);
			if (node->a) rs_check_statement(checker, node->a);
			if (node->b) rs_check_condition(checker, node->b);
			RS_GROW(checker->loops, checker->loop_count, checker->loop_capacity);
			checker->loops[checker->loop_count++] = node;
			if (node->c) rs_check_statement(checker, node->c);
			rs_check_block(checker, node->d, 1);
			checker->loop_count--;
			rs_scope_pop(checker, node->end);
			break;
		case RS_N_BREAK:
		case RS_N_CONTINUE:
			if (checker->loop_count == 0) {
				rs_report(batch, RS_SEVERITY_ERROR, "RS110", "Invalid jump.", node->unit, node->start, node->end, NULL,
					"'%s' is only allowed inside a loop (for, while).", node->kind == RS_N_BREAK ? "break" : "continue");
			} else if (node->kind == RS_N_BREAK) {
				checker->loops[checker->loop_count - 1]->has_break = 1;
			}
			break;
		case RS_N_RETURN:
			if (rs_is(checker->ret, RS_T_VOID)) {
				if (node->a) {
					rs_type value = rs_check_expression(checker, node->a, rs_error_type());
					if (!rs_is(value, RS_T_VOID)) {
						rs_report(batch, RS_SEVERITY_ERROR, "RS109", "Invalid return.", node->a->unit, node->a->start, node->a->end,
							"Help: give the function a return type: fn name(): number { ... }.", "This function returns nothing (void), but a value is returned.");
					}
				}
			} else if (!node->a) {
				rs_report(batch, RS_SEVERITY_ERROR, "RS109", "Invalid return.", node->unit, node->start, node->end, NULL,
					"This function returns a '%s': give the value.", rs_type_name(checker->ret));
			} else {
				rs_type value = rs_check_value(checker, node->a, checker->ret, "returned");
				if (!rs_type_assignable(checker->ret, value)) rs_report_mismatch(checker, node->a, checker->ret, value);
			}
			break;
		case RS_N_BLOCK:
			rs_check_block(checker, node, 1);
			break;
		default:
			break;
	}
}

static void rs_check_block(rs_checker* checker, rs_node* block, int new_scope) {
	uint32_t i;
	int reported_unreachable = 0;
	if (!block) return;
	if (new_scope) rs_scope_push(checker);
	for (i = 0; i < block->count; i++) {
		rs_check_statement(checker, block->list[i]);
		if (!reported_unreachable && i + 1 < block->count) {
			int kind = block->list[i]->kind;
			if (kind == RS_N_RETURN || kind == RS_N_BREAK || kind == RS_N_CONTINUE) {
				rs_node* next = block->list[i + 1];
				rs_report(checker->batch, RS_SEVERITY_WARNING, "RS112", "Unreachable code.", next->unit, next->start, next->end, NULL,
					"This code never runs: it follows a '%s'.", kind == RS_N_RETURN ? "return" : (kind == RS_N_BREAK ? "break" : "continue"));
				reported_unreachable = 1;
			}
		}
	}
	if (new_scope) rs_scope_pop(checker, block->end);
}

/* ---------------------------------------------------------------------------
 *  Declarations
 * ------------------------------------------------------------------------- */

/** A name that cannot be given to a function of a script. */
static int rs_check_function_name(rs_checker* checker, rs_node* node) {
	rs_batch* batch = checker->batch;
	const char* name = node->text;
	rs_sig sig;
	if (rs_native_find(name, NULL) || rs_intrinsic_find(name, NULL) >= 0) {
		rs_report(batch, RS_SEVERITY_ERROR, "RS107", "Duplicate declaration.", node->unit, node->name_start, node->name_end, "Help: give the function another name.",
			"'%s' is a native function of Raven: it cannot be %s.", name, node->is_declare ? "declared again" : "replaced");
		return 0;
	}
	if (strcmp(name, "number") == 0 || strcmp(name, "string") == 0 || strcmp(name, "bool") == 0 || strcmp(name, "void") == 0) {
		rs_report(batch, RS_SEVERITY_ERROR, "RS107", "Duplicate declaration.", node->unit, node->name_start, node->name_end, NULL, "'%s' is a type: it cannot name a function.", name);
		return 0;
	}
	if (batch->env->host.lookup) {
		rs_external_kind kind = batch->env->host.lookup(batch->env->host.user_data, name, &sig);
		if (kind != RS_EXTERNAL_NONE && !node->is_declare) {
			rs_report(batch, RS_SEVERITY_ERROR, "RS107", "Duplicate declaration.", node->unit, node->name_start, node->name_end, "Help: give the function another name.",
				"The action '%s' is already bound by the application (or built into Raven).", name);
			return 0;
		}
		if (kind == RS_EXTERNAL_TYPED && node->is_declare) {
			rs_sig declared;
			uint32_t i;
			int same = 1;
			memset(&declared, 0, sizeof(declared));
			declared.ret = node->type;
			declared.count = (int)node->param_count;
			for (i = 0; i < node->param_count && i < RS_MAX_PARAMS; i++) declared.params[i] = node->params[i].type;
			if (declared.count != sig.count || !rs_type_equal(declared.ret, sig.ret)) same = 0;
			for (i = 0; same && i < (uint32_t)declared.count; i++) {
				if (!rs_type_equal(declared.params[i], sig.params[i])) same = 0;
			}
			if (!same) {
				rs_buf bound = { 0 };
				rs_sig_format(&sig, &bound);
				rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", node->unit, node->name_start, node->end, NULL,
					"This declaration does not match the function the application binds:\n%s%s", name, bound.data);
				rs_buf_free(&bound);
			}
		}
	}
	return 1;
}

static void rs_declare_functions(rs_checker* checker) {
	rs_batch* batch = checker->batch;
	uint32_t u, i;
	for (u = 0; u < batch->unit_count; u++) {
		rs_program* program = &batch->programs[u];
		for (i = 0; i < program->count; i++) {
			rs_node* node = program->items[i];
			rs_fn_info* existing;
			rs_fn_info* info;
			uint32_t p;
			size_t storage = 0;
			if (node->kind != RS_N_FN) continue;
			existing = rs_batch_function(batch, node->text);
			if (existing) {
				rs_report_related(batch, RS_SEVERITY_ERROR, "RS107", "Duplicate declaration.", node->unit, node->name_start, node->name_end,
					existing->unit, existing->name_start, existing->name_end, "It is declared here.", "Help: give it another name.",
					"The function '%s' is declared twice.", node->text);
				continue;
			}
			if (rs_batch_global(batch, node->text)) {
				rs_report(batch, RS_SEVERITY_ERROR, "RS107", "Duplicate declaration.", node->unit, node->name_start, node->name_end, NULL,
					"'%s' is already the name of a variable.", node->text);
				continue;
			}
			rs_check_function_name(checker, node);
			RS_GROW(batch->functions, batch->function_count, batch->function_capacity);
			info = &batch->functions[batch->function_count++];
			memset(info, 0, sizeof(*info));
			info->name = node->text;
			info->unit = node->unit;
			info->name_start = node->name_start;
			info->name_end = node->name_end;
			info->node = node;
			info->sig.ret = node->type;
			info->sig.count = (int)node->param_count;
			for (p = 0; p < node->param_count; p++) storage += strlen(node->params[p].name) + 1;
			info->names_storage = (char*)rs_arena_alloc(&batch->arena, storage + 1);
			storage = 0;
			for (p = 0; p < node->param_count && p < RS_MAX_PARAMS; p++) {
				size_t length = strlen(node->params[p].name);
				info->sig.params[p] = node->params[p].type;
				memcpy(info->names_storage + storage, node->params[p].name, length + 1);
				info->sig.names[p] = info->names_storage + storage;
				storage += length + 1;
			}
			info->symbol = rs_symbol_add(batch, node->is_declare ? RS_SYM_DECLARED : RS_SYM_FUNCTION, node->text, node->type, &info->sig, node->unit, node->name_start, node->name_end);
			rs_reference_add(batch, info->symbol, node->unit, node->name_start, node->name_end);
			rs_hover_function(batch, node->unit, node->name_start, node->name_end, node->is_declare ? "declare fn " : "fn ", node->text, &info->sig, NULL);
			if (node->is_declare) {
				uint32_t k;
				for (k = 0; k < node->param_count; k++) {
					if (node->params[k].type.depth > 0) {
						rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", node->unit, node->params[k].type_start, node->params[k].type_end, NULL,
							"A function of the application takes numbers, strings and booleans, not arrays.");
					}
				}
				if (node->type.depth > 0) {
					rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", node->unit, node->annotation_start, node->annotation_end, NULL,
						"A function of the application returns a number, a string, a boolean or nothing, not an array.");
				}
			}
			if (node->type.base == RS_T_NULL && node->type.depth == 0) {
				rs_report(batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", node->unit, node->annotation_start, node->annotation_end, NULL, "A function cannot return 'null': write void.");
			}
		}
	}
}

static void rs_declare_globals(rs_checker* checker) {
	rs_batch* batch = checker->batch;
	uint32_t u, i;
	for (u = 0; u < batch->unit_count; u++) {
		rs_program* program = &batch->programs[u];
		for (i = 0; i < program->count; i++) {
			rs_node* node = program->items[i];
			rs_global_info* existing;
			rs_global_info* info;
			if (node->kind != RS_N_LET) continue;
			existing = rs_batch_global(batch, node->text);
			if (existing) {
				rs_report_related(batch, RS_SEVERITY_ERROR, "RS107", "Duplicate declaration.", node->unit, node->name_start, node->name_end,
					existing->node->unit, existing->node->name_start, existing->node->name_end, "It is declared here.", "Help: give it another name.",
					"'%s' is declared twice.", node->text);
				continue;
			}
			if (strchr(node->text, '.') == NULL && rs_batch_function(batch, node->text)) {
				rs_report(batch, RS_SEVERITY_ERROR, "RS107", "Duplicate declaration.", node->unit, node->name_start, node->name_end, NULL,
					"'%s' is already the name of a function.", node->text);
			}
			if (strcmp(node->text, "number") == 0 || strcmp(node->text, "string") == 0 || strcmp(node->text, "bool") == 0 || strcmp(node->text, "void") == 0) {
				rs_report(batch, RS_SEVERITY_ERROR, "RS107", "Duplicate declaration.", node->unit, node->name_start, node->name_end, NULL, "'%s' is a type: it cannot name a variable.", node->text);
			}
			RS_GROW(batch->globals, batch->global_count, batch->global_capacity);
			info = &batch->globals[batch->global_count++];
			info->node = node;
			info->slot = (int)(batch->global_count - 1);
			info->symbol = -1;
			node->type = rs_error_type();
		}
	}
}

static void rs_check_globals(rs_checker* checker) {
	rs_batch* batch = checker->batch;
	uint32_t i;
	checker->initializing = 1;
	for (i = 0; i < batch->global_count; i++) {
		rs_global_info* info = &batch->globals[i];
		rs_node* node = info->node;
		checker->initializing_index = i;
		node->type = rs_check_declaration_type(checker, node);
		info->symbol = rs_symbol_add(batch, node->op ? RS_SYM_CONST : RS_SYM_GLOBAL, node->text, node->type, NULL, node->unit, node->name_start, node->name_end);
		rs_reference_add(batch, info->symbol, node->unit, node->name_start, node->name_end);
		rs_hover_add(batch, node->unit, node->name_start, node->name_end, "```rvs\n%s %s: %s\n```", node->op ? "const" : "let", node->text, rs_type_name(node->type));
	}
	checker->initializing = 0;
}

static void rs_check_function(rs_checker* checker, rs_fn_info* info, int index) {
	rs_node* node = info->node;
	uint32_t p;
	if (node->is_declare || !node->a) return;
	checker->function = info;
	checker->function_index = info->symbol;
	checker->ret = node->type;
	checker->next_slot = 0;
	checker->max_slot = 0;
	checker->var_count = 0;
	checker->scope_count = 0;
	checker->loop_count = 0;
	rs_scope_push(checker);
	for (p = 0; p < node->param_count; p++) {
		rs_param* param = &node->params[p];
		if (rs_type_is_error(param->type)) continue;
		if (param->type.depth == 0 && (param->type.base == RS_T_VOID)) {
			rs_report(checker->batch, RS_SEVERITY_ERROR, "RS102", "Type mismatch.", node->unit, param->type_start, param->type_end, NULL, "A parameter cannot be of type 'void'.");
		}
		rs_var_declare(checker, param->name, param->type, 0, node->unit, param->start, param->end, 1);
	}
	rs_check_block(checker, node->a, 1);
	rs_scope_pop(checker, node->end);
	node->locals = checker->max_slot;
	if (!rs_is(node->type, RS_T_VOID) && !rs_type_is_error(node->type) && !rs_always_returns(node->a)) {
		uint32_t end = node->a->end;
		rs_report(checker->batch, RS_SEVERITY_ERROR, "RS108", "Missing return.", node->unit, end > node->a->start ? end - 1 : end, end, "Help: add a return at the end of the function.",
			"The function '%s' must return a '%s' on every path.", node->text, rs_type_name(node->type));
	}
	RS_UNUSED(index);
	checker->function = NULL;
	checker->function_index = -1;
}

void rs_check(rs_batch* batch) {
	rs_checker checker;
	uint32_t i;
	memset(&checker, 0, sizeof(checker));
	checker.batch = batch;
	checker.function_index = -1;
	checker.ret = T(RS_T_VOID);
	rs_declare_functions(&checker);
	rs_declare_globals(&checker);
	rs_check_globals(&checker);
	for (i = 0; i < batch->function_count; i++) rs_check_function(&checker, &batch->functions[i], (int)i);
	rs_free(checker.vars);
	rs_free(checker.scopes);
	rs_free(checker.loops);
}
