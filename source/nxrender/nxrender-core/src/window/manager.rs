//! Window Manager
//!
//! Manages the lifecycle and ordering of all windows in the application.

use super::{Window, WindowId, WindowLevel, WindowState};
use super::focus::FocusManager;
use nxgfx::{Point, Rect};
use std::collections::HashMap;

/// Window manager for handling multiple windows
pub struct WindowManager {
    windows: HashMap<WindowId, Window>,
    /// Window ordering (front to back)
    z_order: Vec<WindowId>,
    /// Focus management
    focus: FocusManager,
    /// Screen bounds for maximizing
    screen_bounds: Rect,
}

impl WindowManager {
    /// Create a new window manager
    pub fn new() -> Self {
        Self {
            windows: HashMap::new(),
            z_order: Vec::new(),
            focus: FocusManager::new(),
            screen_bounds: Rect::new(0.0, 0.0, 1920.0, 1080.0),
        }
    }
    
    /// Set the screen bounds
    pub fn set_screen_bounds(&mut self, bounds: Rect) {
        self.screen_bounds = bounds;
    }
    
    /// Get the screen bounds
    pub fn screen_bounds(&self) -> Rect {
        self.screen_bounds
    }
    
    /// Add a window and give it focus
    pub fn add(&mut self, window: Window) -> WindowId {
        let id = window.id();
        let level = window.level();
        
        self.windows.insert(id, window);
        self.insert_in_z_order(id, level);
        
        // Focus the new window if it's visible
        if let Some(w) = self.windows.get(&id) {
            if w.is_visible() {
                self.focus_window(id);
            }
        }
        
        id
    }
    
    /// Create and add a window
    pub fn create(&mut self, title: impl Into<String>) -> WindowId {
        let window = Window::new(title);
        self.add(window)
    }
    
    /// Get a window by ID
    pub fn get(&self, id: WindowId) -> Option<&Window> {
        self.windows.get(&id)
    }
    
    /// Get a mutable window by ID
    pub fn get_mut(&mut self, id: WindowId) -> Option<&mut Window> {
        self.windows.get_mut(&id)
    }
    
    /// Remove a window
    pub fn remove(&mut self, id: WindowId) -> Option<Window> {
        self.z_order.retain(|&wid| wid != id);
        
        if self.focus.focused_window() == Some(id) {
            // Focus next window
            if let Some(&next) = self.z_order.first() {
                self.focus_window(next);
            } else {
                self.focus.clear_focus();
            }
        }
        
        self.windows.remove(&id)
    }
    
    /// Close a window (same as remove)
    pub fn close(&mut self, id: WindowId) -> Option<Window> {
        self.remove(id)
    }
    
    /// Get the number of windows
    pub fn count(&self) -> usize {
        self.windows.len()
    }
    
    /// Check if a window exists
    pub fn contains(&self, id: WindowId) -> bool {
        self.windows.contains_key(&id)
    }
    
    /// Get all window IDs in z-order (front to back)
    pub fn window_ids(&self) -> &[WindowId] {
        &self.z_order
    }
    
    /// Iterate windows in z-order (front to back)
    pub fn windows_front_to_back(&self) -> impl Iterator<Item = &Window> {
        self.z_order.iter().filter_map(|id| self.windows.get(id))
    }
    
    /// Iterate windows in reverse z-order (back to front, for rendering)
    pub fn windows_back_to_front(&self) -> impl Iterator<Item = &Window> {
        self.z_order.iter().rev().filter_map(|id| self.windows.get(id))
    }
    
    /// Insert a window in the z-order based on its level (at front of its group)
    fn insert_in_z_order(&mut self, id: WindowId, level: WindowLevel) {
        // Find the first window with strictly lower priority - we insert before it
        // This puts the new window at the FRONT of its priority level
        let insert_pos = self.z_order.iter().position(|&wid| {
            if let Some(w) = self.windows.get(&wid) {
                window_level_priority(w.level()) <= window_level_priority(level)
            } else {
                true
            }
        }).unwrap_or(0);
        
        self.z_order.insert(insert_pos, id);
    }
    
    /// Bring a window to the front
    pub fn bring_to_front(&mut self, id: WindowId) {
        if let Some(window) = self.windows.get(&id) {
            let level = window.level();
            self.z_order.retain(|&wid| wid != id);
            self.insert_in_z_order(id, level);
        }
    }
    
    /// Send a window to the back
    pub fn send_to_back(&mut self, id: WindowId) {
        if let Some(pos) = self.z_order.iter().position(|&wid| wid == id) {
            let id = self.z_order.remove(pos);
            self.z_order.push(id);
        }
    }
    
    /// Focus a window
    pub fn focus_window(&mut self, id: WindowId) {
        // Unfocus current window
        if let Some(current) = self.focus.focused_window() {
            if let Some(window) = self.windows.get_mut(&current) {
                window.set_focused(false);
            }
        }
        
        // Focus new window
        if let Some(window) = self.windows.get_mut(&id) {
            window.set_focused(true);
            self.focus.set_focused_window(id);
            self.bring_to_front(id);
        }
    }
    
    /// Get the focused window
    pub fn focused_window(&self) -> Option<&Window> {
        self.focus.focused_window().and_then(|id| self.windows.get(&id))
    }
    
    /// Get the focused window ID
    pub fn focused_window_id(&self) -> Option<WindowId> {
        self.focus.focused_window()
    }
    
    /// Find window at a point (topmost)
    pub fn window_at_point(&self, point: Point) -> Option<WindowId> {
        for &id in &self.z_order {
            if let Some(window) = self.windows.get(&id) {
                if window.is_visible() && window.contains_point(point) {
                    return Some(id);
                }
            }
        }
        None
    }
    
    /// Maximize a window
    pub fn maximize(&mut self, id: WindowId) {
        if let Some(window) = self.windows.get_mut(&id) {
            window.maximize(self.screen_bounds);
        }
    }
    
    /// Minimize a window
    pub fn minimize(&mut self, id: WindowId) {
        if let Some(window) = self.windows.get_mut(&id) {
            window.minimize();
            
            // Focus next window
            if self.focus.focused_window() == Some(id) {
                if let Some(&next) = self.z_order.iter()
                    .filter(|&&wid| wid != id)
                    .find(|&&wid| self.windows.get(&wid).map(|w| w.is_visible()).unwrap_or(false))
                {
                    self.focus_window(next);
                }
            }
        }
    }
    
    /// Restore a window from minimized/maximized
    pub fn restore(&mut self, id: WindowId) {
        if let Some(window) = self.windows.get_mut(&id) {
            window.restore();
        }
    }
    
    /// Show a window
    pub fn show(&mut self, id: WindowId) {
        if let Some(window) = self.windows.get_mut(&id) {
            window.show();
            self.focus_window(id);
        }
    }
    
    /// Hide a window
    pub fn hide(&mut self, id: WindowId) {
        if let Some(window) = self.windows.get_mut(&id) {
            window.hide();
            
            if self.focus.focused_window() == Some(id) {
                // Focus next visible window
                if let Some(&next) = self.z_order.iter()
                    .filter(|&&wid| wid != id)
                    .find(|&&wid| self.windows.get(&wid).map(|w| w.is_visible()).unwrap_or(false))
                {
                    self.focus_window(next);
                }
            }
        }
    }
    
    /// Get child windows of a parent
    pub fn children(&self, parent: WindowId) -> Vec<WindowId> {
        self.windows
            .iter()
            .filter(|(_, w)| w.parent() == Some(parent))
            .map(|(id, _)| *id)
            .collect()
    }
    
    /// Close all child windows of a parent
    pub fn close_children(&mut self, parent: WindowId) {
        let children: Vec<_> = self.children(parent);
        for child in children {
            self.close(child);
        }
    }
    
    /// Get focus manager
    pub fn focus_manager(&self) -> &FocusManager {
        &self.focus
    }
    
    /// Get mutable focus manager
    pub fn focus_manager_mut(&mut self) -> &mut FocusManager {
        &mut self.focus
    }
}

impl Default for WindowManager {
    fn default() -> Self {
        Self::new()
    }
}

/// Get priority for window level (higher = more in front)
fn window_level_priority(level: WindowLevel) -> i32 {
    match level {
        WindowLevel::Background => -100,
        WindowLevel::Normal => 0,
        WindowLevel::Floating => 100,
        WindowLevel::Modal => 200,
        WindowLevel::Popup => 300,
        WindowLevel::AlwaysOnTop => 1000,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_window_manager_create() {
        let mut wm = WindowManager::new();
        let id = wm.create("Test Window");
        
        assert_eq!(wm.count(), 1);
        assert!(wm.contains(id));
    }
    
    #[test]
    fn test_window_focus() {
        let mut wm = WindowManager::new();
        let id1 = wm.create("Window 1");
        let id2 = wm.create("Window 2");
        
        // Last created should be focused
        assert_eq!(wm.focused_window_id(), Some(id2));
        
        wm.focus_window(id1);
        assert_eq!(wm.focused_window_id(), Some(id1));
    }
    
    #[test]
    fn test_window_z_order() {
        let mut wm = WindowManager::new();
        let id1 = wm.create("Window 1");
        let id2 = wm.create("Window 2");
        
        // Both windows are created, id2 was focused last so it's at front
        // The behavior is: new window gets focus → bring_to_front is called
        
        // Bring id1 to front explicitly
        wm.bring_to_front(id1);
        
        // Now id1 should be at front
        let windows: Vec<_> = wm.window_ids().iter().collect();
        assert!(windows.contains(&&id1));
        assert!(windows.contains(&&id2));
        
        // Check that bring_to_front moved id1 to index 0
        assert_eq!(wm.window_ids()[0], id1);
    }
    
    #[test]
    fn test_window_at_point() {
        let mut wm = WindowManager::new();
        
        // Create windows with known positions
        let mut w1 = Window::new("Window 1");
        w1.set_bounds(Rect::new(0.0, 0.0, 200.0, 200.0));
        let id1 = wm.add(w1);
        
        let mut w2 = Window::new("Window 2");
        w2.set_bounds(Rect::new(100.0, 100.0, 200.0, 200.0));
        let id2 = wm.add(w2);
        
        // Explicitly bring w2 to front
        wm.bring_to_front(id2);
        
        // Point in overlap should find topmost (id2, since it's at front)
        assert_eq!(wm.window_at_point(Point::new(150.0, 150.0)), Some(id2));
        
        // Point only in w1 should find id1
        assert_eq!(wm.window_at_point(Point::new(50.0, 50.0)), Some(id1));
        
        // Point outside both should return None
        assert_eq!(wm.window_at_point(Point::new(500.0, 500.0)), None);
    }
}
