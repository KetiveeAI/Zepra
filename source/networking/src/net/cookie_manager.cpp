#include "net/cookie_manager.h"
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace ZepraNet {

CookieManager::CookieManager() {}
CookieManager::~CookieManager() {}

void CookieManager::setCookie(const Cookie& cookie) {
    if (!isFirstParty(cookie)) return;
    auto& cookies = m_cookies[cookie.domain];
    auto it = std::find_if(cookies.begin(), cookies.end(), [&](const Cookie& c) {
        return c.name == cookie.name && c.path == cookie.path;
    });
    if (it != cookies.end()) {
        *it = cookie;
    } else {
        cookies.push_back(cookie);
    }
}

std::vector<Cookie> CookieManager::getCookies(const std::string& domain) const {
    auto it = m_cookies.find(domain);
    if (it != m_cookies.end()) {
        return it->second;
    }
    return {};
}

void CookieManager::deleteCookie(const std::string& domain, const std::string& name) {
    auto it = m_cookies.find(domain);
    if (it != m_cookies.end()) {
        auto& cookies = it->second;
        cookies.erase(std::remove_if(cookies.begin(), cookies.end(), [&](const Cookie& c) {
            return c.name == name;
        }), cookies.end());
    }
}

void CookieManager::deleteAllCookies(const std::string& domain) {
    m_cookies.erase(domain);
}

void CookieManager::clearAllCookies() {
    m_cookies.clear();
}

void CookieManager::loadFromFile(const std::string& filename) {
    std::ifstream in(filename);
    if (!in) return;
    nlohmann::json j;
    in >> j;
    m_cookies.clear();
    for (auto& [domain, arr] : j.items()) {
        for (auto& c : arr) {
            Cookie cookie;
            cookie.name = c["name"];
            cookie.value = c["value"];
            cookie.domain = c["domain"];
            cookie.path = c["path"];
            cookie.expires = c["expires"];
            cookie.secure = c["secure"];
            cookie.httpOnly = c["httpOnly"];
            m_cookies[domain].push_back(cookie);
        }
    }
}

void CookieManager::saveToFile(const std::string& filename) const {
    nlohmann::json j;
    for (const auto& [domain, cookies] : m_cookies) {
        for (const auto& c : cookies) {
            j[domain].push_back({
                {"name", c.name},
                {"value", c.value},
                {"domain", c.domain},
                {"path", c.path},
                {"expires", c.expires},
                {"secure", c.secure},
                {"httpOnly", c.httpOnly}
            });
        }
    }
    std::ofstream out(filename);
    out << j.dump(2);
}

bool CookieManager::isFirstParty(const Cookie& cookie) const {
    // Only allow cookies if the domain matches exactly or is a subdomain of the current site
    // For now, assume current site domain is available as a global or passed in (to be improved)
    extern std::string g_currentSiteDomain; // This should be set by the browser when loading a site
    if (cookie.domain.empty() || g_currentSiteDomain.empty()) return false;
    // Exact match
    if (cookie.domain == g_currentSiteDomain) return true;
    // Subdomain match (e.g., .example.com matches www.example.com)
    if (cookie.domain[0] == '.') {
        std::string sub = g_currentSiteDomain;
        if (sub.length() > cookie.domain.length() - 1) {
            if (sub.compare(sub.length() - (cookie.domain.length() - 1), cookie.domain.length() - 1, cookie.domain.substr(1)) == 0) {
                return true;
            }
        }
    }
    return false;
}

} // namespace ZepraNet 