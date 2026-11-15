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
 * R_WINDOW_SYSTEM_UIKIT with the CAMetalLayer*, from which it makes a
 * VK_EXT_metal_surface as it does for Cocoa.
 *
 * Compiled with ARC (see CMakeLists.txt).
 */

#import <UIKit/UIKit.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>

#include "../platform.h"
/* (SHORTCUT: the platform reads the id of the core's window from its user pointer, as the Android platform reaches the core) */
#include "../../widget/widget.h"

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
@interface RavenUIKitView : UIView <UITextInput> {
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
		/* dark / light mode of the system: the core asks system_dark again */
		[self registerForTraitChanges:@[ [UITraitUserInterfaceStyle class] ]
		                   withHandler:^(__kindof id<UITraitEnvironment> environment, UITraitCollection* previous) {
			(void)environment; (void)previous;
			rvn_pevent_t e;
			memset(&e, 0, sizeof(e));
			e.type = RVN_PEVENT_APPEARANCE;
			rvn_uikit_post(&e);
		}];
	}
	return self;
}

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

static int rvn_uikit_init(rvn_pevent_handler_t handler, rvn_pregion_handler_t regions) {
	NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
	NSOperationQueue* main = [NSOperationQueue mainQueue];
	RVN_UNUSED(regions);
	memset(&g_uikit, 0, sizeof(g_uikit));
	g_uikit.handler = handler;
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
		}]
	]);
	return R_TRUE;
}

static void rvn_uikit_shutdown(void) {
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
		.window_get_keyboard_height = rvn_uikit_window_get_keyboard_height
	};
	return &platform;
}
