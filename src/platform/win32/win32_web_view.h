/**
 * Raven - platform/win32/win32_web_view.h
 * The web view of the system on Windows: Microsoft Edge WebView2, for the
 * Win32 platform (rvn_platform_t::web_view_*). The runtime of WebView2 is
 * found and opened at run time; nothing of it is linked.
 */

#ifndef RVN_WIN32_WEB_VIEW_H
#define RVN_WIN32_WEB_VIEW_H

#include "../platform.h"

int   rvn_wv2_supported(void);
rvn_pweb_view_t* rvn_wv2_create(void* user);
void  rvn_wv2_destroy(rvn_pweb_view_t* view);
/** `parent`: the HWND of the window, or NULL to take the view out of it. */
void  rvn_wv2_set_parent_window(rvn_pweb_view_t* view, void* parent);
void  rvn_wv2_set_rect(rvn_pweb_view_t* view, const int rect[4], const int clip[4], float scale, int visible);
void  rvn_wv2_load_url(rvn_pweb_view_t* view, const char* url);
void  rvn_wv2_load_html(rvn_pweb_view_t* view, const char* html, const char* base_url);
void  rvn_wv2_command(rvn_pweb_view_t* view, rvn_pweb_command_t command);
int   rvn_wv2_can(rvn_pweb_view_t* view, rvn_pweb_command_t command);
void  rvn_wv2_run_javascript(rvn_pweb_view_t* view, const char* script, uint64_t request);
void  rvn_wv2_focus(rvn_pweb_view_t* view, int focused);
void* rvn_wv2_native(rvn_pweb_view_t* view);
/** Frees what is left (the environment, the hidden window): rvn_win32_shutdown. */
void  rvn_wv2_shutdown(void);

#endif /* RVN_WIN32_WEB_VIEW_H */
