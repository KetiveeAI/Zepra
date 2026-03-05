// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_free_list_test.cpp

#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cstdlib>

namespace Zepra::Test {

// Simulated free list for testing.
struct SimFreeList {
    static constexpr size_t NUM_CLASSES = 6;
    static constexpr size_t SIZES[NUM_CLASSES] = {16, 32, 64, 128, 256, 512};

    struct Block { uintptr_t addr; size_t size; };
    std::vector<Block> lists[NUM_CLASSES];
    std::vector<Block> oversize;
    size_t freeBytes = 0;

    static size_t classIdx(size_t bytes) {
        for (size_t i = 0; i < NUM_CLASSES; i++) if (bytes <= SIZES[i]) return i;
        return NUM_CLASSES;
    }

    void returnBlock(uintptr_t addr, size_t size) {
        size_t idx = classIdx(size);
        if (idx >= NUM_CLASSES) { oversize.push_back({addr, size}); }
        else { lists[idx].push_back({addr, size}); }
        freeBytes += size;
    }

    uintptr_t allocate(size_t bytes) {
        size_t idx = classIdx(bytes);
        if (idx >= NUM_CLASSES) return 0;
        if (!lists[idx].empty()) {
            auto b = lists[idx].back();
            lists[idx].pop_back();
            freeBytes -= b.size;
            return b.addr;
        }
        for (size_t i = idx + 1; i < NUM_CLASSES; i++) {
            if (!lists[i].empty()) {
                auto b = lists[i].back();
                lists[i].pop_back();
                freeBytes -= b.size;
                return b.addr;
            }
        }
        return 0;
    }
};

using FLTestFn = std::function<bool(const char*& fail)>;

class FLTestRunner {
public:
    void add(const char* name, FLTestFn fn) { tests_.push_back({name, std::move(fn)}); }
    size_t runAll() {
        size_t p = 0, f = 0;
        fprintf(stderr, "\n=== Free List Tests ===\n\n");
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
    std::vector<std::pair<const char*, FLTestFn>> tests_;
};

static void registerFLTests(FLTestRunner& runner) {
    runner.add("ReturnAndAllocate", [](const char*& fail) -> bool {
        SimFreeList fl;
        fl.returnBlock(0x1000, 32);
        auto addr = fl.allocate(32);
        if (addr != 0x1000) { fail = "should return same block"; return false; }
        return true;
    });

    runner.add("NextLargerClass", [](const char*& fail) -> bool {
        SimFreeList fl;
        fl.returnBlock(0x2000, 64);
        auto addr = fl.allocate(32);
        if (addr != 0x2000) { fail = "should use larger class"; return false; }
        return true;
    });

    runner.add("EmptyReturnsZero", [](const char*& fail) -> bool {
        SimFreeList fl;
        if (fl.allocate(32) != 0) { fail = "empty should return 0"; return false; }
        return true;
    });

    runner.add("MultipleBlocks", [](const char*& fail) -> bool {
        SimFreeList fl;
        fl.returnBlock(0x1000, 32);
        fl.returnBlock(0x2000, 32);
        fl.returnBlock(0x3000, 32);
        auto a = fl.allocate(32);
        auto b = fl.allocate(32);
        auto c = fl.allocate(32);
        if (a == 0 || b == 0 || c == 0) { fail = "all should succeed"; return false; }
        if (fl.allocate(32) != 0) { fail = "fourth should fail"; return false; }
        return true;
    });

    runner.add("FreeBytesTracking", [](const char*& fail) -> bool {
        SimFreeList fl;
        fl.returnBlock(0x1000, 64);
        fl.returnBlock(0x2000, 128);
        if (fl.freeBytes != 192) { fail = "expected 192 free"; return false; }
        fl.allocate(64);
        if (fl.freeBytes != 128) { fail = "expected 128 free"; return false; }
        return true;
    });

    runner.add("OversizeGoesToOversize", [](const char*& fail) -> bool {
        SimFreeList fl;
        fl.returnBlock(0x5000, 1024);
        if (fl.oversize.size() != 1) { fail = "should be in oversize"; return false; }
        return true;
    });
}

static size_t runFLTests() {
    FLTestRunner runner;
    registerFLTests(runner);
    return runner.runAll();
}

} // namespace Zepra::Test
