//! Base Widget Module

mod widget;
mod event;

pub use widget::{Widget, WidgetId, EventResult, Constraints, WidgetState, Callback, ValueCallback};
pub use event::{Event, MouseButton, Key, Modifiers};
