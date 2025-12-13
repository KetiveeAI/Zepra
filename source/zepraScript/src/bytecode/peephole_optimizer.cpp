/**
 * @file peephole_optimizer.cpp
 * @brief Bytecode peephole optimizer implementation
 */

#include "zeprascript/bytecode/peephole_optimizer.hpp"
#include "zeprascript/bytecode/bytecode_generator.hpp"
#include <algorithm>

namespace Zepra::Bytecode {

OptimizationStats PeepholeOptimizer::optimize(BytecodeChunk* chunk) {
    OptimizationStats stats;
    
    if (!chunk) return stats;
    
    // Get mutable access to bytecode
    // Note: BytecodeChunk needs a mutableCode() method or we work with a copy
    // For now, we'll note patterns that could be optimized
    // Full in-place optimization requires BytecodeChunk modification
    
    const auto& code = chunk->code();
    size_t originalSize = code.size();
    
    // Single pass - identify optimization opportunities
    for (size_t i = 0; i + 1 < code.size(); ++i) {
        Opcode current = static_cast<Opcode>(code[i]);
        Opcode next = static_cast<Opcode>(code[i + 1]);
        
        // Pattern: OP_DUP followed by OP_POP = redundant
        if (current == Opcode::OP_DUP && next == Opcode::OP_POP) {
            stats.redundantPops++;
            stats.totalBytesRemoved += 2;
        }
        
        // Pattern: Any push followed immediately by OP_POP
        if (next == Opcode::OP_POP) {
            // Check if current is a side-effect-free operation
            if (current == Opcode::OP_CONSTANT || 
                current == Opcode::OP_NIL ||
                current == Opcode::OP_TRUE ||
                current == Opcode::OP_FALSE ||
                current == Opcode::OP_ZERO ||
                current == Opcode::OP_ONE ||
                current == Opcode::OP_GET_LOCAL) {
                stats.redundantPops++;
                // Account for operand bytes
                if (current == Opcode::OP_CONSTANT || 
                    current == Opcode::OP_GET_LOCAL) {
                    stats.totalBytesRemoved += 3; // opcode + operand + pop
                    i++; // Skip operand
                } else {
                    stats.totalBytesRemoved += 2;
                }
            }
        }
        
        // Pattern: OP_JUMP with offset 0 (jump to next instruction)
        if (current == Opcode::OP_JUMP && i + 2 < code.size()) {
            uint16_t offset = (static_cast<uint16_t>(code[i + 1]) << 8) | code[i + 2];
            if (offset == 0) {
                stats.deadJumps++;
                stats.totalBytesRemoved += 3;
            }
        }
        
        // Pattern: OP_ZERO followed by OP_ADD = no-op (adding 0)
        if (current == Opcode::OP_ZERO && next == Opcode::OP_ADD) {
            stats.constantFolding++;
            stats.totalBytesRemoved += 2;
        }
        
        // Pattern: OP_ONE followed by OP_ADD = OP_INCREMENT
        if (current == Opcode::OP_ONE && next == Opcode::OP_ADD) {
            stats.constantFolding++;
            // This one replaces 2 ops with 1, so 1 byte saved
            stats.totalBytesRemoved += 1;
        }
        
        // Pattern: OP_ONE followed by OP_SUBTRACT = OP_DECREMENT
        if (current == Opcode::OP_ONE && next == Opcode::OP_SUBTRACT) {
            stats.constantFolding++;
            stats.totalBytesRemoved += 1;
        }
    }
    
    return stats;
}

bool PeepholeOptimizer::eliminateRedundantPushPop(std::vector<uint8_t>& code, size_t& i) {
    if (i + 1 >= code.size()) return false;
    
    Opcode current = static_cast<Opcode>(code[i]);
    Opcode next = static_cast<Opcode>(code[i + 1]);
    
    // Check for push-like operation followed by pop
    if (next == Opcode::OP_POP) {
        // Only eliminate if the push has no side effects
        switch (current) {
            case Opcode::OP_NIL:
            case Opcode::OP_TRUE:
            case Opcode::OP_FALSE:
            case Opcode::OP_ZERO:
            case Opcode::OP_ONE:
            case Opcode::OP_DUP:
                nopOut(code, i, 2);
                return true;
            default:
                break;
        }
    }
    
    return false;
}

bool PeepholeOptimizer::eliminateDeadJump(std::vector<uint8_t>& code, size_t& i) {
    if (i + 2 >= code.size()) return false;
    
    if (static_cast<Opcode>(code[i]) == Opcode::OP_JUMP) {
        uint16_t offset = (static_cast<uint16_t>(code[i + 1]) << 8) | code[i + 2];
        if (offset == 0) {
            nopOut(code, i, 3);
            return true;
        }
    }
    
    return false;
}

bool PeepholeOptimizer::foldConstantOps(std::vector<uint8_t>& code, size_t& i) {
    if (i + 1 >= code.size()) return false;
    
    Opcode current = static_cast<Opcode>(code[i]);
    Opcode next = static_cast<Opcode>(code[i + 1]);
    
    // OP_ZERO + OP_ADD = nothing (adding 0)
    if (current == Opcode::OP_ZERO && next == Opcode::OP_ADD) {
        nopOut(code, i, 2);
        return true;
    }
    
    // OP_ONE + OP_ADD = OP_INCREMENT
    if (current == Opcode::OP_ONE && next == Opcode::OP_ADD) {
        code[i] = static_cast<uint8_t>(Opcode::OP_INCREMENT);
        code[i + 1] = static_cast<uint8_t>(Opcode::OP_NOP);
        return true;
    }
    
    // OP_ONE + OP_SUBTRACT = OP_DECREMENT
    if (current == Opcode::OP_ONE && next == Opcode::OP_SUBTRACT) {
        code[i] = static_cast<uint8_t>(Opcode::OP_DECREMENT);
        code[i + 1] = static_cast<uint8_t>(Opcode::OP_NOP);
        return true;
    }
    
    return false;
}

void PeepholeOptimizer::nopOut(std::vector<uint8_t>& code, size_t start, size_t count) {
    for (size_t i = 0; i < count && start + i < code.size(); ++i) {
        code[start + i] = static_cast<uint8_t>(Opcode::OP_NOP);
    }
}

} // namespace Zepra::Bytecode
