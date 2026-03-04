/**
 * @file GlobalEnhancementsAPI.h
 * @brief Global Object Enhancements
 */

#pragma once

#include <string>
#include <cmath>
#include <limits>
#include <optional>
#include <map>
#include <any>
#include <sstream>

namespace Zepra::Runtime {

// =============================================================================
// globalThis Utilities
// =============================================================================

class GlobalThis {
public:
    static GlobalThis& instance() {
        static GlobalThis global;
        return global;
    }
    
    template<typename T>
    void set(const std::string& name, T value) {
        properties_[name] = std::any(value);
    }
    
    template<typename T>
    std::optional<T> get(const std::string& name) const {
        auto it = properties_.find(name);
        if (it == properties_.end()) return std::nullopt;
        try {
            return std::any_cast<T>(it->second);
        } catch (...) {
            return std::nullopt;
        }
    }
    
    bool has(const std::string& name) const {
        return properties_.find(name) != properties_.end();
    }

private:
    std::map<std::string, std::any> properties_;
};

// =============================================================================
// Enhanced Number Checks
// =============================================================================

inline bool isNaN(double x) {
    return std::isnan(x);
}

inline bool isFinite(double x) {
    return std::isfinite(x);
}

inline bool isInteger(double x) {
    return std::isfinite(x) && std::trunc(x) == x;
}

inline bool isSafeInteger(double x) {
    constexpr double MAX_SAFE = 9007199254740991.0;
    constexpr double MIN_SAFE = -9007199254740991.0;
    return isInteger(x) && x >= MIN_SAFE && x <= MAX_SAFE;
}

// =============================================================================
// Enhanced parseInt/parseFloat
// =============================================================================

inline std::optional<int64_t> parseIntStrict(const std::string& str, int radix = 10) {
    if (str.empty()) return std::nullopt;
    if (radix < 2 || radix > 36) return std::nullopt;
    
    size_t idx = 0;
    while (idx < str.size() && std::isspace(str[idx])) ++idx;
    if (idx == str.size()) return std::nullopt;
    
    bool negative = false;
    if (str[idx] == '-') { negative = true; ++idx; }
    else if (str[idx] == '+') { ++idx; }
    
    if (idx == str.size()) return std::nullopt;
    
    if (radix == 16 && idx + 1 < str.size() && str[idx] == '0' && 
        (str[idx + 1] == 'x' || str[idx + 1] == 'X')) {
        idx += 2;
    }
    
    int64_t result = 0;
    bool hasDigit = false;
    
    while (idx < str.size()) {
        char c = str[idx];
        int digit = -1;
        
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'z') digit = 10 + c - 'a';
        else if (c >= 'A' && c <= 'Z') digit = 10 + c - 'A';
        
        if (digit < 0 || digit >= radix) break;
        
        result = result * radix + digit;
        hasDigit = true;
        ++idx;
    }
    
    if (!hasDigit) return std::nullopt;
    return negative ? -result : result;
}

inline std::optional<double> parseFloatStrict(const std::string& str) {
    if (str.empty()) return std::nullopt;
    
    size_t idx = 0;
    while (idx < str.size() && std::isspace(str[idx])) ++idx;
    if (idx == str.size()) return std::nullopt;
    
    try {
        size_t pos = 0;
        double result = std::stod(str.substr(idx), &pos);
        if (pos == 0) return std::nullopt;
        return result;
    } catch (...) {
        return std::nullopt;
    }
}

// =============================================================================
// Type Conversion Utilities
// =============================================================================

inline std::string toString(double x) {
    if (std::isnan(x)) return "NaN";
    if (std::isinf(x)) return x > 0 ? "Infinity" : "-Infinity";
    
    std::ostringstream oss;
    oss << x;
    return oss.str();
}

inline double toNumber(const std::string& str) {
    auto result = parseFloatStrict(str);
    return result.value_or(std::numeric_limits<double>::quiet_NaN());
}

inline bool toBoolean(const std::string& str) {
    return !str.empty();
}

inline bool toBoolean(double x) {
    return x != 0 && !std::isnan(x);
}

// =============================================================================
// Object Type Checking
// =============================================================================

template<typename T>
std::string typeOf(const T&) {
    if constexpr (std::is_same_v<T, std::nullptr_t>) return "object";
    if constexpr (std::is_same_v<T, bool>) return "boolean";
    if constexpr (std::is_arithmetic_v<T>) return "number";
    if constexpr (std::is_same_v<T, std::string>) return "string";
    if constexpr (std::is_function_v<T>) return "function";
    return "object";
}

} // namespace Zepra::Runtime
