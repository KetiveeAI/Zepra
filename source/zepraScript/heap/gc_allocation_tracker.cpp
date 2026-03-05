// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_allocation_tracker.cpp — Per-site allocation tracking for profiling

#include <mutex>
#include <unordered_map>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <algorithm>

namespace Zepra::Heap {

// Tracks allocation patterns per bytecode site. Used by:
// - Pretenuring (survival rate → old-gen allocation)
// - JIT (hot allocation sites → inline allocation)
// - DevTools (allocation profiling timeline)

struct AllocationSiteStats {
    uint64_t siteId;
    uint64_t count;
    uint64_t totalBytes;
    uint64_t survived;       // After latest GC
    uint64_t promoted;
    uint8_t lastTypeTag;
    uint16_t lastSizeClass;

    AllocationSiteStats()
        : siteId(0), count(0), totalBytes(0), survived(0)
        , promoted(0), lastTypeTag(0), lastSizeClass(0) {}

    double survivalRate() const {
        return count > 0 ? static_cast<double>(survived) / count : 0;
    }

    size_t avgSize() const {
        return count > 0 ? totalBytes / count : 0;
    }
};

class AllocationTracker {
public:
    void recordAllocation(uint64_t siteId, size_t bytes, uint8_t typeTag) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& s = sites_[siteId];
        s.siteId = siteId;
        s.count++;
        s.totalBytes += bytes;
        s.lastTypeTag = typeTag;
        s.lastSizeClass = static_cast<uint16_t>(bytes / 8);
        totalAllocated_ += bytes;
    }

    void recordSurvival(uint64_t siteId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sites_.find(siteId);
        if (it != sites_.end()) it->second.survived++;
    }

    void recordPromotion(uint64_t siteId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sites_.find(siteId);
        if (it != sites_.end()) it->second.promoted++;
    }

    // Get top N allocation sites by byte volume.
    std::vector<AllocationSiteStats> topSites(size_t n) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<AllocationSiteStats> sorted;
        for (auto& [id, s] : sites_) sorted.push_back(s);

        std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) {
                return a.totalBytes > b.totalBytes;
            });

        if (sorted.size() > n) sorted.resize(n);
        return sorted;
    }

    // Get sites eligible for pretenuring.
    std::vector<uint64_t> pretenureCandidates(double threshold, uint64_t minSamples) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<uint64_t> result;
        for (auto& [id, s] : sites_) {
            if (s.count >= minSamples && s.survivalRate() >= threshold) {
                result.push_back(id);
            }
        }
        return result;
    }

    void decayCounters() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [id, s] : sites_) {
            s.count /= 2;
            s.totalBytes /= 2;
            s.survived /= 2;
            s.promoted /= 2;
        }
    }

    size_t siteCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sites_.size();
    }

    uint64_t totalAllocated() const { return totalAllocated_; }

private:
    mutable std::mutex mutex_;
    std::unordered_map<uint64_t, AllocationSiteStats> sites_;
    uint64_t totalAllocated_ = 0;
};

} // namespace Zepra::Heap
