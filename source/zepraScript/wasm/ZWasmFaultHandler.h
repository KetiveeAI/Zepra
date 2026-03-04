/**
 * @file ZWasmFaultHandler.h
 * @brief ZepraScript signal-based memory bounds checking
 * 
 * Uses SIGSEGV/SIGBUS signals with guard pages for fast
 * out-of-bounds detection. Part of ZepraScript's independent WASM runtime.
 */

#pragma once

#include "ZWasmTrap.h"
#include <cstdint>
#include <cstddef>

namespace Zepra::Wasm {

// =============================================================================
// Guard Page Configuration
// =============================================================================

struct GuardPageConfig {
    static constexpr size_t GuardSize = 2 * 1024 * 1024;  // 2MB guard region
    static constexpr size_t RedZoneSize = 32 * 1024;       // 32KB red zone
    
    // Check if address is in guard region
    static bool isInGuardRegion(void* addr, void* memBase, size_t memSize);
};

// =============================================================================
// Memory Region Tracking
// =============================================================================

struct WasmMemoryRegion {
    void* base;
    size_t size;
    size_t mappedSize;  // Including guard pages
    uint32_t instanceId;
    
    bool containsFaultAddress(void* addr) const;
};

// =============================================================================
// Signal Handler Registration
// =============================================================================

class ZFaultHandler {
public:
    // Install signal handlers (SIGSEGV, SIGBUS)
    static bool install();
    
    // Uninstall signal handlers
    static void uninstall();
    
    // Check if handlers are installed
    static bool isInstalled();
    
    // Register a WASM memory region for fault detection
    static void registerMemoryRegion(const WasmMemoryRegion& region);
    
    // Unregister a memory region
    static void unregisterMemoryRegion(void* base);
    
    // Handle a fault - returns true if it was a WASM OOB access
    static bool handleFault(void* faultAddr, void* context);
    
    // Get the exception info for the last fault
    static ZTrapInfo getLastFaultInfo();

    // Signal handler internals (used by OS signal callbacks in .cpp)
    static void signalHandler(int sig, void* info, void* context);
    static WasmMemoryRegion* findRegion(void* addr);
    static thread_local ZTrapInfo lastFault_;
    static thread_local bool inFaultHandler_;
};

// =============================================================================
// RAII Guard for Signal Handler Installation
// =============================================================================

class SignalHandlerScope {
public:
    SignalHandlerScope() {
        installed_ = ZFaultHandler::install();
    }
    
    ~SignalHandlerScope() {
        if (installed_) {
            ZFaultHandler::uninstall();
        }
    }
    
    bool valid() const { return installed_; }
    
private:
    bool installed_;
};

// =============================================================================
// Platform-Specific Exception Recovery
// =============================================================================

#if defined(__linux__) || defined(__APPLE__)

// POSIX signal-based recovery
struct PlatformExceptionContext {
    void* pc;           // Program counter at fault
    void* sp;           // Stack pointer
    void* bp;           // Base pointer
    void* faultAddr;    // Address that caused fault
    
    // Extract from signal context
    static PlatformExceptionContext from(void* ucontext);
    
    // Modify context to jump to trap handler
    void setPC(void* newPC);
};

#elif defined(_WIN32)

// Windows SEH-based recovery
struct PlatformExceptionContext {
    void* pc;
    void* sp;
    void* bp;
    void* faultAddr;
    
    static PlatformExceptionContext from(void* exceptionInfo);
    void setPC(void* newPC);
};

#endif

// =============================================================================
// Trap Landing Pad
// =============================================================================

// Assembly routine that:
// 1. Saves registers
// 2. Calls C++ trap handler
// 3. Never returns (throws to JS)
extern "C" void wasmTrapLandingPad() __attribute__((noreturn));

// C++ trap handler called from landing pad
extern "C" void handleZWasmTrap(ZTrap type, uint32_t funcIdx, uint32_t offset);

} // namespace Zepra::Wasm
