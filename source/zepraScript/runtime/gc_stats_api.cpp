// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_stats_api.cpp — Runtime API exposing GC stats to JS

#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <string>
#include <chrono>

namespace Zepra::Runtime {

// Exposes GC performance data to JavaScript via performance.gc()
// and the DevTools Memory panel. All data is read-only from JS side.

struct GCStatsSnapshot {
    uint64_t minorCollections;
    uint64_t majorCollections;
    double totalPauseMs;
    double lastPauseMs;
    double avgPauseMs;
    double maxPauseMs;
    size_t heapUsed;
    size_t heapCapacity;
    size_t externalMemory;
    double allocationRateMBps;
    double survivalRate;
    size_t nurseryUsed;
    size_t nurseryCapacity;
    size_t oldSpaceUsed;
    size_t oldSpaceCapacity;
    size_t largeObjectBytes;
    uint64_t promotedBytes;
};

class GCStatsAPI {
public:
    using SnapshotProvider = std::function<GCStatsSnapshot()>;

    void setProvider(SnapshotProvider provider) {
        provider_ = std::move(provider);
    }

    GCStatsSnapshot getStats() const {
        if (provider_) return provider_();
        return {};
    }

    // performance.measureUserAgentSpecificMemory() equivalent.
    struct MemoryMeasurement {
        size_t jsHeapUsed;
        size_t jsHeapCapacity;
        size_t domNodeCount;    // If browser integration present
        size_t externalBytes;
    };

    using MemoryProvider = std::function<MemoryMeasurement()>;

    void setMemoryProvider(MemoryProvider provider) {
        memProvider_ = std::move(provider);
    }

    MemoryMeasurement measureMemory() const {
        if (memProvider_) return memProvider_();
        return {};
    }

    // GC notification callback for performance observers.
    using GCNotifyFn = std::function<void(const GCStatsSnapshot&)>;

    uint32_t addObserver(GCNotifyFn fn) {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t id = nextId_++;
        observers_.push_back({id, std::move(fn)});
        return id;
    }

    void removeObserver(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        observers_.erase(
            std::remove_if(observers_.begin(), observers_.end(),
                [id](const auto& o) { return o.first == id; }),
            observers_.end());
    }

    // Called by GC after each collection.
    void notifyObservers(const GCStatsSnapshot& snapshot) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [id, fn] : observers_) {
            try { fn(snapshot); } catch (...) {}
        }
    }

    // gc() builtin — force GC from JS (only in debug builds).
    using ForceGCFn = std::function<void()>;
    void setForceGC(ForceGCFn fn) { forceGC_ = std::move(fn); }
    void forceGC() { if (forceGC_) forceGC_(); }

private:
    SnapshotProvider provider_;
    MemoryProvider memProvider_;
    ForceGCFn forceGC_;
    std::mutex mutex_;
    std::vector<std::pair<uint32_t, GCNotifyFn>> observers_;
    uint32_t nextId_ = 1;
};

} // namespace Zepra::Runtime
