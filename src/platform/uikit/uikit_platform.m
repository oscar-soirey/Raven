/**
 * Raven - platform/uikit/uikit_platform.m
 * iOS and iPadOS: the one window is a full-screen UIView drawn through a CAMetalLayer.
 *
 * The system owns the loop (UIApplicationMain); Rrun hands the tick of the application to a CADisplayLink
 * (platform->run). Raven owns the Metal device (the backend), so the layer is made here and given to the
 * backend through RNativeWindow, as on macOS; it is a sublayer of the view, which lets it exist before the
 * application has started (Rwindow_create runs in main(), before UIApplicationMain).
 *
 *   touch     UIView touches become RVN_PEVENT_TOUCH (gestures: widget/touch.c)
 *   keyboard  the view is a UIKeyInput first responder: insertText / deleteBackward from the soft keyboard and
 *             from hardware keyboards; UIPress gives the keys themselves. The soft keyboard shows while a text
 *             widget has the focus (window_text_input): without one the input view is empty (no keyboard).
 *   layout    the layer fills the safe area, above the keyboard when it is shown (like the web build)
 *   clipboard UIPasteboard
 *
 * Not yet: marked text of input methods (UITextInput), safe area as a query instead of a margin, scenes
 * (UIScene lifecycle; the classic app delegate is used), pointer hover, drag and drop, accessibility.
 *
 * Written without an iOS SDK at hand: expect a first fix or two. Compiled without ARC, like the Cocoa platform.
 */

#import <UIKit/UIKit.h>
#import <QuartzCore/CAMetalLayer.h>

#include "../platform.h"

#include <math.h>
#include <stdlib.h>

#define RVN_UIKIT_MAX_TOUCHES 16

@class RavenView;

struct rvn_pwindow_t {
	void* user;
	CAMetalLayer* layer;
	RavenView* view;
	/** Size of the layer in pixels, and pixels per point. */
	int width, height;
	float scale;
	int focused;
};

static struct {
	rvn_pevent_handler_t handler;
	rvn_pwindow_t* window;
	int (*tick)(void* user);
	void* tick_user;
	CADisplayLink* link;
	UIWindow* ui_window;
} g_ui;

static void rvn_uikit_send(rvn_pevent_t* event) {
	if (g_ui.handler) g_ui.handler(event);
}

static void rvn_uikit_pevent(rvn_pevent_t* event, rvn_pevent_type_t type) {
	memset(event, 0, sizeof(*event));
	event->type = type;
	event->user = g_ui.window ? g_ui.window->user : NULL;
}

/* ---------------------------------------------------------------------------
 *  Keys
 * ------------------------------------------------------------------------- */

/* USB HID usage (UIKeyboardHIDUsage) to RKey */
static RKey rvn_uikit_key(long usage) {
	if (usage >= 0x04 && usage <= 0x1D) return (RKey)(R_KEY_A + (usage - 0x04));
	if (usage >= 0x1E && usage <= 0x26) return (RKey)(R_KEY_1 + (usage - 0x1E));
	if (usage == 0x27) return R_KEY_0;
	if (usage >= 0x3A && usage <= 0x45) return (RKey)(R_KEY_F1 + (usage - 0x3A));
	if (usage >= 0x59 && usage <= 0x61) return (RKey)(R_KEY_KP_1 + (usage - 0x59));
	switch (usage) {
		case 0x28: return R_KEY_ENTER;
		case 0x29: return R_KEY_ESCAPE;
		case 0x2A: return R_KEY_BACKSPACE;
		case 0x2B: return R_KEY_TAB;
		case 0x2C: return R_KEY_SPACE;
		case 0x2D: return R_KEY_MINUS;
		case 0x2E: return R_KEY_EQUAL;
		case 0x2F: return R_KEY_LEFT_BRACKET;
		case 0x30: return R_KEY_RIGHT_BRACKET;
		case 0x31: return R_KEY_BACKSLASH;
		case 0x33: return R_KEY_SEMICOLON;
		case 0x34: return R_KEY_APOSTROPHE;
		case 0x35: return R_KEY_GRAVE;
		case 0x36: return R_KEY_COMMA;
		case 0x37: return R_KEY_PERIOD;
		case 0x38: return R_KEY_SLASH;
		case 0x39: return R_KEY_CAPS_LOCK;
		case 0x46: return R_KEY_PRINT_SCREEN;
		case 0x48: return R_KEY_PAUSE;
		case 0x49: return R_KEY_INSERT;
		case 0x4A: return R_KEY_HOME;
		case 0x4B: return R_KEY_PAGE_UP;
		case 0x4C: return R_KEY_DELETE;
		case 0x4D: return R_KEY_END;
		case 0x4E: return R_KEY_PAGE_DOWN;
		case 0x4F: return R_KEY_RIGHT;
		case 0x50: return R_KEY_LEFT;
		case 0x51: return R_KEY_DOWN;
		case 0x52: return R_KEY_UP;
		case 0x54: return R_KEY_KP_DIVIDE;
		case 0x55: return R_KEY_KP_MULTIPLY;
		case 0x56: return R_KEY_KP_SUBTRACT;
		case 0x57: return R_KEY_KP_ADD;
		case 0x58: return R_KEY_KP_ENTER;
		case 0x62: return R_KEY_KP_0;
		case 0x63: return R_KEY_KP_DECIMAL;
		case 0x65: return R_KEY_MENU;
		case 0xE0: return R_KEY_LEFT_CTRL;
		case 0xE1: return R_KEY_LEFT_SHIFT;
		case 0xE2: return R_KEY_LEFT_ALT;
		case 0xE3: return R_KEY_LEFT_SUPER;
		case 0xE4: return R_KEY_RIGHT_CTRL;
		case 0xE5: return R_KEY_RIGHT_SHIFT;
		case 0xE6: return R_KEY_RIGHT_ALT;
		case 0xE7: return R_KEY_RIGHT_SUPER;
		default: return R_KEY_UNKNOWN;
	}
}

static uint32_t rvn_uikit_modifiers(UIKeyModifierFlags flags) {
	return ((flags & UIKeyModifierShift) ? R_MOD_SHIFT : 0) | ((flags & UIKeyModifierControl) ? R_MOD_CTRL : 0) |
		((flags & UIKeyModifierAlternate) ? R_MOD_ALT : 0) | ((flags & UIKeyModifierCommand) ? R_MOD_SUPER : 0);
}

/* ---------------------------------------------------------------------------
 *  The view
 * ------------------------------------------------------------------------- */

@interface RavenView : UIView <UIKeyInput> {
@public
	rvn_pwindow_t* ravenWindow;
	UITouch* touchSlots[RVN_UIKIT_MAX_TOUCHES];
	BOOL textActive;
	CGFloat keyboardTop;
	UIView* emptyInput;
	RKey lastHardKey;
	NSTimeInterval lastHardKeyTime;
}
- (void)setTextActive:(BOOL)active;
- (void)applyLayout;
@end

@implementation RavenView

- (instancetype)initWithFrame:(CGRect)frame {
	self = [super initWithFrame:frame];
	if (self) {
		self.multipleTouchEnabled = YES;
		self.opaque = YES;
		self.backgroundColor = [UIColor colorWithRed:0.106 green:0.114 blue:0.133 alpha:1.0];
		self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
		emptyInput = [[UIView alloc] initWithFrame:CGRectZero];
		keyboardTop = CGFLOAT_MAX;
		self.inputAssistantItem.leadingBarButtonGroups = @[];
		self.inputAssistantItem.trailingBarButtonGroups = @[];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardChanged:)
			name:UIKeyboardWillChangeFrameNotification object:nil];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardHidden:)
			name:UIKeyboardWillHideNotification object:nil];
	}
	return self;
}

- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[emptyInput release];
	[super dealloc];
}

/* ---- layout: the safe area, above the keyboard ---- */

- (void)keyboardChanged:(NSNotification*)note {
	CGRect screen = [[note.userInfo objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
	CGRect inWindow = [self.window convertRect:screen fromWindow:nil];
	CGRect inView = [self convertRect:inWindow fromView:self.window];
	keyboardTop = CGRectGetMinY(inView);
	[self applyLayout];
}

- (void)keyboardHidden:(NSNotification*)note {
	(void)note;
	keyboardTop = CGFLOAT_MAX;
	[self applyLayout];
}

- (void)applyLayout {
	rvn_pwindow_t* window = ravenWindow;
	UIEdgeInsets safe = self.safeAreaInsets;
	CGRect bounds = self.bounds;
	CGFloat left = safe.left, top = safe.top, right = safe.right, bottom = safe.bottom;
	CGFloat scale = [UIScreen mainScreen].nativeScale;
	CGRect frame;
	int width, height;
	if (!window) return;
	/* the keyboard covers the bottom of the view: the window ends where it starts */
	if (keyboardTop < bounds.size.height) bottom = MAX(0.0, bounds.size.height - keyboardTop);
	frame = CGRectMake(left, top, MAX(bounds.size.width - left - right, 1.0), MAX(bounds.size.height - top - bottom, 1.0));
	width = (int)lround(frame.size.width * scale);
	height = (int)lround(frame.size.height * scale);
	[CATransaction begin];
	[CATransaction setDisableActions:YES];
	window->layer.frame = frame;
	window->layer.contentsScale = scale;
	[CATransaction commit];
	if (width == window->width && height == window->height && fabsf((float)scale - window->scale) < 0.001f) return;
	window->width = width;
	window->height = height;
	window->scale = (float)scale;
	{
		rvn_pevent_t event;
		rvn_uikit_pevent(&event, RVN_PEVENT_RESIZE);
		event.width = width;
		event.height = height;
		event.scale = (float)scale;
		rvn_uikit_send(&event);
	}
}

- (void)layoutSubviews {
	[super layoutSubviews];
	[self applyLayout];
}

- (void)safeAreaInsetsDidChange {
	[super safeAreaInsetsDidChange];
	[self applyLayout];
}

/* ---- touch ---- */

- (int)slotForTouch:(UITouch*)touch create:(BOOL)create {
	int i, free_slot = -1;
	for (i = 0; i < RVN_UIKIT_MAX_TOUCHES; i++) {
		if (touchSlots[i] == touch) return i;
		if (!touchSlots[i] && free_slot < 0) free_slot = i;
	}
	if (create && free_slot >= 0) {
		touchSlots[free_slot] = touch;
		return free_slot;
	}
	return -1;
}

- (void)sendTouches:(NSSet<UITouch*>*)touches phase:(int)phase {
	rvn_pwindow_t* window = ravenWindow;
	UITouch* touch;
	if (!window) return;
	for (touch in touches) {
		int slot = [self slotForTouch:touch create:phase == R_TOUCH_DOWN];
		CGPoint point;
		rvn_pevent_t event;
		if (slot < 0) continue;
		/* the layer is inset from the view: positions are those of the window */
		point = [touch locationInView:self];
		point.x -= window->layer.frame.origin.x;
		point.y -= window->layer.frame.origin.y;
		if (touch.type == UITouchTypePencil) {
			/* the Apple Pencil: a pen with pressure and tilt (it does not hover here), not a finger */
			CGFloat force = touch.maximumPossibleForce > 0.0 ? touch.force / touch.maximumPossibleForce : 1.0;
			CGFloat tilt = M_PI_2 - touch.altitudeAngle;
			CGFloat azimuth = [touch azimuthAngleInView:self];
			rvn_uikit_pevent(&event, RVN_PEVENT_PEN);
			event.touch_id = slot;
			event.phase = phase == R_TOUCH_DOWN ? R_PEN_DOWN : phase == R_TOUCH_MOVE ? R_PEN_MOVE : R_PEN_UP;
			event.fx = (float)(point.x * window->scale);
			event.fy = (float)(point.y * window->scale);
			event.pressure = (float)force;
			event.tilt_x = (float)(tilt * cos(azimuth) * 180.0 / M_PI);
			event.tilt_y = (float)(tilt * sin(azimuth) * 180.0 / M_PI);
			rvn_uikit_send(&event);
		} else {
			rvn_uikit_pevent(&event, RVN_PEVENT_TOUCH);
			event.touch_id = slot;
			event.phase = phase;
			event.fx = (float)(point.x * window->scale);
			event.fy = (float)(point.y * window->scale);
			event.pressure = touch.maximumPossibleForce > 0.0 ? (float)(touch.force / touch.maximumPossibleForce) : 1.0f;
			rvn_uikit_send(&event);
		}
		if (phase == R_TOUCH_UP || phase == R_TOUCH_CANCEL) touchSlots[slot] = nil;
	}
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
	(void)event;
	[self sendTouches:touches phase:R_TOUCH_DOWN];
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
	(void)event;
	[self sendTouches:touches phase:R_TOUCH_MOVE];
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
	(void)event;
	[self sendTouches:touches phase:R_TOUCH_UP];
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
	(void)event;
	[self sendTouches:touches phase:R_TOUCH_CANCEL];
}

/* ---- keyboard ---- */

- (BOOL)canBecomeFirstResponder {
	return YES;
}

/* without a text widget in focus the "keyboard" is an empty view of no height: nothing appears, hardware keys still arrive */
- (UIView*)inputView {
	return textActive ? nil : emptyInput;
}

- (void)setTextActive:(BOOL)active {
	if (textActive == active) return;
	textActive = active;
	[self reloadInputViews];
}

- (UITextAutocorrectionType)autocorrectionType { return UITextAutocorrectionTypeNo; }
- (UITextAutocapitalizationType)autocapitalizationType { return UITextAutocapitalizationTypeNone; }
- (UITextSpellCheckingType)spellCheckingType { return UITextSpellCheckingTypeNo; }
- (UITextSmartQuotesType)smartQuotesType { return UITextSmartQuotesTypeNo; }
- (UITextSmartDashesType)smartDashesType { return UITextSmartDashesTypeNo; }
- (UIKeyboardType)keyboardType { return UIKeyboardTypeDefault; }

/* Raven keeps the text: there is always something to delete, so that Backspace is always reported */
- (BOOL)hasText {
	return YES;
}

- (void)sendKey:(RKey)key {
	rvn_pevent_t event;
	if (!ravenWindow) return;
	rvn_uikit_pevent(&event, RVN_PEVENT_KEY);
	event.key = key;
	event.pressed = R_TRUE;
	rvn_uikit_send(&event);
	event.pressed = R_FALSE;
	rvn_uikit_send(&event);
}

/* A hardware key that also arrives as text or a deletion (Return, Backspace) was already sent by pressesBegan. */
- (BOOL)alreadySent:(RKey)key {
	if (lastHardKey == key && [NSProcessInfo processInfo].systemUptime - lastHardKeyTime < 0.25) {
		lastHardKey = R_KEY_UNKNOWN;
		return YES;
	}
	return NO;
}

- (void)insertText:(NSString*)text {
	rvn_pevent_t event;
	const char* utf8;
	const unsigned char* s;
	if (!ravenWindow) return;
	if ([text isEqualToString:@"\n"]) {
		if (![self alreadySent:R_KEY_ENTER]) [self sendKey:R_KEY_ENTER];
		return;
	}
	utf8 = [text UTF8String];
	s = (const unsigned char*)(utf8 ? utf8 : "");
	while (*s) {
		uint32_t cp;
		int length, i;
		if (s[0] < 0x80) { cp = s[0]; length = 1; }
		else if ((s[0] & 0xE0) == 0xC0) { cp = s[0] & 0x1F; length = 2; }
		else if ((s[0] & 0xF0) == 0xE0) { cp = s[0] & 0x0F; length = 3; }
		else if ((s[0] & 0xF8) == 0xF0) { cp = s[0] & 0x07; length = 4; }
		else { s++; continue; }
		for (i = 1; i < length && (s[i] & 0xC0) == 0x80; i++) cp = (cp << 6) | (s[i] & 0x3F);
		s += i;
		if (cp < 0x20 || cp == 0x7F) continue;
		rvn_uikit_pevent(&event, RVN_PEVENT_TEXT);
		event.codepoint = cp;
		rvn_uikit_send(&event);
	}
}

- (void)deleteBackward {
	if (![self alreadySent:R_KEY_BACKSPACE]) [self sendKey:R_KEY_BACKSPACE];
}

- (void)sendPresses:(NSSet<UIPress*>*)presses pressed:(BOOL)pressed {
	/* UIKey: iOS 13.4 */
	if (@available(iOS 13.4, *)) {
		UIPress* press;
		for (press in presses) {
			UIKey* key = press.key;
			rvn_pevent_t event;
			RKey code;
			if (!key || !ravenWindow) continue;
			code = rvn_uikit_key((long)key.keyCode);
			if (code == R_KEY_UNKNOWN) continue;
			rvn_uikit_pevent(&event, RVN_PEVENT_KEY);
			event.key = code;
			event.pressed = pressed;
			event.modifiers = rvn_uikit_modifiers(key.modifierFlags);
			rvn_uikit_send(&event);
			if (pressed && (code == R_KEY_BACKSPACE || code == R_KEY_ENTER)) {
				lastHardKey = code;
				lastHardKeyTime = [NSProcessInfo processInfo].systemUptime;
			}
		}
	}
}

- (void)pressesBegan:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event {
	[self sendPresses:presses pressed:YES];
	[super pressesBegan:presses withEvent:event];
}

- (void)pressesEnded:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event {
	[self sendPresses:presses pressed:NO];
	[super pressesEnded:presses withEvent:event];
}

- (void)pressesCancelled:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event {
	[self sendPresses:presses pressed:NO];
	[super pressesCancelled:presses withEvent:event];
}

@end

/* ---------------------------------------------------------------------------
 *  Application
 * ------------------------------------------------------------------------- */

@interface RavenAppDelegate : UIResponder <UIApplicationDelegate>
@property (nonatomic, retain) UIWindow* window;
@end

@interface RavenViewController : UIViewController
@end

@implementation RavenViewController
- (BOOL)prefersStatusBarHidden { return NO; }
- (UIInterfaceOrientationMask)supportedInterfaceOrientations { return UIInterfaceOrientationMaskAll; }
@end

static void rvn_uikit_focus(int focused) {
	rvn_pevent_t event;
	if (!g_ui.window) return;
	g_ui.window->focused = focused;
	rvn_uikit_pevent(&event, RVN_PEVENT_FOCUS);
	event.focused = focused;
	rvn_uikit_send(&event);
}

@implementation RavenAppDelegate

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)options {
	RavenViewController* controller;
	RavenView* view;
	(void)application;
	(void)options;
	self.window = [[[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds] autorelease];
	controller = [[[RavenViewController alloc] init] autorelease];
	view = [[[RavenView alloc] initWithFrame:self.window.bounds] autorelease];
	controller.view = view;
	self.window.rootViewController = controller;
	g_ui.ui_window = self.window;
	if (g_ui.window) {
		g_ui.window->view = view;
		view->ravenWindow = g_ui.window;
		/* the layer made by window_create: a sublayer, so it did not need the view */
		[view.layer addSublayer:g_ui.window->layer];
	}
	[self.window makeKeyAndVisible];
	[view becomeFirstResponder];
	[view applyLayout];

	g_ui.link = [CADisplayLink displayLinkWithTarget:self selector:@selector(frame:)];
	[g_ui.link addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
	return YES;
}

- (void)frame:(CADisplayLink*)link {
	if (g_ui.tick && !g_ui.tick(g_ui.tick_user)) {
		/* the application ended (Rquit, last window closed): an iOS application has no menu to quit from; leave */
		[link invalidate];
		g_ui.link = nil;
		exit(0);
	}
}

- (void)applicationDidBecomeActive:(UIApplication*)application {
	(void)application;
	g_ui.link.paused = NO;
	rvn_uikit_focus(R_TRUE);
}

- (void)applicationWillResignActive:(UIApplication*)application {
	(void)application;
	rvn_uikit_focus(R_FALSE);
}

/* the GPU may not be used in the background: no frames until the application is back */
- (void)applicationDidEnterBackground:(UIApplication*)application {
	(void)application;
	g_ui.link.paused = YES;
}

- (void)applicationWillEnterForeground:(UIApplication*)application {
	(void)application;
	g_ui.link.paused = NO;
}

@end

/* ---------------------------------------------------------------------------
 *  Platform
 * ------------------------------------------------------------------------- */

/* Fingers are less precise than a mouse: controls are made larger (about 38 pt), a tablet less than a phone */
static float rvn_uikit_default_ui_scale(void) {
	return [UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPad ? 1.2f : 1.35f;
}

static int rvn_uikit_init(rvn_pevent_handler_t handler, rvn_pregion_handler_t regions) {
	(void)regions;
	memset(&g_ui, 0, sizeof(g_ui));
	g_ui.handler = handler;
	return R_TRUE;
}

static void rvn_uikit_shutdown(void) {
	memset(&g_ui, 0, sizeof(g_ui));
}

static void* rvn_uikit_native_display(void) {
	return NULL;
}

static rvn_pwindow_t* rvn_uikit_window_create(const rvn_pwindow_desc_t* desc) {
	rvn_pwindow_t* window;
	CGRect screen = [UIScreen mainScreen].bounds;
	CGFloat scale = [UIScreen mainScreen].nativeScale;
	if (g_ui.window) return NULL;
	window = (rvn_pwindow_t*)rvn_calloc(sizeof(*window));
	window->user = desc->user;
	window->focused = R_TRUE;
	window->scale = (float)scale;
	/* the size of the screen until the view has been laid out (the application has not started yet) */
	window->width = (int)lround(screen.size.width * scale);
	window->height = (int)lround(screen.size.height * scale);
	window->layer = [[CAMetalLayer layer] retain];
	window->layer.frame = CGRectMake(0, 0, screen.size.width, screen.size.height);
	window->layer.contentsScale = scale;
	window->layer.opaque = YES;
	g_ui.window = window;
	return window;
}

static void rvn_uikit_window_destroy(rvn_pwindow_t* window) {
	if (g_ui.window == window) g_ui.window = NULL;
	if (window->view) window->view->ravenWindow = NULL;
	[window->layer removeFromSuperlayer];
	[window->layer release];
	rvn_free(window);
}

static void rvn_uikit_window_set_title(rvn_pwindow_t* window, const char* title) {
	(void)window;
	(void)title;
}

static void rvn_uikit_window_set_size(rvn_pwindow_t* window, int width, int height) {
	/* the screen decides */
	(void)window;
	(void)width;
	(void)height;
}

static void rvn_uikit_window_get_size(rvn_pwindow_t* window, int* width, int* height) {
	*width = window->width;
	*height = window->height;
}

static void rvn_uikit_window_set_min_size(rvn_pwindow_t* window, int width, int height) {
	(void)window;
	(void)width;
	(void)height;
}

static void rvn_uikit_window_set_position(rvn_pwindow_t* window, int x, int y) {
	(void)window;
	(void)x;
	(void)y;
}

static void rvn_uikit_window_get_position(rvn_pwindow_t* window, int* x, int* y) {
	(void)window;
	*x = 0;
	*y = 0;
}

static float rvn_uikit_window_get_scale(rvn_pwindow_t* window) {
	return window->scale;
}

static void rvn_uikit_window_show(rvn_pwindow_t* window, int visible) {
	(void)window;
	(void)visible;
}

static void rvn_uikit_window_set_state(rvn_pwindow_t* window, rvn_pwindow_state_t state) {
	(void)window;
	(void)state;
}

static rvn_pwindow_state_t rvn_uikit_window_get_state(rvn_pwindow_t* window) {
	(void)window;
	return RVN_PWINDOW_NORMAL;
}

static void rvn_uikit_window_begin_move(rvn_pwindow_t* window) {
	(void)window;
}

static void rvn_uikit_window_begin_resize(rvn_pwindow_t* window, rvn_pregion_t edge) {
	(void)window;
	(void)edge;
}

static void rvn_uikit_window_set_cursor(rvn_pwindow_t* window, RCursor cursor) {
	(void)window;
	(void)cursor;
}

static void rvn_uikit_window_text_input(rvn_pwindow_t* window, int active, int x, int y, int width, int height) {
	(void)x;
	(void)y;
	(void)width;
	(void)height;
	if (window->view) [window->view setTextActive:active ? YES : NO];
}

static void rvn_uikit_window_get_native(rvn_pwindow_t* window, RNativeWindow* native) {
	memset(native, 0, sizeof(*native));
	native->system = R_WINDOW_SYSTEM_UIKIT;
	native->window = (uintptr_t)window->layer;
}

static rvn_pwindow_t* rvn_uikit_child_create(rvn_pwindow_t* parent, int x, int y, int width, int height, void* user) {
	(void)parent;
	(void)x;
	(void)y;
	(void)width;
	(void)height;
	(void)user;
	return NULL;
}

static void rvn_uikit_child_set_rect(rvn_pwindow_t* child, int x, int y, int width, int height, int visible) {
	(void)child;
	(void)x;
	(void)y;
	(void)width;
	(void)height;
	(void)visible;
}

/* UIApplicationMain does not return: the application lives in the callbacks above */
static int rvn_uikit_run(int (*tick)(void* user), void* user) {
	g_ui.tick = tick;
	g_ui.tick_user = user;
	static char* arguments[] = { "raven", NULL };
	@autoreleasepool {
		UIApplicationMain(1, arguments, nil, NSStringFromClass([RavenAppDelegate class]));
	}
	return R_TRUE;
}

static void rvn_uikit_pump(double timeout_seconds) {
	/* UIKit calls the application; nothing to wait for. (An application that does not use Rrun and waits here
	 * would block UIKit: the loop of an iOS application must be Rrun.) */
	(void)timeout_seconds;
}

static void rvn_uikit_wake(void) {
}

static void rvn_uikit_clipboard_set(const char* text) {
	[UIPasteboard generalPasteboard].string = [NSString stringWithUTF8String:text ? text : ""];
}

static char* rvn_uikit_clipboard_get(void) {
	NSString* string = [UIPasteboard generalPasteboard].string;
	return string ? rvn_strdup([string UTF8String]) : NULL;
}

const rvn_platform_t* rvn_platform_uikit(void) {
	static const rvn_platform_t platform = {
		.name = "UIKit",
		.system = R_WINDOW_SYSTEM_UIKIT,
		.init = rvn_uikit_init,
		.shutdown = rvn_uikit_shutdown,
		.native_display = rvn_uikit_native_display,
		.window_create = rvn_uikit_window_create,
		.window_destroy = rvn_uikit_window_destroy,
		.window_set_title = rvn_uikit_window_set_title,
		.window_set_size = rvn_uikit_window_set_size,
		.window_get_size = rvn_uikit_window_get_size,
		.window_set_min_size = rvn_uikit_window_set_min_size,
		.window_set_position = rvn_uikit_window_set_position,
		.window_get_position = rvn_uikit_window_get_position,
		.window_get_scale = rvn_uikit_window_get_scale,
		.window_show = rvn_uikit_window_show,
		.window_set_state = rvn_uikit_window_set_state,
		.window_get_state = rvn_uikit_window_get_state,
		.window_begin_move = rvn_uikit_window_begin_move,
		.window_begin_resize = rvn_uikit_window_begin_resize,
		.window_set_cursor = rvn_uikit_window_set_cursor,
		.window_text_input = rvn_uikit_window_text_input,
		.window_get_native = rvn_uikit_window_get_native,
		.child_create = rvn_uikit_child_create,
		.child_set_rect = rvn_uikit_child_set_rect,
		.single_window = R_TRUE,
		.default_ui_scale = rvn_uikit_default_ui_scale,
		.run = rvn_uikit_run,
		.pump = rvn_uikit_pump,
		.wake = rvn_uikit_wake,
		.clipboard_set = rvn_uikit_clipboard_set,
		.clipboard_get = rvn_uikit_clipboard_get
	};
	return &platform;
}
