// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file style_sheet.h
 * @brief Widget styling layer with cascading and state-dependent styles.
 */

#pragma once

#include "../nxgfx/color.h"
#include "../nxgfx/primitives.h"
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <functional>

namespace NXRender {

/**
 * @brief Widget state for style matching.
 */
enum class StyleState : uint8_t {
    Normal   = 0,
    Hovered  = 1,
    Pressed  = 2,
    Focused  = 3,
    Disabled = 4,
    Active   = 5,
    Selected = 6
};

/**
 * @brief Style property value (color, float, or string).
 */
using StyleValue = std::variant<Color, float, std::string>;

/**
 * @brief A set of style properties for a specific widget state.
 */
class Style {
public:
    Style() = default;

    // Setters (fluent API)
    Style& set(const std::string& key, const Color& value);
    Style& set(const std::string& key, float value);
    Style& set(const std::string& key, const std::string& value);

    // Getters — return default if not found
    Color getColor(const std::string& key, const Color& fallback = Color()) const;
    float getFloat(const std::string& key, float fallback = 0.0f) const;
    std::string getString(const std::string& key, const std::string& fallback = "") const;

    bool has(const std::string& key) const;
    void remove(const std::string& key);
    void clear();

    // Merge another style into this one (other takes precedence)
    void merge(const Style& other);

    size_t propertyCount() const { return properties_.size(); }

private:
    std::unordered_map<std::string, StyleValue> properties_;
};

/**
 * @brief Style rule: matches a widget class/ID and applies styles per state.
 */
struct StyleRule {
    std::string selector;  // Widget class name (e.g. "Button", "TextField")
    std::string id;        // Widget ID (optional, for specific targeting)

    Style normal;
    Style hovered;
    Style pressed;
    Style focused;
    Style disabled;

    /**
     * @brief Get the style for a given state, falling back to normal.
     */
    Style resolveForState(StyleState state) const;
};

/**
 * @brief Widget style sheet — holds a collection of style rules.
 *
 * Styles cascade in priority: widget inline style > ID rule > class rule > theme default.
 */
class StyleSheet {
public:
    StyleSheet() = default;

    /**
     * @brief Add a style rule.
     */
    void addRule(StyleRule rule);

    /**
     * @brief Remove rules matching a selector.
     */
    void removeRules(const std::string& selector);

    /**
     * @brief Clear all rules.
     */
    void clear();

    /**
     * @brief Resolve the effective style for a widget, given its class name,
     * optional ID, and current state.
     */
    Style resolve(const std::string& className, const std::string& id,
                  StyleState state) const;

    /**
     * @brief Number of rules in the sheet.
     */
    size_t ruleCount() const { return rules_.size(); }

    /**
     * @brief Create a default style sheet with standard widget styles.
     */
    static StyleSheet defaultSheet();

private:
    std::vector<StyleRule> rules_;
};

// ======================================================================
// Standard style property keys
// ======================================================================

namespace StyleKeys {
    constexpr const char* BackgroundColor = "backgroundColor";
    constexpr const char* ForegroundColor = "foregroundColor";
    constexpr const char* BorderColor = "borderColor";
    constexpr const char* BorderWidth = "borderWidth";
    constexpr const char* BorderRadius = "borderRadius";
    constexpr const char* FontSize = "fontSize";
    constexpr const char* FontFamily = "fontFamily";
    constexpr const char* FontWeight = "fontWeight";
    constexpr const char* PaddingTop = "paddingTop";
    constexpr const char* PaddingRight = "paddingRight";
    constexpr const char* PaddingBottom = "paddingBottom";
    constexpr const char* PaddingLeft = "paddingLeft";
    constexpr const char* MarginTop = "marginTop";
    constexpr const char* MarginRight = "marginRight";
    constexpr const char* MarginBottom = "marginBottom";
    constexpr const char* MarginLeft = "marginLeft";
    constexpr const char* Opacity = "opacity";
    constexpr const char* Shadow = "shadow";
    constexpr const char* ShadowBlur = "shadowBlur";
    constexpr const char* ShadowColor = "shadowColor";
    constexpr const char* ShadowOffsetX = "shadowOffsetX";
    constexpr const char* ShadowOffsetY = "shadowOffsetY";
    constexpr const char* MinWidth = "minWidth";
    constexpr const char* MinHeight = "minHeight";
    constexpr const char* MaxWidth = "maxWidth";
    constexpr const char* MaxHeight = "maxHeight";
    constexpr const char* CursorType = "cursorType";
    constexpr const char* TextAlign = "textAlign";
    constexpr const char* TextDecoration = "textDecoration";
    constexpr const char* LineHeight = "lineHeight";
    constexpr const char* LetterSpacing = "letterSpacing";
} // namespace StyleKeys

} // namespace NXRender
