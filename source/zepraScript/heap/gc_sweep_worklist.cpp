// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_sweep_worklist.cpp — Work-stealing queue for concurrent sweeping

#include <atomic>
#include <mutex>
#include <deque>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Heap {

// Each page to be swept is a work item. Multiple sweep threads
// pull items from this shared worklist. LIFO for cache locality,
// FIFO stealing for fairness.

struct SweepWorkItem {
    uintptr_t pageBase;
    size_t pageSize;
    uint64_t regionId;
    bool needsCompaction;

    SweepWorkItem()
        : pageBase(0), pageSize(0), regionId(0), needsCompaction(false) {}

    SweepWorkItem(uintptr_t b, size_t s, uint64_t r, bool c = false)
        : pageBase(b), pageSize(s), regionId(r), needsCompaction(c) {}
};

class SweepWorklist {
public:
    void push(const SweepWorkItem& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        items_.push_back(item);
        totalPushed_++;
    }

    void pushBatch(const std::vector<SweepWorkItem>& batch) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& item : batch) {
            items_.push_back(item);
            totalPushed_++;
        }
    }

    // Pop from back (LIFO — the thread that pushed likely has warm cache).
    bool pop(SweepWorkItem& out) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (items_.empty()) return false;
        out = items_.back();
        items_.pop_back();
        totalPopped_++;
        return true;
    }

    // Steal from front (FIFO — steal older items, less contention).
    bool steal(SweepWorkItem& out) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (items_.empty()) return false;
        out = items_.front();
        items_.pop_front();
        totalStolen_++;
        return true;
    }

    bool isEmpty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        items_.clear();
    }

    struct Stats {
        uint64_t totalPushed;
        uint64_t totalPopped;
        uint64_t totalStolen;
    };

    Stats stats() const {
        return {totalPushed_, totalPopped_, totalStolen_};
    }

private:
    mutable std::mutex mutex_;
    std::deque<SweepWorkItem> items_;
    uint64_t totalPushed_ = 0;
    uint64_t totalPopped_ = 0;
    uint64_t totalStolen_ = 0;
};

// Distributes sweep work across N sweeper threads.
class SweepWorkDistributor {
public:
    explicit SweepWorkDistributor(size_t threadCount)
        : worklists_(threadCount) {}

    void distribute(const std::vector<SweepWorkItem>& pages) {
        for (size_t i = 0; i < pages.size(); i++) {
            worklists_[i % worklists_.size()].push(pages[i]);
        }
    }

    // Thread i pops from own list, steals from next if empty.
    bool getWork(size_t threadIdx, SweepWorkItem& out) {
        if (threadIdx >= worklists_.size()) return false;

        if (worklists_[threadIdx].pop(out)) return true;

        // Work stealing: try neighbors.
        for (size_t offset = 1; offset < worklists_.size(); offset++) {
            size_t victim = (threadIdx + offset) % worklists_.size();
            if (worklists_[victim].steal(out)) return true;
        }
        return false;
    }

    bool allEmpty() const {
        for (auto& wl : worklists_) {
            if (!wl.isEmpty()) return false;
        }
        return true;
    }

    size_t threadCount() const { return worklists_.size(); }

private:
    std::vector<SweepWorklist> worklists_;
};

} // namespace Zepra::Heap
