/**
 * @file page_loader.cpp
 * @brief Page loader implementation with DNS, SSL, cache, permissions, cookies
 */

#include "webcore/page_loader.hpp"

#include "networking/http_client.hpp"
#include "networking/http_cache.hpp"
#include "networking/dns_resolver.hpp"
#include "networking/cookie_manager.hpp"
#include "storage/site_settings.hpp"
#include "storage/permission_manager.hpp"

#include <future>
#include <thread>
#include <chrono>
#include <regex>

namespace Zepra::WebCore {

using namespace Networking;
using namespace Storage;

// =============================================================================
// PageLoader Implementation
// =============================================================================

PageLoader::PageLoader() {
    HttpClientConfig config;
    config.userAgent = userAgent_;
    config.followRedirects = true;
    config.maxRedirects = 10;
    config.verifySsl = true;
    config.useCookies = true;
    config.connectTimeoutMs = 10000;
    config.readTimeoutMs = 30000;
    
    httpClient_ = std::make_unique<HttpClient>(config);
}

PageLoader::~PageLoader() {
    cancel();
}

void PageLoader::notifyProgress(LoadState state, float progress) {
    state_ = state;
    if (onProgress_) {
        onProgress_(state, progress);
    }
}

PageLoadResult PageLoader::load(const std::string& url) {
    PageLoadResult result;
    result.url = url;
    
    loading_ = true;
    cancelled_ = false;
    
    auto startTime = std::chrono::steady_clock::now();
    
    // ===== Step 1: Normalize URL and extract origin =====
    notifyProgress(LoadState::Idle, 0);
    
    currentOrigin_ = SiteSettingsManager::normalizeOrigin(url);
    result.isSecure = url.substr(0, 5) == "https";
    
    // ===== Step 2: Check site settings =====
    if (!checkSiteSettings(currentOrigin_)) {
        result.success = false;
        result.error = "Site blocked by user settings";
        notifyProgress(LoadState::Error, 0);
        loading_ = false;
        return result;
    }
    
    if (cancelled_) {
        result.error = "Cancelled";
        loading_ = false;
        return result;
    }
    
    // ===== Step 3: Check cache =====
    if (cacheMode_ == CacheMode::Default || cacheMode_ == CacheMode::ForceCache) {
        PageLoadResult cached = loadFromCache(url);
        if (cached.success) {
            cached.fromCache = true;
            loading_ = false;
            notifyProgress(LoadState::Complete, 1.0f);
            return cached;
        }
    }
    
    if (cancelled_) {
        result.error = "Cancelled";
        loading_ = false;
        return result;
    }
    
    // ===== Step 4-8: Fetch from network =====
    result = fetchFromNetwork(url);
    
    // ===== Step 9: Calculate timing =====
    auto endTime = std::chrono::steady_clock::now();
    result.totalTimeMs = std::chrono::duration<double, std::milli>(
        endTime - startTime).count();
    
    loading_ = false;
    
    if (result.success) {
        notifyProgress(LoadState::Complete, 1.0f);
    } else {
        notifyProgress(LoadState::Error, 0);
    }
    
    return result;
}

void PageLoader::loadAsync(const std::string& url, LoadCompleteCallback onComplete) {
    std::thread([this, url, onComplete]() {
        PageLoadResult result = load(url);
        if (onComplete) {
            onComplete(result);
        }
    }).detach();
}

void PageLoader::cancel() {
    cancelled_ = true;
    // HttpClient would need cancel support
}

bool PageLoader::checkSiteSettings(const std::string& origin) {
    auto& ssm = getSiteSettingsManager();
    auto settings = ssm.getSiteSettings(origin);
    
    // Check if JavaScript is required but disabled
    // For now, allow all sites
    // Could block based on user preferences
    
    return true;
}

PageLoadResult PageLoader::loadFromCache(const std::string& url) {
    PageLoadResult result;
    result.url = url;
    
    HttpRequest request(HttpMethod::GET, url);
    auto& cache = getHttpCache();
    
    if (cache.has(request)) {
        auto entry = cache.getEntry(request);
        
        if (entry.isFresh()) {
            auto response = cache.get(request);
            if (response) {
                result.success = true;
                result.statusCode = response->statusCode();
                result.contentType = response->contentType();
                result.data = response->body();
                result.content = std::string(result.data.begin(), result.data.end());
                result.fromCache = true;
                result.finalUrl = url;
                return result;
            }
        }
    }
    
    return result;  // Not found or stale
}

PageLoadResult PageLoader::fetchFromNetwork(const std::string& url) {
    PageLoadResult result;
    result.url = url;
    
    // ===== DNS Resolution =====
    notifyProgress(LoadState::ResolvingDNS, 0.1f);
    
    auto dnsStart = std::chrono::steady_clock::now();
    
    // Extract host from URL
    std::regex hostRegex(R"(https?://([^/:]+))");
    std::smatch match;
    std::string host;
    if (std::regex_search(url, match, hostRegex)) {
        host = match[1];
    }
    
    auto& dns = getDnsResolver();
    DnsResult dnsResult = dns.resolve(host);
    
    auto dnsEnd = std::chrono::steady_clock::now();
    result.dnsTimeMs = std::chrono::duration<double, std::milli>(dnsEnd - dnsStart).count();
    
    if (!dnsResult.success) {
        result.success = false;
        result.error = "DNS resolution failed: " + dnsResult.error;
        return result;
    }
    
    if (cancelled_) {
        result.error = "Cancelled";
        return result;
    }
    
    // ===== Connect + SSL =====
    notifyProgress(LoadState::Connecting, 0.2f);
    
    auto connectStart = std::chrono::steady_clock::now();
    
    // The HttpClient handles connection internally
    // For SSL timing, we'd need more instrumentation
    
    notifyProgress(LoadState::SSLHandshake, 0.3f);
    
    auto connectEnd = std::chrono::steady_clock::now();
    result.connectTimeMs = std::chrono::duration<double, std::milli>(
        connectEnd - connectStart).count();
    
    if (cancelled_) {
        result.error = "Cancelled";
        return result;
    }
    
    // ===== Build Request with Cookies =====
    notifyProgress(LoadState::SendingRequest, 0.4f);
    
    HttpRequest request(HttpMethod::GET, url);
    request.setHeader("User-Agent", userAgent_);
    
    if (!referrer_.empty()) {
        request.setHeader("Referer", referrer_);
    }
    
    request.setHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    request.setHeader("Accept-Language", "en-US,en;q=0.5");
    request.setHeader("Accept-Encoding", "gzip, deflate");
    
    // Cookies are added by HttpClient based on credentials mode
    
    if (cancelled_) {
        result.error = "Cancelled";
        return result;
    }
    
    // ===== Send Request and Wait for Response =====
    notifyProgress(LoadState::WaitingResponse, 0.5f);
    
    auto requestStart = std::chrono::steady_clock::now();
    
    HttpResponse response = httpClient_->send(request);
    
    auto responseEnd = std::chrono::steady_clock::now();
    result.ttfbMs = std::chrono::duration<double, std::milli>(
        responseEnd - requestStart).count();
    
    if (response.hasError()) {
        result.success = false;
        result.error = response.error();
        return result;
    }
    
    // ===== Process Response =====
    notifyProgress(LoadState::ReceivingData, 0.7f);
    
    result.success = response.isSuccess();
    result.statusCode = response.statusCode();
    result.contentType = response.contentType();
    result.data = response.body();
    result.content = std::string(result.data.begin(), result.data.end());
    result.finalUrl = response.url().empty() ? url : response.url();
    
    // ===== Cache Response =====
    if (result.success && cacheMode_ != CacheMode::NoStore) {
        cacheResponse(url, response);
    }
    
    // ===== Parse (done by caller) =====
    notifyProgress(LoadState::Parsing, 0.9f);
    
    return result;
}

void PageLoader::cacheResponse(const std::string& url, 
                                const HttpResponse& response) {
    HttpRequest request(HttpMethod::GET, url);
    getHttpCache().put(request, response);
}

// =============================================================================
// Global Instance
// =============================================================================

PageLoader& getPageLoader() {
    static PageLoader instance;
    return instance;
}

} // namespace Zepra::WebCore
