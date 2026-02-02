/**
 * @file B3Procedure.h
 * @brief B3 Procedure (Function)
 * 
 * Container for B3 IR - equivalent to a function being compiled.
 */

#pragma once

#include "B3BasicBlock.h"
#include <vector>
#include <memory>

namespace Zepra::B3 {

class Procedure {
public:
    Procedure() = default;
    
    // Block management
    BasicBlock* addBlock() {
        uint32_t index = static_cast<uint32_t>(blocks_.size());
        blocks_.push_back(std::make_unique<BasicBlock>(index));
        return blocks_.back().get();
    }
    
    BasicBlock* block(uint32_t index) { return blocks_[index].get(); }
    const std::vector<std::unique_ptr<BasicBlock>>& blocks() const { return blocks_; }
    uint32_t numBlocks() const { return static_cast<uint32_t>(blocks_.size()); }
    
    BasicBlock* entryBlock() { return blocks_.empty() ? nullptr : blocks_[0].get(); }
    
    // Value management
    Value* addValue(Opcode op, Type type) {
        uint32_t index = static_cast<uint32_t>(values_.size());
        values_.push_back(std::make_unique<Value>(op, type, index));
        return values_.back().get();
    }
    
    template<typename... Args>
    Value* addValue(Opcode op, Type type, Args... inputs) {
        Value* v = addValue(op, type);
        (v->addInput(inputs), ...);
        for (auto* input : {inputs...}) {
            if (input) input->addUser(v);
        }
        return v;
    }
    
    const std::vector<std::unique_ptr<Value>>& values() const { return values_; }
    
    // Constants
    Value* constInt32(int32_t val) {
        Value* v = addValue(Opcode::Const32, Type::Int32);
        v->setConstInt32(val);
        return v;
    }
    
    Value* constInt64(int64_t val) {
        Value* v = addValue(Opcode::Const64, Type::Int64);
        v->setConstInt64(val);
        return v;
    }
    
    Value* constFloat(float val) {
        Value* v = addValue(Opcode::ConstFloat, Type::Float);
        v->setConstFloat(val);
        return v;
    }
    
    Value* constDouble(double val) {
        Value* v = addValue(Opcode::ConstDouble, Type::Double);
        v->setConstDouble(val);
        return v;
    }
    
    // Control flow helpers
    void addJump(BasicBlock* from, BasicBlock* to) {
        Value* jump = addValue(Opcode::Jump, Type::Void);
        from->appendValue(jump);
        from->addSuccessor(to);
        to->addPredecessor(from);
    }
    
    void addBranch(BasicBlock* from, Value* cond, BasicBlock* thenB, BasicBlock* elseB) {
        Value* branch = addValue(Opcode::Branch, Type::Void, cond);
        from->appendValue(branch);
        from->addSuccessor(thenB);
        from->addSuccessor(elseB);
        thenB->addPredecessor(from);
        elseB->addPredecessor(from);
    }
    
    void addReturn(BasicBlock* from, Value* result = nullptr) {
        Value* ret = result ? addValue(Opcode::Return, Type::Void, result)
                           : addValue(Opcode::Return, Type::Void);
        from->appendValue(ret);
    }
    
    // Procedure metadata
    Type resultType() const { return resultType_; }
    void setResultType(Type t) { resultType_ = t; }
    
    uint32_t numParameters() const { return numParams_; }
    void setNumParameters(uint32_t n) { numParams_ = n; }
    
    // Remove dead values
    void removeDeadValues() {
        for (auto& block : blocks_) {
            std::vector<Value*> live;
            for (Value* v : block->values()) {
                if (!v->isDead()) {
                    live.push_back(v);
                }
            }
            // Rebuild block values (simplified)
        }
    }
    
private:
    std::vector<std::unique_ptr<BasicBlock>> blocks_;
    std::vector<std::unique_ptr<Value>> values_;
    
    Type resultType_ = Type::Void;
    uint32_t numParams_ = 0;
};

} // namespace Zepra::B3
