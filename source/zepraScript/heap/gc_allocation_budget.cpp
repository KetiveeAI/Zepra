// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_allocation_budget.cpp — Per-thread allocation budget for incremental GC

#include <atomic>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Heap {

// Each mutator thread gets an allocation budget. When exhausted,
// the thread does a proportional amount of GC work before getting
// a new budget. This paces incremental GC with allocation.

class AllocationBudget {
public:
    static constexpr size_t DEFAULT_BUDGET = 32 * 1024;  // 32KB per step

    explicit AllocationBudget(size_t budget = DEFAULT_BUDGET)
        : budget_(budget), remaining_(budget) {}

    // Called on every allocation. Returns true when budget exhausted.
    bool consume(size_t bytes) {
        if (bytes >= remaining_) {
            remaining_ = 0;
            exhaustions_++;
            return true;
        }
        remaining_ -= bytes;
        return false;
    }

    void refill() { remaining_ = budget_; }
    void refill(size_t newBudget) {
        budget_ = newBudget;
        remaining_ = newBudget;
    }

    size_t remaining() const { return remaining_; }
    size_t budget() const { return budget_; }
    double consumedRatio() const {
        return budget_ > 0 ? 1.0 - static_cast<double>(remaining_) / budget_ : 1.0;
    }

    uint64_t exhaustions() const { return exhaustions_; }

private:
    size_t budget_;
    size_t remaining_;
    uint64_t exhaustions_ = 0;
};

// Manages budgets across N threads, adapts budget size based on
// GC work throughput.
class AllocationBudgetManager {
public:
    explicit AllocationBudgetManager(size_t threadCount = 1)
        : budgets_(threadCount) {}

    AllocationBudget* getBudget(size_t threadIdx) {
        if (threadIdx >= budgets_.size()) return nullptr;
        return &budgets_[threadIdx];
    }

    // Adapt budget based on GC marking throughput.
    void adaptBudgets(double markingRateMBps, double allocationRateMBps) {
        if (markingRateMBps <= 0 || allocationRateMBps <= 0) return;

        // Budget = marking_rate / allocation_rate * base_budget
        // Higher marking throughput → larger budgets (less interruption).
        double ratio = markingRateMBps / allocationRateMBps;
        size_t newBudget = static_cast<size_t>(
            AllocationBudget::DEFAULT_BUDGET * ratio);

        // Clamp to [4KB, 256KB].
        if (newBudget < 4096) newBudget = 4096;
        if (newBudget > 256 * 1024) newBudget = 256 * 1024;

        for (auto& b : budgets_) b.refill(newBudget);
    }

    size_t threadCount() const { return budgets_.size(); }

private:
    std::vector<AllocationBudget> budgets_;
};

} // namespace Zepra::Heap
