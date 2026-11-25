/* What the Swift app needs that Raven.xcframework's module does not give:
 * the display link of the UIKit platform has no public header yet
 * (src/platform/uikit/uikit_platform.m, a SHORTCUT). Swift reads this file
 * with -import-objc-header. */
#import <QuartzCore/QuartzCore.h>

void rvn_uikit_set_display_link(CADisplayLink* link);
