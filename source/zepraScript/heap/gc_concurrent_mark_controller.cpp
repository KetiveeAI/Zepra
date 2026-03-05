// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_concurrent_mark_controller.cpp — Orchestrate concurrent marking phases

#include <atomic>
#include <mutex>
#include <functional>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <chrono>

namespace Zepra::Heap {

// Concurrent marking runs alongside the mutator. Three phases:
// 1. Root scan (brief STW pause)
// 2. Concurrent tracing (mutator runs, write barrier records changes)
// 3. Remark (brief STW pause to process barrier buffer + re-scan dirty cards)

enum class MarkPhase : uint8_t {
    Idle,
    RootScan,      // STW
    Tracing,       // Concurrent
    Remark,        // STW
    Complete
};

class ConcurrentMarkController {
public:
    struct Backend {
        // Pause mutators.
        std::function<void()> enterSTW;
        // Resume mutators.
        std::function<void()> exitSTW;
        // Scan roots into grey set.
        std::function<void(std::function<void(uintptr_t)>)> scanRoots;
        // Trace from grey set (concurrent phase).
        std::function<size_t()> traceGrey;
        // Flush barrier buffers and process.
        std::function<size_t()> processBarriers;
        // Rescan dirty cards (remark phase).
        std::function<size_t()> rescanDirtyCards;
        // Enable/disable write barrier SATB mode.
        std::function<void(bool)> setSATBMode;
        // Notify marking complete.
        std::function<void()> markingDone;
    };

    void setBackend(Backend b) { backend_ = std::move(b); }

    MarkPhase phase() const { return phase_; }

    // Run the full concurrent marking cycle.
    void runMarkCycle() {
        auto start = std::chrono::steady_clock::now();

        // Phase 1: Root scan (STW).
        phase_ = MarkPhase::RootScan;
        if (backend_.enterSTW) backend_.enterSTW();
        if (backend_.setSATBMode) backend_.setSATBMode(true);

        size_t rootCount = 0;
        if (backend_.scanRoots) {
            backend_.scanRoots([&](uintptr_t) { rootCount++; });
        }

        if (backend_.exitSTW) backend_.exitSTW();

        // Phase 2: Concurrent tracing.
        phase_ = MarkPhase::Tracing;
        size_t traced = 0;
        if (backend_.traceGrey) {
            traced = backend_.traceGrey();
        }

        // Phase 3: Remark (STW).
        phase_ = MarkPhase::Remark;
        if (backend_.enterSTW) backend_.enterSTW();

        size_t barrierEntries = 0;
        if (backend_.processBarriers) {
            barrierEntries = backend_.processBarriers();
        }

        size_t dirtyCards = 0;
        if (backend_.rescanDirtyCards) {
            dirtyCards = backend_.rescanDirtyCards();
        }

        // Drain any remaining grey objects from barrier processing.
        size_t remarked = 0;
        if (backend_.traceGrey) {
            remarked = backend_.traceGrey();
        }

        if (backend_.setSATBMode) backend_.setSATBMode(false);
        if (backend_.exitSTW) backend_.exitSTW();

        if (backend_.markingDone) backend_.markingDone();

        phase_ = MarkPhase::Complete;

        double elapsed = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();

        stats_.cycleCount++;
        stats_.totalRoots += rootCount;
        stats_.totalTraced += traced + remarked;
        stats_.totalBarrierEntries += barrierEntries;
        stats_.totalDirtyCards += dirtyCards;
        stats_.lastCycleMs = elapsed;

        phase_ = MarkPhase::Idle;
    }

    struct Stats {
        uint64_t cycleCount;
        uint64_t totalRoots;
        uint64_t totalTraced;
        uint64_t totalBarrierEntries;
        uint64_t totalDirtyCards;
        double lastCycleMs;
    };

    const Stats& stats() const { return stats_; }

private:
    Backend backend_;
    MarkPhase phase_ = MarkPhase::Idle;
    Stats stats_{};
};

} // namespace Zepra::Heap
