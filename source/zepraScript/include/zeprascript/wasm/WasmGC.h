/**
 * @file WasmGC.h
 * @brief WebAssembly GC Proposal Implementation
 * 
 * Implements the WebAssembly GC proposal:
 * - Struct and Array types
 * - Reference type hierarchy
 * - GC integration with ZepraScript
 */

#pragma once

#include "wasm.hpp"
#include <memory>
#include <vector>
#include <cstring>

namespace Zepra::Wasm {

// =============================================================================
// GC Type System
// =============================================================================

/**
 * @brief Field mutability
 */
enum class Mutability : uint8_t {
    Const = 0,
    Var = 1
};

/**
 * @brief Field storage type (packed or normal)
 */
enum class PackedType : uint8_t {
    NotPacked = 0,
    I8 = 1,
    I16 = 2
};

/**
 * @brief GC Reference Type Hierarchy
 * 
 * anyref (top)
 *   ├── eqref
 *   │     ├── i31ref
 *   │     ├── structref
 *   │     │     └── (struct types)
 *   │     └── arrayref
 *   │           └── (array types)
 *   └── funcref
 *         └── (func types)
 */
enum class RefTypeKind : uint8_t {
    Any = 0,      // anyref - top type
    Eq = 1,       // eqref - equatable references
    I31 = 2,      // i31ref - 31-bit integer reference
    Struct = 3,   // structref - abstract struct type
    Array = 4,    // arrayref - abstract array type
    Func = 5,     // funcref - function reference
    Extern = 6,   // externref - external reference
    None = 7,     // nullref
    NoFunc = 8,   // nullfuncref
    NoExtern = 9, // nullexternref
    
    // Concrete type indices (0x10+)
    ConcreteStart = 0x10
};

/**
 * @brief A field in a struct type
 */
struct FieldType {
    ValType type;
    PackedType packed = PackedType::NotPacked;
    Mutability mutable_ = Mutability::Var;
    
    FieldType() = default;
    FieldType(ValType t, Mutability m = Mutability::Var)
        : type(t), packed(PackedType::NotPacked), mutable_(m) {}
    FieldType(PackedType p, Mutability m = Mutability::Var)
        : type(ValType::i32()), packed(p), mutable_(m) {}
    
    size_t size() const {
        if (packed == PackedType::I8) return 1;
        if (packed == PackedType::I16) return 2;
        switch (type.kind) {
            case ValType::Kind::I32: return 4;
            case ValType::Kind::I64: return 8;
            case ValType::Kind::F32: return 4;
            case ValType::Kind::F64: return 8;
            case ValType::Kind::V128: return 16;
            case ValType::Kind::FuncRef:
            case ValType::Kind::ExternRef:
            default: return sizeof(void*);
        }
    }
};

/**
 * @brief Struct type definition
 */
struct StructType {
    std::vector<FieldType> fields;
    uint32_t typeIndex = 0;
    uint32_t superType = UINT32_MAX;  // UINT32_MAX = no supertype
    bool isFinal = false;
    
    StructType() = default;
    
    size_t instanceSize() const {
        size_t size = 0;
        for (const auto& field : fields) {
            size += field.size();
        }
        return size;
    }
    
    size_t fieldOffset(uint32_t fieldIndex) const {
        size_t offset = 0;
        for (uint32_t i = 0; i < fieldIndex && i < fields.size(); i++) {
            offset += fields[i].size();
        }
        return offset;
    }
};

/**
 * @brief Array type definition
 */
struct ArrayType {
    FieldType elementType;
    uint32_t typeIndex = 0;
    uint32_t superType = UINT32_MAX;
    bool isFinal = false;
    
    ArrayType() = default;
    ArrayType(FieldType et) : elementType(et) {}
    
    size_t elementSize() const {
        return elementType.size();
    }
};

// =============================================================================
// GC Opcodes (0xFB prefix)
// =============================================================================

namespace GCOp {
    // Struct operations
    constexpr uint8_t STRUCT_NEW = 0x00;
    constexpr uint8_t STRUCT_NEW_DEFAULT = 0x01;
    constexpr uint8_t STRUCT_GET = 0x02;
    constexpr uint8_t STRUCT_GET_S = 0x03;
    constexpr uint8_t STRUCT_GET_U = 0x04;
    constexpr uint8_t STRUCT_SET = 0x05;
    
    // Array operations
    constexpr uint8_t ARRAY_NEW = 0x06;
    constexpr uint8_t ARRAY_NEW_DEFAULT = 0x07;
    constexpr uint8_t ARRAY_NEW_FIXED = 0x08;
    constexpr uint8_t ARRAY_NEW_DATA = 0x09;
    constexpr uint8_t ARRAY_NEW_ELEM = 0x0A;
    constexpr uint8_t ARRAY_GET = 0x0B;
    constexpr uint8_t ARRAY_GET_S = 0x0C;
    constexpr uint8_t ARRAY_GET_U = 0x0D;
    constexpr uint8_t ARRAY_SET = 0x0E;
    constexpr uint8_t ARRAY_LEN = 0x0F;
    constexpr uint8_t ARRAY_FILL = 0x10;
    constexpr uint8_t ARRAY_COPY = 0x11;
    constexpr uint8_t ARRAY_INIT_DATA = 0x12;
    constexpr uint8_t ARRAY_INIT_ELEM = 0x13;
    
    // Reference operations
    constexpr uint8_t REF_TEST = 0x14;
    constexpr uint8_t REF_TEST_NULL = 0x15;
    constexpr uint8_t REF_CAST = 0x16;
    constexpr uint8_t REF_CAST_NULL = 0x17;
    
    // Branch on type
    constexpr uint8_t BR_ON_CAST = 0x18;
    constexpr uint8_t BR_ON_CAST_FAIL = 0x19;
    
    // i31 operations
    constexpr uint8_t I31_NEW = 0x1C;
    constexpr uint8_t I31_GET_S = 0x1D;
    constexpr uint8_t I31_GET_U = 0x1E;
    
    // Null reference
    constexpr uint8_t REF_NULL = 0xD0;
    constexpr uint8_t REF_IS_NULL = 0xD1;
    constexpr uint8_t REF_AS_NON_NULL = 0xD3;
    constexpr uint8_t REF_EQ = 0xD4;
    
    // Extern conversion
    constexpr uint8_t EXTERN_INTERNALIZE = 0x1A;
    constexpr uint8_t EXTERN_EXTERNALIZE = 0x1B;
    constexpr uint8_t ANY_CONVERT_EXTERN = 0x1A;
    constexpr uint8_t EXTERN_CONVERT_ANY = 0x1B;
}

// =============================================================================
// Runtime GC Objects
// =============================================================================

/**
 * @brief Base class for all GC-managed WASM objects
 */
class WasmGCObject {
public:
    virtual ~WasmGCObject() = default;
    
    enum class Kind : uint8_t {
        Struct,
        Array,
        Func,
        I31,
        Extern
    };
    
    Kind kind() const { return kind_; }
    uint32_t typeIndex() const { return typeIndex_; }
    
    // For GC integration
    virtual void trace(void* tracer) = 0;
    virtual size_t objectSize() const = 0;
    
protected:
    WasmGCObject(Kind k, uint32_t ti) : kind_(k), typeIndex_(ti) {}
    
    Kind kind_;
    uint32_t typeIndex_;
};

/**
 * @brief Runtime struct instance
 */
class WasmStruct : public WasmGCObject {
public:
    WasmStruct(const StructType* type)
        : WasmGCObject(Kind::Struct, type->typeIndex)
        , structType_(type)
        , dataSize_(type->instanceSize()) {
        data_ = new uint8_t[dataSize_];
        std::memset(data_, 0, dataSize_);
    }
    
    ~WasmStruct() {
        delete[] data_;
    }
    
    template<typename T>
    T getField(uint32_t fieldIndex) const {
        size_t offset = structType_->fieldOffset(fieldIndex);
        T value;
        std::memcpy(&value, data_ + offset, sizeof(T));
        return value;
    }
    
    template<typename T>
    void setField(uint32_t fieldIndex, T value) {
        size_t offset = structType_->fieldOffset(fieldIndex);
        std::memcpy(data_ + offset, &value, sizeof(T));
    }
    
    void* fieldPtr(uint32_t fieldIndex) {
        return data_ + structType_->fieldOffset(fieldIndex);
    }
    
    void trace(void* tracer) override {
        // Trace reference fields
        for (size_t i = 0; i < structType_->fields.size(); i++) {
            const auto& field = structType_->fields[i];
            if (field.type.isRef()) {
                // Get reference at field offset and trace it
                (void)tracer;
            }
        }
    }
    
    size_t objectSize() const override {
        return sizeof(WasmStruct) + dataSize_;
    }
    
private:
    const StructType* structType_;
    uint8_t* data_;
    size_t dataSize_;
};

/**
 * @brief Runtime array instance
 */
class WasmArray : public WasmGCObject {
public:
    WasmArray(const ArrayType* type, uint32_t length)
        : WasmGCObject(Kind::Array, type->typeIndex)
        , arrayType_(type)
        , length_(length) {
        size_t dataSize = length * type->elementSize();
        data_ = new uint8_t[dataSize];
        std::memset(data_, 0, dataSize);
    }
    
    ~WasmArray() {
        delete[] data_;
    }
    
    uint32_t length() const { return length_; }
    
    template<typename T>
    T getElement(uint32_t index) const {
        size_t offset = index * arrayType_->elementSize();
        T value;
        std::memcpy(&value, data_ + offset, sizeof(T));
        return value;
    }
    
    template<typename T>
    void setElement(uint32_t index, T value) {
        size_t offset = index * arrayType_->elementSize();
        std::memcpy(data_ + offset, &value, sizeof(T));
    }
    
    void* elementPtr(uint32_t index) {
        return data_ + index * arrayType_->elementSize();
    }
    
    void fill(uint32_t destIndex, const void* value, uint32_t count) {
        size_t elemSize = arrayType_->elementSize();
        for (uint32_t i = 0; i < count; i++) {
            std::memcpy(data_ + (destIndex + i) * elemSize, value, elemSize);
        }
    }
    
    void copy(WasmArray* src, uint32_t destIndex, uint32_t srcIndex, uint32_t count) {
        size_t elemSize = arrayType_->elementSize();
        std::memmove(data_ + destIndex * elemSize,
                     src->data_ + srcIndex * elemSize,
                     count * elemSize);
    }
    
    void trace(void* tracer) override {
        if (arrayType_->elementType.type.isRef()) {
            (void)tracer;
        }
    }
    
    size_t objectSize() const override {
        return sizeof(WasmArray) + length_ * arrayType_->elementSize();
    }
    
private:
    const ArrayType* arrayType_;
    uint8_t* data_;
    uint32_t length_;
};

/**
 * @brief i31ref - 31-bit integer as reference
 * 
 * Uses pointer tagging: low bit = 1 indicates i31
 */
class WasmI31 {
public:
    static constexpr uintptr_t I31_TAG = 1;
    
    static uintptr_t encode(int32_t value) {
        // Shift left 1 and set tag bit
        return (static_cast<uintptr_t>(value) << 1) | I31_TAG;
    }
    
    static int32_t decodeS(uintptr_t tagged) {
        // Sign-extend from 31 bits
        int32_t shifted = static_cast<int32_t>(tagged >> 1);
        return (shifted << 1) >> 1;  // Sign extend
    }
    
    static uint32_t decodeU(uintptr_t tagged) {
        return static_cast<uint32_t>((tagged >> 1) & 0x7FFFFFFF);
    }
    
    static bool isI31(uintptr_t ptr) {
        return (ptr & I31_TAG) != 0;
    }
};

// =============================================================================
// Type Registry and Subtyping
// =============================================================================

/**
 * @brief Manages GC type definitions and subtyping checks
 */
class TypeRegistry {
public:
    uint32_t addStructType(StructType type) {
        type.typeIndex = static_cast<uint32_t>(structTypes_.size());
        structTypes_.push_back(std::move(type));
        return type.typeIndex;
    }
    
    uint32_t addArrayType(ArrayType type) {
        type.typeIndex = static_cast<uint32_t>(arrayTypes_.size());
        arrayTypes_.push_back(std::move(type));
        return type.typeIndex;
    }
    
    const StructType* getStructType(uint32_t index) const {
        return index < structTypes_.size() ? &structTypes_[index] : nullptr;
    }
    
    const ArrayType* getArrayType(uint32_t index) const {
        return index < arrayTypes_.size() ? &arrayTypes_[index] : nullptr;
    }
    
    // Check if sub is a subtype of super
    bool isSubtype(uint32_t subIndex, uint32_t superIndex) const {
        if (subIndex == superIndex) return true;
        
        const StructType* sub = getStructType(subIndex);
        if (sub && sub->superType != UINT32_MAX) {
            return isSubtype(sub->superType, superIndex);
        }
        return false;
    }
    
    // Runtime type check
    bool refTest(const WasmGCObject* obj, uint32_t targetType) const {
        if (!obj) return false;
        return isSubtype(obj->typeIndex(), targetType);
    }
    
    // Runtime cast (throws on failure)
    WasmGCObject* refCast(WasmGCObject* obj, uint32_t targetType) const {
        if (!obj || !isSubtype(obj->typeIndex(), targetType)) {
            return nullptr;  // Would trap in real impl
        }
        return obj;
    }
    
private:
    std::vector<StructType> structTypes_;
    std::vector<ArrayType> arrayTypes_;
};

// =============================================================================
// GC Allocation
// =============================================================================

/**
 * @brief Interface to ZepraScript's GC for allocating WASM GC objects
 */
class WasmGCAllocator {
public:
    virtual ~WasmGCAllocator() = default;
    
    virtual WasmStruct* allocStruct(const StructType* type) = 0;
    virtual WasmArray* allocArray(const ArrayType* type, uint32_t length) = 0;
    
    virtual void triggerGC() = 0;
    virtual size_t heapSize() const = 0;
};

/**
 * @brief Simple bump allocator for WASM GC objects
 */
class SimpleWasmGCAllocator : public WasmGCAllocator {
public:
    WasmStruct* allocStruct(const StructType* type) override {
        auto* obj = new WasmStruct(type);
        objects_.push_back(std::unique_ptr<WasmGCObject>(obj));
        return obj;
    }
    
    WasmArray* allocArray(const ArrayType* type, uint32_t length) override {
        auto* obj = new WasmArray(type, length);
        objects_.push_back(std::unique_ptr<WasmGCObject>(obj));
        return obj;
    }
    
    void triggerGC() override {
        // Simple GC: just clear unreachable objects
        // Real implementation would use mark-sweep or similar
    }
    
    size_t heapSize() const override {
        size_t size = 0;
        for (const auto& obj : objects_) {
            size += obj->objectSize();
        }
        return size;
    }
    
private:
    std::vector<std::unique_ptr<WasmGCObject>> objects_;
};

} // namespace Zepra::Wasm
