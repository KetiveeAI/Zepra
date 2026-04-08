// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "theme/style_sheet.h"
#include <algorithm>

namespace NXRender {

// ==================================================================
// Style
// ==================================================================

Style& Style::set(const std::string& key, const Color& value) {
    properties_[key] = value;
    return *this;
}

Style& Style::set(const std::string& key, float value) {
    properties_[key] = value;
    return *this;
}

Style& Style::set(const std::string& key, const std::string& value) {
    properties_[key] = value;
    return *this;
}

Color Style::getColor(const std::string& key, const Color& fallback) const {
    auto it = properties_.find(key);
    if (it == properties_.end()) return fallback;
    if (auto* c = std::get_if<Color>(&it->second)) return *c;
    return fallback;
}

float Style::getFloat(const std::string& key, float fallback) const {
    auto it = properties_.find(key);
    if (it == properties_.end()) return fallback;
    if (auto* f = std::get_if<float>(&it->second)) return *f;
    return fallback;
}

std::string Style::getString(const std::string& key, const std::string& fallback) const {
    auto it = properties_.find(key);
    if (it == properties_.end()) return fallback;
    if (auto* s = std::get_if<std::string>(&it->second)) return *s;
    return fallback;
}

bool Style::has(const std::string& key) const {
    return properties_.count(key) > 0;
}

void Style::remove(const std::string& key) {
    properties_.erase(key);
}

void Style::clear() {
    properties_.clear();
}

void Style::merge(const Style& other) {
    for (const auto& [key, value] : other.properties_) {
        properties_[key] = value;
    }
}

// ==================================================================
// StyleRule
// ==================================================================

Style StyleRule::resolveForState(StyleState state) const {
    Style resolved = normal;

    switch (state) {
        case StyleState::Hovered:
            resolved.merge(hovered);
            break;
        case StyleState::Pressed:
            resolved.merge(hovered);
            resolved.merge(pressed);
            break;
        case StyleState::Focused:
            resolved.merge(focused);
            break;
        case StyleState::Disabled:
            resolved.merge(disabled);
            break;
        default:
            break;
    }

    return resolved;
}

// ==================================================================
// StyleSheet
// ==================================================================

void StyleSheet::addRule(StyleRule rule) {
    rules_.push_back(std::move(rule));
}

void StyleSheet::removeRules(const std::string& selector) {
    rules_.erase(
        std::remove_if(rules_.begin(), rules_.end(),
                       [&selector](const StyleRule& r) { return r.selector == selector; }),
        rules_.end());
}

void StyleSheet::clear() {
    rules_.clear();
}

Style StyleSheet::resolve(const std::string& className, const std::string& id,
                           StyleState state) const {
    Style resolved;

    // Apply class rules first
    for (const auto& rule : rules_) {
        if (rule.selector == className && rule.id.empty()) {
            resolved.merge(rule.resolveForState(state));
        }
    }

    // Apply ID rules (higher specificity)
    if (!id.empty()) {
        for (const auto& rule : rules_) {
            if (rule.id == id) {
                resolved.merge(rule.resolveForState(state));
            }
        }
    }

    return resolved;
}

StyleSheet StyleSheet::defaultSheet() {
    StyleSheet sheet;

    // Button
    {
        StyleRule rule;
        rule.selector = "Button";
        rule.normal
            .set(StyleKeys::BackgroundColor, Color(0x2196F3))
            .set(StyleKeys::ForegroundColor, Color(255, 255, 255))
            .set(StyleKeys::BorderRadius, 4.0f)
            .set(StyleKeys::PaddingTop, 8.0f)
            .set(StyleKeys::PaddingRight, 16.0f)
            .set(StyleKeys::PaddingBottom, 8.0f)
            .set(StyleKeys::PaddingLeft, 16.0f)
            .set(StyleKeys::FontSize, 14.0f)
            .set(StyleKeys::CursorType, std::string("hand"));
        rule.hovered
            .set(StyleKeys::BackgroundColor, Color(0x1976D2));
        rule.pressed
            .set(StyleKeys::BackgroundColor, Color(0x1565C0));
        rule.focused
            .set(StyleKeys::BorderColor, Color(0x64B5F6))
            .set(StyleKeys::BorderWidth, 2.0f);
        rule.disabled
            .set(StyleKeys::BackgroundColor, Color(0xBDBDBD))
            .set(StyleKeys::ForegroundColor, Color(0x757575))
            .set(StyleKeys::Opacity, 0.6f);
        sheet.addRule(std::move(rule));
    }

    // TextField
    {
        StyleRule rule;
        rule.selector = "TextField";
        rule.normal
            .set(StyleKeys::BackgroundColor, Color(255, 255, 255))
            .set(StyleKeys::ForegroundColor, Color(0x212121))
            .set(StyleKeys::BorderColor, Color(0xBDBDBD))
            .set(StyleKeys::BorderWidth, 1.0f)
            .set(StyleKeys::BorderRadius, 4.0f)
            .set(StyleKeys::PaddingTop, 8.0f)
            .set(StyleKeys::PaddingRight, 12.0f)
            .set(StyleKeys::PaddingBottom, 8.0f)
            .set(StyleKeys::PaddingLeft, 12.0f)
            .set(StyleKeys::FontSize, 14.0f)
            .set(StyleKeys::CursorType, std::string("text"));
        rule.hovered
            .set(StyleKeys::BorderColor, Color(0x757575));
        rule.focused
            .set(StyleKeys::BorderColor, Color(0x2196F3))
            .set(StyleKeys::BorderWidth, 2.0f);
        rule.disabled
            .set(StyleKeys::BackgroundColor, Color(0xF5F5F5))
            .set(StyleKeys::ForegroundColor, Color(0x9E9E9E))
            .set(StyleKeys::Opacity, 0.7f);
        sheet.addRule(std::move(rule));
    }

    // Label
    {
        StyleRule rule;
        rule.selector = "Label";
        rule.normal
            .set(StyleKeys::ForegroundColor, Color(0x212121))
            .set(StyleKeys::FontSize, 14.0f);
        sheet.addRule(std::move(rule));
    }

    // Container
    {
        StyleRule rule;
        rule.selector = "Container";
        rule.normal
            .set(StyleKeys::BackgroundColor, Color::transparent());
        sheet.addRule(std::move(rule));
    }

    // ScrollView
    {
        StyleRule rule;
        rule.selector = "ScrollView";
        rule.normal
            .set(StyleKeys::BackgroundColor, Color::transparent());
        sheet.addRule(std::move(rule));
    }

    // TabView
    {
        StyleRule rule;
        rule.selector = "TabView";
        rule.normal
            .set(StyleKeys::BackgroundColor, Color(0xF5F5F5))
            .set(StyleKeys::BorderColor, Color(0xE0E0E0))
            .set(StyleKeys::BorderWidth, 1.0f);
        sheet.addRule(std::move(rule));
    }

    // Menu
    {
        StyleRule rule;
        rule.selector = "MenuItem";
        rule.normal
            .set(StyleKeys::BackgroundColor, Color::transparent())
            .set(StyleKeys::ForegroundColor, Color(0x212121))
            .set(StyleKeys::PaddingTop, 6.0f)
            .set(StyleKeys::PaddingRight, 24.0f)
            .set(StyleKeys::PaddingBottom, 6.0f)
            .set(StyleKeys::PaddingLeft, 24.0f)
            .set(StyleKeys::FontSize, 13.0f);
        rule.hovered
            .set(StyleKeys::BackgroundColor, Color(0xE3F2FD));
        rule.pressed
            .set(StyleKeys::BackgroundColor, Color(0xBBDEFB));
        rule.disabled
            .set(StyleKeys::ForegroundColor, Color(0xBDBDBD));
        sheet.addRule(std::move(rule));
    }

    // ProgressBar
    {
        StyleRule rule;
        rule.selector = "ProgressBar";
        rule.normal
            .set(StyleKeys::BackgroundColor, Color(0xE0E0E0))
            .set(StyleKeys::ForegroundColor, Color(0x2196F3));
        sheet.addRule(std::move(rule));
    }

    // Dialog
    {
        StyleRule rule;
        rule.selector = "Dialog";
        rule.normal
            .set(StyleKeys::BackgroundColor, Color(255, 255, 255))
            .set(StyleKeys::BorderRadius, 8.0f)
            .set(StyleKeys::ShadowBlur, 20.0f)
            .set(StyleKeys::ShadowColor, Color(0, 0, 0, 80))
            .set(StyleKeys::ShadowOffsetY, 4.0f);
        sheet.addRule(std::move(rule));
    }

    // Tooltip
    {
        StyleRule rule;
        rule.selector = "Tooltip";
        rule.normal
            .set(StyleKeys::BackgroundColor, Color(50, 50, 50, 230))
            .set(StyleKeys::ForegroundColor, Color(255, 255, 255))
            .set(StyleKeys::BorderRadius, 4.0f)
            .set(StyleKeys::FontSize, 12.0f)
            .set(StyleKeys::PaddingTop, 4.0f)
            .set(StyleKeys::PaddingRight, 8.0f)
            .set(StyleKeys::PaddingBottom, 4.0f)
            .set(StyleKeys::PaddingLeft, 8.0f);
        sheet.addRule(std::move(rule));
    }

    // Scrollbar
    {
        StyleRule rule;
        rule.selector = "Scrollbar";
        rule.normal
            .set(StyleKeys::BackgroundColor, Color(0, 0, 0, 30))
            .set(StyleKeys::BorderRadius, 4.0f)
            .set(StyleKeys::Opacity, 0.6f);
        rule.hovered
            .set(StyleKeys::BackgroundColor, Color(0, 0, 0, 60))
            .set(StyleKeys::Opacity, 1.0f);
        sheet.addRule(std::move(rule));
    }

    return sheet;
}

} // namespace NXRender
