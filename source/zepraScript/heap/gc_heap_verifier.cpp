// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_heap_verifier.cpp — Debug-mode heap invariant checks

#include <mutex>
#include <functional>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Heap {

// Runs in debug builds after each GC cycle to verify heap integrity.
// Catches bugs early: dangling pointers, missed write barriers,
// corrupted headers, orphaned objects.

struct VerificationError {
    uintptr_t addr;
    const char* description;
    uint64_t errorId;
};

class HeapVerifier {
public:
    struct Backend {
        std::function<bool(uintptr_t)> isValidHeapAddr;
        std::function<bool(uintptr_t)> isMarked;
        std::function<size_t(uintptr_t)> objectSize;
        std::function<void(uintptr_t, std::function<void(uintptr_t)>)> iterateSlots;
        std::function<bool(uintptr_t)> isInNursery;
        std::function<bool(uintptr_t)> isCardDirty;
    };

    void setBackend(Backend b) { backend_ = std::move(b); }

    // Verify all live objects have valid references.
    size_t verifyReferences(const std::vector<uintptr_t>& liveObjects) {
        size_t errors = 0;
        for (auto obj : liveObjects) {
            if (!backend_.iterateSlots) continue;
            backend_.iterateSlots(obj, [&](uintptr_t child) {
                if (child == 0) return;
                if (backend_.isValidHeapAddr && !backend_.isValidHeapAddr(child)) {
                    recordError(obj, "dangling pointer to non-heap address");
                    errors++;
                }
            });
        }
        return errors;
    }

    // Verify old→young references have dirty cards.
    size_t verifyRememberedSet(const std::vector<uintptr_t>& oldGenObjects) {
        size_t errors = 0;
        for (auto obj : oldGenObjects) {
            if (!backend_.iterateSlots) continue;
            backend_.iterateSlots(obj, [&](uintptr_t child) {
                if (child == 0) return;
                if (backend_.isInNursery && backend_.isInNursery(child)) {
                    if (backend_.isCardDirty && !backend_.isCardDirty(obj)) {
                        recordError(obj, "old→young ref without dirty card");
                        errors++;
                    }
                }
            });
        }
        return errors;
    }

    // Verify no unmarked objects are referenced by live objects.
    size_t verifyMarking(const std::vector<uintptr_t>& liveObjects) {
        size_t errors = 0;
        for (auto obj : liveObjects) {
            if (!backend_.isMarked(obj)) {
                recordError(obj, "live object not marked");
                errors++;
            }
        }
        return errors;
    }

    // Verify object sizes are consistent.
    size_t verifySizes(const std::vector<uintptr_t>& objects) {
        size_t errors = 0;
        for (auto obj : objects) {
            if (!backend_.objectSize) continue;
            size_t size = backend_.objectSize(obj);
            if (size == 0 || size > 64 * 1024 * 1024) {
                recordError(obj, "suspicious object size");
                errors++;
            }
        }
        return errors;
    }

    // Run all checks.
    size_t verifyAll(const std::vector<uintptr_t>& allObjects,
                      const std::vector<uintptr_t>& oldGenObjects) {
        size_t total = 0;
        total += verifyReferences(allObjects);
        total += verifyRememberedSet(oldGenObjects);
        total += verifyMarking(allObjects);
        total += verifySizes(allObjects);

        if (total > 0) {
            fprintf(stderr, "[gc-verify] %zu errors detected\n", total);
        }
        return total;
    }

    const std::vector<VerificationError>& errors() const { return errors_; }
    void clearErrors() { errors_.clear(); }

private:
    void recordError(uintptr_t addr, const char* desc) {
        errors_.push_back({addr, desc, errorCount_++});
    }

    Backend backend_;
    std::vector<VerificationError> errors_;
    uint64_t errorCount_ = 0;
};

} // namespace Zepra::Heap
