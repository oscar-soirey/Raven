/**
 * Raven
 * raven_dialogs.h : dialogs, message boxes, file dialogs and the file browser.
 *
 * DIALOGS
 *
 * A dialog is any widget shown as a sheet over the content of a window: a
 * veil falls on the window, the sheet rises in the middle, and the window
 * only listens to the sheet until it is closed. Rdialog_open returns at
 * once; Rdialog_run waits for the dialog to close and returns its result.
 *
 * FILE DIALOGS
 *
 * Rfile_dialog_open / _save / _select_folder ask the user for a path and
 * return when it is chosen. They show the dialog of the operating system
 * (Windows: the Explorer dialog; Linux: the one of the desktop, through
 * zenity or kdialog) and, where there is none, the file browser of Raven in a
 * dialog. R_FILE_DIALOG_RAVEN always uses the latter. They do not exist on
 * iOS and Android (they return NULL): Rfile_open_async and the other
 * asynchronous forms of raven_device.h work everywhere.
 *
 * The file browser is also a widget, R_WIDGET_FILE_BROWSER, to put in any
 * layout: a side panel showing a project folder, a custom "open" page...
 *
 *     <FileBrowser Name="files" Directory="~/Projects" Filter="*.xml;*.c" Controls="false"/>
 *
 * The functions that wait (Rdialog_run, Rmessage_box, Rfile_dialog_...) run
 * the frames of the application themselves while they wait: timers, tasks
 * and the frame callback keep running, the windows keep being drawn.
 */

#ifndef RAVEN_DIALOGS_H
#define RAVEN_DIALOGS_H

#include "raven_widget.h"

/** How much a notification of the system asks for attention (Rnotification_show). */
typedef enum RNotificationUrgency{
	/** No sound; may only go to the list of notifications. */
	R_NOTIFICATION_LOW = 0x04F0,
	R_NOTIFICATION_NORMAL,
	/** Stays until the user dismisses it, where the system allows it. */
	R_NOTIFICATION_CRITICAL
}RNotificationUrgency;

/** What happened to a notification (Rnotification_set_callback). */
typedef enum RNotificationEvent{
	/** The user clicked the notification itself (not one of its buttons). */
	R_NOTIFICATION_ACTIVATED = 0x0C20,
	/** The user clicked one of its buttons: `action` is the id of that button. */
	R_NOTIFICATION_ACTION_INVOKED,
	/** It went away without being clicked: closed by the user, or its time ran out (where the system tells). */
	R_NOTIFICATION_DISMISSED
}RNotificationEvent;

/** What the notifications of the system can do here (Rnotification_supports). */
typedef enum RNotificationFeature{
	/** Buttons (RNotificationDesc::actions). */
	R_NOTIFICATION_FEATURE_ACTIONS   = (1 << 0),
	/** A notification shown again with the same tag replaces the first one. */
	R_NOTIFICATION_FEATURE_REPLACE   = (1 << 1),
	/** Rnotification_close takes a notification away. */
	R_NOTIFICATION_FEATURE_CLOSE     = (1 << 2),
	/** An image beside the text (RNotificationDesc::icon). */
	R_NOTIFICATION_FEATURE_ICON      = (1 << 3),
	/** A click on the notification is reported (R_NOTIFICATION_ACTIVATED). */
	R_NOTIFICATION_FEATURE_ACTIVATED = (1 << 4),
	/** Its dismissal is reported (R_NOTIFICATION_DISMISSED). */
	R_NOTIFICATION_FEATURE_DISMISSED = (1 << 5)
}RNotificationFeature;

/** A button of a notification. */
typedef struct RNotificationAction {
	/** Given back to the callback ("reply", "snooze"); not empty, and not "default". */
	const char* id;
	/** Shown to the user ("Reply"). */
	const char* label;
} RNotificationAction;

/**
 * A notification for Rnotification_show_desc. `size` must be
 * sizeof(RNotificationDesc): the fields that later versions of Raven add
 * come after the others, and a smaller size tells Raven they were not
 * given. The other fields may be left to zero.
 */
typedef struct RNotificationDesc {
	/** sizeof(RNotificationDesc). */
	Ruint size;
	/**
	 * Names the notification: shown again with the same tag, it replaces the
	 * one still shown instead of adding another; Rnotification_close takes it
	 * away; the callback is given it. NULL or "": a tag of its own
	 * ("raven.1", "raven.2"...).
	 */
	const char* tag;
	const char* title;
	const char* body;
	/** 0: R_NOTIFICATION_NORMAL. */
	RNotificationUrgency urgency;
	/** An image file shown beside the text (PNG, JPEG, SVG...: what Raven reads), or NULL. */
	const char* icon;
	/** Up to 3 buttons, or NULL. */
	const RNotificationAction* actions;
	int action_count;
	/** The window brought back when the notification is clicked. R_INVALID_ID: the main window (the first one opened). */
	Rid window;
} RNotificationDesc;

/** A kind of file in a file dialog. */
typedef struct RFileFilter {
	/** Shown to the user: "Images". */
	const char* name;
	/** Patterns separated by ';': "*.png;*.jpg". "*" matches everything. */
	const char* patterns;
} RFileFilter;

typedef enum RFileDialogFlags{
	/** Several files can be chosen (Rfile_dialog_open): see Rfile_dialog_get_result. */
	R_FILE_DIALOG_MULTIPLE    = (1 << 0),
	/** Never the dialog of the OS: the file browser of Raven, in a dialog of the parent window. */
	R_FILE_DIALOG_RAVEN       = (1 << 1),
	R_FILE_DIALOG_SHOW_HIDDEN = (1 << 2)
}RFileDialogFlags;

/** Every field may be left to zero. */
typedef struct RFileDialogDesc {
	/** Window the dialog belongs to. R_INVALID_ID: none (a window is opened when Raven needs one). */
	Rid parent;
	/** NULL: "Open", "Save as"... */
	const char* title;
	/** Folder shown first. NULL: the one of the previous dialog, else the home of the user. */
	const char* directory;
	/** Name proposed to the user (save dialogs). */
	const char* file_name;
	const RFileFilter* filters;
	int filter_count;
	/** Save dialogs: added to a name typed without extension ("xml"). */
	const char* default_extension;
	/** Bitmask of RFileDialogFlags. */
	Ruint flags;
} RFileDialogDesc;

typedef enum RFileBrowserMode{
	/** Choose an existing file. */
	R_FILE_BROWSER_OPEN = 0x0380,
	/** Choose where to save: a name is typed; an existing file must be confirmed. */
	R_FILE_BROWSER_SAVE,
	/** Choose a folder. */
	R_FILE_BROWSER_FOLDER
}RFileBrowserMode;

typedef enum RMessageButtons{
	R_MESSAGE_OK = 0x0390,
	R_MESSAGE_OK_CANCEL,
	R_MESSAGE_YES_NO,
	R_MESSAGE_YES_NO_CANCEL
}RMessageButtons;

typedef enum RMessageResult{
	/** Dismissed (Escape), or the Cancel button. */
	R_MESSAGE_RESULT_CANCEL = 0,
	R_MESSAGE_RESULT_OK,
	R_MESSAGE_RESULT_YES,
	R_MESSAGE_RESULT_NO
}RMessageResult;

/** @brief Called when a dialog closes. `result` is the value given to Rdialog_close, 0 when dismissed. */
typedef void (*RDialogClosedCallback)(Rid content, int result, void* user_data);


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  DIALOGS
	 * ============================================================================ */

	/**
	 * @brief Shows a widget as a dialog of a window. The window takes
	 * ownership of `content`: it is deleted when the dialog closes, so read
	 * what you need from it before, or in `closed`.
	 * The Escape key dismisses the dialog (result 0).
	 */
	RAVEN_API void Rdialog_open(Rid window, Rid content, RDialogClosedCallback closed, void* user_data);

	/**
	 * @brief Closes the dialog that contains a widget (the content itself or
	 * anything inside it: the button that was clicked).
	 * @param result Passed to the closed callback and returned by Rdialog_run.
	 */
	RAVEN_API void Rdialog_close(Rid widget, int result);

	/** @brief Shows a dialog and waits until it closes. Returns its result. */
	RAVEN_API int Rdialog_run(Rid window, Rid content);

	/**
	 * @brief Shows a widget as a popup over the window, its top-left corner at
	 * x, y (window coordinates; moved to stay inside the window): a search box,
	 * a palette, a picker. Unlike a dialog it does not block the window: a click
	 * outside it, or Escape, closes it. The application keeps the content (it is
	 * not deleted on close) and can open it again. `closed` (may be NULL) is
	 * called when it closes, with the result given to Rpopup_close (0 when dismissed).
	 * The content draws its own background (a Border with a Background...).
	 */
	RAVEN_API void Rpopup_open(Rid window, Rid content, float x, float y, RDialogClosedCallback closed, void* user_data);
	/** @brief Closes the popup that contains a widget (the content or anything inside it). */
	RAVEN_API void Rpopup_close(Rid widget, int result);
	/** @return R_TRUE while the widget is shown as a popup. */
	RAVEN_API int Rpopup_is_open(Rid content);

	/** @return R_TRUE while a dialog is shown in the window. */
	RAVEN_API int Rwindow_has_dialog(Rid window);

	/**
	 * @brief Shows a message with buttons and waits for the answer.
	 * @param window R_INVALID_ID: the first window of the application.
	 * @param title  May be NULL.
	 */
	RAVEN_API RMessageResult Rmessage_box(Rid window, const char* title, const char* text, RMessageButtons buttons);


	/* ============================================================================
	 *  FILE DIALOGS
	 * ============================================================================ */

	/**
	 * @brief Asks for a file to open.
	 * @param desc May be NULL.
	 * @return The path (UTF-8), valid until the next file dialog, or NULL when the user cancelled.
	 */
	RAVEN_API const char* Rfile_dialog_open(const RFileDialogDesc* desc);
	/** @brief Asks where to save a file. An existing file was confirmed by the user. */
	RAVEN_API const char* Rfile_dialog_save(const RFileDialogDesc* desc);
	/** @brief Asks for a folder. */
	RAVEN_API const char* Rfile_dialog_select_folder(const RFileDialogDesc* desc);

	/** @brief What the last file dialog returned: several paths with R_FILE_DIALOG_MULTIPLE. */
	RAVEN_API int Rfile_dialog_get_result_count(void);
	RAVEN_API const char* Rfile_dialog_get_result(int index);

	/**
	 * @brief Registers a file held in memory under a path: every file Raven
	 * reads (XML documents and their Include, Source and Font attributes,
	 * themes, textures, fonts, Rfile_read) is looked up here first, so that
	 * resources embedded in the executable need no file beside it. A relative
	 * path names the file as it is written in the XML once joined to the
	 * folder of the document that names it (Rxml_set_base_path for a
	 * document given as text), e.g. "ui/editor.xml". Paths compare with '/'
	 * and '\\' alike. Memory files are not watched for live reload. Call it
	 * after Rinit; Rshutdown forgets every file.
	 * @param copy R_TRUE: Raven keeps a copy; R_FALSE: `data` must outlive Raven (static data).
	 */
	RAVEN_API int Rfile_register_memory(const char* path, const void* data, size_t size, int copy);
	RAVEN_API void Rfile_unregister_memory(const char* path);
	/** @brief R_TRUE when `path` is a file registered with Rfile_register_memory. */
	RAVEN_API int Rfile_is_memory(const char* path);

	/**
	 * Whole files, with UTF-8 paths on every platform.
	 * On Android and iOS, a relative path that names no file held in memory
	 * and no file on disk is read from the app's package (the APK's assets/,
	 * the bundle's resources), and so is every file Raven reads by its path.
	 * @return Rfile_read: the content followed by a '\0', to release with
	 * Rmemory_free; NULL when the file cannot be read. `size` may be NULL.
	 */
	RAVEN_API char* Rfile_read(const char* path, size_t* size);
	RAVEN_API int Rfile_write(const char* path, const void* data, size_t size);


	/* ============================================================================
	 *  FILE BROWSER
	 * ============================================================================ */

	RAVEN_API Rid Rfile_browser_create(void);

	RAVEN_API void Rfile_browser_set_mode(Rid browser, RFileBrowserMode mode);
	/** @brief Shows a folder. "~" is the home of the user. Returns R_FALSE when it cannot be read. */
	RAVEN_API int Rfile_browser_set_directory(Rid browser, const char* path);
	RAVEN_API const char* Rfile_browser_get_directory(Rid browser);
	/** @brief Reads the folder again. */
	RAVEN_API void Rfile_browser_refresh(Rid browser);

	/** @brief Kinds of files the user chooses from; the first one is active. The strings are copied. */
	RAVEN_API void Rfile_browser_set_filters(Rid browser, const RFileFilter* filters, int count);
	RAVEN_API void Rfile_browser_set_show_hidden(Rid browser, int show);
	/** @brief R_TRUE lets several files be selected (Ctrl and Shift with a click). */
	RAVEN_API void Rfile_browser_set_multiple(Rid browser, int multiple);
	/**
	 * @brief R_FALSE hides the name field, the filter list and the buttons:
	 * what remains is an explorer to put in a side panel. Shown by default.
	 */
	RAVEN_API void Rfile_browser_set_controls_visible(Rid browser, int visible);
	/** @brief R_FALSE hides the list of places on the left. */
	RAVEN_API void Rfile_browser_set_places_visible(Rid browser, int visible);

	/** @brief Name typed in the name field (save mode). */
	RAVEN_API void Rfile_browser_set_file_name(Rid browser, const char* name);
	/** @brief Added to a name typed without extension, in save mode. */
	RAVEN_API void Rfile_browser_set_default_extension(Rid browser, const char* extension);

	/** @brief Paths of the selected entries. The strings are valid until the selection or the folder changes. */
	RAVEN_API int Rfile_browser_get_selection_count(Rid browser);
	RAVEN_API const char* Rfile_browser_get_selection(Rid browser, int index);
	/** @brief Selects an entry of the folder by its name. */
	RAVEN_API void Rfile_browser_select(Rid browser, const char* name);

	/**
	 * @brief Called when the user chooses: a double click or Enter on a file,
	 * the Open / Save button. `path` is the first chosen path; with several
	 * files, read them with Rfile_browser_get_selection. The action of the
	 * widget (OnActivated in XML) fires at the same time.
	 */
	typedef void (*RFileBrowserCallback)(Rid browser, const char* path, void* user_data);
	RAVEN_API void Rfile_browser_set_accepted_callback(Rid browser, RFileBrowserCallback callback, void* user_data);
	/** @brief Called by the Cancel button. */
	RAVEN_API void Rfile_browser_set_cancelled_callback(Rid browser, RFileBrowserCallback callback, void* user_data);
	/** @brief Called when the selection changes; `path` is the entry under the cursor, or NULL. */
	RAVEN_API void Rfile_browser_set_selection_changed_callback(Rid browser, RFileBrowserCallback callback, void* user_data);


	/* ============================================================================
	 *  NOTIFICATIONS OF THE SYSTEM
	 *
	 * Rnotification_show asks the system to show a notification outside of the
	 * windows: Windows (a toast of the notification area), macOS (the
	 * Notification Center, for an application in a bundle), Linux and the
	 * BSDs (org.freedesktop.Notifications: GNOME, KDE, mako, dunst...). It is
	 * named after the application (RInitDesc::app_name).
	 *
	 * Where the system shows nothing (no notification server, the null
	 * platform, an executable outside of a bundle on macOS, notifications
	 * the user turned off), the fallback given to Rnotification_set_fallback
	 * is called instead, for instance to show the message in a Toast of the
	 * main window. On macOS the first notification asks the user for the
	 * permission: if the answer is no, that notification goes to the fallback
	 * once the answer is given, after Rnotification_show returned R_TRUE.
	 * ============================================================================ */

	/** @return R_TRUE when the system took the notification. */
	RAVEN_API int Rnotification_show(const char* title, const char* body, RNotificationUrgency urgency);
	/**
	 * @brief R_FALSE when the system shows no notifications here (Rnotification_show then always uses
	 * the fallback): the null platform, no notification server on Linux, an executable outside of a
	 * bundle on macOS.
	 */
	RAVEN_API int Rnotification_is_supported(void);
	typedef void (*RNotificationFallback)(const char* title, const char* body, RNotificationUrgency urgency, void* user_data);
	/** @brief Called with the notifications the system could not show; NULL: none. */
	RAVEN_API void Rnotification_set_fallback(RNotificationFallback callback, void* user_data);
	typedef void (*RNotificationClickedCallback)(void* user_data);
	/**
	 * @brief Called when the user clicks a notification of Rnotification_show
	 * (on the UI thread), as R_NOTIFICATION_ACTIVATED of
	 * Rnotification_set_callback, which also tells which one. NULL: none.
	 */
	RAVEN_API void Rnotification_set_clicked_callback(RNotificationClickedCallback callback, void* user_data);

	/*
	 * Tags, buttons, an icon, and what the user did with the notification.
	 * The fallback toast (Rnotification_set_fallback_toast) shows the same
	 * notification in a Toast of the application, with its buttons, when the
	 * system shows none, and reports to the same callback: an application
	 * behaves the same wherever its notifications are shown.
	 *
	 *     static const RNotificationAction actions[] = { { "reply", "Reply" }, { "later", "Later" } };
	 *     RNotificationDesc desc = { sizeof(RNotificationDesc) };
	 *     desc.tag = "chat-42";
	 *     desc.title = "Ada";
	 *     desc.body = "Are we still on for tonight?";
	 *     desc.actions = actions;
	 *     desc.action_count = 2;
	 *     Rnotification_show_desc(&desc);
	 *
	 * What the systems do: macOS (inside a bundle) shows the buttons, the
	 * icon (PNG, JPEG, GIF, HEIC) and reports every event. Linux and the BSDs
	 * show the buttons where the notification server has them (GNOME, KDE,
	 * dunst, mako...), the icon, and report every event. Windows shows a
	 * balloon of the notification area: one at a time, no buttons, no icon
	 * of its own; a click is reported. Rnotification_supports tells.
	 */

	/**
	 * @brief Shows a notification described by `desc` (see RNotificationDesc).
	 * @return R_TRUE when the system took it; R_FALSE when it went to the fallback (or nowhere).
	 */
	RAVEN_API int Rnotification_show_desc(const RNotificationDesc* desc);
	/** @brief Takes away the notification of that tag, shown by the system or by the fallback toast. The callback is not called. */
	RAVEN_API void Rnotification_close(const char* tag);
	/**
	 * @brief Called on the UI thread when the user clicks a notification or
	 * one of its buttons, or dismisses it. `tag` names the notification,
	 * `action` is the id of the button for R_NOTIFICATION_ACTION_INVOKED, else NULL.
	 * NULL: none.
	 */
	typedef void (*RNotificationCallback)(const char* tag, RNotificationEvent event, const char* action, void* user_data);
	RAVEN_API void Rnotification_set_callback(RNotificationCallback callback, void* user_data);
	/**
	 * @brief R_TRUE (the default): a click on a notification (not on one of
	 * its buttons) brings its window back before the callback is called:
	 * shown again if it was hidden or minimized, in front, with the keyboard.
	 * R_FALSE: the application does what it wants in the callback.
	 */
	RAVEN_API void Rnotification_set_bring_to_front(int enabled);
	/**
	 * @brief A Toast that shows the notifications the system could not show,
	 * with their title, icon and buttons; a click on one, or on a button, is
	 * reported to Rnotification_set_callback as for the system's. Put it in
	 * an Overlay (Queue="Stack" shows several at once). It takes the place
	 * of the fallback callback (Rnotification_set_fallback). R_INVALID_ID: none.
	 */
	RAVEN_API void Rnotification_set_fallback_toast(Rid toast);
	/** @brief Whether the notifications of the system have that feature here (R_FALSE where there are none). */
	RAVEN_API int Rnotification_supports(RNotificationFeature feature);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_DIALOGS_H */
