#pragma once

/**
 * @file set.hpp
 * @brief ES6 Set implementation
 */

#include "config.hpp"
#include "value.hpp"
#include "object.hpp"
#include <vector>
#include <functional>

namespace Zepra::Runtime {

/**
 * @brief ES6 Set
 * Set object stores unique values of any type
 */
class Set : public Object {
public:
    Set();
    
    // Core methods
    void add(const Value& value);
    bool has(const Value& value) const;
    bool deleteValue(const Value& value);
    void clear();
    
    // Size
    size_t size() const { return values_.size(); }
    
    // Iteration
    std::vector<Value> values() const { return values_; }
    void forEach(std::function<void(const Value&)> callback) const;
    
private:
    std::vector<Value> values_;
    size_t findIndex(const Value& value) const;
};

} // namespace Zepra::Runtime
