/*
 * Raven mobile showcase: the UI shared by the iOS and the Android app.
 *
 * The app calls, on the UI thread:
 *   Rinit / Rinit_ex, Rtheme_follow_system(R_TRUE), then showcase_create().
 * Everything visible comes from here (showcase.c + showcase.xml).
 */
#ifndef SHOWCASE_H
#define SHOWCASE_H

#include <Raven.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Builds the window and the whole UI. Returns the window, or R_INVALID_ID. */
Rid showcase_create(void);

/* Debug: logs the window size, safe area and theme (after a launch or rotation). */
void showcase_report(const char* why);

/* Logging that reaches the console on iOS (stderr) and logcat on Android. */
void showcase_log(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif
