/**
 * Raven - platform/cocoa/cocoa_print.m
 * Printing a PDF document on macOS (Rpdf_print, widgets/pdf_print.c): an
 * NSPrintOperation over a view that draws each page with Core Graphics,
 * shrunk to the area the printer prints when it is larger. The print panel
 * chooses the printer, the copies, the pages and the paper; it also saves
 * the job as a PDF or opens it in Preview.
 *
 * Manual reference counting, as the rest of the Cocoa backend.
 */

#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>

#include "../../widgets/common.h"

@interface RavenPdfPrintView : NSView {
@public
	CGPDFDocumentRef document;
	size_t pageCount;
	CGFloat slot;
}
@end

@implementation RavenPdfPrintView

- (BOOL)isFlipped {
	return NO;
}

- (BOOL)knowsPageRange:(NSRangePointer)range {
	range->location = 1;
	range->length = pageCount;
	return YES;
}

/* Page n in a slot of its own, the first at the top; as large as the paper chosen in the panel. */
- (NSRect)rectForPage:(NSInteger)page {
	NSPrintOperation* operation = [NSPrintOperation currentOperation];
	NSSize paper = operation ? [[operation printInfo] paperSize] : NSMakeSize(612, 792);
	return NSMakeRect(0, (CGFloat)((NSInteger)pageCount - page) * slot, paper.width, paper.height);
}

- (void)drawRect:(NSRect)dirty {
	NSPrintOperation* operation = [NSPrintOperation currentOperation];
	NSInteger number = operation ? [operation currentPage] : 1;
	CGPDFPageRef page;
	CGContextRef context;
	NSRect frame, area;
	CGRect box;
	(void)dirty;
	if (number < 1 || (size_t)number > pageCount) return;
	page = CGPDFDocumentGetPage(document, (size_t)number);
	context = [[NSGraphicsContext currentContext] CGContext];
	if (!page || !context) return;
	frame = [self rectForPage:number];
	/* what the printer can print, on this paper */
	area = operation ? [[operation printInfo] imageablePageBounds] : NSMakeRect(0, 0, frame.size.width, frame.size.height);
	area.origin.x += frame.origin.x;
	area.origin.y += frame.origin.y;
	box = CGPDFPageGetBoxRect(page, kCGPDFCropBox);
	CGContextSaveGState(context);
	/* centered, rotated as the page asks, made smaller only when it does not fit */
	CGContextConcatCTM(context, CGPDFPageGetDrawingTransform(page, kCGPDFCropBox, NSRectToCGRect(area), 0, true));
	CGContextClipToRect(context, box);
	CGContextDrawPDFPage(context, page);
	CGContextRestoreGState(context);
}

@end

int rvn_cocoa_print_pdf(const char* data, size_t size, const char* title, const char* printer, int copies,
	int show_panel, const char* output, const char* password, void* window) {
	int ok = R_FALSE;
	(void)window;
	@autoreleasepool {
		CGDataProviderRef provider;
		CGPDFDocumentRef document;
		RavenPdfPrintView* view;
		NSPrintInfo* info;
		NSPrintOperation* operation;
		CGPDFPageRef first;
		CGFloat slot;
		size_t count;

		provider = CGDataProviderCreateWithData(NULL, data, size, NULL);
		document = provider ? CGPDFDocumentCreateWithProvider(provider) : NULL;
		CGDataProviderRelease(provider);
		if (!document) {
			rvn_error(R_ERROR_INVALID_FILE_FORMAT, R_SEVERITY_ERROR, "Rpdf_print: the document cannot be opened");
			return R_FALSE;
		}
		if (CGPDFDocumentIsEncrypted(document) && !CGPDFDocumentIsUnlocked(document)) {
			if (!password || !CGPDFDocumentUnlockWithPassword(document, password)) {
				rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rpdf_print: the document needs its password");
				CGPDFDocumentRelease(document);
				return R_FALSE;
			}
		}
		count = CGPDFDocumentGetNumberOfPages(document);
		if (count == 0) {
			CGPDFDocumentRelease(document);
			return R_FALSE;
		}

		info = [[[NSPrintInfo sharedPrintInfo] copy] autorelease];
		if (printer && printer[0]) {
			NSPrinter* named = [NSPrinter printerWithName:[NSString stringWithUTF8String:printer]];
			if (!named) {
				rvn_error(R_ERROR_PLATFORM, R_SEVERITY_ERROR, "Rpdf_print: no printer named '%s'", printer);
				CGPDFDocumentRelease(document);
				return R_FALSE;
			}
			[info setPrinter:named];
		}
		/* a document of wide pages is printed across the paper */
		first = CGPDFDocumentGetPage(document, 1);
		if (first) {
			CGRect box = CGPDFPageGetBoxRect(first, kCGPDFCropBox);
			int rotation = CGPDFPageGetRotationAngle(first);
			int wide = box.size.width > box.size.height;
			if (rotation == 90 || rotation == 270) wide = !wide;
			[info setOrientation:wide ? NSPaperOrientationLandscape : NSPaperOrientationPortrait];
		}
		[info setTopMargin:0];
		[info setBottomMargin:0];
		[info setLeftMargin:0];
		[info setRightMargin:0];
		[info setHorizontalPagination:NSPrintingPaginationModeClip];
		[info setVerticalPagination:NSPrintingPaginationModeClip];
		[info setHorizontallyCentered:NO];
		[info setVerticallyCentered:NO];
		[[info dictionary] setObject:[NSNumber numberWithInt:copies > 0 ? copies : 1] forKey:NSPrintCopies];
		if (output && output[0]) {
			[info setJobDisposition:NSPrintSaveJob];
			[[info dictionary] setObject:[NSURL fileURLWithPath:[NSString stringWithUTF8String:output]] forKey:NSPrintJobSavingURL];
		}

		/* slots as large as the largest paper, whichever the panel chooses */
		slot = 14400;
		view = [[[RavenPdfPrintView alloc] initWithFrame:NSMakeRect(0, 0, slot, slot * (CGFloat)count)] autorelease];
		view->document = document;
		view->pageCount = count;
		view->slot = slot;

		operation = [NSPrintOperation printOperationWithView:view printInfo:info];
		[operation setJobTitle:[NSString stringWithUTF8String:title && title[0] ? title : "Document"]];
		[operation setShowsPrintPanel:show_panel ? YES : NO];
		[operation setShowsProgressPanel:show_panel ? YES : NO];
		[[operation printPanel] setOptions:NSPrintPanelShowsCopies | NSPrintPanelShowsPageRange |
			NSPrintPanelShowsPaperSize | NSPrintPanelShowsOrientation | NSPrintPanelShowsPreview];
		ok = [operation runOperation] ? R_TRUE : R_FALSE;
		CGPDFDocumentRelease(document);
	}
	return ok;
}
