// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_tlab.cpp — Thread-Local Allocation Buffers

#include <atomic>
#include <mutex>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <memory>
#include <functional>

namespace Zepra::Heap {

// Per-thread bump-pointer allocation region carved from the nursery.
// No synchronization on the fast path — each thread owns its TLAB.

struct TLABDescriptor {
    uintptr_t start;
    uintptr_t cursor;
    uintptr_t end;
    uint32_t threadId;
    uint64_t totalAllocated;
    uint64_t refills;

    TLABDescriptor()
        : start(0), cursor(0), end(0), threadId(0)
        , totalAllocated(0), refills(0) {}

    size_t remaining() const { return end > cursor ? end - cursor : 0; }
    size_t used() const { return cursor > start ? cursor - start : 0; }
    bool isEmpty() const { return cursor == start; }
};

class TLAB {
public:
    explicit TLAB(uint32_t threadId, size_t defaultSize = 32 * 1024)
        : defaultSize_(defaultSize) {
        desc_.threadId = threadId;
    }

    // Fast-path allocation — no locks, no atomics.
    uintptr_t allocate(size_t bytes) {
        bytes = (bytes + 7) & ~size_t(7);
        if (desc_.cursor + bytes > desc_.end) return 0;

        uintptr_t addr = desc_.cursor;
        desc_.cursor += bytes;
        desc_.totalAllocated += bytes;
        return addr;
    }

    // Called when TLAB is exhausted — gets a new chunk from the nursery.
    bool refill(uintptr_t newStart, size_t size) {
        desc_.start = newStart;
        desc_.cursor = newStart;
        desc_.end = newStart + size;
        desc_.refills++;
        return true;
    }

    void retire() {
        desc_.start = 0;
        desc_.cursor = 0;
        desc_.end = 0;
    }

    const TLABDescriptor& descriptor() const { return desc_; }
    size_t defaultSize() const { return defaultSize_; }

private:
    TLABDescriptor desc_;
    size_t defaultSize_;
};

// Manages TLABs for all mutator threads.
class TLABManager {
public:
    struct Config {
        size_t defaultTlabSize;
        size_t maxTlabSize;
        size_t minTlabSize;
        bool adaptiveSize;

        Config()
            : defaultTlabSize(32 * 1024)
            , maxTlabSize(256 * 1024)
            , minTlabSize(4 * 1024)
            , adaptiveSize(true) {}
    };

    explicit TLABManager(const Config& config = Config{})
        : config_(config) {}

    using RefillCallback = std::function<uintptr_t(size_t size)>;

    void setRefillCallback(RefillCallback cb) {
        refillCb_ = std::move(cb);
    }

    TLAB* createTLAB(uint32_t threadId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto tlab = std::make_unique<TLAB>(threadId, config_.defaultTlabSize);
        auto* ptr = tlab.get();
        tlabs_.push_back(std::move(tlab));
        return ptr;
    }

    bool refillTLAB(TLAB* tlab) {
        if (!refillCb_) return false;

        size_t size = computeTlabSize(tlab);
        uintptr_t chunk = refillCb_(size);
        if (chunk == 0) return false;

        return tlab->refill(chunk, size);
    }

    // Retire all TLABs before GC (push cursors back to nursery).
    void retireAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& tlab : tlabs_) {
            tlab->retire();
        }
    }

    size_t tlabCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return tlabs_.size();
    }

    struct Stats {
        uint64_t totalAllocated;
        uint64_t totalRefills;
    };

    Stats computeStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        Stats s{};
        for (auto& tlab : tlabs_) {
            s.totalAllocated += tlab->descriptor().totalAllocated;
            s.totalRefills += tlab->descriptor().refills;
        }
        return s;
    }

private:
    size_t computeTlabSize(TLAB* tlab) {
        if (!config_.adaptiveSize) return config_.defaultTlabSize;

        // Adaptive: fast threads get bigger TLABs.
        auto& d = tlab->descriptor();
        if (d.refills > 10 && d.totalAllocated / d.refills > config_.defaultTlabSize) {
            return std::min(config_.maxTlabSize,
                config_.defaultTlabSize * 2);
        }
        return config_.defaultTlabSize;
    }

    Config config_;
    mutable std::mutex mutex_;
    std::vector<std::unique_ptr<TLAB>> tlabs_;
    RefillCallback refillCb_;
};

} // namespace Zepra::Heap
