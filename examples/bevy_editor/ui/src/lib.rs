//! The interface of the Raven Bevy Editor.
//!
//! The window and its panels are described in `src/editor.xml` (Raven XML), and everything
//! they do is the Rust of `src/editor.rs`, which builds the window from the XML and binds the
//! actions it names. What draws the scene is given by the program: Bevy (`raven-bevy-editor`)
//! or [`SoftwareRenderer`].

pub mod render;

/// The editor: the interface of src/editor.xml and what it does.
#[allow(clippy::all)]
pub mod editor;

pub use editor::{run, run_loop, start, stop, with, Editor};
pub use render::{Frame, RenderInput, SceneRenderer, SoftwareRenderer};
