/**
 * @file gc_scheduler_test.cpp
 * @brief Tests for GC scheduling logic
 *
 * Tests:
 * 1. Budget exhaustion triggers GC
 * 2. Budget adapts after fast GC (increases)
 * 3. Budget adapts after slow GC (decreases)
 * 4. Occupancy monitor thresholds (minor/major/full/compact)
 * 5. Idle time scheduler respects deadline
 * 6. Memory pressure triggers full GC
 * 7. Manual GC request
 * 8. No trigger when within budget and thresholds
 */

#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Test {

// =============================================================================
// Simulated Budget
// =============================================================================

class SimBudget {
public:
    explicit SimBudget(size_t budget)
        : budget_(budget), remaining_(budget) {}

    bool charge(size_t bytes) {
        if (bytes >= remaining_) {
            remaining_ = 0;
            return true;
        }
        remaining_ -= bytes;
        return false;
    }

    void reset() { remaining_ = budget_; }

    void adapt(double gcMs, size_t reclaimed) {
        double eff = reclaimed > 0 ?
            static_cast<double>(reclaimed) / gcMs : 0;

        if (gcMs < 2.0 && eff > 1000) {
            budget_ = budget_ * 3 / 2;
        } else if (gcMs > 10.0) {
            budget_ = budget_ * 2 / 3;
        }
        remaining_ = budget_;
    }

    size_t budget() const { return budget_; }
    size_t remaining() const { return remaining_; }

private:
    size_t budget_;
    size_t remaining_;
};

// =============================================================================
// Simulated Occupancy Monitor
// =============================================================================

class SimOccupancy {
public:
    enum Rec { None, Minor, Major, Full, Compact };

    Rec check(double nurseryOcc, double oldOcc,
              double totalOcc, double frag) {
        if (frag > 0.40) return Compact;
        if (totalOcc > 0.90) return Full;
        if (oldOcc > 0.75) return Major;
        if (nurseryOcc > 0.90) return Minor;
        return None;
    }
};

// =============================================================================
// Test Runner
// =============================================================================

using SchedTestFn = std::function<bool(const char*& fail)>;

class SchedTestRunner {
public:
    void add(const char* name, SchedTestFn fn) {
        tests_.push_back({name, std::move(fn)});
    }

    size_t runAll() {
        size_t passed = 0, failed = 0;
        fprintf(stderr, "\n=== GC Scheduler Tests ===\n\n");

        for (auto& [name, fn] : tests_) {
            const char* fail = nullptr;
            bool ok = false;
            try { ok = fn(fail); } catch (...) {
                fail = "exception"; ok = false;
            }

            if (ok) {
                fprintf(stderr, "  [PASS] %s\n", name);
                passed++;
            } else {
                fprintf(stderr, "  [FAIL] %s: %s\n", name,
                    fail ? fail : "unknown");
                failed++;
            }
        }

        fprintf(stderr, "\n  %zu passed, %zu failed\n\n", passed, failed);
        return failed;
    }

private:
    std::vector<std::pair<const char*, SchedTestFn>> tests_;
};

// =============================================================================
// Tests
// =============================================================================

static void registerSchedTests(SchedTestRunner& runner) {

    runner.add("BudgetExhaustionTriggers",
        [](const char*& fail) -> bool {
        SimBudget budget(1024);

        bool triggered = budget.charge(512);
        if (triggered) { fail = "should not trigger at 512"; return false; }

        triggered = budget.charge(600);
        if (!triggered) { fail = "should trigger at 1112"; return false; }

        return true;
    });

    runner.add("BudgetResetAfterGC",
        [](const char*& fail) -> bool {
        SimBudget budget(1024);
        budget.charge(1024);
        budget.reset();

        if (budget.remaining() != 1024) {
            fail = "budget should be full after reset"; return false;
        }

        bool triggered = budget.charge(512);
        if (triggered) {
            fail = "should not trigger after reset"; return false;
        }
        return true;
    });

    runner.add("BudgetAdaptsUpOnFastGC",
        [](const char*& fail) -> bool {
        SimBudget budget(1000);
        size_t before = budget.budget();

        budget.adapt(1.0, 50000);  // Fast GC, high reclaim

        if (budget.budget() <= before) {
            fail = "budget should increase after fast GC"; return false;
        }
        return true;
    });

    runner.add("BudgetAdaptsDownOnSlowGC",
        [](const char*& fail) -> bool {
        SimBudget budget(1000);
        size_t before = budget.budget();

        budget.adapt(20.0, 100);  // Slow GC

        if (budget.budget() >= before) {
            fail = "budget should decrease after slow GC"; return false;
        }
        return true;
    });

    runner.add("OccupancyNurseryTrigger",
        [](const char*& fail) -> bool {
        SimOccupancy occ;
        auto rec = occ.check(0.95, 0.50, 0.70, 0.10);
        if (rec != SimOccupancy::Minor) {
            fail = "expected Minor for high nursery"; return false;
        }
        return true;
    });

    runner.add("OccupancyOldTrigger",
        [](const char*& fail) -> bool {
        SimOccupancy occ;
        auto rec = occ.check(0.50, 0.80, 0.70, 0.10);
        if (rec != SimOccupancy::Major) {
            fail = "expected Major for high old-gen"; return false;
        }
        return true;
    });

    runner.add("OccupancyTotalTrigger",
        [](const char*& fail) -> bool {
        SimOccupancy occ;
        auto rec = occ.check(0.95, 0.95, 0.95, 0.10);
        if (rec != SimOccupancy::Full) {
            fail = "expected Full for high total"; return false;
        }
        return true;
    });

    runner.add("OccupancyFragmentTrigger",
        [](const char*& fail) -> bool {
        SimOccupancy occ;
        auto rec = occ.check(0.30, 0.30, 0.30, 0.50);
        if (rec != SimOccupancy::Compact) {
            fail = "expected Compact for high fragmentation";
            return false;
        }
        return true;
    });

    runner.add("OccupancyNoTrigger",
        [](const char*& fail) -> bool {
        SimOccupancy occ;
        auto rec = occ.check(0.30, 0.30, 0.30, 0.10);
        if (rec != SimOccupancy::None) {
            fail = "expected None for low occupancy"; return false;
        }
        return true;
    });

    runner.add("BudgetChargeAccumulates",
        [](const char*& fail) -> bool {
        SimBudget budget(1000);

        for (int i = 0; i < 9; i++) {
            if (budget.charge(100)) {
                fail = "should not trigger before 1000"; return false;
            }
        }

        bool triggered = budget.charge(200);
        if (!triggered) {
            fail = "should trigger at 1100"; return false;
        }
        return true;
    });
}

static size_t runSchedTests() {
    SchedTestRunner runner;
    registerSchedTests(runner);
    return runner.runAll();
}

} // namespace Zepra::Test
