/**
 * @file MathAPI.h
 * @brief Math Implementation
 */

#pragma once

#include <cmath>
#include <algorithm>
#include <random>
#include <limits>
#include <cstdint>

namespace Zepra::Runtime {

class Math {
public:
    // Constants
    static constexpr double E = 2.718281828459045;
    static constexpr double LN10 = 2.302585092994046;
    static constexpr double LN2 = 0.6931471805599453;
    static constexpr double LOG10E = 0.4342944819032518;
    static constexpr double LOG2E = 1.4426950408889634;
    static constexpr double PI = 3.141592653589793;
    static constexpr double SQRT1_2 = 0.7071067811865476;
    static constexpr double SQRT2 = 1.4142135623730951;
    
    // Basic operations
    static double abs(double x) { return std::abs(x); }
    static double ceil(double x) { return std::ceil(x); }
    static double floor(double x) { return std::floor(x); }
    static double round(double x) { return std::round(x); }
    static double trunc(double x) { return std::trunc(x); }
    static double sign(double x) { return (x > 0) - (x < 0); }
    
    // Power and roots
    static double sqrt(double x) { return std::sqrt(x); }
    static double cbrt(double x) { return std::cbrt(x); }
    static double pow(double base, double exp) { return std::pow(base, exp); }
    static double hypot(double x, double y) { return std::hypot(x, y); }
    
    // Exponential and logarithmic
    static double exp(double x) { return std::exp(x); }
    static double expm1(double x) { return std::expm1(x); }
    static double log(double x) { return std::log(x); }
    static double log10(double x) { return std::log10(x); }
    static double log2(double x) { return std::log2(x); }
    static double log1p(double x) { return std::log1p(x); }
    
    // Trigonometric
    static double sin(double x) { return std::sin(x); }
    static double cos(double x) { return std::cos(x); }
    static double tan(double x) { return std::tan(x); }
    static double asin(double x) { return std::asin(x); }
    static double acos(double x) { return std::acos(x); }
    static double atan(double x) { return std::atan(x); }
    static double atan2(double y, double x) { return std::atan2(y, x); }
    
    // Hyperbolic
    static double sinh(double x) { return std::sinh(x); }
    static double cosh(double x) { return std::cosh(x); }
    static double tanh(double x) { return std::tanh(x); }
    static double asinh(double x) { return std::asinh(x); }
    static double acosh(double x) { return std::acosh(x); }
    static double atanh(double x) { return std::atanh(x); }
    
    // Min/Max
    static double min(double a, double b) { return std::min(a, b); }
    static double max(double a, double b) { return std::max(a, b); }
    
    template<typename... Args>
    static double min(double first, Args... rest) {
        return std::min(first, min(rest...));
    }
    
    template<typename... Args>
    static double max(double first, Args... rest) {
        return std::max(first, max(rest...));
    }
    
    // Random
    static double random() {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        static std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(gen);
    }
    
    // ES6+ additions
    static int32_t imul(int32_t a, int32_t b) {
        return static_cast<int32_t>(static_cast<uint32_t>(a) * static_cast<uint32_t>(b));
    }
    
    static float fround(double x) { return static_cast<float>(x); }
    
    static int clz32(uint32_t x) {
        if (x == 0) return 32;
        int n = 0;
        if ((x & 0xFFFF0000) == 0) { n += 16; x <<= 16; }
        if ((x & 0xFF000000) == 0) { n += 8; x <<= 8; }
        if ((x & 0xF0000000) == 0) { n += 4; x <<= 4; }
        if ((x & 0xC0000000) == 0) { n += 2; x <<= 2; }
        if ((x & 0x80000000) == 0) { n += 1; }
        return n;
    }
};

} // namespace Zepra::Runtime
