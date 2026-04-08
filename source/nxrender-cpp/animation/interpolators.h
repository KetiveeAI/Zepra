// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file interpolators.h
 * @brief Type-specific interpolation for colors, rects, radii, and edge insets.
 */

#pragma once

#include "../nxgfx/color.h"
#include "../nxgfx/primitives.h"
#include <cmath>
#include <algorithm>

namespace NXRender {

/**
 * @brief Interpolate between two colors in linear RGB space.
 */
inline Color interpolateColor(const Color& a, const Color& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return Color(
        static_cast<uint8_t>(a.r + (b.r - a.r) * t),
        static_cast<uint8_t>(a.g + (b.g - a.g) * t),
        static_cast<uint8_t>(a.b + (b.b - a.b) * t),
        static_cast<uint8_t>(a.a + (b.a - a.a) * t)
    );
}

/**
 * @brief Interpolate between two colors in perceptual LAB space.
 *
 * LAB interpolation produces more visually uniform transitions than RGB,
 * especially between colors far apart on the color wheel.
 */
inline Color interpolateColorLab(const Color& a, const Color& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);

    // Convert sRGB to linear RGB
    auto toLinear = [](float srgb) -> float {
        if (srgb <= 0.04045f) return srgb / 12.92f;
        return std::pow((srgb + 0.055f) / 1.055f, 2.4f);
    };

    auto fromLinear = [](float linear) -> float {
        if (linear <= 0.0031308f) return linear * 12.92f;
        return 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
    };

    // RGB → XYZ (D65 illuminant)
    auto toXYZ = [&toLinear](const Color& c, float& x, float& y, float& z) {
        float r = toLinear(c.r / 255.0f);
        float g = toLinear(c.g / 255.0f);
        float bl = toLinear(c.b / 255.0f);
        x = 0.4124564f * r + 0.3575761f * g + 0.1804375f * bl;
        y = 0.2126729f * r + 0.7151522f * g + 0.0721750f * bl;
        z = 0.0193339f * r + 0.1191920f * g + 0.9503041f * bl;
    };

    // XYZ → LAB
    auto f = [](float t_val) -> float {
        const float delta = 6.0f / 29.0f;
        if (t_val > delta * delta * delta)
            return std::cbrt(t_val);
        return t_val / (3.0f * delta * delta) + 4.0f / 29.0f;
    };

    auto toLab = [&toXYZ, &f](const Color& c, float& L, float& A, float& B) {
        float x, y, z;
        toXYZ(c, x, y, z);
        // D65 reference white
        float xn = 0.95047f, yn = 1.0f, zn = 1.08883f;
        L = 116.0f * f(y / yn) - 16.0f;
        A = 500.0f * (f(x / xn) - f(y / yn));
        B = 200.0f * (f(y / yn) - f(z / zn));
    };

    // LAB → XYZ → RGB
    auto finverse = [](float t_val) -> float {
        const float delta = 6.0f / 29.0f;
        if (t_val > delta)
            return t_val * t_val * t_val;
        return 3.0f * delta * delta * (t_val - 4.0f / 29.0f);
    };

    auto fromLab = [&finverse, &fromLinear](float L, float A, float B) -> Color {
        float xn = 0.95047f, yn = 1.0f, zn = 1.08883f;
        float fy = (L + 16.0f) / 116.0f;
        float fx = A / 500.0f + fy;
        float fz = fy - B / 200.0f;

        float x = xn * finverse(fx);
        float y = yn * finverse(fy);
        float z = zn * finverse(fz);

        // XYZ → linear RGB
        float r =  3.2404542f * x - 1.5371385f * y - 0.4985314f * z;
        float g = -0.9692660f * x + 1.8760108f * y + 0.0415560f * z;
        float bl =  0.0556434f * x - 0.2040259f * y + 1.0572252f * z;

        r = std::clamp(fromLinear(r), 0.0f, 1.0f);
        g = std::clamp(fromLinear(g), 0.0f, 1.0f);
        bl = std::clamp(fromLinear(bl), 0.0f, 1.0f);

        return Color(
            static_cast<uint8_t>(r * 255.0f),
            static_cast<uint8_t>(g * 255.0f),
            static_cast<uint8_t>(bl * 255.0f)
        );
    };

    float La, Aa, Ba, Lb, Ab, Bb;
    toLab(a, La, Aa, Ba);
    toLab(b, Lb, Ab, Bb);

    float L = La + (Lb - La) * t;
    float A_interp = Aa + (Ab - Aa) * t;
    float B_interp = Ba + (Bb - Ba) * t;

    Color result = fromLab(L, A_interp, B_interp);
    result.a = static_cast<uint8_t>(a.a + (b.a - a.a) * t);
    return result;
}

/**
 * @brief Interpolate between two rectangles.
 */
inline Rect interpolateRect(const Rect& a, const Rect& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return Rect(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.width + (b.width - a.width) * t,
        a.height + (b.height - a.height) * t
    );
}

/**
 * @brief Interpolate between two corner radii.
 */
inline CornerRadii interpolateCornerRadii(const CornerRadii& a, const CornerRadii& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return CornerRadii(
        a.topLeft + (b.topLeft - a.topLeft) * t,
        a.topRight + (b.topRight - a.topRight) * t,
        a.bottomRight + (b.bottomRight - a.bottomRight) * t,
        a.bottomLeft + (b.bottomLeft - a.bottomLeft) * t
    );
}

/**
 * @brief Interpolate between two edge insets.
 */
inline EdgeInsets interpolateEdgeInsets(const EdgeInsets& a, const EdgeInsets& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return EdgeInsets(
        a.top + (b.top - a.top) * t,
        a.right + (b.right - a.right) * t,
        a.bottom + (b.bottom - a.bottom) * t,
        a.left + (b.left - a.left) * t
    );
}

/**
 * @brief Interpolate a float value.
 */
inline float interpolateFloat(float a, float b, float t) {
    return a + (b - a) * std::clamp(t, 0.0f, 1.0f);
}

/**
 * @brief Interpolate a point.
 */
inline Point interpolatePoint(const Point& a, const Point& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return Point(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}

/**
 * @brief Interpolate a size.
 */
inline Size interpolateSize(const Size& a, const Size& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return Size(a.width + (b.width - a.width) * t, a.height + (b.height - a.height) * t);
}

} // namespace NXRender
