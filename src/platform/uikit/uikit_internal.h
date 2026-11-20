/**
 * Raven - platform/uikit/uikit_internal.h
 * What the files of the UIKit platform share: uikit_platform.m (windows,
 * events) and uikit_services.m (the services of the device, raven_device.h).
 * Objective-C only.
 */

#ifndef RAVEN_UIKIT_INTERNAL_H
#define RAVEN_UIKIT_INTERNAL_H

#import <UIKit/UIKit.h>

#include "../platform.h"

/* ---- uikit_platform.m ---- */

/** The view controller to present a screen of the system over, for `window` (NULL: Raven's first window): the topmost one shown. nil when there is none. Main thread. */
UIViewController* rvn_uikit_presenter(rvn_pwindow_t* window);
/** The view that shows `window` (NULL: Raven's first window), or nil. Main thread. */
UIView* rvn_uikit_view(rvn_pwindow_t* window);

/* ---- uikit_services.m (entries of rvn_platform_t: see platform.h, docs/device-services.md) ---- */

/** Empties the copies of the last run, fetches the status of notifications, observes the return to the foreground. */
void rvn_uikit_services_init(void);
void rvn_uikit_services_shutdown(void);

int   rvn_uikit_permission_status(int permission);
void  rvn_uikit_permission_request(int permission);
int   rvn_uikit_permission_open_settings(void);
int   rvn_uikit_notify(const char* app, const char* title, const char* body, int urgency);
void  rvn_uikit_file_open(const rvn_service_request_t* request);
void  rvn_uikit_file_save(const rvn_service_request_t* request);
int   rvn_uikit_camera_available(void);
void  rvn_uikit_camera_take_photo(const rvn_service_request_t* request);
void  rvn_uikit_photo_pick(const rvn_service_request_t* request);
void  rvn_uikit_share(const rvn_service_request_t* request);
int   rvn_uikit_open_url(const char* url);
int   rvn_uikit_haptic_supported(void);
int   rvn_uikit_haptic(int haptic);
int   rvn_uikit_download(const char* url, uint8_t** data, size_t* size, char** error);
int   rvn_uikit_clipboard_has_text(void);

#endif /* RAVEN_UIKIT_INTERNAL_H */
