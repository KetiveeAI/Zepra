/**
 * @file memory_pool.cpp
 * @brief Fixed-size slab allocator for hot-path objects
 * 
 * Pre-allocates blocks of same-sized objects to avoid malloc overhead
 * on frequent allocations (small objects, property maps, AST nodes).
 */

#include "config.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cassert>

namespace Zepra::Memory {

/**
 * Fixed-size slab allocator.
 * Each slab holds N objects of size ObjectSize.
 * Free objects are linked via an embedded free-list pointer.
 */
class MemoryPool {
public:
    MemoryPool(size_t objectSize, size_t slabCapacity = ZEPRA_OBJECT_POOL_SIZE)
        : objectSize_(objectSize < sizeof(void*) ? sizeof(void*) : objectSize)
        , slabCapacity_(slabCapacity)
        , freeList_(nullptr)
        , allocCount_(0)
        , totalAllocated_(0) {}

    ~MemoryPool() {
        for (auto* slab : slabs_) {
            std::free(slab);
        }
    }

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    void* allocate() {
        if (!freeList_) {
            growPool();
        }

        void* obj = freeList_;
        freeList_ = *reinterpret_cast<void**>(freeList_);
        allocCount_++;
        return obj;
    }

    void deallocate(void* ptr) {
        if (!ptr) return;
        // Push onto free list
        *reinterpret_cast<void**>(ptr) = freeList_;
        freeList_ = ptr;
        allocCount_--;
    }

    size_t allocatedCount() const { return allocCount_; }
    size_t totalCapacity() const { return totalAllocated_; }
    size_t objectSize() const { return objectSize_; }

    /**
     * Reset pool — returns all objects to free list without freeing memory.
     * Use when you know all objects are dead (e.g., after GC full sweep).
     */
    void resetAll() {
        freeList_ = nullptr;
        allocCount_ = 0;

        for (auto* slab : slabs_) {
            char* base = static_cast<char*>(slab);
            for (size_t i = 0; i < slabCapacity_; i++) {
                void* obj = base + i * objectSize_;
                *reinterpret_cast<void**>(obj) = freeList_;
                freeList_ = obj;
            }
        }
    }

private:
    void growPool() {
        size_t slabSize = objectSize_ * slabCapacity_;
        void* slab = std::malloc(slabSize);
        if (!slab) throw std::bad_alloc();

        std::memset(slab, 0, slabSize);
        slabs_.push_back(slab);
        totalAllocated_ += slabCapacity_;

        // Build free list from the new slab (reverse order for locality)
        char* base = static_cast<char*>(slab);
        for (size_t i = 0; i < slabCapacity_; i++) {
            void* obj = base + i * objectSize_;
            *reinterpret_cast<void**>(obj) = freeList_;
            freeList_ = obj;
        }
    }

    size_t objectSize_;
    size_t slabCapacity_;
    void* freeList_;
    size_t allocCount_;
    size_t totalAllocated_;
    std::vector<void*> slabs_;
};

} // namespace Zepra::Memory
