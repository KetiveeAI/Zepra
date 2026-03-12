// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file theme.cpp
 * @brief Theme system implementation
 */

#include "theme/theme.h"

namespace NXRender {

static Theme g_currentTheme;
static bool g_themeInitialized = false;

Theme* currentTheme() {
    if (!g_themeInitialized) {
        g_currentTheme = Theme::light();
        g_themeInitialized = true;
    }
    return &g_currentTheme;
}

void setTheme(const Theme& theme) {
    g_currentTheme = theme;
    g_themeInitialized = true;
}

Theme::Theme() {
    // Initialize with safe defaults, definitely NOT Theme::light() to avoid recursion!
    name = "Uninitialized";
    isDark = false;
}

Theme Theme::light() {
    Theme t;
    t.name = "Light";
    t.isDark = false;
    
    // Primary colors (Material Blue)
    t.colors.primary = Color(0x2196F3);
    t.colors.primaryHover = Color(0x1976D2);
    t.colors.primaryPressed = Color(0x1565C0);
    t.colors.onPrimary = Color::white();
    
    // Secondary colors (Material Teal)
    t.colors.secondary = Color(0x009688);
    t.colors.secondaryHover = Color(0x00796B);
    t.colors.onSecondary = Color::white();
    
    // Surface colors
    t.colors.background = Color(0xFAFAFA);
    t.colors.surface = Color::white();
    t.colors.surfaceElevated = Color(0xFFFFFF);
    t.colors.onBackground = Color(0x212121);
    t.colors.onSurface = Color(0x212121);
    
    // Accent
    t.colors.accent = Color(0xFF5722);
    t.colors.accentHover = Color(0xE64A19);
    t.colors.onAccent = Color::white();
    
    // Status colors
    t.colors.success = Color(0x4CAF50);
    t.colors.warning = Color(0xFFC107);
    t.colors.error = Color(0xF44336);
    t.colors.info = Color(0x2196F3);
    
    // Borders & dividers
    t.colors.border = Color(0xE0E0E0);
    t.colors.divider = Color(0xBDBDBD);
    
    // Text
    t.colors.textPrimary = Color(0x212121);
    t.colors.textSecondary = Color(0x757575);
    t.colors.textDisabled = Color(0xBDBDBD);
    t.colors.textLink = Color(0x1976D2);
    
    // Shadow
    t.colors.shadow = Color(0, 0, 0, 40);
    
    return t;
}

Theme Theme::dark() {
    Theme t;
    t.name = "Dark";
    t.isDark = true;
    
    // Primary colors
    t.colors.primary = Color(0x90CAF9);
    t.colors.primaryHover = Color(0x64B5F6);
    t.colors.primaryPressed = Color(0x42A5F5);
    t.colors.onPrimary = Color(0x212121);
    
    // Secondary colors
    t.colors.secondary = Color(0x80CBC4);
    t.colors.secondaryHover = Color(0x4DB6AC);
    t.colors.onSecondary = Color(0x212121);
    
    // Surface colors
    t.colors.background = Color(0x121212);
    t.colors.surface = Color(0x1E1E1E);
    t.colors.surfaceElevated = Color(0x2D2D2D);
    t.colors.onBackground = Color(0xE0E0E0);
    t.colors.onSurface = Color(0xE0E0E0);
    
    // Accent
    t.colors.accent = Color(0xFF8A65);
    t.colors.accentHover = Color(0xFF7043);
    t.colors.onAccent = Color(0x212121);
    
    // Status colors
    t.colors.success = Color(0x81C784);
    t.colors.warning = Color(0xFFD54F);
    t.colors.error = Color(0xE57373);
    t.colors.info = Color(0x64B5F6);
    
    // Borders & dividers
    t.colors.border = Color(0x424242);
    t.colors.divider = Color(0x616161);
    
    // Text
    t.colors.textPrimary = Color(0xE0E0E0);
    t.colors.textSecondary = Color(0x9E9E9E);
    t.colors.textDisabled = Color(0x616161);
    t.colors.textLink = Color(0x64B5F6);
    
    // Shadow
    t.colors.shadow = Color(0, 0, 0, 80);
    
    return t;
}

Theme Theme::highContrast() {
    Theme t;
    t.name = "High Contrast";
    t.isDark = true;
    
    // Primary colors - bright yellow
    t.colors.primary = Color(0xFFFF00);
    t.colors.primaryHover = Color(0xFFEB3B);
    t.colors.primaryPressed = Color(0xFDD835);
    t.colors.onPrimary = Color::black();
    
    // Surface colors - pure black
    t.colors.background = Color::black();
    t.colors.surface = Color::black();
    t.colors.surfaceElevated = Color(0x1A1A1A);
    t.colors.onBackground = Color::white();
    t.colors.onSurface = Color::white();
    
    // Borders - white
    t.colors.border = Color::white();
    t.colors.divider = Color::white();
    
    // Text - white
    t.colors.textPrimary = Color::white();
    t.colors.textSecondary = Color(0xCCCCCC);
    t.colors.textDisabled = Color(0x808080);
    t.colors.textLink = Color(0x00FFFF);
    
    return t;
}

} // namespace NXRender
