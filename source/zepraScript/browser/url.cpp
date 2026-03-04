/**
 * @file url.cpp
 * @brief URL and URLSearchParams implementation (WHATWG URL Standard)
 * 
 * Implements URL parsing and manipulation following the WHATWG URL Standard.
 * Supports:
 * - Full URL parsing with all components
 * - Relative URL resolution
 * - Query string manipulation via URLSearchParams
 * - Percent encoding/decoding
 */

#include "browser/url.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>

namespace Zepra::Browser {

// =============================================================================
// URLSearchParams Implementation
// =============================================================================

URLSearchParams::URLSearchParams(const std::string& query) 
    : Object(Runtime::ObjectType::Ordinary) {
    
    std::string input = query;
    
    // Remove leading '?' if present
    if (!input.empty() && input[0] == '?') {
        input = input.substr(1);
    }
    
    // Parse key=value pairs
    size_t pos = 0;
    while (pos < input.size()) {
        size_t ampPos = input.find('&', pos);
        if (ampPos == std::string::npos) {
            ampPos = input.size();
        }
        
        std::string pair = input.substr(pos, ampPos - pos);
        
        if (!pair.empty()) {
            size_t eqPos = pair.find('=');
            std::string name, value;
            
            if (eqPos != std::string::npos) {
                name = URL::percentDecode(pair.substr(0, eqPos));
                value = URL::percentDecode(pair.substr(eqPos + 1));
            } else {
                name = URL::percentDecode(pair);
            }
            
            // Replace '+' with space
            std::replace(name.begin(), name.end(), '+', ' ');
            std::replace(value.begin(), value.end(), '+', ' ');
            
            params_.emplace_back(name, value);
        }
        
        pos = ampPos + 1;
    }
}

void URLSearchParams::append(const std::string& name, const std::string& value) {
    params_.emplace_back(name, value);
}

void URLSearchParams::deleteParam(const std::string& name) {
    params_.erase(
        std::remove_if(params_.begin(), params_.end(),
            [&name](const auto& p) { return p.first == name; }),
        params_.end());
}

std::optional<std::string> URLSearchParams::getParam(const std::string& name) const {
    for (const auto& p : params_) {
        if (p.first == name) {
            return p.second;
        }
    }
    return std::nullopt;
}

std::vector<std::string> URLSearchParams::getAll(const std::string& name) const {
    std::vector<std::string> result;
    for (const auto& p : params_) {
        if (p.first == name) {
            result.push_back(p.second);
        }
    }
    return result;
}

bool URLSearchParams::has(const std::string& name) const {
    for (const auto& p : params_) {
        if (p.first == name) {
            return true;
        }
    }
    return false;
}

void URLSearchParams::set(const std::string& name, const std::string& value) {
    bool found = false;
    for (auto& p : params_) {
        if (p.first == name) {
            if (!found) {
                p.second = value;
                found = true;
            }
        }
    }
    
    if (found) {
        // Remove duplicates
        bool seenFirst = false;
        params_.erase(
            std::remove_if(params_.begin(), params_.end(),
                [&name, &seenFirst](const auto& p) {
                    if (p.first == name) {
                        if (seenFirst) return true;
                        seenFirst = true;
                    }
                    return false;
                }),
            params_.end());
    } else {
        params_.emplace_back(name, value);
    }
}

void URLSearchParams::sort() {
    std::stable_sort(params_.begin(), params_.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });
}

std::string URLSearchParams::toString() const {
    std::ostringstream ss;
    bool first = true;
    
    for (const auto& p : params_) {
        if (!first) ss << '&';
        first = false;
        
        ss << URL::percentEncode(p.first) << '=' << URL::percentEncode(p.second);
    }
    
    return ss.str();
}

// =============================================================================
// URL Implementation
// =============================================================================

URL::URL(const std::string& url, const std::string& base) 
    : Object(Runtime::ObjectType::Ordinary) {
    valid_ = parse(url, base);
    searchParams_ = new URLSearchParams(search_);
}

std::string URL::href() const {
    std::ostringstream ss;
    
    ss << protocol_ << "//";
    
    if (!username_.empty()) {
        ss << username_;
        if (!password_.empty()) {
            ss << ':' << password_;
        }
        ss << '@';
    }
    
    ss << hostname_;
    
    if (!port_.empty()) {
        ss << ':' << port_;
    }
    
    ss << pathname_;
    
    if (!search_.empty()) {
        ss << search_;
    }
    
    if (!hash_.empty()) {
        ss << hash_;
    }
    
    return ss.str();
}

std::string URL::origin() const {
    std::ostringstream ss;
    ss << protocol_ << "//" << hostname_;
    if (!port_.empty()) {
        ss << ':' << port_;
    }
    return ss.str();
}

void URL::setHref(const std::string& value) {
    parse(value);
    updateSearchParams();
}

void URL::setProtocol(const std::string& value) {
    protocol_ = value;
    if (!protocol_.empty() && protocol_.back() != ':') {
        protocol_ += ':';
    }
}

void URL::setUsername(const std::string& value) {
    username_ = value;
}

void URL::setPassword(const std::string& value) {
    password_ = value;
}

void URL::setHost(const std::string& value) {
    size_t colonPos = value.find(':');
    if (colonPos != std::string::npos) {
        hostname_ = value.substr(0, colonPos);
        port_ = value.substr(colonPos + 1);
    } else {
        hostname_ = value;
        port_.clear();
    }
    host_ = value;
}

void URL::setHostname(const std::string& value) {
    hostname_ = value;
    if (port_.empty()) {
        host_ = hostname_;
    } else {
        host_ = hostname_ + ":" + port_;
    }
}

void URL::setPort(const std::string& value) {
    port_ = value;
    if (port_.empty()) {
        host_ = hostname_;
    } else {
        host_ = hostname_ + ":" + port_;
    }
}

void URL::setPathname(const std::string& value) {
    pathname_ = value;
    if (pathname_.empty()) {
        pathname_ = "/";
    } else if (pathname_[0] != '/') {
        pathname_ = "/" + pathname_;
    }
}

void URL::setSearch(const std::string& value) {
    search_ = value;
    if (!search_.empty() && search_[0] != '?') {
        search_ = "?" + search_;
    }
    updateSearchParams();
}

void URL::setHash(const std::string& value) {
    hash_ = value;
    if (!hash_.empty() && hash_[0] != '#') {
        hash_ = "#" + hash_;
    }
}

bool URL::canParse(const std::string& url, const std::string& base) {
    URL testUrl(url, base);
    return testUrl.isValid();
}

void URL::updateSearchParams() {
    if (searchParams_) {
        delete searchParams_;
    }
    searchParams_ = new URLSearchParams(search_);
}

bool URL::parse(const std::string& input, const std::string& base) {
    std::string url = input;
    
    // Trim whitespace
    while (!url.empty() && std::isspace(static_cast<unsigned char>(url.front()))) {
        url.erase(0, 1);
    }
    while (!url.empty() && std::isspace(static_cast<unsigned char>(url.back()))) {
        url.pop_back();
    }
    
    if (url.empty()) {
        return false;
    }
    
    size_t pos = 0;
    
    // Parse scheme (protocol)
    size_t schemeEnd = url.find("://");
    if (schemeEnd != std::string::npos) {
        protocol_ = url.substr(0, schemeEnd + 1);  // Include ':'
        pos = schemeEnd + 3;  // Skip '://'
    } else if (!base.empty()) {
        // Relative URL - use base
        URL baseUrl(base);
        if (!baseUrl.isValid()) return false;
        
        protocol_ = baseUrl.protocol();
        username_ = baseUrl.username();
        password_ = baseUrl.password();
        hostname_ = baseUrl.hostname();
        port_ = baseUrl.port();
        
        if (url.empty() || url[0] == '?') {
            pathname_ = baseUrl.pathname();
            search_ = url;
            return true;
        } else if (url[0] == '#') {
            pathname_ = baseUrl.pathname();
            search_ = baseUrl.search();
            hash_ = url;
            return true;
        } else if (url[0] != '/') {
            // Relative path
            std::string basePath = baseUrl.pathname();
            size_t lastSlash = basePath.rfind('/');
            if (lastSlash != std::string::npos) {
                pathname_ = basePath.substr(0, lastSlash + 1) + url;
            } else {
                pathname_ = "/" + url;
            }
            
            // Parse search and hash from pathname
            size_t hashPos = pathname_.find('#');
            if (hashPos != std::string::npos) {
                hash_ = pathname_.substr(hashPos);
                pathname_ = pathname_.substr(0, hashPos);
            }
            
            size_t searchPos = pathname_.find('?');
            if (searchPos != std::string::npos) {
                search_ = pathname_.substr(searchPos);
                pathname_ = pathname_.substr(0, searchPos);
            }
            
            return true;
        }
        pos = 0;
    } else {
        return false;  // No scheme and no base
    }
    
    // Parse authority (userinfo@host:port)
    size_t pathStart = url.find('/', pos);
    size_t searchStart = url.find('?', pos);
    size_t hashStart = url.find('#', pos);
    
    size_t authorityEnd = std::min({
        pathStart != std::string::npos ? pathStart : url.size(),
        searchStart != std::string::npos ? searchStart : url.size(),
        hashStart != std::string::npos ? hashStart : url.size()
    });
    
    std::string authority = url.substr(pos, authorityEnd - pos);
    
    // Check for userinfo@
    size_t atPos = authority.find('@');
    if (atPos != std::string::npos) {
        std::string userinfo = authority.substr(0, atPos);
        size_t colonPos = userinfo.find(':');
        if (colonPos != std::string::npos) {
            username_ = userinfo.substr(0, colonPos);
            password_ = userinfo.substr(colonPos + 1);
        } else {
            username_ = userinfo;
        }
        authority = authority.substr(atPos + 1);
    }
    
    // Parse host:port
    size_t colonPos = authority.rfind(':');
    if (colonPos != std::string::npos) {
        // Check if it's an IPv6 address
        if (authority.find('[') == std::string::npos || 
            authority.find(']') < colonPos) {
            hostname_ = authority.substr(0, colonPos);
            port_ = authority.substr(colonPos + 1);
        } else {
            hostname_ = authority;
        }
    } else {
        hostname_ = authority;
    }
    
    host_ = port_.empty() ? hostname_ : hostname_ + ":" + port_;
    
    pos = authorityEnd;
    
    // Parse pathname
    if (pathStart != std::string::npos && pos == pathStart) {
        size_t pathEnd = std::min({
            searchStart != std::string::npos ? searchStart : url.size(),
            hashStart != std::string::npos ? hashStart : url.size()
        });
        pathname_ = url.substr(pathStart, pathEnd - pathStart);
        pos = pathEnd;
    } else {
        pathname_ = "/";
    }
    
    // Parse search (query string)
    if (searchStart != std::string::npos && pos == searchStart) {
        size_t searchEnd = hashStart != std::string::npos ? hashStart : url.size();
        search_ = url.substr(searchStart, searchEnd - searchStart);
        pos = searchEnd;
    }
    
    // Parse hash (fragment)
    if (hashStart != std::string::npos && pos == hashStart) {
        hash_ = url.substr(hashStart);
    }
    
    return !hostname_.empty();
}

std::string URL::percentEncode(const std::string& str) {
    std::ostringstream ss;
    
    for (unsigned char c : str) {
        if ((c >= 'A' && c <= 'Z') || 
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            ss << c;
        } else {
            ss << '%' << std::uppercase << std::hex << std::setw(2) 
               << std::setfill('0') << static_cast<int>(c);
        }
    }
    
    return ss.str();
}

std::string URL::percentDecode(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            std::string hex = str.substr(i + 1, 2);
            try {
                int value = std::stoi(hex, nullptr, 16);
                result += static_cast<char>(value);
                i += 2;
            } catch (...) {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    
    return result;
}

} // namespace Zepra::Browser
