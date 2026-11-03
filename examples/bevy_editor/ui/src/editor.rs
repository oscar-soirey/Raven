//! The Raven Bevy Editor: what it does. Its interface is src/editor.xml, built by
//! `editor_create`: `OnClicked="editor.save"` in the XML fires the action bound here to
//! `on_save`, and `ui().viewport` is the widget named viewport.

use crate::render::{self, Frame, RenderInput, SceneRenderer};
use editor_core::gizmo::GizmoMode;
use editor_core::{io, pick, Gizmo, History, Kind, OrbitCamera, Scene, Shape, Transform, Vec3};
use editor_core::EntityId;
use raven::*;
use std::cell::RefCell;
use std::collections::HashSet;
use std::os::raw::{c_int, c_void};
use std::path::{Path, PathBuf};

// ---------------------------------------------------------------------------
//  The state of the editor
// ---------------------------------------------------------------------------

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum MouseMode {
    None,
    /// Turning around the point looked at (right button, or left button dragged on nothing).
    Orbit,
    /// Moving the point looked at (middle button, or Shift + right button).
    Pan,
    /// Holding a handle of the gizmo.
    Gizmo,
    /// The left button is down on the scene: a click selects, a drag turns.
    Click,
}

struct Mouse {
    mode: MouseMode,
    start: (f32, f32),
    last: (f32, f32),
    /// The scene when a handle was taken: the step of the history is written at the first change.
    before: Option<Scene>,
}

pub struct Editor {
    pub scene: Scene,
    pub selection: Option<EntityId>,
    pub history: History,
    pub camera: OrbitCamera,
    pub gizmo: Gizmo,
    pub path: Option<PathBuf>,
    pub show_grid: bool,
    pub snapping: bool,
    pub log: Vec<String>,
    renderer: Box<dyn SceneRenderer>,
    texture: Rid,
    frame: Option<Frame>,
    need_render: bool,
    need_overlay: bool,
    pixel_size: (u32, u32),
    mouse: Mouse,
    tree_items: Vec<(Rid, EntityId)>,
    collapsed: HashSet<EntityId>,
    parent_choices: Vec<Option<EntityId>>,
    default_layout: String,
    window: Rid,
    pub frames_shown: u64,
}

thread_local! {
    static EDITOR: RefCell<Option<Editor>> = RefCell::new(None);
}

/// Runs `f` on the editor. None when there is no editor, or when it is already busy: a widget
/// the editor changes may call back at once, and that call is then rightly ignored.
pub fn with<R>(f: impl FnOnce(&mut Editor) -> R) -> Option<R> {
    EDITOR.with(|cell| {
        let mut guard = cell.try_borrow_mut().ok()?;
        let editor = guard.as_mut()?;
        Some(f(editor))
    })
}

fn set_shown(widget: Rid, shown: bool) {
    unsafe { Rwidget_set_visibility(widget, if shown { R_VISIBLE } else { R_COLLAPSED }) };
}

fn set_text(label: Rid, value: &str) {
    let c = cstr(value);
    unsafe { Rlabel_set_text(label, c.as_ptr()) };
}

/// Sets a number field, unless it holds it already (so that typing in it is not disturbed).
fn set_spin(spin: Rid, value: f32) {
    unsafe {
        if (Rspin_box_get_value(spin) - value as f64).abs() > 1e-5 {
            Rspin_box_set_value(spin, value as f64);
        }
    }
}

fn set_slider(slider: Rid, value: f32) {
    unsafe {
        if (Rslider_get_value(slider) - value).abs() > 1e-5 {
            Rslider_set_value(slider, value);
        }
    }
}

fn set_color(picker: Rid, c: [f32; 4]) {
    unsafe {
        let (mut r, mut g, mut b, mut a) = (0f32, 0f32, 0f32, 0f32);
        Rcolor_picker_get_color(picker, &mut r, &mut g, &mut b, &mut a);
        if (r - c[0]).abs() + (g - c[1]).abs() + (b - c[2]).abs() + (a - c[3]).abs() > 1e-4 {
            Rcolor_picker_set_color(picker, c[0], c[1], c[2], c[3]);
        }
    }
}

fn get_color(picker: Rid) -> [f32; 4] {
    let (mut r, mut g, mut b, mut a) = (0f32, 0f32, 0f32, 0f32);
    unsafe { Rcolor_picker_get_color(picker, &mut r, &mut g, &mut b, &mut a) };
    [r, g, b, a]
}

fn rgb(c: [f32; 4]) -> [f32; 3] {
    [c[0], c[1], c[2]]
}

fn rgba(c: [f32; 3]) -> [f32; 4] {
    [c[0], c[1], c[2], 1.0]
}

fn checked(checkbox: Rid) -> bool {
    unsafe { Rcheckbox_is_checked(checkbox) != 0 }
}

fn set_checked(checkbox: Rid, value: bool) {
    unsafe {
        if (Rcheckbox_is_checked(checkbox) != 0) != value {
            Rcheckbox_set_checked(checkbox, value as c_int);
        }
    }
}

fn spin(spin: Rid) -> f32 {
    unsafe { Rspin_box_get_value(spin) as f32 }
}

/// The file dialog of the system, for scenes.
fn ask_file(window: Rid, save: bool, proposed: &str) -> Option<String> {
    let (n0, p0, n1, p1) = (cstr("Raven Bevy scenes"), cstr("*.scene"), cstr("All files"), cstr("*"));
    let filters = [RFileFilter { name: n0.as_ptr(), patterns: p0.as_ptr() }, RFileFilter { name: n1.as_ptr(), patterns: p1.as_ptr() }];
    let (extension, name) = (cstr("scene"), cstr(proposed));
    let mut desc: RFileDialogDesc = unsafe { std::mem::zeroed() };
    desc.parent = window;
    desc.filters = filters.as_ptr();
    desc.filter_count = 2;
    desc.default_extension = extension.as_ptr();
    desc.file_name = name.as_ptr();
    let chosen = unsafe { if save { Rfile_dialog_save(&desc) } else { Rfile_dialog_open(&desc) } };
    if chosen.is_null() {
        None
    } else {
        Some(unsafe { text(chosen) })
    }
}

impl Editor {
    fn new(renderer: Box<dyn SceneRenderer>, window: Rid) -> Editor {
        Editor {
            scene: Scene::starter(),
            selection: None,
            history: History::new(),
            camera: OrbitCamera::default(),
            gizmo: Gizmo::default(),
            path: None,
            show_grid: true,
            snapping: false,
            log: Vec::new(),
            renderer,
            texture: R_INVALID_ID,
            frame: None,
            need_render: true,
            need_overlay: true,
            pixel_size: (0, 0),
            mouse: Mouse { mode: MouseMode::None, start: (0.0, 0.0), last: (0.0, 0.0), before: None },
            tree_items: Vec::new(),
            collapsed: HashSet::new(),
            parent_choices: Vec::new(),
            default_layout: String::new(),
            window,
            frames_shown: 0,
        }
    }

    // ---- what happens after a change ----

    pub fn say(&mut self, message: &str) {
        self.log.push(message.to_string());
        if self.log.len() > 300 {
            self.log.remove(0);
        }
        let all = cstr(&self.log.join("\n"));
        unsafe {
            Rtext_editor_set_text(ui().console, all.as_ptr());
            Rtext_editor_scroll_to_line(ui().console, self.log.len() as c_int - 1);
        }
        set_text(ui().status, message);
    }

    /// After any change of the scene: draw it again, and show it in the panels.
    /// `structure`: entities came, went, were renamed or moved in the hierarchy.
    fn changed(&mut self, structure: bool) {
        if let Some(id) = self.selection {
            if !self.scene.contains(id) {
                self.selection = None;
            }
        }
        self.need_render = true;
        self.need_overlay = true;
        if structure {
            self.rebuild_tree();
        }
        self.refresh_inspector();
        self.refresh_chrome();
    }

    /// Changes the scene as one step of the history. `merge`: steps with the same key join.
    fn edit(&mut self, label: &str, merge: Option<&str>, change: impl FnOnce(&mut Scene) -> bool) -> bool {
        let before = self.scene.clone();
        if !change(&mut self.scene) {
            return false;
        }
        self.history.record(label, &before, self.selection, merge);
        true
    }

    pub fn select(&mut self, id: Option<EntityId>) {
        let id = id.filter(|i| self.scene.contains(*i));
        if self.selection == id {
            return;
        }
        self.selection = id;
        self.history.close();
        self.gizmo.hovered = None;
        self.need_overlay = true;
        // the renderer may draw the selection itself (an outline)
        self.need_render = true;
        self.sync_tree_selection();
        self.refresh_inspector();
    }

    fn refresh_chrome(&mut self) {
        let name = self.path.as_ref().and_then(|p| p.file_name()).map(|n| n.to_string_lossy().into_owned()).unwrap_or_else(|| "Untitled".into());
        let star = if self.history.is_dirty() { "*" } else { "" };
        let title = cstr(&format!("{}{} - Raven Bevy Editor", name, star));
        unsafe { Rwindow_set_title(self.window, title.as_ptr()) };
        let ui = ui();
        let undo = match self.history.undo_label() {
            Some(l) => format!("Undo {}", l.to_lowercase()),
            None => "Undo".into(),
        };
        let redo = match self.history.redo_label() {
            Some(l) => format!("Redo {}", l.to_lowercase()),
            None => "Redo".into(),
        };
        let (u, r) = (cstr(&undo), cstr(&redo));
        unsafe {
            Rmenu_item_set_text(ui.menu_undo, u.as_ptr());
            Rmenu_item_set_text(ui.menu_redo, r.as_ptr());
            Rwidget_set_enabled(ui.menu_undo, self.history.can_undo() as c_int);
            Rwidget_set_enabled(ui.menu_redo, self.history.can_redo() as c_int);
            Rmenu_item_set_checked(ui.menu_grid, self.show_grid as c_int);
            Rmenu_item_set_checked(ui.menu_snap, self.snapping as c_int);
        }
        set_checked(ui.snap, self.snapping);
        for (button, mode) in [(ui.mode_move, GizmoMode::Move), (ui.mode_rotate, GizmoMode::Rotate), (ui.mode_scale, GizmoMode::Scale)] {
            let style = cstr(if self.gizmo.mode == mode { "Primary" } else { "Flat" });
            let key = cstr("Style");
            unsafe { Rwidget_set_attribute(button, key.as_ptr(), style.as_ptr()) };
        }
        let lights = self.scene.entities.iter().filter(|e| matches!(e.kind, Kind::PointLight { .. } | Kind::DirectionalLight { .. })).count();
        set_text(ui.stats, &format!("{} entities, {} lights  |  {}", self.scene.entities.len(), lights, self.renderer.name()));
    }

    // ---- the hierarchy ----

    fn rebuild_tree(&mut self) {
        let tree = ui().tree;
        unsafe {
            // keep what the user closed
            for (item, id) in &self.tree_items {
                if Rtree_item_is_valid(*item) != 0 && Rtree_item_get_child_count(*item) > 0 {
                    if Rtree_item_is_expanded(*item) == 0 {
                        self.collapsed.insert(*id);
                    } else {
                        self.collapsed.remove(id);
                    }
                }
            }
            Rtree_view_clear_items(tree);
        }
        self.tree_items.clear();
        for e in &self.scene.entities {
            let parent_item = e.parent.and_then(|p| self.tree_items.iter().find(|(_, id)| *id == p)).map(|(item, _)| *item).unwrap_or(R_INVALID_ID);
            let label = if e.visible { e.name.clone() } else { format!("{}  (hidden)", e.name) };
            let label = cstr(&label);
            let item = unsafe { Rtree_view_add_item(tree, parent_item, label.as_ptr()) };
            unsafe {
                Rtree_item_set_user_handle(item, e.id as usize as *mut c_void);
                if !self.scene.is_shown(e.id) {
                    Rtree_item_set_text_color(item, 0.55, 0.55, 0.58, 1.0);
                }
            }
            self.tree_items.push((item, e.id));
        }
        for (item, id) in &self.tree_items {
            unsafe { Rtree_item_set_expanded(*item, (!self.collapsed.contains(id)) as c_int) };
        }
        self.sync_tree_selection();
    }

    fn sync_tree_selection(&self) {
        let tree = ui().tree;
        let item = self.selection.and_then(|s| self.tree_items.iter().find(|(_, id)| *id == s)).map(|(item, _)| *item);
        unsafe {
            match item {
                Some(item) => {
                    if Rtree_view_get_selected_item(tree) != item {
                        Rtree_view_set_selected_item(tree, item);
                        Rtree_view_scroll_to_item(tree, item);
                    }
                }
                None => Rtree_view_clear_selection(tree),
            }
        }
    }

    // ---- the inspector ----

    fn refresh_inspector(&mut self) {
        let ui = ui();
        let entity = self.selection.and_then(|id| self.scene.get(id)).cloned();
        set_shown(ui.section_entity, entity.is_some());
        set_shown(ui.section_transform, entity.is_some());
        set_shown(ui.section_scene, entity.is_none());
        let kind = entity.as_ref().map(|e| e.kind.clone());
        set_shown(ui.section_mesh, matches!(kind, Some(Kind::Mesh { .. })));
        set_shown(ui.section_point, matches!(kind, Some(Kind::PointLight { .. })));
        set_shown(ui.section_sun, matches!(kind, Some(Kind::DirectionalLight { .. })));
        set_shown(ui.section_camera, matches!(kind, Some(Kind::Camera { .. })));
        for button in [ui.btn_up, ui.btn_down, ui.btn_in, ui.btn_out, ui.btn_duplicate, ui.btn_delete] {
            unsafe { Rwidget_set_enabled(button, entity.is_some() as c_int) };
        }

        let Some(e) = entity else {
            set_text(ui.inspector_title, "Scene");
            set_text(ui.inspector_kind, "Nothing is selected: the settings of the whole scene");
            let s = &self.scene;
            set_color(ui.scene_background, rgba(s.background));
            set_color(ui.scene_ambient, rgba(s.ambient));
            set_spin(ui.scene_ambient_brightness, s.ambient_brightness);
            return;
        };
        set_text(ui.inspector_title, &e.name);
        set_text(ui.inspector_kind, &format!("{}  |  id {}", e.kind.title(), e.id));
        unsafe {
            if text(Rtext_input_get_text(ui.name_input)) != e.name {
                let n = cstr(&e.name);
                Rtext_input_set_text(ui.name_input, n.as_ptr());
            }
        }
        set_checked(ui.visible_check, e.visible);

        // the parents it can have: anything that is not itself or below it
        let subtree = self.scene.subtree(e.id);
        self.parent_choices = vec![None];
        unsafe {
            Rcombo_box_clear_items(ui.parent_combo);
            let none = cstr("(the scene)");
            Rcombo_box_add_item(ui.parent_combo, none.as_ptr());
        }
        let mut selected = 0;
        for other in &self.scene.entities {
            if subtree.contains(&other.id) {
                continue;
            }
            if Some(other.id) == e.parent {
                selected = self.parent_choices.len() as c_int;
            }
            self.parent_choices.push(Some(other.id));
            let label = cstr(&format!("{}{}", "    ".repeat(self.scene.depth(other.id)), other.name));
            unsafe { Rcombo_box_add_item(ui.parent_combo, label.as_ptr()) };
        }
        unsafe { Rcombo_box_set_selected(ui.parent_combo, selected) };

        let t = &e.transform;
        for (field, value) in [
            (ui.pos_x, t.position.x),
            (ui.pos_y, t.position.y),
            (ui.pos_z, t.position.z),
            (ui.rot_x, t.rotation.x),
            (ui.rot_y, t.rotation.y),
            (ui.rot_z, t.rotation.z),
            (ui.scl_x, t.scale.x),
            (ui.scl_y, t.scale.y),
            (ui.scl_z, t.scale.z),
        ] {
            set_spin(field, value);
        }
        match &e.kind {
            Kind::Mesh { shape, color, metallic, roughness } => {
                let index = Shape::ALL.iter().position(|s| s == shape).unwrap_or(0) as c_int;
                unsafe {
                    if Rcombo_box_get_selected(ui.shape_combo) != index {
                        Rcombo_box_set_selected(ui.shape_combo, index);
                    }
                }
                set_color(ui.mesh_color, *color);
                set_slider(ui.mesh_metallic, *metallic);
                set_slider(ui.mesh_roughness, *roughness);
            }
            Kind::PointLight { color, intensity, range, shadows } => {
                set_color(ui.point_color, rgba(*color));
                set_spin(ui.point_intensity, *intensity);
                set_spin(ui.point_range, *range);
                set_checked(ui.point_shadows, *shadows);
            }
            Kind::DirectionalLight { color, illuminance, shadows } => {
                set_color(ui.sun_color, rgba(*color));
                set_spin(ui.sun_illuminance, *illuminance);
                set_checked(ui.sun_shadows, *shadows);
            }
            Kind::Camera { fov } => set_spin(ui.camera_fov, *fov),
            Kind::Empty => {}
        }
    }

    /// Writes a change of a field of the selected entity (one step per field while it is edited).
    fn edit_selected(&mut self, label: &str, field: &str, change: impl FnOnce(&mut editor_core::Entity) -> bool) {
        let Some(id) = self.selection else { return };
        let key = format!("{}.{}", field, id);
        let structure = field == "name" || field == "visible";
        if self.edit(label, Some(&key), |s| s.get_mut(id).map(change).unwrap_or(false)) {
            self.changed(structure);
        }
    }

    // ---- commands ----

    pub fn add(&mut self, kind: Kind, name: &str) -> EntityId {
        let inside = if checked(ui().add_inside) { self.selection } else { None };
        let target = self.camera.target;
        let eye = self.camera.eye();
        let camera_rotation = self.camera.rotation_euler();
        let mut new_id = 0;
        let label = format!("Add {}", name.to_lowercase());
        self.edit(&label, None, |s| {
            let id = s.add(name, kind.clone(), inside);
            let e = s.get_mut(id).unwrap();
            if inside.is_none() {
                // where the camera looks, on the ground for what stands on it
                let ground = match &kind {
                    Kind::Mesh { shape: Shape::Plane, .. } => 0.0,
                    Kind::Mesh { shape: Shape::Capsule, .. } => 1.0,
                    Kind::Mesh { .. } => 0.5,
                    Kind::PointLight { .. } => 2.0,
                    Kind::DirectionalLight { .. } => 4.0,
                    _ => target.y,
                };
                e.transform.position = Vec3::new((target.x * 2.0).round() / 2.0, ground, (target.z * 2.0).round() / 2.0);
            }
            match &kind {
                Kind::DirectionalLight { .. } => e.transform.rotation = Vec3::new(-50.0, 30.0, 0.0),
                Kind::Camera { .. } if inside.is_none() => e.transform = Transform { position: eye, rotation: camera_rotation, scale: Vec3::ONE },
                _ => {}
            }
            new_id = id;
            true
        });
        self.changed(true);
        self.select(Some(new_id));
        self.say(&format!("Added {}", self.scene.get(new_id).map(|e| e.name.clone()).unwrap_or_default()));
        new_id
    }

    pub fn delete_selection(&mut self) {
        let Some(id) = self.selection else { return };
        let parent = self.scene.get(id).and_then(|e| e.parent);
        let name = self.scene.get(id).map(|e| e.name.clone()).unwrap_or_default();
        let mut count = 0;
        self.edit(&format!("Delete {}", name), None, |s| {
            count = s.remove(id);
            count > 0
        });
        self.selection = parent;
        self.changed(true);
        self.say(&if count > 1 { format!("Deleted {} and what was inside ({} entities)", name, count) } else { format!("Deleted {}", name) });
    }

    pub fn duplicate_selection(&mut self) {
        let Some(id) = self.selection else { return };
        let mut copy = None;
        self.edit("Duplicate", None, |s| {
            copy = s.duplicate(id);
            if let Some(c) = copy {
                s.get_mut(c).unwrap().transform.position.x += 1.0;
            }
            copy.is_some()
        });
        self.changed(true);
        self.select(copy);
        if let Some(c) = copy {
            let name = self.scene.get(c).map(|e| e.name.clone()).unwrap_or_default();
            self.say(&format!("Duplicated as {}", name));
        }
    }

    fn arrange(&mut self, label: &str, how: impl FnOnce(&mut Scene, EntityId) -> bool) {
        let Some(id) = self.selection else { return };
        if self.edit(label, None, |s| how(s, id)) {
            self.changed(true);
        }
    }

    pub fn undo(&mut self) {
        let mut selection = self.selection;
        if let Some(label) = self.history.undo(&mut self.scene, &mut selection) {
            self.selection = selection;
            self.changed(true);
            self.say(&format!("Undone: {}", label));
        }
    }

    pub fn redo(&mut self) {
        let mut selection = self.selection;
        if let Some(label) = self.history.redo(&mut self.scene, &mut selection) {
            self.selection = selection;
            self.changed(true);
            self.say(&format!("Redone: {}", label));
        }
    }

    pub fn set_mode(&mut self, mode: GizmoMode) {
        if self.gizmo.mode != mode {
            self.gizmo.end();
            self.gizmo.mode = mode;
            self.gizmo.hovered = None;
            self.need_overlay = true;
            self.refresh_chrome();
        }
    }

    /// Looks at the selection, or at everything.
    pub fn frame_selection(&mut self) {
        let ids: Vec<EntityId> = match self.selection {
            Some(id) => vec![id],
            None => self.scene.children(None),
        };
        let mut bounds: Option<(Vec3, Vec3)> = None;
        for id in ids {
            if let Some((lo, hi)) = pick::world_bounds(&self.scene, id) {
                bounds = Some(match bounds {
                    Some((a, b)) => (a.min(lo), b.max(hi)),
                    None => (lo, hi),
                });
            }
        }
        if let Some((lo, hi)) = bounds {
            // a huge ground would push the camera far away when everything is framed
            let (lo, hi) = if self.selection.is_none() { (lo.max(Vec3::splat(-8.0)), hi.min(Vec3::splat(8.0))) } else { (lo, hi) };
            self.camera.frame(lo, hi);
            self.need_render = true;
            self.need_overlay = true;
        }
    }

    pub fn look_through_camera(&mut self) {
        let Some(id) = self.selection else { return };
        let Some(Kind::Camera { fov }) = self.scene.get(id).map(|e| e.kind.clone()) else { return };
        let world = self.scene.world_matrix(id);
        let eye = world.origin();
        let forward = world.transform_vector(-Vec3::Z).normalize();
        self.camera.yaw = (-forward.x).atan2(-forward.z).to_degrees();
        self.camera.pitch = forward.y.clamp(-1.0, 1.0).asin().to_degrees();
        self.camera.distance = 5.0;
        self.camera.target = eye + forward * 5.0;
        self.camera.fov = fov;
        self.need_render = true;
        self.need_overlay = true;
    }

    // ---- files ----

    /// True when what is not saved may be lost (saved, or the user said so).
    fn confirm_discard(&mut self) -> bool {
        if !self.history.is_dirty() {
            return true;
        }
        let (title, message) = (cstr("Unsaved changes"), cstr("The scene has changes that are not saved. Save them first?"));
        match unsafe { Rmessage_box(self.window, title.as_ptr(), message.as_ptr(), R_MESSAGE_YES_NO_CANCEL) } {
            R_MESSAGE_RESULT_YES => self.save(false),
            R_MESSAGE_RESULT_NO => true,
            _ => false,
        }
    }

    pub fn new_scene(&mut self) {
        if !self.confirm_discard() {
            return;
        }
        self.scene = Scene::starter();
        self.selection = None;
        self.path = None;
        self.history.clear();
        self.camera = OrbitCamera::default();
        self.changed(true);
        self.say("New scene");
    }

    pub fn open_file(&mut self, path: &Path) -> bool {
        match std::fs::read_to_string(path).map_err(|e| e.to_string()).and_then(|t| io::load(&t).map_err(|e| e.to_string())) {
            Ok(scene) => {
                self.scene = scene;
                self.selection = None;
                self.path = Some(path.to_path_buf());
                self.history.clear();
                self.changed(true);
                self.frame_selection();
                self.say(&format!("Opened {}", path.display()));
                true
            }
            Err(error) => {
                let message = format!("{} cannot be opened: {}", path.display(), error);
                self.say(&message);
                false
            }
        }
    }

    pub fn save_to(&mut self, path: &Path) -> bool {
        match std::fs::write(path, io::save(&self.scene)) {
            Ok(()) => {
                self.path = Some(path.to_path_buf());
                self.history.mark_saved();
                self.refresh_chrome();
                self.say(&format!("Saved {}", path.display()));
                true
            }
            Err(error) => {
                let message = format!("{} cannot be written: {}", path.display(), error);
                self.say(&message);
                let (t, m) = (cstr("Save"), cstr(&message));
                unsafe { Rmessage_box(self.window, t.as_ptr(), m.as_ptr(), R_MESSAGE_OK) };
                false
            }
        }
    }

    fn save(&mut self, ask: bool) -> bool {
        let path = match (&self.path, ask) {
            (Some(p), false) => p.clone(),
            _ => {
                let proposed = self.path.as_ref().and_then(|p| p.file_name()).map(|n| n.to_string_lossy().into_owned()).unwrap_or_else(|| "scene.scene".into());
                match ask_file(self.window, true, &proposed) {
                    Some(p) => PathBuf::from(p),
                    None => return false,
                }
            }
        };
        self.save_to(&path)
    }

    // ---- the viewport ----

    /// The viewport in the window (logical units) and its size in pixels.
    fn viewport_geometry(&self) -> (f32, f32, u32, u32) {
        let (mut x, mut y, mut w, mut h) = (0f32, 0f32, 0f32, 0f32);
        let (mut pw, mut ph) = (0 as c_int, 0 as c_int);
        unsafe {
            Rwidget_get_rect(ui().viewport, &mut x, &mut y, &mut w, &mut h);
            Rviewport_get_pixel_size(ui().viewport, &mut pw, &mut ph);
        }
        (w.max(1.0), h.max(1.0), pw.max(1) as u32, ph.max(1) as u32)
    }

    /// A point of the viewport (logical, from its corner) in the pixels the camera works in.
    fn to_pixels(&self, x: f32, y: f32) -> (f32, f32, f32, f32) {
        let (w, h, pw, ph) = self.viewport_geometry();
        (x * pw as f32 / w, y * ph as f32 / h, pw as f32, ph as f32)
    }

    fn viewport_event(&mut self, event: &REvent) -> bool {
        let (x, y) = (event.local_x, event.local_y);
        let (px, py, w, h) = self.to_pixels(x, y);
        let modifiers = event.modifiers;
        let shift = modifiers & R_MOD_SHIFT as u32 != 0;
        let ctrl = modifiers & R_MOD_CTRL as u32 != 0;
        let alt = modifiers & R_MOD_ALT as u32 != 0;
        match event.type_ {
            R_EVENT_MOUSE_DOWN => {
                unsafe { Rwidget_focus(ui().viewport) };
                self.mouse.start = (x, y);
                self.mouse.last = (x, y);
                self.mouse.mode = match event.button {
                    R_MOUSE_RIGHT if shift => MouseMode::Pan,
                    R_MOUSE_RIGHT => MouseMode::Orbit,
                    R_MOUSE_MIDDLE => MouseMode::Pan,
                    R_MOUSE_LEFT if alt => MouseMode::Orbit,
                    R_MOUSE_LEFT => {
                        let taken = match self.selection {
                            Some(id) if self.scene.is_shown(id) => self.gizmo.begin(&self.scene, id, &self.camera, px, py, w, h),
                            _ => false,
                        };
                        if taken {
                            self.mouse.before = Some(self.scene.clone());
                            MouseMode::Gizmo
                        } else {
                            MouseMode::Click
                        }
                    }
                    _ => MouseMode::None,
                };
                self.need_overlay = true;
                true
            }
            R_EVENT_MOUSE_MOVE => {
                let (dx, dy) = (x - self.mouse.last.0, y - self.mouse.last.1);
                self.mouse.last = (x, y);
                match self.mouse.mode {
                    MouseMode::Click => {
                        // dragged on the scene: turn around it
                        if (x - self.mouse.start.0).abs() + (y - self.mouse.start.1).abs() > 4.0 {
                            self.mouse.mode = MouseMode::Orbit;
                        }
                    }
                    MouseMode::Orbit => {
                        self.camera.orbit(-dx * 0.35, -dy * 0.35);
                        self.need_render = true;
                        self.need_overlay = true;
                    }
                    MouseMode::Pan => {
                        let (_, logical_h, _, _) = self.viewport_geometry();
                        self.camera.pan(dx, dy, logical_h);
                        self.need_render = true;
                        self.need_overlay = true;
                    }
                    MouseMode::Gizmo => {
                        let snapping = self.snapping != ctrl;
                        if self.gizmo.drag(&mut self.scene, &self.camera, px, py, w, h, snapping) {
                            if let Some(before) = self.mouse.before.take() {
                                let label = match self.gizmo.mode {
                                    GizmoMode::Move => "Move",
                                    GizmoMode::Rotate => "Rotate",
                                    GizmoMode::Scale => "Scale",
                                };
                                self.history.record(label, &before, self.selection, Some("gizmo"));
                            }
                            self.changed(false);
                        }
                    }
                    MouseMode::None => {
                        if let Some(id) = self.selection {
                            let before = self.gizmo.hovered;
                            if self.scene.is_shown(id) {
                                self.gizmo.hit(&self.scene, id, &self.camera, px, py, w, h);
                            }
                            if self.gizmo.hovered != before {
                                self.need_overlay = true;
                            }
                        }
                    }
                }
                true
            }
            R_EVENT_MOUSE_UP => {
                match self.mouse.mode {
                    MouseMode::Click => {
                        let ray = self.camera.ray(px, py, w, h);
                        let hit = pick::pick(&self.scene, &ray).map(|(id, _)| id);
                        self.select(hit);
                    }
                    MouseMode::Gizmo => {
                        self.gizmo.end();
                        self.history.close();
                        self.mouse.before = None;
                        self.refresh_chrome();
                    }
                    _ => {}
                }
                self.mouse.mode = MouseMode::None;
                self.need_overlay = true;
                true
            }
            R_EVENT_MOUSE_WHEEL => {
                self.camera.zoom(event.wheel_y);
                self.need_render = true;
                self.need_overlay = true;
                true
            }
            R_EVENT_KEY_DOWN => {
                match event.key {
                    R_KEY_W if !ctrl => self.set_mode(GizmoMode::Move),
                    R_KEY_E if !ctrl => self.set_mode(GizmoMode::Rotate),
                    R_KEY_R if !ctrl => self.set_mode(GizmoMode::Scale),
                    R_KEY_F if !ctrl => self.frame_selection(),
                    R_KEY_G if !ctrl => {
                        self.show_grid = !self.show_grid;
                        self.need_render = true;
                        self.refresh_chrome();
                    }
                    R_KEY_DELETE | R_KEY_BACKSPACE => self.delete_selection(),
                    R_KEY_ESCAPE => {
                        if self.mouse.mode == MouseMode::Gizmo {
                            self.gizmo.cancel(&mut self.scene);
                            if self.mouse.before.is_none() {
                                // a step was written for this drag: there is nothing to undo any more
                                self.history.forget_last();
                            }
                            self.mouse.mode = MouseMode::None;
                            self.mouse.before = None;
                            self.changed(false);
                        } else {
                            self.select(None);
                        }
                    }
                    _ => return false,
                }
                true
            }
            _ => false,
        }
    }

    /// One turn of the loop: the renderer works, its picture goes into the viewport with the
    /// editor's lines on top.
    pub fn pump(&mut self) {
        self.renderer.tick();
        let (_, _, pw, ph) = self.viewport_geometry();
        if (pw, ph) != self.pixel_size && pw > 1 && ph > 1 {
            self.pixel_size = (pw, ph);
            self.need_render = true;
        }
        if self.need_render && self.pixel_size.0 > 1 {
            self.need_render = false;
            let input = RenderInput {
                scene: &self.scene,
                selection: self.selection,
                camera: &self.camera,
                show_grid: self.show_grid,
                width: self.pixel_size.0,
                height: self.pixel_size.1,
            };
            self.renderer.request(&input);
        }
        if let Some(frame) = self.renderer.poll() {
            self.frame = Some(frame);
            self.need_overlay = true;
        }
        if self.need_overlay {
            if let Some(frame) = &self.frame {
                let mut shown = frame.clone();
                let gizmo = match self.selection {
                    Some(id) if self.scene.is_shown(id) => self.gizmo.lines(&self.scene, id, &self.camera),
                    _ => Vec::new(),
                };
                let lines = render::overlay_lines(&self.scene, self.selection, &self.camera, &gizmo);
                let gizmo_from = lines.len() - gizmo.len();
                render::draw_overlay(&mut shown, &self.camera, &lines, gizmo_from);
                unsafe {
                    if self.texture == R_INVALID_ID {
                        self.texture = Rtexture_create_from_pixels(shown.pixels.as_ptr(), shown.width as c_int, shown.height as c_int);
                        Rviewport_set_texture(ui().viewport, self.texture);
                    } else {
                        Rtexture_update(self.texture, shown.pixels.as_ptr(), shown.width as c_int, shown.height as c_int);
                    }
                    Rwidget_invalidate(ui().viewport);
                }
                self.frames_shown += 1;
                self.need_overlay = false;
            }
        }
    }

    /// True while the loop should turn without waiting (a picture is on its way).
    pub fn busy(&self) -> bool {
        self.need_render || self.renderer.busy()
    }

    pub fn texture(&self) -> Rid {
        self.texture
    }
}

// ---------------------------------------------------------------------------
//  What the interface calls (the {functions} of the XML)
// ---------------------------------------------------------------------------

fn on_new(_: Rid, _: &str) {
    with(|e| e.new_scene());
}
fn on_open(_: Rid, _: &str) {
    with(|e| {
        if e.confirm_discard() {
            if let Some(path) = ask_file(e.window, false, "") {
                e.open_file(Path::new(&path));
            }
        }
    });
}
fn on_save(_: Rid, _: &str) {
    with(|e| e.save(false));
}
fn on_save_as(_: Rid, _: &str) {
    with(|e| e.save(true));
}
fn on_quit(_: Rid, _: &str) {
    if with(|e| e.confirm_discard()).unwrap_or(true) {
        unsafe { Rquit(0) };
    }
}
fn on_undo(_: Rid, _: &str) {
    with(|e| e.undo());
}
fn on_redo(_: Rid, _: &str) {
    with(|e| e.redo());
}
fn on_duplicate(_: Rid, _: &str) {
    with(|e| e.duplicate_selection());
}
fn on_delete(_: Rid, _: &str) {
    with(|e| e.delete_selection());
}
fn on_move_up(_: Rid, _: &str) {
    with(|e| e.arrange("Move up", |s, id| s.move_among_siblings(id, -1)));
}
fn on_move_down(_: Rid, _: &str) {
    with(|e| e.arrange("Move down", |s, id| s.move_among_siblings(id, 1)));
}
fn on_indent(_: Rid, _: &str) {
    with(|e| e.arrange("Move into", |s, id| s.indent(id)));
}
fn on_outdent(_: Rid, _: &str) {
    with(|e| e.arrange("Move out", |s, id| s.outdent(id)));
}
fn on_mode_move(_: Rid, _: &str) {
    with(|e| e.set_mode(GizmoMode::Move));
}
fn on_mode_rotate(_: Rid, _: &str) {
    with(|e| e.set_mode(GizmoMode::Rotate));
}
fn on_mode_scale(_: Rid, _: &str) {
    with(|e| e.set_mode(GizmoMode::Scale));
}
fn on_snap(sender: Rid, _: &str) {
    let menu = sender == ui().menu_snap;
    with(|e| {
        e.snapping = if menu { !e.snapping } else { checked(sender) };
        e.refresh_chrome();
    });
}
fn on_toggle_grid(_: Rid, _: &str) {
    with(|e| {
        e.show_grid = !e.show_grid;
        e.need_render = true;
        e.refresh_chrome();
    });
}
fn on_frame(_: Rid, _: &str) {
    with(|e| e.frame_selection());
}
fn on_reset_camera(_: Rid, _: &str) {
    with(|e| {
        e.camera = OrbitCamera::default();
        e.need_render = true;
        e.need_overlay = true;
    });
}
fn on_look_through(_: Rid, _: &str) {
    with(|e| e.look_through_camera());
}
fn on_reset_panels(_: Rid, _: &str) {
    with(|e| {
        let layout = cstr(&e.default_layout);
        unsafe { Rdock_area_load_layout(ui().dock, layout.as_ptr()) };
    });
}
fn on_about(_: Rid, _: &str) {
    with(|e| {
        let (t, m) = (
            cstr("Raven Bevy Editor"),
            cstr(&format!(
                "A visual editor of Bevy scenes.\n\nThe interface is Raven: src/editor.xml, driven by the Rust of src/editor.rs.\nThe scene is drawn by: {}.\n\nRight button: turn. Middle button or Shift + right: move. Wheel: zoom.\nW, E, R: move, rotate, scale. F: frame. Ctrl while dragging: by steps.",
                e.renderer.name()
            )),
        );
        unsafe { Rmessage_box(e.window, t.as_ptr(), m.as_ptr(), R_MESSAGE_OK) };
    });
}

fn on_tree_select(sender: Rid, _: &str) {
    let item = unsafe { Rtree_view_get_selected_item(sender) };
    with(|e| {
        let id = e.tree_items.iter().find(|(i, _)| *i == item).map(|(_, id)| *id);
        if id.is_some() && id != e.selection {
            e.select(id);
        }
    });
}

fn on_name(sender: Rid, _: &str) {
    let name = unsafe { text(Rtext_input_get_text(sender)) };
    let name = name.trim().to_string();
    if name.is_empty() {
        return;
    }
    with(|e| {
        e.edit_selected("Rename", "name", |entity| {
            if entity.name == name {
                return false;
            }
            entity.name = name;
            true
        })
    });
}

fn on_visible(sender: Rid, _: &str) {
    let visible = checked(sender);
    with(|e| {
        e.edit_selected(if visible { "Show" } else { "Hide" }, "visible", |entity| {
            let changed = entity.visible != visible;
            entity.visible = visible;
            changed
        })
    });
}

fn on_parent(sender: Rid, _: &str) {
    let index = unsafe { Rcombo_box_get_selected(sender) };
    with(|e| {
        let Some(id) = e.selection else { return };
        let Some(parent) = e.parent_choices.get(index.max(0) as usize).copied() else { return };
        if e.edit("Change the parent", None, |s| s.reparent(id, parent)) {
            e.changed(true);
        }
    });
}

fn on_transform(_: Rid, _: &str) {
    let ui = ui();
    let t = Transform {
        position: Vec3::new(spin(ui.pos_x), spin(ui.pos_y), spin(ui.pos_z)),
        rotation: Vec3::new(spin(ui.rot_x), spin(ui.rot_y), spin(ui.rot_z)),
        scale: Vec3::new(spin(ui.scl_x), spin(ui.scl_y), spin(ui.scl_z)),
    };
    with(|e| {
        e.edit_selected("Transform", "transform", |entity| {
            if (entity.transform.position - t.position).length() + (entity.transform.rotation - t.rotation).length() + (entity.transform.scale - t.scale).length() < 1e-5 {
                return false;
            }
            entity.transform = t;
            true
        })
    });
}

fn on_reset_transform(_: Rid, _: &str) {
    with(|e| {
        let Some(id) = e.selection else { return };
        if e.edit("Reset the transform", None, |s| {
            let entity = s.get_mut(id).unwrap();
            let changed = entity.transform != Transform::default();
            entity.transform = Transform::default();
            changed
        }) {
            e.changed(false);
        }
    });
}

fn on_mesh(_: Rid, _: &str) {
    let ui = ui();
    let shape = Shape::ALL[unsafe { Rcombo_box_get_selected(ui.shape_combo) }.clamp(0, 5) as usize];
    let color = get_color(ui.mesh_color);
    let (metallic, roughness) = unsafe { (Rslider_get_value(ui.mesh_metallic), Rslider_get_value(ui.mesh_roughness)) };
    let kind = Kind::Mesh { shape, color, metallic, roughness };
    with(|e| {
        e.edit_selected("Change the mesh", "mesh", |entity| {
            if entity.kind == kind {
                return false;
            }
            // a mesh that turns into another shape keeps its name if it was the name of the shape
            if let Kind::Mesh { shape: old, .. } = entity.kind {
                if old != shape && entity.name.starts_with(old.title()) {
                    entity.name = entity.name.replacen(old.title(), shape.title(), 1);
                }
            }
            entity.kind = kind;
            true
        });
        e.rebuild_tree();
    });
}

fn on_point(_: Rid, _: &str) {
    let ui = ui();
    let kind = Kind::PointLight { color: rgb(get_color(ui.point_color)), intensity: spin(ui.point_intensity), range: spin(ui.point_range), shadows: checked(ui.point_shadows) };
    with(|e| {
        e.edit_selected("Change the light", "point", |entity| {
            let changed = entity.kind != kind;
            entity.kind = kind;
            changed
        })
    });
}

fn on_sun(_: Rid, _: &str) {
    let ui = ui();
    let kind = Kind::DirectionalLight { color: rgb(get_color(ui.sun_color)), illuminance: spin(ui.sun_illuminance), shadows: checked(ui.sun_shadows) };
    with(|e| {
        e.edit_selected("Change the light", "sun", |entity| {
            let changed = entity.kind != kind;
            entity.kind = kind;
            changed
        })
    });
}

fn on_camera(_: Rid, _: &str) {
    let kind = Kind::Camera { fov: spin(ui().camera_fov) };
    with(|e| {
        e.edit_selected("Change the camera", "camera", |entity| {
            let changed = entity.kind != kind;
            entity.kind = kind;
            changed
        })
    });
}

fn on_scene_settings(_: Rid, _: &str) {
    let ui = ui();
    let (background, ambient, brightness) = (rgb(get_color(ui.scene_background)), rgb(get_color(ui.scene_ambient)), spin(ui.scene_ambient_brightness));
    with(|e| {
        let changed = e.edit("Change the scene", Some("scene"), |s| {
            let changed = s.background != background || s.ambient != ambient || (s.ambient_brightness - brightness).abs() > 1e-4;
            s.background = background;
            s.ambient = ambient;
            s.ambient_brightness = brightness;
            changed
        });
        if changed {
            e.changed(false);
        }
    });
}

unsafe extern "C" fn viewport_event(_widget: Rid, event: *const REvent, _: *mut c_void) -> c_int {
    let event = &*event;
    with(|e| e.viewport_event(event)).unwrap_or(false) as c_int
}

unsafe extern "C" fn tree_event(_widget: Rid, event: *const REvent, _: *mut c_void) -> c_int {
    let event = &*event;
    if event.type_ == R_EVENT_KEY_DOWN && (event.key == R_KEY_DELETE || event.key == R_KEY_F) {
        let delete = event.key == R_KEY_DELETE;
        return with(|e| if delete { e.delete_selection() } else { e.frame_selection() }).is_some() as c_int;
    }
    0
}

unsafe extern "C" fn on_close_window(_window: Rid, _: *mut c_void) -> c_int {
    with(|e| e.confirm_discard()).unwrap_or(true) as c_int
}

// ---------------------------------------------------------------------------
//  The interface: src/editor.xml
// ---------------------------------------------------------------------------

/// The interface of the editor, compiled into the program.
const EDITOR_XML: &str = include_str!("editor.xml");

/// The widgets of the interface that have a Name: `ui().status`.
#[derive(Clone, Copy, Debug)]
#[allow(dead_code)]
pub struct Ui {
    pub header: Rid,
    pub menu_undo: Rid,
    pub menu_redo: Rid,
    pub menu_snap: Rid,
    pub menu_grid: Rid,
    pub mode_move: Rid,
    pub mode_rotate: Rid,
    pub mode_scale: Rid,
    pub snap: Rid,
    pub dock: Rid,
    pub panel_scene: Rid,
    pub viewport: Rid,
    pub panel_hierarchy: Rid,
    pub tree: Rid,
    pub btn_up: Rid,
    pub btn_down: Rid,
    pub btn_in: Rid,
    pub btn_out: Rid,
    pub btn_duplicate: Rid,
    pub btn_delete: Rid,
    pub panel_add: Rid,
    pub add_inside: Rid,
    pub panel_inspector: Rid,
    pub inspector_title: Rid,
    pub inspector_kind: Rid,
    pub section_entity: Rid,
    pub name_input: Rid,
    pub visible_check: Rid,
    pub parent_combo: Rid,
    pub section_transform: Rid,
    pub pos_x: Rid,
    pub pos_y: Rid,
    pub pos_z: Rid,
    pub rot_x: Rid,
    pub rot_y: Rid,
    pub rot_z: Rid,
    pub scl_x: Rid,
    pub scl_y: Rid,
    pub scl_z: Rid,
    pub section_mesh: Rid,
    pub shape_combo: Rid,
    pub mesh_color: Rid,
    pub mesh_metallic: Rid,
    pub mesh_roughness: Rid,
    pub section_point: Rid,
    pub point_color: Rid,
    pub point_intensity: Rid,
    pub point_range: Rid,
    pub point_shadows: Rid,
    pub section_sun: Rid,
    pub sun_color: Rid,
    pub sun_illuminance: Rid,
    pub sun_shadows: Rid,
    pub section_camera: Rid,
    pub camera_fov: Rid,
    pub section_scene: Rid,
    pub scene_background: Rid,
    pub scene_ambient: Rid,
    pub scene_ambient_brightness: Rid,
    pub console: Rid,
    pub status: Rid,
    pub stats: Rid,
}

static UI: std::sync::Mutex<Ui> = std::sync::Mutex::new(Ui { header: R_INVALID_ID, menu_undo: R_INVALID_ID, menu_redo: R_INVALID_ID, menu_snap: R_INVALID_ID, menu_grid: R_INVALID_ID, mode_move: R_INVALID_ID, mode_rotate: R_INVALID_ID, mode_scale: R_INVALID_ID, snap: R_INVALID_ID, dock: R_INVALID_ID, panel_scene: R_INVALID_ID, viewport: R_INVALID_ID, panel_hierarchy: R_INVALID_ID, tree: R_INVALID_ID, btn_up: R_INVALID_ID, btn_down: R_INVALID_ID, btn_in: R_INVALID_ID, btn_out: R_INVALID_ID, btn_duplicate: R_INVALID_ID, btn_delete: R_INVALID_ID, panel_add: R_INVALID_ID, add_inside: R_INVALID_ID, panel_inspector: R_INVALID_ID, inspector_title: R_INVALID_ID, inspector_kind: R_INVALID_ID, section_entity: R_INVALID_ID, name_input: R_INVALID_ID, visible_check: R_INVALID_ID, parent_combo: R_INVALID_ID, section_transform: R_INVALID_ID, pos_x: R_INVALID_ID, pos_y: R_INVALID_ID, pos_z: R_INVALID_ID, rot_x: R_INVALID_ID, rot_y: R_INVALID_ID, rot_z: R_INVALID_ID, scl_x: R_INVALID_ID, scl_y: R_INVALID_ID, scl_z: R_INVALID_ID, section_mesh: R_INVALID_ID, shape_combo: R_INVALID_ID, mesh_color: R_INVALID_ID, mesh_metallic: R_INVALID_ID, mesh_roughness: R_INVALID_ID, section_point: R_INVALID_ID, point_color: R_INVALID_ID, point_intensity: R_INVALID_ID, point_range: R_INVALID_ID, point_shadows: R_INVALID_ID, section_sun: R_INVALID_ID, sun_color: R_INVALID_ID, sun_illuminance: R_INVALID_ID, sun_shadows: R_INVALID_ID, section_camera: R_INVALID_ID, camera_fov: R_INVALID_ID, section_scene: R_INVALID_ID, scene_background: R_INVALID_ID, scene_ambient: R_INVALID_ID, scene_ambient_brightness: R_INVALID_ID, console: R_INVALID_ID, status: R_INVALID_ID, stats: R_INVALID_ID });

/// The widgets of the interface: `ui().status` is the widget named status.
#[allow(dead_code)]
pub fn ui() -> Ui {
    *UI.lock().unwrap()
}

/// Builds the window of src/editor.xml, binds the actions it names to the functions of
/// this file, and finds its widgets.
pub fn editor_create() -> Rid {
    action_bind("editor.about", |sender, action| on_about(sender, action));
    action_bind("editor.camera", |sender, action| on_camera(sender, action));
    action_bind("editor.delete", |sender, action| on_delete(sender, action));
    action_bind("editor.duplicate", |sender, action| on_duplicate(sender, action));
    action_bind("editor.frame", |sender, action| on_frame(sender, action));
    action_bind("editor.indent", |sender, action| on_indent(sender, action));
    action_bind("editor.look_through", |sender, action| on_look_through(sender, action));
    action_bind("editor.mesh", |sender, action| on_mesh(sender, action));
    action_bind("editor.mode_move", |sender, action| on_mode_move(sender, action));
    action_bind("editor.mode_rotate", |sender, action| on_mode_rotate(sender, action));
    action_bind("editor.mode_scale", |sender, action| on_mode_scale(sender, action));
    action_bind("editor.move_down", |sender, action| on_move_down(sender, action));
    action_bind("editor.move_up", |sender, action| on_move_up(sender, action));
    action_bind("editor.name", |sender, action| on_name(sender, action));
    action_bind("editor.new", |sender, action| on_new(sender, action));
    action_bind("editor.open", |sender, action| on_open(sender, action));
    action_bind("editor.outdent", |sender, action| on_outdent(sender, action));
    action_bind("editor.parent", |sender, action| on_parent(sender, action));
    action_bind("editor.point", |sender, action| on_point(sender, action));
    action_bind("editor.quit", |sender, action| on_quit(sender, action));
    action_bind("editor.redo", |sender, action| on_redo(sender, action));
    action_bind("editor.reset_camera", |sender, action| on_reset_camera(sender, action));
    action_bind("editor.reset_panels", |sender, action| on_reset_panels(sender, action));
    action_bind("editor.reset_transform", |sender, action| on_reset_transform(sender, action));
    action_bind("editor.save", |sender, action| on_save(sender, action));
    action_bind("editor.save_as", |sender, action| on_save_as(sender, action));
    action_bind("editor.scene_settings", |sender, action| on_scene_settings(sender, action));
    action_bind("editor.snap", |sender, action| on_snap(sender, action));
    action_bind("editor.sun", |sender, action| on_sun(sender, action));
    action_bind("editor.toggle_grid", |sender, action| on_toggle_grid(sender, action));
    action_bind("editor.transform", |sender, action| on_transform(sender, action));
    action_bind("editor.tree_select", |sender, action| on_tree_select(sender, action));
    action_bind("editor.undo", |sender, action| on_undo(sender, action));
    action_bind("editor.visible", |sender, action| on_visible(sender, action));
    let window = window_from_xml(EDITOR_XML);
    if window != R_INVALID_ID {
        let mut ui = UI.lock().unwrap();
        ui.header = widget_find(window, "header");
        ui.menu_undo = widget_find(window, "menu_undo");
        ui.menu_redo = widget_find(window, "menu_redo");
        ui.menu_snap = widget_find(window, "menu_snap");
        ui.menu_grid = widget_find(window, "menu_grid");
        ui.mode_move = widget_find(window, "mode_move");
        ui.mode_rotate = widget_find(window, "mode_rotate");
        ui.mode_scale = widget_find(window, "mode_scale");
        ui.snap = widget_find(window, "snap");
        ui.dock = widget_find(window, "dock");
        ui.panel_scene = widget_find(window, "panel_scene");
        ui.viewport = widget_find(window, "viewport");
        ui.panel_hierarchy = widget_find(window, "panel_hierarchy");
        ui.tree = widget_find(window, "tree");
        ui.btn_up = widget_find(window, "btn_up");
        ui.btn_down = widget_find(window, "btn_down");
        ui.btn_in = widget_find(window, "btn_in");
        ui.btn_out = widget_find(window, "btn_out");
        ui.btn_duplicate = widget_find(window, "btn_duplicate");
        ui.btn_delete = widget_find(window, "btn_delete");
        ui.panel_add = widget_find(window, "panel_add");
        ui.add_inside = widget_find(window, "add_inside");
        ui.panel_inspector = widget_find(window, "panel_inspector");
        ui.inspector_title = widget_find(window, "inspector_title");
        ui.inspector_kind = widget_find(window, "inspector_kind");
        ui.section_entity = widget_find(window, "section_entity");
        ui.name_input = widget_find(window, "name_input");
        ui.visible_check = widget_find(window, "visible_check");
        ui.parent_combo = widget_find(window, "parent_combo");
        ui.section_transform = widget_find(window, "section_transform");
        ui.pos_x = widget_find(window, "pos_x");
        ui.pos_y = widget_find(window, "pos_y");
        ui.pos_z = widget_find(window, "pos_z");
        ui.rot_x = widget_find(window, "rot_x");
        ui.rot_y = widget_find(window, "rot_y");
        ui.rot_z = widget_find(window, "rot_z");
        ui.scl_x = widget_find(window, "scl_x");
        ui.scl_y = widget_find(window, "scl_y");
        ui.scl_z = widget_find(window, "scl_z");
        ui.section_mesh = widget_find(window, "section_mesh");
        ui.shape_combo = widget_find(window, "shape_combo");
        ui.mesh_color = widget_find(window, "mesh_color");
        ui.mesh_metallic = widget_find(window, "mesh_metallic");
        ui.mesh_roughness = widget_find(window, "mesh_roughness");
        ui.section_point = widget_find(window, "section_point");
        ui.point_color = widget_find(window, "point_color");
        ui.point_intensity = widget_find(window, "point_intensity");
        ui.point_range = widget_find(window, "point_range");
        ui.point_shadows = widget_find(window, "point_shadows");
        ui.section_sun = widget_find(window, "section_sun");
        ui.sun_color = widget_find(window, "sun_color");
        ui.sun_illuminance = widget_find(window, "sun_illuminance");
        ui.sun_shadows = widget_find(window, "sun_shadows");
        ui.section_camera = widget_find(window, "section_camera");
        ui.camera_fov = widget_find(window, "camera_fov");
        ui.section_scene = widget_find(window, "section_scene");
        ui.scene_background = widget_find(window, "scene_background");
        ui.scene_ambient = widget_find(window, "scene_ambient");
        ui.scene_ambient_brightness = widget_find(window, "scene_ambient_brightness");
        ui.console = widget_find(window, "console");
        ui.status = widget_find(window, "status");
        ui.stats = widget_find(window, "stats");
    }
    window
}

// ---------------------------------------------------------------------------
//  Starting and running
// ---------------------------------------------------------------------------

/// Builds the window of the editor (Raven must be started) with the renderer of the scene.
/// `open`: a scene file to open at once.
pub fn start(renderer: Box<dyn SceneRenderer>, open: Option<&Path>) -> Rid {
    let window = editor_create();
    // the Add menu and the Add panel share their actions: one per thing that can be added
    let things: Vec<(&str, Kind, &str)> = vec![
        ("add.empty", Kind::Empty, "Empty"),
        ("add.cube", Kind::mesh(Shape::Cube), "Cube"),
        ("add.sphere", Kind::mesh(Shape::Sphere), "Sphere"),
        ("add.plane", Kind::mesh(Shape::Plane), "Plane"),
        ("add.cylinder", Kind::mesh(Shape::Cylinder), "Cylinder"),
        ("add.capsule", Kind::mesh(Shape::Capsule), "Capsule"),
        ("add.torus", Kind::mesh(Shape::Torus), "Torus"),
        ("add.point_light", Kind::point_light(), "Point light"),
        ("add.directional_light", Kind::directional_light(), "Sun"),
        ("add.camera", Kind::camera(), "Camera"),
    ];
    for (action, kind, name) in things {
        action_bind(action, move |_, _| {
            with(|e| e.add(kind.clone(), name));
        });
    }
    unsafe {
        Rwidget_set_event_callback(ui().viewport, Some(viewport_event), std::ptr::null_mut());
        Rwidget_set_event_callback(ui().tree, Some(tree_event), std::ptr::null_mut());
        Rwindow_set_close_callback(window, Some(on_close_window), std::ptr::null_mut());
    }
    let mut editor = Editor::new(renderer, window);
    editor.default_layout = unsafe { text(Rdock_area_save_layout(ui().dock)) };
    EDITOR.with(|cell| *cell.borrow_mut() = Some(editor));
    with(|e| {
        e.changed(true);
        let name = e.renderer.name();
        e.say(&format!("Ready. The scene is drawn by {}.", name));
        if let Some(path) = open {
            e.open_file(path);
        }
    });
    window
}

/// The loop of the interface, until the window is closed. Returns the exit code.
pub fn run_loop() -> i32 {
    loop {
        let busy = with(|e| e.busy()).unwrap_or(false);
        if unsafe { Rwait_frame(if busy { 0.002 } else { 0.5 }) } == 0 {
            break;
        }
        with(|e| e.pump());
    }
    unsafe { Rget_exit_code() }
}

/// Starts Raven, runs the editor, stops Raven: all a program needs.
pub fn run(renderer: Box<dyn SceneRenderer>, open: Option<&Path>) -> i32 {
    unsafe {
        if Rinit(R_BACKEND_VULKAN) == 0 {
            eprintln!("Raven could not start (is Vulkan there?)");
            return 1;
        }
    }
    start(renderer, open);
    let code = run_loop();
    stop();
    unsafe { Rshutdown() };
    code
}

/// Lets the editor go (its renderer with it), before Rshutdown.
pub fn stop() {
    EDITOR.with(|cell| *cell.borrow_mut() = None);
}
