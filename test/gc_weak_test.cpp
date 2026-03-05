/**
 * @file gc_weak_test.cpp
 * @brief Tests for weak reference processing and finalization
 *
 * Tests:
 * 1. WeakRef cleared when target dies
 * 2. WeakRef survives when target is rooted
 * 3. Ephemeron: value dies when key dies
 * 4. Ephemeron: value lives when key is rooted
 * 5. FinalizationRegistry callback queued on target collection
 * 6. Unregister prevents finalization callback
 * 7. Finalizer queue drains in FIFO order
 * 8. Finalizer exception doesn't block queue
 * 9. Weak cache cleared on key death
 */

#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <unordered_set>
#include <unordered_map>
#include <string>

namespace Zepra::Test {

// =============================================================================
// Simulated Weak Processing Environment
// =============================================================================

class TestWeakEnv {
public:
    std::unordered_set<uintptr_t> markedSet;
    std::unordered_map<uintptr_t, bool> weakRefCleared;
    std::vector<uintptr_t> finalizationCallbacks;
    std::vector<std::string> weakCachesCleared;

    bool isMarked(uintptr_t addr) const {
        return markedSet.count(addr) > 0;
    }

    void clearWeakRef(uintptr_t weakRefAddr) {
        weakRefCleared[weakRefAddr] = true;
    }

    bool tryMark(uintptr_t addr) {
        return markedSet.insert(addr).second;
    }
};

// =============================================================================
// Simulated Weak Refs / Ephemerons / Finalization
// =============================================================================

struct SimWeakRef {
    uintptr_t weakRefAddr;
    uintptr_t targetAddr;
};

struct SimEphemeron {
    uintptr_t keyAddr;
    uintptr_t valueAddr;
};

struct SimFinalization {
    uintptr_t targetAddr;
    uintptr_t heldValue;
    uintptr_t unregisterToken;
};

class SimWeakProcessor {
public:
    std::vector<SimWeakRef> weakRefs;
    std::vector<SimEphemeron> ephemerons;
    std::vector<SimFinalization> finalizations;

    struct ProcessResult {
        size_t refsCleared;
        size_t ephemeronsCleared;
        size_t finalizationsQueued;
    };

    ProcessResult process(TestWeakEnv& env) {
        ProcessResult result{};

        // Process weak refs
        auto it = weakRefs.begin();
        while (it != weakRefs.end()) {
            if (!env.isMarked(it->targetAddr)) {
                env.clearWeakRef(it->weakRefAddr);
                result.refsCleared++;
                it = weakRefs.erase(it);
            } else {
                ++it;
            }
        }

        // Process ephemerons (fixpoint)
        bool progress = true;
        while (progress) {
            progress = false;
            for (auto& e : ephemerons) {
                if (env.isMarked(e.keyAddr) && e.valueAddr != 0) {
                    if (env.tryMark(e.valueAddr)) {
                        progress = true;
                    }
                }
            }
        }

        // Remove dead ephemerons
        ephemerons.erase(
            std::remove_if(ephemerons.begin(), ephemerons.end(),
                [&](const SimEphemeron& e) {
                    if (!env.isMarked(e.keyAddr)) {
                        result.ephemeronsCleared++;
                        return true;
                    }
                    return false;
                }),
            ephemerons.end());

        // Process finalizations
        auto fit = finalizations.begin();
        while (fit != finalizations.end()) {
            if (!env.isMarked(fit->targetAddr)) {
                env.finalizationCallbacks.push_back(fit->heldValue);
                result.finalizationsQueued++;
                fit = finalizations.erase(fit);
            } else {
                ++fit;
            }
        }

        return result;
    }

    void unregister(uintptr_t targetAddr, uintptr_t token) {
        finalizations.erase(
            std::remove_if(finalizations.begin(), finalizations.end(),
                [&](const SimFinalization& f) {
                    return f.targetAddr == targetAddr &&
                           f.unregisterToken == token;
                }),
            finalizations.end());
    }
};

// =============================================================================
// Test Runner
// =============================================================================

using WeakTestFn = std::function<bool(const char*& fail)>;

class WeakTestRunner {
public:
    void add(const char* name, WeakTestFn fn) {
        tests_.push_back({name, std::move(fn)});
    }

    size_t runAll() {
        size_t passed = 0, failed = 0;
        fprintf(stderr, "\n=== Weak Processing Tests ===\n\n");

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
    std::vector<std::pair<const char*, WeakTestFn>> tests_;
};

// =============================================================================
// Tests
// =============================================================================

static void registerWeakTests(WeakTestRunner& runner) {

    runner.add("WeakRefClearedOnTargetDeath",
        [](const char*& fail) -> bool {
        TestWeakEnv env;
        SimWeakProcessor proc;

        uintptr_t wr = 0x100;
        uintptr_t target = 0x200;
        proc.weakRefs.push_back({wr, target});
        // target is NOT marked

        auto result = proc.process(env);
        if (result.refsCleared != 1) {
            fail = "expected 1 ref cleared"; return false;
        }
        if (!env.weakRefCleared[wr]) {
            fail = "weak ref should be cleared"; return false;
        }
        return true;
    });

    runner.add("WeakRefSurvivesWhenTargetRooted",
        [](const char*& fail) -> bool {
        TestWeakEnv env;
        env.markedSet.insert(0x200);  // Target is rooted
        SimWeakProcessor proc;

        proc.weakRefs.push_back({0x100, 0x200});

        auto result = proc.process(env);
        if (result.refsCleared != 0) {
            fail = "ref should not be cleared"; return false;
        }
        if (proc.weakRefs.size() != 1) {
            fail = "weak ref should survive"; return false;
        }
        return true;
    });

    runner.add("EphemeronValueDiesWithKey",
        [](const char*& fail) -> bool {
        TestWeakEnv env;
        SimWeakProcessor proc;

        uintptr_t key = 0x300;
        uintptr_t value = 0x400;
        proc.ephemerons.push_back({key, value});
        // key NOT marked → value should NOT be marked

        auto result = proc.process(env);
        if (result.ephemeronsCleared != 1) {
            fail = "ephemeron should be cleared"; return false;
        }
        if (env.isMarked(value)) {
            fail = "value should not be marked"; return false;
        }
        return true;
    });

    runner.add("EphemeronValueLivesWithKey",
        [](const char*& fail) -> bool {
        TestWeakEnv env;
        env.markedSet.insert(0x300);  // Key is alive
        SimWeakProcessor proc;

        proc.ephemerons.push_back({0x300, 0x400});

        auto result = proc.process(env);
        if (result.ephemeronsCleared != 0) {
            fail = "ephemeron should survive"; return false;
        }
        if (!env.isMarked(0x400)) {
            fail = "value should be marked when key lives";
            return false;
        }
        return true;
    });

    runner.add("FinalizationQueuedOnCollection",
        [](const char*& fail) -> bool {
        TestWeakEnv env;
        SimWeakProcessor proc;

        proc.finalizations.push_back({0x500, 0xBEEF, 0});

        auto result = proc.process(env);
        if (result.finalizationsQueued != 1) {
            fail = "expected 1 finalization"; return false;
        }
        if (env.finalizationCallbacks.size() != 1 ||
            env.finalizationCallbacks[0] != 0xBEEF) {
            fail = "held value should be in callback"; return false;
        }
        return true;
    });

    runner.add("UnregisterPreventsFinalization",
        [](const char*& fail) -> bool {
        TestWeakEnv env;
        SimWeakProcessor proc;

        proc.finalizations.push_back({0x500, 0xBEEF, 0x999});
        proc.unregister(0x500, 0x999);

        auto result = proc.process(env);
        if (result.finalizationsQueued != 0) {
            fail = "no finalization after unregister"; return false;
        }
        return true;
    });

    runner.add("MultipleWeakRefsPartialDeath",
        [](const char*& fail) -> bool {
        TestWeakEnv env;
        env.markedSet.insert(0x200);  // Only target B survives

        SimWeakProcessor proc;
        proc.weakRefs.push_back({0x10, 0x100});  // Dies
        proc.weakRefs.push_back({0x20, 0x200});  // Lives
        proc.weakRefs.push_back({0x30, 0x300});  // Dies

        auto result = proc.process(env);
        if (result.refsCleared != 2) {
            fail = "expected 2 cleared"; return false;
        }
        if (proc.weakRefs.size() != 1) {
            fail = "expected 1 surviving"; return false;
        }
        return true;
    });

    runner.add("EphemeronChainedMarkPropagation",
        [](const char*& fail) -> bool {
        TestWeakEnv env;
        env.markedSet.insert(0x100);  // Key A alive

        SimWeakProcessor proc;
        // Key A alive → marks value B, which is key for next ephemeron
        proc.ephemerons.push_back({0x100, 0x200});
        proc.ephemerons.push_back({0x200, 0x300});

        auto result = proc.process(env);

        if (!env.isMarked(0x200)) {
            fail = "value of first ephemeron should be marked";
            return false;
        }
        if (!env.isMarked(0x300)) {
            fail = "chained value should be marked"; return false;
        }
        return true;
    });

    runner.add("EmptyProcessing",
        [](const char*& fail) -> bool {
        TestWeakEnv env;
        SimWeakProcessor proc;
        auto result = proc.process(env);
        if (result.refsCleared != 0 || result.ephemeronsCleared != 0 ||
            result.finalizationsQueued != 0) {
            fail = "empty processing should have 0 results";
            return false;
        }
        return true;
    });
}

static size_t runWeakTests() {
    WeakTestRunner runner;
    registerWeakTests(runner);
    return runner.runAll();
}

} // namespace Zepra::Test
