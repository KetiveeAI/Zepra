// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_nursery_sizing.cpp — Dynamic nursery size adaptation

#include <cstdint>
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <cmath>

namespace Zepra::Heap {

// Adapts nursery size based on allocation rate and scavenge overhead.
// Fast-allocating workloads get a bigger nursery (fewer scavenges).
// Low-allocation workloads get a smaller nursery (lower memory footprint).

class NurserySizingPolicy {
public:
    struct Config {
        size_t minSize;
        size_t maxSize;
        size_t pageSize;
        double targetScavengeMs;   // Target scavenge pause (ms)
        double growFactor;
        double shrinkFactor;

        Config()
            : minSize(512 * 1024)
            , maxSize(16 * 1024 * 1024)
            , pageSize(256 * 1024)
            , targetScavengeMs(2.0)
            , growFactor(2.0)
            , shrinkFactor(0.5) {}
    };

    explicit NurserySizingPolicy(const Config& config = Config{})
        : config_(config), currentSize_(config.minSize) {}

    struct ScavengeResult {
        double pauseMs;
        size_t liveBytes;
        size_t nurseryCapacity;
        double allocationRateMBps;
    };

    size_t recommend(const ScavengeResult& result) {
        size_t newSize = result.nurseryCapacity;

        // If pause exceeds target, shrink nursery (less to scan).
        if (result.pauseMs > config_.targetScavengeMs * 1.5) {
            newSize = static_cast<size_t>(
                result.nurseryCapacity * config_.shrinkFactor);
        }
        // If pause is well under target and high allocation rate, grow.
        else if (result.pauseMs < config_.targetScavengeMs * 0.5 &&
                 result.allocationRateMBps > 50.0) {
            newSize = static_cast<size_t>(
                result.nurseryCapacity * config_.growFactor);
        }
        // If survival rate is very low (<10%), nursery is right-sized.
        else if (result.liveBytes < result.nurseryCapacity / 10) {
            // Keep current size.
        }
        // If survival rate is high (>50%), consider shrinking.
        else if (result.liveBytes > result.nurseryCapacity / 2) {
            newSize = static_cast<size_t>(
                result.nurseryCapacity * 0.75);
        }

        newSize = std::max(newSize, config_.minSize);
        newSize = std::min(newSize, config_.maxSize);
        newSize = alignUp(newSize, config_.pageSize);

        currentSize_ = newSize;
        return newSize;
    }

    size_t currentSize() const { return currentSize_; }

private:
    static size_t alignUp(size_t v, size_t a) { return (v + a - 1) & ~(a - 1); }

    Config config_;
    size_t currentSize_;
};

} // namespace Zepra::Heap
