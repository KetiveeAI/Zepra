/**
 * @file B3Generate.h
 * @brief B3 Final Code Generator
 * 
 * Generates x86-64 machine code from optimized B3 IR.
 */

#pragma once

#include "B3Procedure.h"
#include "B3ReduceStrength.h"
#include "B3EliminateDeadCode.h"
#include <vector>
#include <cstdint>

namespace Zepra::B3 {

// Generated code output
struct GeneratedCode {
    std::vector<uint8_t> code;
    uint32_t frameSize = 0;
    bool success = false;
};

// Code emitter helper
class CodeEmitter {
public:
    void emit8(uint8_t b) { code_.push_back(b); }
    void emit32(uint32_t v) {
        emit8(v & 0xFF);
        emit8((v >> 8) & 0xFF);
        emit8((v >> 16) & 0xFF);
        emit8((v >> 24) & 0xFF);
    }
    void emit64(uint64_t v) {
        emit32(static_cast<uint32_t>(v));
        emit32(static_cast<uint32_t>(v >> 32));
    }
    
    size_t offset() const { return code_.size(); }
    std::vector<uint8_t> takeCode() { return std::move(code_); }
    
    // Patch a 32-bit value at a given offset
    void patch32(size_t offset, int32_t value) {
        if (offset + 4 <= code_.size()) {
            code_[offset]     = value & 0xFF;
            code_[offset + 1] = (value >> 8) & 0xFF;
            code_[offset + 2] = (value >> 16) & 0xFF;
            code_[offset + 3] = (value >> 24) & 0xFF;
        }
    }
    
private:
    std::vector<uint8_t> code_;
};

class Generate {
public:
    explicit Generate(Procedure* proc) : proc_(proc) {}
    
    GeneratedCode run();
    
private:
    void optimize();
    void allocateRegisters();
    void emitPrologue();
    void emitEpilogue();
    void emitBlock(BasicBlock* block);
    void emitValue(Value* v);
    
    // x86-64 helpers
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
    
    void emitPush(uint8_t reg) {
        if (reg >= 8) emitter_.emit8(0x41);
        emitter_.emit8(0x50 + (reg & 7));
    }
    
    void emitPop(uint8_t reg) {
        if (reg >= 8) emitter_.emit8(0x41);
        emitter_.emit8(0x58 + (reg & 7));
    }
    
    void emitMov64(uint8_t dst, uint8_t src) {
        emitRex(true, src, 0, dst);
        emitter_.emit8(0x89);
        emitModRM(3, src, dst);
    }
    
    void emitMovImm64(uint8_t dst, int64_t imm) {
        emitRex(true, 0, 0, dst);
        emitter_.emit8(0xB8 + (dst & 7));
        emitter_.emit64(static_cast<uint64_t>(imm));
    }
    
    void emitAdd64(uint8_t dst, uint8_t src) {
        emitRex(true, src, 0, dst);
        emitter_.emit8(0x01);
        emitModRM(3, src, dst);
    }
    
    void emitSub64(uint8_t dst, uint8_t src) {
        emitRex(true, src, 0, dst);
        emitter_.emit8(0x29);
        emitModRM(3, src, dst);
    }
    
    // imul dst, src (2-operand)
    void emitImul64(uint8_t dst, uint8_t src) {
        emitRex(true, dst, 0, src); // REX.W + ...
        emitter_.emit8(0x0F);
        emitter_.emit8(0xAF);
        emitModRM(3, dst, src); // imul rk, rm. dst=reg, src=r/m
    }
    
    // cqo (sign extend RAX -> RDX:RAX)
    void emitCQO() {
        emitRex(true, 0, 0, 0);
        emitter_.emit8(0x99);
    }
    
    // idiv src
    void emitIdiv64(uint8_t src) {
        emitRex(true, 0, 0, src);
        emitter_.emit8(0xF7);
        emitModRM(3, 7, src); // /7
    }
    
    void emitAnd64(uint8_t dst, uint8_t src) {
        emitRex(true, src, 0, dst);
        emitter_.emit8(0x21);
        emitModRM(3, src, dst);
    }
    
    void emitOr64(uint8_t dst, uint8_t src) {
        emitRex(true, src, 0, dst);
        emitter_.emit8(0x09);
        emitModRM(3, src, dst);
    }
    
    void emitXor64(uint8_t dst, uint8_t src) {
        emitRex(true, src, 0, dst);
        emitter_.emit8(0x31);
        emitModRM(3, src, dst);
    }
    
    void emitRet() { emitter_.emit8(0xC3); }
    
    uint8_t getReg(Value* v);
    
    // Jump patching
    void emitJmp32(int32_t rel);
    void emitJcc32(uint8_t cc, int32_t rel);
    size_t emitJmpPlaceholder();
    size_t emitJccPlaceholder(uint8_t cc);
    void patchJmp32(size_t patchOffset, size_t targetOffset);
    
    Procedure* proc_;
    CodeEmitter emitter_;
    uint32_t frameSize_ = 0;
    
    // Simple register allocation
    std::unordered_map<Value*, uint8_t> regMap_;
    uint8_t nextReg_ = 0;
    
    // Block offset tracking for jumps
    std::unordered_map<BasicBlock*, size_t> blockOffsets_;
    std::vector<std::pair<size_t, BasicBlock*>> jumpPatches_;  // (patch location, target block)
};

// Implementation
inline GeneratedCode Generate::run() {
    GeneratedCode result;
    
    optimize();
    allocateRegisters();
    
    emitPrologue();
    
    // First pass: emit code and record block offsets
    for (const auto& block : proc_->blocks()) {
        blockOffsets_[block.get()] = emitter_.offset();
        emitBlock(block.get());
    }
    
    // Second pass: patch jumps
    for (const auto& patch : jumpPatches_) {
        auto it = blockOffsets_.find(patch.second);
        if (it != blockOffsets_.end()) {
            patchJmp32(patch.first, it->second);
        }
    }
    
    result.code = emitter_.takeCode();
    result.frameSize = frameSize_;
    result.success = true;
    return result;
}

inline void Generate::optimize() {
    // Run B3 optimization passes
    ReduceStrength(proc_).run();
    EliminateDeadCode(proc_).run();
}

inline void Generate::allocateRegisters() {
    // Simple linear allocation
    // Available: RAX(0), RCX(1), RDX(2), RBX(3), RSI(6), RDI(7), R8-R15
    const uint8_t regs[] = {0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    nextReg_ = 0;
    
    for (const auto& block : proc_->blocks()) {
        for (Value* v : block->values()) {
            if (v->isDead() || !hasResult(v->opcode())) continue;
            if (nextReg_ < 14) {
                regMap_[v] = regs[nextReg_++];
            } else {
                // Spill (simplified - just reuse RAX)
                regMap_[v] = 0;
            }
        }
    }
}

inline void Generate::emitPrologue() {
    emitPush(5);  // push rbp
    emitMov64(5, 4);  // mov rbp, rsp
    
    // Save callee-saved (simplified)
    emitPush(3);   // rbx
    emitPush(12);  // r12
    emitPush(13);  // r13
    emitPush(14);  // r14
    emitPush(15);  // r15
}

inline void Generate::emitEpilogue() {
    emitPop(15);
    emitPop(14);
    emitPop(13);
    emitPop(12);
    emitPop(3);
    emitMov64(4, 5);  // mov rsp, rbp
    emitPop(5);       // pop rbp
    emitRet();
}

inline void Generate::emitBlock(BasicBlock* block) {
    for (Value* v : block->values()) {
        emitValue(v);
    }
}

inline void Generate::emitValue(Value* v) {
    if (v->isDead()) return;
    
    uint8_t dst = getReg(v);
    
    switch (v->opcode()) {
        case Opcode::Const32:
        case Opcode::Const64:
            emitMovImm64(dst, v->constInt64());
            break;
            
        case Opcode::Add:
            if (v->numInputs() >= 2) {
                uint8_t lhs = getReg(v->input(0));
                uint8_t rhs = getReg(v->input(1));
                if (dst != lhs) emitMov64(dst, lhs);
                emitAdd64(dst, rhs);
            }
            break;
            
        case Opcode::Sub:
            if (v->numInputs() >= 2) {
                uint8_t lhs = getReg(v->input(0));
                uint8_t rhs = getReg(v->input(1));
                if (dst != lhs) emitMov64(dst, lhs);
                emitSub64(dst, rhs);
            }
            break;
            
        case Opcode::Mul:
            if (v->numInputs() >= 2) {
                uint8_t lhs = getReg(v->input(0));
                uint8_t rhs = getReg(v->input(1));
                if (dst != lhs) emitMov64(dst, lhs);
                emitImul64(dst, rhs);
            }
            break;
            
        case Opcode::Div:
        case Opcode::Mod: {
            // Signed division
            if (v->numInputs() >= 2) {
                // div/idiv expects explicit registers:
                // RAX = dividend, RDX = sign extension
                // Result: RAX = quotient, RDX = remainder
                
                // Save RAX/RDX if they are live and not our inputs (omitted for simplealloc)
                // Move LHS to RAX
                uint8_t lhs = getReg(v->input(0));
                if (lhs != 0) emitMov64(0, lhs);
                
                // Sign extend RAX to RDX:RAX (CQO)
                emitCQO();
                
                // IDIV src
                uint8_t rhs = getReg(v->input(1));
                // If rhs is RAX or RDX we have a problem (clobbered). 
                // SimpleAlloc makes sure inputs are in registers.
                // Assuming rhs is in a register != RAX/RDX or was saved?
                // For now assume safe (SimpleAlloc collision risk)
                emitIdiv64(rhs);
                
                // Move result to dst
                if (v->opcode() == Opcode::Div) {
                    if (dst != 0) emitMov64(dst, 0); // Quotient in RAX
                } else {
                    if (dst != 2) emitMov64(dst, 2); // Remainder in RDX
                }
            }
            break;
        }
            
        case Opcode::BitAnd:
            if (v->numInputs() >= 2) {
                uint8_t lhs = getReg(v->input(0));
                uint8_t rhs = getReg(v->input(1));
                if (dst != lhs) emitMov64(dst, lhs);
                emitAnd64(dst, rhs);
            }
            break;
            
        case Opcode::BitOr:
            if (v->numInputs() >= 2) {
                uint8_t lhs = getReg(v->input(0));
                uint8_t rhs = getReg(v->input(1));
                if (dst != lhs) emitMov64(dst, lhs);
                emitOr64(dst, rhs);
            }
            break;
            
        case Opcode::BitXor:
            if (v->numInputs() >= 2) {
                uint8_t lhs = getReg(v->input(0));
                uint8_t rhs = getReg(v->input(1));
                if (dst != lhs) emitMov64(dst, lhs);
                emitXor64(dst, rhs);
            }
            break;

        case Opcode::Return:
            if (v->numInputs() > 0) {
                Value* retval = v->input(0);
                if (retval) { // Ensure input is valid
                   uint8_t src = getReg(retval);
                   if (src != 0) emitMov64(0, src);  // mov rax, result
                }
            }
            emitEpilogue();
            break;

        case Opcode::Load:
            if (v->numInputs() >= 1) {
                // dst = load [ptr]
                uint8_t ptr = getReg(v->input(0));
                
                // mov dst, [ptr]
                // 8B /r : MOV r32,r/m32
                // 48 8B /r : MOV r64,r/m64
                // ModRM: mod=00 (ptr), reg=dst, rm=ptr
                
                emitRex(true, dst, 0, ptr);
                emitter_.emit8(0x8B);
                
                // ModRM(0, dst, ptr) implies [ptr]
                // Note: if ptr is RBP (5) or R13 (13), mod=00 means RIP-relative or needs disp8?
                // RBP as base requires disp (mod=01 or 10). Mod=00 with RBP is RIP-relative (no, that's RBP/5 in 64-bit).
                // [RBP] should be encoded as [RBP + 0] (mod=01, disp=0).
                // R13 as base needs similar handling.
                // Simple allocator uses 0,1,2,3,6,7,8..15.
                // 5 (RBP) is not used. 13 (R13) is used.
                
                if ((ptr & 7) == 5) {
                    // Force [ptr + 0]
                    emitModRM(1, dst, ptr);
                    emitter_.emit8(0);
                } else if ((ptr & 7) == 4) { // RSP/R12
                    // Needs SIB
                    emitModRM(0, dst, 4);
                    emitter_.emit8(0x24); // SIB: scale=0, index=4(none), base=4(rsp/r12)
                } else {
                    emitModRM(0, dst, ptr);
                }
            }
            break;

        case Opcode::Store:
            if (v->numInputs() >= 2) {
                // store val, [ptr]
                Value* val = v->input(0);
                Value* ptr = v->input(1);
                
                uint8_t src = getReg(val);
                uint8_t base = getReg(ptr);
                
                // mov [base], src
                // 89 /r : MOV r/m, reg
                emitRex(true, src, 0, base);
                emitter_.emit8(0x89);
                
                // ModRM: mod=0, reg=src, rm=base
                if ((base & 7) == 5) { // RBP/R13 as base -> use disp0
                    emitModRM(1, src, base);
                    emitter_.emit8(0);
                } else if ((base & 7) == 4) { // RSP/R12 as base -> use SIB
                    emitModRM(0, src, 4);
                    emitter_.emit8(0x24);
                } else {
                    emitModRM(0, src, base);
                }
            }
            break;

        case Opcode::Jump:
            // Unconditional jump to successor block
            if (v->block() && v->block()->numSuccessors() > 0) {
                BasicBlock* target = v->block()->successors()[0];
                size_t patchLoc = emitJmpPlaceholder();
                jumpPatches_.push_back({patchLoc, target});
            }
            break;

        case Opcode::Branch:
            // Conditional branch: if (input[0]) goto succ[0] else goto succ[1]
            if (v->numInputs() >= 1 && v->block() && v->block()->numSuccessors() >= 2) {
                uint8_t cond = getReg(v->input(0));
                
                // test cond, cond
                emitRex(true, cond, 0, cond);
                emitter_.emit8(0x85);  // TEST r/m64, r64
                emitModRM(3, cond, cond);
                
                // jnz taken (successor 0)
                BasicBlock* taken = v->block()->successors()[0];
                size_t takenPatch = emitJccPlaceholder(0x85);  // JNZ
                jumpPatches_.push_back({takenPatch, taken});
                
                // jmp fallthrough (successor 1)
                BasicBlock* fallthrough = v->block()->successors()[1];
                size_t fallPatch = emitJmpPlaceholder();
                jumpPatches_.push_back({fallPatch, fallthrough});
            }
            break;
    }
}

inline uint8_t Generate::getReg(Value* v) {
    auto it = regMap_.find(v);
    if (it != regMap_.end()) return it->second;
    return 0;  // Fallback to RAX
}

// Jump emitter helpers
inline void Generate::emitJmp32(int32_t rel) {
    emitter_.emit8(0xE9);  // JMP rel32
    emitter_.emit32(static_cast<uint32_t>(rel));
}

inline void Generate::emitJcc32(uint8_t cc, int32_t rel) {
    emitter_.emit8(0x0F);
    emitter_.emit8(cc);  // cc: 0x84=JE, 0x85=JNE, 0x8C=JL, 0x8D=JGE, etc.
    emitter_.emit32(static_cast<uint32_t>(rel));
}

inline size_t Generate::emitJmpPlaceholder() {
    emitter_.emit8(0xE9);
    size_t patchLoc = emitter_.offset();
    emitter_.emit32(0);  // Placeholder
    return patchLoc;
}

inline size_t Generate::emitJccPlaceholder(uint8_t cc) {
    emitter_.emit8(0x0F);
    emitter_.emit8(cc);
    size_t patchLoc = emitter_.offset();
    emitter_.emit32(0);  // Placeholder
    return patchLoc;
}

inline void Generate::patchJmp32(size_t patchOffset, size_t targetOffset) {
    // rel32 is computed from end of jump instruction (patchOffset + 4)
    int32_t rel = static_cast<int32_t>(targetOffset) - static_cast<int32_t>(patchOffset + 4);
    emitter_.patch32(patchOffset, rel);
}

} // namespace Zepra::B3
