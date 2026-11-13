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

static void test_table_selection(Rid window, NSView* view) {
	Rid table = Rwidget_create(R_WIDGET_TABLE);
	NSArray* children;
	id element, row;
	Rtable_add_column(table, "Name", 200);
	Rtable_add_row(table, "First");
	Rtable_add_row(table, "Second");
	Rtable_add_row(table, "Third");
	Rtable_set_multi_select(table, R_TRUE);
	Rtable_set_row_selected(table, 0, R_TRUE);
	Rtable_set_row_selected(table, 2, R_TRUE);
	Rwindow_set_content(window, table);
	Rdraw_frame();
	children = [view accessibilityChildren];
	CHECK([children count] == 1);
	if ([children count] != 1) return;
	element = [children objectAtIndex:0];
	CHECK([[element accessibilityRole] isEqualToString:NSAccessibilityTableRole]);
	CHECK([[element accessibilityRows] count] == 3);
	CHECK([[element accessibilitySelectedRows] count] == 2);
	row = [[element accessibilityRows] objectAtIndex:2];
	CHECK([[row accessibilityRole] isEqualToString:NSAccessibilityRowRole]);
	CHECK([row isAccessibilitySelected]);
	CHECK([element isAccessibilitySelectorAllowed:@selector(setAccessibilitySelectedRows:)]);
	[element setAccessibilitySelectedRows:@[row]];
	CHECK(Rtable_get_selection_count(table) == 1 && Rtable_is_row_selected(table, 2));
	[row setAccessibilitySelected:NO];
	CHECK(Rtable_get_selection_count(table) == 0);
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
	test_table_selection(window, view);
	Rwindow_destroy(window);
	Rshutdown();
	return test_report("test_cocoa_input");
}
