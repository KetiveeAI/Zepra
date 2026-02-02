/**
 * @file B3TailDuplication.h
 * @brief Tail Duplication Optimization
 * 
 * Duplicate block tails to:
 * - Straighten control flow
 * - Enable better phi elimination
 * - Reduce branch overhead
 */

#pragma once

#include "../B3Procedure.h"
#include "../B3BasicBlock.h"
#include "../B3Value.h"

#include <unordered_map>

namespace Zepra::B3 {

/**
 * @brief Tail duplication transformation
 */
class TailDuplication {
public:
    explicit TailDuplication(Procedure* proc) : proc_(proc) {}
    
    // Run tail duplication
    bool run() {
        std::vector<BasicBlock*> candidates = findCandidates();
        
        for (BasicBlock* block : candidates) {
            if (shouldDuplicate(block)) {
                duplicateTail(block);
                duplicatedCount_++;
            }
        }
        
        return duplicatedCount_ > 0;
    }
    
    size_t duplicatedCount() const { return duplicatedCount_; }
    
private:
    std::vector<BasicBlock*> findCandidates() {
        std::vector<BasicBlock*> result;
        
        proc_->forEachBlock([&](BasicBlock* block) {
            // Candidate: block has multiple predecessors and small size
            if (block->predecessors().size() > 1 && 
                block->size() <= maxDuplicateSize_) {
                result.push_back(block);
            }
        });
        
        return result;
    }
    
    bool shouldDuplicate(BasicBlock* block) {
        // Don't duplicate entry block
        if (block == proc_->entryBlock()) return false;
        
        // Don't duplicate if it would cause code explosion
        size_t numPreds = block->predecessors().size();
        size_t expansion = block->size() * (numPreds - 1);
        if (totalExpansion_ + expansion > maxTotalExpansion_) {
            return false;
        }
        
        // Check if duplication enables other optimizations
        return benefitsFromDuplication(block);
    }
    
    bool benefitsFromDuplication(BasicBlock* block) {
        // Benefits if:
        // 1. Block ends with conditional branch
        // 2. Different predecessors take different paths
        // 3. Enables constant folding in duplicates
        
        if (block->successors().empty()) return false;
        
        // Has phi nodes that would be eliminated
        for (Value* v : *block) {
            if (v->isPhi()) return true;
        }
        
        return false;
    }
    
    void duplicateTail(BasicBlock* block) {
        std::vector<BasicBlock*> preds(block->predecessors().begin(),
                                       block->predecessors().end());
        
        // Keep first predecessor pointing to original
        // Duplicate for remaining predecessors
        for (size_t i = 1; i < preds.size(); i++) {
            BasicBlock* pred = preds[i];
            BasicBlock* clone = cloneBlock(block, pred);
            
            // Redirect predecessor to clone
            pred->replaceSuccessor(block, clone);
            
            totalExpansion_ += block->size();
        }
    }
    
    BasicBlock* cloneBlock(BasicBlock* block, BasicBlock* forPred) {
        BasicBlock* clone = proc_->createBlock();
        
        std::unordered_map<Value*, Value*> valueMap;
        
        for (Value* v : *block) {
            Value* cloned;
            
            if (v->isPhi()) {
                // Phi becomes the value from forPred
                for (size_t i = 0; i < v->numChildren(); i++) {
                    // Would check which child corresponds to forPred
                    cloned = clone->appendValue(v->child(i)->opcode(), v->type());
                    break;
                }
            } else {
                cloned = clone->appendValue(v->opcode(), v->type());
                // Map operands
                for (size_t i = 0; i < v->numChildren(); i++) {
                    Value* child = v->child(i);
                    if (valueMap.count(child)) {
                        cloned->setChild(i, valueMap[child]);
                    } else {
                        cloned->setChild(i, child);
                    }
                }
            }
            
            valueMap[v] = cloned;
        }
        
        // Clone successor edges
        for (BasicBlock* succ : block->successors()) {
            clone->addSuccessor(succ);
        }
        
        return clone;
    }
    
    Procedure* proc_;
    size_t duplicatedCount_ = 0;
    size_t totalExpansion_ = 0;
    
    static constexpr size_t maxDuplicateSize_ = 10;
    static constexpr size_t maxTotalExpansion_ = 100;
};

} // namespace Zepra::B3
