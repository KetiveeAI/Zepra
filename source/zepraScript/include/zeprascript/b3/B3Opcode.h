/**
 * @file B3Opcode.h
 * @brief B3 Backend IR Opcodes
 * 
 * Low-level SSA IR optimized for machine code generation.
 * B3 operates closer to hardware than DFG.
 */

#pragma once

#include <cstdint>

namespace Zepra::B3 {

enum class Opcode : uint8_t {
    // No-op
    Nop,
    Identity,
    
    // Constants
    Const32,
    Const64,
    ConstFloat,
    ConstDouble,
    
    // Polymorphic arithmetic
    Add,
    Sub,
    Mul,
    Div,
    UDiv,
    Mod,
    UMod,
    Neg,
    
    // Bitwise operations
    BitAnd,
    BitOr,
    BitXor,
    Shl,
    SShr,      // Arithmetic shift right
    ZShr,      // Logical shift right
    RotR,
    RotL,
    Clz,       // Count leading zeros
    
    // Floating point
    Abs,
    Ceil,
    Floor,
    FTrunc,
    Sqrt,
    FMin,
    FMax,
    
    // Type conversions
    BitwiseCast,
    SExt8,
    SExt16,
    SExt32,
    ZExt32,
    Trunc,
    IToD,      // Int to double
    IToF,      // Int to float
    FloatToDouble,
    DoubleToFloat,
    
    // Comparisons (return i32 0 or 1)
    Equal,
    NotEqual,
    LessThan,
    GreaterThan,
    LessEqual,
    GreaterEqual,
    Above,     // Unsigned >
    Below,     // Unsigned <
    AboveEqual,
    BelowEqual,
    
    // Select (ternary)
    Select,
    
    // Memory operations
    Load8Z,    // Zero extend
    Load8S,    // Sign extend
    Load16Z,
    Load16S,
    Load,
    Store8,
    Store16,
    Store,
    
    // Atomics
    AtomicXchg,
    AtomicXchgAdd,
    AtomicCmpXchg,
    Fence,
    
    // WASM-specific
    WasmAddress,
    WasmBoundsCheck,
    
    // SSA
    Phi,
    Upsilon,   // Phi input
    
    // Control flow
    Jump,
    Branch,
    Switch,
    Return,
    Oops,      // Unreachable
    
    // Calls
    CCall,
    Patchpoint,
    
    // Checked math (with overflow handling)
    CheckAdd,
    CheckSub,
    CheckMul,
};

// Type codes for B3 values
enum class Type : uint8_t {
    Void,
    Int32,
    Int64,
    Float,
    Double,
    Ptr,    // Pointer (Int64 on 64-bit)
};

inline bool isConstant(Opcode op) {
    return op == Opcode::Const32 || op == Opcode::Const64 ||
           op == Opcode::ConstFloat || op == Opcode::ConstDouble;
}

inline bool isTerminal(Opcode op) {
    return op == Opcode::Jump || op == Opcode::Branch ||
           op == Opcode::Switch || op == Opcode::Return ||
           op == Opcode::Oops;
}

inline bool isLoad(Opcode op) {
    return op == Opcode::Load8Z || op == Opcode::Load8S ||
           op == Opcode::Load16Z || op == Opcode::Load16S ||
           op == Opcode::Load;
}

inline bool isStore(Opcode op) {
    return op == Opcode::Store8 || op == Opcode::Store16 ||
           op == Opcode::Store;
}

inline bool isMemoryAccess(Opcode op) {
    return isLoad(op) || isStore(op);
}

inline bool hasResult(Opcode op) {
    if (isTerminal(op)) return false;
    if (isStore(op)) return false;
    if (op == Opcode::Fence) return false;
    return true;
}

} // namespace Zepra::B3
