/**
 * @file LinearScan.h
 * @brief Linear Scan Register Allocator
 * 
 * Assigns physical registers to virtual registers using
 * the Linear Scan algorithm (Poletto & Sarkar).
 * Fast allocation suitable for JIT compilers.
 */

#pragma once

#include "LivenessAnalysis.h"
#include <vector>
#include <algorithm>
#include <set>

namespace Zepra::JIT::RegAlloc {

class LinearScan {
public:
    LinearScan(const std::vector<LiveInterval>& intervals, int numPhysRegs)
        : unhandled_(intervals), numPhysRegs_(numPhysRegs) {}
        
    void allocate() {
        // Sort intervals by start position
        std::sort(unhandled_.begin(), unhandled_.end(),
            [](const LiveInterval& a, const LiveInterval& b) {
                return a.start < b.start;
            });
            
        active_.clear();
        
        for (auto& interval : unhandled_) {
            expireOldIntervals(interval.start);
            
            if (active_.size() == static_cast<size_t>(numPhysRegs_)) {
                spillAtInterval(interval);
            } else {
                assignRegister(interval);
                active_.push_back(interval);
            }
        }
    }
    
private:
    void expireOldIntervals(uint32_t position) {
        // Remove intervals that ended before current position
        auto it = std::remove_if(active_.begin(), active_.end(),
            [position](const LiveInterval& i) { return i.end <= position; });
            
        // Free registers associated with expired intervals
        // (Implementation detail skipped for brevity)
        
        active_.erase(it, active_.end());
    }
    
    void spillAtInterval(LiveInterval& interval) {
        // Spill header logic: choose interval that ends last
        // If current interval ends last, spill it
        // Else spill the active one ending last and replace with current
        
        // Placeholder: Always spill current
        interval.stackSlot = nextStackSlot_++;
    }
    
    void assignRegister(LiveInterval& interval) {
        // Find free register
        // Placeholder: Just assign next available index
        // Real impl tracks register usage bitmask
        interval.physReg = 1; // Dummy
    }
    
    std::vector<LiveInterval> unhandled_;
    std::vector<LiveInterval> active_;
    int numPhysRegs_;
    int nextStackSlot_ = 0;
};

} // namespace Zepra::JIT::RegAlloc
