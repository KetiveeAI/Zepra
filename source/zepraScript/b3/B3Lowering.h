/**
 * @file B3Lowering.h
 * @brief DFG to B3 Lowering
 * 
 * Lowers DFG IR to B3 backend IR for final code generation.
 */

#pragma once

#include "B3Procedure.h"
#include "../dfg/DFGGraph.h"
#include <unordered_map>

namespace Zepra::B3 {

class DFGLowering {
public:
    DFGLowering(DFG::Graph* dfg, Procedure* b3)
        : dfg_(dfg), b3_(b3) {}
    
    bool lower();
    
private:
    Value* lowerValue(DFG::Value* dfgVal);
    BasicBlock* lowerBlock(DFG::BasicBlock* dfgBlock);
    
    Type convertType(DFG::Type t);
    Opcode convertOpcode(DFG::Opcode op);
    
    DFG::Graph* dfg_;
    Procedure* b3_;
    
    std::unordered_map<DFG::BasicBlock*, BasicBlock*> blockMap_;
    std::unordered_map<DFG::Value*, Value*> valueMap_;
};

// =============================================================================
// Implementation
// =============================================================================

inline bool DFGLowering::lower() {
    // Create B3 blocks for each DFG block
    for (uint32_t i = 0; i < dfg_->numBlocks(); ++i) {
        DFG::BasicBlock* dfgBlock = dfg_->block(i);
        BasicBlock* b3Block = b3_->addBlock();
        blockMap_[dfgBlock] = b3Block;
    }
    
    // Lower each block's values
    for (DFG::BasicBlock* dfgBlock : dfg_->reversePostOrder()) {
        BasicBlock* b3Block = blockMap_[dfgBlock];
        
        // Lower phi nodes first
        for (DFG::Value* phi : dfgBlock->phis()) {
            Value* b3Phi = lowerValue(phi);
            b3Block->appendValue(b3Phi);
        }
        
        // Lower regular values
        for (DFG::Value* dfgVal : dfgBlock->values()) {
            if (dfgVal->isDead()) continue;
            Value* b3Val = lowerValue(dfgVal);
            if (b3Val) {
                b3Block->appendValue(b3Val);
            }
        }
    }
    
    // Connect control flow edges
    for (uint32_t i = 0; i < dfg_->numBlocks(); ++i) {
        DFG::BasicBlock* dfgBlock = dfg_->block(i);
        BasicBlock* b3Block = blockMap_[dfgBlock];
        
        for (DFG::BasicBlock* dfgSucc : dfgBlock->successors()) {
            BasicBlock* b3Succ = blockMap_[dfgSucc];
            b3Block->addSuccessor(b3Succ);
            b3Succ->addPredecessor(b3Block);
        }
    }
    
    return true;
}

inline Value* DFGLowering::lowerValue(DFG::Value* dfgVal) {
    // Check if already lowered
    auto it = valueMap_.find(dfgVal);
    if (it != valueMap_.end()) {
        return it->second;
    }
    
    Opcode op = convertOpcode(dfgVal->opcode());
    Type type = convertType(dfgVal->type());
    
    Value* b3Val = nullptr;
    
    // Handle special cases
    switch (dfgVal->opcode()) {
        case DFG::Opcode::Const32:
            b3Val = b3_->constInt32(dfgVal->asInt32());
            break;
            
        case DFG::Opcode::Const64:
            b3Val = b3_->constInt64(dfgVal->asInt64());
            break;
            
        case DFG::Opcode::ConstF32:
            b3Val = b3_->constFloat(dfgVal->asFloat32());
            break;
            
        case DFG::Opcode::ConstF64:
            b3Val = b3_->constDouble(dfgVal->asFloat64());
            break;
            
        case DFG::Opcode::Phi: {
            b3Val = b3_->addValue(Opcode::Phi, type);
            // Add inputs later after all values are lowered
            break;
        }
        
        case DFG::Opcode::Return: {
            if (dfgVal->numInputs() > 0 && dfgVal->input(0)) {
                Value* result = lowerValue(dfgVal->input(0));
                b3Val = b3_->addValue(Opcode::Return, Type::Void, result);
            } else {
                b3Val = b3_->addValue(Opcode::Return, Type::Void);
            }
            break;
        }
        
        case DFG::Opcode::Jump: {
            b3Val = b3_->addValue(Opcode::Jump, Type::Void);
            // Connect successors later
            break;
        }
        
        case DFG::Opcode::Branch: {
            Value* cond = lowerValue(dfgVal->input(0));
            b3Val = b3_->addValue(Opcode::Branch, Type::Void, cond);
            break;
        }
        
        default: {
            // Generic lowering: convert opcode and inputs
            b3Val = b3_->addValue(op, type);
            for (uint32_t i = 0; i < dfgVal->numInputs(); ++i) {
                DFG::Value* dfgInput = dfgVal->input(i);
                if (dfgInput) {
                    Value* b3Input = lowerValue(dfgInput);
                    b3Val->addInput(b3Input);
                    b3Input->addUser(b3Val);
                }
            }
            break;
        }
    }
    
    if (b3Val) {
        valueMap_[dfgVal] = b3Val;
    }
    
    return b3Val;
}

inline Type DFGLowering::convertType(DFG::Type t) {
    switch (t) {
        case DFG::Type::I32: return Type::Int32;
        case DFG::Type::I64: return Type::Int64;
        case DFG::Type::F32: return Type::Float;
        case DFG::Type::F64: return Type::Double;
        case DFG::Type::Void: return Type::Void;
        default: return Type::Int64;
    }
}

inline Opcode DFGLowering::convertOpcode(DFG::Opcode op) {
    using DFGOp = DFG::Opcode;
    
    switch (op) {
        // Arithmetic
        case DFGOp::Add32:
        case DFGOp::Add64:
            return Opcode::Add;
        case DFGOp::Sub32:
        case DFGOp::Sub64:
            return Opcode::Sub;
        case DFGOp::Mul32:
        case DFGOp::Mul64:
            return Opcode::Mul;
        case DFGOp::Div32S:
        case DFGOp::Div64S:
            return Opcode::Div;
        case DFGOp::Div32U:
        case DFGOp::Div64U:
            return Opcode::UDiv;
        case DFGOp::Rem32S:
        case DFGOp::Rem64S:
            return Opcode::Mod;
        case DFGOp::Rem32U:
        case DFGOp::Rem64U:
            return Opcode::UMod;
            
        // Bitwise
        case DFGOp::And32:
        case DFGOp::And64:
            return Opcode::BitAnd;
        case DFGOp::Or32:
        case DFGOp::Or64:
            return Opcode::BitOr;
        case DFGOp::Xor32:
        case DFGOp::Xor64:
            return Opcode::BitXor;
        case DFGOp::Shl32:
        case DFGOp::Shl64:
            return Opcode::Shl;
        case DFGOp::Shr32S:
        case DFGOp::Shr64S:
            return Opcode::SShr;
        case DFGOp::Shr32U:
        case DFGOp::Shr64U:
            return Opcode::ZShr;
            
        // Float
        case DFGOp::AddF32:
        case DFGOp::AddF64:
            return Opcode::Add;
        case DFGOp::SubF32:
        case DFGOp::SubF64:
            return Opcode::Sub;
        case DFGOp::MulF32:
        case DFGOp::MulF64:
            return Opcode::Mul;
        case DFGOp::DivF32:
        case DFGOp::DivF64:
            return Opcode::Div;
        case DFGOp::SqrtF32:
        case DFGOp::SqrtF64:
            return Opcode::Sqrt;
        case DFGOp::AbsF32:
        case DFGOp::AbsF64:
            return Opcode::Abs;
        case DFGOp::NegF32:
        case DFGOp::NegF64:
            return Opcode::Neg;
            
        // Comparisons
        case DFGOp::Eq32:
        case DFGOp::Eq64:
            return Opcode::Equal;
        case DFGOp::Ne32:
        case DFGOp::Ne64:
            return Opcode::NotEqual;
        case DFGOp::Lt32S:
        case DFGOp::Lt64S:
            return Opcode::LessThan;
        case DFGOp::Lt32U:
        case DFGOp::Lt64U:
            return Opcode::Below;
        case DFGOp::Le32S:
        case DFGOp::Le64S:
            return Opcode::LessEqual;
        case DFGOp::Le32U:
        case DFGOp::Le64U:
            return Opcode::BelowEqual;
        case DFGOp::Gt32S:
        case DFGOp::Gt64S:
            return Opcode::GreaterThan;
        case DFGOp::Gt32U:
        case DFGOp::Gt64U:
            return Opcode::Above;
        case DFGOp::Ge32S:
        case DFGOp::Ge64S:
            return Opcode::GreaterEqual;
        case DFGOp::Ge32U:
        case DFGOp::Ge64U:
            return Opcode::AboveEqual;
            
        // Memory
        case DFGOp::Load32:
        case DFGOp::Load64:
        case DFGOp::LoadF32:
        case DFGOp::LoadF64:
            return Opcode::Load;
        case DFGOp::Store32:
        case DFGOp::Store64:
        case DFGOp::StoreF32:
        case DFGOp::StoreF64:
            return Opcode::Store;
            
        // Control
        case DFGOp::Return:
            return Opcode::Return;
        case DFGOp::Jump:
            return Opcode::Jump;
        case DFGOp::Branch:
            return Opcode::Branch;
        case DFGOp::Phi:
            return Opcode::Phi;
        case DFGOp::Unreachable:
            return Opcode::Oops;
            
        // Calls
        case DFGOp::Call:
            return Opcode::CCall;
            
        default:
            return Opcode::Nop;
    }
}

} // namespace Zepra::B3
