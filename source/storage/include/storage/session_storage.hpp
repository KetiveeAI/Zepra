/**
 * @file session_storage.hpp
 * @brief Web SessionStorage implementation (per-tab storage)
 */

#pragma once

#include "storage/local_storage.hpp"
#include <string>
#include <unordered_map>
#include <mutex>

namespace Zepra::Storage {

/**
 * @brief SessionStorage - per-tab/window key-value storage
 * 
 * Data cleared when tab/window closes.
 * Each tab has isolated storage even for same origin.
 */
class SessionStorage {
public:
    SessionStorage(const std::string& origin, const std::string& tabId);
    ~SessionStorage();
    
    /**
     * @brief Get item by key
     */
    std::optional<std::string> getItem(const std::string& key) const;
    
    /**
     * @brief Set item
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
     * @brief Get total size
     */
    size_t size() const;
    
    /**
     * @brief Get origin
     */
    const std::string& origin() const { return origin_; }
    
    /**
     * @brief Get tab ID
     */
    const std::string& tabId() const { return tabId_; }
    
    /**
     * @brief Set quota
     */
    void setQuota(size_t bytes) { quota_ = bytes; }
    
    /**
     * @brief Set change handler
     */
    void setOnChange(StorageEventHandler handler) { onChange_ = std::move(handler); }
    
private:
    std::string origin_;
    std::string tabId_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> data_;
    size_t currentSize_ = 0;
    size_t quota_ = 5 * 1024 * 1024;
    StorageEventHandler onChange_;
};

/**
 * @brief Get SessionStorage for tab
 */
SessionStorage& getSessionStorage(const std::string& origin, const std::string& tabId);

/**
 * @brief Destroy SessionStorage for tab (call on tab close)
 */
void destroySessionStorage(const std::string& tabId);

} // namespace Zepra::Storage
