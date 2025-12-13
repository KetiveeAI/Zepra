//! NXGFX - Low-level Graphics Backend for NXRender
//!
//! Supports 8K, HDR10+, 60Hz+ rendering.

pub mod context;
pub mod buffer;
pub mod primitives;
pub mod text;
pub mod image;
pub mod effects;
pub mod backend;
pub mod display;
pub mod sysinfo;

// Core exports
pub use context::GpuContext;
pub use primitives::{Color, Rect, Point, Size};

// Display exports
pub use display::{Resolution, RefreshRate, HDRMode, ColorSpace, DisplayConfig, ScreenManager, ScreenInfo};

// Text exports
pub use text::{Font, FontId, FontCache, TextLayout, TextLayoutOptions, TextAlign};

// Image exports  
pub use image::{ImageData, ImageFormat, TextureCache, TextureKey};

// Effects exports
pub use effects::{LinearGradient, RadialGradient, Shadow, Blur};

#[derive(Debug, thiserror::Error)]
pub enum GfxError {
    #[error("Initialization failed: {0}")]
    InitializationFailed(String),
    #[error("Shader error: {0}")]
    ShaderCompilationFailed(String),
    #[error("Allocation failed: {0}")]
    AllocationFailed(String),
    #[error("Backend error: {0}")]
    BackendError(String),
    #[error("Font error: {0}")]
    FontError(String),
    #[error("Image error: {0}")]
    ImageError(String),
}

pub type Result<T> = std::result::Result<T, GfxError>;
