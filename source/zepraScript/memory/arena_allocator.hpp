#pragma once

/**
 * @file arena_allocator.hpp
 * @brief Arena allocator for fast temporary allocations
 */

#include "../config.hpp"
#include <cstddef>
#include <vector>
#include <memory>

namespace Zepra::Memory {

/**
 * @brief Arena allocator for fast bump-pointer allocation
 * 
 * Allocates memory linearly from large blocks. All memory is
 * freed at once when the arena is reset or destroyed.
 */
class ArenaAllocator {
public:
    explicit ArenaAllocator(size_t blockSize = 64 * 1024);
    ~ArenaAllocator();
    
    // Non-copyable
    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;
    
    // Movable
    ArenaAllocator(ArenaAllocator&&) noexcept;
    ArenaAllocator& operator=(ArenaAllocator&&) noexcept;
    
    /**
     * @brief Allocate memory from arena
     */
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t));
    
    /**
     * @brief Allocate and zero-initialize
     */
    void* allocateZeroed(size_t size, size_t alignment = alignof(std::max_align_t));
    
    /**
     * @brief Reset arena, freeing all allocations
     */
    void reset();
    
    /**
     * @brief Get total bytes allocated
     */
    size_t bytesUsed() const { return bytesUsed_; }
    
    /**
     * @brief Get total capacity
     */
    size_t capacity() const { return capacity_; }
    
    /**
     * @brief Typed allocation helper
     */
    template<typename T, typename... Args>
    T* create(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }
    
private:
    struct Block {
        std::unique_ptr<uint8_t[]> data;
        size_t size;
        size_t used;
    };
    
    void allocateBlock();
    
    std::vector<Block> blocks_;
    size_t blockSize_;
    size_t bytesUsed_ = 0;
    size_t capacity_ = 0;
};

/**
 * @brief Scoped arena for RAII-style temporary allocations
 */
class ScopedArena {
public:
    explicit ScopedArena(ArenaAllocator& arena);
    ~ScopedArena();
    
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t));
    
    template<typename T, typename... Args>
    T* create(Args&&... args) {
        return arena_.create<T>(std::forward<Args>(args)...);
    }
    
private:
    ArenaAllocator& arena_;
    size_t mark_;
};

} // namespace Zepra::Memory
