/**
 * @file FormattingAPI.h
 * @brief String Formatting Utilities
 */

#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstdarg>
#include <stdexcept>

namespace Zepra::Runtime {

// =============================================================================
// Printf-style Formatting
// =============================================================================

inline std::string format(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    va_list argsCopy;
    va_copy(argsCopy, args);
    int size = std::vsnprintf(nullptr, 0, fmt, argsCopy) + 1;
    va_end(argsCopy);
    
    std::string result(size, '\0');
    std::vsnprintf(result.data(), size, fmt, args);
    va_end(args);
    
    result.pop_back();
    return result;
}

// =============================================================================
// Template Literal Formatter
// =============================================================================

class TemplateFormatter {
public:
    template<typename... Args>
    static std::string format(const std::string& tmpl, Args&&... args) {
        std::vector<std::string> values = {toString(std::forward<Args>(args))...};
        return substitutePositional(tmpl, values);
    }
    
    static std::string formatNamed(const std::string& tmpl,
                                   const std::map<std::string, std::string>& values) {
        std::string result = tmpl;
        for (const auto& [key, value] : values) {
            std::string placeholder = "${" + key + "}";
            size_t pos;
            while ((pos = result.find(placeholder)) != std::string::npos) {
                result.replace(pos, placeholder.length(), value);
            }
        }
        return result;
    }

private:
    template<typename T>
    static std::string toString(const T& value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }
    
    static std::string substitutePositional(const std::string& tmpl,
                                           const std::vector<std::string>& values) {
        std::string result = tmpl;
        for (size_t i = 0; i < values.size(); ++i) {
            std::string placeholder = "${" + std::to_string(i) + "}";
            size_t pos;
            while ((pos = result.find(placeholder)) != std::string::npos) {
                result.replace(pos, placeholder.length(), values[i]);
            }
        }
        return result;
    }
};

// =============================================================================
// Number Formatting
// =============================================================================

inline std::string formatNumber(double value, int precision = 2) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

inline std::string formatWithCommas(int64_t value) {
    std::string str = std::to_string(std::abs(value));
    std::string result;
    
    int count = 0;
    for (auto it = str.rbegin(); it != str.rend(); ++it) {
        if (count > 0 && count % 3 == 0) {
            result = ',' + result;
        }
        result = *it + result;
        ++count;
    }
    
    if (value < 0) result = '-' + result;
    return result;
}

inline std::string formatBytes(uint64_t bytes, int precision = 2) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    
    double size = static_cast<double>(bytes);
    int unitIndex = 0;
    
    while (size >= 1024 && unitIndex < 5) {
        size /= 1024;
        ++unitIndex;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << size << " " << units[unitIndex];
    return oss.str();
}

inline std::string formatPercentage(double value, int precision = 1) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << (value * 100) << "%";
    return oss.str();
}

// =============================================================================
// Padding
// =============================================================================

inline std::string padStart(const std::string& str, size_t targetLength, char padChar = ' ') {
    if (str.length() >= targetLength) return str;
    return std::string(targetLength - str.length(), padChar) + str;
}

inline std::string padEnd(const std::string& str, size_t targetLength, char padChar = ' ') {
    if (str.length() >= targetLength) return str;
    return str + std::string(targetLength - str.length(), padChar);
}

inline std::string center(const std::string& str, size_t width, char padChar = ' ') {
    if (str.length() >= width) return str;
    size_t padding = width - str.length();
    size_t left = padding / 2;
    size_t right = padding - left;
    return std::string(left, padChar) + str + std::string(right, padChar);
}

// =============================================================================
// Case Conversion
// =============================================================================

inline std::string toUpperCase(const std::string& str) {
    std::string result = str;
    for (char& c : result) c = std::toupper(c);
    return result;
}

inline std::string toLowerCase(const std::string& str) {
    std::string result = str;
    for (char& c : result) c = std::tolower(c);
    return result;
}

inline std::string toTitleCase(const std::string& str) {
    std::string result = str;
    bool capitalizeNext = true;
    for (char& c : result) {
        if (std::isspace(c)) {
            capitalizeNext = true;
        } else if (capitalizeNext) {
            c = std::toupper(c);
            capitalizeNext = false;
        } else {
            c = std::tolower(c);
        }
    }
    return result;
}

} // namespace Zepra::Runtime
