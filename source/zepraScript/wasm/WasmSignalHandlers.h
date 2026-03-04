/**
 * @file WasmSignalHandlers.h
 * @brief WebAssembly Signal-Based Trap Handling
 * 
 * Implements crash recovery for WASM:
 * - SIGSEGV/SIGBUS handling
 * - Fault address inspection
 * - Trap generation from hardware faults
 */

#pragma once

#include <cstdint>
#include <functional>

namespace Zepra::Wasm {

/**
 * @brief Manages POSIX signal handlers for WASM
 */
class SignalHandlers {
public:
    // Initialize signal handlers (call once at startup)
    static void init();
    
    // Thread-local state for signal handling
    static void enterJITCode();
    static void exitJITCode();
    
    // Check if PC is in JIT code
    static bool isJITAddress(uintptr_t pc);
    
    // Register JIT code region
    static void registerJITRegion(uintptr_t start, size_t size);
    static void unregisterJITRegion(uintptr_t start, size_t size);

private:
    static void installHandlers();
};

/**
 * @brief Exception thrown when a hardware fault is converted to a trap
 */
class WasmTrapException : public std::exception {
public:
    enum class TrapReason {
        OutOfBounds,
        DivisionByZero,
        StackOverflow,
        Unreachable,
        IndirectCallMismatch,
        NullDereference
    };
    
    explicit WasmTrapException(TrapReason reason, uintptr_t faultAddr = 0) 
        : reason_(reason), faultAddr_(faultAddr) {}
    
    TrapReason reason() const { return reason_; }
    uintptr_t faultAddress() const { return faultAddr_; }
    
    const char* what() const noexcept override {
        switch (reason_) {
            case TrapReason::OutOfBounds: return "wasm trap: out of bounds memory access";
            case TrapReason::DivisionByZero: return "wasm trap: integer divide by zero";
            case TrapReason::StackOverflow: return "wasm trap: call stack exhausted";
            case TrapReason::Unreachable: return "wasm trap: unreachable executed";
            case TrapReason::IndirectCallMismatch: return "wasm trap: indirect call type mismatch";
            case TrapReason::NullDereference: return "wasm trap: null pointer dereference";
            default: return "wasm trap: unknown";
        }
    }
    
private:
    TrapReason reason_;
    uintptr_t faultAddr_;
};

} // namespace Zepra::Wasm
