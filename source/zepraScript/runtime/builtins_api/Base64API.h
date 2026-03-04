/**
 * @file Base64API.h
 * @brief Base64 Encoding/Decoding Implementation
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace Zepra::Runtime {

// =============================================================================
// Base64
// =============================================================================

class Base64 {
public:
    static std::string encode(const std::vector<uint8_t>& data) {
        static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        
        std::string result;
        result.reserve((data.size() + 2) / 3 * 4);
        
        size_t i = 0;
        while (i < data.size()) {
            size_t remaining = data.size() - i;
            
            uint32_t n = data[i++] << 16;
            if (remaining > 1) n |= data[i++] << 8;
            if (remaining > 2) n |= data[i++];
            
            result += chars[(n >> 18) & 0x3F];
            result += chars[(n >> 12) & 0x3F];
            result += (remaining > 1) ? chars[(n >> 6) & 0x3F] : '=';
            result += (remaining > 2) ? chars[n & 0x3F] : '=';
        }
        
        return result;
    }
    
    static std::string encode(const std::string& str) {
        return encode(std::vector<uint8_t>(str.begin(), str.end()));
    }
    
    static std::vector<uint8_t> decode(const std::string& encoded) {
        static const int8_t lookup[] = {
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
            52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
            -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
            15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
            -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
            41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
        };
        
        std::vector<uint8_t> result;
        result.reserve(encoded.size() * 3 / 4);
        
        for (size_t i = 0; i < encoded.size(); i += 4) {
            if (i + 4 > encoded.size()) break;
            
            // Count non-padding characters
            int validChars = 4;
            if (encoded[i + 3] == '=') validChars--;
            if (encoded[i + 2] == '=') validChars--;
            
            uint32_t n = 0;
            for (int j = 0; j < validChars; ++j) {
                char c = encoded[i + j];
                if (c < 0 || c >= 128) throw std::runtime_error("Invalid Base64");
                int8_t val = lookup[static_cast<uint8_t>(c)];
                if (val < 0) throw std::runtime_error("Invalid Base64");
                n = (n << 6) | val;
            }
            
            // Shift to account for missing characters
            n <<= (4 - validChars) * 6;
            
            result.push_back((n >> 16) & 0xFF);
            if (validChars > 2) result.push_back((n >> 8) & 0xFF);
            if (validChars > 3) result.push_back(n & 0xFF);
        }
        
        return result;
    }
    
    static std::string decodeToString(const std::string& encoded) {
        auto bytes = decode(encoded);
        return std::string(bytes.begin(), bytes.end());
    }
};

// =============================================================================
// btoa / atob (Web API compatible)
// =============================================================================

inline std::string btoa(const std::string& binaryString) {
    for (char c : binaryString) {
        if (static_cast<unsigned char>(c) > 255) {
            throw std::runtime_error("Invalid character for btoa");
        }
    }
    return Base64::encode(binaryString);
}

inline std::string atob(const std::string& encoded) {
    return Base64::decodeToString(encoded);
}

// =============================================================================
// Base64URL (URL-safe variant)
// =============================================================================

class Base64URL {
public:
    static std::string encode(const std::vector<uint8_t>& data) {
        std::string result = Base64::encode(data);
        
        for (char& c : result) {
            if (c == '+') c = '-';
            else if (c == '/') c = '_';
        }
        
        while (!result.empty() && result.back() == '=') {
            result.pop_back();
        }
        
        return result;
    }
    
    static std::vector<uint8_t> decode(std::string encoded) {
        for (char& c : encoded) {
            if (c == '-') c = '+';
            else if (c == '_') c = '/';
        }
        
        while (encoded.size() % 4 != 0) {
            encoded += '=';
        }
        
        return Base64::decode(encoded);
    }
};

} // namespace Zepra::Runtime
