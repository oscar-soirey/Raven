# Building Raven for the browser

Raven's Emscripten platform runs the C widget tree in WebAssembly and renders
with WebGL 2. The existing `web/` library embeds Chromium in a native
application; it is a separate feature and is disabled in browser builds.

## Build and run

Install and activate the official [Emscripten
SDK](https://emscripten.org/docs/getting_started/downloads.html).
The browser integration suite is verified with Emscripten 7.0.0.
`emcmake` comes with that SDK, not with CMake. On Linux/macOS, load the SDK
environment in each new terminal before building:

```sh
source /path/to/emsdk/emsdk_env.sh
```

Replace `/path/to/emsdk` with your SDK installation directory.

```sh
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release -DRAVEN_BUILD_EXAMPLES=ON
cmake --build build-web --parallel
python3 -m http.server 8000 --directory build-web/bin
```

Open `http://localhost:8000/index.html` for the example gallery. The browser
build includes all 14 portable desktop examples: hello, editor, glass,
notepad, animations, workspace, components, memory, studio, themes, overlay,
effects, plot and music. Each also has its own `raven_<name>.html` URL. The
additional `raven_web_demo.html` shows browser file import and persistence.
Serve the HTML, JavaScript, `.data` asset bundles
and WebAssembly files together; opening the HTML through `file://` is not
supported. Production hosts should serve `.wasm` as `application/wasm`.

CMake detects Emscripten before checking desktop operating systems.
`RAVEN_PLATFORM=emscripten` and `RAVEN_BACKEND_WEBGL=ON` are the defaults.
Vulkan, shared-library builds, native Chromium embedding, desktop tools,
and native tests are disabled by default. Portable examples are enabled for
top-level builds; `RAVEN_BUILD_EXAMPLES=OFF` omits the gallery and these
examples. Incompatible
Vulkan, shared-library, or Chromium settings produce a configuration error.
`RAVEN_BUILD_WEB_EXAMPLE=OFF` omits the standalone browser-services demo.
The native Chromium browser example and Vulkan/native-child OpenGL viewport
examples require desktop APIs and are excluded. XML, themes, shader sources
and other runtime assets are preloaded into each example’s virtual filesystem.
`RAVEN_BUILD_WEB_TESTS=ON` builds the browser integration fixture.
Use `-DRAVEN_BUILD_EXAMPLES=ON` when reusing a build directory created before
the gallery was enabled by default.

On Fedora/Nobara, install the build prerequisites and use the official SDK
(the distribution repositories may not provide Emscripten):

```sh
sudo dnf install git python3 cmake make
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install 7.0.0
./emsdk activate 7.0.0
source ./emsdk_env.sh
```

Return to the Raven checkout before running the build commands above. Source
`emsdk_env.sh` in each new terminal that builds browser applications.

## Application lifecycle

Include `Raven.h` and `raven/raven_emscripten.h`. Create widgets normally,
then call `Rweb_start()`. It returns immediately and schedules
`Rdraw_frame()` through the browser animation-frame loop. Calling `Rquit()`
or closing the last Raven window stops the loop and calls `Rshutdown()`.
`Rweb_start_ex(shutdown)` calls your `shutdown` function instead, which frees
the application's own state and then calls `Rshutdown()` itself.

The loop asks the browser for an animation frame only while there is
something to do, as `Rwait_frame(-1)` sleeps on the desktop: input, layout,
a redraw (`Rwindow_request_redraw`, `Rwindow_set_continuous_redraw`), an
animation, a posted task. Until the next `Rtimer` is due it waits with a
browser timeout, and otherwise not at all: an idle page uses no CPU. It wakes
on DOM input, resizing, visibility and focus changes, the on-screen keyboard,
the completion of image fetches, file imports, IndexedDB and clipboard
requests, `Rtask_post` and `Rwake_up`, and any Raven API call, including
calls an application makes from its own JavaScript or browser callbacks.
`Module.ravenLoop.frames` counts the frames it ran (the browser tests check
that it stands still on an idle page).

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
`Rdraw_frame()` themselves and handle shutdown themselves; such a loop
(`emscripten_set_main_loop`) keeps running on every animation frame.
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
file/text drops enter the same event paths as desktop input. The
accessibility tree is mirrored as ARIA nodes with names, roles, state,
text/range editing, actions and live announcements. Browser contrast and reduced-motion preferences are exposed
through Raven's accessibility settings. Actual screen-reader behavior and
mobile virtual keyboards should also be checked on the devices you ship to.

### Text input and on-screen keyboards

The keyboard types into a hidden element placed at Raven's caret, where input
method candidate windows then appear:

- **A `TextInput`**: a textarea that holds the text around the caret, up to
  512 UTF-16 units on each side of the selection, and the selection itself.
  Predictive text, autocorrection and the composition of on-screen keyboards
  see the real words. Their edits (`insertText`, `insertReplacementText`,
  `deleteContentBackward`, `deleteWordBackward`, ...) are applied as one
  replaced range of the field; an insertion at the selection is typed as
  text events, as a hardware keyboard types. A selection moved by the
  keyboard (cursor keys of the keyboard, its menu) moves Raven's, and
  Raven's caret, selection and text are copied back after every change.
  The composition is drawn in the field, underlined; a word composed again
  (Android keyboards) leaves the field while it is composed.
- **A password `TextInput`**: an `<input type="password">`
  (`autocomplete="current-password"`), so password managers can fill it.
  It never keeps the text: every input goes to Raven and the element is
  emptied.
- **A `TextEditor` or `SpinBox`**: the core does not expose their text yet, so
  the textarea stays empty and receives keys and typed text as before,
  with the composition drawn in the field.

The attributes follow the field: `inputmode` and `autocomplete` from
`Rtext_input_set_input_type` (email, numbers, decimal, phone, URL, search),
`enterkeyhint` from `Rtext_input_set_return_key`, `readonly` for a read-only
field. Spelling suggestions, autocorrection and automatic capitals are on for
plain text and search fields, off for passwords, email, numbers, URLs, the
`TextEditor` (often code) and the `SpinBox`. The elements use a 16 px font so
that iOS does not zoom into the page when they take the focus. Shortcuts
that Raven applies to the field itself (select all, undo, redo) do not also
reach the textarea.

When an on-screen keyboard shrinks the visual viewport
(`window.visualViewport`) while a field has the focus, the part of the canvas
it covers is reported as the keyboard height: `Rwindow_get_keyboard_area`,
the keyboard callback, the layout above it, and the focused field scrolled
into view, as on iOS and Android. A pinch zoom is not taken for a keyboard.
`Rwindow_get_safe_area` reports the CSS safe-area insets
(`env(safe-area-inset-*)`) where the canvas reaches into them; the page needs
`viewport-fit=cover` in its viewport meta tag, as the demo shell has. On iOS
Safari the page may still scroll when the keyboard opens, since the layout
viewport does not shrink there.

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
to the virtual filesystem. Native directory watching is unavailable, so XML
live reload is off by default in the browser (it would poll the files and keep
an idle page awake); `RAVEN_LIVE_RELOAD=1` or `Rxml_set_live_reload(R_TRUE)`
turns it on.

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
Tests also open every gallery example, verify bundled assets load and visible
UI renders, then exercise pointer movement, wheel input and resizing.
Tests check that an idle page runs no frame for two seconds and that input
and a due timer wake it; that the textarea holds the text and selection of a
`TextInput` and that replacements, deletions, insertions in the middle,
recomposed words and UTF-16 offsets after an emoji reach Raven (as
`beforeinput`/`input` and composition events dispatched to the textarea); that
a password field uses a password input that stays empty and an email field
gets its keyboard hints; and that a shrunken visual viewport is reported as
the keyboard height. `tests/web/platform-events.cjs` runs the same adapter
code without a browser, and `test_web_input` checks the UTF-16 offsets, the
text window and the field attributes natively.
Tests exercise actual browser input, composition, paste, ARIA state, all
built-in effects, custom effects, GPU capture, HTTP images, multiple windows,
file import/export, IndexedDB across reloads, high DPI, resizing, context
loss/restoration and shutdown cleanup. They do not replace manual mobile
keyboard and assistive-technology checks. Headless Firefox sends a native
file-picker cancellation before Playwright supplies a selected file; the
test harness suppresses that automation artifact. Production cancellation
handling remains enabled.

## Published previews

The `Web demos and website` workflow builds Release versions of all 14 portable
examples and the browser-services demo. It tests the packaged gallery in
Chromium and uploads `raven-web-demos`, a downloadable static-site artifact,
on pull requests as well as main-branch builds. Extract that artifact and serve
it with `python3 -m http.server 8000` to preview it locally.

On `main`, the workflow also publishes the existing `docs/` website with the
gallery at `demos/` using GitHub Pages. A repository administrator must first
select **Settings → Pages → Build and deployment → Source → GitHub Actions**
instead of the legacy `main` / `docs` source. No generated WebAssembly binaries
need to be committed. Forks can use the same workflow and their own Pages URL;
pull requests only build and test, and do not deploy.

The upstream gallery URL is <https://oscar-soirey.github.io/Raven/demos/> once
this workflow is merged and Pages is configured. The browser-services demo is
available at `demos/raven_web_demo.html`.
