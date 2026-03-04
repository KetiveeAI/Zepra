/**
 * @file DFGCodeGen.h
 * @brief DFG Code Generator
 * 
 * Generates x86-64 machine code from optimized DFG graph
 * using register allocation results.
 */

#pragma once

#include "jit/dfg/DFGGraph.h"
#include "DFGRegAlloc.h"
#include <vector>
#include <cstdint>

namespace Zepra::DFG {

// Code buffer for emitting bytes
class CodeEmitter {
public:
    void emit8(uint8_t byte) { code_.push_back(byte); }
    void emit16(uint16_t val) {
        emit8(val & 0xFF);
        emit8((val >> 8) & 0xFF);
    }
    void emit32(uint32_t val) {
        emit8(val & 0xFF);
        emit8((val >> 8) & 0xFF);
        emit8((val >> 16) & 0xFF);
        emit8((val >> 24) & 0xFF);
    }
    void emit64(uint64_t val) {
        emit32(static_cast<uint32_t>(val));
        emit32(static_cast<uint32_t>(val >> 32));
    }
    
    size_t offset() const { return code_.size(); }
    const std::vector<uint8_t>& code() const { return code_; }
    std::vector<uint8_t> takeCode() { return std::move(code_); }
    
    void patch32(size_t offset, uint32_t val) {
        code_[offset] = val & 0xFF;
        code_[offset + 1] = (val >> 8) & 0xFF;
        code_[offset + 2] = (val >> 16) & 0xFF;
        code_[offset + 3] = (val >> 24) & 0xFF;
    }
    
private:
    std::vector<uint8_t> code_;
};

// x86-64 register codes
enum class GPR : uint8_t {
    RAX = 0, RCX = 1, RDX = 2, RBX = 3,
    RSP = 4, RBP = 5, RSI = 6, RDI = 7,
    R8 = 8, R9 = 9, R10 = 10, R11 = 11,
    R12 = 12, R13 = 13, R14 = 14, R15 = 15
};

// Generated function result
struct GeneratedCode {
    std::vector<uint8_t> code;
    uint32_t frameSize = 0;
    bool success = false;
};

class CodeGenerator {
public:
    CodeGenerator(Graph* graph, const RegisterAllocation& regAlloc)
        : graph_(graph), regAlloc_(regAlloc) {}
    
    GeneratedCode generate();
    
private:
    void emitPrologue();
    void emitEpilogue();
    void emitBlock(BasicBlock* block);
    void emitValue(Value* v);
    
    // x86-64 instruction helpers
    void emitRexW() { emitter_.emit8(0x48); }
    void emitRex(bool w, uint8_t r, uint8_t x, uint8_t b) {
        uint8_t rex = 0x40;
        if (w) rex |= 0x08;
        if (r >= 8) rex |= 0x04;
        if (x >= 8) rex |= 0x02;
        if (b >= 8) rex |= 0x01;
        if (rex != 0x40) emitter_.emit8(rex);
    }
    
    void emitModRM(uint8_t mod, uint8_t reg, uint8_t rm) {
        emitter_.emit8((mod << 6) | ((reg & 7) << 3) | (rm & 7));
    }
    
    // mov reg, reg (64-bit)
    void emitMov64(uint8_t dst, uint8_t src) {
        emitRex(true, src, 0, dst);
        emitter_.emit8(0x89);
        emitModRM(3, src, dst);
    }
    
    // mov reg, imm64
    void emitMovImm64(uint8_t dst, int64_t imm) {
        emitRex(true, 0, 0, dst);
        emitter_.emit8(0xB8 + (dst & 7));
        emitter_.emit64(static_cast<uint64_t>(imm));
    }
    
    // mov reg, imm32
    void emitMovImm32(uint8_t dst, int32_t imm) {
        if (dst >= 8) emitter_.emit8(0x41);
        emitter_.emit8(0xB8 + (dst & 7));
        emitter_.emit32(static_cast<uint32_t>(imm));
    }
    
    // add dst, src (64-bit)
    void emitAdd64(uint8_t dst, uint8_t src) {
        emitRex(true, src, 0, dst);
        emitter_.emit8(0x01);
        emitModRM(3, src, dst);
    }
    
    // sub dst, src (64-bit)
    void emitSub64(uint8_t dst, uint8_t src) {
        emitRex(true, src, 0, dst);
        emitter_.emit8(0x29);
        emitModRM(3, src, dst);
    }
    
    // push reg
    void emitPush(uint8_t reg) {
        if (reg >= 8) emitter_.emit8(0x41);
        emitter_.emit8(0x50 + (reg & 7));
    }
    
    // pop reg
    void emitPop(uint8_t reg) {
        if (reg >= 8) emitter_.emit8(0x41);
        emitter_.emit8(0x58 + (reg & 7));
    }
    
    // ret
    void emitRet() { emitter_.emit8(0xC3); }
    
    // Load from stack slot
    void emitLoadSpill(uint8_t dst, int32_t slot) {
        int32_t offset = -(static_cast<int32_t>(slot + 1) * 8);
        emitRex(true, dst, 0, static_cast<uint8_t>(GPR::RBP));
        emitter_.emit8(0x8B);  // mov reg, [rbp+disp]
        if (offset >= -128 && offset <= 127) {
            emitModRM(1, dst, static_cast<uint8_t>(GPR::RBP));
            emitter_.emit8(static_cast<uint8_t>(offset));
        } else {
            emitModRM(2, dst, static_cast<uint8_t>(GPR::RBP));
            emitter_.emit32(static_cast<uint32_t>(offset));
        }
    }
    
    // Store to stack slot
    void emitStoreSpill(int32_t slot, uint8_t src) {
        int32_t offset = -(static_cast<int32_t>(slot + 1) * 8);
        emitRex(true, src, 0, static_cast<uint8_t>(GPR::RBP));
        emitter_.emit8(0x89);  // mov [rbp+disp], reg
        if (offset >= -128 && offset <= 127) {
            emitModRM(1, src, static_cast<uint8_t>(GPR::RBP));
            emitter_.emit8(static_cast<uint8_t>(offset));
        } else {
            emitModRM(2, src, static_cast<uint8_t>(GPR::RBP));
            emitter_.emit32(static_cast<uint32_t>(offset));
        }
    }
    
    uint8_t getOperandReg(Value* v);
    
    Graph* graph_;
    const RegisterAllocation& regAlloc_;
    CodeEmitter emitter_;
    uint32_t frameSize_ = 0;
};

// =============================================================================
// Implementation
// =============================================================================

inline GeneratedCode CodeGenerator::generate() {
    GeneratedCode result;
    
    // Calculate frame size
    frameSize_ = regAlloc_.numSpillSlots * 8;
    frameSize_ = (frameSize_ + 15) & ~15;  // Align to 16 bytes
    
    emitPrologue();
    
    for (BasicBlock* block : graph_->reversePostOrder()) {
        emitBlock(block);
    }
    
    result.code = emitter_.takeCode();
    result.frameSize = frameSize_;
    result.success = true;
    return result;
}

inline void CodeGenerator::emitPrologue() {
    // push rbp
    emitPush(static_cast<uint8_t>(GPR::RBP));
    
    // mov rbp, rsp
    emitMov64(static_cast<uint8_t>(GPR::RBP), static_cast<uint8_t>(GPR::RSP));
    
    // sub rsp, frameSize
    if (frameSize_ > 0) {
        emitRexW();
        emitter_.emit8(0x81);
        emitModRM(3, 5, static_cast<uint8_t>(GPR::RSP));
        emitter_.emit32(frameSize_);
    }
    
    // Save callee-saved registers (RBX, R12-R15)
    emitPush(static_cast<uint8_t>(GPR::RBX));
    emitPush(static_cast<uint8_t>(GPR::R12));
    emitPush(static_cast<uint8_t>(GPR::R13));
    emitPush(static_cast<uint8_t>(GPR::R14));
    emitPush(static_cast<uint8_t>(GPR::R15));
}

inline void CodeGenerator::emitEpilogue() {
    // Restore callee-saved registers
    emitPop(static_cast<uint8_t>(GPR::R15));
    emitPop(static_cast<uint8_t>(GPR::R14));
    emitPop(static_cast<uint8_t>(GPR::R13));
    emitPop(static_cast<uint8_t>(GPR::R12));
    emitPop(static_cast<uint8_t>(GPR::RBX));
    
    // mov rsp, rbp
    emitMov64(static_cast<uint8_t>(GPR::RSP), static_cast<uint8_t>(GPR::RBP));
    
    // pop rbp
    emitPop(static_cast<uint8_t>(GPR::RBP));
    
    // ret
    emitRet();
}

inline void CodeGenerator::emitBlock(BasicBlock* block) {
    for (Value* v : block->values()) {
        emitValue(v);
    }
}

inline void CodeGenerator::emitValue(Value* v) {
    if (v->isDead()) return;
    
    int8_t dstReg = regAlloc_.getRegister(v);
    int32_t dstSlot = regAlloc_.getSpillSlot(v);
    
    switch (v->opcode()) {
        case Opcode::Const32: {
            uint8_t dst = (dstReg >= 0) ? static_cast<uint8_t>(dstReg) : 
                          static_cast<uint8_t>(GPR::RAX);
            emitMovImm32(dst, v->asInt32());
            if (dstSlot >= 0) {
                emitStoreSpill(dstSlot, dst);
            }
            break;
        }
        
        case Opcode::Const64: {
            uint8_t dst = (dstReg >= 0) ? static_cast<uint8_t>(dstReg) : 
                          static_cast<uint8_t>(GPR::RAX);
            emitMovImm64(dst, v->asInt64());
            if (dstSlot >= 0) {
                emitStoreSpill(dstSlot, dst);
            }
            break;
        }
        
        case Opcode::Add32:
        case Opcode::Add64: {
            uint8_t lhs = getOperandReg(v->input(0));
            uint8_t rhs = getOperandReg(v->input(1));
            uint8_t dst = (dstReg >= 0) ? static_cast<uint8_t>(dstReg) : lhs;
            
            if (dst != lhs) {
                emitMov64(dst, lhs);
            }
            if (v->opcode() == Opcode::Add64) {
                emitAdd64(dst, rhs);
            } else {
                // 32-bit add
                if (dst >= 8 || rhs >= 8) emitter_.emit8(0x40 | ((dst >= 8) ? 1 : 0) | ((rhs >= 8) ? 4 : 0));
                emitter_.emit8(0x01);
                emitModRM(3, rhs, dst);
            }
            
            if (dstSlot >= 0) {
                emitStoreSpill(dstSlot, dst);
            }
            break;
        }
        
        case Opcode::Sub32:
        case Opcode::Sub64: {
            uint8_t lhs = getOperandReg(v->input(0));
            uint8_t rhs = getOperandReg(v->input(1));
            uint8_t dst = (dstReg >= 0) ? static_cast<uint8_t>(dstReg) : lhs;
            
            if (dst != lhs) {
                emitMov64(dst, lhs);
            }
            if (v->opcode() == Opcode::Sub64) {
                emitSub64(dst, rhs);
            } else {
                if (dst >= 8 || rhs >= 8) emitter_.emit8(0x40 | ((dst >= 8) ? 1 : 0) | ((rhs >= 8) ? 4 : 0));
                emitter_.emit8(0x29);
                emitModRM(3, rhs, dst);
            }
            
            if (dstSlot >= 0) {
                emitStoreSpill(dstSlot, dst);
            }
            break;
        }
        
        case Opcode::Return:
            // Move return value to RAX if needed
            if (v->numInputs() > 0 && v->input(0)) {
                uint8_t src = getOperandReg(v->input(0));
                if (src != static_cast<uint8_t>(GPR::RAX)) {
                    emitMov64(static_cast<uint8_t>(GPR::RAX), src);
                }
            }
            emitEpilogue();
            break;
            
        default:
            // Other opcodes - skeleton for now
            break;
    }
}

inline uint8_t CodeGenerator::getOperandReg(Value* v) {
    int8_t reg = regAlloc_.getRegister(v);
    if (reg >= 0) return static_cast<uint8_t>(reg);
    
    // Value is spilled - load into scratch register
    int32_t slot = regAlloc_.getSpillSlot(v);
    if (slot >= 0) {
        emitLoadSpill(static_cast<uint8_t>(GPR::R11), slot);
        return static_cast<uint8_t>(GPR::R11);
    }
    
    return static_cast<uint8_t>(GPR::RAX);  // Fallback
}

} // namespace Zepra::DFG
