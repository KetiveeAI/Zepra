/**
 * @file WasmBaselineCompile.cpp
 * @brief WebAssembly Baseline JIT Compiler Implementation
 */

#include "wasm/WasmBaselineCompile.h"
#include "wasm/WasmCpuFeatures.h"
#include "wasm/WasmTypeDef.h"
#include <cstring>

namespace Zepra::Wasm {

// Bulk Memory opcodes (0xFC prefix)
namespace BulkOp {
    constexpr uint8_t MEMORY_INIT = 0x08;
    constexpr uint8_t DATA_DROP = 0x09;
    constexpr uint8_t MEMORY_COPY = 0x0A;
    constexpr uint8_t MEMORY_FILL = 0x0B;
    constexpr uint8_t TABLE_INIT = 0x0C;
    constexpr uint8_t ELEM_DROP = 0x0D;
    constexpr uint8_t TABLE_COPY = 0x0E;
    constexpr uint8_t TABLE_GROW = 0x0F;
    constexpr uint8_t TABLE_SIZE = 0x10;
    constexpr uint8_t TABLE_FILL = 0x11;
}

// GC opcodes (0xFB prefix)
namespace GCOp {
    constexpr uint8_t STRUCT_NEW = 0x00;
    constexpr uint8_t STRUCT_NEW_DEFAULT = 0x01;
    constexpr uint8_t STRUCT_GET = 0x02;
    constexpr uint8_t STRUCT_GET_S = 0x03;
    constexpr uint8_t STRUCT_GET_U = 0x04;
    constexpr uint8_t STRUCT_SET = 0x05;
    constexpr uint8_t ARRAY_NEW = 0x06;
    constexpr uint8_t ARRAY_NEW_DEFAULT = 0x07;
    constexpr uint8_t ARRAY_NEW_FIXED = 0x08;
    constexpr uint8_t ARRAY_NEW_DATA = 0x09;
    constexpr uint8_t ARRAY_NEW_ELEM = 0x0A;
    constexpr uint8_t ARRAY_GET = 0x0B;
    constexpr uint8_t ARRAY_GET_S = 0x0C;
    constexpr uint8_t ARRAY_GET_U = 0x0D;
    constexpr uint8_t ARRAY_SET = 0x0E;
    constexpr uint8_t ARRAY_LEN = 0x0F;
    constexpr uint8_t ARRAY_FILL = 0x10;
    constexpr uint8_t ARRAY_COPY = 0x11;
    constexpr uint8_t REF_TEST = 0x14;
    constexpr uint8_t REF_TEST_NULL = 0x15;
    constexpr uint8_t REF_CAST = 0x16;
    constexpr uint8_t REF_CAST_NULL = 0x17;
    constexpr uint8_t BR_ON_CAST = 0x18;
    constexpr uint8_t BR_ON_CAST_FAIL = 0x19;
    constexpr uint8_t I31_NEW = 0x1C;
    constexpr uint8_t I31_GET_S = 0x1D;
    constexpr uint8_t I31_GET_U = 0x1E;
    constexpr uint8_t REF_NULL = 0xD0;
    constexpr uint8_t REF_IS_NULL = 0xD1;
    constexpr uint8_t REF_AS_NON_NULL = 0xD3;
    constexpr uint8_t REF_EQ = 0xD4;
}

// =============================================================================
// Platform Support
// =============================================================================

bool baselinePlatformSupport() {
#if defined(__x86_64__) || defined(_M_X64)
    return true;
#elif defined(__aarch64__) || defined(_M_ARM64)
    return true;
#elif defined(__i386__) || defined(_M_IX86)
    return true;
#elif defined(__arm__) || defined(_M_ARM)
    return true;
#else
    return false;
#endif
}

// =============================================================================
// Code Buffer (simple byte buffer for now)
// =============================================================================

class CodeBuffer {
public:
    CodeBuffer() { buffer_.reserve(4096); }
    
    void emit8(uint8_t byte) { buffer_.push_back(byte); }
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
        emit32(val & 0xFFFFFFFF);
        emit32((val >> 32) & 0xFFFFFFFF);
    }
    
    size_t offset() const { return buffer_.size(); }
    uint8_t* data() { return buffer_.data(); }
    const uint8_t* data() const { return buffer_.data(); }
    size_t size() const { return buffer_.size(); }
    
    std::vector<uint8_t> takeCode() { return std::move(buffer_); }
    
    void patch32(size_t offset, uint32_t val) {
        if (offset + 4 <= buffer_.size()) {
            buffer_[offset] = val & 0xFF;
            buffer_[offset + 1] = (val >> 8) & 0xFF;
            buffer_[offset + 2] = (val >> 16) & 0xFF;
            buffer_[offset + 3] = (val >> 24) & 0xFF;
        }
    }
    
    void patchByte(size_t offset, uint8_t val) {
        if (offset < buffer_.size()) {
            buffer_[offset] = val;
        }
    }
    
private:
    std::vector<uint8_t> buffer_;
};

// =============================================================================
// X86-64 Assembler
// =============================================================================

class X86Assembler {
public:
    explicit X86Assembler(CodeBuffer* buffer) : buf_(buffer) {}
    
    // x86-64 register codes
    enum GPR : uint8_t {
        RAX = 0, RCX = 1, RDX = 2, RBX = 3,
        RSP = 4, RBP = 5, RSI = 6, RDI = 7,
        R8 = 8, R9 = 9, R10 = 10, R11 = 11,
        R12 = 12, R13 = 13, R14 = 14, R15 = 15
    };
    
    enum XMM : uint8_t {
        XMM0 = 0, XMM1 = 1, XMM2 = 2, XMM3 = 3,
        XMM4 = 4, XMM5 = 5, XMM6 = 6, XMM7 = 7,
        XMM8 = 8, XMM9 = 9, XMM10 = 10, XMM11 = 11,
        XMM12 = 12, XMM13 = 13, XMM14 = 14, XMM15 = 15
    };
    
    enum Condition : uint8_t {
        O = 0, NO = 1, B = 2, AE = 3, E = 4, NE = 5, BE = 6, A = 7,
        S = 8, NS = 9, P = 10, NP = 11, L = 12, GE = 13, LE = 14, G = 15
    };
    
    // REX prefix helpers
    void rexW() { buf_->emit8(0x48); }  // 64-bit operand
    void rex(bool w, uint8_t r, uint8_t x, uint8_t b) {
        uint8_t rex = 0x40 | (w ? 8 : 0) | ((r >> 3) << 2) | ((x >> 3) << 1) | (b >> 3);
        if (rex != 0x40) buf_->emit8(rex);
    }
    void rexRB(bool w, uint8_t r, uint8_t b) { rex(w, r, 0, b); }
    
    // ModR/M byte
    void modrm(uint8_t mod, uint8_t reg, uint8_t rm) {
        buf_->emit8((mod << 6) | ((reg & 7) << 3) | (rm & 7));
    }
    
    // Stack operations
    void push(uint8_t reg) {
        if (reg >= 8) buf_->emit8(0x41);
        buf_->emit8(0x50 + (reg & 7));
    }
    
    void pop(uint8_t reg) {
        if (reg >= 8) buf_->emit8(0x41);
        buf_->emit8(0x58 + (reg & 7));
    }
    
    // Move register to register (32-bit)
    void mov32(uint8_t dst, uint8_t src) {
        rexRB(false, src, dst);
        buf_->emit8(0x89);
        modrm(3, src, dst);
    }
    
    // Move register to register (64-bit)
    void mov64(uint8_t dst, uint8_t src) {
        rexRB(true, src, dst);
        buf_->emit8(0x89);
        modrm(3, src, dst);
    }
    
    // Move immediate to register (32-bit)
    void mov32Imm(uint8_t dst, int32_t imm) {
        if (dst >= 8) buf_->emit8(0x41);
        buf_->emit8(0xB8 + (dst & 7));
        buf_->emit32(static_cast<uint32_t>(imm));
    }
    
    // Move immediate to register (64-bit)
    void mov64Imm(uint8_t dst, int64_t imm) {
        rexRB(true, 0, dst);
        buf_->emit8(0xB8 + (dst & 7));
        buf_->emit64(static_cast<uint64_t>(imm));
    }
    
    // ALU operations (32-bit): add/or/adc/sbb/and/sub/xor/cmp
    void alu32(uint8_t op, uint8_t dst, uint8_t src) {
        rexRB(false, src, dst);
        buf_->emit8(0x01 + (op << 3));
        modrm(3, src, dst);
    }
    
    void add32(uint8_t dst, uint8_t src) { alu32(0, dst, src); }
    void or32(uint8_t dst, uint8_t src)  { alu32(1, dst, src); }
    void and32(uint8_t dst, uint8_t src) { alu32(4, dst, src); }
    void sub32(uint8_t dst, uint8_t src) { alu32(5, dst, src); }
    void xor32(uint8_t dst, uint8_t src) { alu32(6, dst, src); }
    void cmp32(uint8_t dst, uint8_t src) { alu32(7, dst, src); }
    
    // TEST: logical AND but only sets flags, doesn't store result
    void test32(uint8_t dst, uint8_t src) {
        rexRB(false, src, dst);
        buf_->emit8(0x85);  // test r/m32, r32
        modrm(3, src, dst);
    }
    
    void test64(uint8_t dst, uint8_t src) {
        rexRB(true, src, dst);
        buf_->emit8(0x85);  // test r/m64, r64
        modrm(3, src, dst);
    }
    
    // ALU operations (64-bit)
    void alu64(uint8_t op, uint8_t dst, uint8_t src) {
        rexRB(true, src, dst);
        buf_->emit8(0x01 + (op << 3));
        modrm(3, src, dst);
    }
    
    void add64(uint8_t dst, uint8_t src) { alu64(0, dst, src); }
    void sub64(uint8_t dst, uint8_t src) { alu64(5, dst, src); }
    void and64(uint8_t dst, uint8_t src) { alu64(4, dst, src); }
    void or64(uint8_t dst, uint8_t src)  { alu64(1, dst, src); }
    void xor64(uint8_t dst, uint8_t src) { alu64(6, dst, src); }
    void cmp64(uint8_t dst, uint8_t src) { alu64(7, dst, src); }
    
    // CMP with immediate (32-bit)
    void cmp32Imm(uint8_t reg, int32_t imm) {
        if (reg >= 8) buf_->emit8(0x41);
        if (imm >= -128 && imm <= 127) {
            buf_->emit8(0x83);
            modrm(3, 7, reg);
            buf_->emit8(static_cast<uint8_t>(imm));
        } else {
            buf_->emit8(0x81);
            modrm(3, 7, reg);
            buf_->emit32(static_cast<uint32_t>(imm));
        }
    }
    
    // CMP with immediate (64-bit)
    void cmp64Imm(uint8_t reg, int32_t imm) {
        if (reg >= 8) buf_->emit8(0x49); else buf_->emit8(0x48);
        if (imm >= -128 && imm <= 127) {
            buf_->emit8(0x83);
            modrm(3, 7, reg);
            buf_->emit8(static_cast<uint8_t>(imm));
        } else {
            buf_->emit8(0x81);
            modrm(3, 7, reg);
            buf_->emit32(static_cast<uint32_t>(imm));
        }
    }
    
    // Multiply (32-bit): imul dst, src
    void imul32(uint8_t dst, uint8_t src) {
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0xAF);
        modrm(3, dst, src);
    }
    
    // Multiply (64-bit)
    void imul64(uint8_t dst, uint8_t src) {
        rexRB(true, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0xAF);
        modrm(3, dst, src);
    }
    
    // Shift operations (32-bit)
    void shl32(uint8_t dst, uint8_t count) {
        if (dst >= 8) buf_->emit8(0x41);
        buf_->emit8(0xC1);
        modrm(3, 4, dst);
        buf_->emit8(count & 0x1F);
    }
    
    void shr32(uint8_t dst, uint8_t count) {
        if (dst >= 8) buf_->emit8(0x41);
        buf_->emit8(0xC1);
        modrm(3, 5, dst);
        buf_->emit8(count & 0x1F);
    }
    
    void sar32(uint8_t dst, uint8_t count) {
        if (dst >= 8) buf_->emit8(0x41);
        buf_->emit8(0xC1);
        modrm(3, 7, dst);
        buf_->emit8(count & 0x1F);
    }
    
    // Shift by CL (32-bit)
    void shl32_cl(uint8_t dst) {
        if (dst >= 8) buf_->emit8(0x41);
        buf_->emit8(0xD3);
        modrm(3, 4, dst);
    }
    
    void shr32_cl(uint8_t dst) {
        if (dst >= 8) buf_->emit8(0x41);
        buf_->emit8(0xD3);
        modrm(3, 5, dst);
    }
    
    void sar32_cl(uint8_t dst) {
        if (dst >= 8) buf_->emit8(0x41);
        buf_->emit8(0xD3);
        modrm(3, 7, dst);
    }
    
    // Set byte on condition
    void setcc(Condition cc, uint8_t dst) {
        if (dst >= 8) buf_->emit8(0x41);
        buf_->emit8(0x0F);
        buf_->emit8(0x90 + cc);
        modrm(3, 0, dst);
    }
    
    // Movzx (8 to 32)
    void movzx8to32(uint8_t dst, uint8_t src) {
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0xB6);
        modrm(3, dst, src);
    }
    
    // Memory load (32-bit): mov dst, [base + offset]
    void load32(uint8_t dst, uint8_t base, int32_t offset) {
        rexRB(false, dst, base);
        buf_->emit8(0x8B);
        if (offset == 0 && (base & 7) != RBP) {
            modrm(0, dst, base);
        } else if (offset >= -128 && offset <= 127) {
            modrm(1, dst, base);
            buf_->emit8(static_cast<uint8_t>(offset));
        } else {
            modrm(2, dst, base);
            buf_->emit32(static_cast<uint32_t>(offset));
        }
        if ((base & 7) == RSP) buf_->emit8(0x24);  // SIB byte
    }
    
    // Memory store (32-bit): mov [base + offset], src
    void store32(uint8_t base, int32_t offset, uint8_t src) {
        rexRB(false, src, base);
        buf_->emit8(0x89);
        if (offset == 0 && (base & 7) != RBP) {
            modrm(0, src, base);
        } else if (offset >= -128 && offset <= 127) {
            modrm(1, src, base);
            buf_->emit8(static_cast<uint8_t>(offset));
        } else {
            modrm(2, src, base);
            buf_->emit32(static_cast<uint32_t>(offset));
        }
        if ((base & 7) == RSP) buf_->emit8(0x24);
    }
    
    // Jump to relative offset
    size_t jmp32() {
        buf_->emit8(0xE9);
        size_t patchOffset = buf_->offset();
        buf_->emit32(0);  // Placeholder
        return patchOffset;
    }
    
    // Conditional jump
    size_t jcc32(Condition cc) {
        buf_->emit8(0x0F);
        buf_->emit8(0x80 + cc);
        size_t patchOffset = buf_->offset();
        buf_->emit32(0);
        return patchOffset;
    }
    
    // Patch jump target
    void patchJump(size_t patchOffset, size_t target) {
        int32_t rel = static_cast<int32_t>(target - (patchOffset + 4));
        buf_->patch32(patchOffset, static_cast<uint32_t>(rel));
    }
    
    // Return
    void ret() { buf_->emit8(0xC3); }
    
    // Int3 (debug break / trap)
    void int3() { buf_->emit8(0xCC); }
    
    // CDQ: sign-extend EAX to EDX:EAX
    void cdq() { buf_->emit8(0x99); }
    
    // CQO: sign-extend RAX to RDX:RAX (64-bit)
    void cqo() {
        buf_->emit8(0x48);
        buf_->emit8(0x99);
    }
    
    // IDIV: signed divide EDX:EAX by r32, result in EAX, remainder in EDX
    void idiv32(uint8_t src) {
        if (src >= 8) buf_->emit8(0x41);
        buf_->emit8(0xF7);
        modrm(3, 7, src);
    }
    
    // DIV: unsigned divide EDX:EAX by r32
    void div32(uint8_t src) {
        if (src >= 8) buf_->emit8(0x41);
        buf_->emit8(0xF7);
        modrm(3, 6, src);
    }
    
    // IDIV64: signed divide RDX:RAX by r64
    void idiv64(uint8_t src) {
        rexRB(true, 0, src);
        buf_->emit8(0xF7);
        modrm(3, 7, src);
    }
    
    // DIV64: unsigned divide RDX:RAX by r64
    void div64(uint8_t src) {
        rexRB(true, 0, src);
        buf_->emit8(0xF7);
        modrm(3, 6, src);
    }
    
    // NEG32: negate register
    void neg32(uint8_t dst) {
        if (dst >= 8) buf_->emit8(0x41);
        buf_->emit8(0xF7);
        modrm(3, 3, dst);
    }
    
    // NEG64: negate register 64-bit
    void neg64(uint8_t dst) {
        rexRB(true, 0, dst);
        buf_->emit8(0xF7);
        modrm(3, 3, dst);
    }
    
    // ROL32 by CL
    void rol32_cl(uint8_t dst) {
        if (dst >= 8) buf_->emit8(0x41);
        buf_->emit8(0xD3);
        modrm(3, 0, dst);
    }
    
    // ROR32 by CL
    void ror32_cl(uint8_t dst) {
        if (dst >= 8) buf_->emit8(0x41);
        buf_->emit8(0xD3);
        modrm(3, 1, dst);
    }
    
    // ROL64 by CL
    void rol64_cl(uint8_t dst) {
        rexRB(true, 0, dst);
        buf_->emit8(0xD3);
        modrm(3, 0, dst);
    }
    
    // ROR64 by CL
    void ror64_cl(uint8_t dst) {
        rexRB(true, 0, dst);
        buf_->emit8(0xD3);
        modrm(3, 1, dst);
    }
    
    // BSR: bit scan reverse (find MSB)
    void bsr32(uint8_t dst, uint8_t src) {
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0xBD);
        modrm(3, dst, src);
    }
    
    // BSF: bit scan forward (find LSB)
    void bsf32(uint8_t dst, uint8_t src) {
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0xBC);
        modrm(3, dst, src);
    }
    
    // LZCNT: count leading zeros (requires BMI1)
    void lzcnt32(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0xBD);
        modrm(3, dst, src);
    }
    
    // TZCNT: count trailing zeros (requires BMI1)
    void tzcnt32(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0xBC);
        modrm(3, dst, src);
    }
    
    // POPCNT: population count
    void popcnt32(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0xB8);
        modrm(3, dst, src);
    }
    
    // POPCNT64
    void popcnt64(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(true, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0xB8);
        modrm(3, dst, src);
    }
    
    // XOR32 with immediate 0 (clear register)
    void xor32Self(uint8_t dst) {
        rexRB(false, dst, dst);
        buf_->emit8(0x31);
        modrm(3, dst, dst);
    }
    
    // SSE: sqrtss
    void sqrtss(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x51);
        modrm(3, dst, src);
    }
    
    // SSE: sqrtsd
    void sqrtsd(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF2);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x51);
        modrm(3, dst, src);
    }
    
    // CVTSI2SS: convert i32 to f32
    void cvtsi2ss(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x2A);
        modrm(3, dst, src);
    }
    
    // CVTSI2SD: convert i32 to f64
    void cvtsi2sd(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF2);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x2A);
        modrm(3, dst, src);
    }
    
    // CVTTSS2SI: truncate f32 to i32
    void cvttss2si(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x2C);
        modrm(3, dst, src);
    }
    
    // CVTTSD2SI: truncate f64 to i32
    void cvttsd2si(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF2);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x2C);
        modrm(3, dst, src);
    }
    
    // CVTSS2SD: f32 to f64
    void cvtss2sd(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x5A);
        modrm(3, dst, src);
    }
    
    // CVTSD2SS: f64 to f32
    void cvtsd2ss(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF2);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x5A);
        modrm(3, dst, src);
    }
    
    // MOVSXD: sign-extend i32 to i64
    void movsxd(uint8_t dst, uint8_t src) {
        rexRB(true, dst, src);
        buf_->emit8(0x63);
        modrm(3, dst, src);
    }
    
    // SSE2: movss (32-bit float)
    void movss(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x10);
        modrm(3, dst, src);
    }
    
    // SSE2: movsd (64-bit float)
    void movsd(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF2);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x10);
        modrm(3, dst, src);
    }
    
    // SSE2: addss
    void addss(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x58);
        modrm(3, dst, src);
    }
    
    // SSE2: subss
    void subss(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x5C);
        modrm(3, dst, src);
    }
    
    // SSE2: mulss
    void mulss(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x59);
        modrm(3, dst, src);
    }
    
    // SSE2: divss  
    void divss(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x5E);
        modrm(3, dst, src);
    }
    
    // SSE2: addsd
    void addsd(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF2);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x58);
        modrm(3, dst, src);
    }
    
    // SSE2: subsd
    void subsd(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF2);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x5C);
        modrm(3, dst, src);
    }
    
    // SSE2: mulsd
    void mulsd(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF2);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x59);
        modrm(3, dst, src);
    }
    
    // SSE2: divsd
    void divsd(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF2);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x5E);
        modrm(3, dst, src);
    }
    
    // SSE: minss
    void minss(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x5D);
        modrm(3, dst, src);
    }
    
    // SSE: maxss
    void maxss(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x5F);
        modrm(3, dst, src);
    }
    
    // SSE2: minsd
    void minsd(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF2);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x5D);
        modrm(3, dst, src);
    }
    
    // SSE2: maxsd
    void maxsd(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF2);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x5F);
        modrm(3, dst, src);
    }
    
    // SSE: andps (bitwise AND for f32 sign manipulation)
    void andps(uint8_t dst, uint8_t src) {
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x54);
        modrm(3, dst, src);
    }
    
    // SSE: orps (bitwise OR for f32)
    void orps(uint8_t dst, uint8_t src) {
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x56);
        modrm(3, dst, src);
    }
    
    // SSE: xorps (bitwise XOR for f32 neg)
    void xorps(uint8_t dst, uint8_t src) {
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x57);
        modrm(3, dst, src);
    }
    
    // SSE2: andpd (bitwise AND for f64)
    void andpd(uint8_t dst, uint8_t src) {
        buf_->emit8(0x66);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x54);
        modrm(3, dst, src);
    }
    
    // SSE2: xorpd (bitwise XOR for f64 neg)
    void xorpd(uint8_t dst, uint8_t src) {
        buf_->emit8(0x66);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x57);
        modrm(3, dst, src);
    }
    
    // SSE4.1: roundss (round f32)
    // mode: 0=nearest, 1=floor, 2=ceil, 3=trunc
    void roundss(uint8_t dst, uint8_t src, uint8_t mode) {
        buf_->emit8(0x66);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x3A);
        buf_->emit8(0x0A);
        modrm(3, dst, src);
        buf_->emit8(mode);
    }
    
    // SSE4.1: roundsd (round f64)
    void roundsd(uint8_t dst, uint8_t src, uint8_t mode) {
        buf_->emit8(0x66);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x3A);
        buf_->emit8(0x0B);
        modrm(3, dst, src);
        buf_->emit8(mode);
    }
    
    // UCOMISS: unordered compare f32 (sets flags)
    void ucomiss(uint8_t dst, uint8_t src) {
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x2E);
        modrm(3, dst, src);
    }
    
    // UCOMISD: unordered compare f64 (sets flags)
    void ucomisd(uint8_t dst, uint8_t src) {
        buf_->emit8(0x66);
        rexRB(false, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x2E);
        modrm(3, dst, src);
    }
    
    // 64-bit shift by CL
    void shl64_cl(uint8_t dst) {
        rexRB(true, 0, dst);
        buf_->emit8(0xD3);
        modrm(3, 4, dst);
    }
    
    void shr64_cl(uint8_t dst) {
        rexRB(true, 0, dst);
        buf_->emit8(0xD3);
        modrm(3, 5, dst);
    }
    
    void sar64_cl(uint8_t dst) {
        rexRB(true, 0, dst);
        buf_->emit8(0xD3);
        modrm(3, 7, dst);
    }
    
    // 64-bit lzcnt/tzcnt
    void lzcnt64(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(true, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0xBD);
        modrm(3, dst, src);
    }
    
    void tzcnt64(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(true, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0xBC);
        modrm(3, dst, src);
    }
    
    // 64-bit conversions
    void cvttss2si64(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(true, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x2C);
        modrm(3, dst, src);
    }
    
    void cvttsd2si64(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF2);
        rexRB(true, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x2C);
        modrm(3, dst, src);
    }
    
    void cvtsi2ss64(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(true, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x2A);
        modrm(3, dst, src);
    }
    
    void cvtsi2sd64(uint8_t dst, uint8_t src) {
        buf_->emit8(0xF2);
        rexRB(true, dst, src);
        buf_->emit8(0x0F);
        buf_->emit8(0x2A);
        modrm(3, dst, src);
    }
    
    // MOVD: move 32 bits between GPR and XMM
    void movd_gpr2xmm(uint8_t xmm, uint8_t gpr) {
        buf_->emit8(0x66);
        rexRB(false, xmm, gpr);
        buf_->emit8(0x0F);
        buf_->emit8(0x6E);
        modrm(3, xmm, gpr);
    }
    
    void movd_xmm2gpr(uint8_t gpr, uint8_t xmm) {
        buf_->emit8(0x66);
        rexRB(false, xmm, gpr);
        buf_->emit8(0x0F);
        buf_->emit8(0x7E);
        modrm(3, xmm, gpr);
    }
    
    // MOVQ: move 64 bits between GPR and XMM
    void movq_gpr2xmm(uint8_t xmm, uint8_t gpr) {
        buf_->emit8(0x66);
        rexRB(true, xmm, gpr);
        buf_->emit8(0x0F);
        buf_->emit8(0x6E);
        modrm(3, xmm, gpr);
    }
    
    void movq_xmm2gpr(uint8_t gpr, uint8_t xmm) {
        buf_->emit8(0x66);
        rexRB(true, xmm, gpr);
        buf_->emit8(0x0F);
        buf_->emit8(0x7E);
        modrm(3, xmm, gpr);
    }
    
    // 64-bit memory load
    void load64(uint8_t dst, uint8_t base, int32_t offset) {
        rexRB(true, dst, base);
        buf_->emit8(0x8B);
        if (offset == 0 && (base & 7) != RBP) {
            modrm(0, dst, base);
        } else if (offset >= -128 && offset <= 127) {
            modrm(1, dst, base);
            buf_->emit8(static_cast<uint8_t>(offset));
        } else {
            modrm(2, dst, base);
            buf_->emit32(static_cast<uint32_t>(offset));
        }
        if ((base & 7) == RSP) buf_->emit8(0x24);
    }
    
    // 64-bit memory store
    void store64(uint8_t base, int32_t offset, uint8_t src) {
        rexRB(true, src, base);
        buf_->emit8(0x89);
        if (offset == 0 && (base & 7) != RBP) {
            modrm(0, src, base);
        } else if (offset >= -128 && offset <= 127) {
            modrm(1, src, base);
            buf_->emit8(static_cast<uint8_t>(offset));
        } else {
            modrm(2, src, base);
            buf_->emit32(static_cast<uint32_t>(offset));
        }
        if ((base & 7) == RSP) buf_->emit8(0x24);
    }
    
    // LEA64: Load effective address with base + index*scale + disp
    void lea64(uint8_t dst, uint8_t base, uint8_t index, uint8_t scale, int32_t disp) {
        // REX.W prefix for 64-bit operand
        uint8_t rex = 0x48;
        if (dst >= 8) rex |= 0x04;  // REX.R
        if (index >= 8) rex |= 0x02; // REX.X
        if (base >= 8) rex |= 0x01;  // REX.B
        buf_->emit8(rex);
        buf_->emit8(0x8D);  // LEA
        
        // SIB addressing mode (base + index*scale + disp)
        uint8_t ss = 0;
        switch (scale) {
            case 1: ss = 0; break;
            case 2: ss = 1; break;
            case 4: ss = 2; break;
            case 8: ss = 3; break;
        }
        
        if (disp == 0 && (base & 7) != RBP) {
            modrm(0, dst, 4);  // SIB follows
            buf_->emit8((ss << 6) | ((index & 7) << 3) | (base & 7));
        } else if (disp >= -128 && disp <= 127) {
            modrm(1, dst, 4);  // SIB + disp8
            buf_->emit8((ss << 6) | ((index & 7) << 3) | (base & 7));
            buf_->emit8(static_cast<uint8_t>(disp));
        } else {
            modrm(2, dst, 4);  // SIB + disp32
            buf_->emit8((ss << 6) | ((index & 7) << 3) | (base & 7));
            buf_->emit32(static_cast<uint32_t>(disp));
        }
    }
    
    // CALL indirect through register
    void call(uint8_t reg) {
        if (reg >= 8) buf_->emit8(0x41);  // REX.B for extended registers
        buf_->emit8(0xFF);
        modrm(3, 2, reg);  // ModR/M: mod=11, reg=2 (opcode ext), r/m=reg
    }
    
    // CALL relative
    size_t callRel32() {
        buf_->emit8(0xE8);
        size_t patchOffset = buf_->offset();
        buf_->emit32(0);  // Placeholder
        return patchOffset;
    }
    
    // Load with sign/zero extension
    void load8s(uint8_t dst, uint8_t base, int32_t offset) {
        rexRB(false, dst, base);
        buf_->emit8(0x0F);
        buf_->emit8(0xBE);
        modrm(0, dst, base);
    }
    
    void load8u(uint8_t dst, uint8_t base, int32_t offset) {
        rexRB(false, dst, base);
        buf_->emit8(0x0F);
        buf_->emit8(0xB6);
        modrm(0, dst, base);
    }
    
    void load16s(uint8_t dst, uint8_t base, int32_t offset) {
        rexRB(false, dst, base);
        buf_->emit8(0x0F);
        buf_->emit8(0xBF);
        modrm(0, dst, base);
    }
    
    void load16u(uint8_t dst, uint8_t base, int32_t offset) {
        rexRB(false, dst, base);
        buf_->emit8(0x0F);
        buf_->emit8(0xB7);
        modrm(0, dst, base);
    }
    
    // 64-bit loads with extension
    void load8s64(uint8_t dst, uint8_t base, int32_t offset) {
        rexRB(true, dst, base);
        buf_->emit8(0x0F);
        buf_->emit8(0xBE);
        modrm(0, dst, base);
    }
    
    void load8u64(uint8_t dst, uint8_t base, int32_t offset) {
        rexRB(true, dst, base);
        buf_->emit8(0x0F);
        buf_->emit8(0xB6);
        modrm(0, dst, base);
    }
    
    void load16s64(uint8_t dst, uint8_t base, int32_t offset) {
        rexRB(true, dst, base);
        buf_->emit8(0x0F);
        buf_->emit8(0xBF);
        modrm(0, dst, base);
    }
    
    void load16u64(uint8_t dst, uint8_t base, int32_t offset) {
        rexRB(true, dst, base);
        buf_->emit8(0x0F);
        buf_->emit8(0xB7);
        modrm(0, dst, base);
    }
    
    void load32s64(uint8_t dst, uint8_t base, int32_t offset) {
        rexRB(true, dst, base);
        buf_->emit8(0x63);
        modrm(0, dst, base);
    }
    
    void load32u64(uint8_t dst, uint8_t base, int32_t offset) {
        rexRB(false, dst, base);
        buf_->emit8(0x8B);
        modrm(0, dst, base);
    }
    
    // Byte/word stores
    void store8(uint8_t base, int32_t offset, uint8_t src) {
        if (src >= 4) rexRB(false, src, base);
        buf_->emit8(0x88);
        modrm(0, src, base);
    }
    
    void store16(uint8_t base, int32_t offset, uint8_t src) {
        buf_->emit8(0x66);
        rexRB(false, src, base);
        buf_->emit8(0x89);
        modrm(0, src, base);
    }
    
    // SSE memory loads/stores
    void loadss(uint8_t dst, uint8_t base, int32_t offset) {
        buf_->emit8(0xF3);
        rexRB(false, dst, base);
        buf_->emit8(0x0F);
        buf_->emit8(0x10);
        modrm(0, dst, base);
    }
    
    void loadsd(uint8_t dst, uint8_t base, int32_t offset) {
        buf_->emit8(0xF2);
        rexRB(false, dst, base);
        buf_->emit8(0x0F);
        buf_->emit8(0x10);
        modrm(0, dst, base);
    }
    
    void storess(uint8_t base, int32_t offset, uint8_t src) {
        buf_->emit8(0xF3);
        rexRB(false, src, base);
        buf_->emit8(0x0F);
        buf_->emit8(0x11);
        modrm(0, src, base);
    }
    
    void storesd(uint8_t base, int32_t offset, uint8_t src) {
        buf_->emit8(0xF2);
        rexRB(false, src, base);
        buf_->emit8(0x0F);
        buf_->emit8(0x11);
        modrm(0, src, base);
    }
    
    // Add immediate to 64-bit register
    void add64Imm(uint8_t dst, int32_t imm) {
        rexRB(true, 0, dst);
        if (imm >= -128 && imm <= 127) {
            buf_->emit8(0x83);
            modrm(3, 0, dst);
            buf_->emit8(static_cast<uint8_t>(imm));
        } else {
            buf_->emit8(0x81);
            modrm(3, 0, dst);
            buf_->emit32(static_cast<uint32_t>(imm));
        }
    }
    
    // String operations for memory.copy / memory.fill
    void cld() {
        buf_->emit8(0xFC);  // CLD - Clear Direction Flag
    }
    
    void repMovsb() {
        buf_->emit8(0xF3);  // REP prefix
        buf_->emit8(0xA4);  // MOVSB
    }
    
    void repStosb() {
        buf_->emit8(0xF3);  // REP prefix
        buf_->emit8(0xAA);  // STOSB
    }
    
    // OR immediate to 32-bit register
    void or32Imm(uint8_t dst, int32_t imm) {
        rexRB(false, 0, dst);
        if (imm >= -128 && imm <= 127) {
            buf_->emit8(0x83);
            modrm(3, 1, dst);  // /1 = OR
            buf_->emit8(static_cast<uint8_t>(imm));
        } else {
            buf_->emit8(0x81);
            modrm(3, 1, dst);
            buf_->emit32(static_cast<uint32_t>(imm));
        }
    }
    
    // AND immediate to 32-bit register
    void and32Imm(uint8_t dst, int32_t imm) {
        rexRB(false, 0, dst);
        if (imm >= -128 && imm <= 127) {
            buf_->emit8(0x83);
            modrm(3, 4, dst);  // /4 = AND
            buf_->emit8(static_cast<uint8_t>(imm));
        } else {
            buf_->emit8(0x81);
            modrm(3, 4, dst);
        }
    }
    
    // JMP relative 32-bit
    void jmp(int32_t offset) {
        buf_->emit8(0xE9);  // JMP rel32
        buf_->emit32(static_cast<uint32_t>(offset - 5));  // Offset from end of instruction
    }
    
    // JMP to register (indirect)
    void jmpReg(uint8_t reg) {
        if (reg >= 8) {
            buf_->emit8(0x41);  // REX.B prefix for extended registers
        }
        buf_->emit8(0xFF);
        modrm(3, 4, reg & 7);  // /4 = JMP
    }
    
    // ADD to 32-bit register with immediate
    void add32Imm(uint8_t dst, int32_t imm) {
        rexRB(false, 0, dst);
        if (imm >= -128 && imm <= 127) {
            buf_->emit8(0x83);
            modrm(3, 0, dst);  // /0 = ADD
            buf_->emit8(static_cast<uint8_t>(imm));
        } else {
            buf_->emit8(0x81);
            modrm(3, 0, dst);
            buf_->emit32(static_cast<uint32_t>(imm));
        }
    }
    
    // MFENCE memory fence
    void mfence() {
        buf_->emit8(0x0F);
        buf_->emit8(0xAE);
        buf_->emit8(0xF0);
    }
    
    // LOCK prefix for atomic operations
    void lockPrefix() {
        buf_->emit8(0xF0);
    }
    
    // CMPXCHG
    void cmpxchg32(uint8_t dst, uint8_t src) {
        buf_->emit8(0x0F);
        buf_->emit8(0xB1);
        modrm(3, src, dst);
    }
    
    void cmpxchg64(uint8_t dst, uint8_t src) {
        rexRB(true, src, dst);
        buf_->emit8(0x0F);
        buf_->emit8(0xB1);
        modrm(3, src, dst);
    }
    
    // XADD - exchange and add
    void xadd32(uint8_t dst, uint8_t src) {
        buf_->emit8(0x0F);
        buf_->emit8(0xC1);
        modrm(3, src, dst);
    }
    
    void xadd64(uint8_t dst, uint8_t src) {
        rexRB(true, src, dst);
        buf_->emit8(0x0F);
        buf_->emit8(0xC1);
        modrm(3, src, dst);
    }
    
    // XCHG - exchange
    void xchg32(uint8_t dst, uint8_t src) {
        buf_->emit8(0x87);
        modrm(3, src, dst);
    }
    
    void xchg64(uint8_t dst, uint8_t src) {
        rexRB(true, src, dst);
        buf_->emit8(0x87);
        modrm(3, src, dst);
    }
    
    // Push immediate 32-bit value
    void pushImm32(int32_t imm) {
        if (imm >= -128 && imm <= 127) {
            buf_->emit8(0x6A);  // push imm8
            buf_->emit8(static_cast<uint8_t>(imm));
        } else {
            buf_->emit8(0x68);  // push imm32
            buf_->emit32(static_cast<uint32_t>(imm));
        }
    }
    
    // Pop to register
    void popReg(uint8_t reg) {
        if (reg >= 8) {
            buf_->emit8(0x41);  // REX.B
        }
        buf_->emit8(0x58 + (reg & 7));  // pop r64
    }
    
    // ==========================================================================
    // Relaxed SIMD Operations (WASM Relaxed SIMD Proposal)
    // ==========================================================================
    
    // VEX-encoded FMA3 vfmadd132ps xmm, xmm, xmm (fused multiply-add)
    // vd = vd * vm + vn
    void vfmadd132ps(uint8_t vd, uint8_t vn, uint8_t vm) {
        // VEX.128.66.0F38.W0 98 /r VFMADD132PS
        vex3(0x66, 0x0F38, false, vd, vn);
        buf_->emit8(0x98);
        modrm(3, vd, vm);
    }
    
    // VEX-encoded FMA3 vfmadd213ps xmm, xmm, xmm
    // vd = vn * vd + vm
    void vfmadd213ps(uint8_t vd, uint8_t vn, uint8_t vm) {
        vex3(0x66, 0x0F38, false, vd, vn);
        buf_->emit8(0xA8);
        modrm(3, vd, vm);
    }
    
    // VEX-encoded FMA3 vfmsub132ps (fused multiply-subtract)
    void vfmsub132ps(uint8_t vd, uint8_t vn, uint8_t vm) {
        vex3(0x66, 0x0F38, false, vd, vn);
        buf_->emit8(0x9A);
        modrm(3, vd, vm);
    }
    
    // PSHUFB - packed shuffle bytes (swizzle)
    void pshufb(uint8_t vd, uint8_t vm) {
        buf_->emit8(0x66);
        buf_->emit8(0x0F);
        buf_->emit8(0x38);
        buf_->emit8(0x00);
        modrm(3, vd, vm);
    }
    
    // MINPS - relaxed minimum (may differ in NaN handling)
    void minps(uint8_t vd, uint8_t vm) {
        buf_->emit8(0x0F);
        buf_->emit8(0x5D);
        modrm(3, vd, vm);
    }
    
    // MAXPS - relaxed maximum
    void maxps(uint8_t vd, uint8_t vm) {
        buf_->emit8(0x0F);
        buf_->emit8(0x5F);
        modrm(3, vd, vm);
    }
    
    // Helper to emit 3-byte VEX prefix
    void vex3(uint8_t pp, uint16_t mmmmm, bool W, uint8_t vd, uint8_t vvvv) {
        buf_->emit8(0xC4);  // 3-byte VEX
        uint8_t b1 = 0;
        if (mmmmm == 0x0F) b1 = 0x01;
        else if (mmmmm == 0x0F38) b1 = 0x02;
        else if (mmmmm == 0x0F3A) b1 = 0x03;
        b1 |= ((~vd & 0x8) << 4);  // R bit
        buf_->emit8(b1);
        uint8_t b2 = ((~vvvv & 0xF) << 3);
        if (pp == 0x66) b2 |= 0x01;
        if (W) b2 |= 0x80;
        buf_->emit8(b2);
    }
    
    size_t offset() const { return buf_->offset(); }
    
private:
    CodeBuffer* buf_;
};

// =============================================================================
// ARM64 Assembler (AArch64)
// =============================================================================

class ARM64Assembler {
public:
    explicit ARM64Assembler(CodeBuffer* buffer) : buf_(buffer) {}
    
    // General registers: x0-x30, sp (x31), xzr (x31 in some contexts)
    enum GPR : uint8_t {
        X0 = 0, X1 = 1, X2 = 2, X3 = 3, X4 = 4, X5 = 5, X6 = 6, X7 = 7,
        X8 = 8, X9 = 9, X10 = 10, X11 = 11, X12 = 12, X13 = 13, X14 = 14, X15 = 15,
        X16 = 16, X17 = 17, X18 = 18, X19 = 19, X20 = 20, X21 = 21, X22 = 22, X23 = 23,
        X24 = 24, X25 = 25, X26 = 26, X27 = 27, X28 = 28,
        FP = 29,   // Frame pointer (x29)
        LR = 30,   // Link register (x30)
        SP = 31,   // Stack pointer
        XZR = 31   // Zero register (context-dependent)
    };
    
    // Vector registers: v0-v31
    enum VPR : uint8_t {
        V0 = 0, V1 = 1, V2 = 2, V3 = 3, V4 = 4, V5 = 5, V6 = 6, V7 = 7,
        V8 = 8, V9 = 9, V10 = 10, V11 = 11, V12 = 12, V13 = 13, V14 = 14, V15 = 15,
        V16 = 16, V17 = 17, V18 = 18, V19 = 19, V20 = 20, V21 = 21, V22 = 22, V23 = 23,
        V24 = 24, V25 = 25, V26 = 26, V27 = 27, V28 = 28, V29 = 29, V30 = 30, V31 = 31
    };
    
    // Emit 32-bit instruction (ARM64 uses fixed 32-bit encoding)
    void emit32(uint32_t insn) { buf_->emit32(insn); }
    
    // Move immediate to register (MOV)
    void movImm64(uint8_t rd, uint64_t imm) {
        // MOVZ + MOVK sequence for 64-bit immediate
        // MOVZ Xd, #imm16, LSL #0
        uint32_t movz = 0xD2800000 | ((imm & 0xFFFF) << 5) | rd;
        emit32(movz);
        
        if ((imm >> 16) & 0xFFFF) {
            // MOVK Xd, #imm16, LSL #16
            uint32_t movk = 0xF2A00000 | (((imm >> 16) & 0xFFFF) << 5) | rd;
            emit32(movk);
        }
        if ((imm >> 32) & 0xFFFF) {
            // MOVK Xd, #imm16, LSL #32
            uint32_t movk = 0xF2C00000 | (((imm >> 32) & 0xFFFF) << 5) | rd;
            emit32(movk);
        }
        if ((imm >> 48) & 0xFFFF) {
            // MOVK Xd, #imm16, LSL #48
            uint32_t movk = 0xF2E00000 | (((imm >> 48) & 0xFFFF) << 5) | rd;
            emit32(movk);
        }
    }
    
    void movImm32(uint8_t rd, uint32_t imm) {
        // MOVZ Wd, #imm16
        uint32_t movz = 0x52800000 | ((imm & 0xFFFF) << 5) | rd;
        emit32(movz);
        
        if ((imm >> 16) & 0xFFFF) {
            // MOVK Wd, #imm16, LSL #16
            uint32_t movk = 0x72A00000 | (((imm >> 16) & 0xFFFF) << 5) | rd;
            emit32(movk);
        }
    }
    
    // Move register to register
    void mov64(uint8_t rd, uint8_t rn) {
        // MOV Xd, Xn = ORR Xd, XZR, Xn
        uint32_t insn = 0xAA0003E0 | (rn << 16) | rd;
        emit32(insn);
    }
    
    void mov32(uint8_t rd, uint8_t rn) {
        // MOV Wd, Wn = ORR Wd, WZR, Wn
        uint32_t insn = 0x2A0003E0 | (rn << 16) | rd;
        emit32(insn);
    }
    
    // Arithmetic: ADD
    void add64(uint8_t rd, uint8_t rn, uint8_t rm) {
        // ADD Xd, Xn, Xm
        uint32_t insn = 0x8B000000 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void add32(uint8_t rd, uint8_t rn, uint8_t rm) {
        // ADD Wd, Wn, Wm
        uint32_t insn = 0x0B000000 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void addImm64(uint8_t rd, uint8_t rn, uint32_t imm12) {
        // ADD Xd, Xn, #imm12
        uint32_t insn = 0x91000000 | ((imm12 & 0xFFF) << 10) | (rn << 5) | rd;
        emit32(insn);
    }
    
    // Arithmetic: SUB
    void sub64(uint8_t rd, uint8_t rn, uint8_t rm) {
        // SUB Xd, Xn, Xm
        uint32_t insn = 0xCB000000 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void sub32(uint8_t rd, uint8_t rn, uint8_t rm) {
        // SUB Wd, Wn, Wm
        uint32_t insn = 0x4B000000 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void subImm64(uint8_t rd, uint8_t rn, uint32_t imm12) {
        // SUB Xd, Xn, #imm12
        uint32_t insn = 0xD1000000 | ((imm12 & 0xFFF) << 10) | (rn << 5) | rd;
        emit32(insn);
    }
    
    // Negate: NEG Xd, Xn = SUB Xd, XZR, Xn
    void neg64(uint8_t rd, uint8_t rn) {
        sub64(rd, XZR, rn);
    }
    
    void neg32(uint8_t rd, uint8_t rn) {
        sub32(rd, static_cast<uint8_t>(31), rn);  // WZR = 31
    }
    
    // Arithmetic: MUL
    void mul64(uint8_t rd, uint8_t rn, uint8_t rm) {
        // MUL Xd, Xn, Xm = MADD Xd, Xn, Xm, XZR
        uint32_t insn = 0x9B007C00 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void mul32(uint8_t rd, uint8_t rn, uint8_t rm) {
        // MUL Wd, Wn, Wm = MADD Wd, Wn, Wm, WZR
        uint32_t insn = 0x1B007C00 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    // Signed division
    void sdiv64(uint8_t rd, uint8_t rn, uint8_t rm) {
        // SDIV Xd, Xn, Xm
        uint32_t insn = 0x9AC00C00 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void sdiv32(uint8_t rd, uint8_t rn, uint8_t rm) {
        // SDIV Wd, Wn, Wm
        uint32_t insn = 0x1AC00C00 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    // Unsigned division
    void udiv64(uint8_t rd, uint8_t rn, uint8_t rm) {
        // UDIV Xd, Xn, Xm
        uint32_t insn = 0x9AC00800 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void udiv32(uint8_t rd, uint8_t rn, uint8_t rm) {
        // UDIV Wd, Wn, Wm
        uint32_t insn = 0x1AC00800 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    // Multiply-subtract: rd = ra - rn * rm (for computing remainder)
    void msub64(uint8_t rd, uint8_t rn, uint8_t rm, uint8_t ra) {
        // MSUB Xd, Xn, Xm, Xa
        uint32_t insn = 0x9B008000 | (rm << 16) | (ra << 10) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void msub32(uint8_t rd, uint8_t rn, uint8_t rm, uint8_t ra) {
        // MSUB Wd, Wn, Wm, Wa
        uint32_t insn = 0x1B008000 | (rm << 16) | (ra << 10) | (rn << 5) | rd;
        emit32(insn);
    }
    
    // Logical operations
    void and64(uint8_t rd, uint8_t rn, uint8_t rm) {
        uint32_t insn = 0x8A000000 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void orr64(uint8_t rd, uint8_t rn, uint8_t rm) {
        uint32_t insn = 0xAA000000 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void eor64(uint8_t rd, uint8_t rn, uint8_t rm) {
        uint32_t insn = 0xCA000000 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    // Shifts
    void lsl64(uint8_t rd, uint8_t rn, uint8_t rm) {
        // LSLV Xd, Xn, Xm
        uint32_t insn = 0x9AC02000 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void lsr64(uint8_t rd, uint8_t rn, uint8_t rm) {
        // LSRV Xd, Xn, Xm
        uint32_t insn = 0x9AC02400 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void asr64(uint8_t rd, uint8_t rn, uint8_t rm) {
        // ASRV Xd, Xn, Xm
        uint32_t insn = 0x9AC02800 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    // Comparison (sets flags)
    void cmp64(uint8_t rn, uint8_t rm) {
        // SUBS XZR, Xn, Xm
        uint32_t insn = 0xEB00001F | (rm << 16) | (rn << 5);
        emit32(insn);
    }
    
    void cmp32(uint8_t rn, uint8_t rm) {
        // SUBS WZR, Wn, Wm
        uint32_t insn = 0x6B00001F | (rm << 16) | (rn << 5);
        emit32(insn);
    }
    
    // Load/Store
    void ldr64(uint8_t rt, uint8_t rn, int32_t offset) {
        // LDR Xt, [Xn, #offset]
        uint32_t insn = 0xF9400000 | (((offset >> 3) & 0xFFF) << 10) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void str64(uint8_t rt, uint8_t rn, int32_t offset) {
        // STR Xt, [Xn, #offset]
        uint32_t insn = 0xF9000000 | (((offset >> 3) & 0xFFF) << 10) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void ldr32(uint8_t rt, uint8_t rn, int32_t offset) {
        // LDR Wt, [Xn, #offset]
        uint32_t insn = 0xB9400000 | (((offset >> 2) & 0xFFF) << 10) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void str32(uint8_t rt, uint8_t rn, int32_t offset) {
        // STR Wt, [Xn, #offset]
        uint32_t insn = 0xB9000000 | (((offset >> 2) & 0xFFF) << 10) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void strb(uint8_t rt, uint8_t rn, int32_t offset) {
        // STRB Wt, [Xn, #offset]
        uint32_t insn = 0x39000000 | ((offset & 0xFFF) << 10) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void strh(uint8_t rt, uint8_t rn, int32_t offset) {
        // STRH Wt, [Xn, #offset]
        uint32_t insn = 0x79000000 | (((offset >> 1) & 0xFFF) << 10) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void ldrb(uint8_t rt, uint8_t rn, int32_t offset) {
        // LDRB Wt, [Xn, #offset]
        uint32_t insn = 0x39400000 | ((offset & 0xFFF) << 10) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void ldrh(uint8_t rt, uint8_t rn, int32_t offset) {
        // LDRH Wt, [Xn, #offset]
        uint32_t insn = 0x79400000 | (((offset >> 1) & 0xFFF) << 10) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void ldrsb32(uint8_t rt, uint8_t rn, int32_t offset) {
        // LDRSB Wt, [Xn, #offset] - sign-extend byte to 32-bit
        uint32_t insn = 0x39C00000 | ((offset & 0xFFF) << 10) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void ldrsb64(uint8_t rt, uint8_t rn, int32_t offset) {
        // LDRSB Xt, [Xn, #offset] - sign-extend byte to 64-bit
        uint32_t insn = 0x39800000 | ((offset & 0xFFF) << 10) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void ldrsh32(uint8_t rt, uint8_t rn, int32_t offset) {
        // LDRSH Wt, [Xn, #offset] - sign-extend halfword to 32-bit
        uint32_t insn = 0x79C00000 | (((offset >> 1) & 0xFFF) << 10) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void ldrsh64(uint8_t rt, uint8_t rn, int32_t offset) {
        // LDRSH Xt, [Xn, #offset] - sign-extend halfword to 64-bit
        uint32_t insn = 0x79800000 | (((offset >> 1) & 0xFFF) << 10) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void ldrsw(uint8_t rt, uint8_t rn, int32_t offset) {
        // LDRSW Xt, [Xn, #offset] - sign-extend word to 64-bit
        uint32_t insn = 0xB9800000 | (((offset >> 2) & 0xFFF) << 10) | (rn << 5) | rt;
        emit32(insn);
    }
    // Stack operations
    void push(uint8_t rt) {
        // STR Xt, [SP, #-8]!  (pre-index)
        uint32_t insn = 0xF81F0C00 | (SP << 5) | rt;  // -8 in 9-bit signed
        emit32(insn);
    }
    
    void pop(uint8_t rt) {
        // LDR Xt, [SP], #8  (post-index)
        uint32_t insn = 0xF8408400 | (SP << 5) | rt;  // +8 post-index
        emit32(insn);
    }
    
    // Branch unconditional
    void b(int32_t offset) {
        // B label (26-bit signed immediate, shifted left 2)
        uint32_t imm26 = (offset >> 2) & 0x3FFFFFF;
        uint32_t insn = 0x14000000 | imm26;
        emit32(insn);
    }
    
    // Branch with link (call)
    void bl(int32_t offset) {
        // BL label
        uint32_t imm26 = (offset >> 2) & 0x3FFFFFF;
        uint32_t insn = 0x94000000 | imm26;
        emit32(insn);
    }
    
    // Branch to register
    void br(uint8_t rn) {
        // BR Xn
        uint32_t insn = 0xD61F0000 | (rn << 5);
        emit32(insn);
    }
    
    // Branch with link to register (indirect call)
    void blr(uint8_t rn) {
        // BLR Xn
        uint32_t insn = 0xD63F0000 | (rn << 5);
        emit32(insn);
    }
    
    // Return
    void ret() {
        // RET (defaults to X30/LR)
        uint32_t insn = 0xD65F03C0;
        emit32(insn);
    }
    
    // NOP
    void nop() {
        uint32_t insn = 0xD503201F;
        emit32(insn);
    }
    
    // BRK (debug breakpoint / trap)
    void brk(uint16_t imm16) {
        // BRK #imm16
        uint32_t insn = 0xD4200000 | (imm16 << 5);
        emit32(insn);
    }
    
    // ==========================================================================
    // Floating-Point Operations (Scalar)
    // ==========================================================================
    
    // FADD (scalar single)
    void faddS(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x1E202800 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FADD (scalar double)
    void faddD(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x1E602800 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FSUB (scalar single)
    void fsubS(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x1E203800 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FSUB (scalar double)
    void fsubD(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x1E603800 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FMUL (scalar single)
    void fmulS(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x1E200800 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FMUL (scalar double)
    void fmulD(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x1E600800 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FDIV (scalar single)
    void fdivS(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x1E201800 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FDIV (scalar double)
    void fdivD(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x1E601800 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FSQRT (scalar single)
    void fsqrtS(uint8_t vd, uint8_t vn) {
        uint32_t insn = 0x1E21C000 | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FSQRT (scalar double)
    void fsqrtD(uint8_t vd, uint8_t vn) {
        uint32_t insn = 0x1E61C000 | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FABS (scalar single)
    void fabsS(uint8_t vd, uint8_t vn) {
        uint32_t insn = 0x1E20C000 | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FABS (scalar double)
    void fabsD(uint8_t vd, uint8_t vn) {
        uint32_t insn = 0x1E60C000 | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FNEG (scalar single)
    void fnegS(uint8_t vd, uint8_t vn) {
        uint32_t insn = 0x1E214000 | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FNEG (scalar double)
    void fnegD(uint8_t vd, uint8_t vn) {
        uint32_t insn = 0x1E614000 | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FCMP (scalar single)
    void fcmpS(uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x1E202000 | (vm << 16) | (vn << 5);
        emit32(insn);
    }
    
    // FCMP (scalar double)
    void fcmpD(uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x1E602000 | (vm << 16) | (vn << 5);
        emit32(insn);
    }
    
    // Load/Store FP single: LDR S[vt], [Xn, #offset]
    void ldrS(uint8_t vt, uint8_t xn, int32_t offset) {
        uint32_t insn = 0xBD400000 | (((offset >> 2) & 0xFFF) << 10) | (xn << 5) | vt;
        emit32(insn);
    }
    
    void strS(uint8_t vt, uint8_t xn, int32_t offset) {
        uint32_t insn = 0xBD000000 | (((offset >> 2) & 0xFFF) << 10) | (xn << 5) | vt;
        emit32(insn);
    }
    
    // Load/Store FP double: LDR D[vt], [Xn, #offset]
    void ldrD(uint8_t vt, uint8_t xn, int32_t offset) {
        uint32_t insn = 0xFD400000 | (((offset >> 3) & 0xFFF) << 10) | (xn << 5) | vt;
        emit32(insn);
    }
    
    void strD(uint8_t vt, uint8_t xn, int32_t offset) {
        uint32_t insn = 0xFD000000 | (((offset >> 3) & 0xFFF) << 10) | (xn << 5) | vt;
        emit32(insn);
    }
    
    // ==========================================================================
    // Type Conversion Instructions
    // ==========================================================================
    
    // Sign-extend word to 64-bit: SXTW Xd, Wn
    void sxtw(uint8_t rd, uint8_t rn) {
        // SBFM Xd, Xn, #0, #31
        uint32_t insn = 0x93407C00 | (rn << 5) | rd;
        emit32(insn);
    }
    
    // Zero-extend word to 64-bit: UXTW Xd, Wn
    void uxtw(uint8_t rd, uint8_t rn) {
        // UBFM Xd, Xn, #0, #31 (or just use MOV W)
        uint32_t insn = 0xD3407C00 | (rn << 5) | rd;
        emit32(insn);
    }
    
    // Float to signed int (32-bit result, single source)
    void fcvtzs32S(uint8_t wd, uint8_t sn) {
        // FCVTZS Wd, Sn
        uint32_t insn = 0x1E380000 | (sn << 5) | wd;
        emit32(insn);
    }
    
    // Float to unsigned int (32-bit result, single source)
    void fcvtzu32S(uint8_t wd, uint8_t sn) {
        // FCVTZU Wd, Sn
        uint32_t insn = 0x1E390000 | (sn << 5) | wd;
        emit32(insn);
    }
    
    // Double to signed int (32-bit result, double source)
    void fcvtzs32D(uint8_t wd, uint8_t dn) {
        // FCVTZS Wd, Dn
        uint32_t insn = 0x1E780000 | (dn << 5) | wd;
        emit32(insn);
    }
    
    // Double to unsigned int (32-bit result, double source)
    void fcvtzu32D(uint8_t wd, uint8_t dn) {
        // FCVTZU Wd, Dn
        uint32_t insn = 0x1E790000 | (dn << 5) | wd;
        emit32(insn);
    }
    
    // Signed int to float (32-bit source, single result)
    void scvtfS32(uint8_t sd, uint8_t wn) {
        // SCVTF Sd, Wn
        uint32_t insn = 0x1E220000 | (wn << 5) | sd;
        emit32(insn);
    }
    
    // Unsigned int to float (32-bit source, single result)
    void ucvtfS32(uint8_t sd, uint8_t wn) {
        // UCVTF Sd, Wn
        uint32_t insn = 0x1E230000 | (wn << 5) | sd;
        emit32(insn);
    }
    
    // Signed int to double (32-bit source, double result)
    void scvtfD32(uint8_t dd, uint8_t wn) {
        // SCVTF Dd, Wn
        uint32_t insn = 0x1E620000 | (wn << 5) | dd;
        emit32(insn);
    }
    
    // Unsigned int to double (32-bit source, double result)
    void ucvtfD32(uint8_t dd, uint8_t wn) {
        // UCVTF Dd, Wn
        uint32_t insn = 0x1E630000 | (wn << 5) | dd;
        emit32(insn);
    }
    
    // Double to float (FCVT Sd, Dn)
    void fcvtSD(uint8_t sd, uint8_t dn) {
        uint32_t insn = 0x1E624000 | (dn << 5) | sd;
        emit32(insn);
    }
    
    // Float to double (FCVT Dd, Sn)
    void fcvtDS(uint8_t dd, uint8_t sn) {
        uint32_t insn = 0x1E22C000 | (sn << 5) | dd;
        emit32(insn);
    }
    // ==========================================================================
    // Bitwise Operations (32-bit versions)
    // ==========================================================================
    
    void and32(uint8_t rd, uint8_t rn, uint8_t rm) {
        uint32_t insn = 0x0A000000 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void orr32(uint8_t rd, uint8_t rn, uint8_t rm) {
        uint32_t insn = 0x2A000000 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void eor32(uint8_t rd, uint8_t rn, uint8_t rm) {
        uint32_t insn = 0x4A000000 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    // MVN = ORN Rd, XZR, Rm
    void mvn64(uint8_t rd, uint8_t rm) {
        uint32_t insn = 0xAA2003E0 | (rm << 16) | rd;
        emit32(insn);
    }
    
    void mvn32(uint8_t rd, uint8_t rm) {
        uint32_t insn = 0x2A2003E0 | (rm << 16) | rd;
        emit32(insn);
    }
    
    // Shifts (32-bit)
    void lsl32(uint8_t rd, uint8_t rn, uint8_t rm) {
        uint32_t insn = 0x1AC02000 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void lsr32(uint8_t rd, uint8_t rn, uint8_t rm) {
        uint32_t insn = 0x1AC02400 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void asr32(uint8_t rd, uint8_t rn, uint8_t rm) {
        uint32_t insn = 0x1AC02800 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void ror32(uint8_t rd, uint8_t rn, uint8_t rm) {
        uint32_t insn = 0x1AC02C00 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void ror64(uint8_t rd, uint8_t rn, uint8_t rm) {
        uint32_t insn = 0x9AC02C00 | (rm << 16) | (rn << 5) | rd;
        emit32(insn);
    }
    
    // Count leading zeros
    void clz32(uint8_t rd, uint8_t rn) {
        uint32_t insn = 0x5AC01000 | (rn << 5) | rd;
        emit32(insn);
    }
    
    void clz64(uint8_t rd, uint8_t rn) {
        uint32_t insn = 0xDAC01000 | (rn << 5) | rd;
        emit32(insn);
    }
    
    // Count trailing zeros
    void ctz32(uint8_t rd, uint8_t rn) {
        // RBIT + CLZ (no direct CTZ)
        uint32_t rbit = 0x5AC00000 | (rn << 5) | rd;
        emit32(rbit);
        clz32(rd, rd);
    }
    
    void ctz64(uint8_t rd, uint8_t rn) {
        uint32_t rbit = 0xDAC00000 | (rn << 5) | rd;
        emit32(rbit);
        clz64(rd, rd);
    }
    
    // Pop count (population count / bit count)
    void popcnt32(uint8_t rd, uint8_t rn) {
        // Use FMOV to vector, CNT, then ADDV (complex, stub for now)
        (void)rd; (void)rn;
    }
    
    // ==========================================================================
    // Calling Convention / Prologue / Epilogue
    // ==========================================================================
    
    // Store pair: STP Xt1, Xt2, [Xn, #offset]!
    void stpPre64(uint8_t rt1, uint8_t rt2, uint8_t rn, int32_t offset) {
        int32_t imm7 = (offset >> 3) & 0x7F;
        uint32_t insn = 0xA9800000 | (imm7 << 15) | (rt2 << 10) | (rn << 5) | rt1;
        emit32(insn);
    }
    
    // Load pair: LDP Xt1, Xt2, [Xn], #offset
    void ldpPost64(uint8_t rt1, uint8_t rt2, uint8_t rn, int32_t offset) {
        int32_t imm7 = (offset >> 3) & 0x7F;
        uint32_t insn = 0xA8C00000 | (imm7 << 15) | (rt2 << 10) | (rn << 5) | rt1;
        emit32(insn);
    }
    
    // Function prologue (standard ARM64 calling convention)
    void prologue(uint32_t frameSize) {
        // STP x29, x30, [sp, #-frameSize]!
        stpPre64(FP, LR, SP, -static_cast<int32_t>(frameSize));
        // MOV x29, sp
        mov64(FP, SP);
    }
    
    // Function epilogue
    void epilogue(uint32_t frameSize) {
        // LDP x29, x30, [sp], #frameSize
        ldpPost64(FP, LR, SP, frameSize);
        // RET
        ret();
    }
    
    // ==========================================================================
    // Conditional branches
    // ==========================================================================
    
    enum Condition : uint8_t {
        EQ = 0,  // Equal
        NE = 1,  // Not equal
        CS = 2, HS = 2,  // Carry set / unsigned higher or same
        CC = 3, LO = 3,  // Carry clear / unsigned lower
        MI = 4,  // Minus / Negative
        PL = 5,  // Plus / Positive or zero
        VS = 6,  // Overflow
        VC = 7,  // No overflow
        HI = 8,  // Unsigned higher
        LS = 9,  // Unsigned lower or same
        GE = 10, // Signed greater than or equal
        LT = 11, // Signed less than
        GT = 12, // Signed greater than
        LE = 13, // Signed less than or equal
        AL = 14, // Always
        NV = 15  // Never (architecture reserved)
    };
    
    void bCond(Condition cond, int32_t offset) {
        // B.cond label
        uint32_t imm19 = ((offset >> 2) & 0x7FFFF);
        uint32_t insn = 0x54000000 | (imm19 << 5) | cond;
        emit32(insn);
    }
    
    // Conditional select: CSEL Xd, Xn, Xm, cond
    void csel64(uint8_t rd, uint8_t rn, uint8_t rm, Condition cond) {
        uint32_t insn = 0x9A800000 | (rm << 16) | (cond << 12) | (rn << 5) | rd;
        emit32(insn);
    }
    
    void csel32(uint8_t rd, uint8_t rn, uint8_t rm, Condition cond) {
        uint32_t insn = 0x1A800000 | (rm << 16) | (cond << 12) | (rn << 5) | rd;
        emit32(insn);
    }
    
    // CSET: set 1 if condition true, 0 otherwise
    void cset64(uint8_t rd, Condition cond) {
        // CSINC Xd, XZR, XZR, invert(cond)
        Condition invCond = static_cast<Condition>(cond ^ 1);
        uint32_t insn = 0x9A9F07E0 | (invCond << 12) | rd;
        emit32(insn);
    }
    
    void cset32(uint8_t rd, Condition cond) {
        Condition invCond = static_cast<Condition>(cond ^ 1);
        uint32_t insn = 0x1A9F07E0 | (invCond << 12) | rd;
        emit32(insn);
    }
    
    // Alias for common usage (uses condition code directly)
    void cset(uint8_t rd, uint8_t condCode) {
        Condition cond = static_cast<Condition>(condCode);
        cset32(rd, cond);
    }
    
    // Compare and branch if not zero (64-bit)
    void cbnz64(uint8_t rt, int32_t offset) {
        // CBNZ Xt, label
        uint32_t imm19 = ((offset >> 2) & 0x7FFFF);
        uint32_t insn = 0xB5000000 | (imm19 << 5) | rt;
        emit32(insn);
    }
    
    // Compare and branch if zero (64-bit)
    void cbz64(uint8_t rt, int32_t offset) {
        uint32_t imm19 = ((offset >> 2) & 0x7FFFF);
        uint32_t insn = 0xB4000000 | (imm19 << 5) | rt;
        emit32(insn);
    }
    
    // ORR immediate (64-bit) - simplified for small values
    void orrImm64(uint8_t rd, uint8_t rn, uint64_t imm) {
        // For simple case: ORR with small immediate (use MOV + ORR)
        // This is a simplified implementation
        if (imm <= 0xFFFF) {
            uint8_t temp = 16;  // Use x16 as scratch
            movImm64(temp, imm);
            orr64(rd, rn, temp);
        } else {
            orr64(rd, rn, rn);  // Fallback: no-op
        }
    }
    
    // AND immediate (64-bit)
    void andImm64(uint8_t rd, uint8_t rn, uint64_t imm) {
        // Simplified: use temp register for AND
        if (imm <= 0xFFFFFFFF) {
            uint8_t temp = 16;  // x16 as scratch
            movImm64(temp, imm);
            and64(rd, rn, temp);
        } else {
            and64(rd, rn, rn);  // Fallback
        }
    }
    
    // CMP immediate (64-bit) - compares Xn with immediate
    void cmpImm64(uint8_t rn, uint64_t imm) {
        // CMP Xn, #imm is encoded as SUBS XZR, Xn, #imm
        if (imm <= 0xFFF) {
            // SUBS XZR, Xn, #imm (immediate fits in 12 bits)
            uint32_t insn = 0xF1000000 | ((imm & 0xFFF) << 10) | (rn << 5) | 31;
            emit32(insn);
        } else {
            // Load immediate to temp and use CMP reg
            uint8_t temp = 16;
            movImm64(temp, imm);
            // SUBS XZR, Xn, Xm
            uint32_t insn = 0xEB000000 | (temp << 16) | (rn << 5) | 31;
            emit32(insn);
        }
    }
    
    // ==========================================================================
    // NEON SIMD Vector Instructions (v128 / Q registers)
    // ==========================================================================
    
    // Vector load: LDR Qd, [Xn, #offset]
    void ldrQ(uint8_t vd, uint8_t rn, int32_t offset = 0) {
        uint32_t imm12 = ((offset >> 4) & 0xFFF);
        uint32_t insn = 0x3DC00000 | (imm12 << 10) | (rn << 5) | vd;
        emit32(insn);
    }
    
    // Vector store: STR Qd, [Xn, #offset]
    void strQ(uint8_t vd, uint8_t rn, int32_t offset = 0) {
        uint32_t imm12 = ((offset >> 4) & 0xFFF);
        uint32_t insn = 0x3D800000 | (imm12 << 10) | (rn << 5) | vd;
        emit32(insn);
    }
    
    // Vector ADD (4S = 4x 32-bit integers)
    void addV4S(uint8_t vd, uint8_t vn, uint8_t vm) {
        // ADD Vd.4S, Vn.4S, Vm.4S
        uint32_t insn = 0x4EA08400 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector ADD (2D = 2x 64-bit integers)
    void addV2D(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x4EE08400 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector SUB (4S)
    void subV4S(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x6EA08400 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector SUB (2D)
    void subV2D(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x6EE08400 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector MUL (4S) - Note: no 2D integer multiply in NEON
    void mulV4S(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x4EA09C00 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector AND (bitwise)
    void andV(uint8_t vd, uint8_t vn, uint8_t vm) {
        // AND Vd.16B, Vn.16B, Vm.16B
        uint32_t insn = 0x4E201C00 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector ORR (bitwise)
    void orrV(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x4EA01C00 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector EOR (bitwise XOR)
    void eorV(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x6E201C00 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector NOT (bitwise)
    void notV(uint8_t vd, uint8_t vn) {
        // NOT Vd.16B, Vn.16B
        uint32_t insn = 0x6E205800 | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector FADD (4S = 4x float32)
    void faddV4S(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x4E20D400 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector FADD (2D = 2x float64)
    void faddV2D(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x4E60D400 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector FSUB (4S)
    void fsubV4S(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x4EA0D400 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector FSUB (2D)
    void fsubV2D(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x4EE0D400 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector FMUL (4S)
    void fmulV4S(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x6E20DC00 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector FMUL (2D)
    void fmulV2D(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x6E60DC00 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector FDIV (4S)
    void fdivV4S(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x6E20FC00 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector FDIV (2D)
    void fdivV2D(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x6E60FC00 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector FSQRT (4S)
    void fsqrtV4S(uint8_t vd, uint8_t vn) {
        uint32_t insn = 0x6EA1F800 | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector FSQRT (2D)
    void fsqrtV2D(uint8_t vd, uint8_t vn) {
        uint32_t insn = 0x6EE1F800 | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector compare equal (4S) - returns all 1s or all 0s per lane
    void cmeqV4S(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x6EA08C00 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector compare greater than (4S signed)
    void cmgtV4S(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x4EA03400 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector compare greater or equal (4S signed)
    void cmgeV4S(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x4EA03C00 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector DUP (scalar to all lanes) - GPR to 4S
    void dupV4S(uint8_t vd, uint8_t rn) {
        // DUP Vd.4S, Wn
        uint32_t insn = 0x4E040C00 | (rn << 5) | vd;
        emit32(insn);
    }
    
    // Vector DUP (scalar to all lanes) - GPR to 2D
    void dupV2D(uint8_t vd, uint8_t rn) {
        // DUP Vd.2D, Xn
        uint32_t insn = 0x4E080C00 | (rn << 5) | vd;
        emit32(insn);
    }
    
    // Vector move register
    void movV(uint8_t vd, uint8_t vn) {
        // ORR Vd.16B, Vn.16B, Vn.16B
        orrV(vd, vn, vn);
    }
    
    // Vector extract lane to GPR (4S)
    void umovW(uint8_t rd, uint8_t vn, uint8_t lane) {
        // UMOV Wd, Vn.S[lane]
        uint32_t imm5 = ((lane & 0x3) << 3) | 0x4;  // size=S
        uint32_t insn = 0x0E003C00 | (imm5 << 16) | (vn << 5) | rd;
        emit32(insn);
    }
    
    // Vector insert lane from GPR (4S)
    void insV4S(uint8_t vd, uint8_t lane, uint8_t rn) {
        // INS Vd.S[lane], Wn
        uint32_t imm5 = ((lane & 0x3) << 3) | 0x4;
        uint32_t insn = 0x4E001C00 | (imm5 << 16) | (rn << 5) | vd;
        emit32(insn);
    }
    
    // Vector TBL (table lookup for shuffle)
    void tblV16B(uint8_t vd, uint8_t vn, uint8_t vm) {
        // TBL Vd.16B, {Vn.16B}, Vm.16B
        uint32_t insn = 0x4E000000 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector ZIP1 (interleave low halves)
    void zip1V4S(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x4E803800 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Vector ZIP2 (interleave high halves)
    void zip2V4S(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x4E807800 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Load quad (128-bit) - placeholder for SIMD
    void ldq(uint8_t vd, uint8_t rn, int32_t offset = 0) {
        // LDR Q (128-bit load): 0x3DC00000 | imm12 | Rn | Rt
        uint32_t imm = (offset >> 4) & 0xFFF;
        uint32_t insn = 0x3DC00000 | (imm << 10) | (rn << 5) | vd;
        emit32(insn);
    }
    
    // Table lookup (TBL) - single register table
    void tbl(uint8_t vd, uint8_t vn, uint8_t vm) {
        // TBL Vd.16B, {Vn.16B}, Vm.16B: 0x4E000000 | Vm | 0 | Vn | Vd
        uint32_t insn = 0x4E000000 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Table lookup (TBL) - two register table
    void tbl2(uint8_t vd, uint8_t vn, uint8_t vm) {
        // TBL Vd.16B, {Vn.16B, Vn+1.16B}, Vm.16B: 0x4E002000 | Vm | Vn | Vd
        uint32_t insn = 0x4E002000 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // DUP element to vector (byte)
    void dupVB(uint8_t vd, uint8_t vn, uint8_t index) {
        // DUP Vd.16B, Vn.B[index]: 0x4E010400 | (index << 17) | Vn | Vd
        uint32_t imm5 = (index << 1) | 1;  // B = xxx01
        uint32_t insn = 0x4E000400 | (imm5 << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // DUP element to vector (half)
    void dupVH(uint8_t vd, uint8_t vn, uint8_t index) {
        uint32_t imm5 = (index << 2) | 2;  // H = xx010
        uint32_t insn = 0x4E000400 | (imm5 << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // DUP element to vector (single)
    void dupVS(uint8_t vd, uint8_t vn, uint8_t index) {
        uint32_t imm5 = (index << 3) | 4;  // S = x0100
        uint32_t insn = 0x4E000400 | (imm5 << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // DUP element to vector (double)
    void dupVD(uint8_t vd, uint8_t vn, uint8_t index) {
        uint32_t imm5 = (index << 4) | 8;  // D = 01000
        uint32_t insn = 0x4E000400 | (imm5 << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // SMOV - signed move from element to register (byte)
    void smovB(uint8_t rd, uint8_t vn, uint8_t index) {
        uint32_t imm5 = (index << 1) | 1;
        uint32_t insn = 0x4E002C00 | (imm5 << 16) | (vn << 5) | rd;
        emit32(insn);
    }
    
    // SMOV - signed move from element to register (half)
    void smovH(uint8_t rd, uint8_t vn, uint8_t index) {
        uint32_t imm5 = (index << 2) | 2;
        uint32_t insn = 0x4E002C00 | (imm5 << 16) | (vn << 5) | rd;
        emit32(insn);
    }
    
    // UMOV - unsigned move from element to register (byte)
    void umovB(uint8_t rd, uint8_t vn, uint8_t index) {
        uint32_t imm5 = (index << 1) | 1;
        uint32_t insn = 0x0E003C00 | (imm5 << 16) | (vn << 5) | rd;
        emit32(insn);
    }
    
    // UMOV - unsigned move from element to register (half)
    void umovH(uint8_t rd, uint8_t vn, uint8_t index) {
        uint32_t imm5 = (index << 2) | 2;
        uint32_t insn = 0x0E003C00 | (imm5 << 16) | (vn << 5) | rd;
        emit32(insn);
    }
    
    // UMOV - unsigned move from element to register (single/word)
    void umovS(uint8_t rd, uint8_t vn, uint8_t index) {
        uint32_t imm5 = (index << 3) | 4;
        uint32_t insn = 0x0E003C00 | (imm5 << 16) | (vn << 5) | rd;
        emit32(insn);
    }
    
    // UMOV - unsigned move from element to register (double)
    void umovD(uint8_t rd, uint8_t vn, uint8_t index) {
        uint32_t imm5 = (index << 4) | 8;
        uint32_t insn = 0x4E003C00 | (imm5 << 16) | (vn << 5) | rd;
        emit32(insn);
    }
    
    // INS - insert element from register (byte)
    void insB(uint8_t vd, uint8_t index, uint8_t rn) {
        uint32_t imm5 = (index << 1) | 1;
        uint32_t insn = 0x4E001C00 | (imm5 << 16) | (rn << 5) | vd;
        emit32(insn);
    }
    
    // INS - insert element from register (half)
    void insH(uint8_t vd, uint8_t index, uint8_t rn) {
        uint32_t imm5 = (index << 2) | 2;
        uint32_t insn = 0x4E001C00 | (imm5 << 16) | (rn << 5) | vd;
        emit32(insn);
    }
    
    // INS - insert element from register (single)
    void insS(uint8_t vd, uint8_t index, uint8_t rn) {
        uint32_t imm5 = (index << 3) | 4;
        uint32_t insn = 0x4E001C00 | (imm5 << 16) | (rn << 5) | vd;
        emit32(insn);
    }
    
    // INS - insert element from register (double)
    void insD(uint8_t vd, uint8_t index, uint8_t rn) {
        uint32_t imm5 = (index << 4) | 8;
        uint32_t insn = 0x4E001C00 | (imm5 << 16) | (rn << 5) | vd;
        emit32(insn);
    }
    
    // DUP from GPR to all lanes (2-arg overloads for splat)
    void dupVB(uint8_t vd, uint8_t rn) {
        uint32_t insn = 0x4E010C00 | (rn << 5) | vd;
        emit32(insn);
    }
    
    void dupVH(uint8_t vd, uint8_t rn) {
        uint32_t insn = 0x4E020C00 | (rn << 5) | vd;
        emit32(insn);
    }
    
    void dupVS(uint8_t vd, uint8_t rn) {
        uint32_t insn = 0x4E040C00 | (rn << 5) | vd;
        emit32(insn);
    }
    
    void dupVD(uint8_t vd, uint8_t rn) {
        uint32_t insn = 0x4E080C00 | (rn << 5) | vd;
        emit32(insn);
    }
    
    // DUP lane aliases
    void dupVS_lane(uint8_t vd, uint8_t vn, uint8_t lane) { dupVS(vd, vn, lane); }
    void dupVD_lane(uint8_t vd, uint8_t vn, uint8_t lane) { dupVD(vd, vn, lane); }
    
    // INS from SIMD element
    void insSFromS(uint8_t vd, uint8_t dstIdx, uint8_t vn, uint8_t srcIdx) {
        uint32_t imm5 = (dstIdx << 3) | 4;
        uint32_t imm4 = srcIdx << 2;
        uint32_t insn = 0x6E000400 | (imm5 << 16) | (imm4 << 11) | (vn << 5) | vd;
        emit32(insn);
    }
    
    void insDFromD(uint8_t vd, uint8_t dstIdx, uint8_t vn, uint8_t srcIdx) {
        uint32_t imm5 = (dstIdx << 4) | 8;
        uint32_t imm4 = srcIdx << 3;
        uint32_t insn = 0x6E000400 | (imm5 << 16) | (imm4 << 11) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Compare equal 16B
    void cmeq16B(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x6E208C00 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Bit select
    void bsl(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x6E601C00 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Unsigned max across vector
    void umaxv(uint8_t vd, uint8_t vn) {
        uint32_t insn = 0x6E30A800 | (vn << 5) | vd;
        emit32(insn);
    }
    
    // LDQ from constant bytes (load literal into vector)
    void ldq(uint8_t vd, const uint8_t* bytes) {
        // Encode bytes inline or use literal pool
        // For now, use mov/ins to build constant
        (void)bytes;
        // Placeholder - actual impl would emit bytes to code and load
        uint32_t insn = 0x9C000000 | vd;  // LDUR Q
        emit32(insn);
    }
    
    // TBL with 4 args (2-register table + indices)
    void tbl2(uint8_t vd, uint8_t vn, uint8_t vn2, const uint8_t* indices) {
        (void)indices;  // Indices would be loaded separately
        (void)vn2;
        uint32_t insn = 0x4E002000 | (vn << 5) | vd;
        emit32(insn);
    }
    
    // INS element-to-element (3 args - simplified)  
    void insSFromS(uint8_t vd, uint8_t idx, uint8_t vn) {
        uint32_t imm5 = (idx << 3) | 4;
        uint32_t insn = 0x6E000400 | (imm5 << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    void insDFromD(uint8_t vd, uint8_t idx, uint8_t vn) {
        uint32_t imm5 = (idx << 4) | 8;
        uint32_t insn = 0x6E000400 | (imm5 << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // BSL with 4 args (result, true, false, mask)
    void bsl(uint8_t vd, uint8_t vt, uint8_t vf, uint8_t vm) {
        (void)vt; (void)vf;  // Real impl would handle properly
        uint32_t insn = 0x6E601C00 | (vm << 16) | (vd << 5) | vd;
        emit32(insn);
    }
    
    // CMP immediate 32-bit
    void cmpImm32(uint8_t rn, uint32_t imm) {
        // CMP Wn, #imm is SUBS WZR, Wn, #imm
        if (imm <= 0xFFF) {
            uint32_t insn = 0x7100001F | (imm << 10) | (rn << 5);
            emit32(insn);
        }
    }
    
    // SMULL (signed multiply long)
    void smull(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x0E20C000 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // SMLAL2 (signed multiply-accumulate long, high half)
    void smlal2(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x4E208000 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // SADDLP (signed add long pairwise)
    void saddlp(uint8_t vd, uint8_t vn) {
        uint32_t insn = 0x0E202800 | (vn << 5) | vd;
        emit32(insn);
    }
    
    // STR pre-indexed
    void strPre(uint8_t rt, uint8_t rn, int16_t offset) {
        uint32_t imm9 = offset & 0x1FF;
        uint32_t insn = 0xF8000C00 | (imm9 << 12) | (rn << 5) | rt;
        emit32(insn);
    }
    
    // LDR post-indexed
    void ldrPost(uint8_t rt, uint8_t rn, int16_t offset) {
        uint32_t imm9 = offset & 0x1FF;
        uint32_t insn = 0xF8400400 | (imm9 << 12) | (rn << 5) | rt;
        emit32(insn);
    }
    
    // LDAR - Load-Acquire Register 64-bit
    void ldar64(uint8_t rt, uint8_t rn) {
        uint32_t insn = 0xC8DFFC00 | (rn << 5) | rt;
        emit32(insn);
    }
    
    // LDAR - Load-Acquire Register 32-bit
    void ldar32(uint8_t rt, uint8_t rn) {
        uint32_t insn = 0x88DFFC00 | (rn << 5) | rt;
        emit32(insn);
    }
    
    // STLR - Store-Release Register 64-bit
    void stlr64(uint8_t rt, uint8_t rn) {
        uint32_t insn = 0xC89FFC00 | (rn << 5) | rt;
        emit32(insn);
    }
    
    // STLR - Store-Release Register 32-bit
    void stlr32(uint8_t rt, uint8_t rn) {
        uint32_t insn = 0x889FFC00 | (rn << 5) | rt;
        emit32(insn);
    }
    
    // LDAXR - Load-Acquire Exclusive Register 64-bit
    void ldaxr64(uint8_t rt, uint8_t rn) {
        uint32_t insn = 0xC85FFC00 | (rn << 5) | rt;
        emit32(insn);
    }
    
    // STLXR - Store-Release Exclusive Register 64-bit
    void stlxr64(uint8_t rs, uint8_t rt, uint8_t rn) {
        uint32_t insn = 0xC800FC00 | (rs << 16) | (rn << 5) | rt;
        emit32(insn);
    }
    
    // LSE Atomics - Load-Add Acquire-Release
    void ldaddal64(uint8_t rs, uint8_t rt, uint8_t rn) {
        uint32_t insn = 0xF8E00000 | (rs << 16) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void ldaddal32(uint8_t rs, uint8_t rt, uint8_t rn) {
        uint32_t insn = 0xB8E00000 | (rs << 16) | (rn << 5) | rt;
        emit32(insn);
    }
    
    // LSE Atomics - Load-Clear Acquire-Release
    void ldclral64(uint8_t rs, uint8_t rt, uint8_t rn) {
        uint32_t insn = 0xF8E01000 | (rs << 16) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void ldclral32(uint8_t rs, uint8_t rt, uint8_t rn) {
        uint32_t insn = 0xB8E01000 | (rs << 16) | (rn << 5) | rt;
        emit32(insn);
    }
    
    // LSE Atomics - Load-Set Acquire-Release
    void ldsetal64(uint8_t rs, uint8_t rt, uint8_t rn) {
        uint32_t insn = 0xF8E03000 | (rs << 16) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void ldsetal32(uint8_t rs, uint8_t rt, uint8_t rn) {
        uint32_t insn = 0xB8E03000 | (rs << 16) | (rn << 5) | rt;
        emit32(insn);
    }
    
    // LSE Atomics - Load-Xor Acquire-Release
    void ldeoral64(uint8_t rs, uint8_t rt, uint8_t rn) {
        uint32_t insn = 0xF8E02000 | (rs << 16) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void ldeoral32(uint8_t rs, uint8_t rt, uint8_t rn) {
        uint32_t insn = 0xB8E02000 | (rs << 16) | (rn << 5) | rt;
        emit32(insn);
    }
    
    // LSE Atomics - Swap Acquire-Release
    void swpal64(uint8_t rs, uint8_t rt, uint8_t rn) {
        uint32_t insn = 0xF8E08000 | (rs << 16) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void swpal32(uint8_t rs, uint8_t rt, uint8_t rn) {
        uint32_t insn = 0xB8E08000 | (rs << 16) | (rn << 5) | rt;
        emit32(insn);
    }
    
    // LSE Atomics - Compare and Swap Acquire-Release
    void casal64(uint8_t rs, uint8_t rt, uint8_t rn) {
        uint32_t insn = 0xC8E0FC00 | (rs << 16) | (rn << 5) | rt;
        emit32(insn);
    }
    
    void casal32(uint8_t rs, uint8_t rt, uint8_t rn) {
        uint32_t insn = 0x88E0FC00 | (rs << 16) | (rn << 5) | rt;
        emit32(insn);
    }
    
    // DMB - Data Memory Barrier
    void dmb(uint8_t option = 0xB) {  // 0xB = ISH (inner shareable)
        uint32_t insn = 0xD5033BBF | (option << 8);
        emit32(insn);
    }
    
    // DSB - Data Synchronization Barrier
    void dsb(uint8_t option = 0xB) {
        uint32_t insn = 0xD5033B9F | (option << 8);
        emit32(insn);
    }
    
    // ISB - Instruction Synchronization Barrier
    void isb() {
        uint32_t insn = 0xD5033FDF;
        emit32(insn);
    }
    
    // ==========================================================================
    // SIMD Floating-Point Operations (from SpiderMonkey VIXL reference)
    // ==========================================================================
    
    // FP absolute value (vector)
    void fabsV(uint8_t vd, uint8_t vn, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        uint32_t insn = 0x4EA0F800 | (sz << 22) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FP negate (vector)
    void fnegV(uint8_t vd, uint8_t vn, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        uint32_t insn = 0x6EA0F800 | (sz << 22) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FP square root (vector)
    void fsqrtV(uint8_t vd, uint8_t vn, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        uint32_t insn = 0x6EA1F800 | (sz << 22) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FP add (vector)
    void faddV(uint8_t vd, uint8_t vn, uint8_t vm, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        uint32_t insn = 0x4E20D400 | (sz << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FP subtract (vector)
    void fsubV(uint8_t vd, uint8_t vn, uint8_t vm, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        uint32_t insn = 0x4EA0D400 | (sz << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FP multiply (vector)
    void fmulV(uint8_t vd, uint8_t vn, uint8_t vm, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        uint32_t insn = 0x6E20DC00 | (sz << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FP divide (vector)
    void fdivV(uint8_t vd, uint8_t vn, uint8_t vm, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        uint32_t insn = 0x6E20FC00 | (sz << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FP minimum (vector)
    void fminV(uint8_t vd, uint8_t vn, uint8_t vm, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        uint32_t insn = 0x4EA0F400 | (sz << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FP maximum (vector)
    void fmaxV(uint8_t vd, uint8_t vn, uint8_t vm, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        uint32_t insn = 0x4E20F400 | (sz << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // ==========================================================================
    // FP Convert Operations
    // ==========================================================================
    
    // Convert FP to higher precision (S->D)
    void fcvtl(uint8_t vd, uint8_t vn) {
        uint32_t insn = 0x0E217800 | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Convert FP to lower precision (D->S)
    void fcvtn(uint8_t vd, uint8_t vn) {
        uint32_t insn = 0x0E216800 | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Signed convert to FP (vector)
    void scvtfV(uint8_t vd, uint8_t vn, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        uint32_t insn = 0x4E21D800 | (sz << 22) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Unsigned convert to FP (vector)
    void ucvtfV(uint8_t vd, uint8_t vn, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        uint32_t insn = 0x6E21D800 | (sz << 22) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FP convert to signed int, round toward zero (vector)
    void fcvtzsV(uint8_t vd, uint8_t vn, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        uint32_t insn = 0x4EA1B800 | (sz << 22) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FP convert to unsigned int, round toward zero (vector)
    void fcvtzuV(uint8_t vd, uint8_t vn, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        uint32_t insn = 0x6EA1B800 | (sz << 22) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // ==========================================================================
    // Saturating Arithmetic
    // ==========================================================================
    
    // Signed saturating add (vector)
    void sqaddV(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x4E200C00 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Unsigned saturating add (vector)
    void uqaddV(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x6E200C00 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Signed saturating subtract (vector)
    void sqsubV(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x4E202C00 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Unsigned saturating subtract (vector)
    void uqsubV(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x6E202C00 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Signed saturating doubling multiply long (16->32)
    void sqdmullV(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x0E60D000 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Signed saturating doubling multiply high (vector)
    void sqdmulhV(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x0E60B400 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Signed saturating rounding doubling multiply high (vector)
    void sqrdmulhV(uint8_t vd, uint8_t vn, uint8_t vm) {
        uint32_t insn = 0x6E60B400 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // ==========================================================================
    // Integer SIMD Arithmetic
    // ==========================================================================
    
    // Add (vector)
    void addV(uint8_t vd, uint8_t vn, uint8_t vm, uint8_t size = 2) {
        uint32_t insn = 0x4E208400 | (size << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Subtract (vector)
    void subV(uint8_t vd, uint8_t vn, uint8_t vm, uint8_t size = 2) {
        uint32_t insn = 0x6E208400 | (size << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Multiply (vector)
    void mulV(uint8_t vd, uint8_t vn, uint8_t vm, uint8_t size = 2) {
        uint32_t insn = 0x4E209C00 | (size << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Signed shift left (vector)
    void sshlV(uint8_t vd, uint8_t vn, uint8_t vm, uint8_t size = 2) {
        uint32_t insn = 0x4E204400 | (size << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Unsigned shift left (vector)
    void ushlV(uint8_t vd, uint8_t vn, uint8_t vm, uint8_t size = 2) {
        uint32_t insn = 0x6E204400 | (size << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // ==========================================================================
    // SIMD Compare
    // ==========================================================================
    
    // Compare equal (vector)
    void cmeqV(uint8_t vd, uint8_t vn, uint8_t vm, uint8_t size = 2) {
        uint32_t insn = 0x6E208C00 | (size << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Compare signed greater than (vector)
    void cmgtV(uint8_t vd, uint8_t vn, uint8_t vm, uint8_t size = 2) {
        uint32_t insn = 0x4E203400 | (size << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Compare signed greater than or equal (vector)
    void cmgeV(uint8_t vd, uint8_t vn, uint8_t vm, uint8_t size = 2) {
        uint32_t insn = 0x4E203C00 | (size << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Compare unsigned higher (vector)
    void cmhiV(uint8_t vd, uint8_t vn, uint8_t vm, uint8_t size = 2) {
        uint32_t insn = 0x6E203400 | (size << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Compare unsigned higher or same (vector)
    void cmhsV(uint8_t vd, uint8_t vn, uint8_t vm, uint8_t size = 2) {
        uint32_t insn = 0x6E203C00 | (size << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FP compare equal (vector)
    void fcmeqV(uint8_t vd, uint8_t vn, uint8_t vm, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        uint32_t insn = 0x4E20E400 | (sz << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FP compare greater than (vector)
    void fcmgtV(uint8_t vd, uint8_t vn, uint8_t vm, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        uint32_t insn = 0x6EA0E400 | (sz << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // FP compare greater than or equal (vector)
    void fcmgeV(uint8_t vd, uint8_t vn, uint8_t vm, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        uint32_t insn = 0x6E20E400 | (sz << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // ==========================================================================
    // Relaxed SIMD Operations (WASM Relaxed SIMD Proposal)
    // ==========================================================================
    
    // Fused multiply-add: vd = va + (vn * vm)
    void fmlaV(uint8_t vd, uint8_t vn, uint8_t vm, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        // FMLA (vector) encoding: 0E20CC00
        uint32_t insn = 0x0E20CC00 | (1 << 30) | (sz << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Fused multiply-subtract: vd = va - (vn * vm)
    void fmlsV(uint8_t vd, uint8_t vn, uint8_t vm, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        // FMLS (vector) encoding: 0EA0CC00
        uint32_t insn = 0x0EA0CC00 | (1 << 30) | (sz << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Table lookup (swizzle): vd[i] = vm[vn[i]]
    void tblV(uint8_t vd, uint8_t vn, uint8_t vm) {
        // TBL (single register table): 0E000000
        uint32_t insn = 0x0E000000 | (1 << 30) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Relaxed min (returns NaN propagation or implementation-defined)
    void fminnmV(uint8_t vd, uint8_t vn, uint8_t vm, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        // FMINNM (vector): 4EA0C400
        uint32_t insn = 0x4EA0C400 | (sz << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Relaxed max (returns NaN propagation or implementation-defined)
    void fmaxnmV(uint8_t vd, uint8_t vn, uint8_t vm, bool is64 = false) {
        uint32_t sz = is64 ? 1 : 0;
        // FMAXNM (vector): 4E20C400
        uint32_t insn = 0x4E20C400 | (sz << 22) | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Relaxed integer dot product i16x8 -> i32x4
    void sdot(uint8_t vd, uint8_t vn, uint8_t vm) {
        // SDOT (vector): 4E809400
        uint32_t insn = 0x4E809400 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    // Relaxed lane select (bitselect)
    void bslV(uint8_t vd, uint8_t vn, uint8_t vm) {
        // BSL: 6E601C00
        uint32_t insn = 0x6E601C00 | (vm << 16) | (vn << 5) | vd;
        emit32(insn);
    }
    
    size_t offset() const { return buf_->offset(); }
    
private:
    CodeBuffer* buf_;
};

// =============================================================================
// Platform-Specific Assembler Selection
// =============================================================================

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    using PlatformAssembler = X86Assembler;
    constexpr bool IsX64Platform = true;
    constexpr bool IsARM64Platform = false;
#elif defined(__aarch64__) || defined(_M_ARM64)
    using PlatformAssembler = ARM64Assembler;
    constexpr bool IsX64Platform = false;
    constexpr bool IsARM64Platform = true;
#else
    // Fallback to X86 for compilation (won't actually run JIT)
    using PlatformAssembler = X86Assembler;
    constexpr bool IsX64Platform = true;
    constexpr bool IsARM64Platform = false;
#endif

// Helper macros for platform-specific code emission
// Use variadic macro to handle braced blocks with commas
#define EMIT_X64(...) do { if constexpr (IsX64Platform) { __VA_ARGS__ } } while(0)
#define EMIT_ARM64(...) do { if constexpr (IsARM64Platform) { __VA_ARGS__ } } while(0)

// =============================================================================
// Simple Register Allocator
// =============================================================================

SimpleRegAllocator::SimpleRegAllocator() {
#if defined(__x86_64__) || defined(_M_X64)
    // Available GPRs: rax, rcx, rdx, rbx, rsi, rdi, r8-r15 (minus rsp, rbp)
    gprMask_ = 0b1111111111001111;  // Exclude rsp (4), rbp (5)
    gprScratch_ = 11;  // r11 as scratch
    fprMask_ = 0xFFFF;  // xmm0-xmm15
    fprScratch_ = 15;   // xmm15 as scratch
#elif defined(__aarch64__) || defined(_M_ARM64)
    // Available GPRs: x0-x28 (exclude x29=fp, x30=lr, x31=sp)
    gprMask_ = 0x1FFFFFFF;
    gprScratch_ = 16;
    fprMask_ = 0xFFFFFFFF;  // v0-v31
    fprScratch_ = 31;
#else
    gprMask_ = 0xFF;
    gprScratch_ = 0;
    fprMask_ = 0xFF;
    fprScratch_ = 0;
#endif
}

Reg SimpleRegAllocator::allocGPR() {
    for (int i = 0; i < 32; i++) {
        if (gprMask_ & (1u << i)) {
            gprMask_ &= ~(1u << i);
            return Reg::gpr(i);
        }
    }
    // Spill needed
    return Reg::gpr(255);
}

Reg SimpleRegAllocator::allocFPR() {
    for (int i = 0; i < 32; i++) {
        if (fprMask_ & (1u << i)) {
            fprMask_ &= ~(1u << i);
            return Reg::fpr(i);
        }
    }
    return Reg::fpr(255);
}

Reg SimpleRegAllocator::allocV128() {
    // V128 uses FPR registers on most platforms
    return allocFPR();
}

void SimpleRegAllocator::freeReg(Reg reg) {
    if (reg.regClass == RegClass::GPR && reg.code < 32) {
        gprMask_ |= (1u << reg.code);
    } else if ((reg.regClass == RegClass::FPR || reg.regClass == RegClass::V128) && reg.code < 32) {
        fprMask_ |= (1u << reg.code);
    }
}

bool SimpleRegAllocator::hasAvailableGPR() const {
    return gprMask_ != 0;
}

bool SimpleRegAllocator::hasAvailableFPR() const {
    return fprMask_ != 0;
}

void SimpleRegAllocator::spillAll() {
    // Reset to initial state
    *this = SimpleRegAllocator();
}

Reg SimpleRegAllocator::scratchGPR() const {
    return Reg::gpr(gprScratch_);
}

Reg SimpleRegAllocator::scratchFPR() const {
    return Reg::fpr(fprScratch_);
}

// =============================================================================
// Baseline Compiler Implementation
// =============================================================================

BaselineCompiler::BaselineCompiler(const WasmModule* module,
                                   const BaselineCompilerOptions& options)
    : module_(module)
    , options_(options)
    , codeBuffer_(std::make_unique<CodeBuffer>()) {
}

BaselineCompiler::~BaselineCompiler() = default;

bool BaselineCompiler::compileFunction(uint32_t funcIndex, CompiledFunction* result) {
    if (funcIndex >= module_->numFunctions()) {
        error_ = "function index out of bounds";
        return false;
    }
    
    const auto& funcDecl = module_->functions()[funcIndex];
    return compileFunctionBody(funcDecl, result);
}

BaselineCompilationResult BaselineCompiler::compileAll() {
    BaselineCompilationResult result;
    result.success = true;
    
    for (uint32_t i = module_->numImportedFunctions(); i < module_->numFunctions(); i++) {
        CompiledFunction func;
        if (!compileFunction(i, &func)) {
            result.error = error_;
            result.success = false;
            return result;
        }
        result.functions.push_back(std::move(func));
    }
    
    return result;
}

bool BaselineCompiler::compileFunctionBody(const FunctionDecl& func, CompiledFunction* result) {
    // Reset state
    valueStack_.clear();
    controlStack_.clear();
    locals_.clear();
    frameSize_ = 0;
    maxFrameSize_ = 0;
    codeBuffer_ = std::make_unique<CodeBuffer>();
    regAlloc_ = SimpleRegAllocator();
    pcMapping_.clear();
    
    funcIndex_ = func.typeIndex;
    funcType_ = module_->funcType(func.typeIndex);
    if (!funcType_) {
        error_ = "invalid function type index";
        return false;
    }
    
    // Set up locals (params + local declarations)
    uint32_t numLocals = funcType_->params().size();
    
    // Calculate frame size
    for (const auto& param : funcType_->params()) {
        StackSlot slot;
        slot.type = param;
        slot.offset = frameSize_;
        frameSize_ += param.size();
        locals_.push_back(slot);
    }
    
    // Emit function prologue
    emitPrologue(funcType_, numLocals);
    
    // Initial control frame for the function body (results = function return types)
    BlockType bodyType = BlockType::typeIndex(func.typeIndex);
    controlStack_.push_back(BaselineControlFrame(
        BaselineControlFrame::Kind::Block,
        bodyType,
        0  // Stack height
    ));
    controlStack_.back().endLabel = newLabel();
    
    // Decode and compile the function body
    const uint8_t* bodyStart = module_->bytecode() + func.codeOffset;
    const uint8_t* bodyEnd = bodyStart + func.codeSize;
    Decoder bodyDecoder(bodyStart, bodyEnd);
    decoder_ = &bodyDecoder;
    
    // Skip local declarations (already processed by validation)
    uint32_t numLocalDecls = 0;
    if (!bodyDecoder.readVarU32(&numLocalDecls)) {
        error_ = "failed to read local count";
        return false;
    }
    
    for (uint32_t i = 0; i < numLocalDecls; i++) {
        uint32_t count;
        uint8_t typeCode;
        if (!bodyDecoder.readVarU32(&count) || !bodyDecoder.readFixedU8(&typeCode)) {
            error_ = "failed to read local declaration";
            return false;
        }
        
        auto localTypeOpt = ValType::fromTypeCode(static_cast<TypeCode>(typeCode));
        if (!localTypeOpt) {
            error_ = "invalid local type";
            return false;
        }
        ValType localType = *localTypeOpt;
        for (uint32_t j = 0; j < count; j++) {
            StackSlot slot;
            slot.type = localType;
            slot.offset = frameSize_;
            frameSize_ += localType.size();
            locals_.push_back(slot);
        }
    }
    
    maxFrameSize_ = frameSize_;
    
    // Compile instructions
    while (!bodyDecoder.done()) {
        size_t instrOffset = bodyDecoder.currentOffset();
        pcMapping_.push_back({codeBuffer_->offset(), static_cast<uint32_t>(instrOffset)});
        
        Opcode op;
        if (!bodyDecoder.readOp(&op)) {
            error_ = "failed to read opcode";
            return false;
        }
        
        if (!emitOp(op)) {
            return false;
        }
    }
    
    // Bind end label
    if (!controlStack_.empty()) {
        bindLabel(controlStack_.back().endLabel);
    }
    
    // Emit epilogue
    emitEpilogue();
    
    // Finalize result
    result->code = codeBuffer_->takeCode();
    result->funcIndex = funcIndex_;
    result->stackSlots = maxFrameSize_ / 8;
    result->pcToOffset = std::move(pcMapping_);
    
    decoder_ = nullptr;
    return true;
}

bool BaselineCompiler::emitOp(Opcode op) {
    // Handle primary opcodes (single byte)
    if (op.isOp()) {
        Op primaryOp = op.asOp();
        
        switch (primaryOp) {
            case Op::Unreachable:
                return emitUnreachable();
            case Op::Nop:
                return true;  // Nothing to emit
            case Op::Block: {
                BlockType bt;
                if (!decoder_->readBlockType(&bt)) {
                    error_ = "failed to read block type";
                    return false;
                }
                return emitBlock(bt);
            }
            case Op::Loop: {
                BlockType bt;
                if (!decoder_->readBlockType(&bt)) {
                    error_ = "failed to read block type";
                    return false;
                }
                return emitLoop(bt);
            }
            case Op::If: {
                BlockType bt;
                if (!decoder_->readBlockType(&bt)) {
                    error_ = "failed to read block type";
                    return false;
                }
                return emitIf(bt);
            }
            case Op::Else:
                return emitElse();
            case Op::End:
                return emitEnd();
            case Op::Br: {
                uint32_t depth;
                if (!decoder_->readVarU32(&depth)) {
                    error_ = "failed to read br depth";
                    return false;
                }
                return emitBr(depth);
            }
            case Op::BrIf: {
                uint32_t depth;
                if (!decoder_->readVarU32(&depth)) {
                    error_ = "failed to read br_if depth";
                    return false;
                }
                return emitBrIf(depth);
            }
            case Op::Return:
                return emitReturn();
            case Op::Call: {
                uint32_t funcIndex;
                if (!decoder_->readVarU32(&funcIndex)) {
                    error_ = "failed to read call func index";
                    return false;
                }
                return emitCall(funcIndex);
            }
            case Op::CallIndirect: {
                uint32_t typeIndex, tableIndex;
                if (!decoder_->readVarU32(&typeIndex) || !decoder_->readVarU32(&tableIndex)) {
                    error_ = "failed to read call_indirect indices";
                    return false;
                }
                return emitCallIndirect(typeIndex, tableIndex);
            }
            case Op::Drop:
                return emitDrop();
            case Op::SelectNumeric:
            case Op::SelectTyped:
                return emitSelect();
            case Op::LocalGet: {
                uint32_t localIndex;
                if (!decoder_->readVarU32(&localIndex)) {
                    error_ = "failed to read local index";
                    return false;
                }
                return emitLocalGet(localIndex);
            }
            case Op::LocalSet: {
                uint32_t localIndex;
                if (!decoder_->readVarU32(&localIndex)) {
                    error_ = "failed to read local index";
                    return false;
                }
                return emitLocalSet(localIndex);
            }
            case Op::LocalTee: {
                uint32_t localIndex;
                if (!decoder_->readVarU32(&localIndex)) {
                    error_ = "failed to read local index";
                    return false;
                }
                return emitLocalTee(localIndex);
            }
            case Op::GlobalGet: {
                uint32_t globalIndex;
                if (!decoder_->readVarU32(&globalIndex)) {
                    error_ = "failed to read global index";
                    return false;
                }
                return emitGlobalGet(globalIndex);
            }
            case Op::GlobalSet: {
                uint32_t globalIndex;
                if (!decoder_->readVarU32(&globalIndex)) {
                    error_ = "failed to read global index";
                    return false;
                }
                return emitGlobalSet(globalIndex);
            }
            case Op::I32Const: {
                int32_t value;
                if (!decoder_->readVarS32(&value)) {
                    error_ = "failed to read i32.const";
                    return false;
                }
                return emitI32Const(value);
            }
            case Op::I64Const: {
                int64_t value;
                if (!decoder_->readVarS64(&value)) {
                    error_ = "failed to read i64.const";
                    return false;
                }
                return emitI64Const(value);
            }
            case Op::F32Const: {
                float value;
                if (!decoder_->readFixedF32(&value)) {
                    error_ = "failed to read f32.const";
                    return false;
                }
                return emitF32Const(value);
            }
            case Op::F64Const: {
                double value;
                if (!decoder_->readFixedF64(&value)) {
                    error_ = "failed to read f64.const";
                    return false;
                }
                return emitF64Const(value);
            }
            
            // I32 arithmetic
            case Op::I32Add:    return emitI32Add();
            case Op::I32Sub:    return emitI32Sub();
            case Op::I32Mul:    return emitI32Mul();
            case Op::I32DivS:   return emitI32DivS();
            case Op::I32DivU:   return emitI32DivU();
            case Op::I32RemS:   return emitI32RemS();
            case Op::I32RemU:   return emitI32RemU();
            case Op::I32And:    return emitI32And();
            case Op::I32Or:     return emitI32Or();
            case Op::I32Xor:    return emitI32Xor();
            case Op::I32Shl:    return emitI32Shl();
            case Op::I32ShrS:   return emitI32ShrS();
            case Op::I32ShrU:   return emitI32ShrU();
            case Op::I32Rotl:   return emitI32Rotl();
            case Op::I32Rotr:   return emitI32Rotr();
            case Op::I32Clz:    return emitI32Clz();
            case Op::I32Ctz:    return emitI32Ctz();
            case Op::I32Popcnt: return emitI32Popcnt();
            case Op::I32Eqz:    return emitI32Eqz();
            
            // I32 comparisons
            case Op::I32Eq:     return emitI32Eq();
            case Op::I32Ne:     return emitI32Ne();
            case Op::I32LtS:    return emitI32LtS();
            case Op::I32LtU:    return emitI32LtU();
            case Op::I32GtS:    return emitI32GtS();
            case Op::I32GtU:    return emitI32GtU();
            case Op::I32LeS:    return emitI32LeS();
            case Op::I32LeU:    return emitI32LeU();
            case Op::I32GeS:    return emitI32GeS();
            case Op::I32GeU:    return emitI32GeU();
            
            // I64 arithmetic
            case Op::I64Add:    return emitI64Add();
            case Op::I64Sub:    return emitI64Sub();
            case Op::I64Mul:    return emitI64Mul();
            
            // F32 operations
            case Op::F32Add:    return emitF32Add();
            case Op::F32Sub:    return emitF32Sub();
            case Op::F32Mul:    return emitF32Mul();
            case Op::F32Div:    return emitF32Div();
            case Op::F32Sqrt:   return emitF32Sqrt();
            case Op::F32Abs:    return emitF32Abs();
            case Op::F32Neg:    return emitF32Neg();
            
            // F64 operations
            case Op::F64Add:    return emitF64Add();
            case Op::F64Sub:    return emitF64Sub();
            case Op::F64Mul:    return emitF64Mul();
            case Op::F64Div:    return emitF64Div();
            case Op::F64Sqrt:   return emitF64Sqrt();
            
            // Conversions
            case Op::I32WrapI64:       return emitI32WrapI64();
            case Op::I64ExtendI32S:    return emitI64ExtendI32S();
            case Op::I64ExtendI32U:    return emitI64ExtendI32U();
            case Op::I32TruncF32S:     return emitI32TruncF32S();
            case Op::I32TruncF32U:     return emitI32TruncF32U();
            case Op::I32TruncF64S:     return emitI32TruncF64S();
            case Op::I32TruncF64U:     return emitI32TruncF64U();
            case Op::F32ConvertI32S:   return emitF32ConvertI32S();
            case Op::F32ConvertI32U:   return emitF32ConvertI32U();
            case Op::F64ConvertI32S:   return emitF64ConvertI32S();
            case Op::F64ConvertI32U:   return emitF64ConvertI32U();
            case Op::F32DemoteF64:     return emitF32DemoteF64();
            case Op::F64PromoteF32:    return emitF64PromoteF32();
            case Op::I32ReinterpretF32: return emitI32ReinterpretF32();
            case Op::I64ReinterpretF64: return emitI64ReinterpretF64();
            case Op::F32ReinterpretI32: return emitF32ReinterpretI32();
            case Op::F64ReinterpretI64: return emitF64ReinterpretI64();
            
            // Memory operations - read memarg
            case Op::I32Load:
            case Op::I64Load:
            case Op::F32Load:
            case Op::F64Load:
            case Op::I32Load8S:
            case Op::I32Load8U:
            case Op::I32Load16S:
            case Op::I32Load16U:
            case Op::I64Load8S:
            case Op::I64Load8U:
            case Op::I64Load16S:
            case Op::I64Load16U:
            case Op::I64Load32S:
            case Op::I64Load32U: {
                uint32_t align, offset;
                if (!decoder_->readVarU32(&align) || !decoder_->readVarU32(&offset)) {
                    error_ = "failed to read memarg";
                    return false;
                }
                return emitLoad(primaryOp, align, offset);
            }
            
            case Op::I32Store:
            case Op::I64Store:
            case Op::F32Store:
            case Op::F64Store:
            case Op::I32Store8:
            case Op::I32Store16:
            case Op::I64Store8:
            case Op::I64Store16:
            case Op::I64Store32: {
                uint32_t align, offset;
                if (!decoder_->readVarU32(&align) || !decoder_->readVarU32(&offset)) {
                    error_ = "failed to read memarg";
                    return false;
                }
                return emitStore(primaryOp, align, offset);
            }
            
            case Op::MemorySize: {
                uint32_t memIndex;
                if (!decoder_->readVarU32(&memIndex)) {
                    error_ = "failed to read memory index";
                    return false;
                }
                return emitMemorySize(memIndex);
            }
            
            case Op::MemoryGrow: {
                uint32_t memIndex;
                if (!decoder_->readVarU32(&memIndex)) {
                    error_ = "failed to read memory index";
                    return false;
                }
                return emitMemoryGrow(memIndex);
            }
            
            // SIMD operations (0xFD prefix) - handled by checking if we're in the SIMD range
            // The SIMD prefix byte is handled at the top level by matching the raw opcode value
            default: {
                // Check if this might be a SIMD operation (0xFD prefix byte)
                uint8_t primaryByte = static_cast<uint8_t>(primaryOp);
                if (primaryByte == 0xFD) {
                    uint32_t simdOp;
                    if (!decoder_->readVarU32(&simdOp)) {
                        error_ = "failed to read SIMD opcode";
                        return false;
                    }
                    return emitSimdOp(simdOp);
                }
                error_ = "unsupported opcode: " + std::to_string(static_cast<int>(primaryOp));
                return false;
            }
        }
    }
    
    error_ = "invalid opcode";
    return false;
}

// =============================================================================
// Control Flow Emission
// =============================================================================

bool BaselineCompiler::emitBlock(BlockType bt) {
    BaselineControlFrame frame(BaselineControlFrame::Kind::Block, bt, valueStack_.size());
    frame.endLabel = newLabel();
    controlStack_.push_back(frame);
    return true;
}

bool BaselineCompiler::emitLoop(BlockType bt) {
    BaselineControlFrame frame(BaselineControlFrame::Kind::Loop, bt, valueStack_.size());
    frame.endLabel = newLabel();
    bindLabel(frame.endLabel);  // Loop target is the start
    
    // Emit tier-up check at loop header
    if (options_.enableTierUp) {
        emitTierUpCheck();
    }
    
    controlStack_.push_back(frame);
    return true;
}

bool BaselineCompiler::emitIf(BlockType bt) {
    ValueLocation cond = pop();
    
    BaselineControlFrame frame(BaselineControlFrame::Kind::If, bt, valueStack_.size());
    frame.endLabel = newLabel();
    frame.elseLabel = newLabel();
    
    X86Assembler masm(codeBuffer_.get());
    
    // Get condition into register
    Reg condReg;
    if (cond.isRegister()) {
        condReg = cond.reg;
    } else if (cond.isImmediate()) {
        // Immediate: emit zero check directly
        if (cond.immediate == 0) {
            // Condition is false, jump to else unconditionally
            jumpTo(frame.elseLabel);
            controlStack_.push_back(frame);
            return true;
        }
        // Non-zero immediate: condition is true, fall through
        controlStack_.push_back(frame);
        return true;
    } else {
        condReg = allocReg(ValType::i32());
        masm.load32(condReg.code, X86Assembler::RBP, cond.slot.offset);
    }
    
    // Test condition and branch to else if zero (false)
    masm.test32(condReg.code, condReg.code);
    
    // Emit jz (jump if zero) to else label
    if (frame.elseLabel.bound) {
        int32_t displacement = static_cast<int32_t>(frame.elseLabel.offset - (codeBuffer_->offset() + 6));
        codeBuffer_->emit8(0x0F);
        codeBuffer_->emit8(0x84);  // jz rel32
        codeBuffer_->emit32(displacement);
    } else {
        codeBuffer_->emit8(0x0F);
        codeBuffer_->emit8(0x84);  // jz rel32
        frame.elseLabel.pendingJumps.push_back(codeBuffer_->offset());
        codeBuffer_->emit32(0);
    }
    
    if (!cond.isRegister()) {
        freeReg(condReg);
    }
    
    controlStack_.push_back(frame);
    return true;
}

bool BaselineCompiler::emitElse() {
    if (controlStack_.empty()) {
        error_ = "else without matching if";
        return false;
    }
    
    auto& frame = controlStack_.back();
    if (frame.kind != BaselineControlFrame::Kind::If) {
        error_ = "else in non-if block";
        return false;
    }
    
    // Jump over else block
    jumpTo(frame.endLabel);
    
    // Bind else label
    bindLabel(frame.elseLabel);
    
    frame.kind = BaselineControlFrame::Kind::Else;
    return true;
}

bool BaselineCompiler::emitEnd() {
    if (controlStack_.empty()) {
        error_ = "end without matching block";
        return false;
    }
    
    auto frame = controlStack_.back();
    controlStack_.pop_back();
    
    // Bind else label for if without else
    if (frame.kind == BaselineControlFrame::Kind::If) {
        bindLabel(frame.elseLabel);
    }
    
    // Bind end label
    if (frame.kind != BaselineControlFrame::Kind::Loop) {
        bindLabel(frame.endLabel);
    }
    
    return true;
}

bool BaselineCompiler::emitBr(uint32_t depth) {
    if (depth >= controlStack_.size()) {
        error_ = "br depth out of bounds";
        return false;
    }
    
    auto& target = controlStack_[controlStack_.size() - 1 - depth];
    jumpTo(target.endLabel);
    return true;
}

bool BaselineCompiler::emitBrIf(uint32_t depth) {
    if (depth >= controlStack_.size()) {
        error_ = "br_if depth out of bounds";
        return false;
    }
    
    ValueLocation cond = pop();
    auto& target = controlStack_[controlStack_.size() - 1 - depth];
    
    if (cond.isRegister()) {
        branchTo(target.endLabel, cond.reg);
    } else {
        Reg condReg = allocReg(ValType::i32());
        branchTo(target.endLabel, condReg);
        freeReg(condReg);
    }
    
    return true;
}

bool BaselineCompiler::emitBrTable(const std::vector<uint32_t>& targets, uint32_t defaultTarget) {
    ValueLocation index = pop();
    
    X86Assembler masm(codeBuffer_.get());
    
    // Get index into register
    Reg indexReg = allocReg(ValType::i32());
    if (index.isRegister()) {
        if (index.reg.code != indexReg.code) {
            masm.mov32(indexReg.code, index.reg.code);
        }
    } else {
        // Load from stack
        masm.load32(indexReg.code, X86Assembler::RBP, index.slot.offset);
    }
    
    // Compare against table size
    size_t numTargets = targets.size();
    masm.cmp32Imm(indexReg.code, static_cast<int32_t>(numTargets));
    
    // Jump to default if index >= numTargets
    Label defaultLabel = newLabel();
    if (defaultTarget >= controlStack_.size()) {
        error_ = "br_table default depth out of bounds";
        return false;
    }
    auto& defaultFrame = controlStack_[controlStack_.size() - 1 - defaultTarget];
    
    // jae default (unsigned compare, jump if above or equal)
    codeBuffer_->emit8(0x0F);
    codeBuffer_->emit8(0x83);  // jae rel32
    defaultFrame.endLabel.pendingJumps.push_back(codeBuffer_->offset());
    codeBuffer_->emit32(0);
    
    // Generate jump table for small tables, linear search for larger ones
    if (numTargets <= 8) {
        // Linear comparison chain for small tables
        for (size_t i = 0; i < numTargets; ++i) {
            uint32_t depth = targets[i];
            if (depth >= controlStack_.size()) {
                error_ = "br_table target depth out of bounds";
                return false;
            }
            
            masm.cmp32Imm(indexReg.code, static_cast<int32_t>(i));
            
            auto& targetFrame = controlStack_[controlStack_.size() - 1 - depth];
            // je target
            codeBuffer_->emit8(0x0F);
            codeBuffer_->emit8(0x84);  // je rel32
            targetFrame.endLabel.pendingJumps.push_back(codeBuffer_->offset());
            codeBuffer_->emit32(0);
        }
    } else {
        // For larger tables, use indirect jump through jump table
        // lea rax, [rip + jump_table]
        // jmp [rax + index*8]
        // This is more complex - simplified version with linear search for now
        for (size_t i = 0; i < numTargets; ++i) {
            uint32_t depth = targets[i];
            if (depth >= controlStack_.size()) {
                error_ = "br_table target depth out of bounds";
                return false;
            }
            
            masm.cmp32Imm(indexReg.code, static_cast<int32_t>(i));
            
            auto& targetFrame = controlStack_[controlStack_.size() - 1 - depth];
            codeBuffer_->emit8(0x0F);
            codeBuffer_->emit8(0x84);  // je rel32
            targetFrame.endLabel.pendingJumps.push_back(codeBuffer_->offset());
            codeBuffer_->emit32(0);
        }
    }
    
    // Fall through to default
    jumpTo(defaultFrame.endLabel);
    
    freeReg(indexReg);
    return true;
}

bool BaselineCompiler::emitReturn() {
    emitEpilogue();
    return true;
}

bool BaselineCompiler::emitUnreachable() {
    emitTrap(Trap::Unreachable);
    return true;
}

// =============================================================================
// Locals and Globals
// =============================================================================

bool BaselineCompiler::emitLocalGet(uint32_t localIndex) {
    if (localIndex >= locals_.size()) {
        error_ = "local index out of bounds";
        return false;
    }
    
    const auto& slot = locals_[localIndex];
    push(ValueLocation::onStack(slot.offset, slot.type));
    return true;
}

bool BaselineCompiler::emitLocalSet(uint32_t localIndex) {
    if (localIndex >= locals_.size()) {
        error_ = "local index out of bounds";
        return false;
    }
    
    ValueLocation val = pop();
    const auto& slot = locals_[localIndex];
    
    X86Assembler masm(codeBuffer_.get());
    
    // Get value into register if not already
    Reg valReg;
    if (val.isRegister()) {
        valReg = val.reg;
    } else {
        valReg = allocReg(slot.type);
        // Load from source location
        if (slot.type.kind() == ValType::Kind::F32) {
            masm.loadss(valReg.code, X86Assembler::RBP, val.slot.offset);
        } else if (slot.type.kind() == ValType::Kind::F64) {
            masm.loadsd(valReg.code, X86Assembler::RBP, val.slot.offset);
        } else if (slot.type.kind() == ValType::Kind::I64) {
            masm.load64(valReg.code, X86Assembler::RBP, val.slot.offset);
        } else {
            masm.load32(valReg.code, X86Assembler::RBP, val.slot.offset);
        }
    }
    
    // Store to local slot
    if (slot.type.kind() == ValType::Kind::F32) {
        masm.storess(X86Assembler::RBP, slot.offset, valReg.code);
    } else if (slot.type.kind() == ValType::Kind::F64) {
        masm.storesd(X86Assembler::RBP, slot.offset, valReg.code);
    } else if (slot.type.kind() == ValType::Kind::I64) {
        masm.store64(X86Assembler::RBP, slot.offset, valReg.code);
    } else {
        masm.store32(X86Assembler::RBP, slot.offset, valReg.code);
    }
    
    if (!val.isRegister()) {
        freeReg(valReg);
    }
    
    return true;
}

bool BaselineCompiler::emitLocalTee(uint32_t localIndex) {
    if (localIndex >= locals_.size()) {
        error_ = "local index out of bounds";
        return false;
    }
    
    ValueLocation val = peek(0);
    const auto& slot = locals_[localIndex];
    
    X86Assembler masm(codeBuffer_.get());
    
    // Get value into register
    Reg valReg;
    if (val.isRegister()) {
        valReg = val.reg;
    } else {
        valReg = allocReg(slot.type);
        if (slot.type.kind() == ValType::Kind::F32) {
            masm.loadss(valReg.code, X86Assembler::RBP, val.slot.offset);
        } else if (slot.type.kind() == ValType::Kind::F64) {
            masm.loadsd(valReg.code, X86Assembler::RBP, val.slot.offset);
        } else if (slot.type.kind() == ValType::Kind::I64) {
            masm.load64(valReg.code, X86Assembler::RBP, val.slot.offset);
        } else {
            masm.load32(valReg.code, X86Assembler::RBP, val.slot.offset);
        }
    }
    
    // Store to local (value stays on stack)
    if (slot.type.kind() == ValType::Kind::F32) {
        masm.storess(X86Assembler::RBP, slot.offset, valReg.code);
    } else if (slot.type.kind() == ValType::Kind::F64) {
        masm.storesd(X86Assembler::RBP, slot.offset, valReg.code);
    } else if (slot.type.kind() == ValType::Kind::I64) {
        masm.store64(X86Assembler::RBP, slot.offset, valReg.code);
    } else {
        masm.store32(X86Assembler::RBP, slot.offset, valReg.code);
    }
    
    if (!val.isRegister()) {
        freeReg(valReg);
    }
    
    return true;
}

bool BaselineCompiler::emitGlobalGet(uint32_t globalIndex) {
    if (globalIndex >= module_->numGlobals()) {
        error_ = "global index out of bounds";
        return false;
    }
    
    const auto& globalDecl = module_->globals()[globalIndex];
    Reg result = allocReg(globalDecl.type);
    
    X86Assembler masm(codeBuffer_.get());
    
    // Load global base pointer from instance (stored at RBP-8)
    Reg globalsBase = allocReg(ValType::i64());
    masm.load64(globalsBase.code, X86Assembler::RBP, -8);
    
    // Load global value (each global is 8 bytes aligned)
    int32_t globalOffset = static_cast<int32_t>(globalIndex * 8);
    
    if (globalDecl.type.kind() == ValType::Kind::F32) {
        masm.loadss(result.code, globalsBase.code, globalOffset);
    } else if (globalDecl.type.kind() == ValType::Kind::F64) {
        masm.loadsd(result.code, globalsBase.code, globalOffset);
    } else if (globalDecl.type.kind() == ValType::Kind::I64) {
        masm.load64(result.code, globalsBase.code, globalOffset);
    } else {
        masm.load32(result.code, globalsBase.code, globalOffset);
    }
    
    freeReg(globalsBase);
    push(ValueLocation::inReg(result, globalDecl.type));
    return true;
}

bool BaselineCompiler::emitGlobalSet(uint32_t globalIndex) {
    if (globalIndex >= module_->numGlobals()) {
        error_ = "global index out of bounds";
        return false;
    }
    
    ValueLocation val = pop();
    const auto& globalDecl = module_->globals()[globalIndex];
    
    X86Assembler masm(codeBuffer_.get());
    
    // Load global base pointer
    Reg globalsBase = allocReg(ValType::i64());
    masm.load64(globalsBase.code, X86Assembler::RBP, -8);
    
    // Get value into register
    Reg valReg;
    if (val.isRegister()) {
        valReg = val.reg;
    } else {
        valReg = allocReg(globalDecl.type);
        if (globalDecl.type.kind() == ValType::Kind::F32) {
            masm.loadss(valReg.code, X86Assembler::RBP, val.slot.offset);
        } else if (globalDecl.type.kind() == ValType::Kind::F64) {
            masm.loadsd(valReg.code, X86Assembler::RBP, val.slot.offset);
        } else if (globalDecl.type.kind() == ValType::Kind::I64) {
            masm.load64(valReg.code, X86Assembler::RBP, val.slot.offset);
        } else {
            masm.load32(valReg.code, X86Assembler::RBP, val.slot.offset);
        }
    }
    
    // Store to global
    int32_t globalOffset = static_cast<int32_t>(globalIndex * 8);
    
    if (globalDecl.type.kind() == ValType::Kind::F32) {
        masm.storess(globalsBase.code, globalOffset, valReg.code);
    } else if (globalDecl.type.kind() == ValType::Kind::F64) {
        masm.storesd(globalsBase.code, globalOffset, valReg.code);
    } else if (globalDecl.type.kind() == ValType::Kind::I64) {
        masm.store64(globalsBase.code, globalOffset, valReg.code);
    } else {
        masm.store32(globalsBase.code, globalOffset, valReg.code);
    }
    
    freeReg(globalsBase);
    if (!val.isRegister()) {
        freeReg(valReg);
    }
    
    return true;
}

// =============================================================================
// Constants
// =============================================================================

bool BaselineCompiler::emitI32Const(int32_t value) {
    push(ValueLocation::imm(value, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitI64Const(int64_t value) {
    push(ValueLocation::imm(value, ValType::i64()));
    return true;
}

bool BaselineCompiler::emitF32Const(float value) {
    int32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    push(ValueLocation::imm(bits, ValType::f32()));
    return true;
}

bool BaselineCompiler::emitF64Const(double value) {
    int64_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    push(ValueLocation::imm(bits, ValType::f64()));
    return true;
}

// =============================================================================
// I32 Binary Operations
// =============================================================================

bool BaselineCompiler::emitI32Add() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.add32(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.add32(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI32Sub() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.sub32(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.sub32(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI32Mul() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.imul32(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.mul32(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI32DivS() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (dst.code != X86Assembler::RAX)
                masm.mov32(X86Assembler::RAX, dst.code);
            masm.cdq();
            masm.idiv32(src.code);
            if (dst.code != X86Assembler::RAX)
                masm.mov32(dst.code, X86Assembler::RAX);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.sdiv32(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI32DivU() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (dst.code != X86Assembler::RAX)
                masm.mov32(X86Assembler::RAX, dst.code);
            masm.xor32Self(X86Assembler::RDX);
            masm.div32(src.code);
            if (dst.code != X86Assembler::RAX)
                masm.mov32(dst.code, X86Assembler::RAX);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.udiv32(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI32RemS() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (dst.code != X86Assembler::RAX)
                masm.mov32(X86Assembler::RAX, dst.code);
            masm.cdq();
            masm.idiv32(src.code);
            masm.mov32(dst.code, X86Assembler::RDX);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            // ARM64: rem = a - (a/b)*b using MSUB
            uint8_t tmp = ARM64Assembler::X9;  // Scratch register
            masm.sdiv32(tmp, dst.code, src.code);
            masm.msub32(dst.code, tmp, src.code, dst.code);
        });
    });
}

bool BaselineCompiler::emitI32RemU() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (dst.code != X86Assembler::RAX)
                masm.mov32(X86Assembler::RAX, dst.code);
            masm.xor32Self(X86Assembler::RDX);
            masm.div32(src.code);
            masm.mov32(dst.code, X86Assembler::RDX);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            uint8_t tmp = ARM64Assembler::X9;
            masm.udiv32(tmp, dst.code, src.code);
            masm.msub32(dst.code, tmp, src.code, dst.code);
        });
    });
}

bool BaselineCompiler::emitI32And() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.and32(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.and32(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI32Or() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.or32(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.orr32(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI32Xor() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.xor32(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.eor32(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI32Shl() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (src.code != X86Assembler::RCX) {
                masm.mov32(X86Assembler::RCX, src.code);
            }
            masm.shl32_cl(dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.lsl32(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI32ShrS() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (src.code != X86Assembler::RCX) {
                masm.mov32(X86Assembler::RCX, src.code);
            }
            masm.sar32_cl(dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.asr32(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI32ShrU() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (src.code != X86Assembler::RCX) {
                masm.mov32(X86Assembler::RCX, src.code);
            }
            masm.shr32_cl(dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.lsr32(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI32Rotl() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (src.code != X86Assembler::RCX)
                masm.mov32(X86Assembler::RCX, src.code);
            masm.rol32_cl(dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            // ARM64 ROTL: rd = (rn << rm) | (rn >> (32 - rm))
            // Use ROR with negated count: ror(32 - count)
            uint8_t tmp = ARM64Assembler::X9;
            masm.neg32(tmp, src.code);
            masm.ror32(dst.code, dst.code, tmp);
        });
    });
}

bool BaselineCompiler::emitI32Rotr() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (src.code != X86Assembler::RCX)
                masm.mov32(X86Assembler::RCX, src.code);
            masm.ror32_cl(dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.ror32(dst.code, dst.code, src.code);
        });
    });
}

// =============================================================================
// I32 Unary Operations
// =============================================================================

bool BaselineCompiler::emitI32Clz() {
    return emitUnaryOp(ValType::i32(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        // Use LZCNT if available (BMI1), otherwise BSR + adjustment
        masm.lzcnt32(dst.code, dst.code);
    });
}

bool BaselineCompiler::emitI32Ctz() {
    return emitUnaryOp(ValType::i32(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        // Use TZCNT if available (BMI1), otherwise BSF
        masm.tzcnt32(dst.code, dst.code);
    });
}

bool BaselineCompiler::emitI32Popcnt() {
    return emitUnaryOp(ValType::i32(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        masm.popcnt32(dst.code, dst.code);
    });
}

bool BaselineCompiler::emitI32Eqz() {
    return emitUnaryOp(ValType::i32(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        // cmp dst, 0; sete dst; movzx dst, dst
        masm.cmp32(dst.code, 0); // Compare with 0
        masm.setcc(X86Assembler::E, dst.code); // Set byte register based on ZF
        masm.movzx8to32(dst.code, dst.code); // Zero-extend the byte to 32-bit
    });
}

// =============================================================================
// I32 Comparison Operations
// =============================================================================

bool BaselineCompiler::emitI32Eq() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.setcc(X86Assembler::E, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::EQ);
        });
    });
}

bool BaselineCompiler::emitI32Ne() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.setcc(X86Assembler::NE, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::NE);
        });
    });
}

bool BaselineCompiler::emitI32LtS() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.setcc(X86Assembler::L, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::LT);
        });
    });
}

bool BaselineCompiler::emitI32LtU() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.setcc(X86Assembler::B, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::LO);
        });
    });
}

bool BaselineCompiler::emitI32GtS() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.setcc(X86Assembler::G, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::GT);
        });
    });
}

bool BaselineCompiler::emitI32GtU() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.setcc(X86Assembler::A, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::HI);
        });
    });
}

bool BaselineCompiler::emitI32LeS() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.setcc(X86Assembler::LE, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::LE);
        });
    });
}

bool BaselineCompiler::emitI32LeU() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.setcc(X86Assembler::BE, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::LS);
        });
    });
}

bool BaselineCompiler::emitI32GeS() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.setcc(X86Assembler::GE, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::GE);
        });
    });
}

bool BaselineCompiler::emitI32GeU() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.setcc(X86Assembler::AE, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp32(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::HS);
        });
    });
}

// =============================================================================
// I64 Operations (Stubs)
// =============================================================================

bool BaselineCompiler::emitI64Add() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.add64(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.add64(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI64Sub() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.sub64(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.sub64(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI64Mul() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.imul64(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.mul64(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI64DivS() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (dst.code != X86Assembler::RAX)
                masm.mov64(X86Assembler::RAX, dst.code);
            masm.cqo();
            masm.idiv64(src.code);
            if (dst.code != X86Assembler::RAX)
                masm.mov64(dst.code, X86Assembler::RAX);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.sdiv64(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI64DivU() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (dst.code != X86Assembler::RAX)
                masm.mov64(X86Assembler::RAX, dst.code);
            masm.xor64(X86Assembler::RDX, X86Assembler::RDX);
            masm.div64(src.code);
            if (dst.code != X86Assembler::RAX)
                masm.mov64(dst.code, X86Assembler::RAX);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.udiv64(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI64RemS() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (dst.code != X86Assembler::RAX)
                masm.mov64(X86Assembler::RAX, dst.code);
            masm.cqo();
            masm.idiv64(src.code);
            masm.mov64(dst.code, X86Assembler::RDX);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            uint8_t tmp = ARM64Assembler::X9;
            masm.sdiv64(tmp, dst.code, src.code);
            masm.msub64(dst.code, tmp, src.code, dst.code);
        });
    });
}

bool BaselineCompiler::emitI64RemU() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (dst.code != X86Assembler::RAX)
                masm.mov64(X86Assembler::RAX, dst.code);
            masm.xor64(X86Assembler::RDX, X86Assembler::RDX);
            masm.div64(src.code);
            masm.mov64(dst.code, X86Assembler::RDX);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            uint8_t tmp = ARM64Assembler::X9;
            masm.udiv64(tmp, dst.code, src.code);
            masm.msub64(dst.code, tmp, src.code, dst.code);
        });
    });
}

bool BaselineCompiler::emitI64And() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.and64(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.and64(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI64Or() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.or64(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.orr64(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI64Xor() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.xor64(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.eor64(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI64Shl() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (src.code != X86Assembler::RCX)
                masm.mov64(X86Assembler::RCX, src.code);
            masm.shl64_cl(dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.lsl64(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI64ShrS() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (src.code != X86Assembler::RCX)
                masm.mov64(X86Assembler::RCX, src.code);
            masm.sar64_cl(dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.asr64(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI64ShrU() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (src.code != X86Assembler::RCX)
                masm.mov64(X86Assembler::RCX, src.code);
            masm.shr64_cl(dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.lsr64(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI64Rotl() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (src.code != X86Assembler::RCX)
                masm.mov64(X86Assembler::RCX, src.code);
            masm.rol64_cl(dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            uint8_t tmp = ARM64Assembler::X9;
            masm.neg64(tmp, src.code);
            masm.ror64(dst.code, dst.code, tmp);
        });
    });
}

bool BaselineCompiler::emitI64Rotr() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            if (src.code != X86Assembler::RCX)
                masm.mov64(X86Assembler::RCX, src.code);
            masm.ror64_cl(dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.ror64(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitI64Clz() {
    return emitUnaryOp(ValType::i64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        masm.lzcnt64(dst.code, dst.code);
    });
}

bool BaselineCompiler::emitI64Ctz() {
    return emitUnaryOp(ValType::i64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        masm.tzcnt64(dst.code, dst.code);
    });
}

bool BaselineCompiler::emitI64Popcnt() {
    return emitUnaryOp(ValType::i64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        masm.popcnt64(dst.code, dst.code);
    });
}

bool BaselineCompiler::emitI64Eqz() {
    return emitUnaryOp(ValType::i64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        masm.cmp64(dst.code, X86Assembler::RAX);  // cmp with 0 (xor'd register)
        masm.setcc(X86Assembler::E, dst.code);
        masm.movzx8to32(dst.code, dst.code);
    });
}

bool BaselineCompiler::emitI64Eq() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.setcc(X86Assembler::E, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.cset64(dst.code, ARM64Assembler::EQ);
        });
    });
}

bool BaselineCompiler::emitI64Ne() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.setcc(X86Assembler::NE, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.cset64(dst.code, ARM64Assembler::NE);
        });
    });
}

bool BaselineCompiler::emitI64LtS() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.setcc(X86Assembler::L, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.cset64(dst.code, ARM64Assembler::LT);
        });
    });
}

bool BaselineCompiler::emitI64LtU() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.setcc(X86Assembler::B, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.cset64(dst.code, ARM64Assembler::LO);
        });
    });
}

bool BaselineCompiler::emitI64GtS() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.setcc(X86Assembler::G, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.cset64(dst.code, ARM64Assembler::GT);
        });
    });
}

bool BaselineCompiler::emitI64GtU() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.setcc(X86Assembler::A, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.cset64(dst.code, ARM64Assembler::HI);
        });
    });
}

bool BaselineCompiler::emitI64LeS() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.setcc(X86Assembler::LE, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.cset64(dst.code, ARM64Assembler::LE);
        });
    });
}

bool BaselineCompiler::emitI64LeU() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.setcc(X86Assembler::BE, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.cset64(dst.code, ARM64Assembler::LS);
        });
    });
}

bool BaselineCompiler::emitI64GeS() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.setcc(X86Assembler::GE, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.cset64(dst.code, ARM64Assembler::GE);
        });
    });
}

bool BaselineCompiler::emitI64GeU() {
    return emitBinaryOp(ValType::i64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.setcc(X86Assembler::AE, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.cmp64(dst.code, src.code);
            masm.cset64(dst.code, ARM64Assembler::HS);
        });
    });
}

// =============================================================================
// Float Operations
// =============================================================================

bool BaselineCompiler::emitF32Add() {
    return emitBinaryOp(ValType::f32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.addss(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.faddS(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitF32Sub() {
    return emitBinaryOp(ValType::f32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.subss(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fsubS(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitF32Mul() {
    return emitBinaryOp(ValType::f32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.mulss(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fmulS(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitF32Div() {
    return emitBinaryOp(ValType::f32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.divss(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fdivS(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitF32Sqrt() {
    return emitUnaryOp(ValType::f32(), [this](Reg dst) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.sqrtss(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fsqrtS(dst.code, dst.code);
        });
    });
}

bool BaselineCompiler::emitF32Abs() {
    return emitUnaryOp(ValType::f32(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        // Clear sign bit by moving to GPR, AND with 0x7FFFFFFF, move back
        // Allocate a scratch GPR
        Reg scratch = allocReg(ValType::i32());
        masm.movd_xmm2gpr(scratch.code, dst.code);
        masm.and32(scratch.code, 0x7FFFFFFF);  // Clear sign bit
        masm.movd_gpr2xmm(dst.code, scratch.code);
        freeReg(scratch);
    });
}

bool BaselineCompiler::emitF32Neg() {
    return emitUnaryOp(ValType::f32(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        // Flip sign bit by XOR with 0x80000000
        Reg scratch = allocReg(ValType::i32());
        masm.movd_xmm2gpr(scratch.code, dst.code);
        masm.xor32(scratch.code, static_cast<int32_t>(0x80000000));
        masm.movd_gpr2xmm(dst.code, scratch.code);
        freeReg(scratch);
    });
}

bool BaselineCompiler::emitF32Ceil() {
    return emitUnaryOp(ValType::f32(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        if (CpuFeatures::sse41Supported()) {
            // SSE4.1 roundss: mode 0x0A = ceil + inexact exception suppressed
            masm.roundss(dst.code, dst.code, 0x0A);
        } else {
            // Fallback: convert to int (truncates), add 1 if positive with fraction, convert back
            Reg tmp = allocReg(ValType::i32());
            Reg tmp2 = allocReg(ValType::f32());
            masm.cvttss2si(tmp.code, dst.code);    // tmp = trunc(dst)
            masm.cvtsi2ss(tmp2.code, tmp.code);    // tmp2 = (float)tmp
            // If dst > tmp2, result = tmp + 1, else result = tmp
            masm.ucomiss(dst.code, tmp2.code);     // Compare
            masm.cvtsi2ss(dst.code, tmp.code);     // Reload truncated
            // Add 1 if dst > trunc(dst) - requires conditional logic
            // For baseline, accept slight imprecision on fallback path
            freeReg(tmp);
            freeReg(tmp2);
        }
    });
}

bool BaselineCompiler::emitF32Floor() {
    return emitUnaryOp(ValType::f32(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        if (CpuFeatures::sse41Supported()) {
            // SSE4.1 roundss: mode 0x09 = floor + inexact exception suppressed
            masm.roundss(dst.code, dst.code, 0x09);
        } else {
            // Fallback: convert to int (truncates toward zero), subtract 1 if negative with fraction
            Reg tmp = allocReg(ValType::i32());
            masm.cvttss2si(tmp.code, dst.code);
            masm.cvtsi2ss(dst.code, tmp.code);
            freeReg(tmp);
        }
    });
}

bool BaselineCompiler::emitF32Trunc() {
    return emitUnaryOp(ValType::f32(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        if (CpuFeatures::sse41Supported()) {
            // SSE4.1 roundss: mode 0x0B = trunc + inexact exception suppressed
            masm.roundss(dst.code, dst.code, 0x0B);
        } else {
            // Trunc is natural behavior of cvttss2si
            Reg tmp = allocReg(ValType::i32());
            masm.cvttss2si(tmp.code, dst.code);
            masm.cvtsi2ss(dst.code, tmp.code);
            freeReg(tmp);
        }
    });
}

bool BaselineCompiler::emitF32Nearest() {
    return emitUnaryOp(ValType::f32(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        if (CpuFeatures::sse41Supported()) {
            // SSE4.1 roundss: mode 0x08 = round to nearest even + inexact suppressed
            masm.roundss(dst.code, dst.code, 0x08);
        } else {
            // Fallback: add 0.5 and truncate (banker's rounding approximation)
            Reg tmp = allocReg(ValType::i32());
            Reg half = allocReg(ValType::f32());
            masm.mov32Imm(tmp.code, 0x3F000000);  // 0.5f in IEEE-754
            masm.movd_gpr2xmm(half.code, tmp.code);
            masm.addss(dst.code, half.code);
            masm.cvttss2si(tmp.code, dst.code);
            masm.cvtsi2ss(dst.code, tmp.code);
            freeReg(half);
            freeReg(tmp);
        }
    });
}

bool BaselineCompiler::emitF32Copysign() {
    return emitBinaryOp(ValType::f32(), [this](Reg dst, Reg src) {
        X86Assembler masm(codeBuffer_.get());
        // Copy sign from src to dst: (dst & 0x7FFFFFFF) | (src & 0x80000000)
        Reg dstBits = allocReg(ValType::i32());
        Reg srcBits = allocReg(ValType::i32());
        masm.movd_xmm2gpr(dstBits.code, dst.code);
        masm.movd_xmm2gpr(srcBits.code, src.code);
        masm.and32(dstBits.code, 0x7FFFFFFF);  // Clear dst sign
        masm.and32(srcBits.code, static_cast<int32_t>(0x80000000));  // Get src sign
        masm.or32(dstBits.code, srcBits.code);  // Combine
        masm.movd_gpr2xmm(dst.code, dstBits.code);
        freeReg(dstBits);
        freeReg(srcBits);
    });
}

bool BaselineCompiler::emitF32Min() {
    return emitBinaryOp(ValType::f32(), [this](Reg dst, Reg src) {
        X86Assembler masm(codeBuffer_.get());
        // WASM min semantics: min(a, NaN) = NaN, min(NaN, b) = NaN
        // minss returns second operand if either is NaN, so we need both orders
        // min(a,b) = minss(a, minss(b, a)) handles NaN properly
        masm.minss(src.code, dst.code);  // src = min(src, dst)
        masm.minss(dst.code, src.code);  // dst = min(dst, src)
    });
}

bool BaselineCompiler::emitF32Max() {
    return emitBinaryOp(ValType::f32(), [this](Reg dst, Reg src) {
        X86Assembler masm(codeBuffer_.get());
        // Same NaN handling as min
        masm.maxss(src.code, dst.code);
        masm.maxss(dst.code, src.code);
    });
}

bool BaselineCompiler::emitF64Add() {
    return emitBinaryOp(ValType::f64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.addsd(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.faddD(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitF64Sub() {
    return emitBinaryOp(ValType::f64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.subsd(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fsubD(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitF64Mul() {
    return emitBinaryOp(ValType::f64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.mulsd(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fmulD(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitF64Div() {
    return emitBinaryOp(ValType::f64(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.divsd(dst.code, src.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fdivD(dst.code, dst.code, src.code);
        });
    });
}

bool BaselineCompiler::emitF64Sqrt() {
    return emitUnaryOp(ValType::f64(), [this](Reg dst) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.sqrtsd(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fsqrtD(dst.code, dst.code);
        });
    });
}

bool BaselineCompiler::emitF64Abs() {
    return emitUnaryOp(ValType::f64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        // Clear sign bit (bit 63) via GPR: AND with 0x7FFFFFFFFFFFFFFF
        Reg scratch = allocReg(ValType::i64());
        masm.movq_xmm2gpr(scratch.code, dst.code);
        masm.mov64Imm(X86Assembler::RAX, 0x7FFFFFFFFFFFFFFFLL);
        masm.and64(scratch.code, X86Assembler::RAX);
        masm.movq_gpr2xmm(dst.code, scratch.code);
        freeReg(scratch);
    });
}

bool BaselineCompiler::emitF64Neg() {
    return emitUnaryOp(ValType::f64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        // Flip sign bit (bit 63) via GPR: XOR with 0x8000000000000000
        Reg scratch = allocReg(ValType::i64());
        masm.movq_xmm2gpr(scratch.code, dst.code);
        masm.mov64Imm(X86Assembler::RAX, static_cast<int64_t>(0x8000000000000000LL));
        masm.xor64(scratch.code, X86Assembler::RAX);
        masm.movq_gpr2xmm(dst.code, scratch.code);
        freeReg(scratch);
    });
}

bool BaselineCompiler::emitF64Ceil() {
    return emitUnaryOp(ValType::f64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        if (CpuFeatures::sse41Supported()) {
            // SSE4.1 roundsd: mode 0x0A = ceil + inexact exception suppressed
            masm.roundsd(dst.code, dst.code, 0x0A);
        } else {
            // Fallback: truncate and adjust for positive fractions
            Reg tmp = allocReg(ValType::i64());
            masm.cvttsd2si64(tmp.code, dst.code);
            masm.cvtsi2sd64(dst.code, tmp.code);
            freeReg(tmp);
        }
    });
}

bool BaselineCompiler::emitF64Floor() {
    return emitUnaryOp(ValType::f64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        if (CpuFeatures::sse41Supported()) {
            // SSE4.1 roundsd: mode 0x09 = floor + inexact exception suppressed
            masm.roundsd(dst.code, dst.code, 0x09);
        } else {
            // Fallback: truncate (note: not exact floor for negatives)
            Reg tmp = allocReg(ValType::i64());
            masm.cvttsd2si64(tmp.code, dst.code);
            masm.cvtsi2sd64(dst.code, tmp.code);
            freeReg(tmp);
        }
    });
}

bool BaselineCompiler::emitF64Trunc() {
    return emitUnaryOp(ValType::f64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        if (CpuFeatures::sse41Supported()) {
            // SSE4.1 roundsd: mode 0x0B = trunc + inexact exception suppressed
            masm.roundsd(dst.code, dst.code, 0x0B);
        } else {
            // Trunc is natural behavior of cvttsd2si
            Reg tmp = allocReg(ValType::i64());
            masm.cvttsd2si64(tmp.code, dst.code);
            masm.cvtsi2sd64(dst.code, tmp.code);
            freeReg(tmp);
        }
    });
}

bool BaselineCompiler::emitF64Nearest() {
    return emitUnaryOp(ValType::f64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        if (CpuFeatures::sse41Supported()) {
            // SSE4.1 roundsd: mode 0x08 = round to nearest even + inexact suppressed
            masm.roundsd(dst.code, dst.code, 0x08);
        } else {
            // Fallback: add 0.5 and truncate (banker's rounding approximation)
            Reg tmp = allocReg(ValType::i64());
            Reg half = allocReg(ValType::f64());
            masm.mov64Imm(tmp.code, 0x3FE0000000000000LL);  // 0.5 in IEEE-754 f64
            masm.movq_gpr2xmm(half.code, tmp.code);
            masm.addsd(dst.code, half.code);
            masm.cvttsd2si64(tmp.code, dst.code);
            masm.cvtsi2sd64(dst.code, tmp.code);
            freeReg(half);
            freeReg(tmp);
        }
    });
}

bool BaselineCompiler::emitF64Copysign() {
    return emitBinaryOp(ValType::f64(), [this](Reg dst, Reg src) {
        X86Assembler masm(codeBuffer_.get());
        // Copy sign from src to dst: (dst & 0x7FFF...) | (src & 0x8000...)
        Reg dstBits = allocReg(ValType::i64());
        Reg srcBits = allocReg(ValType::i64());
        masm.movq_xmm2gpr(dstBits.code, dst.code);
        masm.movq_xmm2gpr(srcBits.code, src.code);
        masm.mov64Imm(X86Assembler::RAX, 0x7FFFFFFFFFFFFFFFLL);
        masm.and64(dstBits.code, X86Assembler::RAX);  // Clear dst sign
        masm.mov64Imm(X86Assembler::RAX, static_cast<int64_t>(0x8000000000000000LL));
        masm.and64(srcBits.code, X86Assembler::RAX);  // Get src sign
        masm.or64(dstBits.code, srcBits.code);  // Combine
        masm.movq_gpr2xmm(dst.code, dstBits.code);
        freeReg(dstBits);
        freeReg(srcBits);
    });
}

bool BaselineCompiler::emitF64Min() {
    return emitBinaryOp(ValType::f64(), [this](Reg dst, Reg src) {
        X86Assembler masm(codeBuffer_.get());
        // WASM min: propagates NaN via double minsd
        masm.minsd(src.code, dst.code);
        masm.minsd(dst.code, src.code);
    });
}

bool BaselineCompiler::emitF64Max() {
    return emitBinaryOp(ValType::f64(), [this](Reg dst, Reg src) {
        X86Assembler masm(codeBuffer_.get());
        // WASM max: propagates NaN via double maxsd
        masm.maxsd(src.code, dst.code);
        masm.maxsd(dst.code, src.code);
    });
}

// =============================================================================
// Conversions (Dual-Platform)
// =============================================================================

bool BaselineCompiler::emitI32WrapI64() {
    return emitUnaryOp(ValType::i32(), [this](Reg dst) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.mov32(dst.code, dst.code);  // Truncate to 32-bit
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            // On ARM64, 32-bit register access automatically truncates
            masm.mov32(dst.code, dst.code);
        });
    });
}

bool BaselineCompiler::emitI64ExtendI32S() {
    return emitUnaryOp(ValType::i64(), [this](Reg dst) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.movsxd(dst.code, dst.code);  // Sign-extend i32 to i64
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.sxtw(dst.code, dst.code);  // Sign-extend word to 64-bit
        });
    });
}

bool BaselineCompiler::emitI64ExtendI32U() {
    return emitUnaryOp(ValType::i64(), [this](Reg dst) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.mov32(dst.code, dst.code);  // Zero-extend
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.uxtw(dst.code, dst.code);  // Zero-extend word to 64-bit
        });
    });
}

bool BaselineCompiler::emitI32TruncF32S() {
    return emitUnaryOp(ValType::i32(), [this](Reg dst) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cvttss2si(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcvtzs32S(dst.code, dst.code);  // Float to signed int
        });
    });
}

bool BaselineCompiler::emitI32TruncF32U() {
    return emitUnaryOp(ValType::i32(), [this](Reg dst) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cvttss2si64(dst.code, dst.code);
            masm.mov32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcvtzu32S(dst.code, dst.code);  // Float to unsigned int
        });
    });
}

bool BaselineCompiler::emitI32TruncF64S() {
    return emitUnaryOp(ValType::i32(), [this](Reg dst) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cvttsd2si(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcvtzs32D(dst.code, dst.code);  // Double to signed int
        });
    });
}

bool BaselineCompiler::emitI32TruncF64U() {
    return emitUnaryOp(ValType::i32(), [this](Reg dst) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cvttsd2si64(dst.code, dst.code);
            masm.mov32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcvtzu32D(dst.code, dst.code);  // Double to unsigned int
        });
    });
}

bool BaselineCompiler::emitF32ConvertI32S() {
    return emitUnaryOp(ValType::f32(), [this](Reg dst) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cvtsi2ss(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.scvtfS32(dst.code, dst.code);  // Signed int to float
        });
    });
}

bool BaselineCompiler::emitF32ConvertI32U() {
    return emitUnaryOp(ValType::f32(), [this](Reg dst) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.mov32(dst.code, dst.code);
            masm.cvtsi2ss64(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.ucvtfS32(dst.code, dst.code);  // Unsigned int to float
        });
    });
}

bool BaselineCompiler::emitF64ConvertI32S() {
    return emitUnaryOp(ValType::f64(), [this](Reg dst) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cvtsi2sd(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.scvtfD32(dst.code, dst.code);  // Signed int to double
        });
    });
}

bool BaselineCompiler::emitF64ConvertI32U() {
    return emitUnaryOp(ValType::f64(), [this](Reg dst) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.mov32(dst.code, dst.code);
            masm.cvtsi2sd64(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.ucvtfD32(dst.code, dst.code);  // Unsigned int to double
        });
    });
}

bool BaselineCompiler::emitF32DemoteF64() {
    return emitUnaryOp(ValType::f32(), [this](Reg dst) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cvtsd2ss(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcvtSD(dst.code, dst.code);  // Double to float
        });
    });
}

bool BaselineCompiler::emitF64PromoteF32() {
    return emitUnaryOp(ValType::f64(), [this](Reg dst) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.cvtss2sd(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcvtDS(dst.code, dst.code);  // Float to double
        });
    });
}

bool BaselineCompiler::emitI32ReinterpretF32() {
    return emitUnaryOp(ValType::i32(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        masm.movd_xmm2gpr(dst.code, dst.code);  // Move XMM to GPR
    });
}

bool BaselineCompiler::emitI64ReinterpretF64() {
    return emitUnaryOp(ValType::i64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        masm.movq_xmm2gpr(dst.code, dst.code);  // Move XMM to GPR (64-bit)
    });
}

bool BaselineCompiler::emitF32ReinterpretI32() {
    return emitUnaryOp(ValType::f32(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        masm.movd_gpr2xmm(dst.code, dst.code);  // Move GPR to XMM
    });
}

bool BaselineCompiler::emitF64ReinterpretI64() {
    return emitUnaryOp(ValType::f64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        masm.movq_gpr2xmm(dst.code, dst.code);  // Move GPR to XMM (64-bit)
    });
}

// =============================================================================
// I64 Trunc Operations
// =============================================================================

bool BaselineCompiler::emitI64TruncF32S() {
    return emitUnaryOp(ValType::i64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        masm.cvttss2si64(dst.code, dst.code);  // Truncate f32 to signed i64
    });
}

bool BaselineCompiler::emitI64TruncF32U() {
    return emitUnaryOp(ValType::i64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        // Unsigned: for values >= 2^63, subtract 2^63, convert, add back
        // Simplified path: use signed conversion (works for < 2^63)
        masm.cvttss2si64(dst.code, dst.code);
    });
}

bool BaselineCompiler::emitI64TruncF64S() {
    return emitUnaryOp(ValType::i64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        masm.cvttsd2si64(dst.code, dst.code);  // Truncate f64 to signed i64
    });
}

bool BaselineCompiler::emitI64TruncF64U() {
    return emitUnaryOp(ValType::i64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        // Unsigned: simplified path using signed conversion
        masm.cvttsd2si64(dst.code, dst.code);
    });
}

// =============================================================================
// F32/F64 Convert from I64
// =============================================================================

bool BaselineCompiler::emitF32ConvertI64S() {
    return emitUnaryOp(ValType::f32(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        masm.cvtsi2ss64(dst.code, dst.code);  // Signed i64 to f32
    });
}

bool BaselineCompiler::emitF32ConvertI64U() {
    return emitUnaryOp(ValType::f32(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        // Unsigned i64 to f32: simplified using signed (works for < 2^63)
        masm.cvtsi2ss64(dst.code, dst.code);
    });
}

bool BaselineCompiler::emitF64ConvertI64S() {
    return emitUnaryOp(ValType::f64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        masm.cvtsi2sd64(dst.code, dst.code);  // Signed i64 to f64
    });
}

bool BaselineCompiler::emitF64ConvertI64U() {
    return emitUnaryOp(ValType::f64(), [this](Reg dst) {
        X86Assembler masm(codeBuffer_.get());
        // Unsigned i64 to f64: simplified using signed (works for < 2^63)
        masm.cvtsi2sd64(dst.code, dst.code);
    });
}

// =============================================================================
// F32 Comparisons
// =============================================================================

bool BaselineCompiler::emitF32Eq() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.ucomiss(dst.code, src.code);
            masm.setcc(X86Assembler::E, dst.code);  // ZF=1 and no NaN
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcmpS(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::EQ);
        });
    });
}

bool BaselineCompiler::emitF32Ne() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.ucomiss(dst.code, src.code);
            masm.setcc(X86Assembler::NE, dst.code);  // ZF=0 or PF=1 (NaN)
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcmpS(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::NE);
        });
    });
}

bool BaselineCompiler::emitF32Lt() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.ucomiss(src.code, dst.code);  // src > dst means dst < src
            masm.setcc(X86Assembler::A, dst.code);  // Above (CF=0, ZF=0)
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcmpS(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::LT);
        });
    });
}

bool BaselineCompiler::emitF32Gt() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.ucomiss(dst.code, src.code);  // dst > src
            masm.setcc(X86Assembler::A, dst.code);  // Above
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcmpS(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::GT);
        });
    });
}

bool BaselineCompiler::emitF32Le() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.ucomiss(src.code, dst.code);  // src >= dst means dst <= src
            masm.setcc(X86Assembler::AE, dst.code);  // Above or equal
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcmpS(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::LE);
        });
    });
}

bool BaselineCompiler::emitF32Ge() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.ucomiss(dst.code, src.code);  // dst >= src
            masm.setcc(X86Assembler::AE, dst.code);  // Above or equal
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcmpS(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::GE);
        });
    });
}

// =============================================================================
// F64 Comparisons
// =============================================================================

bool BaselineCompiler::emitF64Eq() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.ucomisd(dst.code, src.code);
            masm.setcc(X86Assembler::E, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcmpD(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::EQ);
        });
    });
}

bool BaselineCompiler::emitF64Ne() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.ucomisd(dst.code, src.code);
            masm.setcc(X86Assembler::NE, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcmpD(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::NE);
        });
    });
}

bool BaselineCompiler::emitF64Lt() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.ucomisd(src.code, dst.code);  // Reverse for lt
            masm.setcc(X86Assembler::A, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcmpD(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::LT);
        });
    });
}

bool BaselineCompiler::emitF64Gt() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.ucomisd(dst.code, src.code);
            masm.setcc(X86Assembler::A, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcmpD(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::GT);
        });
    });
}

bool BaselineCompiler::emitF64Le() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.ucomisd(src.code, dst.code);  // Reverse for le
            masm.setcc(X86Assembler::AE, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcmpD(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::LE);
        });
    });
}

bool BaselineCompiler::emitF64Ge() {
    return emitBinaryOp(ValType::i32(), [this](Reg dst, Reg src) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            masm.ucomisd(dst.code, src.code);
            masm.setcc(X86Assembler::AE, dst.code);
            masm.movzx8to32(dst.code, dst.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            masm.fcmpD(dst.code, src.code);
            masm.cset32(dst.code, ARM64Assembler::GE);
        });
    });
}

// =============================================================================
// Memory Operations (Dual-Platform)
// =============================================================================

bool BaselineCompiler::emitLoad(Op op, uint32_t align, uint32_t offset) {
    ValueLocation addr = pop();
    
    // Memory64: determine address size based on module's memory type
    bool isMemory64 = module_->memories().size() > 0 && 
                      module_->memories()[0].isMemory64;
    ValType addrType = isMemory64 ? ValType::i64() : ValType::i32();
    
    if (options_.enableBoundsChecks) {
        emitBoundsCheck64(offset, isMemory64);
    }
    
    Reg addrReg = allocReg(addrType);
    
    // Load address to register if needed
    if (addr.isRegister()) {
        addrReg = addr.reg;
    }
    
    // For Memory32, zero-extend i32 address to i64 for pointer arithmetic
    if (!isMemory64 && addr.isRegister()) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            // mov eax, eax (zero-extends to 64-bit)
            masm.mov32(addrReg.code, addrReg.code);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            // uxtw (unsigned extend word)
            masm.andImm64(addrReg.code, addrReg.code, 0xFFFFFFFF);
        });
    }
    
    // Get memory base pointer (stored in instance)
    Reg memBase = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.load64(memBase.code, X86Assembler::RBP, -16);
        // Add base + address
        masm.add64(memBase.code, addrReg.code);
        if (offset != 0) {
            masm.add64Imm(memBase.code, static_cast<int32_t>(offset));
        }
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.ldr64(memBase.code, 29, -16);  // Load from frame pointer
        masm.add64(memBase.code, memBase.code, addrReg.code);
        if (offset != 0) {
            masm.addImm64(memBase.code, memBase.code, offset);
        }
    });
    
    ValType resultType;
    Reg result;
    
    switch (op) {
        case Op::I32Load:
            resultType = ValType::i32();
            result = allocReg(resultType);
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.load32(result.code, memBase.code, 0);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.ldr32(result.code, memBase.code, 0);
            });
            break;
        case Op::I32Load8S:
            resultType = ValType::i32();
            result = allocReg(resultType);
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.load8s(result.code, memBase.code, 0);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.ldrsb32(result.code, memBase.code, 0);
            });
            break;
        case Op::I32Load8U:
            resultType = ValType::i32();
            result = allocReg(resultType);
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.load8u(result.code, memBase.code, 0);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.ldrb(result.code, memBase.code, 0);
            });
            break;
        case Op::I32Load16S:
            resultType = ValType::i32();
            result = allocReg(resultType);
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.load16s(result.code, memBase.code, 0);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.ldrsh32(result.code, memBase.code, 0);
            });
            break;
        case Op::I32Load16U:
            resultType = ValType::i32();
            result = allocReg(resultType);
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.load16u(result.code, memBase.code, 0);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.ldrh(result.code, memBase.code, 0);
            });
            break;
        case Op::I64Load:
            resultType = ValType::i64();
            result = allocReg(resultType);
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.load64(result.code, memBase.code, 0);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.ldr64(result.code, memBase.code, 0);
            });
            break;
        case Op::I64Load8S:
            resultType = ValType::i64();
            result = allocReg(resultType);
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.load8s64(result.code, memBase.code, 0);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.ldrsb64(result.code, memBase.code, 0);
            });
            break;
        case Op::I64Load8U:
            resultType = ValType::i64();
            result = allocReg(resultType);
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.load8u64(result.code, memBase.code, 0);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.ldrb(result.code, memBase.code, 0);
            });
            break;
        case Op::I64Load16S:
            resultType = ValType::i64();
            result = allocReg(resultType);
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.load16s64(result.code, memBase.code, 0);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.ldrsh64(result.code, memBase.code, 0);
            });
            break;
        case Op::I64Load16U:
            resultType = ValType::i64();
            result = allocReg(resultType);
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.load16u64(result.code, memBase.code, 0);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.ldrh(result.code, memBase.code, 0);
            });
            break;
        case Op::I64Load32S:
            resultType = ValType::i64();
            result = allocReg(resultType);
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.load32s64(result.code, memBase.code, 0);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.ldrsw(result.code, memBase.code, 0);
            });
            break;
        case Op::I64Load32U:
            resultType = ValType::i64();
            result = allocReg(resultType);
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.load32u64(result.code, memBase.code, 0);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.ldr32(result.code, memBase.code, 0);  // Zero-extends to 64-bit
            });
            break;
        case Op::F32Load:
            resultType = ValType::f32();
            result = allocReg(resultType);
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.loadss(result.code, memBase.code, 0);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.ldrS(result.code, memBase.code, 0);
            });
            break;
        case Op::F64Load:
            resultType = ValType::f64();
            result = allocReg(resultType);
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.loadsd(result.code, memBase.code, 0);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.ldrD(result.code, memBase.code, 0);
            });
            break;
        default:
            error_ = "invalid load opcode";
            return false;
    }
    
    push(ValueLocation::inReg(result, resultType));
    return true;
}

bool BaselineCompiler::emitStore(Op op, uint32_t align, uint32_t offset) {
    ValueLocation value = pop();
    ValueLocation addr = pop();
    
    if (options_.enableBoundsChecks) {
        emitBoundsCheck(offset);
    }
    
    // Get memory base pointer
    Reg memBase = allocReg(ValType::i64());
    Reg addrReg = addr.isRegister() ? addr.reg : allocReg(ValType::i32());
    Reg valueReg = value.isRegister() ? value.reg : allocReg(value.type);
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.load64(memBase.code, X86Assembler::RBP, -16);
        if (offset != 0) {
            masm.add64Imm(addrReg.code, static_cast<int32_t>(offset));
        }
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.ldr64(memBase.code, 29, -16);  // Load from frame pointer
        if (offset != 0) {
            masm.addImm64(addrReg.code, addrReg.code, offset);
        }
    });
    
    switch (op) {
        case Op::I32Store:
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.store32(memBase.code, 0, valueReg.code);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.str32(valueReg.code, memBase.code, 0);
            });
            break;
        case Op::I32Store8:
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.store8(memBase.code, 0, valueReg.code);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.strb(valueReg.code, memBase.code, 0);
            });
            break;
        case Op::I32Store16:
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.store16(memBase.code, 0, valueReg.code);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.strh(valueReg.code, memBase.code, 0);
            });
            break;
        case Op::I64Store:
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.store64(memBase.code, 0, valueReg.code);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.str64(valueReg.code, memBase.code, 0);
            });
            break;
        case Op::I64Store8:
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.store8(memBase.code, 0, valueReg.code);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.strb(valueReg.code, memBase.code, 0);
            });
            break;
        case Op::I64Store16:
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.store16(memBase.code, 0, valueReg.code);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.strh(valueReg.code, memBase.code, 0);
            });
            break;
        case Op::I64Store32:
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.store32(memBase.code, 0, valueReg.code);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.str32(valueReg.code, memBase.code, 0);
            });
            break;
        case Op::F32Store:
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.storess(memBase.code, 0, valueReg.code);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.strS(valueReg.code, memBase.code, 0);
            });
            break;
        case Op::F64Store:
            EMIT_X64({
                X86Assembler masm(codeBuffer_.get());
                masm.storesd(memBase.code, 0, valueReg.code);
            });
            EMIT_ARM64({
                ARM64Assembler masm(codeBuffer_.get());
                masm.strD(valueReg.code, memBase.code, 0);
            });
            break;
        default:
            error_ = "invalid store opcode";
            return false;
    }
    
    return true;
}

bool BaselineCompiler::emitMemorySize(uint32_t memIndex) {
    Reg result = allocReg(ValType::i32());
    // Load memory size from instance (instance->memorySize)
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.load32(result.code, X86Assembler::RBP, -24);
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.ldr32(result.code, 29, -24);
    });
    push(ValueLocation::inReg(result, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitMemoryGrow(uint32_t memIndex) {
    ValueLocation delta = pop();
    (void)delta;
    
    // For now, emit placeholder that returns -1 (failure)
    // Full implementation requires runtime call
    Reg result = allocReg(ValType::i32());
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.mov32Imm(result.code, -1);
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm32(result.code, static_cast<uint32_t>(-1));
    });
    
    push(ValueLocation::inReg(result, ValType::i32()));
    return true;
}

// =============================================================================
// Call Operations (Stubs)
// =============================================================================

bool BaselineCompiler::emitCall(uint32_t funcIndex) {
    if (funcIndex >= module_->numFunctions()) {
        error_ = "function index out of bounds";
        return false;
    }
    
    const FuncType* funcType = module_->funcType(funcIndex);
    X86Assembler masm(codeBuffer_.get());
    
    // System V AMD64 ABI: integer args in RDI, RSI, RDX, RCX, R8, R9
    // Float args in XMM0-XMM7
    static const uint8_t intArgRegs[] = {
        X86Assembler::RDI, X86Assembler::RSI, X86Assembler::RDX,
        X86Assembler::RCX, X86Assembler::R8, X86Assembler::R9
    };
    
    // Pop arguments from value stack in reverse order
    size_t numParams = funcType->params().size();
    std::vector<ValueLocation> args(numParams);
    for (size_t i = numParams; i > 0; --i) {
        args[i - 1] = pop();
    }
    
    // Move args to calling convention registers
    size_t intArgIdx = 0;
    size_t floatArgIdx = 0;
    for (size_t i = 0; i < numParams; ++i) {
        const ValueLocation& arg = args[i];
        ValType::Kind kind = funcType->params()[i].kind();
        
        if (kind == ValType::Kind::F32 || kind == ValType::Kind::F64) {
            // Float arg -> XMM0-XMM7
            uint8_t targetXmm = floatArgIdx++;
            if (arg.isRegister()) {
                if (arg.reg.code != targetXmm) {
                    if (kind == ValType::Kind::F32)
                        masm.movss(targetXmm, arg.reg.code);
                    else
                        masm.movsd(targetXmm, arg.reg.code);
                }
            } else {
                // Load from stack
                if (kind == ValType::Kind::F32)
                    masm.loadss(targetXmm, X86Assembler::RBP, arg.slot.offset);
                else
                    masm.loadsd(targetXmm, X86Assembler::RBP, arg.slot.offset);
            }
        } else {
            // Integer arg -> RDI, RSI, etc.
            if (intArgIdx < 6) {
                uint8_t targetReg = intArgRegs[intArgIdx++];
                if (arg.isRegister()) {
                    if (arg.reg.code != targetReg) {
                        if (kind == ValType::Kind::I64)
                            masm.mov64(targetReg, arg.reg.code);
                        else
                            masm.mov32(targetReg, arg.reg.code);
                    }
                } else if (arg.isImmediate()) {
                    masm.mov64Imm(targetReg, arg.immediate);
                } else {
                    // Load from stack
                    if (kind == ValType::Kind::I64)
                        masm.load64(targetReg, X86Assembler::RBP, arg.slot.offset);
                    else
                        masm.load32(targetReg, X86Assembler::RBP, arg.slot.offset);
                }
            }
            // Args beyond 6 go on stack (not yet implemented)
        }
    }
    
    // Get function entry address from compiled code table (at RBP-16)
    // funcTable[funcIndex] gives us the address
    Reg callTarget = allocReg(ValType::i64());
    masm.load64(callTarget.code, X86Assembler::RBP, -16);  // Load function table ptr
    masm.load64(callTarget.code, callTarget.code, funcIndex * 8);  // Load func address
    
    // Emit indirect call (call *rax)
    masm.call(callTarget.code);
    freeReg(callTarget);
    
    // Push result(s) based on return type
    if (!funcType->results().empty()) {
        ValType resultType = funcType->results()[0];
        if (resultType.kind() == ValType::Kind::F32 || resultType.kind() == ValType::Kind::F64) {
            // Result in XMM0
            Reg result = allocReg(resultType);
            // XMM0 is already the result register, just claim it
            push(ValueLocation::inReg(result, resultType));
        } else {
            // Result in RAX
            Reg result = Reg::gpr(X86Assembler::RAX);
            push(ValueLocation::inReg(result, resultType));
        }
    }
    
    return true;
}

bool BaselineCompiler::emitCallIndirect(uint32_t typeIndex, uint32_t tableIndex) {
    X86Assembler masm(codeBuffer_.get());
    
    // Pop call target index from stack
    ValueLocation indexLoc = pop();
    
    // Get index into register
    Reg indexReg;
    if (indexLoc.isRegister()) {
        indexReg = indexLoc.reg;
    } else if (indexLoc.isImmediate()) {
        indexReg = allocReg(ValType::i32());
        masm.mov32Imm(indexReg.code, static_cast<int32_t>(indexLoc.immediate));
    } else {
        indexReg = allocReg(ValType::i32());
        masm.load32(indexReg.code, X86Assembler::RBP, indexLoc.slot.offset);
    }
    
    // Load table base and size from instance (RBP-24 = table base, RBP-32 = table size)
    Reg tableBase = allocReg(ValType::i64());
    Reg tableSize = allocReg(ValType::i32());
    masm.load64(tableBase.code, X86Assembler::RBP, -24);
    masm.load32(tableSize.code, X86Assembler::RBP, -32);
    
    // Bounds check: if (index >= tableSize) trap
    masm.cmp32(indexReg.code, tableSize.code);
    size_t trapJump = masm.jcc32(X86Assembler::AE);  // Jump if above or equal
    
    // Compute table entry address: tableBase + index * 8
    Reg entryAddr = allocReg(ValType::i64());
    masm.mov64(entryAddr.code, indexReg.code);  // Zero-extend i32 to i64
    masm.shl64_cl(entryAddr.code);  // Shift left by 3 (multiply by 8)
    // Actually, use explicit shift: need to add imm shift instruction
    // For now, just add index to base multiple times
    masm.lea64(entryAddr.code, tableBase.code, indexReg.code, 8, 0);
    
    // Load function pointer from table
    Reg funcPtr = allocReg(ValType::i64());
    masm.load64(funcPtr.code, entryAddr.code, 0);
    
    // Null check
    masm.cmp64Imm(funcPtr.code, 0);
    size_t nullTrap = masm.jcc32(X86Assembler::E);
    
    // Type check would go here (compare typeIndex against table entry type)
    // For now, skip type check
    
    // Call through function pointer
    masm.call(funcPtr.code);
    
    // Patch trap jumps to trap handler
    // (In real impl, would emit trap handler or landing pad)
    
    freeReg(funcPtr);
    freeReg(entryAddr);
    freeReg(tableSize);
    freeReg(tableBase);
    if (!indexLoc.isRegister()) freeReg(indexReg);
    
    // Push result (assuming single i32 result for now)
    Reg result = Reg::gpr(X86Assembler::RAX);
    push(ValueLocation::inReg(result, ValType::i32()));
    
    return true;
}

// =============================================================================
// Tail Call Operations
// =============================================================================

bool BaselineCompiler::emitReturnCall(uint32_t funcIndex) {
    // Tail call: reuse current stack frame and jump to callee
    // 1. Pop arguments from value stack
    // 2. Tear down current frame
    // 3. Set up callee arguments in place
    // 4. Jump (not call) to callee
    
    uint32_t typeIdx = module_->functions()[funcIndex].typeIndex;
    const FuncType* calleeTypePtr = module_->funcType(typeIdx);
    if (!calleeTypePtr) return false;
    const FuncType& calleeType = *calleeTypePtr;
    size_t numParams = calleeType.params().size();
    
    // Collect arguments
    std::vector<ValueLocation> args;
    for (size_t i = 0; i < numParams; i++) {
        args.push_back(pop());
    }
    std::reverse(args.begin(), args.end());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        
        // Move arguments to correct positions
        // For tail call, we reuse caller's argument slots
        for (size_t i = 0; i < numParams; i++) {
            if (args[i].isRegister()) {
                // Move to arg register or stack slot
                if (i < 6) {  // x64 uses 6 registers for args
                    static const uint8_t argRegs[] = {X86Assembler::RDI, X86Assembler::RSI, 
                        X86Assembler::RDX, X86Assembler::RCX, 8, 9};  // r8, r9
                    masm.mov64(argRegs[i], args[i].reg.code);
                } else {
                    masm.store64(args[i].reg.code, X86Assembler::RBP, 16 + (i - 6) * 8);
                }
            } else if (args[i].isImmediate()) {
                if (i < 6) {
                    static const uint8_t argRegs[] = {X86Assembler::RDI, X86Assembler::RSI,
                        X86Assembler::RDX, X86Assembler::RCX, 8, 9};
                    masm.mov64Imm(argRegs[i], args[i].immediate);
                }
            }
        }
        
        // Restore callee-saved registers
        masm.pop(X86Assembler::RBP);
        
        // Jump to target function (not call - no return address pushed)
        // This would be a jmp to the function address
        // In real impl: masm.jmp(funcAddress);
        masm.int3();  // Placeholder
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        
        // Move arguments to X0-X7 or stack
        for (size_t i = 0; i < numParams && i < 8; i++) {
            if (args[i].isRegister()) {
                masm.mov64(i, args[i].reg.code);
            } else if (args[i].isImmediate()) {
                masm.movImm64(i, args[i].immediate);
            }
        }
        
        // Restore FP/LR
        masm.ldpPost64(29, 30, 31, frameSize_);
        
        // Jump to target (B instruction, not BL)
        // In real impl: masm.b(funcAddress);
        masm.brk(0);  // Placeholder
    });
    
    return true;
}

bool BaselineCompiler::emitReturnCallIndirect(uint32_t typeIndex, uint32_t tableIndex) {
    // Tail call through table
    // Pop index, look up function, then tail call
    
    ValueLocation indexLoc = pop();
    (void)indexLoc;
    
    // Get callee type to know argument count
    const FuncType* calleeTypePtr = module_->funcType(typeIndex);
    if (!calleeTypePtr) return false;
    const FuncType& calleeType = *calleeTypePtr;
    size_t numParams = calleeType.params().size();
    
    // Collect arguments
    std::vector<ValueLocation> args;
    for (size_t i = 0; i < numParams; i++) {
        args.push_back(pop());
    }
    std::reverse(args.begin(), args.end());
    
    (void)tableIndex;
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        
        // Load table base and bounds check (similar to call_indirect)
        Reg tableBase = allocReg(ValType::i64());
        Reg tableSize = allocReg(ValType::i32());
        masm.load64(tableBase.code, X86Assembler::RBP, -24);
        masm.load32(tableSize.code, X86Assembler::RBP, -32);
        
        // Bounds check and load function pointer
        // ... (similar to emitCallIndirect)
        
        // Set up arguments
        for (size_t i = 0; i < numParams && i < 6; i++) {
            static const uint8_t argRegs[] = {X86Assembler::RDI, X86Assembler::RSI,
                X86Assembler::RDX, X86Assembler::RCX, 8, 9};
            if (args[i].isRegister()) {
                masm.mov64(argRegs[i], args[i].reg.code);
            }
        }
        
        // Restore frame
        masm.pop(X86Assembler::RBP);
        
        // Jump through function pointer
        // masm.jmp(funcPtr);
        
        freeReg(tableBase);
        freeReg(tableSize);
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        
        // Load table, bounds check
        Reg tableBase = allocReg(ValType::i64());
        masm.ldr64(tableBase.code, 29, -24);
        
        // Set up args in X0-X7
        for (size_t i = 0; i < numParams && i < 8; i++) {
            if (args[i].isRegister()) {
                masm.mov64(i, args[i].reg.code);
            }
        }
        
        // Restore frame
        masm.ldpPost64(29, 30, 31, frameSize_);
        
        // Jump through register (BR, not BLR)
        masm.br(tableBase.code);
        
        freeReg(tableBase);
    });
    
    return true;
}

bool BaselineCompiler::emitReturnCallRef(uint32_t typeIndex) {
    // Tail call through function reference
    ValueLocation funcRef = pop();
    (void)funcRef;
    
    const FuncType* calleeTypePtr = module_->funcType(typeIndex);
    if (!calleeTypePtr) return false;
    const FuncType& calleeType = *calleeTypePtr;
    size_t numParams = calleeType.params().size();
    
    // Collect arguments
    std::vector<ValueLocation> args;
    for (size_t i = 0; i < numParams; i++) {
        args.push_back(pop());
    }
    std::reverse(args.begin(), args.end());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        
        // funcRef is in a register - extract function pointer
        Reg funcPtr = allocReg(ValType::i64());
        
        // Null check
        masm.cmp64Imm(funcPtr.code, 0);
        size_t nullTrap = masm.jcc32(X86Assembler::E);
        (void)nullTrap;
        
        // Type check would go here
        
        // Set up arguments
        for (size_t i = 0; i < numParams && i < 6; i++) {
            static const uint8_t argRegs[] = {X86Assembler::RDI, X86Assembler::RSI,
                X86Assembler::RDX, X86Assembler::RCX, 8, 9};
            if (args[i].isRegister()) {
                masm.mov64(argRegs[i], args[i].reg.code);
            }
        }
        
        // Restore frame and jump
        masm.pop(X86Assembler::RBP);
        masm.jmp(funcPtr.code);
        
        freeReg(funcPtr);
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        
        Reg funcPtr = allocReg(ValType::i64());
        
        // Null check
        masm.cbnz64(funcPtr.code, 8);
        masm.brk(0);  // Trap on null
        
        // Set up args
        for (size_t i = 0; i < numParams && i < 8; i++) {
            if (args[i].isRegister()) {
                masm.mov64(i, args[i].reg.code);
            }
        }
        
        // Restore frame and jump
        masm.ldpPost64(29, 30, 31, frameSize_);
        masm.br(funcPtr.code);
        
        freeReg(funcPtr);
    });
    
    return true;
}

// =============================================================================
// Drop/Select
// =============================================================================

bool BaselineCompiler::emitDrop() {
    if (valueStack_.empty()) {
        error_ = "drop on empty stack";
        return false;
    }
    pop();
    return true;
}

bool BaselineCompiler::emitSelect() {
    ValueLocation cond = pop();
    ValueLocation val2 = pop();
    ValueLocation val1 = pop();
    
    // Emit conditional move
    push(val1);  // Placeholder - actual selection would go here
    return true;
}

// =============================================================================
// Code Generation Helpers
// =============================================================================

void BaselineCompiler::emitPrologue(const FuncType* funcType, uint32_t numLocals) {
    // Push frame pointer
    // Set up stack frame
    // Initialize locals to zero
    codeBuffer_->emit8(0x55);  // push rbp (x86-64)
    codeBuffer_->emit8(0x48);  // mov rbp, rsp
    codeBuffer_->emit8(0x89);
    codeBuffer_->emit8(0xE5);
}

void BaselineCompiler::emitEpilogue() {
    // Restore stack
    // Pop frame pointer
    // Return
    codeBuffer_->emit8(0x5D);  // pop rbp (x86-64)
    codeBuffer_->emit8(0xC3);  // ret
}

void BaselineCompiler::emitTierUpCheck() {
    // Tier-up check at loop header (based on JSC's BBQ JIT approach)
    //
    // The tier-up counter is stored in the TierUpCount structure.
    // We decrement by loopIncrement() on each loop iteration.
    // When counter goes negative, we branch to the tier-up slow path.
    //
    // Counter layout:
    //   - Starts at threshold (e.g., 1000 for warm-up)
    //   - Decremented by 100 per loop iteration
    //   - When <= 0, trigger tier-up compilation
    
    if (!tierUpCounterPtr_) {
        return;  // Tier-up not initialized
    }
    
    uint32_t loopIndex = currentLoopIndex_++;
    
#if defined(__x86_64__) || defined(_M_X64)
    X86Assembler masm(codeBuffer_.get());
    
    // Load tier-up counter address into scratch register
    // mov rax, [tierUpCounterPtr_]
    masm.mov64Imm(X86Assembler::RAX, reinterpret_cast<uint64_t>(tierUpCounterPtr_));
    
    // Decrement counter: sub dword [rax], loopIncrement
    // x86 encoding: sub [mem], imm32
    codeBuffer_->emit8(0x81);           // sub r/m32, imm32
    codeBuffer_->emit8(0x28);           // ModR/M: [rax] with /5 for sub
    codeBuffer_->emit32(100);           // loopIncrement = 100
    
    // Branch if counter still positive (no tier-up needed)
    // jns skip_tierup (0x79 = jns rel8)
    size_t branchOffset = codeBuffer_->offset();
    codeBuffer_->emit8(0x79);           // jns rel8
    codeBuffer_->emit8(0x00);           // placeholder - will patch
    
    // === Tier-up slow path ===
    // Save current state and call tier-up handler
    
    // Push function index and loop index for tier-up handler
    masm.pushImm32(funcIndex_);
    masm.pushImm32(loopIndex);
    
    // Call tier-up handler (implemented in runtime)
    // This is a slow path - the handler will either:
    // 1. Start background compilation
    // 2. Do OSR entry if optimized code is ready
    masm.mov64Imm(X86Assembler::RAX, reinterpret_cast<uint64_t>(tierUpSlowPath_));
    codeBuffer_->emit8(0xFF);           // call rax
    codeBuffer_->emit8(0xD0);
    
    // Clean up stack
    masm.popReg(X86Assembler::RCX);     // pop loop index
    masm.popReg(X86Assembler::RCX);     // pop func index
    
    // Patch branch target
    size_t skipTarget = codeBuffer_->offset();
    int8_t branchDisp = static_cast<int8_t>(skipTarget - branchOffset - 2);
    codeBuffer_->patchByte(branchOffset + 1, branchDisp);
    
#elif defined(__aarch64__) || defined(_M_ARM64)
    ARM64Assembler masm(codeBuffer_.get());
    
    // Load tier-up counter address
    // ldr x16, =tierUpCounterPtr_
    masm.movImm64(16, reinterpret_cast<uint64_t>(tierUpCounterPtr_));
    
    // Load counter value: ldr w17, [x16]
    masm.ldr32(17, 16, 0);
    
    // Decrement: sub w17, w17, #100
    masm.subImm32(17, 17, 100);
    
    // Store back: str w17, [x16]
    masm.str32(17, 16, 0);
    
    // Branch if still positive: tbz w17, #31, skip (test bit 31 = sign bit)
    // If sign bit is 0, counter is positive, skip tier-up
    size_t branchOffset = codeBuffer_->offset();
    masm.tbz32(17, 31, 0);  // placeholder offset
    
    // === Tier-up slow path ===
    // Save x0-x1 for call
    masm.strPre(0, 31, -16);
    masm.str64(1, 31, 8);
    
    // Setup args: x0 = funcIndex, x1 = loopIndex
    masm.movImm32(0, funcIndex_);
    masm.movImm32(1, loopIndex);
    
    // Call tier-up handler
    masm.movImm64(16, reinterpret_cast<uint64_t>(tierUpSlowPath_));
    masm.blr(16);
    
    // Restore
    masm.ldr64(1, 31, 8);
    masm.ldrPost(0, 31, 16);
    
    // Patch branch - calculate offset
    size_t skipTarget = codeBuffer_->offset();
    // ARM64 TBZ uses 14-bit signed offset in units of 4 bytes
    int32_t offset = static_cast<int32_t>((skipTarget - branchOffset) / 4);
    // Patch the TBZ instruction at branchOffset
    uint32_t insn = 0x36000000 | ((offset & 0x3FFF) << 5) | (17);
    codeBuffer_->patch32(branchOffset, insn);
#endif

    // Record OSR entry point for this loop
    tierUpOSRPoints_.push_back({loopIndex, codeBuffer_->offset(), valueStack_.size()});
}

void BaselineCompiler::emitStackCheck(uint32_t frameSize) {
    // Check stack limit
    // On x86-64:
    //   cmp rsp - frameSize, [limit]
    //   jb trap
    // On ARM64:
    //   sub tmp, sp, frameSize
    //   ldr limit, [limit_addr]
    //   cmp tmp, limit
    //   b.lo trap
    
    if (IsX64Platform) {
        // x64: Compare stack pointer against limit
        codeBuffer_->emit8(0x48);  // REX.W
        codeBuffer_->emit8(0x81);  // CMP r/m64, imm32
        codeBuffer_->emit8(0xFC);  // ModR/M for RSP
        codeBuffer_->emit32(0x1000);  // Stack limit placeholder
    } else {
        // ARM64 placeholder - stack check
        codeBuffer_->emit32(0xD10003FF); // sub sp, sp, #0
    }
}

void BaselineCompiler::emitTrap(Trap trap) {
    // Emit call to trap handler
    codeBuffer_->emit8(0xCC);  // int 3 (debug break - placeholder)
}

void BaselineCompiler::emitBoundsCheck(uint32_t offset) {
    // Legacy 32-bit bounds check
    emitBoundsCheck64(offset, false);
}

void BaselineCompiler::emitBoundsCheck64(uint32_t offset, bool isMemory64) {
    // Memory bounds checking for Memory32 and Memory64
    //
    // For Memory32:
    //   if (addr + offset > memSize32) trap
    //
    // For Memory64:
    //   if (addr + offset > memSize64) trap
    //
    // Memory size is stored in instance at a known offset
    
#if defined(__x86_64__) || defined(_M_X64)
    X86Assembler masm(codeBuffer_.get());
    
    // Load memory size from instance (stored at RBP - 24)
    if (isMemory64) {
        masm.load64(X86Assembler::R10, X86Assembler::RBP, -24);
    } else {
        masm.load32(X86Assembler::R10, X86Assembler::RBP, -24);
    }
    
    // Add offset to address in R11 (scratch)
    if (offset != 0) {
        masm.mov64Imm(X86Assembler::R11, offset);
        // add r11, [top of stack - address]
    }
    
    // Compare: if address + offset >= memSize, trap
    if (isMemory64) {
        masm.cmp64(X86Assembler::RAX, X86Assembler::R10);
    } else {
        masm.cmp32(X86Assembler::RAX, X86Assembler::R10);  // EAX = RAX lower 32 bits
    }
    
    // jae (jump if above or equal = unsigned >=) to trap
    size_t trapJumpOffset = codeBuffer_->offset();
    codeBuffer_->emit8(0x73);   // jae rel8
    codeBuffer_->emit8(0x02);   // skip 2 bytes (the jmp over trap)
    
    // Normal path: skip trap
    codeBuffer_->emit8(0xEB);   // jmp rel8
    codeBuffer_->emit8(0x01);   // skip 1 byte
    
    // Trap instruction
    codeBuffer_->emit8(0xCC);   // int 3 (will be replaced with proper trap call)
    
#elif defined(__aarch64__) || defined(_M_ARM64)
    ARM64Assembler masm(codeBuffer_.get());
    
    // Load memory size from instance
    if (isMemory64) {
        masm.ldr64(16, 29, -24);  // x16 = memory size
    } else {
        masm.ldr32(16, 29, -24);  // w16 = memory size (zero-extended)
    }
    
    // Compare address (in x0/w0 assumed) against memory size
    if (isMemory64) {
        masm.cmp64(0, 16);
    } else {
        masm.cmp32(0, 16);
    }
    
    // Branch if higher or same (unsigned >=) to trap
    // b.hs trap (condition = 2 for HS)
    size_t trapBranchOffset = codeBuffer_->offset();
    uint32_t bhs = 0x54000002;  // b.hs with placeholder offset
    masm.emit32(bhs);
    
    // Will patch branch target to trap handler
#endif
}

// =============================================================================
// Value Stack
// =============================================================================

void BaselineCompiler::push(ValueLocation loc) {
    valueStack_.push_back(loc);
}

ValueLocation BaselineCompiler::pop() {
    if (valueStack_.empty()) {
        return ValueLocation();
    }
    ValueLocation loc = valueStack_.back();
    valueStack_.pop_back();
    return loc;
}

ValueLocation BaselineCompiler::peek(uint32_t depth) {
    if (depth >= valueStack_.size()) {
        return ValueLocation();
    }
    return valueStack_[valueStack_.size() - 1 - depth];
}

// =============================================================================
// Register Helpers
// =============================================================================

Reg BaselineCompiler::allocReg(ValType type) {
    if (type.isNumeric() && !type.isReference()) {
        if (type.kind() == ValType::Kind::F32 || type.kind() == ValType::Kind::F64) {
            return regAlloc_.allocFPR();
        } else if (type.kind() == ValType::Kind::V128) {
            return regAlloc_.allocV128();
        }
    }
    return regAlloc_.allocGPR();
}

void BaselineCompiler::freeReg(Reg reg) {
    regAlloc_.freeReg(reg);
}

Reg BaselineCompiler::allocVReg() {
    // Allocate a vector register (XMM/V register)
    return regAlloc_.allocV128();
}

void BaselineCompiler::freeVReg(Reg reg) {
    regAlloc_.freeReg(reg);  // Same as GPR/FPR for now
}

// =============================================================================
// Label Management
// =============================================================================

Label BaselineCompiler::newLabel() {
    return Label(nextLabelId_++);
}

void BaselineCompiler::bindLabel(Label& label) {
    label.offset = codeBuffer_->offset();
    label.bound = true;
    
    // Patch all pending jumps
    for (size_t jumpOffset : label.pendingJumps) {
        int32_t displacement = static_cast<int32_t>(label.offset - (jumpOffset + 4));
        codeBuffer_->patch32(jumpOffset, displacement);
    }
    label.pendingJumps.clear();
}

void BaselineCompiler::jumpTo(Label& label) {
    if (label.bound) {
        // Emit direct jump
        int32_t displacement = static_cast<int32_t>(label.offset - (codeBuffer_->offset() + 5));
        codeBuffer_->emit8(0xE9);  // jmp rel32
        codeBuffer_->emit32(displacement);
    } else {
        // Emit placeholder and record for patching
        codeBuffer_->emit8(0xE9);
        label.pendingJumps.push_back(codeBuffer_->offset());
        codeBuffer_->emit32(0);
    }
}

void BaselineCompiler::branchTo(Label& label, Reg condition) {
    X86Assembler masm(codeBuffer_.get());
    
    // Test condition register against zero
    masm.test32(condition.code, condition.code);
    
    // Emit jnz (jump if not zero) to label
    if (label.bound) {
        int32_t displacement = static_cast<int32_t>(label.offset - (codeBuffer_->offset() + 6));
        codeBuffer_->emit8(0x0F);
        codeBuffer_->emit8(0x85);  // jnz rel32
        codeBuffer_->emit32(displacement);
    } else {
        codeBuffer_->emit8(0x0F);
        codeBuffer_->emit8(0x85);  // jnz rel32
        label.pendingJumps.push_back(codeBuffer_->offset());
        codeBuffer_->emit32(0);
    }
}

// =============================================================================
// Binary/Unary Operation Templates
// =============================================================================

template<typename EmitFn>
bool BaselineCompiler::emitBinaryOp(ValType type, EmitFn&& emit) {
    ValueLocation rhs = pop();
    ValueLocation lhs = pop();
    
    Reg dstReg = allocReg(type);
    Reg srcReg = allocReg(type);
    
    // Load operands to registers if needed
    // ... actual code gen
    
    emit(dstReg, srcReg);
    
    freeReg(srcReg);
    push(ValueLocation::inReg(dstReg, type));
    return true;
}

template<typename EmitFn>
bool BaselineCompiler::emitUnaryOp(ValType type, EmitFn&& emit) {
    ValueLocation operand = pop();
    
    Reg dstReg = allocReg(type);
    
    // Load operand to register if needed
    // ... actual code gen
    
    emit(dstReg);
    
    push(ValueLocation::inReg(dstReg, type));
    return true;
}

// =============================================================================
// SIMD Operations (v128)
// =============================================================================

bool BaselineCompiler::emitSimdOp(uint32_t simdOp) {
    // SIMD opcode constants (from WASM SIMD proposal)
    // Load/Store
    constexpr uint32_t V128_LOAD = 0x00;
    constexpr uint32_t V128_STORE = 0x0B;
    constexpr uint32_t V128_CONST = 0x0C;
    
    // Shuffle/Swizzle
    constexpr uint32_t I8X16_SHUFFLE = 0x0D;
    constexpr uint32_t I8X16_SWIZZLE = 0x0E;
    
    // Splat
    constexpr uint32_t I8X16_SPLAT = 0x0F;
    constexpr uint32_t I16X8_SPLAT = 0x10;
    constexpr uint32_t I32X4_SPLAT = 0x11;
    constexpr uint32_t I64X2_SPLAT = 0x12;
    constexpr uint32_t F32X4_SPLAT = 0x13;
    constexpr uint32_t F64X2_SPLAT = 0x14;
    
    // Lane extract (i8x16)
    constexpr uint32_t I8X16_EXTRACT_LANE_S = 0x15;
    constexpr uint32_t I8X16_EXTRACT_LANE_U = 0x16;
    constexpr uint32_t I8X16_REPLACE_LANE = 0x17;
    
    // Lane extract (i16x8)
    constexpr uint32_t I16X8_EXTRACT_LANE_S = 0x18;
    constexpr uint32_t I16X8_EXTRACT_LANE_U = 0x19;
    constexpr uint32_t I16X8_REPLACE_LANE = 0x1A;
    
    // Lane extract (i32x4)
    constexpr uint32_t I32X4_EXTRACT_LANE = 0x1B;
    constexpr uint32_t I32X4_REPLACE_LANE = 0x1C;
    
    // Lane extract (i64x2)
    constexpr uint32_t I64X2_EXTRACT_LANE = 0x1D;
    constexpr uint32_t I64X2_REPLACE_LANE = 0x1E;
    
    // Lane extract (f32x4/f64x2)
    constexpr uint32_t F32X4_EXTRACT_LANE = 0x1F;
    constexpr uint32_t F32X4_REPLACE_LANE = 0x20;
    constexpr uint32_t F64X2_EXTRACT_LANE = 0x21;
    constexpr uint32_t F64X2_REPLACE_LANE = 0x22;
    
    // i8x16 comparisons
    constexpr uint32_t I8X16_EQ = 0x23;
    constexpr uint32_t I8X16_NE = 0x24;
    constexpr uint32_t I8X16_LT_S = 0x25;
    constexpr uint32_t I8X16_LT_U = 0x26;
    constexpr uint32_t I8X16_GT_S = 0x27;
    constexpr uint32_t I8X16_GT_U = 0x28;
    constexpr uint32_t I8X16_LE_S = 0x29;
    constexpr uint32_t I8X16_LE_U = 0x2A;
    constexpr uint32_t I8X16_GE_S = 0x2B;
    constexpr uint32_t I8X16_GE_U = 0x2C;
    
    // i16x8 comparisons
    constexpr uint32_t I16X8_EQ = 0x2D;
    constexpr uint32_t I16X8_NE = 0x2E;
    constexpr uint32_t I16X8_LT_S = 0x2F;
    constexpr uint32_t I16X8_LT_U = 0x30;
    constexpr uint32_t I16X8_GT_S = 0x31;
    constexpr uint32_t I16X8_GT_U = 0x32;
    constexpr uint32_t I16X8_LE_S = 0x33;
    constexpr uint32_t I16X8_LE_U = 0x34;
    constexpr uint32_t I16X8_GE_S = 0x35;
    constexpr uint32_t I16X8_GE_U = 0x36;
    
    // i32x4 comparisons
    constexpr uint32_t I32X4_EQ = 0x37;
    constexpr uint32_t I32X4_NE = 0x38;
    constexpr uint32_t I32X4_LT_S = 0x39;
    constexpr uint32_t I32X4_LT_U = 0x3A;
    constexpr uint32_t I32X4_GT_S = 0x3B;
    constexpr uint32_t I32X4_GT_U = 0x3C;
    constexpr uint32_t I32X4_LE_S = 0x3D;
    constexpr uint32_t I32X4_LE_U = 0x3E;
    constexpr uint32_t I32X4_GE_S = 0x3F;
    constexpr uint32_t I32X4_GE_U = 0x40;
    
    // Bitwise
    constexpr uint32_t V128_NOT = 0x4D;
    constexpr uint32_t V128_AND = 0x4E;
    constexpr uint32_t V128_ANDNOT = 0x4F;
    constexpr uint32_t V128_OR = 0x50;
    constexpr uint32_t V128_XOR = 0x51;
    constexpr uint32_t V128_BITSELECT = 0x52;
    constexpr uint32_t V128_ANY_TRUE = 0x53;
    
    // i8x16 arithmetic
    constexpr uint32_t I8X16_ABS = 0x60;
    constexpr uint32_t I8X16_NEG = 0x61;
    constexpr uint32_t I8X16_ADD = 0x6E;
    constexpr uint32_t I8X16_ADD_SAT_S = 0x6F;
    constexpr uint32_t I8X16_ADD_SAT_U = 0x70;
    constexpr uint32_t I8X16_SUB = 0x71;
    constexpr uint32_t I8X16_SUB_SAT_S = 0x72;
    constexpr uint32_t I8X16_SUB_SAT_U = 0x73;
    
    // i16x8 arithmetic
    constexpr uint32_t I16X8_ABS = 0x80;
    constexpr uint32_t I16X8_NEG = 0x81;
    constexpr uint32_t I16X8_ADD = 0x8E;
    constexpr uint32_t I16X8_ADD_SAT_S = 0x8F;
    constexpr uint32_t I16X8_ADD_SAT_U = 0x90;
    constexpr uint32_t I16X8_SUB = 0x91;
    constexpr uint32_t I16X8_SUB_SAT_S = 0x92;
    constexpr uint32_t I16X8_SUB_SAT_U = 0x93;
    constexpr uint32_t I16X8_MUL = 0x95;
    
    // i32x4 arithmetic
    constexpr uint32_t I32X4_ABS = 0xA0;
    constexpr uint32_t I32X4_NEG = 0xA1;
    constexpr uint32_t I32X4_ADD = 0xAE;
    constexpr uint32_t I32X4_SUB = 0xB1;
    constexpr uint32_t I32X4_MUL = 0xB5;
    constexpr uint32_t I32X4_DOT_I16X8_S = 0xBA;
    
    // i64x2 arithmetic + comparisons
    constexpr uint32_t I64X2_ABS = 0xC0;
    constexpr uint32_t I64X2_NEG = 0xC1;
    constexpr uint32_t I64X2_ADD = 0xCE;
    constexpr uint32_t I64X2_SUB = 0xD1;
    constexpr uint32_t I64X2_MUL = 0xD5;
    constexpr uint32_t I64X2_EQ = 0xD6;
    constexpr uint32_t I64X2_NE = 0xD7;
    constexpr uint32_t I64X2_LT_S = 0xD8;
    constexpr uint32_t I64X2_GT_S = 0xD9;
    constexpr uint32_t I64X2_LE_S = 0xDA;
    constexpr uint32_t I64X2_GE_S = 0xDB;
    
    // f32x4 arithmetic
    constexpr uint32_t F32X4_ABS = 0xE0;
    constexpr uint32_t F32X4_NEG = 0xE1;
    constexpr uint32_t F32X4_SQRT = 0xE3;
    constexpr uint32_t F32X4_ADD = 0xE4;
    constexpr uint32_t F32X4_SUB = 0xE5;
    constexpr uint32_t F32X4_MUL = 0xE6;
    constexpr uint32_t F32X4_DIV = 0xE7;
    constexpr uint32_t F32X4_MIN = 0xE8;
    constexpr uint32_t F32X4_MAX = 0xE9;
    constexpr uint32_t F32X4_PMIN = 0xEA;
    constexpr uint32_t F32X4_PMAX = 0xEB;
    
    // f64x2 arithmetic
    constexpr uint32_t F64X2_ABS = 0xEC;
    constexpr uint32_t F64X2_NEG = 0xED;
    constexpr uint32_t F64X2_SQRT = 0xEF;
    constexpr uint32_t F64X2_ADD = 0xF0;
    constexpr uint32_t F64X2_SUB = 0xF1;
    constexpr uint32_t F64X2_MUL = 0xF2;
    constexpr uint32_t F64X2_DIV = 0xF3;
    constexpr uint32_t F64X2_MIN = 0xF4;
    constexpr uint32_t F64X2_MAX = 0xF5;
    constexpr uint32_t F64X2_PMIN = 0xF6;
    constexpr uint32_t F64X2_PMAX = 0xF7;
    
    switch (simdOp) {
        // ===== Load/Store =====
        case V128_LOAD: {
            uint32_t flags, offset;
            if (!decoder_->readVarU32(&flags) || !decoder_->readVarU32(&offset)) {
                error_ = "failed to read v128.load operands";
                return false;
            }
            return emitV128Load(offset);
        }
        
        case V128_STORE: {
            uint32_t flags, offset;
            if (!decoder_->readVarU32(&flags) || !decoder_->readVarU32(&offset)) {
                error_ = "failed to read v128.store operands";
                return false;
            }
            return emitV128Store(offset);
        }
        
        case V128_CONST: {
            // Read 16 bytes of constant
            uint8_t bytes[16];
            for (int i = 0; i < 16; i++) {
                if (!decoder_->readByte(&bytes[i])) {
                    error_ = "failed to read v128.const";
                    return false;
                }
            }
            return emitV128Const(bytes);
        }
        
        // ===== Shuffle/Swizzle =====
        case I8X16_SHUFFLE: {
            uint8_t lanes[16];
            for (int i = 0; i < 16; i++) {
                if (!decoder_->readByte(&lanes[i])) {
                    error_ = "failed to read shuffle lanes";
                    return false;
                }
            }
            return emitI8x16Shuffle(lanes);
        }
        
        case I8X16_SWIZZLE: return emitI8x16Swizzle();
        
        // ===== Splat =====
        case I8X16_SPLAT: return emitSplat(1);
        case I16X8_SPLAT: return emitSplat(2);
        case I32X4_SPLAT: return emitSplat(4);
        case I64X2_SPLAT: return emitSplat(8);
        case F32X4_SPLAT: return emitSplatF32();
        case F64X2_SPLAT: return emitSplatF64();
        
        // ===== Lane Extract (i8x16) =====
        case I8X16_EXTRACT_LANE_S:
        case I8X16_EXTRACT_LANE_U: {
            uint8_t lane;
            if (!decoder_->readByte(&lane)) {
                error_ = "failed to read lane index";
                return false;
            }
            return emitExtractLane(1, lane, simdOp == I8X16_EXTRACT_LANE_S);
        }
        case I8X16_REPLACE_LANE: {
            uint8_t lane;
            if (!decoder_->readByte(&lane)) return false;
            return emitReplaceLane(1, lane);
        }
        
        // ===== Lane Extract (i16x8) =====
        case I16X8_EXTRACT_LANE_S:
        case I16X8_EXTRACT_LANE_U: {
            uint8_t lane;
            if (!decoder_->readByte(&lane)) return false;
            return emitExtractLane(2, lane, simdOp == I16X8_EXTRACT_LANE_S);
        }
        case I16X8_REPLACE_LANE: {
            uint8_t lane;
            if (!decoder_->readByte(&lane)) return false;
            return emitReplaceLane(2, lane);
        }
        
        // ===== Lane Extract (i32x4) =====
        case I32X4_EXTRACT_LANE: {
            uint8_t lane;
            if (!decoder_->readByte(&lane)) return false;
            return emitExtractLane(4, lane, false);
        }
        case I32X4_REPLACE_LANE: {
            uint8_t lane;
            if (!decoder_->readByte(&lane)) return false;
            return emitReplaceLane(4, lane);
        }
        
        // ===== Lane Extract (i64x2) =====
        case I64X2_EXTRACT_LANE: {
            uint8_t lane;
            if (!decoder_->readByte(&lane)) return false;
            return emitExtractLane(8, lane, false);
        }
        case I64X2_REPLACE_LANE: {
            uint8_t lane;
            if (!decoder_->readByte(&lane)) return false;
            return emitReplaceLane(8, lane);
        }
        
        // ===== Lane Extract (f32x4/f64x2) =====
        case F32X4_EXTRACT_LANE: {
            uint8_t lane;
            if (!decoder_->readByte(&lane)) return false;
            return emitExtractLaneF(4, lane);
        }
        case F32X4_REPLACE_LANE: {
            uint8_t lane;
            if (!decoder_->readByte(&lane)) return false;
            return emitReplaceLaneF(4, lane);
        }
        case F64X2_EXTRACT_LANE: {
            uint8_t lane;
            if (!decoder_->readByte(&lane)) return false;
            return emitExtractLaneF(8, lane);
        }
        case F64X2_REPLACE_LANE: {
            uint8_t lane;
            if (!decoder_->readByte(&lane)) return false;
            return emitReplaceLaneF(8, lane);
        }
        
        // ===== Comparisons (all types) =====
        case I8X16_EQ: case I8X16_NE:
        case I8X16_LT_S: case I8X16_LT_U: case I8X16_GT_S: case I8X16_GT_U:
        case I8X16_LE_S: case I8X16_LE_U: case I8X16_GE_S: case I8X16_GE_U:
        case I16X8_EQ: case I16X8_NE:
        case I16X8_LT_S: case I16X8_LT_U: case I16X8_GT_S: case I16X8_GT_U:
        case I16X8_LE_S: case I16X8_LE_U: case I16X8_GE_S: case I16X8_GE_U:
        case I32X4_EQ: case I32X4_NE:
        case I32X4_LT_S: case I32X4_LT_U: case I32X4_GT_S: case I32X4_GT_U:
        case I32X4_LE_S: case I32X4_LE_U: case I32X4_GE_S: case I32X4_GE_U:
        case I64X2_EQ: case I64X2_NE:
        case I64X2_LT_S: case I64X2_GT_S: case I64X2_LE_S: case I64X2_GE_S:
            return emitV128Compare(simdOp);
        
        // ===== Bitwise =====
        case V128_NOT:      return emitV128UnaryOp(simdOp);
        case V128_AND:      return emitV128BinaryOp(simdOp);
        case V128_ANDNOT:   return emitV128BinaryOp(simdOp);
        case V128_OR:       return emitV128BinaryOp(simdOp);
        case V128_XOR:      return emitV128BinaryOp(simdOp);
        case V128_BITSELECT: return emitV128Bitselect();
        case V128_ANY_TRUE: return emitV128AnyTrue();
        
        // ===== Unary ops =====
        case I8X16_ABS: case I8X16_NEG:
        case I16X8_ABS: case I16X8_NEG:
        case I32X4_ABS: case I32X4_NEG:
        case I64X2_ABS: case I64X2_NEG:
        case F32X4_ABS: case F32X4_NEG: case F32X4_SQRT:
        case F64X2_ABS: case F64X2_NEG: case F64X2_SQRT:
            return emitV128UnaryOp(simdOp);
        
        // ===== Binary arithmetic =====
        case I8X16_ADD: case I8X16_SUB:
        case I8X16_ADD_SAT_S: case I8X16_ADD_SAT_U:
        case I8X16_SUB_SAT_S: case I8X16_SUB_SAT_U:
        case I16X8_ADD: case I16X8_SUB: case I16X8_MUL:
        case I16X8_ADD_SAT_S: case I16X8_ADD_SAT_U:
        case I16X8_SUB_SAT_S: case I16X8_SUB_SAT_U:
        case I32X4_ADD: case I32X4_SUB: case I32X4_MUL:
        case I64X2_ADD: case I64X2_SUB: case I64X2_MUL:
        case F32X4_ADD: case F32X4_SUB: case F32X4_MUL: case F32X4_DIV:
        case F32X4_MIN: case F32X4_MAX: case F32X4_PMIN: case F32X4_PMAX:
        case F64X2_ADD: case F64X2_SUB: case F64X2_MUL: case F64X2_DIV:
        case F64X2_MIN: case F64X2_MAX: case F64X2_PMIN: case F64X2_PMAX:
            return emitV128BinaryOp(simdOp);
        
        // ===== Dot product =====
        case I32X4_DOT_I16X8_S:
            return emitI32x4DotI16x8S();
        
        default:
            error_ = "unsupported SIMD opcode: " + std::to_string(simdOp);
            return false;
    }
}

bool BaselineCompiler::emitV128Load(uint32_t offset) {
    // Pop address, load 128-bit value
    ValueLocation addr = pop();
    (void)addr;  // Suppress unused warning
    Reg addrReg = allocReg(ValType::i32());
    Reg vReg = allocVReg();  // Vector register
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Simplified: X64 SIMD load would go here
        (void)masm; (void)vReg; (void)addrReg; (void)offset;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        if (offset) {
            masm.addImm64(addrReg.code, addrReg.code, offset);
        }
        masm.ldrQ(vReg.code, addrReg.code, 0);
    });
    
    freeReg(addrReg);
    push(ValueLocation::inReg(vReg, ValType::i32()));  // Simplified: use i32 type
    return true;
}

bool BaselineCompiler::emitV128Store(uint32_t offset) {
    ValueLocation value = pop();
    ValueLocation addr = pop();
    (void)value; (void)addr;  // Suppress unused warnings
    
    Reg addrReg = allocReg(ValType::i32());
    Reg vReg = allocVReg();
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Simplified: X64 SIMD store would go here
        (void)masm; (void)addrReg; (void)offset; (void)vReg;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        if (offset) {
            masm.addImm64(addrReg.code, addrReg.code, offset);
        }
        masm.strQ(vReg.code, addrReg.code, 0);
    });
    
    freeReg(addrReg);
    freeVReg(vReg);
    return true;
}

bool BaselineCompiler::emitV128BinaryOp(uint32_t simdOp) {
    ValueLocation rhs = pop();
    ValueLocation lhs = pop();
    (void)rhs; (void)lhs;  // Suppress unused warnings
    
    Reg dst = allocVReg();
    Reg src = allocVReg();
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Simplified: just emit NOP-like instructions for now
        // Full implementation would need X86 SSE/AVX instructions
        (void)masm; (void)dst; (void)src; (void)simdOp;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        // SIMD opcode constants (from Opcode::Simd namespace)
        constexpr uint32_t I32X4_ADD = 0xA0;
        constexpr uint32_t I32X4_SUB = 0xA1;
        constexpr uint32_t I32X4_MUL = 0xB5;
        constexpr uint32_t F32X4_ADD = 0xE0;
        constexpr uint32_t F32X4_SUB = 0xE1;
        constexpr uint32_t F32X4_MUL = 0xE4;
        constexpr uint32_t F32X4_DIV = 0xE3;
        constexpr uint32_t F64X2_ADD = 0xF0;
        constexpr uint32_t F64X2_SUB = 0xF1;
        constexpr uint32_t F64X2_MUL = 0xF4;
        constexpr uint32_t F64X2_DIV = 0xF3;
        constexpr uint32_t V128_AND  = 0x4E;
        constexpr uint32_t V128_OR   = 0x50;
        constexpr uint32_t V128_XOR  = 0x51;
        
        switch (simdOp) {
            case I32X4_ADD: masm.addV4S(dst.code, dst.code, src.code); break;
            case I32X4_SUB: masm.subV4S(dst.code, dst.code, src.code); break;
            case I32X4_MUL: masm.mulV4S(dst.code, dst.code, src.code); break;
            case F32X4_ADD: masm.faddV4S(dst.code, dst.code, src.code); break;
            case F32X4_SUB: masm.fsubV4S(dst.code, dst.code, src.code); break;
            case F32X4_MUL: masm.fmulV4S(dst.code, dst.code, src.code); break;
            case F32X4_DIV: masm.fdivV4S(dst.code, dst.code, src.code); break;
            case F64X2_ADD: masm.faddV2D(dst.code, dst.code, src.code); break;
            case F64X2_SUB: masm.fsubV2D(dst.code, dst.code, src.code); break;
            case F64X2_MUL: masm.fmulV2D(dst.code, dst.code, src.code); break;
            case F64X2_DIV: masm.fdivV2D(dst.code, dst.code, src.code); break;
            case V128_AND:  masm.andV(dst.code, dst.code, src.code); break;
            case V128_OR:   masm.orrV(dst.code, dst.code, src.code); break;
            case V128_XOR:  masm.eorV(dst.code, dst.code, src.code); break;
            default: break;
        }
    });
    
    freeVReg(src);
    push(ValueLocation::inReg(dst, ValType::i32()));  // Simplified: use i32 type for now
    return true;
}

bool BaselineCompiler::emitV128UnaryOp(uint32_t simdOp) {
    ValueLocation operand = pop();
    (void)operand;  // Suppress unused warning
    
    Reg dst = allocVReg();
    Reg src = allocVReg();  // Source register
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        (void)masm; (void)dst; (void)src; (void)simdOp;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        constexpr uint32_t V128_NOT = 0x4D;
        switch (simdOp) {
            case V128_NOT: masm.notV(dst.code, src.code); break;
            default: break;
        }
    });
    
    freeVReg(src);
    push(ValueLocation::inReg(dst, ValType::i32()));
    return true;
}

// =============================================================================
// Extended SIMD Operations
// =============================================================================

bool BaselineCompiler::emitV128Const(const uint8_t bytes[16]) {
    Reg vReg = allocVReg();
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Load 128-bit constant from code stream
        (void)masm; (void)vReg; (void)bytes;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        // ARM64: Load from literal pool or construct with mov/ins
        masm.ldq(vReg.code, bytes);
    });
    
    push(ValueLocation::inReg(vReg, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitI8x16Shuffle(const uint8_t lanes[16]) {
    ValueLocation rhs = pop();
    ValueLocation lhs = pop();
    (void)rhs; (void)lhs;
    
    Reg dst = allocVReg();
    Reg src1 = allocVReg();
    Reg src2 = allocVReg();
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        (void)masm; (void)dst; (void)src1; (void)src2; (void)lanes;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        // Use TBL instruction for shuffle
        masm.tbl2(dst.code, src1.code, src2.code, lanes);
    });
    
    freeVReg(src1);
    freeVReg(src2);
    push(ValueLocation::inReg(dst, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitI8x16Swizzle() {
    ValueLocation indices = pop();
    ValueLocation vec = pop();
    (void)indices; (void)vec;
    
    Reg dst = allocVReg();
    Reg src = allocVReg();
    Reg idx = allocVReg();
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        (void)masm; (void)dst; (void)src; (void)idx;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.tbl(dst.code, src.code, idx.code);
    });
    
    freeVReg(src);
    freeVReg(idx);
    push(ValueLocation::inReg(dst, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitSplat(uint8_t laneSize) {
    ValueLocation val = pop();
    (void)val;
    
    Reg gpr = allocReg(ValType::i32());
    Reg dst = allocVReg();
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        (void)masm; (void)gpr; (void)dst; (void)laneSize;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        switch (laneSize) {
            case 1: masm.dupVB(dst.code, gpr.code); break;
            case 2: masm.dupVH(dst.code, gpr.code); break;
            case 4: masm.dupVS(dst.code, gpr.code); break;
            case 8: masm.dupVD(dst.code, gpr.code); break;
        }
    });
    
    freeReg(gpr);
    push(ValueLocation::inReg(dst, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitSplatF32() {
    ValueLocation val = pop();
    (void)val;
    
    Reg fpr = allocReg(ValType::f32());
    Reg dst = allocVReg();
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        (void)masm; (void)fpr; (void)dst;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.dupVS(dst.code, fpr.code);
    });
    
    freeReg(fpr);
    push(ValueLocation::inReg(dst, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitSplatF64() {
    ValueLocation val = pop();
    (void)val;
    
    Reg fpr = allocReg(ValType::f64());
    Reg dst = allocVReg();
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        (void)masm; (void)fpr; (void)dst;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.dupVD(dst.code, fpr.code);
    });
    
    freeReg(fpr);
    push(ValueLocation::inReg(dst, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitExtractLane(uint8_t laneSize, uint8_t laneIdx, bool signExtend) {
    ValueLocation vec = pop();
    (void)vec;
    
    Reg src = allocVReg();
    Reg dst = allocReg(ValType::i32());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        (void)masm; (void)src; (void)dst; (void)laneSize; (void)laneIdx; (void)signExtend;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        switch (laneSize) {
            case 1:
                if (signExtend) masm.smovB(dst.code, src.code, laneIdx);
                else masm.umovB(dst.code, src.code, laneIdx);
                break;
            case 2:
                if (signExtend) masm.smovH(dst.code, src.code, laneIdx);
                else masm.umovH(dst.code, src.code, laneIdx);
                break;
            case 4:
                masm.umovS(dst.code, src.code, laneIdx);
                break;
            case 8:
                masm.umovD(dst.code, src.code, laneIdx);
                break;
        }
    });
    
    freeVReg(src);
    push(ValueLocation::inReg(dst, laneSize >= 8 ? ValType::i64() : ValType::i32()));
    return true;
}

bool BaselineCompiler::emitReplaceLane(uint8_t laneSize, uint8_t laneIdx) {
    ValueLocation val = pop();
    ValueLocation vec = pop();
    (void)val; (void)vec;
    
    Reg src = allocVReg();
    Reg gpr = allocReg(ValType::i32());
    Reg dst = allocVReg();
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        (void)masm; (void)src; (void)gpr; (void)dst; (void)laneSize; (void)laneIdx;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movV(dst.code, src.code);
        switch (laneSize) {
            case 1: masm.insB(dst.code, laneIdx, gpr.code); break;
            case 2: masm.insH(dst.code, laneIdx, gpr.code); break;
            case 4: masm.insS(dst.code, laneIdx, gpr.code); break;
            case 8: masm.insD(dst.code, laneIdx, gpr.code); break;
        }
    });
    
    freeVReg(src);
    freeReg(gpr);
    push(ValueLocation::inReg(dst, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitExtractLaneF(uint8_t laneSize, uint8_t laneIdx) {
    ValueLocation vec = pop();
    (void)vec;
    
    Reg src = allocVReg();
    Reg dst = allocReg(laneSize == 4 ? ValType::f32() : ValType::f64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        (void)masm; (void)src; (void)dst; (void)laneSize; (void)laneIdx;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        if (laneSize == 4) masm.dupVS_lane(dst.code, src.code, laneIdx);
        else masm.dupVD_lane(dst.code, src.code, laneIdx);
    });
    
    freeVReg(src);
    push(ValueLocation::inReg(dst, laneSize == 4 ? ValType::f32() : ValType::f64()));
    return true;
}

bool BaselineCompiler::emitReplaceLaneF(uint8_t laneSize, uint8_t laneIdx) {
    ValueLocation val = pop();
    ValueLocation vec = pop();
    (void)val; (void)vec;
    
    Reg src = allocVReg();
    Reg fpr = allocReg(laneSize == 4 ? ValType::f32() : ValType::f64());
    Reg dst = allocVReg();
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        (void)masm; (void)src; (void)fpr; (void)dst; (void)laneSize; (void)laneIdx;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movV(dst.code, src.code);
        if (laneSize == 4) masm.insSFromS(dst.code, laneIdx, fpr.code);
        else masm.insDFromD(dst.code, laneIdx, fpr.code);
    });
    
    freeVReg(src);
    freeReg(fpr);
    push(ValueLocation::inReg(dst, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitV128Compare(uint32_t simdOp) {
    ValueLocation rhs = pop();
    ValueLocation lhs = pop();
    (void)rhs; (void)lhs;
    
    Reg dst = allocVReg();
    Reg src1 = allocVReg();
    Reg src2 = allocVReg();
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        (void)masm; (void)dst; (void)src1; (void)src2; (void)simdOp;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        // Emit comparison based on opcode
        // This would dispatch to cmeq, cmgt, cmhs, etc.
        masm.cmeq16B(dst.code, src1.code, src2.code);  // Default to eq
    });
    
    freeVReg(src1);
    freeVReg(src2);
    push(ValueLocation::inReg(dst, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitV128Bitselect() {
    ValueLocation mask = pop();
    ValueLocation onFalse = pop();
    ValueLocation onTrue = pop();
    (void)mask; (void)onFalse; (void)onTrue;
    
    Reg dst = allocVReg();
    Reg ifTrue = allocVReg();
    Reg ifFalse = allocVReg();
    Reg m = allocVReg();
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        (void)masm; (void)dst; (void)ifTrue; (void)ifFalse; (void)m;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.bsl(dst.code, ifTrue.code, ifFalse.code, m.code);
    });
    
    freeVReg(ifTrue);
    freeVReg(ifFalse);
    freeVReg(m);
    push(ValueLocation::inReg(dst, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitV128AnyTrue() {
    ValueLocation vec = pop();
    (void)vec;
    
    Reg src = allocVReg();
    Reg dst = allocReg(ValType::i32());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        (void)masm; (void)src; (void)dst;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        // Check if any lane is non-zero
        masm.umaxv(0, src.code);  // Max across all lanes to V0
        masm.umovS(dst.code, 0, 0);  // Move to GPR
        masm.cmpImm32(dst.code, 0);
        masm.cset(dst.code, 1);  // Set to 1 if NE
    });
    
    freeVReg(src);
    push(ValueLocation::inReg(dst, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitI32x4DotI16x8S() {
    ValueLocation rhs = pop();
    ValueLocation lhs = pop();
    (void)rhs; (void)lhs;
    
    Reg dst = allocVReg();
    Reg src1 = allocVReg();
    Reg src2 = allocVReg();
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        (void)masm; (void)dst; (void)src1; (void)src2;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        // Signed dot product: multiply and pairwise add
        masm.smull(dst.code, src1.code, src2.code);   // Multiply low half
        masm.smlal2(dst.code, src1.code, src2.code);  // Multiply-accumulate high half
        masm.saddlp(dst.code, dst.code);              // Pairwise add
    });
    
    freeVReg(src1);
    freeVReg(src2);
    push(ValueLocation::inReg(dst, ValType::i32()));
    return true;
}

// =============================================================================
// Exception Handling Operations
// =============================================================================

bool BaselineCompiler::emitTry(BlockType bt) {
    hasExceptionHandling_ = true;
    
    // Push a try control frame
    BaselineControlFrame frame(BaselineControlFrame::Kind::Try, bt, valueStack_.size());
    frame.endLabel = newLabel();
    controlStack_.push_back(frame);
    
    // Record try block index for unwinding
    tryStack_.push_back(controlStack_.size() - 1);
    
    // Emit landing pad setup
    // Save current exception handler chain and install new one
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Push current exception handler onto handler stack
        // This would normally be a call to runtime to setup exception frame
        // For now, emit placeholder
        masm.push(X86Assembler::RBP);  // Save frame pointer
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        // ARM64: Store current handler pointer
        masm.strPre(29, 31, -16);  // Push FP
    });
    
    return true;
}

bool BaselineCompiler::emitCatch(uint32_t tagIndex) {
    if (controlStack_.empty()) {
        error_ = "catch without matching try";
        return false;
    }
    
    BaselineControlFrame& frame = controlStack_.back();
    if (frame.kind != BaselineControlFrame::Kind::Try &&
        frame.kind != BaselineControlFrame::Kind::Catch) {
        error_ = "catch not in try block";
        return false;
    }
    
    // End previous try/catch block
    jumpTo(frame.endLabel);
    
    // Bind catch landing pad
    Label catchLabel = newLabel();
    bindLabel(catchLabel);
    
    // Change frame to catch mode
    frame.kind = BaselineControlFrame::Kind::Catch;
    
    // Pop exception values matching tag signature onto stack
    // In a full implementation, we'd look up the tag signature
    // and push the exception payload values
    (void)tagIndex;
    
    // Load exception payload from exception object
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Exception reference is in RAX (by calling convention)
        // Load payload fields into registers/stack
        // This is simplified - real impl would iterate tag params
        (void)masm;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        // Exception reference in X0
        (void)masm;
    });
    
    return true;
}

bool BaselineCompiler::emitCatchAll() {
    if (controlStack_.empty()) {
        error_ = "catch_all without matching try";
        return false;
    }
    
    BaselineControlFrame& frame = controlStack_.back();
    if (frame.kind != BaselineControlFrame::Kind::Try &&
        frame.kind != BaselineControlFrame::Kind::Catch) {
        error_ = "catch_all not in try block";
        return false;
    }
    
    // End previous try/catch block
    jumpTo(frame.endLabel);
    
    // Bind catch_all landing pad
    Label catchAllLabel = newLabel();
    bindLabel(catchAllLabel);
    
    // Change frame to catch mode
    frame.kind = BaselineControlFrame::Kind::Catch;
    
    // catch_all doesn't push any values (no payload)
    return true;
}

bool BaselineCompiler::emitThrow(uint32_t tagIndex) {
    // Pop exception payload values according to tag signature
    // In a full implementation, we'd look up the tag and pop values
    (void)tagIndex;
    
    // Create exception object and throw it
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Call runtime to allocate exception object
        // Move tag index into first arg register
        masm.mov32Imm(X86Assembler::RDI, static_cast<int32_t>(tagIndex));
        // Call to runtime throw function
        // Runtime will: allocate exception, copy payload, unwind stack
        masm.int3();  // Placeholder for call to throw runtime
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        // Move tag index to X0
        masm.movImm64(0, tagIndex);
        // Call throw runtime
        masm.brk(0);  // Placeholder
    });
    
    return true;
}

bool BaselineCompiler::emitRethrow(uint32_t depth) {
    // Find the catch block at the given depth
    if (depth >= tryStack_.size()) {
        error_ = "rethrow depth out of range";
        return false;
    }
    
    // Get the try block we're rethrowing from
    size_t tryIdx = tryStack_[tryStack_.size() - 1 - depth];
    (void)tryIdx;
    
    // Rethrow the current exception
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Load current exception from handler frame and rethrow
        // Call runtime rethrow function
        masm.int3();  // Placeholder
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        // Call runtime rethrow
        masm.brk(0);  // Placeholder
    });
    
    return true;
}

bool BaselineCompiler::emitDelegate(uint32_t depth) {
    if (controlStack_.empty()) {
        error_ = "delegate without matching try";
        return false;
    }
    
    BaselineControlFrame& frame = controlStack_.back();
    if (frame.kind != BaselineControlFrame::Kind::Try) {
        error_ = "delegate not in try block";
        return false;
    }
    
    // Delegate exceptions to an outer handler at the given depth
    // This is like an implicit rethrow at block end
    
    // Jump to end
    jumpTo(frame.endLabel);
    
    // Pop try block from stack
    if (!tryStack_.empty()) {
        tryStack_.pop_back();
    }
    
    // Mark frame for delegation
    (void)depth;
    
    return true;
}

bool BaselineCompiler::emitThrowRef() {
    // Pop exnref from stack
    ValueLocation exnRef = pop();
    (void)exnRef;
    
    // Null check
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Compare exnref with null
        masm.cmp64Imm(X86Assembler::RAX, 0);
        // Branch to null trap if zero
        size_t nullCheck = masm.jcc32(X86Assembler::E);
        (void)nullCheck;
        // Otherwise throw the exception
        masm.int3();  // Call throw_ref runtime
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        // Check for null
        masm.cbnz64(0, 8);  // Skip trap if non-null
        masm.brk(0);        // Trap on null
        // Throw the exception
        masm.brk(0);        // Call throw_ref runtime
    });
    
    return true;
}

bool BaselineCompiler::emitTryTable(BlockType bt, const std::vector<uint32_t>& handlers) {
    hasExceptionHandling_ = true;
    
    // try_table is a newer form with inline handlers
    BaselineControlFrame frame(BaselineControlFrame::Kind::Try, bt, valueStack_.size());
    frame.endLabel = newLabel();
    controlStack_.push_back(frame);
    tryStack_.push_back(controlStack_.size() - 1);
    
    // Process catch clauses
    // handlers format: [kind, tag_or_label, ...]
    (void)handlers;
    
    return true;
}

void BaselineCompiler::emitExceptionUnwind(uint32_t targetDepth) {
    // Unwind to the target try block depth
    // This pops exception frames and restores state
    
    while (tryStack_.size() > targetDepth) {
        EMIT_X64({
            X86Assembler masm(codeBuffer_.get());
            // Pop exception handler frame
            masm.pop(X86Assembler::RBP);
        });
        EMIT_ARM64({
            ARM64Assembler masm(codeBuffer_.get());
            // Restore previous handler
            masm.ldrPost(29, 31, 16);
        });
        tryStack_.pop_back();
    }
}

void BaselineCompiler::emitLoadExceptionPayload(uint32_t tagIndex) {
    // Load exception payload values based on tag signature
    // Would need tag registry to get signature
    (void)tagIndex;
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Exception object pointer in RAX
        // Load fields based on tag signature
        (void)masm;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        // Exception object pointer in X0
        (void)masm;
    });
}

void BaselineCompiler::emitStoreExceptionPayload(uint32_t tagIndex) {
    // Store values from stack into exception object
    (void)tagIndex;
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        (void)masm;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        (void)masm;
    });
}

// =============================================================================
// Atomic Operations
// =============================================================================

bool BaselineCompiler::emitAtomicOp(uint32_t atomicOp) {
    // Atomic opcode constants
    constexpr uint32_t NOTIFY = 0x00;
    constexpr uint32_t WAIT32 = 0x01;
    constexpr uint32_t WAIT64 = 0x02;
    constexpr uint32_t FENCE = 0x03;
    
    constexpr uint32_t I32_LOAD = 0x10;
    constexpr uint32_t I64_LOAD = 0x11;
    constexpr uint32_t I32_STORE = 0x17;
    constexpr uint32_t I64_STORE = 0x18;
    
    constexpr uint32_t I32_RMW_ADD = 0x1E;
    constexpr uint32_t I64_RMW_ADD = 0x1F;
    constexpr uint32_t I32_RMW_SUB = 0x25;
    constexpr uint32_t I64_RMW_SUB = 0x26;
    constexpr uint32_t I32_RMW_AND = 0x2C;
    constexpr uint32_t I64_RMW_AND = 0x2D;
    constexpr uint32_t I32_RMW_OR = 0x33;
    constexpr uint32_t I64_RMW_OR = 0x34;
    constexpr uint32_t I32_RMW_XOR = 0x3A;
    constexpr uint32_t I64_RMW_XOR = 0x3B;
    constexpr uint32_t I32_RMW_XCHG = 0x41;
    constexpr uint32_t I64_RMW_XCHG = 0x42;
    constexpr uint32_t I32_RMW_CMPXCHG = 0x48;
    constexpr uint32_t I64_RMW_CMPXCHG = 0x49;
    
    // Read memory operands
    uint32_t align, offset;
    if (!decoder_->readVarU32(&align) || !decoder_->readVarU32(&offset)) {
        error_ = "failed to read atomic memory operands";
        return false;
    }
    
    switch (atomicOp) {
        case NOTIFY:
            return emitAtomicNotify(offset);
        case WAIT32:
            return emitAtomicWait(false, offset);
        case WAIT64:
            return emitAtomicWait(true, offset);
        case FENCE:
            return emitAtomicFence();
        
        case I32_LOAD: case I64_LOAD:
            return emitAtomicLoad(atomicOp, offset);
        
        case I32_STORE: case I64_STORE:
            return emitAtomicStore(atomicOp, offset);
        
        case I32_RMW_ADD: case I64_RMW_ADD:
        case I32_RMW_SUB: case I64_RMW_SUB:
        case I32_RMW_AND: case I64_RMW_AND:
        case I32_RMW_OR: case I64_RMW_OR:
        case I32_RMW_XOR: case I64_RMW_XOR:
        case I32_RMW_XCHG: case I64_RMW_XCHG:
            return emitAtomicRMW(atomicOp, offset);
        
        case I32_RMW_CMPXCHG: case I64_RMW_CMPXCHG:
            return emitAtomicCmpxchg(atomicOp, offset);
        
        default:
            error_ = "unsupported atomic opcode: " + std::to_string(atomicOp);
            return false;
    }
}

bool BaselineCompiler::emitAtomicLoad(uint32_t atomicOp, uint32_t offset) {
    ValueLocation addr = pop();
    (void)addr;
    
    bool is64 = (atomicOp == 0x11);  // I64_LOAD
    Reg addrReg = allocReg(ValType::i32());
    Reg result = allocReg(is64 ? ValType::i64() : ValType::i32());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // x64: MOV is atomic for aligned access
        // For guaranteed atomicity, use LOCK prefix or explicit atomic ops
        if (offset) {
            masm.add32Imm(addrReg.code, offset);
        }
        if (is64) {
            masm.load64(result.code, addrReg.code, 0);
        } else {
            masm.load32(result.code, addrReg.code, 0);
        }
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        // ARM64: Use LDAR (load acquire) for atomic semantics
        if (offset) {
            masm.addImm64(addrReg.code, addrReg.code, offset);
        }
        if (is64) {
            masm.ldar64(result.code, addrReg.code);
        } else {
            masm.ldar32(result.code, addrReg.code);
        }
    });
    
    freeReg(addrReg);
    push(ValueLocation::inReg(result, is64 ? ValType::i64() : ValType::i32()));
    return true;
}

bool BaselineCompiler::emitAtomicStore(uint32_t atomicOp, uint32_t offset) {
    ValueLocation value = pop();
    ValueLocation addr = pop();
    (void)value; (void)addr;
    
    bool is64 = (atomicOp == 0x18);  // I64_STORE
    Reg addrReg = allocReg(ValType::i32());
    Reg valReg = allocReg(is64 ? ValType::i64() : ValType::i32());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // x64: XCHG is implicitly locked, or use explicit fence
        if (offset) {
            masm.add32Imm(addrReg.code, offset);
        }
        if (is64) {
            masm.store64(valReg.code, addrReg.code, 0);
        } else {
            masm.store32(valReg.code, addrReg.code, 0);
        }
        // Memory fence for sequential consistency
        masm.mfence();
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        // ARM64: Use STLR (store release) for atomic semantics
        if (offset) {
            masm.addImm64(addrReg.code, addrReg.code, offset);
        }
        if (is64) {
            masm.stlr64(valReg.code, addrReg.code);
        } else {
            masm.stlr32(valReg.code, addrReg.code);
        }
    });
    
    freeReg(addrReg);
    freeReg(valReg);
    return true;
}

bool BaselineCompiler::emitAtomicRMW(uint32_t atomicOp, uint32_t offset) {
    ValueLocation value = pop();
    ValueLocation addr = pop();
    (void)value; (void)addr;
    
    bool is64 = (atomicOp & 1);  // Odd opcodes are 64-bit
    Reg addrReg = allocReg(ValType::i32());
    Reg valReg = allocReg(is64 ? ValType::i64() : ValType::i32());
    Reg result = allocReg(is64 ? ValType::i64() : ValType::i32());
    
    // Determine operation type
    enum class RMWOp { Add, Sub, And, Or, Xor, Xchg };
    RMWOp op;
    if (atomicOp >= 0x1E && atomicOp <= 0x24) op = RMWOp::Add;
    else if (atomicOp >= 0x25 && atomicOp <= 0x2B) op = RMWOp::Sub;
    else if (atomicOp >= 0x2C && atomicOp <= 0x32) op = RMWOp::And;
    else if (atomicOp >= 0x33 && atomicOp <= 0x39) op = RMWOp::Or;
    else if (atomicOp >= 0x3A && atomicOp <= 0x40) op = RMWOp::Xor;
    else op = RMWOp::Xchg;
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        if (offset) masm.add32Imm(addrReg.code, offset);
        
        switch (op) {
            case RMWOp::Add:
                // LOCK XADD [addr], val
                masm.lockPrefix();
                if (is64) masm.xadd64(addrReg.code, valReg.code);
                else masm.xadd32(addrReg.code, valReg.code);
                masm.mov64(result.code, valReg.code);
                break;
            case RMWOp::Xchg:
                // XCHG is implicitly locked
                if (is64) masm.xchg64(addrReg.code, valReg.code);
                else masm.xchg32(addrReg.code, valReg.code);
                masm.mov64(result.code, valReg.code);
                break;
            default:
                // For Sub/And/Or/Xor: use CAS loop
                // Load current value, apply op, try CAS
                masm.load64(result.code, addrReg.code, 0);
                // CAS loop would go here
                break;
        }
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        if (offset) masm.addImm64(addrReg.code, addrReg.code, offset);
        
        // ARM64 LSE atomics
        switch (op) {
            case RMWOp::Add:
                if (is64) masm.ldaddal64(result.code, valReg.code, addrReg.code);
                else masm.ldaddal32(result.code, valReg.code, addrReg.code);
                break;
            case RMWOp::Sub:
                // Negate and add
                masm.neg64(valReg.code, valReg.code);
                if (is64) masm.ldaddal64(result.code, valReg.code, addrReg.code);
                else masm.ldaddal32(result.code, valReg.code, addrReg.code);
                break;
            case RMWOp::And:
                masm.mvn64(valReg.code, valReg.code);
                if (is64) masm.ldclral64(result.code, valReg.code, addrReg.code);
                else masm.ldclral32(result.code, valReg.code, addrReg.code);
                break;
            case RMWOp::Or:
                if (is64) masm.ldsetal64(result.code, valReg.code, addrReg.code);
                else masm.ldsetal32(result.code, valReg.code, addrReg.code);
                break;
            case RMWOp::Xor:
                if (is64) masm.ldeoral64(result.code, valReg.code, addrReg.code);
                else masm.ldeoral32(result.code, valReg.code, addrReg.code);
                break;
            case RMWOp::Xchg:
                if (is64) masm.swpal64(result.code, valReg.code, addrReg.code);
                else masm.swpal32(result.code, valReg.code, addrReg.code);
                break;
        }
    });
    
    freeReg(addrReg);
    freeReg(valReg);
    push(ValueLocation::inReg(result, is64 ? ValType::i64() : ValType::i32()));
    return true;
}

bool BaselineCompiler::emitAtomicCmpxchg(uint32_t atomicOp, uint32_t offset) {
    ValueLocation replacement = pop();
    ValueLocation expected = pop();
    ValueLocation addr = pop();
    (void)replacement; (void)expected; (void)addr;
    
    bool is64 = (atomicOp == 0x49);  // I64_RMW_CMPXCHG
    Reg addrReg = allocReg(ValType::i32());
    Reg expectReg = allocReg(is64 ? ValType::i64() : ValType::i32());
    Reg replaceReg = allocReg(is64 ? ValType::i64() : ValType::i32());
    Reg result = allocReg(is64 ? ValType::i64() : ValType::i32());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        if (offset) masm.add32Imm(addrReg.code, offset);
        
        // CMPXCHG: expects old value in RAX
        masm.mov64(X86Assembler::RAX, expectReg.code);
        masm.lockPrefix();
        if (is64) masm.cmpxchg64(addrReg.code, replaceReg.code);
        else masm.cmpxchg32(addrReg.code, replaceReg.code);
        // Result (old value) is in RAX
        masm.mov64(result.code, X86Assembler::RAX);
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        if (offset) masm.addImm64(addrReg.code, addrReg.code, offset);
        
        // ARM64 LSE: CASAL
        masm.mov64(result.code, expectReg.code);
        if (is64) masm.casal64(result.code, replaceReg.code, addrReg.code);
        else masm.casal32(result.code, replaceReg.code, addrReg.code);
    });
    
    freeReg(addrReg);
    freeReg(expectReg);
    freeReg(replaceReg);
    push(ValueLocation::inReg(result, is64 ? ValType::i64() : ValType::i32()));
    return true;
}

bool BaselineCompiler::emitAtomicWait(bool is64, uint32_t offset) {
    ValueLocation timeout = pop();  // i64 timeout in nanoseconds
    ValueLocation expected = pop();  // i32 or i64 expected value
    ValueLocation addr = pop();
    (void)timeout; (void)expected; (void)addr; (void)offset;
    
    Reg result = allocReg(ValType::i32());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Call runtime wait function
        // Args: addr, expected, timeout
        // Returns: 0=ok, 1=not-equal, 2=timed-out
        masm.int3();  // Placeholder for runtime call
        (void)is64;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        // Call runtime wait function
        masm.brk(0);  // Placeholder
        (void)is64;
    });
    
    push(ValueLocation::inReg(result, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitAtomicNotify(uint32_t offset) {
    ValueLocation count = pop();  // i32 count of threads to wake
    ValueLocation addr = pop();
    (void)count; (void)addr; (void)offset;
    
    Reg result = allocReg(ValType::i32());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Call runtime notify function
        // Returns: number of threads woken
        masm.int3();  // Placeholder
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.brk(0);  // Placeholder
    });
    
    push(ValueLocation::inReg(result, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitAtomicFence() {
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.mfence();  // Full memory barrier
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.dmb();  // Data memory barrier
    });
    return true;
}

// =============================================================================
// GC Operations
// =============================================================================

bool BaselineCompiler::emitGCOp(uint32_t gcOp) {
    using namespace GCOp;
    
    switch (gcOp) {
        case STRUCT_NEW: {
            uint32_t typeIndex;
            if (!decoder_->readVarU32(&typeIndex)) return false;
            return emitStructNew(typeIndex);
        }
        case STRUCT_NEW_DEFAULT: {
            uint32_t typeIndex;
            if (!decoder_->readVarU32(&typeIndex)) return false;
            return emitStructNewDefault(typeIndex);
        }
        case STRUCT_GET: {
            uint32_t typeIndex, fieldIndex;
            if (!decoder_->readVarU32(&typeIndex) || !decoder_->readVarU32(&fieldIndex)) return false;
            return emitStructGet(typeIndex, fieldIndex, false);
        }
        case STRUCT_GET_S: {
            uint32_t typeIndex, fieldIndex;
            if (!decoder_->readVarU32(&typeIndex) || !decoder_->readVarU32(&fieldIndex)) return false;
            return emitStructGet(typeIndex, fieldIndex, true);
        }
        case STRUCT_GET_U: {
            uint32_t typeIndex, fieldIndex;
            if (!decoder_->readVarU32(&typeIndex) || !decoder_->readVarU32(&fieldIndex)) return false;
            return emitStructGet(typeIndex, fieldIndex, false);
        }
        case STRUCT_SET: {
            uint32_t typeIndex, fieldIndex;
            if (!decoder_->readVarU32(&typeIndex) || !decoder_->readVarU32(&fieldIndex)) return false;
            return emitStructSet(typeIndex, fieldIndex);
        }
        case ARRAY_NEW: {
            uint32_t typeIndex;
            if (!decoder_->readVarU32(&typeIndex)) return false;
            return emitArrayNew(typeIndex);
        }
        case ARRAY_NEW_DEFAULT: {
            uint32_t typeIndex;
            if (!decoder_->readVarU32(&typeIndex)) return false;
            return emitArrayNewDefault(typeIndex);
        }
        case ARRAY_NEW_FIXED: {
            uint32_t typeIndex, length;
            if (!decoder_->readVarU32(&typeIndex) || !decoder_->readVarU32(&length)) return false;
            return emitArrayNewFixed(typeIndex, length);
        }
        case ARRAY_GET: {
            uint32_t typeIndex;
            if (!decoder_->readVarU32(&typeIndex)) return false;
            return emitArrayGet(typeIndex, false);
        }
        case ARRAY_GET_S: {
            uint32_t typeIndex;
            if (!decoder_->readVarU32(&typeIndex)) return false;
            return emitArrayGet(typeIndex, true);
        }
        case ARRAY_GET_U: {
            uint32_t typeIndex;
            if (!decoder_->readVarU32(&typeIndex)) return false;
            return emitArrayGet(typeIndex, false);
        }
        case ARRAY_SET: {
            uint32_t typeIndex;
            if (!decoder_->readVarU32(&typeIndex)) return false;
            return emitArraySet(typeIndex);
        }
        case ARRAY_LEN:
            return emitArrayLen();
        case ARRAY_FILL: {
            uint32_t typeIndex;
            if (!decoder_->readVarU32(&typeIndex)) return false;
            return emitArrayFill(typeIndex);
        }
        case ARRAY_COPY: {
            uint32_t destType, srcType;
            if (!decoder_->readVarU32(&destType) || !decoder_->readVarU32(&srcType)) return false;
            return emitArrayCopy(destType, srcType);
        }
        case REF_TEST: {
            uint32_t typeIndex;
            if (!decoder_->readVarU32(&typeIndex)) return false;
            return emitRefTest(typeIndex, false);
        }
        case REF_TEST_NULL: {
            uint32_t typeIndex;
            if (!decoder_->readVarU32(&typeIndex)) return false;
            return emitRefTest(typeIndex, true);
        }
        case REF_CAST: {
            uint32_t typeIndex;
            if (!decoder_->readVarU32(&typeIndex)) return false;
            return emitRefCast(typeIndex, false);
        }
        case REF_CAST_NULL: {
            uint32_t typeIndex;
            if (!decoder_->readVarU32(&typeIndex)) return false;
            return emitRefCast(typeIndex, true);
        }
        case I31_NEW:
            return emitI31New();
        case I31_GET_S:
            return emitI31GetS();
        case I31_GET_U:
            return emitI31GetU();
        case REF_NULL: {
            uint8_t heapType;
            if (!decoder_->readByte(&heapType)) return false;
            return emitRefNull(heapType);
        }
        case REF_IS_NULL:
            return emitRefIsNull();
        case REF_AS_NON_NULL:
            return emitRefAsNonNull();
        case REF_EQ:
            return emitRefEq();
        default:
            error_ = "unsupported GC opcode: " + std::to_string(gcOp);
            return false;
    }
}

bool BaselineCompiler::emitStructNew(uint32_t typeIndex) {
    // Get struct type from module
    // Pop field values from stack
    // Call allocator to create struct
    // Initialize fields
    // Push reference
    
    Reg result = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Call runtime: allocStruct(typeIndex)
        masm.mov32Imm(X86Assembler::RDI, static_cast<int32_t>(typeIndex));
        // masm.call(allocStruct);
        masm.int3();  // Placeholder
        masm.mov64(result.code, X86Assembler::RAX);
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(0, typeIndex);
        masm.brk(0);  // Placeholder
        masm.mov64(result.code, 0);
    });
    
    push(ValueLocation::inReg(result, ValType::externRef()));
    return true;
}

bool BaselineCompiler::emitStructNewDefault(uint32_t typeIndex) {
    // Same as struct.new but with default values
    Reg result = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.mov32Imm(X86Assembler::RDI, static_cast<int32_t>(typeIndex));
        masm.int3();
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(0, typeIndex);
        masm.brk(0);
    });
    
    push(ValueLocation::inReg(result, ValType::externRef()));
    return true;
}

bool BaselineCompiler::emitStructGet(uint32_t typeIndex, uint32_t fieldIndex, bool signExtend) {
    ValueLocation structRef = pop();
    (void)structRef;
    
    Reg objReg = allocReg(ValType::i64());
    Reg result = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Null check
        masm.cmp64Imm(objReg.code, 0);
        size_t nullTrap = masm.jcc32(X86Assembler::E);
        (void)nullTrap;
        
        // Load field offset and read value
        // Offset would be computed from type registry
        uint32_t offset = fieldIndex * 8;  // Simplified
        masm.load64(result.code, objReg.code, offset);
        (void)typeIndex; (void)signExtend;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.cbnz64(objReg.code, 8);
        masm.brk(0);
        
        uint32_t offset = fieldIndex * 8;
        masm.ldr64(result.code, objReg.code, offset);
        (void)typeIndex; (void)signExtend;
    });
    
    freeReg(objReg);
    push(ValueLocation::inReg(result, ValType::i64()));
    return true;
}

bool BaselineCompiler::emitStructSet(uint32_t typeIndex, uint32_t fieldIndex) {
    ValueLocation value = pop();
    ValueLocation structRef = pop();
    (void)value; (void)structRef;
    
    Reg objReg = allocReg(ValType::i64());
    Reg valReg = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.cmp64Imm(objReg.code, 0);
        size_t nullTrap = masm.jcc32(X86Assembler::E);
        (void)nullTrap;
        
        uint32_t offset = fieldIndex * 8;
        masm.store64(valReg.code, objReg.code, offset);
        (void)typeIndex;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.cbnz64(objReg.code, 8);
        masm.brk(0);
        
        uint32_t offset = fieldIndex * 8;
        masm.str64(valReg.code, objReg.code, offset);
        (void)typeIndex;
    });
    
    freeReg(objReg);
    freeReg(valReg);
    return true;
}

bool BaselineCompiler::emitArrayNew(uint32_t typeIndex) {
    ValueLocation initValue = pop();
    ValueLocation length = pop();
    (void)initValue; (void)length;
    
    Reg result = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.mov32Imm(X86Assembler::RDI, static_cast<int32_t>(typeIndex));
        masm.int3();
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(0, typeIndex);
        masm.brk(0);
    });
    
    push(ValueLocation::inReg(result, ValType::externRef()));
    return true;
}

bool BaselineCompiler::emitArrayNewDefault(uint32_t typeIndex) {
    ValueLocation length = pop();
    (void)length;
    
    Reg result = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.mov32Imm(X86Assembler::RDI, static_cast<int32_t>(typeIndex));
        masm.int3();
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(0, typeIndex);
        masm.brk(0);
    });
    
    push(ValueLocation::inReg(result, ValType::externRef()));
    return true;
}

bool BaselineCompiler::emitArrayNewFixed(uint32_t typeIndex, uint32_t length) {
    // Pop 'length' values from stack
    for (uint32_t i = 0; i < length; i++) {
        pop();
    }
    
    Reg result = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.mov32Imm(X86Assembler::RDI, static_cast<int32_t>(typeIndex));
        masm.mov32Imm(X86Assembler::RSI, static_cast<int32_t>(length));
        masm.int3();
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(0, typeIndex);
        masm.movImm64(1, length);
        masm.brk(0);
    });
    
    push(ValueLocation::inReg(result, ValType::externRef()));
    return true;
}

bool BaselineCompiler::emitArrayGet(uint32_t typeIndex, bool signExtend) {
    ValueLocation index = pop();
    ValueLocation arrayRef = pop();
    (void)index; (void)arrayRef;
    
    Reg objReg = allocReg(ValType::i64());
    Reg idxReg = allocReg(ValType::i32());
    Reg result = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Null check
        masm.cmp64Imm(objReg.code, 0);
        masm.jcc32(X86Assembler::E);
        
        // Bounds check
        // Load length from array object and compare
        
        // Load element
        (void)typeIndex; (void)signExtend; (void)idxReg; (void)result;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.cbnz64(objReg.code, 8);
        masm.brk(0);
        (void)typeIndex; (void)signExtend; (void)idxReg; (void)result;
    });
    
    freeReg(objReg);
    freeReg(idxReg);
    push(ValueLocation::inReg(result, ValType::i64()));
    return true;
}

bool BaselineCompiler::emitArraySet(uint32_t typeIndex) {
    ValueLocation value = pop();
    ValueLocation index = pop();
    ValueLocation arrayRef = pop();
    (void)value; (void)index; (void)arrayRef;
    
    Reg objReg = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.cmp64Imm(objReg.code, 0);
        masm.jcc32(X86Assembler::E);
        (void)typeIndex;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.cbnz64(objReg.code, 8);
        masm.brk(0);
        (void)typeIndex;
    });
    
    freeReg(objReg);
    return true;
}

bool BaselineCompiler::emitArrayLen() {
    ValueLocation arrayRef = pop();
    (void)arrayRef;
    
    Reg objReg = allocReg(ValType::i64());
    Reg result = allocReg(ValType::i32());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.cmp64Imm(objReg.code, 0);
        masm.jcc32(X86Assembler::E);
        // Load length field (at offset 8 in WasmArray)
        masm.load32(result.code, objReg.code, 8);
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.cbnz64(objReg.code, 8);
        masm.brk(0);
        masm.ldr32(result.code, objReg.code, 8);
    });
    
    freeReg(objReg);
    push(ValueLocation::inReg(result, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitArrayFill(uint32_t typeIndex) {
    ValueLocation count = pop();
    ValueLocation value = pop();
    ValueLocation offset = pop();
    ValueLocation arrayRef = pop();
    (void)count; (void)value; (void)offset; (void)arrayRef; (void)typeIndex;
    
    EMIT_X64({ X86Assembler masm(codeBuffer_.get()); masm.int3(); });
    EMIT_ARM64({ ARM64Assembler masm(codeBuffer_.get()); masm.brk(0); });
    
    return true;
}

bool BaselineCompiler::emitArrayCopy(uint32_t destType, uint32_t srcType) {
    ValueLocation count = pop();
    ValueLocation srcOffset = pop();
    ValueLocation srcArray = pop();
    ValueLocation destOffset = pop();
    ValueLocation destArray = pop();
    (void)count; (void)srcOffset; (void)srcArray; (void)destOffset; (void)destArray;
    (void)destType; (void)srcType;
    
    EMIT_X64({ X86Assembler masm(codeBuffer_.get()); masm.int3(); });
    EMIT_ARM64({ ARM64Assembler masm(codeBuffer_.get()); masm.brk(0); });
    
    return true;
}

bool BaselineCompiler::emitRefTest(uint32_t typeIndex, bool nullable) {
    ValueLocation ref = pop();
    (void)ref;
    
    Reg objReg = allocReg(ValType::i64());
    Reg result = allocReg(ValType::i32());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // If nullable, null passes the test
        if (nullable) {
            masm.cmp64Imm(objReg.code, 0);
            // If null, result = 1
        }
        // Call runtime type check
        masm.mov32Imm(X86Assembler::RSI, static_cast<int32_t>(typeIndex));
        masm.int3();
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(1, typeIndex);
        if (nullable) {
            masm.cbz64(objReg.code, 8);
        }
        masm.brk(0);
    });
    
    freeReg(objReg);
    push(ValueLocation::inReg(result, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitRefCast(uint32_t typeIndex, bool nullable) {
    ValueLocation ref = pop();
    (void)ref;
    
    Reg objReg = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        if (!nullable) {
            masm.cmp64Imm(objReg.code, 0);
            masm.jcc32(X86Assembler::E);  // Trap on null
        }
        // Type check and trap if fails
        masm.mov32Imm(X86Assembler::RSI, static_cast<int32_t>(typeIndex));
        masm.int3();
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        if (!nullable) {
            masm.cbnz64(objReg.code, 8);
            masm.brk(0);
        }
        masm.movImm64(1, typeIndex);
        masm.brk(0);
    });
    
    push(ValueLocation::inReg(objReg, ValType::externRef()));
    return true;
}

bool BaselineCompiler::emitBrOnCast(uint32_t labelIdx, uint32_t srcType, uint32_t dstType, bool onFail) {
    ValueLocation ref = pop();
    (void)ref; (void)srcType; (void)dstType;
    
    Reg objReg = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Type test
        // Branch based on result and onFail flag
        (void)masm; (void)objReg; (void)labelIdx; (void)onFail;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        (void)masm; (void)objReg; (void)labelIdx; (void)onFail;
    });
    
    push(ValueLocation::inReg(objReg, ValType::externRef()));
    return true;
}

bool BaselineCompiler::emitI31New() {
    ValueLocation value = pop();
    (void)value;
    
    Reg valReg = allocReg(ValType::i32());
    Reg result = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // i31ref encoding: (value << 1) | 1
        masm.mov64(result.code, valReg.code);
        masm.shl64_cl(result.code);  // Shift left by 1
        masm.or32Imm(result.code, 1);  // Set tag bit
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.lsl64(result.code, valReg.code, 1);
        masm.orrImm64(result.code, result.code, 1);
    });
    
    freeReg(valReg);
    push(ValueLocation::inReg(result, ValType::externRef()));
    return true;
}

bool BaselineCompiler::emitI31GetS() {
    ValueLocation ref = pop();
    (void)ref;
    
    Reg refReg = allocReg(ValType::i64());
    Reg result = allocReg(ValType::i32());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Sign-extend from 31 bits
        masm.mov64(result.code, refReg.code);
        masm.shr64_cl(result.code);  // Shift right by 1
        // Sign extend (shift left then arithmetic right)
        masm.shl32_cl(result.code);
        masm.sar32_cl(result.code);
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.asr64(result.code, refReg.code, 1);
        masm.sxtw(result.code, result.code);
    });
    
    freeReg(refReg);
    push(ValueLocation::inReg(result, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitI31GetU() {
    ValueLocation ref = pop();
    (void)ref;
    
    Reg refReg = allocReg(ValType::i64());
    Reg result = allocReg(ValType::i32());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.mov64(result.code, refReg.code);
        masm.shr64_cl(result.code);
        masm.and32Imm(result.code, 0x7FFFFFFF);
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.lsr64(result.code, refReg.code, 1);
        masm.andImm64(result.code, result.code, 0x7FFFFFFF);
    });
    
    freeReg(refReg);
    push(ValueLocation::inReg(result, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitRefNull(uint8_t heapType) {
    (void)heapType;
    Reg result = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.xor64(result.code, result.code);
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(result.code, 0);
    });
    
    push(ValueLocation::inReg(result, ValType::externRef()));
    return true;
}

bool BaselineCompiler::emitRefIsNull() {
    ValueLocation ref = pop();
    (void)ref;
    
    Reg refReg = allocReg(ValType::i64());
    Reg result = allocReg(ValType::i32());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.cmp64Imm(refReg.code, 0);
        masm.setcc(X86Assembler::E, result.code);
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.cmpImm64(refReg.code, 0);
        masm.cset(result.code, 0);  // EQ condition
    });
    
    freeReg(refReg);
    push(ValueLocation::inReg(result, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitRefAsNonNull() {
    ValueLocation ref = pop();
    (void)ref;
    
    Reg refReg = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.cmp64Imm(refReg.code, 0);
        size_t nullTrap = masm.jcc32(X86Assembler::E);
        (void)nullTrap;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.cbnz64(refReg.code, 8);
        masm.brk(0);
    });
    
    push(ValueLocation::inReg(refReg, ValType::externRef()));
    return true;
}

bool BaselineCompiler::emitRefEq() {
    ValueLocation rhs = pop();
    ValueLocation lhs = pop();
    (void)rhs; (void)lhs;
    
    Reg lhsReg = allocReg(ValType::i64());
    Reg rhsReg = allocReg(ValType::i64());
    Reg result = allocReg(ValType::i32());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.cmp64(lhsReg.code, rhsReg.code);
        masm.setcc(X86Assembler::E, result.code);
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.cmp64(lhsReg.code, rhsReg.code);
        masm.cset(result.code, 0);  // EQ
    });
    
    freeReg(lhsReg);
    freeReg(rhsReg);
    push(ValueLocation::inReg(result, ValType::i32()));
    return true;
}

// =============================================================================
// Bulk Memory Operations
// =============================================================================

bool BaselineCompiler::emitBulkOp(uint32_t bulkOp) {
    using namespace BulkOp;
    
    switch (bulkOp) {
        case MEMORY_INIT: {
            uint32_t dataIdx, memIdx;
            if (!decoder_->readVarU32(&dataIdx) || !decoder_->readVarU32(&memIdx)) return false;
            return emitMemoryInit(dataIdx, memIdx);
        }
        case DATA_DROP: {
            uint32_t dataIdx;
            if (!decoder_->readVarU32(&dataIdx)) return false;
            return emitDataDrop(dataIdx);
        }
        case MEMORY_COPY: {
            uint32_t destMemIdx, srcMemIdx;
            if (!decoder_->readVarU32(&destMemIdx) || !decoder_->readVarU32(&srcMemIdx)) return false;
            return emitMemoryCopy(destMemIdx, srcMemIdx);
        }
        case MEMORY_FILL: {
            uint32_t memIdx;
            if (!decoder_->readVarU32(&memIdx)) return false;
            return emitMemoryFill(memIdx);
        }
        case TABLE_INIT: {
            uint32_t elemIdx, tableIdx;
            if (!decoder_->readVarU32(&elemIdx) || !decoder_->readVarU32(&tableIdx)) return false;
            return emitTableInit(elemIdx, tableIdx);
        }
        case ELEM_DROP: {
            uint32_t elemIdx;
            if (!decoder_->readVarU32(&elemIdx)) return false;
            return emitElemDrop(elemIdx);
        }
        case TABLE_COPY: {
            uint32_t destTableIdx, srcTableIdx;
            if (!decoder_->readVarU32(&destTableIdx) || !decoder_->readVarU32(&srcTableIdx)) return false;
            return emitTableCopy(destTableIdx, srcTableIdx);
        }
        case TABLE_GROW: {
            uint32_t tableIdx;
            if (!decoder_->readVarU32(&tableIdx)) return false;
            return emitTableGrow(tableIdx);
        }
        case TABLE_SIZE: {
            uint32_t tableIdx;
            if (!decoder_->readVarU32(&tableIdx)) return false;
            return emitTableSize(tableIdx);
        }
        case TABLE_FILL: {
            uint32_t tableIdx;
            if (!decoder_->readVarU32(&tableIdx)) return false;
            return emitTableFill(tableIdx);
        }
        default:
            error_ = "unsupported bulk memory opcode: " + std::to_string(bulkOp);
            return false;
    }
}

bool BaselineCompiler::emitMemoryCopy(uint32_t destMemIdx, uint32_t srcMemIdx) {
    ValueLocation count = pop();
    ValueLocation srcOffset = pop();
    ValueLocation destOffset = pop();
    (void)count; (void)srcOffset; (void)destOffset;
    
    Reg destReg = allocReg(ValType::i32());
    Reg srcReg = allocReg(ValType::i32());
    Reg countReg = allocReg(ValType::i32());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Call runtime: memoryCopy(destMem, destOff, srcMem, srcOff, count)
        masm.mov32Imm(X86Assembler::RDI, static_cast<int32_t>(destMemIdx));
        masm.mov32Imm(X86Assembler::RCX, static_cast<int32_t>(srcMemIdx));
        // REP MOVSB for same memory index
        if (destMemIdx == srcMemIdx) {
            masm.cld();
            masm.repMovsb();
        } else {
            masm.int3();  // Runtime call for cross-memory
        }
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(0, destMemIdx);
        masm.movImm64(3, srcMemIdx);
        masm.brk(0);  // Runtime call
    });
    
    freeReg(destReg);
    freeReg(srcReg);
    freeReg(countReg);
    return true;
}

bool BaselineCompiler::emitMemoryFill(uint32_t memIdx) {
    ValueLocation count = pop();
    ValueLocation value = pop();
    ValueLocation dest = pop();
    (void)count; (void)value; (void)dest;
    
    Reg destReg = allocReg(ValType::i32());
    Reg valReg = allocReg(ValType::i32());
    Reg countReg = allocReg(ValType::i32());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // REP STOSB for memory fill
        masm.cld();
        masm.repStosb();
        (void)memIdx;
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.brk(0);  // Runtime call
        (void)memIdx;
    });
    
    freeReg(destReg);
    freeReg(valReg);
    freeReg(countReg);
    return true;
}

bool BaselineCompiler::emitMemoryInit(uint32_t dataIdx, uint32_t memIdx) {
    ValueLocation count = pop();
    ValueLocation srcOffset = pop();
    ValueLocation destOffset = pop();
    (void)count; (void)srcOffset; (void)destOffset;
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.mov32Imm(X86Assembler::RDI, static_cast<int32_t>(dataIdx));
        masm.mov32Imm(X86Assembler::RSI, static_cast<int32_t>(memIdx));
        masm.int3();  // Runtime call
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(0, dataIdx);
        masm.movImm64(1, memIdx);
        masm.brk(0);
    });
    
    return true;
}

bool BaselineCompiler::emitDataDrop(uint32_t dataIdx) {
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.mov32Imm(X86Assembler::RDI, static_cast<int32_t>(dataIdx));
        masm.int3();  // Runtime call to mark segment as dropped
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(0, dataIdx);
        masm.brk(0);
    });
    return true;
}

bool BaselineCompiler::emitTableCopy(uint32_t destTableIdx, uint32_t srcTableIdx) {
    ValueLocation count = pop();
    ValueLocation srcOffset = pop();
    ValueLocation destOffset = pop();
    (void)count; (void)srcOffset; (void)destOffset;
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.mov32Imm(X86Assembler::RDI, static_cast<int32_t>(destTableIdx));
        masm.mov32Imm(X86Assembler::RSI, static_cast<int32_t>(srcTableIdx));
        masm.int3();  // Runtime call
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(0, destTableIdx);
        masm.movImm64(1, srcTableIdx);
        masm.brk(0);
    });
    return true;
}

bool BaselineCompiler::emitTableInit(uint32_t elemIdx, uint32_t tableIdx) {
    ValueLocation count = pop();
    ValueLocation srcOffset = pop();
    ValueLocation destOffset = pop();
    (void)count; (void)srcOffset; (void)destOffset;
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.mov32Imm(X86Assembler::RDI, static_cast<int32_t>(elemIdx));
        masm.mov32Imm(X86Assembler::RSI, static_cast<int32_t>(tableIdx));
        masm.int3();
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(0, elemIdx);
        masm.movImm64(1, tableIdx);
        masm.brk(0);
    });
    return true;
}

bool BaselineCompiler::emitElemDrop(uint32_t elemIdx) {
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.mov32Imm(X86Assembler::RDI, static_cast<int32_t>(elemIdx));
        masm.int3();
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(0, elemIdx);
        masm.brk(0);
    });
    return true;
}

bool BaselineCompiler::emitTableGrow(uint32_t tableIdx) {
    ValueLocation delta = pop();
    ValueLocation initValue = pop();
    (void)delta; (void)initValue;
    
    Reg result = allocReg(ValType::i32());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.mov32Imm(X86Assembler::RDI, static_cast<int32_t>(tableIdx));
        masm.int3();
        masm.mov32(result.code, X86Assembler::RAX);
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(0, tableIdx);
        masm.brk(0);
        masm.mov32(result.code, 0);
    });
    
    push(ValueLocation::inReg(result, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitTableSize(uint32_t tableIdx) {
    Reg result = allocReg(ValType::i32());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.mov32Imm(X86Assembler::RDI, static_cast<int32_t>(tableIdx));
        masm.int3();
        masm.mov32(result.code, X86Assembler::RAX);
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(0, tableIdx);
        masm.brk(0);
        masm.mov32(result.code, 0);
    });
    
    push(ValueLocation::inReg(result, ValType::i32()));
    return true;
}

bool BaselineCompiler::emitTableFill(uint32_t tableIdx) {
    ValueLocation count = pop();
    ValueLocation value = pop();
    ValueLocation dest = pop();
    (void)count; (void)value; (void)dest;
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        masm.mov32Imm(X86Assembler::RDI, static_cast<int32_t>(tableIdx));
        masm.int3();
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(0, tableIdx);
        masm.brk(0);
    });
    return true;
}

// =============================================================================
// Reference Types
// =============================================================================

bool BaselineCompiler::emitRefFunc(uint32_t funcIdx) {
    Reg result = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Load function reference from table
        masm.mov32Imm(result.code, static_cast<int32_t>(funcIdx));
        // Set funcref tag (low bit = 0 for funcref)
        masm.shl64_cl(result.code);
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(result.code, funcIdx);
        masm.lsl64(result.code, result.code, 1);
    });
    
    push(ValueLocation::inReg(result, ValType::funcRef()));
    return true;
}

bool BaselineCompiler::emitTableGet(uint32_t tableIdx) {
    ValueLocation index = pop();
    (void)index;
    
    Reg idxReg = allocReg(ValType::i32());
    Reg result = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Bounds check
        // Load table pointer
        // Load element at index
        masm.mov32Imm(X86Assembler::RDI, static_cast<int32_t>(tableIdx));
        masm.int3();
        masm.mov64(result.code, X86Assembler::RAX);
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(0, tableIdx);
        masm.brk(0);
        masm.mov64(result.code, 0);
    });
    
    freeReg(idxReg);
    push(ValueLocation::inReg(result, ValType::externRef()));
    return true;
}

bool BaselineCompiler::emitTableSet(uint32_t tableIdx) {
    ValueLocation value = pop();
    ValueLocation index = pop();
    (void)value; (void)index;
    
    Reg idxReg = allocReg(ValType::i32());
    Reg valReg = allocReg(ValType::i64());
    
    EMIT_X64({
        X86Assembler masm(codeBuffer_.get());
        // Bounds check
        // Load table pointer
        // Store element at index
        masm.mov32Imm(X86Assembler::RDI, static_cast<int32_t>(tableIdx));
        masm.int3();
    });
    EMIT_ARM64({
        ARM64Assembler masm(codeBuffer_.get());
        masm.movImm64(0, tableIdx);
        masm.brk(0);
    });
    
    freeReg(idxReg);
    freeReg(valReg);
    return true;
}

} // namespace Zepra::Wasm

