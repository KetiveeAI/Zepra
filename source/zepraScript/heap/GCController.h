/**
 * @file GCController.h
 * @brief Unified GC controller coordinating all GC components
 * 
 * Implements:
 * - GC scheduling and triggers
 * - Coordination between nursery/old-gen
 * - Concurrent/incremental GC management
 * 
 * Based on V8/JSC GC schedulers
 */

#pragma once

#include "Nursery.h"
#include "OldGeneration.h"
#include "WriteBarrier.h"
#include "concurrent_gc.hpp"
#include <chrono>
#include <atomic>

namespace Zepra::GC {

enum class GCTrigger : uint8_t {
    Allocation,
    Timer,
    Memory,
    Explicit,
    Emergency
};

enum class GCPhase : uint8_t {
    Idle,
    MinorGC,
    MajorGCMarking,
    MajorGCSweeping,
    Compacting
};

struct GCControllerStats {
    size_t minorGCs = 0;
    size_t majorGCs = 0;
    size_t totalPauseMs = 0;
    size_t maxPauseMs = 0;
    size_t totalAllocated = 0;
    size_t totalReclaimed = 0;
    size_t heapSize = 0;
    size_t liveSize = 0;
};

struct GCSchedule {
    size_t nurseryThreshold = 3 * 1024 * 1024;
    double heapGrowthFactor = 2.0;
    size_t minHeapForMajor = 8 * 1024 * 1024;
    bool enableConcurrent = true;
    unsigned concurrentWorkers = 2;
    double fragmentationThreshold = 0.3;
    size_t targetPauseMs = 10;
    size_t maxPauseMs = 50;
};

class GCController {
public:
    GCController() = default;
    ~GCController() = default;
    
    bool init(size_t nurserySize = Nursery::DEFAULT_SIZE) {
        if (!nursery_.init(nurserySize)) return false;
        if (!oldGen_.init()) return false;
        barriers_.init(nursery_.allocPtrAddress(), nurserySize);
        if (schedule_.enableConcurrent) {
            concurrentGC_.init(nullptr);
            concurrentGC_.setWorkerCount(schedule_.concurrentWorkers);
        }
        phase_.store(GCPhase::Idle, std::memory_order_release);
        return true;
    }
    
    template<typename T, typename... Args>
    T* allocate(Args&&... args) {
        size_t size = sizeof(Runtime::ObjectHeader) + sizeof(T);
        
        void* mem = nursery_.allocate(size);
        if (!mem) { collectMinor(); mem = nursery_.allocate(size); }
        if (!mem) { mem = oldGen_.allocate(size); }
        if (!mem) { collectMajor(GCTrigger::Emergency); mem = oldGen_.allocate(size); }
        if (!mem) return nullptr;
        
        auto* header = new (mem) Runtime::ObjectHeader();
        header->size = sizeof(T);
        header->generation = Runtime::Generation::Young;
        
        T* obj = new (header->object()) T(std::forward<Args>(args)...);
        stats_.totalAllocated += size;
        return obj;
    }
    
    void collectMinor();
    void collectMajor(GCTrigger trigger = GCTrigger::Allocation);
    
    void maybeGC() {
        if (nursery_.needsScavenge()) collectMinor();
        if (shouldTriggerMajorGC()) collectMajor();
    }
    
    void writeBarrier(Runtime::Object* source, Runtime::Object** slot, Runtime::Object* target) {
        if (concurrentGC_.isMarking()) {
            Runtime::Object* old = *slot;
            if (old) concurrentGC_.satbWriteBarrier(old);
        }
        auto* srcHeader = Runtime::ObjectHeader::fromObject(source);
        if (srcHeader->generation == Runtime::Generation::Old && target) {
            auto* tgtHeader = Runtime::ObjectHeader::fromObject(target);
            if (tgtHeader->generation == Runtime::Generation::Young) {
                barriers_.barrierSlow(source, slot, target);
            }
        }
        *slot = target;
    }
    
    Nursery& nursery() { return nursery_; }
    OldGeneration& oldGen() { return oldGen_; }
    WriteBarrierManager& barriers() { return barriers_; }
    GCPhase phase() const { return phase_.load(std::memory_order_acquire); }
    const GCControllerStats& stats() const { return stats_; }
    GCSchedule& schedule() { return schedule_; }
    
private:
    Nursery nursery_;
    OldGeneration oldGen_;
    WriteBarrierManager barriers_;
    ConcurrentGC concurrentGC_;
    std::unique_ptr<Scavenger> scavenger_;
    
    std::atomic<GCPhase> phase_{GCPhase::Idle};
    GCControllerStats stats_;
    GCSchedule schedule_;
    size_t lastLiveSize_ = 0;
    
    void markAll();
    bool shouldCompact() const;
    void compact();
    
    bool shouldTriggerMajorGC() {
        size_t heapSize = stats_.heapSize;
        size_t threshold = static_cast<size_t>(lastLiveSize_ * schedule_.heapGrowthFactor);
        return heapSize >= threshold && heapSize >= schedule_.minHeapForMajor;
    }
    
    void startConcurrentMajorGC() {
        if (!concurrentGC_.startCycle()) return;
        phase_.store(GCPhase::MajorGCMarking, std::memory_order_release);
    }
};

class GCSafeScope {
public:
    explicit GCSafeScope(GCController& gc) : gc_(gc) {}
    ~GCSafeScope() {}
private:
    GCController& gc_;
};

} // namespace Zepra::GC
