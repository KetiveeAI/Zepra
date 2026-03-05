// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_root_set_test.cpp

#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Test {

struct SimRootSet {
    struct Entry { uintptr_t* loc; uint32_t id; };
    std::vector<Entry> roots;
    uint32_t nextId = 1;

    uint32_t add(uintptr_t* loc) {
        uint32_t id = nextId++;
        roots.push_back({loc, id});
        return id;
    }

    void remove(uint32_t id) {
        roots.erase(std::remove_if(roots.begin(), roots.end(),
            [id](const Entry& e) { return e.id == id; }), roots.end());
    }

    void enumerate(std::function<void(uintptr_t)> fn) const {
        for (auto& e : roots) {
            if (e.loc && *e.loc != 0) fn(*e.loc);
        }
    }

    void updateRoots(std::function<uintptr_t(uintptr_t)> fwd) {
        for (auto& e : roots) {
            if (e.loc && *e.loc != 0) {
                *e.loc = fwd(*e.loc);
            }
        }
    }
};

using RootTestFn = std::function<bool(const char*& fail)>;

class RootTestRunner {
public:
    void add(const char* name, RootTestFn fn) { tests_.push_back({name, std::move(fn)}); }
    size_t runAll() {
        size_t p = 0, f = 0;
        fprintf(stderr, "\n=== Root Set Tests ===\n\n");
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
    std::vector<std::pair<const char*, RootTestFn>> tests_;
};

static void registerRootTests(RootTestRunner& runner) {
    runner.add("AddAndEnumerate", [](const char*& fail) -> bool {
        SimRootSet rs;
        uintptr_t val = 0xBEEF;
        rs.add(&val);
        size_t count = 0;
        rs.enumerate([&](uintptr_t v) { if (v == 0xBEEF) count++; });
        if (count != 1) { fail = "expected 1 root"; return false; }
        return true;
    });

    runner.add("RemoveRoot", [](const char*& fail) -> bool {
        SimRootSet rs;
        uintptr_t val = 0x1234;
        uint32_t id = rs.add(&val);
        rs.remove(id);
        size_t count = 0;
        rs.enumerate([&](uintptr_t) { count++; });
        if (count != 0) { fail = "should be empty"; return false; }
        return true;
    });

    runner.add("NullRootSkipped", [](const char*& fail) -> bool {
        SimRootSet rs;
        uintptr_t val = 0;
        rs.add(&val);
        size_t count = 0;
        rs.enumerate([&](uintptr_t) { count++; });
        if (count != 0) { fail = "null root should skip"; return false; }
        return true;
    });

    runner.add("UpdateAfterCompaction", [](const char*& fail) -> bool {
        SimRootSet rs;
        uintptr_t val1 = 0x1000, val2 = 0x2000;
        rs.add(&val1);
        rs.add(&val2);
        rs.updateRoots([](uintptr_t old) { return old + 0x5000; });
        if (val1 != 0x6000) { fail = "val1 not updated"; return false; }
        if (val2 != 0x7000) { fail = "val2 not updated"; return false; }
        return true;
    });

    runner.add("MultipleRoots", [](const char*& fail) -> bool {
        SimRootSet rs;
        uintptr_t vals[5] = {0x10, 0x20, 0x30, 0x40, 0x50};
        for (auto& v : vals) rs.add(&v);
        size_t count = 0;
        rs.enumerate([&](uintptr_t) { count++; });
        if (count != 5) { fail = "expected 5 roots"; return false; }
        return true;
    });

    runner.add("RemoveMiddle", [](const char*& fail) -> bool {
        SimRootSet rs;
        uintptr_t a = 1, b = 2, c = 3;
        rs.add(&a);
        uint32_t bId = rs.add(&b);
        rs.add(&c);
        rs.remove(bId);
        if (rs.roots.size() != 2) { fail = "expected 2 after remove"; return false; }
        return true;
    });
}

static size_t runRootTests() {
    RootTestRunner runner;
    registerRootTests(runner);
    return runner.runAll();
}

} // namespace Zepra::Test
