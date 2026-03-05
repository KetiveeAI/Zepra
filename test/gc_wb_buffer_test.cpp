// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_wb_buffer_test.cpp

#include <vector>
#include <functional>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Test {

struct SimWBBuffer {
    static constexpr size_t CAP = 8;  // Small for testing
    struct Entry { uintptr_t src, oldVal, newVal; };

    Entry entries[CAP];
    size_t cursor = 0;

    bool record(uintptr_t src, uintptr_t oldV, uintptr_t newV) {
        if (cursor >= CAP) return true;
        entries[cursor++] = {src, oldV, newV};
        return cursor >= CAP;
    }

    size_t flush(std::function<void(uintptr_t, uintptr_t, uintptr_t)> proc) {
        size_t count = cursor;
        for (size_t i = 0; i < count; i++) {
            proc(entries[i].src, entries[i].oldVal, entries[i].newVal);
        }
        cursor = 0;
        return count;
    }

    size_t pending() const { return cursor; }
    bool isFull() const { return cursor >= CAP; }
    void reset() { cursor = 0; }
};

using WBTestFn = std::function<bool(const char*& fail)>;

class WBTestRunner {
public:
    void add(const char* name, WBTestFn fn) { tests_.push_back({name, std::move(fn)}); }
    size_t runAll() {
        size_t p = 0, f = 0;
        fprintf(stderr, "\n=== WB Buffer Tests ===\n\n");
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
    std::vector<std::pair<const char*, WBTestFn>> tests_;
};

static void registerWBTests(WBTestRunner& runner) {
    runner.add("RecordAndFlush", [](const char*& fail) -> bool {
        SimWBBuffer buf;
        buf.record(0x100, 0x200, 0x300);
        if (buf.pending() != 1) { fail = "expected 1 pending"; return false; }
        size_t count = 0;
        buf.flush([&](uintptr_t s, uintptr_t o, uintptr_t n) {
            if (s == 0x100 && o == 0x200 && n == 0x300) count++;
        });
        if (count != 1) { fail = "expected 1 flushed"; return false; }
        if (buf.pending() != 0) { fail = "should be empty"; return false; }
        return true;
    });

    runner.add("FillToCapacity", [](const char*& fail) -> bool {
        SimWBBuffer buf;
        for (size_t i = 0; i < 7; i++) {
            if (buf.record(i, 0, 0)) { fail = "should not be full yet"; return false; }
        }
        bool full = buf.record(7, 0, 0);
        if (!full) { fail = "should be full"; return false; }
        return true;
    });

    runner.add("OverflowRejects", [](const char*& fail) -> bool {
        SimWBBuffer buf;
        for (size_t i = 0; i < 10; i++) buf.record(i, 0, 0);
        if (buf.pending() != 8) { fail = "should cap at 8"; return false; }
        return true;
    });

    runner.add("FlushCallsAllEntries", [](const char*& fail) -> bool {
        SimWBBuffer buf;
        for (size_t i = 0; i < 5; i++) buf.record(i, i * 2, i * 3);
        size_t count = buf.flush([](uintptr_t, uintptr_t, uintptr_t) {});
        if (count != 5) { fail = "expected 5"; return false; }
        return true;
    });

    runner.add("ResetClearsBuffer", [](const char*& fail) -> bool {
        SimWBBuffer buf;
        buf.record(1, 2, 3);
        buf.reset();
        if (buf.pending() != 0) { fail = "should be 0"; return false; }
        return true;
    });

    runner.add("FlushPreservesOrder", [](const char*& fail) -> bool {
        SimWBBuffer buf;
        buf.record(10, 0, 0);
        buf.record(20, 0, 0);
        buf.record(30, 0, 0);
        std::vector<uintptr_t> order;
        buf.flush([&](uintptr_t s, uintptr_t, uintptr_t) { order.push_back(s); });
        if (order.size() != 3) { fail = "expected 3"; return false; }
        if (order[0] != 10 || order[1] != 20 || order[2] != 30) {
            fail = "wrong order"; return false;
        }
        return true;
    });
}

static size_t runWBTests() {
    WBTestRunner runner;
    registerWBTests(runner);
    return runner.runAll();
}

} // namespace Zepra::Test
