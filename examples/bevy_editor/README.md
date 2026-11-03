# Raven Bevy Editor

A visual editor of 3D scenes for [Bevy](https://bevyengine.org), in Rust. Its
whole interface — window, menus, docked panels, hierarchy, inspector — is
described in **[`ui/src/editor.xml`](ui/src/editor.xml)**, and the code behind
it is the Rust of [`ui/src/editor.rs`](ui/src/editor.rs), which builds the
window from that XML (compiled into the program with `include_str!`), binds
the actions it names (`OnClicked="editor.save"` calls `on_save`) and finds its
widgets by name. Bevy draws the scene into the viewport of the interface.

![The viewport](../../docs/images/bevy_editor_viewport.png)

## What it does

- **Hierarchy**: entities inside entities; move up, down, into the one above,
  out of the parent (keeping their place in the world); duplicate with what is
  inside; delete.
- **Viewport**: the right button turns around the scene, the middle button
  (or Shift + right) moves, the wheel zooms, a click selects, `F` frames the
  selection, `G` shows the grid.
- **Gizmos**: `W` arrows to move, `E` rings to rotate, `R` boxes to scale (the
  middle scales on every axis, or moves in the plane of the view); `Ctrl`
  while dragging, or *Snap*, moves by steps (0.5 m, 15°, 0.1); `Escape`
  puts it back.
- **Inspector**: name, visibility, parent, position / rotation / scale, then
  what the entity is: mesh (shape, color, metallic, roughness), point light,
  directional light, camera (field of view, *look through it*); with nothing
  selected, the background and the ambient light of the scene.
- **Add**: cube, sphere, plane, cylinder, capsule, torus, lights, camera,
  empty — where the camera looks, or inside the selection.
- **Undo and redo** of everything, one step per gesture (a whole drag, a
  whole edit of a field).
- **Files**: scenes saved as readable text (`.scene`), with a question before
  losing unsaved changes.

## How it is made

```
core/   editor_core: the scene, the history, the file, the camera, picking, the gizmos (no dependency)
ui/     editor_ui: the interface (src/editor.xml, src/editor.rs) and a software renderer
app/    raven-bevy-editor: Bevy renders the scene
```

- The interface asks a `SceneRenderer` for a picture of the scene and draws
  what belongs to the editor on top: the gizmo, the box of the selection, the
  icons of lights and cameras.
- `app/` is that renderer with Bevy 0.16: a Bevy app without a window
  (`WinitPlugin` off), whose camera renders into an image; the image is read
  back from the GPU (`Readback`) and given to the Raven viewport
  (`Rtexture_update`). Bevy only runs when a picture is asked for (the scene
  changed, the camera moved), so an idle editor costs nothing. The hierarchy
  stays the editor's: Bevy gets where each entity is in the world.
- Without Bevy, `SoftwareRenderer` draws flat-shaded triangles with a depth
  buffer: the editor runs anywhere, and the tests use it.

## Build and run

The Raven package gives the library:

```sh
export RAVEN_LIB_DIR=<Raven package>/lib          # and on Linux: LD_LIBRARY_PATH=$RAVEN_LIB_DIR

cargo run --release -p raven-bevy-editor                      # the scene drawn by Bevy
cargo run --release -p raven-bevy-editor -- --software        # drawn on the CPU
cargo run --release -p editor_ui --bin raven-scene-editor-software   # without compiling Bevy at all
cargo test -p editor_core -p editor_ui                        # the tests, without a screen
```

Or open the folder in the Raven IDE and press F5.

## What was tested

- `editor_core`: 17 tests (hierarchy, duplicate, reparenting that keeps the
  world place, undo and merged steps, the file and its repairs, camera rays
  and projection, picking through rotated parents, the three gizmos).
- `editor_ui`: the software renderer, and a test that drives the real
  interface without a screen (Raven's null backend): a click on the cube
  selects it, the X arrow dragged moves it along X only, Ctrl+Z / Ctrl+Y,
  `R` and the inspector, adding inside the selection, changing the parent,
  a light, Delete and undo, a click on the sky, save and open again.
- `app/` (Bevy) was written against the Bevy 0.16 API but could not be
  compiled where it was written (no access to crates.io): build it once and
  report what the compiler says.
