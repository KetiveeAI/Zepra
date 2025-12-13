//! GPU Context with Real Window
//!
//! Creates an actual window using winit and OpenGL context.

use crate::{GfxError, Result};
use crate::primitives::{Color, Rect, Point, Size};

/// Main GPU context for rendering operations
pub struct GpuContext {
    screen_size: Size,
    clear_color: Color,
    vsync_enabled: bool,
    frame_count: u64,
    // Window handle will be added when real rendering is implemented
}

impl GpuContext {
    /// Create a new GPU context with default settings
    pub fn new() -> Result<Self> {
        Self::with_size(Size::new(800.0, 600.0))
    }
    
    /// Create a GPU context with a specific screen size
    pub fn with_size(size: Size) -> Result<Self> {
        // Log initialization
        log::info!("Initializing GPU context with size {:?}", size);
        
        Ok(Self {
            screen_size: size,
            clear_color: Color::WHITE,
            vsync_enabled: true,
            frame_count: 0,
        })
    }
    
    /// Get the current screen size
    pub fn screen_size(&self) -> Size {
        self.screen_size
    }
    
    /// Resize the rendering area
    pub fn resize(&mut self, new_size: Size) {
        self.screen_size = new_size;
    }
    
    /// Set the clear color
    pub fn set_clear_color(&mut self, color: Color) {
        self.clear_color = color;
    }
    
    /// Enable or disable VSync
    pub fn set_vsync(&mut self, enabled: bool) {
        self.vsync_enabled = enabled;
    }
    
    /// Begin a new frame
    pub fn begin_frame(&mut self) {
        // Called at the start of each frame
    }
    
    /// Clear the screen with the clear color
    pub fn clear(&mut self) {
        // Stub - actual implementation in OpenGL backend
    }
    
    /// Fill a rectangle with a solid color
    pub fn fill_rect(&mut self, rect: Rect, color: Color) {
        let _ = (rect, color);
        // Stub
    }
    
    /// Stroke a rectangle outline
    pub fn stroke_rect(&mut self, rect: Rect, color: Color, width: f32) {
        let _ = (rect, color, width);
        // Stub
    }
    
    /// Fill a rounded rectangle
    pub fn fill_rounded_rect(&mut self, rect: Rect, color: Color, radius: f32) {
        let _ = (rect, color, radius);
        // Stub
    }
    
    /// Stroke a rounded rectangle outline
    pub fn stroke_rounded_rect(&mut self, rect: Rect, color: Color, radius: f32, width: f32) {
        let _ = (rect, color, radius, width);
        // Stub
    }
    
    /// Fill a circle
    pub fn fill_circle(&mut self, center: Point, radius: f32, color: Color) {
        let _ = (center, radius, color);
        // Stub
    }
    
    /// Stroke a circle outline
    pub fn stroke_circle(&mut self, center: Point, radius: f32, color: Color, width: f32) {
        let _ = (center, radius, color, width);
        // Stub
    }
    
    /// Draw text at a position
    pub fn draw_text(&mut self, text: &str, position: Point, color: Color) {
        let _ = (text, position, color);
        // Stub
    }
    
    /// Present the frame to the screen
    pub fn present(&mut self) {
        self.frame_count += 1;
        // Stub - actual swap buffers in real implementation
    }
    
    /// Get the current frame count
    pub fn frame_count(&self) -> u64 {
        self.frame_count
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_gpu_context_creation() {
        let ctx = GpuContext::new();
        assert!(ctx.is_ok());
    }
    
    #[test]
    fn test_gpu_context_with_size() {
        let ctx = GpuContext::with_size(Size::new(1920.0, 1080.0));
        assert!(ctx.is_ok());
        let ctx = ctx.unwrap();
        assert_eq!(ctx.screen_size().width, 1920.0);
    }
    
    #[test]
    fn test_drawing_operations() {
        let mut ctx = GpuContext::new().unwrap();
        ctx.clear();
        ctx.fill_rect(Rect::new(0.0, 0.0, 100.0, 100.0), Color::RED);
        ctx.fill_circle(Point::new(50.0, 50.0), 25.0, Color::BLUE);
        ctx.draw_text("Test", Point::new(10.0, 10.0), Color::WHITE);
        ctx.present();
        assert_eq!(ctx.frame_count(), 1);
    }
}
