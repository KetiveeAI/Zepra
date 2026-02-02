/**
 * @file B3PhiElimination.h
 * @brief Phi Node Elimination Pass
 * 
 * Simplify SSA by eliminating redundant Phi nodes:
 * - Single-operand Phi → replace with operand
 * - All-same-operand Phi → replace with constant
 * - Dead Phi removal
 */

#pragma once

#include "../B3Procedure.h"
#include "../B3BasicBlock.h"
#include "../B3Value.h"

#include <unordered_set>

namespace Zepra::B3 {

/**
 * @brief Eliminates redundant Phi nodes
 */
class PhiElimination {
public:
    explicit PhiElimination(Procedure* proc) : proc_(proc) {}
    
    bool run() {
        bool changed = true;
        while (changed) {
            changed = false;
            
            proc_->forEachBlock([this, &changed](BasicBlock* block) {
                changed |= processBlock(block);
            });
        }
        
        removeDeadPhis();
        return eliminatedCount_ > 0;
    }
    
    size_t eliminatedCount() const { return eliminatedCount_; }
    
private:
    bool processBlock(BasicBlock* block) {
        bool changed = false;
        
        for (auto it = block->begin(); it != block->end(); ) {
            Value* v = *it;
            
            if (!v->isPhi()) {
                ++it;
                continue;
            }
            
            // Single operand Phi
            if (v->numChildren() == 1) {
                v->replaceAllUsesWith(v->child(0));
                it = block->remove(it);
                eliminatedCount_++;
                changed = true;
                continue;
            }
            
            // All same operand
            Value* first = v->child(0);
            bool allSame = true;
            for (size_t i = 1; i < v->numChildren(); i++) {
                if (v->child(i) != first && v->child(i) != v) {
                    allSame = false;
                    break;
                }
            }
            
            if (allSame) {
                v->replaceAllUsesWith(first);
                it = block->remove(it);
                eliminatedCount_++;
                changed = true;
                continue;
            }
            
            ++it;
        }
        
        return changed;
    }
    
    void removeDeadPhis() {
        proc_->forEachBlock([](BasicBlock* block) {
            for (auto it = block->begin(); it != block->end(); ) {
                Value* v = *it;
                if (v->isPhi() && v->uses().empty()) {
                    it = block->remove(it);
                } else {
                    ++it;
                }
            }
        });
    }
    
    Procedure* proc_;
    size_t eliminatedCount_ = 0;
};

} // namespace Zepra::B3
