/**
 * @file WasmCanonicalABI.h
 * @brief Canonical ABI for Component Model
 * 
 * Implements:
 * - Canonical lifting/lowering for primitive types
 * - String encoding (UTF-8/UTF-16/Latin1)
 * - Complex type handling (records, variants, lists)
 * - Resource handle lifecycle
 */

#pragma once

#include "wasm.hpp"
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <cstdint>
#include <cstring>

namespace Zepra::Wasm {

// =============================================================================
// String Encodings
// =============================================================================

enum class StringEncoding : uint8_t {
    UTF8 = 0,
    UTF16 = 1,
    Latin1 = 2
};

/**
 * @brief String conversion utilities
 */
namespace StringConvert {

// UTF-8 to UTF-16
inline std::u16string utf8ToUtf16(const std::string& utf8) {
    std::u16string result;
    for (size_t i = 0; i < utf8.size();) {
        uint32_t cp;
        uint8_t c = utf8[i];
        if ((c & 0x80) == 0) {
            cp = c;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            cp = (c & 0x1F) << 6;
            cp |= (utf8[i + 1] & 0x3F);
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            cp = (c & 0x0F) << 12;
            cp |= (utf8[i + 1] & 0x3F) << 6;
            cp |= (utf8[i + 2] & 0x3F);
            i += 3;
        } else {
            cp = (c & 0x07) << 18;
            cp |= (utf8[i + 1] & 0x3F) << 12;
            cp |= (utf8[i + 2] & 0x3F) << 6;
            cp |= (utf8[i + 3] & 0x3F);
            i += 4;
        }
        
        if (cp <= 0xFFFF) {
            result.push_back(static_cast<char16_t>(cp));
        } else {
            // Surrogate pair
            cp -= 0x10000;
            result.push_back(static_cast<char16_t>(0xD800 | (cp >> 10)));
            result.push_back(static_cast<char16_t>(0xDC00 | (cp & 0x3FF)));
        }
    }
    return result;
}

// UTF-16 to UTF-8
inline std::string utf16ToUtf8(const std::u16string& utf16) {
    std::string result;
    for (size_t i = 0; i < utf16.size();) {
        uint32_t cp = utf16[i];
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < utf16.size()) {
            // Surrogate pair
            uint32_t low = utf16[i + 1];
            if (low >= 0xDC00 && low <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                i += 2;
            } else {
                i += 1;
            }
        } else {
            i += 1;
        }
        
        if (cp < 0x80) {
            result.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            result.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            result.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            result.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return result;
}

// Latin1 to UTF-8
inline std::string latin1ToUtf8(const std::string& latin1) {
    std::string result;
    for (char c : latin1) {
        uint8_t byte = static_cast<uint8_t>(c);
        if (byte < 0x80) {
            result.push_back(c);
        } else {
            result.push_back(static_cast<char>(0xC0 | (byte >> 6)));
            result.push_back(static_cast<char>(0x80 | (byte & 0x3F)));
        }
    }
    return result;
}

} // namespace StringConvert

// =============================================================================
// Canonical ABI Types
// =============================================================================

/**
 * @brief Flat representation of a value (used in ABI)
 */
struct FlatValue {
    std::vector<uint32_t> i32s;
    std::vector<uint64_t> i64s;
    std::vector<float> f32s;
    std::vector<double> f64s;
};

/**
 * @brief Canonical type for ABI
 */
enum class CanonicalType {
    Bool,
    U8, U16, U32, U64,
    S8, S16, S32, S64,
    F32, F64,
    Char,
    String,
    List,
    Record,
    Tuple,
    Variant,
    Enum,
    Option,
    Result,
    Flags,
    Own,     // Owned resource handle
    Borrow   // Borrowed resource handle
};

// =============================================================================
// Lifting and Lowering
// =============================================================================

/**
 * @brief Canonical ABI context
 */
class CanonicalABI {
public:
    explicit CanonicalABI(void* memory, size_t memorySize, 
                          StringEncoding encoding = StringEncoding::UTF8)
        : memory_(static_cast<uint8_t*>(memory))
        , memorySize_(memorySize)
        , encoding_(encoding) {}
    
    // -------------------------------------------------------------------------
    // Primitive Lowering (host value -> core wasm)
    // -------------------------------------------------------------------------
    
    uint32_t lowerBool(bool value) { return value ? 1 : 0; }
    uint32_t lowerU8(uint8_t value) { return value; }
    uint32_t lowerU16(uint16_t value) { return value; }
    uint32_t lowerU32(uint32_t value) { return value; }
    uint64_t lowerU64(uint64_t value) { return value; }
    int32_t lowerS8(int8_t value) { return value; }
    int32_t lowerS16(int16_t value) { return value; }
    int32_t lowerS32(int32_t value) { return value; }
    int64_t lowerS64(int64_t value) { return value; }
    float lowerF32(float value) { return value; }
    double lowerF64(double value) { return value; }
    uint32_t lowerChar(char32_t value) { return static_cast<uint32_t>(value); }
    
    // Lower string to (ptr, len) pair
    std::pair<uint32_t, uint32_t> lowerString(const std::string& str, uint32_t ptr) {
        size_t len = str.size();
        if (ptr + len > memorySize_) {
            return {0, 0};  // Out of bounds
        }
        std::memcpy(memory_ + ptr, str.c_str(), len);
        return {ptr, static_cast<uint32_t>(len)};
    }
    
    // -------------------------------------------------------------------------
    // Primitive Lifting (core wasm -> host value)
    // -------------------------------------------------------------------------
    
    bool liftBool(uint32_t value) { return value != 0; }
    uint8_t liftU8(uint32_t value) { return static_cast<uint8_t>(value); }
    uint16_t liftU16(uint32_t value) { return static_cast<uint16_t>(value); }
    uint32_t liftU32(uint32_t value) { return value; }
    uint64_t liftU64(uint64_t value) { return value; }
    int8_t liftS8(int32_t value) { return static_cast<int8_t>(value); }
    int16_t liftS16(int32_t value) { return static_cast<int16_t>(value); }
    int32_t liftS32(int32_t value) { return value; }
    int64_t liftS64(int64_t value) { return value; }
    float liftF32(float value) { return value; }
    double liftF64(double value) { return value; }
    char32_t liftChar(uint32_t value) { return static_cast<char32_t>(value); }
    
    // Lift string from (ptr, len) pair
    std::string liftString(uint32_t ptr, uint32_t len) {
        if (ptr + len > memorySize_) {
            return "";  // Out of bounds
        }
        return std::string(reinterpret_cast<char*>(memory_ + ptr), len);
    }
    
    // -------------------------------------------------------------------------
    // List Operations
    // -------------------------------------------------------------------------
    
    // Lower a list of i32 values
    std::pair<uint32_t, uint32_t> lowerListI32(const std::vector<int32_t>& list, uint32_t ptr) {
        size_t size = list.size() * sizeof(int32_t);
        if (ptr + size > memorySize_) {
            return {0, 0};
        }
        std::memcpy(memory_ + ptr, list.data(), size);
        return {ptr, static_cast<uint32_t>(list.size())};
    }
    
    // Lift a list of i32 values
    std::vector<int32_t> liftListI32(uint32_t ptr, uint32_t count) {
        std::vector<int32_t> result(count);
        if (ptr + count * sizeof(int32_t) > memorySize_) {
            return {};
        }
        std::memcpy(result.data(), memory_ + ptr, count * sizeof(int32_t));
        return result;
    }
    
    // -------------------------------------------------------------------------
    // Option Type
    // -------------------------------------------------------------------------
    
    template<typename T>
    std::pair<uint32_t, T> lowerOption(const std::optional<T>& opt, T defaultVal) {
        if (opt.has_value()) {
            return {1, *opt};
        }
        return {0, defaultVal};
    }
    
    template<typename T>
    std::optional<T> liftOption(uint32_t discriminant, T value) {
        if (discriminant == 0) {
            return std::nullopt;
        }
        return value;
    }
    
    // -------------------------------------------------------------------------
    // Result Type
    // -------------------------------------------------------------------------
    
    template<typename T, typename E>
    std::tuple<uint32_t, T, E> lowerResult(const std::variant<T, E>& result, T okDefault, E errDefault) {
        if (std::holds_alternative<T>(result)) {
            return {0, std::get<T>(result), errDefault};
        }
        return {1, okDefault, std::get<E>(result)};
    }
    
    template<typename T, typename E>
    std::variant<T, E> liftResult(uint32_t discriminant, T okVal, E errVal) {
        if (discriminant == 0) {
            return okVal;
        }
        return errVal;
    }
    
    // -------------------------------------------------------------------------
    // Memory Management
    // -------------------------------------------------------------------------
    
    // Allocate memory in linear memory (requires realloc export)
    uint32_t allocate(uint32_t size, uint32_t align) {
        // Would call realloc export
        (void)align;
        uint32_t ptr = nextAlloc_;
        nextAlloc_ += size;
        return ptr;
    }
    
    void setAllocator(uint32_t nextAlloc) { nextAlloc_ = nextAlloc; }
    
private:
    uint8_t* memory_;
    size_t memorySize_;
    StringEncoding encoding_;
    uint32_t nextAlloc_ = 0;
};

// =============================================================================
// Resource Handles
// =============================================================================

/**
 * @brief Resource handle for component model
 */
template<typename T>
class ResourceHandle {
public:
    explicit ResourceHandle(uint32_t rep) : rep_(rep) {}
    
    uint32_t rep() const { return rep_; }
    
    // Own semantics - transfers ownership
    static ResourceHandle own(uint32_t rep) {
        return ResourceHandle(rep);
    }
    
    // Borrow semantics - temporary access
    static ResourceHandle borrow(uint32_t rep) {
        return ResourceHandle(rep);
    }
    
private:
    uint32_t rep_;  // Resource representation
};

/**
 * @brief Resource table for managing owned resources
 */
class ResourceTable {
public:
    // Create a new resource, returns handle rep
    template<typename T>
    uint32_t create(T* resource) {
        uint32_t rep = nextRep_++;
        resources_[rep] = resource;
        return rep;
    }
    
    // Get resource by handle rep
    template<typename T>
    T* get(uint32_t rep) {
        auto it = resources_.find(rep);
        return it != resources_.end() ? static_cast<T*>(it->second) : nullptr;
    }
    
    // Drop a resource
    void drop(uint32_t rep) {
        resources_.erase(rep);
    }
    
private:
    std::unordered_map<uint32_t, void*> resources_;
    uint32_t nextRep_ = 1;
};

} // namespace Zepra::Wasm
