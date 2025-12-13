/**
 * @file content_policy.hpp
 * @brief Zepra Strict Mode Content Policy
 * 
 * Enforces strict web rules to protect users from:
 * - Cookie stealing and tracking
 * - Excessive ads and popups
 * - User experience exploitation
 */

#ifndef WEBCORE_CONTENT_POLICY_HPP
#define WEBCORE_CONTENT_POLICY_HPP

#include <string>
#include <unordered_set>
#include <unordered_map>

namespace Zepra::WebCore {

/**
 * Content Policy Mode
 */
enum class PolicyMode {
    Strict,      // Maximum protection (default)
    Compatible   // Relaxed for compatibility
};

/**
 * Content blocking decisions
 */
enum class BlockDecision {
    Allow,
    Block,
    RequireGesture,
    RateLimit
};

/**
 * ContentPolicy - Enforces strict web browsing rules
 */
class ContentPolicy {
public:
    ContentPolicy() : mode_(PolicyMode::Strict) {}
    
    void setMode(PolicyMode mode) { mode_ = mode; }
    PolicyMode mode() const { return mode_; }
    
    // =========================================================================
    // Cookie Policy
    // =========================================================================
    
    bool allowThirdPartyCookies() const {
        return mode_ == PolicyMode::Compatible;
    }
    
    bool allowCrossOriginCookieAccess() const {
        return false; // ALWAYS blocked - prevents cookie theft
    }
    
    bool enforceSameSiteStrict() const {
        return mode_ == PolicyMode::Strict;
    }
    
    // =========================================================================
    // Popup Policy
    // =========================================================================
    
    BlockDecision checkPopup(bool hasUserGesture) const {
        if (mode_ == PolicyMode::Compatible) return BlockDecision::Allow;
        return hasUserGesture ? BlockDecision::Allow : BlockDecision::Block;
    }
    
    int maxPopupsPerGesture() const { return 1; }
    
    // =========================================================================
    // Ad/Overlay Policy
    // =========================================================================
    
    bool blockOverlayAds() const { return mode_ == PolicyMode::Strict; }
    bool blockInterstitialAds() const { return mode_ == PolicyMode::Strict; }
    bool blockAutoExpandAds() const { return mode_ == PolicyMode::Strict; }
    
    // =========================================================================
    // Redirect Policy
    // =========================================================================
    
    int maxRedirectChain() const { return 3; }
    
    bool allowMetaRefresh() const {
        return mode_ == PolicyMode::Compatible;
    }
    
    // =========================================================================
    // Autoplay Policy
    // =========================================================================
    
    bool allowAutoplayWithSound() const {
        return false; // Always require muted or user gesture
    }
    
    bool allowAutoplayMuted() const {
        return true; // Muted autoplay is okay
    }
    
    // =========================================================================
    // Notification Policy
    // =========================================================================
    
    int maxNotificationPrompts() const { return 1; } // Per session per site
    
    // =========================================================================
    // Download Policy
    // =========================================================================
    
    BlockDecision checkDownload(bool hasUserGesture) const {
        return hasUserGesture ? BlockDecision::Allow : BlockDecision::Block;
    }
    
    // =========================================================================
    // Rate Limiting
    // =========================================================================
    
    int maxAlertCalls() const { return 3; }
    int maxConfirmCalls() const { return 3; }
    int maxPromptCalls() const { return 1; }
    
    // =========================================================================
    // Per-Site Overrides
    // =========================================================================
    
    void allowSiteCookies(const std::string& origin) {
        trustedOrigins_.insert(origin);
    }
    
    void allowSitePopups(const std::string& origin) {
        popupAllowedOrigins_.insert(origin);
    }
    
    bool isTrustedOrigin(const std::string& origin) const {
        return trustedOrigins_.count(origin) > 0;
    }
    
    bool isPopupAllowed(const std::string& origin) const {
        return popupAllowedOrigins_.count(origin) > 0;
    }
    
private:
    PolicyMode mode_;
    std::unordered_set<std::string> trustedOrigins_;
    std::unordered_set<std::string> popupAllowedOrigins_;
};

/**
 * Global content policy instance
 */
inline ContentPolicy& contentPolicy() {
    static ContentPolicy instance;
    return instance;
}

} // namespace Zepra::WebCore

#endif // WEBCORE_CONTENT_POLICY_HPP
