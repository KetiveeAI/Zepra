/**
 * @file quota_manager.cpp
 * @brief Quota manager implementation
 */

#include "storage/quota_manager.hpp"

namespace Zepra::Storage {

QuotaManager::QuotaManager() = default;
QuotaManager::~QuotaManager() = default;

QuotaManager::OriginData& QuotaManager::getOrCreate(const std::string& origin) {
    auto it = origins_.find(origin);
    if (it == origins_.end()) {
        OriginData data;
        data.quota = defaultQuota_;
        origins_[origin] = data;
        return origins_[origin];
    }
    return it->second;
}

QuotaEstimate QuotaManager::estimate(const std::string& origin) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& data = getOrCreate(origin);
    
    QuotaEstimate est;
    est.usage = data.usage.total();
    est.quota = data.quota > 0 ? data.quota : defaultQuota_;
    
    return est;
}

StorageUsage QuotaManager::getUsage(const std::string& origin) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = origins_.find(origin);
    if (it != origins_.end()) {
        return it->second.usage;
    }
    return StorageUsage{};
}

bool QuotaManager::canStore(const std::string& origin, size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& data = getOrCreate(origin);
    size_t quota = data.quota > 0 ? data.quota : defaultQuota_;
    
    return data.usage.total() + bytes <= quota;
}

void QuotaManager::requestPersist(const std::string& origin, 
                                   PersistCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& data = getOrCreate(origin);
    
    // For now, always grant persistence (should show UI prompt)
    data.persistent = true;
    data.quota = 500 * 1024 * 1024;  // 500MB for persistent
    
    if (callback) callback(true);
}

bool QuotaManager::isPersisted(const std::string& origin) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = origins_.find(origin);
    if (it != origins_.end()) {
        return it->second.persistent;
    }
    return false;
}

void QuotaManager::updateUsage(const std::string& origin, 
                                const StorageUsage& usage) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& data = getOrCreate(origin);
    data.usage = usage;
}

void QuotaManager::setQuota(const std::string& origin, size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& data = getOrCreate(origin);
    data.quota = bytes;
}

void QuotaManager::clearOrigin(const std::string& origin) {
    std::lock_guard<std::mutex> lock(mutex_);
    origins_.erase(origin);
}

size_t QuotaManager::evict(const std::string& origin, size_t bytesNeeded) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find and evict non-persistent data
    size_t evicted = 0;
    
    for (auto it = origins_.begin(); it != origins_.end() && evicted < bytesNeeded;) {
        if (!it->second.persistent && it->first != origin) {
            evicted += it->second.usage.total();
            it = origins_.erase(it);
        } else {
            ++it;
        }
    }
    
    return evicted;
}

QuotaManager& getQuotaManager() {
    static QuotaManager instance;
    return instance;
}

} // namespace Zepra::Storage
