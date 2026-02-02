/**
 * @file B3BasicBlock.h
 * @brief B3 Basic Block
 * 
 * Control flow graph building block for B3 IR.
 */

#pragma once

#include "B3Value.h"
#include <vector>
#include <cstdint>

namespace Zepra::B3 {

class Procedure;

class BasicBlock {
public:
    explicit BasicBlock(uint32_t index) : index_(index) {}
    
    uint32_t index() const { return index_; }
    double frequency() const { return frequency_; }
    void setFrequency(double f) { frequency_ = f; }
    
    // Values in this block
    const std::vector<Value*>& values() const { return values_; }
    void appendValue(Value* v) {
        values_.push_back(v);
        v->setBlock(this);
    }
    void removeValue(Value* v) {
        for (auto it = values_.begin(); it != values_.end(); ++it) {
            if (*it == v) { values_.erase(it); return; }
        }
    }
    
    // Control flow
    const std::vector<BasicBlock*>& predecessors() const { return predecessors_; }
    const std::vector<BasicBlock*>& successors() const { return successors_; }
    
    void addPredecessor(BasicBlock* b) { predecessors_.push_back(b); }
    void addSuccessor(BasicBlock* b) { successors_.push_back(b); }
    
    uint32_t numPredecessors() const { return static_cast<uint32_t>(predecessors_.size()); }
    uint32_t numSuccessors() const { return static_cast<uint32_t>(successors_.size()); }
    
    // Terminator
    Value* terminator() {
        if (values_.empty()) return nullptr;
        Value* last = values_.back();
        return isTerminal(last->opcode()) ? last : nullptr;
    }
    
    // Last value
    Value* lastValue() { return values_.empty() ? nullptr : values_.back(); }
    
private:
    uint32_t index_;
    double frequency_ = 1.0;
    
    std::vector<Value*> values_;
    std::vector<BasicBlock*> predecessors_;
    std::vector<BasicBlock*> successors_;
};

} // namespace Zepra::B3
