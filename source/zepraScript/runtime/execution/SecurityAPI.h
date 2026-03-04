/**
 * @file SecurityAPI.h
 * @brief Security and Sandbox Implementation
 */

#pragma once

#include <string>
#include <set>
#include <map>
#include <functional>
#include <memory>
#include <stdexcept>

namespace Zepra::Runtime {

// =============================================================================
// Capability
// =============================================================================

enum class Capability {
    FileRead,
    FileWrite,
    NetworkAccess,
    ProcessSpawn,
    EnvAccess,
    EvalExecution,
    DOMAccess,
    StorageAccess,
    TimerAccess,
    CryptoAccess
};

// =============================================================================
// Permission Policy
// =============================================================================

class PermissionPolicy {
public:
    void grant(Capability cap) { granted_.insert(cap); }
    void revoke(Capability cap) { granted_.erase(cap); }
    bool has(Capability cap) const { return granted_.count(cap) > 0; }
    
    void grantAll() {
        grant(Capability::FileRead);
        grant(Capability::FileWrite);
        grant(Capability::NetworkAccess);
        grant(Capability::ProcessSpawn);
        grant(Capability::EnvAccess);
        grant(Capability::EvalExecution);
        grant(Capability::DOMAccess);
        grant(Capability::StorageAccess);
        grant(Capability::TimerAccess);
        grant(Capability::CryptoAccess);
    }
    
    void revokeAll() { granted_.clear(); }
    
    static PermissionPolicy restrictive() {
        PermissionPolicy policy;
        policy.grant(Capability::TimerAccess);
        return policy;
    }
    
    static PermissionPolicy permissive() {
        PermissionPolicy policy;
        policy.grantAll();
        return policy;
    }

private:
    std::set<Capability> granted_;
};

// =============================================================================
// Security Context
// =============================================================================

class SecurityContext {
public:
    explicit SecurityContext(PermissionPolicy policy = {}) : policy_(std::move(policy)) {}
    
    void require(Capability cap) const {
        if (!policy_.has(cap)) {
            throw std::runtime_error("Permission denied: " + capabilityName(cap));
        }
    }
    
    bool check(Capability cap) const { return policy_.has(cap); }
    
    PermissionPolicy& policy() { return policy_; }
    const PermissionPolicy& policy() const { return policy_; }

private:
    static std::string capabilityName(Capability cap) {
        switch (cap) {
            case Capability::FileRead: return "FileRead";
            case Capability::FileWrite: return "FileWrite";
            case Capability::NetworkAccess: return "NetworkAccess";
            case Capability::ProcessSpawn: return "ProcessSpawn";
            case Capability::EnvAccess: return "EnvAccess";
            case Capability::EvalExecution: return "EvalExecution";
            case Capability::DOMAccess: return "DOMAccess";
            case Capability::StorageAccess: return "StorageAccess";
            case Capability::TimerAccess: return "TimerAccess";
            case Capability::CryptoAccess: return "CryptoAccess";
            default: return "Unknown";
        }
    }
    
    PermissionPolicy policy_;
};

// =============================================================================
// Sandbox
// =============================================================================

class Sandbox {
public:
    explicit Sandbox(PermissionPolicy policy = PermissionPolicy::restrictive())
        : context_(std::move(policy)) {}
    
    template<typename F>
    auto execute(F&& fn) -> decltype(fn(context_)) {
        return fn(context_);
    }
    
    SecurityContext& context() { return context_; }

private:
    SecurityContext context_;
};

// =============================================================================
// Object Security
// =============================================================================

class ObjectSecurity {
public:
    template<typename T>
    static void freeze(T& obj) {
        frozen_.insert(&obj);
    }
    
    template<typename T>
    static bool isFrozen(const T& obj) {
        return frozen_.count(&obj) > 0;
    }
    
    template<typename T>
    static void seal(T& obj) {
        sealed_.insert(&obj);
    }
    
    template<typename T>
    static bool isSealed(const T& obj) {
        return sealed_.count(&obj) > 0;
    }
    
    template<typename T>
    static void preventExtensions(T& obj) {
        nonExtensible_.insert(&obj);
    }
    
    template<typename T>
    static bool isExtensible(const T& obj) {
        return nonExtensible_.count(&obj) == 0;
    }

private:
    static inline std::set<const void*> frozen_;
    static inline std::set<const void*> sealed_;
    static inline std::set<const void*> nonExtensible_;
};

} // namespace Zepra::Runtime
