#pragma once

/**
 * @file symbol.hpp
 * @brief JavaScript Symbol builtin
 */

#include "../config.hpp"
#include "../runtime/object.hpp"
#include "../runtime/value.hpp"
#include <string>
#include <unordered_map>
#include <atomic>

namespace Zepra::Runtime { class Context; }

namespace Zepra::Builtins {

using Runtime::Value;
using Runtime::Object;
using Runtime::ObjectType;

/**
 * @brief JavaScript Symbol primitive
 * 
 * Symbols are unique identifiers used as property keys.
 */
class SymbolObject : public Object {
public:
    explicit SymbolObject(const std::string& description = "");
    
    // Symbol API
    uint64_t id() const { return id_; }
    const std::string& description() const { return description_; }
    
    // Comparison (symbols are unique)
    bool equals(const SymbolObject* other) const {
        return other && id_ == other->id_;
    }
    
    std::string toString() const;
    
private:
    uint64_t id_;
    std::string description_;
    
    static std::atomic<uint64_t> nextId_;
};

/**
 * @brief Symbol builtin functions
 */
class SymbolBuiltin {
public:
    // Symbol() constructor (creates unique symbol)
    static Value constructor(Runtime::Context* ctx, const std::vector<Value>& args);
    
    // Symbol.for(key) - global symbol registry
    static Value forKey(Runtime::Context* ctx, const std::vector<Value>& args);
    
    // Symbol.keyFor(sym) - get key from global registry
    static Value keyFor(Runtime::Context* ctx, const std::vector<Value>& args);
    
    // Well-known symbols
    static SymbolObject* iterator();
    static SymbolObject* toStringTag();
    static SymbolObject* hasInstance();
    static SymbolObject* isConcatSpreadable();
    
    static Object* createSymbolPrototype();
    
private:
    // Global symbol registry
    static std::unordered_map<std::string, SymbolObject*>& registry();
};

} // namespace Zepra::Builtins
