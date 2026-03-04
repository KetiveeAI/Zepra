/**
 * @file WeakRefAPI.h
 * @brief WeakRef and FinalizationRegistry Implementation
 */

#pragma once

#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <optional>

namespace Zepra::Runtime {

// =============================================================================
// WeakRef
// =============================================================================

template<typename T>
class WeakRef {
public:
    WeakRef() = default;
    explicit WeakRef(std::shared_ptr<T> target) : target_(target) {}
    
    std::shared_ptr<T> deref() const {
        return target_.lock();
    }
    
    bool expired() const {
        return target_.expired();
    }

private:
    std::weak_ptr<T> target_;
};

// =============================================================================
// FinalizationRegistry
// =============================================================================

template<typename T>
class FinalizationRegistry {
public:
    using CleanupCallback = std::function<void(T)>;
    using UnregisterToken = void*;
    
    explicit FinalizationRegistry(CleanupCallback callback)
        : callback_(std::move(callback)) {}
    
    void register_(std::shared_ptr<void> target, T heldValue, UnregisterToken token = nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        Registration reg;
        reg.target = target;
        reg.heldValue = std::move(heldValue);
        reg.token = token;
        
        registrations_.push_back(std::move(reg));
    }
    
    bool unregister(UnregisterToken token) {
        if (!token) return false;
        
        std::lock_guard<std::mutex> lock(mutex_);
        bool removed = false;
        
        registrations_.erase(
            std::remove_if(registrations_.begin(), registrations_.end(),
                [token, &removed](const Registration& reg) {
                    if (reg.token == token) {
                        removed = true;
                        return true;
                    }
                    return false;
                }),
            registrations_.end()
        );
        
        return removed;
    }
    
    void cleanupSome() {
        std::vector<T> toCleanup;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            registrations_.erase(
                std::remove_if(registrations_.begin(), registrations_.end(),
                    [&toCleanup](Registration& reg) {
                        if (reg.target.expired()) {
                            toCleanup.push_back(std::move(reg.heldValue));
                            return true;
                        }
                        return false;
                    }),
                registrations_.end()
            );
        }
        
        for (auto& value : toCleanup) {
            callback_(std::move(value));
        }
    }

private:
    struct Registration {
        std::weak_ptr<void> target;
        T heldValue;
        UnregisterToken token = nullptr;
    };
    
    CleanupCallback callback_;
    std::vector<Registration> registrations_;
    std::mutex mutex_;
};

// =============================================================================
// WeakMap
// =============================================================================

template<typename K, typename V>
class WeakMap {
public:
    void set(std::shared_ptr<K> key, V value) {
        std::lock_guard<std::mutex> lock(mutex_);
        cleanup();
        entries_[key.get()] = {key, std::move(value)};
    }
    
    std::optional<V> get(std::shared_ptr<K> key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(key.get());
        if (it != entries_.end() && !it->second.weakKey.expired()) {
            return it->second.value;
        }
        return std::nullopt;
    }
    
    bool has(std::shared_ptr<K> key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(key.get());
        return it != entries_.end() && !it->second.weakKey.expired();
    }
    
    bool delete_(std::shared_ptr<K> key) {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.erase(key.get()) > 0;
    }

private:
    void cleanup() {
        for (auto it = entries_.begin(); it != entries_.end();) {
            if (it->second.weakKey.expired()) {
                it = entries_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    struct Entry {
        std::weak_ptr<K> weakKey;
        V value;
    };
    
    mutable std::mutex mutex_;
    std::unordered_map<K*, Entry> entries_;
};

// =============================================================================
// WeakSet
// =============================================================================

template<typename T>
class WeakSet {
public:
    void add(std::shared_ptr<T> value) {
        std::lock_guard<std::mutex> lock(mutex_);
        cleanup();
        entries_[value.get()] = value;
    }
    
    bool has(std::shared_ptr<T> value) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(value.get());
        return it != entries_.end() && !it->second.expired();
    }
    
    bool delete_(std::shared_ptr<T> value) {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.erase(value.get()) > 0;
    }

private:
    void cleanup() {
        for (auto it = entries_.begin(); it != entries_.end();) {
            if (it->second.expired()) {
                it = entries_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    mutable std::mutex mutex_;
    std::unordered_map<T*, std::weak_ptr<T>> entries_;
};

} // namespace Zepra::Runtime
