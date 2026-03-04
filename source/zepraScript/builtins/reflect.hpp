#pragma once

/**
 * @file reflect.hpp
 * @brief ES6 Reflect API
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
 * @brief ES6 Reflect built-in object
 */
class ReflectBuiltin {
public:
    // Reflect.apply(target, thisArgument, argumentsList)
    static Runtime::Value apply(const Runtime::FunctionCallInfo& info);
    
    // Reflect.construct(target, argumentsList, newTarget?)
    static Runtime::Value construct(const Runtime::FunctionCallInfo& info);
    
    // Reflect.defineProperty(target, propertyKey, attributes)
    static Runtime::Value defineProperty(const Runtime::FunctionCallInfo& info);
    
    // Reflect.deleteProperty(target, propertyKey)
    static Runtime::Value deleteProperty(const Runtime::FunctionCallInfo& info);
    
    // Reflect.get(target, propertyKey, receiver?)
    static Runtime::Value get(const Runtime::FunctionCallInfo& info);
    
    // Reflect.getOwnPropertyDescriptor(target, propertyKey)
    static Runtime::Value getOwnPropertyDescriptor(const Runtime::FunctionCallInfo& info);
    
    // Reflect.getPrototypeOf(target)
    static Runtime::Value getPrototypeOf(const Runtime::FunctionCallInfo& info);
    
    // Reflect.has(target, propertyKey)
    static Runtime::Value has(const Runtime::FunctionCallInfo& info);
    
    // Reflect.isExtensible(target)
    static Runtime::Value isExtensible(const Runtime::FunctionCallInfo& info);
    
    // Reflect.ownKeys(target)
    static Runtime::Value ownKeys(const Runtime::FunctionCallInfo& info);
    
    // Reflect.preventExtensions(target)
    static Runtime::Value preventExtensions(const Runtime::FunctionCallInfo& info);
    
    // Reflect.set(target, propertyKey, value, receiver?)
    static Runtime::Value set(const Runtime::FunctionCallInfo& info);
    
    // Reflect.setPrototypeOf(target, prototype)
    static Runtime::Value setPrototypeOf(const Runtime::FunctionCallInfo& info);
    
    // Create Reflect object
    static Runtime::Object* createReflectObject(Runtime::Context* ctx);
};

} // namespace Zepra::Builtins
