/**
 * @file resource_loader.hpp
 * @brief Resource loading and caching
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <future>
#include "networking/resource_loader.hpp"

namespace Zepra::WebCore {

/**
 * @brief HTTP method
 */
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    HEAD,
    OPTIONS,
    PATCH
};

/**
 * @brief Resource type
 */
enum class ResourceType {
    Document,
    Stylesheet,
    Script,
    Image,
    Font,
    Media,
    XHR,
    Fetch,
    Other
};

/**
 * @brief Request state
 */
enum class RequestState {
    Pending,
    Loading,
    Complete,
    Failed,
    Cancelled
};

/**
 * @brief HTTP headers
 */
class Headers {
public:
    void set(const std::string& name, const std::string& value);
    std::string get(const std::string& name) const;
    bool has(const std::string& name) const;
    void remove(const std::string& name);
    
    const std::unordered_map<std::string, std::string>& all() const { return headers_; }
    
private:
    std::unordered_map<std::string, std::string> headers_;
};

/**
 * @brief HTTP Request
 */
class Request {
public:
    Request(const std::string& url, HttpMethod method = HttpMethod::GET);
    
    const std::string& url() const { return url_; }
    HttpMethod method() const { return method_; }
    
    Headers& headers() { return headers_; }
    const Headers& headers() const { return headers_; }
    
    void setBody(const std::string& body) { body_ = body; }
    const std::string& body() const { return body_; }
    
    void setResourceType(ResourceType type) { resourceType_ = type; }
    ResourceType resourceType() const { return resourceType_; }
    
    // Credentials
    void setCredentials(bool include) { credentials_ = include; }
    bool credentials() const { return credentials_; }
    
    // Timeout
    void setTimeout(int ms) { timeout_ = ms; }
    int timeout() const { return timeout_; }
    
private:
    std::string url_;
    HttpMethod method_;
    Headers headers_;
    std::string body_;
    ResourceType resourceType_ = ResourceType::Other;
    bool credentials_ = false;
    int timeout_ = 30000; // 30 seconds
};

/**
 * @brief HTTP Response
 */
class Response {
public:
    Response();
    Response(int status, const std::string& statusText = "");
    
    int status() const { return status_; }
    const std::string& statusText() const { return statusText_; }
    bool ok() const { return status_ >= 200 && status_ < 300; }
    
    Headers& headers() { return headers_; }
    const Headers& headers() const { return headers_; }
    
    // Body
    const std::vector<uint8_t>& body() const { return body_; }
    std::string text() const;
    void setBody(const std::vector<uint8_t>& data) { body_ = data; }
    void setBody(const std::string& text);
    
    // URL (after redirects)
    const std::string& url() const { return url_; }
    void setUrl(const std::string& url) { url_ = url; }
    
private:
    int status_ = 0;
    std::string statusText_;
    Headers headers_;
    std::vector<uint8_t> body_;
    std::string url_;
};

/**
 * @brief Resource loading callbacks
 */
struct ResourceCallbacks {
    std::function<void(const Response&)> onComplete;
    std::function<void(const std::string&)> onError;
    std::function<void(size_t loaded, size_t total)> onProgress;
};

/**
 * @brief Resource loader
 */
class ResourceLoader {
public:
    ResourceLoader();
    ~ResourceLoader();
    
    // Sync loading (for simple cases)
    Response load(const Request& request);
    
    // Async loading
    void loadAsync(const Request& request, const ResourceCallbacks& callbacks);
    
    // Convenience methods
    Response loadURL(const std::string& url);
    void loadURLAsync(const std::string& url, 
                      std::function<void(const Response&)> onComplete,
                      std::function<void(const std::string&)> onError = nullptr);
    
    // Cancel pending request
    void cancel(const std::string& url);
    void cancelAll();
    
    // Cache control
    void setCacheEnabled(bool enabled) { cacheEnabled_ = enabled; }
    void clearCache();
    
private:
    bool cacheEnabled_ = true;
    std::unordered_map<std::string, Response> cache_;
    std::unique_ptr<Networking::ResourceLoader> networkLoader_;
};

/**
 * @brief URL utilities
 */
class URL {
public:
    URL(const std::string& url);
    URL(const std::string& url, const std::string& base);
    
    const std::string& href() const { return href_; }
    const std::string& protocol() const { return protocol_; }
    const std::string& host() const { return host_; }
    const std::string& hostname() const { return hostname_; }
    int port() const { return port_; }
    const std::string& pathname() const { return pathname_; }
    const std::string& search() const { return search_; }
    const std::string& hash() const { return hash_; }
    
    std::string origin() const { return protocol_ + "//" + host_; }
    
    bool isValid() const { return valid_; }
    
    // Resolve relative URL
    static std::string resolve(const std::string& base, const std::string& relative);
    
private:
    void parse(const std::string& url);
    
    std::string href_;
    std::string protocol_;
    std::string host_;
    std::string hostname_;
    int port_ = 0;
    std::string pathname_;
    std::string search_;
    std::string hash_;
    bool valid_ = false;
};

} // namespace Zepra::WebCore
