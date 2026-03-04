/**
 * @file ServiceWorker.h
 * @brief Service Worker API skeleton
 * 
 * Basic Service Worker registration and lifecycle.
 * Full implementation in future phases.
 * 
 * Reference: https://www.w3.org/TR/service-workers/
 */

#pragma once

#include "../config.hpp"
#include "runtime/objects/value.hpp"
#include "runtime/objects/object.hpp"
#include "runtime/async/promise.hpp"
#include <string>
#include <functional>
#include <vector>

namespace Zepra::Browser {

using Runtime::Value;
using Runtime::Object;
using Runtime::Promise;

// Forward declarations
class ServiceWorker;
class ServiceWorkerRegistration;
class ServiceWorkerContainer;

// =============================================================================
// ServiceWorker
// =============================================================================

/**
 * @brief Represents a service worker
 */
class ServiceWorker : public Object {
public:
    enum class State {
        Parsed,
        Installing,
        Installed,
        Activating,
        Activated,
        Redundant
    };
    
    ServiceWorker(const std::string& scriptURL);
    
    const std::string& scriptURL() const { return scriptURL_; }
    State state() const { return state_; }
    
    /**
     * @brief Post message to service worker
     */
    void postMessage(const Value& message, const std::vector<Object*>& transfer = {});
    
    // Event handler
    std::function<void(ServiceWorker*)> onstatechange;
    std::function<void(const Value&)> onerror;
    
private:
    friend class ServiceWorkerRegistration;
    friend class ServiceWorkerContainer;
    friend class ServiceWorkerGlobalScope;
    void setState(State state);
    
    std::string scriptURL_;
    State state_ = State::Parsed;
};

// =============================================================================
// ServiceWorkerRegistration
// =============================================================================

/**
 * @brief Represents a service worker registration
 */
class ServiceWorkerRegistration : public Object {
public:
    ServiceWorkerRegistration(const std::string& scope);
    
    ServiceWorker* active() const { return active_; }
    ServiceWorker* waiting() const { return waiting_; }
    ServiceWorker* installing() const { return installing_; }
    const std::string& scope() const { return scope_; }
    
    /**
     * @brief Update the service worker
     */
    Promise* update();
    
    /**
     * @brief Unregister this registration
     */
    Promise* unregister();
    
    // Event handlers
    std::function<void(ServiceWorkerRegistration*)> onupdatefound;
    
private:
    friend class ServiceWorkerContainer;
    friend class ServiceWorkerGlobalScope;
    
    std::string scope_;
    ServiceWorker* active_ = nullptr;
    ServiceWorker* waiting_ = nullptr;
    ServiceWorker* installing_ = nullptr;
};

// =============================================================================
// ServiceWorkerContainer
// =============================================================================

/**
 * @brief navigator.serviceWorker interface
 */
class ServiceWorkerContainer : public Object {
public:
    ServiceWorkerContainer();
    
    /**
     * @brief Get the active service worker for this page
     */
    ServiceWorker* controller() const { return controller_; }
    
    /**
     * @brief Get ready promise (resolves when active worker available)
     */
    Promise* ready();
    
    /**
     * @brief Register a service worker
     * @param scriptURL URL of the service worker script
     * @param scope Optional scope (defaults to script directory)
     */
    Promise* registerWorker(const std::string& scriptURL, 
                            const std::string& scope = "");
    
    /**
     * @brief Get registration for a scope
     */
    Promise* getRegistration(const std::string& scope = "");
    
    /**
     * @brief Get all registrations
     */
    Promise* getRegistrations();
    
    /**
     * @brief Start messages (if they were deferred)
     */
    void startMessages();
    
    // Event handlers
    std::function<void(ServiceWorker*)> oncontrollerchange;
    std::function<void(const Value&)> onmessage;
    std::function<void(const Value&)> onmessageerror;
    
    // Singleton access
    static ServiceWorkerContainer* instance();
    
private:
    ServiceWorker* controller_ = nullptr;
    std::vector<ServiceWorkerRegistration*> registrations_;
    Promise* readyPromise_ = nullptr;
    bool messagesStarted_ = false;
};

// =============================================================================
// ServiceWorkerGlobalScope
// =============================================================================

/**
 * @brief Global scope inside a service worker
 */
class ServiceWorkerGlobalScope : public Object {
public:
    ServiceWorkerGlobalScope();
    
    /**
     * @brief Skip waiting and activate immediately
     */
    Promise* skipWaiting();
    
    /**
     * @brief Claim all clients
     */
    Promise* clients();
    
    ServiceWorkerRegistration* registration() const { return registration_; }
    ServiceWorker* serviceWorker() const { return serviceWorker_; }
    
    // Event handlers for service worker lifecycle
    std::function<void()> oninstall;
    std::function<void()> onactivate;
    std::function<void(const Value&)> onfetch;
    std::function<void(const Value&)> onmessage;
    std::function<void()> onpush;
    std::function<void()> onsync;
    
private:
    ServiceWorkerRegistration* registration_ = nullptr;
    ServiceWorker* serviceWorker_ = nullptr;
};

// =============================================================================
// Builtin Functions
// =============================================================================

Value serviceWorkerBuiltin(void* ctx, const std::vector<Value>& args);
void initServiceWorker();

} // namespace Zepra::Browser
