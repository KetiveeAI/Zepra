// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file easing.h
 * @brief Easing functions for animations
 * 
 * Standard easing functions following CSS timing functions.
 * All functions take t in [0,1] and return value in [0,1].
 */

#pragma once

#include <cmath>
#include <functional>
#include <string>

namespace NXRender {

/**
 * @brief Easing function type
 */
using EasingFunction = std::function<float(float)>;

/**
 * @brief Standard easing functions
 */
namespace Easing {

// ============================================================================
// Linear
// ============================================================================
inline float linear(float t) { return t; }

// ============================================================================
// Quadratic
// ============================================================================
inline float easeInQuad(float t) { return t * t; }
inline float easeOutQuad(float t) { return t * (2 - t); }
inline float easeInOutQuad(float t) {
    return t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t;
}

// ============================================================================
// Cubic
// ============================================================================
inline float easeInCubic(float t) { return t * t * t; }
inline float easeOutCubic(float t) { float t1 = t - 1; return t1 * t1 * t1 + 1; }
inline float easeInOutCubic(float t) {
    return t < 0.5f ? 4 * t * t * t : (t - 1) * (2 * t - 2) * (2 * t - 2) + 1;
}

// ============================================================================
// Quartic
// ============================================================================
inline float easeInQuart(float t) { return t * t * t * t; }
inline float easeOutQuart(float t) { float t1 = t - 1; return 1 - t1 * t1 * t1 * t1; }
inline float easeInOutQuart(float t) {
    return t < 0.5f ? 8 * t * t * t * t : 1 - 8 * (t - 1) * (t - 1) * (t - 1) * (t - 1);
}

// ============================================================================
// Quintic
// ============================================================================
inline float easeInQuint(float t) { return t * t * t * t * t; }
inline float easeOutQuint(float t) { float t1 = t - 1; return 1 + t1 * t1 * t1 * t1 * t1; }
inline float easeInOutQuint(float t) {
    return t < 0.5f ? 16 * t * t * t * t * t : 1 + 16 * (t - 1) * (t - 1) * (t - 1) * (t - 1) * (t - 1);
}

// ============================================================================
// Sine
// ============================================================================
inline float easeInSine(float t) { return 1 - std::cos(t * 3.14159265f / 2); }
inline float easeOutSine(float t) { return std::sin(t * 3.14159265f / 2); }
inline float easeInOutSine(float t) { return 0.5f * (1 - std::cos(3.14159265f * t)); }

// ============================================================================
// Exponential
// ============================================================================
inline float easeInExpo(float t) { return t == 0 ? 0 : std::pow(2, 10 * (t - 1)); }
inline float easeOutExpo(float t) { return t == 1 ? 1 : 1 - std::pow(2, -10 * t); }
inline float easeInOutExpo(float t) {
    if (t == 0) return 0;
    if (t == 1) return 1;
    if (t < 0.5f) return 0.5f * std::pow(2, 20 * t - 10);
    return 1 - 0.5f * std::pow(2, -20 * t + 10);
}

// ============================================================================
// Circular
// ============================================================================
inline float easeInCirc(float t) { return 1 - std::sqrt(1 - t * t); }
inline float easeOutCirc(float t) { return std::sqrt(1 - (t - 1) * (t - 1)); }
inline float easeInOutCirc(float t) {
    return t < 0.5f
        ? 0.5f * (1 - std::sqrt(1 - 4 * t * t))
        : 0.5f * (std::sqrt(1 - (2 * t - 2) * (2 * t - 2)) + 1);
}

// ============================================================================
// Elastic
// ============================================================================
inline float easeInElastic(float t) {
    if (t == 0 || t == 1) return t;
    return -std::pow(2, 10 * (t - 1)) * std::sin((t - 1.1f) * 5 * 3.14159265f);
}
inline float easeOutElastic(float t) {
    if (t == 0 || t == 1) return t;
    return std::pow(2, -10 * t) * std::sin((t - 0.1f) * 5 * 3.14159265f) + 1;
}
inline float easeInOutElastic(float t) {
    if (t == 0 || t == 1) return t;
    t *= 2;
    if (t < 1) return -0.5f * std::pow(2, 10 * (t - 1)) * std::sin((t - 1.1f) * 5 * 3.14159265f);
    return 0.5f * std::pow(2, -10 * (t - 1)) * std::sin((t - 1.1f) * 5 * 3.14159265f) + 1;
}

// ============================================================================
// Back (overshoot)
// ============================================================================
constexpr float BACK_OVERSHOOT = 1.70158f;
inline float easeInBack(float t) { return t * t * ((BACK_OVERSHOOT + 1) * t - BACK_OVERSHOOT); }
inline float easeOutBack(float t) {
    float t1 = t - 1;
    return t1 * t1 * ((BACK_OVERSHOOT + 1) * t1 + BACK_OVERSHOOT) + 1;
}
inline float easeInOutBack(float t) {
    float s = BACK_OVERSHOOT * 1.525f;
    t *= 2;
    if (t < 1) return 0.5f * (t * t * ((s + 1) * t - s));
    t -= 2;
    return 0.5f * (t * t * ((s + 1) * t + s) + 2);
}

// ============================================================================
// Bounce
// ============================================================================
inline float easeOutBounce(float t) {
    if (t < 1 / 2.75f) return 7.5625f * t * t;
    if (t < 2 / 2.75f) { t -= 1.5f / 2.75f; return 7.5625f * t * t + 0.75f; }
    if (t < 2.5f / 2.75f) { t -= 2.25f / 2.75f; return 7.5625f * t * t + 0.9375f; }
    t -= 2.625f / 2.75f;
    return 7.5625f * t * t + 0.984375f;
}
inline float easeInBounce(float t) { return 1 - easeOutBounce(1 - t); }
inline float easeInOutBounce(float t) {
    return t < 0.5f ? 0.5f * easeInBounce(t * 2) : 0.5f * easeOutBounce(t * 2 - 1) + 0.5f;
}

// ============================================================================
// Cubic Bezier (CSS timing function)
// ============================================================================
inline float cubicBezier(float t, float x1, float y1, float x2, float y2) {
    // Newton-Raphson to solve for t given x
    auto sampleCurveX = [x1, x2](float t) {
        return 3 * x1 * t * (1 - t) * (1 - t) + 3 * x2 * t * t * (1 - t) + t * t * t;
    };
    auto sampleCurveY = [y1, y2](float t) {
        return 3 * y1 * t * (1 - t) * (1 - t) + 3 * y2 * t * t * (1 - t) + t * t * t;
    };
    
    float guess = t;
    for (int i = 0; i < 8; i++) {
        float x = sampleCurveX(guess) - t;
        if (std::abs(x) < 0.001f) break;
        float dx = 3 * x1 * (1 - guess) * (1 - guess) + 6 * (x2 - x1) * guess * (1 - guess) + 3 * x2 * guess * guess;
        if (dx == 0) break;
        guess -= x / dx;
    }
    return sampleCurveY(guess);
}

// CSS preset cubic-beziers
inline float ease(float t) { return cubicBezier(t, 0.25f, 0.1f, 0.25f, 1.0f); }
inline float easeIn(float t) { return cubicBezier(t, 0.42f, 0.0f, 1.0f, 1.0f); }
inline float easeOut(float t) { return cubicBezier(t, 0.0f, 0.0f, 0.58f, 1.0f); }
inline float easeInOut(float t) { return cubicBezier(t, 0.42f, 0.0f, 0.58f, 1.0f); }

// ============================================================================
// Get easing by name (for CSS parsing)
// ============================================================================
inline EasingFunction getByName(const std::string& name) {
    if (name == "linear") return linear;
    if (name == "ease") return ease;
    if (name == "ease-in") return easeIn;
    if (name == "ease-out") return easeOut;
    if (name == "ease-in-out") return easeInOut;
    if (name == "ease-in-quad") return easeInQuad;
    if (name == "ease-out-quad") return easeOutQuad;
    if (name == "ease-in-cubic") return easeInCubic;
    if (name == "ease-out-cubic") return easeOutCubic;
    if (name == "ease-in-elastic") return easeInElastic;
    if (name == "ease-out-elastic") return easeOutElastic;
    if (name == "ease-in-bounce") return easeInBounce;
    if (name == "ease-out-bounce") return easeOutBounce;
    return linear;  // Default
}

} // namespace Easing
} // namespace NXRender
