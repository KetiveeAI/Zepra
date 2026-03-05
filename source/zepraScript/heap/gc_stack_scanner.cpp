// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_stack_scanner.cpp — Conservative stack scanning for GC roots

#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <algorithm>

namespace Zepra::Heap {

// Conservative scanner: treat every aligned word on the stack as a
// potential heap pointer. Filter using heap bounds check + object
// header validation. False positives pin objects but never cause
// correctness issues.

struct HeapBounds {
    uintptr_t start;
    uintptr_t end;
    bool contains(uintptr_t addr) const { return addr >= start && addr < end; }
};

class ConservativeStackScanner {
public:
    struct Callbacks {
        std::function<bool(uintptr_t)> isValidObject;
        std::function<void(uintptr_t)> reportRoot;
    };

    void setCallbacks(Callbacks cb) { cb_ = std::move(cb); }
    void setHeapBounds(HeapBounds bounds) { bounds_ = bounds; }

    // Scan a contiguous stack region [stackBottom, stackTop).
    size_t scanStack(uintptr_t stackBottom, uintptr_t stackTop) {
        if (stackBottom >= stackTop) return 0;
        size_t found = 0;

        // Walk word-aligned addresses.
        uintptr_t aligned = (stackBottom + 7) & ~uintptr_t(7);
        for (uintptr_t addr = aligned; addr + sizeof(uintptr_t) <= stackTop;
             addr += sizeof(uintptr_t)) {
            uintptr_t value = *reinterpret_cast<uintptr_t*>(addr);
            if (isCandidate(value)) {
                if (cb_.reportRoot) cb_.reportRoot(value);
                found++;
            }
        }

        stats_.wordsScanned += (stackTop - aligned) / sizeof(uintptr_t);
        stats_.rootsFound += found;
        return found;
    }

    // Scan register spill area (platform-specific).
    size_t scanRegisters(const uintptr_t* regs, size_t count) {
        size_t found = 0;
        for (size_t i = 0; i < count; i++) {
            if (isCandidate(regs[i])) {
                if (cb_.reportRoot) cb_.reportRoot(regs[i]);
                found++;
            }
        }
        stats_.rootsFound += found;
        return found;
    }

    struct Stats {
        uint64_t wordsScanned;
        uint64_t rootsFound;
        uint64_t falseCandidates;
    };

    const Stats& stats() const { return stats_; }

private:
    bool isCandidate(uintptr_t value) const {
        // Quick bounds check.
        if (!bounds_.contains(value)) return false;
        // Alignment check: heap objects are 8-byte aligned.
        if (value & 0x7) return false;
        // Validate via callback (checks object header magic, etc).
        if (cb_.isValidObject && !cb_.isValidObject(value)) {
            return false;
        }
        return true;
    }

    Callbacks cb_;
    HeapBounds bounds_;
    Stats stats_{};
};

// Per-thread stack metadata for root scanning.
struct ThreadStackInfo {
    uint32_t threadId;
    uintptr_t stackBase;    // Bottom of stack (highest address on most platforms)
    uintptr_t stackLimit;   // Top (lowest address / guard page)
    uintptr_t sp;           // Current stack pointer at safepoint

    ThreadStackInfo()
        : threadId(0), stackBase(0), stackLimit(0), sp(0) {}

    size_t usedSize() const {
        return stackBase > sp ? stackBase - sp : 0;
    }
};

class StackScanManager {
public:
    void registerThread(const ThreadStackInfo& info) {
        std::lock_guard<std::mutex> lock(mutex_);
        threads_.push_back(info);
    }

    void unregisterThread(uint32_t threadId) {
        std::lock_guard<std::mutex> lock(mutex_);
        threads_.erase(
            std::remove_if(threads_.begin(), threads_.end(),
                [threadId](const ThreadStackInfo& t) {
                    return t.threadId == threadId;
                }),
            threads_.end());
    }

    void updateSP(uint32_t threadId, uintptr_t sp) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& t : threads_) {
            if (t.threadId == threadId) { t.sp = sp; return; }
        }
    }

    // Scan all registered thread stacks.
    size_t scanAll(ConservativeStackScanner& scanner) {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t total = 0;
        for (auto& t : threads_) {
            if (t.sp > 0 && t.stackBase > t.sp) {
                total += scanner.scanStack(t.sp, t.stackBase);
            }
        }
        return total;
    }

    size_t threadCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return threads_.size();
    }

private:
    mutable std::mutex mutex_;
    std::vector<ThreadStackInfo> threads_;
};

} // namespace Zepra::Heap
