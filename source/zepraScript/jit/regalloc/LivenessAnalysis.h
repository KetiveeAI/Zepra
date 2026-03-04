/**
 * @file LivenessAnalysis.h
 * @brief Liveness Analysis for Register Allocation
 * 
 * Computes live ranges for virtual registers to determine
 * when they are active and need physical registers.
 */

#pragma once

#include "jit/dfg/DFGGraph.h"
#include <vector>
#include <map>
#include <algorithm>

namespace Zepra::JIT::RegAlloc {

struct LiveInterval {
    uint32_t vreg;
    uint32_t start; // Start instruction index
    uint32_t end;   // End instruction index
    
    // Physical register assigned (if any)
    int32_t physReg = -1; 
    
    // Stack slot assigned (if spilled)
    int32_t stackSlot = -1;
    
    bool intersects(const LiveInterval& other) const {
        return start < other.end && other.start < end;
    }
};

class LivenessAnalysis {
public:
    explicit LivenessAnalysis(const DFG::Graph& graph) : graph_(graph) {}
    
    void compute() {
        // 1. Linearize blocks (compute instruction indices)
        linearize();
        
        // 2. Compute live ranges
        // Simple approach: Iterate backwards from end
        // Real implementation builds live-in/live-out sets per block
        // For baseline, we assume single block or simple flow
        
        // Placeholder: Dummy interval for each node
        for (const auto& node : graph_.nodes()) {
            if (node->vreg != 0) {
                // Assume live from definition to graph end (pessimistic)
                getOrCreateInterval(node->vreg).start = node->id; // Use ID as proxy for index
                getOrCreateInterval(node->vreg).end = node->id + 100; // Fake end
            }
        }
    }
    
    const std::vector<LiveInterval>& intervals() const { return intervals_; }
    
private:
    LiveInterval& getOrCreateInterval(uint32_t vreg) {
        for (auto& interval : intervals_) {
            if (interval.vreg == vreg) return interval;
        }
        intervals_.push_back({vreg, 0, 0});
        return intervals_.back();
    }
    
    void linearize() {
        // Build linear order of instructions
    }
    
    const DFG::Graph& graph_;
    std::vector<LiveInterval> intervals_;
};

} // namespace Zepra::JIT::RegAlloc
