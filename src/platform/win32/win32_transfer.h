/**
 * Raven - platform/win32/win32_transfer.h
 * Data for the other applications on Windows (win32_transfer.c): the formats
 * of the clipboard and the drags out of the windows (OLE).
 */

#ifndef RVN_WIN32_TRANSFER_H
#define RVN_WIN32_TRANSFER_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "../platform.h"

int   rvn_win32_clipboard_set_data(const RDataItem* items, int count);
int   rvn_win32_clipboard_has_data(const char* mime);
char* rvn_win32_clipboard_get_data(const char* mime, size_t* size);
/**
 * Runs a drag out of `window` (DoDragDrop: returns when it ended). R_FALSE
 * when it could not start (OLE could not be initialized on this thread);
 * else `*operation` receives the RDragOperation done.
 */
int   rvn_win32_drag(HWND window, const RDataItem* items, int count, uint32_t operations,
	const uint8_t* rgba, int width, int height, int* operation);
void  rvn_win32_transfer_shutdown(void);

#endif /* RVN_WIN32_TRANSFER_H */
