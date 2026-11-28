/* Starts Raven headless (no window system, no GPU), builds a window from XML, reads it back. */
#include <Raven.h>

#include <stdio.h>
#include <string.h>

int main(void) {
	static const char xml[] = "<Window><Label Name=\"hello\" Text=\"Hello\"/></Window>";
	RInitDesc desc;
	Rid window;
	int ok;

	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_NULL;
	desc.headless = R_TRUE;
	desc.worker_threads = -1;
	if (!Rinit_ex(&desc)) return 1;
	window = Rwindow_create_from_xml(xml, strlen(xml));
	Rwait_frame(0.0);
	ok = strcmp(Rlabel_get_text(Rwidget_find(window, "hello")), "Hello") == 0;
	printf("Raven %s %s\n", Rget_version(), ok ? "OK" : "FAILED");
	Rwindow_destroy(window);
	Rshutdown();
	return ok ? 0 : 1;
}
