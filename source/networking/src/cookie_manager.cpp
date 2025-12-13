/**
 * @file cookie_manager.cpp
 * @brief HTTP Cookie management implementation
 */

#include "networking/cookie_manager.hpp"

#include <sstream>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <ctime>

namespace Zepra::Networking {

// =============================================================================
// Cookie Implementation
// =============================================================================

bool Cookie::isExpired() const {
    if (!persistent) return false;
    return std::chrono::system_clock::now() > expires;
}

bool Cookie::matches(const std::string& url, bool isSecure) const {
    if (secure && !isSecure) return false;
    
    // Extract domain and path from URL
    std::string domain;
    std::string path = "/";
    
    size_t start = url.find("://");
    if (start != std::string::npos) {
        start += 3;
        size_t end = url.find('/', start);
        if (end != std::string::npos) {
            domain = url.substr(start, end - start);
            path = url.substr(end);
        } else {
            domain = url.substr(start);
        }
    }
    
    // Remove port from domain
    size_t portPos = domain.find(':');
    if (portPos != std::string::npos) {
        domain = domain.substr(0, portPos);
    }
    
    return matchesDomain(domain) && matchesPath(path);
}

bool Cookie::matchesDomain(const std::string& requestDomain) const {
    if (domain.empty()) return false;
    
    std::string cookieDomain = domain;
    std::string reqDomain = requestDomain;
    
    // Normalize to lowercase
    std::transform(cookieDomain.begin(), cookieDomain.end(), 
                   cookieDomain.begin(), ::tolower);
    std::transform(reqDomain.begin(), reqDomain.end(), 
                   reqDomain.begin(), ::tolower);
    
    // Remove leading dot
    if (!cookieDomain.empty() && cookieDomain[0] == '.') {
        cookieDomain = cookieDomain.substr(1);
    }
    
    // Exact match
    if (reqDomain == cookieDomain) return true;
    
    // Domain suffix match
    if (reqDomain.length() > cookieDomain.length()) {
        size_t pos = reqDomain.length() - cookieDomain.length();
        if (reqDomain[pos - 1] == '.' && 
            reqDomain.substr(pos) == cookieDomain) {
            return true;
        }
    }
    
    return false;
}

bool Cookie::matchesPath(const std::string& requestPath) const {
    if (path.empty() || path == "/") return true;
    if (requestPath.empty()) return false;
    
    // Exact match
    if (requestPath == path) return true;
    
    // Prefix match
    if (requestPath.length() > path.length()) {
        if (requestPath.substr(0, path.length()) == path) {
            if (path.back() == '/' || requestPath[path.length()] == '/') {
                return true;
            }
        }
    }
    
    return false;
}

std::string Cookie::toSetCookieString() const {
    std::ostringstream oss;
    oss << name << "=" << value;
    
    if (!domain.empty()) {
        oss << "; Domain=" << domain;
    }
    if (!path.empty()) {
        oss << "; Path=" << path;
    }
    if (persistent) {
        auto time = std::chrono::system_clock::to_time_t(expires);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&time));
        oss << "; Expires=" << buf;
    }
    if (secure) {
        oss << "; Secure";
    }
    if (httpOnly) {
        oss << "; HttpOnly";
    }
    switch (sameSite) {
        case SameSite::Strict: oss << "; SameSite=Strict"; break;
        case SameSite::Lax: oss << "; SameSite=Lax"; break;
        case SameSite::None: oss << "; SameSite=None"; break;
    }
    
    return oss.str();
}

std::string Cookie::toCookieString() const {
    return name + "=" + value;
}

// =============================================================================
// Parse Cookie
// =============================================================================

Cookie parseCookie(const std::string& setCookieHeader, const std::string& requestUrl) {
    Cookie cookie;
    cookie.creationTime = std::chrono::system_clock::now();
    cookie.lastAccessTime = cookie.creationTime;
    
    // Extract domain from URL
    size_t start = requestUrl.find("://");
    if (start != std::string::npos) {
        start += 3;
        size_t end = requestUrl.find('/', start);
        if (end != std::string::npos) {
            cookie.domain = requestUrl.substr(start, end - start);
            cookie.path = requestUrl.substr(end);
            size_t pathEnd = cookie.path.rfind('/');
            if (pathEnd != std::string::npos) {
                cookie.path = cookie.path.substr(0, pathEnd + 1);
            }
        } else {
            cookie.domain = requestUrl.substr(start);
        }
    }
    
    // Remove port from domain
    size_t portPos = cookie.domain.find(':');
    if (portPos != std::string::npos) {
        cookie.domain = cookie.domain.substr(0, portPos);
    }
    
    // Parse header
    std::istringstream iss(setCookieHeader);
    std::string token;
    bool first = true;
    
    while (std::getline(iss, token, ';')) {
        // Trim whitespace
        size_t fstart = token.find_first_not_of(" \t");
        size_t fend = token.find_last_not_of(" \t");
        if (fstart == std::string::npos) continue;
        token = token.substr(fstart, fend - fstart + 1);
        
        size_t eq = token.find('=');
        std::string key = (eq != std::string::npos) ? token.substr(0, eq) : token;
        std::string val = (eq != std::string::npos) ? token.substr(eq + 1) : "";
        
        // Lowercase key for comparison
        std::string keyLower = key;
        std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), ::tolower);
        
        if (first) {
            cookie.name = key;
            cookie.value = val;
            first = false;
        }
        else if (keyLower == "domain") {
            cookie.domain = val;
        }
        else if (keyLower == "path") {
            cookie.path = val;
        }
        else if (keyLower == "expires") {
            struct tm tm = {};
            if (strptime(val.c_str(), "%a, %d %b %Y %H:%M:%S", &tm) ||
                strptime(val.c_str(), "%a, %d-%b-%Y %H:%M:%S", &tm)) {
                cookie.expires = std::chrono::system_clock::from_time_t(timegm(&tm));
                cookie.persistent = true;
            }
        }
        else if (keyLower == "max-age") {
            try {
                int seconds = std::stoi(val);
                cookie.expires = std::chrono::system_clock::now() + 
                                 std::chrono::seconds(seconds);
                cookie.persistent = true;
            } catch (...) {}
        }
        else if (keyLower == "secure") {
            cookie.secure = true;
        }
        else if (keyLower == "httponly") {
            cookie.httpOnly = true;
        }
        else if (keyLower == "samesite") {
            std::string valLower = val;
            std::transform(valLower.begin(), valLower.end(), valLower.begin(), ::tolower);
            if (valLower == "strict") cookie.sameSite = SameSite::Strict;
            else if (valLower == "lax") cookie.sameSite = SameSite::Lax;
            else if (valLower == "none") cookie.sameSite = SameSite::None;
        }
    }
    
    return cookie;
}

// =============================================================================
// FileCookieStore Implementation
// =============================================================================

FileCookieStore::FileCookieStore(const std::string& path) : path_(path) {}

bool FileCookieStore::save(const std::vector<Cookie>& cookies) {
    std::ofstream file(path_);
    if (!file.is_open()) return false;
    
    for (const auto& cookie : cookies) {
        if (cookie.isExpired()) continue;
        
        file << cookie.domain << "\t"
             << (cookie.domain[0] == '.' ? "TRUE" : "FALSE") << "\t"
             << cookie.path << "\t"
             << (cookie.secure ? "TRUE" : "FALSE") << "\t"
             << std::chrono::system_clock::to_time_t(cookie.expires) << "\t"
             << cookie.name << "\t"
             << cookie.value << "\n";
    }
    
    return true;
}

std::vector<Cookie> FileCookieStore::load() {
    std::vector<Cookie> cookies;
    
    std::ifstream file(path_);
    if (!file.is_open()) return cookies;
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string domain, domainFlag, path, secureFlag, expiresStr, name, value;
        
        if (std::getline(iss, domain, '\t') &&
            std::getline(iss, domainFlag, '\t') &&
            std::getline(iss, path, '\t') &&
            std::getline(iss, secureFlag, '\t') &&
            std::getline(iss, expiresStr, '\t') &&
            std::getline(iss, name, '\t') &&
            std::getline(iss, value)) {
            
            Cookie cookie;
            cookie.domain = domain;
            cookie.path = path;
            cookie.secure = (secureFlag == "TRUE");
            cookie.name = name;
            cookie.value = value;
            
            try {
                time_t expires = std::stol(expiresStr);
                cookie.expires = std::chrono::system_clock::from_time_t(expires);
                cookie.persistent = true;
            } catch (...) {}
            
            cookie.creationTime = std::chrono::system_clock::now();
            cookie.lastAccessTime = cookie.creationTime;
            
            if (!cookie.isExpired()) {
                cookies.push_back(cookie);
            }
        }
    }
    
    return cookies;
}

void FileCookieStore::clear() {
    std::ofstream file(path_, std::ios::trunc);
}

// =============================================================================
// CookieManager Implementation
// =============================================================================

CookieManager::CookieManager() {}

CookieManager::CookieManager(std::unique_ptr<CookieStore> store)
    : store_(std::move(store)) {
    loadFromDisk();
}

CookieManager::~CookieManager() {
    saveToDisk();
}

void CookieManager::setCookie(const std::string& setCookieHeader, 
                               const std::string& requestUrl) {
    Cookie cookie = parseCookie(setCookieHeader, requestUrl);
    setCookie(cookie);
}

void CookieManager::setCookie(const Cookie& cookie) {
    std::lock_guard<std::mutex> lock(mutex_);
    addCookie(cookie);
}

void CookieManager::addCookie(Cookie cookie) {
    // Validate
    if (cookie.name.empty() || cookie.domain.empty()) return;
    
    // Check limits
    auto& domainCookies = cookies_[cookie.domain];
    
    if (domainCookies.size() >= maxCookiesPerDomain_) {
        // Remove oldest cookie
        auto oldest = domainCookies.begin();
        for (auto it = domainCookies.begin(); it != domainCookies.end(); ++it) {
            if (it->second.lastAccessTime < oldest->second.lastAccessTime) {
                oldest = it;
            }
        }
        domainCookies.erase(oldest);
    }
    
    // Add or update
    std::string key = cookie.name + "|" + cookie.path;
    bool isNew = domainCookies.find(key) == domainCookies.end();
    
    domainCookies[key] = cookie;
    
    // Callback
    if (changeCallback_) {
        changeCallback_(cookie, false);
    }
}

std::string CookieManager::getCookiesForUrl(const std::string& url, bool isSecure) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Cookie*> matching;
    auto now = std::chrono::system_clock::now();
    
    for (auto& [domain, domainCookies] : cookies_) {
        for (auto& [key, cookie] : domainCookies) {
            if (cookie.isExpired()) continue;
            if (cookie.matches(url, isSecure)) {
                cookie.lastAccessTime = now;
                matching.push_back(&cookie);
            }
        }
    }
    
    // Sort by path length (longer paths first)
    std::sort(matching.begin(), matching.end(), [](Cookie* a, Cookie* b) {
        return a->path.length() > b->path.length();
    });
    
    // Build cookie header
    std::ostringstream oss;
    for (size_t i = 0; i < matching.size(); ++i) {
        if (i > 0) oss << "; ";
        oss << matching[i]->toCookieString();
    }
    
    return oss.str();
}

std::vector<Cookie> CookieManager::getCookiesForDomain(const std::string& domain) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Cookie> result;
    auto it = cookies_.find(domain);
    if (it != cookies_.end()) {
        for (auto& [key, cookie] : it->second) {
            if (!cookie.isExpired()) {
                result.push_back(cookie);
            }
        }
    }
    return result;
}

std::vector<Cookie> CookieManager::getAllCookies() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Cookie> result;
    for (auto& [domain, domainCookies] : cookies_) {
        for (auto& [key, cookie] : domainCookies) {
            if (!cookie.isExpired()) {
                result.push_back(cookie);
            }
        }
    }
    return result;
}

void CookieManager::deleteCookie(const std::string& domain, 
                                  const std::string& name,
                                  const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = cookies_.find(domain);
    if (it != cookies_.end()) {
        std::string key = name + "|" + path;
        auto cit = it->second.find(key);
        if (cit != it->second.end()) {
            Cookie removed = cit->second;
            it->second.erase(cit);
            if (changeCallback_) {
                changeCallback_(removed, true);
            }
        }
    }
}

void CookieManager::deleteCookiesForDomain(const std::string& domain) {
    std::lock_guard<std::mutex> lock(mutex_);
    cookies_.erase(domain);
}

void CookieManager::deleteAllCookies() {
    std::lock_guard<std::mutex> lock(mutex_);
    cookies_.clear();
}

void CookieManager::deleteExpiredCookies() {
    std::lock_guard<std::mutex> lock(mutex_);
    removeExpired();
}

void CookieManager::removeExpired() {
    for (auto& [domain, domainCookies] : cookies_) {
        for (auto it = domainCookies.begin(); it != domainCookies.end();) {
            if (it->second.isExpired()) {
                it = domainCookies.erase(it);
            } else {
                ++it;
            }
        }
    }
}

bool CookieManager::saveToDisk() {
    if (!store_) return false;
    return store_->save(getAllCookies());
}

bool CookieManager::loadFromDisk() {
    if (!store_) return false;
    
    std::vector<Cookie> loaded = store_->load();
    for (const auto& cookie : loaded) {
        addCookie(cookie);
    }
    return true;
}

size_t CookieManager::count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t total = 0;
    for (const auto& [domain, cookies] : cookies_) {
        total += cookies.size();
    }
    return total;
}

void CookieManager::setChangeCallback(CookieChangeCallback callback) {
    changeCallback_ = std::move(callback);
}

// =============================================================================
// Global Instance
// =============================================================================

CookieManager& getCookieManager() {
    static CookieManager instance;
    return instance;
}

} // namespace Zepra::Networking
