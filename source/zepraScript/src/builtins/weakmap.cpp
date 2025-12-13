/**
 * @file weakmap.cpp
 * @brief JavaScript WeakMap builtin implementation
 */

#include "zeprascript/builtins/weakmap.hpp"
#include "zeprascript/runtime/function.hpp"

namespace Zepra::Builtins {

// =============================================================================
// WeakMapObject Implementation
// =============================================================================

void WeakMapObject::set(Object* key, const Value& value) {
    if (key) {
        entries_[key] = value;
    }
}

Value WeakMapObject::get(Object* key) const {
    if (!key) return Value::undefined();
    
    auto it = entries_.find(key);
    if (it != entries_.end()) {
        return it->second;
    }
    return Value::undefined();
}

bool WeakMapObject::has(Object* key) const {
    if (!key) return false;
    return entries_.find(key) != entries_.end();
}

bool WeakMapObject::deleteKey(Object* key) {
    if (!key) return false;
    return entries_.erase(key) > 0;
}

// =============================================================================
// WeakMapBuiltin Implementation
// =============================================================================

Value WeakMapBuiltin::constructor(Runtime::Context*, const std::vector<Value>&) {
    return Value::object(new WeakMapObject());
}

Value WeakMapBuiltin::get(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject()) return Value::undefined();
    
    WeakMapObject* wm = dynamic_cast<WeakMapObject*>(args[0].asObject());
    if (!wm) return Value::undefined();
    
    if (!args[1].isObject()) return Value::undefined();
    return wm->get(args[1].asObject());
}

Value WeakMapBuiltin::set(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isObject()) return Value::undefined();
    
    WeakMapObject* wm = dynamic_cast<WeakMapObject*>(args[0].asObject());
    if (!wm) return Value::undefined();
    
    if (!args[1].isObject()) return Value::undefined();
    wm->set(args[1].asObject(), args[2]);
    return args[0]; // Return WeakMap for chaining
}

Value WeakMapBuiltin::has(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject()) return Value::boolean(false);
    
    WeakMapObject* wm = dynamic_cast<WeakMapObject*>(args[0].asObject());
    if (!wm) return Value::boolean(false);
    
    if (!args[1].isObject()) return Value::boolean(false);
    return Value::boolean(wm->has(args[1].asObject()));
}

Value WeakMapBuiltin::deleteMethod(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject()) return Value::boolean(false);
    
    WeakMapObject* wm = dynamic_cast<WeakMapObject*>(args[0].asObject());
    if (!wm) return Value::boolean(false);
    
    if (!args[1].isObject()) return Value::boolean(false);
    return Value::boolean(wm->deleteKey(args[1].asObject()));
}

Object* WeakMapBuiltin::createWeakMapPrototype() {
    return new Object();
}

} // namespace Zepra::Builtins
