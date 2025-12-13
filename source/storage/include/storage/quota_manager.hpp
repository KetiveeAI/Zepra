/**
 * @file quota_manager.hpp
 * @brief Storage quota management per origin
 */

#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>

namespace Zepra::Storage {

/**
 * @brief Storage usage by type
 */
struct StorageUsage {
    size_t localStorage = 0;
    size_t sessionStorage = 0;
    size_t indexedDB = 0;
    size_t cacheStorage = 0;
    size_t serviceWorkers = 0;
    
    size_t total() const {
        return localStorage + sessionStorage + indexedDB + cacheStorage + serviceWorkers;
    }
};

/**
 * @brief Quota estimate
 */
struct QuotaEstimate {
    size_t usage = 0;       // Current usage
    size_t quota = 0;       // Total available
    size_t available() const { return quota > usage ? quota - usage : 0; }
};

/**
 * @brief Quota Manager
 * 
 * Manages storage quotas per origin:
 * - Default 50MB per origin
 * - Persistent storage can request more
 * - Tracks usage across all storage types
 */
class QuotaManager {
public:
    QuotaManager();
    ~QuotaManager();
    
    /**
     * @brief Get quota estimate for origin
     */
    QuotaEstimate estimate(const std::string& origin);
    
    /**
     * @brief Get detailed usage for origin
     */
    StorageUsage getUsage(const std::string& origin);
    
    /**
     * @brief Check if origin can store bytes
     */
    bool canStore(const std::string& origin, size_t bytes);
    
    /**
     * @brief Request persistent storage
     */
    using PersistCallback = std::function<void(bool)>;
    void requestPersist(const std::string& origin, PersistCallback callback);
    
    /**
     * @brief Check if origin has persistent storage
     */
    bool isPersisted(const std::string& origin);
    
    /**
     * @brief Update usage for origin
     */
    void updateUsage(const std::string& origin, const StorageUsage& usage);
    
    /**
     * @brief Set quota for origin
     */
    void setQuota(const std::string& origin, size_t bytes);
    
    /**
     * @brief Get default quota
     */
    size_t defaultQuota() const { return defaultQuota_; }
    void setDefaultQuota(size_t bytes) { defaultQuota_ = bytes; }
    
    /**
     * @brief Clear usage for origin
     */
    void clearOrigin(const std::string& origin);
    
    /**
     * @brief Evict data to free space
     */
    size_t evict(const std::string& origin, size_t bytesNeeded);
    
private:
    struct OriginData {
        StorageUsage usage;
        size_t quota = 0;
        bool persistent = false;
    };
    
    OriginData& getOrCreate(const std::string& origin);
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, OriginData> origins_;
    size_t defaultQuota_ = 50 * 1024 * 1024;  // 50MB
};

/**
 * @brief Global quota manager
 */
QuotaManager& getQuotaManager();

} // namespace Zepra::Storage
