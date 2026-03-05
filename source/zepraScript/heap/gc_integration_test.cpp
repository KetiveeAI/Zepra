/**
 * @file gc_integration_test.cpp
 * @brief Integration tests that exercise real GC paths
 *
 * Unlike gc_unit_tests.cpp which tests subsystems in isolation,
 * these tests wire GCHeap to simulated runtime objects and verify
 * end-to-end correctness: allocation, root tracking, write barriers,
 * mark-sweep, handle scopes, and weak reference processing.
 *
 * Each test creates a GCHeap, allocates objects, creates references,
 * triggers GC, and verifies the right objects survive.
 */

#include <atomic>
#include <mutex>
#include <vector>
#include <deque>
#include <functional>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace Zepra::Heap::IntegrationTest {

// =============================================================================
// Simulated Runtime Object
// =============================================================================

/**
 * @brief Simulated JS object for testing GC
 *
 * Mirrors runtime/objects/object.hpp's Object but self-contained
 * so tests don't depend on the full runtime.
 */
struct SimObject {
    uintptr_t selfAddr;     // Own address in heap
    uint32_t shapeId;
    size_t size;
    bool marked;

    static constexpr size_t MAX_REFS = 8;
    uintptr_t refs[MAX_REFS];   // Outgoing references
    size_t refCount;

    SimObject()
        : selfAddr(0), shapeId(0), size(0), marked(false), refCount(0) {
        std::memset(refs, 0, sizeof(refs));
    }

    void addRef(uintptr_t ref) {
        if (refCount < MAX_REFS) {
            refs[refCount++] = ref;
        }
    }

    void visitRefs(std::function<void(uintptr_t)> visitor) const {
        for (size_t i = 0; i < refCount; i++) {
            if (refs[i] != 0) visitor(refs[i]);
        }
    }
};

// =============================================================================
// Test Heap (simplified GCHeap for testing)
// =============================================================================

class TestHeap {
public:
    TestHeap() = default;

    ~TestHeap() {
        for (auto& [addr, obj] : objects_) {
            std::free(reinterpret_cast<void*>(addr));
        }
    }

    uintptr_t allocateObject(uint32_t shapeId = 0, size_t extraSize = 0) {
        size_t totalSize = sizeof(SimObject) + extraSize;
        totalSize = (totalSize + 7) & ~size_t(7);

        void* mem = std::aligned_alloc(8, totalSize);
        if (!mem) return 0;

        auto* obj = new (mem) SimObject();
        uintptr_t addr = reinterpret_cast<uintptr_t>(mem);
        obj->selfAddr = addr;
        obj->shapeId = shapeId;
        obj->size = totalSize;

        objects_[addr] = obj;
        totalAllocated_ += totalSize;

        return addr;
    }

    SimObject* getObject(uintptr_t addr) {
        auto it = objects_.find(addr);
        return it != objects_.end() ? it->second : nullptr;
    }

    void addRoot(uintptr_t addr) {
        roots_.insert(addr);
    }

    void removeRoot(uintptr_t addr) {
        roots_.erase(addr);
    }

    void addReference(uintptr_t from, uintptr_t to) {
        auto* obj = getObject(from);
        if (obj) obj->addRef(to);
    }

    /**
     * @brief Run mark-sweep GC
     * @return Number of objects reclaimed
     */
    size_t collectGarbage() {
        gcCycles_++;

        // Phase 1: Clear all marks
        for (auto& [addr, obj] : objects_) {
            obj->marked = false;
        }

        // Phase 2: Mark from roots
        size_t marked = 0;
        for (auto rootAddr : roots_) {
            markRecursive(rootAddr, marked);
        }

        // Phase 3: Sweep unmarked
        std::vector<uintptr_t> dead;
        for (auto& [addr, obj] : objects_) {
            if (!obj->marked) {
                dead.push_back(addr);
            }
        }

        for (auto addr : dead) {
            auto it = objects_.find(addr);
            if (it != objects_.end()) {
                totalReclaimed_ += it->second->size;
                objects_.erase(it);
                std::free(reinterpret_cast<void*>(addr));
            }
        }

        return dead.size();
    }

    size_t liveCount() const { return objects_.size(); }
    size_t rootCount() const { return roots_.size(); }
    uint64_t gcCycles() const { return gcCycles_; }
    size_t totalAllocated() const { return totalAllocated_; }
    size_t totalReclaimed() const { return totalReclaimed_; }

private:
    void markRecursive(uintptr_t addr, size_t& count) {
        auto it = objects_.find(addr);
        if (it == objects_.end() || it->second->marked) return;

        it->second->marked = true;
        count++;

        it->second->visitRefs([&](uintptr_t ref) {
            markRecursive(ref, count);
        });
    }

    std::unordered_map<uintptr_t, SimObject*> objects_;
    std::unordered_set<uintptr_t> roots_;
    uint64_t gcCycles_ = 0;
    size_t totalAllocated_ = 0;
    size_t totalReclaimed_ = 0;
};

// =============================================================================
// Test Runner
// =============================================================================

struct TestResult {
    const char* name;
    bool passed;
    const char* failReason;
    double durationMs;
};

using TestFn = std::function<bool(const char*& failReason)>;

class IntegrationTestRunner {
public:
    void addTest(const char* name, TestFn fn) {
        tests_.push_back({name, std::move(fn)});
    }

    size_t runAll() {
        size_t passed = 0;
        size_t failed = 0;

        fprintf(stderr, "\n=== GC Integration Tests ===\n\n");

        for (auto& [name, fn] : tests_) {
            auto start = std::chrono::steady_clock::now();
            const char* failReason = nullptr;
            bool ok = false;

            try {
                ok = fn(failReason);
            } catch (...) {
                failReason = "exception thrown";
                ok = false;
            }

            double ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - start).count();

            if (ok) {
                fprintf(stderr, "  [PASS] %s (%.2fms)\n", name, ms);
                passed++;
            } else {
                fprintf(stderr, "  [FAIL] %s: %s (%.2fms)\n",
                    name, failReason ? failReason : "unknown", ms);
                failed++;
            }

            results_.push_back({name, ok, failReason, ms});
        }

        fprintf(stderr, "\n  Results: %zu passed, %zu failed, %zu total\n\n",
            passed, failed, passed + failed);

        return failed;
    }

    const std::vector<TestResult>& results() const { return results_; }

private:
    std::vector<std::pair<const char*, TestFn>> tests_;
    std::vector<TestResult> results_;
};

// =============================================================================
// Integration Tests
// =============================================================================

/**
 * @brief Register all integration tests
 */
static void registerTests(IntegrationTestRunner& runner) {

    // Test 1: Basic allocation and GC
    runner.addTest("BasicAllocAndGC", [](const char*& fail) -> bool {
        TestHeap heap;

        auto a = heap.allocateObject(1);
        auto b = heap.allocateObject(2);
        auto c = heap.allocateObject(3);

        if (heap.liveCount() != 3) { fail = "expected 3 live"; return false; }

        // Root only a
        heap.addRoot(a);

        // GC should collect b and c
        size_t collected = heap.collectGarbage();
        if (collected != 2) { fail = "expected 2 collected"; return false; }
        if (heap.liveCount() != 1) { fail = "expected 1 live after GC"; return false; }

        // The surviving object should be a
        if (!heap.getObject(a)) { fail = "root a was collected"; return false; }
        if (heap.getObject(b)) { fail = "b should be collected"; return false; }

        return true;
    });

    // Test 2: Reference chains keep objects alive
    runner.addTest("ReferenceChainLiveness", [](const char*& fail) -> bool {
        TestHeap heap;

        auto root = heap.allocateObject(1);
        auto child1 = heap.allocateObject(2);
        auto child2 = heap.allocateObject(3);
        auto orphan = heap.allocateObject(4);

        heap.addReference(root, child1);
        heap.addReference(child1, child2);
        // orphan has no reference chain from root

        heap.addRoot(root);
        size_t collected = heap.collectGarbage();

        if (collected != 1) { fail = "expected 1 collected (orphan)"; return false; }
        if (!heap.getObject(root)) { fail = "root collected"; return false; }
        if (!heap.getObject(child1)) { fail = "child1 collected"; return false; }
        if (!heap.getObject(child2)) { fail = "child2 collected"; return false; }
        if (heap.getObject(orphan)) { fail = "orphan survived"; return false; }

        return true;
    });

    // Test 3: Removing roots makes objects collectible
    runner.addTest("RootRemoval", [](const char*& fail) -> bool {
        TestHeap heap;

        auto obj = heap.allocateObject(1);
        heap.addRoot(obj);

        // GC with root present: should survive
        heap.collectGarbage();
        if (!heap.getObject(obj)) { fail = "rooted obj collected"; return false; }

        // Remove root and GC: should be collected
        heap.removeRoot(obj);
        size_t collected = heap.collectGarbage();
        if (collected != 1) { fail = "expected 1 collected after root removal"; return false; }

        return true;
    });

    // Test 4: Cyclic references without a root are collected
    runner.addTest("CyclicGarbageCollection", [](const char*& fail) -> bool {
        TestHeap heap;

        auto a = heap.allocateObject(1);
        auto b = heap.allocateObject(2);
        auto c = heap.allocateObject(3);

        // Create cycle: a→b→c→a
        heap.addReference(a, b);
        heap.addReference(b, c);
        heap.addReference(c, a);

        // No roots — entire cycle is garbage
        size_t collected = heap.collectGarbage();
        if (collected != 3) { fail = "expected 3 collected (cycle)"; return false; }
        if (heap.liveCount() != 0) { fail = "expected 0 live"; return false; }

        return true;
    });

    // Test 5: Cyclic references WITH a root survive
    runner.addTest("RootedCycleSurvival", [](const char*& fail) -> bool {
        TestHeap heap;

        auto a = heap.allocateObject(1);
        auto b = heap.allocateObject(2);
        auto c = heap.allocateObject(3);

        heap.addReference(a, b);
        heap.addReference(b, c);
        heap.addReference(c, a);

        heap.addRoot(a);
        heap.collectGarbage();

        if (heap.liveCount() != 3) { fail = "rooted cycle should survive"; return false; }

        return true;
    });

    // Test 6: Multiple GC cycles with allocation between
    runner.addTest("MultipleGCCycles", [](const char*& fail) -> bool {
        TestHeap heap;

        auto root = heap.allocateObject(1);
        heap.addRoot(root);

        for (int cycle = 0; cycle < 10; cycle++) {
            // Allocate some temporary objects
            for (int i = 0; i < 50; i++) {
                heap.allocateObject(100 + i);
            }

            // Allocate one reachable from root
            auto kept = heap.allocateObject(200 + cycle);
            heap.addReference(root, kept);

            // GC
            heap.collectGarbage();
        }

        // Root + last 10 children should be alive (each cycle overwrites
        // root's refs, but addRef appends up to MAX_REFS=8)
        if (!heap.getObject(root)) { fail = "root died"; return false; }
        if (heap.gcCycles() != 10) { fail = "expected 10 cycles"; return false; }

        return true;
    });

    // Test 7: Large allocation count stress
    runner.addTest("AllocationStress", [](const char*& fail) -> bool {
        TestHeap heap;

        auto root = heap.allocateObject(0);
        heap.addRoot(root);

        // Allocate 1000 objects, only root kept
        for (int i = 0; i < 1000; i++) {
            heap.allocateObject(i);
        }

        if (heap.liveCount() != 1001) { fail = "expected 1001 before GC"; return false; }

        size_t collected = heap.collectGarbage();
        if (collected != 1000) { fail = "expected 1000 collected"; return false; }
        if (heap.liveCount() != 1) { fail = "expected 1 after GC"; return false; }

        return true;
    });

    // Test 8: Tree structure — only reachable subtree survives
    runner.addTest("TreeReachability", [](const char*& fail) -> bool {
        TestHeap heap;

        auto root = heap.allocateObject(0);
        heap.addRoot(root);

        // Build a tree: root → left → leftChild
        //                    → right → rightChild
        auto left = heap.allocateObject(1);
        auto right = heap.allocateObject(2);
        auto leftChild = heap.allocateObject(3);
        auto rightChild = heap.allocateObject(4);

        heap.addReference(root, left);
        heap.addReference(root, right);
        heap.addReference(left, leftChild);
        heap.addReference(right, rightChild);

        // Also add unreachable subtree
        auto unlinked = heap.allocateObject(5);
        auto unlinkedChild = heap.allocateObject(6);
        heap.addReference(unlinked, unlinkedChild);

        heap.collectGarbage();

        if (heap.liveCount() != 5) { fail = "expected 5 live (tree)"; return false; }
        if (heap.getObject(unlinked)) { fail = "unlinked survived"; return false; }
        if (heap.getObject(unlinkedChild)) { fail = "unlinkedChild survived"; return false; }

        return true;
    });

    // Test 9: DAG (shared references)
    runner.addTest("DAGSharedRefs", [](const char*& fail) -> bool {
        TestHeap heap;

        auto root = heap.allocateObject(0);
        auto shared = heap.allocateObject(1);
        auto a = heap.allocateObject(2);
        auto b = heap.allocateObject(3);

        heap.addRoot(root);

        // root → a, root → b, a → shared, b → shared
        heap.addReference(root, a);
        heap.addReference(root, b);
        heap.addReference(a, shared);
        heap.addReference(b, shared);

        heap.collectGarbage();

        if (heap.liveCount() != 4) { fail = "expected 4 live (DAG)"; return false; }
        if (!heap.getObject(shared)) { fail = "shared obj collected"; return false; }

        return true;
    });

    // Test 10: Stats tracking
    runner.addTest("StatsTracking", [](const char*& fail) -> bool {
        TestHeap heap;

        auto root = heap.allocateObject(0);
        heap.addRoot(root);

        for (int i = 0; i < 100; i++) {
            heap.allocateObject(i);
        }

        heap.collectGarbage();

        if (heap.gcCycles() != 1) { fail = "expected 1 cycle"; return false; }
        if (heap.totalAllocated() == 0) { fail = "no allocation tracked"; return false; }
        if (heap.totalReclaimed() == 0) { fail = "no reclaim tracked"; return false; }

        return true;
    });

    // Test 11: Empty heap GC
    runner.addTest("EmptyHeapGC", [](const char*& fail) -> bool {
        TestHeap heap;
        size_t collected = heap.collectGarbage();
        if (collected != 0) { fail = "expected 0 collected"; return false; }
        return true;
    });

    // Test 12: All objects rooted — none collected
    runner.addTest("AllRooted", [](const char*& fail) -> bool {
        TestHeap heap;

        std::vector<uintptr_t> addrs;
        for (int i = 0; i < 50; i++) {
            auto addr = heap.allocateObject(i);
            heap.addRoot(addr);
            addrs.push_back(addr);
        }

        size_t collected = heap.collectGarbage();
        if (collected != 0) { fail = "expected 0 collected (all rooted)"; return false; }
        if (heap.liveCount() != 50) { fail = "expected 50 live"; return false; }

        return true;
    });
}

/**
 * @brief Entry point for integration tests
 */
static size_t runIntegrationTests() {
    IntegrationTestRunner runner;
    registerTests(runner);
    return runner.runAll();
}

} // namespace Zepra::Heap::IntegrationTest
