/*
 * Raven iOS - UIKit entry: app delegate, scene delegate, CADisplayLink.
 * Raven creates its own UIWindow on the connected scene (uikit_platform.m).
 * Everything visible comes from ../shared (showcase.c / showcase.xml).
 */

#import <UIKit/UIKit.h>
#include <stdio.h>
#include <stdlib.h>
#include "showcase.h"

static void on_error(RError code, RSeverity severity, const char* detail) {
	fprintf(stderr, "[raven] error %d severity %d: %s\n", (int)code, (int)severity, detail ? detail : "");
}

/* After the scene is connected: Raven, the system look, the shared UI. */
static int app_start(void) {
	RInitDesc desc = {0};
	Rerror_set_callback(on_error);
	desc.backend = R_BACKEND_VULKAN;
	desc.app_name = "Raven Showcase";
	if (!Rinit_ex(&desc)) {
		fprintf(stderr, "[raven] Rinit failed\n");
		return 0;
	}
	Rtheme_follow_system(R_TRUE);
	/* no exit handler: an iOS app does not quit itself */
	return showcase_create() != R_INVALID_ID;
}
/* from uikit_platform.m (no public header yet: experimental) */
void rvn_uikit_set_display_link(CADisplayLink* link);

@interface HelloAppDelegate : UIResponder <UIApplicationDelegate>
@end
@implementation HelloAppDelegate
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)options {
	(void)application; (void)options;
	return YES;
}
- (UISceneConfiguration*)application:(UIApplication*)application configurationForConnectingSceneSession:(UISceneSession*)session options:(UISceneConnectionOptions*)options {
	(void)application; (void)options;
	UISceneConfiguration* config = [[UISceneConfiguration alloc] initWithName:@"Default" sessionRole:session.role];
	config.delegateClass = NSClassFromString(@"HelloSceneDelegate");
	return config;
}
@end

@interface HelloSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property (strong, nonatomic) UIWindow* window;
@property (strong, nonatomic) CADisplayLink* link;
@property (nonatomic) int ticks;
@end
@implementation HelloSceneDelegate
- (void)scene:(UIScene*)scene willConnectToSession:(UISceneSession*)session options:(UISceneConnectionOptions*)options {
	(void)scene; (void)session; (void)options;
	if (!app_start()) return;
	self.link = [CADisplayLink displayLinkWithTarget:self selector:@selector(tick:)];
	[self.link addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
	/* Raven pauses the display link while it has nothing to draw (Rwait_frame in tick:) */
	rvn_uikit_set_display_link(self.link);
	/* debug: how many frames ran, to see the idle pause */
	[NSTimer scheduledTimerWithTimeInterval:5.0 repeats:YES block:^(NSTimer* timer) {
		(void)timer;
		fprintf(stderr, "[raven] ticks in the last 5 s: %d (display link %s)\n", self.ticks, self.link.paused ? "paused" : "running");
		self.ticks = 0;
	}];
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), dispatch_get_main_queue(), ^{ showcase_report("launch"); });
	[self debugRotate:(UIWindowScene*)scene];
	[self debugDismiss:(UIWindowScene*)scene];
}

/*
 * Testing hook: RAVEN_DISMISS=<seconds> closes the on-screen keyboard from
 * outside Raven that long after each time it is shown (endEditing: on the
 * scene's windows, as a "Done" bar or a swipe would), to test what the
 * platform does when someone else than Raven closes the keyboard.
 */
- (void)debugDismiss:(UIWindowScene*)scene {
	const char* after = getenv("RAVEN_DISMISS");
	double delay = after ? atof(after) : 0.0;
	if (delay <= 0.0) return;
	[[NSNotificationCenter defaultCenter] addObserverForName:UIKeyboardDidShowNotification object:nil queue:[NSOperationQueue mainQueue]
	                                              usingBlock:^(NSNotification* note) {
		(void)note;
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delay * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
			fprintf(stderr, "[raven] debug: endEditing (keyboard closed from outside Raven)\n");
			for (UIWindow* window in scene.windows) [window endEditing:YES];
		});
	}];
}

/*
 * Testing hook: rotates the scene without touching the simulator, for
 * screenshots. RAVEN_ROTATE is a list of orientations (portrait, landscape,
 * landscape-left, landscape-right), one every RAVEN_ROTATE_EVERY seconds
 * (default 3), the first after that delay too. From simctl:
 *   SIMCTL_CHILD_RAVEN_ROTATE=landscape,portrait xcrun simctl launch ...
 */
- (void)debugRotate:(UIWindowScene*)scene {
	const char* list = getenv("RAVEN_ROTATE");
	const char* every = getenv("RAVEN_ROTATE_EVERY");
	double step = every ? atof(every) : 3.0;
	if (!list || !*list) return;
	NSArray<NSString*>* names = [[NSString stringWithUTF8String:list] componentsSeparatedByString:@","];
	for (NSUInteger i = 0; i < names.count; i++) {
		NSString* name = names[i];
		UIInterfaceOrientationMask mask = UIInterfaceOrientationMaskPortrait;
		if ([name isEqualToString:@"landscape"] || [name isEqualToString:@"landscape-right"]) mask = UIInterfaceOrientationMaskLandscapeRight;
		else if ([name isEqualToString:@"landscape-left"]) mask = UIInterfaceOrientationMaskLandscapeLeft;
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(step * (double)(i + 1) * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
			UIWindowSceneGeometryPreferencesIOS* prefs = [[UIWindowSceneGeometryPreferencesIOS alloc] initWithInterfaceOrientations:mask];
			fprintf(stderr, "[raven] debug: request %s\n", name.UTF8String);
			[scene requestGeometryUpdateWithPreferences:prefs errorHandler:^(NSError* error) {
				fprintf(stderr, "[raven] debug: rotation refused: %s\n", error.localizedDescription.UTF8String);
			}];
			dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
				showcase_report(name.UTF8String);
			});
		});
	}
}
- (void)tick:(CADisplayLink*)link {
	static int frames;
	self.ticks++;
	/* waits for nothing: the platform pauses the display link instead (idle) */
	if (!Rwait_frame(-1.0)) {
		[link invalidate];
		fprintf(stderr, "[raven] Rdraw_frame returned false\n");
		return;
	}
	if (++frames == 60) fprintf(stderr, "[raven] 60 frames drawn\n");
}
@end

int main(int argc, char* argv[]) {
	@autoreleasepool {
		return UIApplicationMain(argc, argv, nil, NSStringFromClass([HelloAppDelegate class]));
	}
}
