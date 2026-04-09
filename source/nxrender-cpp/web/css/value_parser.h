// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "property_ids.h"
#include <string>
#include <vector>
#include <optional>

namespace NXRender {
namespace Web {

// ==================================================================
// CSS Value Parser — tokenizes and resolves CSS values
// ==================================================================

struct ComputedLength {
    float px = 0;

    static ComputedLength resolve(float value, CSSLengthUnit unit,
                                   float fontSize, float rootFontSize,
                                   float viewportWidth, float viewportHeight,
                                   float containerSize = 0);
};

struct ComputedColor {
    uint8_t r = 0, g = 0, b = 0, a = 255;

    uint32_t toRGBA() const { return (r << 24) | (g << 16) | (b << 8) | a; }
    uint32_t toARGB() const { return (a << 24) | (r << 16) | (g << 8) | b; }

    static ComputedColor fromHex(const std::string& hex);
    static ComputedColor fromRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    static ComputedColor fromHSL(float h, float s, float l, float a = 1.0f);
    static ComputedColor fromName(const std::string& name);
    static std::optional<ComputedColor> parse(const std::string& str);
};

// ==================================================================
// Gradient
// ==================================================================

struct GradientStop {
    ComputedColor color;
    float position;         // 0.0 - 1.0, or -1 for auto
    bool hasPosition;
};

struct Gradient {
    enum class Type { Linear, Radial, Conic } type = Type::Linear;
    float angle = 180.0f;   // degrees for linear
    std::string direction;  // "to bottom", "to right", etc.
    std::vector<GradientStop> stops;
    bool repeating = false;
};

// ==================================================================
// Transform function
// ==================================================================

struct TransformFunction {
    enum class Type {
        Translate, TranslateX, TranslateY, TranslateZ, Translate3D,
        Scale, ScaleX, ScaleY, ScaleZ, Scale3D,
        Rotate, RotateX, RotateY, RotateZ, Rotate3D,
        Skew, SkewX, SkewY,
        Matrix, Matrix3D,
        Perspective,
    } type;
    std::vector<float> args;
};

// ==================================================================
// Box shadow
// ==================================================================

struct BoxShadowValue {
    float offsetX = 0, offsetY = 0;
    float blurRadius = 0;
    float spreadRadius = 0;
    ComputedColor color;
    bool inset = false;
};

// ==================================================================
// Parser
// ==================================================================

class CSSValueParser {
public:
    CSSValueParser() = default;

    // Parse a single CSS value for a given property
    CSSValue parseValue(CSSPropertyID property, const std::string& input);

    // Parse specific value types
    static CSSValue parseLength(const std::string& input);
    static CSSValue parseColor(const std::string& input);
    static CSSValue parseNumber(const std::string& input);

    // Parse complex values
    static std::optional<ComputedColor> parseColorValue(const std::string& input);
    static std::optional<Gradient> parseGradient(const std::string& input);
    static std::vector<TransformFunction> parseTransform(const std::string& input);
    static std::vector<BoxShadowValue> parseBoxShadow(const std::string& input);

    // Length resolution
    static ComputedLength resolveLength(const CSSValue& value,
                                         float fontSize, float rootFontSize,
                                         float viewportWidth, float viewportHeight);

    // Shorthand expansion
    static std::vector<std::pair<CSSPropertyID, CSSValue>>
        expandShorthand(CSSPropertyID shorthand, const std::string& value);

private:
    static float parseFloat(const std::string& s, size_t& pos);
    static std::string parseIdent(const std::string& s, size_t& pos);
    static void skipWhitespace(const std::string& s, size_t& pos);
    static bool matchChar(const std::string& s, size_t& pos, char c);
};

} // namespace Web
} // namespace NXRender
