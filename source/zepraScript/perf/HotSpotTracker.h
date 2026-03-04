/**
 * @file HotSpotTracker.h
 * @brief Track and identify performance hot spots
 * 
 * Implements:
 * - Function execution time tracking
 * - Line-level timing
 * - Call graph profiling
 * - JIT tier suggestions
 * 
 * Based on profiler hot spot detection algorithms
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <atomic>

namespace Zepra::Perf {

// =============================================================================
// Hot Spot Types
// =============================================================================

enum class HotSpotType : uint8_t {
    Function,       // Hot function
    Loop,           // Hot loop
    Line,           // Hot line
    Allocation      // Allocation hot spot
};

// =============================================================================
// Timing Entry
// =============================================================================

struct TimingEntry {
    std::chrono::nanoseconds selfTime{0};
    std::chrono::nanoseconds totalTime{0};
    uint64_t hitCount = 0;
    
    void addSample(std::chrono::nanoseconds self, std::chrono::nanoseconds total) {
        selfTime += self;
        totalTime += total;
        hitCount++;
    }
    
    double selfTimeMs() const { 
        return selfTime.count() / 1e6; 
    }
    
    double totalTimeMs() const { 
        return totalTime.count() / 1e6; 
    }
    
    double avgSelfTimeUs() const {
        return hitCount > 0 ? (selfTime.count() / 1e3) / hitCount : 0;
    }
};

// =============================================================================
// Function Profile
// =============================================================================

struct FunctionProfile {
    std::string functionName;
    std::string scriptUrl;
    uint32_t lineNumber = 0;
    uint32_t columnNumber = 0;
    
    TimingEntry timing;
    
    // Bytecode size for optimization decisions
    size_t bytecodeSize = 0;
    
    // Execution tier
    enum class Tier { Interpreter, Baseline, Optimized } tier = Tier::Interpreter;
    
    // Optimization state
    bool isOptimized = false;
    bool wasDeoptimized = false;
    uint32_t deoptCount = 0;
    
    // Inline cache performance
    uint32_t icHits = 0;
    uint32_t icMisses = 0;
    double icHitRate() const {
        uint32_t total = icHits + icMisses;
        return total > 0 ? static_cast<double>(icHits) / total : 0;
    }
};

// =============================================================================
// Loop Profile
// =============================================================================

struct LoopProfile {
    std::string functionName;
    uint32_t bytecodeOffset = 0;
    uint32_t lineNumber = 0;
    
    TimingEntry timing;
    
    // Loop analysis
    uint64_t iterationCount = 0;
    double avgIterationsPerCall = 0;
    
    // OSR candidate score
    double osrScore() const {
        return static_cast<double>(timing.hitCount) * avgIterationsPerCall;
    }
};

// =============================================================================
// Hot Spot
// =============================================================================

struct HotSpot {
    HotSpotType type;
    std::string identifier;     // Function name or location
    std::string scriptUrl;
    uint32_t lineNumber;
    
    double selfTimePercent;
    double totalTimePercent;
    uint64_t hitCount;
    
    // Optimization recommendation
    std::string recommendation;
};

// =============================================================================
// Call Graph Node
// =============================================================================

struct CallGraphNode {
    std::string functionName;
    TimingEntry timing;
    
    // Edges: callee name -> timing
    std::unordered_map<std::string, TimingEntry> callees;
    
    // Parent tracking
    std::vector<std::string> callers;
};

// =============================================================================
// Hot Spot Tracker
// =============================================================================

class HotSpotTracker {
public:
    HotSpotTracker() = default;
    
    // =========================================================================
    // Control
    // =========================================================================
    
    void start();
    void stop();
    bool isTracking() const { return tracking_.load(); }
    void reset();
    
    // =========================================================================
    // Function Tracking
    // =========================================================================
    
    /**
     * @brief Enter function (start timing)
     */
    void enterFunction(const std::string& functionName,
                       const std::string& scriptUrl,
                       uint32_t line);
    
    /**
     * @brief Exit function (stop timing)
     */
    void exitFunction();
    
    /**
     * @brief Record function profile data
     */
    void recordFunctionProfile(const FunctionProfile& profile);
    
    // =========================================================================
    // Loop Tracking
    // =========================================================================
    
    void enterLoop(const std::string& functionName, uint32_t bytecodeOffset, uint32_t line);
    void exitLoop();
    void recordLoopIteration();
    
    // =========================================================================
    // IC Tracking
    // =========================================================================
    
    void recordICHit(const std::string& functionName);
    void recordICMiss(const std::string& functionName);
    
    // =========================================================================
    // Analysis
    // =========================================================================
    
    /**
     * @brief Get top N hot spots by self time
     */
    std::vector<HotSpot> getHotSpots(size_t limit = 10) const;
    
    /**
     * @brief Get functions ordered by total time
     */
    std::vector<FunctionProfile> getFunctionsByTotalTime(size_t limit = 50) const;
    
    /**
     * @brief Get hot loops (OSR candidates)
     */
    std::vector<LoopProfile> getHotLoops(size_t limit = 20) const;
    
    /**
     * @brief Get call graph
     */
    std::unordered_map<std::string, CallGraphNode> getCallGraph() const;
    
    /**
     * @brief Get optimization recommendations
     */
    struct OptimizationRecommendation {
        std::string functionName;
        std::string reason;
        enum class Action { 
            None, 
            TierUp,         // Compile to higher tier
            Inline,         // Inline into callers
            OSR,            // On-stack replacement
            Deoptimize      // Too unstable for optimization
        } action;
        double priority;    // 0-1, higher = more urgent
    };
    
    std::vector<OptimizationRecommendation> getRecommendations() const;
    
    // =========================================================================
    // Statistics
    // =========================================================================
    
    struct Stats {
        std::chrono::nanoseconds totalTrackedTime{0};
        size_t functionsTracked = 0;
        size_t loopsTracked = 0;
        size_t samplesCollected = 0;
    };
    
    Stats getStats() const;
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    void setTierUpThreshold(uint64_t hitCount) { tierUpThreshold_ = hitCount; }
    void setInlineThreshold(size_t bytecodes) { inlineThreshold_ = bytecodes; }
    
private:
    std::atomic<bool> tracking_{false};
    mutable std::mutex mutex_;
    
    // Profiles
    std::unordered_map<std::string, FunctionProfile> functions_;
    std::unordered_map<uint64_t, LoopProfile> loops_;  // key: func_hash + offset
    
    // Call stack for timing
    struct CallEntry {
        std::string functionName;
        std::chrono::steady_clock::time_point enterTime;
        std::chrono::nanoseconds childTime{0};
    };
    std::vector<CallEntry> callStack_;
    
    // Loop stack
    struct LoopEntry {
        uint64_t key;
        std::chrono::steady_clock::time_point enterTime;
        uint64_t iterations = 0;
    };
    std::vector<LoopEntry> loopStack_;
    
    // Call graph
    std::unordered_map<std::string, CallGraphNode> callGraph_;
    
    // Thresholds
    uint64_t tierUpThreshold_ = 1000;
    size_t inlineThreshold_ = 100;
    
    Stats stats_;
    
    uint64_t makeLoopKey(const std::string& func, uint32_t offset) const;
    void updateCallGraph(const std::string& caller, const std::string& callee, 
                         std::chrono::nanoseconds time);
};

} // namespace Zepra::Perf
