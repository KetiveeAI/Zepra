// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_event_log.cpp — Runtime GC event log for diagnostics

#include <mutex>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <chrono>
#include <cstring>

namespace Zepra::Runtime {

// Ring buffer of GC events for post-mortem analysis and DevTools
// integration. Keeps the last N events without unbounded growth.

enum class GCEventType : uint8_t {
    MinorStart,
    MinorEnd,
    MajorStart,
    MajorEnd,
    IncrementalStep,
    CompactionStart,
    CompactionEnd,
    AllocationFailure,
    HeapGrow,
    HeapShrink,
    OOM,
    SafepointEnter,
    SafepointExit
};

struct GCEvent {
    GCEventType type;
    uint64_t timestampUs;
    size_t heapUsed;
    size_t heapCapacity;
    double durationMs;
    uint32_t extra;       // Type-specific data

    GCEvent()
        : type(GCEventType::MinorStart), timestampUs(0)
        , heapUsed(0), heapCapacity(0), durationMs(0), extra(0) {}
};

class GCEventLog {
public:
    static constexpr size_t DEFAULT_CAPACITY = 512;

    explicit GCEventLog(size_t capacity = DEFAULT_CAPACITY)
        : capacity_(capacity), cursor_(0) {
        events_.resize(capacity);
    }

    void record(GCEventType type, size_t heapUsed, size_t heapCap,
                 double durationMs = 0, uint32_t extra = 0) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& evt = events_[cursor_ % capacity_];
        evt.type = type;
        evt.timestampUs = now();
        evt.heapUsed = heapUsed;
        evt.heapCapacity = heapCap;
        evt.durationMs = durationMs;
        evt.extra = extra;
        cursor_++;
    }

    // Get the last N events (oldest first).
    std::vector<GCEvent> recentEvents(size_t count) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<GCEvent> result;
        size_t total = cursor_ < capacity_ ? cursor_ : capacity_;
        if (count > total) count = total;

        size_t start = cursor_ >= count ? cursor_ - count : 0;
        for (size_t i = start; i < cursor_; i++) {
            result.push_back(events_[i % capacity_]);
        }
        return result;
    }

    size_t totalEvents() const { return cursor_; }

    // Dump to stderr for debugging.
    void dump(size_t count = 20) const {
        auto events = recentEvents(count);
        fprintf(stderr, "\n=== GC Event Log (last %zu) ===\n", events.size());
        for (auto& e : events) {
            fprintf(stderr, "  [%llu] type=%u heap=%zuKB/%zuKB dur=%.2fms\n",
                static_cast<unsigned long long>(e.timestampUs),
                static_cast<unsigned>(e.type),
                e.heapUsed / 1024, e.heapCapacity / 1024, e.durationMs);
        }
    }

private:
    static uint64_t now() {
        auto t = std::chrono::steady_clock::now().time_since_epoch();
        return std::chrono::duration_cast<std::chrono::microseconds>(t).count();
    }

    mutable std::mutex mutex_;
    std::vector<GCEvent> events_;
    size_t capacity_;
    size_t cursor_;
};

} // namespace Zepra::Runtime
