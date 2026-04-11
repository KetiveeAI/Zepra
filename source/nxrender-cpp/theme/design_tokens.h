// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "theme.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace NXRender {

// ==================================================================
// Design Tokens — system-wide style primitives
// ==================================================================

enum class TokenType : uint8_t {
    Color, Dimension, Duration, FontFamily, FontWeight,
    Number, Shadow, Radius, Spacing, Opacity, ZIndex
};

struct DesignToken {
    std::string name;      // e.g. "color.primary.500"
    TokenType type = TokenType::Color;
    std::string value;     // Raw CSS value
    std::string reference; // Points to another token (aliases)
    std::string description;
};

class DesignTokens {
public:
    void define(const std::string& name, TokenType type, const std::string& value,
                 const std::string& description = "");
    void alias(const std::string& name, const std::string& ref);

    std::string resolve(const std::string& name) const;
    const DesignToken* get(const std::string& name) const;
    bool has(const std::string& name) const;

    std::vector<std::string> tokenNames() const;
    std::vector<std::string> tokensOfType(TokenType type) const;

    // Bulk operations
    void merge(const DesignTokens& other);
    void exportToCSS(std::string& output) const;

    // Built-in token sets
    static DesignTokens materialDesign();
    static DesignTokens zepraDefaults();

private:
    std::unordered_map<std::string, DesignToken> tokens_;
    std::string resolveChain(const std::string& name, int depth = 0) const;
};

// ==================================================================
// Component Theme Variants
// ==================================================================

struct ComponentStyle {
    std::unordered_map<std::string, std::string> properties;

    std::string get(const std::string& prop, const std::string& fallback = "") const;
    void set(const std::string& prop, const std::string& value);
};

enum class ComponentState : uint8_t {
    Default, Hover, Active, Focus, Disabled, Selected, Error, Loading
};

struct ComponentVariant {
    std::string name; // e.g. "primary", "secondary", "outlined"
    std::unordered_map<ComponentState, ComponentStyle> states;
    ComponentStyle baseStyle;

    const ComponentStyle& styleForState(ComponentState state) const;
};

class ComponentTheme {
public:
    // Register component styles
    void defineComponent(const std::string& component, const ComponentVariant& variant);

    // Lookup
    const ComponentVariant* variant(const std::string& component,
                                      const std::string& variant = "default") const;
    const ComponentStyle* style(const std::string& component,
                                 const std::string& variant = "default",
                                 ComponentState state = ComponentState::Default) const;

    // List
    std::vector<std::string> components() const;
    std::vector<std::string> variants(const std::string& component) const;

    // Factory
    static ComponentTheme defaultTheme();

private:
    // component → variant_name → ComponentVariant
    std::unordered_map<std::string,
        std::unordered_map<std::string, ComponentVariant>> components_;
};

// ==================================================================
// Animation Presets
// ==================================================================

struct AnimationPreset {
    std::string name;
    float duration = 300;    // ms
    std::string easing;      // "ease", "ease-in-out", "cubic-bezier(...)"
    float delay = 0;

    struct Keyframe {
        float offset;    // 0..1
        std::unordered_map<std::string, std::string> properties;
    };
    std::vector<Keyframe> keyframes;
};

class AnimationPresets {
public:
    static AnimationPresets& instance();

    void define(const std::string& name, const AnimationPreset& preset);
    const AnimationPreset* get(const std::string& name) const;
    std::vector<std::string> names() const;

    // Built-in presets
    void registerDefaults();

private:
    AnimationPresets() = default;
    std::unordered_map<std::string, AnimationPreset> presets_;
};

// ==================================================================
// Elevation / Shadow system
// ==================================================================

struct ElevationLevel {
    int level = 0;   // 0-24 (Material design)
    struct ShadowLayer {
        float offsetX, offsetY, blur, spread;
        Color color;
    };
    std::vector<ShadowLayer> shadows;
};

class ElevationSystem {
public:
    static ElevationSystem& instance();

    const ElevationLevel& elevation(int level) const;
    std::string toCSS(int level) const;

    void registerDefaults();

private:
    ElevationSystem() = default;
    std::vector<ElevationLevel> levels_;
};

// ==================================================================
// Responsive breakpoints
// ==================================================================

struct Breakpoint {
    std::string name;  // "xs", "sm", "md", "lg", "xl", "xxl"
    int minWidth = 0;
    int maxWidth = 0;
};

class BreakpointSystem {
public:
    static BreakpointSystem& instance();

    void define(const std::string& name, int minWidth, int maxWidth = 0);
    const Breakpoint* get(const std::string& name) const;
    const Breakpoint* current(int viewportWidth) const;
    std::vector<Breakpoint> all() const;

    void registerDefaults();

private:
    BreakpointSystem() = default;
    std::vector<Breakpoint> breakpoints_;
};

// ==================================================================
// Theme manager — runtime theme switching
// ==================================================================

class ThemeManager {
public:
    static ThemeManager& instance();

    // Register themes
    void registerTheme(const std::string& name, const Theme& theme);
    void registerTokens(const std::string& name, const DesignTokens& tokens);
    void registerComponentTheme(const std::string& name, const ComponentTheme& ct);

    // Switch
    void setActiveTheme(const std::string& name);
    const Theme& activeTheme() const;
    const DesignTokens& activeTokens() const;
    const ComponentTheme& activeComponentTheme() const;
    const std::string& activeThemeName() const { return activeName_; }

    // OS preference
    enum class ColorScheme { Light, Dark, Auto };
    void setPreferredColorScheme(ColorScheme scheme);
    ColorScheme preferredColorScheme() const { return preferredScheme_; }

    // Contrast
    enum class ContrastPreference { Normal, More, Less, Custom };
    void setContrastPreference(ContrastPreference pref);
    ContrastPreference contrastPreference() const { return contrastPref_; }

    // Change notification
    using ThemeChangeCallback = std::function<void(const std::string& name)>;
    void onThemeChange(ThemeChangeCallback cb) { onThemeChange_ = cb; }

    // List
    std::vector<std::string> availableThemes() const;

private:
    ThemeManager() = default;

    std::unordered_map<std::string, Theme> themes_;
    std::unordered_map<std::string, DesignTokens> tokenSets_;
    std::unordered_map<std::string, ComponentTheme> componentThemes_;

    std::string activeName_ = "light";
    ColorScheme preferredScheme_ = ColorScheme::Auto;
    ContrastPreference contrastPref_ = ContrastPreference::Normal;
    ThemeChangeCallback onThemeChange_;
};

} // namespace NXRender
