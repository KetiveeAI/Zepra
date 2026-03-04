/**
 * @file B3LoopPeeling.h
 * @brief Loop Peeling Optimization
 * 
 * Peel first iteration of loops to:
 * - Specialize on loop-invariant conditions
 * - Enable better inlining decisions
 * - Reduce branch mispredictions
 */

#pragma once

#include "b3/B3Procedure.h"
#include "b3/B3BasicBlock.h"
#include "b3/B3Value.h"

#include <vector>
#include <unordered_map>

namespace Zepra::B3 {

/**
 * @brief Loop information
 */
struct LoopInfo {
    BasicBlock* header = nullptr;
    BasicBlock* preheader = nullptr;
    std::vector<BasicBlock*> body;
    std::vector<BasicBlock*> exits;
    BasicBlock* latch = nullptr;  // Back edge source
    
    bool isSimple() const {
        // Single latch, single exit
        return latch && exits.size() == 1;
    }
    
    size_t bodySize() const {
        size_t count = 0;
        for (auto* block : body) {
            count += block->size();
        }
        return count;
    }
};

/**
 * @brief Loop detection
 */
class LoopDetector {
public:
    explicit LoopDetector(Procedure* proc) : proc_(proc) {}
    
    std::vector<LoopInfo> detectLoops() {
        std::vector<LoopInfo> loops;
        
        // Find back edges (basic loop detection)
        proc_->forEachBlock([&](BasicBlock* block) {
            for (BasicBlock* succ : block->successors()) {
                if (dominates(succ, block)) {
                    // Back edge: block → succ
                    LoopInfo loop;
                    loop.header = succ;
                    loop.latch = block;
                    collectLoopBody(loop);
                    loops.push_back(loop);
                }
            }
        });
        
        return loops;
    }
    
private:
    bool dominates(BasicBlock* a, BasicBlock* b) {
        // Simplified: a dominates b if a appears before b in RPO
        // Would use proper dominator tree in production
        return a->index() <= b->index();
    }
    
    void collectLoopBody(LoopInfo& loop) {
        std::unordered_set<BasicBlock*> visited;
        std::vector<BasicBlock*> worklist = {loop.latch};
        
        while (!worklist.empty()) {
            BasicBlock* block = worklist.back();
            worklist.pop_back();
            
            if (visited.count(block)) continue;
            visited.insert(block);
            
            loop.body.push_back(block);
            
            if (block == loop.header) continue;
            
            for (BasicBlock* pred : block->predecessors()) {
                worklist.push_back(pred);
            }
        }
        
        // Find preheader (predecessor of header not in loop)
        for (BasicBlock* pred : loop.header->predecessors()) {
            if (!visited.count(pred)) {
                loop.preheader = pred;
                break;
            }
        }
        
        // Find exits
        for (BasicBlock* block : loop.body) {
            for (BasicBlock* succ : block->successors()) {
                if (!visited.count(succ)) {
                    loop.exits.push_back(succ);
                }
            }
        }
    }
    
    Procedure* proc_;
};

/**
 * @brief Loop peeling transformation
 */
class LoopPeeling {
public:
    explicit LoopPeeling(Procedure* proc) : proc_(proc) {}
    
    // Peel first iteration of simple loops
    bool run() {
        LoopDetector detector(proc_);
        auto loops = detector.detectLoops();
        
        for (const LoopInfo& loop : loops) {
            if (shouldPeel(loop)) {
                peelLoop(loop);
                peeledCount_++;
            }
        }
        
        return peeledCount_ > 0;
    }
    
    size_t peeledCount() const { return peeledCount_; }
    
private:
    bool shouldPeel(const LoopInfo& loop) {
        // Only peel simple loops
        if (!loop.isSimple()) return false;
        
        // Don't peel large loops
        if (loop.bodySize() > 50) return false;
        
        // Would check for loop-invariant conditions
        return true;
    }
    
    void peelLoop(const LoopInfo& loop) {
        // 1. Clone loop body
        std::unordered_map<BasicBlock*, BasicBlock*> cloneMap;
        
        for (BasicBlock* block : loop.body) {
            BasicBlock* clone = proc_->createBlock();
            cloneMap[block] = clone;
            
            // Copy values
            for (Value* v : *block) {
                Value* cloned = clone->appendValue(v->opcode(), v->type());
                // Would copy operands
            }
        }
        
        // 2. Redirect preheader to cloned header
        if (loop.preheader) {
            BasicBlock* clonedHeader = cloneMap[loop.header];
            loop.preheader->replaceSuccessor(loop.header, clonedHeader);
        }
        
        // 3. Redirect cloned latch to original header
        BasicBlock* clonedLatch = cloneMap[loop.latch];
        (void)clonedLatch;  // Would add jump
    }
    
    Procedure* proc_;
    size_t peeledCount_ = 0;
};

} // namespace Zepra::B3
