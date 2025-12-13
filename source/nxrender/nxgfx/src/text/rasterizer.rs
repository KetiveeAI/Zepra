//! Glyph Rasterization
//!
//! Converts font glyphs to bitmap images for GPU texture upload.

use rusttype::{Scale, point, PositionedGlyph};
use std::collections::HashMap;

use crate::primitives::{Color, Rect, Size};
use super::font::Font;

/// A rasterized glyph ready for rendering
#[derive(Clone)]
pub struct RasterizedGlyph {
    /// Pixel data (grayscale, one byte per pixel)
    pub pixels: Vec<u8>,
    /// Width in pixels
    pub width: u32,
    /// Height in pixels
    pub height: u32,
    /// Horizontal offset from origin
    pub bearing_x: f32,
    /// Vertical offset from baseline
    pub bearing_y: f32,
    /// Horizontal advance to next glyph
    pub advance: f32,
}

impl RasterizedGlyph {
    /// Create an empty glyph (for spaces etc.)
    pub fn empty(advance: f32) -> Self {
        Self {
            pixels: Vec::new(),
            width: 0,
            height: 0,
            bearing_x: 0.0,
            bearing_y: 0.0,
            advance,
        }
    }
    
    /// Check if glyph has actual pixel data
    pub fn has_pixels(&self) -> bool {
        !self.pixels.is_empty() && self.width > 0 && self.height > 0
    }
}

/// Cache key for rasterized glyphs
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
struct GlyphKey {
    character: char,
    size_tenths: u32, // Size in 1/10 pixels for better cache hits
}

/// Glyph rasterizer with caching
pub struct GlyphRasterizer {
    cache: HashMap<GlyphKey, RasterizedGlyph>,
    max_cache_size: usize,
}

impl GlyphRasterizer {
    /// Create a new glyph rasterizer
    pub fn new() -> Self {
        Self {
            cache: HashMap::new(),
            max_cache_size: 1024,
        }
    }
    
    /// Set the maximum cache size
    pub fn set_cache_size(&mut self, size: usize) {
        self.max_cache_size = size;
    }
    
    /// Clear the glyph cache
    pub fn clear_cache(&mut self) {
        self.cache.clear();
    }
    
    /// Rasterize a single character
    pub fn rasterize(&mut self, font: &Font, character: char, size: f32) -> RasterizedGlyph {
        let key = GlyphKey {
            character,
            size_tenths: (size * 10.0) as u32,
        };
        
        // Check cache
        if let Some(cached) = self.cache.get(&key) {
            return cached.clone();
        }
        
        // Rasterize the glyph
        let glyph = self.rasterize_uncached(font, character, size);
        
        // Cache if not too full
        if self.cache.len() < self.max_cache_size {
            self.cache.insert(key, glyph.clone());
        }
        
        glyph
    }
    
    /// Rasterize without caching
    fn rasterize_uncached(&self, font: &Font, character: char, size: f32) -> RasterizedGlyph {
        let scale = Scale::uniform(size);
        let glyph = font.inner().glyph(character).scaled(scale);
        let h_metrics = glyph.h_metrics();
        
        // Position at origin
        let positioned = glyph.positioned(point(0.0, 0.0));
        
        // Get bounding box
        let bb = match positioned.pixel_bounding_box() {
            Some(bb) => bb,
            None => {
                // Glyph has no pixels (space, etc.)
                return RasterizedGlyph::empty(h_metrics.advance_width);
            }
        };
        
        let width = (bb.max.x - bb.min.x) as u32;
        let height = (bb.max.y - bb.min.y) as u32;
        
        if width == 0 || height == 0 {
            return RasterizedGlyph::empty(h_metrics.advance_width);
        }
        
        // Create pixel buffer
        let mut pixels = vec![0u8; (width * height) as usize];
        
        // Rasterize
        positioned.draw(|x, y, v| {
            let x = x as u32;
            let y = y as u32;
            if x < width && y < height {
                let idx = (y * width + x) as usize;
                pixels[idx] = (v * 255.0) as u8;
            }
        });
        
        RasterizedGlyph {
            pixels,
            width,
            height,
            bearing_x: bb.min.x as f32,
            bearing_y: bb.min.y as f32,
            advance: h_metrics.advance_width,
        }
    }
    
    /// Rasterize a string of text
    pub fn rasterize_text(
        &mut self,
        font: &Font,
        text: &str,
        size: f32,
    ) -> Vec<(RasterizedGlyph, f32, f32)> {
        let scale = Scale::uniform(size);
        let v_metrics = font.inner().v_metrics(scale);
        
        let mut result = Vec::new();
        let mut x = 0.0f32;
        let mut last_glyph_id = None;
        
        for c in text.chars() {
            let glyph = font.inner().glyph(c).scaled(scale);
            
            // Add kerning
            if let Some(last_id) = last_glyph_id {
                x += font.inner().pair_kerning(scale, last_id, glyph.id());
            }
            
            let rasterized = self.rasterize(font, c, size);
            
            // Position: x + bearing_x, baseline + bearing_y
            let glyph_x = x + rasterized.bearing_x;
            let glyph_y = v_metrics.ascent + rasterized.bearing_y;
            
            result.push((rasterized.clone(), glyph_x, glyph_y));
            
            x += rasterized.advance;
            last_glyph_id = Some(glyph.id());
        }
        
        result
    }
}

impl Default for GlyphRasterizer {
    fn default() -> Self {
        Self::new()
    }
}

/// Generate a texture atlas from rasterized glyphs
pub struct GlyphAtlas {
    /// Combined pixel data (grayscale)
    pub pixels: Vec<u8>,
    /// Atlas width
    pub width: u32,
    /// Atlas height
    pub height: u32,
    /// UV coordinates for each character
    pub uvs: HashMap<char, GlyphUV>,
}

/// UV coordinates and metrics for a glyph in the atlas
#[derive(Debug, Clone, Copy)]
pub struct GlyphUV {
    /// Top-left U
    pub u0: f32,
    /// Top-left V
    pub v0: f32,
    /// Bottom-right U
    pub u1: f32,
    /// Bottom-right V
    pub v1: f32,
    /// Original glyph metrics
    pub bearing_x: f32,
    pub bearing_y: f32,
    pub advance: f32,
    pub width: u32,
    pub height: u32,
}

impl GlyphAtlas {
    /// Create an atlas from a set of characters
    pub fn create(
        rasterizer: &mut GlyphRasterizer,
        font: &Font,
        chars: &str,
        size: f32,
    ) -> Self {
        // Rasterize all characters
        let mut glyphs: Vec<(char, RasterizedGlyph)> = chars
            .chars()
            .map(|c| (c, rasterizer.rasterize(font, c, size)))
            .filter(|(_, g)| g.has_pixels())
            .collect();
        
        // Sort by height for better packing
        glyphs.sort_by(|a, b| b.1.height.cmp(&a.1.height));
        
        // Calculate atlas size (simple row-based packing)
        let padding = 2u32;
        let mut atlas_width = 512u32;
        let mut atlas_height = 512u32;
        
        // Pack glyphs
        let mut x = padding;
        let mut y = padding;
        let mut row_height = 0u32;
        let mut uvs = HashMap::new();
        
        for (c, glyph) in &glyphs {
            if x + glyph.width + padding > atlas_width {
                // Move to next row
                x = padding;
                y += row_height + padding;
                row_height = 0;
            }
            
            if y + glyph.height + padding > atlas_height {
                // Need larger atlas
                atlas_height *= 2;
            }
            
            // Record UV
            uvs.insert(*c, GlyphUV {
                u0: x as f32 / atlas_width as f32,
                v0: y as f32 / atlas_height as f32,
                u1: (x + glyph.width) as f32 / atlas_width as f32,
                v1: (y + glyph.height) as f32 / atlas_height as f32,
                bearing_x: glyph.bearing_x,
                bearing_y: glyph.bearing_y,
                advance: glyph.advance,
                width: glyph.width,
                height: glyph.height,
            });
            
            x += glyph.width + padding;
            row_height = row_height.max(glyph.height);
        }
        
        // Create pixel buffer
        let mut pixels = vec![0u8; (atlas_width * atlas_height) as usize];
        
        // Copy glyphs
        x = padding;
        y = padding;
        row_height = 0;
        
        for (c, glyph) in &glyphs {
            if x + glyph.width + padding > atlas_width {
                x = padding;
                y += row_height + padding;
                row_height = 0;
            }
            
            // Copy pixels
            for gy in 0..glyph.height {
                for gx in 0..glyph.width {
                    let src_idx = (gy * glyph.width + gx) as usize;
                    let dst_idx = ((y + gy) * atlas_width + (x + gx)) as usize;
                    if src_idx < glyph.pixels.len() && dst_idx < pixels.len() {
                        pixels[dst_idx] = glyph.pixels[src_idx];
                    }
                }
            }
            
            x += glyph.width + padding;
            row_height = row_height.max(glyph.height);
        }
        
        Self {
            pixels,
            width: atlas_width,
            height: atlas_height,
            uvs,
        }
    }
    
    /// Get UV for a character
    pub fn get_uv(&self, c: char) -> Option<&GlyphUV> {
        self.uvs.get(&c)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_rasterized_glyph_empty() {
        let glyph = RasterizedGlyph::empty(10.0);
        assert!(!glyph.has_pixels());
        assert_eq!(glyph.advance, 10.0);
    }
}
