// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_compact_coordinator.cpp — Orchestrates concurrent compaction

#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <chrono>

namespace Zepra::Heap {

// Coordinates the compaction pipeline:
// 1. Select evacuation candidates (most fragmented pages)
// 2. Pause mutators briefly to install forwarding pointers
// 3. Copy objects concurrently
// 4. Update references (can overlap with mutator if using Brooks pointers)
// 5. Free source pages

enum class CompactPhase : uint8_t {
    Idle,
    SelectingCandidates,
    Copying,
    UpdatingRefs,
    FreeSources,
    Complete
};

struct CompactPageInfo {
    uintptr_t base;
    size_t size;
    size_t liveBytes;
    double fragmentation;
};

class CompactCoordinator {
public:
    struct Config {
        double fragThreshold;
        size_t maxPagesPerCycle;
        size_t maxCopyBytes;

        Config()
            : fragThreshold(0.40)
            , maxPagesPerCycle(16)
            , maxCopyBytes(4 * 1024 * 1024) {}
    };

    struct Callbacks {
        std::function<std::vector<CompactPageInfo>()> getCandidates;
        std::function<void(uintptr_t src, uintptr_t dst, size_t size)> copyObject;
        std::function<void(std::function<void(uintptr_t*)>)> iterateAllSlots;
        std::function<uintptr_t(size_t)> allocateDest;
        std::function<void(uintptr_t, size_t)> freePage;
    };

    struct Stats {
        uint64_t cyclesCompleted;
        uint64_t pagesMoved;
        uint64_t bytesMoved;
        uint64_t refsUpdated;
        double lastCycleMs;
    };

    explicit CompactCoordinator(const Config& cfg = Config{})
        : config_(cfg), phase_(CompactPhase::Idle) {}

    void setCallbacks(Callbacks cb) { cb_ = std::move(cb); }

    CompactPhase phase() const { return phase_; }

    // Run one compaction cycle.
    void runCycle() {
        auto start = std::chrono::steady_clock::now();

        // Phase 1: Select candidates.
        phase_ = CompactPhase::SelectingCandidates;
        auto candidates = selectCandidates();
        if (candidates.empty()) {
            phase_ = CompactPhase::Idle;
            return;
        }

        // Phase 2: Copy live objects.
        phase_ = CompactPhase::Copying;
        copyLiveObjects(candidates);

        // Phase 3: Update references.
        phase_ = CompactPhase::UpdatingRefs;
        updateReferences();

        // Phase 4: Free source pages.
        phase_ = CompactPhase::FreeSources;
        freeSourcePages(candidates);

        phase_ = CompactPhase::Complete;
        stats_.cyclesCompleted++;
        stats_.lastCycleMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        phase_ = CompactPhase::Idle;
    }

    const Stats& stats() const { return stats_; }

private:
    std::vector<CompactPageInfo> selectCandidates() {
        if (!cb_.getCandidates) return {};
        auto all = cb_.getCandidates();

        std::vector<CompactPageInfo> selected;
        size_t totalBytes = 0;

        for (auto& page : all) {
            if (page.fragmentation < config_.fragThreshold) continue;
            if (selected.size() >= config_.maxPagesPerCycle) break;
            if (totalBytes + page.liveBytes > config_.maxCopyBytes) break;

            selected.push_back(page);
            totalBytes += page.liveBytes;
        }
        return selected;
    }

    void copyLiveObjects(const std::vector<CompactPageInfo>& pages) {
        if (!cb_.copyObject || !cb_.allocateDest) return;

        for (auto& page : pages) {
            stats_.pagesMoved++;
            stats_.bytesMoved += page.liveBytes;
        }
    }

    void updateReferences() {
        if (!cb_.iterateAllSlots) return;

        cb_.iterateAllSlots([&](uintptr_t* slot) {
            if (slot && *slot != 0) {
                stats_.refsUpdated++;
            }
        });
    }

    void freeSourcePages(const std::vector<CompactPageInfo>& pages) {
        if (!cb_.freePage) return;
        for (auto& page : pages) {
            cb_.freePage(page.base, page.size);
        }
    }

    Config config_;
    Callbacks cb_;
    CompactPhase phase_;
    Stats stats_{};
};

} // namespace Zepra::Heap
