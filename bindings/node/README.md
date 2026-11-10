# Raven for Node.js

An addon written against Node-API (`src/raven_napi.c`) and an ES module over it
(`src/raven.js`, `src/raven.d.ts`): no dependency, no `node-gyp` at run time.
They are written by `tools/bindgen/gen_napi.py` from the C headers, so they have
the whole API under the same names. Do not edit them.

Node-API is ABI-stable: one `.node` per platform works across versions of
Node.js, and the same file loads in Bun and in Deno. It has the same surface as
`bindings/bun`: a program written against one runs on the other.

Build Raven as a shared library, the addon beside it, then point the binding at
both:

    cmake -S . -B build-shared -DCMAKE_BUILD_TYPE=Release -DRAVEN_BUILD_SHARED=ON
    cmake --build build-shared --parallel
    cmake -S bindings/node -B bindings/node/build -DCMAKE_BUILD_TYPE=Release
    cmake --build bindings/node/build

    RAVEN_LIBRARY=/path/to/libraven.so node src/example.mjs

The addon is looked for in `RAVEN_ADDON`, beside `raven.js`, then in `build/`; the
library in `RAVEN_LIBRARY`, beside `raven.js`, then where the system looks.

```js
import * as r from "./raven.js";

r.Rinit(r.R_BACKEND_VULKAN);
const window = r.Rwindow_create_from_xml_file("main.xml");
r.Raction_bind("hello.clicked", () => {
	r.Rlabel_set_text(r.Rwidget_find(window, "status"), "Clicked");
});
await r.runLoop();
r.Rshutdown();
```

## What differs from C

The same as for Bun:

- Texts are strings on both sides.
- A JavaScript function can be given wherever C takes a callback. An exception
  it throws is printed, and the callback returns 0, because an exception must
  not cross the frames of C.
- A parameter that C writes through is returned instead:
  `const [x, y, width, height] = r.Rwidget_get_rect(widget)`.
- The size after a text or bytes is left out, and so is the count after a list.
- A structure is a class with an accessor per field: `new r.RInitDesc()`. One
  received by a callback reads the memory of the library, so it is only valid
  during the call.

And two functions Bun has in `bun:ffi` are here, for reading what the library
returns as an address: `r.toArrayBuffer(address, offset, size)` (the pixels of
`Rwindow_capture`, for instance) and `r.ptr(typedArray)`.

An argument of the wrong type throws a `TypeError` instead of reaching C. `int`
parameters also take booleans.

## The loop

`Rwait_frame(-1)` sleeps inside C, so timers, promises and the callbacks that
arrive from other threads wait with it. `await r.runLoop()` keeps them running
beside the interface: `Rwait_frame` still wakes at once for an event of a window,
and JavaScript gets its turn at least every `interval_seconds` (1/60 by default).

## How long a callback lives

Unlike the Bun binding, a callback is released as soon as Raven has forgotten
it, so a replaced closure can be collected:

- A callback set on an object (`Rbutton_set_clicked_callback`, `Rwindow_set_close_callback`...)
  lives until it is replaced or cleared on that object, or until the object is
  gone. The addon learns that a widget is gone through its destroy callback, so
  `Rwidget_set_destroy_callback` of the program is kept beside it and called
  first. Windows are checked after every frame.
- An action or a tag of XML lives until it is unbound (`Raction_unbind`,
  `Rxml_unregister_tag`) or replaced under the same name.
- `Rtimer_create` without repeat, `Rtask_post` and `Rdialog_open` call once and
  release the function after the call. A repeating timer lives until
  `Rtimer_destroy`.
- `Rshutdown` releases everything.

A callback that replaces itself while it runs is safe: the release waits for the
end of the call.

## Threads

Raven starts three kinds of threads: the workers of the job system, the render
thread, and the thread that watches files. Only some of what they do calls back
into the application:

| Callback | Thread | Here |
|---|---|---|
| `Rtask_run_async` | a worker, when workers exist | queued to the JavaScript thread with a thread-safe function; the process stays alive until it arrives |
| `Rerror_set_callback` | the thread that raised the error (a worker, the render thread) | queued the same way when it comes from another thread, called at once on the JavaScript thread |
| `Rtask_post` and every other callback | the UI thread, which is the JavaScript thread | called directly |
| `Rparallel_for`, `Rviewport_set_draw_callback` | workers, the render thread; C waits for them to finish | the address of a native function only: a JavaScript function cannot run on those threads, and C waiting for it would block the thread that must run it |

The paint callbacks of `Custom`, `ListView`, `Table`, `TileView`, `NodeGraph` and
`Plot` are UI-thread only (the source marks those widgets `RVN_CF_UI_PAINT`), and
the thread that watches files posts to the UI thread with `Rtask_post`.

Worth knowing:

- A callback that arrives from another thread runs when the event loop of
  JavaScript runs: not inside `Rwait_frame(-1)`, and not until the current
  JavaScript stack returns. Use `await r.runLoop()`.
- `Rinit` and a new `RInitDesc` leave the render thread off, as Bun does, because
  everything is simpler on one thread. It can be turned on (`disable_render_thread = 0`)
  and `Rerror_set_callback` still works.
- `RAllocator` and `RHostPlatformDesc` hold addresses of native functions.

Raven belongs to the thread that starts it: call it from that thread only, not
from a `Worker`.

## Tests

    RAVEN_LIBRARY=/path/to/libraven.so RAVEN_ADDON=/path/to/raven_napi.node \
        node --expose-gc src/example.mjs --test

runs without a window system and without a GPU, as `ctest` does (`binding_node`
under Node.js and `binding_node_bun` under Bun, which also needs `node_api.h`
to compile the addon).

`npm run typecheck` needs `npm install` first.
