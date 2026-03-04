/**
 * @file DFGOpcode.h
 * @brief DFG Graph Opcodes for ZepraScript Optimizing JIT
 * 
 * Defines all operations in the DFG intermediate representation.
 * These are higher-level than machine instructions but lower than bytecode.
 */

#pragma once

#include <cstdint>

namespace Zepra::DFG {

// =============================================================================
// DFG Opcode Definitions
// =============================================================================

enum class Opcode : uint16_t {
    // =========================================================================
    // Control Flow
    // =========================================================================
    Phi,                // SSA phi node
    Jump,               // Unconditional jump
    Branch,             // Conditional branch
    Switch,             // Multi-way branch
    Return,             // Function return
    Unreachable,        // Trap / unreachable
    
    // =========================================================================
    // Constants
    // =========================================================================
    Const32,            // 32-bit integer constant
    Const64,            // 64-bit integer constant
    ConstF32,           // 32-bit float constant
    ConstF64,           // 64-bit float constant
    
    // =========================================================================
    // Parameters and Locals
    // =========================================================================
    Parameter,          // Function parameter
    GetLocal,           // Load local variable
    SetLocal,           // Store local variable
    
    // =========================================================================
    // Integer Arithmetic (32-bit)
    // =========================================================================
    Add32,
    Sub32,
    Mul32,
    Div32S,
    Div32U,
    Rem32S,
    Rem32U,
    
    // =========================================================================
    // Integer Arithmetic (64-bit)
    // =========================================================================
    Add64,
    Sub64,
    Mul64,
    Div64S,
    Div64U,
    Rem64S,
    Rem64U,
    
    // =========================================================================
    // Bitwise Operations (32-bit)
    // =========================================================================
    And32,
    Or32,
    Xor32,
    Shl32,
    Shr32S,
    Shr32U,
    Rotl32,
    Rotr32,
    
    // =========================================================================
    // Bitwise Operations (64-bit)
    // =========================================================================
    And64,
    Or64,
    Xor64,
    Shl64,
    Shr64S,
    Shr64U,
    Rotl64,
    Rotr64,
    
    // =========================================================================
    // Unary Integer Operations
    // =========================================================================
    Clz32,
    Ctz32,
    Popcnt32,
    Clz64,
    Ctz64,
    Popcnt64,
    
    // =========================================================================
    // Comparisons (produce i32 0 or 1)
    // =========================================================================
    Eq32,
    Ne32,
    Lt32S,
    Lt32U,
    Le32S,
    Le32U,
    Gt32S,
    Gt32U,
    Ge32S,
    Ge32U,
    
    Eq64,
    Ne64,
    Lt64S,
    Lt64U,
    Le64S,
    Le64U,
    Gt64S,
    Gt64U,
    Ge64S,
    Ge64U,
    
    Eqz32,
    Eqz64,
    
    // =========================================================================
    // Float Arithmetic (32-bit)
    // =========================================================================
    AddF32,
    SubF32,
    MulF32,
    DivF32,
    SqrtF32,
    AbsF32,
    NegF32,
    CeilF32,
    FloorF32,
    TruncF32,
    NearestF32,
    MinF32,
    MaxF32,
    CopysignF32,
    
    // =========================================================================
    // Float Arithmetic (64-bit)
    // =========================================================================
    AddF64,
    SubF64,
    MulF64,
    DivF64,
    SqrtF64,
    AbsF64,
    NegF64,
    CeilF64,
    FloorF64,
    TruncF64,
    NearestF64,
    MinF64,
    MaxF64,
    CopysignF64,
    
    // =========================================================================
    // Float Comparisons
    // =========================================================================
    EqF32,
    NeF32,
    LtF32,
    LeF32,
    GtF32,
    GeF32,
    
    EqF64,
    NeF64,
    LtF64,
    LeF64,
    GtF64,
    GeF64,
    
    // =========================================================================
    // Conversions
    // =========================================================================
    Wrap64To32,         // i64 -> i32
    Extend32STo64,      // i32 -> i64 (signed)
    Extend32UTo64,      // i32 -> i64 (unsigned)
    
    TruncF32ToI32S,
    TruncF32ToI32U,
    TruncF64ToI32S,
    TruncF64ToI32U,
    TruncF32ToI64S,
    TruncF32ToI64U,
    TruncF64ToI64S,
    TruncF64ToI64U,
    
    ConvertI32SToF32,
    ConvertI32UToF32,
    ConvertI64SToF32,
    ConvertI64UToF32,
    ConvertI32SToF64,
    ConvertI32UToF64,
    ConvertI64SToF64,
    ConvertI64UToF64,
    
    DemoteF64ToF32,
    PromoteF32ToF64,
    
    ReinterpretI32AsF32,
    ReinterpretI64AsF64,
    ReinterpretF32AsI32,
    ReinterpretF64AsI64,
    
    // =========================================================================
    // Memory Operations
    // =========================================================================
    Load8S,             // Sign-extended 8-bit load
    Load8U,             // Zero-extended 8-bit load
    Load16S,
    Load16U,
    Load32,
    Load64,
    LoadF32,
    LoadF64,
    
    Store8,
    Store16,
    Store32,
    Store64,
    StoreF32,
    StoreF64,
    
    // =========================================================================
    // Memory Management
    // =========================================================================
    MemorySize,
    MemoryGrow,
    
    // =========================================================================
    // Calls
    // =========================================================================
    Call,               // Direct function call
    CallIndirect,       // Indirect function call via table
    
    // =========================================================================
    // Stack Operations
    // =========================================================================
    Drop,               // Discard value
    Select,             // Conditional select (ternary)
    
    // =========================================================================
    // Bounds Check (for optimization)
    // =========================================================================
    BoundsCheck,        // Memory bounds check
    CheckNotNull,       // Null reference check
    
    // =========================================================================
    // OSR (On-Stack Replacement)
    // =========================================================================
    OSREntry,           // OSR entry point
    InvalidatableCheck, // Check that may trigger deoptimization
    
    // =========================================================================
    // Sentinel
    // =========================================================================
    NumOpcodes
};

// =============================================================================
// Opcode Properties
// =============================================================================

inline bool isTerminator(Opcode op) {
    switch (op) {
        case Opcode::Jump:
        case Opcode::Branch:
        case Opcode::Switch:
        case Opcode::Return:
        case Opcode::Unreachable:
            return true;
        default:
            return false;
    }
}

inline bool hasResult(Opcode op) {
    switch (op) {
        case Opcode::Jump:
        case Opcode::Branch:
        case Opcode::Switch:
        case Opcode::Return:
        case Opcode::Unreachable:
        case Opcode::SetLocal:
        case Opcode::Store8:
        case Opcode::Store16:
        case Opcode::Store32:
        case Opcode::Store64:
        case Opcode::StoreF32:
        case Opcode::StoreF64:
        case Opcode::Drop:
            return false;
        default:
            return true;
    }
}

inline bool isBinaryOp(Opcode op) {
    switch (op) {
        case Opcode::Add32: case Opcode::Sub32: case Opcode::Mul32:
        case Opcode::Div32S: case Opcode::Div32U:
        case Opcode::Rem32S: case Opcode::Rem32U:
        case Opcode::Add64: case Opcode::Sub64: case Opcode::Mul64:
        case Opcode::Div64S: case Opcode::Div64U:
        case Opcode::Rem64S: case Opcode::Rem64U:
        case Opcode::And32: case Opcode::Or32: case Opcode::Xor32:
        case Opcode::Shl32: case Opcode::Shr32S: case Opcode::Shr32U:
        case Opcode::Rotl32: case Opcode::Rotr32:
        case Opcode::And64: case Opcode::Or64: case Opcode::Xor64:
        case Opcode::Shl64: case Opcode::Shr64S: case Opcode::Shr64U:
        case Opcode::Rotl64: case Opcode::Rotr64:
        case Opcode::AddF32: case Opcode::SubF32: case Opcode::MulF32: case Opcode::DivF32:
        case Opcode::AddF64: case Opcode::SubF64: case Opcode::MulF64: case Opcode::DivF64:
        case Opcode::MinF32: case Opcode::MaxF32: case Opcode::CopysignF32:
        case Opcode::MinF64: case Opcode::MaxF64: case Opcode::CopysignF64:
            return true;
        default:
            return false;
    }
}

inline bool isCommutative(Opcode op) {
    switch (op) {
        case Opcode::Add32: case Opcode::Mul32:
        case Opcode::Add64: case Opcode::Mul64:
        case Opcode::And32: case Opcode::Or32: case Opcode::Xor32:
        case Opcode::And64: case Opcode::Or64: case Opcode::Xor64:
        case Opcode::AddF32: case Opcode::MulF32:
        case Opcode::AddF64: case Opcode::MulF64:
        case Opcode::Eq32: case Opcode::Ne32:
        case Opcode::Eq64: case Opcode::Ne64:
        case Opcode::EqF32: case Opcode::NeF32:
        case Opcode::EqF64: case Opcode::NeF64:
            return true;
        default:
            return false;
    }
}

inline bool hasSideEffects(Opcode op) {
    switch (op) {
        case Opcode::Call:
        case Opcode::CallIndirect:
        case Opcode::Store8:
        case Opcode::Store16:
        case Opcode::Store32:
        case Opcode::Store64:
        case Opcode::StoreF32:
        case Opcode::StoreF64:
        case Opcode::MemoryGrow:
            return true;
        default:
            return false;
    }
}

inline const char* opcodeName(Opcode op) {
    switch (op) {
        case Opcode::Phi: return "Phi";
        case Opcode::Jump: return "Jump";
        case Opcode::Branch: return "Branch";
        case Opcode::Return: return "Return";
        case Opcode::Const32: return "Const32";
        case Opcode::Const64: return "Const64";
        case Opcode::ConstF32: return "ConstF32";
        case Opcode::ConstF64: return "ConstF64";
        case Opcode::Add32: return "Add32";
        case Opcode::Sub32: return "Sub32";
        case Opcode::Mul32: return "Mul32";
        case Opcode::Call: return "Call";
        // ... (abbreviated for brevity)
        default: return "Unknown";
    }
}

} // namespace Zepra::DFG
