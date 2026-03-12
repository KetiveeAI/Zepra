// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file color.cpp
 * @brief Color implementation
 */

#include "nxgfx/color.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace NXRender {

// Named color lookup table
static const std::unordered_map<std::string, uint32_t> namedColors = {
    {"black", 0x000000}, {"white", 0xFFFFFF}, {"red", 0xFF0000},
    {"green", 0x00FF00}, {"blue", 0x0000FF}, {"yellow", 0xFFFF00},
    {"cyan", 0x00FFFF}, {"magenta", 0xFF00FF}, {"orange", 0xFFA500},
    {"purple", 0x800080}, {"pink", 0xFFC0CB}, {"gray", 0x808080},
    {"grey", 0x808080}, {"silver", 0xC0C0C0}, {"maroon", 0x800000},
    {"navy", 0x000080}, {"olive", 0x808000}, {"teal", 0x008080},
    {"aqua", 0x00FFFF}, {"fuchsia", 0xFF00FF}, {"lime", 0x00FF00},
    {"transparent", 0x00000000}
};

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

Color Color::parse(const std::string& str) {
    if (str.empty()) return Color::black();
    
    std::string s = str;
    // Trim whitespace
    while (!s.empty() && std::isspace(s.front())) s.erase(s.begin());
    while (!s.empty() && std::isspace(s.back())) s.pop_back();
    
    // Hex color
    if (s[0] == '#') {
        s = s.substr(1);
        if (s.length() == 3) {
            // #RGB -> #RRGGBB
            uint8_t r = hexValue(s[0]) * 17;
            uint8_t g = hexValue(s[1]) * 17;
            uint8_t b = hexValue(s[2]) * 17;
            return Color(r, g, b);
        } else if (s.length() == 4) {
            // #RGBA
            uint8_t r = hexValue(s[0]) * 17;
            uint8_t g = hexValue(s[1]) * 17;
            uint8_t b = hexValue(s[2]) * 17;
            uint8_t a = hexValue(s[3]) * 17;
            return Color(r, g, b, a);
        } else if (s.length() == 6) {
            // #RRGGBB
            uint8_t r = hexValue(s[0]) * 16 + hexValue(s[1]);
            uint8_t g = hexValue(s[2]) * 16 + hexValue(s[3]);
            uint8_t b = hexValue(s[4]) * 16 + hexValue(s[5]);
            return Color(r, g, b);
        } else if (s.length() == 8) {
            // #RRGGBBAA
            uint8_t r = hexValue(s[0]) * 16 + hexValue(s[1]);
            uint8_t g = hexValue(s[2]) * 16 + hexValue(s[3]);
            uint8_t b = hexValue(s[4]) * 16 + hexValue(s[5]);
            uint8_t a = hexValue(s[6]) * 16 + hexValue(s[7]);
            return Color(r, g, b, a);
        }
    }
    
    // rgb() or rgba()
    if (s.substr(0, 4) == "rgba" || s.substr(0, 3) == "rgb") {
        size_t start = s.find('(');
        size_t end = s.find(')');
        if (start != std::string::npos && end != std::string::npos) {
            std::string values = s.substr(start + 1, end - start - 1);
            std::replace(values.begin(), values.end(), ',', ' ');
            std::istringstream iss(values);
            int r = 0, g = 0, b = 0;
            float a = 1.0f;
            iss >> r >> g >> b;
            if (s.substr(0, 4) == "rgba") {
                iss >> a;
            }
            return Color(
                static_cast<uint8_t>(std::clamp(r, 0, 255)),
                static_cast<uint8_t>(std::clamp(g, 0, 255)),
                static_cast<uint8_t>(std::clamp(b, 0, 255)),
                static_cast<uint8_t>(std::clamp(a * 255.0f, 0.0f, 255.0f))
            );
        }
    }
    
    // Named color
    auto it = namedColors.find(toLower(s));
    if (it != namedColors.end()) {
        if (s == "transparent") {
            return Color::transparent();
        }
        return Color(it->second);
    }
    
    return Color::black();
}

} // namespace NXRender
