#pragma once

/**
 * @file string.hpp
 * @brief String builtin methods
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
 * @brief String built-in implementation
 */
class StringBuiltin {
public:
    // Static methods
    static Runtime::Value fromCharCode(const Runtime::FunctionCallInfo& info);
    static Runtime::Value fromCodePoint(const Runtime::FunctionCallInfo& info);
    
    // Prototype methods
    static Runtime::Value charAt(const Runtime::FunctionCallInfo& info);
    static Runtime::Value charCodeAt(const Runtime::FunctionCallInfo& info);
    static Runtime::Value codePointAt(const Runtime::FunctionCallInfo& info);
    static Runtime::Value concat(const Runtime::FunctionCallInfo& info);
    static Runtime::Value includes(const Runtime::FunctionCallInfo& info);
    static Runtime::Value indexOf(const Runtime::FunctionCallInfo& info);
    static Runtime::Value lastIndexOf(const Runtime::FunctionCallInfo& info);
    static Runtime::Value slice(const Runtime::FunctionCallInfo& info);
    static Runtime::Value substring(const Runtime::FunctionCallInfo& info);
    static Runtime::Value substr(const Runtime::FunctionCallInfo& info);
    static Runtime::Value toLowerCase(const Runtime::FunctionCallInfo& info);
    static Runtime::Value toUpperCase(const Runtime::FunctionCallInfo& info);
    static Runtime::Value trim(const Runtime::FunctionCallInfo& info);
    static Runtime::Value trimStart(const Runtime::FunctionCallInfo& info);
    static Runtime::Value trimEnd(const Runtime::FunctionCallInfo& info);
    static Runtime::Value split(const Runtime::FunctionCallInfo& info);
    static Runtime::Value repeat(const Runtime::FunctionCallInfo& info);
    static Runtime::Value startsWith(const Runtime::FunctionCallInfo& info);
    static Runtime::Value endsWith(const Runtime::FunctionCallInfo& info);
    static Runtime::Value padStart(const Runtime::FunctionCallInfo& info);
    static Runtime::Value padEnd(const Runtime::FunctionCallInfo& info);
    static Runtime::Value replace(const Runtime::FunctionCallInfo& info);
    static Runtime::Value replaceAll(const Runtime::FunctionCallInfo& info);
    static Runtime::Value match(const Runtime::FunctionCallInfo& info);
    static Runtime::Value search(const Runtime::FunctionCallInfo& info);
    static Runtime::Value localeCompare(const Runtime::FunctionCallInfo& info);
    static Runtime::Value normalize(const Runtime::FunctionCallInfo& info);
    static Runtime::Value at(const Runtime::FunctionCallInfo& info);
    
    // Property accessors
    static Runtime::Value getLength(const Runtime::FunctionCallInfo& info);
    
    // Create prototype
    static Runtime::Object* createStringPrototype(Runtime::Context* ctx);
};

} // namespace Zepra::Builtins
