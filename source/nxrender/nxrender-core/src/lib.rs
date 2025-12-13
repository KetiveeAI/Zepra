//! NXRender Core - Compositor, Renderer, and Application Framework

pub mod compositor;
pub mod window;
pub mod renderer;
pub mod application;

pub use compositor::{Compositor, Surface, Layer};
pub use window::Window;
pub use renderer::Painter;
pub use application::{Application, AppConfig};
