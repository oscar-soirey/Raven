# Raven architecture

This document explains how the library is organized and why, for someone who
wants to read, debug or extend it.

## Layers

```
            application
                │  Raven.h (handles, callbacks)
┌───────────────┴────────────────────────────────────────────┐
│ xml/        parser, loader, declared widgets, documents    │
│ widgets/    one file per widget type                       │
│ widget/     tree, layout, events, painting, animation,     │
│             windows (popups, dialogs, drag and drop), style│
├──────────────────────────────┬─────────────────────────────┤
│ render/  draw lists, fonts,  │ platform/  windows, input,  │
│          textures, renderer  │            clipboard        │
│   ├─ vulkan/                 │   ├─ win32/                 │
│   └─ null/                   │   ├─ x11/                   │
│                              │   ├─ wayland/               │
│                              │   ├─ cocoa/                 │
│                              │   └─ null/                  │
├──────────────────────────────┴─────────────────────────────┤
│ core/   allocator, handles, threads, jobs, files, loop     │
└────────────────────────────────────────────────────────────┘
```

A layer only uses the layers below it. `platform/` and `render/` do not know
each other: the core passes the native window handle from one to the other.
Both are tables of functions (`rvn_platform_t`, `RBackendVTable`), chosen
at `Rinit`.

Naming: the public API is `R<object>_<action>` for functions
(`Rwindow_get_size`), `R<Name>` for types (`REvent`), `R_<NAME>` for constants
and `RAVEN_<NAME>` for build macros; everything internal is `rvn_` / `RVN_`
and is not exported. One global, `g_rvn` (`core/context.h`), holds the state
of the library. `include/Raven.h` includes every public header.

## Handles

`core/handle.h`. A handle is 32 bits: `[ type : 4 | generation : 8 | index : 20 ]`.
Each object type has a pool mapping the index to a pointer. Deleting an
object increments the generation of its slot, so a stale handle no longer
matches and the call reports `R_ERROR_INVALID_ID`. `0` and `R_INVALID_ID`
are never valid. Every public function starts by resolving its handles
(`RVN_WIDGET(widget, id, type)` in `widget/widget.h`), which also checks the
widget type.

## Threads

```
 UI thread                         worker threads              render thread
 (the one that called Rinit)    (job system)
 ─────────────────────────────     ───────────────────────     ─────────────────────
 events, callbacks                 paint jobs of large trees   every backend call:
 widget tree, layout               Rtask_run_async                surfaces, textures,
 paint -> frame packet ──────────────────────────────────────▶ frames, present
 Rtask_post tasks                Rparallel_for             viewport draw callbacks
                                   texture decoding
```

**UI thread.** Owns the widget tree. Nothing else reads or writes widgets,
except paint jobs, which only read and only run while the UI thread waits
for them.

**Render thread** (`render/renderer.c`). Every backend call becomes a command
in one ordered queue: create a surface, upload a texture, render a frame. A
frame is a packet (`rvn_frame_t`) holding its own copy of the instances, so
the UI thread goes on while the frame is rendered. The UI thread only blocks
when two frames of a window are already queued; that back-pressure is what
paces a UI that redraws continuously. `RInitDesc::disable_render_thread`
runs the commands inline instead.

**Job system** (`core/jobs.c`). A fixed pool of workers and one queue. A
thread waiting for a group of jobs runs jobs itself instead of sleeping, so
nested parallel loops cannot deadlock and no core idles. By default there is
one worker per core left after the UI and render threads.

**Parallel painting** (`widget/paint.c`). A widget whose subtree holds at
least 512 widgets splits its children into chunks; each chunk is painted
into its own draw list by a job, and the lists are concatenated in child
order, which gives exactly the result of a serial paint. Widgets whose
painting calls application code (custom widgets) are always painted on the
UI thread.

**Fonts** are the one structure paint jobs write to: a glyph that is not in
the atlas yet must be rasterized. Lookups go through a hash map that is
lock-free for readers (`core/lfmap.c`); only a miss takes a mutex.

**Deferred destruction.** Deleting a widget or a window from inside one of
its own callbacks is allowed: while an event is being dispatched
(`g_rvn.dispatch_depth > 0`), deleted objects are marked dead and freed when
the dispatch ends.

## A frame

The loop belongs to the application: `Rdraw_frame` (and `Rwait_frame`, which
sleeps first) produces one frame. Internally (`rvn_update`, `core/context.c`)
it runs, for each window that needs it:

1. **Tasks and timers**, then the frame callback of the application.
2. **Layout** (`widget/layout.c`, only if something changed):
   - *measure*, bottom-up: each widget computes its desired size from its
     children. The result is cached; a change marks the widget and its
     ancestors dirty, nothing else.
   - *arrange*, top-down: each panel gives a rectangle to its children
     according to their slots. A widget whose rectangle and content did not
     change is not arranged again.
3. **Paint** (`widget/paint.c`): the tree is walked and each widget appends
   *instances* to a draw list through the painter (`render/draw.c`), which
   converts logical units to pixels, snaps to the pixel grid and applies
   clipping and opacity. Subtrees outside the clip rectangle are skipped.
4. **Submit**: the frame packet goes to the render thread.

A window is only laid out or painted when something asked for it (a property
changed, an event, a realtime viewport, an animation still moving).
`Rwait_frame` sleeps in the window system until an event, a timer, a posted
task or `Rwake_up`.

## Rendering model

`include/raven/raven_backend.h`. The whole UI is a flat array of
`RDrawInstance` (80 bytes) and a short list of commands.

- One instance is one primitive: rounded rectangle with optional outline and
  texture, glyph, soft shadow, line, triangle, glass, blur, cubic curve.
- The vertex shader expands each instance into a quad (`gl_VertexIndex`,
  no vertex or index buffer). The fragment shader evaluates the exact shape
  as a signed distance in pixels, so edges are anti-aliased at any size and
  rounded corners cost nothing more than a rectangle.
- Clipping is a rectangle stored in each instance and tested in the fragment
  shader, so a clip change never breaks a batch.
- Untextured instances do not care which texture is bound. A new command
  only starts when a textured instance needs another texture. Text uses one
  atlas texture, so a typical window is 1 to 3 draw calls.
- Colors are premultiplied in the shader; blending is `(ONE, ONE_MINUS_SRC_ALPHA)`.
  The UI is not color managed: values are written as they are to a UNORM
  surface.

**Text** (`render/font.c`). Glyphs are rasterized by stb_truetype at the
exact pixel size (logical size x window scale) and packed into 1024 x 1024
single-channel atlas pages shared by all fonts. Pages changed during a paint
are uploaded before the frame that uses them. Missing glyphs follow a chain
of fallback fonts.

## Glass

The glass themes (`R_THEME_GLASS_DARK`, `R_THEME_GLASS_LIGHT`) are the same
widgets painted with another primitive, `R_PRIM_GLASS`: a rounded slab that
shows the frame *behind* it.

**In the draw list** (`render/draw.c`). `rvn_paint_glass` first asks for a
copy of what has been drawn so far, with a `R_DRAW_BACKDROP` command, then
emits the slab. Asking is cheap and every piece of glass does it; most
requests are useless, because nothing was drawn under that piece since the
place was last copied. After the paint, `rvn_drawlist_resolve_backdrops`
walks the list with a short list of rectangles: where the frame differs from
its copy. Every primitive adds its bounds to it. A request that overlaps
none of these rectangles is removed; one that does becomes a capture of
those rectangles, which leave the list. The first capture of a frame is the
whole window, the next ones are small: in the `glass` example, the card
under each chip, then each bar. A tool bar of ten glass buttons costs one
capture of the bar for the ten buttons. The batches that removed commands
were separating are merged again. A frame makes at most 24 captures; glass
beyond that sees a copy that may be out of date.

**In the backend** (`render/vulkan/vk_glass.c`). A frame that contains glass
is rendered into an offscreen image, the *scene*, instead of the swapchain.
At each backdrop command the render pass is interrupted and the scene is
reduced three times (1/2, 1/4, 1/8) and enlarged twice with a dual-filter
blur, whose radius follows the scale of the window. The 1/2 image is the
sharp backdrop, the result of the blur is the frosted one; both are bound as
a second descriptor set. A capture of an area runs the same five passes with
a scissor: the area plus the reach of the blur, the rest of the images
keeping what the earlier captures put there. The scene pass then continues,
and at the end the scene is copied to the swapchain.
Frames without glass skip all of this and render straight to the swapchain.

**In the shader** (`shaders/ui.frag`, `glass()`). From the signed distance
and its direction, the shader derives how far the pixel is into the rounded
rim of the slab (`bend`, 0 in the flat middle, 1 on the edge), then:

- reads the backdrop shifted toward the inside by `bend x refraction`, the
  sharp copy with red and blue slightly apart (dispersion), and mixes it with
  the frosted copy;
- raises the saturation, so that colors survive the blur, and applies the tint;
- adds a thin highlight on the rim, strongest where the rim faces a light at
  the top left, with a weaker echo on the opposite side;
- adds a reflection: the frosted backdrop read *outside* the slab, along the
  normal, so that the rim takes the colors of what surrounds it.

The parameters of a slab travel in the fields of its instance that glass
does not need otherwise (`uv`, `border_width`, `border_color`); they are
listed in `raven_backend.h`. A backend that does not implement glass can draw
`R_PRIM_GLASS` as a rounded rectangle and ignore `R_DRAW_BACKDROP`.

**In the widgets** (`widgets/common.c`). `rvn_paint_surface` draws the body
of a control: a flat box with a flat theme, a slab with a glass theme
(`rvn_glass_style` turns a role such as *control*, *input*, *bar* or *popup*
into frost, refraction, tint and shadow). Controls keep two eased values,
hover and press (`rvn_paint_animate`); while one of them is moving, the
window keeps producing frames, then goes back to sleep.

## The Vulkan backend

`src/render/vulkan/`, about 3 400 lines.

- `vk_loader.c` opens the Vulkan library at run time and fills a table of
  function pointers (`rvn_vk.CreateImage(...)`). Raven never links to Vulkan.
- `vk_device.c` creates the instance and device, or adopts the ones of the
  application (`Rvulkan_set_external_device`). Vulkan 1.0 is enough; 1.1 is
  only used for shared textures.
- `vk_surface.c` holds one surface and swapchain per window. The window
  system headers are not included: the create-info structures of the surface
  extensions are declared locally. X11 windows work with either
  `VK_KHR_xlib_surface` or `VK_KHR_xcb_surface`, Wayland windows with
  `VK_KHR_wayland_surface`; `VK_EXT_headless_surface`
  renders without a window (tests).
- `vk_pipeline.c`: per swapchain format, the render passes and the two
  pipelines (UI, blur); one descriptor set per texture.
- `vk_glass.c`: the offscreen images of a window that shows glass, and the
  blur between them. They are created with the first frame that needs them.
- `vk_texture.c`: uploads are batched into one command buffer per frame.
  Textures can wrap an image view of the application or export their memory.
- `vk_frame.c`: two frames in flight per window, each with a persistently
  mapped instance buffer; viewport callbacks; capture.

Objects are destroyed after waiting for the frames that may use them
(`rvn_vk_wait_frames`); this only happens on rare events (a texture is
deleted, a swapchain is resized), never in a steady frame.

## Platforms

`platform/platform.h` is the whole contract: create a window, change it,
pump events into one handler, clipboard, child windows, wake-up.

**Win32.** Messages are dispatched by `rvn_win32_pump`. Two things are
specific to Windows:

- While the user moves or resizes a window, Windows runs its own message
  loop and the loop of the application is not running. A timer emits `RVN_PEVENT_MODAL_TICK`,
  on which the core produces a frame, so the content follows the resize.
- Graphics drivers send messages to the window from the thread that creates
  or resizes a swapchain. If the UI thread were blocked on the render thread
  at that moment, both would wait forever. Whenever the UI thread waits for
  the render thread, it therefore keeps answering *sent* messages
  (`rvn_platform_t::wait_service`, `rvn_renderer_wait`), without dispatching
  anything to the core.

**X11.** Xlib only. Text input goes through an X input context (dead keys,
compose). The scale comes from the `Xft.dpi` resource. A self-pipe wakes the
event loop from other threads.

**Wayland.** Compiled beside X11; `rvn_platform_select` connects to the
compositor first and keeps X11 when Wayland cannot be used (no
libwayland-client or libxkbcommon, or no title bars: no `xdg-decoration` and
no libdecor). Nothing of Wayland is linked: `libwayland-client`,
`libxkbcommon`, `libwayland-cursor` and `libdecor` are opened with `dlopen`,
and `wayland_protocols.h`, written by `tools/wayland/gen_wayland.py` from the
protocol files in `third_party/wayland-protocols`, holds the interfaces as
private copies whose requests call the library through function pointers.
Things that differ from the other systems:

- *A surface becomes a window once.* The `xdg_toplevel` role is given when
  the window is created, before the renderer presents anything; a surface
  that had a buffer could no longer get it. Hiding unmaps (a null buffer)
  and drops the role; showing gives it again and waits for the first
  configure. `Rwindow_hide` syncs with the render thread first, so that no
  queued frame maps the window again.
- *Sizes.* The protocol speaks logical units, the core pixels. With
  `wp_viewporter` the buffer keeps a scale of 1 and the viewport shows it at
  the logical size: any scale, fractional or not, and a frame of the old size
  during a change is only stretched. Without it, the integer
  `wl_surface.set_buffer_scale`.
- *Input.* Key repeat is the client's job (a `timerfd` in the poll of the
  pump). Moving and resizing a window with a custom header hand the serial of
  the last button press to `xdg_toplevel.move` / `.resize`.
- *The clipboard* is read through a pipe from the other application; a
  selection of our own is answered without going through the compositor.

`tests/test_wayland.c` runs a small compositor of its own (on
`libwayland-server`) and checks the dialogue: configure, resize, scale,
pointer, keyboard, clipboard, drops, hide and show, close.

## Memory

`core/heap.c` is a TLSF heap over regions of 4 MB taken from the OS
(`VirtualAlloc`, `mmap`); a block of 1 MB or more gets a mapping of its own.
Allocating and freeing are constant time, free neighbours are merged, and an
empty region goes back to the OS (`Rmemory_trim`, and at `Rshutdown`).
`core/memory.c` is what the library calls (`rvn_alloc`, `rvn_free`...): it
forwards to the heap, or to the functions given with `Rset_allocator`, keeps
the statistics, and in a Debug build fills new blocks with `0xCD` and freed
ones with `0xDD`. The stb decoders are compiled with these functions.
A build with AddressSanitizer uses `malloc` by default, so that the
sanitizer sees every block.

## Animation

`widget/anim.c`. A spring (`RSpring`) is a value, a velocity and a target; it
is advanced by the time of the frame with a fixed sub-step, with the
stiffness and damping of its `RMotion`. Three things are built on it:

- **Paint springs**: `rvn_paint_spring` (public: `Rdraw_spring`) advances a
  spring from inside a paint function and, while it moves, asks its window
  for another frame. Hover, press, switches, selections, scrollbars use it.
- **Transitions**: a widget that appears gets a small state (kind, start,
  delay); when it is painted, the instances it produced are transformed
  after the fact (`rvn_drawlist_transform`: scale, offset, opacity around a
  pivot). The layout is not involved, so neighbours and hit testing see the
  widget in place from the first frame.
- **Layout springs**: `rvn_widget_animate_layout` makes the rectangle of a
  widget chase the one the layout gave it. Docked panels and the island use
  it; the window steps these springs before painting.

A popup that closes leaves a *ghost*: the window keeps painting it, shrinking
back into what opened it, until its spring is at rest. `Rset_animation_time_step`
replaces the clock (tests, films); `Rset_animations_enabled(R_FALSE)` puts
every spring on its target at once.

## Windows: header, dialogs, drag and drop

`widget/window.c` holds what belongs to a window rather than to a widget.

- **Custom header.** The platform asks the core what is under a point
  (`rvn_window_region_at`: client, caption, one of the eight edges, a
  button). On Windows the answer is given to `WM_NCHITTEST`, so the system
  itself moves, resizes and snaps the window. On X11 the core starts the
  move or the resize (`_NET_WM_MOVERESIZE`, or by hand without a window
  manager) from `rvn_window_press_region`; on Wayland, `xdg_toplevel.move`
  and `.resize`.
- **Popups** are roots of their own, laid out and painted after the content:
  menus, combo lists, the completion list, tooltips. **Dialogs** are popups
  with the `RVN_POPUP_DIALOG` placement: a veil over the window, the content
  on a sheet that rises; the window owns the content. `Rdialog_run` and
  `Rmessage_box` run a nested loop.
- **Drag and drop.** The platforms translate `WM_DROPFILES` and the XDND
  protocol into `RVN_PEVENT_DRAG` / `RVN_PEVENT_DROP`. The target is the
  nearest widget under the pointer that has a drop callback, else the
  window; it is highlighted while it accepts.
- **File dialogs** (`widgets/dialogs.c`): the common dialogs on Windows, a
  child process (`zenity`, `kdialog`) on Linux during which input to the
  windows of Raven is blocked, and otherwise a dialog of Raven around the
  `FileBrowser` widget (`widgets/file_browser.c`, on `core/fs.c`).

## Docking

`widgets/dock_area.c`. The panels are the children of the widget; their
arrangement is a binary tree beside them: a leaf is a group of tabs, a node
splits its rectangle in two with a ratio. Arranging walks the tree; each
group then *chases* its rectangle with layout springs, which is what makes
the panels fly when the tree changes. Dragging a tab shows where it would go
(a tab bar, the middle or a side of a group, an edge of the area) and
dropping it edits the tree. `Rdock_area_save_layout` writes the tree as text:
`H(250,T(0;files),V(500,T(0;editor),T(1;log,about)))`.

## Text editor

`widgets/text_editor.c`. The text is an array of lines, each a small UTF-8
buffer; every change goes through one function (`rvn_ed_replace`) that also
records what undoes it. Only the visible rows are measured, wrapped, colored
and painted.

`widgets/syntax.c` holds the languages: a description (`RLanguageDesc`: words,
comments, quotes, flags) and three table-driven lexers that work line by
line, carrying one byte of state from a line to the next (inside a block
comment, inside a text that spans lines, inside the script of an HTML page).
The editor keeps that byte for each line and lexes again from the first line
that changed. Proposals, bracket matching and indentation read the same
tokens. There is no parser: the editor knows what a word is, not what it means.

## Themes and styles

`widget/theme.c`, `widget/style.c`; the guide is [THEMES.md](THEMES.md). A
theme (`rvn_theme_t`) is a table of colors and metrics, a material, a font,
the colors of code and a list of style rules. The theme of the application
is a copy (`g_rvn.theme`) that `Rtheme_set_...` changes; named themes
(the four built-in ones, made on first use, and those read from files) are
kept at addresses that do not move, so that a widget can point to one.

A widget asks `rvn_widget_color(widget, role)`: what was set on the widget,
else what the rules of its theme give it, else the theme. The rules that
apply to a widget (its type, its classes) are merged once into a "match"
kept by the theme; the widget keeps a pointer to it and a generation number,
and asks again when the styles or its classes changed. A theme given to a
widget (`Theme="..."`) is a pointer inherited down the tree when children
are added; the island uses the "dark twin" of the current theme that way.

## Files loaded again

`xml/xml_watch.c`. A timer compares the date and size of the watched files
five times a second (no thread, no API of the system: it behaves the same
everywhere). For an interface file, Raven kept the document it was built
from, each element knowing its widget. The new document is checked first
(every element must be buildable), then compared with the old one: same tag
and no attribute removed means the widget is kept and only the attributes
that changed are set; children are paired by `Name`, else by identical
content, else by position among those of the same tag; the rest is built or
deleted. Classes that read their own child elements are built again when
those differ. A file that does not parse is tried again twice (an editor may
be in the middle of writing it) before the error is reported.

## Markdown

`widgets/markdown.c` parses a text into blocks and spans, lays them out for
a width (runs of text of one font and color, and boxes: bullets, rules, code
backgrounds, table lines), and paints them; `widgets/label.c` uses it when
`Markdown` is set and keeps the layout until the text or the width changes.
Bold and italic are *views* of a font (`render/font.c`): the same advances,
glyphs drawn twice a fraction of a pixel apart from the outline (bold) or
sheared row by row (italic), cached under their own key. In the text editor
Markdown is one more language of `widgets/syntax.c`, whose tokens the editor
draws with those views.

## Blur, windows that are seen through, curves

Three additions to the rendering model, all in `ui.frag`:

- `R_PRIM_BLUR` samples the copy of the frame that glass already uses, with
  a radius of its own (a spiral of taps over the blurred copy), a grain and
  a saturation; no rim, no bending. `BackgroundBlur` draws it.
- `R_DRAW_ERASE` is a command, not a primitive: its instances are blended
  with `(ZERO, ONE_MINUS_SRC_ALPHA)`, which removes what was drawn where
  they are. In a window that is seen through, that opens a hole down to the
  desktop: the `Native` style of `BackgroundBlur`, where the system does
  the blurring (`rvn_platform_t::window_set_backdrop`).
- `R_PRIM_CURVE` is a cubic Bezier curve in one instance: the fragment
  shader follows it as 24 segments, keeps the nearest one and how far along
  it is, and from that draws the stroke, a halo, a color that shades from
  one end to the other, and pulses that run along it. The links of
  `NodeGraph` are one instance each.

A transparent window asks the platform for a visual with alpha (X11: a
32-bit visual and its colormap, only when a compositor owns the screen;
Win32: the DWM; Wayland: always, without an opaque region) and the swapchain for premultiplied alpha; its frame is
cleared to nothing and the wallpaper of the theme is not drawn.

## A canvas that zooms: NodeGraph

`widgets/node_graph.c`. Everything that zooms is painted in *canvas* units
with a painter whose scale is the scale of the window times the zoom asked
for, so that text is rasterized at the size it will have, not stretched; the
little that is left while the zoom glides there is a scale applied afterwards
to what was drawn (`rvn_drawlist_transform`). The zoom asked for is rounded
to a twentieth, which bounds the number of glyph sizes it can create.

The widgets inside the nodes are real children of the graph, laid out in
that canvas. Two small hooks of the widget base make them work at any zoom:
`child_scale` (the hit test divides by it; `events.c` converts the position
of the mouse for each widget that receives an event; popups opened from
there are placed where their owner really shows) and the class callback
`child_at` (a child is only reachable where its node is the one on top).

A wire hangs and swings: its control points are offset by a spring that is
pushed back by whatever its ends just did.

## Libraries beside Raven: raven_web

`web/` is a separate library built on the public API only; the guide is
[WEB.md](WEB.md). One browser process, started with two pipes (socket pairs
on Unix, anonymous pipes whose handles are given on the command line on
Windows); one thread reads its messages. Pictures of pages are decoded on
that thread (`Rimage_decode`) and acknowledged at once; everything else is
handed over as text. The UI thread is woken with `Rtask_post`, and is the
only one that touches the views. A view is a `R_WIDGET_CUSTOM` with a paint
callback (the texture), an event callback (the input, sent as commands of
the protocol) and a destroy callback (the page is closed; the browser stops
with the last view). What it needed from Raven and did not find was added to
the public API for everyone: `Rxml_register_tag`,
`Rwidget_set_destroy_callback`, `Rimage_decode`, `Rxml_resolve_path`.

## XML: declared widgets and documents

`xml/xml_parser.c` reads a text into a tree in an arena. `xml/xml_components.c`
rewrites that tree before anything is built: `<Widget>` declarations are
copied into a registry and removed, their uses are replaced by their bodies
with the expressions computed, `<Repeat>`, `<ForEach>`, `<If>` and
`<Include>` are carried out. `xml/xml_loader.c` then builds widgets from a
plain tree, as before.

`xml/xml_dom.c` is the other way to hold a document: nodes that are objects
with handles, for tools. To build, it is turned into the tree of the loader
with each element carrying the handle of its node (`source`); the rewriting
keeps it, and gives everything a declared widget expands into the node where
the tag is used. `xml/xml_schema.c` lists the tags and attributes as data; a
test gives every attribute it lists to the loader, so the list cannot drift
from what the classes accept.

## Tools

`tools/designer/main.c` is an application of the public API only (it is also
a test of it: `designer_script` drives it without a window system).
`tools/bindgen/` reads the public headers with a small parser that only knows
the shapes they use (`cparse.py`) and writes `bindings/`; a test fails when
`bindings/` is not what it would write.

## Adding things

**A widget.** Copy a small one (`widgets/progress_bar.c`): a structure that
starts with `rvn_widget_t`, a `rvn_widget_class_t` with the callbacks it
needs (`measure`, `arrange`, `paint`, `event`, `set_attribute`), and its
public functions. Then add the type to `RWidgetType`, the class to the
table in `widget/registry.c` and the file to `CMakeLists.txt`. The XML tag
is the class name.

**A window system.** Implement `rvn_platform_t` in `platform/<name>/`, return
it from `rvn_platform_select` (`platform/platform.c`), and add the matching
surface creation in `vk_surface.c`. `platform/null/null_platform.c` is the
smallest implementation.

**A language of the text editor.** No code: an `RLanguageDesc` given to
`Rlanguage_register`, or an entry in `rvn_languages_built_in` (`widgets/syntax.c`).

**A widget that arranges children by hand** must still measure them
(`rvn_widget_measure`) in its own `measure`, and creates them in `init`, not
while measuring. One that paints its children itself sets
`RVN_CF_OWN_CHILDREN`.

**A widget from outside the library.** A `R_WIDGET_CUSTOM` and its
callbacks; `Rxml_register_tag` gives it a tag and attributes of its own.
`web/src/web_view.c` is an example of a complete one.

**An attribute.** Handle it in the `set_attribute` of the class, and list it
in `xml/xml_schema.c` (the designer and `test_schema` read it from there).

**A binding.** `tools/bindgen/gen_<language>.py` with a `generate(api)` function.

**A backend.** Implement the ten functions of `RBackendVTable`; an
application can do it without touching Raven (`Rbackend_register`, then
`Rinit(R_BACKEND_CUSTOM)`). The shaders to port are
`render/vulkan/shaders/ui.vert` and `ui.frag`, and `blur.vert` / `blur.frag`
for the glass themes.

## Tests

| | |
|---|---|
| `test_layout` | panels, slots, dirty propagation |
| `test_widgets` | controls driven by injected mouse and keyboard events |
| `test_xml` | parser, loader, attributes, error reporting, declared widgets, documents, the list of tags and attributes |
| `test_threads` | jobs, posted tasks, asynchronous textures, parallel paint equal to serial paint |
| `test_memory` | the heap (random allocations checked block by block), statistics, a custom allocator |
| `test_animation` | springs, transitions, popups that close, time step |
| `test_text_editor` | editing, history, wrapping, the lexer of every language, proposals, pairs, markers |
| `test_shell` | header bar, drag and drop, hit testing, docking, island |
| `test_dialogs` | dialogs, message boxes, the file browser on real folders |
| `test_themes` | styles, classes, a theme for a part of the tree, theme files, files loaded again |
| `test_markdown` | parsing, layout, links, the Markdown language of the editor |
| `test_tools` | color picker, timeline, node graph (links pulled with the mouse, widgets inside zoomed nodes), background blur, windows seen through, tags made by code, destroy callback, image decoding |
| `test_render` | real frames through Vulkan, pixels compared (shapes, clipping, textures, text, glass, materials, curves, blur, node graph); skipped without a Vulkan driver |
| `test_web` | `raven_web`: JSON and base64 always; with a browser of the Chromium family, pages, scripts, messages, input and history (skipped without one) |
| `designer_script` | the designer adding, moving, wrapping, undoing, and checking its XML |
| `bindings_up_to_date`, `binding_cpp`, `binding_python`, `binding_rust`, `binding_go` | the bindings match the headers; their examples run (the last three need the shared library and their tools) |

All but `test_render` run with the null platform and the null backend: no
display, no GPU (`test_web` starts a hidden browser when it finds one). Every test program also fails when a block is still
allocated after `Rshutdown`. Input is injected with `Rwindow_inject_mouse_move`, `Rwindow_inject_key`...,
which follow the same path as real events.
