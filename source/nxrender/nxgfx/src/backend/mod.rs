//! Graphics Backends
//!
//! Multiple rendering backends for different platforms:
//! - OpenGL ES 3.0 (default, cross-platform)
//! - Vulkan (future, high-performance)
//! - Software (CPU fallback for testing)

#[cfg(feature = "opengl")]
pub mod opengl;

#[cfg(feature = "vulkan")]
pub mod vulkan;

#[cfg(feature = "software")]
pub mod software;

// Re-export the default backend
#[cfg(feature = "opengl")]
pub use opengl::OpenGLBackend;

/// Backend trait that all rendering backends must implement
pub trait Backend {
    /// Initialize the backend
    fn init(&mut self) -> crate::Result<()>;
    
    /// Begin a new frame
    fn begin_frame(&mut self);
    
    /// End the current frame and present
    fn end_frame(&mut self);
    
    /// Clear the screen
    fn clear(&mut self, color: crate::primitives::Color);
    
    /// Draw a filled rectangle
    fn fill_rect(&mut self, rect: crate::primitives::Rect, color: crate::primitives::Color);
    
    /// Resize the viewport
    fn resize(&mut self, width: u32, height: u32);
}
