/**
 * @file generational_gc.cpp
 * @brief Generational Garbage Collector Orchestrator
 *
 * This file implements the main Generational GC manager for ZepraScript.
 * It coordinates the Nursery, Old Generation, Write Barriers, and Compaction.
 */

#include "heap/GCController.h"
#include "heap/Nursery.h"
#include "heap/OldGeneration.h"
#include "heap/WriteBarrier.h"
#include "heap/Compaction.h"
#include "runtime/objects/object.hpp"
#include <vector>
#include <chrono>

namespace Zepra::GC {

// =============================================================================
// Nursery Implementation
// =============================================================================

Runtime::Object* Scavenger::evacuateOrPromote(Runtime::Object* obj, OldGeneration& oldGen) {
    auto* header = Runtime::ObjectHeader::fromObject(obj);
    
    if (header->marked) {
        return reinterpret_cast<Runtime::Object*>(header->object());
    }
    
    size_t size = sizeof(Runtime::ObjectHeader) + header->size;
    
    header->age++;
    bool promote = header->age >= 4; // PROMOTION_AGE
    
    void* dest;
    if (promote) {
        dest = oldGen.allocate(size);
        nursery_.stats_.promotedObjects++;
        nursery_.stats_.promotedBytes += size;
    } else {
        dest = oldGen.allocate(size); 
    }
    
    if (!dest) return obj;  // OOM
    
    std::memcpy(dest, header, size);
    
    auto* newHeader = static_cast<Runtime::ObjectHeader*>(dest);
    if (promote) {
        newHeader->generation = Runtime::Generation::Old;
    }
    
    header->marked = true;
    new (header->object()) void*(newHeader->object());
    
    workQueue_.push_back(static_cast<Runtime::Object*>(newHeader->object()));
    
    nursery_.stats_.survivorBytes += size;
    
    return static_cast<Runtime::Object*>(newHeader->object());
}

void Scavenger::scanRegion(void* start, void* end, OldGeneration& oldGen) {
    char* ptr = static_cast<char*>(start);
    char* endPtr = static_cast<char*>(end);
    
    while (ptr < endPtr) {
        auto* header = reinterpret_cast<Runtime::ObjectHeader*>(ptr);
        if (header->size == 0) break;
        
        Runtime::Object* obj = static_cast<Runtime::Object*>(header->object());
        
        obj->visitRefs([&](Runtime::Object* ref) {
            if (ref && nursery_.contains(ref)) {
                evacuateOrPromote(ref, oldGen);
            }
        });
        
        ptr += sizeof(Runtime::ObjectHeader) + header->size;
    }
}

void Scavenger::drainWorkQueue(OldGeneration& oldGen) {
    while (!workQueue_.empty()) {
        Runtime::Object* obj = workQueue_.back();
        workQueue_.pop_back();
        
        obj->visitRefs([&](Runtime::Object* ref) {
            if (ref && nursery_.contains(ref)) {
                evacuateOrPromote(ref, oldGen);
            }
        });
    }
}

void Scavenger::scavenge(const std::vector<Runtime::Object**>& roots, OldGeneration& oldGen, WriteBarrierManager& barriers) {
    for (auto* root : roots) {
        Runtime::Object* obj = *root;
        if (obj && nursery_.contains(obj)) {
            *root = evacuateOrPromote(obj, oldGen);
        }
    }
    
    barriers.processDirtyCards([&](void* start, void* end) {
        scanRegion(start, end, oldGen);
    });
    
    drainWorkQueue(oldGen);
    
    nursery_.reset();
    barriers.clearAfterGC();
    
    nursery_.stats_.scavenges++;
}

// =============================================================================
// GC Controller Implementation
// =============================================================================

void GCController::collectMinor() {
    if (phase_.load() != GCPhase::Idle) return;
    
    phase_.store(GCPhase::MinorGC, std::memory_order_release);
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<Runtime::Object**> roots; 
    
    scavenger_ = std::make_unique<Scavenger>(nursery_);
    scavenger_->scavenge(roots, oldGen_, barriers_);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto pauseMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    stats_.minorGCs++;
    stats_.totalPauseMs += pauseMs;
    stats_.maxPauseMs = std::max(stats_.maxPauseMs, static_cast<size_t>(pauseMs));
    
    phase_.store(GCPhase::Idle, std::memory_order_release);
}

void GCController::collectMajor(GCTrigger trigger) {
    if (schedule_.enableConcurrent && trigger != GCTrigger::Emergency) {
        startConcurrentMajorGC();
        return;
    }
    
    phase_.store(GCPhase::MajorGCMarking, std::memory_order_release);
    auto start = std::chrono::high_resolution_clock::now();
    
    markAll();
    
    phase_.store(GCPhase::MajorGCSweeping, std::memory_order_release);
    oldGen_.sweep(nullptr);
    
    if (shouldCompact()) {
        phase_.store(GCPhase::Compacting, std::memory_order_release);
        compact();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto pauseMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    stats_.majorGCs++;
    stats_.totalPauseMs += pauseMs;
    stats_.maxPauseMs = std::max(stats_.maxPauseMs, static_cast<size_t>(pauseMs));
    
    phase_.store(GCPhase::Idle, std::memory_order_release);
}

void GCController::markAll() {
    std::vector<Runtime::Object*> workQueue;
    std::vector<Runtime::Object*> roots; 
    
    for(auto* root : roots) {
        Runtime::ObjectHeader* hdr = Runtime::ObjectHeader::fromObject(root);
        if (!hdr->marked) {
            hdr->marked = true;
            workQueue.push_back(root);
        }
    }
    
    while(!workQueue.empty()) {
        Runtime::Object* current = workQueue.back();
        workQueue.pop_back();
        current->visitRefs([&](Runtime::Object* ref) {
            if (ref) {
                Runtime::ObjectHeader* hdr = Runtime::ObjectHeader::fromObject(ref);
                if (!hdr->marked) {
                    hdr->marked = true;
                    workQueue.push_back(ref);
                }
            }
        });
    }
}

bool GCController::shouldCompact() const {
    return (oldGen_.stats().fragmentationBytes > (stats_.heapSize * schedule_.fragmentationThreshold));
}

void GCController::compact() {
    oldGen_.compact(nullptr, [](void* from, void* to) {
    });
}

} // namespace Zepra::GC
