/**
 * @file security_checker.cpp
 * @brief Security checker implementation
 */

#include "security/security_checker.h"
#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

namespace zepra {

// Known phishing indicators
static const std::vector<std::string> PHISHING_KEYWORDS = {
    "login", "signin", "verify", "account", "secure", "update", 
    "confirm", "banking", "password", "credential", "suspend"
};

// Suspicious TLDs often used in phishing
static const std::vector<std::string> SUSPICIOUS_TLDS = {
    ".tk", ".ml", ".ga", ".cf", ".gq", ".xyz", ".top", ".work", ".click"
};

// Known safe origins
static const std::vector<std::string> TRUSTED_DOMAINS = {
    "google.com", "microsoft.com", "apple.com", "github.com",
    "amazon.com", "facebook.com", "twitter.com", "youtube.com"
};

// Dangerous file extensions
static const std::vector<std::string> DANGEROUS_EXTENSIONS = {
    ".exe", ".msi", ".bat", ".cmd", ".ps1", ".vbs", ".js",
    ".scr", ".pif", ".com", ".jar", ".wsf", ".hta"
};

SecurityChecker::SecurityChecker() {
    // Initialize trusted origins
    for (const auto& domain : TRUSTED_DOMAINS) {
        trusted_origins_.insert(domain);
    }
}

SecurityChecker::~SecurityChecker() {}

void SecurityChecker::setWarningCallback(WarningCallback callback) {
    warning_callback_ = callback;
}

void SecurityChecker::addWarning(SecurityThreatType type, SecurityWarningLevel level,
                                  const std::string& message, const std::string& url) {
    SecurityWarning warning;
    warning.type = type;
    warning.level = level;
    warning.message = message;
    warning.url = url;
    warning.timestamp = std::chrono::system_clock::now();
    warning.blocked = (level >= SecurityWarningLevel::HIGH);
    
    if (warning.blocked) blocked_count_++;
    
    warnings_.push_back(warning);
    
    if (warning_callback_) {
        warning_callback_(warning);
    }
}

// ============================================================================
// Origin Helpers
// ============================================================================

std::string SecurityChecker::getOrigin(const std::string& url) {
    // Extract scheme://host:port
    size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos) return "";
    
    size_t host_start = scheme_end + 3;
    size_t host_end = url.find('/', host_start);
    if (host_end == std::string::npos) host_end = url.length();
    
    // Check for port
    size_t port_start = url.find(':', host_start);
    if (port_start != std::string::npos && port_start < host_end) {
        // Include port in origin
        return url.substr(0, host_end);
    }
    
    // Add default port for comparison
    std::string origin = url.substr(0, host_end);
    return origin;
}

bool SecurityChecker::isSameOrigin(const std::string& url1, const std::string& url2) {
    std::string origin1 = getOrigin(url1);
    std::string origin2 = getOrigin(url2);
    
    // Case-insensitive comparison
    std::string lower1 = origin1, lower2 = origin2;
    std::transform(lower1.begin(), lower1.end(), lower1.begin(), ::tolower);
    std::transform(lower2.begin(), lower2.end(), lower2.begin(), ::tolower);
    
    return lower1 == lower2;
}

// ============================================================================
// CORS Checking
// ============================================================================

bool SecurityChecker::checkCors(const std::string& source_origin,
                                 const std::string& target_url,
                                 const std::string& method) {
    std::string target_origin = getOrigin(target_url);
    
    // Same-origin is always allowed
    if (isSameOrigin(source_origin, target_url)) {
        return true;
    }
    
    // Simple requests (GET, HEAD, POST with certain content-types) are allowed
    // but response may be opaque
    std::string upper_method = method;
    std::transform(upper_method.begin(), upper_method.end(), upper_method.begin(), ::toupper);
    
    bool is_simple = (upper_method == "GET" || upper_method == "HEAD" || upper_method == "POST");
    
    if (!is_simple) {
        // Non-simple requests require preflight
        addWarning(SecurityThreatType::CORS_VIOLATION, SecurityWarningLevel::MEDIUM,
                   "Cross-origin " + method + " request requires preflight",
                   target_url);
    }
    
    // Log cross-origin request
    addWarning(SecurityThreatType::CORS_VIOLATION, SecurityWarningLevel::INFO,
               "Cross-origin request from " + source_origin + " to " + target_origin,
               target_url);
    
    return true;  // Allow but log
}

CorsPolicy SecurityChecker::parseCorsHeaders(const std::vector<std::pair<std::string, std::string>>& headers) {
    CorsPolicy policy;
    
    for (const auto& header : headers) {
        std::string name = header.first;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        
        if (name == "access-control-allow-origin") {
            policy.allowed_origins.push_back(header.second);
        } else if (name == "access-control-allow-methods") {
            // Split by comma
            std::stringstream ss(header.second);
            std::string item;
            while (std::getline(ss, item, ',')) {
                // Trim whitespace
                size_t start = item.find_first_not_of(" \t");
                size_t end = item.find_last_not_of(" \t");
                if (start != std::string::npos) {
                    policy.allowed_methods.push_back(item.substr(start, end - start + 1));
                }
            }
        } else if (name == "access-control-allow-credentials") {
            policy.allow_credentials = (header.second == "true");
        } else if (name == "access-control-max-age") {
            policy.max_age = std::stoi(header.second);
        }
    }
    
    return policy;
}

// ============================================================================
// Phishing Detection
// ============================================================================

static std::string extractDomain(const std::string& url) {
    size_t start = url.find("://");
    if (start == std::string::npos) return url;
    start += 3;
    
    size_t end = url.find('/', start);
    if (end == std::string::npos) end = url.length();
    
    // Remove port
    size_t port = url.find(':', start);
    if (port != std::string::npos && port < end) end = port;
    
    return url.substr(start, end - start);
}

SecurityWarningLevel SecurityChecker::checkPhishing(const std::string& url) {
    std::string domain = extractDomain(url);
    std::string lower_url = url;
    std::transform(lower_url.begin(), lower_url.end(), lower_url.begin(), ::tolower);
    
    int suspicion_score = 0;
    
    // Check if domain is blocked
    if (isDomainBlocked(domain)) {
        addWarning(SecurityThreatType::PHISHING_SUSPECTED, SecurityWarningLevel::CRITICAL,
                   "Domain is on blocklist", url);
        return SecurityWarningLevel::CRITICAL;
    }
    
    // Check for IP address as domain (suspicious)
    std::regex ip_regex(R"(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})");
    if (std::regex_match(domain, ip_regex)) {
        suspicion_score += 30;
    }
    
    // Check for suspicious TLD
    for (const auto& tld : SUSPICIOUS_TLDS) {
        if (domain.length() > tld.length() && 
            domain.substr(domain.length() - tld.length()) == tld) {
            suspicion_score += 20;
            break;
        }
    }
    
    // Check for phishing keywords in URL
    for (const auto& keyword : PHISHING_KEYWORDS) {
        if (lower_url.find(keyword) != std::string::npos) {
            suspicion_score += 10;
        }
    }
    
    // Check for lookalike domains (e.g., g00gle.com)
    for (const auto& trusted : TRUSTED_DOMAINS) {
        // Simple Levenshtein-like check
        if (domain != trusted && domain.find(trusted.substr(0, 4)) != std::string::npos) {
            suspicion_score += 25;
            break;
        }
    }
    
    // Check for excessive subdomains
    int dot_count = std::count(domain.begin(), domain.end(), '.');
    if (dot_count > 3) {
        suspicion_score += 15;
    }
    
    // Check for @ symbol (URL confusion)
    if (url.find('@') != std::string::npos) {
        suspicion_score += 40;
    }
    
    // Convert score to level
    SecurityWarningLevel level = SecurityWarningLevel::NONE;
    if (suspicion_score >= 60) {
        level = SecurityWarningLevel::HIGH;
        addWarning(SecurityThreatType::PHISHING_SUSPECTED, level,
                   "URL has multiple phishing indicators", url);
    } else if (suspicion_score >= 30) {
        level = SecurityWarningLevel::MEDIUM;
        addWarning(SecurityThreatType::PHISHING_SUSPECTED, level,
                   "URL has suspicious characteristics", url);
    } else if (suspicion_score >= 15) {
        level = SecurityWarningLevel::LOW;
    }
    
    return level;
}

bool SecurityChecker::isDomainBlocked(const std::string& domain) {
    std::string lower = domain;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return blocked_domains_.count(lower) > 0;
}

void SecurityChecker::blockDomain(const std::string& domain) {
    std::string lower = domain;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    blocked_domains_.insert(lower);
}

// ============================================================================
// Mixed Content
// ============================================================================

bool SecurityChecker::checkMixedContent(const std::string& page_url, const std::string& resource_url) {
    bool page_https = page_url.find("https://") == 0;
    bool resource_https = resource_url.find("https://") == 0;
    
    if (page_https && !resource_https && resource_url.find("http://") == 0) {
        addWarning(SecurityThreatType::MIXED_CONTENT, SecurityWarningLevel::MEDIUM,
                   "Loading insecure resource on secure page", resource_url);
        return true;  // Is mixed content
    }
    
    return false;
}

// ============================================================================
// Certificate Checks
// ============================================================================

SecurityWarningLevel SecurityChecker::checkCertificate(const std::string& domain,
                                                        const std::string& issuer,
                                                        bool expired,
                                                        bool self_signed) {
    if (expired) {
        addWarning(SecurityThreatType::CERTIFICATE_ERROR, SecurityWarningLevel::HIGH,
                   "SSL certificate has expired", domain);
        return SecurityWarningLevel::HIGH;
    }
    
    if (self_signed) {
        addWarning(SecurityThreatType::CERTIFICATE_ERROR, SecurityWarningLevel::MEDIUM,
                   "SSL certificate is self-signed", domain);
        return SecurityWarningLevel::MEDIUM;
    }
    
    // Check for known bad issuers (placeholder)
    (void)issuer;
    
    return SecurityWarningLevel::NONE;
}

// ============================================================================
// XSS Detection
// ============================================================================

bool SecurityChecker::detectXss(const std::string& input) {
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    // Check for script tags
    if (lower.find("<script") != std::string::npos) return true;
    
    // Check for event handlers
    std::vector<std::string> handlers = {
        "onerror=", "onload=", "onclick=", "onmouseover=", 
        "onfocus=", "onblur=", "onsubmit=", "onkeydown="
    };
    for (const auto& h : handlers) {
        if (lower.find(h) != std::string::npos) return true;
    }
    
    // Check for javascript: URLs
    if (lower.find("javascript:") != std::string::npos) return true;
    
    // Check for data: URLs with scripts
    if (lower.find("data:text/html") != std::string::npos) return true;
    
    return false;
}

std::string SecurityChecker::sanitizeHtml(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    
    for (char c : input) {
        switch (c) {
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '&': result += "&amp;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default: result += c; break;
        }
    }
    
    return result;
}

// ============================================================================
// Download Safety
// ============================================================================

bool SecurityChecker::isDangerousFileType(const std::string& filename) {
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    for (const auto& ext : DANGEROUS_EXTENSIONS) {
        if (lower.length() > ext.length() &&
            lower.substr(lower.length() - ext.length()) == ext) {
            addWarning(SecurityThreatType::SUSPICIOUS_DOWNLOAD, SecurityWarningLevel::HIGH,
                       "Potentially dangerous file type", filename);
            return true;
        }
    }
    
    return false;
}

// ============================================================================
// Warning Management
// ============================================================================

const std::vector<SecurityWarning>& SecurityChecker::getWarnings() const {
    return warnings_;
}

void SecurityChecker::clearWarnings() {
    warnings_.clear();
}

size_t SecurityChecker::getBlockedCount() const {
    return blocked_count_;
}

// Global instance
static SecurityChecker g_security_checker;

SecurityChecker& getSecurityChecker() {
    return g_security_checker;
}

} // namespace zepra
