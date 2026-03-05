// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_region_metadata.cpp — Per-region bookkeeping for GC

#include <atomic>
#include <mutex>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <algorithm>

namespace Zepra::Heap {

enum class RegionState : uint8_t {
    Free,
    Allocating,
    Full,
    PendingSweep,
    Sweeping,
    Compacting,
    Evacuating,
    Pinned
};

enum class RegionGeneration : uint8_t {
    Nursery,
    Young,
    Old,
    LargeObject,
    Code,
    ReadOnly
};

struct RegionMetadata {
    uintptr_t base;
    size_t size;
    RegionState state;
    RegionGeneration generation;
    uint64_t regionId;
    size_t liveBytes;
    size_t allocatedBytes;
    uint32_t survivedGCs;
    bool isEvacuationCandidate;
    bool hasPinnedObjects;
    uint64_t lastGCCycle;

    RegionMetadata()
        : base(0), size(0)
        , state(RegionState::Free)
        , generation(RegionGeneration::Old)
        , regionId(0), liveBytes(0), allocatedBytes(0)
        , survivedGCs(0), isEvacuationCandidate(false)
        , hasPinnedObjects(false), lastGCCycle(0) {}

    double fragmentation() const {
        return allocatedBytes > 0 ?
            1.0 - (static_cast<double>(liveBytes) / allocatedBytes) : 0;
    }

    double occupancy() const {
        return size > 0 ?
            static_cast<double>(allocatedBytes) / size : 0;
    }

    bool shouldEvacuate(double threshold) const {
        return !hasPinnedObjects && fragmentation() > threshold;
    }
};

// Tracks all regions in the heap.
class RegionMetadataTable {
public:
    uint64_t addRegion(uintptr_t base, size_t size,
                        RegionGeneration gen) {
        std::lock_guard<std::mutex> lock(mutex_);
        RegionMetadata meta;
        meta.base = base;
        meta.size = size;
        meta.generation = gen;
        meta.regionId = nextId_++;
        meta.state = RegionState::Allocating;
        regions_.push_back(meta);
        return meta.regionId;
    }

    void removeRegion(uint64_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        regions_.erase(
            std::remove_if(regions_.begin(), regions_.end(),
                [id](const RegionMetadata& r) { return r.regionId == id; }),
            regions_.end());
    }

    RegionMetadata* findRegion(uintptr_t addr) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& r : regions_) {
            if (addr >= r.base && addr < r.base + r.size) return &r;
        }
        return nullptr;
    }

    RegionMetadata* findById(uint64_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& r : regions_) {
            if (r.regionId == id) return &r;
        }
        return nullptr;
    }

    // Select regions for evacuation based on fragmentation.
    std::vector<uint64_t> selectEvacuationCandidates(
            double fragThreshold, size_t maxCandidates) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::pair<double, uint64_t>> candidates;

        for (auto& r : regions_) {
            if (r.generation == RegionGeneration::Old &&
                r.state == RegionState::Full &&
                !r.hasPinnedObjects &&
                r.fragmentation() > fragThreshold) {
                candidates.push_back({r.fragmentation(), r.regionId});
            }
        }

        // Sort by fragmentation descending — most fragmented first.
        std::sort(candidates.begin(), candidates.end(),
            [](const auto& a, const auto& b) {
                return a.first > b.first;
            });

        std::vector<uint64_t> result;
        for (size_t i = 0; i < std::min(candidates.size(), maxCandidates); i++) {
            result.push_back(candidates[i].second);
            for (auto& r : regions_) {
                if (r.regionId == candidates[i].second) {
                    r.isEvacuationCandidate = true;
                    break;
                }
            }
        }
        return result;
    }

    void updateLiveBytes(uint64_t id, size_t live) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& r : regions_) {
            if (r.regionId == id) { r.liveBytes = live; return; }
        }
    }

    void setState(uint64_t id, RegionState state) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& r : regions_) {
            if (r.regionId == id) { r.state = state; return; }
        }
    }

    struct Summary {
        size_t totalRegions;
        size_t totalBytes;
        size_t liveBytes;
        size_t freeRegions;
        double avgFragmentation;
    };

    Summary computeSummary() const {
        std::lock_guard<std::mutex> lock(mutex_);
        Summary s{};
        s.totalRegions = regions_.size();
        double fragSum = 0;
        size_t fragCount = 0;
        for (auto& r : regions_) {
            s.totalBytes += r.size;
            s.liveBytes += r.liveBytes;
            if (r.state == RegionState::Free) s.freeRegions++;
            if (r.allocatedBytes > 0) {
                fragSum += r.fragmentation();
                fragCount++;
            }
        }
        s.avgFragmentation = fragCount > 0 ? fragSum / fragCount : 0;
        return s;
    }

private:
    mutable std::mutex mutex_;
    std::vector<RegionMetadata> regions_;
    uint64_t nextId_ = 1;
};

} // namespace Zepra::Heap
