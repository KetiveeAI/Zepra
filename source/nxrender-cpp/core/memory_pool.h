// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file memory_pool.h
 * @brief Arena allocator and typed pool allocator for per-frame transient allocations.
 *
 * Hot path allocations (display list entries, temp layout buffers, etc.) go through
 * ArenaAllocator to avoid per-object heap allocation. The arena is bulk-reset at
 * frame boundaries.
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <cassert>
#include <new>
#include <type_traits>

namespace NXRender {

/**
 * @brief Bump-pointer arena allocator with bulk reset.
 *
 * Allocates from a contiguous memory block. Individual deallocations are not
 * supported — call reset() to free all allocations at once.
 *
 * Thread-safety: NOT thread-safe. Each thread should own its own arena.
 */
class ArenaAllocator {
public:
    static constexpr size_t kDefaultBlockSize = 64 * 1024; // 64 KB

    explicit ArenaAllocator(size_t blockSize = kDefaultBlockSize)
        : blockSize_(blockSize) {
        allocateBlock(blockSize_);
    }

    ~ArenaAllocator() {
        for (auto& block : blocks_) {
            ::operator delete(block.data);
        }
    }

    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    ArenaAllocator(ArenaAllocator&& other) noexcept
        : blocks_(std::move(other.blocks_))
        , currentBlockIdx_(other.currentBlockIdx_)
        , blockSize_(other.blockSize_)
        , totalAllocated_(other.totalAllocated_)
        , peakAllocated_(other.peakAllocated_) {
        other.currentBlockIdx_ = 0;
        other.totalAllocated_ = 0;
    }

    /**
     * @brief Allocate `size` bytes with `alignment` alignment.
     * @return Pointer to allocated memory. Never null (asserts on OOM).
     */
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        assert(size > 0);
        assert(alignment > 0 && (alignment & (alignment - 1)) == 0); // power of 2

        Block& block = blocks_[currentBlockIdx_];
        uintptr_t current = reinterpret_cast<uintptr_t>(block.data) + block.used;
        uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
        size_t padding = aligned - current;
        size_t totalNeeded = padding + size;

        if (block.used + totalNeeded <= block.capacity) {
            block.used += totalNeeded;
            totalAllocated_ += totalNeeded;
            if (totalAllocated_ > peakAllocated_) peakAllocated_ = totalAllocated_;
            return reinterpret_cast<void*>(aligned);
        }

        // Current block full — try next existing block or allocate new one
        size_t newBlockSize = size + alignment > blockSize_ ? size + alignment : blockSize_;
        if (currentBlockIdx_ + 1 < blocks_.size()) {
            // Reuse a previously allocated block (from a prior frame)
            currentBlockIdx_++;
            blocks_[currentBlockIdx_].used = 0;
        } else {
            allocateBlock(newBlockSize);
            currentBlockIdx_ = blocks_.size() - 1;
        }

        return allocate(size, alignment); // Recurse once into the fresh block
    }

    /**
     * @brief Allocate and construct an object of type T.
     */
    template <typename T, typename... Args>
    T* create(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

    /**
     * @brief Allocate an array of `count` T objects (default-constructed).
     */
    template <typename T>
    T* allocateArray(size_t count) {
        static_assert(std::is_trivially_destructible<T>::value,
                      "ArenaAllocator array elements must be trivially destructible");
        void* mem = allocate(sizeof(T) * count, alignof(T));
        T* arr = reinterpret_cast<T*>(mem);
        for (size_t i = 0; i < count; i++) {
            new (&arr[i]) T();
        }
        return arr;
    }

    /**
     * @brief Reset all allocations. Does NOT free memory — retains blocks for reuse.
     */
    void reset() {
        for (auto& block : blocks_) {
            block.used = 0;
        }
        currentBlockIdx_ = 0;
        totalAllocated_ = 0;
    }

    /**
     * @brief Free all blocks and release memory back to the OS.
     */
    void releaseAll() {
        for (auto& block : blocks_) {
            ::operator delete(block.data);
        }
        blocks_.clear();
        currentBlockIdx_ = 0;
        totalAllocated_ = 0;
        allocateBlock(blockSize_);
    }

    // Stats
    size_t totalAllocated() const { return totalAllocated_; }
    size_t peakAllocated() const { return peakAllocated_; }
    size_t blockCount() const { return blocks_.size(); }

    size_t totalCapacity() const {
        size_t total = 0;
        for (const auto& b : blocks_) total += b.capacity;
        return total;
    }

private:
    struct Block {
        void* data;
        size_t capacity;
        size_t used;
    };

    void allocateBlock(size_t capacity) {
        void* data = ::operator new(capacity);
        blocks_.push_back({data, capacity, 0});
    }

    std::vector<Block> blocks_;
    size_t currentBlockIdx_ = 0;
    size_t blockSize_;
    size_t totalAllocated_ = 0;
    size_t peakAllocated_ = 0;
};

/**
 * @brief Typed pool allocator backed by ArenaAllocator.
 *
 * Provides STL-compatible allocator interface for containers that need
 * arena-backed allocation.
 */
template <typename T>
class PoolAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = size_t;

    explicit PoolAllocator(ArenaAllocator& arena) noexcept : arena_(&arena) {}

    template <typename U>
    PoolAllocator(const PoolAllocator<U>& other) noexcept : arena_(other.arena_) {}

    T* allocate(size_t n) {
        return reinterpret_cast<T*>(arena_->allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T*, size_t) noexcept {
        // Arena doesn't support individual deallocation
    }

    template <typename U>
    bool operator==(const PoolAllocator<U>& other) const noexcept {
        return arena_ == other.arena_;
    }

    template <typename U>
    bool operator!=(const PoolAllocator<U>& other) const noexcept {
        return arena_ != other.arena_;
    }

    ArenaAllocator* arena_;
};

/**
 * @brief Fixed-size object pool for frequently allocated/freed objects.
 *
 * Maintains a free list of recycled objects. Objects are stored in contiguous
 * chunks for cache friendliness.
 */
template <typename T>
class ObjectPool {
public:
    static constexpr size_t kChunkSize = 64;

    ObjectPool() = default;

    ~ObjectPool() {
        for (auto* chunk : chunks_) {
            ::operator delete(static_cast<void*>(chunk));
        }
    }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    T* acquire() {
        if (freeList_) {
            FreeNode* node = freeList_;
            freeList_ = node->next;
            freeCount_--;
            return reinterpret_cast<T*>(node);
        }

        // Free list empty — allocate from current chunk
        if (chunkOffset_ >= kChunkSize) {
            allocateChunk();
        }

        T* obj = &chunks_.back()[chunkOffset_++];
        totalAllocated_++;
        return obj;
    }

    void release(T* obj) {
        obj->~T();
        FreeNode* node = reinterpret_cast<FreeNode*>(obj);
        node->next = freeList_;
        freeList_ = node;
        freeCount_++;
    }

    size_t totalAllocated() const { return totalAllocated_; }
    size_t freeCount() const { return freeCount_; }
    size_t activeCount() const { return totalAllocated_ - freeCount_; }

private:
    struct FreeNode { FreeNode* next; };

    void allocateChunk() {
        static_assert(sizeof(T) >= sizeof(FreeNode),
                      "Pool element must be at least pointer-sized");
        T* chunk = reinterpret_cast<T*>(::operator new(sizeof(T) * kChunkSize));
        chunks_.push_back(chunk);
        chunkOffset_ = 0;
    }

    std::vector<T*> chunks_;
    size_t chunkOffset_ = kChunkSize; // Force first allocation to create a chunk
    FreeNode* freeList_ = nullptr;
    size_t totalAllocated_ = 0;
    size_t freeCount_ = 0;
};

} // namespace NXRender
