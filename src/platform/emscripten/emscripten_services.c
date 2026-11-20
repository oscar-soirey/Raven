#include "raven/raven_emscripten.h"
#include "../../core/context.h"
#include <emscripten.h>

typedef struct rvn_web_request_t {
    uint32_t id;
    RWebCallback callback;
    void* user;
    struct rvn_web_request_t* next;
} rvn_web_request_t;
static rvn_web_request_t* g_requests;
static uint32_t g_request_id;

/*
 * The loop of Rweb_start. It asks the browser for an animation frame only
 * while there is something to do (rvn_wait_timeout: input, layout, a redraw,
 * an animation, a task), sleeps with a timeout until the next timer, and
 * sleeps without limit otherwise. Whatever may change the interface between
 * frames asks for one again (rvn_web_loop_poke): DOM events, API calls, the
 * callbacks of fetches, IndexedDB, the file picker and the clipboard.
 */
static int g_loop;
static int g_in_frame;
static int g_frame_asked;
static RWebShutdownCallback g_loop_shutdown;

/* delay <= 0: an animation frame; otherwise a timer that asks for one after `delay` seconds. */
EM_JS(void, rvn_web_loop_ask, (double delay), {
    const loop = Module.ravenLoop || (Module.ravenLoop = {frame:0, timer:0, frames:0});
    if (delay > 0) {
        if (loop.frame) return;
        if (loop.timer) clearTimeout(loop.timer);
        loop.timer = setTimeout(() => { loop.timer = 0; _rvn_web_loop_poke(); },
                                Math.ceil(delay * 1000) + 1);
        return;
    }
    if (loop.timer) { clearTimeout(loop.timer); loop.timer = 0; }
    if (!loop.frame) loop.frame = requestAnimationFrame(() => {
        loop.frame = 0; loop.frames++; _rvn_web_loop_frame();
    });
})

EM_JS(void, rvn_web_loop_cancel, (void), {
    const loop = Module.ravenLoop;
    if (!loop) return;
    if (loop.frame) cancelAnimationFrame(loop.frame);
    if (loop.timer) clearTimeout(loop.timer);
    loop.frame = 0; loop.timer = 0;
})

#if defined(__EMSCRIPTEN_PTHREADS__)
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
static void rvn_web_loop_poke_main(void* unused) {
    (void)unused;
    rvn_web_loop_poke();
}
#endif

EMSCRIPTEN_KEEPALIVE void rvn_web_loop_poke(void) {
#if defined(__EMSCRIPTEN_PTHREADS__)
    /* (Rwake_up and Rtask_post from a worker: the loop belongs to the main thread) */
    if (!emscripten_is_main_runtime_thread()) {
        emscripten_proxy_async(emscripten_proxy_get_system_queue(),
                               emscripten_main_runtime_thread_id(),
                               rvn_web_loop_poke_main, NULL);
        return;
    }
#endif
    if (!g_loop || g_in_frame || g_frame_asked) return;
    g_frame_asked = R_TRUE;
    rvn_web_loop_ask(0.0);
}

static void rvn_web_loop_end(void) {
    RWebShutdownCallback shutdown = g_loop_shutdown;
    g_loop = R_FALSE;
    g_frame_asked = R_FALSE;
    g_loop_shutdown = NULL;
    rvn_web_loop_cancel();
    if (shutdown) shutdown();
    else Rshutdown();
}

EMSCRIPTEN_KEEPALIVE void rvn_web_loop_frame(void) {
    double wait;
    int running;
    if (!g_loop) return;
    g_frame_asked = R_FALSE;
    g_in_frame = R_TRUE;
    running = Rdraw_frame();
    g_in_frame = R_FALSE;
    if (!g_loop) return;
    if (!running) {
        rvn_web_loop_end();
        return;
    }
    wait = rvn_wait_timeout(-1.0);
    if (wait == 0.0) {
        g_frame_asked = R_TRUE;
        rvn_web_loop_ask(0.0);
    } else if (wait > 0.0) {
        rvn_web_loop_ask(wait);
    }
    /* (negative: nothing to do until something pokes the loop) */
}

static int rvn_web_services_ready(const char* function) {
    if (!rvn_api_check(function)) return R_FALSE;
    if (!g_rvn.platform ||
        g_rvn.platform->system != R_WINDOW_SYSTEM_EMSCRIPTEN) {
        rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR,
                  "%s requires the browser platform", function);
        return R_FALSE;
    }
    return R_TRUE;
}

static uint32_t rvn_web_request(RWebCallback callback, void* user) {
    rvn_web_request_t* request = rvn_alloc(sizeof(*request));
    request->id = ++g_request_id;
    if (!request->id) request->id = ++g_request_id;
    request->callback = callback; request->user = user;
    request->next = g_requests; g_requests = request;
    return request->id;
}

EMSCRIPTEN_KEEPALIVE void rvn_web_service_done(uint32_t id, int success,
                                               const char* result) {
    rvn_web_request_t** link = &g_requests;
    rvn_web_request_t* request;
    while (*link && (*link)->id != id) link = &(*link)->next;
    if (!*link) return;
    request = *link; *link = request->next;
    if (request->callback)
        request->callback(success, result ? result : "", request->user);
    rvn_free(request);
    rvn_web_loop_poke();
}

EM_JS(void, rvn_web_import, (uint32_t id, const char* accept), {
    const generation = Module.ravenLifecycle;
    const input = document.createElement('input');
    input.type = 'file'; input.accept = UTF8ToString(accept);
    input.hidden = true; document.body.appendChild(input);
    if (!generation.inputs) generation.inputs = new Set();
    generation.inputs.add(input);
    let done = false;
    const finish = (ok, value) => {
        if (done) return; done = true; input.remove();
        generation.inputs.delete(input);
        if (Module.ravenLifecycle !== generation) return;
        const p = stringToNewUTF8(value);
        _rvn_web_service_done(id, ok, p); _free(p);
    };
    input.addEventListener('cancel', () => finish(0, "Cancelled"), {once:true});
    input.addEventListener('change', async () => {
        const file = input.files[0];
        if (!file) return finish(0, "Cancelled");
        if (file.size > 33554432) return finish(0, "File exceeds 32 MiB");
        try {
            const data = new Uint8Array(await file.arrayBuffer());
            if (Module.ravenLifecycle !== generation) { input.remove(); return; }
            FS.mkdirTree('/uploads');
            const name = file.name.replace(/[^a-zA-Z0-9._-]/g, '_') || 'file';
            const path = '/uploads/' + id + '-' + name;
            FS.writeFile(path, data); finish(1, path);
        } catch (e) { finish(0, String(e)); }
    }, {once:true});
    input.click();
})

EM_JS(int, rvn_web_download, (const void* data, size_t size, const char* name), {
    try {
        const blob = new Blob([HEAPU8.slice(data, data + size)],
                              {type:'application/octet-stream'});
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url; a.download = UTF8ToString(name);
        document.body.appendChild(a); a.click(); a.remove();
        setTimeout(() => URL.revokeObjectURL(url), 1000);
        return 1;
    } catch (_) { return 0; }
})

EM_JS(void, rvn_web_persist, (uint32_t id, int mount), {
    const generation = Module.ravenLifecycle;
    const done = (error) => {
        if (Module.ravenLifecycle !== generation) return;
        generation.persistBusy = false;
        if (!error && mount) generation.persistReady = true;
        const p = stringToNewUTF8(error ? String(error) : '/persistent');
        _rvn_web_service_done(id, error ? 0 : 1, p); _free(p);
    };
    if (generation.persistBusy) {
        const p = stringToNewUTF8('A persistence operation is already running');
        _rvn_web_service_done(id, 0, p); _free(p); return;
    }
    if (mount && generation.persistReady) return done(null);
    if (!mount && !generation.persistReady) return done('Mount /persistent first');
    generation.persistBusy = true;
    try {
        if (mount) {
            FS.mkdirTree('/persistent');
            if (!Module.ravenPersistentMounted) {
                FS.mount(IDBFS, {}, '/persistent');
                Module.ravenPersistentMounted = true;
            }
        }
        FS.syncfs(!!mount, done);
    } catch (e) { done(e); }
})

EM_JS(void, rvn_web_clipboard_read, (uint32_t id), {
    const generation = Module.ravenLifecycle;
    const done = (ok, value) => {
        if (Module.ravenLifecycle !== generation) return;
        const p = stringToNewUTF8(value);
        if (ok) _rvn_web_clipboard(p);
        _rvn_web_service_done(id, ok, p); _free(p);
    };
    if (!navigator.clipboard) return done(0, 'Clipboard unavailable');
    navigator.clipboard.readText().then(text => done(1, text),
                                       error => done(0, String(error)));
})

void Rweb_start(void) {
    Rweb_start_ex(NULL);
}
void Rweb_start_ex(RWebShutdownCallback shutdown) {
    RVN_API_GUARD_VOID();
    if (g_loop) return;
    g_loop = R_TRUE;
    g_loop_shutdown = shutdown;
    g_frame_asked = R_TRUE;
    rvn_web_loop_ask(0.0);
}
void Rweb_import_file(const char* accept, RWebCallback callback, void* user) {
    if (!rvn_web_services_ready(__func__)) return;
    rvn_web_import(rvn_web_request(callback, user), accept ? accept : "");
}
int Rweb_download_file(const char* path, const char* filename) {
    size_t size = 0;
    char* data;
    int result;
    if (!rvn_web_services_ready(__func__)) return R_FALSE;
    data = rvn_read_file(path, &size);
    if (!data) return R_FALSE;
    result = rvn_web_download(data, size, filename ? filename : "download");
    rvn_free(data);
    return result;
}
void Rweb_mount_persistent(RWebCallback callback, void* user) {
    if (!rvn_web_services_ready(__func__)) return;
    rvn_web_persist(rvn_web_request(callback, user), R_TRUE);
}
void Rweb_sync_persistent(RWebCallback callback, void* user) {
    if (!rvn_web_services_ready(__func__)) return;
    rvn_web_persist(rvn_web_request(callback, user), R_FALSE);
}
void Rweb_read_clipboard(RWebCallback callback, void* user) {
    if (!rvn_web_services_ready(__func__)) return;
    rvn_web_clipboard_read(rvn_web_request(callback, user));
}
void rvn_web_services_shutdown(void) {
    if (g_loop) rvn_web_loop_cancel();
    g_loop = R_FALSE;
    g_frame_asked = R_FALSE;
    g_loop_shutdown = NULL;
    while (g_requests) {
        rvn_web_request_t* request = g_requests;
        g_requests = request->next;
        rvn_free(request);
    }
}
