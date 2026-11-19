/**
 * Raven example - ui_test
 * A test of an interface (raven_testing.h): the window of login.xml is
 * driven by the names of its widgets, as a user would, then what it shows
 * is checked, as text and as a picture compared with a golden image.
 *
 *     ./raven_ui_test                          runs the test (exit code 0: passed)
 *     RAVEN_UPDATE_GOLDENS=1 ./raven_ui_test   writes the golden images again
 *
 * The goldens are in goldens/, one per OS and backend (login.macos-metal.png):
 * what the GPU draws differs a little between backends. Without a GPU, or
 * without a golden for this OS and backend, the picture is not compared.
 * A test program of an application is just this: a main that returns 0.
 */

#include <Raven.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef RAVEN_EXAMPLE_DIR
#define RAVEN_EXAMPLE_DIR "."
#endif

static Rid g_window;
static int g_failures;

#define EXPECT(condition) do { \
	if (!(condition)) { \
		g_failures++; \
		fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
	} \
} while (0)

static void on_error(RError code, RSeverity severity, const char* detail) {
	g_failures++;
	fprintf(stderr, "[Raven] %s (%s): %s\n", Rerror_to_string(code), Rseverity_to_string(severity), detail);
}

/* The application: what the buttons do. */
static void on_submit(Rid sender, const char* action, void* user_data) {
	const char* user = Rtext_input_get_text(Rwidget_find(g_window, "user"));
	char text[128];
	(void)sender;
	(void)action;
	(void)user_data;
	snprintf(text, sizeof(text), user[0] ? "Welcome, %s" : "Type a user name", user);
	Rlabel_set_text(Rwidget_find(g_window, "status"), text);
}

int main(void) {
	RInitDesc desc;
	RGoldenOptions options;
	RGoldenResult result;
	char golden[512];
	FILE* file;

	/* no window on the screen: the frames are rendered offscreen and read back */
	memset(&desc, 0, sizeof(desc));
	desc.backend = getenv("RAVEN_BACKEND") && strcmp(getenv("RAVEN_BACKEND"), "null") == 0 ? R_BACKEND_NULL : R_BACKEND_DEFAULT;
	desc.headless = R_TRUE;
	if (!Rinit_ex(&desc)) return 77;
	/* (an error of the library from here on fails the test) */
	Rerror_set_callback(on_error);
	Raction_bind("login.submit", on_submit, NULL);

	/* the same frames on every run: a fixed look, a fixed clock, carets that do not blink */
	Rlook_set(R_LOOK_RAVEN_DARK);
	Rtest_set_deterministic(R_TRUE);

	g_window = Rwindow_create_from_xml_file(RAVEN_EXAMPLE_DIR "/login.xml");
	EXPECT(Rwindow_is_valid(g_window));
	Rtest_settle(2.0);

	/* nothing typed: the form says so */
	EXPECT(Rtest_click(Rtest_find(g_window, "form/submit")));
	EXPECT(strcmp(Rtest_get_text(Rtest_find(g_window, "status")), "Type a user name") == 0);

	/* a user types a name and a password, then signs in */
	EXPECT(Rtest_type_text(Rtest_find(g_window, "user"), "ada"));
	EXPECT(Rtest_type_text(Rtest_find(g_window, "password"), "secret\n"));
	EXPECT(Rtest_click(Rtest_find(g_window, "form/submit")));
	EXPECT(Rtest_hover_at(g_window, 0.0f, 0.0f));
	EXPECT(Rtest_settle(2.0));
	EXPECT(strcmp(Rtest_get_text(Rtest_find(g_window, "user")), "ada") == 0);
	EXPECT(strcmp(Rtest_get_text(Rtest_find(g_window, "status")), "Welcome, ada") == 0);

	/* and the window looks as it should */
	memset(&options, 0, sizeof(options));
	options.channel_tolerance = 8;
	options.max_diff_ratio = 0.002f;
	options.per_platform = R_TRUE;
	snprintf(golden, sizeof(golden), "%s/goldens/login.%s.png", RAVEN_EXAMPLE_DIR, Rtest_get_platform_tag());
	file = fopen(golden, "rb");
	if (file) fclose(file);
	if (file || getenv("RAVEN_UPDATE_GOLDENS")) {
		EXPECT(Rtest_expect_golden(g_window, RAVEN_EXAMPLE_DIR "/goldens/login.png", &options, &result));
		printf("%s\n", result.message);
	} else {
		printf("no golden for %s: the picture is not compared\n", Rtest_get_platform_tag());
	}

	Rshutdown();
	printf("ui_test: %s\n", g_failures ? "FAILED" : "passed");
	return g_failures ? 1 : 0;
}
