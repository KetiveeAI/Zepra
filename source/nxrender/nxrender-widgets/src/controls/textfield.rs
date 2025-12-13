//! TextField Widget
//!
//! A text input widget with editing support.

use crate::base::{Widget, WidgetId, EventResult, Constraints, WidgetState, Event, Key, MouseButton};
use nxgfx::{Rect, Size, Point, Color};
use nxrender_core::Painter;

/// TextField style configuration
#[derive(Debug, Clone)]
pub struct TextFieldStyle {
    /// Background color
    pub background: Color,
    /// Border color when normal
    pub border_color: Color,
    /// Border color when focused
    pub border_color_focused: Color,
    /// Border width
    pub border_width: f32,
    /// Border radius
    pub border_radius: f32,
    /// Text color
    pub text_color: Color,
    /// Placeholder color
    pub placeholder_color: Color,
    /// Cursor color
    pub cursor_color: Color,
    /// Selection background
    pub selection_color: Color,
    /// Padding
    pub padding: f32,
    /// Font size
    pub font_size: f32,
}

impl Default for TextFieldStyle {
    fn default() -> Self {
        Self {
            background: Color::WHITE,
            border_color: Color::rgb(209, 213, 219),
            border_color_focused: Color::rgb(59, 130, 246),
            border_width: 1.0,
            border_radius: 6.0,
            text_color: Color::rgb(17, 24, 39),
            placeholder_color: Color::rgb(156, 163, 175),
            cursor_color: Color::rgb(59, 130, 246),
            selection_color: Color::rgba(59, 130, 246, 40),
            padding: 10.0,
            font_size: 14.0,
        }
    }
}

/// TextField widget
pub struct TextField {
    id: WidgetId,
    text: String,
    placeholder: String,
    bounds: Rect,
    state: WidgetState,
    style: TextFieldStyle,
    cursor_pos: usize,
    selection_start: Option<usize>,
    on_change: Option<Box<dyn FnMut(&str) + Send + Sync>>,
    on_submit: Option<Box<dyn FnMut(&str) + Send + Sync>>,
    /// Maximum character length (0 = unlimited)
    max_length: usize,
    /// Is password field
    is_password: bool,
}

impl TextField {
    /// Create a new text field
    pub fn new() -> Self {
        Self {
            id: WidgetId::new(),
            text: String::new(),
            placeholder: String::new(),
            bounds: Rect::ZERO,
            state: WidgetState::new(),
            style: TextFieldStyle::default(),
            cursor_pos: 0,
            selection_start: None,
            on_change: None,
            on_submit: None,
            max_length: 0,
            is_password: false,
        }
    }
    
    /// Set placeholder text
    pub fn placeholder(mut self, placeholder: impl Into<String>) -> Self {
        self.placeholder = placeholder.into();
        self
    }
    
    /// Set initial text
    pub fn text(mut self, text: impl Into<String>) -> Self {
        self.text = text.into();
        self.cursor_pos = self.text.len();
        self
    }
    
    /// Set the style
    pub fn style(mut self, style: TextFieldStyle) -> Self {
        self.style = style;
        self
    }
    
    /// Set change handler
    pub fn on_change<F: FnMut(&str) + Send + Sync + 'static>(mut self, handler: F) -> Self {
        self.on_change = Some(Box::new(handler));
        self
    }
    
    /// Set submit handler (Enter key)
    pub fn on_submit<F: FnMut(&str) + Send + Sync + 'static>(mut self, handler: F) -> Self {
        self.on_submit = Some(Box::new(handler));
        self
    }
    
    /// Set max length
    pub fn max_length(mut self, len: usize) -> Self {
        self.max_length = len;
        self
    }
    
    /// Set as password field
    pub fn password(mut self) -> Self {
        self.is_password = true;
        self
    }
    
    /// Get the current text
    pub fn get_text(&self) -> &str {
        &self.text
    }
    
    /// Set the text
    pub fn set_text(&mut self, text: impl Into<String>) {
        self.text = text.into();
        self.cursor_pos = self.text.len().min(self.cursor_pos);
    }
    
    /// Get display text (masked for password)
    fn display_text(&self) -> String {
        if self.is_password {
            "•".repeat(self.text.len())
        } else {
            self.text.clone()
        }
    }
    
    /// Insert character at cursor
    fn insert_char(&mut self, c: char) {
        if self.max_length > 0 && self.text.len() >= self.max_length {
            return;
        }
        self.text.insert(self.cursor_pos, c);
        self.cursor_pos += 1;
    }
    
    /// Delete character before cursor
    fn backspace(&mut self) {
        if self.cursor_pos > 0 {
            self.cursor_pos -= 1;
            self.text.remove(self.cursor_pos);
        }
    }
    
    /// Delete character at cursor
    fn delete(&mut self) {
        if self.cursor_pos < self.text.len() {
            self.text.remove(self.cursor_pos);
        }
    }
    
    /// Get current border color
    fn current_border_color(&self) -> Color {
        if self.state.focused {
            self.style.border_color_focused
        } else {
            self.style.border_color
        }
    }
}

impl Default for TextField {
    fn default() -> Self {
        Self::new()
    }
}

impl Widget for TextField {
    fn id(&self) -> WidgetId {
        self.id
    }
    
    fn type_name(&self) -> &'static str {
        "TextField"
    }
    
    fn bounds(&self) -> Rect {
        self.bounds
    }
    
    fn set_bounds(&mut self, bounds: Rect) {
        self.bounds = bounds;
    }
    
    fn measure(&self, constraints: Constraints) -> Size {
        let min_width = 100.0;
        let height = self.style.font_size + self.style.padding * 2.0;
        
        constraints.constrain(Size::new(min_width, height))
    }
    
    fn render(&self, painter: &mut Painter) {
        if !self.state.visible {
            return;
        }
        
        // Background
        painter.fill_rounded_rect(
            self.bounds,
            self.style.background,
            self.style.border_radius,
        );
        
        // Border
        painter.stroke_rounded_rect(
            self.bounds,
            self.current_border_color(),
            self.style.border_width,
            self.style.border_radius,
        );
        
        // Text or placeholder
        let text_x = self.bounds.x + self.style.padding;
        let text_y = self.bounds.y + (self.bounds.height - self.style.font_size) / 2.0;
        
        if self.text.is_empty() && !self.placeholder.is_empty() {
            painter.draw_text(
                &self.placeholder,
                Point::new(text_x, text_y),
                self.style.placeholder_color,
            );
        } else {
            painter.draw_text(
                &self.display_text(),
                Point::new(text_x, text_y),
                self.style.text_color,
            );
        }
        
        // Cursor (if focused)
        if self.state.focused {
            let cursor_x = text_x + (self.cursor_pos as f32 * self.style.font_size * 0.6);
            let cursor_y = self.bounds.y + self.style.padding / 2.0;
            let cursor_height = self.bounds.height - self.style.padding;
            
            painter.fill_rect(
                Rect::new(cursor_x, cursor_y, 2.0, cursor_height),
                self.style.cursor_color,
            );
        }
    }
    
    fn handle_event(&mut self, event: &Event) -> EventResult {
        if !self.state.enabled {
            return EventResult::Ignored;
        }
        
        match event {
            Event::MouseDown { button: MouseButton::Left, pos } => {
                if self.bounds.contains(*pos) {
                    // Calculate cursor position from click
                    let text_x = self.bounds.x + self.style.padding;
                    let click_offset = pos.x - text_x;
                    let char_width = self.style.font_size * 0.6;
                    let new_pos = (click_offset / char_width).round() as usize;
                    self.cursor_pos = new_pos.min(self.text.len());
                    
                    EventResult::NeedsRedraw
                } else {
                    EventResult::Ignored
                }
            }
            Event::FocusGained => {
                self.state.focused = true;
                EventResult::NeedsRedraw
            }
            Event::FocusLost => {
                self.state.focused = false;
                self.selection_start = None;
                EventResult::NeedsRedraw
            }
            Event::TextInput { text } if self.state.focused => {
                for c in text.chars() {
                    self.insert_char(c);
                }
                if let Some(handler) = &mut self.on_change {
                    handler(&self.text);
                }
                EventResult::NeedsRedraw
            }
            Event::KeyDown { key, .. } if self.state.focused => {
                match key {
                    Key::Backspace => {
                        self.backspace();
                        if let Some(handler) = &mut self.on_change {
                            handler(&self.text);
                        }
                        EventResult::NeedsRedraw
                    }
                    Key::Delete => {
                        self.delete();
                        if let Some(handler) = &mut self.on_change {
                            handler(&self.text);
                        }
                        EventResult::NeedsRedraw
                    }
                    Key::Left => {
                        if self.cursor_pos > 0 {
                            self.cursor_pos -= 1;
                        }
                        EventResult::NeedsRedraw
                    }
                    Key::Right => {
                        if self.cursor_pos < self.text.len() {
                            self.cursor_pos += 1;
                        }
                        EventResult::NeedsRedraw
                    }
                    Key::Home => {
                        self.cursor_pos = 0;
                        EventResult::NeedsRedraw
                    }
                    Key::End => {
                        self.cursor_pos = self.text.len();
                        EventResult::NeedsRedraw
                    }
                    Key::Enter => {
                        if let Some(handler) = &mut self.on_submit {
                            handler(&self.text);
                        }
                        EventResult::Handled
                    }
                    _ => EventResult::Ignored,
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
    fn test_textfield_creation() {
        let tf = TextField::new().placeholder("Enter text...");
        assert!(tf.get_text().is_empty());
    }
    
    #[test]
    fn test_textfield_insert() {
        let mut tf = TextField::new();
        tf.insert_char('H');
        tf.insert_char('i');
        assert_eq!(tf.get_text(), "Hi");
    }
    
    #[test]
    fn test_textfield_backspace() {
        let mut tf = TextField::new().text("Hello");
        tf.backspace();
        assert_eq!(tf.get_text(), "Hell");
    }
}
