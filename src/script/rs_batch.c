/**
 * Raven Script - script/rs_batch.c
 * A batch: the scripts of one document (its <Script> elements and the
 * files they name), compiled together so that they see each other, and
 * installed together, or not at all when one of them has an error.
 */

#include "rs_internal.h"

rs_batch* rs_batch_create(rs_env* env, int flags) {
	rs_batch* batch = (rs_batch*)rs_calloc(1, sizeof(rs_batch));
	batch->env = env;
	batch->flags = flags;
	return batch;
}

void rs_batch_destroy(rs_batch* batch) {
	uint32_t i;
	if (!batch) return;
	for (i = 0; i < batch->function_count; i++) rs_chunk_release(batch->functions[i].chunk);
	if (batch->inits) {
		for (i = 0; i < batch->unit_count; i++) rs_chunk_release(batch->inits[i]);
	}
	if (batch->sources) {
		for (i = 0; i < batch->unit_count; i++) rs_source_release(batch->sources[i]);
	}
	if (batch->programs) {
		for (i = 0; i < batch->unit_count; i++) rs_free(batch->programs[i].items);
	}
	for (i = 0; i < batch->unit_count; i++) rs_unit_free(&batch->units[i]);
	for (i = 0; i < batch->symbol_count; i++) {
		rs_free(batch->symbols[i].name);
		rs_free(batch->symbols[i].file);
	}
	for (i = 0; i < batch->hover_count; i++) rs_free(batch->hovers[i].text);
	for (i = 0; i < batch->call_count; i++) {
		rs_free(batch->calls[i].commas);
		rs_free(batch->calls[i].label);
		rs_free(batch->calls[i].doc);
	}
	rs_free(batch->inits);
	rs_free(batch->sources);
	rs_free(batch->programs);
	rs_free(batch->units);
	rs_free(batch->functions);
	rs_free(batch->globals);
	rs_free(batch->symbols);
	rs_free(batch->references);
	rs_free(batch->hovers);
	rs_free(batch->calls);
	rs_buf_free(&batch->diagnostics_json);
	rs_arena_free(&batch->arena);
	rs_free(batch);
}

int rs_batch_add(rs_batch* batch, const char* file, const char* text, size_t length, int line, int column) {
	if (batch->compiled) return -1;
	RS_GROW(batch->units, batch->unit_count, batch->unit_capacity);
	rs_unit_init(&batch->units[batch->unit_count], file, text, length, line, column);
	return (int)batch->unit_count++;
}

int rs_batch_error_count(const rs_batch* batch) {
	return batch->errors;
}

int rs_batch_warning_count(const rs_batch* batch) {
	return batch->warnings;
}

/* ---------------------------------------------------------------------------
 *  Reports
 * ------------------------------------------------------------------------- */

void rs_batch_locate(const rs_batch* batch, int unit, uint32_t start, uint32_t end, rs_location* location) {
	memset(location, 0, sizeof(*location));
	location->unit = -1;
	if (unit < 0 || (uint32_t)unit >= batch->unit_count) return;
	rs_unit_locate(&batch->units[unit], unit, start, end, location, NULL);
}

static void rs_json_location(rs_buf* out, const rs_location* location) {
	rs_buf_printf(out, "{\"unit\":%d,\"start\":%d,\"end\":%d,\"file\":", location->unit, location->start, location->end);
	rs_buf_json_string(out, location->file ? location->file : "");
	rs_buf_printf(out, ",\"line\":%d,\"column\":%d}", location->line, location->column);
}

void rs_batch_report(rs_batch* batch, const rs_diag* diag) {
	if (diag->severity == RS_SEVERITY_ERROR) batch->errors++;
	else if (diag->severity == RS_SEVERITY_WARNING) batch->warnings++;
	if (batch->flags & RS_BATCH_ANALYSIS) {
		rs_buf* out = &batch->diagnostics_json;
		if (batch->diagnostic_count++ > 0) rs_buf_putc(out, ',');
		rs_buf_printf(out, "{\"severity\":%d,\"code\":", (int)diag->severity);
		rs_buf_json_string(out, diag->code);
		rs_buf_puts(out, ",\"title\":");
		rs_buf_json_string(out, diag->title);
		rs_buf_puts(out, ",\"message\":");
		rs_buf_json_string(out, diag->message);
		rs_buf_puts(out, ",\"help\":");
		rs_buf_json_string(out, diag->help ? diag->help : "");
		rs_buf_puts(out, ",\"location\":");
		if (diag->has_location) rs_json_location(out, &diag->location);
		else rs_buf_puts(out, "null");
		rs_buf_puts(out, ",\"related\":");
		if (diag->has_related) {
			rs_json_location(out, &diag->related);
			rs_buf_puts(out, ",\"relatedNote\":");
			rs_buf_json_string(out, diag->related_note ? diag->related_note : "");
		} else {
			rs_buf_puts(out, "null");
		}
		rs_buf_putc(out, '}');
	}
	rs_env_report(batch->env, diag);
}

static void rs_report_with(rs_batch* batch, rs_severity severity, const char* code, const char* title, int unit, uint32_t start, uint32_t end,
	int related_unit, uint32_t related_start, uint32_t related_end, const char* related_note, const char* help, const char* format, va_list arguments) {
	rs_diag diag;
	rs_buf message = { 0 };
	rs_buf line = { 0 };
	rs_buf related_line = { 0 };
	memset(&diag, 0, sizeof(diag));
	diag.severity = severity;
	diag.code = code;
	diag.title = title;
	diag.help = help;
	rs_buf_vprintf(&message, format, arguments);
	diag.message = message.data ? message.data : "";
	if (unit >= 0 && (uint32_t)unit < batch->unit_count) {
		rs_unit_locate(&batch->units[unit], unit, start, end, &diag.location, &line);
		diag.has_location = 1;
	}
	if (related_unit >= 0 && (uint32_t)related_unit < batch->unit_count) {
		rs_unit_locate(&batch->units[related_unit], related_unit, related_start, related_end, &diag.related, &related_line);
		diag.has_related = 1;
		diag.related_note = related_note;
	}
	rs_batch_report(batch, &diag);
	rs_buf_free(&message);
	rs_buf_free(&line);
	rs_buf_free(&related_line);
}

void rs_report(rs_batch* batch, rs_severity severity, const char* code, const char* title, int unit, uint32_t start, uint32_t end, const char* help, const char* format, ...) {
	va_list arguments;
	va_start(arguments, format);
	rs_report_with(batch, severity, code, title, unit, start, end, -1, 0, 0, NULL, help, format, arguments);
	va_end(arguments);
}

void rs_report_related(rs_batch* batch, rs_severity severity, const char* code, const char* title, int unit, uint32_t start, uint32_t end,
	int related_unit, uint32_t related_start, uint32_t related_end, const char* related_note, const char* help, const char* format, ...) {
	va_list arguments;
	va_start(arguments, format);
	rs_report_with(batch, severity, code, title, unit, start, end, related_unit, related_start, related_end, related_note, help, format, arguments);
	va_end(arguments);
}

/* ---------------------------------------------------------------------------
 *  Compiling and installing
 * ------------------------------------------------------------------------- */

int rs_batch_compile(rs_batch* batch) {
	uint32_t i;
	if (batch->compiled) return batch->compile_errors;
	batch->compiled = 1;
	batch->programs = (rs_program*)rs_calloc(batch->unit_count ? batch->unit_count : 1, sizeof(rs_program));
	for (i = 0; i < batch->unit_count; i++) rs_parse(batch, (int)i, &batch->programs[i]);
	rs_check(batch);
	if (batch->errors == 0) rs_compile(batch);
	batch->compile_errors = batch->errors;
	return batch->errors;
}

static void rs_relocate(rs_chunk* chunk, const uint32_t* slots) {
	uint32_t i;
	if (!chunk) return;
	for (i = 0; i < chunk->count; i++) {
		rs_instr* instr = &chunk->code[i];
		if ((instr->op == OP_LOAD_GLOBAL || instr->op == OP_STORE_GLOBAL) && instr->b == 1) {
			instr->a = (int32_t)slots[instr->a];
			instr->b = 0;
		}
	}
}

int rs_batch_install(rs_batch* batch) {
	rs_env* env = batch->env;
	uint32_t i, added = 0, total;
	uint32_t* slots;
	char error[256];
	if (!batch->compiled) rs_batch_compile(batch);
	if (batch->compile_errors > 0 || batch->installed) return 0;
	if (env->running) {
		/* a script that loads a document would replace the code running it */
		rs_diag diag;
		memset(&diag, 0, sizeof(diag));
		diag.severity = RS_SEVERITY_ERROR;
		diag.code = "RS000";
		diag.title = "Scripts cannot be loaded now.";
		diag.message = "A document with scripts was loaded while a script runs: its scripts are ignored.";
		rs_batch_report(batch, &diag);
		return 0;
	}
	/* the slots of the globals: a global declared again (a document reloaded) keeps its slot,
	   so that the code of the other documents sees the new one */
	slots = (uint32_t*)rs_malloc((batch->global_count ? batch->global_count : 1) * sizeof(uint32_t));
	for (i = 0; i < batch->global_count; i++) {
		int existing = rs_env_global_index(env, batch->globals[i].node->text);
		slots[i] = existing >= 0 ? (uint32_t)existing : env->global_count + added++;
	}
	total = env->global_count + added;
	for (i = 0; i < batch->function_count; i++) rs_relocate(batch->functions[i].chunk, slots);
	for (i = 0; i < batch->unit_count; i++) rs_relocate(batch->inits[i], slots);
	/* nothing runs that was not validated */
	for (i = 0; i < batch->function_count + batch->unit_count; i++) {
		rs_chunk* chunk = i < batch->function_count ? batch->functions[i].chunk : batch->inits[i - batch->function_count];
		if (chunk && !rs_chunk_validate(chunk, total, error, sizeof(error))) {
			rs_diag diag;
			memset(&diag, 0, sizeof(diag));
			diag.severity = RS_SEVERITY_ERROR;
			diag.code = "RS000";
			diag.title = "Internal error.";
			diag.message = error;
			rs_batch_report(batch, &diag);
			rs_free(slots);
			return 0;
		}
	}
	for (i = 0; i < batch->global_count; i++) {
		rs_node* node = batch->globals[i].node;
		int line, column;
		rs_unit_position(&batch->units[node->unit], node->name_start, &line, &column);
		rs_env_define_global(env, slots[i], node->text, node->type, node->op, batch->units[node->unit].file, line, column);
	}
	rs_free(slots);
	for (i = 0; i < batch->function_count; i++) {
		rs_fn_info* info = &batch->functions[i];
		int line, column;
		if (!info->chunk) continue;
		rs_unit_position(&batch->units[info->unit], info->name_start, &line, &column);
		rs_env_set_function(env, info->name, info->chunk, batch->units[info->unit].file, line, column);
	}
	batch->installed = 1;
	return 1;
}

int rs_batch_initialize(rs_batch* batch) {
	uint32_t i;
	int ok = 1;
	if (!batch->installed || batch->initialized) return 0;
	batch->initialized = 1;
	/* unit after unit: an error stops its unit, the globals it did not reach keep their default value */
	for (i = 0; i < batch->unit_count; i++) {
		if (batch->inits[i] && rs_vm_run(batch->env, batch->inits[i], NULL, 0, NULL) != RS_CALL_OK) ok = 0;
	}
	return ok;
}

/* ---------------------------------------------------------------------------
 *  Functions of the batch
 * ------------------------------------------------------------------------- */

int rs_batch_function_count(const rs_batch* batch) {
	return (int)batch->function_count;
}

const char* rs_batch_function_name(const rs_batch* batch, int index) {
	return index >= 0 && (uint32_t)index < batch->function_count ? batch->functions[index].name : NULL;
}

int rs_batch_function_sig(const rs_batch* batch, int index, rs_sig* sig) {
	if (index < 0 || (uint32_t)index >= batch->function_count) return 0;
	*sig = batch->functions[index].sig;
	return 1;
}

int rs_batch_function_location(const rs_batch* batch, int index, rs_location* location) {
	const rs_fn_info* info;
	if (index < 0 || (uint32_t)index >= batch->function_count) return 0;
	info = &batch->functions[index];
	rs_batch_locate(batch, info->unit, info->name_start, info->name_end, location);
	return 1;
}

static int rs_batch_function_index(const rs_batch* batch, const char* name) {
	uint32_t i;
	for (i = 0; i < batch->function_count; i++) {
		if (strcmp(batch->functions[i].name, name) == 0) return (int)i;
	}
	return -1;
}

int rs_batch_find_function(const rs_batch* batch, const char* name, rs_sig* sig, rs_location* location) {
	int index = rs_batch_function_index(batch, name);
	if (index >= 0) {
		if (sig) *sig = batch->functions[index].sig;
		if (location) rs_batch_function_location(batch, index, location);
		return 1;
	}
	if (rs_env_find_function(batch->env, name, sig)) {
		if (location) rs_env_function_location(batch->env, name, location);
		return 1;
	}
	return 0;
}

static void rs_describe_parameters(rs_buf* out, const rs_base* types, int count) {
	int i;
	if (count == 0) {
		rs_buf_puts(out, "no parameter");
		return;
	}
	rs_buf_printf(out, "%d parameter%s of type ", count, count == 1 ? "" : "s");
	for (i = 0; i < count; i++) {
		if (i) rs_buf_puts(out, i + 1 == count ? " and " : ", ");
		rs_buf_printf(out, "'%s'", rs_type_name(rs_type_make(types[i], 0)));
	}
}

int rs_batch_check_handler(rs_batch* batch, const char* tag, const char* attribute, const char* action, const rs_location* where) {
	const rs_event_sig* event = rs_event_find(tag);
	rs_base none[1] = { RS_T_ERROR };
	const rs_base* types = event ? event->types : none;
	int count = event ? event->count : 0, bad = 0, index;
	rs_sig sig;
	rs_location declared;
	rs_buf event_name = { 0 };
	rs_buf message = { 0 };
	rs_buf help = { 0 };
	rs_handler_result result;
	rs_diag diag;
	if (!action || !action[0] || !rs_batch_find_function(batch, action, &sig, &declared)) return 1;
	/* a declared function (declare fn) is the application's: it is checked when it is bound */
	index = rs_batch_function_index(batch, action);
	if (index >= 0 && batch->functions[index].node->is_declare) return 1;
	result = rs_handler_check(&sig, types, count, &bad);
	if (result == RS_HANDLER_OK) return 1;
	if (event) rs_buf_puts(&event_name, event->event);
	else rs_buf_printf(&event_name, "%s.%s", tag, attribute && attribute[0] ? attribute : "Action");
	if (result == RS_HANDLER_TOO_MANY_PARAMETERS) {
		rs_buf_printf(&message, "%s provides %d argument%s,\nbut '%s' requires %d.", event_name.data, count, count == 1 ? "" : "s", action, sig.count);
		rs_buf_printf(&help, "Remove the additional required parameter%s.", sig.count - count > 1 ? "s" : "");
	} else {
		rs_base got[RS_MAX_PARAMS];
		int i, shown = sig.count < count ? sig.count : count;
		for (i = 0; i < sig.count && i < RS_MAX_PARAMS; i++) got[i] = sig.params[i].depth ? RS_T_ERROR : (rs_base)sig.params[i].base;
		rs_buf_printf(&message, "%s expects a function with\n", event_name.data);
		rs_describe_parameters(&message, types, shown);
		rs_buf_printf(&message, ".\n\nBut '%s' has:\n", action);
		if (sig.params[bad].depth) {
			rs_buf_printf(&message, "parameter %d of type '%s'", bad + 1, rs_type_name(sig.params[bad]));
		} else {
			rs_describe_parameters(&message, got, shown);
		}
		rs_buf_putc(&message, '.');
		rs_buf_printf(&help, "Suggestion:\nChange the parameter type from '%s' to '%s'.", rs_type_name(sig.params[bad]), rs_type_name(rs_type_make(types[bad], 0)));
	}
	memset(&diag, 0, sizeof(diag));
	diag.severity = RS_SEVERITY_ERROR;
	diag.code = "RS103";
	diag.title = "Invalid event handler signature.";
	diag.message = message.data;
	diag.help = help.data;
	if (where) {
		diag.location = *where;
		diag.has_location = 1;
	}
	{
		/* point at the type of the parameter that is wrong, or at the name of the function */
		rs_buf line = { 0 };
		if (index >= 0) {
			const rs_fn_info* info = &batch->functions[index];
			uint32_t start = info->name_start, end = info->name_end;
			if (result == RS_HANDLER_WRONG_TYPE && (uint32_t)bad < info->node->param_count) {
				start = info->node->params[bad].type_start;
				end = info->node->params[bad].type_end;
			} else if (result == RS_HANDLER_TOO_MANY_PARAMETERS && (uint32_t)count < info->node->param_count) {
				start = info->node->params[count].start;
				end = info->node->params[info->node->param_count - 1].type_end;
			}
			rs_unit_locate(&batch->units[info->unit], info->unit, start, end, &diag.related, &line);
		} else {
			diag.related = declared;
		}
		diag.has_related = 1;
		diag.related_note = "";
		rs_batch_report(batch, &diag);
		rs_buf_free(&line);
	}
	rs_buf_free(&event_name);
	rs_buf_free(&message);
	rs_buf_free(&help);
	return 0;
}
