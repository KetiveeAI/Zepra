/**
 * @file gc_bridge.hpp
 * @brief Garbage Collection bridge between JavaScript and DOM
 * 
 * This module handles the complex relationship between:
 * - JavaScript objects (managed by VM's GC)
 * - DOM nodes (managed by C++ unique_ptr/shared_ptr)
 * - Event listeners and callbacks
 * 
 * Key concepts:
 * - Weak references from JS wrappers to DOM nodes
 * - Mark-and-sweep coordination
 * - prevent leaks from cyclic references
 */

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>

namespace Zepra {

namespace Runtime {
    class Object;
    class Value;
    class VM;
}

namespace WebCore {

class DOMNode;

/**
 * @brief Handle wrapper for GC-tracked objects
 */
using GCHandle = uint64_t;
constexpr GCHandle INVALID_GC_HANDLE = 0;

/**
 * @brief Status of a GC-tracked reference
 */
enum class GCRefStatus {
    Active,     // Reference is alive
    Weak,       // Weak reference (can be collected)
    Dead        // Already collected
};

/**
 * @brief A reference from DOM to a JS callback
 * Used for event listeners, setTimeout callbacks, etc.
 */
struct DOMToJSRef {
    GCHandle handle = INVALID_GC_HANDLE;
    Runtime::Object* jsObject = nullptr;  // The JS callback/closure
    void* domNode = nullptr;              // The DOM node holding the ref
    std::string eventType;                // If event listener, the event type
    GCRefStatus status = GCRefStatus::Active;
};

/**
 * @brief A reference from JS wrapper to DOM node
 */
struct JSToDOMRef {
    GCHandle handle = INVALID_GC_HANDLE;
    DOMNode* domNode = nullptr;           // The underlying DOM node
    Runtime::Object* jsWrapper = nullptr;  // The JS wrapper object
    bool isStrong = true;                  // Strong vs weak reference
    GCRefStatus status = GCRefStatus::Active;
};

/**
 * @brief GC Bridge manages the bidirectional references between JS and DOM
 * 
 * It ensures that:
 * 1. DOM nodes are not deleted while JS has references
 * 2. JS objects holding DOM refs are marked during GC
 * 3. Event listeners don't create leaks
 * 4. WeakRefs work properly across the bridge
 */
class GCBridge {
public:
    static GCBridge& instance();
    
    /**
     * @brief Register a DOM node being wrapped in JS
     * @param domNode The DOM node
     * @param jsWrapper The JavaScript wrapper object
     * @return Handle for tracking
     */
    GCHandle registerDOMWrapper(DOMNode* domNode, Runtime::Object* jsWrapper);
    
    /**
     * @brief Register a JS callback held by DOM (e.g., event listener)
     * @param domNode The DOM node holding the callback
     * @param jsCallback The JavaScript callback
     * @param eventType Optional event type for event listeners
     * @return Handle for tracking
     */
                                 const std::string& eventType = "");
    
    /**
     * @brief Unregister a reference by handle
     */
    void unregister(GCHandle handle);
    
    /**
     * @brief Get JS wrapper for a DOM node (if exists)
     */
    Runtime::Object* getWrapper(DOMNode* domNode);
    
    /**
     * @brief Get DOM node from JS wrapper (if valid)
     */
    DOMNode* getDOMNode(Runtime::Object* jsWrapper);
    
    /**
     * @brief Called when a DOM node is being destroyed
     * Invalidates all JS references to it
     */
    void onDOMNodeDestroyed(DOMNode* domNode);
    
    /**
     * @brief Called when a JS object is being collected
     * Cleans up DOM->JS references
     */
    void onJSObjectCollected(Runtime::Object* jsObject);
    
    // -----------------------
    // GC Cycle Integration
    // -----------------------
    
    /**
     * @brief Mark phase: called by VM's GC to mark reachable objects
     * This ensures DOM-held callbacks are not collected
     */
    void markReachableObjects(std::function<void(Runtime::Object*)> marker);
    
    /**
     * @brief Sweep phase: clean up dead references
     */
    void sweep();
    
    /**
     * @brief Force a full GC cycle
     */
    void collectGarbage();
    
    // -----------------------
    // Weak References
    // -----------------------
    
    /**
     * @brief Create a weak reference to a DOM node
     * WeakRef can be checked but won't prevent collection
     */
    GCHandle createWeakRef(DOMNode* domNode);
    
    /**
     * @brief Check if weak ref is still valid
     */
    bool isWeakRefAlive(GCHandle handle);
    
    /**
     * @brief Dereference weak ref (returns nullptr if collected)
     */
    DOMNode* derefWeakRef(GCHandle handle);
    
    // -----------------------
    // FinalizationRegistry support
    // -----------------------
    
    using FinalizerCallback = std::function<void(GCHandle)>;
    
    /**
     * @brief Register a callback to run when object is collected
     */
    void registerFinalizer(GCHandle handle, FinalizerCallback callback);
    
    /**
     * @brief Process pending finalizers
     * Call from main thread/event loop
     */
    void processFinalizers();
    
    // -----------------------
    // Statistics
    // -----------------------
    
    size_t activeWrapperCount() const;
    size_t activeCallbackCount() const;
    size_t pendingFinalizerCount() const;

private:
    GCBridge() = default;
    
    GCHandle nextHandle();
    
    std::mutex mutex_;
    std::atomic<uint64_t> nextHandle_{1};
    
    // DOM -> JS references (e.g., event listeners)
    std::unordered_map<GCHandle, DOMToJSRef> domToJsRefs_;
    
    // JS -> DOM references (wrappers)
    std::unordered_map<GCHandle, JSToDOMRef> jsToDomRefs_;
    
    // Quick lookup: DOM node -> all its JS callbacks
    std::unordered_map<void*, std::vector<GCHandle>> domNodeCallbacks_;
    
    // Quick lookup: DOM node -> its JS wrapper
    std::unordered_map<DOMNode*, GCHandle> domToWrapper_;
    
    // Quick lookup: JS wrapper -> its DOM node
    std::unordered_map<Runtime::Object*, GCHandle> wrapperToDom_;
    
    // Weak references
    std::unordered_map<GCHandle, DOMNode*> weakRefs_;
    
    // Pending finalizers
    std::vector<std::pair<GCHandle, FinalizerCallback>> finalizers_;
    std::vector<GCHandle> pendingFinalizations_;
};

/**
 * @brief RAII guard for preventing GC during critical sections
 */
class GCGuard {
public:
    explicit GCGuard(Runtime::VM* vm);
    ~GCGuard();
    
    GCGuard(const GCGuard&) = delete;
    GCGuard& operator=(const GCGuard&) = delete;
    
private:
    Runtime::VM* vm_;
};

/**
 * @brief Helper to ensure proper reference counting
 */
template<typename T>
class GCTrackedRef {
public:
    GCTrackedRef() = default;
    explicit GCTrackedRef(T* ptr, Runtime::Object* wrapper = nullptr);
    ~GCTrackedRef();
    
    GCTrackedRef(const GCTrackedRef& other);
    GCTrackedRef& operator=(const GCTrackedRef& other);
    GCTrackedRef(GCTrackedRef&& other) noexcept;
    GCTrackedRef& operator=(GCTrackedRef&& other) noexcept;
    
    T* get() const { return ptr_; }
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    explicit operator bool() const { return ptr_ != nullptr; }
    
    void reset();
    
private:
    T* ptr_ = nullptr;
    GCHandle handle_ = INVALID_GC_HANDLE;
};

} // namespace WebCore
} // namespace Zepra
