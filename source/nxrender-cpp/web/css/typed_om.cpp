// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "typed_om.h"
#include "custom_properties.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <cstring>

namespace NXRender {
namespace Web {

// ==================================================================
// CSSStyleValue::parse
// ==================================================================

std::unique_ptr<CSSStyleValue> CSSStyleValue::parse(const std::string& property,
                                                       const std::string& value) {
    if (value.empty()) return nullptr;

    // Check for math functions
    if (CSSCalcEngine::isMathFunction(value)) {
        auto expr = CSSCalcEngine::parse(value);
        if (expr) return std::make_unique<CSSMathValue>(std::move(expr));
    }

    // Check for var()
    if (CSSVariableResolver::containsVar(value)) {
        auto unparsed = std::make_unique<CSSUnparsedValue>();
        auto refs = CSSVariableResolver::extractVarReferences(value);
        for (const auto& ref : refs) {
            unparsed->addVariable(ref.name, ref.fallback);
        }
        return unparsed;
    }

    // Check for url()
    if (value.find("url(") == 0) {
        auto paren = value.find('(');
        auto end = value.rfind(')');
        if (paren != std::string::npos && end != std::string::npos) {
            std::string url = value.substr(paren + 1, end - paren - 1);
            // Strip quotes
            if (url.size() >= 2 && (url.front() == '"' || url.front() == '\'')) {
                url = url.substr(1, url.size() - 2);
            }
            return std::make_unique<CSSURLImageValue>(url);
        }
    }

    // Check for color
    if (value[0] == '#' || value.find("rgb") == 0 || value.find("hsl") == 0 ||
        CSSColorValue::isNamedColor(value)) {
        auto color = CSSColorValue::parse(value);
        return std::make_unique<CSSColorVal>(color);
    }

    // Check for transform
    if (value.find("translate") != std::string::npos ||
        value.find("rotate") != std::string::npos ||
        value.find("scale") != std::string::npos ||
        value.find("skew") != std::string::npos ||
        value.find("matrix") != std::string::npos ||
        value.find("perspective") != std::string::npos) {
        if (property == "transform") {
            auto tv = CSSTransformValue::parse(value);
            return std::make_unique<CSSTransformValue>(std::move(tv));
        }
    }

    // Try numeric
    char* end;
    float num = std::strtof(value.c_str(), &end);
    if (end != value.c_str()) {
        std::string unitStr(end);
        // Trim whitespace
        while (!unitStr.empty() && std::isspace(unitStr.front()))
            unitStr.erase(unitStr.begin());
        CSSUnit unit = unitStr.empty() ? CSSUnit::Number : CSSUnitValue::parseUnit(unitStr);
        return std::make_unique<CSSUnitVal>(num, unit);
    }

    // Keyword
    return std::make_unique<CSSKeywordValue>(value);
}

// ==================================================================
// CSSNumericValue
// ==================================================================

CSSStyleValueType CSSNumericValue::type() const {
    if (CSSUnitValue::isAngle(unit_)) return CSSStyleValueType::Angle;
    if (CSSUnitValue::isTime(unit_)) return CSSStyleValueType::Time;
    if (unit_ == CSSUnit::Percent) return CSSStyleValueType::Percentage;
    if (unit_ == CSSUnit::Number) return CSSStyleValueType::Number;
    return CSSStyleValueType::Length;
}

std::string CSSNumericValue::toString() const {
    std::ostringstream ss;
    ss << value_;
    switch (unit_) {
        case CSSUnit::Px: ss << "px"; break;
        case CSSUnit::Em: ss << "em"; break;
        case CSSUnit::Rem: ss << "rem"; break;
        case CSSUnit::Percent: ss << "%"; break;
        case CSSUnit::Vw: ss << "vw"; break;
        case CSSUnit::Vh: ss << "vh"; break;
        case CSSUnit::Deg: ss << "deg"; break;
        case CSSUnit::Rad: ss << "rad"; break;
        case CSSUnit::S: ss << "s"; break;
        case CSSUnit::Ms: ss << "ms"; break;
        case CSSUnit::Fr: ss << "fr"; break;
        default: break;
    }
    return ss.str();
}

std::unique_ptr<CSSNumericValue> CSSNumericValue::add(const CSSNumericValue& other) const {
    if (unit_ == other.unit_) {
        return std::make_unique<CSSUnitVal>(value_ + other.value_, unit_);
    }
    // Mixed units — would need calc() wrapper
    return std::make_unique<CSSUnitVal>(value_ + other.value_, unit_);
}

std::unique_ptr<CSSNumericValue> CSSNumericValue::sub(const CSSNumericValue& other) const {
    if (unit_ == other.unit_) {
        return std::make_unique<CSSUnitVal>(value_ - other.value_, unit_);
    }
    return std::make_unique<CSSUnitVal>(value_ - other.value_, unit_);
}

std::unique_ptr<CSSNumericValue> CSSNumericValue::mul(float factor) const {
    return std::make_unique<CSSUnitVal>(value_ * factor, unit_);
}

std::unique_ptr<CSSNumericValue> CSSNumericValue::div(float divisor) const {
    return std::make_unique<CSSUnitVal>(divisor != 0 ? value_ / divisor : 0, unit_);
}

std::unique_ptr<CSSNumericValue> CSSNumericValue::negate() const {
    return std::make_unique<CSSUnitVal>(-value_, unit_);
}

std::unique_ptr<CSSNumericValue> CSSNumericValue::to(CSSUnit targetUnit) const {
    if (unit_ == targetUnit) return std::make_unique<CSSUnitVal>(value_, unit_);

    // Absolute unit conversions
    float px = CSSUnitValue{value_, unit_}.toPx(16, 16, 1920, 1080, 0);
    CSSUnitValue target{1, targetUnit};
    float onePxInTarget = target.toPx(16, 16, 1920, 1080, 0);
    if (onePxInTarget != 0) {
        return std::make_unique<CSSUnitVal>(px / onePxInTarget, targetUnit);
    }
    return std::make_unique<CSSUnitVal>(value_, unit_);
}

std::unique_ptr<CSSNumericValue> CSSNumericValue::create(float value, CSSUnit unit) {
    return std::make_unique<CSSUnitVal>(value, unit);
}

// ==================================================================
// CSSMathValue
// ==================================================================

std::string CSSMathValue::toString() const {
    return "calc(" + CSSCalcEngine::serialize(*expr_) + ")";
}

float CSSMathValue::resolve(const CSSCalcEngine::Context& ctx) const {
    return CSSCalcEngine::evaluate(*expr_, ctx);
}

// ==================================================================
// CSSTransformComponent
// ==================================================================

void CSSTransformComponent::toMatrix4x4(float out[16]) const {
    // Identity
    for (int i = 0; i < 16; i++) out[i] = (i % 5 == 0) ? 1.0f : 0.0f;

    switch (type) {
        case Type::TranslateX:
            if (!values.empty()) out[12] = values[0];
            break;
        case Type::TranslateY:
            if (!values.empty()) out[13] = values[0];
            break;
        case Type::TranslateZ:
            if (!values.empty()) out[14] = values[0];
            break;
        case Type::Translate:
        case Type::Translate3D:
            if (values.size() >= 1) out[12] = values[0];
            if (values.size() >= 2) out[13] = values[1];
            if (values.size() >= 3) out[14] = values[2];
            break;
        case Type::ScaleX:
            if (!values.empty()) out[0] = values[0];
            break;
        case Type::ScaleY:
            if (!values.empty()) out[5] = values[0];
            break;
        case Type::ScaleZ:
            if (!values.empty()) out[10] = values[0];
            break;
        case Type::Scale:
        case Type::Scale3D:
            if (values.size() >= 1) out[0] = values[0];
            if (values.size() >= 2) out[5] = values[1];
            else if (values.size() >= 1) out[5] = values[0];
            if (values.size() >= 3) out[10] = values[2];
            break;
        case Type::Rotate:
        case Type::RotateZ: {
            if (!values.empty()) {
                float rad = values[0] * 3.14159265f / 180.0f;
                float c = std::cos(rad), s = std::sin(rad);
                out[0] = c;  out[1] = s;
                out[4] = -s; out[5] = c;
            }
            break;
        }
        case Type::RotateX: {
            if (!values.empty()) {
                float rad = values[0] * 3.14159265f / 180.0f;
                float c = std::cos(rad), s = std::sin(rad);
                out[5] = c;  out[6] = s;
                out[9] = -s; out[10] = c;
            }
            break;
        }
        case Type::RotateY: {
            if (!values.empty()) {
                float rad = values[0] * 3.14159265f / 180.0f;
                float c = std::cos(rad), s = std::sin(rad);
                out[0] = c;  out[2] = -s;
                out[8] = s;  out[10] = c;
            }
            break;
        }
        case Type::SkewX: {
            if (!values.empty()) {
                out[4] = std::tan(values[0] * 3.14159265f / 180.0f);
            }
            break;
        }
        case Type::SkewY: {
            if (!values.empty()) {
                out[1] = std::tan(values[0] * 3.14159265f / 180.0f);
            }
            break;
        }
        case Type::Skew: {
            if (values.size() >= 1) out[4] = std::tan(values[0] * 3.14159265f / 180.0f);
            if (values.size() >= 2) out[1] = std::tan(values[1] * 3.14159265f / 180.0f);
            break;
        }
        case Type::Matrix:
            if (values.size() >= 6) {
                out[0] = values[0]; out[1] = values[1];
                out[4] = values[2]; out[5] = values[3];
                out[12] = values[4]; out[13] = values[5];
            }
            break;
        case Type::Matrix3D:
            if (values.size() >= 16) {
                for (int i = 0; i < 16; i++) out[i] = values[i];
            }
            break;
        case Type::Perspective:
            if (!values.empty() && values[0] != 0) {
                out[11] = -1.0f / values[0];
            }
            break;
        default: break;
    }
}

std::string CSSTransformComponent::toString() const {
    std::ostringstream ss;
    const char* name = "none";
    switch (type) {
        case Type::Translate: name = "translate"; break;
        case Type::TranslateX: name = "translateX"; break;
        case Type::TranslateY: name = "translateY"; break;
        case Type::TranslateZ: name = "translateZ"; break;
        case Type::Translate3D: name = "translate3d"; break;
        case Type::Rotate: name = "rotate"; break;
        case Type::RotateX: name = "rotateX"; break;
        case Type::RotateY: name = "rotateY"; break;
        case Type::RotateZ: name = "rotateZ"; break;
        case Type::Scale: name = "scale"; break;
        case Type::ScaleX: name = "scaleX"; break;
        case Type::ScaleY: name = "scaleY"; break;
        case Type::Skew: name = "skew"; break;
        case Type::SkewX: name = "skewX"; break;
        case Type::SkewY: name = "skewY"; break;
        case Type::Matrix: name = "matrix"; break;
        case Type::Matrix3D: name = "matrix3d"; break;
        case Type::Perspective: name = "perspective"; break;
        default: break;
    }
    ss << name << "(";
    for (size_t i = 0; i < values.size(); i++) {
        if (i > 0) ss << ", ";
        ss << values[i];
    }
    ss << ")";
    return ss.str();
}

// ==================================================================
// CSSTransformValue
// ==================================================================

std::string CSSTransformValue::toString() const {
    std::string result;
    for (size_t i = 0; i < components_.size(); i++) {
        if (i > 0) result += " ";
        result += components_[i].toString();
    }
    return result;
}

void CSSTransformValue::toMatrix4x4(float out[16]) const {
    // Start with identity
    for (int i = 0; i < 16; i++) out[i] = (i % 5 == 0) ? 1.0f : 0.0f;

    for (const auto& comp : components_) {
        float m[16];
        comp.toMatrix4x4(m);

        // Matrix multiply: out = out * m
        float tmp[16];
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                tmp[col * 4 + row] =
                    out[0 * 4 + row] * m[col * 4 + 0] +
                    out[1 * 4 + row] * m[col * 4 + 1] +
                    out[2 * 4 + row] * m[col * 4 + 2] +
                    out[3 * 4 + row] * m[col * 4 + 3];
            }
        }
        std::memcpy(out, tmp, sizeof(tmp));
    }
}

CSSTransformValue CSSTransformValue::interpolate(const CSSTransformValue& from,
                                                    const CSSTransformValue& to, float t) {
    CSSTransformValue result;
    size_t count = std::max(from.components_.size(), to.components_.size());

    for (size_t i = 0; i < count; i++) {
        CSSTransformComponent comp;

        if (i < from.components_.size() && i < to.components_.size() &&
            from.components_[i].type == to.components_[i].type) {
            comp.type = from.components_[i].type;
            size_t valCount = std::max(from.components_[i].values.size(),
                                         to.components_[i].values.size());
            for (size_t j = 0; j < valCount; j++) {
                float fromVal = (j < from.components_[i].values.size()) ?
                    from.components_[i].values[j] : 0;
                float toVal = (j < to.components_[i].values.size()) ?
                    to.components_[i].values[j] : 0;
                comp.values.push_back(fromVal + (toVal - fromVal) * t);
            }
        } else {
            // Fallback: matrix decomposition would go here
            // For now: use whichever side has the component
            if (i < to.components_.size()) {
                comp = to.components_[i];
            } else if (i < from.components_.size()) {
                comp = from.components_[i];
            }
        }

        result.addComponent(comp);
    }

    return result;
}

CSSTransformValue CSSTransformValue::parse(const std::string& value) {
    CSSTransformValue result;
    size_t pos = 0;

    while (pos < value.size()) {
        while (pos < value.size() && std::isspace(value[pos])) pos++;
        if (pos >= value.size()) break;

        // Read function name
        size_t nameStart = pos;
        while (pos < value.size() && value[pos] != '(') pos++;
        std::string name = value.substr(nameStart, pos - nameStart);
        // Trim
        while (!name.empty() && std::isspace(name.back())) name.pop_back();

        if (pos >= value.size()) break;
        pos++; // skip '('

        // Read arguments until ')'
        size_t argStart = pos;
        int depth = 1;
        while (pos < value.size() && depth > 0) {
            if (value[pos] == '(') depth++;
            else if (value[pos] == ')') depth--;
            if (depth > 0) pos++;
        }
        std::string argsStr = value.substr(argStart, pos - argStart);
        if (pos < value.size()) pos++; // skip ')'

        CSSTransformComponent comp;
        if (name == "translate") comp.type = CSSTransformComponent::Type::Translate;
        else if (name == "translateX") comp.type = CSSTransformComponent::Type::TranslateX;
        else if (name == "translateY") comp.type = CSSTransformComponent::Type::TranslateY;
        else if (name == "translateZ") comp.type = CSSTransformComponent::Type::TranslateZ;
        else if (name == "translate3d") comp.type = CSSTransformComponent::Type::Translate3D;
        else if (name == "rotate") comp.type = CSSTransformComponent::Type::Rotate;
        else if (name == "rotateX") comp.type = CSSTransformComponent::Type::RotateX;
        else if (name == "rotateY") comp.type = CSSTransformComponent::Type::RotateY;
        else if (name == "rotateZ") comp.type = CSSTransformComponent::Type::RotateZ;
        else if (name == "scale") comp.type = CSSTransformComponent::Type::Scale;
        else if (name == "scaleX") comp.type = CSSTransformComponent::Type::ScaleX;
        else if (name == "scaleY") comp.type = CSSTransformComponent::Type::ScaleY;
        else if (name == "scaleZ") comp.type = CSSTransformComponent::Type::ScaleZ;
        else if (name == "scale3d") comp.type = CSSTransformComponent::Type::Scale3D;
        else if (name == "skew") comp.type = CSSTransformComponent::Type::Skew;
        else if (name == "skewX") comp.type = CSSTransformComponent::Type::SkewX;
        else if (name == "skewY") comp.type = CSSTransformComponent::Type::SkewY;
        else if (name == "matrix") comp.type = CSSTransformComponent::Type::Matrix;
        else if (name == "matrix3d") comp.type = CSSTransformComponent::Type::Matrix3D;
        else if (name == "perspective") comp.type = CSSTransformComponent::Type::Perspective;
        else continue;

        // Parse comma-separated numeric arguments
        std::istringstream argStream(argsStr);
        std::string arg;
        while (std::getline(argStream, arg, ',')) {
            while (!arg.empty() && std::isspace(arg.front())) arg.erase(arg.begin());
            while (!arg.empty() && std::isspace(arg.back())) arg.pop_back();
            auto uv = CSSUnitValue::parse(arg);
            comp.values.push_back(uv.value);
        }

        result.addComponent(comp);
    }

    return result;
}

// ==================================================================
// CSSPositionValue
// ==================================================================

std::string CSSPositionValue::toString() const {
    std::ostringstream ss;
    ss << x_.value;
    switch (x_.unit) {
        case CSSUnit::Px: ss << "px"; break;
        case CSSUnit::Percent: ss << "%"; break;
        default: break;
    }
    ss << " ";
    ss << y_.value;
    switch (y_.unit) {
        case CSSUnit::Px: ss << "px"; break;
        case CSSUnit::Percent: ss << "%"; break;
        default: break;
    }
    return ss.str();
}

CSSPositionValue CSSPositionValue::parse(const std::string& value) {
    CSSUnitValue x{50, CSSUnit::Percent};
    CSSUnitValue y{50, CSSUnit::Percent};

    // Named keywords
    if (value == "center") return CSSPositionValue(x, y);
    if (value == "left") { x = {0, CSSUnit::Percent}; return CSSPositionValue(x, y); }
    if (value == "right") { x = {100, CSSUnit::Percent}; return CSSPositionValue(x, y); }
    if (value == "top") { y = {0, CSSUnit::Percent}; return CSSPositionValue(x, y); }
    if (value == "bottom") { y = {100, CSSUnit::Percent}; return CSSPositionValue(x, y); }

    // Two-value
    auto space = value.find(' ');
    if (space != std::string::npos) {
        x = CSSUnitValue::parse(value.substr(0, space));
        y = CSSUnitValue::parse(value.substr(space + 1));
    } else {
        x = CSSUnitValue::parse(value);
    }

    return CSSPositionValue(x, y);
}

// ==================================================================
// CSSUnparsedValue
// ==================================================================

std::string CSSUnparsedValue::toString() const {
    std::string result;
    for (const auto& frag : fragments_) {
        if (frag.type == Fragment::Type::String) {
            result += frag.value;
        } else {
            result += "var(" + frag.value;
            if (!frag.fallback.empty()) result += ", " + frag.fallback;
            result += ")";
        }
    }
    return result;
}

void CSSUnparsedValue::addString(const std::string& s) {
    fragments_.push_back({Fragment::Type::String, s, ""});
}

void CSSUnparsedValue::addVariable(const std::string& name, const std::string& fallback) {
    fragments_.push_back({Fragment::Type::Variable, name, fallback});
}

// ==================================================================
// StylePropertyMap
// ==================================================================

void StylePropertyMap::set(const std::string& property, std::unique_ptr<CSSStyleValue> value) {
    map_[property].clear();
    map_[property].push_back(std::move(value));
}

CSSStyleValue* StylePropertyMap::get(const std::string& property) const {
    auto it = map_.find(property);
    if (it != map_.end() && !it->second.empty()) return it->second[0].get();
    return nullptr;
}

bool StylePropertyMap::has(const std::string& property) const {
    return map_.find(property) != map_.end();
}

void StylePropertyMap::remove(const std::string& property) {
    map_.erase(property);
}

void StylePropertyMap::clear() {
    map_.clear();
}

std::vector<std::string> StylePropertyMap::properties() const {
    std::vector<std::string> result;
    for (const auto& [key, _] : map_) result.push_back(key);
    return result;
}

void StylePropertyMap::append(const std::string& property, std::unique_ptr<CSSStyleValue> value) {
    map_[property].push_back(std::move(value));
}

std::vector<CSSStyleValue*> StylePropertyMap::getAll(const std::string& property) const {
    std::vector<CSSStyleValue*> result;
    auto it = map_.find(property);
    if (it != map_.end()) {
        for (const auto& v : it->second) result.push_back(v.get());
    }
    return result;
}

// ==================================================================
// CSSPropertyRegistry
// ==================================================================

CSSPropertyRegistry& CSSPropertyRegistry::instance() {
    static CSSPropertyRegistry inst;
    return inst;
}

void CSSPropertyRegistry::registerProperty(const CSSPropertyDefinition& def) {
    props_[def.name] = def;
}

const CSSPropertyDefinition* CSSPropertyRegistry::find(const std::string& name) const {
    auto it = props_.find(name);
    return (it != props_.end()) ? &it->second : nullptr;
}

bool CSSPropertyRegistry::isAnimatable(const std::string& name) const {
    auto* def = find(name);
    return def && def->animatable;
}

CSSStyleValueType CSSPropertyRegistry::expectedType(const std::string& name) const {
    auto* def = find(name);
    return def ? def->expectedType : CSSStyleValueType::Unknown;
}

void CSSPropertyRegistry::registerBuiltins() {
    auto reg = [&](const char* name, CSSStyleValueType type, bool inherits,
                    const char* initial, bool animatable) {
        registerProperty({name, type, inherits, initial, "*", animatable});
    };

    // Layout
    reg("display", CSSStyleValueType::Keyword, false, "inline", false);
    reg("position", CSSStyleValueType::Keyword, false, "static", false);
    reg("float", CSSStyleValueType::Keyword, false, "none", false);
    reg("clear", CSSStyleValueType::Keyword, false, "none", false);
    reg("overflow", CSSStyleValueType::Keyword, false, "visible", false);

    // Box model
    reg("width", CSSStyleValueType::Length, false, "auto", true);
    reg("height", CSSStyleValueType::Length, false, "auto", true);
    reg("min-width", CSSStyleValueType::Length, false, "auto", true);
    reg("max-width", CSSStyleValueType::Length, false, "none", true);
    reg("min-height", CSSStyleValueType::Length, false, "auto", true);
    reg("max-height", CSSStyleValueType::Length, false, "none", true);
    reg("margin-top", CSSStyleValueType::Length, false, "0", true);
    reg("margin-right", CSSStyleValueType::Length, false, "0", true);
    reg("margin-bottom", CSSStyleValueType::Length, false, "0", true);
    reg("margin-left", CSSStyleValueType::Length, false, "0", true);
    reg("padding-top", CSSStyleValueType::Length, false, "0", true);
    reg("padding-right", CSSStyleValueType::Length, false, "0", true);
    reg("padding-bottom", CSSStyleValueType::Length, false, "0", true);
    reg("padding-left", CSSStyleValueType::Length, false, "0", true);

    // Visual
    reg("color", CSSStyleValueType::Color, true, "black", true);
    reg("background-color", CSSStyleValueType::Color, false, "transparent", true);
    reg("border-color", CSSStyleValueType::Color, false, "currentcolor", true);
    reg("opacity", CSSStyleValueType::Number, false, "1", true);
    reg("visibility", CSSStyleValueType::Keyword, true, "visible", true);
    reg("z-index", CSSStyleValueType::Number, false, "auto", true);

    // Typography
    reg("font-size", CSSStyleValueType::Length, true, "medium", true);
    reg("font-weight", CSSStyleValueType::Number, true, "400", true);
    reg("line-height", CSSStyleValueType::Number, true, "normal", true);
    reg("letter-spacing", CSSStyleValueType::Length, true, "normal", true);
    reg("word-spacing", CSSStyleValueType::Length, true, "normal", true);
    reg("text-indent", CSSStyleValueType::Length, true, "0", true);

    // Borders
    reg("border-width", CSSStyleValueType::Length, false, "medium", true);
    reg("border-radius", CSSStyleValueType::Length, false, "0", true);
    reg("outline-width", CSSStyleValueType::Length, false, "medium", true);
    reg("outline-offset", CSSStyleValueType::Length, false, "0", true);

    // Flex/Grid
    reg("flex-grow", CSSStyleValueType::Number, false, "0", true);
    reg("flex-shrink", CSSStyleValueType::Number, false, "1", true);
    reg("flex-basis", CSSStyleValueType::Length, false, "auto", true);
    reg("gap", CSSStyleValueType::Length, false, "normal", true);
    reg("order", CSSStyleValueType::Number, false, "0", true);

    // Transform
    reg("transform", CSSStyleValueType::Transform, false, "none", true);
    reg("transform-origin", CSSStyleValueType::Position, false, "50% 50%", true);

    // Shadows
    reg("box-shadow", CSSStyleValueType::Unknown, false, "none", true);
    reg("text-shadow", CSSStyleValueType::Unknown, false, "none", true);

    // Misc
    reg("aspect-ratio", CSSStyleValueType::Number, false, "auto", true);
    reg("object-fit", CSSStyleValueType::Keyword, false, "fill", false);
    reg("cursor", CSSStyleValueType::Keyword, true, "auto", false);
    reg("pointer-events", CSSStyleValueType::Keyword, true, "auto", false);
}

} // namespace Web
} // namespace NXRender
