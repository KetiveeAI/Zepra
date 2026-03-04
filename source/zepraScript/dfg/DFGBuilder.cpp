/**
 * @file DFGBuilder.cpp
 * @brief DFG Graph Builder - WASM to SSA construction
 * 
 * Translates WASM bytecode into SSA-form DFG IR using
 * the Cytron algorithm for phi node insertion.
 */

#include "dfg/DFGBuilder.h"
#include "dfg/DFGGraph.h"
#include <cstdio>
#include <cstring>  // For std::memcpy

namespace Zepra::DFG {

// =============================================================================
// Main Entry Point
// =============================================================================

bool Builder::build(uint32_t funcIndex, Graph* graph) {
    graph_ = graph;
    
    if (!initializeFunction(funcIndex)) {
        return false;
    }
    
    // Create entry block
    BasicBlock* entry = graph_->addBlock();
    graph_->setEntryBlock(entry);
    currentBlock_ = entry;
    
    // Create parameters and locals
    createParameters();
    createLocals();
    
    // Get function bytecode range
    if (funcIndex >= module_->numFunctions()) {
        setError("Function index out of bounds");
        return false;
    }
    
    const auto& funcDecl = module_->functions()[funcIndex];
    const uint8_t* code = module_->bytecode() + funcDecl.codeOffset;
    const uint8_t* codeEnd = code + funcDecl.codeSize;
    
    // Process bytecode
    while (code < codeEnd) {
        uint8_t opByte = *code++;
        Wasm::Op op = static_cast<Wasm::Op>(opByte);
        
        if (!processOpcode(op, code, codeEnd)) {
            return false;
        }
    }
    
    return true;
}

// =============================================================================
// Initialization
// =============================================================================

bool Builder::initializeFunction(uint32_t funcIndex) {
    if (!module_) {
        setError("No module set");
        return false;
    }
    
    if (funcIndex >= module_->numFunctions()) {
        setError("Function index out of bounds");
        return false;
    }
    
    graph_->setFunctionIndex(funcIndex);
    
    // Get function type
    const auto& funcDecl = module_->functions()[funcIndex];
    const Wasm::FuncType* funcType = module_->funcType(funcDecl.typeIndex);
    
    if (!funcType) {
        setError("Invalid function type index");
        return false;
    }
    
    numParams_ = funcType->params().size();
    
    // Build local types: params first, then locals
    localTypes_.clear();
    for (const Wasm::ValType& vt : funcType->params()) {
        localTypes_.push_back(wasmToType(vt));
    }
    
    // Note: locals would be parsed from the code section
    // For now we'll add them as we encounter local.get/set
    
    graph_->setNumParams(numParams_);
    
    // Set result type
    if (funcType->results().empty()) {
        graph_->setResultType(Type::Void);
    } else {
        graph_->setResultType(wasmToType(funcType->results()[0]));
    }
    
    return true;
}

void Builder::createParameters() {
    for (uint32_t i = 0; i < numParams_; ++i) {
        Type paramType = localTypes_[i];
        Value* param = graph_->addValue(Opcode::Parameter, paramType);
        param->setAuxInt(i);
        currentBlock_->appendValue(param);
        
        // Initialize current definition for this param
        writeLocal(i, param);
    }
}

void Builder::createLocals() {
    // Locals beyond params are initialized to zero
    for (uint32_t i = numParams_; i < localTypes_.size(); ++i) {
        Type localType = localTypes_[i];
        Value* zero = nullptr;
        
        switch (localType) {
            case Type::I32: zero = graph_->constInt32(0); break;
            case Type::I64: zero = graph_->constInt64(0); break;
            case Type::F32: zero = graph_->constFloat32(0.0f); break;
            case Type::F64: zero = graph_->constFloat64(0.0); break;
            default: zero = graph_->constInt32(0); break;
        }
        
        currentBlock_->appendValue(zero);
        writeLocal(i, zero);
    }
}

// =============================================================================
// Bytecode Processing
// =============================================================================

bool Builder::processOpcode(Wasm::Op op, const uint8_t*& code, const uint8_t* codeEnd) {
    using namespace Wasm;
    
    switch (op) {
        // Control flow
        case Op::Unreachable:
            return processUnreachable();
        case Op::Nop:
            return true;
        case Op::Block:
            return processBlock(readBlockType(code));
        case Op::Loop:
            return processLoop(readBlockType(code));
        case Op::If:
            return processIf(readBlockType(code));
        case Op::Else:
            return processElse();
        case Op::End:
            return processEnd();
        case Op::Br:
            return processBr(readU32LEB(code));
        case Op::BrIf:
            return processBrIf(readU32LEB(code));
        case Op::Return:
            return processReturn();
            
        // Constants
        case Op::I32Const:
            processI32Const(readS32LEB(code));
            return true;
        case Op::I64Const:
            processI64Const(readS64LEB(code));
            return true;
        case Op::F32Const:
            processF32Const(readF32(code));
            return true;
        case Op::F64Const:
            processF64Const(readF64(code));
            return true;
            
        // Locals
        case Op::LocalGet:
            processLocalGet(readU32LEB(code));
            return true;
        case Op::LocalSet:
            processLocalSet(readU32LEB(code));
            return true;
        case Op::LocalTee:
            processLocalTee(readU32LEB(code));
            return true;
            
        // I32 arithmetic
        case Op::I32Add:
            emitBinaryOp(Opcode::Add32, Type::I32);
            return true;
        case Op::I32Sub:
            emitBinaryOp(Opcode::Sub32, Type::I32);
            return true;
        case Op::I32Mul:
            emitBinaryOp(Opcode::Mul32, Type::I32);
            return true;
        case Op::I32DivS:
            emitBinaryOp(Opcode::Div32S, Type::I32);
            return true;
        case Op::I32DivU:
            emitBinaryOp(Opcode::Div32U, Type::I32);
            return true;
        case Op::I32RemS:
            emitBinaryOp(Opcode::Rem32S, Type::I32);
            return true;
        case Op::I32RemU:
            emitBinaryOp(Opcode::Rem32U, Type::I32);
            return true;
        case Op::I32And:
            emitBinaryOp(Opcode::And32, Type::I32);
            return true;
        case Op::I32Or:
            emitBinaryOp(Opcode::Or32, Type::I32);
            return true;
        case Op::I32Xor:
            emitBinaryOp(Opcode::Xor32, Type::I32);
            return true;
        case Op::I32Shl:
            emitBinaryOp(Opcode::Shl32, Type::I32);
            return true;
        case Op::I32ShrS:
            emitBinaryOp(Opcode::Shr32S, Type::I32);
            return true;
        case Op::I32ShrU:
            emitBinaryOp(Opcode::Shr32U, Type::I32);
            return true;
        case Op::I32Rotl:
            emitBinaryOp(Opcode::Rotl32, Type::I32);
            return true;
        case Op::I32Rotr:
            emitBinaryOp(Opcode::Rotr32, Type::I32);
            return true;
            
        // I32 comparisons
        case Op::I32Eqz:
            emitUnaryOp(Opcode::Eqz32, Type::I32);
            return true;
        case Op::I32Eq:
            emitCompareOp(Opcode::Eq32);
            return true;
        case Op::I32Ne:
            emitCompareOp(Opcode::Ne32);
            return true;
        case Op::I32LtS:
            emitCompareOp(Opcode::Lt32S);
            return true;
        case Op::I32LtU:
            emitCompareOp(Opcode::Lt32U);
            return true;
        case Op::I32GtS:
            emitCompareOp(Opcode::Gt32S);
            return true;
        case Op::I32GtU:
            emitCompareOp(Opcode::Gt32U);
            return true;
        case Op::I32LeS:
            emitCompareOp(Opcode::Le32S);
            return true;
        case Op::I32LeU:
            emitCompareOp(Opcode::Le32U);
            return true;
        case Op::I32GeS:
            emitCompareOp(Opcode::Ge32S);
            return true;
        case Op::I32GeU:
            emitCompareOp(Opcode::Ge32U);
            return true;
            
        // I64 arithmetic
        case Op::I64Add:
            emitBinaryOp(Opcode::Add64, Type::I64);
            return true;
        case Op::I64Sub:
            emitBinaryOp(Opcode::Sub64, Type::I64);
            return true;
        case Op::I64Mul:
            emitBinaryOp(Opcode::Mul64, Type::I64);
            return true;
        case Op::I64DivS:
            emitBinaryOp(Opcode::Div64S, Type::I64);
            return true;
        case Op::I64DivU:
            emitBinaryOp(Opcode::Div64U, Type::I64);
            return true;
        case Op::I64And:
            emitBinaryOp(Opcode::And64, Type::I64);
            return true;
        case Op::I64Or:
            emitBinaryOp(Opcode::Or64, Type::I64);
            return true;
        case Op::I64Xor:
            emitBinaryOp(Opcode::Xor64, Type::I64);
            return true;
        case Op::I64Shl:
            emitBinaryOp(Opcode::Shl64, Type::I64);
            return true;
        case Op::I64ShrS:
            emitBinaryOp(Opcode::Shr64S, Type::I64);
            return true;
        case Op::I64ShrU:
            emitBinaryOp(Opcode::Shr64U, Type::I64);
            return true;
            
        // I64 comparisons
        case Op::I64Eqz:
            emitUnaryOp(Opcode::Eqz64, Type::I32);
            return true;
        case Op::I64Eq:
            emitCompareOp(Opcode::Eq64);
            return true;
        case Op::I64Ne:
            emitCompareOp(Opcode::Ne64);
            return true;
        case Op::I64LtS:
            emitCompareOp(Opcode::Lt64S);
            return true;
        case Op::I64LtU:
            emitCompareOp(Opcode::Lt64U);
            return true;
            
        // F32 arithmetic
        case Op::F32Add:
            emitBinaryOp(Opcode::AddF32, Type::F32);
            return true;
        case Op::F32Sub:
            emitBinaryOp(Opcode::SubF32, Type::F32);
            return true;
        case Op::F32Mul:
            emitBinaryOp(Opcode::MulF32, Type::F32);
            return true;
        case Op::F32Div:
            emitBinaryOp(Opcode::DivF32, Type::F32);
            return true;
        case Op::F32Sqrt:
            emitUnaryOp(Opcode::SqrtF32, Type::F32);
            return true;
        case Op::F32Abs:
            emitUnaryOp(Opcode::AbsF32, Type::F32);
            return true;
        case Op::F32Neg:
            emitUnaryOp(Opcode::NegF32, Type::F32);
            return true;
            
        // F64 arithmetic
        case Op::F64Add:
            emitBinaryOp(Opcode::AddF64, Type::F64);
            return true;
        case Op::F64Sub:
            emitBinaryOp(Opcode::SubF64, Type::F64);
            return true;
        case Op::F64Mul:
            emitBinaryOp(Opcode::MulF64, Type::F64);
            return true;
        case Op::F64Div:
            emitBinaryOp(Opcode::DivF64, Type::F64);
            return true;
        case Op::F64Sqrt:
            emitUnaryOp(Opcode::SqrtF64, Type::F64);
            return true;
        case Op::F64Abs:
            emitUnaryOp(Opcode::AbsF64, Type::F64);
            return true;
        case Op::F64Neg:
            emitUnaryOp(Opcode::NegF64, Type::F64);
            return true;
            
        // Conversions
        case Op::I32WrapI64:
            emitUnaryOp(Opcode::Wrap64To32, Type::I32);
            return true;
        case Op::I64ExtendI32S:
            emitUnaryOp(Opcode::Extend32STo64, Type::I64);
            return true;
        case Op::I64ExtendI32U:
            emitUnaryOp(Opcode::Extend32UTo64, Type::I64);
            return true;
            
        // Stack operations
        case Op::Drop:
            processDrop();
            return true;
        case Op::SelectNumeric:
        case Op::SelectTyped:
            processSelect();
            return true;
            
        // Call
        case Op::Call:
            processCall(readU32LEB(code));
            return true;
            
        default:
            // Handle memory operations and other opcodes
            break;
    }
    
    // Unhandled opcode - skip for now
    return true;
}

// =============================================================================
// Control Flow
// =============================================================================

bool Builder::processBlock(Wasm::BlockType bt) {
    Type resultType = blockTypeToType(bt);
    
    BasicBlock* mergeBlock = graph_->addBlock();
    pushControl(ControlFrame::Kind::Block, mergeBlock, resultType);
    
    return true;
}

bool Builder::processLoop(Wasm::BlockType bt) {
    Type resultType = blockTypeToType(bt);
    
    // Loop header block
    BasicBlock* loopHeader = graph_->addBlock();
    loopHeader->setLoopHeader(true);
    
    // Jump from current block to loop header
    graph_->addJump(currentBlock_, loopHeader);
    sealBlock(currentBlock_);
    
    currentBlock_ = loopHeader;
    pushControl(ControlFrame::Kind::Loop, loopHeader, resultType);
    
    return true;
}

bool Builder::processIf(Wasm::BlockType bt) {
    Type resultType = blockTypeToType(bt);
    Value* cond = pop();
    
    if (!cond) {
        setError("Empty stack for if condition");
        return false;
    }
    
    BasicBlock* thenBlock = graph_->addBlock();
    BasicBlock* elseBlock = graph_->addBlock();
    BasicBlock* mergeBlock = graph_->addBlock();
    
    graph_->addBranch(currentBlock_, cond, thenBlock, elseBlock);
    sealBlock(currentBlock_);
    
    ControlFrame frame(ControlFrame::Kind::If, mergeBlock, 
                       static_cast<uint32_t>(valueStack_.size()), resultType);
    frame.elseBlock = elseBlock;
    controlStack_.push_back(frame);
    
    currentBlock_ = thenBlock;
    return true;
}

bool Builder::processElse() {
    if (controlStack_.empty()) {
        setError("else without matching if");
        return false;
    }
    
    ControlFrame& frame = controlStack_.back();
    if (frame.kind != ControlFrame::Kind::If) {
        setError("else in non-if block");
        return false;
    }
    
    // Handle result value from then branch
    Value* thenResult = nullptr;
    if (frame.resultType != Type::Void && !valueStack_.empty()) {
        thenResult = pop();
    }
    
    // Jump to merge block
    if (thenResult) {
        // Pass result through phi
        graph_->addJump(currentBlock_, frame.mergeBlock);
    } else {
        graph_->addJump(currentBlock_, frame.mergeBlock);
    }
    sealBlock(currentBlock_);
    
    // Switch to else block
    frame.kind = ControlFrame::Kind::Else;
    currentBlock_ = frame.elseBlock;
    
    return true;
}

bool Builder::processEnd() {
    if (controlStack_.empty()) {
        // End of function
        Value* result = nullptr;
        if (graph_->resultType() != Type::Void && !valueStack_.empty()) {
            result = pop();
        }
        graph_->addReturn(currentBlock_, result);
        return true;
    }
    
    ControlFrame frame = controlStack_.back();
    controlStack_.pop_back();
    
    // Handle result value
    Value* result = nullptr;
    if (frame.resultType != Type::Void && !valueStack_.empty()) {
        result = pop();
    }
    
    // Jump to merge block (unless loop)
    if (frame.kind != ControlFrame::Kind::Loop) {
        graph_->addJump(currentBlock_, frame.mergeBlock);
        sealBlock(currentBlock_);
        currentBlock_ = frame.mergeBlock;
    } else {
        // Loop: seal the header
        sealBlock(frame.mergeBlock);
        // Continue after loop
        BasicBlock* afterLoop = graph_->addBlock();
        graph_->addJump(currentBlock_, afterLoop);
        sealBlock(currentBlock_);
        currentBlock_ = afterLoop;
    }
    
    // If: handle else fallthrough
    if (frame.kind == ControlFrame::Kind::If && frame.elseBlock) {
        graph_->addJump(frame.elseBlock, frame.mergeBlock);
        sealBlock(frame.elseBlock);
    }
    
    // Push result if any
    if (result) {
        push(result);
    }
    
    return true;
}

bool Builder::processBr(uint32_t depth) {
    if (depth >= controlStack_.size()) {
        setError("br depth out of bounds");
        return false;
    }
    
    ControlFrame& target = controlStack_[controlStack_.size() - 1 - depth];
    
    // For loop, branch to header; for block, branch to merge
    BasicBlock* targetBlock = target.mergeBlock;
    
    graph_->addJump(currentBlock_, targetBlock);
    
    // Start new unreachable block
    currentBlock_ = graph_->addBlock();
    
    return true;
}

bool Builder::processBrIf(uint32_t depth) {
    if (depth >= controlStack_.size()) {
        setError("br_if depth out of bounds");
        return false;
    }
    
    Value* cond = pop();
    if (!cond) {
        setError("Empty stack for br_if");
        return false;
    }
    
    ControlFrame& target = controlStack_[controlStack_.size() - 1 - depth];
    BasicBlock* continuation = graph_->addBlock();
    
    graph_->addBranch(currentBlock_, cond, target.mergeBlock, continuation);
    sealBlock(currentBlock_);
    
    currentBlock_ = continuation;
    return true;
}

bool Builder::processReturn() {
    Value* result = nullptr;
    if (graph_->resultType() != Type::Void && !valueStack_.empty()) {
        result = pop();
    }
    
    graph_->addReturn(currentBlock_, result);
    
    // Start new unreachable block
    currentBlock_ = graph_->addBlock();
    return true;
}

bool Builder::processUnreachable() {
    Value* trap = graph_->addValue(Opcode::Unreachable, Type::Void);
    currentBlock_->appendValue(trap);
    
    // Start new unreachable block
    currentBlock_ = graph_->addBlock();
    return true;
}

// =============================================================================
// Constants
// =============================================================================

void Builder::processI32Const(int32_t val) {
    Value* c = graph_->constInt32(val);
    currentBlock_->appendValue(c);
    push(c);
}

void Builder::processI64Const(int64_t val) {
    Value* c = graph_->constInt64(val);
    currentBlock_->appendValue(c);
    push(c);
}

void Builder::processF32Const(float val) {
    Value* c = graph_->constFloat32(val);
    currentBlock_->appendValue(c);
    push(c);
}

void Builder::processF64Const(double val) {
    Value* c = graph_->constFloat64(val);
    currentBlock_->appendValue(c);
    push(c);
}

// =============================================================================
// Locals
// =============================================================================

void Builder::processLocalGet(uint32_t idx) {
    Value* val = readLocal(idx);
    push(val);
}

void Builder::processLocalSet(uint32_t idx) {
    Value* val = pop();
    if (val) {
        writeLocal(idx, val);
    }
}

void Builder::processLocalTee(uint32_t idx) {
    Value* val = peek();
    if (val) {
        writeLocal(idx, val);
    }
}

// =============================================================================
// Stack Operations
// =============================================================================

void Builder::processDrop() {
    pop();
}

void Builder::processSelect() {
    Value* cond = pop();
    Value* falseVal = pop();
    Value* trueVal = pop();
    
    if (!cond || !falseVal || !trueVal) {
        return;
    }
    
    Value* sel = graph_->addValue(Opcode::Select, trueVal->type(), trueVal, falseVal, cond);
    currentBlock_->appendValue(sel);
    push(sel);
}

void Builder::processCall(uint32_t funcIdx) {
    // Get function type
    if (funcIdx >= module_->numFunctions()) {
        return;
    }
    
    const auto& funcDecl = module_->functions()[funcIdx];
    const Wasm::FuncType* funcType = module_->funcType(funcDecl.typeIndex);
    
    if (!funcType) return;
    
    // Pop arguments in reverse order
    std::vector<Value*> args;
    for (size_t i = 0; i < funcType->params().size(); ++i) {
        Value* arg = pop();
        if (arg) args.push_back(arg);
    }
    std::reverse(args.begin(), args.end());
    
    // Create call value
    Type resultType = funcType->results().empty() ? 
                      Type::Void : wasmToType(funcType->results()[0]);
    
    Value* call = graph_->addValue(Opcode::Call, resultType, args);
    call->setAuxInt(funcIdx);
    currentBlock_->appendValue(call);
    
    if (resultType != Type::Void) {
        push(call);
    }
}

// =============================================================================
// SSA Construction
// =============================================================================

void Builder::writeLocal(uint32_t idx, Value* val) {
    // Ensure local type exists
    while (idx >= localTypes_.size()) {
        localTypes_.push_back(val->type());
    }
    
    // Update current definition
    auto& defs = currentDef_[currentBlock_];
    while (idx >= defs.size()) {
        defs.push_back(nullptr);
    }
    defs[idx] = val;
}

Value* Builder::readLocal(uint32_t idx) {
    // Check current block first
    auto it = currentDef_.find(currentBlock_);
    if (it != currentDef_.end() && idx < it->second.size()) {
        if (it->second[idx]) {
            return it->second[idx];
        }
    }
    
    // Need to look up in predecessors or create phi
    if (currentBlock_->hasSinglePredecessor()) {
        // Simple case: just look in predecessor
        BasicBlock* pred = currentBlock_->singlePredecessor();
        auto predIt = currentDef_.find(pred);
        if (predIt != currentDef_.end() && idx < predIt->second.size()) {
            return predIt->second[idx];
        }
    } else if (currentBlock_->numPredecessors() > 1) {
        // Need a phi node
        Type localType = idx < localTypes_.size() ? localTypes_[idx] : Type::I32;
        Value* phi = graph_->addPhi(localType, currentBlock_);
        
        // Add operands from predecessors
        for (BasicBlock* pred : currentBlock_->predecessors()) {
            auto predIt = currentDef_.find(pred);
            if (predIt != currentDef_.end() && idx < predIt->second.size()) {
                phi->addInput(predIt->second[idx]);
            } else {
                phi->addInput(nullptr); // Will be filled when block is sealed
            }
        }
        
        // Record in incomplete phis if block not sealed
        if (idx >= sealedBlocks_.size() || !sealedBlocks_[currentBlock_->index()]) {
            incompletePhis_[currentBlock_].push_back({idx, phi});
        }
        
        writeLocal(idx, phi);
        return phi;
    }
    
    // No definition found - return zero
    Type localType = idx < localTypes_.size() ? localTypes_[idx] : Type::I32;
    Value* zero = nullptr;
    switch (localType) {
        case Type::I32: zero = graph_->constInt32(0); break;
        case Type::I64: zero = graph_->constInt64(0); break;
        case Type::F32: zero = graph_->constFloat32(0.0f); break;
        case Type::F64: zero = graph_->constFloat64(0.0); break;
        default: zero = graph_->constInt32(0); break;
    }
    currentBlock_->appendValue(zero);
    return zero;
}

void Builder::sealBlock(BasicBlock* block) {
    while (block->index() >= sealedBlocks_.size()) {
        sealedBlocks_.push_back(false);
    }
    
    if (sealedBlocks_[block->index()]) return;
    sealedBlocks_[block->index()] = true;
    
    // Fill incomplete phis
    auto it = incompletePhis_.find(block);
    if (it != incompletePhis_.end()) {
        for (auto& [localIdx, phi] : it->second) {
            // Add operands from all predecessors
            for (uint32_t i = 0; i < block->numPredecessors(); ++i) {
                BasicBlock* pred = block->predecessors()[i];
                auto predIt = currentDef_.find(pred);
                if (predIt != currentDef_.end() && localIdx < predIt->second.size()) {
                    if (i < phi->numInputs()) {
                        phi->setInput(i, predIt->second[localIdx]);
                    }
                }
            }
            
            // Try to simplify trivial phi
            tryRemoveTrivialPhi(phi);
        }
        incompletePhis_.erase(it);
    }
}

Value* Builder::tryRemoveTrivialPhi(Value* phi) {
    if (phi->opcode() != Opcode::Phi) return phi;
    
    Value* same = nullptr;
    for (uint32_t i = 0; i < phi->numInputs(); ++i) {
        Value* op = phi->input(i);
        if (op == same || op == phi) continue;
        if (same != nullptr) return phi; // Multiple different values
        same = op;
    }
    
    if (same == nullptr) {
        same = graph_->constInt32(0); // Undefined phi
    }
    
    // Replace phi with same
    phi->replaceAllUsesWith(same);
    phi->markDead();
    
    return same;
}

// =============================================================================
// Control Stack
// =============================================================================

void Builder::pushControl(ControlFrame::Kind kind, BasicBlock* merge, Type resultType) {
    ControlFrame frame(kind, merge, static_cast<uint32_t>(valueStack_.size()), resultType);
    controlStack_.push_back(frame);
}

ControlFrame& Builder::topControl() {
    return controlStack_.back();
}

ControlFrame Builder::popControl() {
    ControlFrame frame = controlStack_.back();
    controlStack_.pop_back();
    return frame;
}

// =============================================================================
// Helpers
// =============================================================================

Type Builder::wasmToType(Wasm::ValType vt) {
    if (vt.isI32()) return Type::I32;
    if (vt.isI64()) return Type::I64;
    if (vt.isF32()) return Type::F32;
    if (vt.isF64()) return Type::F64;
    if (vt.isV128()) return Type::V128;
    if (vt.isReference()) {
        if (vt.refType().heapType().isFunc()) return Type::FuncRef;
        if (vt.refType().heapType().isExtern()) return Type::ExternRef;
    }
    return Type::I32;
}

Type Builder::blockTypeToType(Wasm::BlockType bt) {
    if (bt.isVoid()) {
        return Type::Void;
    } else if (bt.isSingle()) {
        return wasmToType(bt.singleType());
    } else {
        // Type index - get from type section
        const Wasm::FuncType* ft = module_->funcType(bt.typeIndex());
        if (ft && !ft->results().empty()) {
            return wasmToType(ft->results()[0]);
        }
        return Type::Void;
    }
}

void Builder::emitBinaryOp(Opcode op, Type type) {
    Value* rhs = pop();
    Value* lhs = pop();
    
    if (!lhs || !rhs) return;
    
    Value* result = graph_->addValue(op, type, lhs, rhs);
    currentBlock_->appendValue(result);
    push(result);
}

void Builder::emitUnaryOp(Opcode op, Type resultType) {
    Value* operand = pop();
    
    if (!operand) return;
    
    Value* result = graph_->addValue(op, resultType, operand);
    currentBlock_->appendValue(result);
    push(result);
}

void Builder::emitCompareOp(Opcode op) {
    Value* rhs = pop();
    Value* lhs = pop();
    
    if (!lhs || !rhs) return;
    
    Value* result = graph_->addValue(op, Type::I32, lhs, rhs);
    currentBlock_->appendValue(result);
    push(result);
}

// =============================================================================
// LEB128 and bytecode reading helpers
// =============================================================================

uint32_t Builder::readU32LEB(const uint8_t*& ptr) {
    uint32_t result = 0;
    uint32_t shift = 0;
    uint8_t byte;
    do {
        byte = *ptr++;
        result |= static_cast<uint32_t>(byte & 0x7F) << shift;
        shift += 7;
        if (shift > 35) break;  // Overflow protection: max 5 bytes for u32
    } while (byte & 0x80);
    return result;
}

int32_t Builder::readS32LEB(const uint8_t*& ptr) {
    int32_t result = 0;
    uint32_t shift = 0;
    uint8_t byte;
    do {
        byte = *ptr++;
        result |= static_cast<int32_t>(byte & 0x7F) << shift;
        shift += 7;
        if (shift > 35) break;  // Overflow protection: max 5 bytes for s32
    } while (byte & 0x80);
    // Sign extend
    if ((shift < 32) && (byte & 0x40)) {
        result |= -(1 << shift);  // More portable sign extension
    }
    return result;
}

int64_t Builder::readS64LEB(const uint8_t*& ptr) {
    int64_t result = 0;
    uint32_t shift = 0;
    uint8_t byte;
    do {
        byte = *ptr++;
        result |= static_cast<int64_t>(byte & 0x7F) << shift;
        shift += 7;
        if (shift > 70) break;  // Overflow protection: max 10 bytes for s64
    } while (byte & 0x80);
    // Sign extend
    if ((shift < 64) && (byte & 0x40)) {
        result |= -(1LL << shift);  // More portable sign extension
    }
    return result;
}

float Builder::readF32(const uint8_t*& ptr) {
    float result;
    std::memcpy(&result, ptr, sizeof(float));
    ptr += sizeof(float);
    return result;
}

double Builder::readF64(const uint8_t*& ptr) {
    double result;
    std::memcpy(&result, ptr, sizeof(double));
    ptr += sizeof(double);
    return result;
}

Wasm::BlockType Builder::readBlockType(const uint8_t*& ptr) {
    int8_t byte = static_cast<int8_t>(*ptr++);
    
    if (byte == 0x40) {
        // Empty block type (void)
        return Wasm::BlockType::void_();
    } else if (byte >= 0) {
        // Type index
        return Wasm::BlockType::typeIndex(static_cast<uint32_t>(byte));
    } else {
        // Single value type
        auto vt = Wasm::ValType::fromTypeCode(static_cast<Wasm::TypeCode>(byte & 0x7F));
        if (vt) {
            return Wasm::BlockType::single(*vt);
        }
        return Wasm::BlockType::void_();
    }
}

} // namespace Zepra::DFG
