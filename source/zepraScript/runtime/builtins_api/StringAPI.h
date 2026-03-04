/**
 * @file StringAPI.h
 * @brief String Implementation with ES2024 methods
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <regex>
#include <cctype>
#include <sstream>

namespace Zepra::Runtime {

class String {
public:
    String() = default;
    String(const char* str) : data_(str) {}
    String(const std::string& str) : data_(str) {}
    String(std::string&& str) : data_(std::move(str)) {}
    
    // Properties
    size_t length() const { return data_.length(); }
    bool empty() const { return data_.empty(); }
    
    // Static methods
    static String fromCharCode(int code) { return String(std::string(1, static_cast<char>(code))); }
    static String fromCodePoint(uint32_t codePoint) {
        std::string result;
        if (codePoint < 0x80) {
            result += static_cast<char>(codePoint);
        } else if (codePoint < 0x800) {
            result += static_cast<char>(0xC0 | (codePoint >> 6));
            result += static_cast<char>(0x80 | (codePoint & 0x3F));
        } else if (codePoint < 0x10000) {
            result += static_cast<char>(0xE0 | (codePoint >> 12));
            result += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (codePoint & 0x3F));
        } else {
            result += static_cast<char>(0xF0 | (codePoint >> 18));
            result += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
            result += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (codePoint & 0x3F));
        }
        return String(result);
    }
    
    // Character access
    std::optional<char> at(int index) const {
        if (index < 0) index += static_cast<int>(data_.length());
        if (index < 0 || index >= static_cast<int>(data_.length())) return std::nullopt;
        return data_[index];
    }
    
    std::optional<char> charAt(size_t index) const {
        if (index >= data_.length()) return std::nullopt;
        return data_[index];
    }
    
    std::optional<int> charCodeAt(size_t index) const {
        if (index >= data_.length()) return std::nullopt;
        return static_cast<unsigned char>(data_[index]);
    }
    
    std::optional<uint32_t> codePointAt(size_t index) const {
        if (index >= data_.length()) return std::nullopt;
        unsigned char c = data_[index];
        if (c < 0x80) return c;
        
        uint32_t codePoint = 0;
        int bytes = 0;
        if ((c & 0xE0) == 0xC0) { codePoint = c & 0x1F; bytes = 1; }
        else if ((c & 0xF0) == 0xE0) { codePoint = c & 0x0F; bytes = 2; }
        else if ((c & 0xF8) == 0xF0) { codePoint = c & 0x07; bytes = 3; }
        
        for (int i = 0; i < bytes && index + i + 1 < data_.length(); ++i) {
            codePoint = (codePoint << 6) | (data_[index + i + 1] & 0x3F);
        }
        return codePoint;
    }
    
    // Substring methods
    String slice(int start, int end = INT_MAX) const {
        int len = static_cast<int>(data_.length());
        if (start < 0) start = std::max(0, len + start);
        if (end < 0) end = std::max(0, len + end);
        end = std::min(end, len);
        if (start >= end) return String();
        return String(data_.substr(start, end - start));
    }
    
    String substring(size_t start, size_t end = std::string::npos) const {
        start = std::min(start, data_.length());
        end = std::min(end, data_.length());
        if (start > end) std::swap(start, end);
        return String(data_.substr(start, end - start));
    }
    
    String substr(int start, size_t length = std::string::npos) const {
        int len = static_cast<int>(data_.length());
        if (start < 0) start = std::max(0, len + start);
        return String(data_.substr(start, length));
    }
    
    // Case methods
    String toLowerCase() const {
        std::string result = data_;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return String(result);
    }
    
    String toUpperCase() const {
        std::string result = data_;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return String(result);
    }
    
    // Trim methods
    String trim() const { return trimStart().trimEnd(); }
    
    String trimStart() const {
        size_t start = 0;
        while (start < data_.length() && std::isspace(data_[start])) ++start;
        return String(data_.substr(start));
    }
    
    String trimEnd() const {
        size_t end = data_.length();
        while (end > 0 && std::isspace(data_[end - 1])) --end;
        return String(data_.substr(0, end));
    }
    
    // Padding
    String padStart(size_t targetLength, const std::string& padString = " ") const {
        if (data_.length() >= targetLength || padString.empty()) return *this;
        size_t padLen = targetLength - data_.length();
        std::string padding;
        while (padding.length() < padLen) padding += padString;
        return String(padding.substr(0, padLen) + data_);
    }
    
    String padEnd(size_t targetLength, const std::string& padString = " ") const {
        if (data_.length() >= targetLength || padString.empty()) return *this;
        size_t padLen = targetLength - data_.length();
        std::string padding;
        while (padding.length() < padLen) padding += padString;
        return String(data_ + padding.substr(0, padLen));
    }
    
    // Repeat
    String repeat(size_t count) const {
        std::string result;
        result.reserve(data_.length() * count);
        for (size_t i = 0; i < count; ++i) result += data_;
        return String(result);
    }
    
    // Search
    int indexOf(const std::string& search, size_t position = 0) const {
        size_t pos = data_.find(search, position);
        return pos != std::string::npos ? static_cast<int>(pos) : -1;
    }
    
    int lastIndexOf(const std::string& search, size_t position = std::string::npos) const {
        size_t pos = data_.rfind(search, position);
        return pos != std::string::npos ? static_cast<int>(pos) : -1;
    }
    
    bool includes(const std::string& search, size_t position = 0) const {
        return indexOf(search, position) != -1;
    }
    
    bool startsWith(const std::string& search, size_t position = 0) const {
        if (position + search.length() > data_.length()) return false;
        return data_.compare(position, search.length(), search) == 0;
    }
    
    bool endsWith(const std::string& search, size_t length = std::string::npos) const {
        length = std::min(length, data_.length());
        if (search.length() > length) return false;
        return data_.compare(length - search.length(), search.length(), search) == 0;
    }
    
    // Replace
    String replace(const std::string& search, const std::string& replacement) const {
        size_t pos = data_.find(search);
        if (pos == std::string::npos) return *this;
        std::string result = data_;
        result.replace(pos, search.length(), replacement);
        return String(result);
    }
    
    String replaceAll(const std::string& search, const std::string& replacement) const {
        if (search.empty()) return *this;
        std::string result = data_;
        size_t pos = 0;
        while ((pos = result.find(search, pos)) != std::string::npos) {
            result.replace(pos, search.length(), replacement);
            pos += replacement.length();
        }
        return String(result);
    }
    
    // Split
    std::vector<String> split(const std::string& separator, size_t limit = SIZE_MAX) const {
        std::vector<String> result;
        if (separator.empty()) {
            for (char c : data_) {
                if (result.size() >= limit) break;
                result.push_back(String(std::string(1, c)));
            }
            return result;
        }
        
        size_t start = 0, pos;
        while ((pos = data_.find(separator, start)) != std::string::npos && result.size() < limit) {
            result.push_back(String(data_.substr(start, pos - start)));
            start = pos + separator.length();
        }
        if (result.size() < limit) {
            result.push_back(String(data_.substr(start)));
        }
        return result;
    }
    
    // Concat
    String concat(const String& other) const { return String(data_ + other.data_); }
    String operator+(const String& other) const { return concat(other); }
    
    // Comparison
    int localeCompare(const String& other) const { return data_.compare(other.data_); }
    bool operator==(const String& other) const { return data_ == other.data_; }
    bool operator!=(const String& other) const { return data_ != other.data_; }
    bool operator<(const String& other) const { return data_ < other.data_; }
    
    // ES2024
    bool isWellFormed() const {
        for (size_t i = 0; i < data_.length(); ++i) {
            unsigned char c = data_[i];
            if (c >= 0xD8 && c <= 0xDF) {
                if (i + 1 >= data_.length()) return false;
                unsigned char c2 = data_[i + 1];
                if (c >= 0xDC || c2 < 0xDC || c2 > 0xDF) return false;
                ++i;
            }
        }
        return true;
    }
    
    String toWellFormed() const {
        if (isWellFormed()) return *this;
        std::string result;
        for (size_t i = 0; i < data_.length(); ++i) {
            unsigned char c = data_[i];
            if (c >= 0xD8 && c <= 0xDF) {
                result += "\xEF\xBF\xBD";
            } else {
                result += c;
            }
        }
        return String(result);
    }
    
    // Conversion
    const std::string& toString() const { return data_; }
    const char* c_str() const { return data_.c_str(); }
    operator std::string() const { return data_; }

private:
    std::string data_;
};

} // namespace Zepra::Runtime
