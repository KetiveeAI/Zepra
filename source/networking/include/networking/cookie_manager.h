#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>

namespace ZepraNet {

struct Cookie {
    std::string name;
    std::string value;
    std::string domain;
    std::string path;
    std::time_t expires = 0;
    bool secure = false;
    bool httpOnly = false;
};

class CookieManager {
public:
    CookieManager();
    ~CookieManager();

    // Set a cookie for a domain (first-party only)
    void setCookie(const Cookie& cookie);

    // Get all cookies for a domain
    std::vector<Cookie> getCookies(const std::string& domain) const;

    // Delete a cookie by name for a domain
    void deleteCookie(const std::string& domain, const std::string& name);

    // Delete all cookies for a domain
    void deleteAllCookies(const std::string& domain);

    // Delete all cookies (global)
    void clearAllCookies();

    // Load/save cookies to disk
    void loadFromFile(const std::string& filename);
    void saveToFile(const std::string& filename) const;

private:
    // domain -> vector of cookies
    std::unordered_map<std::string, std::vector<Cookie>> m_cookies;

    // Helper to enforce first-party only
    bool isFirstParty(const Cookie& cookie) const;
};

} // namespace ZepraNet 