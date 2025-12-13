/**
 * @file http_response.cpp
 * @brief HTTP response implementation
 */

#include "networking/http_response.hpp"

#include <algorithm>
#include <sstream>

namespace Zepra::Networking {

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
    // Normalize header name to lowercase for consistent lookup
    std::string normalizedName = name;
    std::transform(normalizedName.begin(), normalizedName.end(), 
                   normalizedName.begin(), ::tolower);
    headers_[normalizedName] = value;
}

std::string HttpResponse::header(const std::string& name) const {
    std::string normalizedName = name;
    std::transform(normalizedName.begin(), normalizedName.end(), 
                   normalizedName.begin(), ::tolower);
    
    auto it = headers_.find(normalizedName);
    return (it != headers_.end()) ? it->second : "";
}

bool HttpResponse::hasHeader(const std::string& name) const {
    std::string normalizedName = name;
    std::transform(normalizedName.begin(), normalizedName.end(), 
                   normalizedName.begin(), ::tolower);
    return headers_.find(normalizedName) != headers_.end();
}

void HttpResponse::appendBody(const uint8_t* data, size_t size) {
    body_.insert(body_.end(), data, data + size);
}

std::string HttpResponse::bodyString() const {
    return std::string(body_.begin(), body_.end());
}

size_t HttpResponse::contentLength() const {
    std::string cl = header("Content-Length");
    if (!cl.empty()) {
        try {
            return std::stoull(cl);
        } catch (...) {}
    }
    return body_.size();
}

std::string HttpResponse::contentType() const {
    return header("Content-Type");
}

std::string HttpResponse::mimeType() const {
    std::string ct = contentType();
    size_t semicolon = ct.find(';');
    if (semicolon != std::string::npos) {
        ct = ct.substr(0, semicolon);
    }
    // Trim whitespace
    size_t start = ct.find_first_not_of(" \t");
    size_t end = ct.find_last_not_of(" \t");
    if (start != std::string::npos && end != std::string::npos) {
        return ct.substr(start, end - start + 1);
    }
    return ct;
}

std::string HttpResponse::charset() const {
    std::string ct = contentType();
    size_t charsetPos = ct.find("charset=");
    if (charsetPos != std::string::npos) {
        std::string charset = ct.substr(charsetPos + 8);
        size_t end = charset.find_first_of("; \t");
        if (end != std::string::npos) {
            charset = charset.substr(0, end);
        }
        // Remove quotes
        if (charset.size() >= 2 && charset.front() == '"' && charset.back() == '"') {
            charset = charset.substr(1, charset.size() - 2);
        }
        return charset;
    }
    return "utf-8";  // Default
}

std::string HttpResponse::location() const {
    return header("Location");
}

std::vector<std::string> HttpResponse::setCookieHeaders() const {
    std::vector<std::string> cookies;
    
    // Look for all Set-Cookie headers
    // In our implementation, multiple Set-Cookie headers are joined with \n
    std::string setCookie = header("Set-Cookie");
    if (!setCookie.empty()) {
        std::istringstream iss(setCookie);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty()) {
                cookies.push_back(line);
            }
        }
    }
    
    return cookies;
}

} // namespace Zepra::Networking
