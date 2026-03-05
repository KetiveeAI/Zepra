/**
 * @file gc_large_object.cpp
 * @brief Large object space — dedicated allocation for big objects
 *
 * Objects larger than a threshold (e.g., 8KB) are allocated
 * in their own dedicated pages. They are never moved (pinned).
 *
 * Benefits:
 * - No copying overhead during scavenge
 * - No fragmentation in nursery or old-gen
 * - Simple mark-sweep (just flip a bit)
 *
 * Each large object gets its own mmap allocation.
 * The GC tracks them in a list and frees unmapped ones on sweep.
 */

#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <memory>
#include <algorithm>

#ifdef __linux__
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace Zepra::Heap {

// =============================================================================
// Large Object Descriptor
// =============================================================================

struct LargeObjectDescriptor {
    uintptr_t address;
    size_t size;          // Allocation size (page-aligned)
    size_t objectSize;    // Actual object size
    uint32_t shapeId;
    bool marked;
    bool pinned;

    LargeObjectDescriptor()
        : address(0), size(0), objectSize(0)
        , shapeId(0), marked(false), pinned(true) {}
};

// =============================================================================
// Large Object Space
// =============================================================================

class LargeObjectSpace {
public:
    static constexpr size_t LARGE_OBJECT_THRESHOLD = 8192;  // 8KB

    struct Stats {
        uint64_t allocations;
        uint64_t deallocations;
        size_t currentCount;
        size_t currentBytes;
        size_t peakBytes;
        uint64_t totalAllocated;
        uint64_t totalFreed;
    };

    LargeObjectSpace() = default;
    ~LargeObjectSpace() { destroyAll(); }

    /**
     * @brief Allocate a large object
     */
    uintptr_t allocate(size_t size, uint32_t shapeId = 0) {
        size_t pageSize = 4096;
#ifdef __linux__
        pageSize = static_cast<size_t>(sysconf(_SC_PAGESIZE));
#endif
        size_t allocSize = (size + pageSize - 1) & ~(pageSize - 1);

        uintptr_t addr = 0;
#ifdef __linux__
        void* mem = mmap(nullptr, allocSize, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mem == MAP_FAILED) return 0;
        addr = reinterpret_cast<uintptr_t>(mem);
#else
        void* mem = std::aligned_alloc(pageSize, allocSize);
        if (!mem) return 0;
        addr = reinterpret_cast<uintptr_t>(mem);
        std::memset(mem, 0, allocSize);
#endif

        LargeObjectDescriptor desc;
        desc.address = addr;
        desc.size = allocSize;
        desc.objectSize = size;
        desc.shapeId = shapeId;
        desc.marked = false;
        desc.pinned = true;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            objects_.push_back(desc);
            stats_.allocations++;
            stats_.currentCount++;
            stats_.currentBytes += allocSize;
            stats_.totalAllocated += allocSize;
            if (stats_.currentBytes > stats_.peakBytes) {
                stats_.peakBytes = stats_.currentBytes;
            }
        }

        return addr;
    }

    /**
     * @brief Check if address is in large object space
     */
    bool contains(uintptr_t addr) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& obj : objects_) {
            if (addr >= obj.address && addr < obj.address + obj.size) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Mark a large object as live
     */
    bool mark(uintptr_t addr) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& obj : objects_) {
            if (obj.address == addr) {
                bool wasMarked = obj.marked;
                obj.marked = true;
                return !wasMarked;
            }
        }
        return false;
    }

    /**
     * @brief Sweep: free unmarked large objects
     * @return bytes freed
     */
    size_t sweep() {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t freed = 0;

        auto it = objects_.begin();
        while (it != objects_.end()) {
            if (!it->marked) {
                freed += it->size;
                freeLargeObject(*it);
                it = objects_.erase(it);
            } else {
                it->marked = false;  // Reset for next cycle
                ++it;
            }
        }

        return freed;
    }

    /**
     * @brief Clear all marks (before marking phase)
     */
    void clearMarks() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& obj : objects_) {
            obj.marked = false;
        }
    }

    void destroyAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& obj : objects_) {
            freeLargeObject(obj);
        }
        objects_.clear();
        stats_.currentCount = 0;
        stats_.currentBytes = 0;
    }

    /**
     * @brief Iterate all large objects
     */
    void forEach(std::function<void(uintptr_t addr, size_t size,
                                     bool marked)> visitor) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& obj : objects_) {
            visitor(obj.address, obj.objectSize, obj.marked);
        }
    }

    Stats stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }

    size_t objectCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return objects_.size();
    }

private:
    void freeLargeObject(const LargeObjectDescriptor& desc) {
#ifdef __linux__
        munmap(reinterpret_cast<void*>(desc.address), desc.size);
#else
        std::free(reinterpret_cast<void*>(desc.address));
#endif
        stats_.deallocations++;
        stats_.currentCount--;
        stats_.currentBytes -= desc.size;
        stats_.totalFreed += desc.size;
    }

    mutable std::mutex mutex_;
    std::vector<LargeObjectDescriptor> objects_;
    Stats stats_{};
};

} // namespace Zepra::Heap
