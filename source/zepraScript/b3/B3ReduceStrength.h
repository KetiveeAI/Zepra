/**
 * @file B3ReduceStrength.h
 * @brief B3 Strength Reduction Pass
 * 
 * Algebraic simplifications and canonicalizations for B3 IR.
 */

#pragma once

#include "B3Procedure.h"

namespace Zepra::B3 {

class ReduceStrength {
public:
    explicit ReduceStrength(Procedure* proc) : proc_(proc) {}
    
    bool run();
    
private:
    bool reduceValue(Value* v);
    Value* reduceAdd(Value* v);
    Value* reduceSub(Value* v);
    Value* reduceMul(Value* v);
    Value* reduceDiv(Value* v);
    Value* reduceBitAnd(Value* v);
    Value* reduceBitOr(Value* v);
    Value* reduceShl(Value* v);
    Value* reduceSShr(Value* v);
    
    bool isPowerOf2(int64_t v) const { return v > 0 && (v & (v - 1)) == 0; }
    int log2(int64_t v) const {
        int r = 0;
        while (v > 1) { v >>= 1; r++; }
        return r;
    }
    
    Procedure* proc_;
};

// Implementation
inline bool ReduceStrength::run() {
    bool changed = false;
    
    for (const auto& block : proc_->blocks()) {
        for (Value* v : block->values()) {
            if (reduceValue(v)) changed = true;
        }
    }
    
    return changed;
}

inline bool ReduceStrength::reduceValue(Value* v) {
    if (v->isDead()) return false;
    
    Value* replacement = nullptr;
    
    switch (v->opcode()) {
        case Opcode::Add: replacement = reduceAdd(v); break;
        case Opcode::Sub: replacement = reduceSub(v); break;
        case Opcode::Mul: replacement = reduceMul(v); break;
        case Opcode::Div:
        case Opcode::UDiv: replacement = reduceDiv(v); break;
        case Opcode::BitAnd: replacement = reduceBitAnd(v); break;
        case Opcode::BitOr: replacement = reduceBitOr(v); break;
        case Opcode::Shl: replacement = reduceShl(v); break;
        case Opcode::SShr:
        case Opcode::ZShr: replacement = reduceSShr(v); break;
        default: break;
    }
    
    if (replacement && replacement != v) {
        v->replaceAllUsesWith(replacement);
        v->markDead();
        return true;
    }
    return false;
}

inline Value* ReduceStrength::reduceAdd(Value* v) {
    if (v->numInputs() < 2) return nullptr;
    Value* lhs = v->input(0);
    Value* rhs = v->input(1);
    
    // x + 0 → x
    if (rhs->isConstant() && rhs->constInt64() == 0) return lhs;
    if (lhs->isConstant() && lhs->constInt64() == 0) return rhs;
    
    return nullptr;
}

inline Value* ReduceStrength::reduceSub(Value* v) {
    if (v->numInputs() < 2) return nullptr;
    Value* lhs = v->input(0);
    Value* rhs = v->input(1);
    
    // x - 0 → x
    if (rhs->isConstant() && rhs->constInt64() == 0) return lhs;
    // x - x → 0
    if (lhs == rhs) return proc_->constInt64(0);
    
    return nullptr;
}

inline Value* ReduceStrength::reduceMul(Value* v) {
    if (v->numInputs() < 2) return nullptr;
    Value* lhs = v->input(0);
    Value* rhs = v->input(1);
    
    if (rhs->isConstant()) {
        int64_t val = rhs->constInt64();
        if (val == 0) return proc_->constInt64(0);
        if (val == 1) return lhs;
        if (isPowerOf2(val)) {
            Value* shift = proc_->constInt32(log2(val));
            return proc_->addValue(Opcode::Shl, v->type(), lhs, shift);
        }
    }
    
    return nullptr;
}

inline Value* ReduceStrength::reduceDiv(Value* v) {
    if (v->numInputs() < 2) return nullptr;
    Value* lhs = v->input(0);
    Value* rhs = v->input(1);
    
    // x / 1 → x
    if (rhs->isConstant() && rhs->constInt64() == 1) return lhs;
    
    // Unsigned div by power of 2 → shift
    if (v->opcode() == Opcode::UDiv && rhs->isConstant()) {
        int64_t val = rhs->constInt64();
        if (isPowerOf2(val)) {
            Value* shift = proc_->constInt32(log2(val));
            return proc_->addValue(Opcode::ZShr, v->type(), lhs, shift);
        }
    }
    
    return nullptr;
}

inline Value* ReduceStrength::reduceBitAnd(Value* v) {
    if (v->numInputs() < 2) return nullptr;
    Value* lhs = v->input(0);
    Value* rhs = v->input(1);
    
    // x & 0 → 0
    if (rhs->isConstant() && rhs->constInt64() == 0) return proc_->constInt64(0);
    // x & -1 → x
    if (rhs->isConstant() && rhs->constInt64() == -1) return lhs;
    // x & x → x
    if (lhs == rhs) return lhs;
    
    return nullptr;
}

inline Value* ReduceStrength::reduceBitOr(Value* v) {
    if (v->numInputs() < 2) return nullptr;
    Value* lhs = v->input(0);
    Value* rhs = v->input(1);
    
    // x | 0 → x
    if (rhs->isConstant() && rhs->constInt64() == 0) return lhs;
    // x | x → x
    if (lhs == rhs) return lhs;
    
    return nullptr;
}

inline Value* ReduceStrength::reduceShl(Value* v) {
    if (v->numInputs() < 2) return nullptr;
    Value* lhs = v->input(0);
    Value* rhs = v->input(1);
    
    // x << 0 → x
    if (rhs->isConstant() && rhs->constInt32() == 0) return lhs;
    // 0 << x → 0
    if (lhs->isConstant() && lhs->constInt64() == 0) return proc_->constInt64(0);
    
    return nullptr;
}

inline Value* ReduceStrength::reduceSShr(Value* v) {
    if (v->numInputs() < 2) return nullptr;
    Value* lhs = v->input(0);
    Value* rhs = v->input(1);
    
    // x >> 0 → x
    if (rhs->isConstant() && rhs->constInt32() == 0) return lhs;
    
    return nullptr;
}

} // namespace Zepra::B3
