/**
 * @file gc_finalizer_queue.cpp
 * @brief Finalizer queue for GC-driven cleanup callbacks
 *
 * When the GC collects an object that has a destructor or
 * FinalizationRegistry callback, the actual cleanup runs
 * AFTER the GC pause (never during STW). This queue holds
 * pending finalizers and drains them during the next event
 * loop tick.
 *
 * Safety:
 * - Finalizers MUST NOT allocate (would re-enter GC)
 * - Finalizers run in FIFO order
 * - Finalizers that throw are caught and logged
 * - Queue has a drain limit per tick to avoid stalls
 */

#include <atomic>
#include <mutex>
#include <deque>
#include <vector>
#include <functional>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Heap {

// =============================================================================
// Finalizer Record
// =============================================================================

struct FinalizerRecord {
    enum class Type : uint8_t {
        NativeDestructor,        // C++ destructor
        FinalizationCallback,    // FinalizationRegistry cleanup
        WeakCallback,            // PersistentHandle weak callback
        InternalCleanup          // Engine-internal cleanup
    };

    Type type;
    uintptr_t objectAddr;
    uintptr_t heldValue;          // For FinalizationRegistry
    std::function<void()> callback;
    uint64_t enqueuedCycle;

    FinalizerRecord()
        : type(Type::NativeDestructor)
        , objectAddr(0), heldValue(0), enqueuedCycle(0) {}
};

// =============================================================================
// Finalizer Queue
// =============================================================================

class FinalizerQueue {
public:
    struct Config {
        size_t maxDrainPerTick;    // Limit finalizers per event loop tick
        double maxDrainTimeMs;     // Time limit per drain
        bool logExceptions;

        Config()
            : maxDrainPerTick(100)
            , maxDrainTimeMs(5.0)
            , logExceptions(true) {}
    };

    struct Stats {
        uint64_t totalEnqueued;
        uint64_t totalDrained;
        uint64_t totalExceptions;
        uint64_t drainCalls;
        double totalDrainMs;
    };

    explicit FinalizerQueue(const Config& config = Config{})
        : config_(config) {}

    /**
     * @brief Enqueue a finalizer (called during GC, within STW)
     */
    void enqueue(const FinalizerRecord& record) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(record);
        stats_.totalEnqueued++;
    }

    void enqueueNative(uintptr_t addr, std::function<void()> dtor) {
        FinalizerRecord rec;
        rec.type = FinalizerRecord::Type::NativeDestructor;
        rec.objectAddr = addr;
        rec.callback = std::move(dtor);
        rec.enqueuedCycle = currentCycle_;
        enqueue(rec);
    }

    void enqueueFinalizationCallback(uintptr_t addr, uintptr_t heldValue,
                                       std::function<void()> cb) {
        FinalizerRecord rec;
        rec.type = FinalizerRecord::Type::FinalizationCallback;
        rec.objectAddr = addr;
        rec.heldValue = heldValue;
        rec.callback = std::move(cb);
        rec.enqueuedCycle = currentCycle_;
        enqueue(rec);
    }

    void enqueueWeakCallback(uintptr_t addr, std::function<void()> cb) {
        FinalizerRecord rec;
        rec.type = FinalizerRecord::Type::WeakCallback;
        rec.objectAddr = addr;
        rec.callback = std::move(cb);
        rec.enqueuedCycle = currentCycle_;
        enqueue(rec);
    }

    // -------------------------------------------------------------------------
    // Draining (called from event loop, NOT during GC)
    // -------------------------------------------------------------------------

    /**
     * @brief Drain pending finalizers up to limit
     * @return Number of finalizers executed
     */
    size_t drain() {
        auto start = std::chrono::steady_clock::now();
        stats_.drainCalls++;

        std::vector<FinalizerRecord> batch;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            size_t count = std::min(queue_.size(), config_.maxDrainPerTick);
            for (size_t i = 0; i < count; i++) {
                batch.push_back(std::move(queue_.front()));
                queue_.pop_front();
            }
        }

        size_t executed = 0;
        for (auto& rec : batch) {
            double elapsed = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - start).count();
            if (elapsed > config_.maxDrainTimeMs && executed > 0) {
                // Time budget exceeded — re-enqueue remaining
                std::lock_guard<std::mutex> lock(mutex_);
                for (size_t i = executed; i < batch.size(); i++) {
                    queue_.push_front(std::move(batch[i]));
                }
                break;
            }

            try {
                if (rec.callback) rec.callback();
                executed++;
                stats_.totalDrained++;
            } catch (...) {
                stats_.totalExceptions++;
                executed++;
                stats_.totalDrained++;
                if (config_.logExceptions) {
                    fprintf(stderr, "[gc-finalizer] Exception in "
                        "finalizer for 0x%lx (type=%u)\n",
                        static_cast<unsigned long>(rec.objectAddr),
                        static_cast<unsigned>(rec.type));
                }
            }
        }

        double ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        stats_.totalDrainMs += ms;

        return executed;
    }

    /**
     * @brief Drain all (for shutdown)
     */
    size_t drainAll() {
        size_t total = 0;
        while (!isEmpty()) {
            total += drain();
        }
        return total;
    }

    bool isEmpty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    size_t pendingCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    void setCurrentCycle(uint64_t cycle) { currentCycle_ = cycle; }
    Stats computeStats() const { return stats_; }

private:
    Config config_;
    mutable std::mutex mutex_;
    std::deque<FinalizerRecord> queue_;
    Stats stats_{};
    uint64_t currentCycle_ = 0;
};

} // namespace Zepra::Heap
