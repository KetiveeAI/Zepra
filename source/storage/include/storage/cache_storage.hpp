/**
 * @file cache_storage.hpp
 * @brief Cache Storage API for service workers
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <optional>

namespace Zepra::Storage {

/**
 * @brief Cached request
 */
struct CachedRequest {
    std::string url;
    std::string method = "GET";
    std::unordered_map<std::string, std::string> headers;
    std::vector<uint8_t> body;
};

/**
 * @brief Cached response
 */
struct CachedResponse {
    int status = 200;
    std::string statusText;
    std::unordered_map<std::string, std::string> headers;
    std::vector<uint8_t> body;
    std::string url;
};

/**
 * @brief Cache match options
 */
struct CacheMatchOptions {
    bool ignoreSearch = false;
    bool ignoreMethod = false;
    bool ignoreVary = false;
};

/**
 * @brief Individual cache
 */
class Cache {
public:
    explicit Cache(const std::string& name);
    ~Cache();
    
    /**
     * @brief Match request
     */
    std::optional<CachedResponse> match(const CachedRequest& request,
                                         const CacheMatchOptions& options = {});
    
    /**
     * @brief Match all
     */
    std::vector<CachedResponse> matchAll(const CachedRequest& request,
                                          const CacheMatchOptions& options = {});
    
    /**
     * @brief Add response for request
     */
    void put(const CachedRequest& request, const CachedResponse& response);
    
    /**
     * @brief Add request (fetches and caches)
     */
    bool add(const std::string& url);
    
    /**
     * @brief Add multiple
     */
    bool addAll(const std::vector<std::string>& urls);
    
    /**
     * @brief Delete entry
     */
    bool deleteEntry(const CachedRequest& request, 
                     const CacheMatchOptions& options = {});
    
    /**
     * @brief Get all keys
     */
    std::vector<CachedRequest> keys(const CacheMatchOptions& options = {});
    
    /**
     * @brief Get name
     */
    const std::string& name() const { return name_; }
    
    /**
     * @brief Get size
     */
    size_t size() const;
    
private:
    std::string makeKey(const CachedRequest& request, 
                        const CacheMatchOptions& options) const;
    
    std::string name_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::pair<CachedRequest, CachedResponse>> entries_;
};

/**
 * @brief Cache Storage
 */
class CacheStorage {
public:
    explicit CacheStorage(const std::string& origin);
    ~CacheStorage();
    
    /**
     * @brief Open cache by name (creates if not exists)
     */
    std::shared_ptr<Cache> open(const std::string& name);
    
    /**
     * @brief Check if cache exists
     */
    bool has(const std::string& name) const;
    
    /**
     * @brief Delete cache
     */
    bool deleteCache(const std::string& name);
    
    /**
     * @brief Get all cache names
     */
    std::vector<std::string> keys() const;
    
    /**
     * @brief Match across all caches
     */
    std::optional<CachedResponse> match(const CachedRequest& request,
                                         const CacheMatchOptions& options = {});
    
    /**
     * @brief Get origin
     */
    const std::string& origin() const { return origin_; }
    
    /**
     * @brief Get total size
     */
    size_t size() const;
    
    /**
     * @brief Load from disk
     */
    bool load(const std::string& path);
    
    /**
     * @brief Save to disk
     */
    bool save(const std::string& path);
    
private:
    std::string origin_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Cache>> caches_;
};

/**
 * @brief Get CacheStorage for origin
 */
CacheStorage& getCacheStorage(const std::string& origin);

} // namespace Zepra::Storage
