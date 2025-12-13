//! Color types and utilities

/// RGBA Color
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Color {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub a: u8,
}

impl Color {
    // Common colors
    pub const WHITE: Color = Color::rgb(255, 255, 255);
    pub const BLACK: Color = Color::rgb(0, 0, 0);
    pub const RED: Color = Color::rgb(255, 0, 0);
    pub const GREEN: Color = Color::rgb(0, 255, 0);
    pub const BLUE: Color = Color::rgb(0, 0, 255);
    pub const YELLOW: Color = Color::rgb(255, 255, 0);
    pub const CYAN: Color = Color::rgb(0, 255, 255);
    pub const MAGENTA: Color = Color::rgb(255, 0, 255);
    pub const TRANSPARENT: Color = Color::rgba(0, 0, 0, 0);
    
    /// Create an RGB color (fully opaque)
    pub const fn rgb(r: u8, g: u8, b: u8) -> Self {
        Self { r, g, b, a: 255 }
    }
    
    /// Create an RGBA color
    pub const fn rgba(r: u8, g: u8, b: u8, a: u8) -> Self {
        Self { r, g, b, a }
    }
    
    /// Create a grayscale color
    pub const fn gray(value: u8) -> Self {
        Self::rgb(value, value, value)
    }
    
    /// Create a color from HSL values
    pub fn from_hsl(h: f32, s: f32, l: f32) -> Self {
        let h = h % 360.0;
        let s = s.clamp(0.0, 1.0);
        let l = l.clamp(0.0, 1.0);
        
        let c = (1.0 - (2.0 * l - 1.0).abs()) * s;
        let x = c * (1.0 - ((h / 60.0) % 2.0 - 1.0).abs());
        let m = l - c / 2.0;
        
        let (r, g, b) = if h < 60.0 {
            (c, x, 0.0)
        } else if h < 120.0 {
            (x, c, 0.0)
        } else if h < 180.0 {
            (0.0, c, x)
        } else if h < 240.0 {
            (0.0, x, c)
        } else if h < 300.0 {
            (x, 0.0, c)
        } else {
            (c, 0.0, x)
        };
        
        Self::rgb(
            ((r + m) * 255.0) as u8,
            ((g + m) * 255.0) as u8,
            ((b + m) * 255.0) as u8,
        )
    }
    
    /// Create a color from a hex string (e.g., "#FF5500" or "FF5500")
    pub fn from_hex(hex: &str) -> Option<Self> {
        let hex = hex.trim_start_matches('#');
        
        if hex.len() == 6 {
            let r = u8::from_str_radix(&hex[0..2], 16).ok()?;
            let g = u8::from_str_radix(&hex[2..4], 16).ok()?;
            let b = u8::from_str_radix(&hex[4..6], 16).ok()?;
            Some(Self::rgb(r, g, b))
        } else if hex.len() == 8 {
            let r = u8::from_str_radix(&hex[0..2], 16).ok()?;
            let g = u8::from_str_radix(&hex[2..4], 16).ok()?;
            let b = u8::from_str_radix(&hex[4..6], 16).ok()?;
            let a = u8::from_str_radix(&hex[6..8], 16).ok()?;
            Some(Self::rgba(r, g, b, a))
        } else {
            None
        }
    }
    
    /// Convert to a u32 value (RGBA format)
    pub fn to_u32(&self) -> u32 {
        ((self.r as u32) << 24)
            | ((self.g as u32) << 16)
            | ((self.b as u32) << 8)
            | (self.a as u32)
    }
    
    /// Get as normalized floats (0.0 - 1.0)
    pub fn to_f32_array(&self) -> [f32; 4] {
        [
            self.r as f32 / 255.0,
            self.g as f32 / 255.0,
            self.b as f32 / 255.0,
            self.a as f32 / 255.0,
        ]
    }
    
    /// Set alpha value
    pub fn with_alpha(self, a: u8) -> Self {
        Self { a, ..self }
    }
    
    /// Lighten the color by a percentage (0.0 - 1.0)
    pub fn lighten(&self, amount: f32) -> Self {
        let amount = amount.clamp(0.0, 1.0);
        Self::rgb(
            (self.r as f32 + (255.0 - self.r as f32) * amount) as u8,
            (self.g as f32 + (255.0 - self.g as f32) * amount) as u8,
            (self.b as f32 + (255.0 - self.b as f32) * amount) as u8,
        )
    }
    
    /// Darken the color by a percentage (0.0 - 1.0)
    pub fn darken(&self, amount: f32) -> Self {
        let amount = amount.clamp(0.0, 1.0);
        Self::rgb(
            (self.r as f32 * (1.0 - amount)) as u8,
            (self.g as f32 * (1.0 - amount)) as u8,
            (self.b as f32 * (1.0 - amount)) as u8,
        )
    }
    
    /// Interpolate between two colors
    pub fn lerp(a: Color, b: Color, t: f32) -> Color {
        let t = t.clamp(0.0, 1.0);
        Color::rgba(
            (a.r as f32 + (b.r as f32 - a.r as f32) * t) as u8,
            (a.g as f32 + (b.g as f32 - a.g as f32) * t) as u8,
            (a.b as f32 + (b.b as f32 - a.b as f32) * t) as u8,
            (a.a as f32 + (b.a as f32 - a.a as f32) * t) as u8,
        )
    }
}

impl Default for Color {
    fn default() -> Self {
        Self::BLACK
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_color_from_hex() {
        assert_eq!(
            Color::from_hex("#FF5500"),
            Some(Color::rgb(255, 85, 0))
        );
        assert_eq!(
            Color::from_hex("00FF00"),
            Some(Color::rgb(0, 255, 0))
        );
    }
    
    #[test]
    fn test_color_lerp() {
        let black = Color::BLACK;
        let white = Color::WHITE;
        let mid = Color::lerp(black, white, 0.5);
        
        assert_eq!(mid.r, 127);
        assert_eq!(mid.g, 127);
        assert_eq!(mid.b, 127);
    }
}
