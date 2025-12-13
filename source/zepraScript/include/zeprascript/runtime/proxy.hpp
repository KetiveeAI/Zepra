#pragma once

/**
 * @file proxy.hpp
 * @brief JavaScript Proxy and Reflect implementation
 */

#include "../config.hpp"
#include "../runtime/object.hpp"
#include "../runtime/value.hpp"
#include <functional>

namespace Zepra::Runtime {

class Context;  // Forward declaration

/**
 * @brief Proxy handler traps
 */
struct ProxyHandler {
    // Object operations
    std::function<Value(Object*, const std::string&, Object*)> get;
    std::function<bool(Object*, const std::string&, Value, Object*)> set;
    std::function<bool(Object*, const std::string&)> has;
    std::function<bool(Object*, const std::string&)> deleteProperty;
    std::function<std::vector<std::string>(Object*)> ownKeys;
    
    // Function operations  
    std::function<Value(Object*, Value, std::vector<Value>&)> apply;
    std::function<Value(Object*, std::vector<Value>&)> construct;
    
    // Property descriptor operations
    std::function<Value(Object*, const std::string&)> getOwnPropertyDescriptor;
    std::function<bool(Object*, const std::string&, Value)> defineProperty;
    
    // Prototype operations
    std::function<Object*(Object*)> getPrototypeOf;
    std::function<bool(Object*, Object*)> setPrototypeOf;
    
    // Extensibility
    std::function<bool(Object*)> isExtensible;
    std::function<bool(Object*)> preventExtensions;
};

/**
 * @brief JavaScript Proxy object
 */
class Proxy : public Object {
public:
    Proxy(Object* target, ProxyHandler handler);
    
    Object* target() const { return target_; }
    const ProxyHandler& handler() const { return handler_; }
    
    bool isRevoked() const { return revoked_; }
    void revoke() { revoked_ = true; }
    
    // Overridden Object methods that trigger traps
    Value get(const std::string& key) const override;
    bool set(const std::string& key, Value value) override;
    bool hasProperty(const std::string& key) const;
    bool deleteProperty(const std::string& key);
    
    // Static factory
    static Proxy* create(Object* target, Object* handler);
    static std::pair<Proxy*, std::function<void()>> createRevocable(Object* target, Object* handler);
    
private:
    Object* target_;
    ProxyHandler handler_;
    bool revoked_ = false;
};

/**
 * @brief JavaScript Reflect object methods
 */
class Reflect {
public:
    // Object operations
    static Value get(Object* target, const std::string& key, Object* receiver = nullptr);
    static bool set(Object* target, const std::string& key, Value value, Object* receiver = nullptr);
    static bool has(Object* target, const std::string& key);
    static bool deleteProperty(Object* target, const std::string& key);
    static std::vector<std::string> ownKeys(Object* target);
    
    // Function operations
    static Value apply(Object* target, Value thisArg, const std::vector<Value>& args);
    static Value construct(Object* target, const std::vector<Value>& args);
    
    // Property operations
    static Value getOwnPropertyDescriptor(Object* target, const std::string& key);
    static bool defineProperty(Object* target, const std::string& key, Object* descriptor);
    
    // Prototype operations
    static Object* getPrototypeOf(Object* target);
    static bool setPrototypeOf(Object* target, Object* proto);
    
    // Extensibility
    static bool isExtensible(Object* target);
    static bool preventExtensions(Object* target);
};

/**
 * @brief Reflect builtin functions
 */
class ReflectBuiltin {
public:
    static Value get(Context* ctx, const std::vector<Value>& args);
    static Value set(Context* ctx, const std::vector<Value>& args);
    static Value has(Context* ctx, const std::vector<Value>& args);
    static Value deleteProperty(Context* ctx, const std::vector<Value>& args);
    static Value ownKeys(Context* ctx, const std::vector<Value>& args);
    static Value apply(Context* ctx, const std::vector<Value>& args);
    static Value construct(Context* ctx, const std::vector<Value>& args);
};

} // namespace Zepra::Runtime
