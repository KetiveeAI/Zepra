// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_pretenuring.cpp — Allocation-site feedback for direct old-gen allocation

#include <atomic>
#include <mutex>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <unordered_map>

namespace Zepra::Heap {

// Objects allocated at certain code sites consistently survive to old gen.
// Pretenuring skips the nursery for those sites, reducing minor GC work.
//
// The JIT records allocation-site metadata. After several GC cycles,
// sites with high survival rates are marked for old-gen allocation.

struct AllocationSiteInfo {
    uint64_t siteId;        // Bytecode offset or JIT code address
    uint64_t totalAllocs;
    uint64_t survived;      // Survived at least one scavenge
    uint64_t promoted;      // Promoted to old gen
    bool pretenure;         // Allocate directly in old gen

    AllocationSiteInfo()
        : siteId(0), totalAllocs(0), survived(0)
        , promoted(0), pretenure(false) {}

    double survivalRate() const {
        return totalAllocs > 0 ?
            static_cast<double>(survived) / totalAllocs : 0;
    }
};

class PretenureDecisionEngine {
public:
    struct Config {
        double survivalThreshold;   // Pretenure if survival rate exceeds this
        uint64_t minSamples;        // Minimum allocations before deciding
        uint64_t decayInterval;     // Reset counters every N GC cycles

        Config()
            : survivalThreshold(0.80)
            , minSamples(100)
            , decayInterval(10) {}
    };

    explicit PretenureDecisionEngine(const Config& config = Config{})
        : config_(config), gcCycles_(0) {}

    void recordAllocation(uint64_t siteId) {
        std::lock_guard<std::mutex> lock(mutex_);
        sites_[siteId].siteId = siteId;
        sites_[siteId].totalAllocs++;
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

    // Called after each GC to update decisions.
    void updateDecisions() {
        std::lock_guard<std::mutex> lock(mutex_);
        gcCycles_++;

        for (auto& [id, info] : sites_) {
            if (info.totalAllocs >= config_.minSamples) {
                info.pretenure = info.survivalRate() >= config_.survivalThreshold;
            }
        }

        // Periodic decay to adapt to phase changes.
        if (gcCycles_ % config_.decayInterval == 0) {
            for (auto& [id, info] : sites_) {
                info.totalAllocs /= 2;
                info.survived /= 2;
                info.promoted /= 2;
            }
        }
    }

    bool shouldPretenure(uint64_t siteId) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sites_.find(siteId);
        return it != sites_.end() && it->second.pretenure;
    }

    struct Summary {
        size_t trackedSites;
        size_t pretenuredSites;
        uint64_t totalAllocs;
    };

    Summary computeSummary() const {
        std::lock_guard<std::mutex> lock(mutex_);
        Summary s{};
        s.trackedSites = sites_.size();
        for (auto& [id, info] : sites_) {
            if (info.pretenure) s.pretenuredSites++;
            s.totalAllocs += info.totalAllocs;
        }
        return s;
    }

private:
    Config config_;
    mutable std::mutex mutex_;
    std::unordered_map<uint64_t, AllocationSiteInfo> sites_;
    uint64_t gcCycles_;
};

} // namespace Zepra::Heap
