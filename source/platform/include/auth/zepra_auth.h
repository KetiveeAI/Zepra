#ifndef ZEPRA_AUTH_H
#define ZEPRA_AUTH_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <mutex>
#include <atomic>

// Forward declarations
struct NxHttpClient;
struct json_object;

namespace ZepraAuth {

// Authentication states
enum class AuthState {
    UNAUTHENTICATED,
    AUTHENTICATING,
    AUTHENTICATED,
    EXPIRED,
    LOCKED,
    ERROR
};

// User roles
enum class UserRole {
    USER,
    DEVELOPER,
    ADMIN
};

// 2FA status
enum class TwoFactorStatus {
    DISABLED,
    ENABLED,
    REQUIRED,
    VERIFIED
};

// Authentication token structure
struct AuthToken {
    std::string token;
    std::string refreshToken;
    std::chrono::system_clock::time_point expiresAt;
    std::chrono::system_clock::time_point refreshExpiresAt;
    bool isValid;
    
    AuthToken() : isValid(false) {}
};

// User information structure
struct UserInfo {
    std::string id;
    std::string email;
    std::string firstName;
    std::string lastName;
    UserRole role;
    std::string avatar;
    bool emailVerified;
    TwoFactorStatus twoFactorStatus;
    std::chrono::system_clock::time_point lastLogin;
    std::chrono::system_clock::time_point createdAt;
    
    UserInfo() : role(UserRole::USER), emailVerified(false), twoFactorStatus(TwoFactorStatus::DISABLED) {}
};

// Cookie management
struct Cookie {
    std::string name;
    std::string value;
    std::string domain;
    std::string path;
    std::chrono::system_clock::time_point expiresAt;
    bool httpOnly;
    bool secure;
    std::string sameSite;
    
    Cookie() : httpOnly(false), secure(false) {}
};

// Authentication callback types
using AuthCallback = std::function<void(AuthState, const UserInfo&)>;
using TwoFactorCallback = std::function<void(const std::string&)>;
using PasswordPromptCallback = std::function<void(const std::string&, const std::string&)>;

// Main authentication manager class
class ZepraAuthManager {
public:
    static ZepraAuthManager& getInstance();
    
    // Initialization
    bool initialize(const std::string& authServerUrl = "https://auth.ketivee.com");
    void shutdown();
    
    // Authentication methods
    bool login(const std::string& email, const std::string& password);
    bool loginWith2FA(const std::string& tempToken, const std::string& code);
    bool logout();
    bool refreshToken();
    
    // Session management
    bool checkSession();
    bool verifyToken();
    void clearSession();
    
    // Cookie management
    bool setCookie(const Cookie& cookie);
    bool getCookie(const std::string& name, Cookie& cookie);
    bool clearCookie(const std::string& name);
    void clearAllCookies();
    std::vector<Cookie> getAllCookies();
    
    // User management
    UserInfo getCurrentUser() const;
    AuthState getAuthState() const;
    bool isAuthenticated() const;
    bool isTokenExpired() const;
    
    // Callbacks
    void setAuthCallback(AuthCallback callback);
    void setTwoFactorCallback(TwoFactorCallback callback);
    void setPasswordPromptCallback(PasswordPromptCallback callback);
    
    // Third-party website authentication
    bool authenticateForWebsite(const std::string& websiteUrl);
    bool promptForPassword(const std::string& websiteUrl);
    bool promptFor2FA(const std::string& websiteUrl);
    
    // Configuration
    void setAuthServerUrl(const std::string& url);
    void setSessionTimeout(std::chrono::seconds timeout);
    void enableAutoRefresh(bool enable);
    
    // Security
    void enableSecureMode(bool enable);
    void setAllowedDomains(const std::vector<std::string>& domains);
    bool isDomainAllowed(const std::string& domain) const;

private:
    ZepraAuthManager();
    ~ZepraAuthManager();
    
    // HTTP methods
    bool httpGet(const std::string& url, std::string& response);
    bool httpPost(const std::string& url, const std::string& data, std::string& response);
    
    // JSON parsing
    bool parseAuthResponse(const std::string& json, AuthToken& token, UserInfo& user);
    bool parseUserInfo(const json_object* obj, UserInfo& user);
    bool parseTokenInfo(const json_object* obj, AuthToken& token);
    
    // Token management
    bool decodeJWT(const std::string& token, std::map<std::string, std::string>& claims);
    bool isTokenValid(const AuthToken& token) const;
    bool shouldRefreshToken(const AuthToken& token) const;
    
    // Cookie parsing
    bool parseSetCookieHeader(const std::string& header, Cookie& cookie);
    std::string formatCookieHeader() const;
    
    // Security
    std::string hashPassword(const std::string& password);
    std::string generateNonce();
    bool validateDomain(const std::string& domain) const;
    
    // Member variables
    std::string m_authServerUrl;
    AuthState m_authState;
    AuthToken m_currentToken;
    UserInfo m_currentUser;
    std::vector<Cookie> m_cookies;
    std::vector<std::string> m_allowedDomains;
    
    // Callbacks
    AuthCallback m_authCallback;
    TwoFactorCallback m_twoFactorCallback;
    PasswordPromptCallback m_passwordPromptCallback;
    
    // Configuration
    std::chrono::seconds m_sessionTimeout;
    bool m_autoRefresh;
    bool m_secureMode;
    
    // Threading
    mutable std::mutex m_mutex;
    std::atomic<bool> m_initialized;
    
    // HTTP Client
    NxHttpClient* m_httpClient;
    
    // Disable copy constructor and assignment
    ZepraAuthManager(const ZepraAuthManager&) = delete;
    ZepraAuthManager& operator=(const ZepraAuthManager&) = delete;
};

// Utility functions
namespace Utils {
    std::string base64Encode(const std::string& input);
    std::string base64Decode(const std::string& input);
    std::string urlEncode(const std::string& input);
    std::string urlDecode(const std::string& input);
    std::string generateUUID();
    std::string getCurrentTimestamp();
    bool isValidEmail(const std::string& email);
    std::string hashString(const std::string& input);
    
    // Domain extraction from URL
    inline std::string extractDomain(const std::string& url) {
        size_t start = url.find("://");
        if (start == std::string::npos) start = 0;
        else start += 3;
        size_t end = url.find('/', start);
        if (end == std::string::npos) end = url.length();
        return url.substr(start, end - start);
    }
}

// Constants
namespace Constants {
    constexpr const char* AUTH_ENDPOINT_LOGIN = "/api/auth/login";
    constexpr const char* AUTH_ENDPOINT_VERIFY = "/api/agent/verify";
    constexpr const char* AUTH_ENDPOINT_LOGOUT = "/api/agent/logout";
    constexpr const char* AUTH_ENDPOINT_REFRESH = "/api/auth/refresh";
    constexpr const char* AUTH_ENDPOINT_2FA = "/api/auth/verify-2fa";
    
    constexpr const char* COOKIE_AUTH_TOKEN = "authToken";
    constexpr const char* COOKIE_USER_INFO = "userInfo";
    constexpr const char* COOKIE_SESSION_ID = "sessionId";
    
    constexpr int TOKEN_REFRESH_THRESHOLD_MINUTES = 30;
    constexpr int SESSION_TIMEOUT_MINUTES = 60 * 24 * 7; // 7 days
    constexpr int MAX_LOGIN_ATTEMPTS = 5;
    constexpr int LOCKOUT_DURATION_MINUTES = 15;
}

} // namespace ZepraAuth

#endif // ZEPRA_AUTH_H 