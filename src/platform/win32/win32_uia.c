/**
 * Raven - platform/win32/win32_uia.c
 * The windows of Raven for the screen readers of Windows (Narrator, NVDA,
 * JAWS): a UI Automation provider, in C.
 *
 * Each node of core/accessibility.h is a provider object made on first use
 * and kept in a table: IRawElementProviderSimple and ...Fragment for all,
 * ...FragmentRoot for the windows, and the patterns that apply: Invoke
 * (buttons, menu items, tabs), Toggle (check boxes), Value (text fields),
 * RangeValue (sliders, spin boxes, progress bars), ExpandCollapse (expanders,
 * combo boxes, tree items), SelectionItem (rows, tabs).
 *
 * The window answers WM_GETOBJECT with the provider of its root
 * (win32_platform.c). UI Automation is called in the apartment of the UI
 * thread (CoInitializeEx STA, ProviderOptions_UseComThreading); a call that
 * still arrives on another thread is sent to the UI thread with
 * SendMessage, so the library is only ever touched there.
 *
 * UIAutomationCore.dll and OleAut32.dll are loaded when first needed: Raven
 * links with nothing more.
 */

#if defined(_WIN32)

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ole2.h>
#include <oleauto.h>
#include <uiautomation.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../platform.h"
#include "../../core/accessibility.h"
#include "win32_uia.h"

#ifndef UIA_E_ELEMENTNOTAVAILABLE
#define UIA_E_ELEMENTNOTAVAILABLE ((HRESULT)0x80040201L)
#endif

/* ---------------------------------------------------------------------------
 *  Functions loaded at run time
 * ------------------------------------------------------------------------- */

typedef HRESULT (WINAPI *uia_host_fn)(HWND, IRawElementProviderSimple**);
typedef LRESULT (WINAPI *uia_return_fn)(HWND, WPARAM, LPARAM, IRawElementProviderSimple*);
typedef HRESULT (WINAPI *uia_event_fn)(IRawElementProviderSimple*, EVENTID);
typedef HRESULT (WINAPI *uia_property_fn)(IRawElementProviderSimple*, PROPERTYID, VARIANT, VARIANT);
typedef HRESULT (WINAPI *uia_structure_fn)(IRawElementProviderSimple*, enum StructureChangeType, int*, int);
typedef HRESULT (WINAPI *uia_notification_fn)(IRawElementProviderSimple*, int, int, BSTR, BSTR);
typedef BOOL (WINAPI *uia_listening_fn)(void);
typedef HRESULT (WINAPI *uia_disconnect_fn)(IRawElementProviderSimple*);
typedef BSTR (WINAPI *sys_alloc_fn)(const OLECHAR*);
typedef void (WINAPI *sys_free_fn)(BSTR);
typedef SAFEARRAY* (WINAPI *sa_create_fn)(VARTYPE, LONG, ULONG);
typedef HRESULT (WINAPI *sa_put_fn)(SAFEARRAY*, LONG*, void*);
typedef HRESULT (WINAPI *sa_destroy_fn)(SAFEARRAY*);
typedef HRESULT (WINAPI *co_init_fn)(LPVOID, DWORD);
typedef LSTATUS (WINAPI *reg_get_fn)(HKEY, LPCWSTR, LPCWSTR, DWORD, LPDWORD, PVOID, LPDWORD);

#define RVN_WM_UIA (WM_APP + 0x5A1)

typedef struct rvn_uia_t rvn_uia_t;

static struct {
	int loaded, failed;
	uia_host_fn host;
	uia_return_fn return_provider;
	uia_event_fn raise_event;
	uia_property_fn raise_property;
	uia_structure_fn raise_structure;
	uia_notification_fn raise_notification;
	uia_listening_fn listening;
	uia_disconnect_fn disconnect;
	sys_alloc_fn sys_alloc;
	sys_free_fn sys_free;
	sa_create_fn sa_create;
	sa_put_fn sa_put;
	sa_destroy_fn sa_destroy;
	DWORD ui_thread;
	/* providers made so far, by node */
	rvn_uia_t** table;
	int count, capacity;
} g_uia;

static int rvn_uia_load(void) {
	HMODULE core, oleaut, ole;
	if (g_uia.loaded) return R_TRUE;
	if (g_uia.failed) return R_FALSE;
	core = LoadLibraryW(L"UIAutomationCore.dll");
	oleaut = LoadLibraryW(L"OleAut32.dll");
	ole = LoadLibraryW(L"Ole32.dll");
	if (!core || !oleaut) {
		g_uia.failed = R_TRUE;
		return R_FALSE;
	}
	g_uia.host = (uia_host_fn)(void*)GetProcAddress(core, "UiaHostProviderFromHwnd");
	g_uia.return_provider = (uia_return_fn)(void*)GetProcAddress(core, "UiaReturnRawElementProvider");
	g_uia.raise_event = (uia_event_fn)(void*)GetProcAddress(core, "UiaRaiseAutomationEvent");
	g_uia.raise_property = (uia_property_fn)(void*)GetProcAddress(core, "UiaRaiseAutomationPropertyChangedEvent");
	g_uia.raise_structure = (uia_structure_fn)(void*)GetProcAddress(core, "UiaRaiseStructureChangedEvent");
	/* (Windows 10 1709 and later) */
	g_uia.raise_notification = (uia_notification_fn)(void*)GetProcAddress(core, "UiaRaiseNotificationEvent");
	g_uia.listening = (uia_listening_fn)(void*)GetProcAddress(core, "UiaClientsAreListening");
	g_uia.disconnect = (uia_disconnect_fn)(void*)GetProcAddress(core, "UiaDisconnectProvider");
	g_uia.sys_alloc = (sys_alloc_fn)(void*)GetProcAddress(oleaut, "SysAllocString");
	g_uia.sys_free = (sys_free_fn)(void*)GetProcAddress(oleaut, "SysFreeString");
	g_uia.sa_create = (sa_create_fn)(void*)GetProcAddress(oleaut, "SafeArrayCreateVector");
	g_uia.sa_put = (sa_put_fn)(void*)GetProcAddress(oleaut, "SafeArrayPutElement");
	g_uia.sa_destroy = (sa_destroy_fn)(void*)GetProcAddress(oleaut, "SafeArrayDestroy");
	if (!g_uia.host || !g_uia.return_provider || !g_uia.raise_event || !g_uia.sys_alloc || !g_uia.sa_create || !g_uia.sa_put || !g_uia.sa_destroy) {
		g_uia.failed = R_TRUE;
		return R_FALSE;
	}
	/* the UI thread is a single-threaded apartment: UI Automation calls the providers there */
	if (ole) {
		co_init_fn init = (co_init_fn)(void*)GetProcAddress(ole, "CoInitializeEx");
		if (init) init(NULL, COINIT_APARTMENTTHREADED);
	}
	g_uia.ui_thread = GetCurrentThreadId();
	g_uia.loaded = R_TRUE;
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  The provider object
 * ------------------------------------------------------------------------- */

struct rvn_uia_t {
	IRawElementProviderSimple simple;
	IRawElementProviderFragment fragment;
	IRawElementProviderFragmentRoot root;
	IInvokeProvider invoke;
	IToggleProvider toggle;
	IValueProvider value;
	IRangeValueProvider range;
	IExpandCollapseProvider expand;
	ISelectionItemProvider selection;
	ISelectionProvider selection_group;
	LONG refs;
	rvn_a11y_id_t id;
	HWND hwnd;
	int disconnected;
};

#define FROM(pointer, member) ((rvn_uia_t*)((char*)(pointer) - offsetof(rvn_uia_t, member)))

static rvn_uia_t* rvn_uia_provider(HWND hwnd, rvn_a11y_id_t id);
static HWND rvn_uia_hwnd_of(rvn_a11y_id_t id);

static int rvn_uia_alive(rvn_uia_t* self) {
	return !self->disconnected && rvn_a11y_exists(self->id);
}

/* ---- calls from other threads: done on the UI thread ---- */

typedef struct rvn_uia_call_t rvn_uia_call_t;
struct rvn_uia_call_t {
	HRESULT (*fn)(rvn_uia_call_t*);
	rvn_uia_t* self;
	intptr_t a, b;
	double d;
	const wchar_t* text;
	void* out;
	HRESULT result;
};

static HRESULT rvn_uia_call(rvn_uia_call_t* call) {
	if (GetCurrentThreadId() == g_uia.ui_thread) return call->fn(call);
	call->result = UIA_E_ELEMENTNOTAVAILABLE;
	if (call->self->hwnd) SendMessageW(call->self->hwnd, RVN_WM_UIA, 0, (LPARAM)call);
	return call->result;
}

int rvn_uia_handle_message(UINT message, WPARAM wparam, LPARAM lparam) {
	(void)wparam;
	if (message != RVN_WM_UIA) return R_FALSE;
	{
		rvn_uia_call_t* call = (rvn_uia_call_t*)lparam;
		call->result = call->fn(call);
	}
	return R_TRUE;
}

#define CALL(name, iface, member, ...) \
	rvn_uia_call_t call; \
	memset(&call, 0, sizeof(call)); \
	call.fn = name; \
	call.self = FROM(iface, member); \
	__VA_ARGS__; \
	return rvn_uia_call(&call)

/* ---- strings ---- */

static BSTR rvn_uia_bstr(const char* utf8) {
	int n;
	wchar_t* wide;
	BSTR result;
	if (!utf8) utf8 = "";
	n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	wide = (wchar_t*)rvn_alloc((size_t)(n > 0 ? n : 1) * sizeof(wchar_t));
	wide[0] = 0;
	if (n > 0) MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, n);
	result = g_uia.sys_alloc(wide);
	rvn_free(wide);
	return result;
}

static char* rvn_uia_utf8(const wchar_t* wide) {
	int n = WideCharToMultiByte(CP_UTF8, 0, wide ? wide : L"", -1, NULL, 0, NULL, NULL);
	char* out = (char*)rvn_alloc((size_t)(n > 0 ? n : 1));
	out[0] = 0;
	if (n > 0) WideCharToMultiByte(CP_UTF8, 0, wide ? wide : L"", -1, out, n, NULL, NULL);
	return out;
}

/* ---- IUnknown, shared by every interface of the object ---- */

static ULONG rvn_uia_addref(rvn_uia_t* self) { return (ULONG)InterlockedIncrement(&self->refs); }

static ULONG rvn_uia_release(rvn_uia_t* self) {
	LONG refs = InterlockedDecrement(&self->refs);
	if (refs == 0) rvn_free(self);
	return (ULONG)refs;
}

static HRESULT rvn_uia_query(rvn_uia_t* self, REFIID riid, void** out) {
	*out = NULL;
	if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IRawElementProviderSimple)) *out = &self->simple;
	else if (IsEqualIID(riid, &IID_IRawElementProviderFragment)) *out = &self->fragment;
	else if (IsEqualIID(riid, &IID_IRawElementProviderFragmentRoot) && !(self->id >> 32) && rvn_a11y_parent(self->id) == 0) *out = &self->root;
	else if (IsEqualIID(riid, &IID_IInvokeProvider)) *out = &self->invoke;
	else if (IsEqualIID(riid, &IID_IToggleProvider)) *out = &self->toggle;
	else if (IsEqualIID(riid, &IID_IValueProvider)) *out = &self->value;
	else if (IsEqualIID(riid, &IID_IRangeValueProvider)) *out = &self->range;
	else if (IsEqualIID(riid, &IID_IExpandCollapseProvider)) *out = &self->expand;
	else if (IsEqualIID(riid, &IID_ISelectionItemProvider)) *out = &self->selection;
	else if (IsEqualIID(riid, &IID_ISelectionProvider)) *out = &self->selection_group;
	else return E_NOINTERFACE;
	rvn_uia_addref(self);
	return S_OK;
}

#define UNKNOWN(prefix, type, member) \
	static HRESULT STDMETHODCALLTYPE prefix##_query(type* This, REFIID riid, void** out) { return rvn_uia_query(FROM(This, member), riid, out); } \
	static ULONG STDMETHODCALLTYPE prefix##_addref(type* This) { return rvn_uia_addref(FROM(This, member)); } \
	static ULONG STDMETHODCALLTYPE prefix##_release(type* This) { return rvn_uia_release(FROM(This, member)); }

UNKNOWN(simple, IRawElementProviderSimple, simple)
UNKNOWN(fragment, IRawElementProviderFragment, fragment)
UNKNOWN(root, IRawElementProviderFragmentRoot, root)
UNKNOWN(invoke, IInvokeProvider, invoke)
UNKNOWN(toggle, IToggleProvider, toggle)
UNKNOWN(value, IValueProvider, value)
UNKNOWN(range, IRangeValueProvider, range)
UNKNOWN(expand, IExpandCollapseProvider, expand)
UNKNOWN(selection, ISelectionItemProvider, selection)
UNKNOWN(selection_group, ISelectionProvider, selection_group)

/* ---------------------------------------------------------------------------
 *  IRawElementProviderSimple
 * ------------------------------------------------------------------------- */

static long rvn_uia_control_type(const rvn_a11y_node_t* node) {
	if (node->states & RVN_A11Y_TABLE_ROW) return UIA_DataItemControlTypeId;
	switch (node->role) {
		case R_ROLE_WINDOW: return UIA_WindowControlTypeId;
		case R_ROLE_GROUP: return UIA_GroupControlTypeId;
		case R_ROLE_TEXT: case R_ROLE_HEADING: return UIA_TextControlTypeId;
		case R_ROLE_LINK: return UIA_HyperlinkControlTypeId;
		case R_ROLE_BUTTON: case R_ROLE_TOGGLE_BUTTON: return UIA_ButtonControlTypeId;
		case R_ROLE_CHECKBOX: return UIA_CheckBoxControlTypeId;
		case R_ROLE_RADIO_BUTTON: return UIA_RadioButtonControlTypeId;
		case R_ROLE_SLIDER: return UIA_SliderControlTypeId;
		case R_ROLE_PROGRESS_BAR: return UIA_ProgressBarControlTypeId;
		case R_ROLE_SPIN_BUTTON: return UIA_SpinnerControlTypeId;
		case R_ROLE_TEXT_FIELD: return UIA_EditControlTypeId;
		case R_ROLE_DOCUMENT: return UIA_DocumentControlTypeId;
		case R_ROLE_COMBO_BOX: return UIA_ComboBoxControlTypeId;
		case R_ROLE_LIST: return UIA_ListControlTypeId;
		case R_ROLE_LIST_ITEM: return UIA_ListItemControlTypeId;
		case R_ROLE_TREE: return UIA_TreeControlTypeId;
		case R_ROLE_TREE_ITEM: return UIA_TreeItemControlTypeId;
		case R_ROLE_TABLE: return UIA_TableControlTypeId;
		case R_ROLE_MENU_BAR: return UIA_MenuBarControlTypeId;
		case R_ROLE_MENU: return UIA_MenuControlTypeId;
		case R_ROLE_MENU_ITEM: return UIA_MenuItemControlTypeId;
		case R_ROLE_TAB_LIST: return UIA_TabControlTypeId;
		case R_ROLE_TAB: return UIA_TabItemControlTypeId;
		case R_ROLE_TAB_PANEL: return UIA_PaneControlTypeId;
		case R_ROLE_TOOLBAR: return UIA_ToolBarControlTypeId;
		case R_ROLE_STATUS_BAR: return UIA_StatusBarControlTypeId;
		case R_ROLE_DIALOG: return UIA_WindowControlTypeId;
		case R_ROLE_ALERT: return UIA_TextControlTypeId;
		case R_ROLE_IMAGE: return UIA_ImageControlTypeId;
		case R_ROLE_SEPARATOR: return UIA_SeparatorControlTypeId;
		case R_ROLE_SCROLL_BAR: return UIA_ScrollBarControlTypeId;
		case R_ROLE_CANVAS: case R_ROLE_PANE: return UIA_PaneControlTypeId;
		default: return UIA_CustomControlTypeId;
	}
}

static HRESULT STDMETHODCALLTYPE simple_options(IRawElementProviderSimple* This, enum ProviderOptions* out) {
	(void)This;
	*out = (enum ProviderOptions)(ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading);
	return S_OK;
}

static HRESULT do_pattern(rvn_uia_call_t* c) {
	rvn_a11y_node_t node;
	IUnknown** out = (IUnknown**)c->out;
	PATTERNID pattern = (PATTERNID)c->a;
	void* found = NULL;
	*out = NULL;
	if (!rvn_uia_alive(c->self) || !rvn_a11y_describe(c->self->id, &node)) return UIA_E_ELEMENTNOTAVAILABLE;
	if (pattern == UIA_InvokePatternId && (node.states & RVN_A11Y_INVOKABLE) && !(node.states & RVN_A11Y_CHECKABLE) &&
		node.role != R_ROLE_COMBO_BOX && node.role != R_ROLE_LIST_ITEM && node.role != R_ROLE_TAB) found = &c->self->invoke;
	else if (pattern == UIA_TogglePatternId && (node.states & RVN_A11Y_CHECKABLE)) found = &c->self->toggle;
	else if (pattern == UIA_ValuePatternId && ((node.states & RVN_A11Y_EDITABLE) || node.role == R_ROLE_COMBO_BOX)) found = &c->self->value;
	else if (pattern == UIA_RangeValuePatternId && (node.states & RVN_A11Y_HAS_RANGE)) found = &c->self->range;
	else if (pattern == UIA_ExpandCollapsePatternId && (node.states & RVN_A11Y_EXPANDABLE)) found = &c->self->expand;
	else if (pattern == UIA_SelectionItemPatternId && (node.states & RVN_A11Y_SELECTABLE)) found = &c->self->selection;
	else if (pattern == UIA_SelectionPatternId && node.role == R_ROLE_TABLE) found = &c->self->selection_group;
	if (found) {
		rvn_uia_addref(c->self);
		*out = (IUnknown*)found;
	}
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE simple_pattern(IRawElementProviderSimple* This, PATTERNID pattern, IUnknown** out) {
	CALL(do_pattern, This, simple, call.a = pattern; call.out = out);
}

/* the property ids of UI Automation (UIAutomationClient.h): MSVC declares them as
   const variables in C, which a case label cannot use */
#define RVN_UIA_ControlType 30003
#define RVN_UIA_LocalizedControlType 30004
#define RVN_UIA_Name 30005
#define RVN_UIA_HasKeyboardFocus 30008
#define RVN_UIA_IsKeyboardFocusable 30009
#define RVN_UIA_IsEnabled 30010
#define RVN_UIA_AutomationId 30011
#define RVN_UIA_ClassName 30012
#define RVN_UIA_HelpText 30013
#define RVN_UIA_Culture 30015
#define RVN_UIA_IsControlElement 30016
#define RVN_UIA_IsContentElement 30017
#define RVN_UIA_IsPassword 30019
#define RVN_UIA_IsOffscreen 30022
#define RVN_UIA_FrameworkId 30024
#define RVN_UIA_ProviderDescription 30107
#define RVN_UIA_IsDialog 30174

static HRESULT do_property(rvn_uia_call_t* c) {
	rvn_a11y_node_t node;
	VARIANT* out = (VARIANT*)c->out;
	PROPERTYID property = (PROPERTYID)c->a;
	memset(out, 0, sizeof(*out));
	out->vt = VT_EMPTY;
	if (!rvn_uia_alive(c->self) || !rvn_a11y_describe(c->self->id, &node)) return UIA_E_ELEMENTNOTAVAILABLE;
	switch (property) {
		case RVN_UIA_ControlType: out->vt = VT_I4; out->lVal = rvn_uia_control_type(&node); break;
		case RVN_UIA_Name: out->vt = VT_BSTR; out->bstrVal = rvn_uia_bstr(node.name); break;
		case RVN_UIA_HelpText:
			if (node.description[0]) { out->vt = VT_BSTR; out->bstrVal = rvn_uia_bstr(node.description); }
			break;
		case RVN_UIA_LocalizedControlType:
			if (node.role == R_ROLE_CANVAS || node.role == R_ROLE_TAB_PANEL) { out->vt = VT_BSTR; out->bstrVal = rvn_uia_bstr(rvn_a11y_role_name(node.role)); }
			break;
		case RVN_UIA_AutomationId: {
			char text[32];
			snprintf(text, sizeof(text), "raven-%llx", (unsigned long long)node.id);
			out->vt = VT_BSTR;
			out->bstrVal = rvn_uia_bstr(text);
			break;
		}
		case RVN_UIA_ClassName: out->vt = VT_BSTR; out->bstrVal = rvn_uia_bstr("Raven"); break;
		case RVN_UIA_FrameworkId: out->vt = VT_BSTR; out->bstrVal = rvn_uia_bstr("Raven"); break;
		case RVN_UIA_IsKeyboardFocusable: out->vt = VT_BOOL; out->boolVal = (node.states & RVN_A11Y_FOCUSABLE) ? VARIANT_TRUE : VARIANT_FALSE; break;
		case RVN_UIA_HasKeyboardFocus: out->vt = VT_BOOL; out->boolVal = (node.states & RVN_A11Y_FOCUSED) ? VARIANT_TRUE : VARIANT_FALSE; break;
		case RVN_UIA_IsEnabled: out->vt = VT_BOOL; out->boolVal = (node.states & RVN_A11Y_DISABLED) ? VARIANT_FALSE : VARIANT_TRUE; break;
		case RVN_UIA_IsControlElement:
		case RVN_UIA_IsContentElement: out->vt = VT_BOOL; out->boolVal = VARIANT_TRUE; break;
		case RVN_UIA_IsPassword: out->vt = VT_BOOL; out->boolVal = (node.states & RVN_A11Y_PASSWORD) ? VARIANT_TRUE : VARIANT_FALSE; break;
		case RVN_UIA_IsOffscreen: out->vt = VT_BOOL; out->boolVal = VARIANT_FALSE; break;
		case RVN_UIA_IsDialog: out->vt = VT_BOOL; out->boolVal = (node.states & RVN_A11Y_MODAL) ? VARIANT_TRUE : VARIANT_FALSE; break;
		case RVN_UIA_ProviderDescription: out->vt = VT_BSTR; out->bstrVal = rvn_uia_bstr("Raven UI Automation provider"); break;
		case RVN_UIA_Culture: {
			/* the language of the application */
			wchar_t wide[LOCALE_NAME_MAX_LENGTH];
			if (MultiByteToWideChar(CP_UTF8, 0, rvn_a11y_locale(), -1, wide, LOCALE_NAME_MAX_LENGTH) > 0) {
				out->vt = VT_I4;
				out->lVal = (LONG)LocaleNameToLCID(wide, 0);
			}
			break;
		}
		default: break;
	}
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE simple_property(IRawElementProviderSimple* This, PROPERTYID property, VARIANT* out) {
	CALL(do_property, This, simple, call.a = property; call.out = out);
}

static HRESULT STDMETHODCALLTYPE simple_host(IRawElementProviderSimple* This, IRawElementProviderSimple** out) {
	rvn_uia_t* self = FROM(This, simple);
	*out = NULL;
	/* the window: UI Automation adds what it knows of the HWND (title bar, process...) */
	if (!(self->id >> 32) && rvn_a11y_parent(self->id) == 0 && self->hwnd) return g_uia.host(self->hwnd, out);
	return S_OK;
}

/* ---------------------------------------------------------------------------
 *  IRawElementProviderFragment
 * ------------------------------------------------------------------------- */

static HRESULT do_navigate(rvn_uia_call_t* c) {
	IRawElementProviderFragment** out = (IRawElementProviderFragment**)c->out;
	rvn_a11y_id_t id = c->self->id, target = 0, parent;
	int index, count;
	*out = NULL;
	if (!rvn_uia_alive(c->self)) return UIA_E_ELEMENTNOTAVAILABLE;
	switch ((enum NavigateDirection)c->a) {
		case NavigateDirection_Parent:
			target = rvn_a11y_parent(id);
			break;
		case NavigateDirection_FirstChild:
			if (rvn_a11y_child_count(id) > 0) target = rvn_a11y_child(id, 0);
			break;
		case NavigateDirection_LastChild:
			count = rvn_a11y_child_count(id);
			if (count > 0) target = rvn_a11y_child(id, count - 1);
			break;
		case NavigateDirection_NextSibling:
		case NavigateDirection_PreviousSibling:
			parent = rvn_a11y_parent(id);
			if (!parent) break;
			index = rvn_a11y_index_in_parent(id);
			count = rvn_a11y_child_count(parent);
			index += (enum NavigateDirection)c->a == NavigateDirection_NextSibling ? 1 : -1;
			if (index >= 0 && index < count) target = rvn_a11y_child(parent, index);
			break;
	}
	if (target) {
		rvn_uia_t* provider = rvn_uia_provider(c->self->hwnd, target);
		if (provider) {
			rvn_uia_addref(provider);
			*out = &provider->fragment;
		}
	}
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE fragment_navigate(IRawElementProviderFragment* This, enum NavigateDirection direction, IRawElementProviderFragment** out) {
	CALL(do_navigate, This, fragment, call.a = direction; call.out = out);
}

static HRESULT STDMETHODCALLTYPE fragment_runtime_id(IRawElementProviderFragment* This, SAFEARRAY** out) {
	rvn_uia_t* self = FROM(This, fragment);
	LONG i;
	int values[3];
	*out = NULL;
	/* (the window takes the runtime id of its HWND) */
	if (!(self->id >> 32) && rvn_a11y_parent(self->id) == 0) return S_OK;
	values[0] = UiaAppendRuntimeId;
	values[1] = (int)(uint32_t)(self->id & 0xFFFFFFFFu);
	values[2] = (int)(uint32_t)(self->id >> 32);
	*out = g_uia.sa_create(VT_I4, 0, 3);
	if (!*out) return E_OUTOFMEMORY;
	for (i = 0; i < 3; i++) g_uia.sa_put(*out, &i, &values[i]);
	return S_OK;
}

static HRESULT do_bounds(rvn_uia_call_t* c) {
	struct UiaRect* out = (struct UiaRect*)c->out;
	int x, y, w, h;
	memset(out, 0, sizeof(*out));
	if (!rvn_uia_alive(c->self)) return UIA_E_ELEMENTNOTAVAILABLE;
	rvn_a11y_screen_rect(c->self->id, &x, &y, &w, &h);
	out->left = x;
	out->top = y;
	out->width = w;
	out->height = h;
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE fragment_bounds(IRawElementProviderFragment* This, struct UiaRect* out) {
	CALL(do_bounds, This, fragment, call.out = out);
}

static HRESULT STDMETHODCALLTYPE fragment_embedded(IRawElementProviderFragment* This, SAFEARRAY** out) {
	(void)This;
	*out = NULL;
	return S_OK;
}

static HRESULT do_set_focus(rvn_uia_call_t* c) {
	if (!rvn_uia_alive(c->self)) return UIA_E_ELEMENTNOTAVAILABLE;
	rvn_a11y_act(c->self->id, RVN_A11Y_FOCUS);
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE fragment_set_focus(IRawElementProviderFragment* This) {
	CALL(do_set_focus, This, fragment, (void)0);
}

static HRESULT do_fragment_root(rvn_uia_call_t* c) {
	IRawElementProviderFragmentRoot** out = (IRawElementProviderFragmentRoot**)c->out;
	rvn_uia_t* root;
	*out = NULL;
	if (!rvn_uia_alive(c->self)) return UIA_E_ELEMENTNOTAVAILABLE;
	root = rvn_uia_provider(c->self->hwnd, (rvn_a11y_id_t)rvn_a11y_window(c->self->id));
	if (root) {
		rvn_uia_addref(root);
		*out = &root->root;
	}
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE fragment_root(IRawElementProviderFragment* This, IRawElementProviderFragmentRoot** out) {
	CALL(do_fragment_root, This, fragment, call.out = out);
}

/* ---------------------------------------------------------------------------
 *  IRawElementProviderFragmentRoot
 * ------------------------------------------------------------------------- */

static HRESULT do_from_point(rvn_uia_call_t* c) {
	IRawElementProviderFragment** out = (IRawElementProviderFragment**)c->out;
	int wx, wy;
	float scale;
	rvn_a11y_id_t hit;
	*out = NULL;
	if (!rvn_uia_alive(c->self)) return UIA_E_ELEMENTNOTAVAILABLE;
	rvn_a11y_window_origin(c->self->id, &wx, &wy, &scale);
	hit = rvn_a11y_hit(rvn_a11y_window(c->self->id), (float)((double)c->a - wx) / scale, (float)((double)c->b - wy) / scale);
	if (hit) {
		rvn_uia_t* provider = rvn_uia_provider(c->self->hwnd, hit);
		if (provider) {
			rvn_uia_addref(provider);
			*out = &provider->fragment;
		}
	}
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE root_from_point(IRawElementProviderFragmentRoot* This, double x, double y, IRawElementProviderFragment** out) {
	CALL(do_from_point, This, root, call.a = (intptr_t)x; call.b = (intptr_t)y; call.out = out);
}

static HRESULT do_get_focus(rvn_uia_call_t* c) {
	IRawElementProviderFragment** out = (IRawElementProviderFragment**)c->out;
	rvn_a11y_id_t focus;
	*out = NULL;
	if (!rvn_uia_alive(c->self)) return UIA_E_ELEMENTNOTAVAILABLE;
	focus = rvn_a11y_focus(rvn_a11y_window(c->self->id));
	if (focus && focus != c->self->id) {
		rvn_uia_t* provider = rvn_uia_provider(c->self->hwnd, focus);
		if (provider) {
			rvn_uia_addref(provider);
			*out = &provider->fragment;
		}
	}
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE root_get_focus(IRawElementProviderFragmentRoot* This, IRawElementProviderFragment** out) {
	CALL(do_get_focus, This, root, call.out = out);
}

/* ---------------------------------------------------------------------------
 *  Patterns
 * ------------------------------------------------------------------------- */

static HRESULT do_act(rvn_uia_call_t* c) {
	if (!rvn_uia_alive(c->self)) return UIA_E_ELEMENTNOTAVAILABLE;
	rvn_a11y_act(c->self->id, (rvn_a11y_action_t)c->a);
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE invoke_invoke(IInvokeProvider* This) { CALL(do_act, This, invoke, call.a = RVN_A11Y_PRESS); }
static HRESULT STDMETHODCALLTYPE toggle_toggle(IToggleProvider* This) { CALL(do_act, This, toggle, call.a = RVN_A11Y_TOGGLE); }
static HRESULT STDMETHODCALLTYPE expand_expand(IExpandCollapseProvider* This) { CALL(do_act, This, expand, call.a = RVN_A11Y_EXPAND); }
static HRESULT STDMETHODCALLTYPE expand_collapse(IExpandCollapseProvider* This) { CALL(do_act, This, expand, call.a = RVN_A11Y_COLLAPSE); }
static HRESULT STDMETHODCALLTYPE selection_select(ISelectionItemProvider* This) { CALL(do_act, This, selection, call.a = RVN_A11Y_SELECT); }
static HRESULT STDMETHODCALLTYPE selection_add(ISelectionItemProvider* This) { CALL(do_act, This, selection, call.a = RVN_A11Y_ADD_SELECTION); }
static HRESULT STDMETHODCALLTYPE selection_remove(ISelectionItemProvider* This) { CALL(do_act, This, selection, call.a = RVN_A11Y_REMOVE_SELECTION); }

static HRESULT do_get_selection(rvn_uia_call_t* c) {
	SAFEARRAY** out = (SAFEARRAY**)c->out;
	LONG i;
	int count;
	*out = NULL;
	if (!rvn_uia_alive(c->self)) return UIA_E_ELEMENTNOTAVAILABLE;
	count = rvn_a11y_selection_count(c->self->id);
	*out = g_uia.sa_create(VT_UNKNOWN, 0, (ULONG)count);
	if (!*out) return E_OUTOFMEMORY;
	for (i = 0; i < count; i++) {
		rvn_uia_t* row = rvn_uia_provider(c->self->hwnd, rvn_a11y_selection_at(c->self->id, (int)i));
		HRESULT hr = row ? g_uia.sa_put(*out, &i, &row->simple) : E_OUTOFMEMORY;
		if (FAILED(hr)) {
			g_uia.sa_destroy(*out);
			*out = NULL;
			return hr;
		}
	}
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE selection_group_get(ISelectionProvider* This, SAFEARRAY** out) {
	CALL(do_get_selection, This, selection_group, call.out = out);
}

static HRESULT STDMETHODCALLTYPE selection_group_multi(ISelectionProvider* This, BOOL* out) {
	uint32_t states = 0;
	HRESULT hr = rvn_uia_read(FROM(This, selection_group), 0, &states);
	*out = (states & RVN_A11Y_MULTI_SELECT) ? TRUE : FALSE;
	return hr;
}

static HRESULT STDMETHODCALLTYPE selection_group_required(ISelectionProvider* This, BOOL* out) {
	(void)This;
	*out = FALSE;
	return S_OK;
}

/* What is asked of a node: 0 states, 1 number, 2 minimum, 3 maximum, 4 step, 5 value text, 6 selection container */
static HRESULT do_read(rvn_uia_call_t* c) {
	rvn_a11y_node_t node;
	if (!rvn_uia_alive(c->self) || !rvn_a11y_describe(c->self->id, &node)) return UIA_E_ELEMENTNOTAVAILABLE;
	switch (c->a) {
		case 0: *(uint32_t*)c->out = node.states; break;
		case 1: *(double*)c->out = node.number; break;
		case 2: *(double*)c->out = node.minimum; break;
		case 3: *(double*)c->out = node.maximum; break;
		case 4: *(double*)c->out = node.step > 0.0 ? node.step : 1.0; break;
		case 5: *(BSTR*)c->out = rvn_uia_bstr(node.value); break;
		case 6: {
			rvn_uia_t* parent = rvn_uia_provider(c->self->hwnd, rvn_a11y_parent(c->self->id));
			*(IRawElementProviderSimple**)c->out = NULL;
			if (parent) {
				rvn_uia_addref(parent);
				*(IRawElementProviderSimple**)c->out = &parent->simple;
			}
			break;
		}
	}
	return S_OK;
}

static HRESULT rvn_uia_read(rvn_uia_t* self, int what, void* out) {
	rvn_uia_call_t call;
	memset(&call, 0, sizeof(call));
	call.fn = do_read;
	call.self = self;
	call.a = what;
	call.out = out;
	return rvn_uia_call(&call);
}

static HRESULT STDMETHODCALLTYPE toggle_state(IToggleProvider* This, enum ToggleState* out) {
	uint32_t states = 0;
	HRESULT hr = rvn_uia_read(FROM(This, toggle), 0, &states);
	*out = (states & RVN_A11Y_CHECKED) ? ToggleState_On : ToggleState_Off;
	return hr;
}

static HRESULT STDMETHODCALLTYPE expand_state(IExpandCollapseProvider* This, enum ExpandCollapseState* out) {
	uint32_t states = 0;
	HRESULT hr = rvn_uia_read(FROM(This, expand), 0, &states);
	*out = !(states & RVN_A11Y_EXPANDABLE) ? ExpandCollapseState_LeafNode : (states & RVN_A11Y_EXPANDED) ? ExpandCollapseState_Expanded : ExpandCollapseState_Collapsed;
	return hr;
}

static HRESULT STDMETHODCALLTYPE selection_is_selected(ISelectionItemProvider* This, BOOL* out) {
	uint32_t states = 0;
	HRESULT hr = rvn_uia_read(FROM(This, selection), 0, &states);
	*out = (states & RVN_A11Y_SELECTED) ? TRUE : FALSE;
	return hr;
}

static HRESULT STDMETHODCALLTYPE selection_container(ISelectionItemProvider* This, IRawElementProviderSimple** out) {
	return rvn_uia_read(FROM(This, selection), 6, out);
}

static HRESULT do_set_value(rvn_uia_call_t* c) {
	char* text;
	int ok;
	if (!rvn_uia_alive(c->self)) return UIA_E_ELEMENTNOTAVAILABLE;
	text = rvn_uia_utf8(c->text);
	ok = rvn_a11y_set_value(c->self->id, text);
	rvn_free(text);
	return ok ? S_OK : UIA_E_ELEMENTNOTAVAILABLE;
}

static HRESULT STDMETHODCALLTYPE value_set(IValueProvider* This, LPCWSTR text) { CALL(do_set_value, This, value, call.text = text); }
static HRESULT STDMETHODCALLTYPE value_get(IValueProvider* This, BSTR* out) { return rvn_uia_read(FROM(This, value), 5, out); }

static HRESULT STDMETHODCALLTYPE value_read_only(IValueProvider* This, BOOL* out) {
	uint32_t states = 0;
	HRESULT hr = rvn_uia_read(FROM(This, value), 0, &states);
	*out = (!(states & RVN_A11Y_EDITABLE) || (states & RVN_A11Y_READONLY)) ? TRUE : FALSE;
	return hr;
}

static HRESULT do_set_number(rvn_uia_call_t* c) {
	if (!rvn_uia_alive(c->self)) return UIA_E_ELEMENTNOTAVAILABLE;
	return rvn_a11y_set_number(c->self->id, c->d) ? S_OK : E_INVALIDARG;
}

static HRESULT STDMETHODCALLTYPE range_set(IRangeValueProvider* This, double value) { CALL(do_set_number, This, range, call.d = value); }
static HRESULT STDMETHODCALLTYPE range_value(IRangeValueProvider* This, double* out) { return rvn_uia_read(FROM(This, range), 1, out); }
static HRESULT STDMETHODCALLTYPE range_minimum(IRangeValueProvider* This, double* out) { return rvn_uia_read(FROM(This, range), 2, out); }
static HRESULT STDMETHODCALLTYPE range_maximum(IRangeValueProvider* This, double* out) { return rvn_uia_read(FROM(This, range), 3, out); }
static HRESULT STDMETHODCALLTYPE range_small(IRangeValueProvider* This, double* out) { return rvn_uia_read(FROM(This, range), 4, out); }

static HRESULT STDMETHODCALLTYPE range_large(IRangeValueProvider* This, double* out) {
	double step = 1.0;
	HRESULT hr = rvn_uia_read(FROM(This, range), 4, &step);
	*out = step * 10.0;
	return hr;
}

static HRESULT STDMETHODCALLTYPE range_read_only(IRangeValueProvider* This, BOOL* out) {
	uint32_t states = 0;
	HRESULT hr = rvn_uia_read(FROM(This, range), 0, &states);
	*out = (states & RVN_A11Y_READONLY) ? TRUE : FALSE;
	return hr;
}

/* ---------------------------------------------------------------------------
 *  Tables of functions
 * ------------------------------------------------------------------------- */

static IRawElementProviderSimpleVtbl g_simple_vtbl = {
	simple_query, simple_addref, simple_release, simple_options, simple_pattern, simple_property, simple_host
};
static IRawElementProviderFragmentVtbl g_fragment_vtbl = {
	fragment_query, fragment_addref, fragment_release, fragment_navigate, fragment_runtime_id, fragment_bounds,
	fragment_embedded, fragment_set_focus, fragment_root
};
static IRawElementProviderFragmentRootVtbl g_root_vtbl = {
	root_query, root_addref, root_release, root_from_point, root_get_focus
};
static IInvokeProviderVtbl g_invoke_vtbl = { invoke_query, invoke_addref, invoke_release, invoke_invoke };
static IToggleProviderVtbl g_toggle_vtbl = { toggle_query, toggle_addref, toggle_release, toggle_toggle, toggle_state };
static IValueProviderVtbl g_value_vtbl = { value_query, value_addref, value_release, value_set, value_get, value_read_only };
static IRangeValueProviderVtbl g_range_vtbl = {
	range_query, range_addref, range_release, range_set, range_value, range_read_only, range_maximum, range_minimum, range_large, range_small
};
static IExpandCollapseProviderVtbl g_expand_vtbl = { expand_query, expand_addref, expand_release, expand_expand, expand_collapse, expand_state };
static ISelectionItemProviderVtbl g_selection_vtbl = {
	selection_query, selection_addref, selection_release, selection_select, selection_add, selection_remove, selection_is_selected, selection_container
};
static ISelectionProviderVtbl g_selection_group_vtbl = {
	selection_group_query, selection_group_addref, selection_group_release,
	selection_group_get, selection_group_multi, selection_group_required
};

/* The provider of a node (made on first use; the table keeps a reference). */
static rvn_uia_t* rvn_uia_provider(HWND hwnd, rvn_a11y_id_t id) {
	rvn_uia_t* self;
	int i;
	if (!id) return NULL;
	for (i = 0; i < g_uia.count; i++) {
		if (g_uia.table[i]->id == id && !g_uia.table[i]->disconnected) return g_uia.table[i];
	}
	if (!hwnd) hwnd = rvn_uia_hwnd_of(id);
	self = (rvn_uia_t*)rvn_calloc(sizeof(rvn_uia_t));
	self->simple.lpVtbl = &g_simple_vtbl;
	self->fragment.lpVtbl = &g_fragment_vtbl;
	self->root.lpVtbl = &g_root_vtbl;
	self->invoke.lpVtbl = &g_invoke_vtbl;
	self->toggle.lpVtbl = &g_toggle_vtbl;
	self->value.lpVtbl = &g_value_vtbl;
	self->range.lpVtbl = &g_range_vtbl;
	self->expand.lpVtbl = &g_expand_vtbl;
	self->selection.lpVtbl = &g_selection_vtbl;
	self->selection_group.lpVtbl = &g_selection_group_vtbl;
	self->refs = 1;
	self->id = id;
	self->hwnd = hwnd;
	if (g_uia.count == g_uia.capacity) {
		g_uia.capacity = g_uia.capacity ? g_uia.capacity * 2 : 64;
		g_uia.table = (rvn_uia_t**)rvn_realloc(g_uia.table, (size_t)g_uia.capacity * sizeof(rvn_uia_t*));
	}
	g_uia.table[g_uia.count++] = self;
	return self;
}

/* The providers of nodes that are gone are dropped now and then. */
static void rvn_uia_collect(void) {
	int i = 0;
	while (i < g_uia.count) {
		rvn_uia_t* self = g_uia.table[i];
		if (!self->disconnected && rvn_a11y_exists(self->id)) {
			i++;
			continue;
		}
		if (!self->disconnected && g_uia.disconnect) {
			/* UI Automation may still hold the provider if COM disconnect fails. */
			if (FAILED(g_uia.disconnect(&self->simple))) {
				i++;
				continue;
			}
		}
		self->disconnected = R_TRUE;
		g_uia.table[i] = g_uia.table[--g_uia.count];
		rvn_uia_release(self);
	}
}

/* ---------------------------------------------------------------------------
 *  Platform
 * ------------------------------------------------------------------------- */

typedef struct rvn_uia_window_t { HWND hwnd; void* user; } rvn_uia_window_t;
static rvn_uia_window_t g_windows[64];
static int g_window_count;

static HWND rvn_uia_hwnd_of(rvn_a11y_id_t id) {
	Rid window = rvn_a11y_window(id);
	int i;
	for (i = 0; i < g_window_count; i++)
		if (rvn_a11y_window_of(g_windows[i].user) == window) return g_windows[i].hwnd;
	return NULL;
}

int rvn_uia_get_object(HWND hwnd, void* user, WPARAM wparam, LPARAM lparam, LRESULT* result) {
	rvn_uia_t* root;
	Rid window;
	int i;
	if ((long)lparam != (long)UiaRootObjectId || !rvn_uia_load()) return R_FALSE;
	for (i = 0; i < g_window_count && g_windows[i].hwnd != hwnd; i++) {}
	if (i == g_window_count && g_window_count < (int)RVN_COUNT_OF(g_windows)) {
		g_windows[g_window_count].hwnd = hwnd;
		g_windows[g_window_count].user = user;
		g_window_count++;
	}
	window = rvn_a11y_window_of(user);
	if (!window) return R_FALSE;
	rvn_uia_collect();
	root = rvn_uia_provider(hwnd, (rvn_a11y_id_t)window);
	if (!root) return R_FALSE;
	*result = g_uia.return_provider(hwnd, wparam, lparam, &root->simple);
	return R_TRUE;
}

void rvn_uia_window_destroyed(HWND hwnd) {
	int i;
	if (!g_uia.loaded) return;
	g_uia.return_provider(hwnd, 0, 0, NULL);
	for (i = 0; i < g_uia.count; i++) {
		if (g_uia.table[i]->hwnd == hwnd && !g_uia.table[i]->disconnected) {
			if (g_uia.disconnect) g_uia.disconnect(&g_uia.table[i]->simple);
			g_uia.table[i]->disconnected = R_TRUE;
		}
	}
	rvn_uia_collect();
	for (i = 0; i < g_window_count; i++) {
		if (g_windows[i].hwnd == hwnd) {
			g_windows[i] = g_windows[--g_window_count];
			break;
		}
	}
}

void rvn_uia_shutdown(void) {
	int i;
	for (i = 0; i < g_uia.count; i++) {
		if (!g_uia.table[i]->disconnected && g_uia.disconnect) g_uia.disconnect(&g_uia.table[i]->simple);
		g_uia.table[i]->disconnected = R_TRUE;
		rvn_uia_release(g_uia.table[i]);
	}
	rvn_free(g_uia.table);
	g_uia.table = NULL;
	g_uia.count = g_uia.capacity = 0;
	g_window_count = 0;
}

int rvn_uia_active(void) {
	BOOL reader = FALSE;
	if (SystemParametersInfoW(SPI_GETSCREENREADER, 0, &reader, 0) && reader) return R_TRUE;
	return rvn_uia_load() && g_uia.listening && g_uia.listening();
}

void rvn_uia_event(HWND hwnd, int event, uint64_t node) {
	rvn_uia_t* provider;
	rvn_a11y_node_t info;
	VARIANT old_value, new_value;
	if (!rvn_uia_load() || (g_uia.listening && !g_uia.listening())) return;
	provider = rvn_uia_provider(hwnd, node);
	if (!provider) return;
	switch (event) {
		case RVN_A11Y_EVENT_FOCUS:
			g_uia.raise_event(&provider->simple, UIA_AutomationFocusChangedEventId);
			break;
		case RVN_A11Y_EVENT_VALUE:
		case RVN_A11Y_EVENT_NAME:
		case RVN_A11Y_EVENT_STATE:
			if (!g_uia.raise_property || !rvn_a11y_describe(node, &info)) break;
			memset(&old_value, 0, sizeof(old_value));
			memset(&new_value, 0, sizeof(new_value));
			old_value.vt = VT_EMPTY;
			if (info.role == R_ROLE_TABLE) {
				g_uia.raise_event(&provider->simple, UIA_Selection_InvalidatedEventId);
				break;
			}
			if (info.states & RVN_A11Y_HAS_RANGE) {
				new_value.vt = VT_R8;
				new_value.dblVal = info.number;
				g_uia.raise_property(&provider->simple, UIA_RangeValueValuePropertyId, old_value, new_value);
			} else if (info.states & RVN_A11Y_CHECKABLE) {
				new_value.vt = VT_I4;
				new_value.lVal = (info.states & RVN_A11Y_CHECKED) ? ToggleState_On : ToggleState_Off;
				g_uia.raise_property(&provider->simple, UIA_ToggleToggleStatePropertyId, old_value, new_value);
			} else if (info.states & RVN_A11Y_EXPANDABLE) {
				new_value.vt = VT_I4;
				new_value.lVal = (info.states & RVN_A11Y_EXPANDED) ? ExpandCollapseState_Expanded : ExpandCollapseState_Collapsed;
				g_uia.raise_property(&provider->simple, UIA_ExpandCollapseExpandCollapseStatePropertyId, old_value, new_value);
			} else if (info.value[0] || (info.states & RVN_A11Y_EDITABLE)) {
				new_value.vt = VT_BSTR;
				new_value.bstrVal = rvn_uia_bstr(info.value);
				g_uia.raise_property(&provider->simple, UIA_ValueValuePropertyId, old_value, new_value);
				if (g_uia.sys_free) g_uia.sys_free(new_value.bstrVal);
			} else {
				new_value.vt = VT_BSTR;
				new_value.bstrVal = rvn_uia_bstr(info.name);
				g_uia.raise_property(&provider->simple, UIA_NamePropertyId, old_value, new_value);
				if (g_uia.sys_free) g_uia.sys_free(new_value.bstrVal);
			}
			break;
		case RVN_A11Y_EVENT_STRUCTURE:
			rvn_uia_collect();
			if (g_uia.raise_structure) g_uia.raise_structure(&provider->simple, StructureChangeType_ChildrenInvalidated, NULL, 0);
			break;
		default:
			break;
	}
}

void rvn_uia_announce(HWND hwnd, void* user, const char* text, int assertive) {
	rvn_uia_t* root;
	BSTR message, activity;
	if (!rvn_uia_load() || !g_uia.raise_notification) return;
	root = rvn_uia_provider(hwnd, (rvn_a11y_id_t)rvn_a11y_window_of(user));
	if (!root) return;
	message = rvn_uia_bstr(text);
	activity = rvn_uia_bstr("RavenAnnouncement");
	/* NotificationKind_Other, NotificationProcessing_ImportantMostRecent / _MostRecent */
	g_uia.raise_notification(&root->simple, 4, assertive ? 1 : 3, message, activity);
	if (g_uia.sys_free) {
		g_uia.sys_free(message);
		g_uia.sys_free(activity);
	}
}

void rvn_uia_settings(RAccessibilitySettings* settings) {
	HIGHCONTRASTW contrast;
	BOOL animations = TRUE;
	HMODULE advapi;
	memset(&contrast, 0, sizeof(contrast));
	contrast.cbSize = sizeof(contrast);
	if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0)) settings->high_contrast = (contrast.dwFlags & HCF_HIGHCONTRASTON) != 0;
	if (SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animations, 0)) settings->reduce_motion = !animations;
	/* Settings > Accessibility > Text size: 100 to 225 % */
	advapi = LoadLibraryW(L"Advapi32.dll");
	if (advapi) {
		reg_get_fn get = (reg_get_fn)(void*)GetProcAddress(advapi, "RegGetValueW");
		DWORD percent = 0, size = sizeof(percent);
		if (get && get(HKEY_CURRENT_USER, L"Software\\Microsoft\\Accessibility", L"TextScaleFactor", RRF_RT_REG_DWORD, NULL, &percent, &size) == ERROR_SUCCESS &&
			percent >= 100 && percent <= 400) {
			settings->text_scale = (float)percent / 100.0f;
		}
	}
}

#endif
