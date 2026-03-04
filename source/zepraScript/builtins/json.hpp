#pragma once

/**
 * @file json.hpp
 * @brief JSON builtin methods (parse, stringify)
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
 * @brief JSON built-in implementation
 */
class JSONBuiltin {
public:
    // Static methods
    static Runtime::Value parse(const Runtime::FunctionCallInfo& info);
    static Runtime::Value stringify(const Runtime::FunctionCallInfo& info);
    
    // Create JSON global object
    static Runtime::Object* createJSONObject(Runtime::Context* ctx);
};

} // namespace Zepra::Builtins
