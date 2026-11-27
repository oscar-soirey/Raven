/**
 * Raven - platform/win32/webview2_com.h
 * The few COM interfaces of Microsoft Edge WebView2 that win32_web_view.c
 * uses, declared in C, so that Raven needs neither the WebView2 SDK
 * (WebView2.h, NuGet) nor WebView2Loader.lib to build.
 *
 * Copied from WebView2.h of the SDK 1.0.2903.40 (package
 * Microsoft.Web.WebView2, the MIDL output of WebView2.idl), the C section
 * ("C style interface"): same identifiers, same slots in the same order.
 * The interfaces of WebView2 never change once released (new methods go to
 * new interfaces: ICoreWebView2_2, _3...), and only the first version of
 * each is used here, so any runtime has them.
 *
 * A vtable is written up to the last slot used; the slots before it that are
 * not called are `void*` named after the method they hold (unused_*), so
 * that the order can be compared line by line with WebView2.h. Slots 0 to 2
 * are those of IUnknown. Enumerations are passed as `int` (what a C enum
 * is), EventRegistrationToken as the 8-byte structure it is.
 */

#ifndef RVN_WEBVIEW2_COM_H
#define RVN_WEBVIEW2_COM_H

#include <windows.h>
#include <objidl.h>

/* EventRegistrationToken of eventtoken.h: { __int64 value; } */
typedef struct rvn_wv2_token_t {
	__int64 value;
} rvn_wv2_token_t;

/* COREWEBVIEW2_WEB_ERROR_STATUS (the values used here) */
#define RVN_WV2_ERROR_UNKNOWN 0
#define RVN_WV2_ERROR_SERVER_UNREACHABLE 6
#define RVN_WV2_ERROR_TIMEOUT 7
#define RVN_WV2_ERROR_CONNECTION_ABORTED 9
#define RVN_WV2_ERROR_CONNECTION_RESET 10
#define RVN_WV2_ERROR_DISCONNECTED 11
#define RVN_WV2_ERROR_CANNOT_CONNECT 12
#define RVN_WV2_ERROR_HOST_NAME_NOT_RESOLVED 13
#define RVN_WV2_ERROR_OPERATION_CANCELED 14
/* COREWEBVIEW2_WEB_RESOURCE_CONTEXT_DOCUMENT */
#define RVN_WV2_RESOURCE_DOCUMENT 1
/* COREWEBVIEW2_PROCESS_FAILED_KIND: the processes whose end stops the page (the others are started again) */
#define RVN_WV2_BROWSER_PROCESS_EXITED 0
#define RVN_WV2_RENDER_PROCESS_EXITED 1
#define RVN_WV2_RENDER_PROCESS_UNRESPONSIVE 2
/* COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC */
#define RVN_WV2_FOCUS_PROGRAMMATIC 0

typedef struct rvn_wv2_environment_t rvn_wv2_environment_t;
typedef struct rvn_wv2_controller_t rvn_wv2_controller_t;
typedef struct rvn_wv2_core_t rvn_wv2_core_t;
typedef struct rvn_wv2_navigation_starting_args_t rvn_wv2_navigation_starting_args_t;
typedef struct rvn_wv2_navigation_completed_args_t rvn_wv2_navigation_completed_args_t;
typedef struct rvn_wv2_message_args_t rvn_wv2_message_args_t;
typedef struct rvn_wv2_new_window_args_t rvn_wv2_new_window_args_t;
typedef struct rvn_wv2_resource_args_t rvn_wv2_resource_args_t;
typedef struct rvn_wv2_resource_request_t rvn_wv2_resource_request_t;
typedef struct rvn_wv2_process_failed_args_t rvn_wv2_process_failed_args_t;
/* (objects given back and only passed on or released: their own methods are not called) */
typedef IUnknown rvn_wv2_resource_response_t;

/*
 * The handlers Raven implements all have the shape of
 * ICoreWebView2...Handler: IUnknown, then Invoke. Each has its own vtable
 * type below (the arguments of Invoke differ).
 */

/* ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler {4E8A3389-C9D8-4BD2-B6B5-124FEE6CC14D} */
typedef struct rvn_wv2_environment_done_t rvn_wv2_environment_done_t;
typedef struct rvn_wv2_environment_done_vtbl_t {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(rvn_wv2_environment_done_t* self, REFIID riid, void** object);
	ULONG (STDMETHODCALLTYPE *AddRef)(rvn_wv2_environment_done_t* self);
	ULONG (STDMETHODCALLTYPE *Release)(rvn_wv2_environment_done_t* self);
	HRESULT (STDMETHODCALLTYPE *Invoke)(rvn_wv2_environment_done_t* self, HRESULT errorCode, rvn_wv2_environment_t* result);
} rvn_wv2_environment_done_vtbl_t;
struct rvn_wv2_environment_done_t { const rvn_wv2_environment_done_vtbl_t* lpVtbl; };

/* ICoreWebView2CreateCoreWebView2ControllerCompletedHandler {6C4819F3-C9B7-4260-8127-C9F5BDE7F68C} */
typedef struct rvn_wv2_controller_done_t rvn_wv2_controller_done_t;
typedef struct rvn_wv2_controller_done_vtbl_t {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(rvn_wv2_controller_done_t* self, REFIID riid, void** object);
	ULONG (STDMETHODCALLTYPE *AddRef)(rvn_wv2_controller_done_t* self);
	ULONG (STDMETHODCALLTYPE *Release)(rvn_wv2_controller_done_t* self);
	HRESULT (STDMETHODCALLTYPE *Invoke)(rvn_wv2_controller_done_t* self, HRESULT errorCode, rvn_wv2_controller_t* result);
} rvn_wv2_controller_done_vtbl_t;
struct rvn_wv2_controller_done_t { const rvn_wv2_controller_done_vtbl_t* lpVtbl; };

/*
 * ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler {B99369F3-9B11-47B5-BC6F-8E7895FCEA17}
 * and ICoreWebView2ExecuteScriptCompletedHandler {49511172-CC67-4BCA-9923-137112F4C4CC}:
 * both Invoke(HRESULT errorCode, LPCWSTR result).
 */
typedef struct rvn_wv2_string_done_t rvn_wv2_string_done_t;
typedef struct rvn_wv2_string_done_vtbl_t {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(rvn_wv2_string_done_t* self, REFIID riid, void** object);
	ULONG (STDMETHODCALLTYPE *AddRef)(rvn_wv2_string_done_t* self);
	ULONG (STDMETHODCALLTYPE *Release)(rvn_wv2_string_done_t* self);
	HRESULT (STDMETHODCALLTYPE *Invoke)(rvn_wv2_string_done_t* self, HRESULT errorCode, LPCWSTR result);
} rvn_wv2_string_done_vtbl_t;
struct rvn_wv2_string_done_t { const rvn_wv2_string_done_vtbl_t* lpVtbl; };

/*
 * The event handlers: Invoke(ICoreWebView2* sender, <args>* args).
 *   ICoreWebView2NavigationStartingEventHandler   {9ADBE429-F36D-432B-9DDC-F8881FBD76E3}
 *   ICoreWebView2NavigationCompletedEventHandler  {D33A35BF-1C49-4F98-93AB-006E0533FE1C}
 *   ICoreWebView2SourceChangedEventHandler        {3C067F9F-5388-4772-8B48-79F7EF1AB37C}
 *   ICoreWebView2DocumentTitleChangedEventHandler {F5F2B923-953E-4042-9F95-F3A118E1AFD4}
 *   ICoreWebView2WebMessageReceivedEventHandler   {57213F19-00E6-49FA-8E07-898EA01ECBD2}
 *   ICoreWebView2NewWindowRequestedEventHandler   {D4C185FE-C81C-4989-97AF-2D3FA7AB5651}
 *   ICoreWebView2ProcessFailedEventHandler        {79E0AEA4-990B-42D9-AA1D-0FCC2E5BC7F1}
 *   ICoreWebView2WebResourceRequestedEventHandler {AB00B74C-15F1-4646-80E8-E76341D25D71}
 * The arguments are an interface pointer in every case; `void*` here, each
 * Invoke casts it to the interface of its event.
 */
typedef struct rvn_wv2_event_handler_t rvn_wv2_event_handler_t;
typedef struct rvn_wv2_event_handler_vtbl_t {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(rvn_wv2_event_handler_t* self, REFIID riid, void** object);
	ULONG (STDMETHODCALLTYPE *AddRef)(rvn_wv2_event_handler_t* self);
	ULONG (STDMETHODCALLTYPE *Release)(rvn_wv2_event_handler_t* self);
	HRESULT (STDMETHODCALLTYPE *Invoke)(rvn_wv2_event_handler_t* self, rvn_wv2_core_t* sender, void* args);
} rvn_wv2_event_handler_vtbl_t;
struct rvn_wv2_event_handler_t { const rvn_wv2_event_handler_vtbl_t* lpVtbl; };

/* ICoreWebView2Environment {B96D755E-0319-4E92-A296-23436F46A1FC} */
typedef struct rvn_wv2_environment_vtbl_t {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(rvn_wv2_environment_t* self, REFIID riid, void** object);
	ULONG (STDMETHODCALLTYPE *AddRef)(rvn_wv2_environment_t* self);
	ULONG (STDMETHODCALLTYPE *Release)(rvn_wv2_environment_t* self);
	HRESULT (STDMETHODCALLTYPE *CreateCoreWebView2Controller)(rvn_wv2_environment_t* self, HWND parentWindow, rvn_wv2_controller_done_t* handler);
	HRESULT (STDMETHODCALLTYPE *CreateWebResourceResponse)(rvn_wv2_environment_t* self, IStream* content, int statusCode,
		LPCWSTR reasonPhrase, LPCWSTR headers, rvn_wv2_resource_response_t** response);
	HRESULT (STDMETHODCALLTYPE *get_BrowserVersionString)(rvn_wv2_environment_t* self, LPWSTR* versionInfo);
} rvn_wv2_environment_vtbl_t;
struct rvn_wv2_environment_t { const rvn_wv2_environment_vtbl_t* lpVtbl; };

/* ICoreWebView2Controller {4D00C0D1-9434-4EB6-8078-8697A560334F} */
typedef struct rvn_wv2_controller_vtbl_t {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(rvn_wv2_controller_t* self, REFIID riid, void** object);
	ULONG (STDMETHODCALLTYPE *AddRef)(rvn_wv2_controller_t* self);
	ULONG (STDMETHODCALLTYPE *Release)(rvn_wv2_controller_t* self);
	void* unused_get_IsVisible;
	HRESULT (STDMETHODCALLTYPE *put_IsVisible)(rvn_wv2_controller_t* self, BOOL isVisible);
	void* unused_get_Bounds;
	HRESULT (STDMETHODCALLTYPE *put_Bounds)(rvn_wv2_controller_t* self, RECT bounds);
	void* unused_get_ZoomFactor;
	void* unused_put_ZoomFactor;
	void* unused_add_ZoomFactorChanged;
	void* unused_remove_ZoomFactorChanged;
	void* unused_SetBoundsAndZoomFactor;
	HRESULT (STDMETHODCALLTYPE *MoveFocus)(rvn_wv2_controller_t* self, int reason);
	void* unused_add_MoveFocusRequested;
	void* unused_remove_MoveFocusRequested;
	void* unused_add_GotFocus;
	void* unused_remove_GotFocus;
	void* unused_add_LostFocus;
	void* unused_remove_LostFocus;
	void* unused_add_AcceleratorKeyPressed;
	void* unused_remove_AcceleratorKeyPressed;
	void* unused_get_ParentWindow;
	HRESULT (STDMETHODCALLTYPE *put_ParentWindow)(rvn_wv2_controller_t* self, HWND parentWindow);
	HRESULT (STDMETHODCALLTYPE *NotifyParentWindowPositionChanged)(rvn_wv2_controller_t* self);
	HRESULT (STDMETHODCALLTYPE *Close)(rvn_wv2_controller_t* self);
	HRESULT (STDMETHODCALLTYPE *get_CoreWebView2)(rvn_wv2_controller_t* self, rvn_wv2_core_t** coreWebView2);
} rvn_wv2_controller_vtbl_t;
struct rvn_wv2_controller_t { const rvn_wv2_controller_vtbl_t* lpVtbl; };

/* ICoreWebView2 {76ECEACB-0462-4D94-AC83-423A6793775E} */
typedef struct rvn_wv2_core_vtbl_t {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(rvn_wv2_core_t* self, REFIID riid, void** object);
	ULONG (STDMETHODCALLTYPE *AddRef)(rvn_wv2_core_t* self);
	ULONG (STDMETHODCALLTYPE *Release)(rvn_wv2_core_t* self);
	void* unused_get_Settings;
	HRESULT (STDMETHODCALLTYPE *get_Source)(rvn_wv2_core_t* self, LPWSTR* uri);
	HRESULT (STDMETHODCALLTYPE *Navigate)(rvn_wv2_core_t* self, LPCWSTR uri);
	HRESULT (STDMETHODCALLTYPE *NavigateToString)(rvn_wv2_core_t* self, LPCWSTR htmlContent);
	HRESULT (STDMETHODCALLTYPE *add_NavigationStarting)(rvn_wv2_core_t* self, rvn_wv2_event_handler_t* eventHandler, rvn_wv2_token_t* token);
	HRESULT (STDMETHODCALLTYPE *remove_NavigationStarting)(rvn_wv2_core_t* self, rvn_wv2_token_t token);
	void* unused_add_ContentLoading;
	void* unused_remove_ContentLoading;
	HRESULT (STDMETHODCALLTYPE *add_SourceChanged)(rvn_wv2_core_t* self, rvn_wv2_event_handler_t* eventHandler, rvn_wv2_token_t* token);
	HRESULT (STDMETHODCALLTYPE *remove_SourceChanged)(rvn_wv2_core_t* self, rvn_wv2_token_t token);
	void* unused_add_HistoryChanged;
	void* unused_remove_HistoryChanged;
	HRESULT (STDMETHODCALLTYPE *add_NavigationCompleted)(rvn_wv2_core_t* self, rvn_wv2_event_handler_t* eventHandler, rvn_wv2_token_t* token);
	HRESULT (STDMETHODCALLTYPE *remove_NavigationCompleted)(rvn_wv2_core_t* self, rvn_wv2_token_t token);
	void* unused_add_FrameNavigationStarting;
	void* unused_remove_FrameNavigationStarting;
	void* unused_add_FrameNavigationCompleted;
	void* unused_remove_FrameNavigationCompleted;
	void* unused_add_ScriptDialogOpening;
	void* unused_remove_ScriptDialogOpening;
	void* unused_add_PermissionRequested;
	void* unused_remove_PermissionRequested;
	HRESULT (STDMETHODCALLTYPE *add_ProcessFailed)(rvn_wv2_core_t* self, rvn_wv2_event_handler_t* eventHandler, rvn_wv2_token_t* token);
	HRESULT (STDMETHODCALLTYPE *remove_ProcessFailed)(rvn_wv2_core_t* self, rvn_wv2_token_t token);
	HRESULT (STDMETHODCALLTYPE *AddScriptToExecuteOnDocumentCreated)(rvn_wv2_core_t* self, LPCWSTR javaScript, rvn_wv2_string_done_t* handler);
	void* unused_RemoveScriptToExecuteOnDocumentCreated;
	HRESULT (STDMETHODCALLTYPE *ExecuteScript)(rvn_wv2_core_t* self, LPCWSTR javaScript, rvn_wv2_string_done_t* handler);
	void* unused_CapturePreview;
	HRESULT (STDMETHODCALLTYPE *Reload)(rvn_wv2_core_t* self);
	void* unused_PostWebMessageAsJson;
	void* unused_PostWebMessageAsString;
	HRESULT (STDMETHODCALLTYPE *add_WebMessageReceived)(rvn_wv2_core_t* self, rvn_wv2_event_handler_t* handler, rvn_wv2_token_t* token);
	HRESULT (STDMETHODCALLTYPE *remove_WebMessageReceived)(rvn_wv2_core_t* self, rvn_wv2_token_t token);
	void* unused_CallDevToolsProtocolMethod;
	void* unused_get_BrowserProcessId;
	HRESULT (STDMETHODCALLTYPE *get_CanGoBack)(rvn_wv2_core_t* self, BOOL* canGoBack);
	HRESULT (STDMETHODCALLTYPE *get_CanGoForward)(rvn_wv2_core_t* self, BOOL* canGoForward);
	HRESULT (STDMETHODCALLTYPE *GoBack)(rvn_wv2_core_t* self);
	HRESULT (STDMETHODCALLTYPE *GoForward)(rvn_wv2_core_t* self);
	void* unused_GetDevToolsProtocolEventReceiver;
	HRESULT (STDMETHODCALLTYPE *Stop)(rvn_wv2_core_t* self);
	HRESULT (STDMETHODCALLTYPE *add_NewWindowRequested)(rvn_wv2_core_t* self, rvn_wv2_event_handler_t* eventHandler, rvn_wv2_token_t* token);
	HRESULT (STDMETHODCALLTYPE *remove_NewWindowRequested)(rvn_wv2_core_t* self, rvn_wv2_token_t token);
	HRESULT (STDMETHODCALLTYPE *add_DocumentTitleChanged)(rvn_wv2_core_t* self, rvn_wv2_event_handler_t* eventHandler, rvn_wv2_token_t* token);
	HRESULT (STDMETHODCALLTYPE *remove_DocumentTitleChanged)(rvn_wv2_core_t* self, rvn_wv2_token_t token);
	HRESULT (STDMETHODCALLTYPE *get_DocumentTitle)(rvn_wv2_core_t* self, LPWSTR* title);
	void* unused_AddHostObjectToScript;
	void* unused_RemoveHostObjectFromScript;
	void* unused_OpenDevToolsWindow;
	void* unused_add_ContainsFullScreenElementChanged;
	void* unused_remove_ContainsFullScreenElementChanged;
	void* unused_get_ContainsFullScreenElement;
	HRESULT (STDMETHODCALLTYPE *add_WebResourceRequested)(rvn_wv2_core_t* self, rvn_wv2_event_handler_t* eventHandler, rvn_wv2_token_t* token);
	HRESULT (STDMETHODCALLTYPE *remove_WebResourceRequested)(rvn_wv2_core_t* self, rvn_wv2_token_t token);
	HRESULT (STDMETHODCALLTYPE *AddWebResourceRequestedFilter)(rvn_wv2_core_t* self, LPCWSTR uri, int resourceContext);
	HRESULT (STDMETHODCALLTYPE *RemoveWebResourceRequestedFilter)(rvn_wv2_core_t* self, LPCWSTR uri, int resourceContext);
} rvn_wv2_core_vtbl_t;
struct rvn_wv2_core_t { const rvn_wv2_core_vtbl_t* lpVtbl; };

/* ICoreWebView2NavigationStartingEventArgs {5B495469-E119-438A-9B18-7604F25F2E49} */
typedef struct rvn_wv2_navigation_starting_args_vtbl_t {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(rvn_wv2_navigation_starting_args_t* self, REFIID riid, void** object);
	ULONG (STDMETHODCALLTYPE *AddRef)(rvn_wv2_navigation_starting_args_t* self);
	ULONG (STDMETHODCALLTYPE *Release)(rvn_wv2_navigation_starting_args_t* self);
	HRESULT (STDMETHODCALLTYPE *get_Uri)(rvn_wv2_navigation_starting_args_t* self, LPWSTR* uri);
} rvn_wv2_navigation_starting_args_vtbl_t;
struct rvn_wv2_navigation_starting_args_t { const rvn_wv2_navigation_starting_args_vtbl_t* lpVtbl; };

/* ICoreWebView2NavigationCompletedEventArgs {30D68B7D-20D9-4752-A9CA-EC8448FBB5C1} */
typedef struct rvn_wv2_navigation_completed_args_vtbl_t {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(rvn_wv2_navigation_completed_args_t* self, REFIID riid, void** object);
	ULONG (STDMETHODCALLTYPE *AddRef)(rvn_wv2_navigation_completed_args_t* self);
	ULONG (STDMETHODCALLTYPE *Release)(rvn_wv2_navigation_completed_args_t* self);
	HRESULT (STDMETHODCALLTYPE *get_IsSuccess)(rvn_wv2_navigation_completed_args_t* self, BOOL* isSuccess);
	HRESULT (STDMETHODCALLTYPE *get_WebErrorStatus)(rvn_wv2_navigation_completed_args_t* self, int* webErrorStatus);
} rvn_wv2_navigation_completed_args_vtbl_t;
struct rvn_wv2_navigation_completed_args_t { const rvn_wv2_navigation_completed_args_vtbl_t* lpVtbl; };

/* ICoreWebView2WebMessageReceivedEventArgs {0F99A40C-E962-4207-9E92-E3D542EFF849} */
typedef struct rvn_wv2_message_args_vtbl_t {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(rvn_wv2_message_args_t* self, REFIID riid, void** object);
	ULONG (STDMETHODCALLTYPE *AddRef)(rvn_wv2_message_args_t* self);
	ULONG (STDMETHODCALLTYPE *Release)(rvn_wv2_message_args_t* self);
	void* unused_get_Source;
	HRESULT (STDMETHODCALLTYPE *get_WebMessageAsJson)(rvn_wv2_message_args_t* self, LPWSTR* value);
	HRESULT (STDMETHODCALLTYPE *TryGetWebMessageAsString)(rvn_wv2_message_args_t* self, LPWSTR* value);
} rvn_wv2_message_args_vtbl_t;
struct rvn_wv2_message_args_t { const rvn_wv2_message_args_vtbl_t* lpVtbl; };

/* ICoreWebView2NewWindowRequestedEventArgs {34ACB11C-FC37-4418-9132-F9C21D1EAFB9} */
typedef struct rvn_wv2_new_window_args_vtbl_t {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(rvn_wv2_new_window_args_t* self, REFIID riid, void** object);
	ULONG (STDMETHODCALLTYPE *AddRef)(rvn_wv2_new_window_args_t* self);
	ULONG (STDMETHODCALLTYPE *Release)(rvn_wv2_new_window_args_t* self);
	HRESULT (STDMETHODCALLTYPE *get_Uri)(rvn_wv2_new_window_args_t* self, LPWSTR* uri);
	void* unused_put_NewWindow;
	void* unused_get_NewWindow;
	HRESULT (STDMETHODCALLTYPE *put_Handled)(rvn_wv2_new_window_args_t* self, BOOL handled);
} rvn_wv2_new_window_args_vtbl_t;
struct rvn_wv2_new_window_args_t { const rvn_wv2_new_window_args_vtbl_t* lpVtbl; };

/* ICoreWebView2WebResourceRequestedEventArgs {453E667F-12C7-49D4-BE6D-DDBE7956F57A} */
typedef struct rvn_wv2_resource_args_vtbl_t {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(rvn_wv2_resource_args_t* self, REFIID riid, void** object);
	ULONG (STDMETHODCALLTYPE *AddRef)(rvn_wv2_resource_args_t* self);
	ULONG (STDMETHODCALLTYPE *Release)(rvn_wv2_resource_args_t* self);
	HRESULT (STDMETHODCALLTYPE *get_Request)(rvn_wv2_resource_args_t* self, rvn_wv2_resource_request_t** request);
	void* unused_get_Response;
	HRESULT (STDMETHODCALLTYPE *put_Response)(rvn_wv2_resource_args_t* self, rvn_wv2_resource_response_t* response);
} rvn_wv2_resource_args_vtbl_t;
struct rvn_wv2_resource_args_t { const rvn_wv2_resource_args_vtbl_t* lpVtbl; };

/* ICoreWebView2WebResourceRequest {97055CD4-512C-4264-8B5F-E3F446CEA6A5} */
typedef struct rvn_wv2_resource_request_vtbl_t {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(rvn_wv2_resource_request_t* self, REFIID riid, void** object);
	ULONG (STDMETHODCALLTYPE *AddRef)(rvn_wv2_resource_request_t* self);
	ULONG (STDMETHODCALLTYPE *Release)(rvn_wv2_resource_request_t* self);
	HRESULT (STDMETHODCALLTYPE *get_Uri)(rvn_wv2_resource_request_t* self, LPWSTR* uri);
} rvn_wv2_resource_request_vtbl_t;
struct rvn_wv2_resource_request_t { const rvn_wv2_resource_request_vtbl_t* lpVtbl; };

/* ICoreWebView2ProcessFailedEventArgs {8155A9A4-1474-4A86-8CAE-151B0FA6B8CA} */
typedef struct rvn_wv2_process_failed_args_vtbl_t {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(rvn_wv2_process_failed_args_t* self, REFIID riid, void** object);
	ULONG (STDMETHODCALLTYPE *AddRef)(rvn_wv2_process_failed_args_t* self);
	ULONG (STDMETHODCALLTYPE *Release)(rvn_wv2_process_failed_args_t* self);
	HRESULT (STDMETHODCALLTYPE *get_ProcessFailedKind)(rvn_wv2_process_failed_args_t* self, int* value);
} rvn_wv2_process_failed_args_vtbl_t;
struct rvn_wv2_process_failed_args_t { const rvn_wv2_process_failed_args_vtbl_t* lpVtbl; };

/* The identifiers of the handlers Raven implements (QueryInterface answers them). */
static const IID rvn_wv2_iid_environment_done = { 0x4e8a3389, 0xc9d8, 0x4bd2, { 0xb6, 0xb5, 0x12, 0x4f, 0xee, 0x6c, 0xc1, 0x4d } };
static const IID rvn_wv2_iid_controller_done = { 0x6c4819f3, 0xc9b7, 0x4260, { 0x81, 0x27, 0xc9, 0xf5, 0xbd, 0xe7, 0xf6, 0x8c } };
static const IID rvn_wv2_iid_add_script_done = { 0xb99369f3, 0x9b11, 0x47b5, { 0xbc, 0x6f, 0x8e, 0x78, 0x95, 0xfc, 0xea, 0x17 } };
static const IID rvn_wv2_iid_execute_script_done = { 0x49511172, 0xcc67, 0x4bca, { 0x99, 0x23, 0x13, 0x71, 0x12, 0xf4, 0xc4, 0xcc } };
static const IID rvn_wv2_iid_navigation_starting = { 0x9adbe429, 0xf36d, 0x432b, { 0x9d, 0xdc, 0xf8, 0x88, 0x1f, 0xbd, 0x76, 0xe3 } };
static const IID rvn_wv2_iid_navigation_completed = { 0xd33a35bf, 0x1c49, 0x4f98, { 0x93, 0xab, 0x00, 0x6e, 0x05, 0x33, 0xfe, 0x1c } };
static const IID rvn_wv2_iid_source_changed = { 0x3c067f9f, 0x5388, 0x4772, { 0x8b, 0x48, 0x79, 0xf7, 0xef, 0x1a, 0xb3, 0x7c } };
static const IID rvn_wv2_iid_title_changed = { 0xf5f2b923, 0x953e, 0x4042, { 0x9f, 0x95, 0xf3, 0xa1, 0x18, 0xe1, 0xaf, 0xd4 } };
static const IID rvn_wv2_iid_message_received = { 0x57213f19, 0x00e6, 0x49fa, { 0x8e, 0x07, 0x89, 0x8e, 0xa0, 0x1e, 0xcb, 0xd2 } };
static const IID rvn_wv2_iid_new_window = { 0xd4c185fe, 0xc81c, 0x4989, { 0x97, 0xaf, 0x2d, 0x3f, 0xa7, 0xab, 0x56, 0x51 } };
static const IID rvn_wv2_iid_process_failed = { 0x79e0aea4, 0x990b, 0x42d9, { 0xaa, 0x1d, 0x0f, 0xcc, 0x2e, 0x5b, 0xc7, 0xf1 } };
static const IID rvn_wv2_iid_resource_requested = { 0xab00b74c, 0x15f1, 0x4646, { 0x80, 0xe8, 0xe7, 0x63, 0x41, 0xd2, 0x5d, 0x71 } };

#endif /* RVN_WEBVIEW2_COM_H */
