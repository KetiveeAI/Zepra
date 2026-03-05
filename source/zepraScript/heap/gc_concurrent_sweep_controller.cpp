// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_concurrent_sweep_controller.cpp — Orchestrate concurrent sweep phases

#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <chrono>

namespace Zepra::Heap {

// After marking completes, sweeping reclaims unmarked objects.
// Concurrent sweeping runs alongside the mutator — pages are
// swept lazily or by background threads.

enum class SweepPhase : uint8_t {
    Idle,
    Preparing,       // Build page list, sort by liveness
    SweepingPages,   // Background threads sweep pages
    Finalizing,      // Run finalizers for dead objects
    ReturningPages,  // Return empty pages to segment manager
    Complete
};

class ConcurrentSweepController {
public:
    struct Backend {
        std::function<std::vector<uint64_t>()> getPagesToSweep;
        std::function<size_t(uint64_t pageId)> sweepPage;
        std::function<void(uint64_t pageId)> runFinalizers;
        std::function<void(uint64_t pageId)> returnPage;
        std::function<void(size_t freedBytes)> updateFreeList;
    };

    void setBackend(Backend b) { backend_ = std::move(b); }
    SweepPhase phase() const { return phase_; }

    void runSweepCycle() {
        auto start = std::chrono::steady_clock::now();

        // Phase 1: Get pages to sweep.
        phase_ = SweepPhase::Preparing;
        std::vector<uint64_t> pages;
        if (backend_.getPagesToSweep) pages = backend_.getPagesToSweep();

        // Phase 2: Sweep pages (can be parallel).
        phase_ = SweepPhase::SweepingPages;
        size_t totalFreed = 0;
        for (auto pageId : pages) {
            if (backend_.sweepPage) {
                totalFreed += backend_.sweepPage(pageId);
            }
            stats_.pagesSwept++;
        }

        // Phase 3: Run finalizers.
        phase_ = SweepPhase::Finalizing;
        for (auto pageId : pages) {
            if (backend_.runFinalizers) backend_.runFinalizers(pageId);
        }

        // Phase 4: Return empty pages.
        phase_ = SweepPhase::ReturningPages;
        if (backend_.updateFreeList) backend_.updateFreeList(totalFreed);

        phase_ = SweepPhase::Complete;
        stats_.cycleCount++;
        stats_.totalFreedBytes += totalFreed;
        stats_.lastCycleMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        phase_ = SweepPhase::Idle;
    }

    // Lazy sweep: sweep one page on allocation failure.
    size_t sweepOnePage() {
        if (!backend_.getPagesToSweep || !backend_.sweepPage) return 0;
        auto pages = backend_.getPagesToSweep();
        if (pages.empty()) return 0;
        size_t freed = backend_.sweepPage(pages[0]);
        stats_.pagesSwept++;
        stats_.totalFreedBytes += freed;
        return freed;
    }

    struct Stats {
        uint64_t cycleCount;
        uint64_t pagesSwept;
        uint64_t totalFreedBytes;
        double lastCycleMs;
    };

    const Stats& stats() const { return stats_; }

private:
    Backend backend_;
    SweepPhase phase_ = SweepPhase::Idle;
    Stats stats_{};
};

} // namespace Zepra::Heap
