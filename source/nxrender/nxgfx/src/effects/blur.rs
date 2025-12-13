//! Blur Effects
//!
//! Provides blur effects for UI elements like glassmorphism.

use crate::primitives::{Color, Rect};

/// Blur configuration
#[derive(Debug, Clone, Copy)]
pub struct Blur {
    /// Blur radius in pixels
    pub radius: f32,
    /// Blur quality (higher = better but slower)
    pub quality: BlurQuality,
}

/// Blur quality setting
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum BlurQuality {
    /// Fast box blur (single pass)
    Low,
    /// Standard Gaussian blur
    #[default]
    Medium,
    /// High quality multi-pass
    High,
}

impl Blur {
    /// Create a new blur effect
    pub fn new(radius: f32) -> Self {
        Self {
            radius,
            quality: BlurQuality::Medium,
        }
    }
    
    /// Set blur quality
    pub fn quality(mut self, quality: BlurQuality) -> Self {
        self.quality = quality;
        self
    }
    
    /// Get the number of blur passes for this quality
    pub fn passes(&self) -> u32 {
        match self.quality {
            BlurQuality::Low => 1,
            BlurQuality::Medium => 2,
            BlurQuality::High => 3,
        }
    }
    
    /// Calculate Gaussian kernel for given radius
    pub fn gaussian_kernel(&self, size: usize) -> Vec<f32> {
        let sigma = self.radius / 2.0;
        let mut kernel = Vec::with_capacity(size);
        let mut sum = 0.0f32;
        
        let half = (size / 2) as i32;
        
        for i in 0..size {
            let x = (i as i32 - half) as f32;
            let g = (-x * x / (2.0 * sigma * sigma)).exp();
            kernel.push(g);
            sum += g;
        }
        
        // Normalize
        for v in &mut kernel {
            *v /= sum;
        }
        
        kernel
    }
    
    /// Calculate the expanded bounds needed for blur
    pub fn expanded_bounds(&self, rect: Rect) -> Rect {
        let expand = self.radius.ceil();
        Rect::new(
            rect.x - expand,
            rect.y - expand,
            rect.width + expand * 2.0,
            rect.height + expand * 2.0,
        )
    }
}

impl Default for Blur {
    fn default() -> Self {
        Self::new(8.0)
    }
}

/// Apply a box blur to pixel data (CPU implementation)
pub fn box_blur_rgba(
    pixels: &mut [u8],
    width: u32,
    height: u32,
    radius: u32,
) {
    if radius == 0 || width == 0 || height == 0 {
        return;
    }
    
    let mut temp = vec![0u8; pixels.len()];
    
    // Horizontal pass
    for y in 0..height {
        for x in 0..width {
            let mut r = 0u32;
            let mut g = 0u32;
            let mut b = 0u32;
            let mut a = 0u32;
            let mut count = 0u32;
            
            let x_start = x.saturating_sub(radius);
            let x_end = (x + radius + 1).min(width);
            
            for sx in x_start..x_end {
                let idx = ((y * width + sx) * 4) as usize;
                r += pixels[idx] as u32;
                g += pixels[idx + 1] as u32;
                b += pixels[idx + 2] as u32;
                a += pixels[idx + 3] as u32;
                count += 1;
            }
            
            let idx = ((y * width + x) * 4) as usize;
            temp[idx] = (r / count) as u8;
            temp[idx + 1] = (g / count) as u8;
            temp[idx + 2] = (b / count) as u8;
            temp[idx + 3] = (a / count) as u8;
        }
    }
    
    // Vertical pass
    for y in 0..height {
        for x in 0..width {
            let mut r = 0u32;
            let mut g = 0u32;
            let mut b = 0u32;
            let mut a = 0u32;
            let mut count = 0u32;
            
            let y_start = y.saturating_sub(radius);
            let y_end = (y + radius + 1).min(height);
            
            for sy in y_start..y_end {
                let idx = ((sy * width + x) * 4) as usize;
                r += temp[idx] as u32;
                g += temp[idx + 1] as u32;
                b += temp[idx + 2] as u32;
                a += temp[idx + 3] as u32;
                count += 1;
            }
            
            let idx = ((y * width + x) * 4) as usize;
            pixels[idx] = (r / count) as u8;
            pixels[idx + 1] = (g / count) as u8;
            pixels[idx + 2] = (b / count) as u8;
            pixels[idx + 3] = (a / count) as u8;
        }
    }
}

/// Apply box blur multiple times for smoother result
pub fn box_blur_rgba_multi(
    pixels: &mut [u8],
    width: u32,
    height: u32,
    radius: u32,
    passes: u32,
) {
    for _ in 0..passes {
        box_blur_rgba(pixels, width, height, radius);
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_blur_creation() {
        let blur = Blur::new(10.0).quality(BlurQuality::High);
        assert_eq!(blur.radius, 10.0);
        assert_eq!(blur.quality, BlurQuality::High);
        assert_eq!(blur.passes(), 3);
    }
    
    #[test]
    fn test_gaussian_kernel() {
        let blur = Blur::new(4.0);
        let kernel = blur.gaussian_kernel(9);
        
        // Kernel should sum to approximately 1
        let sum: f32 = kernel.iter().sum();
        assert!((sum - 1.0).abs() < 0.01);
        
        // Center should be largest
        assert!(kernel[4] > kernel[0]);
        assert!(kernel[4] > kernel[8]);
    }
    
    #[test]
    fn test_box_blur() {
        // Create a 4x4 image with a white pixel in the center
        let mut pixels = vec![0u8; 4 * 4 * 4]; // 4x4 RGBA
        
        // Set center pixel to white
        let center_idx = (1 * 4 + 1) * 4;
        pixels[center_idx] = 255;
        pixels[center_idx + 1] = 255;
        pixels[center_idx + 2] = 255;
        pixels[center_idx + 3] = 255;
        
        box_blur_rgba(&mut pixels, 4, 4, 1);
        
        // After blur, the center should have spread to neighbors
        // so it should be less than 255
        assert!(pixels[center_idx] < 255);
    }
}
