/**
 * @file unicode.cpp
 * @brief UTF-8/UTF-16 conversion and codepoint operations
 */

#include "config.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace Zepra::Utils {

// =============================================================================
// UTF-8 Decoding
// =============================================================================

static constexpr uint32_t REPLACEMENT_CHAR = 0xFFFD;

uint32_t decodeUtf8(const char* data, size_t len, size_t& pos) {
    if (pos >= len) return REPLACEMENT_CHAR;

    uint8_t b0 = static_cast<uint8_t>(data[pos]);

    // 1-byte (ASCII)
    if (b0 < 0x80) {
        pos += 1;
        return b0;
    }

    // 2-byte
    if ((b0 & 0xE0) == 0xC0) {
        if (pos + 1 >= len) { pos++; return REPLACEMENT_CHAR; }
        uint8_t b1 = static_cast<uint8_t>(data[pos + 1]);
        if ((b1 & 0xC0) != 0x80) { pos++; return REPLACEMENT_CHAR; }
        pos += 2;
        uint32_t cp = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
        return (cp < 0x80) ? REPLACEMENT_CHAR : cp; // overlong
    }

    // 3-byte
    if ((b0 & 0xF0) == 0xE0) {
        if (pos + 2 >= len) { pos++; return REPLACEMENT_CHAR; }
        uint8_t b1 = static_cast<uint8_t>(data[pos + 1]);
        uint8_t b2 = static_cast<uint8_t>(data[pos + 2]);
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) { pos++; return REPLACEMENT_CHAR; }
        pos += 3;
        uint32_t cp = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
        if (cp < 0x800) return REPLACEMENT_CHAR; // overlong
        if (cp >= 0xD800 && cp <= 0xDFFF) return REPLACEMENT_CHAR; // surrogate
        return cp;
    }

    // 4-byte
    if ((b0 & 0xF8) == 0xF0) {
        if (pos + 3 >= len) { pos++; return REPLACEMENT_CHAR; }
        uint8_t b1 = static_cast<uint8_t>(data[pos + 1]);
        uint8_t b2 = static_cast<uint8_t>(data[pos + 2]);
        uint8_t b3 = static_cast<uint8_t>(data[pos + 3]);
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) {
            pos++; return REPLACEMENT_CHAR;
        }
        pos += 4;
        uint32_t cp = ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) |
                      ((b2 & 0x3F) << 6) | (b3 & 0x3F);
        if (cp < 0x10000 || cp > 0x10FFFF) return REPLACEMENT_CHAR;
        return cp;
    }

    pos++;
    return REPLACEMENT_CHAR;
}

// =============================================================================
// UTF-8 Encoding
// =============================================================================

void encodeUtf8(uint32_t cp, std::string& out) {
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0x10FFFF) {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

// =============================================================================
// Codepoint Count (JS String.length semantics — UTF-16 code unit count)
// =============================================================================

size_t utf16Length(const std::string& utf8) {
    size_t count = 0;
    size_t pos = 0;
    while (pos < utf8.size()) {
        uint32_t cp = decodeUtf8(utf8.data(), utf8.size(), pos);
        count += (cp >= 0x10000) ? 2 : 1; // surrogate pair
    }
    return count;
}

size_t codepointCount(const std::string& utf8) {
    size_t count = 0;
    size_t pos = 0;
    while (pos < utf8.size()) {
        decodeUtf8(utf8.data(), utf8.size(), pos);
        count++;
    }
    return count;
}

// =============================================================================
// UTF-8 → UTF-16 Conversion
// =============================================================================

std::vector<uint16_t> utf8ToUtf16(const std::string& utf8) {
    std::vector<uint16_t> result;
    result.reserve(utf8.size());

    size_t pos = 0;
    while (pos < utf8.size()) {
        uint32_t cp = decodeUtf8(utf8.data(), utf8.size(), pos);
        if (cp < 0x10000) {
            result.push_back(static_cast<uint16_t>(cp));
        } else {
            // Surrogate pair
            cp -= 0x10000;
            result.push_back(static_cast<uint16_t>(0xD800 + (cp >> 10)));
            result.push_back(static_cast<uint16_t>(0xDC00 + (cp & 0x3FF)));
        }
    }
    return result;
}

// =============================================================================
// UTF-16 → UTF-8 Conversion
// =============================================================================

std::string utf16ToUtf8(const uint16_t* data, size_t len) {
    std::string result;
    result.reserve(len);

    for (size_t i = 0; i < len; i++) {
        uint32_t cp = data[i];

        // Check for surrogate pair
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < len) {
            uint16_t lo = data[i + 1];
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                i++;
            }
        }

        encodeUtf8(cp, result);
    }
    return result;
}

// =============================================================================
// Codepoint Access (for String.prototype.codePointAt)
// =============================================================================

uint32_t codepointAt(const std::string& utf8, size_t index) {
    size_t pos = 0;
    size_t current = 0;
    while (pos < utf8.size()) {
        uint32_t cp = decodeUtf8(utf8.data(), utf8.size(), pos);
        if (current == index) return cp;
        current++;
    }
    return REPLACEMENT_CHAR;
}

// =============================================================================
// String.fromCodePoint
// =============================================================================

std::string fromCodePoint(const std::vector<uint32_t>& codepoints) {
    std::string result;
    for (uint32_t cp : codepoints) {
        if (cp > 0x10FFFF) {
            throw std::range_error("Invalid code point");
        }
        encodeUtf8(cp, result);
    }
    return result;
}

// =============================================================================
// Unicode Category Helpers
// =============================================================================

bool isWhitespace(uint32_t cp) {
    switch (cp) {
        case 0x0009: case 0x000A: case 0x000B: case 0x000C: case 0x000D:
        case 0x0020: case 0x00A0: case 0x1680:
        case 0x2000: case 0x2001: case 0x2002: case 0x2003: case 0x2004:
        case 0x2005: case 0x2006: case 0x2007: case 0x2008: case 0x2009:
        case 0x200A: case 0x2028: case 0x2029: case 0x202F: case 0x205F:
        case 0x3000: case 0xFEFF:
            return true;
        default:
            return false;
    }
}

bool isIdentifierStart(uint32_t cp) {
    if (cp == '_' || cp == '$') return true;
    if (cp >= 'a' && cp <= 'z') return true;
    if (cp >= 'A' && cp <= 'Z') return true;
    // Simplified — full Unicode ID_Start requires UCD tables
    if (cp >= 0x00C0 && cp <= 0x024F) return true; // Latin Extended
    return false;
}

bool isIdentifierPart(uint32_t cp) {
    if (isIdentifierStart(cp)) return true;
    if (cp >= '0' && cp <= '9') return true;
    if (cp == 0x200C || cp == 0x200D) return true; // ZWNJ, ZWJ
    return false;
}

} // namespace Zepra::Utils
