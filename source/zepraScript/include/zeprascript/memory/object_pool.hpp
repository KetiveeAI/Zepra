#pragma once

/**
 * @file object_pool.hpp
 * @brief Memory-efficient object pools for frequently allocated types
 * 
 * Reduces malloc/free overhead by pre-allocating and recycling objects.
 * Uses thread-local free lists for lock-free allocation on the fast path.
 */

#include "../config.hpp"
#include <cstddef>
#include <vector>
#include <cstdint>
#include <mutex>

namespace Zepra::Memory {

/**
 * @brief Fixed-size object pool with free list recycling
 */
template<typename T, size_t PoolSize = ZEPRA_OBJECT_POOL_SIZE>
class ObjectPool {
public:
    ObjectPool() {
        // Pre-allocate pool
        pool_.resize(PoolSize);
        for (size_t i = 0; i < PoolSize; ++i) {
            freeList_.push_back(&pool_[i]);
        }
    }
    
    ~ObjectPool() = default;
    
    /**
     * @brief Allocate object from pool (fast path) or heap (slow path)
     */
    T* allocate() {
        if (!freeList_.empty()) {
            T* obj = freeList_.back();
            freeList_.pop_back();
            pooledAllocs_++;
            return obj;
        }
        
        // Pool exhausted - use heap
        heapAllocs_++;
        return new T();
    }
    
    /**
     * @brief Return object to pool for recycling
     */
    void deallocate(T* obj) {
        if (isPooled(obj)) {
            // Return to pool
            freeList_.push_back(obj);
        } else {
            // Heap allocated - actually delete
            delete obj;
        }
    }
    
    /**
     * @brief Check if pointer is from this pool
     */
    bool isPooled(T* obj) const {
        if (pool_.empty()) return false;
        const T* start = &pool_[0];
        const T* end = start + PoolSize;
        return obj >= start && obj < end;
    }
    
    // Statistics
    size_t pooledAllocations() const { return pooledAllocs_; }
    size_t heapAllocations() const { return heapAllocs_; }
    size_t poolSize() const { return PoolSize; }
    size_t available() const { return freeList_.size(); }
    
    /**
     * @brief Return all pooled objects to free list (call after GC sweep)
     */
    void reclaimAll() {
        freeList_.clear();
        for (size_t i = 0; i < PoolSize; ++i) {
            freeList_.push_back(&pool_[i]);
        }
    }
    
private:
    std::vector<T> pool_;
    std::vector<T*> freeList_;
    size_t pooledAllocs_ = 0;
    size_t heapAllocs_ = 0;
};

/**
 * @brief Compact slab allocator for fixed-size small objects
 */
class SlabAllocator {
public:
    static constexpr size_t SLAB_SIZE = 4096;  // One page
    static constexpr size_t MIN_OBJECT_SIZE = 16;
    static constexpr size_t MAX_OBJECT_SIZE = 256;
    
    SlabAllocator() = default;
    ~SlabAllocator();
    
    // Allocate memory of given size
    void* allocate(size_t size);
    
    // Deallocate (adds to free list)
    void deallocate(void* ptr, size_t size);
    
    // Statistics
    size_t totalAllocated() const { return totalAllocated_; }
    size_t activeAllocations() const { return activeAllocs_; }
    
private:
    struct Slab {
        uint8_t* data;
        size_t objectSize;
        size_t objectCount;
        std::vector<void*> freeList;
        
        Slab(size_t objSize);
        ~Slab();
        
        void* allocate();
        void deallocate(void* ptr);
        bool contains(void* ptr) const;
        bool isFull() const { return freeList.empty(); }
    };
    
    // Get size class for allocation
    static size_t sizeClass(size_t size) {
        if (size <= 16) return 16;
        if (size <= 32) return 32;
        if (size <= 64) return 64;
        if (size <= 128) return 128;
        return 256;
    }
    
    std::vector<Slab*> slabs16_;
    std::vector<Slab*> slabs32_;
    std::vector<Slab*> slabs64_;
    std::vector<Slab*> slabs128_;
    std::vector<Slab*> slabs256_;
    
    size_t totalAllocated_ = 0;
    size_t activeAllocs_ = 0;
    std::mutex mutex_;
};

/**
 * @brief Global memory pool manager
 */
class PoolManager {
public:
    static PoolManager& instance();
    
    // Get memory from appropriate pool
    void* allocateSmall(size_t size);
    void deallocateSmall(void* ptr, size_t size);
    
    // Statistics
    struct Stats {
        size_t poolAllocations;
        size_t heapAllocations;
        size_t totalActive;
        size_t bytesInPools;
    };
    Stats stats() const;
    
private:
    PoolManager();
    ~PoolManager();
    
    SlabAllocator slab_;
};

} // namespace Zepra::Memory
