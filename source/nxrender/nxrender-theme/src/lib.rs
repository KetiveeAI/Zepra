//! NXRender Theme System
//!
//! Provides theming with colors, typography, spacing, and presets.

pub mod theme;
pub mod colors;
pub mod fonts;
pub mod icons;
pub mod presets;

// Main exports
pub use theme::{Theme, ThemeMode, Spacing, BorderRadius, Shadow, Shadows};
pub use colors::{ColorPalette, ColorScale, SemanticColors, SurfaceColors, TextColors, BorderColors};
pub use fonts::{Typography, TextStyle, FontWeight, FontStyle};
