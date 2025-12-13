//! Focus Management
//!
//! Manages window and widget focus for keyboard input routing.

use super::window::WindowId;
use std::collections::VecDeque;

/// Widget identifier for focus tracking
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct WidgetId(pub u64);

impl WidgetId {
    /// Generate a new unique widget ID
    pub fn new() -> Self {
        use std::sync::atomic::{AtomicU64, Ordering};
        static NEXT_ID: AtomicU64 = AtomicU64::new(1);
        Self(NEXT_ID.fetch_add(1, Ordering::SeqCst))
    }
}

impl Default for WidgetId {
    fn default() -> Self {
        Self::new()
    }
}

/// Focus direction for keyboard navigation
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FocusDirection {
    /// Move to next focusable element
    Forward,
    /// Move to previous focusable element
    Backward,
    /// Move up
    Up,
    /// Move down
    Down,
    /// Move left
    Left,
    /// Move right
    Right,
}

/// Focus scope for grouping focusable widgets (future use)
#[allow(dead_code)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct FocusScopeId(pub u64);

/// Manages window and widget focus
pub struct FocusManager {
    /// Currently focused window
    focused_window: Option<WindowId>,
    /// Currently focused widget within the focused window
    focused_widget: Option<WidgetId>,
    /// Focus history for each window (for returning focus)
    focus_history: VecDeque<(WindowId, Option<WidgetId>)>,
    /// Maximum history entries
    max_history: usize,
}

impl FocusManager {
    /// Create a new focus manager
    pub fn new() -> Self {
        Self {
            focused_window: None,
            focused_widget: None,
            focus_history: VecDeque::new(),
            max_history: 32,
        }
    }
    
    /// Get the currently focused window
    pub fn focused_window(&self) -> Option<WindowId> {
        self.focused_window
    }
    
    /// Set the focused window
    pub fn set_focused_window(&mut self, id: WindowId) {
        // Save current focus to history
        if let Some(current) = self.focused_window {
            self.push_history(current, self.focused_widget);
        }
        
        self.focused_window = Some(id);
        self.focused_widget = None; // Widget focus handled separately
    }
    
    /// Get the currently focused widget
    pub fn focused_widget(&self) -> Option<WidgetId> {
        self.focused_widget
    }
    
    /// Set the focused widget within the current window
    pub fn set_focused_widget(&mut self, id: Option<WidgetId>) {
        self.focused_widget = id;
    }
    
    /// Focus a specific widget
    pub fn focus_widget(&mut self, widget: WidgetId) {
        self.focused_widget = Some(widget);
    }
    
    /// Clear widget focus (keep window focus)
    pub fn clear_widget_focus(&mut self) {
        self.focused_widget = None;
    }
    
    /// Clear all focus
    pub fn clear_focus(&mut self) {
        if let Some(window) = self.focused_window {
            self.push_history(window, self.focused_widget);
        }
        self.focused_window = None;
        self.focused_widget = None;
    }
    
    /// Check if a window is focused
    pub fn is_window_focused(&self, id: WindowId) -> bool {
        self.focused_window == Some(id)
    }
    
    /// Check if a widget is focused
    pub fn is_widget_focused(&self, id: WidgetId) -> bool {
        self.focused_widget == Some(id)
    }
    
    /// Push focus state to history
    fn push_history(&mut self, window: WindowId, widget: Option<WidgetId>) {
        self.focus_history.push_front((window, widget));
        while self.focus_history.len() > self.max_history {
            self.focus_history.pop_back();
        }
    }
    
    /// Restore focus from history
    pub fn restore_focus(&mut self) -> bool {
        if let Some((window, widget)) = self.focus_history.pop_front() {
            self.focused_window = Some(window);
            self.focused_widget = widget;
            true
        } else {
            false
        }
    }
    
    /// Get focus history for a window
    pub fn get_window_history(&self, window: WindowId) -> Option<WidgetId> {
        self.focus_history
            .iter()
            .find(|(w, _)| *w == window)
            .and_then(|(_, widget)| *widget)
    }
}

impl Default for FocusManager {
    fn default() -> Self {
        Self::new()
    }
}

/// Focus event for listeners
#[derive(Debug, Clone)]
pub enum FocusEvent {
    /// Window gained focus
    WindowFocused(WindowId),
    /// Window lost focus
    WindowBlurred(WindowId),
    /// Widget gained focus
    WidgetFocused(WidgetId),
    /// Widget lost focus
    WidgetBlurred(WidgetId),
    /// Focus moved in a direction
    FocusMoved(FocusDirection),
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_focus_manager() {
        let mut fm = FocusManager::new();
        
        let win1 = WindowId(1);
        let win2 = WindowId(2);
        
        fm.set_focused_window(win1);
        assert_eq!(fm.focused_window(), Some(win1));
        
        fm.set_focused_window(win2);
        assert_eq!(fm.focused_window(), Some(win2));
        
        // Previous window should be in history
        assert!(fm.restore_focus());
        assert_eq!(fm.focused_window(), Some(win1));
    }
    
    #[test]
    fn test_widget_focus() {
        let mut fm = FocusManager::new();
        
        let win = WindowId(1);
        let widget = WidgetId(1);
        
        fm.set_focused_window(win);
        fm.focus_widget(widget);
        
        assert!(fm.is_window_focused(win));
        assert!(fm.is_widget_focused(widget));
        
        fm.clear_widget_focus();
        assert!(!fm.is_widget_focused(widget));
        assert!(fm.is_window_focused(win));
    }
}
