/*
 * A program that uses an installed Raven, as a user's program does: built
 * from outside the tree of Raven, with pkg-config (package_consumer.cmake),
 * find_package(raven) (CMakeLists.txt beside it), or by a package manager
 * (the test of the Homebrew formula, the vcpkg port, the Conan recipe).
 *
 * It runs headless (null platform, null backend): no display and no GPU.
 * Exit code 0 when the library starts, builds a window from XML and gives
 * back what it was given.
 */
#include <Raven.h>

#include <stdio.h>
#include <string.h>

static const char* g_xml =
	"<Window Title=\"Package\" Width=\"200\" Height=\"100\">"
	"  <Label Name=\"hello\" Text=\"Hello from an installed Raven\"/>"
	"</Window>";

int main(void) {
	RInitDesc desc;
	Rid window;
	const char* text;
	char version[32];
	int ok = 1;

	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_NULL;
	desc.headless = R_TRUE;
	desc.worker_threads = -1;
	if (!Rinit_ex(&desc)) {
		printf("consumer: Rinit_ex failed\n");
		return 1;
	}

	/* the headers and the library are of the same version */
	snprintf(version, sizeof(version), "%d.%d.%d", RAVEN_VERSION_MAJOR, RAVEN_VERSION_MINOR, RAVEN_VERSION_PATCH);
	if (strcmp(version, Rget_version()) != 0) {
		printf("consumer: headers %s, library %s\n", version, Rget_version());
		ok = 0;
	}

	window = Rwindow_create_from_xml(g_xml, strlen(g_xml));
	Rwait_frame(0.0);
	text = Rlabel_get_text(Rwidget_find(window, "hello"));
	if (!text || strcmp(text, "Hello from an installed Raven") != 0) {
		printf("consumer: the label reads \"%s\"\n", text ? text : "(null)");
		ok = 0;
	}
	Rwindow_destroy(window);
	Rshutdown();

	printf("consumer: Raven %s %s\n", Rget_version(), ok ? "OK" : "FAILED");
	return ok ? 0 : 1;
}
