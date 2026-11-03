# Raven for Bun

One TypeScript module, `src/raven.ts`, over `bun:ffi`: no dependency, no build
step. It is written by `tools/bindgen/gen_bun.py` from the C headers, so it has
the whole API under the same names. Do not edit it.

Build Raven as a shared library, then point the binding at it:

    cmake -S . -B build-shared -DCMAKE_BUILD_TYPE=Release -DRAVEN_BUILD_SHARED=ON
    cmake --build build-shared --parallel

    RAVEN_LIBRARY=/path/to/libraven.so bun run src/example.ts

```ts
import * as r from "./raven";

r.Rinit(r.R_BACKEND_VULKAN);
const window = r.Rwindow_create_from_xml_file("main.xml");
r.Raction_bind("hello.clicked", () => {
	r.Rlabel_set_text(r.Rwidget_find(window, "status"), "Clicked");
});
await r.runLoop();
r.Rshutdown();
```

## What differs from C

- Texts are strings on both sides.
- A JavaScript function can be given wherever C takes a callback. It stays
  alive as long as the program. An exception it throws is printed, and the
  callback returns 0, because an exception must not cross the frames of C.
- A parameter that C writes through is returned instead:
  `const [x, y, width, height] = r.Rwidget_get_rect(widget)`.
- The size after a text or bytes is left out, and so is the count after a list.
- A structure is a class with an accessor per field: `new r.RInitDesc()`. One
  received by a callback reads the memory of the library, so it is only valid
  during the call.

## The loop

`Rwait_frame(-1)` sleeps inside C, so timers and promises wait with it.
`await r.runLoop()` keeps them running beside the interface: `Rwait_frame` still
wakes at once for an event of a window, and JavaScript gets its turn at least
every `interval_seconds` (1/60 by default).

## Threads

A `JSCallback` may only be called on the JavaScript thread: called from another
one it never runs, and `Rshutdown` then waits for it forever. So:

- `Rinit` and a new `RInitDesc` leave the render thread off, because errors are
  reported on the thread that raises them. To turn it on, set
  `disable_render_thread = 0` on a desc given to `Rinit_ex`, and do not give
  `Rerror_set_callback` a JavaScript function: errors of the renderer would
  reach it from the render thread.
- `Rparallel_for` and `Rviewport_set_draw_callback` only take the address of a
  native function, since their callbacks must run on other threads at once.
- A function given to `Rtask_run_async` runs later on the JavaScript thread, not
  on a worker: use a `Worker` for long work.
- `RAllocator` and `RHostPlatformDesc` hold addresses of native functions.

Raven belongs to the thread that starts it: call it from that thread only, not
from a `Worker`.

## One executable

`bun build --compile` can hold the library and the interface too, so that the
program needs nothing beside it. The binding opens the library as soon as it is
imported, so its path must be set first, in a module of its own:

```ts
// native.ts
import library from "./libraven.so" with { type: "file" };
process.env.RAVEN_LIBRARY ??= library;
```

```ts
// app.ts
import "./native";
import * as r from "./raven";
import ui from "./main.xml" with { type: "text" };

r.Rinit(r.R_BACKEND_VULKAN);
r.Rwindow_create_from_xml(String(ui));
await r.runLoop();
r.Rshutdown();
```

    bun build --compile --no-compile-autoload-dotenv app.ts --outfile app

The executable is for the platform of the library it holds. It still needs what
the library needs from the system (libX11 and a Vulkan driver on Linux), and Bun
unpacks the library to a file in the temporary folder when it starts.

TypeScript needs `declare module "*.so" { const path: string; export default path; }`.
`String(ui)` is there because bun-types describe `*.xml` as parsed, which is true
of an import without `type: "text"`.

## Tests

    RAVEN_LIBRARY=/path/to/libraven.so bun run src/example.ts --test

runs without a window system and without a GPU, as `ctest` does (`binding_bun`).
`bun run typecheck` needs `bun install` first.
