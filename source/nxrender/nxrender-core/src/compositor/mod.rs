//! Compositor Module
//!
//! The compositor manages surfaces, layers, and renders them to the screen.
//! It handles z-ordering, blending, damage tracking, and efficient redraws.

mod surface;
mod layer;
mod damage;

pub use surface::{Surface, PixelBuffer, PixelFormat};
pub use layer::{Layer, BlendMode, LayerTransform};
pub use damage::DamageTracker;

use nxgfx::{GpuContext, Size, Rect, Color, Point};
use std::collections::HashMap;

/// Unique identifier for a surface
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct SurfaceId(pub u64);

/// Unique identifier for a layer
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct LayerId(pub u64);

/// Result of a composition operation
#[derive(Debug)]
pub enum CompositeResult {
    /// Nothing changed, frame skipped
    NoChange,
    /// Frame was rendered successfully
    Success {
        /// Number of layers rendered
        layers_rendered: usize,
        /// Percentage of screen that was redrawn
        damage_percent: f32,
    },
    /// Error occurred during composition
    Error(String),
}

/// Compositor configuration
#[derive(Debug, Clone)]
pub struct CompositorConfig {
    /// Enable vsync
    pub vsync: bool,
    /// Enable damage tracking optimization
    pub damage_tracking: bool,
    /// Background color
    pub background_color: Color,
    /// Maximum layers
    pub max_layers: usize,
}

impl Default for CompositorConfig {
    fn default() -> Self {
        Self {
            vsync: true,
            damage_tracking: true,
            background_color: Color::rgb(30, 30, 40),
            max_layers: 256,
        }
    }
}

/// Main compositor for managing surfaces and layers
pub struct Compositor {
    surfaces: HashMap<SurfaceId, Surface>,
    layers: Vec<Layer>,
    damage_tracker: DamageTracker,
    gpu: GpuContext,
    config: CompositorConfig,
    frame_count: u64,
    next_surface_id: u64,
    next_layer_id: u64,
    screen_size: Size,
    needs_sort: bool,
}

impl Compositor {
    /// Create a new compositor
    pub fn new(gpu: GpuContext) -> Self {
        Self::with_config(gpu, CompositorConfig::default())
    }
    
    /// Create with configuration
    pub fn with_config(gpu: GpuContext, config: CompositorConfig) -> Self {
        Self {
            surfaces: HashMap::new(),
            layers: Vec::new(),
            damage_tracker: DamageTracker::new(),
            gpu,
            config,
            frame_count: 0,
            next_surface_id: 1,
            next_layer_id: 1,
            screen_size: Size::new(800.0, 600.0),
            needs_sort: false,
        }
    }
    
    /// Set the screen size
    pub fn set_screen_size(&mut self, size: Size) {
        self.screen_size = size;
        self.damage_tracker.set_bounds(Rect::from_size(size));
        self.damage_tracker.add_full_damage();
    }
    
    /// Get the screen size
    pub fn screen_size(&self) -> Size {
        self.screen_size
    }
    
    /// Create a new surface
    pub fn create_surface(&mut self, size: Size) -> SurfaceId {
        let id = SurfaceId(self.next_surface_id);
        self.next_surface_id += 1;
        
        let surface = Surface::new(id, size);
        self.surfaces.insert(id, surface);
        
        id
    }
    
    /// Create a surface with specific format
    pub fn create_surface_with_format(&mut self, size: Size, format: PixelFormat) -> SurfaceId {
        let id = SurfaceId(self.next_surface_id);
        self.next_surface_id += 1;
        
        let surface = Surface::with_format(id, size, format);
        self.surfaces.insert(id, surface);
        
        id
    }
    
    /// Get a surface by ID
    pub fn get_surface(&self, id: SurfaceId) -> Option<&Surface> {
        self.surfaces.get(&id)
    }
    
    /// Get a surface mutably
    pub fn get_surface_mut(&mut self, id: SurfaceId) -> Option<&mut Surface> {
        self.surfaces.get_mut(&id)
    }
    
    /// Destroy a surface
    pub fn destroy_surface(&mut self, id: SurfaceId) {
        if self.surfaces.remove(&id).is_some() {
            // Remove all layers using this surface
            self.layers.retain(|l| l.surface_id() != id);
            // Mark everything as damaged
            self.damage_tracker.add_full_damage();
        }
    }
    
    /// Add a layer for a surface
    pub fn add_layer(&mut self, surface_id: SurfaceId, z_index: i32) -> Option<LayerId> {
        // Verify surface exists
        if !self.surfaces.contains_key(&surface_id) {
            return None;
        }
        
        if self.layers.len() >= self.config.max_layers {
            log::warn!("Maximum layers reached");
            return None;
        }
        
        let id = LayerId(self.next_layer_id);
        self.next_layer_id += 1;
        
        let layer = Layer::new(id, surface_id, z_index);
        self.layers.push(layer);
        self.needs_sort = true;
        
        Some(id)
    }
    
    /// Get a layer by ID
    pub fn get_layer(&self, id: LayerId) -> Option<&Layer> {
        self.layers.iter().find(|l| l.id() == id)
    }
    
    /// Get a layer mutably
    pub fn get_layer_mut(&mut self, id: LayerId) -> Option<&mut Layer> {
        self.layers.iter_mut().find(|l| l.id() == id)
    }
    
    /// Remove a layer
    pub fn remove_layer(&mut self, id: LayerId) {
        if let Some(idx) = self.layers.iter().position(|l| l.id() == id) {
            let layer = self.layers.remove(idx);
            self.damage_tracker.add_damage(layer.bounds());
        }
    }
    
    /// Move a layer to a new z-index
    pub fn set_layer_z_index(&mut self, id: LayerId, z_index: i32) {
        // Find the layer and get its current state
        let layer_info = self.layers.iter()
            .find(|l| l.id() == id)
            .map(|l| (l.z_index(), l.bounds()));
        
        if let Some((current_z, bounds)) = layer_info {
            if current_z != z_index {
                // Now do the mutation
                if let Some(layer) = self.layers.iter_mut().find(|l| l.id() == id) {
                    layer.set_z_index(z_index);
                }
                self.needs_sort = true;
                self.damage_tracker.add_damage(bounds);
            }
        }
    }
    
    /// Set layer bounds and mark damage
    pub fn set_layer_bounds(&mut self, id: LayerId, new_bounds: Rect) {
        // Get old bounds first
        let old_bounds = self.layers.iter()
            .find(|l| l.id() == id)
            .map(|l| l.bounds());
        
        if let Some(old) = old_bounds {
            // Now do the mutation
            if let Some(layer) = self.layers.iter_mut().find(|l| l.id() == id) {
                layer.set_bounds(new_bounds);
            }
            // Damage both old and new bounds
            self.damage_tracker.add_damage(old);
            self.damage_tracker.add_damage(new_bounds);
        }
    }
    
    /// Sort layers by z-index if needed
    fn sort_layers_if_needed(&mut self) {
        if self.needs_sort {
            self.layers.sort_by_key(|l| l.z_index());
            self.needs_sort = false;
        }
    }
    
    /// Main compositing function - runs every frame
    pub fn composite(&mut self) -> CompositeResult {
        // 0. Sort layers by z-index
        self.sort_layers_if_needed();
        
        // 1. Check if any surfaces are dirty
        let any_dirty = self.surfaces.values().any(|s| s.is_dirty());
        
        // 2. Collect damage regions
        let damage_regions = if self.config.damage_tracking {
            self.damage_tracker.collect()
        } else {
            vec![Rect::from_size(self.screen_size)]
        };
        
        // 3. Skip if nothing changed and no damage
        if !any_dirty && damage_regions.is_empty() {
            return CompositeResult::NoChange;
        }
        
        // 4. Begin frame
        self.gpu.begin_frame();
        self.gpu.clear_color(self.config.background_color);
        self.gpu.clear();
        
        let damage_percent = self.damage_tracker.damage_percentage();
        
        // 5. Collect layer render info to avoid borrow issues
        let mut render_list: Vec<(Rect, f32, Color)> = Vec::new();
        
        for layer in &self.layers {
            if !layer.is_visible() {
                continue;
            }
            
            // Check if layer intersects any damage region
            let effective_bounds = layer.effective_bounds();
            let needs_render = damage_regions.iter().any(|d| d.intersects(&effective_bounds));
            
            if !needs_render && self.config.damage_tracking {
                continue;
            }
            
            // Collect info for rendering
            render_list.push((
                layer.bounds(),
                layer.opacity(),
                self.config.background_color,
            ));
        }
        
        let layers_rendered = render_list.len();
        
        // 6. Actually render the layers
        for (bounds, opacity, _) in render_list {
            // For now, just draw a placeholder rectangle
            let alpha = (opacity * 128.0) as u8;
            self.gpu.fill_rect(bounds, Color::rgba(100, 100, 100, alpha));
        }
        
        // 7. Present
        self.gpu.present();
        self.frame_count += 1;
        
        // 8. Clear damage and dirty flags for next frame
        self.damage_tracker.clear();
        for surface in self.surfaces.values_mut() {
            if surface.is_dirty() {
                surface.swap_buffers();
            }
        }
        
        CompositeResult::Success {
            layers_rendered,
            damage_percent,
        }
    }
    
    /// Mark a region as needing redraw
    pub fn mark_damage(&mut self, region: Rect) {
        self.damage_tracker.add_damage(region);
    }
    
    /// Mark full screen as damaged
    pub fn mark_full_damage(&mut self) {
        self.damage_tracker.add_full_damage();
    }
    
    /// Get frame count
    pub fn frame_count(&self) -> u64 {
        self.frame_count
    }
    
    /// Get number of surfaces
    pub fn surface_count(&self) -> usize {
        self.surfaces.len()
    }
    
    /// Get number of layers
    pub fn layer_count(&self) -> usize {
        self.layers.len()
    }
    
    /// Get visible layer count
    pub fn visible_layer_count(&self) -> usize {
        self.layers.iter().filter(|l| l.is_visible()).count()
    }
    
    /// Get access to the GPU context
    pub fn gpu(&self) -> &GpuContext {
        &self.gpu
    }
    
    /// Get mutable access to the GPU context
    pub fn gpu_mut(&mut self) -> &mut GpuContext {
        &mut self.gpu
    }
    
    /// Iterate over all layers in z-order
    pub fn layers(&self) -> impl Iterator<Item = &Layer> {
        self.layers.iter()
    }
    
    /// Iterate over all surfaces
    pub fn surfaces(&self) -> impl Iterator<Item = (&SurfaceId, &Surface)> {
        self.surfaces.iter()
    }
    
    /// Find layers at a point (for hit testing)
    pub fn layers_at_point(&self, point: Point) -> Vec<LayerId> {
        self.layers
            .iter()
            .rev() // Top to bottom
            .filter(|l| l.is_visible() && l.contains_point(point))
            .map(|l| l.id())
            .collect()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_compositor_creation() {
        let gpu = GpuContext::new().unwrap();
        let compositor = Compositor::new(gpu);
        
        assert_eq!(compositor.surface_count(), 0);
        assert_eq!(compositor.layer_count(), 0);
    }
    
    #[test]
    fn test_surface_lifecycle() {
        let gpu = GpuContext::new().unwrap();
        let mut compositor = Compositor::new(gpu);
        
        let surface_id = compositor.create_surface(Size::new(100.0, 100.0));
        assert_eq!(compositor.surface_count(), 1);
        
        compositor.destroy_surface(surface_id);
        assert_eq!(compositor.surface_count(), 0);
    }
    
    #[test]
    fn test_layer_lifecycle() {
        let gpu = GpuContext::new().unwrap();
        let mut compositor = Compositor::new(gpu);
        
        let surface_id = compositor.create_surface(Size::new(100.0, 100.0));
        let layer_id = compositor.add_layer(surface_id, 0).unwrap();
        
        assert_eq!(compositor.layer_count(), 1);
        
        compositor.remove_layer(layer_id);
        assert_eq!(compositor.layer_count(), 0);
    }
    
    #[test]
    fn test_layer_z_ordering() {
        let gpu = GpuContext::new().unwrap();
        let mut compositor = Compositor::new(gpu);
        
        let s1 = compositor.create_surface(Size::new(100.0, 100.0));
        let s2 = compositor.create_surface(Size::new(100.0, 100.0));
        
        let l1 = compositor.add_layer(s1, 10).unwrap();
        let l2 = compositor.add_layer(s2, 5).unwrap();
        
        compositor.sort_layers_if_needed();
        
        // l2 should come before l1 (lower z-index)
        let layers: Vec<_> = compositor.layers().collect();
        assert_eq!(layers[0].id(), l2);
        assert_eq!(layers[1].id(), l1);
    }
}
