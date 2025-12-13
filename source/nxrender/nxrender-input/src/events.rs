//! Input Events

use nxgfx::Point;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum MouseButton {
    Left,
    Right,
    Middle,
    Back,
    Forward,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct Modifiers {
    pub shift: bool,
    pub ctrl: bool,
    pub alt: bool,
    pub meta: bool,
}

impl Modifiers {
    pub const NONE: Self = Self { shift: false, ctrl: false, alt: false, meta: false };
    
    pub fn with_shift(mut self) -> Self { self.shift = true; self }
    pub fn with_ctrl(mut self) -> Self { self.ctrl = true; self }
    pub fn with_alt(mut self) -> Self { self.alt = true; self }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum KeyCode {
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Escape, Tab, Backspace, Enter, Space,
    Insert, Delete, Home, End, PageUp, PageDown,
    Left, Right, Up, Down,
    Unknown(u32),
}

#[derive(Debug, Clone)]
pub enum Event {
    MouseMove { pos: Point, modifiers: Modifiers },
    MouseDown { pos: Point, button: MouseButton, modifiers: Modifiers },
    MouseUp { pos: Point, button: MouseButton, modifiers: Modifiers },
    MouseEnter { pos: Point },
    MouseLeave,
    Scroll { pos: Point, delta_x: f32, delta_y: f32, modifiers: Modifiers },
    
    KeyDown { key: KeyCode, modifiers: Modifiers },
    KeyUp { key: KeyCode, modifiers: Modifiers },
    TextInput { text: String },
    
    TouchStart { id: u64, pos: Point },
    TouchMove { id: u64, pos: Point },
    TouchEnd { id: u64, pos: Point },
    TouchCancel { id: u64 },
    
    FocusGained,
    FocusLost,
}

impl Event {
    pub fn position(&self) -> Option<Point> {
        match self {
            Event::MouseMove { pos, .. } |
            Event::MouseDown { pos, .. } |
            Event::MouseUp { pos, .. } |
            Event::MouseEnter { pos } |
            Event::Scroll { pos, .. } |
            Event::TouchStart { pos, .. } |
            Event::TouchMove { pos, .. } |
            Event::TouchEnd { pos, .. } => Some(*pos),
            _ => None,
        }
    }
    
    pub fn modifiers(&self) -> Modifiers {
        match self {
            Event::MouseMove { modifiers, .. } |
            Event::MouseDown { modifiers, .. } |
            Event::MouseUp { modifiers, .. } |
            Event::Scroll { modifiers, .. } |
            Event::KeyDown { modifiers, .. } |
            Event::KeyUp { modifiers, .. } => *modifiers,
            _ => Modifiers::NONE,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_event_position() {
        let event = Event::MouseMove { pos: Point::new(10.0, 20.0), modifiers: Modifiers::NONE };
        assert_eq!(event.position(), Some(Point::new(10.0, 20.0)));
    }
    
    #[test]
    fn test_modifiers() {
        let m = Modifiers::NONE.with_ctrl().with_shift();
        assert!(m.ctrl && m.shift && !m.alt);
    }
}
