//! Window Module
//!
//! Provides window abstraction and management for the UI toolkit.

mod window;
mod manager;
mod focus;

pub use window::{Window, WindowId, WindowConfig, WindowState, WindowLevel, WindowDecoration};
pub use manager::WindowManager;
pub use focus::{FocusManager, FocusDirection, FocusEvent, WidgetId};
