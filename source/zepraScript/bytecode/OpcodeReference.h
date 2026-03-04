/**
 * @file OpcodeReference.h
 * @brief Complete bytecode opcode reference documentation
 * 
 * Defines all bytecode operations with:
 * - Opcode encoding
 * - Operand formats  
 * - Stack effects
 * - Semantics documentation
 * 
 * Based on V8/JSC bytecode formats
 */

#pragma once

#include <cstdint>
#include <string_view>

namespace Zepra::Bytecode {

// =============================================================================
// Opcode Categories
// =============================================================================

enum class OpcodeCategory : uint8_t {
    Load,           // Load values onto stack
    Store,          // Store values to locations
    Arithmetic,     // Math operations
    Bitwise,        // Bit operations
    Comparison,     // Comparisons
    Control,        // Control flow
    Call,           // Function calls
    Object,         // Object operations
    Array,          // Array operations
    Property,       // Property access
    Variable,       // Variable access
    Exception,      // Try/catch/throw
    Iterator,       // Iterator protocol
    Async,          // Async/await
    Class,          // Class operations
    Module,         // Module operations
    Debug           // Debug operations
};

// =============================================================================
// Operand Types
// =============================================================================

enum class OperandType : uint8_t {
    None,           // No operand
    Reg,            // Register index (8-bit)
    RegPair,        // Two registers
    Imm8,           // 8-bit immediate
    Imm16,          // 16-bit immediate
    Imm32,          // 32-bit immediate
    Idx8,           // 8-bit constant pool index
    Idx16,          // 16-bit constant pool index
    Idx32,          // 32-bit constant pool index
    Offset16,       // 16-bit branch offset
    Offset32,       // 32-bit branch offset
    Count8,         // 8-bit count (args, etc.)
    Slot,           // Frame slot index
    String,         // String constant index
    Native          // Native function index
};

// =============================================================================
// Stack Effect
// =============================================================================

struct StackEffect {
    int8_t pops;      // Number of values popped
    int8_t pushes;    // Number of values pushed
    
    int8_t net() const { return pushes - pops; }
};

// =============================================================================
// Opcode Definition
// =============================================================================

struct OpcodeInfo {
    uint8_t code;
    const char* name;
    OpcodeCategory category;
    OperandType operands[4];
    uint8_t operandCount;
    StackEffect stackEffect;
    uint8_t length;     // Total instruction length in bytes
    const char* documentation;
};

// =============================================================================
// Full Opcode Enumeration
// =============================================================================

enum class Opcode : uint8_t {
    // =========================================================================
    // Load operations (0x00 - 0x1F)
    // =========================================================================
    Ldc_i32         = 0x00,  // Load 32-bit integer constant
    Ldc_i64         = 0x01,  // Load 64-bit integer constant
    Ldc_f64         = 0x02,  // Load 64-bit float constant
    Ldc_str         = 0x03,  // Load string constant
    Ldc_null        = 0x04,  // Load null
    Ldc_undefined   = 0x05,  // Load undefined
    Ldc_true        = 0x06,  // Load true
    Ldc_false       = 0x07,  // Load false
    Ldc_zero        = 0x08,  // Load integer 0
    Ldc_one         = 0x09,  // Load integer 1
    Ldc_bigint      = 0x0A,  // Load BigInt constant
    Ldc_regex       = 0x0B,  // Load RegExp constant
    Ldc_template    = 0x0C,  // Load template literal
    Ldc_symbol      = 0x0D,  // Load symbol
    
    // =========================================================================
    // Variable operations (0x10 - 0x2F)
    // =========================================================================
    Ldloc           = 0x10,  // Load local variable
    Stloc           = 0x11,  // Store local variable
    Ldarg           = 0x12,  // Load argument
    Starg           = 0x13,  // Store argument
    Ldglo           = 0x14,  // Load global variable
    Stglo           = 0x15,  // Store global variable
    Ldenv           = 0x16,  // Load from closure environment
    Stenv           = 0x17,  // Store to closure environment
    Ldthis          = 0x18,  // Load 'this'
    Ldsuper         = 0x19,  // Load super reference
    Ldnewt          = 0x1A,  // Load new.target
    Ldimpmeta       = 0x1B,  // Load import.meta
    
    // =========================================================================
    // Arithmetic operations (0x30 - 0x4F)
    // =========================================================================
    Add             = 0x30,  // a + b
    Sub             = 0x31,  // a - b
    Mul             = 0x32,  // a * b
    Div             = 0x33,  // a / b
    Mod             = 0x34,  // a % b
    Pow             = 0x35,  // a ** b
    Neg             = 0x36,  // -a
    Inc             = 0x37,  // ++a
    Dec             = 0x38,  // --a
    ToNumber        = 0x39,  // Convert to number
    ToNumeric       = 0x3A,  // Convert to numeric
    
    // =========================================================================
    // Bitwise operations (0x50 - 0x5F)
    // =========================================================================
    BitAnd          = 0x50,  // a & b
    BitOr           = 0x51,  // a | b
    BitXor          = 0x52,  // a ^ b
    BitNot          = 0x53,  // ~a
    Shl             = 0x54,  // a << b
    Shr             = 0x55,  // a >> b
    Ushr            = 0x56,  // a >>> b
    
    // =========================================================================
    // Comparison operations (0x60 - 0x6F)
    // =========================================================================
    Eq              = 0x60,  // a == b
    Ne              = 0x61,  // a != b
    Seq             = 0x62,  // a === b
    Sne             = 0x63,  // a !== b
    Lt              = 0x64,  // a < b
    Le              = 0x65,  // a <= b
    Gt              = 0x66,  // a > b
    Ge              = 0x67,  // a >= b
    In              = 0x68,  // a in b
    Instanceof      = 0x69,  // a instanceof b
    Typeof          = 0x6A,  // typeof a
    
    // =========================================================================
    // Control flow (0x70 - 0x8F)
    // =========================================================================
    Jmp             = 0x70,  // Unconditional jump
    Jt              = 0x71,  // Jump if true
    Jf              = 0x72,  // Jump if false
    Jnull           = 0x73,  // Jump if null
    Jundefined      = 0x74,  // Jump if undefined
    Jnullish        = 0x75,  // Jump if null or undefined
    Switch          = 0x76,  // Switch statement
    LoopHint        = 0x77,  // Loop optimization hint
    
    // =========================================================================
    // Function calls (0x90 - 0x9F)
    // =========================================================================
    Call            = 0x90,  // Call function
    CallMethod      = 0x91,  // Call method
    CallSpread      = 0x92,  // Call with spread args
    CallSuper       = 0x93,  // Call super constructor
    TailCall        = 0x94,  // Tail call optimization
    Construct       = 0x95,  // new expression
    Apply           = 0x96,  // Function.prototype.apply
    Ret             = 0x97,  // Return from function
    
    // =========================================================================
    // Object operations (0xA0 - 0xAF)
    // =========================================================================
    CreateObject    = 0xA0,  // Create empty object
    CreateArray     = 0xA1,  // Create array
    CreateFunction  = 0xA2,  // Create function
    CreateClass     = 0xA3,  // Create class
    CreateClosure   = 0xA4,  // Create closure
    CreateIterResult = 0xA5, // Create iterator result
    
    // =========================================================================
    // Property access (0xB0 - 0xBF)
    // =========================================================================
    GetProp         = 0xB0,  // obj.prop
    SetProp         = 0xB1,  // obj.prop = val
    DelProp         = 0xB2,  // delete obj.prop
    GetElem         = 0xB3,  // obj[key]
    SetElem         = 0xB4,  // obj[key] = val
    DelElem         = 0xB5,  // delete obj[key]
    HasProp         = 0xB6,  // prop in obj
    DefineGetter    = 0xB7,  // Define getter
    DefineSetter    = 0xB8,  // Define setter
    GetPrivate      = 0xB9,  // #privateField
    SetPrivate      = 0xBA,  // #privateField = val
    
    // =========================================================================
    // Exception handling (0xC0 - 0xCF)
    // =========================================================================
    Throw           = 0xC0,  // Throw exception
    TryStart        = 0xC1,  // Try block start
    TryEnd          = 0xC2,  // Try block end
    Catch           = 0xC3,  // Catch block
    Finally         = 0xC4,  // Finally block
    Rethrow         = 0xC5,  // Rethrow exception
    
    // =========================================================================
    // Iterator/Generator (0xD0 - 0xDF)
    // =========================================================================
    GetIterator     = 0xD0,  // Get iterator
    IterNext        = 0xD1,  // Iterator.next()
    IterDone        = 0xD2,  // Check if iterator done
    IterValue       = 0xD3,  // Get iterator value
    Yield           = 0xD4,  // Generator yield
    YieldStar       = 0xD5,  // yield* expression
    Await           = 0xD6,  // Await promise
    AsyncStart      = 0xD7,  // Start async function
    AsyncEnd        = 0xD8,  // End async function
    
    // =========================================================================
    // Stack/misc (0xE0 - 0xEF)
    // =========================================================================
    Pop             = 0xE0,  // Pop top of stack
    Dup             = 0xE1,  // Duplicate top of stack
    Swap            = 0xE2,  // Swap top two values
    Rot             = 0xE3,  // Rotate top three
    Nop             = 0xE4,  // No operation
    
    // =========================================================================
    // Debug (0xF0 - 0xFF)
    // =========================================================================
    Debugger        = 0xF0,  // Debugger statement
    BreakPoint      = 0xF1,  // Breakpoint
    ProfileStart    = 0xF2,  // Start profiling
    ProfileEnd      = 0xF3,  // End profiling
    
    // =========================================================================
    // Extended opcodes (0xFE prefix)
    // =========================================================================
    Wide            = 0xFE,  // Wide prefix (16-bit operands)
    Extended        = 0xFF   // Extended opcode follows
};

// =============================================================================
// Opcode Info Table
// =============================================================================

constexpr OpcodeInfo OPCODE_INFO[] = {
    // Load operations
    {0x00, "Ldc_i32", OpcodeCategory::Load, {OperandType::Imm32}, 1, {0, 1}, 5, "Push 32-bit integer constant"},
    {0x01, "Ldc_i64", OpcodeCategory::Load, {OperandType::Idx16}, 1, {0, 1}, 3, "Push 64-bit integer from constant pool"},
    {0x02, "Ldc_f64", OpcodeCategory::Load, {OperandType::Idx16}, 1, {0, 1}, 3, "Push 64-bit float from constant pool"},
    {0x03, "Ldc_str", OpcodeCategory::Load, {OperandType::Idx16}, 1, {0, 1}, 3, "Push string from constant pool"},
    {0x04, "Ldc_null", OpcodeCategory::Load, {}, 0, {0, 1}, 1, "Push null"},
    {0x05, "Ldc_undefined", OpcodeCategory::Load, {}, 0, {0, 1}, 1, "Push undefined"},
    {0x06, "Ldc_true", OpcodeCategory::Load, {}, 0, {0, 1}, 1, "Push true"},
    {0x07, "Ldc_false", OpcodeCategory::Load, {}, 0, {0, 1}, 1, "Push false"},
    {0x08, "Ldc_zero", OpcodeCategory::Load, {}, 0, {0, 1}, 1, "Push integer 0"},
    {0x09, "Ldc_one", OpcodeCategory::Load, {}, 0, {0, 1}, 1, "Push integer 1"},
    
    // Variable operations
    {0x10, "Ldloc", OpcodeCategory::Variable, {OperandType::Slot}, 1, {0, 1}, 2, "Load local variable"},
    {0x11, "Stloc", OpcodeCategory::Variable, {OperandType::Slot}, 1, {1, 0}, 2, "Store local variable"},
    {0x12, "Ldarg", OpcodeCategory::Variable, {OperandType::Imm8}, 1, {0, 1}, 2, "Load argument"},
    {0x13, "Starg", OpcodeCategory::Variable, {OperandType::Imm8}, 1, {1, 0}, 2, "Store argument"},
    {0x14, "Ldglo", OpcodeCategory::Variable, {OperandType::Idx16}, 1, {0, 1}, 3, "Load global variable"},
    {0x15, "Stglo", OpcodeCategory::Variable, {OperandType::Idx16}, 1, {1, 0}, 3, "Store global variable"},
    
    // Arithmetic
    {0x30, "Add", OpcodeCategory::Arithmetic, {}, 0, {2, 1}, 1, "Pop a, b; push a + b"},
    {0x31, "Sub", OpcodeCategory::Arithmetic, {}, 0, {2, 1}, 1, "Pop a, b; push a - b"},
    {0x32, "Mul", OpcodeCategory::Arithmetic, {}, 0, {2, 1}, 1, "Pop a, b; push a * b"},
    {0x33, "Div", OpcodeCategory::Arithmetic, {}, 0, {2, 1}, 1, "Pop a, b; push a / b"},
    {0x34, "Mod", OpcodeCategory::Arithmetic, {}, 0, {2, 1}, 1, "Pop a, b; push a % b"},
    {0x35, "Pow", OpcodeCategory::Arithmetic, {}, 0, {2, 1}, 1, "Pop a, b; push a ** b"},
    {0x36, "Neg", OpcodeCategory::Arithmetic, {}, 0, {1, 1}, 1, "Pop a; push -a"},
    
    // Control flow
    {0x70, "Jmp", OpcodeCategory::Control, {OperandType::Offset16}, 1, {0, 0}, 3, "Unconditional jump"},
    {0x71, "Jt", OpcodeCategory::Control, {OperandType::Offset16}, 1, {1, 0}, 3, "Jump if true"},
    {0x72, "Jf", OpcodeCategory::Control, {OperandType::Offset16}, 1, {1, 0}, 3, "Jump if false"},
    
    // Calls
    {0x90, "Call", OpcodeCategory::Call, {OperandType::Count8}, 1, {-1, 1}, 2, "Call function (argc dynamic)"},
    {0x97, "Ret", OpcodeCategory::Call, {}, 0, {1, 0}, 1, "Return from function"},
};

// =============================================================================
// Opcode Lookup
// =============================================================================

inline const OpcodeInfo* getOpcodeInfo(Opcode op) {
    uint8_t code = static_cast<uint8_t>(op);
    for (const auto& info : OPCODE_INFO) {
        if (info.code == code) return &info;
    }
    return nullptr;
}

inline std::string_view getOpcodeName(Opcode op) {
    if (auto* info = getOpcodeInfo(op)) {
        return info->name;
    }
    return "Unknown";
}

inline uint8_t getOpcodeLength(Opcode op) {
    if (auto* info = getOpcodeInfo(op)) {
        return info->length;
    }
    return 1;
}

} // namespace Zepra::Bytecode
