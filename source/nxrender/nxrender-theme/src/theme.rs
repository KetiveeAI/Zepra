//! Theme Definition
//!
//! Main theme structure combining colors, typography, and spacing.

use crate::colors::ColorPalette;
use crate::fonts::Typography;

/// Spacing scale
#[derive(Debug, Clone)]
pub struct Spacing {
    /// Extra small - 4px
    pub xs: f32,
    /// Small - 8px
    pub sm: f32,
    /// Medium - 16px
    pub md: f32,
    /// Large - 24px
    pub lg: f32,
    /// Extra large - 32px
    pub xl: f32,
    /// 2x extra large - 48px
    pub xxl: f32,
}

impl Default for Spacing {
    fn default() -> Self {
        Self {
            xs: 4.0,
            sm: 8.0,
            md: 16.0,
            lg: 24.0,
            xl: 32.0,
            xxl: 48.0,
        }
    }
}

/// Border radius scale
#[derive(Debug, Clone)]
pub struct BorderRadius {
    /// None - 0px
    pub none: f32,
    /// Small - 4px
    pub sm: f32,
    /// Medium - 6px
    pub md: f32,
    /// Large - 8px
    pub lg: f32,
    /// Extra large - 12px
    pub xl: f32,
    /// Full circle
    pub full: f32,
}

impl Default for BorderRadius {
    fn default() -> Self {
        Self {
            none: 0.0,
            sm: 4.0,
            md: 6.0,
            lg: 8.0,
            xl: 12.0,
            full: 9999.0,
        }
    }
}

/// Shadow definition
#[derive(Debug, Clone)]
pub struct Shadow {
    pub offset_x: f32,
    pub offset_y: f32,
    pub blur: f32,
    pub spread: f32,
    pub color: nxgfx::Color,
}

/// Shadow scale
#[derive(Debug, Clone)]
pub struct Shadows {
    /// No shadow
    pub none: Shadow,
    /// Small shadow (buttons, inputs)
    pub sm: Shadow,
    /// Medium shadow (cards, dropdowns)
    pub md: Shadow,
    /// Large shadow (modals, popovers)
    pub lg: Shadow,
    /// Extra large shadow (dialogs)
    pub xl: Shadow,
}

impl Default for Shadows {
    fn default() -> Self {
        use nxgfx::Color;
        
        Self {
            none: Shadow {
                offset_x: 0.0,
                offset_y: 0.0,
                blur: 0.0,
                spread: 0.0,
                color: Color::TRANSPARENT,
            },
            sm: Shadow {
                offset_x: 0.0,
                offset_y: 1.0,
                blur: 2.0,
                spread: 0.0,
                color: Color::rgba(0, 0, 0, 20),
            },
            md: Shadow {
                offset_x: 0.0,
                offset_y: 4.0,
                blur: 6.0,
                spread: -1.0,
                color: Color::rgba(0, 0, 0, 25),
            },
            lg: Shadow {
                offset_x: 0.0,
                offset_y: 10.0,
                blur: 15.0,
                spread: -3.0,
                color: Color::rgba(0, 0, 0, 30),
            },
            xl: Shadow {
                offset_x: 0.0,
                offset_y: 20.0,
                blur: 25.0,
                spread: -5.0,
                color: Color::rgba(0, 0, 0, 35),
            },
        }
    }
}

/// Theme mode
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum ThemeMode {
    #[default]
    Light,
    Dark,
}

/// Complete theme definition
#[derive(Debug, Clone)]
pub struct Theme {
    /// Theme name
    pub name: String,
    /// Theme mode
    pub mode: ThemeMode,
    /// Color palette
    pub colors: ColorPalette,
    /// Typography
    pub typography: Typography,
    /// Spacing scale
    pub spacing: Spacing,
    /// Border radius scale
    pub radius: BorderRadius,
    /// Shadow scale
    pub shadows: Shadows,
}

impl Default for Theme {
    fn default() -> Self {
        Self::light()
    }
}

impl Theme {
    /// Create a light theme
    pub fn light() -> Self {
        Self {
            name: String::from("Light"),
            mode: ThemeMode::Light,
            colors: ColorPalette::light(),
            typography: Typography::default(),
            spacing: Spacing::default(),
            radius: BorderRadius::default(),
            shadows: Shadows::default(),
        }
    }
    
    /// Create a dark theme
    pub fn dark() -> Self {
        Self {
            name: String::from("Dark"),
            mode: ThemeMode::Dark,
            colors: ColorPalette::dark(),
            typography: Typography::default(),
            spacing: Spacing::default(),
            radius: BorderRadius::default(),
            shadows: Shadows::default(),
        }
    }
    
    /// Check if this is a dark theme
    pub fn is_dark(&self) -> bool {
        self.mode == ThemeMode::Dark
    }
    
    /// Check if this is a light theme
    pub fn is_light(&self) -> bool {
        self.mode == ThemeMode::Light
    }
    
    /// Create a custom theme
    pub fn custom(name: impl Into<String>, mode: ThemeMode) -> Self {
        let colors = match mode {
            ThemeMode::Light => ColorPalette::light(),
            ThemeMode::Dark => ColorPalette::dark(),
        };
        
        Self {
            name: name.into(),
            mode,
            colors,
            typography: Typography::default(),
            spacing: Spacing::default(),
            radius: BorderRadius::default(),
            shadows: Shadows::default(),
        }
    }
    
    /// With custom colors
    pub fn with_colors(mut self, colors: ColorPalette) -> Self {
        self.colors = colors;
        self
    }
    
    /// With custom typography
    pub fn with_typography(mut self, typography: Typography) -> Self {
        self.typography = typography;
        self
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_theme_light() {
        let theme = Theme::light();
        assert!(theme.is_light());
        assert!(!theme.is_dark());
    }
    
    #[test]
    fn test_theme_dark() {
        let theme = Theme::dark();
        assert!(theme.is_dark());
        assert!(!theme.is_light());
    }
    
    #[test]
    fn test_theme_spacing() {
        let theme = Theme::default();
        assert_eq!(theme.spacing.md, 16.0);
    }
}
