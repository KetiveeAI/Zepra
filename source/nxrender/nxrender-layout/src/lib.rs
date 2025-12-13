//! NXRender Layout Engine
//!
//! Provides multiple layout algorithms:
//! - Flexbox: Row/column layouts with alignment and gaps
//! - Grid: Two-dimensional grid layouts
//! - Stack: Z-axis layering
//! - Absolute: Manual positioning

pub mod flexbox;
pub mod grid;
pub mod stack;
pub mod absolute;
pub mod constraints;

// Flexbox exports
pub use flexbox::{FlexLayout, FlexDirection, JustifyContent, AlignItems, AlignSelf, FlexWrap, FlexChild};

// Grid exports
pub use grid::{GridLayout, GridTrack, GridPlacement, GridBuilder};

// Stack exports
pub use stack::StackLayout;
