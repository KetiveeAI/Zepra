/**
 * @file WasmGCTypes.h
 * @brief WebAssembly GC Proposal Type Definitions
 * 
 * Defines struct, array, and function reference types for the GC proposal.
 * Based on the WasmGC specification.
 */

#pragma once

#include "WasmValType.h"
#include <vector>
#include <cstdint>
#include <optional>

namespace Zepra::Wasm {

// =============================================================================
// Heap Type (Reference Type Targets)
// =============================================================================

enum class HeapType : uint8_t {
    // Abstract types
    Func,       // (ref func) - any function
    Extern,     // (ref extern) - external reference
    Any,        // (ref any) - supertype of all heap types
    Eq,         // (ref eq) - equatable references (subtypes: i31, struct, array)
    I31,        // (ref i31) - 31-bit integer packed into reference
    Struct,     // (ref struct) - any struct
    Array,      // (ref array) - any array
    None,       // (ref none) - bottom type for GC
    NoFunc,     // (ref nofunc) - bottom type for functions
    NoExtern,   // (ref noextern) - bottom type for externrefs
    
    // Concrete types (index into type section)
    Concrete,
};

// =============================================================================
// Reference Type with Nullability
// =============================================================================

struct RefType {
    HeapType heapType;
    uint32_t typeIndex;     // Only valid when heapType == Concrete
    bool nullable;
    
    static RefType func(bool nullable = true) {
        return {HeapType::Func, 0, nullable};
    }
    
    static RefType extern_(bool nullable = true) {
        return {HeapType::Extern, 0, nullable};
    }
    
    static RefType any(bool nullable = true) {
        return {HeapType::Any, 0, nullable};
    }
    
    static RefType eq(bool nullable = true) {
        return {HeapType::Eq, 0, nullable};
    }
    
    static RefType i31(bool nullable = true) {
        return {HeapType::I31, 0, nullable};
    }
    
    static RefType struct_(bool nullable = true) {
        return {HeapType::Struct, 0, nullable};
    }
    
    static RefType array(bool nullable = true) {
        return {HeapType::Array, 0, nullable};
    }
    
    static RefType concrete(uint32_t idx, bool nullable = true) {
        return {HeapType::Concrete, idx, nullable};
    }
    
    bool isAbstract() const { return heapType != HeapType::Concrete; }
    
    // Subtyping check
    bool isSubtypeOf(const RefType& other) const;
};

// =============================================================================
// Field Mutability
// =============================================================================

enum class Mutability : uint8_t {
    Const,  // Immutable field
    Var     // Mutable field
};

// =============================================================================
// Packed Field Types (for structs/arrays)
// =============================================================================

enum class PackedType : uint8_t {
    I8,     // Packed 8-bit integer
    I16     // Packed 16-bit integer
};

// Storage type can be ValType or PackedType
struct StorageType {
    enum class Kind : uint8_t { Val, Packed };
    Kind kind;
    
    union {
        ValType valType;
        PackedType packedType;
    };
    
    static StorageType fromVal(ValType vt) {
        StorageType st;
        st.kind = Kind::Val;
        st.valType = vt;
        return st;
    }
    
    static StorageType i8() {
        StorageType st;
        st.kind = Kind::Packed;
        st.packedType = PackedType::I8;
        return st;
    }
    
    static StorageType i16() {
        StorageType st;
        st.kind = Kind::Packed;
        st.packedType = PackedType::I16;
        return st;
    }
    
    size_t byteSize() const;
    size_t alignment() const;
};

// =============================================================================
// Field Type (for struct fields)
// =============================================================================

struct FieldType {
    StorageType storageType;
    Mutability mutability;
    
    // Layout info (computed after type definition)
    uint32_t offset = 0;
};

// =============================================================================
// Struct Type Definition
// =============================================================================

struct StructType {
    std::vector<FieldType> fields;
    std::optional<uint32_t> superType;  // Index of supertype (if any)
    bool isFinal = true;
    
    // Layout info (computed)
    uint32_t instanceSize = 0;
    uint32_t alignment = 1;
    
    // Compute field offsets and total size
    void computeLayout();
    
    // Get field offset by index
    uint32_t fieldOffset(uint32_t fieldIdx) const {
        return fields[fieldIdx].offset;
    }
    
    // Check if this is subtype of another struct
    bool isSubtypeOf(const StructType& other) const;
};

// =============================================================================
// Array Type Definition
// =============================================================================

struct ArrayType {
    FieldType elementType;
    std::optional<uint32_t> superType;
    bool isFinal = true;
    
    size_t elementSize() const {
        return elementType.storageType.byteSize();
    }
    
    size_t instanceSize(uint32_t length) const;
    
    bool isSubtypeOf(const ArrayType& other) const;
};

// =============================================================================
// Function Type (already exists, but extended for GC)
// =============================================================================

struct FuncType {
    std::vector<ValType> params;
    std::vector<ValType> results;
    std::optional<uint32_t> superType;
    bool isFinal = true;
    
    bool isSubtypeOf(const FuncType& other) const;
};

// =============================================================================
// Composite Type (struct, array, or func)
// =============================================================================

struct CompositeType {
    enum class Kind : uint8_t { Struct, Array, Func };
    Kind kind;
    
    union {
        StructType* structType;
        ArrayType* arrayType;
        FuncType* funcType;
    };
    
    // Recursive type group index (for isorecursive canonicalization)
    uint32_t recGroupIndex = 0;
    uint32_t recGroupOffset = 0;
    
    bool isStruct() const { return kind == Kind::Struct; }
    bool isArray() const { return kind == Kind::Array; }
    bool isFunc() const { return kind == Kind::Func; }
};

// =============================================================================
// Recursive Type Group
// =============================================================================

struct RecGroup {
    std::vector<uint32_t> typeIndices;  // Indices of types in this group
    
    // Canonical signature for hashing (isorecursive canonicalization)
    std::vector<uint8_t> canonicalSignature;
    
    void computeCanonicalSignature(const std::vector<CompositeType>& types);
};

// =============================================================================
// GC Object Header
// =============================================================================

struct GCObjectHeader {
    uint32_t typeIndex;         // Index into type section
    uint32_t gcMark : 2;        // GC mark bits
    uint32_t gcFlags : 6;       // Additional flags
    uint32_t reserved : 24;     // Reserved/hashcode
    
    static constexpr size_t Size = 8;
};

// =============================================================================
// Runtime Representation
// =============================================================================

// i31ref packs a 31-bit signed integer into a tagged pointer
struct I31Ref {
    static constexpr uintptr_t Tag = 1;
    static constexpr int32_t MinValue = -(1 << 30);
    static constexpr int32_t MaxValue = (1 << 30) - 1;
    
    static uintptr_t pack(int32_t value) {
        return (static_cast<uintptr_t>(value) << 1) | Tag;
    }
    
    static int32_t unpack(uintptr_t ref) {
        return static_cast<int32_t>(ref) >> 1;
    }
    
    static bool isI31(uintptr_t ref) {
        return (ref & Tag) != 0;
    }
};

// =============================================================================
// Type Matching and Casting
// =============================================================================

class TypeMatcher {
public:
    explicit TypeMatcher(const std::vector<CompositeType>& types)
        : types_(types) {}
    
    // Check if type A is a subtype of type B
    bool isSubtype(uint32_t typeA, uint32_t typeB) const;
    
    // Check if a runtime value matches expected type
    bool matches(void* value, uint32_t expectedType) const;
    
    // Cast value to target type (returns null on failure)
    void* cast(void* value, uint32_t targetType) const;
    
private:
    const std::vector<CompositeType>& types_;
};

} // namespace Zepra::Wasm
