// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "design_tokens.h"
#include <sstream>
#include <algorithm>

namespace NXRender {

// ==================================================================
// DesignTokens
// ==================================================================

void DesignTokens::define(const std::string& name, TokenType type, const std::string& value,
                            const std::string& description) {
    tokens_[name] = {name, type, value, "", description};
}

void DesignTokens::alias(const std::string& name, const std::string& ref) {
    tokens_[name] = {name, TokenType::Color, "", ref, ""};
}

std::string DesignTokens::resolve(const std::string& name) const {
    return resolveChain(name, 0);
}

std::string DesignTokens::resolveChain(const std::string& name, int depth) const {
    if (depth > 8) return "";
    auto it = tokens_.find(name);
    if (it == tokens_.end()) return "";
    if (!it->second.reference.empty()) {
        return resolveChain(it->second.reference, depth + 1);
    }
    return it->second.value;
}

const DesignToken* DesignTokens::get(const std::string& name) const {
    auto it = tokens_.find(name);
    return (it != tokens_.end()) ? &it->second : nullptr;
}

bool DesignTokens::has(const std::string& name) const {
    return tokens_.count(name) > 0;
}

std::vector<std::string> DesignTokens::tokenNames() const {
    std::vector<std::string> names;
    for (const auto& [k, _] : tokens_) names.push_back(k);
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::string> DesignTokens::tokensOfType(TokenType type) const {
    std::vector<std::string> names;
    for (const auto& [k, v] : tokens_) {
        if (v.type == type) names.push_back(k);
    }
    std::sort(names.begin(), names.end());
    return names;
}

void DesignTokens::merge(const DesignTokens& other) {
    for (const auto& [k, v] : other.tokens_) {
        tokens_[k] = v;
    }
}

void DesignTokens::exportToCSS(std::string& output) const {
    output += ":root {\n";
    auto names = tokenNames();
    for (const auto& name : names) {
        std::string cssName = "--" + name;
        std::replace(cssName.begin(), cssName.end(), '.', '-');
        output += "  " + cssName + ": " + resolve(name) + ";\n";
    }
    output += "}\n";
}

DesignTokens DesignTokens::materialDesign() {
    DesignTokens t;

    // Color scale (Material Blue)
    t.define("color.primary.50", TokenType::Color, "#E3F2FD");
    t.define("color.primary.100", TokenType::Color, "#BBDEFB");
    t.define("color.primary.200", TokenType::Color, "#90CAF9");
    t.define("color.primary.300", TokenType::Color, "#64B5F6");
    t.define("color.primary.400", TokenType::Color, "#42A5F5");
    t.define("color.primary.500", TokenType::Color, "#2196F3");
    t.define("color.primary.600", TokenType::Color, "#1E88E5");
    t.define("color.primary.700", TokenType::Color, "#1976D2");
    t.define("color.primary.800", TokenType::Color, "#1565C0");
    t.define("color.primary.900", TokenType::Color, "#0D47A1");

    // Error
    t.define("color.error.500", TokenType::Color, "#F44336");
    t.define("color.success.500", TokenType::Color, "#4CAF50");
    t.define("color.warning.500", TokenType::Color, "#FFC107");

    // Neutral
    t.define("color.neutral.0", TokenType::Color, "#FFFFFF");
    t.define("color.neutral.50", TokenType::Color, "#FAFAFA");
    t.define("color.neutral.100", TokenType::Color, "#F5F5F5");
    t.define("color.neutral.200", TokenType::Color, "#EEEEEE");
    t.define("color.neutral.300", TokenType::Color, "#E0E0E0");
    t.define("color.neutral.400", TokenType::Color, "#BDBDBD");
    t.define("color.neutral.500", TokenType::Color, "#9E9E9E");
    t.define("color.neutral.600", TokenType::Color, "#757575");
    t.define("color.neutral.700", TokenType::Color, "#616161");
    t.define("color.neutral.800", TokenType::Color, "#424242");
    t.define("color.neutral.900", TokenType::Color, "#212121");
    t.define("color.neutral.1000", TokenType::Color, "#000000");

    // Semantic aliases
    t.alias("color.background", "color.neutral.0");
    t.alias("color.surface", "color.neutral.0");
    t.alias("color.on-background", "color.neutral.900");
    t.alias("color.on-surface", "color.neutral.900");
    t.alias("color.border", "color.neutral.300");
    t.alias("color.text.primary", "color.neutral.900");
    t.alias("color.text.secondary", "color.neutral.600");
    t.alias("color.text.disabled", "color.neutral.400");

    // Spacing (4px base grid)
    t.define("spacing.0", TokenType::Spacing, "0px");
    t.define("spacing.1", TokenType::Spacing, "4px");
    t.define("spacing.2", TokenType::Spacing, "8px");
    t.define("spacing.3", TokenType::Spacing, "12px");
    t.define("spacing.4", TokenType::Spacing, "16px");
    t.define("spacing.5", TokenType::Spacing, "20px");
    t.define("spacing.6", TokenType::Spacing, "24px");
    t.define("spacing.8", TokenType::Spacing, "32px");
    t.define("spacing.10", TokenType::Spacing, "40px");
    t.define("spacing.12", TokenType::Spacing, "48px");
    t.define("spacing.16", TokenType::Spacing, "64px");

    // Radii
    t.define("radius.none", TokenType::Radius, "0px");
    t.define("radius.sm", TokenType::Radius, "4px");
    t.define("radius.md", TokenType::Radius, "8px");
    t.define("radius.lg", TokenType::Radius, "12px");
    t.define("radius.xl", TokenType::Radius, "16px");
    t.define("radius.full", TokenType::Radius, "9999px");

    // Typography
    t.define("font.family.sans", TokenType::FontFamily, "Inter, system-ui, -apple-system, sans-serif");
    t.define("font.family.mono", TokenType::FontFamily, "JetBrains Mono, Menlo, monospace");
    t.define("font.size.xs", TokenType::Dimension, "12px");
    t.define("font.size.sm", TokenType::Dimension, "14px");
    t.define("font.size.md", TokenType::Dimension, "16px");
    t.define("font.size.lg", TokenType::Dimension, "18px");
    t.define("font.size.xl", TokenType::Dimension, "20px");
    t.define("font.size.2xl", TokenType::Dimension, "24px");
    t.define("font.size.3xl", TokenType::Dimension, "30px");
    t.define("font.size.4xl", TokenType::Dimension, "36px");
    t.define("font.weight.normal", TokenType::FontWeight, "400");
    t.define("font.weight.medium", TokenType::FontWeight, "500");
    t.define("font.weight.bold", TokenType::FontWeight, "700");
    t.define("font.lineHeight.tight", TokenType::Number, "1.25");
    t.define("font.lineHeight.normal", TokenType::Number, "1.5");
    t.define("font.lineHeight.relaxed", TokenType::Number, "1.75");

    // Duration
    t.define("duration.fast", TokenType::Duration, "150ms");
    t.define("duration.normal", TokenType::Duration, "300ms");
    t.define("duration.slow", TokenType::Duration, "500ms");

    // Z-index scale
    t.define("z.dropdown", TokenType::ZIndex, "1000");
    t.define("z.sticky", TokenType::ZIndex, "1020");
    t.define("z.fixed", TokenType::ZIndex, "1030");
    t.define("z.modal-backdrop", TokenType::ZIndex, "1040");
    t.define("z.modal", TokenType::ZIndex, "1050");
    t.define("z.popover", TokenType::ZIndex, "1060");
    t.define("z.tooltip", TokenType::ZIndex, "1070");

    return t;
}

DesignTokens DesignTokens::zepraDefaults() {
    auto t = materialDesign();

    // Zepra-specific overrides
    t.define("color.primary.500", TokenType::Color, "#6366F1"); // Indigo
    t.define("color.primary.600", TokenType::Color, "#4F46E5");
    t.define("color.primary.700", TokenType::Color, "#4338CA");
    t.define("color.accent.500", TokenType::Color, "#10B981"); // Emerald
    t.define("font.family.sans", TokenType::FontFamily, "Outfit, system-ui, sans-serif");

    return t;
}

// ==================================================================
// ComponentStyle
// ==================================================================

std::string ComponentStyle::get(const std::string& prop, const std::string& fallback) const {
    auto it = properties.find(prop);
    return (it != properties.end()) ? it->second : fallback;
}

void ComponentStyle::set(const std::string& prop, const std::string& value) {
    properties[prop] = value;
}

const ComponentStyle& ComponentVariant::styleForState(ComponentState state) const {
    auto it = states.find(state);
    return (it != states.end()) ? it->second : baseStyle;
}

// ==================================================================
// ComponentTheme
// ==================================================================

void ComponentTheme::defineComponent(const std::string& component,
                                       const ComponentVariant& variant) {
    components_[component][variant.name] = variant;
}

const ComponentVariant* ComponentTheme::variant(const std::string& component,
                                                   const std::string& variant) const {
    auto cit = components_.find(component);
    if (cit == components_.end()) return nullptr;
    auto vit = cit->second.find(variant);
    return (vit != cit->second.end()) ? &vit->second : nullptr;
}

const ComponentStyle* ComponentTheme::style(const std::string& component,
                                               const std::string& variant,
                                               ComponentState state) const {
    auto* v = this->variant(component, variant);
    return v ? &v->styleForState(state) : nullptr;
}

std::vector<std::string> ComponentTheme::components() const {
    std::vector<std::string> result;
    for (const auto& [k, _] : components_) result.push_back(k);
    return result;
}

std::vector<std::string> ComponentTheme::variants(const std::string& component) const {
    std::vector<std::string> result;
    auto it = components_.find(component);
    if (it != components_.end()) {
        for (const auto& [k, _] : it->second) result.push_back(k);
    }
    return result;
}

ComponentTheme ComponentTheme::defaultTheme() {
    ComponentTheme ct;

    // Button
    auto mkButton = [](const std::string& name, const std::string& bg,
                         const std::string& fg, const std::string& hoverBg) {
        ComponentVariant v;
        v.name = name;
        v.baseStyle.set("background-color", bg);
        v.baseStyle.set("color", fg);
        v.baseStyle.set("border-radius", "8px");
        v.baseStyle.set("padding", "8px 16px");
        v.baseStyle.set("font-weight", "500");
        v.baseStyle.set("cursor", "pointer");
        v.baseStyle.set("transition", "all 150ms ease");

        ComponentStyle hover;
        hover.set("background-color", hoverBg);
        v.states[ComponentState::Hover] = hover;

        ComponentStyle disabled;
        disabled.set("background-color", "#E0E0E0");
        disabled.set("color", "#9E9E9E");
        disabled.set("cursor", "not-allowed");
        v.states[ComponentState::Disabled] = disabled;

        ComponentStyle focus;
        focus.set("outline", "2px solid #2196F3");
        focus.set("outline-offset", "2px");
        v.states[ComponentState::Focus] = focus;

        return v;
    };

    ct.defineComponent("button", mkButton("primary", "#2196F3", "#FFFFFF", "#1976D2"));
    ct.defineComponent("button", mkButton("secondary", "#E0E0E0", "#212121", "#BDBDBD"));
    ct.defineComponent("button", mkButton("danger", "#F44336", "#FFFFFF", "#D32F2F"));

    // Input
    ComponentVariant input;
    input.name = "default";
    input.baseStyle.set("background-color", "#FFFFFF");
    input.baseStyle.set("border", "1px solid #E0E0E0");
    input.baseStyle.set("border-radius", "4px");
    input.baseStyle.set("padding", "8px 12px");
    input.baseStyle.set("font-size", "14px");
    ComponentStyle inputFocus;
    inputFocus.set("border-color", "#2196F3");
    inputFocus.set("box-shadow", "0 0 0 3px rgba(33, 150, 243, 0.1)");
    input.states[ComponentState::Focus] = inputFocus;
    ComponentStyle inputError;
    inputError.set("border-color", "#F44336");
    input.states[ComponentState::Error] = inputError;
    ct.defineComponent("input", input);

    // Card
    ComponentVariant card;
    card.name = "default";
    card.baseStyle.set("background-color", "#FFFFFF");
    card.baseStyle.set("border-radius", "12px");
    card.baseStyle.set("padding", "16px");
    card.baseStyle.set("box-shadow", "0 2px 8px rgba(0,0,0,0.08)");
    ComponentStyle cardHover;
    cardHover.set("box-shadow", "0 4px 16px rgba(0,0,0,0.12)");
    card.states[ComponentState::Hover] = cardHover;
    ct.defineComponent("card", card);

    return ct;
}

// ==================================================================
// AnimationPresets
// ==================================================================

AnimationPresets& AnimationPresets::instance() {
    static AnimationPresets inst;
    return inst;
}

void AnimationPresets::define(const std::string& name, const AnimationPreset& preset) {
    presets_[name] = preset;
}

const AnimationPreset* AnimationPresets::get(const std::string& name) const {
    auto it = presets_.find(name);
    return (it != presets_.end()) ? &it->second : nullptr;
}

std::vector<std::string> AnimationPresets::names() const {
    std::vector<std::string> result;
    for (const auto& [k, _] : presets_) result.push_back(k);
    return result;
}

void AnimationPresets::registerDefaults() {
    // Fade in
    define("fade-in", {"fade-in", 300, "ease", 0,
        {{0, {{"opacity", "0"}}}, {1, {{"opacity", "1"}}}}});

    // Fade out
    define("fade-out", {"fade-out", 300, "ease", 0,
        {{0, {{"opacity", "1"}}}, {1, {{"opacity", "0"}}}}});

    // Slide up
    define("slide-up", {"slide-up", 300, "ease-out", 0,
        {{0, {{"transform", "translateY(20px)"}, {"opacity", "0"}}},
         {1, {{"transform", "translateY(0)"}, {"opacity", "1"}}}}});

    // Slide down
    define("slide-down", {"slide-down", 300, "ease-out", 0,
        {{0, {{"transform", "translateY(-20px)"}, {"opacity", "0"}}},
         {1, {{"transform", "translateY(0)"}, {"opacity", "1"}}}}});

    // Scale in
    define("scale-in", {"scale-in", 200, "cubic-bezier(0.16, 1, 0.3, 1)", 0,
        {{0, {{"transform", "scale(0.95)"}, {"opacity", "0"}}},
         {1, {{"transform", "scale(1)"}, {"opacity", "1"}}}}});

    // Bounce
    define("bounce", {"bounce", 500, "cubic-bezier(0.34, 1.56, 0.64, 1)", 0,
        {{0, {{"transform", "scale(1)"}}},
         {0.5f, {{"transform", "scale(1.1)"}}},
         {1, {{"transform", "scale(1)"}}}}});

    // Shake
    define("shake", {"shake", 500, "ease-in-out", 0,
        {{0, {{"transform", "translateX(0)"}}},
         {0.1f, {{"transform", "translateX(-5px)"}}},
         {0.3f, {{"transform", "translateX(5px)"}}},
         {0.5f, {{"transform", "translateX(-5px)"}}},
         {0.7f, {{"transform", "translateX(5px)"}}},
         {0.9f, {{"transform", "translateX(-5px)"}}},
         {1, {{"transform", "translateX(0)"}}}}});

    // Spin
    define("spin", {"spin", 1000, "linear", 0,
        {{0, {{"transform", "rotate(0deg)"}}},
         {1, {{"transform", "rotate(360deg)"}}}}});

    // Pulse
    define("pulse", {"pulse", 2000, "ease-in-out", 0,
        {{0, {{"opacity", "1"}}},
         {0.5f, {{"opacity", "0.5"}}},
         {1, {{"opacity", "1"}}}}});
}

// ==================================================================
// ElevationSystem
// ==================================================================

ElevationSystem& ElevationSystem::instance() {
    static ElevationSystem inst;
    return inst;
}

const ElevationLevel& ElevationSystem::elevation(int level) const {
    if (level >= 0 && level < static_cast<int>(levels_.size())) {
        return levels_[level];
    }
    static ElevationLevel none;
    return none;
}

std::string ElevationSystem::toCSS(int level) const {
    auto& el = elevation(level);
    if (el.shadows.empty()) return "none";

    std::ostringstream ss;
    for (size_t i = 0; i < el.shadows.size(); i++) {
        if (i > 0) ss << ", ";
        auto& s = el.shadows[i];
        ss << s.offsetX << "px " << s.offsetY << "px " << s.blur << "px " << s.spread << "px "
           << "rgba(" << static_cast<int>(s.color.r) << ", " << static_cast<int>(s.color.g)
           << ", " << static_cast<int>(s.color.b) << ", " << (s.color.a / 255.0f) << ")";
    }
    return ss.str();
}

void ElevationSystem::registerDefaults() {
    levels_.resize(25);
    Color shadowColor = Color(0, 0, 0, 50);

    for (int i = 0; i <= 24; i++) {
        levels_[i].level = i;
        if (i == 0) continue;

        float factor = static_cast<float>(i);
        levels_[i].shadows.push_back({
            0, factor * 0.5f, factor * 1.5f, 0, shadowColor
        });
        if (i >= 2) {
            levels_[i].shadows.push_back({
                0, factor * 0.25f, factor * 0.75f, 0, Color(0, 0, 0, 30)
            });
        }
    }
}

// ==================================================================
// BreakpointSystem
// ==================================================================

BreakpointSystem& BreakpointSystem::instance() {
    static BreakpointSystem inst;
    return inst;
}

void BreakpointSystem::define(const std::string& name, int minWidth, int maxWidth) {
    breakpoints_.push_back({name, minWidth, maxWidth});
    std::sort(breakpoints_.begin(), breakpoints_.end(),
              [](const Breakpoint& a, const Breakpoint& b) {
                  return a.minWidth < b.minWidth;
              });
}

const Breakpoint* BreakpointSystem::get(const std::string& name) const {
    for (const auto& bp : breakpoints_) {
        if (bp.name == name) return &bp;
    }
    return nullptr;
}

const Breakpoint* BreakpointSystem::current(int viewportWidth) const {
    const Breakpoint* result = nullptr;
    for (const auto& bp : breakpoints_) {
        if (viewportWidth >= bp.minWidth) result = &bp;
    }
    return result;
}

std::vector<Breakpoint> BreakpointSystem::all() const {
    return breakpoints_;
}

void BreakpointSystem::registerDefaults() {
    define("xs", 0, 575);
    define("sm", 576, 767);
    define("md", 768, 991);
    define("lg", 992, 1199);
    define("xl", 1200, 1599);
    define("xxl", 1600, 0);
}

// ==================================================================
// ThemeManager
// ==================================================================

ThemeManager& ThemeManager::instance() {
    static ThemeManager inst;
    return inst;
}

void ThemeManager::registerTheme(const std::string& name, const Theme& theme) {
    themes_[name] = theme;
}

void ThemeManager::registerTokens(const std::string& name, const DesignTokens& tokens) {
    tokenSets_[name] = tokens;
}

void ThemeManager::registerComponentTheme(const std::string& name, const ComponentTheme& ct) {
    componentThemes_[name] = ct;
}

void ThemeManager::setActiveTheme(const std::string& name) {
    if (themes_.count(name)) {
        activeName_ = name;
        setTheme(themes_[name]);
        if (onThemeChange_) onThemeChange_(name);
    }
}

const Theme& ThemeManager::activeTheme() const {
    auto it = themes_.find(activeName_);
    if (it != themes_.end()) return it->second;
    static Theme fallback;
    return fallback;
}

const DesignTokens& ThemeManager::activeTokens() const {
    auto it = tokenSets_.find(activeName_);
    if (it != tokenSets_.end()) return it->second;
    static DesignTokens fallback;
    return fallback;
}

const ComponentTheme& ThemeManager::activeComponentTheme() const {
    auto it = componentThemes_.find(activeName_);
    if (it != componentThemes_.end()) return it->second;
    static ComponentTheme fallback;
    return fallback;
}

void ThemeManager::setPreferredColorScheme(ColorScheme scheme) {
    preferredScheme_ = scheme;
    if (scheme == ColorScheme::Dark) {
        setActiveTheme("dark");
    } else if (scheme == ColorScheme::Light) {
        setActiveTheme("light");
    }
}

void ThemeManager::setContrastPreference(ContrastPreference pref) {
    contrastPref_ = pref;
    if (pref == ContrastPreference::More) {
        setActiveTheme("high-contrast");
    }
}

std::vector<std::string> ThemeManager::availableThemes() const {
    std::vector<std::string> names;
    for (const auto& [k, _] : themes_) names.push_back(k);
    return names;
}

} // namespace NXRender
