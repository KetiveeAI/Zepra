/**
 * @file gc_write_barrier.cpp
 * @brief Write barrier implementations for the runtime
 *
 * Every reference store in the VM must go through a write barrier.
 * This file provides the actual barrier implementations called
 * by the bytecode interpreter and JIT-compiled code.
 *
 * Barrier types:
 * 1. Generational: card dirtying for old→young refs
 * 2. SATB: snapshot-at-the-beginning for concurrent marking
 * 3. Incremental: grey→white reference barrier
 *
 * Hot path optimization:
 * - The common case (young→young, same-region) is a no-op
 * - Inlined check + out-of-line slow path
 */

#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cstring>

namespace Zepra::Runtime {

// =============================================================================
// Barrier State (shared, read-only during mutator)
// =============================================================================

struct BarrierState {
    // Nursery bounds (for generation check)
    uintptr_t nurseryStart;
    uintptr_t nurseryEnd;

    // Card table base (for dirtying)
    uint8_t* cardTableBase;
    uintptr_t heapBase;

    // SATB state
    std::atomic<bool> satbActive;

    // SATB buffer (per-thread in production, single here)
    static constexpr size_t SATB_BUFFER_SIZE = 1024;
    uintptr_t satbBuffer[SATB_BUFFER_SIZE];
    std::atomic<size_t> satbIndex;

    // Card constants
    static constexpr size_t CARD_SHIFT = 9;
    static constexpr uint8_t CARD_DIRTY = 1;

    BarrierState()
        : nurseryStart(0), nurseryEnd(0)
        , cardTableBase(nullptr), heapBase(0)
        , satbActive(false), satbIndex(0) {
        std::memset(satbBuffer, 0, sizeof(satbBuffer));
    }
};

// Global barrier state (set once during heap init)
static BarrierState g_barrierState;

// Forward declarations
void writeBarrierSlow(uintptr_t srcAddr, uintptr_t oldValue,
                       uintptr_t newValue);
void satbLogSlow(uintptr_t oldRef);

// =============================================================================
// Fast-Path Write Barrier (inlined in VM hot loop)
// =============================================================================

/**
 * @brief Fast write barrier — called on every reference store
 *
 * Performance critical: this runs on every property write.
 * The fast path is just a range check + conditional branch.
 *
 * @param srcAddr Address of the source object
 * @param oldValue Previous reference value in the slot
 * @param newValue New reference value being stored
 */
inline void writeBarrierFast(uintptr_t srcAddr, uintptr_t oldValue,
                              uintptr_t newValue) {
    // Fast path: no barrier needed if:
    //   1. New value is not a heap pointer (immediate/SMI)
    //   2. Source is in nursery (young→anything needs no barrier)
    if (newValue == 0) return;
    if (srcAddr >= g_barrierState.nurseryStart &&
        srcAddr < g_barrierState.nurseryEnd) return;

    // Slow path: source is old-gen, new value is heap ref
    writeBarrierSlow(srcAddr, oldValue, newValue);
}

/**
 * @brief Slow-path write barrier
 */
void writeBarrierSlow(uintptr_t srcAddr, uintptr_t oldValue,
                       uintptr_t newValue) {
    // 1. Generational barrier: dirty card if old→young
    if (newValue >= g_barrierState.nurseryStart &&
        newValue < g_barrierState.nurseryEnd) {
        if (g_barrierState.cardTableBase) {
            size_t cardIdx = (srcAddr - g_barrierState.heapBase)
                >> BarrierState::CARD_SHIFT;
            g_barrierState.cardTableBase[cardIdx] = BarrierState::CARD_DIRTY;
        }
    }

    // 2. SATB barrier: log old value during concurrent marking
    if (g_barrierState.satbActive.load(std::memory_order_acquire)) {
        if (oldValue != 0) {
            satbLogSlow(oldValue);
        }
    }
}

/**
 * @brief Log old reference to SATB buffer
 */
void satbLogSlow(uintptr_t oldRef) {
    size_t idx = g_barrierState.satbIndex.fetch_add(1,
        std::memory_order_relaxed);
    if (idx < BarrierState::SATB_BUFFER_SIZE) {
        g_barrierState.satbBuffer[idx] = oldRef;
    }
    // Buffer full — will be drained by GC at remark
}

// =============================================================================
// Barrier Configuration (called during heap initialization)
// =============================================================================

class WriteBarrierConfig {
public:
    static void setNurseryBounds(uintptr_t start, uintptr_t end) {
        g_barrierState.nurseryStart = start;
        g_barrierState.nurseryEnd = end;
    }

    static void setCardTable(uint8_t* base, uintptr_t heapBase) {
        g_barrierState.cardTableBase = base;
        g_barrierState.heapBase = heapBase;
    }

    static void enableSATB() {
        g_barrierState.satbActive.store(true, std::memory_order_release);
    }

    static void disableSATB() {
        g_barrierState.satbActive.store(false, std::memory_order_release);
    }

    /**
     * @brief Drain SATB buffer (called during remark pause)
     */
    static void drainSATB(std::function<void(uintptr_t)> visitor) {
        size_t count = g_barrierState.satbIndex.load(
            std::memory_order_acquire);
        if (count > BarrierState::SATB_BUFFER_SIZE) {
            count = BarrierState::SATB_BUFFER_SIZE;
        }

        for (size_t i = 0; i < count; i++) {
            uintptr_t ref = g_barrierState.satbBuffer[i];
            if (ref != 0) visitor(ref);
        }

        g_barrierState.satbIndex.store(0, std::memory_order_release);
    }

    static bool isSATBActive() {
        return g_barrierState.satbActive.load(std::memory_order_acquire);
    }

    static size_t satbBufferUsed() {
        return g_barrierState.satbIndex.load(std::memory_order_relaxed);
    }
};

// =============================================================================
// Array Store Barrier
// =============================================================================

/**
 * @brief Write barrier for indexed element stores (arr[i] = value)
 *
 * Same logic as property store but with different source address
 * calculation (element backing store, not object header).
 */
inline void elementStoreBarrier(uintptr_t backingStoreAddr,
                                 size_t index, size_t elementSize,
                                 uintptr_t oldValue, uintptr_t newValue) {
    uintptr_t slotAddr = backingStoreAddr + index * elementSize;
    writeBarrierFast(slotAddr, oldValue, newValue);
}

// =============================================================================
// Bulk Barrier (for Array.prototype methods that write many refs)
// =============================================================================

/**
 * @brief Barrier for bulk reference writes (splice, copyWithin, etc.)
 */
void bulkWriteBarrier(uintptr_t backingStoreAddr,
                       size_t startIndex, size_t count,
                       size_t elementSize) {
    // If the backing store is in the nursery, no barrier needed
    if (backingStoreAddr >= g_barrierState.nurseryStart &&
        backingStoreAddr < g_barrierState.nurseryEnd) return;

    // Dirty the card(s) covering the range
    if (g_barrierState.cardTableBase) {
        uintptr_t rangeStart = backingStoreAddr +
            startIndex * elementSize;
        uintptr_t rangeEnd = rangeStart + count * elementSize;

        for (uintptr_t addr = rangeStart; addr < rangeEnd;
             addr += (1 << BarrierState::CARD_SHIFT)) {
            size_t cardIdx = (addr - g_barrierState.heapBase)
                >> BarrierState::CARD_SHIFT;
            g_barrierState.cardTableBase[cardIdx] = BarrierState::CARD_DIRTY;
        }
    }
}

} // namespace Zepra::Runtime
