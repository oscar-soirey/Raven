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

static void test_native_menu_headers(Rid window) {
	Rid bar = Rwidget_create(R_WIDGET_MENU_BAR);
	Rid menu;
	NSMenu* native;
	NSMenuItem* header;
	Rmenu_bar_add_divider(bar);
	menu = Rmenu_add_menu(bar, "Devices");
	Rmenu_set_title_color(menu, 1, 0, 0, 1);
	Rmenu_add_header(menu, "pdk1");
	Rmenu_add_item(menu, "Configure");
	Rmenu_bar_add_divider(bar);
	Rwindow_set_content(window, bar);
	Rwindow_show(window);
	Rwindow_focus(window);
	Rdraw_frame();
	native = [[[NSApp mainMenu] itemWithTitle:@"Devices"] submenu];
	CHECK(native != nil);
	CHECK([[NSApp mainMenu] numberOfItems] == 2); /* app menu and Devices; dividers omitted */
	CHECK([native numberOfItems] == 2);
	if ([native numberOfItems] != 2) return;
	header = [native itemAtIndex:0];
	CHECK([[header title] isEqualToString:@"pdk1"]);
	CHECK(![header isEnabled]);
	CHECK([header action] == nil);
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 140000
	if (@available(macOS 14.0, *)) CHECK([header isSectionHeader]);
#endif
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
	test_native_menu_headers(window);
	Rwindow_destroy(window);
	Rshutdown();
	return test_report("test_cocoa_input");
}
