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

static void test_owned_window(Rid owner) {
	RNativeWindow owner_native, tool_native;
	Rid tool = Rwindow_create("Tool", 100, 100, 0);
	NSWindow* native;
	Rwindow_get_native_handle(owner, &owner_native);
	Rwindow_get_native_handle(tool, &tool_native);
	native = (NSWindow*)tool_native.display;
	Rwindow_set_owner(tool, owner);
	CHECK([native parentWindow] == (NSWindow*)owner_native.display);
	CHECK([native level] == NSNormalWindowLevel);
	CHECK([native isExcludedFromWindowsMenu]);
	Rwindow_hide(owner);
	CHECK(![native isVisible]);
	Rwindow_show(owner);
	CHECK([native isVisible]);
	Rwindow_hide(tool);
	Rwindow_hide(owner);
	Rwindow_show(owner);
	CHECK(![native isVisible]);
	Rwindow_show(tool);
	Rwindow_set_owner(tool, R_INVALID_ID);
	CHECK([native parentWindow] == nil);
	CHECK(![native isExcludedFromWindowsMenu]);
	Rwindow_set_owner(tool, owner);
	{
		Rid temporary_owner = Rwindow_create("Temporary owner", 100, 100, 0);
		Rwindow_set_owner(tool, temporary_owner);
		Rwindow_destroy(temporary_owner);
		CHECK(Rwindow_is_valid(tool));
		CHECK([native parentWindow] == nil);
		CHECK(Rwindow_get_owner(tool) == R_INVALID_ID);
		Rwindow_set_owner(tool, owner);
	}
	Rwindow_destroy(tool);
	CHECK([[(NSWindow*)owner_native.display childWindows] count] == 0);
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
	test_owned_window(window);
	Rwindow_destroy(window);
	Rshutdown();
	return test_report("test_cocoa_input");
}
