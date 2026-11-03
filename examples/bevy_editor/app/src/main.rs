//! The Raven Bevy Editor.
//!
//! The interface is Raven (editor_ui: editor.xml and the Rust that drives it). The scene is drawn by Bevy:
//! a Bevy app without a window renders the scene with its own camera into a picture, the
//! picture is read back from the GPU and shown in the viewport of the interface. Bevy only
//! works when the editor asks for a picture (the scene changed, the camera moved), so an
//! idle editor costs nothing.
//!
//!   raven-bevy-editor [scene.scene]         the scene drawn by Bevy
//!   raven-bevy-editor --software [scene]    the scene drawn on the CPU (no Bevy)

use bevy::asset::RenderAssetUsages;
use bevy::prelude::*;
use bevy::render::camera::RenderTarget;
use bevy::render::gpu_readback::{Readback, ReadbackComplete};
use bevy::render::pipelined_rendering::PipelinedRenderingPlugin;
use bevy::render::render_resource::{Extent3d, TextureDimension, TextureFormat, TextureUsages};
use bevy::window::ExitCondition;
use bevy::winit::WinitPlugin;
use editor_core::{EntityId, Kind, Shape};
use editor_ui::{Frame, RenderInput, SceneRenderer};
use std::collections::HashMap;
use std::sync::{Arc, Mutex};

/// The newest picture read back from the GPU, and the size it must have.
#[derive(Resource, Clone, Default)]
struct Latest {
    frame: Arc<Mutex<Option<Frame>>>,
    size: Arc<Mutex<(u32, u32)>>,
}

#[derive(Resource, Default)]
struct Grid {
    shown: bool,
    center: Vec3,
}

/// What an entity of the editor became in Bevy: changing what it is means making it again.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
enum Made {
    Nothing,
    Mesh(Shape),
    PointLight,
    DirectionalLight,
}

fn made(kind: &Kind) -> Made {
    match kind {
        Kind::Mesh { shape, .. } => Made::Mesh(*shape),
        Kind::PointLight { .. } => Made::PointLight,
        Kind::DirectionalLight { .. } => Made::DirectionalLight,
        Kind::Empty | Kind::Camera { .. } => Made::Nothing,
    }
}

struct Spawned {
    entity: Entity,
    made: Made,
    material: Option<Handle<StandardMaterial>>,
}

pub struct BevyRenderer {
    app: App,
    image: Handle<Image>,
    camera: Entity,
    meshes: HashMap<Shape, Handle<Mesh>>,
    spawned: HashMap<EntityId, Spawned>,
    latest: Latest,
    size: (u32, u32),
    /// Frames still to run for the asked picture to come back (rendering, then the copy).
    updates_left: u32,
    adapter: String,
}

fn color3(c: [f32; 3]) -> Color {
    Color::srgb(c[0], c[1], c[2])
}

fn color4(c: [f32; 4]) -> Color {
    Color::srgba(c[0], c[1], c[2], c[3])
}

/// Called with what the GPU copied: rows may be padded to 256 bytes.
fn on_readback(trigger: Trigger<ReadbackComplete>, latest: Res<Latest>) {
    let data = &trigger.event().0;
    let (w, h) = *latest.size.lock().unwrap();
    if w == 0 || h == 0 {
        return;
    }
    let row = (w * 4) as usize;
    let padded = row.div_ceil(256) * 256;
    let pixels = if data.len() == row * h as usize {
        data.clone()
    } else if data.len() == padded * h as usize {
        let mut out = Vec::with_capacity(row * h as usize);
        for y in 0..h as usize {
            out.extend_from_slice(&data[y * padded..y * padded + row]);
        }
        out
    } else {
        // a picture of the size before a resize
        return;
    };
    *latest.frame.lock().unwrap() = Some(Frame { pixels, width: w, height: h });
}

fn draw_grid(mut gizmos: Gizmos, grid: Res<Grid>) {
    if !grid.shown {
        return;
    }
    let c = Vec3::new(grid.center.x.round(), 0.0, grid.center.z.round());
    gizmos.grid(Isometry3d::new(c, Quat::from_rotation_x(std::f32::consts::FRAC_PI_2)), UVec2::splat(60), Vec2::splat(1.0), Color::srgba(1.0, 1.0, 1.0, 0.14));
    gizmos.line(Vec3::new(-30.0 + c.x, 0.001, 0.0), Vec3::new(30.0 + c.x, 0.001, 0.0), Color::srgb(0.86, 0.3, 0.32));
    gizmos.line(Vec3::new(0.0, 0.001, -30.0 + c.z), Vec3::new(0.0, 0.001, 30.0 + c.z), Color::srgb(0.3, 0.5, 0.95));
}

fn target_image(width: u32, height: u32) -> Image {
    let size = Extent3d { width, height, depth_or_array_layers: 1 };
    let mut image = Image::new_fill(size, TextureDimension::D2, &[0, 0, 0, 255], TextureFormat::Rgba8UnormSrgb, RenderAssetUsages::default());
    image.texture_descriptor.usage = TextureUsages::TEXTURE_BINDING | TextureUsages::COPY_SRC | TextureUsages::COPY_DST | TextureUsages::RENDER_ATTACHMENT;
    image
}

impl BevyRenderer {
    pub fn new() -> BevyRenderer {
        let mut app = App::new();
        app.add_plugins(
            DefaultPlugins
                .build()
                // the window is Raven's: Bevy opens none and is driven by the editor
                .disable::<WinitPlugin>()
                .disable::<PipelinedRenderingPlugin>()
                .set(WindowPlugin { primary_window: None, exit_condition: ExitCondition::DontExit, close_when_requested: false, ..default() }),
        );
        let latest = Latest::default();
        app.insert_resource(latest.clone());
        app.insert_resource(Grid { shown: true, center: Vec3::ZERO });
        app.add_systems(Update, draw_grid);
        app.finish();
        app.cleanup();

        let world = app.world_mut();
        let image = world.resource_mut::<Assets<Image>>().add(target_image(64, 64));
        let camera = world
            .spawn((
                Camera3d::default(),
                Camera { target: RenderTarget::Image(image.clone().into()), clear_color: ClearColorConfig::Custom(Color::BLACK), ..default() },
                Transform::from_xyz(0.0, 3.0, 8.0).looking_at(Vec3::ZERO, Vec3::Y),
            ))
            .id();
        world.spawn(Readback::texture(image.clone())).observe(on_readback);
        let mut meshes = HashMap::new();
        {
            let mut assets = world.resource_mut::<Assets<Mesh>>();
            meshes.insert(Shape::Cube, assets.add(Cuboid::new(1.0, 1.0, 1.0)));
            meshes.insert(Shape::Sphere, assets.add(Sphere::new(0.5).mesh().uv(32, 18)));
            meshes.insert(Shape::Plane, assets.add(Plane3d::default().mesh().size(1.0, 1.0)));
            meshes.insert(Shape::Cylinder, assets.add(Cylinder::new(0.5, 1.0)));
            meshes.insert(Shape::Capsule, assets.add(Capsule3d::new(0.5, 1.0)));
            meshes.insert(Shape::Torus, assets.add(Torus::new(0.25, 0.5)));
        }
        let adapter = world
            .get_resource::<bevy::render::renderer::RenderAdapterInfo>()
            .map(|info| info.name.clone())
            .unwrap_or_default();
        BevyRenderer { app, image, camera, meshes, spawned: HashMap::new(), latest, size: (0, 0), updates_left: 2, adapter }
    }

    fn sync_scene(&mut self, input: &RenderInput) {
        let scene = input.scene;
        let world = self.app.world_mut();
        // gone, or no longer the same thing
        let stale: Vec<EntityId> = self
            .spawned
            .iter()
            .filter(|(id, s)| scene.get(**id).map(|e| made(&e.kind) != s.made).unwrap_or(true))
            .map(|(id, _)| *id)
            .collect();
        for id in stale {
            if let Some(s) = self.spawned.remove(&id) {
                world.despawn(s.entity);
                if let Some(m) = s.material {
                    world.resource_mut::<Assets<StandardMaterial>>().remove(&m);
                }
            }
        }
        for e in &scene.entities {
            // the hierarchy is the editor's: Bevy gets where each entity is in the world
            let m = scene.world_matrix(e.id);
            let transform = Transform::from_matrix(Mat4::from_cols_array(&m.m));
            let visibility = if scene.is_shown(e.id) { Visibility::Inherited } else { Visibility::Hidden };
            if !self.spawned.contains_key(&e.id) {
                let mut material = None;
                let entity = match &e.kind {
                    Kind::Mesh { shape, .. } => {
                        let handle = world.resource_mut::<Assets<StandardMaterial>>().add(StandardMaterial::default());
                        material = Some(handle.clone());
                        world.spawn((Mesh3d(self.meshes[shape].clone()), MeshMaterial3d(handle), transform, visibility)).id()
                    }
                    Kind::PointLight { .. } => world.spawn((PointLight::default(), transform, visibility)).id(),
                    Kind::DirectionalLight { .. } => world.spawn((DirectionalLight::default(), transform, visibility)).id(),
                    _ => world.spawn((transform, visibility)).id(),
                };
                self.spawned.insert(e.id, Spawned { entity, made: made(&e.kind), material });
            }
            let s = &self.spawned[&e.id];
            let mut entity = world.entity_mut(s.entity);
            entity.insert((transform, visibility));
            match &e.kind {
                Kind::PointLight { color, intensity, range, shadows } => {
                    entity.insert(PointLight { color: color3(*color), intensity: *intensity, range: *range, shadows_enabled: *shadows, ..default() });
                }
                Kind::DirectionalLight { color, illuminance, shadows } => {
                    entity.insert(DirectionalLight { color: color3(*color), illuminance: *illuminance, shadows_enabled: *shadows, ..default() });
                }
                Kind::Mesh { shape, color, metallic, roughness } => {
                    if let Some(handle) = s.material.clone() {
                        if let Some(mat) = world.resource_mut::<Assets<StandardMaterial>>().get_mut(&handle) {
                            mat.base_color = color4(*color);
                            mat.metallic = *metallic;
                            mat.perceptual_roughness = *roughness;
                            mat.alpha_mode = if color[3] < 1.0 { AlphaMode::Blend } else { AlphaMode::Opaque };
                            // a plane is seen from below too
                            mat.double_sided = *shape == Shape::Plane;
                            mat.cull_mode = if *shape == Shape::Plane { None } else { Some(bevy::render::render_resource::Face::Back) };
                        }
                    }
                }
                _ => {}
            }
        }
        world.insert_resource(AmbientLight { color: color3(scene.ambient), brightness: scene.ambient_brightness, ..default() });
    }
}

impl SceneRenderer for BevyRenderer {
    fn name(&self) -> String {
        if self.adapter.is_empty() {
            "Bevy 0.16".into()
        } else {
            format!("Bevy 0.16 on {}", self.adapter)
        }
    }

    fn request(&mut self, input: &RenderInput) {
        let (w, h) = (input.width.max(1), input.height.max(1));
        if (w, h) != self.size {
            self.size = (w, h);
            *self.latest.size.lock().unwrap() = (w, h);
            let image = self.image.clone();
            if let Some(img) = self.app.world_mut().resource_mut::<Assets<Image>>().get_mut(&image) {
                img.resize(Extent3d { width: w, height: h, depth_or_array_layers: 1 });
            }
        }
        self.sync_scene(input);
        let cam = input.camera;
        let world = self.app.world_mut();
        let eye = cam.eye();
        let target = cam.target;
        let mut camera = world.entity_mut(self.camera);
        camera.insert((
            Transform::from_xyz(eye.x, eye.y, eye.z).looking_at(Vec3::new(target.x, target.y, target.z), Vec3::Y),
            Projection::from(PerspectiveProjection { fov: cam.fov.to_radians(), near: cam.near, far: cam.far, ..default() }),
        ));
        if let Some(mut c) = camera.get_mut::<Camera>() {
            c.clear_color = ClearColorConfig::Custom(color3(input.scene.background));
        }
        world.insert_resource(Grid { shown: input.show_grid, center: Vec3::new(target.x, 0.0, target.z) });
        // one frame to draw, then the copy to come back
        self.updates_left = 4;
    }

    fn tick(&mut self) {
        if self.updates_left > 0 {
            self.app.update();
            self.updates_left -= 1;
        }
    }

    fn poll(&mut self) -> Option<Frame> {
        self.latest.frame.lock().unwrap().take()
    }

    fn busy(&self) -> bool {
        self.updates_left > 0
    }
}

fn main() {
    let mut args: Vec<String> = std::env::args().skip(1).collect();
    let software = args.iter().any(|a| a == "--software");
    args.retain(|a| a != "--software");
    let open = args.first().map(std::path::PathBuf::from);
    let renderer: Box<dyn SceneRenderer> = if software { Box::new(editor_ui::SoftwareRenderer::new()) } else { Box::new(BevyRenderer::new()) };
    std::process::exit(editor_ui::run(renderer, open.as_deref()));
}
