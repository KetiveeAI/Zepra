#pragma once

/**
 * @file url.hpp
 * @brief URL and URLSearchParams classes (WHATWG URL Standard)
 */

#include "../config.hpp"
#include "../runtime/object.hpp"
#include "../runtime/value.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace Zepra::Browser {

using Runtime::Value;
using Runtime::Object;

/**
 * @brief URLSearchParams - Query string manipulation
 */
class URLSearchParams : public Object {
public:
    URLSearchParams() : Object(Runtime::ObjectType::Ordinary) {}
    
    explicit URLSearchParams(const std::string& query);
    
    void append(const std::string& name, const std::string& value);
    void deleteParam(const std::string& name);
    std::optional<std::string> getParam(const std::string& name) const;
    std::vector<std::string> getAll(const std::string& name) const;
    bool has(const std::string& name) const;
    void set(const std::string& name, const std::string& value);
    void sort();
    
    std::string toString() const;
    
    // Iterator support
    const std::vector<std::pair<std::string, std::string>>& entries() const { return params_; }
    
private:
    std::vector<std::pair<std::string, std::string>> params_;
};

/**
 * @brief URL - WHATWG URL Standard implementation
 */
class URL : public Object {
public:
    URL() : Object(Runtime::ObjectType::Ordinary) {}
    
    /**
     * @brief Parse URL string
     * @param url URL to parse
     * @param base Optional base URL for relative URLs
     */
    explicit URL(const std::string& url, const std::string& base = "");
    
    // Accessors
    std::string href() const;
    std::string origin() const;
    const std::string& protocol() const { return protocol_; }
    const std::string& username() const { return username_; }
    const std::string& password() const { return password_; }
    const std::string& host() const { return host_; }
    const std::string& hostname() const { return hostname_; }
    const std::string& port() const { return port_; }
    const std::string& pathname() const { return pathname_; }
    const std::string& search() const { return search_; }
    const std::string& hash() const { return hash_; }
    
    URLSearchParams* searchParams() const { return searchParams_; }
    
    // Mutators
    void setHref(const std::string& value);
    void setProtocol(const std::string& value);
    void setUsername(const std::string& value);
    void setPassword(const std::string& value);
    void setHost(const std::string& value);
    void setHostname(const std::string& value);
    void setPort(const std::string& value);
    void setPathname(const std::string& value);
    void setSearch(const std::string& value);
    void setHash(const std::string& value);
    
    std::string toString() const { return href(); }
    std::string toJSON() const { return href(); }
    
    // Static methods
    static bool canParse(const std::string& url, const std::string& base = "");
    
    // Validation
    bool isValid() const { return valid_; }
    
    // Static helper methods (public for URLSearchParams)
    static std::string percentEncode(const std::string& str);
    static std::string percentDecode(const std::string& str);
    
private:
    bool parse(const std::string& input, const std::string& base = "");
    void updateSearchParams();
    
    std::string protocol_;    // e.g., "https:"
    std::string username_;
    std::string password_;
    std::string host_;        // hostname:port
    std::string hostname_;    // e.g., "example.com"
    std::string port_;        // e.g., "8080"
    std::string pathname_;    // e.g., "/path/to/resource"
    std::string search_;      // e.g., "?key=value"
    std::string hash_;        // e.g., "#section"
    
    URLSearchParams* searchParams_ = nullptr;
    bool valid_ = false;
};

} // namespace Zepra::Browser
