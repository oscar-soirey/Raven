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

/*
 * The input method, as AppKit drives an NSTextInputClient: marked text is
 * shown by the field (Rwindow_get_text_composition) and is not in its text
 * until committed; the keys it takes while composing do not reach the field.
 */
static void test_input_method(Rid window, NSView* view) {
	id<NSTextInputClient> client = (id<NSTextInputClient>)view;
	NSRange none = NSMakeRange(NSNotFound, 0), range;
	NSAttributedString* sub;
	Rid box = Rwidget_create(R_WIDGET_VBOX);
	Rid input = Rwidget_add_child(box, Rwidget_create(R_WIDGET_TEXT_INPUT));
	Rid other = Rwidget_add_child(box, Rbutton_create("Other"));
	int start = -1, end = -1, x, y, w, h;
	CHECK([view conformsToProtocol:@protocol(NSTextInputClient)]);
	Rwindow_set_content(window, box);
	Rdraw_frame();
	Rwidget_focus(input);
	Rtext_input_set_text(input, "ab");
	CHECK(Rwindow_set_text_input_selection(window, 1, 1));
	CHECK(![client hasMarkedText]);
	CHECK([client markedRange].location == NSNotFound);
	range = [client selectedRange];
	CHECK(range.location == 1 && range.length == 0);

	/* preedit */
	[client setMarkedText:@"に" selectedRange:NSMakeRange(1, 0) replacementRange:none];
	CHECK([client hasMarkedText]);
	CHECK_STR(Rwindow_get_text_composition(window, &start, &end), "\xE3\x81\xAB");
	CHECK(start == 3 && end == 3);
	[client setMarkedText:[[[NSAttributedString alloc] initWithString:@"にほんご"] autorelease]
		selectedRange:NSMakeRange(1, 2) replacementRange:none];
	CHECK_STR(Rwindow_get_text_composition(window, &start, &end), "\xE3\x81\xAB\xE3\x81\xBB\xE3\x82\x93\xE3\x81\x94");
	CHECK(start == 3 && end == 9);
	CHECK_STR(Rtext_input_get_text(input), "ab");
	range = [client markedRange];
	CHECK(range.location == 1 && range.length == 4);
	range = [client selectedRange];
	CHECK(range.location == 2 && range.length == 2);
	sub = [client attributedSubstringForProposedRange:[client markedRange] actualRange:NULL];
	CHECK([[sub string] isEqualToString:@"にほんご"]);
	sub = [client attributedSubstringForProposedRange:NSMakeRange(0, 100) actualRange:&range];
	CHECK([[sub string] isEqualToString:@"aにほんごb"] && range.length == 6);
	CHECK([[client validAttributesForMarkedText] count] > 0);

	/* the candidate window goes to the caret, in screen coordinates */
	Rdraw_frame();
	if (Rwindow_get_text_input_rect(window, &x, &y, &w, &h)) {
		NSRect rect = [client firstRectForCharacterRange:[client markedRange] actualRange:NULL];
		NSRect frame = [[view window] convertRectToScreen:[view convertRect:[view bounds] toView:nil]];
		CHECK(NSContainsRect(NSInsetRect(frame, -1, -1), rect) && rect.size.height > 0);
		printf("candidate rect %.0f,%.0f %.0fx%.0f in window %.0f,%.0f %.0fx%.0f\n", rect.origin.x, rect.origin.y,
			rect.size.width, rect.size.height, frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
	} else printf("no caret painted (window not active): candidate rect not checked\n");

	/* the keys the input method takes while composing are not keys for the field */
	cocoa_key(view, 51, @"\x7F", 0, NO);
	cocoa_key(view, 123, @"", 0, NO);
	CHECK_STR(Rtext_input_get_text(input), "ab");
	CHECK(Rwindow_get_text_input_selection(window, &start, &end) && start == 1 && end == 1);

	/* commit */
	[client insertText:@"日本語" replacementRange:none];
	CHECK(![client hasMarkedText]);
	CHECK(Rwindow_get_text_composition(window, NULL, NULL) == NULL);
	CHECK_STR(Rtext_input_get_text(input), "a\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E" "b");

	/* cancel */
	[client setMarkedText:@"か" selectedRange:NSMakeRange(1, 0) replacementRange:none];
	[client setMarkedText:@"" selectedRange:NSMakeRange(0, 0) replacementRange:none];
	CHECK(![client hasMarkedText]);
	CHECK(Rwindow_get_text_composition(window, NULL, NULL) == NULL);
	CHECK_STR(Rtext_input_get_text(input), "a\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E" "b");

	/* a dead key: the accent is marked, the next key commits the letter */
	Rtext_input_set_text(input, "");
	[client setMarkedText:@"´" selectedRange:NSMakeRange(1, 0) replacementRange:none];
	[client insertText:@"é" replacementRange:none];
	CHECK_STR(Rtext_input_get_text(input), "\xC3\xA9");
	/* unmarkText accepts the marked text as it is */
	[client setMarkedText:@"¨" selectedRange:NSMakeRange(1, 0) replacementRange:none];
	[client unmarkText];
	CHECK_STR(Rtext_input_get_text(input), "\xC3\xA9\xC2\xA8");
	CHECK(Rwindow_get_text_composition(window, NULL, NULL) == NULL);

	/* a click commits what was being composed */
	Rtext_input_set_text(input, "");
	[client setMarkedText:@"ご" selectedRange:NSMakeRange(1, 0) replacementRange:none];
	[view mouseDown:[NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:NSMakePoint(1, 1) modifierFlags:0
		timestamp:0 windowNumber:[[view window] windowNumber] context:nil eventNumber:0 clickCount:1 pressure:1]];
	[view mouseUp:[NSEvent mouseEventWithType:NSEventTypeLeftMouseUp location:NSMakePoint(1, 1) modifierFlags:0
		timestamp:0 windowNumber:[[view window] windowNumber] context:nil eventNumber:0 clickCount:1 pressure:0]];
	CHECK(![client hasMarkedText]);
	CHECK_STR(Rtext_input_get_text(input), "\xE3\x81\x94");

	/* the focus moves away: the composition ends (nothing typed), and a button composes nothing */
	Rwidget_focus(input);
	[client setMarkedText:@"ご" selectedRange:NSMakeRange(1, 0) replacementRange:none];
	Rwidget_focus(other);
	CHECK(![client hasMarkedText]);
	[client setMarkedText:@"ご" selectedRange:NSMakeRange(1, 0) replacementRange:none];
	CHECK(![client hasMarkedText]);
	CHECK_STR(Rtext_input_get_text(input), "\xE3\x81\x94");
	Rwidget_focus(input);

	/* without composition, typing still gives the key, then its text */
	Rtext_input_set_text(input, "");
	cocoa_key(view, 0, @"a", 0, NO);
	cocoa_key(view, 51, @"\x7F", 0, NO);
	cocoa_key(view, 11, @"b", 0, NO);
	CHECK_STR(Rtext_input_get_text(input), "b");
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

static void test_native_menu_headers(Rid window) {
	Rid bar = Rwidget_create(R_WIDGET_MENU_BAR);
	Rid menu;
	NSMenu* native;
	NSMenuItem* header;
	NSWindow* handle;
	RNativeWindow handles;
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
	/* The bar of the system shows the menus of the key window, and a window
	 * becomes key only once the application is active. macOS (14 and later)
	 * does not make an application active while another one is in front, as
	 * when the tests run beside other programs or on a build machine: it
	 * becomes key when the user comes to it. That moment is played here. */
	CHECK(Rwindow_get_native_handle(window, &handles));
	handle = (NSWindow*)handles.display;
	if (![handle isKeyWindow]) {
		[[NSNotificationCenter defaultCenter] postNotificationName:NSWindowDidBecomeKeyNotification object:handle];
		Rdraw_frame();
	}
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
	test_input_method(window, view);
	test_table_selection(window, view);
	test_accessible_action(window, view);
	test_native_menu_headers(window);
	Rwindow_destroy(window);
	Rshutdown();
	return test_report("test_cocoa_input");
}
