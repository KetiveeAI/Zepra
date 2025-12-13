/**
 * @file session_storage.cpp
 * @brief SessionStorage implementation
 */

#include "storage/session_storage.hpp"

#include <algorithm>
#include <memory>
#include <unordered_map>

namespace Zepra::Storage {

// =============================================================================
// SessionStorage Implementation
// =============================================================================

SessionStorage::SessionStorage(const std::string& origin, const std::string& tabId)
    : origin_(origin), tabId_(tabId) {}

SessionStorage::~SessionStorage() = default;

std::optional<std::string> SessionStorage::getItem(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void SessionStorage::setItem(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string oldValue;
    auto it = data_.find(key);
    if (it != data_.end()) {
        oldValue = it->second;
        currentSize_ -= it->first.size() + it->second.size();
    }
    
    size_t newEntrySize = key.size() + value.size();
    if (currentSize_ + newEntrySize > quota_) {
        throw std::runtime_error("QuotaExceededError: SessionStorage quota exceeded");
    }
    
    data_[key] = value;
    currentSize_ += newEntrySize;
    
    // Notify change
    if (onChange_) {
        StorageEvent event;
        event.key = key;
        event.oldValue = oldValue;
        event.newValue = value;
        event.url = origin_;
        event.storageArea = "sessionStorage";
        onChange_(event);
    }
}

void SessionStorage::removeItem(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = data_.find(key);
    if (it != data_.end()) {
        currentSize_ -= it->first.size() + it->second.size();
        data_.erase(it);
    }
}

void SessionStorage::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    data_.clear();
    currentSize_ = 0;
}

std::optional<std::string> SessionStorage::key(size_t index) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (index >= data_.size()) {
        return std::nullopt;
    }
    
    auto it = data_.begin();
    std::advance(it, index);
    return it->first;
}

size_t SessionStorage::length() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.size();
}

size_t SessionStorage::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentSize_;
}

// =============================================================================
// Global Functions
// =============================================================================

static std::mutex sessionStoragesMutex;
static std::unordered_map<std::string, std::unique_ptr<SessionStorage>> sessionStorages;

static std::string makeKey(const std::string& origin, const std::string& tabId) {
    return origin + "|" + tabId;
}

SessionStorage& getSessionStorage(const std::string& origin, const std::string& tabId) {
    std::lock_guard<std::mutex> lock(sessionStoragesMutex);
    
    std::string key = makeKey(origin, tabId);
    auto it = sessionStorages.find(key);
    if (it != sessionStorages.end()) {
        return *it->second;
    }
    
    auto storage = std::make_unique<SessionStorage>(origin, tabId);
    SessionStorage& ref = *storage;
    sessionStorages[key] = std::move(storage);
    return ref;
}

void destroySessionStorage(const std::string& tabId) {
    std::lock_guard<std::mutex> lock(sessionStoragesMutex);
    
    for (auto it = sessionStorages.begin(); it != sessionStorages.end();) {
        if (it->second->tabId() == tabId) {
            it = sessionStorages.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace Zepra::Storage
