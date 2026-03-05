/**
 * @file gc_telemetry.cpp
 * @brief GC telemetry & metrics collection
 *
 * Collects detailed timing, throughput, and pause data for
 * every GC cycle. Used for:
 * - Performance regression detection
 * - Heap sizing decisions
 * - DevTools memory panel
 * - Verbose GC logging (ZEPRA_GC_LOG=1)
 *
 * Metrics are lock-free on the hot path (allocation counting)
 * and use a ring buffer for recent GC events.
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
#include <cstring>
#include <cmath>
#include <string>

namespace Zepra::Heap {

// =============================================================================
// GC Event Record
// =============================================================================

struct GCTelemetryEvent {
    enum class Type : uint8_t {
        Minor,
        Major,
        Full,
        Compact,
        IncrementalStep
    };

    Type type;
    uint64_t cycleId;
    double totalMs;
    double stwMs;
    double markMs;
    double sweepMs;
    double compactMs;
    size_t heapBefore;
    size_t heapAfter;
    size_t bytesReclaimed;
    size_t objectsReclaimed;
    size_t bytesPromoted;
    size_t rootsScanned;
    double mutatorUtilization;
    uint64_t timestamp;

    GCTelemetryEvent()
        : type(Type::Minor), cycleId(0)
        , totalMs(0), stwMs(0), markMs(0), sweepMs(0), compactMs(0)
        , heapBefore(0), heapAfter(0), bytesReclaimed(0)
        , objectsReclaimed(0), bytesPromoted(0), rootsScanned(0)
        , mutatorUtilization(0), timestamp(0) {}
};

// =============================================================================
// Ring Buffer for Recent Events
// =============================================================================

template<typename T, size_t N>
class TelemetryRingBuffer {
public:
    TelemetryRingBuffer() : head_(0), count_(0) {}

    void push(const T& item) {
        buffer_[head_ % N] = item;
        head_++;
        if (count_ < N) count_++;
    }

    const T& at(size_t index) const {
        assert(index < count_);
        size_t start = head_ > N ? head_ - N : 0;
        return buffer_[(start + index) % N];
    }

    size_t size() const { return count_; }
    bool empty() const { return count_ == 0; }

    const T& latest() const {
        assert(count_ > 0);
        return buffer_[(head_ - 1) % N];
    }

    void forEach(std::function<void(const T&)> fn) const {
        size_t start = head_ > N ? head_ - N : 0;
        for (size_t i = 0; i < count_; i++) {
            fn(buffer_[(start + i) % N]);
        }
    }

private:
    T buffer_[N];
    size_t head_;
    size_t count_;
};

// =============================================================================
// Pause Time Histogram
// =============================================================================

class PauseTimeHistogram {
public:
    static constexpr size_t NUM_BUCKETS = 12;

    static constexpr double BUCKET_LIMITS[NUM_BUCKETS] = {
        0.1, 0.5, 1.0, 2.0, 5.0, 10.0,
        20.0, 50.0, 100.0, 200.0, 500.0, 1e9
    };

    PauseTimeHistogram() {
        std::memset(counts_, 0, sizeof(counts_));
    }

    void record(double pauseMs) {
        for (size_t i = 0; i < NUM_BUCKETS; i++) {
            if (pauseMs < BUCKET_LIMITS[i]) {
                counts_[i]++;
                totalCount_++;
                totalMs_ += pauseMs;
                if (pauseMs > maxMs_) maxMs_ = pauseMs;
                return;
            }
        }
        counts_[NUM_BUCKETS - 1]++;
        totalCount_++;
        totalMs_ += pauseMs;
        if (pauseMs > maxMs_) maxMs_ = pauseMs;
    }

    double p50() const { return percentile(0.50); }
    double p99() const { return percentile(0.99); }
    double max() const { return maxMs_; }
    double mean() const {
        return totalCount_ > 0 ? totalMs_ / totalCount_ : 0;
    }
    uint64_t count() const { return totalCount_; }

    void report(FILE* out) const {
        fprintf(out, "  Pause histogram (n=%lu, mean=%.2fms, "
            "p50=%.2fms, p99=%.2fms, max=%.2fms):\n",
            static_cast<unsigned long>(totalCount_),
            mean(), p50(), p99(), maxMs_);

        const char* labels[NUM_BUCKETS] = {
            "<0.1ms", "<0.5ms", "<1ms", "<2ms", "<5ms", "<10ms",
            "<20ms", "<50ms", "<100ms", "<200ms", "<500ms", ">=500ms"
        };

        for (size_t i = 0; i < NUM_BUCKETS; i++) {
            if (counts_[i] > 0) {
                fprintf(out, "    %8s: %lu\n", labels[i],
                    static_cast<unsigned long>(counts_[i]));
            }
        }
    }

private:
    double percentile(double p) const {
        if (totalCount_ == 0) return 0;
        uint64_t target = static_cast<uint64_t>(p * totalCount_);
        uint64_t cumulative = 0;
        for (size_t i = 0; i < NUM_BUCKETS; i++) {
            cumulative += counts_[i];
            if (cumulative >= target) {
                return i > 0 ? BUCKET_LIMITS[i - 1] : 0;
            }
        }
        return maxMs_;
    }

    uint64_t counts_[NUM_BUCKETS];
    uint64_t totalCount_ = 0;
    double totalMs_ = 0;
    double maxMs_ = 0;
};

// =============================================================================
// Allocation Rate Sampler
// =============================================================================

class AllocationRateSampler {
public:
    void recordAllocation(size_t bytes) {
        bytesThisWindow_.fetch_add(bytes, std::memory_order_relaxed);
        allocsThisWindow_.fetch_add(1, std::memory_order_relaxed);
    }

    double sample() {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(
            now - windowStart_).count();

        size_t bytes = bytesThisWindow_.exchange(0,
            std::memory_order_relaxed);
        allocsThisWindow_.store(0, std::memory_order_relaxed);

        windowStart_ = now;

        if (elapsed > 0) {
            lastRateBps_ = bytes / elapsed;
        }

        return lastRateBps_;
    }

    double lastRate() const { return lastRateBps_; }

private:
    std::atomic<size_t> bytesThisWindow_{0};
    std::atomic<size_t> allocsThisWindow_{0};
    std::chrono::steady_clock::time_point windowStart_ =
        std::chrono::steady_clock::now();
    double lastRateBps_ = 0;
};

// =============================================================================
// GC Telemetry Collector
// =============================================================================

class GCTelemetry {
public:
    struct Summary {
        uint64_t totalCycles;
        uint64_t minorCycles;
        uint64_t majorCycles;
        double totalGCMs;
        double totalStwMs;
        double avgPauseMs;
        double maxPauseMs;
        double p99PauseMs;
        size_t totalBytesReclaimed;
        size_t totalBytesPromoted;
        double allocationRateMBps;
        double mutatorUtilization;
    };

    void recordEvent(const GCTelemetryEvent& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push(event);

        totalCycles_++;
        if (event.type == GCTelemetryEvent::Type::Minor) minorCycles_++;
        else majorCycles_++;

        totalGCMs_ += event.totalMs;
        totalStwMs_ += event.stwMs;
        totalBytesReclaimed_ += event.bytesReclaimed;
        totalBytesPromoted_ += event.bytesPromoted;

        pauseHist_.record(event.stwMs);
    }

    void recordAllocation(size_t bytes) {
        allocRate_.recordAllocation(bytes);
    }

    Summary computeSummary() {
        std::lock_guard<std::mutex> lock(mutex_);
        Summary s;
        s.totalCycles = totalCycles_;
        s.minorCycles = minorCycles_;
        s.majorCycles = majorCycles_;
        s.totalGCMs = totalGCMs_;
        s.totalStwMs = totalStwMs_;
        s.avgPauseMs = pauseHist_.mean();
        s.maxPauseMs = pauseHist_.max();
        s.p99PauseMs = pauseHist_.p99();
        s.totalBytesReclaimed = totalBytesReclaimed_;
        s.totalBytesPromoted = totalBytesPromoted_;
        s.allocationRateMBps = allocRate_.lastRate() / (1024 * 1024);
        s.mutatorUtilization = totalGCMs_ > 0 ?
            1.0 - totalGCMs_ / (totalGCMs_ + mutatorMs_) : 1.0;
        return s;
    }

    void recordMutatorTime(double ms) { mutatorMs_ += ms; }

    void reportToStderr() {
        auto s = computeSummary();
        fprintf(stderr, "[gc-telemetry] Cycles: %lu minor, %lu major\n",
            static_cast<unsigned long>(s.minorCycles),
            static_cast<unsigned long>(s.majorCycles));
        fprintf(stderr, "[gc-telemetry] GC time: %.1fms total, "
            "%.1fms STW\n", s.totalGCMs, s.totalStwMs);
        fprintf(stderr, "[gc-telemetry] Reclaimed: %.1fMB, "
            "Promoted: %.1fMB\n",
            s.totalBytesReclaimed / (1024.0 * 1024),
            s.totalBytesPromoted / (1024.0 * 1024));
        fprintf(stderr, "[gc-telemetry] Alloc: %.1fMB/s, "
            "Util: %.1f%%\n",
            s.allocationRateMBps, s.mutatorUtilization * 100);

        pauseHist_.report(stderr);
    }

    size_t recentEventCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return events_.size();
    }

private:
    mutable std::mutex mutex_;
    TelemetryRingBuffer<GCTelemetryEvent, 256> events_;
    PauseTimeHistogram pauseHist_;
    AllocationRateSampler allocRate_;

    uint64_t totalCycles_ = 0;
    uint64_t minorCycles_ = 0;
    uint64_t majorCycles_ = 0;
    double totalGCMs_ = 0;
    double totalStwMs_ = 0;
    double mutatorMs_ = 0;
    size_t totalBytesReclaimed_ = 0;
    size_t totalBytesPromoted_ = 0;
};

} // namespace Zepra::Heap
