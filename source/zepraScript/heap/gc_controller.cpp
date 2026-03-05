// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_controller.cpp — Unified GC controller connecting all subsystems

#include <atomic>
#include <mutex>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <chrono>
#include <vector>

namespace Zepra::Heap {

// Single entry point for the VM to interact with the entire GC system.
// Wires together: nursery, old-gen, large-object space, marking,
// sweeping, compaction, scheduling, telemetry, and safety.

enum class GCType : uint8_t {
    Minor,       // Nursery scavenge
    Major,       // Full mark-sweep
    Incremental, // Incremental marking step
    Compaction,  // Compaction cycle
    Emergency    // OOM recovery
};

class GCController {
public:
    // Subsystem interfaces — set by heap initialization.
    struct Subsystems {
        // Minor GC (scavenge nursery).
        std::function<void()> scavenge;
        // Major GC (mark-sweep-compact old gen).
        std::function<void()> majorCollect;
        // Incremental marking step.
        std::function<size_t(size_t budget)> incrementalStep;
        // Run compaction cycle.
        std::function<void()> compact;
        // Safepoint: request all threads to park.
        std::function<void()> requestSafepoint;
        // Safepoint: resume all threads.
        std::function<void()> clearSafepoint;
        // Wait for all threads to park.
        std::function<void(size_t)> waitForParked;
        // Get thread count.
        std::function<size_t()> threadCount;
        // Retire all TLABs before GC.
        std::function<void()> retireTLABs;
        // Flush write barrier buffers.
        std::function<size_t()> flushBarrierBuffers;
        // Scan roots.
        std::function<void(std::function<void(uintptr_t)>)> scanRoots;
        // Sweep completed — resume allocations.
        std::function<void()> sweepComplete;
        // Telemetry: record GC event.
        std::function<void(GCType, double ms, size_t freed)> recordEvent;
    };

    void setSubsystems(Subsystems s) { subs_ = std::move(s); }

    // The VM calls this on allocation failure or budget exhaustion.
    void collectGarbage(GCType type) {
        auto start = std::chrono::steady_clock::now();

        // Pre-GC: retire TLABs, flush barriers.
        if (subs_.retireTLABs) subs_.retireTLABs();
        if (subs_.flushBarrierBuffers) subs_.flushBarrierBuffers();

        switch (type) {
            case GCType::Minor:
                runMinorGC();
                break;
            case GCType::Major:
                runMajorGC();
                break;
            case GCType::Incremental:
                runIncrementalStep();
                break;
            case GCType::Compaction:
                runCompaction();
                break;
            case GCType::Emergency:
                runEmergencyGC();
                break;
        }

        double elapsed = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();

        if (subs_.recordEvent) subs_.recordEvent(type, elapsed, 0);

        stats_.collections++;
        stats_.totalPauseMs += elapsed;
        if (elapsed > stats_.maxPauseMs) stats_.maxPauseMs = elapsed;
    }

    // Called by VM every N allocations for incremental marking.
    void allocationBudgetExhausted(size_t allocatedBytes) {
        if (incrementalActive_) {
            if (subs_.incrementalStep) {
                subs_.incrementalStep(allocatedBytes / 4);
            }
        }
    }

    void startIncrementalCycle() { incrementalActive_ = true; }
    void finishIncrementalCycle() { incrementalActive_ = false; }

    struct Stats {
        uint64_t collections;
        double totalPauseMs;
        double maxPauseMs;
    };

    const Stats& stats() const { return stats_; }

private:
    void runMinorGC() {
        enterSafepoint();
        if (subs_.scavenge) subs_.scavenge();
        exitSafepoint();
    }

    void runMajorGC() {
        enterSafepoint();
        if (subs_.majorCollect) subs_.majorCollect();
        exitSafepoint();
    }

    void runIncrementalStep() {
        // No safepoint needed — runs concurrently.
        if (subs_.incrementalStep) subs_.incrementalStep(4096);
    }

    void runCompaction() {
        enterSafepoint();
        if (subs_.compact) subs_.compact();
        exitSafepoint();
    }

    void runEmergencyGC() {
        enterSafepoint();
        if (subs_.majorCollect) subs_.majorCollect();
        if (subs_.compact) subs_.compact();
        exitSafepoint();
    }

    void enterSafepoint() {
        if (subs_.requestSafepoint) subs_.requestSafepoint();
        if (subs_.waitForParked && subs_.threadCount) {
            subs_.waitForParked(subs_.threadCount());
        }
    }

    void exitSafepoint() {
        if (subs_.clearSafepoint) subs_.clearSafepoint();
    }

    Subsystems subs_;
    Stats stats_{};
    bool incrementalActive_ = false;
};

} // namespace Zepra::Heap
