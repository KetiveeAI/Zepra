// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_age_table.cpp — Object age tracking for generational promotion

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <algorithm>

namespace Zepra::Heap {

// Each nursery object has an age: how many scavenges it has survived.
// When age reaches the tenuring threshold, the object is promoted
// to old space. The table adapts the threshold based on survivor
// space pressure.

class AgeTable {
public:
    static constexpr uint8_t MAX_AGE = 15;
    static constexpr uint8_t DEFAULT_THRESHOLD = 6;

    AgeTable() : threshold_(DEFAULT_THRESHOLD) {
        std::memset(counts_, 0, sizeof(counts_));
    }

    void recordAge(uint8_t age) {
        if (age <= MAX_AGE) counts_[age]++;
    }

    void clear() { std::memset(counts_, 0, sizeof(counts_)); }

    // After scavenge: compute optimal threshold so survivor space
    // doesn't exceed targetOccupancy fraction of survivor capacity.
    uint8_t computeThreshold(size_t survivorCapacity,
                               double targetOccupancy,
                               size_t avgObjectSize) {
        size_t targetBytes = static_cast<size_t>(
            survivorCapacity * targetOccupancy);
        size_t cumulative = 0;

        for (uint8_t age = 0; age <= MAX_AGE; age++) {
            cumulative += counts_[age] * avgObjectSize;
            if (cumulative > targetBytes) {
                threshold_ = std::max<uint8_t>(1, age);
                return threshold_;
            }
        }

        threshold_ = MAX_AGE;
        return threshold_;
    }

    bool shouldPromote(uint8_t age) const {
        return age >= threshold_;
    }

    uint8_t threshold() const { return threshold_; }
    void setThreshold(uint8_t t) { threshold_ = std::min(t, MAX_AGE); }

    uint64_t countAtAge(uint8_t age) const {
        return age <= MAX_AGE ? counts_[age] : 0;
    }

    uint64_t totalSurvivors() const {
        uint64_t total = 0;
        for (uint8_t i = 0; i <= MAX_AGE; i++) total += counts_[i];
        return total;
    }

    // Proportion of objects below vs at/above threshold.
    double promotionRate() const {
        uint64_t total = totalSurvivors();
        if (total == 0) return 0;
        uint64_t promoted = 0;
        for (uint8_t i = threshold_; i <= MAX_AGE; i++) promoted += counts_[i];
        return static_cast<double>(promoted) / total;
    }

private:
    uint64_t counts_[MAX_AGE + 1];
    uint8_t threshold_;
};

} // namespace Zepra::Heap
