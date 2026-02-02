/**
 * @file WasmConstants.h
 * @brief WebAssembly constants, opcodes, type codes, and trap definitions
 * 
 * Based on WebAssembly 2.0 specification with proposal extensions.
 * Reference: Firefox SpiderMonkey WasmConstants.h
 */

#pragma once

#include <cstdint>

namespace Zepra::Wasm {

// =============================================================================
// Magic and Version
// =============================================================================

static constexpr uint32_t MagicNumber = 0x6d736100;  // "\0asm"
static constexpr uint32_t EncodingVersion = 0x01;

// =============================================================================
// Section IDs
// =============================================================================

enum class SectionId : uint8_t {
    Custom = 0,
    Type = 1,
    Import = 2,
    Function = 3,
    Table = 4,
    Memory = 5,
    Global = 6,
    Export = 7,
    Start = 8,
    Element = 9,
    Code = 10,
    Data = 11,
    DataCount = 12,
    Tag = 13,
};

// =============================================================================
// Type Codes (SLEB128 encoded in binary)
// =============================================================================

enum class TypeCode : uint8_t {
    // Numeric types
    I32 = 0x7F,         // SLEB128(-0x01)
    I64 = 0x7E,         // SLEB128(-0x02)
    F32 = 0x7D,         // SLEB128(-0x03)
    F64 = 0x7C,         // SLEB128(-0x04)
    V128 = 0x7B,        // SLEB128(-0x05) SIMD
    
    // Packed types (GC proposal)
    I8 = 0x78,          // SLEB128(-0x08)
    I16 = 0x77,         // SLEB128(-0x09)
    
    // Reference types
    FuncRef = 0x70,     // SLEB128(-0x10)
    ExternRef = 0x6F,   // SLEB128(-0x11)
    AnyRef = 0x6E,      // SLEB128(-0x12) GC
    EqRef = 0x6D,       // SLEB128(-0x13) GC
    I31Ref = 0x6C,      // SLEB128(-0x14) GC
    StructRef = 0x6B,   // SLEB128(-0x15) GC
    ArrayRef = 0x6A,    // SLEB128(-0x16) GC
    ExnRef = 0x69,      // SLEB128(-0x17) Exception handling
    
    // Null types
    NullAnyRef = 0x71,      // SLEB128(-0x0F)
    NullExternRef = 0x72,   // SLEB128(-0x0E)
    NullFuncRef = 0x73,     // SLEB128(-0x0D)
    NullExnRef = 0x74,      // SLEB128(-0x0C)
    
    // Type constructors
    Ref = 0x64,             // SLEB128(-0x1C) Non-nullable ref
    NullableRef = 0x63,     // SLEB128(-0x1D) Nullable ref
    Func = 0x60,            // SLEB128(-0x20) Function type
    Struct = 0x5F,          // SLEB128(-0x21) Struct type (GC)
    Array = 0x5E,           // SLEB128(-0x22) Array type (GC)
    
    // Special
    BlockVoid = 0x40,       // SLEB128(-0x40) Empty block type
    RecGroup = 0x4E,        // SLEB128(-0x31) Recursion group (GC)
    SubNoFinalType = 0x50,  // SLEB128(-0x30) Parent type (GC)
    SubFinalType = 0x4F,    // SLEB128(-0x32) Final type (GC)
    
    Limit = 0x80
};

// =============================================================================
// Definition Kinds (for imports/exports)
// =============================================================================

enum class DefinitionKind : uint8_t {
    Function = 0x00,
    Table = 0x01,
    Memory = 0x02,
    Global = 0x03,
    Tag = 0x04,
};

// =============================================================================
// Limits Flags
// =============================================================================

enum class LimitsFlags : uint8_t {
    Default = 0x0,
    HasMaximum = 0x1,
    IsShared = 0x2,
    IsI64 = 0x4,
};

// =============================================================================
// Segment Kinds
// =============================================================================

enum class DataSegmentKind : uint8_t {
    Active = 0x00,
    Passive = 0x01,
    ActiveWithMemoryIndex = 0x02
};

enum class ElemSegmentKind : uint8_t {
    Active = 0x0,
    Passive = 0x1,
    ActiveWithTableIndex = 0x2,
    Declared = 0x3,
};

enum class ElemSegmentPayload : uint8_t {
    Indices = 0x0,
    Expressions = 0x4,
};

// =============================================================================
// Traps (runtime errors)
// =============================================================================

enum class Trap : uint8_t {
    Unreachable,
    IntegerOverflow,
    InvalidConversionToInteger,
    IntegerDivideByZero,
    OutOfBounds,
    UnalignedAccess,
    IndirectCallToNull,
    IndirectCallBadSig,
    NullPointerDereference,
    BadCast,
    StackOverflow,
    OutOfMemory,
    CheckInterrupt,
    ThrowReported,
    
    Limit
};

inline const char* trapMessage(Trap trap) {
    switch (trap) {
        case Trap::Unreachable: return "unreachable executed";
        case Trap::IntegerOverflow: return "integer overflow";
        case Trap::InvalidConversionToInteger: return "invalid conversion to integer";
        case Trap::IntegerDivideByZero: return "integer divide by zero";
        case Trap::OutOfBounds: return "out of bounds memory access";
        case Trap::UnalignedAccess: return "unaligned atomic access";
        case Trap::IndirectCallToNull: return "indirect call to null";
        case Trap::IndirectCallBadSig: return "indirect call signature mismatch";
        case Trap::NullPointerDereference: return "null pointer dereference";
        case Trap::BadCast: return "cast failure";
        case Trap::StackOverflow: return "call stack exhausted";
        case Trap::OutOfMemory: return "out of memory";
        case Trap::CheckInterrupt: return "interrupt";
        case Trap::ThrowReported: return "exception thrown";
        default: return "unknown trap";
    }
}

// =============================================================================
// Opcodes (primary instruction set)
// =============================================================================

enum class Op : uint16_t {
    // Control flow (0x00-0x1F)
    Unreachable = 0x00,
    Nop = 0x01,
    Block = 0x02,
    Loop = 0x03,
    If = 0x04,
    Else = 0x05,
    Try = 0x06,
    Catch = 0x07,
    Throw = 0x08,
    Rethrow = 0x09,
    ThrowRef = 0x0A,
    End = 0x0B,
    Br = 0x0C,
    BrIf = 0x0D,
    BrTable = 0x0E,
    Return = 0x0F,
    
    // Calls
    Call = 0x10,
    CallIndirect = 0x11,
    ReturnCall = 0x12,
    ReturnCallIndirect = 0x13,
    CallRef = 0x14,
    ReturnCallRef = 0x15,
    
    // Exception handling
    Delegate = 0x18,
    CatchAll = 0x19,
    
    // Parametric
    Drop = 0x1A,
    SelectNumeric = 0x1B,
    SelectTyped = 0x1C,
    
    TryTable = 0x1F,
    
    // Variable access (0x20-0x26)
    LocalGet = 0x20,
    LocalSet = 0x21,
    LocalTee = 0x22,
    GlobalGet = 0x23,
    GlobalSet = 0x24,
    TableGet = 0x25,
    TableSet = 0x26,
    
    // Memory loads (0x28-0x35)
    I32Load = 0x28,
    I64Load = 0x29,
    F32Load = 0x2A,
    F64Load = 0x2B,
    I32Load8S = 0x2C,
    I32Load8U = 0x2D,
    I32Load16S = 0x2E,
    I32Load16U = 0x2F,
    I64Load8S = 0x30,
    I64Load8U = 0x31,
    I64Load16S = 0x32,
    I64Load16U = 0x33,
    I64Load32S = 0x34,
    I64Load32U = 0x35,
    
    // Memory stores (0x36-0x3E)
    I32Store = 0x36,
    I64Store = 0x37,
    F32Store = 0x38,
    F64Store = 0x39,
    I32Store8 = 0x3A,
    I32Store16 = 0x3B,
    I64Store8 = 0x3C,
    I64Store16 = 0x3D,
    I64Store32 = 0x3E,
    
    // Memory operations
    MemorySize = 0x3F,
    MemoryGrow = 0x40,
    
    // Constants (0x41-0x44)
    I32Const = 0x41,
    I64Const = 0x42,
    F32Const = 0x43,
    F64Const = 0x44,
    
    // i32 comparison (0x45-0x4F)
    I32Eqz = 0x45,
    I32Eq = 0x46,
    I32Ne = 0x47,
    I32LtS = 0x48,
    I32LtU = 0x49,
    I32GtS = 0x4A,
    I32GtU = 0x4B,
    I32LeS = 0x4C,
    I32LeU = 0x4D,
    I32GeS = 0x4E,
    I32GeU = 0x4F,
    
    // i64 comparison (0x50-0x5A)
    I64Eqz = 0x50,
    I64Eq = 0x51,
    I64Ne = 0x52,
    I64LtS = 0x53,
    I64LtU = 0x54,
    I64GtS = 0x55,
    I64GtU = 0x56,
    I64LeS = 0x57,
    I64LeU = 0x58,
    I64GeS = 0x59,
    I64GeU = 0x5A,
    
    // f32 comparison (0x5B-0x60)
    F32Eq = 0x5B,
    F32Ne = 0x5C,
    F32Lt = 0x5D,
    F32Gt = 0x5E,
    F32Le = 0x5F,
    F32Ge = 0x60,
    
    // f64 comparison (0x61-0x66)
    F64Eq = 0x61,
    F64Ne = 0x62,
    F64Lt = 0x63,
    F64Gt = 0x64,
    F64Le = 0x65,
    F64Ge = 0x66,
    
    // i32 arithmetic (0x67-0x78)
    I32Clz = 0x67,
    I32Ctz = 0x68,
    I32Popcnt = 0x69,
    I32Add = 0x6A,
    I32Sub = 0x6B,
    I32Mul = 0x6C,
    I32DivS = 0x6D,
    I32DivU = 0x6E,
    I32RemS = 0x6F,
    I32RemU = 0x70,
    I32And = 0x71,
    I32Or = 0x72,
    I32Xor = 0x73,
    I32Shl = 0x74,
    I32ShrS = 0x75,
    I32ShrU = 0x76,
    I32Rotl = 0x77,
    I32Rotr = 0x78,
    
    // i64 arithmetic (0x79-0x8A)
    I64Clz = 0x79,
    I64Ctz = 0x7A,
    I64Popcnt = 0x7B,
    I64Add = 0x7C,
    I64Sub = 0x7D,
    I64Mul = 0x7E,
    I64DivS = 0x7F,
    I64DivU = 0x80,
    I64RemS = 0x81,
    I64RemU = 0x82,
    I64And = 0x83,
    I64Or = 0x84,
    I64Xor = 0x85,
    I64Shl = 0x86,
    I64ShrS = 0x87,
    I64ShrU = 0x88,
    I64Rotl = 0x89,
    I64Rotr = 0x8A,
    
    // f32 arithmetic (0x8B-0x98)
    F32Abs = 0x8B,
    F32Neg = 0x8C,
    F32Ceil = 0x8D,
    F32Floor = 0x8E,
    F32Trunc = 0x8F,
    F32Nearest = 0x90,
    F32Sqrt = 0x91,
    F32Add = 0x92,
    F32Sub = 0x93,
    F32Mul = 0x94,
    F32Div = 0x95,
    F32Min = 0x96,
    F32Max = 0x97,
    F32CopySign = 0x98,
    
    // f64 arithmetic (0x99-0xA6)
    F64Abs = 0x99,
    F64Neg = 0x9A,
    F64Ceil = 0x9B,
    F64Floor = 0x9C,
    F64Trunc = 0x9D,
    F64Nearest = 0x9E,
    F64Sqrt = 0x9F,
    F64Add = 0xA0,
    F64Sub = 0xA1,
    F64Mul = 0xA2,
    F64Div = 0xA3,
    F64Min = 0xA4,
    F64Max = 0xA5,
    F64CopySign = 0xA6,
    
    // Conversions (0xA7-0xBF)
    I32WrapI64 = 0xA7,
    I32TruncF32S = 0xA8,
    I32TruncF32U = 0xA9,
    I32TruncF64S = 0xAA,
    I32TruncF64U = 0xAB,
    I64ExtendI32S = 0xAC,
    I64ExtendI32U = 0xAD,
    I64TruncF32S = 0xAE,
    I64TruncF32U = 0xAF,
    I64TruncF64S = 0xB0,
    I64TruncF64U = 0xB1,
    F32ConvertI32S = 0xB2,
    F32ConvertI32U = 0xB3,
    F32ConvertI64S = 0xB4,
    F32ConvertI64U = 0xB5,
    F32DemoteF64 = 0xB6,
    F64ConvertI32S = 0xB7,
    F64ConvertI32U = 0xB8,
    F64ConvertI64S = 0xB9,
    F64ConvertI64U = 0xBA,
    F64PromoteF32 = 0xBB,
    
    // Reinterpretations
    I32ReinterpretF32 = 0xBC,
    I64ReinterpretF64 = 0xBD,
    F32ReinterpretI32 = 0xBE,
    F64ReinterpretI64 = 0xBF,
    
    // Sign extension (0xC0-0xC4)
    I32Extend8S = 0xC0,
    I32Extend16S = 0xC1,
    I64Extend8S = 0xC2,
    I64Extend16S = 0xC3,
    I64Extend32S = 0xC4,
    
    // Reference types (0xD0-0xD6)
    RefNull = 0xD0,
    RefIsNull = 0xD1,
    RefFunc = 0xD2,
    RefEq = 0xD3,
    RefAsNonNull = 0xD4,
    BrOnNull = 0xD5,
    BrOnNonNull = 0xD6,
    
    // Prefix bytes for extension opcodes
    GcPrefix = 0xFB,
    MiscPrefix = 0xFC,
    SimdPrefix = 0xFD,
    ThreadPrefix = 0xFE,
    
    Limit = 0x100
};

inline bool isPrefixByte(uint8_t b) {
    return b >= 0xFB;
}

// =============================================================================
// GC Opcodes (0xFB prefix)
// =============================================================================

enum class GcOp : uint8_t {
    // Struct operations
    StructNew = 0x00,
    StructNewDefault = 0x01,
    StructGet = 0x02,
    StructGetS = 0x03,
    StructGetU = 0x04,
    StructSet = 0x05,
    
    // Array operations
    ArrayNew = 0x06,
    ArrayNewDefault = 0x07,
    ArrayNewFixed = 0x08,
    ArrayNewData = 0x09,
    ArrayNewElem = 0x0A,
    ArrayGet = 0x0B,
    ArrayGetS = 0x0C,
    ArrayGetU = 0x0D,
    ArraySet = 0x0E,
    ArrayLen = 0x0F,
    ArrayFill = 0x10,
    ArrayCopy = 0x11,
    ArrayInitData = 0x12,
    ArrayInitElem = 0x13,
    
    // Ref operations
    RefTest = 0x14,
    RefTestNull = 0x15,
    RefCast = 0x16,
    RefCastNull = 0x17,
    BrOnCast = 0x18,
    BrOnCastFail = 0x19,
    
    // Extern/any conversion
    AnyConvertExtern = 0x1A,
    ExternConvertAny = 0x1B,
    
    // I31 operations
    RefI31 = 0x1C,
    I31GetS = 0x1D,
    I31GetU = 0x1E,
    
    Limit
};

// =============================================================================
// Misc Opcodes (0xFC prefix)
// =============================================================================

enum class MiscOp : uint16_t {
    // Saturating truncation
    I32TruncSatF32S = 0x00,
    I32TruncSatF32U = 0x01,
    I32TruncSatF64S = 0x02,
    I32TruncSatF64U = 0x03,
    I64TruncSatF32S = 0x04,
    I64TruncSatF32U = 0x05,
    I64TruncSatF64S = 0x06,
    I64TruncSatF64U = 0x07,
    
    // Memory operations
    MemoryInit = 0x08,
    DataDrop = 0x09,
    MemoryCopy = 0x0A,
    MemoryFill = 0x0B,
    
    // Table operations
    TableInit = 0x0C,
    ElemDrop = 0x0D,
    TableCopy = 0x0E,
    TableGrow = 0x0F,
    TableSize = 0x10,
    TableFill = 0x11,
    
    Limit
};

// =============================================================================
// Thread Opcodes (0xFE prefix) - Atomics
// =============================================================================

enum class ThreadOp : uint16_t {
    // Atomic wait/notify
    MemoryAtomicNotify = 0x00,
    MemoryAtomicWait32 = 0x01,
    MemoryAtomicWait64 = 0x02,
    AtomicFence = 0x03,
    
    // Atomic loads
    I32AtomicLoad = 0x10,
    I64AtomicLoad = 0x11,
    I32AtomicLoad8U = 0x12,
    I32AtomicLoad16U = 0x13,
    I64AtomicLoad8U = 0x14,
    I64AtomicLoad16U = 0x15,
    I64AtomicLoad32U = 0x16,
    
    // Atomic stores
    I32AtomicStore = 0x17,
    I64AtomicStore = 0x18,
    I32AtomicStore8 = 0x19,
    I32AtomicStore16 = 0x1A,
    I64AtomicStore8 = 0x1B,
    I64AtomicStore16 = 0x1C,
    I64AtomicStore32 = 0x1D,
    
    // Atomic RMW add
    I32AtomicRmwAdd = 0x1E,
    I64AtomicRmwAdd = 0x1F,
    I32AtomicRmw8AddU = 0x20,
    I32AtomicRmw16AddU = 0x21,
    I64AtomicRmw8AddU = 0x22,
    I64AtomicRmw16AddU = 0x23,
    I64AtomicRmw32AddU = 0x24,
    
    // Atomic RMW sub
    I32AtomicRmwSub = 0x25,
    I64AtomicRmwSub = 0x26,
    I32AtomicRmw8SubU = 0x27,
    I32AtomicRmw16SubU = 0x28,
    I64AtomicRmw8SubU = 0x29,
    I64AtomicRmw16SubU = 0x2A,
    I64AtomicRmw32SubU = 0x2B,
    
    // Atomic RMW and
    I32AtomicRmwAnd = 0x2C,
    I64AtomicRmwAnd = 0x2D,
    I32AtomicRmw8AndU = 0x2E,
    I32AtomicRmw16AndU = 0x2F,
    I64AtomicRmw8AndU = 0x30,
    I64AtomicRmw16AndU = 0x31,
    I64AtomicRmw32AndU = 0x32,
    
    // Atomic RMW or
    I32AtomicRmwOr = 0x33,
    I64AtomicRmwOr = 0x34,
    I32AtomicRmw8OrU = 0x35,
    I32AtomicRmw16OrU = 0x36,
    I64AtomicRmw8OrU = 0x37,
    I64AtomicRmw16OrU = 0x38,
    I64AtomicRmw32OrU = 0x39,
    
    // Atomic RMW xor
    I32AtomicRmwXor = 0x3A,
    I64AtomicRmwXor = 0x3B,
    I32AtomicRmw8XorU = 0x3C,
    I32AtomicRmw16XorU = 0x3D,
    I64AtomicRmw8XorU = 0x3E,
    I64AtomicRmw16XorU = 0x3F,
    I64AtomicRmw32XorU = 0x40,
    
    // Atomic RMW exchange
    I32AtomicRmwXchg = 0x41,
    I64AtomicRmwXchg = 0x42,
    I32AtomicRmw8XchgU = 0x43,
    I32AtomicRmw16XchgU = 0x44,
    I64AtomicRmw8XchgU = 0x45,
    I64AtomicRmw16XchgU = 0x46,
    I64AtomicRmw32XchgU = 0x47,
    
    // Atomic compare-exchange
    I32AtomicRmwCmpxchg = 0x48,
    I64AtomicRmwCmpxchg = 0x49,
    I32AtomicRmw8CmpxchgU = 0x4A,
    I32AtomicRmw16CmpxchgU = 0x4B,
    I64AtomicRmw8CmpxchgU = 0x4C,
    I64AtomicRmw16CmpxchgU = 0x4D,
    I64AtomicRmw32CmpxchgU = 0x4E,
    
    Limit
};

// =============================================================================
// SIMD Opcodes (0xFD prefix) - subset of most common
// =============================================================================

enum class SimdOp : uint16_t {
    // Memory
    V128Load = 0x00,
    V128Load8x8S = 0x01,
    V128Load8x8U = 0x02,
    V128Load16x4S = 0x03,
    V128Load16x4U = 0x04,
    V128Load32x2S = 0x05,
    V128Load32x2U = 0x06,
    V128Load8Splat = 0x07,
    V128Load16Splat = 0x08,
    V128Load32Splat = 0x09,
    V128Load64Splat = 0x0A,
    V128Store = 0x0B,
    
    // Constant
    V128Const = 0x0C,
    
    // Shuffle
    I8x16Shuffle = 0x0D,
    I8x16Swizzle = 0x0E,
    
    // Splat
    I8x16Splat = 0x0F,
    I16x8Splat = 0x10,
    I32x4Splat = 0x11,
    I64x2Splat = 0x12,
    F32x4Splat = 0x13,
    F64x2Splat = 0x14,
    
    // Lane operations
    I8x16ExtractLaneS = 0x15,
    I8x16ExtractLaneU = 0x16,
    I8x16ReplaceLane = 0x17,
    I16x8ExtractLaneS = 0x18,
    I16x8ExtractLaneU = 0x19,
    I16x8ReplaceLane = 0x1A,
    I32x4ExtractLane = 0x1B,
    I32x4ReplaceLane = 0x1C,
    I64x2ExtractLane = 0x1D,
    I64x2ReplaceLane = 0x1E,
    F32x4ExtractLane = 0x1F,
    F32x4ReplaceLane = 0x20,
    F64x2ExtractLane = 0x21,
    F64x2ReplaceLane = 0x22,
    
    // Comparisons (subset)
    I8x16Eq = 0x23,
    I8x16Ne = 0x24,
    I16x8Eq = 0x2D,
    I16x8Ne = 0x2E,
    I32x4Eq = 0x37,
    I32x4Ne = 0x38,
    
    // Bitwise
    V128Not = 0x4D,
    V128And = 0x4E,
    V128AndNot = 0x4F,
    V128Or = 0x50,
    V128Xor = 0x51,
    V128Bitselect = 0x52,
    V128AnyTrue = 0x53,
    
    // i8x16 arithmetic
    I8x16Abs = 0x60,
    I8x16Neg = 0x61,
    I8x16Add = 0x6E,
    I8x16Sub = 0x71,
    
    // i16x8 arithmetic
    I16x8Abs = 0x80,
    I16x8Neg = 0x81,
    I16x8Add = 0x8E,
    I16x8Sub = 0x91,
    I16x8Mul = 0x95,
    
    // i32x4 arithmetic
    I32x4Abs = 0xA0,
    I32x4Neg = 0xA1,
    I32x4Add = 0xAE,
    I32x4Sub = 0xB1,
    I32x4Mul = 0xB5,
    
    // i64x2 arithmetic
    I64x2Abs = 0xC0,
    I64x2Neg = 0xC1,
    I64x2Add = 0xCE,
    I64x2Sub = 0xD1,
    I64x2Mul = 0xD5,
    
    // f32x4 arithmetic
    F32x4Abs = 0xE0,
    F32x4Neg = 0xE1,
    F32x4Sqrt = 0xE3,
    F32x4Add = 0xE4,
    F32x4Sub = 0xE5,
    F32x4Mul = 0xE6,
    F32x4Div = 0xE7,
    
    // f64x2 arithmetic
    F64x2Abs = 0xEC,
    F64x2Neg = 0xED,
    F64x2Sqrt = 0xEF,
    F64x2Add = 0xF0,
    F64x2Sub = 0xF1,
    F64x2Mul = 0xF2,
    F64x2Div = 0xF3,
    
    Limit = 0x200
};

// =============================================================================
// Opcode Classification Utilities
// =============================================================================

inline bool isControlOp(Op op) {
    uint16_t v = static_cast<uint16_t>(op);
    return v <= 0x1F || op == Op::End;
}

inline bool isMemoryOp(Op op) {
    uint16_t v = static_cast<uint16_t>(op);
    return v >= 0x28 && v <= 0x40;
}

inline bool isNumericOp(Op op) {
    uint16_t v = static_cast<uint16_t>(op);
    return v >= 0x45 && v <= 0xC4;
}

inline bool isReferenceOp(Op op) {
    uint16_t v = static_cast<uint16_t>(op);
    return v >= 0xD0 && v <= 0xD6;
}

// =============================================================================
// Compilation Tiers
// =============================================================================

enum class CompilationTier : uint8_t {
    None,           // Not compiled
    Interpreter,    // Bytecode interpreter
    Baseline,       // Fast single-pass JIT
    Optimized       // Optimizing JIT (Ion equivalent)
};

enum class CompilationStatus : uint8_t {
    NotCompiled,
    InProgress,
    Compiled,
    Failed
};

} // namespace Zepra::Wasm
