// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file color.h
 * @brief Color types and utilities for NXRender
 */

#pragma once

#include <cstdint>
#include <string>
#include <algorithm>

namespace NXRender {

/**
 * @brief RGBA Color
 */
struct Color {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;
    
    constexpr Color() = default;
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}
    
    // Construct from hex (0xRRGGBB or 0xAARRGGBB)
    constexpr explicit Color(uint32_t hex, bool hasAlpha = false) {
        if (hasAlpha) {
            a = (hex >> 24) & 0xFF;
            r = (hex >> 16) & 0xFF;
            g = (hex >> 8) & 0xFF;
            b = hex & 0xFF;
        } else {
            r = (hex >> 16) & 0xFF;
            g = (hex >> 8) & 0xFF;
            b = hex & 0xFF;
            a = 255;
        }
    }
    
    // Convert to 0xAARRGGBB
    constexpr uint32_t toARGB() const {
        return (static_cast<uint32_t>(a) << 24) |
               (static_cast<uint32_t>(r) << 16) |
               (static_cast<uint32_t>(g) << 8) |
               static_cast<uint32_t>(b);
    }
    
    // Convert to 0xRRGGBBAA
    constexpr uint32_t toRGBA() const {
        return (static_cast<uint32_t>(r) << 24) |
               (static_cast<uint32_t>(g) << 16) |
               (static_cast<uint32_t>(b) << 8) |
               static_cast<uint32_t>(a);
    }
    
    // Blend with another color
    Color blend(const Color& other, float t) const {
        return Color(
            static_cast<uint8_t>(r + (other.r - r) * t),
            static_cast<uint8_t>(g + (other.g - g) * t),
            static_cast<uint8_t>(b + (other.b - b) * t),
            static_cast<uint8_t>(a + (other.a - a) * t)
        );
    }
    
    // Darken
    Color darken(float amount) const {
        float factor = 1.0f - std::clamp(amount, 0.0f, 1.0f);
        return Color(
            static_cast<uint8_t>(r * factor),
            static_cast<uint8_t>(g * factor),
            static_cast<uint8_t>(b * factor),
            a
        );
    }
    
    // Lighten
    Color lighten(float amount) const {
        float factor = std::clamp(amount, 0.0f, 1.0f);
        return Color(
            static_cast<uint8_t>(r + (255 - r) * factor),
            static_cast<uint8_t>(g + (255 - g) * factor),
            static_cast<uint8_t>(b + (255 - b) * factor),
            a
        );
    }
    
    // With alpha
    constexpr Color withAlpha(uint8_t newAlpha) const {
        return Color(r, g, b, newAlpha);
    }
    
    // Common colors
    static constexpr Color black() { return Color(0, 0, 0); }
    static constexpr Color white() { return Color(255, 255, 255); }
    static constexpr Color red() { return Color(255, 0, 0); }
    static constexpr Color green() { return Color(0, 255, 0); }
    static constexpr Color blue() { return Color(0, 0, 255); }
    static constexpr Color transparent() { return Color(0, 0, 0, 0); }
    
    // Parse from string (#RGB, #RRGGBB, #AARRGGBB, rgb(...), rgba(...))
    static Color parse(const std::string& str);
};

// Equality
inline constexpr bool operator==(const Color& a, const Color& b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

inline constexpr bool operator!=(const Color& a, const Color& b) {
    return !(a == b);
}

} // namespace NXRender
