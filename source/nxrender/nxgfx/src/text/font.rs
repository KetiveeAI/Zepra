//! Font Loading and Management
//!
//! Provides font loading and glyph access using rusttype.

use rusttype::{Font as RtFont, Scale, point};
use std::collections::HashMap;
use std::path::Path;
use std::sync::Arc;

use crate::{GfxError, Result};

/// Font weight
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum FontWeight {
    Thin,
    Light,
    Regular,
    Medium,
    SemiBold,
    Bold,
    ExtraBold,
    Black,
}

impl Default for FontWeight {
    fn default() -> Self {
        Self::Regular
    }
}

/// Font style
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub enum FontStyle {
    #[default]
    Normal,
    Italic,
    Oblique,
}

/// Font identifier
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct FontId {
    pub family: String,
    pub weight: FontWeight,
    pub style: FontStyle,
}

impl FontId {
    pub fn new(family: impl Into<String>) -> Self {
        Self {
            family: family.into(),
            weight: FontWeight::Regular,
            style: FontStyle::Normal,
        }
    }
    
    pub fn weight(mut self, weight: FontWeight) -> Self {
        self.weight = weight;
        self
    }
    
    pub fn style(mut self, style: FontStyle) -> Self {
        self.style = style;
        self
    }
}

/// A loaded font
pub struct Font {
    inner: Arc<RtFont<'static>>,
    data: Arc<Vec<u8>>,
}

impl Font {
    /// Load a font from a file path
    pub fn from_file(path: impl AsRef<Path>) -> Result<Self> {
        let data = std::fs::read(path.as_ref())
            .map_err(|e| GfxError::FontError(format!("Failed to read font file: {}", e)))?;
        
        Self::from_bytes(data)
    }
    
    /// Load a font from bytes
    pub fn from_bytes(data: Vec<u8>) -> Result<Self> {
        let data = Arc::new(data);
        
        // We need to use unsafe here because rusttype requires 'static lifetime
        // but we manage the data lifetime ourselves with Arc
        let font_data: &'static [u8] = unsafe {
            std::slice::from_raw_parts(data.as_ptr(), data.len())
        };
        
        let inner = RtFont::try_from_bytes(font_data)
            .ok_or_else(|| GfxError::FontError("Failed to parse font data".into()))?;
        
        Ok(Self {
            inner: Arc::new(inner),
            data,
        })
    }
    
    /// Get the underlying rusttype font
    pub fn inner(&self) -> &RtFont<'static> {
        &self.inner
    }
    
    /// Get font metrics at a given size
    pub fn metrics(&self, size: f32) -> FontMetrics {
        let scale = Scale::uniform(size);
        let v_metrics = self.inner.v_metrics(scale);
        
        FontMetrics {
            ascent: v_metrics.ascent,
            descent: v_metrics.descent,
            line_gap: v_metrics.line_gap,
            line_height: v_metrics.ascent - v_metrics.descent + v_metrics.line_gap,
        }
    }
    
    /// Measure the width of a string
    pub fn measure_text(&self, text: &str, size: f32) -> TextMetrics {
        let scale = Scale::uniform(size);
        let v_metrics = self.inner.v_metrics(scale);
        
        let mut width = 0.0f32;
        let mut last_glyph_id = None;
        
        for c in text.chars() {
            let glyph = self.inner.glyph(c);
            let scaled = glyph.scaled(scale);
            
            // Add kerning
            if let Some(last_id) = last_glyph_id {
                width += self.inner.pair_kerning(scale, last_id, scaled.id());
            }
            
            width += scaled.h_metrics().advance_width;
            last_glyph_id = Some(scaled.id());
        }
        
        TextMetrics {
            width,
            height: v_metrics.ascent - v_metrics.descent,
            ascent: v_metrics.ascent,
            descent: v_metrics.descent,
        }
    }
}

impl Clone for Font {
    fn clone(&self) -> Self {
        Self {
            inner: Arc::clone(&self.inner),
            data: Arc::clone(&self.data),
        }
    }
}

/// Font metrics at a specific size
#[derive(Debug, Clone, Copy)]
pub struct FontMetrics {
    /// Distance from baseline to top
    pub ascent: f32,
    /// Distance from baseline to bottom (typically negative)
    pub descent: f32,
    /// Gap between lines
    pub line_gap: f32,
    /// Total line height
    pub line_height: f32,
}

/// Text measurement result
#[derive(Debug, Clone, Copy)]
pub struct TextMetrics {
    /// Total width of the text
    pub width: f32,
    /// Height of the text (ascent - descent)
    pub height: f32,
    /// Ascent value
    pub ascent: f32,
    /// Descent value
    pub descent: f32,
}

/// Font cache for managing loaded fonts
pub struct FontCache {
    fonts: HashMap<FontId, Font>,
    default_font: Option<Font>,
}

impl FontCache {
    /// Create a new font cache
    pub fn new() -> Self {
        Self {
            fonts: HashMap::new(),
            default_font: None,
        }
    }
    
    /// Load and cache a font
    pub fn load(&mut self, id: FontId, path: impl AsRef<Path>) -> Result<()> {
        let font = Font::from_file(path)?;
        self.fonts.insert(id, font);
        Ok(())
    }
    
    /// Load a font from bytes and cache it
    pub fn load_from_bytes(&mut self, id: FontId, data: Vec<u8>) -> Result<()> {
        let font = Font::from_bytes(data)?;
        self.fonts.insert(id, font);
        Ok(())
    }
    
    /// Set the default font
    pub fn set_default(&mut self, font: Font) {
        self.default_font = Some(font);
    }
    
    /// Get a font by ID
    pub fn get(&self, id: &FontId) -> Option<&Font> {
        self.fonts.get(id)
    }
    
    /// Get the default font
    pub fn default_font(&self) -> Option<&Font> {
        self.default_font.as_ref()
    }
    
    /// Load the built-in default font
    pub fn load_builtin_default(&mut self) {
        // Use a minimal built-in font if available
        // For now, this is a placeholder - in production you'd embed a font
        log::warn!("No built-in font available, text rendering may not work");
    }
}

impl Default for FontCache {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_font_id_builder() {
        let id = FontId::new("Arial")
            .weight(FontWeight::Bold)
            .style(FontStyle::Italic);
        
        assert_eq!(id.family, "Arial");
        assert_eq!(id.weight, FontWeight::Bold);
        assert_eq!(id.style, FontStyle::Italic);
    }
}
