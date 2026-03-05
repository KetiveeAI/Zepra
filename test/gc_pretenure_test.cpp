// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_pretenure_test.cpp

#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Test {

struct SimPretenure {
    struct Site {
        uint64_t allocs;
        uint64_t survived;
        bool pretenure;
        Site() : allocs(0), survived(0), pretenure(false) {}
    };

    std::unordered_map<uint64_t, Site> sites;
    double threshold;
    uint64_t minSamples;

    SimPretenure(double thresh = 0.80, uint64_t minSamp = 10)
        : threshold(thresh), minSamples(minSamp) {}

    void recordAlloc(uint64_t siteId) { sites[siteId].allocs++; }
    void recordSurvival(uint64_t siteId) { sites[siteId].survived++; }

    void updateDecisions() {
        for (auto& [id, s] : sites) {
            if (s.allocs >= minSamples) {
                double rate = static_cast<double>(s.survived) / s.allocs;
                s.pretenure = rate >= threshold;
            }
        }
    }

    bool shouldPretenure(uint64_t siteId) const {
        auto it = sites.find(siteId);
        return it != sites.end() && it->second.pretenure;
    }
};

using PretenureTestFn = std::function<bool(const char*& fail)>;

class PretenureTestRunner {
public:
    void add(const char* name, PretenureTestFn fn) {
        tests_.push_back({name, std::move(fn)});
    }
    size_t runAll() {
        size_t p = 0, f = 0;
        fprintf(stderr, "\n=== Pretenure Tests ===\n\n");
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
    std::vector<std::pair<const char*, PretenureTestFn>> tests_;
};

static void registerPretenureTests(PretenureTestRunner& runner) {

    runner.add("HighSurvivalPretenures", [](const char*& fail) -> bool {
        SimPretenure pt(0.80, 10);
        for (int i = 0; i < 20; i++) { pt.recordAlloc(1); pt.recordSurvival(1); }
        pt.updateDecisions();
        if (!pt.shouldPretenure(1)) { fail = "100% survival should pretenure"; return false; }
        return true;
    });

    runner.add("LowSurvivalNoPretenure", [](const char*& fail) -> bool {
        SimPretenure pt(0.80, 10);
        for (int i = 0; i < 20; i++) pt.recordAlloc(2);
        for (int i = 0; i < 5; i++) pt.recordSurvival(2);
        pt.updateDecisions();
        if (pt.shouldPretenure(2)) { fail = "25% should not pretenure"; return false; }
        return true;
    });

    runner.add("BelowMinSamplesNoDecision", [](const char*& fail) -> bool {
        SimPretenure pt(0.80, 100);
        for (int i = 0; i < 10; i++) { pt.recordAlloc(3); pt.recordSurvival(3); }
        pt.updateDecisions();
        if (pt.shouldPretenure(3)) { fail = "below min samples"; return false; }
        return true;
    });

    runner.add("MultipleSitesIndependent", [](const char*& fail) -> bool {
        SimPretenure pt(0.80, 10);
        for (int i = 0; i < 20; i++) { pt.recordAlloc(10); pt.recordSurvival(10); }
        for (int i = 0; i < 20; i++) pt.recordAlloc(20);
        pt.updateDecisions();
        if (!pt.shouldPretenure(10)) { fail = "site 10 should pretenure"; return false; }
        if (pt.shouldPretenure(20)) { fail = "site 20 should not"; return false; }
        return true;
    });

    runner.add("UnknownSiteNoPretenure", [](const char*& fail) -> bool {
        SimPretenure pt;
        if (pt.shouldPretenure(999)) { fail = "unknown should not pretenure"; return false; }
        return true;
    });

    runner.add("ExactThreshold", [](const char*& fail) -> bool {
        SimPretenure pt(0.80, 10);
        for (int i = 0; i < 10; i++) pt.recordAlloc(5);
        for (int i = 0; i < 8; i++) pt.recordSurvival(5);
        pt.updateDecisions();
        if (!pt.shouldPretenure(5)) { fail = "80% should pretenure at 0.80 threshold"; return false; }
        return true;
    });
}

static size_t runPretenureTests() {
    PretenureTestRunner runner;
    registerPretenureTests(runner);
    return runner.runAll();
}

} // namespace Zepra::Test
