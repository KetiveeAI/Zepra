//! Label Widget
//!
//! A text display widget with customizable appearance.

use crate::base::{Widget, WidgetId, EventResult, Constraints, WidgetState, Event};
use nxgfx::{Rect, Size, Point, Color};
use nxrender_core::Painter;

/// Text alignment
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum TextAlign {
    #[default]
    Left,
    Center,
    Right,
}

/// Label style configuration
#[derive(Debug, Clone)]
pub struct LabelStyle {
    /// Text color
    pub text_color: Color,
    /// Font size (points)
    pub font_size: f32,
    /// Text alignment
    pub align: TextAlign,
    /// Line height multiplier
    pub line_height: f32,
    /// Maximum lines (0 = unlimited)
    pub max_lines: usize,
    /// Whether to wrap text
    pub wrap: bool,
}

impl Default for LabelStyle {
    fn default() -> Self {
        Self {
            text_color: Color::rgb(31, 41, 55), // Dark gray
            font_size: 14.0,
            align: TextAlign::Left,
            line_height: 1.4,
            max_lines: 0,
            wrap: false,
        }
    }
}

impl LabelStyle {
    /// Title style (large, bold)
    pub fn title() -> Self {
        Self {
            font_size: 24.0,
            text_color: Color::rgb(17, 24, 39),
            ..Default::default()
        }
    }
    
    /// Heading style
    pub fn heading() -> Self {
        Self {
            font_size: 18.0,
            text_color: Color::rgb(31, 41, 55),
            ..Default::default()
        }
    }
    
    /// Body text style
    pub fn body() -> Self {
        Self::default()
    }
    
    /// Caption style (smaller, lighter)
    pub fn caption() -> Self {
        Self {
            font_size: 12.0,
            text_color: Color::rgb(107, 114, 128),
            ..Default::default()
        }
    }
    
    /// Muted/secondary text
    pub fn muted() -> Self {
        Self {
            text_color: Color::rgb(156, 163, 175),
            ..Default::default()
        }
    }
}

/// Label widget
pub struct Label {
    id: WidgetId,
    text: String,
    bounds: Rect,
    state: WidgetState,
    style: LabelStyle,
}

impl Label {
    /// Create a new label
    pub fn new(text: impl Into<String>) -> Self {
        Self {
            id: WidgetId::new(),
            text: text.into(),
            bounds: Rect::ZERO,
            state: WidgetState::new(),
            style: LabelStyle::default(),
        }
    }
    
    /// Set the label style
    pub fn style(mut self, style: LabelStyle) -> Self {
        self.style = style;
        self
    }
    
    /// Set text color
    pub fn color(mut self, color: Color) -> Self {
        self.style.text_color = color;
        self
    }
    
    /// Set font size
    pub fn font_size(mut self, size: f32) -> Self {
        self.style.font_size = size;
        self
    }
    
    /// Set text alignment
    pub fn align(mut self, align: TextAlign) -> Self {
        self.style.align = align;
        self
    }
    
    /// Enable text wrapping
    pub fn wrap(mut self) -> Self {
        self.style.wrap = true;
        self
    }
    
    /// Get the text
    pub fn text(&self) -> &str {
        &self.text
    }
    
    /// Set the text
    pub fn set_text(&mut self, text: impl Into<String>) {
        self.text = text.into();
    }
    
    /// Calculate text position based on alignment
    fn text_position(&self) -> Point {
        let text_width = self.text.len() as f32 * (self.style.font_size * 0.6);
        
        let x = match self.style.align {
            TextAlign::Left => self.bounds.x,
            TextAlign::Center => self.bounds.x + (self.bounds.width - text_width) / 2.0,
            TextAlign::Right => self.bounds.right() - text_width,
        };
        
        let y = self.bounds.y + (self.bounds.height - self.style.font_size) / 2.0;
        
        Point::new(x, y)
    }
}

impl Widget for Label {
    fn id(&self) -> WidgetId {
        self.id
    }
    
    fn type_name(&self) -> &'static str {
        "Label"
    }
    
    fn bounds(&self) -> Rect {
        self.bounds
    }
    
    fn set_bounds(&mut self, bounds: Rect) {
        self.bounds = bounds;
    }
    
    fn measure(&self, constraints: Constraints) -> Size {
        // Estimate text size
        let char_width = self.style.font_size * 0.6;
        let text_width = self.text.len() as f32 * char_width;
        let text_height = self.style.font_size * self.style.line_height;
        
        // TODO: Handle wrapping in measurement
        constraints.constrain(Size::new(text_width, text_height))
    }
    
    fn render(&self, painter: &mut Painter) {
        if !self.state.visible || self.text.is_empty() {
            return;
        }
        
        let pos = self.text_position();
        painter.draw_text(&self.text, pos, self.style.text_color);
    }
    
    fn handle_event(&mut self, _event: &Event) -> EventResult {
        // Labels don't handle events by default
        EventResult::Ignored
    }
    
    fn state(&self) -> WidgetState {
        self.state
    }
    
    fn can_focus(&self) -> bool {
        false // Labels are not focusable
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_label_creation() {
        let label = Label::new("Hello World");
        assert_eq!(label.text(), "Hello World");
    }
    
    #[test]
    fn test_label_style() {
        let label = Label::new("Title")
            .style(LabelStyle::title());
        
        assert_eq!(label.style.font_size, 24.0);
    }
    
    #[test]
    fn test_label_measure() {
        let label = Label::new("Test");
        let size = label.measure(Constraints::unbounded());
        
        assert!(size.width > 0.0);
        assert!(size.height > 0.0);
    }
}
