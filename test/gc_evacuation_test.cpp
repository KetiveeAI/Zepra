// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_evacuation_test.cpp

#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <unordered_map>

namespace Zepra::Test {

struct SimForwarding {
    std::unordered_map<uintptr_t, uintptr_t> table;

    void record(uintptr_t old, uintptr_t neu) { table[old] = neu; }
    uintptr_t lookup(uintptr_t old) const {
        auto it = table.find(old);
        return it != table.end() ? it->second : old;
    }
    bool has(uintptr_t old) const { return table.count(old) > 0; }
    size_t size() const { return table.size(); }
    void clear() { table.clear(); }
};

struct SimObject {
    uint32_t id;
    size_t size;
    bool live;
    std::vector<uintptr_t*> refSlots;
};

using EvacTestFn = std::function<bool(const char*& fail)>;

class EvacTestRunner {
public:
    void add(const char* name, EvacTestFn fn) {
        tests_.push_back({name, std::move(fn)});
    }
    size_t runAll() {
        size_t p = 0, f = 0;
        fprintf(stderr, "\n=== Evacuation Tests ===\n\n");
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
    std::vector<std::pair<const char*, EvacTestFn>> tests_;
};

static void registerEvacTests(EvacTestRunner& runner) {

    runner.add("ForwardingRecordAndLookup", [](const char*& fail) -> bool {
        SimForwarding fwd;
        fwd.record(0x1000, 0x2000);
        if (fwd.lookup(0x1000) != 0x2000) { fail = "wrong forward"; return false; }
        if (fwd.lookup(0x9999) != 0x9999) { fail = "unknown should return same"; return false; }
        return true;
    });

    runner.add("ForwardingMultipleEntries", [](const char*& fail) -> bool {
        SimForwarding fwd;
        for (uint32_t i = 0; i < 100; i++) {
            fwd.record(0x1000 + i * 64, 0x5000 + i * 64);
        }
        if (fwd.size() != 100) { fail = "expected 100 entries"; return false; }
        if (fwd.lookup(0x1000 + 50 * 64) != 0x5000 + 50 * 64) {
            fail = "mid lookup failed"; return false;
        }
        return true;
    });

    runner.add("ForwardingClear", [](const char*& fail) -> bool {
        SimForwarding fwd;
        fwd.record(0x1000, 0x2000);
        fwd.clear();
        if (fwd.size() != 0) { fail = "should be empty"; return false; }
        if (fwd.has(0x1000)) { fail = "should not have entry"; return false; }
        return true;
    });

    runner.add("ReferenceUpdate", [](const char*& fail) -> bool {
        SimForwarding fwd;
        fwd.record(0x1000, 0x3000);
        fwd.record(0x2000, 0x4000);

        uintptr_t ref1 = 0x1000;
        uintptr_t ref2 = 0x2000;
        uintptr_t ref3 = 0x9999;

        ref1 = fwd.lookup(ref1);
        ref2 = fwd.lookup(ref2);
        ref3 = fwd.lookup(ref3);

        if (ref1 != 0x3000) { fail = "ref1 not updated"; return false; }
        if (ref2 != 0x4000) { fail = "ref2 not updated"; return false; }
        if (ref3 != 0x9999) { fail = "ref3 should be unchanged"; return false; }
        return true;
    });

    runner.add("OverwriteForwarding", [](const char*& fail) -> bool {
        SimForwarding fwd;
        fwd.record(0x1000, 0x2000);
        fwd.record(0x1000, 0x3000);
        if (fwd.lookup(0x1000) != 0x3000) { fail = "should use latest"; return false; }
        if (fwd.size() != 1) { fail = "should still be 1 entry"; return false; }
        return true;
    });

    runner.add("LargeScaleEvacuation", [](const char*& fail) -> bool {
        SimForwarding fwd;
        for (uint32_t i = 0; i < 10000; i++) {
            fwd.record(i * 32, 0x100000 + i * 32);
        }
        if (fwd.size() != 10000) { fail = "expected 10k"; return false; }
        // Spot check.
        if (fwd.lookup(5000 * 32) != 0x100000 + 5000 * 32) {
            fail = "spot check failed"; return false;
        }
        return true;
    });
}

static size_t runEvacTests() {
    EvacTestRunner runner;
    registerEvacTests(runner);
    return runner.runAll();
}

} // namespace Zepra::Test
