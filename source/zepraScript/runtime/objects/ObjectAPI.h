/**
 * @file ObjectAPI.h
 * @brief Object Runtime Implementation
 * 
 * ECMAScript Object based on:
 * - JSC JSObject.h
 * - ECMA-262 19.1 Object Objects
 */

#pragma once

#include <string>
#include <variant>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <functional>

namespace Zepra::Runtime {

// =============================================================================
// Property Attributes
// =============================================================================

struct PropertyAttributes {
    bool writable = true;
    bool enumerable = true;
    bool configurable = true;
    
    static PropertyAttributes defaultData() { return {true, true, true}; }
    static PropertyAttributes frozen() { return {false, true, false}; }
    static PropertyAttributes sealed() { return {true, true, false}; }
};

// =============================================================================
// Property Descriptor
// =============================================================================

class PropertyDescriptor {
public:
    using Value = std::variant<std::monostate, bool, double, std::string, std::shared_ptr<void>>;
    using Getter = std::function<Value()>;
    using Setter = std::function<void(Value)>;
    
    PropertyDescriptor() = default;
    
    static PropertyDescriptor dataDescriptor(Value value, PropertyAttributes attrs = {}) {
        PropertyDescriptor desc;
        desc.value_ = std::move(value);
        desc.attrs_ = attrs;
        desc.isData_ = true;
        return desc;
    }
    
    static PropertyDescriptor accessorDescriptor(Getter get, Setter set, PropertyAttributes attrs = {}) {
        PropertyDescriptor desc;
        desc.getter_ = std::move(get);
        desc.setter_ = std::move(set);
        desc.attrs_ = attrs;
        desc.isData_ = false;
        return desc;
    }
    
    bool isDataDescriptor() const { return isData_ && value_.has_value(); }
    bool isAccessorDescriptor() const { return !isData_ && (getter_ || setter_); }
    bool isEmpty() const { return !value_.has_value() && !getter_ && !setter_; }
    
    const std::optional<Value>& value() const { return value_; }
    void setValue(Value v) { value_ = std::move(v); isData_ = true; }
    
    const Getter& getter() const { return getter_; }
    void setGetter(Getter g) { getter_ = std::move(g); isData_ = false; }
    
    const Setter& setter() const { return setter_; }
    void setSetter(Setter s) { setter_ = std::move(s); isData_ = false; }
    
    const PropertyAttributes& attributes() const { return attrs_; }
    void setAttributes(PropertyAttributes a) { attrs_ = a; }
    
    bool writable() const { return attrs_.writable; }
    bool enumerable() const { return attrs_.enumerable; }
    bool configurable() const { return attrs_.configurable; }

private:
    std::optional<Value> value_;
    Getter getter_;
    Setter setter_;
    PropertyAttributes attrs_;
    bool isData_ = true;
};

// =============================================================================
// Object
// =============================================================================

class Object : public std::enable_shared_from_this<Object> {
public:
    using Value = PropertyDescriptor::Value;
    
    Object() = default;
    explicit Object(std::shared_ptr<Object> prototype) : prototype_(prototype) {}
    
    // Prototype
    std::shared_ptr<Object> getPrototypeOf() const { return prototype_; }
    
    bool setPrototypeOf(std::shared_ptr<Object> proto) {
        if (!extensible_) return false;
        
        auto current = proto;
        while (current) {
            if (current.get() == this) return false;
            current = current->prototype_;
        }
        
        prototype_ = std::move(proto);
        return true;
    }
    
    // Extensibility
    bool isExtensible() const { return extensible_; }
    
    bool preventExtensions() {
        extensible_ = false;
        return true;
    }
    
    // Property access
    bool hasOwnProperty(const std::string& key) const {
        return properties_.find(key) != properties_.end();
    }
    
    bool has(const std::string& key) const {
        if (hasOwnProperty(key)) return true;
        if (prototype_) return prototype_->has(key);
        return false;
    }
    
    std::optional<Value> get(const std::string& key) const {
        auto it = properties_.find(key);
        if (it != properties_.end()) {
            const auto& desc = it->second;
            if (desc.isDataDescriptor()) {
                return desc.value();
            } else if (desc.getter()) {
                return desc.getter()();
            }
        }
        if (prototype_) {
            return prototype_->get(key);
        }
        return std::nullopt;
    }
    
    bool set(const std::string& key, Value value) {
        auto it = properties_.find(key);
        if (it != properties_.end()) {
            auto& desc = it->second;
            if (desc.isDataDescriptor()) {
                if (!desc.writable()) return false;
                desc.setValue(std::move(value));
                return true;
            } else if (desc.setter()) {
                desc.setter()(std::move(value));
                return true;
            }
            return false;
        }
        
        if (!extensible_) return false;
        
        properties_[key] = PropertyDescriptor::dataDescriptor(std::move(value));
        return true;
    }
    
    bool deleteProperty(const std::string& key) {
        auto it = properties_.find(key);
        if (it == properties_.end()) return true;
        if (!it->second.configurable()) return false;
        properties_.erase(it);
        return true;
    }
    
    // Property descriptor access
    std::optional<PropertyDescriptor> getOwnPropertyDescriptor(const std::string& key) const {
        auto it = properties_.find(key);
        if (it != properties_.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    bool defineProperty(const std::string& key, PropertyDescriptor desc) {
        auto it = properties_.find(key);
        if (it == properties_.end()) {
            if (!extensible_) return false;
            properties_[key] = std::move(desc);
            return true;
        }
        
        auto& existing = it->second;
        if (!existing.configurable()) {
            if (desc.configurable()) return false;
            if (desc.enumerable() != existing.enumerable()) return false;
        }
        
        properties_[key] = std::move(desc);
        return true;
    }
    
    // Key enumeration
    std::vector<std::string> ownKeys() const {
        std::vector<std::string> keys;
        for (const auto& [key, _] : properties_) {
            keys.push_back(key);
        }
        return keys;
    }
    
    std::vector<std::string> keys() const {
        std::vector<std::string> result;
        for (const auto& [key, desc] : properties_) {
            if (desc.enumerable()) {
                result.push_back(key);
            }
        }
        return result;
    }
    
    std::vector<Value> values() const {
        std::vector<Value> result;
        for (const auto& [key, desc] : properties_) {
            if (desc.enumerable() && desc.value()) {
                result.push_back(*desc.value());
            }
        }
        return result;
    }
    
    std::vector<std::pair<std::string, Value>> entries() const {
        std::vector<std::pair<std::string, Value>> result;
        for (const auto& [key, desc] : properties_) {
            if (desc.enumerable() && desc.value()) {
                result.emplace_back(key, *desc.value());
            }
        }
        return result;
    }
    
    // Freeze/Seal
    bool freeze() {
        extensible_ = false;
        for (auto& [key, desc] : properties_) {
            auto attrs = desc.attributes();
            attrs.configurable = false;
            if (desc.isDataDescriptor()) {
                attrs.writable = false;
            }
            desc.setAttributes(attrs);
        }
        return true;
    }
    
    bool isFrozen() const {
        if (extensible_) return false;
        for (const auto& [key, desc] : properties_) {
            if (desc.configurable()) return false;
            if (desc.isDataDescriptor() && desc.writable()) return false;
        }
        return true;
    }
    
    bool seal() {
        extensible_ = false;
        for (auto& [key, desc] : properties_) {
            auto attrs = desc.attributes();
            attrs.configurable = false;
            desc.setAttributes(attrs);
        }
        return true;
    }
    
    bool isSealed() const {
        if (extensible_) return false;
        for (const auto& [key, desc] : properties_) {
            if (desc.configurable()) return false;
        }
        return true;
    }
    
    // Assign
    static void assign(std::shared_ptr<Object> target, const std::shared_ptr<Object>& source) {
        if (!source) return;
        for (const auto& key : source->keys()) {
            if (auto value = source->get(key)) {
                target->set(key, *value);
            }
        }
    }
    
    // From entries
    static std::shared_ptr<Object> fromEntries(const std::vector<std::pair<std::string, Value>>& entries) {
        auto obj = std::make_shared<Object>();
        for (const auto& [key, value] : entries) {
            obj->set(key, value);
        }
        return obj;
    }

private:
    std::unordered_map<std::string, PropertyDescriptor> properties_;
    std::shared_ptr<Object> prototype_;
    bool extensible_ = true;
};

// =============================================================================
// Object Factory
// =============================================================================

inline std::shared_ptr<Object> createObject() {
    return std::make_shared<Object>();
}

inline std::shared_ptr<Object> createObject(std::shared_ptr<Object> prototype) {
    return std::make_shared<Object>(std::move(prototype));
}

} // namespace Zepra::Runtime
