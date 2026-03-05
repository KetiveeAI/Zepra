// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_state_machine.cpp — GC state transitions and invariants

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Heap {

// Enforces valid GC state transitions. The heap must be in the
// correct state for each operation. Invalid transitions are bugs
// in the GC implementation.

enum class GCState : uint8_t {
    Idle,
    MarkingStarted,
    RootScanActive,
    ConcurrentTracing,
    RemarkActive,
    MarkingComplete,
    SweepStarted,
    ConcurrentSweeping,
    SweepComplete,
    CompactionStarted,
    Compacting,
    CompactionComplete,
    Paused  // STW
};

class GCStateMachine {
public:
    GCStateMachine() : state_(static_cast<uint8_t>(GCState::Idle)) {}

    GCState state() const {
        return static_cast<GCState>(state_.load(std::memory_order_acquire));
    }

    bool transition(GCState from, GCState to) {
        uint8_t expected = static_cast<uint8_t>(from);
        uint8_t desired = static_cast<uint8_t>(to);
        bool ok = state_.compare_exchange_strong(expected, desired,
            std::memory_order_acq_rel);

        if (!ok) {
            fprintf(stderr, "[gc-state] Invalid transition: %u → %u "
                "(current: %u)\n",
                static_cast<unsigned>(from),
                static_cast<unsigned>(to),
                static_cast<unsigned>(expected));
        }

        return ok;
    }

    // Convenience transitions.
    bool startMarking() {
        return transition(GCState::Idle, GCState::MarkingStarted);
    }

    bool enterRootScan() {
        return transition(GCState::MarkingStarted, GCState::RootScanActive);
    }

    bool startConcurrentTracing() {
        return transition(GCState::RootScanActive, GCState::ConcurrentTracing);
    }

    bool enterRemark() {
        return transition(GCState::ConcurrentTracing, GCState::RemarkActive);
    }

    bool completeMarking() {
        return transition(GCState::RemarkActive, GCState::MarkingComplete);
    }

    bool startSweeping() {
        return transition(GCState::MarkingComplete, GCState::SweepStarted);
    }

    bool startConcurrentSweeping() {
        return transition(GCState::SweepStarted, GCState::ConcurrentSweeping);
    }

    bool completeSweeping() {
        return transition(GCState::ConcurrentSweeping, GCState::SweepComplete);
    }

    bool startCompaction() {
        return transition(GCState::SweepComplete, GCState::CompactionStarted);
    }

    bool completeCompaction() {
        return transition(GCState::Compacting, GCState::CompactionComplete);
    }

    bool returnToIdle() {
        GCState cur = state();
        if (cur == GCState::SweepComplete || cur == GCState::CompactionComplete) {
            return transition(cur, GCState::Idle);
        }
        return false;
    }

    bool isIdle() const { return state() == GCState::Idle; }
    bool isMarking() const {
        auto s = state();
        return s == GCState::MarkingStarted || s == GCState::RootScanActive ||
               s == GCState::ConcurrentTracing || s == GCState::RemarkActive;
    }
    bool isSweeping() const {
        auto s = state();
        return s == GCState::SweepStarted || s == GCState::ConcurrentSweeping;
    }

private:
    std::atomic<uint8_t> state_;
};

} // namespace Zepra::Heap
