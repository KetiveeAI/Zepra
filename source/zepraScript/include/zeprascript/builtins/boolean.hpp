#pragma once

/**
 * @file boolean.hpp
 * @brief JavaScript Boolean object
 */

#include "../config.hpp"
#include "../runtime/object.hpp"
#include "../runtime/value.hpp"

namespace Zepra::Runtime { class Context; }

namespace Zepra::Builtins {

using Runtime::Value;
using Runtime::Object;

/**
 * @brief JavaScript Boolean object wrapper
 */
class BooleanObject : public Object {
public:
    explicit BooleanObject(bool value);
    
    bool valueOf() const { return value_; }
    std::string toString() const { return value_ ? "true" : "false"; }
    
private:
    bool value_;
};

/**
 * @brief Boolean builtin methods
 */
class BooleanBuiltin {
public:
    static Value constructor(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value valueOf(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value toString(Runtime::Context* ctx, const std::vector<Value>& args);
    
    static Object* createBooleanPrototype();
};

} // namespace Zepra::Builtins
