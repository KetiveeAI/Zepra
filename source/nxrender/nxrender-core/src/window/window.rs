//! Window Abstraction
//!
//! Provides a platform-independent window abstraction for the compositor.

use nxgfx::{Size, Point, Rect, Color};
use crate::compositor::SurfaceId;
use std::sync::atomic::{AtomicU64, Ordering};

/// Unique window identifier
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct WindowId(pub u64);

impl WindowId {
    /// Generate a new unique window ID
    pub fn new() -> Self {
        static NEXT_ID: AtomicU64 = AtomicU64::new(1);
        Self(NEXT_ID.fetch_add(1, Ordering::SeqCst))
    }
}

impl Default for WindowId {
    fn default() -> Self {
        Self::new()
    }
}

/// Window state
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum WindowState {
    /// Normal window
    #[default]
    Normal,
    /// Minimized to taskbar/dock
    Minimized,
    /// Maximized to fill screen
    Maximized,
    /// Fullscreen mode
    Fullscreen,
}

/// Window type/level
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum WindowLevel {
    /// Below normal windows
    Background,
    /// Normal window level
    #[default]
    Normal,
    /// Floats above normal windows
    Floating,
    /// Modal dialog
    Modal,
    /// Tooltip/popup
    Popup,
    /// Always on top
    AlwaysOnTop,
}

/// Window decoration style
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum WindowDecoration {
    /// Full decorations (title bar, borders)
    #[default]
    Full,
    /// Minimal decorations (just title)
    Minimal,
    /// No decorations (borderless)
    None,
}

/// Window configuration
#[derive(Debug, Clone)]
pub struct WindowConfig {
    /// Window title
    pub title: String,
    /// Initial size
    pub size: Size,
    /// Initial position (None = system decides)
    pub position: Option<Point>,
    /// Minimum size
    pub min_size: Option<Size>,
    /// Maximum size
    pub max_size: Option<Size>,
    /// Whether window is resizable
    pub resizable: bool,
    /// Window decoration style
    pub decoration: WindowDecoration,
    /// Window level
    pub level: WindowLevel,
    /// Background color
    pub background_color: Color,
    /// Parent window (for dialogs)
    pub parent: Option<WindowId>,
}

impl Default for WindowConfig {
    fn default() -> Self {
        Self {
            title: String::from("Window"),
            size: Size::new(800.0, 600.0),
            position: None,
            min_size: Some(Size::new(100.0, 100.0)),
            max_size: None,
            resizable: true,
            decoration: WindowDecoration::Full,
            level: WindowLevel::Normal,
            background_color: Color::rgb(240, 240, 240),
            parent: None,
        }
    }
}

impl WindowConfig {
    /// Create with a title
    pub fn new(title: impl Into<String>) -> Self {
        Self {
            title: title.into(),
            ..Default::default()
        }
    }
    
    /// Set the size
    pub fn with_size(mut self, width: f32, height: f32) -> Self {
        self.size = Size::new(width, height);
        self
    }
    
    /// Set the position
    pub fn with_position(mut self, x: f32, y: f32) -> Self {
        self.position = Some(Point::new(x, y));
        self
    }
    
    /// Set as non-resizable
    pub fn fixed_size(mut self) -> Self {
        self.resizable = false;
        self
    }
    
    /// Set decoration style
    pub fn with_decoration(mut self, decoration: WindowDecoration) -> Self {
        self.decoration = decoration;
        self
    }
    
    /// Set window level
    pub fn with_level(mut self, level: WindowLevel) -> Self {
        self.level = level;
        self
    }
    
    /// Set parent window (for modal dialogs)
    pub fn with_parent(mut self, parent: WindowId) -> Self {
        self.parent = Some(parent);
        self.level = WindowLevel::Modal;
        self
    }
    
    /// Create as a dialog
    pub fn as_dialog(mut self, parent: WindowId) -> Self {
        self.parent = Some(parent);
        self.level = WindowLevel::Modal;
        self.resizable = false;
        self
    }
}

/// Top-level window
pub struct Window {
    id: WindowId,
    config: WindowConfig,
    state: WindowState,
    position: Point,
    size: Size,
    visible: bool,
    focused: bool,
    surface_id: Option<SurfaceId>,
    /// Saved bounds for restoring from maximized/minimized
    saved_bounds: Option<Rect>,
}

impl Window {
    /// Create a new window with default configuration
    pub fn new(title: impl Into<String>) -> Self {
        Self::with_config(WindowConfig::new(title))
    }
    
    /// Create a window with full configuration
    pub fn with_config(config: WindowConfig) -> Self {
        let position = config.position.unwrap_or(Point::new(100.0, 100.0));
        let size = config.size;
        
        Self {
            id: WindowId::new(),
            config,
            state: WindowState::Normal,
            position,
            size,
            visible: true,
            focused: false,
            surface_id: None,
            saved_bounds: None,
        }
    }
    
    /// Get the window ID
    pub fn id(&self) -> WindowId {
        self.id
    }
    
    /// Get the window title
    pub fn title(&self) -> &str {
        &self.config.title
    }
    
    /// Set the window title
    pub fn set_title(&mut self, title: impl Into<String>) {
        self.config.title = title.into();
    }
    
    /// Get the window position
    pub fn position(&self) -> Point {
        self.position
    }
    
    /// Set the window position
    pub fn set_position(&mut self, position: Point) {
        self.position = position;
    }
    
    /// Get the window size
    pub fn size(&self) -> Size {
        self.size
    }
    
    /// Set the window size
    pub fn set_size(&mut self, size: Size) {
        // Apply min/max constraints
        let mut new_size = size;
        
        if let Some(min) = self.config.min_size {
            new_size.width = new_size.width.max(min.width);
            new_size.height = new_size.height.max(min.height);
        }
        
        if let Some(max) = self.config.max_size {
            new_size.width = new_size.width.min(max.width);
            new_size.height = new_size.height.min(max.height);
        }
        
        self.size = new_size;
    }
    
    /// Get the window bounds
    pub fn bounds(&self) -> Rect {
        Rect::new(self.position.x, self.position.y, self.size.width, self.size.height)
    }
    
    /// Set the window bounds
    pub fn set_bounds(&mut self, bounds: Rect) {
        self.position = Point::new(bounds.x, bounds.y);
        self.set_size(Size::new(bounds.width, bounds.height));
    }
    
    /// Get the window state
    pub fn state(&self) -> WindowState {
        self.state
    }
    
    /// Set the window state
    pub fn set_state(&mut self, state: WindowState) {
        if self.state == state {
            return;
        }
        
        // Save bounds before maximizing/minimizing
        if state == WindowState::Maximized || state == WindowState::Fullscreen {
            if self.state == WindowState::Normal {
                self.saved_bounds = Some(self.bounds());
            }
        }
        
        self.state = state;
    }
    
    /// Maximize the window
    pub fn maximize(&mut self, screen_bounds: Rect) {
        if self.state == WindowState::Normal {
            self.saved_bounds = Some(self.bounds());
        }
        self.set_bounds(screen_bounds);
        self.state = WindowState::Maximized;
    }
    
    /// Restore from maximized/minimized
    pub fn restore(&mut self) {
        if let Some(bounds) = self.saved_bounds.take() {
            self.set_bounds(bounds);
        }
        self.state = WindowState::Normal;
    }
    
    /// Minimize the window
    pub fn minimize(&mut self) {
        if self.state == WindowState::Normal {
            self.saved_bounds = Some(self.bounds());
        }
        self.state = WindowState::Minimized;
    }
    
    /// Check if window is visible
    pub fn is_visible(&self) -> bool {
        self.visible && self.state != WindowState::Minimized
    }
    
    /// Set visibility
    pub fn set_visible(&mut self, visible: bool) {
        self.visible = visible;
    }
    
    /// Show the window
    pub fn show(&mut self) {
        self.visible = true;
        if self.state == WindowState::Minimized {
            self.restore();
        }
    }
    
    /// Hide the window
    pub fn hide(&mut self) {
        self.visible = false;
    }
    
    /// Check if window is focused
    pub fn is_focused(&self) -> bool {
        self.focused
    }
    
    /// Set focus state (called by WindowManager)
    pub(crate) fn set_focused(&mut self, focused: bool) {
        self.focused = focused;
    }
    
    /// Get the window level
    pub fn level(&self) -> WindowLevel {
        self.config.level
    }
    
    /// Get the parent window
    pub fn parent(&self) -> Option<WindowId> {
        self.config.parent
    }
    
    /// Check if window is resizable
    pub fn is_resizable(&self) -> bool {
        self.config.resizable && self.state == WindowState::Normal
    }
    
    /// Get the background color
    pub fn background_color(&self) -> Color {
        self.config.background_color
    }
    
    /// Set the background color
    pub fn set_background_color(&mut self, color: Color) {
        self.config.background_color = color;
    }
    
    /// Get the associated surface ID
    pub fn surface_id(&self) -> Option<SurfaceId> {
        self.surface_id
    }
    
    /// Set the associated surface ID
    pub fn set_surface_id(&mut self, id: SurfaceId) {
        self.surface_id = Some(id);
    }
    
    /// Check if a point is inside the window
    pub fn contains_point(&self, point: Point) -> bool {
        self.bounds().contains(point)
    }
    
    /// Get the decoration type
    pub fn decoration(&self) -> WindowDecoration {
        self.config.decoration
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_window_creation() {
        let window = Window::new("Test Window");
        assert_eq!(window.title(), "Test Window");
        assert!(window.is_visible());
    }
    
    #[test]
    fn test_window_config() {
        let config = WindowConfig::new("Dialog")
            .with_size(400.0, 300.0)
            .with_position(100.0, 100.0)
            .fixed_size();
        
        let window = Window::with_config(config);
        
        assert_eq!(window.size().width, 400.0);
        assert_eq!(window.position().x, 100.0);
        assert!(!window.is_resizable());
    }
    
    #[test]
    fn test_window_maximize_restore() {
        let mut window = Window::new("Test");
        let original_bounds = window.bounds();
        
        window.maximize(Rect::new(0.0, 0.0, 1920.0, 1080.0));
        assert_eq!(window.state(), WindowState::Maximized);
        assert_eq!(window.size().width, 1920.0);
        
        window.restore();
        assert_eq!(window.state(), WindowState::Normal);
        assert_eq!(window.bounds(), original_bounds);
    }
}
