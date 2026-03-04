/**
 * @file WasmBaselineCompile.h
 * @brief WebAssembly Baseline JIT Compiler
 * 
 * Single-pass JIT compiler that generates native code quickly.
 * Focuses on compilation speed over optimization.
 * 
 * Based on Firefox SpiderMonkey / WebKit JSC Baseline compilers
 */

#pragma once

#include "WasmConstants.h"
#include "WasmValType.h"
#include "WasmTypeDef.h"
#include "WasmBinary.h"
#include "WasmModule.h"
#include "WasmInstance.h"
#include "ZWasmTierController.h"
#include <vector>
#include <memory>
#include <cstdint>
#include <unordered_map>
#include <functional>

namespace Zepra::Wasm {

// Forward declarations
class MacroAssembler;
class CodeBuffer;

// =============================================================================
// Register Allocation Types
// =============================================================================

enum class RegClass : uint8_t {
    GPR,        // General purpose registers
    FPR,        // Floating point registers
    V128        // SIMD registers
};

struct Reg {
    RegClass regClass;
    uint8_t code;
    
    static Reg gpr(uint8_t c) { return {RegClass::GPR, c}; }
    static Reg fpr(uint8_t c) { return {RegClass::FPR, c}; }
    static Reg v128(uint8_t c) { return {RegClass::V128, c}; }
    
    bool operator==(const Reg& other) const {
        return regClass == other.regClass && code == other.code;
    }
};

// =============================================================================
// Stack Value Location
// =============================================================================

struct StackSlot {
    int32_t offset;     // Offset from frame pointer (negative for locals)
    ValType type;
    
    StackSlot() : offset(0), type(ValType::i32()) {}
    StackSlot(int32_t off, ValType t) : offset(off), type(t) {}
};

// =============================================================================
// Value Location (register or stack)
// =============================================================================

struct ValueLocation {
    enum class Kind : uint8_t {
        Register,
        Stack,
        Immediate
    };
    
    Kind kind = Kind::Stack;
    ValType type;
    
    union {
        Reg reg;
        StackSlot slot;
        int64_t immediate;
    };
    
    ValueLocation() : kind(Kind::Stack), type(ValType::i32()) { slot = StackSlot(); }
    
    static ValueLocation inReg(Reg r, ValType t) {
        ValueLocation loc;
        loc.kind = Kind::Register;
        loc.type = t;
        loc.reg = r;
        return loc;
    }
    
    static ValueLocation onStack(int32_t offset, ValType t) {
        ValueLocation loc;
        loc.kind = Kind::Stack;
        loc.type = t;
        loc.slot = StackSlot(offset, t);
        return loc;
    }
    
    static ValueLocation imm(int64_t val, ValType t) {
        ValueLocation loc;
        loc.kind = Kind::Immediate;
        loc.type = t;
        loc.immediate = val;
        return loc;
    }
    
    bool isRegister() const { return kind == Kind::Register; }
    bool isStack() const { return kind == Kind::Stack; }
    bool isImmediate() const { return kind == Kind::Immediate; }
};

// =============================================================================
// Control Flow Label
// =============================================================================

struct Label {
    uint32_t id = 0;
    size_t offset = 0;          // Offset in code buffer (0 if unbound)
    bool bound = false;
    
    std::vector<size_t> pendingJumps;  // Offsets that need patching
    
    Label() = default;
    explicit Label(uint32_t _id) : id(_id) {}
};

// =============================================================================
// Control Frame for Baseline
// =============================================================================

struct BaselineControlFrame {
    enum class Kind : uint8_t {
        Block,
        Loop,
        If,
        Else,
        Try,
        Catch
    };
    
    Kind kind;
    BlockType blockType;
    Label endLabel;
    Label elseLabel;        // For if blocks
    
    size_t stackHeight;     // Value stack height at entry
    size_t frameOffset;     // Stack frame offset at entry
    
    BaselineControlFrame(Kind k, BlockType bt, size_t sh)
        : kind(k), blockType(bt), stackHeight(sh), frameOffset(0) {}
};

// =============================================================================
// Compiled Code Result
// =============================================================================

struct CompiledFunction {
    std::vector<uint8_t> code;
    uint32_t funcIndex;
    uint32_t stackSlots;        // Number of stack slots used
    
    // Metadata for debugging/OSR
    std::vector<std::pair<size_t, uint32_t>> pcToOffset;  // Code offset -> bytecode offset
    
    // Tier-up check locations
    std::vector<size_t> tierUpCheckOffsets;
};

struct BaselineCompilationResult {
    std::vector<CompiledFunction> functions;
    std::string error;
    bool success = false;
};

// =============================================================================
// Baseline Compiler Options
// =============================================================================

struct BaselineCompilerOptions {
    bool enableTierUp = true;
    bool enableOSR = true;
    bool enableBoundsChecks = true;
    bool enableStackChecks = true;
    bool enableDebugInfo = false;
    
    // Tier-up thresholds
    uint32_t tierUpThreshold = 10000;
};

// =============================================================================
// Register Allocator (Simple Linear)
// =============================================================================

class SimpleRegAllocator {
public:
    SimpleRegAllocator();
    
    // Allocate a register for the given type
    Reg allocGPR();
    Reg allocFPR();
    Reg allocV128();
    
    // Free a register
    void freeReg(Reg reg);
    
    // Check availability
    bool hasAvailableGPR() const;
    bool hasAvailableFPR() const;
    
    // Spill all registers to stack
    void spillAll();
    
    // Get scratch registers
    Reg scratchGPR() const;
    Reg scratchFPR() const;
    
private:
    uint32_t gprMask_;      // Bitmap of available GPRs
    uint32_t fprMask_;      // Bitmap of available FPRs
    uint32_t gprScratch_;
    uint32_t fprScratch_;
};

// =============================================================================
// Baseline Compiler Class
// =============================================================================

class BaselineCompiler {
public:
    explicit BaselineCompiler(const WasmModule* module, 
                              const BaselineCompilerOptions& options = {});
    ~BaselineCompiler();
    
    // No copy
    BaselineCompiler(const BaselineCompiler&) = delete;
    BaselineCompiler& operator=(const BaselineCompiler&) = delete;
    
    // ==========================================================================
    // Compilation
    // ==========================================================================
    
    // Compile a single function
    bool compileFunction(uint32_t funcIndex, CompiledFunction* result);
    
    // Compile all functions in module
    BaselineCompilationResult compileAll();
    
    // Get error message
    const std::string& error() const { return error_; }
    
private:
    // ==========================================================================
    // Bytecode Processing
    // ==========================================================================
    
    bool compileFunctionBody(const FunctionDecl& func, CompiledFunction* result);
    
    // Opcode handlers
    bool emitOp(Opcode op);
    
    // Control flow
    bool emitBlock(BlockType bt);
    bool emitLoop(BlockType bt);
    bool emitIf(BlockType bt);
    bool emitElse();
    bool emitEnd();
    bool emitBr(uint32_t depth);
    bool emitBrIf(uint32_t depth);
    bool emitBrTable(const std::vector<uint32_t>& targets, uint32_t defaultTarget);
    bool emitReturn();
    bool emitUnreachable();
    
    // Calls
    bool emitCall(uint32_t funcIndex);
    bool emitCallIndirect(uint32_t typeIndex, uint32_t tableIndex);
    
    // Tail Calls
    bool emitReturnCall(uint32_t funcIndex);
    bool emitReturnCallIndirect(uint32_t typeIndex, uint32_t tableIndex);
    bool emitReturnCallRef(uint32_t typeIndex);
    
    // Locals
    bool emitLocalGet(uint32_t localIndex);
    bool emitLocalSet(uint32_t localIndex);
    bool emitLocalTee(uint32_t localIndex);
    
    // Globals
    bool emitGlobalGet(uint32_t globalIndex);
    bool emitGlobalSet(uint32_t globalIndex);
    
    // Memory
    bool emitLoad(Op op, uint32_t align, uint32_t offset);
    bool emitStore(Op op, uint32_t align, uint32_t offset);
    bool emitMemorySize(uint32_t memIndex);
    bool emitMemoryGrow(uint32_t memIndex);
    
    // Constants
    bool emitI32Const(int32_t value);
    bool emitI64Const(int64_t value);
    bool emitF32Const(float value);
    bool emitF64Const(double value);
    
    // Arithmetic (i32)
    bool emitI32Add();
    bool emitI32Sub();
    bool emitI32Mul();
    bool emitI32DivS();
    bool emitI32DivU();
    bool emitI32RemS();
    bool emitI32RemU();
    bool emitI32And();
    bool emitI32Or();
    bool emitI32Xor();
    bool emitI32Shl();
    bool emitI32ShrS();
    bool emitI32ShrU();
    bool emitI32Rotl();
    bool emitI32Rotr();
    bool emitI32Clz();
    bool emitI32Ctz();
    bool emitI32Popcnt();
    bool emitI32Eqz();
    
    // Comparison (i32)
    bool emitI32Eq();
    bool emitI32Ne();
    bool emitI32LtS();
    bool emitI32LtU();
    bool emitI32GtS();
    bool emitI32GtU();
    bool emitI32LeS();
    bool emitI32LeU();
    bool emitI32GeS();
    bool emitI32GeU();
    
    // I64 operations
    bool emitI64Add();
    bool emitI64Sub();
    bool emitI64Mul();
    bool emitI64DivS();
    bool emitI64DivU();
    bool emitI64RemS();
    bool emitI64RemU();
    bool emitI64And();
    bool emitI64Or();
    bool emitI64Xor();
    bool emitI64Shl();
    bool emitI64ShrS();
    bool emitI64ShrU();
    bool emitI64Rotl();
    bool emitI64Rotr();
    bool emitI64Clz();
    bool emitI64Ctz();
    bool emitI64Popcnt();
    bool emitI64Eqz();
    bool emitI64Eq();
    bool emitI64Ne();
    bool emitI64LtS();
    bool emitI64LtU();
    bool emitI64GtS();
    bool emitI64GtU();
    bool emitI64LeS();
    bool emitI64LeU();
    bool emitI64GeS();
    bool emitI64GeU();
    
    // Float operations
    bool emitF32Add();
    bool emitF32Sub();
    bool emitF32Mul();
    bool emitF32Div();
    bool emitF32Sqrt();
    bool emitF32Abs();
    bool emitF32Neg();
    bool emitF32Ceil();
    bool emitF32Floor();
    bool emitF32Trunc();
    bool emitF32Nearest();
    bool emitF32Copysign();
    bool emitF32Min();
    bool emitF32Max();
    
    // Double operations
    bool emitF64Add();
    bool emitF64Sub();
    bool emitF64Mul();
    bool emitF64Div();
    bool emitF64Sqrt();
    bool emitF64Abs();
    bool emitF64Neg();
    bool emitF64Ceil();
    bool emitF64Floor();
    bool emitF64Trunc();
    bool emitF64Nearest();
    bool emitF64Copysign();
    bool emitF64Min();
    bool emitF64Max();
    
    // Conversions
    bool emitI32WrapI64();
    bool emitI64ExtendI32S();
    bool emitI64ExtendI32U();
    bool emitI32TruncF32S();
    bool emitI32TruncF32U();
    bool emitI32TruncF64S();
    bool emitI32TruncF64U();
    bool emitI64TruncF32S();
    bool emitI64TruncF32U();
    bool emitI64TruncF64S();
    bool emitI64TruncF64U();
    bool emitF32ConvertI32S();
    bool emitF32ConvertI32U();
    bool emitF64ConvertI32S();
    bool emitF64ConvertI32U();
    bool emitF32ConvertI64S();
    bool emitF32ConvertI64U();
    bool emitF64ConvertI64S();
    bool emitF64ConvertI64U();
    bool emitF32DemoteF64();
    bool emitF64PromoteF32();
    bool emitI32ReinterpretF32();
    bool emitI64ReinterpretF64();
    bool emitF32ReinterpretI32();
    bool emitF64ReinterpretI64();
    
    // F32 Comparisons
    bool emitF32Eq();
    bool emitF32Ne();
    bool emitF32Lt();
    bool emitF32Gt();
    bool emitF32Le();
    bool emitF32Ge();
    
    // F64 Comparisons
    bool emitF64Eq();
    bool emitF64Ne();
    bool emitF64Lt();
    bool emitF64Gt();
    bool emitF64Le();
    bool emitF64Ge();
    
    // Drop/Select
    bool emitDrop();
    bool emitSelect();
    
    // SIMD Operations (v128)
    bool emitSimdOp(uint32_t simdOp);
    bool emitV128Load(uint32_t offset);
    bool emitV128Store(uint32_t offset);
    bool emitV128BinaryOp(uint32_t simdOp);
    bool emitV128UnaryOp(uint32_t simdOp);
    Reg allocVReg();
    void freeVReg(Reg reg);
    
    // Extended SIMD Operations
    bool emitV128Const(const uint8_t bytes[16]);
    bool emitI8x16Shuffle(const uint8_t lanes[16]);
    bool emitI8x16Swizzle();
    bool emitSplat(uint8_t laneSize);
    bool emitSplatF32();
    bool emitSplatF64();
    bool emitExtractLane(uint8_t laneSize, uint8_t laneIdx, bool signExtend);
    bool emitReplaceLane(uint8_t laneSize, uint8_t laneIdx);
    bool emitExtractLaneF(uint8_t laneSize, uint8_t laneIdx);
    bool emitReplaceLaneF(uint8_t laneSize, uint8_t laneIdx);
    bool emitV128Compare(uint32_t simdOp);
    bool emitV128Bitselect();
    bool emitV128AnyTrue();
    bool emitI32x4DotI16x8S();
    
    // Exception Handling Operations
    bool emitTry(BlockType bt);
    bool emitCatch(uint32_t tagIndex);
    bool emitCatchAll();
    bool emitThrow(uint32_t tagIndex);
    bool emitRethrow(uint32_t depth);
    bool emitDelegate(uint32_t depth);
    bool emitThrowRef();
    bool emitTryTable(BlockType bt, const std::vector<uint32_t>& handlers);
    
    // Exception handling helpers
    void emitExceptionUnwind(uint32_t targetDepth);
    void emitLoadExceptionPayload(uint32_t tagIndex);
    void emitStoreExceptionPayload(uint32_t tagIndex);
    
    // Atomic Operations
    bool emitAtomicOp(uint32_t atomicOp);
    bool emitAtomicLoad(uint32_t atomicOp, uint32_t offset);
    bool emitAtomicStore(uint32_t atomicOp, uint32_t offset);
    bool emitAtomicRMW(uint32_t atomicOp, uint32_t offset);
    bool emitAtomicCmpxchg(uint32_t atomicOp, uint32_t offset);
    bool emitAtomicWait(bool is64, uint32_t offset);
    bool emitAtomicNotify(uint32_t offset);
    bool emitAtomicFence();
    
    // GC Operations
    bool emitGCOp(uint32_t gcOp);
    bool emitStructNew(uint32_t typeIndex);
    bool emitStructNewDefault(uint32_t typeIndex);
    bool emitStructGet(uint32_t typeIndex, uint32_t fieldIndex, bool signExtend);
    bool emitStructSet(uint32_t typeIndex, uint32_t fieldIndex);
    bool emitArrayNew(uint32_t typeIndex);
    bool emitArrayNewDefault(uint32_t typeIndex);
    bool emitArrayNewFixed(uint32_t typeIndex, uint32_t length);
    bool emitArrayGet(uint32_t typeIndex, bool signExtend);
    bool emitArraySet(uint32_t typeIndex);
    bool emitArrayLen();
    bool emitArrayFill(uint32_t typeIndex);
    bool emitArrayCopy(uint32_t destType, uint32_t srcType);
    bool emitRefTest(uint32_t typeIndex, bool nullable);
    bool emitRefCast(uint32_t typeIndex, bool nullable);
    bool emitBrOnCast(uint32_t labelIdx, uint32_t srcType, uint32_t dstType, bool onFail);
    bool emitI31New();
    bool emitI31GetS();
    bool emitI31GetU();
    bool emitRefNull(uint8_t heapType);
    bool emitRefIsNull();
    bool emitRefAsNonNull();
    bool emitRefEq();
    
    // Bulk Memory Operations
    bool emitBulkOp(uint32_t bulkOp);
    bool emitMemoryCopy(uint32_t destMemIdx, uint32_t srcMemIdx);
    bool emitMemoryFill(uint32_t memIdx);
    bool emitMemoryInit(uint32_t dataIdx, uint32_t memIdx);
    bool emitDataDrop(uint32_t dataIdx);
    bool emitTableCopy(uint32_t destTableIdx, uint32_t srcTableIdx);
    bool emitTableInit(uint32_t elemIdx, uint32_t tableIdx);
    bool emitElemDrop(uint32_t elemIdx);
    bool emitTableGrow(uint32_t tableIdx);
    bool emitTableSize(uint32_t tableIdx);
    bool emitTableFill(uint32_t tableIdx);
    
    // Reference Types
    bool emitRefFunc(uint32_t funcIdx);
    bool emitTableGet(uint32_t tableIdx);
    bool emitTableSet(uint32_t tableIdx);
    
    // ==========================================================================
    // Code Generation Helpers
    // ==========================================================================
    
    void emitPrologue(const FuncType* funcType, uint32_t numLocals);
    void emitEpilogue();
    void emitTierUpCheck();
    void emitStackCheck(uint32_t frameSize);
    void emitTrap(Trap trap);
    
    // Value stack management
    void push(ValueLocation loc);
    ValueLocation pop();
    ValueLocation peek(uint32_t depth = 0);
    
    // Register allocation
    Reg allocReg(ValType type);
    void freeReg(Reg reg);
    ValueLocation ensureInRegister(const ValueLocation& loc);
    void spillToStack(const ValueLocation& loc);
    
    // Memory addressing
    void emitBoundsCheck(uint32_t offset);
    void emitBoundsCheck64(uint32_t offset, bool isMemory64);
    ValueLocation computeAddress(uint32_t memIndex, uint32_t offset);
    
    // Control flow
    Label newLabel();
    void bindLabel(Label& label);
    void jumpTo(Label& label);
    void branchTo(Label& label, Reg condition);
    
    // Binary operations helper
    template<typename EmitFn>
    bool emitBinaryOp(ValType type, EmitFn&& emit);
    
    template<typename EmitFn>
    bool emitUnaryOp(ValType type, EmitFn&& emit);
    
    // ==========================================================================
    // State
    // ==========================================================================
    
    const WasmModule* module_;
    BaselineCompilerOptions options_;
    std::string error_;
    
    // Current function state
    Decoder* decoder_ = nullptr;
    const FuncType* funcType_ = nullptr;
    uint32_t funcIndex_ = 0;
    
    // Value stack (locations of values)
    std::vector<ValueLocation> valueStack_;
    
    // Control stack
    std::vector<BaselineControlFrame> controlStack_;
    
    // Local variables
    std::vector<StackSlot> locals_;
    
    // Frame layout
    int32_t frameSize_ = 0;
    int32_t maxFrameSize_ = 0;
    
    // Code buffer
    std::unique_ptr<CodeBuffer> codeBuffer_;
    
    // Register allocator
    SimpleRegAllocator regAlloc_;
    
    // Label management
    uint32_t nextLabelId_ = 0;
    
    // PC mapping for debugging
    std::vector<std::pair<size_t, uint32_t>> pcMapping_;
    
    // Exception handling state
    std::vector<size_t> tryStack_;  // Indices into controlStack_ for active try blocks
    bool hasExceptionHandling_ = false;
    
    // Tier-up state (for OSR to optimized tier)
    int32_t* tierUpCounterPtr_ = nullptr;       // Pointer to tier-up counter
    void* tierUpSlowPath_ = nullptr;            // Tier-up slow path handler
    uint32_t currentLoopIndex_ = 0;             // Current loop index for OSR
    
    struct OSRPoint {
        uint32_t loopIndex;
        size_t codeOffset;
        size_t stackHeight;
    };
    std::vector<OSRPoint> tierUpOSRPoints_;     // OSR entry points for this function
};

// =============================================================================
// Quick Compilation Functions
// =============================================================================

// Compile a single function
inline CompiledFunction compileFunction(const WasmModule* module, uint32_t funcIndex,
                                        std::string* errorOut = nullptr) {
    BaselineCompiler compiler(module);
    CompiledFunction result;
    if (!compiler.compileFunction(funcIndex, &result)) {
        if (errorOut) *errorOut = compiler.error();
    }
    return result;
}

// Compile all functions
inline BaselineCompilationResult compileModule(const WasmModule* module) {
    BaselineCompiler compiler(module);
    return compiler.compileAll();
}

// =============================================================================
// Platform Support Check
// =============================================================================

bool baselinePlatformSupport();

} // namespace Zepra::Wasm
