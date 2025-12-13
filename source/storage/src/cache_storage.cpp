/**
 * @file cache_storage.cpp
 * @brief Cache Storage implementation for service workers
 */

#include "storage/cache_storage.hpp"

#include <algorithm>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace Zepra::Storage {

// =============================================================================
// Cache Implementation
// =============================================================================

Cache::Cache(const std::string& name) : name_(name) {}
Cache::~Cache() = default;

std::string Cache::makeKey(const CachedRequest& request,
                            const CacheMatchOptions& options) const {
    std::string key = request.url;
    
    if (!options.ignoreMethod) {
        key = request.method + ":" + key;
    }
    
    if (options.ignoreSearch) {
        size_t queryPos = key.find('?');
        if (queryPos != std::string::npos) {
            key = key.substr(0, queryPos);
        }
    }
    
    return key;
}

std::optional<CachedResponse> Cache::match(const CachedRequest& request,
                                            const CacheMatchOptions& options) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = makeKey(request, options);
    auto it = entries_.find(key);
    
    if (it != entries_.end()) {
        return it->second.second;
    }
    
    return std::nullopt;
}

std::vector<CachedResponse> Cache::matchAll(const CachedRequest& request,
                                             const CacheMatchOptions& options) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<CachedResponse> results;
    std::string key = makeKey(request, options);
    
    for (const auto& [k, v] : entries_) {
        if (k.find(key) != std::string::npos) {
            results.push_back(v.second);
        }
    }
    
    return results;
}

void Cache::put(const CachedRequest& request, const CachedResponse& response) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = makeKey(request, {});
    entries_[key] = {request, response};
}

bool Cache::add(const std::string& url) {
    // Would need to actually fetch the URL
    // For now, just create an entry
    CachedRequest req;
    req.url = url;
    req.method = "GET";
    
    CachedResponse resp;
    resp.status = 200;
    resp.url = url;
    
    put(req, resp);
    return true;
}

bool Cache::addAll(const std::vector<std::string>& urls) {
    for (const auto& url : urls) {
        if (!add(url)) return false;
    }
    return true;
}

bool Cache::deleteEntry(const CachedRequest& request,
                         const CacheMatchOptions& options) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = makeKey(request, options);
    return entries_.erase(key) > 0;
}

std::vector<CachedRequest> Cache::keys(const CacheMatchOptions& options) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<CachedRequest> result;
    result.reserve(entries_.size());
    
    for (const auto& [key, value] : entries_) {
        result.push_back(value.first);
    }
    
    return result;
}

size_t Cache::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t total = 0;
    for (const auto& [key, value] : entries_) {
        total += value.first.url.size() + value.first.body.size();
        total += value.second.body.size();
    }
    return total;
}

// =============================================================================
// CacheStorage Implementation
// =============================================================================

CacheStorage::CacheStorage(const std::string& origin) : origin_(origin) {}
CacheStorage::~CacheStorage() = default;

std::shared_ptr<Cache> CacheStorage::open(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = caches_.find(name);
    if (it != caches_.end()) {
        return it->second;
    }
    
    auto cache = std::make_shared<Cache>(name);
    caches_[name] = cache;
    return cache;
}

bool CacheStorage::has(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return caches_.find(name) != caches_.end();
}

bool CacheStorage::deleteCache(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return caches_.erase(name) > 0;
}

std::vector<std::string> CacheStorage::keys() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    result.reserve(caches_.size());
    
    for (const auto& [name, cache] : caches_) {
        result.push_back(name);
    }
    
    return result;
}

std::optional<CachedResponse> CacheStorage::match(const CachedRequest& request,
                                                    const CacheMatchOptions& options) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& [name, cache] : caches_) {
        auto result = cache->match(request, options);
        if (result) {
            return result;
        }
    }
    
    return std::nullopt;
}

size_t CacheStorage::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t total = 0;
    for (const auto& [name, cache] : caches_) {
        total += cache->size();
    }
    return total;
}

bool CacheStorage::load(const std::string& path) {
    // TODO: Implement serialization
    return false;
}

bool CacheStorage::save(const std::string& path) {
    // TODO: Implement serialization
    return false;
}

// =============================================================================
// Global Function
// =============================================================================

static std::mutex cacheStoragesMutex;
static std::unordered_map<std::string, std::unique_ptr<CacheStorage>> cacheStorages;

CacheStorage& getCacheStorage(const std::string& origin) {
    std::lock_guard<std::mutex> lock(cacheStoragesMutex);
    
    auto it = cacheStorages.find(origin);
    if (it != cacheStorages.end()) {
        return *it->second;
    }
    
    auto storage = std::make_unique<CacheStorage>(origin);
    CacheStorage& ref = *storage;
    cacheStorages[origin] = std::move(storage);
    return ref;
}

} // namespace Zepra::Storage
