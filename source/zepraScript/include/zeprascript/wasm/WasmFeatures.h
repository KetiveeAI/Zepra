/**
 * @file WasmFeatures.h
 * @brief WebAssembly Feature Flags and Runtime Detection
 * 
 * Per-proposal feature flags for compile-time and runtime feature gating.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Zepra::Wasm {

// =============================================================================
// Feature Flags
// =============================================================================

/**
 * @brief Feature flags for each WASM proposal
 */
struct FeatureFlags {
    // MVP Features (always enabled)
    bool mvp : 1 = true;
    
    // Phase-1 Proposals (Standardized)
    bool mutableGlobals : 1 = true;
    bool signExtension : 1 = true;
    bool nonTrappingConversions : 1 = true;
    bool multiValue : 1 = true;
    bool referenceTypes : 1 = true;
    bool bulkMemory : 1 = true;
    bool simd : 1 = true;
    
    // Phase-2 Proposals (Recently Standardized)
    bool tailCall : 1 = true;
    bool extendedConst : 1 = true;
    bool relaxedSimd : 1 = false;  // Not yet enabled by default
    
    // Phase-3 Proposals (In Progress)
    bool exceptionHandling : 1 = true;
    bool threads : 1 = true;
    bool memory64 : 1 = false;
    bool gc : 1 = true;
    bool functionReferences : 1 = true;
    bool typedFunctionReferences : 1 = true;
    
    // Phase-4 Proposals (Experimental)
    bool componentModel : 1 = false;
    bool esm : 1 = false;  // ES Module integration
    bool stringRef : 1 = false;
    bool jsPromiseIntegration : 1 = false;
    
    // Multi-Memory Extensions
    bool multiMemory : 1 = true;
    
    // Debug/Development Features
    bool debugNames : 1 = true;
    bool sourceMapping : 1 = true;
    
    // Default constructor with common defaults
    FeatureFlags() = default;
    
    // All features enabled (for testing)
    static FeatureFlags all() {
        FeatureFlags f;
        f.relaxedSimd = true;
        f.memory64 = true;
        f.componentModel = true;
        f.esm = true;
        f.stringRef = true;
        f.jsPromiseIntegration = true;
        return f;
    }
    
    // Minimal features (MVP only)
    static FeatureFlags minimal() {
        FeatureFlags f;
        f.mutableGlobals = false;
        f.signExtension = false;
        f.nonTrappingConversions = false;
        f.multiValue = false;
        f.referenceTypes = false;
        f.bulkMemory = false;
        f.simd = false;
        f.tailCall = false;
        f.exceptionHandling = false;
        f.threads = false;
        f.gc = false;
        f.functionReferences = false;
        f.typedFunctionReferences = false;
        f.multiMemory = false;
        f.debugNames = false;
        f.sourceMapping = false;
        return f;
    }
};

/**
 * @brief Feature metadata for documentation and diagnostics
 */
struct FeatureInfo {
    const char* name;
    const char* proposal;
    const char* description;
    bool enabled;
};

/**
 * @brief Feature detection and compatibility checking
 */
class FeatureDetector {
public:
    explicit FeatureDetector(const FeatureFlags& flags = FeatureFlags())
        : flags_(flags) {}
    
    const FeatureFlags& flags() const { return flags_; }
    FeatureFlags& flags() { return flags_; }
    
    // Check if a specific feature is enabled
    bool isEnabled(const std::string& feature) const {
        if (feature == "mvp") return flags_.mvp;
        if (feature == "mutable-globals") return flags_.mutableGlobals;
        if (feature == "sign-extension") return flags_.signExtension;
        if (feature == "nontrapping-fptoint") return flags_.nonTrappingConversions;
        if (feature == "multi-value") return flags_.multiValue;
        if (feature == "reference-types") return flags_.referenceTypes;
        if (feature == "bulk-memory") return flags_.bulkMemory;
        if (feature == "simd") return flags_.simd;
        if (feature == "tail-call") return flags_.tailCall;
        if (feature == "relaxed-simd") return flags_.relaxedSimd;
        if (feature == "exception-handling") return flags_.exceptionHandling;
        if (feature == "threads") return flags_.threads;
        if (feature == "memory64") return flags_.memory64;
        if (feature == "gc") return flags_.gc;
        if (feature == "typed-function-references") return flags_.typedFunctionReferences;
        if (feature == "component-model") return flags_.componentModel;
        if (feature == "multi-memory") return flags_.multiMemory;
        return false;
    }
    
    // Get list of enabled features
    std::vector<FeatureInfo> enabledFeatures() const {
        std::vector<FeatureInfo> result;
        if (flags_.mvp) result.push_back({"mvp", "MVP", "Core WebAssembly", true});
        if (flags_.simd) result.push_back({"simd", "SIMD", "128-bit packed SIMD", true});
        if (flags_.threads) result.push_back({"threads", "Threads", "Shared memory and atomics", true});
        if (flags_.exceptionHandling) result.push_back({"exception-handling", "Exception Handling", "Try/catch/throw", true});
        if (flags_.gc) result.push_back({"gc", "GC", "Garbage collection", true});
        if (flags_.tailCall) result.push_back({"tail-call", "Tail Call", "Proper tail calls", true});
        if (flags_.multiMemory) result.push_back({"multi-memory", "Multi-Memory", "Multiple memories", true});
        if (flags_.bulkMemory) result.push_back({"bulk-memory", "Bulk Memory", "Bulk memory operations", true});
        if (flags_.referenceTypes) result.push_back({"reference-types", "Reference Types", "externref/funcref", true});
        return result;
    }
    
    // Check browser compatibility
    struct BrowserSupport {
        bool chrome;
        bool firefox;
        bool safari;
        bool edge;
    };
    
    static BrowserSupport checkBrowserSupport(const std::string& feature) {
        // Compatibility matrix based on current browser support
        if (feature == "simd") return {true, true, true, true};
        if (feature == "threads") return {true, true, true, true};
        if (feature == "exception-handling") return {true, true, true, true};
        if (feature == "tail-call") return {true, true, true, true};
        if (feature == "gc") return {true, true, false, true};  // Safari partial
        if (feature == "relaxed-simd") return {true, true, false, true};
        if (feature == "multi-memory") return {true, true, false, true};
        return {false, false, false, false};
    }
    
private:
    FeatureFlags flags_;
};

// =============================================================================
// Compile-Time Feature Gating Macros
// =============================================================================

#define WASM_FEATURE_SIMD 1
#define WASM_FEATURE_THREADS 1
#define WASM_FEATURE_EXCEPTION_HANDLING 1
#define WASM_FEATURE_GC 1
#define WASM_FEATURE_TAIL_CALL 1
#define WASM_FEATURE_MULTI_MEMORY 1
#define WASM_FEATURE_BULK_MEMORY 1
#define WASM_FEATURE_REFERENCE_TYPES 1

// Experimental features (disabled by default)
#define WASM_FEATURE_RELAXED_SIMD 0
#define WASM_FEATURE_MEMORY64 0
#define WASM_FEATURE_COMPONENT_MODEL 0

// Feature check macros
#if WASM_FEATURE_SIMD
#define IF_SIMD(code) code
#else
#define IF_SIMD(code)
#endif

#if WASM_FEATURE_THREADS
#define IF_THREADS(code) code
#else
#define IF_THREADS(code)
#endif

#if WASM_FEATURE_GC
#define IF_GC(code) code
#else
#define IF_GC(code)
#endif

} // namespace Zepra::Wasm
