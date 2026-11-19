/**
 * Raven - platform/apple/webkit_web_view.h
 * The web view of the system on macOS and iOS: a WKWebView, for the Cocoa
 * and UIKit platforms (rvn_platform_t::web_view_*). WebKit.framework is
 * opened at run time, when the first view is made.
 */

#ifndef RVN_WEBKIT_WEB_VIEW_H
#define RVN_WEBKIT_WEB_VIEW_H

#include "../platform.h"

int   rvn_webkit_supported(void);
rvn_pweb_view_t* rvn_webkit_create(void* user);
void  rvn_webkit_destroy(rvn_pweb_view_t* view);
/** `parent_view`: the NSView* / UIView* of the window, or NULL to take the view out of it. */
void  rvn_webkit_set_parent_view(rvn_pweb_view_t* view, void* parent_view);
void  rvn_webkit_set_rect(rvn_pweb_view_t* view, const int rect[4], const int clip[4], float scale, int visible);
void  rvn_webkit_load_url(rvn_pweb_view_t* view, const char* url);
void  rvn_webkit_load_html(rvn_pweb_view_t* view, const char* html, const char* base_url);
void  rvn_webkit_command(rvn_pweb_view_t* view, rvn_pweb_command_t command);
int   rvn_webkit_can(rvn_pweb_view_t* view, rvn_pweb_command_t command);
void  rvn_webkit_run_javascript(rvn_pweb_view_t* view, const char* script, uint64_t request);
void  rvn_webkit_focus(rvn_pweb_view_t* view, int focused);
void* rvn_webkit_native(rvn_pweb_view_t* view);

#endif /* RVN_WEBKIT_WEB_VIEW_H */
