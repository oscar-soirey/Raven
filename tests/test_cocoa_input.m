/* Native AppKit input: injected Raven events cannot test Cocoa's text conversion. */
#import <AppKit/AppKit.h>
#include "test.h"

static void cocoa_key(NSView* view, unsigned short key, NSString* text, NSEventModifierFlags modifiers, BOOL repeat) {
	NSEvent* event = [NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint
		modifierFlags:modifiers timestamp:0 windowNumber:[[view window] windowNumber]
		context:nil characters:text charactersIgnoringModifiers:text isARepeat:repeat keyCode:key];
	[view keyDown:event];
}

static void test_function_keys(Rid input, NSView* view) {
	/* AppKit reserves this whole range for function keys, including F13-F35. */
	unichar ch;
	Rtext_input_set_text(input, "abc");
	cocoa_key(view, 123, @"\uF702", 0, NO);
	cocoa_key(view, 7, @"x", 0, NO);
	CHECK_STR(Rtext_input_get_text(input), "abxc");
	Rtext_input_set_text(input, "abc");
	for (ch = 0xF700; ch <= 0xF8FF; ch++) {
		NSString* text = [NSString stringWithCharacters:&ch length:1];
		cocoa_key(view, 123, text, 0, NO);
		CHECK_STR(Rtext_input_get_text(input), "abc");
	}
	/* Preserve private-use text outside AppKit's range, and supplementary Unicode. */
	Rtext_input_set_text(input, "");
	cocoa_key(view, 0, @"\uF6FF\uF900\U0001F600", 0, NO);
	CHECK_STR(Rtext_input_get_text(input), "\xEF\x9B\xBF\xEF\xA4\x80\xF0\x9F\x98\x80");
	Rtext_input_set_text(input, "");
	cocoa_key(view, 0, @"a", NSEventModifierFlagCommand, NO);
	CHECK_STR(Rtext_input_get_text(input), "");
}

static int g_key_presses, g_key_repeats;

static int observe_key(Rid window, const REvent* event, void* user) {
	(void)window;
	(void)user;
	if (event->type == R_EVENT_KEY_DOWN) {
		g_key_presses++;
		if (event->repeat) g_key_repeats++;
	}
	return R_FALSE;
}

static void test_key_repeat(Rid window, Rid input, NSView* view) {
	Rtext_input_set_text(input, "");
	Rwindow_set_event_hook(window, observe_key, NULL);
	g_key_presses = g_key_repeats = 0;
	cocoa_key(view, 40, @"k", 0, NO);
	cocoa_key(view, 40, @"k", 0, YES);
	cocoa_key(view, 40, @"k", 0, YES);
	CHECK_STR(Rtext_input_get_text(input), "kkk");
	CHECK(g_key_presses == 3);
	CHECK(g_key_repeats == 2);
	/* Repeated navigation still changes the caret, without inserting text. */
	cocoa_key(view, 123, @"\uF702", 0, YES);
	cocoa_key(view, 123, @"\uF702", 0, YES);
	cocoa_key(view, 7, @"x", 0, NO);
	CHECK_STR(Rtext_input_get_text(input), "kxkk");
	/* A modified key repeat must not bypass shortcut text suppression. */
	Rtext_input_set_text(input, "");
	cocoa_key(view, 40, @"k", NSEventModifierFlagCommand, YES);
	CHECK_STR(Rtext_input_get_text(input), "");
	Rwindow_set_event_hook(window, NULL, NULL);
}

int main(void) {
	RInitDesc desc;
	RNativeWindow native;
	Rid window, input;
	NSView* view;
	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_NULL;
	desc.worker_threads = 1;
	Rerror_set_callback(test_error_callback);
	if (!Rinit_ex(&desc)) return 1;
	Rset_animations_enabled(R_FALSE);
	window = Rwindow_create("Cocoa input tests", 320, 120, R_WINDOW_DEFAULT);
	input = Rwidget_create(R_WIDGET_TEXT_INPUT);
	Rwindow_set_content(window, input);
	Rdraw_frame();
	Rwidget_focus(input);
	CHECK(Rwindow_get_native_handle(window, &native));
	view = [(NSWindow*)native.display contentView];
	test_function_keys(input, view);
	test_key_repeat(window, input, view);
	Rwindow_destroy(window);
	Rshutdown();
	return test_report("test_cocoa_input");
}
