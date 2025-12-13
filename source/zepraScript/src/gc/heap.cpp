/**
 * @file heap.cpp
 * @brief Mark-sweep garbage collector implementation
 */

#include "zeprascript/gc/heap.hpp"
#include "zeprascript/runtime/object.hpp"
#include <cstdlib>
#include <algorithm>
#include <chrono>

namespace Zepra::GC {

using Runtime::Object;

Heap::Heap(size_t initialSize, size_t maxSize)
    : initialSize_(initialSize), maxSize_(maxSize)
    , nextGCThreshold_(initialSize * ZEPRA_GC_THRESHOLD_RATIO) {}

Heap::~Heap() {
    // Free all remaining objects using free() since we used malloc()
    // Note: This is raw memory, not constructed Objects
    for (void* obj : objects_) {
        free(obj);
    }
    objects_.clear();
}

void* Heap::allocate(size_t size) {
    // Check if GC is needed
    if (shouldCollect()) {
        collectGarbage();
    }
    
    // Allocate new memory
    void* mem = malloc(size);
    if (mem) {
        objects_.push_back(mem);
        bytesAllocated_ += size;
        stats_.objectCount++;
    }
    return mem;
}

void Heap::collectGarbage(bool) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Phase 1: Mark all reachable objects
    mark();
    
    // Phase 2: Sweep unmarked objects
    sweep();
    
    // Update stats
    auto end = std::chrono::high_resolution_clock::now();
    stats_.lastGcTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
    stats_.gcCount++;
    
    // Adjust threshold based on surviving objects
    nextGCThreshold_ = bytesAllocated_ * 2;
    if (nextGCThreshold_ > maxSize_) {
        nextGCThreshold_ = maxSize_;
    }
}

void Heap::addRoot(void* ptr) { 
    roots_.push_back(ptr); 
}

void Heap::removeRoot(void* ptr) {
    roots_.erase(std::remove(roots_.begin(), roots_.end(), ptr), roots_.end());
}

bool Heap::shouldCollect() const {
    return bytesAllocated_ > nextGCThreshold_;
}

void Heap::mark() {
    // Clear all marks first
    for (void* obj : objects_) {
        Object* runtimeObj = static_cast<Object*>(obj);
        runtimeObj->clearMark();
    }
    
    // Mark from roots
    for (void* root : roots_) {
        markObject(root);
    }
}

void Heap::markObject(void* ptr) {
    if (!ptr) return;
    
    Object* obj = static_cast<Object*>(ptr);
    
    // Already marked - avoid cycles
    if (obj->isMarked()) return;
    
    // Mark this object
    obj->markGC();
    
    // Recursively mark prototype
    if (obj->prototype()) {
        markObject(obj->prototype());
    }
    
    // Mark properties (for objects with property storage)
    // The Object class handles its own property marking in markGC()
}

void Heap::sweep() {
    size_t freedCount = 0;
    size_t freedBytes = 0;
    
    // Partition: move dead objects to end
    auto newEnd = std::remove_if(objects_.begin(), objects_.end(),
        [&](void* ptr) {
            Object* obj = static_cast<Object*>(ptr);
            if (!obj->isMarked()) {
                // Dead object - free it
                freedCount++;
                freedBytes += sizeof(Object); // Approximate
                delete obj;
                return true; // Remove from vector
            }
            return false; // Keep alive
        });
    
    // Erase dead entries
    objects_.erase(newEnd, objects_.end());
    
    // Update stats
    stats_.objectCount -= freedCount;
    bytesAllocated_ -= freedBytes;
    stats_.usedHeapSize = bytesAllocated_;
}

} // namespace Zepra::GC

