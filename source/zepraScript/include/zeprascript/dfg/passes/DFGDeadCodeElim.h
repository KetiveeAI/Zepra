/**
 * @file DFGDeadCodeElim.h
 * @brief DFG Dead Code Elimination Pass
 * 
 * Removes values with no users (that don't have side effects).
 */

#pragma once

#include "../DFGGraph.h"

namespace Zepra::DFG {

class DeadCodeElimination {
public:
    explicit DeadCodeElimination(Graph* graph) : graph_(graph) {}
    
    bool run() {
        bool changed = false;
        bool localChanged;
        
        // Iterate until fixpoint
        do {
            localChanged = false;
            
            for (BasicBlock* block : graph_->reversePostOrder()) {
                // Process in reverse order within block
                auto& values = block->values();
                for (int i = static_cast<int>(values.size()) - 1; i >= 0; --i) {
                    Value* v = values[i];
                    if (shouldEliminate(v)) {
                        eliminate(v);
                        localChanged = true;
                    }
                }
                
                // Process phis
                auto& phis = block->phis();
                for (int i = static_cast<int>(phis.size()) - 1; i >= 0; --i) {
                    Value* phi = phis[i];
                    if (shouldEliminate(phi)) {
                        eliminate(phi);
                        localChanged = true;
                    }
                }
            }
            
            if (localChanged) {
                changed = true;
            }
        } while (localChanged);
        
        // Remove dead values from graph
        if (changed) {
            graph_->removeDeadValues();
        }
        
        return changed;
    }
    
private:
    bool shouldEliminate(Value* v) {
        if (v->isDead()) return false;
        if (v->hasUsers()) return false;
        if (hasSideEffects(v->opcode())) return false;
        if (isTerminator(v->opcode())) return false;
        return true;
    }
    
    void eliminate(Value* v) {
        // Remove from users of our inputs
        v->clearInputs();
        v->markDead();
    }
    
    Graph* graph_;
};

} // namespace Zepra::DFG
