/**
 * @file gc_incremental.cpp
 * @brief Incremental GC marking — spread marking work across allocations
 *
 * Instead of marking the entire heap in one STW pause, incremental
 * marking interleaves small marking steps with mutator execution:
 *
 *   allocate() → mark N objects → return
 *
 * This keeps individual pauses short (target <1ms per step).
 *
 * Key mechanism: tri-color marking with write barriers
 * - White: not yet seen
 * - Grey: seen but children not traced
 * - Black: fully traced
 *
 * Write barrier ensures correctness: if a black object gets
 * a reference to a white object, the white object is greyed.
 */

#include <atomic>
#include <mutex>
#include <vector>
#include <deque>
#include <functional>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <unordered_set>

namespace Zepra::Heap {

// =============================================================================
// Incremental Marking State
// =============================================================================

enum class IncrementalState : uint8_t {
    Idle,
    Marking,
    MarkingComplete,
    Sweeping,
    SweepComplete
};

// =============================================================================
// Grey Set (worklist for incremental marking)
// =============================================================================

class GreySet {
public:
    void push(uintptr_t addr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (dedup_.find(addr) != dedup_.end()) return;
        worklist_.push_back(addr);
        dedup_.insert(addr);
    }

    uintptr_t pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (worklist_.empty()) return 0;
        uintptr_t addr = worklist_.back();
        worklist_.pop_back();
        dedup_.erase(addr);
        return addr;
    }

    bool isEmpty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return worklist_.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return worklist_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        worklist_.clear();
        dedup_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::vector<uintptr_t> worklist_;
    std::unordered_set<uintptr_t> dedup_;
};

// =============================================================================
// Incremental Marker
// =============================================================================

class IncrementalMarker {
public:
    struct Config {
        size_t bytesPerStep;       // Allocation bytes between steps
        size_t objectsPerStep;     // Max objects to mark per step
        double maxStepMs;          // Time budget per step

        Config()
            : bytesPerStep(4096)
            , objectsPerStep(64)
            , maxStepMs(1.0) {}
    };

    struct Callbacks {
        std::function<bool(uintptr_t addr)> tryMark;
        std::function<void(uintptr_t addr,
            std::function<void(uintptr_t ref)>)> traceObject;
        std::function<void(std::function<void(uintptr_t)>)> enumerateRoots;
    };

    struct Stats {
        uint64_t totalSteps;
        uint64_t totalObjectsMarked;
        double totalStepMs;
        double avgStepMs;
        double maxStepMs;
        size_t stepsToComplete;
    };

    explicit IncrementalMarker(const Config& config = Config{})
        : config_(config)
        , state_(IncrementalState::Idle)
        , bytesSinceStep_(0) {}

    void setCallbacks(Callbacks cb) { cb_ = std::move(cb); }

    // -------------------------------------------------------------------------
    // Start/Stop
    // -------------------------------------------------------------------------

    void startMarking() {
        state_.store(IncrementalState::Marking, std::memory_order_release);
        greySet_.clear();
        stats_ = {};

        // Scan roots → grey set
        if (cb_.enumerateRoots) {
            cb_.enumerateRoots([&](uintptr_t root) {
                if (cb_.tryMark && cb_.tryMark(root)) {
                    greySet_.push(root);
                }
            });
        }
    }

    void completeMarking() {
        // Final drain: empty the grey set fully (during STW)
        while (!greySet_.isEmpty()) {
            uintptr_t obj = greySet_.pop();
            if (obj != 0) markObject(obj);
        }

        state_.store(IncrementalState::MarkingComplete,
                      std::memory_order_release);
    }

    // -------------------------------------------------------------------------
    // Per-allocation stepping
    // -------------------------------------------------------------------------

    /**
     * @brief Called on allocation to interleave marking work
     */
    void onAllocation(size_t bytes) {
        if (state_.load(std::memory_order_acquire) !=
            IncrementalState::Marking) return;

        bytesSinceStep_ += bytes;
        if (bytesSinceStep_ >= config_.bytesPerStep) {
            doStep();
            bytesSinceStep_ = 0;
        }
    }

    /**
     * @brief Run one marking step
     */
    void doStep() {
        if (state_.load(std::memory_order_acquire) !=
            IncrementalState::Marking) return;

        auto start = std::chrono::steady_clock::now();
        size_t objectsMarked = 0;

        while (objectsMarked < config_.objectsPerStep) {
            uintptr_t obj = greySet_.pop();
            if (obj == 0) {
                // No more grey objects → marking complete
                state_.store(IncrementalState::MarkingComplete,
                              std::memory_order_release);
                break;
            }

            markObject(obj);
            objectsMarked++;

            // Check time budget
            double elapsed = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - start).count();
            if (elapsed > config_.maxStepMs) break;
        }

        double stepMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();

        stats_.totalSteps++;
        stats_.totalObjectsMarked += objectsMarked;
        stats_.totalStepMs += stepMs;
        stats_.avgStepMs = stats_.totalStepMs / stats_.totalSteps;
        if (stepMs > stats_.maxStepMs) stats_.maxStepMs = stepMs;
    }

    // -------------------------------------------------------------------------
    // Write barrier integration
    // -------------------------------------------------------------------------

    /**
     * @brief Called from write barrier when black→white edge created
     *
     * If a black (fully traced) object gets a reference to a white
     * (not yet seen) object during incremental marking, the white
     * object must be greyed to maintain the invariant.
     */
    void writeBarrierBlackToWhite(uintptr_t newRefAddr) {
        if (state_.load(std::memory_order_acquire) !=
            IncrementalState::Marking) return;

        if (cb_.tryMark && cb_.tryMark(newRefAddr)) {
            greySet_.push(newRefAddr);
        }
    }

    IncrementalState currentState() const {
        return state_.load(std::memory_order_acquire);
    }

    bool isMarking() const {
        return currentState() == IncrementalState::Marking;
    }

    const Stats& stats() const { return stats_; }

private:
    void markObject(uintptr_t addr) {
        if (cb_.traceObject) {
            cb_.traceObject(addr, [&](uintptr_t ref) {
                if (cb_.tryMark && cb_.tryMark(ref)) {
                    greySet_.push(ref);
                }
            });
        }
    }

    Config config_;
    Callbacks cb_;
    Stats stats_{};

    std::atomic<IncrementalState> state_;
    GreySet greySet_;
    size_t bytesSinceStep_;
};

} // namespace Zepra::Heap
