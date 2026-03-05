// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_free_list.cpp — Segregated free list allocator for old-gen pages

#include <mutex>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Heap {

// Size classes: 16, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 2048, 4096
// Objects >4096 go to large-object space.

class FreeListAllocator {
public:
    static constexpr size_t NUM_CLASSES = 14;
    static constexpr size_t SIZE_CLASSES[NUM_CLASSES] = {
        16, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 2048, 4096
    };
    static constexpr size_t MAX_SMALL = SIZE_CLASSES[NUM_CLASSES - 1];

    struct FreeBlock {
        FreeBlock* next;
        size_t size;
    };

    FreeListAllocator() { heads_.fill(nullptr); }

    static size_t sizeClassIndex(size_t bytes) {
        for (size_t i = 0; i < NUM_CLASSES; i++) {
            if (bytes <= SIZE_CLASSES[i]) return i;
        }
        return NUM_CLASSES;
    }

    // Add freed memory to appropriate free list.
    void returnBlock(uintptr_t addr, size_t size) {
        size_t idx = sizeClassIndex(size);
        if (idx >= NUM_CLASSES) {
            addToOversize(addr, size);
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        auto* block = reinterpret_cast<FreeBlock*>(addr);
        block->size = size;
        block->next = heads_[idx];
        heads_[idx] = block;
        freeBytes_ += size;
        freeBlocks_++;
    }

    // Allocate from free list.
    uintptr_t allocate(size_t bytes) {
        size_t idx = sizeClassIndex(bytes);
        if (idx >= NUM_CLASSES) return 0;

        std::lock_guard<std::mutex> lock(mutex_);
        // Try exact class first.
        if (heads_[idx]) {
            auto* block = heads_[idx];
            heads_[idx] = block->next;
            freeBytes_ -= block->size;
            freeBlocks_--;
            return reinterpret_cast<uintptr_t>(block);
        }

        // Try next larger class.
        for (size_t i = idx + 1; i < NUM_CLASSES; i++) {
            if (heads_[i]) {
                auto* block = heads_[i];
                heads_[i] = block->next;
                size_t remaining = block->size - SIZE_CLASSES[idx];
                freeBytes_ -= block->size;
                freeBlocks_--;

                // Split: return remainder to appropriate list.
                if (remaining >= SIZE_CLASSES[0]) {
                    uintptr_t remainder = reinterpret_cast<uintptr_t>(block) + SIZE_CLASSES[idx];
                    returnBlockLocked(remainder, remaining);
                }

                return reinterpret_cast<uintptr_t>(block);
            }
        }
        return 0;
    }

    size_t freeBytes() const { return freeBytes_; }
    size_t freeBlocks() const { return freeBlocks_; }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        heads_.fill(nullptr);
        oversizeHead_ = nullptr;
        freeBytes_ = 0;
        freeBlocks_ = 0;
    }

private:
    void returnBlockLocked(uintptr_t addr, size_t size) {
        size_t idx = sizeClassIndex(size);
        if (idx >= NUM_CLASSES) return;
        auto* block = reinterpret_cast<FreeBlock*>(addr);
        block->size = size;
        block->next = heads_[idx];
        heads_[idx] = block;
        freeBytes_ += size;
        freeBlocks_++;
    }

    void addToOversize(uintptr_t addr, size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto* block = reinterpret_cast<FreeBlock*>(addr);
        block->size = size;
        block->next = oversizeHead_;
        oversizeHead_ = block;
        freeBytes_ += size;
        freeBlocks_++;
    }

    std::mutex mutex_;
    std::array<FreeBlock*, NUM_CLASSES> heads_;
    FreeBlock* oversizeHead_ = nullptr;
    size_t freeBytes_ = 0;
    size_t freeBlocks_ = 0;
};

} // namespace Zepra::Heap
