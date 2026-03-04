/**
 * @file Compaction.h
 * @brief Memory compaction for GC
 * 
 * Implements:
 * - Mark-compact algorithm
 * - Forwarding pointer management
 * - Live object sliding
 */

#pragma once

#include "gc_heap.hpp"
#include <vector>
#include <unordered_map>

namespace Zepra::GC {

/**
 * @brief Forwarding table for object relocation
 */
class ForwardingTable {
public:
    void addForwarding(void* from, void* to) {
        forwards_[from] = to;
    }
    
    void* getForwarded(void* obj) const {
        auto it = forwards_.find(obj);
        return it != forwards_.end() ? it->second : obj;
    }
    
    bool hasForwarding(void* obj) const {
        return forwards_.count(obj) > 0;
    }
    
    void clear() { forwards_.clear(); }
    size_t size() const { return forwards_.size(); }
    
    template<typename Fn>
    void forEach(Fn&& fn) const {
        for (const auto& [from, to] : forwards_) {
            fn(from, to);
        }
    }
    
private:
    std::unordered_map<void*, void*> forwards_;
};

/**
 * @brief Compactor statistics
 */
struct CompactionStats {
    size_t liveBytes = 0;
    size_t movedBytes = 0;
    size_t objectsMoved = 0;
    size_t ptrUpdates = 0;
    size_t fragmentationBefore = 0;
    size_t fragmentationAfter = 0;
};

/**
 * @brief Memory compactor for old generation
 * 
 * Uses mark-compact with sliding compaction.
 * Objects are moved to start of heap, eliminating fragmentation.
 */
class Compactor {
public:
    Compactor() = default;
    
    /**
     * @brief Compact a memory region
     * @param start Heap region start
     * @param end Heap region end
     * @param firstObject First object header in region
     * @param updatePointer Callback to update external pointers
     */
    template<typename UpdateFn>
    void compact(void* start, void* end, 
                 Runtime::ObjectHeader* firstObject,
                 UpdateFn&& updatePointer) {
        stats_ = {};
        forwards_.clear();
        
        // Phase 1: Calculate forwarding addresses
        computeForwardingAddresses(start, firstObject);
        
        // Phase 2: Update all pointers to use forwarding
        updatePointers(firstObject, std::forward<UpdateFn>(updatePointer));
        
        // Phase 3: Move objects to new locations
        moveObjects(firstObject);
        
        forwards_.clear();
    }
    
    const CompactionStats& stats() const { return stats_; }
    
private:
    ForwardingTable forwards_;
    CompactionStats stats_;
    
    void computeForwardingAddresses(void* heapStart, Runtime::ObjectHeader* head) {
        char* freePtr = static_cast<char*>(heapStart);
        
        for (auto* obj = head; obj; obj = obj->next) {
            if (obj->marked) {
                size_t size = sizeof(Runtime::ObjectHeader) + obj->size;
                void* newAddr = freePtr;
                
                if (newAddr != obj) {
                    forwards_.addForwarding(obj, newAddr);
                    stats_.objectsMoved++;
                    stats_.movedBytes += size;
                }
                
                freePtr += size;
                stats_.liveBytes += size;
            }
        }
    }
    
    template<typename UpdateFn>
    void updatePointers(Runtime::ObjectHeader* head, UpdateFn&& updateExternal) {
        // Update internal pointers in live objects
        for (auto* obj = head; obj; obj = obj->next) {
            if (obj->marked) {
                // Object's own next/prev pointers
                if (obj->next && forwards_.hasForwarding(obj->next)) {
                    obj->next = static_cast<Runtime::ObjectHeader*>(
                        forwards_.getForwarded(obj->next));
                    stats_.ptrUpdates++;
                }
            }
        }
        
        // Update external pointers via callback
        forwards_.forEach([&](void* from, void* to) {
            updateExternal(from, to);
        });
    }
    
    void moveObjects(Runtime::ObjectHeader* head) {
        for (auto* obj = head; obj; ) {
            Runtime::ObjectHeader* next = obj->next;
            
            if (obj->marked && forwards_.hasForwarding(obj)) {
                void* dest = forwards_.getForwarded(obj);
                size_t size = sizeof(Runtime::ObjectHeader) + obj->size;
                
                // Safe move using memmove (handles overlap)
                std::memmove(dest, obj, size);
            }
            
            obj = next;
        }
    }
};

/**
 * @brief Evacuation-based compaction (for parallel GC)
 * 
 * Instead of sliding, copies live objects to new region.
 * Allows parallel copying with less synchronization.
 */
class EvacuationCompactor {
public:
    struct Region {
        void* start;
        void* end;
        void* allocPtr;
    };
    
    EvacuationCompactor() = default;
    
    void setEvacuationTarget(Region target) {
        target_ = target;
        allocPtr_ = static_cast<char*>(target.start);
    }
    
    // Evacuate single object
    void* evacuate(Runtime::ObjectHeader* obj) {
        size_t size = sizeof(Runtime::ObjectHeader) + obj->size;
        
        if (allocPtr_ + size > target_.end) {
            return nullptr;  // Target full
        }
        
        void* dest = allocPtr_;
        std::memcpy(dest, obj, size);
        
        // Update forwarding in old location
        forwards_.addForwarding(obj, dest);
        
        allocPtr_ += size;
        stats_.objectsMoved++;
        stats_.movedBytes += size;
        
        return dest;
    }
    
    const ForwardingTable& forwards() const { return forwards_; }
    const CompactionStats& stats() const { return stats_; }
    
private:
    Region target_;
    char* allocPtr_ = nullptr;
    ForwardingTable forwards_;
    CompactionStats stats_;
};

} // namespace Zepra::GC
