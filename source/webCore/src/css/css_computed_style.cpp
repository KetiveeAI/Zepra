/**
 * @file css_computed_style.cpp
 * @brief ComputedStyle and CSS value resolution
 */

#include "webcore/css/css_computed_style.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_map>

namespace Zepra::WebCore {

// =============================================================================
// CSSLength
// =============================================================================

CSSLength CSSLength::parse(const std::string& str) {
    CSSLength result;
    
    if (str.empty() || str == "auto") {
        result.unit = Unit::Auto;
        return result;
    }
    
    // Parse number
    std::istringstream iss(str);
    iss >> result.value;
    
    // Parse unit
    std::string unitStr;
    iss >> unitStr;
    
    // If no unit parsed, extract from string
    if (unitStr.empty()) {
        size_t numEnd = 0;
        for (size_t i = 0; i < str.size(); ++i) {
            if (std::isdigit(str[i]) || str[i] == '.' || str[i] == '-') {
                numEnd = i + 1;
            } else {
                break;
            }
        }
        if (numEnd < str.size()) {
            unitStr = str.substr(numEnd);
        }
    }
    
    // Convert to lowercase for comparison
    std::transform(unitStr.begin(), unitStr.end(), unitStr.begin(), ::tolower);
    
    if (unitStr.empty() || unitStr == "px") result.unit = Unit::Px;
    else if (unitStr == "em") result.unit = Unit::Em;
    else if (unitStr == "rem") result.unit = Unit::Rem;
    else if (unitStr == "ex") result.unit = Unit::Ex;
    else if (unitStr == "ch") result.unit = Unit::Ch;
    else if (unitStr == "vw") result.unit = Unit::Vw;
    else if (unitStr == "vh") result.unit = Unit::Vh;
    else if (unitStr == "vmin") result.unit = Unit::Vmin;
    else if (unitStr == "vmax") result.unit = Unit::Vmax;
    else if (unitStr == "%") result.unit = Unit::Percent;
    else if (unitStr == "fr") result.unit = Unit::Fr;
    else if (unitStr == "cm") result.unit = Unit::Cm;
    else if (unitStr == "mm") result.unit = Unit::Mm;
    else if (unitStr == "in") result.unit = Unit::In;
    else if (unitStr == "pt") result.unit = Unit::Pt;
    else if (unitStr == "pc") result.unit = Unit::Pc;
    
    return result;
}

float CSSLength::toPx(float fontSize, float rootFontSize, float viewportWidth, 
                       float viewportHeight, float containerSize) const {
    switch (unit) {
        case Unit::Auto:
            return 0;  // Handled separately
        case Unit::Px:
            return value;
        case Unit::Em:
            return value * fontSize;
        case Unit::Rem:
            return value * rootFontSize;
        case Unit::Ex:
            return value * fontSize * 0.5f;  // Approximate
        case Unit::Ch:
            return value * fontSize * 0.5f;  // Approximate
        case Unit::Vw:
            return value * viewportWidth / 100.0f;
        case Unit::Vh:
            return value * viewportHeight / 100.0f;
        case Unit::Vmin:
            return value * std::min(viewportWidth, viewportHeight) / 100.0f;
        case Unit::Vmax:
            return value * std::max(viewportWidth, viewportHeight) / 100.0f;
        case Unit::Percent:
            return value * containerSize / 100.0f;
        case Unit::Fr:
            return value;  // Fractional, handled in grid layout
        case Unit::Cm:
            return value * 96.0f / 2.54f;
        case Unit::Mm:
            return value * 96.0f / 25.4f;
        case Unit::In:
            return value * 96.0f;
        case Unit::Pt:
            return value * 96.0f / 72.0f;
        case Unit::Pc:
            return value * 96.0f / 6.0f;
    }
    return value;
}

// =============================================================================
// CSSColor
// =============================================================================

CSSColor CSSColor::parse(const std::string& str) {
    if (str.empty()) return transparent();
    
    // Hex color
    if (str[0] == '#') {
        return fromHex(str);
    }
    
    // rgb() / rgba()
    if (str.find("rgb") == 0) {
        CSSColor c;
        size_t start = str.find('(');
        size_t end = str.rfind(')');
        if (start != std::string::npos && end != std::string::npos) {
            std::string values = str.substr(start + 1, end - start - 1);
            std::istringstream iss(values);
            int r, g, b;
            char sep;
            iss >> r >> sep >> g >> sep >> b;
            c.r = static_cast<uint8_t>(std::clamp(r, 0, 255));
            c.g = static_cast<uint8_t>(std::clamp(g, 0, 255));
            c.b = static_cast<uint8_t>(std::clamp(b, 0, 255));
            
            // Check for alpha
            float alpha = 1.0f;
            if (iss >> sep >> alpha) {
                c.a = static_cast<uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * 255);
            }
        }
        return c;
    }
    
    // Named colors
    static const std::unordered_map<std::string, CSSColor> namedColors = {
        {"black", {0, 0, 0, 255}},
        {"white", {255, 255, 255, 255}},
        {"red", {255, 0, 0, 255}},
        {"green", {0, 128, 0, 255}},
        {"blue", {0, 0, 255, 255}},
        {"yellow", {255, 255, 0, 255}},
        {"cyan", {0, 255, 255, 255}},
        {"magenta", {255, 0, 255, 255}},
        {"gray", {128, 128, 128, 255}},
        {"grey", {128, 128, 128, 255}},
        {"silver", {192, 192, 192, 255}},
        {"maroon", {128, 0, 0, 255}},
        {"olive", {128, 128, 0, 255}},
        {"lime", {0, 255, 0, 255}},
        {"aqua", {0, 255, 255, 255}},
        {"teal", {0, 128, 128, 255}},
        {"navy", {0, 0, 128, 255}},
        {"fuchsia", {255, 0, 255, 255}},
        {"purple", {128, 0, 128, 255}},
        {"orange", {255, 165, 0, 255}},
        {"pink", {255, 192, 203, 255}},
        {"transparent", {0, 0, 0, 0}},
        {"currentcolor", {0, 0, 0, 255}},  // Placeholder
    };
    
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    auto it = namedColors.find(lower);
    if (it != namedColors.end()) {
        return it->second;
    }
    
    return transparent();
}

CSSColor CSSColor::fromHex(const std::string& hex) {
    CSSColor c;
    
    std::string h = hex;
    if (h[0] == '#') h = h.substr(1);
    
    if (h.size() == 3) {
        // #RGB -> #RRGGBB
        c.r = std::stoi(std::string(2, h[0]), nullptr, 16);
        c.g = std::stoi(std::string(2, h[1]), nullptr, 16);
        c.b = std::stoi(std::string(2, h[2]), nullptr, 16);
        c.a = 255;
    } else if (h.size() == 4) {
        // #RGBA
        c.r = std::stoi(std::string(2, h[0]), nullptr, 16);
        c.g = std::stoi(std::string(2, h[1]), nullptr, 16);
        c.b = std::stoi(std::string(2, h[2]), nullptr, 16);
        c.a = std::stoi(std::string(2, h[3]), nullptr, 16);
    } else if (h.size() == 6) {
        // #RRGGBB
        c.r = std::stoi(h.substr(0, 2), nullptr, 16);
        c.g = std::stoi(h.substr(2, 2), nullptr, 16);
        c.b = std::stoi(h.substr(4, 2), nullptr, 16);
        c.a = 255;
    } else if (h.size() == 8) {
        // #RRGGBBAA
        c.r = std::stoi(h.substr(0, 2), nullptr, 16);
        c.g = std::stoi(h.substr(2, 2), nullptr, 16);
        c.b = std::stoi(h.substr(4, 2), nullptr, 16);
        c.a = std::stoi(h.substr(6, 2), nullptr, 16);
    }
    
    return c;
}

CSSColor CSSColor::fromRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return {r, g, b, a};
}

// =============================================================================
// ComputedStyle
// =============================================================================

CSSComputedStyle::CSSComputedStyle() {
    // Initialize with default values
    width = CSSLength::auto_();
    height = CSSLength::auto_();
    minWidth = CSSLength::px(0);
    minHeight = CSSLength::px(0);
    maxWidth = CSSLength{0, CSSLength::Unit::Auto};  // none
    maxHeight = CSSLength{0, CSSLength::Unit::Auto};
    
    marginTop = CSSLength::px(0);
    marginRight = CSSLength::px(0);
    marginBottom = CSSLength::px(0);
    marginLeft = CSSLength::px(0);
    
    paddingTop = CSSLength::px(0);
    paddingRight = CSSLength::px(0);
    paddingBottom = CSSLength::px(0);
    paddingLeft = CSSLength::px(0);
    
    color = CSSColor::black();
    backgroundColor = CSSColor::transparent();
}

CSSComputedStyle CSSComputedStyle::inherit(const CSSComputedStyle& parent) {
    CSSComputedStyle style;
    
    // Inherited properties
    style.color = parent.color;
    style.fontFamily = parent.fontFamily;
    style.fontSize = parent.fontSize;
    style.fontWeight = parent.fontWeight;
    style.fontStyle = parent.fontStyle;
    style.lineHeight = parent.lineHeight;
    style.textAlign = parent.textAlign;
    style.textDecoration = parent.textDecoration;
    style.textTransform = parent.textTransform;
    style.letterSpacing = parent.letterSpacing;
    style.wordSpacing = parent.wordSpacing;
    style.whiteSpace = parent.whiteSpace;
    style.visibility = parent.visibility;
    style.cursor = parent.cursor;
    
    return style;
}

std::string CSSComputedStyle::getPropertyValue(const std::string& property) const {
    // Map property names to values
    if (property == "display") {
        switch (display) {
            case DisplayValue::None: return "none";
            case DisplayValue::Block: return "block";
            case DisplayValue::Inline: return "inline";
            case DisplayValue::InlineBlock: return "inline-block";
            case DisplayValue::Flex: return "flex";
            case DisplayValue::Grid: return "grid";
            default: return "inline";
        }
    }
    if (property == "position") {
        switch (position) {
            case PositionValue::Static: return "static";
            case PositionValue::Relative: return "relative";
            case PositionValue::Absolute: return "absolute";
            case PositionValue::Fixed: return "fixed";
            case PositionValue::Sticky: return "sticky";
        }
    }
    if (property == "font-size") {
        return std::to_string(fontSize) + "px";
    }
    if (property == "opacity") {
        return std::to_string(opacity);
    }
    
    // ... more properties
    return "";
}

bool CSSComputedStyle::inherits(const std::string& property) {
    static const std::unordered_map<std::string, bool> inheritMap = {
        {"color", true},
        {"font-family", true},
        {"font-size", true},
        {"font-weight", true},
        {"font-style", true},
        {"line-height", true},
        {"text-align", true},
        {"text-decoration", false},
        {"text-transform", true},
        {"letter-spacing", true},
        {"word-spacing", true},
        {"white-space", true},
        {"visibility", true},
        {"cursor", true},
        {"display", false},
        {"position", false},
        {"width", false},
        {"height", false},
        {"margin", false},
        {"padding", false},
        {"border", false},
        {"background", false},
    };
    
    auto it = inheritMap.find(property);
    return it != inheritMap.end() && it->second;
}

std::string CSSComputedStyle::initialValue(const std::string& property) {
    static const std::unordered_map<std::string, std::string> initialMap = {
        {"display", "inline"},
        {"position", "static"},
        {"visibility", "visible"},
        {"opacity", "1"},
        {"color", "black"},
        {"background-color", "transparent"},
        {"font-family", "sans-serif"},
        {"font-size", "16px"},
        {"font-weight", "normal"},
        {"text-align", "start"},
        {"width", "auto"},
        {"height", "auto"},
        {"margin", "0"},
        {"padding", "0"},
        {"border-width", "0"},
        {"box-sizing", "content-box"},
    };
    
    auto it = initialMap.find(property);
    return it != initialMap.end() ? it->second : "";
}

} // namespace Zepra::WebCore
