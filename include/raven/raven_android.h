/**
 * Raven
 * raven_android.h : running on Android (NativeActivity + android_native_app_glue).
 *
 * The application's android_main gives Raven the android_app before Rinit:
 *
 *   void android_main(struct android_app* app) {
 *       Randroid_set_app(app);
 *       Rinit(R_BACKEND_VULKAN);
 *       ... Rwindow_create ...
 *       while (Rwait_frame(-1.0)) { }
 *       Rshutdown();
 *   }
 *
 * Raven then owns app->onAppCmd and app->onInputEvent. Only one window: the
 * activity's surface. Rwindow_create waits for the surface (APP_CMD_INIT_WINDOW).
 */

#ifndef RAVEN_ANDROID_H
#define RAVEN_ANDROID_H

#include "raven_core.h"

struct android_app;

#ifdef __cplusplus
extern "C" {
#endif

	/** @brief The android_app given to android_main. Call before Rinit. */
	RAVEN_API void Randroid_set_app(struct android_app* app);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_ANDROID_H */
