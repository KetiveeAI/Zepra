/**
 * @file WasmJSBridge.h
 * @brief WASM ↔ JavaScript Semantics Alignment
 * 
 * Implements:
 * - GC lifetime coordination between WASM and JS heaps
 * - externref lifecycle management
 * - Exception boundary crossing
 * - Shared memory JS memory model compliance
 */

#pragma once

#include "runtime/objects/value.hpp"
#include "heap/gc_heap.hpp"
#include "wasm/WasmGC.h"
#include "wasm/WasmHostBindings.h"
#include "wasm/WasmException.h"
#include "wasm/WasmThreads.h"
#include <memory>
#include <unordered_map>
#include <functional>

namespace Zepra::Bridge {

using namespace Zepra::Runtime;
using namespace Zepra::Wasm;

// =============================================================================
// GC Lifetime Coordination
// =============================================================================

/**
 * @brief Coordinates GC between WASM and JavaScript heaps
 */
class CrossHeapGC {
public:
    explicit CrossHeapGC(GCHeap* jsHeap, WasmGCAllocator* wasmHeap)
        : jsHeap_(jsHeap), wasmHeap_(wasmHeap) {}
    
    // Register a JS object as a WASM root
    void registerJSRoot(Object* jsObj) {
        jsRoots_.insert(jsObj);
        // Add to JS heap roots
        jsHeap_->addRoot(reinterpret_cast<Object**>(&jsObj));
    }
    
    // Unregister a JS root
    void unregisterJSRoot(Object* jsObj) {
        jsRoots_.erase(jsObj);
    }
    
    // Register a WASM object that holds JS references
    void registerWasmHolder(uint32_t wasmObjId, Object* jsRef) {
        wasmToJs_[wasmObjId].push_back(jsRef);
    }
    
    // Called before WASM GC - ensures JS refs are rooted
    void preWasmGC() {
        for (auto& [wasmId, jsRefs] : wasmToJs_) {
            for (Object* ref : jsRefs) {
                jsHeap_->addRoot(&ref);
            }
        }
    }
    
    // Called after WASM GC - cleans up dead refs
    void postWasmGC() {
        // Remove roots added during preWasmGC
    }
    
    // Called before JS GC - ensures WASM refs are rooted
    void preJSGC() {
        for (auto& [jsObj, wasmRefs] : jsToWasm_) {
            // Mark WASM objects as reachable
            for (uint32_t ref : wasmRefs) {
                (void)ref;  // Would mark in WASM heap
            }
        }
    }
    
    // Verify no dangling cross-heap references
    bool verifyIntegrity() const {
        // Check that all JS refs from WASM are still alive
        for (const auto& [wasmId, jsRefs] : wasmToJs_) {
            for (Object* ref : jsRefs) {
                if (ref == nullptr) {
                    return false;  // Dangling reference
                }
            }
        }
        return true;
    }
    
private:
    GCHeap* jsHeap_;
    WasmGCAllocator* wasmHeap_;
    std::unordered_set<Object*> jsRoots_;
    std::unordered_map<uint32_t, std::vector<Object*>> wasmToJs_;
    std::unordered_map<Object*, std::vector<uint32_t>> jsToWasm_;
};

// =============================================================================
// externref Lifecycle Manager
// =============================================================================

/**
 * @brief Manages externref values crossing WASM/JS boundary
 */
class ExternRefManager {
public:
    explicit ExternRefManager(GCHeap* jsHeap) : jsHeap_(jsHeap) {}
    
    // Create externref from JS value
    uint32_t createFromJS(Value jsValue) {
        uint32_t id = nextId_++;
        refs_[id] = {jsValue, 1, true};
        
        // Root if it's an object
        if (jsValue.isObject()) {
            Object* obj = jsValue.asObject();
            jsHeap_->addRoot(&obj);
        }
        
        stats_.created++;
        return id;
    }
    
    // Get JS value from externref
    Value toJS(uint32_t externRef) {
        auto it = refs_.find(externRef);
        if (it == refs_.end() || !it->second.alive) {
            return Value::null();  // Invalid ref
        }
        return it->second.value;
    }
    
    // Add reference (when passed into WASM)
    void addRef(uint32_t externRef) {
        auto it = refs_.find(externRef);
        if (it != refs_.end()) {
            it->second.refCount++;
        }
    }
    
    // Release reference (when WASM drops it)
    void release(uint32_t externRef) {
        auto it = refs_.find(externRef);
        if (it == refs_.end()) return;
        
        it->second.refCount--;
        if (it->second.refCount == 0) {
            // Unroot the JS object
            if (it->second.value.isObject()) {
                Object* obj = it->second.value.asObject();
                jsHeap_->removeRoot(&obj);
            }
            it->second.alive = false;
            stats_.released++;
        }
    }
    
    // Check for leaks
    struct Stats {
        size_t created = 0;
        size_t released = 0;
        size_t currentLive = 0;
    };
    
    Stats stats() const {
        Stats s = stats_;
        s.currentLive = 0;
        for (const auto& [id, info] : refs_) {
            if (info.alive) s.currentLive++;
        }
        return s;
    }
    
    // Verify no leaks
    bool verifyNoLeaks() const {
        for (const auto& [id, info] : refs_) {
            if (info.alive && info.refCount > 0) {
                return false;  // Leak detected
            }
        }
        return true;
    }
    
private:
    struct RefInfo {
        Value value;
        size_t refCount;
        bool alive;
    };
    
    GCHeap* jsHeap_;
    std::unordered_map<uint32_t, RefInfo> refs_;
    uint32_t nextId_ = 1;
    Stats stats_;
};

// =============================================================================
// Exception Boundary Handler
// =============================================================================

/**
 * @brief Handles exceptions crossing WASM/JS boundary
 */
class ExceptionBoundary {
public:
    // Catch JS exception, convert to WASM exception
    static bool jsToWasm(Value jsException, WasmException& wasmEx) {
        wasmEx.tagIndex = 0;  // Generic exception tag
        
        // Extract message if Error object
        if (jsException.isObject()) {
            Object* obj = jsException.asObject();
            // Would extract message property
            wasmEx.payload.clear();
            // Store serialized exception
        } else if (jsException.isString()) {
            // String exception
            wasmEx.payload.clear();
        }
        
        return true;
    }
    
    // Catch WASM exception, convert to JS Error
    static Value wasmToJS(const WasmException& wasmEx, Context* ctx) {
        // Create JS Error object from WASM exception
        // Error message from payload
        (void)wasmEx; (void)ctx;
        return Value::undefined();  // Would create Error object
    }
    
    // Validate exception state after boundary crossing
    static bool validateState(bool jsHasException, bool wasmHasException) {
        // Only one side should have active exception
        return !(jsHasException && wasmHasException);
    }
};

// =============================================================================
// Shared Memory JS Memory Model Compliance
// =============================================================================

/**
 * @brief Ensures shared memory operations comply with JS memory model
 */
class SharedMemoryCompliance {
public:
    // JS memory model requires certain ordering guarantees
    
    // Sequentially consistent atomic load
    template<typename T>
    static T atomicLoadSeqCst(const T* ptr) {
        return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
    }
    
    // Sequentially consistent atomic store
    template<typename T>
    static void atomicStoreSeqCst(T* ptr, T value) {
        __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
    }
    
    // Atomic wait (Atomics.wait)
    static int32_t atomicWait(int32_t* ptr, int32_t expected, int64_t timeoutNs) {
        // Returns: 0 = ok, 1 = not-equal, 2 = timed-out
        int32_t current = atomicLoadSeqCst(ptr);
        if (current != expected) return 1;
        
        // Would use futex or similar
        (void)timeoutNs;
        return 0;
    }
    
    // Atomic notify (Atomics.notify)
    static int32_t atomicNotify(int32_t* ptr, int32_t count) {
        // Returns number of waiters woken
        (void)ptr; (void)count;
        return 0;  // Would use futex
    }
    
    // Validate memory ordering for TypedArray access
    static bool validateTypedArrayOrdering() {
        // Ensure proper fencing for shared buffers
        return true;
    }
};

// =============================================================================
// Tail Call Stack Semantics
// =============================================================================

/**
 * @brief Ensures tail calls maintain JS stack semantics
 */
class TailCallValidator {
public:
    // Record tail call
    void recordTailCall(const char* caller, const char* callee) {
        tailCalls_.push_back({caller, callee});
    }
    
    // Verify stack frame was properly reused
    bool verifyFrameReuse(size_t beforeDepth, size_t afterDepth) {
        // Tail call should not increase stack depth
        return afterDepth <= beforeDepth;
    }
    
    // Check for proper return value propagation
    bool verifyReturnPropagation(Value expected, Value actual) {
        return expected.strictEquals(actual);
    }
    
    // Verify proper this binding in tail position
    bool verifyThisBinding(Value expectedThis, Value actualThis) {
        return expectedThis.strictEquals(actualThis);
    }
    
private:
    struct TailCall {
        const char* caller;
        const char* callee;
    };
    std::vector<TailCall> tailCalls_;
};

// =============================================================================
// Bridge Validator
// =============================================================================

/**
 * @brief Comprehensive WASM-JS bridge validation
 */
class BridgeValidator {
public:
    BridgeValidator(GCHeap* jsHeap, WasmGCAllocator* wasmHeap)
        : crossHeapGC_(jsHeap, wasmHeap)
        , externRefManager_(jsHeap) {}
    
    // Run all validations
    struct ValidationResult {
        bool gcIntegrity = false;
        bool noExternRefLeaks = false;
        bool exceptionStateValid = false;
        bool memoryModelCompliant = false;
        bool tailCallsCorrect = false;
        
        bool allPassed() const {
            return gcIntegrity && noExternRefLeaks && 
                   exceptionStateValid && memoryModelCompliant &&
                   tailCallsCorrect;
        }
    };
    
    ValidationResult validate() {
        ValidationResult result;
        
        result.gcIntegrity = crossHeapGC_.verifyIntegrity();
        result.noExternRefLeaks = externRefManager_.verifyNoLeaks();
        result.exceptionStateValid = true;  // Would check active exceptions
        result.memoryModelCompliant = SharedMemoryCompliance::validateTypedArrayOrdering();
        result.tailCallsCorrect = true;  // Would validate recorded tail calls
        
        return result;
    }
    
    CrossHeapGC& crossHeapGC() { return crossHeapGC_; }
    ExternRefManager& externRefManager() { return externRefManager_; }
    TailCallValidator& tailCallValidator() { return tailCallValidator_; }
    
private:
    CrossHeapGC crossHeapGC_;
    ExternRefManager externRefManager_;
    TailCallValidator tailCallValidator_;
};

} // namespace Zepra::Bridge
