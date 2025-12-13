//! Drawing Primitives
//!
//! Core types for geometry and colors used throughout NXGFX.

mod rect;
mod color;

pub use rect::{Rect, Point, Size};
pub use color::Color;

/// 2D vector
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Vector {
    pub x: f32,
    pub y: f32,
}

impl Vector {
    pub const ZERO: Vector = Vector { x: 0.0, y: 0.0 };
    
    pub fn new(x: f32, y: f32) -> Self {
        Self { x, y }
    }
    
    pub fn length(&self) -> f32 {
        (self.x * self.x + self.y * self.y).sqrt()
    }
    
    pub fn normalize(&self) -> Self {
        let len = self.length();
        if len > 0.0 {
            Self::new(self.x / len, self.y / len)
        } else {
            *self
        }
    }
}

/// Edge insets for padding/margin
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct EdgeInsets {
    pub top: f32,
    pub right: f32,
    pub bottom: f32,
    pub left: f32,
}

impl EdgeInsets {
    pub const ZERO: EdgeInsets = EdgeInsets {
        top: 0.0,
        right: 0.0,
        bottom: 0.0,
        left: 0.0,
    };
    
    pub fn all(value: f32) -> Self {
        Self {
            top: value,
            right: value,
            bottom: value,
            left: value,
        }
    }
    
    pub fn symmetric(horizontal: f32, vertical: f32) -> Self {
        Self {
            top: vertical,
            right: horizontal,
            bottom: vertical,
            left: horizontal,
        }
    }
    
    pub fn horizontal(&self) -> f32 {
        self.left + self.right
    }
    
    pub fn vertical(&self) -> f32 {
        self.top + self.bottom
    }
}

impl Default for EdgeInsets {
    fn default() -> Self {
        Self::ZERO
    }
}
