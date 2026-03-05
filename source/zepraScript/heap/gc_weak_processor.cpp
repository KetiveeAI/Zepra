/**
 * @file gc_weak_processor.cpp
 * @brief Weak reference processing for GC
 *
 * After marking, weak references to unmarked objects must be cleared.
 * This handles:
 *   - WeakRef (ES2021)
 *   - WeakMap / WeakSet (ephemeron semantics)
 *   - FinalizationRegistry callbacks
 *   - Internal weak caches (string interning, shape table)
 *
 * Processing order:
 *   1. Clear WeakRefs pointing to dead objects
 *   2. Process WeakMaps (ephemeron algorithm — keys are weak)
 *   3. Queue FinalizationRegistry callbacks
 *   4. Clear internal weak caches
 */

#include <atomic>
#include <mutex>
#include <vector>
#include <deque>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <chrono>

namespace Zepra::Heap {

// =============================================================================
// Weak Reference Entry
// =============================================================================

struct WeakRefEntry {
    uintptr_t weakRefAddr;   // The WeakRef object itself
    uintptr_t targetAddr;    // The object it weakly points to
    bool cleared;

    WeakRefEntry()
        : weakRefAddr(0), targetAddr(0), cleared(false) {}

    WeakRefEntry(uintptr_t ref, uintptr_t target)
        : weakRefAddr(ref), targetAddr(target), cleared(false) {}
};

// =============================================================================
// FinalizationRegistry Entry
// =============================================================================

struct FinalizationEntry {
    uintptr_t registryAddr;   // The FinalizationRegistry
    uintptr_t targetAddr;     // The target object
    uintptr_t heldValue;      // Value passed to cleanup callback
    uintptr_t unregisterToken;

    FinalizationEntry()
        : registryAddr(0), targetAddr(0)
        , heldValue(0), unregisterToken(0) {}
};

// =============================================================================
// WeakMap Entry (Ephemeron)
// =============================================================================

struct EphemeronEntry {
    uintptr_t mapAddr;     // The WeakMap
    uintptr_t keyAddr;     // Weak key
    uintptr_t valueAddr;   // Strong value (kept alive only if key is alive)

    EphemeronEntry()
        : mapAddr(0), keyAddr(0), valueAddr(0) {}

    EphemeronEntry(uintptr_t m, uintptr_t k, uintptr_t v)
        : mapAddr(m), keyAddr(k), valueAddr(v) {}
};

// =============================================================================
// Weak Cache Entry (for internal caches)
// =============================================================================

struct WeakCacheEntry {
    const char* cacheName;
    uintptr_t keyAddr;
    std::function<void()> clearCallback;

    WeakCacheEntry()
        : cacheName(""), keyAddr(0) {}
};

// =============================================================================
// GC Weak Processor
// =============================================================================

class GCWeakProcessor {
public:
    struct Callbacks {
        std::function<bool(uintptr_t addr)> isMarked;
        std::function<void(uintptr_t weakRefAddr)> clearWeakRef;
        std::function<bool(uintptr_t addr)> tryMark;
    };

    struct Stats {
        uint64_t weakRefsProcessed;
        uint64_t weakRefsCleared;
        uint64_t ephemeronsCycled;
        uint64_t ephemeronsCleared;
        uint64_t finalizationsQueued;
        uint64_t weakCachesCleared;
        double processingMs;
    };

    void setCallbacks(Callbacks cb) { cb_ = std::move(cb); }

    // -------------------------------------------------------------------------
    // Registration
    // -------------------------------------------------------------------------

    void registerWeakRef(uintptr_t weakRefAddr, uintptr_t target) {
        std::lock_guard<std::mutex> lock(mutex_);
        weakRefs_.push_back({weakRefAddr, target});
    }

    void registerFinalization(const FinalizationEntry& entry) {
        std::lock_guard<std::mutex> lock(mutex_);
        finalizations_.push_back(entry);
    }

    void registerEphemeron(uintptr_t mapAddr, uintptr_t key,
                            uintptr_t value) {
        std::lock_guard<std::mutex> lock(mutex_);
        ephemerons_.push_back({mapAddr, key, value});
    }

    void registerWeakCache(const char* name, uintptr_t key,
                            std::function<void()> clearCb) {
        std::lock_guard<std::mutex> lock(mutex_);
        WeakCacheEntry entry;
        entry.cacheName = name;
        entry.keyAddr = key;
        entry.clearCallback = std::move(clearCb);
        weakCaches_.push_back(std::move(entry));
    }

    // -------------------------------------------------------------------------
    // Processing (called after marking, during STW pause)
    // -------------------------------------------------------------------------

    Stats processAll() {
        stats_ = {};
        auto start = std::chrono::steady_clock::now();

        processWeakRefs();
        processEphemerons();
        processFinalizationRegistries();
        processWeakCaches();

        stats_.processingMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();

        return stats_;
    }

    /**
     * @brief Get pending finalization callbacks
     */
    std::vector<FinalizationEntry> drainFinalizationQueue() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto result = std::move(pendingFinalizations_);
        pendingFinalizations_.clear();
        return result;
    }

    void unregisterTarget(uintptr_t targetAddr,
                           uintptr_t unregisterToken) {
        std::lock_guard<std::mutex> lock(mutex_);
        finalizations_.erase(
            std::remove_if(finalizations_.begin(), finalizations_.end(),
                [&](const FinalizationEntry& e) {
                    return e.targetAddr == targetAddr &&
                           e.unregisterToken == unregisterToken;
                }),
            finalizations_.end());
    }

    const Stats& lastStats() const { return stats_; }

private:
    void processWeakRefs() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!cb_.isMarked) return;

        auto it = weakRefs_.begin();
        while (it != weakRefs_.end()) {
            stats_.weakRefsProcessed++;

            if (!cb_.isMarked(it->targetAddr)) {
                // Target is dead → clear the WeakRef
                if (cb_.clearWeakRef) {
                    cb_.clearWeakRef(it->weakRefAddr);
                }
                it->cleared = true;
                stats_.weakRefsCleared++;
                it = weakRefs_.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * @brief Ephemeron processing (WeakMap semantics)
     *
     * A WeakMap value is only kept alive if its key is alive.
     * We iterate until no new objects are marked:
     *   loop:
     *     for each ephemeron where key is marked:
     *       mark the value (and any transitive refs)
     *     if no new marks → done
     */
    void processEphemerons() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!cb_.isMarked || !cb_.tryMark) return;

        bool progress = true;
        while (progress) {
            progress = false;

            auto it = ephemerons_.begin();
            while (it != ephemerons_.end()) {
                stats_.ephemeronsCycled++;

                if (cb_.isMarked(it->keyAddr)) {
                    // Key is alive → mark value
                    if (it->valueAddr != 0) {
                        if (cb_.tryMark(it->valueAddr)) {
                            progress = true;
                        }
                    }
                    ++it;
                } else {
                    ++it;
                }
            }
        }

        // Remove entries with dead keys
        ephemerons_.erase(
            std::remove_if(ephemerons_.begin(), ephemerons_.end(),
                [&](const EphemeronEntry& e) {
                    if (!cb_.isMarked(e.keyAddr)) {
                        stats_.ephemeronsCleared++;
                        return true;
                    }
                    return false;
                }),
            ephemerons_.end());
    }

    void processFinalizationRegistries() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!cb_.isMarked) return;

        auto it = finalizations_.begin();
        while (it != finalizations_.end()) {
            if (!cb_.isMarked(it->targetAddr)) {
                // Target collected → queue callback
                pendingFinalizations_.push_back(*it);
                stats_.finalizationsQueued++;
                it = finalizations_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void processWeakCaches() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!cb_.isMarked) return;

        auto it = weakCaches_.begin();
        while (it != weakCaches_.end()) {
            if (!cb_.isMarked(it->keyAddr)) {
                if (it->clearCallback) it->clearCallback();
                stats_.weakCachesCleared++;
                it = weakCaches_.erase(it);
            } else {
                ++it;
            }
        }
    }

    Callbacks cb_;
    Stats stats_{};

    mutable std::mutex mutex_;
    std::vector<WeakRefEntry> weakRefs_;
    std::vector<FinalizationEntry> finalizations_;
    std::vector<EphemeronEntry> ephemerons_;
    std::vector<WeakCacheEntry> weakCaches_;
    std::vector<FinalizationEntry> pendingFinalizations_;
};

} // namespace Zepra::Heap
