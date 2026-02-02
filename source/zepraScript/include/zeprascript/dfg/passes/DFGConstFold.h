/**
 * @file DFGConstFold.h
 * @brief DFG Constant Folding Pass
 * 
 * Evaluates operations with constant operands at compile time.
 */

#pragma once

#include "../DFGGraph.h"

namespace Zepra::DFG {

class ConstantFolding {
public:
    explicit ConstantFolding(Graph* graph) : graph_(graph) {}
    
    bool run();
    
private:
    bool foldValue(Value* v);
    
    // Fold specific operations
    Value* foldBinaryI32(Value* v);
    Value* foldBinaryI64(Value* v);
    Value* foldBinaryF32(Value* v);
    Value* foldBinaryF64(Value* v);
    Value* foldUnary(Value* v);
    Value* foldCompare(Value* v);
    
    Graph* graph_;
};

// =============================================================================
// Implementation
// =============================================================================

inline bool ConstantFolding::run() {
    bool changed = false;
    
    for (BasicBlock* block : graph_->reversePostOrder()) {
        for (Value* v : block->values()) {
            if (foldValue(v)) {
                changed = true;
            }
        }
    }
    
    if (changed) {
        graph_->removeDeadValues();
    }
    
    return changed;
}

inline bool ConstantFolding::foldValue(Value* v) {
    if (v->isDead()) return false;
    if (!hasResult(v->opcode())) return false;
    if (v->numInputs() == 0) return false;
    
    // Check if all inputs are constants
    for (uint32_t i = 0; i < v->numInputs(); ++i) {
        if (!v->input(i) || !v->input(i)->isConstant()) {
            return false;
        }
    }
    
    Value* folded = nullptr;
    
    switch (v->type()) {
        case Type::I32:
            folded = foldBinaryI32(v);
            break;
        case Type::I64:
            folded = foldBinaryI64(v);
            break;
        case Type::F32:
            folded = foldBinaryF32(v);
            break;
        case Type::F64:
            folded = foldBinaryF64(v);
            break;
        default:
            break;
    }
    
    if (!folded) {
        folded = foldCompare(v);
    }
    
    if (folded) {
        v->replaceAllUsesWith(folded);
        v->markDead();
        return true;
    }
    
    return false;
}

inline Value* ConstantFolding::foldBinaryI32(Value* v) {
    if (v->numInputs() < 2) return nullptr;
    
    int32_t lhs = v->input(0)->asInt32();
    int32_t rhs = v->input(1)->asInt32();
    int32_t result = 0;
    
    switch (v->opcode()) {
        case Opcode::Add32: result = lhs + rhs; break;
        case Opcode::Sub32: result = lhs - rhs; break;
        case Opcode::Mul32: result = lhs * rhs; break;
        case Opcode::And32: result = lhs & rhs; break;
        case Opcode::Or32:  result = lhs | rhs; break;
        case Opcode::Xor32: result = lhs ^ rhs; break;
        case Opcode::Shl32: result = lhs << (rhs & 31); break;
        case Opcode::Shr32U: result = static_cast<int32_t>(static_cast<uint32_t>(lhs) >> (rhs & 31)); break;
        case Opcode::Shr32S: result = lhs >> (rhs & 31); break;
        case Opcode::Div32S:
            if (rhs == 0) return nullptr;  // Don't fold division by zero
            result = lhs / rhs;
            break;
        case Opcode::Div32U:
            if (rhs == 0) return nullptr;
            result = static_cast<int32_t>(static_cast<uint32_t>(lhs) / static_cast<uint32_t>(rhs));
            break;
        case Opcode::Rem32S:
            if (rhs == 0) return nullptr;
            result = lhs % rhs;
            break;
        case Opcode::Rem32U:
            if (rhs == 0) return nullptr;
            result = static_cast<int32_t>(static_cast<uint32_t>(lhs) % static_cast<uint32_t>(rhs));
            break;
        default:
            return nullptr;
    }
    
    return graph_->constInt32(result);
}

inline Value* ConstantFolding::foldBinaryI64(Value* v) {
    if (v->numInputs() < 2) return nullptr;
    
    int64_t lhs = v->input(0)->asInt64();
    int64_t rhs = v->input(1)->asInt64();
    int64_t result = 0;
    
    switch (v->opcode()) {
        case Opcode::Add64: result = lhs + rhs; break;
        case Opcode::Sub64: result = lhs - rhs; break;
        case Opcode::Mul64: result = lhs * rhs; break;
        case Opcode::And64: result = lhs & rhs; break;
        case Opcode::Or64:  result = lhs | rhs; break;
        case Opcode::Xor64: result = lhs ^ rhs; break;
        case Opcode::Shl64: result = lhs << (rhs & 63); break;
        case Opcode::Shr64U: result = static_cast<int64_t>(static_cast<uint64_t>(lhs) >> (rhs & 63)); break;
        case Opcode::Shr64S: result = lhs >> (rhs & 63); break;
        case Opcode::Div64S:
            if (rhs == 0) return nullptr;
            result = lhs / rhs;
            break;
        case Opcode::Div64U:
            if (rhs == 0) return nullptr;
            result = static_cast<int64_t>(static_cast<uint64_t>(lhs) / static_cast<uint64_t>(rhs));
            break;
        default:
            return nullptr;
    }
    
    return graph_->constInt64(result);
}

inline Value* ConstantFolding::foldBinaryF32(Value* v) {
    if (v->numInputs() < 2) return nullptr;
    
    float lhs = v->input(0)->asFloat32();
    float rhs = v->input(1)->asFloat32();
    float result = 0;
    
    switch (v->opcode()) {
        case Opcode::AddF32: result = lhs + rhs; break;
        case Opcode::SubF32: result = lhs - rhs; break;
        case Opcode::MulF32: result = lhs * rhs; break;
        case Opcode::DivF32: result = lhs / rhs; break;
        default:
            return nullptr;
    }
    
    return graph_->constFloat32(result);
}

inline Value* ConstantFolding::foldBinaryF64(Value* v) {
    if (v->numInputs() < 2) return nullptr;
    
    double lhs = v->input(0)->asFloat64();
    double rhs = v->input(1)->asFloat64();
    double result = 0;
    
    switch (v->opcode()) {
        case Opcode::AddF64: result = lhs + rhs; break;
        case Opcode::SubF64: result = lhs - rhs; break;
        case Opcode::MulF64: result = lhs * rhs; break;
        case Opcode::DivF64: result = lhs / rhs; break;
        default:
            return nullptr;
    }
    
    return graph_->constFloat64(result);
}

inline Value* ConstantFolding::foldUnary(Value* v) {
    if (v->numInputs() < 1) return nullptr;
    
    Value* input = v->input(0);
    
    switch (v->opcode()) {
        case Opcode::Eqz32:
            return graph_->constInt32(input->asInt32() == 0 ? 1 : 0);
        case Opcode::Eqz64:
            return graph_->constInt32(input->asInt64() == 0 ? 1 : 0);
        default:
            return nullptr;
    }
}

inline Value* ConstantFolding::foldCompare(Value* v) {
    if (v->numInputs() < 2) return nullptr;
    
    int32_t result = 0;
    
    // I32 comparisons
    if (v->input(0)->type() == Type::I32) {
        int32_t lhs = v->input(0)->asInt32();
        int32_t rhs = v->input(1)->asInt32();
        uint32_t ulhs = static_cast<uint32_t>(lhs);
        uint32_t urhs = static_cast<uint32_t>(rhs);
        
        switch (v->opcode()) {
            case Opcode::Eq32: result = (lhs == rhs) ? 1 : 0; break;
            case Opcode::Ne32: result = (lhs != rhs) ? 1 : 0; break;
            case Opcode::Lt32S: result = (lhs < rhs) ? 1 : 0; break;
            case Opcode::Lt32U: result = (ulhs < urhs) ? 1 : 0; break;
            case Opcode::Le32S: result = (lhs <= rhs) ? 1 : 0; break;
            case Opcode::Le32U: result = (ulhs <= urhs) ? 1 : 0; break;
            case Opcode::Gt32S: result = (lhs > rhs) ? 1 : 0; break;
            case Opcode::Gt32U: result = (ulhs > urhs) ? 1 : 0; break;
            case Opcode::Ge32S: result = (lhs >= rhs) ? 1 : 0; break;
            case Opcode::Ge32U: result = (ulhs >= urhs) ? 1 : 0; break;
            default: return nullptr;
        }
    }
    // I64 comparisons
    else if (v->input(0)->type() == Type::I64) {
        int64_t lhs = v->input(0)->asInt64();
        int64_t rhs = v->input(1)->asInt64();
        
        switch (v->opcode()) {
            case Opcode::Eq64: result = (lhs == rhs) ? 1 : 0; break;
            case Opcode::Ne64: result = (lhs != rhs) ? 1 : 0; break;
            case Opcode::Lt64S: result = (lhs < rhs) ? 1 : 0; break;
            case Opcode::Le64S: result = (lhs <= rhs) ? 1 : 0; break;
            case Opcode::Gt64S: result = (lhs > rhs) ? 1 : 0; break;
            case Opcode::Ge64S: result = (lhs >= rhs) ? 1 : 0; break;
            default: return nullptr;
        }
    }
    else {
        return nullptr;
    }
    
    return graph_->constInt32(result);
}

} // namespace Zepra::DFG
