/**
 * @file WasmTable.h
 * @brief WebAssembly table implementation
 * 
 * Provides table management for WASM including:
 * - Function tables
 * - Reference tables (externref, anyref)
 * - Table grow operations
 * 
 * Based on Firefox SpiderMonkey / WebKit JSC
 */

#pragma once

#include "WasmConstants.h"
#include "WasmValType.h"
#include <vector>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace Zepra::Wasm {

// Forward declarations
class WasmInstance;
class WasmModule;

// =============================================================================
// Table Limits
// =============================================================================

struct TableLimits {
    uint32_t initial = 0;
    uint32_t maximum = 0;
    bool hasMaximum = false;
};

// =============================================================================
// Table Element (can hold func ref or extern ref)
// =============================================================================

struct TableElement {
    enum class Kind : uint8_t {
        Null,
        FuncRef,
        ExternRef,
        AnyRef
    };
    
    Kind kind = Kind::Null;
    
    union {
        uint32_t funcIndex;     // For FuncRef (index into instance's functions)
        void* externPtr;        // For ExternRef (host object pointer)
        void* anyPtr;           // For AnyRef (GC object pointer)
    };
    
    // Instance that owns the function (for funcref)
    WasmInstance* instance = nullptr;
    
    TableElement() : kind(Kind::Null), funcIndex(0) {}
    
    static TableElement null() {
        return TableElement();
    }
    
    static TableElement funcRef(WasmInstance* inst, uint32_t idx) {
        TableElement e;
        e.kind = Kind::FuncRef;
        e.instance = inst;
        e.funcIndex = idx;
        return e;
    }
    
    static TableElement externRef(void* ref) {
        TableElement e;
        e.kind = Kind::ExternRef;
        e.externPtr = ref;
        return e;
    }
    
    bool isNull() const { return kind == Kind::Null; }
    bool isFuncRef() const { return kind == Kind::FuncRef; }
    bool isExternRef() const { return kind == Kind::ExternRef; }
};

// =============================================================================
// WasmTable Class
// =============================================================================

class WasmTable {
public:
    WasmTable(RefType elementType, const TableLimits& limits);
    ~WasmTable() = default;
    
    // No copy
    WasmTable(const WasmTable&) = delete;
    WasmTable& operator=(const WasmTable&) = delete;
    
    // Move
    WasmTable(WasmTable&& other) noexcept;
    WasmTable& operator=(WasmTable&& other) noexcept;
    
    // ==========================================================================
    // Accessors
    // ==========================================================================
    
    RefType elementType() const { return elementType_; }
    uint32_t length() const { return static_cast<uint32_t>(elements_.size()); }
    uint32_t maximum() const { return limits_.maximum; }
    bool hasMaximum() const { return limits_.hasMaximum; }
    
    // ==========================================================================
    // Element Access
    // ==========================================================================
    
    TableElement getElement(uint32_t index) const {
        if (index >= elements_.size()) {
            throw std::runtime_error("table index out of bounds");
        }
        return elements_[index];
    }
    
    void setElement(uint32_t index, const TableElement& elem) {
        if (index >= elements_.size()) {
            throw std::runtime_error("table index out of bounds");
        }
        elements_[index] = elem;
    }
    
    // ==========================================================================
    // Operations
    // ==========================================================================
    
    // Grow table by delta elements, initialized to null
    // Returns old length on success, -1 on failure
    int32_t grow(uint32_t delta, const TableElement& initValue = TableElement::null());
    
    // Fill a range with a value
    void fill(uint32_t start, uint32_t count, const TableElement& value);
    
    // Copy elements from another table
    void copy(WasmTable& src, uint32_t destStart, uint32_t srcStart, uint32_t count);
    
    // Initialize from element segment
    void init(uint32_t destStart, const std::vector<TableElement>& segment, 
              uint32_t srcStart, uint32_t count);
    
    // ==========================================================================
    // Direct access for JIT
    // ==========================================================================
    
    TableElement* elements() { return elements_.data(); }
    const TableElement* elements() const { return elements_.data(); }
    
private:
    RefType elementType_;
    TableLimits limits_;
    std::vector<TableElement> elements_;
    std::mutex mutex_;
};

// =============================================================================
// Table Implementation
// =============================================================================

inline WasmTable::WasmTable(RefType elementType, const TableLimits& limits)
    : elementType_(elementType)
    , limits_(limits) {
    elements_.resize(limits.initial);
}

inline WasmTable::WasmTable(WasmTable&& other) noexcept
    : elementType_(other.elementType_)
    , limits_(other.limits_)
    , elements_(std::move(other.elements_)) {
}

inline WasmTable& WasmTable::operator=(WasmTable&& other) noexcept {
    if (this != &other) {
        elementType_ = other.elementType_;
        limits_ = other.limits_;
        elements_ = std::move(other.elements_);
    }
    return *this;
}

inline int32_t WasmTable::grow(uint32_t delta, const TableElement& initValue) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    uint32_t oldSize = length();
    uint64_t newSize = static_cast<uint64_t>(oldSize) + delta;
    
    // Check limits
    if (limits_.hasMaximum && newSize > limits_.maximum) {
        return -1;
    }
    
    // Check for overflow (max table size is 2^32 - 1)
    if (newSize > UINT32_MAX) {
        return -1;
    }
    
    try {
        elements_.resize(static_cast<size_t>(newSize), initValue);
    } catch (...) {
        return -1;
    }
    
    return static_cast<int32_t>(oldSize);
}

inline void WasmTable::fill(uint32_t start, uint32_t count, const TableElement& value) {
    if (start + count > length()) {
        throw std::runtime_error("table fill out of bounds");
    }
    for (uint32_t i = 0; i < count; i++) {
        elements_[start + i] = value;
    }
}

inline void WasmTable::copy(WasmTable& src, uint32_t destStart, uint32_t srcStart, uint32_t count) {
    if (destStart + count > length() || srcStart + count > src.length()) {
        throw std::runtime_error("table copy out of bounds");
    }
    
    // Handle overlapping copy
    if (&src == this && destStart > srcStart && destStart < srcStart + count) {
        // Copy backwards
        for (uint32_t i = count; i > 0; i--) {
            elements_[destStart + i - 1] = src.elements_[srcStart + i - 1];
        }
    } else {
        for (uint32_t i = 0; i < count; i++) {
            elements_[destStart + i] = src.elements_[srcStart + i];
        }
    }
}

inline void WasmTable::init(uint32_t destStart, const std::vector<TableElement>& segment,
                            uint32_t srcStart, uint32_t count) {
    if (destStart + count > length() || srcStart + count > segment.size()) {
        throw std::runtime_error("table init out of bounds");
    }
    for (uint32_t i = 0; i < count; i++) {
        elements_[destStart + i] = segment[srcStart + i];
    }
}

} // namespace Zepra::Wasm
