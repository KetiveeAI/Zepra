/**
 * @file WasmStringBuiltins.h
 * @brief WebAssembly String Builtins Proposal
 * 
 * Implements WASM stringref type and string operations:
 * - stringref reference type
 * - string.new_utf8, string.new_wtf16
 * - string.encode_utf8, string.encode_wtf16
 * - string.concat, string.eq
 * - string.iter, string.hash
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <string_view>

namespace Zepra::Wasm {

// =============================================================================
// String Reference Type
// =============================================================================

/**
 * @brief Stringref - interned immutable string
 * 
 * Strings in WASM are immutable UTF-8 or WTF-16 encoded.
 */
class WasmString {
public:
    enum class Encoding { UTF8, WTF8, WTF16 };
    
    WasmString() : encoding_(Encoding::UTF8) {}
    
    WasmString(std::string utf8, Encoding enc = Encoding::UTF8)
        : utf8Data_(std::move(utf8)), encoding_(enc) {}
    
    static WasmString fromUTF8(const uint8_t* data, size_t len) {
        return WasmString(std::string(reinterpret_cast<const char*>(data), len), Encoding::UTF8);
    }
    
    static WasmString fromWTF16(const uint16_t* data, size_t len) {
        // Convert WTF-16 to UTF-8 for storage
        std::string utf8;
        utf8.reserve(len * 3);  // Worst case
        for (size_t i = 0; i < len; ++i) {
            uint32_t cp = data[i];
            // Handle surrogate pairs
            if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < len) {
                uint32_t low = data[i + 1];
                if (low >= 0xDC00 && low <= 0xDFFF) {
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                    ++i;
                }
            }
            // Encode as UTF-8
            if (cp < 0x80) {
                utf8.push_back(static_cast<char>(cp));
            } else if (cp < 0x800) {
                utf8.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                utf8.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else if (cp < 0x10000) {
                utf8.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                utf8.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                utf8.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else {
                utf8.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                utf8.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                utf8.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                utf8.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
        }
        return WasmString(std::move(utf8), Encoding::WTF16);
    }
    
    const std::string& utf8() const { return utf8Data_; }
    size_t length() const { return utf8Data_.size(); }
    Encoding encoding() const { return encoding_; }
    
    bool equals(const WasmString& other) const {
        return utf8Data_ == other.utf8Data_;
    }
    
    int compare(const WasmString& other) const {
        return utf8Data_.compare(other.utf8Data_);
    }
    
    uint32_t hash() const {
        uint32_t h = 0;
        for (char c : utf8Data_) {
            h = h * 31 + static_cast<uint8_t>(c);
        }
        return h;
    }
    
    WasmString concat(const WasmString& other) const {
        return WasmString(utf8Data_ + other.utf8Data_, encoding_);
    }
    
    WasmString substring(size_t start, size_t length) const {
        return WasmString(utf8Data_.substr(start, length), encoding_);
    }
    
    // Encode to memory
    size_t encodeUTF8(uint8_t* memory, size_t maxLen) const {
        size_t len = std::min(utf8Data_.size(), maxLen);
        std::memcpy(memory, utf8Data_.data(), len);
        return len;
    }
    
    std::vector<uint16_t> encodeWTF16() const {
        std::vector<uint16_t> result;
        result.reserve(utf8Data_.size());
        
        for (size_t i = 0; i < utf8Data_.size(); ) {
            uint32_t cp;
            uint8_t c = utf8Data_[i];
            if ((c & 0x80) == 0) {
                cp = c;
                i += 1;
            } else if ((c & 0xE0) == 0xC0) {
                cp = (c & 0x1F) << 6;
                cp |= (utf8Data_[i+1] & 0x3F);
                i += 2;
            } else if ((c & 0xF0) == 0xE0) {
                cp = (c & 0x0F) << 12;
                cp |= (utf8Data_[i+1] & 0x3F) << 6;
                cp |= (utf8Data_[i+2] & 0x3F);
                i += 3;
            } else {
                cp = (c & 0x07) << 18;
                cp |= (utf8Data_[i+1] & 0x3F) << 12;
                cp |= (utf8Data_[i+2] & 0x3F) << 6;
                cp |= (utf8Data_[i+3] & 0x3F);
                i += 4;
            }
            
            // Encode to WTF-16
            if (cp < 0x10000) {
                result.push_back(static_cast<uint16_t>(cp));
            } else {
                cp -= 0x10000;
                result.push_back(static_cast<uint16_t>(0xD800 + (cp >> 10)));
                result.push_back(static_cast<uint16_t>(0xDC00 + (cp & 0x3FF)));
            }
        }
        return result;
    }
    
private:
    std::string utf8Data_;
    Encoding encoding_;
};

// =============================================================================
// String Iterator
// =============================================================================

/**
 * @brief Iterator over string codepoints
 */
class WasmStringIterator {
public:
    WasmStringIterator(const WasmString* str) : str_(str), pos_(0) {}
    
    bool hasNext() const { return pos_ < str_->utf8().size(); }
    
    uint32_t next() {
        if (!hasNext()) return 0;
        
        const std::string& data = str_->utf8();
        uint8_t c = data[pos_];
        uint32_t cp;
        
        if ((c & 0x80) == 0) {
            cp = c;
            pos_ += 1;
        } else if ((c & 0xE0) == 0xC0) {
            cp = (c & 0x1F) << 6;
            cp |= (data[pos_+1] & 0x3F);
            pos_ += 2;
        } else if ((c & 0xF0) == 0xE0) {
            cp = (c & 0x0F) << 12;
            cp |= (data[pos_+1] & 0x3F) << 6;
            cp |= (data[pos_+2] & 0x3F);
            pos_ += 3;
        } else {
            cp = (c & 0x07) << 18;
            cp |= (data[pos_+1] & 0x3F) << 12;
            cp |= (data[pos_+2] & 0x3F) << 6;
            cp |= (data[pos_+3] & 0x3F);
            pos_ += 4;
        }
        return cp;
    }
    
    void reset() { pos_ = 0; }
    
private:
    const WasmString* str_;
    size_t pos_;
};

// =============================================================================
// String Builtins Namespace
// =============================================================================

namespace StringBuiltins {
    // Opcodes (0xFB prefix, string section)
    constexpr uint16_t op_string_new_utf8 = 0x80;
    constexpr uint16_t op_string_new_wtf8 = 0x81;
    constexpr uint16_t op_string_new_wtf16 = 0x82;
    constexpr uint16_t op_string_const = 0x83;
    
    constexpr uint16_t op_string_measure_utf8 = 0x84;
    constexpr uint16_t op_string_measure_wtf8 = 0x85;
    constexpr uint16_t op_string_measure_wtf16 = 0x86;
    
    constexpr uint16_t op_string_encode_utf8 = 0x87;
    constexpr uint16_t op_string_encode_wtf8 = 0x88;
    constexpr uint16_t op_string_encode_wtf16 = 0x89;
    
    constexpr uint16_t op_string_concat = 0x8A;
    constexpr uint16_t op_string_eq = 0x8B;
    constexpr uint16_t op_string_is_usv = 0x8C;
    constexpr uint16_t op_string_as_wtf8 = 0x8D;
    constexpr uint16_t op_string_as_wtf16 = 0x8E;
    constexpr uint16_t op_string_as_iter = 0x8F;
    
    constexpr uint16_t op_stringview_wtf8_advance = 0x90;
    constexpr uint16_t op_stringview_wtf8_slice = 0x91;
    constexpr uint16_t op_stringview_wtf16_length = 0x92;
    constexpr uint16_t op_stringview_wtf16_get_code = 0x93;
    constexpr uint16_t op_stringview_iter_next = 0x94;
    constexpr uint16_t op_stringview_iter_advance = 0x95;
    constexpr uint16_t op_stringview_iter_rewind = 0x96;
    constexpr uint16_t op_stringview_iter_slice = 0x97;
    
    constexpr uint16_t op_string_compare = 0x98;
    constexpr uint16_t op_string_from_code_point = 0x99;
    constexpr uint16_t op_string_hash = 0x9A;
}

// =============================================================================
// String Table
// =============================================================================

/**
 * @brief Manages interned strings
 */
class StringTable {
public:
    uint32_t intern(const WasmString& str) {
        uint32_t h = str.hash();
        auto it = hashToIndex_.find(h);
        if (it != hashToIndex_.end()) {
            // Check for collision
            if (strings_[it->second].equals(str)) {
                return it->second;
            }
        }
        
        uint32_t idx = static_cast<uint32_t>(strings_.size());
        strings_.push_back(str);
        hashToIndex_[h] = idx;
        return idx;
    }
    
    const WasmString* get(uint32_t index) const {
        return index < strings_.size() ? &strings_[index] : nullptr;
    }
    
private:
    std::vector<WasmString> strings_;
    std::unordered_map<uint32_t, uint32_t> hashToIndex_;
};

} // namespace Zepra::Wasm
