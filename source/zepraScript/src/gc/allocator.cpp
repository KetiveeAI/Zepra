/**
 * @file allocator.cpp
 * @brief Arena-based memory allocator for GC heap
 * 
 * Implements a size-segregated allocator with:
 * - Small objects (<= 256 bytes): Size-class segregation with free lists
 * - Medium objects (256 - 8KB): Bump pointer allocation in arenas
 * - Large objects (> 8KB): Direct allocation with separate tracking
 */

#include "zeprascript/gc/heap.hpp"
#include "zeprascript/config.hpp"
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <array>
#include <vector>
#include <cassert>

namespace Zepra::GC {

// =============================================================================
// Constants
// =============================================================================

static constexpr size_t SMALL_OBJECT_THRESHOLD = 256;
static constexpr size_t LARGE_OBJECT_THRESHOLD = 8 * 1024;
static constexpr size_t ARENA_SIZE = 64 * 1024;  // 64KB arenas
static constexpr size_t ALIGNMENT = 8;
static constexpr size_t NUM_SIZE_CLASSES = 32;   // Size classes for small objects

// Size class boundaries: 8, 16, 24, 32, 48, 64, 80, 96, 128, 160, 192, 256
static constexpr std::array<size_t, NUM_SIZE_CLASSES> SIZE_CLASSES = {
    8, 16, 24, 32, 40, 48, 56, 64,
    72, 80, 88, 96, 112, 128, 144, 160,
    176, 192, 208, 224, 240, 256, 288, 320,
    384, 448, 512, 640, 768, 1024, 1280, 2048
};

// =============================================================================
// Arena - Contiguous memory block for bump allocation
// =============================================================================

struct Arena {
    uint8_t* memory;
    size_t size;
    size_t used;
    Arena* next;
    
    Arena(size_t arenaSize) 
        : memory(static_cast<uint8_t*>(std::malloc(arenaSize)))
        , size(arenaSize)
        , used(0)
        , next(nullptr) {
        if (memory) {
            std::memset(memory, 0, arenaSize);
        }
    }
    
    ~Arena() {
        std::free(memory);
    }
    
    void* allocate(size_t bytes) {
        // Align the allocation
        size_t alignedUsed = (used + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
        if (alignedUsed + bytes > size) {
            return nullptr;
        }
        void* ptr = memory + alignedUsed;
        used = alignedUsed + bytes;
        return ptr;
    }
    
    size_t remaining() const {
        return size - used;
    }
    
    bool contains(void* ptr) const {
        uint8_t* p = static_cast<uint8_t*>(ptr);
        return p >= memory && p < memory + size;
    }
    
    void reset() {
        used = 0;
        std::memset(memory, 0, size);
    }
};

// =============================================================================
// FreeList - For recycling small objects
// =============================================================================

struct FreeListNode {
    FreeListNode* next;
};

struct FreeList {
    FreeListNode* head;
    size_t objectSize;
    size_t freeCount;
    
    FreeList(size_t size) : head(nullptr), objectSize(size), freeCount(0) {}
    
    void* allocate() {
        if (!head) return nullptr;
        FreeListNode* node = head;
        head = head->next;
        freeCount--;
        return static_cast<void*>(node);
    }
    
    void deallocate(void* ptr) {
        FreeListNode* node = static_cast<FreeListNode*>(ptr);
        node->next = head;
        head = node;
        freeCount++;
    }
    
    bool empty() const {
        return head == nullptr;
    }
};

// =============================================================================
// LargeObjectInfo - Tracking for large allocations
// =============================================================================

struct LargeObjectInfo {
    void* ptr;
    size_t size;
    bool marked;
};

// =============================================================================
// Allocator - Main allocator implementation
// =============================================================================

class Allocator {
public:
    Allocator() {
        // Initialize free lists for each size class
        for (size_t i = 0; i < NUM_SIZE_CLASSES; ++i) {
            freeLists_.emplace_back(SIZE_CLASSES[i]);
        }
        
        // Create initial arena
        createArena();
    }
    
    ~Allocator() {
        // Free all arenas
        Arena* arena = firstArena_;
        while (arena) {
            Arena* next = arena->next;
            delete arena;
            arena = next;
        }
        
        // Free large objects
        for (auto& info : largeObjects_) {
            std::free(info.ptr);
        }
    }
    
    /**
     * @brief Allocate memory of given size
     */
    void* allocate(size_t size) {
        if (size == 0) size = 1;
        
        // Align size
        size = (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
        
        void* ptr = nullptr;
        
        if (size <= SMALL_OBJECT_THRESHOLD) {
            // Small object: try free list first
            size_t classIndex = getSizeClassIndex(size);
            ptr = freeLists_[classIndex].allocate();
            
            if (!ptr) {
                // Allocate from arena
                size_t allocSize = SIZE_CLASSES[classIndex];
                ptr = allocateFromArena(allocSize);
            }
            
            if (ptr) {
                stats_.smallAllocations++;
                stats_.smallBytesAllocated += size;
            }
        } else if (size <= LARGE_OBJECT_THRESHOLD) {
            // Medium object: bump allocate from arena
            ptr = allocateFromArena(size);
            
            if (ptr) {
                stats_.mediumAllocations++;
                stats_.mediumBytesAllocated += size;
            }
        } else {
            // Large object: direct allocation
            ptr = allocateLarge(size);
            
            if (ptr) {
                stats_.largeAllocations++;
                stats_.largeBytesAllocated += size;
            }
        }
        
        if (ptr) {
            stats_.totalAllocations++;
            stats_.totalBytesAllocated += size;
        }
        
        return ptr;
    }
    
    /**
     * @brief Return memory to free list (for small objects only)
     */
    void deallocate(void* ptr, size_t size) {
        if (!ptr) return;
        
        size = (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
        
        if (size <= SMALL_OBJECT_THRESHOLD) {
            size_t classIndex = getSizeClassIndex(size);
            freeLists_[classIndex].deallocate(ptr);
            stats_.smallBytesFreed += size;
        } else if (size > LARGE_OBJECT_THRESHOLD) {
            // Remove from large object list and free
            auto it = std::find_if(largeObjects_.begin(), largeObjects_.end(),
                [ptr](const LargeObjectInfo& info) { return info.ptr == ptr; });
            if (it != largeObjects_.end()) {
                std::free(it->ptr);
                stats_.largeBytesFreed += it->size;
                largeObjects_.erase(it);
            }
        }
        // Medium objects stay in arenas until arena is recycled
        
        stats_.totalBytesFreed += size;
    }
    
    /**
     * @brief Reset all arenas for reuse (after full GC)
     */
    void resetArenas() {
        Arena* arena = firstArena_;
        while (arena) {
            arena->reset();
            arena = arena->next;
        }
        currentArena_ = firstArena_;
        
        // Clear free lists
        for (auto& freeList : freeLists_) {
            freeList.head = nullptr;
            freeList.freeCount = 0;
        }
    }
    
    /**
     * @brief Check if pointer is within managed memory
     */
    bool contains(void* ptr) const {
        // Check arenas
        Arena* arena = firstArena_;
        while (arena) {
            if (arena->contains(ptr)) return true;
            arena = arena->next;
        }
        
        // Check large objects
        for (const auto& info : largeObjects_) {
            if (info.ptr == ptr) return true;
        }
        
        return false;
    }
    
    /**
     * @brief Get allocator statistics
     */
    struct Stats {
        size_t totalAllocations = 0;
        size_t totalBytesAllocated = 0;
        size_t totalBytesFreed = 0;
        size_t smallAllocations = 0;
        size_t smallBytesAllocated = 0;
        size_t smallBytesFreed = 0;
        size_t mediumAllocations = 0;
        size_t mediumBytesAllocated = 0;
        size_t largeAllocations = 0;
        size_t largeBytesAllocated = 0;
        size_t largeBytesFreed = 0;
        size_t arenaCount = 0;
        size_t arenaMemoryUsed = 0;
        size_t arenaMemoryTotal = 0;
    };
    
    Stats getStats() const {
        Stats result = stats_;
        
        // Count arenas
        Arena* arena = firstArena_;
        while (arena) {
            result.arenaCount++;
            result.arenaMemoryUsed += arena->used;
            result.arenaMemoryTotal += arena->size;
            arena = arena->next;
        }
        
        return result;
    }
    
    /**
     * @brief Get large object list for GC traversal
     */
    std::vector<LargeObjectInfo>& largeObjects() { return largeObjects_; }
    const std::vector<LargeObjectInfo>& largeObjects() const { return largeObjects_; }
    
private:
    /**
     * @brief Get size class index for a given size
     */
    size_t getSizeClassIndex(size_t size) const {
        for (size_t i = 0; i < NUM_SIZE_CLASSES; ++i) {
            if (SIZE_CLASSES[i] >= size) {
                return i;
            }
        }
        return NUM_SIZE_CLASSES - 1;
    }
    
    /**
     * @brief Allocate from current arena, creating new if needed
     */
    void* allocateFromArena(size_t size) {
        // Try current arena first
        if (currentArena_) {
            void* ptr = currentArena_->allocate(size);
            if (ptr) return ptr;
        }
        
        // Try other arenas
        Arena* arena = firstArena_;
        while (arena) {
            if (arena != currentArena_ && arena->remaining() >= size) {
                void* ptr = arena->allocate(size);
                if (ptr) {
                    currentArena_ = arena;
                    return ptr;
                }
            }
            arena = arena->next;
        }
        
        // Create new arena
        createArena();
        if (currentArena_) {
            return currentArena_->allocate(size);
        }
        
        return nullptr;
    }
    
    /**
     * @brief Allocate large object directly
     */
    void* allocateLarge(size_t size) {
        void* ptr = std::malloc(size);
        if (ptr) {
            std::memset(ptr, 0, size);
            largeObjects_.push_back({ptr, size, false});
        }
        return ptr;
    }
    
    /**
     * @brief Create a new arena
     */
    void createArena() {
        Arena* arena = new Arena(ARENA_SIZE);
        if (!arena->memory) {
            delete arena;
            return;
        }
        
        arena->next = firstArena_;
        firstArena_ = arena;
        currentArena_ = arena;
    }
    
    Arena* firstArena_ = nullptr;
    Arena* currentArena_ = nullptr;
    std::vector<FreeList> freeLists_;
    std::vector<LargeObjectInfo> largeObjects_;
    Stats stats_;
};

// =============================================================================
// Global Allocator Instance (for GC integration)
// =============================================================================

static Allocator* globalAllocator = nullptr;

void initializeAllocator() {
    if (!globalAllocator) {
        globalAllocator = new Allocator();
    }
}

void shutdownAllocator() {
    delete globalAllocator;
    globalAllocator = nullptr;
}

Allocator* getAllocator() {
    if (!globalAllocator) {
        initializeAllocator();
    }
    return globalAllocator;
}

} // namespace Zepra::GC
