/**
 * @file concurrent_gc.cpp
 * @brief Concurrent garbage collector implementation
 */

#include "heap/concurrent_gc.hpp"
#include "runtime/objects/object.hpp"
#include <chrono>

namespace Zepra::GC {

using namespace std::chrono;

// =============================================================================
// Constructor/Destructor
// =============================================================================

ConcurrentGC::ConcurrentGC() = default;

ConcurrentGC::~ConcurrentGC() {
    stopping_.store(true, std::memory_order_release);
    workQueue_.stop();
    
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ConcurrentGC::init(Runtime::GCHeap* heap) {
    heap_ = heap;
}

// =============================================================================
// Cycle Management
// =============================================================================

bool ConcurrentGC::startCycle() {
    bool expected = false;
    if (!marking_.compare_exchange_strong(expected, true, 
                                          std::memory_order_acq_rel)) {
        return false;  // Already marking
    }
    
    stats_.concurrentCycles++;
    auto startTime = high_resolution_clock::now();
    
    // Phase 1: Stop-the-world root snapshot
    processRoots();
    
    auto pauseEnd = high_resolution_clock::now();
    stats_.pauseTime += duration_cast<microseconds>(pauseEnd - startTime).count();
    
    // Phase 2: Start concurrent marking
    workQueue_.reset();
    
    // Push root objects to work queue
    for (Object* root : rootSnapshot_) {
        markGray(root);
    }
    
    // Start worker threads if needed
    if (workers_.empty()) {
        for (unsigned i = 0; i < workerCount_; i++) {
            workers_.emplace_back(&ConcurrentGC::workerLoop, this);
        }
    }
    
    return true;
}

void ConcurrentGC::waitForCycle() {
    // Drain work queue
    drainWorkQueue();
    
    // Final mark (handle SATB buffer)
    finalMark();
    
    // Mark cycle complete
    marking_.store(false, std::memory_order_release);
    
    auto endTime = high_resolution_clock::now();
    // Note: marking time would need start time tracked
}

// =============================================================================
// Write Barrier
// =============================================================================

void ConcurrentGC::satbWriteBarrier(Object* oldValue) {
    if (!oldValue) return;
    if (!marking_.load(std::memory_order_acquire)) return;
    
    std::lock_guard<std::mutex> lock(satbMutex_);
    satbBuffer_.push_back(oldValue);
}

// =============================================================================
// Marking
// =============================================================================

void ConcurrentGC::markGray(Object* obj) {
    if (!obj) return;
    
    MarkColor currentColor = getColor(obj);
    if (currentColor != MarkColor::White) return;
    
    setColor(obj, MarkColor::Gray);
    workQueue_.push(obj);
}

MarkColor ConcurrentGC::getColor(Object* obj) const {
    std::lock_guard<std::mutex> lock(colorMutex_);
    auto it = colors_.find(obj);
    return (it != colors_.end()) ? it->second : MarkColor::White;
}

void ConcurrentGC::setColor(Object* obj, MarkColor color) {
    std::lock_guard<std::mutex> lock(colorMutex_);
    colors_[obj] = color;
}

void ConcurrentGC::markObject(Object* obj) {
    if (!obj) return;
    
    MarkColor color = getColor(obj);
    if (color == MarkColor::Black) return;
    
    // Mark gray and trace
    setColor(obj, MarkColor::Gray);
    traceObject(obj);
    setColor(obj, MarkColor::Black);
    
    stats_.objectsMarked++;
    
    Runtime::ObjectHeader* header = Runtime::ObjectHeader::fromObject(obj);
    stats_.bytesMarked += header->size;
}

void ConcurrentGC::traceObject(Object* obj) {
    if (!obj || !heap_) return;
    
    // Use heap's tracers to find references
    // This is a simplified version - production would iterate object fields
}

// =============================================================================
// Worker Thread
// =============================================================================

void ConcurrentGC::workerLoop() {
    while (!stopping_.load(std::memory_order_acquire)) {
        Object* obj = workQueue_.waitAndPop();
        if (!obj) {
            if (stopping_.load(std::memory_order_acquire)) break;
            continue;
        }
        
        markObject(obj);
    }
}

void ConcurrentGC::drainWorkQueue() {
    while (!workQueue_.empty()) {
        Object* obj = workQueue_.pop();
        if (obj) {
            markObject(obj);
        }
    }
}

// =============================================================================
// Root Processing
// =============================================================================

void ConcurrentGC::processRoots() {
    rootSnapshot_.clear();
    
    // Would iterate heap's root set
    // For now, simplified
}

void ConcurrentGC::processSatbBuffer() {
    std::vector<Object*> buffer;
    {
        std::lock_guard<std::mutex> lock(satbMutex_);
        buffer.swap(satbBuffer_);
    }
    
    for (Object* obj : buffer) {
        markGray(obj);
    }
}

void ConcurrentGC::finalMark() {
    // Stop-the-world for final marking
    processSatbBuffer();
    drainWorkQueue();
    
    // Process SATB buffer again in case it accumulated during drain
    processSatbBuffer();
    drainWorkQueue();
}

// =============================================================================
// Configuration
// =============================================================================

void ConcurrentGC::setWorkerCount(unsigned count) {
    workerCount_ = count > 0 ? count : 1;
}

} // namespace Zepra::GC
