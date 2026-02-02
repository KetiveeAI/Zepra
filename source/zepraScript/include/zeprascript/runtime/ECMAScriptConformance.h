/**
 * @file ECMAScriptConformance.h
 * @brief ECMAScript Specification Conformance Utilities
 * 
 * Implements spec-compliant operations for:
 * - Abstract operations (ToPrimitive, ToNumber, ToString, etc.)
 * - Type coercions with correct edge cases
 * - Comparison algorithms (Abstract Equality, Strict Equality)
 * - Error timing and exception semantics
 */

#pragma once

#include "zeprascript/runtime/value.hpp"
#include <string>
#include <optional>
#include <cmath>
#include <limits>

namespace Zepra::Runtime {

// Forward declarations
class Object;
class String;
class Context;

// =============================================================================
// Abstract Operations (ECMA-262 Section 7)
// =============================================================================

/**
 * @brief Type conversion hint for ToPrimitive
 */
enum class PreferredType {
    Default,
    Number,
    String
};

/**
 * @brief ECMAScript Type checking (7.1)
 */
class TypeOps {
public:
    // 7.1.1 ToPrimitive
    static Value toPrimitive(Value input, PreferredType hint = PreferredType::Default);
    
    // 7.1.2 ToBoolean
    static bool toBoolean(Value argument) {
        if (argument.isUndefined() || argument.isNull()) return false;
        if (argument.isBoolean()) return argument.asBoolean();
        if (argument.isNumber()) {
            double n = argument.asNumber();
            return n != 0 && !std::isnan(n);
        }
        if (argument.isString()) {
            return !argument.asString()->empty();
        }
        // Objects are always truthy
        return true;
    }
    
    // 7.1.3 ToNumber
    static double toNumber(Value argument) {
        if (argument.isUndefined()) return std::nan("");
        if (argument.isNull()) return 0.0;
        if (argument.isBoolean()) return argument.asBoolean() ? 1.0 : 0.0;
        if (argument.isNumber()) return argument.asNumber();
        if (argument.isString()) return stringToNumber(argument.toString());
        if (argument.isSymbol()) {
            // TypeError - handled by caller
            return std::nan("");
        }
        // Object: ToPrimitive then ToNumber
        return toNumber(toPrimitive(argument, PreferredType::Number));
    }
    
    // 7.1.4 ToInteger (legacy, now ToIntegerOrInfinity)
    static double toIntegerOrInfinity(Value argument) {
        double n = toNumber(argument);
        if (std::isnan(n) || n == 0.0) return 0.0;
        if (std::isinf(n)) return n;
        return std::trunc(n);
    }
    
    // 7.1.5 ToInt32
    static int32_t toInt32(Value argument) {
        double n = toNumber(argument);
        if (std::isnan(n) || std::isinf(n) || n == 0.0) return 0;
        
        // Modulo 2^32
        double int32bit = std::fmod(std::trunc(n), 4294967296.0);
        if (int32bit < 0) int32bit += 4294967296.0;
        
        // To signed
        if (int32bit >= 2147483648.0) {
            int32bit -= 4294967296.0;
        }
        return static_cast<int32_t>(int32bit);
    }
    
    // 7.1.6 ToUint32
    static uint32_t toUint32(Value argument) {
        double n = toNumber(argument);
        if (std::isnan(n) || std::isinf(n) || n == 0.0) return 0;
        
        double int32bit = std::fmod(std::trunc(n), 4294967296.0);
        if (int32bit < 0) int32bit += 4294967296.0;
        return static_cast<uint32_t>(int32bit);
    }
    
    // 7.1.7 ToInt16
    static int16_t toInt16(Value argument) {
        double n = toNumber(argument);
        if (std::isnan(n) || std::isinf(n) || n == 0.0) return 0;
        
        double int16bit = std::fmod(std::trunc(n), 65536.0);
        if (int16bit < 0) int16bit += 65536.0;
        
        if (int16bit >= 32768.0) {
            int16bit -= 65536.0;
        }
        return static_cast<int16_t>(int16bit);
    }
    
    // 7.1.17 ToString
    static std::string toString(Value argument) {
        if (argument.isUndefined()) return "undefined";
        if (argument.isNull()) return "null";
        if (argument.isBoolean()) return argument.asBoolean() ? "true" : "false";
        if (argument.isNumber()) return numberToString(argument.asNumber());
        if (argument.isString()) return argument.toString();
        if (argument.isSymbol()) {
            // TypeError - handled by caller
            return "";
        }
        // Object: ToPrimitive then ToString
        return toString(toPrimitive(argument, PreferredType::String));
    }
    
    // 7.1.18 ToObject
    static Object* toObject(Context* ctx, Value argument);
    
    // 7.1.19 ToPropertyKey
    static std::string toPropertyKey(Value argument) {
        if (argument.isSymbol()) {
            return "[Symbol:" + std::to_string(argument.asSymbol()) + "]";
        }
        return toString(argument);
    }
    
    // 7.1.20 ToLength
    static uint64_t toLength(Value argument) {
        double len = toIntegerOrInfinity(argument);
        if (len <= 0) return 0;
        return static_cast<uint64_t>(std::min(len, 9007199254740991.0));  // 2^53 - 1
    }
    
    // 7.1.21 CanonicalNumericIndexString
    static std::optional<double> canonicalNumericIndexString(const std::string& s) {
        if (s == "-0") return -0.0;
        double n = stringToNumber(s);
        if (numberToString(n) != s) return std::nullopt;
        return n;
    }
    
private:
    // Helper: String to Number conversion
    static double stringToNumber(const std::string& s) {
        // Trim whitespace
        size_t start = s.find_first_not_of(" \t\n\r\f\v");
        if (start == std::string::npos) return 0.0;  // Empty/whitespace = 0
        
        size_t end = s.find_last_not_of(" \t\n\r\f\v");
        std::string trimmed = s.substr(start, end - start + 1);
        
        if (trimmed.empty()) return 0.0;
        
        // Handle special cases
        if (trimmed == "Infinity" || trimmed == "+Infinity") {
            return std::numeric_limits<double>::infinity();
        }
        if (trimmed == "-Infinity") {
            return -std::numeric_limits<double>::infinity();
        }
        
        // Handle hex
        if (trimmed.size() > 2 && trimmed[0] == '0' && 
            (trimmed[1] == 'x' || trimmed[1] == 'X')) {
            try {
                return static_cast<double>(std::stoll(trimmed, nullptr, 16));
            } catch (...) {
                return std::nan("");
            }
        }
        
        // Parse as decimal
        try {
            size_t pos;
            double result = std::stod(trimmed, &pos);
            if (pos != trimmed.size()) return std::nan("");
            return result;
        } catch (...) {
            return std::nan("");
        }
    }
    
    // Helper: Number to String conversion (7.1.17.1)
    static std::string numberToString(double n) {
        if (std::isnan(n)) return "NaN";
        if (n == 0.0) return "0";  // Both +0 and -0
        if (n < 0.0) return "-" + numberToString(-n);
        if (std::isinf(n)) return "Infinity";
        
        // Check if integer
        if (n == std::floor(n) && n < 9007199254740992.0) {
            return std::to_string(static_cast<int64_t>(n));
        }
        
        // Use standard double formatting
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.17g", n);
        return buf;
    }
};

// =============================================================================
// Comparison Operations (7.2)
// =============================================================================

class CompareOps {
public:
    // 7.2.13 Abstract Equality Comparison (==)
    static bool abstractEquals(Value x, Value y) {
        // Same type
        if (x.type() == y.type()) {
            return strictEquals(x, y);
        }
        
        // null == undefined
        if (x.isNull() && y.isUndefined()) return true;
        if (x.isUndefined() && y.isNull()) return true;
        
        // Number comparisons
        if (x.isNumber() && y.isString()) {
            return abstractEquals(x, Value::number(TypeOps::toNumber(y)));
        }
        if (x.isString() && y.isNumber()) {
            return abstractEquals(Value::number(TypeOps::toNumber(x)), y);
        }
        
        // Boolean conversion
        if (x.isBoolean()) {
            return abstractEquals(Value::number(TypeOps::toNumber(x)), y);
        }
        if (y.isBoolean()) {
            return abstractEquals(x, Value::number(TypeOps::toNumber(y)));
        }
        
        // Object to primitive
        if ((x.isString() || x.isNumber() || x.isSymbol()) && y.isObject()) {
            return abstractEquals(x, TypeOps::toPrimitive(y));
        }
        if (x.isObject() && (y.isString() || y.isNumber() || y.isSymbol())) {
            return abstractEquals(TypeOps::toPrimitive(x), y);
        }
        
        return false;
    }
    
    // 7.2.14 Strict Equality Comparison (===)
    static bool strictEquals(Value x, Value y) {
        if (x.type() != y.type()) return false;
        
        if (x.isUndefined()) return true;
        if (x.isNull()) return true;
        if (x.isNumber()) {
            double xn = x.asNumber();
            double yn = y.asNumber();
            if (std::isnan(xn) || std::isnan(yn)) return false;
            if (xn == yn) return true;  // Handles +0 == -0
            return false;
        }
        if (x.isString()) {
            return x.toString() == y.toString();
        }
        if (x.isBoolean()) {
            return x.asBoolean() == y.asBoolean();
        }
        if (x.isSymbol()) {
            return x.asSymbol() == y.asSymbol();
        }
        if (x.isObject()) {
            return x.asObject() == y.asObject();  // Same identity
        }
        return false;
    }
    
    // 7.2.15 Abstract Relational Comparison
    static std::optional<bool> abstractLessThan(Value x, Value y, bool leftFirst = true) {
        Value px, py;
        
        if (leftFirst) {
            px = TypeOps::toPrimitive(x, PreferredType::Number);
            py = TypeOps::toPrimitive(y, PreferredType::Number);
        } else {
            py = TypeOps::toPrimitive(y, PreferredType::Number);
            px = TypeOps::toPrimitive(x, PreferredType::Number);
        }
        
        // String comparison
        if (px.isString() && py.isString()) {
            return px.toString() < py.toString();
        }
        
        // Numeric comparison
        double nx = TypeOps::toNumber(px);
        double ny = TypeOps::toNumber(py);
        
        if (std::isnan(nx) || std::isnan(ny)) return std::nullopt;
        
        return nx < ny;
    }
};

// =============================================================================
// Object Operations (7.3)
// =============================================================================

class ObjectOps {
public:
    // 7.3.1 MakeBasicObject (placeholder)
    // 7.3.2 Get
    static Value get(Object* obj, const std::string& key);
    
    // 7.3.3 GetV
    static Value getV(Value v, const std::string& key);
    
    // 7.3.4 Set
    static bool set(Object* obj, const std::string& key, Value value, bool throw_ = false);
    
    // 7.3.8 HasProperty
    static bool hasProperty(Object* obj, const std::string& key);
    
    // 7.3.9 HasOwnProperty
    static bool hasOwnProperty(Object* obj, const std::string& key);
    
    // 7.3.18 Call
    static Value call(Context* ctx, Value func, Value thisValue, 
                     const std::vector<Value>& args);
    
    // 7.3.19 Construct
    static Object* construct(Context* ctx, Value constructor, 
                            const std::vector<Value>& args);
};

// =============================================================================
// Testing Helpers (7.4)
// =============================================================================

class TestOps {
public:
    // 7.4.1 Iterator-related (placeholder)
    
    // IsCallable
    static bool isCallable(Value v) {
        if (!v.isObject()) return false;
        // Check for [[Call]] internal method
        return true;  // Simplified
    }
    
    // IsConstructor
    static bool isConstructor(Value v) {
        if (!v.isObject()) return false;
        // Check for [[Construct]] internal method
        return true;  // Simplified
    }
    
    // IsArray
    static bool isArray(Value v);
    
    // IsRegExp
    static bool isRegExp(Value v);
};

// =============================================================================
// Spec Conformance Assertions
// =============================================================================

/**
 * @brief Assertion utilities for spec compliance testing
 */
class SpecAssert {
public:
    // Verify TDZ behavior
    static bool checkTDZ(const std::string& varName, bool shouldThrow);
    
    // Verify strict mode behavior
    static bool checkStrictMode(bool expectedStrict);
    
    // Verify this binding
    static bool checkThisBinding(Value expected, Value actual);
    
    // Verify exception type
    static bool checkExceptionType(const std::string& expected, const std::string& actual);
};

} // namespace Zepra::Runtime
