// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_age_table_test.cpp

#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Test {

struct SimAgeTable {
    static constexpr uint8_t MAX_AGE = 15;

    uint64_t counts[MAX_AGE + 1] = {};
    uint8_t threshold = 6;

    void recordAge(uint8_t age) { if (age <= MAX_AGE) counts[age]++; }
    void clear() { for (auto& c : counts) c = 0; }

    bool shouldPromote(uint8_t age) const { return age >= threshold; }

    uint8_t computeThreshold(size_t survivorCap, double target, size_t avgObjSize) {
        size_t targetBytes = static_cast<size_t>(survivorCap * target);
        size_t cum = 0;
        for (uint8_t age = 0; age <= MAX_AGE; age++) {
            cum += counts[age] * avgObjSize;
            if (cum > targetBytes) {
                threshold = age > 0 ? age : 1;
                return threshold;
            }
        }
        threshold = MAX_AGE;
        return threshold;
    }

    double promotionRate() const {
        uint64_t total = 0, promoted = 0;
        for (uint8_t i = 0; i <= MAX_AGE; i++) total += counts[i];
        if (total == 0) return 0;
        for (uint8_t i = threshold; i <= MAX_AGE; i++) promoted += counts[i];
        return static_cast<double>(promoted) / total;
    }
};

using AgeTestFn = std::function<bool(const char*& fail)>;

class AgeTestRunner {
public:
    void add(const char* name, AgeTestFn fn) { tests_.push_back({name, std::move(fn)}); }
    size_t runAll() {
        size_t p = 0, f = 0;
        fprintf(stderr, "\n=== Age Table Tests ===\n\n");
        for (auto& [name, fn] : tests_) {
            const char* fail = nullptr;
            bool ok = false;
            try { ok = fn(fail); } catch (...) { fail = "exception"; }
            if (ok) { fprintf(stderr, "  [PASS] %s\n", name); p++; }
            else { fprintf(stderr, "  [FAIL] %s: %s\n", name, fail ? fail : "?"); f++; }
        }
        fprintf(stderr, "\n  %zu passed, %zu failed\n\n", p, f);
        return f;
    }
private:
    std::vector<std::pair<const char*, AgeTestFn>> tests_;
};

static void registerAgeTests(AgeTestRunner& runner) {
    runner.add("DefaultThreshold", [](const char*& fail) -> bool {
        SimAgeTable at;
        if (!at.shouldPromote(6)) { fail = "age 6 should promote"; return false; }
        if (at.shouldPromote(5)) { fail = "age 5 should not"; return false; }
        return true;
    });

    runner.add("AdaptiveThresholdDown", [](const char*& fail) -> bool {
        SimAgeTable at;
        // Many survivors at low ages → lower threshold.
        for (int i = 0; i < 1000; i++) at.recordAge(1);
        for (int i = 0; i < 1000; i++) at.recordAge(2);
        // Survivor capacity 1000 * 64 bytes, target 50%.
        at.computeThreshold(1000 * 64, 0.50, 64);
        if (at.threshold >= 6) { fail = "threshold should drop"; return false; }
        return true;
    });

    runner.add("AdaptiveThresholdUp", [](const char*& fail) -> bool {
        SimAgeTable at;
        // Very few survivors → threshold goes to MAX.
        at.recordAge(1);
        at.computeThreshold(100000, 0.50, 64);
        if (at.threshold != 15) { fail = "threshold should be MAX"; return false; }
        return true;
    });

    runner.add("PromotionRate", [](const char*& fail) -> bool {
        SimAgeTable at;
        at.threshold = 3;
        for (int i = 0; i < 10; i++) at.recordAge(1);
        for (int i = 0; i < 10; i++) at.recordAge(5);
        double rate = at.promotionRate();
        if (rate < 0.49 || rate > 0.51) { fail = "expected ~50%"; return false; }
        return true;
    });

    runner.add("ClearResetsAll", [](const char*& fail) -> bool {
        SimAgeTable at;
        for (int i = 0; i < 100; i++) at.recordAge(3);
        at.clear();
        if (at.promotionRate() != 0) { fail = "should be 0 after clear"; return false; }
        return true;
    });

    runner.add("BoundaryAge", [](const char*& fail) -> bool {
        SimAgeTable at;
        at.recordAge(0);
        at.recordAge(15);
        if (at.counts[0] != 1) { fail = "age 0 missing"; return false; }
        if (at.counts[15] != 1) { fail = "age 15 missing"; return false; }
        return true;
    });
}

static size_t runAgeTests() {
    AgeTestRunner runner;
    registerAgeTests(runner);
    return runner.runAll();
}

} // namespace Zepra::Test
