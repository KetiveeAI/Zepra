/**
 * @file ReflectAPI.h
 * @brief Complete Reflect Implementation
 */

#pragma once

#include <any>
#include <vector>
#include <string>
#include <optional>
#include <functional>
#include <memory>

namespace Zepra::Runtime {

class ReflectTarget {
public:
    virtual ~ReflectTarget() = default;
    
    virtual std::any get(const std::string& key) const = 0;
    virtual bool set(const std::string& key, std::any value) = 0;
    virtual bool has(const std::string& key) const = 0;
    virtual bool deleteProperty(const std::string& key) = 0;
    virtual std::vector<std::string> ownKeys() const = 0;
    
    virtual bool isExtensible() const = 0;
    virtual bool preventExtensions() = 0;
    
    virtual std::shared_ptr<ReflectTarget> getPrototypeOf() const = 0;
    virtual bool setPrototypeOf(std::shared_ptr<ReflectTarget>) = 0;
    
    virtual std::optional<std::any> getOwnPropertyDescriptor(const std::string&) const = 0;
    virtual bool defineProperty(const std::string&, std::any) = 0;
};

class Reflect {
public:
    using Args = std::vector<std::any>;
    using Callable = std::function<std::any(std::any, Args)>;
    
    // Function operations
    static std::any apply(Callable target, std::any thisArg, const Args& args) {
        return target(thisArg, args);
    }
    
    static std::any construct(Callable target, const Args& args, std::optional<Callable> newTarget = std::nullopt) {
        return target(std::any{}, args);
    }
    
    // Property operations
    static std::any get(ReflectTarget* target, const std::string& key, std::any receiver = {}) {
        return target->get(key);
    }
    
    static bool set(ReflectTarget* target, const std::string& key, std::any value, std::any receiver = {}) {
        return target->set(key, std::move(value));
    }
    
    static bool has(ReflectTarget* target, const std::string& key) {
        return target->has(key);
    }
    
    static bool deleteProperty(ReflectTarget* target, const std::string& key) {
        return target->deleteProperty(key);
    }
    
    // Key enumeration
    static std::vector<std::string> ownKeys(ReflectTarget* target) {
        return target->ownKeys();
    }
    
    // Property descriptor
    static std::optional<std::any> getOwnPropertyDescriptor(ReflectTarget* target, const std::string& key) {
        return target->getOwnPropertyDescriptor(key);
    }
    
    static bool defineProperty(ReflectTarget* target, const std::string& key, std::any descriptor) {
        return target->defineProperty(key, std::move(descriptor));
    }
    
    // Prototype operations
    static std::shared_ptr<ReflectTarget> getPrototypeOf(ReflectTarget* target) {
        return target->getPrototypeOf();
    }
    
    static bool setPrototypeOf(ReflectTarget* target, std::shared_ptr<ReflectTarget> proto) {
        return target->setPrototypeOf(std::move(proto));
    }
    
    // Extensibility
    static bool isExtensible(ReflectTarget* target) {
        return target->isExtensible();
    }
    
    static bool preventExtensions(ReflectTarget* target) {
        return target->preventExtensions();
    }
};

} // namespace Zepra::Runtime
