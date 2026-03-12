// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file theme.h
 * @brief Theme system for NXRender
 */

#pragma once

#include "../nxgfx/color.h"
#include <string>

namespace NXRender {

/**
 * @brief Color palette for theming
 */
struct ColorPalette {
    // Primary colors
    Color primary;          // Main brand color
    Color primaryHover;
    Color primaryPressed;
    Color onPrimary;        // Text on primary
    
    // Secondary colors
    Color secondary;
    Color secondaryHover;
    Color onSecondary;
    
    // Surface colors
    Color background;       // Main background
    Color surface;          // Cards, panels
    Color surfaceElevated;  // Elevated surfaces
    Color onBackground;     // Text on background
    Color onSurface;        // Text on surface
    
    // Accents
    Color accent;
    Color accentHover;
    Color onAccent;
    
    // Status colors
    Color success;
    Color warning;
    Color error;
    Color info;
    
    // Borders & dividers
    Color border;
    Color divider;
    
    // Text
    Color textPrimary;
    Color textSecondary;
    Color textDisabled;
    Color textLink;
    
    // Shadows
    Color shadow;
};

/**
 * @brief Typography settings
 */
struct Typography {
    std::string fontFamily = "sans-serif";
    float fontSize = 14.0f;
    float lineHeight = 1.4f;
    
    float h1Size = 32.0f;
    float h2Size = 24.0f;
    float h3Size = 20.0f;
    float h4Size = 16.0f;
    
    float smallSize = 12.0f;
    float captionSize = 11.0f;
};

/**
 * @brief Spacing scale
 */
struct Spacing {
    float xs = 4.0f;
    float sm = 8.0f;
    float md = 16.0f;
    float lg = 24.0f;
    float xl = 32.0f;
    float xxl = 48.0f;
};

/**
 * @brief Border radius scale
 */
struct BorderRadius {
    float none = 0.0f;
    float sm = 4.0f;
    float md = 8.0f;
    float lg = 12.0f;
    float xl = 16.0f;
    float full = 9999.0f;  // Pill shape
};

/**
 * @brief Complete theme
 */
class Theme {
public:
    Theme();
    ~Theme() = default;
    
    // Color scheme
    ColorPalette colors;
    
    // Typography
    Typography typography;
    
    // Spacing
    Spacing spacing;
    
    // Border radius
    BorderRadius radius;
    
    // Theme name
    std::string name = "Default";
    bool isDark = false;
    
    // Factory methods
    static Theme light();
    static Theme dark();
    static Theme highContrast();
};

// Global theme accessor
Theme* currentTheme();
void setTheme(const Theme& theme);

} // namespace NXRender
