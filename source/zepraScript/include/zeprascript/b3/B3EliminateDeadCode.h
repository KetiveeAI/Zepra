/**
 * @file B3EliminateDeadCode.h
 * @brief B3 Dead Code Elimination
 * 
 * Removes dead values from B3 IR.
 */

#pragma once

#include "B3Procedure.h"
#include <vector>

namespace Zepra::B3 {

class EliminateDeadCode {
public:
    explicit EliminateDeadCode(Procedure* proc) : proc_(proc) {}
    
    bool run();
    
private:
    void markLive(Value* v);
    
    Procedure* proc_;
    std::vector<bool> live_;
};

inline bool EliminateDeadCode::run() {
    // Initialize liveness
    live_.resize(proc_->values().size(), false);
    
    // Mark all values with side effects or users as live
    for (const auto& block : proc_->blocks()) {
        for (Value* v : block->values()) {
            // Terminators are always live
            if (isTerminal(v->opcode())) {
                markLive(v);
            }
            // Memory stores are live
            if (isStore(v->opcode())) {
                markLive(v);
            }
            // Calls may have side effects
            if (v->opcode() == Opcode::CCall) {
                markLive(v);
            }
        }
    }
    
    // Mark dead values
    bool changed = false;
    for (const auto& value : proc_->values()) {
        if (!live_[value->index()] && !value->isDead()) {
            value->markDead();
            changed = true;
        }
    }
    
    return changed;
}

inline void EliminateDeadCode::markLive(Value* v) {
    if (!v || v->index() >= live_.size()) return;
    if (live_[v->index()]) return;
    
    live_[v->index()] = true;
    
    // Mark all inputs as live
    for (uint32_t i = 0; i < v->numInputs(); ++i) {
        markLive(v->input(i));
    }
}

} // namespace Zepra::B3
