# Phase 0b: does the Raven core port to WebAssembly and iOS?

The core (widgets, layout, XML, fonts, text editor, themes, the job system and threads) was built for WebAssembly and for
the iOS simulator with the null platform and no GPU backend, and its headless tests were run in Node and in the simulator.
**Both pass: 15 of 15 programs, 0 failures, on each.** Results feed `docs/SOKOL_BACKEND_PLAN.md` (section 14).

## Web (WebAssembly), tested

Toolchain: Emscripten 3.1.6 (Ubuntu 24.04 package, old) and Node 22 in the development sandbox, and **the maintainer's Mac with
Emscripten 6.0.13 (Homebrew) and a current Node: the same 15 of 15 programs pass with 0 failures**, so the result does not
depend on the old toolchain.

| Question | Answer |
|---|---|
| Does the library compile? | Yes, all 88 source files. One change was needed: the URL download starts `curl` with `fork`/`exec` (`src/core/fs.c`); a build for platforms without subprocesses (`RVN_NO_SUBPROCESS`, set for Emscripten and iOS) now stubs it. `Ropen_url` uses `window.open` on the web; the native file dialogs (`widgets/dialogs.c`) are stubbed the same way. |
| Do the tests pass? | **With threads: 15 of 15 programs, 0 failures** (about 24,500 checks: layout, widgets, XML, text editor, themes, markdown, tools, dialogs, shell, memory, animation, locale, dock tearing, and `test_threads`: render thread, worker pool, parallel painting). |
| Without threads? | Everything passes except the checks that need a thread: the render thread cannot start (Raven falls back to rendering on the UI thread, which is by design, and says so in a warning the test harness counts), the worker count, and the parallel paint jobs. |
| Speed (12,042 widgets, `raven_benchmark`) | repaint: **1.70 ms** threaded wasm, 2.09 ms single-threaded wasm, 1.59 ms native; first layout 5.0 / 3.9 / 2.4 ms, first paint 12.2 / 10.7 / 4.4 ms (glyph rasterization). About native speed in steady state, 2 to 3 times slower at start-up. |
| Does the Sokol GLES3 backend compile for the web? | Yes, unchanged (`-DRAVEN_SOKOL_GLES3`). Its native part (`sg_egl.c`, a context and a window surface) does not apply: a browser needs its own (a WebGL context of the canvas). Not run: there is no browser here. |

Not part of this probe, still to do for the web: the browser-owned frame loop (`Rrun`), in-surface windows, touch, the DOM text
input, a canvas platform, asynchronous download, and a GPU backend running in a real browser. Threads in a browser need the page
to be cross-origin isolated (COOP/COEP headers); the single-threaded build has no such requirement.

Toolchain notes: this old Emscripten does not link a `main(int, char**)` (it finds no `main`, and the program does nothing):
`tests/test_dock_tearing.c` and `tests/benchmark.c` need a `main(void)` copy (the script does this). Node 22 needs
`-sWASM_ASYNC_COMPILATION=0` with it. A current emsdk (6.0.13) ran the script fine; the script still uses the `main(void)` copy for `test_dock_tearing`, so whether
the new Emscripten links `main(int, char**)` is not known. Emscripten 6 suggests `-sGROWABLE_ARRAYBUFFERS=2` instead of plain
memory growth for threaded builds (slower non-wasm paths otherwise), to try for the real web build. (`ravenc` and the XML compiler no longer exist upstream.)

```sh
./probe_wasm.sh threads        # or: nothreads
```

## iOS (simulator), tested by the maintainer on a Mac (Xcode, iPhone 17 simulator)

**All 88 files compile for the iOS simulator (arm64) and the 15 headless test programs pass inside the simulator, 0 failures**
(about 24,500 checks, threads included). Two things needed a change, both found by running the script:
`ravenc` is never an app bundle (CMake makes every executable one on iOS), and the library links CoreFoundation on every
Apple platform (the system language of `locale.c`; the Cocoa platform used to pull it in on macOS, the null platform does not).

Before that, a static scan found the places that cannot compile for iOS (its SDK marks `fork` and `exec*` unavailable): `Ropen_url`,
the `curl` download and the native file dialogs. They are now behind `RVN_NO_SUBPROCESS`; forcing the macro on in a Linux build
(`-DCMAKE_C_FLAGS=-DRVN_NO_SUBPROCESS`) passes the whole suite (28 of 28). Nothing else the scan looked for turned up
(`system`, `popen`, `dlopen`, `posix_spawn`, `waitpid` outside the platform folders).

`probe_ios.sh` builds the library for the iOS simulator and runs the same headless tests in a booted simulator:

```sh
xcrun simctl list devices available   # pick a simulator that this Xcode has
xcrun simctl boot "iPhone 17"          # once; use the name from the list
./probe_ios.sh
```

## Web build and browser test

`web_browser_test.js` runs the real web build (`examples/hello` with the web platform and the Sokol/WebGL 2 backend) in headless
Chromium through Playwright: it checks that the canvas follows the page (in physical pixels), that a mouse click and a touch tap
reach the button, and that nothing is reported to the console.

    emcmake cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release -DRAVEN_BACKEND_VULKAN=OFF -DRAVEN_BACKEND_SOKOL=ON \
      -DRAVEN_BUILD_EXAMPLES=ON -DRAVEN_BUILD_TOOLS=OFF -DRAVEN_BUILD_TESTS=OFF
    cmake --build build-web
    node spikes/portability_probe/web_browser_test.js build-web/bin

To look at it yourself: `python3 -m http.server -d build-web/bin` and open `http://localhost:8000/raven_hello.html`.

