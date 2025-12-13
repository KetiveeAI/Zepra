//! High-level Painter API

use nxgfx::{GpuContext, Color, Rect, Point};

/// High-level drawing API for widgets
pub struct Painter<'a> {
    gpu: &'a mut GpuContext,
    clip_stack: Vec<Rect>,
}

impl<'a> Painter<'a> {
    /// Create a new painter
    pub fn new(gpu: &'a mut GpuContext) -> Self {
        Self {
            gpu,
            clip_stack: Vec::new(),
        }
    }
    
    /// Fill a rectangle
    pub fn fill_rect(&mut self, rect: Rect, color: Color) {
        self.gpu.fill_rect(rect, color);
    }
    
    /// Stroke a rectangle
    pub fn stroke_rect(&mut self, rect: Rect, color: Color, width: f32) {
        self.gpu.stroke_rect(rect, color, width);
    }
    
    /// Fill a rounded rectangle
    pub fn fill_rounded_rect(&mut self, rect: Rect, color: Color, radius: f32) {
        self.gpu.fill_rounded_rect(rect, color, radius);
    }
    
    /// Stroke a rounded rectangle
    pub fn stroke_rounded_rect(&mut self, rect: Rect, color: Color, width: f32, radius: f32) {
        self.gpu.stroke_rounded_rect(rect, color, width, radius);
    }
    
    /// Fill a circle
    pub fn fill_circle(&mut self, center: Point, radius: f32, color: Color) {
        self.gpu.fill_circle(center, radius, color);
    }
    
    /// Stroke a circle
    pub fn stroke_circle(&mut self, center: Point, radius: f32, color: Color, width: f32) {
        self.gpu.stroke_circle(center, radius, color, width);
    }
    
    /// Draw text
    pub fn draw_text(&mut self, text: &str, pos: Point, color: Color) {
        self.gpu.draw_text(text, pos, color);
    }
    
    /// Push a clipping rectangle
    pub fn push_clip(&mut self, rect: Rect) {
        self.clip_stack.push(rect);
        // TODO: Apply clipping
    }
    
    /// Pop the last clipping rectangle
    pub fn pop_clip(&mut self) {
        self.clip_stack.pop();
        // TODO: Restore clipping
    }
    
    /// Clear with a color
    pub fn clear(&mut self, color: Color) {
        self.gpu.set_clear_color(color);
        self.gpu.clear();
    }
}
