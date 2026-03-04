/**
 * @file security_checker.h
 * @brief Browser security module - CORS, phishing, and threat detection
 */

#ifndef SECURITY_CHECKER_H
#define SECURITY_CHECKER_H

#include <string>
#include <vector>
#include <unordered_set>
#include <functional>
#include <chrono>

namespace zepra {

// ============================================================================
// Security Warning Types
// ============================================================================

enum class SecurityWarningLevel {
    NONE = 0,
    INFO,           // Informational
    LOW,            // Minor concern
    MEDIUM,         // Potential risk
    HIGH,           // Likely threat
    CRITICAL        // Confirmed threat - block
};

enum class SecurityThreatType {
    NONE = 0,
    CORS_VIOLATION,         // Cross-origin request blocked
    MIXED_CONTENT,          // HTTP on HTTPS page
    PHISHING_SUSPECTED,     // URL looks like phishing
    MALWARE_DOMAIN,         // Known malware domain
    CERTIFICATE_ERROR,      // Invalid/expired SSL cert
    REDIRECT_LOOP,          // Too many redirects
    SUSPICIOUS_DOWNLOAD,    // Dangerous file type
    XSS_ATTEMPT,            // Script injection detected
    CLICKJACKING,           // Frame-buster detected
    INSECURE_FORM           // Form submitting to HTTP
};

struct SecurityWarning {
    SecurityThreatType type;
    SecurityWarningLevel level;
    std::string message;
    std::string url;
    std::string source_origin;
    std::string target_origin;
    std::chrono::system_clock::time_point timestamp;
    bool blocked = false;
};

// ============================================================================
// CORS Policy
// ============================================================================

struct CorsPolicy {
    bool allow_credentials = false;
    std::vector<std::string> allowed_origins;    // "*" for any
    std::vector<std::string> allowed_methods;    // GET, POST, etc.
    std::vector<std::string> allowed_headers;
    std::vector<std::string> exposed_headers;
    int max_age = 86400;  // Preflight cache time
};

// ============================================================================
// Security Checker
// ============================================================================

class SecurityChecker {
public:
    using WarningCallback = std::function<void(const SecurityWarning&)>;
    
    SecurityChecker();
    ~SecurityChecker();
    
    // Set callback for warnings
    void setWarningCallback(WarningCallback callback);
    
    // ========================================================================
    // CORS Checking
    // ========================================================================
    
    /**
     * Check if cross-origin request is allowed.
     * @param source_origin Origin of the requesting page
     * @param target_url URL being requested
     * @param method HTTP method
     * @return true if allowed
     */
    bool checkCors(const std::string& source_origin, 
                   const std::string& target_url,
                   const std::string& method = "GET");
    
    /**
     * Parse CORS headers from response.
     */
    CorsPolicy parseCorsHeaders(const std::vector<std::pair<std::string, std::string>>& headers);
    
    /**
     * Check if origins are same-origin.
     */
    static bool isSameOrigin(const std::string& url1, const std::string& url2);
    
    /**
     * Extract origin from URL.
     */
    static std::string getOrigin(const std::string& url);
    
    // ========================================================================
    // Phishing Detection
    // ========================================================================
    
    /**
     * Check URL for phishing indicators.
     * @return Threat level (NONE = safe)
     */
    SecurityWarningLevel checkPhishing(const std::string& url);
    
    /**
     * Check if domain is in known blocklist.
     */
    bool isDomainBlocked(const std::string& domain);
    
    /**
     * Add domain to local blocklist.
     */
    void blockDomain(const std::string& domain);
    
    // ========================================================================
    // Mixed Content
    // ========================================================================
    
    /**
     * Check for mixed content (HTTP on HTTPS page).
     */
    bool checkMixedContent(const std::string& page_url, const std::string& resource_url);
    
    // ========================================================================
    // Certificate Checks
    // ========================================================================
    
    /**
     * Validate SSL certificate info.
     */
    SecurityWarningLevel checkCertificate(const std::string& domain,
                                          const std::string& issuer,
                                          bool expired,
                                          bool self_signed);
    
    // ========================================================================
    // XSS Detection
    // ========================================================================
    
    /**
     * Check for potential XSS in input.
     */
    bool detectXss(const std::string& input);
    
    /**
     * Sanitize string for XSS.
     */
    std::string sanitizeHtml(const std::string& input);
    
    // ========================================================================
    // Download Safety
    // ========================================================================
    
    /**
     * Check if file extension is dangerous.
     */
    bool isDangerousFileType(const std::string& filename);
    
    // ========================================================================
    // Warning Management
    // ========================================================================
    
    const std::vector<SecurityWarning>& getWarnings() const;
    void clearWarnings();
    size_t getBlockedCount() const;
    
private:
    void addWarning(SecurityThreatType type, SecurityWarningLevel level,
                    const std::string& message, const std::string& url);
    
    std::vector<SecurityWarning> warnings_;
    std::unordered_set<std::string> blocked_domains_;
    std::unordered_set<std::string> trusted_origins_;
    WarningCallback warning_callback_;
    size_t blocked_count_ = 0;
};

// Global security checker instance
SecurityChecker& getSecurityChecker();

} // namespace zepra

#endif // SECURITY_CHECKER_H
