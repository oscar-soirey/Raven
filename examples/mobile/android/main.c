/*
 * Raven on Android (NativeActivity; Raven's only Java is its text view, see build.sh): the entry and the loop.
 * Everything visible comes from ../shared (showcase.c / showcase.xml).
 */
#include "showcase.h"
#include <raven/raven_android.h>
#include <android_native_app_glue.h>
#include <android/log.h>

#define TAG "RavenHello"

static struct android_app* g_app;

static void on_error(RError code, RSeverity severity, const char* detail) {
	__android_log_print(ANDROID_LOG_ERROR, TAG, "Raven error %d severity %d: %s", (int)code, (int)severity, detail ? detail : "");
}

/* back with nothing left to go back to: leave, as the system would have */
static void on_exit_request(void) {
	ANativeActivity_finish(g_app->activity);
}

void android_main(struct android_app* app) {
	RInitDesc desc = { 0 };

	g_app = app;
	__android_log_print(ANDROID_LOG_INFO, TAG, "android_main");
	Rerror_set_callback(on_error);
	Randroid_set_app(app);
	desc.backend = R_BACKEND_VULKAN;
	desc.app_name = "RavenShowcase";
	if (!Rinit_ex(&desc)) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Rinit_ex failed");
		return;
	}
	/* dark or light with the system, now and when it changes */
	Rtheme_follow_system(R_TRUE);
	showcase_set_exit_handler(on_exit_request);
	if (showcase_create() == R_INVALID_ID) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "showcase_create failed");
	}
	showcase_report("launch");
	while (Rwait_frame(-1.0)) {
	}
	__android_log_print(ANDROID_LOG_INFO, TAG, "loop ended");
	Rshutdown();
}
