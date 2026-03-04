/**
 * @file TextEncodingAPI.h
 * @brief Text Encoding/Decoding Implementation
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <optional>

namespace Zepra::Runtime {

// =============================================================================
// TextEncoder
// =============================================================================

class TextEncoder {
public:
    TextEncoder() : encoding_("utf-8") {}
    
    const std::string& encoding() const { return encoding_; }
    
    std::vector<uint8_t> encode(const std::string& input) const {
        return std::vector<uint8_t>(input.begin(), input.end());
    }
    
    struct EncodeIntoResult {
        size_t read = 0;
        size_t written = 0;
    };
    
    EncodeIntoResult encodeInto(const std::string& source, uint8_t* destination, size_t destLength) const {
        EncodeIntoResult result;
        size_t srcLen = source.length();
        result.read = std::min(srcLen, destLength);
        result.written = result.read;
        std::copy(source.begin(), source.begin() + result.read, destination);
        return result;
    }

private:
    std::string encoding_;
};

// =============================================================================
// TextDecoder
// =============================================================================

class TextDecoder {
public:
    struct Options {
        bool fatal = false;
        bool ignoreBOM = false;
    };
    
    TextDecoder(const std::string& label = "utf-8", Options options = {})
        : encoding_(normalizeEncoding(label)), options_(options) {}
    
    const std::string& encoding() const { return encoding_; }
    bool fatal() const { return options_.fatal; }
    bool ignoreBOM() const { return options_.ignoreBOM; }
    
    std::string decode(const std::vector<uint8_t>& input, bool stream = false) {
        std::string result;
        result.reserve(input.size());
        
        size_t start = 0;
        if (!ignoreBOM() && input.size() >= 3) {
            if (input[0] == 0xEF && input[1] == 0xBB && input[2] == 0xBF) {
                start = 3;
            }
        }
        
        for (size_t i = start; i < input.size(); ++i) {
            uint8_t c = input[i];
            
            if (c < 0x80) {
                result += static_cast<char>(c);
            } else if ((c & 0xE0) == 0xC0 && i + 1 < input.size()) {
                result += static_cast<char>(c);
                result += static_cast<char>(input[++i]);
            } else if ((c & 0xF0) == 0xE0 && i + 2 < input.size()) {
                result += static_cast<char>(c);
                result += static_cast<char>(input[++i]);
                result += static_cast<char>(input[++i]);
            } else if ((c & 0xF8) == 0xF0 && i + 3 < input.size()) {
                result += static_cast<char>(c);
                result += static_cast<char>(input[++i]);
                result += static_cast<char>(input[++i]);
                result += static_cast<char>(input[++i]);
            } else if (fatal()) {
                throw std::runtime_error("Invalid UTF-8 sequence");
            } else {
                result += "\xEF\xBF\xBD";
            }
        }
        
        return result;
    }
    
    std::string decode(const uint8_t* data, size_t length, bool stream = false) {
        return decode(std::vector<uint8_t>(data, data + length), stream);
    }

private:
    static std::string normalizeEncoding(const std::string& label) {
        std::string lower;
        for (char c : label) lower += std::tolower(c);
        if (lower == "utf-8" || lower == "utf8") return "utf-8";
        if (lower == "utf-16" || lower == "utf16") return "utf-16";
        if (lower == "utf-16le") return "utf-16le";
        if (lower == "utf-16be") return "utf-16be";
        if (lower == "ascii" || lower == "us-ascii") return "ascii";
        if (lower == "iso-8859-1" || lower == "latin1") return "iso-8859-1";
        return "utf-8";
    }
    
    std::string encoding_;
    Options options_;
};

// =============================================================================
// TextEncoderStream / TextDecoderStream (for streaming)
// =============================================================================

class TextEncoderStream {
public:
    std::vector<uint8_t> transform(const std::string& chunk) {
        return encoder_.encode(chunk);
    }

private:
    TextEncoder encoder_;
};

class TextDecoderStream {
public:
    explicit TextDecoderStream(const std::string& label = "utf-8") : decoder_(label) {}
    
    std::string transform(const std::vector<uint8_t>& chunk) {
        return decoder_.decode(chunk, true);
    }

private:
    TextDecoder decoder_;
};

} // namespace Zepra::Runtime
