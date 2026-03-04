/**
 * @file WasmInterfaceTypes.h
 * @brief Interface Types for Component Model
 * 
 * Implements:
 * - Interface type definitions
 * - Type adapter generation
 * - Cross-component value passing
 */

#pragma once

#include "wasm.hpp"
#include "WasmCanonicalABI.h"
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <unordered_map>

namespace Zepra::Wasm {

// =============================================================================
// Interface Type Definitions
// =============================================================================

/**
 * @brief Interface type kinds
 */
enum class InterfaceTypeKind {
    // Primitives
    Bool,
    U8, U16, U32, U64,
    S8, S16, S32, S64,
    F32, F64,
    Char,
    String,
    
    // Compound types
    List,
    Record,
    Tuple,
    Variant,
    Enum,
    Option,
    Result,
    Flags,
    
    // Resource types
    Own,
    Borrow
};

// Forward declaration
class InterfaceType;

/**
 * @brief Field in a record type
 */
struct RecordField {
    std::string name;
    std::shared_ptr<InterfaceType> type;
};

/**
 * @brief Case in a variant type
 */
struct VariantCase {
    std::string name;
    std::shared_ptr<InterfaceType> type;  // nullptr for empty case
};

/**
 * @brief Complete interface type definition
 */
class InterfaceType {
public:
    InterfaceTypeKind kind;
    
    // For list types
    std::shared_ptr<InterfaceType> elementType;
    
    // For record types
    std::vector<RecordField> fields;
    
    // For tuple types
    std::vector<std::shared_ptr<InterfaceType>> tupleTypes;
    
    // For variant types
    std::vector<VariantCase> cases;
    
    // For enum types
    std::vector<std::string> enumCases;
    
    // For option types
    std::shared_ptr<InterfaceType> someType;
    
    // For result types
    std::shared_ptr<InterfaceType> okType;
    std::shared_ptr<InterfaceType> errType;
    
    // For flags types
    std::vector<std::string> flagNames;
    
    // For resource types
    std::string resourceName;
    
    // Factory methods for primitives
    static std::shared_ptr<InterfaceType> Bool() {
        auto t = std::make_shared<InterfaceType>();
        t->kind = InterfaceTypeKind::Bool;
        return t;
    }
    
    static std::shared_ptr<InterfaceType> U32() {
        auto t = std::make_shared<InterfaceType>();
        t->kind = InterfaceTypeKind::U32;
        return t;
    }
    
    static std::shared_ptr<InterfaceType> String() {
        auto t = std::make_shared<InterfaceType>();
        t->kind = InterfaceTypeKind::String;
        return t;
    }
    
    static std::shared_ptr<InterfaceType> List(std::shared_ptr<InterfaceType> elem) {
        auto t = std::make_shared<InterfaceType>();
        t->kind = InterfaceTypeKind::List;
        t->elementType = std::move(elem);
        return t;
    }
    
    static std::shared_ptr<InterfaceType> Record(std::vector<RecordField> fields) {
        auto t = std::make_shared<InterfaceType>();
        t->kind = InterfaceTypeKind::Record;
        t->fields = std::move(fields);
        return t;
    }
    
    static std::shared_ptr<InterfaceType> Variant(std::vector<VariantCase> cases) {
        auto t = std::make_shared<InterfaceType>();
        t->kind = InterfaceTypeKind::Variant;
        t->cases = std::move(cases);
        return t;
    }
    
    static std::shared_ptr<InterfaceType> Option(std::shared_ptr<InterfaceType> some) {
        auto t = std::make_shared<InterfaceType>();
        t->kind = InterfaceTypeKind::Option;
        t->someType = std::move(some);
        return t;
    }
    
    static std::shared_ptr<InterfaceType> Result(std::shared_ptr<InterfaceType> ok,
                                                  std::shared_ptr<InterfaceType> err) {
        auto t = std::make_shared<InterfaceType>();
        t->kind = InterfaceTypeKind::Result;
        t->okType = std::move(ok);
        t->errType = std::move(err);
        return t;
    }
};

// =============================================================================
// Interface Values
// =============================================================================

/**
 * @brief Runtime interface value
 */
class InterfaceValue {
public:
    using ValueVariant = std::variant<
        bool,                                    // Bool
        uint8_t, uint16_t, uint32_t, uint64_t,  // Unsigned
        int8_t, int16_t, int32_t, int64_t,      // Signed
        float, double,                           // Float
        char32_t,                               // Char
        std::string,                            // String
        std::vector<InterfaceValue>,            // List/Tuple
        std::unordered_map<std::string, InterfaceValue>,  // Record
        std::pair<uint32_t, InterfaceValue>,    // Variant (discriminant, value)
        std::optional<InterfaceValue>,          // Option
        uint32_t                                // ResourceHandle
    >;
    
    ValueVariant value;
    std::shared_ptr<InterfaceType> type;
    
    // Accessors
    bool asBool() const { return std::get<bool>(value); }
    uint32_t asU32() const { return std::get<uint32_t>(value); }
    int32_t asS32() const { return std::get<int32_t>(value); }
    const std::string& asString() const { return std::get<std::string>(value); }
    const std::vector<InterfaceValue>& asList() const { 
        return std::get<std::vector<InterfaceValue>>(value); 
    }
    const auto& asRecord() const {
        return std::get<std::unordered_map<std::string, InterfaceValue>>(value);
    }
};

// =============================================================================
// Type Adapter
// =============================================================================

/**
 * @brief Adapts values between interface types and core WASM types
 */
class TypeAdapter {
public:
    explicit TypeAdapter(CanonicalABI* abi) : abi_(abi) {}
    
    // Lower interface value to flat values
    FlatValue lower(const InterfaceValue& value) {
        FlatValue result;
        lowerValue(value, result);
        return result;
    }
    
    // Lift flat values to interface value
    InterfaceValue lift(const FlatValue& flat, std::shared_ptr<InterfaceType> type) {
        size_t i32Idx = 0, i64Idx = 0, f32Idx = 0, f64Idx = 0;
        return liftValue(flat, type, i32Idx, i64Idx, f32Idx, f64Idx);
    }
    
private:
    void lowerValue(const InterfaceValue& value, FlatValue& out) {
        switch (value.type->kind) {
            case InterfaceTypeKind::Bool:
                out.i32s.push_back(abi_->lowerBool(value.asBool()));
                break;
            case InterfaceTypeKind::U32:
                out.i32s.push_back(value.asU32());
                break;
            case InterfaceTypeKind::S32:
                out.i32s.push_back(static_cast<uint32_t>(value.asS32()));
                break;
            case InterfaceTypeKind::String: {
                // Would need to allocate in linear memory
                break;
            }
            case InterfaceTypeKind::List: {
                for (const auto& elem : value.asList()) {
                    lowerValue(elem, out);
                }
                break;
            }
            case InterfaceTypeKind::Record: {
                for (const auto& field : value.type->fields) {
                    auto it = value.asRecord().find(field.name);
                    if (it != value.asRecord().end()) {
                        lowerValue(it->second, out);
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    
    InterfaceValue liftValue(const FlatValue& flat, 
                             std::shared_ptr<InterfaceType> type,
                             size_t& i32Idx, size_t& i64Idx,
                             size_t& f32Idx, size_t& f64Idx) {
        InterfaceValue result;
        result.type = type;
        
        switch (type->kind) {
            case InterfaceTypeKind::Bool:
                result.value = abi_->liftBool(flat.i32s[i32Idx++]);
                break;
            case InterfaceTypeKind::U32:
                result.value = flat.i32s[i32Idx++];
                break;
            case InterfaceTypeKind::S32:
                result.value = static_cast<int32_t>(flat.i32s[i32Idx++]);
                break;
            case InterfaceTypeKind::U64:
                result.value = flat.i64s[i64Idx++];
                break;
            case InterfaceTypeKind::F32:
                result.value = flat.f32s[f32Idx++];
                break;
            case InterfaceTypeKind::F64:
                result.value = flat.f64s[f64Idx++];
                break;
            case InterfaceTypeKind::String: {
                uint32_t ptr = flat.i32s[i32Idx++];
                uint32_t len = flat.i32s[i32Idx++];
                result.value = abi_->liftString(ptr, len);
                break;
            }
            default:
                break;
        }
        
        return result;
    }
    
    CanonicalABI* abi_;
};

// =============================================================================
// Interface Definition
// =============================================================================

/**
 * @brief A function in an interface
 */
struct InterfaceFunction {
    std::string name;
    std::vector<std::pair<std::string, std::shared_ptr<InterfaceType>>> params;
    std::vector<std::shared_ptr<InterfaceType>> results;
};

/**
 * @brief An interface definition (like wasi:filesystem/types)
 */
struct InterfaceDefinition {
    std::string package;  // e.g., "wasi:filesystem"
    std::string name;     // e.g., "types"
    
    std::vector<std::pair<std::string, std::shared_ptr<InterfaceType>>> types;
    std::vector<InterfaceFunction> functions;
    std::vector<std::string> resources;
};

/**
 * @brief A complete world definition
 */
struct WorldDefinition {
    std::string name;
    
    // Imported interfaces
    std::vector<std::pair<std::string, InterfaceDefinition>> imports;
    
    // Exported interfaces
    std::vector<std::pair<std::string, InterfaceDefinition>> exports;
};

// =============================================================================
// Component
// =============================================================================

/**
 * @brief A instantiated component
 */
class Component {
public:
    std::string name;
    WorldDefinition world;
    
    // Adapted imports (from host or other components)
    std::unordered_map<std::string, void*> adaptedImports;
    
    // Adapted exports (to host or other components)
    std::unordered_map<std::string, void*> adaptedExports;
    
    // Call an exported function
    InterfaceValue call(const std::string& funcName, 
                       const std::vector<InterfaceValue>& args) {
        // Would lookup export and call with adapted values
        (void)funcName; (void)args;
        return {};
    }
};

} // namespace Zepra::Wasm
