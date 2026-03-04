/**
 * @file StorageAPI.h
 * @brief Web Storage API Implementation
 * 
 * HTML Standard Storage:
 * - localStorage: Persistent storage
 * - sessionStorage: Session-only storage
 * - StorageEvent: Change notifications
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <mutex>
#include <optional>

namespace Zepra::API {

// =============================================================================
// Storage Event
// =============================================================================

/**
 * @brief Event fired when storage changes
 */
struct StorageEvent {
    std::string key;              // Changed key (empty if clear())
    std::optional<std::string> oldValue;
    std::optional<std::string> newValue;
    std::string url;              // Origin URL
    std::string storageArea;      // "localStorage" or "sessionStorage"
};

using StorageEventCallback = std::function<void(const StorageEvent&)>;

// =============================================================================
// Storage
// =============================================================================

/**
 * @brief Web Storage interface (localStorage/sessionStorage)
 */
class Storage {
public:
    explicit Storage(const std::string& origin, const std::string& type)
        : origin_(origin), type_(type) {}
    
    // Number of items
    size_t length() const {
        std::lock_guard lock(mutex_);
        return data_.size();
    }
    
    // Get key at index
    std::optional<std::string> key(size_t index) const {
        std::lock_guard lock(mutex_);
        if (index >= keys_.size()) return std::nullopt;
        return keys_[index];
    }
    
    // Get item
    std::optional<std::string> getItem(const std::string& key) const {
        std::lock_guard lock(mutex_);
        auto it = data_.find(key);
        return it != data_.end() ? std::optional(it->second) : std::nullopt;
    }
    
    // Set item
    void setItem(const std::string& key, const std::string& value) {
        std::optional<std::string> oldValue;
        
        {
            std::lock_guard lock(mutex_);
            
            auto it = data_.find(key);
            if (it != data_.end()) {
                oldValue = it->second;
                it->second = value;
            } else {
                data_[key] = value;
                keys_.push_back(key);
            }
        }
        
        // Fire event
        fireEvent(key, oldValue, value);
    }
    
    // Remove item
    void removeItem(const std::string& key) {
        std::optional<std::string> oldValue;
        
        {
            std::lock_guard lock(mutex_);
            
            auto it = data_.find(key);
            if (it != data_.end()) {
                oldValue = it->second;
                data_.erase(it);
                keys_.erase(std::remove(keys_.begin(), keys_.end(), key), keys_.end());
            }
        }
        
        if (oldValue) {
            fireEvent(key, oldValue, std::nullopt);
        }
    }
    
    // Clear all
    void clear() {
        {
            std::lock_guard lock(mutex_);
            data_.clear();
            keys_.clear();
        }
        
        StorageEvent event;
        event.storageArea = type_;
        event.url = origin_;
        notifyListeners(event);
    }
    
    // Event listeners
    void addEventListener(StorageEventCallback callback) {
        std::lock_guard lock(listenerMutex_);
        listeners_.push_back(std::move(callback));
    }
    
private:
    void fireEvent(const std::string& key,
                   std::optional<std::string> oldValue,
                   std::optional<std::string> newValue) {
        StorageEvent event;
        event.key = key;
        event.oldValue = oldValue;
        event.newValue = newValue;
        event.storageArea = type_;
        event.url = origin_;
        
        notifyListeners(event);
    }
    
    void notifyListeners(const StorageEvent& event) {
        std::vector<StorageEventCallback> callbacks;
        {
            std::lock_guard lock(listenerMutex_);
            callbacks = listeners_;
        }
        
        for (const auto& cb : callbacks) {
            cb(event);
        }
    }
    
    std::string origin_;
    std::string type_;  // "localStorage" or "sessionStorage"
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> data_;
    std::vector<std::string> keys_;  // Maintain insertion order
    
    std::mutex listenerMutex_;
    std::vector<StorageEventCallback> listeners_;
};

// =============================================================================
// Storage Manager
// =============================================================================

/**
 * @brief Manages storage per origin
 */
class StorageManager {
public:
    static StorageManager& instance() {
        static StorageManager mgr;
        return mgr;
    }
    
    // Get localStorage for origin
    Storage& localStorage(const std::string& origin) {
        std::lock_guard lock(mutex_);
        
        auto key = origin + ":local";
        auto it = storages_.find(key);
        if (it != storages_.end()) {
            return *it->second;
        }
        
        auto storage = std::make_unique<Storage>(origin, "localStorage");
        Storage* ptr = storage.get();
        storages_[key] = std::move(storage);
        return *ptr;
    }
    
    // Get sessionStorage for origin
    Storage& sessionStorage(const std::string& origin) {
        std::lock_guard lock(mutex_);
        
        auto key = origin + ":session";
        auto it = storages_.find(key);
        if (it != storages_.end()) {
            return *it->second;
        }
        
        auto storage = std::make_unique<Storage>(origin, "sessionStorage");
        Storage* ptr = storage.get();
        storages_[key] = std::move(storage);
        return *ptr;
    }
    
    // Clear session storage (on session end)
    void clearSessionStorage() {
        std::lock_guard lock(mutex_);
        
        std::vector<std::string> toRemove;
        for (const auto& [key, _] : storages_) {
            if (key.find(":session") != std::string::npos) {
                toRemove.push_back(key);
            }
        }
        
        for (const auto& key : toRemove) {
            storages_.erase(key);
        }
    }
    
    // Persistence (for localStorage)
    void save(const std::string& path);
    void load(const std::string& path);
    
private:
    StorageManager() = default;
    
    std::mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<Storage>> storages_;
};

// =============================================================================
// Global Access
// =============================================================================

inline Storage& localStorage() {
    return StorageManager::instance().localStorage("default");
}

inline Storage& sessionStorage() {
    return StorageManager::instance().sessionStorage("default");
}

} // namespace Zepra::API
