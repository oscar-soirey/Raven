/**
 * Raven - platform/apple/webkit_web_view.m
 * The web view of the system on macOS and iOS: a WKWebView kept over a
 * SystemWebView widget (see widgets/system_web_view.c), for the Cocoa and
 * UIKit platforms. Compiled with ARC.
 *
 * WebKit.framework is not linked: it is opened with dlopen when the first view
 * is made, and its classes are found by name, so a program that shows no web
 * page does not load WebKit (and Raven adds no framework to what an
 * application links). Only the declarations of the WebKit headers are used:
 * nothing here makes the linker look for a symbol of WebKit (no class
 * reference, no constant; the error domain and keys are written as strings).
 *
 * The view sits in a container of its own, a child of the content view of the
 * window: the container is the part of the widget that is seen, the WKWebView
 * inside it the whole widget, so that a ScrollBox cuts the page as it cuts the
 * other widgets.
 *
 * What WebKit tells (title, address, loading, messages, results) is sent to
 * the core later, from the main queue, never during a call of the core: a
 * load can change the address of the view before it returns.
 */

#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#define RVN_WK_VIEW UIView
#else
#import <AppKit/AppKit.h>
#define RVN_WK_VIEW NSView
#endif
#import <WebKit/WebKit.h>
#include <dlfcn.h>

#include "webkit_web_view.h"

#if !__has_feature(objc_arc)
#error "webkit_web_view.m is compiled with -fobjc-arc"
#endif

/* Defines window.raven.postMessage in each page, before its own scripts: a string as it is, anything else as JSON. */
static NSString* const RVN_WK_BRIDGE_SCRIPT =
	@"(function () {"
	"  var handler = window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.raven;"
	"  if (!handler) return;"
	"  var raven = { postMessage: function (value) {"
	"    handler.postMessage(typeof value === 'string' ? value : JSON.stringify(value === undefined ? null : value));"
	"  } };"
	"  Object.defineProperty(window, 'raven', { value: Object.freeze(raven), writable: false, configurable: false });"
	"})();";

/* The result of a promise is waited for; indirect eval runs the script as a program of the page (its last value). */
static NSString* const RVN_WK_RUN_SCRIPT = @"return await (0, eval)(ravenScript);";

static void* g_rvn_wk_observe = &g_rvn_wk_observe;

static struct {
	int tried;
	int available;
} g_rvn_wk;

int rvn_webkit_supported(void) {
	if (!g_rvn_wk.tried) {
		void* library;
		g_rvn_wk.tried = 1;
		library = dlopen("/System/Library/Frameworks/WebKit.framework/WebKit", RTLD_LAZY | RTLD_LOCAL);
		/* (the handle stays open: the classes of WebKit live as long as the program) */
		g_rvn_wk.available = library && NSClassFromString(@"WKWebView") && NSClassFromString(@"WKWebViewConfiguration") &&
			NSClassFromString(@"WKUserScript");
	}
	return g_rvn_wk.available;
}

static NSString* rvn_wk_string(const char* text) {
	NSString* string = text ? [NSString stringWithUTF8String:text] : nil;
	return string ? string : @"";
}

/* A value of JavaScript, as WebKit gives it, written as JSON. */
static NSString* rvn_wk_json(id value) {
	NSData* data;
	if (!value || value == [NSNull null]) return @"null";
	if ([value isKindOfClass:[NSDate class]]) {
		NSISO8601DateFormatter* format = [[NSISO8601DateFormatter alloc] init];
		value = [format stringFromDate:(NSDate*)value];
	}
	if ([value isKindOfClass:[NSNumber class]]) {
		double number = [(NSNumber*)value doubleValue];
		if (number != number || number - number != 0.0) return @"null"; /* NaN and the infinities are not JSON */
	}
	if (!([value isKindOfClass:[NSString class]] || [value isKindOfClass:[NSNumber class]] ||
		[NSJSONSerialization isValidJSONObject:value])) return @"null";
	data = [NSJSONSerialization dataWithJSONObject:value options:NSJSONWritingFragmentsAllowed error:nil];
	if (!data) return @"null";
	return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
}

#if !TARGET_OS_IPHONE
/* The part of the widget that is seen: counted from its top left corner, as the content view of Raven is. */
@interface RavenWebContainer : NSView
@end
@implementation RavenWebContainer
- (BOOL)isFlipped { return YES; }
@end
#endif

@interface RavenWebBridge : NSObject <WKNavigationDelegate, WKUIDelegate, WKScriptMessageHandler>
{
@public
	void* user;
	BOOL closed;
	RVN_WK_VIEW* container;
	WKWebView* web;
}
@end

@implementation RavenWebBridge

/* Sends an event to the core from the main queue, unless the view was destroyed in between. */
- (void)send:(rvn_pweb_event_type_t)type text:(NSString*)text error:(NSString*)error request:(uint64_t)request {
	RavenWebBridge* bridge = self;
	dispatch_async(dispatch_get_main_queue(), ^{
		rvn_pweb_event_t event;
		if (bridge->closed) return;
		memset(&event, 0, sizeof(event));
		event.type = type;
		event.user = bridge->user;
		event.text = text ? [text UTF8String] : NULL;
		event.error = error ? [error UTF8String] : NULL;
		event.request = request;
		rvn_web_view_event(&event);
	});
}

- (void)observeValueForKeyPath:(NSString*)keyPath ofObject:(id)object change:(NSDictionary*)change context:(void*)context {
	if (context != g_rvn_wk_observe) {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
		return;
	}
	if (closed) return;
	if ([keyPath isEqualToString:@"title"]) {
		NSString* title = web.title;
		[self send:RVN_PWEB_TITLE text:title ? title : @"" error:nil request:0];
	} else if ([keyPath isEqualToString:@"URL"]) {
		NSString* url = web.URL ? web.URL.absoluteString : nil;
		[self send:RVN_PWEB_URL text:url ? url : @"" error:nil request:0];
	}
}

- (void)webView:(WKWebView*)webView didStartProvisionalNavigation:(WKNavigation*)navigation {
	(void)webView; (void)navigation;
	[self send:RVN_PWEB_LOADING text:nil error:nil request:0];
}

- (void)webView:(WKWebView*)webView didFinishNavigation:(WKNavigation*)navigation {
	(void)webView; (void)navigation;
	[self send:RVN_PWEB_LOADED text:nil error:nil request:0];
}

- (void)failed:(NSError*)error {
	/* a load stopped by another one (or by stop) is not a failure */
	if ([error.domain isEqualToString:NSURLErrorDomain] && error.code == NSURLErrorCancelled) return;
	[self send:RVN_PWEB_FAILED text:nil error:error.localizedDescription ? error.localizedDescription : @"the page could not be loaded" request:0];
}

- (void)webView:(WKWebView*)webView didFailProvisionalNavigation:(WKNavigation*)navigation withError:(NSError*)error {
	(void)webView; (void)navigation;
	[self failed:error];
}

- (void)webView:(WKWebView*)webView didFailNavigation:(WKNavigation*)navigation withError:(NSError*)error {
	(void)webView; (void)navigation;
	[self failed:error];
}

- (void)webViewWebContentProcessDidTerminate:(WKWebView*)webView {
	(void)webView;
	[self send:RVN_PWEB_FAILED text:nil error:@"the process that showed the page stopped" request:0];
}

/* A link to a new window (target="_blank", window.open) opens in this view: there is no other. */
- (WKWebView*)webView:(WKWebView*)webView createWebViewWithConfiguration:(WKWebViewConfiguration*)configuration
	forNavigationAction:(WKNavigationAction*)action windowFeatures:(WKWindowFeatures*)features {
	(void)configuration; (void)features;
	if (!action.targetFrame || !action.targetFrame.isMainFrame) [webView loadRequest:action.request];
	return nil;
}

- (void)userContentController:(WKUserContentController*)controller didReceiveScriptMessage:(WKScriptMessage*)message {
	id body = message.body;
	(void)controller;
	if (closed) return;
	[self send:RVN_PWEB_MESSAGE text:[body isKindOfClass:[NSString class]] ? (NSString*)body : rvn_wk_json(body) error:nil request:0];
}

/* The end of a script: its value, or why it failed. */
- (void)finish:(uint64_t)request result:(id)result error:(NSError*)error {
	if (error) {
		NSString* message = error.userInfo[@"WKJavaScriptExceptionMessage"];
		/* WebKit refuses some values (a function, a node): the script ran, its value is not one JSON can say */
		if ([error.domain isEqualToString:@"WKErrorDomain"] && error.code == WKErrorJavaScriptResultTypeIsUnsupported) {
			[self send:RVN_PWEB_RESULT text:@"null" error:nil request:request];
			return;
		}
		if (!message.length) message = error.localizedDescription;
		[self send:RVN_PWEB_RESULT text:nil error:message.length ? message : @"the script failed" request:request];
		return;
	}
	[self send:RVN_PWEB_RESULT text:rvn_wk_json(result) error:nil request:request];
}

- (void)evaluate:(NSString*)script request:(uint64_t)request {
	RavenWebBridge* bridge = self;
	[web evaluateJavaScript:script completionHandler:^(id result, NSError* error) {
		[bridge finish:request result:result error:error];
	}];
}

- (void)run:(NSString*)script request:(uint64_t)request {
	RavenWebBridge* bridge = self;
	if (@available(macOS 11.0, iOS 14.0, *)) {
		Class world = NSClassFromString(@"WKContentWorld");
		if (world) {
			[web callAsyncJavaScript:RVN_WK_RUN_SCRIPT arguments:@{ @"ravenScript": script } inFrame:nil
				inContentWorld:[world pageWorld] completionHandler:^(id result, NSError* error) {
				NSString* message = error ? error.userInfo[@"WKJavaScriptExceptionMessage"] : nil;
				if (bridge->closed) return;
				/* a page whose Content-Security-Policy forbids eval: the script did not run, it runs as a plain program */
				if (message && ([message rangeOfString:@"EvalError"].location != NSNotFound ||
					[message rangeOfString:@"unsafe-eval"].location != NSNotFound)) {
					[bridge evaluate:script request:request];
					return;
				}
				[bridge finish:request result:result error:error];
			}];
			return;
		}
	}
	[self evaluate:script request:request];
}

- (void)close {
	if (closed) return;
	closed = YES;
	[web removeObserver:self forKeyPath:@"title" context:g_rvn_wk_observe];
	[web removeObserver:self forKeyPath:@"URL" context:g_rvn_wk_observe];
	/* (the content controller holds its handler: the bridge would never be freed) */
	[web.configuration.userContentController removeScriptMessageHandlerForName:@"raven"];
	web.navigationDelegate = nil;
	web.UIDelegate = nil;
	[web stopLoading];
	[web removeFromSuperview];
	[container removeFromSuperview];
	web = nil;
	container = nil;
}

@end

static RavenWebBridge* rvn_wk_bridge(rvn_pweb_view_t* view) {
	return (__bridge RavenWebBridge*)(void*)view;
}

rvn_pweb_view_t* rvn_webkit_create(void* user) {
	RavenWebBridge* bridge;
	WKWebViewConfiguration* configuration;
	WKUserScript* script;
	CGRect frame = CGRectMake(0.0, 0.0, 64.0, 64.0);
	if (!rvn_webkit_supported()) return NULL;
	bridge = [[RavenWebBridge alloc] init];
	bridge->user = user;
	configuration = [[NSClassFromString(@"WKWebViewConfiguration") alloc] init];
	script = [[NSClassFromString(@"WKUserScript") alloc] initWithSource:RVN_WK_BRIDGE_SCRIPT
		injectionTime:WKUserScriptInjectionTimeAtDocumentStart forMainFrameOnly:YES];
	[configuration.userContentController addUserScript:script];
	[configuration.userContentController addScriptMessageHandler:bridge name:@"raven"];
	bridge->web = [[NSClassFromString(@"WKWebView") alloc] initWithFrame:frame configuration:configuration];
	if (!bridge->web) return NULL;
	bridge->web.navigationDelegate = bridge;
	bridge->web.UIDelegate = bridge;
	[bridge->web addObserver:bridge forKeyPath:@"title" options:NSKeyValueObservingOptionNew context:g_rvn_wk_observe];
	[bridge->web addObserver:bridge forKeyPath:@"URL" options:NSKeyValueObservingOptionNew context:g_rvn_wk_observe];
#if TARGET_OS_IPHONE
	bridge->container = [[UIView alloc] initWithFrame:frame];
	bridge->container.clipsToBounds = YES;
#else
	bridge->container = [[RavenWebContainer alloc] initWithFrame:frame];
	[bridge->container setWantsLayer:YES];
	bridge->container.layer.masksToBounds = YES;
#endif
	bridge->container.hidden = YES;
	[bridge->container addSubview:bridge->web];
	return (rvn_pweb_view_t*)CFBridgingRetain(bridge);
}

void rvn_webkit_destroy(rvn_pweb_view_t* view) {
	RavenWebBridge* bridge;
	if (!view) return;
	bridge = (RavenWebBridge*)CFBridgingRelease((CFTypeRef)view);
	[bridge close];
}

void rvn_webkit_set_parent_view(rvn_pweb_view_t* view, void* parent_view) {
	RavenWebBridge* bridge = rvn_wk_bridge(view);
	RVN_WK_VIEW* parent = (__bridge RVN_WK_VIEW*)parent_view;
	if (!bridge || bridge->closed) return;
	bridge->container.hidden = YES;
	if (!parent) {
		[bridge->container removeFromSuperview];
		return;
	}
	if (bridge->container.superview != parent) [parent addSubview:bridge->container];
}

void rvn_webkit_set_rect(rvn_pweb_view_t* view, const int rect[4], const int clip[4], float scale, int visible) {
	RavenWebBridge* bridge = rvn_wk_bridge(view);
	CGFloat s = scale > 0.0f ? (CGFloat)scale : 1.0;
	if (!bridge || bridge->closed) return;
	if (!visible) {
		bridge->container.hidden = YES;
		return;
	}
	/* points of the content view (top left origin on both systems: Raven's NSView is flipped) */
	bridge->container.frame = CGRectMake(clip[0] / s, clip[1] / s, clip[2] / s, clip[3] / s);
	bridge->web.frame = CGRectMake((rect[0] - clip[0]) / s, (rect[1] - clip[1]) / s, rect[2] / s, rect[3] / s);
	bridge->container.hidden = NO;
}

void rvn_webkit_load_url(rvn_pweb_view_t* view, const char* url) {
	RavenWebBridge* bridge = rvn_wk_bridge(view);
	NSURL* address;
	if (!bridge || bridge->closed) return;
	address = [NSURL URLWithString:rvn_wk_string(url)];
	if (!address) {
		[bridge send:RVN_PWEB_FAILED text:nil error:@"not a valid address" request:0];
		return;
	}
	if (address.isFileURL) {
		/* a page of the computer may read the files of its folder */
		[bridge->web loadFileURL:address allowingReadAccessToURL:[address URLByDeletingLastPathComponent]];
	} else {
		[bridge->web loadRequest:[NSURLRequest requestWithURL:address]];
	}
}

void rvn_webkit_load_html(rvn_pweb_view_t* view, const char* html, const char* base_url) {
	RavenWebBridge* bridge = rvn_wk_bridge(view);
	NSURL* base = base_url ? [NSURL URLWithString:rvn_wk_string(base_url)] : nil;
	if (!bridge || bridge->closed) return;
	[bridge->web loadHTMLString:rvn_wk_string(html) baseURL:base];
}

void rvn_webkit_command(rvn_pweb_view_t* view, rvn_pweb_command_t command) {
	RavenWebBridge* bridge = rvn_wk_bridge(view);
	if (!bridge || bridge->closed) return;
	switch (command) {
		case RVN_PWEB_BACK: [bridge->web goBack]; break;
		case RVN_PWEB_FORWARD: [bridge->web goForward]; break;
		case RVN_PWEB_RELOAD: [bridge->web reload]; break;
		case RVN_PWEB_STOP: [bridge->web stopLoading]; break;
	}
}

int rvn_webkit_can(rvn_pweb_view_t* view, rvn_pweb_command_t command) {
	RavenWebBridge* bridge = rvn_wk_bridge(view);
	if (!bridge || bridge->closed) return 0;
	if (command == RVN_PWEB_BACK) return bridge->web.canGoBack ? 1 : 0;
	if (command == RVN_PWEB_FORWARD) return bridge->web.canGoForward ? 1 : 0;
	return 1;
}

void rvn_webkit_run_javascript(rvn_pweb_view_t* view, const char* script, uint64_t request) {
	RavenWebBridge* bridge = rvn_wk_bridge(view);
	if (!bridge || bridge->closed) return;
	[bridge run:rvn_wk_string(script) request:request];
}

void rvn_webkit_focus(rvn_pweb_view_t* view, int focused) {
	RavenWebBridge* bridge = rvn_wk_bridge(view);
	if (!bridge || bridge->closed) return;
#if TARGET_OS_IPHONE
	if (focused) [bridge->web becomeFirstResponder];
	else [bridge->web endEditing:YES];
#else
	{
		NSWindow* window = [bridge->web window];
		NSResponder* responder = [window firstResponder];
		if (!window) return;
		if (focused) {
			[window makeFirstResponder:bridge->web];
		} else if ([responder isKindOfClass:[NSView class]] && [(NSView*)responder isDescendantOf:bridge->container]) {
			/* the keyboard goes back to Raven's content view */
			[window makeFirstResponder:bridge->container.superview];
		}
	}
#endif
}

void* rvn_webkit_native(rvn_pweb_view_t* view) {
	RavenWebBridge* bridge = rvn_wk_bridge(view);
	return bridge && !bridge->closed ? (__bridge void*)bridge->web : NULL;
}
