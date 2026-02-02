/**
 * @file WasmThreads.h
 * @brief WebAssembly Threads and Atomics Support
 * 
 * Implements the WebAssembly Threads proposal:
 * - Atomic memory operations
 * - Wait/notify primitives
 * - Shared memory
 */

#pragma once

#include "wasm.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <unordered_map>

namespace Zepra::Wasm {

// =============================================================================
// Atomic Operation Types
// =============================================================================

/**
 * @brief Atomic operation opcodes (prefixed with 0xFE)
 */
namespace AtomicOp {
    // Memory operations
    constexpr uint8_t NOTIFY = 0x00;
    constexpr uint8_t WAIT32 = 0x01;
    constexpr uint8_t WAIT64 = 0x02;
    constexpr uint8_t FENCE = 0x03;
    
    // Atomic loads
    constexpr uint8_t I32_LOAD = 0x10;
    constexpr uint8_t I64_LOAD = 0x11;
    constexpr uint8_t I32_LOAD8_U = 0x12;
    constexpr uint8_t I32_LOAD16_U = 0x13;
    constexpr uint8_t I64_LOAD8_U = 0x14;
    constexpr uint8_t I64_LOAD16_U = 0x15;
    constexpr uint8_t I64_LOAD32_U = 0x16;
    
    // Atomic stores
    constexpr uint8_t I32_STORE = 0x17;
    constexpr uint8_t I64_STORE = 0x18;
    constexpr uint8_t I32_STORE8 = 0x19;
    constexpr uint8_t I32_STORE16 = 0x1A;
    constexpr uint8_t I64_STORE8 = 0x1B;
    constexpr uint8_t I64_STORE16 = 0x1C;
    constexpr uint8_t I64_STORE32 = 0x1D;
    
    // Atomic RMW - Add
    constexpr uint8_t I32_RMW_ADD = 0x1E;
    constexpr uint8_t I64_RMW_ADD = 0x1F;
    constexpr uint8_t I32_RMW8_ADD_U = 0x20;
    constexpr uint8_t I32_RMW16_ADD_U = 0x21;
    constexpr uint8_t I64_RMW8_ADD_U = 0x22;
    constexpr uint8_t I64_RMW16_ADD_U = 0x23;
    constexpr uint8_t I64_RMW32_ADD_U = 0x24;
    
    // Atomic RMW - Sub
    constexpr uint8_t I32_RMW_SUB = 0x25;
    constexpr uint8_t I64_RMW_SUB = 0x26;
    constexpr uint8_t I32_RMW8_SUB_U = 0x27;
    constexpr uint8_t I32_RMW16_SUB_U = 0x28;
    constexpr uint8_t I64_RMW8_SUB_U = 0x29;
    constexpr uint8_t I64_RMW16_SUB_U = 0x2A;
    constexpr uint8_t I64_RMW32_SUB_U = 0x2B;
    
    // Atomic RMW - And
    constexpr uint8_t I32_RMW_AND = 0x2C;
    constexpr uint8_t I64_RMW_AND = 0x2D;
    constexpr uint8_t I32_RMW8_AND_U = 0x2E;
    constexpr uint8_t I32_RMW16_AND_U = 0x2F;
    constexpr uint8_t I64_RMW8_AND_U = 0x30;
    constexpr uint8_t I64_RMW16_AND_U = 0x31;
    constexpr uint8_t I64_RMW32_AND_U = 0x32;
    
    // Atomic RMW - Or
    constexpr uint8_t I32_RMW_OR = 0x33;
    constexpr uint8_t I64_RMW_OR = 0x34;
    constexpr uint8_t I32_RMW8_OR_U = 0x35;
    constexpr uint8_t I32_RMW16_OR_U = 0x36;
    constexpr uint8_t I64_RMW8_OR_U = 0x37;
    constexpr uint8_t I64_RMW16_OR_U = 0x38;
    constexpr uint8_t I64_RMW32_OR_U = 0x39;
    
    // Atomic RMW - Xor
    constexpr uint8_t I32_RMW_XOR = 0x3A;
    constexpr uint8_t I64_RMW_XOR = 0x3B;
    constexpr uint8_t I32_RMW8_XOR_U = 0x3C;
    constexpr uint8_t I32_RMW16_XOR_U = 0x3D;
    constexpr uint8_t I64_RMW8_XOR_U = 0x3E;
    constexpr uint8_t I64_RMW16_XOR_U = 0x3F;
    constexpr uint8_t I64_RMW32_XOR_U = 0x40;
    
    // Atomic RMW - Xchg
    constexpr uint8_t I32_RMW_XCHG = 0x41;
    constexpr uint8_t I64_RMW_XCHG = 0x42;
    constexpr uint8_t I32_RMW8_XCHG_U = 0x43;
    constexpr uint8_t I32_RMW16_XCHG_U = 0x44;
    constexpr uint8_t I64_RMW8_XCHG_U = 0x45;
    constexpr uint8_t I64_RMW16_XCHG_U = 0x46;
    constexpr uint8_t I64_RMW32_XCHG_U = 0x47;
    
    // Atomic RMW - Cmpxchg
    constexpr uint8_t I32_RMW_CMPXCHG = 0x48;
    constexpr uint8_t I64_RMW_CMPXCHG = 0x49;
    constexpr uint8_t I32_RMW8_CMPXCHG_U = 0x4A;
    constexpr uint8_t I32_RMW16_CMPXCHG_U = 0x4B;
    constexpr uint8_t I64_RMW8_CMPXCHG_U = 0x4C;
    constexpr uint8_t I64_RMW16_CMPXCHG_U = 0x4D;
    constexpr uint8_t I64_RMW32_CMPXCHG_U = 0x4E;
}

// =============================================================================
// Wait/Notify Infrastructure
// =============================================================================

/**
 * @brief Wait result codes
 */
enum class WaitResult : int32_t {
    OK = 0,        // Woken by notify
    NotEqual = 1,  // Value didn't match
    TimedOut = 2   // Timeout expired
};

/**
 * @brief Manages wait queues for atomic.wait/notify
 */
class WaiterList {
public:
    struct Waiter {
        std::condition_variable cv;
        bool notified = false;
    };
    
    // Wait on memory address
    WaitResult wait32(void* addr, int32_t expected, int64_t timeoutNs);
    WaitResult wait64(void* addr, int64_t expected, int64_t timeoutNs);
    
    // Notify waiters at address
    uint32_t notify(void* addr, uint32_t count);
    
private:
    std::mutex mutex_;
    std::unordered_map<void*, std::vector<Waiter*>> waiters_;
};

// =============================================================================
// Shared Memory Support
// =============================================================================

/**
 * @brief Shared WebAssembly.Memory
 * 
 * Thread-safe memory that can be shared between agents/workers
 */
class SharedWasmMemory : public WasmMemory {
public:
    explicit SharedWasmMemory(const MemoryType& type);
    
    // Thread-safe grow
    size_t grow(size_t deltaPages);
    
    // Atomic operations on memory
    template<typename T>
    T atomicLoad(uint32_t offset) const;
    
    template<typename T>
    void atomicStore(uint32_t offset, T value);
    
    template<typename T>
    T atomicRMW_Add(uint32_t offset, T value);
    
    template<typename T>
    T atomicRMW_Sub(uint32_t offset, T value);
    
    template<typename T>
    T atomicRMW_And(uint32_t offset, T value);
    
    template<typename T>
    T atomicRMW_Or(uint32_t offset, T value);
    
    template<typename T>
    T atomicRMW_Xor(uint32_t offset, T value);
    
    template<typename T>
    T atomicRMW_Xchg(uint32_t offset, T value);
    
    template<typename T>
    T atomicRMW_Cmpxchg(uint32_t offset, T expected, T replacement);
    
    // Wait/notify
    WaitResult wait32(uint32_t offset, int32_t expected, int64_t timeoutNs);
    WaitResult wait64(uint32_t offset, int64_t expected, int64_t timeoutNs);
    uint32_t notify(uint32_t offset, uint32_t count);
    
private:
    mutable std::mutex growMutex_;
    WaiterList waiterList_;
};

// =============================================================================
// Template Implementations
// =============================================================================

template<typename T>
T SharedWasmMemory::atomicLoad(uint32_t offset) const {
    static_assert(sizeof(T) <= 8, "Atomic load only for 1/2/4/8 byte types");
    auto* ptr = reinterpret_cast<std::atomic<T>*>(buffer() + offset);
    return ptr->load(std::memory_order_seq_cst);
}

template<typename T>
void SharedWasmMemory::atomicStore(uint32_t offset, T value) {
    static_assert(sizeof(T) <= 8, "Atomic store only for 1/2/4/8 byte types");
    auto* ptr = reinterpret_cast<std::atomic<T>*>(buffer() + offset);
    ptr->store(value, std::memory_order_seq_cst);
}

template<typename T>
T SharedWasmMemory::atomicRMW_Add(uint32_t offset, T value) {
    auto* ptr = reinterpret_cast<std::atomic<T>*>(buffer() + offset);
    return ptr->fetch_add(value, std::memory_order_seq_cst);
}

template<typename T>
T SharedWasmMemory::atomicRMW_Sub(uint32_t offset, T value) {
    auto* ptr = reinterpret_cast<std::atomic<T>*>(buffer() + offset);
    return ptr->fetch_sub(value, std::memory_order_seq_cst);
}

template<typename T>
T SharedWasmMemory::atomicRMW_And(uint32_t offset, T value) {
    auto* ptr = reinterpret_cast<std::atomic<T>*>(buffer() + offset);
    return ptr->fetch_and(value, std::memory_order_seq_cst);
}

template<typename T>
T SharedWasmMemory::atomicRMW_Or(uint32_t offset, T value) {
    auto* ptr = reinterpret_cast<std::atomic<T>*>(buffer() + offset);
    return ptr->fetch_or(value, std::memory_order_seq_cst);
}

template<typename T>
T SharedWasmMemory::atomicRMW_Xor(uint32_t offset, T value) {
    auto* ptr = reinterpret_cast<std::atomic<T>*>(buffer() + offset);
    return ptr->fetch_xor(value, std::memory_order_seq_cst);
}

template<typename T>
T SharedWasmMemory::atomicRMW_Xchg(uint32_t offset, T value) {
    auto* ptr = reinterpret_cast<std::atomic<T>*>(buffer() + offset);
    return ptr->exchange(value, std::memory_order_seq_cst);
}

template<typename T>
T SharedWasmMemory::atomicRMW_Cmpxchg(uint32_t offset, T expected, T replacement) {
    auto* ptr = reinterpret_cast<std::atomic<T>*>(buffer() + offset);
    ptr->compare_exchange_strong(expected, replacement, std::memory_order_seq_cst);
    return expected;
}

} // namespace Zepra::Wasm
