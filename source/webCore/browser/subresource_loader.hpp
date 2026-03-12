/**
 * @file subresource_loader.hpp
 * @brief Subresource loading for CSS, JS, images, fonts, etc.
 */

#pragma once

#include "networking/http_client.hpp"
#include "storage/site_settings.hpp"

#include <string>
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <unordered_set>

namespace Zepra::WebCore {

/**
 * @brief Resource type
 */
enum class ResourceType {
    Document,
    Stylesheet,
    Script,
    Image,
    Font,
    Media,      // Audio/Video
    XHR,        // XMLHttpRequest / Fetch
    WebSocket,
    Manifest,
    Other
};

/**
 * @brief Resource priority
 */
enum class ResourcePriority {
    VeryHigh,   // Main document, critical CSS
    High,       // Scripts, important images
    Medium,     // Normal images, fonts
    Low,        // Prefetch, lazy images
    VeryLow     // Speculative
};

/**
 * @brief Subresource request
 */
struct SubResourceRequest {
    std::string url;
    std::string initiatorUrl;   // Page that requested this
    ResourceType type = ResourceType::Other;
    ResourcePriority priority = ResourcePriority::Medium;
    bool crossOrigin = false;
    std::string integrity;      // SRI hash
    std::string referrerPolicy;
    bool isAsync = false;       // For scripts
    bool isDefer = false;       // For scripts
};

/**
 * @brief Subresource response
 */
struct SubResourceResponse {
    bool success = false;
    std::string url;
    std::string contentType;
    std::vector<uint8_t> data;
    int statusCode = 0;
    std::string error;
    bool fromCache = false;
    double loadTimeMs = 0;
};

/**
 * @brief Subresource loader callback
 */
using SubResourceCallback = std::function<void(const SubResourceResponse&)>;

/**
 * @brief Request in queue
 */
struct QueuedRequest {
    SubResourceRequest request;
    SubResourceCallback callback;
    
    bool operator<(const QueuedRequest& other) const {
        // Lower priority value = higher priority = should come first
        return static_cast<int>(request.priority) > static_cast<int>(other.request.priority);
    }
};

/**
 * @brief SubResourceLoader
 * 
 * Loads CSS, JS, images, fonts for a page.
 * Features:
 * - Priority queue
 * - Concurrent connections (max 6 per host)
 * - Cross-origin checks
 * - SRI (Subresource Integrity)
 * - Caching
 */
class SubResourceLoader {
public:
    SubResourceLoader(const std::string& documentOrigin);
    ~SubResourceLoader();
    
    /**
     * @brief Load resource
     */
    void load(const SubResourceRequest& request, SubResourceCallback callback);
    
    /**
     * @brief Preload resource (hint)
     */
    void preload(const std::string& url, ResourceType type);
    
    /**
     * @brief Prefetch resource (low priority)
     */
    void prefetch(const std::string& url);
    
    /**
     * @brief Cancel all pending requests
     */
    void cancelAll();
    
    /**
     * @brief Cancel request for URL
     */
    void cancel(const std::string& url);
    
    /**
     * @brief Get number of pending requests
     */
    size_t pendingCount() const;
    
    /**
     * @brief Check if cross-origin request allowed
     */
    bool isCrossOriginAllowed(const std::string& url) const;
    
    /**
     * @brief Verify SRI hash
     */
    bool verifySRI(const std::vector<uint8_t>& data, const std::string& integrity);
    
    /**
     * @brief Set max concurrent connections per host
     */
    void setMaxConnectionsPerHost(int max) { maxConnectionsPerHost_ = max; }
    
    /**
     * @brief Set document origin (for CORS)
     */
    void setDocumentOrigin(const std::string& origin) { documentOrigin_ = origin; }
    
private:
    void workerLoop();
    SubResourceResponse fetchResource(const SubResourceRequest& request);
    std::string normalizeUrl(const std::string& url) const;
    
    std::string documentOrigin_;
    int maxConnectionsPerHost_ = 6;
    
    mutable std::mutex mutex_;
    std::priority_queue<QueuedRequest> queue_;
    std::unordered_set<std::string> inProgress_;
    std::unordered_set<std::string> cancelled_;
    
    std::atomic<bool> running_{true};
    std::vector<std::thread> workers_;
    std::condition_variable cv_;
    
    std::unique_ptr<Networking::HttpClient> httpClient_;
};

} // namespace Zepra::WebCore
