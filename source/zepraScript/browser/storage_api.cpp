/**
 * @file storage_api.cpp
 * @brief JavaScript Storage API implementation
 */

#include "browser/storage_api.hpp"
#include "runtime/objects/function.hpp"
#include <algorithm>

namespace Zepra::Browser {

// =============================================================================
// Storage Implementation
// =============================================================================

std::string Storage::key(size_t index) const {
    if (index >= keyOrder_.size()) return "";
    return keyOrder_[index];
}

std::string Storage::getItem(const std::string& key) const {
    auto it = data_.find(key);
    return it != data_.end() ? it->second : "";
}

void Storage::setItem(const std::string& key, const std::string& value) {
    if (data_.find(key) == data_.end()) {
        keyOrder_.push_back(key);
    }
    data_[key] = value;
}

void Storage::removeItem(const std::string& key) {
    data_.erase(key);
    keyOrder_.erase(
        std::remove(keyOrder_.begin(), keyOrder_.end(), key),
        keyOrder_.end());
}

void Storage::clear() {
    data_.clear();
    keyOrder_.clear();
}

// =============================================================================
// Global Storage Instances
// =============================================================================

Storage& localStorage() {
    static Storage instance;
    return instance;
}

Storage& sessionStorage() {
    static Storage instance;
    return instance;
}

// =============================================================================
// StorageBuiltin Implementation
// =============================================================================

Value StorageBuiltin::getItem(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject() || !args[1].isString()) {
        return Value::null();
    }
    
    Storage* storage = dynamic_cast<Storage*>(args[0].asObject());
    if (!storage) return Value::null();
    
    std::string key = static_cast<Runtime::String*>(args[1].asObject())->value();
    std::string value = storage->getItem(key);
    
    if (value.empty()) return Value::null();
    return Value::string(new Runtime::String(value));
}

Value StorageBuiltin::setItem(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isObject() || 
        !args[1].isString() || !args[2].isString()) {
        return Value::undefined();
    }
    
    Storage* storage = dynamic_cast<Storage*>(args[0].asObject());
    if (!storage) return Value::undefined();
    
    std::string key = static_cast<Runtime::String*>(args[1].asObject())->value();
    std::string value = static_cast<Runtime::String*>(args[2].asObject())->value();
    storage->setItem(key, value);
    
    return Value::undefined();
}

Value StorageBuiltin::removeItem(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject() || !args[1].isString()) {
        return Value::undefined();
    }
    
    Storage* storage = dynamic_cast<Storage*>(args[0].asObject());
    if (storage) {
        std::string key = static_cast<Runtime::String*>(args[1].asObject())->value();
        storage->removeItem(key);
    }
    
    return Value::undefined();
}

Value StorageBuiltin::clear(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) {
        return Value::undefined();
    }
    
    Storage* storage = dynamic_cast<Storage*>(args[0].asObject());
    if (storage) storage->clear();
    
    return Value::undefined();
}

Value StorageBuiltin::key(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject() || !args[1].isNumber()) {
        return Value::null();
    }
    
    Storage* storage = dynamic_cast<Storage*>(args[0].asObject());
    if (!storage) return Value::null();
    
    size_t index = static_cast<size_t>(args[1].asNumber());
    std::string k = storage->key(index);
    
    if (k.empty()) return Value::null();
    return Value::string(new Runtime::String(k));
}

Value StorageBuiltin::length(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) {
        return Value::number(0);
    }
    
    Storage* storage = dynamic_cast<Storage*>(args[0].asObject());
    if (!storage) return Value::number(0);
    
    return Value::number(static_cast<double>(storage->length()));
}

} // namespace Zepra::Browser
