// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_tlab_test.cpp — TLAB allocation tests

#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Test {

struct SimTLAB {
    uintptr_t start, cursor, end;
    uint64_t refills;

    SimTLAB(uintptr_t s, size_t size)
        : start(s), cursor(s), end(s + size), refills(0) {}

    uintptr_t allocate(size_t bytes) {
        bytes = (bytes + 7) & ~size_t(7);
        if (cursor + bytes > end) return 0;
        uintptr_t addr = cursor;
        cursor += bytes;
        return addr;
    }

    bool refill(uintptr_t newStart, size_t size) {
        start = newStart;
        cursor = newStart;
        end = newStart + size;
        refills++;
        return true;
    }

    size_t remaining() const { return end - cursor; }
    void retire() { start = cursor = end = 0; }
};

using TLABTestFn = std::function<bool(const char*& fail)>;

class TLABTestRunner {
public:
    void add(const char* name, TLABTestFn fn) {
        tests_.push_back({name, std::move(fn)});
    }

    size_t runAll() {
        size_t p = 0, f = 0;
        fprintf(stderr, "\n=== TLAB Tests ===\n\n");
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
    std::vector<std::pair<const char*, TLABTestFn>> tests_;
};

static void registerTLABTests(TLABTestRunner& runner) {

    runner.add("BasicAllocation", [](const char*& fail) -> bool {
        SimTLAB tlab(0x1000, 1024);
        auto a = tlab.allocate(64);
        auto b = tlab.allocate(64);
        if (a == 0 || b == 0) { fail = "alloc failed"; return false; }
        if (b != a + 64) { fail = "not bump-pointer"; return false; }
        return true;
    });

    runner.add("ExhaustsCapacity", [](const char*& fail) -> bool {
        SimTLAB tlab(0x1000, 256);
        for (int i = 0; i < 4; i++) {
            if (tlab.allocate(64) == 0) { fail = "early exhaust"; return false; }
        }
        if (tlab.allocate(64) != 0) { fail = "should be exhausted"; return false; }
        return true;
    });

    runner.add("RefillAndContinue", [](const char*& fail) -> bool {
        SimTLAB tlab(0x1000, 128);
        tlab.allocate(128);
        if (tlab.allocate(8) != 0) { fail = "should fail before refill"; return false; }
        tlab.refill(0x2000, 256);
        if (tlab.allocate(64) == 0) { fail = "should succeed after refill"; return false; }
        if (tlab.refills != 1) { fail = "refill count wrong"; return false; }
        return true;
    });

    runner.add("Retire", [](const char*& fail) -> bool {
        SimTLAB tlab(0x1000, 1024);
        tlab.allocate(512);
        tlab.retire();
        if (tlab.allocate(8) != 0) { fail = "retired TLAB should fail"; return false; }
        return true;
    });

    runner.add("Alignment", [](const char*& fail) -> bool {
        SimTLAB tlab(0x1000, 1024);
        auto a = tlab.allocate(13);
        auto b = tlab.allocate(7);
        if (a % 8 != 0) { fail = "first not aligned"; return false; }
        if (b % 8 != 0) { fail = "second not aligned"; return false; }
        return true;
    });

    runner.add("ZeroSizeAlloc", [](const char*& fail) -> bool {
        SimTLAB tlab(0x1000, 256);
        auto a = tlab.allocate(0);
        auto b = tlab.allocate(0);
        if (a == 0 || b == 0) { fail = "zero-size should succeed"; return false; }
        return true;
    });
}

static size_t runTLABTests() {
    TLABTestRunner runner;
    registerTLABTests(runner);
    return runner.runAll();
}

} // namespace Zepra::Test
