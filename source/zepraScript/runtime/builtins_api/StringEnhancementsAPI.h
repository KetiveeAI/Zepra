/**
 * @file StringEnhancementsAPI.h
 * @brief String Enhancements Implementation
 */

#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <regex>

namespace Zepra::Runtime {

// =============================================================================
// String Dedent
// =============================================================================

inline std::string dedent(const std::string& str) {
    std::vector<std::string> lines;
    std::istringstream iss(str);
    std::string line;
    
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    
    if (lines.empty()) return str;
    
    size_t minIndent = SIZE_MAX;
    for (const auto& l : lines) {
        if (l.empty()) continue;
        size_t indent = 0;
        while (indent < l.size() && (l[indent] == ' ' || l[indent] == '\t')) {
            ++indent;
        }
        if (indent < l.size()) {
            minIndent = std::min(minIndent, indent);
        }
    }
    
    if (minIndent == SIZE_MAX) minIndent = 0;
    
    std::ostringstream result;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) result << '\n';
        if (lines[i].size() > minIndent) {
            result << lines[i].substr(minIndent);
        }
    }
    
    return result.str();
}

// =============================================================================
// String Cooked (raw string to cooked)
// =============================================================================

inline std::string cooked(const std::string& raw) {
    std::string result;
    result.reserve(raw.size());
    
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\\' && i + 1 < raw.size()) {
            switch (raw[i + 1]) {
                case 'n': result += '\n'; ++i; break;
                case 'r': result += '\r'; ++i; break;
                case 't': result += '\t'; ++i; break;
                case '\\': result += '\\'; ++i; break;
                case '\'': result += '\''; ++i; break;
                case '"': result += '"'; ++i; break;
                case '0': result += '\0'; ++i; break;
                default: result += raw[i]; break;
            }
        } else {
            result += raw[i];
        }
    }
    
    return result;
}

// =============================================================================
// Unicode Utilities
// =============================================================================

class UnicodeString {
public:
    static bool isWellFormed(const std::string& str) {
        for (size_t i = 0; i < str.size(); ++i) {
            unsigned char c = str[i];
            if (c < 0x80) continue;
            
            int bytes = 0;
            if ((c & 0xE0) == 0xC0) bytes = 2;
            else if ((c & 0xF0) == 0xE0) bytes = 3;
            else if ((c & 0xF8) == 0xF0) bytes = 4;
            else return false;
            
            if (i + bytes > str.size()) return false;
            for (int j = 1; j < bytes; ++j) {
                if ((str[i + j] & 0xC0) != 0x80) return false;
            }
            i += bytes - 1;
        }
        return true;
    }
    
    static std::string toWellFormed(const std::string& str) {
        if (isWellFormed(str)) return str;
        
        std::string result;
        for (size_t i = 0; i < str.size(); ++i) {
            unsigned char c = str[i];
            if (c < 0x80) {
                result += c;
            } else if ((c & 0xE0) == 0xC0 && i + 1 < str.size() && (str[i+1] & 0xC0) == 0x80) {
                result += str.substr(i, 2);
                ++i;
            } else if ((c & 0xF0) == 0xE0 && i + 2 < str.size()) {
                result += str.substr(i, 3);
                i += 2;
            } else if ((c & 0xF8) == 0xF0 && i + 3 < str.size()) {
                result += str.substr(i, 4);
                i += 3;
            } else {
                result += "\xEF\xBF\xBD";
            }
        }
        return result;
    }
    
    static size_t codePointLength(const std::string& str) {
        size_t count = 0;
        for (size_t i = 0; i < str.size(); ++i) {
            unsigned char c = str[i];
            if ((c & 0xC0) != 0x80) ++count;
        }
        return count;
    }
    
    static std::vector<uint32_t> toCodePoints(const std::string& str) {
        std::vector<uint32_t> result;
        for (size_t i = 0; i < str.size(); ) {
            unsigned char c = str[i];
            uint32_t cp = 0;
            int bytes = 1;
            
            if (c < 0x80) {
                cp = c;
            } else if ((c & 0xE0) == 0xC0) {
                cp = c & 0x1F;
                bytes = 2;
            } else if ((c & 0xF0) == 0xE0) {
                cp = c & 0x0F;
                bytes = 3;
            } else if ((c & 0xF8) == 0xF0) {
                cp = c & 0x07;
                bytes = 4;
            }
            
            for (int j = 1; j < bytes && i + j < str.size(); ++j) {
                cp = (cp << 6) | (str[i + j] & 0x3F);
            }
            
            result.push_back(cp);
            i += bytes;
        }
        return result;
    }
};

// =============================================================================
// Additional String Methods
// =============================================================================

inline std::string capitalize(const std::string& str) {
    if (str.empty()) return str;
    std::string result = str;
    result[0] = std::toupper(result[0]);
    return result;
}

inline std::string reverse(const std::string& str) {
    return std::string(str.rbegin(), str.rend());
}

inline std::vector<std::string> words(const std::string& str) {
    std::vector<std::string> result;
    std::istringstream iss(str);
    std::string word;
    while (iss >> word) {
        result.push_back(word);
    }
    return result;
}

} // namespace Zepra::Runtime
