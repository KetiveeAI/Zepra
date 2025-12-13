/**
 * @file js_realm.cpp
 * @brief Sandboxed JavaScript execution realms implementation
 */

#include "webcore/js_realm.hpp"
#include "webcore/script_context.hpp"
#include "zeprascript/runtime/vm.hpp"
#include "zeprascript/runtime/object.hpp"
#include <sstream>
#include <random>
#include <algorithm>

namespace Zepra::WebCore {

// Static ID generator
std::atomic<uint64_t> JSRealm::nextId_{1};

// =============================================================================
// SecurityOrigin Implementation
// =============================================================================

SecurityOrigin::SecurityOrigin(const std::string& protocol, const std::string& host, int port)
    : protocol_(protocol), host_(host), port_(port), opaque_(false) {}

SecurityOrigin SecurityOrigin::fromURL(const std::string& url) {
    SecurityOrigin origin;
    
    // Simple URL parsing
    size_t protocolEnd = url.find("://");
    if (protocolEnd == std::string::npos) {
        // Relative URL or invalid
        return SecurityOrigin::unique();
    }
    
    origin.protocol_ = url.substr(0, protocolEnd);
    size_t hostStart = protocolEnd + 3;
    size_t hostEnd = url.find_first_of("/?#", hostStart);
    if (hostEnd == std::string::npos) {
        hostEnd = url.length();
    }
    
    std::string hostPort = url.substr(hostStart, hostEnd - hostStart);
    size_t colonPos = hostPort.rfind(':');
    
    if (colonPos != std::string::npos && hostPort.find('[') == std::string::npos) {
        // IPv4 or hostname with port
        origin.host_ = hostPort.substr(0, colonPos);
        try {
            origin.port_ = std::stoi(hostPort.substr(colonPos + 1));
        } catch (...) {
            origin.host_ = hostPort;
            origin.port_ = 0;
        }
    } else {
        origin.host_ = hostPort;
        origin.port_ = 0;
    }
    
    // Default ports
    if (origin.port_ == 0) {
        if (origin.protocol_ == "http" || origin.protocol_ == "ws") {
            origin.port_ = 80;
        } else if (origin.protocol_ == "https" || origin.protocol_ == "wss") {
            origin.port_ = 443;
        }
    }
    
    origin.opaque_ = false;
    
    return origin;
}

SecurityOrigin SecurityOrigin::unique() {
    SecurityOrigin origin;
    origin.opaque_ = true;
    
    // Generate unique opaque ID
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    std::stringstream ss;
    ss << "opaque-" << dis(gen);
    origin.opaqueId_ = ss.str();
    
    return origin;
}

bool SecurityOrigin::isSameOrigin(const SecurityOrigin& other) const {
    if (opaque_ || other.opaque_) {
        return false;  // Opaque origins are never same-origin
    }
    return protocol_ == other.protocol_ &&
           host_ == other.host_ &&
           port_ == other.port_;
}

bool SecurityOrigin::canAccess(const SecurityOrigin& other) const {
    // Same-origin check
    if (isSameOrigin(other)) return true;
    
    // document.domain relaxation could go here
    // CORS could be checked here
    
    return false;
}

std::string SecurityOrigin::serialize() const {
    if (opaque_) {
        return "null";  // Opaque origins serialize to "null"
    }
    
    std::stringstream ss;
    ss << protocol_ << "://" << host_;
    
    // Only include non-default ports
    bool isDefaultPort = (protocol_ == "http" && port_ == 80) ||
                         (protocol_ == "https" && port_ == 443) ||
                         port_ == 0;
    if (!isDefaultPort) {
        ss << ":" << port_;
    }
    
    return ss.str();
}

// =============================================================================
// JSRealm Implementation
// =============================================================================

JSRealm::JSRealm(Type type, const SecurityOrigin& origin)
    : id_(nextId_++), type_(type), origin_(origin) {}

JSRealm::~JSRealm() {
    destroy();
}

bool JSRealm::initialize() {
    if (active_) return true;
    
    try {
        // Create script context for this realm
        // Note: Each realm gets its own ScriptContext which manages VM isolation
        scriptContext_ = std::make_unique<ScriptContext>();
        
        // Setup globals specific to realm type
        setupGlobals();
        setupMessageAPI();
        
        active_ = true;
        return true;
    } catch (...) {
        return false;
    }
}

void JSRealm::attachDocument(DOMDocument* doc) {
    document_ = doc;
    if (scriptContext_ && doc) {
        scriptContext_->initialize(doc);
    }
}

void JSRealm::evaluate(const std::string& script) {
    if (!active_ || !scriptContext_) return;
    scriptContext_->evaluate(script);
}

Runtime::Object* JSRealm::globalObject() {
    // Global object is managed through ScriptContext
    // Return nullptr as placeholder - real impl uses ScriptContext's VM access
    return nullptr;
}

void JSRealm::postMessage(const RealmMessage& message) {
    std::lock_guard<std::mutex> lock(messageMutex_);
    messageQueue_.push_back(message);
}

void JSRealm::setMessageHandler(RealmMessageCallback callback) {
    messageHandler_ = std::move(callback);
}

void JSRealm::processMessages() {
    if (!messageHandler_) return;
    
    std::vector<RealmMessage> messages;
    {
        std::lock_guard<std::mutex> lock(messageMutex_);
        messages.swap(messageQueue_);
    }
    
    for (const auto& msg : messages) {
        // Check cross-origin access
        if (!origin_.isOpaque()) {
            // Process the message
            messageHandler_(msg);
        }
    }
}

void JSRealm::destroy() {
    if (!active_) return;
    
    active_ = false;
    
    // Remove from parent
    if (parentRealm_) {
        parentRealm_->removeChildRealm(this);
        parentRealm_ = nullptr;
    }
    
    // Destroy children
    for (auto* child : childRealms_) {
        child->parentRealm_ = nullptr;
        child->destroy();
    }
    childRealms_.clear();
    
    // Cleanup
    scriptContext_.reset();
    vm_.reset();
    messageQueue_.clear();
}

void JSRealm::setParentRealm(JSRealm* parent) {
    parentRealm_ = parent;
}

void JSRealm::addChildRealm(JSRealm* child) {
    if (child && std::find(childRealms_.begin(), childRealms_.end(), child) == childRealms_.end()) {
        childRealms_.push_back(child);
        child->setParentRealm(this);
    }
}

void JSRealm::removeChildRealm(JSRealm* child) {
    childRealms_.erase(
        std::remove(childRealms_.begin(), childRealms_.end(), child),
        childRealms_.end()
    );
}

void JSRealm::setupGlobals() {
    // Realm-type specific globals would be set up here
    // Window realms get window, document, etc.
    // Worker realms get self, importScripts, etc.
}

void JSRealm::setupMessageAPI() {
    // postMessage and addEventListener for "message" events
    // would be exposed to the JS runtime here
}

// =============================================================================
// RealmManager Implementation
// =============================================================================

RealmManager& RealmManager::instance() {
    static RealmManager instance;
    return instance;
}

JSRealm* RealmManager::createRealm(JSRealm::Type type, const SecurityOrigin& origin) {
    auto realm = std::make_unique<JSRealm>(type, origin);
    
    if (!realm->initialize()) {
        return nullptr;
    }
    
    uint64_t id = realm->id();
    std::lock_guard<std::mutex> lock(realmsMutex_);
    realms_[id] = std::move(realm);
    return realms_[id].get();
}

JSRealm* RealmManager::getRealm(uint64_t id) {
    std::lock_guard<std::mutex> lock(realmsMutex_);
    auto it = realms_.find(id);
    return it != realms_.end() ? it->second.get() : nullptr;
}

void RealmManager::destroyRealm(uint64_t id) {
    std::lock_guard<std::mutex> lock(realmsMutex_);
    auto it = realms_.find(id);
    if (it != realms_.end()) {
        it->second->destroy();
        realms_.erase(it);
    }
}

void RealmManager::destroyAllRealms() {
    std::lock_guard<std::mutex> lock(realmsMutex_);
    for (auto& [id, realm] : realms_) {
        realm->destroy();
    }
    realms_.clear();
}

bool RealmManager::postMessage(uint64_t sourceId, uint64_t targetId, const std::string& data) {
    JSRealm* source = getRealm(sourceId);
    JSRealm* target = getRealm(targetId);
    
    if (!source || !target) return false;
    
    RealmMessage msg;
    msg.sourceRealmId = sourceId;
    msg.targetRealmId = targetId;
    msg.type = "message";
    msg.data = data;
    msg.origin = source->origin().serialize();
    
    target->postMessage(msg);
    return true;
}

void RealmManager::processAllMessages() {
    std::vector<JSRealm*> activeRealms;
    {
        std::lock_guard<std::mutex> lock(realmsMutex_);
        for (auto& [id, realm] : realms_) {
            if (realm->isActive()) {
                activeRealms.push_back(realm.get());
            }
        }
    }
    
    for (auto* realm : activeRealms) {
        realm->processMessages();
    }
}

size_t RealmManager::realmCount() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(realmsMutex_));
    return realms_.size();
}

// =============================================================================
// CrossOriginChecker Implementation
// =============================================================================

bool CrossOriginChecker::canAccess(const JSRealm* source, const JSRealm* target) {
    if (!source || !target) return false;
    return source->origin().canAccess(target->origin());
}

bool CrossOriginChecker::canReadFromURL(const SecurityOrigin& origin, const std::string& url) {
    SecurityOrigin urlOrigin = SecurityOrigin::fromURL(url);
    return origin.canAccess(urlOrigin);
}

bool CrossOriginChecker::canPostMessage(const JSRealm* source, const JSRealm* target, 
                                         const std::string& targetOrigin) {
    if (!source || !target) return false;
    
    // "*" allows any origin
    if (targetOrigin == "*") return true;
    
    // "/" means same origin as source
    if (targetOrigin == "/") {
        return source->origin().isSameOrigin(target->origin());
    }
    
    // Specific origin check
    SecurityOrigin allowed = SecurityOrigin::fromURL(targetOrigin);
    return target->origin().isSameOrigin(allowed);
}

} // namespace Zepra::WebCore
