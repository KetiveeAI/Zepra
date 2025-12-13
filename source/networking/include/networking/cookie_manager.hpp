/**
 * @file cookie_manager.hpp
 * @brief HTTP Cookie management and storage
 * 
 * Implements RFC 6265 cookie handling with secure storage,
 * expiration, domain/path matching, and SameSite support.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <functional>

namespace Zepra::Networking {

/**
 * @brief SameSite cookie attribute
 */
enum class SameSite {
    None,
    Lax,
    Strict
};

/**
 * @brief HTTP Cookie
 */
struct Cookie {
    std::string name;
    std::string value;
    std::string domain;
    std::string path = "/";
    
    std::chrono::system_clock::time_point expires;
    bool persistent = false;  // Has explicit expiry
    
    bool secure = false;      // HTTPS only
    bool httpOnly = false;    // No JavaScript access
    SameSite sameSite = SameSite::Lax;
    
    std::chrono::system_clock::time_point creationTime;
    std::chrono::system_clock::time_point lastAccessTime;
    
    /**
     * @brief Check if cookie is expired
     */
    bool isExpired() const;
    
    /**
     * @brief Check if cookie matches URL
     */
    bool matches(const std::string& url, bool isSecure) const;
    
    /**
     * @brief Check if domain matches
     */
    bool matchesDomain(const std::string& requestDomain) const;
    
    /**
     * @brief Check if path matches
     */
    bool matchesPath(const std::string& requestPath) const;
    
    /**
     * @brief Serialize to Set-Cookie format
     */
    std::string toSetCookieString() const;
    
    /**
     * @brief Serialize to Cookie header format
     */
    std::string toCookieString() const;
};

/**
 * @brief Parse Set-Cookie header
 */
Cookie parseCookie(const std::string& setCookieHeader, const std::string& requestUrl);

/**
 * @brief Cookie storage backend interface
 */
class CookieStore {
public:
    virtual ~CookieStore() = default;
    
    virtual bool save(const std::vector<Cookie>& cookies) = 0;
    virtual std::vector<Cookie> load() = 0;
    virtual void clear() = 0;
};

/**
 * @brief File-based cookie storage
 */
class FileCookieStore : public CookieStore {
public:
    explicit FileCookieStore(const std::string& path);
    
    bool save(const std::vector<Cookie>& cookies) override;
    std::vector<Cookie> load() override;
    void clear() override;
    
private:
    std::string path_;
};

/**
 * @brief SQLite-based cookie storage
 */
class SQLiteCookieStore : public CookieStore {
public:
    explicit SQLiteCookieStore(const std::string& dbPath);
    ~SQLiteCookieStore();
    
    bool save(const std::vector<Cookie>& cookies) override;
    std::vector<Cookie> load() override;
    void clear() override;
    
private:
    void initDatabase();
    
    std::string dbPath_;
    void* db_ = nullptr;  // sqlite3*
};

/**
 * @brief Cookie Manager
 * 
 * Thread-safe cookie management with persistent storage.
 */
class CookieManager {
public:
    CookieManager();
    explicit CookieManager(std::unique_ptr<CookieStore> store);
    ~CookieManager();
    
    /**
     * @brief Set cookie from Set-Cookie header
     */
    void setCookie(const std::string& setCookieHeader, const std::string& requestUrl);
    
    /**
     * @brief Set cookie directly
     */
    void setCookie(const Cookie& cookie);
    
    /**
     * @brief Get cookies for URL
     * @return Cookie header value
     */
    std::string getCookiesForUrl(const std::string& url, bool isSecure = true);
    
    /**
     * @brief Get all cookies for domain
     */
    std::vector<Cookie> getCookiesForDomain(const std::string& domain);
    
    /**
     * @brief Get all cookies
     */
    std::vector<Cookie> getAllCookies();
    
    /**
     * @brief Delete cookie
     */
    void deleteCookie(const std::string& domain, const std::string& name, 
                      const std::string& path = "/");
    
    /**
     * @brief Delete all cookies for domain
     */
    void deleteCookiesForDomain(const std::string& domain);
    
    /**
     * @brief Delete all cookies
     */
    void deleteAllCookies();
    
    /**
     * @brief Delete expired cookies
     */
    void deleteExpiredCookies();
    
    /**
     * @brief Save cookies to persistent storage
     */
    bool saveToDisk();
    
    /**
     * @brief Load cookies from persistent storage
     */
    bool loadFromDisk();
    
    /**
     * @brief Get cookie count
     */
    size_t count() const;
    
    /**
     * @brief Set cookie change callback
     */
    using CookieChangeCallback = std::function<void(const Cookie&, bool removed)>;
    void setChangeCallback(CookieChangeCallback callback);
    
private:
    void addCookie(Cookie cookie);
    void removeExpired();
    std::string extractDomain(const std::string& url) const;
    std::string extractPath(const std::string& url) const;
    bool isSecureUrl(const std::string& url) const;
    
    mutable std::mutex mutex_;
    
    // Domain -> (name+path -> Cookie)
    std::unordered_map<std::string, 
        std::unordered_map<std::string, Cookie>> cookies_;
    
    std::unique_ptr<CookieStore> store_;
    CookieChangeCallback changeCallback_;
    
    size_t maxCookiesPerDomain_ = 50;
    size_t maxTotalCookies_ = 3000;
};

/**
 * @brief Global cookie manager instance
 */
CookieManager& getCookieManager();

} // namespace Zepra::Networking
