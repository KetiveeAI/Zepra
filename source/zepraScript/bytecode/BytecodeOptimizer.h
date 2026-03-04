/**
 * @file BytecodeOptimizer.h
 * @brief Bytecode-level optimizations
 * 
 * Implements:
 * - Peephole optimizations
 * - Constant folding
 * - Dead code elimination
 * - Jump threading
 * 
 * Based on V8/SpiderMonkey bytecode optimizer
 */

#pragma once

#include "OpcodeReference.h"
#include <vector>
#include <span>
#include <functional>

namespace Zepra::Bytecode {

// =============================================================================
// Bytecode Stream
// =============================================================================

struct Instruction {
    size_t offset;
    Opcode opcode;
    std::vector<uint8_t> operands;
    
    size_t length() const { return 1 + operands.size(); }
};

class BytecodeStream {
public:
    explicit BytecodeStream(std::vector<uint8_t>& code) : code_(code) {}
    
    bool hasMore() const { return pos_ < code_.size(); }
    size_t position() const { return pos_; }
    void seek(size_t pos) { pos_ = pos; }
    
    Instruction read();
    void write(const Instruction& instr);
    void remove(size_t offset, size_t length);
    void replace(size_t offset, const Instruction& instr);
    
    std::vector<uint8_t>& buffer() { return code_; }
    
private:
    std::vector<uint8_t>& code_;
    size_t pos_ = 0;
};

// =============================================================================
// Optimization Pass Interface
// =============================================================================

class OptimizationPass {
public:
    virtual ~OptimizationPass() = default;
    virtual const char* name() const = 0;
    virtual bool run(BytecodeStream& code) = 0;
};

// =============================================================================
// Peephole Optimizer
// =============================================================================

class PeepholeOptimizer : public OptimizationPass {
public:
    const char* name() const override { return "Peephole"; }
    
    bool run(BytecodeStream& code) override;
    
private:
    // Pattern: OP_NOT OP_NOT -> remove both
    bool optimizeDoubleNot(BytecodeStream& code, size_t offset);
    
    // Pattern: OP_POP after side-effect-free op -> remove both
    bool optimizeDeadPop(BytecodeStream& code, size_t offset);
    
    // Pattern: JUMP to JUMP -> direct jump
    bool optimizeJumpToJump(BytecodeStream& code, size_t offset);
    
    // Pattern: JUMP to next instruction -> remove jump
    bool optimizeRedundantJump(BytecodeStream& code, size_t offset);
    
    // Pattern: OP_TRUE OP_JUMP_IF_FALSE -> remove both
    bool optimizeConstantCondition(BytecodeStream& code, size_t offset);
    
    // Pattern: OP_GET_LOCAL N OP_GET_LOCAL N -> DUP
    bool optimizeDuplicateLoad(BytecodeStream& code, size_t offset);
    
    // Pattern: Consecutive constant arithmetic -> fold
    bool optimizeConstantArithmetic(BytecodeStream& code, size_t offset);
};

// =============================================================================
// Constant Folder
// =============================================================================

class ConstantFolder : public OptimizationPass {
public:
    const char* name() const override { return "ConstantFold"; }
    
    bool run(BytecodeStream& code) override;
    
private:
    struct ConstantValue {
        enum Type { None, Integer, Float, String, Boolean } type = None;
        union {
            int64_t i64;
            double f64;
            bool b;
        };
    };
    
    bool foldBinaryOp(Opcode op, ConstantValue a, ConstantValue b, ConstantValue& result);
    bool foldUnaryOp(Opcode op, ConstantValue a, ConstantValue& result);
};

// =============================================================================
// Dead Code Eliminator
// =============================================================================

class DeadCodeEliminator : public OptimizationPass {
public:
    const char* name() const override { return "DeadCode"; }
    
    bool run(BytecodeStream& code) override;
    
private:
    // Find unreachable code after unconditional jumps/returns
    std::vector<std::pair<size_t, size_t>> findDeadRanges(BytecodeStream& code);
    
    // Check if opcode is a terminator (ends basic block)
    bool isTerminator(Opcode op);
};

// =============================================================================
// Jump Threading
// =============================================================================

class JumpThreader : public OptimizationPass {
public:
    const char* name() const override { return "JumpThread"; }
    
    bool run(BytecodeStream& code) override;
    
private:
    // Follow jump chain and return ultimate target
    size_t followJumps(BytecodeStream& code, size_t target, size_t maxDepth = 10);
};

// =============================================================================
// Bytecode Optimizer Pipeline
// =============================================================================

struct OptimizationStats {
    size_t peepholeOptimizations = 0;
    size_t constantFolds = 0;
    size_t deadBytesRemoved = 0;
    size_t jumpsThreaded = 0;
    size_t originalSize = 0;
    size_t optimizedSize = 0;
};

class BytecodeOptimizer {
public:
    BytecodeOptimizer() {
        // Default pass order
        addPass<PeepholeOptimizer>();
        addPass<ConstantFolder>();
        addPass<JumpThreader>();
        addPass<DeadCodeEliminator>();
    }
    
    template<typename Pass>
    void addPass() {
        passes_.push_back(std::make_unique<Pass>());
    }
    
    void optimize(std::vector<uint8_t>& code);
    
    const OptimizationStats& stats() const { return stats_; }
    
    void setMaxIterations(size_t n) { maxIterations_ = n; }
    
private:
    std::vector<std::unique_ptr<OptimizationPass>> passes_;
    OptimizationStats stats_;
    size_t maxIterations_ = 3;
};

} // namespace Zepra::Bytecode
