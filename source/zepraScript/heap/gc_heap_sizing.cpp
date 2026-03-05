// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_heap_sizing.cpp — Dynamic heap growth/shrink policy

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <algorithm>

namespace Zepra::Heap {

// After each major GC, decide whether to grow or shrink the heap.
// Goal: keep GC overhead (time in GC / total time) below target.
//
// If overhead is high → grow heap (fewer collections needed).
// If overhead is low → shrink heap (return memory to OS).

class HeapSizingPolicy {
public:
    struct Config {
        double targetOverhead;     // Target GC overhead ratio (default 5%)
        double growthFactor;       // Multiply capacity by this when growing
        double shrinkFactor;       // Multiply capacity by this when shrinking
        size_t minHeapSize;
        size_t maxHeapSize;
        size_t pageSize;           // Grow/shrink in page multiples

        Config()
            : targetOverhead(0.05)
            , growthFactor(1.5)
            , shrinkFactor(0.75)
            , minHeapSize(4 * 1024 * 1024)
            , maxHeapSize(512 * 1024 * 1024)
            , pageSize(256 * 1024) {}
    };

    explicit HeapSizingPolicy(const Config& config = Config{})
        : config_(config) {}

    struct GCResult {
        size_t liveBytes;
        size_t heapCapacity;
        double gcDurationMs;
        double mutatorDurationMs;
    };

    // Returns new recommended heap capacity.
    size_t recommend(const GCResult& result) {
        double overhead = 0;
        double total = result.gcDurationMs + result.mutatorDurationMs;
        if (total > 0) overhead = result.gcDurationMs / total;

        size_t newCapacity = result.heapCapacity;

        if (overhead > config_.targetOverhead * 1.5) {
            // High overhead → grow aggressively.
            newCapacity = static_cast<size_t>(
                result.heapCapacity * config_.growthFactor);
        } else if (overhead > config_.targetOverhead) {
            // Slightly above target → grow modestly.
            newCapacity = static_cast<size_t>(
                result.heapCapacity * 1.2);
        } else if (overhead < config_.targetOverhead * 0.25 &&
                   result.liveBytes < result.heapCapacity * 0.3) {
            // Very low overhead and low occupancy → shrink.
            newCapacity = static_cast<size_t>(
                result.heapCapacity * config_.shrinkFactor);
        }

        // Ensure we have enough room for live data + growth margin.
        size_t minRequired = static_cast<size_t>(result.liveBytes * 1.5);
        newCapacity = std::max(newCapacity, minRequired);

        // Clamp to bounds.
        newCapacity = std::max(newCapacity, config_.minHeapSize);
        newCapacity = std::min(newCapacity, config_.maxHeapSize);

        // Align to page boundary.
        newCapacity = alignUp(newCapacity, config_.pageSize);

        lastOverhead_ = overhead;
        lastRecommended_ = newCapacity;
        return newCapacity;
    }

    double lastOverhead() const { return lastOverhead_; }
    size_t lastRecommended() const { return lastRecommended_; }

private:
    static size_t alignUp(size_t v, size_t align) {
        return (v + align - 1) & ~(align - 1);
    }

    Config config_;
    double lastOverhead_ = 0;
    size_t lastRecommended_ = 0;
};

} // namespace Zepra::Heap
