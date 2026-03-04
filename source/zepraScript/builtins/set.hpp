#pragma once

/**
 * @file set.hpp
 * @brief JavaScript Set builtin
 */

#include "../config.hpp"
#include "runtime/objects/object.hpp"
#include "runtime/objects/value.hpp"
#include <vector>

namespace Zepra::Runtime { class Context; }

namespace Zepra::Builtins {

using Runtime::Value;
using Runtime::Object;
using Runtime::ObjectType;

/**
 * @brief JavaScript Set object
 */
class SetObject : public Object {
public:
    SetObject() : Object(ObjectType::Set) {}
    
    // Set API
    void add(const Value& value);
    bool has(const Value& value) const;
    bool deleteValue(const Value& value);
    void clear();
    size_t size() const { return values_.size(); }
    
    // Iteration
    std::vector<Value> values() const { return values_; }
    
    size_t length() const override { return values_.size(); }
    
private:
    std::vector<Value> values_;
    
    int findValue(const Value& value) const;
};

/**
 * @brief Set builtin functions
 */
class SetBuiltin {
public:
    static Value constructor(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value add(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value has(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value deleteMethod(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value clear(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value size(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value values(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value forEach(Runtime::Context* ctx, const std::vector<Value>& args);
    
    static Object* createSetPrototype();
};

} // namespace Zepra::Builtins
