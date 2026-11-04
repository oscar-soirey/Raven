# Raven development plan

What comes next for Raven, by priority. Each item says why it matters and
roughly what it takes. `[x]` is done, `[~]` started, `[ ]` to do. The list
is a plan, not a promise: issues and pull requests can change the order.

## Done since 0.2.1 (for the next version)

- [x] **Table widget** (`R_WIDGET_TABLE`, `<Table>`): columns that are resized
  (double click on an edge fits the column) and sorted, filling columns,
  stored or virtual rows, striped rows, grid lines, keyboard, wheel,
  touch, screen readers, designer.
- [x] **SVG images**: nanosvg (zlib) in `third_party/`; any image path
  accepts an `.svg`, drawn at 2 pixels per unit and shown at its own size
  (`Rtexture_get_density`); `Rtexture_create_from_svg` for another scale.
  Damaged files are tamed before drawing (fuzzed under AddressSanitizer).
- [x] **Notifications of the system** (`Rnotification_show`): Windows
  (notification area, shown as a toast), macOS (Notification Center, inside
  a bundle), Linux / BSD (`org.freedesktop.Notifications`, libdbus loaded at
  run time); a fallback for the application elsewhere.
- [x] **Small widgets**: `DatePicker` (calendar popup, range, first day of the
  week, formats, translated names), `TimePicker`, `Toast`, `Breadcrumb`,
  `Rating`, `Pagination`.

## 1. Before 1.0: the API that cannot change afterwards

- [ ] **ABI of the structures.** Give every public structure passed by
  pointer (`RInitDesc`, `RWindowDesc`, `RFileDialogDesc`, `RTileItem`...) a
  `size` (or `version`) first field, so that fields can be added without
  breaking applications built against an older Raven.
- [ ] **Naming review.** One pass over the 1000+ functions: getters
  `_get_` / `_is_`, plural / singular, `_set_..._callback` everywhere, the
  same parameter names for the same things. Done once, before 1.0.
- [ ] **Lifetime of returned pointers.** Document, function by function,
  how long a returned `const char*` stays valid (until the next call, until
  the widget changes, until it is destroyed), and make the rule uniform
  where possible.
- [ ] **Deprecation policy.** How a name is retired (an alias kept for one
  minor version, a warning in the error callback).
- [ ] **CHANGELOG.md** kept with each pull request, from which the release
  notes are written.

## 2. Reliability

- [ ] **Tests on Windows and macOS in CI.** Today the tests run on Linux
  only; the Win32 and Cocoa platforms are only compiled. Run the headless
  tests on `windows-latest` and `macos-latest`, and the platform tests where
  a display is available.
- [ ] **`test_web` is flaky** (it drives an installed Chromium and depends
  on its start time). Give it a longer wait and a retry, or mark it
  `DISABLED` in CI and run it on its own job that may fail.
- [ ] **Win32 event queue.** The queue of events gathered while Windows runs
  its modal loops (moving, resizing) has a fixed size; make it grow, and
  test it with injected bursts.
- [ ] **`.gitattributes`**: `* text=auto`, `*.c`, `*.h` `eol=lf` (or `crlf`),
  binary files marked, so that line endings stop changing with the machine.
- [ ] **Fuzzing in CI.** The font and SVG decoders and the XML parser have
  been fuzzed by hand; add libFuzzer targets and run them for a few minutes
  on each pull request.
- [ ] **Reference documentation generated** from the headers
  (`docs/reference.html` is updated by hand; about twenty functions are
  missing from it). `tools/bindgen/cparse.py` already reads every comment.

## 3. Missing features

### Text

- [ ] **Input methods on Windows, macOS and Wayland** (Chinese, Japanese,
  Korean): today only X11 (XIM) has them. TSF / IMM32 on Windows,
  `NSTextInputClient` on macOS, `text-input-v3` on Wayland; the core already
  tracks the composition rectangle (`Rwindow_get_text_input_rect`).
- [ ] Placement of the composition window of the IME.
- [ ] Text editor: multiple cursors, search and replace, code folding, a
  language server client.
- [ ] Full OpenType shaping (Indic scripts, kerning).

### Widgets

- [ ] Table: several selected rows, cells that are edited in place, cells
  holding a widget (a checkbox, a progress bar), frozen first column.
- [ ] Date picker: a range of two dates; a time zone aware time picker; a
  12-hour clock where the locale uses one.
- [ ] Toast queue: several messages one after the other, or stacked.
- [ ] Notifications: actions (buttons), a click that brings the window
  back, replacing a notification, an icon; UNUserNotificationCenter on macOS.
- [ ] Tray icon (notification area, status item, StatusNotifierItem) with
  its menu.
- [ ] Node graph: knots on links, groups and comments, minimap, undo.
- [ ] Timeline: nested tracks, curves between keys.
- [ ] Markdown: images from the network, HTML, footnotes.

### Images

- [ ] SVG at the scale of the screen: draw the SVG again when the window
  moves to a screen of another density, instead of a fixed 2.
- [ ] SVG icons tinted with the theme (`currentColor`), with a set of icons
  shipped with Raven.
- [ ] Animated images (GIF, APNG).

### Windows and system

- [ ] Dragging from a Raven window to another application (files, text).
- [ ] Glass over a native child window.
- [ ] Embedding the system web view (WebView2, WebKitGTK).

### Rendering

- [ ] A Metal backend for macOS (MoltenVK is used today).
- [ ] A GPU memory sub-allocator; presenting several windows in one call.
- [ ] WebAssembly: Raven in a web page (WebGPU or WebGL backend, a browser
  platform), first for the tutorials of the documentation.

## 4. From `idee.txt`

- [ ] A demo in the look of iOS Safari (glass bars, tab overview).
- [ ] Glassy icons: icons drawn through the glass material.
- [ ] Audio: a small mixer for the sounds of the interface (clicks,
  notifications), optional.

## 5. Trackpad gestures (after 0.2.1)

- [ ] Rotation on Windows (Direct Manipulation) and on Wayland
  (`pointer-gestures` `rotate` events of version 2+).
- [ ] Swipes of three fingers for back / forward where the system leaves
  them to applications.
- [ ] Smart zoom (double tap with two fingers) on macOS.
- [ ] Momentum curves that match each system exactly.

## 6. For the people who use Raven

- [ ] **Runtime inspector**: a key (F12) that opens a panel listing the
  widget tree of a window, the layout rectangles, the styles that apply, the
  time spent in layout and paint.
- [ ] **Packages**: vcpkg, Conan, Homebrew, a `pkg-config` file, and the
  CMake config already installed.
- [ ] **CONTRIBUTING.md**, issue and pull request templates, a code of
  conduct.
- [ ] **Transparency about how Raven is made** (issue #1, "AI disclosure"):
  a short section in the README.
- [ ] More examples: a file manager, a settings window, a data browser with
  the table, a dashboard.
