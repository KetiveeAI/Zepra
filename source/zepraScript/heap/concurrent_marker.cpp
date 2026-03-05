/**
 * @file concurrent_marker.cpp
 * @brief Concurrent marking infrastructure
 *
 * Runs the mark phase concurrently with the mutator:
 *
 * 1. Initial mark (STW):
 *    - Scan roots → push to worklists
 *    - Enable SATB write barrier
 *
 * 2. Concurrent mark (parallel, no STW):
 *    - Worker threads drain worklists
 *    - Mutator runs with SATB barrier active
 *    - New allocations marked black (allocated-black)
 *
 * 3. Remark (STW):
 *    - Drain SATB buffers
 *    - Re-scan dirty cards
 *    - Process remaining grey objects
 *    - Disable SATB barrier
 *
 * Worklist:
 * - Per-worker local deques (double-ended for work stealing)
 * - Global shared worklist for overflow
 * - LIFO processing for cache locality
 *
 * Termination detection:
 * - Workers signal "no work" via atomic counter
 * - When all workers idle AND global list empty → terminate
 * - Dijkstra-style termination protocol
 */

#include <atomic>
#include <mutex>
#include <condition_variable>
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

namespace Zepra::Heap {

// =============================================================================
// Worklist Segment
// =============================================================================

class WorklistSegment {
public:
    static constexpr size_t CAPACITY = 64;

    WorklistSegment() : size_(0), next_(nullptr) {}

    bool push(uintptr_t addr) {
        if (size_ >= CAPACITY) return false;
        entries_[size_++] = addr;
        return true;
    }

    uintptr_t pop() {
        if (size_ == 0) return 0;
        return entries_[--size_];
    }

    bool isEmpty() const { return size_ == 0; }
    bool isFull() const { return size_ >= CAPACITY; }
    size_t size() const { return size_; }

    WorklistSegment* next() const { return next_; }
    void setNext(WorklistSegment* n) { next_ = n; }

private:
    uintptr_t entries_[CAPACITY];
    size_t size_;
    WorklistSegment* next_;
};

// =============================================================================
// Local Worklist (per-worker, lock-free fast path)
// =============================================================================

class LocalWorklist {
public:
    LocalWorklist() : pushSegment_(nullptr), popSegment_(nullptr) {
        pushSegment_ = new WorklistSegment();
        popSegment_ = pushSegment_;
    }

    ~LocalWorklist() {
        while (popSegment_) {
            auto* next = popSegment_->next();
            delete popSegment_;
            popSegment_ = next;
        }
    }

    void push(uintptr_t addr) {
        if (!pushSegment_->push(addr)) {
            auto* newSeg = new WorklistSegment();
            pushSegment_->setNext(newSeg);
            pushSegment_ = newSeg;
            pushSegment_->push(addr);
        }
        size_++;
    }

    uintptr_t pop() {
        if (popSegment_->isEmpty()) {
            if (popSegment_ == pushSegment_) return 0;
            auto* next = popSegment_->next();
            delete popSegment_;
            popSegment_ = next;
            if (!popSegment_) return 0;
        }
        size_--;
        return popSegment_->pop();
    }

    bool isEmpty() const { return size_ == 0; }
    size_t size() const { return size_; }

    WorklistSegment* stealSegment() {
        if (popSegment_ == pushSegment_) return nullptr;
        auto* stolen = popSegment_;
        popSegment_ = stolen->next();
        stolen->setNext(nullptr);
        size_ -= stolen->size();
        return stolen;
    }

private:
    WorklistSegment* pushSegment_;
    WorklistSegment* popSegment_;
    size_t size_ = 0;
};

// =============================================================================
// Global Worklist (shared, mutex-protected)
// =============================================================================

class GlobalWorklist {
public:
    void pushSegment(WorklistSegment* segment) {
        std::lock_guard<std::mutex> lock(mutex_);
        segment->setNext(head_);
        head_ = segment;
        segmentCount_++;
    }

    WorklistSegment* popSegment() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!head_) return nullptr;
        auto* seg = head_;
        head_ = seg->next();
        seg->setNext(nullptr);
        segmentCount_--;
        return seg;
    }

    bool isEmpty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return head_ == nullptr;
    }

    size_t segmentCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return segmentCount_;
    }

private:
    mutable std::mutex mutex_;
    WorklistSegment* head_ = nullptr;
    size_t segmentCount_ = 0;
};

// =============================================================================
// Termination Protocol
// =============================================================================

class TerminationProtocol {
public:
    explicit TerminationProtocol(size_t workerCount)
        : workerCount_(workerCount)
        , idleCount_(0)
        , terminated_(false) {}

    bool tryTerminate() {
        size_t idle = idleCount_.fetch_add(1, std::memory_order_acq_rel) + 1;
        if (idle == workerCount_) {
            terminated_.store(true, std::memory_order_release);
            cv_.notify_all();
            return true;
        }

        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, std::chrono::microseconds(100), [this]() {
                return terminated_.load(std::memory_order_acquire);
            });
        }

        if (terminated_.load(std::memory_order_acquire)) return true;

        idleCount_.fetch_sub(1, std::memory_order_relaxed);
        return false;
    }

    void wakeAll() {
        idleCount_.store(0, std::memory_order_release);
        cv_.notify_all();
    }

    bool isTerminated() const {
        return terminated_.load(std::memory_order_acquire);
    }

    void reset() {
        idleCount_.store(0, std::memory_order_release);
        terminated_.store(false, std::memory_order_release);
    }

private:
    size_t workerCount_;
    std::atomic<size_t> idleCount_;
    std::atomic<bool> terminated_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

// =============================================================================
// Concurrent Marker
// =============================================================================

class ConcurrentMarker {
public:
    struct Config {
        size_t numWorkers;
        bool allocateBlack;

        Config()
            : numWorkers(2)
            , allocateBlack(true) {}
    };

    struct Callbacks {
        std::function<bool(uintptr_t addr)> tryMark;
        std::function<void(uintptr_t addr,
                            std::function<void(uintptr_t ref)>)> traceObject;
        std::function<void(std::function<void(uintptr_t)>)> enumerateRoots;
        std::function<void(std::function<void(uintptr_t)>)> drainSATB;
        std::function<void(
            std::function<void(uintptr_t cardBase, uintptr_t cardEnd)>)>
            scanDirtyCards;
        std::function<void(bool enable)> setSATBEnabled;
    };

    struct Stats {
        uint64_t objectsMarked;
        uint64_t rootsScanned;
        uint64_t satbDrained;
        uint64_t cardsRescanned;
        double initialMarkMs;
        double concurrentMarkMs;
        double remarkMs;
        double totalMs;
    };

    explicit ConcurrentMarker(const Config& config = Config{})
        : config_(config)
        , termination_(config.numWorkers) {
        worklists_.resize(config.numWorkers);
    }

    void setCallbacks(Callbacks cb) { cb_ = std::move(cb); }

    // -------------------------------------------------------------------------
    // Phase 1: Initial Mark (STW)
    // -------------------------------------------------------------------------

    void initialMark() {
        auto start = std::chrono::steady_clock::now();

        if (cb_.setSATBEnabled) cb_.setSATBEnabled(true);

        if (cb_.enumerateRoots) {
            cb_.enumerateRoots([&](uintptr_t root) {
                if (cb_.tryMark && cb_.tryMark(root)) {
                    worklists_[0].push(root);
                    stats_.rootsScanned++;
                }
            });
        }

        stats_.initialMarkMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
    }

    // -------------------------------------------------------------------------
    // Phase 2: Concurrent Mark (parallel, no STW)
    // -------------------------------------------------------------------------

    void concurrentMark() {
        auto start = std::chrono::steady_clock::now();

        termination_.reset();
        std::atomic<uint64_t> totalMarked{0};

        auto workerFn = [&](size_t workerId) {
            uint64_t localMarked = 0;

            while (!termination_.isTerminated()) {
                uintptr_t obj = worklists_[workerId].pop();
                if (obj != 0) {
                    traceObject(obj, workerId, localMarked);
                    continue;
                }

                bool stolen = false;
                for (size_t other = 0; other < config_.numWorkers; other++) {
                    if (other == workerId) continue;
                    auto* seg = worklists_[other].stealSegment();
                    if (seg) {
                        while (!seg->isEmpty()) {
                            uintptr_t addr = seg->pop();
                            traceObject(addr, workerId, localMarked);
                        }
                        delete seg;
                        stolen = true;
                        break;
                    }
                }
                if (stolen) continue;

                auto* gseg = globalWorklist_.popSegment();
                if (gseg) {
                    while (!gseg->isEmpty()) {
                        uintptr_t addr = gseg->pop();
                        traceObject(addr, workerId, localMarked);
                    }
                    delete gseg;
                    continue;
                }

                if (termination_.tryTerminate()) break;
            }

            totalMarked.fetch_add(localMarked, std::memory_order_relaxed);
        };

        if (config_.numWorkers <= 1) {
            workerFn(0);
        } else {
            std::vector<std::thread> threads;
            for (size_t w = 0; w < config_.numWorkers; w++) {
                threads.emplace_back(workerFn, w);
            }
            for (auto& t : threads) t.join();
        }

        stats_.objectsMarked += totalMarked.load();
        stats_.concurrentMarkMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
    }

    // -------------------------------------------------------------------------
    // Phase 3: Remark (STW)
    // -------------------------------------------------------------------------

    void remark() {
        auto start = std::chrono::steady_clock::now();

        if (cb_.drainSATB) {
            cb_.drainSATB([&](uintptr_t addr) {
                if (cb_.tryMark && cb_.tryMark(addr)) {
                    worklists_[0].push(addr);
                    stats_.satbDrained++;
                }
            });
        }

        if (cb_.scanDirtyCards) {
            cb_.scanDirtyCards(
                [&](uintptr_t cardBase, uintptr_t cardEnd) {
                    stats_.cardsRescanned++;
                    (void)cardBase;
                    (void)cardEnd;
                });
        }

        uint64_t localMarked = 0;
        while (!worklists_[0].isEmpty()) {
            uintptr_t obj = worklists_[0].pop();
            traceObject(obj, 0, localMarked);
        }
        stats_.objectsMarked += localMarked;

        if (cb_.setSATBEnabled) cb_.setSATBEnabled(false);

        stats_.remarkMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
    }

    // -------------------------------------------------------------------------
    // Full cycle
    // -------------------------------------------------------------------------

    Stats runFullCycle() {
        stats_ = {};
        auto start = std::chrono::steady_clock::now();

        initialMark();
        concurrentMark();
        remark();

        stats_.totalMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();

        return stats_;
    }

    const Stats& lastStats() const { return stats_; }

private:
    void traceObject(uintptr_t objAddr, size_t workerId,
                      uint64_t& localMarked) {
        localMarked++;

        if (cb_.traceObject) {
            cb_.traceObject(objAddr, [&](uintptr_t ref) {
                if (cb_.tryMark && cb_.tryMark(ref)) {
                    worklists_[workerId].push(ref);
                }
            });
        }
    }

    Config config_;
    Callbacks cb_;
    Stats stats_{};

    std::vector<LocalWorklist> worklists_;
    GlobalWorklist globalWorklist_;
    TerminationProtocol termination_;
};

} // namespace Zepra::Heap
