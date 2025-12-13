/**
 * @file sandboxed_frame.hpp
 * @brief Sandboxed iframe with origin isolation
 */

#pragma once

#include "storage/site_settings.hpp"
#include <string>
#include <memory>
#include <unordered_set>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief Sandbox flags (matching HTML iframe sandbox attribute)
 */
enum class SandboxFlag : uint32_t {
    None                    = 0,
    AllowDownloads          = 1 << 0,
    AllowForms              = 1 << 1,
    AllowModals             = 1 << 2,
    AllowOrientationLock    = 1 << 3,
    AllowPointerLock        = 1 << 4,
    AllowPopups             = 1 << 5,
    AllowPopupsToEscapeSandbox = 1 << 6,
    AllowPresentation       = 1 << 7,
    AllowSameOrigin         = 1 << 8,
    AllowScripts            = 1 << 9,
    AllowStorageAccessByUserActivation = 1 << 10,
    AllowTopNavigation      = 1 << 11,
    AllowTopNavigationByUserActivation = 1 << 12,
    AllowTopNavigationToCustomProtocols = 1 << 13
};

inline SandboxFlag operator|(SandboxFlag a, SandboxFlag b) {
    return static_cast<SandboxFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline SandboxFlag operator&(SandboxFlag a, SandboxFlag b) {
    return static_cast<SandboxFlag>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool hasFlag(SandboxFlag flags, SandboxFlag flag) {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

/**
 * @brief Frame security context
 */
struct FrameSecurityContext {
    std::string origin;
    std::string effectiveOrigin;  // May be opaque for sandboxed
    bool isSandboxed = false;
    SandboxFlag sandboxFlags = SandboxFlag::None;
    
    // Cross-Origin policies
    bool crossOriginIsolated = false;
    std::string crossOriginEmbedderPolicy;  // require-corp, credentialless
    std::string crossOriginOpenerPolicy;    // same-origin, same-origin-allow-popups
    std::string crossOriginResourcePolicy;  // same-origin, same-site, cross-origin
    
    // Content Security Policy
    std::string contentSecurityPolicy;
    
    // Permissions Policy
    std::unordered_set<std::string> allowedFeatures;
};

/**
 * @brief Sandboxed Frame
 * 
 * Represents an iframe with security sandboxing and origin isolation.
 */
class SandboxedFrame {
public:
    SandboxedFrame(const std::string& parentOrigin);
    ~SandboxedFrame();
    
    /**
     * @brief Set sandbox attribute (from HTML)
     */
    void setSandbox(const std::string& sandboxAttr);
    
    /**
     * @brief Parse sandbox tokens
     */
    static SandboxFlag parseSandboxTokens(const std::string& sandboxAttr);
    
    /**
     * @brief Get sandbox flags
     */
    SandboxFlag sandboxFlags() const { return sandboxFlags_; }
    
    /**
     * @brief Load URL in frame
     */
    void load(const std::string& url);
    
    /**
     * @brief Get frame origin
     */
    const std::string& origin() const { return context_.origin; }
    
    /**
     * @brief Get effective origin (may be opaque)
     */
    const std::string& effectiveOrigin() const { return context_.effectiveOrigin; }
    
    /**
     * @brief Check if same origin as parent
     */
    bool isSameOriginWithParent() const;
    
    /**
     * @brief Check if cross-origin isolated
     */
    bool isCrossOriginIsolated() const { return context_.crossOriginIsolated; }
    
    // ===== Permission Checks =====
    
    /**
     * @brief Check if scripts allowed
     */
    bool canRunScripts() const;
    
    /**
     * @brief Check if forms allowed
     */
    bool canSubmitForms() const;
    
    /**
     * @brief Check if popups allowed
     */
    bool canOpenPopups() const;
    
    /**
     * @brief Check if storage access allowed
     */
    bool canAccessStorage() const;
    
    /**
     * @brief Check if top navigation allowed
     */
    bool canNavigateTop() const;
    
    /**
     * @brief Check if feature allowed by permissions policy
     */
    bool isFeatureAllowed(const std::string& feature) const;
    
    // ===== Cross-Origin Communication =====
    
    /**
     * @brief Post message to frame
     */
    void postMessage(const std::string& message, const std::string& targetOrigin);
    
    /**
     * @brief Set message handler
     */
    using MessageHandler = std::function<void(const std::string& message, 
                                               const std::string& origin)>;
    void setOnMessage(MessageHandler handler) { onMessage_ = std::move(handler); }
    
    /**
     * @brief Receive message from parent/child
     */
    void receiveMessage(const std::string& message, const std::string& sourceOrigin);
    
    // ===== Content Security =====
    
    /**
     * @brief Set CSP
     */
    void setContentSecurityPolicy(const std::string& csp);
    
    /**
     * @brief Set Permissions-Policy
     */
    void setPermissionsPolicy(const std::string& policy);
    
    /**
     * @brief Check CSP for resource
     */
    bool isResourceAllowedByCSP(const std::string& url, const std::string& type) const;
    
private:
    void updateEffectiveOrigin();
    std::string generateOpaqueOrigin();
    
    std::string parentOrigin_;
    SandboxFlag sandboxFlags_ = SandboxFlag::None;
    FrameSecurityContext context_;
    MessageHandler onMessage_;
};

/**
 * @brief Frame tree for nested iframes
 */
class FrameTree {
public:
    struct Node {
        std::unique_ptr<SandboxedFrame> frame;
        std::string name;
        std::vector<std::unique_ptr<Node>> children;
        Node* parent = nullptr;
    };
    
    FrameTree(const std::string& mainOrigin);
    
    /**
     * @brief Get main frame
     */
    SandboxedFrame* mainFrame() { return root_->frame.get(); }
    
    /**
     * @brief Add child frame
     */
    SandboxedFrame* addChild(Node* parent, const std::string& name);
    
    /**
     * @brief Find frame by name
     */
    SandboxedFrame* findFrame(const std::string& name);
    
    /**
     * @brief Get frame ancestry
     */
    std::vector<SandboxedFrame*> getAncestors(Node* node);
    
private:
    Node* findNode(Node* node, const std::string& name);
    
    std::unique_ptr<Node> root_;
};

} // namespace Zepra::WebCore
