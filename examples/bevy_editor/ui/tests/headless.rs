//! The editor driven like a user would, without a screen: Raven's null backend, the software
//! renderer, and mouse and keys injected into the window.

use editor_core::gizmo::GizmoMode;
use editor_core::{Kind, Shape, Vec3};
use raven::*;

fn frames(n: usize) {
    for _ in 0..n {
        unsafe { Rdraw_frame() };
        editor_ui::with(|e| e.pump());
    }
}

fn rect(widget: Rid) -> (f32, f32, f32, f32) {
    let (mut x, mut y, mut w, mut h) = (0f32, 0f32, 0f32, 0f32);
    unsafe { Rwidget_get_rect(widget, &mut x, &mut y, &mut w, &mut h) };
    (x, y, w, h)
}

/// Where a point of the world is in the window (the viewport's camera works in its pixels).
fn on_screen(point: Vec3) -> (f32, f32) {
    let viewport = editor_ui::editor::ui().viewport;
    let (vx, vy, vw, vh) = rect(viewport);
    let (mut pw, mut ph) = (0, 0);
    unsafe { Rviewport_get_pixel_size(viewport, &mut pw, &mut ph) };
    let (px, py, _) = editor_ui::with(|e| e.camera.project(point, pw as f32, ph as f32)).unwrap().unwrap();
    (vx + px * vw / pw as f32, vy + py * vh / ph as f32)
}

fn click(window: Rid, x: f32, y: f32) {
    unsafe {
        Rwindow_inject_mouse_move(window, x, y);
        Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 1);
        Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 0);
    }
    frames(2);
}

fn key(window: Rid, k: RKey, modifiers: u32) {
    unsafe {
        Rwindow_inject_key(window, k, 1, modifiers);
        Rwindow_inject_key(window, k, 0, modifiers);
    }
    frames(2);
}

fn fire(action: &str) {
    fire_from(action, R_INVALID_ID);
}

/// What a widget does when the user changes it (setting it from code fires nothing):
/// the action of `on_x` in src/editor.xml is "editor.x".
fn fire_from(action: &str, sender: Rid) {
    let a = cstr(action);
    unsafe { Raction_fire(a.as_ptr(), sender) };
    frames(2);
}

#[test]
fn edit_a_scene_with_the_mouse_and_the_keys() {
    let mut desc: RInitDesc = unsafe { std::mem::zeroed() };
    desc.backend = R_BACKEND_NULL;
    desc.headless = 1;
    desc.worker_threads = -1;
    assert!(unsafe { Rinit_ex(&desc) } != 0);
    let window = editor_ui::start(Box::new(editor_ui::SoftwareRenderer::new()), None);
    unsafe { Rwindow_set_size(window, 1400, 900) };
    frames(4);

    // the starter scene is in the hierarchy, and drawn in the viewport
    let ui = editor_ui::editor::ui();
    assert_eq!(unsafe { Rtree_view_get_item_count(ui.tree) }, 4);
    let texture = editor_ui::with(|e| e.texture()).unwrap();
    assert!(texture != R_INVALID_ID, "the viewport has no picture");
    let (mut tw, mut th) = (0, 0);
    unsafe { Rtexture_get_size(texture, &mut tw, &mut th) };
    let (mut pw, mut ph) = (0, 0);
    unsafe { Rviewport_get_pixel_size(ui.viewport, &mut pw, &mut ph) };
    assert!(tw == pw && th == ph && pw > 200 && ph > 200, "texture {}x{}, viewport {}x{}", tw, th, pw, ph);

    // a click on the cube selects it, and the inspector shows it
    let cube = editor_ui::with(|e| e.scene.entities.iter().find(|x| x.name == "Cube").unwrap().id).unwrap();
    let (cx, cy) = on_screen(Vec3::new(0.0, 0.5, 0.0) + Vec3::new(0.25, 0.3, 0.25));
    click(window, cx, cy);
    assert_eq!(editor_ui::with(|e| e.selection).unwrap(), Some(cube));
    assert_eq!(unsafe { text(Rlabel_get_text(ui.inspector_title)) }, "Cube");
    assert!((unsafe { Rspin_box_get_value(ui.pos_y) } - 0.5).abs() < 1e-6);

    // the arrow X of the gizmo, dragged 120 pixels to the right
    let size = editor_ui::with(|e| (Vec3::new(0.0, 0.5, 0.0) - e.camera.eye()).dot(e.camera.forward()) * (e.camera.fov.to_radians() * 0.5).tan() * 0.28).unwrap();
    let (ax, ay) = on_screen(Vec3::new(size * 0.7, 0.5, 0.0));
    unsafe {
        Rwindow_inject_mouse_move(window, ax, ay);
        Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 1);
    }
    frames(1);
    for step in 1..=6 {
        unsafe { Rwindow_inject_mouse_move(window, ax + 20.0 * step as f32, ay) };
        frames(1);
    }
    unsafe { Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 0) };
    frames(2);
    let moved = editor_ui::with(|e| e.scene.get(cube).unwrap().transform.position).unwrap();
    assert!(moved.x > 0.3 && (moved.y - 0.5).abs() < 1e-4 && moved.z.abs() < 1e-4, "{:?}", moved);
    // the inspector follows
    assert!((unsafe { Rspin_box_get_value(ui.pos_x) } as f32 - moved.x).abs() < 1e-3);
    // one step in the history for the whole drag, undone by Ctrl+Z
    key(window, R_KEY_Z, R_MOD_CTRL as u32);
    let back = editor_ui::with(|e| e.scene.get(cube).unwrap().transform.position).unwrap();
    assert!(back.x.abs() < 1e-5, "{:?}", back);
    key(window, R_KEY_Y, R_MOD_CTRL as u32);
    assert!(editor_ui::with(|e| e.scene.get(cube).unwrap().transform.position.x).unwrap() > 0.3);

    // R: the scale gizmo; typing in the inspector changes the entity
    unsafe { Rwidget_focus(ui.viewport) };
    key(window, R_KEY_R, 0);
    assert_eq!(editor_ui::with(|e| e.gizmo.mode).unwrap(), GizmoMode::Scale);
    unsafe { Rspin_box_set_value(ui.scl_y, 3.0) };
    fire_from("editor.transform", ui.scl_y);
    assert_eq!(editor_ui::with(|e| e.scene.get(cube).unwrap().transform.scale.y).unwrap(), 3.0);

    // the Add panel: a sphere, inside the cube when asked
    unsafe { Rcheckbox_set_checked(ui.add_inside, 1) };
    fire("add.sphere");
    let (sphere, parent, count) = editor_ui::with(|e| {
        let s = e.selection.unwrap();
        (s, e.scene.get(s).unwrap().parent, e.scene.entities.len())
    })
    .unwrap();
    assert_eq!(parent, Some(cube));
    assert_eq!(count, 5);
    assert!(matches!(editor_ui::with(|e| e.scene.get(sphere).unwrap().kind.clone()).unwrap(), Kind::Mesh { shape: Shape::Sphere, .. }));
    // the tree counts the rows at its top: the sphere is a row inside the cube's
    assert_eq!(unsafe { Rtree_view_get_item_count(ui.tree) }, 4);
    let cube_row = unsafe { Rtree_view_get_item(ui.tree, 1) };
    assert_eq!(unsafe { Rtree_item_get_child_count(cube_row) }, 1);

    // the parent combo: out of the cube, where it is in the world staying the same
    let world_before = editor_ui::with(|e| e.scene.world_matrix(sphere).origin()).unwrap();
    unsafe { Rcombo_box_set_selected(ui.parent_combo, 0) };
    fire_from("editor.parent", ui.parent_combo);
    let (parent, world_after) = editor_ui::with(|e| (e.scene.get(sphere).unwrap().parent, e.scene.world_matrix(sphere).origin())).unwrap();
    assert_eq!(parent, None);
    assert!((world_after - world_before).length() < 1e-4);

    // a light, its intensity from the inspector
    unsafe { Rcheckbox_set_checked(ui.add_inside, 0) };
    fire("add.point_light");
    unsafe { Rspin_box_set_value(ui.point_intensity, 250000.0) };
    fire_from("editor.point", ui.point_intensity);
    let light = editor_ui::with(|e| e.scene.get(e.selection.unwrap()).unwrap().kind.clone()).unwrap();
    assert!(matches!(light, Kind::PointLight { intensity, .. } if (intensity - 250000.0).abs() < 1.0), "{:?}", light);

    // Delete in the viewport, then Ctrl+Z brings it back
    unsafe { Rwidget_focus(ui.viewport) };
    key(window, R_KEY_DELETE, 0);
    assert_eq!(editor_ui::with(|e| e.scene.entities.len()).unwrap(), 5);
    key(window, R_KEY_Z, R_MOD_CTRL as u32);
    assert_eq!(editor_ui::with(|e| e.scene.entities.len()).unwrap(), 6);

    // a click on the empty sky selects nothing: the inspector shows the scene
    let (vx, vy, _, _) = rect(ui.viewport);
    click(window, vx + 12.0, vy + 12.0);
    assert_eq!(editor_ui::with(|e| e.selection).unwrap(), None);
    assert_eq!(unsafe { text(Rlabel_get_text(ui.inspector_title)) }, "Scene");

    // saved, then opened again: the same scene
    let path = std::env::temp_dir().join(format!("raven-bevy-editor-{}.scene", std::process::id()));
    assert!(editor_ui::with(|e| e.save_to(&path)).unwrap());
    assert!(!editor_ui::with(|e| e.history.is_dirty()).unwrap());
    let saved = editor_ui::with(|e| e.scene.clone()).unwrap();
    editor_ui::with(|e| {
        e.history.mark_saved();
        e.new_scene();
    });
    assert_eq!(editor_ui::with(|e| e.scene.entities.len()).unwrap(), 4);
    assert!(editor_ui::with(|e| e.open_file(&path)).unwrap());
    assert_eq!(editor_ui::with(|e| e.scene.clone()).unwrap(), saved);
    assert_eq!(unsafe { Rtree_view_get_item_count(ui.tree) }, 6);
    let _ = std::fs::remove_file(&path);

    let shown = editor_ui::with(|e| e.frames_shown).unwrap();
    assert!(shown > 5, "{}", shown);
    editor_ui::stop();
    unsafe { Rshutdown() };
}
