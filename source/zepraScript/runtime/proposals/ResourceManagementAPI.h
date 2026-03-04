/**
 * @file ResourceManagementAPI.h
 * @brief Explicit Resource Management
 */

#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <exception>
#include <stdexcept>

namespace Zepra::Runtime {

// =============================================================================
// Resource Interface
// =============================================================================

class Resource {
public:
    virtual ~Resource() = default;
    virtual void dispose() = 0;
};

class AsyncResource {
public:
    virtual ~AsyncResource() = default;
    virtual void disposeAsync(std::function<void()> callback) = 0;
};

// =============================================================================
// RAII Resource Guard
// =============================================================================

template<typename T>
class ResourceGuard {
public:
    explicit ResourceGuard(T resource, std::function<void(T&)> cleanup)
        : resource_(std::move(resource)), cleanup_(std::move(cleanup)) {}
    
    ~ResourceGuard() {
        if (cleanup_ && !released_) {
            cleanup_(resource_);
        }
    }
    
    ResourceGuard(const ResourceGuard&) = delete;
    ResourceGuard& operator=(const ResourceGuard&) = delete;
    
    ResourceGuard(ResourceGuard&& other) noexcept
        : resource_(std::move(other.resource_))
        , cleanup_(std::move(other.cleanup_))
        , released_(other.released_) {
        other.released_ = true;
    }
    
    T& get() { return resource_; }
    const T& get() const { return resource_; }
    
    T* operator->() { return &resource_; }
    const T* operator->() const { return &resource_; }
    
    T release() {
        released_ = true;
        return std::move(resource_);
    }

private:
    T resource_;
    std::function<void(T&)> cleanup_;
    bool released_ = false;
};

template<typename T>
ResourceGuard<T> makeResource(T resource, std::function<void(T&)> cleanup) {
    return ResourceGuard<T>(std::move(resource), std::move(cleanup));
}

// =============================================================================
// Using Statement Simulation
// =============================================================================

template<typename T, typename F>
auto using_(T resource, std::function<void(T&)> cleanup, F&& fn) -> decltype(fn(resource)) {
    ResourceGuard<T> guard(std::move(resource), std::move(cleanup));
    return fn(guard.get());
}

template<typename R, typename F>
auto using_(std::shared_ptr<R> resource, F&& fn) -> decltype(fn(*resource)) 
    requires std::is_base_of_v<Resource, R> {
    
    try {
        auto result = fn(*resource);
        resource->dispose();
        return result;
    } catch (...) {
        resource->dispose();
        throw;
    }
}

// =============================================================================
// Resource Pool
// =============================================================================

template<typename T>
class ResourcePool {
public:
    using Factory = std::function<T()>;
    using Disposer = std::function<void(T&)>;
    
    ResourcePool(size_t maxSize, Factory factory, Disposer disposer)
        : maxSize_(maxSize), factory_(factory), disposer_(disposer) {}
    
    ~ResourcePool() {
        for (auto& resource : pool_) {
            disposer_(resource);
        }
    }
    
    T acquire() {
        if (!pool_.empty()) {
            T resource = std::move(pool_.back());
            pool_.pop_back();
            return resource;
        }
        return factory_();
    }
    
    void release(T resource) {
        if (pool_.size() < maxSize_) {
            pool_.push_back(std::move(resource));
        } else {
            disposer_(resource);
        }
    }
    
    class Handle {
    public:
        Handle(ResourcePool& pool, T resource)
            : pool_(pool), resource_(std::move(resource)) {}
        
        ~Handle() { pool_.release(std::move(resource_)); }
        
        T& get() { return resource_; }
        T* operator->() { return &resource_; }

    private:
        ResourcePool& pool_;
        T resource_;
    };
    
    Handle acquireHandle() {
        return Handle(*this, acquire());
    }

private:
    size_t maxSize_;
    Factory factory_;
    Disposer disposer_;
    std::vector<T> pool_;
};

// =============================================================================
// Defer (Go-style)
// =============================================================================

class DeferStack {
public:
    ~DeferStack() {
        while (!deferred_.empty()) {
            try {
                deferred_.back()();
            } catch (...) {
            }
            deferred_.pop_back();
        }
    }
    
    void defer(std::function<void()> fn) {
        deferred_.push_back(std::move(fn));
    }

private:
    std::vector<std::function<void()>> deferred_;
};

#define DEFER_CONCAT_(a, b) a##b
#define DEFER_CONCAT(a, b) DEFER_CONCAT_(a, b)
#define defer(fn) DeferStack DEFER_CONCAT(_defer_, __LINE__); DEFER_CONCAT(_defer_, __LINE__).defer(fn)

} // namespace Zepra::Runtime
