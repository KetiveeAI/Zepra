#pragma once

/**
 * @file baseline_jit.hpp
 * @brief Baseline JIT compiler for ZepraScript
 * 
 * Simple one-pass JIT that converts bytecode to native machine code.
 * Generates straightforward code with minimal optimization.
 */

#include "../config.hpp"
#include "../bytecode/opcode.hpp"
#include <cstdint>
#include <vector>
#include <memory>

namespace Zepra::Bytecode { class BytecodeChunk; }
namespace Zepra::Runtime { class Function; class VM; }

namespace Zepra::JIT {

/**
 * @brief Compiled native code block
 */
class CompiledCode {
public:
    CompiledCode(void* code, size_t size);
    ~CompiledCode();
    
    // Execute the compiled code
    void* entryPoint() const { return code_; }
    size_t size() const { return size_; }
    
    // Prevent copying (owns executable memory)
    CompiledCode(const CompiledCode&) = delete;
    CompiledCode& operator=(const CompiledCode&) = delete;
    
private:
    void* code_;
    size_t size_;
};

/**
 * @brief Code buffer for generating machine code
 */
class CodeBuffer {
public:
    CodeBuffer(size_t initialCapacity = 4096);
    
    // Emit bytes
    void emit8(uint8_t byte);
    void emit16(uint16_t value);
    void emit32(uint32_t value);
    void emit64(uint64_t value);
    
    // Emit x86-64 instructions
    void emitPush(int reg);        // push reg
    void emitPop(int reg);         // pop reg
    void emitMov(int dst, int src);// mov dst, src
    void emitMovImm64(int reg, uint64_t value); // mov reg, imm64
    void emitAdd(int dst, int src);// add dst, src
    void emitSub(int dst, int src);// sub dst, src
    void emitRet();                // ret
    void emitCall(void* target);   // call target
    void emitNop();                // nop
    
    // Get raw buffer
    const std::vector<uint8_t>& buffer() const { return buffer_; }
    size_t size() const { return buffer_.size(); }
    
    // Finalize to executable memory
    std::unique_ptr<CompiledCode> finalize();
    
private:
    std::vector<uint8_t> buffer_;
};

/**
 * @brief Baseline JIT compiler
 * 
 * Compiles bytecode to native machine code with minimal optimization.
 * Focus on fast compile time rather than optimal code.
 */
class BaselineJIT {
public:
    BaselineJIT(Runtime::VM* vm);
    
    /**
     * @brief Compile a bytecode chunk to native code
     * @return Compiled native code, or nullptr on failure
     */
    std::unique_ptr<CompiledCode> compile(const Bytecode::BytecodeChunk* chunk);
    
    /**
     * @brief Check if JIT is available on this platform
     */
    static bool isAvailable();
    
    /**
     * @brief Statistics
     */
    size_t functionsCompiled() const { return functionsCompiled_; }
    size_t bytesGenerated() const { return bytesGenerated_; }
    
private:
    // Compile individual opcodes
    void compileOpcode(CodeBuffer& buffer, Bytecode::Opcode op, 
                       const uint8_t* bytecode, size_t& ip);
    
    Runtime::VM* vm_;
    size_t functionsCompiled_ = 0;
    size_t bytesGenerated_ = 0;
};

// x86-64 register encoding
namespace Reg {
    constexpr int RAX = 0;
    constexpr int RCX = 1;
    constexpr int RDX = 2;
    constexpr int RBX = 3;
    constexpr int RSP = 4;
    constexpr int RBP = 5;
    constexpr int RSI = 6;
    constexpr int RDI = 7;
    constexpr int R8 = 8;
    constexpr int R9 = 9;
    constexpr int R10 = 10;
    constexpr int R11 = 11;
}

} // namespace Zepra::JIT
