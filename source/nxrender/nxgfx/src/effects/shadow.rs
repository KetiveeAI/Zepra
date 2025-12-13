//! Shadow Rendering
//!
//! Provides drop shadows and box shadows for UI elements.

use crate::primitives::{Color, Rect, Point};

/// Shadow configuration
#[derive(Debug, Clone, Copy)]
pub struct Shadow {
    /// Horizontal offset
    pub offset_x: f32,
    /// Vertical offset
    pub offset_y: f32,
    /// Blur radius (larger = more blur)
    pub blur_radius: f32,
    /// Spread radius (larger = bigger shadow)
    pub spread_radius: f32,
    /// Shadow color
    pub color: Color,
    /// Whether shadow is inset (inside the element)
    pub inset: bool,
}

impl Shadow {
    /// Create a new shadow
    pub fn new(offset_x: f32, offset_y: f32, blur_radius: f32, color: Color) -> Self {
        Self {
            offset_x,
            offset_y,
            blur_radius,
            spread_radius: 0.0,
            color,
            inset: false,
        }
    }
    
    /// Create a drop shadow with default styling
    pub fn drop(blur_radius: f32) -> Self {
        Self {
            offset_x: 0.0,
            offset_y: blur_radius / 2.0,
            blur_radius,
            spread_radius: 0.0,
            color: Color::rgba(0, 0, 0, 64),
            inset: false,
        }
    }
    
    /// Create an elevation shadow (like Material Design)
    pub fn elevation(level: u8) -> Self {
        let level = level.min(5);
        let blur = match level {
            0 => 0.0,
            1 => 2.0,
            2 => 4.0,
            3 => 8.0,
            4 => 12.0,
            5 => 16.0,
            _ => 0.0,
        };
        let alpha = match level {
            0 => 0,
            1 => 20,
            2 => 30,
            3 => 40,
            4 => 50,
            5 => 60,
            _ => 0,
        };
        
        Self {
            offset_x: 0.0,
            offset_y: level as f32,
            blur_radius: blur,
            spread_radius: 0.0,
            color: Color::rgba(0, 0, 0, alpha),
            inset: false,
        }
    }
    
    /// Create an inset shadow
    pub fn inset(blur_radius: f32) -> Self {
        Self {
            offset_x: 0.0,
            offset_y: 0.0,
            blur_radius,
            spread_radius: 0.0,
            color: Color::rgba(0, 0, 0, 32),
            inset: true,
        }
    }
    
    /// Set the offset
    pub fn offset(mut self, x: f32, y: f32) -> Self {
        self.offset_x = x;
        self.offset_y = y;
        self
    }
    
    /// Set the spread radius
    pub fn spread(mut self, radius: f32) -> Self {
        self.spread_radius = radius;
        self
    }
    
    /// Set the color
    pub fn color(mut self, color: Color) -> Self {
        self.color = color;
        self
    }
    
    /// Calculate the shadow bounds for a given element rect
    pub fn bounds(&self, element_rect: Rect) -> Rect {
        let expand = self.blur_radius + self.spread_radius;
        Rect::new(
            element_rect.x + self.offset_x - expand,
            element_rect.y + self.offset_y - expand,
            element_rect.width + expand * 2.0,
            element_rect.height + expand * 2.0,
        )
    }
    
    /// Generate shadow layers for rendering
    /// Returns a list of (rect, color) pairs from outer to inner
    pub fn generate_layers(&self, element_rect: Rect, num_layers: u32) -> Vec<(Rect, Color)> {
        if self.blur_radius <= 0.0 {
            // No blur, just offset solid shadow
            let shadow_rect = Rect::new(
                element_rect.x + self.offset_x,
                element_rect.y + self.offset_y,
                element_rect.width + self.spread_radius * 2.0,
                element_rect.height + self.spread_radius * 2.0,
            );
            return vec![(shadow_rect, self.color)];
        }
        
        let num_layers = num_layers.max(1);
        let mut layers = Vec::with_capacity(num_layers as usize);
        
        let step = self.blur_radius / num_layers as f32;
        let alpha_step = self.color.a as f32 / num_layers as f32;
        
        for i in 0..num_layers {
            let expand = self.blur_radius - (step * i as f32) + self.spread_radius;
            let alpha = (alpha_step * (i + 1) as f32).min(255.0) as u8;
            
            let shadow_rect = Rect::new(
                element_rect.x + self.offset_x - expand,
                element_rect.y + self.offset_y - expand,
                element_rect.width + expand * 2.0,
                element_rect.height + expand * 2.0,
            );
            
            let layer_color = Color::rgba(
                self.color.r,
                self.color.g,
                self.color.b,
                alpha / num_layers as u8,
            );
            
            layers.push((shadow_rect, layer_color));
        }
        
        layers
    }
}

impl Default for Shadow {
    fn default() -> Self {
        Self::elevation(2)
    }
}

/// Multiple shadows for an element
#[derive(Debug, Clone, Default)]
pub struct BoxShadows {
    shadows: Vec<Shadow>,
}

impl BoxShadows {
    pub fn new() -> Self {
        Self { shadows: Vec::new() }
    }
    
    pub fn add(mut self, shadow: Shadow) -> Self {
        self.shadows.push(shadow);
        self
    }
    
    pub fn shadows(&self) -> &[Shadow] {
        &self.shadows
    }
    
    pub fn is_empty(&self) -> bool {
        self.shadows.is_empty()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_shadow_bounds() {
        let shadow = Shadow::new(5.0, 5.0, 10.0, Color::BLACK);
        let element = Rect::new(100.0, 100.0, 50.0, 50.0);
        let bounds = shadow.bounds(element);
        
        assert_eq!(bounds.x, 95.0); // 100 + 5 - 10
        assert_eq!(bounds.y, 95.0);
        assert_eq!(bounds.width, 70.0); // 50 + 20
        assert_eq!(bounds.height, 70.0);
    }
    
    #[test]
    fn test_elevation_levels() {
        let shadow0 = Shadow::elevation(0);
        assert_eq!(shadow0.blur_radius, 0.0);
        
        let shadow5 = Shadow::elevation(5);
        assert!(shadow5.blur_radius > 0.0);
        assert!(shadow5.color.a > 0);
    }
}
