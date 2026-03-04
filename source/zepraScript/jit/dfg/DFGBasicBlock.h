/**
 * @file DFGBasicBlock.h
 * @brief Data Flow Graph Basic Block
 * 
 * Represents a sequence of nodes with single entry and exit points.
 */

#pragma once

#include "DFGNode.h"
#include <vector>
#include <memory>

namespace Zepra::JIT::DFG {

class BasicBlock {
public:
    BasicBlock(uint32_t id) : id_(id) {}
    
    uint32_t id() const { return id_; }
    
    // Nodes in execution order
    const std::vector<Node*>& nodes() const { return nodes_; }
    
    // Add node to block
    void append(Node* node) {
        nodes_.push_back(node);
    }
    
    // Accessors
    Node* first() const { return nodes_.empty() ? nullptr : nodes_.front(); }
    Node* last() const { return nodes_.empty() ? nullptr : nodes_.back(); }
    
    // Control Flow Graph Links
    std::vector<BasicBlock*> predecessors;
    std::vector<BasicBlock*> successors;
    
    // Dominator Tree
    BasicBlock* immediateDominator = nullptr;
    std::vector<BasicBlock*> dominatedBlocks;

private:
    uint32_t id_;
    std::vector<Node*> nodes_;
};

} // namespace Zepra::JIT::DFG
