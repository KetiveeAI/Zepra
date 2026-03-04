/**
 * @file marking.cpp
 * @brief Tri-color marking implementation for incremental GC
 * 
 * Implements the marking phase of garbage collection using tri-color
 * abstraction (white/gray/black) with a worklist-based traversal.
 * Supports incremental marking to reduce pause times.
 */

#include "heap/heap.hpp"
#include "heap/incremental_gc.hpp"
#include "runtime/objects/object.hpp"
#include <vector>
#include <queue>
#include <cassert>

namespace Zepra::GC {

// =============================================================================
// MarkingWorkList - Efficient worklist for gray objects
// =============================================================================

class MarkingWorkList {
public:
    static constexpr size_t SEGMENT_SIZE = 256;
    
    struct Segment {
        Runtime::Object* objects[SEGMENT_SIZE];
        size_t count;
        Segment* next;
        
        Segment() : count(0), next(nullptr) {}
        
        bool push(Runtime::Object* obj) {
            if (count >= SEGMENT_SIZE) return false;
            objects[count++] = obj;
            return true;
        }
        
        Runtime::Object* pop() {
            if (count == 0) return nullptr;
            return objects[--count];
        }
        
        bool empty() const { return count == 0; }
        bool full() const { return count >= SEGMENT_SIZE; }
    };
    
    MarkingWorkList() {
        currentSegment_ = new Segment();
    }
    
    ~MarkingWorkList() {
        while (currentSegment_) {
            Segment* next = currentSegment_->next;
            delete currentSegment_;
            currentSegment_ = next;
        }
        
        // Free cached segments
        while (freeSegments_) {
            Segment* next = freeSegments_->next;
            delete freeSegments_;
            freeSegments_ = next;
        }
    }
    
    /**
     * @brief Push an object to the worklist (making it gray)
     */
    void push(Runtime::Object* obj) {
        if (!obj) return;
        
        if (currentSegment_->full()) {
            // Get or create new segment
            Segment* newSeg = allocateSegment();
            newSeg->next = currentSegment_;
            currentSegment_ = newSeg;
        }
        
        currentSegment_->push(obj);
        size_++;
    }
    
    /**
     * @brief Pop an object from the worklist for processing
     */
    Runtime::Object* pop() {
        if (empty()) return nullptr;
        
        Runtime::Object* obj = currentSegment_->pop();
        
        if (currentSegment_->empty() && currentSegment_->next) {
            // Move to next segment
            Segment* empty = currentSegment_;
            currentSegment_ = currentSegment_->next;
            
            // Cache the empty segment for reuse
            empty->next = freeSegments_;
            freeSegments_ = empty;
        }
        
        size_--;
        return obj;
    }
    
    /**
     * @brief Check if worklist is empty
     */
    bool empty() const {
        return size_ == 0;
    }
    
    /**
     * @brief Get number of objects in worklist
     */
    size_t size() const {
        return size_;
    }
    
    /**
     * @brief Clear the worklist
     */
    void clear() {
        // Move all segments to free list
        while (currentSegment_->next) {
            Segment* toFree = currentSegment_;
            currentSegment_ = currentSegment_->next;
            toFree->count = 0;
            toFree->next = freeSegments_;
            freeSegments_ = toFree;
        }
        currentSegment_->count = 0;
        size_ = 0;
    }
    
private:
    Segment* allocateSegment() {
        if (freeSegments_) {
            Segment* seg = freeSegments_;
            freeSegments_ = freeSegments_->next;
            seg->next = nullptr;
            seg->count = 0;
            return seg;
        }
        return new Segment();
    }
    
    Segment* currentSegment_;
    Segment* freeSegments_ = nullptr;
    size_t size_ = 0;
};

// =============================================================================
// ObjectVisitor - Interface for visiting object references
// =============================================================================

/**
 * @brief Visitor callback type for object references
 */
using ObjectVisitor = std::function<void(Runtime::Object**)>;

/**
 * @brief Visit all object references in an object
 * 
 * This function must be updated when new object types are added.
 */
void visitObjectReferences(Runtime::Object* obj, const ObjectVisitor& visitor) {
    if (!obj) return;
    
    // Visit prototype chain
    Runtime::Object* proto = obj->prototype();
    if (proto) {
        // Create temporary pointer for visitor
        Runtime::Object* protoPtr = proto;
        visitor(&protoPtr);
    }
    
    // Visit properties (objects with property storage)
    // The Object class stores properties in a map
    // For production, Object should expose a property iteration method
    
    // For now, we rely on Object::markGC() to handle property marking
    // This is already implemented in the existing code
}

// =============================================================================
// Marker - Manages the marking process
// =============================================================================

class Marker {
public:
    explicit Marker(Heap* heap) : heap_(heap) {}
    
    /**
     * @brief Initialize marking phase
     * @param roots Root objects to start marking from
     */
    void initialize(const std::vector<Runtime::Object*>& roots) {
        worklist_.clear();
        bytesMarked_ = 0;
        objectsMarked_ = 0;
        
        // Push all roots to worklist (make them gray)
        for (Runtime::Object* root : roots) {
            if (root && !root->isMarked()) {
                worklist_.push(root);
            }
        }
    }
    
    /**
     * @brief Perform incremental marking
     * @param budget Maximum number of objects to mark
     * @return true if marking is complete
     */
    bool markIncremental(size_t budget) {
        size_t processed = 0;
        
        while (!worklist_.empty() && processed < budget) {
            Runtime::Object* obj = worklist_.pop();
            
            if (!obj || obj->isMarked()) continue;
            
            // Mark object black
            obj->markGC();
            objectsMarked_++;
            bytesMarked_ += sizeof(Runtime::Object);  // Approximate
            
            // Visit references and add unvisited to worklist (make them gray)
            visitObjectReferences(obj, [this](Runtime::Object** ref) {
                if (ref && *ref && !(*ref)->isMarked()) {
                    worklist_.push(*ref);
                }
            });
            
            processed++;
        }
        
        return worklist_.empty();
    }
    
    /**
     * @brief Complete all remaining marking (for full GC)
     */
    void markComplete() {
        while (!worklist_.empty()) {
            markIncremental(SIZE_MAX);
        }
    }
    
    /**
     * @brief Write barrier for incremental correctness
     * Called when a black object gains a reference to a white object
     */
    void onWriteBarrier(Runtime::Object* holder, Runtime::Object* newValue) {
        // If holder is marked (black) and newValue is not (white),
        // we need to add newValue to the worklist (make it gray)
        if (holder && holder->isMarked() && newValue && !newValue->isMarked()) {
            worklist_.push(newValue);
        }
    }
    
    /**
     * @brief Check if marking is complete
     */
    bool isComplete() const {
        return worklist_.empty();
    }
    
    /**
     * @brief Get number of marked objects
     */
    size_t objectsMarked() const { return objectsMarked_; }
    
    /**
     * @brief Get approximate bytes marked
     */
    size_t bytesMarked() const { return bytesMarked_; }
    
    /**
     * @brief Get worklist size (gray objects remaining)
     */
    size_t worklistSize() const { return worklist_.size(); }
    
private:
    Heap* heap_;
    MarkingWorkList worklist_;
    size_t bytesMarked_ = 0;
    size_t objectsMarked_ = 0;
};

// =============================================================================
// MarkingStats - Statistics for the marking phase
// =============================================================================

struct MarkingStats {
    size_t objectsVisited = 0;
    size_t objectsMarked = 0;
    size_t bytesMarked = 0;
    double markingTimeMs = 0;
    size_t incrementCount = 0;
    
    void reset() {
        objectsVisited = 0;
        objectsMarked = 0;
        bytesMarked = 0;
        markingTimeMs = 0;
        incrementCount = 0;
    }
};

// =============================================================================
// Global Marker Instance
// =============================================================================

static Marker* globalMarker = nullptr;
static MarkingStats globalMarkingStats;

void initializeMarker(Heap* heap) {
    if (!globalMarker) {
        globalMarker = new Marker(heap);
    }
}

void shutdownMarker() {
    delete globalMarker;
    globalMarker = nullptr;
}

Marker* getMarker() {
    return globalMarker;
}

const MarkingStats& getMarkingStats() {
    return globalMarkingStats;
}

} // namespace Zepra::GC
