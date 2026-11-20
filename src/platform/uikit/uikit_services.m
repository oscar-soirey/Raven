/**
 * Raven - platform/uikit/uikit_services.m
 * The services of the device on iOS (raven_device.h): permissions,
 * notifications, the document picker, the camera, the photo picker, haptics,
 * the share sheet, opening URLs, downloads, the clipboard. The contract
 * (threads, answers, the copies of picked files) is docs/device-services.md;
 * the entries are documented in platform.h.
 *
 * Every entry is called on the main thread (Raven's UI thread on iOS) and
 * returns at once. A screen of the system is presented over Raven's window;
 * its delegate (a RavenDeviceRequest) answers the core once with
 * rvn_service_complete, from the main thread or from the queue that copied
 * the files. Picked files are copied into <Caches>/raven/picked/<id>/, photos
 * taken into <Caches>/raven/camera/; both are emptied at the next start.
 *
 * RAVEN_DEVICE_LOG=1 in the environment logs what is asked of the system
 * (haptics play nothing in the Simulator: this shows they were asked).
 *
 * Frameworks: AVFoundation, Photos, PhotosUI, UniformTypeIdentifiers,
 * UserNotifications, ImageIO, CoreLocation (CMakeLists.txt). Compiled with ARC.
 */

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#import <Photos/Photos.h>
#import <PhotosUI/PhotosUI.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <UserNotifications/UserNotifications.h>
#import <ImageIO/ImageIO.h>
#import <CoreLocation/CoreLocation.h>
#include <stdatomic.h>

#include "uikit_internal.h"

static int rvn_uikit_device_log(void) {
	static int state = -1;
	if (state < 0) {
		const char* value = getenv("RAVEN_DEVICE_LOG");
		state = value && value[0] && value[0] != '0';
	}
	return state;
}
#define RVN_DEVICE_LOG(...) do { if (rvn_uikit_device_log()) NSLog(__VA_ARGS__); } while (0)

/* ===========================================================================
 *  The cache: <Caches>/raven/<folder>
 * ========================================================================= */

static NSString* rvn_uikit_cache(NSString* folder) {
	NSString* caches = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES).firstObject;
	if (!caches) caches = NSTemporaryDirectory();
	caches = [caches stringByAppendingPathComponent:@"raven"];
	return folder.length ? [caches stringByAppendingPathComponent:folder] : caches;
}

/* <Caches>/raven/<folder>/<id>, made */
static NSString* rvn_uikit_cache_for(NSString* folder, uint32_t id) {
	NSString* path = [rvn_uikit_cache(folder) stringByAppendingPathComponent:[NSString stringWithFormat:@"%u", id]];
	[[NSFileManager defaultManager] createDirectoryAtPath:path withIntermediateDirectories:YES attributes:nil error:NULL];
	return path;
}

/* `name` in `folder`, as "name 2.ext" when taken (several files of the same name) */
static NSString* rvn_uikit_unique(NSString* folder, NSString* name) {
	NSFileManager* files = [NSFileManager defaultManager];
	NSString* path;
	int n;
	if (!name.length) name = @"file";
	path = [folder stringByAppendingPathComponent:name];
	for (n = 2; [files fileExistsAtPath:path]; n++) {
		NSString* stem = [name stringByDeletingPathExtension];
		NSString* extension = name.pathExtension;
		NSString* other = extension.length ? [NSString stringWithFormat:@"%@ %d.%@", stem, n, extension] : [NSString stringWithFormat:@"%@ %d", stem, n];
		path = [folder stringByAppendingPathComponent:other];
	}
	return path;
}

static void rvn_uikit_remove_later(NSString* path) {
	if (!path) return;
	dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{
		[[NSFileManager defaultManager] removeItemAtPath:path error:NULL];
	});
}

/*
 * The copies of the last run go (the header promises they last until the
 * next start at most). They are moved aside at once (a pick may come soon,
 * into the same folders) and removed on a queue: a 4 GB video must not
 * delay the start.
 */
static void rvn_uikit_cache_clean(void) {
	NSFileManager* files = [NSFileManager defaultManager];
	NSString* root = rvn_uikit_cache(nil);
	NSString* trash = [root stringByAppendingPathComponent:[@"trash-" stringByAppendingString:[NSUUID UUID].UUIDString]];
	for (NSString* folder in @[ @"picked", @"camera", @"shared", @"saving" ]) {
		NSString* path = [root stringByAppendingPathComponent:folder];
		if (![files fileExistsAtPath:path]) continue;
		[files createDirectoryAtPath:trash withIntermediateDirectories:YES attributes:nil error:NULL];
		[files moveItemAtPath:path toPath:[trash stringByAppendingPathComponent:folder] error:NULL];
	}
	dispatch_async(dispatch_get_global_queue(QOS_CLASS_BACKGROUND, 0), ^{
		NSFileManager* manager = [NSFileManager defaultManager];
		/* (the trash of a run that was killed while it emptied it, too) */
		for (NSString* name in [manager contentsOfDirectoryAtPath:root error:NULL]) {
			if ([name hasPrefix:@"trash-"]) [manager removeItemAtPath:[root stringByAppendingPathComponent:name] error:NULL];
		}
	});
}

static NSString* rvn_uikit_mime_of(NSString* path) {
	UTType* type = path.pathExtension.length ? [UTType typeWithFilenameExtension:path.pathExtension] : nil;
	return type.preferredMIMEType;
}

static uint64_t rvn_uikit_size_of(NSString* path) {
	NSDictionary* attributes = [[NSFileManager defaultManager] attributesOfItemAtPath:path error:NULL];
	return attributes ? attributes.fileSize : 0;
}

/* A file the core can read (`path`, absolute or in the bundle) as an absolute path: a copy in <Caches>/raven/<folder>/<id>/ when it is not one. */
static NSString* rvn_uikit_absolute_file(const char* path, NSString* folder, uint32_t id, NSString* name) {
	NSString* string = [NSString stringWithUTF8String:path];
	char* content;
	size_t size = 0;
	NSString* copy;
	BOOL wrote;
	if (string.isAbsolutePath && [[NSFileManager defaultManager] isReadableFileAtPath:string]) return string;
	/* SHORTCUT: read whole, on the calling thread (the files of the bundle a share or an export names are small) */
	content = rvn_read_file(path, &size);
	if (!content) return nil;
	copy = rvn_uikit_unique(rvn_uikit_cache_for(folder, id), name.length ? name : string.lastPathComponent);
	wrote = [[NSData dataWithBytesNoCopy:content length:size freeWhenDone:NO] writeToFile:copy atomically:NO];
	rvn_free(content);
	return wrote ? copy : nil;
}

/* ===========================================================================
 *  Permissions
 * ========================================================================= */

/* the status of notifications, an RPermissionStatus: fetched at init and on each return to the foreground (the center answers asynchronously) */
static _Atomic int g_notify_status = R_PERMISSION_NOT_ASKED;

static int rvn_uikit_notify_status_of(UNAuthorizationStatus status) {
	switch (status) {
	case UNAuthorizationStatusAuthorized: return R_PERMISSION_GRANTED;
	/* (App Clips: allowed for a while) */
	case UNAuthorizationStatusEphemeral: return R_PERMISSION_GRANTED;
	case UNAuthorizationStatusProvisional: return R_PERMISSION_LIMITED;
	/* (iOS never asks twice) */
	case UNAuthorizationStatusDenied: return R_PERMISSION_BLOCKED;
	case UNAuthorizationStatusNotDetermined:
	default: return R_PERMISSION_NOT_ASKED;
	}
}

/* fetches the status of notifications; `then` (or nil) runs with it, on the center's queue */
static void rvn_uikit_notify_refresh(void (^then)(int status)) {
	[[UNUserNotificationCenter currentNotificationCenter] getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings* settings) {
		int status = rvn_uikit_notify_status_of(settings.authorizationStatus);
		atomic_store(&g_notify_status, status);
		RVN_DEVICE_LOG(@"Raven device: notifications are %s", Rpermission_status_to_string((RPermissionStatus)status));
		if (then) then(status);
	}];
}

static int rvn_uikit_av_status(AVMediaType media) {
	switch ([AVCaptureDevice authorizationStatusForMediaType:media]) {
	case AVAuthorizationStatusAuthorized: return R_PERMISSION_GRANTED;
	case AVAuthorizationStatusDenied:
	case AVAuthorizationStatusRestricted: return R_PERMISSION_BLOCKED;
	case AVAuthorizationStatusNotDetermined:
	default: return R_PERMISSION_NOT_ASKED;
	}
}

static int rvn_uikit_photos_status(PHAuthorizationStatus status) {
	switch (status) {
	case PHAuthorizationStatusAuthorized: return R_PERMISSION_GRANTED;
	case PHAuthorizationStatusLimited: return R_PERMISSION_LIMITED;
	case PHAuthorizationStatusDenied:
	case PHAuthorizationStatusRestricted: return R_PERMISSION_BLOCKED;
	case PHAuthorizationStatusNotDetermined:
	default: return R_PERMISSION_NOT_ASKED;
	}
}

/*
 * Location: the permission only (Raven has no API that reads the position).
 * A CLLocationManager made on the main thread, kept, with a delegate: its
 * question is answered in locationManagerDidChangeAuthorization:, which iOS
 * also calls once when the manager is made, and on every change made in the
 * Settings. Only the location while in use is asked for. Denied is blocked:
 * iOS does not ask again (the Settings of the app can change it).
 */
@interface RavenLocationDelegate : NSObject <CLLocationManagerDelegate> {
@public
	BOOL asking;
}
@end

static CLLocationManager* g_location;
static RavenLocationDelegate* g_location_delegate;

static int rvn_uikit_location_status(CLLocationManager* manager) {
	switch (manager.authorizationStatus) {
	case kCLAuthorizationStatusAuthorizedAlways:
	case kCLAuthorizationStatusAuthorizedWhenInUse:
		/* the approximate location only: allowed in part, as Android's COARSE without FINE */
		return manager.accuracyAuthorization == CLAccuracyAuthorizationReducedAccuracy ? R_PERMISSION_LIMITED : R_PERMISSION_GRANTED;
	case kCLAuthorizationStatusDenied:
	case kCLAuthorizationStatusRestricted: return R_PERMISSION_BLOCKED;
	case kCLAuthorizationStatusNotDetermined:
	default: return R_PERMISSION_NOT_ASKED;
	}
}

@implementation RavenLocationDelegate
- (void)locationManagerDidChangeAuthorization:(CLLocationManager*)manager {
	int status = rvn_uikit_location_status(manager);
	/* (the call made when the manager is made, while nothing is decided yet, is no answer) */
	if (!asking || status == R_PERMISSION_NOT_ASKED) return;
	asking = NO;
	rvn_permission_answer(R_PERMISSION_LOCATION, status);
}
@end

/* (main thread) */
static CLLocationManager* rvn_uikit_location(void) {
	if (!g_location) {
		g_location_delegate = [[RavenLocationDelegate alloc] init];
		g_location = [[CLLocationManager alloc] init];
		g_location.delegate = g_location_delegate;
	}
	return g_location;
}

/*
 * The Info.plist of the application declares why it asks (NSCameraUsageDescription...):
 * without it iOS kills the application when it asks, so Raven does not ask
 * (R_PERMISSION_UNSUPPORTED, with a warning once).
 */
static BOOL rvn_uikit_declared(int permission) {
	static int warned[8];
	NSString* key = nil;
	int index = permission - (int)R_PERMISSION_CAMERA;
	switch (permission) {
	case R_PERMISSION_CAMERA: key = @"NSCameraUsageDescription"; break;
	case R_PERMISSION_PHOTOS: key = @"NSPhotoLibraryUsageDescription"; break;
	case R_PERMISSION_MICROPHONE: key = @"NSMicrophoneUsageDescription"; break;
	case R_PERMISSION_LOCATION: key = @"NSLocationWhenInUseUsageDescription"; break;
	default: return YES;
	}
	if ([[NSBundle mainBundle] objectForInfoDictionaryKey:key]) return YES;
	if (index >= 0 && index < 8 && !warned[index]) {
		warned[index] = 1;
		rvn_error(R_ERROR_PLATFORM, R_SEVERITY_WARNING, "%s: the Info.plist of the application has no %s: Raven cannot ask for it",
			Rpermission_to_string((RPermission)permission), key.UTF8String);
	}
	return NO;
}

int rvn_uikit_permission_status(int permission) {
	switch (permission) {
	case R_PERMISSION_CAMERA:
		return rvn_uikit_declared(permission) ? rvn_uikit_av_status(AVMediaTypeVideo) : R_PERMISSION_UNSUPPORTED;
	case R_PERMISSION_MICROPHONE:
		return rvn_uikit_declared(permission) ? rvn_uikit_av_status(AVMediaTypeAudio) : R_PERMISSION_UNSUPPORTED;
	case R_PERMISSION_PHOTOS:
		return rvn_uikit_declared(permission) ? rvn_uikit_photos_status([PHPhotoLibrary authorizationStatusForAccessLevel:PHAccessLevelReadWrite])
		                                      : R_PERMISSION_UNSUPPORTED;
	case R_PERMISSION_NOTIFICATIONS:
		return atomic_load(&g_notify_status);
	case R_PERMISSION_LOCATION:
		return rvn_uikit_declared(permission) ? rvn_uikit_location_status(rvn_uikit_location()) : R_PERMISSION_UNSUPPORTED;
	default:
		return R_PERMISSION_UNSUPPORTED;
	}
}

static const UNAuthorizationOptions g_notify_options = UNAuthorizationOptionAlert | UNAuthorizationOptionSound | UNAuthorizationOptionBadge;

/* (the completion handlers run on queues of the system: the answer goes from there) */
void rvn_uikit_permission_request(int permission) {
	RVN_DEVICE_LOG(@"Raven device: ask for %s", Rpermission_to_string((RPermission)permission));
	switch (permission) {
	case R_PERMISSION_CAMERA:
	case R_PERMISSION_MICROPHONE: {
		AVMediaType media = permission == R_PERMISSION_CAMERA ? AVMediaTypeVideo : AVMediaTypeAudio;
		if (!rvn_uikit_declared(permission)) break;
		[AVCaptureDevice requestAccessForMediaType:media completionHandler:^(BOOL granted) {
			(void)granted;
			rvn_permission_answer(permission, rvn_uikit_av_status(media));
		}];
		return;
	}
	case R_PERMISSION_PHOTOS:
		if (!rvn_uikit_declared(permission)) break;
		[PHPhotoLibrary requestAuthorizationForAccessLevel:PHAccessLevelReadWrite handler:^(PHAuthorizationStatus status) {
			rvn_permission_answer(permission, rvn_uikit_photos_status(status));
		}];
		return;
	case R_PERMISSION_NOTIFICATIONS:
		[[UNUserNotificationCenter currentNotificationCenter] requestAuthorizationWithOptions:g_notify_options
		                                                                    completionHandler:^(BOOL granted, NSError* error) {
			(void)granted;
			if (error) RVN_DEVICE_LOG(@"Raven device: notifications: %@", error.localizedDescription);
			/* (the status as it is now: provisional, or a no) */
			rvn_uikit_notify_refresh(^(int status) { rvn_permission_answer(R_PERMISSION_NOTIFICATIONS, status); });
		}];
		return;
	case R_PERMISSION_LOCATION: {
		CLLocationManager* manager;
		if (!rvn_uikit_declared(permission)) break;
		manager = rvn_uikit_location();
		/* decided already: iOS shows nothing and calls no delegate, the answer is the status */
		if (rvn_uikit_location_status(manager) != R_PERMISSION_NOT_ASKED) {
			rvn_permission_answer(permission, rvn_uikit_location_status(manager));
			return;
		}
		g_location_delegate->asking = YES;
		[manager requestWhenInUseAuthorization];
		return;
	}
	default:
		break;
	}
	rvn_permission_answer(permission, R_PERMISSION_UNSUPPORTED);
}

int rvn_uikit_permission_open_settings(void) {
	NSURL* url = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
	if (!url) return R_FALSE;
	RVN_DEVICE_LOG(@"Raven device: open the Settings of the app");
	[[UIApplication sharedApplication] openURL:url options:@{} completionHandler:nil];
	return R_TRUE;
}

/* ===========================================================================
 *  Notifications (Rnotification_show)
 * ========================================================================= */

/* Shows Raven's notifications while the application is in front too, and tells the core when one is tapped. */
@interface RavenNotificationDelegate : NSObject <UNUserNotificationCenterDelegate>
@end
@implementation RavenNotificationDelegate
- (void)userNotificationCenter:(UNUserNotificationCenter*)center
       willPresentNotification:(UNNotification*)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions))completionHandler {
	(void)center; (void)notification;
	completionHandler(UNNotificationPresentationOptionBanner | UNNotificationPresentationOptionList | UNNotificationPresentationOptionSound);
}
- (void)userNotificationCenter:(UNUserNotificationCenter*)center
    didReceiveNotificationResponse:(UNNotificationResponse*)response
             withCompletionHandler:(void (^)(void))completionHandler {
	(void)center;
	if ([response.actionIdentifier isEqualToString:UNNotificationDefaultActionIdentifier] &&
		[response.notification.request.identifier hasPrefix:@"raven."]) {
		dispatch_async(dispatch_get_main_queue(), ^{ rvn_notification_clicked(); });
	}
	completionHandler();
}
@end

static RavenNotificationDelegate* g_notify_delegate;
static _Atomic unsigned long g_notify_serial;

static void rvn_uikit_notify_fallback(NSString* title, NSString* body, int urgency, NSString* reason) {
	dispatch_async(dispatch_get_main_queue(), ^{
		rvn_notification_fallback(title.UTF8String, body.UTF8String, urgency, reason.UTF8String);
	});
}

/*
 * Rnotification_show: asks for the permission the first time (the answer
 * goes to Rpermission_request too), then shows it, or hands it to the
 * fallback when refused. R_FALSE at once when refused before.
 */
int rvn_uikit_notify(const char* app, const char* title, const char* body, int urgency) {
	UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
	UNMutableNotificationContent* content = [[UNMutableNotificationContent alloc] init];
	int status = atomic_load(&g_notify_status);
	NSString* identifier;
	UNNotificationRequest* request;
	NSString* the_title = [NSString stringWithUTF8String:title && *title ? title : app ? app : ""];
	NSString* the_body = [NSString stringWithUTF8String:body ? body : ""];

	if (status == R_PERMISSION_DENIED || status == R_PERMISSION_BLOCKED) return R_FALSE;
	content.title = the_title;
	content.body = the_body;
	if (urgency > 0) content.sound = [UNNotificationSound defaultSound];
	/* (low: in the list, no banner; critical alerts need an entitlement of Apple: shown as normal) */
	if (urgency == 0) content.interruptionLevel = UNNotificationInterruptionLevelPassive;
	identifier = [NSString stringWithFormat:@"raven.%lu", atomic_fetch_add(&g_notify_serial, 1) + 1];
	request = [UNNotificationRequest requestWithIdentifier:identifier content:content trigger:nil];

	void (^show)(void) = ^{
		[center addNotificationRequest:request withCompletionHandler:^(NSError* error) {
			RVN_DEVICE_LOG(@"Raven device: notification %@ %@", identifier, error ? error.localizedDescription : @"handed to the system");
			if (error) rvn_uikit_notify_fallback(the_title, the_body, urgency, error.localizedDescription);
		}];
	};
	if (status == R_PERMISSION_GRANTED || status == R_PERMISSION_LIMITED) {
		show();
		return R_TRUE;
	}
	/* not asked yet (or not known yet: then the system answers without asking) */
	[center requestAuthorizationWithOptions:g_notify_options completionHandler:^(BOOL granted, NSError* error) {
		NSString* reason = error ? error.localizedDescription : nil;
		rvn_uikit_notify_refresh(^(int now) {
			rvn_permission_answer(R_PERMISSION_NOTIFICATIONS, now);
			if (granted) show();
			else rvn_uikit_notify_fallback(the_title, the_body, urgency, reason ? reason : @"notifications are not allowed");
		});
	}];
	return R_TRUE;
}

/* ===========================================================================
 *  Requests that show a screen of the system
 * ========================================================================= */

/* (formats Raven's decoder reads as they are) */
static BOOL rvn_uikit_raven_reads(NSString* type) {
	return [type isEqualToString:UTTypeJPEG.identifier] || [type isEqualToString:UTTypePNG.identifier] ||
		[type isEqualToString:UTTypeGIF.identifier] || [type isEqualToString:UTTypeBMP.identifier];
}

/*
 * A photo the picker lent (`url`, valid during the handler only) into
 * `folder`: copied as it is when Raven reads it and it is upright, else
 * drawn upright into a JPEG (HEIC, TIFF, a rotated JPEG). NO when it could
 * not be read.
 */
static BOOL rvn_uikit_photo_copy(NSURL* url, NSString* folder, NSString* name, NSString** path, int* width, int* height) {
	CGImageSourceRef source = CGImageSourceCreateWithURL((__bridge CFURLRef)url, NULL);
	NSDictionary* properties;
	NSString* type;
	int orientation;
	BOOL done = NO;
	if (!source) return NO;
	properties = CFBridgingRelease(CGImageSourceCopyPropertiesAtIndex(source, 0, NULL));
	type = (__bridge NSString*)CGImageSourceGetType(source);
	orientation = [properties[(__bridge NSString*)kCGImagePropertyOrientation] intValue];
	*width = [properties[(__bridge NSString*)kCGImagePropertyPixelWidth] intValue];
	*height = [properties[(__bridge NSString*)kCGImagePropertyPixelHeight] intValue];
	if (rvn_uikit_raven_reads(type) && orientation <= 1) {
		*path = rvn_uikit_unique(folder, name);
		done = [[NSFileManager defaultManager] copyItemAtURL:url toURL:[NSURL fileURLWithPath:*path] error:NULL];
	} else {
		NSDictionary* options = @{
			(__bridge NSString*)kCGImageSourceCreateThumbnailFromImageAlways: @YES,
			(__bridge NSString*)kCGImageSourceCreateThumbnailWithTransform: @YES,
			(__bridge NSString*)kCGImageSourceThumbnailMaxPixelSize: @(*width > *height ? *width : *height)
		};
		CGImageRef image = CGImageSourceCreateThumbnailAtIndex(source, 0, (__bridge CFDictionaryRef)options);
		if (image) {
			CGImageDestinationRef destination;
			*path = rvn_uikit_unique(folder, [[name stringByDeletingPathExtension] stringByAppendingPathExtension:@"jpg"]);
			destination = CGImageDestinationCreateWithURL((__bridge CFURLRef)[NSURL fileURLWithPath:*path], (__bridge CFStringRef)UTTypeJPEG.identifier, 1, NULL);
			if (destination) {
				CGImageDestinationAddImage(destination, image, (__bridge CFDictionaryRef)@{ (__bridge NSString*)kCGImageDestinationLossyCompressionQuality: @0.9 });
				done = CGImageDestinationFinalize(destination);
				CFRelease(destination);
			}
			*width = (int)CGImageGetWidth(image);
			*height = (int)CGImageGetHeight(image);
			CGImageRelease(image);
		}
	}
	CFRelease(source);
	return done;
}

typedef enum rvn_uikit_service_t {
	RVN_UIKIT_OPEN,
	RVN_UIKIT_SAVE,
	RVN_UIKIT_CAMERA,
	RVN_UIKIT_PHOTOS,
	RVN_UIKIT_SHARE
} rvn_uikit_service_t;

/*
 * A request shown: the delegate of its screen, kept in g_requests until it
 * is answered (the screens hold their delegates weakly). take: YES once,
 * the first time it is answered (a screen may report its end twice:
 * cancelled and dismissed).
 */
@interface RavenDeviceRequest : NSObject <UIDocumentPickerDelegate, UIImagePickerControllerDelegate, UINavigationControllerDelegate,
	PHPickerViewControllerDelegate, UIAdaptivePresentationControllerDelegate> {
@public
	uint32_t ravenId;
	rvn_uikit_service_t kind;
	BOOL answered;
	/* file_save: the folder of the file exported (removed afterwards), its size */
	NSString* exportFolder;
	uint64_t exportSize;
}
@end

/* (main thread only) */
static NSMutableDictionary<NSNumber*, RavenDeviceRequest*>* g_requests;

static RavenDeviceRequest* rvn_uikit_request(uint32_t id, rvn_uikit_service_t kind) {
	RavenDeviceRequest* request = [[RavenDeviceRequest alloc] init];
	request->ravenId = id;
	request->kind = kind;
	if (!g_requests) g_requests = [NSMutableDictionary dictionary];
	g_requests[@(id)] = request;
	return request;
}

static void rvn_uikit_complete(uint32_t id, int result, NSString* error) {
	RVN_DEVICE_LOG(@"Raven device: request %u: %s%s%@", id, result == R_DEVICE_DONE ? "done" : result == R_DEVICE_CANCELLED ? "cancelled" : "failed",
		error ? ": " : "", error ? error : @"");
	rvn_service_complete(id, result, NULL, 0, error.UTF8String);
}

@implementation RavenDeviceRequest

/* main thread: YES the first time; the request leaves g_requests */
- (BOOL)take {
	if (answered) return NO;
	answered = YES;
	[g_requests removeObjectForKey:@(ravenId)];
	return YES;
}

/* (the document picker: a file to read, or the place of a file to save) */
- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls {
	uint32_t id = ravenId;
	(void)controller;
	if (![self take]) return;
	if (kind == RVN_UIKIT_SAVE) {
		NSString* name = urls.firstObject.lastPathComponent;
		RPickedFile file;
		memset(&file, 0, sizeof(file));
		file.name = name.UTF8String;
		file.mime_type = rvn_uikit_mime_of(name).UTF8String;
		file.size = exportSize;
		RVN_DEVICE_LOG(@"Raven device: request %u: saved as %@", id, urls.firstObject);
		rvn_service_complete(id, R_DEVICE_DONE, &file, 1, NULL);
		rvn_uikit_remove_later(exportFolder);
		return;
	}
	/* copies (asCopy: local files, no security scope) moved into the cache, off the main thread */
	dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
		@autoreleasepool {
			NSFileManager* files = [NSFileManager defaultManager];
			NSString* folder = rvn_uikit_cache_for(@"picked", id);
			NSMutableArray<NSString*>* paths = [NSMutableArray array];
			NSString* failure = nil;
			RPickedFile* picked;
			NSUInteger i;
			for (NSURL* url in urls) {
				NSString* path = rvn_uikit_unique(folder, url.lastPathComponent);
				BOOL scoped = [url startAccessingSecurityScopedResource];
				NSError* error = nil;
				BOOL done = [files moveItemAtURL:url toURL:[NSURL fileURLWithPath:path] error:&error] ||
					[files copyItemAtURL:url toURL:[NSURL fileURLWithPath:path] error:&error];
				if (scoped) [url stopAccessingSecurityScopedResource];
				if (done) [paths addObject:path];
				else failure = error.localizedDescription;
			}
			if (!paths.count) {
				rvn_uikit_complete(id, R_DEVICE_FAILED, failure ? failure : @"the file could not be copied");
				return;
			}
			picked = (RPickedFile*)calloc(paths.count, sizeof(RPickedFile));
			for (i = 0; i < paths.count; i++) {
				picked[i].path = paths[i].fileSystemRepresentation;
				picked[i].name = paths[i].lastPathComponent.UTF8String;
				picked[i].mime_type = rvn_uikit_mime_of(paths[i]).UTF8String;
				picked[i].size = rvn_uikit_size_of(paths[i]);
			}
			RVN_DEVICE_LOG(@"Raven device: request %u: %lu file(s), the first: %@", id, (unsigned long)paths.count, paths.firstObject);
			rvn_service_complete(id, R_DEVICE_DONE, picked, (int)paths.count, NULL);
			free(picked);
		}
	});
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller {
	(void)controller;
	if (![self take]) return;
	rvn_uikit_remove_later(exportFolder);
	rvn_uikit_complete(ravenId, R_DEVICE_CANCELLED, nil);
}

/* swiped down (a sheet the user dismissed without its buttons) */
- (void)presentationControllerDidDismiss:(UIPresentationController*)presentationController {
	(void)presentationController;
	if (![self take]) return;
	rvn_uikit_remove_later(exportFolder);
	rvn_uikit_complete(ravenId, R_DEVICE_CANCELLED, nil);
}

/* (the camera) */
- (void)imagePickerController:(UIImagePickerController*)picker didFinishPickingMediaWithInfo:(NSDictionary<UIImagePickerControllerInfoKey, id>*)info {
	uint32_t id = ravenId;
	UIImage* image = info[UIImagePickerControllerOriginalImage];
	if (![self take]) return;
	[picker dismissViewControllerAnimated:YES completion:nil];
	if (!image) {
		rvn_uikit_complete(id, R_DEVICE_FAILED, @"the camera gave no image");
		return;
	}
	/* drawn upright (the camera's image has an orientation that Raven's decoder would ignore), as a JPEG, off the main thread */
	dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
		@autoreleasepool {
			CGSize size = image.size;
			UIGraphicsImageRendererFormat* format = [UIGraphicsImageRendererFormat formatForTraitCollection:
				[UITraitCollection traitCollectionWithDisplayScale:image.scale]];
			UIImage* upright;
			NSData* jpeg;
			NSString* folder = rvn_uikit_cache(@"camera");
			NSString* path = [folder stringByAppendingPathComponent:[NSString stringWithFormat:@"photo-%u.jpg", id]];
			RPickedFile file;
			format.opaque = YES;
			format.preferredRange = UIGraphicsImageRendererFormatRangeStandard;
			upright = [[[UIGraphicsImageRenderer alloc] initWithSize:size format:format] imageWithActions:^(UIGraphicsImageRendererContext* context) {
				(void)context;
				[image drawInRect:CGRectMake(0, 0, size.width, size.height)];
			}];
			jpeg = UIImageJPEGRepresentation(upright, 0.9);
			[[NSFileManager defaultManager] createDirectoryAtPath:folder withIntermediateDirectories:YES attributes:nil error:NULL];
			if (!jpeg || ![jpeg writeToFile:path atomically:NO]) {
				rvn_uikit_complete(id, R_DEVICE_FAILED, @"the photo could not be written");
				return;
			}
			memset(&file, 0, sizeof(file));
			file.path = path.fileSystemRepresentation;
			file.name = path.lastPathComponent.UTF8String;
			file.mime_type = "image/jpeg";
			file.size = jpeg.length;
			file.width = (int)lround(size.width * upright.scale);
			file.height = (int)lround(size.height * upright.scale);
			rvn_service_complete(id, R_DEVICE_DONE, &file, 1, NULL);
		}
	});
}

- (void)imagePickerControllerDidCancel:(UIImagePickerController*)picker {
	if (![self take]) return;
	[picker dismissViewControllerAnimated:YES completion:nil];
	rvn_uikit_complete(ravenId, R_DEVICE_CANCELLED, nil);
}

/* (the photo picker: each photo copied as the picker lends it, on the picker's queues; the answer when all are) */
- (void)picker:(PHPickerViewController*)picker didFinishPicking:(NSArray<PHPickerResult*>*)results {
	uint32_t id = ravenId;
	dispatch_group_t group;
	NSString* folder;
	NSMutableArray* copies;
	NSUInteger i;
	if (![self take]) return;
	[picker dismissViewControllerAnimated:YES completion:nil];
	if (!results.count) {
		rvn_uikit_complete(id, R_DEVICE_CANCELLED, nil);
		return;
	}
	group = dispatch_group_create();
	folder = rvn_uikit_cache_for(@"picked", id);
	/* (one entry per photo, in the order picked: a dictionary, or the error) */
	copies = [NSMutableArray arrayWithCapacity:results.count];
	for (i = 0; i < results.count; i++) [copies addObject:[NSNull null]];
	for (i = 0; i < results.count; i++) {
		NSItemProvider* provider = results[i].itemProvider;
		NSString* type = UTTypeImage.identifier;
		NSUInteger index = i;
		for (NSString* registered in provider.registeredTypeIdentifiers) {
			if ([[UTType typeWithIdentifier:registered] conformsToType:UTTypeImage]) { type = registered; break; }
		}
		dispatch_group_enter(group);
		[provider loadFileRepresentationForTypeIdentifier:type completionHandler:^(NSURL* url, NSError* error) {
			NSString* path = nil;
			int width = 0, height = 0;
			NSString* name = provider.suggestedName.length ? provider.suggestedName : [NSString stringWithFormat:@"photo-%u-%lu", id, (unsigned long)index + 1];
			if (url.pathExtension.length) name = [name stringByAppendingPathExtension:url.pathExtension];
			if (url && rvn_uikit_photo_copy(url, folder, name, &path, &width, &height)) {
				@synchronized (copies) { copies[index] = @{ @"path": path, @"width": @(width), @"height": @(height) }; }
			} else {
				@synchronized (copies) { copies[index] = error ? error.localizedDescription : @"the photo could not be read"; }
			}
			dispatch_group_leave(group);
		}];
	}
	dispatch_group_notify(group, dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
		@autoreleasepool {
			RPickedFile* files = (RPickedFile*)calloc(copies.count, sizeof(RPickedFile));
			NSString* failure = nil;
			int count = 0;
			for (NSObject* each in copies) {
				NSDictionary* entry;
				NSString* path;
				if (![each isKindOfClass:[NSDictionary class]]) {
					if ([each isKindOfClass:[NSString class]]) failure = (NSString*)each;
					continue;
				}
				entry = (NSDictionary*)each;
				path = entry[@"path"];
				files[count].path = path.fileSystemRepresentation;
				files[count].name = path.lastPathComponent.UTF8String;
				files[count].mime_type = rvn_uikit_mime_of(path).UTF8String;
				files[count].size = rvn_uikit_size_of(path);
				files[count].width = [entry[@"width"] intValue];
				files[count].height = [entry[@"height"] intValue];
				count++;
			}
			if (count) {
				RVN_DEVICE_LOG(@"Raven device: request %u: %d photo(s), the first %s (%d x %d)", id, count, files[0].path, files[0].width, files[0].height);
				rvn_service_complete(id, R_DEVICE_DONE, files, count, NULL);
			} else {
				rvn_uikit_complete(id, R_DEVICE_FAILED, failure ? failure : @"the photos could not be read");
			}
			free(files);
		}
	});
}
@end

/*
 * Shows `controller` over Raven's window. A screen still going away (the
 * last picker, dismissed) is waited for: UIKit presents nothing over it.
 * No window: the request fails.
 */
static void rvn_uikit_show(UIViewController* controller, rvn_pwindow_t* window, RavenDeviceRequest* request, int tries) {
	UIViewController* presenter = rvn_uikit_presenter(window);
	if (request->answered) return;
	if (presenter && (presenter.presentedViewController || presenter.isBeingDismissed || presenter.isBeingPresented) && tries < 30) {
		/* SHORTCUT: polled every 0.1 s, at most 3 s, rather than following the transition */
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC / 10), dispatch_get_main_queue(), ^{
			/* (the window may be gone meanwhile: the first one then) */
			rvn_uikit_show(controller, NULL, request, tries + 1);
		});
		return;
	}
	if (!presenter || presenter.presentedViewController) {
		if ([request take]) rvn_uikit_complete(request->ravenId, R_DEVICE_FAILED, @"no window to show the screen of the system over");
		return;
	}
	[presenter presentViewController:controller animated:YES completion:nil];
}

/* The types a request's filters name (UTTypeItem: any file). */
static NSArray<UTType*>* rvn_uikit_types(const rvn_service_request_t* request) {
	NSMutableArray<UTType*>* types = [NSMutableArray array];
	int count = 0, i;
	char** extensions = rvn_service_extensions(request, &count);
	for (i = 0; i < count; i++) {
		UTType* type = [UTType typeWithFilenameExtension:[NSString stringWithUTF8String:extensions[i]]];
		if (type && ![types containsObject:type]) [types addObject:type];
	}
	rvn_service_extensions_free(extensions, count);
	if (!types.count) [types addObject:UTTypeItem];
	return types;
}

/* ---- files ---- */

void rvn_uikit_file_open(const rvn_service_request_t* request) {
	RavenDeviceRequest* delegate = rvn_uikit_request(request->id, RVN_UIKIT_OPEN);
	UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:rvn_uikit_types(request) asCopy:YES];
	picker.delegate = delegate;
	picker.allowsMultipleSelection = request->multiple ? YES : NO;
	picker.presentationController.delegate = delegate;
	RVN_DEVICE_LOG(@"Raven device: request %u: document picker (open%s)", request->id, request->multiple ? ", several" : "");
	rvn_uikit_show(picker, request->window, delegate, 0);
}

/*
 * The bytes (or the file) are written into <Caches>/raven/saving/<id>/ under
 * the name proposed, off the main thread, then the document picker exports
 * that file where the user chooses; the copy goes afterwards.
 */
void rvn_uikit_file_save(const rvn_service_request_t* request) {
	uint32_t id = request->id;
	RavenDeviceRequest* delegate = rvn_uikit_request(id, RVN_UIKIT_SAVE);
	NSString* name = request->file_name && *request->file_name ? [NSString stringWithUTF8String:request->file_name]
	               : request->path ? [NSString stringWithUTF8String:request->path].lastPathComponent : @"Untitled";
	NSData* data = request->data ? [NSData dataWithBytes:request->data length:request->size] : nil;
	NSString* source = nil;
	rvn_pwindow_t* window = request->window;
	name = [name stringByReplacingOccurrencesOfString:@"/" withString:@"-"];
	if (!name.pathExtension.length && request->default_extension && *request->default_extension)
		name = [name stringByAppendingPathExtension:[NSString stringWithUTF8String:request->default_extension]];
	if (!data && request->path) {
		/* (a file of the bundle is read now: the request's path is valid during the call only) */
		source = rvn_uikit_absolute_file(request->path, @"saving", id, name);
		if (!source) {
			[delegate take];
			rvn_uikit_complete(id, R_DEVICE_FAILED, [NSString stringWithFormat:@"cannot read %s", request->path]);
			return;
		}
	}
	RVN_DEVICE_LOG(@"Raven device: request %u: document picker (save %@)", id, name);
	dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
		NSString* folder = rvn_uikit_cache_for(@"saving", id);
		NSString* path = [folder stringByAppendingPathComponent:name];
		NSError* error = nil;
		BOOL done;
		/* (a file of the bundle was copied there already) */
		if (!data && [source isEqualToString:path]) done = YES;
		else {
			[[NSFileManager defaultManager] removeItemAtPath:path error:NULL];
			if (data) done = [data writeToFile:path options:0 error:&error];
			else done = [[NSFileManager defaultManager] copyItemAtPath:source toPath:path error:&error];
		}
		/* (UIKit throws when the file to export does not exist) */
		if (done && ![[NSFileManager defaultManager] fileExistsAtPath:path]) done = NO;
		dispatch_async(dispatch_get_main_queue(), ^{
			UIDocumentPickerViewController* picker;
			if (!done) {
				if ([delegate take]) rvn_uikit_complete(id, R_DEVICE_FAILED, error ? error.localizedDescription : @"the file could not be written");
				rvn_uikit_remove_later(folder);
				return;
			}
			delegate->exportFolder = folder;
			delegate->exportSize = rvn_uikit_size_of(path);
			picker = [[UIDocumentPickerViewController alloc] initForExportingURLs:@[ [NSURL fileURLWithPath:path] ] asCopy:YES];
			picker.delegate = delegate;
			picker.presentationController.delegate = delegate;
			/* (the window of the request may have gone meanwhile: it is only looked up among those alive) */
			{
				rvn_pwindow_t* alive = NULL;
				UIView* view = rvn_uikit_view(window);
				if (view && view.window) alive = window;
				rvn_uikit_show(picker, alive, delegate, 0);
			}
		});
	});
}

/* ---- the camera ---- */

int rvn_uikit_camera_available(void) {
	return [UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypeCamera] ? R_TRUE : R_FALSE;
}

void rvn_uikit_camera_take_photo(const rvn_service_request_t* request) {
	RavenDeviceRequest* delegate = rvn_uikit_request(request->id, RVN_UIKIT_CAMERA);
	UIImagePickerController* picker;
	if (!rvn_uikit_camera_available()) {
		[delegate take];
		rvn_uikit_complete(request->id, R_DEVICE_UNSUPPORTED, nil);
		return;
	}
	picker = [[UIImagePickerController alloc] init];
	picker.sourceType = UIImagePickerControllerSourceTypeCamera;
	picker.mediaTypes = @[ UTTypeImage.identifier ];
	picker.delegate = delegate;
	RVN_DEVICE_LOG(@"Raven device: request %u: camera", request->id);
	rvn_uikit_show(picker, request->window, delegate, 0);
}

/* ---- the photo picker ---- */

void rvn_uikit_photo_pick(const rvn_service_request_t* request) {
	RavenDeviceRequest* delegate = rvn_uikit_request(request->id, RVN_UIKIT_PHOTOS);
	PHPickerConfiguration* configuration = [[PHPickerConfiguration alloc] init];
	PHPickerViewController* picker;
	configuration.filter = [PHPickerFilter imagesFilter];
	configuration.selectionLimit = request->max_count > 0 ? request->max_count : 0;
	/* (JPEG rather than HEIC where the system can) */
	configuration.preferredAssetRepresentationMode = PHPickerConfigurationAssetRepresentationModeCompatible;
	picker = [[PHPickerViewController alloc] initWithConfiguration:configuration];
	picker.delegate = delegate;
	picker.presentationController.delegate = delegate;
	RVN_DEVICE_LOG(@"Raven device: request %u: photo picker (at most %d)", request->id, request->max_count);
	rvn_uikit_show(picker, request->window, delegate, 0);
}

/* ---- the share sheet ---- */

/* the share sheet is still shown: its handler said "not done" for an activity the user left, which may leave the sheet open */
static void rvn_uikit_share_check(UIActivityViewController* sheet, RavenDeviceRequest* delegate) {
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC / 2), dispatch_get_main_queue(), ^{
		if (sheet.presentingViewController && !sheet.isBeingDismissed) return;
		if ([delegate take]) rvn_uikit_complete(delegate->ravenId, R_DEVICE_CANCELLED, nil);
	});
}

void rvn_uikit_share(const rvn_service_request_t* request) {
	uint32_t id = request->id;
	RavenDeviceRequest* delegate = rvn_uikit_request(id, RVN_UIKIT_SHARE);
	NSMutableArray* items = [NSMutableArray array];
	UIActivityViewController* sheet;
	UIPopoverPresentationController* popover;
	UIView* view = rvn_uikit_view(request->window);
	__weak UIActivityViewController* weak;
	if (request->text && *request->text) [items addObject:[NSString stringWithUTF8String:request->text]];
	if (request->path && *request->path) {
		NSString* path = rvn_uikit_absolute_file(request->path, @"shared", id, nil);
		if (!path) {
			[delegate take];
			rvn_uikit_complete(id, R_DEVICE_FAILED, [NSString stringWithFormat:@"cannot read %s", request->path]);
			return;
		}
		[items addObject:[NSURL fileURLWithPath:path]];
	}
	sheet = [[UIActivityViewController alloc] initWithActivityItems:items applicationActivities:nil];
	weak = sheet;
	/* (iPad: a popover pointing at the anchor, in pixels of the window; width 0: the middle, no arrow) */
	popover = sheet.popoverPresentationController;
	if (popover && view) {
		CGFloat scale = view.contentScaleFactor > 0 ? view.contentScaleFactor : 1;
		popover.sourceView = view;
		if (request->anchor[2] > 0) {
			popover.sourceRect = CGRectMake(request->anchor[0] / scale, request->anchor[1] / scale, request->anchor[2] / scale, request->anchor[3] / scale);
		} else {
			popover.sourceRect = CGRectMake(view.bounds.size.width / 2, view.bounds.size.height / 2, 0, 0);
			popover.permittedArrowDirections = 0;
		}
	}
	sheet.completionWithItemsHandler = ^(UIActivityType activity, BOOL completed, NSArray* returned, NSError* error) {
		(void)returned;
		RVN_DEVICE_LOG(@"Raven device: request %u: share sheet: %@ %s", id, activity ? activity : @"(none)", completed ? "completed" : "not completed");
		if (completed) {
			if ([delegate take]) rvn_service_complete(id, R_DEVICE_DONE, NULL, 0, NULL);
		} else if (error) {
			if ([delegate take]) rvn_uikit_complete(id, R_DEVICE_FAILED, error.localizedDescription);
		} else if (!activity) {
			if ([delegate take]) rvn_uikit_complete(id, R_DEVICE_CANCELLED, nil);
		} else if (weak) {
			rvn_uikit_share_check(weak, delegate);
		} else if ([delegate take]) {
			rvn_uikit_complete(id, R_DEVICE_CANCELLED, nil);
		}
	};
	RVN_DEVICE_LOG(@"Raven device: request %u: share sheet (%lu item(s))", id, (unsigned long)items.count);
	rvn_uikit_show(sheet, request->window, delegate, 0);
}

/* ===========================================================================
 *  URLs, haptics, downloads, clipboard
 * ========================================================================= */

int rvn_uikit_open_url(const char* url) {
	NSString* string = [NSString stringWithUTF8String:url];
	NSURL* target = string ? [NSURL URLWithString:string] : nil;
	if (!target || !target.scheme.length) return R_FALSE;
	RVN_DEVICE_LOG(@"Raven device: open %@", target);
	[[UIApplication sharedApplication] openURL:target options:@{} completionHandler:^(BOOL success) {
		if (!success) RVN_DEVICE_LOG(@"Raven device: no application opened %@", target);
	}];
	return R_TRUE;
}

/* (main thread; kept: a generator made just before playing is late) */
static UIImpactFeedbackGenerator* g_impacts[3];
static UISelectionFeedbackGenerator* g_selection;
static UINotificationFeedbackGenerator* g_outcome;

/* An iPhone has the Taptic Engine; an iPad and the Simulator play nothing (Rhaptic_play still makes the calls, and logs them) */
int rvn_uikit_haptic_supported(void) {
#if TARGET_OS_SIMULATOR
	return R_FALSE;
#else
	return [UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPhone ? R_TRUE : R_FALSE;
#endif
}

int rvn_uikit_haptic(int haptic) {
	static const char* const names[] = { "light", "medium", "heavy", "selection", "success", "warning", "error" };
	int index = haptic - (int)R_HAPTIC_LIGHT;
	if (index < 0 || index > 6) return R_FALSE;
	RVN_DEVICE_LOG(@"Raven device: haptic %s", names[index]);
	if (index <= 2) {
		static const UIImpactFeedbackStyle styles[] = { UIImpactFeedbackStyleLight, UIImpactFeedbackStyleMedium, UIImpactFeedbackStyleHeavy };
		if (!g_impacts[index]) g_impacts[index] = [[UIImpactFeedbackGenerator alloc] initWithStyle:styles[index]];
		[g_impacts[index] impactOccurred];
		[g_impacts[index] prepare];
	} else if (index == 3) {
		if (!g_selection) g_selection = [[UISelectionFeedbackGenerator alloc] init];
		[g_selection selectionChanged];
		[g_selection prepare];
	} else {
		static const UINotificationFeedbackType types[] = { UINotificationFeedbackTypeSuccess, UINotificationFeedbackTypeWarning, UINotificationFeedbackTypeError };
		if (!g_outcome) g_outcome = [[UINotificationFeedbackGenerator alloc] init];
		[g_outcome notificationOccurred:types[index - 4]];
		[g_outcome prepare];
	}
	return R_TRUE;
}

#define RVN_UIKIT_DOWNLOAD_MAX (32u * 1024u * 1024u)

/* One session for every download, made once (any thread). */
static NSURLSession* rvn_uikit_session(void) {
	static NSURLSession* session;
	static dispatch_once_t once;
	dispatch_once(&once, ^{
		NSURLSessionConfiguration* configuration = [NSURLSessionConfiguration defaultSessionConfiguration];
		configuration.timeoutIntervalForRequest = 20.0;
		configuration.timeoutIntervalForResource = 25.0;
		session = [NSURLSession sessionWithConfiguration:configuration];
	});
	return session;
}

static char* rvn_uikit_error_text(NSString* text) {
	return rvn_strdup(text.UTF8String ? text.UTF8String : "the system did not say why");
}

/* Blocks on Raven's worker thread (never the main thread on purpose) until the data task ends. */
int rvn_uikit_download(const char* url, uint8_t** data, size_t* size, char** error) {
	@autoreleasepool {
		NSString* string = [NSString stringWithUTF8String:url];
		NSURL* target = string ? [NSURL URLWithString:string] : nil;
		NSMutableURLRequest* request;
		dispatch_semaphore_t done = dispatch_semaphore_create(0);
		__block NSData* body = nil;
		__block NSURLResponse* response = nil;
		__block NSError* failure = nil;
		NSInteger status;
		NSString* scheme = target.scheme.lowercaseString;
		*data = NULL;
		*size = 0;
		if (!target || !([scheme isEqualToString:@"https"] || [scheme isEqualToString:@"http"])) {
			*error = rvn_uikit_error_text(@"not an http:// or https:// URL");
			return R_FALSE;
		}
		RVN_DEVICE_LOG(@"Raven device: download %@", target);
		request = [NSMutableURLRequest requestWithURL:target cachePolicy:NSURLRequestUseProtocolCachePolicy timeoutInterval:20.0];
		[[rvn_uikit_session() dataTaskWithRequest:request completionHandler:^(NSData* got, NSURLResponse* answer, NSError* problem) {
			body = got;
			response = answer;
			failure = problem;
			dispatch_semaphore_signal(done);
		}] resume];
		dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);
		if (failure) {
			if (failure.code == NSURLErrorAppTransportSecurityRequiresSecureConnection)
				*error = rvn_uikit_error_text(@"blocked by App Transport Security (http:// needs NSAppTransportSecurity in the Info.plist)");
			else
				*error = rvn_uikit_error_text(failure.localizedDescription);
			return R_FALSE;
		}
		status = [response isKindOfClass:[NSHTTPURLResponse class]] ? ((NSHTTPURLResponse*)response).statusCode : 200;
		if (status < 200 || status > 299) {
			*error = rvn_uikit_error_text([NSString stringWithFormat:@"HTTP %ld", (long)status]);
			return R_FALSE;
		}
		if (body.length > RVN_UIKIT_DOWNLOAD_MAX) {
			*error = rvn_uikit_error_text([NSString stringWithFormat:@"%lu bytes, more than 32 MiB", (unsigned long)body.length]);
			return R_FALSE;
		}
		*data = (uint8_t*)rvn_alloc(body.length ? body.length : 1);
		if (body.length) memcpy(*data, body.bytes, body.length);
		*size = body.length;
		RVN_DEVICE_LOG(@"Raven device: downloaded %lu bytes from %@", (unsigned long)body.length, target);
		return R_TRUE;
	}
}

/* (hasStrings reads nothing: no "Allow Paste" question) */
int rvn_uikit_clipboard_has_text(void) {
	return [UIPasteboard generalPasteboard].hasStrings ? R_TRUE : R_FALSE;
}

/* ===========================================================================
 *  Start and end
 * ========================================================================= */

static id g_foreground_observer;

void rvn_uikit_services_init(void) {
	UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
	dispatch_semaphore_t known = dispatch_semaphore_create(0);
	rvn_uikit_cache_clean();
	/* Raven's notifications are shown in front too, unless the application has its own delegate */
	if (!center.delegate) {
		g_notify_delegate = [[RavenNotificationDelegate alloc] init];
		center.delegate = g_notify_delegate;
	}
	/*
	 * The status of notifications, asynchronous: waited for a moment, so that
	 * the first Rpermission_get_status knows it (it comes in a few
	 * milliseconds); R_PERMISSION_NOT_ASKED until then.
	 */
	atomic_store(&g_notify_status, R_PERMISSION_NOT_ASKED);
	rvn_uikit_notify_refresh(^(int status) {
		(void)status;
		dispatch_semaphore_signal(known);
	});
	dispatch_semaphore_wait(known, dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC / 4));
	/* (before R_APP_DID_RESUME, when the application reads the status again) */
	g_foreground_observer = [[NSNotificationCenter defaultCenter] addObserverForName:UIApplicationWillEnterForegroundNotification object:nil
	                                                                           queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification* note) {
		(void)note;
		rvn_uikit_notify_refresh(nil);
	}];
}

void rvn_uikit_services_shutdown(void) {
	UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
	int i;
	if (g_foreground_observer) [[NSNotificationCenter defaultCenter] removeObserver:g_foreground_observer];
	g_foreground_observer = nil;
	if (g_notify_delegate && center.delegate == g_notify_delegate) center.delegate = nil;
	g_notify_delegate = nil;
	/* (a screen still shown keeps its delegate weakly: its answer is dropped; the core ignores late answers anyway) */
	for (RavenDeviceRequest* request in g_requests.allValues) request->answered = YES;
	g_requests = nil;
	for (i = 0; i < 3; i++) g_impacts[i] = nil;
	g_selection = nil;
	g_outcome = nil;
	/* (a question still shown gets no answer: the core ignores late answers after Rshutdown anyway) */
	if (g_location) g_location.delegate = nil;
	g_location = nil;
	g_location_delegate = nil;
}
