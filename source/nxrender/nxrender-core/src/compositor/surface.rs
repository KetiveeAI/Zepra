//! Surface Management
//!
//! Surfaces are drawable buffers that hold rendered content.
//! They support double-buffering for tear-free updates.

use super::SurfaceId;
use nxgfx::{Size, Rect, Color};

/// Buffer format for surface pixels
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum PixelFormat {
    /// 8-bit RGBA (32 bits per pixel)
    #[default]
    Rgba8,
    /// 8-bit BGRA (32 bits per pixel, for some backends)
    Bgra8,
    /// 8-bit RGB (24 bits per pixel)
    Rgb8,
    /// 8-bit alpha only
    Alpha8,
}

impl PixelFormat {
    /// Bytes per pixel for this format
    pub fn bytes_per_pixel(&self) -> usize {
        match self {
            Self::Rgba8 | Self::Bgra8 => 4,
            Self::Rgb8 => 3,
            Self::Alpha8 => 1,
        }
    }
}

/// A pixel buffer holding raw image data
#[derive(Clone)]
pub struct PixelBuffer {
    /// Raw pixel data
    pub pixels: Vec<u8>,
    /// Width in pixels
    pub width: u32,
    /// Height in pixels
    pub height: u32,
    /// Pixel format
    pub format: PixelFormat,
}

impl PixelBuffer {
    /// Create a new pixel buffer
    pub fn new(width: u32, height: u32, format: PixelFormat) -> Self {
        let size = (width * height) as usize * format.bytes_per_pixel();
        Self {
            pixels: vec![0u8; size],
            width,
            height,
            format,
        }
    }
    
    /// Create a buffer filled with a color
    pub fn filled(width: u32, height: u32, color: Color) -> Self {
        let mut buffer = Self::new(width, height, PixelFormat::Rgba8);
        buffer.clear(color);
        buffer
    }
    
    /// Clear the buffer with a color
    pub fn clear(&mut self, color: Color) {
        match self.format {
            PixelFormat::Rgba8 => {
                for chunk in self.pixels.chunks_exact_mut(4) {
                    chunk[0] = color.r;
                    chunk[1] = color.g;
                    chunk[2] = color.b;
                    chunk[3] = color.a;
                }
            }
            PixelFormat::Bgra8 => {
                for chunk in self.pixels.chunks_exact_mut(4) {
                    chunk[0] = color.b;
                    chunk[1] = color.g;
                    chunk[2] = color.r;
                    chunk[3] = color.a;
                }
            }
            PixelFormat::Rgb8 => {
                for chunk in self.pixels.chunks_exact_mut(3) {
                    chunk[0] = color.r;
                    chunk[1] = color.g;
                    chunk[2] = color.b;
                }
            }
            PixelFormat::Alpha8 => {
                self.pixels.fill(color.a);
            }
        }
    }
    
    /// Get a pixel at (x, y)
    pub fn get_pixel(&self, x: u32, y: u32) -> Option<Color> {
        if x >= self.width || y >= self.height {
            return None;
        }
        
        let bpp = self.format.bytes_per_pixel();
        let idx = ((y * self.width + x) as usize) * bpp;
        
        match self.format {
            PixelFormat::Rgba8 => Some(Color::rgba(
                self.pixels[idx],
                self.pixels[idx + 1],
                self.pixels[idx + 2],
                self.pixels[idx + 3],
            )),
            PixelFormat::Bgra8 => Some(Color::rgba(
                self.pixels[idx + 2],
                self.pixels[idx + 1],
                self.pixels[idx],
                self.pixels[idx + 3],
            )),
            PixelFormat::Rgb8 => Some(Color::rgb(
                self.pixels[idx],
                self.pixels[idx + 1],
                self.pixels[idx + 2],
            )),
            PixelFormat::Alpha8 => Some(Color::rgba(255, 255, 255, self.pixels[idx])),
        }
    }
    
    /// Set a pixel at (x, y)
    pub fn set_pixel(&mut self, x: u32, y: u32, color: Color) {
        if x >= self.width || y >= self.height {
            return;
        }
        
        let bpp = self.format.bytes_per_pixel();
        let idx = ((y * self.width + x) as usize) * bpp;
        
        match self.format {
            PixelFormat::Rgba8 => {
                self.pixels[idx] = color.r;
                self.pixels[idx + 1] = color.g;
                self.pixels[idx + 2] = color.b;
                self.pixels[idx + 3] = color.a;
            }
            PixelFormat::Bgra8 => {
                self.pixels[idx] = color.b;
                self.pixels[idx + 1] = color.g;
                self.pixels[idx + 2] = color.r;
                self.pixels[idx + 3] = color.a;
            }
            PixelFormat::Rgb8 => {
                self.pixels[idx] = color.r;
                self.pixels[idx + 1] = color.g;
                self.pixels[idx + 2] = color.b;
            }
            PixelFormat::Alpha8 => {
                self.pixels[idx] = color.a;
            }
        }
    }
    
    /// Blit (copy) a region from another buffer
    pub fn blit(&mut self, src: &PixelBuffer, dst_x: i32, dst_y: i32) {
        self.blit_region(src, 0, 0, src.width, src.height, dst_x, dst_y);
    }
    
    /// Blit a region from another buffer
    pub fn blit_region(
        &mut self,
        src: &PixelBuffer,
        src_x: u32, src_y: u32,
        width: u32, height: u32,
        dst_x: i32, dst_y: i32,
    ) {
        // TODO: Handle format conversion
        if self.format != src.format {
            log::warn!("Blitting between different formats not yet supported");
            return;
        }
        
        let bpp = self.format.bytes_per_pixel();
        
        for y in 0..height {
            let sy = src_y + y;
            let dy = dst_y + y as i32;
            
            if sy >= src.height || dy < 0 || dy >= self.height as i32 {
                continue;
            }
            
            for x in 0..width {
                let sx = src_x + x;
                let dx = dst_x + x as i32;
                
                if sx >= src.width || dx < 0 || dx >= self.width as i32 {
                    continue;
                }
                
                let src_idx = ((sy * src.width + sx) as usize) * bpp;
                let dst_idx = ((dy as u32 * self.width + dx as u32) as usize) * bpp;
                
                self.pixels[dst_idx..dst_idx + bpp]
                    .copy_from_slice(&src.pixels[src_idx..src_idx + bpp]);
            }
        }
    }
    
    /// Get the byte size of the buffer
    pub fn byte_size(&self) -> usize {
        self.pixels.len()
    }
    
    /// Resize the buffer
    pub fn resize(&mut self, new_width: u32, new_height: u32) {
        let new_size = (new_width * new_height) as usize * self.format.bytes_per_pixel();
        self.pixels.resize(new_size, 0);
        self.width = new_width;
        self.height = new_height;
    }
}

/// A drawable surface with double buffering
pub struct Surface {
    pub(crate) id: SurfaceId,
    /// Front buffer (currently displayed)
    front_buffer: PixelBuffer,
    /// Back buffer (being drawn to)
    back_buffer: PixelBuffer,
    /// Surface size
    size: Size,
    /// Whether back buffer has pending changes
    dirty: bool,
    /// Generation counter for cache invalidation
    generation: u64,
    /// Pixel format
    format: PixelFormat,
}

impl Surface {
    /// Create a new surface with double buffering
    pub fn new(id: SurfaceId, size: Size) -> Self {
        Self::with_format(id, size, PixelFormat::Rgba8)
    }
    
    /// Create a surface with a specific pixel format
    pub fn with_format(id: SurfaceId, size: Size, format: PixelFormat) -> Self {
        let width = size.width as u32;
        let height = size.height as u32;
        
        Self {
            id,
            front_buffer: PixelBuffer::new(width, height, format),
            back_buffer: PixelBuffer::new(width, height, format),
            size,
            dirty: true,
            generation: 0,
            format,
        }
    }
    
    /// Get the surface ID
    pub fn id(&self) -> SurfaceId {
        self.id
    }
    
    /// Get the surface size
    pub fn size(&self) -> Size {
        self.size
    }
    
    /// Get the pixel format
    pub fn format(&self) -> PixelFormat {
        self.format
    }
    
    /// Get the back buffer for drawing
    pub fn back_buffer(&self) -> &PixelBuffer {
        &self.back_buffer
    }
    
    /// Get the back buffer mutably for drawing
    pub fn back_buffer_mut(&mut self) -> &mut PixelBuffer {
        self.dirty = true;
        &mut self.back_buffer
    }
    
    /// Get the front buffer (current display)
    pub fn front_buffer(&self) -> &PixelBuffer {
        &self.front_buffer
    }
    
    /// Swap front and back buffers (present)
    pub fn swap_buffers(&mut self) {
        std::mem::swap(&mut self.front_buffer, &mut self.back_buffer);
        self.dirty = false;
        self.generation += 1;
    }
    
    /// Clear the back buffer
    pub fn clear(&mut self, color: Color) {
        self.back_buffer.clear(color);
        self.dirty = true;
    }
    
    /// Resize the surface
    pub fn resize(&mut self, new_size: Size) {
        let width = new_size.width as u32;
        let height = new_size.height as u32;
        
        self.front_buffer.resize(width, height);
        self.back_buffer.resize(width, height);
        self.size = new_size;
        self.dirty = true;
        self.generation += 1;
    }
    
    /// Mark the surface as dirty
    pub fn mark_dirty(&mut self) {
        self.dirty = true;
    }
    
    /// Check if surface needs redraw
    pub fn is_dirty(&self) -> bool {
        self.dirty
    }
    
    /// Get the generation counter
    pub fn generation(&self) -> u64 {
        self.generation
    }
    
    /// Get bounds as a rectangle
    pub fn bounds(&self) -> Rect {
        Rect::from_size(self.size)
    }
    
    /// Get the raw pixel data from the front buffer
    pub fn pixels(&self) -> &[u8] {
        &self.front_buffer.pixels
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_pixel_buffer_creation() {
        let buffer = PixelBuffer::new(100, 100, PixelFormat::Rgba8);
        assert_eq!(buffer.width, 100);
        assert_eq!(buffer.height, 100);
        assert_eq!(buffer.pixels.len(), 100 * 100 * 4);
    }
    
    #[test]
    fn test_pixel_buffer_set_get() {
        let mut buffer = PixelBuffer::new(10, 10, PixelFormat::Rgba8);
        let color = Color::rgb(255, 128, 64);
        
        buffer.set_pixel(5, 5, color);
        let retrieved = buffer.get_pixel(5, 5).unwrap();
        
        assert_eq!(retrieved.r, color.r);
        assert_eq!(retrieved.g, color.g);
        assert_eq!(retrieved.b, color.b);
    }
    
    #[test]
    fn test_surface_double_buffering() {
        let mut surface = Surface::new(SurfaceId(1), Size::new(100.0, 100.0));
        
        // Draw to back buffer
        surface.back_buffer_mut().set_pixel(0, 0, Color::RED);
        assert!(surface.is_dirty());
        
        // Swap buffers
        surface.swap_buffers();
        assert!(!surface.is_dirty());
        
        // Front buffer should now have the pixel
        let color = surface.front_buffer().get_pixel(0, 0).unwrap();
        assert_eq!(color.r, 255);
    }
}
