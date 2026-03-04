/**
 * @file macro_assembler.cpp
 * @brief x86-64 MacroAssembler implementation
 *
 * Emits raw x86-64 machine code into a buffer.
 * Supports: mov, add, sub, mul, div, cmp, jcc, call, ret,
 * push/pop, SSE2 double-precision, labels, and patching.
 *
 * Ref: Intel® 64 and IA-32 Architectures SDM, Vol 2
 *      firefox/js/src/jit/x64/
 */

#include "jit/MacroAssembler.h"
#include <cstring>
#include <cassert>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace Zepra::JIT {

// =============================================================================
// Encoding helpers
// =============================================================================

void MacroAssembler::encodeREX(bool w, bool r, bool x, bool b) {
    uint8_t rex = 0x40;
    if (w) rex |= 0x08;
    if (r) rex |= 0x04;
    if (x) rex |= 0x02;
    if (b) rex |= 0x01;
    emit8(rex);
}

void MacroAssembler::encodeModRM(uint8_t mod, uint8_t reg, uint8_t rm) {
    emit8((mod << 6) | ((reg & 7) << 3) | (rm & 7));
}

void MacroAssembler::encodeSIB(Scale scale, uint8_t index, uint8_t base) {
    emit8((static_cast<uint8_t>(scale) << 6) | ((index & 7) << 3) | (base & 7));
}

void MacroAssembler::encodeAddress(Register reg, const Address& addr) {
    uint8_t baseId = addr.base.id & 7;
    uint8_t regId = reg.id & 7;

    if (addr.hasIndex()) {
        // SIB byte needed
        if (addr.offset == 0 && baseId != 5) {
            encodeModRM(0x00, regId, 0x04);
            encodeSIB(addr.scale, addr.index.id, addr.base.id);
        } else if (addr.offset >= -128 && addr.offset <= 127) {
            encodeModRM(0x01, regId, 0x04);
            encodeSIB(addr.scale, addr.index.id, addr.base.id);
            emit8(static_cast<uint8_t>(addr.offset));
        } else {
            encodeModRM(0x02, regId, 0x04);
            encodeSIB(addr.scale, addr.index.id, addr.base.id);
            emit32(static_cast<uint32_t>(addr.offset));
        }
    } else if (baseId == 4) {
        // RSP/R12 always need SIB
        if (addr.offset == 0) {
            encodeModRM(0x00, regId, 0x04);
            encodeSIB(Scale::x1, 0x04, baseId);
        } else if (addr.offset >= -128 && addr.offset <= 127) {
            encodeModRM(0x01, regId, 0x04);
            encodeSIB(Scale::x1, 0x04, baseId);
            emit8(static_cast<uint8_t>(addr.offset));
        } else {
            encodeModRM(0x02, regId, 0x04);
            encodeSIB(Scale::x1, 0x04, baseId);
            emit32(static_cast<uint32_t>(addr.offset));
        }
    } else if (addr.offset == 0 && baseId != 5) {
        encodeModRM(0x00, regId, baseId);
    } else if (addr.offset >= -128 && addr.offset <= 127) {
        encodeModRM(0x01, regId, baseId);
        emit8(static_cast<uint8_t>(addr.offset));
    } else {
        encodeModRM(0x02, regId, baseId);
        emit32(static_cast<uint32_t>(addr.offset));
    }
}

void MacroAssembler::emit16(uint16_t word) {
    emit8(word & 0xFF);
    emit8((word >> 8) & 0xFF);
}

void MacroAssembler::emit32(uint32_t dword) {
    emit8(dword & 0xFF);
    emit8((dword >> 8) & 0xFF);
    emit8((dword >> 16) & 0xFF);
    emit8((dword >> 24) & 0xFF);
}

void MacroAssembler::emit64(uint64_t qword) {
    emit32(static_cast<uint32_t>(qword & 0xFFFFFFFF));
    emit32(static_cast<uint32_t>(qword >> 32));
}

void MacroAssembler::patch32(size_t offset, int32_t value) {
    assert(offset + 4 <= code_.size());
    std::memcpy(code_.data() + offset, &value, 4);
}

void MacroAssembler::patch64(size_t offset, int64_t value) {
    assert(offset + 8 <= code_.size());
    std::memcpy(code_.data() + offset, &value, 8);
}

// =============================================================================
// Data movement
// =============================================================================

void MacroAssembler::mov(Register dst, Register src) {
    // REX.W + 89 /r  — MOV r/m64, r64
    encodeREX(true, src.id > 7, false, dst.id > 7);
    emit8(0x89);
    encodeModRM(0x03, src.id, dst.id);
}

void MacroAssembler::mov(Register dst, int64_t imm) {
    if (imm >= 0 && imm <= 0xFFFFFFFF) {
        // 32-bit immediate (zero-extends to 64-bit)
        if (dst.id > 7) {
            encodeREX(false, false, false, true);
        }
        emit8(0xB8 + (dst.id & 7));
        emit32(static_cast<uint32_t>(imm));
    } else {
        // Full 64-bit immediate: REX.W + B8+rd io
        encodeREX(true, false, false, dst.id > 7);
        emit8(0xB8 + (dst.id & 7));
        emit64(static_cast<uint64_t>(imm));
    }
}

void MacroAssembler::mov(Register dst, Address src) {
    // REX.W + 8B /r — MOV r64, r/m64
    encodeREX(true, dst.id > 7, false, src.base.id > 7);
    emit8(0x8B);
    encodeAddress(dst, src);
}

void MacroAssembler::mov(Address dst, Register src) {
    // REX.W + 89 /r — MOV r/m64, r64
    encodeREX(true, src.id > 7, false, dst.base.id > 7);
    emit8(0x89);
    encodeAddress(src, dst);
}

void MacroAssembler::mov(Address dst, int32_t imm) {
    // REX.W + C7 /0 id — MOV r/m64, imm32 (sign-extended)
    encodeREX(true, false, false, dst.base.id > 7);
    emit8(0xC7);
    encodeAddress({0, RegClass::GPR}, dst);
    emit32(static_cast<uint32_t>(imm));
}

void MacroAssembler::movzx8(Register dst, Address src) {
    encodeREX(true, dst.id > 7, false, src.base.id > 7);
    emit8(0x0F);
    emit8(0xB6);
    encodeAddress(dst, src);
}

void MacroAssembler::movzx16(Register dst, Address src) {
    encodeREX(true, dst.id > 7, false, src.base.id > 7);
    emit8(0x0F);
    emit8(0xB7);
    encodeAddress(dst, src);
}

void MacroAssembler::movsx8(Register dst, Address src) {
    encodeREX(true, dst.id > 7, false, src.base.id > 7);
    emit8(0x0F);
    emit8(0xBE);
    encodeAddress(dst, src);
}

void MacroAssembler::movsx16(Register dst, Address src) {
    encodeREX(true, dst.id > 7, false, src.base.id > 7);
    emit8(0x0F);
    emit8(0xBF);
    encodeAddress(dst, src);
}

void MacroAssembler::movsx32(Register dst, Address src) {
    // MOVSXD r64, r/m32 — REX.W + 63 /r
    encodeREX(true, dst.id > 7, false, src.base.id > 7);
    emit8(0x63);
    encodeAddress(dst, src);
}

// =============================================================================
// Arithmetic
// =============================================================================

void MacroAssembler::add(Register dst, Register src) {
    encodeREX(true, src.id > 7, false, dst.id > 7);
    emit8(0x01);
    encodeModRM(0x03, src.id, dst.id);
}

void MacroAssembler::add(Register dst, int32_t imm) {
    encodeREX(true, false, false, dst.id > 7);
    if (imm >= -128 && imm <= 127) {
        emit8(0x83);
        encodeModRM(0x03, 0, dst.id);
        emit8(static_cast<uint8_t>(imm));
    } else {
        if (dst.id == 0) {
            // ADD RAX, imm32
            emit8(0x05);
        } else {
            emit8(0x81);
            encodeModRM(0x03, 0, dst.id);
        }
        emit32(static_cast<uint32_t>(imm));
    }
}

void MacroAssembler::add(Register dst, Address src) {
    encodeREX(true, dst.id > 7, false, src.base.id > 7);
    emit8(0x03);
    encodeAddress(dst, src);
}

void MacroAssembler::sub(Register dst, Register src) {
    encodeREX(true, src.id > 7, false, dst.id > 7);
    emit8(0x29);
    encodeModRM(0x03, src.id, dst.id);
}

void MacroAssembler::sub(Register dst, int32_t imm) {
    encodeREX(true, false, false, dst.id > 7);
    if (imm >= -128 && imm <= 127) {
        emit8(0x83);
        encodeModRM(0x03, 5, dst.id);
        emit8(static_cast<uint8_t>(imm));
    } else {
        if (dst.id == 0) {
            emit8(0x2D);
        } else {
            emit8(0x81);
            encodeModRM(0x03, 5, dst.id);
        }
        emit32(static_cast<uint32_t>(imm));
    }
}

void MacroAssembler::mul(Register dst, Register src) {
    // IMUL r64, r/m64 — REX.W + 0F AF /r
    encodeREX(true, dst.id > 7, false, src.id > 7);
    emit8(0x0F);
    emit8(0xAF);
    encodeModRM(0x03, dst.id, src.id);
}

void MacroAssembler::imul(Register dst, Register src, int32_t imm) {
    encodeREX(true, dst.id > 7, false, src.id > 7);
    if (imm >= -128 && imm <= 127) {
        emit8(0x6B);
        encodeModRM(0x03, dst.id, src.id);
        emit8(static_cast<uint8_t>(imm));
    } else {
        emit8(0x69);
        encodeModRM(0x03, dst.id, src.id);
        emit32(static_cast<uint32_t>(imm));
    }
}

void MacroAssembler::div(Register divisor) {
    // DIV r/m64 — REX.W + F7 /6
    encodeREX(true, false, false, divisor.id > 7);
    emit8(0xF7);
    encodeModRM(0x03, 6, divisor.id);
}

void MacroAssembler::idiv(Register divisor) {
    // IDIV r/m64 — REX.W + F7 /7
    encodeREX(true, false, false, divisor.id > 7);
    emit8(0xF7);
    encodeModRM(0x03, 7, divisor.id);
}

void MacroAssembler::neg(Register reg) {
    encodeREX(true, false, false, reg.id > 7);
    emit8(0xF7);
    encodeModRM(0x03, 3, reg.id);
}

void MacroAssembler::inc(Register reg) {
    encodeREX(true, false, false, reg.id > 7);
    emit8(0xFF);
    encodeModRM(0x03, 0, reg.id);
}

void MacroAssembler::dec(Register reg) {
    encodeREX(true, false, false, reg.id > 7);
    emit8(0xFF);
    encodeModRM(0x03, 1, reg.id);
}

// =============================================================================
// Bitwise
// =============================================================================

void MacroAssembler::and_(Register dst, Register src) {
    encodeREX(true, src.id > 7, false, dst.id > 7);
    emit8(0x21);
    encodeModRM(0x03, src.id, dst.id);
}

void MacroAssembler::and_(Register dst, int32_t imm) {
    encodeREX(true, false, false, dst.id > 7);
    if (imm >= -128 && imm <= 127) {
        emit8(0x83);
        encodeModRM(0x03, 4, dst.id);
        emit8(static_cast<uint8_t>(imm));
    } else {
        emit8(0x81);
        encodeModRM(0x03, 4, dst.id);
        emit32(static_cast<uint32_t>(imm));
    }
}

void MacroAssembler::or_(Register dst, Register src) {
    encodeREX(true, src.id > 7, false, dst.id > 7);
    emit8(0x09);
    encodeModRM(0x03, src.id, dst.id);
}

void MacroAssembler::or_(Register dst, int32_t imm) {
    encodeREX(true, false, false, dst.id > 7);
    if (imm >= -128 && imm <= 127) {
        emit8(0x83);
        encodeModRM(0x03, 1, dst.id);
        emit8(static_cast<uint8_t>(imm));
    } else {
        emit8(0x81);
        encodeModRM(0x03, 1, dst.id);
        emit32(static_cast<uint32_t>(imm));
    }
}

void MacroAssembler::xor_(Register dst, Register src) {
    encodeREX(true, src.id > 7, false, dst.id > 7);
    emit8(0x31);
    encodeModRM(0x03, src.id, dst.id);
}

void MacroAssembler::xor_(Register dst, int32_t imm) {
    encodeREX(true, false, false, dst.id > 7);
    if (imm >= -128 && imm <= 127) {
        emit8(0x83);
        encodeModRM(0x03, 6, dst.id);
        emit8(static_cast<uint8_t>(imm));
    } else {
        emit8(0x81);
        encodeModRM(0x03, 6, dst.id);
        emit32(static_cast<uint32_t>(imm));
    }
}

void MacroAssembler::not_(Register reg) {
    encodeREX(true, false, false, reg.id > 7);
    emit8(0xF7);
    encodeModRM(0x03, 2, reg.id);
}

void MacroAssembler::shl(Register dst, uint8_t count) {
    encodeREX(true, false, false, dst.id > 7);
    if (count == 1) {
        emit8(0xD1);
        encodeModRM(0x03, 4, dst.id);
    } else {
        emit8(0xC1);
        encodeModRM(0x03, 4, dst.id);
        emit8(count);
    }
}

void MacroAssembler::shl(Register dst, Register count) {
    // SHL r/m64, CL — REX.W + D3 /4 (count must be CL)
    (void)count; // CL is implicit
    encodeREX(true, false, false, dst.id > 7);
    emit8(0xD3);
    encodeModRM(0x03, 4, dst.id);
}

void MacroAssembler::shr(Register dst, uint8_t count) {
    encodeREX(true, false, false, dst.id > 7);
    if (count == 1) {
        emit8(0xD1);
        encodeModRM(0x03, 5, dst.id);
    } else {
        emit8(0xC1);
        encodeModRM(0x03, 5, dst.id);
        emit8(count);
    }
}

void MacroAssembler::shr(Register dst, Register count) {
    (void)count;
    encodeREX(true, false, false, dst.id > 7);
    emit8(0xD3);
    encodeModRM(0x03, 5, dst.id);
}

void MacroAssembler::sar(Register dst, uint8_t count) {
    encodeREX(true, false, false, dst.id > 7);
    if (count == 1) {
        emit8(0xD1);
        encodeModRM(0x03, 7, dst.id);
    } else {
        emit8(0xC1);
        encodeModRM(0x03, 7, dst.id);
        emit8(count);
    }
}

void MacroAssembler::sar(Register dst, Register count) {
    (void)count;
    encodeREX(true, false, false, dst.id > 7);
    emit8(0xD3);
    encodeModRM(0x03, 7, dst.id);
}

// =============================================================================
// Comparison
// =============================================================================

void MacroAssembler::cmp(Register lhs, Register rhs) {
    encodeREX(true, rhs.id > 7, false, lhs.id > 7);
    emit8(0x39);
    encodeModRM(0x03, rhs.id, lhs.id);
}

void MacroAssembler::cmp(Register lhs, int32_t imm) {
    encodeREX(true, false, false, lhs.id > 7);
    if (imm >= -128 && imm <= 127) {
        emit8(0x83);
        encodeModRM(0x03, 7, lhs.id);
        emit8(static_cast<uint8_t>(imm));
    } else {
        emit8(0x81);
        encodeModRM(0x03, 7, lhs.id);
        emit32(static_cast<uint32_t>(imm));
    }
}

void MacroAssembler::cmp(Register lhs, Address rhs) {
    encodeREX(true, lhs.id > 7, false, rhs.base.id > 7);
    emit8(0x3B);
    encodeAddress(lhs, rhs);
}

void MacroAssembler::test(Register lhs, Register rhs) {
    encodeREX(true, rhs.id > 7, false, lhs.id > 7);
    emit8(0x85);
    encodeModRM(0x03, rhs.id, lhs.id);
}

void MacroAssembler::test(Register lhs, int32_t imm) {
    encodeREX(true, false, false, lhs.id > 7);
    if (lhs.id == 0) {
        emit8(0xA9);
    } else {
        emit8(0xF7);
        encodeModRM(0x03, 0, lhs.id);
    }
    emit32(static_cast<uint32_t>(imm));
}

// =============================================================================
// Branches
// =============================================================================

static uint8_t conditionCode(Condition cond) {
    switch (cond) {
        case Condition::Equal:        return 0x04;
        case Condition::NotEqual:     return 0x05;
        case Condition::Less:         return 0x0C;
        case Condition::LessEqual:    return 0x0E;
        case Condition::Greater:      return 0x0F;
        case Condition::GreaterEqual: return 0x0D;
        case Condition::Below:        return 0x02;
        case Condition::BelowEqual:   return 0x06;
        case Condition::Above:        return 0x07;
        case Condition::AboveEqual:   return 0x03;
        case Condition::Overflow:     return 0x00;
        case Condition::NoOverflow:   return 0x01;
        case Condition::Zero:         return 0x04;
        case Condition::NonZero:      return 0x05;
    }
    return 0x04;
}

void MacroAssembler::jmp(Label& target) {
    if (target.isBound()) {
        int32_t rel = target.offset - static_cast<int32_t>(code_.size()) - 5;
        emit8(0xE9);
        emit32(static_cast<uint32_t>(rel));
    } else {
        target.pendingJumps.push_back(static_cast<int32_t>(code_.size()));
        emit8(0xE9);
        emit32(0); // Placeholder
    }
}

void MacroAssembler::jmp(Register target) {
    if (target.id > 7) {
        encodeREX(false, false, false, true);
    }
    emit8(0xFF);
    encodeModRM(0x03, 4, target.id);
}

void MacroAssembler::jcc(Condition cond, Label& target) {
    if (target.isBound()) {
        int32_t rel = target.offset - static_cast<int32_t>(code_.size()) - 6;
        emit8(0x0F);
        emit8(0x80 + conditionCode(cond));
        emit32(static_cast<uint32_t>(rel));
    } else {
        target.pendingJumps.push_back(static_cast<int32_t>(code_.size()));
        emit8(0x0F);
        emit8(0x80 + conditionCode(cond));
        emit32(0); // Placeholder
    }
}

void MacroAssembler::call(Label& target) {
    if (target.isBound()) {
        int32_t rel = target.offset - static_cast<int32_t>(code_.size()) - 5;
        emit8(0xE8);
        emit32(static_cast<uint32_t>(rel));
    } else {
        target.pendingJumps.push_back(static_cast<int32_t>(code_.size()));
        emit8(0xE8);
        emit32(0);
    }
}

void MacroAssembler::call(Register target) {
    if (target.id > 7) {
        encodeREX(false, false, false, true);
    }
    emit8(0xFF);
    encodeModRM(0x03, 2, target.id);
}

void MacroAssembler::call(Address target) {
    encodeREX(true, false, false, target.base.id > 7);
    emit8(0xFF);
    encodeAddress({2, RegClass::GPR}, target);
}

void MacroAssembler::ret() {
    emit8(0xC3);
}

// =============================================================================
// Labels
// =============================================================================

void MacroAssembler::bind(Label& label) {
    label.offset = static_cast<int32_t>(code_.size());

    // Patch all pending jumps
    for (int32_t jumpSite : label.pendingJumps) {
        // Jump instructions encode offset after the instruction bytes
        // E9/E8 = 1-byte opcode + 4-byte rel32 → rel is at jumpSite+1
        // 0F 8x = 2-byte opcode + 4-byte rel32 → rel is at jumpSite+2
        uint8_t opcode = code_[jumpSite];
        int32_t relOffset;

        if (opcode == 0x0F) {
            // Conditional jump: 2-byte opcode
            relOffset = label.offset - (jumpSite + 6);
            patch32(jumpSite + 2, relOffset);
        } else {
            // JMP or CALL: 1-byte opcode
            relOffset = label.offset - (jumpSite + 5);
            patch32(jumpSite + 1, relOffset);
        }
    }
    label.pendingJumps.clear();
}

// =============================================================================
// Stack operations
// =============================================================================

void MacroAssembler::push(Register reg) {
    if (reg.id > 7) {
        encodeREX(false, false, false, true);
    }
    emit8(0x50 + (reg.id & 7));
}

void MacroAssembler::push(int32_t imm) {
    if (imm >= -128 && imm <= 127) {
        emit8(0x6A);
        emit8(static_cast<uint8_t>(imm));
    } else {
        emit8(0x68);
        emit32(static_cast<uint32_t>(imm));
    }
}

void MacroAssembler::push(Address src) {
    if (src.base.id > 7) {
        encodeREX(false, false, false, true);
    }
    emit8(0xFF);
    encodeAddress({6, RegClass::GPR}, src);
}

void MacroAssembler::pop(Register reg) {
    if (reg.id > 7) {
        encodeREX(false, false, false, true);
    }
    emit8(0x58 + (reg.id & 7));
}

void MacroAssembler::adjustStackPointer(int32_t delta) {
    if (delta > 0) {
        sub(Reg::stackPointer, delta);
    } else if (delta < 0) {
        add(Reg::stackPointer, -delta);
    }
}

// =============================================================================
// Function prologue/epilogue
// =============================================================================

void MacroAssembler::functionPrologue(size_t frameSize) {
    push(Reg::framePointer);
    mov(Reg::framePointer, Reg::stackPointer);
    if (frameSize > 0) {
        // Align to 16 bytes
        frameSize = (frameSize + 15) & ~15;
        sub(Reg::stackPointer, static_cast<int32_t>(frameSize));
    }
}

void MacroAssembler::functionEpilogue() {
    mov(Reg::stackPointer, Reg::framePointer);
    pop(Reg::framePointer);
    ret();
}

// =============================================================================
// Floating point (SSE2)
// =============================================================================

void MacroAssembler::movsd(Register dst, Register src) {
    // F2 0F 10 /r — MOVSD xmm1, xmm2
    emit8(0xF2);
    if (dst.id > 7 || src.id > 7) {
        encodeREX(false, dst.id > 7, false, src.id > 7);
    }
    emit8(0x0F);
    emit8(0x10);
    encodeModRM(0x03, dst.id, src.id);
}

void MacroAssembler::movsd(Register dst, Address src) {
    emit8(0xF2);
    if (dst.id > 7 || src.base.id > 7) {
        encodeREX(false, dst.id > 7, false, src.base.id > 7);
    }
    emit8(0x0F);
    emit8(0x10);
    encodeAddress(dst, src);
}

void MacroAssembler::movsd(Address dst, Register src) {
    emit8(0xF2);
    if (src.id > 7 || dst.base.id > 7) {
        encodeREX(false, src.id > 7, false, dst.base.id > 7);
    }
    emit8(0x0F);
    emit8(0x11);
    encodeAddress(src, dst);
}

void MacroAssembler::addsd(Register dst, Register src) {
    emit8(0xF2);
    if (dst.id > 7 || src.id > 7) {
        encodeREX(false, dst.id > 7, false, src.id > 7);
    }
    emit8(0x0F);
    emit8(0x58);
    encodeModRM(0x03, dst.id, src.id);
}

void MacroAssembler::subsd(Register dst, Register src) {
    emit8(0xF2);
    if (dst.id > 7 || src.id > 7) {
        encodeREX(false, dst.id > 7, false, src.id > 7);
    }
    emit8(0x0F);
    emit8(0x5C);
    encodeModRM(0x03, dst.id, src.id);
}

void MacroAssembler::mulsd(Register dst, Register src) {
    emit8(0xF2);
    if (dst.id > 7 || src.id > 7) {
        encodeREX(false, dst.id > 7, false, src.id > 7);
    }
    emit8(0x0F);
    emit8(0x59);
    encodeModRM(0x03, dst.id, src.id);
}

void MacroAssembler::divsd(Register dst, Register src) {
    emit8(0xF2);
    if (dst.id > 7 || src.id > 7) {
        encodeREX(false, dst.id > 7, false, src.id > 7);
    }
    emit8(0x0F);
    emit8(0x5E);
    encodeModRM(0x03, dst.id, src.id);
}

void MacroAssembler::cvtsi2sd(Register dst, Register src) {
    // F2 REX.W 0F 2A /r — CVTSI2SD xmm, r64
    emit8(0xF2);
    encodeREX(true, dst.id > 7, false, src.id > 7);
    emit8(0x0F);
    emit8(0x2A);
    encodeModRM(0x03, dst.id, src.id);
}

void MacroAssembler::cvttsd2si(Register dst, Register src) {
    // F2 REX.W 0F 2C /r — CVTTSD2SI r64, xmm
    emit8(0xF2);
    encodeREX(true, dst.id > 7, false, src.id > 7);
    emit8(0x0F);
    emit8(0x2C);
    encodeModRM(0x03, dst.id, src.id);
}

// =============================================================================
// Memory barriers
// =============================================================================

void MacroAssembler::mfence() {
    emit8(0x0F);
    emit8(0xAE);
    emit8(0xF0);
}

void MacroAssembler::lfence() {
    emit8(0x0F);
    emit8(0xAE);
    emit8(0xE8);
}

void MacroAssembler::sfence() {
    emit8(0x0F);
    emit8(0xAE);
    emit8(0xF8);
}

// =============================================================================
// ExecutableBuffer
// =============================================================================

ExecutableBuffer::~ExecutableBuffer() {
    if (code_) {
#ifdef _WIN32
        VirtualFree(code_, 0, MEM_RELEASE);
#else
        munmap(code_, size_);
#endif
    }
}

bool ExecutableBuffer::allocate(size_t size) {
    // Round up to page size
    long pageSize = sysconf(_SC_PAGE_SIZE);
    size_ = (size + pageSize - 1) & ~(pageSize - 1);

#ifdef _WIN32
    code_ = VirtualAlloc(nullptr, size_, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    code_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (code_ == MAP_FAILED) {
        code_ = nullptr;
        return false;
    }
#endif
    return code_ != nullptr;
}

bool ExecutableBuffer::copyFrom(const MacroAssembler& masm) {
    if (!code_ || masm.size() > size_) return false;
    std::memcpy(code_, masm.data(), masm.size());
    return true;
}

void ExecutableBuffer::makeExecutable() {
    if (!code_) return;
#ifdef _WIN32
    DWORD old;
    VirtualProtect(code_, size_, PAGE_EXECUTE_READ, &old);
#else
    mprotect(code_, size_, PROT_READ | PROT_EXEC);
#endif
}

} // namespace Zepra::JIT
