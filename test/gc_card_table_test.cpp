// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_card_table_test.cpp

#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Test {

struct SimCardTable {
    static constexpr size_t CARD_SIZE = 512;
    enum State : uint8_t { Clean = 0, Dirty = 1, Scanned = 2 };

    std::vector<uint8_t> cards;
    uintptr_t base;

    SimCardTable(uintptr_t b, size_t heapSize)
        : base(b), cards(heapSize / CARD_SIZE, Clean) {}

    size_t idx(uintptr_t addr) const { return (addr - base) / CARD_SIZE; }

    void dirty(uintptr_t addr) {
        size_t i = idx(addr);
        if (i < cards.size()) cards[i] = Dirty;
    }

    void clean(uintptr_t addr) {
        size_t i = idx(addr);
        if (i < cards.size()) cards[i] = Clean;
    }

    bool isDirty(uintptr_t addr) const {
        size_t i = idx(addr);
        return i < cards.size() && cards[i] == Dirty;
    }

    size_t forEachDirty(uintptr_t start, uintptr_t end,
                         std::function<void(uintptr_t)> fn) {
        size_t s = idx(start), e = idx(end);
        if (e > cards.size()) e = cards.size();
        size_t count = 0;
        for (size_t i = s; i < e; i++) {
            if (cards[i] == Dirty) {
                fn(base + i * CARD_SIZE);
                cards[i] = Scanned;
                count++;
            }
        }
        return count;
    }

    size_t dirtyCount() const {
        size_t c = 0;
        for (auto v : cards) if (v == Dirty) c++;
        return c;
    }
};

using CardTestFn = std::function<bool(const char*& fail)>;

class CardTestRunner {
public:
    void add(const char* name, CardTestFn fn) { tests_.push_back({name, std::move(fn)}); }
    size_t runAll() {
        size_t p = 0, f = 0;
        fprintf(stderr, "\n=== Card Table Tests ===\n\n");
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
    std::vector<std::pair<const char*, CardTestFn>> tests_;
};

static void registerCardTests(CardTestRunner& runner) {
    runner.add("DirtyAndCheck", [](const char*& fail) -> bool {
        SimCardTable ct(0x10000, 64 * 1024);
        ct.dirty(0x10000);
        if (!ct.isDirty(0x10000)) { fail = "should be dirty"; return false; }
        if (ct.isDirty(0x10200)) { fail = "should be clean"; return false; }
        return true;
    });

    runner.add("CleanAfterDirty", [](const char*& fail) -> bool {
        SimCardTable ct(0x10000, 64 * 1024);
        ct.dirty(0x10000);
        ct.clean(0x10000);
        if (ct.isDirty(0x10000)) { fail = "should be clean"; return false; }
        return true;
    });

    runner.add("MultipleDirtyCards", [](const char*& fail) -> bool {
        SimCardTable ct(0x10000, 64 * 1024);
        ct.dirty(0x10000);
        ct.dirty(0x10400);  // +1024 = next card
        ct.dirty(0x10800);
        if (ct.dirtyCount() != 3) { fail = "expected 3 dirty"; return false; }
        return true;
    });

    runner.add("ForEachDirtyTransition", [](const char*& fail) -> bool {
        SimCardTable ct(0x10000, 64 * 1024);
        ct.dirty(0x10000);
        ct.dirty(0x10200);

        size_t found = ct.forEachDirty(0x10000, 0x10400, [](uintptr_t) {});
        if (found != 2) { fail = "expected 2 dirty"; return false; }
        // After scan, cards should be Scanned, not Dirty.
        if (ct.isDirty(0x10000)) { fail = "should be scanned"; return false; }
        return true;
    });

    runner.add("SameCardMultipleAddresses", [](const char*& fail) -> bool {
        SimCardTable ct(0x10000, 64 * 1024);
        ct.dirty(0x10000);
        ct.dirty(0x10010);  // Same 512-byte card
        ct.dirty(0x101F0);  // Still same card
        if (ct.dirtyCount() != 1) { fail = "should be 1 card"; return false; }
        return true;
    });

    runner.add("EmptyIteration", [](const char*& fail) -> bool {
        SimCardTable ct(0x10000, 64 * 1024);
        size_t found = ct.forEachDirty(0x10000, 0x20000, [](uintptr_t) {});
        if (found != 0) { fail = "expected 0"; return false; }
        return true;
    });
}

static size_t runCardTests() {
    CardTestRunner runner;
    registerCardTests(runner);
    return runner.runAll();
}

} // namespace Zepra::Test
