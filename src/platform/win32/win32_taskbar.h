/**
 * Raven - platform/win32/win32_taskbar.h
 * The taskbar of Windows (win32_taskbar.c): progress, badge, flashing, jump list.
 */

#ifndef RVN_WIN32_TASKBAR_H
#define RVN_WIN32_TASKBAR_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "../platform.h"

/** The message Windows sends when the taskbar button of a window was made: what was asked is given again then. */
UINT rvn_win32_taskbar_created_message(void);
/** state: 0 none, 1 normal, 2 paused, 3 error, 4 indeterminate; value 0..1. */
int  rvn_win32_taskbar_progress(HWND window, int state, float value);
/** The text of the badge (NULL: none); rvn_win32_taskbar_overlay puts it on a button. */
int  rvn_win32_taskbar_set_badge(const char* text);
int  rvn_win32_taskbar_overlay(HWND window);
int  rvn_win32_taskbar_flash(HWND window, int critical);
int  rvn_win32_taskbar_jump_list(const RJumpTask* tasks, int count);
void rvn_win32_taskbar_shutdown(void);

/*
 * Tray icons: in the notification area (Shell_NotifyIcon), with a hidden
 * window of their own that receives their messages. `emit` sends an event
 * of no window to the core, in its turn with the others.
 */
void rvn_win32_tray_set_emit(void (*emit)(rvn_pevent_t* event));
rvn_ptray_t* rvn_win32_tray_create(void* user);
void rvn_win32_tray_destroy(rvn_ptray_t* tray);
void rvn_win32_tray_set_icon(rvn_ptray_t* tray, const uint8_t* rgba, int width, int height);
void rvn_win32_tray_set_tooltip(rvn_ptray_t* tray, const char* text);
void rvn_win32_tray_set_menu(rvn_ptray_t* tray, const rvn_pmenu_t* items, uint32_t count);
void rvn_win32_tray_shutdown(void);

#endif /* RVN_WIN32_TASKBAR_H */
