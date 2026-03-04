/**
 * @file WasmGlobal.h
 * @brief WebAssembly global variable implementation
 * 
 * Provides global variable management for WASM including:
 * - Mutable and immutable globals
 * - All value types (i32, i64, f32, f64, v128, ref types)
 * 
 * Based on Firefox SpiderMonkey / WebKit JSC
 */

#pragma once

#include "WasmConstants.h"
#include "WasmValType.h"
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <array>

namespace Zepra::Wasm {

// =============================================================================
// WasmValue (runtime value representation)
// =============================================================================

union WasmValueUnion {
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
    std::array<uint8_t, 16> v128;
    void* ref;
    
    WasmValueUnion() : i64(0) {}
};

struct WasmValue {
    ValType type;
    WasmValueUnion value;
    
    WasmValue() : type(ValType::i32()) { value.i64 = 0; }
    
    static WasmValue i32(int32_t v) {
        WasmValue val;
        val.type = ValType::i32();
        val.value.i32 = v;
        return val;
    }
    
    static WasmValue i64(int64_t v) {
        WasmValue val;
        val.type = ValType::i64();
        val.value.i64 = v;
        return val;
    }
    
    static WasmValue f32(float v) {
        WasmValue val;
        val.type = ValType::f32();
        val.value.f32 = v;
        return val;
    }
    
    static WasmValue f64(double v) {
        WasmValue val;
        val.type = ValType::f64();
        val.value.f64 = v;
        return val;
    }
    
    static WasmValue v128(const uint8_t data[16]) {
        WasmValue val;
        val.type = ValType::v128();
        std::memcpy(val.value.v128.data(), data, 16);
        return val;
    }
    
    static WasmValue funcRef(void* ref) {
        WasmValue val;
        val.type = ValType::funcRef();
        val.value.ref = ref;
        return val;
    }
    
    static WasmValue externRef(void* ref) {
        WasmValue val;
        val.type = ValType::externRef();
        val.value.ref = ref;
        return val;
    }
    
    static WasmValue nullRef(RefType refType) {
        WasmValue val;
        val.type = ValType::ref(refType.heapType(), refType.isNullable());
        val.value.ref = nullptr;
        return val;
    }
    
    int32_t asI32() const { return value.i32; }
    int64_t asI64() const { return value.i64; }
    float asF32() const { return value.f32; }
    double asF64() const { return value.f64; }
    const uint8_t* asV128() const { return value.v128.data(); }
    void* asRef() const { return value.ref; }
    
    bool isNull() const {
        return type.isReference() && value.ref == nullptr;
    }
};

// =============================================================================
// Global Type Descriptor
// =============================================================================

struct GlobalType {
    ValType type;
    bool mutable_ = false;
    
    GlobalType() = default;
    GlobalType(ValType t, bool m) : type(t), mutable_(m) {}
};

// =============================================================================
// WasmGlobal Class
// =============================================================================

class WasmGlobal {
public:
    WasmGlobal(const GlobalType& type, const WasmValue& initial);
    ~WasmGlobal() = default;
    
    // No copy (use shared_ptr for sharing)
    WasmGlobal(const WasmGlobal&) = delete;
    WasmGlobal& operator=(const WasmGlobal&) = delete;
    
    // Move
    WasmGlobal(WasmGlobal&& other) noexcept;
    WasmGlobal& operator=(WasmGlobal&& other) noexcept;
    
    // ==========================================================================
    // Accessors
    // ==========================================================================
    
    ValType type() const { return type_.type; }
    bool isMutable() const { return type_.mutable_; }
    
    // ==========================================================================
    // Value Access
    // ==========================================================================
    
    WasmValue getValue() const { return value_; }
    
    void setValue(const WasmValue& val) {
        if (!type_.mutable_) {
            throw std::runtime_error("cannot set immutable global");
        }
        if (!val.type.isSubtypeOf(type_.type)) {
            throw std::runtime_error("global type mismatch");
        }
        value_ = val;
    }
    
    // ==========================================================================
    // Direct access for JIT
    // ==========================================================================
    
    // Get pointer to storage (for JIT to read/write directly)
    void* storage() { return &value_.value; }
    const void* storage() const { return &value_.value; }
    
    // Typed accessors for common cases
    int32_t* i32Storage() { return &value_.value.i32; }
    int64_t* i64Storage() { return &value_.value.i64; }
    float* f32Storage() { return &value_.value.f32; }
    double* f64Storage() { return &value_.value.f64; }
    void** refStorage() { return &value_.value.ref; }
    
private:
    GlobalType type_;
    WasmValue value_;
};

// =============================================================================
// Implementation
// =============================================================================

inline WasmGlobal::WasmGlobal(const GlobalType& type, const WasmValue& initial)
    : type_(type)
    , value_(initial) {
}

inline WasmGlobal::WasmGlobal(WasmGlobal&& other) noexcept
    : type_(other.type_)
    , value_(other.value_) {
}

inline WasmGlobal& WasmGlobal::operator=(WasmGlobal&& other) noexcept {
    if (this != &other) {
        type_ = other.type_;
        value_ = other.value_;
    }
    return *this;
}

} // namespace Zepra::Wasm
