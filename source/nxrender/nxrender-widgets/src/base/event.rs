//! Input Event Types
//!
//! Defines all input events that widgets can handle.

use nxgfx::Point;

/// Mouse button
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MouseButton {
    Left,
    Right,
    Middle,
}

/// Keyboard modifier keys
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct Modifiers {
    pub shift: bool,
    pub ctrl: bool,
    pub alt: bool,
    pub meta: bool, // Command on Mac, Windows key on Windows
}

impl Modifiers {
    pub fn none() -> Self {
        Self::default()
    }
    
    pub fn shift() -> Self {
        Self { shift: true, ..Default::default() }
    }
    
    pub fn ctrl() -> Self {
        Self { ctrl: true, ..Default::default() }
    }
}

/// Key code for keyboard events
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Key {
    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    
    // Numbers
    Num0, Num1, Num2, Num3, Num4,
    Num5, Num6, Num7, Num8, Num9,
    
    // Function keys
    F1, F2, F3, F4, F5, F6,
    F7, F8, F9, F10, F11, F12,
    
    // Control keys
    Escape, Tab, Backspace, Enter, Space,
    Insert, Delete, Home, End, PageUp, PageDown,
    
    // Arrow keys
    Left, Right, Up, Down,
    
    // Other
    Unknown,
}

/// Input event
#[derive(Debug, Clone)]
pub enum Event {
    // Mouse events
    MouseMove { pos: Point },
    MouseDown { pos: Point, button: MouseButton },
    MouseUp { pos: Point, button: MouseButton },
    MouseEnter,
    MouseLeave,
    Scroll { delta_x: f32, delta_y: f32 },
    
    // Keyboard events
    KeyDown { key: Key, modifiers: Modifiers },
    KeyUp { key: Key, modifiers: Modifiers },
    TextInput { text: String },
    
    // Focus events
    FocusGained,
    FocusLost,
    
    // Touch events
    TouchStart { id: u64, pos: Point },
    TouchMove { id: u64, pos: Point },
    TouchEnd { id: u64, pos: Point },
}

impl Event {
    /// Get the position for mouse/touch events
    pub fn position(&self) -> Option<Point> {
        match self {
            Event::MouseMove { pos } => Some(*pos),
            Event::MouseDown { pos, .. } => Some(*pos),
            Event::MouseUp { pos, .. } => Some(*pos),
            Event::TouchStart { pos, .. } => Some(*pos),
            Event::TouchMove { pos, .. } => Some(*pos),
            Event::TouchEnd { pos, .. } => Some(*pos),
            _ => None,
        }
    }
}
