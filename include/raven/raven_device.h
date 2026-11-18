/**
 * Raven
 * raven_device.h : the services of the device: permissions, the pickers and
 * the camera of the system, haptics, sharing.
 *
 * ASYNCHRONOUS
 *
 * On a phone these services show a screen of the system (a question, a
 * picker, the camera) while the application keeps running, and the answer
 * comes later. Every request here returns at once; its callback is called
 * later, on the UI thread, during a frame (Rwait_frame, Rdraw_frame), never
 * from inside the call that asked. A callback is always called exactly once,
 * also when the service does not exist here (R_DEVICE_UNSUPPORTED), unless
 * Raven shuts down first.
 *
 * While the system shows its screen the application may receive
 * R_APP_WILL_PAUSE and R_APP_DID_RESUME (Rset_app_event_callback): a
 * question of the system takes the focus from it, and on Android the
 * camera or a picker is another activity in front of it.
 *
 * Only one screen of the system at a time: a request made while a picker,
 * the camera or a share sheet is still open is answered R_DEVICE_BUSY.
 *
 * FILES
 *
 * The files the user picks are given as paths that Raven and the C library
 * can read: Rfile_read, Rtexture_create_from_file, Rfont_create_from_file,
 * fopen. On iOS and Android, where the system gives a security-scoped URL or
 * a content:// URI rather than a path, the file is first copied into the
 * cache folder of the application: the copy may be removed by the system
 * when it needs space, and at the next start of the application.
 *
 * On desktops these functions use the file dialogs of raven_dialogs.h
 * (Rfile_dialog_open...), which wait for the user, then answer at the next
 * frame. The camera and the share sheet do not exist there (R_DEVICE_UNSUPPORTED).
 *
 * WHAT THE APPLICATION DECLARES
 *
 *   Service                Android manifest                        iOS Info.plist
 *   ---------------------  --------------------------------------  ------------------------------
 *   Rcamera_take_photo     CAMERA (uses-permission; only needed    NSCameraUsageDescription
 *                          when declared: then it is asked first)
 *   Rphoto_pick            nothing (the photo picker)              nothing (PHPickerViewController)
 *   R_PERMISSION_PHOTOS    READ_MEDIA_IMAGES (13+),                NSPhotoLibraryUsageDescription
 *                          READ_EXTERNAL_STORAGE (12 and older)
 *   R_PERMISSION_MICROPHONE RECORD_AUDIO                           NSMicrophoneUsageDescription
 *   R_PERMISSION_LOCATION  ACCESS_FINE_LOCATION or _COARSE_        NSLocationWhenInUseUsageDescription
 *   Notifications          POST_NOTIFICATIONS (13+)                nothing
 *   Rfile_open_async,      nothing (the system's document          nothing (UIDocumentPickerViewController)
 *   Rfile_save_async       picker)
 *   Images from a URL      INTERNET (no question at run time)      nothing for https://; http:// needs
 *                                                                   NSAppTransportSecurity
 *   Ropen_url, Rshare_show nothing                                 nothing
 *   Rhaptic_play           VIBRATE                                 nothing
 *
 * Android also needs Raven's two Java classes in the manifest (the activity
 * that asks and receives the answers, the provider that hands files to other
 * applications): see examples/mobile/android/AndroidManifest.xml.
 */

#ifndef RAVEN_DEVICE_H
#define RAVEN_DEVICE_H

#include "raven_dialogs.h"

/** What the application may be allowed to use (Rpermission_request). */
typedef enum RPermission{
	R_PERMISSION_CAMERA = 0x0C00,
	/** Notifications of the system (Rnotification_show). */
	R_PERMISSION_NOTIFICATIONS,
	/** Reading the whole photo library. Rphoto_pick needs none: the system's picker gives only what the user chose. */
	R_PERMISSION_PHOTOS,
	R_PERMISSION_MICROPHONE,
	/** The location of the device while the application is in use. */
	R_PERMISSION_LOCATION
}RPermission;

/** What the user answered, or would be asked (Rpermission_get_status). */
typedef enum RPermissionStatus{
	/**
	 * There is nothing to ask for here: the platform has no such permission,
	 * or Raven cannot ask for it on this platform. Whether the service itself
	 * exists is told by its own function (Rcamera_is_available,
	 * Rnotification_is_supported).
	 */
	R_PERMISSION_UNSUPPORTED = 0x0C10,
	/** Not asked yet: Rpermission_request shows the question of the system. */
	R_PERMISSION_NOT_ASKED,
	/** Allowed (also where the system asks nothing: notifications on Windows and Linux). */
	R_PERMISSION_GRANTED,
	/** Allowed in part: some photos only (iOS, Android 14), quiet notifications (iOS provisional), the approximate location only. */
	R_PERMISSION_LIMITED,
	/** Refused, and the system may ask again (Android, after a first refusal). */
	R_PERMISSION_DENIED,
	/**
	 * Refused for good, or not the user's to give (parental controls, a
	 * profile of the company): the system shows no question any more, only
	 * its Settings can change it (Rpermission_open_settings).
	 */
	R_PERMISSION_BLOCKED
}RPermissionStatus;

/** How a request of a picker, the camera or a share sheet ended. */
typedef enum RDeviceResult{
	/** Done: the files were chosen, the photo taken, the file saved, the sheet shown. */
	R_DEVICE_DONE = 0x0C20,
	/** The user closed the screen of the system without choosing. */
	R_DEVICE_CANCELLED,
	/** A permission it needs was refused (Rcamera_take_photo: the camera). */
	R_DEVICE_DENIED,
	/** The platform does not have this service. */
	R_DEVICE_UNSUPPORTED,
	/** Another screen of the system is still open: one at a time. */
	R_DEVICE_BUSY,
	/** The system reported an error (also reported as an R_ERROR_PLATFORM warning). */
	R_DEVICE_FAILED
}RDeviceResult;

/** A short vibration that answers a touch (Rhaptic_play). */
typedef enum RHaptic{
	/** Impacts: something touched, snapped, landed. */
	R_HAPTIC_LIGHT = 0x0C30,
	R_HAPTIC_MEDIUM,
	R_HAPTIC_HEAVY,
	/** The selection moved (a picker wheel, a switch, a segmented control). */
	R_HAPTIC_SELECTION,
	/** The outcome of an action. */
	R_HAPTIC_SUCCESS,
	R_HAPTIC_WARNING,
	R_HAPTIC_ERROR
}RHaptic;

/** A file given by a service of the device. Valid during the callback only: copy what you keep. */
typedef struct RPickedFile {
	/**
	 * A path Raven and the C library can read (Rfile_read, textures, fonts,
	 * fopen). On iOS and Android a copy in the cache of the application.
	 * NULL for Rfile_save_async on iOS and Android: the file went where the
	 * user chose, which the application cannot write to again.
	 */
	const char* path;
	/** The name the user saw ("IMG_0042.JPG"), to show. */
	const char* name;
	/** The MIME type when the system tells ("image/jpeg"), or NULL. */
	const char* mime_type;
	/** Bytes. */
	uint64_t size;
	/** Pixels of a photo (Rcamera_take_photo, Rphoto_pick) when known, else 0. */
	int width, height;
} RPickedFile;

/** What a share sheet offers to the other applications (Rshare_show). Every field may be left to zero, but not all of them. */
typedef struct RShareDesc {
	/** Text, or a link ("https://..."). May be NULL. */
	const char* text;
	/** A file to send (a path Raven can read), or NULL. */
	const char* path;
	/** Its MIME type ("image/png"); NULL: guessed from its extension. */
	const char* mime_type;
	/** The title of the sheet, where the system shows one (Android's chooser). May be NULL. */
	const char* title;
	/** The widget the sheet points at on a tablet (the button that shares). 0 or R_INVALID_ID: the middle of the window. */
	Rid anchor;
} RShareDesc;

/** @brief The answer to Rpermission_request. */
typedef void (*RPermissionCallback)(RPermission permission, RPermissionStatus status, void* user_data);

/**
 * @brief The end of a request of a picker, the camera, a save or a share
 * sheet. `files` (count of them) are what it gave: none unless `result` is
 * R_DEVICE_DONE. Called on the UI thread, once.
 */
typedef void (*RDeviceCallback)(RDeviceResult result, const RPickedFile* files, int count, void* user_data);


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  PERMISSIONS
	 *
	 * Raven asks for what its own services need: Rcamera_take_photo asks for
	 * the camera first when it was never asked, and on iOS and Android
	 * Rnotification_show asks for notifications the first time (macOS does it
	 * too); a refusal sends the notification to its fallback. Ask yourself
	 * beforehand to choose the moment, or to explain why first.
	 *
	 * The user can change an answer in the Settings of the system while the
	 * application is in the background: read the status again on
	 * R_APP_DID_RESUME.
	 * ============================================================================ */

	/**
	 * @brief What the user answered, without asking. Cheap: the platform
	 * remembers it. On iOS the status of notifications is known shortly after
	 * the start (R_PERMISSION_NOT_ASKED until then).
	 */
	RAVEN_API RPermissionStatus Rpermission_get_status(RPermission permission);
	/**
	 * @brief Asks the user, with the question of the system, when the status
	 * is R_PERMISSION_NOT_ASKED or R_PERMISSION_DENIED; otherwise only answers
	 * with the status. Several requests for the same permission share one
	 * question. `callback` may be NULL.
	 */
	RAVEN_API void Rpermission_request(RPermission permission, RPermissionCallback callback, void* user_data);
	/**
	 * @brief Opens the page of the application in the Settings of the system,
	 * where the user changes what was refused for good (R_PERMISSION_BLOCKED).
	 * @return R_FALSE when the platform has no such page.
	 */
	RAVEN_API int Rpermission_open_settings(void);
	/** @return The name of a permission or a status ("camera", "granted"...), for logs. */
	RAVEN_API const char* Rpermission_to_string(RPermission permission);
	RAVEN_API const char* Rpermission_status_to_string(RPermissionStatus status);


	/* ============================================================================
	 *  FILES: the pickers of the system
	 *
	 * The asynchronous forms of the file dialogs (raven_dialogs.h), the only
	 * ones on iOS and Android. `desc` is read during the call only; its
	 * `parent` is the window the picker belongs to (R_INVALID_ID: the first),
	 * `filters` the kinds of files that can be chosen (patterns of
	 * extensions: "*.png;*.jpg"; the platforms turn them into types of their
	 * own), R_FILE_DIALOG_MULTIPLE lets several files be chosen. `directory`
	 * and R_FILE_DIALOG_RAVEN are for desktops only.
	 * ============================================================================ */

	/** @brief Asks for one file to read (several with R_FILE_DIALOG_MULTIPLE). `desc` may be NULL. */
	RAVEN_API void Rfile_open_async(const RFileDialogDesc* desc, RDeviceCallback callback, void* user_data);
	/**
	 * @brief Saves `size` bytes (copied: `data` may be freed at once) as a
	 * file where the user chooses, proposed under `desc->file_name`. The
	 * callback gives the file as saved: its name, and its path on desktops.
	 */
	RAVEN_API void Rfile_save_async(const RFileDialogDesc* desc, const void* data, size_t size, RDeviceCallback callback, void* user_data);
	/** @brief Same as Rfile_save_async, for a file that already exists (a path Raven can read), copied where the user chooses. */
	RAVEN_API void Rfile_export_async(const RFileDialogDesc* desc, const char* path, RDeviceCallback callback, void* user_data);
	/** @brief Asks for a folder: desktops only for now (R_DEVICE_UNSUPPORTED on iOS and Android). */
	RAVEN_API void Rfile_select_folder_async(const RFileDialogDesc* desc, RDeviceCallback callback, void* user_data);


	/* ============================================================================
	 *  CAMERA AND PHOTOS
	 * ============================================================================ */

	/** @return R_TRUE when Rcamera_take_photo can show a camera (none on desktops, nor in the iOS Simulator). */
	RAVEN_API int Rcamera_is_available(void);
	/**
	 * @brief Shows the camera of the system to take one photo. It asks for
	 * R_PERMISSION_CAMERA first when needed (R_DEVICE_DENIED when refused).
	 * The photo is a JPEG file in the cache of the application, with its
	 * size in pixels, upright.
	 */
	RAVEN_API void Rcamera_take_photo(RDeviceCallback callback, void* user_data);
	/**
	 * @brief Shows the photo picker of the system (desktops: a file dialog for
	 * images). No permission is needed: the application only sees what the
	 * user picks. Each photo is a file Raven can load (iOS: converted to JPEG
	 * when it is HEIC).
	 * @param max_count How many photos may be picked; 0: no limit.
	 */
	RAVEN_API void Rphoto_pick(int max_count, RDeviceCallback callback, void* user_data);


	/* ============================================================================
	 *  HAPTICS, SHARING
	 * ============================================================================ */

	/** @return R_TRUE when the device can play haptics (a phone; never a desktop). */
	RAVEN_API int Rhaptic_is_supported(void);
	/**
	 * @brief A short vibration, as the controls of the system play when
	 * touched. Nothing happens where there are none, or when the user turned
	 * them off. @return R_TRUE when it was handed to the system.
	 */
	RAVEN_API int Rhaptic_play(RHaptic haptic);

	/**
	 * @brief Shows the share sheet of the system with a text and / or a file,
	 * to send them to another application (mail, chat, Files...). The
	 * callback says R_DEVICE_DONE once something was chosen (Android: once the
	 * sheet was shown; it does not tell more), R_DEVICE_CANCELLED when it was
	 * closed. R_DEVICE_UNSUPPORTED on desktops.
	 */
	RAVEN_API void Rshare_show(const RShareDesc* desc, RDeviceCallback callback, void* user_data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_DEVICE_H */
