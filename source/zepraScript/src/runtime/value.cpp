/**
 * @file value.cpp
 * @brief JavaScript value implementation
 */

#include "zeprascript/runtime/value.hpp"
#include "zeprascript/runtime/object.hpp"
#include "zeprascript/runtime/function.hpp"
#include <sstream>
#include <cmath>
#include <limits>

namespace Zepra::Runtime {

ValueType Value::type() const {
    if (isUndefined()) return ValueType::Undefined;
    if (isNull()) return ValueType::Null;
    if (isBoolean()) return ValueType::Boolean;
    if (isNumber()) return ValueType::Number;
    if (isString()) return ValueType::String;
    if (isObject()) return ValueType::Object;
    return ValueType::Undefined;
}

bool Value::isFalsy() const {
    if (isUndefined() || isNull()) return true;
    if (isBoolean()) return !asBoolean();
    if (isNumber()) {
        double n = asNumber();
        return n == 0.0 || std::isnan(n);
    }
    if (isString()) return asString()->value().empty();
    return false;
}

Function* Value::asFunction() const {
    if (!isObject()) return nullptr;
    Object* obj = asObject();
    if (!obj->isFunction()) return nullptr;
    return static_cast<Function*>(obj);
}

Array* Value::asArray() const {
    if (!isObject()) return nullptr;
    Object* obj = asObject();
    if (!obj->isArray()) return nullptr;
    return static_cast<Array*>(obj);
}

bool Value::toBoolean() const {
    return !isFalsy();
}

double Value::toNumber() const {
    switch (type()) {
        case ValueType::Undefined:
            return std::nan("");
        case ValueType::Null:
            return 0.0;
        case ValueType::Boolean:
            return asBoolean() ? 1.0 : 0.0;
        case ValueType::Number:
            return asNumber();
        case ValueType::String: {
            const std::string& str = asString()->value();
            if (str.empty()) return 0.0;
            try {
                return std::stod(str);
            } catch (...) {
                return std::nan("");
            }
        }
        case ValueType::Object:
            // TODO: Call valueOf, then toString
            return std::nan("");
        default:
            return std::nan("");
    }
}

std::string Value::toString() const {
    switch (type()) {
        case ValueType::Undefined:
            return "undefined";
        case ValueType::Null:
            return "null";
        case ValueType::Boolean:
            return asBoolean() ? "true" : "false";
        case ValueType::Number: {
            double n = asNumber();
            if (std::isnan(n)) return "NaN";
            if (std::isinf(n)) return n > 0 ? "Infinity" : "-Infinity";
            if (n == 0.0) return "0";
            
            // Format number
            std::ostringstream oss;
            oss << n;
            return oss.str();
        }
        case ValueType::String:
            return asString()->value();
        case ValueType::Object: {
            // TODO: Call toString method
            return "[object Object]";
        }
        default:
            return "";
    }
}

Object* Value::toObject() const {
    if (isObject()) return asObject();
    // TODO: Boxing for primitives
    return nullptr;
}

bool Value::equals(const Value& other) const {
    // Same type, use strict equality
    if (type() == other.type()) {
        return strictEquals(other);
    }
    
    // null == undefined
    if ((isNull() && other.isUndefined()) ||
        (isUndefined() && other.isNull())) {
        return true;
    }
    
    // Number comparisons
    if (isNumber() && other.isNumber()) {
        return asNumber() == other.asNumber();
    }
    
    // String to number
    if (isNumber() && other.isString()) {
        return asNumber() == other.toNumber();
    }
    if (isString() && other.isNumber()) {
        return toNumber() == other.asNumber();
    }
    
    // Boolean to number
    if (isBoolean()) {
        return Value::number(toNumber()).equals(other);
    }
    if (other.isBoolean()) {
        return equals(Value::number(other.toNumber()));
    }
    
    return false;
}

bool Value::strictEquals(const Value& other) const {
    if (type() != other.type()) return false;
    
    switch (type()) {
        case ValueType::Undefined:
        case ValueType::Null:
            return true;
        case ValueType::Boolean:
            return asBoolean() == other.asBoolean();
        case ValueType::Number: {
            double a = asNumber(), b = other.asNumber();
            if (std::isnan(a) || std::isnan(b)) return false;
            return a == b;
        }
        case ValueType::String:
            return asString()->value() == other.asString()->value();
        case ValueType::Object:
            return asObject() == other.asObject();
        default:
            return false;
    }
}

// Arithmetic operations
Value Value::add(const Value& left, const Value& right) {
    // String concatenation
    if (left.isString() || right.isString()) {
        return Value::string(new String(left.toString() + right.toString()));
    }
    
    // Numeric addition
    return Value::number(left.toNumber() + right.toNumber());
}

Value Value::subtract(const Value& left, const Value& right) {
    return Value::number(left.toNumber() - right.toNumber());
}

Value Value::multiply(const Value& left, const Value& right) {
    return Value::number(left.toNumber() * right.toNumber());
}

Value Value::divide(const Value& left, const Value& right) {
    double a = left.toNumber();
    double b = right.toNumber();
    
    if (b == 0.0) {
        if (a == 0.0) return Value::number(std::nan(""));
        return Value::number(a > 0 ? std::numeric_limits<double>::infinity() 
                                   : -std::numeric_limits<double>::infinity());
    }
    
    return Value::number(a / b);
}

Value Value::modulo(const Value& left, const Value& right) {
    double a = left.toNumber();
    double b = right.toNumber();
    return Value::number(std::fmod(a, b));
}

Value Value::power(const Value& left, const Value& right) {
    return Value::number(std::pow(left.toNumber(), right.toNumber()));
}

Value Value::bitwiseAnd(const Value& left, const Value& right) {
    int32_t a = static_cast<int32_t>(left.toNumber());
    int32_t b = static_cast<int32_t>(right.toNumber());
    return Value::number(static_cast<double>(a & b));
}

Value Value::bitwiseOr(const Value& left, const Value& right) {
    int32_t a = static_cast<int32_t>(left.toNumber());
    int32_t b = static_cast<int32_t>(right.toNumber());
    return Value::number(static_cast<double>(a | b));
}

Value Value::bitwiseXor(const Value& left, const Value& right) {
    int32_t a = static_cast<int32_t>(left.toNumber());
    int32_t b = static_cast<int32_t>(right.toNumber());
    return Value::number(static_cast<double>(a ^ b));
}

Value Value::bitwiseNot(const Value& val) {
    int32_t a = static_cast<int32_t>(val.toNumber());
    return Value::number(static_cast<double>(~a));
}

Value Value::leftShift(const Value& left, const Value& right) {
    int32_t a = static_cast<int32_t>(left.toNumber());
    uint32_t b = static_cast<uint32_t>(right.toNumber()) & 0x1F;
    return Value::number(static_cast<double>(a << b));
}

Value Value::rightShift(const Value& left, const Value& right) {
    int32_t a = static_cast<int32_t>(left.toNumber());
    uint32_t b = static_cast<uint32_t>(right.toNumber()) & 0x1F;
    return Value::number(static_cast<double>(a >> b));
}

Value Value::unsignedRightShift(const Value& left, const Value& right) {
    uint32_t a = static_cast<uint32_t>(left.toNumber());
    uint32_t b = static_cast<uint32_t>(right.toNumber()) & 0x1F;
    return Value::number(static_cast<double>(a >> b));
}

Value Value::negate(const Value& val) {
    return Value::number(-val.toNumber());
}

Value Value::logicalNot(const Value& val) {
    return Value::boolean(!val.toBoolean());
}

Value Value::lessThan(const Value& left, const Value& right) {
    if (left.isString() && right.isString()) {
        return Value::boolean(left.asString()->value() < right.asString()->value());
    }
    return Value::boolean(left.toNumber() < right.toNumber());
}

Value Value::lessEqual(const Value& left, const Value& right) {
    if (left.isString() && right.isString()) {
        return Value::boolean(left.asString()->value() <= right.asString()->value());
    }
    return Value::boolean(left.toNumber() <= right.toNumber());
}

Value Value::greaterThan(const Value& left, const Value& right) {
    if (left.isString() && right.isString()) {
        return Value::boolean(left.asString()->value() > right.asString()->value());
    }
    return Value::boolean(left.toNumber() > right.toNumber());
}

Value Value::greaterEqual(const Value& left, const Value& right) {
    if (left.isString() && right.isString()) {
        return Value::boolean(left.asString()->value() >= right.asString()->value());
    }
    return Value::boolean(left.toNumber() >= right.toNumber());
}

} // namespace Zepra::Runtime
