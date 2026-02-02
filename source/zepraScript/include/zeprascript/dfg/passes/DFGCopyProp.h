/**
 * @file DFGCopyProp.h
 * @brief DFG Copy Propagation Pass
 * 
 * Replaces uses of copies with the original value,
 * enabling other optimizations to work on canonical values.
 */

#pragma once

#include "../DFGGraph.h"

namespace Zepra::DFG {

class CopyPropagation {
public:
    explicit CopyPropagation(Graph* graph) : graph_(graph) {}
    
    bool run();
    
private:
    bool propagateValue(Value* v);
    Value* findOriginal(Value* v);
    bool isCopyLike(Value* v) const;
    
    Graph* graph_;
};

// =============================================================================
// Implementation
// =============================================================================

inline bool CopyPropagation::run() {
    bool changed = false;
    
    // Propagate copies until fixed point
    bool iterChanged;
    do {
        iterChanged = false;
        
        for (BasicBlock* block : graph_->reversePostOrder()) {
            for (Value* v : block->values()) {
                if (propagateValue(v)) {
                    iterChanged = true;
                    changed = true;
                }
            }
            
            // Also process phi nodes
            for (Value* phi : block->phis()) {
                if (propagateValue(phi)) {
                    iterChanged = true;
                    changed = true;
                }
            }
        }
    } while (iterChanged);
    
    if (changed) {
        graph_->removeDeadValues();
    }
    
    return changed;
}

inline bool CopyPropagation::propagateValue(Value* v) {
    if (v->isDead()) return false;
    if (v->numInputs() == 0) return false;
    
    bool changed = false;
    
    // Replace copy-like inputs with their originals
    for (uint32_t i = 0; i < v->numInputs(); ++i) {
        Value* input = v->input(i);
        if (!input) continue;
        
        Value* original = findOriginal(input);
        if (original != input) {
            v->setInput(i, original);
            changed = true;
        }
    }
    
    // If this is a trivial phi (all inputs same), replace with that input
    if (v->opcode() == Opcode::Phi) {
        Value* same = nullptr;
        bool trivial = true;
        
        for (uint32_t i = 0; i < v->numInputs(); ++i) {
            Value* input = v->input(i);
            if (!input || input == v) continue;
            
            if (same == nullptr) {
                same = input;
            } else if (same != input) {
                trivial = false;
                break;
            }
        }
        
        if (trivial && same) {
            v->replaceAllUsesWith(same);
            v->markDead();
            return true;
        }
    }
    
    return changed;
}

inline Value* CopyPropagation::findOriginal(Value* v) {
    if (!v) return nullptr;
    
    // Follow copy chains
    while (isCopyLike(v) && v->numInputs() > 0 && v->input(0)) {
        v = v->input(0);
    }
    
    return v;
}

inline bool CopyPropagation::isCopyLike(Value* v) const {
    if (!v || v->numInputs() != 1) return false;
    
    // Operations that just pass through their input unchanged
    switch (v->opcode()) {
        // Identity conversions (same bit width)
        case Opcode::Extend32STo64:
        case Opcode::Extend32UTo64:
            // Not copies - different types
            return false;
            
        default:
            // Phi with single input is copy-like
            if (v->opcode() == Opcode::Phi) {
                Value* same = nullptr;
                for (uint32_t i = 0; i < v->numInputs(); ++i) {
                    Value* input = v->input(i);
                    if (!input || input == v) continue;
                    if (same == nullptr) same = input;
                    else if (same != input) return false;
                }
                return true;
            }
            return false;
    }
}

} // namespace Zepra::DFG
