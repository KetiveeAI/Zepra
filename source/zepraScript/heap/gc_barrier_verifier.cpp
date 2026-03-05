// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_barrier_verifier.cpp — Debug-mode write barrier verification

#include <mutex>
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <functional>

namespace Zepra::Heap {

// In debug builds, verify that every old→young reference has a
// corresponding card table dirty entry. Missed barriers cause
// dangling pointer bugs that are extremely hard to debug.
// This verifier catches them at the point of the store.

struct BarrierViolation {
    uintptr_t srcAddr;
    uintptr_t dstAddr;
    const char* opName;
    uint64_t timestamp;
};

class BarrierVerifier {
public:
    struct Callbacks {
        std::function<bool(uintptr_t)> isInNursery;
        std::function<bool(uintptr_t)> isCardDirty;
    };

    void setCallbacks(Callbacks cb) { cb_ = std::move(cb); }
    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }
    bool isEnabled() const { return enabled_; }

    // Called after every reference store.
    void verifyStore(uintptr_t src, uintptr_t dst, const char* op) {
        if (!enabled_ || !cb_.isInNursery || !cb_.isCardDirty) return;

        // Only verify old→young stores.
        if (cb_.isInNursery(src)) return;      // Source in nursery: skip
        if (!cb_.isInNursery(dst)) return;      // Dest not in nursery: skip

        // Old→young store: card must be dirty.
        if (!cb_.isCardDirty(src)) {
            std::lock_guard<std::mutex> lock(mutex_);
            violations_.push_back({src, dst, op, violationCount_++});

            fprintf(stderr, "[barrier-verifier] VIOLATION: old(0x%lx) → "
                "young(0x%lx) at %s without card dirty\n",
                static_cast<unsigned long>(src),
                static_cast<unsigned long>(dst), op);
        }
    }

    size_t violationCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return violations_.size();
    }

    const std::vector<BarrierViolation>& violations() const {
        return violations_;
    }

    void clearViolations() {
        std::lock_guard<std::mutex> lock(mutex_);
        violations_.clear();
    }

private:
    Callbacks cb_;
    bool enabled_ = false;
    mutable std::mutex mutex_;
    std::vector<BarrierViolation> violations_;
    uint64_t violationCount_ = 0;
};

} // namespace Zepra::Heap
