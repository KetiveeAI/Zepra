/**
 * @file http_request.cpp
 * @brief HTTP request implementation
 */

#include "networking/http_request.hpp"

#include <algorithm>
#include <sstream>

namespace Zepra::Networking {

HttpRequest::HttpRequest(const std::string& url) : url_(url) {}

HttpRequest::HttpRequest(HttpMethod method, const std::string& url)
    : url_(url), method_(method) {}

void HttpRequest::setUrl(const std::string& url) {
    url_ = url;
}

std::string HttpRequest::methodString() const {
    switch (method_) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::PATCH: return "PATCH";
        case HttpMethod::HEAD: return "HEAD";
        case HttpMethod::OPTIONS: return "OPTIONS";
    }
    return "GET";
}

void HttpRequest::setHeader(const std::string& name, const std::string& value) {
    headers_[name] = value;
}

void HttpRequest::addHeader(const std::string& name, const std::string& value) {
    auto it = headers_.find(name);
    if (it != headers_.end()) {
        it->second += ", " + value;
    } else {
        headers_[name] = value;
    }
}

std::string HttpRequest::header(const std::string& name) const {
    auto it = headers_.find(name);
    return (it != headers_.end()) ? it->second : "";
}

bool HttpRequest::hasHeader(const std::string& name) const {
    return headers_.find(name) != headers_.end();
}

void HttpRequest::setBody(const std::string& body) {
    body_.assign(body.begin(), body.end());
}

void HttpRequest::setBody(const std::vector<uint8_t>& body) {
    body_ = body;
}

std::string HttpRequest::bodyString() const {
    return std::string(body_.begin(), body_.end());
}

void HttpRequest::setContentType(const std::string& type) {
    setHeader("Content-Type", type);
}

std::string HttpRequest::contentType() const {
    return header("Content-Type");
}

std::string HttpRequest::scheme() const {
    size_t pos = url_.find("://");
    if (pos != std::string::npos) {
        return url_.substr(0, pos);
    }
    return "http";
}

std::string HttpRequest::host() const {
    size_t start = url_.find("://");
    if (start == std::string::npos) {
        start = 0;
    } else {
        start += 3;
    }
    
    size_t end = url_.find('/', start);
    std::string hostPort = (end != std::string::npos) 
        ? url_.substr(start, end - start) 
        : url_.substr(start);
    
    // Remove port
    size_t portPos = hostPort.find(':');
    if (portPos != std::string::npos) {
        return hostPort.substr(0, portPos);
    }
    return hostPort;
}

int HttpRequest::port() const {
    size_t start = url_.find("://");
    if (start == std::string::npos) {
        start = 0;
    } else {
        start += 3;
    }
    
    size_t end = url_.find('/', start);
    std::string hostPort = (end != std::string::npos) 
        ? url_.substr(start, end - start) 
        : url_.substr(start);
    
    size_t portPos = hostPort.find(':');
    if (portPos != std::string::npos) {
        try {
            return std::stoi(hostPort.substr(portPos + 1));
        } catch (...) {}
    }
    
    return isSecure() ? 443 : 80;
}

std::string HttpRequest::path() const {
    size_t start = url_.find("://");
    if (start == std::string::npos) {
        start = 0;
    } else {
        start += 3;
    }
    
    size_t pathStart = url_.find('/', start);
    if (pathStart == std::string::npos) {
        return "/";
    }
    
    size_t queryStart = url_.find('?', pathStart);
    if (queryStart != std::string::npos) {
        return url_.substr(pathStart, queryStart - pathStart);
    }
    return url_.substr(pathStart);
}

std::string HttpRequest::query() const {
    size_t pos = url_.find('?');
    if (pos != std::string::npos) {
        return url_.substr(pos + 1);
    }
    return "";
}

bool HttpRequest::isSecure() const {
    std::string s = scheme();
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s == "https";
}

} // namespace Zepra::Networking
