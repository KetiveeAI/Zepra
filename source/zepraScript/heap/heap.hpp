#pragma once

/**
 * @file heap.hpp
 * @brief Garbage collected heap
 */

#include "../config.hpp"
#include <cstddef>
#include <vector>
#include <memory>

namespace Zepra {
namespace Runtime { class Object; }  // Forward declaration in correct namespace
}

namespace Zepra::GC {

// Forward declarations
class Handle;

/**
 * @brief GC statistics
 */
struct HeapStats {
    size_t totalHeapSize = 0;
    size_t usedHeapSize = 0;
    size_t objectCount = 0;
    size_t gcCount = 0;
    double lastGcTimeMs = 0;
};

/**
 * @brief Garbage collected heap
 * 
 * Mark-and-sweep collector with generational support.
 */
class Heap {
public:
    Heap(size_t initialSize = ZEPRA_GC_INITIAL_HEAP_SIZE,
         size_t maxSize = ZEPRA_GC_MAX_HEAP_SIZE);
    ~Heap();
    
    /**
     * @brief Allocate memory for an object
     */
    void* allocate(size_t size);
    
    /**
     * @brief Allocate a typed object
     */
    template<typename T, typename... Args>
    T* create(Args&&... args) {
        void* mem = allocate(sizeof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }
    
    /**
     * @brief Request garbage collection
     */
    void collectGarbage(bool full = false);
    
    /**
     * @brief Add a GC root
     */
    void addRoot(void* ptr);
    
    /**
     * @brief Remove a GC root
     */
    void removeRoot(void* ptr);
    
    /**
     * @brief Get heap statistics
     */
    HeapStats stats() const { return stats_; }
    
    /**
     * @brief Check if GC should run
     */
    bool shouldCollect() const;
    
    /**
     * @brief Visit all GC roots
     * @param visitor Callback receiving each root Object*
     */
    template<typename Visitor>
    void visitRoots(Visitor&& visitor) {
        for (void* ptr : roots_) {
            if (ptr) visitor(static_cast<Runtime::Object*>(ptr));
        }
    }
    
    /**
     * @brief Get all allocated objects for traversal
     */
    const std::vector<void*>& getAllObjects() const { return objects_; }
    
private:
    void mark();
    void markObject(void* obj);
    void sweep();
    
    size_t initialSize_;
    size_t maxSize_;
    size_t bytesAllocated_ = 0;
    size_t nextGCThreshold_;
    
    std::vector<void*> roots_;
    std::vector<void*> objects_;
    
    HeapStats stats_;
};

} // namespace Zepra::GC
