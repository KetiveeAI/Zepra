/**
 * @file incremental_gc.cpp
 * @brief Incremental garbage collection implementation
 */

#include "zeprascript/gc/incremental_gc.hpp"
#include "zeprascript/runtime/object.hpp"
#include <algorithm>

namespace Zepra::GC {

IncrementalGC::IncrementalGC(Heap* heap) : heap_(heap) {}

void IncrementalGC::startCollection() {
    if (phase_ != GCPhase::Idle) return;
    
    phase_ = GCPhase::Marking;
    markIndex_ = 0;
    sweepIndex_ = 0;
    markQueue_.clear();
    
    // TODO: Get root objects when Heap::visitRoots is implemented
    // For now, collection starts with empty queue and completes immediately
}

bool IncrementalGC::step() {
    stepStart_ = std::chrono::steady_clock::now();
    
    bool complete = false;
    
    switch (phase_) {
        case GCPhase::Idle:
            return true;
            
        case GCPhase::Marking:
            complete = markStep();
            if (complete) {
                phase_ = GCPhase::Sweeping;
                sweepIndex_ = 0;
            }
            break;
            
        case GCPhase::Sweeping:
            complete = sweepStep();
            if (complete) {
                finishCollection();
            }
            break;
            
        case GCPhase::Complete:
            return true;
    }
    
    auto elapsed = std::chrono::steady_clock::now() - stepStart_;
    totalPauseMs_ += std::chrono::duration<double, std::milli>(elapsed).count();
    
    return phase_ == GCPhase::Idle;
}

bool IncrementalGC::markStep() {
    size_t processed = 0;
    
    while (markIndex_ < markQueue_.size() && 
           processed < config_.markBatchSize) {
        Runtime::Object* obj = markQueue_[markIndex_++];
        
        if (!obj || obj->isMarked()) continue;
        
        obj->markGC();
        processed++;
        
        // TODO: Traverse object references when Object::visitRefs is added
        // For now, just mark the direct objects
        
        // Check time limit
        auto elapsed = std::chrono::steady_clock::now() - stepStart_;
        if (std::chrono::duration<double, std::milli>(elapsed).count() 
            >= config_.maxIncrementMs) {
            break;
        }
    }
    
    return markIndex_ >= markQueue_.size();
}

bool IncrementalGC::sweepStep() {
    // TODO: Implement incremental sweep when Heap exposes public method
    // For now, sweep is handled by Heap::collect()
    return true;
}

void IncrementalGC::finishCollection() {
    phase_ = GCPhase::Idle;
    markQueue_.clear();
    markQueue_.shrink_to_fit();
}

// =============================================================================
// Write Barrier
// =============================================================================

void WriteBarrier::onFieldWrite(Runtime::Object* holder, Runtime::Object* newValue) {
    // If holder is black and newValue is white, we need to re-gray holder
    // This maintains the tri-color invariant
    if (holder && holder->isMarked() && 
        newValue && !newValue->isMarked()) {
        // Re-add to mark queue or mark directly
        newValue->markGC();
    }
}

void WriteBarrier::onArrayWrite(Runtime::Object* array, size_t, Runtime::Object* newValue) {
    onFieldWrite(array, newValue);
}

} // namespace Zepra::GC
