/**
 * @file URIAPI.h
 * @brief URI Encoding/Decoding
 */

#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <stdexcept>

namespace Zepra::Runtime {

class URI {
public:
    static std::string encodeURI(const std::string& str) {
        return encode(str, uriReserved_);
    }
    
    static std::string decodeURI(const std::string& str) {
        return decode(str, uriReserved_);
    }
    
    static std::string encodeURIComponent(const std::string& str) {
        return encode(str, "");
    }
    
    static std::string decodeURIComponent(const std::string& str) {
        return decode(str, "");
    }
    
    static std::string escape(const std::string& str) {
        std::ostringstream result;
        for (unsigned char c : str) {
            if (isAlphanumeric(c) || c == '@' || c == '*' || c == '_' || 
                c == '+' || c == '-' || c == '.' || c == '/') {
                result << c;
            } else if (c < 256) {
                result << '%' << std::uppercase << std::hex << std::setw(2) 
                       << std::setfill('0') << static_cast<int>(c);
            }
        }
        return result.str();
    }
    
    static std::string unescape(const std::string& str) {
        std::string result;
        for (size_t i = 0; i < str.length(); ++i) {
            if (str[i] == '%' && i + 2 < str.length()) {
                int value = 0;
                if (parseHex(str[i + 1], str[i + 2], value)) {
                    result += static_cast<char>(value);
                    i += 2;
                } else {
                    result += str[i];
                }
            } else {
                result += str[i];
            }
        }
        return result;
    }

private:
    static constexpr const char* uriReserved_ = ";/?:@&=+$,#";
    static constexpr const char* uriUnescaped_ = "-_.!~*'()";
    
    static bool isAlphanumeric(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
    }
    
    static bool isUnescaped(char c) {
        if (isAlphanumeric(c)) return true;
        for (const char* p = uriUnescaped_; *p; ++p) {
            if (c == *p) return true;
        }
        return false;
    }
    
    static bool isReserved(char c, const std::string& reserved) {
        return reserved.find(c) != std::string::npos;
    }
    
    static std::string encode(const std::string& str, const std::string& dontEncode) {
        std::ostringstream result;
        for (size_t i = 0; i < str.length(); ++i) {
            unsigned char c = str[i];
            
            if (isUnescaped(c) || isReserved(c, dontEncode)) {
                result << c;
            } else if ((c & 0x80) == 0) {
                result << '%' << std::uppercase << std::hex << std::setw(2) 
                       << std::setfill('0') << static_cast<int>(c);
            } else {
                int bytes = 0;
                if ((c & 0xE0) == 0xC0) bytes = 2;
                else if ((c & 0xF0) == 0xE0) bytes = 3;
                else if ((c & 0xF8) == 0xF0) bytes = 4;
                else throw std::runtime_error("URIError: Invalid UTF-8");
                
                for (int j = 0; j < bytes && i < str.length(); ++j, ++i) {
                    result << '%' << std::uppercase << std::hex << std::setw(2) 
                           << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(str[i]));
                }
                --i;
            }
        }
        return result.str();
    }
    
    static std::string decode(const std::string& str, const std::string& dontDecode) {
        std::string result;
        for (size_t i = 0; i < str.length(); ++i) {
            if (str[i] == '%' && i + 2 < str.length()) {
                int value = 0;
                if (parseHex(str[i + 1], str[i + 2], value)) {
                    char c = static_cast<char>(value);
                    if (isReserved(c, dontDecode)) {
                        result += str.substr(i, 3);
                    } else {
                        result += c;
                    }
                    i += 2;
                } else {
                    result += str[i];
                }
            } else {
                result += str[i];
            }
        }
        return result;
    }
    
    static bool parseHex(char h1, char h2, int& result) {
        int v1 = hexValue(h1);
        int v2 = hexValue(h2);
        if (v1 < 0 || v2 < 0) return false;
        result = (v1 << 4) | v2;
        return true;
    }
    
    static int hexValue(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + c - 'a';
        if (c >= 'A' && c <= 'F') return 10 + c - 'A';
        return -1;
    }
};

// Global functions
inline std::string encodeURI(const std::string& str) { return URI::encodeURI(str); }
inline std::string decodeURI(const std::string& str) { return URI::decodeURI(str); }
inline std::string encodeURIComponent(const std::string& str) { return URI::encodeURIComponent(str); }
inline std::string decodeURIComponent(const std::string& str) { return URI::decodeURIComponent(str); }
inline std::string escape(const std::string& str) { return URI::escape(str); }
inline std::string unescape(const std::string& str) { return URI::unescape(str); }

} // namespace Zepra::Runtime
