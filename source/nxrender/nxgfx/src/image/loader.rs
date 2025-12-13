//! Image Loading
//!
//! Provides image loading for PNG, JPEG, WebP, and other formats
//! using the `image` crate.

use image::{DynamicImage, GenericImageView, ImageError};
use std::path::Path;

use crate::{GfxError, Result};
use crate::primitives::Size;

/// Loaded image data
pub struct ImageData {
    /// RGBA pixel data
    pub pixels: Vec<u8>,
    /// Image width
    pub width: u32,
    /// Image height
    pub height: u32,
    /// Bytes per pixel (always 4 for RGBA)
    pub channels: u8,
}

impl ImageData {
    /// Load an image from a file
    pub fn from_file(path: impl AsRef<Path>) -> Result<Self> {
        let img = image::open(path.as_ref())
            .map_err(|e| GfxError::ImageError(format!("Failed to load image: {}", e)))?;
        
        Ok(Self::from_dynamic_image(img))
    }
    
    /// Load an image from bytes
    pub fn from_bytes(data: &[u8]) -> Result<Self> {
        let img = image::load_from_memory(data)
            .map_err(|e| GfxError::ImageError(format!("Failed to decode image: {}", e)))?;
        
        Ok(Self::from_dynamic_image(img))
    }
    
    /// Load an image from bytes with format hint
    pub fn from_bytes_with_format(data: &[u8], format: ImageFormat) -> Result<Self> {
        let img_format = match format {
            ImageFormat::Png => image::ImageFormat::Png,
            ImageFormat::Jpeg => image::ImageFormat::Jpeg,
            ImageFormat::WebP => image::ImageFormat::WebP,
            ImageFormat::Gif => image::ImageFormat::Gif,
            ImageFormat::Bmp => image::ImageFormat::Bmp,
            ImageFormat::Unknown => {
                return Self::from_bytes(data);
            }
        };
        
        let img = image::load_from_memory_with_format(data, img_format)
            .map_err(|e| GfxError::ImageError(format!("Failed to decode image: {}", e)))?;
        
        Ok(Self::from_dynamic_image(img))
    }
    
    fn from_dynamic_image(img: DynamicImage) -> Self {
        let (width, height) = img.dimensions();
        let rgba = img.into_rgba8();
        let pixels = rgba.into_raw();
        
        Self {
            pixels,
            width,
            height,
            channels: 4,
        }
    }
    
    /// Create a solid color image
    pub fn solid_color(width: u32, height: u32, r: u8, g: u8, b: u8, a: u8) -> Self {
        let pixels: Vec<u8> = (0..width * height)
            .flat_map(|_| [r, g, b, a])
            .collect();
        
        Self {
            pixels,
            width,
            height,
            channels: 4,
        }
    }
    
    /// Create a checkerboard pattern (useful for missing textures)
    pub fn checkerboard(width: u32, height: u32, size: u32) -> Self {
        let mut pixels = Vec::with_capacity((width * height * 4) as usize);
        
        for y in 0..height {
            for x in 0..width {
                let is_white = ((x / size) + (y / size)) % 2 == 0;
                let color = if is_white { 255u8 } else { 192u8 };
                pixels.extend_from_slice(&[color, color, color, 255]);
            }
        }
        
        Self {
            pixels,
            width,
            height,
            channels: 4,
        }
    }
    
    /// Get the size of the image
    pub fn size(&self) -> Size {
        Size::new(self.width as f32, self.height as f32)
    }
    
    /// Get a pixel at (x, y)
    pub fn get_pixel(&self, x: u32, y: u32) -> Option<[u8; 4]> {
        if x >= self.width || y >= self.height {
            return None;
        }
        
        let idx = ((y * self.width + x) * 4) as usize;
        if idx + 3 < self.pixels.len() {
            Some([
                self.pixels[idx],
                self.pixels[idx + 1],
                self.pixels[idx + 2],
                self.pixels[idx + 3],
            ])
        } else {
            None
        }
    }
    
    /// Resize the image
    pub fn resize(&self, new_width: u32, new_height: u32) -> Self {
        // Simple nearest-neighbor resize
        let mut pixels = Vec::with_capacity((new_width * new_height * 4) as usize);
        
        for y in 0..new_height {
            for x in 0..new_width {
                let src_x = (x * self.width) / new_width;
                let src_y = (y * self.height) / new_height;
                
                if let Some(pixel) = self.get_pixel(src_x, src_y) {
                    pixels.extend_from_slice(&pixel);
                } else {
                    pixels.extend_from_slice(&[0, 0, 0, 0]);
                }
            }
        }
        
        Self {
            pixels,
            width: new_width,
            height: new_height,
            channels: 4,
        }
    }
    
    /// Flip the image vertically (useful for OpenGL which has Y-up)
    pub fn flip_vertical(&mut self) {
        let row_size = (self.width * 4) as usize;
        let mut temp_row = vec![0u8; row_size];
        
        for y in 0..self.height / 2 {
            let top_start = (y * self.width * 4) as usize;
            let bottom_y = self.height - 1 - y;
            let bottom_start = (bottom_y * self.width * 4) as usize;
            
            // Swap rows
            temp_row.copy_from_slice(&self.pixels[top_start..top_start + row_size]);
            self.pixels.copy_within(bottom_start..bottom_start + row_size, top_start);
            self.pixels[bottom_start..bottom_start + row_size].copy_from_slice(&temp_row);
        }
    }
}

/// Image format
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ImageFormat {
    Png,
    Jpeg,
    WebP,
    Gif,
    Bmp,
    Unknown,
}

impl ImageFormat {
    /// Detect format from file extension
    pub fn from_extension(ext: &str) -> Self {
        match ext.to_lowercase().as_str() {
            "png" => Self::Png,
            "jpg" | "jpeg" => Self::Jpeg,
            "webp" => Self::WebP,
            "gif" => Self::Gif,
            "bmp" => Self::Bmp,
            _ => Self::Unknown,
        }
    }
    
    /// Detect format from magic bytes
    pub fn from_magic(data: &[u8]) -> Self {
        if data.len() < 4 {
            return Self::Unknown;
        }
        
        if data.starts_with(b"\x89PNG") {
            Self::Png
        } else if data.starts_with(b"\xFF\xD8\xFF") {
            Self::Jpeg
        } else if data.starts_with(b"RIFF") && data.len() >= 12 && &data[8..12] == b"WEBP" {
            Self::WebP
        } else if data.starts_with(b"GIF87a") || data.starts_with(b"GIF89a") {
            Self::Gif
        } else if data.starts_with(b"BM") {
            Self::Bmp
        } else {
            Self::Unknown
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_solid_color() {
        let img = ImageData::solid_color(2, 2, 255, 0, 0, 255);
        assert_eq!(img.width, 2);
        assert_eq!(img.height, 2);
        assert_eq!(img.pixels.len(), 16); // 2x2x4
        assert_eq!(img.get_pixel(0, 0), Some([255, 0, 0, 255]));
    }
    
    #[test]
    fn test_checkerboard() {
        let img = ImageData::checkerboard(4, 4, 2);
        assert_eq!(img.width, 4);
        assert_eq!(img.height, 4);
    }
    
    #[test]
    fn test_format_detection() {
        assert_eq!(ImageFormat::from_extension("png"), ImageFormat::Png);
        assert_eq!(ImageFormat::from_extension("JPG"), ImageFormat::Jpeg);
        
        assert_eq!(ImageFormat::from_magic(b"\x89PNG\r\n\x1a\n"), ImageFormat::Png);
        assert_eq!(ImageFormat::from_magic(b"\xFF\xD8\xFF\xE0"), ImageFormat::Jpeg);
    }
}
