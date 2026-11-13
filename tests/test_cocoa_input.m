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

static void accessible_press(Rid widget, void* user) {
	(*(int*)user)++;
	Rwidget_set_accessible_checked(widget, R_TRUE);
}

static void test_accessible_action(Rid window, NSView* view) {
	Rid custom = Rwidget_create(R_WIDGET_CUSTOM);
	int presses = 0;
	NSArray* children;
	id element;
	Rwindow_set_content(window, custom);
	Rwidget_set_focusable(custom, R_TRUE);
	Rwidget_set_accessible_role(custom, R_ROLE_TOGGLE_BUTTON);
	Rwidget_set_accessible_name(custom, "Box tool");
	Rwidget_set_accessible_checked(custom, 0);
	Rwidget_set_accessible_action(custom, accessible_press, &presses);
	Rdraw_frame();
	children = [view accessibilityChildren];
	CHECK([children count] == 1);
	if ([children count] != 1) return;
	element = [children objectAtIndex:0];
	CHECK([element isAccessibilitySelectorAllowed:@selector(accessibilityPerformPress)]);
	CHECK([[element accessibilityValue] intValue] == 0);
	CHECK([element accessibilityPerformPress]);
	CHECK(presses == 1);
	CHECK([[element accessibilityValue] intValue] == 1);
	Rwidget_set_enabled(custom, R_FALSE);
	CHECK(![element accessibilityPerformPress]);
	CHECK(presses == 1);
	Rwidget_set_enabled(custom, R_TRUE);
	Rwidget_set_accessible_action(custom, NULL, NULL);
	Rwidget_set_accessible_checked(custom, -1);
	CHECK(![element isAccessibilitySelectorAllowed:@selector(accessibilityPerformPress)]);
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
	test_accessible_action(window, view);
	Rwindow_destroy(window);
	Rshutdown();
	return test_report("test_cocoa_input");
}
