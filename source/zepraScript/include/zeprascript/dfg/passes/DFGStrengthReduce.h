/**
 * @file DFGStrengthReduce.h
 * @brief DFG Strength Reduction Pass
 * 
 * Replaces expensive operations with cheaper equivalents:
 * - Multiply/divide by powers of 2 → shifts
 * - Multiply by 0 → 0
 * - Add/sub 0 → identity
 * - Multiply by 1 → identity
 * - Division by 1 → identity
 */

#pragma once

#include "../DFGGraph.h"

namespace Zepra::DFG {

class StrengthReduction {
public:
    explicit StrengthReduction(Graph* graph) : graph_(graph) {}
    
    bool run();
    
private:
    bool reduceValue(Value* v);
    Value* reduceAdd(Value* v);
    Value* reduceSub(Value* v);
    Value* reduceMul(Value* v);
    Value* reduceDiv(Value* v);
    Value* reduceAnd(Value* v);
    Value* reduceOr(Value* v);
    Value* reduceShl(Value* v);
    Value* reduceShr(Value* v);
    
    bool isPowerOf2(int64_t val) const;
    int log2(int64_t val) const;
    
    Graph* graph_;
};

// =============================================================================
// Implementation
// =============================================================================

inline bool StrengthReduction::run() {
    bool changed = false;
    
    for (BasicBlock* block : graph_->reversePostOrder()) {
        for (Value* v : block->values()) {
            if (reduceValue(v)) {
                changed = true;
            }
        }
    }
    
    if (changed) {
        graph_->removeDeadValues();
    }
    
    return changed;
}

inline bool StrengthReduction::reduceValue(Value* v) {
    if (v->isDead()) return false;
    if (v->numInputs() < 2) return false;
    
    Value* reduced = nullptr;
    
    switch (v->opcode()) {
        case Opcode::Add32:
        case Opcode::Add64:
            reduced = reduceAdd(v);
            break;
            
        case Opcode::Sub32:
        case Opcode::Sub64:
            reduced = reduceSub(v);
            break;
            
        case Opcode::Mul32:
        case Opcode::Mul64:
            reduced = reduceMul(v);
            break;
            
        case Opcode::Div32S:
        case Opcode::Div32U:
        case Opcode::Div64S:
        case Opcode::Div64U:
            reduced = reduceDiv(v);
            break;
            
        case Opcode::And32:
        case Opcode::And64:
            reduced = reduceAnd(v);
            break;
            
        case Opcode::Or32:
        case Opcode::Or64:
            reduced = reduceOr(v);
            break;
            
        case Opcode::Shl32:
        case Opcode::Shl64:
            reduced = reduceShl(v);
            break;
            
        case Opcode::Shr32S:
        case Opcode::Shr32U:
        case Opcode::Shr64S:
        case Opcode::Shr64U:
            reduced = reduceShr(v);
            break;
            
        default:
            break;
    }
    
    if (reduced) {
        v->replaceAllUsesWith(reduced);
        v->markDead();
        return true;
    }
    
    return false;
}

inline Value* StrengthReduction::reduceAdd(Value* v) {
    Value* lhs = v->input(0);
    Value* rhs = v->input(1);
    
    // x + 0 → x
    if (rhs->isConstant()) {
        if (v->type() == Type::I32 && rhs->asInt32() == 0) return lhs;
        if (v->type() == Type::I64 && rhs->asInt64() == 0) return lhs;
    }
    if (lhs->isConstant()) {
        if (v->type() == Type::I32 && lhs->asInt32() == 0) return rhs;
        if (v->type() == Type::I64 && lhs->asInt64() == 0) return rhs;
    }
    
    return nullptr;
}

inline Value* StrengthReduction::reduceSub(Value* v) {
    Value* lhs = v->input(0);
    Value* rhs = v->input(1);
    
    // x - 0 → x
    if (rhs->isConstant()) {
        if (v->type() == Type::I32 && rhs->asInt32() == 0) return lhs;
        if (v->type() == Type::I64 && rhs->asInt64() == 0) return lhs;
    }
    
    // x - x → 0
    if (lhs == rhs) {
        return v->type() == Type::I32 ? graph_->constInt32(0) : graph_->constInt64(0);
    }
    
    return nullptr;
}

inline Value* StrengthReduction::reduceMul(Value* v) {
    Value* lhs = v->input(0);
    Value* rhs = v->input(1);
    
    if (rhs->isConstant()) {
        int64_t val = (v->type() == Type::I32) ? rhs->asInt32() : rhs->asInt64();
        
        // x * 0 → 0
        if (val == 0) {
            return v->type() == Type::I32 ? graph_->constInt32(0) : graph_->constInt64(0);
        }
        
        // x * 1 → x
        if (val == 1) return lhs;
        
        // x * -1 → 0 - x (negate)
        if (val == -1) {
            Value* zero = v->type() == Type::I32 ? graph_->constInt32(0) : graph_->constInt64(0);
            Opcode subOp = v->type() == Type::I32 ? Opcode::Sub32 : Opcode::Sub64;
            return graph_->addValue(subOp, v->type(), zero, lhs);
        }
        
        // x * power-of-2 → x << log2
        if (val > 0 && isPowerOf2(val)) {
            int shift = log2(val);
            Value* shiftConst = v->type() == Type::I32 ? 
                graph_->constInt32(shift) : graph_->constInt64(shift);
            Opcode shlOp = v->type() == Type::I32 ? Opcode::Shl32 : Opcode::Shl64;
            return graph_->addValue(shlOp, v->type(), lhs, shiftConst);
        }
    }
    
    // Commutative: check lhs too
    if (lhs->isConstant()) {
        int64_t val = (v->type() == Type::I32) ? lhs->asInt32() : lhs->asInt64();
        if (val == 0) {
            return v->type() == Type::I32 ? graph_->constInt32(0) : graph_->constInt64(0);
        }
        if (val == 1) return rhs;
        if (val > 0 && isPowerOf2(val)) {
            int shift = log2(val);
            Value* shiftConst = v->type() == Type::I32 ? 
                graph_->constInt32(shift) : graph_->constInt64(shift);
            Opcode shlOp = v->type() == Type::I32 ? Opcode::Shl32 : Opcode::Shl64;
            return graph_->addValue(shlOp, v->type(), rhs, shiftConst);
        }
    }
    
    return nullptr;
}

inline Value* StrengthReduction::reduceDiv(Value* v) {
    Value* lhs = v->input(0);
    Value* rhs = v->input(1);
    
    if (rhs->isConstant()) {
        int64_t val = (v->type() == Type::I32) ? rhs->asInt32() : rhs->asInt64();
        
        // x / 1 → x
        if (val == 1) return lhs;
        
        // Unsigned division by power-of-2 → shift right
        bool isUnsigned = (v->opcode() == Opcode::Div32U || v->opcode() == Opcode::Div64U);
        if (isUnsigned && val > 0 && isPowerOf2(val)) {
            int shift = log2(val);
            Value* shiftConst = v->type() == Type::I32 ? 
                graph_->constInt32(shift) : graph_->constInt64(shift);
            Opcode shrOp = v->type() == Type::I32 ? Opcode::Shr32U : Opcode::Shr64U;
            return graph_->addValue(shrOp, v->type(), lhs, shiftConst);
        }
    }
    
    return nullptr;
}

inline Value* StrengthReduction::reduceAnd(Value* v) {
    Value* lhs = v->input(0);
    Value* rhs = v->input(1);
    
    // x & 0 → 0
    if (rhs->isConstant()) {
        if (v->type() == Type::I32 && rhs->asInt32() == 0) return graph_->constInt32(0);
        if (v->type() == Type::I64 && rhs->asInt64() == 0) return graph_->constInt64(0);
    }
    
    // x & -1 → x (all bits set)
    if (rhs->isConstant()) {
        if (v->type() == Type::I32 && rhs->asInt32() == -1) return lhs;
        if (v->type() == Type::I64 && rhs->asInt64() == -1) return lhs;
    }
    
    // x & x → x
    if (lhs == rhs) return lhs;
    
    return nullptr;
}

inline Value* StrengthReduction::reduceOr(Value* v) {
    Value* lhs = v->input(0);
    Value* rhs = v->input(1);
    
    // x | 0 → x
    if (rhs->isConstant()) {
        if (v->type() == Type::I32 && rhs->asInt32() == 0) return lhs;
        if (v->type() == Type::I64 && rhs->asInt64() == 0) return lhs;
    }
    
    // x | -1 → -1 (all bits set)
    if (rhs->isConstant()) {
        if (v->type() == Type::I32 && rhs->asInt32() == -1) return graph_->constInt32(-1);
        if (v->type() == Type::I64 && rhs->asInt64() == -1) return graph_->constInt64(-1);
    }
    
    // x | x → x
    if (lhs == rhs) return lhs;
    
    return nullptr;
}

inline Value* StrengthReduction::reduceShl(Value* v) {
    Value* lhs = v->input(0);
    Value* rhs = v->input(1);
    
    // x << 0 → x
    if (rhs->isConstant()) {
        if (v->type() == Type::I32 && rhs->asInt32() == 0) return lhs;
        if (v->type() == Type::I64 && rhs->asInt64() == 0) return lhs;
    }
    
    // 0 << x → 0
    if (lhs->isConstant()) {
        if (v->type() == Type::I32 && lhs->asInt32() == 0) return graph_->constInt32(0);
        if (v->type() == Type::I64 && lhs->asInt64() == 0) return graph_->constInt64(0);
    }
    
    return nullptr;
}

inline Value* StrengthReduction::reduceShr(Value* v) {
    Value* lhs = v->input(0);
    Value* rhs = v->input(1);
    
    // x >> 0 → x
    if (rhs->isConstant()) {
        if (v->type() == Type::I32 && rhs->asInt32() == 0) return lhs;
        if (v->type() == Type::I64 && rhs->asInt64() == 0) return lhs;
    }
    
    // 0 >> x → 0
    if (lhs->isConstant()) {
        if (v->type() == Type::I32 && lhs->asInt32() == 0) return graph_->constInt32(0);
        if (v->type() == Type::I64 && lhs->asInt64() == 0) return graph_->constInt64(0);
    }
    
    return nullptr;
}

inline bool StrengthReduction::isPowerOf2(int64_t val) const {
    return val > 0 && (val & (val - 1)) == 0;
}

inline int StrengthReduction::log2(int64_t val) const {
    int result = 0;
    while (val > 1) {
        val >>= 1;
        result++;
    }
    return result;
}

} // namespace Zepra::DFG
