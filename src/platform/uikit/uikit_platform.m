/**
 * Raven - platform/uikit/uikit_platform.m
 * iOS window system (UIKit + CAMetalLayer). SHORTCUT: the smallest table that
 * shows a full-screen Raven window and forwards fingers.
 *
 * UIKit owns the main thread: the application calls Rdraw_frame from a
 * CADisplayLink (Raven has no loop of its own). Events that UIKit delivers
 * outside of a frame (layout, touches) are queued here and handed to the
 * core in pump, inside Rdraw_frame, as the desktop platforms do.
 *
 * Each Raven window is a UIWindow on the first connected UIWindowScene, with
 * a view controller whose view carries a CAMetalLayer. The renderer is given
 * R_WINDOW_SYSTEM_UIKIT with the CAMetalLayer*: the Metal backend draws into
 * it, Vulkan makes a VK_EXT_metal_surface of it as it does for Cocoa.
 *
 * The application's lifecycle (UIApplication notifications) is handed to the
 * core at once as RVN_PEVENT_APP: in the background no frame comes, and the
 * core must have stopped drawing and drained the GPU before
 * didEnterBackground returns (the system may kill an application that uses
 * the GPU in the background). The CAMetalLayer and the swapchain are kept
 * meanwhile: nothing is presented, and they are used again on return. The
 * Metal backend is also suspended in between (rvn_metal_set_suspended), so a
 * frame already under way cannot commit a command buffer.
 * SHORTCUT: a scene that is disconnected (the system reclaims it) is not
 * handled: its windows are not moved to a new scene.
 *
 * Text selected with a finger (widget/text_touch.c): the core draws the
 * selection and its handles; the menu of Cut, Copy, Paste, Select All is
 * the system's (UIEditMenuInteraction, iOS 16+; UIMenuController on iOS 15),
 * shown by edit_menu_show over the view. See "The edit menu".
 *
 * Relative paths read the resources of the app's bundle: see "Files of the
 * bundle". The services of the device (permissions, pickers, camera,
 * haptics, sharing, notifications, downloads) are in uikit_services.m.
 *
 * Compiled with ARC (see CMakeLists.txt).
 */

#import <UIKit/UIKit.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#include <sys/stat.h>

#include "../platform.h"
#include "../apple/webkit_web_view.h"
#if defined(RAVEN_BACKEND_METAL_ENABLED)
#include "../../render/renderer.h"
#endif
#include "uikit_internal.h"
/* (SHORTCUT: the platform reads the id of the core's window from its user pointer, as the Android platform reaches the core) */
#include "../../widget/widget.h"
#include "../../core/accessibility.h"

struct rvn_pwindow_t {
	void* user;
	/* retained UIWindow*, RavenUIKitView* (CFBridgingRetain) */
	void* window;
	void* view;
	int transparent;
	struct rvn_pwindow_t* next;
	/* on-screen keyboard: the part of the view it covers, in points (0: hidden) */
	float keyboard_points;
	/* what pump saw last: Rwindow_wants_text_input and the focused widget (edge-triggered show / hide) */
	int keyboard_wanted;
	Rid keyboard_focus;
	/* the keyboard went away without Raven asking (the user or the system): pump drops Raven's focus */
	int keyboard_dismissed;
	/* VoiceOver: the nodes at the top of the window when last told (a change is a new screen) */
	uint32_t a11y_top;
};

#define RVN_UIKIT_QUEUE 256

static struct {
	rvn_pevent_handler_t handler;
	rvn_pevent_t queue[RVN_UIKIT_QUEUE];
	int count;
	/* the CADisplayLink of the application that calls Rwait_frame (rvn_uikit_set_display_link), not retained */
	__unsafe_unretained CADisplayLink* link;
	/* bumped on each resume: a timed resume of an older pause does nothing */
	unsigned pause_generation;
	/* every Raven window (pump shows and hides the keyboard for each) */
	rvn_pwindow_t* windows;
	/* the observers of the keyboard and of the scene (retained NSArray of NSNotificationCenter tokens) */
	void* observers;
	/* the last frame of the on-screen keyboard, in the screen's coordinates (keyboard_shown: on screen) */
	CGRect keyboard_frame;
	int keyboard_shown;
	/*
	 * Paste of the edit menu: the pasteboard read in paste: itself (what the
	 * user chose: no "Allow Paste" question), given to the core as the
	 * clipboard while pump hands the command over (retained NSString*).
	 */
	void* pasted;
} g_uikit;

/* main thread: the display link ticks again */
static void rvn_uikit_resume(void) {
	g_uikit.pause_generation++;
	if (g_uikit.link && g_uikit.link.paused) g_uikit.link.paused = NO;
}

static void rvn_uikit_post(const rvn_pevent_t* event) {
	if (g_uikit.count < RVN_UIKIT_QUEUE) g_uikit.queue[g_uikit.count++] = *event;
	rvn_uikit_resume();
}

/*
 * SHORTCUT (no public API yet): the application hands over the CADisplayLink
 * that calls Rwait_frame. pump then pauses it while Raven has nothing to
 * draw, and events, wake and timers start it again. NULL to stop.
 */
void rvn_uikit_set_display_link(CADisplayLink* link) {
	g_uikit.link = link;
	g_uikit.pause_generation++;
}

static float rvn_uikit_scale_of(UIView* view) {
	UIScreen* screen = view.window ? view.window.screen : nil;
	if (!screen) {
		for (UIScene* scene in [UIApplication sharedApplication].connectedScenes) {
			if ([scene isKindOfClass:[UIWindowScene class]]) { screen = ((UIWindowScene*)scene).screen; break; }
		}
	}
	return screen ? (float)screen.scale : 1.0f;
}

/*
 * The view takes the on-screen keyboard itself: it becomes the first
 * responder while Raven wants text.
 *
 * It is a UITextInput, so that the system's text services (predictive
 * suggestions, autocorrection, autocapitalization, marked text of CJK input
 * methods, dictation) can read and edit the text around the caret. When the
 * focus is in a TextInput the document is that field, read and edited
 * directly through Rwindow_get_text_input_text / _selection and
 * Rwindow_replace_text_input (UTF-16 offsets here, UTF-8 bytes in the core).
 * Otherwise (SpinBox, TextEditor) the document is empty and typing goes to
 * Raven as text and key events, as with UIKeyInput alone. The return key is
 * always Enter, sent as a key.
 */
@interface RavenUIKitView : UIView <UITextInput, UIEditMenuInteractionDelegate> {
@public
	rvn_pwindow_t* ravenWindow;
	/* pump asked for the keyboard (only then can the view be the first responder) */
	BOOL ravenWantsKeyboard;
	/* pump itself is letting the keyboard go: resignFirstResponder does not count it as the user's */
	BOOL ravenResigning;
	/*
	 * A key (Enter, Tab) is queued for the next frame: until pump hands it
	 * over, typing is queued too (as text events), so that it stays behind it
	 * (Enter may move the focus to another field).
	 */
	BOOL ravenKeysQueued;
	/* the marked text (composition) in the field, UTF-16; location NSNotFound: none. The core does not know it. */
	NSRange ravenMarked;
	/*
	 * What the system was told last of the field: pump compares it with the
	 * field and calls the input delegate when Raven changed the text or the
	 * selection itself (a tap, Select All, the application).
	 */
	NSString* ravenToldText;
	NSRange ravenToldSelection;
	Rid ravenToldFocus;
	UITextInputStringTokenizer* ravenTokenizer;
	/* the edit menu (edit_menu_show): its RVN_EDIT_* entries (0: none shown), next to this rectangle (points), and iOS 16's interaction */
	unsigned ravenMenuItems;
	CGRect ravenMenuRect;
	id ravenEditMenu;
}
@end

/* A place in the focused field's text: an offset in UTF-16 units. */
@interface RavenTextPosition : UITextPosition
@property (nonatomic, readonly) NSInteger offset;
+ (instancetype)at:(NSInteger)offset;
@end
@implementation RavenTextPosition
+ (instancetype)at:(NSInteger)offset {
	RavenTextPosition* position = [[self alloc] init];
	position->_offset = offset;
	return position;
}
@end

@interface RavenTextRange : UITextRange
@property (nonatomic, readonly) NSRange range;
+ (instancetype)with:(NSRange)range;
@end
@implementation RavenTextRange
+ (instancetype)with:(NSRange)range {
	RavenTextRange* result = [[self alloc] init];
	result->_range = range;
	return result;
}
- (UITextPosition*)start { return [RavenTextPosition at:(NSInteger)_range.location]; }
- (UITextPosition*)end { return [RavenTextPosition at:(NSInteger)NSMaxRange(_range)]; }
- (BOOL)isEmpty { return _range.length == 0; }
@end

/* One line of a selection (a field has one line). */
@interface RavenSelectionRect : UITextSelectionRect {
@public
	CGRect ravenRect;
}
@end
@implementation RavenSelectionRect
- (CGRect)rect { return ravenRect; }
- (NSWritingDirection)writingDirection { return NSWritingDirectionLeftToRight; }
- (BOOL)containsStart { return YES; }
- (BOOL)containsEnd { return YES; }
- (BOOL)isVertical { return NO; }
@end

/* RAVEN_TEXT_LOG=1 in the environment: the platform logs what the text system does (marked text, notifications) */
static int rvn_uikit_text_log(void) {
	static int state = -1;
	if (state < 0) {
		const char* value = getenv("RAVEN_TEXT_LOG");
		state = value && value[0] && value[0] != '0';
	}
	return state;
}
#define RVN_TEXT_LOG(...) do { if (rvn_uikit_text_log()) NSLog(__VA_ARGS__); } while (0)

/* A UTF-16 offset inside the string, not between the two halves of a surrogate pair. */
static NSUInteger rvn_uikit_clamp_units(NSString* text, NSInteger offset) {
	NSUInteger length = text.length, i;
	if (offset < 0) return 0;
	i = (NSUInteger)offset > length ? length : (NSUInteger)offset;
	if (i > 0 && i < length && CFStringIsSurrogateLowCharacter([text characterAtIndex:i]) &&
		CFStringIsSurrogateHighCharacter([text characterAtIndex:i - 1]))
		i--;
	return i;
}
/* UTF-16 offset -> UTF-8 byte offset */
static int rvn_uikit_bytes(NSString* text, NSInteger offset) {
	NSUInteger i = rvn_uikit_clamp_units(text, offset);
	return (int)[[text substringToIndex:i] lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
}
/* UTF-8 byte offset (on a character boundary, as the core gives them) -> UTF-16 offset */
static NSUInteger rvn_uikit_units(const char* text, int bytes) {
	NSString* prefix;
	if (!text || bytes <= 0) return 0;
	prefix = [[NSString alloc] initWithBytes:text length:(NSUInteger)bytes encoding:NSUTF8StringEncoding];
	return prefix ? prefix.length : 0;
}
static NSRange rvn_uikit_clamp_range(NSString* text, NSRange range) {
	NSUInteger start = rvn_uikit_clamp_units(text, (NSInteger)(range.location == NSNotFound ? 0 : range.location));
	NSUInteger end = rvn_uikit_clamp_units(text, (NSInteger)(range.location == NSNotFound ? 0 : range.location + range.length));
	return NSMakeRange(start, end > start ? end - start : 0);
}

/*
 * The part of the window's view the on-screen keyboard covers, from the last
 * keyboard frame UIKit announced; RVN_PEVENT_INSETS when it changed (the core
 * then asks window_get_keyboard_height).
 */
static void rvn_uikit_keyboard_measure(rvn_pwindow_t* window) {
	UIView* view = (__bridge UIView*)window->view;
	float points = 0.0f;
	if (g_uikit.keyboard_shown && view.window) {
		/* (by hand: CGRectIntersection would make every application link CoreGraphics) */
		CGRect frame = [view convertRect:g_uikit.keyboard_frame fromCoordinateSpace:view.window.screen.coordinateSpace];
		CGRect bounds = view.bounds;
		double top = frame.origin.y > bounds.origin.y ? frame.origin.y : bounds.origin.y;
		double bottom = bounds.origin.y + bounds.size.height;
		double left = frame.origin.x > bounds.origin.x ? frame.origin.x : bounds.origin.x;
		double right = frame.origin.x + frame.size.width < bounds.origin.x + bounds.size.width ? frame.origin.x + frame.size.width
		                                                                                     : bounds.origin.x + bounds.size.width;
		if (frame.origin.y + frame.size.height < bottom) bottom = frame.origin.y + frame.size.height;
		/* (the whole bottom from the keyboard's top edge down: an undocked keyboard still hides what is under it) */
		if (bottom > top && right > left) points = (float)(bounds.origin.y + bounds.size.height - top);
	}
	if (points != window->keyboard_points) {
		rvn_pevent_t e;
		memset(&e, 0, sizeof(e));
		window->keyboard_points = points;
		e.type = RVN_PEVENT_INSETS;
		e.user = window->user;
		rvn_uikit_post(&e);
	}
}

static void rvn_uikit_post_appearance(void) {
	rvn_pevent_t e;
	memset(&e, 0, sizeof(e));
	e.type = RVN_PEVENT_APPEARANCE;
	rvn_uikit_post(&e);
}

static void rvn_uikit_post_key(rvn_pwindow_t* window, RKey key) {
	rvn_pevent_t e;
	memset(&e, 0, sizeof(e));
	e.type = RVN_PEVENT_KEY;
	e.user = window->user;
	e.key = key;
	e.pressed = R_TRUE;
	rvn_uikit_post(&e);
	e.pressed = R_FALSE;
	rvn_uikit_post(&e);
}

@implementation RavenUIKitView
+ (Class)layerClass { return [CAMetalLayer class]; }

/* ---- the on-screen keyboard: first responder ---- */
- (BOOL)canBecomeFirstResponder { return ravenWindow && ravenWantsKeyboard; }
- (BOOL)resignFirstResponder {
	BOOL was = self.isFirstResponder;
	BOOL done = [super resignFirstResponder];
	/* someone else than Raven closed the keyboard (endEditing, another responder): pump drops Raven's focus */
	if (done && was && !ravenResigning && ravenWindow && ravenWantsKeyboard) {
		ravenWantsKeyboard = NO;
		ravenWindow->keyboard_dismissed = R_TRUE;
		rvn_uikit_resume();
	}
	return done;
}

/* ---- the focused field, through the core ---- */
- (Rid)ravenCore {
	rvn_window_t* core = ravenWindow ? (rvn_window_t*)ravenWindow->user : NULL;
	return core ? core->id : R_INVALID_ID;
}
/* R_TRUE when the focus is in a TextInput: the document is its text (otherwise it is empty, typing goes as events) */
- (BOOL)ravenField:(RTextInputInfo*)info {
	RTextInputInfo found;
	Rid window = [self ravenCore];
	if (info) memset(info, 0, sizeof(*info));
	if (window == R_INVALID_ID || !ravenWantsKeyboard || !Rwindow_get_text_input_info(window, &found)) return NO;
	if (info) *info = found;
	return YES;
}
- (NSString*)ravenText {
	Rid window = [self ravenCore];
	const char* text = window != R_INVALID_ID && ravenWantsKeyboard ? Rwindow_get_text_input_text(window) : NULL;
	NSString* string = text ? [NSString stringWithUTF8String:text] : nil;
	return string ? string : @"";
}
- (NSRange)ravenSelection {
	Rid window = [self ravenCore];
	const char* text = window != R_INVALID_ID && ravenWantsKeyboard ? Rwindow_get_text_input_text(window) : NULL;
	int start, end;
	NSUInteger from, to;
	if (!text || !Rwindow_get_text_input_selection(window, &start, &end)) return NSMakeRange(0, 0);
	from = rvn_uikit_units(text, start);
	to = rvn_uikit_units(text, end);
	return NSMakeRange(from, to > from ? to - from : 0);
}
/* selects [range) of the field; the system asked: it is not told back */
- (void)ravenSelect:(NSRange)range {
	NSString* text = [self ravenText];
	if (![self ravenField:NULL]) return;
	range = rvn_uikit_clamp_range(text, range);
	Rwindow_set_text_input_selection([self ravenCore], rvn_uikit_bytes(text, (NSInteger)range.location),
		rvn_uikit_bytes(text, (NSInteger)NSMaxRange(range)));
	ravenToldSelection = range;
	rvn_uikit_resume();
}
/*
 * Replaces [range) of the field with `string`, the caret after it (the
 * changed callback of the application runs). What the system expects
 * afterwards is remembered as told: if the application or MaxLength made it
 * otherwise, pump tells the system.
 */
- (BOOL)ravenReplace:(NSRange)range with:(NSString*)string {
	NSString* text = [self ravenText];
	if (![self ravenField:NULL]) return NO;
	range = rvn_uikit_clamp_range(text, range);
	if (!string) string = @"";
	/* (the same text again, as an input method does when only the selection in its marked text moves: no edit, no changed callback) */
	if ([[text substringWithRange:range] isEqualToString:string]) {
		[self ravenSelect:NSMakeRange(range.location + string.length, 0)];
		return YES;
	}
	if (!Rwindow_replace_text_input([self ravenCore], rvn_uikit_bytes(text, (NSInteger)range.location),
			rvn_uikit_bytes(text, (NSInteger)NSMaxRange(range)), string.UTF8String))
		return NO;
	ravenToldText = [text stringByReplacingCharactersInRange:range withString:string];
	ravenToldSelection = NSMakeRange(range.location + string.length, 0);
	ravenToldFocus = Rwindow_get_focused_widget([self ravenCore]);
	rvn_uikit_resume();
	return YES;
}

/* ---- typing (UIKeyInput) ---- */
/* the old path: one RVN_PEVENT_TEXT per code point, Enter and Tab as keys (SpinBox, TextEditor, or behind a queued key) */
- (void)ravenPostText:(NSString*)text {
	NSUInteger i = 0, length = text.length;
	while (i < length) {
		unichar c = [text characterAtIndex:i++];
		uint32_t codepoint = c;
		if (CFStringIsSurrogateHighCharacter(c) && i < length && CFStringIsSurrogateLowCharacter([text characterAtIndex:i]))
			codepoint = (uint32_t)CFStringGetLongCharacterForSurrogatePair(c, [text characterAtIndex:i++]);
		if (codepoint == '\n' || codepoint == '\r') { rvn_uikit_post_key(ravenWindow, R_KEY_ENTER); ravenKeysQueued = YES; }
		else if (codepoint == '\t') { rvn_uikit_post_key(ravenWindow, R_KEY_TAB); ravenKeysQueued = YES; }
		else if (codepoint >= 0x20 && codepoint != 0x7f) {
			rvn_pevent_t e;
			memset(&e, 0, sizeof(e));
			e.type = RVN_PEVENT_TEXT;
			e.user = ravenWindow->user;
			e.codepoint = codepoint;
			rvn_uikit_post(&e);
		}
	}
}
- (BOOL)hasText {
	/* (no field: YES, so that the keyboard always sends deleteBackward; Raven knows whether there is something to delete) */
	return [self ravenField:NULL] ? [self ravenText].length > 0 : YES;
}
/*
 * Committed text: it replaces the marked text, or else the selection. The
 * return key is Enter and a tab (hardware keyboard) is Tab, sent as keys;
 * what follows them is queued behind them.
 */
- (void)insertText:(NSString*)text {
	NSMutableString* run;
	NSUInteger i, length = text.length;
	if (!ravenWindow) return;
	if (ravenKeysQueued || ![self ravenField:NULL]) { [self ravenPostText:text]; return; }
	run = [NSMutableString string];
	for (i = 0; i < length; i++) {
		unichar c = [text characterAtIndex:i];
		if (c == '\n' || c == '\r' || c == '\t') {
			[self ravenCommit:run];
			[self ravenPostText:[text substringFromIndex:i]];
			return;
		}
		if (c >= 0x20 && c != 0x7f) [run appendFormat:@"%C", c];
	}
	[self ravenCommit:run];
}
- (void)ravenCommit:(NSString*)run {
	NSRange target = ravenMarked.location != NSNotFound ? ravenMarked : [self ravenSelection];
	if (run.length == 0 && ravenMarked.location == NSNotFound && target.length == 0) return;
	if (ravenMarked.location != NSNotFound) RVN_TEXT_LOG(@"Raven text: insertText \"%@\" over the marked text", run);
	ravenMarked = NSMakeRange(NSNotFound, 0);
	[self ravenReplace:target with:run];
}
/* the selection, else the character (composed sequence) before the caret */
- (void)deleteBackward {
	NSString* text;
	NSRange selection, range;
	if (!ravenWindow) return;
	if (ravenKeysQueued || ![self ravenField:NULL]) {
		rvn_uikit_post_key(ravenWindow, R_KEY_BACKSPACE);
		ravenKeysQueued = YES;
		return;
	}
	text = [self ravenText];
	selection = [self ravenSelection];
	if (selection.length > 0) range = selection;
	else if (selection.location > 0) range = [text rangeOfComposedCharacterSequenceAtIndex:selection.location - 1];
	else return;
	if (ravenMarked.location != NSNotFound) {
		/* (inside the marked text: it gets shorter; elsewhere the composition ends) */
		if (range.location >= ravenMarked.location && NSMaxRange(range) <= NSMaxRange(ravenMarked)) {
			ravenMarked.length -= range.length;
			if (ravenMarked.length == 0) ravenMarked = NSMakeRange(NSNotFound, 0);
		} else ravenMarked = NSMakeRange(NSNotFound, 0);
	}
	[self ravenReplace:range with:@""];
}

/* ---- the document (UITextInput) ---- */
- (NSString*)textInRange:(UITextRange*)range {
	NSString* text = [self ravenText];
	if (![range isKindOfClass:[RavenTextRange class]]) return @"";
	return [text substringWithRange:rvn_uikit_clamp_range(text, ((RavenTextRange*)range).range)];
}
- (void)replaceRange:(UITextRange*)range withText:(NSString*)text {
	NSRange target;
	if (![range isKindOfClass:[RavenTextRange class]]) return;
	target = ((RavenTextRange*)range).range;
	RVN_TEXT_LOG(@"Raven text: replace %@ \"%@\" with \"%@\"", NSStringFromRange(target),
		[[self ravenText] substringWithRange:rvn_uikit_clamp_range([self ravenText], target)], text);
	/* (a replacement over the marked text ends the composition) */
	if (ravenMarked.location != NSNotFound && NSIntersectionRange(ravenMarked, target).length > 0) ravenMarked = NSMakeRange(NSNotFound, 0);
	else if (ravenMarked.location != NSNotFound && NSMaxRange(target) <= ravenMarked.location)
		ravenMarked.location = ravenMarked.location - target.length + text.length;
	[self ravenReplace:target with:text];
}
- (UITextRange*)selectedTextRange { return [RavenTextRange with:[self ravenSelection]]; }
- (void)setSelectedTextRange:(UITextRange*)range {
	if (![range isKindOfClass:[RavenTextRange class]]) return;
	RVN_TEXT_LOG(@"Raven text: select %@", NSStringFromRange(((RavenTextRange*)range).range));
	[self ravenSelect:((RavenTextRange*)range).range];
}
- (UITextRange*)markedTextRange {
	if (ravenMarked.location == NSNotFound) return nil;
	return [RavenTextRange with:ravenMarked];
}
@synthesize markedTextStyle = _markedTextStyle;
@synthesize inputDelegate = _inputDelegate;
/*
 * Composition (CJK input methods, dead keys): Raven has no marked text of its
 * own, so the marked text is put into the field as ordinary text (the
 * application's changed callback sees it) and its range is kept here; the
 * next marked text or the committed text replaces it.
 */
- (void)setMarkedText:(NSString*)markedText selectedRange:(NSRange)selectedRange {
	NSString* before = [self ravenText];
	NSRange target;
	NSInteger inserted;
	if (![self ravenField:NULL]) {
		/* (no field: nothing to compose in; the committed text comes through insertText) */
		return;
	}
	if (!markedText) markedText = @"";
	target = rvn_uikit_clamp_range(before, ravenMarked.location != NSNotFound ? ravenMarked : [self ravenSelection]);
	if (![self ravenReplace:target with:markedText]) return;
	/* (MaxLength may have cut it) */
	inserted = (NSInteger)[self ravenText].length - ((NSInteger)before.length - (NSInteger)target.length);
	if (inserted < 0) inserted = 0;
	if (inserted > (NSInteger)markedText.length) inserted = (NSInteger)markedText.length;
	ravenMarked = inserted > 0 ? NSMakeRange(target.location, (NSUInteger)inserted) : NSMakeRange(NSNotFound, 0);
	if (inserted > 0) {
		NSUInteger start = selectedRange.location > (NSUInteger)inserted ? (NSUInteger)inserted : selectedRange.location;
		NSUInteger end = start + selectedRange.length > (NSUInteger)inserted ? (NSUInteger)inserted : start + selectedRange.length;
		[self ravenSelect:NSMakeRange(target.location + start, end - start)];
	}
	RVN_TEXT_LOG(@"Raven text: marked \"%@\" at %@, selected %@ -> field \"%@\"", markedText, NSStringFromRange(ravenMarked),
		NSStringFromRange(selectedRange), [self ravenText]);
}
- (void)unmarkText {
	if (ravenMarked.location != NSNotFound) RVN_TEXT_LOG(@"Raven text: unmark %@ -> field \"%@\"", NSStringFromRange(ravenMarked), [self ravenText]);
	ravenMarked = NSMakeRange(NSNotFound, 0);
}
- (UITextPosition*)beginningOfDocument { return [RavenTextPosition at:0]; }
- (UITextPosition*)endOfDocument { return [RavenTextPosition at:(NSInteger)[self ravenText].length]; }
- (UITextRange*)textRangeFromPosition:(UITextPosition*)from toPosition:(UITextPosition*)to {
	NSInteger a, b;
	if (![from isKindOfClass:[RavenTextPosition class]] || ![to isKindOfClass:[RavenTextPosition class]]) return nil;
	a = ((RavenTextPosition*)from).offset;
	b = ((RavenTextPosition*)to).offset;
	if (a > b) { NSInteger t = a; a = b; b = t; }
	return [RavenTextRange with:NSMakeRange((NSUInteger)a, (NSUInteger)(b - a))];
}
- (UITextPosition*)positionFromPosition:(UITextPosition*)position offset:(NSInteger)offset {
	NSInteger result;
	if (![position isKindOfClass:[RavenTextPosition class]]) return nil;
	result = ((RavenTextPosition*)position).offset + offset;
	if (result < 0 || result > (NSInteger)[self ravenText].length) return nil;
	return [RavenTextPosition at:result];
}
/* one line: up goes to the start, down to the end */
- (UITextPosition*)positionFromPosition:(UITextPosition*)position inDirection:(UITextLayoutDirection)direction offset:(NSInteger)offset {
	NSInteger result, length = (NSInteger)[self ravenText].length;
	if (![position isKindOfClass:[RavenTextPosition class]]) return nil;
	result = ((RavenTextPosition*)position).offset;
	switch (direction) {
		case UITextLayoutDirectionRight: result += offset; break;
		case UITextLayoutDirectionLeft: result -= offset; break;
		case UITextLayoutDirectionUp: if (offset > 0) result = 0; break;
		case UITextLayoutDirectionDown: if (offset > 0) result = length; break;
	}
	if (result < 0) result = 0;
	if (result > length) result = length;
	return [RavenTextPosition at:result];
}
- (NSComparisonResult)comparePosition:(UITextPosition*)position toPosition:(UITextPosition*)other {
	NSInteger a = [position isKindOfClass:[RavenTextPosition class]] ? ((RavenTextPosition*)position).offset : 0;
	NSInteger b = [other isKindOfClass:[RavenTextPosition class]] ? ((RavenTextPosition*)other).offset : 0;
	return a < b ? NSOrderedAscending : a > b ? NSOrderedDescending : NSOrderedSame;
}
- (NSInteger)offsetFromPosition:(UITextPosition*)from toPosition:(UITextPosition*)to {
	NSInteger a = [from isKindOfClass:[RavenTextPosition class]] ? ((RavenTextPosition*)from).offset : 0;
	NSInteger b = [to isKindOfClass:[RavenTextPosition class]] ? ((RavenTextPosition*)to).offset : 0;
	return b - a;
}
- (id<UITextInputTokenizer>)tokenizer {
	if (!ravenTokenizer) ravenTokenizer = [[UITextInputStringTokenizer alloc] initWithTextInput:self];
	return ravenTokenizer;
}
- (UITextPosition*)positionWithinRange:(UITextRange*)range farthestInDirection:(UITextLayoutDirection)direction {
	if (![range isKindOfClass:[RavenTextRange class]]) return nil;
	return direction == UITextLayoutDirectionLeft || direction == UITextLayoutDirectionUp ? range.start : range.end;
}
- (UITextRange*)characterRangeByExtendingPosition:(UITextPosition*)position inDirection:(UITextLayoutDirection)direction {
	NSInteger offset, length = (NSInteger)[self ravenText].length;
	if (![position isKindOfClass:[RavenTextPosition class]]) return nil;
	offset = ((RavenTextPosition*)position).offset;
	if (direction == UITextLayoutDirectionLeft || direction == UITextLayoutDirectionUp)
		return [RavenTextRange with:NSMakeRange(0, (NSUInteger)offset)];
	return [RavenTextRange with:NSMakeRange((NSUInteger)offset, (NSUInteger)(length > offset ? length - offset : 0))];
}
- (NSWritingDirection)baseWritingDirectionForPosition:(UITextPosition*)position inDirection:(UITextStorageDirection)direction {
	(void)position; (void)direction;
	return NSWritingDirectionNatural;
}
- (void)setBaseWritingDirection:(NSWritingDirection)writingDirection forRange:(UITextRange*)range { (void)writingDirection; (void)range; }

/* ---- geometry (UITextInput) ---- */
/*
 * Raven tells only the caret rectangle (Rwindow_get_text_input_rect, once
 * the field was painted with the focus). The other positions are placed
 * from it with the width of the text between them and the caret, measured
 * in the theme's font and size (what a TextInput uses unless styled
 * otherwise): close enough for the system (the candidate bar of an input
 * method, the loupe, the space-bar cursor).
 */
- (BOOL)ravenCaret:(CGRect*)caret offset:(NSUInteger*)offset text:(NSString*)text {
	Rid window = [self ravenCore];
	int x, y, width, height;
	float scale = rvn_uikit_scale_of(self);
	NSRange selection;
	if (window == R_INVALID_ID || ![self ravenField:NULL] || !Rwindow_get_text_input_rect(window, &x, &y, &width, &height)) return NO;
	*caret = CGRectMake(x / scale, y / scale, (width > 0 ? width : 1) / scale, height / scale);
	selection = [self ravenSelection];
	/* (the core gives the selection in order: the caret is taken at its end) */
	*offset = rvn_uikit_clamp_units(text, (NSInteger)NSMaxRange(selection));
	return YES;
}
/* the width of the first `units` UTF-16 units, in points */
- (CGFloat)ravenWidthOf:(NSString*)text units:(NSUInteger)units {
	Rid window = [self ravenCore];
	RTextInputInfo info;
	NSString* prefix = [text substringToIndex:rvn_uikit_clamp_units(text, (NSInteger)units)];
	float width = 0.0f;
	if ([self ravenField:&info] && info.password) {
		NSMutableString* bullets = [NSMutableString string];
		NSUInteger i;
		for (i = 0; i < prefix.length; i++) [bullets appendString:@"•"];
		prefix = bullets;
	}
	Rfont_measure_text(R_INVALID_ID, 0.0f, prefix.UTF8String, &width, NULL);
	return (CGFloat)(width * Rwindow_get_scale(window) / rvn_uikit_scale_of(self));
}
- (CGFloat)ravenXOf:(NSUInteger)position text:(NSString*)text caret:(CGRect)caret offset:(NSUInteger)offset {
	return caret.origin.x + [self ravenWidthOf:text units:position] - [self ravenWidthOf:text units:offset];
}
- (CGRect)ravenRectFrom:(NSUInteger)start to:(NSUInteger)end {
	NSString* text = [self ravenText];
	CGRect caret;
	NSUInteger offset;
	CGFloat x0, x1;
	if (![self ravenCaret:&caret offset:&offset text:text]) return CGRectMake(0, 0, 0, 0);
	x0 = [self ravenXOf:start text:text caret:caret offset:offset];
	x1 = end == start ? x0 + caret.size.width : [self ravenXOf:end text:text caret:caret offset:offset];
	return CGRectMake(x0, caret.origin.y, x1 > x0 ? x1 - x0 : caret.size.width, caret.size.height);
}
- (CGRect)firstRectForRange:(UITextRange*)range {
	NSRange r;
	if (![range isKindOfClass:[RavenTextRange class]]) return CGRectMake(0, 0, 0, 0);
	r = ((RavenTextRange*)range).range;
	return [self ravenRectFrom:r.location to:NSMaxRange(r)];
}
- (CGRect)caretRectForPosition:(UITextPosition*)position {
	NSUInteger offset;
	CGRect rect;
	if (![position isKindOfClass:[RavenTextPosition class]]) return CGRectMake(0, 0, 0, 0);
	offset = (NSUInteger)((RavenTextPosition*)position).offset;
	rect = [self ravenRectFrom:offset to:offset];
	return rect;
}
- (NSArray<UITextSelectionRect*>*)selectionRectsForRange:(UITextRange*)range {
	RavenSelectionRect* rect;
	if (![range isKindOfClass:[RavenTextRange class]]) return @[];
	rect = [[RavenSelectionRect alloc] init];
	rect->ravenRect = [self firstRectForRange:range];
	return @[ rect ];
}
- (UITextPosition*)closestPositionToPoint:(CGPoint)point {
	NSString* text = [self ravenText];
	CGRect caret;
	NSUInteger offset, i = 0, best = 0;
	CGFloat best_distance = CGFLOAT_MAX;
	if (![self ravenCaret:&caret offset:&offset text:text]) return [RavenTextPosition at:(NSInteger)[self ravenSelection].location];
	for (;;) {
		CGFloat distance = fabs([self ravenXOf:i text:text caret:caret offset:offset] - point.x);
		if (distance < best_distance) { best_distance = distance; best = i; }
		if (i >= text.length) break;
		i = NSMaxRange([text rangeOfComposedCharacterSequenceAtIndex:i]);
	}
	return [RavenTextPosition at:(NSInteger)best];
}
- (UITextPosition*)closestPositionToPoint:(CGPoint)point withinRange:(UITextRange*)range {
	RavenTextPosition* position = (RavenTextPosition*)[self closestPositionToPoint:point];
	NSRange r;
	if (![range isKindOfClass:[RavenTextRange class]]) return position;
	r = ((RavenTextRange*)range).range;
	if ((NSUInteger)position.offset < r.location) return range.start;
	if ((NSUInteger)position.offset > NSMaxRange(r)) return range.end;
	return position;
}
- (UITextRange*)characterRangeAtPoint:(CGPoint)point {
	NSString* text = [self ravenText];
	NSUInteger offset = (NSUInteger)((RavenTextPosition*)[self closestPositionToPoint:point]).offset;
	if (text.length == 0) return [RavenTextRange with:NSMakeRange(0, 0)];
	if (offset >= text.length) offset = text.length - 1;
	return [RavenTextRange with:[text rangeOfComposedCharacterSequenceAtIndex:offset]];
}
- (UIView*)textInputView { return self; }

/* ---- the keyboard for the field (UITextInputTraits), from Rwindow_get_text_input_info ---- */
/* (no field: SpinBox, TextEditor: the system cannot see the text, so no corrections and no capitals of its own) */
- (UIKeyboardType)keyboardType {
	RTextInputInfo info;
	if (![self ravenField:&info]) return UIKeyboardTypeDefault;
	switch (info.type) {
		case R_TEXT_INPUT_TYPE_EMAIL: return UIKeyboardTypeEmailAddress;
		case R_TEXT_INPUT_TYPE_NUMBER: return UIKeyboardTypeNumberPad;
		case R_TEXT_INPUT_TYPE_DECIMAL: return UIKeyboardTypeDecimalPad;
		case R_TEXT_INPUT_TYPE_PHONE: return UIKeyboardTypePhonePad;
		case R_TEXT_INPUT_TYPE_URL: return UIKeyboardTypeURL;
		case R_TEXT_INPUT_TYPE_SEARCH: return UIKeyboardTypeWebSearch;
		default: return UIKeyboardTypeDefault;
	}
}
- (UIReturnKeyType)returnKeyType {
	RTextInputInfo info;
	if (![self ravenField:&info]) return UIReturnKeyDefault;
	switch (info.return_key) {
		case R_RETURN_KEY_DONE: return UIReturnKeyDone;
		case R_RETURN_KEY_GO: return UIReturnKeyGo;
		case R_RETURN_KEY_NEXT: return UIReturnKeyNext;
		case R_RETURN_KEY_SEARCH: return UIReturnKeySearch;
		case R_RETURN_KEY_SEND: return UIReturnKeySend;
		default: return UIReturnKeyDefault;
	}
}
- (UITextContentType)textContentType {
	RTextInputInfo info;
	if (![self ravenField:&info]) return nil;
	if (info.password) return UITextContentTypePassword;
	switch (info.type) {
		case R_TEXT_INPUT_TYPE_EMAIL: return UITextContentTypeEmailAddress;
		case R_TEXT_INPUT_TYPE_PHONE: return UITextContentTypeTelephoneNumber;
		case R_TEXT_INPUT_TYPE_URL: return UITextContentTypeURL;
		default: return nil;
	}
}
/* words: free text (Text, Search) that is not a password */
- (BOOL)ravenWords {
	RTextInputInfo info;
	if (![self ravenField:&info] || info.password) return NO;
	return info.type == R_TEXT_INPUT_TYPE_TEXT || info.type == R_TEXT_INPUT_TYPE_SEARCH;
}
- (UITextAutocapitalizationType)autocapitalizationType {
	RTextInputInfo info;
	if (![self ravenField:&info] || info.password || info.type != R_TEXT_INPUT_TYPE_TEXT) return UITextAutocapitalizationTypeNone;
	return UITextAutocapitalizationTypeSentences;
}
- (UITextAutocorrectionType)autocorrectionType { return [self ravenWords] ? UITextAutocorrectionTypeYes : UITextAutocorrectionTypeNo; }
- (UITextSpellCheckingType)spellCheckingType { return [self ravenWords] ? UITextSpellCheckingTypeYes : UITextSpellCheckingTypeNo; }
- (UITextSmartQuotesType)smartQuotesType { return [self ravenWords] ? UITextSmartQuotesTypeDefault : UITextSmartQuotesTypeNo; }
- (UITextSmartDashesType)smartDashesType { return [self ravenWords] ? UITextSmartDashesTypeDefault : UITextSmartDashesTypeNo; }
- (UITextSmartInsertDeleteType)smartInsertDeleteType { return [self ravenWords] ? UITextSmartInsertDeleteTypeDefault : UITextSmartInsertDeleteTypeNo; }
/* (inline predictions draw their grey text in the field: Raven cannot show them) */
- (UITextInlinePredictionType)inlinePredictionType API_AVAILABLE(ios(17.0)) { return UITextInlinePredictionTypeNo; }
- (BOOL)isSecureTextEntry {
	RTextInputInfo info;
	return [self ravenField:&info] && info.password;
}
- (BOOL)enablesReturnKeyAutomatically { return NO; }
/*
 * The number and phone pads of an iPhone have no return key: a bar above
 * them carries it (Done, Next...), as Enter.
 */
- (UIView*)inputAccessoryView {
	RTextInputInfo info;
	UIToolbar* bar;
	UIBarButtonItem* key;
	if (self.traitCollection.userInterfaceIdiom != UIUserInterfaceIdiomPhone || ![self ravenField:&info]) return nil;
	if (info.type != R_TEXT_INPUT_TYPE_NUMBER && info.type != R_TEXT_INPUT_TYPE_DECIMAL && info.type != R_TEXT_INPUT_TYPE_PHONE) return nil;
	bar = [[UIToolbar alloc] initWithFrame:CGRectMake(0, 0, self.bounds.size.width, 44)];
	switch (info.return_key) {
		case R_RETURN_KEY_NEXT: key = [[UIBarButtonItem alloc] initWithTitle:@"Next" style:UIBarButtonItemStylePlain target:self action:@selector(ravenReturnKey)]; break;
		case R_RETURN_KEY_GO: key = [[UIBarButtonItem alloc] initWithTitle:@"Go" style:UIBarButtonItemStyleDone target:self action:@selector(ravenReturnKey)]; break;
		case R_RETURN_KEY_SEARCH: key = [[UIBarButtonItem alloc] initWithTitle:@"Search" style:UIBarButtonItemStyleDone target:self action:@selector(ravenReturnKey)]; break;
		case R_RETURN_KEY_SEND: key = [[UIBarButtonItem alloc] initWithTitle:@"Send" style:UIBarButtonItemStyleDone target:self action:@selector(ravenReturnKey)]; break;
		default: key = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(ravenReturnKey)]; break;
	}
	bar.items = @[ [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace target:nil action:NULL], key ];
	[bar sizeToFit];
	return bar;
}
- (void)ravenReturnKey { [self insertText:@"\n"]; }

/*
 * After the events of a frame (pump): when Raven changed the field itself
 * (a tap moved the caret, Select All, the application set the text) the
 * system is told, so that its suggestions, capitals and composition follow.
 * When the focus moved to another field while the keyboard stays, the
 * keyboard is reloaded for it (type, return key...).
 */
- (void)ravenSync {
	id<UITextInputDelegate> delegate = self.inputDelegate;
	Rid window = [self ravenCore], focus;
	NSString* text;
	NSRange selection;
	BOOL textChanged, selectionChanged;
	if (!self.isFirstResponder || window == R_INVALID_ID) {
		ravenToldFocus = R_INVALID_ID;
		ravenToldText = nil;
		ravenMarked = NSMakeRange(NSNotFound, 0);
		return;
	}
	focus = Rwindow_get_focused_widget(window);
	text = [self ravenText];
	selection = [self ravenSelection];
	if (focus != ravenToldFocus) {
		BOOL moved = ravenToldFocus != R_INVALID_ID;
		RVN_TEXT_LOG(@"Raven text: focus %u -> %u%s", (unsigned)ravenToldFocus, (unsigned)focus, moved ? " (keyboard reloaded)" : "");
		ravenMarked = NSMakeRange(NSNotFound, 0);
		ravenToldFocus = focus;
		ravenToldText = text;
		ravenToldSelection = selection;
		[delegate selectionWillChange:self];
		[delegate textWillChange:self];
		if (moved) [self reloadInputViews];
		[delegate textDidChange:self];
		[delegate selectionDidChange:self];
		return;
	}
	textChanged = !ravenToldText || ![text isEqualToString:ravenToldText];
	selectionChanged = !NSEqualRanges(selection, ravenToldSelection);
	if (!textChanged && !selectionChanged) return;
	RVN_TEXT_LOG(@"Raven text: changed by Raven:%s%s \"%@\" %@", textChanged ? " text" : "", selectionChanged ? " selection" : "",
		text, NSStringFromRange(selection));
	if (ravenMarked.location != NSNotFound) {
		/* (the composition does not survive a change from Raven) */
		ravenMarked = NSMakeRange(NSNotFound, 0);
	}
	ravenToldText = text;
	ravenToldSelection = selection;
	if (selectionChanged) [delegate selectionWillChange:self];
	if (textChanged) [delegate textWillChange:self];
	if (textChanged) [delegate textDidChange:self];
	if (selectionChanged) [delegate selectionDidChange:self];
}

- (instancetype)initWithFrame:(CGRect)frame {
	self = [super initWithFrame:frame];
	if (self) {
		CAMetalLayer* layer = (CAMetalLayer*)self.layer;
		layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
		layer.opaque = YES;
		id<MTLDevice> device = MTLCreateSystemDefaultDevice();
		if (device) layer.device = device;
		self.multipleTouchEnabled = YES;
		ravenMarked = NSMakeRange(NSNotFound, 0);
		ravenToldFocus = R_INVALID_ID;
		/* dark / light mode of the system: the core asks system_dark again (iOS 15 and 16: traitCollectionDidChange:) */
		if (@available(iOS 17.0, *)) {
			[self registerForTraitChanges:@[ [UITraitUserInterfaceStyle class] ]
			                   withHandler:^(__kindof id<UITraitEnvironment> environment, UITraitCollection* previous) {
				(void)environment; (void)previous;
				rvn_uikit_post_appearance();
			}];
		}
	}
	return self;
}

/* iOS 15 and 16 (deprecated in 17, where registerForTraitChanges: above tells instead) */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
- (void)traitCollectionDidChange:(UITraitCollection*)previous {
	[super traitCollectionDidChange:previous];
	if (@available(iOS 17.0, *)) return;
	if (!previous || previous.userInterfaceStyle != self.traitCollection.userInterfaceStyle) rvn_uikit_post_appearance();
}
#pragma clang diagnostic pop

- (void)layoutSubviews {
	[super layoutSubviews];
	float scale = rvn_uikit_scale_of(self);
	CGSize size = self.bounds.size;
	CAMetalLayer* layer = (CAMetalLayer*)self.layer;
	self.contentScaleFactor = scale;
	layer.contentsScale = scale;
	layer.drawableSize = CGSizeMake(lrint(size.width * scale), lrint(size.height * scale));
	if (!ravenWindow) return;
	rvn_pevent_t e;
	memset(&e, 0, sizeof(e));
	e.type = RVN_PEVENT_RESIZE;
	e.user = ravenWindow->user;
	e.width = (int)lrint(size.width * scale);
	e.height = (int)lrint(size.height * scale);
	e.scale = scale;
	rvn_uikit_post(&e);
	e.type = RVN_PEVENT_EXPOSE;
	rvn_uikit_post(&e);
	/* (rotation: the safe area moves with the bounds; the core ignores it when nothing changed) */
	e.type = RVN_PEVENT_INSETS;
	rvn_uikit_post(&e);
	/* (the part the keyboard covers moves with the bounds too) */
	rvn_uikit_keyboard_measure(ravenWindow);
}

- (void)safeAreaInsetsDidChange {
	[super safeAreaInsetsDidChange];
	if (!ravenWindow) return;
	rvn_pevent_t e;
	memset(&e, 0, sizeof(e));
	e.type = RVN_PEVENT_INSETS;
	e.user = ravenWindow->user;
	rvn_uikit_post(&e);
}

/* ---- the edit menu (edit_menu_show): Cut, Copy, Paste, Select All for the text the finger selected ---- */
- (void)ravenPostEdit:(unsigned)command {
	rvn_pevent_t e;
	if (!ravenWindow) return;
	RVN_TEXT_LOG(@"Raven text: edit menu entry %u", command);
	memset(&e, 0, sizeof(e));
	e.type = RVN_PEVENT_EDIT;
	e.user = ravenWindow->user;
	e.edit = command;
	rvn_uikit_post(&e);
}
- (void)ravenShowEditMenu:(CGRect)rect items:(unsigned)items {
	ravenMenuItems = items;
	ravenMenuRect = rect;
	RVN_TEXT_LOG(@"Raven text: edit menu %u at %@ (first responder %d)", items, NSStringFromCGRect(rect), self.isFirstResponder);
	if (@available(iOS 16.0, *)) {
		UIEditMenuInteraction* menu = (UIEditMenuInteraction*)ravenEditMenu;
		UIEditMenuConfiguration* configuration;
		if (!menu) {
			menu = [[UIEditMenuInteraction alloc] initWithDelegate:self];
			[self addInteraction:menu];
			ravenEditMenu = menu;
		}
		configuration = [UIEditMenuConfiguration configurationWithIdentifier:nil
			sourcePoint:CGPointMake(CGRectGetMidX(rect), CGRectGetMinY(rect))];
		[menu presentEditMenuWithConfiguration:configuration];
	} else {
		/* (iOS 15: the menu of the first responder; the view is it while the keyboard is up) */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		[[UIMenuController sharedMenuController] showMenuFromView:self rect:rect];
#pragma clang diagnostic pop
	}
}
- (void)ravenHideEditMenu {
	RVN_TEXT_LOG(@"Raven text: edit menu hidden");
	ravenMenuItems = 0;
	if (@available(iOS 16.0, *)) {
		[(UIEditMenuInteraction*)ravenEditMenu dismissMenu];
	} else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		[[UIMenuController sharedMenuController] hideMenuFromView:self];
#pragma clang diagnostic pop
	}
}
/* the entries Raven asked for, while its menu is up; otherwise none of them (Cmd+C... stay keys for the focused widget) */
- (BOOL)canPerformAction:(SEL)action withSender:(id)sender {
	if (action == @selector(cut:)) return (ravenMenuItems & RVN_EDIT_CUT) != 0;
	if (action == @selector(copy:)) return (ravenMenuItems & RVN_EDIT_COPY) != 0;
	if (action == @selector(paste:)) return (ravenMenuItems & RVN_EDIT_PASTE) != 0;
	if (action == @selector(selectAll:)) return (ravenMenuItems & RVN_EDIT_SELECT_ALL) != 0;
	/* (Look Up, Translate, Share... would need the text of the selection: not in Raven's menu) */
	if (ravenMenuItems) return NO;
	return [super canPerformAction:action withSender:sender];
}
- (void)cut:(id)sender { (void)sender; [self ravenPostEdit:RVN_EDIT_CUT]; }
- (void)copy:(id)sender { (void)sender; [self ravenPostEdit:RVN_EDIT_COPY]; }
- (void)selectAll:(id)sender { (void)sender; [self ravenPostEdit:RVN_EDIT_SELECT_ALL]; }
- (void)paste:(id)sender {
	NSString* string = [UIPasteboard generalPasteboard].string;
	(void)sender;
	if (g_uikit.pasted) CFBridgingRelease(g_uikit.pasted);
	g_uikit.pasted = string ? (void*)CFBridgingRetain(string) : NULL;
	[self ravenPostEdit:RVN_EDIT_PASTE];
}
- (CGRect)editMenuInteraction:(UIEditMenuInteraction*)interaction targetRectForConfiguration:(UIEditMenuConfiguration*)configuration API_AVAILABLE(ios(16.0)) {
	(void)interaction;
	(void)configuration;
	return ravenMenuRect;
}
- (UIMenu*)editMenuInteraction:(UIEditMenuInteraction*)interaction menuForConfiguration:(UIEditMenuConfiguration*)configuration
	suggestedActions:(NSArray<UIMenuElement*>*)suggestedActions API_AVAILABLE(ios(16.0)) {
	NSMutableArray<UIMenuElement*>* own;
	(void)interaction;
	(void)configuration;
	RVN_TEXT_LOG(@"Raven text: edit menu, %u suggested entries", (unsigned)suggestedActions.count);
	/* the system's entries (its titles, in the user's language), from canPerformAction: above */
	if (suggestedActions.count > 0) return [UIMenu menuWithChildren:suggestedActions];
	/* SHORTCUT: when the system suggests nothing (the view is not the first responder), English titles */
	own = [NSMutableArray array];
	if (ravenMenuItems & RVN_EDIT_CUT) [own addObject:[UICommand commandWithTitle:@"Cut" image:nil action:@selector(cut:) propertyList:nil]];
	if (ravenMenuItems & RVN_EDIT_COPY) [own addObject:[UICommand commandWithTitle:@"Copy" image:nil action:@selector(copy:) propertyList:nil]];
	if (ravenMenuItems & RVN_EDIT_PASTE) [own addObject:[UICommand commandWithTitle:@"Paste" image:nil action:@selector(paste:) propertyList:nil]];
	if (ravenMenuItems & RVN_EDIT_SELECT_ALL) [own addObject:[UICommand commandWithTitle:@"Select All" image:nil action:@selector(selectAll:) propertyList:nil]];
	return [UIMenu menuWithChildren:own];
}
- (void)editMenuInteraction:(UIEditMenuInteraction*)interaction willDismissMenuForConfiguration:(UIEditMenuConfiguration*)configuration
	animator:(id<UIEditMenuInteractionAnimating>)animator API_AVAILABLE(ios(16.0)) {
	(void)interaction;
	(void)configuration;
	(void)animator;
	/* (a touch elsewhere, an entry chosen: the core is told the menu is gone) */
	if (ravenMenuItems) [self ravenPostEdit:0];
}

- (void)ravenTouches:(NSSet<UITouch*>*)touches phase:(int)phase {
	if (!ravenWindow) return;
	float scale = rvn_uikit_scale_of(self);
	for (UITouch* touch in touches) {
		CGPoint p = [touch locationInView:self];
		rvn_pevent_t e;
		memset(&e, 0, sizeof(e));
		e.type = RVN_PEVENT_TOUCH;
		e.user = ravenWindow->user;
		e.phase = phase;
		/* the UITouch object lives for the whole touch: its address names the finger */
		e.touch_id = (int)(((uintptr_t)(__bridge void*)touch >> 4) & 0x7fffffff);
		e.fx = (float)(p.x * scale);
		e.fy = (float)(p.y * scale);
		e.x = (int)e.fx;
		e.y = (int)e.fy;
		e.pressure = touch.maximumPossibleForce > 0 ? (float)(touch.force / touch.maximumPossibleForce) : 1.0f;
		e.contact_w = e.contact_h = (float)(touch.majorRadius * 2.0 * scale);
		rvn_uikit_post(&e);
	}
}
- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event { (void)event; [self ravenTouches:touches phase:R_TOUCH_DOWN]; }
- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event { (void)event; [self ravenTouches:touches phase:R_TOUCH_MOVE]; }
- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event { (void)event; [self ravenTouches:touches phase:R_TOUCH_UP]; }
- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event { (void)event; [self ravenTouches:touches phase:R_TOUCH_CANCEL]; }
@end

@interface RavenUIKitViewController : UIViewController
@end

@implementation RavenUIKitViewController
/* the status bar is shown; its default style follows the dark / light mode, as Rtheme_follow_system does */
- (BOOL)prefersStatusBarHidden { return NO; }
- (UIStatusBarStyle)preferredStatusBarStyle { return UIStatusBarStyleDefault; }
/* every orientation but upside down on iPhone (the Info.plist of the application has the last word) */
- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
	return self.traitCollection.userInterfaceIdiom == UIUserInterfaceIdiomPad ? UIInterfaceOrientationMaskAll
	                                                                          : UIInterfaceOrientationMaskAllButUpsideDown;
}
@end

static UIWindowScene* rvn_uikit_scene(void) {
	UIWindowScene* found = nil;
	for (UIScene* scene in [UIApplication sharedApplication].connectedScenes) {
		if (![scene isKindOfClass:[UIWindowScene class]]) continue;
		if (!found || scene.activationState == UISceneActivationStateForegroundActive) found = (UIWindowScene*)scene;
	}
	return found;
}

/* ---------------------------------------------------------------------------
 *  Accessibility: VoiceOver (UIAccessibility)
 *
 *  The view is a container (not an element itself) that stands for the
 *  window node of core/accessibility.h. Every other node is a
 *  RavenUIKitElement (a UIAccessibilityElement), made on first use and kept
 *  in a table by node; all it says is asked of the core when VoiceOver asks
 *  (UIKit and the core share the main thread), with the mapping of
 *  core/accessibility_touch.c, which TalkBack on Android uses too:
 *   - a node with children is a container of them; a node that is also an
 *     element (an expander, a custom widget with children) comes first among
 *     them, as a second RavenUIKitElement for the same node (ravenSelf);
 *   - label = name, value = value, hint = description, traits from the role
 *     and the states; the frame is the node's rectangle (points: the logical
 *     units of Raven on iOS) given to UIAccessibilityConvertFrameToScreenCoordinates;
 *   - a double tap is rvn_a11y_activate, a swipe up / down rvn_a11y_adjust.
 *  The nodes out of view (rows of a list scrolled away, pages not shown) are left out.
 *  SHORTCUT: three-finger scrolling (accessibilityScroll) is not handled.
 * ------------------------------------------------------------------------- */

@interface RavenUIKitElement : UIAccessibilityElement {
@public
	rvn_a11y_id_t ravenNode;
	rvn_pwindow_t* ravenWindow;
	/* the node itself, first among its own children (it is an element and a container) */
	BOOL ravenSelf;
}
@end

/* node -> RavenUIKitElement (containers and elements), and node -> its ravenSelf element */
static NSMutableDictionary* g_uikit_elements;
static NSMutableDictionary* g_uikit_selves;

static rvn_a11y_id_t rvn_uikit_a11y_root(rvn_pwindow_t* window) {
	return window ? (rvn_a11y_id_t)rvn_a11y_window_of(window->user) : 0;
}

static id rvn_uikit_a11y_object(rvn_pwindow_t* window, rvn_a11y_id_t node);

static RavenUIKitElement* rvn_uikit_a11y_element(NSMutableDictionary* __strong* table, rvn_pwindow_t* window, rvn_a11y_id_t node, id container, BOOL itself) {
	NSNumber* key = [NSNumber numberWithUnsignedLongLong:(unsigned long long)node];
	RavenUIKitElement* element;
	if (!*table) *table = [[NSMutableDictionary alloc] init];
	element = [*table objectForKey:key];
	if (element) {
		/* (a node can move to another parent) */
		if (container && element.accessibilityContainer != container) element.accessibilityContainer = container;
		return element;
	}
	if (!container) return nil;
	element = [[RavenUIKitElement alloc] initWithAccessibilityContainer:container];
	element->ravenNode = node;
	element->ravenWindow = window;
	element->ravenSelf = itself;
	[*table setObject:element forKey:key];
	return element;
}

/* The elements in a container: the node itself first when it is an element too, then its children shown. */
static NSArray* rvn_uikit_a11y_children(rvn_pwindow_t* window, rvn_a11y_id_t node, id container, BOOL with_self) {
	int i, count = rvn_a11y_child_count(node);
	NSMutableArray* elements = [NSMutableArray arrayWithCapacity:(NSUInteger)(count > 0 ? count + 1 : 1)];
	if (with_self) {
		RavenUIKitElement* itself = rvn_uikit_a11y_element(&g_uikit_selves, window, node, container, YES);
		if (itself) [elements addObject:itself];
	}
	for (i = 0; i < count; i++) {
		rvn_a11y_id_t child = rvn_a11y_child(node, i);
		rvn_a11y_node_t info;
		rvn_a11y_touch_t touch;
		RavenUIKitElement* element;
		if (!rvn_a11y_describe(child, &info) || !rvn_a11y_touch_shown(&info)) continue;
		rvn_a11y_touch(&info, &touch);
		/* (a node that is not an element and holds nothing says nothing) */
		if (!(touch.flags & RVN_A11Y_TOUCH_ELEMENT) && rvn_a11y_child_count(child) == 0) continue;
		element = rvn_uikit_a11y_element(&g_uikit_elements, window, child, container, NO);
		if (element) [elements addObject:element];
	}
	return elements;
}

/* The object VoiceOver should be on for a node: the view for the window, the element (or its ravenSelf) for the others. */
static id rvn_uikit_a11y_object(rvn_pwindow_t* window, rvn_a11y_id_t node) {
	rvn_a11y_node_t info;
	rvn_a11y_touch_t touch;
	id container;
	if (!window || !node || !window->view) return nil;
	if (node == rvn_uikit_a11y_root(window)) return (__bridge RavenUIKitView*)window->view;
	if (!rvn_a11y_describe(node, &info)) return nil;
	rvn_a11y_touch(&info, &touch);
	container = rvn_uikit_a11y_object(window, rvn_a11y_parent(node));
	if (!container) return nil;
	if ([container isKindOfClass:[RavenUIKitElement class]] && ((RavenUIKitElement*)container)->ravenSelf) return nil;
	if ((touch.flags & RVN_A11Y_TOUCH_ELEMENT) && rvn_a11y_child_count(node) > 0) {
		id holder = rvn_uikit_a11y_element(&g_uikit_elements, window, node, container, NO);
		return rvn_uikit_a11y_element(&g_uikit_selves, window, node, holder, YES);
	}
	return rvn_uikit_a11y_element(&g_uikit_elements, window, node, container, NO);
}

static void rvn_uikit_a11y_forget_in(NSMutableDictionary* table, rvn_pwindow_t* window) {
	NSMutableArray* keys = [NSMutableArray array];
	for (NSNumber* key in table) {
		RavenUIKitElement* element = [table objectForKey:key];
		/* a window that goes takes its elements; else only the nodes that are gone */
		if (window ? element->ravenWindow == window : !rvn_a11y_exists(element->ravenNode)) {
			element->ravenWindow = NULL;
			[keys addObject:key];
		}
	}
	[table removeObjectsForKeys:keys];
}

static void rvn_uikit_a11y_forget(rvn_pwindow_t* window) {
	if (g_uikit_elements) rvn_uikit_a11y_forget_in(g_uikit_elements, window);
	if (g_uikit_selves) rvn_uikit_a11y_forget_in(g_uikit_selves, window);
}

static UIAccessibilityTraits rvn_uikit_a11y_traits(const rvn_a11y_node_t* node, const rvn_a11y_touch_t* touch) {
	UIAccessibilityTraits traits = UIAccessibilityTraitNone;
	uint32_t t = touch->traits;
	if (t & RVN_A11Y_TRAIT_BUTTON) traits |= UIAccessibilityTraitButton;
	if (t & RVN_A11Y_TRAIT_LINK) traits |= UIAccessibilityTraitLink;
	if (t & RVN_A11Y_TRAIT_HEADER) traits |= UIAccessibilityTraitHeader;
	if (t & RVN_A11Y_TRAIT_IMAGE) traits |= UIAccessibilityTraitImage;
	if (t & RVN_A11Y_TRAIT_STATIC_TEXT) traits |= UIAccessibilityTraitStaticText;
	if (t & RVN_A11Y_TRAIT_ADJUSTABLE) traits |= UIAccessibilityTraitAdjustable;
	if (t & RVN_A11Y_TRAIT_SELECTED) traits |= UIAccessibilityTraitSelected;
	if (t & RVN_A11Y_TRAIT_NOT_ENABLED) traits |= UIAccessibilityTraitNotEnabled;
	if (t & RVN_A11Y_TRAIT_UPDATES_FREQUENTLY) traits |= UIAccessibilityTraitUpdatesFrequently;
	if (t & RVN_A11Y_TRAIT_TAB_BAR) traits |= UIAccessibilityTraitTabBar;
	if (t & RVN_A11Y_TRAIT_TOGGLE) {
		/* iOS 17: "switch button, on"; before: a button, "selected" when checked */
		if (@available(iOS 17.0, *)) traits |= UIAccessibilityTraitToggleButton;
		else if (node->states & RVN_A11Y_CHECKED) traits |= UIAccessibilityTraitSelected;
	}
	return traits;
}

static NSString* rvn_uikit_a11y_string(const char* text) {
	return text && text[0] ? [NSString stringWithUTF8String:text] : nil;
}

@implementation RavenUIKitElement
- (BOOL)ravenDescribe:(rvn_a11y_node_t*)node touch:(rvn_a11y_touch_t*)touch {
	if (!ravenWindow || !rvn_a11y_describe(ravenNode, node)) return NO;
	rvn_a11y_touch(node, touch);
	return YES;
}
- (BOOL)isAccessibilityElement {
	rvn_a11y_node_t node;
	rvn_a11y_touch_t touch;
	if (![self ravenDescribe:&node touch:&touch]) return NO;
	if (ravenSelf) return YES;
	return (touch.flags & RVN_A11Y_TOUCH_ELEMENT) && rvn_a11y_child_count(ravenNode) == 0 ? YES : NO;
}
- (NSArray*)accessibilityElements {
	rvn_a11y_node_t node;
	rvn_a11y_touch_t touch;
	if (ravenSelf || ![self ravenDescribe:&node touch:&touch] || rvn_a11y_child_count(ravenNode) == 0) return nil;
	return rvn_uikit_a11y_children(ravenWindow, ravenNode, self, (touch.flags & RVN_A11Y_TOUCH_ELEMENT) ? YES : NO);
}
- (UIAccessibilityContainerType)accessibilityContainerType {
	rvn_a11y_node_t node;
	rvn_a11y_touch_t touch;
	if (ravenSelf || ![self ravenDescribe:&node touch:&touch]) return UIAccessibilityContainerTypeNone;
	/* (a tab bar is said by its trait) */
	if (touch.traits & RVN_A11Y_TRAIT_TAB_BAR) return UIAccessibilityContainerTypeNone;
	if (touch.flags & RVN_A11Y_TOUCH_COLLECTION) return UIAccessibilityContainerTypeList;
	if (touch.flags & RVN_A11Y_TOUCH_GROUP) return UIAccessibilityContainerTypeSemanticGroup;
	return UIAccessibilityContainerTypeNone;
}
- (NSString*)accessibilityLabel {
	rvn_a11y_node_t node;
	rvn_a11y_touch_t touch;
	return [self ravenDescribe:&node touch:&touch] ? rvn_uikit_a11y_string(node.name) : nil;
}
- (NSString*)accessibilityValue {
	rvn_a11y_node_t node;
	rvn_a11y_touch_t touch;
	if (![self ravenDescribe:&node touch:&touch]) return nil;
	/* (a toggle button reads "1" / "0" as on / off) */
	if (touch.traits & RVN_A11Y_TRAIT_TOGGLE) {
		if (@available(iOS 17.0, *)) return (node.states & RVN_A11Y_CHECKED) ? @"1" : @"0";
	}
	return rvn_uikit_a11y_string(node.value);
}
- (NSString*)accessibilityHint {
	rvn_a11y_node_t node;
	rvn_a11y_touch_t touch;
	return [self ravenDescribe:&node touch:&touch] ? rvn_uikit_a11y_string(node.description) : nil;
}
- (UIAccessibilityTraits)accessibilityTraits {
	rvn_a11y_node_t node;
	rvn_a11y_touch_t touch;
	if (![self ravenDescribe:&node touch:&touch]) return UIAccessibilityTraitNone;
	/* (a container is not a button: only its ravenSelf element is) */
	if (!ravenSelf && rvn_a11y_child_count(ravenNode) > 0) return (touch.traits & RVN_A11Y_TRAIT_TAB_BAR) ? UIAccessibilityTraitTabBar : UIAccessibilityTraitNone;
	return rvn_uikit_a11y_traits(&node, &touch);
}
- (CGRect)accessibilityFrame {
	rvn_a11y_node_t node;
	rvn_a11y_touch_t touch;
	UIView* view;
	if (![self ravenDescribe:&node touch:&touch] || !ravenWindow->view) return CGRectZero;
	view = (__bridge UIView*)ravenWindow->view;
	return UIAccessibilityConvertFrameToScreenCoordinates(CGRectMake(node.x, node.y, node.width, node.height), view);
}
- (BOOL)accessibilityViewIsModal {
	rvn_a11y_node_t node;
	rvn_a11y_touch_t touch;
	return !ravenSelf && [self ravenDescribe:&node touch:&touch] && (node.states & RVN_A11Y_MODAL) ? YES : NO;
}
- (BOOL)accessibilityActivate {
	return ravenWindow && rvn_a11y_activate(ravenNode) ? YES : NO;
}
- (void)accessibilityIncrement {
	if (ravenWindow) rvn_a11y_adjust(ravenNode, 1);
}
- (void)accessibilityDecrement {
	if (ravenWindow) rvn_a11y_adjust(ravenNode, -1);
}
@end

@implementation RavenUIKitView (RavenAccessibility)
- (BOOL)isAccessibilityElement { return NO; }
- (NSArray*)accessibilityElements {
	rvn_a11y_id_t root = rvn_uikit_a11y_root(ravenWindow);
	if (!root) return nil;
	return rvn_uikit_a11y_children(ravenWindow, root, self, NO);
}
@end

/* A hash of the nodes at the top of a window: when it changes (a dialog, a popup, another page), the screen changed. */
static uint32_t rvn_uikit_a11y_top(rvn_a11y_id_t root) {
	uint32_t hash = 2166136261u;
	int i, count = rvn_a11y_child_count(root);
	for (i = 0; i < count; i++) {
		rvn_a11y_id_t child = rvn_a11y_child(root, i);
		hash = (hash ^ (uint32_t)child) * 16777619u;
		hash = (hash ^ (uint32_t)(child >> 32)) * 16777619u;
	}
	return hash;
}

static void rvn_uikit_a11y_event(rvn_pwindow_t* window, int event, uint64_t node) {
	switch (event) {
		case RVN_A11Y_EVENT_FOCUS: {
			/* VoiceOver goes where the focus went (Next on the keyboard, a field focused by the application) */
			id object = node == rvn_uikit_a11y_root(window) ? nil : rvn_uikit_a11y_object(window, (rvn_a11y_id_t)node);
			if (object) UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification, object);
			break;
		}
		case RVN_A11Y_EVENT_STRUCTURE: {
			uint32_t top = rvn_uikit_a11y_top(rvn_uikit_a11y_root(window));
			rvn_uikit_a11y_forget(NULL);
			if (top != window->a11y_top) {
				window->a11y_top = top;
				UIAccessibilityPostNotification(UIAccessibilityScreenChangedNotification, nil);
			} else {
				UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification, nil);
			}
			break;
		}
		default:
			/* (a value, a name, a state: VoiceOver asks the element again when it reads it; after a swipe up / down it reads the new value) */
			break;
	}
}

static void rvn_uikit_a11y_announce(rvn_pwindow_t* window, const char* text, int assertive) {
	NSString* string = rvn_uikit_a11y_string(text);
	RVN_UNUSED(window);
	if (!string) return;
	if (@available(iOS 17.0, *)) {
		NSAttributedString* message = [[NSAttributedString alloc] initWithString:string attributes:@{
			UIAccessibilitySpeechAttributeAnnouncementPriority: assertive ? UIAccessibilityPriorityHigh : UIAccessibilityPriorityDefault }];
		UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, message);
	} else {
		UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, string);
	}
}

static int rvn_uikit_a11y_active(void) {
	return UIAccessibilityIsVoiceOverRunning() || UIAccessibilityIsSwitchControlRunning() ? R_TRUE : R_FALSE;
}

static void rvn_uikit_a11y_settings(RAccessibilitySettings* settings) {
	settings->reduce_motion = UIAccessibilityIsReduceMotionEnabled() ? R_TRUE : R_FALSE;
	settings->high_contrast = UIAccessibilityDarkerSystemColorsEnabled() ? R_TRUE : R_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Table
 * ------------------------------------------------------------------------- */

/* The keyboard moved, appeared or is going away (UIKeyboardWillChangeFrame / WillHide). */
static void rvn_uikit_keyboard_changed(NSNotification* note, int hiding) {
	rvn_pwindow_t* window;
	NSValue* end = note.userInfo[UIKeyboardFrameEndUserInfoKey];
	g_uikit.keyboard_shown = !hiding && end != nil;
	if (end) g_uikit.keyboard_frame = end.CGRectValue;
	for (window = g_uikit.windows; window; window = window->next) rvn_uikit_keyboard_measure(window);
}

/*
 * The application's lifecycle: the core gets it now, after what was queued,
 * since no frame may come before the system suspends the application.
 */
static void rvn_uikit_app_event(RAppEvent app_event) {
	rvn_pevent_t e;
	int i;
	if (!g_uikit.handler) return;
#if defined(RAVEN_BACKEND_METAL_ENABLED)
	/* no command buffer may reach the GPU in the background: the Metal backend refuses them (and waits for the GPU) until the application comes back */
	if (app_event == R_APP_DID_PAUSE) rvn_metal_set_suspended(R_TRUE);
	else if (app_event == R_APP_WILL_RESUME || app_event == R_APP_DID_RESUME) rvn_metal_set_suspended(R_FALSE);
#endif
	for (i = 0; i < g_uikit.count; i++) g_uikit.handler(&g_uikit.queue[i]);
	g_uikit.count = 0;
	memset(&e, 0, sizeof(e));
	e.type = RVN_PEVENT_APP;
	e.app_event = (int)app_event;
	g_uikit.handler(&e);
	/* (outside a frame: the display link ticks again for what the application changed, or what changed in the background; not in the background) */
	if (app_event != R_APP_DID_PAUSE && app_event != R_APP_WILL_TERMINATE) rvn_uikit_resume();
}

/* The scene became active or is about to resign (app switcher, home, a call): the core's "active" window. */
static void rvn_uikit_scene_focus(NSNotification* note, int focused) {
	rvn_pwindow_t* window;
	for (window = g_uikit.windows; window; window = window->next) {
		UIWindow* uiwindow = (__bridge UIWindow*)window->window;
		rvn_pevent_t e;
		if (note.object && uiwindow.windowScene != note.object) continue;
		memset(&e, 0, sizeof(e));
		e.type = RVN_PEVENT_FOCUS;
		e.user = window->user;
		e.focused = focused;
		rvn_uikit_post(&e);
	}
}

/* ---------------------------------------------------------------------------
 *  Files of the bundle: plain files under [NSBundle mainBundle].resourcePath
 *  (core/file.c asks for the relative paths it cannot find; the current
 *  folder of an iOS app is "/"). Not in g_uikit: textures are read on worker
 *  threads.
 * ------------------------------------------------------------------------- */

static char g_uikit_resources[1024];

static void* rvn_uikit_resource_open(const char* path) {
	char full[2048];
	FILE* file;
	struct stat info;
	if (!g_uikit_resources[0]) return NULL;
	snprintf(full, sizeof(full), "%s/%s", g_uikit_resources, path);
	file = fopen(full, "rb");
	/* (a folder opens too: only regular files are read) */
	if (file && (fstat(fileno(file), &info) != 0 || !S_ISREG(info.st_mode))) {
		fclose(file);
		file = NULL;
	}
	return file;
}

static size_t rvn_uikit_resource_size(void* file) {
	struct stat info;
	return fstat(fileno((FILE*)file), &info) == 0 && info.st_size > 0 ? (size_t)info.st_size : 0;
}

static size_t rvn_uikit_resource_read(void* file, void* buffer, size_t size) {
	return fread(buffer, 1, size, (FILE*)file);
}

static void rvn_uikit_resource_close(void* file) {
	fclose((FILE*)file);
}

static const rvn_file_provider_t g_uikit_resource_provider = {
	rvn_uikit_resource_open, rvn_uikit_resource_size, rvn_uikit_resource_read, rvn_uikit_resource_close
};

static int rvn_uikit_init(rvn_pevent_handler_t handler, rvn_pregion_handler_t regions) {
	NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
	NSOperationQueue* main = [NSOperationQueue mainQueue];
	RVN_UNUSED(regions);
	memset(&g_uikit, 0, sizeof(g_uikit));
	g_uikit.handler = handler;
	{
		NSString* resources = [NSBundle mainBundle].resourcePath;
		if (resources && [resources getFileSystemRepresentation:g_uikit_resources maxLength:sizeof(g_uikit_resources)]) {
			rvn_file_set_provider(&g_uikit_resource_provider);
		} else {
			g_uikit_resources[0] = '\0';
		}
	}
	g_uikit.observers = (void*)CFBridgingRetain(@[
		[center addObserverForName:UIKeyboardWillChangeFrameNotification object:nil queue:main usingBlock:^(NSNotification* note) {
			rvn_uikit_keyboard_changed(note, R_FALSE);
		}],
		[center addObserverForName:UIKeyboardWillHideNotification object:nil queue:main usingBlock:^(NSNotification* note) {
			rvn_uikit_keyboard_changed(note, R_TRUE);
		}],
		[center addObserverForName:UISceneDidActivateNotification object:nil queue:main usingBlock:^(NSNotification* note) {
			rvn_uikit_scene_focus(note, R_TRUE);
		}],
		[center addObserverForName:UISceneWillDeactivateNotification object:nil queue:main usingBlock:^(NSNotification* note) {
			rvn_uikit_scene_focus(note, R_FALSE);
		}],
		/* the whole application (all its scenes) */
		[center addObserverForName:UIApplicationWillResignActiveNotification object:nil queue:main usingBlock:^(NSNotification* note) {
			(void)note;
			rvn_uikit_app_event(R_APP_WILL_PAUSE);
		}],
		[center addObserverForName:UIApplicationDidEnterBackgroundNotification object:nil queue:main usingBlock:^(NSNotification* note) {
			(void)note;
			rvn_uikit_app_event(R_APP_DID_PAUSE);
		}],
		[center addObserverForName:UIApplicationWillEnterForegroundNotification object:nil queue:main usingBlock:^(NSNotification* note) {
			(void)note;
			rvn_uikit_app_event(R_APP_WILL_RESUME);
		}],
		[center addObserverForName:UIApplicationDidBecomeActiveNotification object:nil queue:main usingBlock:^(NSNotification* note) {
			(void)note;
			rvn_uikit_app_event(R_APP_DID_RESUME);
		}],
		[center addObserverForName:UIApplicationDidReceiveMemoryWarningNotification object:nil queue:main usingBlock:^(NSNotification* note) {
			(void)note;
			rvn_uikit_app_event(R_APP_LOW_MEMORY);
		}],
		[center addObserverForName:UIApplicationWillTerminateNotification object:nil queue:main usingBlock:^(NSNotification* note) {
			(void)note;
			rvn_uikit_app_event(R_APP_WILL_TERMINATE);
		}]
	]);
	rvn_uikit_services_init();
	return R_TRUE;
}

static void rvn_uikit_shutdown(void) {
	rvn_uikit_services_shutdown();
	rvn_file_set_provider(NULL);
	if (g_uikit.observers) {
		NSArray* observers = CFBridgingRelease(g_uikit.observers);
		for (id observer in observers) [[NSNotificationCenter defaultCenter] removeObserver:observer];
	}
	memset(&g_uikit, 0, sizeof(g_uikit));
}

static void* rvn_uikit_native_display(void) { return NULL; }

static rvn_pwindow_t* rvn_uikit_window_create(const rvn_pwindow_desc_t* desc) {
	UIWindowScene* scene = rvn_uikit_scene();
	if (!scene) {
		NSLog(@"Raven: no UIWindowScene is connected: create Raven windows from scene:willConnectToSession:");
		return NULL;
	}
	rvn_pwindow_t* window = (rvn_pwindow_t*)rvn_calloc(sizeof(*window));
	window->user = desc->user;
	window->transparent = (desc->flags & R_WINDOW_TRANSPARENT) != 0;

	UIWindow* uiwindow = [[UIWindow alloc] initWithWindowScene:scene];
	RavenUIKitViewController* controller = [[RavenUIKitViewController alloc] init];
	RavenUIKitView* view = [[RavenUIKitView alloc] initWithFrame:uiwindow.bounds];
	/* the scale and the drawable size before the first layout: the swapchain is made from them */
	{
		float scale = (float)scene.screen.scale;
		CAMetalLayer* layer = (CAMetalLayer*)view.layer;
		view.contentScaleFactor = scale;
		layer.contentsScale = scale;
		layer.drawableSize = CGSizeMake(lrint(uiwindow.bounds.size.width * scale), lrint(uiwindow.bounds.size.height * scale));
	}
	view->ravenWindow = window;
	view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	controller.view = view;
	uiwindow.rootViewController = controller;
	window->window = (void*)CFBridgingRetain(uiwindow);
	window->view = (void*)CFBridgingRetain(view);
	window->keyboard_focus = R_INVALID_ID;
	window->next = g_uikit.windows;
	g_uikit.windows = window;
	return window;
}

static void rvn_uikit_window_destroy(rvn_pwindow_t* window) {
	rvn_pwindow_t** link;
	if (!window) return;
	RavenUIKitView* view = (__bridge RavenUIKitView*)window->view;
	UIWindow* uiwindow = (__bridge UIWindow*)window->window;
	for (link = &g_uikit.windows; *link; link = &(*link)->next)
		if (*link == window) { *link = window->next; break; }
	view->ravenWantsKeyboard = NO;
	view->ravenResigning = YES;
	if (view.isFirstResponder) [view resignFirstResponder];
	view->ravenWindow = NULL;
	rvn_uikit_a11y_forget(window);
	uiwindow.hidden = YES;
	CFBridgingRelease(window->view);
	CFBridgingRelease(window->window);
	rvn_free(window);
}

static void rvn_uikit_window_set_title(rvn_pwindow_t* window, const char* title) { RVN_UNUSED(window); RVN_UNUSED(title); }
/* the system decides the size of a window on iOS */
static void rvn_uikit_window_set_size(rvn_pwindow_t* window, int width, int height) { RVN_UNUSED(window); RVN_UNUSED(width); RVN_UNUSED(height); }
static void rvn_uikit_window_get_size(rvn_pwindow_t* window, int* width, int* height) {
	RavenUIKitView* view = (__bridge RavenUIKitView*)window->view;
	UIWindow* uiwindow = (__bridge UIWindow*)window->window;
	float scale = rvn_uikit_scale_of(view);
	CGSize size = view.bounds.size;
	if (size.width <= 0 || size.height <= 0) size = uiwindow.bounds.size;
	*width = (int)lrint(size.width * scale);
	*height = (int)lrint(size.height * scale);
}
/* the safe area of the view (status bar, Dynamic Island, home indicator), in pixels: top, right, bottom, left */
static void rvn_uikit_window_get_safe_area(rvn_pwindow_t* window, float insets[4]) {
	RavenUIKitView* view = (__bridge RavenUIKitView*)window->view;
	float scale = rvn_uikit_scale_of(view);
	UIEdgeInsets safe = view.safeAreaInsets;
	insets[0] = (float)safe.top * scale;
	insets[1] = (float)safe.right * scale;
	insets[2] = (float)safe.bottom * scale;
	insets[3] = (float)safe.left * scale;
}
/* the part of the window the on-screen keyboard covers, in pixels (0: hidden) */
static float rvn_uikit_window_get_keyboard_height(rvn_pwindow_t* window) {
	return window->keyboard_points * rvn_uikit_scale_of((__bridge UIView*)window->view);
}
static void rvn_uikit_window_set_min_size(rvn_pwindow_t* window, int width, int height) { RVN_UNUSED(window); RVN_UNUSED(width); RVN_UNUSED(height); }
static void rvn_uikit_window_set_position(rvn_pwindow_t* window, int x, int y) { RVN_UNUSED(window); RVN_UNUSED(x); RVN_UNUSED(y); }
static void rvn_uikit_window_get_position(rvn_pwindow_t* window, int* x, int* y) { RVN_UNUSED(window); *x = 0; *y = 0; }
static float rvn_uikit_window_get_scale(rvn_pwindow_t* window) { return rvn_uikit_scale_of((__bridge UIView*)window->view); }
static void rvn_uikit_window_show(rvn_pwindow_t* window, int visible) {
	UIWindow* uiwindow = (__bridge UIWindow*)window->window;
	if (visible) [uiwindow makeKeyAndVisible];
	else uiwindow.hidden = YES;
}
static void rvn_uikit_window_set_state(rvn_pwindow_t* window, rvn_pwindow_state_t state) { RVN_UNUSED(window); RVN_UNUSED(state); }
static rvn_pwindow_state_t rvn_uikit_window_get_state(rvn_pwindow_t* window) { RVN_UNUSED(window); return RVN_PWINDOW_MAXIMIZED; }
static void rvn_uikit_window_begin_move(rvn_pwindow_t* window) { RVN_UNUSED(window); }
static void rvn_uikit_window_begin_resize(rvn_pwindow_t* window, rvn_pregion_t edge) { RVN_UNUSED(window); RVN_UNUSED(edge); }
static void rvn_uikit_window_set_cursor(rvn_pwindow_t* window, RCursor cursor) { RVN_UNUSED(window); RVN_UNUSED(cursor); }
static void rvn_uikit_window_get_native(rvn_pwindow_t* window, RNativeWindow* native) {
	memset(native, 0, sizeof(*native));
	native->system = R_WINDOW_SYSTEM_UIKIT;
	native->display = window->window;
	native->window = (uintptr_t)(__bridge void*)((__bridge UIView*)window->view).layer;
	native->transparent = R_FALSE;
}
static rvn_pwindow_t* rvn_uikit_child_create(rvn_pwindow_t* parent, int x, int y, int width, int height, void* user) {
	RVN_UNUSED(parent); RVN_UNUSED(x); RVN_UNUSED(y); RVN_UNUSED(width); RVN_UNUSED(height); RVN_UNUSED(user);
	return NULL;
}
static void rvn_uikit_child_set_rect(rvn_pwindow_t* child, int x, int y, int width, int height, int visible) {
	RVN_UNUSED(child); RVN_UNUSED(x); RVN_UNUSED(y); RVN_UNUSED(width); RVN_UNUSED(height); RVN_UNUSED(visible);
}
/* The web view of the system (platform/apple/webkit_web_view.m) goes into the view of the window. */
static void rvn_uikit_web_view_set_parent(rvn_pweb_view_t* view, rvn_pwindow_t* parent) {
	rvn_webkit_set_parent_view(view, parent ? parent->view : NULL);
}

/*
 * Shows and hides the on-screen keyboard after the events of the frame, as
 * Rwindow_wants_text_input goes. Edge-triggered: it is shown when a text
 * field takes the focus (or the focus moves to another one), hidden when
 * none has it. When the user or the system closed it while the field kept
 * the focus, it is not shown again: the field lets go of the focus instead
 * (no caret without a keyboard), and a tap on a field shows it again.
 */
static void rvn_uikit_keyboard_sync(void) {
	rvn_pwindow_t* window;
	for (window = g_uikit.windows; window; window = window->next) {
		RavenUIKitView* view = (__bridge RavenUIKitView*)window->view;
		rvn_window_t* core = (rvn_window_t*)window->user;
		int wants;
		Rid focus;
		if (!core) continue;
		/* (the queued keys were handed over) */
		view->ravenKeysQueued = NO;
		wants = Rwindow_wants_text_input(core->id);
		focus = wants ? Rwindow_get_focused_widget(core->id) : R_INVALID_ID;
		if (window->keyboard_dismissed) {
			window->keyboard_dismissed = R_FALSE;
			window->keyboard_wanted = R_FALSE;
			if (wants && focus == window->keyboard_focus) {
				Rwindow_clear_focus(core->id);
				wants = Rwindow_wants_text_input(core->id);
				focus = wants ? Rwindow_get_focused_widget(core->id) : R_INVALID_ID;
				/* (a frame for the field without its caret) */
				rvn_uikit_resume();
			}
		}
		if (wants && (!window->keyboard_wanted || focus != window->keyboard_focus)) {
			view->ravenWantsKeyboard = YES;
			/* (not first responder yet, e.g. the scene is not active: tried again on the next frame) */
			if (!view.isFirstResponder && ![view becomeFirstResponder]) {
				view->ravenWantsKeyboard = NO;
				wants = R_FALSE;
			}
		} else if (!wants && window->keyboard_wanted) {
			view->ravenWantsKeyboard = NO;
			view->ravenResigning = YES;
			[view resignFirstResponder];
			view->ravenResigning = NO;
		}
		window->keyboard_wanted = wants;
		window->keyboard_focus = focus;
		/* (the system's view of the field: keyboard for another field, changes made by Raven) */
		[view ravenSync];
	}
}

/*
 * UIKit runs the run loop and calls the application (CADisplayLink): pump
 * only hands over what UIKit delivered since the previous frame, and never
 * blocks. A timeout (Rwait_frame) means Raven has nothing to draw until then:
 * with nothing queued, the display link given to rvn_uikit_set_display_link
 * is paused, for ever (timeout < 0) or until the timeout (next timer).
 * Short waits (frame pacing, animations) keep it running.
 */
static void rvn_uikit_pump(double timeout_seconds) {
	int i;
	if (!g_uikit.handler) return;
	if (g_uikit.count == 0 && g_uikit.link && (timeout_seconds < 0.0 || timeout_seconds >= 0.05)) {
		unsigned generation = ++g_uikit.pause_generation;
		g_uikit.link.paused = YES;
		if (timeout_seconds > 0.0) {
			dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(timeout_seconds * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
				if (generation == g_uikit.pause_generation) rvn_uikit_resume();
			});
		}
	}
	for (i = 0; i < g_uikit.count; i++) g_uikit.handler(&g_uikit.queue[i]);
	g_uikit.count = 0;
	/* (the text a Paste of the edit menu read was the clipboard for its command only) */
	if (g_uikit.pasted) {
		CFBridgingRelease(g_uikit.pasted);
		g_uikit.pasted = NULL;
	}
	rvn_uikit_keyboard_sync();
}
/* any thread: Rwindow_request_redraw, Rpost, quit... */
static void rvn_uikit_wake(void) {
	if ([NSThread isMainThread]) rvn_uikit_resume();
	else dispatch_async(dispatch_get_main_queue(), ^{ rvn_uikit_resume(); });
}

static int rvn_uikit_system_dark(void) {
	UIWindowScene* scene = rvn_uikit_scene();
	UITraitCollection* traits = scene ? scene.traitCollection : [UITraitCollection currentTraitCollection];
	switch (traits.userInterfaceStyle) {
		case UIUserInterfaceStyleDark: return R_TRUE;
		case UIUserInterfaceStyleLight: return R_FALSE;
		default: return -1;
	}
}

static void rvn_uikit_clipboard_set(const char* text) {
	NSString* string = [NSString stringWithUTF8String:text ? text : ""];
	dispatch_async(dispatch_get_main_queue(), ^{ [UIPasteboard generalPasteboard].string = string; });
}
static char* rvn_uikit_clipboard_get(void) {
	NSString* string = g_uikit.pasted ? (__bridge NSString*)g_uikit.pasted : [UIPasteboard generalPasteboard].string;
	return string ? rvn_strdup([string UTF8String]) : NULL;
}

/* ---------------------------------------------------------------------------
 *  The edit menu
 *
 * For the text a finger selected (widget/text_touch.c): the rectangle of the
 * selection, in pixels of the window, becomes points of the view. The
 * entries come back as RVN_PEVENT_EDIT (cut:, copy:, paste:, selectAll: of
 * the view), and the core runs them as the keys of the focused widget.
 * SHORTCUT: a read-only field has no keyboard, so the view is not the first
 * responder: on iOS 15 its menu does not show, on iOS 16+ with English
 * titles.
 * ------------------------------------------------------------------------- */

static int rvn_uikit_edit_menu_show(rvn_pwindow_t* window, const int rect[4], unsigned items) {
	RavenUIKitView* view = window ? (__bridge RavenUIKitView*)window->view : nil;
	float scale;
	if (!view) return R_FALSE;
	scale = rvn_uikit_scale_of(view);
	[view ravenShowEditMenu:CGRectMake(rect[0] / scale, rect[1] / scale, rect[2] / scale, rect[3] / scale) items:items];
	return R_TRUE;
}

static void rvn_uikit_edit_menu_hide(rvn_pwindow_t* window) {
	RavenUIKitView* view = window ? (__bridge RavenUIKitView*)window->view : nil;
	[view ravenHideEditMenu];
}

/* ---------------------------------------------------------------------------
 *  For uikit_services.m: where the screens of the system are shown
 * ------------------------------------------------------------------------- */

/* the window given, else Raven's first window still shown (windows are listed newest first) */
static rvn_pwindow_t* rvn_uikit_window_or_first(rvn_pwindow_t* window) {
	rvn_pwindow_t* found = NULL;
	rvn_pwindow_t* each;
	if (window) return window;
	for (each = g_uikit.windows; each; each = each->next) {
		UIWindow* uiwindow = (__bridge UIWindow*)each->window;
		if (!uiwindow.hidden) found = each;
	}
	return found ? found : g_uikit.windows;
}

UIViewController* rvn_uikit_presenter(rvn_pwindow_t* window) {
	UIWindow* uiwindow = nil;
	UIViewController* top;
	window = rvn_uikit_window_or_first(window);
	if (window) uiwindow = (__bridge UIWindow*)window->window;
	if (!uiwindow) {
		UIWindowScene* scene = rvn_uikit_scene();
		for (UIWindow* each in scene.windows) {
			if (each.isKeyWindow || !uiwindow) uiwindow = each;
		}
	}
	top = uiwindow.rootViewController;
	/* (what is going away is not presented over: the caller waits for it) */
	while (top.presentedViewController && !top.presentedViewController.isBeingDismissed) top = top.presentedViewController;
	return top;
}

UIView* rvn_uikit_view(rvn_pwindow_t* window) {
	window = rvn_uikit_window_or_first(window);
	return window ? (__bridge UIView*)window->view : nil;
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
		.native_regions = R_FALSE,
		.window_set_cursor = rvn_uikit_window_set_cursor,
		.window_get_native = rvn_uikit_window_get_native,
		.child_create = rvn_uikit_child_create,
		.child_set_rect = rvn_uikit_child_set_rect,
		.pump = rvn_uikit_pump,
		.wake = rvn_uikit_wake,
		.clipboard_set = rvn_uikit_clipboard_set,
		.clipboard_get = rvn_uikit_clipboard_get,
		.window_get_safe_area = rvn_uikit_window_get_safe_area,
		.system_dark = rvn_uikit_system_dark,
		.window_get_keyboard_height = rvn_uikit_window_get_keyboard_height,
		.a11y_event = rvn_uikit_a11y_event,
		.a11y_announce = rvn_uikit_a11y_announce,
		.a11y_active = rvn_uikit_a11y_active,
		.a11y_settings = rvn_uikit_a11y_settings,
		.notify = rvn_uikit_notify,
		.notify_close = rvn_uikit_notify_close,
		.notify_features = rvn_uikit_notify_features,
		/* the services of the device (uikit_services.m); folder_select: none on iOS for now */
		.permission_status = rvn_uikit_permission_status,
		.permission_request = rvn_uikit_permission_request,
		.permission_open_settings = rvn_uikit_permission_open_settings,
		.file_open = rvn_uikit_file_open,
		.file_save = rvn_uikit_file_save,
		.camera_available = rvn_uikit_camera_available,
		.camera_take_photo = rvn_uikit_camera_take_photo,
		.photo_pick = rvn_uikit_photo_pick,
		.share = rvn_uikit_share,
		.open_url = rvn_uikit_open_url,
		.haptic_supported = rvn_uikit_haptic_supported,
		.haptic = rvn_uikit_haptic,
		.download = rvn_uikit_download,
		.clipboard_has_text = rvn_uikit_clipboard_has_text,
		.edit_menu_show = rvn_uikit_edit_menu_show,
		.edit_menu_hide = rvn_uikit_edit_menu_hide,
		.web_view_supported = rvn_webkit_supported,
		.web_view_create = rvn_webkit_create,
		.web_view_destroy = rvn_webkit_destroy,
		.web_view_set_parent = rvn_uikit_web_view_set_parent,
		.web_view_set_rect = rvn_webkit_set_rect,
		.web_view_load_url = rvn_webkit_load_url,
		.web_view_load_html = rvn_webkit_load_html,
		.web_view_command = rvn_webkit_command,
		.web_view_can = rvn_webkit_can,
		.web_view_run_javascript = rvn_webkit_run_javascript,
		.web_view_focus = rvn_webkit_focus,
		.web_view_native = rvn_webkit_native
	};
	return &platform;
}
