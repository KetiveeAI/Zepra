/**
 * @file js_realm.hpp
 * @brief Sandboxed JavaScript execution realms for multi-tab/iframe isolation
 * 
 * Each realm has its own:
 * - Global object scope
 * - DOM bindings
 * - SecurityOrigin
 * - Message passing channel
 */

#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>
#include <mutex>
#include <atomic>

namespace Zepra {
namespace Runtime {
    class VM;
    class Object;
    class Value;
}

namespace WebCore {

class DOMDocument;
class ScriptContext;

/**
 * @brief Security origin for cross-origin policy
 */
class SecurityOrigin {
public:
    SecurityOrigin() = default;
    SecurityOrigin(const std::string& protocol, const std::string& host, int port);
    
    static SecurityOrigin fromURL(const std::string& url);
    static SecurityOrigin unique();  // opaque origin (e.g., data URLs)
    
    bool isSameOrigin(const SecurityOrigin& other) const;
    bool canAccess(const SecurityOrigin& other) const;
    
    std::string serialize() const;  // "protocol://host:port"
    
    bool isSecure() const { return protocol_ == "https" || protocol_ == "wss"; }
    bool isLocalFile() const { return protocol_ == "file"; }
    bool isOpaque() const { return opaque_; }
    
    const std::string& protocol() const { return protocol_; }
    const std::string& host() const { return host_; }
    int port() const { return port_; }

private:
    std::string protocol_;
    std::string host_;
    int port_ = 0;
    bool opaque_ = false;
    std::string opaqueId_;
};

/**
 * @brief Message for cross-realm communication
 */
struct RealmMessage {
    uint64_t sourceRealmId = 0;
    uint64_t targetRealmId = 0;
    std::string type;           // "message", "error", etc.
    std::string data;           // Serialized data (JSON)
    std::string origin;         // Source origin
    std::vector<uint64_t> ports;  // MessagePort IDs (for structured clone)
};

/**
 * @brief Callback for incoming messages
 */
using RealmMessageCallback = std::function<void(const RealmMessage&)>;

/**
 * @brief A JavaScript execution realm (isolated context)
 * 
 * Realms are used for:
 * - Each browser tab
 * - Each iframe (potentially cross-origin)
 * - Each Web Worker
 * - Each Service Worker
 */
class JSRealm {
public:
    /**
     * @brief Realm type
     */
    enum class Type {
        Window,         // Main window/tab
        IFrame,         // Embedded iframe
        Worker,         // Web Worker
        ServiceWorker,  // Service Worker
        Worklet         // Audio/Paint Worklet
    };
    
    JSRealm(Type type, const SecurityOrigin& origin);
    ~JSRealm();
    
    // Non-copyable
    JSRealm(const JSRealm&) = delete;
    JSRealm& operator=(const JSRealm&) = delete;
    
    /**
     * @brief Initialize the realm's runtime
     */
    bool initialize();
    
    /**
     * @brief Attach DOM document to realm
     */
    void attachDocument(DOMDocument* doc);
    
    /**
     * @brief Get the realm's unique ID
     */
    uint64_t id() const { return id_; }
    
    /**
     * @brief Get realm type
     */
    Type type() const { return type_; }
    
    /**
     * @brief Get security origin
     */
    const SecurityOrigin& origin() const { return origin_; }
    
    /**
     * @brief Execute script in this realm
     */
    void evaluate(const std::string& script);
    
    /**
     * @brief Get the realm's global object
     */
    Runtime::Object* globalObject();
    
    /**
     * @brief Get the VM instance for this realm
     */
    Runtime::VM* vm() { return vm_.get(); }
    
    /**
     * @brief Get the script context
     */
    ScriptContext* scriptContext() { return scriptContext_.get(); }
    
    // =====================
    // Cross-realm communication
    // =====================
    
    /**
     * @brief Post a message to this realm
     */
    void postMessage(const RealmMessage& message);
    
    /**
     * @brief Set handler for incoming messages
     */
    void setMessageHandler(RealmMessageCallback callback);
    
    /**
     * @brief Process pending messages (call from event loop)
     */
    void processMessages();
    
    // =====================
    // Realm lifecycle
    // =====================
    
    /**
     * @brief Check if realm is active
     */
    bool isActive() const { return active_; }
    
    /**
     * @brief Destroy the realm (cleanup)
     */
    void destroy();
    
    // =====================
    // Parent/child relationships
    // =====================
    
    /**
     * @brief Set parent realm (for iframes)
     */
    void setParentRealm(JSRealm* parent);
    
    /**
     * @brief Get parent realm
     */
    JSRealm* parentRealm() const { return parentRealm_; }
    
    /**
     * @brief Add child realm
     */
    void addChildRealm(JSRealm* child);
    
    /**
     * @brief Remove child realm
     */
    void removeChildRealm(JSRealm* child);
    
    /**
     * @brief Get all child realms
     */
    const std::vector<JSRealm*>& childRealms() const { return childRealms_; }

private:
    /**
     * @brief Setup global object with realm-specific globals
     */
    void setupGlobals();
    
    /**
     * @brief Setup postMessage/addEventListener for messaging
     */
    void setupMessageAPI();

    uint64_t id_;
    Type type_;
    SecurityOrigin origin_;
    
    std::unique_ptr<Runtime::VM> vm_;
    std::unique_ptr<ScriptContext> scriptContext_;
    DOMDocument* document_ = nullptr;
    
    // Relationships
    JSRealm* parentRealm_ = nullptr;
    std::vector<JSRealm*> childRealms_;
    
    // Messaging
    std::mutex messageMutex_;
    std::vector<RealmMessage> messageQueue_;
    RealmMessageCallback messageHandler_;
    
    // State
    std::atomic<bool> active_{false};
    
    // ID generator
    static std::atomic<uint64_t> nextId_;
};

/**
 * @brief Manages all realms in the browser
 */
class RealmManager {
public:
    static RealmManager& instance();
    
    /**
     * @brief Create a new realm
     */
    JSRealm* createRealm(JSRealm::Type type, const SecurityOrigin& origin);
    
    /**
     * @brief Get realm by ID
     */
    JSRealm* getRealm(uint64_t id);
    
    /**
     * @brief Destroy a realm
     */
    void destroyRealm(uint64_t id);
    
    /**
     * @brief Destroy all realms
     */
    void destroyAllRealms();
    
    /**
     * @brief Post message between realms
     */
    bool postMessage(uint64_t sourceId, uint64_t targetId, const std::string& data);
    
    /**
     * @brief Process all realm message queues
     */
    void processAllMessages();
    
    /**
     * @brief Get count of active realms
     */
    size_t realmCount() const;

private:
    RealmManager() = default;
    
    std::mutex realmsMutex_;
    std::unordered_map<uint64_t, std::unique_ptr<JSRealm>> realms_;
};

/**
 * @brief Cross-origin isolation check utility
 */
class CrossOriginChecker {
public:
    /**
     * @brief Check if source can access target realm
     */
    static bool canAccess(const JSRealm* source, const JSRealm* target);
    
    /**
     * @brief Check if script can read from URL
     */
    static bool canReadFromURL(const SecurityOrigin& origin, const std::string& url);
    
    /**
     * @brief Check if can post message
     */
    static bool canPostMessage(const JSRealm* source, const JSRealm* target, 
                                const std::string& targetOrigin);
};

} // namespace WebCore
} // namespace Zepra
