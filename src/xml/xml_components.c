/**
 * Raven - xml/xml_components.c
 * Widgets declared in XML, and what makes a description more than a list of
 * tags: parameters, expressions, conditions, repetitions, included files.
 *
 *     <Widget Name="Card" Title="Untitled" Wide="false">
 *       <Border Padding="12" Width="{Wide ? 320 : 200}">
 *         <VBox Spacing="6">
 *           <Label Text="{Title}" TextSize="17"/>
 *           <Children/>
 *         </VBox>
 *       </Border>
 *     </Widget>
 *     ...
 *     <Card Title="Hello" Slot.Fill="1"><Button Text="Inside"/></Card>
 *
 * Nothing of this reaches the widgets: a parsed document is rewritten into a
 * plain one before it is built (rvn_xml_expand). Each use of a declared
 * widget is replaced by a copy of its body in which the {expressions} are
 * computed, <Children/> is replaced by the children given at the use, and the
 * attributes that are not parameters are given to the root of the copy.
 *
 * {expressions} are only looked for where there are variables: inside the
 * body of a <Widget>, of a <Repeat> or of a <ForEach>. Anywhere else a brace
 * is a brace, so existing documents (source code in a text editor...) keep
 * their meaning. Where they are looked for, "{{" and "}}" write a brace.
 */

#include "xml.h"
#include "xml_watch.h"
#include "../widgets/common.h"

#define RVN_XC_MAX_DEPTH 48

typedef struct rvn_component_t {
	char* name;
	/** Owns the copy of the <Widget> element, which is its root. */
	rvn_xml_document_t document;
} rvn_component_t;

static struct {
	rvn_component_t* items;
	uint32_t count;
	uint32_t cap;
	/** Number of instances made so far: the value of {Instance}. */
	uint32_t instances;
	/**
	 * Declarations replaced while a document is being expanded: what is being
	 * expanded may be one of them (a <Widget> declared again inside its own
	 * body, or inside a use of it). They are freed once the expansion is over.
	 */
	rvn_component_t* retired;
	uint32_t retired_count, retired_cap;
	int expanding;
} g_components;

typedef struct rvn_xc_var_t {
	const char* name;
	const char* value;
	struct rvn_xc_var_t* next;
} rvn_xc_var_t;

typedef struct rvn_xc_scope_t {
	rvn_xc_var_t* vars;
	/** Enclosing variables (<Repeat> inside a <Widget>); NULL for the body of a <Widget>, which only sees its parameters. */
	const struct rvn_xc_scope_t* parent;
	/** Where the widget being expanded is used: its children are what <Children/> stands for. */
	const rvn_xml_node_t* use;
	const struct rvn_xc_scope_t* use_scope;
} rvn_xc_scope_t;

typedef struct rvn_xc_t {
	rvn_xml_document_t* document;
	int depth;
	int failed;
	int registered;
} rvn_xc_t;

static void rvn_xc_expand(rvn_xc_t* ctx, const rvn_xml_node_t* node, const rvn_xc_scope_t* scope, rvn_xml_node_t* holder);

/* ---------------------------------------------------------------------------
 *  Messages
 * ------------------------------------------------------------------------- */

static void rvn_xc_report(const rvn_xml_node_t* node, RSeverity severity, const char* format, ...) {
	char message[256];
	va_list args;
	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);
	rvn_error(R_ERROR_XML, severity, "XML line %d, column %d: %s", node ? node->line : 0, node ? node->column : 0, message);
}

/* ---------------------------------------------------------------------------
 *  Nodes
 * ------------------------------------------------------------------------- */

static rvn_xml_node_t* rvn_xc_node(rvn_arena_t* arena, const rvn_xml_node_t* like, const char* name) {
	rvn_xml_node_t* node = (rvn_xml_node_t*)rvn_arena_alloc(arena, sizeof(rvn_xml_node_t));
	memset(node, 0, sizeof(*node));
	node->name = rvn_arena_strndup(arena, name, strlen(name));
	node->text = "";
	if (like) {
		node->line = like->line;
		node->column = like->column;
		node->source = like->source;
	}
	return node;
}

static void rvn_xc_append(rvn_xml_node_t* parent, rvn_xml_node_t* child) {
	child->parent = parent;
	child->next = NULL;
	if (parent->last_child) parent->last_child->next = child;
	else parent->first_child = child;
	parent->last_child = child;
}

/* Sets an attribute: replaces the value of one that exists (names without case), else adds it at the end. */
static void rvn_xc_set_attribute(rvn_arena_t* arena, rvn_xml_node_t* node, const char* name, const char* value) {
	rvn_xml_attribute_t* attribute;
	rvn_xml_attribute_t* last = NULL;
	for (attribute = node->attributes; attribute; attribute = attribute->next) {
		if (rvn_stricmp(attribute->name, name) == 0) {
			attribute->value = value;
			return;
		}
		last = attribute;
	}
	attribute = (rvn_xml_attribute_t*)rvn_arena_alloc(arena, sizeof(rvn_xml_attribute_t));
	attribute->name = rvn_arena_strndup(arena, name, strlen(name));
	attribute->value = value;
	attribute->next = NULL;
	if (last) last->next = attribute;
	else node->attributes = attribute;
}

static void rvn_xc_remove_attribute(rvn_xml_node_t* node, const char* name) {
	rvn_xml_attribute_t** link = &node->attributes;
	while (*link) {
		if (rvn_stricmp((*link)->name, name) == 0) *link = (*link)->next;
		else link = &(*link)->next;
	}
}

/* Copies an element and everything under it, as it is. */
static rvn_xml_node_t* rvn_xc_copy(rvn_arena_t* arena, const rvn_xml_node_t* source) {
	rvn_xml_node_t* node = rvn_xc_node(arena, source, source->name);
	const rvn_xml_attribute_t* attribute;
	const rvn_xml_node_t* child;
	rvn_xml_attribute_t* last = NULL;
	node->text = rvn_arena_strndup(arena, source->text, strlen(source->text));
	for (attribute = source->attributes; attribute; attribute = attribute->next) {
		rvn_xml_attribute_t* copy = (rvn_xml_attribute_t*)rvn_arena_alloc(arena, sizeof(rvn_xml_attribute_t));
		copy->name = rvn_arena_strndup(arena, attribute->name, strlen(attribute->name));
		copy->value = rvn_arena_strndup(arena, attribute->value, strlen(attribute->value));
		copy->next = NULL;
		if (last) last->next = copy;
		else node->attributes = copy;
		last = copy;
	}
	for (child = source->first_child; child; child = child->next) rvn_xc_append(node, rvn_xc_copy(arena, child));
	return node;
}

/* ---------------------------------------------------------------------------
 *  Registry
 * ------------------------------------------------------------------------- */

static rvn_component_t* rvn_component_find(const char* name) {
	uint32_t i;
	for (i = 0; i < g_components.count; i++) {
		if (rvn_stricmp(g_components.items[i].name, name) == 0) return &g_components.items[i];
	}
	return NULL;
}

static void rvn_component_release(rvn_component_t* component) {
	rvn_free(component->name);
	rvn_xml_free(&component->document);
}

/* A declaration goes away: at once, or after the expansion that may be reading it. */
static void rvn_component_retire(rvn_component_t* component) {
	if (g_components.expanding <= 0) {
		rvn_component_release(component);
		return;
	}
	RVN_ARRAY_RESERVE(g_components.retired, g_components.retired_cap, g_components.retired_count + 1);
	g_components.retired[g_components.retired_count++] = *component;
}

static void rvn_components_free_retired(void) {
	uint32_t i;
	for (i = 0; i < g_components.retired_count; i++) rvn_component_release(&g_components.retired[i]);
	g_components.retired_count = 0;
}

void rvn_xml_components_shutdown(void) {
	uint32_t i;
	for (i = 0; i < g_components.count; i++) rvn_component_release(&g_components.items[i]);
	rvn_components_free_retired();
	rvn_free(g_components.retired);
	rvn_free(g_components.items);
	memset(&g_components, 0, sizeof(g_components));
}

static int rvn_xc_reserved(const char* name) {
	static const char* const reserved[] = {
		"Window", "Widget", "Widgets", "Children", "Repeat", "ForEach", "If", "Include", "Strings", "Shortcut", "Item", "Separator"
	};
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(reserved); i++) {
		if (rvn_stricmp(name, reserved[i]) == 0) return R_TRUE;
	}
	return R_FALSE;
}

static void rvn_xc_clear_source(rvn_xml_node_t* node) {
	rvn_xml_node_t* child;
	node->source = 0;
	for (child = node->first_child; child; child = child->next) rvn_xc_clear_source(child);
}

/* Keeps a <Widget Name="..."> for later uses of its tag. Declaring a name again replaces it. */
static void rvn_xc_register(rvn_xc_t* ctx, const rvn_xml_node_t* definition) {
	const char* name = rvn_xml_attribute(definition, "Name");
	rvn_component_t* component;
	const char* cursor;

	if (!name || !name[0]) {
		rvn_xc_report(definition, R_SEVERITY_ERROR, "<Widget> needs a Name: the tag it declares");
		return;
	}
	for (cursor = name; *cursor; cursor++) {
		char c = *cursor;
		int letter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
		if (!letter && !(cursor > name && ((c >= '0' && c <= '9') || c == '.' || c == '-'))) {
			rvn_xc_report(definition, R_SEVERITY_ERROR, "<Widget Name=\"%.60s\">: this cannot be the name of a tag", name);
			return;
		}
	}
	if (rvn_xc_reserved(name) || rvn_widget_class_by_name(name)) {
		rvn_xc_report(definition, R_SEVERITY_ERROR, "<Widget Name=\"%.60s\">: this tag already exists", name);
		return;
	}
	if (!definition->first_child) {
		rvn_xc_report(definition, R_SEVERITY_ERROR, "<Widget Name=\"%.60s\"> is empty: it needs the element it stands for", name);
		return;
	}
	component = rvn_component_find(name);
	if (component) {
		rvn_component_retire(component);
	} else {
		RVN_ARRAY_RESERVE(g_components.items, g_components.cap, g_components.count + 1);
		component = &g_components.items[g_components.count++];
	}
	memset(component, 0, sizeof(*component));
	component->name = rvn_strdup(name);
	rvn_arena_init(&component->document.arena, 4096);
	component->document.root = rvn_xc_copy(&component->document.arena, definition);
	/* (what a use of it becomes comes from the use, not from here) */
	rvn_xc_clear_source(component->document.root);
	ctx->registered++;
}

/* ---------------------------------------------------------------------------
 *  Variables
 * ------------------------------------------------------------------------- */

static rvn_xc_var_t* rvn_xc_find_var(const rvn_xc_scope_t* scope, const char* name, size_t length) {
	for (; scope; scope = scope->parent) {
		rvn_xc_var_t* var;
		for (var = scope->vars; var; var = var->next) {
			if (strlen(var->name) == length && rvn_strnicmp(var->name, name, length) == 0) return var;
		}
	}
	return NULL;
}

static rvn_xc_var_t* rvn_xc_add_var(rvn_xc_t* ctx, rvn_xc_scope_t* scope, const char* name, const char* value) {
	rvn_xc_var_t* var = (rvn_xc_var_t*)rvn_arena_alloc(&ctx->document->arena, sizeof(rvn_xc_var_t));
	var->name = name;
	var->value = value;
	var->next = scope->vars;
	scope->vars = var;
	return var;
}

static const char* rvn_xc_number_text(rvn_xc_t* ctx, double number) {
	char buffer[48];
	if (number == floor(number) && fabs(number) < 1e15) snprintf(buffer, sizeof(buffer), "%.0f", number);
	else snprintf(buffer, sizeof(buffer), "%.6g", number);
	return rvn_arena_strndup(&ctx->document->arena, buffer, strlen(buffer));
}

/* ---------------------------------------------------------------------------
 *  Expressions
 *
 *    expression := or [ '?' expression ':' expression ]
 *    or         := and { '||' and }
 *    and        := compare { '&&' compare }
 *    compare    := sum [ ( '==' | '!=' | '<' | '<=' | '>' | '>=' ) sum ]
 *    sum        := product { ( '+' | '-' ) product }
 *    product    := unary { ( '*' | '/' | '%' ) unary }
 *    unary      := ( '!' | '-' ) unary | primary
 *    primary    := number | 'text' | name | name '(' arguments ')' | '(' expression ')'
 *
 *  A value is a text; it is also a number when it reads as one. '+' adds
 *  numbers and joins texts; "", "0" and "false" are false.
 * ------------------------------------------------------------------------- */

typedef struct rvn_xv_t {
	const char* text;
	double number;
	int is_number;
} rvn_xv_t;

typedef struct rvn_xe_t {
	rvn_xc_t* ctx;
	const rvn_xc_scope_t* scope;
	const rvn_xml_node_t* where;
	const char* cursor;
	const char* end;
	int failed;
	int depth;
} rvn_xe_t;

static rvn_xv_t rvn_xe_expression(rvn_xe_t* e);

static rvn_xv_t rvn_xv_text(const char* text) {
	rvn_xv_t value;
	char* end = NULL;
	value.text = text;
	value.number = 0.0;
	value.is_number = R_FALSE;
	if (text[0] && text[0] != ' ') {
		double number = strtod(text, &end);
		if (end && *end == '\0') {
			value.number = number;
			value.is_number = R_TRUE;
		}
	}
	return value;
}

static rvn_xv_t rvn_xv_number(rvn_xe_t* e, double number) {
	rvn_xv_t value;
	value.text = rvn_xc_number_text(e->ctx, number);
	value.number = number;
	value.is_number = R_TRUE;
	return value;
}

static rvn_xv_t rvn_xv_bool(int flag) {
	rvn_xv_t value;
	value.text = flag ? "true" : "false";
	value.number = flag ? 1.0 : 0.0;
	value.is_number = R_FALSE;
	return value;
}

static int rvn_xv_truth(rvn_xv_t value) {
	if (value.is_number) return value.number != 0.0;
	return value.text[0] != '\0' && rvn_stricmp(value.text, "false") != 0;
}

static void rvn_xe_fail(rvn_xe_t* e, const char* message) {
	if (e->failed) return;
	e->failed = R_TRUE;
	rvn_xc_report(e->where, R_SEVERITY_WARNING, "in an expression: %s", message);
}

static void rvn_xe_space(rvn_xe_t* e) {
	while (e->cursor < e->end && (*e->cursor == ' ' || *e->cursor == '\t' || *e->cursor == '\n' || *e->cursor == '\r')) e->cursor++;
}

static int rvn_xe_take(rvn_xe_t* e, const char* token) {
	size_t length = strlen(token);
	rvn_xe_space(e);
	if ((size_t)(e->end - e->cursor) < length || memcmp(e->cursor, token, length) != 0) return R_FALSE;
	e->cursor += length;
	return R_TRUE;
}

static rvn_xv_t rvn_xe_call(rvn_xe_t* e, const char* name, size_t length) {
	rvn_xv_t args[4];
	int count = 0;
	char text[32];

	rvn_xe_space(e);
	if (!rvn_xe_take(e, ")")) {
		do {
			if (count == 4) {
				rvn_xe_fail(e, "too many arguments");
				break;
			}
			args[count++] = rvn_xe_expression(e);
		} while (!e->failed && rvn_xe_take(e, ","));
		if (!e->failed && !rvn_xe_take(e, ")")) rvn_xe_fail(e, "expected ')'");
	}
	if (e->failed) return rvn_xv_text("");
	snprintf(text, sizeof(text), "%.*s", (int)(length < 30 ? length : 30), name);

	if (count == 2 && rvn_stricmp(text, "min") == 0) return rvn_xv_number(e, args[0].number < args[1].number ? args[0].number : args[1].number);
	if (count == 2 && rvn_stricmp(text, "max") == 0) return rvn_xv_number(e, args[0].number > args[1].number ? args[0].number : args[1].number);
	if (count == 1 && rvn_stricmp(text, "round") == 0) return rvn_xv_number(e, floor(args[0].number + 0.5));
	if (count == 1 && rvn_stricmp(text, "floor") == 0) return rvn_xv_number(e, floor(args[0].number));
	if (count == 1 && rvn_stricmp(text, "ceil") == 0) return rvn_xv_number(e, ceil(args[0].number));
	if (count == 1 && rvn_stricmp(text, "abs") == 0) return rvn_xv_number(e, fabs(args[0].number));
	if (count == 1 && rvn_stricmp(text, "len") == 0) return rvn_xv_number(e, (double)rvn_utf8_count(args[0].text, (int)strlen(args[0].text)));
	if (count == 1 && rvn_stricmp(text, "empty") == 0) return rvn_xv_bool(args[0].text[0] == '\0');
	/* the first argument that is not empty: {default(Title, 'Untitled')} */
	if (count == 2 && rvn_stricmp(text, "default") == 0) return args[0].text[0] ? args[0] : args[1];
	if (count == 1 && (rvn_stricmp(text, "upper") == 0 || rvn_stricmp(text, "lower") == 0)) {
		char* copy = rvn_arena_strndup(&e->ctx->document->arena, args[0].text, strlen(args[0].text));
		char* cursor;
		int upper = rvn_stricmp(text, "upper") == 0;
		for (cursor = copy; *cursor; cursor++) {
			if (upper && *cursor >= 'a' && *cursor <= 'z') *cursor = (char)(*cursor - 32);
			else if (!upper && *cursor >= 'A' && *cursor <= 'Z') *cursor = (char)(*cursor + 32);
		}
		return rvn_xv_text(copy);
	}
	rvn_xe_fail(e, "unknown function, or wrong number of arguments");
	return rvn_xv_text("");
}

static rvn_xv_t rvn_xe_primary(rvn_xe_t* e) {
	const char* start;
	rvn_xe_space(e);
	if (e->cursor >= e->end) {
		rvn_xe_fail(e, "something is missing");
		return rvn_xv_text("");
	}
	start = e->cursor;
	if (*start == '(') {
		rvn_xv_t value;
		e->cursor++;
		value = rvn_xe_expression(e);
		if (!e->failed && !rvn_xe_take(e, ")")) rvn_xe_fail(e, "expected ')'");
		return value;
	}
	if (*start == '\'' || *start == '"') {
		char quote = *start;
		const char* close = start + 1;
		while (close < e->end && *close != quote) close++;
		if (close >= e->end) {
			rvn_xe_fail(e, "a text is not closed");
			return rvn_xv_text("");
		}
		e->cursor = close + 1;
		/* (a quoted text is a text, even when it reads as a number) */
		{
			rvn_xv_t value;
			value.text = rvn_arena_strndup(&e->ctx->document->arena, start + 1, (size_t)(close - start - 1));
			value.number = strtod(value.text, NULL);
			value.is_number = R_FALSE;
			return value;
		}
	}
	if ((*start >= '0' && *start <= '9') || *start == '.') {
		char* end = NULL;
		char buffer[64];
		size_t length = (size_t)(e->end - start) < sizeof(buffer) - 1 ? (size_t)(e->end - start) : sizeof(buffer) - 1;
		double number;
		memcpy(buffer, start, length);
		buffer[length] = '\0';
		number = strtod(buffer, &end);
		if (end == buffer) {
			rvn_xe_fail(e, "a number is wrong");
			return rvn_xv_text("");
		}
		e->cursor = start + (end - buffer);
		return rvn_xv_number(e, number);
	}
	if ((*start >= 'a' && *start <= 'z') || (*start >= 'A' && *start <= 'Z') || *start == '_') {
		const char* close = start;
		size_t length;
		rvn_xc_var_t* var;
		while (close < e->end && ((*close >= 'a' && *close <= 'z') || (*close >= 'A' && *close <= 'Z') ||
			(*close >= '0' && *close <= '9') || *close == '_' || *close == '.')) close++;
		length = (size_t)(close - start);
		e->cursor = close;
		rvn_xe_space(e);
		if (e->cursor < e->end && *e->cursor == '(') {
			e->cursor++;
			return rvn_xe_call(e, start, length);
		}
		if (length == 4 && rvn_strnicmp(start, "true", 4) == 0) return rvn_xv_bool(R_TRUE);
		if (length == 5 && rvn_strnicmp(start, "false", 5) == 0) return rvn_xv_bool(R_FALSE);
		var = rvn_xc_find_var(e->scope, start, length);
		if (!var) {
			char message[96];
			snprintf(message, sizeof(message), "'%.*s' is not a parameter here", (int)(length < 60 ? length : 60), start);
			rvn_xe_fail(e, message);
			return rvn_xv_text("");
		}
		return rvn_xv_text(var->value);
	}
	rvn_xe_fail(e, "unexpected character");
	return rvn_xv_text("");
}

static rvn_xv_t rvn_xe_unary(rvn_xe_t* e) {
	rvn_xe_space(e);
	if (e->cursor < e->end && *e->cursor == '!' && !(e->cursor + 1 < e->end && e->cursor[1] == '=')) {
		e->cursor++;
		return rvn_xv_bool(!rvn_xv_truth(rvn_xe_unary(e)));
	}
	if (e->cursor < e->end && *e->cursor == '-') {
		e->cursor++;
		return rvn_xv_number(e, -rvn_xe_unary(e).number);
	}
	return rvn_xe_primary(e);
}

static rvn_xv_t rvn_xe_product(rvn_xe_t* e) {
	rvn_xv_t left = rvn_xe_unary(e);
	while (!e->failed) {
		rvn_xv_t right;
		char op;
		rvn_xe_space(e);
		if (e->cursor >= e->end || (*e->cursor != '*' && *e->cursor != '/' && *e->cursor != '%')) break;
		op = *e->cursor++;
		right = rvn_xe_unary(e);
		if (op == '*') left = rvn_xv_number(e, left.number * right.number);
		else if (right.number == 0.0) left = rvn_xv_number(e, 0.0);
		else if (op == '/') left = rvn_xv_number(e, left.number / right.number);
		else left = rvn_xv_number(e, fmod(left.number, right.number));
	}
	return left;
}

static rvn_xv_t rvn_xe_sum(rvn_xe_t* e) {
	rvn_xv_t left = rvn_xe_product(e);
	while (!e->failed) {
		rvn_xv_t right;
		char op;
		rvn_xe_space(e);
		if (e->cursor >= e->end || (*e->cursor != '+' && *e->cursor != '-')) break;
		op = *e->cursor++;
		right = rvn_xe_product(e);
		if (op == '-') {
			left = rvn_xv_number(e, left.number - right.number);
		} else if (left.is_number && right.is_number) {
			left = rvn_xv_number(e, left.number + right.number);
		} else {
			/* texts are joined */
			size_t a = strlen(left.text), b = strlen(right.text);
			char* joined = (char*)rvn_arena_alloc(&e->ctx->document->arena, a + b + 1);
			memcpy(joined, left.text, a);
			memcpy(joined + a, right.text, b + 1);
			left.text = joined;
			left.is_number = R_FALSE;
			left.number = 0.0;
		}
	}
	return left;
}

static rvn_xv_t rvn_xe_compare(rvn_xe_t* e) {
	static const char* const operators[] = { "==", "!=", "<=", ">=", "<", ">" };
	rvn_xv_t left = rvn_xe_sum(e), right;
	int which, order;
	for (which = 0; which < 6; which++) {
		if (rvn_xe_take(e, operators[which])) break;
	}
	if (which == 6 || e->failed) return left;
	right = rvn_xe_sum(e);
	if (left.is_number && right.is_number) order = left.number < right.number ? -1 : (left.number > right.number ? 1 : 0);
	else order = strcmp(left.text, right.text);
	switch (which) {
		case 0: return rvn_xv_bool(order == 0);
		case 1: return rvn_xv_bool(order != 0);
		case 2: return rvn_xv_bool(order <= 0);
		case 3: return rvn_xv_bool(order >= 0);
		case 4: return rvn_xv_bool(order < 0);
		default: return rvn_xv_bool(order > 0);
	}
}

static rvn_xv_t rvn_xe_and(rvn_xe_t* e) {
	rvn_xv_t left = rvn_xe_compare(e);
	while (!e->failed && rvn_xe_take(e, "&&")) {
		rvn_xv_t right = rvn_xe_compare(e);
		left = rvn_xv_bool(rvn_xv_truth(left) && rvn_xv_truth(right));
	}
	return left;
}

static rvn_xv_t rvn_xe_or(rvn_xe_t* e) {
	rvn_xv_t left = rvn_xe_and(e);
	while (!e->failed && rvn_xe_take(e, "||")) {
		rvn_xv_t right = rvn_xe_and(e);
		left = rvn_xv_bool(rvn_xv_truth(left) || rvn_xv_truth(right));
	}
	return left;
}

static rvn_xv_t rvn_xe_expression(rvn_xe_t* e) {
	rvn_xv_t condition;
	if (++e->depth > 64) {
		rvn_xe_fail(e, "nested too deeply");
		e->depth--;
		return rvn_xv_text("");
	}
	condition = rvn_xe_or(e);
	if (!e->failed && rvn_xe_take(e, "?")) {
		rvn_xv_t yes = rvn_xe_expression(e), no;
		if (!e->failed && !rvn_xe_take(e, ":")) rvn_xe_fail(e, "expected ':'");
		no = rvn_xe_expression(e);
		condition = rvn_xv_truth(condition) ? yes : no;
	}
	e->depth--;
	return condition;
}

/* Computes one expression (what is between two braces). */
static rvn_xv_t rvn_xc_evaluate(rvn_xc_t* ctx, const rvn_xc_scope_t* scope, const rvn_xml_node_t* where, const char* text, size_t length) {
	rvn_xe_t e;
	rvn_xv_t value;
	memset(&e, 0, sizeof(e));
	e.ctx = ctx;
	e.scope = scope;
	e.where = where;
	e.cursor = text;
	e.end = text + length;
	value = rvn_xe_expression(&e);
	rvn_xe_space(&e);
	if (!e.failed && e.cursor < e.end) rvn_xe_fail(&e, "something is left after the end");
	return e.failed ? rvn_xv_text("") : value;
}

/*
 * A text of the source with its {expressions} computed. Without variables
 * around (scope NULL), the text is copied as it is.
 */
static const char* rvn_xc_text(rvn_xc_t* ctx, const rvn_xc_scope_t* scope, const rvn_xml_node_t* where, const char* text) {
	rvn_arena_t* arena = &ctx->document->arena;
	char* out = NULL;
	uint32_t count = 0, cap = 0;
	const char* cursor = text;
	const char* result;

	if (!scope || (!strchr(text, '{') && !strchr(text, '}'))) return rvn_arena_strndup(arena, text, strlen(text));
	while (*cursor) {
		if ((cursor[0] == '{' && cursor[1] == '{') || (cursor[0] == '}' && cursor[1] == '}')) {
			RVN_ARRAY_RESERVE(out, cap, count + 2);
			out[count++] = cursor[0];
			cursor += 2;
		} else if (cursor[0] == '{') {
			const char* close = cursor + 1;
			char quote = 0;
			rvn_xv_t value;
			size_t length;
			/* (a closing brace inside a quoted text does not end the expression) */
			while (*close && (quote || *close != '}')) {
				if (quote && *close == quote) quote = 0;
				else if (!quote && (*close == '\'' || *close == '"')) quote = *close;
				close++;
			}
			if (!*close) {
				rvn_xc_report(where, R_SEVERITY_WARNING, "'{' without its '}' (write \"{{\" for a brace)");
				break;
			}
			value = rvn_xc_evaluate(ctx, scope, where, cursor + 1, (size_t)(close - cursor - 1));
			length = strlen(value.text);
			RVN_ARRAY_RESERVE(out, cap, count + (uint32_t)length + 1);
			memcpy(out + count, value.text, length);
			count += (uint32_t)length;
			cursor = close + 1;
		} else {
			RVN_ARRAY_RESERVE(out, cap, count + 2);
			out[count++] = *cursor++;
		}
	}
	result = rvn_arena_strndup(arena, out ? out : "", count);
	rvn_free(out);
	return result;
}

/* ---------------------------------------------------------------------------
 *  Expansion
 * ------------------------------------------------------------------------- */

static void rvn_xc_expand_children(rvn_xc_t* ctx, const rvn_xml_node_t* node, const rvn_xc_scope_t* scope, rvn_xml_node_t* holder) {
	const rvn_xml_node_t* child;
	for (child = node->first_child; child && !ctx->failed; child = child->next) rvn_xc_expand(ctx, child, scope, holder);
}

/* A new set of variables inside `parent`, with the same <Children/>. */
static void rvn_xc_scope_inside(rvn_xc_scope_t* scope, const rvn_xc_scope_t* parent) {
	memset(scope, 0, sizeof(*scope));
	scope->parent = parent;
	if (parent) {
		scope->use = parent->use;
		scope->use_scope = parent->use_scope;
	}
}

static const char* rvn_xc_attribute(rvn_xc_t* ctx, const rvn_xml_node_t* node, const rvn_xc_scope_t* scope, const char* name) {
	const char* value = rvn_xml_attribute(node, name);
	return value ? rvn_xc_text(ctx, scope, node, value) : NULL;
}

/* <Repeat Count="3" Var="i">: its content, Count times, with {i} going from 0. */
static void rvn_xc_repeat(rvn_xc_t* ctx, const rvn_xml_node_t* node, const rvn_xc_scope_t* scope, rvn_xml_node_t* holder) {
	const char* count_text = rvn_xc_attribute(ctx, node, scope, "Count");
	const char* var = rvn_xml_attribute(node, "Var");
	int count = count_text ? (int)strtod(count_text, NULL) : 0, i;
	if (!count_text) rvn_xc_report(node, R_SEVERITY_WARNING, "<Repeat> needs Count=\"...\"");
	if (count > 10000) {
		rvn_xc_report(node, R_SEVERITY_WARNING, "<Repeat Count=\"%d\">: limited to 10000", count);
		count = 10000;
	}
	for (i = 0; i < count && !ctx->failed; i++) {
		rvn_xc_scope_t frame;
		rvn_xc_scope_inside(&frame, scope);
		rvn_xc_add_var(ctx, &frame, var && var[0] ? var : "Index", rvn_xc_number_text(ctx, (double)i));
		rvn_xc_expand_children(ctx, node, &frame, holder);
	}
}

/* <ForEach Items="a|b|c" Var="name">: its content once per item, with {name} and {Index}. */
static void rvn_xc_for_each(rvn_xc_t* ctx, const rvn_xml_node_t* node, const rvn_xc_scope_t* scope, rvn_xml_node_t* holder) {
	const char* items = rvn_xc_attribute(ctx, node, scope, "Items");
	const char* var = rvn_xml_attribute(node, "Var");
	const char* index_var = rvn_xml_attribute(node, "Index");
	const char* separator = rvn_xml_attribute(node, "Separator");
	char split = separator && separator[0] ? separator[0] : '|';
	int index = 0;
	if (!items) {
		rvn_xc_report(node, R_SEVERITY_WARNING, "<ForEach> needs Items=\"a|b|c\"");
		return;
	}
	while (*items && !ctx->failed) {
		const char* end = items;
		rvn_xc_scope_t frame;
		while (*end && *end != split) end++;
		rvn_xc_scope_inside(&frame, scope);
		rvn_xc_add_var(ctx, &frame, var && var[0] ? var : "Item", rvn_arena_strndup(&ctx->document->arena, items, (size_t)(end - items)));
		rvn_xc_add_var(ctx, &frame, index_var && index_var[0] ? index_var : "Index", rvn_xc_number_text(ctx, (double)index++));
		rvn_xc_expand_children(ctx, node, &frame, holder);
		items = *end ? end + 1 : end;
	}
}

/* <Children/>: the children given where the widget is used; its own content when there are none. */
static void rvn_xc_children(rvn_xc_t* ctx, const rvn_xml_node_t* node, const rvn_xc_scope_t* scope, rvn_xml_node_t* holder) {
	const char* name = rvn_xc_attribute(ctx, node, scope, "Name");
	const rvn_xml_node_t* child;
	rvn_xml_node_t* before = holder->last_child;
	rvn_xml_node_t* added;
	int found = R_FALSE;

	if (!scope || !scope->use) {
		rvn_xc_report(node, R_SEVERITY_WARNING, "<Children/> only means something inside a <Widget>");
		return;
	}
	for (child = scope->use->first_child; child && !ctx->failed; child = child->next) {
		const char* into = rvn_xml_attribute(child, "Into");
		int match = (name && name[0]) ? (into && rvn_stricmp(into, name) == 0) : (!into || !into[0]);
		if (!match) continue;
		found = R_TRUE;
		/* (they belong to the place of the use: its variables, and its own <Children/>) */
		rvn_xc_expand(ctx, child, scope->use_scope, holder);
	}
	if (!found) rvn_xc_expand_children(ctx, node, scope, holder);
	/* "Into" was for us: the widgets do not know it */
	for (added = before ? before->next : holder->first_child; added; added = added->next) rvn_xc_remove_attribute(added, "Into");
}

/* Marks the elements that come from nowhere as made from `source`. */
static void rvn_xc_set_source(rvn_xml_node_t* node, Rid source) {
	rvn_xml_node_t* child;
	if (!node->source) node->source = source;
	for (child = node->first_child; child; child = child->next) rvn_xc_set_source(child, source);
}

/* A use of a declared widget: its body, with the parameters of this use. */
static void rvn_xc_instantiate(rvn_xc_t* ctx, const rvn_component_t* component, const rvn_xml_node_t* use,
	const rvn_xc_scope_t* scope, rvn_xml_node_t* holder) {

	rvn_arena_t* arena = &ctx->document->arena;
	/* (taken now: the array of declarations may grow while the body is expanded, and `component` move) */
	const rvn_xml_node_t* definition = component->document.root;
	const char* name = component->name;
	const rvn_xml_attribute_t* attribute;
	rvn_xml_node_t* extra = rvn_xc_node(arena, use, "extra");
	rvn_xml_node_t* out = rvn_xc_node(arena, use, "out");
	rvn_xml_node_t* root;
	rvn_xc_scope_t frame;
	rvn_xc_var_t* var;

	memset(&frame, 0, sizeof(frame));
	frame.use = use;
	frame.use_scope = scope;
	/* the parameters and their default values: the attributes of the declaration */
	for (attribute = definition->attributes; attribute; attribute = attribute->next) {
		if (rvn_stricmp(attribute->name, "Name") == 0) continue;
		rvn_xc_add_var(ctx, &frame, rvn_arena_strndup(arena, attribute->name, strlen(attribute->name)),
			rvn_arena_strndup(arena, attribute->value, strlen(attribute->value)));
	}
	/* the attributes of the use: values of parameters, or attributes for the widget it becomes */
	for (attribute = use->attributes; attribute; attribute = attribute->next) {
		const char* value;
		/* (If="..." decided whether the tag is used at all: it is not for the widget) */
		if (rvn_stricmp(attribute->name, "If") == 0) continue;
		value = rvn_xc_text(ctx, scope, use, attribute->value);
		var = rvn_xc_find_var(&frame, attribute->name, strlen(attribute->name));
		if (var) var->value = value;
		else rvn_xc_set_attribute(arena, extra, attribute->name, value);
	}
	/* <Card>some text</Card>: the text is the Text parameter, or the Text of the widget */
	if (use->text[0] && !rvn_xml_attribute(use, "Text")) {
		const char* text = rvn_xc_text(ctx, scope, use, use->text);
		var = rvn_xc_find_var(&frame, "Text", 4);
		if (var) var->value = text;
		else rvn_xc_set_attribute(arena, extra, "Text", text);
	}
	rvn_xc_add_var(ctx, &frame, "Instance", rvn_xc_number_text(ctx, (double)++g_components.instances));

	rvn_xc_expand_children(ctx, definition, &frame, out);
	if (ctx->failed) return;
	root = out->first_child;
	if (!root || root->next) {
		rvn_xc_report(use, R_SEVERITY_ERROR, "<%.60s>: a <Widget> stands for exactly one element, this one gives %s", name,
			root ? "several (wrap them in a panel)" : "none");
		ctx->failed = R_TRUE;
		return;
	}
	for (attribute = extra->attributes; attribute; attribute = attribute->next) {
		rvn_xc_set_attribute(arena, root, attribute->name, attribute->value);
	}
	root->line = use->line;
	root->column = use->column;
	/* everything the declaration gave comes from this use; the children it was given keep where they come from */
	rvn_xc_set_source(root, use->source);
	rvn_xc_append(holder, root);
}

static void rvn_xc_include(rvn_xc_t* ctx, const rvn_xml_node_t* node, const rvn_xc_scope_t* scope, rvn_xml_node_t* holder) {
	const char* source = rvn_xc_attribute(ctx, node, scope, "Source");
	rvn_xml_document_t included;
	char* path;
	char* text;
	size_t size = 0;

	if (!source || !source[0]) {
		rvn_xc_report(node, R_SEVERITY_ERROR, "<Include> needs Source=\"file.xml\"");
		return;
	}
	path = rvn_xml_resolve_path(source);
	rvn_live_dependency(path);
	text = rvn_read_file(path, &size);
	if (!text) {
		rvn_xc_report(node, R_SEVERITY_ERROR, "<Include>: cannot read '%.160s'", path);
		rvn_free(path);
		return;
	}
	if (!rvn_xml_parse(&included, text, size)) {
		rvn_xc_report(node, R_SEVERITY_ERROR, "<Include Source=\"%.80s\">: line %d, column %d: %s", source,
			included.error_line, included.error_column, included.error);
	} else {
		/* (what the file declares is declared; what it contains comes here) */
		rvn_xc_expand(ctx, included.root, NULL, holder);
	}
	rvn_xml_free(&included);
	rvn_free(text);
	rvn_free(path);
}

/* Rewrites one element of the source into zero, one or several plain elements added to `holder`. */
static void rvn_xc_expand(rvn_xc_t* ctx, const rvn_xml_node_t* node, const rvn_xc_scope_t* scope, rvn_xml_node_t* holder) {
	rvn_arena_t* arena = &ctx->document->arena;
	const rvn_xml_attribute_t* attribute;
	const rvn_component_t* component;
	const char* condition;
	rvn_xml_node_t* copy;

	if (ctx->failed || node->name[0] == '#') return;
	if (++ctx->depth > RVN_XC_MAX_DEPTH) {
		rvn_xc_report(node, R_SEVERITY_ERROR, "<%.60s>: widgets are nested too deeply (does a <Widget> contain itself?)", node->name);
		ctx->failed = R_TRUE;
		ctx->depth--;
		return;
	}

	/* If="..." on any element */
	condition = rvn_xml_attribute(node, "If");
	if (condition && rvn_stricmp(node->name, "If") != 0) {
		const char* value = rvn_xc_text(ctx, scope, node, condition);
		if (!rvn_xv_truth(rvn_xv_text(value))) {
			ctx->depth--;
			return;
		}
	}

	if (rvn_stricmp(node->name, "Widget") == 0) {
		rvn_xc_register(ctx, node);
	} else if (rvn_stricmp(node->name, "Widgets") == 0) {
		/* a file of declarations: only a container */
		rvn_xc_expand_children(ctx, node, scope, holder);
	} else if (rvn_stricmp(node->name, "Include") == 0) {
		rvn_xc_include(ctx, node, scope, holder);
	} else if (rvn_stricmp(node->name, "Strings") == 0) {
		/* <Strings Source="strings.csv"/>: a table of translations (raven_locale.h) */
		const char* source = rvn_xc_attribute(ctx, node, scope, "Source");
		if (!source || !source[0]) {
			rvn_xc_report(node, R_SEVERITY_ERROR, "<Strings> needs Source=\"strings.csv\"");
		} else {
			char* path = rvn_xml_resolve_path(source);
			if (Rlocale_load_csv(path) < 0) rvn_xc_report(node, R_SEVERITY_ERROR, "<Strings>: cannot load '%.160s'", path);
			rvn_free(path);
		}
	} else if (rvn_stricmp(node->name, "Children") == 0) {
		rvn_xc_children(ctx, node, scope, holder);
	} else if (rvn_stricmp(node->name, "Repeat") == 0) {
		rvn_xc_repeat(ctx, node, scope, holder);
	} else if (rvn_stricmp(node->name, "ForEach") == 0) {
		rvn_xc_for_each(ctx, node, scope, holder);
	} else if (rvn_stricmp(node->name, "If") == 0) {
		const char* test = rvn_xc_attribute(ctx, node, scope, "Test");
		if (!test) rvn_xc_report(node, R_SEVERITY_WARNING, "<If> needs Test=\"...\"");
		else if (rvn_xv_truth(rvn_xv_text(test))) rvn_xc_expand_children(ctx, node, scope, holder);
	} else if ((component = rvn_component_find(node->name)) != NULL) {
		rvn_xc_instantiate(ctx, component, node, scope, holder);
	} else {
		/* an ordinary element: itself, with what is inside rewritten */
		copy = rvn_xc_node(arena, node, node->name);
		copy->text = rvn_xc_text(ctx, scope, node, node->text);
		for (attribute = node->attributes; attribute; attribute = attribute->next) {
			if (rvn_stricmp(attribute->name, "If") == 0) continue;
			rvn_xc_set_attribute(arena, copy, attribute->name, rvn_xc_text(ctx, scope, node, attribute->value));
		}
		rvn_xc_append(holder, copy);
		rvn_xc_expand_children(ctx, node, scope, copy);
	}
	ctx->depth--;
}

static int rvn_xml_expand_document(rvn_xml_document_t* document, int* registered);

int rvn_xml_expand(rvn_xml_document_t* document, int* registered) {
	int ok;
	g_components.expanding++;
	ok = rvn_xml_expand_document(document, registered);
	if (--g_components.expanding == 0) rvn_components_free_retired();
	return ok;
}

static int rvn_xml_expand_document(rvn_xml_document_t* document, int* registered) {
	rvn_xc_t ctx;
	rvn_xml_node_t* holder;

	if (registered) *registered = 0;
	if (!document->root) return R_TRUE;
	memset(&ctx, 0, sizeof(ctx));
	ctx.document = document;
	holder = rvn_xc_node(&document->arena, document->root, "document");
	rvn_xc_expand(&ctx, document->root, NULL, holder);
	if (registered) *registered = ctx.registered;
	if (ctx.failed) {
		document->root = NULL;
		return R_FALSE;
	}
	if (holder->first_child && holder->first_child->next) {
		rvn_xc_report(holder->first_child->next, R_SEVERITY_ERROR, "a document has a single root element besides its <Widget> declarations");
		document->root = NULL;
		return R_FALSE;
	}
	document->root = holder->first_child;
	if (document->root) document->root->parent = NULL;
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

int Rxml_register_widgets(const char* xml, size_t size) {
	rvn_xml_document_t document;
	int registered = 0;
	RVN_API_GUARD(0);
	if (!xml) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rxml_register_widgets: NULL text");
		return 0;
	}
	if (!rvn_xml_parse(&document, xml, size)) {
		rvn_error(R_ERROR_XML, R_SEVERITY_ERROR, "Rxml_register_widgets: line %d, column %d: %s",
			document.error_line, document.error_column, document.error);
		rvn_xml_free(&document);
		return 0;
	}
	rvn_xml_expand(&document, &registered);
	rvn_xml_free(&document);
	return registered;
}

int rvn_xml_component_parameter(const char* name, int index, const char** parameter, const char** default_value) {
	const rvn_component_t* component = name ? rvn_component_find(name) : NULL;
	const rvn_xml_attribute_t* attribute;
	if (!component || index < 0) return R_FALSE;
	for (attribute = component->document.root->attributes; attribute; attribute = attribute->next) {
		if (rvn_stricmp(attribute->name, "Name") == 0) continue;
		if (index-- == 0) {
			if (parameter) *parameter = attribute->name;
			if (default_value) *default_value = attribute->value;
			return R_TRUE;
		}
	}
	return R_FALSE;
}

void Rxml_unregister_widget(const char* name) {
	rvn_component_t* component;
	RVN_API_GUARD_VOID();
	component = name ? rvn_component_find(name) : NULL;
	if (!component) return;
	rvn_component_retire(component);
	*component = g_components.items[--g_components.count];
}

int Rxml_is_widget_registered(const char* name) {
	return g_rvn.initialized && name && rvn_component_find(name) != NULL;
}

int Rxml_get_registered_widget_count(void) {
	return g_rvn.initialized ? (int)g_components.count : 0;
}

const char* Rxml_get_registered_widget_name(int index) {
	if (!g_rvn.initialized || index < 0 || index >= (int)g_components.count) return NULL;
	return g_components.items[index].name;
}
