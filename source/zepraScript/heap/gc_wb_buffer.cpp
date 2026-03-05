// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_wb_buffer.cpp — Write barrier buffer for batched processing

#include <atomic>
#include <mutex>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <functional>

namespace Zepra::Heap {

// Instead of processing each write barrier immediately (expensive
// during concurrent marking), buffer stores and process in batch
// at the next safe point or when the buffer fills.

class WriteBarrierBuffer {
public:
    static constexpr size_t BUFFER_CAPACITY = 1024;

    struct Entry {
        uintptr_t srcAddr;    // Object being written to
        uintptr_t oldValue;   // Previous reference (for SATB)
        uintptr_t newValue;   // New reference
    };

    WriteBarrierBuffer() : cursor_(0) {}

    WriteBarrierBuffer(const WriteBarrierBuffer& other)
        : cursor_(other.cursor_.load(std::memory_order_relaxed)) {
        std::memcpy(entries_, other.entries_, sizeof(entries_));
    }

    WriteBarrierBuffer& operator=(const WriteBarrierBuffer&) = delete;

    // Fast path: append to buffer. Returns true if buffer is full.
    bool record(uintptr_t src, uintptr_t oldVal, uintptr_t newVal) {
        size_t idx = cursor_.fetch_add(1, std::memory_order_relaxed);
        if (idx >= BUFFER_CAPACITY) {
            cursor_.store(BUFFER_CAPACITY, std::memory_order_relaxed);
            return true;  // Full — caller should flush.
        }
        entries_[idx] = {src, oldVal, newVal};
        return false;
    }

    // Drain the buffer, calling processor for each entry.
    size_t flush(std::function<void(uintptr_t src, uintptr_t oldVal,
                                      uintptr_t newVal)> processor) {
        size_t count = cursor_.exchange(0, std::memory_order_acq_rel);
        if (count > BUFFER_CAPACITY) count = BUFFER_CAPACITY;

        for (size_t i = 0; i < count; i++) {
            processor(entries_[i].srcAddr, entries_[i].oldValue,
                       entries_[i].newValue);
        }
        return count;
    }

    size_t pendingCount() const {
        size_t c = cursor_.load(std::memory_order_relaxed);
        return c > BUFFER_CAPACITY ? BUFFER_CAPACITY : c;
    }

    bool isFull() const {
        return cursor_.load(std::memory_order_relaxed) >= BUFFER_CAPACITY;
    }

    void reset() { cursor_.store(0, std::memory_order_relaxed); }

private:
    Entry entries_[BUFFER_CAPACITY];
    std::atomic<size_t> cursor_;
};

// Per-thread buffer with global drain.
class WriteBarrierBufferPool {
public:
    WriteBarrierBuffer* getBuffer(uint32_t threadIdx) {
        if (threadIdx >= buffers_.size()) return nullptr;
        return &buffers_[threadIdx];
    }

    void resize(size_t threadCount) {
        std::lock_guard<std::mutex> lock(mutex_);
        buffers_.resize(threadCount);
    }

    size_t flushAll(std::function<void(uintptr_t, uintptr_t, uintptr_t)> proc) {
        size_t total = 0;
        for (auto& buf : buffers_) {
            total += buf.flush(proc);
        }
        return total;
    }

private:
    std::mutex mutex_;
    std::vector<WriteBarrierBuffer> buffers_;
};

} // namespace Zepra::Heap
