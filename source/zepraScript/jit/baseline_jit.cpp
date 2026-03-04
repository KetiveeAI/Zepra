/**
 * @file baseline_jit.cpp
 * @brief Baseline JIT compiler implementation
 * 
 * Simple one-pass JIT that generates x86-64 machine code from bytecode.
 */

#include "jit/baseline_jit.hpp"
#include "bytecode/bytecode_generator.hpp"
#include "runtime/execution/vm.hpp"
#include <cstring>

#ifdef __linux__
#include <sys/mman.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

namespace Zepra::JIT {

// =============================================================================
// CompiledCode - Executable memory management
// =============================================================================

CompiledCode::CompiledCode(void* code, size_t size) 
    : code_(code), size_(size) {}

CompiledCode::~CompiledCode() {
    if (code_) {
#ifdef __linux__
        munmap(code_, size_);
#elif defined(_WIN32)
        VirtualFree(code_, 0, MEM_RELEASE);
#endif
    }
}

// =============================================================================
// CodeBuffer - Machine code generation
// =============================================================================

CodeBuffer::CodeBuffer(size_t initialCapacity) {
    buffer_.reserve(initialCapacity);
}

void CodeBuffer::emit8(uint8_t byte) {
    buffer_.push_back(byte);
}

void CodeBuffer::emit16(uint16_t value) {
    emit8(value & 0xFF);
    emit8((value >> 8) & 0xFF);
}

void CodeBuffer::emit32(uint32_t value) {
    emit8(value & 0xFF);
    emit8((value >> 8) & 0xFF);
    emit8((value >> 16) & 0xFF);
    emit8((value >> 24) & 0xFF);
}

void CodeBuffer::emit64(uint64_t value) {
    emit32(value & 0xFFFFFFFF);
    emit32((value >> 32) & 0xFFFFFFFF);
}

// x86-64 instruction encoding
void CodeBuffer::emitPush(int reg) {
    if (reg >= 8) {
        emit8(0x41);  // REX.B prefix
        emit8(0x50 + (reg - 8));
    } else {
        emit8(0x50 + reg);
    }
}

void CodeBuffer::emitPop(int reg) {
    if (reg >= 8) {
        emit8(0x41);  // REX.B prefix
        emit8(0x58 + (reg - 8));
    } else {
        emit8(0x58 + reg);
    }
}

void CodeBuffer::emitMov(int dst, int src) {
    uint8_t rex = 0x48;  // REX.W for 64-bit
    if (dst >= 8) rex |= 0x04;  // REX.R
    if (src >= 8) rex |= 0x01;  // REX.B
    emit8(rex);
    emit8(0x89);  // MOV r/m64, r64
    emit8(0xC0 | ((src & 7) << 3) | (dst & 7));
}

void CodeBuffer::emitMovImm64(int reg, uint64_t value) {
    uint8_t rex = 0x48;  // REX.W for 64-bit
    if (reg >= 8) rex |= 0x01;  // REX.B
    emit8(rex);
    emit8(0xB8 + (reg & 7));  // MOV r64, imm64
    emit64(value);
}

void CodeBuffer::emitAdd(int dst, int src) {
    uint8_t rex = 0x48;  // REX.W
    if (dst >= 8) rex |= 0x04;  // REX.R
    if (src >= 8) rex |= 0x01;  // REX.B
    emit8(rex);
    emit8(0x01);  // ADD r/m64, r64
    emit8(0xC0 | ((src & 7) << 3) | (dst & 7));
}

void CodeBuffer::emitSub(int dst, int src) {
    uint8_t rex = 0x48;  // REX.W
    if (dst >= 8) rex |= 0x04;  // REX.R
    if (src >= 8) rex |= 0x01;  // REX.B
    emit8(rex);
    emit8(0x29);  // SUB r/m64, r64
    emit8(0xC0 | ((src & 7) << 3) | (dst & 7));
}

void CodeBuffer::emitRet() {
    emit8(0xC3);  // RET
}

void CodeBuffer::emitCall(void* target) {
    // mov rax, target
    emitMovImm64(Reg::RAX, reinterpret_cast<uint64_t>(target));
    // call rax
    emit8(0xFF);  // CALL r/m64
    emit8(0xD0);  // ModR/M: reg=2 (call), r/m=0 (rax)
}

void CodeBuffer::emitNop() {
    emit8(0x90);
}

std::unique_ptr<CompiledCode> CodeBuffer::finalize() {
    size_t codeSize = buffer_.size();
    void* execMem = nullptr;
    
#ifdef __linux__
    execMem = mmap(nullptr, codeSize, 
                   PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (execMem == MAP_FAILED) return nullptr;
#elif defined(_WIN32)
    execMem = VirtualAlloc(nullptr, codeSize, MEM_COMMIT | MEM_RESERVE, 
                           PAGE_EXECUTE_READWRITE);
    if (!execMem) return nullptr;
#else
    return nullptr;  // Unsupported platform
#endif
    
    // Copy code to executable memory
    memcpy(execMem, buffer_.data(), codeSize);
    
    return std::make_unique<CompiledCode>(execMem, codeSize);
}

// =============================================================================
// BaselineJIT - Compiler
// =============================================================================

BaselineJIT::BaselineJIT(Runtime::VM* vm) : vm_(vm) {}

bool BaselineJIT::isAvailable() {
#if defined(__x86_64__) || defined(_M_X64)
    return true;
#else
    return false;
#endif
}

std::unique_ptr<CompiledCode> BaselineJIT::compile(const Bytecode::BytecodeChunk* chunk) {
    if (!chunk || !isAvailable()) return nullptr;
    
    CodeBuffer buffer;
    
    // Prologue: push rbp; mov rbp, rsp
    buffer.emitPush(Reg::RBP);
    buffer.emitMov(Reg::RBP, Reg::RSP);
    
    // Compile bytecode
    const auto& code = chunk->code();
    size_t ip = 0;
    
    while (ip < code.size()) {
        Bytecode::Opcode op = static_cast<Bytecode::Opcode>(code[ip++]);
        compileOpcode(buffer, op, code.data(), ip);
    }
    
    // Epilogue: pop rbp; ret
    buffer.emitPop(Reg::RBP);
    buffer.emitRet();
    
    auto compiled = buffer.finalize();
    if (compiled) {
        functionsCompiled_++;
        bytesGenerated_ += compiled->size();
    }
    
    return compiled;
}

void BaselineJIT::compileOpcode(CodeBuffer& buffer, Bytecode::Opcode op,
                                 const uint8_t* bytecode, size_t& ip) {
    using Bytecode::Opcode;
    
    switch (op) {
        // =====================================================================
        // Stack Operations
        // =====================================================================
        case Opcode::OP_NOP:
            buffer.emitNop();
            break;
            
        case Opcode::OP_POP:
            // Discard top of stack
            buffer.emitPop(Reg::RAX);
            break;
            
        case Opcode::OP_DUP:
            // Duplicate top of stack: pop, push, push
            buffer.emitPop(Reg::RAX);
            buffer.emitPush(Reg::RAX);
            buffer.emitPush(Reg::RAX);
            break;
            
        case Opcode::OP_SWAP:
            // Swap top two: pop both, push in reverse
            buffer.emitPop(Reg::RAX);
            buffer.emitPop(Reg::RDX);
            buffer.emitPush(Reg::RAX);
            buffer.emitPush(Reg::RDX);
            break;
            
        // =====================================================================
        // Constants
        // =====================================================================
        case Opcode::OP_CONSTANT: {
            // Push constant from pool (index in next byte)
            uint8_t constIndex = bytecode[ip++];
            // For now, push the index as a placeholder
            // Real implementation would load from constant pool
            buffer.emitMovImm64(Reg::RAX, constIndex);
            buffer.emitPush(Reg::RAX);
            break;
        }
            
        case Opcode::OP_NIL:
            // Push undefined (encoded as special NaN-boxed value)
            // TAG_UNDEFINED = 0x7FF8000000000000 | 0x0007000000000000
            buffer.emitMovImm64(Reg::RAX, 0x7FFF000000000000ULL);
            buffer.emitPush(Reg::RAX);
            break;
            
        case Opcode::OP_TRUE:
            // Push true (NaN-boxed boolean)
            buffer.emitMovImm64(Reg::RAX, 0x7FFB000000000000ULL);
            buffer.emitPush(Reg::RAX);
            break;
            
        case Opcode::OP_FALSE:
            // Push false (NaN-boxed boolean)
            buffer.emitMovImm64(Reg::RAX, 0x7FFA000000000000ULL);
            buffer.emitPush(Reg::RAX);
            break;
            
        case Opcode::OP_ZERO:
            // Push 0.0 (IEEE 754 double)
            buffer.emitMovImm64(Reg::RAX, 0x0000000000000000ULL);
            buffer.emitPush(Reg::RAX);
            break;
            
        case Opcode::OP_ONE:
            // Push 1.0 (IEEE 754 double)
            buffer.emitMovImm64(Reg::RAX, 0x3FF0000000000000ULL);
            buffer.emitPush(Reg::RAX);
            break;
            
        // =====================================================================
        // Arithmetic (integer path - full impl needs fp handling)
        // =====================================================================
        case Opcode::OP_ADD:
            // Pop two, add, push result
            buffer.emitPop(Reg::RDX);   // right operand
            buffer.emitPop(Reg::RAX);   // left operand
            buffer.emitAdd(Reg::RAX, Reg::RDX);
            buffer.emitPush(Reg::RAX);
            break;
            
        case Opcode::OP_SUBTRACT:
            // Pop two, subtract, push result
            buffer.emitPop(Reg::RDX);   // right operand
            buffer.emitPop(Reg::RAX);   // left operand
            buffer.emitSub(Reg::RAX, Reg::RDX);
            buffer.emitPush(Reg::RAX);
            break;
            
        case Opcode::OP_NEGATE: {
            // Negate top of stack: pop, negate, push
            buffer.emitPop(Reg::RAX);
            // neg rax: REX.W + F7 /3
            buffer.emit8(0x48);  // REX.W
            buffer.emit8(0xF7);  // NEG r/m64
            buffer.emit8(0xD8);  // ModR/M: reg=3 (neg), r/m=0 (rax)
            buffer.emitPush(Reg::RAX);
            break;
        }
            
        case Opcode::OP_MULTIPLY: {
            // Pop two, multiply, push result
            buffer.emitPop(Reg::RDX);   // right operand
            buffer.emitPop(Reg::RAX);   // left operand
            // imul rax, rdx: REX.W + 0F AF /r
            buffer.emit8(0x48);  // REX.W
            buffer.emit8(0x0F);
            buffer.emit8(0xAF);
            buffer.emit8(0xC2);  // ModR/M: rax *= rdx
            buffer.emitPush(Reg::RAX);
            break;
        }
            
        // =====================================================================
        // Local Variables
        // =====================================================================
        case Opcode::OP_GET_LOCAL: {
            // Load local variable from stack frame
            uint8_t slot = bytecode[ip++];
            // mov rax, [rbp - 8*(slot+1)]
            buffer.emit8(0x48);  // REX.W
            buffer.emit8(0x8B);  // MOV r64, r/m64
            buffer.emit8(0x45);  // ModR/M: [rbp + disp8]
            buffer.emit8(static_cast<uint8_t>(-8 * (slot + 1)));
            buffer.emitPush(Reg::RAX);
            break;
        }
            
        case Opcode::OP_SET_LOCAL: {
            // Store top of stack to local variable
            uint8_t slot = bytecode[ip++];
            buffer.emitPop(Reg::RAX);
            // mov [rbp - 8*(slot+1)], rax
            buffer.emit8(0x48);  // REX.W
            buffer.emit8(0x89);  // MOV r/m64, r64
            buffer.emit8(0x45);  // ModR/M: [rbp + disp8]
            buffer.emit8(static_cast<uint8_t>(-8 * (slot + 1)));
            break;
        }
            
        // =====================================================================
        // Control Flow
        // =====================================================================
        case Opcode::OP_RETURN:
            // Epilogue handled by compile(), but support early return
            buffer.emitPop(Reg::RBP);
            buffer.emitRet();
            break;
            
        case Opcode::OP_JUMP: {
            // Unconditional jump (16-bit offset)
            int16_t offset = static_cast<int16_t>(bytecode[ip] | (bytecode[ip + 1] << 8));
            ip += 2;
            // jmp rel32: E9 + rel32
            buffer.emit8(0xE9);
            buffer.emit32(static_cast<uint32_t>(offset - 5));  // Adjust for instruction size
            break;
        }
            
        case Opcode::OP_JUMP_IF_FALSE: {
            // Jump if top of stack is falsy
            int16_t offset = static_cast<int16_t>(bytecode[ip] | (bytecode[ip + 1] << 8));
            ip += 2;
            buffer.emitPop(Reg::RAX);
            // test rax, rax
            buffer.emit8(0x48);  // REX.W
            buffer.emit8(0x85);  // TEST r/m64, r64
            buffer.emit8(0xC0);  // ModR/M: rax, rax
            // jz rel32: 0F 84 + rel32
            buffer.emit8(0x0F);
            buffer.emit8(0x84);
            buffer.emit32(static_cast<uint32_t>(offset - 9));  // Adjust for instruction size
            break;
        }
            
        case Opcode::OP_JUMP_IF_TRUE: {
            // Jump if top of stack is truthy
            int16_t offset = static_cast<int16_t>(bytecode[ip] | (bytecode[ip + 1] << 8));
            ip += 2;
            buffer.emitPop(Reg::RAX);
            // test rax, rax
            buffer.emit8(0x48);  // REX.W
            buffer.emit8(0x85);  // TEST
            buffer.emit8(0xC0);  // rax, rax
            // jnz rel32: 0F 85 + rel32
            buffer.emit8(0x0F);
            buffer.emit8(0x85);
            buffer.emit32(static_cast<uint32_t>(offset - 9));
            break;
        }
            
        case Opcode::OP_LOOP: {
            // Backwards jump (for loops)
            int16_t offset = static_cast<int16_t>(bytecode[ip] | (bytecode[ip + 1] << 8));
            ip += 2;
            // jmp rel32 (negative offset)
            buffer.emit8(0xE9);
            buffer.emit32(static_cast<uint32_t>(-offset - 5));
            break;
        }
            
        // =====================================================================
        // Comparison Operations
        // =====================================================================
        case Opcode::OP_EQUAL:
        case Opcode::OP_STRICT_EQUAL: {
            // Compare two values for equality
            buffer.emitPop(Reg::RDX);   // right
            buffer.emitPop(Reg::RAX);   // left
            // cmp rax, rdx
            buffer.emit8(0x48);  // REX.W
            buffer.emit8(0x39);  // CMP r/m64, r64
            buffer.emit8(0xD0);  // ModR/M: rax, rdx
            // sete al (set if equal)
            buffer.emit8(0x0F);
            buffer.emit8(0x94);
            buffer.emit8(0xC0);  // al
            // movzx rax, al (zero-extend)
            buffer.emit8(0x48);
            buffer.emit8(0x0F);
            buffer.emit8(0xB6);
            buffer.emit8(0xC0);
            buffer.emitPush(Reg::RAX);
            break;
        }
            
        case Opcode::OP_NOT_EQUAL:
        case Opcode::OP_STRICT_NOT_EQUAL: {
            // Compare for inequality
            buffer.emitPop(Reg::RDX);
            buffer.emitPop(Reg::RAX);
            // cmp rax, rdx
            buffer.emit8(0x48);
            buffer.emit8(0x39);
            buffer.emit8(0xD0);
            // setne al
            buffer.emit8(0x0F);
            buffer.emit8(0x95);
            buffer.emit8(0xC0);
            // movzx rax, al
            buffer.emit8(0x48);
            buffer.emit8(0x0F);
            buffer.emit8(0xB6);
            buffer.emit8(0xC0);
            buffer.emitPush(Reg::RAX);
            break;
        }
            
        case Opcode::OP_LESS: {
            buffer.emitPop(Reg::RDX);   // right
            buffer.emitPop(Reg::RAX);   // left
            // cmp rax, rdx
            buffer.emit8(0x48);
            buffer.emit8(0x39);
            buffer.emit8(0xD0);
            // setl al (set if less, signed)
            buffer.emit8(0x0F);
            buffer.emit8(0x9C);
            buffer.emit8(0xC0);
            // movzx rax, al
            buffer.emit8(0x48);
            buffer.emit8(0x0F);
            buffer.emit8(0xB6);
            buffer.emit8(0xC0);
            buffer.emitPush(Reg::RAX);
            break;
        }
            
        case Opcode::OP_LESS_EQUAL: {
            buffer.emitPop(Reg::RDX);
            buffer.emitPop(Reg::RAX);
            // cmp rax, rdx
            buffer.emit8(0x48);
            buffer.emit8(0x39);
            buffer.emit8(0xD0);
            // setle al
            buffer.emit8(0x0F);
            buffer.emit8(0x9E);
            buffer.emit8(0xC0);
            // movzx rax, al
            buffer.emit8(0x48);
            buffer.emit8(0x0F);
            buffer.emit8(0xB6);
            buffer.emit8(0xC0);
            buffer.emitPush(Reg::RAX);
            break;
        }
            
        case Opcode::OP_GREATER: {
            buffer.emitPop(Reg::RDX);
            buffer.emitPop(Reg::RAX);
            // cmp rax, rdx
            buffer.emit8(0x48);
            buffer.emit8(0x39);
            buffer.emit8(0xD0);
            // setg al
            buffer.emit8(0x0F);
            buffer.emit8(0x9F);
            buffer.emit8(0xC0);
            // movzx rax, al
            buffer.emit8(0x48);
            buffer.emit8(0x0F);
            buffer.emit8(0xB6);
            buffer.emit8(0xC0);
            buffer.emitPush(Reg::RAX);
            break;
        }
            
        case Opcode::OP_GREATER_EQUAL: {
            buffer.emitPop(Reg::RDX);
            buffer.emitPop(Reg::RAX);
            // cmp rax, rdx
            buffer.emit8(0x48);
            buffer.emit8(0x39);
            buffer.emit8(0xD0);
            // setge al
            buffer.emit8(0x0F);
            buffer.emit8(0x9D);
            buffer.emit8(0xC0);
            // movzx rax, al
            buffer.emit8(0x48);
            buffer.emit8(0x0F);
            buffer.emit8(0xB6);
            buffer.emit8(0xC0);
            buffer.emitPush(Reg::RAX);
            break;
        }
            
        // =====================================================================
        // Logical Operations
        // =====================================================================
        case Opcode::OP_NOT: {
            // Logical NOT: pop, negate truthiness, push
            buffer.emitPop(Reg::RAX);
            // test rax, rax
            buffer.emit8(0x48);
            buffer.emit8(0x85);
            buffer.emit8(0xC0);
            // sete al (set if zero, i.e., falsy)
            buffer.emit8(0x0F);
            buffer.emit8(0x94);
            buffer.emit8(0xC0);
            // movzx rax, al
            buffer.emit8(0x48);
            buffer.emit8(0x0F);
            buffer.emit8(0xB6);
            buffer.emit8(0xC0);
            buffer.emitPush(Reg::RAX);
            break;
        }
            
        // =====================================================================
        // Fallback - unhandled opcodes emit nop (Tier 2 will handle)
        // =====================================================================
        default:
            buffer.emitNop();
            break;
    }
}

} // namespace Zepra::JIT
