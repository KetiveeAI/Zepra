/**
 * @file WeakRefAPI.h
 * @brief WeakRef and FinalizationRegistry Implementation
 * 
 * ECMAScript WeakRefs:
 * - WeakRef: Weak reference to object
 * - FinalizationRegistry: Cleanup callbacks
 */

#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <unordered_map>

namespace Zepra::Runtime {

// =============================================================================
// WeakRef
// =============================================================================

/**
 * @brief Weak reference that doesn't prevent GC
 */
template<typename T>
class WeakRef {
public:
    explicit WeakRef(std::shared_ptr<T> target)
        : target_(target) {}
    
    // Dereference
    std::shared_ptr<T> deref() const {
        return target_.lock();
    }
    
    // Check if still alive
    bool expired() const {
        return target_.expired();
    }
    
private:
    std::weak_ptr<T> target_;
};

// =============================================================================
// Weak Reference Cell (for GC integration)
// =============================================================================

/**
 * @brief Internal weak cell for GC
 */
class WeakCell {
public:
    using Target = std::shared_ptr<void>;
    
    explicit WeakCell(Target target) : target_(target) {}
    
    Target deref() const { return target_.lock(); }
    bool expired() const { return target_.expired(); }
    
    // Holdings kept alive while target is alive
    void setHoldings(std::shared_ptr<void> holdings) {
        holdings_ = holdings;
    }
    
    std::shared_ptr<void> holdings() const { return holdings_; }
    
private:
    std::weak_ptr<void> target_;
    std::shared_ptr<void> holdings_;
};

// =============================================================================
// Cleanup Callback
// =============================================================================

struct FinalizationEntry {
    std::weak_ptr<void> target;
    std::shared_ptr<void> heldValue;
    std::shared_ptr<void> unregisterToken;
};

// =============================================================================
// FinalizationRegistry
// =============================================================================

/**
 * @brief Registry for cleanup callbacks when objects are GC'd
 */
class FinalizationRegistry {
public:
    using CleanupCallback = std::function<void(std::shared_ptr<void>)>;
    
    explicit FinalizationRegistry(CleanupCallback callback)
        : callback_(std::move(callback)) {}
    
    // Register an object for cleanup
    void register_(std::shared_ptr<void> target,
                   std::shared_ptr<void> heldValue,
                   std::shared_ptr<void> unregisterToken = nullptr) {
        std::lock_guard lock(mutex_);
        
        FinalizationEntry entry;
        entry.target = target;
        entry.heldValue = heldValue;
        entry.unregisterToken = unregisterToken;
        
        entries_.push_back(std::move(entry));
    }
    
    // Unregister by token
    bool unregister(std::shared_ptr<void> unregisterToken) {
        if (!unregisterToken) return false;
        
        std::lock_guard lock(mutex_);
        
        bool removed = false;
        entries_.erase(
            std::remove_if(entries_.begin(), entries_.end(),
                [&unregisterToken, &removed](const FinalizationEntry& e) {
                    if (e.unregisterToken == unregisterToken) {
                        removed = true;
                        return true;
                    }
                    return false;
                }),
            entries_.end()
        );
        
        return removed;
    }
    
    // Called by GC to run pending cleanups
    void cleanupSome() {
        std::vector<std::shared_ptr<void>> toCleanup;
        
        {
            std::lock_guard lock(mutex_);
            
            entries_.erase(
                std::remove_if(entries_.begin(), entries_.end(),
                    [&toCleanup](FinalizationEntry& e) {
                        if (e.target.expired()) {
                            toCleanup.push_back(e.heldValue);
                            return true;
                        }
                        return false;
                    }),
                entries_.end()
            );
        }
        
        // Run callbacks outside lock
        for (const auto& held : toCleanup) {
            if (callback_) {
                callback_(held);
            }
        }
    }
    
private:
    CleanupCallback callback_;
    std::vector<FinalizationEntry> entries_;
    std::mutex mutex_;
};

// =============================================================================
// WeakMap
// =============================================================================

/**
 * @brief Map with weak keys
 */
template<typename K, typename V>
class WeakMap {
public:
    void set(std::shared_ptr<K> key, V value) {
        std::lock_guard lock(mutex_);
        entries_[key.get()] = {std::weak_ptr<K>(key), std::move(value)};
    }
    
    std::optional<V> get(std::shared_ptr<K> key) const {
        std::lock_guard lock(mutex_);
        
        auto it = entries_.find(key.get());
        if (it != entries_.end() && !it->second.first.expired()) {
            return it->second.second;
        }
        return std::nullopt;
    }
    
    bool has(std::shared_ptr<K> key) const {
        std::lock_guard lock(mutex_);
        
        auto it = entries_.find(key.get());
        return it != entries_.end() && !it->second.first.expired();
    }
    
    bool delete_(std::shared_ptr<K> key) {
        std::lock_guard lock(mutex_);
        return entries_.erase(key.get()) > 0;
    }
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<K*, std::pair<std::weak_ptr<K>, V>> entries_;
};

// =============================================================================
// WeakSet
// =============================================================================

/**
 * @brief Set with weak members
 */
template<typename T>
class WeakSet {
public:
    void add(std::shared_ptr<T> value) {
        std::lock_guard lock(mutex_);
        entries_[value.get()] = std::weak_ptr<T>(value);
    }
    
    bool has(std::shared_ptr<T> value) const {
        std::lock_guard lock(mutex_);
        
        auto it = entries_.find(value.get());
        return it != entries_.end() && !it->second.expired();
    }
    
    bool delete_(std::shared_ptr<T> value) {
        std::lock_guard lock(mutex_);
        return entries_.erase(value.get()) > 0;
    }
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<T*, std::weak_ptr<T>> entries_;
};

} // namespace Zepra::Runtime
