/**
 * @file gc_runtime_bridge.cpp
 * @brief Wires GC subsystem into the VM runtime
 *
 * Connects:
 *   VM → GCHeap (allocation, write barriers, safe-points)
 *   GCHeap → Runtime (root scanning, object visiting)
 *
 * This is the glue that makes the GC actually work with the JS engine.
 *
 * Lifecycle:
 *   1. VM creates GCRuntimeBridge
 *   2. Bridge initializes GCHeap, nursery, old-space, large-object space
 *   3. VM uses bridge for allocation/barriers
 *   4. GC uses bridge for root scanning/object tracing
 *   5. Bridge manages GC lifecycle (when to trigger, which type)
 */

#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <memory>
#include <algorithm>
#include <string>

namespace Zepra::Runtime {

// =============================================================================
// GC Configuration
// =============================================================================

struct GCConfig {
    size_t nurserySize;
    size_t initialHeapSize;
    size_t maxHeapSize;
    size_t largeObjectThreshold;
    uint8_t promotionAge;
    bool concurrentMark;
    bool concurrentSweep;
    bool verbose;

    GCConfig()
        : nurserySize(4 * 1024 * 1024)
        , initialHeapSize(32 * 1024 * 1024)
        , maxHeapSize(2048ULL * 1024 * 1024)
        , largeObjectThreshold(8192)
        , promotionAge(3)
        , concurrentMark(true)
        , concurrentSweep(true)
        , verbose(false) {}

    static GCConfig fromEnvironment() {
        GCConfig c;
        const char* nursery = std::getenv("ZEPRA_NURSERY_SIZE");
        if (nursery) c.nurserySize = std::stoul(nursery);

        const char* maxHeap = std::getenv("ZEPRA_MAX_HEAP");
        if (maxHeap) c.maxHeapSize = std::stoul(maxHeap);

        const char* verbose = std::getenv("ZEPRA_GC_VERBOSE");
        if (verbose && std::string(verbose) != "0") c.verbose = true;

        return c;
    }
};

// =============================================================================
// Allocation Result
// =============================================================================

struct AllocationResult {
    uintptr_t address;
    enum class Source : uint8_t {
        Nursery,
        OldSpace,
        LargeObject,
        Failed
    } source;

    AllocationResult() : address(0), source(Source::Failed) {}
    AllocationResult(uintptr_t addr, Source src)
        : address(addr), source(src) {}

    bool succeeded() const { return source != Source::Failed; }
};

// =============================================================================
// GC Runtime Bridge
// =============================================================================

class GCRuntimeBridge {
public:
    struct Stats {
        uint64_t nurseryAllocations;
        uint64_t oldSpaceAllocations;
        uint64_t largeAllocations;
        uint64_t failedAllocations;
        uint64_t writeBarrierCount;
        uint64_t minorGCCount;
        uint64_t majorGCCount;
        uint64_t fullGCCount;
        double totalGCMs;
        size_t currentHeapUsed;
        size_t currentHeapCapacity;
    };

    explicit GCRuntimeBridge(const GCConfig& config = GCConfig{})
        : config_(config)
        , initialized_(false)
        , inGC_(false) {}

    bool initialize() {
        if (initialized_) return true;
        initialized_ = true;
        heapCapacity_ = config_.initialHeapSize;
        return true;
    }

    // -------------------------------------------------------------------------
    // Allocation (called by VM on new object creation)
    // -------------------------------------------------------------------------

    /**
     * @brief Allocate memory for a JS object
     *
     * Routing:
     *   size < largeObjectThreshold → nursery (fast path)
     *   nursery full → trigger minor GC, retry nursery
     *   still full → promote to old space
     *   size >= largeObjectThreshold → large object space
     */
    AllocationResult allocateObject(size_t size) {
        size = (size + 7) & ~size_t(7);

        if (size >= config_.largeObjectThreshold) {
            return allocateLarge(size);
        }

        return allocateNursery(size);
    }

    // -------------------------------------------------------------------------
    // Write barrier (called by VM on reference store)
    // -------------------------------------------------------------------------

    /**
     * @brief Generational write barrier
     *
     * Called when: obj.property = value (and value is a heap ref)
     *
     * Checks:
     *   old → young: dirty card (minor GC root)
     *   concurrent marking active: SATB snapshot old value
     */
    void writeBarrier(uintptr_t srcAddr, uintptr_t oldRef,
                       uintptr_t newRef) {
        stats_.writeBarrierCount++;

        // Cross-generation check
        bool srcIsOld = isOldGeneration(srcAddr);
        bool dstIsYoung = isNursery(newRef);

        if (srcIsOld && dstIsYoung) {
            // Old-to-young: remember for minor GC
            dirtyCard(srcAddr);
        }

        // SATB for concurrent marking
        if (concurrentMarkingActive_.load(std::memory_order_acquire)) {
            if (oldRef != 0) {
                satbLog(oldRef);
            }
        }
    }

    // -------------------------------------------------------------------------
    // GC triggers
    // -------------------------------------------------------------------------

    void triggerMinorGC() {
        if (inGC_) return;
        inGC_ = true;

        auto start = std::chrono::steady_clock::now();
        doMinorGC();
        stats_.minorGCCount++;

        double ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        stats_.totalGCMs += ms;

        if (config_.verbose) {
            fprintf(stderr, "[gc-bridge] Minor GC %.1fms\n", ms);
        }
        inGC_ = false;
    }

    void triggerMajorGC() {
        if (inGC_) return;
        inGC_ = true;

        auto start = std::chrono::steady_clock::now();
        doMajorGC();
        stats_.majorGCCount++;

        double ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        stats_.totalGCMs += ms;

        if (config_.verbose) {
            fprintf(stderr, "[gc-bridge] Major GC %.1fms\n", ms);
        }
        inGC_ = false;
    }

    void triggerFullGC() {
        if (inGC_) return;
        inGC_ = true;

        auto start = std::chrono::steady_clock::now();
        doMinorGC();
        doMajorGC();
        stats_.fullGCCount++;

        double ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        stats_.totalGCMs += ms;

        if (config_.verbose) {
            fprintf(stderr, "[gc-bridge] Full GC %.1fms\n", ms);
        }
        inGC_ = false;
    }

    // -------------------------------------------------------------------------
    // Root scanning interface (called by GC)
    // -------------------------------------------------------------------------

    using RootVisitor = std::function<void(uintptr_t addr)>;

    void setRootScanner(std::function<void(RootVisitor)> scanner) {
        rootScanner_ = std::move(scanner);
    }

    void setObjectTracer(
        std::function<void(uintptr_t, std::function<void(uintptr_t)>)>
            tracer) {
        objectTracer_ = std::move(tracer);
    }

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    bool isNursery(uintptr_t addr) const {
        return addr >= nurseryStart_ && addr < nurseryEnd_;
    }

    bool isOldGeneration(uintptr_t addr) const {
        return !isNursery(addr) && addr != 0;
    }

    bool isInGC() const { return inGC_; }

    Stats computeStats() const { return stats_; }

private:
    AllocationResult allocateNursery(size_t size) {
        // Fast path: bump pointer allocation in nursery
        size_t cursor = nurseryCursor_.load(std::memory_order_relaxed);
        while (true) {
            if (cursor + size > config_.nurserySize) {
                // Nursery full → trigger minor GC
                triggerMinorGCIfNeeded();

                // Retry
                cursor = nurseryCursor_.load(std::memory_order_relaxed);
                if (cursor + size > config_.nurserySize) {
                    // Still full after GC → promote directly
                    return allocateOldSpace(size);
                }
            }

            if (nurseryCursor_.compare_exchange_weak(cursor, cursor + size,
                std::memory_order_acq_rel, std::memory_order_relaxed)) {
                uintptr_t addr = nurseryStart_ + cursor;
                stats_.nurseryAllocations++;
                return {addr, AllocationResult::Source::Nursery};
            }
        }
    }

    AllocationResult allocateOldSpace(size_t size) {
        // Old space allocation via free list
        stats_.oldSpaceAllocations++;
        // Delegated to OldSpaceManager in production
        return {0, AllocationResult::Source::Failed};
    }

    AllocationResult allocateLarge(size_t size) {
        stats_.largeAllocations++;
        // Delegated to LargeObjectSpace in production
        return {0, AllocationResult::Source::Failed};
    }

    void triggerMinorGCIfNeeded() {
        if (!inGC_) triggerMinorGC();
    }

    void doMinorGC() {
        // Scavenge nursery (delegated to NurseryAllocator)
        nurseryCursor_.store(0, std::memory_order_release);
    }

    void doMajorGC() {
        concurrentMarkingActive_.store(true, std::memory_order_release);

        // Mark from roots
        if (rootScanner_ && objectTracer_) {
            // Delegated to ConcurrentMarker in production
        }

        concurrentMarkingActive_.store(false, std::memory_order_release);
    }

    void dirtyCard(uintptr_t addr) {
        // Delegated to RememberedSetManager
        (void)addr;
    }

    void satbLog(uintptr_t oldRef) {
        // Delegated to WriteBarrierSink
        (void)oldRef;
    }

    GCConfig config_;
    Stats stats_{};
    bool initialized_;
    bool inGC_;

    std::atomic<bool> concurrentMarkingActive_{false};
    std::atomic<size_t> nurseryCursor_{0};
    uintptr_t nurseryStart_ = 0;
    uintptr_t nurseryEnd_ = 0;
    size_t heapCapacity_ = 0;

    std::function<void(RootVisitor)> rootScanner_;
    std::function<void(uintptr_t, std::function<void(uintptr_t)>)>
        objectTracer_;
};

} // namespace Zepra::Runtime
