/**
 * @file URLAPI.h
 * @brief URL and URLSearchParams API
 * 
 * WHATWG URL Standard:
 * - URL: Parse, construct, manipulate URLs
 * - URLSearchParams: Query string handling
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <stdexcept>

namespace Zepra::API {

// =============================================================================
// URLSearchParams
// =============================================================================

/**
 * @brief Query string parameters
 */
class URLSearchParams {
public:
    URLSearchParams() = default;
    
    // From query string
    explicit URLSearchParams(const std::string& query) {
        parse(query);
    }
    
    // From pairs
    explicit URLSearchParams(std::initializer_list<std::pair<std::string, std::string>> init) {
        for (const auto& [key, value] : init) {
            append(key, value);
        }
    }
    
    // Append (allows duplicates)
    void append(const std::string& name, const std::string& value) {
        params_.push_back({name, value});
    }
    
    // Set (replaces all existing)
    void set(const std::string& name, const std::string& value) {
        deleteAll(name);
        append(name, value);
    }
    
    // Get first value
    std::optional<std::string> get(const std::string& name) const {
        for (const auto& [k, v] : params_) {
            if (k == name) return v;
        }
        return std::nullopt;
    }
    
    // Get all values
    std::vector<std::string> getAll(const std::string& name) const {
        std::vector<std::string> result;
        for (const auto& [k, v] : params_) {
            if (k == name) result.push_back(v);
        }
        return result;
    }
    
    // Has parameter
    bool has(const std::string& name) const {
        for (const auto& [k, _] : params_) {
            if (k == name) return true;
        }
        return false;
    }
    
    // Delete all with name
    void deleteAll(const std::string& name) {
        params_.erase(
            std::remove_if(params_.begin(), params_.end(),
                [&name](const auto& p) { return p.first == name; }),
            params_.end());
    }
    
    // Sort by key
    void sort() {
        std::stable_sort(params_.begin(), params_.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
    }
    
    // Serialize
    std::string toString() const {
        std::string result;
        for (size_t i = 0; i < params_.size(); i++) {
            if (i > 0) result += "&";
            result += encode(params_[i].first) + "=" + encode(params_[i].second);
        }
        return result;
    }
    
    // Iteration
    const std::vector<std::pair<std::string, std::string>>& entries() const {
        return params_;
    }
    
    std::vector<std::string> keys() const {
        std::vector<std::string> result;
        for (const auto& [k, _] : params_) {
            result.push_back(k);
        }
        return result;
    }
    
    std::vector<std::string> values() const {
        std::vector<std::string> result;
        for (const auto& [_, v] : params_) {
            result.push_back(v);
        }
        return result;
    }
    
private:
    void parse(const std::string& query) {
        std::string q = query;
        if (!q.empty() && q[0] == '?') {
            q = q.substr(1);
        }
        
        size_t pos = 0;
        while (pos < q.length()) {
            size_t end = q.find('&', pos);
            if (end == std::string::npos) end = q.length();
            
            std::string pair = q.substr(pos, end - pos);
            size_t eq = pair.find('=');
            
            if (eq != std::string::npos) {
                std::string name = decode(pair.substr(0, eq));
                std::string value = decode(pair.substr(eq + 1));
                params_.push_back({name, value});
            } else if (!pair.empty()) {
                params_.push_back({decode(pair), ""});
            }
            
            pos = end + 1;
        }
    }
    
    static std::string encode(const std::string& str) {
        std::string result;
        for (char c : str) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                result += c;
            } else if (c == ' ') {
                result += '+';
            } else {
                char buf[4];
                snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(c));
                result += buf;
            }
        }
        return result;
    }
    
    static std::string decode(const std::string& str) {
        std::string result;
        for (size_t i = 0; i < str.length(); i++) {
            if (str[i] == '%' && i + 2 < str.length()) {
                int val = 0;
                if (sscanf(str.c_str() + i + 1, "%2x", &val) == 1) {
                    result += static_cast<char>(val);
                    i += 2;
                } else {
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
    
    std::vector<std::pair<std::string, std::string>> params_;
};

// =============================================================================
// URL
// =============================================================================

/**
 * @brief WHATWG URL interface
 */
class URL {
public:
    // Parse URL
    explicit URL(const std::string& url, const std::string& base = "") {
        parse(url, base);
    }
    
    // Components
    std::string href() const { return toString(); }
    void setHref(const std::string& url) { parse(url, ""); }
    
    const std::string& protocol() const { return protocol_; }
    void setProtocol(const std::string& p) { 
        protocol_ = p;
        if (!protocol_.empty() && protocol_.back() != ':') {
            protocol_ += ':';
        }
    }
    
    std::string host() const {
        if (port_.empty()) return hostname_;
        return hostname_ + ":" + port_;
    }
    void setHost(const std::string& h) {
        size_t colon = h.find(':');
        if (colon != std::string::npos) {
            hostname_ = h.substr(0, colon);
            port_ = h.substr(colon + 1);
        } else {
            hostname_ = h;
            port_.clear();
        }
    }
    
    const std::string& hostname() const { return hostname_; }
    void setHostname(const std::string& h) { hostname_ = h; }
    
    const std::string& port() const { return port_; }
    void setPort(const std::string& p) { port_ = p; }
    
    const std::string& pathname() const { return pathname_; }
    void setPathname(const std::string& p) { 
        pathname_ = p;
        if (pathname_.empty() || pathname_[0] != '/') {
            pathname_ = "/" + pathname_;
        }
    }
    
    std::string search() const {
        std::string s = searchParams_.toString();
        return s.empty() ? "" : "?" + s;
    }
    void setSearch(const std::string& s) {
        searchParams_ = URLSearchParams(s);
    }
    
    URLSearchParams& searchParams() { return searchParams_; }
    const URLSearchParams& searchParams() const { return searchParams_; }
    
    const std::string& hash() const { return hash_; }
    void setHash(const std::string& h) {
        hash_ = h;
        if (!hash_.empty() && hash_[0] != '#') {
            hash_ = "#" + hash_;
        }
    }
    
    std::string origin() const {
        return protocol_ + "//" + host();
    }
    
    const std::string& username() const { return username_; }
    void setUsername(const std::string& u) { username_ = u; }
    
    const std::string& password() const { return password_; }
    void setPassword(const std::string& p) { password_ = p; }
    
    // Serialize
    std::string toString() const {
        std::string result = protocol_ + "//";
        
        if (!username_.empty()) {
            result += username_;
            if (!password_.empty()) {
                result += ":" + password_;
            }
            result += "@";
        }
        
        result += hostname_;
        if (!port_.empty()) {
            result += ":" + port_;
        }
        
        result += pathname_;
        
        std::string s = searchParams_.toString();
        if (!s.empty()) {
            result += "?" + s;
        }
        
        result += hash_;
        
        return result;
    }
    
    std::string toJSON() const { return toString(); }
    
    // Static
    static bool canParse(const std::string& url, const std::string& base = "") {
        try {
            URL u(url, base);
            return true;
        } catch (...) {
            return false;
        }
    }
    
private:
    void parse(const std::string& url, const std::string& base) {
        std::string input = url;
        
        // Protocol
        size_t protoEnd = input.find("://");
        if (protoEnd != std::string::npos) {
            protocol_ = input.substr(0, protoEnd + 1);
            input = input.substr(protoEnd + 3);
        } else if (!base.empty()) {
            URL baseUrl(base);
            protocol_ = baseUrl.protocol_;
            hostname_ = baseUrl.hostname_;
            port_ = baseUrl.port_;
        } else {
            throw std::runtime_error("Invalid URL");
        }
        
        // Hash
        size_t hashPos = input.find('#');
        if (hashPos != std::string::npos) {
            hash_ = input.substr(hashPos);
            input = input.substr(0, hashPos);
        }
        
        // Search
        size_t queryPos = input.find('?');
        if (queryPos != std::string::npos) {
            searchParams_ = URLSearchParams(input.substr(queryPos + 1));
            input = input.substr(0, queryPos);
        }
        
        // Path
        size_t pathPos = input.find('/');
        if (pathPos != std::string::npos) {
            pathname_ = input.substr(pathPos);
            input = input.substr(0, pathPos);
        } else {
            pathname_ = "/";
        }
        
        // Auth
        size_t atPos = input.find('@');
        if (atPos != std::string::npos) {
            std::string auth = input.substr(0, atPos);
            size_t colonPos = auth.find(':');
            if (colonPos != std::string::npos) {
                username_ = auth.substr(0, colonPos);
                password_ = auth.substr(colonPos + 1);
            } else {
                username_ = auth;
            }
            input = input.substr(atPos + 1);
        }
        
        // Host:port
        size_t portPos = input.find(':');
        if (portPos != std::string::npos) {
            hostname_ = input.substr(0, portPos);
            port_ = input.substr(portPos + 1);
        } else {
            hostname_ = input;
        }
    }
    
    std::string protocol_;
    std::string hostname_;
    std::string port_;
    std::string pathname_ = "/";
    URLSearchParams searchParams_;
    std::string hash_;
    std::string username_;
    std::string password_;
};

} // namespace Zepra::API
