/**
 * @file gc_tracing_engine.cpp
 * @brief GC tracing (marking) engine — the core of the mark phase
 *
 * Implements the actual object graph traversal:
 *
 * 1. Root scanning: enumerate all roots (stack, globals, handles)
 * 2. Tri-color marking: white → grey → black
 *    - White: not yet seen
 *    - Grey: seen but children not yet traced
 *    - Black: seen and all children traced
 * 3. Work stealing: parallel workers process grey objects
 * 4. SATB drain: process SATB buffers for concurrent correctness
 *
 * Mark stack:
 * - Per-worker local mark stacks (no contention on fast path)
 * - Global overflow stack (when local stack full)
 * - Work stealing between workers (dequeue from other workers)
 *
 * Optimizations:
 * - Prefetching: prefetch objects before processing
 * - Batching: process objects in batches for cache locality
 * - Mark bitmap: external bitmap for cache-friendly marking
 */

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <deque>
#include <functional>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <memory>
#include <algorithm>
#include <array>

namespace Zepra::Heap {

// =============================================================================
// Mark Color
// =============================================================================

enum class MarkColor : uint8_t {
    White = 0,  // Unreachable (to be collected)
    Grey = 1,   // Reachable, children not yet traced
    Black = 2,  // Reachable, all children traced
};

// =============================================================================
// Local Mark Stack (per-worker)
// =============================================================================

class LocalMarkStack {
public:
    static constexpr size_t CAPACITY = 4096;

    LocalMarkStack() : top_(0) {}

    bool push(uintptr_t addr) {
        if (top_ >= CAPACITY) return false;
        stack_[top_++] = addr;
        return true;
    }

    uintptr_t pop() {
        if (top_ == 0) return 0;
        return stack_[--top_];
    }

    bool isEmpty() const { return top_ == 0; }
    size_t size() const { return top_; }
    bool isFull() const { return top_ >= CAPACITY; }

    /**
     * @brief Steal half the stack (work stealing)
     */
    size_t stealHalf(uintptr_t* dest, size_t maxCount) {
        size_t toSteal = std::min(top_ / 2, maxCount);
        if (toSteal == 0) return 0;

        size_t start = top_ - toSteal;
        for (size_t i = 0; i < toSteal; i++) {
            dest[i] = stack_[start + i];
        }
        top_ = start;
        return toSteal;
    }

private:
    uintptr_t stack_[CAPACITY];
    size_t top_;
};

// =============================================================================
// Global Overflow Stack
// =============================================================================

class GlobalOverflowStack {
public:
    void push(uintptr_t addr) {
        std::lock_guard<std::mutex> lock(mutex_);
        stack_.push_back(addr);
        overflowCount_++;
    }

    void pushBatch(const uintptr_t* addrs, size_t count) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (size_t i = 0; i < count; i++) {
            stack_.push_back(addrs[i]);
        }
        overflowCount_ += count;
    }

    size_t popBatch(uintptr_t* dest, size_t maxCount) {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t count = std::min(stack_.size(), maxCount);
        for (size_t i = 0; i < count; i++) {
            dest[i] = stack_.back();
            stack_.pop_back();
        }
        return count;
    }

    bool isEmpty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stack_.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stack_.size();
    }

    uint64_t overflowCount() const { return overflowCount_; }

private:
    mutable std::mutex mutex_;
    std::vector<uintptr_t> stack_;
    uint64_t overflowCount_ = 0;
};

// =============================================================================
// Mark Bitmap (external, cache-friendly)
// =============================================================================

/**
 * @brief External mark bitmap
 *
 * Marks are stored in a separate bitmap rather than in
 * object headers. This is more cache-friendly during marking
 * because the bitmap is compact and sequential.
 *
 * 1 bit per 8-byte cell (granularity of allocation alignment).
 */
class MarkBitmap {
public:
    MarkBitmap() : base_(0), size_(0) {}

    bool initialize(uintptr_t heapBase, size_t heapSize) {
        base_ = heapBase;
        size_ = heapSize;
        size_t cells = heapSize / 8;
        size_t bitmapBytes = (cells + 7) / 8;
        bitmap_.resize(bitmapBytes, 0);
        return true;
    }

    /**
     * @brief Mark an address (atomic for concurrent marking)
     * @return true if newly marked (was white before)
     */
    bool tryMark(uintptr_t addr) {
        if (addr < base_ || addr >= base_ + size_) return false;

        size_t cellIndex = (addr - base_) / 8;
        size_t byteIndex = cellIndex / 8;
        uint8_t bitMask = 1 << (cellIndex % 8);

        if (byteIndex >= bitmap_.size()) return false;

        // Atomic test-and-set
        auto* byte = reinterpret_cast<std::atomic<uint8_t>*>(
            &bitmap_[byteIndex]);
        uint8_t old = byte->fetch_or(bitMask, std::memory_order_acq_rel);
        return (old & bitMask) == 0;
    }

    bool isMarked(uintptr_t addr) const {
        if (addr < base_ || addr >= base_ + size_) return false;

        size_t cellIndex = (addr - base_) / 8;
        size_t byteIndex = cellIndex / 8;
        uint8_t bitMask = 1 << (cellIndex % 8);

        if (byteIndex >= bitmap_.size()) return false;
        return (bitmap_[byteIndex] & bitMask) != 0;
    }

    void clear() {
        std::memset(bitmap_.data(), 0, bitmap_.size());
    }

    /**
     * @brief Count marked cells in a range
     */
    size_t countMarked(uintptr_t start, uintptr_t end) const {
        size_t count = 0;
        size_t startCell = (start - base_) / 8;
        size_t endCell = (end - base_) / 8;

        for (size_t i = startCell; i < endCell; i++) {
            size_t byteIdx = i / 8;
            if (byteIdx < bitmap_.size()) {
                if (bitmap_[byteIdx] & (1 << (i % 8))) count++;
            }
        }

        return count;
    }

    /**
     * @brief Iterate marked addresses in a range
     */
    void forEachMarked(uintptr_t start, uintptr_t end,
                        std::function<void(uintptr_t addr)> visitor) const {
        if (start < base_) start = base_;
        if (end > base_ + size_) end = base_ + size_;

        size_t startByte = ((start - base_) / 8) / 8;
        size_t endByte = std::min(((end - base_) / 8 + 7) / 8,
                                   bitmap_.size());

        for (size_t byteIdx = startByte; byteIdx < endByte; byteIdx++) {
            uint8_t byte = bitmap_[byteIdx];
            if (byte == 0) continue;

            for (int bit = 0; bit < 8; bit++) {
                if (byte & (1 << bit)) {
                    size_t cellIdx = byteIdx * 8 + bit;
                    uintptr_t addr = base_ + cellIdx * 8;
                    if (addr >= start && addr < end) {
                        visitor(addr);
                    }
                }
            }
        }
    }

private:
    uintptr_t base_;
    size_t size_;
    std::vector<uint8_t> bitmap_;
};

// =============================================================================
// Worker Statistics
// =============================================================================

struct WorkerStats {
    uint64_t objectsMarked = 0;
    uint64_t bytesTraced = 0;
    uint64_t overflowPushes = 0;
    uint64_t steals = 0;
    uint64_t stolenObjects = 0;
    uint64_t prefetches = 0;
    double durationMs = 0;
};

// =============================================================================
// Tracing Engine
// =============================================================================

class GCTracingEngine {
public:
    struct Config {
        size_t numWorkers;
        size_t prefetchDistance;
        size_t batchSize;
        bool enableWorkStealing;

        Config()
            : numWorkers(2)
            , prefetchDistance(4)
            , batchSize(64)
            , enableWorkStealing(true) {}
    };

    struct Callbacks {
        // Get object size
        std::function<size_t(uintptr_t addr)> objectSize;

        // Get outgoing references from object
        std::function<void(uintptr_t addr,
                            std::function<void(uintptr_t ref)>)>
            traceObject;

        // Enumerate root pointers
        std::function<void(std::function<void(uintptr_t root)>)>
            enumerateRoots;

        // Drain SATB buffers (for concurrent mode)
        std::function<void(std::function<void(uintptr_t addr)>)>
            drainSATB;
    };

    struct Stats {
        uint64_t totalObjectsMarked;
        uint64_t totalBytesTraced;
        uint64_t rootCount;
        uint64_t overflowCount;
        uint64_t stealCount;
        double rootScanMs;
        double markingMs;
        double satbDrainMs;
        double totalMs;
        std::vector<WorkerStats> workerStats;
    };

    explicit GCTracingEngine(const Config& config = Config{})
        : config_(config) {
        workerStacks_.resize(config.numWorkers);
    }

    void setCallbacks(Callbacks cb) { cb_ = std::move(cb); }
    void setMarkBitmap(MarkBitmap* bitmap) { bitmap_ = bitmap; }

    // -------------------------------------------------------------------------
    // Phase 1: Root scanning
    // -------------------------------------------------------------------------

    /**
     * @brief Scan all roots and push to mark stack
     *
     * Called during initial mark (STW).
     */
    size_t scanRoots() {
        if (!cb_.enumerateRoots || !bitmap_) return 0;

        size_t rootCount = 0;
        auto start = std::chrono::steady_clock::now();

        cb_.enumerateRoots([&](uintptr_t root) {
            if (bitmap_->tryMark(root)) {
                if (!workerStacks_[0].push(root)) {
                    overflowStack_.push(root);
                }
                rootCount++;
            }
        });

        stats_.rootScanMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        stats_.rootCount = rootCount;

        return rootCount;
    }

    // -------------------------------------------------------------------------
    // Phase 2: Parallel marking
    // -------------------------------------------------------------------------

    /**
     * @brief Run parallel marking with work stealing
     */
    void markParallel() {
        if (!cb_.traceObject || !bitmap_) return;

        auto start = std::chrono::steady_clock::now();

        std::atomic<size_t> totalMarked{0};
        std::atomic<bool> workAvailable{true};

        size_t numWorkers = std::min(config_.numWorkers,
                                      workerStacks_.size());

        auto workerFn = [&](size_t workerId) {
            WorkerStats wstats{};
            auto wstart = std::chrono::steady_clock::now();

            while (workAvailable.load(std::memory_order_acquire)) {
                // Process local stack
                while (!workerStacks_[workerId].isEmpty()) {
                    uintptr_t obj = workerStacks_[workerId].pop();
                    traceOneObject(obj, workerId, wstats);
                }

                // Try overflow stack
                uintptr_t batch[64];
                size_t got = overflowStack_.popBatch(batch, 64);
                if (got > 0) {
                    for (size_t i = 0; i < got; i++) {
                        traceOneObject(batch[i], workerId, wstats);
                    }
                    continue;
                }

                // Try work stealing
                if (config_.enableWorkStealing) {
                    bool stolen = false;
                    for (size_t other = 0; other < numWorkers; other++) {
                        if (other == workerId) continue;
                        uintptr_t stolen_batch[32];
                        size_t count = workerStacks_[other].stealHalf(
                            stolen_batch, 32);
                        if (count > 0) {
                            wstats.steals++;
                            wstats.stolenObjects += count;
                            for (size_t i = 0; i < count; i++) {
                                traceOneObject(stolen_batch[i], workerId,
                                               wstats);
                            }
                            stolen = true;
                            break;
                        }
                    }
                    if (stolen) continue;
                }

                // No work found → termination check
                break;
            }

            wstats.durationMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - wstart).count();
            totalMarked.fetch_add(wstats.objectsMarked,
                                   std::memory_order_relaxed);

            // Store worker stats
            if (workerId < workerStatsArr_.size()) {
                workerStatsArr_[workerId] = wstats;
            }
        };

        workerStatsArr_.resize(numWorkers);

        if (numWorkers <= 1) {
            workerFn(0);
        } else {
            std::vector<std::thread> threads;
            for (size_t w = 0; w < numWorkers; w++) {
                threads.emplace_back(workerFn, w);
            }
            for (auto& t : threads) t.join();
        }

        workAvailable.store(false, std::memory_order_release);

        stats_.totalObjectsMarked = totalMarked.load();
        stats_.markingMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        stats_.overflowCount = overflowStack_.overflowCount();
        stats_.workerStats.assign(workerStatsArr_.begin(),
                                   workerStatsArr_.end());
    }

    // -------------------------------------------------------------------------
    // Phase 3: SATB drain (for concurrent marking correctness)
    // -------------------------------------------------------------------------

    /**
     * @brief Drain SATB buffers and mark any new grey objects
     *
     * Called during remark (STW) to process references written
     * during concurrent marking.
     */
    size_t drainSATB() {
        if (!cb_.drainSATB || !bitmap_) return 0;

        auto start = std::chrono::steady_clock::now();
        size_t drained = 0;

        cb_.drainSATB([&](uintptr_t addr) {
            if (bitmap_->tryMark(addr)) {
                if (!workerStacks_[0].push(addr)) {
                    overflowStack_.push(addr);
                }
                drained++;
            }
        });

        // Process newly grey objects
        while (!workerStacks_[0].isEmpty() || !overflowStack_.isEmpty()) {
            while (!workerStacks_[0].isEmpty()) {
                uintptr_t obj = workerStacks_[0].pop();
                WorkerStats dummy{};
                traceOneObject(obj, 0, dummy);
                drained++;
            }

            uintptr_t batch[64];
            size_t got = overflowStack_.popBatch(batch, 64);
            for (size_t i = 0; i < got; i++) {
                if (!workerStacks_[0].push(batch[i])) break;
            }
        }

        stats_.satbDrainMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();

        return drained;
    }

    // -------------------------------------------------------------------------
    // Full trace cycle
    // -------------------------------------------------------------------------

    Stats trace() {
        stats_ = {};
        auto start = std::chrono::steady_clock::now();

        scanRoots();
        markParallel();

        stats_.totalMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();

        return stats_;
    }

    void reset() {
        for (auto& stack : workerStacks_) {
            while (!stack.isEmpty()) stack.pop();
        }
        if (bitmap_) bitmap_->clear();
        stats_ = {};
    }

    const Stats& lastStats() const { return stats_; }

private:
    void traceOneObject(uintptr_t objAddr, size_t workerId,
                         WorkerStats& wstats) {
        wstats.objectsMarked++;

        if (cb_.objectSize) {
            wstats.bytesTraced += cb_.objectSize(objAddr);
        }

        // Trace child references
        if (cb_.traceObject) {
            cb_.traceObject(objAddr, [&](uintptr_t ref) {
                if (bitmap_->tryMark(ref)) {
                    if (!workerStacks_[workerId].push(ref)) {
                        overflowStack_.push(ref);
                        wstats.overflowPushes++;
                    }
                }
            });
        }
    }

    Config config_;
    Callbacks cb_;
    MarkBitmap* bitmap_ = nullptr;

    std::vector<LocalMarkStack> workerStacks_;
    GlobalOverflowStack overflowStack_;

    Stats stats_{};
    std::vector<WorkerStats> workerStatsArr_;
};

} // namespace Zepra::Heap
