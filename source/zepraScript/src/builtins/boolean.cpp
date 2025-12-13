/**
 * @file boolean.cpp
 * @brief JavaScript Boolean object implementation
 */

#include "zeprascript/builtins/boolean.hpp"
#include "zeprascript/runtime/function.hpp"

namespace Zepra::Builtins {

BooleanObject::BooleanObject(bool value)
    : Object(Runtime::ObjectType::Ordinary)
    , value_(value) {}

Value BooleanBuiltin::constructor(Runtime::Context*, const std::vector<Value>& args) {
    bool value = false;
    if (!args.empty()) {
        const Value& arg = args[0];
        if (arg.isBoolean()) {
            value = arg.asBoolean();
        } else if (arg.isNumber()) {
            value = arg.asNumber() != 0 && !std::isnan(arg.asNumber());
        } else if (arg.isString()) {
            value = static_cast<Runtime::String*>(arg.asObject())->length() > 0;
        } else if (arg.isObject()) {
            value = true;
        }
    }
    return Value::object(new BooleanObject(value));
}

Value BooleanBuiltin::valueOf(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::boolean(false);
    BooleanObject* b = dynamic_cast<BooleanObject*>(args[0].asObject());
    return b ? Value::boolean(b->valueOf()) : Value::boolean(false);
}

Value BooleanBuiltin::toString(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::string(new Runtime::String("false"));
    BooleanObject* b = dynamic_cast<BooleanObject*>(args[0].asObject());
    return b ? Value::string(new Runtime::String(b->toString())) 
             : Value::string(new Runtime::String("false"));
}

Object* BooleanBuiltin::createBooleanPrototype() {
    return new Object();
}

} // namespace Zepra::Builtins
