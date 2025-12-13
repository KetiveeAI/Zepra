/**
 * @file sandboxed_frame.cpp
 * @brief Sandboxed iframe implementation
 */

#include "webcore/sandboxed_frame.hpp"
#include "storage/site_settings.hpp"

#include <sstream>
#include <random>
#include <algorithm>
#include <iomanip>

namespace Zepra::WebCore {

using namespace Storage;

// =============================================================================
// SandboxedFrame Implementation
// =============================================================================

SandboxedFrame::SandboxedFrame(const std::string& parentOrigin)
    : parentOrigin_(parentOrigin) {
    context_.origin = parentOrigin;
    context_.effectiveOrigin = parentOrigin;
}

SandboxedFrame::~SandboxedFrame() = default;

void SandboxedFrame::setSandbox(const std::string& sandboxAttr) {
    sandboxFlags_ = parseSandboxTokens(sandboxAttr);
    context_.isSandboxed = true;
    updateEffectiveOrigin();
}

SandboxFlag SandboxedFrame::parseSandboxTokens(const std::string& sandboxAttr) {
    SandboxFlag flags = SandboxFlag::None;
    
    std::istringstream iss(sandboxAttr);
    std::string token;
    
    while (iss >> token) {
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        
        if (token == "allow-downloads") {
            flags = flags | SandboxFlag::AllowDownloads;
        } else if (token == "allow-forms") {
            flags = flags | SandboxFlag::AllowForms;
        } else if (token == "allow-modals") {
            flags = flags | SandboxFlag::AllowModals;
        } else if (token == "allow-orientation-lock") {
            flags = flags | SandboxFlag::AllowOrientationLock;
        } else if (token == "allow-pointer-lock") {
            flags = flags | SandboxFlag::AllowPointerLock;
        } else if (token == "allow-popups") {
            flags = flags | SandboxFlag::AllowPopups;
        } else if (token == "allow-popups-to-escape-sandbox") {
            flags = flags | SandboxFlag::AllowPopupsToEscapeSandbox;
        } else if (token == "allow-presentation") {
            flags = flags | SandboxFlag::AllowPresentation;
        } else if (token == "allow-same-origin") {
            flags = flags | SandboxFlag::AllowSameOrigin;
        } else if (token == "allow-scripts") {
            flags = flags | SandboxFlag::AllowScripts;
        } else if (token == "allow-storage-access-by-user-activation") {
            flags = flags | SandboxFlag::AllowStorageAccessByUserActivation;
        } else if (token == "allow-top-navigation") {
            flags = flags | SandboxFlag::AllowTopNavigation;
        } else if (token == "allow-top-navigation-by-user-activation") {
            flags = flags | SandboxFlag::AllowTopNavigationByUserActivation;
        } else if (token == "allow-top-navigation-to-custom-protocols") {
            flags = flags | SandboxFlag::AllowTopNavigationToCustomProtocols;
        }
    }
    
    return flags;
}

void SandboxedFrame::load(const std::string& url) {
    context_.origin = SiteSettingsManager::normalizeOrigin(url);
    updateEffectiveOrigin();
}

bool SandboxedFrame::isSameOriginWithParent() const {
    return SiteSettingsManager::isSameOrigin(context_.effectiveOrigin, parentOrigin_);
}

void SandboxedFrame::updateEffectiveOrigin() {
    if (context_.isSandboxed && !hasFlag(sandboxFlags_, SandboxFlag::AllowSameOrigin)) {
        // Sandboxed without allow-same-origin gets opaque origin
        context_.effectiveOrigin = generateOpaqueOrigin();
    } else {
        context_.effectiveOrigin = context_.origin;
    }
}

std::string SandboxedFrame::generateOpaqueOrigin() {
    // Generate unique opaque origin
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::ostringstream oss;
    oss << "opaque://";
    for (int i = 0; i < 16; ++i) {
        oss << std::hex << std::setfill('0') << std::setw(2) << dis(gen);
    }
    
    return oss.str();
}

bool SandboxedFrame::canRunScripts() const {
    if (!context_.isSandboxed) return true;
    return hasFlag(sandboxFlags_, SandboxFlag::AllowScripts);
}

bool SandboxedFrame::canSubmitForms() const {
    if (!context_.isSandboxed) return true;
    return hasFlag(sandboxFlags_, SandboxFlag::AllowForms);
}

bool SandboxedFrame::canOpenPopups() const {
    if (!context_.isSandboxed) return true;
    return hasFlag(sandboxFlags_, SandboxFlag::AllowPopups);
}

bool SandboxedFrame::canAccessStorage() const {
    if (!context_.isSandboxed) return true;
    
    // Need allow-same-origin to access storage
    if (!hasFlag(sandboxFlags_, SandboxFlag::AllowSameOrigin)) {
        return false;
    }
    
    return true;
}

bool SandboxedFrame::canNavigateTop() const {
    if (!context_.isSandboxed) return true;
    return hasFlag(sandboxFlags_, SandboxFlag::AllowTopNavigation) ||
           hasFlag(sandboxFlags_, SandboxFlag::AllowTopNavigationByUserActivation);
}

bool SandboxedFrame::isFeatureAllowed(const std::string& feature) const {
    // Check permissions policy
    if (context_.allowedFeatures.empty()) return true;  // Default allow all
    return context_.allowedFeatures.count(feature) > 0;
}

void SandboxedFrame::postMessage(const std::string& message, 
                                  const std::string& targetOrigin) {
    // Would dispatch to target frame
    // For now, just validate target origin
    if (targetOrigin != "*" && targetOrigin != context_.effectiveOrigin) {
        return;  // Target origin doesn't match
    }
    
    // Message would be dispatched to frame's message event
}

void SandboxedFrame::receiveMessage(const std::string& message, 
                                     const std::string& sourceOrigin) {
    if (onMessage_) {
        onMessage_(message, sourceOrigin);
    }
}

void SandboxedFrame::setContentSecurityPolicy(const std::string& csp) {
    context_.contentSecurityPolicy = csp;
}

void SandboxedFrame::setPermissionsPolicy(const std::string& policy) {
    // Parse permissions policy
    // e.g., "camera=(self), microphone=()"
    context_.allowedFeatures.clear();
    
    std::istringstream iss(policy);
    std::string token;
    
    while (std::getline(iss, token, ',')) {
        size_t eq = token.find('=');
        if (eq != std::string::npos) {
            std::string feature = token.substr(0, eq);
            // Trim
            feature.erase(0, feature.find_first_not_of(" "));
            feature.erase(feature.find_last_not_of(" ") + 1);
            
            std::string allowlist = token.substr(eq + 1);
            if (allowlist.find("self") != std::string::npos ||
                allowlist.find("*") != std::string::npos) {
                context_.allowedFeatures.insert(feature);
            }
        }
    }
}

bool SandboxedFrame::isResourceAllowedByCSP(const std::string& url,
                                             const std::string& type) const {
    // Simplified CSP check
    if (context_.contentSecurityPolicy.empty()) return true;
    
    // Parse directive for type
    std::string directive;
    if (type == "script") directive = "script-src";
    else if (type == "style") directive = "style-src";
    else if (type == "image") directive = "img-src";
    else if (type == "font") directive = "font-src";
    else if (type == "connect") directive = "connect-src";
    else if (type == "frame") directive = "frame-src";
    else directive = "default-src";
    
    // Find directive in CSP
    size_t pos = context_.contentSecurityPolicy.find(directive);
    if (pos == std::string::npos) {
        // Check default-src
        pos = context_.contentSecurityPolicy.find("default-src");
    }
    
    if (pos == std::string::npos) return true;  // No restriction
    
    // Extract sources
    size_t start = context_.contentSecurityPolicy.find(' ', pos);
    size_t end = context_.contentSecurityPolicy.find(';', pos);
    
    if (start == std::string::npos) return false;
    
    std::string sources = context_.contentSecurityPolicy.substr(
        start, end == std::string::npos ? std::string::npos : end - start);
    
    // Check if allowed
    if (sources.find("'none'") != std::string::npos) return false;
    if (sources.find("*") != std::string::npos) return true;
    if (sources.find("'self'") != std::string::npos) {
        return SiteSettingsManager::isSameOrigin(context_.origin, url);
    }
    
    // Check if URL matches any source
    return sources.find(SiteSettingsManager::normalizeOrigin(url)) != std::string::npos;
}

// =============================================================================
// FrameTree Implementation
// =============================================================================

FrameTree::FrameTree(const std::string& mainOrigin) {
    root_ = std::make_unique<Node>();
    root_->frame = std::make_unique<SandboxedFrame>(mainOrigin);
    root_->name = "_main";
}

SandboxedFrame* FrameTree::addChild(Node* parent, const std::string& name) {
    auto child = std::make_unique<Node>();
    child->frame = std::make_unique<SandboxedFrame>(parent->frame->origin());
    child->name = name;
    child->parent = parent;
    
    SandboxedFrame* frame = child->frame.get();
    parent->children.push_back(std::move(child));
    
    return frame;
}

SandboxedFrame* FrameTree::findFrame(const std::string& name) {
    Node* node = findNode(root_.get(), name);
    return node ? node->frame.get() : nullptr;
}

FrameTree::Node* FrameTree::findNode(Node* node, const std::string& name) {
    if (node->name == name) return node;
    
    for (auto& child : node->children) {
        if (Node* found = findNode(child.get(), name)) {
            return found;
        }
    }
    
    return nullptr;
}

std::vector<SandboxedFrame*> FrameTree::getAncestors(Node* node) {
    std::vector<SandboxedFrame*> ancestors;
    
    Node* current = node->parent;
    while (current) {
        ancestors.push_back(current->frame.get());
        current = current->parent;
    }
    
    return ancestors;
}

} // namespace Zepra::WebCore
