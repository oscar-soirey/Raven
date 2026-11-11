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
static int g_loop;

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

static void rvn_web_frame(void) {
    if (!Rdraw_frame()) {
        emscripten_cancel_main_loop();
        g_loop = R_FALSE;
        Rshutdown();
    }
}
void Rweb_start(void) {
    RVN_API_GUARD_VOID();
    if (g_loop) return;
    g_loop = R_TRUE;
    emscripten_set_main_loop(rvn_web_frame, 0, 0);
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
    if (g_loop) emscripten_cancel_main_loop();
    g_loop = R_FALSE;
    while (g_requests) {
        rvn_web_request_t* request = g_requests;
        g_requests = request->next;
        rvn_free(request);
    }
}
