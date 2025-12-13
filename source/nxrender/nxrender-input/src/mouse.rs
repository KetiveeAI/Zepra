//! Mouse Input Handling

use nxgfx::Point;
use crate::events::{Event, MouseButton, Modifiers};
use std::time::{Duration, Instant};

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum ClickType {
    Single,
    Double,
    Triple,
}

pub struct MouseState {
    position: Point,
    buttons: [bool; 5],
    last_click_time: Option<Instant>,
    last_click_pos: Point,
    click_count: u32,
    modifiers: Modifiers,
}

impl Default for MouseState {
    fn default() -> Self {
        Self::new()
    }
}

impl MouseState {
    pub fn new() -> Self {
        Self {
            position: Point::ZERO,
            buttons: [false; 5],
            last_click_time: None,
            last_click_pos: Point::ZERO,
            click_count: 0,
            modifiers: Modifiers::NONE,
        }
    }
    
    pub fn position(&self) -> Point { self.position }
    
    pub fn is_button_down(&self, button: MouseButton) -> bool {
        self.buttons[button as usize]
    }
    
    pub fn is_left_down(&self) -> bool { self.buttons[0] }
    pub fn is_right_down(&self) -> bool { self.buttons[1] }
    pub fn is_middle_down(&self) -> bool { self.buttons[2] }
    
    pub fn modifiers(&self) -> Modifiers { self.modifiers }
    
    pub fn process_event(&mut self, event: &Event) -> Option<ClickType> {
        match event {
            Event::MouseMove { pos, modifiers } => {
                self.position = *pos;
                self.modifiers = *modifiers;
                None
            }
            Event::MouseDown { pos, button, modifiers } => {
                self.position = *pos;
                self.buttons[*button as usize] = true;
                self.modifiers = *modifiers;
                None
            }
            Event::MouseUp { pos, button, modifiers } => {
                self.position = *pos;
                self.buttons[*button as usize] = false;
                self.modifiers = *modifiers;
                
                if *button == MouseButton::Left {
                    self.detect_click(*pos)
                } else {
                    None
                }
            }
            _ => None,
        }
    }
    
    fn detect_click(&mut self, pos: Point) -> Option<ClickType> {
        const DOUBLE_CLICK_TIME: Duration = Duration::from_millis(500);
        const DOUBLE_CLICK_DISTANCE: f32 = 5.0;
        
        let now = Instant::now();
        let distance = ((pos.x - self.last_click_pos.x).powi(2) + 
                       (pos.y - self.last_click_pos.y).powi(2)).sqrt();
        
        if let Some(last) = self.last_click_time {
            if now.duration_since(last) < DOUBLE_CLICK_TIME && distance < DOUBLE_CLICK_DISTANCE {
                self.click_count += 1;
            } else {
                self.click_count = 1;
            }
        } else {
            self.click_count = 1;
        }
        
        self.last_click_time = Some(now);
        self.last_click_pos = pos;
        
        match self.click_count {
            1 => Some(ClickType::Single),
            2 => Some(ClickType::Double),
            _ => Some(ClickType::Triple),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_mouse_state() {
        let mut state = MouseState::new();
        assert!(!state.is_left_down());
        
        state.process_event(&Event::MouseDown { 
            pos: Point::new(10.0, 10.0), 
            button: MouseButton::Left,
            modifiers: Modifiers::NONE,
        });
        
        assert!(state.is_left_down());
    }
}
