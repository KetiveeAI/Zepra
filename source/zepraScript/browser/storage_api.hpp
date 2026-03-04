#pragma once

/**
 * @file storage_api.hpp
 * @brief JavaScript Storage API (localStorage, sessionStorage)
 */

#include "../config.hpp"
#include "runtime/objects/object.hpp"
#include "runtime/objects/value.hpp"
#include <string>
#include <unordered_map>

namespace Zepra::Runtime { class Context; }

namespace Zepra::Browser {

using Runtime::Value;
using Runtime::Object;

/**
 * @brief Storage interface (localStorage, sessionStorage)
 */
class Storage : public Object {
public:
    Storage() : Object(Runtime::ObjectType::Ordinary) {}
    
    /**
     * @brief Get number of stored items
     */
    size_t length() const override { return data_.size(); }
    
    /**
     * @brief Get key at index
     */
    std::string key(size_t index) const;
    
    /**
     * @brief Get item by key
     */
    std::string getItem(const std::string& key) const;
    
    /**
     * @brief Set item
     */
    void setItem(const std::string& key, const std::string& value);
    
    /**
     * @brief Remove item
     */
    void removeItem(const std::string& key);
    
    /**
     * @brief Clear all items
     */
    void clear();
    
private:
    std::unordered_map<std::string, std::string> data_;
    std::vector<std::string> keyOrder_; // For key() method
};

/**
 * @brief Storage builtin functions
 */
class StorageBuiltin {
public:
    static Value getItem(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value setItem(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value removeItem(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value clear(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value key(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value length(Runtime::Context* ctx, const std::vector<Value>& args);
};

/**
 * @brief Global localStorage instance
 */
Storage& localStorage();

/**
 * @brief Global sessionStorage instance
 */
Storage& sessionStorage();

} // namespace Zepra::Browser
