/**
 * @file gc_bridge.cpp
 * @brief GC Bridge implementation for JS/DOM memory management
 */

#include "webcore/gc_bridge.hpp"
#include "webcore/dom.hpp"
#include <algorithm>

namespace Zepra::WebCore {

// =============================================================================
// GCBridge Implementation
// =============================================================================

GCBridge& GCBridge::instance() {
    static GCBridge instance;
    return instance;
}

GCHandle GCBridge::nextHandle() {
    return nextHandle_++;
}

GCHandle GCBridge::registerDOMWrapper(DOMNode* domNode, Runtime::Object* jsWrapper) {
    if (!domNode || !jsWrapper) return INVALID_GC_HANDLE;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if already registered
    auto it = domToWrapper_.find(domNode);
    if (it != domToWrapper_.end()) {
        // Update existing wrapper
        auto& ref = jsToDomRefs_[it->second];
        ref.jsWrapper = jsWrapper;
        wrapperToDom_[jsWrapper] = it->second;
        return it->second;
    }
    
    // Create new tracking entry
    GCHandle handle = nextHandle();
    
    JSToDOMRef ref;
    ref.handle = handle;
    ref.domNode = domNode;
    ref.jsWrapper = jsWrapper;
    ref.isStrong = true;
    ref.status = GCRefStatus::Active;
    
    jsToDomRefs_[handle] = ref;
    domToWrapper_[domNode] = handle;
    wrapperToDom_[jsWrapper] = handle;
    
    return handle;
}

GCHandle GCBridge::registerJSCallback(void* domNode, Runtime::Object* jsCallback,
                                       const std::string& eventType) {
    if (!domNode || !jsCallback) return INVALID_GC_HANDLE;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    GCHandle handle = nextHandle();
    
    DOMToJSRef ref;
    ref.handle = handle;
    ref.jsObject = jsCallback;
    ref.domNode = domNode;
    ref.eventType = eventType;
    ref.status = GCRefStatus::Active;
    
    domToJsRefs_[handle] = ref;
    domNodeCallbacks_[domNode].push_back(handle);
    
    return handle;
}

void GCBridge::unregister(GCHandle handle) {
    if (handle == INVALID_GC_HANDLE) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check JS->DOM refs
    auto jsToDomIt = jsToDomRefs_.find(handle);
    if (jsToDomIt != jsToDomRefs_.end()) {
        auto& ref = jsToDomIt->second;
        domToWrapper_.erase(ref.domNode);
        wrapperToDom_.erase(ref.jsWrapper);
        jsToDomRefs_.erase(jsToDomIt);
        return;
    }
    
    // Check DOM->JS refs
    auto domToJsIt = domToJsRefs_.find(handle);
    if (domToJsIt != domToJsRefs_.end()) {
        auto& ref = domToJsIt->second;
        
        // Remove from domNodeCallbacks
        auto& callbacks = domNodeCallbacks_[ref.domNode];
        callbacks.erase(std::remove(callbacks.begin(), callbacks.end(), handle), 
                        callbacks.end());
        if (callbacks.empty()) {
            domNodeCallbacks_.erase(ref.domNode);
        }
        
        domToJsRefs_.erase(domToJsIt);
    }
    
    // Check weak refs
    weakRefs_.erase(handle);
}

Runtime::Object* GCBridge::getWrapper(DOMNode* domNode) {
    if (!domNode) return nullptr;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = domToWrapper_.find(domNode);
    if (it == domToWrapper_.end()) return nullptr;
    
    auto refIt = jsToDomRefs_.find(it->second);
    if (refIt == jsToDomRefs_.end()) return nullptr;
    
    return refIt->second.jsWrapper;
}

DOMNode* GCBridge::getDOMNode(Runtime::Object* jsWrapper) {
    if (!jsWrapper) return nullptr;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = wrapperToDom_.find(jsWrapper);
    if (it == wrapperToDom_.end()) return nullptr;
    
    auto refIt = jsToDomRefs_.find(it->second);
    if (refIt == jsToDomRefs_.end()) return nullptr;
    
    return refIt->second.domNode;
}

void GCBridge::onDOMNodeDestroyed(DOMNode* domNode) {
    if (!domNode) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get the wrapper handle
    auto wrapperIt = domToWrapper_.find(domNode);
    if (wrapperIt != domToWrapper_.end()) {
        GCHandle handle = wrapperIt->second;
        
        // Mark as dead, don't delete immediately (GC will clean up)
        auto refIt = jsToDomRefs_.find(handle);
        if (refIt != jsToDomRefs_.end()) {
            refIt->second.domNode = nullptr;
            refIt->second.status = GCRefStatus::Dead;
            wrapperToDom_.erase(refIt->second.jsWrapper);
        }
        
        domToWrapper_.erase(wrapperIt);
    }
    
    // Invalidate any callbacks held by this DOM node
    auto callbacksIt = domNodeCallbacks_.find(domNode);
    if (callbacksIt != domNodeCallbacks_.end()) {
        for (GCHandle handle : callbacksIt->second) {
            auto refIt = domToJsRefs_.find(handle);
            if (refIt != domToJsRefs_.end()) {
                refIt->second.status = GCRefStatus::Dead;
                refIt->second.domNode = nullptr;
                // Add to pending finalizations
                pendingFinalizations_.push_back(handle);
            }
        }
        domNodeCallbacks_.erase(callbacksIt);
    }
    
    // Invalidate weak refs
    for (auto& [handle, node] : weakRefs_) {
        if (node == domNode) {
            node = nullptr;
        }
    }
}

void GCBridge::onJSObjectCollected(Runtime::Object* jsObject) {
    if (!jsObject) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Clean up wrapper reference
    auto wrapperIt = wrapperToDom_.find(jsObject);
    if (wrapperIt != wrapperToDom_.end()) {
        GCHandle handle = wrapperIt->second;
        
        auto refIt = jsToDomRefs_.find(handle);
        if (refIt != jsToDomRefs_.end()) {
            domToWrapper_.erase(refIt->second.domNode);
            jsToDomRefs_.erase(refIt);
        }
        
        wrapperToDom_.erase(wrapperIt);
    }
    
    // Clean up any callbacks that were this object
    for (auto it = domToJsRefs_.begin(); it != domToJsRefs_.end(); ) {
        if (it->second.jsObject == jsObject) {
            // Remove from domNodeCallbacks
            auto& callbacks = domNodeCallbacks_[it->second.domNode];
            callbacks.erase(std::remove(callbacks.begin(), callbacks.end(), it->first),
                            callbacks.end());
            it = domToJsRefs_.erase(it);
        } else {
            ++it;
        }
    }
}

void GCBridge::markReachableObjects(std::function<void(Runtime::Object*)> marker) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Mark all JS callbacks held by active DOM nodes
    for (auto& [handle, ref] : domToJsRefs_) {
        if (ref.status == GCRefStatus::Active && ref.domNode != nullptr && ref.jsObject) {
            marker(ref.jsObject);
        }
    }
}

void GCBridge::sweep() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Remove dead JS->DOM refs
    for (auto it = jsToDomRefs_.begin(); it != jsToDomRefs_.end(); ) {
        if (it->second.status == GCRefStatus::Dead) {
            it = jsToDomRefs_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Remove dead DOM->JS refs
    for (auto it = domToJsRefs_.begin(); it != domToJsRefs_.end(); ) {
        if (it->second.status == GCRefStatus::Dead) {
            it = domToJsRefs_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Remove dead weak refs
    for (auto it = weakRefs_.begin(); it != weakRefs_.end(); ) {
        if (it->second == nullptr) {
            it = weakRefs_.erase(it);
        } else {
            ++it;
        }
    }
}

void GCBridge::collectGarbage() {
    // In a real implementation, this would coordinate with the VM's GC
    sweep();
    processFinalizers();
}

GCHandle GCBridge::createWeakRef(DOMNode* domNode) {
    if (!domNode) return INVALID_GC_HANDLE;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    GCHandle handle = nextHandle();
    weakRefs_[handle] = domNode;
    return handle;
}

bool GCBridge::isWeakRefAlive(GCHandle handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = weakRefs_.find(handle);
    return it != weakRefs_.end() && it->second != nullptr;
}

DOMNode* GCBridge::derefWeakRef(GCHandle handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = weakRefs_.find(handle);
    return it != weakRefs_.end() ? it->second : nullptr;
}

void GCBridge::registerFinalizer(GCHandle handle, FinalizerCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    finalizers_.emplace_back(handle, std::move(callback));
}

void GCBridge::processFinalizers() {
    std::vector<std::pair<GCHandle, FinalizerCallback>> toRun;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Find finalizers for pending handles
        for (GCHandle handle : pendingFinalizations_) {
            for (auto it = finalizers_.begin(); it != finalizers_.end(); ) {
                if (it->first == handle) {
                    toRun.push_back(std::move(*it));
                    it = finalizers_.erase(it);
                } else {
                    ++it;
                }
            }
        }
        pendingFinalizations_.clear();
    }
    
    // Run finalizers outside the lock
    for (auto& [handle, callback] : toRun) {
        if (callback) {
            callback(handle);
        }
    }
}

size_t GCBridge::activeWrapperCount() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    return jsToDomRefs_.size();
}

size_t GCBridge::activeCallbackCount() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    return domToJsRefs_.size();
}

size_t GCBridge::pendingFinalizerCount() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    return finalizers_.size();
}

// =============================================================================
// GCGuard Implementation
// =============================================================================

GCGuard::GCGuard(Runtime::VM* vm) : vm_(vm) {
    // In a real implementation, this would pause GC
    // vm_->pauseGC();
}

GCGuard::~GCGuard() {
    // In a real implementation, this would resume GC
    // vm_->resumeGC();
}

} // namespace Zepra::WebCore
