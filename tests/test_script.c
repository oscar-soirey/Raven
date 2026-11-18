/**
 * Raven tests - Raven Script in the library: <Script> in XML documents
 * (inline and Src), actions and their events, Raction_bind_with_parameters,
 * the natives (app, ui, window), the diagnostics, live reload, the
 * document API, the public functions of raven_script.h.
 */

#include "test.h"

#include <stdlib.h>

/* ---------------------------------------------------------------------------
 *  What the scripts say
 * ------------------------------------------------------------------------- */

static char g_log[4096];
static char g_diagnostics[16384];
static char g_codes[512];
static int g_diagnostic_count;

static void on_log(const char* message, void* user_data) {
	(void)user_data;
	if (strlen(g_log) + strlen(message) + 2 < sizeof(g_log)) {
		strcat(g_log, message);
		strcat(g_log, "\n");
	}
}

static void on_diagnostic(RSeverity severity, const char* code, const char* file, int line, int column, const char* text, void* user_data) {
	(void)severity;
	(void)file;
	(void)line;
	(void)column;
	(void)user_data;
	g_diagnostic_count++;
	if (strlen(g_codes) + strlen(code) + 2 < sizeof(g_codes)) {
		strcat(g_codes, code);
		strcat(g_codes, " ");
	}
	if (strlen(g_diagnostics) + strlen(text) + 2 < sizeof(g_diagnostics)) {
		strcat(g_diagnostics, text);
		strcat(g_diagnostics, "\n");
	}
	if (getenv("RS_TEST_VERBOSE")) printf("%s\n", text);
}

static void clear(void) {
	g_log[0] = '\0';
	g_diagnostics[0] = '\0';
	g_codes[0] = '\0';
	g_diagnostic_count = 0;
}

/* a text and its length */
#define TEXT(literal) literal, strlen(literal)

#define CHECK_CONTAINS(haystack, needle) do { \
	const char* test_h_ = (haystack); \
	g_test_checks++; \
	if (!test_h_ || !strstr(test_h_, (needle))) { \
		g_test_failures++; \
		printf("  FAIL %s:%d: \"%s\" does not contain \"%s\"\n", __FILE__, __LINE__, test_h_ ? test_h_ : "(null)", (needle)); \
	} \
} while (0)

static Rid window_from(const char* xml) {
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	if (window != R_INVALID_ID) Rdraw_frame();
	return window;
}

static const char* temp_dir(void) {
	const char* temp = getenv("TMPDIR") ? getenv("TMPDIR") : (getenv("TEMP") ? getenv("TEMP") : "/tmp");
	return temp;
}

static void write_text(const char* path, const char* text) {
	CHECK(Rfile_write(path, text, strlen(text)));
}

/* ---------------------------------------------------------------------------
 *  Tests
 * ------------------------------------------------------------------------- */

/* The example of the specification (16): a counter. The script uses '<' and '&&' as they are. */
static void test_counter(void) {
	static const char xml[] =
		"<Window Title=\"Counter\" Width=\"300\" Height=\"200\">\n"
		"    <Script>\n"
		"        let counter: number = 0;\n"
		"\n"
		"        fn app.increment() {\n"
		"            counter += 1;\n"
		"            if (counter > 0 && counter < 100) {\n"
		"                ui.setText(\"counter\", counter.toString());\n"
		"            }\n"
		"        }\n"
		"\n"
		"        fn app.reset() {\n"
		"            counter = 0;\n"
		"            ui.setText(\"counter\", \"0\");\n"
		"        }\n"
		"    </Script>\n"
		"\n"
		"    <VBox>\n"
		"        <Text Name=\"counter\" Text=\"0\"/>\n"
		"        <Button Name=\"increment\" Text=\"Increment\" OnClicked=\"app.increment\"/>\n"
		"        <Button Name=\"reset\" Text=\"Reset\" OnClicked=\"app.reset()\"/>\n"
		"    </VBox>\n"
		"</Window>\n";
	Rid window, label, increment, reset;
	clear();
	window = window_from(xml);
	CHECK(window != R_INVALID_ID);
	CHECK(g_diagnostic_count == 0);
	label = Rwidget_find(window, "counter");
	increment = Rwidget_find(window, "increment");
	reset = Rwidget_find(window, "reset");
	CHECK(Rscript_has_function("app.increment"));
	/* the <Script> is not a widget: the window has its VBox */
	CHECK(Rwidget_get_type(Rwindow_get_content(window)) == R_WIDGET_VBOX);
	/* "app.reset()" names the same action as "app.reset" */
	CHECK_STR(Rwidget_get_action(reset), "app.reset");
	test_click(window, increment);
	test_click(window, increment);
	CHECK_STR(Rlabel_get_text(label), "2");
	test_click(window, increment);
	CHECK_STR(Rlabel_get_text(label), "3");
	test_click(window, reset);
	CHECK_STR(Rlabel_get_text(label), "0");
	/* the action is in the registry of Raction_bind: Raction_fire runs it */
	Raction_fire("app.increment", increment);
	CHECK_STR(Rlabel_get_text(label), "1");
	CHECK(g_diagnostic_count == 0);
	Rwindow_destroy(window);
}

static int g_volume_calls;
static double g_volume;

static void on_typed_volume(RActionContext* context, void* user_data) {
	(void)user_data;
	g_volume_calls++;
	CHECK(Raction_get_argument_count(context) == 1);
	g_volume = Raction_get_number(context, 0);
	CHECK(Raction_get_sender(context) != R_INVALID_ID);
	CHECK_STR(Raction_get_name(context), "c.volume");
}

/* Every widget gives its function what its event carries. */
static void test_events(void) {
	static const char xml[] =
		"<Window Title=\"Events\" Width=\"600\" Height=\"500\">\n"
		"  <Script>\n"
		"    fn app.volume(value: number) {\n"
		"        app.log(\"Volume: \" + value.toString());\n"
		"    }\n"
		"    fn app.checked(checked: bool) {\n"
		"        if (checked) {\n"
		"            app.log(\"Enabled\");\n"
		"        }\n"
		"        else {\n"
		"            app.log(\"Disabled\");\n"
		"        }\n"
		"    }\n"
		"    fn app.typed(text: string) {\n"
		"        app.log(\"Text: \" + text);\n"
		"    }\n"
		"    fn app.chosen(index: number, text: string) {\n"
		"        app.log(\"Chosen: \" + index.toString() + \" \" + text);\n"
		"    }\n"
		"    fn app.index_only(index: number) {\n"
		"        app.log(\"Index: \" + index.toString());\n"
		"    }\n"
		"    fn app.nothing() {\n"
		"        app.log(\"Nothing\");\n"
		"    }\n"
		"    fn app.date(year: number, month: number, day: number) {\n"
		"        app.log(\"Date: \" + year.toString() + \"-\" + month.toString() + \"-\" + day.toString());\n"
		"    }\n"
		"  </Script>\n"
		"  <VBox>\n"
		"    <Slider Name=\"slider\" Min=\"0\" Max=\"100\" OnChanged=\"app.volume\"/>\n"
		"    <Checkbox Name=\"check\" Text=\"Enable\" OnChanged=\"app.checked\"/>\n"
		"    <TextInput Name=\"input\" OnCommitted=\"app.typed\"/>\n"
		"    <ComboBox Name=\"combo\" Items=\"Red|Green|Blue\" OnChanged=\"app.chosen\"/>\n"
		"    <ComboBox Name=\"combo2\" Items=\"A|B\" OnChanged=\"app.index_only\"/>\n"
		"    <ComboBox Name=\"combo3\" Items=\"A|B\" OnChanged=\"app.nothing\"/>\n"
		"    <DatePicker Name=\"date\" OnChanged=\"app.date\"/>\n"
		"    <Slider Name=\"typed\" Min=\"0\" Max=\"10\" OnChanged=\"c.volume\"/>\n"
		"  </VBox>\n"
		"</Window>\n";
	Rid window, check;
	clear();
	Rscript_set_log_callback(on_log, NULL);
	window = window_from(xml);
	CHECK(window != R_INVALID_ID);
	CHECK(g_diagnostic_count == 0);
	check = Rwidget_find(window, "check");
	/* a real click on the check box */
	test_click(window, check);
	CHECK(Rcheckbox_is_checked(check));
	CHECK_STR(g_log, "Enabled\n");
	test_click(window, check);
	CHECK_STR(g_log, "Enabled\nDisabled\n");
	/* the others: their value set, then their action fired as they fire it */
	clear();
	Rslider_set_value(Rwidget_find(window, "slider"), 42.0f);
	Raction_fire("app.volume", Rwidget_find(window, "slider"));
	Rtext_input_set_text(Rwidget_find(window, "input"), "héllo");
	Raction_fire("app.typed", Rwidget_find(window, "input"));
	Rcombo_box_set_selected(Rwidget_find(window, "combo"), 2);
	Raction_fire("app.chosen", Rwidget_find(window, "combo"));
	Rcombo_box_set_selected(Rwidget_find(window, "combo2"), 1);
	Raction_fire("app.index_only", Rwidget_find(window, "combo2"));
	Raction_fire("app.nothing", Rwidget_find(window, "combo3"));
	Rdate_picker_set_date(Rwidget_find(window, "date"), 2026, 11, 18);
	Raction_fire("app.date", Rwidget_find(window, "date"));
	CHECK_STR(g_log, "Volume: 42\nText: héllo\nChosen: 2 Blue\nIndex: 1\nNothing\nDate: 2026-11-18\n");
	CHECK(g_diagnostic_count == 0);

	/* a function bound in C with parameters gets the value of the event too */
	CHECK(Raction_bind_with_parameters("c.volume", "(value: number)", on_typed_volume, NULL));
	g_volume_calls = 0;
	Rslider_set_value(Rwidget_find(window, "typed"), 7.0f);
	Raction_fire("c.volume", Rwidget_find(window, "typed"));
	CHECK(g_volume_calls == 1 && g_volume == 7.0);
	/* fired without a widget, it is not called: the event gives no value */
	Raction_fire("c.volume", R_INVALID_ID);
	CHECK(g_volume_calls == 1);
	CHECK_CONTAINS(g_codes, "RS103");
	/* a script function the same */
	clear();
	Raction_fire("app.volume", R_INVALID_ID);
	CHECK_CONTAINS(g_codes, "RS103");
	CHECK_CONTAINS(g_diagnostics, "requires 1");
	CHECK_STR(g_log, "");
	Raction_unbind("c.volume");
	Rscript_set_log_callback(NULL, NULL);
	Rwindow_destroy(window);
}

/* RS103 when the document loads (specification 12.5 and 12.6) */
static void test_handler_signatures(void) {
	static const char xml[] =
		"<Window Title=\"Bad\" Width=\"300\" Height=\"200\">\n"
		"  <Script>\n"
		"    fn app.on_volume_changed(value: bool) {\n"
		"        app.log(\"Changed\");\n"
		"    }\n"
		"    fn app.two(value: number, enabled: bool) {\n"
		"        app.log(\"Changed\");\n"
		"    }\n"
		"  </Script>\n"
		"  <VBox>\n"
		"    <Slider OnChanged=\"app.on_volume_changed\"/>\n"
		"    <Slider OnChanged=\"app.two\"/>\n"
		"    <Button OnClicked=\"app.two\"/>\n"
		"  </VBox>\n"
		"</Window>\n";
	Rid window;
	clear();
	window = window_from(xml);
	/* the window loads: a wrong handler does not stop the interface */
	CHECK(window != R_INVALID_ID);
	CHECK_STR(g_codes, "RS103 RS103 RS103 ");
	CHECK_CONTAINS(g_diagnostics, "--> <xml>:11:5");
	CHECK_CONTAINS(g_diagnostics, "<Slider OnChanged=\"app.on_volume_changed\"/>");
	CHECK_CONTAINS(g_diagnostics, "Slider.OnChanged expects a function with\n  1 parameter of type 'number'.");
	CHECK_CONTAINS(g_diagnostics, "--> <xml>:3:37");
	CHECK_CONTAINS(g_diagnostics, "Change the parameter type from 'bool' to 'number'.");
	CHECK_CONTAINS(g_diagnostics, "Slider.OnChanged provides 1 argument,\n  but 'app.two' requires 2.");
	CHECK_CONTAINS(g_diagnostics, "Button.OnClicked provides 0 arguments,\n  but 'app.two' requires 2.");
	Rwindow_destroy(window);
}

/* A mistake in a script: reported with its place in the document, nothing of the scripts installed, the window loads. */
static void test_compile_errors(void) {
	static const char xml[] =
		"<Window Title=\"Errors\" Width=\"300\" Height=\"200\">\n"
		"  <Script>\n"
		"    fn app.fine() {\n"
		"    }\n"
		"  </Script>\n"
		"  <Script>\n"
		"    let count: number = \"hello\";\n"
		"  </Script>\n"
		"  <Label Text=\"still here\"/>\n"
		"</Window>\n";
	Rid window;
	clear();
	window = window_from(xml);
	CHECK(window != R_INVALID_ID);
	CHECK_STR(g_codes, "RS102 ");
	CHECK_CONTAINS(g_diagnostics, "--> <xml>:7:25");
	CHECK_CONTAINS(g_diagnostics, " 7 |     let count: number = \"hello\";\n   |                         ^^^^^^^");
	CHECK(!Rscript_has_function("app.fine"));
	CHECK(Rwidget_get_type(Rwindow_get_content(window)) == R_WIDGET_LABEL);
	Rwindow_destroy(window);
	/* XML errors of a <Script> */
	EXPECT_ERRORS_BEGIN();
	CHECK(window_from("<Window><Script>fn a.b() {}</Window>") == R_INVALID_ID);
	EXPECT_ERRORS_END(1);
	EXPECT_ERRORS_BEGIN();
	CHECK(Rwidget_create_from_xml(TEXT("<Script>fn a.b() {}</Script>")) == R_INVALID_ID);
	EXPECT_ERRORS_END(1);
}

/* Errors while running stop the script, not the application; the next event runs. */
static void test_runtime_errors(void) {
	static const char xml[] =
		"<Window Title=\"Run\" Width=\"300\" Height=\"200\">\n"
		"  <Script><![CDATA[\n"
		"    let items: number[] = [1, 2, 3];\n"
		"    fn app.crash() {\n"
		"        let zero = 0;\n"
		"        app.log((1 / zero).toString());\n"
		"    }\n"
		"    fn app.index() {\n"
		"        app.log(items[10].toString());\n"
		"    }\n"
		"    fn app.forever() {\n"
		"        while (true) {\n"
		"        }\n"
		"    }\n"
		"    fn app.deep(n: number): number {\n"
		"        return app.deep(n + 1);\n"
		"    }\n"
		"    fn app.go_deep() {\n"
		"        app.deep(0);\n"
		"    }\n"
		"    fn app.nobody() {\n"
		"        ui.setText(\"nobody\", \"x\");\n"
		"    }\n"
		"    fn app.ok() {\n"
		"        app.log(\"ok \" + items.length.toString());\n"
		"    }\n"
		"  ]]></Script>\n"
		"  <Button Name=\"b\" OnClicked=\"app.crash\"/>\n"
		"</Window>\n";
	Rid window, button;
	clear();
	Rscript_set_log_callback(on_log, NULL);
	Rscript_set_limits(200000, 0, 0);
	window = window_from(xml);
	CHECK(window != R_INVALID_ID);
	CHECK_STR(g_codes, "RS301 ");
	button = Rwidget_find(window, "b");
	clear();
	test_click(window, button);
	CHECK_STR(g_codes, "RS201 ");
	CHECK_CONTAINS(g_diagnostics, "--> <xml>:6:17");
	Raction_fire("app.index", button);
	Raction_fire("app.forever", button);
	Raction_fire("app.go_deep", button);
	Raction_fire("app.nobody", button);
	CHECK_STR(g_codes, "RS201 RS204 RS203 RS202 RS301 ");
	Raction_fire("app.ok", button);
	CHECK_STR(g_log, "ok 3\n");
	Rscript_set_limits(10000000, 200, 64u * 1024u * 1024u);
	Rscript_set_log_callback(NULL, NULL);
	Rwindow_destroy(window);
}

/* The natives that act on widgets and windows */
static void test_natives(void) {
	static const char xml[] =
		"<Window Title=\"Natives\" Width=\"400\" Height=\"300\">\n"
		"  <Script>\n"
		"    let title: string = ui.getText(\"title\");\n"
		"    fn app.run() {\n"
		"        ui.setText(\"title\", title + \"!\");\n"
		"        ui.setVisible(\"panel\", false);\n"
		"        ui.setEnabled(\"button\", false);\n"
		"        ui.setValue(\"slider\", 0.75);\n"
		"        ui.setChecked(\"check\", true);\n"
		"        ui.setValue(\"combo\", 1);\n"
		"        ui.setAttribute(\"title\", \"Opacity\", \"0.5\");\n"
		"        app.log(ui.getText(\"title\") + \" \" + ui.isVisible(\"panel\").toString() + \" \" + ui.isEnabled(\"button\").toString());\n"
		"        app.log(ui.getValue(\"slider\").toString() + \" \" + ui.isChecked(\"check\").toString() + \" \" + ui.getText(\"combo\") + \" \" + ui.getValue(\"combo\").toString());\n"
		"        app.log(ui.exists(\"title\").toString() + \" \" + ui.exists(\"nothing\").toString() + \" \" + ui.getText(\"input\"));\n"
		"        window.setTitle(\"Changed\");\n"
		"        ui.focus(\"input\");\n"
		"    }\n"
		"    fn app.bad_value() {\n"
		"        ui.setValue(\"title\", 1);\n"
		"    }\n"
		"    fn app.bad_attribute() {\n"
		"        ui.setAttribute(\"title\", \"Nonsense\", \"1\");\n"
		"    }\n"
		"    fn app.show() {\n"
		"        app.fire(\"show:panel\");\n"
		"    }\n"
		"  </Script>\n"
		"  <VBox>\n"
		"    <Label Name=\"title\" Text=\"Hello\"/>\n"
		"    <VBox Name=\"panel\"><Label Text=\"inside\"/></VBox>\n"
		"    <Button Name=\"button\" Text=\"Run\" OnClicked=\"app.run\"/>\n"
		"    <Slider Name=\"slider\" Min=\"0\" Max=\"1\"/>\n"
		"    <Checkbox Name=\"check\"/>\n"
		"    <ComboBox Name=\"combo\" Items=\"One|Two\"/>\n"
		"    <TextInput Name=\"input\" Text=\"typed\"/>\n"
		"  </VBox>\n"
		"</Window>\n";
	Rid window, button;
	clear();
	Rscript_set_log_callback(on_log, NULL);
	window = window_from(xml);
	CHECK(window != R_INVALID_ID);
	CHECK(g_diagnostic_count == 0);
	button = Rwidget_find(window, "button");
	test_click(window, button);
	CHECK_STR(g_log, "Hello! false false\n0.75 true Two 1\ntrue false typed\n");
	CHECK_STR(Rwindow_get_title(window), "Changed");
	CHECK(Rwidget_get_visibility(Rwidget_find(window, "panel")) == R_COLLAPSED);
	CHECK(!Rwidget_is_enabled(button));
	CHECK(Rwidget_is_focused(Rwidget_find(window, "input")));
	Raction_fire("app.show", button);
	CHECK(Rwidget_get_visibility(Rwidget_find(window, "panel")) == R_VISIBLE);
	Raction_fire("app.bad_value", button);
	Raction_fire("app.bad_attribute", button);
	CHECK_STR(g_codes, "RS205 RS205 ");
	Rscript_set_log_callback(NULL, NULL);
	Rwindow_destroy(window);
}

static int g_plain_calls;
static void on_plain(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	g_plain_calls++;
}

static void on_add(RActionContext* context, void* user_data) {
	(void)user_data;
	Raction_return_number(context, Raction_get_number(context, 0) + Raction_get_number(context, 1));
}

static void on_greet(RActionContext* context, void* user_data) {
	char text[128];
	(void)user_data;
	snprintf(text, sizeof(text), "Hello, %s%s", Raction_get_string(context, 0), Raction_get_bool(context, 1) ? "!" : ".");
	Raction_return_string(context, text);
}

static void on_fail(RActionContext* context, void* user_data) {
	(void)user_data;
	Raction_fail(context, "the disk is full");
}

static void on_wrong(RActionContext* context, void* user_data) {
	(void)user_data;
	Raction_return_string(context, "not a number");
}

static void on_nothing(RActionContext* context, void* user_data) {
	(void)context;
	(void)user_data;
}

/* Functions of the application, with and without parameters, called by scripts */
static void test_native_bindings(void) {
	static const char script[] =
		"declare fn my.later(x: number): number;\n"
		"fn app.compute() {\n"
		"    plain.action();\n"
		"    app.log(my.add(2, 3).toString());\n"
		"    app.log(my.greet(\"Raven\", true));\n"
		"}\n"
		"fn app.fails() {\n"
		"    my.fail();\n"
		"}\n"
		"fn app.wrong(): number {\n"
		"    return my.wrong() + 1;\n"
		"}\n"
		"fn app.nothing(): number {\n"
		"    return my.nothing();\n"
		"}\n"
		"fn app.later() {\n"
		"    app.log(my.later(4).toString());\n"
		"}\n";
	clear();
	Rscript_set_log_callback(on_log, NULL);
	g_plain_calls = 0;
	Raction_bind("plain.action", on_plain, NULL);
	CHECK(Raction_bind_with_parameters("my.add", "(a: number, b: number): number", on_add, NULL));
	CHECK(Raction_bind_with_parameters("my.greet", "(string, bool): string", on_greet, NULL));
	CHECK(Raction_bind_with_parameters("my.fail", "()", on_fail, NULL));
	CHECK(Raction_bind_with_parameters("my.wrong", "(): number", on_wrong, NULL));
	CHECK(Raction_bind_with_parameters("my.nothing", "(): number", on_nothing, NULL));
	EXPECT_ERRORS_BEGIN();
	CHECK(!Raction_bind_with_parameters("my.bad", "(number[])", on_nothing, NULL));
	CHECK(!Raction_bind_with_parameters("my.bad", "(thing)", on_nothing, NULL));
	CHECK(!Raction_bind_with_parameters("my.bad", "number", on_nothing, NULL));
	EXPECT_ERRORS_END(3);
	CHECK(Rscript_load(script, strlen(script), "bindings.rvs"));
	CHECK(g_diagnostic_count == 0);
	Raction_fire("app.compute", R_INVALID_ID);
	CHECK(g_plain_calls == 1);
	CHECK_STR(g_log, "5\nHello, Raven!\n");
	Raction_fire("app.fails", R_INVALID_ID);
	CHECK_STR(g_codes, "RS205 ");
	CHECK_CONTAINS(g_diagnostics, "the disk is full");
	CHECK_CONTAINS(g_diagnostics, "--> bindings.rvs:8:5");
	Raction_fire("app.wrong", R_INVALID_ID);
	Raction_fire("app.nothing", R_INVALID_ID);
	CHECK_STR(g_codes, "RS205 RS205 RS205 ");
	/* declared, bound later */
	clear();
	Raction_fire("app.later", R_INVALID_ID);
	CHECK_STR(g_codes, "RS101 ");
	/* (bound with another signature than its declaration: refused when called) */
	Raction_bind_with_parameters("my.later", "(x: string): number", on_add, NULL);
	clear();
	Raction_fire("app.later", R_INVALID_ID);
	CHECK_STR(g_codes, "RS205 ");
	CHECK_CONTAINS(g_diagnostics, "argument");
	/* the type checker knows what the application bound */
	clear();
	CHECK(!Rscript_load(TEXT("fn x.y() {\n    my.add(\"a\", 1);\n}\n"), "check.rvs"));
	CHECK_STR(g_codes, "RS102 ");
	clear();
	CHECK(!Rscript_load(TEXT("fn my.add() {\n}\n"), "check.rvs"));
	CHECK_STR(g_codes, "RS107 ");
	/* binding a name replaces what it was bound to, and Raction_unbind removes it */
	Raction_bind("my.add", on_plain, NULL);
	clear();
	CHECK(Rscript_check(TEXT("fn x.y() {\n    my.add();\n}\n"), "check.rvs") == 0);
	Raction_unbind("my.add");
	clear();
	CHECK(Rscript_check(TEXT("fn x.y() {\n    my.add();\n}\n"), "check.rvs") == 1);
	Raction_unbind("plain.action");
	Raction_unbind("my.greet");
	Raction_unbind("my.fail");
	Raction_unbind("my.wrong");
	Raction_unbind("my.nothing");
	Raction_unbind("my.later");
	Rscript_set_log_callback(NULL, NULL);
}

/* <Script Src="..."> and Rscript_load_file, with live reload */
static void test_files(void) {
	char xml_path[512], script_path[512], text[1024];
	Rid window, label;
	clear();
	snprintf(script_path, sizeof(script_path), "%s/raven_test_main.rvs", temp_dir());
	snprintf(xml_path, sizeof(xml_path), "%s/raven_test_script.xml", temp_dir());
	write_text(script_path, "fn app.hello() {\n    ui.setText(\"out\", \"one\");\n}\n");
	snprintf(text, sizeof(text),
		"<Window Title=\"Files\" Width=\"300\" Height=\"200\">\n"
		"  <Script Src=\"raven_test_main.rvs\"/>\n"
		"  <VBox>\n"
		"    <Label Name=\"out\" Text=\"none\"/>\n"
		"    <Button Name=\"go\" OnClicked=\"app.hello\"/>\n"
		"  </VBox>\n"
		"</Window>\n");
	write_text(xml_path, text);
	Rxml_set_live_reload(R_TRUE);
	window = Rwindow_create_from_xml_file(xml_path);
	CHECK(window != R_INVALID_ID);
	CHECK(g_diagnostic_count == 0);
	label = Rwidget_find(window, "out");
	Raction_fire("app.hello", Rwidget_find(window, "go"));
	CHECK_STR(Rlabel_get_text(label), "one");
	/* the script changes: the window follows */
	write_text(script_path, "fn app.hello() {\n    ui.setText(\"out\", \"two\");\n}\n");
	Rxml_check_files();
	Raction_fire("app.hello", Rwidget_find(window, "go"));
	CHECK_STR(Rlabel_get_text(Rwidget_find(window, "out")), "two");
	/* a mistake in it: reported with the name of the file, the last version stays */
	clear();
	write_text(script_path, "fn app.hello() {\n    ui.setText(\"out\", 3);\n}\n");
	Rxml_check_files();
	CHECK_CONTAINS(g_diagnostics, "raven_test_main.rvs:2:23");
	Raction_fire("app.hello", Rwidget_find(window, "go"));
	CHECK_STR(Rlabel_get_text(Rwidget_find(window, "out")), "two");
	/* the document changes: an inline script replaces the file */
	write_text(xml_path,
		"<Window Title=\"Files\" Width=\"300\" Height=\"200\">\n"
		"  <Script>\n"
		"    fn app.hello() {\n"
		"        ui.setText(\"out\", \"three\");\n"
		"    }\n"
		"  </Script>\n"
		"  <VBox>\n"
		"    <Label Name=\"out\" Text=\"none\"/>\n"
		"    <Button Name=\"go\" OnClicked=\"app.hello\"/>\n"
		"  </VBox>\n"
		"</Window>\n");
	Rxml_check_files();
	Raction_fire("app.hello", Rwidget_find(window, "go"));
	CHECK_STR(Rlabel_get_text(Rwidget_find(window, "out")), "three");
	Rxml_set_live_reload(R_FALSE);
	Rwindow_destroy(window);
	/* a missing file */
	clear();
	EXPECT_ERRORS_BEGIN();
	window = window_from("<Window><Script Src=\"no_such_file.rvs\"/><Label/></Window>");
	EXPECT_ERRORS_END(1);
	CHECK(window != R_INVALID_ID);
	Rwindow_destroy(window);
	/* Rscript_load_file, Rscript_check_file */
	write_text(script_path, "let n = 41;\nfn app.answer(): number {\n    n += 1;\n    return n;\n}\n");
	clear();
	CHECK(Rscript_load_file(script_path));
	CHECK(Rscript_has_function("app.answer"));
	CHECK(Rscript_check_file(script_path) == 0);
	CHECK(Rscript_check_file(xml_path) == 0);
	write_text(xml_path, "<Window><Script>fn app.v(x: bool) {}</Script><Slider OnChanged=\"app.v\"/></Window>");
	CHECK(Rscript_check_file(xml_path) == 1);
	CHECK_STR(g_codes, "RS103 ");
	EXPECT_ERRORS_BEGIN();
	CHECK(Rscript_check_file("/no/such/file.rvs") == -1);
	CHECK(!Rscript_load_file("/no/such/file.rvs"));
	EXPECT_ERRORS_END(2);
	remove(script_path);
	remove(xml_path);
}

/* The document API keeps the code of a <Script> as it is */
static void test_documents(void) {
	static const char xml[] =
		"<Window Title=\"Dom\">\n"
		"  <Script>\n"
		"    fn app.compare(): bool {\n"
		"        return 1 < 2 && 3 > 2;\n"
		"    }\n"
		"  </Script>\n"
		"  <Label/>\n"
		"</Window>\n";
	RXmlError error;
	Rid document = Rxml_document_parse(xml, strlen(xml), &error);
	Rid script, window;
	const char* text;
	CHECK(document != R_INVALID_ID);
	script = Rxml_node_get_child(Rxml_document_get_root(document), 0);
	CHECK_STR(Rxml_node_get_tag(script), "Script");
	CHECK_CONTAINS(Rxml_node_get_text(script), "return 1 < 2 && 3 > 2;");
	text = Rxml_document_to_string(document);
	CHECK_CONTAINS(text, "return 1 < 2 && 3 > 2;");
	clear();
	window = Rxml_document_create_window(document);
	CHECK(window != R_INVALID_ID);
	CHECK(g_diagnostic_count == 0);
	CHECK(Rscript_has_function("app.compare"));
	Rwindow_destroy(window);
	Rxml_document_destroy(document);
}

/* Declared widgets (<Widget>) copy the code of their <Script> as it is: its braces are not parameters */
static void test_components(void) {
	static const char xml[] =
		"<Window Title=\"Components\">\n"
		"  <Widget Name=\"Greeter\" Message=\"hi\">\n"
		"    <VBox>\n"
		"      <Script>\n"
		"        fn greeter.say() {\n"
		"            if (true) { app.log(\"{Message}\"); }\n"
		"        }\n"
		"      </Script>\n"
		"      <Button Text=\"{Message}\" OnClicked=\"greeter.say\"/>\n"
		"    </VBox>\n"
		"  </Widget>\n"
		"  <Greeter Message=\"hello\"/>\n"
		"</Window>\n";
	Rid window;
	clear();
	Rscript_set_log_callback(on_log, NULL);
	window = window_from(xml);
	CHECK(window != R_INVALID_ID);
	CHECK(g_diagnostic_count == 0);
	Raction_fire("greeter.say", R_INVALID_ID);
	CHECK_STR(g_log, "{Message}\n");
	Rscript_set_log_callback(NULL, NULL);
	Rwindow_destroy(window);
	Rxml_unregister_widget("Greeter");
}

static void test_api(void) {
	const char* formatted;
	Rid window;
	/* formatting */
	formatted = Rscript_format(TEXT("fn a.b(){let x=1;}"), NULL);
	CHECK_STR(formatted, "fn a.b() {\n    let x = 1;\n}\n");
	formatted = Rscript_format(TEXT("fn a.b(){let x=1;}"), "\t");
	CHECK_STR(formatted, "fn a.b() {\n\tlet x = 1;\n}\n");
	CHECK(Rscript_format(TEXT("fn a.b( {"), NULL) == NULL);
	/* checking text */
	clear();
	CHECK(Rscript_check(TEXT("fn a.b() {\n    let x: number = true;\n}\n"), "c.rvs") == 1);
	CHECK_STR(g_codes, "RS102 ");
	CHECK(!Rscript_has_function("a.b"));
	/* turned off: the scripts are not run, the <Script> elements are still not widgets */
	Rscript_set_enabled(R_FALSE);
	CHECK(!Rscript_is_enabled());
	clear();
	window = window_from("<Window><Script>fn off.f() {}</Script><Button OnClicked=\"off.f\"/></Window>");
	CHECK(window != R_INVALID_ID);
	CHECK(!Rscript_has_function("off.f"));
	CHECK(Rwidget_get_type(Rwindow_get_content(window)) == R_WIDGET_BUTTON);
	Rwindow_destroy(window);
	Rscript_set_enabled(R_TRUE);
	/* a widget made from XML: its initializers find its widgets before it is shown */
	clear();
	{
		static const char piece[] =
			"<VBox>\n"
			"  <Script>\n"
			"    let first: string = ui.getText(\"name\");\n"
			"    fn piece.first(): string {\n"
			"        return first;\n"
			"    }\n"
			"  </Script>\n"
			"  <Label Name=\"name\" Text=\"Ada\"/>\n"
			"</VBox>\n";
		Rid widget = Rwidget_create_from_xml(piece, strlen(piece));
		CHECK(widget != R_INVALID_ID);
		CHECK(g_diagnostic_count == 0);
		CHECK(Rwidget_get_child_count(widget) == 1);
		Rwidget_destroy(widget);
	}
	/* reset: the functions and their actions go */
	CHECK(Rscript_has_function("piece.first"));
	Rscript_reset();
	CHECK(!Rscript_has_function("piece.first"));
	CHECK(!Rscript_has_function("app.increment"));
	/* the diagnostics go to the error callback when no other function takes them */
	Rscript_set_diagnostic_callback(NULL, NULL);
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rscript_load(TEXT("let x: number = \"a\";"), "err.rvs"));
	EXPECT_ERRORS_END(1);
	{
		const char* detail = NULL;
		RSeverity severity;
		CHECK(Rerror_get_last(&detail, &severity) == R_ERROR_SCRIPT);
		CHECK_CONTAINS(detail, "error[RS102]: Type mismatch.");
		CHECK_CONTAINS(detail, "--> err.rvs:1:17");
	}
	Rscript_set_diagnostic_callback(on_diagnostic, NULL);
}

int main(void) {
	if (!test_init(0)) {
		printf("test_script: could not start the library\n");
		return 1;
	}
	Rscript_set_diagnostic_callback(on_diagnostic, NULL);
	RUN(test_counter);
	RUN(test_events);
	RUN(test_handler_signatures);
	RUN(test_compile_errors);
	RUN(test_runtime_errors);
	RUN(test_natives);
	RUN(test_native_bindings);
	RUN(test_files);
	RUN(test_documents);
	RUN(test_components);
	RUN(test_api);
	/* scripts left loaded: Rshutdown frees them (test_report checks that nothing leaks) */
	CHECK(Rscript_load(TEXT("let kept: string[] = [\"a\"];\nfn kept.f() {\n}\n"), "kept.rvs"));
	Rshutdown();
	return test_report("test_script");
}
