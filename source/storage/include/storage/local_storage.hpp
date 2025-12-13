/**
 * @file local_storage.hpp
 * @brief Web LocalStorage implementation (persistent key-value storage)
 */

#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <optional>

namespace Zepra::Storage {

/**
 * @brief Storage event for change notifications
 */
struct StorageEvent {
    std::string key;
    std::string oldValue;
    std::string newValue;
    std::string url;
    std::string storageArea;  // "localStorage" or "sessionStorage"
};

using StorageEventHandler = std::function<void(const StorageEvent&)>;

/**
 * @brief LocalStorage - persistent key-value storage per origin
 * 
 * Data persists across browser sessions.
 * Quota: 5MB per origin by default.
 */
class LocalStorage {
public:
    explicit LocalStorage(const std::string& origin);
    ~LocalStorage();
    
    /**
     * @brief Get item by key
     * @return value or nullopt if not found
     */
    std::optional<std::string> getItem(const std::string& key) const;
    
    /**
     * @brief Set item
     * @throws QuotaExceededError if over quota
     */
    void setItem(const std::string& key, const std::string& value);
    
    /**
     * @brief Remove item
     */
    void removeItem(const std::string& key);
    
    /**
     * @brief Clear all items
     */
    void clear();
    
    /**
     * @brief Get key at index
     */
    std::optional<std::string> key(size_t index) const;
    
    /**
     * @brief Get number of items
     */
    size_t length() const;
    
    /**
     * @brief Get total size in bytes
     */
    size_t size() const;
    
    /**
     * @brief Get origin
     */
    const std::string& origin() const { return origin_; }
    
    /**
     * @brief Set storage change handler
     */
    void setOnChange(StorageEventHandler handler) { onChange_ = std::move(handler); }
    
    /**
     * @brief Load from disk
     */
    bool load();
    
    /**
     * @brief Save to disk
     */
    bool save();
    
    /**
     * @brief Set storage directory
     */
    static void setStorageDirectory(const std::string& path);
    
    /**
     * @brief Set quota in bytes (default 5MB)
     */
    void setQuota(size_t bytes) { quota_ = bytes; }
    size_t quota() const { return quota_; }
    
private:
    std::string getStoragePath() const;
    void notifyChange(const std::string& key, const std::string& oldVal, 
                      const std::string& newVal);
    
    std::string origin_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> data_;
    size_t currentSize_ = 0;
    size_t quota_ = 5 * 1024 * 1024;  // 5MB
    StorageEventHandler onChange_;
    
    static std::string storageDir_;
};

/**
 * @brief Get LocalStorage for origin
 */
LocalStorage& getLocalStorage(const std::string& origin);

/**
 * @brief Clear all LocalStorage data
 */
void clearAllLocalStorage();

} // namespace Zepra::Storage
