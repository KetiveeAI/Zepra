/**
 * @file WasmValType.h
 * @brief WebAssembly value types and heap types
 * 
 * Provides type representation for WASM values with support for:
 * - Numeric types (i32, i64, f32, f64, v128)
 * - Reference types (funcref, externref, anyref, etc.)
 * - GC types (structref, arrayref, i31ref)
 * 
 * Based on Firefox SpiderMonkey WasmValType.h
 */

#pragma once

#include "WasmConstants.h"
#include <cstdint>
#include <string>
#include <optional>
#include <variant>

namespace Zepra::Wasm {

// =============================================================================
// Packed Types (for struct/array fields)
// =============================================================================

enum class PackedType : uint8_t {
    I8,
    I16,
    Not  // Not packed (full type)
};

// =============================================================================
// Heap Type (for reference types)
// =============================================================================

class HeapType {
public:
    enum class Kind : uint8_t {
        // Abstract heap types
        Func,       // (ref func)
        Extern,     // (ref extern)
        Any,        // (ref any)
        Eq,         // (ref eq)
        I31,        // (ref i31)
        Struct,     // (ref struct)
        Array,      // (ref array)
        None,       // (ref none) - bottom of any
        NoFunc,     // (ref nofunc) - bottom of func
        NoExtern,   // (ref noextern) - bottom of extern
        Exn,        // (ref exn) - exception
        NoExn,      // (ref noexn) - bottom of exn
        
        // Concrete types (index into type section)
        TypeIndex
    };
    
    HeapType() : kind_(Kind::Any), typeIndex_(0) {}
    explicit HeapType(Kind kind) : kind_(kind), typeIndex_(0) {}
    HeapType(Kind kind, uint32_t typeIndex) : kind_(kind), typeIndex_(typeIndex) {}
    
    static HeapType func() { return HeapType(Kind::Func); }
    static HeapType extern_() { return HeapType(Kind::Extern); }
    static HeapType any() { return HeapType(Kind::Any); }
    static HeapType eq() { return HeapType(Kind::Eq); }
    static HeapType i31() { return HeapType(Kind::I31); }
    static HeapType struct_() { return HeapType(Kind::Struct); }
    static HeapType array() { return HeapType(Kind::Array); }
    static HeapType none() { return HeapType(Kind::None); }
    static HeapType noFunc() { return HeapType(Kind::NoFunc); }
    static HeapType noExtern() { return HeapType(Kind::NoExtern); }
    static HeapType exn() { return HeapType(Kind::Exn); }
    
    static HeapType fromTypeIndex(uint32_t idx) {
        return HeapType(Kind::TypeIndex, idx);
    }
    
    Kind kind() const { return kind_; }
    bool isTypeIndex() const { return kind_ == Kind::TypeIndex; }
    uint32_t typeIndex() const { return typeIndex_; }
    
    bool isFunc() const { return kind_ == Kind::Func; }
    bool isExtern() const { return kind_ == Kind::Extern; }
    bool isAny() const { return kind_ == Kind::Any; }
    bool isEq() const { return kind_ == Kind::Eq; }
    bool isI31() const { return kind_ == Kind::I31; }
    bool isStruct() const { return kind_ == Kind::Struct; }
    bool isArray() const { return kind_ == Kind::Array; }
    bool isNone() const { return kind_ == Kind::None; }
    bool isExn() const { return kind_ == Kind::Exn; }
    
    // Check if this is a bottom type
    bool isBottom() const {
        return kind_ == Kind::None || kind_ == Kind::NoFunc || 
               kind_ == Kind::NoExtern || kind_ == Kind::NoExn;
    }
    
    // Subtyping
    bool isSubtypeOf(const HeapType& other) const;
    
    bool operator==(const HeapType& other) const {
        return kind_ == other.kind_ && typeIndex_ == other.typeIndex_;
    }
    bool operator!=(const HeapType& other) const { return !(*this == other); }
    
    std::string toString() const;
    
private:
    Kind kind_;
    uint32_t typeIndex_;
};

// =============================================================================
// Reference Type
// =============================================================================

class RefType {
public:
    RefType() : heapType_(HeapType::any()), nullable_(true) {}
    RefType(HeapType heapType, bool nullable)
        : heapType_(heapType), nullable_(nullable) {}
    
    static RefType funcRef() { return RefType(HeapType::func(), true); }
    static RefType externRef() { return RefType(HeapType::extern_(), true); }
    static RefType anyRef() { return RefType(HeapType::any(), true); }
    static RefType eqRef() { return RefType(HeapType::eq(), true); }
    static RefType i31Ref() { return RefType(HeapType::i31(), true); }
    static RefType structRef() { return RefType(HeapType::struct_(), true); }
    static RefType arrayRef() { return RefType(HeapType::array(), true); }
    static RefType exnRef() { return RefType(HeapType::exn(), true); }
    
    static RefType nullFuncRef() { return RefType(HeapType::noFunc(), true); }
    static RefType nullExternRef() { return RefType(HeapType::noExtern(), true); }
    static RefType nullRef() { return RefType(HeapType::none(), true); }
    
    HeapType heapType() const { return heapType_; }
    bool isNullable() const { return nullable_; }
    
    RefType asNonNullable() const {
        return RefType(heapType_, false);
    }
    
    RefType asNullable() const {
        return RefType(heapType_, true);
    }
    
    bool isSubtypeOf(const RefType& other) const {
        // Non-nullable is subtype of nullable with same heap type
        if (!other.nullable_ && nullable_) return false;
        return heapType_.isSubtypeOf(other.heapType_);
    }
    
    bool operator==(const RefType& other) const {
        return heapType_ == other.heapType_ && nullable_ == other.nullable_;
    }
    bool operator!=(const RefType& other) const { return !(*this == other); }
    
    std::string toString() const;
    
private:
    HeapType heapType_;
    bool nullable_;
};

// =============================================================================
// Value Type
// =============================================================================

class ValType {
public:
    enum class Kind : uint8_t {
        I32,
        I64,
        F32,
        F64,
        V128,
        Ref
    };
    
    ValType() : kind_(Kind::I32) {}
    explicit ValType(Kind kind) : kind_(kind) {}
    ValType(Kind kind, RefType refType) : kind_(kind), refType_(refType) {}
    
    // Numeric type constructors
    static ValType i32() { return ValType(Kind::I32); }
    static ValType i64() { return ValType(Kind::I64); }
    static ValType f32() { return ValType(Kind::F32); }
    static ValType f64() { return ValType(Kind::F64); }
    static ValType v128() { return ValType(Kind::V128); }
    
    // Reference type constructors
    static ValType funcRef() {
        return ValType(Kind::Ref, RefType::funcRef());
    }
    static ValType externRef() {
        return ValType(Kind::Ref, RefType::externRef());
    }
    static ValType anyRef() {
        return ValType(Kind::Ref, RefType::anyRef());
    }
    static ValType eqRef() {
        return ValType(Kind::Ref, RefType::eqRef());
    }
    static ValType i31Ref() {
        return ValType(Kind::Ref, RefType::i31Ref());
    }
    static ValType structRef() {
        return ValType(Kind::Ref, RefType::structRef());
    }
    static ValType arrayRef() {
        return ValType(Kind::Ref, RefType::arrayRef());
    }
    static ValType exnRef() {
        return ValType(Kind::Ref, RefType::exnRef());
    }
    
    static ValType ref(HeapType heapType, bool nullable) {
        return ValType(Kind::Ref, RefType(heapType, nullable));
    }
    
    // From TypeCode
    static std::optional<ValType> fromTypeCode(TypeCode code);
    
    Kind kind() const { return kind_; }
    
    bool isNumeric() const {
        return kind_ == Kind::I32 || kind_ == Kind::I64 ||
               kind_ == Kind::F32 || kind_ == Kind::F64;
    }
    
    bool isVector() const { return kind_ == Kind::V128; }
    bool isReference() const { return kind_ == Kind::Ref; }
    
    bool isI32() const { return kind_ == Kind::I32; }
    bool isI64() const { return kind_ == Kind::I64; }
    bool isF32() const { return kind_ == Kind::F32; }
    bool isF64() const { return kind_ == Kind::F64; }
    bool isV128() const { return kind_ == Kind::V128; }
    
    RefType refType() const { return refType_; }
    
    // Size in bytes
    size_t size() const {
        switch (kind_) {
            case Kind::I32: return 4;
            case Kind::I64: return 8;
            case Kind::F32: return 4;
            case Kind::F64: return 8;
            case Kind::V128: return 16;
            case Kind::Ref: return sizeof(void*);
        }
        return 0;
    }
    
    // Alignment
    size_t alignment() const { return size(); }
    
    // Subtyping
    bool isSubtypeOf(const ValType& other) const {
        if (kind_ != other.kind_) return false;
        if (kind_ == Kind::Ref) {
            return refType_.isSubtypeOf(other.refType_);
        }
        return true;
    }
    
    bool operator==(const ValType& other) const {
        if (kind_ != other.kind_) return false;
        if (kind_ == Kind::Ref) return refType_ == other.refType_;
        return true;
    }
    bool operator!=(const ValType& other) const { return !(*this == other); }
    
    TypeCode toTypeCode() const;
    std::string toString() const;
    
private:
    Kind kind_;
    RefType refType_;  // Only valid when kind_ == Ref
};

// =============================================================================
// Block Type (for control instructions)
// =============================================================================

class BlockType {
public:
    enum class Kind : uint8_t {
        Void,           // No parameters, no results
        Single,         // No parameters, single result
        TypeIndex       // Function type index
    };
    
    BlockType() : kind_(Kind::Void) {}
    
    static BlockType void_() { return BlockType(); }
    
    static BlockType single(ValType type) {
        BlockType bt;
        bt.kind_ = Kind::Single;
        bt.singleType_ = type;
        return bt;
    }
    
    static BlockType typeIndex(uint32_t idx) {
        BlockType bt;
        bt.kind_ = Kind::TypeIndex;
        bt.typeIndex_ = idx;
        return bt;
    }
    
    Kind kind() const { return kind_; }
    bool isVoid() const { return kind_ == Kind::Void; }
    bool isSingle() const { return kind_ == Kind::Single; }
    bool isTypeIndex() const { return kind_ == Kind::TypeIndex; }
    
    ValType singleType() const { return singleType_; }
    uint32_t typeIndex() const { return typeIndex_; }
    
private:
    Kind kind_ = Kind::Void;
    ValType singleType_;
    uint32_t typeIndex_ = 0;
};

// =============================================================================
// Field Type (for structs/arrays)
// =============================================================================

class FieldType {
public:
    FieldType() = default;
    FieldType(ValType valType, bool mutable_)
        : valType_(valType), packed_(PackedType::Not), mutable_(mutable_) {}
    FieldType(PackedType packed, bool mutable_)
        : valType_(ValType::i32()), packed_(packed), mutable_(mutable_) {}
    
    bool isPacked() const { return packed_ != PackedType::Not; }
    PackedType packedType() const { return packed_; }
    ValType valType() const { return valType_; }
    bool isMutable() const { return mutable_; }
    
    // Unpacked type (for loading)
    ValType unpackedType() const {
        if (isPacked()) return ValType::i32();
        return valType_;
    }
    
    // Size in bytes
    size_t size() const {
        switch (packed_) {
            case PackedType::I8: return 1;
            case PackedType::I16: return 2;
            case PackedType::Not: return valType_.size();
        }
        return 0;
    }
    
    bool operator==(const FieldType& other) const {
        return valType_ == other.valType_ && 
               packed_ == other.packed_ && 
               mutable_ == other.mutable_;
    }
    bool operator!=(const FieldType& other) const { return !(*this == other); }
    
private:
    ValType valType_;
    PackedType packed_ = PackedType::Not;
    bool mutable_ = false;
};

// =============================================================================
// Implementation of inline methods
// =============================================================================

inline bool HeapType::isSubtypeOf(const HeapType& other) const {
    if (*this == other) return true;
    
    // Bottom types are subtypes of everything in their hierarchy
    if (kind_ == Kind::None) {
        return other.isAny() || other.isEq() || other.isI31() || 
               other.isStruct() || other.isArray();
    }
    if (kind_ == Kind::NoFunc) return other.isFunc();
    if (kind_ == Kind::NoExtern) return other.isExtern();
    if (kind_ == Kind::NoExn) return other.isExn();
    
    // Hierarchy: any > eq > {i31, struct, array}
    if (other.isAny()) {
        return kind_ == Kind::Eq || kind_ == Kind::I31 || 
               kind_ == Kind::Struct || kind_ == Kind::Array;
    }
    if (other.isEq()) {
        return kind_ == Kind::I31 || kind_ == Kind::Struct || kind_ == Kind::Array;
    }
    
    return false;
}

inline std::string HeapType::toString() const {
    switch (kind_) {
        case Kind::Func: return "func";
        case Kind::Extern: return "extern";
        case Kind::Any: return "any";
        case Kind::Eq: return "eq";
        case Kind::I31: return "i31";
        case Kind::Struct: return "struct";
        case Kind::Array: return "array";
        case Kind::None: return "none";
        case Kind::NoFunc: return "nofunc";
        case Kind::NoExtern: return "noextern";
        case Kind::Exn: return "exn";
        case Kind::NoExn: return "noexn";
        case Kind::TypeIndex: return "$" + std::to_string(typeIndex_);
    }
    return "unknown";
}

inline std::string RefType::toString() const {
    std::string result = "(ref ";
    if (nullable_) result += "null ";
    result += heapType_.toString();
    result += ")";
    return result;
}

inline std::optional<ValType> ValType::fromTypeCode(TypeCode code) {
    switch (code) {
        case TypeCode::I32: return ValType::i32();
        case TypeCode::I64: return ValType::i64();
        case TypeCode::F32: return ValType::f32();
        case TypeCode::F64: return ValType::f64();
        case TypeCode::V128: return ValType::v128();
        case TypeCode::FuncRef: return ValType::funcRef();
        case TypeCode::ExternRef: return ValType::externRef();
        case TypeCode::AnyRef: return ValType::anyRef();
        case TypeCode::EqRef: return ValType::eqRef();
        case TypeCode::I31Ref: return ValType::i31Ref();
        case TypeCode::StructRef: return ValType::structRef();
        case TypeCode::ArrayRef: return ValType::arrayRef();
        case TypeCode::ExnRef: return ValType::exnRef();
        default: return std::nullopt;
    }
}

inline TypeCode ValType::toTypeCode() const {
    switch (kind_) {
        case Kind::I32: return TypeCode::I32;
        case Kind::I64: return TypeCode::I64;
        case Kind::F32: return TypeCode::F32;
        case Kind::F64: return TypeCode::F64;
        case Kind::V128: return TypeCode::V128;
        case Kind::Ref: {
            if (!refType_.isNullable()) return TypeCode::Ref;
            switch (refType_.heapType().kind()) {
                case HeapType::Kind::Func: return TypeCode::FuncRef;
                case HeapType::Kind::Extern: return TypeCode::ExternRef;
                case HeapType::Kind::Any: return TypeCode::AnyRef;
                case HeapType::Kind::Eq: return TypeCode::EqRef;
                case HeapType::Kind::I31: return TypeCode::I31Ref;
                case HeapType::Kind::Struct: return TypeCode::StructRef;
                case HeapType::Kind::Array: return TypeCode::ArrayRef;
                case HeapType::Kind::Exn: return TypeCode::ExnRef;
                default: return TypeCode::NullableRef;
            }
        }
    }
    return TypeCode::I32;
}

inline std::string ValType::toString() const {
    switch (kind_) {
        case Kind::I32: return "i32";
        case Kind::I64: return "i64";
        case Kind::F32: return "f32";
        case Kind::F64: return "f64";
        case Kind::V128: return "v128";
        case Kind::Ref: return refType_.toString();
    }
    return "unknown";
}

} // namespace Zepra::Wasm
