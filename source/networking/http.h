// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once

#include "common/types.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>

namespace zepra {

// HTTP request method
enum class HTTPMethod {
    GET,
    POST,
    PUT,
    DELETE,
    HEAD,
    OPTIONS,
    PATCH
};

// HTTP response status
struct HTTPResponse {
    int statusCode;
    String statusText;
    String body;
    std::map<String, String> headers;
    String url;
    bool success;
    String error;
    
    HTTPResponse() : statusCode(0), success(false) {}
};

// HTTP request configuration
struct HTTPRequest {
    HTTPMethod method;
    String url;
    String body;
    std::map<String, String> headers;
    int timeout;
    bool followRedirects;
    int maxRedirects;
    String userAgent;
    
    HTTPRequest() : method(HTTPMethod::GET), timeout(30), followRedirects(true), maxRedirects(10) {
        userAgent = BROWSER_USER_AGENT;
    }
};

// HTTP client callback types
using HTTPResponseCallback = std::function<void(const HTTPResponse&)>;
using HTTPProgressCallback = std::function<void(size_t downloaded, size_t total)>;

// HTTP client class
class HTTPClient {
public:
    HTTPClient();
    ~HTTPClient();
    
    // Synchronous requests
    HTTPResponse get(const String& url);
    HTTPResponse post(const String& url, const String& data);
    HTTPResponse put(const String& url, const String& data);
    HTTPResponse delete_(const String& url);
    HTTPResponse request(const HTTPRequest& request);
    
    // Asynchronous requests
    void getAsync(const String& url, HTTPResponseCallback callback);
    void postAsync(const String& url, const String& data, HTTPResponseCallback callback);
    void requestAsync(const HTTPRequest& request, HTTPResponseCallback callback);
    
    // Configuration
    void setTimeout(int seconds);
    void setUserAgent(const String& userAgent);
    void setDefaultHeaders(const std::map<String, String>& headers);
    void setProxy(const String& proxy);
    void setFollowRedirects(bool follow);
    void setMaxRedirects(int maxRedirects);
    
    // Cookie management
    void setCookie(const String& name, const String& value, const String& domain = "");
    String getCookie(const String& name, const String& domain = "") const;
    void clearCookies();
    
    // SSL/TLS configuration
    void setVerifySSL(bool verify);
    void setSSLCertificate(const String& certPath);
    void setSSLKey(const String& keyPath);
    
    // Progress tracking
    void setProgressCallback(HTTPProgressCallback callback);
    
    // Request cancellation
    void cancelAllRequests();
    void cancelRequest(const String& requestId);
    
private:
    // Implementation details
    struct Impl;
    std::unique_ptr<Impl> pImpl;
    
    // Helper methods
    HTTPResponse executeRequest(const HTTPRequest& request);
    void executeRequestAsync(const HTTPRequest& request, HTTPResponseCallback callback);
    String buildRequestString(const HTTPRequest& request);
    void parseResponseHeaders(const String& headerString, HTTPResponse& response);
    bool isValidUrl(const String& url) const;
    String urlEncode(const String& text) const;
};

// HTTP utilities
namespace http_utils {
    // URL parsing
    struct URL {
        String scheme;
        String host;
        int port;
        String path;
        String query;
        String fragment;
        
        URL() : port(0) {}
    };
    
    URL parseUrl(const String& url);
    String buildUrl(const URL& url);
    String getDomain(const String& url);
    String getPath(const String& url);
    String getQuery(const String& url);
    
    // Header utilities
    String getHeaderValue(const std::map<String, String>& headers, const String& name);
    void setHeaderValue(std::map<String, String>& headers, const String& name, const String& value);
    void removeHeader(std::map<String, String>& headers, const String& name);
    
    // Content type utilities
    String getContentType(const String& filename);
    String getMimeType(const String& extension);
    bool isTextContent(const String& contentType);
    bool isImageContent(const String& contentType);
    bool isVideoContent(const String& contentType);
    bool isAudioContent(const String& contentType);
    
    // Encoding utilities
    String urlEncode(const String& text);
    String urlDecode(const String& text);
    String base64Encode(const String& text);
    String base64Decode(const String& text);
    
    // Response utilities
    bool isSuccessStatus(int statusCode);
    bool isRedirectStatus(int statusCode);
    bool isClientErrorStatus(int statusCode);
    bool isServerErrorStatus(int statusCode);
    String getStatusText(int statusCode);
    
    // Request utilities
    String getMethodString(HTTPMethod method);
    HTTPMethod getMethodFromString(const String& method);
    String buildQueryString(const std::map<String, String>& params);
    std::map<String, String> parseQueryString(const String& query);
    
    // Cookie utilities
    struct Cookie {
        String name;
        String value;
        String domain;
        String path;
        String expires;
        bool secure;
        bool httpOnly;
        
        Cookie() : secure(false), httpOnly(false) {}
    };
    
    String serializeCookie(const Cookie& cookie);
    Cookie parseCookie(const String& cookieString);
    std::vector<Cookie> parseCookieHeader(const String& header);
    String buildCookieHeader(const std::vector<Cookie>& cookies);
    
    // Security utilities
    bool isValidDomain(const String& domain);
    bool isSecureProtocol(const String& scheme);
    String sanitizeUrl(const String& url);
    bool isBlockedUrl(const String& url);
    
    // Network utilities
    String getLocalIP();
    String getPublicIP();
    bool isOnline();
    int getNetworkLatency(const String& host);
    
    // Download utilities
    bool downloadFile(const String& url, const String& filename, HTTPProgressCallback progress = nullptr);
    String downloadString(const String& url);
    std::vector<uint8_t> downloadBytes(const String& url);
    
    // Upload utilities
    HTTPResponse uploadFile(const String& url, const String& filename, const String& fieldName = "file");
    HTTPResponse uploadData(const String& url, const std::vector<uint8_t>& data, const String& fieldName = "data");
}

// HTTP session manager
class HTTPSession {
public:
    HTTPSession();
    ~HTTPSession();
    
    // Session management
    void start();
    void stop();
    bool isActive() const;
    
    // Request management
    HTTPResponse request(const HTTPRequest& request);
    void requestAsync(const HTTPRequest& request, HTTPResponseCallback callback);
    
    // Session configuration
    void setBaseUrl(const String& baseUrl);
    void setDefaultHeaders(const std::map<String, String>& headers);
    void setAuthentication(const String& username, const String& password);
    void setBearerToken(const String& token);
    
    // Session state
    void clearCookies();
    void clearHeaders();
    std::map<String, String> getCookies() const;
    std::map<String, String> getHeaders() const;
    
private:
    std::unique_ptr<HTTPClient> client;
    String baseUrl;
    std::map<String, String> defaultHeaders;
    String authToken;
    bool active;
};

} // namespace zepra 