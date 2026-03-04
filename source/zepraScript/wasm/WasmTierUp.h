/**
 * @file WasmTierUp.h
 * @brief WebAssembly Tier-Up and OSR Implementation
 * 
 * Based on JavaScriptCore's TierUpCount design.
 * Manages execution counters and compilation status for hot code.
 */

#pragma once

#include <cstdint>
#include <atomic>
#include <vector>
#include <mutex>
#include <memory>

namespace Zepra::Wasm {

// =============================================================================
// Tier-Up Constants
// =============================================================================

constexpr int32_t kTierUpLoopIncrement = 100;
constexpr int32_t kTierUpFunctionEntryIncrement = 15;
constexpr int32_t kTierUpThresholdWarmUp = 1000;
constexpr int32_t kTierUpThresholdHot = 10000;
constexpr int32_t kTierUpThresholdSoon = 500;

// =============================================================================
// Tier-Up Enums
// =============================================================================

/**
 * @brief Reason for triggering tier-up at an OSR entry point
 */
enum class TierUpTriggerReason : uint8_t {
    DontTrigger = 0,      // Don't trigger tier-up yet
    CompilationDone = 1,  // Compilation complete, ready to OSR
    StartCompilation = 2  // Should start compilation
};

/**
 * @brief Compilation status for a function
 */
enum class TierUpCompilationStatus : uint8_t {
    NotCompiled = 0,      // Not yet compiled to optimized tier
    StartCompilation = 1, // Compilation has started
    Compiled = 2,         // Successfully compiled
    Failed = 3            // Compilation failed
};

/**
 * @brief Tier of compilation
 */
enum class CompilationTier : uint8_t {
    Baseline = 0,         // Baseline JIT (current)
    Optimized = 1,        // Optimized (future: equivalent to JSC's OMG)
    Native = 2            // Full native (future)
};

// =============================================================================
// OSR Entry Data
// =============================================================================

/**
 * @brief Data needed for On-Stack Replacement at a loop header
 */
struct OSREntryData {
    uint32_t loopIndex;
    uint32_t bytecodeOffset;
    uint32_t stackHeight;
    std::vector<uint8_t> localTypes;  // Types of locals at OSR point
    void* optimizedEntryPoint;        // Entry into optimized code
    
    OSREntryData(uint32_t loop, uint32_t offset, uint32_t stack)
        : loopIndex(loop), bytecodeOffset(offset), stackHeight(stack)
        , optimizedEntryPoint(nullptr) {}
};

// =============================================================================
// Execution Counter
// =============================================================================

/**
 * @brief Base class for execution counting
 */
class ExecutionCounter {
public:
    ExecutionCounter(int32_t threshold = kTierUpThresholdWarmUp)
        : count_(threshold) {}
    
    int32_t count() const { return count_.load(std::memory_order_relaxed); }
    
    void increment(int32_t delta) {
        count_.fetch_sub(delta, std::memory_order_relaxed);
    }
    
    bool checkThresholdReached() {
        return count_.load(std::memory_order_relaxed) <= 0;
    }
    
    void setThreshold(int32_t threshold) {
        count_.store(threshold, std::memory_order_relaxed);
    }
    
    void deferIndefinitely() {
        count_.store(INT32_MAX, std::memory_order_relaxed);
    }
    
protected:
    std::atomic<int32_t> count_;
};

// =============================================================================
// Tier-Up Count
// =============================================================================

/**
 * @brief Manages tier-up execution counts for a WASM function
 * 
 * Based on JSC's TierUpCount. Key features:
 * - Countdown counter (negative = ready to tier up)
 * - Per-loop OSR entry triggers
 * - Compilation status tracking
 */
class TierUpCount : public ExecutionCounter {
public:
    TierUpCount() 
        : ExecutionCounter(kTierUpThresholdWarmUp)
        , compilationStatus_(TierUpCompilationStatus::NotCompiled)
        , osrCompilationStatus_(TierUpCompilationStatus::NotCompiled) {}
    
    ~TierUpCount() = default;
    
    // Non-copyable
    TierUpCount(const TierUpCount&) = delete;
    TierUpCount& operator=(const TierUpCount&) = delete;
    
    // -------------------------------------------------------------------------
    // Increment Functions
    // -------------------------------------------------------------------------
    
    static int32_t loopIncrement() { return kTierUpLoopIncrement; }
    static int32_t functionEntryIncrement() { return kTierUpFunctionEntryIncrement; }
    
    void incrementForLoop() { increment(loopIncrement()); }
    void incrementForEntry() { increment(functionEntryIncrement()); }
    
    // -------------------------------------------------------------------------
    // Optimization Thresholds
    // -------------------------------------------------------------------------
    
    void optimizeAfterWarmUp() {
        setThreshold(kTierUpThresholdWarmUp);
    }
    
    void optimizeNextInvocation() {
        setThreshold(0);
    }
    
    void optimizeSoon() {
        setThreshold(kTierUpThresholdSoon);
    }
    
    void dontOptimizeAnytimeSoon() {
        deferIndefinitely();
    }
    
    // -------------------------------------------------------------------------
    // Compilation Status
    // -------------------------------------------------------------------------
    
    TierUpCompilationStatus compilationStatus() const {
        return compilationStatus_.load(std::memory_order_acquire);
    }
    
    void setCompilationStatus(TierUpCompilationStatus status) {
        compilationStatus_.store(status, std::memory_order_release);
    }
    
    TierUpCompilationStatus osrCompilationStatus() const {
        return osrCompilationStatus_.load(std::memory_order_acquire);
    }
    
    void setOsrCompilationStatus(TierUpCompilationStatus status) {
        osrCompilationStatus_.store(status, std::memory_order_release);
    }
    
    // -------------------------------------------------------------------------
    // OSR Entry Triggers (per-loop)
    // -------------------------------------------------------------------------
    
    std::vector<TierUpTriggerReason>& osrEntryTriggers() { return osrEntryTriggers_; }
    const std::vector<TierUpTriggerReason>& osrEntryTriggers() const { return osrEntryTriggers_; }
    
    void addLoopOSRPoint() {
        osrEntryTriggers_.push_back(TierUpTriggerReason::DontTrigger);
    }
    
    void setLoopTrigger(uint32_t loopIndex, TierUpTriggerReason reason) {
        if (loopIndex < osrEntryTriggers_.size()) {
            osrEntryTriggers_[loopIndex] = reason;
        }
    }
    
    TierUpTriggerReason getLoopTrigger(uint32_t loopIndex) const {
        return loopIndex < osrEntryTriggers_.size() 
            ? osrEntryTriggers_[loopIndex] 
            : TierUpTriggerReason::DontTrigger;
    }
    
    // -------------------------------------------------------------------------
    // OSR Entry Data
    // -------------------------------------------------------------------------
    
    OSREntryData& addOSREntryData(uint32_t loopIndex, uint32_t bytecodeOffset, uint32_t stackHeight) {
        std::lock_guard<std::mutex> lock(lock_);
        osrEntryData_.push_back(std::make_unique<OSREntryData>(loopIndex, bytecodeOffset, stackHeight));
        return *osrEntryData_.back();
    }
    
    OSREntryData* getOSREntryData(uint32_t loopIndex) {
        std::lock_guard<std::mutex> lock(lock_);
        for (auto& data : osrEntryData_) {
            if (data->loopIndex == loopIndex) {
                return data.get();
            }
        }
        return nullptr;
    }
    
    // -------------------------------------------------------------------------
    // Outer Loops (for nested loop optimization)
    // -------------------------------------------------------------------------
    
    std::vector<uint32_t>& outerLoops() { return outerLoops_; }
    
    std::mutex& getLock() { return lock_; }
    
private:
    std::atomic<TierUpCompilationStatus> compilationStatus_;
    std::atomic<TierUpCompilationStatus> osrCompilationStatus_;
    std::vector<TierUpTriggerReason> osrEntryTriggers_;
    std::vector<uint32_t> outerLoops_;
    std::vector<std::unique_ptr<OSREntryData>> osrEntryData_;
    std::mutex lock_;
};

// =============================================================================
// Tier-Up Manager
// =============================================================================

/**
 * @brief Manages tier-up for all functions in a module
 */
class TierUpManager {
public:
    TierUpManager() = default;
    
    TierUpCount& getOrCreateCounter(uint32_t funcIndex) {
        std::lock_guard<std::mutex> lock(lock_);
        if (funcIndex >= counters_.size()) {
            counters_.resize(funcIndex + 1);
        }
        if (!counters_[funcIndex]) {
            counters_[funcIndex] = std::make_unique<TierUpCount>();
        }
        return *counters_[funcIndex];
    }
    
    TierUpCount* getCounter(uint32_t funcIndex) {
        std::lock_guard<std::mutex> lock(lock_);
        return funcIndex < counters_.size() ? counters_[funcIndex].get() : nullptr;
    }
    
    bool shouldTierUp(uint32_t funcIndex) {
        auto* counter = getCounter(funcIndex);
        if (!counter) return false;
        
        if (counter->compilationStatus() != TierUpCompilationStatus::NotCompiled) {
            return false;  // Already compiling or compiled
        }
        
        return counter->checkThresholdReached();
    }
    
    bool startTierUpCompilation(uint32_t funcIndex) {
        auto* counter = getCounter(funcIndex);
        if (!counter) return false;
        
        TierUpCompilationStatus expected = TierUpCompilationStatus::NotCompiled;
        // Atomic compare-exchange to avoid double compilation
        if (counter->compilationStatus() == expected) {
            counter->setCompilationStatus(TierUpCompilationStatus::StartCompilation);
            return true;
        }
        return false;
    }
    
    void markCompilationComplete(uint32_t funcIndex, bool success) {
        auto* counter = getCounter(funcIndex);
        if (!counter) return;
        
        counter->setCompilationStatus(
            success ? TierUpCompilationStatus::Compiled : TierUpCompilationStatus::Failed
        );
    }
    
private:
    std::vector<std::unique_ptr<TierUpCount>> counters_;
    std::mutex lock_;
};

} // namespace Zepra::Wasm
