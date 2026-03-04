/**
 * @file IsolatedGlobal.h
 * @brief Secure Global Object Isolation for ZepraScript
 * 
 * Provides sandboxed global objects that restrict access to dangerous APIs
 * based on security policy.
 */

#pragma once

#include "Sandbox.h"
#include "runtime/objects/value.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace Zepra::Runtime {

// Forward declarations
class Object;
class Function;
class Context;

// =============================================================================
// Isolated Global Object
// =============================================================================

/**
 * @brief A sandboxed global object that enforces security policy
 */
class IsolatedGlobal {
public:
    explicit IsolatedGlobal(const SecurityPolicy& policy)
        : policy_(policy) {
        initializeSafeBuiltins();
    }
    
    /**
     * @brief Check if a global name is accessible
     */
    bool isAllowed(const std::string& name) const {
        // Explicit deny takes precedence
        if (policy_.deniedGlobals.count(name)) {
            return false;
        }
        
        // If allowlist is not empty, check allowlist
        if (!policy_.allowedGlobals.empty()) {
            return policy_.allowedGlobals.count(name) > 0;
        }
        
        // Check dangerous APIs
        if (dangerousGlobals_.count(name)) {
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief Get a global value with security check
     */
    Value get(const std::string& name) const {
        if (!isAllowed(name)) {
            return Value::undefined();
        }
        
        auto it = values_.find(name);
        return it != values_.end() ? it->second : Value::undefined();
    }
    
    /**
     * @brief Set a global value with security check
     */
    bool set(const std::string& name, Value value) {
        if (!isAllowed(name)) {
            return false;
        }
        
        // Prevent overwriting frozen globals
        if (frozenGlobals_.count(name)) {
            return false;
        }
        
        values_[name] = value;
        return true;
    }
    
    /**
     * @brief Check if eval is allowed
     */
    bool isEvalAllowed() const {
        return policy_.allowEval;
    }
    
    /**
     * @brief Check if fetch is allowed
     */
    bool isFetchAllowed() const {
        return policy_.allowFetch;
    }
    
    /**
     * @brief Check if Workers are allowed
     */
    bool isWorkersAllowed() const {
        return policy_.allowWorkers;
    }
    
    /**
     * @brief Check if WASM is allowed
     */
    bool isWasmAllowed() const {
        return policy_.allowWasm;
    }
    
    /**
     * @brief Check if network access to origin is allowed
     */
    bool isOriginAllowed(const std::string& origin) const {
        if (!policy_.allowNetworkAccess) {
            return false;
        }
        
        if (policy_.allowedOrigins.empty()) {
            return true;  // All origins allowed
        }
        
        return policy_.allowedOrigins.count(origin) > 0;
    }
    
    /**
     * @brief Freeze a global (make it immutable)
     */
    void freeze(const std::string& name) {
        frozenGlobals_.insert(name);
    }
    
    /**
     * @brief Get the security policy
     */
    const SecurityPolicy& policy() const { return policy_; }
    
private:
    void initializeSafeBuiltins() {
        // Safe builtins always available
        safeBuiltins_ = {
            "undefined", "NaN", "Infinity",
            "Object", "Array", "String", "Number", "Boolean",
            "Symbol", "BigInt", "Date", "RegExp", "Error",
            "TypeError", "RangeError", "SyntaxError", "ReferenceError",
            "JSON", "Math", "console",
            "Promise", "Map", "Set", "WeakMap", "WeakSet",
            "ArrayBuffer", "DataView", "Int8Array", "Uint8Array",
            "Int16Array", "Uint16Array", "Int32Array", "Uint32Array",
            "Float32Array", "Float64Array", "BigInt64Array", "BigUint64Array",
            "Reflect", "Proxy", "Intl"
        };
        
        // Dangerous/restricted globals
        dangerousGlobals_ = {
            "eval", "Function",  // Code execution
            "XMLHttpRequest", "fetch",  // Network (conditionally allowed)
            "WebSocket",  // Network
            "Worker", "SharedWorker", "ServiceWorker",  // Workers
            "Atomics", "SharedArrayBuffer",  // Shared memory
            "require", "import",  // Module loading
            "__proto__", "constructor"  // Prototype pollution
        };
    }
    
    SecurityPolicy policy_;
    std::unordered_map<std::string, Value> values_;
    std::unordered_set<std::string> frozenGlobals_;
    std::unordered_set<std::string> safeBuiltins_;
    std::unordered_set<std::string> dangerousGlobals_;
};

// =============================================================================
// Secure Context
// =============================================================================

/**
 * @brief A secure execution context with sandboxing
 */
class SecureContext {
public:
    SecureContext(const SandboxConfig& config)
        : config_(config)
        , global_(config.policy)
        , monitor_(config.limits) {}
    
    IsolatedGlobal& global() { return global_; }
    const IsolatedGlobal& global() const { return global_; }
    
    ResourceMonitor& monitor() { return monitor_; }
    const ResourceMonitor& monitor() const { return monitor_; }
    
    const SandboxConfig& config() const { return config_; }
    
    /**
     * @brief Report a security violation
     */
    void reportViolation(const std::string& message) {
        if (config_.onSecurityViolation) {
            config_.onSecurityViolation(message);
        }
    }
    
private:
    SandboxConfig config_;
    IsolatedGlobal global_;
    ResourceMonitor monitor_;
};

} // namespace Zepra::Runtime
