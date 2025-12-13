/**
 * @file http_cache.hpp
 * @brief HTTP cache for resource caching
 */

#pragma once

#include "networking/http_request.hpp"
#include "networking/http_response.hpp"

#include <string>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <mutex>

namespace Zepra::Networking {

/**
 * @brief Cache entry metadata
 */
struct CacheEntry {
    std::string url;
    std::string etag;
    std::string lastModified;
    std::chrono::system_clock::time_point expires;
    std::chrono::system_clock::time_point date;
    size_t size = 0;
    std::string mimeType;
    
    bool isFresh() const;
    bool isStale() const { return !isFresh(); }
};

/**
 * @brief HTTP Cache
 */
class HttpCache {
public:
    HttpCache();
    explicit HttpCache(const std::string& cacheDir);
    ~HttpCache();
    
    /**
     * @brief Get cached response
     * @return nullptr if not cached or expired
     */
    std::unique_ptr<HttpResponse> get(const HttpRequest& request);
    
    /**
     * @brief Store response in cache
     */
    void put(const HttpRequest& request, const HttpResponse& response);
    
    /**
     * @brief Check if response is cached
     */
    bool has(const HttpRequest& request) const;
    
    /**
     * @brief Get cache entry metadata
     */
    CacheEntry getEntry(const HttpRequest& request) const;
    
    /**
     * @brief Remove from cache
     */
    void remove(const std::string& url);
    
    /**
     * @brief Clear all cache
     */
    void clear();
    
    /**
     * @brief Evict stale entries
     */
    void evictStale();
    
    /**
     * @brief Get cache size
     */
    size_t size() const;
    
    /**
     * @brief Set max cache size in bytes
     */
    void setMaxSize(size_t bytes);
    
private:
    std::string cacheKey(const HttpRequest& request) const;
    bool isCacheable(const HttpRequest& request, const HttpResponse& response) const;
    void evictLRU();
    
    std::string cacheDir_;
    size_t maxSize_ = 100 * 1024 * 1024;  // 100MB default
    size_t currentSize_ = 0;
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, CacheEntry> entries_;
};

/**
 * @brief Global HTTP cache
 */
HttpCache& getHttpCache();

} // namespace Zepra::Networking
