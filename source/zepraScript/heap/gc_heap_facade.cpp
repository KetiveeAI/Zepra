/**
 * @file gc_heap_facade.cpp
 * @brief GCHeap facade — the bridge between GC subsystem and runtime
 *
 * The VM accesses GC through a single GCHeap* pointer (vm.hpp:setGCHeap).
 * This file implements GCHeap, wiring together all the GC subsystems:
 * - RegionAllocator for allocation
 * - GCTracingEngine for marking
 * - GCPipelineOrchestrator for cycle management
 * - HeapControllerV2 for sizing/triggering
 * - SafePointCoordinator for thread synchronization
 * - ShapeTable for hidden class management
 *
 * The runtime calls:
 *   gcHeap->allocate(size)         → TLAB fast path
 *   gcHeap->writeBarrier(src, dst) → generational barrier
 *   gcHeap->addRoot(ptr)           → root set management
 *   gcHeap->collectGarbage()       → trigger GC cycle
 *
 * Object::visitRefs() is the callback the GC uses to trace objects.
 */

#include <atomic>
#include <mutex>
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
#include <unordered_set>

namespace Zepra::Heap {

// =============================================================================
// Forward Declarations (these live in the runtime)
// =============================================================================

// From runtime/objects/object.hpp
// class Object { void visitRefs(visitor); bool isMarked(); void clearMark(); };
// From runtime/objects/value.hpp
// class Value { bool isObject(); Object* asObject(); uint64_t rawBits(); };

// =============================================================================
// Root Set
// =============================================================================

/**
 * @brief Tracks roots — pointers from the stack, globals, and handles
 *         into the heap. The GC traces from these.
 */
class RootSet {
public:
    void addRoot(void** slot) {
        std::lock_guard<std::mutex> lock(mutex_);
        roots_.insert(slot);
    }

    void removeRoot(void** slot) {
        std::lock_guard<std::mutex> lock(mutex_);
        roots_.erase(slot);
    }

    void addStrongHandle(uintptr_t handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        strongHandles_.insert(handle);
    }

    void removeStrongHandle(uintptr_t handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        strongHandles_.erase(handle);
    }

    void enumerate(std::function<void(void** slot)> visitor) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto* slot : roots_) {
            visitor(slot);
        }
    }

    void enumerateHandles(std::function<void(uintptr_t addr)> visitor) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto addr : strongHandles_) {
            visitor(addr);
        }
    }

    size_t rootCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return roots_.size();
    }

    size_t handleCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return strongHandles_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        roots_.clear();
        strongHandles_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_set<void**> roots_;
    std::unordered_set<uintptr_t> strongHandles_;
};

// =============================================================================
// Write Barrier Sink
// =============================================================================

/**
 * @brief Receives write barrier notifications from the runtime
 *
 * When the runtime writes a reference (e.g., obj.x = otherObj),
 * it calls writeBarrier(src, slot, newValue).
 *
 * This records:
 * 1. The old value (for SATB snapshot-at-the-beginning)
 * 2. Dirty card marking (for remembered set)
 */
class WriteBarrierSink {
public:
    struct Config {
        size_t satbBufferSize;
        Config() : satbBufferSize(1024) {}
    };

    explicit WriteBarrierSink(const Config& config = Config{})
        : config_(config)
        , satbEnabled_(false)
        , cardDirtyCallback_(nullptr) {}

    /**
     * @brief Pre-write barrier (SATB): capture old value before overwrite
     */
    void preWrite(uintptr_t oldValue) {
        if (!satbEnabled_.load(std::memory_order_acquire)) return;
        if (oldValue == 0) return;

        std::lock_guard<std::mutex> lock(satbMutex_);
        satbBuffer_.push_back(oldValue);

        if (satbBuffer_.size() >= config_.satbBufferSize) {
            flushSATBLocked();
        }
    }

    /**
     * @brief Post-write barrier: mark card dirty for cross-region refs
     */
    void postWrite(uintptr_t srcAddr, uintptr_t dstAddr) {
        if (cardDirtyCallback_) {
            cardDirtyCallback_(srcAddr, dstAddr);
        }
    }

    /**
     * @brief Combined barrier: pre + post
     */
    void writeBarrier(uintptr_t srcAddr, uintptr_t oldValue,
                       uintptr_t newValue) {
        preWrite(oldValue);
        if (newValue != 0) {
            postWrite(srcAddr, newValue);
        }
    }

    void enableSATB() {
        satbEnabled_.store(true, std::memory_order_release);
    }

    void disableSATB() {
        satbEnabled_.store(false, std::memory_order_release);
    }

    void drainSATB(std::function<void(uintptr_t addr)> visitor) {
        std::lock_guard<std::mutex> lock(satbMutex_);
        for (auto addr : satbBuffer_) {
            visitor(addr);
        }
        satbBuffer_.clear();
        for (auto addr : overflowBuffer_) {
            visitor(addr);
        }
        overflowBuffer_.clear();
    }

    using CardDirtyFn = std::function<void(uintptr_t src, uintptr_t dst)>;
    void setCardDirtyCallback(CardDirtyFn fn) { cardDirtyCallback_ = std::move(fn); }

private:
    void flushSATBLocked() {
        overflowBuffer_.insert(overflowBuffer_.end(),
            satbBuffer_.begin(), satbBuffer_.end());
        satbBuffer_.clear();
    }

    Config config_;
    std::atomic<bool> satbEnabled_;
    std::mutex satbMutex_;
    std::vector<uintptr_t> satbBuffer_;
    std::vector<uintptr_t> overflowBuffer_;
    CardDirtyFn cardDirtyCallback_;
};

// =============================================================================
// GCHeap — the unified facade
// =============================================================================

/**
 * @brief The GC heap facade exposed to the VM
 *
 * This is the object that vm.setGCHeap() receives.
 * It coordinates all GC subsystems into a single API.
 */
class GCHeap {
public:
    struct Config {
        size_t initialHeapSize;
        size_t maxHeapSize;
        size_t nurserySize;
        bool verbose;

        Config()
            : initialHeapSize(32 * 1024 * 1024)
            , maxHeapSize(2048ULL * 1024 * 1024)
            , nurserySize(4 * 1024 * 1024)
            , verbose(false) {}
    };

    struct Stats {
        uint64_t totalAllocations;
        uint64_t totalBytesAllocated;
        uint64_t totalGCCycles;
        uint64_t minorGCCycles;
        uint64_t majorGCCycles;
        uint64_t totalBytesReclaimed;
        double totalGCTimeMs;
        double totalStwMs;
        size_t currentHeapUsed;
        size_t currentHeapCapacity;
        size_t rootCount;
        size_t handleCount;
    };

    explicit GCHeap(const Config& config = Config{})
        : config_(config)
        , bytesAllocated_(0)
        , heapUsed_(0)
        , gcCycles_(0)
        , initialized_(false) {}

    /**
     * @brief Initialize the heap
     */
    bool initialize() {
        if (initialized_) return true;
        initialized_ = true;
        heapCapacity_ = config_.initialHeapSize;
        return true;
    }

    // -------------------------------------------------------------------------
    // Allocation (called by runtime for new objects)
    // -------------------------------------------------------------------------

    /**
     * @brief Allocate memory for a new object
     *
     * Fast path: bump pointer in TLAB (no lock)
     * Slow path: refill TLAB or allocate new region
     * Last resort: trigger GC and retry
     *
     * @param size Object size in bytes
     * @return Allocated address, or 0 on OOM
     */
    uintptr_t allocate(size_t size) {
        size = (size + 7) & ~size_t(7);  // 8-byte alignment

        bytesAllocated_.fetch_add(size, std::memory_order_relaxed);
        heapUsed_.fetch_add(size, std::memory_order_relaxed);
        stats_.totalAllocations++;
        stats_.totalBytesAllocated += size;

        // Check if we should trigger GC
        if (shouldTriggerGC()) {
            collectGarbage(false);  // Try minor GC
        }

        // In a full implementation, this calls RegionAllocator.
        // For now, use the system allocator as backing.
        void* mem = std::aligned_alloc(8, size);
        if (!mem) {
            // OOM — try emergency GC
            collectGarbage(true);
            mem = std::aligned_alloc(8, size);
            if (!mem) return 0;
        }

        std::memset(mem, 0, size);
        return reinterpret_cast<uintptr_t>(mem);
    }

    /**
     * @brief Allocate with explicit type for GC metadata
     */
    uintptr_t allocateTyped(size_t size, uint8_t objectType) {
        uintptr_t addr = allocate(size);
        if (addr != 0) {
            // Track object type for snapshot/diagnostics
            std::lock_guard<std::mutex> lock(trackingMutex_);
            liveObjects_[addr] = {size, objectType};
        }
        return addr;
    }

    // -------------------------------------------------------------------------
    // Write barrier (called by runtime on reference writes)
    // -------------------------------------------------------------------------

    void writeBarrier(uintptr_t srcAddr, uintptr_t oldRef,
                       uintptr_t newRef) {
        barrierSink_.writeBarrier(srcAddr, oldRef, newRef);
    }

    // -------------------------------------------------------------------------
    // Root management
    // -------------------------------------------------------------------------

    void addRoot(void** slot) { rootSet_.addRoot(slot); }
    void removeRoot(void** slot) { rootSet_.removeRoot(slot); }
    void addStrongHandle(uintptr_t handle) {
        rootSet_.addStrongHandle(handle);
    }
    void removeStrongHandle(uintptr_t handle) {
        rootSet_.removeStrongHandle(handle);
    }

    // -------------------------------------------------------------------------
    // GC trigger
    // -------------------------------------------------------------------------

    /**
     * @brief Trigger garbage collection
     * @param full If true, do a full GC; otherwise try minor first
     */
    void collectGarbage(bool full = false) {
        gcCycles_++;

        auto start = std::chrono::steady_clock::now();

        if (full) {
            stats_.majorGCCycles++;
            doFullGC();
        } else {
            stats_.minorGCCycles++;
            doMinorGC();
        }

        double elapsed = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        stats_.totalGCTimeMs += elapsed;

        if (config_.verbose) {
            fprintf(stderr, "[gc-heap] Cycle %lu: %s %.1fms "
                "used=%zuKB cap=%zuKB\n",
                static_cast<unsigned long>(gcCycles_),
                full ? "full" : "minor",
                elapsed,
                heapUsed_.load() / 1024,
                heapCapacity_ / 1024);
        }
    }

    // -------------------------------------------------------------------------
    // Object tracking (for GC tracing via Object::visitRefs)
    // -------------------------------------------------------------------------

    using ObjectVisitor = std::function<void(uintptr_t addr)>;
    using RefTracer = std::function<void(uintptr_t addr,
                        std::function<void(uintptr_t ref)>)>;

    void setRefTracer(RefTracer tracer) { refTracer_ = std::move(tracer); }

    /**
     * @brief Mark an object as live (called during tracing)
     */
    void markObject(uintptr_t addr) {
        std::lock_guard<std::mutex> lock(trackingMutex_);
        auto it = liveObjects_.find(addr);
        if (it != liveObjects_.end()) {
            it->second.marked = true;
        }
    }

    // -------------------------------------------------------------------------
    // Stats
    // -------------------------------------------------------------------------

    Stats computeStats() const {
        Stats s = stats_;
        s.currentHeapUsed = heapUsed_.load();
        s.currentHeapCapacity = heapCapacity_;
        s.rootCount = rootSet_.rootCount();
        s.handleCount = rootSet_.handleCount();
        s.totalGCCycles = gcCycles_;
        return s;
    }

    size_t heapUsed() const { return heapUsed_.load(); }
    size_t heapCapacity() const { return heapCapacity_; }
    uint64_t gcCycleCount() const { return gcCycles_; }

private:
    bool shouldTriggerGC() const {
        size_t used = heapUsed_.load(std::memory_order_relaxed);
        return used > heapCapacity_ * 3 / 4;
    }

    void doMinorGC() {
        // Mark phase: trace from roots
        size_t marked = 0;
        rootSet_.enumerateHandles([&](uintptr_t addr) {
            markRecursive(addr, marked);
        });

        // Sweep: reclaim unmarked objects
        size_t reclaimed = sweepDead();
        stats_.totalBytesReclaimed += reclaimed;
    }

    void doFullGC() {
        // Enable SATB for concurrent correctness
        barrierSink_.enableSATB();

        // Mark from roots
        size_t marked = 0;
        rootSet_.enumerate([&](void** slot) {
            if (slot && *slot) {
                auto addr = reinterpret_cast<uintptr_t>(*slot);
                markRecursive(addr, marked);
            }
        });

        rootSet_.enumerateHandles([&](uintptr_t addr) {
            markRecursive(addr, marked);
        });

        // Drain SATB
        barrierSink_.drainSATB([&](uintptr_t addr) {
            markRecursive(addr, marked);
        });

        barrierSink_.disableSATB();

        // Sweep
        size_t reclaimed = sweepDead();
        stats_.totalBytesReclaimed += reclaimed;
    }

    void markRecursive(uintptr_t addr, size_t& marked) {
        std::lock_guard<std::mutex> lock(trackingMutex_);
        auto it = liveObjects_.find(addr);
        if (it == liveObjects_.end() || it->second.marked) return;

        it->second.marked = true;
        marked++;

        // Trace children via registered tracer
        if (refTracer_) {
            auto refs = collectRefs(addr);
            for (auto ref : refs) {
                auto it2 = liveObjects_.find(ref);
                if (it2 != liveObjects_.end() && !it2->second.marked) {
                    it2->second.marked = true;
                    marked++;
                }
            }
        }
    }

    std::vector<uintptr_t> collectRefs(uintptr_t addr) {
        std::vector<uintptr_t> refs;
        if (refTracer_) {
            refTracer_(addr, [&](uintptr_t ref) {
                refs.push_back(ref);
            });
        }
        return refs;
    }

    size_t sweepDead() {
        std::lock_guard<std::mutex> lock(trackingMutex_);
        size_t reclaimed = 0;

        std::vector<uintptr_t> dead;
        for (auto& [addr, info] : liveObjects_) {
            if (!info.marked) {
                dead.push_back(addr);
                reclaimed += info.size;
            }
        }

        for (auto addr : dead) {
            liveObjects_.erase(addr);
            std::free(reinterpret_cast<void*>(addr));
        }

        // Reset marks for next cycle
        for (auto& [addr, info] : liveObjects_) {
            info.marked = false;
        }

        heapUsed_.fetch_sub(reclaimed, std::memory_order_relaxed);
        return reclaimed;
    }

    struct ObjectInfo {
        size_t size;
        uint8_t type;
        bool marked = false;
    };

    Config config_;
    Stats stats_{};
    RootSet rootSet_;
    WriteBarrierSink barrierSink_;
    RefTracer refTracer_;

    std::mutex trackingMutex_;
    std::unordered_map<uintptr_t, ObjectInfo> liveObjects_;

    std::atomic<size_t> bytesAllocated_;
    std::atomic<size_t> heapUsed_;
    size_t heapCapacity_ = 0;
    uint64_t gcCycles_ = 0;
    bool initialized_;
};

} // namespace Zepra::Heap
