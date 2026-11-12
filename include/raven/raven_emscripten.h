/* Browser-specific lifecycle and asynchronous services. Emscripten builds only. */
#ifndef RAVEN_EMSCRIPTEN_H
#define RAVEN_EMSCRIPTEN_H
#include "raven_core.h"
#ifdef __cplusplus
extern "C" {
#endif
/** Callback strings are borrowed and valid only for the callback.
 * success is false on rejection, cancellation, or unavailable browser support.
 * Pending callbacks are cancelled without delivery when Raven shuts down. */
typedef void (*RWebCallback)(int success, const char* result, void* user_data);
/** Creates a WebGL effect from a GLSL ES function:
 * vec4 raven_effect(RavenEffect fx) { return fx.color; }
 * Do not include version/header directives. Raven supplies the effect ABI.
 * Destroy with Rshader_destroy. SPIR-V is not accepted by WebGL. */
RAVEN_API Rid Rweb_shader_create(const char* source, Ruint flags);
/** Starts a browser animation-frame loop and returns immediately.
 * The loop calls Rdraw_frame; when Raven quits it cancels itself and shuts down.
 * Do not put Rshutdown after this call. Custom loops may call Rdraw_frame instead. */
RAVEN_API void Rweb_start(void);
/** Opens a browser file picker. A successful result is a path in /uploads.
 * accept is a browser accept filter, e.g. ".xml,image/png". Maximum size: 32 MiB. */
RAVEN_API void Rweb_import_file(const char* accept, RWebCallback callback,
                               void* user_data);
/** Downloads a virtual filesystem file through the browser. Call from a gesture. */
RAVEN_API int Rweb_download_file(const char* path, const char* filename);
/** Mounts /persistent with IDBFS and loads saved data asynchronously.
 * Call once per library lifecycle, before reading persistent files. */
RAVEN_API void Rweb_mount_persistent(RWebCallback callback, void* user_data);
/** Saves /persistent to IndexedDB asynchronously. Mount must have completed. */
RAVEN_API void Rweb_sync_persistent(RWebCallback callback, void* user_data);
/** Reads the browser clipboard asynchronously. May require a gesture/permission.
 * On success result contains text and Raven's synchronous paste cache is updated. */
RAVEN_API void Rweb_read_clipboard(RWebCallback callback, void* user_data);
#ifdef __cplusplus
}
#endif
#endif
