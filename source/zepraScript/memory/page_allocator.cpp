/**
 * @file page_allocator.cpp
 * @brief Page-based memory allocator for large allocations
 * 
 * Provides efficient allocation of memory in page-sized chunks
 * for the garbage collector and large object storage.
 */

#include "config.hpp"
#include <cstdlib>
#include <cstring>
#include <vector>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace Zepra::Memory {

// =============================================================================
// PageAllocator - Allocates memory in page-sized chunks
// =============================================================================

class PageAllocator {
public:
    // Standard page size (4KB on most systems)
    static constexpr size_t PAGE_SIZE = 4096;
    
    /**
     * @brief Get the singleton instance
     */
    static PageAllocator& instance() {
        static PageAllocator allocator;
        return allocator;
    }
    
    /**
     * @brief Allocate a single page
     * @return Pointer to allocated page, or nullptr on failure
     */
    void* allocatePage() {
        return allocatePages(1);
    }
    
    /**
     * @brief Allocate multiple contiguous pages
     * @param count Number of pages to allocate
     * @return Pointer to allocated pages, or nullptr on failure
     */
    void* allocatePages(size_t count) {
        if (count == 0) return nullptr;
        
        size_t size = count * PAGE_SIZE;
        void* ptr = nullptr;
        
#ifdef _WIN32
        ptr = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
        ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, 
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED) ptr = nullptr;
#endif
        
        if (ptr) {
            std::lock_guard<std::mutex> lock(mutex_);
            allocations_.push_back({ptr, size});
            totalAllocated_ += size;
        }
        
        return ptr;
    }
    
    /**
     * @brief Free a single page
     * @param page Pointer to page to free
     */
    void freePage(void* page) {
        freePages(page, 1);
    }
    
    /**
     * @brief Free multiple contiguous pages
     * @param pages Pointer to first page
     * @param count Number of pages to free
     */
    void freePages(void* pages, size_t count) {
        if (!pages || count == 0) return;
        
        size_t size = count * PAGE_SIZE;
        
#ifdef _WIN32
        VirtualFree(pages, 0, MEM_RELEASE);
#else
        munmap(pages, size);
#endif
        
        std::lock_guard<std::mutex> lock(mutex_);
        // Remove from tracking
        for (auto it = allocations_.begin(); it != allocations_.end(); ++it) {
            if (it->ptr == pages) {
                totalAllocated_ -= it->size;
                allocations_.erase(it);
                break;
            }
        }
    }
    
    /**
     * @brief Get system page size
     */
    static size_t pageSize() {
#ifdef _WIN32
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        return si.dwPageSize;
#else
        return static_cast<size_t>(sysconf(_SC_PAGESIZE));
#endif
    }
    
    /**
     * @brief Total bytes allocated
     */
    size_t totalAllocated() const { return totalAllocated_; }
    
    /**
     * @brief Number of allocations
     */
    size_t allocationCount() const { return allocations_.size(); }
    
private:
    PageAllocator() = default;
    ~PageAllocator() {
        // Free all remaining allocations
        for (const auto& alloc : allocations_) {
#ifdef _WIN32
            VirtualFree(alloc.ptr, 0, MEM_RELEASE);
#else
            munmap(alloc.ptr, alloc.size);
#endif
        }
    }
    
    PageAllocator(const PageAllocator&) = delete;
    PageAllocator& operator=(const PageAllocator&) = delete;
    
    struct Allocation {
        void* ptr;
        size_t size;
    };
    
    std::vector<Allocation> allocations_;
    size_t totalAllocated_ = 0;
    std::mutex mutex_;
};

// =============================================================================
// Free Functions for C-style API
// =============================================================================

void* allocatePage() {
    return PageAllocator::instance().allocatePage();
}

void* allocatePages(size_t count) {
    return PageAllocator::instance().allocatePages(count);
}

void freePage(void* page) {
    PageAllocator::instance().freePage(page);
}

void freePages(void* pages, size_t count) {
    PageAllocator::instance().freePages(pages, count);
}

size_t getPageSize() {
    return PageAllocator::pageSize();
}

} // namespace Zepra::Memory
