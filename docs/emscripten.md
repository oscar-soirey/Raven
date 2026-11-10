# Building Raven for the browser

Raven's Emscripten platform runs the C widget tree in WebAssembly and renders
with WebGL 2. The existing `web/` library embeds Chromium in a native
application; it is a separate feature and is disabled in browser builds.

## Build and run

Install and activate the official [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html).
The browser integration suite is verified with Emscripten 7.0.0.
`emcmake` comes with that SDK, not with CMake. On Linux/macOS, load the SDK
environment in each new terminal before building:

```sh
source /path/to/emsdk/emsdk_env.sh
```

Replace `/path/to/emsdk` with your SDK installation directory.

```sh
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web --parallel
python3 -m http.server 8000 --directory build-web/bin
```

Open `http://localhost:8000/raven_web_demo.html`. Serve the HTML, JavaScript
and WebAssembly files together; opening the HTML through `file://` is not
supported. Production hosts should serve `.wasm` as `application/wasm`.

CMake detects Emscripten before checking desktop operating systems.
`RAVEN_PLATFORM=emscripten` and `RAVEN_BACKEND_WEBGL=ON` are the defaults.
Vulkan, shared-library builds, native Chromium embedding, desktop tools,
desktop examples, and native tests are disabled by default. Incompatible
Vulkan, shared-library, or Chromium settings produce a configuration error.
`RAVEN_BUILD_WEB_EXAMPLE=OFF` omits the demo.
`RAVEN_BUILD_WEB_TESTS=ON` builds the browser integration fixture.

## Application lifecycle

Include `Raven.h` and `raven/raven_emscripten.h`. Create widgets normally,
then call `Rweb_start()`. It returns immediately and schedules
`Rdraw_frame()` through the browser animation-frame loop. Calling `Rquit()`
or closing the last Raven window stops the loop and calls `Rshutdown()`.

```c
#include <Raven.h>
#include <raven/raven_emscripten.h>

int main(void) {
    if (!Rinit(0)) return 1;
    Rid window = Rwindow_create("Hello", 800, 600, 0);
    Rwindow_set_content(window, Rlabel_create("Hello from Raven"));
    Rwindow_show(window);
    Rweb_start();
    return 0;
}
```

Do not put a native blocking event loop or `Rshutdown()` after
`Rweb_start()`. Applications that already own a browser loop may call
`Rdraw_frame()` themselves and handle shutdown themselves.
`Rpoll_events()` and `Rwait_events()` do not block in this platform.
An unfinished asynchronous texture cannot be waited on synchronously:
`Rtexture_wait()` reports an error; poll `Rtexture_is_ready()` instead.

The browser UI and renderer run on the main thread. Without Emscripten
pthreads, jobs execute inline. The default build needs no cross-origin
isolation headers. A custom pthread-enabled application must arrange its
worker pool and COOP/COEP hosting headers; the renderer still stays on the
browser main thread.

## Embedding and windows

Supply a canvas with `id="canvas"`, or assign a canvas to `Module.canvas`
before startup. Give it a nonzero CSS size. The demo shell fills the page.
Raven owns the backing pixel dimensions and follows CSS resizing and
device-pixel ratio changes.

The first Raven window uses this canvas. Additional windows use floating
canvases inside the same page. Positioning, minimum sizes, centering, opacity, stacking, hiding,
maximizing, minimizing, and custom-header move/resize operate inside the
page. The first canvas follows the host page's layout; it is not draggable.
A browser window/tab is never opened for each Raven window.

All windows share a hidden WebGL canvas, textures and programs. Presentation
copies its rendered image into each visible canvas. Applications must not
replace these canvases' contexts or draw into Raven's presentation canvases.
For external drawing use a Viewport callback, where the shared WebGL context
is current and viewport/scissor are configured, or update a Raven texture.
Native child OS viewports are unavailable.

Mouse, keyboard, wheel, touch, pen, text composition, clipboard events and
file/text drops enter the same event paths as desktop input. A DOM textarea
anchors composition at Raven's caret. The accessibility tree is mirrored as
ARIA nodes with names, roles, state, text/range editing, actions and live
announcements. Browser contrast and reduced-motion preferences are exposed
through Raven's accessibility settings. Actual screen-reader behavior and
mobile virtual keyboards should also be checked on the devices you ship to.

## Rendering and effects

The renderer supports Raven's analytic primitives, glyph atlases, textures,
clipping, premultiplied blending, backdrop capture, blur, glass, pixel capture,
and Viewport callbacks. Reference shaders are adapted by
`python3 src/render/webgl/generate_shaders.py`; commit the generated headers
when changing the reference shaders.

All built-in effects work in WebGL. WebGL cannot load SPIR-V; custom effects
use `Rweb_shader_create(source, flags)`. Supply only the
`vec4 raven_effect(RavenEffect fx)` function and any helper functions.
Raven supplies the GLSL ES preamble and the effect ABI described in
`include/raven/shaders/raven_effect.glsl`. Destroy the result with
`Rshader_destroy()`.

On WebGL context restoration Raven rebuilds its GPU resources and calls
the existing device-reset callback. Downloaded images retain their encoded
data for recovery. CPU-provided large textures follow Raven's existing
recovery contract: applications may need to repopulate them in that callback.

## Assets, files, networking and clipboard

Emscripten's virtual filesystem holds application files. Bundle assets with
an application link option such as `--preload-file assets@/assets`, use
`Rfile_register_memory()`, or import user files. Bundle/register additional
fonts for scripts beyond the embedded fonts; browser system font files
cannot be enumerated like desktop font directories.

`Rtexture_create_from_file_async()` accepts HTTP(S) URLs. Browser fetches
are asynchronous and obey CORS, mixed-content and other browser policies.
Downloads are limited to 32 MiB and a 20-second timeout. Local paths refer
to the virtual filesystem. Native directory watching is unavailable.

Browser-specific services use `RWebCallback`; result strings are borrowed
during the callback. Requests are cancelled without callback delivery when
Raven shuts down.

| API | Behavior |
| --- | --- |
| `Rweb_import_file` | File picker; returns an imported path under `/uploads`, up to 32 MiB. |
| `Rweb_download_file` | Downloads a virtual file through the browser. |
| `Rweb_mount_persistent` | Mounts and loads IndexedDB storage at `/persistent`. |
| `Rweb_sync_persistent` | Saves changes under `/persistent`; wait for its callback before leaving the page. |
| `Rweb_read_clipboard` | Asynchronously reads text and updates Raven's paste cache. |

Mount persistence before reading saved files. Keep only one persistence
operation in flight. File pickers, downloads, external links and clipboard
access may require a user gesture, permission or a secure context.
Ordinary browser paste events work directly; synchronous clipboard reads
return Raven's most recently copied or pasted text.
`Ropen_url()` opens HTTP(S) or mail links with browser opener isolation;
its return value means the request was submitted, since popup policy may
still block it.

Desktop tray icons, taskbar integration, system menu bars, native browser
embedding, and OS backdrop materials have no browser equivalent. Optional
platform capabilities report unavailable; Raven's own menus, dialogs and
in-application materials remain usable.

## Browser integration tests

```sh
emcmake cmake -S . -B build-web -DRAVEN_BUILD_WEB_TESTS=ON
cmake --build build-web --parallel
python3 -m http.server 8765 --directory build-web/bin
# In another terminal:
cd tests/web
npm install
npx playwright install chromium firefox webkit
npm test
RAVEN_BROWSER_ENGINE=firefox npm test
RAVEN_BROWSER_ENGINE=webkit npm test
```

`RAVEN_WEB_URL` changes the test URL and `RAVEN_BROWSER` selects an installed
browser executable. `RAVEN_WEB_BYPASS_PROXY=1` bypasses desktop proxy
settings for the loopback test server. `RAVEN_WEB_SCREENSHOT` changes the screenshot destination.
Tests exercise actual browser input, composition, paste, ARIA state, all
built-in effects, custom effects, GPU capture, HTTP images, multiple windows,
file import/export, IndexedDB across reloads, high DPI, resizing, context
loss/restoration and shutdown cleanup. They do not replace manual mobile
keyboard and assistive-technology checks. Headless Firefox sends a native
file-picker cancellation before Playwright supplies a selected file; the
test harness suppresses that automation artifact. Production cancellation
handling remains enabled.
