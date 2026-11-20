# Device services: the platform contract

`include/raven/raven_device.h` gives applications the services of a phone:
permissions, the document pickers, the camera and the photo picker, haptics,
the share sheet; `Ropen_url`, `Rtexture_create_from_url_async` and the
clipboard go through the same platform entries. This page is for whoever
implements them for a platform (iOS: `src/platform/uikit/uikit_services.m`,
Android: `src/platform/android/android_platform.c` and `src/platform/android/java/raven/android/RavenDevice.java`). The entries
themselves are documented one by one in `src/platform/platform.h` (section
"services of the device"); the core side is `src/platform/services.c`.

## How it works

```
application            core (services.c)                    platform
-----------            -----------------                    --------
Rfile_open_async  -->  request id 7, one sheet at a time -->  file_open(request)    returns at once
                                                              ... the user picks ...
                       task posted, Raven woken          <--  rvn_service_complete(7, R_DEVICE_DONE, files)
callback(DONE, files) <-- next frame, UI thread                (any thread)
```

- Every entry is called **on Raven's UI thread** (iOS: the main thread;
  Android: the `android_main` thread, *not* the Java main thread) and
  **returns at once**.
- A request that shows a screen of the system is answered by **exactly one**
  call, from **any thread**: `rvn_permission_answer(permission, status)` for
  `permission_request`, `rvn_service_complete(id, result, files, count, error)`
  for `file_open`, `file_save`, `folder_select`, `camera_take_photo`,
  `photo_pick`, `share`. The core copies what it is given (the platform can
  free its strings when the call returns), posts it as a task, wakes the UI
  thread, and calls the application's callback at the next frame. An answer
  may even come before the entry returns.
- The platform must always answer: a cancelled screen is `R_DEVICE_CANCELLED`,
  a screen dismissed because the activity / scene went away is
  `R_DEVICE_CANCELLED` too, a system error `R_DEVICE_FAILED` with the
  message in `error`. A permission question that ends without an answer
  reports the status as it now is. Until the platform answers, the core
  shows nothing else (one screen of the system at a time: the others get
  `R_DEVICE_BUSY`) and asks no other question for that permission.
- The core asks for the camera permission before it calls
  `camera_take_photo` (and answers `R_DEVICE_DENIED` itself when refused);
  `camera_take_photo` can assume it is granted, or that `permission_status`
  said `R_PERMISSION_UNSUPPORTED` (nothing to ask).
- After `Rshutdown` the core ignores late answers: a completion block or a
  Java callback that fires then is harmless. The platform should still drop
  its own references in its `shutdown` entry.
- An entry left `NULL` means "not supported": the core answers
  `R_DEVICE_UNSUPPORTED` / `R_PERMISSION_UNSUPPORTED` (on iOS and Android the
  file pickers too; desktops fall back to their file dialogs). The showcase
  then shows "not supported here (yet)", so each entry can be added alone.

### Files handed back

Each `RPickedFile::path` must be a plain path that `fopen` can read: Raven
reads picked files with `Rfile_read`, decodes images on worker threads and
loads fonts by path, so a `content://` URI or a security-scoped URL is not
enough. **Copy the file into the application's cache** and give that path:

- folder: `<cache>/raven/picked/<request id>/<display name>` (iOS: `NSCachesDirectory`;
  Android: `getCacheDir()`), the photos taken in `<cache>/raven/camera/`;
- copy **off the main thread** (a 4 GB video must not freeze the UI), then
  call `rvn_service_complete` from that thread;
- empty `<cache>/raven/picked` and `<cache>/raven/camera` in `init` (the
  header promises that the copies last until the next start at most);
- give `name` (what the user saw), `mime_type` when known, `size` (0: the
  core reads it from the file), and `width` / `height` in pixels for photos.

A provider (`rvn_file_provider_t`) was considered instead of copies: it only
serves relative paths of the package, there is one of them, and security
scopes / URI grants would have to stay open for as long as the application
may read. Copies are simpler and work with everything that takes a path.

`file_save` gets the bytes (`data` / `size`, valid during the call only:
copy them) or a file to copy (`path`, when `data` is NULL). Answer with one
file whose `name` is what the user chose and `path` NULL: the place belongs
to the user, the application cannot write there again.

`rvn_service_extensions(request, &count)` turns `request->filters`
(`"*.png;*.jpg"`) into lowercase extensions (`"png"`, `"jpg"`) for
`UTType typeWithFilenameExtension:` or `MimeTypeMap.getMimeTypeFromExtension`;
count 0 means any file.

### Permissions

`permission_status(permission)` is called often (every time the application
asks for the status): it must not block. Mapping:

| | `R_PERMISSION_...` |
|---|---|
| never asked | `NOT_ASKED` |
| allowed | `GRANTED` |
| allowed in part (iOS limited photos, provisional notifications; Android 14 selected photos; the approximate location only, on both) | `LIMITED` |
| refused, the system may ask again (Android after one refusal) | `DENIED` |
| refused and the system will not ask (iOS after any refusal; Android "don't ask again" or two refusals; restricted by parental controls / MDM) | `BLOCKED` |
| the platform has no such permission, or the platform does not handle it yet | `UNSUPPORTED` |

`permission_request(permission)` is only called for `NOT_ASKED` and `DENIED`,
once until answered. Answer with `rvn_permission_answer`, from any thread.
Call `rvn_permission_answer` also when the platform learns a status by
itself (its `notify` asked the user): requests waiting for that permission
get it.

`notify` (`Rnotification_show`) on iOS and Android: when the notification
permission is `NOT_ASKED`, ask (and report the answer with
`rvn_permission_answer`), then show the notification if granted, or call
`rvn_notification_fallback(title, body, urgency, reason)` if not (as macOS
does). Return `R_FALSE` at once when it is `DENIED` / `BLOCKED`.

### Lifecycle

A question or a screen of the system takes the focus: the application sees
`R_APP_WILL_PAUSE` then `R_APP_DID_RESUME` (iOS: an alert makes the
application resign active; Android: the helper activity, the permission
dialog and the pickers are other activities, so the NativeActivity is
paused, and stopped when the new one is opaque: `R_APP_DID_PAUSE`, and its
surface goes, `RVN_PEVENT_SURFACE_LOST`). Nothing special is needed from
the platform for this; it must just not lose the request meanwhile. The
answer may arrive while Raven is paused: the core delivers it at the next
frame, which comes when the application is back.

SHORTCUT (both): when Android kills the process while the camera is in
front, the request is lost with it; Raven does not restore it after the
restart.

## Entries, threads, and what to use

| Entry | iOS | Android |
|---|---|---|
| `permission_status` | camera / microphone: `AVCaptureDevice authorizationStatusForMediaType:`; photos: `PHPhotoLibrary authorizationStatusForAccessLevel:PHAccessLevelReadWrite`; notifications: `UNUserNotificationCenter getNotificationSettings` is asynchronous: keep its last `authorizationStatus`, fetched in `init` and on `UIApplicationWillEnterForegroundNotification`; location: `CLLocationManager.authorizationStatus` | `checkSelfPermission` (CAMERA, POST_NOTIFICATIONS on 33+, READ_MEDIA_IMAGES on 33+ / READ_EXTERNAL_STORAGE before, RECORD_AUDIO, ACCESS_FINE/COARSE_LOCATION); notifications below 33: `NotificationManager.areNotificationsEnabled()` (`GRANTED` / `BLOCKED`); not granted: `NOT_ASKED` if never asked (remember asking in `SharedPreferences`), else `shouldShowRequestPermissionRationale` ? `DENIED` : `BLOCKED` |
| `permission_request` | `AVCaptureDevice requestAccessForMediaType:`, `UNUserNotificationCenter requestAuthorizationWithOptions:`, `PHPhotoLibrary requestAuthorizationForAccessLevel:`, `CLLocationManager requestWhenInUseAuthorization` (answer from `locationManagerDidChangeAuthorization:`). Completion handlers run on any queue: call `rvn_permission_answer` there | `RavenRequestActivity`: `requestPermissions()` in it, `onRequestPermissionsResult` → a native method → `rvn_permission_answer`, `finish()`. NativeActivity gets no `onRequestPermissionsResult`, hence the helper |
| `permission_open_settings` | `UIApplicationOpenSettingsURLString` with `openURL:` | `Settings.ACTION_APPLICATION_DETAILS_SETTINGS`, `Uri.fromParts("package", packageName, null)` |
| `file_open` | `UIDocumentPickerViewController initForOpeningContentTypes:asCopy:YES` (types from `rvn_service_extensions`, `UTTypeItem` for any), `allowsMultipleSelection`; `asCopy` gives local copies, no security scope: move them into the cache folder | `RavenRequestActivity` with `ACTION_OPEN_DOCUMENT` (`CATEGORY_OPENABLE`, `EXTRA_MIME_TYPES`, `EXTRA_ALLOW_MULTIPLE`); copy each URI with `ContentResolver.openInputStream` on a thread; the display name and size from `OpenableColumns` |
| `file_save` | write the bytes to a temporary file named `file_name`, then `UIDocumentPickerViewController initForExportingURLs:asCopy:YES`; the picked URL gives the name | `ACTION_CREATE_DOCUMENT` (`EXTRA_TITLE` = `file_name`, its MIME type); keep the bytes until the URI comes, write them with `ContentResolver.openOutputStream` |
| `folder_select` | `NULL` for now | `NULL` for now (`ACTION_OPEN_DOCUMENT_TREE` gives a tree URI, not a path: open question) |
| `camera_available` | `UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypeCamera` (NO in the Simulator) | `PackageManager.hasSystemFeature(FEATURE_CAMERA_ANY)` and an activity for `ACTION_IMAGE_CAPTURE` (the manifest has the `<queries>` entry) |
| `camera_take_photo` | `UIImagePickerController` with the camera source (or AVFoundation); draw the `UIImage` upright, JPEG (0.9) into the cache, width / height in pixels | `RavenRequestActivity` with `MediaStore.ACTION_IMAGE_CAPTURE`, `EXTRA_OUTPUT` = a `RavenFileProvider` URI of `<cache>/raven/camera/<id>.jpg`; then read its EXIF orientation and rotate if needed (`BitmapFactory`, `ExifInterface`) so that the file is upright |
| `photo_pick` | `PHPickerViewController` (no permission), `filter = images`, `selectionLimit = max_count`; `loadFileRepresentationForTypeIdentifier:`; HEIC (and anything stb cannot read) converted to JPEG with ImageIO; size from `CGImageSourceCopyPropertiesAtIndex` | `MediaStore.ACTION_PICK_IMAGES` (API 33+, or 30+ with the SDK extension; `EXTRA_PICK_IMAGES_MAX`), else `ACTION_GET_CONTENT` `image/*`; copy like `file_open`; HEIC: decode with `BitmapFactory` and write a JPEG |
| `share` | `UIActivityViewController` with the text and `[NSURL fileURLWithPath:]`; on iPad `popoverPresentationController.sourceView` = Raven's view and `sourceRect` = `anchor` divided by the view's `contentScaleFactor` (the anchor is in pixels; width 0: the middle); `completionWithItemsHandler` → `DONE` / `CANCELLED` | `Intent.ACTION_SEND` (`EXTRA_TEXT`; `EXTRA_STREAM` = a `RavenFileProvider` URI of the file, copied into `<cache>/raven/shared/` when it is elsewhere, with `FLAG_GRANT_READ_URI_PERMISSION` and `ClipData`), `Intent.createChooser(intent, share_title)`; answer `DONE` once started (Android does not tell more) |
| `open_url` | `[UIApplication.sharedApplication openURL:options:@{} completionHandler:nil]`, return YES | `Intent.ACTION_VIEW` with `Uri.parse(url)`; `ActivityNotFoundException` → `R_FALSE` |
| `haptic_supported`, `haptic` | YES on an iPhone (the Simulator plays nothing); light / medium / heavy: `UIImpactFeedbackGenerator` with that style; selection: `UISelectionFeedbackGenerator selectionChanged`; success / warning / error: `UINotificationFeedbackGenerator notificationOccurred:` | `Vibrator.hasVibrator()`; `View.performHapticFeedback` on the decor view, **on the Java main thread** (post it): light `KEYBOARD_TAP`, medium `VIRTUAL_KEY`, heavy `LONG_PRESS`, selection `CLOCK_TICK` (`SEGMENT_TICK` 34+), success `CONFIRM` (30+), warning / error `REJECT` (30+); it follows the user's "touch feedback" setting. `VIBRATE` is in the manifest for a `Vibrator` fallback (`VibrationEffect.createPredefined`) |
| `download` | `NSURLSession dataTaskWithRequest:` (timeout 20 s) and wait on a semaphore: it is called on Raven's worker threads, which may block; HTTP status outside 200..299, more than 32 MiB, and ATS errors (`NSURLErrorAppTransportSecurityRequiresSecureConnection`) are failures with a message in `*error` | a static Java helper (`HttpURLConnection`, redirects followed, timeouts) returning a `byte[]`; the worker thread is not attached to the JavaVM: `AttachCurrentThread` on first use and detach when the thread ends (`pthread_key_create` destructor), and use a global reference to the class taken in `init` (`FindClass` on a worker thread sees only the system's classes) |
| `clipboard_has_text` | `UIPasteboard.generalPasteboard.hasStrings` (no paste prompt) | `ClipboardManager.hasPrimaryClip()` and a `text/*` description |
| `clipboard_get` / `_set` (exist) | `UIPasteboard`; a read shows "Allow Paste?" unless it follows a paste of the user: Raven reads only on Paste, the application should too | `ClipboardManager` (through `RavenDevice.java`); Android 10+ lets only the focused app read, 12+ shows a toast after a read |

Threads on Android, in short: Raven's UI thread is `android_main`, attached to
the JavaVM by `init` (`g_android.env`). Java code that must run on the Java
main thread (views, haptics, starting `RavenRequestActivity` is fine from
either) is posted with `Activity.runOnUiThread` or the existing pipe watched
by the UI looper (`rvn_android_ui_request`). Answers come back on the Java
main thread (activity results) or a copying thread: a native method of
`RavenRequestActivity`, registered with `RegisterNatives` as `nativeOp` is
for `RavenTextView`, calls `rvn_permission_answer` / `rvn_service_complete`
directly; both are thread-safe and wake `android_main` through the task queue.

Threads on iOS: Raven's UI thread is the main thread. UIKit delegates and
`PHPicker` / `NSItemProvider` completions run on the main queue or on
background queues; call the `rvn_*` answer functions from there, no need to
hop back to the main queue.

## What the application declares

| Service | Android manifest | iOS `Info.plist` |
|---|---|---|
| Images from a URL | `INTERNET` | nothing for `https://`; `http://` needs `NSAppTransportSecurity` exceptions |
| `Rcamera_take_photo` | `CAMERA` (once declared, `ACTION_IMAGE_CAPTURE` requires it granted: Raven asks first); `uses-feature android.hardware.camera.any required="false"`; `<queries>` for `android.media.action.IMAGE_CAPTURE` | `NSCameraUsageDescription` |
| `Rphoto_pick` | nothing | nothing |
| `R_PERMISSION_PHOTOS` | `READ_MEDIA_IMAGES` (33+), `READ_EXTERNAL_STORAGE` (32 and older) | `NSPhotoLibraryUsageDescription` |
| "Save Image" in the share sheet | | `NSPhotoLibraryAddUsageDescription` (the share sheet crashes without it) |
| `R_PERMISSION_MICROPHONE` | `RECORD_AUDIO` | `NSMicrophoneUsageDescription` |
| `R_PERMISSION_LOCATION` | `ACCESS_FINE_LOCATION` or `ACCESS_COARSE_LOCATION` | `NSLocationWhenInUseUsageDescription` |
| Notifications | `POST_NOTIFICATIONS` (33+) | nothing |
| `Rhaptic_play` | `VIBRATE` (for the `Vibrator` fallback) | nothing |
| Pickers, open URL, share, clipboard | nothing | nothing |
| Raven's Java | `raven.android.RavenRequestActivity` (exported false, `Theme.Translucent.NoTitleBar`) and `raven.android.RavenFileProvider` (authority `<package>.raven.files`, exported false, `grantUriPermissions` true) | |

`examples/mobile/android/AndroidManifest.xml` and
`examples/mobile/ios/Info.plist` have all of these that the showcase uses.
iOS links more frameworks for them: AVFoundation, Photos, PhotosUI,
UniformTypeIdentifiers, UserNotifications, ImageIO, CoreLocation
(`RAVEN_LIBRARIES` of the uikit platform in `CMakeLists.txt`, the link line of
`examples/mobile/ios/build.sh`, the module map of `Raven.xcframework`).

## Testing

- `tests/test_device.c` plays a phone through the host platform
  (`raven_host.h`: `permission_status`, `permission_request`,
  `service_request`, `open_url`, `haptic`, `download`, answered with
  `Rhost_permission_answer` / `Rhost_service_complete`), then a headless
  desktop.
- `examples/mobile`: the Device tab has a button for each service and shows
  each answer; `raven_mobile --check` runs it on a desktop without a window.
