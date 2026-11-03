/**
 * Raven - platform/cocoa/cocoa_platform.m
 * Native macOS window system. Cocoa owns the windows and input event queue;
 * each content view carries a CAMetalLayer for Vulkan's VK_EXT_metal_surface.
 */
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#import <Metal/Metal.h>
#import <CoreGraphics/CoreGraphics.h>

#include "../platform.h"
#include "../../core/accessibility.h"

struct rvn_pwindow_t {
    NSWindow* handle;
    NSView* view;
    id delegate;
    void* user;
    uint32_t flags;
    int is_child;
    int transparent;
    int min_width, min_height;
    rvn_pwindow_state_t state;
    NSCursor* cursor;
    /* an overlay of the camera housing (RVN_PWINDOW_OVERLAY): may it take the keyboard now, the part of
       it that takes the mouse (pixels of its client area), and the timer that watches the pointer */
    int overlay;
    int overlay_key;
    NSRect overlay_input;
    int pointer_inside;
    NSTimer* overlay_poll;
};

static struct {
    rvn_pevent_handler_t handler;
    rvn_pregion_handler_t regions;
    NSAutoreleasePool* pool;
    int initialized;
    rvn_pwindow_t* resize_window;
    rvn_pregion_t resize_region;
    /* the end of the pen near the tablet is its eraser (tabletProximity) */
    int pen_eraser;
    /* the pinch of the trackpad comes as two streams, the magnification and the rotation: the parts that are going on */
    int pinch_parts;
    NSPoint resize_start;
    NSRect resize_frame;
} g_cocoa;

static float rvn_cocoa_scale(rvn_pwindow_t* window) {
    NSScreen* screen = window && window->handle ? [window->handle screen] : [NSScreen mainScreen];
    CGFloat scale = screen ? [screen backingScaleFactor] : 1.0;
    return scale > 0.0 ? (float)scale : 1.0f;
}

static void rvn_cocoa_emit(rvn_pwindow_t* window, rvn_pevent_t* event) {
    if (!window || !g_cocoa.handler) return;
    event->user = window->user;
    if (event->scale == 0.0f) event->scale = rvn_cocoa_scale(window);
    g_cocoa.handler(event);
}

static uint32_t rvn_cocoa_modifiers(NSEventModifierFlags flags) {
    uint32_t result = 0;
    if (flags & NSEventModifierFlagShift) result |= R_MOD_SHIFT;
    if (flags & NSEventModifierFlagControl) result |= R_MOD_CTRL;
    if (flags & NSEventModifierFlagOption) result |= R_MOD_ALT;
    if (flags & NSEventModifierFlagCommand) result |= R_MOD_SUPER;
    return result;
}

static RKey rvn_cocoa_key(unsigned short code, NSString* characters) {
    switch (code) {
        case 36: case 76: return R_KEY_ENTER;
        case 48: return R_KEY_TAB;
        case 51: return R_KEY_BACKSPACE;
        case 117: return R_KEY_DELETE;
        case 53: return R_KEY_ESCAPE;
        case 114: return R_KEY_INSERT;
        case 115: return R_KEY_HOME;
        case 119: return R_KEY_END;
        case 116: return R_KEY_PAGE_UP;
        case 121: return R_KEY_PAGE_DOWN;
        case 123: return R_KEY_LEFT;
        case 124: return R_KEY_RIGHT;
        case 125: return R_KEY_DOWN;
        case 126: return R_KEY_UP;
        case 57: return R_KEY_CAPS_LOCK;
        case 105: return R_KEY_F1; /* non-US keyboard fallback; corrected by characters below when printable */
        case 122: return R_KEY_F1;
        case 120: return R_KEY_F2;
        case 99: return R_KEY_F3;
        case 118: return R_KEY_F4;
        case 96: return R_KEY_F5;
        case 97: return R_KEY_F6;
        case 98: return R_KEY_F7;
        case 100: return R_KEY_F8;
        case 101: return R_KEY_F9;
        case 109: return R_KEY_F10;
        case 103: return R_KEY_F11;
        case 111: return R_KEY_F12;
        case 56: return R_KEY_LEFT_SHIFT;
        case 60: return R_KEY_RIGHT_SHIFT;
        case 59: return R_KEY_LEFT_CTRL;
        case 62: return R_KEY_RIGHT_CTRL;
        case 58: return R_KEY_LEFT_ALT;
        case 61: return R_KEY_RIGHT_ALT;
        case 55: return R_KEY_LEFT_SUPER;
        case 54: return R_KEY_RIGHT_SUPER;
        case 49: return R_KEY_SPACE;
        case 0: return R_KEY_A; case 1: return R_KEY_S; case 2: return R_KEY_D; case 3: return R_KEY_F;
        case 4: return R_KEY_H; case 5: return R_KEY_G; case 6: return R_KEY_Z; case 7: return R_KEY_X;
        case 8: return R_KEY_C; case 9: return R_KEY_V; case 11: return R_KEY_B; case 12: return R_KEY_Q;
        case 13: return R_KEY_W; case 14: return R_KEY_E; case 15: return R_KEY_R; case 16: return R_KEY_Y;
        case 17: return R_KEY_T; case 31: return R_KEY_O; case 32: return R_KEY_U; case 34: return R_KEY_I;
        case 35: return R_KEY_P; case 37: return R_KEY_L; case 38: return R_KEY_J; case 40: return R_KEY_K;
        case 45: return R_KEY_N; case 46: return R_KEY_M;
        case 18: return R_KEY_1; case 19: return R_KEY_2; case 20: return R_KEY_3; case 21: return R_KEY_4;
        case 23: return R_KEY_5; case 22: return R_KEY_6; case 26: return R_KEY_7; case 28: return R_KEY_8;
        case 25: return R_KEY_9; case 29: return R_KEY_0;
        case 27: return R_KEY_MINUS; case 24: return R_KEY_EQUAL; case 33: return R_KEY_LEFT_BRACKET;
        case 30: return R_KEY_RIGHT_BRACKET; case 42: return R_KEY_BACKSLASH; case 41: return R_KEY_SEMICOLON;
        case 39: return R_KEY_APOSTROPHE; case 43: return R_KEY_COMMA; case 47: return R_KEY_PERIOD;
        case 44: return R_KEY_SLASH; case 50: return R_KEY_GRAVE;
        default: break;
    }
    if ([characters length]) {
        unichar ch = [[characters uppercaseString] characterAtIndex:0];
        if (ch >= 'A' && ch <= 'Z') return (RKey)ch;
        if (ch >= '0' && ch <= '9') return (RKey)ch;
    }
    return R_KEY_UNKNOWN;
}

static void rvn_cocoa_emit_text(rvn_pwindow_t* window, NSString* value, uint32_t modifiers) {
    NSUInteger i, n;
    if (!value || ![value length] || (modifiers & (R_MOD_CTRL | R_MOD_ALT | R_MOD_SUPER))) return;
    n = [value length];
    for (i = 0; i < n; i++) {
        uint32_t codepoint = [value characterAtIndex:i];
        if (codepoint >= 0xD800 && codepoint <= 0xDBFF && i + 1 < n) {
            uint32_t low = [value characterAtIndex:i + 1];
            if (low >= 0xDC00 && low <= 0xDFFF) {
                codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
                i++;
            }
        }
        rvn_pevent_t event;
        memset(&event, 0, sizeof(event));
        event.type = RVN_PEVENT_TEXT;
        event.codepoint = codepoint;
        event.modifiers = modifiers;
        rvn_cocoa_emit(window, &event);
    }
}

static void rvn_cocoa_resize_drag(rvn_pwindow_t* window) {
    if (!window || g_cocoa.resize_window != window) return;
    NSPoint point = [NSEvent mouseLocation];
    CGFloat dx = point.x - g_cocoa.resize_start.x;
    CGFloat dy = point.y - g_cocoa.resize_start.y;
    NSRect frame = g_cocoa.resize_frame;
    rvn_pregion_t edge = g_cocoa.resize_region;
    int left = edge == RVN_PREGION_LEFT || edge == RVN_PREGION_TOP_LEFT || edge == RVN_PREGION_BOTTOM_LEFT;
    int right = edge == RVN_PREGION_RIGHT || edge == RVN_PREGION_TOP_RIGHT || edge == RVN_PREGION_BOTTOM_RIGHT;
    int top = edge == RVN_PREGION_TOP || edge == RVN_PREGION_TOP_LEFT || edge == RVN_PREGION_TOP_RIGHT;
    int bottom = edge == RVN_PREGION_BOTTOM || edge == RVN_PREGION_BOTTOM_LEFT || edge == RVN_PREGION_BOTTOM_RIGHT;
    CGFloat scale = rvn_cocoa_scale(window);
    CGFloat min_w = window->min_width > 0 ? window->min_width / scale : 64;
    CGFloat min_h = window->min_height > 0 ? window->min_height / scale : 48;
    if (left) { CGFloat w = MAX(min_w, g_cocoa.resize_frame.size.width - dx); frame.origin.x += g_cocoa.resize_frame.size.width - w; frame.size.width = w; }
    if (right) frame.size.width = MAX(min_w, g_cocoa.resize_frame.size.width + dx);
    if (top) frame.size.height = MAX(min_h, g_cocoa.resize_frame.size.height + dy);
    if (bottom) { CGFloat h = MAX(min_h, g_cocoa.resize_frame.size.height - dy); frame.origin.y += g_cocoa.resize_frame.size.height - h; frame.size.height = h; }
    [window->handle setFrame:frame display:YES];
}

/*
 * The window of an island in the camera housing: a panel above the menu
 * bar, without frame, that never becomes the main window and becomes the key
 * window only when the core asks (a text typed in the island).
 */
@interface RavenCocoaPanel : NSPanel {
@public rvn_pwindow_t* ravenWindow;
}
@end
@implementation RavenCocoaPanel
- (BOOL)canBecomeKeyWindow { return ravenWindow && ravenWindow->overlay_key; }
- (BOOL)canBecomeMainWindow { return NO; }
/* (AppKit keeps windows under the menu bar; this one is meant to be in it, around the camera) */
- (NSRect)constrainFrameRect:(NSRect)frameRect toScreen:(NSScreen*)screen { (void)screen; return frameRect; }
@end

@interface RavenCocoaView : NSView <NSDraggingDestination> {
@public rvn_pwindow_t* ravenWindow;
    NSTrackingArea* trackingArea;
}
@end

@implementation RavenCocoaView
- (BOOL)isFlipped { return YES; }
- (void)updateTrackingAreas {
    if (trackingArea) { [self removeTrackingArea:trackingArea]; [trackingArea release]; trackingArea = nil; }
    trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds] options:NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways owner:self userInfo:nil];
    [self addTrackingArea:trackingArea];
    [super updateTrackingAreas];
}
- (void)dealloc { if (trackingArea) [trackingArea release]; [super dealloc]; }
- (BOOL)acceptsFirstResponder { return YES; }
/* (the island of the camera housing takes the click at once, without becoming the key window first) */
- (BOOL)acceptsFirstMouse:(NSEvent*)event { (void)event; return ravenWindow && ravenWindow->overlay; }
- (BOOL)wantsUpdateLayer { return YES; }
- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;
    rvn_pevent_t event; memset(&event, 0, sizeof(event)); event.type = RVN_PEVENT_EXPOSE;
    rvn_cocoa_emit(ravenWindow, &event);
}
- (void)updateLayer {
    rvn_pevent_t event; memset(&event, 0, sizeof(event)); event.type = RVN_PEVENT_EXPOSE;
    rvn_cocoa_emit(ravenWindow, &event);
}
/*
 * A pen on a tablet: macOS makes it the mouse itself, with a pressure and a
 * tilt on the events (subtype tablet point). The core is told about the pen
 * as well, for the pressure and the palm (pen_native_mouse: it does not make
 * a second mouse of it).
 */
- (void)rvnPen:(NSEvent*)event phase:(int)phase {
    if ([event type] != NSEventTypeTabletPoint && [event subtype] != NSEventSubtypeTabletPoint) return;
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil]; float s = rvn_cocoa_scale(ravenWindow);
    rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_PEN; e.phase = phase; e.pen_native_mouse = 1;
    e.fx = (float)(p.x * s); e.fy = (float)(p.y * s);
    e.pressure = (phase == R_PEN_DOWN || phase == R_PEN_MOVE) ? (float)[event pressure] : 0.0f;
    /* (the tilt goes from -1 to 1, about 60 degrees at the ends; up is negative on the screen) */
    e.tilt_x = (float)[event tilt].x * 60.0f; e.tilt_y = (float)-[event tilt].y * 60.0f;
    e.rotation = (float)[event rotation];
    e.eraser = g_cocoa.pen_eraser;
    if ([event buttonMask] & NSEventButtonMaskPenLowerSide) e.pen_buttons |= R_PEN_BUTTON_BARREL;
    if ([event buttonMask] & NSEventButtonMaskPenUpperSide) e.pen_buttons |= R_PEN_BUTTON_BARREL2;
    e.modifiers = rvn_cocoa_modifiers([event modifierFlags]); rvn_cocoa_emit(ravenWindow, &e);
}
- (void)tabletProximity:(NSEvent*)event {
    g_cocoa.pen_eraser = [event pointingDeviceType] == NSPointingDeviceTypeEraser;
    if (![event isEnteringProximity]) {
        rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_PEN; e.phase = R_PEN_LEAVE; e.pen_native_mouse = 1; rvn_cocoa_emit(ravenWindow, &e);
    }
}
- (void)tabletPoint:(NSEvent*)event { [self rvnPen:event phase:[event pressure] > 0.0 ? R_PEN_MOVE : R_PEN_HOVER]; }
/*
 * The gestures of the trackpad, for the core. The pinch comes as two streams
 * of events with their own phases, the magnification and the rotation: the
 * core sees one gesture, which begins with the first and ends with the last.
 */
- (void)rvnPinch:(NSEvent*)event part:(int)part scale:(float)scale rotation:(float)rotation {
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil]; float s = rvn_cocoa_scale(ravenWindow);
    NSEventPhase phase = [event phase];
    int was = g_cocoa.pinch_parts;
    rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_GESTURE; e.gesture_kind = R_GESTURE_PINCH; e.gesture_fingers = 2;
    if (phase & NSEventPhaseBegan) g_cocoa.pinch_parts |= part;
    if (phase & (NSEventPhaseEnded | NSEventPhaseCancelled)) g_cocoa.pinch_parts &= ~part;
    if (!was && g_cocoa.pinch_parts) e.phase = R_GESTURE_BEGIN;
    else if (was && !g_cocoa.pinch_parts) e.phase = R_GESTURE_END;
    else e.phase = R_GESTURE_UPDATE;
    e.gesture_cancelled = e.phase == R_GESTURE_END && (phase & NSEventPhaseCancelled) != 0;
    e.fx = (float)(p.x * s); e.fy = (float)(p.y * s);
    e.gesture_scale = scale;
    e.rotation = rotation;
    e.modifiers = rvn_cocoa_modifiers([event modifierFlags]); rvn_cocoa_emit(ravenWindow, &e);
}
- (void)magnifyWithEvent:(NSEvent*)event {
    [self rvnPinch:event part:1 scale:(float)(1.0 + [event magnification]) rotation:0.0f];
}
/* (AppKit turns counterclockwise; Raven clockwise) */
- (void)rotateWithEvent:(NSEvent*)event {
    [self rvnPinch:event part:2 scale:1.0f rotation:(float)-[event rotation]];
}
/*
 * Three fingers swiped across (when the system is set to swipe between pages
 * with three fingers): AppKit sends the swipe whole, once recognized. The core
 * gets it as a swipe that went far, toward the right (back) when deltaX > 0.
 */
- (void)swipeWithEvent:(NSEvent*)event {
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil]; float s = rvn_cocoa_scale(ravenWindow);
    float dx = (float)[event deltaX], dy = (float)[event deltaY];
    int phases[3] = { R_GESTURE_BEGIN, R_GESTURE_UPDATE, R_GESTURE_END }, i;
    if (dx == 0.0f && dy == 0.0f) return;
    for (i = 0; i < 3; i++) {
        rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_GESTURE; e.gesture_kind = R_GESTURE_SWIPE; e.gesture_fingers = 3;
        e.phase = phases[i];
        e.fx = (float)(p.x * s); e.fy = (float)(p.y * s);
        e.gesture_scale = 1.0f;
        /* (a whole swipe: far beyond any distance the settings may ask) */
        if (phases[i] == R_GESTURE_UPDATE) { e.wheel_x = dx * 10000.0f * s; e.wheel_y = -dy * 10000.0f * s; }
        e.modifiers = rvn_cocoa_modifiers([event modifierFlags]); rvn_cocoa_emit(ravenWindow, &e);
        if (!ravenWindow) return;
    }
}
- (void)mouseMoved:(NSEvent*)event {
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil]; float s = rvn_cocoa_scale(ravenWindow);
    rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_MOUSE_MOVE; e.x = (int)lrint(p.x * s); e.y = (int)lrint(p.y * s); e.modifiers = rvn_cocoa_modifiers([event modifierFlags]); rvn_cocoa_emit(ravenWindow, &e);
    [self rvnPen:event phase:[event type] == NSEventTypeLeftMouseDragged ? R_PEN_MOVE : R_PEN_HOVER];
}
- (void)mouseDragged:(NSEvent*)event { rvn_cocoa_resize_drag(ravenWindow); [self mouseMoved:event]; }
- (void)rightMouseDragged:(NSEvent*)event { rvn_cocoa_resize_drag(ravenWindow); [self mouseMoved:event]; }
- (void)otherMouseDragged:(NSEvent*)event { rvn_cocoa_resize_drag(ravenWindow); [self mouseMoved:event]; }
- (void)mouseEntered:(NSEvent*)event { (void)event; }
- (void)mouseExited:(NSEvent*)event {
    (void)event; rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_MOUSE_LEAVE; rvn_cocoa_emit(ravenWindow, &e);
}
- (void)rvnMouse:(NSEvent*)event button:(int)button pressed:(int)pressed {
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil]; float s = rvn_cocoa_scale(ravenWindow);
    rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_MOUSE_BUTTON;
    e.button = button; e.pressed = pressed; e.x = (int)lrint(p.x * s); e.y = (int)lrint(p.y * s);
    e.modifiers = rvn_cocoa_modifiers([event modifierFlags]); rvn_cocoa_emit(ravenWindow, &e);
}
- (void)mouseDown:(NSEvent*)event { [self rvnMouse:event button:0 pressed:1]; [self rvnPen:event phase:R_PEN_DOWN]; }
- (void)mouseUp:(NSEvent*)event { [self rvnMouse:event button:0 pressed:0]; [self rvnPen:event phase:R_PEN_UP]; if (g_cocoa.resize_window == ravenWindow) g_cocoa.resize_window = NULL; }
- (void)rightMouseDown:(NSEvent*)event { [self rvnMouse:event button:1 pressed:1]; }
- (void)rightMouseUp:(NSEvent*)event { [self rvnMouse:event button:1 pressed:0]; if (g_cocoa.resize_window == ravenWindow) g_cocoa.resize_window = NULL; }
- (void)otherMouseDown:(NSEvent*)event { [self rvnMouse:event button:(int)[event buttonNumber] pressed:1]; }
- (void)otherMouseUp:(NSEvent*)event { [self rvnMouse:event button:(int)[event buttonNumber] pressed:0]; if (g_cocoa.resize_window == ravenWindow) g_cocoa.resize_window = NULL; }
/*
 * A trackpad (or a Magic Mouse) gives its deltas in points, the content
 * following the fingers: RVN_WHEEL_LINE points make a line, and the wheel is
 * precise. A wheel gives lines. The momentum after a flick is made by AppKit
 * (events of its momentum phase): nothing for the core to make.
 */
- (void)scrollWheel:(NSEvent*)event {
    rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_MOUSE_WHEEL;
    e.wheel_x = (float)[event scrollingDeltaX]; e.wheel_y = (float)[event scrollingDeltaY];
    if ([event hasPreciseScrollingDeltas]) {
        e.wheel_x /= RVN_WHEEL_LINE; e.wheel_y /= RVN_WHEEL_LINE;
        e.wheel_precise = 1;
    }
    if (e.wheel_x == 0.0f && e.wheel_y == 0.0f) return;
    e.modifiers = rvn_cocoa_modifiers([event modifierFlags]); rvn_cocoa_emit(ravenWindow, &e);
}
- (void)keyDown:(NSEvent*)event {
    uint32_t mods = rvn_cocoa_modifiers([event modifierFlags]);
    rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_KEY; e.key = rvn_cocoa_key([event keyCode], [event charactersIgnoringModifiers]);
    e.pressed = 1; e.repeat = [event isARepeat]; e.modifiers = mods; rvn_cocoa_emit(ravenWindow, &e);
    if (!e.repeat) rvn_cocoa_emit_text(ravenWindow, [event characters], mods);
}
- (void)keyUp:(NSEvent*)event {
    rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_KEY; e.key = rvn_cocoa_key([event keyCode], [event charactersIgnoringModifiers]);
    e.pressed = 0; e.modifiers = rvn_cocoa_modifiers([event modifierFlags]); rvn_cocoa_emit(ravenWindow, &e);
}
- (void)flagsChanged:(NSEvent*)event {
    RKey key = rvn_cocoa_key([event keyCode], [event charactersIgnoringModifiers]);
    rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_KEY; e.key = key;
    e.modifiers = rvn_cocoa_modifiers([event modifierFlags]);
    e.pressed = (key == R_KEY_LEFT_SHIFT || key == R_KEY_RIGHT_SHIFT) ? ((e.modifiers & R_MOD_SHIFT) != 0) :
        (key == R_KEY_LEFT_CTRL || key == R_KEY_RIGHT_CTRL) ? ((e.modifiers & R_MOD_CTRL) != 0) :
        (key == R_KEY_LEFT_ALT || key == R_KEY_RIGHT_ALT) ? ((e.modifiers & R_MOD_ALT) != 0) :
        ((e.modifiers & R_MOD_SUPER) != 0);
    rvn_cocoa_emit(ravenWindow, &e);
}
- (NSDragOperation)rvnDrag:(id<NSDraggingInfo>)sender leaving:(int)leaving {
    NSPoint p = [self convertPoint:[sender draggingLocation] fromView:nil];
    float scale = rvn_cocoa_scale(ravenWindow);
    int accepted = R_FALSE;
    rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_DRAG;
    e.x = (int)lrint(p.x * scale); e.y = (int)lrint(p.y * scale);
    e.leaving = leaving; e.accepted = &accepted; rvn_cocoa_emit(ravenWindow, &e);
    return accepted ? NSDragOperationCopy : NSDragOperationNone;
}
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender { return [self rvnDrag:sender leaving:0]; }
- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender { return [self rvnDrag:sender leaving:0]; }
- (void)draggingExited:(id<NSDraggingInfo>)sender { (void)[self rvnDrag:sender leaving:1]; }
- (BOOL)prepareForDragOperation:(id<NSDraggingInfo>)sender { (void)sender; return YES; }
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
    NSPasteboard* board = [sender draggingPasteboard];
    NSArray* urls = [board readObjectsForClasses:@[[NSURL class]] options:@{NSPasteboardURLReadingFileURLsOnlyKey:@(YES)}];
    if (urls && [urls count]) {
        NSUInteger i, n = [urls count];
        const char** paths = (const char**)calloc(n, sizeof(char*));
        NSMutableArray* strings = [NSMutableArray arrayWithCapacity:n];
        for (i = 0; i < n; i++) {
            NSString* s = [[urls objectAtIndex:i] path];
            if (s) { NSData* utf8 = [s dataUsingEncoding:NSUTF8StringEncoding]; NSString* stable = [[[NSString alloc] initWithData:utf8 encoding:NSUTF8StringEncoding] autorelease]; [strings addObject:stable ? stable : @""]; paths[i] = [[strings objectAtIndex:i] UTF8String]; }
        }
        rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_DROP; e.drop_paths = paths; e.drop_count = (int)n; rvn_cocoa_emit(ravenWindow, &e); free(paths); return YES;
    }
    NSString* text = [board stringForType:NSPasteboardTypeString];
    if (text) { rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_DROP; e.drop_text = [text UTF8String]; rvn_cocoa_emit(ravenWindow, &e); return YES; }
    return NO;
}
@end

@interface RavenCocoaDelegate : NSObject <NSWindowDelegate> {
@public rvn_pwindow_t* ravenWindow;
}
@end
@implementation RavenCocoaDelegate
- (BOOL)windowShouldClose:(id)sender {
    (void)sender; rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_CLOSE; rvn_cocoa_emit(ravenWindow, &e); return NO;
}
- (void)windowDidChangeBackingProperties:(NSNotification*)notification {
    (void)notification; if (!ravenWindow || ravenWindow->is_child) return;
    float scale = rvn_cocoa_scale(ravenWindow);
    CAMetalLayer* layer = (CAMetalLayer*)[ravenWindow->view layer]; layer.contentsScale = scale;
    [ravenWindow->view setNeedsDisplay:YES];
    NSRect bounds = [ravenWindow->view bounds];
    rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_RESIZE;
    e.width = (int)lrint(bounds.size.width * scale); e.height = (int)lrint(bounds.size.height * scale); e.scale = scale;
    rvn_cocoa_emit(ravenWindow, &e);
}
- (void)windowDidResize:(NSNotification*)notification {
    (void)notification; if (!ravenWindow || ravenWindow->is_child) return;
    NSRect bounds = [ravenWindow->view bounds]; CGFloat s = rvn_cocoa_scale(ravenWindow);
    rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_RESIZE;
    e.width = (int)lrint(bounds.size.width * s); e.height = (int)lrint(bounds.size.height * s); e.scale = (float)s; rvn_cocoa_emit(ravenWindow, &e);
}
- (void)windowDidBecomeKey:(NSNotification*)notification { (void)notification; rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_FOCUS; e.focused = 1; rvn_cocoa_emit(ravenWindow, &e); }
- (void)windowDidResignKey:(NSNotification*)notification { (void)notification; rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_FOCUS; e.focused = 0; rvn_cocoa_emit(ravenWindow, &e); }
- (void)windowDidMiniaturize:(NSNotification*)notification { (void)notification; ravenWindow->state = RVN_PWINDOW_MINIMIZED; rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_STATE; e.state = ravenWindow->state; rvn_cocoa_emit(ravenWindow, &e); }
- (void)windowDidDeminiaturize:(NSNotification*)notification { (void)notification; ravenWindow->state = RVN_PWINDOW_NORMAL; rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_STATE; e.state = ravenWindow->state; rvn_cocoa_emit(ravenWindow, &e); }
- (void)windowDidEndLiveResize:(NSNotification*)notification { (void)notification; rvn_pevent_t e; memset(&e, 0, sizeof(e)); e.type = RVN_PEVENT_MODAL_TICK; rvn_cocoa_emit(ravenWindow, &e); }
@end

static int rvn_cocoa_init(rvn_pevent_handler_t handler, rvn_pregion_handler_t regions) {
    g_cocoa.handler = handler;
    g_cocoa.regions = regions;
    g_cocoa.pool = [[NSAutoreleasePool alloc] init];
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp finishLaunching];
    [NSApp activateIgnoringOtherApps:YES];
    g_cocoa.initialized = R_TRUE;
    return R_TRUE;
}
static void rvn_cocoa_shutdown(void) {
    g_cocoa.handler = NULL; g_cocoa.regions = NULL; g_cocoa.initialized = R_FALSE;
    [g_cocoa.pool drain]; g_cocoa.pool = nil;
}
static void* rvn_cocoa_native_display(void) { return NULL; }

static void rvn_cocoa_install_layer(NSView* view, float scale, int transparent) {
    [view setWantsLayer:YES];
    CAMetalLayer* layer = [CAMetalLayer layer];
    layer.contentsScale = scale > 0.0f ? scale : 1.0f;
    layer.opaque = transparent ? NO : YES;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device) layer.device = device;
    [view setLayer:layer];
}

static rvn_pwindow_t* rvn_cocoa_window_create(const rvn_pwindow_desc_t* desc) {
    rvn_pwindow_t* window = (rvn_pwindow_t*)rvn_calloc(sizeof(*window));
    NSWindowStyleMask style;
    /* Window descriptions carry logical units (already multiplied by UI scale).
       AppKit uses points here; the backing scale turns these into framebuffer pixels. */
    NSRect rect = NSMakeRect(100, 100, rvn_maxi(desc->width, 1), rvn_maxi(desc->height, 1));
    BOOL borderless = (desc->flags & (R_WINDOW_BORDERLESS | R_WINDOW_CUSTOM_HEADER)) != 0;
    if (borderless) style = NSWindowStyleMaskBorderless | ((desc->flags & R_WINDOW_NO_RESIZE) ? 0 : NSWindowStyleMaskResizable);
    else {
        style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
        if (!(desc->flags & R_WINDOW_NO_RESIZE)) style |= NSWindowStyleMaskResizable;
    }
    window->flags = desc->flags; window->user = desc->user;
    window->transparent = (desc->flags & R_WINDOW_TRANSPARENT) != 0;
    window->state = RVN_PWINDOW_NORMAL;
    window->overlay = (desc->flags & RVN_PWINDOW_OVERLAY) != 0;
    if (window->overlay) {
        RavenCocoaPanel* panel = [[RavenCocoaPanel alloc] initWithContentRect:rect styleMask:NSWindowStyleMaskBorderless | NSWindowStyleMaskNonactivatingPanel
            backing:NSBackingStoreBuffered defer:NO];
        if (panel) panel->ravenWindow = window;
        window->handle = panel;
        window->transparent = R_TRUE;
    } else {
        window->handle = [[NSWindow alloc] initWithContentRect:rect styleMask:style backing:NSBackingStoreBuffered defer:NO];
    }
    if (!window->handle) { rvn_free(window); return NULL; }
    [window->handle setReleasedWhenClosed:NO];
    [window->handle setTitle:[NSString stringWithUTF8String:desc->title ? desc->title : "Raven"]];
    [window->handle setAcceptsMouseMovedEvents:YES];
    [window->handle setHasShadow:YES];
    [window->handle setMovableByWindowBackground:NO];
    [window->handle setOpaque:!window->transparent];
    if (window->transparent) [window->handle setBackgroundColor:[NSColor clearColor]];
    if (desc->flags & R_WINDOW_NO_RESIZE) [window->handle setMinSize:rect.size];
    window->view = [[RavenCocoaView alloc] initWithFrame:NSMakeRect(0, 0, rect.size.width, rect.size.height)];
    ((RavenCocoaView*)window->view)->ravenWindow = window;
    rvn_cocoa_install_layer(window->view, rvn_cocoa_scale(window), window->transparent);
    [window->view registerForDraggedTypes:@[NSPasteboardTypeFileURL, NSPasteboardTypeString]];
    [window->handle setContentView:window->view];
    window->delegate = [[RavenCocoaDelegate alloc] init];
    ((RavenCocoaDelegate*)window->delegate)->ravenWindow = window;
    [window->handle setDelegate:window->delegate];
    if (window->overlay) {
        NSPanel* panel = (NSPanel*)window->handle;
        /* above the menu bar, on every space (full screen ones too), out of the lists of windows, hidden while the application is behind */
        [panel setFloatingPanel:YES];
        [panel setLevel:NSMainMenuWindowLevel + 2];
        [panel setHidesOnDeactivate:YES];
        [panel setBecomesKeyOnlyIfNeeded:YES];
        [panel setHasShadow:NO];
        [panel setOpaque:NO];
        [panel setBackgroundColor:[NSColor clearColor]];
        [panel setMovable:NO];
        [panel setExcludedFromWindowsMenu:YES];
        [panel setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorStationary |
            NSWindowCollectionBehaviorFullScreenAuxiliary | NSWindowCollectionBehaviorIgnoresCycle];
        /* the pointer goes through until the core says which part takes it (overlay_set_input) */
        [panel setIgnoresMouseEvents:YES];
        return window;
    }
    if (!(desc->flags & R_WINDOW_HIDDEN)) [window->handle makeKeyAndOrderFront:nil];
    if (desc->flags & R_WINDOW_MAXIMIZED) [window->handle zoom:nil];
    return window;
}
static void rvn_cocoa_a11y_forget(rvn_pwindow_t* window);
static void rvn_cocoa_window_destroy(rvn_pwindow_t* window) {
    if (!window) return;
    rvn_cocoa_a11y_forget(window);
    if (window->overlay_poll) { [window->overlay_poll invalidate]; window->overlay_poll = nil; }
    if (window->handle) { [window->handle setDelegate:nil]; [window->handle close]; [window->handle release]; }
    if (window->delegate) [window->delegate release];
    if (window->view) {
        if (window->is_child) [window->view removeFromSuperview];
        [window->view release];
    }
    rvn_free(window);
}
static void rvn_cocoa_window_set_title(rvn_pwindow_t* window, const char* title) { [window->handle setTitle:[NSString stringWithUTF8String:title ? title : ""]]; }
static void rvn_cocoa_window_set_size(rvn_pwindow_t* window, int width, int height) {
    CGFloat scale = rvn_cocoa_scale(window);
    [window->handle setContentSize:NSMakeSize(rvn_maxi(width, 1) / scale, rvn_maxi(height, 1) / scale)];
}
static void rvn_cocoa_window_get_size(rvn_pwindow_t* window, int* width, int* height) {
    NSRect bounds = [window->view bounds]; float s = rvn_cocoa_scale(window);
    *width = (int)lrint(bounds.size.width * s); *height = (int)lrint(bounds.size.height * s);
}
static void rvn_cocoa_window_set_min_size(rvn_pwindow_t* window, int width, int height) {
    CGFloat scale = rvn_cocoa_scale(window);
    window->min_width = width; window->min_height = height;
    [window->handle setContentMinSize:NSMakeSize(width / scale, height / scale)];
}
static void rvn_cocoa_window_set_position(rvn_pwindow_t* window, int x, int y) {
    NSRect frame = [window->handle frame]; NSScreen* screen = [window->handle screen] ? [window->handle screen] : [NSScreen mainScreen];
    CGFloat screen_h = screen ? [screen frame].size.height : frame.size.height;
    frame.origin = NSMakePoint(x, screen_h - y - frame.size.height); [window->handle setFrameOrigin:frame.origin];
}
static void rvn_cocoa_window_get_position(rvn_pwindow_t* window, int* x, int* y) {
    NSRect frame = [window->handle frame]; NSScreen* screen = [window->handle screen] ? [window->handle screen] : [NSScreen mainScreen];
    CGFloat screen_h = screen ? [screen frame].size.height : frame.size.height;
    *x = (int)lrint(frame.origin.x); *y = (int)lrint(screen_h - frame.origin.y - frame.size.height);
}
static float rvn_cocoa_window_get_scale(rvn_pwindow_t* window) { return rvn_cocoa_scale(window); }
static void rvn_cocoa_window_show(rvn_pwindow_t* window, int visible) {
    /* (an overlay shows without taking the keyboard: overlay_set_key gives it) */
    if (visible && window->overlay) [window->handle orderFront:nil];
    else if (visible) [window->handle makeKeyAndOrderFront:nil];
    else [window->handle orderOut:nil];
}
static void rvn_cocoa_window_set_state(rvn_pwindow_t* window, rvn_pwindow_state_t state) {
    if (state == RVN_PWINDOW_MINIMIZED) { [window->handle miniaturize:nil]; window->state = state; }
    else if (state == RVN_PWINDOW_MAXIMIZED) { if (![window->handle isZoomed]) [window->handle zoom:nil]; window->state = state; }
    else { if ([window->handle isMiniaturized]) [window->handle deminiaturize:nil]; if ([window->handle isZoomed]) [window->handle zoom:nil]; window->state = state; }
}
static rvn_pwindow_state_t rvn_cocoa_window_get_state(rvn_pwindow_t* window) {
    if ([window->handle isMiniaturized]) return RVN_PWINDOW_MINIMIZED;
    if ([window->handle isZoomed]) return RVN_PWINDOW_MAXIMIZED;
    return RVN_PWINDOW_NORMAL;
}
static void rvn_cocoa_window_begin_move(rvn_pwindow_t* window) {
    NSEvent* event = [NSApp currentEvent];
    if (event && [event type] == NSEventTypeLeftMouseDown) [window->handle performWindowDragWithEvent:event];
}
static void rvn_cocoa_window_begin_resize(rvn_pwindow_t* window, rvn_pregion_t edge) {
    if (!window || !window->handle || edge < RVN_PREGION_LEFT || edge > RVN_PREGION_BOTTOM_RIGHT) return;
    g_cocoa.resize_window = window;
    g_cocoa.resize_region = edge;
    g_cocoa.resize_start = [NSEvent mouseLocation];
    g_cocoa.resize_frame = [window->handle frame];
}
static int rvn_cocoa_window_set_backdrop(rvn_pwindow_t* window, int backdrop, int dark) {
    RVN_UNUSED(dark);
    if (!window->transparent) return (int)R_BACKDROP_NONE;
    if (backdrop == (int)R_BACKDROP_NONE) { [window->handle setBackgroundColor:[NSColor windowBackgroundColor]]; [window->handle setOpaque:YES]; return (int)R_BACKDROP_NONE; }
    [window->handle setBackgroundColor:[NSColor clearColor]]; [window->handle setOpaque:NO];
    return (int)R_BACKDROP_TRANSPARENT;
}
static int rvn_cocoa_window_set_opacity(rvn_pwindow_t* window, float opacity) { [window->handle setAlphaValue:rvn_clampf(opacity, 0.0f, 1.0f)]; return R_TRUE; }
static void rvn_cocoa_window_set_cursor(rvn_pwindow_t* window, RCursor cursor) {
    NSCursor* native = [NSCursor arrowCursor];
    switch (cursor) {
        case R_CURSOR_IBEAM: native = [NSCursor IBeamCursor]; break;
        case R_CURSOR_HAND: native = [NSCursor pointingHandCursor]; break;
        case R_CURSOR_CROSSHAIR: native = [NSCursor crosshairCursor]; break;
        case R_CURSOR_RESIZE_H: native = [NSCursor resizeLeftRightCursor]; break;
        case R_CURSOR_RESIZE_V: native = [NSCursor resizeUpDownCursor]; break;
        case R_CURSOR_MOVE: native = [NSCursor openHandCursor]; break;
        case R_CURSOR_NOT_ALLOWED: native = [NSCursor operationNotAllowedCursor]; break;
        default: break;
    }
    window->cursor = native;
    [native set];
}
static void rvn_cocoa_window_get_native(rvn_pwindow_t* window, RNativeWindow* native) {
    memset(native, 0, sizeof(*native)); native->system = R_WINDOW_SYSTEM_COCOA;
    native->display = (void*)window->handle;
    /* VK_EXT_metal_surface requires CAMetalLayer*, not NSWindow*. */
    native->window = (uintptr_t)[window->view layer];
    native->transparent = window->transparent;
}
static rvn_pwindow_t* rvn_cocoa_child_create(rvn_pwindow_t* parent, int x, int y, int width, int height, void* user) {
    rvn_pwindow_t* child = (rvn_pwindow_t*)rvn_calloc(sizeof(*child));
    child->user = user; child->is_child = R_TRUE; child->transparent = parent->transparent;
    CGFloat scale = rvn_cocoa_scale(parent);
    child->view = [[RavenCocoaView alloc] initWithFrame:NSMakeRect(x / scale, y / scale, rvn_maxi(width, 1) / scale, rvn_maxi(height, 1) / scale)];
    ((RavenCocoaView*)child->view)->ravenWindow = child;
    rvn_cocoa_install_layer(child->view, rvn_cocoa_scale(parent), child->transparent);
    [child->view registerForDraggedTypes:@[NSPasteboardTypeFileURL, NSPasteboardTypeString]];
    [parent->view addSubview:child->view];
    return child;
}
static void rvn_cocoa_child_set_rect(rvn_pwindow_t* child, int x, int y, int width, int height, int visible) {
    if (visible) {
        CGFloat scale = rvn_cocoa_scale(child);
        [child->view setFrame:NSMakeRect(x / scale, y / scale, rvn_maxi(width, 1) / scale, rvn_maxi(height, 1) / scale)];
        CAMetalLayer* layer = (CAMetalLayer*)[child->view layer]; layer.contentsScale = scale;
        [child->view setHidden:NO];
    }
    else [child->view setHidden:YES];
}
static void rvn_cocoa_pump(double timeout_seconds) {
    NSDate* limit = timeout_seconds < 0.0 ? [NSDate distantFuture] :
        (timeout_seconds == 0.0 ? [NSDate distantPast] : [NSDate dateWithTimeIntervalSinceNow:timeout_seconds]);
    for (;;) {
        NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:limit inMode:NSDefaultRunLoopMode dequeue:YES];
        if (!event) break;
        if ([event type] != NSEventTypeApplicationDefined) {
            [NSApp sendEvent:event];
            [NSApp updateWindows];
        }
        /* A zero timeout drains every event already queued, rather than only
           the first one. A positive timeout does not extend while processing. */
        if (timeout_seconds > 0.0 && [limit timeIntervalSinceNow] <= 0.0) break;
        /* Like the other platforms, wait only for the first event: once
           something arrived (an input or a wake), drain what is already
           queued and return, so Raven can update and draw. */
        limit = [NSDate distantPast];
    }
    [g_cocoa.pool drain]; g_cocoa.pool = [[NSAutoreleasePool alloc] init];
}
static void rvn_cocoa_wake(void) {
    NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:0 context:nil subtype:0 data1:0 data2:0];
    [NSApp postEvent:event atStart:YES];
}

/* ---------------------------------------------------------------------------
 *  The island in the camera housing (platform.h, screen_notch)
 * ------------------------------------------------------------------------- */

static NSScreen* rvn_cocoa_screen_of(rvn_pwindow_t* window) {
    NSScreen* screen = window && window->handle ? [window->handle screen] : nil;
    return screen ? screen : [NSScreen mainScreen];
}

/* The housing of the screen: what the menu bar leaves between its two parts (macOS 12 and later). */
static int rvn_cocoa_screen_notch(rvn_pwindow_t* window, int* width, int* height) {
    NSScreen* screen = rvn_cocoa_screen_of(window);
    *width = *height = 0;
    if (!screen) return R_FALSE;
    if (@available(macOS 12.0, *)) {
        NSEdgeInsets insets = [screen safeAreaInsets];
        NSRect left = [screen auxiliaryTopLeftArea], right = [screen auxiliaryTopRightArea];
        CGFloat scale = [screen backingScaleFactor];
        /* (the housing is in the middle of the top edge: what the two areas beside it leave) */
        CGFloat w = [screen frame].size.width - left.size.width - right.size.width;
        if (insets.top <= 0.0 || NSIsEmptyRect(left) || NSIsEmptyRect(right) || w <= 0.0) return R_FALSE;
        *width = (int)lrint(w * (scale > 0.0 ? scale : 1.0));
        *height = (int)lrint(insets.top * (scale > 0.0 ? scale : 1.0));
        return R_TRUE;
    }
    return R_FALSE;
}

/* Against the top edge of the screen of the owner, centered on the housing. */
static void rvn_cocoa_overlay_place(rvn_pwindow_t* overlay, rvn_pwindow_t* owner, int width, int height) {
    NSScreen* screen = rvn_cocoa_screen_of(owner);
    CGFloat scale;
    NSRect bounds, frame;
    if (!screen || !overlay->handle) return;
    scale = [screen backingScaleFactor];
    if (scale <= 0.0) scale = 1.0;
    bounds = [screen frame];
    frame.size = NSMakeSize(rvn_maxi(width, 1) / scale, rvn_maxi(height, 1) / scale);
    frame.origin = NSMakePoint(floor(NSMidX(bounds) - frame.size.width * 0.5), NSMaxY(bounds) - frame.size.height);
    [overlay->handle setFrame:frame display:YES];
}

/*
 * The pointer over an overlay: in the part that takes the mouse, the panel
 * takes it; elsewhere it goes through to what is under it (and the core is
 * told that the pointer left, as when it leaves a window). Looked at sixty
 * times a second: a panel that lets the mouse through hears nothing of it.
 */
static void rvn_cocoa_overlay_poll(rvn_pwindow_t* window) {
    NSPoint mouse = [NSEvent mouseLocation];
    NSRect frame = [window->handle frame];
    CGFloat scale = rvn_cocoa_scale(window);
    CGFloat x = (mouse.x - frame.origin.x) * scale, y = (NSMaxY(frame) - mouse.y) * scale;
    int inside = [window->handle isVisible] && x >= NSMinX(window->overlay_input) && y >= NSMinY(window->overlay_input) &&
        x < NSMaxX(window->overlay_input) && y < NSMaxY(window->overlay_input);
    rvn_pevent_t e;
    /* (a button held since a press inside: the panel keeps the pointer until it is released) */
    if (!inside && window->pointer_inside && [NSEvent pressedMouseButtons] != 0) return;
    if (inside == window->pointer_inside) return;
    window->pointer_inside = inside;
    [window->handle setIgnoresMouseEvents:inside ? NO : YES];
    memset(&e, 0, sizeof(e));
    if (inside) {
        /* the pointer is over the island now: no need to wait for it to move */
        e.type = RVN_PEVENT_MOUSE_MOVE; e.x = (int)lrint(x); e.y = (int)lrint(y);
        e.modifiers = rvn_cocoa_modifiers([NSEvent modifierFlags]);
    } else {
        e.type = RVN_PEVENT_MOUSE_LEAVE;
    }
    rvn_cocoa_emit(window, &e);
    /* (the loop of the application waits for an event: this one came from a timer) */
    rvn_cocoa_wake();
}

static void rvn_cocoa_overlay_set_input(rvn_pwindow_t* overlay, int x, int y, int width, int height) {
    overlay->overlay_input = NSMakeRect(x, y, width, height);
    if (!overlay->overlay_poll) {
        rvn_pwindow_t* window = overlay;
        overlay->overlay_poll = [NSTimer timerWithTimeInterval:1.0 / 60.0 repeats:YES block:^(NSTimer* timer) { (void)timer; rvn_cocoa_overlay_poll(window); }];
        [[NSRunLoop currentRunLoop] addTimer:overlay->overlay_poll forMode:NSRunLoopCommonModes];
    }
    rvn_cocoa_overlay_poll(overlay);
}

static void rvn_cocoa_overlay_set_key(rvn_pwindow_t* overlay, rvn_pwindow_t* owner, int key) {
    if (key) {
        overlay->overlay_key = 1;
        [overlay->handle makeKeyWindow];
    } else {
        overlay->overlay_key = 0;
        if (owner && owner->handle && [overlay->handle isKeyWindow] && [owner->handle isVisible]) [owner->handle makeKeyWindow];
    }
}
static void rvn_cocoa_clipboard_set(const char* text) {
    NSPasteboard* board = [NSPasteboard generalPasteboard]; [board clearContents];
    NSString* value = [NSString stringWithUTF8String:text ? text : ""];
    if (value) [board setString:value forType:NSPasteboardTypeString];
}
static char* rvn_cocoa_clipboard_get(void) {
    NSString* value = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
    if (!value) return NULL;
    const char* utf8 = [value UTF8String]; return utf8 ? rvn_strdup(utf8) : NULL;
}

/* ---------------------------------------------------------------------------
 *  Accessibility: VoiceOver (NSAccessibility)
 *
 *  The view of a window stands for the window node; every other node of
 *  core/accessibility.h is a RavenAccessibilityElement, made on first use and
 *  kept in a table by node. The geometry is in points: the logical units of
 *  Raven on macOS (the scale of a window is its backing scale).
 * ------------------------------------------------------------------------- */

@interface RavenAccessibilityElement : NSAccessibilityElement {
@public
    rvn_a11y_id_t nodeId;
    rvn_pwindow_t* ravenWindow;
}
@end

static NSMutableDictionary* g_cocoa_elements;

static id rvn_cocoa_a11y_object(rvn_pwindow_t* window, rvn_a11y_id_t node);

static NSString* rvn_cocoa_string(const char* text) {
    NSString* string = text ? [NSString stringWithUTF8String:text] : nil;
    return string ? string : @"";
}

/* Logical units of the window (top-left origin) -> screen (bottom-left origin). */
static NSRect rvn_cocoa_a11y_screen_rect(rvn_pwindow_t* window, const rvn_a11y_node_t* node) {
    NSRect local = NSMakeRect(node->x, node->y, node->width, node->height);
    NSRect in_window;
    if (!window || !window->view || !window->handle) return NSZeroRect;
    in_window = [window->view convertRect:local toView:nil];
    return [window->handle convertRectToScreen:in_window];
}

static NSAccessibilityRole rvn_cocoa_a11y_role(const rvn_a11y_node_t* node) {
    switch (node->role) {
        case R_ROLE_WINDOW: return NSAccessibilityWindowRole;
        case R_ROLE_TEXT: case R_ROLE_HEADING: case R_ROLE_ALERT: return NSAccessibilityStaticTextRole;
        case R_ROLE_LINK: return NSAccessibilityLinkRole;
        case R_ROLE_BUTTON: case R_ROLE_TOGGLE_BUTTON: return NSAccessibilityButtonRole;
        case R_ROLE_CHECKBOX: return NSAccessibilityCheckBoxRole;
        case R_ROLE_RADIO_BUTTON: return NSAccessibilityRadioButtonRole;
        case R_ROLE_SLIDER: return NSAccessibilitySliderRole;
        case R_ROLE_PROGRESS_BAR: return NSAccessibilityProgressIndicatorRole;
        case R_ROLE_SPIN_BUTTON: return NSAccessibilityIncrementorRole;
        case R_ROLE_TEXT_FIELD: return NSAccessibilityTextFieldRole;
        case R_ROLE_DOCUMENT: return NSAccessibilityTextAreaRole;
        case R_ROLE_COMBO_BOX: return NSAccessibilityPopUpButtonRole;
        case R_ROLE_LIST: return NSAccessibilityListRole;
        case R_ROLE_LIST_ITEM: return NSAccessibilityStaticTextRole;
        case R_ROLE_TREE: return NSAccessibilityOutlineRole;
        case R_ROLE_TREE_ITEM: return NSAccessibilityRowRole;
        case R_ROLE_TABLE: return NSAccessibilityTableRole;
        case R_ROLE_MENU_BAR: return NSAccessibilityMenuBarRole;
        case R_ROLE_MENU: return NSAccessibilityMenuRole;
        case R_ROLE_MENU_ITEM: return NSAccessibilityMenuItemRole;
        case R_ROLE_TAB_LIST: return NSAccessibilityTabGroupRole;
        case R_ROLE_TAB: return NSAccessibilityRadioButtonRole;
        case R_ROLE_TOOLBAR: return NSAccessibilityToolbarRole;
        case R_ROLE_IMAGE: return NSAccessibilityImageRole;
        case R_ROLE_SEPARATOR: return NSAccessibilitySplitterRole;
        case R_ROLE_SCROLL_BAR: return NSAccessibilityScrollBarRole;
        case R_ROLE_DIALOG: return NSAccessibilitySheetRole;
        default: return NSAccessibilityGroupRole;
    }
}

static NSArray* rvn_cocoa_a11y_children(rvn_pwindow_t* window, rvn_a11y_id_t node) {
    int i, count = rvn_a11y_child_count(node);
    NSMutableArray* children = [NSMutableArray arrayWithCapacity:(NSUInteger)(count > 0 ? count : 0)];
    for (i = 0; i < count; i++) {
        id child = rvn_cocoa_a11y_object(window, rvn_a11y_child(node, i));
        if (child) [children addObject:child];
    }
    return children;
}

@implementation RavenAccessibilityElement
- (BOOL)isAccessibilityElement { return rvn_a11y_exists(nodeId) ? YES : NO; }
- (NSAccessibilityRole)accessibilityRole {
    rvn_a11y_node_t node;
    if (!rvn_a11y_describe(nodeId, &node)) return NSAccessibilityUnknownRole;
    return rvn_cocoa_a11y_role(&node);
}
- (NSString*)accessibilityRoleDescription {
    rvn_a11y_node_t node;
    if (!rvn_a11y_describe(nodeId, &node)) return @"";
    if (node.role == R_ROLE_TAB) return @"tab";
    if (node.role == R_ROLE_HEADING) return @"heading";
    if (node.role == R_ROLE_CANVAS) return @"canvas";
    return NSAccessibilityRoleDescription([self accessibilityRole], nil);
}
- (NSString*)accessibilityLabel {
    rvn_a11y_node_t node;
    return rvn_a11y_describe(nodeId, &node) ? rvn_cocoa_string(node.name) : @"";
}
- (NSString*)accessibilityHelp {
    rvn_a11y_node_t node;
    return rvn_a11y_describe(nodeId, &node) ? rvn_cocoa_string(node.description) : @"";
}
- (id)accessibilityValue {
    rvn_a11y_node_t node;
    if (!rvn_a11y_describe(nodeId, &node)) return nil;
    if (node.states & RVN_A11Y_CHECKABLE) return [NSNumber numberWithInt:(node.states & RVN_A11Y_CHECKED) ? 1 : 0];
    if (node.states & RVN_A11Y_HAS_RANGE) return [NSNumber numberWithDouble:node.number];
    if (node.role == R_ROLE_TAB) return [NSNumber numberWithInt:(node.states & RVN_A11Y_SELECTED) ? 1 : 0];
    return rvn_cocoa_string(node.value);
}
- (void)setAccessibilityValue:(id)value {
    if ([value isKindOfClass:[NSNumber class]]) rvn_a11y_set_number(nodeId, [value doubleValue]);
    else if ([value isKindOfClass:[NSString class]]) rvn_a11y_set_value(nodeId, [value UTF8String]);
}
- (id)accessibilityMinValue {
    rvn_a11y_node_t node;
    return rvn_a11y_describe(nodeId, &node) && (node.states & RVN_A11Y_HAS_RANGE) ? [NSNumber numberWithDouble:node.minimum] : nil;
}
- (id)accessibilityMaxValue {
    rvn_a11y_node_t node;
    return rvn_a11y_describe(nodeId, &node) && (node.states & RVN_A11Y_HAS_RANGE) ? [NSNumber numberWithDouble:node.maximum] : nil;
}
- (NSRect)accessibilityFrame {
    rvn_a11y_node_t node;
    return rvn_a11y_describe(nodeId, &node) ? rvn_cocoa_a11y_screen_rect(ravenWindow, &node) : NSZeroRect;
}
- (id)accessibilityParent {
    rvn_a11y_id_t parent = rvn_a11y_parent(nodeId);
    return rvn_cocoa_a11y_object(ravenWindow, parent);
}
- (NSArray*)accessibilityChildren { return rvn_cocoa_a11y_children(ravenWindow, nodeId); }
- (id)accessibilityWindow { return ravenWindow ? ravenWindow->handle : nil; }
- (id)accessibilityTopLevelUIElement { return ravenWindow ? ravenWindow->handle : nil; }
- (BOOL)isAccessibilityFocused {
    rvn_a11y_node_t node;
    return rvn_a11y_describe(nodeId, &node) && (node.states & RVN_A11Y_FOCUSED) ? YES : NO;
}
- (void)setAccessibilityFocused:(BOOL)focused { if (focused) rvn_a11y_act(nodeId, RVN_A11Y_FOCUS); }
- (BOOL)isAccessibilityEnabled {
    rvn_a11y_node_t node;
    return rvn_a11y_describe(nodeId, &node) && !(node.states & RVN_A11Y_DISABLED) ? YES : NO;
}
- (BOOL)isAccessibilitySelected {
    rvn_a11y_node_t node;
    return rvn_a11y_describe(nodeId, &node) && (node.states & RVN_A11Y_SELECTED) ? YES : NO;
}
- (BOOL)isAccessibilityExpanded {
    rvn_a11y_node_t node;
    return rvn_a11y_describe(nodeId, &node) && (node.states & RVN_A11Y_EXPANDED) ? YES : NO;
}
- (void)setAccessibilityExpanded:(BOOL)expanded { rvn_a11y_act(nodeId, expanded ? RVN_A11Y_EXPAND : RVN_A11Y_COLLAPSE); }
- (BOOL)accessibilityPerformPress {
    rvn_a11y_node_t node;
    if (!rvn_a11y_describe(nodeId, &node)) return NO;
    return rvn_a11y_act(nodeId, (node.states & RVN_A11Y_CHECKABLE) ? RVN_A11Y_TOGGLE : RVN_A11Y_PRESS) ? YES : NO;
}
- (BOOL)accessibilityPerformIncrement {
    rvn_a11y_node_t node;
    if (!rvn_a11y_describe(nodeId, &node) || !(node.states & RVN_A11Y_HAS_RANGE)) return NO;
    return rvn_a11y_set_number(nodeId, node.number + (node.step > 0.0 ? node.step : 1.0)) ? YES : NO;
}
- (BOOL)accessibilityPerformDecrement {
    rvn_a11y_node_t node;
    if (!rvn_a11y_describe(nodeId, &node) || !(node.states & RVN_A11Y_HAS_RANGE)) return NO;
    return rvn_a11y_set_number(nodeId, node.number - (node.step > 0.0 ? node.step : 1.0)) ? YES : NO;
}
- (BOOL)isAccessibilitySelectorAllowed:(SEL)selector {
    rvn_a11y_node_t node;
    if (!rvn_a11y_describe(nodeId, &node)) return NO;
    if (selector == @selector(accessibilityPerformPress)) return (node.states & (RVN_A11Y_INVOKABLE | RVN_A11Y_CHECKABLE)) ? YES : NO;
    if (selector == @selector(accessibilityPerformIncrement) || selector == @selector(accessibilityPerformDecrement))
        return (node.states & RVN_A11Y_HAS_RANGE) && !(node.states & RVN_A11Y_READONLY) ? YES : NO;
    if (selector == @selector(setAccessibilityValue:)) return (node.states & (RVN_A11Y_EDITABLE | RVN_A11Y_HAS_RANGE)) ? YES : NO;
    return [super isAccessibilitySelectorAllowed:selector];
}
@end

/* The object of a node: the view for the window, an element for the others. */
static id rvn_cocoa_a11y_object(rvn_pwindow_t* window, rvn_a11y_id_t node) {
    NSNumber* key;
    RavenAccessibilityElement* element;
    if (!node || !window) return nil;
    if (node == (rvn_a11y_id_t)rvn_a11y_window_of(window->user)) return window->view;
    if (!g_cocoa_elements) g_cocoa_elements = [[NSMutableDictionary alloc] init];
    key = [NSNumber numberWithUnsignedLongLong:(unsigned long long)node];
    element = [g_cocoa_elements objectForKey:key];
    if (element) return element;
    element = [[RavenAccessibilityElement alloc] init];
    element->nodeId = node;
    element->ravenWindow = window;
    [g_cocoa_elements setObject:element forKey:key];
    [element release];
    return element;
}

static void rvn_cocoa_a11y_forget(rvn_pwindow_t* window) {
    NSMutableArray* keys;
    if (!g_cocoa_elements) return;
    keys = [NSMutableArray array];
    for (NSNumber* key in g_cocoa_elements) {
        RavenAccessibilityElement* element = [g_cocoa_elements objectForKey:key];
        /* a window that goes takes its elements; else only the nodes that are gone */
        if (window ? element->ravenWindow == window : !rvn_a11y_exists(element->nodeId)) {
            element->ravenWindow = NULL;
            [keys addObject:key];
        }
    }
    [g_cocoa_elements removeObjectsForKeys:keys];
}

@implementation RavenCocoaView (RavenAccessibility)
- (BOOL)isAccessibilityElement { return YES; }
- (NSAccessibilityRole)accessibilityRole { return NSAccessibilityGroupRole; }
- (NSString*)accessibilityLabel {
    rvn_a11y_node_t node;
    Rid window = ravenWindow ? rvn_a11y_window_of(ravenWindow->user) : R_INVALID_ID;
    return window && rvn_a11y_describe((rvn_a11y_id_t)window, &node) ? rvn_cocoa_string(node.name) : @"";
}
- (NSArray*)accessibilityChildren {
    Rid window = ravenWindow ? rvn_a11y_window_of(ravenWindow->user) : R_INVALID_ID;
    if (!window) return [NSArray array];
    return rvn_cocoa_a11y_children(ravenWindow, (rvn_a11y_id_t)window);
}
- (id)accessibilityHitTest:(NSPoint)point {
    Rid window = ravenWindow ? rvn_a11y_window_of(ravenWindow->user) : R_INVALID_ID;
    NSRect on_screen = NSMakeRect(point.x, point.y, 0.0, 0.0);
    NSPoint in_view;
    id hit;
    if (!window || !ravenWindow->handle) return self;
    in_view = [self convertPoint:[ravenWindow->handle convertRectFromScreen:on_screen].origin fromView:nil];
    hit = rvn_cocoa_a11y_object(ravenWindow, rvn_a11y_hit(window, (float)in_view.x, (float)in_view.y));
    return hit ? hit : self;
}
- (id)accessibilityFocusedUIElement {
    Rid window = ravenWindow ? rvn_a11y_window_of(ravenWindow->user) : R_INVALID_ID;
    id focus = window ? rvn_cocoa_a11y_object(ravenWindow, rvn_a11y_focus(window)) : nil;
    return focus ? focus : self;
}
@end

static void rvn_cocoa_a11y_event(rvn_pwindow_t* window, int event, uint64_t node) {
    id object = rvn_cocoa_a11y_object(window, (rvn_a11y_id_t)node);
    if (!object) return;
    switch (event) {
        case RVN_A11Y_EVENT_FOCUS:
            NSAccessibilityPostNotification(object, NSAccessibilityFocusedUIElementChangedNotification);
            break;
        case RVN_A11Y_EVENT_VALUE:
        case RVN_A11Y_EVENT_STATE:
            NSAccessibilityPostNotification(object, NSAccessibilityValueChangedNotification);
            break;
        case RVN_A11Y_EVENT_NAME:
            NSAccessibilityPostNotification(object, NSAccessibilityTitleChangedNotification);
            break;
        case RVN_A11Y_EVENT_STRUCTURE:
            rvn_cocoa_a11y_forget(NULL);
            if (window->view) NSAccessibilityPostNotification(window->view, NSAccessibilityLayoutChangedNotification);
            break;
        default:
            break;
    }
}

static void rvn_cocoa_a11y_announce(rvn_pwindow_t* window, const char* text, int assertive) {
    NSDictionary* info = [NSDictionary dictionaryWithObjectsAndKeys:
        rvn_cocoa_string(text), NSAccessibilityAnnouncementKey,
        [NSNumber numberWithInteger:assertive ? NSAccessibilityPriorityHigh : NSAccessibilityPriorityMedium], NSAccessibilityPriorityKey, nil];
    id target = window && window->handle ? (id)window->handle : (id)NSApp;
    NSAccessibilityPostNotificationWithUserInfo(target, NSAccessibilityAnnouncementRequestedNotification, info);
}

static int rvn_cocoa_a11y_active(void) {
    NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
    if ([workspace respondsToSelector:@selector(isVoiceOverEnabled)]) return [workspace isVoiceOverEnabled] ? R_TRUE : R_FALSE;
    return R_TRUE;
}

static void rvn_cocoa_a11y_settings(RAccessibilitySettings* settings) {
    NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
    if ([workspace respondsToSelector:@selector(accessibilityDisplayShouldIncreaseContrast)])
        settings->high_contrast = [workspace accessibilityDisplayShouldIncreaseContrast] ? R_TRUE : R_FALSE;
    if ([workspace respondsToSelector:@selector(accessibilityDisplayShouldReduceMotion)])
        settings->reduce_motion = [workspace accessibilityDisplayShouldReduceMotion] ? R_TRUE : R_FALSE;
}

/* The accent color chosen in the System Settings (Appearance), for the look of macOS. */
static int rvn_cocoa_accent_color(float rgb[3]) {
    @autoreleasepool {
        NSColor* color = nil;
        if (@available(macOS 10.14, *)) color = [NSColor controlAccentColor];
        if (!color) return R_FALSE;
        color = [color colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
        if (!color) return R_FALSE;
        rgb[0] = (float)[color redComponent];
        rgb[1] = (float)[color greenComponent];
        rgb[2] = (float)[color blueComponent];
        return R_TRUE;
    }
}

const rvn_platform_t* rvn_platform_cocoa(void) {
    static const rvn_platform_t platform = {
        .name = "Cocoa", .system = R_WINDOW_SYSTEM_COCOA,
        .init = rvn_cocoa_init, .shutdown = rvn_cocoa_shutdown, .native_display = rvn_cocoa_native_display,
        .window_create = rvn_cocoa_window_create, .window_destroy = rvn_cocoa_window_destroy,
        .window_set_title = rvn_cocoa_window_set_title, .window_set_size = rvn_cocoa_window_set_size,
        .window_get_size = rvn_cocoa_window_get_size, .window_set_min_size = rvn_cocoa_window_set_min_size,
        .window_set_position = rvn_cocoa_window_set_position, .window_get_position = rvn_cocoa_window_get_position,
        .window_get_scale = rvn_cocoa_window_get_scale, .window_show = rvn_cocoa_window_show,
        .window_set_state = rvn_cocoa_window_set_state, .window_get_state = rvn_cocoa_window_get_state,
        .window_begin_move = rvn_cocoa_window_begin_move, .window_begin_resize = rvn_cocoa_window_begin_resize,
        .native_regions = R_FALSE, .window_set_backdrop = rvn_cocoa_window_set_backdrop,
        .window_set_opacity = rvn_cocoa_window_set_opacity, .window_set_cursor = rvn_cocoa_window_set_cursor,
        .window_get_native = rvn_cocoa_window_get_native, .child_create = rvn_cocoa_child_create,
        .child_set_rect = rvn_cocoa_child_set_rect, .pump = rvn_cocoa_pump, .wake = rvn_cocoa_wake,
        .clipboard_set = rvn_cocoa_clipboard_set, .clipboard_get = rvn_cocoa_clipboard_get,
        .a11y_event = rvn_cocoa_a11y_event, .a11y_announce = rvn_cocoa_a11y_announce,
        .a11y_active = rvn_cocoa_a11y_active, .a11y_settings = rvn_cocoa_a11y_settings,
        .accent_color = rvn_cocoa_accent_color,
        .screen_notch = rvn_cocoa_screen_notch, .overlay_place = rvn_cocoa_overlay_place,
        .overlay_set_input = rvn_cocoa_overlay_set_input, .overlay_set_key = rvn_cocoa_overlay_set_key
    };
    return &platform;
}
