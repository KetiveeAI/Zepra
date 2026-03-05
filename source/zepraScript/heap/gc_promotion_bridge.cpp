// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_promotion_bridge.cpp — Nursery→OldGen promotion logic

#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Heap {

// During minor GC (scavenge), surviving nursery objects are either
// copied to the other semi-space (if young) or promoted to old gen
// (if above tenuring threshold). This bridge handles the promotion path.

struct PromotionRequest {
    uintptr_t srcAddr;
    size_t objectSize;
    uint8_t age;
    uint8_t typeTag;
    uint32_t shapeId;
};

struct PromotionResult {
    uintptr_t destAddr;
    bool promoted;      // true = old gen, false = survivor space
};

class PromotionBridge {
public:
    struct Backend {
        // Allocate in old gen for promoted objects.
        std::function<uintptr_t(size_t)> oldGenAllocate;
        // Allocate in survivor semi-space for young objects.
        std::function<uintptr_t(size_t)> survivorAllocate;
        // Copy object bytes.
        std::function<void(uintptr_t dst, uintptr_t src, size_t size)> copyObject;
        // Install forwarding pointer at source.
        std::function<void(uintptr_t src, uintptr_t dst)> installForwarding;
        // Record in remembered set (old→young ref after promotion).
        std::function<void(uintptr_t addr)> addToRememberedSet;
        // Dirty card for old gen object.
        std::function<void(uintptr_t addr)> dirtyCard;
        // Pretenure feedback.
        std::function<void(uint32_t siteId)> recordPromotion;
    };

    void setBackend(Backend b) { backend_ = std::move(b); }
    void setTenuringThreshold(uint8_t t) { threshold_ = t; }

    PromotionResult promote(const PromotionRequest& req) {
        PromotionResult result;

        if (req.age >= threshold_) {
            // Promote to old gen.
            result.destAddr = backend_.oldGenAllocate ?
                backend_.oldGenAllocate(req.objectSize) : 0;
            result.promoted = true;

            if (result.destAddr != 0) {
                // Copy object.
                if (backend_.copyObject)
                    backend_.copyObject(result.destAddr, req.srcAddr, req.objectSize);

                // Install forwarding pointer.
                if (backend_.installForwarding)
                    backend_.installForwarding(req.srcAddr, result.destAddr);

                // Dirty card so old-gen refs get scanned.
                if (backend_.dirtyCard)
                    backend_.dirtyCard(result.destAddr);

                // Pretenure feedback.
                if (backend_.recordPromotion)
                    backend_.recordPromotion(req.shapeId);

                stats_.promoted++;
                stats_.promotedBytes += req.objectSize;
            }
        } else {
            // Copy to survivor space.
            result.destAddr = backend_.survivorAllocate ?
                backend_.survivorAllocate(req.objectSize) : 0;
            result.promoted = false;

            if (result.destAddr != 0) {
                if (backend_.copyObject)
                    backend_.copyObject(result.destAddr, req.srcAddr, req.objectSize);

                if (backend_.installForwarding)
                    backend_.installForwarding(req.srcAddr, result.destAddr);

                stats_.survived++;
                stats_.survivedBytes += req.objectSize;
            }
        }

        return result;
    }

    // Batch-promote a list of nursery objects (used by parallel scavenger).
    void promoteBatch(const std::vector<PromotionRequest>& batch) {
        for (auto& req : batch) {
            promote(req);
        }
    }

    struct Stats {
        uint64_t promoted;
        uint64_t promotedBytes;
        uint64_t survived;
        uint64_t survivedBytes;
    };

    const Stats& stats() const { return stats_; }

    void resetStats() { stats_ = {}; }

private:
    Backend backend_;
    uint8_t threshold_ = 6;
    Stats stats_{};
};

} // namespace Zepra::Heap
