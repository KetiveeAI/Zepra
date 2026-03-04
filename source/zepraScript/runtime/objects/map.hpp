#pragma once

/**
 * @file map.hpp
 * @brief ES6 Map implementation
 */

#include "config.hpp"
#include "value.hpp"
#include "object.hpp"
#include <vector>
#include <utility>

namespace Zepra::Runtime {

/**
 * @brief ES6 Map
 * Map object holds key-value pairs, keys can be any value
 */
class Map : public Object {
public:
    Map();
    
    // Core methods
    void set(const Value& key, const Value& value);
    Value get(const Value& key) const;
    bool has(const Value& key) const;
    bool deleteKey(const Value& key);
    void clear();
    
    // Size
    size_t size() const { return entries_.size(); }
    
    // Iteration
    std::vector<Value> keys() const;
    std::vector<Value> values() const;
    std::vector<std::pair<Value, Value>> entries() const;
    
    void forEach(std::function<void(const Value&, const Value&)> callback) const;
    
private:
    struct Entry {
        Value key;
        Value value;
    };
    
    std::vector<Entry> entries_;
    
    size_t findIndex(const Value& key) const;
};

} // namespace Zepra::Runtime
