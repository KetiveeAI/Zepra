// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file peephole_optimizer.cpp
 * @brief Bytecode peephole optimizer implementation
 */

#include "bytecode/peephole_optimizer.hpp"
#include "bytecode/bytecode_generator.hpp"
#include <algorithm>

namespace Zepra::Bytecode {

OptimizationStats PeepholeOptimizer::optimize(BytecodeChunk* chunk) {
    OptimizationStats stats;
    
    if (!chunk) return stats;
    
    // Get a mutable copy of the bytecode for in-place optimization.
    // BytecodeChunk exposes const& code(), so we copy, modify, write back.
    std::vector<uint8_t> code(chunk->code().begin(), chunk->code().end());
    size_t originalSize = code.size();
    bool modified = false;
    
    // Single pass — apply all rewrite rules
    for (size_t i = 0; i + 1 < code.size(); ++i) {
        // Skip NOPs from previous rewrites
        if (static_cast<Opcode>(code[i]) == Opcode::OP_NOP) continue;
        
        if (eliminateRedundantPushPop(code, i)) {
            stats.redundantPops++;
            modified = true;
            continue;
        }
        
        if (eliminateDeadJump(code, i)) {
            stats.deadJumps++;
            modified = true;
            continue;
        }
        
        if (foldConstantOps(code, i)) {
            stats.constantFolding++;
            modified = true;
            continue;
        }
    }
    
    // Count bytes removed (NOPs inserted)
    if (modified) {
        for (size_t i = 0; i < code.size(); ++i) {
            if (static_cast<Opcode>(code[i]) == Opcode::OP_NOP) {
                stats.totalBytesRemoved++;
            }
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
