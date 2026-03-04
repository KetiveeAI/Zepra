/**
 * @file DFGBasicBlock.h
 * @brief DFG Basic Block representation
 * 
 * A basic block is a sequence of operations with single entry and exit.
 * Contains phi nodes at the beginning for SSA merge points.
 */

#pragma once

#include "DFGValue.h"
#include <vector>
#include <memory>

namespace Zepra::DFG {

// Forward declarations
class Graph;

// =============================================================================
// Basic Block
// =============================================================================

class BasicBlock {
public:
    explicit BasicBlock(uint32_t index)
        : index_(index)
        , loopDepth_(0)
        , flags_(0) {}
    
    ~BasicBlock() = default;
    
    // =========================================================================
    // Identity
    // =========================================================================
    
    uint32_t index() const { return index_; }
    void setIndex(uint32_t idx) { index_ = idx; }
    
    // =========================================================================
    // Phi Nodes
    // =========================================================================
    
    void addPhi(Value* phi) {
        phi->setBlock(this);
        phis_.push_back(phi);
    }
    
    const std::vector<Value*>& phis() const { return phis_; }
    std::vector<Value*>& phis() { return phis_; }
    
    uint32_t numPhis() const { return static_cast<uint32_t>(phis_.size()); }
    
    // =========================================================================
    // Values (non-phi instructions)
    // =========================================================================
    
    void appendValue(Value* v) {
        v->setBlock(this);
        values_.push_back(v);
    }
    
    void insertValueBefore(Value* newVal, Value* before) {
        newVal->setBlock(this);
        for (auto it = values_.begin(); it != values_.end(); ++it) {
            if (*it == before) {
                values_.insert(it, newVal);
                return;
            }
        }
        values_.push_back(newVal);
    }
    
    void removeValue(Value* v) {
        for (auto it = values_.begin(); it != values_.end(); ++it) {
            if (*it == v) {
                values_.erase(it);
                return;
            }
        }
    }
    
    const std::vector<Value*>& values() const { return values_; }
    std::vector<Value*>& values() { return values_; }
    
    uint32_t numValues() const { return static_cast<uint32_t>(values_.size()); }
    
    Value* lastValue() const {
        return values_.empty() ? nullptr : values_.back();
    }
    
    Value* terminator() const {
        Value* last = lastValue();
        if (last && isTerminator(last->opcode())) {
            return last;
        }
        return nullptr;
    }
    
    // =========================================================================
    // Control Flow Graph
    // =========================================================================
    
    void addPredecessor(BasicBlock* pred) {
        for (BasicBlock* p : predecessors_) {
            if (p == pred) return;
        }
        predecessors_.push_back(pred);
    }
    
    void addSuccessor(BasicBlock* succ) {
        for (BasicBlock* s : successors_) {
            if (s == succ) return;
        }
        successors_.push_back(succ);
        succ->addPredecessor(this);
    }
    
    void removePredecessor(BasicBlock* pred) {
        for (auto it = predecessors_.begin(); it != predecessors_.end(); ++it) {
            if (*it == pred) {
                predecessors_.erase(it);
                return;
            }
        }
    }
    
    void removeSuccessor(BasicBlock* succ) {
        for (auto it = successors_.begin(); it != successors_.end(); ++it) {
            if (*it == succ) {
                successors_.erase(it);
                succ->removePredecessor(this);
                return;
            }
        }
    }
    
    const std::vector<BasicBlock*>& predecessors() const { return predecessors_; }
    const std::vector<BasicBlock*>& successors() const { return successors_; }
    
    uint32_t numPredecessors() const { return static_cast<uint32_t>(predecessors_.size()); }
    uint32_t numSuccessors() const { return static_cast<uint32_t>(successors_.size()); }
    
    bool hasSinglePredecessor() const { return predecessors_.size() == 1; }
    bool hasSingleSuccessor() const { return successors_.size() == 1; }
    
    BasicBlock* singlePredecessor() const {
        return hasSinglePredecessor() ? predecessors_[0] : nullptr;
    }
    
    BasicBlock* singleSuccessor() const {
        return hasSingleSuccessor() ? successors_[0] : nullptr;
    }
    
    // =========================================================================
    // Loop Information
    // =========================================================================
    
    uint32_t loopDepth() const { return loopDepth_; }
    void setLoopDepth(uint32_t depth) { loopDepth_ = depth; }
    
    bool isLoopHeader() const { return (flags_ & LoopHeader) != 0; }
    void setLoopHeader(bool v) {
        if (v) flags_ |= LoopHeader;
        else flags_ &= ~LoopHeader;
    }
    
    // =========================================================================
    // Flags
    // =========================================================================
    
    enum Flag : uint32_t {
        Visited = 1 << 0,
        LoopHeader = 1 << 1,
        Unreachable = 1 << 2,
    };
    
    bool hasFlag(Flag f) const { return (flags_ & f) != 0; }
    void setFlag(Flag f) { flags_ |= f; }
    void clearFlag(Flag f) { flags_ &= ~f; }
    void clearAllFlags() { flags_ = 0; }
    
    // =========================================================================
    // Iteration helpers
    // =========================================================================
    
    // Iterate over all values (phis first, then regular values)
    template<typename Func>
    void forEachValue(Func&& fn) {
        for (Value* v : phis_) fn(v);
        for (Value* v : values_) fn(v);
    }
    
    template<typename Func>
    void forEachValue(Func&& fn) const {
        for (Value* v : phis_) fn(v);
        for (Value* v : values_) fn(v);
    }
    
private:
    uint32_t index_;
    uint32_t loopDepth_;
    uint32_t flags_;
    
    std::vector<Value*> phis_;
    std::vector<Value*> values_;
    std::vector<BasicBlock*> predecessors_;
    std::vector<BasicBlock*> successors_;
};

} // namespace Zepra::DFG
