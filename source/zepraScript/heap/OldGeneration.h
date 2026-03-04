/**
 * @file OldGeneration.h
 * @brief Tenured/Old generation heap management
 * 
 * Implements:
 * - Free-list allocator
 * - Mark-sweep collection
 * - Mark-sweep collection
 * - Large object space
 */

#pragma once

#include "gc_heap.hpp"
#include "Compaction.h"
#include <map>
#include <list>

namespace Zepra::GC {

/**
 * @brief Free list entry
 */
struct FreeBlock {
    size_t size;
    FreeBlock* next;
};

/**
 * @brief Old generation statistics
 */
struct OldGenStats {
    size_t allocations = 0;
    size_t bytesAllocated = 0;
    size_t collections = 0;
    size_t bytesReclaimed = 0;
    size_t fragmentationBytes = 0;
    size_t largeObjectBytes = 0;
};

/**
 * @brief Free-list based allocator for old generation
 * 
 * Uses segregated free lists for common sizes.
 * Falls back to first-fit for larger allocations.
 */
class FreeListAllocator {
public:
    // Size classes for segregated free lists
    static constexpr size_t SIZE_CLASSES[] = {
        16, 32, 48, 64, 80, 96, 128, 192, 256, 384, 512, 768, 1024, 2048
    };
    static constexpr size_t NUM_SIZE_CLASSES = sizeof(SIZE_CLASSES) / sizeof(SIZE_CLASSES[0]);
    static constexpr size_t LARGE_THRESHOLD = 2048;
    
    FreeListAllocator() {
        for (size_t i = 0; i < NUM_SIZE_CLASSES; ++i) {
            freeLists_[i] = nullptr;
        }
    }
    
    /**
     * @brief Allocate from free list
     */
    void* allocate(size_t size) {
        // Round up to 8 bytes
        size = (size + 7) & ~7;
        
        // Try segregated list first
        int sizeClass = findSizeClass(size);
        if (sizeClass >= 0 && freeLists_[sizeClass]) {
            FreeBlock* block = freeLists_[sizeClass];
            freeLists_[sizeClass] = block->next;
            stats_.allocations++;
            stats_.bytesAllocated += SIZE_CLASSES[sizeClass];
            return block;
        }
        
        // Try large free list
        return allocateLarge(size);
    }
    
    /**
     * @brief Return block to free list
     */
    void free(void* ptr, size_t size) {
        size = (size + 7) & ~7;
        
        FreeBlock* block = static_cast<FreeBlock*>(ptr);
        
        int sizeClass = findSizeClass(size);
        if (sizeClass >= 0) {
            block->size = SIZE_CLASSES[sizeClass];
            block->next = freeLists_[sizeClass];
            freeLists_[sizeClass] = block;
        } else {
            // Add to large list
            block->size = size;
            block->next = largeList_;
            largeList_ = block;
        }
    }
    
    /**
     * @brief Add chunk to allocator
     */
    void addChunk(void* start, size_t size) {
        FreeBlock* block = static_cast<FreeBlock*>(start);
        block->size = size;
        block->next = largeList_;
        largeList_ = block;
    }
    
    const OldGenStats& stats() const { return stats_; }
    
private:
    FreeBlock* freeLists_[NUM_SIZE_CLASSES];
    FreeBlock* largeList_ = nullptr;
    OldGenStats stats_;
    
    int findSizeClass(size_t size) {
        for (size_t i = 0; i < NUM_SIZE_CLASSES; ++i) {
            if (SIZE_CLASSES[i] >= size) return static_cast<int>(i);
        }
        return -1;  // Large object
    }
    
    void* allocateLarge(size_t size) {
        FreeBlock** prev = &largeList_;
        for (FreeBlock* block = largeList_; block; block = block->next) {
            if (block->size >= size) {
                *prev = block->next;
                
                // Split if much larger
                if (block->size >= size + 64) {
                    size_t remaining = block->size - size;
                    FreeBlock* rest = reinterpret_cast<FreeBlock*>(
                        reinterpret_cast<char*>(block) + size);
                    rest->size = remaining;
                    rest->next = largeList_;
                    largeList_ = rest;
                }
                
                stats_.allocations++;
                stats_.bytesAllocated += size;
                return block;
            }
            prev = &block->next;
        }
        return nullptr;  // No suitable block
    }
};

/**
 * @brief Old generation (tenured) heap
 */
class OldGeneration {
public:
    static constexpr size_t CHUNK_SIZE = 1024 * 1024;  // 1MB chunks
    static constexpr size_t LARGE_OBJECT_THRESHOLD = 32 * 1024;  // 32KB
    
    OldGeneration() = default;
    ~OldGeneration() { destroy(); }
    
    /**
     * @brief Initialize with initial capacity
     */
    bool init(size_t initialCapacity = CHUNK_SIZE) {
        return addChunk(initialCapacity);
    }
    
    /**
     * @brief Allocate in old generation
     */
    void* allocate(size_t size) {
        if (size >= LARGE_OBJECT_THRESHOLD) {
            return allocateLargeObject(size);
        }
        
        void* result = allocator_.allocate(size);
        if (!result) {
            // Need more memory
            if (addChunk(CHUNK_SIZE)) {
                result = allocator_.allocate(size);
            }
        }
        return result;
    }
    
    /**
     * @brief Sweep dead objects and rebuild free list
     */
    void sweep(Runtime::ObjectHeader* head) {
        for (auto* obj = head; obj; ) {
            Runtime::ObjectHeader* next = obj->next;
            
            if (!obj->marked && obj->generation == Runtime::Generation::Old) {
                size_t size = sizeof(Runtime::ObjectHeader) + obj->size;
                allocator_.free(obj, size);
                stats_.bytesReclaimed += size;
            } else {
                obj->marked = false;  // Clear mark for next cycle
            }
            
            obj = next;
        }
        stats_.collections++;
    }
    
    /**
     * @brief Perform compaction
     */
    template<typename UpdateFn>
    void compact(Runtime::ObjectHeader* head, UpdateFn&& updatePtr) {
        if (chunks_.empty()) return;
        
        Compactor compactor;
        compactor.compact(chunks_[0].start, chunks_[0].end, head, 
                          std::forward<UpdateFn>(updatePtr));
    }
    
    const OldGenStats& stats() const { return stats_; }
    
private:
    struct Chunk {
        void* start;
        void* end;
        size_t size;
    };
    
    std::vector<Chunk> chunks_;
    FreeListAllocator allocator_;
    OldGenStats stats_;
    
    // Large object space (separate tracking)
    std::list<void*> largeObjects_;
    
    bool addChunk(size_t size) {
        void* mem = std::aligned_alloc(4096, size);
        if (!mem) return false;
        
        chunks_.push_back({mem, static_cast<char*>(mem) + size, size});
        allocator_.addChunk(mem, size);
        return true;
    }
    
    void* allocateLargeObject(size_t size) {
        void* mem = std::aligned_alloc(4096, size);
        if (mem) {
            largeObjects_.push_back(mem);
            stats_.largeObjectBytes += size;
        }
        return mem;
    }
    
    void destroy() {
        for (auto& chunk : chunks_) {
            std::free(chunk.start);
        }
        chunks_.clear();
        
        for (auto* obj : largeObjects_) {
            std::free(obj);
        }
        largeObjects_.clear();
    }
};

} // namespace Zepra::GC
