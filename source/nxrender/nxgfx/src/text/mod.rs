//! Text Rendering Module
//!
//! Provides text rendering capabilities:
//! - Font loading and management
//! - Glyph rasterization
//! - Text layout with wrapping and alignment
//! - Text shaping (basic, HarfBuzz in future)

pub mod font;
pub mod rasterizer;
pub mod layout;
pub mod shaper;

pub use font::{Font, FontId, FontWeight, FontStyle, FontCache, FontMetrics, TextMetrics};
pub use rasterizer::{GlyphRasterizer, RasterizedGlyph, GlyphAtlas, GlyphUV};
pub use layout::{TextLayout, TextLayoutOptions, TextAlign, VerticalAlign, TextWrap, TextOverflow, LayoutGlyph, LayoutLine};
