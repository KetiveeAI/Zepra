//! Typography System
//!
//! Font styles and text sizing for consistent typography.

/// Font weight
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum FontWeight {
    Thin,       // 100
    ExtraLight, // 200
    Light,      // 300
    #[default]
    Regular,    // 400
    Medium,     // 500
    SemiBold,   // 600
    Bold,       // 700
    ExtraBold,  // 800
    Black,      // 900
}

impl FontWeight {
    /// Get the numeric weight value
    pub fn value(&self) -> u16 {
        match self {
            FontWeight::Thin => 100,
            FontWeight::ExtraLight => 200,
            FontWeight::Light => 300,
            FontWeight::Regular => 400,
            FontWeight::Medium => 500,
            FontWeight::SemiBold => 600,
            FontWeight::Bold => 700,
            FontWeight::ExtraBold => 800,
            FontWeight::Black => 900,
        }
    }
}

/// Font style (normal vs italic)
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum FontStyle {
    #[default]
    Normal,
    Italic,
}

/// A text style definition
#[derive(Debug, Clone)]
pub struct TextStyle {
    /// Font family name
    pub family: String,
    /// Font size in points
    pub size: f32,
    /// Font weight
    pub weight: FontWeight,
    /// Font style
    pub style: FontStyle,
    /// Line height multiplier
    pub line_height: f32,
    /// Letter spacing in pixels
    pub letter_spacing: f32,
}

impl Default for TextStyle {
    fn default() -> Self {
        Self {
            family: String::from("Inter"),
            size: 14.0,
            weight: FontWeight::Regular,
            style: FontStyle::Normal,
            line_height: 1.5,
            letter_spacing: 0.0,
        }
    }
}

impl TextStyle {
    /// Create a text style with a specific size
    pub fn size(size: f32) -> Self {
        Self { size, ..Default::default() }
    }
    
    /// Set the font weight
    pub fn weight(mut self, weight: FontWeight) -> Self {
        self.weight = weight;
        self
    }
    
    /// Set the font family
    pub fn family(mut self, family: impl Into<String>) -> Self {
        self.family = family.into();
        self
    }
    
    /// Make bold
    pub fn bold(self) -> Self {
        self.weight(FontWeight::Bold)
    }
    
    /// Make medium weight
    pub fn medium(self) -> Self {
        self.weight(FontWeight::Medium)
    }
    
    /// Make italic
    pub fn italic(mut self) -> Self {
        self.style = FontStyle::Italic;
        self
    }
}

/// Typography scale with predefined text styles
#[derive(Debug, Clone)]
pub struct Typography {
    /// Display text (hero, large headings) - 48px
    pub display_large: TextStyle,
    /// Display medium - 36px
    pub display_medium: TextStyle,
    /// Display small - 30px
    pub display_small: TextStyle,
    
    /// Headline large - 24px
    pub headline_large: TextStyle,
    /// Headline medium - 20px
    pub headline_medium: TextStyle,
    /// Headline small - 18px
    pub headline_small: TextStyle,
    
    /// Title large - 16px bold
    pub title_large: TextStyle,
    /// Title medium - 14px bold
    pub title_medium: TextStyle,
    /// Title small - 12px bold
    pub title_small: TextStyle,
    
    /// Body large - 16px
    pub body_large: TextStyle,
    /// Body medium - 14px
    pub body_medium: TextStyle,
    /// Body small - 12px
    pub body_small: TextStyle,
    
    /// Label large - 14px medium
    pub label_large: TextStyle,
    /// Label medium - 12px medium
    pub label_medium: TextStyle,
    /// Label small - 10px medium
    pub label_small: TextStyle,
}

impl Default for Typography {
    fn default() -> Self {
        Self::with_font("Inter")
    }
}

impl Typography {
    /// Create typography with a specific font family
    pub fn with_font(family: &str) -> Self {
        let family = family.to_string();
        
        Self {
            display_large: TextStyle {
                family: family.clone(),
                size: 48.0,
                weight: FontWeight::Bold,
                line_height: 1.2,
                ..Default::default()
            },
            display_medium: TextStyle {
                family: family.clone(),
                size: 36.0,
                weight: FontWeight::Bold,
                line_height: 1.2,
                ..Default::default()
            },
            display_small: TextStyle {
                family: family.clone(),
                size: 30.0,
                weight: FontWeight::SemiBold,
                line_height: 1.2,
                ..Default::default()
            },
            
            headline_large: TextStyle {
                family: family.clone(),
                size: 24.0,
                weight: FontWeight::SemiBold,
                line_height: 1.3,
                ..Default::default()
            },
            headline_medium: TextStyle {
                family: family.clone(),
                size: 20.0,
                weight: FontWeight::SemiBold,
                line_height: 1.3,
                ..Default::default()
            },
            headline_small: TextStyle {
                family: family.clone(),
                size: 18.0,
                weight: FontWeight::SemiBold,
                line_height: 1.3,
                ..Default::default()
            },
            
            title_large: TextStyle {
                family: family.clone(),
                size: 16.0,
                weight: FontWeight::Medium,
                line_height: 1.4,
                ..Default::default()
            },
            title_medium: TextStyle {
                family: family.clone(),
                size: 14.0,
                weight: FontWeight::Medium,
                line_height: 1.4,
                ..Default::default()
            },
            title_small: TextStyle {
                family: family.clone(),
                size: 12.0,
                weight: FontWeight::Medium,
                line_height: 1.4,
                ..Default::default()
            },
            
            body_large: TextStyle {
                family: family.clone(),
                size: 16.0,
                weight: FontWeight::Regular,
                line_height: 1.5,
                ..Default::default()
            },
            body_medium: TextStyle {
                family: family.clone(),
                size: 14.0,
                weight: FontWeight::Regular,
                line_height: 1.5,
                ..Default::default()
            },
            body_small: TextStyle {
                family: family.clone(),
                size: 12.0,
                weight: FontWeight::Regular,
                line_height: 1.5,
                ..Default::default()
            },
            
            label_large: TextStyle {
                family: family.clone(),
                size: 14.0,
                weight: FontWeight::Medium,
                line_height: 1.4,
                letter_spacing: 0.5,
                ..Default::default()
            },
            label_medium: TextStyle {
                family: family.clone(),
                size: 12.0,
                weight: FontWeight::Medium,
                line_height: 1.4,
                letter_spacing: 0.5,
                ..Default::default()
            },
            label_small: TextStyle {
                family: family.clone(),
                size: 10.0,
                weight: FontWeight::Medium,
                line_height: 1.4,
                letter_spacing: 0.5,
                ..Default::default()
            },
        }
    }
    
    /// Create system font typography (platform specific)
    pub fn system() -> Self {
        #[cfg(target_os = "macos")]
        return Self::with_font("SF Pro");
        
        #[cfg(target_os = "windows")]
        return Self::with_font("Segoe UI");
        
        #[cfg(not(any(target_os = "macos", target_os = "windows")))]
        return Self::with_font("Inter");
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_typography_creation() {
        let typography = Typography::default();
        assert_eq!(typography.body_medium.size, 14.0);
    }
    
    #[test]
    fn test_text_style_builder() {
        let style = TextStyle::size(20.0).bold().italic();
        assert_eq!(style.size, 20.0);
        assert_eq!(style.weight, FontWeight::Bold);
        assert_eq!(style.style, FontStyle::Italic);
    }
    
    #[test]
    fn test_font_weight_value() {
        assert_eq!(FontWeight::Regular.value(), 400);
        assert_eq!(FontWeight::Bold.value(), 700);
    }
}
