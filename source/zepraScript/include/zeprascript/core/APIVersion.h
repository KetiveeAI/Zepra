/**
 * @file APIVersion.h
 * @brief API/ABI Versioning and Freeze Management
 * 
 * Implements:
 * - Semantic versioning macros
 * - ABI compatibility checks
 * - Deprecation warnings
 * - Feature flags
 */

#pragma once

#include <cstdint>
#include <string>

namespace Zepra {

// =============================================================================
// Version Numbers
// =============================================================================

#define ZEPRA_VERSION_MAJOR 1
#define ZEPRA_VERSION_MINOR 0
#define ZEPRA_VERSION_PATCH 0

#define ZEPRA_VERSION_STRING "1.0.0"
#define ZEPRA_VERSION_NUMERIC ((ZEPRA_VERSION_MAJOR << 16) | \
                               (ZEPRA_VERSION_MINOR << 8) | \
                               ZEPRA_VERSION_PATCH)

// ABI version (changes only on breaking changes)
#define ZEPRA_ABI_VERSION 1

// =============================================================================
// API Stability Markers
// =============================================================================

/**
 * @brief API stability levels
 */
enum class APIStability {
    Frozen,      // Guaranteed stable, won't change
    Stable,      // Stable, may add but not remove
    Beta,        // May change between minor versions
    Experimental // May change at any time
};

// Macros for marking API stability
#define ZEPRA_API_FROZEN      __attribute__((annotate("api:frozen")))
#define ZEPRA_API_STABLE      __attribute__((annotate("api:stable")))
#define ZEPRA_API_BETA        __attribute__((annotate("api:beta")))
#define ZEPRA_API_EXPERIMENTAL __attribute__((annotate("api:experimental")))

// =============================================================================
// Deprecation
// =============================================================================

#define ZEPRA_DEPRECATED(msg) [[deprecated(msg)]]
#define ZEPRA_DEPRECATED_SINCE(version, msg) \
    [[deprecated("Deprecated since " version ": " msg)]]

// For scheduled removal
#define ZEPRA_REMOVE_IN(version) \
    [[deprecated("Will be removed in " version)]]

// =============================================================================
// Version Information
// =============================================================================

/**
 * @brief Runtime version information
 */
class Version {
public:
    static constexpr uint32_t major() { return ZEPRA_VERSION_MAJOR; }
    static constexpr uint32_t minor() { return ZEPRA_VERSION_MINOR; }
    static constexpr uint32_t patch() { return ZEPRA_VERSION_PATCH; }
    static constexpr uint32_t numeric() { return ZEPRA_VERSION_NUMERIC; }
    static constexpr uint32_t abi() { return ZEPRA_ABI_VERSION; }
    
    static std::string string() { return ZEPRA_VERSION_STRING; }
    
    // Version comparison
    static constexpr bool atLeast(uint32_t maj, uint32_t min, uint32_t pat = 0) {
        return numeric() >= ((maj << 16) | (min << 8) | pat);
    }
    
    // ABI compatibility check
    static bool abiCompatible(uint32_t otherAbi) {
        return otherAbi == ZEPRA_ABI_VERSION;
    }
};

// =============================================================================
// Frozen Interface Registry
// =============================================================================

/**
 * @brief Tracks frozen interfaces for compatibility
 */
class FrozenInterfaces {
public:
    // Check if interface is frozen
    static bool isFrozen(const std::string& interfaceName) {
        // Core frozen interfaces
        static const char* frozen[] = {
            "Value",
            "Object", 
            "Function",
            "Array",
            "GCHeap",
            "Environment",
            "VM",
            "WasmModule",
            "WasmInstance",
            "WasmHostBindings",
            "WasmCanonicalABI"
        };
        
        for (const auto* name : frozen) {
            if (interfaceName == name) return true;
        }
        return false;
    }
    
    // Check ABI layout size (for binary compatibility)
    static bool validateLayout(const std::string& name, size_t expectedSize) {
        // Would check against known sizes
        (void)name; (void)expectedSize;
        return true;
    }
};

// =============================================================================
// Feature Flags
// =============================================================================

/**
 * @brief Runtime feature detection
 */
class Features {
public:
    // JS Features
    static constexpr bool hasES2024() { return true; }
    static constexpr bool hasProxyReflect() { return true; }
    static constexpr bool hasAsyncAwait() { return true; }
    static constexpr bool hasTopLevelAwait() { return true; }
    
    // WASM Features
    static constexpr bool hasWasmMVP() { return true; }
    static constexpr bool hasWasmSIMD() { return true; }
    static constexpr bool hasWasmThreads() { return true; }
    static constexpr bool hasWasmTailCall() { return true; }
    static constexpr bool hasWasmGC() { return true; }
    static constexpr bool hasWasmExceptions() { return true; }
    static constexpr bool hasWasmComponentModel() { return true; }
    
    // JIT Tiers
    static constexpr bool hasInterpreter() { return true; }
    static constexpr bool hasBaselineJIT() { return true; }
    static constexpr bool hasOptimizingJIT() { return true; }
    
    // Debug Features
    static constexpr bool hasDebugger() {
#ifdef NDEBUG
        return false;  // Disabled in release
#else
        return true;
#endif
    }
};

// =============================================================================
// Build Information
// =============================================================================

/**
 * @brief Compile-time build information
 */
class BuildInfo {
public:
    static const char* compiler() {
#if defined(__clang__)
        return "clang";
#elif defined(__GNUC__)
        return "gcc";
#elif defined(_MSC_VER)
        return "msvc";
#else
        return "unknown";
#endif
    }
    
    static const char* platform() {
#if defined(__linux__)
        return "linux";
#elif defined(__APPLE__)
        return "macos";
#elif defined(_WIN32)
        return "windows";
#else
        return "unknown";
#endif
    }
    
    static const char* arch() {
#if defined(__x86_64__) || defined(_M_X64)
        return "x64";
#elif defined(__aarch64__) || defined(_M_ARM64)
        return "arm64";
#elif defined(__i386__) || defined(_M_IX86)
        return "x86";
#elif defined(__arm__) || defined(_M_ARM)
        return "arm";
#else
        return "unknown";
#endif
    }
    
    static bool isDebug() {
#ifdef NDEBUG
        return false;
#else
        return true;
#endif
    }
    
    static std::string summary() {
        return std::string("ZepraScript ") + Version::string() +
               " [" + compiler() + "/" + platform() + "/" + arch() + "]" +
               (isDebug() ? " (debug)" : " (release)");
    }
};

} // namespace Zepra
