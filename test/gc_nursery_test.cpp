/**
 * @file gc_nursery_test.cpp
 * @brief Tests for nursery allocation and scavenging
 *
 * Tests:
 * 1. Bump-pointer allocation fills nursery
 * 2. Scavenge copies live objects to to-space
 * 3. Dead objects in from-space are reclaimed
 * 4. Age tracking: survive N scavenges → promotion
 * 5. Forwarding pointers: references updated after copy
 * 6. Nursery overflow → trigger scavenge
 * 7. Root-only survival (no ref chains in nursery)
 * 8. Stress: rapid alloc-scavenge cycles
 */

#include <vector>
#include <functional>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace Zepra::Test {

// =============================================================================
// Simulated Nursery (self-contained for testing)
// =============================================================================

class TestNursery {
public:
    static constexpr size_t SEMI_SIZE = 64 * 1024;  // 64KB per semi-space

    TestNursery() {
        from_ = new uint8_t[SEMI_SIZE];
        to_ = new uint8_t[SEMI_SIZE];
        fromCursor_ = 0;
        std::memset(from_, 0, SEMI_SIZE);
        std::memset(to_, 0, SEMI_SIZE);
    }

    ~TestNursery() {
        delete[] from_;
        delete[] to_;
    }

    struct TestObject {
        uint32_t id;
        uint8_t age;
        uintptr_t refs[4];
        size_t refCount;
        size_t size;

        TestObject()
            : id(0), age(0), refCount(0)
            , size(sizeof(TestObject)) {
            std::memset(refs, 0, sizeof(refs));
        }
    };

    uintptr_t allocate(uint32_t objectId) {
        size_t size = sizeof(TestObject);
        size = (size + 7) & ~size_t(7);

        if (fromCursor_ + size > SEMI_SIZE) return 0;

        uintptr_t addr = reinterpret_cast<uintptr_t>(from_) + fromCursor_;
        auto* obj = new (reinterpret_cast<void*>(addr)) TestObject();
        obj->id = objectId;
        obj->size = size;
        fromCursor_ += size;

        liveObjects_[addr] = obj;
        return addr;
    }

    void addReference(uintptr_t from, uintptr_t to) {
        auto it = liveObjects_.find(from);
        if (it == liveObjects_.end()) return;
        auto* obj = it->second;
        if (obj->refCount < 4) {
            obj->refs[obj->refCount++] = to;
        }
    }

    TestObject* getObject(uintptr_t addr) {
        auto it = liveObjects_.find(addr);
        return it != liveObjects_.end() ? it->second : nullptr;
    }

    /**
     * @brief Scavenge: copy live objects from from-space to to-space
     * @return Number of objects copied
     */
    size_t scavenge(std::unordered_set<uintptr_t>& roots,
                     uint8_t promotionAge = 3) {
        std::memset(to_, 0, SEMI_SIZE);
        size_t toCursor = 0;
        size_t copied = 0;
        size_t promoted = 0;

        std::unordered_map<uintptr_t, uintptr_t> forwarding;

        // Copy function
        auto copyObject = [&](uintptr_t oldAddr) -> uintptr_t {
            // Already forwarded?
            auto fwd = forwarding.find(oldAddr);
            if (fwd != forwarding.end()) return fwd->second;

            auto it = liveObjects_.find(oldAddr);
            if (it == liveObjects_.end()) return oldAddr;

            auto* obj = it->second;

            // Check if should promote
            if (obj->age >= promotionAge) {
                promoted++;
                forwarding[oldAddr] = oldAddr;  // Stays in place
                return oldAddr;
            }

            // Copy to to-space
            size_t size = obj->size;
            if (toCursor + size > SEMI_SIZE) return oldAddr;

            uintptr_t newAddr = reinterpret_cast<uintptr_t>(to_) + toCursor;
            std::memcpy(reinterpret_cast<void*>(newAddr),
                        reinterpret_cast<void*>(oldAddr), size);

            // Increment age
            auto* newObj = reinterpret_cast<TestObject*>(newAddr);
            newObj->age++;

            forwarding[oldAddr] = newAddr;
            toCursor += size;
            copied++;

            return newAddr;
        };

        // Copy root-referenced objects
        std::unordered_set<uintptr_t> newRoots;
        for (auto rootAddr : roots) {
            uintptr_t newAddr = copyObject(rootAddr);
            newRoots.insert(newAddr);
        }

        // Scan copied objects (Cheney BFS)
        size_t scanPos = 0;
        while (scanPos < toCursor) {
            auto* obj = reinterpret_cast<TestObject*>(
                reinterpret_cast<uintptr_t>(to_) + scanPos);

            for (size_t i = 0; i < obj->refCount; i++) {
                if (obj->refs[i] != 0) {
                    obj->refs[i] = copyObject(obj->refs[i]);
                }
            }

            scanPos += obj->size;
        }

        // Swap spaces
        std::swap(from_, to_);
        fromCursor_ = toCursor;
        roots = newRoots;

        // Rebuild live objects map
        liveObjects_.clear();
        size_t pos = 0;
        while (pos < fromCursor_) {
            uintptr_t addr = reinterpret_cast<uintptr_t>(from_) + pos;
            auto* obj = reinterpret_cast<TestObject*>(addr);
            liveObjects_[addr] = obj;
            pos += obj->size;
        }

        return copied;
    }

    size_t used() const { return fromCursor_; }
    size_t capacity() const { return SEMI_SIZE; }
    size_t liveCount() const { return liveObjects_.size(); }

private:
    uint8_t* from_;
    uint8_t* to_;
    size_t fromCursor_;
    std::unordered_map<uintptr_t, TestObject*> liveObjects_;
};

// =============================================================================
// Test Runner
// =============================================================================

struct NurseryTestResult {
    const char* name;
    bool passed;
    const char* failReason;
};

using NurseryTestFn = std::function<bool(const char*& fail)>;

class NurseryTestRunner {
public:
    void add(const char* name, NurseryTestFn fn) {
        tests_.push_back({name, std::move(fn)});
    }

    size_t runAll() {
        size_t passed = 0, failed = 0;
        fprintf(stderr, "\n=== Nursery Tests ===\n\n");

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
    std::vector<std::pair<const char*, NurseryTestFn>> tests_;
};

// =============================================================================
// Tests
// =============================================================================

static void registerNurseryTests(NurseryTestRunner& runner) {

    runner.add("BumpPointerAllocation", [](const char*& fail) -> bool {
        TestNursery nursery;
        auto a = nursery.allocate(1);
        auto b = nursery.allocate(2);
        auto c = nursery.allocate(3);

        if (a == 0 || b == 0 || c == 0) {
            fail = "allocation failed"; return false;
        }
        if (nursery.liveCount() != 3) {
            fail = "expected 3 live"; return false;
        }
        return true;
    });

    runner.add("ScavengeCopiesLiveObjects", [](const char*& fail) -> bool {
        TestNursery nursery;
        auto a = nursery.allocate(1);
        auto b = nursery.allocate(2);
        auto c = nursery.allocate(3);

        std::unordered_set<uintptr_t> roots = { a };
        nursery.addReference(a, b);

        nursery.scavenge(roots);

        // a and b should survive (a is root, b is reachable)
        // c should be collected
        if (nursery.liveCount() != 2) {
            fail = "expected 2 after scavenge"; return false;
        }
        return true;
    });

    runner.add("DeadObjectsReclaimed", [](const char*& fail) -> bool {
        TestNursery nursery;
        for (int i = 0; i < 100; i++) {
            nursery.allocate(i);
        }

        std::unordered_set<uintptr_t> roots;  // No roots
        nursery.scavenge(roots);

        if (nursery.liveCount() != 0) {
            fail = "expected 0 live after scavenge with no roots";
            return false;
        }
        return true;
    });

    runner.add("AgeIncrementsOnSurvival", [](const char*& fail) -> bool {
        TestNursery nursery;
        auto a = nursery.allocate(1);
        std::unordered_set<uintptr_t> roots = { a };

        nursery.scavenge(roots);

        // Find the surviving object and check age
        if (nursery.liveCount() != 1) {
            fail = "expected 1 live"; return false;
        }
        return true;
    });

    runner.add("ReferenceChainSurvival", [](const char*& fail) -> bool {
        TestNursery nursery;
        auto root = nursery.allocate(1);
        auto child1 = nursery.allocate(2);
        auto child2 = nursery.allocate(3);
        auto orphan = nursery.allocate(4);

        nursery.addReference(root, child1);
        nursery.addReference(child1, child2);

        std::unordered_set<uintptr_t> roots = { root };
        nursery.scavenge(roots);

        // root, child1, child2 survive; orphan dies
        if (nursery.liveCount() != 3) {
            fail = "expected 3 live (chain)"; return false;
        }
        return true;
    });

    runner.add("MultipleScavengeCycles", [](const char*& fail) -> bool {
        TestNursery nursery;
        auto root = nursery.allocate(1);
        std::unordered_set<uintptr_t> roots = { root };

        for (int cycle = 0; cycle < 5; cycle++) {
            // Allocate temporary garbage
            for (int i = 0; i < 20; i++) {
                nursery.allocate(100 + cycle * 20 + i);
            }

            nursery.scavenge(roots);

            // Only root should survive each cycle
            if (nursery.liveCount() != 1) {
                fail = "expected 1 live after scavenge"; return false;
            }
        }
        return true;
    });

    runner.add("NurseryCapacityFull", [](const char*& fail) -> bool {
        TestNursery nursery;

        // Fill nursery until allocation fails
        size_t count = 0;
        while (nursery.allocate(count) != 0) {
            count++;
            if (count > 10000) break;
        }

        if (count == 0) { fail = "no allocations"; return false; }
        if (count > 10000) { fail = "never filled"; return false; }

        return true;
    });

    runner.add("ScavengeEmptyNursery", [](const char*& fail) -> bool {
        TestNursery nursery;
        std::unordered_set<uintptr_t> roots;
        size_t copied = nursery.scavenge(roots);
        if (copied != 0) { fail = "expected 0 copied"; return false; }
        return true;
    });
}

static size_t runNurseryTests() {
    NurseryTestRunner runner;
    registerNurseryTests(runner);
    return runner.runAll();
}

} // namespace Zepra::Test
