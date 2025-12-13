//! NXRender Input Handling
//!
//! Mouse, keyboard, touch, gesture, and translation.

pub mod events;
pub mod mouse;
pub mod keyboard;
pub mod touch;
pub mod gestures;
pub mod translation;

pub use events::{Event, MouseButton, KeyCode, Modifiers};
pub use mouse::{MouseState, ClickType};
pub use keyboard::{KeyboardState, KeyboardShortcut};
pub use touch::{TouchState, TouchPoint};
pub use gestures::{GestureRecognizer, Gesture, GestureType, SwipeDirection};
pub use translation::{GestureTranslator, GestureAction, GesturePattern, GestureBinding};
