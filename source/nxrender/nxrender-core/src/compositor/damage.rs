//! Damage Tracking
//!
//! Tracks which regions of the screen have changed and need
//! to be redrawn. This optimization saves GPU cycles when
//! most of the screen is static.

use nxgfx::Rect;

/// Tracks damaged (changed) regions of the screen
pub struct DamageTracker {
    /// List of damaged rectangles
    regions: Vec<Rect>,
    /// Whether full redraw is needed
    full_damage: bool,
    /// Screen bounds for full damage
    screen_bounds: Rect,
}

impl DamageTracker {
    /// Create a new damage tracker
    pub fn new() -> Self {
        Self {
            regions: Vec::new(),
            full_damage: true, // Start with full redraw
            screen_bounds: Rect::ZERO,
        }
    }
    
    /// Create with screen bounds
    pub fn with_bounds(bounds: Rect) -> Self {
        Self {
            regions: Vec::new(),
            full_damage: true,
            screen_bounds: bounds,
        }
    }
    
    /// Set the screen bounds
    pub fn set_bounds(&mut self, bounds: Rect) {
        self.screen_bounds = bounds;
    }
    
    /// Add a damaged region
    pub fn add_damage(&mut self, rect: Rect) {
        if rect.width <= 0.0 || rect.height <= 0.0 {
            return;
        }
        
        // Clip to screen bounds
        if let Some(clipped) = rect.intersection(&self.screen_bounds) {
            self.regions.push(clipped);
        }
    }
    
    /// Mark the entire screen as damaged
    pub fn add_full_damage(&mut self) {
        self.full_damage = true;
    }
    
    /// Check if full redraw is needed
    pub fn needs_full_redraw(&self) -> bool {
        self.full_damage
    }
    
    /// Collect and optimize damage regions
    pub fn collect(&self) -> Vec<Rect> {
        if self.full_damage {
            return vec![self.screen_bounds];
        }
        
        if self.regions.is_empty() {
            return Vec::new();
        }
        
        // Merge overlapping rectangles
        self.merge_regions()
    }
    
    /// Merge overlapping or adjacent rectangles
    fn merge_regions(&self) -> Vec<Rect> {
        if self.regions.len() <= 1 {
            return self.regions.clone();
        }
        
        let mut merged: Vec<Rect> = Vec::new();
        let mut remaining: Vec<Rect> = self.regions.clone();
        
        while let Some(current) = remaining.pop() {
            let mut current = current;
            let mut merged_any = true;
            
            while merged_any {
                merged_any = false;
                let mut new_remaining = Vec::new();
                
                for rect in remaining.drain(..) {
                    if current.intersects(&rect) || current.touches(&rect) {
                        // Merge into bounding box
                        current = current.union(&rect);
                        merged_any = true;
                    } else {
                        new_remaining.push(rect);
                    }
                }
                
                remaining = new_remaining;
            }
            
            merged.push(current);
        }
        
        merged
    }
    
    /// Check if any region intersects with a given rect
    pub fn intersects(&self, rect: &Rect) -> bool {
        if self.full_damage {
            return self.screen_bounds.intersects(rect);
        }
        self.regions.iter().any(|r| r.intersects(rect))
    }
    
    /// Check if there is any damage
    pub fn is_empty(&self) -> bool {
        !self.full_damage && self.regions.is_empty()
    }
    
    /// Clear all damage for the next frame
    pub fn clear(&mut self) {
        self.regions.clear();
        self.full_damage = false;
    }
    
    /// Get the number of damage regions
    pub fn region_count(&self) -> usize {
        if self.full_damage {
            1
        } else {
            self.regions.len()
        }
    }
    
    /// Get total damaged area in pixels
    pub fn damaged_area(&self) -> f32 {
        if self.full_damage {
            return self.screen_bounds.width * self.screen_bounds.height;
        }
        
        self.collect().iter().map(|r| r.width * r.height).sum()
    }
    
    /// Get damage percentage (0.0 - 1.0)
    pub fn damage_percentage(&self) -> f32 {
        let screen_area = self.screen_bounds.width * self.screen_bounds.height;
        if screen_area <= 0.0 {
            return 1.0;
        }
        (self.damaged_area() / screen_area).min(1.0)
    }
}

impl Default for DamageTracker {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_damage_tracker_empty() {
        let mut tracker = DamageTracker::with_bounds(Rect::new(0.0, 0.0, 800.0, 600.0));
        tracker.clear();
        
        assert!(tracker.is_empty());
        assert_eq!(tracker.region_count(), 0);
    }
    
    #[test]
    fn test_damage_tracker_add() {
        let mut tracker = DamageTracker::with_bounds(Rect::new(0.0, 0.0, 800.0, 600.0));
        tracker.clear();
        
        tracker.add_damage(Rect::new(10.0, 10.0, 100.0, 100.0));
        
        assert!(!tracker.is_empty());
        assert_eq!(tracker.region_count(), 1);
    }
    
    #[test]
    fn test_damage_tracker_merge() {
        let mut tracker = DamageTracker::with_bounds(Rect::new(0.0, 0.0, 800.0, 600.0));
        tracker.clear();
        
        // Add two overlapping rectangles
        tracker.add_damage(Rect::new(0.0, 0.0, 100.0, 100.0));
        tracker.add_damage(Rect::new(50.0, 50.0, 100.0, 100.0));
        
        let collected = tracker.collect();
        
        // Should be merged into one
        assert_eq!(collected.len(), 1);
        
        // Merged rect should contain both
        let merged = &collected[0];
        assert_eq!(merged.x, 0.0);
        assert_eq!(merged.y, 0.0);
        assert_eq!(merged.width, 150.0);
        assert_eq!(merged.height, 150.0);
    }
    
    #[test]
    fn test_damage_tracker_full_damage() {
        let mut tracker = DamageTracker::with_bounds(Rect::new(0.0, 0.0, 800.0, 600.0));
        tracker.add_full_damage();
        
        assert!(tracker.needs_full_redraw());
        assert_eq!(tracker.damage_percentage(), 1.0);
    }
}
