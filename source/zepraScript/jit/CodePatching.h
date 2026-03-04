/**
 * @file CodePatching.h
 * @brief Runtime code patching for inline caches and deoptimization
 * 
 * Implements:
 * - Jump patching for IC updates
 * - Call site patching
 * - NOP sleds for patchable regions
 * - Thread-safe patching
 * 
 * Based on V8/JSC IC patching mechanisms
 */

#pragma once

#include "MacroAssembler.h"
#include <atomic>
#include <mutex>

namespace Zepra::JIT {

// =============================================================================
// Patch Point Types
// =============================================================================

enum class PatchType : uint8_t {
    Jump,           // Conditional or unconditional jump
    Call,           // Direct call
    Immediate,      // Embedded immediate value
    Pointer,        // Embedded pointer
    Monomorphic,    // Monomorphic IC site
    Polymorphic,    // Polymorphic IC stub pointer
    Megamorphic     // Megamorphic state
};

/**
 * @brief Represents a patchable location in generated code
 */
struct PatchSite {
    size_t offset;          // Offset in code buffer
    PatchType type;
    uint8_t size;           // Size of patchable region in bytes
    
    // For IC sites
    uint32_t cacheKey = 0;  // Shape/structure ID
    void* stub = nullptr;   // Current stub pointer
};

// =============================================================================
// Code Patcher
// =============================================================================

/**
 * @brief Thread-safe code patcher
 * 
 * Uses atomic operations and memory barriers for safe patching
 * on running code (where architecture allows).
 */
class CodePatcher {
public:
    CodePatcher() = default;
    
    /**
     * @brief Patch a jump target
     * @param code Base of code buffer
     * @param site Patch site descriptor
     * @param newTarget New jump target address
     */
    static void patchJump(void* code, const PatchSite& site, void* newTarget);
    
    /**
     * @brief Patch a call target
     */
    static void patchCall(void* code, const PatchSite& site, void* newTarget);
    
    /**
     * @brief Patch an immediate value
     */
    static void patchImmediate32(void* code, const PatchSite& site, int32_t value);
    static void patchImmediate64(void* code, const PatchSite& site, int64_t value);
    
    /**
     * @brief Patch a pointer value
     */
    static void patchPointer(void* code, const PatchSite& site, void* ptr);
    
    /**
     * @brief Replace instruction with NOP sled
     */
    static void nopOut(void* code, size_t offset, size_t length);
    
    /**
     * @brief Flush instruction cache after patching
     */
    static void flushICache(void* start, size_t length);
    
private:
    // Platform-specific atomic write
    static void atomicWrite32(void* addr, uint32_t value);
    static void atomicWrite64(void* addr, uint64_t value);
};

// =============================================================================
// Inline Cache State Machine
// =============================================================================

enum class ICState : uint8_t {
    Uninitialized,  // Never executed
    Monomorphic,    // Single type seen
    Polymorphic,    // 2-4 types seen
    Megamorphic     // Too many types, use generic
};

/**
 * @brief Inline cache entry
 */
struct ICEntry {
    std::atomic<ICState> state{ICState::Uninitialized};
    std::atomic<void*> cachedStub{nullptr};
    std::atomic<uint32_t> shapeId{0};
    
    // Polymorphic IC can have multiple entries
    static constexpr size_t MAX_POLYMORPHIC = 4;
    struct PolyEntry {
        uint32_t shapeId;
        void* stub;
    };
    PolyEntry polyEntries[MAX_POLYMORPHIC];
    uint8_t polyCount = 0;
};

/**
 * @brief IC update manager
 */
class ICManager {
public:
    /**
     * @brief Update IC after miss
     * @param entry IC entry to update
     * @param shapeId New shape/structure ID
     * @param stub New optimized stub
     * @return New state
     */
    static ICState update(ICEntry& entry, uint32_t shapeId, void* stub);
    
    /**
     * @brief Transition IC to megamorphic
     */
    static void goMegamorphic(ICEntry& entry, void* genericStub);
    
    /**
     * @brief Reset IC to uninitialized
     */
    static void reset(ICEntry& entry);
};

// =============================================================================
// Self-Modifying Code Safety
// =============================================================================

/**
 * @brief Scope guard for safe code modification
 * 
 * Ensures proper memory barriers and I-cache flushes.
 */
class CodeModificationScope {
public:
    explicit CodeModificationScope(void* start, size_t size)
        : start_(start), size_(size) {
        // Ensure all readers complete before modification
        std::atomic_thread_fence(std::memory_order_acquire);
    }
    
    ~CodeModificationScope() {
        // Flush I-cache and ensure visibility
        CodePatcher::flushICache(start_, size_);
        std::atomic_thread_fence(std::memory_order_release);
    }
    
private:
    void* start_;
    size_t size_;
};

// =============================================================================
// Patchable Jump
// =============================================================================

/**
 * @brief A jump instruction that can be safely patched
 */
class PatchableJump {
public:
    PatchableJump() = default;
    
    void emit(MacroAssembler& masm, Label& target) {
        offset_ = masm.currentOffset();
        masm.jmp(target);
        size_ = masm.currentOffset() - offset_;
    }
    
    void emitConditional(MacroAssembler& masm, Condition cond, Label& target) {
        offset_ = masm.currentOffset();
        masm.jcc(cond, target);
        size_ = masm.currentOffset() - offset_;
    }
    
    void patch(void* code, void* newTarget) {
        PatchSite site{offset_, PatchType::Jump, static_cast<uint8_t>(size_)};
        CodePatcher::patchJump(code, site, newTarget);
    }
    
    size_t offset() const { return offset_; }
    
private:
    size_t offset_ = 0;
    size_t size_ = 0;
};

/**
 * @brief A call instruction that can be safely patched
 */
class PatchableCall {
public:
    PatchableCall() = default;
    
    void emit(MacroAssembler& masm, Label& target) {
        offset_ = masm.currentOffset();
        masm.call(target);
        size_ = masm.currentOffset() - offset_;
    }
    
    void patch(void* code, void* newTarget) {
        PatchSite site{offset_, PatchType::Call, static_cast<uint8_t>(size_)};
        CodePatcher::patchCall(code, site, newTarget);
    }
    
    size_t offset() const { return offset_; }
    
private:
    size_t offset_ = 0;
    size_t size_ = 0;
};

} // namespace Zepra::JIT
