// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include <memory>

namespace NXRender {
namespace Web {

// ==================================================================
// CSS Math Expressions — calc(), min(), max(), clamp()
// ==================================================================

enum class CSSUnit : uint8_t {
    None,
    Px, Em, Rem, Percent,
    Vw, Vh, Vmin, Vmax,
    Ch, Ex, Ic, Lh, Rlh,
    Cm, Mm, In, Pt, Pc, Q,
    Svw, Svh, Lvw, Lvh, Dvw, Dvh,
    Fr, // Grid fraction
    Deg, Rad, Grad, Turn, // Angles
    S, Ms, // Time
    Dpi, Dpcm, Dppx, // Resolution
    Number,
};

struct CSSUnitValue {
    float value = 0;
    CSSUnit unit = CSSUnit::Px;

    float toPx(float fontSize, float rootFontSize, float viewWidth, float viewHeight,
               float parentSize = 0) const;

    static CSSUnit parseUnit(const std::string& str);
    static CSSUnitValue parse(const std::string& str);
    static bool isAbsolute(CSSUnit u);
    static bool isRelative(CSSUnit u);
    static bool isAngle(CSSUnit u);
    static bool isTime(CSSUnit u);
};

// Expression tree for calc()
struct CSSMathExpr {
    enum class Op : uint8_t {
        Value, Add, Subtract, Multiply, Divide,
        Min, Max, Clamp,
        Negate, Abs, Sign, Round, Mod, Rem,
        Sin, Cos, Tan, Asin, Acos, Atan, Atan2,
        Pow, Sqrt, Hypot, Log, Exp,
    };

    Op op = Op::Value;
    CSSUnitValue value;
    std::vector<std::unique_ptr<CSSMathExpr>> children;

    static std::unique_ptr<CSSMathExpr> makeValue(float v, CSSUnit u = CSSUnit::Px);
    static std::unique_ptr<CSSMathExpr> makeBinary(Op op,
                                                     std::unique_ptr<CSSMathExpr> lhs,
                                                     std::unique_ptr<CSSMathExpr> rhs);
    static std::unique_ptr<CSSMathExpr> makeFunction(Op op,
                                                       std::vector<std::unique_ptr<CSSMathExpr>> args);

    CSSMathExpr clone() const;
};

// ==================================================================
// CSS calc() parser and evaluator
// ==================================================================

class CSSCalcEngine {
public:
    struct Context {
        float fontSize = 16;        // em base
        float rootFontSize = 16;    // rem base
        float viewportWidth = 1920;
        float viewportHeight = 1080;
        float parentSize = 0;       // for percentage resolution
        float containerWidth = 0;   // cqw base
        float containerHeight = 0;  // cqh base
    };

    // Parse calc(), min(), max(), clamp() expressions
    static std::unique_ptr<CSSMathExpr> parse(const std::string& expr);

    // Evaluate to a concrete pixel value
    static float evaluate(const CSSMathExpr& expr, const Context& ctx);

    // Check if a string contains a math function
    static bool isMathFunction(const std::string& value);

    // Simplify expression tree (constant folding)
    static std::unique_ptr<CSSMathExpr> simplify(std::unique_ptr<CSSMathExpr> expr);

    // Serialize back to CSS string
    static std::string serialize(const CSSMathExpr& expr);

private:
    struct Parser {
        const std::string& input;
        size_t pos = 0;

        std::unique_ptr<CSSMathExpr> parseExpression();
        std::unique_ptr<CSSMathExpr> parseTerm();
        std::unique_ptr<CSSMathExpr> parseFactor();
        std::unique_ptr<CSSMathExpr> parseAtom();
        std::unique_ptr<CSSMathExpr> parseFunction(const std::string& name);

        void skipWhitespace();
        bool match(char c);
        bool peek(char c) const;
        std::string readNumber();
        std::string readUnit();
        std::string readIdentifier();
    };

    static float resolveUnit(const CSSUnitValue& val, const Context& ctx);
};

// ==================================================================
// CSS Color math — color-mix(), relative colors
// ==================================================================

struct CSSColorValue {
    float r = 0, g = 0, b = 0, a = 1; // Linear sRGB 0-1

    static CSSColorValue fromHex(const std::string& hex);
    static CSSColorValue fromRGB(int r, int g, int b, float a = 1);
    static CSSColorValue fromHSL(float h, float s, float l, float a = 1);
    static CSSColorValue fromHWB(float h, float w, float b, float a = 1);
    static CSSColorValue fromOKLCH(float l, float c, float h, float a = 1);
    static CSSColorValue fromOKLAB(float l, float a_axis, float b_axis, float alpha = 1);

    // Convert
    void toHSL(float& h, float& s, float& l) const;
    void toOKLCH(float& ll, float& c, float& h) const;
    std::string toHexString() const;
    std::string toRGBString() const;

    // color-mix()
    static CSSColorValue mix(const CSSColorValue& c1, const CSSColorValue& c2,
                              float amount, const std::string& colorSpace = "srgb");

    // Interpolation
    static CSSColorValue interpolate(const CSSColorValue& from, const CSSColorValue& to,
                                       float t, const std::string& colorSpace = "oklch");

    // Named colors
    static CSSColorValue fromName(const std::string& name);
    static bool isNamedColor(const std::string& name);

    // Parse any CSS color value
    static CSSColorValue parse(const std::string& str);
};

} // namespace Web
} // namespace NXRender
