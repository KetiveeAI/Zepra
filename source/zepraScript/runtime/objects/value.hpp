#pragma once

/**
 * @file value.hpp
 * @brief JavaScript value representation using NaN-boxing
 * 
 * NaN-boxing encodes JavaScript values in 64 bits:
 * - IEEE 754 doubles use the full 64 bits
 * - NaN values have a specific bit pattern, leaving room for type tags
 * - Pointers and other values are encoded in the payload
 */

#include "config.hpp"
#include <cstdint>
#include <string>
#include <cmath>
#include <type_traits>

namespace Zepra::Runtime {

// Forward declarations
class Object;
class String;
class Function;
class Array;
class BigIntObject;

/**
 * @brief JavaScript value type tags
 */
enum class ValueType : uint8 {
    Undefined,
    Null,
    Boolean,
    Number,
    String,
    Object,
    Symbol,
    BigInt
};

/**
 * @brief A JavaScript value using NaN-boxing
 * 
 * Memory layout (64 bits):
 * - Number: Raw IEEE 754 double
 * - Other types: NaN with type tag and payload
 * 
 * NaN bit pattern: 0x7FF8000000000000 | (tag << 48) | payload
 */
class Value {
public:
    // NaN-boxing constants
    static constexpr uint64_t QNAN_MASK = 0x7FF8000000000000ULL;
    static constexpr uint64_t TAG_MASK  = 0x0007000000000000ULL;
    static constexpr uint64_t PAYLOAD_MASK = 0x0000FFFFFFFFFFFFULL;
    
    // NOTE: TAG_UNDEFINED must be non-zero to avoid collision with IEEE NaN
    // IEEE NaN = 0x7FF8000000000000, so we use TAG 0x7 for undefined
    static constexpr uint64_t TAG_UNDEFINED = 0x0007000000000000ULL;
    static constexpr uint64_t TAG_NULL      = 0x0001000000000000ULL;
    static constexpr uint64_t TAG_FALSE     = 0x0002000000000000ULL;
    static constexpr uint64_t TAG_TRUE      = 0x0003000000000000ULL;
    static constexpr uint64_t TAG_OBJECT    = 0x0004000000000000ULL;
    static constexpr uint64_t TAG_STRING    = 0x0005000000000000ULL;
    static constexpr uint64_t TAG_SYMBOL    = 0x0006000000000000ULL;
    
    // Constructors
    Value() : bits_(QNAN_MASK | TAG_UNDEFINED) {}
    
    static Value undefined() { return Value(); }
    static Value null() { return Value(QNAN_MASK | TAG_NULL); }
    static Value boolean(bool b) { 
        return Value(QNAN_MASK | (b ? TAG_TRUE : TAG_FALSE)); 
    }
    static Value number(double n);
    static Value object(Object* obj);
    static Value string(String* str);
    static Value symbol(uint32_t id);  // Symbol using unique ID
    static Value bigint(BigIntObject* bi);  // BigInt (stored as Object)
    
    // Type checks
    bool isUndefined() const { return bits_ == (QNAN_MASK | TAG_UNDEFINED); }
    bool isNull() const { return bits_ == (QNAN_MASK | TAG_NULL); }
    bool isBoolean() const { 
        return bits_ == (QNAN_MASK | TAG_FALSE) || bits_ == (QNAN_MASK | TAG_TRUE);
    }
    bool isNumber() const;
    bool isObject() const { return (bits_ & (QNAN_MASK | TAG_MASK)) == (QNAN_MASK | TAG_OBJECT); }
    bool isString() const { return (bits_ & (QNAN_MASK | TAG_MASK)) == (QNAN_MASK | TAG_STRING); }
    bool isSymbol() const { return (bits_ & (QNAN_MASK | TAG_MASK)) == (QNAN_MASK | TAG_SYMBOL); }
    bool isBigInt() const;  // Checks isObject() && objectType == BigInt
    
    bool isNullOrUndefined() const { return isNull() || isUndefined(); }
    bool isFalsy() const;
    bool isTruthy() const { return !isFalsy(); }
    
    // Type conversions
    ValueType type() const;
    
    bool asBoolean() const { return bits_ == (QNAN_MASK | TAG_TRUE); }
    double asNumber() const;
    Object* asObject() const;
    String* asString() const;
    uint32_t asSymbol() const;  // Returns symbol ID
    Function* asFunction() const;
    Array* asArray() const;
    BigIntObject* asBigInt() const;
    
    // JavaScript type coercion
    bool toBoolean() const;
    double toNumber() const;
    std::string toString() const;
    Object* toObject() const;
    
    // Comparison
    bool equals(const Value& other) const;        // ==
    bool strictEquals(const Value& other) const;  // ===
    
    // Operators
    static Value add(const Value& left, const Value& right);
    static Value subtract(const Value& left, const Value& right);
    static Value multiply(const Value& left, const Value& right);
    static Value divide(const Value& left, const Value& right);
    static Value modulo(const Value& left, const Value& right);
    static Value power(const Value& left, const Value& right);
    
    static Value bitwiseAnd(const Value& left, const Value& right);
    static Value bitwiseOr(const Value& left, const Value& right);
    static Value bitwiseXor(const Value& left, const Value& right);
    static Value bitwiseNot(const Value& val);
    static Value leftShift(const Value& left, const Value& right);
    static Value rightShift(const Value& left, const Value& right);
    static Value unsignedRightShift(const Value& left, const Value& right);
    
    static Value negate(const Value& val);
    static Value logicalNot(const Value& val);
    
    // Comparison operators
    static Value lessThan(const Value& left, const Value& right);
    static Value lessEqual(const Value& left, const Value& right);
    static Value greaterThan(const Value& left, const Value& right);
    static Value greaterEqual(const Value& left, const Value& right);
    
    // Raw access
    uint64_t rawBits() const { return bits_; }
    
private:
    explicit Value(uint64_t bits) : bits_(bits) {}
    
    union {
        uint64_t bits_;
        double number_;
    };
};

// Compile-time checks
static_assert(sizeof(Value) == 8, "Value must be 64 bits");

/**
 * @brief Check if a double is a NaN value
 */
inline bool isNaN(double d) {
    return std::isnan(d);
}

inline Value Value::number(double n) {
    Value v;
    v.number_ = n;
    return v;
}

inline bool Value::isNumber() const {
    // Check for all non-number tagged types first using exact bit patterns
    // This avoids the circular dependency issue
    
    // Check undefined: exact match to QNAN_MASK | TAG_UNDEFINED
    if (bits_ == (QNAN_MASK | TAG_UNDEFINED)) return false;
    
    // Check null: exact match
    if (bits_ == (QNAN_MASK | TAG_NULL)) return false;
    
    // Check booleans: exact matches
    if (bits_ == (QNAN_MASK | TAG_FALSE)) return false;
    if (bits_ == (QNAN_MASK | TAG_TRUE)) return false;
    
    // Check object/string/symbol: check tag pattern (these have payload bits too)
    uint64_t tag = bits_ & (QNAN_MASK | TAG_MASK);
    if (tag == (QNAN_MASK | TAG_OBJECT)) return false;
    if (tag == (QNAN_MASK | TAG_STRING)) return false;
    if (tag == (QNAN_MASK | TAG_SYMBOL)) return false;
    
    // Everything else is a number (including regular doubles and IEEE NaN)
    return true;
}

inline double Value::asNumber() const {
    return number_;
}

inline Value Value::object(Object* obj) {
    Value v;
    v.bits_ = QNAN_MASK | TAG_OBJECT | (reinterpret_cast<uint64_t>(obj) & PAYLOAD_MASK);
    return v;
}

inline Object* Value::asObject() const {
    return reinterpret_cast<Object*>(bits_ & PAYLOAD_MASK);
}

inline Value Value::string(String* str) {
    Value v;
    v.bits_ = QNAN_MASK | TAG_STRING | (reinterpret_cast<uint64_t>(str) & PAYLOAD_MASK);
    return v;
}

inline String* Value::asString() const {
    return reinterpret_cast<String*>(bits_ & PAYLOAD_MASK);
}

inline Value Value::symbol(uint32_t id) {
    Value v;
    v.bits_ = QNAN_MASK | TAG_SYMBOL | (static_cast<uint64_t>(id) & PAYLOAD_MASK);
    return v;
}

inline uint32_t Value::asSymbol() const {
    return static_cast<uint32_t>(bits_ & PAYLOAD_MASK);
}

} // namespace Zepra::Runtime
