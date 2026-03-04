#pragma once

/**
 * @file object_builtins.hpp
 * @brief Object builtin methods (Object.keys, Object.values, Object.hasOwn, etc.)
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
 * @brief Object built-in implementation
 */
class ObjectBuiltin {
public:
    // Static methods
    static Runtime::Value keys(const Runtime::FunctionCallInfo& info);
    static Runtime::Value values(const Runtime::FunctionCallInfo& info);
    static Runtime::Value entries(const Runtime::FunctionCallInfo& info);
    static Runtime::Value assign(const Runtime::FunctionCallInfo& info);
    static Runtime::Value freeze(const Runtime::FunctionCallInfo& info);
    static Runtime::Value seal(const Runtime::FunctionCallInfo& info);
    static Runtime::Value isFrozen(const Runtime::FunctionCallInfo& info);
    static Runtime::Value isSealed(const Runtime::FunctionCallInfo& info);
    static Runtime::Value create(const Runtime::FunctionCallInfo& info);
    static Runtime::Value getPrototypeOf(const Runtime::FunctionCallInfo& info);
    static Runtime::Value setPrototypeOf(const Runtime::FunctionCallInfo& info);
    static Runtime::Value getOwnPropertyNames(const Runtime::FunctionCallInfo& info);
    static Runtime::Value getOwnPropertyDescriptor(const Runtime::FunctionCallInfo& info);
    static Runtime::Value defineProperty(const Runtime::FunctionCallInfo& info);
    static Runtime::Value defineProperties(const Runtime::FunctionCallInfo& info);
    static Runtime::Value is(const Runtime::FunctionCallInfo& info);
    
    // ES2017+
    static Runtime::Value fromEntries(const Runtime::FunctionCallInfo& info);
    
    // ES2022+
    static Runtime::Value hasOwn(const Runtime::FunctionCallInfo& info);
    
    // ES2024
    static Runtime::Value groupBy(const Runtime::FunctionCallInfo& info);
    
    // Prototype methods
    static Runtime::Value hasOwnProperty(const Runtime::FunctionCallInfo& info);
    static Runtime::Value isPrototypeOf(const Runtime::FunctionCallInfo& info);
    static Runtime::Value propertyIsEnumerable(const Runtime::FunctionCallInfo& info);
    static Runtime::Value toString(const Runtime::FunctionCallInfo& info);
    static Runtime::Value valueOf(const Runtime::FunctionCallInfo& info);
    
    // Create Object constructor and prototype
    static Runtime::Object* createObjectConstructor(Runtime::Context* ctx);
    static Runtime::Object* createObjectPrototype(Runtime::Context* ctx);
};

} // namespace Zepra::Builtins
