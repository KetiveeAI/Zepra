//! NXRender Widgets - UI Widget Library
//!
//! Provides a comprehensive set of UI widgets including:
//! - Controls: Button, TextField, Checkbox, Switch
//! - Display: Label
//! - Containers: (coming soon)
//! - Advanced: (coming soon)

pub mod base;
pub mod controls;
pub mod display;
pub mod containers;
pub mod advanced;

// Re-exports
pub use base::{Widget, WidgetId, EventResult, Constraints, WidgetState, Event, MouseButton, Key, Modifiers};

// Control widgets
pub use controls::{Button, ButtonStyle, TextField, TextFieldStyle, Checkbox, CheckboxStyle, Switch};

// Display widgets
pub use display::{Label, LabelStyle, TextAlign};
