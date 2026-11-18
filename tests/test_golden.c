/**
 * Raven tests - golden images rendered for real (raven_testing.h).
 *
 * A small form is built, driven (typing, a click, a check box), and its
 * frames are compared with the PNG files of tests/goldens/, one per OS and
 * backend (RGoldenOptions.per_platform): form.macos-metal.png...
 *
 * The test renders with the GPU backend of the platform (RAVEN_BACKEND=
 * vulkan or metal picks one), without a window system when it can. It is
 * reported as skipped (exit code 77) without a usable driver, and when no
 * golden of this OS and backend is checked in yet: run it once with
 * RAVEN_UPDATE_GOLDENS=1 to write them, look at them, and add them.
 * What differs is written to the build folder (golden_failures/).
 */

#include "test.h"

#include <stdlib.h>

#define SKIP_CODE 77

#ifndef RAVEN_TEST_GOLDENS
#define RAVEN_TEST_GOLDENS "goldens"
#endif
#ifndef RAVEN_TEST_OUTPUT
#define RAVEN_TEST_OUTPUT "golden_failures"
#endif

static RBackend g_backend = R_BACKEND_DEFAULT;

/* R_FALSE when there is no GPU that can render and be read back. */
static int start(int headless) {
	RInitDesc desc;
	Rid probe;
	uint8_t* pixels;
	int width = 0, height = 0;
	memset(&desc, 0, sizeof(desc));
	desc.backend = g_backend;
	desc.headless = headless;
	desc.app_name = "test_golden";
	Rerror_set_callback(test_error_callback);
	if (!Rinit_ex(&desc)) return 0;
	probe = Rwindow_create("probe", 64, 64, R_WINDOW_DEFAULT);
	pixels = probe != R_INVALID_ID ? Rwindow_capture(probe, &width, &height) : NULL;
	Rpixels_free(pixels);
	if (probe != R_INVALID_ID) Rwindow_destroy(probe);
	if (!pixels || Rtest_get_platform_tag()[0] == 'n') {
		Rshutdown();
		return 0;
	}
	return 1;
}

static int file_exists(const char* path) {
	FILE* file = fopen(path, "rb");
	if (!file) return 0;
	fclose(file);
	return 1;
}

static RGoldenOptions golden_options(void) {
	RGoldenOptions options;
	memset(&options, 0, sizeof(options));
	/* GPUs of the same backend round a little differently: a small tolerance, a few pixels */
	options.channel_tolerance = 8;
	options.max_diff_ratio = 0.002f;
	options.per_platform = R_TRUE;
	options.output_directory = RAVEN_TEST_OUTPUT;
	return options;
}

#define CHECK_GOLDEN(target, name) do { \
	RGoldenOptions test_o_ = golden_options(); \
	RGoldenResult test_r_; \
	g_test_checks++; \
	if (!Rtest_expect_golden((target), RAVEN_TEST_GOLDENS "/" name, &test_o_, &test_r_)) { \
		g_test_failures++; \
		printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, test_r_.message ? test_r_.message : "golden"); \
	} else { \
		printf("  %s\n", test_r_.message); \
	} \
} while (0)

static const char* g_form =
	"<Window Width=\"320\" Height=\"240\">"
	"  <VBox Name=\"form\" Slot.Padding=\"16\" Spacing=\"10\">"
	"    <Label Name=\"title\">Sign in</Label>"
	"    <TextInput Name=\"user\" Placeholder=\"User name\"/>"
	"    <Checkbox Name=\"remember\" Text=\"Remember me\"/>"
	"    <HBox Spacing=\"8\">"
	"      <Button Name=\"cancel\" Text=\"Cancel\"/>"
	"      <Button Name=\"login\" Text=\"Sign in\" Style=\"Primary\" OnClicked=\"form.login\"/>"
	"    </HBox>"
	"    <Label Name=\"status\"></Label>"
	"  </VBox>"
	"</Window>";

static Rid g_window;

static void on_login(Rid sender, const char* action, void* user_data) {
	char text[128];
	(void)sender;
	(void)action;
	(void)user_data;
	snprintf(text, sizeof(text), "Welcome, %s", Rtext_input_get_text(Rtest_find(g_window, "user")));
	Rlabel_set_text(Rtest_find(g_window, "status"), text);
}

static void test_form(void) {
	g_window = Rwindow_create_from_xml(g_form, strlen(g_form));
	CHECK(Rwindow_is_valid(g_window));
	Raction_bind("form.login", on_login, NULL);
	CHECK(Rtest_settle(2.0));
	CHECK_GOLDEN(g_window, "form.png");

	/* typed, checked, clicked: the frame after the animations */
	CHECK(Rtest_type_text(Rtest_find(g_window, "user"), "ada"));
	CHECK(Rtest_click(Rtest_find(g_window, "remember")));
	CHECK(Rtest_click(Rtest_find(g_window, "form/login")));
	/* (the pointer stays away from the buttons: no hover in the picture) */
	CHECK(Rtest_hover_at(g_window, 310.0f, 230.0f));
	CHECK(Rtest_settle(2.0));
	CHECK_STR(Rtest_get_text(Rtest_find(g_window, "status")), "Welcome, ada");
	CHECK(Rtest_is_checked(Rtest_find(g_window, "remember")));
	CHECK_GOLDEN(g_window, "form_filled.png");
	/* one widget alone */
	CHECK_GOLDEN(Rtest_find(g_window, "login"), "form_button.png");

	Raction_unbind("form.login");
	Rwindow_destroy(g_window);
}

int main(void) {
	const char* backend = getenv("RAVEN_BACKEND");
	char golden[512];
	if (backend && strcmp(backend, "metal") == 0) g_backend = R_BACKEND_METAL;
	else if (backend && strcmp(backend, "vulkan") == 0) g_backend = R_BACKEND_VULKAN;
	/* failures to start are expected on machines without a GPU: stay quiet */
	EXPECT_ERRORS_BEGIN();
	if (!start(R_TRUE) && !start(R_FALSE)) {
		printf("test_golden: SKIPPED, no usable GPU backend\n");
		return SKIP_CODE;
	}
	g_test_expect_errors = 0;
	g_test_checks = 0;
	g_test_failures = 0;

	snprintf(golden, sizeof(golden), "%s/form.%s.png", RAVEN_TEST_GOLDENS, Rtest_get_platform_tag());
	if (!file_exists(golden) && !getenv("RAVEN_UPDATE_GOLDENS")) {
		printf("test_golden: SKIPPED, no golden for %s (RAVEN_UPDATE_GOLDENS=1 writes them in %s)\n",
			Rtest_get_platform_tag(), RAVEN_TEST_GOLDENS);
		Rshutdown();
		return SKIP_CODE;
	}
	printf("test_golden: %s\n", Rtest_get_platform_tag());

	/* the same look everywhere, whatever the system prefers; frames that depend only on the test */
	Rlook_set(R_LOOK_RAVEN_DARK);
	Rtest_set_deterministic(R_TRUE);

	RUN(test_form);

	Rshutdown();
	return test_report("test_golden");
}
