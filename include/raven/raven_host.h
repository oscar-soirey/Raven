/**
 * Raven
 * raven_host.h : a window system supplied by the application.
 *
 * With the host platform, Raven opens no OS window and reads no OS event
 * queue. Every window is a surface of the application (for example a panel
 * of an editor that has windows of its own): the application tells Raven
 * its size, scale, focus and input, and Raven asks the application for the
 * cursor, the clipboard and to be woken up.
 *
 * Call Rhost_platform_register before Rinit / Rinit_ex. Positions are in
 * pixels of the window, origin top-left, as with a native window. Every
 * function must be called from the UI thread, except Rhost_permission_answer
 * and Rhost_service_complete.
 *
 * The services of the device (raven_device.h) go to the application too,
 * when it gives the functions for them: a phone application that embeds
 * Raven in its own views, or a test that plays the user. Raven asks with a
 * callback of RHostPlatformDesc and the application answers later (or at
 * once) with Rhost_permission_answer or Rhost_service_complete; Raven hands
 * the answer to the requester at its next frame. A service without its
 * callback is unsupported, except the file dialogs: they are then the ones
 * of Raven (Rfile_dialog_open).
 */

#ifndef RAVEN_HOST_H
#define RAVEN_HOST_H

#include "raven_core.h"
#include "raven_window.h"
#include "raven_device.h"

/** What a request of a service of the device asks the application for (RHostServiceRequest). */
typedef enum RHostService{
	/** Files to read (Rfile_open_async): `filters`, `flags` (R_FILE_DIALOG_MULTIPLE). */
	R_HOST_SERVICE_OPEN_FILES = 0x0C40,
	/** Save `data` / `size`, or the file `path` when data is NULL, as `file_name` (Rfile_save_async, Rfile_export_async). */
	R_HOST_SERVICE_SAVE_FILE,
	/** A folder (Rfile_select_folder_async). */
	R_HOST_SERVICE_SELECT_FOLDER,
	/** A photo from the camera (Rcamera_take_photo); camera permission already asked. */
	R_HOST_SERVICE_TAKE_PHOTO,
	/** Up to `max_count` photos (0: no limit) from the library (Rphoto_pick). */
	R_HOST_SERVICE_PICK_PHOTOS,
	/** The share sheet: `text` and / or the file `path` (`mime_type`), pointing at `anchor_*` (Rshare_show). */
	R_HOST_SERVICE_SHARE
}RHostService;

/** A request of a service of the device, valid during the call only. Fields that do not concern the service are zero. */
typedef struct RHostServiceRequest {
	/** Given back to Rhost_service_complete. */
	Ruint id;
	RHostService service;
	/** The window it is for (R_INVALID_ID: none). */
	Rid window;
	/** What it points at (a share sheet on a tablet), in pixels of the window; width 0: the middle of the window. */
	float anchor_x, anchor_y, anchor_width, anchor_height;
	const char* title;
	const RFileFilter* filters;
	int filter_count;
	/** Bitmask of RFileDialogFlags. */
	Ruint flags;
	const char* file_name;
	const char* default_extension;
	const void* data;
	size_t size;
	const char* path;
	int max_count;
	const char* text;
	const char* mime_type;
	const char* share_title;
} RHostServiceRequest;

typedef struct RHostPlatformDesc {
	/** The pointer over `window` should look like `cursor`. May be NULL. */
	void (*set_cursor)(Rid window, RCursor cursor, void* user_data);
	/** Copies UTF-8 text to the clipboard of the application. NULL: the clipboard stays in the process. */
	void (*clipboard_set)(const char* text, void* user_data);
	/** The clipboard text, valid until the next call, or NULL. NULL: the clipboard stays in the process. */
	const char* (*clipboard_get)(void* user_data);
	/** Raven has work to do (a timer, a posted task, a redraw): produce a frame soon. Thread-safe. May be NULL. */
	void (*wake)(void* user_data);
	void* user_data;

	/* ---- services of the device (raven_device.h); each may be NULL: unsupported ---- */

	/** An RPermissionStatus, without asking. NULL: as on desktops (notifications granted, the rest unsupported). */
	RPermissionStatus (*permission_status)(RPermission permission, void* user_data);
	/** Ask the user (the status is not asked or denied); answer with Rhost_permission_answer. */
	void (*permission_request)(RPermission permission, void* user_data);
	/** A picker, the camera, a save or a share sheet; answer with Rhost_service_complete. NULL: the file dialogs of Raven, nothing else. */
	void (*service_request)(const RHostServiceRequest* request, void* user_data);
	/** R_TRUE: there is a camera (Rcamera_is_available) for R_HOST_SERVICE_TAKE_PHOTO. */
	int has_camera;
	/** Ropen_url. Returns R_FALSE when it could not. NULL: as on desktops (`open`, `xdg-open`...). */
	int (*open_url)(const char* url, void* user_data);
	/** Rhaptic_play: an RHaptic. Returns R_TRUE when played. */
	int (*haptic)(RHaptic haptic, void* user_data);
	/**
	 * Downloads an http(s) URL (images from a URL): returns its content in a
	 * buffer from Rmemory_alloc and its size, or NULL. Called on worker
	 * threads, several at a time, and may block. NULL: as on desktops.
	 */
	void* (*download)(const char* url, size_t* size, void* user_data);
} RHostPlatformDesc;

#ifdef __cplusplus
extern "C" {
#endif

	/** @brief Makes the next Rinit use the host platform. The structure is copied; NULL restores the default. */
	RAVEN_API void Rhost_platform_register(const RHostPlatformDesc* desc);
	/** @return R_TRUE while the running library uses the host platform. */
	RAVEN_API int Rhost_platform_is_active(void);

	/** @brief Size of a host window in pixels, and pixels per logical unit. */
	RAVEN_API void Rhost_window_set_size(Rid window, int width, int height, float scale);
	/**
	 * @brief What covers a host window on a phone or a tablet, in pixels: the
	 * safe area (status bar, notch, home indicator; NULL: none) and the height
	 * of the on-screen keyboard over its bottom (0: hidden). The content is laid
	 * out inside them (Rwindow_get_safe_area, Rwindow_get_keyboard_area).
	 */
	RAVEN_API void Rhost_window_set_insets(Rid window, const RInsets* safe_area, int keyboard_height);
	/**
	 * @brief The application's system is in dark (R_TRUE) or light (R_FALSE)
	 * mode, -1 when it does not tell (the default): Rtheme_get_system_dark,
	 * and Rtheme_follow_system follows it.
	 */
	RAVEN_API void Rhost_set_system_dark(int dark);
	RAVEN_API void Rhost_inject_mouse_move(Rid window, int x, int y, Ruint modifiers);
	RAVEN_API void Rhost_inject_mouse_button(Rid window, int x, int y, RMouseButton button, int pressed, Ruint modifiers);
	RAVEN_API void Rhost_inject_mouse_wheel(Rid window, float wheel_x, float wheel_y, Ruint modifiers);
	/** @brief The pointer left the window: hover states end. */
	RAVEN_API void Rhost_inject_mouse_leave(Rid window);
	/** @param repeat R_TRUE for a key held down (auto-repeat). */
	RAVEN_API void Rhost_inject_key(Rid window, RKey key, int pressed, Ruint modifiers, int repeat);
	/** @brief One typed Unicode code point. */
	RAVEN_API void Rhost_inject_text(Rid window, uint32_t codepoint);
	/**
	 * @brief The input method of the application's system composes `text`
	 * (preedit), with its selection in bytes (see Rwindow_inject_composition).
	 * NULL or "" cancels it; the committed text then comes through
	 * Rhost_inject_text. Rwindow_get_text_input_rect tells where to put the
	 * candidate window.
	 */
	RAVEN_API void Rhost_inject_composition(Rid window, const char* text, int selection_start, int selection_end);
	/** @brief The window gained or lost the keyboard. Losing it closes popups and drops captures. */
	RAVEN_API void Rhost_inject_focus(Rid window, int focused);
	/** @brief The user asked to close the window (its close callback decides). */
	RAVEN_API void Rhost_inject_close(Rid window);

	/**
	 * @brief The user answered a permission_request (or the status changed
	 * by itself). The waiting Rpermission_request callbacks get it at the
	 * next frame. Any thread.
	 */
	RAVEN_API void Rhost_permission_answer(RPermission permission, RPermissionStatus status);
	/**
	 * @brief The end of a service_request: the files it gave (copied; paths
	 * Raven can read; size 0 is read from the file), at the next frame. Once
	 * per request. Any thread.
	 */
	RAVEN_API void Rhost_service_complete(Ruint request, RDeviceResult result, const RPickedFile* files, int count);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_HOST_H */
