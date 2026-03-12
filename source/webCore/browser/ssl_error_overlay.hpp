/**
 * @file ssl_error_overlay.hpp
 * @brief SSL certificate error overlay UI
 */

#pragma once

#include "networking/ssl_context.hpp"
#include <string>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief SSL error type for display
 */
enum class SSLErrorType {
    None,
    Expired,
    NotYetValid,
    HostnameMismatch,
    SelfSigned,
    UntrustedRoot,
    Revoked,
    WeakKey,
    MixedContent,
    Unknown
};

/**
 * @brief SSL error details
 */
struct SSLErrorInfo {
    SSLErrorType type = SSLErrorType::None;
    std::string url;
    std::string hostname;
    std::string issuer;
    std::string validFrom;
    std::string validTo;
    std::string fingerprint;
    bool allowProceed = false;  // Can user bypass?
    std::string errorCode;
    std::string errorDescription;
};

/**
 * @brief User decision on SSL error
 */
enum class SSLErrorDecision {
    GoBack,         // User goes back
    Proceed,        // User proceeds anyway (if allowed)
    LearnMore       // User wants more info
};

using SSLErrorCallback = std::function<void(SSLErrorDecision)>;

/**
 * @brief SSL Error Overlay
 * 
 * Renders a full-page warning for SSL/TLS errors.
 */
class SSLErrorOverlay {
public:
    SSLErrorOverlay();
    ~SSLErrorOverlay();
    
    /**
     * @brief Show overlay for error
     */
    void show(const SSLErrorInfo& info, SSLErrorCallback callback);
    
    /**
     * @brief Hide overlay
     */
    void hide();
    
    /**
     * @brief Check if visible
     */
    bool isVisible() const { return visible_; }
    
    /**
     * @brief Get HTML for overlay
     */
    std::string getOverlayHTML() const;
    
    /**
     * @brief Handle user action from overlay
     */
    void handleAction(const std::string& action);
    
    /**
     * @brief Convert CertVerifyResult to SSLErrorType
     */
    static SSLErrorType fromCertResult(Networking::CertVerifyResult result);
    
    /**
     * @brief Get error title
     */
    static std::string getErrorTitle(SSLErrorType type);
    
    /**
     * @brief Get error description
     */
    static std::string getErrorDescription(SSLErrorType type);
    
    /**
     * @brief Check if error is bypassable
     */
    static bool canBypass(SSLErrorType type);
    
private:
    bool visible_ = false;
    SSLErrorInfo currentError_;
    SSLErrorCallback callback_;
};

/**
 * @brief Generate SSL warning HTML page
 */
std::string generateSSLWarningHTML(const SSLErrorInfo& info);

} // namespace Zepra::WebCore
