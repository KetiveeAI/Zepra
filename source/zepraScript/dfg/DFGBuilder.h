/**
 * @file DFGBuilder.h
 * @brief DFG Graph Builder from WASM Bytecode
 * 
 * Constructs SSA-form DFG IR from WASM function bytecode.
 * Handles control flow, phi node insertion, and SSA construction.
 */

#pragma once

#include "DFGGraph.h"
#include "../wasm/WasmModule.h"
#include "../wasm/WasmBinary.h"
#include <stack>
#include <unordered_map>

namespace Zepra::DFG {

// =============================================================================
// Control Frame for SSA Construction
// =============================================================================

struct ControlFrame {
    enum class Kind : uint8_t {
        Block,
        Loop,
        If,
        Else
    };
    
    Kind kind;
    BasicBlock* mergeBlock;     // Block where control flow merges
    BasicBlock* elseBlock;      // For If: the else target
    uint32_t stackHeight;       // Value stack height at entry
    Type resultType;            // Block result type
    
    ControlFrame(Kind k, BasicBlock* merge, uint32_t stackH, Type result = Type::Void)
        : kind(k)
        , mergeBlock(merge)
        , elseBlock(nullptr)
        , stackHeight(stackH)
        , resultType(result) {}
};

// =============================================================================
// DFG Builder
// =============================================================================

class Builder {
public:
    explicit Builder(const Wasm::WasmModule* module)
        : module_(module)
        , graph_(nullptr)
        , currentBlock_(nullptr) {}
    
    // =========================================================================
    // Main Entry Point
    // =========================================================================
    
    bool build(uint32_t funcIndex, Graph* graph);
    
    const std::string& error() const { return error_; }
    
private:
    // =========================================================================
    // Initialization
    // =========================================================================
    
    bool initializeFunction(uint32_t funcIndex);
    void createParameters();
    void createLocals();
    
    // =========================================================================
    // Bytecode Processing
    // =========================================================================
    
    bool processOpcode(Wasm::Op op, const uint8_t*& code, const uint8_t* codeEnd);
    
    // Control flow
    bool processBlock(Wasm::BlockType bt);
    bool processLoop(Wasm::BlockType bt);
    bool processIf(Wasm::BlockType bt);
    bool processElse();
    bool processEnd();
    bool processBr(uint32_t depth);
    bool processBrIf(uint32_t depth);
    bool processBrTable(const std::vector<uint32_t>& targets, uint32_t defaultTarget);
    bool processReturn();
    bool processUnreachable();
    
    // Constants
    void processI32Const(int32_t val);
    void processI64Const(int64_t val);
    void processF32Const(float val);
    void processF64Const(double val);
    
    // Locals
    void processLocalGet(uint32_t idx);
    void processLocalSet(uint32_t idx);
    void processLocalTee(uint32_t idx);
    
    // Calls
    void processCall(uint32_t funcIdx);
    void processCallIndirect(uint32_t typeIdx, uint32_t tableIdx);
    
    // Stack operations
    void processDrop();
    void processSelect();
    
    // =========================================================================
    // Value Stack
    // =========================================================================
    
    void push(Value* v) { valueStack_.push_back(v); }
    
    Value* pop() {
        if (valueStack_.empty()) return nullptr;
        Value* v = valueStack_.back();
        valueStack_.pop_back();
        return v;
    }
    
    Value* peek(uint32_t depth = 0) {
        if (depth >= valueStack_.size()) return nullptr;
        return valueStack_[valueStack_.size() - 1 - depth];
    }
    
    // =========================================================================
    // Control Stack
    // =========================================================================
    
    void pushControl(ControlFrame::Kind kind, BasicBlock* merge, Type resultType);
    ControlFrame& topControl();
    ControlFrame popControl();
    
    // =========================================================================
    // SSA Construction
    // =========================================================================
    
    void writeLocal(uint32_t idx, Value* val);
    Value* readLocal(uint32_t idx);
    void sealBlock(BasicBlock* block);
    void addPhiOperand(Value* phi, BasicBlock* pred, Value* val);
    Value* tryRemoveTrivialPhi(Value* phi);
    
    // =========================================================================
    // Helpers
    // =========================================================================
    
    Type wasmToType(Wasm::ValType vt);
    Type blockTypeToType(Wasm::BlockType bt);
    
    void emitBinaryOp(Opcode op, Type type);
    void emitUnaryOp(Opcode op, Type type);
    void emitCompareOp(Opcode op);
    
    // Bytecode reading
    uint32_t readU32LEB(const uint8_t*& ptr);
    int32_t readS32LEB(const uint8_t*& ptr);
    int64_t readS64LEB(const uint8_t*& ptr);
    float readF32(const uint8_t*& ptr);
    double readF64(const uint8_t*& ptr);
    Wasm::BlockType readBlockType(const uint8_t*& ptr);
    
    void setError(const std::string& msg) { error_ = msg; }
    
    // =========================================================================
    // State
    // =========================================================================
    
    const Wasm::WasmModule* module_;
    Graph* graph_;
    std::string error_;
    
    BasicBlock* currentBlock_;
    std::vector<Value*> valueStack_;
    std::vector<ControlFrame> controlStack_;
    
    // SSA: current definition of each local per block
    std::unordered_map<BasicBlock*, std::vector<Value*>> currentDef_;
    
    // SSA: incomplete phis (awaiting seal)
    std::unordered_map<BasicBlock*, std::vector<std::pair<uint32_t, Value*>>> incompletePhis_;
    
    // SSA: sealed blocks
    std::vector<bool> sealedBlocks_;
    
    // Locals metadata
    std::vector<Type> localTypes_;
    uint32_t numParams_ = 0;
};

} // namespace Zepra::DFG
