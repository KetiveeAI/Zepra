//! Widget Trait and Core Types

use nxgfx::{Rect, Size, Point};
use nxrender_core::Painter;
use std::sync::atomic::{AtomicU64, Ordering};

use super::event::Event;

/// Unique widget identifier
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct WidgetId(pub u64);

impl WidgetId {
    /// Create a new unique widget ID
    pub fn new() -> Self {
        static NEXT_ID: AtomicU64 = AtomicU64::new(1);
        Self(NEXT_ID.fetch_add(1, Ordering::SeqCst))
    }
}

impl Default for WidgetId {
    fn default() -> Self {
        Self::new()
    }
}

/// Result of handling an event
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum EventResult {
    /// Event was handled and consumed
    Handled,
    /// Event was ignored, bubble to parent
    Ignored,
    /// Event was handled and widget needs redraw
    NeedsRedraw,
}

/// Layout constraints for measuring widgets
#[derive(Debug, Clone, Copy)]
pub struct Constraints {
    pub min_width: f32,
    pub max_width: f32,
    pub min_height: f32,
    pub max_height: f32,
}

impl Constraints {
    /// Unbounded constraints
    pub fn unbounded() -> Self {
        Self {
            min_width: 0.0,
            max_width: f32::INFINITY,
            min_height: 0.0,
            max_height: f32::INFINITY,
        }
    }
    
    /// Loose constraints that allow any size up to max
    pub fn loose(max: Size) -> Self {
        Self {
            min_width: 0.0,
            max_width: max.width,
            min_height: 0.0,
            max_height: max.height,
        }
    }
    
    /// Tight constraints that require exact size
    pub fn tight(size: Size) -> Self {
        Self {
            min_width: size.width,
            max_width: size.width,
            min_height: size.height,
            max_height: size.height,
        }
    }
    
    /// Constrain a size to fit within these constraints
    pub fn constrain(&self, size: Size) -> Size {
        Size::new(
            size.width.max(self.min_width).min(self.max_width),
            size.height.max(self.min_height).min(self.max_height),
        )
    }
}

impl Default for Constraints {
    fn default() -> Self {
        Self::unbounded()
    }
}

/// Widget state flags
#[derive(Debug, Clone, Copy, Default)]
pub struct WidgetState {
    pub visible: bool,
    pub enabled: bool,
    pub hovered: bool,
    pub pressed: bool,
    pub focused: bool,
}

impl WidgetState {
    pub fn new() -> Self {
        Self {
            visible: true,
            enabled: true,
            hovered: false,
            pressed: false,
            focused: false,
        }
    }
}

/// Base trait for all widgets
pub trait Widget {
    /// Get the widget's unique ID
    fn id(&self) -> WidgetId;
    
    /// Get the widget type name (for debugging)
    fn type_name(&self) -> &'static str;
    
    /// Get the current bounds
    fn bounds(&self) -> Rect;
    
    /// Set the bounds
    fn set_bounds(&mut self, bounds: Rect);
    
    /// Measure the desired size given constraints
    fn measure(&self, constraints: Constraints) -> Size;
    
    /// Render the widget
    fn render(&self, painter: &mut Painter);
    
    /// Handle an input event
    fn handle_event(&mut self, event: &Event) -> EventResult {
        let _ = event;
        EventResult::Ignored
    }
    
    /// Update animation state
    fn update(&mut self, dt: std::time::Duration) {
        let _ = dt;
    }
    
    /// Get the widget state
    fn state(&self) -> WidgetState {
        WidgetState::new()
    }
    
    /// Check if widget is visible
    fn is_visible(&self) -> bool {
        self.state().visible
    }
    
    /// Check if widget is enabled
    fn is_enabled(&self) -> bool {
        self.state().enabled
    }
    
    /// Check if widget can receive focus
    fn can_focus(&self) -> bool {
        false
    }
    
    /// Check if point is within widget bounds
    fn hit_test(&self, point: Point) -> bool {
        self.bounds().contains(point)
    }
}

/// A callback function type
pub type Callback = Box<dyn FnMut() + Send + Sync>;

/// A callback that receives a value
pub type ValueCallback<T> = Box<dyn FnMut(T) + Send + Sync>;
