//! Checkbox Widget
//!
//! A toggle checkbox with label.

use crate::base::{Widget, WidgetId, EventResult, Constraints, WidgetState, Event, MouseButton};
use nxgfx::{Rect, Size, Point, Color};
use nxrender_core::Painter;

/// Checkbox style configuration
#[derive(Debug, Clone)]
pub struct CheckboxStyle {
    /// Box size
    pub box_size: f32,
    /// Border color
    pub border_color: Color,
    /// Border color when checked
    pub border_color_checked: Color,
    /// Background when checked
    pub background_checked: Color,
    /// Checkmark color
    pub checkmark_color: Color,
    /// Border radius
    pub border_radius: f32,
    /// Border width
    pub border_width: f32,
    /// Text color
    pub text_color: Color,
    /// Gap between checkbox and label
    pub gap: f32,
}

impl Default for CheckboxStyle {
    fn default() -> Self {
        Self {
            box_size: 18.0,
            border_color: Color::rgb(156, 163, 175),
            border_color_checked: Color::rgb(59, 130, 246),
            background_checked: Color::rgb(59, 130, 246),
            checkmark_color: Color::WHITE,
            border_radius: 4.0,
            border_width: 2.0,
            text_color: Color::rgb(31, 41, 55),
            gap: 8.0,
        }
    }
}

/// Checkbox widget
pub struct Checkbox {
    id: WidgetId,
    label: String,
    checked: bool,
    bounds: Rect,
    state: WidgetState,
    style: CheckboxStyle,
    on_change: Option<Box<dyn FnMut(bool) + Send + Sync>>,
}

impl Checkbox {
    /// Create a new checkbox
    pub fn new(label: impl Into<String>) -> Self {
        Self {
            id: WidgetId::new(),
            label: label.into(),
            checked: false,
            bounds: Rect::ZERO,
            state: WidgetState::new(),
            style: CheckboxStyle::default(),
            on_change: None,
        }
    }
    
    /// Set initial checked state
    pub fn checked(mut self, checked: bool) -> Self {
        self.checked = checked;
        self
    }
    
    /// Set change handler
    pub fn on_change<F: FnMut(bool) + Send + Sync + 'static>(mut self, handler: F) -> Self {
        self.on_change = Some(Box::new(handler));
        self
    }
    
    /// Set style
    pub fn style(mut self, style: CheckboxStyle) -> Self {
        self.style = style;
        self
    }
    
    /// Get checked state
    pub fn is_checked(&self) -> bool {
        self.checked
    }
    
    /// Set checked state
    pub fn set_checked(&mut self, checked: bool) {
        self.checked = checked;
    }
    
    /// Toggle the checkbox
    pub fn toggle(&mut self) {
        self.checked = !self.checked;
        if let Some(handler) = &mut self.on_change {
            handler(self.checked);
        }
    }
    
    /// Get the checkbox box bounds
    fn box_bounds(&self) -> Rect {
        Rect::new(
            self.bounds.x,
            self.bounds.y + (self.bounds.height - self.style.box_size) / 2.0,
            self.style.box_size,
            self.style.box_size,
        )
    }
}

impl Widget for Checkbox {
    fn id(&self) -> WidgetId {
        self.id
    }
    
    fn type_name(&self) -> &'static str {
        "Checkbox"
    }
    
    fn bounds(&self) -> Rect {
        self.bounds
    }
    
    fn set_bounds(&mut self, bounds: Rect) {
        self.bounds = bounds;
    }
    
    fn measure(&self, constraints: Constraints) -> Size {
        let text_width = self.label.len() as f32 * 8.0;
        let width = self.style.box_size + self.style.gap + text_width;
        let height = self.style.box_size.max(20.0);
        
        constraints.constrain(Size::new(width, height))
    }
    
    fn render(&self, painter: &mut Painter) {
        if !self.state.visible {
            return;
        }
        
        let box_bounds = self.box_bounds();
        
        // Draw box background
        if self.checked {
            painter.fill_rounded_rect(
                box_bounds,
                self.style.background_checked,
                self.style.border_radius,
            );
        } else {
            painter.fill_rounded_rect(
                box_bounds,
                Color::WHITE,
                self.style.border_radius,
            );
            painter.stroke_rounded_rect(
                box_bounds,
                self.style.border_color,
                self.style.border_width,
                self.style.border_radius,
            );
        }
        
        // Draw checkmark if checked
        if self.checked {
            // Simple checkmark using lines would go here
            // For now, draw a simple indicator
            let inner = box_bounds.inset(4.0);
            painter.fill_rect(inner, self.style.checkmark_color);
        }
        
        // Draw label
        if !self.label.is_empty() {
            let text_x = box_bounds.right() + self.style.gap;
            let text_y = self.bounds.y + (self.bounds.height - 14.0) / 2.0;
            painter.draw_text(&self.label, Point::new(text_x, text_y), self.style.text_color);
        }
    }
    
    fn handle_event(&mut self, event: &Event) -> EventResult {
        if !self.state.enabled {
            return EventResult::Ignored;
        }
        
        match event {
            Event::MouseDown { button: MouseButton::Left, pos } => {
                if self.bounds.contains(*pos) {
                    self.toggle();
                    EventResult::NeedsRedraw
                } else {
                    EventResult::Ignored
                }
            }
            _ => EventResult::Ignored,
        }
    }
    
    fn state(&self) -> WidgetState {
        self.state
    }
    
    fn can_focus(&self) -> bool {
        self.state.enabled
    }
}

/// Switch widget (toggle switch)
pub struct Switch {
    id: WidgetId,
    label: String,
    on: bool,
    bounds: Rect,
    state: WidgetState,
    on_change: Option<Box<dyn FnMut(bool) + Send + Sync>>,
}

impl Switch {
    /// Create a new switch
    pub fn new(label: impl Into<String>) -> Self {
        Self {
            id: WidgetId::new(),
            label: label.into(),
            on: false,
            bounds: Rect::ZERO,
            state: WidgetState::new(),
            on_change: None,
        }
    }
    
    /// Set initial state
    pub fn on(mut self, on: bool) -> Self {
        self.on = on;
        self
    }
    
    /// Set change handler
    pub fn on_change<F: FnMut(bool) + Send + Sync + 'static>(mut self, handler: F) -> Self {
        self.on_change = Some(Box::new(handler));
        self
    }
    
    /// Get on state
    pub fn is_on(&self) -> bool {
        self.on
    }
    
    /// Toggle the switch
    pub fn toggle(&mut self) {
        self.on = !self.on;
        if let Some(handler) = &mut self.on_change {
            handler(self.on);
        }
    }
}

impl Widget for Switch {
    fn id(&self) -> WidgetId {
        self.id
    }
    
    fn type_name(&self) -> &'static str {
        "Switch"
    }
    
    fn bounds(&self) -> Rect {
        self.bounds
    }
    
    fn set_bounds(&mut self, bounds: Rect) {
        self.bounds = bounds;
    }
    
    fn measure(&self, constraints: Constraints) -> Size {
        let switch_width = 44.0;
        let switch_height = 24.0;
        let text_width = self.label.len() as f32 * 8.0;
        
        constraints.constrain(Size::new(switch_width + 8.0 + text_width, switch_height))
    }
    
    fn render(&self, painter: &mut Painter) {
        if !self.state.visible {
            return;
        }
        
        let track_width = 44.0;
        let track_height = 24.0;
        let thumb_size = 20.0;
        
        let track_bounds = Rect::new(
            self.bounds.x,
            self.bounds.y + (self.bounds.height - track_height) / 2.0,
            track_width,
            track_height,
        );
        
        // Track
        let track_color = if self.on {
            Color::rgb(59, 130, 246)
        } else {
            Color::rgb(209, 213, 219)
        };
        painter.fill_rounded_rect(track_bounds, track_color, track_height / 2.0);
        
        // Thumb
        let thumb_x = if self.on {
            track_bounds.right() - thumb_size - 2.0
        } else {
            track_bounds.x + 2.0
        };
        let thumb_bounds = Rect::new(
            thumb_x,
            track_bounds.y + 2.0,
            thumb_size,
            thumb_size,
        );
        painter.fill_rounded_rect(thumb_bounds, Color::WHITE, thumb_size / 2.0);
        
        // Label
        if !self.label.is_empty() {
            let text_x = track_bounds.right() + 8.0;
            let text_y = self.bounds.y + (self.bounds.height - 14.0) / 2.0;
            painter.draw_text(&self.label, Point::new(text_x, text_y), Color::rgb(31, 41, 55));
        }
    }
    
    fn handle_event(&mut self, event: &Event) -> EventResult {
        if !self.state.enabled {
            return EventResult::Ignored;
        }
        
        match event {
            Event::MouseDown { button: MouseButton::Left, pos } => {
                if self.bounds.contains(*pos) {
                    self.toggle();
                    EventResult::NeedsRedraw
                } else {
                    EventResult::Ignored
                }
            }
            _ => EventResult::Ignored,
        }
    }
    
    fn state(&self) -> WidgetState {
        self.state
    }
    
    fn can_focus(&self) -> bool {
        self.state.enabled
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_checkbox_creation() {
        let cb = Checkbox::new("Accept terms");
        assert!(!cb.is_checked());
    }
    
    #[test]
    fn test_checkbox_toggle() {
        let mut cb = Checkbox::new("Test");
        cb.toggle();
        assert!(cb.is_checked());
        cb.toggle();
        assert!(!cb.is_checked());
    }
    
    #[test]
    fn test_switch_creation() {
        let sw = Switch::new("Enable notifications");
        assert!(!sw.is_on());
    }
}
