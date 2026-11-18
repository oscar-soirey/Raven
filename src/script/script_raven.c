/**
 * Raven - script/script_raven.c
 * Raven Script in the library: the host of the core of the language.
 *
 *   - Actions: a script function is bound as an action (Raction_bind) to a
 *     trampoline, and so is a function bound in C with parameters
 *     (Raction_bind_with_parameters). When a widget fires the action, the
 *     trampoline reads what its event gives (the value of a slider...) and
 *     calls the function with it. The registry of actions stays the one of
 *     Raction_bind: binding a name replaces what it was bound to, whoever
 *     bound it.
 *   - Natives: app.log, app.quit, app.fire, ui.*, window.* run here; a
 *     function bound in C with parameters of the same name replaces one.
 *   - Documents: the <Script> elements of an XML document are compiled
 *     before its widgets are built (their actions are checked against
 *     them), and their globals initialized after.
 */

#include "script_raven.h"
#include "rs.h"
#include "../core/context.h"
#include "../widget/widget.h"
#include "../xml/xml_watch.h"

/* ---------------------------------------------------------------------------
 *  State
 * ------------------------------------------------------------------------- */

/** A function bound as an action through the trampoline: a script function, or a C function with parameters. */
typedef struct rvn_binding_t {
	char* name;
	int is_script;
	/* C */
	rs_sig sig;
	char names[256];
	RActionParametersCallback callback;
	void* user_data;
	struct rvn_binding_t* next;
} rvn_binding_t;

struct rvn_script_document_t {
	rvn_script_document_t* previous;
	rs_batch* batch;
	/* a batch without units: the checks of a document that has no script */
	rs_batch* empty;
	char* path;
	const char* source;
	size_t source_size;
	/* the names of the widgets of the document (RS301) */
	char** names;
	uint32_t name_count;
	uint32_t name_cap;
};

#define RVN_SCRIPT_MAX_SENDERS 256

static struct {
	rs_env* env;
	int disabled;
	RScriptLogCallback log;
	void* log_user;
	RScriptDiagnosticCallback diagnostic;
	void* diagnostic_user;
	rvn_binding_t* bindings;
	rvn_script_document_t* current;
	/* the widget whose event runs each invocation (ui.* and window.* act on its window) */
	Rid senders[RVN_SCRIPT_MAX_SENDERS];
	int sender_count;
	/* while the initializers of a document run: what it built */
	Rid built;
	const char* file;
	char* format_result;
	uint32_t max_instructions;
	int max_depth;
	size_t max_memory;
} g_script;

static void rvn_script_trampoline(Rid sender, const char* action, void* user_data);

/* ---------------------------------------------------------------------------
 *  Bindings
 * ------------------------------------------------------------------------- */

static rvn_binding_t* rvn_binding_find(const char* name) {
	rvn_binding_t* binding;
	for (binding = g_script.bindings; binding; binding = binding->next) {
		if (strcmp(binding->name, name) == 0) return binding;
	}
	return NULL;
}

static rvn_binding_t* rvn_binding_get(const char* name) {
	rvn_binding_t* binding = rvn_binding_find(name);
	if (binding) return binding;
	binding = (rvn_binding_t*)rvn_alloc(sizeof(rvn_binding_t));
	memset(binding, 0, sizeof(*binding));
	binding->name = rvn_strdup(name);
	binding->next = g_script.bindings;
	g_script.bindings = binding;
	return binding;
}

/** The binding the action of that name is bound to now, or NULL (bound to something else, or to nothing). */
static rvn_binding_t* rvn_binding_active(const char* name) {
	RActionCallback callback;
	void* user_data;
	if (!rvn_action_lookup(name, &callback, &user_data) || callback != rvn_script_trampoline) return NULL;
	return (rvn_binding_t*)user_data;
}

/* ---------------------------------------------------------------------------
 *  Diagnostics
 * ------------------------------------------------------------------------- */

static void rvn_script_report(void* user_data, const rs_diag* diag) {
	rs_buf text = { 0 };
	RSeverity severity = diag->severity == RS_SEVERITY_ERROR ? R_SEVERITY_ERROR : (diag->severity == RS_SEVERITY_WARNING ? R_SEVERITY_WARNING : R_SEVERITY_WEAK_WARNING);
	RVN_UNUSED(user_data);
	rs_diag_format(diag, &text);
	while (text.length > 0 && text.data[text.length - 1] == '\n') text.data[--text.length] = '\0';
	if (g_script.diagnostic) {
		g_script.diagnostic(severity, diag->code, diag->has_location ? diag->location.file : NULL,
			diag->has_location ? diag->location.line : 0, diag->has_location ? diag->location.column : 0, text.data ? text.data : "", g_script.diagnostic_user);
	} else {
		rvn_error(R_ERROR_SCRIPT, severity, "%s", text.data ? text.data : "");
	}
	rs_buf_free(&text);
}

/** A diagnostic made here (not by the core). */
static void rvn_script_report_text(const char* code, const char* title, const char* format, ...) {
	rs_diag diag;
	rs_buf message = { 0 };
	va_list args;
	memset(&diag, 0, sizeof(diag));
	diag.severity = RS_SEVERITY_ERROR;
	diag.code = code;
	diag.title = title;
	diag.runtime = 1;
	va_start(args, format);
	rs_buf_vprintf(&message, format, args);
	va_end(args);
	diag.message = message.data;
	rvn_script_report(NULL, &diag);
	rs_buf_free(&message);
}

/* ---------------------------------------------------------------------------
 *  Where the natives act
 * ------------------------------------------------------------------------- */

static Rid rvn_script_sender(void) {
	return g_script.sender_count > 0 ? g_script.senders[g_script.sender_count - 1] : R_INVALID_ID;
}

/** The window the running script works in: the one of the widget that fired the event, or of what the document built. */
static rvn_window_t* rvn_script_window(void) {
	Rid places[2];
	int i;
	places[0] = rvn_script_sender();
	places[1] = g_script.built;
	for (i = 0; i < 2; i++) {
		rvn_widget_t* widget;
		rvn_window_t* window;
		if (places[i] == R_INVALID_ID) continue;
		window = rvn_window_get(places[i]);
		if (window) return window;
		widget = rvn_widget_get(places[i]);
		if (widget && widget->window) return widget->window;
	}
	return NULL;
}

static rvn_widget_t* rvn_script_find_in(rvn_widget_t* widget, const char* name) {
	uint32_t i;
	if (!widget) return NULL;
	if (widget->name && strcmp(widget->name, name) == 0) return widget;
	for (i = 0; i < widget->child_count; i++) {
		rvn_widget_t* found = rvn_script_find_in(widget->children[i], name);
		if (found) return found;
	}
	return NULL;
}

/** The widget of that name: in the window of the script, in what the document built, then in every window. */
static rvn_widget_t* rvn_script_widget(const char* name) {
	rvn_window_t* window = rvn_script_window();
	rvn_widget_t* found = NULL;
	uint32_t i;
	if (!name || !name[0]) return NULL;
	if (window) {
		Rid id = Rwidget_find(window->id, name);
		if (id != R_INVALID_ID) return rvn_widget_get(id);
	}
	{
		/* a tree not shown yet (Rwidget_create_from_xml), or the one of the sender */
		Rid places[2];
		int k;
		places[0] = g_script.built;
		places[1] = rvn_script_sender();
		for (k = 0; k < 2 && !found; k++) {
			rvn_widget_t* widget = places[k] != R_INVALID_ID ? rvn_widget_get(places[k]) : NULL;
			while (widget && widget->parent) widget = widget->parent;
			found = rvn_script_find_in(widget, name);
		}
		if (found) return found;
	}
	for (i = 0; i < g_rvn.windows.used; i++) {
		rvn_window_t* other = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		Rid id;
		if (!other || other == window) continue;
		id = Rwidget_find(other->id, name);
		if (id != R_INVALID_ID) return rvn_widget_get(id);
	}
	return NULL;
}

static const char* rvn_class_name(const rvn_widget_t* widget) {
	return widget && widget->cls && widget->cls->name ? widget->cls->name : "widget";
}

/* ---------------------------------------------------------------------------
 *  Events: the arguments a widget gives the function of its action
 * ------------------------------------------------------------------------- */

static int rvn_script_event_arguments(Rid sender, rs_value* args, char* event, size_t event_size) {
	rvn_widget_t* widget = sender != R_INVALID_ID ? rvn_widget_get(sender) : NULL;
	const rs_event_sig* signature;
	Rid id;
	event[0] = '\0';
	if (!widget) {
		snprintf(event, event_size, "Raction_fire");
		return 0;
	}
	id = widget->id;
	signature = rs_event_find(rvn_class_name(widget));
	if (!signature) {
		int type = widget->cls->type;
		snprintf(event, event_size, "%s.%s", rvn_class_name(widget), type == R_WIDGET_BUTTON || type == R_WIDGET_MENU_ITEM ? "OnClicked" : "Action");
		return 0;
	}
	snprintf(event, event_size, "%s", signature->event);
	switch (widget->cls->type) {
		case R_WIDGET_CHECKBOX: args[0] = rs_value_bool(Rcheckbox_is_checked(id)); return 1;
		case R_WIDGET_SLIDER: args[0] = rs_value_number(Rslider_get_value(id)); return 1;
		case R_WIDGET_SPIN_BOX: args[0] = rs_value_number(Rspin_box_get_value(id)); return 1;
		case R_WIDGET_RATING: args[0] = rs_value_number(Rrating_get_value(id)); return 1;
		case R_WIDGET_TEXT_INPUT: args[0] = rs_value_cstring(Rtext_input_get_text(id)); return 1;
		case R_WIDGET_COMBO_BOX: {
			int selected = Rcombo_box_get_selected(id);
			args[0] = rs_value_number(selected);
			args[1] = rs_value_cstring(selected >= 0 ? Rcombo_box_get_item_text(id, selected) : "");
			return 2;
		}
		case R_WIDGET_LIST_VIEW: {
			int selected = Rlist_view_get_selected(id);
			args[0] = rs_value_number(selected);
			args[1] = rs_value_cstring(selected >= 0 ? Rlist_view_get_item_text(id, selected) : "");
			return 2;
		}
		case R_WIDGET_TAB_VIEW: args[0] = rs_value_number(Rtab_view_get_active(id)); return 1;
		case R_WIDGET_PAGINATION: args[0] = rs_value_number(Rpagination_get_page(id)); return 1;
		case R_WIDGET_CAROUSEL: args[0] = rs_value_number(Rcarousel_get_page(id)); return 1;
		case R_WIDGET_EXPANDER: args[0] = rs_value_bool(Rexpander_is_expanded(id)); return 1;
		case R_WIDGET_ISLAND: args[0] = rs_value_bool(Risland_is_expanded(id)); return 1;
		case R_WIDGET_COLOR_PICKER: args[0] = rs_value_cstring(Rcolor_picker_get_hex(id)); return 1;
		case R_WIDGET_DATE_PICKER: {
			int year = 0, month = 0, day = 0;
			Rdate_picker_get_date(id, &year, &month, &day);
			args[0] = rs_value_number(year);
			args[1] = rs_value_number(month);
			args[2] = rs_value_number(day);
			return 3;
		}
		case R_WIDGET_TIME_PICKER: {
			int hour = 0, minute = 0, second = 0;
			Rtime_picker_get_time(id, &hour, &minute, &second);
			args[0] = rs_value_number(hour);
			args[1] = rs_value_number(minute);
			args[2] = rs_value_number(second);
			return 3;
		}
		default:
			/* (a class of the table without a case here: no argument) */
			return 0;
	}
}

/* ---------------------------------------------------------------------------
 *  Functions bound in C with parameters
 * ------------------------------------------------------------------------- */

struct RActionContext {
	Rid sender;
	const char* name;
	const rs_value* args;
	int count;
	const rs_sig* sig;
	rs_value result;
	int failed;
	rs_buf error;
};

static int rvn_context_argument(const RActionContext* context, int index, rs_vtype type, const char* function) {
	if (!context) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: NULL context", function);
		return R_FALSE;
	}
	if (index < 0 || index >= context->count) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: '%s' has %d argument%s, there is no argument %d", function, context->name, context->count, context->count == 1 ? "" : "s", index);
		return R_FALSE;
	}
	if (context->args[index].type != type) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: argument %d of '%s' is a %s", function, index, context->name,
			rs_type_name(rs_value_static_type(context->args[index])));
		return R_FALSE;
	}
	return R_TRUE;
}

Rid Raction_get_sender(const RActionContext* context) {
	return context ? context->sender : R_INVALID_ID;
}

const char* Raction_get_name(const RActionContext* context) {
	return context ? context->name : "";
}

int Raction_get_argument_count(const RActionContext* context) {
	return context ? context->count : 0;
}

double Raction_get_number(const RActionContext* context, int index) {
	if (!rvn_context_argument(context, index, RS_V_NUMBER, __func__)) return 0.0;
	return context->args[index].as.number;
}

const char* Raction_get_string(const RActionContext* context, int index) {
	if (!rvn_context_argument(context, index, RS_V_STRING, __func__)) return "";
	return context->args[index].as.string->text;
}

int Raction_get_bool(const RActionContext* context, int index) {
	if (!rvn_context_argument(context, index, RS_V_BOOL, __func__)) return R_FALSE;
	return context->args[index].as.boolean;
}

static void rvn_context_set(RActionContext* context, rs_value value) {
	if (!context) {
		rs_value_release(value);
		return;
	}
	rs_value_release(context->result);
	context->result = value;
}

void Raction_return_number(RActionContext* context, double value) {
	rvn_context_set(context, rs_value_number(value));
}

void Raction_return_string(RActionContext* context, const char* value) {
	rvn_context_set(context, rs_value_cstring(value ? value : ""));
}

void Raction_return_bool(RActionContext* context, int value) {
	rvn_context_set(context, rs_value_bool(value));
}

void Raction_fail(RActionContext* context, const char* message) {
	if (!context) return;
	context->failed = R_TRUE;
	context->error.length = 0;
	rs_buf_puts(&context->error, message && message[0] ? message : "it failed");
}

int Raction_bind_with_parameters(const char* name, const char* signature, RActionParametersCallback callback, void* user_data) {
	rs_sig sig;
	char names[256];
	char problem[160];
	rvn_binding_t* binding;
	RVN_API_GUARD(R_FALSE);
	if (!name || !name[0]) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Raction_bind_with_parameters: empty action name");
		return R_FALSE;
	}
	if (!callback) {
		Raction_unbind(name);
		return R_TRUE;
	}
	if (!rs_sig_parse(signature, &sig, names, sizeof(names), problem, sizeof(problem))) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Raction_bind_with_parameters: '%s': invalid signature \"%s\": %s", name, signature ? signature : "", problem);
		return R_FALSE;
	}
	binding = rvn_binding_get(name);
	binding->is_script = R_FALSE;
	memcpy(binding->names, names, sizeof(names));
	binding->sig = sig;
	{
		/* the names point into this binding */
		int i;
		for (i = 0; i < sig.count; i++) {
			if (sig.names[i]) binding->sig.names[i] = binding->names + (sig.names[i] - names);
		}
	}
	binding->callback = callback;
	binding->user_data = user_data;
	Raction_bind(name, rvn_script_trampoline, binding);
	return R_TRUE;
}

/** Calls a C function bound with parameters; R_FALSE (and `error`) when it failed. */
static int rvn_binding_call(rvn_binding_t* binding, Rid sender, const char* name, const rs_value* args, int count, rs_value* result, rs_buf* error) {
	RActionContext context;
	RActionParametersCallback callback = binding->callback;
	void* user_data = binding->user_data;
	memset(&context, 0, sizeof(context));
	context.sender = sender;
	context.name = name;
	context.args = args;
	context.count = count;
	context.sig = &binding->sig;
	context.result = rs_value_void();
	callback(&context, user_data);
	if (context.failed) {
		if (error) rs_buf_puts(error, context.error.data ? context.error.data : "it failed");
		rs_buf_free(&context.error);
		rs_value_release(context.result);
		return R_FALSE;
	}
	rs_buf_free(&context.error);
	if (result) *result = context.result;
	else rs_value_release(context.result);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  The trampoline: an action fired (by a widget, Raction_fire, app.fire)
 * ------------------------------------------------------------------------- */

static void rvn_script_trampoline(Rid sender, const char* action, void* user_data) {
	rvn_binding_t* binding = (rvn_binding_t*)user_data;
	rs_value args[4];
	char event[96];
	int count, i;
	char* name;
	if (g_script.sender_count >= RVN_SCRIPT_MAX_SENDERS) {
		rvn_script_report_text("RS202", "Maximum recursion depth exceeded.", "The action '%s' was fired from scripts nested %d deep: it was not run.", action, RVN_SCRIPT_MAX_SENDERS);
		return;
	}
	/* (the name may be freed by what runs: a copy) */
	name = rvn_strdup(action);
	count = rvn_script_event_arguments(sender, args, event, sizeof(event));
	g_script.senders[g_script.sender_count++] = sender;
	if (binding->is_script) {
		if (g_script.env) rs_env_call(g_script.env, name, args, count, RS_CALL_EVENT, event, NULL);
	} else {
		rs_base types[4];
		int bad = 0;
		rs_handler_result fits;
		for (i = 0; i < count; i++) types[i] = (rs_base)rs_value_static_type(args[i]).base;
		fits = rs_handler_check(&binding->sig, types, count, &bad);
		if (fits == RS_HANDLER_TOO_MANY_PARAMETERS) {
			rvn_script_report_text("RS103", "Invalid event handler signature.", "%s provides %d argument%s,\nbut '%s' (bound with Raction_bind_with_parameters) requires %d.",
				event, count, count == 1 ? "" : "s", name, binding->sig.count);
		} else if (fits == RS_HANDLER_WRONG_TYPE) {
			rvn_script_report_text("RS103", "Invalid event handler signature.", "%s gives argument %d of type '%s',\nbut '%s' (bound with Raction_bind_with_parameters) takes a '%s'.",
				event, bad + 1, rs_type_name(rs_type_make(types[bad], 0)), name, rs_type_name(binding->sig.params[bad]));
		} else {
			rs_buf error = { 0 };
			/* (the value it returns goes nowhere: an event handler) */
			if (!rvn_binding_call(binding, sender, name, args, binding->sig.count, NULL, &error)) {
				rvn_script_report_text("RS205", "Native function failure.", "'%s' failed: %s", name, error.data ? error.data : "");
			}
			rs_buf_free(&error);
		}
	}
	g_script.sender_count--;
	for (i = 0; i < count; i++) rs_value_release(args[i]);
	rvn_free(name);
}

/* ---------------------------------------------------------------------------
 *  The host
 * ------------------------------------------------------------------------- */

static rs_external_kind rvn_script_lookup(void* user_data, const char* name, rs_sig* sig) {
	RActionCallback callback;
	void* bound;
	RVN_UNUSED(user_data);
	if (rvn_action_lookup(name, &callback, &bound)) {
		if (callback == rvn_script_trampoline) {
			rvn_binding_t* binding = (rvn_binding_t*)bound;
			/* a script function is the scripts' own: the type checker finds it among them */
			if (binding->is_script) return RS_EXTERNAL_NONE;
			*sig = binding->sig;
			return RS_EXTERNAL_TYPED;
		}
		return RS_EXTERNAL_ACTION;
	}
	if (Raction_is_builtin(name)) return RS_EXTERNAL_ACTION;
	return RS_EXTERNAL_NONE;
}

#define RVN_NEED_WIDGET(variable) \
	rvn_widget_t* variable = rvn_script_widget(rs_value_text(args[0])); \
	if (!variable) { \
		rs_buf_printf(error, "RS301: No widget is named '%s'%s.", rs_value_text(args[0]), rvn_script_window() ? " in the window" : ""); \
		return RS_CALL_FAILED; \
	}

static rs_call_status rvn_script_ui(const char* name, const rs_value* args, int count, rs_value* result, rs_buf* error) {
	RVN_UNUSED(count);
	if (strcmp(name, "ui.exists") == 0) {
		*result = rs_value_bool(rvn_script_widget(rs_value_text(args[0])) != NULL);
		return RS_CALL_OK;
	}
	if (strcmp(name, "ui.setText") == 0) {
		RVN_NEED_WIDGET(widget);
		if (!rvn_widget_set_attribute(widget, "Text", rs_value_text(args[1]))) {
			rs_buf_printf(error, "a %s has no text.", rvn_class_name(widget));
			return RS_CALL_FAILED;
		}
		return RS_CALL_OK;
	}
	if (strcmp(name, "ui.getText") == 0) {
		const char* text = NULL;
		Rid id;
		RVN_NEED_WIDGET(widget);
		id = widget->id;
		switch (widget->cls->type) {
			case R_WIDGET_LABEL: text = Rlabel_get_text(id); break;
			case R_WIDGET_TEXT_INPUT: text = Rtext_input_get_text(id); break;
			case R_WIDGET_TEXT_EDITOR: text = Rtext_editor_get_text(id); break;
			case R_WIDGET_CHECKBOX: text = Rcheckbox_get_text(id); break;
			case R_WIDGET_MENU_ITEM: text = Rmenu_item_get_text(id); break;
			case R_WIDGET_EXPANDER: text = Rexpander_get_title(id); break;
			case R_WIDGET_DATE_PICKER: text = Rdate_picker_get_text(id); break;
			case R_WIDGET_COLOR_PICKER: text = Rcolor_picker_get_hex(id); break;
			case R_WIDGET_COMBO_BOX: {
				int selected = Rcombo_box_get_selected(id);
				text = selected >= 0 ? Rcombo_box_get_item_text(id, selected) : "";
				break;
			}
			case R_WIDGET_LIST_VIEW: {
				int selected = Rlist_view_get_selected(id);
				text = selected >= 0 ? Rlist_view_get_item_text(id, selected) : "";
				break;
			}
			default: break;
		}
		if (!text) {
			rs_buf_printf(error, "the text of a %s cannot be read.", rvn_class_name(widget));
			return RS_CALL_FAILED;
		}
		*result = rs_value_cstring(text);
		return RS_CALL_OK;
	}
	if (strcmp(name, "ui.setVisible") == 0) {
		RVN_NEED_WIDGET(widget);
		Rwidget_set_visibility(widget->id, args[1].as.boolean ? R_VISIBLE : R_COLLAPSED);
		return RS_CALL_OK;
	}
	if (strcmp(name, "ui.isVisible") == 0) {
		RVN_NEED_WIDGET(widget);
		*result = rs_value_bool(Rwidget_get_visibility(widget->id) == R_VISIBLE);
		return RS_CALL_OK;
	}
	if (strcmp(name, "ui.setEnabled") == 0) {
		RVN_NEED_WIDGET(widget);
		Rwidget_set_enabled(widget->id, args[1].as.boolean);
		return RS_CALL_OK;
	}
	if (strcmp(name, "ui.isEnabled") == 0) {
		RVN_NEED_WIDGET(widget);
		*result = rs_value_bool(Rwidget_is_enabled(widget->id));
		return RS_CALL_OK;
	}
	if (strcmp(name, "ui.setValue") == 0 || strcmp(name, "ui.getValue") == 0) {
		int set = name[3] == 's';
		double value = set ? args[1].as.number : 0.0;
		int whole = (int)(value < -2147483647.0 ? -2147483647.0 : (value > 2147483647.0 ? 2147483647.0 : value));
		Rid id;
		RVN_NEED_WIDGET(widget);
		id = widget->id;
		switch (widget->cls->type) {
			case R_WIDGET_SLIDER: if (set) Rslider_set_value(id, (float)value); else value = Rslider_get_value(id); break;
			case R_WIDGET_SPIN_BOX: if (set) Rspin_box_set_value(id, value); else value = Rspin_box_get_value(id); break;
			case R_WIDGET_RATING: if (set) Rrating_set_value(id, (float)value); else value = Rrating_get_value(id); break;
			case R_WIDGET_PROGRESSBAR: if (set) Rprogress_bar_set_value(id, (float)value); else value = Rprogress_bar_get_value(id); break;
			case R_WIDGET_COMBO_BOX: if (set) Rcombo_box_set_selected(id, whole); else value = Rcombo_box_get_selected(id); break;
			case R_WIDGET_LIST_VIEW: if (set) Rlist_view_set_selected(id, whole); else value = Rlist_view_get_selected(id); break;
			case R_WIDGET_TAB_VIEW: if (set) Rtab_view_set_active(id, whole); else value = Rtab_view_get_active(id); break;
			case R_WIDGET_PAGINATION: if (set) Rpagination_set_page(id, whole); else value = Rpagination_get_page(id); break;
			case R_WIDGET_CAROUSEL: if (set) Rcarousel_set_page(id, whole, R_TRUE); else value = Rcarousel_get_page(id); break;
			default:
				rs_buf_printf(error, "a %s has no value (ui.%s works on a Slider, a SpinBox, a Rating, a ProgressBar, a ComboBox, a ListView, a TabView, a Pagination and a Carousel).", rvn_class_name(widget), name + 3);
				return RS_CALL_FAILED;
		}
		if (!set) *result = rs_value_number(value);
		return RS_CALL_OK;
	}
	if (strcmp(name, "ui.setChecked") == 0 || strcmp(name, "ui.isChecked") == 0) {
		int set = name[3] == 's';
		int checked = set ? args[1].as.boolean : 0;
		Rid id;
		RVN_NEED_WIDGET(widget);
		id = widget->id;
		switch (widget->cls->type) {
			case R_WIDGET_CHECKBOX: if (set) Rcheckbox_set_checked(id, checked); else checked = Rcheckbox_is_checked(id); break;
			case R_WIDGET_EXPANDER: if (set) Rexpander_set_expanded(id, checked); else checked = Rexpander_is_expanded(id); break;
			case R_WIDGET_MENU_ITEM: if (set) Rmenu_item_set_checked(id, checked); else checked = Rmenu_item_is_checked(id); break;
			default:
				rs_buf_printf(error, "a %s cannot be checked (a Checkbox, an Expander or a MenuItem can).", rvn_class_name(widget));
				return RS_CALL_FAILED;
		}
		if (!set) *result = rs_value_bool(checked);
		return RS_CALL_OK;
	}
	if (strcmp(name, "ui.setAttribute") == 0) {
		RVN_NEED_WIDGET(widget);
		if (!rvn_widget_set_attribute(widget, rs_value_text(args[1]), rs_value_text(args[2]))) {
			rs_buf_printf(error, "a %s has no attribute %s, or \"%s\" is not a value of it.", rvn_class_name(widget), rs_value_text(args[1]), rs_value_text(args[2]));
			return RS_CALL_FAILED;
		}
		return RS_CALL_OK;
	}
	if (strcmp(name, "ui.focus") == 0) {
		RVN_NEED_WIDGET(widget);
		Rwidget_focus(widget->id);
		return RS_CALL_OK;
	}
	return RS_CALL_NOT_FOUND;
}

static rs_call_status rvn_script_call(void* user_data, const char* name, const rs_value* args, int count, rs_value* result, rs_buf* error) {
	RActionCallback callback;
	void* bound;
	Rid sender = rvn_script_sender();
	RVN_UNUSED(user_data);

	/* what the application bound replaces the natives of the same name */
	if (rvn_action_lookup(name, &callback, &bound)) {
		if (callback == rvn_script_trampoline) {
			rvn_binding_t* binding = (rvn_binding_t*)bound;
			int i;
			if (binding->is_script) {
				/* (script functions are called by the virtual machine itself) */
				rs_buf_printf(error, "'%s' is a script function.", name);
				return RS_CALL_FAILED;
			}
			/* a declaration (declare fn) may not match what the application bound later */
			if (count != binding->sig.count) {
				rs_buf_printf(error, "it takes %d argument%s, but %d %s given (its declaration does not match the function bound by the application).",
					binding->sig.count, binding->sig.count == 1 ? "" : "s", count, count == 1 ? "was" : "were");
				return RS_CALL_FAILED;
			}
			for (i = 0; i < count; i++) {
				if (!rs_type_equal(rs_value_static_type(args[i]), binding->sig.params[i])) {
					rs_buf_printf(error, "argument %d is a '%s', but the function bound by the application takes a '%s'.", i + 1,
						rs_type_name(rs_value_static_type(args[i])), rs_type_name(binding->sig.params[i]));
					return RS_CALL_FAILED;
				}
			}
			if (!rvn_binding_call(binding, sender, name, args, count, result, error)) return RS_CALL_FAILED;
			return RS_CALL_OK;
		}
		if (count == 0) {
			/* bound with Raction_bind (or a native without parameters the application replaced) */
			Raction_fire(name, sender);
			return RS_CALL_OK;
		}
	}

	if (strcmp(name, "app.log") == 0) {
		if (g_script.log) g_script.log(rs_value_text(args[0]), g_script.log_user);
		else {
			printf("%s\n", rs_value_text(args[0]));
			fflush(stdout);
		}
		return RS_CALL_OK;
	}
	if (strcmp(name, "app.quit") == 0) {
		Raction_fire("app.quit", sender);
		return RS_CALL_OK;
	}
	if (strcmp(name, "app.fire") == 0) {
		const char* action = rs_value_text(args[0]);
		if (!action[0]) {
			rs_buf_puts(error, "the name of the action is empty.");
			return RS_CALL_FAILED;
		}
		Raction_fire(action, sender);
		return RS_CALL_OK;
	}
	if (strncmp(name, "ui.", 3) == 0) return rvn_script_ui(name, args, count, result, error);
	if (strncmp(name, "window.", 7) == 0) {
		rvn_window_t* window = rvn_script_window();
		if (!window) {
			rs_buf_puts(error, "the script runs outside of any window (no widget fired it, or it is not shown).");
			return RS_CALL_FAILED;
		}
		if (strcmp(name, "window.close") == 0) Rwindow_close(window->id);
		else if (strcmp(name, "window.minimize") == 0) Rwindow_minimize(window->id);
		else if (strcmp(name, "window.maximize") == 0) {
			if (Rwindow_is_maximized(window->id)) Rwindow_restore(window->id);
			else Rwindow_maximize(window->id);
		} else if (strcmp(name, "window.restore") == 0) Rwindow_restore(window->id);
		else if (strcmp(name, "window.setTitle") == 0) Rwindow_set_title(window->id, rs_value_text(args[0]));
		else return RS_CALL_NOT_FOUND;
		return RS_CALL_OK;
	}
	/* the actions Raven knows by itself (theme.dark, island.toggle...) */
	if (count == 0 && Raction_is_builtin(name)) {
		Raction_fire(name, sender);
		return RS_CALL_OK;
	}
	return RS_CALL_NOT_FOUND;
}

static void rvn_script_installed(void* user_data, const char* name) {
	rvn_binding_t* binding = rvn_binding_get(name);
	RVN_UNUSED(user_data);
	binding->is_script = R_TRUE;
	binding->callback = NULL;
	binding->user_data = NULL;
	Raction_bind(name, rvn_script_trampoline, binding);
}

static void rvn_script_removed(void* user_data, const char* name) {
	rvn_binding_t* binding = rvn_binding_active(name);
	RVN_UNUSED(user_data);
	if (binding && binding->is_script) Raction_unbind(name);
}

static int rvn_script_element_exists(void* user_data, const char* name) {
	rvn_script_document_t* document = g_script.current;
	uint32_t i;
	RVN_UNUSED(user_data);
	/* only while a document is compiled, against the names it gives its widgets */
	if (!document || !document->batch) return -1;
	for (i = 0; i < document->name_count; i++) {
		if (strcmp(document->names[i], name) == 0) return 1;
	}
	/* (a widget of another window, or made in C: it may exist) */
	return rvn_script_widget(name) ? 1 : 0;
}

static void* rvn_script_realloc(void* pointer, size_t size) {
	return rvn_realloc(pointer, size);
}

static rs_env* rvn_script_env(void) {
	if (!g_script.env) {
		rs_host host;
		memset(&host, 0, sizeof(host));
		host.diagnostic = rvn_script_report;
		host.lookup = rvn_script_lookup;
		host.call = rvn_script_call;
		host.function_installed = rvn_script_installed;
		host.function_removed = rvn_script_removed;
		host.element_exists = rvn_script_element_exists;
		/* the memory of the scripts is the memory of Raven (its statistics, its allocator) */
		rs_set_allocator(rvn_script_realloc, rvn_free);
		g_script.env = rs_env_create(&host);
		rs_env_set_limits(g_script.env, g_script.max_instructions, g_script.max_depth, g_script.max_memory);
	}
	return g_script.env;
}

/* ---------------------------------------------------------------------------
 *  Documents
 * ------------------------------------------------------------------------- */

const char* rvn_script_set_file(const char* path) {
	const char* previous = g_script.file;
	g_script.file = path;
	return previous;
}

static void rvn_script_note_name(rvn_script_document_t* scripts, const char* name) {
	RVN_ARRAY_RESERVE(scripts->names, scripts->name_cap, scripts->name_count + 1);
	scripts->names[scripts->name_count++] = rvn_strdup(name);
}

/* Takes the <Script> elements out of the tree (into `found`) and notes the names of the widgets. */
static void rvn_script_collect(rvn_script_document_t* scripts, rvn_xml_node_t* node, rvn_xml_node_t*** found, uint32_t* count, uint32_t* cap, int depth) {
	rvn_xml_node_t* child;
	rvn_xml_node_t* previous = NULL;
	const char* name = rvn_xml_attribute(node, "Name");
	if (!name) name = rvn_xml_attribute(node, "Id");
	if (name && name[0]) rvn_script_note_name(scripts, name);
	if (depth > RVN_XML_MAX_DEPTH) return;
	child = node->first_child;
	while (child) {
		rvn_xml_node_t* next = child->next;
		if (rvn_xml_is_script(child->name)) {
			if (previous) previous->next = next;
			else node->first_child = next;
			if (node->last_child == child) node->last_child = previous;
			child->next = NULL;
			RVN_ARRAY_RESERVE(*found, *cap, *count + 1);
			(*found)[(*count)++] = child;
		} else {
			if (strcmp(child->name, RVN_XML_COMMENT) != 0) rvn_script_collect(scripts, child, found, count, cap, depth + 1);
			previous = child;
		}
		child = next;
	}
}

/* The text of a line of the source of the document (excerpts). */
static void rvn_script_source_line(const rvn_script_document_t* scripts, int line, rs_buf* out) {
	const char* p = scripts->source;
	const char* end = p ? p + scripts->source_size : NULL;
	int current = 1;
	out->length = 0;
	if (!p || line < 1) return;
	while (p < end && current < line) {
		if (*p == '\n') current++;
		p++;
	}
	if (current != line) return;
	{
		const char* stop = p;
		while (stop < end && *stop != '\n' && *stop != '\r') stop++;
		rs_buf_put(out, p, (size_t)(stop - p));
	}
}

rvn_script_document_t* rvn_script_document_begin(rvn_xml_document_t* document, const char* path) {
	rvn_script_document_t* scripts = (rvn_script_document_t*)rvn_alloc(sizeof(rvn_script_document_t));
	rvn_xml_node_t** found = NULL;
	uint32_t count = 0, cap = 0, i;
	memset(scripts, 0, sizeof(*scripts));
	scripts->path = rvn_strdup(path ? path : (g_script.file ? g_script.file : "<xml>"));
	scripts->source = document->source;
	scripts->source_size = document->source_size;
	if (document->root) {
		if (rvn_xml_is_script(document->root->name)) {
			rvn_error(R_ERROR_XML, R_SEVERITY_ERROR, "%s: line %d, column %d: a <Script> cannot be the root of a document", scripts->path, document->root->line, document->root->column);
		} else {
			rvn_script_collect(scripts, document->root, &found, &count, &cap, 0);
		}
	}
	scripts->previous = g_script.current;
	g_script.current = scripts;
	if (count == 0 || g_script.disabled) {
		rvn_free(found);
		return scripts;
	}
	scripts->batch = rs_batch_create(rvn_script_env(), 0);
	for (i = 0; i < count; i++) {
		rvn_xml_node_t* node = found[i];
		const rvn_xml_attribute_t* attribute;
		const char* src = NULL;
		int has_text = R_FALSE;
		const char* p;
		for (attribute = node->attributes; attribute; attribute = attribute->next) {
			if (rvn_stricmp(attribute->name, "Src") == 0) src = attribute->value;
			else rvn_error(R_ERROR_XML, R_SEVERITY_WARNING, "%s: line %d, column %d: <Script> has no attribute %s (only Src)", scripts->path, node->line, node->column, attribute->name);
		}
		for (p = node->text; *p; p++) {
			if (!(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
				has_text = R_TRUE;
				break;
			}
		}
		if (src) {
			char* resolved;
			char* text;
			size_t size = 0;
			if (has_text) rvn_error(R_ERROR_XML, R_SEVERITY_WARNING, "%s: line %d, column %d: this <Script> has a Src and a text: both are used", scripts->path, node->line, node->column);
			resolved = rvn_xml_resolve_path(src);
			text = resolved ? rvn_read_file(resolved, &size) : NULL;
			if (!text) {
				rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "%s: line %d, column %d: <Script Src=\"%s\">: cannot read '%s'", scripts->path, node->line, node->column, src, resolved ? resolved : src);
				/* (the document still counts it: its functions are missing, the others must not be installed alone) */
				rs_batch_add(scripts->batch, resolved ? resolved : src, "", 0, 1, 1);
				{
					/* (counted as an error of the batch: nothing of the document is installed) */
					rs_diag diag;
					memset(&diag, 0, sizeof(diag));
					diag.severity = RS_SEVERITY_ERROR;
					diag.code = "RS101";
					diag.title = "Unknown function or variable.";
					diag.message = "A script file of this document could not be read: its scripts are not installed.";
					rs_batch_report(scripts->batch, &diag);
				}
			} else {
				rs_batch_add(scripts->batch, resolved, text, size, 1, 1);
				/* live reload: a change in the script is a change in the document */
				rvn_live_dependency(resolved);
			}
			rvn_free(text);
			rvn_free(resolved);
		}
		if (has_text) rs_batch_add(scripts->batch, scripts->path, node->text, strlen(node->text), node->text_line > 0 ? node->text_line : node->line, node->text_column > 0 ? node->text_column : node->column);
	}
	rvn_free(found);
	if (rs_batch_compile(scripts->batch) == 0) rs_batch_install(scripts->batch);
	return scripts;
}

void rvn_script_document_end(rvn_script_document_t* scripts, Rid built) {
	uint32_t i;
	if (!scripts) return;
	if (g_script.current == scripts) g_script.current = scripts->previous;
	if (scripts->batch) {
		Rid previous = g_script.built;
		/* the initializers run once the widgets exist: ui.setText("title", ...) finds its widget */
		g_script.built = built;
		rs_batch_initialize(scripts->batch);
		g_script.built = previous;
		rs_batch_destroy(scripts->batch);
	}
	if (scripts->empty) rs_batch_destroy(scripts->empty);
	for (i = 0; i < scripts->name_count; i++) rvn_free(scripts->names[i]);
	rvn_free(scripts->names);
	rvn_free(scripts->path);
	rvn_free(scripts);
}

void rvn_script_check_action(const rvn_xml_node_t* node, const char* attribute, const char* action) {
	rvn_script_document_t* scripts = g_script.current;
	rs_batch* batch;
	rs_location where;
	rs_buf line = { 0 };
	char* name;
	if (!scripts || g_script.disabled || !action || !action[0]) return;
	/* nothing to check against */
	if (!scripts->batch && (!g_script.env || rs_env_function_count(g_script.env) == 0)) return;
	batch = scripts->batch;
	if (!batch) {
		if (!scripts->empty) {
			scripts->empty = rs_batch_create(g_script.env, 0);
			rs_batch_compile(scripts->empty);
		}
		batch = scripts->empty;
	}
	name = rvn_action_name_normalize(action);
	memset(&where, 0, sizeof(where));
	where.file = scripts->path;
	where.line = node->line;
	where.column = node->column;
	where.end_line = node->line;
	where.end_column = node->column + 1 + (int)strlen(node->name);
	where.unit = -1;
	where.start = -1;
	where.end = -1;
	rvn_script_source_line(scripts, node->line, &line);
	if (line.data) where.line_text = line.data;
	rs_batch_check_handler(batch, node->name, attribute, name, &where);
	rs_buf_free(&line);
	rvn_free(name);
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

static int rvn_script_load_text(const char* source, size_t size, const char* name) {
	rs_batch* batch;
	int ok;
	batch = rs_batch_create(rvn_script_env(), 0);
	rs_batch_add(batch, name && name[0] ? name : "<script>", source, size, 1, 1);
	ok = rs_batch_compile(batch) == 0 && rs_batch_install(batch);
	if (ok) ok = rs_batch_initialize(batch);
	rs_batch_destroy(batch);
	return ok;
}

int Rscript_load(const char* source, size_t size, const char* name) {
	RVN_API_GUARD(R_FALSE);
	if (!source) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rscript_load: NULL text");
		return R_FALSE;
	}
	if (g_script.disabled) return R_FALSE;
	return rvn_script_load_text(source, size, name);
}

int Rscript_load_file(const char* path) {
	size_t size = 0;
	char* text;
	int ok;
	RVN_API_GUARD(R_FALSE);
	text = path ? rvn_read_file(path, &size) : NULL;
	if (!text) {
		rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "Rscript_load_file: cannot read '%s'", path ? path : "");
		return R_FALSE;
	}
	ok = g_script.disabled ? R_FALSE : rvn_script_load_text(text, size, path);
	rvn_free(text);
	return ok;
}

/* The checks of an XML document: its scripts, the functions its actions name. */
static void rvn_script_check_nodes(rvn_script_document_t* scripts, const rvn_xml_node_t* node, int depth) {
	const rvn_xml_attribute_t* attribute;
	const rvn_xml_node_t* child;
	if (depth > RVN_XML_MAX_DEPTH || strcmp(node->name, RVN_XML_COMMENT) == 0) return;
	for (attribute = node->attributes; attribute; attribute = attribute->next) {
		if (rs_is_action_attribute(attribute->name)) rvn_script_check_action(node, attribute->name, attribute->value);
	}
	for (child = node->first_child; child; child = child->next) rvn_script_check_nodes(scripts, child, depth + 1);
}

int Rscript_check(const char* source, size_t size, const char* name) {
	const char* p = source;
	const char* end;
	rs_env* env;
	int errors;
	rs_host host;
	RVN_API_GUARD(-1);
	if (!source) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rscript_check: NULL text");
		return -1;
	}
	end = source + size;
	if (size >= 3 && memcmp(p, "\xEF\xBB\xBF", 3) == 0) p += 3;
	while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
	/* a check runs nothing and installs nothing: an environment of its own, that sees the functions the application bound */
	env = rvn_script_env();
	host = *rs_env_host(env);
	host.function_installed = NULL;
	host.function_removed = NULL;
	if (p < end && *p == '<') {
		rvn_xml_document_t document;
		rvn_script_document_t* scripts;
		int previous_disabled = g_script.disabled;
		rs_env* saved = g_script.env;
		const char* previous_file = rvn_script_set_file(name && name[0] ? name : "<xml>");
		if (!rvn_xml_load(&document, source, size, name && name[0] ? name : "Rscript_check")) {
			rvn_script_set_file(previous_file);
			return 1;
		}
		/* (compiled in a separate environment: nothing is installed in the real one) */
		g_script.env = rs_env_create(&host);
		g_script.disabled = R_FALSE;
		scripts = rvn_script_document_begin(&document, NULL);
		errors = scripts->batch ? rs_batch_error_count(scripts->batch) : 0;
		if (document.root) rvn_script_check_nodes(scripts, document.root, 0);
		if (scripts->batch) errors = rs_batch_error_count(scripts->batch);
		if (scripts->empty) errors += rs_batch_error_count(scripts->empty);
		/* (no initializer runs) */
		if (scripts->batch) {
			rs_batch_destroy(scripts->batch);
			scripts->batch = NULL;
		}
		rvn_script_document_end(scripts, R_INVALID_ID);
		rs_env_destroy(g_script.env);
		g_script.env = saved;
		g_script.disabled = previous_disabled;
		rvn_script_set_file(previous_file);
		rvn_xml_free(&document);
		return errors;
	}
	{
		rs_env* check = rs_env_create(&host);
		rs_batch* batch = rs_batch_create(check, 0);
		rs_batch_add(batch, name && name[0] ? name : "<script>", source, size, 1, 1);
		errors = rs_batch_compile(batch);
		rs_batch_destroy(batch);
		rs_env_destroy(check);
	}
	return errors;
}

int Rscript_check_file(const char* path) {
	size_t size = 0;
	char* text;
	int errors;
	char* previous_dir;
	RVN_API_GUARD(-1);
	text = path ? rvn_read_file(path, &size) : NULL;
	if (!text) {
		rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "Rscript_check_file: cannot read '%s'", path ? path : "");
		return -1;
	}
	/* (the Src of its scripts are relative to it) */
	rvn_xml_push_file_dir(path, &previous_dir);
	errors = Rscript_check(text, size, path);
	rvn_xml_pop_file_dir(previous_dir);
	rvn_free(text);
	return errors;
}

const char* Rscript_format(const char* source, size_t size, const char* indent) {
	rs_format_options options;
	RVN_API_GUARD(NULL);
	if (!source) return NULL;
	rvn_script_env();
	memset(&options, 0, sizeof(options));
	options.indent = indent;
	rs_free(g_script.format_result);
	g_script.format_result = rs_format(source, size, &options);
	return g_script.format_result;
}

void Rscript_reset(void) {
	RVN_API_GUARD_VOID();
	if (!g_script.env) return;
	if (rs_env_running(g_script.env)) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rscript_reset: a script is running");
		return;
	}
	rs_env_reset(g_script.env);
}

int Rscript_has_function(const char* name) {
	RVN_API_GUARD(R_FALSE);
	return name && g_script.env && rs_env_find_function(g_script.env, name, NULL);
}

void Rscript_set_enabled(int enabled) {
	RVN_API_GUARD_VOID();
	g_script.disabled = !enabled;
}

int Rscript_is_enabled(void) {
	return !g_script.disabled;
}

void Rscript_set_limits(Ruint max_instructions, int max_depth, size_t max_memory) {
	RVN_API_GUARD_VOID();
	if (max_instructions) g_script.max_instructions = max_instructions;
	if (max_depth > 0) g_script.max_depth = max_depth;
	if (max_memory) g_script.max_memory = max_memory;
	if (g_script.env) rs_env_set_limits(g_script.env, max_instructions, max_depth, max_memory);
}

void Rscript_set_log_callback(RScriptLogCallback callback, void* user_data) {
	RVN_API_GUARD_VOID();
	g_script.log = callback;
	g_script.log_user = user_data;
}

void Rscript_set_diagnostic_callback(RScriptDiagnosticCallback callback, void* user_data) {
	RVN_API_GUARD_VOID();
	g_script.diagnostic = callback;
	g_script.diagnostic_user = user_data;
}

void rvn_script_shutdown(void) {
	rvn_binding_t* binding = g_script.bindings;
	/* the actions bound to the trampoline go: it would point to freed bindings */
	while (binding) {
		rvn_binding_t* next = binding->next;
		if (rvn_binding_active(binding->name) == binding) Raction_unbind(binding->name);
		rvn_free(binding->name);
		rvn_free(binding);
		binding = next;
	}
	if (g_script.env) rs_env_destroy(g_script.env);
	rs_free(g_script.format_result);
	rs_set_allocator(NULL, NULL);
	memset(&g_script, 0, sizeof(g_script));
}
