/**
 * @file type_profiler.hpp
 * @brief Type profiling for JIT speculative optimization
 * 
 * Tracks operand types at specific bytecode locations to enable
 * speculative type-specialized code generation.
 * 
 * Production-ready implementation following V8/JSC patterns.
 */

#pragma once

#include "../config.hpp"
#include "runtime/objects/value.hpp"
#include <cstdint>
#include <array>
#include <atomic>
#include <unordered_map>

namespace Zepra::JIT {

/**
 * @brief Observed value types (bit flags for polymorphic tracking)
 */
enum class ObservedType : uint16_t {
    None        = 0,
    Undefined   = 1 << 0,
    Null        = 1 << 1,
    Boolean     = 1 << 2,
    Int32       = 1 << 3,   // Integer that fits in 32 bits
    Double      = 1 << 4,   // Any non-int32 number
    String      = 1 << 5,
    Object      = 1 << 6,
    Array       = 1 << 7,
    Function    = 1 << 8,
    Symbol      = 1 << 9,
    BigInt      = 1 << 10,
    
    // Compound types
    Number      = Int32 | Double,
    Primitive   = Undefined | Null | Boolean | Int32 | Double | String | Symbol | BigInt,
    AnyObject   = Object | Array | Function,
    Any         = 0xFFFF
};

inline ObservedType operator|(ObservedType a, ObservedType b) {
    return static_cast<ObservedType>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

inline ObservedType operator&(ObservedType a, ObservedType b) {
    return static_cast<ObservedType>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}

inline ObservedType& operator|=(ObservedType& a, ObservedType b) {
    a = a | b;
    return a;
}

inline ObservedType operator~(ObservedType a) {
    return static_cast<ObservedType>(~static_cast<uint16_t>(a));
}

/**
 * @brief Type profile for a single bytecode site
 */
struct TypeProfile {
    ObservedType observedTypes = ObservedType::None;
    uint32_t sampleCount = 0;
    
    // For binary operations - track both operands
    ObservedType leftType = ObservedType::None;
    ObservedType rightType = ObservedType::None;
    ObservedType resultType = ObservedType::None;
    
    // Stability tracking
    bool isMonomorphic() const {
        uint16_t t = static_cast<uint16_t>(observedTypes);
        return t != 0 && (t & (t - 1)) == 0;  // Power of 2 = single type
    }
    
    bool isPolymorphic() const {
        return !isMonomorphic() && observedTypes != ObservedType::None;
    }
    
    bool isMegamorphic() const {
        // More than 4 types observed
        int count = 0;
        uint16_t t = static_cast<uint16_t>(observedTypes);
        while (t) { count += t & 1; t >>= 1; }
        return count > 4;
    }
    
    // Check if only numbers observed
    bool isNumberOnly() const {
        return (observedTypes & ~ObservedType::Number) == ObservedType::None 
            && observedTypes != ObservedType::None;
    }
    
    // Check if only int32 observed
    bool isInt32Only() const {
        return observedTypes == ObservedType::Int32;
    }
    
    // Check if only strings observed  
    bool isStringOnly() const {
        return observedTypes == ObservedType::String;
    }
};

/**
 * @brief Operation-specific feedback for optimization
 */
struct BinaryOpFeedback {
    TypeProfile profile;
    
    // Overflow tracking for integer operations
    uint32_t overflowCount = 0;
    bool hasOverflowed() const { return overflowCount > 0; }
    
    // Specialization hints
    enum class Hint : uint8_t {
        None,
        Int32Add,
        Int32Sub, 
        Int32Mul,
        DoubleArithmetic,
        StringConcat,
        Generic
    };
    Hint optimizationHint = Hint::None;
    
    void updateHint() {
        if (profile.isInt32Only() && !hasOverflowed()) {
            if (profile.resultType == ObservedType::Int32) {
                optimizationHint = Hint::Int32Add; // Generalize
            }
        } else if (profile.isNumberOnly()) {
            optimizationHint = Hint::DoubleArithmetic;
        } else if (profile.isStringOnly()) {
            optimizationHint = Hint::StringConcat;
        } else {
            optimizationHint = Hint::Generic;
        }
    }
};

/**
 * @brief Type feedback vector - stores profiles for all IC sites in a function
 */
class TypeFeedbackVector {
public:
    static constexpr size_t MAX_SITES = 64;
    
    TypeProfile* getProfile(size_t siteIndex) {
        if (siteIndex >= MAX_SITES) return nullptr;
        return &sites_[siteIndex];
    }
    
    BinaryOpFeedback* getBinaryOpFeedback(size_t siteIndex) {
        if (siteIndex >= MAX_SITES) return nullptr;
        return &binaryOps_[siteIndex];
    }
    
    void recordType(size_t siteIndex, const Runtime::Value& value);
    void recordBinaryOp(size_t siteIndex, 
                        const Runtime::Value& left, 
                        const Runtime::Value& right,
                        const Runtime::Value& result);
    void recordOverflow(size_t siteIndex);
    
    void clear() {
        for (auto& site : sites_) {
            site = TypeProfile{};
        }
        for (auto& op : binaryOps_) {
            op = BinaryOpFeedback{};
        }
    }
    
private:
    std::array<TypeProfile, MAX_SITES> sites_{};
    std::array<BinaryOpFeedback, MAX_SITES> binaryOps_{};
};

/**
 * @brief Global type profiler manager
 */
class TypeProfiler {
public:
    static TypeProfiler& instance() {
        static TypeProfiler profiler;
        return profiler;
    }
    
    // Get feedback vector for a function
    TypeFeedbackVector* getVector(uintptr_t functionId);
    
    // Quick inline type recording (called from interpreter hot path)
    void recordType(uintptr_t functionId, size_t site, const Runtime::Value& value);
    void recordBinaryOp(uintptr_t functionId, size_t site,
                        const Runtime::Value& left,
                        const Runtime::Value& right,
                        const Runtime::Value& result);
    
    // Analysis for JIT compilation
    bool shouldSpecializeInt32(uintptr_t functionId, size_t site) const;
    bool shouldSpecializeDouble(uintptr_t functionId, size_t site) const;
    bool shouldSpecializeString(uintptr_t functionId, size_t site) const;
    
    // Statistics
    size_t totalProfiles() const { return vectors_.size() * TypeFeedbackVector::MAX_SITES; }
    
    void setEnabled(bool e) { enabled_ = e; }
    bool isEnabled() const { return enabled_; }
    
    void clear() { vectors_.clear(); }
    
private:
    TypeProfiler() = default;
    
    std::unordered_map<uintptr_t, TypeFeedbackVector> vectors_;
    bool enabled_ = true;
};

// Helper: Convert Value to ObservedType
inline ObservedType observeType(const Runtime::Value& value) {
    if (value.isUndefined()) return ObservedType::Undefined;
    if (value.isNull()) return ObservedType::Null;
    if (value.isBoolean()) return ObservedType::Boolean;
    if (value.isNumber()) {
        double n = value.asNumber();
        // Check if it's an integer that fits in int32
        if (n >= INT32_MIN && n <= INT32_MAX && n == static_cast<int32_t>(n)) {
            return ObservedType::Int32;
        }
        return ObservedType::Double;
    }
    if (value.isString()) return ObservedType::String;
    if (value.isObject()) {
        // Could further distinguish Array, Function
        return ObservedType::Object;
    }
    return ObservedType::None;
}

} // namespace Zepra::JIT
