#pragma once

/**
 * @file array.hpp
 * @brief Array builtin methods
 */

#include "../config.hpp"
#include "runtime/objects/value.hpp"

namespace Zepra::Runtime {
class Context;
class Object;
class FunctionCallInfo;
}

namespace Zepra::Builtins {

/**
 * @brief Array built-in implementation
 */
class ArrayBuiltin {
public:
    // Static methods
    static Runtime::Value isArray(const Runtime::FunctionCallInfo& info);
    static Runtime::Value from(const Runtime::FunctionCallInfo& info);
    static Runtime::Value of(const Runtime::FunctionCallInfo& info);
    
    // Prototype methods
    static Runtime::Value push(const Runtime::FunctionCallInfo& info);
    static Runtime::Value pop(const Runtime::FunctionCallInfo& info);
    static Runtime::Value shift(const Runtime::FunctionCallInfo& info);
    static Runtime::Value unshift(const Runtime::FunctionCallInfo& info);
    static Runtime::Value slice(const Runtime::FunctionCallInfo& info);
    static Runtime::Value splice(const Runtime::FunctionCallInfo& info);
    static Runtime::Value concat(const Runtime::FunctionCallInfo& info);
    static Runtime::Value indexOf(const Runtime::FunctionCallInfo& info);
    static Runtime::Value lastIndexOf(const Runtime::FunctionCallInfo& info);
    static Runtime::Value includes(const Runtime::FunctionCallInfo& info);
    static Runtime::Value join(const Runtime::FunctionCallInfo& info);
    static Runtime::Value reverse(const Runtime::FunctionCallInfo& info);
    static Runtime::Value sort(const Runtime::FunctionCallInfo& info);
    static Runtime::Value fill(const Runtime::FunctionCallInfo& info);
    static Runtime::Value copyWithin(const Runtime::FunctionCallInfo& info);
    
    // Iteration methods
    static Runtime::Value forEach(const Runtime::FunctionCallInfo& info);
    static Runtime::Value map(const Runtime::FunctionCallInfo& info);
    static Runtime::Value filter(const Runtime::FunctionCallInfo& info);
    static Runtime::Value reduce(const Runtime::FunctionCallInfo& info);
    static Runtime::Value reduceRight(const Runtime::FunctionCallInfo& info);
    static Runtime::Value find(const Runtime::FunctionCallInfo& info);
    static Runtime::Value findIndex(const Runtime::FunctionCallInfo& info);
    static Runtime::Value findLast(const Runtime::FunctionCallInfo& info);
    static Runtime::Value findLastIndex(const Runtime::FunctionCallInfo& info);
    static Runtime::Value every(const Runtime::FunctionCallInfo& info);
    static Runtime::Value some(const Runtime::FunctionCallInfo& info);
    static Runtime::Value flat(const Runtime::FunctionCallInfo& info);
    static Runtime::Value flatMap(const Runtime::FunctionCallInfo& info);
    
    // ES2022+ methods
    static Runtime::Value at(const Runtime::FunctionCallInfo& info);
    
    // ES2023+ methods (non-mutating)
    static Runtime::Value toSorted(const Runtime::FunctionCallInfo& info);
    static Runtime::Value toReversed(const Runtime::FunctionCallInfo& info);
    static Runtime::Value toSpliced(const Runtime::FunctionCallInfo& info);
    static Runtime::Value with(const Runtime::FunctionCallInfo& info);
    
    // Property accessors
    static Runtime::Value getLength(const Runtime::FunctionCallInfo& info);
    
    // Create prototype
    static Runtime::Object* createArrayPrototype(Runtime::Context* ctx);
};

} // namespace Zepra::Builtins
