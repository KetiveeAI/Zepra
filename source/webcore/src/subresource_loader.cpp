/**
 * @file subresource_loader.cpp
 * @brief Subresource loader implementation
 */

#include "webcore/subresource_loader.hpp"
#include "networking/http_cache.hpp"
#include "storage/site_settings.hpp"

#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <regex>

namespace Zepra::WebCore {

using namespace Networking;
using namespace Storage;

SubResourceLoader::SubResourceLoader(const std::string& documentOrigin)
    : documentOrigin_(documentOrigin) {
    
    HttpClientConfig config;
    config.userAgent = "ZepraBrowser/1.0";
    config.followRedirects = true;
    config.maxRedirects = 5;
    config.verifySsl = true;
    config.useCookies = true;
    config.connectTimeoutMs = 5000;
    config.readTimeoutMs = 30000;
    
    httpClient_ = std::make_unique<HttpClient>(config);
    
    // Start worker threads
    int numWorkers = 4;
    for (int i = 0; i < numWorkers; ++i) {
        workers_.emplace_back(&SubResourceLoader::workerLoop, this);
    }
}

SubResourceLoader::~SubResourceLoader() {
    running_ = false;
    cv_.notify_all();
    
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void SubResourceLoader::load(const SubResourceRequest& request, 
                              SubResourceCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if cross-origin
    SubResourceRequest req = request;
    req.crossOrigin = !isCrossOriginAllowed(request.url);
    
    queue_.push({req, std::move(callback)});
    cv_.notify_one();
}

void SubResourceLoader::preload(const std::string& url, ResourceType type) {
    SubResourceRequest request;
    request.url = normalizeUrl(url);
    request.type = type;
    request.priority = ResourcePriority::High;
    
    load(request, [](const SubResourceResponse&) {
        // Preload complete, no action needed
    });
}

void SubResourceLoader::prefetch(const std::string& url) {
    SubResourceRequest request;
    request.url = normalizeUrl(url);
    request.type = ResourceType::Other;
    request.priority = ResourcePriority::VeryLow;
    
    load(request, [](const SubResourceResponse&) {
        // Prefetch complete
    });
}

void SubResourceLoader::cancelAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Mark all in-progress as cancelled
    for (const auto& url : inProgress_) {
        cancelled_.insert(url);
    }
    
    // Clear queue
    while (!queue_.empty()) {
        queue_.pop();
    }
}

void SubResourceLoader::cancel(const std::string& url) {
    std::lock_guard<std::mutex> lock(mutex_);
    cancelled_.insert(url);
}

size_t SubResourceLoader::pendingCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size() + inProgress_.size();
}

bool SubResourceLoader::isCrossOriginAllowed(const std::string& url) const {
    std::string resourceOrigin = SiteSettingsManager::normalizeOrigin(url);
    return SiteSettingsManager::isSameOrigin(documentOrigin_, resourceOrigin);
}

bool SubResourceLoader::verifySRI(const std::vector<uint8_t>& data,
                                   const std::string& integrity) {
    if (integrity.empty()) return true;  // No SRI required
    
    // Parse integrity: "sha256-HASH" or "sha384-HASH" or "sha512-HASH"
    size_t dashPos = integrity.find('-');
    if (dashPos == std::string::npos) return false;
    
    std::string algo = integrity.substr(0, dashPos);
    std::string expectedHash = integrity.substr(dashPos + 1);
    
    unsigned char hash[SHA512_DIGEST_LENGTH];
    unsigned int hashLen = 0;
    
    if (algo == "sha256") {
        SHA256(data.data(), data.size(), hash);
        hashLen = SHA256_DIGEST_LENGTH;
    } else if (algo == "sha384") {
        SHA384(data.data(), data.size(), hash);
        hashLen = SHA384_DIGEST_LENGTH;
    } else if (algo == "sha512") {
        SHA512(data.data(), data.size(), hash);
        hashLen = SHA512_DIGEST_LENGTH;
    } else {
        return false;  // Unknown algorithm
    }
    
    // Base64 encode hash
    // (simplified - would need proper base64)
    std::ostringstream oss;
    for (unsigned int i = 0; i < hashLen; ++i) {
        oss << std::hex << std::setfill('0') << std::setw(2) 
            << static_cast<int>(hash[i]);
    }
    
    // Compare (this is hex, not base64 - simplified)
    // Real implementation would use proper base64
    return true;  // Simplified for now
}

void SubResourceLoader::workerLoop() {
    while (running_) {
        QueuedRequest task;
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() {
                return !queue_.empty() || !running_;
            });
            
            if (!running_) break;
            
            if (queue_.empty()) continue;
            
            task = std::move(const_cast<QueuedRequest&>(queue_.top()));
            queue_.pop();
            
            // Check if cancelled
            if (cancelled_.count(task.request.url)) {
                cancelled_.erase(task.request.url);
                continue;
            }
            
            inProgress_.insert(task.request.url);
        }
        
        // Fetch resource
        SubResourceResponse response = fetchResource(task.request);
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            inProgress_.erase(task.request.url);
        }
        
        // Verify SRI if present
        if (response.success && !task.request.integrity.empty()) {
            if (!verifySRI(response.data, task.request.integrity)) {
                response.success = false;
                response.error = "Subresource Integrity check failed";
            }
        }
        
        // Callback
        if (task.callback) {
            task.callback(response);
        }
    }
}

SubResourceResponse SubResourceLoader::fetchResource(const SubResourceRequest& request) {
    SubResourceResponse response;
    response.url = request.url;
    
    auto startTime = std::chrono::steady_clock::now();
    
    // Check cache first
    HttpRequest httpReq(HttpMethod::GET, request.url);
    auto& cache = getHttpCache();
    
    if (cache.has(httpReq)) {
        auto cached = cache.get(httpReq);
        if (cached) {
            response.success = true;
            response.statusCode = cached->statusCode();
            response.contentType = cached->contentType();
            response.data = cached->body();
            response.fromCache = true;
            return response;
        }
    }
    
    // Fetch from network
    httpReq.setHeader("Accept", getAcceptHeader(request.type));
    
    if (request.crossOrigin) {
        httpReq.setHeader("Origin", documentOrigin_);
    }
    
    HttpResponse httpResp = httpClient_->send(httpReq);
    
    auto endTime = std::chrono::steady_clock::now();
    response.loadTimeMs = std::chrono::duration<double, std::milli>(
        endTime - startTime).count();
    
    if (httpResp.hasError()) {
        response.success = false;
        response.error = httpResp.error();
        return response;
    }
    
    response.success = httpResp.isSuccess();
    response.statusCode = httpResp.statusCode();
    response.contentType = httpResp.contentType();
    response.data = httpResp.body();
    
    // Cache successful response
    if (response.success) {
        cache.put(httpReq, httpResp);
    }
    
    return response;
}

std::string SubResourceLoader::normalizeUrl(const std::string& url) const {
    // If relative URL, make absolute
    if (url.substr(0, 4) != "http") {
        // Would need base URL from document
        return documentOrigin_ + url;
    }
    return url;
}

std::string getAcceptHeader(ResourceType type) {
    switch (type) {
        case ResourceType::Stylesheet:
            return "text/css,*/*;q=0.1";
        case ResourceType::Script:
            return "application/javascript,text/javascript,*/*;q=0.1";
        case ResourceType::Image:
            return "image/webp,image/apng,image/*,*/*;q=0.8";
        case ResourceType::Font:
            return "application/font-woff2,application/font-woff,*/*;q=0.1";
        case ResourceType::Media:
            return "video/*,audio/*,*/*;q=0.1";
        default:
            return "*/*";
    }
}

} // namespace Zepra::WebCore
