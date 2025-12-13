/**
 * @file http_response.hpp
 * @brief HTTP response representation
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace Zepra::Networking {

/**
 * @brief HTTP Response
 */
class HttpResponse {
public:
    HttpResponse() = default;
    
    // Status
    void setStatusCode(int code) { statusCode_ = code; }
    int statusCode() const { return statusCode_; }
    
    void setStatusMessage(const std::string& msg) { statusMessage_ = msg; }
    const std::string& statusMessage() const { return statusMessage_; }
    
    bool isSuccess() const { return statusCode_ >= 200 && statusCode_ < 300; }
    bool isRedirect() const { return statusCode_ >= 300 && statusCode_ < 400; }
    bool isClientError() const { return statusCode_ >= 400 && statusCode_ < 500; }
    bool isServerError() const { return statusCode_ >= 500; }
    bool isError() const { return statusCode_ >= 400; }
    
    // Headers
    void setHeader(const std::string& name, const std::string& value);
    std::string header(const std::string& name) const;
    bool hasHeader(const std::string& name) const;
    const std::unordered_map<std::string, std::string>& headers() const { return headers_; }
    
    // Body
    void setBody(const std::vector<uint8_t>& body) { body_ = body; }
    void appendBody(const uint8_t* data, size_t size);
    const std::vector<uint8_t>& body() const { return body_; }
    std::string bodyString() const;
    size_t contentLength() const;
    
    // Content type
    std::string contentType() const;
    std::string mimeType() const;
    std::string charset() const;
    
    // Redirects
    std::string location() const;
    
    // Cookies
    std::vector<std::string> setCookieHeaders() const;
    
    // Timing
    void setTimingMs(double ms) { timingMs_ = ms; }
    double timingMs() const { return timingMs_; }
    
    // Error
    void setError(const std::string& error) { error_ = error; }
    const std::string& error() const { return error_; }
    bool hasError() const { return !error_.empty(); }
    
    // URL (final after redirects)
    void setUrl(const std::string& url) { url_ = url; }
    const std::string& url() const { return url_; }
    
private:
    int statusCode_ = 0;
    std::string statusMessage_;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<uint8_t> body_;
    double timingMs_ = 0;
    std::string error_;
    std::string url_;
};

} // namespace Zepra::Networking
