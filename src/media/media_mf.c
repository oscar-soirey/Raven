/**
 * Raven - media/media_mf.c
 * Media players on Windows (media.h): the Media Engine of Media Foundation
 * (IMFMediaEngine, Windows 8 and later) in frame-server mode. It opens the
 * file or the address, decodes, keeps the sound in time and plays it on the
 * output of the system; when a new picture is due (OnVideoStreamTick) it is
 * drawn into a WIC bitmap in BGRA (TransferVideoFrame), which is copied into
 * tight RGBA rows for the texture of the player. No Direct3D device is made:
 * the engine decodes in software without one, and Raven's texture is filled
 * the same way on every backend (one copy per new picture, only while the
 * picture is shown).
 *
 * Nothing is linked: mfplat.dll and oleaut32.dll are opened with LoadLibrary
 * the first time something is played, and the engine and WIC are made with
 * CoCreateInstance. The "N" editions of Windows, and Windows Server without
 * its Media Foundation feature, have no mfplat.dll: a program linked to it
 * would not start there; with it opened at run time it starts, and
 * Rmedia_is_supported is R_FALSE. The GUIDs are written here (no mfuuid.lib).
 *
 * The engine calls back on its own threads (IMFMediaEngineNotify): the
 * callback only sets flags; everything else is asked of the engine from the
 * UI thread when the core polls (media.c).
 */

/* (the Media Engine is declared for Windows 8 and later; Raven itself builds for Windows 7, where it is not found at run time) */
#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0602
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0602
#endif
#if !defined(WINVER) || WINVER < 0x0602
#undef WINVER
#define WINVER 0x0602
#endif
#ifndef COBJMACROS
#define COBJMACROS
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objbase.h>
#include <mfapi.h>
#include <mfmediaengine.h>
#include <wincodec.h>
#include <math.h>

#include "media.h"

/* DXGI_FORMAT_B8G8R8A8_UNORM, without dxgiformat.h */
#define RVN_MF_FORMAT_BGRA 87
#define RVN_MF_VERSION 0x00020070

static const GUID RVN_IID_IUnknown = { 0x00000000, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID RVN_CLSID_MFMediaEngineClassFactory = { 0xb44392da, 0x499b, 0x446b, { 0xa4, 0xcb, 0x00, 0x5f, 0xea, 0xd0, 0xe6, 0xd5 } };
static const GUID RVN_IID_IMFMediaEngineClassFactory = { 0x4d645ace, 0x26aa, 0x4688, { 0x9b, 0xe1, 0xdf, 0x35, 0x16, 0x99, 0x0b, 0x93 } };
static const GUID RVN_IID_IMFMediaEngineNotify = { 0xfee7c112, 0xe776, 0x42b5, { 0x9b, 0xbf, 0x00, 0x48, 0x52, 0x4e, 0x2b, 0xd5 } };
static const GUID RVN_MF_MEDIA_ENGINE_CALLBACK = { 0xc60381b8, 0x83a4, 0x41f8, { 0xa3, 0xd0, 0xde, 0x05, 0x07, 0x68, 0x49, 0xa9 } };
static const GUID RVN_MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT = { 0x5066893c, 0x8cf9, 0x42bc, { 0x8b, 0x8a, 0x47, 0x22, 0x12, 0xe5, 0x27, 0x26 } };
static const GUID RVN_CLSID_WICImagingFactory = { 0xcacaf262, 0x9370, 0x4615, { 0xa1, 0x3b, 0x9f, 0x55, 0x39, 0xda, 0x4c, 0x0a } };
static const GUID RVN_IID_IWICImagingFactory = { 0xec5ec8a9, 0xc395, 0x4314, { 0x9c, 0x77, 0x54, 0xd7, 0xa9, 0x35, 0xff, 0x70 } };
static const GUID RVN_GUID_WICPixelFormat32bppBGRA = { 0x6fddc324, 0x4e03, 0x4bfe, { 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0f } };

typedef HRESULT (WINAPI *rvn_mf_startup_fn)(ULONG, DWORD);
typedef HRESULT (WINAPI *rvn_mf_create_attributes_fn)(IMFAttributes**, UINT32);
typedef BSTR (WINAPI *rvn_sys_alloc_fn)(const OLECHAR*);
typedef void (WINAPI *rvn_sys_free_fn)(BSTR);

static struct {
	int tried;
	int available;
	rvn_mf_create_attributes_fn create_attributes;
	rvn_sys_alloc_fn alloc_string;
	rvn_sys_free_fn free_string;
	IMFMediaEngineClassFactory* factory;
	IWICImagingFactory* wic;
} g_rvn_mf;

/* ---------------------------------------------------------------------------
 *  The callback of the engine (its threads): flags only
 * ------------------------------------------------------------------------- */

typedef struct rvn_mf_notify_t {
	IMFMediaEngineNotify iface;
	volatile LONG refs;
	volatile LONG ended;
	volatile LONG failed;
	volatile LONG failure;
} rvn_mf_notify_t;

static HRESULT STDMETHODCALLTYPE rvn_mf_notify_query(IMFMediaEngineNotify* self, REFIID riid, void** out) {
	if (!out) return E_POINTER;
	if (IsEqualGUID(riid, &RVN_IID_IUnknown) || IsEqualGUID(riid, &RVN_IID_IMFMediaEngineNotify)) {
		*out = self;
		InterlockedIncrement(&((rvn_mf_notify_t*)self)->refs);
		return S_OK;
	}
	*out = NULL;
	return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE rvn_mf_notify_add_ref(IMFMediaEngineNotify* self) {
	return (ULONG)InterlockedIncrement(&((rvn_mf_notify_t*)self)->refs);
}

static ULONG STDMETHODCALLTYPE rvn_mf_notify_release(IMFMediaEngineNotify* self) {
	LONG refs = InterlockedDecrement(&((rvn_mf_notify_t*)self)->refs);
	/* (allocated with HeapAlloc: the engine may release it on its thread after the library is gone) */
	if (refs == 0) HeapFree(GetProcessHeap(), 0, self);
	return (ULONG)refs;
}

static HRESULT STDMETHODCALLTYPE rvn_mf_notify_event(IMFMediaEngineNotify* self, DWORD event, DWORD_PTR param1, DWORD param2) {
	rvn_mf_notify_t* notify = (rvn_mf_notify_t*)self;
	(void)param1;
	if (event == MF_MEDIA_ENGINE_EVENT_ENDED) {
		InterlockedExchange(&notify->ended, 1);
	} else if (event == MF_MEDIA_ENGINE_EVENT_ERROR) {
		InterlockedExchange(&notify->failure, (LONG)param2);
		InterlockedExchange(&notify->failed, 1);
	}
	return S_OK;
}

static IMFMediaEngineNotifyVtbl g_rvn_mf_notify_vtbl = {
	rvn_mf_notify_query,
	rvn_mf_notify_add_ref,
	rvn_mf_notify_release,
	rvn_mf_notify_event
};

/* ---------------------------------------------------------------------------
 *  Loading
 * ------------------------------------------------------------------------- */

static int rvn_mf_available(void) {
	HMODULE mfplat, oleaut;
	rvn_mf_startup_fn startup;
	HRESULT hr;
	if (g_rvn_mf.tried) return g_rvn_mf.available;
	g_rvn_mf.tried = 1;
	mfplat = LoadLibraryW(L"mfplat.dll");
	oleaut = LoadLibraryW(L"oleaut32.dll");
	if (!mfplat || !oleaut) return 0;
	startup = (rvn_mf_startup_fn)(void (*)(void))GetProcAddress(mfplat, "MFStartup");
	g_rvn_mf.create_attributes = (rvn_mf_create_attributes_fn)(void (*)(void))GetProcAddress(mfplat, "MFCreateAttributes");
	g_rvn_mf.alloc_string = (rvn_sys_alloc_fn)(void (*)(void))GetProcAddress(oleaut, "SysAllocString");
	g_rvn_mf.free_string = (rvn_sys_free_fn)(void (*)(void))GetProcAddress(oleaut, "SysFreeString");
	if (!startup || !g_rvn_mf.create_attributes || !g_rvn_mf.alloc_string || !g_rvn_mf.free_string) return 0;
	/* (COM: the window system initialized it already; headless, it is done here; another mode is kept) */
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	(void)hr;
	if (FAILED(startup(RVN_MF_VERSION, 0))) return 0;
	if (FAILED(CoCreateInstance(&RVN_CLSID_MFMediaEngineClassFactory, NULL, CLSCTX_INPROC_SERVER, &RVN_IID_IMFMediaEngineClassFactory,
		(void**)&g_rvn_mf.factory))) return 0;
	if (FAILED(CoCreateInstance(&RVN_CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &RVN_IID_IWICImagingFactory, (void**)&g_rvn_mf.wic))) {
		IMFMediaEngineClassFactory_Release(g_rvn_mf.factory);
		g_rvn_mf.factory = NULL;
		return 0;
	}
	/* (MFShutdown is never called: the engines may still be finishing on their threads at exit) */
	g_rvn_mf.available = 1;
	return 1;
}

typedef struct rvn_mf_stream_t {
	IMFMediaEngine* engine;
	rvn_mf_notify_t* notify;
	IWICBitmap* bitmap;
	int bitmap_width, bitmap_height;
	int playing;
	float rate;
} rvn_mf_stream_t;

static WCHAR* rvn_mf_wide(const char* text) {
	int length = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
	WCHAR* wide;
	if (length <= 0) return NULL;
	wide = (WCHAR*)rvn_alloc(sizeof(WCHAR) * (size_t)length);
	MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, length);
	return wide;
}

static void* rvn_mf_open(const char* location, int is_url, char* error) {
	rvn_mf_stream_t* stream;
	IMFAttributes* attributes = NULL;
	WCHAR* wide;
	BSTR url;
	HRESULT hr;
	(void)is_url;
	stream = (rvn_mf_stream_t*)rvn_calloc(sizeof(rvn_mf_stream_t));
	stream->rate = 1.0f;
	stream->notify = (rvn_mf_notify_t*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(rvn_mf_notify_t));
	if (!stream->notify) {
		rvn_free(stream);
		snprintf(error, 256, "out of memory");
		return NULL;
	}
	stream->notify->iface.lpVtbl = &g_rvn_mf_notify_vtbl;
	stream->notify->refs = 1;
	hr = g_rvn_mf.create_attributes(&attributes, 2);
	if (SUCCEEDED(hr)) hr = IMFAttributes_SetUnknown(attributes, &RVN_MF_MEDIA_ENGINE_CALLBACK, (IUnknown*)&stream->notify->iface);
	/* a format for the pictures and no window: frame-server mode */
	if (SUCCEEDED(hr)) hr = IMFAttributes_SetUINT32(attributes, &RVN_MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, RVN_MF_FORMAT_BGRA);
	if (SUCCEEDED(hr)) hr = IMFMediaEngineClassFactory_CreateInstance(g_rvn_mf.factory, 0, attributes, &stream->engine);
	if (attributes) IMFAttributes_Release(attributes);
	if (FAILED(hr) || !stream->engine) {
		snprintf(error, 256, "Media Foundation cannot make a player (0x%08lX)", (unsigned long)hr);
		rvn_mf_notify_release(&stream->notify->iface);
		rvn_free(stream);
		return NULL;
	}
	IMFMediaEngine_SetAutoPlay(stream->engine, FALSE);
	IMFMediaEngine_SetPreload(stream->engine, MF_MEDIA_ENGINE_PRELOAD_AUTOMATIC);
	wide = rvn_mf_wide(location);
	url = wide ? g_rvn_mf.alloc_string(wide) : NULL;
	rvn_free(wide);
	hr = url ? IMFMediaEngine_SetSource(stream->engine, url) : E_OUTOFMEMORY;
	if (url) g_rvn_mf.free_string(url);
	if (FAILED(hr)) {
		snprintf(error, 256, "Media Foundation cannot open %s (0x%08lX)", location, (unsigned long)hr);
		IMFMediaEngine_Shutdown(stream->engine);
		IMFMediaEngine_Release(stream->engine);
		rvn_mf_notify_release(&stream->notify->iface);
		rvn_free(stream);
		return NULL;
	}
	return stream;
}

static void rvn_mf_close(void* data) {
	rvn_mf_stream_t* stream = (rvn_mf_stream_t*)data;
	IMFMediaEngine_Shutdown(stream->engine);
	IMFMediaEngine_Release(stream->engine);
	if (stream->bitmap) IWICBitmap_Release(stream->bitmap);
	rvn_mf_notify_release(&stream->notify->iface);
	rvn_free(stream);
}

static void rvn_mf_play(void* data) {
	rvn_mf_stream_t* stream = (rvn_mf_stream_t*)data;
	stream->playing = 1;
	InterlockedExchange(&stream->notify->ended, 0);
	IMFMediaEngine_Play(stream->engine);
}

static void rvn_mf_pause(void* data) {
	rvn_mf_stream_t* stream = (rvn_mf_stream_t*)data;
	stream->playing = 0;
	IMFMediaEngine_Pause(stream->engine);
}

static void rvn_mf_seek(void* data, double seconds) {
	rvn_mf_stream_t* stream = (rvn_mf_stream_t*)data;
	InterlockedExchange(&stream->notify->ended, 0);
	IMFMediaEngine_SetCurrentTime(stream->engine, seconds);
}

static void rvn_mf_set_volume(void* data, float volume, int muted) {
	rvn_mf_stream_t* stream = (rvn_mf_stream_t*)data;
	IMFMediaEngine_SetVolume(stream->engine, (double)volume);
	IMFMediaEngine_SetMuted(stream->engine, muted ? TRUE : FALSE);
}

static void rvn_mf_set_rate(void* data, float rate) {
	rvn_mf_stream_t* stream = (rvn_mf_stream_t*)data;
	stream->rate = rate;
	IMFMediaEngine_SetDefaultPlaybackRate(stream->engine, (double)rate);
	IMFMediaEngine_SetPlaybackRate(stream->engine, (double)rate);
}

static const char* rvn_mf_error_text(USHORT code) {
	switch (code) {
		case MF_MEDIA_ENGINE_ERR_ABORTED: return "the load was aborted";
		case MF_MEDIA_ENGINE_ERR_NETWORK: return "a network error stopped the load";
		case MF_MEDIA_ENGINE_ERR_DECODE: return "it cannot be decoded";
		case MF_MEDIA_ENGINE_ERR_SRC_NOT_SUPPORTED: return "this media format is not supported";
		case MF_MEDIA_ENGINE_ERR_ENCRYPTED: return "it is encrypted";
		default: return "Media Foundation cannot play it";
	}
}

static void rvn_mf_poll(void* data, double dt, rvn_media_info_t* info) {
	rvn_mf_stream_t* stream = (rvn_mf_stream_t*)data;
	IMFMediaError* failure = NULL;
	double duration, position;
	DWORD width = 0, height = 0;
	(void)dt;
	if (SUCCEEDED(IMFMediaEngine_GetError(stream->engine, &failure)) && failure) {
		USHORT code = IMFMediaError_GetErrorCode(failure);
		HRESULT hr = IMFMediaError_GetExtendedErrorCode(failure);
		IMFMediaError_Release(failure);
		if (code != MF_MEDIA_ENGINE_ERR_NOERROR) {
			info->status = RVN_MEDIA_FAILED;
			snprintf(info->error, sizeof(info->error), "%s (0x%08lX)", rvn_mf_error_text(code), (unsigned long)hr);
			return;
		}
	}
	if (InterlockedCompareExchange(&stream->notify->failed, 0, 0)) {
		info->status = RVN_MEDIA_FAILED;
		snprintf(info->error, sizeof(info->error), "Media Foundation cannot play it (0x%08lX)",
			(unsigned long)InterlockedCompareExchange(&stream->notify->failure, 0, 0));
		return;
	}
	if (IMFMediaEngine_GetReadyState(stream->engine) < MF_MEDIA_ENGINE_READY_HAVE_METADATA) {
		info->status = RVN_MEDIA_OPENING;
		return;
	}
	info->status = RVN_MEDIA_OPENED;
	duration = IMFMediaEngine_GetDuration(stream->engine);
	position = IMFMediaEngine_GetCurrentTime(stream->engine);
	/* (NaN while unknown, infinity for a live stream) */
	info->duration = duration > 0.0 && duration < 1e12 ? duration : 0.0;
	info->position = position > 0.0 && position < 1e12 ? position : 0.0;
	info->has_audio = IMFMediaEngine_HasAudio(stream->engine) ? 1 : 0;
	info->has_video = IMFMediaEngine_HasVideo(stream->engine) ? 1 : 0;
	if (info->has_video && SUCCEEDED(IMFMediaEngine_GetNativeVideoSize(stream->engine, &width, &height))) {
		info->width = (int)width;
		info->height = (int)height;
	}
	if (InterlockedExchange(&stream->notify->ended, 0)) {
		info->ended = 1;
		stream->playing = 0;
		if (info->duration > 0.0) info->position = info->duration;
	}
}

static uint8_t* rvn_mf_take_frame(void* data, int* width, int* height) {
	rvn_mf_stream_t* stream = (rvn_mf_stream_t*)data;
	LONGLONG time = 0;
	DWORD w = 0, h = 0;
	MFVideoNormalizedRect source = { 0.0f, 0.0f, 1.0f, 1.0f };
	RECT target;
	WICRect all;
	IWICBitmapLock* lock = NULL;
	UINT size = 0, stride = 0;
	BYTE* bytes = NULL;
	uint8_t* pixels = NULL;
	if (!IMFMediaEngine_HasVideo(stream->engine)) return NULL;
	if (IMFMediaEngine_OnVideoStreamTick(stream->engine, &time) != S_OK) return NULL;
	if (FAILED(IMFMediaEngine_GetNativeVideoSize(stream->engine, &w, &h)) || w == 0 || h == 0) return NULL;
	if (!stream->bitmap || stream->bitmap_width != (int)w || stream->bitmap_height != (int)h) {
		if (stream->bitmap) IWICBitmap_Release(stream->bitmap);
		stream->bitmap = NULL;
		if (FAILED(IWICImagingFactory_CreateBitmap(g_rvn_mf.wic, w, h, &RVN_GUID_WICPixelFormat32bppBGRA, WICBitmapCacheOnLoad, &stream->bitmap)))
			return NULL;
		stream->bitmap_width = (int)w;
		stream->bitmap_height = (int)h;
	}
	SetRect(&target, 0, 0, (int)w, (int)h);
	if (FAILED(IMFMediaEngine_TransferVideoFrame(stream->engine, (IUnknown*)stream->bitmap, &source, &target, NULL))) return NULL;
	all.X = 0;
	all.Y = 0;
	all.Width = (INT)w;
	all.Height = (INT)h;
	if (FAILED(IWICBitmap_Lock(stream->bitmap, &all, WICBitmapLockRead, &lock))) return NULL;
	if (SUCCEEDED(IWICBitmapLock_GetStride(lock, &stride)) && SUCCEEDED(IWICBitmapLock_GetDataPointer(lock, &size, &bytes)) && bytes &&
		(size_t)stride * h <= (size_t)size + stride) {
		pixels = (uint8_t*)rvn_alloc((size_t)w * (size_t)h * 4);
		rvn_media_bgra_to_rgba(pixels, bytes, (int)w, (int)h, stride);
		*width = (int)w;
		*height = (int)h;
	}
	IWICBitmapLock_Release(lock);
	return pixels;
}

static const rvn_media_backend_t g_rvn_mf_backend = {
	"Media Foundation",
	rvn_mf_available,
	rvn_mf_open,
	rvn_mf_close,
	rvn_mf_play,
	rvn_mf_pause,
	rvn_mf_seek,
	rvn_mf_set_volume,
	rvn_mf_set_rate,
	rvn_mf_poll,
	rvn_mf_take_frame
};

const rvn_media_backend_t* rvn_media_system_backend(void) {
	return &g_rvn_mf_backend;
}
