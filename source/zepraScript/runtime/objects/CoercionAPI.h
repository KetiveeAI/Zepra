/**
 * @file CoercionAPI.h
 * @brief Type Coercion Implementation
 */

#pragma once

#include <string>
#include <cmath>
#include <limits>
#include <variant>
#include <optional>
#include <sstream>

namespace Zepra::Runtime {

// =============================================================================
// Value Types
// =============================================================================

using JSValue = std::variant<std::monostate, bool, double, std::string, std::nullptr_t>;

enum class JSType {
    Undefined,
    Null,
    Boolean,
    Number,
    String,
    Symbol,
    BigInt,
    Object
};

// =============================================================================
// Type Detection
// =============================================================================

inline JSType typeOf(const JSValue& value) {
    return std::visit([](auto&& v) -> JSType {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) return JSType::Undefined;
        else if constexpr (std::is_same_v<T, std::nullptr_t>) return JSType::Null;
        else if constexpr (std::is_same_v<T, bool>) return JSType::Boolean;
        else if constexpr (std::is_same_v<T, double>) return JSType::Number;
        else if constexpr (std::is_same_v<T, std::string>) return JSType::String;
        else return JSType::Object;
    }, value);
}

// =============================================================================
// ToBoolean
// =============================================================================

inline bool toBoolean(const JSValue& value) {
    return std::visit([](auto&& v) -> bool {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) return false;
        else if constexpr (std::is_same_v<T, std::nullptr_t>) return false;
        else if constexpr (std::is_same_v<T, bool>) return v;
        else if constexpr (std::is_same_v<T, double>) return v != 0 && !std::isnan(v);
        else if constexpr (std::is_same_v<T, std::string>) return !v.empty();
        else return true;
    }, value);
}

// =============================================================================
// ToNumber
// =============================================================================

inline double toNumber(const JSValue& value) {
    return std::visit([](auto&& v) -> double {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) 
            return std::numeric_limits<double>::quiet_NaN();
        else if constexpr (std::is_same_v<T, std::nullptr_t>) 
            return 0.0;
        else if constexpr (std::is_same_v<T, bool>) 
            return v ? 1.0 : 0.0;
        else if constexpr (std::is_same_v<T, double>) 
            return v;
        else if constexpr (std::is_same_v<T, std::string>) {
            if (v.empty()) return 0.0;
            try {
                size_t pos;
                double result = std::stod(v, &pos);
                while (pos < v.size() && std::isspace(v[pos])) ++pos;
                return pos == v.size() ? result : std::numeric_limits<double>::quiet_NaN();
            } catch (...) {
                return std::numeric_limits<double>::quiet_NaN();
            }
        }
        else return std::numeric_limits<double>::quiet_NaN();
    }, value);
}

// =============================================================================
// ToString
// =============================================================================

inline std::string toString(const JSValue& value) {
    return std::visit([](auto&& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) 
            return "undefined";
        else if constexpr (std::is_same_v<T, std::nullptr_t>) 
            return "null";
        else if constexpr (std::is_same_v<T, bool>) 
            return v ? "true" : "false";
        else if constexpr (std::is_same_v<T, double>) {
            if (std::isnan(v)) return "NaN";
            if (std::isinf(v)) return v > 0 ? "Infinity" : "-Infinity";
            if (v == 0) return "0";
            std::ostringstream oss;
            oss << v;
            return oss.str();
        }
        else if constexpr (std::is_same_v<T, std::string>) 
            return v;
        else 
            return "[object Object]";
    }, value);
}

// =============================================================================
// ToInteger
// =============================================================================

inline double toInteger(const JSValue& value) {
    double num = toNumber(value);
    if (std::isnan(num)) return 0;
    if (num == 0 || std::isinf(num)) return num;
    return std::trunc(num);
}

// =============================================================================
// ToInt32 / ToUint32
// =============================================================================

inline int32_t toInt32(const JSValue& value) {
    double num = toNumber(value);
    if (std::isnan(num) || std::isinf(num) || num == 0) return 0;
    
    double posInt = std::trunc(num);
    int64_t int64Val = static_cast<int64_t>(std::fmod(posInt, 4294967296.0));
    if (int64Val >= 2147483648) int64Val -= 4294967296;
    return static_cast<int32_t>(int64Val);
}

inline uint32_t toUint32(const JSValue& value) {
    double num = toNumber(value);
    if (std::isnan(num) || std::isinf(num) || num == 0) return 0;
    
    double posInt = std::trunc(num);
    return static_cast<uint32_t>(static_cast<uint64_t>(std::fmod(posInt, 4294967296.0)));
}

// =============================================================================
// ToLength
// =============================================================================

inline uint64_t toLength(const JSValue& value) {
    double len = toInteger(value);
    if (len <= 0) return 0;
    constexpr double MAX_SAFE = 9007199254740991.0;
    return static_cast<uint64_t>(std::min(len, MAX_SAFE));
}

// =============================================================================
// SameValue / SameValueZero
// =============================================================================

inline bool sameValue(const JSValue& x, const JSValue& y) {
    if (typeOf(x) != typeOf(y)) return false;
    
    if (auto px = std::get_if<double>(&x)) {
        auto py = std::get_if<double>(&y);
        if (std::isnan(*px) && std::isnan(*py)) return true;
        if (*px == 0 && *py == 0) {
            return std::signbit(*px) == std::signbit(*py);
        }
        return *px == *py;
    }
    
    return x == y;
}

inline bool sameValueZero(const JSValue& x, const JSValue& y) {
    if (typeOf(x) != typeOf(y)) return false;
    
    if (auto px = std::get_if<double>(&x)) {
        auto py = std::get_if<double>(&y);
        if (std::isnan(*px) && std::isnan(*py)) return true;
        return *px == *py;
    }
    
    return x == y;
}

} // namespace Zepra::Runtime
