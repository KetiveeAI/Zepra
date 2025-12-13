/**
 * @file baseline_jit.cpp
 * @brief Baseline JIT compiler implementation
 * 
 * Simple one-pass JIT that generates x86-64 machine code from bytecode.
 */

#include "zeprascript/jit/baseline_jit.hpp"
#include "zeprascript/bytecode/bytecode_generator.hpp"
#include "zeprascript/runtime/vm.hpp"
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
                                 const uint8_t*, size_t&) {
    using Bytecode::Opcode;
    
    switch (op) {
        case Opcode::OP_NOP:
            buffer.emitNop();
            break;
            
        case Opcode::OP_RETURN:
            // Already handled by epilogue
            break;
            
        // For now, emit nop for unhandled opcodes
        // TODO: Implement full opcode translation
        default:
            buffer.emitNop();
            break;
    }
}

} // namespace Zepra::JIT
