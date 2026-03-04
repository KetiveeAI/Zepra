/**
 * @file DFGGraph.h
 * @brief DFG Graph - Complete SSA graph representation
 * 
 * The Graph owns all basic blocks and values, and provides
 * the main interface for building and manipulating the DFG IR.
 */

#pragma once

#include "DFGBasicBlock.h"
#include "DFGValue.h"
#include "DFGOpcode.h"
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <unordered_map>

namespace Zepra::DFG {

// =============================================================================
// Graph - Container for the complete DFG IR
// =============================================================================

class Graph {
public:
    Graph() : nextValueIndex_(0), entryBlock_(nullptr) {}
    ~Graph() = default;
    
    // No copy
    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;
    
    // Move OK
    Graph(Graph&&) = default;
    Graph& operator=(Graph&&) = default;
    
    // =========================================================================
    // Basic Block Management
    // =========================================================================
    
    BasicBlock* addBlock() {
        uint32_t index = static_cast<uint32_t>(blocks_.size());
        blocks_.push_back(std::make_unique<BasicBlock>(index));
        return blocks_.back().get();
    }
    
    BasicBlock* block(uint32_t index) const {
        if (index >= blocks_.size()) return nullptr;
        return blocks_[index].get();
    }
    
    uint32_t numBlocks() const { return static_cast<uint32_t>(blocks_.size()); }
    
    BasicBlock* entryBlock() const { return entryBlock_; }
    void setEntryBlock(BasicBlock* b) { entryBlock_ = b; }
    
    // =========================================================================
    // Value Creation
    // =========================================================================
    
    Value* addValue(Opcode op, Type type) {
        values_.push_back(std::make_unique<Value>(op, type, nextValueIndex_++));
        return values_.back().get();
    }
    
    Value* addValue(Opcode op, Type type, Value* input0) {
        Value* v = addValue(op, type);
        v->addInput(input0);
        return v;
    }
    
    Value* addValue(Opcode op, Type type, Value* input0, Value* input1) {
        Value* v = addValue(op, type);
        v->addInput(input0);
        v->addInput(input1);
        return v;
    }
    
    Value* addValue(Opcode op, Type type, Value* input0, Value* input1, Value* input2) {
        Value* v = addValue(op, type);
        v->addInput(input0);
        v->addInput(input1);
        v->addInput(input2);
        return v;
    }
    
    Value* addValue(Opcode op, Type type, const std::vector<Value*>& inputs) {
        Value* v = addValue(op, type);
        for (Value* input : inputs) {
            v->addInput(input);
        }
        return v;
    }
    
    uint32_t numValues() const { return static_cast<uint32_t>(values_.size()); }
    
    // =========================================================================
    // Constant Creation
    // =========================================================================
    
    Value* constInt32(int32_t val) {
        Value* v = addValue(Opcode::Const32, Type::I32);
        v->setConstant(ConstantValue(val));
        return v;
    }
    
    Value* constInt64(int64_t val) {
        Value* v = addValue(Opcode::Const64, Type::I64);
        v->setConstant(ConstantValue(val));
        return v;
    }
    
    Value* constFloat32(float val) {
        Value* v = addValue(Opcode::ConstF32, Type::F32);
        v->setConstant(ConstantValue(val));
        return v;
    }
    
    Value* constFloat64(double val) {
        Value* v = addValue(Opcode::ConstF64, Type::F64);
        v->setConstant(ConstantValue(val));
        return v;
    }
    
    // =========================================================================
    // Phi Node Creation
    // =========================================================================
    
    Value* addPhi(Type type, BasicBlock* block) {
        Value* phi = addValue(Opcode::Phi, type);
        block->addPhi(phi);
        return phi;
    }
    
    // =========================================================================
    // Control Flow Creation
    // =========================================================================
    
    Value* addJump(BasicBlock* from, BasicBlock* target) {
        Value* jump = addValue(Opcode::Jump, Type::Void);
        from->appendValue(jump);
        from->addSuccessor(target);
        return jump;
    }
    
    Value* addBranch(BasicBlock* from, Value* cond, 
                     BasicBlock* taken, BasicBlock* notTaken) {
        Value* branch = addValue(Opcode::Branch, Type::Void, cond);
        from->appendValue(branch);
        from->addSuccessor(taken);
        from->addSuccessor(notTaken);
        return branch;
    }
    
    Value* addReturn(BasicBlock* from, Value* result = nullptr) {
        Value* ret = addValue(Opcode::Return, Type::Void);
        if (result) ret->addInput(result);
        from->appendValue(ret);
        return ret;
    }
    
    // =========================================================================
    // Graph Traversal
    // =========================================================================
    
    // Reverse post-order traversal for dataflow analysis
    std::vector<BasicBlock*> reversePostOrder() const {
        std::vector<BasicBlock*> order;
        std::vector<bool> visited(blocks_.size(), false);
        
        if (entryBlock_) {
            postOrderVisit(entryBlock_, visited, order);
        }
        
        std::reverse(order.begin(), order.end());
        return order;
    }
    
    // =========================================================================
    // Dead Code Removal
    // =========================================================================
    
    void removeDeadValues() {
        // Mark dead values
        for (auto& v : values_) {
            if (!v->isDead() && v->hasNoUsers() && !hasSideEffects(v->opcode())) {
                v->markDead();
            }
        }
        
        // Remove from blocks
        for (auto& block : blocks_) {
            auto& vals = block->values();
            vals.erase(std::remove_if(vals.begin(), vals.end(),
                [](Value* v) { return v->isDead(); }), vals.end());
        }
    }
    
    // =========================================================================
    // Validation
    // =========================================================================
    
    bool validate(std::string* error = nullptr) const {
        // Check entry block exists
        if (!entryBlock_) {
            if (error) *error = "No entry block";
            return false;
        }
        
        // Check all blocks have terminators
        for (auto& block : blocks_) {
            if (!block->terminator()) {
                if (error) *error = "Block " + std::to_string(block->index()) + " has no terminator";
                return false;
            }
        }
        
        // Check phi nodes have correct number of inputs
        for (auto& block : blocks_) {
            uint32_t numPreds = block->numPredecessors();
            for (Value* phi : block->phis()) {
                if (phi->numInputs() != numPreds) {
                    if (error) *error = "Phi has wrong number of inputs";
                    return false;
                }
            }
        }
        
        return true;
    }
    
    // =========================================================================
    // Debug Dump
    // =========================================================================
    
    void dump() const;
    
    // =========================================================================
    // Function Metadata
    // =========================================================================
    
    void setFunctionIndex(uint32_t idx) { funcIndex_ = idx; }
    uint32_t functionIndex() const { return funcIndex_; }
    
    void setNumLocals(uint32_t n) { numLocals_ = n; }
    uint32_t numLocals() const { return numLocals_; }
    
    void setNumParams(uint32_t n) { numParams_ = n; }
    uint32_t numParams() const { return numParams_; }
    
    void setResultType(Type t) { resultType_ = t; }
    Type resultType() const { return resultType_; }
    
private:
    void postOrderVisit(BasicBlock* block, std::vector<bool>& visited,
                        std::vector<BasicBlock*>& order) const {
        if (visited[block->index()]) return;
        visited[block->index()] = true;
        
        for (BasicBlock* succ : block->successors()) {
            postOrderVisit(succ, visited, order);
        }
        
        order.push_back(block);
    }
    
    std::vector<std::unique_ptr<BasicBlock>> blocks_;
    std::vector<std::unique_ptr<Value>> values_;
    uint32_t nextValueIndex_;
    BasicBlock* entryBlock_;
    
    // Function metadata
    uint32_t funcIndex_ = 0;
    uint32_t numLocals_ = 0;
    uint32_t numParams_ = 0;
    Type resultType_ = Type::Void;
};

} // namespace Zepra::DFG
