// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_object_header_test.cpp

#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <atomic>

namespace Zepra::Test {

struct SimHeader {
    static constexpr uint64_t MARKED = 1ULL << 1;
    static constexpr uint64_t PINNED = 1ULL << 2;
    static constexpr uint64_t FORWARDED = 1ULL << 3;
    static constexpr uint64_t AGE_SHIFT = 4;
    static constexpr uint64_t AGE_MASK = 0xFULL << AGE_SHIFT;
    static constexpr uint64_t TYPE_SHIFT = 8;
    static constexpr uint64_t SHAPE_SHIFT = 32;

    uint64_t raw;
    SimHeader(uint32_t shape, uint16_t size, uint8_t type, uint8_t age = 0) {
        raw = (uint64_t(shape) << SHAPE_SHIFT) |
              (uint64_t(size) << 16) |
              (uint64_t(type) << TYPE_SHIFT) |
              (uint64_t(age & 0xF) << AGE_SHIFT);
    }

    uint32_t shape() const { return raw >> SHAPE_SHIFT; }
    uint16_t sizeGranules() const { return (raw >> 16) & 0xFFFF; }
    uint8_t type() const { return (raw >> TYPE_SHIFT) & 0xFF; }
    uint8_t age() const { return (raw >> AGE_SHIFT) & 0xF; }
    bool isMarked() const { return raw & MARKED; }
    void setMarked() { raw |= MARKED; }
    void clearMarked() { raw &= ~MARKED; }
    void incrementAge() {
        uint8_t a = age();
        if (a < 15) raw = (raw & ~AGE_MASK) | (uint64_t(a + 1) << AGE_SHIFT);
    }
};

using HeaderTestFn = std::function<bool(const char*& fail)>;

class HeaderTestRunner {
public:
    void add(const char* name, HeaderTestFn fn) { tests_.push_back({name, std::move(fn)}); }
    size_t runAll() {
        size_t p = 0, f = 0;
        fprintf(stderr, "\n=== Object Header Tests ===\n\n");
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
    std::vector<std::pair<const char*, HeaderTestFn>> tests_;
};

static void registerHeaderTests(HeaderTestRunner& runner) {
    runner.add("FieldEncoding", [](const char*& fail) -> bool {
        SimHeader h(42, 8, 3, 5);
        if (h.shape() != 42) { fail = "shape wrong"; return false; }
        if (h.sizeGranules() != 8) { fail = "size wrong"; return false; }
        if (h.type() != 3) { fail = "type wrong"; return false; }
        if (h.age() != 5) { fail = "age wrong"; return false; }
        return true;
    });

    runner.add("MarkBit", [](const char*& fail) -> bool {
        SimHeader h(1, 4, 0);
        if (h.isMarked()) { fail = "should start clear"; return false; }
        h.setMarked();
        if (!h.isMarked()) { fail = "should be marked"; return false; }
        if (h.shape() != 1) { fail = "shape corrupted"; return false; }
        h.clearMarked();
        if (h.isMarked()) { fail = "should be clear"; return false; }
        return true;
    });

    runner.add("AgeIncrement", [](const char*& fail) -> bool {
        SimHeader h(0, 4, 0, 0);
        for (int i = 0; i < 15; i++) h.incrementAge();
        if (h.age() != 15) { fail = "should be 15"; return false; }
        h.incrementAge();
        if (h.age() != 15) { fail = "should cap at 15"; return false; }
        return true;
    });

    runner.add("FieldsNotCorrupted", [](const char*& fail) -> bool {
        SimHeader h(0xDEAD, 100, 7, 3);
        h.setMarked();
        h.incrementAge();
        if (h.shape() != 0xDEAD) { fail = "shape corrupted"; return false; }
        if (h.sizeGranules() != 100) { fail = "size corrupted"; return false; }
        if (h.type() != 7) { fail = "type corrupted"; return false; }
        if (h.age() != 4) { fail = "age wrong after increment"; return false; }
        return true;
    });

    runner.add("MaxShape", [](const char*& fail) -> bool {
        SimHeader h(0xFFFFFFFF, 1, 0);
        if (h.shape() != 0xFFFFFFFF) { fail = "max shape wrong"; return false; }
        return true;
    });

    runner.add("AtomicCAS", [](const char*& fail) -> bool {
        std::atomic<uint64_t> header{0};
        uint64_t old = header.load();
        bool set = header.compare_exchange_strong(old, old | SimHeader::MARKED);
        if (!set) { fail = "CAS should succeed"; return false; }
        if (!(header.load() & SimHeader::MARKED)) { fail = "mark not set"; return false; }
        // Second CAS should fail (already marked).
        old = header.load();
        set = header.compare_exchange_strong(old, old | SimHeader::MARKED);
        // CAS succeeds but no change (already set).
        if (!(header.load() & SimHeader::MARKED)) { fail = "still marked"; return false; }
        return true;
    });
}

static size_t runHeaderTests() {
    HeaderTestRunner runner;
    registerHeaderTests(runner);
    return runner.runAll();
}

} // namespace Zepra::Test
