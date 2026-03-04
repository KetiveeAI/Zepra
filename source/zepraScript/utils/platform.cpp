/**
 * @file platform.cpp
 * @brief Platform detection, high-resolution timer, and thread utilities
 */

#include "config.hpp"
#include <chrono>
#include <thread>
#include <string>
#include <cstdint>

#if ZEPRA_PLATFORM_POSIX
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#elif ZEPRA_PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace Zepra::Utils {

// =============================================================================
// High-Resolution Timer (backs performance.now())
// =============================================================================

static auto g_startTime = std::chrono::steady_clock::now();

double monotonicTimeMs() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - g_startTime);
    return static_cast<double>(elapsed.count()) / 1000.0;
}

uint64_t monotonicTimeNs() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - g_startTime);
    return static_cast<uint64_t>(elapsed.count());
}

void resetTimerOrigin() {
    g_startTime = std::chrono::steady_clock::now();
}

// =============================================================================
// Platform Info
// =============================================================================

const char* platformName() {
#if ZEPRA_PLATFORM_LINUX
    return "linux";
#elif ZEPRA_PLATFORM_MACOS
    return "darwin";
#elif ZEPRA_PLATFORM_WINDOWS
    return "win32";
#elif ZEPRA_PLATFORM_FREEBSD
    return "freebsd";
#else
    return "unknown";
#endif
}

const char* archName() {
#if defined(ZEPRA_ARCH_X64)
    return "x64";
#elif defined(ZEPRA_ARCH_X86)
    return "ia32";
#elif defined(ZEPRA_ARCH_ARM64)
    return "arm64";
#elif defined(ZEPRA_ARCH_ARM)
    return "arm";
#else
    return "unknown";
#endif
}

// =============================================================================
// Page Size
// =============================================================================

size_t pageSize() {
#if ZEPRA_PLATFORM_POSIX
    static size_t cached = static_cast<size_t>(sysconf(_SC_PAGESIZE));
    return cached;
#elif ZEPRA_PLATFORM_WINDOWS
    static size_t cached = []() {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        return static_cast<size_t>(si.dwPageSize);
    }();
    return cached;
#else
    return 4096;
#endif
}

// =============================================================================
// Hardware Concurrency
// =============================================================================

unsigned int hardwareConcurrency() {
    unsigned int n = std::thread::hardware_concurrency();
    return n > 0 ? n : 1;
}

// =============================================================================
// Stack Guard (estimate remaining stack space)
// =============================================================================

bool hasStackSpace(size_t needed) {
    volatile char marker;
    // Rough heuristic: compare address against a known base
    // This is platform-specific and intentionally conservative
    (void)marker;
    (void)needed;
    // Full implementation requires platform-specific stack limit queries
    // For now, always return true — the VM's call depth limit is the real guard
    return true;
}

} // namespace Zepra::Utils
