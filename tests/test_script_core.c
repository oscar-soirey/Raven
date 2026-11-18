/**
 * Raven tests - the core of Raven Script (src/script/rs_*.c), without the
 * library: lexer, parser, type checker, bytecode, virtual machine, limits,
 * diagnostics, formatter, analysis for the editor.
 *
 * Every example of the specification is checked here, with the code and
 * the text of its diagnostic.
 */

#include "../src/script/rs_internal.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(condition) do { \
	g_checks++; \
	if (!(condition)) { \
		g_failures++; \
		printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
	} \
} while (0)

#define CHECK_STR(actual, expected) do { \
	const char* test_a_ = (actual); \
	const char* test_e_ = (expected); \
	g_checks++; \
	if (!test_a_ || strcmp(test_a_, test_e_) != 0) { \
		g_failures++; \
		printf("  FAIL %s:%d: %s = \"%s\", expected \"%s\"\n", __FILE__, __LINE__, #actual, test_a_ ? test_a_ : "(null)", test_e_); \
	} \
} while (0)

#define CHECK_CONTAINS(haystack, needle) do { \
	const char* test_h_ = (haystack); \
	g_checks++; \
	if (!test_h_ || !strstr(test_h_, (needle))) { \
		g_failures++; \
		printf("  FAIL %s:%d: \"%s\" does not contain \"%s\"\n", __FILE__, __LINE__, test_h_ ? test_h_ : "(null)", (needle)); \
	} \
} while (0)

/* ---------------------------------------------------------------------------
 *  A host that records what happens
 * ------------------------------------------------------------------------- */

static rs_buf g_diagnostics;   /* every diagnostic, formatted */
static rs_buf g_codes;         /* "RS102 RS105 " */
static int g_errors;
static int g_warnings;
static rs_buf g_log;           /* what app.log wrote */
static int g_installed;
static int g_removed;
static int g_quiet;

static void host_diagnostic(void* user_data, const rs_diag* diag) {
	(void)user_data;
	rs_diag_format(diag, &g_diagnostics);
	rs_buf_putc(&g_diagnostics, '\n');
	rs_buf_printf(&g_codes, "%s ", diag->code);
	if (diag->severity == RS_SEVERITY_ERROR) g_errors++;
	else if (diag->severity == RS_SEVERITY_WARNING) g_warnings++;
	if (!g_quiet && getenv("RS_TEST_VERBOSE")) printf("%s\n", g_diagnostics.data);
}

static rs_external_kind host_lookup(void* user_data, const char* name, rs_sig* sig) {
	(void)user_data;
	if (strcmp(name, "test.add") == 0) {
		rs_sig_parse("(a: number, b: number): number", sig, NULL, 0, NULL, 0);
		return RS_EXTERNAL_TYPED;
	}
	if (strcmp(name, "test.fail") == 0 || strcmp(name, "test.wrong") == 0) {
		rs_sig_parse(strcmp(name, "test.fail") == 0 ? "()" : "(): number", sig, NULL, 0, NULL, 0);
		return RS_EXTERNAL_TYPED;
	}
	if (strcmp(name, "plain.action") == 0) return RS_EXTERNAL_ACTION;
	if (strcmp(name, "test.reenter") == 0) {
		rs_sig_parse("()", sig, NULL, 0, NULL, 0);
		return RS_EXTERNAL_TYPED;
	}
	return RS_EXTERNAL_NONE;
}

static int g_plain_fired = 0;
static rs_env* g_env = NULL;

static rs_call_status host_call(void* user_data, const char* name, const rs_value* args, int count, rs_value* result, rs_buf* error) {
	(void)user_data;
	if (strcmp(name, "app.log") == 0) {
		rs_buf_puts(&g_log, rs_value_text(args[0]));
		rs_buf_putc(&g_log, '\n');
		return RS_CALL_OK;
	}
	if (strcmp(name, "test.add") == 0 && count == 2) {
		*result = rs_value_number(args[0].as.number + args[1].as.number);
		return RS_CALL_OK;
	}
	if (strcmp(name, "test.fail") == 0) {
		rs_buf_puts(error, "the disk is full");
		return RS_CALL_FAILED;
	}
	if (strcmp(name, "test.wrong") == 0) {
		*result = rs_value_cstring("not a number");
		return RS_CALL_OK;
	}
	if (strcmp(name, "plain.action") == 0) {
		g_plain_fired++;
		return RS_CALL_OK;
	}
	if (strcmp(name, "test.reenter") == 0) {
		/* a native that runs a script (an event fired from inside a script) */
		rs_value value;
		rs_env_call(g_env, "app.inner", NULL, 0, RS_CALL_STRICT, NULL, &value);
		rs_value_release(value);
		return RS_CALL_OK;
	}
	return RS_CALL_NOT_FOUND;
}

static void host_installed(void* user_data, const char* name) {
	(void)user_data;
	(void)name;
	g_installed++;
}

static void host_removed(void* user_data, const char* name) {
	(void)user_data;
	(void)name;
	g_removed++;
}

static int host_element(void* user_data, const char* name) {
	(void)user_data;
	return strcmp(name, "counter") == 0 || strcmp(name, "title") == 0;
}

static void reset_records(void) {
	rs_buf_free(&g_diagnostics);
	rs_buf_free(&g_codes);
	rs_buf_free(&g_log);
	g_errors = 0;
	g_warnings = 0;
}

static rs_env* new_env(void) {
	rs_host host;
	memset(&host, 0, sizeof(host));
	host.diagnostic = host_diagnostic;
	host.lookup = host_lookup;
	host.call = host_call;
	host.function_installed = host_installed;
	host.function_removed = host_removed;
	host.element_exists = host_element;
	g_env = rs_env_create(&host);
	reset_records();
	return g_env;
}

/** Compiles and installs a script; returns the number of errors. */
static int load(rs_env* env, const char* file, const char* source) {
	rs_batch* batch = rs_batch_create(env, 0);
	int errors;
	rs_batch_add(batch, file, source, strlen(source), 1, 1);
	errors = rs_batch_compile(batch);
	if (errors == 0 && rs_batch_install(batch)) rs_batch_initialize(batch);
	rs_batch_destroy(batch);
	return errors;
}

/** The codes of the diagnostics of a script loaded in a new environment. */
static const char* codes_of(const char* source) {
	static char codes[512];
	rs_env* env = new_env();
	g_quiet = 1;
	load(env, "main.rvs", source);
	g_quiet = 0;
	snprintf(codes, sizeof(codes), "%s", g_codes.data ? g_codes.data : "");
	rs_env_destroy(env);
	return codes;
}

static double call_number(rs_env* env, const char* name, const rs_value* args, int count) {
	rs_value result;
	double number = -12345.0;
	if (rs_env_call(env, name, args, count, RS_CALL_STRICT, NULL, &result) == RS_CALL_OK && result.type == RS_V_NUMBER) number = result.as.number;
	rs_value_release(result);
	return number;
}

static char g_text[4096];
static const char* call_string(rs_env* env, const char* name) {
	rs_value result;
	g_text[0] = '\0';
	if (rs_env_call(env, name, NULL, 0, RS_CALL_STRICT, NULL, &result) == RS_CALL_OK) {
		if (result.type == RS_V_STRING) snprintf(g_text, sizeof(g_text), "%s", result.as.string->text);
		else snprintf(g_text, sizeof(g_text), "<type %d>", result.type);
	} else {
		snprintf(g_text, sizeof(g_text), "<failed>");
	}
	rs_value_release(result);
	return g_text;
}

/* ---------------------------------------------------------------------------
 *  Tests
 * ------------------------------------------------------------------------- */

static void test_syntax(void) {
	printf("syntax\n");
	CHECK_STR(codes_of("fn app.hello() {\n    app.log(\"Hello from Raven!\");\n}\n"), "");
	/* 3.4: function is not a keyword, parentheses are mandatory */
	CHECK_STR(codes_of("function myapp.quit() {\n    app.quit();\n}\n"), "RS001 ");
	CHECK_CONTAINS(g_diagnostics.data, "'function' is not a recognized keyword. Use 'fn'.");
	CHECK_STR(codes_of("fn myapp.quit {\n    app.quit();\n}\n"), "RS002 ");
	CHECK_CONTAINS(g_diagnostics.data, "Parentheses are mandatory, even when the function has no parameters.");
	/* 3.2: a missing return type is void */
	CHECK_STR(codes_of("fn myapp.quit(): void {\n    app.quit();\n}\nfn myapp.quit2() {\n    app.quit();\n}\n"), "");
	CHECK_STR(codes_of("fn a.b() {\n    let x = 1\n    let y = 2;\n}\n"), "RS002 ");
	CHECK_CONTAINS(g_diagnostics.data, "Expected ';' after the declaration");
	CHECK_STR(codes_of("fn a.b() {\n    let s = \"abc;\n}\n"), "RS003 ");
	CHECK_STR(codes_of("fn a.b() {\n    let i = 0;\n    i++;\n}\n"), "RS001 ");
	CHECK_CONTAINS(g_diagnostics.data, "write 'i += 1'");
	CHECK_STR(codes_of("fn a.b() {\n    if (1 === 1) {\n    }\n}\n"), "RS001 ");
	CHECK_STR(codes_of("app.log(\"top\");\n"), "RS001 ");
	CHECK_CONTAINS(g_diagnostics.data, "Only declarations");
	CHECK_STR(codes_of("fn a.b() {\n    if (true) app.quit();\n}\n"), "RS002 ");
	CHECK_STR(codes_of("fn a.b() {\n    let x = 1 & 2;\n}\n"), "RS003 ");
	CHECK_STR(codes_of("fn a.b() {\n    /* never closed\n}\n"), "RS003 ");
	CHECK_STR(codes_of("fn a.b() {\n    let x = \"\\q\";\n}\n"), "RS003 ");
	CHECK_STR(codes_of("fn a.b() {\n    var x = 1;\n}\n"), "RS001 ");
	/* recovery: two independent mistakes, two diagnostics */
	CHECK_STR(codes_of("fn a.b() {\n    let x = ;\n}\nfn a.c() {\n    let y = 1\n}\n"), "RS002 RS002 ");
	/* a stray '}' and a stray '{' */
	CHECK_STR(codes_of("}\nfn a.b() {\n}\n"), "RS001 ");
	CHECK_STR(codes_of("fn a.b() {\n    let x = ((((1));\n}\n"), "RS002 ");
	/* comments, a byte order mark, an empty script */
	CHECK_STR(codes_of("\xEF\xBB\xBF// a comment\n/* another */\nfn a.b() { /* inline */ }\n"), "");
	CHECK_STR(codes_of(""), "");
	CHECK_STR(codes_of("   \n\n  "), "");
	/* nesting is bounded */
	{
		char deep[5000];
		int i, n = 0;
		n += sprintf(deep + n, "fn a.b() { let x = ");
		for (i = 0; i < 600; i++) deep[n++] = '(';
		deep[n++] = '1';
		for (i = 0; i < 600; i++) deep[n++] = ')';
		n += sprintf(deep + n, "; }\n");
		deep[n] = '\0';
		CHECK(strstr(codes_of(deep), "RS001") != NULL);
	}
}

static void test_types(void) {
	printf("types\n");
	/* 5.4 */
	CHECK_STR(codes_of("fn a.b() {\n    const counter: number = 10;\n    counter = 20;\n}\n"), "RS105 ");
	CHECK_CONTAINS(g_diagnostics.data, "error[RS105]: Cannot assign to constant 'counter'.");
	CHECK_CONTAINS(g_diagnostics.data, "'counter' was declared as a constant.");
	CHECK_STR(codes_of("let counter: number = \"Hello\";\n"), "RS102 ");
	CHECK_CONTAINS(g_diagnostics.data, "error[RS102]: Type mismatch.");
	CHECK_CONTAINS(g_diagnostics.data, "Expected 'number', but got 'string'.");
	CHECK_CONTAINS(g_diagnostics.data, "Help: Remove the quotes or convert the value");
	/* 6.2 */
	CHECK_STR(codes_of("let count: number = 10;\nlet result: number = count + \"hello\";\n"), "RS102 ");
	CHECK_CONTAINS(g_diagnostics.data, "error[RS102]: Invalid operation.");
	CHECK_CONTAINS(g_diagnostics.data, "Cannot add values of type 'number' and 'string'.");
	CHECK_CONTAINS(g_diagnostics.data, "Hint: Convert the string to a number first");
	/* 8 */
	CHECK_STR(codes_of("fn a.b() {\n    if (10) {\n        app.log(\"Hello\");\n    }\n}\n"), "RS102 ");
	CHECK_CONTAINS(g_diagnostics.data, "error[RS102]: Invalid condition.");
	CHECK_CONTAINS(g_diagnostics.data, "Expected 'bool', but got 'number'.");
	CHECK_CONTAINS(g_diagnostics.data, "Hint: Use an explicit comparison.");
	/* 10 */
	CHECK_STR(codes_of("fn a.b() {\n    let numbers: number[] = [1, 2, 3];\n    numbers.push(\"Hello\");\n}\n"), "RS102 ");
	CHECK_CONTAINS(g_diagnostics.data, "error[RS102]: Array type mismatch.");
	CHECK_CONTAINS(g_diagnostics.data, "Array 'numbers' only accepts values of type 'number'.");
	/* 11 */
	CHECK_STR(codes_of("fn math.multiply(a: number, b: number): number {\n    return a * b;\n}\nfn a.b() {\n    math.multiply(5);\n}\n"), "RS106 ");
	CHECK_CONTAINS(g_diagnostics.data, "Function 'math.multiply' expects 2 arguments,\n  but 1 was provided.");
	CHECK_STR(codes_of("fn math.multiply(a: number, b: number): number {\n    return a * b;\n}\nfn a.b() {\n    math.multiply(5, \"Hello\");\n}\n"), "RS102 ");
	CHECK_CONTAINS(g_diagnostics.data, "error[RS102]: Argument type mismatch.");
	CHECK_CONTAINS(g_diagnostics.data, "Argument 2 of 'math.multiply' expects 'number',\n  but got 'string'.");
	/* inference */
	CHECK_STR(codes_of("let counter = 10;\nlet name = \"Raven\";\nlet enabled = true;\nfn a.b() {\n    counter = 11;\n    name = \"x\";\n    enabled = false;\n}\n"), "");
	CHECK_STR(codes_of("let counter = 10;\nfn a.b() {\n    counter = \"x\";\n}\n"), "RS102 ");
	CHECK_STR(codes_of("let items = [];\n"), "RS102 ");
	CHECK_STR(codes_of("let items: number[] = [];\nlet grid: number[][] = [[1, 2], [3]];\nlet bad: number[] = [1, \"a\"];\n"), "RS102 ");
	/* names */
	CHECK_STR(codes_of("fn a.b() {\n    countr = 1;\n}\nlet counter = 0;\n"), "RS101 ");
	CHECK_CONTAINS(g_diagnostics.data, "did you mean 'counter'?");
	CHECK_STR(codes_of("fn a.b() {\n    app.lgo(\"x\");\n}\n"), "RS101 ");
	CHECK_CONTAINS(g_diagnostics.data, "did you mean 'app.log'?");
	CHECK_STR(codes_of("fn a.b() {\n    nothing.here();\n}\n"), "RS101 ");
	CHECK_CONTAINS(g_diagnostics.data, "declare fn nothing.here(...)");
	CHECK_STR(codes_of("fn a.b() {\n    let x = 1;\n    let x = 2;\n}\n"), "RS107 ");
	CHECK_STR(codes_of("fn a.b() {\n    let x = 1;\n    if (true) {\n        let x = \"shadow\";\n    }\n}\n"), "");
	CHECK_STR(codes_of("fn a.b() {\n}\nfn a.b() {\n}\n"), "RS107 ");
	CHECK_STR(codes_of("fn app.log(m: string) {\n}\n"), "RS107 ");
	CHECK_STR(codes_of("fn test.add(a: number) {\n}\n"), "RS107 ");
	CHECK_STR(codes_of("let x = y;\nlet y = 1;\n"), "RS101 ");
	CHECK_STR(codes_of("let x = x;\n"), "RS101 ");
	/* functions */
	CHECK_STR(codes_of("fn a.b(): number {\n    if (true) {\n        return 1;\n    }\n}\n"), "RS108 ");
	CHECK_STR(codes_of("fn a.b(): number {\n    if (true) {\n        return 1;\n    }\n    else {\n        return 2;\n    }\n}\n"), "");
	CHECK_STR(codes_of("fn a.b(): number {\n    while (true) {\n    }\n}\n"), "");
	CHECK_STR(codes_of("fn a.b(): number {\n    while (true) {\n        break;\n    }\n}\n"), "RS108 ");
	CHECK_STR(codes_of("fn a.b() {\n    return 1;\n}\n"), "RS109 ");
	CHECK_STR(codes_of("fn a.b(): string {\n    return;\n}\n"), "RS109 ");
	CHECK_STR(codes_of("fn a.b(): string {\n    return 1;\n}\n"), "RS102 ");
	CHECK_STR(codes_of("fn a.b() {\n    break;\n}\n"), "RS110 ");
	CHECK_STR(codes_of("fn a.b() {\n    let x = app.log(\"a\");\n}\n"), "RS102 ");
	CHECK_STR(codes_of("fn a.b() {\n    1 + 2;\n}\n"), "RS111 ");
	CHECK_STR(codes_of("fn a.b() {\n    return;\n    app.quit();\n}\n"), "RS112 ");
	CHECK_STR(codes_of("fn a.b() {\n    let s = \"x\";\n    s();\n}\n"), "RS102 ");
	CHECK_STR(codes_of("fn a.b() {\n    let s = \"x\";\n    let n = s.length();\n}\n"), "RS102 ");
	CHECK_STR(codes_of("fn a.b() {\n    let s = 1;\n    let t = s.toString;\n}\n"), "RS102 ");
	CHECK_STR(codes_of("fn a.b() {\n    let s = 1;\n    let t = s.nothing();\n}\n"), "RS101 ");
	CHECK_STR(codes_of("fn a.b() {\n    let f = app.quit;\n}\n"), "RS102 ");
	CHECK_STR(codes_of("fn a.b() {\n    let s = \"abc\";\n    let c = s[0];\n}\n"), "RS102 ");
	CHECK_STR(codes_of("fn a.b() {\n    let ok = 1 < \"2\";\n}\n"), "RS102 ");
	CHECK_STR(codes_of("fn a.b() {\n    let ok = true && 1;\n}\n"), "RS102 ");
	CHECK_STR(codes_of("fn a.b() {\n    let ok = 1 == \"1\";\n}\n"), "RS102 ");
	CHECK_STR(codes_of("fn a.b() {\n    let s: string = \"a\";\n    let ok = s == null;\n}\n"), "");
	CHECK_STR(codes_of("fn a.b(x: void) {\n}\n"), "RS102 ");
	CHECK_STR(codes_of("fn a.b(x: boolean) {\n}\n"), "RS102 ");
	CHECK_STR(codes_of("fn a.b(x: thing) {\n}\n"), "RS102 ");
	CHECK_STR(codes_of("const c;\n"), "RS002 ");
	CHECK_STR(codes_of("let c;\n"), "RS102 ");
	CHECK_STR(codes_of("let c: number;\n"), "");
	/* natives and the application */
	CHECK_STR(codes_of("fn a.b() {\n    let n = test.add(1, 2);\n    plain.action();\n}\n"), "");
	CHECK_STR(codes_of("fn a.b() {\n    plain.action(1);\n}\n"), "RS106 ");
	CHECK_STR(codes_of("declare fn my.save(path: string): bool;\nfn a.b() {\n    let ok: bool = my.save(\"x\");\n}\n"), "");
	CHECK_STR(codes_of("declare fn test.add(a: string): number;\n"), "RS102 ");
	CHECK_STR(codes_of("declare fn test.add(a: number, b: number): number;\n"), "");
	CHECK_STR(codes_of("declare fn my.save(path: string[]): bool;\n"), "RS102 ");
	CHECK_STR(codes_of("fn a.b() {\n    let n: number = number.parse(\"12\");\n}\n"), "");
	/* RS301: a widget nobody named */
	CHECK_STR(codes_of("fn a.b() {\n    ui.setText(\"counter\", \"1\");\n    ui.setText(\"nobody\", \"1\");\n}\n"), "RS301 ");
}

static void test_events(void) {
	rs_env* env;
	rs_batch* batch;
	rs_location where;
	const char* source =
		"fn app.on_click() {\n}\n"
		"fn app.on_volume_changed(value: number) {\n}\n"
		"fn app.bad_type(value: bool) {\n    app.log(\"Changed\");\n}\n"
		"fn app.two(value: number, enabled: bool) {\n    app.log(\"Changed\");\n}\n"
		"fn app.on_combo(index: number, text: string) {\n}\n";
	const rs_base slider[1] = { RS_T_NUMBER };
	rs_sig sig;
	printf("events\n");
	/* 12.4: the table of the specification */
	memset(&sig, 0, sizeof(sig));
	CHECK(rs_handler_check(&sig, NULL, 0, NULL) == RS_HANDLER_OK);
	CHECK(rs_handler_check(&sig, slider, 1, NULL) == RS_HANDLER_OK);
	sig.count = 1;
	sig.params[0] = rs_type_make(RS_T_NUMBER, 0);
	CHECK(rs_handler_check(&sig, slider, 1, NULL) == RS_HANDLER_OK);
	CHECK(rs_handler_check(&sig, NULL, 0, NULL) == RS_HANDLER_TOO_MANY_PARAMETERS);
	sig.params[0] = rs_type_make(RS_T_BOOL, 0);
	CHECK(rs_handler_check(&sig, slider, 1, NULL) == RS_HANDLER_WRONG_TYPE);
	sig.count = 2;
	sig.params[0] = rs_type_make(RS_T_NUMBER, 0);
	sig.params[1] = rs_type_make(RS_T_NUMBER, 0);
	CHECK(rs_handler_check(&sig, slider, 1, NULL) == RS_HANDLER_TOO_MANY_PARAMETERS);

	CHECK(rs_event_find("Slider") != NULL);
	CHECK(rs_event_find("slider") != NULL);
	CHECK(rs_event_find("CheckBox") == rs_event_find("Checkbox"));
	CHECK(rs_event_find("TextBox") == rs_event_find("TextInput"));
	CHECK(rs_event_find("Button") == NULL);
	CHECK(rs_is_action_attribute("OnClicked") && rs_is_action_attribute("onchanged") && !rs_is_action_attribute("onClick"));

	env = new_env();
	batch = rs_batch_create(env, 0);
	rs_batch_add(batch, "scripts/main.rvs", source, strlen(source), 1, 1);
	CHECK(rs_batch_compile(batch) == 0);
	memset(&where, 0, sizeof(where));
	where.file = "interface.xml";
	where.line = 12;
	where.column = 1;
	where.end_line = 12;
	where.end_column = 9;
	where.line_text = "<Slider OnChanged=\"app.bad_type\"/>";
	where.unit = -1;
	CHECK(rs_batch_check_handler(batch, "Button", "OnClicked", "app.on_click", &where));
	CHECK(rs_batch_check_handler(batch, "Slider", "OnChanged", "app.on_click", &where));
	CHECK(rs_batch_check_handler(batch, "Slider", "OnChanged", "app.on_volume_changed", &where));
	CHECK(rs_batch_check_handler(batch, "ComboBox", "OnChanged", "app.on_combo", &where));
	CHECK(rs_batch_check_handler(batch, "Slider", "OnChanged", "not.a.script.function", &where));
	CHECK(g_errors == 0);
	/* 12.5 */
	CHECK(!rs_batch_check_handler(batch, "Slider", "OnChanged", "app.bad_type", &where));
	CHECK_CONTAINS(g_diagnostics.data, "error[RS103]: Invalid event handler signature.");
	CHECK_CONTAINS(g_diagnostics.data, "--> interface.xml:12:1");
	CHECK_CONTAINS(g_diagnostics.data, "Slider.OnChanged expects a function with\n  1 parameter of type 'number'.");
	CHECK_CONTAINS(g_diagnostics.data, "But 'app.bad_type' has:\n  1 parameter of type 'bool'.");
	CHECK_CONTAINS(g_diagnostics.data, "--> scripts/main.rvs:5:24");
	CHECK_CONTAINS(g_diagnostics.data, "Change the parameter type from 'bool' to 'number'.");
	/* 12.6 */
	reset_records();
	CHECK(!rs_batch_check_handler(batch, "Slider", "OnChanged", "app.two", &where));
	CHECK_CONTAINS(g_diagnostics.data, "Slider.OnChanged provides 1 argument,\n  but 'app.two' requires 2.");
	CHECK_CONTAINS(g_diagnostics.data, "Remove the additional required parameter.");
	reset_records();
	CHECK(!rs_batch_check_handler(batch, "Button", "OnClicked", "app.on_volume_changed", &where));
	CHECK_CONTAINS(g_diagnostics.data, "Button.OnClicked provides 0 arguments");
	CHECK(rs_batch_install(batch));
	rs_batch_destroy(batch);

	/* dispatching: extra arguments dropped, missing ones refused, types checked */
	{
		rs_value args[3];
		args[0] = rs_value_number(42.0);
		args[1] = rs_value_cstring("item");
		args[2] = rs_value_bool(1);
		reset_records();
		CHECK(rs_env_call(env, "app.on_click", args, 3, RS_CALL_EVENT, "Button.OnClicked", NULL) == RS_CALL_OK);
		CHECK(rs_env_call(env, "app.on_volume_changed", args, 1, RS_CALL_EVENT, "Slider.OnChanged", NULL) == RS_CALL_OK);
		CHECK(rs_env_call(env, "app.on_combo", args, 2, RS_CALL_EVENT, "ComboBox.OnChanged", NULL) == RS_CALL_OK);
		CHECK(g_errors == 0);
		CHECK(rs_env_call(env, "app.on_volume_changed", args, 0, RS_CALL_EVENT, "Button.OnClicked", NULL) == RS_CALL_FAILED);
		CHECK_CONTAINS(g_codes.data, "RS103");
		CHECK(rs_env_call(env, "app.bad_type", args, 1, RS_CALL_EVENT, "Slider.OnChanged", NULL) == RS_CALL_FAILED);
		CHECK(rs_env_call(env, "app.on_click", args, 1, RS_CALL_STRICT, NULL, NULL) == RS_CALL_FAILED);
		CHECK_CONTAINS(g_codes.data, "RS106");
		CHECK(rs_env_call(env, "nobody", args, 0, RS_CALL_STRICT, NULL, NULL) == RS_CALL_NOT_FOUND);
		rs_value_release(args[1]);
	}
	rs_env_destroy(env);
}

static void test_run(void) {
	rs_env* env = new_env();
	rs_value args[2];
	const char* source =
		"let counter: number = 0;\n"
		"const app_name: string = \"Raven\";\n"
		"let names: string[] = [\"a\", \"b\"];\n"
		"fn math.add(a: number, b: number): number {\n    return a + b;\n}\n"
		"fn math.fib(n: number): number {\n    if (n < 2) {\n        return n;\n    }\n    return math.fib(n - 1) + math.fib(n - 2);\n}\n"
		"fn app.increment(): number {\n    counter += 1;\n    return counter;\n}\n"
		"fn app.loops(): number {\n"
		"    let total = 0;\n"
		"    for (let i: number = 0; i < 10; i += 1) {\n"
		"        if (i == 5) {\n            break;\n        }\n"
		"        if (i == 2) {\n            continue;\n        }\n"
		"        total += i;\n"
		"    }\n"
		"    let count = 0;\n"
		"    while (count < 5) {\n        count += 1;\n    }\n"
		"    for (;;) {\n        total += 100;\n        break;\n    }\n"
		"    return total * 10 + count;\n"
		"}\n"
		"fn app.text(): string {\n"
		"    let count: number = 42;\n"
		"    let text: string = count.toString();\n"
		"    let value: number = number.parse(\"123\");\n"
		"    let s = text + \"/\" + value.toString() + \"/\" + (3.14).toString() + \"/\" + (1 / 3).toString() + \"/\" + (0.1 + 0.2).toString();\n"
		"    s += \"/\" + true.toString() + \"/\" + app_name.toUpperCase() + \"/\" + \"ÉtÉ àB\".toLowerCase() + \"/\" + (-0).toString() + \"/\" + (1e21).toString();\n"
		"    return s;\n"
		"}\n"
		"fn app.strings(): string {\n"
		"    let s = \"  héllo, world  \".trim();\n"
		"    let parts = s.split(\", \");\n"
		"    let out = parts[1] + \"|\" + s.length.toString() + \"|\" + s.indexOf(\"world\").toString() + \"|\" + s.substring(1, 5) + \"|\" + s.substring(9, 3);\n"
		"    out += \"|\" + s.contains(\"llo\").toString() + s.startsWith(\"hé\").toString() + s.endsWith(\"x\").toString();\n"
		"    out += \"|\" + \"a,b,\".split(\",\").length.toString() + \"|\" + \"abc\".split(\"\").length.toString() + \"|\" + (2.345).toFixed(2) + \"|\" + (-0.0001).toFixed(2) + \"|\" + (5).toFixed(0);\n"
		"    out += \"|\" + \"\\u00E9\\u{1F600}\\t\".length.toString();\n"
		"    return out;\n"
		"}\n"
		"fn app.arrays(): number {\n"
		"    let numbers: number[] = [1, 2, 3, 4, 5];\n"
		"    numbers.push(6);\n"
		"    let last = numbers.pop();\n"
		"    numbers[0] = 10;\n"
		"    numbers[1] += 5;\n"
		"    let grid: number[][] = [[1, 2], [3, 4]];\n"
		"    grid[1][0] = 30;\n"
		"    let alias = numbers;\n"
		"    alias.push(100);\n"
		"    names.push(\"c\");\n"
		"    return numbers.length * 1000 + last * 100 + numbers[0] + numbers[1] + grid[1][0] + names.length;\n"
		"}\n"
		"fn app.logic(): bool {\n"
		"    let a = true;\n"
		"    let b = !a || (1 < 2 && \"a\" < \"b\");\n"
		"    let c = \"x\" != \"y\" && 3 >= 3 && 2 <= 1 == false;\n"
		"    let n: string = \"q\";\n"
		"    return b && c && n != null && -(-5) == 5 && 7 % 3 == 1;\n"
		"}\n"
		"fn app.log_twice() {\n    app.log(\"Hello\");\n    app.log(app_name);\n}\n"
		"fn app.host(): number {\n    plain.action();\n    return test.add(2, 3);\n}\n";
	printf("run\n");
	CHECK(load(env, "run.rvs", source) == 0);
	CHECK(g_errors == 0);
	args[0] = rs_value_number(2.0);
	args[1] = rs_value_number(3.0);
	CHECK(call_number(env, "math.add", args, 2) == 5.0);
	args[0] = rs_value_number(20.0);
	CHECK(call_number(env, "math.fib", args, 1) == 6765.0);
	CHECK(call_number(env, "app.increment", NULL, 0) == 1.0);
	CHECK(call_number(env, "app.increment", NULL, 0) == 2.0);
	CHECK(call_number(env, "app.loops", NULL, 0) == (0 + 1 + 3 + 4 + 100) * 10 + 5);
	CHECK_STR(call_string(env, "app.text"), "42/123/3.14/0.3333333333333333/0.30000000000000004/true/RAVEN/été àb/0/1e+21");
	CHECK_STR(call_string(env, "app.strings"), "world|12|7|éllo|lo, wo|truetruefalse|3|3|2.35|0.00|5|3");
	CHECK(call_number(env, "app.arrays", NULL, 0) == 6000 + 600 + 10 + 7 + 30 + 3);
	{
		rs_value result;
		CHECK(rs_env_call(env, "app.logic", NULL, 0, RS_CALL_STRICT, NULL, &result) == RS_CALL_OK);
		CHECK(result.type == RS_V_BOOL && result.as.boolean);
	}
	CHECK(rs_env_call(env, "app.log_twice", NULL, 0, RS_CALL_STRICT, NULL, NULL) == RS_CALL_OK);
	CHECK_STR(g_log.data, "Hello\nRaven\n");
	CHECK(call_number(env, "app.host", NULL, 0) == 5.0);
	CHECK(g_plain_fired == 1);
	CHECK(g_errors == 0);
	/* nothing leaks: the memory of the values comes back */
	{
		size_t before = rs_env_memory(env);
		int i;
		for (i = 0; i < 50; i++) {
			call_string(env, "app.strings");
			call_number(env, "app.arrays", NULL, 0);
		}
		CHECK(rs_env_memory(env) <= before + 4096);
	}
	rs_env_destroy(env);
}

static void test_runtime_errors(void) {
	rs_env* env = new_env();
	const char* source =
		"let items: number[] = [1, 2, 3];\n"
		"fn app.divide(): number {\n    let zero = 0;\n    return 10 / zero;\n}\n"
		"fn app.modulo(): number {\n    let zero = 0;\n    return 10 % zero;\n}\n"
		"fn app.index(): number {\n    return items[5];\n}\n"
		"fn app.fraction(): number {\n    return items[0.5];\n}\n"
		"fn app.negative() {\n    items[-1] = 3;\n}\n"
		"fn app.pop(): number {\n    let empty: number[] = [];\n    return empty.pop();\n}\n"
		"fn app.forever() {\n    while (true) {\n    }\n}\n"
		"fn app.recurse(n: number): number {\n    return app.recurse(n + 1);\n}\n"
		"fn app.parse(): number {\n    return number.parse(\"12abc\");\n}\n"
		"fn app.fixed(): string {\n    return (1).toFixed(30);\n}\n"
		"fn app.memory(): number {\n    let s = \"0123456789abcdef\";\n    while (true) {\n        s += s;\n    }\n}\n"
		"fn app.native_fails() {\n    test.fail();\n}\n"
		"fn app.native_lies(): number {\n    return test.wrong();\n}\n"
		"declare fn missing.thing(): void;\n"
		"fn app.missing() {\n    missing.thing();\n}\n"
		"fn app.still_works(): number {\n    return items.length;\n}\n"
		"fn app.inner(): number {\n    return 1;\n}\n"
		"fn app.reenter(): number {\n    let total = 0;\n    for (let i = 0; i < 3; i += 1) {\n        test.reenter();\n        total += 1;\n    }\n    return total;\n}\n";
	printf("runtime errors\n");
	rs_env_set_limits(env, 100000, 50, 1024 * 1024);
	CHECK(load(env, "errors.rvs", source) == 0);
	reset_records();
	CHECK(call_number(env, "app.divide", NULL, 0) == -12345.0);
	CHECK_CONTAINS(g_codes.data, "RS201");
	CHECK_CONTAINS(g_diagnostics.data, "error[RS201]: Division by zero.");
	CHECK_CONTAINS(g_diagnostics.data, "--> errors.rvs:4:12");
	CHECK_CONTAINS(g_diagnostics.data, "In 'app.divide'");
	reset_records();
	call_number(env, "app.modulo", NULL, 0);
	CHECK_STR(g_codes.data, "RS201 ");
	reset_records();
	call_number(env, "app.index", NULL, 0);
	CHECK_STR(g_codes.data, "RS204 ");
	CHECK_CONTAINS(g_diagnostics.data, "The index 5 is out of bounds: the array has 3 items.");
	reset_records();
	call_number(env, "app.fraction", NULL, 0);
	CHECK_STR(g_codes.data, "RS204 ");
	reset_records();
	rs_env_call(env, "app.negative", NULL, 0, RS_CALL_STRICT, NULL, NULL);
	CHECK_STR(g_codes.data, "RS204 ");
	reset_records();
	call_number(env, "app.pop", NULL, 0);
	CHECK_STR(g_codes.data, "RS204 ");
	reset_records();
	rs_env_call(env, "app.forever", NULL, 0, RS_CALL_STRICT, NULL, NULL);
	CHECK_STR(g_codes.data, "RS203 ");
	reset_records();
	{
		rs_value zero = rs_value_number(0);
		call_number(env, "app.recurse", &zero, 1);
	}
	CHECK_STR(g_codes.data, "RS202 ");
	CHECK_CONTAINS(g_diagnostics.data, "called from 'app.recurse'");
	reset_records();
	call_number(env, "app.parse", NULL, 0);
	CHECK_STR(g_codes.data, "RS104 ");
	CHECK_CONTAINS(g_diagnostics.data, "'12abc' is not a number.");
	reset_records();
	call_string(env, "app.fixed");
	CHECK_STR(g_codes.data, "RS104 ");
	reset_records();
	call_number(env, "app.memory", NULL, 0);
	CHECK_STR(g_codes.data, "RS206 ");
	reset_records();
	rs_env_call(env, "app.native_fails", NULL, 0, RS_CALL_STRICT, NULL, NULL);
	CHECK_STR(g_codes.data, "RS205 ");
	CHECK_CONTAINS(g_diagnostics.data, "the disk is full");
	reset_records();
	call_number(env, "app.native_lies", NULL, 0);
	CHECK_STR(g_codes.data, "RS205 ");
	reset_records();
	rs_env_call(env, "app.missing", NULL, 0, RS_CALL_STRICT, NULL, NULL);
	CHECK_STR(g_codes.data, "RS101 ");
	/* after all that, the environment works and its memory is back */
	reset_records();
	CHECK(call_number(env, "app.still_works", NULL, 0) == 3.0);
	CHECK(call_number(env, "app.reenter", NULL, 0) == 3.0);
	CHECK(g_errors == 0);
	CHECK(rs_env_running(env) == 0);
	CHECK(env->stack_top == 0);
	CHECK(env->frame_count == 0);
	CHECK(env->depth == 0);
	rs_env_destroy(env);

	/* a runtime error in an initializer: reported, the rest of the batch stays */
	env = new_env();
	CHECK(load(env, "init.rvs", "let items: number[] = [1];\nlet bad: number = items[3];\nlet after: number = 7;\nfn app.get(): number {\n    return after + bad;\n}\n") == 0);
	CHECK_STR(g_codes.data, "RS204 ");
	CHECK(call_number(env, "app.get", NULL, 0) == 0.0);
	rs_env_destroy(env);
}

static void test_reload(void) {
	rs_env* env = new_env();
	printf("reload\n");
	g_installed = 0;
	g_removed = 0;
	CHECK(load(env, "a.rvs", "let counter = 5;\nfn app.value(): number {\n    return counter;\n}\nfn app.other(): number {\n    return app.value() * 2;\n}\n") == 0);
	CHECK(g_installed == 2);
	CHECK(call_number(env, "app.other", NULL, 0) == 10.0);
	/* a second document sees the first one */
	CHECK(load(env, "b.rvs", "fn app.third(): number {\n    counter += 1;\n    return app.value() + counter;\n}\n") == 0);
	CHECK(call_number(env, "app.third", NULL, 0) == 12.0);
	/* the first one again, changed: replaces its functions and globals */
	CHECK(load(env, "a.rvs", "let counter = 100;\nfn app.value(): number {\n    return counter + 1;\n}\n") == 0);
	CHECK(call_number(env, "app.value", NULL, 0) == 101.0);
	CHECK(call_number(env, "app.third", NULL, 0) == 203.0);
	/* a function replaced by one of another signature: the old callers fail cleanly */
	CHECK(load(env, "a.rvs", "fn app.value(x: string): string {\n    return x;\n}\n") == 0);
	reset_records();
	CHECK(call_number(env, "app.other", NULL, 0) == -12345.0);
	CHECK_STR(g_codes.data, "RS106 ");
	/* a batch with an error installs nothing */
	g_installed = 0;
	CHECK(load(env, "c.rvs", "fn app.ok() {\n}\nfn app.broken() {\n    let x: number = \"a\";\n}\n") == 1);
	CHECK(g_installed == 0);
	CHECK(!rs_env_find_function(env, "app.ok", NULL));
	rs_env_reset(env);
	CHECK(rs_env_function_count(env) == 0);
	CHECK(g_removed >= 3);
	rs_env_destroy(env);
}

static void test_multi_unit(void) {
	rs_env* env = new_env();
	rs_batch* batch = rs_batch_create(env, 0);
	const char* first = "fn app.start(): number {\n    return helper.value() + shared;\n}\n";
	const char* second = "let shared = 40;\nfn helper.value(): number {\n    return 2;\n}\n";
	printf("units\n");
	/* the second unit starts at line 20, column 17 of its document */
	rs_batch_add(batch, "window.xml", first, strlen(first), 3, 13);
	rs_batch_add(batch, "window.xml", second, strlen(second), 20, 17);
	CHECK(rs_batch_compile(batch) == 0);
	CHECK(rs_batch_install(batch));
	/* before its initializers, a global has the default value of its type */
	CHECK(call_number(env, "app.start", NULL, 0) == 2.0);
	CHECK(rs_batch_initialize(batch));
	CHECK(!rs_batch_initialize(batch));
	rs_batch_destroy(batch);
	CHECK(call_number(env, "app.start", NULL, 0) == 42.0);
	/* positions are those of the document */
	batch = rs_batch_create(env, 0);
	{
		const char* text = "fn x.y() {\n    let a: number = \"s\";\n}\n";
		rs_batch_add(batch, "window.xml", text, strlen(text), 30, 9);
	}
	reset_records();
	rs_batch_compile(batch);
	CHECK_CONTAINS(g_diagnostics.data, "--> window.xml:31:21");
	rs_batch_destroy(batch);
	batch = rs_batch_create(env, 0);
	rs_batch_add(batch, "window.xml", "let a: number = \"s\";", 20, 30, 17);
	reset_records();
	rs_batch_compile(batch);
	CHECK_CONTAINS(g_diagnostics.data, "--> window.xml:30:33");
	/* the excerpt of a first line is padded to its column */
	CHECK_CONTAINS(g_diagnostics.data, " 30 |                 let a: number = \"s\";\n    |                                 ^^^");
	rs_batch_destroy(batch);
	rs_env_destroy(env);
}

static void test_format_diagnostic(void) {
	rs_diag diag;
	rs_buf out = { 0 };
	printf("diagnostic format\n");
	memset(&diag, 0, sizeof(diag));
	diag.severity = RS_SEVERITY_ERROR;
	diag.code = "RS105";
	diag.title = "Cannot assign to constant 'counter'.";
	diag.message = "'counter' was declared as a constant.";
	diag.has_location = 1;
	diag.location.file = "main.rvs";
	diag.location.line = 2;
	diag.location.column = 5;
	diag.location.end_line = 2;
	diag.location.end_column = 12;
	diag.location.line_text = "    counter = 20;";
	rs_diag_format(&diag, &out);
	CHECK_STR(out.data,
		"error[RS105]: Cannot assign to constant 'counter'.\n"
		"\n"
		"  --> main.rvs:2:5\n"
		"   |\n"
		" 2 |     counter = 20;\n"
		"   |     ^^^^^^^\n"
		"\n"
		"  'counter' was declared as a constant.\n");
	rs_buf_free(&out);
}

static void test_formatter(void) {
	const char* messy =
		"// counter\n"
		"let counter:number=0;\n\n\n"
		"fn app.increment( ) {counter+=1;\n"
		"  ui.setText(\"counter\",counter.toString());  // show it\n"
		"if(counter>10&&!false){counter=-1;}else if (counter==3){app.log(\"three\");} else {app.log( \"other\" );}\n"
		"for(let i=0;i<3;i+=1){app.log(i.toString());}\n"
		"let items:number[]=[1,2,-3];let x=items[0]-1;\n"
		"for (;;) { break; }\n"
		"}\n"
		"fn app.empty() {}\n";
	const char* expected =
		"// counter\n"
		"let counter: number = 0;\n"
		"\n"
		"fn app.increment() {\n"
		"    counter += 1;\n"
		"    ui.setText(\"counter\", counter.toString()); // show it\n"
		"    if (counter > 10 && !false) {\n"
		"        counter = -1;\n"
		"    }\n"
		"    else if (counter == 3) {\n"
		"        app.log(\"three\");\n"
		"    }\n"
		"    else {\n"
		"        app.log(\"other\");\n"
		"    }\n"
		"    for (let i = 0; i < 3; i += 1) {\n"
		"        app.log(i.toString());\n"
		"    }\n"
		"    let items: number[] = [1, 2, -3];\n"
		"    let x = items[0] - 1;\n"
		"    for (;;) {\n"
		"        break;\n"
		"    }\n"
		"}\n"
		"fn app.empty() {}\n";
	char* once;
	char* twice;
	rs_format_options options;
	printf("formatter\n");
	once = rs_format(messy, strlen(messy), NULL);
	CHECK_STR(once, expected);
	twice = once ? rs_format(once, strlen(once), NULL) : NULL;
	CHECK_STR(twice, expected);
	rs_free(once);
	rs_free(twice);
	/* a statement split by its author stays split */
	{
		const char* split = "fn a.b() {\n    let total = test.add(1,\n        2);\n    let s = \"a\" +\n\"b\";\n}\n";
		char* result = rs_format(split, strlen(split), NULL);
		CHECK_STR(result, "fn a.b() {\n    let total = test.add(1,\n        2);\n    let s = \"a\" +\n        \"b\";\n}\n");
		twice = result ? rs_format(result, strlen(result), NULL) : NULL;
		CHECK_STR(twice, result);
		rs_free(result);
		rs_free(twice);
	}
	/* a syntax error: nothing */
	CHECK(rs_format("fn a.b( {", strlen("fn a.b( {"), NULL) == NULL);
	CHECK(rs_format("let s = \"x", strlen("let s = \"x"), NULL) == NULL);
	/* tabs and a prefix (a <Script> indented in XML) */
	memset(&options, 0, sizeof(options));
	options.indent = "\t";
	options.prefix = "        ";
	once = rs_format("fn a.b() {\nlet x = 1;\n\n}", strlen("fn a.b() {\nlet x = 1;\n\n}"), &options);
	CHECK_STR(once, "        fn a.b() {\n        \tlet x = 1;\n        }\n");
	rs_free(once);
	once = rs_format("", 0, NULL);
	CHECK_STR(once, "");
	rs_free(once);
}

static void test_analysis(void) {
	rs_env* env = new_env();
	rs_batch* batch = rs_batch_create(env, RS_BATCH_ANALYSIS);
	const char* source =
		"let counter: number = 0;\n"
		"fn app.increment(step: number) {\n"
		"    let label = \"x\";\n"
		"    counter += step;\n"
		"    ui.setText(\"counter\", counter.toString());\n"
		"    app.\n"
		"}\n";
	char* json;
	char* items;
	uint32_t offset;
	printf("analysis\n");
	g_quiet = 1;
	rs_batch_add(batch, "a.rvs", source, strlen(source), 1, 1);
	rs_batch_compile(batch);
	g_quiet = 0;
	json = rs_batch_analysis_json(batch);
	CHECK_CONTAINS(json, "\"name\":\"app.increment\"");
	CHECK_CONTAINS(json, "\"kind\":\"parameter\",\"name\":\"step\"");
	CHECK_CONTAINS(json, "\"code\":\"RS002\"");
	CHECK_CONTAINS(json, "(native) fn ui.setText(id: string, text: string): void");
	CHECK_CONTAINS(json, "\"label\":\"ui.setText(id: string, text: string): void\"");
	rs_free(json);
	/* after "app." */
	offset = (uint32_t)(strstr(source, "app.\n") - source) + 4;
	items = rs_batch_complete_json(batch, 0, offset);
	CHECK_CONTAINS(items, "\"label\":\"log\"");
	CHECK_CONTAINS(items, "\"label\":\"increment\"");
	CHECK(strstr(items, "\"label\":\"setText\"") == NULL);
	rs_free(items);
	/* after "counter." */
	offset = (uint32_t)(strstr(source, "counter.toString") - source) + 8;
	items = rs_batch_complete_json(batch, 0, offset);
	CHECK_CONTAINS(items, "\"label\":\"toString\"");
	CHECK_CONTAINS(items, "\"label\":\"toFixed\"");
	CHECK(strstr(items, "\"label\":\"push\"") == NULL);
	rs_free(items);
	/* a word inside the function: its locals, the globals, the namespaces */
	offset = (uint32_t)(strstr(source, "    counter +=") - source) + 4;
	items = rs_batch_complete_json(batch, 0, offset);
	CHECK_CONTAINS(items, "\"label\":\"label\"");
	CHECK_CONTAINS(items, "\"label\":\"step\"");
	CHECK_CONTAINS(items, "\"label\":\"counter\"");
	CHECK_CONTAINS(items, "\"label\":\"ui\",\"kind\":\"module\"");
	CHECK_CONTAINS(items, "\"label\":\"while\"");
	rs_free(items);
	/* inside a string: nothing */
	offset = (uint32_t)(strstr(source, "\"counter\"") - source) + 3;
	items = rs_batch_complete_json(batch, 0, offset);
	CHECK_STR(items, "[]");
	rs_free(items);
	/* a type */
	offset = (uint32_t)(strstr(source, "number = 0") - source);
	items = rs_batch_complete_json(batch, 0, offset);
	CHECK_CONTAINS(items, "\"label\":\"string\",\"kind\":\"type\"");
	rs_free(items);
	rs_batch_destroy(batch);
	json = rs_language_json();
	CHECK_CONTAINS(json, "\"event\":\"Slider.OnChanged\"");
	CHECK_CONTAINS(json, "\"name\":\"number.parse\"");
	rs_free(json);
	rs_env_destroy(env);
}

static void test_validation(void) {
	rs_chunk* chunk = rs_chunk_create("bad");
	char error[256];
	printf("validation\n");
	chunk->local_count = 1;
	chunk->code = (rs_instr*)rs_calloc(4, sizeof(rs_instr));
	chunk->capacity = 4;
	/* pops what is not there */
	chunk->code[0].op = OP_POP;
	chunk->code[1].op = OP_RETURN_VOID;
	chunk->count = 2;
	CHECK(!rs_chunk_validate(chunk, 0, error, sizeof(error)));
	CHECK_CONTAINS(error, "underflow");
	/* a jump outside */
	chunk->code[0].op = OP_JUMP;
	chunk->code[0].a = 7;
	CHECK(!rs_chunk_validate(chunk, 0, error, sizeof(error)));
	/* a local outside */
	chunk->code[0].op = OP_LOAD_LOCAL;
	chunk->code[0].a = 3;
	CHECK(!rs_chunk_validate(chunk, 0, error, sizeof(error)));
	/* runs past the end */
	chunk->code[0].op = OP_TRUE;
	chunk->code[1].op = OP_POP;
	CHECK(!rs_chunk_validate(chunk, 0, error, sizeof(error)));
	/* inconsistent stack at a join */
	chunk->code[0].op = OP_TRUE;
	chunk->code[1].op = OP_JUMP_IF_FALSE;
	chunk->code[1].a = 3;
	chunk->code[2].op = OP_TRUE;
	chunk->code[3].op = OP_RETURN_VOID;
	chunk->count = 4;
	CHECK(!rs_chunk_validate(chunk, 0, error, sizeof(error)));
	/* fine */
	chunk->code[2].op = OP_NOP;
	CHECK(rs_chunk_validate(chunk, 0, error, sizeof(error)));
	/* an unknown global */
	chunk->code[2].op = OP_LOAD_GLOBAL;
	chunk->code[2].a = 0;
	CHECK(!rs_chunk_validate(chunk, 0, error, sizeof(error)));
	rs_chunk_release(chunk);
}

static void test_numbers(void) {
	char text[64];
	double value;
	printf("numbers\n");
	CHECK(rs_parse_number("42", 2, &value) && value == 42.0);
	CHECK(rs_parse_number("3.14", 4, &value) && value == 3.14);
	CHECK(rs_parse_number("1e-3", 4, &value) && value == 0.001);
	CHECK(rs_parse_number("0x1F", 4, &value) && value == 31.0);
	CHECK(!rs_parse_number("1e", 2, &value));
	CHECK(!rs_parse_number("12a", 3, &value));
	CHECK(rs_parse_number_text(" -3.5 ", &value) && value == -3.5);
	CHECK(rs_parse_number_text("Infinity", &value) && value > 1e308);
	CHECK(!rs_parse_number_text("", &value));
	CHECK(!rs_parse_number_text("nan", &value));
	CHECK(!rs_parse_number_text("1 2", &value));
	rs_format_number(0.1, text, sizeof(text));
	CHECK_STR(text, "0.1");
	rs_format_number(-2.5e-7, text, sizeof(text));
	CHECK_STR(text, "-2.5e-07");
	rs_format_number(123456789012.0, text, sizeof(text));
	CHECK_STR(text, "123456789012");
	rs_format_number(0.0 / 1.0 * -1.0, text, sizeof(text));
	CHECK_STR(text, "0");
}

/* ---------------------------------------------------------------------------
 *  Fuzzing: mutations of valid scripts never crash anything
 * ------------------------------------------------------------------------- */

static uint32_t g_seed = 12345;
static uint32_t next_random(void) {
	g_seed = g_seed * 1103515245u + 12345u;
	return (g_seed >> 8) & 0xFFFFFF;
}

static void test_fuzz(void) {
	static const char* const seeds[] = {
		"let counter: number = 0;\nfn app.increment() {\n    counter += 1;\n    ui.setText(\"counter\", counter.toString());\n}\n",
		"fn math.fib(n: number): number {\n    if (n < 2) {\n        return n;\n    }\n    return math.fib(n - 1) + math.fib(n - 2);\n}\nfn app.go(): number {\n    return math.fib(10);\n}\n",
		"fn app.example() {\n    let numbers: number[] = [1, 2, 3];\n    for (let i = 0; i < 10; i += 1) {\n        if (i == 5) {\n            break;\n        }\n        numbers.push(i);\n        app.log(numbers[i % 3].toString());\n    }\n    while (numbers.length > 0) {\n        numbers.pop();\n    }\n}\n",
		"let s = \"a,b,c\";\nfn app.text(): string {\n    let parts = s.split(\",\");\n    return parts[0].toUpperCase() + s.substring(1, 3) + number.parse(\"4\").toString();\n}\n"
	};
	static const char* const pieces[] = { "(", ")", "{", "}", "[", "]", ";", ",", ".", "\"", "fn ", "let ", "1", "x", "+", "/", "==", "!", "-", "return ", "null", "[]", "app.", "0x", "1e", "\\", "\n", "while (true) {}", "/*", "*/", "//" };
	int round, rounds = 3000;
	printf("fuzz\n");
	g_quiet = 1;
	/* RS_FUZZ_ROUNDS and RS_FUZZ_SEED run it longer, elsewhere */
	if (getenv("RS_FUZZ_ROUNDS")) rounds = atoi(getenv("RS_FUZZ_ROUNDS"));
	if (getenv("RS_FUZZ_SEED")) g_seed = (uint32_t)atoi(getenv("RS_FUZZ_SEED"));
	for (round = 0; round < rounds; round++) {
		const char* seed = seeds[round % RS_COUNT_OF(seeds)];
		size_t length = strlen(seed);
		char text[2048];
		size_t n = 0, i;
		int mutations = 1 + (int)(next_random() % 4), m;
		rs_env* env;
		memcpy(text, seed, length + 1);
		n = length;
		for (m = 0; m < mutations; m++) {
			uint32_t kind = next_random() % 3;
			size_t at = n ? next_random() % n : 0;
			if (kind == 0 && n > 1) {
				/* delete a few bytes */
				size_t count = 1 + next_random() % 4;
				if (at + count > n) count = n - at;
				memmove(text + at, text + at + count, n - at - count + 1);
				n -= count;
			} else if (kind == 1) {
				/* insert a piece */
				const char* piece = pieces[next_random() % RS_COUNT_OF(pieces)];
				size_t piece_length = strlen(piece);
				if (n + piece_length + 1 >= sizeof(text)) continue;
				memmove(text + at + piece_length, text + at, n - at + 1);
				memcpy(text + at, piece, piece_length);
				n += piece_length;
			} else if (n > 0) {
				/* replace a byte, sometimes by a byte outside ASCII */
				text[at] = (char)(next_random() % 4 == 0 ? 0x80 + next_random() % 0x7F : 0x20 + next_random() % 0x5F);
			}
		}
		if (getenv("RS_FUZZ_TRACE")) {
			FILE* trace = fopen("/tmp/rs/fuzz_last.rvs", "wb");
			if (trace) {
				fwrite(text, 1, n, trace);
				fclose(trace);
			}
			printf("round %d\n", round);
		}
		env = new_env();
		rs_env_set_limits(env, 20000, 30, 1 << 20);
		{
			rs_batch* batch = rs_batch_create(env, RS_BATCH_ANALYSIS);
			char* json;
			rs_batch_add(batch, "fuzz.rvs", text, n, 1, 1);
			if (rs_batch_compile(batch) == 0 && rs_batch_install(batch)) {
				rs_batch_initialize(batch);
				for (i = 0; i < (size_t)rs_env_function_count(env); i++) {
					rs_value result;
					rs_value args[RS_MAX_PARAMS];
					rs_sig sig;
					int p;
					const char* name = rs_env_function_name(env, (int)i);
					rs_env_find_function(env, name, &sig);
					for (p = 0; p < sig.count; p++) args[p] = rs_value_default(NULL, sig.params[p]);
					rs_env_call(env, name, args, sig.count, RS_CALL_STRICT, NULL, &result);
					rs_value_release(result);
					for (p = 0; p < sig.count; p++) rs_value_release(args[p]);
				}
				CHECK(env->stack_top == 0 && env->frame_count == 0 && env->depth == 0);
			}
			json = rs_batch_analysis_json(batch);
			rs_free(json);
			json = rs_batch_complete_json(batch, 0, (uint32_t)(n ? next_random() % (n + 1) : 0));
			rs_free(json);
			rs_batch_destroy(batch);
		}
		{
			char* formatted = rs_format(text, n, NULL);
			if (formatted) {
				/* formatting keeps the meaning: the result parses and formats to itself */
				char* again = rs_format(formatted, strlen(formatted), NULL);
				CHECK(again != NULL);
				if (again && strcmp(again, formatted) != 0) {
					g_failures++;
					printf("  FAIL formatter not idempotent on:\n%s\n---\n%s\n---\n%s\n", text, formatted, again);
				}
				rs_free(again);
				rs_free(formatted);
			}
		}
		rs_env_destroy(env);
	}
	g_quiet = 0;
}

int main(void) {
	setvbuf(stdout, NULL, _IONBF, 0);
	test_numbers();
	test_syntax();
	test_types();
	test_events();
	test_run();
	test_runtime_errors();
	test_reload();
	test_multi_unit();
	test_format_diagnostic();
	test_formatter();
	test_analysis();
	test_validation();
	test_fuzz();
	reset_records();
	printf("test_script_core: %d checks, %d failures\n", g_checks, g_failures);
	return g_failures == 0 ? 0 : 1;
}
