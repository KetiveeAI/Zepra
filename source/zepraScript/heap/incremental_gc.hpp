#pragma once

/**
 * @file incremental_gc.hpp
 * @brief Incremental garbage collection for reduced pause times
 */

#include "../config.hpp"
#include "heap.hpp"
#include <atomic>
#include <chrono>

namespace Zepra::GC {

/**
 * @brief GC phase for incremental collection
 */
enum class GCPhase {
    Idle,
    Marking,
    Sweeping,
    Complete
};

/**
 * @brief Incremental GC configuration
 */
struct IncrementalGCConfig {
    size_t maxIncrementMs = 2;      // Max time per increment
    size_t markBatchSize = 100;      // Objects to mark per batch
    size_t sweepBatchSize = 50;      // Objects to sweep per batch
    bool enabled = true;
};

/**
 * @brief Incremental garbage collector
 * 
 * Spreads GC work across multiple small increments to avoid
 * long pause times during execution.
 */
class IncrementalGC {
public:
    explicit IncrementalGC(Heap* heap);
    
    /**
     * @brief Start incremental collection
     */
    void startCollection();
    
    /**
     * @brief Perform one increment of GC work
     * @return true if collection is complete
     */
    bool step();
    
    /**
     * @brief Check if collection is in progress
     */
    bool isCollecting() const { return phase_ != GCPhase::Idle; }
    
    /**
     * @brief Get current phase
     */
    GCPhase phase() const { return phase_; }
    
    /**
     * @brief Get total pause time
     */
    double totalPauseTimeMs() const { return totalPauseMs_; }
    
    /**
     * @brief Configure incremental GC
     */
    void configure(const IncrementalGCConfig& config) { config_ = config; }
    
private:
    bool markStep();
    bool sweepStep();
    void finishCollection();
    
    Heap* heap_;
    IncrementalGCConfig config_;
    GCPhase phase_ = GCPhase::Idle;
    
    std::vector<Runtime::Object*> markQueue_;
    size_t markIndex_ = 0;
    size_t sweepIndex_ = 0;
    
    double totalPauseMs_ = 0;
    std::chrono::steady_clock::time_point stepStart_;
};

/**
 * @brief Tri-color marking for incremental GC
 */
enum class MarkColor {
    White,  // Not visited
    Gray,   // Visited but children not processed
    Black   // Visited and children processed
};

/**
 * @brief Write barrier for incremental GC correctness
 * 
 * When a black object gains a reference to a white object,
 * we must re-gray the black object.
 */
class WriteBarrier {
public:
    static void onFieldWrite(Runtime::Object* holder, Runtime::Object* newValue);
    static void onArrayWrite(Runtime::Object* array, size_t index, Runtime::Object* newValue);
};

} // namespace Zepra::GC
