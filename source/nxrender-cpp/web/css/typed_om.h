// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "css_math.h"
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <unordered_map>

namespace NXRender {
namespace Web {

// ==================================================================
// CSS Typed OM — CSSStyleValue hierarchy
// ==================================================================

enum class CSSStyleValueType : uint8_t {
    Unknown,
    Keyword,
    Number,
    Percentage,
    Length,
    Angle,
    Time,
    Resolution,
    Color,
    Image,
    Transform,
    Position,
    URLImage,
    Gradient,
    Unparsed,
    Math,
};

class CSSStyleValue {
public:
    virtual ~CSSStyleValue() = default;
    virtual CSSStyleValueType type() const = 0;
    virtual std::string toString() const = 0;

    // Parse a CSS value into typed representation
    static std::unique_ptr<CSSStyleValue> parse(const std::string& property,
                                                  const std::string& value);
};

// ==================================================================
// CSSKeywordValue
// ==================================================================

class CSSKeywordValue : public CSSStyleValue {
public:
    explicit CSSKeywordValue(const std::string& value) : value_(value) {}
    CSSStyleValueType type() const override { return CSSStyleValueType::Keyword; }
    std::string toString() const override { return value_; }

    const std::string& value() const { return value_; }

private:
    std::string value_;
};

// ==================================================================
// CSSNumericValue — base for numeric types
// ==================================================================

class CSSNumericValue : public CSSStyleValue {
public:
    float value() const { return value_; }
    CSSUnit unit() const { return unit_; }

    CSSStyleValueType type() const override;
    std::string toString() const override;

    // Arithmetic
    std::unique_ptr<CSSNumericValue> add(const CSSNumericValue& other) const;
    std::unique_ptr<CSSNumericValue> sub(const CSSNumericValue& other) const;
    std::unique_ptr<CSSNumericValue> mul(float factor) const;
    std::unique_ptr<CSSNumericValue> div(float divisor) const;
    std::unique_ptr<CSSNumericValue> negate() const;

    // Unit conversion
    std::unique_ptr<CSSNumericValue> to(CSSUnit targetUnit) const;

    static std::unique_ptr<CSSNumericValue> create(float value, CSSUnit unit);

protected:
    float value_ = 0;
    CSSUnit unit_ = CSSUnit::Px;
};

class CSSUnitVal : public CSSNumericValue {
public:
    CSSUnitVal(float v, CSSUnit u) { value_ = v; unit_ = u; }
};

// ==================================================================
// CSSMathValue — wraps calc() expressions
// ==================================================================

class CSSMathValue : public CSSStyleValue {
public:
    CSSMathValue(std::unique_ptr<CSSMathExpr> expr) : expr_(std::move(expr)) {}
    CSSStyleValueType type() const override { return CSSStyleValueType::Math; }
    std::string toString() const override;

    const CSSMathExpr& expression() const { return *expr_; }
    float resolve(const CSSCalcEngine::Context& ctx) const;

private:
    std::unique_ptr<CSSMathExpr> expr_;
};

// ==================================================================
// CSSColorValue (Typed OM wrapper)
// ==================================================================

class CSSColorVal : public CSSStyleValue {
public:
    CSSColorVal(const CSSColorValue& color) : color_(color) {}
    CSSStyleValueType type() const override { return CSSStyleValueType::Color; }
    std::string toString() const override { return color_.toRGBString(); }

    const CSSColorValue& color() const { return color_; }

    CSSColorValue color_;
};

// ==================================================================
// CSSTransformValue
// ==================================================================

struct CSSTransformComponent {
    enum class Type : uint8_t {
        Translate, TranslateX, TranslateY, TranslateZ, Translate3D,
        Rotate, RotateX, RotateY, RotateZ, Rotate3D,
        Scale, ScaleX, ScaleY, ScaleZ, Scale3D,
        Skew, SkewX, SkewY,
        Matrix, Matrix3D,
        Perspective,
    } type;

    std::vector<float> values;

    // Matrix representation
    void toMatrix4x4(float out[16]) const;
    std::string toString() const;
};

class CSSTransformValue : public CSSStyleValue {
public:
    CSSStyleValueType type() const override { return CSSStyleValueType::Transform; }
    std::string toString() const override;

    void addComponent(const CSSTransformComponent& comp) { components_.push_back(comp); }
    const std::vector<CSSTransformComponent>& components() const { return components_; }

    // Compute combined 4x4 matrix
    void toMatrix4x4(float out[16]) const;

    // Interpolation
    static CSSTransformValue interpolate(const CSSTransformValue& from,
                                           const CSSTransformValue& to, float t);

    // Parse "translate(10px, 20px) rotate(45deg)" etc.
    static CSSTransformValue parse(const std::string& value);

private:
    std::vector<CSSTransformComponent> components_;
};

// ==================================================================
// CSSPositionValue
// ==================================================================

class CSSPositionValue : public CSSStyleValue {
public:
    CSSPositionValue(const CSSUnitValue& x, const CSSUnitValue& y) : x_(x), y_(y) {}
    CSSStyleValueType type() const override { return CSSStyleValueType::Position; }
    std::string toString() const override;

    const CSSUnitValue& x() const { return x_; }
    const CSSUnitValue& y() const { return y_; }

    static CSSPositionValue parse(const std::string& value);

private:
    CSSUnitValue x_, y_;
};

// ==================================================================
// CSSURLImageValue
// ==================================================================

class CSSURLImageValue : public CSSStyleValue {
public:
    explicit CSSURLImageValue(const std::string& url) : url_(url) {}
    CSSStyleValueType type() const override { return CSSStyleValueType::URLImage; }
    std::string toString() const override { return "url(\"" + url_ + "\")"; }

    const std::string& url() const { return url_; }

private:
    std::string url_;
};

// ==================================================================
// CSSUnparsedValue — preserves raw CSS text + var() refs
// ==================================================================

class CSSUnparsedValue : public CSSStyleValue {
public:
    struct Fragment {
        enum class Type { String, Variable } type;
        std::string value;
        std::string fallback; // for variables
    };

    CSSStyleValueType type() const override { return CSSStyleValueType::Unparsed; }
    std::string toString() const override;

    void addString(const std::string& s);
    void addVariable(const std::string& name, const std::string& fallback = "");

    const std::vector<Fragment>& fragments() const { return fragments_; }

private:
    std::vector<Fragment> fragments_;
};

// ==================================================================
// StylePropertyMap — element.computedStyleMap()
// ==================================================================

class StylePropertyMap {
public:
    void set(const std::string& property, std::unique_ptr<CSSStyleValue> value);
    CSSStyleValue* get(const std::string& property) const;
    bool has(const std::string& property) const;
    void remove(const std::string& property);
    void clear();

    std::vector<std::string> properties() const;
    size_t size() const { return map_.size(); }

    // Append to shorthand expansion
    void append(const std::string& property, std::unique_ptr<CSSStyleValue> value);

    // Get all values for a property (for multi-value properties like background)
    std::vector<CSSStyleValue*> getAll(const std::string& property) const;

private:
    std::unordered_map<std::string, std::vector<std::unique_ptr<CSSStyleValue>>> map_;
};

// ==================================================================
// CSS Property registration for Typed OM
// ==================================================================

struct CSSPropertyDefinition {
    std::string name;
    CSSStyleValueType expectedType = CSSStyleValueType::Unknown;
    bool inherits = false;
    std::string initialValue;
    std::string syntax; // "*", "<length>", "<color>", etc.
    bool animatable = false;
};

class CSSPropertyRegistry {
public:
    static CSSPropertyRegistry& instance();

    void registerProperty(const CSSPropertyDefinition& def);
    const CSSPropertyDefinition* find(const std::string& name) const;
    bool isAnimatable(const std::string& name) const;
    CSSStyleValueType expectedType(const std::string& name) const;

    // Built-in property registration
    void registerBuiltins();

private:
    CSSPropertyRegistry() = default;
    std::unordered_map<std::string, CSSPropertyDefinition> props_;
};

} // namespace Web
} // namespace NXRender
