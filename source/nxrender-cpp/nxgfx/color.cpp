// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file color.cpp
 * @brief Color implementation with HSL, premultiplied alpha, CSS parsing
 */

#include "nxgfx/color.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <cmath>

namespace NXRender {

// =========================================================================
// Named CSS Colors (W3C Level 1 + extended)
// =========================================================================

static const std::unordered_map<std::string, uint32_t> namedColors = {
    {"black", 0x000000}, {"white", 0xFFFFFF}, {"red", 0xFF0000},
    {"green", 0x008000}, {"blue", 0x0000FF}, {"yellow", 0xFFFF00},
    {"cyan", 0x00FFFF}, {"magenta", 0xFF00FF}, {"orange", 0xFFA500},
    {"purple", 0x800080}, {"pink", 0xFFC0CB}, {"gray", 0x808080},
    {"grey", 0x808080}, {"silver", 0xC0C0C0}, {"maroon", 0x800000},
    {"navy", 0x000080}, {"olive", 0x808000}, {"teal", 0x008080},
    {"aqua", 0x00FFFF}, {"fuchsia", 0xFF00FF}, {"lime", 0x00FF00},
    {"darkgray", 0xA9A9A9}, {"darkgrey", 0xA9A9A9},
    {"lightgray", 0xD3D3D3}, {"lightgrey", 0xD3D3D3},
    {"dimgray", 0x696969}, {"dimgrey", 0x696969},
    {"darkred", 0x8B0000}, {"darkgreen", 0x006400}, {"darkblue", 0x00008B},
    {"darkcyan", 0x008B8B}, {"darkmagenta", 0x8B008B},
    {"darkorange", 0xFF8C00}, {"darkviolet", 0x9400D3},
    {"deeppink", 0xFF1493}, {"deepskyblue", 0x00BFFF},
    {"gold", 0xFFD700}, {"goldenrod", 0xDAA520},
    {"hotpink", 0xFF69B4}, {"indigo", 0x4B0082},
    {"ivory", 0xFFFFF0}, {"khaki", 0xF0E68C},
    {"lavender", 0xE6E6FA}, {"lightblue", 0xADD8E6},
    {"lightcoral", 0xF08080}, {"lightcyan", 0xE0FFFF},
    {"lightgreen", 0x90EE90}, {"lightpink", 0xFFB6C1},
    {"lightyellow", 0xFFFFE0}, {"limegreen", 0x32CD32},
    {"coral", 0xFF7F50}, {"cornflowerblue", 0x6495ED},
    {"crimson", 0xDC143C}, {"firebrick", 0xB22222},
    {"forestgreen", 0x228B22}, {"gainsboro", 0xDCDCDC},
    {"honeydew", 0xF0FFF0}, {"indianred", 0xCD5C5C},
    {"mediumblue", 0x0000CD}, {"midnightblue", 0x191970},
    {"mintcream", 0xF5FFFA}, {"mistyrose", 0xFFE4E1},
    {"moccasin", 0xFFE4B5}, {"oldlace", 0xFDF5E6},
    {"orangered", 0xFF4500}, {"orchid", 0xDA70D6},
    {"palegreen", 0x98FB98}, {"peru", 0xCD853F},
    {"plum", 0xDDA0DD}, {"powderblue", 0xB0E0E6},
    {"royalblue", 0x4169E1}, {"salmon", 0xFA8072},
    {"seagreen", 0x2E8B57}, {"sienna", 0xA0522D},
    {"skyblue", 0x87CEEB}, {"slateblue", 0x6A5ACD},
    {"slategray", 0x708090}, {"slategrey", 0x708090},
    {"springgreen", 0x00FF7F}, {"steelblue", 0x4682B4},
    {"tan", 0xD2B48C}, {"thistle", 0xD8BFD8},
    {"tomato", 0xFF6347}, {"turquoise", 0x40E0D0},
    {"violet", 0xEE82EE}, {"wheat", 0xF5DEB3},
    {"whitesmoke", 0xF5F5F5}, {"yellowgreen", 0x9ACD32},
    {"transparent", 0x00000000}
};

// =========================================================================
// Helpers
// =========================================================================

static std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

static int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

static std::string trimWhitespace(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) start++;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) end--;
    return s.substr(start, end - start);
}

// =========================================================================
// HSL ↔ RGB Conversion
// =========================================================================

static float hueToRgb(float p, float q, float t) {
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f / 2.0f) return q;
    if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
    return p;
}

Color Color::fromHSL(float h, float s, float l, uint8_t a) {
    h = std::fmod(h, 360.0f);
    if (h < 0.0f) h += 360.0f;
    h /= 360.0f;
    s = std::clamp(s, 0.0f, 1.0f);
    l = std::clamp(l, 0.0f, 1.0f);

    if (s == 0.0f) {
        uint8_t v = static_cast<uint8_t>(l * 255.0f);
        return Color(v, v, v, a);
    }

    float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
    float p = 2.0f * l - q;

    float rf = hueToRgb(p, q, h + 1.0f / 3.0f);
    float gf = hueToRgb(p, q, h);
    float bf = hueToRgb(p, q, h - 1.0f / 3.0f);

    return Color(
        static_cast<uint8_t>(std::clamp(rf * 255.0f, 0.0f, 255.0f)),
        static_cast<uint8_t>(std::clamp(gf * 255.0f, 0.0f, 255.0f)),
        static_cast<uint8_t>(std::clamp(bf * 255.0f, 0.0f, 255.0f)),
        a
    );
}

HSL Color::toHSL() const {
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;

    float maxC = std::max({rf, gf, bf});
    float minC = std::min({rf, gf, bf});
    float delta = maxC - minC;

    float hue = 0.0f;
    float sat = 0.0f;
    float lit = (maxC + minC) / 2.0f;

    if (delta > 0.0f) {
        sat = lit > 0.5f ? delta / (2.0f - maxC - minC) : delta / (maxC + minC);

        if (maxC == rf) {
            hue = (gf - bf) / delta + (gf < bf ? 6.0f : 0.0f);
        } else if (maxC == gf) {
            hue = (bf - rf) / delta + 2.0f;
        } else {
            hue = (rf - gf) / delta + 4.0f;
        }
        hue *= 60.0f;
    }

    return {hue, sat, lit};
}

// =========================================================================
// Premultiplied Alpha
// =========================================================================

Color Color::premultiply() const {
    if (a == 255) return *this;
    if (a == 0) return Color(0, 0, 0, 0);
    float af = a / 255.0f;
    return Color(
        static_cast<uint8_t>(r * af),
        static_cast<uint8_t>(g * af),
        static_cast<uint8_t>(b * af),
        a
    );
}

Color Color::unpremultiply() const {
    if (a == 255 || a == 0) return *this;
    float invA = 255.0f / a;
    return Color(
        static_cast<uint8_t>(std::min(r * invA, 255.0f)),
        static_cast<uint8_t>(std::min(g * invA, 255.0f)),
        static_cast<uint8_t>(std::min(b * invA, 255.0f)),
        a
    );
}

// =========================================================================
// CSS Color Parsing
// =========================================================================

Color Color::parse(const std::string& str) {
    if (str.empty()) return Color::black();
    
    std::string s = trimWhitespace(str);
    if (s.empty()) return Color::black();
    
    // Hex color
    if (s[0] == '#') {
        s = s.substr(1);
        if (s.length() == 3) {
            uint8_t rv = hexValue(s[0]) * 17;
            uint8_t gv = hexValue(s[1]) * 17;
            uint8_t bv = hexValue(s[2]) * 17;
            return Color(rv, gv, bv);
        } else if (s.length() == 4) {
            uint8_t rv = hexValue(s[0]) * 17;
            uint8_t gv = hexValue(s[1]) * 17;
            uint8_t bv = hexValue(s[2]) * 17;
            uint8_t av = hexValue(s[3]) * 17;
            return Color(rv, gv, bv, av);
        } else if (s.length() == 6) {
            uint8_t rv = hexValue(s[0]) * 16 + hexValue(s[1]);
            uint8_t gv = hexValue(s[2]) * 16 + hexValue(s[3]);
            uint8_t bv = hexValue(s[4]) * 16 + hexValue(s[5]);
            return Color(rv, gv, bv);
        } else if (s.length() == 8) {
            uint8_t rv = hexValue(s[0]) * 16 + hexValue(s[1]);
            uint8_t gv = hexValue(s[2]) * 16 + hexValue(s[3]);
            uint8_t bv = hexValue(s[4]) * 16 + hexValue(s[5]);
            uint8_t av = hexValue(s[6]) * 16 + hexValue(s[7]);
            return Color(rv, gv, bv, av);
        }
    }
    
    // hsl() or hsla()
    std::string lower = toLower(s);
    if (lower.substr(0, 4) == "hsla" || lower.substr(0, 3) == "hsl") {
        size_t start = s.find('(');
        size_t end = s.find(')');
        if (start != std::string::npos && end != std::string::npos) {
            std::string values = s.substr(start + 1, end - start - 1);
            std::replace(values.begin(), values.end(), ',', ' ');
            // Remove '%' signs
            values.erase(std::remove(values.begin(), values.end(), '%'), values.end());
            std::istringstream iss(values);
            float h = 0, sv = 0, l = 0;
            float a = 1.0f;
            iss >> h >> sv >> l;
            sv /= 100.0f;
            l /= 100.0f;
            if (lower.substr(0, 4) == "hsla") {
                iss >> a;
            }
            return fromHSL(h, sv, l, static_cast<uint8_t>(std::clamp(a * 255.0f, 0.0f, 255.0f)));
        }
    }
    
    // rgb() or rgba()
    if (lower.substr(0, 4) == "rgba" || lower.substr(0, 3) == "rgb") {
        size_t start = s.find('(');
        size_t end = s.find(')');
        if (start != std::string::npos && end != std::string::npos) {
            std::string values = s.substr(start + 1, end - start - 1);
            std::replace(values.begin(), values.end(), ',', ' ');
            std::istringstream iss(values);
            int rv = 0, gv = 0, bv = 0;
            float a = 1.0f;
            iss >> rv >> gv >> bv;
            if (lower.substr(0, 4) == "rgba") {
                iss >> a;
            }
            return Color(
                static_cast<uint8_t>(std::clamp(rv, 0, 255)),
                static_cast<uint8_t>(std::clamp(gv, 0, 255)),
                static_cast<uint8_t>(std::clamp(bv, 0, 255)),
                static_cast<uint8_t>(std::clamp(a * 255.0f, 0.0f, 255.0f))
            );
        }
    }
    
    // Named color
    auto it = namedColors.find(toLower(s));
    if (it != namedColors.end()) {
        if (toLower(s) == "transparent") {
            return Color::transparent();
        }
        return Color(it->second);
    }
    
    return Color::black();
}

} // namespace NXRender
