/**
 * Raven - platform/sni.h
 * Tray icons of Linux and the BSDs (platform/sni.c): StatusNotifierItem with
 * its menu over com.canonical.dbusmenu, used by the X11 and Wayland platforms.
 */

#ifndef RVN_SNI_H
#define RVN_SNI_H

#include "platform.h"

/** Where the clicks and the chosen entries go (the handler of the platform). */
void rvn_sni_set_handler(rvn_pevent_handler_t handler);
/** The entries of rvn_platform_t for tray icons. */
int  rvn_sni_supported(void);
rvn_ptray_t* rvn_sni_create(void* user);
void rvn_sni_destroy(rvn_ptray_t* tray);
void rvn_sni_set_icon(rvn_ptray_t* tray, const uint8_t* rgba, int width, int height);
void rvn_sni_set_tooltip(rvn_ptray_t* tray, const char* text);
void rvn_sni_set_menu(rvn_ptray_t* tray, const rvn_pmenu_t* items, uint32_t count);
/** The socket to wait on with the window system (-1 when no icon was made). */
int  rvn_sni_fd(void);
/** Answers the calls of the panel that came in. */
void rvn_sni_dispatch(void);
void rvn_sni_shutdown(void);

#endif
