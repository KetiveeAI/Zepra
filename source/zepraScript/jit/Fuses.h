/**
 * @file Fuses.h
 * @brief Assumption-Based Optimization System
 * 
 * SpiderMonkey 2024 "Fuses" pattern:
 * - Track VM state assumptions
 * - Invalidate JIT code when assumptions break
 * - Enable aggressive speculation without guards
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <bitset>

namespace Zepra::JIT {

// =============================================================================
// Fuse IDs (VM State Assumptions)
// =============================================================================

enum class FuseId : uint32_t {
    // Array prototype assumptions
    ArrayPrototypeIntact,           // Array.prototype not modified
    ArrayPrototypeNoIndexedAccessors, // No indexed getters/setters
    ArrayIteratorPrototypeIntact,   // Array iterator not modified
    
    // Object prototype assumptions  
    ObjectPrototypeIntact,          // Object.prototype not modified
    ObjectPrototypeNoEnumerables,   // No added enumerable properties
    
    // Map/Set assumptions
    MapPrototypeIntact,
    SetPrototypeIntact,
    
    // Function assumptions
    FunctionPrototypeIntact,
    FunctionBindNotModified,
    
    // String assumptions
    StringPrototypeIntact,
    
    // Number assumptions
    NumberPrototypeIntact,
    
    // Promise assumptions
    PromisePrototypeIntact,
    PromiseThenNotModified,
    
    // Symbol assumptions
    SymbolPrototypeIntact,
    SymbolIteratorIntact,
    
    // Reflect/Proxy assumptions
    ReflectIntact,
    ProxyConstructorIntact,
    
    // Global assumptions
    GlobalObjectIntact,             // No shadowing of builtins
    StrictModeDefault,              // Default is strict mode
    
    // Count
    NUM_FUSES
};

constexpr size_t NUM_FUSES = static_cast<size_t>(FuseId::NUM_FUSES);

// =============================================================================
// Fuse State
// =============================================================================

/**
 * @brief State of a single fuse
 */
class Fuse {
public:
    explicit Fuse(FuseId id) : id_(id) {}
    
    FuseId id() const { return id_; }
    
    // Check if fuse is intact (assumption still valid)
    bool isIntact() const { return intact_.load(std::memory_order_acquire); }
    
    // Blow the fuse (assumption violated)
    void blow() {
        bool expected = true;
        if (intact_.compare_exchange_strong(expected, false)) {
            blowCount_++;
            // Would trigger invalidation
        }
    }
    
    // Reset fuse (for testing)
    void reset() { intact_.store(true); }
    
    uint32_t blowCount() const { return blowCount_; }
    
private:
    FuseId id_;
    std::atomic<bool> intact_{true};
    uint32_t blowCount_ = 0;
};

// =============================================================================
// Fuse Box (All Fuses)
// =============================================================================

/**
 * @brief Container for all fuses
 */
class FuseBox {
public:
    static FuseBox& instance() {
        static FuseBox box;
        return box;
    }
    
    // Check single fuse
    bool isIntact(FuseId id) const {
        return intactBits_.test(static_cast<size_t>(id));
    }
    
    // Check multiple fuses at once (fast path)
    bool allIntact(std::initializer_list<FuseId> ids) const {
        for (FuseId id : ids) {
            if (!isIntact(id)) return false;
        }
        return true;
    }
    
    // Blow a fuse
    void blow(FuseId id) {
        size_t idx = static_cast<size_t>(id);
        if (intactBits_.test(idx)) {
            intactBits_.reset(idx);
            notifyInvalidation(id);
        }
    }
    
    // Register invalidation callback
    using InvalidationCallback = std::function<void(FuseId)>;
    void onInvalidation(InvalidationCallback cb) {
        callbacks_.push_back(std::move(cb));
    }
    
    // Get fuse name (for debugging)
    static const char* fuseName(FuseId id) {
        switch (id) {
            case FuseId::ArrayPrototypeIntact: return "ArrayPrototypeIntact";
            case FuseId::ArrayPrototypeNoIndexedAccessors: return "ArrayPrototypeNoIndexedAccessors";
            case FuseId::ObjectPrototypeIntact: return "ObjectPrototypeIntact";
            case FuseId::MapPrototypeIntact: return "MapPrototypeIntact";
            case FuseId::SetPrototypeIntact: return "SetPrototypeIntact";
            case FuseId::PromiseThenNotModified: return "PromiseThenNotModified";
            case FuseId::GlobalObjectIntact: return "GlobalObjectIntact";
            default: return "Unknown";
        }
    }
    
    // Statistics
    struct Stats {
        size_t intact = 0;
        size_t blown = 0;
    };
    
    Stats stats() const {
        Stats s;
        s.intact = intactBits_.count();
        s.blown = NUM_FUSES - s.intact;
        return s;
    }
    
    // Reset all fuses (for testing)
    void resetAll() {
        intactBits_.set();
    }
    
private:
    FuseBox() {
        intactBits_.set();  // All fuses intact initially
    }
    
    void notifyInvalidation(FuseId id) {
        for (auto& cb : callbacks_) {
            cb(id);
        }
    }
    
    std::bitset<NUM_FUSES> intactBits_;
    std::vector<InvalidationCallback> callbacks_;
};

// =============================================================================
// Fuse Guard (RAII for assumptions)
// =============================================================================

/**
 * @brief RAII guard for checking assumption before operation
 */
class FuseGuard {
public:
    explicit FuseGuard(FuseId id) : id_(id) {
        valid_ = FuseBox::instance().isIntact(id);
    }
    
    FuseGuard(std::initializer_list<FuseId> ids) : id_(FuseId::NUM_FUSES) {
        valid_ = FuseBox::instance().allIntact(ids);
    }
    
    bool isValid() const { return valid_; }
    operator bool() const { return valid_; }
    
private:
    FuseId id_;
    bool valid_;
};

// =============================================================================
// Invalidation Handler (JIT Code Discard)
// =============================================================================

/**
 * @brief Handles JIT code invalidation when fuses blow
 */
class InvalidationHandler {
public:
    struct JITCodeEntry {
        void* codeStart;
        size_t codeSize;
        std::bitset<NUM_FUSES> dependentFuses;
    };
    
    // Register JIT code with its fuse dependencies
    void registerCode(void* code, size_t size, std::initializer_list<FuseId> fuses) {
        JITCodeEntry entry;
        entry.codeStart = code;
        entry.codeSize = size;
        for (FuseId id : fuses) {
            entry.dependentFuses.set(static_cast<size_t>(id));
        }
        entries_.push_back(entry);
    }
    
    // Called when fuse blows - invalidate dependent code
    void onFuseBlown(FuseId id) {
        size_t idx = static_cast<size_t>(id);
        
        for (auto it = entries_.begin(); it != entries_.end(); ) {
            if (it->dependentFuses.test(idx)) {
                invalidateCode(*it);
                it = entries_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    size_t activeCodeCount() const { return entries_.size(); }
    
private:
    void invalidateCode(const JITCodeEntry& entry) {
        // Would:
        // 1. Patch entry points to bailout trampolines
        // 2. Mark code for GC collection
        // 3. Deoptimize on-stack frames
        (void)entry;
    }
    
    std::vector<JITCodeEntry> entries_;
};

// =============================================================================
// Example Usage in Optimized Code
// =============================================================================

/*
// FastPath for Array.prototype.push (depends on fuse)
void optimizedArrayPush(Array* arr, Value value) {
    FuseGuard guard(FuseId::ArrayPrototypeNoIndexedAccessors);
    if (!guard) {
        // Fuse blown - use slow path
        return slowArrayPush(arr, value);
    }
    
    // Fast path - no indexed accessors to worry about
    arr->elements[arr->length++] = value;
}
*/

} // namespace Zepra::JIT
