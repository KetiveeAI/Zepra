/**
 * @file set.cpp
 * @brief JavaScript Set builtin implementation
 */

#include "builtins/set.hpp"
#include "runtime/objects/function.hpp"

namespace Zepra::Builtins {

// =============================================================================
// SetObject Implementation
// =============================================================================

int SetObject::findValue(const Value& value) const {
    for (size_t i = 0; i < values_.size(); ++i) {
        if (values_[i].equals(value)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void SetObject::add(const Value& value) {
    if (findValue(value) < 0) {
        values_.push_back(value);
    }
}

bool SetObject::has(const Value& value) const {
    return findValue(value) >= 0;
}

bool SetObject::deleteValue(const Value& value) {
    int idx = findValue(value);
    if (idx >= 0) {
        values_.erase(values_.begin() + idx);
        return true;
    }
    return false;
}

void SetObject::clear() {
    values_.clear();
}

// =============================================================================
// SetBuiltin Implementation
// =============================================================================

Value SetBuiltin::constructor(Runtime::Context*, const std::vector<Value>&) {
    return Value::object(new SetObject());
}

Value SetBuiltin::add(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject()) return Value::undefined();
    
    SetObject* set = dynamic_cast<SetObject*>(args[0].asObject());
    if (!set) return Value::undefined();
    
    set->add(args[1]);
    return args[0]; // Return set for chaining
}

Value SetBuiltin::has(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject()) return Value::boolean(false);
    
    SetObject* set = dynamic_cast<SetObject*>(args[0].asObject());
    if (!set) return Value::boolean(false);
    
    return Value::boolean(set->has(args[1]));
}

Value SetBuiltin::deleteMethod(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject()) return Value::boolean(false);
    
    SetObject* set = dynamic_cast<SetObject*>(args[0].asObject());
    if (!set) return Value::boolean(false);
    
    return Value::boolean(set->deleteValue(args[1]));
}

Value SetBuiltin::clear(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::undefined();
    
    SetObject* set = dynamic_cast<SetObject*>(args[0].asObject());
    if (set) set->clear();
    
    return Value::undefined();
}

Value SetBuiltin::size(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) return Value::number(0);
    
    SetObject* set = dynamic_cast<SetObject*>(args[0].asObject());
    if (!set) return Value::number(0);
    
    return Value::number(static_cast<double>(set->size()));
}

Object* SetBuiltin::createSetPrototype() {
    Object* proto = new Object();

    proto->set("add", Value::object(
        new Runtime::Function("add", [](const Runtime::FunctionCallInfo& info) -> Value {
            if (!info.thisValue().isObject()) return Value::undefined();
            SetObject* s = dynamic_cast<SetObject*>(info.thisValue().asObject());
            if (!s || info.argumentCount() < 1) return info.thisValue();
            s->add(info.argument(0));
            return info.thisValue();
        }, 1)));

    proto->set("has", Value::object(
        new Runtime::Function("has", [](const Runtime::FunctionCallInfo& info) -> Value {
            if (!info.thisValue().isObject()) return Value::boolean(false);
            SetObject* s = dynamic_cast<SetObject*>(info.thisValue().asObject());
            if (!s || info.argumentCount() < 1) return Value::boolean(false);
            return Value::boolean(s->has(info.argument(0)));
        }, 1)));

    proto->set("delete", Value::object(
        new Runtime::Function("delete", [](const Runtime::FunctionCallInfo& info) -> Value {
            if (!info.thisValue().isObject()) return Value::boolean(false);
            SetObject* s = dynamic_cast<SetObject*>(info.thisValue().asObject());
            if (!s || info.argumentCount() < 1) return Value::boolean(false);
            return Value::boolean(s->deleteValue(info.argument(0)));
        }, 1)));

    proto->set("clear", Value::object(
        new Runtime::Function("clear", [](const Runtime::FunctionCallInfo& info) -> Value {
            if (!info.thisValue().isObject()) return Value::undefined();
            SetObject* s = dynamic_cast<SetObject*>(info.thisValue().asObject());
            if (s) s->clear();
            return Value::undefined();
        }, 0)));

    proto->set("forEach", Value::object(
        new Runtime::Function("forEach", [](const Runtime::FunctionCallInfo& info) -> Value {
            if (!info.thisValue().isObject() || info.argumentCount() < 1) return Value::undefined();
            SetObject* s = dynamic_cast<SetObject*>(info.thisValue().asObject());
            if (!s || !info.argument(0).isObject()) return Value::undefined();
            Runtime::Function* callback = dynamic_cast<Runtime::Function*>(info.argument(0).asObject());
            if (!callback) return Value::undefined();

            Value thisArg = info.argumentCount() > 1 ? info.argument(1) : Value::undefined();
            for (const auto& val : s->values()) {
                std::vector<Value> args = {val, val, info.thisValue()};
                callback->call(nullptr, thisArg, args);
            }
            return Value::undefined();
        }, 1)));

    return proto;
}

} // namespace Zepra::Builtins
