/**
 * @file DFGRegAlloc.h
 * @brief DFG Linear Scan Register Allocator
 * 
 * Implements the Poletto-Sarkar linear scan algorithm for
 * register allocation. Fast compilation time with reasonable
 * code quality.
 * 
 * Reference: http://dl.acm.org/citation.cfm?id=330250
 */

#pragma once

#include "jit/dfg/DFGGraph.h"
#include <algorithm>
#include <vector>
#include <cstdint>

namespace Zepra::DFG {

// Live interval for a value
struct LiveInterval {
    Value* value = nullptr;
    uint32_t start = 0;      // First use
    uint32_t end = 0;        // Last use
    int8_t assignedReg = -1; // -1 = spilled
    int32_t spillSlot = -1;  // Stack slot if spilled
    
    bool isSpilled() const { return assignedReg < 0; }
    bool overlaps(const LiveInterval& other) const {
        return start < other.end && end > other.start;
    }
};

// Register allocation result
struct RegisterAllocation {
    std::vector<LiveInterval> intervals;
    uint32_t numSpillSlots = 0;
    
    int8_t getRegister(Value* v) const {
        for (const auto& interval : intervals) {
            if (interval.value == v) return interval.assignedReg;
        }
        return -1;
    }
    
    int32_t getSpillSlot(Value* v) const {
        for (const auto& interval : intervals) {
            if (interval.value == v) return interval.spillSlot;
        }
        return -1;
    }
};

class LinearScanAllocator {
public:
    explicit LinearScanAllocator(Graph* graph) : graph_(graph) {}
    
    RegisterAllocation allocate();
    
private:
    void computeLiveness();
    void buildIntervals();
    void linearScan();
    void expireOldIntervals(uint32_t position);
    void spillAtInterval(LiveInterval& current);
    int8_t allocateRegister(Type type);
    void freeRegister(int8_t reg, Type type);
    
    Graph* graph_;
    RegisterAllocation result_;
    
    // Active intervals sorted by end point
    std::vector<LiveInterval*> active_;
    
    // Available registers
    static constexpr int NUM_GPR = 14;  // Exclude RSP, RBP
    static constexpr int NUM_FPR = 16;  // XMM0-15
    
    uint32_t gprMask_ = (1 << NUM_GPR) - 1;  // Bit per GPR
    uint32_t fprMask_ = (1 << NUM_FPR) - 1;  // Bit per FPR
    
    uint32_t nextSpillSlot_ = 0;
    
    // Value numbering for position
    std::unordered_map<Value*, uint32_t> valuePosition_;
};

// =============================================================================
// Implementation
// =============================================================================

inline RegisterAllocation LinearScanAllocator::allocate() {
    computeLiveness();
    buildIntervals();
    linearScan();
    return result_;
}

inline void LinearScanAllocator::computeLiveness() {
    // Assign positions to values in RPO order
    uint32_t position = 0;
    
    for (BasicBlock* block : graph_->reversePostOrder()) {
        for (Value* phi : block->phis()) {
            valuePosition_[phi] = position++;
        }
        for (Value* v : block->values()) {
            valuePosition_[v] = position++;
        }
    }
}

inline void LinearScanAllocator::buildIntervals() {
    result_.intervals.clear();
    
    for (BasicBlock* block : graph_->blocks()) {
        for (Value* phi : block->phis()) {
            if (phi->isDead() || !hasResult(phi->opcode())) continue;
            
            LiveInterval interval;
            interval.value = phi;
            interval.start = valuePosition_[phi];
            interval.end = interval.start;
            
            // Extend interval to all uses
            for (Value* user : phi->users()) {
                if (valuePosition_.count(user)) {
                    interval.end = std::max(interval.end, valuePosition_[user]);
                }
            }
            
            result_.intervals.push_back(interval);
        }
        
        for (Value* v : block->values()) {
            if (v->isDead() || !hasResult(v->opcode())) continue;
            
            LiveInterval interval;
            interval.value = v;
            interval.start = valuePosition_[v];
            interval.end = interval.start;
            
            // Extend interval to all uses
            for (Value* user : v->users()) {
                if (valuePosition_.count(user)) {
                    interval.end = std::max(interval.end, valuePosition_[user]);
                }
            }
            
            // Ensure interval has non-zero length for values with uses
            if (interval.end == interval.start && !v->users().empty()) {
                interval.end = interval.start + 1;
            }
            
            result_.intervals.push_back(interval);
        }
    }
    
    // Sort intervals by start position
    std::sort(result_.intervals.begin(), result_.intervals.end(),
        [](const LiveInterval& a, const LiveInterval& b) {
            return a.start < b.start;
        });
}

inline void LinearScanAllocator::linearScan() {
    active_.clear();
    gprMask_ = (1 << NUM_GPR) - 1;
    fprMask_ = (1 << NUM_FPR) - 1;
    nextSpillSlot_ = 0;
    
    for (auto& interval : result_.intervals) {
        expireOldIntervals(interval.start);
        
        int8_t reg = allocateRegister(interval.value->type());
        if (reg >= 0) {
            interval.assignedReg = reg;
            
            // Insert into active list sorted by end point
            auto it = std::lower_bound(active_.begin(), active_.end(), &interval,
                [](const LiveInterval* a, const LiveInterval* b) {
                    return a->end < b->end;
                });
            active_.insert(it, &interval);
        } else {
            // Need to spill
            spillAtInterval(interval);
        }
    }
    
    result_.numSpillSlots = nextSpillSlot_;
}

inline void LinearScanAllocator::expireOldIntervals(uint32_t position) {
    auto it = active_.begin();
    while (it != active_.end()) {
        if ((*it)->end <= position) {
            freeRegister((*it)->assignedReg, (*it)->value->type());
            it = active_.erase(it);
        } else {
            break;  // Active is sorted by end, so stop early
        }
    }
}

inline void LinearScanAllocator::spillAtInterval(LiveInterval& current) {
    if (active_.empty()) {
        // No active intervals, must spill current
        current.spillSlot = nextSpillSlot_++;
        return;
    }
    
    // Spill the interval that ends last
    LiveInterval* spill = active_.back();
    
    if (spill->end > current.end) {
        // Spill the active interval that ends last
        current.assignedReg = spill->assignedReg;
        spill->assignedReg = -1;
        spill->spillSlot = nextSpillSlot_++;
        
        active_.pop_back();
        
        // Insert current into active
        auto it = std::lower_bound(active_.begin(), active_.end(), &current,
            [](const LiveInterval* a, const LiveInterval* b) {
                return a->end < b->end;
            });
        active_.insert(it, &current);
    } else {
        // Spill current
        current.spillSlot = nextSpillSlot_++;
    }
}

inline int8_t LinearScanAllocator::allocateRegister(Type type) {
    uint32_t& mask = (type == Type::F32 || type == Type::F64) ? fprMask_ : gprMask_;
    
    if (mask == 0) return -1;  // No registers available
    
    // Find first available register
    int8_t reg = 0;
    while ((mask & (1 << reg)) == 0) reg++;
    
    mask &= ~(1 << reg);  // Mark as used
    return reg;
}

inline void LinearScanAllocator::freeRegister(int8_t reg, Type type) {
    if (reg < 0) return;
    
    uint32_t& mask = (type == Type::F32 || type == Type::F64) ? fprMask_ : gprMask_;
    mask |= (1 << reg);
}

} // namespace Zepra::DFG
