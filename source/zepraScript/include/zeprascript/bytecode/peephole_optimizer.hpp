#pragma once

/**
 * @file peephole_optimizer.hpp
 * @brief Bytecode peephole optimizer for ZepraScript
 * 
 * Optimizes bytecode sequences in-place after compilation.
 * Zero additional memory allocation - modifies existing bytecode.
 */

#include "../config.hpp"
#include "opcode.hpp"
#include <vector>
#include <cstdint>

namespace Zepra::Bytecode {

class BytecodeChunk;

/**
 * @brief Statistics from peephole optimization pass
 */
struct OptimizationStats {
    size_t redundantPops = 0;       // OP_PUSH followed by OP_POP removed
    size_t deadJumps = 0;           // Jumps to next instruction removed
    size_t constantFolding = 0;     // Arithmetic on constants folded
    size_t totalBytesRemoved = 0;   // Total bytes eliminated
};

/**
 * @brief Peephole optimizer for bytecode
 * 
 * Performs single-pass optimization looking for common patterns.
 * All optimizations are semantics-preserving.
 */
class PeepholeOptimizer {
public:
    /**
     * @brief Optimize a bytecode chunk in-place
     * @param chunk The bytecode to optimize
     * @return Statistics about optimizations performed
     */
    static OptimizationStats optimize(BytecodeChunk* chunk);
    
private:
    /**
     * @brief Check and eliminate redundant push-pop pairs
     */
    static bool eliminateRedundantPushPop(std::vector<uint8_t>& code, size_t& i);
    
    /**
     * @brief Check and eliminate dead jumps (jump to next instruction)
     */
    static bool eliminateDeadJump(std::vector<uint8_t>& code, size_t& i);
    
    /**
     * @brief Check and fold constant increment/decrement operations
     */
    static bool foldConstantOps(std::vector<uint8_t>& code, size_t& i);
    
    /**
     * @brief Replace instruction with NOPs (preserves offsets)
     */
    static void nopOut(std::vector<uint8_t>& code, size_t start, size_t count);
};

} // namespace Zepra::Bytecode
