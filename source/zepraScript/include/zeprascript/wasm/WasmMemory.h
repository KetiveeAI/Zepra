/**
 * @file WasmMemory.h
 * @brief WebAssembly linear memory with guard pages and signal handling
 * 
 * Provides memory management for WASM with:
 * - 64KB page size
 * - Guard pages for bounds checking
 * - Signal handler integration
 * - Shared memory support (threads proposal)
 * - 64-bit memory support (memory64 proposal)
 * 
 * Based on Firefox SpiderMonkey / WebKit JSC
 */

#pragma once

#include "WasmConstants.h"
#include "ZWasmTrap.h"
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <mutex>
#include <memory>

namespace Zepra::Wasm {

// =============================================================================
// Memory Mode
// =============================================================================

enum class MemoryMode : uint8_t {
    BoundsChecking,     // Explicit bounds checks (portable, slower)
    Signaling,          // Guard pages + signals (fast, platform-specific)
};

// =============================================================================
// Memory Addressing
// =============================================================================

enum class MemoryAddressing : uint8_t {
    I32,    // 32-bit indices (4GB max)
    I64     // 64-bit indices (memory64 proposal)
};

// =============================================================================
// Memory Limits
// =============================================================================

struct MemoryLimits {
    uint32_t initial = 0;       // Initial size in pages
    uint32_t maximum = 0;       // Maximum size in pages (0 = unlimited)
    bool hasMaximum = false;
    bool shared = false;
    MemoryAddressing addressing = MemoryAddressing::I32;
};

// =============================================================================
// Constants
// =============================================================================

static constexpr size_t PageSize = 64 * 1024;                    // 64KB
static constexpr size_t MaxPages32 = 65536;                      // 4GB / 64KB
static constexpr size_t GuardSize = 2 * 1024 * 1024;             // 2MB guard region
static constexpr size_t MaxMemory32 = MaxPages32 * PageSize;     // 4GB

// For memory64
static constexpr uint64_t MaxPages64 = 1ULL << 48;               // 16 exabytes / 64KB

// =============================================================================
// Memory Class
// =============================================================================

class WasmMemory {
public:
    WasmMemory(const MemoryLimits& limits, MemoryMode mode = MemoryMode::BoundsChecking);
    ~WasmMemory();
    
    // No copy
    WasmMemory(const WasmMemory&) = delete;
    WasmMemory& operator=(const WasmMemory&) = delete;
    
    // Move
    WasmMemory(WasmMemory&& other) noexcept;
    WasmMemory& operator=(WasmMemory&& other) noexcept;
    
    // ==========================================================================
    // Accessors
    // ==========================================================================
    
    uint8_t* base() const { return base_; }
    size_t byteLength() const { return currentPages_ * PageSize; }
    uint32_t currentPages() const { return currentPages_; }
    uint32_t maximumPages() const { return maximumPages_; }
    
    // For JIT: bounds check limit (excludes guard)
    size_t boundsCheckLimit() const { return boundsCheckLimit_; }
    
    // For shared memory
    bool isShared() const { return shared_; }
    
    // Memory mode
    MemoryMode mode() const { return mode_; }
    
    // ==========================================================================
    // Memory Operations
    // ==========================================================================
    
    // Grow memory by delta pages
    // Returns old size in pages, or -1 on failure
    int32_t grow(uint32_t deltaPages);
    
    // Bounds checking (for BoundsChecking mode)
    bool inBounds(size_t offset, size_t size) const {
        return offset + size <= byteLength() && offset + size >= offset;
    }
    
    // ==========================================================================
    // Memory Access (with bounds checking)
    // ==========================================================================
    
    template<typename T>
    T load(size_t offset) const {
        if (!inBounds(offset, sizeof(T))) {
            throw std::runtime_error(trapMessage(ZTrap::OutOfBoundsMemoryAccess));
        }
        return *reinterpret_cast<const T*>(base_ + offset);
    }
    
    template<typename T>
    void store(size_t offset, T value) {
        if (!inBounds(offset, sizeof(T))) {
            throw std::runtime_error(trapMessage(ZTrap::OutOfBoundsMemoryAccess));
        }
        *reinterpret_cast<T*>(base_ + offset) = value;
    }
    
    // Unaligned access (some platforms need this)
    template<typename T>
    T loadUnaligned(size_t offset) const {
        if (!inBounds(offset, sizeof(T))) {
            throw std::runtime_error(trapMessage(ZTrap::OutOfBoundsMemoryAccess));
        }
        T value;
        std::memcpy(&value, base_ + offset, sizeof(T));
        return value;
    }
    
    template<typename T>
    void storeUnaligned(size_t offset, T value) {
        if (!inBounds(offset, sizeof(T))) {
            throw std::runtime_error(trapMessage(ZTrap::OutOfBoundsMemoryAccess));
        }
        std::memcpy(base_ + offset, &value, sizeof(T));
    }
    
    // ==========================================================================
    // Bulk Operations
    // ==========================================================================
    
    void fill(size_t offset, uint8_t value, size_t size);
    void copy(size_t destOffset, size_t srcOffset, size_t size);
    void init(size_t destOffset, const uint8_t* src, size_t srcOffset, size_t size);
    
    // ==========================================================================
    // Atomic Operations (threads proposal)
    // ==========================================================================
    
    template<typename T>
    T atomicLoad(size_t offset) const {
        if (!inBounds(offset, sizeof(T))) {
            throw std::runtime_error(trapMessage(ZTrap::OutOfBoundsMemoryAccess));
        }
        if (offset % sizeof(T) != 0) {
            throw std::runtime_error(trapMessage(ZTrap::UnalignedMemoryAccess));
        }
        return reinterpret_cast<const std::atomic<T>*>(base_ + offset)->load(std::memory_order_seq_cst);
    }
    
    template<typename T>
    void atomicStore(size_t offset, T value) {
        if (!inBounds(offset, sizeof(T))) {
            throw std::runtime_error(trapMessage(ZTrap::OutOfBoundsMemoryAccess));
        }
        if (offset % sizeof(T) != 0) {
            throw std::runtime_error(trapMessage(ZTrap::UnalignedMemoryAccess));
        }
        reinterpret_cast<std::atomic<T>*>(base_ + offset)->store(value, std::memory_order_seq_cst);
    }
    
    template<typename T>
    T atomicAdd(size_t offset, T value) {
        if (!inBounds(offset, sizeof(T))) {
            throw std::runtime_error(trapMessage(ZTrap::OutOfBoundsMemoryAccess));
        }
        if (offset % sizeof(T) != 0) {
            throw std::runtime_error(trapMessage(ZTrap::UnalignedMemoryAccess));
        }
        return reinterpret_cast<std::atomic<T>*>(base_ + offset)->fetch_add(value, std::memory_order_seq_cst);
    }
    
    template<typename T>
    T atomicSub(size_t offset, T value) {
        if (!inBounds(offset, sizeof(T))) {
            throw std::runtime_error(trapMessage(ZTrap::OutOfBoundsMemoryAccess));
        }
        if (offset % sizeof(T) != 0) {
            throw std::runtime_error(trapMessage(ZTrap::UnalignedMemoryAccess));
        }
        return reinterpret_cast<std::atomic<T>*>(base_ + offset)->fetch_sub(value, std::memory_order_seq_cst);
    }
    
    template<typename T>
    T atomicAnd(size_t offset, T value) {
        if (!inBounds(offset, sizeof(T))) {
            throw std::runtime_error(trapMessage(ZTrap::OutOfBoundsMemoryAccess));
        }
        if (offset % sizeof(T) != 0) {
            throw std::runtime_error(trapMessage(ZTrap::UnalignedMemoryAccess));
        }
        return reinterpret_cast<std::atomic<T>*>(base_ + offset)->fetch_and(value, std::memory_order_seq_cst);
    }
    
    template<typename T>
    T atomicOr(size_t offset, T value) {
        if (!inBounds(offset, sizeof(T))) {
            throw std::runtime_error(trapMessage(ZTrap::OutOfBoundsMemoryAccess));
        }
        if (offset % sizeof(T) != 0) {
            throw std::runtime_error(trapMessage(ZTrap::UnalignedMemoryAccess));
        }
        return reinterpret_cast<std::atomic<T>*>(base_ + offset)->fetch_or(value, std::memory_order_seq_cst);
    }
    
    template<typename T>
    T atomicXor(size_t offset, T value) {
        if (!inBounds(offset, sizeof(T))) {
            throw std::runtime_error(trapMessage(ZTrap::OutOfBoundsMemoryAccess));
        }
        if (offset % sizeof(T) != 0) {
            throw std::runtime_error(trapMessage(ZTrap::UnalignedMemoryAccess));
        }
        return reinterpret_cast<std::atomic<T>*>(base_ + offset)->fetch_xor(value, std::memory_order_seq_cst);
    }
    
    template<typename T>
    T atomicExchange(size_t offset, T value) {
        if (!inBounds(offset, sizeof(T))) {
            throw std::runtime_error(trapMessage(ZTrap::OutOfBoundsMemoryAccess));
        }
        if (offset % sizeof(T) != 0) {
            throw std::runtime_error(trapMessage(ZTrap::UnalignedMemoryAccess));
        }
        return reinterpret_cast<std::atomic<T>*>(base_ + offset)->exchange(value, std::memory_order_seq_cst);
    }
    
    template<typename T>
    T atomicCompareExchange(size_t offset, T expected, T desired) {
        if (!inBounds(offset, sizeof(T))) {
            throw std::runtime_error(trapMessage(ZTrap::OutOfBoundsMemoryAccess));
        }
        if (offset % sizeof(T) != 0) {
            throw std::runtime_error(trapMessage(ZTrap::UnalignedMemoryAccess));
        }
        auto* atomic = reinterpret_cast<std::atomic<T>*>(base_ + offset);
        atomic->compare_exchange_strong(expected, desired, std::memory_order_seq_cst);
        return expected;
    }
    
    // ==========================================================================
    // Wait/Notify (threads proposal)
    // ==========================================================================
    
    // Returns: 0 = "ok" (woken), 1 = "not-equal", 2 = "timed-out"
    int32_t atomicWait32(size_t offset, int32_t expected, int64_t timeoutNs);
    int32_t atomicWait64(size_t offset, int64_t expected, int64_t timeoutNs);
    
    // Returns number of waiters woken
    uint32_t atomicNotify(size_t offset, uint32_t count);
    
    // Fence
    void atomicFence() {
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }
    
    // ==========================================================================
    // Guard Page Support
    // ==========================================================================
    
    // Check if an address is in the guard region
    bool isInGuardRegion(void* addr) const;
    
    // Get guard region bounds
    void* guardStart() const;
    void* guardEnd() const;
    
private:
    void allocate();
    void deallocate();
    void setupGuardPages();
    void updateBoundsCheckLimit();
    
    uint8_t* base_ = nullptr;           // Start of usable memory
    uint8_t* mapped_ = nullptr;         // Start of mapped region (includes guard)
    size_t mappedSize_ = 0;             // Total mapped size
    uint32_t currentPages_ = 0;
    uint32_t maximumPages_ = 0;
    size_t boundsCheckLimit_ = 0;
    bool shared_ = false;
    MemoryMode mode_ = MemoryMode::BoundsChecking;
    MemoryAddressing addressing_ = MemoryAddressing::I32;
    
    std::mutex growMutex_;
};

// =============================================================================
// Fault Signal Handler
// =============================================================================

class FaultSignalHandler {
public:
    // Install signal handlers (call once at startup)
    static void install();
    
    // Uninstall signal handlers
    static void uninstall();
    
    // Register a memory instance for fault handling
    static void registerMemory(WasmMemory* memory);
    static void unregisterMemory(WasmMemory* memory);
    
    // Handle a fault (called from signal handler)
    // Returns true if the fault was handled (was a WASM memory access)
    static bool handleFault(void* faultAddr, void* context);
    
private:
    static bool isInstalled_;
};

// =============================================================================
// Memory Pool (for efficient allocation)
// =============================================================================

class MemoryPool {
public:
    static MemoryPool& instance();
    
    // Allocate memory region with optional guard pages
    void* allocate(size_t size, size_t guardSize = 0);
    void deallocate(void* ptr, size_t size);
    
    // Statistics
    size_t totalAllocated() const { return totalAllocated_; }
    size_t totalMapped() const { return totalMapped_; }
    
private:
    MemoryPool() = default;
    
    std::atomic<size_t> totalAllocated_{0};
    std::atomic<size_t> totalMapped_{0};
    std::mutex poolMutex_;
};

} // namespace Zepra::Wasm
