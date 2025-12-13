/**
 * @file set.cpp
 * @brief JavaScript Set builtin implementation
 */

#include "zeprascript/builtins/set.hpp"
#include "zeprascript/runtime/function.hpp"

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
    return proto;
}

} // namespace Zepra::Builtins
