/**
 * @file EngineIntrospection.h
 * @brief Engine internals inspection for debugging and testing
 * 
 * Implements:
 * - Internal engine state inspection
 * - GC statistics
 * - JIT compilation info
 * - Performance counters
 */

#pragma once

#include "runtime/objects/value.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace Zepra::Spec {

// =============================================================================
// GC Statistics
// =============================================================================

struct GCStats {
    size_t heapSize = 0;
    size_t heapUsed = 0;
    size_t nurserySize = 0;
    size_t nurseryUsed = 0;
    size_t oldGenSize = 0;
    size_t oldGenUsed = 0;
    
    size_t minorGCCount = 0;
    size_t majorGCCount = 0;
    double minorGCTime = 0;
    double majorGCTime = 0;
    
    size_t objectsAllocated = 0;
    size_t objectsCollected = 0;
};

// =============================================================================
// JIT Statistics
// =============================================================================

struct JITStats {
    size_t functionsCompiled = 0;
    size_t baselineFunctions = 0;
    size_t optimizedFunctions = 0;
    size_t deoptimizations = 0;
    
    size_t codeSize = 0;
    size_t inlineCacheCount = 0;
    size_t inlineCacheHits = 0;
    size_t inlineCacheMisses = 0;
    
    double compilationTime = 0;
    double optimizationTime = 0;
};

// =============================================================================
// Runtime Statistics
// =============================================================================

struct RuntimeStats {
    size_t executedBytecodes = 0;
    size_t functionCalls = 0;
    size_t propertyAccesses = 0;
    size_t arrayAccesses = 0;
    
    size_t exceptionsThrown = 0;
    size_t exceptionsHandled = 0;
    
    double totalExecutionTime = 0;
};

// =============================================================================
// Engine Introspection
// =============================================================================

class EngineIntrospection {
public:
    EngineIntrospection() = default;
    
    // =========================================================================
    // Statistics Access
    // =========================================================================
    
    GCStats getGCStats() const { return gcStats_; }
    JITStats getJITStats() const { return jitStats_; }
    RuntimeStats getRuntimeStats() const { return runtimeStats_; }
    
    // =========================================================================
    // Value Inspection
    // =========================================================================
    
    /**
     * @brief Get internal representation of value
     */
    static std::string inspectValue(Runtime::Value value);
    
    /**
     * @brief Get object hidden class info
     */
    static std::string inspectHiddenClass(Runtime::Value object);
    
    /**
     * @brief Get function bytecode disassembly
     */
    static std::string disassembleFunction(Runtime::Value function);
    
    /**
     * @brief Get compiled code disassembly
     */
    static std::string disassembleNativeCode(Runtime::Value function);
    
    // =========================================================================
    // Heap Inspection
    // =========================================================================
    
    /**
     * @brief Get heap snapshot
     */
    struct HeapObject {
        uintptr_t address;
        std::string type;
        size_t size;
        std::vector<uintptr_t> references;
    };
    
    std::vector<HeapObject> getHeapSnapshot() const;
    
    /**
     * @brief Find retaining path to object
     */
    std::vector<HeapObject> getRetainingPath(uintptr_t address) const;
    
    // =========================================================================
    // Counters
    // =========================================================================
    
    void incrementBytecodeCount() { runtimeStats_.executedBytecodes++; }
    void incrementFunctionCalls() { runtimeStats_.functionCalls++; }
    void incrementPropertyAccess() { runtimeStats_.propertyAccesses++; }
    void incrementArrayAccess() { runtimeStats_.arrayAccesses++; }
    
    void recordMinorGC(double time) {
        gcStats_.minorGCCount++;
        gcStats_.minorGCTime += time;
    }
    
    void recordMajorGC(double time) {
        gcStats_.majorGCCount++;
        gcStats_.majorGCTime += time;
    }
    
    void recordCompilation(double time) {
        jitStats_.functionsCompiled++;
        jitStats_.compilationTime += time;
    }
    
    void recordDeopt() {
        jitStats_.deoptimizations++;
    }
    
    // =========================================================================
    // Reset
    // =========================================================================
    
    void reset() {
        gcStats_ = {};
        jitStats_ = {};
        runtimeStats_ = {};
    }
    
private:
    GCStats gcStats_;
    JITStats jitStats_;
    RuntimeStats runtimeStats_;
};

// =============================================================================
// Debug Flags
// =============================================================================

struct DebugFlags {
    bool traceExecution = false;
    bool traceBytecode = false;
    bool traceGC = false;
    bool traceJIT = false;
    bool traceIC = false;
    bool traceDeopt = false;
    
    bool verifyHeap = false;
    bool verifyGC = false;
    
    bool dumpBytecode = false;
    bool dumpAST = false;
    bool dumpIR = false;
    bool dumpMachineCode = false;
    
    static DebugFlags& instance() {
        static DebugFlags flags;
        return flags;
    }
};

} // namespace Zepra::Spec
