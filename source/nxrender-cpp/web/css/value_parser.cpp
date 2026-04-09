// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "value_parser.h"
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace NXRender {
namespace Web {

// ==================================================================
// Named CSS colors (CSS Color Level 4)
// ==================================================================

static const std::unordered_map<std::string, uint32_t>& namedColors() {
    static const std::unordered_map<std::string, uint32_t> colors = {
        {"transparent", 0x00000000},
        {"black", 0x000000FF}, {"white", 0xFFFFFFFF},
        {"red", 0xFF0000FF}, {"green", 0x008000FF}, {"blue", 0x0000FFFF},
        {"lime", 0x00FF00FF}, {"yellow", 0xFFFF00FF}, {"cyan", 0x00FFFFFF},
        {"magenta", 0xFF00FFFF}, {"aqua", 0x00FFFFFF}, {"fuchsia", 0xFF00FFFF},
        {"silver", 0xC0C0C0FF}, {"gray", 0x808080FF}, {"grey", 0x808080FF},
        {"maroon", 0x800000FF}, {"olive", 0x808000FF}, {"navy", 0x000080FF},
        {"purple", 0x800080FF}, {"teal", 0x008080FF},
        {"orange", 0xFFA500FF}, {"orangered", 0xFF4500FF},
        {"coral", 0xFF7F50FF}, {"salmon", 0xFA8072FF},
        {"crimson", 0xDC143CFF}, {"firebrick", 0xB22222FF},
        {"darkred", 0x8B0000FF}, {"indianred", 0xCD5C5CFF},
        {"tomato", 0xFF6347FF}, {"gold", 0xFFD700FF},
        {"khaki", 0xF0E68CFF}, {"darkkhaki", 0xBDB76BFF},
        {"lemonchiffon", 0xFFFACDFF}, {"lightyellow", 0xFFFFE0FF},
        {"greenyellow", 0xADFF2FFF}, {"chartreuse", 0x7FFF00FF},
        {"lawngreen", 0x7CFC00FF}, {"springgreen", 0x00FF7FFF},
        {"darkgreen", 0x006400FF}, {"forestgreen", 0x228B22FF},
        {"seagreen", 0x2E8B57FF}, {"lightgreen", 0x90EE90FF},
        {"palegreen", 0x98FB98FF}, {"mediumaquamarine", 0x66CDAAFF},
        {"turquoise", 0x40E0D0FF}, {"lightseagreen", 0x20B2AAFF},
        {"darkcyan", 0x008B8BFF}, {"cadetblue", 0x5F9EA0FF},
        {"steelblue", 0x4682B4FF}, {"lightsteelblue", 0xB0C4DEFF},
        {"powderblue", 0xB0E0E6FF}, {"lightblue", 0xADD8E6FF},
        {"skyblue", 0x87CEEBFF}, {"deepskyblue", 0x00BFFFFF},
        {"dodgerblue", 0x1E90FFFF}, {"cornflowerblue", 0x6495EDFF},
        {"royalblue", 0x4169E1FF}, {"mediumblue", 0x0000CDFF},
        {"darkblue", 0x00008BFF}, {"midnightblue", 0x191970FF},
        {"slateblue", 0x6A5ACDFF}, {"darkslateblue", 0x483D8BFF},
        {"mediumpurple", 0x9370DBFF}, {"blueviolet", 0x8A2BE2FF},
        {"darkviolet", 0x9400D3FF}, {"darkorchid", 0x9932CCFF},
        {"orchid", 0xDA70D6FF}, {"violet", 0xEE82EEFF},
        {"plum", 0xDDA0DDFF}, {"thistle", 0xD8BFD8FF},
        {"lavender", 0xE6E6FAFF}, {"lavenderblush", 0xFFF0F5FF},
        {"pink", 0xFFC0CBFF}, {"lightpink", 0xFFB6C1FF},
        {"hotpink", 0xFF69B4FF}, {"deeppink", 0xFF1493FF},
        {"mediumvioletred", 0xC71585FF}, {"palevioletred", 0xDB7093FF},
        {"mistyrose", 0xFFE4E1FF}, {"antiquewhite", 0xFAEBD7FF},
        {"linen", 0xFAF0E6FF}, {"beige", 0xF5F5DCFF},
        {"bisque", 0xFFE4C4FF}, {"blanchedalmond", 0xFFEBCDFF},
        {"wheat", 0xF5DEB3FF}, {"cornsilk", 0xFFF8DCFF},
        {"peachpuff", 0xFFDAB9FF}, {"navajowhite", 0xFFDEADFF},
        {"moccasin", 0xFFE4B5FF}, {"papayawhip", 0xFFEFD5FF},
        {"oldlace", 0xFDF5E6FF}, {"seashell", 0xFFF5EEFF},
        {"mintcream", 0xF5FFFAFF}, {"honeydew", 0xF0FFF0FF},
        {"aliceblue", 0xF0F8FFFF}, {"ghostwhite", 0xF8F8FFFF},
        {"whitesmoke", 0xF5F5F5FF}, {"gainsboro", 0xDCDCDCFF},
        {"lightgray", 0xD3D3D3FF}, {"lightgrey", 0xD3D3D3FF},
        {"darkgray", 0xA9A9A9FF}, {"darkgrey", 0xA9A9A9FF},
        {"dimgray", 0x696969FF}, {"dimgrey", 0x696969FF},
        {"lightslategray", 0x778899FF}, {"slategray", 0x708090FF},
        {"darkslategray", 0x2F4F4FFF},
        {"snow", 0xFFFAFAFF}, {"ivory", 0xFFFFF0FF},
        {"floralwhite", 0xFFFAF0FF}, {"azure", 0xF0FFFFFF},
        {"tan", 0xD2B48CFF}, {"chocolate", 0xD2691EFF},
        {"saddlebrown", 0x8B4513FF}, {"sienna", 0xA0522DFF},
        {"peru", 0xCD853FFF}, {"burlywood", 0xDEB887FF},
        {"sandybrown", 0xF4A460FF}, {"rosybrown", 0xBC8F8FFF},
        {"goldenrod", 0xDAA520FF}, {"darkgoldenrod", 0xB8860BFF},
        {"brown", 0xA52A2AFF},
        // System colors
        {"currentcolor", 0x00000000},
    };
    return colors;
}

// ==================================================================
// Length parsing
// ==================================================================

static CSSLengthUnit parseLengthUnit(const std::string& s, size_t& pos) {
    if (pos >= s.size()) return CSSLengthUnit::Px;

    std::string unit;
    while (pos < s.size() && std::isalpha(s[pos])) {
        unit += static_cast<char>(std::tolower(s[pos]));
        pos++;
    }
    if (pos < s.size() && s[pos] == '%') {
        pos++;
        return CSSLengthUnit::Percent;
    }

    if (unit == "px") return CSSLengthUnit::Px;
    if (unit == "em") return CSSLengthUnit::Em;
    if (unit == "rem") return CSSLengthUnit::Rem;
    if (unit == "ex") return CSSLengthUnit::Ex;
    if (unit == "ch") return CSSLengthUnit::Ch;
    if (unit == "vw") return CSSLengthUnit::Vw;
    if (unit == "vh") return CSSLengthUnit::Vh;
    if (unit == "vmin") return CSSLengthUnit::Vmin;
    if (unit == "vmax") return CSSLengthUnit::Vmax;
    if (unit == "cm") return CSSLengthUnit::Cm;
    if (unit == "mm") return CSSLengthUnit::Mm;
    if (unit == "in") return CSSLengthUnit::In;
    if (unit == "pt") return CSSLengthUnit::Pt;
    if (unit == "pc") return CSSLengthUnit::Pc;
    if (unit == "q") return CSSLengthUnit::Q;
    if (unit == "fr") return CSSLengthUnit::Fr;
    if (unit == "svw") return CSSLengthUnit::Svw;
    if (unit == "svh") return CSSLengthUnit::Svh;
    if (unit == "lvw") return CSSLengthUnit::Lvw;
    if (unit == "lvh") return CSSLengthUnit::Lvh;
    if (unit == "dvw") return CSSLengthUnit::Dvw;
    if (unit == "dvh") return CSSLengthUnit::Dvh;
    if (unit == "cap") return CSSLengthUnit::Cap;
    if (unit == "ic") return CSSLengthUnit::Ic;
    if (unit == "lh") return CSSLengthUnit::Lh;
    if (unit == "rlh") return CSSLengthUnit::Rlh;

    return CSSLengthUnit::Px;
}

// ==================================================================
// Utility
// ==================================================================

void CSSValueParser::skipWhitespace(const std::string& s, size_t& pos) {
    while (pos < s.size() && std::isspace(s[pos])) pos++;
}

bool CSSValueParser::matchChar(const std::string& s, size_t& pos, char c) {
    if (pos < s.size() && s[pos] == c) { pos++; return true; }
    return false;
}

float CSSValueParser::parseFloat(const std::string& s, size_t& pos) {
    skipWhitespace(s, pos);
    size_t start = pos;
    if (pos < s.size() && (s[pos] == '-' || s[pos] == '+')) pos++;
    while (pos < s.size() && (std::isdigit(s[pos]) || s[pos] == '.')) pos++;
    if (pos == start) return 0;
    return std::strtof(s.c_str() + start, nullptr);
}

std::string CSSValueParser::parseIdent(const std::string& s, size_t& pos) {
    skipWhitespace(s, pos);
    std::string result;
    if (pos < s.size() && (s[pos] == '-' || s[pos] == '_' || std::isalpha(s[pos]))) {
        while (pos < s.size() && (s[pos] == '-' || s[pos] == '_' || std::isalnum(s[pos]))) {
            result += s[pos++];
        }
    }
    return result;
}

// ==================================================================
// Color parsing
// ==================================================================

ComputedColor ComputedColor::fromHex(const std::string& hex) {
    std::string h = hex;
    if (!h.empty() && h[0] == '#') h = h.substr(1);

    ComputedColor c;

    if (h.size() == 3) {
        c.r = static_cast<uint8_t>(std::stoul(h.substr(0, 1) + h.substr(0, 1), nullptr, 16));
        c.g = static_cast<uint8_t>(std::stoul(h.substr(1, 1) + h.substr(1, 1), nullptr, 16));
        c.b = static_cast<uint8_t>(std::stoul(h.substr(2, 1) + h.substr(2, 1), nullptr, 16));
        c.a = 255;
    } else if (h.size() == 4) {
        c.r = static_cast<uint8_t>(std::stoul(h.substr(0, 1) + h.substr(0, 1), nullptr, 16));
        c.g = static_cast<uint8_t>(std::stoul(h.substr(1, 1) + h.substr(1, 1), nullptr, 16));
        c.b = static_cast<uint8_t>(std::stoul(h.substr(2, 1) + h.substr(2, 1), nullptr, 16));
        c.a = static_cast<uint8_t>(std::stoul(h.substr(3, 1) + h.substr(3, 1), nullptr, 16));
    } else if (h.size() == 6) {
        c.r = static_cast<uint8_t>(std::stoul(h.substr(0, 2), nullptr, 16));
        c.g = static_cast<uint8_t>(std::stoul(h.substr(2, 2), nullptr, 16));
        c.b = static_cast<uint8_t>(std::stoul(h.substr(4, 2), nullptr, 16));
        c.a = 255;
    } else if (h.size() == 8) {
        c.r = static_cast<uint8_t>(std::stoul(h.substr(0, 2), nullptr, 16));
        c.g = static_cast<uint8_t>(std::stoul(h.substr(2, 2), nullptr, 16));
        c.b = static_cast<uint8_t>(std::stoul(h.substr(4, 2), nullptr, 16));
        c.a = static_cast<uint8_t>(std::stoul(h.substr(6, 2), nullptr, 16));
    }

    return c;
}

ComputedColor ComputedColor::fromRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return {r, g, b, a};
}

static float hueToRGB(float p, float q, float t) {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1.0f/6) return p + (q - p) * 6 * t;
    if (t < 1.0f/2) return q;
    if (t < 2.0f/3) return p + (q - p) * (2.0f/3 - t) * 6;
    return p;
}

ComputedColor ComputedColor::fromHSL(float h, float s, float l, float a) {
    h = std::fmod(h, 360.0f);
    if (h < 0) h += 360.0f;
    h /= 360.0f;
    s = std::clamp(s / 100.0f, 0.0f, 1.0f);
    l = std::clamp(l / 100.0f, 0.0f, 1.0f);

    float r, g, b;
    if (s == 0) {
        r = g = b = l;
    } else {
        float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
        float p = 2 * l - q;
        r = hueToRGB(p, q, h + 1.0f/3);
        g = hueToRGB(p, q, h);
        b = hueToRGB(p, q, h - 1.0f/3);
    }

    return {
        static_cast<uint8_t>(std::round(r * 255)),
        static_cast<uint8_t>(std::round(g * 255)),
        static_cast<uint8_t>(std::round(b * 255)),
        static_cast<uint8_t>(std::round(std::clamp(a, 0.0f, 1.0f) * 255))
    };
}

ComputedColor ComputedColor::fromName(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    auto& map = namedColors();
    auto it = map.find(lower);
    if (it != map.end()) {
        uint32_t v = it->second;
        return {
            static_cast<uint8_t>((v >> 24) & 0xFF),
            static_cast<uint8_t>((v >> 16) & 0xFF),
            static_cast<uint8_t>((v >> 8) & 0xFF),
            static_cast<uint8_t>(v & 0xFF)
        };
    }
    return {0, 0, 0, 255};
}

std::optional<ComputedColor> ComputedColor::parse(const std::string& str) {
    std::string s = str;
    // Trim
    while (!s.empty() && std::isspace(s.front())) s.erase(s.begin());
    while (!s.empty() && std::isspace(s.back())) s.pop_back();

    if (s.empty()) return std::nullopt;

    // Hex
    if (s[0] == '#') return fromHex(s);

    // rgb() / rgba()
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower.substr(0, 4) == "rgb(" || lower.substr(0, 5) == "rgba(") {
        size_t start = s.find('(');
        size_t end = s.rfind(')');
        if (start == std::string::npos || end == std::string::npos) return std::nullopt;

        std::string inner = s.substr(start + 1, end - start - 1);
        // Replace commas and slashes with spaces
        for (char& c : inner) {
            if (c == ',' || c == '/') c = ' ';
        }

        std::istringstream iss(inner);
        float r, g, b, a = 255;
        if (!(iss >> r >> g >> b)) return std::nullopt;
        iss >> a;

        // If values > 1 but <= 255, assume 0-255 range
        if (a <= 1.0f && r > 1.0f) a *= 255;

        return ComputedColor{
            static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(a, 0.0f, 255.0f))
        };
    }

    // hsl() / hsla()
    if (lower.substr(0, 4) == "hsl(" || lower.substr(0, 5) == "hsla(") {
        size_t start = s.find('(');
        size_t end = s.rfind(')');
        if (start == std::string::npos || end == std::string::npos) return std::nullopt;

        std::string inner = s.substr(start + 1, end - start - 1);
        for (char& c : inner) {
            if (c == ',' || c == '/' || c == '%') c = ' ';
        }

        std::istringstream iss(inner);
        float h, sat, l, a = 1.0f;
        if (!(iss >> h >> sat >> l)) return std::nullopt;
        iss >> a;

        return fromHSL(h, sat, l, a);
    }

    // Named color
    auto& map = namedColors();
    auto it = map.find(lower);
    if (it != map.end()) return fromName(lower);

    return std::nullopt;
}

// ==================================================================
// Length resolution
// ==================================================================

ComputedLength ComputedLength::resolve(float value, CSSLengthUnit unit,
                                        float fontSize, float rootFontSize,
                                        float viewportWidth, float viewportHeight,
                                        float containerSize) {
    ComputedLength result;

    switch (unit) {
        case CSSLengthUnit::Px: result.px = value; break;
        case CSSLengthUnit::Em: result.px = value * fontSize; break;
        case CSSLengthUnit::Rem: result.px = value * rootFontSize; break;
        case CSSLengthUnit::Ex: result.px = value * fontSize * 0.5f; break;
        case CSSLengthUnit::Ch: result.px = value * fontSize * 0.5f; break;
        case CSSLengthUnit::Cap: result.px = value * fontSize * 0.7f; break;
        case CSSLengthUnit::Ic: result.px = value * fontSize; break;
        case CSSLengthUnit::Lh: result.px = value * fontSize * 1.2f; break;
        case CSSLengthUnit::Rlh: result.px = value * rootFontSize * 1.2f; break;
        case CSSLengthUnit::Vw: result.px = value * viewportWidth / 100.0f; break;
        case CSSLengthUnit::Vh: result.px = value * viewportHeight / 100.0f; break;
        case CSSLengthUnit::Vmin: result.px = value * std::min(viewportWidth, viewportHeight) / 100.0f; break;
        case CSSLengthUnit::Vmax: result.px = value * std::max(viewportWidth, viewportHeight) / 100.0f; break;
        case CSSLengthUnit::Svw: result.px = value * viewportWidth / 100.0f; break;
        case CSSLengthUnit::Svh: result.px = value * viewportHeight / 100.0f; break;
        case CSSLengthUnit::Lvw: result.px = value * viewportWidth / 100.0f; break;
        case CSSLengthUnit::Lvh: result.px = value * viewportHeight / 100.0f; break;
        case CSSLengthUnit::Dvw: result.px = value * viewportWidth / 100.0f; break;
        case CSSLengthUnit::Dvh: result.px = value * viewportHeight / 100.0f; break;
        case CSSLengthUnit::Vi: result.px = value * viewportWidth / 100.0f; break;
        case CSSLengthUnit::Vb: result.px = value * viewportHeight / 100.0f; break;
        case CSSLengthUnit::Cm: result.px = value * 96.0f / 2.54f; break;
        case CSSLengthUnit::Mm: result.px = value * 96.0f / 25.4f; break;
        case CSSLengthUnit::In: result.px = value * 96.0f; break;
        case CSSLengthUnit::Pt: result.px = value * 96.0f / 72.0f; break;
        case CSSLengthUnit::Pc: result.px = value * 96.0f / 6.0f; break;
        case CSSLengthUnit::Q: result.px = value * 96.0f / (2.54f * 4.0f); break;
        case CSSLengthUnit::Percent: result.px = value * containerSize / 100.0f; break;
        case CSSLengthUnit::Fr: result.px = value; break; // Fr resolved at grid level
        case CSSLengthUnit::Auto: result.px = 0; break;
    }

    return result;
}

ComputedLength CSSValueParser::resolveLength(const CSSValue& value,
                                               float fontSize, float rootFontSize,
                                               float viewportWidth, float viewportHeight) {
    if (value.type == CSSValueType::Length || value.type == CSSValueType::Percentage) {
        return ComputedLength::resolve(value.numericValue, value.lengthUnit,
                                        fontSize, rootFontSize,
                                        viewportWidth, viewportHeight);
    }
    return {0};
}

// ==================================================================
// Value parsing
// ==================================================================

CSSValue CSSValueParser::parseLength(const std::string& input) {
    std::string s = input;
    while (!s.empty() && std::isspace(s.front())) s.erase(s.begin());
    while (!s.empty() && std::isspace(s.back())) s.pop_back();

    if (s == "auto") return CSSValue::keyword("auto");
    if (s == "none") return CSSValue::keyword("none");
    if (s == "inherit") return CSSValue::inherit();
    if (s == "initial") return CSSValue::initial();

    size_t pos = 0;
    float val = parseFloat(s, pos);

    if (pos < s.size() && s[pos] == '%') {
        pos++;
        return CSSValue::percentage(val);
    }

    CSSLengthUnit unit = parseLengthUnit(s, pos);
    return CSSValue::length(val, unit);
}

CSSValue CSSValueParser::parseColor(const std::string& input) {
    auto color = ComputedColor::parse(input);
    if (color.has_value()) {
        return CSSValue::color(color->toRGBA());
    }
    return CSSValue::keyword(input);
}

CSSValue CSSValueParser::parseNumber(const std::string& input) {
    std::string s = input;
    while (!s.empty() && std::isspace(s.front())) s.erase(s.begin());

    if (s == "inherit") return CSSValue::inherit();
    if (s == "initial") return CSSValue::initial();

    size_t pos = 0;
    float val = parseFloat(s, pos);
    return CSSValue::number(val);
}

std::optional<ComputedColor> CSSValueParser::parseColorValue(const std::string& input) {
    return ComputedColor::parse(input);
}

// ==================================================================
// Shorthand expansion
// ==================================================================

std::vector<std::pair<CSSPropertyID, CSSValue>>
CSSValueParser::expandShorthand(CSSPropertyID shorthand, const std::string& value) {
    std::vector<std::pair<CSSPropertyID, CSSValue>> result;

    // Split value by whitespace
    std::vector<std::string> parts;
    std::istringstream iss(value);
    std::string part;
    while (iss >> part) parts.push_back(part);

    switch (shorthand) {
        case CSSPropertyID::Margin:
        case CSSPropertyID::Padding: {
            CSSPropertyID top, right, bottom, left;
            if (shorthand == CSSPropertyID::Margin) {
                top = CSSPropertyID::MarginTop;
                right = CSSPropertyID::MarginRight;
                bottom = CSSPropertyID::MarginBottom;
                left = CSSPropertyID::MarginLeft;
            } else {
                top = CSSPropertyID::PaddingTop;
                right = CSSPropertyID::PaddingRight;
                bottom = CSSPropertyID::PaddingBottom;
                left = CSSPropertyID::PaddingLeft;
            }

            if (parts.size() == 1) {
                auto v = parseLength(parts[0]);
                result.push_back({top, v});
                result.push_back({right, v});
                result.push_back({bottom, v});
                result.push_back({left, v});
            } else if (parts.size() == 2) {
                auto vV = parseLength(parts[0]);
                auto hV = parseLength(parts[1]);
                result.push_back({top, vV});
                result.push_back({right, hV});
                result.push_back({bottom, vV});
                result.push_back({left, hV});
            } else if (parts.size() == 3) {
                result.push_back({top, parseLength(parts[0])});
                auto hV = parseLength(parts[1]);
                result.push_back({right, hV});
                result.push_back({bottom, parseLength(parts[2])});
                result.push_back({left, hV});
            } else if (parts.size() >= 4) {
                result.push_back({top, parseLength(parts[0])});
                result.push_back({right, parseLength(parts[1])});
                result.push_back({bottom, parseLength(parts[2])});
                result.push_back({left, parseLength(parts[3])});
            }
            break;
        }

        case CSSPropertyID::BorderRadius: {
            if (parts.size() == 1) {
                auto v = parseLength(parts[0]);
                result.push_back({CSSPropertyID::BorderTopLeftRadius, v});
                result.push_back({CSSPropertyID::BorderTopRightRadius, v});
                result.push_back({CSSPropertyID::BorderBottomRightRadius, v});
                result.push_back({CSSPropertyID::BorderBottomLeftRadius, v});
            } else if (parts.size() == 2) {
                result.push_back({CSSPropertyID::BorderTopLeftRadius, parseLength(parts[0])});
                result.push_back({CSSPropertyID::BorderTopRightRadius, parseLength(parts[1])});
                result.push_back({CSSPropertyID::BorderBottomRightRadius, parseLength(parts[0])});
                result.push_back({CSSPropertyID::BorderBottomLeftRadius, parseLength(parts[1])});
            } else if (parts.size() >= 4) {
                result.push_back({CSSPropertyID::BorderTopLeftRadius, parseLength(parts[0])});
                result.push_back({CSSPropertyID::BorderTopRightRadius, parseLength(parts[1])});
                result.push_back({CSSPropertyID::BorderBottomRightRadius, parseLength(parts[2])});
                result.push_back({CSSPropertyID::BorderBottomLeftRadius, parseLength(parts[3])});
            }
            break;
        }

        case CSSPropertyID::Inset: {
            if (parts.size() == 1) {
                auto v = parseLength(parts[0]);
                result.push_back({CSSPropertyID::Top, v});
                result.push_back({CSSPropertyID::Right, v});
                result.push_back({CSSPropertyID::Bottom, v});
                result.push_back({CSSPropertyID::Left, v});
            } else if (parts.size() == 2) {
                result.push_back({CSSPropertyID::Top, parseLength(parts[0])});
                result.push_back({CSSPropertyID::Right, parseLength(parts[1])});
                result.push_back({CSSPropertyID::Bottom, parseLength(parts[0])});
                result.push_back({CSSPropertyID::Left, parseLength(parts[1])});
            } else if (parts.size() >= 4) {
                result.push_back({CSSPropertyID::Top, parseLength(parts[0])});
                result.push_back({CSSPropertyID::Right, parseLength(parts[1])});
                result.push_back({CSSPropertyID::Bottom, parseLength(parts[2])});
                result.push_back({CSSPropertyID::Left, parseLength(parts[3])});
            }
            break;
        }

        case CSSPropertyID::Gap: {
            if (parts.size() == 1) {
                auto v = parseLength(parts[0]);
                result.push_back({CSSPropertyID::RowGap, v});
                result.push_back({CSSPropertyID::ColumnGap, v});
            } else if (parts.size() >= 2) {
                result.push_back({CSSPropertyID::RowGap, parseLength(parts[0])});
                result.push_back({CSSPropertyID::ColumnGap, parseLength(parts[1])});
            }
            break;
        }

        case CSSPropertyID::Overflow: {
            if (parts.size() == 1) {
                auto v = CSSValue::keyword(parts[0]);
                result.push_back({CSSPropertyID::OverflowX, v});
                result.push_back({CSSPropertyID::OverflowY, v});
            } else if (parts.size() >= 2) {
                result.push_back({CSSPropertyID::OverflowX, CSSValue::keyword(parts[0])});
                result.push_back({CSSPropertyID::OverflowY, CSSValue::keyword(parts[1])});
            }
            break;
        }

        case CSSPropertyID::Flex: {
            if (parts.size() == 1) {
                if (parts[0] == "none") {
                    result.push_back({CSSPropertyID::FlexGrow, CSSValue::number(0)});
                    result.push_back({CSSPropertyID::FlexShrink, CSSValue::number(0)});
                    result.push_back({CSSPropertyID::FlexBasis, CSSValue::keyword("auto")});
                } else if (parts[0] == "auto") {
                    result.push_back({CSSPropertyID::FlexGrow, CSSValue::number(1)});
                    result.push_back({CSSPropertyID::FlexShrink, CSSValue::number(1)});
                    result.push_back({CSSPropertyID::FlexBasis, CSSValue::keyword("auto")});
                } else {
                    result.push_back({CSSPropertyID::FlexGrow, parseNumber(parts[0])});
                    result.push_back({CSSPropertyID::FlexShrink, CSSValue::number(1)});
                    result.push_back({CSSPropertyID::FlexBasis, CSSValue::length(0, CSSLengthUnit::Px)});
                }
            } else if (parts.size() == 2) {
                result.push_back({CSSPropertyID::FlexGrow, parseNumber(parts[0])});
                result.push_back({CSSPropertyID::FlexShrink, parseNumber(parts[1])});
                result.push_back({CSSPropertyID::FlexBasis, CSSValue::length(0, CSSLengthUnit::Px)});
            } else if (parts.size() >= 3) {
                result.push_back({CSSPropertyID::FlexGrow, parseNumber(parts[0])});
                result.push_back({CSSPropertyID::FlexShrink, parseNumber(parts[1])});
                result.push_back({CSSPropertyID::FlexBasis, parseLength(parts[2])});
            }
            break;
        }

        default:
            // Unknown shorthand — pass through as single value
            if (!parts.empty()) {
                result.push_back({shorthand, parseLength(parts[0])});
            }
            break;
    }

    return result;
}

// ==================================================================
// Generic value parse
// ==================================================================

CSSValue CSSValueParser::parseValue(CSSPropertyID property, const std::string& input) {
    std::string s = input;
    while (!s.empty() && std::isspace(s.front())) s.erase(s.begin());
    while (!s.empty() && std::isspace(s.back())) s.pop_back();

    // Global keywords
    if (s == "inherit") return CSSValue::inherit();
    if (s == "initial") return CSSValue::initial();
    if (s == "unset") { CSSValue v; v.type = CSSValueType::Unset; return v; }
    if (s == "revert") { CSSValue v; v.type = CSSValueType::Revert; return v; }

    // Property-specific parsing
    switch (property) {
        case CSSPropertyID::Color:
        case CSSPropertyID::BackgroundColor:
        case CSSPropertyID::BorderTopColor:
        case CSSPropertyID::BorderRightColor:
        case CSSPropertyID::BorderBottomColor:
        case CSSPropertyID::BorderLeftColor:
        case CSSPropertyID::OutlineColor:
        case CSSPropertyID::CaretColor:
        case CSSPropertyID::AccentColor:
            return parseColor(s);

        case CSSPropertyID::Opacity:
        case CSSPropertyID::FlexGrow:
        case CSSPropertyID::FlexShrink:
        case CSSPropertyID::Order:
        case CSSPropertyID::ZIndex:
            return parseNumber(s);

        case CSSPropertyID::Display:
        case CSSPropertyID::Position:
        case CSSPropertyID::Float:
        case CSSPropertyID::Clear:
        case CSSPropertyID::BoxSizing:
        case CSSPropertyID::Visibility:
        case CSSPropertyID::TextAlign:
        case CSSPropertyID::TextTransform:
        case CSSPropertyID::WhiteSpace:
        case CSSPropertyID::WordBreak:
        case CSSPropertyID::OverflowWrap:
        case CSSPropertyID::FlexDirection:
        case CSSPropertyID::FlexWrap:
        case CSSPropertyID::JustifyContent:
        case CSSPropertyID::AlignItems:
        case CSSPropertyID::AlignContent:
        case CSSPropertyID::AlignSelf:
        case CSSPropertyID::Cursor:
        case CSSPropertyID::PointerEvents:
        case CSSPropertyID::UserSelect:
        case CSSPropertyID::ObjectFit:
        case CSSPropertyID::Appearance:
        case CSSPropertyID::Isolation:
        case CSSPropertyID::GridAutoFlow:
        case CSSPropertyID::Direction:
        case CSSPropertyID::UnicodeBidi:
        case CSSPropertyID::WritingMode:
        case CSSPropertyID::TableLayout:
        case CSSPropertyID::BorderCollapse:
        case CSSPropertyID::CaptionSide:
        case CSSPropertyID::ListStyleType:
        case CSSPropertyID::ListStylePosition:
        case CSSPropertyID::Resize:
        case CSSPropertyID::ScrollBehavior:
        case CSSPropertyID::ColumnSpan:
        case CSSPropertyID::ColumnFill:
        case CSSPropertyID::BreakBefore:
        case CSSPropertyID::BreakAfter:
        case CSSPropertyID::BreakInside:
        case CSSPropertyID::ContentVisibility:
        case CSSPropertyID::ContainerType:
            return CSSValue::keyword(s);

        default:
            return parseLength(s);
    }
}

// ==================================================================
// Box shadow parsing
// ==================================================================

std::vector<BoxShadowValue> CSSValueParser::parseBoxShadow(const std::string& input) {
    std::vector<BoxShadowValue> shadows;
    if (input == "none" || input.empty()) return shadows;

    // Split by comma (simplified — doesn't handle commas in functions)
    std::vector<std::string> parts;
    std::string current;
    int parenDepth = 0;
    for (char c : input) {
        if (c == '(') parenDepth++;
        else if (c == ')') parenDepth--;
        else if (c == ',' && parenDepth == 0) {
            parts.push_back(current);
            current.clear();
            continue;
        }
        current += c;
    }
    if (!current.empty()) parts.push_back(current);

    for (const auto& shadowStr : parts) {
        BoxShadowValue shadow;
        std::istringstream iss(shadowStr);

        std::string token;
        std::vector<float> numbers;
        std::string colorStr;

        while (iss >> token) {
            std::string lower = token;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            if (lower == "inset") {
                shadow.inset = true;
            } else if (token[0] == '#' || lower.substr(0, 3) == "rgb" ||
                       lower.substr(0, 3) == "hsl" || namedColors().count(lower)) {
                colorStr = token;
                // Absorb rest if it's a function
                if (lower.substr(0, 3) == "rgb" || lower.substr(0, 3) == "hsl") {
                    while (!token.empty() && token.back() != ')' && iss >> token) {
                        colorStr += " " + token;
                    }
                }
            } else {
                size_t pos = 0;
                float val = CSSValueParser::parseFloat(token, pos);
                numbers.push_back(val);
            }
        }

        if (numbers.size() >= 2) {
            shadow.offsetX = numbers[0];
            shadow.offsetY = numbers[1];
        }
        if (numbers.size() >= 3) shadow.blurRadius = numbers[2];
        if (numbers.size() >= 4) shadow.spreadRadius = numbers[3];

        if (!colorStr.empty()) {
            auto c = ComputedColor::parse(colorStr);
            if (c) shadow.color = *c;
        } else {
            shadow.color = {0, 0, 0, 255};
        }

        shadows.push_back(shadow);
    }

    return shadows;
}

// ==================================================================
// Transform parsing
// ==================================================================

std::vector<TransformFunction> CSSValueParser::parseTransform(const std::string& input) {
    std::vector<TransformFunction> transforms;
    if (input == "none" || input.empty()) return transforms;

    size_t pos = 0;
    while (pos < input.size()) {
        skipWhitespace(input, pos);
        if (pos >= input.size()) break;

        std::string fname = parseIdent(input, pos);
        if (fname.empty()) break;

        skipWhitespace(input, pos);
        if (!matchChar(input, pos, '(')) break;

        // Parse arguments
        std::vector<float> args;
        while (pos < input.size() && input[pos] != ')') {
            skipWhitespace(input, pos);
            if (pos < input.size() && input[pos] == ')') break;

            float val = parseFloat(input, pos);
            args.push_back(val);

            // Skip unit suffix
            while (pos < input.size() && (std::isalpha(input[pos]) || input[pos] == '%')) pos++;
            skipWhitespace(input, pos);
            matchChar(input, pos, ',');
        }
        matchChar(input, pos, ')');

        TransformFunction tf;
        tf.args = args;

        std::string lower = fname;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (lower == "translate") tf.type = TransformFunction::Type::Translate;
        else if (lower == "translatex") tf.type = TransformFunction::Type::TranslateX;
        else if (lower == "translatey") tf.type = TransformFunction::Type::TranslateY;
        else if (lower == "translatez") tf.type = TransformFunction::Type::TranslateZ;
        else if (lower == "translate3d") tf.type = TransformFunction::Type::Translate3D;
        else if (lower == "scale") tf.type = TransformFunction::Type::Scale;
        else if (lower == "scalex") tf.type = TransformFunction::Type::ScaleX;
        else if (lower == "scaley") tf.type = TransformFunction::Type::ScaleY;
        else if (lower == "scalez") tf.type = TransformFunction::Type::ScaleZ;
        else if (lower == "scale3d") tf.type = TransformFunction::Type::Scale3D;
        else if (lower == "rotate") tf.type = TransformFunction::Type::Rotate;
        else if (lower == "rotatex") tf.type = TransformFunction::Type::RotateX;
        else if (lower == "rotatey") tf.type = TransformFunction::Type::RotateY;
        else if (lower == "rotatez") tf.type = TransformFunction::Type::RotateZ;
        else if (lower == "rotate3d") tf.type = TransformFunction::Type::Rotate3D;
        else if (lower == "skew") tf.type = TransformFunction::Type::Skew;
        else if (lower == "skewx") tf.type = TransformFunction::Type::SkewX;
        else if (lower == "skewy") tf.type = TransformFunction::Type::SkewY;
        else if (lower == "matrix") tf.type = TransformFunction::Type::Matrix;
        else if (lower == "matrix3d") tf.type = TransformFunction::Type::Matrix3D;
        else if (lower == "perspective") tf.type = TransformFunction::Type::Perspective;
        else continue;

        transforms.push_back(tf);
    }

    return transforms;
}

} // namespace Web
} // namespace NXRender
