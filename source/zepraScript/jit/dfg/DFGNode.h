/**
 * @file DFGNode.h
 * @brief Data Flow Graph Node Definition
 * 
 * Represents an operation in the DFG Intermediate Representation.
 * Nodes uses Single Static Assignment (SSA) form concepts.
 */

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <iostream>

namespace Zepra::JIT::DFG {

// =============================================================================
// Node Types
// =============================================================================

enum class NodeType : uint8_t {
    // Control Flow
    Start,
    End,
    Jump,
    Branch,
    Return,
    Unreachable,
    
    // Constants
    Int32Constant,
    Int64Constant,
    Float32Constant,
    Float64Constant,
    
    // WASM Operations (Integer)
    Int32Add,
    Int32Sub,
    Int32Mul,
    Int32DivS,
    Int32DivU,
    
    // Memory
    Load,
    Store,
    
    // Variables
    GetLocal,
    SetLocal,
    Phi,        // SSA Phi node
    
    // Function Call
    Call,
    
    // Invalid
    Invalid
};

inline const char* nodeTypeToString(NodeType type) {
    switch (type) {
        case NodeType::Start: return "Start";
        case NodeType::End: return "End";
        case NodeType::Jump: return "Jump";
        case NodeType::Branch: return "Branch";
        case NodeType::Return: return "Return";
        case NodeType::Int32Constant: return "Int32Const";
        case NodeType::Int32Add: return "Int32Add";
        case NodeType::Load: return "Load";
        case NodeType::Store: return "Store";
        case NodeType::Phi: return "Phi";
        default: return "Unknown";
    }
}

// =============================================================================
// Node Definition
// =============================================================================

struct Node {
    uint32_t id;            // Unique ID in graph
    NodeType type;          // Operation type
    uint32_t vreg;          // Virtual register (result)
    
    // Operands (inputs) - References to other Node IDs
    std::vector<uint32_t> children;
    
    // Immediate values (for constants, etc.)
    union {
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
    } imm;
    
    bool hasSideEffects = false;
    
    Node(uint32_t id, NodeType type) : id(id), type(type), vreg(0) {}
};

} // namespace Zepra::JIT::DFG
