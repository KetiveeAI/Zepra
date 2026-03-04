/**
 * @file concurrent_gc.hpp
 * @brief Concurrent garbage collector with background marking
 * 
 * Implements concurrent marking using tri-color abstraction:
 * - White: Unmarked (potentially garbage)
 * - Gray: Marked but children not traced
 * - Black: Marked and children traced
 */

#pragma once

#include "gc_heap.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace Zepra::GC {

using Runtime::Object;
using Runtime::ObjectHeader;
using Runtime::Generation;

/**
 * @brief Tri-color marking state
 */
enum class MarkColor : uint8_t {
    White = 0,  // Unmarked - potentially garbage
    Gray = 1,   // Marked but not fully traced
    Black = 2   // Marked and fully traced
};

/**
 * @brief Thread-safe work queue for concurrent marking
 */
class MarkWorkQueue {
public:
    void push(Object* obj) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(obj);
        cv_.notify_one();
    }
    
    Object* pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) return nullptr;
        Object* obj = queue_.front();
        queue_.pop();
        return obj;
    }
    
    Object* waitAndPop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty() || stopped_; });
        if (stopped_ && queue_.empty()) return nullptr;
        Object* obj = queue_.front();
        queue_.pop();
        return obj;
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    
    void stop() {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_ = true;
        cv_.notify_all();
    }
    
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) queue_.pop();
        stopped_ = false;
    }
    
private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<Object*> queue_;
    bool stopped_ = false;
};

/**
 * @brief Concurrent GC statistics
 */
struct ConcurrentGCStats {
    std::atomic<size_t> objectsMarked{0};
    std::atomic<size_t> bytesMarked{0};
    std::atomic<size_t> markTime{0};     // microseconds
    std::atomic<size_t> pauseTime{0};    // microseconds (stop-the-world)
    std::atomic<size_t> concurrentCycles{0};
};

/**
 * @brief Concurrent garbage collector
 * 
 * Uses snapshot-at-the-beginning (SATB) write barrier for correctness.
 * Background thread performs marking concurrently with mutator.
 */
class ConcurrentGC {
public:
    ConcurrentGC();
    ~ConcurrentGC();
    
    // Disable copy/move
    ConcurrentGC(const ConcurrentGC&) = delete;
    ConcurrentGC& operator=(const ConcurrentGC&) = delete;
    
    /**
     * @brief Initialize with heap reference
     */
    void init(Runtime::GCHeap* heap);
    
    /**
     * @brief Start a concurrent GC cycle
     * @return true if cycle started, false if already running
     */
    bool startCycle();
    
    /**
     * @brief Wait for current cycle to complete
     */
    void waitForCycle();
    
    /**
     * @brief Check if concurrent marking is in progress
     */
    bool isMarking() const { return marking_.load(std::memory_order_acquire); }
    
    /**
     * @brief SATB write barrier - call before overwriting a pointer
     * 
     * Must be called by mutator when storing to object slots.
     * Records the old value to prevent lost gray-to-white pointer.
     */
    void satbWriteBarrier(Object* oldValue);
    
    /**
     * @brief Mark an object gray (add to work queue)
     */
    void markGray(Object* obj);
    
    /**
     * @brief Get the color of an object
     */
    MarkColor getColor(Object* obj) const;
    
    /**
     * @brief Set thread count for parallel marking
     */
    void setWorkerCount(unsigned count);
    
    /**
     * @brief Get statistics
     */
    const ConcurrentGCStats& stats() const { return stats_; }
    
private:
    Runtime::GCHeap* heap_ = nullptr;
    
    // Marking state
    std::atomic<bool> marking_{false};
    std::atomic<bool> stopping_{false};
    
    // Work queue for gray objects
    MarkWorkQueue workQueue_;
    
    // SATB buffer for write barrier
    std::mutex satbMutex_;
    std::vector<Object*> satbBuffer_;
    
    // Object color map (could be inlined in ObjectHeader for production)
    mutable std::mutex colorMutex_;
    std::unordered_map<Object*, MarkColor> colors_;
    
    // Worker threads
    std::vector<std::thread> workers_;
    unsigned workerCount_ = 1;
    
    // Root snapshot
    std::vector<Object*> rootSnapshot_;
    
    // Stats
    ConcurrentGCStats stats_;
    
    // Internal methods
    void workerLoop();
    void markObject(Object* obj);
    void traceObject(Object* obj);
    void processRoots();
    void processSatbBuffer();
    void drainWorkQueue();
    void finalMark();
    void setColor(Object* obj, MarkColor color);
};

/**
 * @brief RAII helper for SATB write barrier
 */
class SATBGuard {
public:
    SATBGuard(ConcurrentGC& gc, Object** slot)
        : gc_(gc), slot_(slot), oldValue_(*slot) {}
    
    ~SATBGuard() {
        if (gc_.isMarking() && oldValue_) {
            gc_.satbWriteBarrier(oldValue_);
        }
    }
    
private:
    ConcurrentGC& gc_;
    Object** slot_;
    Object* oldValue_;
};

} // namespace Zepra::GC
