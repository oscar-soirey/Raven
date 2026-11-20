/*
 * The clipboard and the drags out of a window on macOS, through the real
 * NSPasteboard: what Raven writes is read back with AppKit (as another
 * application reads it), and what AppKit writes is read with Raven. The
 * general pasteboard is saved first and given back at the end.
 *
 * A drag cannot be run without moving the mouse: the view gets AppKit mouse
 * events made here (as test_cocoa_input does for keys), and the start of the
 * drag session is caught (beginDraggingSessionWithItems:event:source: of the
 * view replaced) to check what would be dragged; its end is then told to the
 * view as AppKit tells it.
 */
#import <AppKit/AppKit.h>
#import <objc/runtime.h>
#include "test.h"

/* ---------------------------------------------------------------------------
 *  The pasteboard of the user, kept aside
 * ------------------------------------------------------------------------- */

static NSArray* save_pasteboard(void) {
	NSMutableArray* saved = [NSMutableArray array];
	for (NSPasteboardItem* item in [[NSPasteboard generalPasteboard] pasteboardItems]) {
		NSPasteboardItem* copy = [[[NSPasteboardItem alloc] init] autorelease];
		for (NSString* type in [item types]) {
			NSData* data = [item dataForType:type];
			if (data) [copy setData:data forType:type];
		}
		[saved addObject:copy];
	}
	return [saved retain];
}

static void restore_pasteboard(NSArray* saved) {
	NSPasteboard* board = [NSPasteboard generalPasteboard];
	[board clearContents];
	if ([saved count]) [board writeObjects:saved];
	[saved release];
}

/* ---------------------------------------------------------------------------
 *  Clipboard
 * ------------------------------------------------------------------------- */

static uint8_t* make_picture(int width, int height) {
	uint8_t* pixels = (uint8_t*)malloc((size_t)width * (size_t)height * 4);
	int x, y;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			uint8_t* p = pixels + ((size_t)y * (size_t)width + (size_t)x) * 4;
			p[0] = (uint8_t)(x * 255 / (width - 1));
			p[1] = (uint8_t)(y * 255 / (height - 1));
			p[2] = (uint8_t)((x ^ y) * 9);
			p[3] = 255;
		}
	}
	return pixels;
}

static void test_text(void) {
	NSPasteboard* board = [NSPasteboard generalPasteboard];
	char* data;
	size_t size = 0;
	Rclipboard_set_text("caf\xC3\xA9 au lait");
	CHECK([[board stringForType:NSPasteboardTypeString] isEqualToString:@"café au lait"]);
	CHECK_STR(Rclipboard_get_text(), "caf\xC3\xA9 au lait");

	Rclipboard_set_html("bold", "<b>bold</b>");
	CHECK([[board stringForType:NSPasteboardTypeString] isEqualToString:@"bold"]);
	CHECK([board dataForType:NSPasteboardTypeHTML] != nil);
	data = Rclipboard_get_data("text/html", &size);
	CHECK(data && size == 11);
	if (data) CHECK_STR(data, "<b>bold</b>");
	Rmemory_free(data);

	/* written by another application */
	[board clearContents];
	[board setString:@"from AppKit" forType:NSPasteboardTypeString];
	CHECK(Rclipboard_has_data("text/plain"));
	CHECK(!Rclipboard_has_data("text/html"));
	data = Rclipboard_get_data("text/plain", &size);
	CHECK(data && size == 11);
	if (data) CHECK_STR(data, "from AppKit");
	Rmemory_free(data);
}

static void test_custom_and_files(void) {
	static const uint8_t node[5] = { 9, 0, 8, 0, 7 };
	NSPasteboard* board = [NSPasteboard generalPasteboard];
	const char* list = "file:///tmp/raven%20one.png\r\nfile:///tmp/two.txt\r\n";
	RDataItem items[3];
	NSArray* urls;
	char* data;
	size_t size = 0;

	items[0].mime = "application/x-raven-test";
	items[0].data = node;
	items[0].size = sizeof(node);
	items[1].mime = "text/plain";
	items[1].data = "node";
	items[1].size = 4;
	items[2].mime = "text/uri-list";
	items[2].data = list;
	items[2].size = strlen(list);
	CHECK(Rclipboard_set_data(items, 3));
	/* the format of the application under a type made from its MIME type */
	data = Rclipboard_get_data("application/x-raven-test", &size);
	CHECK(data && size == sizeof(node) && memcmp(data, node, sizeof(node)) == 0);
	Rmemory_free(data);
	CHECK([[board types] containsObject:@"org.raven.mime.application-2Fx-2Draven-2Dtest"]);
	CHECK([[board stringForType:NSPasteboardTypeString] isEqualToString:@"node"]);
	/* the files, as the Finder reads them */
	urls = [board readObjectsForClasses:@[[NSURL class]] options:@{NSPasteboardURLReadingFileURLsOnlyKey:@(YES)}];
	CHECK([urls count] == 2);
	if ([urls count] == 2) {
		CHECK([[[urls objectAtIndex:0] path] isEqualToString:@"/tmp/raven one.png"]);
		CHECK([[[urls objectAtIndex:1] path] isEqualToString:@"/tmp/two.txt"]);
	}
	CHECK(Rclipboard_has_data("text/uri-list"));
	data = Rclipboard_get_data("text/uri-list", &size);
	CHECK(data != NULL);
	if (data) CHECK_STR(data, "file:///tmp/raven%20one.png\r\nfile:///tmp/two.txt\r\n");
	Rmemory_free(data);
}

static void test_image(void) {
	const int width = 48, height = 30;
	NSPasteboard* board = [NSPasteboard generalPasteboard];
	uint8_t* pixels = make_picture(width, height);
	NSBitmapImageRep* rep;
	NSData* png;
	int w = 0, h = 0;
	uint8_t* back;

	CHECK(Rclipboard_set_image(pixels, width, height, 0));
	CHECK([[board types] containsObject:NSPasteboardTypePNG]);
	CHECK([[board types] containsObject:NSPasteboardTypeTIFF]);
	/* what an image editor pastes */
	png = [board dataForType:NSPasteboardTypePNG];
	rep = png ? [NSBitmapImageRep imageRepWithData:png] : nil;
	CHECK(rep && [rep pixelsWide] == width && [rep pixelsHigh] == height);
	if (rep && [rep pixelsWide] == width) {
		NSColor* color = [rep colorAtX:width - 1 y:0];
		CHECK(fabs([color redComponent] - 1.0) < 0.02 && fabs([color greenComponent]) < 0.02);
	}
	CHECK(Rclipboard_has_image());
	back = Rclipboard_get_image(&w, &h);
	CHECK(back && w == width && h == height && memcmp(back, pixels, (size_t)width * height * 4) == 0);
	Rpixels_free(back);

	/* a picture copied by another application as TIFF only: converted */
	rep = [[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL pixelsWide:width pixelsHigh:height bitsPerSample:8
		samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bitmapFormat:NSBitmapFormatAlphaNonpremultiplied
		bytesPerRow:width * 4 bitsPerPixel:32] autorelease];
	memcpy([rep bitmapData], pixels, (size_t)width * height * 4);
	[board clearContents];
	[board setData:[rep TIFFRepresentation] forType:NSPasteboardTypeTIFF];
	CHECK(Rclipboard_has_image());
	CHECK(Rclipboard_has_data("image/png"));
	back = Rclipboard_get_image(&w, &h);
	CHECK(back && w == width && h == height);
	if (back && w == width && h == height) {
		/* (through the color management of AppKit: near, not equal) */
		int i, worst = 0;
		for (i = 0; i < width * height * 4; i++) {
			int d = abs((int)back[i] - (int)pixels[i]);
			if (d > worst) worst = d;
		}
		CHECK(worst <= 8);
	}
	Rpixels_free(back);

	[board clearContents];
	[board setString:@"words" forType:NSPasteboardTypeString];
	CHECK(!Rclipboard_has_image());
	CHECK(Rclipboard_get_image(&w, &h) == NULL);
	free(pixels);
}

/* ---------------------------------------------------------------------------
 *  Dragging out of the window
 * ------------------------------------------------------------------------- */

static NSArray* g_dragged;
static NSEvent* g_drag_event;
static id g_drag_source;
static int g_sessions, g_finished, g_operation;
static uint8_t* g_png;
static size_t g_png_size;

static NSDraggingSession* fake_begin_dragging(id self, SEL command, NSArray* items, NSEvent* event, id source) {
	(void)self;
	(void)command;
	g_sessions++;
	[g_dragged release];
	g_dragged = [items retain];
	g_drag_event = event;
	g_drag_source = source;
	/* (any object: the view only checks that a session started) */
	return (NSDraggingSession*)[NSNull null];
}

static void on_finished(Rid source, RDragOperation operation, void* user) {
	(void)source;
	(void)user;
	g_finished++;
	g_operation = (int)operation;
}

static void tile_item(Rid tiles, int index, RTileItem* item, void* user) {
	(void)tiles;
	(void)index;
	(void)user;
	item->text = "Swatch";
}

static void drag_out(Rid tiles, int index, void* user) {
	static const uint8_t image[2 * 2 * 4] = { 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255 };
	const char* list = "file:///tmp/swatch.png\r\n";
	RDataItem items[3];
	(void)index;
	(void)user;
	items[0].mime = "image/png";
	items[0].data = g_png;
	items[0].size = g_png_size;
	items[1].mime = "text/uri-list";
	items[1].data = list;
	items[1].size = strlen(list);
	items[2].mime = "application/x-raven-swatch";
	items[2].data = "3";
	items[2].size = 1;
	CHECK(Rdrag_begin_external(tiles, items, 3, R_DRAG_OPERATION_COPY, on_finished, NULL));
	Rdrag_set_image(image, 2, 2);
}

static NSEvent* mouse(NSView* view, NSEventType type, float x, float y) {
	NSPoint at = [view convertPoint:NSMakePoint(x, y) toView:nil];
	return [NSEvent mouseEventWithType:type location:at modifierFlags:0 timestamp:0
		windowNumber:[[view window] windowNumber] context:nil eventNumber:0 clickCount:1 pressure:1.0f];
}

static void test_drag(Rid window, NSView* view) {
	Rid tiles = Rwidget_create(R_WIDGET_TILE_VIEW);
	uint8_t* pixels = make_picture(16, 16);
	Method original = class_getInstanceMethod([NSView class], @selector(beginDraggingSessionWithItems:event:source:));
	NSEvent* last;
	float x, y, w, h;

	g_png = Rimage_encode_png(pixels, 16, 16, &g_png_size);
	free(pixels);
	CHECK(original != NULL);
	class_replaceMethod([view class], @selector(beginDraggingSessionWithItems:event:source:), (IMP)fake_begin_dragging,
		method_getTypeEncoding(original));
	CHECK(Rdrag_external_supported());

	Rtile_view_set_items(tiles, 3, tile_item, NULL);
	Rtile_view_set_drag_out_callback(tiles, drag_out, NULL);
	Rwindow_set_content(window, tiles);
	Rdraw_frame();
	Rwidget_get_rect(tiles, &x, &y, &w, &h);

	[view mouseDown:mouse(view, NSEventTypeLeftMouseDown, x + 30, y + 30)];
	[view mouseDragged:mouse(view, NSEventTypeLeftMouseDragged, x + 40, y + 40)];
	CHECK(Rdrag_external_is_active());
	CHECK(g_sessions == 0);
	last = mouse(view, NSEventTypeLeftMouseDragged, x + 46, y + 44);
	[view mouseDragged:last];
	CHECK(g_sessions == 1);
	CHECK(g_drag_event == last);
	CHECK(g_drag_source == view);

	/* the formats in the first item, the file in an item of its own (as the Finder writes them) */
	CHECK([g_dragged count] == 2);
	if ([g_dragged count] == 2) {
		NSDraggingItem* first = [g_dragged objectAtIndex:0];
		NSDraggingItem* second = [g_dragged objectAtIndex:1];
		NSPasteboardItem* data = (NSPasteboardItem*)[first item];
		NSPasteboardItem* file = (NSPasteboardItem*)[second item];
		NSData* png = [data dataForType:NSPasteboardTypePNG];
		NSRect frame = [first draggingFrame];
		NSPoint at = [view convertPoint:[last locationInWindow] fromView:nil];
		CHECK([data isKindOfClass:[NSPasteboardItem class]] && [file isKindOfClass:[NSPasteboardItem class]]);
		CHECK(png && [png length] == g_png_size && memcmp([png bytes], g_png, g_png_size) == 0);
		CHECK([data dataForType:NSPasteboardTypeTIFF] != nil);
		CHECK([[[[NSString alloc] initWithData:[data dataForType:@"org.raven.mime.application-2Fx-2Draven-2Dswatch"]
			encoding:NSUTF8StringEncoding] autorelease] isEqualToString:@"3"]);
		CHECK([[file stringForType:NSPasteboardTypeFileURL] isEqualToString:@"file:///tmp/swatch.png"]);
		/* the picture given follows the pointer, held at its center */
		CHECK(NSPointInRect(at, frame));
		CHECK(frame.size.width > 0 && frame.size.width <= 2.0);
	}
	CHECK([(id<NSDraggingSource>)view draggingSession:(NSDraggingSession*)[NSNull null]
		sourceOperationMaskForDraggingContext:NSDraggingContextOutsideApplication] == NSDragOperationCopy);
	CHECK(g_finished == 0);

	/* AppKit tells the end: copied by the target */
	[(id<NSDraggingSource>)view draggingSession:(NSDraggingSession*)[NSNull null] endedAtPoint:NSZeroPoint operation:NSDragOperationCopy];
	CHECK(g_finished == 1 && g_operation == R_DRAG_OPERATION_COPY);
	CHECK(!Rdrag_external_is_active());
	/* (told once) */
	[(id<NSDraggingSource>)view draggingSession:(NSDraggingSession*)[NSNull null] endedAtPoint:NSZeroPoint operation:NSDragOperationNone];
	CHECK(g_finished == 1);

	/* released before the pointer moved on: no session */
	[view mouseUp:mouse(view, NSEventTypeLeftMouseUp, x + 46, y + 44)];
	g_finished = 0;
	[view mouseDown:mouse(view, NSEventTypeLeftMouseDown, x + 60, y + 20)];
	[view mouseDragged:mouse(view, NSEventTypeLeftMouseDragged, x + 70, y + 30)];
	CHECK(Rdrag_external_is_active());
	[view mouseUp:mouse(view, NSEventTypeLeftMouseUp, x + 70, y + 30)];
	CHECK(g_sessions == 1);
	CHECK(g_finished == 1 && g_operation == R_DRAG_OPERATION_NONE);
	CHECK(!Rdrag_external_is_active());

	[g_dragged release];
	g_dragged = nil;
	Rpixels_free(g_png);
}

int main(void) {
	RInitDesc desc;
	RNativeWindow native;
	Rid window;
	NSView* view;
	NSArray* saved;
	@autoreleasepool {
		memset(&desc, 0, sizeof(desc));
		desc.backend = R_BACKEND_NULL;
		desc.worker_threads = 1;
		Rerror_set_callback(test_error_callback);
		if (!Rinit_ex(&desc)) return 1;
		Rset_animations_enabled(R_FALSE);
		window = Rwindow_create("Cocoa transfer tests", 320, 200, R_WINDOW_DEFAULT);
		Rdraw_frame();
		CHECK(Rwindow_get_native_handle(window, &native));
		view = [(NSWindow*)native.display contentView];

		saved = save_pasteboard();
		test_text();
		test_custom_and_files();
		test_image();
		restore_pasteboard(saved);
		test_drag(window, view);

		Rwindow_destroy(window);
		Rshutdown();
	}
	return test_report("test_cocoa_transfer");
}
