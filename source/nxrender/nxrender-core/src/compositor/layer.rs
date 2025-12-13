//! Layer Management
//!
//! Layers are ordered containers for surfaces with blending and transforms.

use super::{LayerId, SurfaceId};
use nxgfx::{Rect, Point};

/// Blend mode for layer compositing
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum BlendMode {
    /// Normal alpha blending: result = src * alpha + dst * (1 - alpha)
    #[default]
    Normal,
    /// Multiply: result = src * dst
    Multiply,
    /// Screen: result = 1 - (1 - src) * (1 - dst)
    Screen,
    /// Overlay: combines multiply and screen
    Overlay,
    /// Add: result = src + dst
    Add,
    /// Subtract: result = dst - src
    Subtract,
    /// Difference: result = |src - dst|
    Difference,
    /// Source only (no blending)
    Source,
    /// Destination only
    Destination,
}

impl BlendMode {
    /// Apply this blend mode to blend src over dst
    pub fn blend(&self, src: [f32; 4], dst: [f32; 4]) -> [f32; 4] {
        let [sr, sg, sb, sa] = src;
        let [dr, dg, db, da] = dst;
        
        match self {
            Self::Normal => {
                // Standard alpha blending
                let out_a = sa + da * (1.0 - sa);
                if out_a == 0.0 {
                    return [0.0, 0.0, 0.0, 0.0];
                }
                let out_r = (sr * sa + dr * da * (1.0 - sa)) / out_a;
                let out_g = (sg * sa + dg * da * (1.0 - sa)) / out_a;
                let out_b = (sb * sa + db * da * (1.0 - sa)) / out_a;
                [out_r, out_g, out_b, out_a]
            }
            Self::Multiply => {
                [sr * dr, sg * dg, sb * db, sa * da]
            }
            Self::Screen => {
                [
                    1.0 - (1.0 - sr) * (1.0 - dr),
                    1.0 - (1.0 - sg) * (1.0 - dg),
                    1.0 - (1.0 - sb) * (1.0 - db),
                    sa + da * (1.0 - sa),
                ]
            }
            Self::Overlay => {
                fn overlay_channel(s: f32, d: f32) -> f32 {
                    if d < 0.5 {
                        2.0 * s * d
                    } else {
                        1.0 - 2.0 * (1.0 - s) * (1.0 - d)
                    }
                }
                [
                    overlay_channel(sr, dr),
                    overlay_channel(sg, dg),
                    overlay_channel(sb, db),
                    sa + da * (1.0 - sa),
                ]
            }
            Self::Add => {
                [
                    (sr + dr).min(1.0),
                    (sg + dg).min(1.0),
                    (sb + db).min(1.0),
                    (sa + da).min(1.0),
                ]
            }
            Self::Subtract => {
                [
                    (dr - sr).max(0.0),
                    (dg - sg).max(0.0),
                    (db - sb).max(0.0),
                    da,
                ]
            }
            Self::Difference => {
                [
                    (sr - dr).abs(),
                    (sg - dg).abs(),
                    (sb - db).abs(),
                    sa + da * (1.0 - sa),
                ]
            }
            Self::Source => src,
            Self::Destination => dst,
        }
    }
}

/// Transform for a layer
#[derive(Debug, Clone, Copy)]
pub struct LayerTransform {
    /// Translation offset
    pub translation: Point,
    /// Scale factor (1.0 = no scale)
    pub scale: f32,
    /// Rotation in degrees
    pub rotation: f32,
    /// Anchor point for rotation/scale (relative to bounds)
    pub anchor: Point,
}

impl Default for LayerTransform {
    fn default() -> Self {
        Self {
            translation: Point::ZERO,
            scale: 1.0,
            rotation: 0.0,
            anchor: Point::new(0.5, 0.5), // Center
        }
    }
}

impl LayerTransform {
    /// Identity transform (no change)
    pub fn identity() -> Self {
        Self::default()
    }
    
    /// Apply translation
    pub fn translate(mut self, x: f32, y: f32) -> Self {
        self.translation.x += x;
        self.translation.y += y;
        self
    }
    
    /// Apply scale
    pub fn scale(mut self, factor: f32) -> Self {
        self.scale *= factor;
        self
    }
    
    /// Apply rotation
    pub fn rotate(mut self, degrees: f32) -> Self {
        self.rotation += degrees;
        self
    }
    
    /// Transform a point
    pub fn apply(&self, point: Point, bounds: Rect) -> Point {
        // Get anchor in absolute coordinates
        let anchor_x = bounds.x + bounds.width * self.anchor.x;
        let anchor_y = bounds.y + bounds.height * self.anchor.y;
        
        // Translate to anchor
        let mut x = point.x - anchor_x;
        let mut y = point.y - anchor_y;
        
        // Scale
        x *= self.scale;
        y *= self.scale;
        
        // Rotate
        if self.rotation != 0.0 {
            let radians = self.rotation.to_radians();
            let cos = radians.cos();
            let sin = radians.sin();
            let new_x = x * cos - y * sin;
            let new_y = x * sin + y * cos;
            x = new_x;
            y = new_y;
        }
        
        // Translate back and add translation offset
        Point::new(
            x + anchor_x + self.translation.x,
            y + anchor_y + self.translation.y,
        )
    }
}

/// A layer in the compositor
pub struct Layer {
    pub(crate) id: LayerId,
    pub(crate) surface_id: SurfaceId,
    pub(crate) bounds: Rect,
    pub(crate) z_index: i32,
    pub(crate) opacity: f32,
    pub(crate) blend_mode: BlendMode,
    pub(crate) visible: bool,
    pub(crate) transform: LayerTransform,
    pub(crate) clip_to_bounds: bool,
    pub(crate) mask_layer: Option<LayerId>,
}

impl Layer {
    /// Create a new layer
    pub fn new(id: LayerId, surface_id: SurfaceId, z_index: i32) -> Self {
        Self {
            id,
            surface_id,
            bounds: Rect::ZERO,
            z_index,
            opacity: 1.0,
            blend_mode: BlendMode::Normal,
            visible: true,
            transform: LayerTransform::default(),
            clip_to_bounds: true,
            mask_layer: None,
        }
    }
    
    /// Get the layer ID
    pub fn id(&self) -> LayerId {
        self.id
    }
    
    /// Get the surface ID
    pub fn surface_id(&self) -> SurfaceId {
        self.surface_id
    }
    
    /// Get the bounds
    pub fn bounds(&self) -> Rect {
        self.bounds
    }
    
    /// Set the layer bounds
    pub fn set_bounds(&mut self, bounds: Rect) {
        self.bounds = bounds;
    }
    
    /// Get the z-index
    pub fn z_index(&self) -> i32 {
        self.z_index
    }
    
    /// Set the z-index
    pub fn set_z_index(&mut self, z_index: i32) {
        self.z_index = z_index;
    }
    
    /// Get the opacity
    pub fn opacity(&self) -> f32 {
        self.opacity
    }
    
    /// Set the opacity (0.0 - 1.0)
    pub fn set_opacity(&mut self, opacity: f32) {
        self.opacity = opacity.clamp(0.0, 1.0);
    }
    
    /// Get the blend mode
    pub fn blend_mode(&self) -> BlendMode {
        self.blend_mode
    }
    
    /// Set the blend mode
    pub fn set_blend_mode(&mut self, mode: BlendMode) {
        self.blend_mode = mode;
    }
    
    /// Check if visible
    pub fn is_visible(&self) -> bool {
        self.visible && self.opacity > 0.0
    }
    
    /// Set visibility
    pub fn set_visible(&mut self, visible: bool) {
        self.visible = visible;
    }
    
    /// Get the transform
    pub fn transform(&self) -> &LayerTransform {
        &self.transform
    }
    
    /// Set the transform
    pub fn set_transform(&mut self, transform: LayerTransform) {
        self.transform = transform;
    }
    
    /// Set whether to clip content to bounds
    pub fn set_clip_to_bounds(&mut self, clip: bool) {
        self.clip_to_bounds = clip;
    }
    
    /// Set a mask layer
    pub fn set_mask(&mut self, mask_id: Option<LayerId>) {
        self.mask_layer = mask_id;
    }
    
    /// Check if a point is inside the layer bounds
    pub fn contains_point(&self, point: Point) -> bool {
        self.bounds.contains(point)
    }
    
    /// Get effective bounds after transform
    pub fn effective_bounds(&self) -> Rect {
        if self.transform.scale == 1.0 && self.transform.rotation == 0.0 {
            // Simple case: just translation
            Rect::new(
                self.bounds.x + self.transform.translation.x,
                self.bounds.y + self.transform.translation.y,
                self.bounds.width,
                self.bounds.height,
            )
        } else {
            // Complex case: need to compute transformed bounding box
            let corners = [
                Point::new(self.bounds.x, self.bounds.y),
                Point::new(self.bounds.x + self.bounds.width, self.bounds.y),
                Point::new(self.bounds.x + self.bounds.width, self.bounds.y + self.bounds.height),
                Point::new(self.bounds.x, self.bounds.y + self.bounds.height),
            ];
            
            let transformed: Vec<Point> = corners
                .iter()
                .map(|p| self.transform.apply(*p, self.bounds))
                .collect();
            
            let min_x = transformed.iter().map(|p| p.x).fold(f32::INFINITY, f32::min);
            let min_y = transformed.iter().map(|p| p.y).fold(f32::INFINITY, f32::min);
            let max_x = transformed.iter().map(|p| p.x).fold(f32::NEG_INFINITY, f32::max);
            let max_y = transformed.iter().map(|p| p.y).fold(f32::NEG_INFINITY, f32::max);
            
            Rect::new(min_x, min_y, max_x - min_x, max_y - min_y)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_blend_mode_normal() {
        let src = [1.0, 0.0, 0.0, 0.5]; // Semi-transparent red
        let dst = [0.0, 1.0, 0.0, 1.0]; // Opaque green
        
        let result = BlendMode::Normal.blend(src, dst);
        
        // Should be a blend of red and green
        assert!(result[0] > 0.0); // Some red
        assert!(result[1] > 0.0); // Some green
    }
    
    #[test]
    fn test_layer_transform() {
        let transform = LayerTransform::identity()
            .translate(10.0, 20.0);
        
        let bounds = Rect::new(0.0, 0.0, 100.0, 100.0);
        let point = Point::new(50.0, 50.0);
        
        let result = transform.apply(point, bounds);
        
        assert_eq!(result.x, 60.0);
        assert_eq!(result.y, 70.0);
    }
    
    #[test]
    fn test_layer_visibility() {
        let mut layer = Layer::new(LayerId(1), SurfaceId(1), 0);
        assert!(layer.is_visible());
        
        layer.set_opacity(0.0);
        assert!(!layer.is_visible());
        
        layer.set_opacity(1.0);
        layer.set_visible(false);
        assert!(!layer.is_visible());
    }
}
