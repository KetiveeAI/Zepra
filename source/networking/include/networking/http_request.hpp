/**
 * @file http_request.hpp
 * @brief HTTP request representation
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace Zepra::Networking {

/**
 * @brief HTTP method
 */
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS
};

/**
 * @brief HTTP Request
 */
class HttpRequest {
public:
    HttpRequest() = default;
    explicit HttpRequest(const std::string& url);
    HttpRequest(HttpMethod method, const std::string& url);
    
    // URL
    void setUrl(const std::string& url);
    const std::string& url() const { return url_; }
    
    // Method
    void setMethod(HttpMethod method) { method_ = method; }
    HttpMethod method() const { return method_; }
    std::string methodString() const;
    
    // Headers
    void setHeader(const std::string& name, const std::string& value);
    void addHeader(const std::string& name, const std::string& value);
    std::string header(const std::string& name) const;
    bool hasHeader(const std::string& name) const;
    const std::unordered_map<std::string, std::string>& headers() const { return headers_; }
    
    // Body
    void setBody(const std::string& body);
    void setBody(const std::vector<uint8_t>& body);
    const std::vector<uint8_t>& body() const { return body_; }
    std::string bodyString() const;
    
    // Content type
    void setContentType(const std::string& type);
    std::string contentType() const;
    
    // Timeout
    void setTimeout(int ms) { timeoutMs_ = ms; }
    int timeout() const { return timeoutMs_; }
    
    // URL components
    std::string scheme() const;
    std::string host() const;
    int port() const;
    std::string path() const;
    std::string query() const;
    
    bool isSecure() const;
    
private:
    std::string url_;
    HttpMethod method_ = HttpMethod::GET;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<uint8_t> body_;
    int timeoutMs_ = 30000;
};

} // namespace Zepra::Networking
