//! Button Widget
//!
//! A clickable button with customizable appearance.

use crate::base::{Widget, WidgetId, EventResult, Constraints, WidgetState, Event, MouseButton};
use nxgfx::{Rect, Size, Color};
use nxrender_core::Painter;

/// Button style configuration
#[derive(Debug, Clone)]
pub struct ButtonStyle {
    /// Background color when normal
    pub background: Color,
    /// Background when hovered
    pub background_hovered: Color,
    /// Background when pressed
    pub background_pressed: Color,
    /// Background when disabled
    pub background_disabled: Color,
    /// Text color
    pub text_color: Color,
    /// Text color when disabled
    pub text_color_disabled: Color,
    /// Border radius
    pub border_radius: f32,
    /// Border width
    pub border_width: f32,
    /// Border color
    pub border_color: Color,
    /// Padding
    pub padding: f32,
}

impl Default for ButtonStyle {
    fn default() -> Self {
        Self {
            background: Color::rgb(59, 130, 246),          // Blue
            background_hovered: Color::rgb(37, 99, 235),   // Darker blue
            background_pressed: Color::rgb(29, 78, 216),   // Even darker
            background_disabled: Color::rgb(156, 163, 175), // Gray
            text_color: Color::WHITE,
            text_color_disabled: Color::rgb(107, 114, 128),
            border_radius: 6.0,
            border_width: 0.0,
            border_color: Color::TRANSPARENT,
            padding: 12.0,
        }
    }
}

impl ButtonStyle {
    /// Primary button style
    pub fn primary() -> Self {
        Self::default()
    }
    
    /// Secondary button (outline)
    pub fn secondary() -> Self {
        Self {
            background: Color::TRANSPARENT,
            background_hovered: Color::rgba(59, 130, 246, 20),
            background_pressed: Color::rgba(59, 130, 246, 40),
            background_disabled: Color::TRANSPARENT,
            text_color: Color::rgb(59, 130, 246),
            text_color_disabled: Color::rgb(156, 163, 175),
            border_width: 1.0,
            border_color: Color::rgb(59, 130, 246),
            ..Default::default()
        }
    }
    
    /// Danger button (red)
    pub fn danger() -> Self {
        Self {
            background: Color::rgb(220, 38, 38),
            background_hovered: Color::rgb(185, 28, 28),
            background_pressed: Color::rgb(153, 27, 27),
            ..Default::default()
        }
    }
    
    /// Success button (green)
    pub fn success() -> Self {
        Self {
            background: Color::rgb(34, 197, 94),
            background_hovered: Color::rgb(22, 163, 74),
            background_pressed: Color::rgb(21, 128, 61),
            ..Default::default()
        }
    }
}

/// Button widget
pub struct Button {
    id: WidgetId,
    label: String,
    bounds: Rect,
    state: WidgetState,
    style: ButtonStyle,
    on_click: Option<Box<dyn FnMut() + Send + Sync>>,
}

impl Button {
    /// Create a new button
    pub fn new(label: impl Into<String>) -> Self {
        Self {
            id: WidgetId::new(),
            label: label.into(),
            bounds: Rect::ZERO,
            state: WidgetState::new(),
            style: ButtonStyle::default(),
            on_click: None,
        }
    }
    
    /// Set the button style
    pub fn style(mut self, style: ButtonStyle) -> Self {
        self.style = style;
        self
    }
    
    /// Set the click handler
    pub fn on_click<F: FnMut() + Send + Sync + 'static>(mut self, handler: F) -> Self {
        self.on_click = Some(Box::new(handler));
        self
    }
    
    /// Set enabled state
    pub fn enabled(mut self, enabled: bool) -> Self {
        self.state.enabled = enabled;
        self
    }
    
    /// Get the label
    pub fn label(&self) -> &str {
        &self.label
    }
    
    /// Set the label
    pub fn set_label(&mut self, label: impl Into<String>) {
        self.label = label.into();
    }
    
    /// Get current background color based on state
    fn current_background(&self) -> Color {
        if !self.state.enabled {
            self.style.background_disabled
        } else if self.state.pressed {
            self.style.background_pressed
        } else if self.state.hovered {
            self.style.background_hovered
        } else {
            self.style.background
        }
    }
    
    /// Get current text color based on state
    fn current_text_color(&self) -> Color {
        if self.state.enabled {
            self.style.text_color
        } else {
            self.style.text_color_disabled
        }
    }
}

impl Widget for Button {
    fn id(&self) -> WidgetId {
        self.id
    }
    
    fn type_name(&self) -> &'static str {
        "Button"
    }
    
    fn bounds(&self) -> Rect {
        self.bounds
    }
    
    fn set_bounds(&mut self, bounds: Rect) {
        self.bounds = bounds;
    }
    
    fn measure(&self, constraints: Constraints) -> Size {
        // Estimate text size (simplified - in real use would use font metrics)
        let text_width = self.label.len() as f32 * 8.0; // Approximate
        let text_height = 16.0;
        
        let width = text_width + self.style.padding * 2.0;
        let height = text_height + self.style.padding * 2.0;
        
        constraints.constrain(Size::new(width, height))
    }
    
    fn render(&self, painter: &mut Painter) {
        if !self.state.visible {
            return;
        }
        
        // Draw background
        if self.style.border_radius > 0.0 {
            painter.fill_rounded_rect(
                self.bounds,
                self.current_background(),
                self.style.border_radius,
            );
        } else {
            painter.fill_rect(self.bounds, self.current_background());
        }
        
        // Draw border if specified
        if self.style.border_width > 0.0 {
            painter.stroke_rect(
                self.bounds,
                self.style.border_color,
                self.style.border_width,
            );
        }
        
        // Draw text (centered)
        let text_color = self.current_text_color();
        painter.draw_text(&self.label, self.bounds.center(), text_color);
    }
    
    fn handle_event(&mut self, event: &Event) -> EventResult {
        if !self.state.enabled {
            return EventResult::Ignored;
        }
        
        match event {
            Event::MouseEnter => {
                self.state.hovered = true;
                EventResult::NeedsRedraw
            }
            Event::MouseLeave => {
                self.state.hovered = false;
                self.state.pressed = false;
                EventResult::NeedsRedraw
            }
            Event::MouseDown { button: MouseButton::Left, .. } => {
                self.state.pressed = true;
                EventResult::NeedsRedraw
            }
            Event::MouseUp { button: MouseButton::Left, pos } => {
                if self.state.pressed {
                    self.state.pressed = false;
                    
                    // Check if still over button
                    if self.bounds.contains(*pos) {
                        if let Some(handler) = &mut self.on_click {
                            handler();
                        }
                    }
                    
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
    fn test_button_creation() {
        let button = Button::new("Click Me");
        assert_eq!(button.label(), "Click Me");
        assert!(button.is_enabled());
    }
    
    #[test]
    fn test_button_style() {
        let button = Button::new("Delete")
            .style(ButtonStyle::danger());
        
        // Should have red background
        assert_eq!(button.style.background, Color::rgb(220, 38, 38));
    }
    
    #[test]
    fn test_button_measure() {
        let button = Button::new("Test");
        let size = button.measure(Constraints::unbounded());
        
        // Should have some reasonable size
        assert!(size.width > 0.0);
        assert!(size.height > 0.0);
    }
}
