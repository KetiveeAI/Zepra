// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_heap_statistics.cpp — Per-generation live/dead/fragmentation stats

#include <mutex>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cmath>

namespace Zepra::Heap {

// Collected after each GC cycle. Used by:
// - Heap sizing policy (grow/shrink decisions)
// - DevTools memory panel
// - GC telemetry

struct GenerationStats {
    size_t capacity;
    size_t used;
    size_t liveBytes;
    size_t deadBytes;
    size_t freeListBytes;
    uint64_t objectCount;
    uint64_t deadCount;

    double occupancy() const { return capacity > 0 ? static_cast<double>(used) / capacity : 0; }
    double fragmentation() const {
        return capacity > 0 ? static_cast<double>(freeListBytes) / capacity : 0;
    }
    double liveRatio() const { return used > 0 ? static_cast<double>(liveBytes) / used : 0; }
};

class HeapStatistics {
public:
    void updateNursery(const GenerationStats& s) {
        std::lock_guard<std::mutex> lock(mutex_);
        nursery_ = s;
    }

    void updateOldGen(const GenerationStats& s) {
        std::lock_guard<std::mutex> lock(mutex_);
        oldGen_ = s;
    }

    void updateLargeObject(const GenerationStats& s) {
        std::lock_guard<std::mutex> lock(mutex_);
        largeObject_ = s;
    }

    GenerationStats nursery() const { std::lock_guard<std::mutex> lock(mutex_); return nursery_; }
    GenerationStats oldGen() const { std::lock_guard<std::mutex> lock(mutex_); return oldGen_; }
    GenerationStats largeObject() const { std::lock_guard<std::mutex> lock(mutex_); return largeObject_; }

    // Aggregate stats.
    size_t totalCapacity() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return nursery_.capacity + oldGen_.capacity + largeObject_.capacity;
    }

    size_t totalUsed() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return nursery_.used + oldGen_.used + largeObject_.used;
    }

    size_t totalLiveBytes() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return nursery_.liveBytes + oldGen_.liveBytes + largeObject_.liveBytes;
    }

    double overallOccupancy() const {
        size_t cap = totalCapacity();
        return cap > 0 ? static_cast<double>(totalUsed()) / cap : 0;
    }

    // Dump to stderr.
    void dump() const {
        std::lock_guard<std::mutex> lock(mutex_);
        fprintf(stderr, "\n=== Heap Statistics ===\n");
        dumpGen("Nursery", nursery_);
        dumpGen("OldGen", oldGen_);
        dumpGen("LargeObj", largeObject_);
        fprintf(stderr, "  Total: %zuKB / %zuKB (%.1f%%)\n\n",
            (nursery_.used + oldGen_.used + largeObject_.used) / 1024,
            (nursery_.capacity + oldGen_.capacity + largeObject_.capacity) / 1024,
            overallOccupancy() * 100);
    }

private:
    static void dumpGen(const char* name, const GenerationStats& s) {
        fprintf(stderr, "  %s: %zuKB/%zuKB live=%zuKB frag=%.1f%%\n",
            name, s.used / 1024, s.capacity / 1024,
            s.liveBytes / 1024, s.fragmentation() * 100);
    }

    mutable std::mutex mutex_;
    GenerationStats nursery_{};
    GenerationStats oldGen_{};
    GenerationStats largeObject_{};
};

} // namespace Zepra::Heap
