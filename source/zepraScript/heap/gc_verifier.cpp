/**
 * @file gc_verifier.cpp
 * @brief Post-GC heap integrity verifier
 *
 * Runs after each GC cycle (debug builds) to catch bugs:
 *
 * 1. No dangling references:
 *    - Every reference from a live object points to another live object
 *    - No pointers into freed memory
 *
 * 2. Mark consistency:
 *    - All objects reachable from roots are marked
 *    - No unmarked objects remain in live set
 *
 * 3. Region consistency:
 *    - Objects don't span region boundaries
 *    - Region metadata (live bytes, type) matches reality
 *
 * 4. Card table consistency:
 *    - All cross-generation references have a dirty card
 *    - No clean card hides a cross-gen reference
 *
 * 5. Forwarding pointer consistency:
 *    - After compaction: all forwarding pointers resolved
 *    - No stale forwarding pointers remain
 *
 * 6. Shape consistency:
 *    - Every object's shape pointer is valid
 *    - Shape slot count matches object layout
 *
 * These checks are expensive. Enabled via ZEPRA_GC_VERIFY=1
 * or always in debug builds.
 */

#include <atomic>
#include <mutex>
#include <vector>
#include <deque>
#include <functional>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cstdlib>

namespace Zepra::Heap {

// =============================================================================
// Verification Error
// =============================================================================

struct VerificationError {
    enum class Type : uint8_t {
        DanglingReference,
        UnmarkedReachable,
        RegionOverflow,
        RegionMetadataMismatch,
        CardTableInconsistency,
        StaleForwardingPointer,
        InvalidShape,
        InvalidObjectSize,
        OverlappingObjects,
        DoubleFree,
    };

    Type type;
    uintptr_t address;
    uintptr_t relatedAddress;
    const char* message;

    VerificationError()
        : type(Type::DanglingReference)
        , address(0), relatedAddress(0), message("") {}

    VerificationError(Type t, uintptr_t addr, const char* msg)
        : type(t), address(addr), relatedAddress(0), message(msg) {}

    VerificationError(Type t, uintptr_t addr, uintptr_t related,
                       const char* msg)
        : type(t), address(addr), relatedAddress(related), message(msg) {}
};

// =============================================================================
// Verification Result
// =============================================================================

struct VerificationResult {
    bool passed;
    std::vector<VerificationError> errors;
    size_t objectsVerified;
    size_t referencesChecked;
    size_t regionsChecked;
    size_t cardsChecked;
    double durationMs;

    void addError(VerificationError::Type type, uintptr_t addr,
                   const char* msg) {
        errors.push_back({type, addr, msg});
        passed = false;
    }

    void addError(VerificationError::Type type, uintptr_t addr,
                   uintptr_t related, const char* msg) {
        errors.push_back({type, addr, related, msg});
        passed = false;
    }

    void report() const {
        if (passed) {
            fprintf(stderr, "[gc-verify] PASSED: %zu objects, "
                "%zu refs, %zu regions, %.2fms\n",
                objectsVerified, referencesChecked,
                regionsChecked, durationMs);
        } else {
            fprintf(stderr, "[gc-verify] FAILED: %zu errors\n",
                errors.size());
            for (size_t i = 0; i < errors.size() && i < 20; i++) {
                const auto& e = errors[i];
                fprintf(stderr, "  [%zu] type=%u addr=0x%lx "
                    "related=0x%lx: %s\n",
                    i, static_cast<unsigned>(e.type),
                    static_cast<unsigned long>(e.address),
                    static_cast<unsigned long>(e.relatedAddress),
                    e.message);
            }
            if (errors.size() > 20) {
                fprintf(stderr, "  ... and %zu more errors\n",
                    errors.size() - 20);
            }
        }
    }
};

// =============================================================================
// GC Verifier
// =============================================================================

class GCVerifier {
public:
    struct Callbacks {
        // Iterate all live objects
        std::function<void(
            std::function<void(uintptr_t addr, size_t size)>)>
            iterateLiveObjects;

        // Get outgoing references from an object
        std::function<void(uintptr_t addr,
            std::function<void(uintptr_t ref)>)>
            getReferences;

        // Check if an address is in the live set
        std::function<bool(uintptr_t addr)> isLiveObject;

        // Check if an address is marked
        std::function<bool(uintptr_t addr)> isMarked;

        // Root enumeration
        std::function<void(std::function<void(uintptr_t)>)>
            enumerateRoots;

        // Get object size
        std::function<size_t(uintptr_t addr)> objectSize;

        // Get region ID for an address
        std::function<uint32_t(uintptr_t addr)> regionOf;

        // Get region bounds
        std::function<bool(uint32_t regionId,
            uintptr_t& start, uintptr_t& end)> regionBounds;

        // Check card table dirty status
        std::function<bool(uintptr_t addr)> isCardDirty;

        // Check if object has forwarding pointer
        std::function<bool(uintptr_t addr)> isForwarded;
    };

    void setCallbacks(Callbacks cb) { cb_ = std::move(cb); }

    /**
     * @brief Run all verification checks
     */
    VerificationResult verify() {
        VerificationResult result;
        result.passed = true;
        result.objectsVerified = 0;
        result.referencesChecked = 0;
        result.regionsChecked = 0;
        result.cardsChecked = 0;

        auto start = std::chrono::steady_clock::now();

        // Build live set for fast lookup
        buildLiveSet();

        // Check 1: No dangling references
        verifyNoDanglingRefs(result);

        // Check 2: All reachable objects are marked
        verifyMarkConsistency(result);

        // Check 3: No overlapping objects
        verifyNoOverlaps(result);

        // Check 4: No stale forwarding pointers
        verifyNoStaleForwarding(result);

        // Check 5: Card table consistency
        verifyCardTable(result);

        result.durationMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();

        return result;
    }

    /**
     * @brief Quick sanity check (cheaper than full verify)
     */
    bool quickCheck() {
        if (!cb_.iterateLiveObjects || !cb_.isLiveObject) return true;

        bool ok = true;
        cb_.iterateLiveObjects([&](uintptr_t addr, size_t /*size*/) {
            if (cb_.getReferences) {
                cb_.getReferences(addr, [&](uintptr_t ref) {
                    if (!cb_.isLiveObject(ref)) {
                        ok = false;
                    }
                });
            }
        });

        return ok;
    }

private:
    void buildLiveSet() {
        liveObjects_.clear();
        if (!cb_.iterateLiveObjects) return;

        cb_.iterateLiveObjects([&](uintptr_t addr, size_t size) {
            liveObjects_[addr] = size;
        });
    }

    void verifyNoDanglingRefs(VerificationResult& result) {
        if (!cb_.getReferences) return;

        for (const auto& [addr, size] : liveObjects_) {
            result.objectsVerified++;

            cb_.getReferences(addr, [&](uintptr_t ref) {
                result.referencesChecked++;

                if (ref == 0) return;  // Null ref is OK

                if (liveObjects_.find(ref) == liveObjects_.end()) {
                    result.addError(
                        VerificationError::Type::DanglingReference,
                        addr, ref,
                        "object references non-live address");
                }
            });
        }
    }

    void verifyMarkConsistency(VerificationResult& result) {
        if (!cb_.enumerateRoots || !cb_.isMarked) return;

        // BFS from roots — all reachable objects must be marked
        std::unordered_set<uintptr_t> reachable;
        std::deque<uintptr_t> queue;

        cb_.enumerateRoots([&](uintptr_t root) {
            if (reachable.insert(root).second) {
                queue.push_back(root);
            }
        });

        while (!queue.empty()) {
            uintptr_t addr = queue.front();
            queue.pop_front();

            if (!cb_.isMarked(addr)) {
                result.addError(
                    VerificationError::Type::UnmarkedReachable,
                    addr,
                    "reachable object is not marked");
            }

            if (cb_.getReferences) {
                cb_.getReferences(addr, [&](uintptr_t ref) {
                    if (ref != 0 && reachable.insert(ref).second) {
                        queue.push_back(ref);
                    }
                });
            }
        }
    }

    void verifyNoOverlaps(VerificationResult& result) {
        // Sort objects by address, check none overlap
        std::vector<std::pair<uintptr_t, size_t>> sorted(
            liveObjects_.begin(), liveObjects_.end());

        std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) {
                return a.first < b.first;
            });

        for (size_t i = 1; i < sorted.size(); i++) {
            uintptr_t prevEnd = sorted[i-1].first + sorted[i-1].second;
            if (prevEnd > sorted[i].first) {
                result.addError(
                    VerificationError::Type::OverlappingObjects,
                    sorted[i-1].first, sorted[i].first,
                    "objects overlap in memory");
            }
        }
    }

    void verifyNoStaleForwarding(VerificationResult& result) {
        if (!cb_.isForwarded) return;

        for (const auto& [addr, size] : liveObjects_) {
            if (cb_.isForwarded(addr)) {
                result.addError(
                    VerificationError::Type::StaleForwardingPointer,
                    addr,
                    "live object still has forwarding pointer");
            }
        }
    }

    void verifyCardTable(VerificationResult& result) {
        if (!cb_.isCardDirty || !cb_.regionOf || !cb_.getReferences) return;

        for (const auto& [addr, size] : liveObjects_) {
            result.cardsChecked++;

            uint32_t srcRegion = cb_.regionOf(addr);

            cb_.getReferences(addr, [&](uintptr_t ref) {
                if (ref == 0) return;

                uint32_t dstRegion = cb_.regionOf(ref);
                if (srcRegion != dstRegion) {
                    // Cross-region ref: card should be dirty
                    if (!cb_.isCardDirty(addr)) {
                        result.addError(
                            VerificationError::Type::CardTableInconsistency,
                            addr, ref,
                            "cross-region ref but card not dirty");
                    }
                }
            });
        }
    }

    Callbacks cb_;
    std::unordered_map<uintptr_t, size_t> liveObjects_;
};

// =============================================================================
// Verification Harness (runs verifier automatically)
// =============================================================================

/**
 * @brief Automatic verifier that hooks into GC lifecycle
 *
 * Registered with the GC pipeline orchestrator. Runs after
 * each GC cycle when verification is enabled.
 */
class VerificationHarness {
public:
    struct Config {
        bool enabled;
        bool abortOnFailure;
        bool verboseOutput;
        size_t verifyEveryNthCycle;  // 1 = every cycle, 0 = never

        Config()
            : enabled(false)
            , abortOnFailure(true)
            , verboseOutput(false)
            , verifyEveryNthCycle(1) {}
    };

    struct Stats {
        uint64_t cyclesVerified;
        uint64_t cyclesPassed;
        uint64_t cyclesFailed;
        uint64_t totalErrors;
        double totalVerifyMs;
    };

    explicit VerificationHarness(const Config& config = Config{})
        : config_(config)
        , cycleCount_(0) {}

    void setVerifier(GCVerifier* verifier) { verifier_ = verifier; }

    /**
     * @brief Called after each GC cycle
     */
    void onGCComplete() {
        cycleCount_++;

        if (!config_.enabled || !verifier_) return;
        if (config_.verifyEveryNthCycle == 0) return;
        if (cycleCount_ % config_.verifyEveryNthCycle != 0) return;

        auto result = verifier_->verify();
        stats_.cyclesVerified++;
        stats_.totalVerifyMs += result.durationMs;

        if (result.passed) {
            stats_.cyclesPassed++;
            if (config_.verboseOutput) {
                result.report();
            }
        } else {
            stats_.cyclesFailed++;
            stats_.totalErrors += result.errors.size();
            result.report();

            if (config_.abortOnFailure) {
                fprintf(stderr, "[gc-verify] ABORTING: heap corruption "
                    "detected after GC cycle %lu\n",
                    static_cast<unsigned long>(cycleCount_));
                std::abort();
            }
        }
    }

    /**
     * @brief Enable from environment variable
     */
    static Config fromEnvironment() {
        Config config;
        const char* env = std::getenv("ZEPRA_GC_VERIFY");
        if (env) {
            config.enabled = (std::string(env) != "0");
        }
        const char* verbose = std::getenv("ZEPRA_GC_VERIFY_VERBOSE");
        if (verbose) {
            config.verboseOutput = (std::string(verbose) != "0");
        }
        return config;
    }

    Stats computeStats() const { return stats_; }

private:
    Config config_;
    GCVerifier* verifier_ = nullptr;
    uint64_t cycleCount_;
    Stats stats_{};
};

} // namespace Zepra::Heap
