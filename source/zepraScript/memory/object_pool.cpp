/**
 * @file object_pool.cpp
 * @brief Memory-efficient object pool implementation
 */

#include "memory/object_pool.hpp"
#include <cstring>
#include <algorithm>

namespace Zepra::Memory {

// =============================================================================
// SlabAllocator::Slab Implementation
// =============================================================================

SlabAllocator::Slab::Slab(size_t objSize) 
    : objectSize(objSize)
    , objectCount(SLAB_SIZE / objSize) {
    
    // Allocate slab memory
    data = new uint8_t[SLAB_SIZE];
    
    // Initialize free list with all slots
    freeList.reserve(objectCount);
    for (size_t i = 0; i < objectCount; ++i) {
        freeList.push_back(data + (i * objectSize));
    }
}

SlabAllocator::Slab::~Slab() {
    delete[] data;
}

void* SlabAllocator::Slab::allocate() {
    if (freeList.empty()) return nullptr;
    
    void* ptr = freeList.back();
    freeList.pop_back();
    return ptr;
}

void SlabAllocator::Slab::deallocate(void* ptr) {
    freeList.push_back(ptr);
}

bool SlabAllocator::Slab::contains(void* ptr) const {
    return ptr >= data && ptr < (data + SLAB_SIZE);
}

// =============================================================================
// SlabAllocator Implementation
// =============================================================================

SlabAllocator::~SlabAllocator() {
    for (Slab* s : slabs16_) delete s;
    for (Slab* s : slabs32_) delete s;
    for (Slab* s : slabs64_) delete s;
    for (Slab* s : slabs128_) delete s;
    for (Slab* s : slabs256_) delete s;
}

void* SlabAllocator::allocate(size_t size) {
    if (size > MAX_OBJECT_SIZE) {
        // Too large for slab - use heap
        totalAllocated_ += size;
        activeAllocs_++;
        return malloc(size);
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t sc = sizeClass(size);
    std::vector<Slab*>* slabs = nullptr;
    
    switch (sc) {
        case 16:  slabs = &slabs16_;  break;
        case 32:  slabs = &slabs32_;  break;
        case 64:  slabs = &slabs64_;  break;
        case 128: slabs = &slabs128_; break;
        case 256: slabs = &slabs256_; break;
    }
    
    if (!slabs) return nullptr;
    
    // Try existing slabs first
    for (Slab* slab : *slabs) {
        if (!slab->isFull()) {
            void* ptr = slab->allocate();
            if (ptr) {
                activeAllocs_++;
                return ptr;
            }
        }
    }
    
    // Create new slab
    Slab* newSlab = new Slab(sc);
    slabs->push_back(newSlab);
    totalAllocated_ += SLAB_SIZE;
    
    void* ptr = newSlab->allocate();
    activeAllocs_++;
    return ptr;
}

void SlabAllocator::deallocate(void* ptr, size_t size) {
    if (!ptr) return;
    
    if (size > MAX_OBJECT_SIZE) {
        // Heap allocated
        free(ptr);
        activeAllocs_--;
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t sc = sizeClass(size);
    std::vector<Slab*>* slabs = nullptr;
    
    switch (sc) {
        case 16:  slabs = &slabs16_;  break;
        case 32:  slabs = &slabs32_;  break;
        case 64:  slabs = &slabs64_;  break;
        case 128: slabs = &slabs128_; break;
        case 256: slabs = &slabs256_; break;
    }
    
    if (!slabs) return;
    
    // Find containing slab
    for (Slab* slab : *slabs) {
        if (slab->contains(ptr)) {
            slab->deallocate(ptr);
            activeAllocs_--;
            return;
        }
    }
    
    // Not found in slabs - must be heap
    free(ptr);
    activeAllocs_--;
}

// =============================================================================
// PoolManager Implementation (Singleton)
// =============================================================================

PoolManager& PoolManager::instance() {
    static PoolManager mgr;
    return mgr;
}

PoolManager::PoolManager() = default;
PoolManager::~PoolManager() = default;

void* PoolManager::allocateSmall(size_t size) {
    return slab_.allocate(size);
}

void PoolManager::deallocateSmall(void* ptr, size_t size) {
    slab_.deallocate(ptr, size);
}

PoolManager::Stats PoolManager::stats() const {
    Stats s;
    s.poolAllocations = 0;  // TODO: Track
    s.heapAllocations = 0;
    s.totalActive = slab_.activeAllocations();
    s.bytesInPools = slab_.totalAllocated();
    return s;
}

} // namespace Zepra::Memory
