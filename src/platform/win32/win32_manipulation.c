/**
 * Raven - platform/win32/win32_manipulation.c
 * The precision touchpads of Windows (Windows 10 and later) through Direct
 * Manipulation, as the browsers read them. Without it a touchpad is only the
 * wheel of a mouse: its scrolling comes in notches, its pinch as Ctrl and the
 * wheel; with it, the fingers come as such:
 *
 *   - DM_POINTERHITTEST: a contact of the touchpad begins over the window,
 *     and is given to the viewport of the window (SetContact);
 *   - the viewport moves and scales a content of its own, which Raven does
 *     not draw: what changed since the previous update is told to the core,
 *     as a precise wheel (two fingers that scroll, then the inertia of the
 *     system) or as a pinch; when the fingers are done the content is put
 *     back where it was;
 *   - Direct Manipulation is updated by hand (MANUALUPDATE), from a timer of
 *     the window, while the fingers act.
 *
 * COM is initialized for the UI thread (apartment threaded) the first time a
 * window is made. Built without the header (old MinGW), on Windows 7, when the
 * thread is already in another apartment, or with RAVEN_DIRECT_MANIPULATION=0,
 * nothing changes: the touchpad stays the wheel.
 */

#include "win32_manipulation.h"

#if defined(_WIN32)

#if !defined(RAVEN_NO_DIRECT_MANIPULATION)
#  if defined(__has_include)
#    if __has_include(<directmanipulation.h>)
#      define RVN_DM 1
#    endif
#  elif defined(_MSC_VER)
/* (every SDK since the one of Windows 8 has it) */
#    define RVN_DM 1
#  endif
#endif

#if defined(RVN_DM)

#include <objbase.h>
#include <directmanipulation.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* the identifiers, here rather than from uuid.lib, which not every toolchain has with them */
static const GUID rvn_dm_clsid_manager = { 0x54e211b6, 0x3650, 0x4f75, { 0x83, 0x34, 0xfa, 0x35, 0x95, 0x98, 0xe1, 0xc5 } };
static const GUID rvn_dm_iid_manager = { 0xfbf5d3b4, 0x70c7, 0x4163, { 0x93, 0x22, 0x5a, 0x6f, 0x66, 0x0d, 0x6f, 0xbc } };
static const GUID rvn_dm_iid_update_manager = { 0xb0ae62fd, 0xbe34, 0x46e7, { 0x9c, 0xaa, 0xd3, 0x61, 0xfa, 0xcb, 0xb9, 0xcc } };
static const GUID rvn_dm_iid_viewport = { 0x28b85a3d, 0x60a0, 0x48bd, { 0x9b, 0xa1, 0x5c, 0xe8, 0xd9, 0xea, 0x3a, 0x6d } };
static const GUID rvn_dm_iid_handler = { 0x952121da, 0xd69f, 0x45f9, { 0xb0, 0xf9, 0xf2, 0x39, 0x44, 0x32, 0x1a, 0x6d } };
static const GUID rvn_dm_iid_unknown = { 0x00000000, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

/* (PT_TOUCHPAD of winuser.h: Windows 8.1) */
#define RVN_DM_PT_TOUCHPAD 5
/* the size of the viewport: only a frame for the transform, nothing is shown in it */
#define RVN_DM_SIZE 1000

enum { RVN_DM_NONE, RVN_DM_SCROLL, RVN_DM_PINCH };

typedef BOOL (WINAPI *rvn_dm_get_pointer_type_fn)(UINT32, DWORD*);

typedef struct rvn_dm_handler_t {
	/* (first: the pointer COM hands around is this one) */
	IDirectManipulationViewportEventHandler iface;
	LONG refs;
	struct rvn_win32_manipulation_t* owner;
} rvn_dm_handler_t;

struct rvn_win32_manipulation_t {
	HWND window;
	IDirectManipulationManager* manager;
	IDirectManipulationUpdateManager* update;
	IDirectManipulationViewport* viewport;
	rvn_dm_handler_t* handler;
	DWORD cookie;
	int has_cookie;
	rvn_win32_manipulation_fn callback;
	void* user;
	DIRECTMANIPULATION_STATUS status;
	int gesture;
	float last_scale, last_x, last_y;
	int timer;
};

/* 1: COM initialized here (undone at the end); -1: it cannot be (the thread is in another apartment) */
static int g_dm_com;
static rvn_dm_get_pointer_type_fn g_dm_get_pointer_type;

/* ---- the handler of the events of a viewport (a COM object) ---- */

static HRESULT STDMETHODCALLTYPE rvn_dm_query_interface(IDirectManipulationViewportEventHandler* self, REFIID riid, void** object) {
	if (!object) return E_POINTER;
	if (memcmp(riid, &rvn_dm_iid_unknown, sizeof(GUID)) == 0 || memcmp(riid, &rvn_dm_iid_handler, sizeof(GUID)) == 0) {
		*object = self;
		self->lpVtbl->AddRef(self);
		return S_OK;
	}
	*object = NULL;
	return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE rvn_dm_add_ref(IDirectManipulationViewportEventHandler* self) {
	return (ULONG)InterlockedIncrement(&((rvn_dm_handler_t*)self)->refs);
}

static ULONG STDMETHODCALLTYPE rvn_dm_release(IDirectManipulationViewportEventHandler* self) {
	LONG refs = InterlockedDecrement(&((rvn_dm_handler_t*)self)->refs);
	if (refs == 0) free(self);
	return (ULONG)refs;
}

static void rvn_dm_emit(rvn_win32_manipulation_t* manipulation, int scroll, float dx, float dy, int pinch_phase, float scale) {
	rvn_win32_manipulation_event_t event;
	memset(&event, 0, sizeof(event));
	event.scroll = scroll;
	event.dx = dx;
	event.dy = dy;
	event.pinch_phase = pinch_phase;
	event.scale = scale;
	manipulation->callback(manipulation->user, &event);
}

static void rvn_dm_reset(rvn_win32_manipulation_t* manipulation) {
	manipulation->gesture = RVN_DM_NONE;
	manipulation->last_scale = 1.0f;
	manipulation->last_x = manipulation->last_y = 0.0f;
}

static HRESULT STDMETHODCALLTYPE rvn_dm_status_changed(IDirectManipulationViewportEventHandler* self, IDirectManipulationViewport* viewport,
	DIRECTMANIPULATION_STATUS current, DIRECTMANIPULATION_STATUS previous) {
	rvn_win32_manipulation_t* manipulation = ((rvn_dm_handler_t*)self)->owner;
	(void)previous;
	if (!manipulation) return S_OK;
	manipulation->status = current;
	if (current == DIRECTMANIPULATION_RUNNING && !manipulation->timer) {
		/* the fingers act: Direct Manipulation is updated until they are done */
		manipulation->timer = SetTimer(manipulation->window, RVN_WIN32_MANIPULATION_TIMER, USER_TIMER_MINIMUM, NULL) != 0;
	} else if (current == DIRECTMANIPULATION_READY) {
		if (manipulation->gesture == RVN_DM_PINCH) rvn_dm_emit(manipulation, R_FALSE, 0.0f, 0.0f, R_GESTURE_END, 1.0f);
		rvn_dm_reset(manipulation);
		/* the content goes back where it was, for the next gesture (the update that follows is not one) */
		viewport->lpVtbl->ZoomToRect(viewport, 0.0f, 0.0f, (float)RVN_DM_SIZE, (float)RVN_DM_SIZE, FALSE);
		if (manipulation->timer) KillTimer(manipulation->window, RVN_WIN32_MANIPULATION_TIMER);
		manipulation->timer = R_FALSE;
	}
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE rvn_dm_viewport_updated(IDirectManipulationViewportEventHandler* self, IDirectManipulationViewport* viewport) {
	(void)self; (void)viewport;
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE rvn_dm_content_updated(IDirectManipulationViewportEventHandler* self, IDirectManipulationViewport* viewport,
	IDirectManipulationContent* content) {
	rvn_win32_manipulation_t* manipulation = ((rvn_dm_handler_t*)self)->owner;
	float transform[6], scale, x, y;
	(void)viewport;
	if (!manipulation) return S_OK;
	/* (only while the fingers act, or the content coasts after them: not the putting back) */
	if (manipulation->status != DIRECTMANIPULATION_RUNNING && manipulation->status != DIRECTMANIPULATION_INERTIA) return S_OK;
	if (FAILED(content->lpVtbl->GetContentTransform(content, transform, 6))) return S_OK;
	scale = transform[0];
	x = transform[4];
	y = transform[5];
	if (!(scale > 0.0f)) return S_OK;
	if (manipulation->gesture == RVN_DM_NONE) {
		/* the first change says what the gesture is: it stays so until the fingers are done */
		if (fabsf(scale - 1.0f) > 0.0001f) {
			manipulation->gesture = RVN_DM_PINCH;
			rvn_dm_emit(manipulation, R_FALSE, 0.0f, 0.0f, R_GESTURE_BEGIN, 1.0f);
		} else if (x != manipulation->last_x || y != manipulation->last_y) {
			manipulation->gesture = RVN_DM_SCROLL;
		}
	}
	if (manipulation->gesture == RVN_DM_PINCH && scale != manipulation->last_scale) {
		/* (the translation of a pinch is the zoom around its center: the core makes its own) */
		rvn_dm_emit(manipulation, R_FALSE, 0.0f, 0.0f, R_GESTURE_UPDATE, scale / manipulation->last_scale);
	} else if (manipulation->gesture == RVN_DM_SCROLL && (x != manipulation->last_x || y != manipulation->last_y)) {
		rvn_dm_emit(manipulation, R_TRUE, x - manipulation->last_x, y - manipulation->last_y, 0, 1.0f);
	}
	manipulation->last_scale = scale;
	manipulation->last_x = x;
	manipulation->last_y = y;
	return S_OK;
}

static IDirectManipulationViewportEventHandlerVtbl g_dm_handler_vtbl = {
	rvn_dm_query_interface, rvn_dm_add_ref, rvn_dm_release,
	rvn_dm_status_changed, rvn_dm_viewport_updated, rvn_dm_content_updated
};

/* ---- the viewport of a window ---- */

rvn_win32_manipulation_t* rvn_win32_manipulation_create(HWND window, rvn_win32_manipulation_fn callback, void* user) {
	rvn_win32_manipulation_t* manipulation;
	const char* choice = getenv("RAVEN_DIRECT_MANIPULATION");
	DIRECTMANIPULATION_CONFIGURATION configuration;
	RECT rect;
	HRESULT result;

	if (choice && choice[0] == '0') return NULL;
	if (!g_dm_get_pointer_type) {
		HMODULE user32 = GetModuleHandleW(L"user32.dll");
		FARPROC function = user32 ? GetProcAddress(user32, "GetPointerType") : NULL;
		g_dm_get_pointer_type = (rvn_dm_get_pointer_type_fn)(void (*)(void))function;
		if (!g_dm_get_pointer_type) return NULL;
	}
	if (g_dm_com == 0) {
		/* S_FALSE: the application had done it already (it is counted all the same) */
		result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		g_dm_com = result == S_OK || result == S_FALSE ? 1 : -1;
	}
	if (g_dm_com < 0) return NULL;

	manipulation = (rvn_win32_manipulation_t*)calloc(1, sizeof(*manipulation));
	if (!manipulation) return NULL;
	manipulation->window = window;
	manipulation->callback = callback;
	manipulation->user = user;
	rvn_dm_reset(manipulation);

	result = CoCreateInstance(&rvn_dm_clsid_manager, NULL, CLSCTX_INPROC_SERVER, &rvn_dm_iid_manager, (void**)&manipulation->manager);
	if (SUCCEEDED(result)) result = manipulation->manager->lpVtbl->GetUpdateManager(manipulation->manager, &rvn_dm_iid_update_manager, (void**)&manipulation->update);
	if (SUCCEEDED(result)) result = manipulation->manager->lpVtbl->CreateViewport(manipulation->manager, NULL, window, &rvn_dm_iid_viewport, (void**)&manipulation->viewport);
	if (SUCCEEDED(result)) {
		/* both ways, kept to one when the fingers start along it, with the inertia of the system; and the pinch */
		configuration = (DIRECTMANIPULATION_CONFIGURATION)(DIRECTMANIPULATION_CONFIGURATION_INTERACTION |
			DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_X | DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_Y |
			DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_INERTIA | DIRECTMANIPULATION_CONFIGURATION_RAILS_X |
			DIRECTMANIPULATION_CONFIGURATION_RAILS_Y | DIRECTMANIPULATION_CONFIGURATION_SCALING);
		result = manipulation->viewport->lpVtbl->ActivateConfiguration(manipulation->viewport, configuration);
	}
	if (SUCCEEDED(result)) result = manipulation->viewport->lpVtbl->SetViewportOptions(manipulation->viewport, DIRECTMANIPULATION_VIEWPORT_OPTIONS_MANUALUPDATE);
	if (SUCCEEDED(result)) {
		manipulation->handler = (rvn_dm_handler_t*)calloc(1, sizeof(rvn_dm_handler_t));
		if (!manipulation->handler) result = E_OUTOFMEMORY;
	}
	if (SUCCEEDED(result)) {
		manipulation->handler->iface.lpVtbl = &g_dm_handler_vtbl;
		manipulation->handler->refs = 1;
		manipulation->handler->owner = manipulation;
		result = manipulation->viewport->lpVtbl->AddEventHandler(manipulation->viewport, window, &manipulation->handler->iface, &manipulation->cookie);
		manipulation->has_cookie = SUCCEEDED(result);
	}
	if (SUCCEEDED(result)) {
		rect.left = rect.top = 0;
		rect.right = rect.bottom = RVN_DM_SIZE;
		result = manipulation->viewport->lpVtbl->SetViewportRect(manipulation->viewport, &rect);
	}
	if (SUCCEEDED(result)) result = manipulation->manager->lpVtbl->Activate(manipulation->manager, window);
	if (SUCCEEDED(result)) result = manipulation->viewport->lpVtbl->Enable(manipulation->viewport);
	if (FAILED(result)) {
		rvn_win32_manipulation_destroy(manipulation);
		return NULL;
	}
	return manipulation;
}

void rvn_win32_manipulation_destroy(rvn_win32_manipulation_t* manipulation) {
	if (!manipulation) return;
	if (manipulation->timer) KillTimer(manipulation->window, RVN_WIN32_MANIPULATION_TIMER);
	if (manipulation->handler) manipulation->handler->owner = NULL;
	if (manipulation->viewport) {
		manipulation->viewport->lpVtbl->Stop(manipulation->viewport);
		if (manipulation->has_cookie) manipulation->viewport->lpVtbl->RemoveEventHandler(manipulation->viewport, manipulation->cookie);
		manipulation->viewport->lpVtbl->Abandon(manipulation->viewport);
		manipulation->viewport->lpVtbl->Release(manipulation->viewport);
	}
	/* (the viewport let go of its reference: this one is the last, unless COM still holds one) */
	if (manipulation->handler) manipulation->handler->iface.lpVtbl->Release(&manipulation->handler->iface);
	if (manipulation->update) manipulation->update->lpVtbl->Release(manipulation->update);
	if (manipulation->manager) {
		manipulation->manager->lpVtbl->Deactivate(manipulation->manager, manipulation->window);
		manipulation->manager->lpVtbl->Release(manipulation->manager);
	}
	free(manipulation);
}

int rvn_win32_manipulation_hit_test(rvn_win32_manipulation_t* manipulation, WPARAM wparam) {
	UINT32 id = (UINT32)LOWORD(wparam);
	DWORD type = 0;
	if (!manipulation || !g_dm_get_pointer_type || !g_dm_get_pointer_type(id, &type)) return R_FALSE;
	if (type != RVN_DM_PT_TOUCHPAD) return R_FALSE;
	return SUCCEEDED(manipulation->viewport->lpVtbl->SetContact(manipulation->viewport, id));
}

void rvn_win32_manipulation_tick(rvn_win32_manipulation_t* manipulation) {
	if (manipulation && manipulation->update) manipulation->update->lpVtbl->Update(manipulation->update, NULL);
}

void rvn_win32_manipulation_shutdown(void) {
	if (g_dm_com > 0) CoUninitialize();
	g_dm_com = 0;
	g_dm_get_pointer_type = NULL;
}

#else /* !RVN_DM */

rvn_win32_manipulation_t* rvn_win32_manipulation_create(HWND window, rvn_win32_manipulation_fn callback, void* user) {
	(void)window; (void)callback; (void)user;
	return NULL;
}
void rvn_win32_manipulation_destroy(rvn_win32_manipulation_t* manipulation) { (void)manipulation; }
int rvn_win32_manipulation_hit_test(rvn_win32_manipulation_t* manipulation, WPARAM wparam) {
	(void)manipulation; (void)wparam;
	return R_FALSE;
}
void rvn_win32_manipulation_tick(rvn_win32_manipulation_t* manipulation) { (void)manipulation; }
void rvn_win32_manipulation_shutdown(void) {}

#endif /* RVN_DM */

#endif /* _WIN32 */
