/**
 * @file http_client.hpp
 * @brief HTTP/HTTPS client for network requests
 */

#pragma once

#include "networking/http_request.hpp"
#include "networking/http_response.hpp"
#include "networking/ssl_context.hpp"
#include "networking/cookie_manager.hpp"

#include <string>
#include <memory>
#include <functional>
#include <future>

namespace Zepra::Networking {

/**
 * @brief HTTP Client configuration
 */
struct HttpClientConfig {
    int connectTimeoutMs = 30000;
    int readTimeoutMs = 60000;
    int maxRedirects = 10;
    bool followRedirects = true;
    bool verifySsl = true;
    bool useCookies = true;
    std::string userAgent = "Zepra/1.0";
    std::string proxyUrl;
};

/**
 * @brief HTTP Client
 */
class HttpClient {
public:
    HttpClient();
    explicit HttpClient(const HttpClientConfig& config);
    ~HttpClient();
    
    /**
     * @brief Synchronous request
     */
    HttpResponse send(const HttpRequest& request);
    
    /**
     * @brief Asynchronous request
     */
    std::future<HttpResponse> sendAsync(const HttpRequest& request);
    
    /**
     * @brief GET request
     */
    HttpResponse get(const std::string& url);
    
    /**
     * @brief POST request
     */
    HttpResponse post(const std::string& url, const std::string& body,
                      const std::string& contentType = "application/x-www-form-urlencoded");
    
    /**
     * @brief Download to file
     */
    using ProgressCallback = std::function<void(size_t downloaded, size_t total)>;
    bool download(const std::string& url, const std::string& filePath,
                  ProgressCallback progress = nullptr);
    
    /**
     * @brief Cancel all pending requests
     */
    void cancelAll();
    
    /**
     * @brief Get/Set configuration
     */
    const HttpClientConfig& config() const { return config_; }
    void setConfig(const HttpClientConfig& config) { config_ = config; }
    
private:
    HttpClientConfig config_;
    std::unique_ptr<class HttpClientImpl> impl_;
};

} // namespace Zepra::Networking
