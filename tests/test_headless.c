/**
 * Raven tests - the tree as JSON and the scripts of steps (raven_testing.h:
 * Rtest_dump_tree, Rtest_save_tree, Rtest_run_script), in the process.
 *
 * The same scripts run in any application with RAVEN_SCRIPT: that path
 * (the environment, the exit codes, the pictures) is tested by
 * headless_cli.cmake with the program headless_app.c.
 *
 * Headless with the null backend: no display and no GPU.
 */

#include "test.h"

#include <stdlib.h>

#ifndef RAVEN_TEST_OUTPUT
#define RAVEN_TEST_OUTPUT "test_headless_output"
#endif

static const char* g_form =
	"<Window Title=\"Sign in\" Width=\"340\" Height=\"260\">"
	"  <VBox Name=\"form\" Slot.Padding=\"20\" Spacing=\"10\">"
	"    <Label Name=\"title\">Sign in to Raven</Label>"
	"    <TextInput Name=\"user\" Placeholder=\"User name\"/>"
	"    <Checkbox Name=\"remember\" Text=\"Remember me\"/>"
	"    <HBox Spacing=\"8\">"
	"      <Button Name=\"cancel\" Text=\"Cancel\" Enabled=\"false\"/>"
	"      <Button Name=\"submit\" Text=\"Sign in\" OnClicked=\"form.submit\"/>"
	"    </HBox>"
	"    <Label Name=\"status\"/>"
	"    <Label Name=\"secret\" Visibility=\"Collapsed\">hidden</Label>"
	"    <Label Name=\"note\">say \"hi\"\tthere</Label>"
	"  </VBox>"
	"</Window>";

static Rid g_window;
static int g_submits;

static void on_submit(Rid sender, const char* action, void* user_data) {
	char text[128];
	(void)sender;
	(void)action;
	(void)user_data;
	g_submits++;
	snprintf(text, sizeof(text), "Welcome, %s", Rtext_input_get_text(Rtest_find(g_window, "user")));
	Rlabel_set_text(Rtest_find(g_window, "status"), text);
}

static int contains(const char* text, const char* part) {
	return text && strstr(text, part) != NULL;
}

/* The braces and brackets of a JSON text are balanced, outside of its strings. */
static int balanced(const char* json) {
	int depth = 0, in_string = 0;
	for (; *json; json++) {
		if (in_string) {
			if (*json == '\\' && json[1]) json++;
			else if (*json == '"') in_string = 0;
		} else if (*json == '"') {
			in_string = 1;
		} else if (*json == '{' || *json == '[') {
			depth++;
		} else if (*json == '}' || *json == ']') {
			if (--depth < 0) return 0;
		}
	}
	return depth == 0 && !in_string;
}

static void test_tree(void) {
	const char* json = Rtest_dump_tree(R_INVALID_ID);
	printf("tree\n");
	CHECK(balanced(json));
	CHECK(contains(json, "\"raven\": \""));
	CHECK(contains(json, "\"backend\": \"null\""));
	CHECK(contains(json, "\"title\": \"Sign in\""));
	CHECK(contains(json, "\"size\": [340, 260]"));
	/* names, paths that Rtest_find takes, roles, texts */
	CHECK(contains(json, "\"name\": \"user\""));
	CHECK(contains(json, "\"path\": \"form/user\""));
	CHECK(contains(json, "\"role\": \"text field\""));
	CHECK(contains(json, "\"text\": \"Sign in to Raven\""));
	CHECK(contains(json, "\"path\": \"form/remember\""));
	CHECK(contains(json, "\"checked\": false"));
	/* bounds in logical units: the padding of the form */
	CHECK(contains(json, "\"bounds\": [20, 20, 300,"));
	/* the disabled button, the collapsed label, its text escaped */
	CHECK(contains(json, "\"enabled\": false"));
	CHECK(contains(json, "\"visibility\": \"collapsed\""));
	CHECK(contains(json, "\"text\": \"say \\\"hi\\\"\\tthere\""));

	/* one widget alone */
	json = Rtest_dump_tree(Rtest_find(g_window, "form/submit"));
	CHECK(balanced(json));
	CHECK(json[0] == '{');
	CHECK(contains(json, "\"type\": \"Button\""));
	CHECK(contains(json, "\"text\": \"Sign in\""));
	CHECK(!contains(json, "\"windows\""));

	/* one window */
	json = Rtest_dump_tree(g_window);
	CHECK(contains(json, "\"windows\""));

	/* to a file */
	CHECK(Rtest_save_tree(R_INVALID_ID, RAVEN_TEST_OUTPUT "/tree/form.json"));
	{
		FILE* file = fopen(RAVEN_TEST_OUTPUT "/tree/form.json", "rb");
		char buffer[256];
		size_t read = file ? fread(buffer, 1, sizeof(buffer) - 1, file) : 0;
		CHECK(file != NULL);
		if (file) fclose(file);
		buffer[read] = '\0';
		CHECK(contains(buffer, "\"windows\""));
	}

	EXPECT_ERRORS_BEGIN();
	CHECK_STR(Rtest_dump_tree((Rid)12345), "");
	CHECK(!Rtest_save_tree(R_INVALID_ID, NULL));
	EXPECT_ERRORS_END(2);
}

static void test_script_passes(void) {
	const char* script =
		"# a user signs in\n"
		"expect status text \"\"\n"
		"click form/user\n"
		"type \"ada\"\n"
		"expect user focused\n"
		"click \"text:Remember me\"   # found by what it reads\n"
		"expect remember checked\n"
		"key user Ctrl+A\n"
		"type user \"grace\"\n"
		"click form/submit\n"
		"expect status text \"Welcome, grace\"\n"
		"expect status contains grace\n"
		"expect cancel disabled\n"
		"expect secret hidden\n"
		"expect nothing missing\n"
		"wait 3\n"
		"wait 0.05s\n"
		"settle 1\n"
		"mark signed-in\n"
		"tree " RAVEN_TEST_OUTPUT "/tree/after\n";
	printf("a script that passes\n");
	g_submits = 0;
	CHECK(Rtest_run_script(script, "signin.script"));
	CHECK_STR(Rtest_get_script_error(), "");
	CHECK_STR(Rtest_get_text(Rtest_find(g_window, "status")), "Welcome, grace");
	CHECK(Rtest_is_checked(Rtest_find(g_window, "remember")));
	CHECK(g_submits == 1);
	{
		FILE* file = fopen(RAVEN_TEST_OUTPUT "/tree/after.json", "rb");
		CHECK(file != NULL);
		if (file) fclose(file);
	}

	/* a point of the window: the middle of the check box, clicked again */
	{
		float x, y, width, height;
		char line[128];
		Rwidget_get_rect(Rtest_find(g_window, "remember"), &x, &y, &width, &height);
		snprintf(line, sizeof(line), "click @%d,%d\nexpect remember unchecked\n", (int)(x + 8.0f), (int)(y + height * 0.5f));
		CHECK(Rtest_run_script(line, NULL));
	}
}

static void test_script_fails(void) {
	printf("scripts that fail\n");
	/* a check that stays false: the line and what was read */
	CHECK(!Rtest_run_script("timeout 0.1\nclick form/submit\nexpect status text \"Bye\"\n", "bad.script"));
	CHECK(contains(Rtest_get_script_error(), "bad.script:3: expect"));
	CHECK(contains(Rtest_get_script_error(), "\"Bye\""));
	/* a widget that never comes */
	CHECK(!Rtest_run_script("timeout 3f\nclick form/nothing\n", "missing.script"));
	CHECK(contains(Rtest_get_script_error(), "missing.script:2: click"));
	CHECK(contains(Rtest_get_script_error(), "form/nothing"));
	/* mistakes found before anything runs */
	g_submits = 0;
	CHECK(!Rtest_run_script("click form/submit\nclack form/submit\n", "typo.script"));
	CHECK(contains(Rtest_get_script_error(), "line 2: unknown command \"clack\""));
	CHECK(g_submits == 0);
	CHECK(!Rtest_run_script("wait soon\n", NULL));
	CHECK(contains(Rtest_get_script_error(), "is not a number"));
	CHECK(!Rtest_run_script("type \"open\n", NULL));
	CHECK(contains(Rtest_get_script_error(), "quote"));
	CHECK(!Rtest_run_script("click\n", NULL));
	CHECK(!Rtest_run_script("key Ctrl+Nothing\n", NULL));
	CHECK(contains(Rtest_get_script_error(), "is not a key"));
	CHECK(!Rtest_run_script("expect status bright\n", NULL));
	CHECK(contains(Rtest_get_script_error(), "unknown state"));
	/* an empty script passes */
	CHECK(Rtest_run_script("# nothing\n\n", NULL));
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	Raction_bind("form.submit", on_submit, NULL);
	g_window = Rwindow_create_from_xml(g_form, strlen(g_form));
	CHECK(Rwindow_is_valid(g_window));
	Rtest_set_deterministic(R_TRUE);
	Rtest_settle(2.0);

	test_tree();
	test_script_passes();
	test_script_fails();

	Rshutdown();
	printf("%d checks, %d failures\n", g_test_checks, g_test_failures);
	return g_test_failures ? 1 : 0;
}
