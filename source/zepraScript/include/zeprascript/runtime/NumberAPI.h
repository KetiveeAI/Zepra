/**
 * @file NumberAPI.h
 * @brief Number Implementation
 */

#pragma once

#include <cmath>
#include <limits>
#include <string>
#include <sstream>
#include <iomanip>
#include <optional>

namespace Zepra::Runtime {

class Number {
public:
    static constexpr double MAX_VALUE = std::numeric_limits<double>::max();
    static constexpr double MIN_VALUE = std::numeric_limits<double>::min();
    static constexpr double MAX_SAFE_INTEGER = 9007199254740991.0;
    static constexpr double MIN_SAFE_INTEGER = -9007199254740991.0;
    static constexpr double POSITIVE_INFINITY = std::numeric_limits<double>::infinity();
    static constexpr double NEGATIVE_INFINITY = -std::numeric_limits<double>::infinity();
    static constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
    static constexpr double EPSILON = std::numeric_limits<double>::epsilon();
    
    Number() : value_(0.0) {}
    Number(double val) : value_(val) {}
    Number(int val) : value_(static_cast<double>(val)) {}
    
    // Static methods
    static bool isNaN(double value) { return std::isnan(value); }
    static bool isFinite(double value) { return std::isfinite(value); }
    static bool isInteger(double value) { return std::isfinite(value) && std::trunc(value) == value; }
    
    static bool isSafeInteger(double value) {
        return isInteger(value) && value >= MIN_SAFE_INTEGER && value <= MAX_SAFE_INTEGER;
    }
    
    static std::optional<double> parseFloat(const std::string& str) {
        try {
            size_t pos;
            double result = std::stod(str, &pos);
            return result;
        } catch (...) {
            return std::nullopt;
        }
    }
    
    static std::optional<int64_t> parseInt(const std::string& str, int radix = 10) {
        if (radix < 2 || radix > 36) return std::nullopt;
        try {
            size_t pos;
            int64_t result = std::stoll(str, &pos, radix);
            return result;
        } catch (...) {
            return std::nullopt;
        }
    }
    
    // Instance methods
    std::string toFixed(int digits = 0) const {
        if (digits < 0 || digits > 100) return "";
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(digits) << value_;
        return oss.str();
    }
    
    std::string toExponential(int digits = 6) const {
        if (digits < 0 || digits > 100) return "";
        std::ostringstream oss;
        oss << std::scientific << std::setprecision(digits) << value_;
        return oss.str();
    }
    
    std::string toPrecision(int precision = 6) const {
        if (precision < 1 || precision > 100) return "";
        std::ostringstream oss;
        oss << std::setprecision(precision) << value_;
        return oss.str();
    }
    
    std::string toString(int radix = 10) const {
        if (radix < 2 || radix > 36) return "";
        if (radix == 10) {
            std::ostringstream oss;
            oss << std::setprecision(17) << value_;
            return oss.str();
        }
        
        if (!std::isfinite(value_)) {
            if (std::isnan(value_)) return "NaN";
            return value_ > 0 ? "Infinity" : "-Infinity";
        }
        
        bool negative = value_ < 0;
        int64_t intPart = static_cast<int64_t>(std::abs(value_));
        
        if (intPart == 0) return "0";
        
        std::string result;
        while (intPart > 0) {
            int digit = intPart % radix;
            result = (digit < 10 ? char('0' + digit) : char('a' + digit - 10)) + result;
            intPart /= radix;
        }
        
        return negative ? "-" + result : result;
    }
    
    double valueOf() const { return value_; }
    operator double() const { return value_; }
    
    // Comparison
    bool operator==(const Number& other) const { return value_ == other.value_; }
    bool operator!=(const Number& other) const { return value_ != other.value_; }
    bool operator<(const Number& other) const { return value_ < other.value_; }
    bool operator<=(const Number& other) const { return value_ <= other.value_; }
    bool operator>(const Number& other) const { return value_ > other.value_; }
    bool operator>=(const Number& other) const { return value_ >= other.value_; }

private:
    double value_;
};

} // namespace Zepra::Runtime
