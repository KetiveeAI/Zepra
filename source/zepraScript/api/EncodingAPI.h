/**
 * @file EncodingAPI.h
 * @brief Text Encoding API
 * 
 * Encoding Standard:
 * - TextEncoder: String → UTF-8 bytes
 * - TextDecoder: Bytes → String
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace Zepra::API {

// =============================================================================
// TextEncoder
// =============================================================================

/**
 * @brief Encode strings to UTF-8 bytes
 */
class TextEncoder {
public:
    TextEncoder() = default;
    
    // Encoding label
    std::string encoding() const { return "utf-8"; }
    
    // Encode string to bytes
    std::vector<uint8_t> encode(const std::string& input) const {
        // Already UTF-8 in C++
        return std::vector<uint8_t>(input.begin(), input.end());
    }
    
    // Encode into existing buffer (returns {read, written})
    struct EncodeResult {
        size_t read;
        size_t written;
    };
    
    EncodeResult encodeInto(const std::string& source, 
                            std::vector<uint8_t>& destination) const {
        size_t toWrite = std::min(source.size(), destination.size());
        std::copy(source.begin(), source.begin() + toWrite, destination.begin());
        return {toWrite, toWrite};
    }
};

// =============================================================================
// TextDecoder
// =============================================================================

struct TextDecodeOptions {
    bool stream = false;  // Incomplete sequences expected
};

/**
 * @brief Decode bytes to string
 */
class TextDecoder {
public:
    explicit TextDecoder(const std::string& label = "utf-8", 
                         bool fatal = false,
                         bool ignoreBOM = false)
        : encoding_(normalizeLabel(label))
        , fatal_(fatal)
        , ignoreBOM_(ignoreBOM) {
        
        if (encoding_.empty()) {
            throw std::runtime_error("Invalid encoding label");
        }
    }
    
    // Properties
    const std::string& encoding() const { return encoding_; }
    bool fatal() const { return fatal_; }
    bool ignoreBOM() const { return ignoreBOM_; }
    
    // Decode bytes to string
    std::string decode(const std::vector<uint8_t>& input,
                       const TextDecodeOptions& options = {}) {
        if (encoding_ == "utf-8") {
            return decodeUTF8(input, options);
        } else if (encoding_ == "utf-16le") {
            return decodeUTF16LE(input);
        } else if (encoding_ == "utf-16be") {
            return decodeUTF16BE(input);
        }
        return std::string(input.begin(), input.end());
    }
    
    // Decode from raw pointer
    std::string decode(const uint8_t* data, size_t length,
                       const TextDecodeOptions& options = {}) {
        return decode(std::vector<uint8_t>(data, data + length), options);
    }
    
private:
    static std::string normalizeLabel(const std::string& label) {
        std::string lower;
        for (char c : label) {
            lower += std::tolower(c);
        }
        
        // Map common labels
        if (lower == "utf-8" || lower == "utf8") return "utf-8";
        if (lower == "utf-16le" || lower == "utf-16") return "utf-16le";
        if (lower == "utf-16be") return "utf-16be";
        if (lower == "ascii" || lower == "us-ascii") return "ascii";
        if (lower == "iso-8859-1" || lower == "latin1") return "iso-8859-1";
        
        return lower;
    }
    
    std::string decodeUTF8(const std::vector<uint8_t>& input,
                           const TextDecodeOptions& options) {
        std::string result;
        result.reserve(input.size());
        
        size_t i = 0;
        
        // Skip BOM if present
        if (!ignoreBOM_ && input.size() >= 3 &&
            input[0] == 0xEF && input[1] == 0xBB && input[2] == 0xBF) {
            i = 3;
        }
        
        while (i < input.size()) {
            uint8_t c = input[i];
            
            if (c < 0x80) {
                result += static_cast<char>(c);
                i++;
            } else if ((c & 0xE0) == 0xC0) {
                if (i + 1 >= input.size()) {
                    if (options.stream) break;
                    if (fatal_) throw std::runtime_error("Invalid UTF-8");
                    result += '\xEF'; result += '\xBF'; result += '\xBD';
                    break;
                }
                result += static_cast<char>(c);
                result += static_cast<char>(input[i + 1]);
                i += 2;
            } else if ((c & 0xF0) == 0xE0) {
                if (i + 2 >= input.size()) {
                    if (options.stream) break;
                    if (fatal_) throw std::runtime_error("Invalid UTF-8");
                    result += '\xEF'; result += '\xBF'; result += '\xBD';
                    break;
                }
                result += static_cast<char>(c);
                result += static_cast<char>(input[i + 1]);
                result += static_cast<char>(input[i + 2]);
                i += 3;
            } else if ((c & 0xF8) == 0xF0) {
                if (i + 3 >= input.size()) {
                    if (options.stream) break;
                    if (fatal_) throw std::runtime_error("Invalid UTF-8");
                    result += '\xEF'; result += '\xBF'; result += '\xBD';
                    break;
                }
                result += static_cast<char>(c);
                result += static_cast<char>(input[i + 1]);
                result += static_cast<char>(input[i + 2]);
                result += static_cast<char>(input[i + 3]);
                i += 4;
            } else {
                if (fatal_) throw std::runtime_error("Invalid UTF-8 byte");
                result += '\xEF'; result += '\xBF'; result += '\xBD';
                i++;
            }
        }
        
        return result;
    }
    
    std::string decodeUTF16LE(const std::vector<uint8_t>& input) {
        std::string result;
        // Simplified: would do proper UTF-16 to UTF-8 conversion
        for (size_t i = 0; i + 1 < input.size(); i += 2) {
            uint16_t codePoint = input[i] | (input[i + 1] << 8);
            if (codePoint < 0x80) {
                result += static_cast<char>(codePoint);
            } else if (codePoint < 0x800) {
                result += static_cast<char>(0xC0 | (codePoint >> 6));
                result += static_cast<char>(0x80 | (codePoint & 0x3F));
            } else {
                result += static_cast<char>(0xE0 | (codePoint >> 12));
                result += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (codePoint & 0x3F));
            }
        }
        return result;
    }
    
    std::string decodeUTF16BE(const std::vector<uint8_t>& input) {
        std::string result;
        for (size_t i = 0; i + 1 < input.size(); i += 2) {
            uint16_t codePoint = (input[i] << 8) | input[i + 1];
            if (codePoint < 0x80) {
                result += static_cast<char>(codePoint);
            } else if (codePoint < 0x800) {
                result += static_cast<char>(0xC0 | (codePoint >> 6));
                result += static_cast<char>(0x80 | (codePoint & 0x3F));
            } else {
                result += static_cast<char>(0xE0 | (codePoint >> 12));
                result += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (codePoint & 0x3F));
            }
        }
        return result;
    }
    
    std::string encoding_;
    bool fatal_;
    bool ignoreBOM_;
};

} // namespace Zepra::API
