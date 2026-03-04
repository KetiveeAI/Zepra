/**
 * @file arena_allocator.cpp
 * @brief Arena allocator implementation
 */

#include "memory/arena_allocator.hpp"
#include <cstring>

namespace Zepra::Memory {

ArenaAllocator::ArenaAllocator(size_t blockSize) : blockSize_(blockSize) {
    allocateBlock();
}

ArenaAllocator::~ArenaAllocator() = default;

ArenaAllocator::ArenaAllocator(ArenaAllocator&&) noexcept = default;
ArenaAllocator& ArenaAllocator::operator=(ArenaAllocator&&) noexcept = default;

void ArenaAllocator::allocateBlock() {
    Block block;
    block.data = std::make_unique<uint8_t[]>(blockSize_);
    block.size = blockSize_;
    block.used = 0;
    capacity_ += blockSize_;
    blocks_.push_back(std::move(block));
}

void* ArenaAllocator::allocate(size_t size, size_t alignment) {
    if (blocks_.empty()) allocateBlock();
    
    Block& current = blocks_.back();
    
    // Align pointer
    size_t ptr = reinterpret_cast<size_t>(current.data.get() + current.used);
    size_t aligned = (ptr + alignment - 1) & ~(alignment - 1);
    size_t padding = aligned - ptr;
    
    size_t totalSize = size + padding;
    
    // Check if fits in current block
    if (current.used + totalSize > current.size) {
        // Need new block
        allocateBlock();
        return allocate(size, alignment);
    }
    
    void* result = current.data.get() + current.used + padding;
    current.used += totalSize;
    bytesUsed_ += totalSize;
    
    return result;
}

void* ArenaAllocator::allocateZeroed(size_t size, size_t alignment) {
    void* ptr = allocate(size, alignment);
    std::memset(ptr, 0, size);
    return ptr;
}

void ArenaAllocator::reset() {
    for (auto& block : blocks_) {
        block.used = 0;
    }
    bytesUsed_ = 0;
    
    // Keep only first block
    if (blocks_.size() > 1) {
        Block first = std::move(blocks_[0]);
        blocks_.clear();
        blocks_.push_back(std::move(first));
        capacity_ = blockSize_;
    }
}

// =============================================================================
// ScopedArena
// =============================================================================

ScopedArena::ScopedArena(ArenaAllocator& arena) 
    : arena_(arena), mark_(arena.bytesUsed()) {}

ScopedArena::~ScopedArena() {
    // Note: Can't actually free in simple arena
    // This is a marker for future optimization
    (void)mark_;
}

void* ScopedArena::allocate(size_t size, size_t alignment) {
    return arena_.allocate(size, alignment);
}

} // namespace Zepra::Memory
