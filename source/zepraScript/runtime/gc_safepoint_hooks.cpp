// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_safepoint_hooks.cpp — Runtime-side safepoint integration

#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Runtime {

// Safepoints are locations in mutator code where it's safe to pause
// for GC. The VM polls a flag at each safepoint. If set, the thread
// parks itself until GC completes. This file handles the runtime
// side: parking mutator threads and resuming after GC.

enum class SafepointReason : uint8_t {
    GC,
    Deoptimization,
    StackScan,
    Shutdown
};

struct ParkingRecord {
    uint32_t threadId;
    SafepointReason reason;
    uintptr_t pc;         // Program counter when parked
    uintptr_t sp;         // Stack pointer when parked
    uint64_t parkTime;    // Timestamp
};

class SafepointCoordinator {
public:
    // GC sets this to request all threads to park.
    void requestSafepoint(SafepointReason reason) {
        reason_ = reason;
        safepointRequested_.store(true, std::memory_order_release);
    }

    void clearSafepoint() {
        safepointRequested_.store(false, std::memory_order_release);
    }

    // Polled by mutator at safepoint locations (loop back-edges, calls).
    bool isRequested() const {
        return safepointRequested_.load(std::memory_order_acquire);
    }

    // Mutator calls this when it reaches a safepoint and sees the flag.
    void parkThread(uint32_t threadId, uintptr_t pc, uintptr_t sp) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ParkingRecord rec;
            rec.threadId = threadId;
            rec.reason = reason_;
            rec.pc = pc;
            rec.sp = sp;
            rec.parkTime = 0;
            parked_.push_back(rec);
            parkedCount_.fetch_add(1, std::memory_order_release);
        }

        // Wait until safepoint is cleared.
        while (safepointRequested_.load(std::memory_order_acquire)) {
            // Spin. In production, use futex/condvar.
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            parkedCount_.fetch_sub(1, std::memory_order_release);
        }
    }

    // GC waits for all threads to park.
    void waitForAllParked(size_t expectedCount) {
        while (parkedCount_.load(std::memory_order_acquire) < expectedCount) {
            // Spin until all mutators parked.
        }
    }

    // Iterate parked thread stacks for root scanning.
    void forEachParkedStack(
            std::function<void(uint32_t threadId, uintptr_t sp)> visitor) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& rec : parked_) {
            visitor(rec.threadId, rec.sp);
        }
    }

    size_t parkedCount() const {
        return parkedCount_.load(std::memory_order_acquire);
    }

    // Clear parking records after GC completes.
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        parked_.clear();
    }

private:
    std::atomic<bool> safepointRequested_{false};
    std::atomic<size_t> parkedCount_{0};
    SafepointReason reason_ = SafepointReason::GC;
    std::mutex mutex_;
    std::vector<ParkingRecord> parked_;
};

// Page-protection based safepoint (alternative to polling).
// Guard page is mapped PROT_NONE; reading it traps to signal handler.
class GuardPageSafepoint {
public:
    struct Config {
        uintptr_t guardPageAddr;
        size_t pageSize;
    };

    explicit GuardPageSafepoint(const Config& config)
        : config_(config), armed_(false) {}

    // Arm: set guard page to PROT_NONE so polling reads trap.
    void arm() { armed_ = true; }

    // Disarm: set guard page to PROT_READ so polling succeeds.
    void disarm() { armed_ = false; }

    bool isArmed() const { return armed_; }
    uintptr_t guardPageAddr() const { return config_.guardPageAddr; }

private:
    Config config_;
    bool armed_;
};

} // namespace Zepra::Runtime
