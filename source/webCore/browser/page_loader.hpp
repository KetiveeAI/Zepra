/**
 * @file page_loader.hpp
 * @brief Page loading with full networking, SSL, permissions, cache integration
 */

#pragma once

#include "networking/http_client.hpp"
#include "networking/http_request.hpp"
#include "networking/http_response.hpp"
#include "networking/http_cache.hpp"
#include "networking/dns_resolver.hpp"
#include "networking/ssl_context.hpp"
#include "networking/cookie_manager.hpp"
#include "storage/site_settings.hpp"
#include "storage/permission_manager.hpp"
#include "storage/local_storage.hpp"

#include <string>
#include <functional>
#include <memory>
#include <chrono>

namespace Zepra::WebCore {

/**
 * @brief Load state for progress tracking
 */
enum class LoadState {
    Idle,
    ResolvingDNS,
    Connecting,
    SSLHandshake,
    SendingRequest,
    WaitingResponse,
    ReceivingData,
    Parsing,
    Complete,
    Error
};

/**
 * @brief Page load result
 */
struct PageLoadResult {
    bool success = false;
    std::string url;
    std::string finalUrl;      // After redirects
    std::string contentType;
    std::string content;       // HTML/text content
    std::vector<uint8_t> data; // Binary data
    
    int statusCode = 0;
    std::string error;
    
    // Security info
    bool isSecure = false;
    std::string certificateIssuer;
    Networking::CertVerifyResult certResult = Networking::CertVerifyResult::UNKNOWN_ERROR;
    
    // Timing
    double dnsTimeMs = 0;
    double connectTimeMs = 0;
    double sslTimeMs = 0;
    double ttfbMs = 0;      // Time to first byte
    double totalTimeMs = 0;
    
    // Cache
    bool fromCache = false;
};

/**
 * @brief Page load callbacks
 */
using LoadProgressCallback = std::function<void(LoadState state, float progress)>;
using LoadCompleteCallback = std::function<void(const PageLoadResult& result)>;
using PermissionRequestCallback = std::function<void(Storage::PermissionType, 
                                                      std::function<void(bool)>)>;

/**
 * @brief PageLoader - Handles full page loading with networking integration
 * 
 * Flow:
 * 1. Normalize URL
 * 2. Check site permissions (JavaScript, images, etc.)
 * 3. Check HTTP cache
 * 4. Resolve DNS
 * 5. Connect + SSL handshake
 * 6. Add cookies
 * 7. Send request
 * 8. Receive response
 * 9. Update cookies
 * 10. Cache response (if cacheable)
 * 11. Return to renderer
 */
class PageLoader {
public:
    PageLoader();
    ~PageLoader();
    
    /**
     * @brief Load page synchronously
     */
    PageLoadResult load(const std::string& url);
    
    /**
     * @brief Load page asynchronously
     */
    void loadAsync(const std::string& url, LoadCompleteCallback onComplete);
    
    /**
     * @brief Set progress callback
     */
    void setOnProgress(LoadProgressCallback callback) { onProgress_ = std::move(callback); }
    
    /**
     * @brief Set permission request callback (for UI prompts)
     */
    void setOnPermissionRequest(PermissionRequestCallback callback) {
        onPermissionRequest_ = std::move(callback);
    }
    
    /**
     * @brief Cancel current load
     */
    void cancel();
    
    /**
     * @brief Check if loading
     */
    bool isLoading() const { return loading_; }
    
    /**
     * @brief Get current state
     */
    LoadState state() const { return state_; }
    
    // ===== Configuration =====
    
    /**
     * @brief Set user agent
     */
    void setUserAgent(const std::string& ua) { userAgent_ = ua; }
    
    /**
     * @brief Set referrer policy
     */
    void setReferrer(const std::string& referrer) { referrer_ = referrer; }
    
    /**
     * @brief Set cache mode
     */
    enum class CacheMode { Default, NoStore, Reload, ForceCache };
    void setCacheMode(CacheMode mode) { cacheMode_ = mode; }
    
    /**
     * @brief Set credentials mode
     */
    enum class CredentialsMode { SameOrigin, Include, Omit };
    void setCredentialsMode(CredentialsMode mode) { credentialsMode_ = mode; }
    
    /**
     * @brief Get origin for current load
     */
    std::string getCurrentOrigin() const { return currentOrigin_; }
    
private:
    // Progress notification
    void notifyProgress(LoadState state, float progress = 0);
    
    // Internal steps
    bool checkSiteSettings(const std::string& origin);
    PageLoadResult loadFromCache(const std::string& url);
    PageLoadResult fetchFromNetwork(const std::string& url);
    void cacheResponse(const std::string& url, const Networking::HttpResponse& response);
    
    // State
    std::atomic<bool> loading_{false};
    std::atomic<bool> cancelled_{false};
    LoadState state_ = LoadState::Idle;
    std::string currentOrigin_;
    
    // Callbacks
    LoadProgressCallback onProgress_;
    PermissionRequestCallback onPermissionRequest_;
    
    // Config
    std::string userAgent_ = "ZepraBrowser/1.0";
    std::string referrer_;
    CacheMode cacheMode_ = CacheMode::Default;
    CredentialsMode credentialsMode_ = CredentialsMode::SameOrigin;
    
    // Networking
    std::unique_ptr<Networking::HttpClient> httpClient_;
};

/**
 * @brief Global page loader
 */
PageLoader& getPageLoader();

} // namespace Zepra::WebCore
