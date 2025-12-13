/**
 * @file http_cache.cpp
 * @brief HTTP cache implementation
 */

#include "networking/http_cache.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

namespace Zepra::Networking {

// =============================================================================
// CacheEntry Implementation
// =============================================================================

bool CacheEntry::isFresh() const {
    auto now = std::chrono::system_clock::now();
    return now < expires;
}

// =============================================================================
// HttpCache Implementation
// =============================================================================

HttpCache::HttpCache() : cacheDir_("/tmp/zepra_cache") {
    fs::create_directories(cacheDir_);
}

HttpCache::HttpCache(const std::string& cacheDir) : cacheDir_(cacheDir) {
    fs::create_directories(cacheDir_);
}

HttpCache::~HttpCache() = default;

std::string HttpCache::cacheKey(const HttpRequest& request) const {
    // Simple hash based on method + URL
    std::hash<std::string> hasher;
    std::string key = request.methodString() + ":" + request.url();
    return std::to_string(hasher(key));
}

bool HttpCache::isCacheable(const HttpRequest& request, 
                             const HttpResponse& response) const {
    // Only cache GET requests
    if (request.method() != HttpMethod::GET) return false;
    
    // Only cache successful responses
    if (!response.isSuccess()) return false;
    
    // Check Cache-Control header
    std::string cacheControl = response.header("Cache-Control");
    std::transform(cacheControl.begin(), cacheControl.end(), 
                   cacheControl.begin(), ::tolower);
    
    if (cacheControl.find("no-store") != std::string::npos) return false;
    if (cacheControl.find("no-cache") != std::string::npos) return false;
    if (cacheControl.find("private") != std::string::npos) return false;
    
    return true;
}

std::unique_ptr<HttpResponse> HttpCache::get(const HttpRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = cacheKey(request);
    auto it = entries_.find(key);
    
    if (it == entries_.end() || it->second.isStale()) {
        return nullptr;
    }
    
    // Read cached body
    std::string bodyPath = cacheDir_ + "/" + key + ".body";
    std::ifstream file(bodyPath, std::ios::binary);
    if (!file.is_open()) {
        return nullptr;
    }
    
    std::vector<uint8_t> body(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    
    auto response = std::make_unique<HttpResponse>();
    response->setStatusCode(200);
    response->setBody(body);
    response->setHeader("Content-Type", it->second.mimeType);
    response->setHeader("X-Cache", "HIT");
    
    return response;
}

void HttpCache::put(const HttpRequest& request, const HttpResponse& response) {
    if (!isCacheable(request, response)) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = cacheKey(request);
    
    // Create cache entry
    CacheEntry entry;
    entry.url = request.url();
    entry.etag = response.header("ETag");
    entry.lastModified = response.header("Last-Modified");
    entry.size = response.body().size();
    entry.mimeType = response.mimeType();
    
    // Calculate expiry
    std::string cacheControl = response.header("Cache-Control");
    int maxAge = 3600;  // Default 1 hour
    
    size_t maxAgePos = cacheControl.find("max-age=");
    if (maxAgePos != std::string::npos) {
        try {
            maxAge = std::stoi(cacheControl.substr(maxAgePos + 8));
        } catch (...) {}
    }
    
    entry.date = std::chrono::system_clock::now();
    entry.expires = entry.date + std::chrono::seconds(maxAge);
    
    // Check size limit
    if (currentSize_ + entry.size > maxSize_) {
        evictLRU();
    }
    
    // Write body to file
    std::string bodyPath = cacheDir_ + "/" + key + ".body";
    std::ofstream file(bodyPath, std::ios::binary);
    if (file.is_open()) {
        const auto& body = response.body();
        file.write(reinterpret_cast<const char*>(body.data()), body.size());
        
        entries_[key] = entry;
        currentSize_ += entry.size;
    }
}

bool HttpCache::has(const HttpRequest& request) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = cacheKey(request);
    auto it = entries_.find(key);
    return it != entries_.end() && it->second.isFresh();
}

CacheEntry HttpCache::getEntry(const HttpRequest& request) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = cacheKey(request);
    auto it = entries_.find(key);
    if (it != entries_.end()) {
        return it->second;
    }
    return CacheEntry{};
}

void HttpCache::remove(const std::string& url) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto it = entries_.begin(); it != entries_.end(); ) {
        if (it->second.url == url) {
            std::string bodyPath = cacheDir_ + "/" + it->first + ".body";
            fs::remove(bodyPath);
            currentSize_ -= it->second.size;
            it = entries_.erase(it);
        } else {
            ++it;
        }
    }
}

void HttpCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& [key, entry] : entries_) {
        std::string bodyPath = cacheDir_ + "/" + key + ".body";
        fs::remove(bodyPath);
    }
    
    entries_.clear();
    currentSize_ = 0;
}

void HttpCache::evictStale() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto it = entries_.begin(); it != entries_.end(); ) {
        if (it->second.isStale()) {
            std::string bodyPath = cacheDir_ + "/" + it->first + ".body";
            fs::remove(bodyPath);
            currentSize_ -= it->second.size;
            it = entries_.erase(it);
        } else {
            ++it;
        }
    }
}

size_t HttpCache::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentSize_;
}

void HttpCache::setMaxSize(size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    maxSize_ = bytes;
    
    // Evict if over new limit
    while (currentSize_ > maxSize_ && !entries_.empty()) {
        evictLRU();
    }
}

void HttpCache::evictLRU() {
    // Find oldest entry
    auto oldest = entries_.begin();
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->second.date < oldest->second.date) {
            oldest = it;
        }
    }
    
    if (oldest != entries_.end()) {
        std::string bodyPath = cacheDir_ + "/" + oldest->first + ".body";
        fs::remove(bodyPath);
        currentSize_ -= oldest->second.size;
        entries_.erase(oldest);
    }
}

// =============================================================================
// Global Instance
// =============================================================================

HttpCache& getHttpCache() {
    static HttpCache instance;
    return instance;
}

} // namespace Zepra::Networking
