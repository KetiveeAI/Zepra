/**
 * @file symbol.cpp
 * @brief JavaScript Symbol builtin implementation
 */

#include "builtins/symbol.hpp"
#include "runtime/objects/function.hpp"

namespace Zepra::Builtins {

// =============================================================================
// SymbolObject Implementation
// =============================================================================

std::atomic<uint64_t> SymbolObject::nextId_{1};

SymbolObject::SymbolObject(const std::string& description)
    : Object(ObjectType::Arguments)  // Using Arguments as Symbol type placeholder
    , id_(nextId_.fetch_add(1))
    , description_(description) {}

std::string SymbolObject::toString() const {
    if (description_.empty()) {
        return "Symbol()";
    }
    return "Symbol(" + description_ + ")";
}

// =============================================================================
// SymbolBuiltin Implementation
// =============================================================================

std::unordered_map<std::string, SymbolObject*>& SymbolBuiltin::registry() {
    static std::unordered_map<std::string, SymbolObject*> reg;
    return reg;
}

Value SymbolBuiltin::constructor(Runtime::Context*, const std::vector<Value>& args) {
    std::string description = "";
    
    if (!args.empty() && args[0].isString()) {
        description = static_cast<Runtime::String*>(args[0].asObject())->value();
    }
    
    return Value::object(new SymbolObject(description));
}

Value SymbolBuiltin::forKey(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) {
        return Value::undefined();
    }
    
    std::string key = static_cast<Runtime::String*>(args[0].asObject())->value();
    
    auto& reg = registry();
    auto it = reg.find(key);
    if (it != reg.end()) {
        return Value::object(it->second);
    }
    
    // Create new symbol and register it
    SymbolObject* sym = new SymbolObject(key);
    reg[key] = sym;
    return Value::object(sym);
}

Value SymbolBuiltin::keyFor(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) {
        return Value::undefined();
    }
    
    SymbolObject* sym = dynamic_cast<SymbolObject*>(args[0].asObject());
    if (!sym) return Value::undefined();
    
    auto& reg = registry();
    for (const auto& [key, val] : reg) {
        if (val->id() == sym->id()) {
            return Value::string(new Runtime::String(key));
        }
    }
    
    return Value::undefined();
}

// Well-known symbols (singletons)
SymbolObject* SymbolBuiltin::iterator() {
    static SymbolObject* sym = new SymbolObject("Symbol.iterator");
    return sym;
}

SymbolObject* SymbolBuiltin::toStringTag() {
    static SymbolObject* sym = new SymbolObject("Symbol.toStringTag");
    return sym;
}

SymbolObject* SymbolBuiltin::hasInstance() {
    static SymbolObject* sym = new SymbolObject("Symbol.hasInstance");
    return sym;
}

SymbolObject* SymbolBuiltin::isConcatSpreadable() {
    static SymbolObject* sym = new SymbolObject("Symbol.isConcatSpreadable");
    return sym;
}

Object* SymbolBuiltin::createSymbolPrototype() {
    Object* proto = new Object();
    return proto;
}

} // namespace Zepra::Builtins
