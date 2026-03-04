/**
 * @file Nursery.h
 * @brief Young generation nursery for fast allocation
 * 
 * Implements:
 * - Bump-pointer allocation
 * - Semi-space nursery (optional)
 * - Scavenger for minor GC
 */

#pragma once

#include "WriteBarrier.h"
#include <cstdlib>
#include <cstring>
#include <vector>

namespace Zepra::Runtime {
    class Object;
}

namespace Zepra::GC {

class OldGeneration;

/**
 * @brief Nursery allocation statistics
 */
struct NurseryStats {
    size_t allocations = 0;
    size_t bytesAllocated = 0;
    size_t scavenges = 0;
    size_t survivorBytes = 0;
    size_t promotedBytes = 0;
    size_t promotedObjects = 0;
};

/**
 * @brief Bump-pointer nursery for young generation
 * 
 * Fast allocation with simple pointer bump.
 * Minor GC (scavenge) promotes survivors to old generation.
 */
class Nursery {
public:
    static constexpr size_t DEFAULT_SIZE = 4 * 1024 * 1024;  // 4MB
    static constexpr size_t MIN_SIZE = 256 * 1024;           // 256KB
    static constexpr size_t MAX_SIZE = 64 * 1024 * 1024;     // 64MB
    
    Nursery() = default;
    ~Nursery() { destroy(); }
    
    // No copy/move
    Nursery(const Nursery&) = delete;
    Nursery& operator=(const Nursery&) = delete;
    
    /**
     * @brief Initialize nursery with given size
     */
    bool init(size_t size = DEFAULT_SIZE) {
        size = std::max(MIN_SIZE, std::min(MAX_SIZE, size));
        
        // Align to page boundary
        size = (size + 4095) & ~4095;
        
        start_ = static_cast<char*>(std::aligned_alloc(4096, size));
        if (!start_) return false;
        
        end_ = start_ + size;
        allocPtr_ = start_;
        capacity_ = size;
        
        std::memset(start_, 0, size);
        return true;
    }
    
    /**
     * @brief Destroy nursery and free memory
     */
    void destroy() {
        if (start_) {
            std::free(start_);
            start_ = nullptr;
            end_ = nullptr;
            allocPtr_ = nullptr;
        }
    }
    
    /**
     * @brief Allocate memory in nursery
     * @param size Bytes to allocate (including header)
     * @return Pointer to allocated memory, or nullptr if full
     */
    void* allocate(size_t size) {
        // Align to 8 bytes
        size = (size + 7) & ~7;
        
        char* result = allocPtr_;
        char* newPtr = allocPtr_ + size;
        
        if (newPtr > end_) {
            return nullptr;  // Nursery full
        }
        
        allocPtr_ = newPtr;
        stats_.allocations++;
        stats_.bytesAllocated += size;
        
        return result;
    }
    
    /**
     * @brief Check if address is in nursery
     */
    bool contains(void* addr) const {
        return addr >= start_ && addr < end_;
    }
    
    /**
     * @brief Reset nursery after scavenge
     */
    void reset() {
        allocPtr_ = start_;
        std::memset(start_, 0, capacity_);
    }
    
    /**
     * @brief Get amount of memory allocated
     */
    size_t used() const { return allocPtr_ - start_; }
    
    /**
     * @brief Get total capacity
     */
    size_t capacity() const { return capacity_; }
    
    /**
     * @brief Check if nursery is full (>90%)
     */
    bool needsScavenge() const {
        return used() > (capacity_ * 9 / 10);
    }
    
    /**
     * @brief Get allocation pointer for inline allocation
     */
    char** allocPtrAddress() { return &allocPtr_; }
    char* endAddress() const { return end_; }
    
    const NurseryStats& stats() const { return stats_; }
    
    friend class Scavenger;
    
private:
    char* start_ = nullptr;
    char* end_ = nullptr;
    char* allocPtr_ = nullptr;
    size_t capacity_ = 0;
    NurseryStats stats_;
};

/**
 * @brief Scavenger for minor GC
 * 
 * Copies live objects from nursery to:
 * - To-space (if using semi-space)
 * - Old generation (if promoted)
 */
class Scavenger {
public:
    explicit Scavenger(Nursery& nursery) : nursery_(nursery) {}
    
    /**
     * @brief Perform scavenge (minor GC)
     * @param roots Root set to trace from
     * @param oldGen Old generation to promote to
     * @param barriers Write barrier manager
     */
    void scavenge(const std::vector<Runtime::Object**>& roots, OldGeneration& oldGen, WriteBarrierManager& barriers);

private:
    Nursery& nursery_;
    std::vector<Runtime::Object*> workQueue_;

    Runtime::Object* evacuateOrPromote(Runtime::Object* obj, OldGeneration& oldGen);
    void scanRegion(void* start, void* end, OldGeneration& oldGen);
    void drainWorkQueue(OldGeneration& oldGen);
};

} // namespace Zepra::GC
