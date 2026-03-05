// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_mark_worklist.cpp — Concurrent marking worklist with local/global split

#include <atomic>
#include <mutex>
#include <vector>
#include <deque>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <algorithm>

namespace Zepra::Heap {

// Each marker thread has a local worklist (no sync). When it overflows
// or empties, it transfers to/from the global shared worklist.
// This minimizes contention on the hot path.

class LocalMarkWorklist {
public:
    static constexpr size_t LOCAL_CAPACITY = 256;

    void push(uintptr_t addr) {
        if (items_.size() >= LOCAL_CAPACITY) {
            overflow_ = true;
        }
        items_.push_back(addr);
    }

    uintptr_t pop() {
        if (items_.empty()) return 0;
        uintptr_t addr = items_.back();
        items_.pop_back();
        return addr;
    }

    bool isEmpty() const { return items_.empty(); }
    bool overflowed() const { return overflow_; }
    size_t size() const { return items_.size(); }

    // Transfer half to global worklist.
    std::vector<uintptr_t> drainHalf() {
        std::vector<uintptr_t> batch;
        size_t count = items_.size() / 2;
        for (size_t i = 0; i < count; i++) {
            batch.push_back(items_.back());
            items_.pop_back();
        }
        overflow_ = false;
        return batch;
    }

    // Fill from global worklist.
    void fill(const std::vector<uintptr_t>& items) {
        for (auto addr : items) items_.push_back(addr);
    }

private:
    std::vector<uintptr_t> items_;
    bool overflow_ = false;
};

class GlobalMarkWorklist {
public:
    void pushBatch(const std::vector<uintptr_t>& batch) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto addr : batch) items_.push_back(addr);
        totalPushed_ += batch.size();
    }

    std::vector<uintptr_t> stealBatch(size_t maxCount) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<uintptr_t> batch;
        size_t count = std::min(maxCount, items_.size());
        for (size_t i = 0; i < count; i++) {
            batch.push_back(items_.front());
            items_.pop_front();
        }
        totalStolen_ += batch.size();
        return batch;
    }

    void push(uintptr_t addr) {
        std::lock_guard<std::mutex> lock(mutex_);
        items_.push_back(addr);
        totalPushed_++;
    }

    bool isEmpty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.size();
    }

    struct Stats {
        uint64_t totalPushed;
        uint64_t totalStolen;
    };

    Stats stats() const { return {totalPushed_, totalStolen_}; }

private:
    mutable std::mutex mutex_;
    std::deque<uintptr_t> items_;
    uint64_t totalPushed_ = 0;
    uint64_t totalStolen_ = 0;
};

// Coordinator for N marker threads.
class MarkWorklistManager {
public:
    explicit MarkWorklistManager(size_t threadCount)
        : locals_(threadCount) {}

    void pushLocal(size_t threadIdx, uintptr_t addr) {
        if (threadIdx >= locals_.size()) return;
        locals_[threadIdx].push(addr);
        if (locals_[threadIdx].overflowed()) {
            auto batch = locals_[threadIdx].drainHalf();
            global_.pushBatch(batch);
        }
    }

    uintptr_t popLocal(size_t threadIdx) {
        if (threadIdx >= locals_.size()) return 0;
        uintptr_t addr = locals_[threadIdx].pop();
        if (addr != 0) return addr;

        // Local empty — steal from global.
        auto batch = global_.stealBatch(LocalMarkWorklist::LOCAL_CAPACITY / 2);
        if (batch.empty()) return 0;
        locals_[threadIdx].fill(batch);
        return locals_[threadIdx].pop();
    }

    bool allEmpty() const {
        if (!global_.isEmpty()) return false;
        for (auto& local : locals_) {
            if (!local.isEmpty()) return false;
        }
        return true;
    }

    size_t threadCount() const { return locals_.size(); }

private:
    std::vector<LocalMarkWorklist> locals_;
    GlobalMarkWorklist global_;
};

} // namespace Zepra::Heap
