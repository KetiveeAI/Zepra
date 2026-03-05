/**
 * @file gc_scheduler.cpp
 * @brief GC scheduling — decides WHEN and WHAT type of GC to run
 *
 * The scheduler monitors allocation pressure, heap occupancy,
 * and mutator utilization to decide:
 *   - When to trigger GC (allocation budget, idle time, manual)
 *   - What type (minor, major, full, compact)
 *   - Whether to run concurrently or STW
 *
 * Strategies:
 * 1. Allocation budget: trigger minor GC every N bytes allocated
 * 2. Heap threshold: trigger major when old-gen > X% capacity
 * 3. Idle-time GC: run during event loop idle periods
 * 4. Memory pressure: OS signals low memory → full GC
 * 5. Periodic: timer-based background GC
 *
 * The scheduler also implements GC pacing — spreading work across
 * mutator allocation to avoid long pauses.
 */

#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <cmath>

namespace Zepra::Heap {

// =============================================================================
// Allocation Budget Tracker
// =============================================================================

class AllocationBudget {
public:
    explicit AllocationBudget(size_t initialBudget = 2 * 1024 * 1024)
        : budget_(initialBudget)
        , remaining_(initialBudget)
        , baseBudget_(initialBudget) {}

    /**
     * @brief Charge allocation against budget
     * @return true if budget exhausted (GC should trigger)
     */
    bool charge(size_t bytes) {
        size_t old = remaining_.load(std::memory_order_relaxed);
        while (true) {
            if (bytes >= old) {
                remaining_.store(0, std::memory_order_release);
                return true;
            }
            if (remaining_.compare_exchange_weak(old, old - bytes,
                std::memory_order_acq_rel, std::memory_order_relaxed)) {
                return false;
            }
        }
    }

    void reset() {
        remaining_.store(budget_, std::memory_order_release);
    }

    /**
     * @brief Adapt budget based on GC performance
     *
     * If GC is fast and reclaims a lot → increase budget (fewer GCs)
     * If GC is slow and reclaims little → decrease budget (more frequent)
     */
    void adapt(double gcTimeMs, size_t reclaimedBytes) {
        double efficiency = reclaimedBytes > 0 ?
            static_cast<double>(reclaimedBytes) / gcTimeMs : 0;

        // Target: keep GC pauses under 5ms
        if (gcTimeMs < 2.0 && efficiency > 1000) {
            budget_ = std::min(budget_ * 3 / 2, baseBudget_ * 4);
        } else if (gcTimeMs > 10.0) {
            budget_ = std::max(budget_ * 2 / 3, baseBudget_ / 4);
        }

        remaining_.store(budget_, std::memory_order_release);
    }

    size_t budget() const { return budget_; }
    size_t remaining() const {
        return remaining_.load(std::memory_order_relaxed);
    }

private:
    size_t budget_;
    std::atomic<size_t> remaining_;
    size_t baseBudget_;
};

// =============================================================================
// Heap Occupancy Monitor
// =============================================================================

class HeapOccupancyMonitor {
public:
    struct Thresholds {
        double minorTrigger;    // Nursery occupancy to trigger minor
        double majorTrigger;    // Old-gen occupancy to trigger major
        double fullTrigger;     // Total heap to trigger full
        double compactTrigger;  // Fragmentation to trigger compact

        Thresholds()
            : minorTrigger(0.90)
            , majorTrigger(0.75)
            , fullTrigger(0.90)
            , compactTrigger(0.40) {}
    };

    explicit HeapOccupancyMonitor(const Thresholds& t = Thresholds{})
        : thresholds_(t) {}

    enum class Recommendation : uint8_t {
        None,
        Minor,
        Major,
        Full,
        Compact
    };

    Recommendation check(size_t nurseryUsed, size_t nurseryCapacity,
                          size_t oldUsed, size_t oldCapacity,
                          double fragmentation) const {
        double nurseryOcc = nurseryCapacity > 0 ?
            static_cast<double>(nurseryUsed) / nurseryCapacity : 0;
        double oldOcc = oldCapacity > 0 ?
            static_cast<double>(oldUsed) / oldCapacity : 0;
        double totalOcc = (nurseryCapacity + oldCapacity) > 0 ?
            static_cast<double>(nurseryUsed + oldUsed) /
            (nurseryCapacity + oldCapacity) : 0;

        if (fragmentation > thresholds_.compactTrigger) {
            return Recommendation::Compact;
        }
        if (totalOcc > thresholds_.fullTrigger) {
            return Recommendation::Full;
        }
        if (oldOcc > thresholds_.majorTrigger) {
            return Recommendation::Major;
        }
        if (nurseryOcc > thresholds_.minorTrigger) {
            return Recommendation::Minor;
        }

        return Recommendation::None;
    }

    const Thresholds& thresholds() const { return thresholds_; }
    void setThresholds(const Thresholds& t) { thresholds_ = t; }

private:
    Thresholds thresholds_;
};

// =============================================================================
// Idle Time GC
// =============================================================================

/**
 * @brief Tracks event loop idle periods for background GC
 *
 * The browser event loop calls notifyIdle(deadline) when it has
 * nothing to do. If there's enough idle time, we run incremental GC.
 */
class IdleTimeScheduler {
public:
    struct Config {
        double minIdleMs;       // Min idle time to start GC work
        double maxStepMs;       // Max time per incremental step
        size_t stepsPerIdle;    // Max steps per idle notification

        Config()
            : minIdleMs(1.0)
            , maxStepMs(2.0)
            , stepsPerIdle(3) {}
    };

    explicit IdleTimeScheduler(const Config& config = Config{})
        : config_(config)
        , pendingWork_(false)
        , stepsCompleted_(0) {}

    /**
     * @brief Called by event loop when idle
     * @param deadlineMs Milliseconds until next scheduled task
     * @param gcStep Callback to run one GC step (returns true if more work)
     */
    void notifyIdle(double deadlineMs,
                     std::function<bool()> gcStep) {
        if (deadlineMs < config_.minIdleMs) return;
        if (!pendingWork_.load(std::memory_order_acquire)) return;

        double remaining = deadlineMs;
        size_t steps = 0;

        while (remaining > config_.maxStepMs &&
               steps < config_.stepsPerIdle) {
            auto start = std::chrono::steady_clock::now();

            bool moreWork = gcStep();

            double elapsed = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - start).count();
            remaining -= elapsed;
            steps++;
            stepsCompleted_++;

            if (!moreWork) {
                pendingWork_.store(false, std::memory_order_release);
                break;
            }
        }
    }

    void setPendingWork(bool pending) {
        pendingWork_.store(pending, std::memory_order_release);
    }

    bool hasPendingWork() const {
        return pendingWork_.load(std::memory_order_acquire);
    }

    uint64_t stepsCompleted() const { return stepsCompleted_; }

private:
    Config config_;
    std::atomic<bool> pendingWork_;
    uint64_t stepsCompleted_;
};

// =============================================================================
// Memory Pressure Handler
// =============================================================================

class MemoryPressureHandler {
public:
    enum class Level : uint8_t {
        None,
        Moderate,
        Critical
    };

    void onMemoryPressure(Level level) {
        lastLevel_ = level;
        pressureCount_++;

        switch (level) {
            case Level::None:
                break;
            case Level::Moderate:
                // Suggest major GC
                if (onModerate_) onModerate_();
                break;
            case Level::Critical:
                // Force full GC + shrink heap
                if (onCritical_) onCritical_();
                break;
        }
    }

    using PressureCallback = std::function<void()>;
    void setModerateHandler(PressureCallback cb) {
        onModerate_ = std::move(cb);
    }
    void setCriticalHandler(PressureCallback cb) {
        onCritical_ = std::move(cb);
    }

    Level lastLevel() const { return lastLevel_; }
    uint64_t pressureCount() const { return pressureCount_; }

private:
    Level lastLevel_ = Level::None;
    uint64_t pressureCount_ = 0;
    PressureCallback onModerate_;
    PressureCallback onCritical_;
};

// =============================================================================
// GC Scheduler (orchestrates all scheduling strategies)
// =============================================================================

class GCScheduler {
public:
    struct Config {
        size_t allocationBudget;
        HeapOccupancyMonitor::Thresholds occupancyThresholds;
        IdleTimeScheduler::Config idleConfig;

        Config()
            : allocationBudget(2 * 1024 * 1024) {}
    };

    struct Stats {
        uint64_t budgetTriggered;
        uint64_t occupancyTriggered;
        uint64_t idleTriggered;
        uint64_t pressureTriggered;
        uint64_t manualTriggered;
        uint64_t totalScheduled;
    };

    explicit GCScheduler(const Config& config = Config{})
        : budget_(config.allocationBudget)
        , occupancy_(config.occupancyThresholds)
        , idle_(config.idleConfig) {}

    using GCTrigger = std::function<void(
        HeapOccupancyMonitor::Recommendation type)>;

    void setTrigger(GCTrigger trigger) { trigger_ = std::move(trigger); }

    /**
     * @brief Called on every allocation
     */
    void onAllocation(size_t bytes) {
        if (budget_.charge(bytes)) {
            stats_.budgetTriggered++;
            stats_.totalScheduled++;
            scheduleGC(HeapOccupancyMonitor::Recommendation::Minor);
            budget_.reset();
        }
    }

    /**
     * @brief Periodic check (called from event loop tick)
     */
    void checkOccupancy(size_t nurseryUsed, size_t nurseryCapacity,
                         size_t oldUsed, size_t oldCapacity,
                         double fragmentation) {
        auto rec = occupancy_.check(nurseryUsed, nurseryCapacity,
                                     oldUsed, oldCapacity, fragmentation);
        if (rec != HeapOccupancyMonitor::Recommendation::None) {
            stats_.occupancyTriggered++;
            stats_.totalScheduled++;
            scheduleGC(rec);
        }
    }

    /**
     * @brief After GC completes: adapt budget
     */
    void onGCComplete(double gcTimeMs, size_t reclaimedBytes) {
        budget_.adapt(gcTimeMs, reclaimedBytes);
    }

    void onMemoryPressure(MemoryPressureHandler::Level level) {
        pressure_.onMemoryPressure(level);
        if (level != MemoryPressureHandler::Level::None) {
            stats_.pressureTriggered++;
            stats_.totalScheduled++;
        }
    }

    void requestGC() {
        stats_.manualTriggered++;
        stats_.totalScheduled++;
        scheduleGC(HeapOccupancyMonitor::Recommendation::Full);
    }

    IdleTimeScheduler& idleScheduler() { return idle_; }
    const Stats& stats() const { return stats_; }

private:
    void scheduleGC(HeapOccupancyMonitor::Recommendation type) {
        if (trigger_) trigger_(type);
    }

    AllocationBudget budget_;
    HeapOccupancyMonitor occupancy_;
    IdleTimeScheduler idle_;
    MemoryPressureHandler pressure_;
    GCTrigger trigger_;
    Stats stats_{};
};

} // namespace Zepra::Heap
