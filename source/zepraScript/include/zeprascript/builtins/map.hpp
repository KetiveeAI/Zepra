#pragma once

/**
 * @file map.hpp
 * @brief JavaScript Map builtin
 */

#include "../config.hpp"
#include "../runtime/object.hpp"
#include "../runtime/value.hpp"
#include <unordered_map>
#include <vector>

namespace Zepra::Runtime { class Context; }

namespace Zepra::Builtins {

using Runtime::Value;
using Runtime::Object;
using Runtime::ObjectType;

/**
 * @brief JavaScript Map object
 */
class MapObject : public Object {
public:
    MapObject() : Object(ObjectType::Map) {}
    
    // Map API
    void set(const Value& key, const Value& value);
    Value get(const Value& key) const;
    bool has(const Value& key) const;
    bool deleteKey(const Value& key);
    void clear();
    size_t size() const { return entries_.size(); }
    
    // Iteration
    std::vector<std::pair<Value, Value>> entries() const;
    std::vector<Value> keys() const;
    std::vector<Value> values() const;
    
    size_t length() const override { return entries_.size(); }
    
private:
    // Using vector of pairs for ordered iteration
    std::vector<std::pair<Value, Value>> entries_;
    
    // Helper to find entry by key
    int findKey(const Value& key) const;
};

/**
 * @brief Map builtin functions
 */
class MapBuiltin {
public:
    static Value constructor(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value get(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value set(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value has(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value deleteMethod(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value clear(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value size(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value keys(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value values(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value entries(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value forEach(Runtime::Context* ctx, const std::vector<Value>& args);
    
    static Object* createMapPrototype();
};

} // namespace Zepra::Builtins
