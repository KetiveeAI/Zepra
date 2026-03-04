/**
 * @file GraphColoringRegAlloc.h
 * @brief Graph coloring register allocator
 * 
 * Implements:
 * - Interference graph construction
 * - Graph coloring with Chaitin-Briggs
 * - Spilling heuristics
 * - Coalescing
 * 
 * Based on classic Chaitin-Briggs and SSA-based allocators
 */

#pragma once

#include "MacroAssembler.h"
#include "../dfg/DFGGraph.h"
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <bitset>

namespace Zepra::JIT {

// Forward declarations
namespace DFG {
    class Graph;
    class Node;
    class BasicBlock;
}

// =============================================================================
// Virtual Register
// =============================================================================

struct VirtualReg {
    uint32_t id;
    RegClass regClass;
    
    bool operator==(const VirtualReg& other) const { return id == other.id; }
    bool operator<(const VirtualReg& other) const { return id < other.id; }
};

// =============================================================================
// Live Range
// =============================================================================

struct UsePosition {
    uint32_t position;
    bool isDefinition;
    bool isFixedReg;
    Register fixedReg;
};

struct LiveRange {
    VirtualReg vreg;
    uint32_t start;
    uint32_t end;
    std::vector<UsePosition> uses;
    
    bool overlaps(const LiveRange& other) const {
        return !(end <= other.start || start >= other.end);
    }
    
    bool contains(uint32_t pos) const {
        return pos >= start && pos < end;
    }
};

// =============================================================================
// Interference Graph
// =============================================================================

class InterferenceGraph {
public:
    InterferenceGraph() = default;
    
    void addNode(VirtualReg vreg) {
        if (nodeIndex_.find(vreg.id) == nodeIndex_.end()) {
            nodeIndex_[vreg.id] = nodes_.size();
            nodes_.push_back(vreg);
            edges_.emplace_back();
        }
    }
    
    void addEdge(VirtualReg a, VirtualReg b) {
        if (a.id == b.id) return;
        
        size_t ia = nodeIndex_[a.id];
        size_t ib = nodeIndex_[b.id];
        
        edges_[ia].insert(b.id);
        edges_[ib].insert(a.id);
    }
    
    bool hasEdge(VirtualReg a, VirtualReg b) const {
        auto it = nodeIndex_.find(a.id);
        if (it == nodeIndex_.end()) return false;
        return edges_[it->second].count(b.id) > 0;
    }
    
    size_t degree(VirtualReg vreg) const {
        auto it = nodeIndex_.find(vreg.id);
        if (it == nodeIndex_.end()) return 0;
        return edges_[it->second].size();
    }
    
    const std::set<uint32_t>& neighbors(VirtualReg vreg) const {
        static std::set<uint32_t> empty;
        auto it = nodeIndex_.find(vreg.id);
        if (it == nodeIndex_.end()) return empty;
        return edges_[it->second];
    }
    
    const std::vector<VirtualReg>& nodes() const { return nodes_; }
    size_t nodeCount() const { return nodes_.size(); }
    
private:
    std::vector<VirtualReg> nodes_;
    std::map<uint32_t, size_t> nodeIndex_;
    std::vector<std::set<uint32_t>> edges_;
};

// =============================================================================
// Register Allocator
// =============================================================================

/**
 * @brief Graph coloring register allocator
 */
class GraphColoringRegAlloc {
public:
    static constexpr size_t NUM_REGS = 14;  // Available GPRs
    static constexpr size_t NUM_FP_REGS = 16;  // Available FPRs
    
    struct AllocationResult {
        std::map<uint32_t, Register> assignments;  // vreg -> physical reg
        std::map<uint32_t, int32_t> spillSlots;    // vreg -> stack offset
        size_t spillCount = 0;
        size_t coalescedMoves = 0;
    };
    
    GraphColoringRegAlloc() = default;
    
    /**
     * @brief Allocate registers for a function
     */
    AllocationResult allocate(const std::vector<LiveRange>& liveRanges, 
                              const std::vector<Register>& availableRegs);
    
private:
    InterferenceGraph graph_;
    std::set<uint32_t> simplifyWorklist_;
    std::set<uint32_t> freezeWorklist_;
    std::set<uint32_t> spillWorklist_;
    std::set<uint32_t> coalescedNodes_;
    std::set<uint32_t> spilledNodes_;
    std::vector<uint32_t> selectStack_;
    
    std::map<uint32_t, Register> color_;
    std::map<uint32_t, int32_t> spillSlots_;
    int32_t nextSpillSlot_ = 0;
    
    // Phase 1: Build interference graph
    void buildGraph(const std::vector<LiveRange>& ranges);
    
    // Phase 2: Simplify (remove low-degree nodes)
    void simplify();
    
    // Phase 3: Coalesce (merge move-related nodes)
    void coalesce();
    
    // Phase 4: Freeze (give up coalescing for low-degree nodes)
    void freeze();
    
    // Phase 5: Potential spill (select high-degree nodes)
    void selectSpill();
    
    // Phase 6: Select (assign colors)
    void select(const std::vector<Register>& availableRegs);
    
    // Helpers
    bool isSimplifiable(VirtualReg vreg, size_t numRegs);
    VirtualReg selectSpillCandidate();
    void decrementDegree(VirtualReg vreg);
    void addWorkList(VirtualReg vreg, size_t numRegs);
};

// =============================================================================
// Move Resolver
// =============================================================================

/**
 * @brief Resolves parallel moves (phi nodes, call arguments)
 * 
 * Handles cycles in move graphs using temporary registers or stack.
 */
class MoveResolver {
public:
    struct Move {
        Register from;
        Register to;
        bool isStack = false;
        int32_t stackOffset = 0;
    };
    
    void addMove(Register from, Register to) {
        moves_.push_back({from, to, false, 0});
    }
    
    void addMoveToStack(Register from, int32_t offset) {
        moves_.push_back({from, Register::invalid(), true, offset});
    }
    
    void addMoveFromStack(int32_t offset, Register to) {
        moves_.push_back({Register::invalid(), to, true, offset});
    }
    
    /**
     * @brief Emit resolved moves
     */
    void emit(MacroAssembler& masm, Register scratch);
    
    void clear() { moves_.clear(); }
    
private:
    std::vector<Move> moves_;
    
    std::vector<Move> orderMoves(Register scratch);
    bool detectCycle(const std::vector<std::vector<size_t>>& graph, size_t start);
};

// =============================================================================
// Spill Code Generator
// =============================================================================

/**
 * @brief Generates spill/reload code
 */
class SpillCodeGen {
public:
    explicit SpillCodeGen(MacroAssembler& masm) : masm_(masm) {}
    
    void spill(Register reg, int32_t stackOffset);
    void reload(int32_t stackOffset, Register reg);
    
    void spillFloat(Register reg, int32_t stackOffset);
    void reloadFloat(int32_t stackOffset, Register reg);
    
private:
    MacroAssembler& masm_;
};

} // namespace Zepra::JIT
