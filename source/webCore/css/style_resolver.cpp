// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * style_resolver.cpp - CSS Style Resolution
 * 
 * Implements style resolution per W3C CSS Cascade spec:
 * 1. Collect matching rules from all stylesheets
 * 2. Apply cascade to determine winning values
 * 3. Handle inheritance for inheritable properties
 * 4. Compute final values
 * 
 */

#include "css/css_engine.hpp"
#include "browser/dom.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <unordered_set>

namespace Zepra::WebCore {

// ============================================================================
// Properties that inherit (per CSS spec)
// ============================================================================

// Static list of inheritable properties (from MDN)
static const std::unordered_set<std::string> INHERITED_PROPERTIES = {
    // Typography
    "color", "font", "font-family", "font-size", "font-style", 
    "font-weight", "font-variant", "line-height", "letter-spacing",
    "word-spacing", "text-align", "text-indent", "text-transform",
    "white-space", "direction",
    // Lists
    "list-style", "list-style-image", "list-style-position", "list-style-type",
    // Other
    "visibility", "cursor", "quotes", "orphans", "widows"
};

// ============================================================================
// StyleResolver Implementation
// ============================================================================

StyleResolver::StyleResolver() {
    // Add default user-agent stylesheet
    addUserAgentStyleSheet();
}

StyleResolver::~StyleResolver() = default;

void StyleResolver::addStyleSheet(std::shared_ptr<CSSStyleSheet> sheet, StyleOrigin origin) {
    stylesheets_.push_back({std::move(sheet), origin});
}

void StyleResolver::addUserAgentStyleSheet() {
    // Default browser styles (simplified user-agent stylesheet)
    const std::string uaStyles = R"(
        html, body { display: block; margin: 8px; }
        head, script, style, meta, link, title { display: none; }
        
        h1 { display: block; font-size: 2em; font-weight: bold; margin: 0.67em 0; }
        h2 { display: block; font-size: 1.5em; font-weight: bold; margin: 0.83em 0; }
        h3 { display: block; font-size: 1.17em; font-weight: bold; margin: 1em 0; }
        h4 { display: block; font-weight: bold; margin: 1.33em 0; }
        h5 { display: block; font-size: 0.83em; font-weight: bold; margin: 1.67em 0; }
        h6 { display: block; font-size: 0.67em; font-weight: bold; margin: 2.33em 0; }
        
        p { display: block; margin: 1em 0; }
        div { display: block; }
        span { display: inline; }
        
        a { color: #0066cc; text-decoration: underline; cursor: pointer; }
        a:visited { color: #551a8b; }
        
        strong, b { font-weight: bold; }
        em, i { font-style: italic; }
        u { text-decoration: underline; }
        s, strike { text-decoration: line-through; }
        
        pre, code { font-family: monospace; }
        pre { white-space: pre; display: block; margin: 1em 0; }
        code { white-space: pre-wrap; }
        
        ul, ol { display: block; margin: 1em 0; padding-left: 40px; }
        li { display: list-item; }
        
        table { display: table; border-collapse: separate; border-spacing: 2px; }
        tr { display: table-row; }
        td, th { display: table-cell; padding: 1px; }
        th { font-weight: bold; text-align: center; }
        
        img { display: inline-block; }
        
        input, textarea, select, button { display: inline-block; }
    )";
    
    CSSParser parser;
    auto sheet = parser.parse(uaStyles);
    if (sheet) {
        stylesheets_.push_back({std::move(sheet), StyleOrigin::UserAgent});
    }
}

CSSComputedStyle StyleResolver::computeStyle(DOMElement* element, const CSSComputedStyle* parentStyle) {
    CSSComputedStyle computed;
    
    if (!element) return computed;
    
    // Step 1: Start with inherited values from parent
    if (parentStyle) {
        computed = CSSComputedStyle::inherit(*parentStyle);
    }
    
    // Step 2: Collect all matching rules
    std::vector<CSSStyleSheet*> sheets;
    for (const auto& entry : stylesheets_) {
        sheets.push_back(entry.sheet.get());
    }
    
    auto matchedRules = cascade_.collectMatchingRules(element, sheets, StyleOrigin::Author);
    
    // Step 3: Sort by cascade order
    cascade_.sortByCascade(matchedRules);
    
    // Step 4: Apply matched rules in order (lowest priority first)
    for (const auto& match : matchedRules) {
        if (match.rule && match.rule->style()) {
            applyDeclarations(computed, match.rule->style(), parentStyle);
        }
    }
    
    // Step 5: Apply inline styles (highest priority)
    std::string inlineStyle = element->getAttribute("style");
    if (!inlineStyle.empty()) {
        CSSParser parser;
        auto decl = parser.parseInlineStyle(inlineStyle);
        if (decl) {
            applyDeclarations(computed, decl.get(), parentStyle);
        }
    }
    
    return computed;
}

const CSSComputedStyle* StyleResolver::getComputedStyle(DOMElement* element) {
    if (!element) return nullptr;
    
    // Check cache
    auto it = styleCache_.find(element);
    if (it != styleCache_.end()) {
        return &it->second;
    }
    
    // Compute parent style first (for inheritance)
    const CSSComputedStyle* parentStyle = nullptr;
    if (auto* parent = dynamic_cast<DOMElement*>(element->parentNode())) {
        parentStyle = getComputedStyle(parent);
    }
    
    // Compute and cache
    CSSComputedStyle style = computeStyle(element, parentStyle);
    styleCache_[element] = std::move(style);
    
    return &styleCache_[element];
}

void StyleResolver::invalidateElement(DOMElement* element) {
    styleCache_.erase(element);
}

void StyleResolver::invalidateAll() {
    styleCache_.clear();
}

// ============================================================================
// Property Application
// ============================================================================

void StyleResolver::applyDeclarations(CSSComputedStyle& style, 
                                       const CSSStyleDeclaration* decl,
                                       const CSSComputedStyle* parentStyle) {
    if (!decl) return;
    
    // Apply each property in the declaration
    for (size_t i = 0; i < decl->length(); i++) {
        std::string property = decl->item(i);
        std::string value = decl->getPropertyValue(property);
        
        if (!value.empty()) {
            applyProperty(style, property, value, parentStyle);
        }
    }
}

void StyleResolver::applyProperty(CSSComputedStyle& style,
                                   const std::string& property,
                                   const std::string& value,
                                   const CSSComputedStyle* parentStyle) {
    // Handle 'inherit' keyword
    if (value == "inherit" && parentStyle) {
        // Copy value from parent
        if (property == "color") style.color = parentStyle->color;
        else if (property == "font-family") style.fontFamily = parentStyle->fontFamily;
        else if (property == "font-size") style.fontSize = parentStyle->fontSize;
        else if (property == "font-weight") style.fontWeight = parentStyle->fontWeight;
        else if (property == "font-style") style.fontStyle = parentStyle->fontStyle;
        else if (property == "line-height") style.lineHeight = parentStyle->lineHeight;
        else if (property == "text-align") style.textAlign = parentStyle->textAlign;
        return;
    }
    
    // Handle 'initial' keyword
    if (value == "initial") {
        // Reset to initial value
        if (property == "color") style.color = CSSColor::black();
        else if (property == "font-size") style.fontSize = 16.0f;
        else if (property == "font-weight") style.fontWeight = FontWeight::Normal;
        return;
    }
    
    // Apply specific properties
    if (property == "display") {
        if (value == "none") style.display = DisplayValue::None;
        else if (value == "block") style.display = DisplayValue::Block;
        else if (value == "inline") style.display = DisplayValue::Inline;
        else if (value == "inline-block") style.display = DisplayValue::InlineBlock;
        else if (value == "flex") style.display = DisplayValue::Flex;
        else if (value == "grid") style.display = DisplayValue::Grid;
    }
    else if (property == "color") {
        style.color = parseColor(value);
    }
    else if (property == "background-color") {
        style.backgroundColor = parseColor(value);
    }
    else if (property == "font-size") {
        CSSLength len = parseLength(value);
        if (!len.isAuto()) {
            // Convert to pixels using parent font size
            float parentFontSize = parentStyle ? parentStyle->fontSize : 16.0f;
            style.fontSize = len.toPx(parentFontSize, 16.0f, 1920, 1080, 0);
        }
    }
    else if (property == "font-family") {
        style.fontFamily = value;
    }
    else if (property == "font-weight") {
        if (value == "bold" || value == "700") style.fontWeight = FontWeight::Bold;
        else if (value == "normal" || value == "400") style.fontWeight = FontWeight::Normal;
        else if (value == "lighter") style.fontWeight = FontWeight::Lighter;
        else if (value == "bolder") style.fontWeight = FontWeight::Bolder;
    }
    else if (property == "font-style") {
        if (value == "italic") style.fontStyle = FontStyle::Italic;
        else if (value == "oblique") style.fontStyle = FontStyle::Oblique;
        else style.fontStyle = FontStyle::Normal;
    }
    else if (property == "text-align") {
        if (value == "left") style.textAlign = TextAlign::Left;
        else if (value == "right") style.textAlign = TextAlign::Right;
        else if (value == "center") style.textAlign = TextAlign::Center;
        else if (value == "justify") style.textAlign = TextAlign::Justify;
    }
    else if (property == "width") {
        style.width = parseLength(value);
    }
    else if (property == "height") {
        style.height = parseLength(value);
    }
    else if (property == "margin") {
        CSSLength len = parseLength(value);
        style.marginTop = len;
        style.marginRight = len;
        style.marginBottom = len;
        style.marginLeft = len;
    }
    else if (property == "margin-top") {
        style.marginTop = parseLength(value);
    }
    else if (property == "margin-right") {
        style.marginRight = parseLength(value);
    }
    else if (property == "margin-bottom") {
        style.marginBottom = parseLength(value);
    }
    else if (property == "margin-left") {
        style.marginLeft = parseLength(value);
    }
    else if (property == "padding") {
        CSSLength len = parseLength(value);
        style.paddingTop = len;
        style.paddingRight = len;
        style.paddingBottom = len;
        style.paddingLeft = len;
    }
    else if (property == "padding-top") {
        style.paddingTop = parseLength(value);
    }
    else if (property == "padding-right") {
        style.paddingRight = parseLength(value);
    }
    else if (property == "padding-bottom") {
        style.paddingBottom = parseLength(value);
    }
    else if (property == "padding-left") {
        style.paddingLeft = parseLength(value);
    }
    else if (property == "line-height") {
        if (value.find_first_of("0123456789.") != std::string::npos) {
            float val = std::stof(value);
            if (value.back() == '%') {
                style.lineHeight = val / 100.0f;
            } else if (value.find("px") != std::string::npos) {
                style.lineHeight = val / style.fontSize;
            } else {
                style.lineHeight = val;
            }
        }
    }
    else if (property == "visibility") {
        if (value == "hidden") style.visibility = Visibility::Hidden;
        else if (value == "collapse") style.visibility = Visibility::Collapse;
        else style.visibility = Visibility::Visible;
    }
}

// ============================================================================
// Value Parsing
// ============================================================================

CSSLength StyleResolver::parseLength(const std::string& value) {
    if (value.empty() || value == "auto") {
        return CSSLength::auto_();
    }
    
    float numValue = 0;
    CSSLength::Unit unit = CSSLength::Unit::Px;
    
    try {
        size_t pos = 0;
        numValue = std::stof(value, &pos);
        
        std::string unitStr = value.substr(pos);
        
        if (unitStr == "px" || unitStr.empty()) unit = CSSLength::Unit::Px;
        else if (unitStr == "em") unit = CSSLength::Unit::Em;
        else if (unitStr == "rem") unit = CSSLength::Unit::Rem;
        else if (unitStr == "%") unit = CSSLength::Unit::Percent;
        else if (unitStr == "vw") unit = CSSLength::Unit::Vw;
        else if (unitStr == "vh") unit = CSSLength::Unit::Vh;
        else if (unitStr == "pt") unit = CSSLength::Unit::Pt;
    } catch (...) {
        return CSSLength::auto_();
    }
    
    return {numValue, unit};
}

CSSColor StyleResolver::parseColor(const std::string& value) {
    return CSSColor::parse(value);
}

} // namespace Zepra::WebCore
