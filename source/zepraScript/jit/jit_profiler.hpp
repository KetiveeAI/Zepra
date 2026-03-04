#pragma once

/**
 * @file jit_profiler.hpp
 * @brief JIT profiler for hot function detection
 * 
 * Tracks function call counts to identify compilation candidates.
 * Minimal overhead - just increments counters.
 */

#include "../config.hpp"
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Zepra::JIT {

/**
 * @brief Tier levels for JIT compilation
 */
enum class TierLevel : uint8_t {
    Interpreter = 0,    // Interpreted bytecode
    Baseline = 1,       // Simple JIT (future)
    Optimized = 2       // Fully optimized (future)
};

/**
 * @brief Profile data for a single function
 * 
 * Memory: 16 bytes per function
 */
struct FunctionProfile {
    uint32_t callCount = 0;         // Total calls
    uint32_t loopIterations = 0;    // Back-edge count
    TierLevel currentTier = TierLevel::Interpreter;
    bool markedForCompilation = false;
    
    // Thresholds
    static constexpr uint32_t HOT_THRESHOLD = 100;      // Calls to become "hot"
    static constexpr uint32_t VERY_HOT_THRESHOLD = 1000; // Candidate for optimization
    
    bool isHot() const { return callCount >= HOT_THRESHOLD; }
    bool isVeryHot() const { return callCount >= VERY_HOT_THRESHOLD; }
};

/**
 * @brief JIT profiler for runtime profiling
 * 
 * Lightweight profiling with bounded memory.
 * Maximum 1024 functions tracked (~16KB).
 */
class JITProfiler {
public:
    static constexpr size_t MAX_FUNCTIONS = 1024;
    
    /**
     * @brief Record a function call
     * @param functionId Unique function identifier
     * @return true if function became hot on this call
     */
    bool recordCall(uintptr_t functionId);
    
    /**
     * @brief Record a loop back-edge
     * @param functionId Function containing the loop
     */
    void recordLoopIteration(uintptr_t functionId);
    
    /**
     * @brief Get profile for a function
     * @return nullptr if function not tracked
     */
    FunctionProfile* getProfile(uintptr_t functionId);
    const FunctionProfile* getProfile(uintptr_t functionId) const;
    
    /**
     * @brief Get list of hot functions
     */
    std::vector<uintptr_t> getHotFunctions() const;
    
    /**
     * @brief Memory usage in bytes
     */
    size_t memoryUsage() const { 
        return profiles_.size() * (sizeof(uintptr_t) + sizeof(FunctionProfile)); 
    }
    
    /**
     * @brief Statistics
     */
    size_t trackedFunctions() const { return profiles_.size(); }
    size_t hotFunctionCount() const;
    
    /**
     * @brief Clear all profiling data
     */
    void reset();
    
    /**
     * @brief Enable/disable profiling
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
private:
    std::unordered_map<uintptr_t, FunctionProfile> profiles_;
    bool enabled_ = true;  // Enabled by default
};

/**
 * @brief Global profiler instance (singleton)
 */
JITProfiler& getProfiler();

} // namespace Zepra::JIT
