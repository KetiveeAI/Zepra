/**
 * @file WasmTypeDef.h
 * @brief WebAssembly type definitions (function, struct, array)
 * 
 * Defines composite types for the type section including
 * function signatures, structs (GC), and arrays (GC).
 * 
 * Based on Firefox SpiderMonkey WasmTypeDef.h
 */

#pragma once

#include "WasmValType.h"
#include <vector>
#include <memory>
#include <optional>

namespace Zepra::Wasm {

// =============================================================================
// Function Type
// =============================================================================

class FuncType {
public:
    FuncType() = default;
    FuncType(std::vector<ValType> params, std::vector<ValType> results)
        : params_(std::move(params)), results_(std::move(results)) {}
    
    const std::vector<ValType>& params() const { return params_; }
    const std::vector<ValType>& results() const { return results_; }
    
    size_t numParams() const { return params_.size(); }
    size_t numResults() const { return results_.size(); }
    
    ValType param(size_t i) const { return params_[i]; }
    ValType result(size_t i) const { return results_[i]; }
    
    bool hasResult() const { return !results_.empty(); }
    bool hasMultipleResults() const { return results_.size() > 1; }
    
    // Check if this can be called with given args
    bool matches(const std::vector<ValType>& args) const {
        if (args.size() != params_.size()) return false;
        for (size_t i = 0; i < args.size(); i++) {
            if (!args[i].isSubtypeOf(params_[i])) return false;
        }
        return true;
    }
    
    bool operator==(const FuncType& other) const {
        return params_ == other.params_ && results_ == other.results_;
    }
    bool operator!=(const FuncType& other) const { return !(*this == other); }
    
    std::string toString() const {
        std::string s = "(func";
        if (!params_.empty()) {
            s += " (param";
            for (const auto& p : params_) {
                s += " " + p.toString();
            }
            s += ")";
        }
        if (!results_.empty()) {
            s += " (result";
            for (const auto& r : results_) {
                s += " " + r.toString();
            }
            s += ")";
        }
        s += ")";
        return s;
    }
    
private:
    std::vector<ValType> params_;
    std::vector<ValType> results_;
};

// =============================================================================
// Struct Type (GC proposal)
// =============================================================================

class StructType {
public:
    struct Field {
        FieldType type;
        std::string name;  // Optional, from name section
        uint32_t offset;   // Computed during layout
    };
    
    StructType() = default;
    explicit StructType(std::vector<Field> fields)
        : fields_(std::move(fields)) {
        computeLayout();
    }
    
    const std::vector<Field>& fields() const { return fields_; }
    size_t numFields() const { return fields_.size(); }
    
    const Field& field(size_t i) const { return fields_[i]; }
    FieldType fieldType(size_t i) const { return fields_[i].type; }
    
    uint32_t instanceSize() const { return instanceSize_; }
    uint32_t alignment() const { return alignment_; }
    
    uint32_t fieldOffset(size_t i) const { return fields_[i].offset; }
    
    bool operator==(const StructType& other) const {
        if (fields_.size() != other.fields_.size()) return false;
        for (size_t i = 0; i < fields_.size(); i++) {
            if (fields_[i].type != other.fields_[i].type) return false;
        }
        return true;
    }
    
    std::string toString() const {
        std::string s = "(struct";
        for (const auto& f : fields_) {
            s += " (field";
            if (!f.name.empty()) s += " $" + f.name;
            if (f.type.isMutable()) s += " (mut";
            s += " " + f.type.valType().toString();
            if (f.type.isMutable()) s += ")";
            s += ")";
        }
        s += ")";
        return s;
    }
    
private:
    void computeLayout() {
        uint32_t offset = 0;
        alignment_ = 1;
        
        for (auto& field : fields_) {
            size_t fieldAlign = field.type.size();
            if (fieldAlign > alignment_) alignment_ = fieldAlign;
            
            // Align offset
            offset = (offset + fieldAlign - 1) & ~(fieldAlign - 1);
            field.offset = offset;
            offset += field.type.size();
        }
        
        // Final size aligned
        instanceSize_ = (offset + alignment_ - 1) & ~(alignment_ - 1);
    }
    
    std::vector<Field> fields_;
    uint32_t instanceSize_ = 0;
    uint32_t alignment_ = 1;
};

// =============================================================================
// Array Type (GC proposal)
// =============================================================================

class ArrayType {
public:
    ArrayType() = default;
    explicit ArrayType(FieldType elementType)
        : elementType_(elementType) {}
    
    FieldType elementType() const { return elementType_; }
    bool isMutable() const { return elementType_.isMutable(); }
    
    size_t elementSize() const { return elementType_.size(); }
    
    size_t instanceSize(uint32_t length) const {
        // Header + elements
        return sizeof(uint32_t) + length * elementSize();
    }
    
    bool operator==(const ArrayType& other) const {
        return elementType_ == other.elementType_;
    }
    
    std::string toString() const {
        std::string s = "(array";
        if (elementType_.isMutable()) s += " (mut";
        s += " " + elementType_.valType().toString();
        if (elementType_.isMutable()) s += ")";
        s += ")";
        return s;
    }
    
private:
    FieldType elementType_;
};

// =============================================================================
// Type Definition (discriminated union)
// =============================================================================

class TypeDef {
public:
    enum class Kind : uint8_t {
        Func,
        Struct,
        Array
    };
    
    TypeDef() : kind_(Kind::Func) {}
    
    static TypeDef func(FuncType type) {
        TypeDef td;
        td.kind_ = Kind::Func;
        td.funcType_ = std::move(type);
        return td;
    }
    
    static TypeDef struct_(StructType type) {
        TypeDef td;
        td.kind_ = Kind::Struct;
        td.structType_ = std::move(type);
        return td;
    }
    
    static TypeDef array(ArrayType type) {
        TypeDef td;
        td.kind_ = Kind::Array;
        td.arrayType_ = std::move(type);
        return td;
    }
    
    Kind kind() const { return kind_; }
    bool isFuncType() const { return kind_ == Kind::Func; }
    bool isStructType() const { return kind_ == Kind::Struct; }
    bool isArrayType() const { return kind_ == Kind::Array; }
    
    const FuncType& funcType() const { return funcType_; }
    const StructType& structType() const { return structType_; }
    const ArrayType& arrayType() const { return arrayType_; }
    
    // Supertype (for subtyping - GC proposal)
    bool hasSuperType() const { return superTypeIndex_.has_value(); }
    uint32_t superTypeIndex() const { return superTypeIndex_.value(); }
    void setSuperTypeIndex(uint32_t idx) { superTypeIndex_ = idx; }
    
    bool isFinal() const { return isFinal_; }
    void setFinal(bool final) { isFinal_ = final; }
    
    std::string toString() const {
        switch (kind_) {
            case Kind::Func: return funcType_.toString();
            case Kind::Struct: return structType_.toString();
            case Kind::Array: return arrayType_.toString();
        }
        return "";
    }
    
private:
    Kind kind_;
    FuncType funcType_;
    StructType structType_;
    ArrayType arrayType_;
    std::optional<uint32_t> superTypeIndex_;
    bool isFinal_ = true;  // Default to final
};

// =============================================================================
// Recursion Group (GC proposal)
// =============================================================================

class RecGroup {
public:
    RecGroup() = default;
    explicit RecGroup(std::vector<TypeDef> types)
        : types_(std::move(types)) {}
    
    const std::vector<TypeDef>& types() const { return types_; }
    size_t size() const { return types_.size(); }
    
    const TypeDef& type(size_t i) const { return types_[i]; }
    TypeDef& type(size_t i) { return types_[i]; }
    
    void addType(TypeDef type) { types_.push_back(std::move(type)); }
    
    friend class TypeSection;
private:
    std::vector<TypeDef> types_;
};

// =============================================================================
// Type Section
// =============================================================================

class TypeSection {
public:
    TypeSection() = default;
    
    void addType(TypeDef type) {
        types_.push_back(std::move(type));
    }
    
    void addRecGroup(RecGroup group) {
        for (auto& type : group.types_) {
            types_.push_back(std::move(type));
        }
    }
    
    size_t size() const { return types_.size(); }
    const TypeDef& type(uint32_t idx) const { return types_[idx]; }
    TypeDef& type(uint32_t idx) { return types_[idx]; }
    
    // Subscript operator for convenient access
    const TypeDef& operator[](uint32_t idx) const { return types_[idx]; }
    TypeDef& operator[](uint32_t idx) { return types_[idx]; }
    
    const FuncType* funcType(uint32_t idx) const {
        if (idx >= types_.size()) return nullptr;
        if (!types_[idx].isFuncType()) return nullptr;
        return &types_[idx].funcType();
    }
    
    const StructType* structType(uint32_t idx) const {
        if (idx >= types_.size()) return nullptr;
        if (!types_[idx].isStructType()) return nullptr;
        return &types_[idx].structType();
    }
    
    const ArrayType* arrayType(uint32_t idx) const {
        if (idx >= types_.size()) return nullptr;
        if (!types_[idx].isArrayType()) return nullptr;
        return &types_[idx].arrayType();
    }
    
    // Subtyping check
    bool isSubtypeOf(uint32_t subIdx, uint32_t superIdx) const {
        if (subIdx == superIdx) return true;
        if (subIdx >= types_.size() || superIdx >= types_.size()) return false;
        
        const auto& sub = types_[subIdx];
        if (!sub.hasSuperType()) return false;
        
        // Walk up the hierarchy
        uint32_t current = sub.superTypeIndex();
        while (current != superIdx) {
            if (current >= types_.size()) return false;
            const auto& currentType = types_[current];
            if (!currentType.hasSuperType()) return false;
            current = currentType.superTypeIndex();
        }
        return true;
    }
    
private:
    std::vector<TypeDef> types_;
};

} // namespace Zepra::Wasm
