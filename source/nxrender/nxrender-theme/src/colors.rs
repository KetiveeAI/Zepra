//! Color Palette System
//!
//! Provides semantic color definitions for theming.

use nxgfx::Color;

/// A complete color palette with shades
#[derive(Debug, Clone)]
pub struct ColorScale {
    /// Lightest shade (50)
    pub shade_50: Color,
    /// Very light shade (100)
    pub shade_100: Color,
    /// Light shade (200)
    pub shade_200: Color,
    /// Light-medium shade (300)
    pub shade_300: Color,
    /// Medium-light shade (400)
    pub shade_400: Color,
    /// Base/medium shade (500)
    pub shade_500: Color,
    /// Medium-dark shade (600)
    pub shade_600: Color,
    /// Dark shade (700)
    pub shade_700: Color,
    /// Very dark shade (800)
    pub shade_800: Color,
    /// Darkest shade (900)
    pub shade_900: Color,
}

impl ColorScale {
    /// Create a color scale from a base color (generates shades automatically)
    pub fn from_base(base: Color) -> Self {
        // Generate lighter shades by mixing with white
        // Generate darker shades by mixing with black
        Self {
            shade_50: base.lighten(0.45),
            shade_100: base.lighten(0.40),
            shade_200: base.lighten(0.30),
            shade_300: base.lighten(0.20),
            shade_400: base.lighten(0.10),
            shade_500: base,
            shade_600: base.darken(0.10),
            shade_700: base.darken(0.20),
            shade_800: base.darken(0.30),
            shade_900: base.darken(0.40),
        }
    }
    
    /// Create a gray scale
    pub fn gray() -> Self {
        Self {
            shade_50: Color::rgb(249, 250, 251),
            shade_100: Color::rgb(243, 244, 246),
            shade_200: Color::rgb(229, 231, 235),
            shade_300: Color::rgb(209, 213, 219),
            shade_400: Color::rgb(156, 163, 175),
            shade_500: Color::rgb(107, 114, 128),
            shade_600: Color::rgb(75, 85, 99),
            shade_700: Color::rgb(55, 65, 81),
            shade_800: Color::rgb(31, 41, 55),
            shade_900: Color::rgb(17, 24, 39),
        }
    }
    
    /// Create a blue scale (primary)
    pub fn blue() -> Self {
        Self {
            shade_50: Color::rgb(239, 246, 255),
            shade_100: Color::rgb(219, 234, 254),
            shade_200: Color::rgb(191, 219, 254),
            shade_300: Color::rgb(147, 197, 253),
            shade_400: Color::rgb(96, 165, 250),
            shade_500: Color::rgb(59, 130, 246),
            shade_600: Color::rgb(37, 99, 235),
            shade_700: Color::rgb(29, 78, 216),
            shade_800: Color::rgb(30, 64, 175),
            shade_900: Color::rgb(30, 58, 138),
        }
    }
    
    /// Create a red scale (error/danger)
    pub fn red() -> Self {
        Self {
            shade_50: Color::rgb(254, 242, 242),
            shade_100: Color::rgb(254, 226, 226),
            shade_200: Color::rgb(254, 202, 202),
            shade_300: Color::rgb(252, 165, 165),
            shade_400: Color::rgb(248, 113, 113),
            shade_500: Color::rgb(239, 68, 68),
            shade_600: Color::rgb(220, 38, 38),
            shade_700: Color::rgb(185, 28, 28),
            shade_800: Color::rgb(153, 27, 27),
            shade_900: Color::rgb(127, 29, 29),
        }
    }
    
    /// Create a green scale (success)
    pub fn green() -> Self {
        Self {
            shade_50: Color::rgb(240, 253, 244),
            shade_100: Color::rgb(220, 252, 231),
            shade_200: Color::rgb(187, 247, 208),
            shade_300: Color::rgb(134, 239, 172),
            shade_400: Color::rgb(74, 222, 128),
            shade_500: Color::rgb(34, 197, 94),
            shade_600: Color::rgb(22, 163, 74),
            shade_700: Color::rgb(21, 128, 61),
            shade_800: Color::rgb(22, 101, 52),
            shade_900: Color::rgb(20, 83, 45),
        }
    }
    
    /// Create an amber/yellow scale (warning)
    pub fn amber() -> Self {
        Self {
            shade_50: Color::rgb(255, 251, 235),
            shade_100: Color::rgb(254, 243, 199),
            shade_200: Color::rgb(253, 230, 138),
            shade_300: Color::rgb(252, 211, 77),
            shade_400: Color::rgb(251, 191, 36),
            shade_500: Color::rgb(245, 158, 11),
            shade_600: Color::rgb(217, 119, 6),
            shade_700: Color::rgb(180, 83, 9),
            shade_800: Color::rgb(146, 64, 14),
            shade_900: Color::rgb(120, 53, 15),
        }
    }
}

/// Semantic colors for UI elements
#[derive(Debug, Clone)]
pub struct SemanticColors {
    /// Primary brand color
    pub primary: Color,
    /// Primary color on hover
    pub primary_hover: Color,
    /// Primary color when pressed
    pub primary_pressed: Color,
    
    /// Secondary actions
    pub secondary: Color,
    pub secondary_hover: Color,
    
    /// Success state
    pub success: Color,
    pub success_hover: Color,
    
    /// Warning state
    pub warning: Color,
    pub warning_hover: Color,
    
    /// Error/danger state
    pub error: Color,
    pub error_hover: Color,
    
    /// Info state
    pub info: Color,
    pub info_hover: Color,
}

impl Default for SemanticColors {
    fn default() -> Self {
        let blue = ColorScale::blue();
        let gray = ColorScale::gray();
        let green = ColorScale::green();
        let amber = ColorScale::amber();
        let red = ColorScale::red();
        
        Self {
            primary: blue.shade_500,
            primary_hover: blue.shade_600,
            primary_pressed: blue.shade_700,
            
            secondary: gray.shade_500,
            secondary_hover: gray.shade_600,
            
            success: green.shade_500,
            success_hover: green.shade_600,
            
            warning: amber.shade_500,
            warning_hover: amber.shade_600,
            
            error: red.shade_500,
            error_hover: red.shade_600,
            
            info: blue.shade_400,
            info_hover: blue.shade_500,
        }
    }
}

/// Surface/background colors
#[derive(Debug, Clone)]
pub struct SurfaceColors {
    /// Main background
    pub background: Color,
    /// Elevated surface (cards, modals)
    pub surface: Color,
    /// Surface on hover
    pub surface_hover: Color,
    /// Surface when selected
    pub surface_selected: Color,
    /// Overlay/scrim
    pub overlay: Color,
}

impl SurfaceColors {
    /// Light surface colors
    pub fn light() -> Self {
        Self {
            background: Color::rgb(255, 255, 255),
            surface: Color::rgb(249, 250, 251),
            surface_hover: Color::rgb(243, 244, 246),
            surface_selected: Color::rgb(239, 246, 255),
            overlay: Color::rgba(0, 0, 0, 128),
        }
    }
    
    /// Dark surface colors
    pub fn dark() -> Self {
        Self {
            background: Color::rgb(17, 24, 39),
            surface: Color::rgb(31, 41, 55),
            surface_hover: Color::rgb(55, 65, 81),
            surface_selected: Color::rgb(30, 58, 138),
            overlay: Color::rgba(0, 0, 0, 180),
        }
    }
}

/// Text colors
#[derive(Debug, Clone)]
pub struct TextColors {
    /// Primary text
    pub primary: Color,
    /// Secondary/muted text
    pub secondary: Color,
    /// Disabled text
    pub disabled: Color,
    /// Text on primary color background
    pub on_primary: Color,
    /// Placeholder text
    pub placeholder: Color,
    /// Link text
    pub link: Color,
}

impl TextColors {
    /// Light theme text colors
    pub fn light() -> Self {
        let gray = ColorScale::gray();
        let blue = ColorScale::blue();
        
        Self {
            primary: gray.shade_900,
            secondary: gray.shade_600,
            disabled: gray.shade_400,
            on_primary: Color::WHITE,
            placeholder: gray.shade_400,
            link: blue.shade_600,
        }
    }
    
    /// Dark theme text colors
    pub fn dark() -> Self {
        let gray = ColorScale::gray();
        let blue = ColorScale::blue();
        
        Self {
            primary: gray.shade_100,
            secondary: gray.shade_400,
            disabled: gray.shade_600,
            on_primary: Color::WHITE,
            placeholder: gray.shade_500,
            link: blue.shade_400,
        }
    }
}

/// Border colors
#[derive(Debug, Clone)]
pub struct BorderColors {
    /// Default border
    pub default: Color,
    /// Border on focus
    pub focus: Color,
    /// Error border
    pub error: Color,
    /// Divider/separator
    pub divider: Color,
}

impl BorderColors {
    /// Light theme borders
    pub fn light() -> Self {
        let gray = ColorScale::gray();
        let blue = ColorScale::blue();
        let red = ColorScale::red();
        
        Self {
            default: gray.shade_300,
            focus: blue.shade_500,
            error: red.shade_500,
            divider: gray.shade_200,
        }
    }
    
    /// Dark theme borders
    pub fn dark() -> Self {
        let gray = ColorScale::gray();
        let blue = ColorScale::blue();
        let red = ColorScale::red();
        
        Self {
            default: gray.shade_600,
            focus: blue.shade_400,
            error: red.shade_400,
            divider: gray.shade_700,
        }
    }
}

/// Complete color palette for a theme
#[derive(Debug, Clone)]
pub struct ColorPalette {
    /// Semantic action colors
    pub semantic: SemanticColors,
    /// Surface/background colors
    pub surface: SurfaceColors,
    /// Text colors
    pub text: TextColors,
    /// Border colors
    pub border: BorderColors,
    /// Color scales for custom use
    pub gray: ColorScale,
    pub blue: ColorScale,
    pub red: ColorScale,
    pub green: ColorScale,
    pub amber: ColorScale,
}

impl ColorPalette {
    /// Create a light color palette
    pub fn light() -> Self {
        Self {
            semantic: SemanticColors::default(),
            surface: SurfaceColors::light(),
            text: TextColors::light(),
            border: BorderColors::light(),
            gray: ColorScale::gray(),
            blue: ColorScale::blue(),
            red: ColorScale::red(),
            green: ColorScale::green(),
            amber: ColorScale::amber(),
        }
    }
    
    /// Create a dark color palette
    pub fn dark() -> Self {
        Self {
            semantic: SemanticColors::default(),
            surface: SurfaceColors::dark(),
            text: TextColors::dark(),
            border: BorderColors::dark(),
            gray: ColorScale::gray(),
            blue: ColorScale::blue(),
            red: ColorScale::red(),
            green: ColorScale::green(),
            amber: ColorScale::amber(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_color_scale_gray() {
        let gray = ColorScale::gray();
        // Verify shades get darker
        assert!(gray.shade_50.r > gray.shade_900.r);
    }
    
    #[test]
    fn test_color_palette_light() {
        let palette = ColorPalette::light();
        // Light background should be close to white
        assert!(palette.surface.background.r > 200);
    }
    
    #[test]
    fn test_color_palette_dark() {
        let palette = ColorPalette::dark();
        // Dark background should be dark
        assert!(palette.surface.background.r < 50);
    }
}
